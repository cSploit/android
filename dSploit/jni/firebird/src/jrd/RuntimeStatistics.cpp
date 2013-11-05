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

#include "firebird.h"
#include "../jrd/gdsassert.h"
#include "../jrd/req.h"

#include "../jrd/RuntimeStatistics.h"

using namespace Firebird;

namespace Jrd {

GlobalPtr<RuntimeStatistics> RuntimeStatistics::dummy;

#ifdef REL_COUNTS_TREE

void RuntimeStatistics::bumpValue(const StatType index, SLONG relation_id)
{
	fb_assert(index >= 0);
	++relChgNumber;

	RelCounters::Accessor accessor(&rel_counts);
	if (accessor.locate(relation_id))
		accessor.current().rlc_counter[index]++;
	else
	{
		RelationCounts counts;
		memset(&counts, 0, sizeof(counts));
		counts.rlc_relation_id = relation_id;
		counts.rlc_counter[index]++;
		rel_counts.add(counts);
	}
}


void RuntimeStatistics::addRelCounts(const RelCounters& other, bool add)
{
	RelCounters::Accessor first(&rel_counts);
	RelCounters::ConstAccessor second(&other);

	if (second.getFirst())
	{
		do
		{
			const RelationCounts& src = second.current();

			if (!first.locate(src.rlc_relation_id))
			{
				RelationCounts counts;
				memset(&counts, 0, sizeof(counts));
				counts.rlc_relation_id = src.rlc_relation_id;
				first.add(counts);

				first.locate(src.rlc_relation_id);
			}

			RelationCounts& dst = first.current();
			fb_assert(src.rlc_relation_id == dst.rlc_relation_id);

			if (add)
			{
				for (int index = 0; index < FB_NELEM(src.rlc_counter); index++)
					dst.rlc_counter[index] += src.rlc_counter[index];
			}
			else
			{
				for (int index = 0; index < FB_NELEM(src.rlc_counter); index++)
					dst.rlc_counter[index] -= src.rlc_counter[index];
			}
		} while(second.getNext());
	}
}


PerformanceInfo* RuntimeStatistics::computeDifference(
	Database* dbb, const RuntimeStatistics& new_stat, PerformanceInfo& dest, TraceCountsArray& temp)
{
	// NOTE: we do not initialize dest.pin_time. This must be done by the caller

	// Calculate database-level statistics
	for (int i = 0; i < TOTAL_ITEMS; i++)
		values[i] = new_stat.values[i] - values[i];

	dest.pin_counters = values;

	// Calculate relation-level statistics
	temp.clear();

	RelCounters::ConstAccessor new_acc(&new_stat.rel_counts);

	if (new_acc.getFirst())
	{
		// This loop assumes that base array is smaller than new one
		RelCounters::Accessor base_acc(&rel_counts);
		bool base_found = base_acc.getFirst();

		do
		{
			const RelationCounts* counts = &new_acc.current();
			if (base_found && base_acc.current().rlc_relation_id == counts->rlc_relation_id)
			{
				RelationCounts* base_counts = &base_acc.current();
				bool all_zeros = true;
				for (int i = 0; i < DBB_max_rel_count; i++)
				{
					if ((base_counts->rlc_counter[i] = counts->rlc_counter[i] - base_counts->rlc_counter[i]))
						all_zeros = false;
				}

				// Point TraceCounts to counts array from baseline object
				if (!all_zeros)
				{
					jrd_rel* relation = counts->rlc_relation_id < dbb->dbb_relations->count() ?
						(*dbb->dbb_relations)[counts->rlc_relation_id] : NULL;
					TraceCounts traceCounts;
					traceCounts.trc_relation_id = counts->rlc_relation_id;
					traceCounts.trc_counters = base_counts->rlc_counter;
					traceCounts.trc_relation_name = relation ? relation->rel_name.c_str() : NULL;
					temp.add(traceCounts);
				}

				base_found = base_acc.getNext();
			}
			else
			{
				jrd_rel* relation = counts->rlc_relation_id < dbb->dbb_relations->count() ?
					(*dbb->dbb_relations)[counts->rlc_relation_id] : NULL;

				// Point TraceCounts to counts array from object with updated counters
				TraceCounts traceCounts;
				traceCounts.trc_relation_id = counts->rlc_relation_id;
				traceCounts.trc_counters = counts->rlc_counter;
				traceCounts.trc_relation_name = relation ? relation->rel_name.c_str() : NULL;
				temp.add(traceCounts);
			}
		} while (new_acc.getNext());
	}

	dest.pin_count = temp.getCount();
	dest.pin_tables = temp.begin();

	return &dest;
}

#else  // REL_COUNTS_TREE


void RuntimeStatistics::bumpValue(const StatType index, SLONG relation_id)
{
	fb_assert(index >= 0);
	++relChgNumber;

	size_t pos;
	if (rel_counts.find(relation_id, pos))
		rel_counts[pos].rlc_counter[index]++;
	else
	{
		RelationCounts counts;
		memset(&counts, 0, sizeof(counts));
		counts.rlc_relation_id = relation_id;
		counts.rlc_counter[index]++;
		rel_counts.add(counts);
	}
}

void RuntimeStatistics::addRelCounts(const RelCounters& other, bool add)
{
	if (other.isEmpty())
		return;

	RelCounters::const_iterator src(other.begin());
	const RelCounters::const_iterator end(other.end());

	size_t pos;
	rel_counts.find(src->rlc_relation_id, pos);
	for (; src != end; ++src)
	{
		const size_t cnt = rel_counts.getCount();

		while (pos < cnt && rel_counts[pos].rlc_relation_id < src->rlc_relation_id)
			pos++;

		if (pos >= cnt || rel_counts[pos].rlc_relation_id > src->rlc_relation_id)
		{
			RelationCounts counts;
			memset(&counts, 0, sizeof(counts));
			counts.rlc_relation_id = src->rlc_relation_id;
			rel_counts.insert(pos, counts);
		}

		fb_assert(pos >= 0 && pos < rel_counts.getCount());

		RelationCounts* dst = &(rel_counts[pos]);
		fb_assert(dst->rlc_relation_id == src->rlc_relation_id);

		if (add)
		{
			for (int index = 0; index < FB_NELEM(src->rlc_counter); index++)
				dst->rlc_counter[index] += src->rlc_counter[index];
		}
		else
		{
			for (int index = 0; index < FB_NELEM(src->rlc_counter); index++)
				dst->rlc_counter[index] -= src->rlc_counter[index];
		}
	}
}

PerformanceInfo* RuntimeStatistics::computeDifference(Database* dbb,
													  const RuntimeStatistics& new_stat,
													  PerformanceInfo& dest,
													  TraceCountsArray& temp)
{
	// NOTE: we do not initialize dest.pin_time. This must be done by the caller

	// Calculate database-level statistics
	for (int i = 0; i < TOTAL_ITEMS; i++)
		values[i] = new_stat.values[i] - values[i];

	dest.pin_counters = values;

	// Calculate relation-level statistics
	temp.clear();

	// This loop assumes that base array is smaller than new one
	RelCounters::iterator base_cnts = rel_counts.begin();
	bool base_found = (base_cnts != rel_counts.end());

	RelCounters::const_iterator new_cnts = new_stat.rel_counts.begin();
	const RelCounters::const_iterator end = new_stat.rel_counts.end();
	for (; new_cnts != end; ++new_cnts)
	{
		if (base_found && base_cnts->rlc_relation_id == new_cnts->rlc_relation_id)
		{
			bool all_zeros = true;
			for (int i = 0; i < DBB_max_rel_count; i++)
			{
				if ((base_cnts->rlc_counter[i] = new_cnts->rlc_counter[i] - base_cnts->rlc_counter[i]))
					all_zeros = false;
			}

			// Point TraceCounts to counts array from baseline object
			if (!all_zeros)
			{
				jrd_rel* relation = size_t(new_cnts->rlc_relation_id) < dbb->dbb_relations->count() ?
					(*dbb->dbb_relations)[new_cnts->rlc_relation_id] : NULL;
				TraceCounts traceCounts;
				traceCounts.trc_relation_id = new_cnts->rlc_relation_id;
				traceCounts.trc_counters = base_cnts->rlc_counter;
				traceCounts.trc_relation_name = relation ? relation->rel_name.c_str() : NULL;
				temp.add(traceCounts);
			}

			++base_cnts;
			base_found = (base_cnts != rel_counts.end());
		}
		else
		{
			jrd_rel* relation = size_t(new_cnts->rlc_relation_id) < dbb->dbb_relations->count() ?
				(*dbb->dbb_relations)[new_cnts->rlc_relation_id] : NULL;

			// Point TraceCounts to counts array from object with updated counters
			TraceCounts traceCounts;
			traceCounts.trc_relation_id = new_cnts->rlc_relation_id;
			traceCounts.trc_counters = new_cnts->rlc_counter;
			traceCounts.trc_relation_name = relation ? relation->rel_name.c_str() : NULL;
			temp.add(traceCounts);
		}
	};

	dest.pin_count = temp.getCount();
	dest.pin_tables = temp.begin();

	return &dest;
}
#endif  // REL_COUNTS_TREE

} // namespace
