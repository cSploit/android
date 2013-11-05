/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pag.h
 *	DESCRIPTION:	Page interface definitions
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

/*
 * Modified by: Patrick J. P. Griffin
 * Date: 11/29/2000
 * Problem:   Bug 116733 Too many generators corrupt database.
 *            DPM_gen_id was not calculating page and offset correctly.
 * Change:    Add pgc_gpg, number of generators per page,
 *            for use in DPM_gen_id.
 */


#ifndef JRD_PAG_H
#define JRD_PAG_H

#include "../include/fb_blk.h"
#include "../common/classes/array.h"
#include "../jrd/ods.h"

namespace Jrd {

/* Page control block -- used by PAG to keep track of critical
   constants */
/**
class PageControl : public pool_alloc<type_pgc>
{
    public:
	SLONG pgc_high_water;		// Lowest PIP with space
	SLONG pgc_ppp;				// Pages per pip
	SLONG pgc_pip;				// First pointer page
	ULONG pgc_bytes;			// Number of bytes of bit in PIP
	ULONG pgc_tpt;				// Transactions per TIP
	ULONG pgc_gpg;				// Generators per generator page
};
**/

// page spaces below TEMP_PAGE_SPACE is regular database pages
// TEMP_PAGE_SPACE and page spaces above TEMP_PAGE_SPACE is temporary pages
const USHORT DB_PAGE_SPACE		= 1;
const USHORT TEMP_PAGE_SPACE	= 256;

class jrd_file;
class Database;
class thread_db;

class PageSpace : public pool_alloc<type_PageSpace>
{
public:
	PageSpace(USHORT aPageSpaceID)
	{
		pageSpaceID = aPageSpaceID;
		pipHighWater = 0;
		ppFirst = 0;
		file = 0;
		maxPageNumber = 0;
	}

	~PageSpace();

	USHORT pageSpaceID;
	SLONG pipHighWater;		// Lowest PIP with space
	SLONG ppFirst;			// First pointer page

	jrd_file*	file;

	inline bool isTemporary() const
	{
		return (pageSpaceID >= TEMP_PAGE_SPACE);
	}

	static inline SLONG generate(const void* , const PageSpace* Item)
	{
		return Item->pageSpaceID;
	}

	// how many pages allocated
	ULONG actAlloc(const USHORT pageSize);
	static ULONG actAlloc(const Database* dbb);

	// number of last allocated page
	ULONG maxAlloc(const USHORT pageSize);
	static ULONG maxAlloc(const Database* dbb);

	// extend page space
	bool extend(thread_db*, const ULONG);

private:
	ULONG	maxPageNumber;
};

class PageManager : public pool_alloc<type_PageManager>
{
public:
	PageManager(Firebird::MemoryPool& aPool) :
		pageSpaces(aPool),
		pool(aPool)
	{
		pagesPerPIP = 0;
		bytesBitPIP = 0;
		transPerTIP = 0;
		gensPerPage = 0;

		dbPageSpace = addPageSpace(DB_PAGE_SPACE);
		// addPageSpace(TEMP_PAGE_SPACE);
	}

	~PageManager()
	{
		for (size_t i = pageSpaces.getCount(); i > 0; --i)
		{
			PageSpace* pageSpace = pageSpaces[i - 1];
			pageSpaces.remove(i - 1);
			delete pageSpace;
		}
	}

	PageSpace* addPageSpace(const USHORT pageSpaceID);
	PageSpace* findPageSpace(const USHORT pageSpaceID) const;
	void delPageSpace(const USHORT pageSpaceID);

	USHORT getTempPageSpaceID(thread_db* tdbb);

	void closeAll();
	void releaseLocks();

	SLONG pagesPerPIP;			// Pages per pip
	ULONG bytesBitPIP;			// Number of bytes of bit in PIP
	SLONG transPerTIP;			// Transactions per TIP
	ULONG gensPerPage;			// Generators per generator page
	PageSpace* dbPageSpace;		// database page space

private:
	typedef Firebird::SortedArray<PageSpace*, Firebird::EmptyStorage<PageSpace*>,
		USHORT, PageSpace> PageSpaceArray;

	PageSpaceArray pageSpaces;
	Firebird::MemoryPool& pool;
};

class PageNumber
{
public:
	inline PageNumber(const USHORT aPageSpace, const SLONG aPageNum)
	{
		pageSpaceID = aPageSpace;
		pageNum	= aPageNum;
	}
	/*
	inline PageNumber(const SLONG aPageNum) {
		pageSpaceID = DB_PAGE_SPACE;
		pageNum	= aPageNum;
	}
	*/
	inline PageNumber(const PageNumber& from)
	{
		pageSpaceID = from.pageSpaceID;
		pageNum	= from.pageNum;
	}

	inline SLONG getPageNum() const
	{
		return pageNum;
	}

	inline SLONG getPageSpaceID() const
	{
		return pageSpaceID;
	}

	inline SLONG setPageSpaceID(const USHORT aPageSpaceID)
	{
		pageSpaceID = aPageSpaceID;
		return pageSpaceID;
	}

	inline bool isTemporary() const
	{
		return (pageSpaceID >= TEMP_PAGE_SPACE);
	}

	inline static SSHORT getLockLen()
	{
		return sizeof(SLONG) + sizeof(ULONG);
	}

	inline void getLockStr(UCHAR* str) const
	{
		memcpy(str, &pageNum, sizeof(SLONG));
		str += sizeof(SLONG);

		const ULONG val = pageSpaceID;
		memcpy(str, &val, sizeof(ULONG));
	}

	inline PageNumber& operator=(const PageNumber& from)
	{
		pageSpaceID = from.pageSpaceID;
		pageNum	= from.pageNum;
		return *this;
	}

	inline SLONG operator=(const SLONG from)
	{
		pageNum	= from;
		return pageNum;
	}

	inline bool operator==(const PageNumber& other) const
	{
		return (pageNum == other.pageNum) &&
			(pageSpaceID == other.pageSpaceID);
	}

	inline bool operator!=(const PageNumber& other) const
	{
		return !(*this == other);
	}

	inline bool operator>(const PageNumber& other) const
	{
		return (pageSpaceID > other.pageSpaceID) ||
			(pageSpaceID == other.pageSpaceID) && (pageNum > other.pageNum);
	}

	inline bool operator>=(const PageNumber& other) const
	{
		return (pageSpaceID > other.pageSpaceID) ||
			(pageSpaceID == other.pageSpaceID) && (pageNum >= other.pageNum);
	}

	inline bool operator<(const PageNumber& other) const
	{
		return !(*this >= other);
	}

	inline bool operator<=(const PageNumber& other) const
	{
		return !(*this > other);
	}

	/*
	inline operator SLONG() const
	{
		return pageNum;
	}
	*/
private:
	SLONG	pageNum;
	USHORT	pageSpaceID;
};

const PageNumber ZERO_PAGE_NUMBER(0, 0);
const PageNumber HEADER_PAGE_NUMBER(DB_PAGE_SPACE, HEADER_PAGE);
const PageNumber LOG_PAGE_NUMBER(DB_PAGE_SPACE, LOG_PAGE);

} //namespace Jrd

#endif /* JRD_PAG_H */
