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

// Stream.cpp: implementation of the Stream class.
//
//////////////////////////////////////////////////////////////////////

#include <memory.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>
#include "firebird.h"
#include "../common/classes/alloc.h"
#include "Stream.h"
#include "StreamSegment.h"


// CVC: The logic in this module seems to overwrite constant params passed into the
// address field of the Segment structure. This includes literal strings (see Element.cpp).

#ifndef MAX
#define MAX(a, b)			((a > b) ? a : b)
#define MIN(a, b)			((a < b) ? a : b)
#endif

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Stream::Stream(int minSegmentSize)
{
	segments = NULL;
	current = NULL;
	totalLength = 0;
	minSegment = minSegmentSize;
	copyFlag = true;
}

Stream::~Stream()
{
	clear();
}

void Stream::putCharacter(char c)
{
	if (!segments || current->length >= currentLength)
		allocSegment (MAX (100, minSegment));

	current->address [current->length] = c;
	++current->length;
	++totalLength;
}

void Stream::putSegment(int length, const char *ptr, bool copy)
{
#ifdef ENGINE
	ASSERT (length >= 0);
#endif
	if (length == 0)
		return;

	const char *address = ptr;
	totalLength += length;

	if (!segments)
	{
		if (copyFlag = copy)
		{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			memcpy (current->address, address, length);
		}
		else
		{
			//copyFlag = copy;
			current = segments = &first;
			current->length = length;
			current->address = (char*) address;
			current->next = NULL;
		}
	}
	else if (copyFlag)
	{
		int l = currentLength - current->length;
		if (l > 0)
		{
			const int l2 = MIN (l, length);
			memcpy (current->address + current->length, address, l2);
			current->length += l2;
			length -= l2;
			address += l2;
		}
		if (length)
		{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			memcpy (current->address, address, length);
		}
	}
	else
	{
		allocSegment (0);
		current->address = (char*) address;
		current->length = length;
	}
}


int Stream::getSegment(int offset, int len, void* ptr) const
{
	int n = 0;
	int length = len;
	char* address = static_cast<char*>(ptr);

	for (const Segment *segment = segments; segment; n += segment->length, segment = segment->next)
	{
		if (n + segment->length >= offset)
		{
			const int off = offset - n;
			const int l = MIN (length, segment->length - off);
			memcpy (address, segment->address + off, l);
			address += l;
			length -= l;
			offset += l;
			if (!length)
				break;
		}
	}

	return len - length;
}

void Stream::setSegment(Segment * segment, int length, void* address)
{
	segment->length = length;
	totalLength += length;

	if (copyFlag)
	{
		segment->address = new char [length];
		memcpy (segment->address, address, length);
	}
	else
		segment->address = (char*) address;
}


void Stream::setMinSegment(int length)
{
	minSegment = length;
}

Stream::Segment* Stream::allocSegment(int tail)
{
	fb_assert(tail >= 0);

	Segment *segment;
	int length = tail;

	if (!current && tail <= FIXED_SEGMENT_SIZE)
	{
		segment = &first;
		length = FIXED_SEGMENT_SIZE;
	}
	else
	{
		// CVC: What do we do here in the release build if tail is smaller than FIXED_SEGMENT_SIZE?
		fb_assert(tail >= FIXED_SEGMENT_SIZE);
		segment = (Segment*) new char [sizeof(Segment) + tail - FIXED_SEGMENT_SIZE];
	}

	segment->address = segment->tail;
	segment->next = NULL;
	segment->length = 0;
	currentLength = length;

	if (current)
	{
		current->next = segment;
		current = segment;
	}
	else
		segments = current = segment;

	return segment;
}

int Stream::getSegment(int offset, int len, void * ptr, char delimiter) const
{
	int n = 0;
	int length = len;
	char *address = static_cast<char*>(ptr);

	for (const Segment *segment = segments; segment; n += segment->length, segment = segment->next)
	{
		if (n + segment->length >= offset)
		{
			const int off = offset - n;
			const int l = MIN (length, segment->length - off);
			const char *p = segment->address + off;
			for (const char *end = p + l; p < end;)
			{
				const char c = *address++ = *p++;
				--length;
				if (c == delimiter)
					return len - length;
			}
			if (!length)
				break;
		}
	}

	return len - length;
}

void Stream::putSegment(const char * string)
{
	if (string [0])
		putSegment ((int) strlen (string), string, true);
}

char* Stream::getString()
{
	char* const string = new char [totalLength + 1];
	getSegment (0, totalLength, string);
	string [totalLength] = 0;

	return string;
}

#ifdef ENGINE
void Stream::compress(int length, const void * address)
{
	//printShorts ("Original data", (length + 1) / 2, (short*) address);
	Segment *segment = allocSegment (length + 5);
	short *q = (short*) segment->address;
	const short *p = (short*) address;
	const short* const end = p + (length + 1) / 2;
	const short* const yellow = end - 2;
	*q++ = length;

	while (p < end)
	{
		short *start = ++q;
		while (p < end && (p >= yellow || p[0] != p[1] || p[1] != p[2]))
			*q++ = *p++;
		int n = q - start;
		if (n)
			start [-1] = -n;
		else
			--q;
		if (p >= end)
			break;
		const short* start2 = p++;
		while (p < end && *p == *start2)
			++p;
		n = p - start2;
		*q++ = n;
		*q++ = *start2;
	}

	totalLength = segment->length = (char*) q - segment->address;
}

char* Stream::decompress(int tableId, int recordNumber)
{
	char *data;
	short *q, *limit;
	int run = 0;
	decompressedLength = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
	{
		if (segment->length == 0)
			continue;
		const short* p = (short*) segment->address;
		const short* end = (short*) (segment->address + segment->length);
		if (decompressedLength == 0)
		{
			decompressedLength = *p++;
			if (decompressedLength <= 0)
			{
				Log::log ("corrupted record, table %d, record %d, decompressed length %d\n",
						  tableId, recordNumber, decompressedLength);
				throw SQLEXCEPTION (RUNTIME_ERROR, "corrupted record, table %d, record %d, decompressed length %d",
									tableId, recordNumber, decompressedLength);
			}
			int len = ((decompressedLength + 1) / 2) * 2;
			//data = new char [len];
			data = ALLOCATE_RECORD (len);
			limit = (short*) (data + decompressedLength);
			if (decompressedLength & 1)
				++limit;
			q = (short*) data;
		}
		while (p < end)
		{
			const short n = *p++;
//#ifdef ENGINE
			if (n == 0 && run == 0)
			{
				Log::log ("corrupted record (zero run), table %d, record %d\n", tableId, recordNumber);
				printShorts ("Zero run", (segment->length + 1) / 2, (short*) segment->address);
				printChars ("Zero run", segment->length, segment->address);
			}
//#endif
			if (run > 0)
			{
				for (; run; --run)
					*q++ = n;
			}
			else if (run < 0)
			{
				*q++ = n;
				++run;
			}
			else
			{
				run = n;
				if (q + run > limit)
				{
//#ifdef ENGINE
					Log::log ("corrupted record (overrun), table %d, record %d\n", tableId, recordNumber);
					printShorts ("Compressed", (segment->length + 1)/2, (short*) segment->address);
					printChars ("Compressed", segment->length, segment->address);
//#endif
					if (q == limit)
						return data;
					throw SQLEXCEPTION (RUNTIME_ERROR, "corrupted record, table %d, record %d", tableId, recordNumber);
				}
			}
		}
	}

	//printShorts ("Decompressed", (decompressedLength + 1) / 2, (short*) data);
	return data;
}

void Stream::printShorts(const char * msg, int length, short * data)
{
	Log::debug ("%s", msg);

	for (int n = 0; n < length; ++n)
	{
		if (n % 10 == 0)
			Log::debug ("\n    ");
		Log::debug ("%d, ", data [n]);
	}

	Log::debug ("\n");
}

void Stream::printChars(const char * msg, int length, const char * data)
{
	Log::debug ("%s", msg);

	for (int n = 0; n < length; ++n)
	{
		if (n % 50 == 0)
			Log::debug ("\n    ");
		const char c = data [n];
		if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9'))
			putchar (c);
		else
			putchar ('.');
	}

	Log::debug ("\n");
}
#endif

void Stream::clear()
{
	Segment *segment;

	while (segment = segments)
	{
		segments = segment->next;
		if (segment != &first)
			delete[] (char*) segment;
	}

	current = NULL;
	totalLength = 0;
}

void Stream::putSegment(int length, const unsigned short *chars)
{
	if (length == 0)
		return;

	totalLength += length;
	const unsigned short *wc = chars;

	if (!segments)
	{
		allocSegment (MAX (length, minSegment));
		current->length = length;
	}
	else
	{
		int l = currentLength - current->length;
		if (l > 0)
		{
			const int l2 = MIN (l, length);
			char *p = current->address + current->length;
			for (int n = 0; n < l2; ++n)
				*p++ = (char) *wc++;
			//memcpy (current->address + current->length, address, l2);
			current->length += l2;
			length -= l2;
			//address += l2;
		}
		if (length)
		{
			allocSegment (MAX (length, minSegment));
			current->length = length;
			//memcpy (current->address, address, length);
		}
	}

	char *p = current->address;

	for (int n = 0; n < length; ++n)
		*p++ = (char) *wc++;

}

int Stream::getLength() const
{
	return totalLength;
}


int Stream::getSegmentLength(int offset) const
{
	int n = 0;

	for (const Segment *segment = segments; segment; segment = segment->next)
	{
		if (offset >= n && offset < n + segment->length)
			return n + segment->length - offset;
		n += segment->length;
	}

	return 0;
}

void* Stream::getSegment(int offset)
{
	int n = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
	{
		if (offset >= n && offset < n + segment->length)
			return segment->address + offset - n;
		n += segment->length;
	}

	return NULL;
}

const void* Stream::getSegment(int offset) const
{
	int n = 0;

	for (const Segment *segment = segments; segment; segment = segment->next)
	{
		if (offset >= n && offset < n + segment->length)
			return segment->address + offset - n;
		n += segment->length;
	}

	return NULL;
}

void Stream::putSegment(Stream * stream)
{
	/***
	for (Segment *segment = stream->segments; segment; segment = segment->next)
		putSegment (segment->length, segment->address, true);
	***/

	if (stream->totalLength == 0)
		return;

	StreamSegment seg(stream);

	if (current)
	{
		for (int len = currentLength - current->length; len && seg.available;)
		{
			const int l = MIN (len, seg.available);
			putSegment (l, seg.data, true);
			seg.advance (l);
			len -= l;
		}
	}

	if (seg.remaining)
		seg.copy (alloc (seg.remaining), seg.remaining);
}



Firebird::string Stream::getFBString() const
{
	Firebird::string string;
	char *p = string.getBuffer (totalLength);

	for (const Segment *segment = segments; segment; segment = segment->next)
	{
		memcpy (p, segment->address, segment->length);
		p += segment->length;
	}

	fb_assert(p - string.begin() == totalLength);

	return string;
}

char* Stream::alloc(int length)
{
	totalLength += length;

	if (!current || length > currentLength - current->length)
		allocSegment (length);

	char* const p = current->tail + current->length;
	current->length += length;

	return p;
}


void Stream::format(const char *pattern, ...)
{
	Firebird::string temp;

	va_list		args;
	va_start	(args, pattern);
	temp.vprintf(pattern, args);
	va_end(args);

	putSegment (temp.c_str());
}

void Stream::truncate(int length)
{
	int n = 0;

	for (Segment *segment = segments; segment; segment = segment->next)
	{
		if (length >= n && length < n + segment->length)
		{
			current = segment;
			current->length = length - n;
			totalLength = length;
			while (segment = current->next)
			{
				current->next = segment->next;
				delete[] (char*) segment; // Was deallocation bug
			}
			return;
		}
		n += segment->length;
	}

}

int Stream::compare(const Stream *stream) const
{
	for (int offset = 0;;)
	{
		const int length1 = getSegmentLength(offset);
		const int length2 = stream->getSegmentLength(offset);
		if (length1 == 0)
		{
			if (length2)
				return -1;

			return 0;
		}
		if (length2 == 0)
			return 1;
		const int length = MIN (length1, length2);
		const char *p1 = (const char*) getSegment (offset);
		const char *p2 = (const char*) stream->getSegment (offset);
		for (const char *end = p1 + length; p1 < end;)
		{
			int n = *p1++ - *p2++;
			if (n)
				return n;
		}
		offset += length;
	}
}
