/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

// StreamSegment.cpp: implementation of the StreamSegment class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "StreamSegment.h"
#include "Stream.h"

#ifndef MAX
//#define MAX(a, b)			((a > b) ? a : b)
#define MIN(a, b)			((a < b) ? a : b)
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

StreamSegment::StreamSegment(Stream *stream)
{
	setStream (stream);
}

StreamSegment::~StreamSegment()
{

}

void StreamSegment::setStream(Stream *stream)
{
	remaining = stream->totalLength;

	if (segment = stream->segments)
	{
		data = segment->address;
		available = segment->length;
	}
	else
	{
		data = NULL;
		available = 0;
	}
}

void StreamSegment::advance()
{
	advance (available);
}

void StreamSegment::advance(int size)
{
	for (int len = size; len;)
	{
		const int l = MIN (available, len);
		available -= l;
		remaining -= l;
		len -= size;
		if (remaining == 0)
			return;
		if (available)
			data += l;
		else
		{
			segment = segment->next;
			data = segment->address;
			available = segment->length;
		}
	}
}

char* StreamSegment::copy(void *target, int length)
{
	char* targ = static_cast<char*>(target);

	for (int len = length; len;)
	{
		const int l = MIN (len, available);
		memcpy (targ, data, l);
		targ += l;
		len -= l;
		advance (l);
	}

	return targ;
}
