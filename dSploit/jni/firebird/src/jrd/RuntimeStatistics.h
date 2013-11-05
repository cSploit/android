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
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_RUNTIME_STATISTICS_H
#define JRD_RUNTIME_STATISTICS_H

#include "../common/classes/alloc.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/init.h"
#include "../common/classes/tree.h"
#include "../jrd/ntrace.h"

namespace Jrd {

// hvlad: what to use for relation's counters - tree or sorted array ?
// #define REL_COUNTS_TREE
// #define REL_COUNTS_PTR

class Database;

// Performance counters for individual table
struct RelationCounts
{
	SLONG rlc_relation_id;	// Relation ID
	SINT64 rlc_counter[DBB_max_rel_count];

#ifdef REL_COUNTS_PTR
	inline static const SLONG* generate(const void* /*sender*/, const RelationCounts* item)
	{
		return &item->rlc_relation_id;
	}
#else
	inline static const SLONG& generate(const void* /*sender*/, const RelationCounts& item)
	{
		return item.rlc_relation_id;
	}
#endif
};

#if defined(REL_COUNTS_TREE)
typedef Firebird::BePlusTree<RelationCounts, SLONG, Firebird::MemoryPool, RelationCounts> RelCounters;
#elif defined(REL_COUNTS_PTR)
typedef Firebird::PointersArray<RelationCounts, Firebird::EmptyStorage<RelationCounts>,
	SLONG, RelationCounts> RelCounters;
#else
typedef Firebird::SortedArray<RelationCounts, Firebird::EmptyStorage<RelationCounts>,
	SLONG, RelationCounts> RelCounters;
#endif

typedef Firebird::HalfStaticArray<TraceCounts, 5> TraceCountsArray;

// Runtime statistics class

class RuntimeStatistics : protected Firebird::AutoStorage
{
public:
	enum StatType {
		PAGE_FETCHES = 0,
		PAGE_READS,
		PAGE_MARKS,
		PAGE_WRITES,
		FLUSHES,
		RECORD_SEQ_READS,
		RECORD_IDX_READS,
		RECORD_INSERTS,
		RECORD_UPDATES,
		RECORD_DELETES,
		RECORD_BACKOUTS,
		RECORD_PURGES,
		RECORD_EXPUNGES,
		SORTS,
		SORT_GETS,
		SORT_PUTS,
		STMT_PREPARES,
		STMT_EXECUTES,
		TOTAL_ITEMS		// last
	};

	RuntimeStatistics()
		: Firebird::AutoStorage(), rel_counts(getPool())
	{
		reset();
	}

	explicit RuntimeStatistics(MemoryPool& pool)
		: Firebird::AutoStorage(pool), rel_counts(getPool())
	{
		reset();
	}

	RuntimeStatistics(const RuntimeStatistics& other)
		: Firebird::AutoStorage(), rel_counts(getPool())
	{
		memcpy(values, other.values, sizeof(values));
		rel_counts = other.rel_counts;

		allChgNumber = other.allChgNumber;
		relChgNumber = other.relChgNumber;
	}

	RuntimeStatistics(MemoryPool& pool, const RuntimeStatistics& other)
		: Firebird::AutoStorage(pool), rel_counts(getPool())
	{
		memcpy(values, other.values, sizeof(values));
		rel_counts = other.rel_counts;

		allChgNumber = other.allChgNumber;
		relChgNumber = other.relChgNumber;
	}

	~RuntimeStatistics() {}

	void reset()
	{
		memset(values, 0, sizeof values);
		rel_counts.clear();
		allChgNumber = 0;
		relChgNumber = 0;
	}

	SINT64 getValue(const StatType index) const
	{
		return values[index];
	}

	void bumpValue(const StatType index)
	{
		++values[index];
		++allChgNumber;
	}

	void bumpValue(const StatType index, SLONG relation_id);

	// Calculate difference between counts stored in this object and current
	// counts of given request. Counts stored in object are destroyed.
	PerformanceInfo* computeDifference(Database* dbb, const RuntimeStatistics& new_stat,
		PerformanceInfo& dest, TraceCountsArray& temp);

	// bool operator==(const RuntimeStatistics& other) const;
	// bool operator!=(const RuntimeStatistics& other) const;

	// add difference between newStats and baseStats to our counters
	// newStats and baseStats must be "in-sync"
	void adjust(const RuntimeStatistics& baseStats, const RuntimeStatistics& newStats)
	{
		if (baseStats.allChgNumber != newStats.allChgNumber)
		{
			allChgNumber++;
			for (size_t i = 0; i < TOTAL_ITEMS; ++i)
			{
				values[i] += newStats.values[i] - baseStats.values[i];
			}

			if (baseStats.relChgNumber != newStats.relChgNumber)
			{
				relChgNumber++;
				addRelCounts(newStats.rel_counts, true);
				addRelCounts(baseStats.rel_counts, false);
			}
		}

	}

	// copy counters values from other instance
	// after copying both instances are "in-sync" i.e. have the same
	// allChgNumber and relChgNumber values
	RuntimeStatistics& assign(const RuntimeStatistics& other)
	{
		if (allChgNumber != other.allChgNumber)
		{
			memcpy(values, other.values, sizeof(values));
			allChgNumber = other.allChgNumber;
		}

		if (relChgNumber != other.relChgNumber)
		{
			rel_counts = other.rel_counts;
			relChgNumber = other.relChgNumber;
		}

		return *this;
	}

	static RuntimeStatistics* getDummy()
	{
		return &dummy;
	}

private:
	void addRelCounts(const RelCounters& other, bool add);

	SINT64 values[TOTAL_ITEMS];
	RelCounters rel_counts;

	// These two numbers are used in adjust() and assign() methods as "generation"
	// values in order to avoid costly operations when two instances of RuntimeStatistics
	// contain equal counters values. This is intended to use *only* with the
	// same pair of class instances, as in jrd_req.
	ULONG allChgNumber;		// incremented when any counter changes
	ULONG relChgNumber;		// incremented when relation counter changes

	// This dummy RuntimeStatistics is used instead of missing elements in tdbb,
	// helping us to avoid conditional checks in time-critical places of code.
	// Values of it contain actually garbage - don't be surprised when debugging.
	static Firebird::GlobalPtr<RuntimeStatistics> dummy;
};

} // namespace

#endif // JRD_RUNTIME_STATISTICS_H
