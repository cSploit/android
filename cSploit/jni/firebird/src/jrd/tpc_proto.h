/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		tpc_proto.h
 *	DESCRIPTION:	Prototype Header file for tpc.cpp
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

#ifndef JRD_TPC_PROTO_H
#define JRD_TPC_PROTO_H

#include "../common/classes/array.h"
#include "../common/classes/SyncObject.h"

namespace Ods {
struct tx_inv_page;
};

namespace Jrd {

class Database;
class thread_db;

class TipCache
{
public:
	explicit TipCache(Database* dbb);
	~TipCache();

	int cacheState(thread_db*, TraNumber number);
	TraNumber findLimbo(thread_db* tdbb, TraNumber minNumber, TraNumber maxNumber);
	void initializeTpc(thread_db*, TraNumber number);
	void setState(TraNumber number, SSHORT state);
	int snapshotState(thread_db*, TraNumber number);
	void updateCache(const Ods::tx_inv_page* tip_page, ULONG sequence);

private:
	class TxPage : public pool_alloc_rpt<SCHAR, type_tpc>
	{
	public:
		TraNumber tpc_base;			// id of first transaction in this block
		UCHAR tpc_transactions[1];	// two bits per transaction

		static const TraNumber generate(const TxPage* item)
		{
			return item->tpc_base;
		}
	};

	TxPage* allocTxPage(TraNumber base);
	TraNumber cacheTransactions(thread_db* tdbb, TraNumber oldest);
	int extendCache(thread_db* tdbb, TraNumber number);
	void clearCache();

	Database* m_dbb;
	Firebird::SyncObject m_sync;
	Firebird::SortedArray<TxPage*, Firebird::EmptyStorage<TxPage*>, TraNumber, TxPage> m_cache;
};


inline int TPC_cache_state(thread_db* tdbb, TraNumber number)
{
	 return tdbb->getDatabase()->dbb_tip_cache->cacheState(tdbb, number);
}

inline TraNumber TPC_find_limbo(thread_db* tdbb, TraNumber minNumber, TraNumber maxNumber)
{
	 return tdbb->getDatabase()->dbb_tip_cache->findLimbo(tdbb, minNumber, maxNumber);
}

inline void TPC_initialize_tpc(thread_db* tdbb, TraNumber number)
{
	 tdbb->getDatabase()->dbb_tip_cache->initializeTpc(tdbb, number);
}

inline void TPC_set_state(thread_db* tdbb, TraNumber number, SSHORT state)
{
	 tdbb->getDatabase()->dbb_tip_cache->setState(number, state);
}

inline int TPC_snapshot_state(thread_db* tdbb, TraNumber number)
{
	 return tdbb->getDatabase()->dbb_tip_cache->snapshotState(tdbb, number);
}

inline void TPC_update_cache(thread_db* tdbb, const Ods::tx_inv_page* tip_page, ULONG sequence)
{
	 tdbb->getDatabase()->dbb_tip_cache->updateCache(tip_page, sequence);
}

} // namespace Jrd

#endif // JRD_TPC_PROTO_H
