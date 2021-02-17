/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		parse.cpp
 *	DESCRIPTION:	BLR parser
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "../jrd/ibase.h"
#include "../remote/remote.h"
#include "../jrd/align.h"
#include "../common/gdsassert.h"
#include "../remote/parse_proto.h"

#if !defined(DEV_BUILD) || (defined(DEV_BUILD) && defined(WIN_NT))
#include "../yvalve/gds_proto.h"	// gds__log()
#endif


static RMessage* parse_error(rem_fmt* format, RMessage* mesage);


RMessage* PARSE_messages(const UCHAR* blr, size_t blr_length)
{
/**************************************
 *
 *	P A R S E _ m e s s a g e s
 *
 **************************************
 *
 * Functional description
 *	Parse the messages of a blr request.  For each message, allocate
 *	a message (msg) and a format (fmt) block.  Return the number of
 *	messages found.  If an error occurs, return -1;
 *
 **************************************/

	if (blr_length < 2)
		return (RMessage*) -1;
	blr_length -= 2;

	const SSHORT version = *blr++;
	if (version != blr_version4 && version != blr_version5)
		return (RMessage*) -1;

	if (*blr++ != blr_begin)
		return 0;

	RMessage* message = NULL;
	ULONG net_length = 0;

	while (*blr++ == blr_message)
	{
		if (blr_length < 4)
			return parse_error(0, message);
		blr_length -= 4;

		const USHORT msg_number = *blr++;
		USHORT count = *blr++;
		count += (*blr++) << 8;
		rem_fmt* const format = new rem_fmt(count);
#ifdef DEBUG_REMOTE_MEMORY
		printf("PARSE_messages            allocate format  %x\n", format);
#endif
		ULONG offset = 0;
		for (dsc* desc = format->fmt_desc.begin(); count; --count, ++desc)
		{
			if (blr_length-- == 0)
				return parse_error(format, message);

			USHORT align = 4;
			switch (*blr++)
			{
			case blr_text:
				if (blr_length < 2)
					return parse_error(format, message);
				blr_length -= 2;
				desc->dsc_dtype = dtype_text;
				desc->dsc_length = *blr++;
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_text];
				break;

			case blr_varying:
				if (blr_length < 2)
					return parse_error(format, message);
				blr_length -= 2;
				desc->dsc_dtype = dtype_varying;
				desc->dsc_length = *blr++ + sizeof(SSHORT);
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_varying];
				break;

			case blr_cstring:
				if (blr_length < 2)
					return parse_error(format, message);
				blr_length -= 2;
				desc->dsc_dtype = dtype_cstring;
				desc->dsc_length = *blr++;
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_cstring];
				break;

				// Parse the tagged blr types correctly

			case blr_text2:
				if (blr_length < 4)
					return parse_error(format, message);
				blr_length -= 4;
				desc->dsc_dtype = dtype_text;
				desc->dsc_scale = *blr++;
				desc->dsc_scale += (*blr++) << 8;
				desc->dsc_length = *blr++;
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_text];
				break;

			case blr_varying2:
				if (blr_length < 4)
					return parse_error(format, message);
				blr_length -= 4;
				desc->dsc_dtype = dtype_varying;
				desc->dsc_scale = *blr++;
				desc->dsc_scale += (*blr++) << 8;
				desc->dsc_length = *blr++ + sizeof(SSHORT);
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_varying];
				break;

			case blr_cstring2:
				if (blr_length < 4)
					return parse_error(format, message);
				blr_length -= 4;
				desc->dsc_dtype = dtype_cstring;
				desc->dsc_scale = *blr++;
				desc->dsc_scale += (*blr++) << 8;
				desc->dsc_length = *blr++;
				desc->dsc_length += (*blr++) << 8;
				align = type_alignments[dtype_cstring];
				break;

			case blr_short:
				if (blr_length-- == 0)
					return parse_error(format, message);
				desc->dsc_dtype = dtype_short;
				desc->dsc_length = sizeof(SSHORT);
				desc->dsc_scale = *blr++;
				align = type_alignments[dtype_short];
				break;

			case blr_long:
				if (blr_length-- == 0)
					return parse_error(format, message);
				desc->dsc_dtype = dtype_long;
				desc->dsc_length = sizeof(SLONG);
				desc->dsc_scale = *blr++;
				align = type_alignments[dtype_long];
				break;

			case blr_int64:
				if (blr_length-- == 0)
					return parse_error(format, message);
				desc->dsc_dtype = dtype_int64;
				desc->dsc_length = sizeof(SINT64);
				desc->dsc_scale = *blr++;
				align = type_alignments[dtype_int64];
				break;

			case blr_quad:
				if (blr_length-- == 0)
					return parse_error(format, message);
				desc->dsc_dtype = dtype_quad;
				desc->dsc_length = sizeof(SLONG) * 2;
				desc->dsc_scale = *blr++;
				align = type_alignments[dtype_quad];
				break;

			case blr_float:
				desc->dsc_dtype = dtype_real;
				desc->dsc_length = sizeof(float);
				align = type_alignments[dtype_real];
				break;

			case blr_double:
			case blr_d_float:
				desc->dsc_dtype = dtype_double;
				desc->dsc_length = sizeof(double);
				align = type_alignments[dtype_double];
				break;

			// this case cannot occur as switch paramater is char and blr_blob
            // is 261. blob_ids are actually passed around as blr_quad.

		    //case blr_blob:
			//	desc->dsc_dtype = dtype_blob;
			//	desc->dsc_length = sizeof (SLONG) * 2;
			//	align = type_alignments [dtype_blob];
			//	break;

			case blr_blob2:
				{
					if (blr_length < 4)
						return parse_error(format, message);
					blr_length -= 4;
					desc->dsc_dtype = dtype_blob;
					desc->dsc_length = sizeof(SLONG) * 2;
					desc->dsc_sub_type = *blr++;
					desc->dsc_sub_type += (*blr++) << 8;

					USHORT textType = *blr++;
					textType += (*blr++) << 8;
					desc->setTextType(textType);

					align = type_alignments[dtype_blob];
				}
				break;

			case blr_timestamp:
				desc->dsc_dtype = dtype_timestamp;
				desc->dsc_length = sizeof(SLONG) * 2;
				align = type_alignments[dtype_timestamp];
				break;

			case blr_sql_date:
				desc->dsc_dtype = dtype_sql_date;
				desc->dsc_length = sizeof(SLONG);
				align = type_alignments[dtype_sql_date];
				break;

			case blr_sql_time:
				desc->dsc_dtype = dtype_sql_time;
				desc->dsc_length = sizeof(ULONG);
				align = type_alignments[dtype_sql_time];
				break;

			case blr_bool:
				desc->dsc_dtype = dtype_boolean;
				desc->dsc_length = sizeof(UCHAR);
				align = type_alignments[dtype_boolean];
				break;

			default:
				fb_assert(FALSE);
				return parse_error(format, message);
			}
			if (desc->dsc_dtype == dtype_varying)
				net_length += 4 + ((desc->dsc_length - 2 + 3) & ~3);
			else
				net_length += (desc->dsc_length + 3) & ~3;
			if (align > 1)
				offset = FB_ALIGN(offset, align);
			desc->dsc_address = (UCHAR*) (IPTR) offset;
			offset += desc->dsc_length;
		}
		format->fmt_length = offset;
		format->fmt_net_length = net_length;
		RMessage* next = new RMessage(format->fmt_length);
#ifdef DEBUG_REMOTE_MEMORY
		printf("PARSE_messages            allocate message %x\n", next);
#endif
		next->msg_next = message;
		message = next;
		message->msg_address = reinterpret_cast<UCHAR*>(format);
		message->msg_number = msg_number;
	}

	return message;
}


static RMessage* parse_error(rem_fmt* format, RMessage* message)
{
	delete format;
	for (RMessage* next = message; next; next = message)
	{
		message = message->msg_next;
		delete next->msg_address;
		delete next;
	}
	return (RMessage*) -1;
}
