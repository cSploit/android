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

#include "../common/classes/alloc.h"
#include "../jrd/GarbageCollector.h"
#include "../jrd/tra.h"

using namespace Jrd;
using namespace Firebird;

namespace Jrd {


void GarbageCollector::RelationData::clear()
{
	TranData::ConstAccessor accessor(&m_tranData);
	if (accessor.getFirst())
	{
		do
		{
			delete accessor.current()->second;
		} while (accessor.getNext());
	}

	m_tranData.clear();
}


void GarbageCollector::RelationData::addPage(const ULONG pageno, const TraNumber tranid)
{
	// look if given page number is already set at given tx bitmap
	PageBitmap* bm = NULL;
	const bool bmExists = m_tranData.get(tranid, bm);
	if (bm && bm->test(pageno))
		return;

	// search for given page at other transactions bitmaps
	// if found at older tx - we are done, just return
	// if found at younger tx - clear it as page should be set at oldest tx (our)
	TranData::ConstAccessor accessor(&m_tranData);
	if (accessor.getFirst())
	{
		do
		{
			const TranBitMap* item = accessor.current();
			if (item->first <= tranid)
			{
				if (item->second->test(pageno))
					return;
			}
			else
			{
				if (item->second->clear(pageno))
					break;
			}
		} while(accessor.getNext());
	}

	// add page to our tx bitmap
	PBM_SET(&m_pool, &bm, pageno);

	if (!bmExists)
		m_tranData.put(tranid, bm);
}


void GarbageCollector::RelationData::getPageBitmap(const TraNumber oldest_snapshot, PageBitmap** sbm)
{
	TranData::Accessor accessor(&m_tranData);
	while (accessor.getFirst())
	{
		TranBitMap* item = accessor.current();

		if (item->first >= oldest_snapshot)
			break;

		PageBitmap* bm_tran = item->second;
		PageBitmap** bm_or = PageBitmap::bit_or(sbm, &bm_tran);

		if (*bm_or == item->second)
		{
			bm_tran = *sbm;
			*sbm = item->second;
			item->second = bm_tran;
		}

		delete item->second;

		m_tranData.remove(item->first);
	}
}


void GarbageCollector::RelationData::swept(const TraNumber oldest_snapshot)
{
	TranData::Accessor accessor(&m_tranData);
	while (accessor.getFirst())
	{
		TranBitMap* item = accessor.current();

		if (item->first >= oldest_snapshot)
			break;

		delete item->second;
		m_tranData.remove(item->first);
	}
}


TraNumber GarbageCollector::RelationData::minTranID() const
{
	TranData::ConstAccessor accessor(&m_tranData);
	if (accessor.getFirst())
		return accessor.current()->first;

	return MAX_TRA_NUMBER;
}


GarbageCollector::~GarbageCollector()
{
	SyncLockGuard exGuard(&m_sync, SYNC_EXCLUSIVE, "GarbageCollector::~GarbageCollector");

	for (size_t pos = 0; pos < m_relations.getCount(); pos++)
	{
		Sync sync(&m_relations[pos]->m_sync, "GarbageCollector::~GarbageCollector");
		sync.lock(SYNC_EXCLUSIVE);
		sync.unlock();
		delete m_relations[pos];
	}

	m_relations.clear();
}


void GarbageCollector::addPage(const USHORT relID, const ULONG pageno, const TraNumber tranid)
{
	Sync syncGC(&m_sync, "GarbageCollector::addPage");
	RelationData* relData = getRelData(syncGC, relID, true);

	SyncLockGuard syncData(&relData->m_sync, SYNC_EXCLUSIVE, "GarbageCollector::addPage");
	syncGC.unlock();

	relData->addPage(pageno, tranid);
}


bool GarbageCollector::getPageBitmap(const TraNumber oldest_snapshot, USHORT &relID, PageBitmap **sbm)
{
	*sbm = NULL;
	SyncLockGuard shGuard(&m_sync, SYNC_EXCLUSIVE, "GarbageCollector::getPageBitmap");

	if (m_relations.isEmpty())
	{
		m_nextRelID = 0;
		return false;
	}

	size_t pos;
	if (!m_relations.find(m_nextRelID, pos) && (pos == m_relations.getCount()))
		pos = 0;

	for (; pos < m_relations.getCount(); pos++)
	{
		RelationData* relData = m_relations[pos];
		SyncLockGuard syncData(&relData->m_sync, SYNC_EXCLUSIVE, "GarbageCollector::getPageBitmap");

		relData->getPageBitmap(oldest_snapshot, sbm);

		if (*sbm)
		{
			relID = relData->getRelID();
			m_nextRelID = relID + 1;
			return true;
		}
	}

	m_nextRelID = 0;
	return false;
}


void GarbageCollector::removeRelation(const USHORT relID)
{
	Sync syncGC(&m_sync, "GarbageCollector::removeRelation");
	syncGC.lock(SYNC_EXCLUSIVE);

	size_t pos;
	if (!m_relations.find(relID, pos))
		return;

	RelationData* relData = m_relations[pos];
	Sync syncData(&relData->m_sync, "GarbageCollector::removeRelation");
	syncData.lock(SYNC_EXCLUSIVE);

	m_relations.remove(pos);
	syncGC.unlock();

	syncData.unlock();
	delete relData;
}


void GarbageCollector::sweptRelation(const TraNumber oldest_snapshot, const USHORT relID)
{
	Sync syncGC(&m_sync, "GarbageCollector::sweptRelation");

	RelationData* relData = getRelData(syncGC, relID, false);
	if (relData)
	{
		SyncLockGuard syncData(&relData->m_sync, SYNC_EXCLUSIVE, "GarbageCollector::sweptRelation");

		syncGC.unlock();
		relData->swept(oldest_snapshot);
	}
}


TraNumber GarbageCollector::minTranID(const USHORT relID)
{
	Sync syncGC(&m_sync, "GarbageCollector::minTranID");

	RelationData* relData = getRelData(syncGC, relID, false);
	if (relData)
	{
		SyncLockGuard syncData(&relData->m_sync, SYNC_SHARED, "GarbageCollector::minTranID");

		syncGC.unlock();
		return relData->minTranID();
	}

	return MAX_TRA_NUMBER;
}


GarbageCollector::RelationData* GarbageCollector::getRelData(Sync &sync, const USHORT relID,
	bool allowCreate)
{
	size_t pos;

	sync.lock(SYNC_SHARED);
	if (!m_relations.find(relID, pos))
	{
		if (!allowCreate)
			return NULL;

		sync.unlock();
		sync.lock(SYNC_EXCLUSIVE);
		if (!m_relations.find(relID, pos))
		{
			m_relations.insert(pos, FB_NEW(m_pool) RelationData(m_pool, relID));
			sync.downgrade(SYNC_SHARED);
		}
	}

	return m_relations[pos];
}


} // namespace Jrd
