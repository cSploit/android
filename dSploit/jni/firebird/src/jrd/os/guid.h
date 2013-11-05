/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		guid.h
 *	DESCRIPTION:	Portable GUID definition
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 *
 */

#ifndef GUID_H
#define GUID_H

#include <stdlib.h>
#include <stdio.h>
#include "fb_types.h"

const int GUID_BUFF_SIZE = 39;
const int GUID_BODY_SIZE = 36;

const char* const GUID_LEGACY_FORMAT =
	"{%04hX%04hX-%04hX-%04hX-%04hX-%04hX%04hX%04hX}";
const char* const GUID_NEW_FORMAT =
	"{%02hX%02hX%02hX%02hX-%02hX%02hX-%02hX%02hX-%02hX%02hX-%02hX%02hX%02hX%02hX%02hX%02hX}";

struct FB_GUID
{
	union
	{
		USHORT data[8];

		struct	// Compatible with Win32 GUID struct layout.
		{
			ULONG data1;
			USHORT data2;
			USHORT data3;
			UCHAR data4[8];
		};
	};
};

void GenerateRandomBytes(void* buffer, size_t size);
void GenerateGuid(FB_GUID* guid);

// These functions receive buffers of at least GUID_BUFF_SIZE length.
// Warning: they are BROKEN in little-endian and should not be used on new code.

inline void GuidToString(char* buffer, const FB_GUID* guid)
{
	sprintf(buffer, GUID_LEGACY_FORMAT,
		guid->data[0], guid->data[1], guid->data[2], guid->data[3],
		guid->data[4], guid->data[5], guid->data[6], guid->data[7]);
}

inline void StringToGuid(FB_GUID* guid, const char* buffer)
{
	sscanf(buffer, GUID_LEGACY_FORMAT,
		&guid->data[0], &guid->data[1], &guid->data[2], &guid->data[3],
		&guid->data[4], &guid->data[5], &guid->data[6], &guid->data[7]);
}

#endif
