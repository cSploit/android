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
 *  Copyright (c) 2005 Vlad Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/Relation.h"

#include "../jrd/tra.h"
#include "../jrd/btr_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/idx_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/vio_debug.h"

using namespace Jrd;

/// jrd_rel

RelationPages* jrd_rel::getPagesInternal(thread_db* tdbb, TraNumber tran, bool allocPages)
{
	if (tdbb->tdbb_flags & TDBB_use_db_page_space)
		return &rel_pages_base;

	Jrd::Attachment* attachment = tdbb->getAttachment();
	Database* dbb = tdbb->getDatabase();

	ULONG inst_id;
	// Vlad asked for this compile-time check to make sure we can contain a txn number here
	typedef int RangeCheck[sizeof(inst_id) >= sizeof(TraNumber)];

	if (rel_flags & REL_temp_tran)
	{
		if (tran > 0 && tran != MAX_TRA_NUMBER) //if (tran > 0)
			inst_id = tran;
		else if (tdbb->tdbb_temp_traid)
			inst_id = tdbb->tdbb_temp_traid;
		else if (tdbb->getTransaction())
			inst_id = tdbb->getTransaction()->tra_number;
		else // called without transaction, maybe from OPT or CMP ?
			return &rel_pages_base;
	}
	else
		inst_id = PAG_attachment_id(tdbb);

	if (!rel_pages_inst)
		rel_pages_inst = FB_NEW(*rel_pool) RelationPagesInstances(*rel_pool);

	size_t pos;
	if (!rel_pages_inst->find(inst_id, pos))
	{
		if (!allocPages)
			return 0;

		RelationPages* newPages = rel_pages_free;
		if (!newPages)
		{
			const size_t BULK_ALLOC = 8;

			RelationPages* allocatedPages = newPages =
				FB_NEW(*rel_pool) RelationPages[BULK_ALLOC];

			rel_pages_free = ++allocatedPages;
			for (size_t i = 1; i < BULK_ALLOC - 1; i++, allocatedPages++)
				allocatedPages->rel_next_free = allocatedPages + 1;
		}
		else
		{
			rel_pages_free = newPages->rel_next_free;
			newPages->rel_next_free = 0;
		}

		fb_assert(newPages->useCount == 0);

		newPages->addRef();
		newPages->rel_instance_id = inst_id;
		newPages->rel_pg_space_id = dbb->dbb_page_manager.getTempPageSpaceID(tdbb);
		rel_pages_inst->add(newPages);

		// create primary pointer page and index root page
		DPM_create_relation_pages(tdbb, this, newPages);

#ifdef VIO_DEBUG
		VIO_trace(DEBUG_WRITES,
			"jrd_rel::getPages inst %"ULONGFORMAT", ppp %"SLONGFORMAT", irp %"SLONGFORMAT", addr 0x%x\n",
			newPages->rel_instance_id,
			newPages->rel_pages ? (*newPages->rel_pages)[0] : 0,
			newPages->rel_index_root,
			newPages);
#endif

		// create indexes
		MemoryPool* pool = tdbb->getDefaultPool();
		const bool poolCreated = !pool;

		if (poolCreated)
			pool = dbb->createPool();
		Jrd::ContextPoolHolder context(tdbb, pool);

		jrd_tra* idxTran = tdbb->getTransaction();
		if (!idxTran)
			idxTran = attachment->getSysTransaction();

		IndexDescAlloc* indices = NULL;
		// read indices from "base" index root page
		const USHORT idx_count = BTR_all(tdbb, this, &indices, &rel_pages_base);

		const index_desc* const end = indices->items + idx_count;
		for (index_desc* idx = indices->items; idx < end; idx++)
		{
			Firebird::MetaName idx_name;
			MET_lookup_index(tdbb, idx_name, this->rel_name, idx->idx_id + 1);

			idx->idx_root = 0;
			SelectivityList selectivity(*pool);
			IDX_create_index(tdbb, this, idx, idx_name.c_str(), NULL, idxTran, selectivity);

#ifdef VIO_DEBUG
			VIO_trace(DEBUG_WRITES,
				"jrd_rel::getPages inst %"ULONGFORMAT", irp %"SLONGFORMAT", idx %u, idx_root %"SLONGFORMAT", addr 0x%x\n",
				newPages->rel_instance_id,
				newPages->rel_index_root,
				idx->idx_id,
				idx->idx_root,
				newPages);
#endif
		}

		if (poolCreated)
			dbb->deletePool(pool);
		delete indices;

		return newPages;
	}

	RelationPages* pages = (*rel_pages_inst)[pos];
	fb_assert(pages->rel_instance_id == inst_id);
	return pages;
}

bool jrd_rel::delPages(thread_db* tdbb, TraNumber tran, RelationPages* aPages)
{
	RelationPages* pages = aPages ? aPages : getPages(tdbb, tran, false);
	if (!pages || !pages->rel_instance_id)
		return false;

	//fb_assert((tran <= 0) || ((tran > 0) && (pages->rel_instance_id == tran)));
	fb_assert(tran == 0 || tran == MAX_TRA_NUMBER ||
		(tran > 0 && pages->rel_instance_id == tran));

	fb_assert(pages->useCount > 0);

	if (--pages->useCount)
		return false;

#ifdef VIO_DEBUG
	VIO_trace(DEBUG_WRITES,
		"jrd_rel::delPages inst %"ULONGFORMAT", ppp %"SLONGFORMAT", irp %"SLONGFORMAT", addr 0x%x\n",
		pages->rel_instance_id,
		pages->rel_pages ? (*pages->rel_pages)[0] : 0,
		pages->rel_index_root,
		pages);
#endif

	size_t pos;
#ifdef DEV_BUILD
	const bool found =
#endif
		rel_pages_inst->find(pages->rel_instance_id, pos);
	fb_assert(found && ((*rel_pages_inst)[pos] == pages) );

	rel_pages_inst->remove(pos);

	if (pages->rel_index_root) {
		IDX_delete_indices(tdbb, this, pages);
	}

	if (pages->rel_pages) {
		DPM_delete_relation_pages(tdbb, this, pages);
	}

	pages->free(rel_pages_free);
	return true;
}

void jrd_rel::getRelLockKey(thread_db* tdbb, UCHAR* key)
{
	const ULONG val = rel_id;
	memcpy(key, &val, sizeof(ULONG));
	key += sizeof(ULONG);

	const ULONG inst_id = getPages(tdbb)->rel_instance_id;
	memcpy(key, &inst_id, sizeof(ULONG));
}

USHORT jrd_rel::getRelLockKeyLength() const
{
	return sizeof(ULONG) + sizeof(SLONG);
}

void jrd_rel::cleanUp()
{
	delete rel_pages_inst;
	rel_pages_inst = NULL;
}


void jrd_rel::fillPagesSnapshot(RelPagesSnapshot& snapshot, const bool attachmentOnly)
{
	if (rel_pages_inst)
	{
		for (size_t i = 0; i < rel_pages_inst->getCount(); i++)
		{
			RelationPages* relPages = (*rel_pages_inst)[i];

			if (!attachmentOnly)
			{
				snapshot.add(relPages);
				relPages->addRef();
			}
			else if ((rel_flags & REL_temp_conn) &&
				(ULONG) PAG_attachment_id(snapshot.spt_tdbb) == relPages->rel_instance_id)
			{
				snapshot.add(relPages);
				relPages->addRef();
			}
			else if (rel_flags & REL_temp_tran)
			{
				const jrd_tra* tran = snapshot.spt_tdbb->getAttachment()->att_transactions;
				for (; tran; tran = tran->tra_next)
				{
					if (tran->tra_number == relPages->rel_instance_id)
					{
						snapshot.add(relPages);
						relPages->addRef();
					}
				}
			}
		}
	}
	else
		snapshot.add(&rel_pages_base);
}

void jrd_rel::RelPagesSnapshot::clear()
{
#ifdef DEV_BUILD
	thread_db* tdbb = NULL;
	SET_TDBB(tdbb);
	fb_assert(tdbb == spt_tdbb);
#endif

	for (size_t i = 0; i < getCount(); i++)
	{
		RelationPages* relPages = (*this)[i];
		(*this)[i] = NULL;

		spt_relation->delPages(spt_tdbb, MAX_TRA_NUMBER, relPages);
	}

	inherited::clear();
}

bool jrd_rel::hasTriggers() const
{
	typedef const trig_vec* ctv;
	ctv trigs[6] = // non-const array, don't want optimization tricks by the compiler.
	{
		rel_pre_erase,
		rel_post_erase,
		rel_pre_modify,
		rel_post_modify,
		rel_pre_store,
		rel_post_store
	};

	for (int i = 0; i < 6; ++i)
	{
		if (trigs[i] && trigs[i]->getCount())
			return true;
	}
	return false;
}

void RelationPages::free(RelationPages*& nextFree)
{
	rel_next_free = nextFree;
	nextFree = this;

	if (rel_pages)
		rel_pages->clear();

	rel_index_root = rel_data_pages = 0;
	rel_slot_space = rel_data_space = 0;
	rel_instance_id = 0;
}
