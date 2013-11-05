/*
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
 *  The Original Code was created by Vlad Horsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Vlad Horsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef DEBUG_INTERFACE_H
#define DEBUG_INTERFACE_H

#include "firebird.h"
#include "../jrd/jrd.h"

#include "../jrd/blb.h"

namespace Firebird {

class MapBlrToSrcItem
{
public:
	USHORT mbs_offset;
	USHORT mbs_src_line;
	USHORT mbs_src_col;

	static USHORT generate(const void*, const MapBlrToSrcItem& Item)
	{ return Item.mbs_offset; }
};

typedef Firebird::SortedArray<
	MapBlrToSrcItem,
	Firebird::EmptyStorage<MapBlrToSrcItem>,
	USHORT,
	MapBlrToSrcItem> MapBlrToSrc;

typedef GenericMap<Pair<Right<USHORT, MetaName> > > MapVarIndexToName;

struct ArgumentInfo
{
	ArgumentInfo(UCHAR aType, USHORT aIndex)
		: type(aType),
		  index(aIndex)
	{
	}

	ArgumentInfo()
		: type(0),
		  index(0)
	{
	}

	UCHAR type;
	USHORT index;

	bool operator >(const ArgumentInfo& x) const
	{
		if (type == x.type)
			return index > x.index;

		return type > x.type;
	}
};

typedef GenericMap<Pair<Right<ArgumentInfo, MetaName> > > MapArgumentInfoToName;

struct DbgInfo
{
	explicit DbgInfo(MemoryPool& p)
		: blrToSrc(p),
		  varIndexToName(p),
		  argInfoToName(p)
	{
	}

	DbgInfo()
	{
	}

	void clear()
	{
		blrToSrc.clear();
		varIndexToName.clear();
		argInfoToName.clear();
	}

	MapBlrToSrc blrToSrc;					// mapping between blr offsets and source text position
	MapVarIndexToName varIndexToName;		// mapping between variable index and name
	MapArgumentInfoToName argInfoToName;	// mapping between argument info (type, index) and name
};

} // namespace Firebird

void DBG_parse_debug_info(Jrd::thread_db*, Jrd::bid*, Firebird::DbgInfo&);
void DBG_parse_debug_info(USHORT, const UCHAR*, Firebird::DbgInfo&);

#endif // DEBUG_INTERFACE_H
