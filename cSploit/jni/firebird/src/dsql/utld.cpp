/*
 *	PROGRAM:	Dynamic SQL runtime support
 *	MODULE:		utld.cpp
 *	DESCRIPTION:	Utility routines for DSQL
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
 *
 * 21 Nov 01 - Ann Harrison - Turn off the code in parse_sqlda that
 *    decides that two statements are the same based on their message
 *    descriptions because it misleads some code in remote/interface.c
 *    and causes problems when two statements are prepared.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "firebird.h"
#include <stdio.h>
#include <string.h>
#include "../dsql/dsql.h"
#include "../dsql/sqlda.h"
#include "../jrd/ibase.h"
#include "../jrd/align.h"
#include "../jrd/constants.h"
#include "../dsql/utld_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/classes/init.h"

using namespace Jrd;

/**

	UTLD_char_length_to_byte_length

	@brief  Return max byte length necessary for a specified character length string

	@param lengthInChars
	@param maxBytesPerChar

**/
USHORT UTLD_char_length_to_byte_length(USHORT lengthInChars, USHORT maxBytesPerChar, USHORT overhead)
{
	return MIN(((MAX_COLUMN_SIZE - overhead) / maxBytesPerChar) * maxBytesPerChar,
			   (ULONG) lengthInChars * maxBytesPerChar);
}


ISC_STATUS UTLD_copy_status(const ISC_STATUS* from, ISC_STATUS* to)
{
/**************************************
 *
 *	c o p y _ s t a t u s
 *
 **************************************
 *
 * Functional description
 *	Copy a status vector.
 *
 **************************************/
	const ISC_STATUS status = from[1];

	const ISC_STATUS* const end = from + ISC_STATUS_LENGTH;
	while (from < end)
		*to++ = *from++;

	return status;
}
