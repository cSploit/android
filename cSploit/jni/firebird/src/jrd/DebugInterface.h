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

// Version 2 of the debug information replaces 16-bit values
// inside the fb_dbg_map_src2blr tag with 32-bit ones.
// Also, it introduces some new tags.
const UCHAR DBG_INFO_VERSION_1 = UCHAR(1);
const UCHAR DBG_INFO_VERSION_2 = UCHAR(2);
const UCHAR CURRENT_DBG_INFO_VERSION = DBG_INFO_VERSION_2;

namespace Firebird {

class MapBlrToSrcItem
{
public:
	ULONG mbs_offset;
	ULONG mbs_src_line;
	ULONG mbs_src_col;

	static ULONG generate(const MapBlrToSrcItem& Item)
	{ return Item.mbs_offset; }
};

typedef Firebird::SortedArray<
	MapBlrToSrcItem,
	Firebird::EmptyStorage<MapBlrToSrcItem>,
	ULONG,
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

struct DbgInfo : public PermanentStorage
{
	explicit DbgInfo(MemoryPool& p)
		: PermanentStorage(p),
		  blrToSrc(p),
		  varIndexToName(p),
		  argInfoToName(p),
		  subFuncs(p),
		  subProcs(p)
	{
	}

	~DbgInfo()
	{
		clear();
	}

	void clear()
	{
		blrToSrc.clear();
		varIndexToName.clear();
		argInfoToName.clear();

		{	// scope
			GenericMap<Pair<Left<MetaName, DbgInfo*> > >::Accessor accessor(&subFuncs);

			for (bool found = accessor.getFirst(); found; found = accessor.getNext())
				delete accessor.current()->second;

			subFuncs.clear();
		}

		{	// scope
			GenericMap<Pair<Left<MetaName, DbgInfo*> > >::Accessor accessor(&subProcs);

			for (bool found = accessor.getFirst(); found; found = accessor.getNext())
				delete accessor.current()->second;

			subProcs.clear();
		}
	}

	MapBlrToSrc blrToSrc;					// mapping between blr offsets and source text position
	MapVarIndexToName varIndexToName;		// mapping between variable index and name
	MapArgumentInfoToName argInfoToName;	// mapping between argument info (type, index) and name
	GenericMap<Pair<Left<MetaName, DbgInfo*> > > subFuncs;	// sub functions
	GenericMap<Pair<Left<MetaName, DbgInfo*> > > subProcs;	// sub procedures
};

} // namespace Firebird

void DBG_parse_debug_info(Jrd::thread_db*, Jrd::bid*, Firebird::DbgInfo&);
void DBG_parse_debug_info(ULONG, const UCHAR*, Firebird::DbgInfo&);

#endif // DEBUG_INTERFACE_H
