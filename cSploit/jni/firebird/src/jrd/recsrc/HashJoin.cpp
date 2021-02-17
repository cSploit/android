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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Dmitry Yemanov <dimitr@firebirdsql.org>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../jrd/jrd.h"
#include "../jrd/btr.h"
#include "../jrd/req.h"
#include "../jrd/intl.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/evl_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/Collation.h"

#include "RecordSource.h"

#include <stdlib.h>

using namespace Firebird;
using namespace Jrd;

// ----------------------
// Data access: hash join
// ----------------------

namespace
{
	typedef int (*qsort_compare_callback)(const void* a1, const void* a2, void* arg);

	struct qsort_ctx_data
	{
	  void* arg;
	  qsort_compare_callback compare;
	};

#if defined(WIN_NT) || defined(DARWIN) || defined(FREEBSD)
	int qsort_ctx_arg_swap(void* arg, const void* a1, const void* a2)
	{
	  struct qsort_ctx_data* ss = (struct qsort_ctx_data*) arg;
	  return (ss->compare)(a1, a2, ss->arg);
	}
#endif

#define USE_QSORT_CTX

	void qsort_ctx(void* base, size_t count, size_t width, qsort_compare_callback compare, void* arg)
	{
#if defined(LINUX) && !defined(__BIONIC__)
		qsort_r(base, count, width, compare, arg);
#elif defined(WIN_NT)
		struct qsort_ctx_data tmp = {arg, compare};
		qsort_s(base, count, width, &qsort_ctx_arg_swap, &tmp);
#elif defined(DARWIN) || defined(FREEBSD)
		struct qsort_ctx_data tmp = {arg, compare};
		qsort_r(base, count, width, &tmp, &qsort_ctx_arg_swap);
#else
#undef USE_QSORT_CTX
#endif
	}

} // namespace

static const size_t HASH_SIZE = 1009;
static const size_t COLLISION_PREALLOCATE_SIZE = 32;			// 256 KB
static const size_t KEYBUF_PREALLOCATE_SIZE = 64 * 1024; 		// 64 KB
static const size_t KEYBUF_SIZE_LIMIT = 1024 * 1024 * 1024; 	// 1 GB

class HashJoin::HashTable : public PermanentStorage
{
	struct Collision
	{
#ifdef USE_QSORT_CTX
		Collision()
			: offset(0), position(0)
		{}

		Collision(void* /*ctx*/, ULONG off, ULONG pos)
			: offset(off), position(pos)
		{}
#else
		Collision()
			: context(NULL), offset(0), position(0)
		{}

		Collision(void* ctx, ULONG off, ULONG pos)
			: context(ctx), offset(off), position(pos)
		{}

		void* context;
#endif

		ULONG offset;
		ULONG position;
	};

	class CollisionList
	{
		static const size_t INVALID_ITERATOR = size_t(~0);

	public:
		CollisionList(MemoryPool& pool, const KeyBuffer* keyBuffer, ULONG itemLength)
			: m_collisions(pool, COLLISION_PREALLOCATE_SIZE),
			  m_keyBuffer(keyBuffer), m_itemLength(itemLength), m_iterator(INVALID_ITERATOR)
		{}

#ifdef USE_QSORT_CTX
		static int compare(const void* p1, const void* p2, void* arg)
#else
		static int compare(const void* p1, const void* p2)
#endif
		{
			const Collision* const c1 = static_cast<const Collision*>(p1);
			const Collision* const c2 = static_cast<const Collision*>(p2);

#ifndef USE_QSORT_CTX
			fb_assert(c1->context == c2->context);
#endif

			const CollisionList* const collisions =
#ifdef USE_QSORT_CTX
				static_cast<const CollisionList*>(arg);
#else
				static_cast<const CollisionList*>(c1->context);
#endif
			const UCHAR* const baseAddress = collisions->m_keyBuffer->begin();

			const UCHAR* const ptr1 = baseAddress + c1->offset;
			const UCHAR* const ptr2 = baseAddress + c2->offset;

			return memcmp(ptr1, ptr2, collisions->m_itemLength);
		}

		void sort()
		{
			Collision* const base = m_collisions.begin();
			const size_t count = m_collisions.getCount();
			const size_t width = sizeof(Collision);

#ifdef USE_QSORT_CTX
			qsort_ctx(base, count, width, compare, this);
#else
			qsort(base, count, width, compare);
#endif
		}

		void add(const Collision& collision)
		{
			m_collisions.add(collision);
		}

		bool locate(ULONG length, const UCHAR* data)
		{
			const UCHAR* const ptr1 = data;
			const ULONG len1 = length;
			const ULONG len2 = m_itemLength;
			const ULONG minLen = MIN(len1, len2);

			size_t highBound = m_collisions.getCount(), lowBound = 0;
			const UCHAR* const baseAddress = m_keyBuffer->begin();

			while (highBound > lowBound)
			{
				const size_t temp = (highBound + lowBound) >> 1;
				const UCHAR* const ptr2 = baseAddress + m_collisions[temp].offset;
				const int result = memcmp(ptr1, ptr2, minLen);

				if (result > 0 || (!result && len1 > len2))
					lowBound = temp + 1;
				else
					highBound = temp;
			}

			if (highBound >= m_collisions.getCount() ||
				lowBound >= m_collisions.getCount())
			{
				m_iterator = INVALID_ITERATOR;
				return false;
			}

			const UCHAR* const ptr2 = baseAddress + m_collisions[lowBound].offset;

			if (memcmp(ptr1, ptr2, minLen))
			{
				m_iterator = INVALID_ITERATOR;
				return false;
			}

			m_iterator = lowBound;
			return true;
		}

		bool iterate(ULONG length, const UCHAR* data, ULONG& position)
		{
			if (m_iterator >= m_collisions.getCount())
				return false;

			const Collision& collision = m_collisions[m_iterator++];
			const UCHAR* const baseAddress = m_keyBuffer->begin();

			const UCHAR* const ptr1 = data;
			const ULONG len1 = length;
			const UCHAR* const ptr2 = baseAddress + collision.offset;
			const ULONG len2 = m_itemLength;
			const ULONG minLen = MIN(len1, len2);

			if (memcmp(ptr1, ptr2, len1))
			{
				m_iterator = INVALID_ITERATOR;
				return false;
			}

			position = collision.position;
			return true;
		}

	private:
		Array<Collision> m_collisions;
		const KeyBuffer* const m_keyBuffer;
		const ULONG m_itemLength;
		size_t m_iterator;
	};

public:
	HashTable(MemoryPool& pool, size_t streamCount, size_t tableSize = HASH_SIZE)
		: PermanentStorage(pool), m_streamCount(streamCount),
		  m_tableSize(tableSize), m_slot(0)
	{
		m_collisions = FB_NEW(pool) CollisionList*[streamCount * tableSize];
		memset(m_collisions, 0, streamCount * tableSize * sizeof(CollisionList*));
	}

	~HashTable()
	{
		for (size_t i = 0; i < m_streamCount * m_tableSize; i++)
			delete m_collisions[i];

		delete[] m_collisions;
	}

	size_t hash(ULONG length, const UCHAR* buffer) const
	{
		ULONG hash_value = 0;

		UCHAR* p = NULL;
		const UCHAR* q = buffer;
		for (size_t l = 0; l < length; l++)
		{
			if (!(l & 3))
				p = (UCHAR*) &hash_value;

			*p++ += *q++;
		}

		return (hash_value % m_tableSize);
	}

	void put(size_t stream,
			 ULONG keyLength, const KeyBuffer* keyBuffer,
			 ULONG offset, ULONG position)
	{
		const size_t slot = hash(keyLength, keyBuffer->begin() + offset);

		fb_assert(stream < m_streamCount);
		fb_assert(slot < m_tableSize);

		CollisionList* collisions = m_collisions[stream * m_tableSize + slot];

		if (!collisions)
		{
			collisions = FB_NEW(getPool()) CollisionList(getPool(), keyBuffer, keyLength);
			m_collisions[stream * m_tableSize + slot] = collisions;
		}

		collisions->add(Collision(collisions, offset, position));
	}

	bool setup(ULONG length, const UCHAR* data)
	{
		const size_t slot = hash(length, data);

		for (size_t i = 0; i < m_streamCount; i++)
		{
			CollisionList* const collisions = m_collisions[i * m_tableSize + slot];

			if (!collisions)
				return false;

			if (!collisions->locate(length, data))
				return false;
		}

		m_slot = slot;
		return true;
	}

	void reset(size_t stream, ULONG length, const UCHAR* data)
	{
		fb_assert(stream < m_streamCount);

		CollisionList* const collisions = m_collisions[stream * m_tableSize + m_slot];
		collisions->locate(length, data);
	}

	bool iterate(size_t stream, ULONG length, const UCHAR* data, ULONG& position)
	{
		fb_assert(stream < m_streamCount);

		CollisionList* const collisions = m_collisions[stream * m_tableSize + m_slot];
		return collisions->iterate(length, data, position);
	}

	void sort()
	{
		for (size_t i = 0; i < m_streamCount * m_tableSize; i++)
		{
			CollisionList* const collisions = m_collisions[i];

			if (collisions)
				collisions->sort();
		}
	}

private:
	const size_t m_streamCount;
	const size_t m_tableSize;
	CollisionList** m_collisions;
	size_t m_slot;
};


HashJoin::HashJoin(thread_db* tdbb, CompilerScratch* csb, size_t count,
				   RecordSource* const* args, NestValueArray* const* keys)
	: m_args(csb->csb_pool, count - 1)
{
	fb_assert(count >= 2);

	m_impure = CMP_impure(csb, sizeof(Impure));

	m_leader.source = args[0];
	m_leader.keys = keys[0];
	m_leader.keyLengths = FB_NEW(csb->csb_pool)
		KeyLengthArray(csb->csb_pool, m_leader.keys->getCount());
	m_leader.totalKeyLength = 0;

	for (size_t j = 0; j < m_leader.keys->getCount(); j++)
	{
		dsc desc;
		(*m_leader.keys)[j]->getDesc(tdbb, csb, &desc);

		USHORT keyLength = desc.dsc_length;

		if (IS_INTL_DATA(&desc))
			keyLength = INTL_key_length(tdbb, INTL_INDEX_TYPE(&desc), desc.dsc_length);
		else if (desc.dsc_dtype == dtype_varying)
			keyLength -= sizeof(USHORT);

		m_leader.keyLengths->add(keyLength);
		m_leader.totalKeyLength += keyLength;
	}

	for (size_t i = 1; i < count; i++)
	{
		RecordSource* const sub_rsb = args[i];
		fb_assert(sub_rsb);

		SubStream sub;
		sub.buffer = FB_NEW(csb->csb_pool) BufferedStream(csb, sub_rsb);
		sub.keys = keys[i];
		sub.keyLengths = FB_NEW(csb->csb_pool)
			KeyLengthArray(csb->csb_pool, sub.keys->getCount());
		sub.totalKeyLength = 0;

		for (size_t j = 0; j < sub.keys->getCount(); j++)
		{
			dsc desc;
			(*sub.keys)[j]->getDesc(tdbb, csb, &desc);

			USHORT keyLength = desc.dsc_length;

			if (IS_INTL_DATA(&desc))
				keyLength = INTL_key_length(tdbb, INTL_INDEX_TYPE(&desc), desc.dsc_length);
			else if (desc.dsc_dtype == dtype_varying)
				keyLength -= sizeof(USHORT);

			sub.keyLengths->add(keyLength);
			sub.totalKeyLength += keyLength;
		}

		m_args.add(sub);
	}
}

void HashJoin::open(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	impure->irsb_flags = irsb_open | irsb_mustread;

	delete impure->irsb_arg_buffer;
	delete impure->irsb_hash_table;
	delete[] impure->irsb_leader_buffer;
	delete[] impure->irsb_record_counts;

	MemoryPool& pool = *tdbb->getDefaultPool();

	const size_t argCount = m_args.getCount();

	impure->irsb_arg_buffer = FB_NEW(pool) KeyBuffer(pool, KEYBUF_PREALLOCATE_SIZE);
	impure->irsb_hash_table = FB_NEW(pool) HashTable(pool, argCount);
	impure->irsb_leader_buffer = FB_NEW(pool) UCHAR[m_leader.totalKeyLength];
	impure->irsb_record_counts = FB_NEW(pool) ULONG[argCount];

	for (size_t i = 0; i < argCount; i++)
	{
		// Read and cache the inner streams. While doing that,
		// hash the join condition values and populate hash tables.

		m_args[i].buffer->open(tdbb);

		ULONG& counter = impure->irsb_record_counts[i];
		counter = 0;

		while (m_args[i].buffer->getRecord(tdbb))
		{
			const ULONG offset = (ULONG) impure->irsb_arg_buffer->getCount();
			if (offset > KEYBUF_SIZE_LIMIT)
				status_exception::raise(Arg::Gds(isc_imp_exc) << Arg::Gds(isc_blktoobig));

			impure->irsb_arg_buffer->resize(offset + m_args[i].totalKeyLength);
			UCHAR* const keys = impure->irsb_arg_buffer->begin() + offset;

			computeKeys(tdbb, request, m_args[i], keys);
			impure->irsb_hash_table->put(i, m_args[i].totalKeyLength,
										 impure->irsb_arg_buffer,
										 offset, counter++);
		}

	}

	impure->irsb_hash_table->sort();

	m_leader.source->open(tdbb);
}

void HashJoin::close(thread_db* tdbb) const
{
	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	invalidateRecords(request);

	if (impure->irsb_flags & irsb_open)
	{
		impure->irsb_flags &= ~irsb_open;

		delete impure->irsb_hash_table;
		impure->irsb_hash_table = NULL;

		delete impure->irsb_arg_buffer;
		impure->irsb_arg_buffer = NULL;

		delete[] impure->irsb_leader_buffer;
		impure->irsb_leader_buffer = NULL;

		delete[] impure->irsb_record_counts;
		impure->irsb_record_counts = NULL;

		for (size_t i = 0; i < m_args.getCount(); i++)
			m_args[i].buffer->close(tdbb);

		m_leader.source->close(tdbb);
	}
}

bool HashJoin::getRecord(thread_db* tdbb) const
{
	if (--tdbb->tdbb_quantum < 0)
		JRD_reschedule(tdbb, 0, true);

	jrd_req* const request = tdbb->getRequest();
	Impure* const impure = request->getImpure<Impure>(m_impure);

	if (!(impure->irsb_flags & irsb_open))
		return false;

	const ULONG leaderKeyLength = m_leader.totalKeyLength;
	UCHAR* leaderKeyBuffer = impure->irsb_leader_buffer;

	while (true)
	{
		if (impure->irsb_flags & irsb_mustread)
		{
			// Fetch the record from the leading stream

			if (!m_leader.source->getRecord(tdbb))
				return false;

			// Compute and hash the comparison keys

			memset(leaderKeyBuffer, 0, leaderKeyLength);
			computeKeys(tdbb, request, m_leader, leaderKeyBuffer);

			// Ensure the every inner stream having matches for this hash slot.
			// Setup the hash table for the iteration through collisions.

			if (!impure->irsb_hash_table->setup(leaderKeyLength, leaderKeyBuffer))
				continue;

			impure->irsb_flags &= ~irsb_mustread;
			impure->irsb_flags |= irsb_first;
		}

		// Fetch collisions from the inner streams

		if (impure->irsb_flags & irsb_first)
		{
			bool found = true;

			for (size_t i = 0; i < m_args.getCount(); i++)
			{
				if (!fetchRecord(tdbb, impure, i))
				{
					found = false;
					break;
				}
			}

			if (!found)
			{
				impure->irsb_flags |= irsb_mustread;
				continue;
			}

			impure->irsb_flags &= ~irsb_first;
		}
		else if (!fetchRecord(tdbb, impure, m_args.getCount() - 1))
		{
			impure->irsb_flags |= irsb_mustread;
			continue;
		}

		break;
	}

	return true;
}

bool HashJoin::refetchRecord(thread_db* /*tdbb*/) const
{
	return true;
}

bool HashJoin::lockRecord(thread_db* /*tdbb*/) const
{
	status_exception::raise(Arg::Gds(isc_record_lock_not_supp));
	return false; // compiler silencer
}

void HashJoin::print(thread_db* tdbb, string& plan, bool detailed, unsigned level) const
{
	if (detailed)
	{
		plan += printIndent(++level) + "Hash Join (inner)";

		m_leader.source->print(tdbb, plan, true, level);

		for (size_t i = 0; i < m_args.getCount(); i++)
			m_args[i].source->print(tdbb, plan, true, level);
	}
	else
	{
		level++;
		plan += "HASH (";
		m_leader.source->print(tdbb, plan, false, level);
		plan += ", ";
		for (size_t i = 0; i < m_args.getCount(); i++)
		{
			if (i)
				plan += ", ";

			m_args[i].source->print(tdbb, plan, false, level);
		}
		plan += ")";
	}
}

void HashJoin::markRecursive()
{
	m_leader.source->markRecursive();

	for (size_t i = 0; i < m_args.getCount(); i++)
		m_args[i].source->markRecursive();
}

void HashJoin::findUsedStreams(StreamList& streams, bool expandAll) const
{
	m_leader.source->findUsedStreams(streams, expandAll);

	for (size_t i = 0; i < m_args.getCount(); i++)
		m_args[i].source->findUsedStreams(streams, expandAll);
}

void HashJoin::invalidateRecords(jrd_req* request) const
{
	m_leader.source->invalidateRecords(request);

	for (size_t i = 0; i < m_args.getCount(); i++)
		m_args[i].source->invalidateRecords(request);
}

void HashJoin::nullRecords(thread_db* tdbb) const
{
	m_leader.source->nullRecords(tdbb);

	for (size_t i = 0; i < m_args.getCount(); i++)
		m_args[i].source->nullRecords(tdbb);
}

void HashJoin::computeKeys(thread_db* tdbb, jrd_req* request,
						   const SubStream& sub, UCHAR* keyBuffer) const
{
	for (size_t i = 0; i < sub.keys->getCount(); i++)
	{
		const dsc* const desc = EVL_expr(tdbb, request, (*sub.keys)[i]);
		const USHORT keyLength = (*sub.keyLengths)[i];

		if (desc && !(request->req_flags & req_null))
		{
			USHORT length = desc->dsc_length;
			UCHAR* address = desc->dsc_address;

			MoveBuffer buffer;

			if (IS_INTL_DATA(desc))
			{
				// Convert the INTL string into the binary comparable form

				address = buffer.getBuffer(keyLength);

				dsc temp;
				temp.makeText(keyLength, ttype_sort_key, address);

				length = INTL_string_to_key(tdbb, INTL_INDEX_TYPE(desc),
											desc, &temp, INTL_KEY_UNIQUE);
			}
			else if (desc->dsc_dtype == dtype_varying)
			{
				length -= sizeof(USHORT);
				address += sizeof(USHORT);
			}

			fb_assert(length <= keyLength);

			memcpy(keyBuffer, address, length);
		}

		keyBuffer += keyLength;
	}
}

bool HashJoin::fetchRecord(thread_db* tdbb, Impure* impure, size_t stream) const
{
	HashTable* const hashTable = impure->irsb_hash_table;

	const BufferedStream* const arg = m_args[stream].buffer;

	const ULONG leaderKeyLength = m_leader.totalKeyLength;
	const UCHAR* leaderKeyBuffer = impure->irsb_leader_buffer;

	ULONG position;
	if (hashTable->iterate(stream, leaderKeyLength, leaderKeyBuffer, position))
	{
		arg->locate(tdbb, position);

		if (arg->getRecord(tdbb))
			return true;
	}

	while (true)
	{
		if (stream == 0 || !fetchRecord(tdbb, impure, stream - 1))
			return false;

		hashTable->reset(stream, leaderKeyLength, leaderKeyBuffer);

		if (hashTable->iterate(stream, leaderKeyLength, leaderKeyBuffer, position))
		{
			arg->locate(tdbb, position);

			if (arg->getRecord(tdbb))
				return true;
		}
	}
}
