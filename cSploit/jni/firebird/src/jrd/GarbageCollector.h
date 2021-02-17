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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2011 Vlad Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_GARBAGE_COLLECTOR_H
#define JRD_GARBAGE_COLLECTOR_H

#include "firebird.h"
#include "../common/classes/array.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/SyncObject.h"
#include "../jrd/sbm.h"


namespace Jrd {

class Database;
class Savepoint;

class GarbageCollector
{
public:
	GarbageCollector(MemoryPool& p, Database* dbb)
	  : m_pool(p), m_relations(m_pool), m_nextRelID(0)
	{}

	~GarbageCollector();

	void addPage(const USHORT relID, const ULONG pageno, const TraNumber tranid);
	bool getPageBitmap(const TraNumber oldest_snapshot, USHORT &relID, PageBitmap** sbm);
	void removeRelation(const USHORT relID);
	void sweptRelation(const TraNumber oldest_snapshot, const USHORT relID);

	TraNumber minTranID(const USHORT relID);

private:
	typedef Firebird::Pair<Firebird::NonPooled<TraNumber, PageBitmap*> > TranBitMap;
	typedef Firebird::GenericMap<TranBitMap> TranData;

	class RelationData
	{
	public:
		explicit RelationData(MemoryPool& p, USHORT relID)
			: m_pool(p), m_tranData(p), m_relID(relID)
		{}

		~RelationData()
		{
			clear();
		}

		void addPage(const ULONG pageno, const TraNumber tranid);
		void getPageBitmap(const TraNumber oldest_snapshot, PageBitmap** sbm);
		void swept(const TraNumber oldest_snapshot);
		TraNumber minTranID() const;

		USHORT getRelID() const
		{
			return m_relID;
		}

		static inline const USHORT generate(const RelationData* item)
		{
			return item->m_relID;
		}

		void clear();

		Firebird::MemoryPool& m_pool;
		Firebird::SyncObject m_sync;
		TranData m_tranData;
		USHORT m_relID;
	};

	typedef	Firebird::SortedArray<
				RelationData*,
				Firebird::EmptyStorage<RelationData*>,
				USHORT,
				RelationData> RelGarbageArray;

	RelationData* getRelData(Firebird::Sync& sync, const USHORT relID, bool allowCreate);

	Firebird::MemoryPool& m_pool;
	Firebird::SyncObject m_sync;
	RelGarbageArray m_relations;
	USHORT m_nextRelID;
};

} // namespace Jrd

#endif	// JRD_GARBAGE_COLLECTOR_H
