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

// Stream.h: interface for the Stream class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
#define AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER >= 1000

#include "../common/classes/fb_string.h"

START_NAMESPACE

class Stream : public Firebird::GlobalStorage
{
public:
	enum { FIXED_SEGMENT_SIZE = 1024 };

	struct Segment
    {
		int		length;
		char	*address;
		Segment	*next;
		char	tail [FIXED_SEGMENT_SIZE];
	};

	explicit Stream (int minSegmentSize = FIXED_SEGMENT_SIZE);
	virtual ~Stream();

	int compare (const Stream *stream) const;
	void truncate (int length);
	void format (const char *pattern, ...);
	void			putSegment (int length, const unsigned short *chars);
	void			putSegment (Stream *stream);
	virtual void	putSegment (const char *string);
	virtual void	putSegment (int length, const char *address, bool copy);
	void			putCharacter (char c);

	virtual void	setSegment (Segment *segment, int length, void* address);
	virtual int		getSegment (int offset, int len, void* ptr) const;
	virtual int		getSegment (int offset, int len, void *ptr, char delimiter) const;
	void*			getSegment (int offset);
	const void*		getSegment (int offset) const;
	int				getSegmentLength(int offset) const;

	Firebird::string	getFBString() const;
	virtual char*	getString();
	void			clear();
	virtual int		getLength() const;
	virtual char*	alloc (int length);

	Segment*		allocSegment (int tail);
	void			setMinSegment (int length);

	int		totalLength;
	//int		useCount;
	Segment	*segments; // used by StreamSegment::setStream
private:
	int		minSegment;
	int		currentLength;
	int		decompressedLength;
	bool	copyFlag;
	Segment	first;
	Segment *current;
};

END_NAMESPACE

#endif // !defined(AFX_STREAM_H__02AD6A53_A433_11D2_AB5B_0000C01D2301__INCLUDED_)
