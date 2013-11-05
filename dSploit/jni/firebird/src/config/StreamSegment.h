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

// StreamSegment.h: interface for the StreamSegment class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_STREAMSEGMENT_H__11469FBB_A55A_4BB1_A96B_D95A4C75F0C7__INCLUDED_)
#define AFX_STREAMSEGMENT_H__11469FBB_A55A_4BB1_A96B_D95A4C75F0C7__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/alloc.h"
#include "Stream.h"

START_NAMESPACE


class StreamSegment : public Firebird::GlobalStorage
{
public:
	explicit StreamSegment(Stream *stream);
	virtual ~StreamSegment();

	char* copy (void *target, int length);
	void advance (int size);
	void advance();
	void setStream (Stream *stream);

private:
	friend class Stream; // for "available" and "remaining"
	int		available;
	int		remaining;
	char*	data;
	Stream::Segment* segment;
};

END_NAMESPACE

#endif // !defined(AFX_STREAMSEGMENT_H__11469FBB_A55A_4BB1_A96B_D95A4C75F0C7__INCLUDED_)
