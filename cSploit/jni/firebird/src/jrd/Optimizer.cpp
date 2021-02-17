/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		Optimizer.cpp
 *	DESCRIPTION:	Optimizer
 *
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
 *  The Original Code was created by Arno Brinkman
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Arno Brinkman <firebird@abvisie.nl>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Adriano dos Santos Fernandes
 *
 */

#include "firebird.h"

#include "../jrd/jrd.h"
#include "../jrd/exe.h"
#include "../jrd/btr.h"
#include "../jrd/intl.h"
#include "../jrd/Collation.h"
#include "../jrd/rse.h"
#include "../jrd/ods.h"
#include "../jrd/Optimizer.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/recsrc/RecordSource.h"
#include "../dsql/BoolNodes.h"
#include "../dsql/ExprNodes.h"
#include "../dsql/StmtNodes.h"

#include "../jrd/btr_proto.h"
#include "../jrd/cch_proto.h"
#include "../jrd/cmp_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/exe_proto.h"
#include "../jrd/ext_proto.h"
#include "../jrd/intl_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/par_proto.h"

using namespace Firebird;

namespace Jrd {


// Check the index for being an expression one and
// matching both the given stream and the given expression tree
bool checkExpressionIndex(const index_desc* idx, ValueExprNode* node, StreamType stream)
{
	fb_assert(idx);

	if (idx->idx_expression)
	{
		// The desired expression can be hidden inside a derived expression node,
		// so try to recover it (see CORE-4118).
		while (!idx->idx_expression->sameAs(node, true))
		{
			DerivedExprNode* const derivedExpr = node->as<DerivedExprNode>();
			if (!derivedExpr)
				return false;
			node = derivedExpr->arg;
		}

		SortedStreamList exprStreams, nodeStreams;
		idx->idx_expression->collectStreams(exprStreams);
		node->collectStreams(nodeStreams);

		if (exprStreams.getCount() == 1 && exprStreams[0] == 0 &&
			nodeStreams.getCount() == 1 && nodeStreams[0] == stream)
		{
			return true;
		}
	}

	return false;
}


double OPT_getRelationCardinality(thread_db* tdbb, jrd_rel* relation, const Format* format)
{
/**************************************
 *
 *	g e t R e l a t i o n C a r d i n a l i t y
 *
 **************************************
 *
 * Functional description
 *	Return the estimated cardinality for
 *  the given relation.
 *
 **************************************/
	SET_TDBB(tdbb);

	if (relation->isVirtual())
	{
		// Just a dumb estimation
		return (double) 100;
	}

	if (relation->rel_file)
	{
		return EXT_cardinality(tdbb, relation);
	}

	MET_post_existence(tdbb, relation);
	const double cardinality = DPM_cardinality(tdbb, relation, format);
	MET_release_existence(tdbb, relation);
	return cardinality;
}


string OPT_make_alias(thread_db* tdbb, const CompilerScratch* csb,
					  const CompilerScratch::csb_repeat* base_tail)
{
/**************************************
 *
 *	m a k e _ a l i a s
 *
 **************************************
 *
 * Functional description
 *	Make an alias string suitable for printing
 *	as part of the plan.  For views, this means
 *	multiple aliases to distinguish the base
 *	table.
 *
 **************************************/
	DEV_BLKCHK(csb, type_csb);
	SET_TDBB(tdbb);

	string alias;

	if (base_tail->csb_view || base_tail->csb_alias)
	{
		ObjectsArray<string> alias_list;

		for (const CompilerScratch::csb_repeat* csb_tail = base_tail; ;
			csb_tail = &csb->csb_rpt[csb_tail->csb_view_stream])
		{
			if (csb_tail->csb_alias)
			{
				alias_list.push(*csb_tail->csb_alias);
			}
			else if (csb_tail->csb_relation)
			{
				alias_list.push(csb_tail->csb_relation->rel_name.c_str());
			}

			if (!csb_tail->csb_view)
				break;

		}

		while (alias_list.getCount())
		{
			alias += alias_list.pop();

			if (alias_list.getCount())
			{
				alias += ' ';
			}
		}
	}
	else if (base_tail->csb_relation)
	{
		alias = base_tail->csb_relation->rel_name.c_str();
	}
	else if (base_tail->csb_procedure)
	{
		alias = base_tail->csb_procedure->getName().toString();
	}
	else
	{
		fb_assert(false);
	}

	return alias;
}


IndexScratchSegment::IndexScratchSegment(MemoryPool& p) :
	matches(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h S e g m e n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	lowerValue = NULL;
	upperValue = NULL;
	excludeLower = false;
	excludeUpper = false;
	scope = 0;
	scanType = segmentScanNone;
}

IndexScratchSegment::IndexScratchSegment(MemoryPool& p, IndexScratchSegment* segment) :
	matches(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h S e g m e n t
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	lowerValue = segment->lowerValue;
	upperValue = segment->upperValue;
	excludeLower = segment->excludeLower;
	excludeUpper = segment->excludeUpper;
	scope = segment->scope;
	scanType = segment->scanType;

	for (size_t i = 0; i < segment->matches.getCount(); i++) {
		matches.add(segment->matches[i]);
	}
}

IndexScratch::IndexScratch(MemoryPool& p, thread_db* tdbb, index_desc* ix,
	CompilerScratch::csb_repeat* csb_tail) :
	idx(ix), segments(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	// Allocate needed segments
	selectivity = MAXIMUM_SELECTIVITY;
	candidate = false;
	scopeCandidate = false;
	lowerCount = 0;
	upperCount = 0;
	nonFullMatchedSegments = 0;
	fuzzy = false;

	segments.grow(idx->idx_count);

	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		segment[i] = FB_NEW(p) IndexScratchSegment(p);
	}

	const int length = ROUNDUP(BTR_key_length(tdbb, csb_tail->csb_relation, idx), sizeof(SLONG));

	// AB: Calculate the cardinality which should reflect the total number
	// of index pages for this index.
	// We assume that the average index-key can be compressed by a factor 0.5
	// In the future the average key-length should be stored and retrieved
	// from a system table (RDB$INDICES for example).
	// Multipling the selectivity with this cardinality gives the estimated
	// number of index pages that are read for the index retrieval.
	double factor = 0.5;
	if (segments.getCount() >= 2)
	{
		// Compound indexes are generally less compressed.
		factor = 0.7;
	}

	const Database* const dbb = tdbb->getDatabase();
	cardinality = (csb_tail->csb_cardinality * (2 + (length * factor))) / (dbb->dbb_page_size - BTR_SIZE);
	cardinality = MAX(cardinality, MINIMUM_CARDINALITY);
}

IndexScratch::IndexScratch(MemoryPool& p, const IndexScratch& scratch) :
	segments(p)
{
/**************************************
 *
 *	I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	selectivity = scratch.selectivity;
	cardinality = scratch.cardinality;
	candidate = scratch.candidate;
	scopeCandidate = scratch.scopeCandidate;
	lowerCount = scratch.lowerCount;
	upperCount = scratch.upperCount;
	nonFullMatchedSegments = scratch.nonFullMatchedSegments;
	fuzzy = scratch.fuzzy;
	idx = scratch.idx;

	// Allocate needed segments
	segments.grow(scratch.segments.getCount());

	IndexScratchSegment* const* scratchSegment = scratch.segments.begin();
	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		segment[i] = FB_NEW(p) IndexScratchSegment(p, scratchSegment[i]);
	}
}

IndexScratch::~IndexScratch()
{
/**************************************
 *
 *	~I n d e x S c r a t c h
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	IndexScratchSegment** segment = segments.begin();
	for (size_t i = 0; i < segments.getCount(); i++) {
		delete segment[i];
	}
}

InversionCandidate::InversionCandidate(MemoryPool& p) :
	matches(p), dependentFromStreams(p)
{
/**************************************
 *
 *	I n v e r s i o n C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	selectivity = MAXIMUM_SELECTIVITY;
	cost = 0;
	indexes = 0;
	dependencies = 0;
	nonFullMatchedSegments = MAX_INDEX_SEGMENTS + 1;
	matchedSegments = 0;
	boolean = NULL;
	condition = NULL;
	inversion = NULL;
	scratch = NULL;
	used = false;
	unique = false;
	navigated = false;
}

OptimizerRetrieval::OptimizerRetrieval(MemoryPool& p, OptimizerBlk* opt,
									   StreamType streamNumber, bool outer,
									   bool inner, SortNode* sortNode)
	: pool(p), alias(p), indexScratches(p), inversionCandidates(p)
{
/**************************************
 *
 *	O p t i m i z e r R e t r i e v a l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	createIndexScanNodes = false;
	setConjunctionsMatched = false;

	tdbb = NULL;
	SET_TDBB(tdbb);

	this->database = tdbb->getDatabase();
	this->stream = streamNumber;
	this->optimizer = opt;
	this->csb = this->optimizer->opt_csb;
	this->innerFlag = inner;
	this->outerFlag = outer;
	this->sort = sortNode;
	this->navigationCandidate = NULL;
	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[this->stream];
	relation = csb_tail->csb_relation;

	// Allocate needed indexScratches

	index_desc* idx = csb_tail->csb_idx->items;
	for (int i = 0; i < csb_tail->csb_indices; ++i, ++idx)
		indexScratches.add(IndexScratch(p, tdbb, idx, csb_tail));
}

OptimizerRetrieval::~OptimizerRetrieval()
{
/**************************************
 *
 *	~O p t i m i z e r R e t r i e v a l
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	for (size_t i = 0; i < inversionCandidates.getCount(); ++i)
		delete inversionCandidates[i];
}

InversionNode* OptimizerRetrieval::composeInversion(InversionNode* node1, InversionNode* node2,
	InversionNode::Type node_type) const
{
/**************************************
 *
 *	c o m p o s e I n v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Melt two inversions together by the
 *  type given in node_type.
 *
 **************************************/

	if (!node2)
		return node1;

	if (!node1)
		return node2;

	if (node_type == InversionNode::TYPE_OR)
	{
		if (node1->type == InversionNode::TYPE_INDEX &&
			node2->type == InversionNode::TYPE_INDEX &&
			node1->retrieval->irb_index == node2->retrieval->irb_index)
		{
			node_type = InversionNode::TYPE_IN;
		}
		else if (node1->type == InversionNode::TYPE_IN &&
			node2->type == InversionNode::TYPE_INDEX &&
			node1->node2->retrieval->irb_index == node2->retrieval->irb_index)
		{
			node_type = InversionNode::TYPE_IN;
		}
	}

	return FB_NEW(pool) InversionNode(node_type, node1, node2);
}

const string& OptimizerRetrieval::getAlias()
{
/**************************************
 *
 *	g e t A l i a s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (alias.isEmpty())
	{
		const CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[this->stream];
		alias = OPT_make_alias(tdbb, csb, csb_tail);
	}
	return alias;
}

InversionCandidate* OptimizerRetrieval::generateInversion()
{
/**************************************
 *
 *	g e n e r a t e I n v e r s i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	OptimizerBlk::opt_conjunct* const opt_begin =
		optimizer->opt_conjuncts.begin() + (outerFlag ? optimizer->opt_base_parent_conjuncts : 0);

	const OptimizerBlk::opt_conjunct* const opt_end =
		innerFlag ? optimizer->opt_conjuncts.begin() + optimizer->opt_base_missing_conjuncts :
					optimizer->opt_conjuncts.end();

	InversionCandidate* invCandidate = NULL;

	if (relation && !relation->rel_file && !relation->isVirtual())
	{
		InversionCandidateList inversions;

		// Check for any DB_KEY comparisons
		for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
		{
			BoolExprNode* const node = tail->opt_conjunct_node;

			if (!(tail->opt_conjunct_flags & opt_conjunct_used) && node)
			{
				invCandidate = matchDbKey(node);

				if (invCandidate)
					inversions.add(invCandidate);
			}
		}

		// First, handle "AND" comparisons (all nodes except OR)
		for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
		{
			BoolExprNode* const node = tail->opt_conjunct_node;
			BinaryBoolNode* booleanNode = node->as<BinaryBoolNode>();

			if (!(tail->opt_conjunct_flags & opt_conjunct_used) && node &&
				(!booleanNode || booleanNode->blrOp != blr_or))
			{
				matchOnIndexes(&indexScratches, node, 1);
			}
		}

		getInversionCandidates(&inversions, &indexScratches, 1);

		// Second, handle "OR" comparisons
		for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
		{
			BoolExprNode* const node = tail->opt_conjunct_node;
			BinaryBoolNode* booleanNode = node->as<BinaryBoolNode>();

			if (!(tail->opt_conjunct_flags & opt_conjunct_used) && node &&
				(booleanNode && booleanNode->blrOp == blr_or))
			{
				invCandidate = matchOnIndexes(&indexScratches, node, 1);

				if (invCandidate)
				{
					invCandidate->boolean = node;
					inversions.add(invCandidate);
				}
			}
		}

		if (sort)
			analyzeNavigation();

#ifdef OPT_DEBUG_RETRIEVAL
		// Debug
		printCandidates(&inversions);
#endif

		invCandidate = makeInversion(&inversions);

		// Clean up inversion list
		InversionCandidate** inversion = inversions.begin();
		for (size_t i = 0; i < inversions.getCount(); i++)
			delete inversion[i];
	}

	if (!invCandidate)
	{
		// No index will be used, thus create a dummy inversion candidate
		// representing the natural table access. All the necessary properties
		// (selectivity: 1.0, cost: 0, unique: false) are set up by the constructor.
		invCandidate = FB_NEW(pool) InversionCandidate(pool);
	}

	if (invCandidate->unique)
	{
		// Set up the unique retrieval cost to be fixed and not dependent on
		// possibly outdated statistics. It includes N index scans plus one data page fetch.
		invCandidate->cost = DEFAULT_INDEX_COST * invCandidate->indexes + 1;
	}
	else
	{
		// Add the records retrieval cost to the priorly calculated index scan cost
		invCandidate->cost += csb->csb_rpt[stream].csb_cardinality * invCandidate->selectivity;
	}

	// Adjust the effective selectivity by treating computable conjunctions as filters
	for (const OptimizerBlk::opt_conjunct* tail = optimizer->opt_conjuncts.begin();
		tail < optimizer->opt_conjuncts.end(); tail++)
	{
		BoolExprNode* const node = tail->opt_conjunct_node;

		if (!(tail->opt_conjunct_flags & opt_conjunct_used) &&
			node->computable(csb, stream, true) &&
			!invCandidate->matches.exist(node))
		{
			const ComparativeBoolNode* const cmpNode = node->as<ComparativeBoolNode>();

			const double factor = (cmpNode && cmpNode->blrOp == blr_eql) ?
				REDUCE_SELECTIVITY_FACTOR_EQUALITY : REDUCE_SELECTIVITY_FACTOR_INEQUALITY;
			invCandidate->selectivity *= factor;
		}
	}

	// Add the streams where this stream is depending on.
	for (size_t i = 0; i < invCandidate->matches.getCount(); i++)
		invCandidate->matches[i]->findDependentFromStreams(this, &invCandidate->dependentFromStreams);

	if (setConjunctionsMatched)
	{
		SortedArray<BoolExprNode*> matches;

		// AB: Putting a unsorted array in a sorted array directly by join isn't
		// very safe at the moment, but in our case Array holds a sorted list.
		// However SortedArray class should be updated to handle join right!

		matches.join(invCandidate->matches);

		for (OptimizerBlk::opt_conjunct* tail = opt_begin; tail < opt_end; tail++)
		{
			if (!(tail->opt_conjunct_flags & opt_conjunct_used))
			{
				if (matches.exist(tail->opt_conjunct_node))
					tail->opt_conjunct_flags |= opt_conjunct_matched;
			}
		}
	}

#ifdef OPT_DEBUG_RETRIEVAL
	// Debug
	printFinalCandidate(invCandidate);
#endif

	return invCandidate;
}

IndexTableScan* OptimizerRetrieval::getNavigation()
{
/**************************************
 *
 *	g e t N a v i g a t i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (!navigationCandidate)
		return NULL;

	// Looks like we can do a navigational walk.  Flag that
	// we have used this index for navigation, and allocate
	// a navigational rsb for it.
	navigationCandidate->idx->idx_runtime_flags |= idx_navigate;

	const USHORT key_length =
		ROUNDUP(BTR_key_length(tdbb, relation, navigationCandidate->idx), sizeof(SLONG));

	InversionNode* const index_node = makeIndexScanNode(navigationCandidate);

	return FB_NEW(*tdbb->getDefaultPool())
		IndexTableScan(csb, getAlias(), stream, index_node, key_length);
}

void OptimizerRetrieval::analyzeNavigation()
{
/**************************************
 *
 *	a n a l y z e N a v i g a t i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	fb_assert(sort);

	for (size_t i = 0; i < indexScratches.getCount(); ++i)
	{
		IndexScratch* const indexScratch = &indexScratches[i];
		const index_desc* const idx = indexScratch->idx;
		const int equalSegments = MIN(indexScratch->lowerCount, indexScratch->upperCount);

		// if the number of fields in the sort is greater than the number of
		// fields in the index, the index will not be used to optimize the
		// sort--note that in the case where the first field is unique, this
		// could be optimized, since the sort will be performed correctly by
		// navigating on a unique index on the first field--deej
		if (sort->expressions.getCount() > idx->idx_count)
			continue;

		// if the user-specified access plan for this request didn't
		// mention this index, forget it
		if ((idx->idx_runtime_flags & idx_plan_dont_use) &&
			!(idx->idx_runtime_flags & idx_plan_navigate))
		{
			continue;
		}

		// only a single-column ORDER BY clause can be mapped to
		// an expression index
		if (idx->idx_flags & idx_expressn)
		{
			if (sort->expressions.getCount() != 1)
				continue;
		}

		// check to see if the fields in the sort match the fields in the index
		// in the exact same order

		bool usableIndex = true;
		const index_desc::idx_repeat* idx_tail = idx->idx_rpt;
		const index_desc::idx_repeat* const idx_end = idx_tail + idx->idx_count;
		NestConst<ValueExprNode>* ptr = sort->expressions.begin();
		const bool* descending = sort->descending.begin();
		const int* nullOrder = sort->nullOrder.begin();

		for (const NestConst<ValueExprNode>* const end = sort->expressions.end();
			 ptr != end;
			 ++ptr, ++descending, ++nullOrder, ++idx_tail)
		{
			ValueExprNode* const node = *ptr;
			FieldNode* fieldNode;

			if (idx->idx_flags & idx_expressn)
			{
				if (!checkExpressionIndex(idx, node, stream))
				{
					usableIndex = false;
					break;
				}
			}
			else if (!(fieldNode = node->as<FieldNode>()) || fieldNode->fieldStream != stream)
			{
				usableIndex = false;
				break;
			}
			else
			{
				for (; idx_tail < idx_end && fieldNode->fieldId != idx_tail->idx_field; idx_tail++)
				{
					const int segmentNumber = idx_tail - idx->idx_rpt;

					if (segmentNumber >= equalSegments)
						break;
				}

				if (idx_tail >= idx_end || fieldNode->fieldId != idx_tail->idx_field)
				{
					usableIndex = false;
					break;
				}
			}

			if ((*descending && !(idx->idx_flags & idx_descending)) ||
				(!*descending && (idx->idx_flags & idx_descending)) ||
				((*nullOrder == rse_nulls_first && *descending) ||
				 (*nullOrder == rse_nulls_last && !*descending)))
			{
				usableIndex = false;
				break;
			}

			dsc desc;
			node->getDesc(tdbb, csb, &desc);

			// ASF: "desc.dsc_ttype() > ttype_last_internal" is to avoid recursion
			// when looking for charsets/collations

			if (DTYPE_IS_TEXT(desc.dsc_dtype) && desc.dsc_ttype() > ttype_last_internal)
			{
				const TextType* const tt = INTL_texttype_lookup(tdbb, desc.dsc_ttype());

				if (idx->idx_flags & idx_unique)
				{
					if (tt->getFlags() & TEXTTYPE_UNSORTED_UNIQUE)
					{
						usableIndex = false;
						break;
					}
				}
				else
				{
					// ASF: We currently can't use non-unique index for GROUP BY and DISTINCT with
					// multi-level and insensitive collation. In NAV, keys are verified with memcmp
					// but there we don't know length of each level.
					if (sort->unique && (tt->getFlags() & TEXTTYPE_SEPARATE_UNIQUE))
					{
						usableIndex = false;
						break;
					}
				}
			}
		}

		if (!usableIndex)
			continue;

		// Looks like we can do a navigational walk. Remember this candidate
		// and compare it against other possible candidates.

		if (!navigationCandidate)
			navigationCandidate = indexScratch;
		else
		{
			int count1 = MAX(indexScratch->lowerCount, indexScratch->upperCount);
			int count2 = MAX(navigationCandidate->lowerCount, navigationCandidate->upperCount);

			if (count1 > count2)
				navigationCandidate = indexScratch;
			else if (count1 == count2)
			{
				count1 = (int) indexScratch->segments.getCount();
				count2 = (int) navigationCandidate->segments.getCount();

				if (count1 < count2)
					navigationCandidate = indexScratch;
				else if (count1 == count2)
				{
					count1 = BTR_key_length(tdbb, relation, indexScratch->idx);
					count2 = BTR_key_length(tdbb, relation, navigationCandidate->idx);

					if (count1 < count2)
						navigationCandidate = indexScratch;
				}
			}
		}
	}
}

void OptimizerRetrieval::getInversionCandidates(InversionCandidateList* inversions,
		IndexScratchList* fromIndexScratches, USHORT scope) const
{
/**************************************
 *
 *	g e t I n v e r s i o n C a n d i d a t e s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	const double cardinality = csb->csb_rpt[stream].csb_cardinality;

	// Walk through indexes to calculate selectivity / candidate
	Array<BoolExprNode*> matches;
	size_t i = 0;

	for (i = 0; i < fromIndexScratches->getCount(); i++)
	{
		IndexScratch& scratch = (*fromIndexScratches)[i];
		scratch.scopeCandidate = false;
		scratch.lowerCount = 0;
		scratch.upperCount = 0;
		scratch.nonFullMatchedSegments = MAX_INDEX_SEGMENTS + 1;
		scratch.fuzzy = false;

		if (scratch.candidate)
		{
			matches.clear();
			scratch.selectivity = MAXIMUM_SELECTIVITY;

			bool unique = false;

			for (int j = 0; j < scratch.idx->idx_count; j++)
			{
				const IndexScratchSegment* const segment = scratch.segments[j];

				if (segment->scope == scope)
					scratch.scopeCandidate = true;

				if (segment->scanType != segmentScanMissing && !(scratch.idx->idx_flags & idx_unique))
				{
					const USHORT iType = scratch.idx->idx_rpt[j].idx_itype;

					if (iType >= idx_first_intl_string)
					{
						TextType* textType = INTL_texttype_lookup(tdbb, INTL_INDEX_TO_TEXT(iType));

						if (textType->getFlags() & TEXTTYPE_SEPARATE_UNIQUE)
						{
							// ASF: Order is more precise than equivalence class.
							// We can't use the next segments, and we'll need to use
							// INTL_KEY_PARTIAL to construct the last segment's key.
							scratch.fuzzy = true;
						}
					}
				}

				// Check if this is the last usable segment
				if (!scratch.fuzzy &&
					(segment->scanType == segmentScanEqual ||
					 segment->scanType == segmentScanEquivalent ||
					 segment->scanType == segmentScanMissing))
				{
					// This is a perfect usable segment thus update root selectivity
					scratch.lowerCount++;
					scratch.upperCount++;
					scratch.selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
					scratch.nonFullMatchedSegments = scratch.idx->idx_count - (j + 1);
					// Add matches for this segment to the main matches list
					matches.join(segment->matches);

					// An equality scan for any unique index cannot retrieve more
					// than one row. The same is true for an equivalence scan for
					// any primary index.
					const bool single_match =
						(segment->scanType == segmentScanEqual &&
							(scratch.idx->idx_flags & idx_unique)) ||
						(segment->scanType == segmentScanEquivalent &&
							(scratch.idx->idx_flags & idx_primary));

					// dimitr: IS NULL scan against primary key is guaranteed
					//		   to return zero rows. Do we need yet another
					//		   special case here?

					if (single_match && ((j + 1) == scratch.idx->idx_count))
					{
						// We have found a full equal matching index and it's unique,
						// so we can stop looking further, because this is the best
						// one we can get.
						unique = true;
						break;
					}

					// dimitr: number of nulls is not reflected by our selectivity,
					//		   so IS NOT DISTINCT and IS NULL scans may retrieve
					//		   much bigger bitmap than expected here. I think
					//		   appropriate reduce selectivity factors are required
					//		   to be applied here.
				}
				else
				{
					// This is our last segment that we can use,
					// estimate the selectivity
					double selectivity = scratch.selectivity;
					double factor = 1;

					switch (segment->scanType)
					{
						case segmentScanBetween:
							scratch.lowerCount++;
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_BETWEEN;
							break;

						case segmentScanLess:
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_LESS;
							break;

						case segmentScanGreater:
							scratch.lowerCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_GREATER;
							break;

						case segmentScanStarting:
						case segmentScanEqual:
						case segmentScanEquivalent:
							scratch.lowerCount++;
							scratch.upperCount++;
							selectivity = scratch.idx->idx_rpt[j].idx_selectivity;
							factor = REDUCE_SELECTIVITY_FACTOR_STARTING;
							break;

						default:
							fb_assert(segment->scanType == segmentScanNone);
							break;
					}

					// Adjust the compound selectivity using the reduce factor.
					// It should be better than the previous segment but worse
					// than a full match.
					const double diffSelectivity = scratch.selectivity - selectivity;
					selectivity += (diffSelectivity * factor);
					fb_assert(selectivity <= scratch.selectivity);
					scratch.selectivity = selectivity;

					if (segment->scanType != segmentScanNone)
					{
						matches.join(segment->matches);
						scratch.nonFullMatchedSegments = scratch.idx->idx_count - j;
					}
					break;
				}
			}

			if (scratch.scopeCandidate)
			{
				// When selectivity is zero the statement is prepared on an
				// empty table or the statistics aren't updated.
				// For an unique index, estimate the selectivity via the stream cardinality.
				// For a non-unique one, assume 1/10 of the maximum selectivity, so that
				// at least some indexes could be chosen by the optimizer.
				double selectivity = scratch.selectivity;

				if (selectivity <= 0)
				{
					if (unique && cardinality)
						selectivity = 1 / cardinality;
					else
						selectivity = DEFAULT_SELECTIVITY;
				}

				InversionCandidate* invCandidate = FB_NEW(pool) InversionCandidate(pool);
				invCandidate->unique = unique;
				invCandidate->selectivity = selectivity;
				// Calculate the cost (only index pages) for this index.
				invCandidate->cost = DEFAULT_INDEX_COST - 1 + scratch.selectivity * scratch.cardinality;
				invCandidate->nonFullMatchedSegments = scratch.nonFullMatchedSegments;
				invCandidate->matchedSegments = MAX(scratch.lowerCount, scratch.upperCount);
				invCandidate->indexes = 1;
				invCandidate->scratch = &scratch;
				invCandidate->matches.join(matches);

				for (size_t k = 0; k < invCandidate->matches.getCount(); k++)
				{
					invCandidate->matches[k]->findDependentFromStreams(this,
						&invCandidate->dependentFromStreams);
				}

				invCandidate->dependencies = (int) invCandidate->dependentFromStreams.getCount();
				inversions->add(invCandidate);
			}
		}
	}
}

ValueExprNode* OptimizerRetrieval::findDbKey(ValueExprNode* dbkey, SLONG* position) const
{
/**************************************
 *
 *	f i n d D b K e y
 *
 **************************************
 *
 * Functional description
 *	Search a dbkey (possibly a concatenated one) for
 *	a dbkey for specified stream.
 *
 **************************************/

	const RecordKeyNode* keyNode = dbkey->as<RecordKeyNode>();

	if (keyNode && keyNode->blrOp == blr_dbkey)
	{
		if (keyNode->recStream == stream)
			return dbkey;

		++*position;
		return NULL;
	}

	ConcatenateNode* concatNode = dbkey->as<ConcatenateNode>();

	if (concatNode)
	{
		ValueExprNode* dbkey_temp = findDbKey(concatNode->arg1, position);

		if (dbkey_temp)
			return dbkey_temp;

		dbkey_temp = findDbKey(concatNode->arg2, position);

		if (dbkey_temp)
			return dbkey_temp;
	}

	return NULL;
}


InversionNode* OptimizerRetrieval::makeIndexNode(const index_desc* idx) const
{
/**************************************
 *
 *	m a k e I n d e x N o d e
 *
 **************************************
 *
 * Functional description
 *	Make an index node and an index retrieval block.
 *
 **************************************/

	// check whether this is during a compile or during
	// a SET INDEX operation
	if (csb)
		CMP_post_resource(&csb->csb_resources, relation, Resource::rsc_index, idx->idx_id);
	else
	{
		CMP_post_resource(&tdbb->getRequest()->getStatement()->resources, relation,
			Resource::rsc_index, idx->idx_id);
	}

	IndexRetrieval* retrieval = FB_NEW_RPT(pool, idx->idx_count * 2) IndexRetrieval();
	retrieval->irb_index = idx->idx_id;
	memcpy(&retrieval->irb_desc, idx, sizeof(retrieval->irb_desc));

	InversionNode* node = FB_NEW(pool) InversionNode(retrieval);

	if (csb)
		node->impure = CMP_impure(csb, sizeof(impure_inversion));

	return node;
}

InversionNode* OptimizerRetrieval::makeIndexScanNode(IndexScratch* indexScratch) const
{
/**************************************
 *
 *	m a k e I n d e x S c a n N o d e
 *
 **************************************
 *
 * Functional description
 *	Build node for index scan.
 *
 **************************************/

	if (!createIndexScanNodes)
		return NULL;

	// Allocate both a index retrieval node and block.
	index_desc* idx = indexScratch->idx;
	InversionNode* node = makeIndexNode(idx);
	IndexRetrieval* retrieval = node->retrieval;
	retrieval->irb_relation = relation;

	// Pick up lower bound segment values
	ValueExprNode** lower = retrieval->irb_value;
	ValueExprNode** upper = retrieval->irb_value + idx->idx_count;
	retrieval->irb_lower_count = indexScratch->lowerCount;
	retrieval->irb_upper_count = indexScratch->upperCount;

	if (idx->idx_flags & idx_descending)
	{
		// switch upper/lower information
		upper = retrieval->irb_value;
		lower = retrieval->irb_value + idx->idx_count;
		retrieval->irb_lower_count = indexScratch->upperCount;
		retrieval->irb_upper_count = indexScratch->lowerCount;
		retrieval->irb_generic |= irb_descending;
	}

	int i = 0;
	bool ignoreNullsOnScan = true;
	IndexScratchSegment** segment = indexScratch->segments.begin();

	for (i = 0; i < MAX(indexScratch->lowerCount, indexScratch->upperCount); i++)
	{
		if (segment[i]->scanType == segmentScanMissing)
		{
			*lower++ = *upper++ = FB_NEW(*tdbb->getDefaultPool()) NullNode(*tdbb->getDefaultPool());
			ignoreNullsOnScan = false;
		}
		else
		{
			if (i < indexScratch->lowerCount)
				*lower++ = segment[i]->lowerValue;

			if (i < indexScratch->upperCount)
				*upper++ = segment[i]->upperValue;

			if (segment[i]->scanType == segmentScanEquivalent)
				ignoreNullsOnScan = false;
		}
	}

	i = MAX(indexScratch->lowerCount, indexScratch->upperCount) - 1;

	if (i >= 0)
	{
		if (segment[i]->scanType == segmentScanStarting)
			retrieval->irb_generic |= irb_starting;

		if (segment[i]->excludeLower)
			retrieval->irb_generic |= irb_exclude_lower;

		if (segment[i]->excludeUpper)
			retrieval->irb_generic |= irb_exclude_upper;
	}

	if (indexScratch->fuzzy)
		retrieval->irb_generic |= irb_starting;	// Flag the need to use INTL_KEY_PARTIAL in btr.

	// This index is never used for IS NULL, thus we can ignore NULLs
	// already at index scan. But this rule doesn't apply to nod_equiv
	// which requires NULLs to be found in the index.
	// A second exception is when this index is used for navigation.
	if (ignoreNullsOnScan && !(idx->idx_runtime_flags & idx_navigate))
		retrieval->irb_generic |= irb_ignore_null_value_key;

	// Check to see if this is really an equality retrieval
	if (retrieval->irb_lower_count == retrieval->irb_upper_count)
	{
		retrieval->irb_generic |= irb_equality;
		segment = indexScratch->segments.begin();

		for (i = 0; i < retrieval->irb_lower_count; i++)
		{
			if (segment[i]->lowerValue != segment[i]->upperValue)
			{
				retrieval->irb_generic &= ~irb_equality;
				break;
			}
		}
	}

	// If we are matching less than the full index, this is a partial match
	if (idx->idx_flags & idx_descending)
	{
		if (retrieval->irb_lower_count < idx->idx_count)
			retrieval->irb_generic |= irb_partial;
	}
	else
	{
		if (retrieval->irb_upper_count < idx->idx_count)
			retrieval->irb_generic |= irb_partial;
	}

	// mark the index as utilized for the purposes of this compile
	idx->idx_runtime_flags |= idx_used;

	return node;
}

InversionCandidate* OptimizerRetrieval::makeInversion(InversionCandidateList* inversions) const
{
/**************************************
 *
 *	m a k e I n v e r s i o n
 *
 **************************************
 *
 * Select best available inversion candidates
 * and compose them to 1 inversion.
 * This was never implemented:
 * If top is true the datapages-cost is
 * also used in the calculation (only needed
 * for top InversionNode generation).
 *
 **************************************/

	if (inversions->isEmpty() && !navigationCandidate)
		return NULL;

	const double streamCardinality =
		MAX(csb->csb_rpt[stream].csb_cardinality, MINIMUM_CARDINALITY);

	// Prepared statements could be optimized against an almost empty table
	// and then cached (such as in the restore process), thus causing slowdown
	// when the table grows. In this case, let's consider all available indices.
	const bool smallTable = (streamCardinality <= THRESHOLD_CARDINALITY);

	// This flag disables our smart index selection algorithm.
	// It's set for any explicit (i.e. user specified) plan which
	// requires all existing indices to be considered for a retrieval.
	// It's also set for internal (system) requests used by the engine itself.
	const bool acceptAll = csb->csb_rpt[stream].csb_plan || (csb->csb_g_flags & csb_internal);

	double totalSelectivity = MAXIMUM_SELECTIVITY; // worst selectivity
	double totalIndexCost = 0;

	// The upper limit to use an index based retrieval is five indexes + almost all datapages
	const double maximumCost = (DEFAULT_INDEX_COST * 5) + (streamCardinality * 0.95);
	const double minimumSelectivity = 1 / streamCardinality;

	double previousTotalCost = maximumCost;

	// Force to always choose at least one index
	bool firstCandidate = true;

	size_t i = 0;
	InversionCandidate* invCandidate = NULL;
	InversionCandidate** inversion = inversions->begin();
	for (i = 0; i < inversions->getCount(); i++)
	{
		inversion[i]->used = false;
		const IndexScratch* const indexScratch = inversion[i]->scratch;

		if (indexScratch && (indexScratch->idx->idx_runtime_flags & idx_plan_dont_use))
			inversion[i]->used = true;
	}

	// The matches returned in this inversion are always sorted.
	SortedArray<BoolExprNode*> matches;

	if (navigationCandidate)
	{
		for (i = 0; i < navigationCandidate->segments.getCount(); i++)
		{
			const IndexScratchSegment* const segment = navigationCandidate->segments[i];

			for (size_t j = 0; j < segment->matches.getCount(); j++)
			{
				if (!matches.exist(segment->matches[j]))
					matches.add(segment->matches[j]);
			}
		}
	}

	for (i = 0; i < inversions->getCount(); i++)
	{
		// Initialize vars before walking through candidates
		InversionCandidate* bestCandidate = NULL;
		bool restartLoop = false;

		for (size_t currentPosition = 0; currentPosition < inversions->getCount(); ++currentPosition)
		{
			InversionCandidate* currentInv = inversion[currentPosition];
			if (!currentInv->used)
			{
				// If this is a unique full equal matched inversion we're done, so
				// we can make the inversion and return it.
				if (currentInv->unique && currentInv->dependencies && !currentInv->condition)
				{
					if (!invCandidate)
						invCandidate = FB_NEW(pool) InversionCandidate(pool);

					if (!currentInv->inversion && currentInv->scratch)
						invCandidate->inversion = makeIndexScanNode(currentInv->scratch);
					else
						invCandidate->inversion = currentInv->inversion;

					invCandidate->unique = currentInv->unique;
					invCandidate->selectivity = currentInv->selectivity;
					invCandidate->cost = currentInv->cost;
					invCandidate->indexes = currentInv->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments = currentInv->matchedSegments;
					invCandidate->dependencies = currentInv->dependencies;
					matches.clear();

					for (size_t j = 0; j < currentInv->matches.getCount(); j++)
					{
						if (!matches.exist(currentInv->matches[j]))
							matches.add(currentInv->matches[j]);
					}

					if (currentInv->boolean)
					{
						if (!matches.exist(currentInv->boolean))
							matches.add(currentInv->boolean);
					}

					invCandidate->matches.join(matches);
					if (acceptAll)
						continue;

					return invCandidate;
				}

				// Look if a match is already used by previous matches.
				bool anyMatchAlreadyUsed = false;
				for (size_t k = 0; k < currentInv->matches.getCount(); k++)
				{
					if (matches.exist(currentInv->matches[k]))
					{
						anyMatchAlreadyUsed = true;
						break;
					}
				}

				if (anyMatchAlreadyUsed && !acceptAll)
				{
					currentInv->used = true;
					// If a match on this index was already used by another
					// index, add also the other matches from this index.
					for (size_t j = 0; j < currentInv->matches.getCount(); j++)
					{
						if (!matches.exist(currentInv->matches[j]))
							matches.add(currentInv->matches[j]);
					}
					// Restart loop, because other indexes could also be excluded now.
					restartLoop = true;
					break;
				}

				if (!bestCandidate)
				{
					// The first candidate
					bestCandidate = currentInv;
				}
				else
				{
					// Prefer unconditional inversions
					if (currentInv->condition)
					{
						currentInv->used = true;
						restartLoop = true;
						break;
					}

					if (bestCandidate->condition)
					{
						bestCandidate = currentInv;
						restartLoop = true;
						break;
					}

					if (currentInv->unique && !bestCandidate->unique)
					{
						// A unique full equal match is better than anything else.
						bestCandidate = currentInv;
					}
					else if (currentInv->unique == bestCandidate->unique)
					{
						if (currentInv->dependencies > bestCandidate->dependencies)
						{
							// Index used for a relationship must be always prefered to
							// the filtering ones, otherwise the nested loop join has
							// no chances to be better than a sort merge.
							// An alternative (simplified) condition might be:
							//   currentInv->dependencies > 0
							//   && bestCandidate->dependencies == 0
							// but so far I tend to think that the current one is better.
							bestCandidate = currentInv;
						}
						else if (currentInv->dependencies == bestCandidate->dependencies)
						{

							const double bestCandidateCost =
								bestCandidate->cost + (bestCandidate->selectivity * streamCardinality);
							const double currentCandidateCost =
								currentInv->cost + (currentInv->selectivity * streamCardinality);

							// Do we have very similar costs?
							double diffCost = currentCandidateCost;
							if (!diffCost && !bestCandidateCost)
							{
								// Two zero costs should be handled as being the same
								// (other comparison criteria should be applied, see below).
								diffCost = 1;
							}
							else if (diffCost)
							{
								// Calculate the difference.
								diffCost = bestCandidateCost / diffCost;
							}
							else {
								diffCost = 0;
							}

							if ((diffCost >= 0.98) && (diffCost <= 1.02))
							{
								// If the "same" costs then compare with the nr of unmatched segments,
								// how many indexes and matched segments. First compare number of indexes.
								int compareSelectivity = (currentInv->indexes - bestCandidate->indexes);
								if (compareSelectivity == 0)
								{
									// For the same number of indexes compare number of matched segments.
									// Note the inverted condition: the more matched segments the better.
									compareSelectivity =
										(bestCandidate->matchedSegments - currentInv->matchedSegments);
									if (compareSelectivity == 0)
									{
										// For the same number of matched segments
										// compare ones that aren't full matched
										compareSelectivity =
											(currentInv->nonFullMatchedSegments - bestCandidate->nonFullMatchedSegments);
									}
								}
								if (compareSelectivity < 0) {
									bestCandidate = currentInv;
								}
							}
							else if (currentCandidateCost < bestCandidateCost)
							{
								// How lower the cost the better.
								bestCandidate = currentInv;
							}
						}
					}
				}
			}
		}

		if (restartLoop)
			continue;

		// If we have a candidate which is interesting build the inversion
		// else we're done.
		if (bestCandidate)
		{
			// AB: Here we test if our new candidate is interesting enough to be added for
			// index retrieval.

			// AB: For now i'll use the calculation that's often used for and-ing selectivities (S1 * S2).
			// I think this calculation is not right for many cases.
			// For example two "good" selectivities will result in a very good selectivity, but
			// mostly a filter is made by adding criteria's where every criteria is an extra filter
			// compared to the previous one. Thus with the second criteria in _most_ cases still
			// records are returned. (Think also on the segment-selectivity in compound indexes)
			// Assume a table with 100000 records and two selectivities of 0.001 (100 records) which
			// are both AND-ed (with S1 * S2 => 0.001 * 0.001 = 0.000001 => 0.1 record).
			//
			// A better formula could be where the result is between "Sbest" and "Sbest * factor"
			// The reducing factor should be between 0 and 1 (Sbest = best selectivity)
			//
			// Example:
			/*
			double newTotalSelectivity = 0;
			double bestSel = bestCandidate->selectivity;
			double worstSel = totalSelectivity;
			if (bestCandidate->selectivity > totalSelectivity)
			{
				worstSel = bestCandidate->selectivity;
				bestSel = totalSelectivity;
			}

			if (bestSel >= MAXIMUM_SELECTIVITY) {
				newTotalSelectivity = MAXIMUM_SELECTIVITY;
			}
			else if (bestSel == 0) {
				newTotalSelectivity = 0;
			}
			else {
				newTotalSelectivity = bestSel - ((1 - worstSel) * (bestSel - (bestSel * 0.01)));
			}
			*/

			const double newTotalSelectivity = bestCandidate->selectivity * totalSelectivity;
			const double newTotalDataCost = newTotalSelectivity * streamCardinality;
			const double newTotalIndexCost = totalIndexCost + bestCandidate->cost;
			const double totalCost = newTotalDataCost + newTotalIndexCost;

			// Test if the new totalCost will be higher than the previous totalCost
			// and if the current selectivity (without the bestCandidate) is already good enough.
			if (acceptAll || smallTable || firstCandidate ||
				(totalCost < previousTotalCost && totalSelectivity > minimumSelectivity))
			{
				// Exclude index from next pass
				bestCandidate->used = true;

				firstCandidate = false;

				previousTotalCost = totalCost;
				totalIndexCost = newTotalIndexCost;
				totalSelectivity = newTotalSelectivity;

				if (!invCandidate)
				{
					invCandidate = FB_NEW(pool) InversionCandidate(pool);
					if (!bestCandidate->inversion && bestCandidate->scratch) {
						invCandidate->inversion = makeIndexScanNode(bestCandidate->scratch);
					}
					else {
						invCandidate->inversion = bestCandidate->inversion;
					}
					invCandidate->unique = bestCandidate->unique;
					invCandidate->selectivity = bestCandidate->selectivity;
					invCandidate->cost = bestCandidate->cost;
					invCandidate->indexes = bestCandidate->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments = bestCandidate->matchedSegments;
					invCandidate->dependencies = bestCandidate->dependencies;
					invCandidate->condition = bestCandidate->condition;

					for (size_t j = 0; j < bestCandidate->matches.getCount(); j++)
					{
						if (!matches.exist(bestCandidate->matches[j]))
							matches.add(bestCandidate->matches[j]);
					}
					if (bestCandidate->boolean)
					{
						if (!matches.exist(bestCandidate->boolean))
							matches.add(bestCandidate->boolean);
					}
				}
				else if (!bestCandidate->condition)
				{
					if (!bestCandidate->inversion && bestCandidate->scratch)
					{
						invCandidate->inversion = composeInversion(invCandidate->inversion,
							makeIndexScanNode(bestCandidate->scratch), InversionNode::TYPE_AND);
					}
					else
					{
						invCandidate->inversion = composeInversion(invCandidate->inversion,
							bestCandidate->inversion, InversionNode::TYPE_AND);
					}

					invCandidate->unique = (invCandidate->unique || bestCandidate->unique);
					invCandidate->selectivity = totalSelectivity;
					invCandidate->cost += bestCandidate->cost;
					invCandidate->indexes += bestCandidate->indexes;
					invCandidate->nonFullMatchedSegments = 0;
					invCandidate->matchedSegments =
						MAX(bestCandidate->matchedSegments, invCandidate->matchedSegments);
					invCandidate->dependencies += bestCandidate->dependencies;

					for (size_t j = 0; j < bestCandidate->matches.getCount(); j++)
					{
						if (!matches.exist(bestCandidate->matches[j]))
	                        matches.add(bestCandidate->matches[j]);
					}

					if (bestCandidate->boolean)
					{
						if (!matches.exist(bestCandidate->boolean))
							matches.add(bestCandidate->boolean);
					}
				}

				if (invCandidate->unique)
				{
					// Single unique full equal match is enough
					if (!acceptAll)
						break;
				}
			}
			else
			{
				// We're done
				break;
			}
		}
		else {
			break;
		}
	}

	// If we have no index used for filtering, but there's a navigational walk,
	// set up the inversion candidate appropriately.
	if (navigationCandidate)
	{
		if (!invCandidate)
			invCandidate = FB_NEW(pool) InversionCandidate(pool);

		invCandidate->selectivity *= navigationCandidate->selectivity;
		invCandidate->cost += DEFAULT_INDEX_COST - 1 +
			navigationCandidate->cardinality * navigationCandidate->selectivity;
		++invCandidate->indexes;
		invCandidate->navigated = true;
	}

	if (invCandidate && matches.getCount())
		invCandidate->matches.join(matches);

	return invCandidate;
}

bool OptimizerRetrieval::matchBoolean(IndexScratch* indexScratch, BoolExprNode* boolean,
	USHORT scope) const
{
/**************************************
 *
 *	m a t c h B o o l e a n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	if (boolean->nodFlags & ExprNode::FLAG_DEOPTIMIZE)
		return false;

	ComparativeBoolNode* cmpNode = boolean->as<ComparativeBoolNode>();
	MissingBoolNode* missingNode = boolean->as<MissingBoolNode>();
	NotBoolNode* notNode = boolean->as<NotBoolNode>();
	RseBoolNode* rseNode = boolean->as<RseBoolNode>();
	bool forward = true;
	ValueExprNode* value = NULL;
	ValueExprNode* match = NULL;

	if (cmpNode)
	{
		match = cmpNode->arg1;
		value = cmpNode->arg2;
	}
	else if (missingNode)
		match = missingNode->arg;
	else if (notNode || rseNode)
		return false;
	else
	{
		fb_assert(false);
		return false;
	}

	ValueExprNode* value2 = (cmpNode && cmpNode->blrOp == blr_between) ?
		cmpNode->arg3 : NULL;

	if (indexScratch->idx->idx_flags & idx_expressn)
	{
		// see if one side or the other is matchable to the index expression

	    fb_assert(indexScratch->idx->idx_expression != NULL);

		if (!checkExpressionIndex(indexScratch->idx, match, stream) ||
			(value && !value->computable(csb, stream, false)))
		{
			if ((!cmpNode || cmpNode->blrOp != blr_starting) && value &&
				checkExpressionIndex(indexScratch->idx, value, stream) &&
				match->computable(csb, stream, false))
			{
				ValueExprNode* temp = match;
				match = value;
				value = temp;
				forward = false;
			}
			else
				return false;
		}
	}
	else
	{
		// If left side is not a field, swap sides.
		// If left side is still not a field, give up

		FieldNode* fieldNode;

		if (!(fieldNode = match->as<FieldNode>()) ||
			fieldNode->fieldStream != stream ||
			(value && !value->computable(csb, stream, false)))
		{
			ValueExprNode* temp = match;
			match = value;
			value = temp;

			if ((!match || !(fieldNode = match->as<FieldNode>())) ||
				fieldNode->fieldStream != stream ||
				!value->computable(csb, stream, false))
			{
				return false;
			}

			forward = false;
		}
	}

	// check datatypes to ensure that the index scan is guaranteed
	// to deliver correct results

	bool excludeBound = cmpNode && (cmpNode->blrOp == blr_gtr || cmpNode->blrOp == blr_lss);

	if (value)
	{
		dsc desc1, desc2;
		match->getDesc(tdbb, csb, &desc1);
		value->getDesc(tdbb, csb, &desc2);

		if (!BTR_types_comparable(desc1, desc2))
			return false;

		// if the indexed column is of type int64, we need to inject an
		// extra cast to deliver the scale value to the BTR level

		if (desc1.dsc_dtype == dtype_int64)
		{
			CastNode* cast = FB_NEW(*tdbb->getDefaultPool()) CastNode(*tdbb->getDefaultPool());
			cast->source = value;
			cast->castDesc = desc1;
			cast->impureOffset = CMP_impure(csb, sizeof(impure_value));

			value = cast;

			if (value2)
			{
				cast = FB_NEW(*tdbb->getDefaultPool()) CastNode(*tdbb->getDefaultPool());
				cast->source = value2;
				cast->castDesc = desc1;
				cast->impureOffset = CMP_impure(csb, sizeof(impure_value));

				value2 = cast;
			}
		}
		else if (desc1.dsc_dtype == dtype_sql_date && desc2.dsc_dtype == dtype_timestamp)
		{
			// for "DATE <op> TIMESTAMP" we need <op> to include the boundary value
			excludeBound = false;
		}
	}

	// match the field to an index, if possible, and save the value to be matched
	// as either the lower or upper bound for retrieval, or both

	const bool isDesc = (indexScratch->idx->idx_flags & idx_descending);
	int count = 0;
	IndexScratchSegment** segment = indexScratch->segments.begin();

	for (int i = 0; i < indexScratch->idx->idx_count; i++)
	{
		FieldNode* fieldNode = match->as<FieldNode>();

		if (!(indexScratch->idx->idx_flags & idx_expressn))
		{
			fb_assert(fieldNode);
		}

		if ((indexScratch->idx->idx_flags & idx_expressn) ||
			fieldNode->fieldId == indexScratch->idx->idx_rpt[i].idx_field)
		{
			if (cmpNode)
			{
				switch (cmpNode->blrOp)
				{
					case blr_between:
						if (!forward || !value2->computable(csb, stream, false))
							return false;
						segment[i]->matches.add(boolean);
						// AB: If we have already an exact match don't
						// override it with worser matches.
						if (!((segment[i]->scanType == segmentScanEqual) ||
							(segment[i]->scanType == segmentScanEquivalent)))
						{
							segment[i]->lowerValue = value;
							segment[i]->upperValue = value2;
							segment[i]->scanType = segmentScanBetween;
							segment[i]->excludeLower = false;
							segment[i]->excludeUpper = false;
						}
						break;

					case blr_equiv:
						segment[i]->matches.add(boolean);
						// AB: If we have already an exact match don't
						// override it with worser matches.
						if (!(segment[i]->scanType == segmentScanEqual))
						{
							segment[i]->lowerValue = segment[i]->upperValue = value;
							segment[i]->scanType = segmentScanEquivalent;
							segment[i]->excludeLower = false;
							segment[i]->excludeUpper = false;
						}
						break;

					case blr_eql:
						segment[i]->matches.add(boolean);
						segment[i]->lowerValue = segment[i]->upperValue = value;
						segment[i]->scanType = segmentScanEqual;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
						break;

					// ASF: Make "NOT boolean" work with indices.
					case blr_neq:
					{
						dsc desc;
						value->getDesc(tdbb, csb, &desc);

						if (desc.dsc_dtype != dtype_boolean)
							return false;

						// Let's make a compare with FALSE so we invert the value.

						static const UCHAR falseValue = '\0';
						LiteralNode* falseLiteral = FB_NEW(*tdbb->getDefaultPool())
							LiteralNode(*tdbb->getDefaultPool());
						falseLiteral->litDesc.makeBoolean(const_cast<UCHAR*>(&falseValue));

						ComparativeBoolNode* newCmp = FB_NEW(*tdbb->getDefaultPool())
							ComparativeBoolNode(*tdbb->getDefaultPool(), blr_eql);
						newCmp->arg1 = value;
						newCmp->arg2 = falseLiteral;

						// Recreate the boolean expression as a value.
						BoolAsValueNode* newValue = FB_NEW(*tdbb->getDefaultPool())
							BoolAsValueNode(*tdbb->getDefaultPool());
						newValue->boolean = newCmp;

						newValue->impureOffset = CMP_impure(csb, sizeof(impure_value));

						// We have the value inverted. Then use it with a segmentScanEqual for the
						// index lookup.
						segment[i]->matches.add(boolean);
						segment[i]->lowerValue = segment[i]->upperValue = newValue;
						segment[i]->scanType = segmentScanEqual;
						segment[i]->excludeLower = false;
						segment[i]->excludeUpper = false;
						break;
					}

					case blr_gtr:
					case blr_geq:
						segment[i]->matches.add(boolean);
						if (!((segment[i]->scanType == segmentScanEqual) ||
							(segment[i]->scanType == segmentScanEquivalent) ||
							(segment[i]->scanType == segmentScanBetween)))
						{
							if (forward != isDesc) // (forward && !isDesc || !forward && isDesc)
								segment[i]->excludeLower = excludeBound;
							else
								segment[i]->excludeUpper = excludeBound;

							if (forward)
							{
								segment[i]->lowerValue = value;
								if (segment[i]->scanType == segmentScanLess)
            						segment[i]->scanType = segmentScanBetween;
								else
            						segment[i]->scanType = segmentScanGreater;
							}
							else
							{
								segment[i]->upperValue = value;
								if (segment[i]->scanType == segmentScanGreater)
									segment[i]->scanType = segmentScanBetween;
								else
									segment[i]->scanType = segmentScanLess;
							}
						}
						break;

					case blr_lss:
					case blr_leq:
						segment[i]->matches.add(boolean);
						if (!((segment[i]->scanType == segmentScanEqual) ||
							(segment[i]->scanType == segmentScanEquivalent) ||
							(segment[i]->scanType == segmentScanBetween)))
						{
							if (forward != isDesc)
								segment[i]->excludeUpper = excludeBound;
							else
								segment[i]->excludeLower = excludeBound;

							if (forward)
							{
								segment[i]->upperValue = value;
								if (segment[i]->scanType == segmentScanGreater)
									segment[i]->scanType = segmentScanBetween;
								else
									segment[i]->scanType = segmentScanLess;
							}
							else
							{
								segment[i]->lowerValue = value;
								if (segment[i]->scanType == segmentScanLess)
            						segment[i]->scanType = segmentScanBetween;
								else
            						segment[i]->scanType = segmentScanGreater;
							}
						}
						break;

					case blr_starting:
						// Check if validate for using index
						if (!forward || !validateStarts(indexScratch, cmpNode, i))
							return false;
						segment[i]->matches.add(boolean);
						if (!((segment[i]->scanType == segmentScanEqual) ||
							(segment[i]->scanType == segmentScanEquivalent)))
						{
							segment[i]->lowerValue = segment[i]->upperValue = value;
							segment[i]->scanType = segmentScanStarting;
							segment[i]->excludeLower = false;
							segment[i]->excludeUpper = false;
						}
						break;

					default:
						return false;
				}
			}
			else if (missingNode)
			{
				segment[i]->matches.add(boolean);
				if (!((segment[i]->scanType == segmentScanEqual) ||
					(segment[i]->scanType == segmentScanEquivalent)))
				{
					segment[i]->lowerValue = segment[i]->upperValue = value;
					segment[i]->scanType = segmentScanMissing;
					segment[i]->excludeLower = false;
					segment[i]->excludeUpper = false;
				}
			}
			else
				return false;

			// A match could be made
			if (segment[i]->scope < scope)
				segment[i]->scope = scope;

			++count;

			if (i == 0)
			{
				// If this is the first segment, then this index is a candidate.
				indexScratch->candidate = true;
			}

		}
	}

	return count >= 1;
}

InversionCandidate* OptimizerRetrieval::matchDbKey(BoolExprNode* boolean) const
{
/**************************************
 *
 *	m a t c h D b K e y
 *
 **************************************
 *
 * Functional description
 *  Check whether a boolean is a DB_KEY based comparison.
 *
 **************************************/
	// If this isn't an equality, it isn't even interesting

	ComparativeBoolNode* cmpNode = boolean->as<ComparativeBoolNode>();

	if (!cmpNode || cmpNode->blrOp != blr_eql)
		return NULL;

	// Find the side of the equality that is potentially a dbkey.  If
	// neither, make the obvious deduction

	ValueExprNode* dbkey = cmpNode->arg1;
	ValueExprNode* value = cmpNode->arg2;
	const RecordKeyNode* keyNode;

	if (!((keyNode = dbkey->as<RecordKeyNode>()) && keyNode->blrOp == blr_dbkey) &&
		!dbkey->is<ConcatenateNode>())
	{
		if (!((keyNode = value->as<RecordKeyNode>()) && keyNode->blrOp == blr_dbkey) &&
			!value->is<ConcatenateNode>())
		{
			return NULL;
		}

		dbkey = value;
		value = cmpNode->arg1;
	}

	// If the value isn't computable, this has been a waste of time

	if (!value->computable(csb, stream, false))
		return NULL;

	// If this is a concatenation, find an appropriate dbkey

	SLONG n = 0;
	if (dbkey->is<ConcatenateNode>())
	{
		dbkey = findDbKey(dbkey, &n);
		if (!dbkey)
			return NULL;
	}

	// Make sure we have the correct stream

	keyNode = dbkey->as<RecordKeyNode>();
	fb_assert(keyNode && keyNode->blrOp == blr_dbkey);

	if (keyNode->recStream != stream)
		return NULL;

	// If this is a dbkey for the appropriate stream, it's invertable

	const double cardinality = csb->csb_rpt[stream].csb_cardinality;

	InversionCandidate* const invCandidate = FB_NEW(pool) InversionCandidate(pool);
	invCandidate->unique = true;
	invCandidate->selectivity = cardinality ? 1 / cardinality : DEFAULT_SELECTIVITY;
	invCandidate->cost = 1;
	invCandidate->matches.add(boolean);
	boolean->findDependentFromStreams(this, &invCandidate->dependentFromStreams);
	invCandidate->dependencies = (int) invCandidate->dependentFromStreams.getCount();

	if (createIndexScanNodes)
	{
		InversionNode* const inversion = FB_NEW(pool) InversionNode(value, n);
		inversion->impure = CMP_impure(csb, sizeof(impure_inversion));
		invCandidate->inversion = inversion;
	}

	return invCandidate;
}

InversionCandidate* OptimizerRetrieval::matchOnIndexes(
	IndexScratchList* inputIndexScratches, BoolExprNode* boolean, USHORT scope) const
{
/**************************************
 *
 *	m a t c h O n I n d e x e s
 *
 **************************************
 *
 * Functional description
 *  Try to match boolean on every index.
 *  If the boolean is an "OR" node then a
 *  inversion candidate could be returned.
 *
 **************************************/
	BinaryBoolNode* binaryNode = boolean->as<BinaryBoolNode>();

	// Handle the "OR" case up front
	if (binaryNode && binaryNode->blrOp == blr_or)
	{
		InversionCandidateList inversions;
		inversions.shrink(0);

		// Make list for index matches
		IndexScratchList indexOrScratches;

		// Copy information from caller

		size_t i = 0;

		for (; i < inputIndexScratches->getCount(); i++)
		{
			IndexScratch& scratch = (*inputIndexScratches)[i];
			indexOrScratches.add(scratch);
		}

		// We use a scope variable to see on how
		// deep we are in a nested or conjunction.
		scope++;

		InversionCandidate* invCandidate1 =
			matchOnIndexes(&indexOrScratches, binaryNode->arg1, scope);

		if (invCandidate1)
			inversions.add(invCandidate1);

		BinaryBoolNode* childBoolNode = binaryNode->arg1->as<BinaryBoolNode>();

		// Get usable inversions based on indexOrScratches and scope
		if (!childBoolNode || childBoolNode->blrOp != blr_or)
			getInversionCandidates(&inversions, &indexOrScratches, scope);

		invCandidate1 = makeInversion(&inversions);

		// Clear list to remove previously matched conjunctions
		indexOrScratches.clear();

		// Copy information from caller

		i = 0;

		for (; i < inputIndexScratches->getCount(); i++)
		{
			IndexScratch& scratch = (*inputIndexScratches)[i];
			indexOrScratches.add(scratch);
		}

		// Clear inversion list
		inversions.clear();

		InversionCandidate* invCandidate2 = matchOnIndexes(
			&indexOrScratches, binaryNode->arg2, scope);

		if (invCandidate2)
			inversions.add(invCandidate2);

		childBoolNode = binaryNode->arg2->as<BinaryBoolNode>();

		// Make inversion based on indexOrScratches and scope
		if (!childBoolNode || childBoolNode->blrOp != blr_or)
			getInversionCandidates(&inversions, &indexOrScratches, scope);

		invCandidate2 = makeInversion(&inversions);

		if (invCandidate1 && invCandidate2)
		{
			InversionCandidate* invCandidate = FB_NEW(pool) InversionCandidate(pool);
			invCandidate->inversion = composeInversion(invCandidate1->inversion,
				invCandidate2->inversion, InversionNode::TYPE_OR);
			invCandidate->selectivity = invCandidate1->selectivity + invCandidate2->selectivity -
				invCandidate1->selectivity * invCandidate2->selectivity;
			invCandidate->cost = invCandidate1->cost + invCandidate2->cost;
			invCandidate->indexes = invCandidate1->indexes + invCandidate2->indexes;
			invCandidate->nonFullMatchedSegments = 0;
			invCandidate->matchedSegments =
				MIN(invCandidate1->matchedSegments, invCandidate2->matchedSegments);
			invCandidate->dependencies = invCandidate1->dependencies + invCandidate2->dependencies;

			if (invCandidate1->condition && invCandidate2->condition)
			{
				BinaryBoolNode* const newNode =
					FB_NEW(*tdbb->getDefaultPool()) BinaryBoolNode(*tdbb->getDefaultPool(), blr_or);
				newNode->arg1 = invCandidate1->condition;
				newNode->arg2 = invCandidate2->condition;
				invCandidate->condition = newNode;
			}
			else if (invCandidate1->condition)
			{
				invCandidate->condition = invCandidate1->condition;
			}
			else if (invCandidate2->condition)
			{
				invCandidate->condition = invCandidate2->condition;
			}

			// Add matches conjunctions that exists in both left and right inversion
			if ((invCandidate1->matches.getCount()) && (invCandidate2->matches.getCount()))
			{
				SortedArray<BoolExprNode*> matches;

				for (size_t j = 0; j < invCandidate1->matches.getCount(); j++)
					matches.add(invCandidate1->matches[j]);

				for (size_t j = 0; j < invCandidate2->matches.getCount(); j++)
				{
					if (matches.exist(invCandidate2->matches[j]))
						invCandidate->matches.add(invCandidate2->matches[j]);
				}
			}

			return invCandidate;
		}

		if (invCandidate1)
		{
			invCandidate1->condition = binaryNode->arg2;
			return invCandidate1;
		}

		if (invCandidate2)
		{
			invCandidate2->condition = binaryNode->arg1;
			return invCandidate2;
		}

		return NULL;
	}

	if (binaryNode && binaryNode->blrOp == blr_and)
	{
		// Recursively call this procedure for every boolean
		// and finally get candidate inversions.
		// Normally we come here from within a OR conjunction.
		InversionCandidateList inversions;
		inversions.shrink(0);

		InversionCandidate* invCandidate = matchOnIndexes(
			inputIndexScratches, binaryNode->arg1, scope);

		if (invCandidate)
			inversions.add(invCandidate);

		invCandidate = matchOnIndexes(inputIndexScratches, binaryNode->arg2, scope);

		if (invCandidate)
			inversions.add(invCandidate);

		return makeInversion(&inversions);
	}

	// Walk through indexes
	for (size_t i = 0; i < inputIndexScratches->getCount(); i++)
	{
		IndexScratch& indexScratch = (*inputIndexScratches)[i];

		// Try to match the boolean against a index.
		if (!(indexScratch.idx->idx_runtime_flags & idx_plan_dont_use) ||
			(indexScratch.idx->idx_runtime_flags & idx_plan_navigate))
		{
			matchBoolean(&indexScratch, boolean, scope);
		}
	}

	return NULL;
}


#ifdef OPT_DEBUG_RETRIEVAL
void OptimizerRetrieval::printCandidate(const InversionCandidate* candidate) const
{
/**************************************
 *
 *	p r i n t C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "     cost(%1.2f), selectivity(%1.10f), indexes(%d), matched(%d, %d)",
		candidate->cost, candidate->selectivity, candidate->indexes, candidate->matchedSegments,
		candidate->nonFullMatchedSegments);
	if (candidate->unique)
		fprintf(opt_debug_file, ", unique");
	int depFromCount = candidate->dependentFromStreams.getCount();
	if (depFromCount >= 1)
	{
		fprintf(opt_debug_file, ", dependent from ");
		for (int i = 0; i < depFromCount; i++)
		{
			if (i == 0)
				fprintf(opt_debug_file, "%d", candidate->dependentFromStreams[i]);
			else
				fprintf(opt_debug_file, ", %d", candidate->dependentFromStreams[i]);
		}
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerRetrieval::printCandidates(const InversionCandidateList* inversions) const
{
/**************************************
 *
 *	p r i n t C a n d i d a t e s
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "    retrieval candidates:\n");
	fclose(opt_debug_file);
	const InversionCandidate* const* inversion = inversions->begin();
	for (int i = 0; i < inversions->getCount(); i++)
	{
		const InversionCandidate* candidate = inversion[i];
		printCandidate(candidate);
	}
}

void OptimizerRetrieval::printFinalCandidate(const InversionCandidate* candidate) const
{
/**************************************
 *
 *	p r i n t F i n a l C a n d i d a t e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (candidate)
	{
		FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
		fprintf(opt_debug_file, "    final candidate: ");
		fclose(opt_debug_file);
		printCandidate(candidate);
	}
}
#endif


bool OptimizerRetrieval::validateStarts(IndexScratch* indexScratch, ComparativeBoolNode* cmpNode,
	USHORT segment) const
{
/**************************************
 *
 *	v a l i d a t e S t a r t s
 *
 **************************************
 *
 * Functional description
 *  Check if the boolean is valid for
 *  using it against the given index segment.
 *
 **************************************/

	fb_assert(cmpNode && cmpNode->blrOp == blr_starting);
	if (!cmpNode || cmpNode->blrOp != blr_starting)
		return false;

	ValueExprNode* field = cmpNode->arg1;
	ValueExprNode* value = cmpNode->arg2;

	if (indexScratch->idx->idx_flags & idx_expressn)
	{
		// AB: What if the expression contains a number/float etc.. and
		// we use starting with against it? Is that allowed?
		fb_assert(indexScratch->idx->idx_expression != NULL);

		if (!(checkExpressionIndex(indexScratch->idx, field, stream) ||
			(value && !value->computable(csb, stream, false))))
		{
			// AB: Can we swap de left and right sides by a starting with?
			// X STARTING WITH 'a' that is never the same as 'a' STARTING WITH X
			if (value &&
				checkExpressionIndex(indexScratch->idx, value, stream) &&
				field->computable(csb, stream, false))
			{
				field = value;
				value = cmpNode->arg1;
			}
			else
				return false;
		}
	}
	else
	{
		FieldNode* fieldNode = field->as<FieldNode>();

		if (!fieldNode)
		{
			// dimitr:	any idea how we can use an index in this case?
			//			The code below produced wrong results.
			// AB: I don't think that it would be effective, because
			// this must include many matches (think about empty string)
			return false;
			/*
			if (!value->is<FieldNode>())
				return NULL;
			field = value;
			value = cmpNode->arg1;
			*/
		}

		// Every string starts with an empty string so don't bother using an index in that case.
		LiteralNode* literal = value->as<LiteralNode>();

		if (literal)
		{
			if ((literal->litDesc.dsc_dtype == dtype_text && literal->litDesc.dsc_length == 0) ||
				(literal->litDesc.dsc_dtype == dtype_varying &&
					literal->litDesc.dsc_length == sizeof(USHORT)))
			{
				return false;
			}
		}

		// AB: Check if the index-segment is usable for using starts.
		// Thus it should be of type string, etc...
		if (fieldNode->fieldStream != stream ||
			fieldNode->fieldId != indexScratch->idx->idx_rpt[segment].idx_field ||
			!(indexScratch->idx->idx_rpt[segment].idx_itype == idx_string ||
				indexScratch->idx->idx_rpt[segment].idx_itype == idx_byte_array ||
				indexScratch->idx->idx_rpt[segment].idx_itype == idx_metadata ||
				indexScratch->idx->idx_rpt[segment].idx_itype >= idx_first_intl_string) ||
			!value->computable(csb, stream, false))
		{
			return false;
		}
	}

	return true;
}

IndexRelationship::IndexRelationship()
{
/**************************************
 *
 *	I n d e x R e l a t i on s h i p
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	stream = 0;
	unique = false;
	cost = 0;
	cardinality = 0;
}


InnerJoinStreamInfo::InnerJoinStreamInfo(MemoryPool& p) :
	indexedRelationships(p)
{
/**************************************
 *
 *	I n n e r J o i n S t r e a m I n f o
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	stream = 0;
	baseUnique = false;
	baseCost = 0;
	baseIndexes = 0;
	baseConjunctionMatches = 0;
	baseNavigated = false;
	used = false;
	previousExpectedStreams = 0;
}

bool InnerJoinStreamInfo::independent() const
{
/**************************************
 *
 *	i n d e p e n d e n t
 *
 **************************************
 *
 *  Return true if this stream can't be
 *  used by other streams and it can't
 *  use index retrieval based on other
 *  streams.
 *
 **************************************/
	return (indexedRelationships.getCount() == 0) && (previousExpectedStreams == 0);
}


OptimizerInnerJoin::OptimizerInnerJoin(MemoryPool& p, OptimizerBlk* opt, const StreamList& streams,
									   SortNode* sort_clause, PlanNode* plan_clause)
	: pool(p), innerStreams(p)
{
/**************************************
 *
 *	O p t i m i z e r I n n e r J o i n
 *
 **************************************
 *
 *  Initialize
 *
 **************************************/
	tdbb = NULL;
	SET_TDBB(tdbb);
	this->database = tdbb->getDatabase();
	this->optimizer = opt;
	this->csb = this->optimizer->opt_csb;
	this->sort = sort_clause;
	this->plan = plan_clause;
	this->remainingStreams = 0;

	innerStreams.grow(streams.getCount());
	InnerJoinStreamInfo** innerStream = innerStreams.begin();
	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		innerStream[i] = FB_NEW(p) InnerJoinStreamInfo(p);
		innerStream[i]->stream = streams[i];
	}

	calculateCardinalities();
	calculateStreamInfo();
}

OptimizerInnerJoin::~OptimizerInnerJoin()
{
/**************************************
 *
 *	~O p t i m i z e r I n n e r J o i n
 *
 **************************************
 *
 *  Finish with giving back memory.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		for (size_t j = 0; j < innerStreams[i]->indexedRelationships.getCount(); j++)
			delete innerStreams[i]->indexedRelationships[j];

		delete innerStreams[i];
	}
}

void OptimizerInnerJoin::calculateCardinalities()
{
/**************************************
 *
 *	c a l c u l a t e C a r d i n a l i t i e s
 *
 **************************************
 *
 *  Get the cardinality for every stream.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		fb_assert(csb_tail);
		if (!csb_tail->csb_cardinality)
		{
			jrd_rel* relation = csb_tail->csb_relation;
			fb_assert(relation);
			const Format* format = CMP_format(tdbb, csb, innerStreams[i]->stream);
			fb_assert(format);
			csb_tail->csb_cardinality = OPT_getRelationCardinality(tdbb, relation, format);
		}
	}
}

void OptimizerInnerJoin::calculateStreamInfo()
{
/**************************************
 *
 *	c a l c u l a t e S t r e a m I n f o
 *
 **************************************
 *
 *  Calculate the needed information for
 *  all streams.
 *
 **************************************/

	size_t i = 0;
	// First get the base cost without any relation to an other inner join stream.
	for (i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		csb_tail->activate();

		OptimizerRetrieval optimizerRetrieval(pool, optimizer, innerStreams[i]->stream,
											  false, false, sort);
		AutoPtr<InversionCandidate> candidate(optimizerRetrieval.getCost());

		innerStreams[i]->baseCost = candidate->cost;
		innerStreams[i]->baseIndexes = candidate->indexes;
		innerStreams[i]->baseUnique = candidate->unique;
		innerStreams[i]->baseConjunctionMatches = (int) candidate->matches.getCount();
		innerStreams[i]->baseNavigated = candidate->navigated;

		csb_tail->deactivate();
	}

	for (i = 0; i < innerStreams.getCount(); i++)
	{
		CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[innerStreams[i]->stream];
		csb_tail->activate();

		// Find streams that have a indexed relationship to this
		// stream and add the information.
		for (size_t j = 0; j < innerStreams.getCount(); j++)
		{
			if (innerStreams[j]->stream != innerStreams[i]->stream)
				getIndexedRelationship(innerStreams[i], innerStreams[j]);
		}

		csb_tail->deactivate();
	}

	// Sort the streams based on independecy and cost.
	// Except when a PLAN was forced.
	if (!plan && (innerStreams.getCount() > 1))
	{
		StreamInfoList tempStreams(pool);

		for (i = 0; i < innerStreams.getCount(); i++)
		{
			size_t index = 0;
			for (; index < tempStreams.getCount(); index++)
			{
				// First those streams which can't be used by other streams
				// or can't depend on a stream.
				if (innerStreams[i]->independent() && !tempStreams[index]->independent())
					break;

				// Next those with the lowest previous expected streams
				const int compare = innerStreams[i]->previousExpectedStreams -
					tempStreams[index]->previousExpectedStreams;

				if (compare < 0)
					break;

				if (compare == 0)
				{
					// Next those with the cheapest base cost
					if (innerStreams[i]->baseCost < tempStreams[index]->baseCost)
						break;
				}
			}
			tempStreams.insert(index, innerStreams[i]);
		}

		// Finally update the innerStreams with the sorted streams
		innerStreams.clear();
		innerStreams.join(tempStreams);
	}
}

bool OptimizerInnerJoin::cheaperRelationship(IndexRelationship* checkRelationship,
		IndexRelationship* withRelationship) const
{
/**************************************
 *
 *	c h e a p e r R e l a t i o n s h i p
 *
 **************************************
 *
 *  Return true if checking relationship
 *	is cheaper as withRelationship.
 *
 **************************************/
	if (checkRelationship->cost == 0)
		return true;

	if (withRelationship->cost == 0)
		return false;

	const double compareValue = checkRelationship->cost / withRelationship->cost;
	if (compareValue >= 0.98 && compareValue <= 1.02)
	{
		// cost is nearly the same, now check on cardinality
		if (checkRelationship->cardinality < withRelationship->cardinality)
			return true;
	}
	else if (checkRelationship->cost < withRelationship->cost)
		return true;

	return false;
}

void OptimizerInnerJoin::estimateCost(StreamType stream, double* cost,
	double* resulting_cardinality, bool start) const
{
/**************************************
 *
 *	e s t i m a t e C o s t
 *
 **************************************
 *
 *  Estimate the cost for the stream.
 *
 **************************************/
	// Create the optimizer retrieval generation class and calculate
	// which indexes will be used and the total estimated selectivity will be returned
	OptimizerRetrieval optimizerRetrieval(pool, optimizer, stream, false, false,
										  (start ? sort : NULL));
	AutoPtr<const InversionCandidate> candidate(optimizerRetrieval.getCost());

	*cost = candidate->cost;

	// Calculate cardinality
	const CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[stream];
	const double cardinality = csb_tail->csb_cardinality * candidate->selectivity;

	*resulting_cardinality = MAX(cardinality, MINIMUM_CARDINALITY);
}

StreamType OptimizerInnerJoin::findJoinOrder()
{
/**************************************
 *
 *	f i n d J o i n O r d e r
 *
 **************************************
 *
 *  Find the best order out of the streams.
 *  First return a stream if it can't use
 *  an index based on a previous stream and
 *  it can't be used by another stream.
 *  Next loop through the remaining streams
 *  and find the best order.
 *
 **************************************/

	optimizer->opt_best_count = 0;

#ifdef OPT_DEBUG
	// Debug
	printStartOrder();
#endif

	unsigned navigations = 0;

	size_t i = 0;
	remainingStreams = 0;

	for (i = 0; i < innerStreams.getCount(); i++)
	{
		if (!innerStreams[i]->used)
		{
			remainingStreams++;

			if (innerStreams[i]->baseNavigated)
				navigations++;

			if (innerStreams[i]->independent())
			{
				if (!optimizer->opt_best_count || innerStreams[i]->baseCost < optimizer->opt_best_cost)
				{
					optimizer->opt_streams[0].opt_best_stream = innerStreams[i]->stream;
					optimizer->opt_best_count = 1;
					optimizer->opt_best_cost = innerStreams[i]->baseCost;
				}
			}
		}
	}

	if (optimizer->opt_best_count == 0)
	{
		IndexedRelationships indexedRelationships(pool);

		for (i = 0; i < innerStreams.getCount(); i++)
		{
			if (!innerStreams[i]->used)
			{
				// If optimization for first rows has been requested and index navigations are possible,
				// then consider only join orders starting with a navigational stream

				if (!optimizer->optimizeFirstRows || !navigations || innerStreams[i]->baseNavigated)
				{
					indexedRelationships.clear();
					findBestOrder(0, innerStreams[i], &indexedRelationships, 0.0, 1.0);

					if (plan)
					{
						// If a explicit PLAN was specified we should be ready;
						break;
					}
				}
#ifdef OPT_DEBUG
				// Debug
				printProcessList(&indexedRelationships, innerStreams[i]->stream);
#endif
			}
		}
	}

	// Mark streams as used
	for (StreamType stream = 0; stream < optimizer->opt_best_count; stream++)
	{
		InnerJoinStreamInfo* streamInfo = getStreamInfo(optimizer->opt_streams[stream].opt_best_stream);
		streamInfo->used = true;
	}

#ifdef OPT_DEBUG
	// Debug
	printBestOrder();
#endif

	return optimizer->opt_best_count;
}

void OptimizerInnerJoin::findBestOrder(StreamType position, InnerJoinStreamInfo* stream,
	IndexedRelationships* processList, double cost, double cardinality)
{
/**************************************
 *
 *	f i n d B e s t O r d e r
 *
 **************************************
 *  Make different combinations to find
 *  out the join order.
 *  For every position we start with the
 *  stream that has the best selectivity
 *  for that position. If we've have
 *  used up all our streams after that
 *  we assume we're done.
 *
 **************************************/

	fb_assert(processList);

	const bool start = (position == 0);

	// do some initializations.
	csb->csb_rpt[stream->stream].activate();
	optimizer->opt_streams[position].opt_stream_number = stream->stream;
	position++;
	const OptimizerBlk::opt_stream* order_end = optimizer->opt_streams.begin() + position;

	// Save the various flag bits from the optimizer block to reset its
	// state after each test.
	HalfStaticArray<bool, OPT_STATIC_ITEMS> streamFlags(pool);
	streamFlags.grow(innerStreams.getCount());
	for (size_t i = 0; i < streamFlags.getCount(); i++)
		streamFlags[i] = innerStreams[i]->used;

	// Compute delta and total estimate cost to fetch this stream.
	double position_cost, position_cardinality, new_cost = 0, new_cardinality = 0;

	if (!plan)
	{
		estimateCost(stream->stream, &position_cost, &position_cardinality, start);
		new_cost = cost + cardinality * position_cost;
		new_cardinality = position_cardinality * cardinality;
	}

	// If the partial order is either longer than any previous partial order,
	// or the same length and cheap, save order as "best".
	if (position > optimizer->opt_best_count ||
		(position == optimizer->opt_best_count && new_cost < optimizer->opt_best_cost))
	{
		optimizer->opt_best_count = position;
		optimizer->opt_best_cost = new_cost;
		for (OptimizerBlk::opt_stream* tail = optimizer->opt_streams.begin(); tail < order_end; tail++)
			tail->opt_best_stream = tail->opt_stream_number;
	}

#ifdef OPT_DEBUG
	// Debug information
	printFoundOrder(position, position_cost, position_cardinality, new_cost, new_cardinality);
#endif

	// mark this stream as "used" in the sense that it is already included
	// in this particular proposed stream ordering.
	stream->used = true;
	bool done = false;

	// if we've used up all the streams there's no reason to go any further.
	if (position == remainingStreams)
		done = true;

	// If we know a combination with all streams used and the
	// current cost is higher as the one from the best we're done.
	if ((optimizer->opt_best_count == remainingStreams) && (optimizer->opt_best_cost < new_cost))
		done = true;

	if (!done && !plan)
	{
		// Add these relations to the processing list
		for (size_t j = 0; j < stream->indexedRelationships.getCount(); j++)
		{
			IndexRelationship* relationship = stream->indexedRelationships[j];
			InnerJoinStreamInfo* relationStreamInfo = getStreamInfo(relationship->stream);
			if (!relationStreamInfo->used)
			{
				bool found = false;
				IndexRelationship** processRelationship = processList->begin();
				size_t index;
				for (index = 0; index < processList->getCount(); index++)
				{
					if (relationStreamInfo->stream == processRelationship[index]->stream)
					{
						// If the cost of this relationship is cheaper then remove the
						// old relationship and add this one.
						if (cheaperRelationship(relationship, processRelationship[index]))
						{
							processList->remove(index);
							break;
						}

						found = true;
						break;
					}
				}
				if (!found)
				{
					// Add relationship sorted on cost (cheapest as first)
					IndexRelationship** relationships = processList->begin();
					for (index = 0; index < processList->getCount(); index++)
					{
						if (cheaperRelationship(relationship, relationships[index]))
							break;
					}
					processList->insert(index, relationship);
				}
			}
		}

		IndexRelationship** nextRelationship = processList->begin();
		for (size_t j = 0; j < processList->getCount(); j++)
		{
			InnerJoinStreamInfo* relationStreamInfo = getStreamInfo(nextRelationship[j]->stream);
			if (!relationStreamInfo->used)
			{
				findBestOrder(position, relationStreamInfo, processList, new_cost, new_cardinality);
				break;
			}
		}
	}

	if (plan)
	{
		// If a explicit PLAN was specific pick the next relation.
		// The order in innerStreams is expected to be exactly the order as
		// specified in the explicit PLAN.
		for (size_t j = 0; j < innerStreams.getCount(); j++)
		{
			InnerJoinStreamInfo* nextStream = innerStreams[j];
			if (!nextStream->used)
			{
				findBestOrder(position, nextStream, processList, new_cost, new_cardinality);
				break;
			}
		}
	}

	// Clean up from any changes made for compute the cost for this stream
	csb->csb_rpt[stream->stream].deactivate();
	for (size_t i = 0; i < streamFlags.getCount(); i++)
		innerStreams[i]->used = streamFlags[i];
}

void OptimizerInnerJoin::getIndexedRelationship(InnerJoinStreamInfo* baseStream,
	InnerJoinStreamInfo* testStream)
{
/**************************************
 *
 *	g e t I n d e x e d R e l a t i o n s h i p
 *
 **************************************
 *
 *  Check if the testStream can use a index
 *  when the baseStream is active. If so
 *  then we create a indexRelationship
 *  and fill it with the needed information.
 *  The reference is added to the baseStream
 *  and the baseStream is added as previous
 *  expected stream to the testStream.
 *
 **************************************/

	CompilerScratch::csb_repeat* csb_tail = &csb->csb_rpt[testStream->stream];
	csb_tail->activate();

	OptimizerRetrieval optimizerRetrieval(pool, optimizer, testStream->stream, false, false, NULL);
	AutoPtr<InversionCandidate> candidate(optimizerRetrieval.getCost());

	if (candidate->dependentFromStreams.exist(baseStream->stream))
	{
		// If we could use more conjunctions on the testing stream
		// with the base stream active as without the base stream
		// then the test stream has a indexed relationship with the base stream.
		IndexRelationship* indexRelationship = FB_NEW(pool) IndexRelationship();
		indexRelationship->stream = testStream->stream;
		indexRelationship->unique = candidate->unique;
		indexRelationship->cost = candidate->cost;
		indexRelationship->cardinality = csb_tail->csb_cardinality * candidate->selectivity;

		// indexRelationship are kept sorted on cost and unique in the indexRelations array.
		// The unique and cheapest indexed relatioships are on the first position.
		size_t index = 0;
		for (; index < baseStream->indexedRelationships.getCount(); index++)
		{
			if (cheaperRelationship(indexRelationship, baseStream->indexedRelationships[index]))
				break;
		}
		baseStream->indexedRelationships.insert(index, indexRelationship);
		testStream->previousExpectedStreams++;
	}

	csb_tail->deactivate();
}

InnerJoinStreamInfo* OptimizerInnerJoin::getStreamInfo(StreamType stream)
{
/**************************************
 *
 *	g e t S t r e a m I n f o
 *
 **************************************
 *
 *  Return stream information based on
 *  the stream number.
 *
 **************************************/

	for (size_t i = 0; i < innerStreams.getCount(); i++)
	{
		if (innerStreams[i]->stream == stream)
			return innerStreams[i];
	}

	// We should never come here
	fb_assert(false);
	return NULL;
}

#ifdef OPT_DEBUG
void OptimizerInnerJoin::printBestOrder() const
{
/**************************************
 *
 *	p r i n t B e s t O r d e r
 *
 **************************************
 *
 *  Dump finally selected stream order.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, " best order, streams: ");
	for (StreamType i = 0; i < optimizer->opt_best_count; i++)
	{
		if (i == 0)
			fprintf(opt_debug_file, "%d", optimizer->opt_streams[i].opt_best_stream);
		else
			fprintf(opt_debug_file, ", %d", optimizer->opt_streams[i].opt_best_stream);
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printFoundOrder(StreamType position, double positionCost,
		double positionCardinality, double cost, double cardinality) const
{
/**************************************
 *
 *	p r i n t F o u n d O r d e r
 *
 **************************************
 *
 *  Dump currently passed streams to a
 *  debug file.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "  position %2.2d:", position);
	fprintf(opt_debug_file, " pos. cardinality(%10.2f) pos. cost(%10.2f)", positionCardinality, positionCost);
	fprintf(opt_debug_file, " cardinality(%10.2f) cost(%10.2f)", cardinality, cost);
	fprintf(opt_debug_file, ", streams: ", position);
	const OptimizerBlk::opt_stream* tail = optimizer->opt_streams.begin();
	const OptimizerBlk::opt_stream* const order_end = tail + position;
	for (; tail < order_end; tail++)
	{
		if (tail == optimizer->opt_streams.begin())
			fprintf(opt_debug_file, "%d", tail->opt_stream_number);
		else
			fprintf(opt_debug_file, ", %d", tail->opt_stream_number);
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printProcessList(const IndexedRelationships* processList,
	StreamType stream) const
{
/**************************************
 *
 *	p r i n t P r o c e s s L i s t
 *
 **************************************
 *
 *  Dump the processlist to a debug file.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "   basestream %d, relationships: stream(cost)", stream);
	const IndexRelationship* const* relationships = processList->begin();
	for (int i = 0; i < processList->getCount(); i++)
		fprintf(opt_debug_file, ", %d (%1.2f)", relationships[i]->stream, relationships[i]->cost);
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}

void OptimizerInnerJoin::printStartOrder() const
{
/**************************************
 *
 *	p r i n t B e s t O r d e r
 *
 **************************************
 *
 *  Dump finally selected stream order.
 *
 **************************************/

	FILE *opt_debug_file = fopen(OPTIMIZER_DEBUG_FILE, "a");
	fprintf(opt_debug_file, "Start join order: with stream(baseCost)");
	bool firstStream = true;
	for (int i = 0; i < innerStreams.getCount(); i++)
	{
		if (!innerStreams[i]->used)
			fprintf(opt_debug_file, ", %d (%1.2f)", innerStreams[i]->stream, innerStreams[i]->baseCost);
	}
	fprintf(opt_debug_file, "\n");
	fclose(opt_debug_file);
}
#endif


} // namespace
