/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			status.cpp
 *	DESCRIPTION:	Status vector filling and parsing.
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Mike Nordell
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2001 Mike Nordell <tamlin at algonet.se>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "firebird.h"
#include <stdlib.h>
#include <string.h>
#include "../jrd/status.h"
#include "../jrd/gdsassert.h"
#include "gen/iberror.h"
#include "../jrd/gds_proto.h"


/** Check that we never overrun the status vector.  The status
 *  vector is 20 elements.  The maximum is 3 entries for a
 * type.  So check for 17 or less
 */

void PARSE_STATUS(const ISC_STATUS* status_vector, int &length, int &warning)
{
	warning = 0;
	length = 0;

    int i = 0;

	for (; status_vector[i] != isc_arg_end; i++, length++)
	{
		switch (status_vector[i])
		{
		case isc_arg_warning:
			if (!warning)
				warning = i;	// find the very first
			// fallthrought intended
		case isc_arg_gds:
		case isc_arg_string:
		case isc_arg_number:
		case isc_arg_interpreted:
		case isc_arg_sql_state:
		case isc_arg_vms:
		case isc_arg_unix:
		case isc_arg_win32:
			i++;
			length++;
			break;

		case isc_arg_cstring:
			i += 2;
			length += 2;
			break;

		default:
			fb_assert(FALSE);
			break;
		}
	}
	if (i) {
		length++;				// isc_arg_end is counted
	}
}
