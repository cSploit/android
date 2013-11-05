/*
 *	PROGRAM:	JRD International support
 *	MODULE:		Collation.h
 *	DESCRIPTION:	International text handling definitions
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
 *  2006.10.10 Adriano dos Santos Fernandes - refactored from intl_classes.h
 *
 */

#ifndef JRD_COLLATION_H
#define JRD_COLLATION_H

#include "TextType.h"


namespace Jrd {

class Lock;

class Collation : public TextType
{
public:
	static Collation* createInstance(MemoryPool& pool, TTYPE_ID id, texttype* tt, CharSet* cs);

protected:
	Collation(TTYPE_ID id, texttype *a_tt, CharSet* a_cs)
		: TextType(id, a_tt, a_cs),
		  useCount(0),
		  existenceLock(NULL),
		  obsolete(false)
	{
	}

public:
	virtual bool matches(MemoryPool& pool, const UCHAR* a, SLONG b, const UCHAR* c, SLONG d) = 0;
	virtual bool sleuthCheck(MemoryPool& pool, USHORT a, const UCHAR* b, SLONG c, const UCHAR* d, SLONG e) = 0;
	virtual ULONG sleuthMerge(MemoryPool& pool, const UCHAR* a, SLONG b, const UCHAR* c, SLONG d, UCHAR* e/*, SLONG f*/) = 0;

	virtual bool starts(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl) = 0;
	virtual PatternMatcher* createStartsMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl) = 0;

	virtual bool like(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length) = 0;
	virtual PatternMatcher* createLikeMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length) = 0;

	virtual bool similarTo(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length) = 0;
	virtual PatternMatcher* createSimilarToMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl, const UCHAR* escape, SLONG escape_length) = 0;

	virtual bool contains(MemoryPool& pool, const UCHAR* s, SLONG sl, const UCHAR* p, SLONG pl) = 0;
	virtual PatternMatcher* createContainsMatcher(MemoryPool& pool, const UCHAR* p, SLONG pl) = 0;

	void release();
	void destroy();
	void incUseCount(thread_db* tdbb);
	void decUseCount(thread_db* tdbb);

public:
	int useCount;
	Lock* existenceLock;
	bool obsolete;
};

}	// namespace Jrd


#endif	// JRD_COLLATION_H
