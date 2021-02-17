/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		Optimizer.h
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
 *
 *
 */

#ifndef OPTIMIZER_H
#define OPTIMIZER_H

//#define OPT_DEBUG
//#define OPT_DEBUG_RETRIEVAL

//#ifdef OPT_DEBUG
#define OPTIMIZER_DEBUG_FILE "opt_debug.out"
//#endif

#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../jrd/RecordSourceNodes.h"
#include "../jrd/rse.h"
#include "../jrd/exe.h"

namespace Jrd {


// AB: 2005-11-05
// Constants below needs some discussions and ideas
const double REDUCE_SELECTIVITY_FACTOR_BETWEEN = 0.0025;
const double REDUCE_SELECTIVITY_FACTOR_LESS = 0.05;
const double REDUCE_SELECTIVITY_FACTOR_GREATER = 0.05;
const double REDUCE_SELECTIVITY_FACTOR_STARTING = 0.01;

const double REDUCE_SELECTIVITY_FACTOR_EQUALITY = 0.1;
const double REDUCE_SELECTIVITY_FACTOR_INEQUALITY = 0.3;

const double MAXIMUM_SELECTIVITY = 1.0;
const double DEFAULT_SELECTIVITY = 0.1;

const double MINIMUM_CARDINALITY = 1.0;
const double THRESHOLD_CARDINALITY = 5.0;

// Default depth of an index tree (including one leaf page),
// also representing the minimal cost of the index scan.
// We assume that the root page would be always cached,
// so it's not included here.
const int DEFAULT_INDEX_COST = 3;


struct index_desc;
class OptimizerBlk;
class jrd_rel;
class IndexTableScan;
class ComparativeBoolNode;
class InversionNode;
class PlanNode;
class SortNode;

double OPT_getRelationCardinality(thread_db*, jrd_rel*, const Format*);
Firebird::string OPT_make_alias(thread_db*, const CompilerScratch*, const CompilerScratch::csb_repeat*);

enum segmentScanType {
	segmentScanNone,
	segmentScanGreater,
	segmentScanLess,
	segmentScanBetween,
	segmentScanEqual,
	segmentScanEquivalent,
	segmentScanMissing,
	segmentScanStarting
};

class IndexScratchSegment
{
public:
	explicit IndexScratchSegment(MemoryPool& p);
	IndexScratchSegment(MemoryPool& p, IndexScratchSegment* segment);


	ValueExprNode* lowerValue;	// lower bound on index value
	ValueExprNode* upperValue;	// upper bound on index value
	bool excludeLower;			// exclude lower bound value from scan
	bool excludeUpper;			// exclude upper bound value from scan
	int scope;					// highest scope level
	segmentScanType scanType;	// scan type

	Firebird::Array<BoolExprNode*> matches;
};

class IndexScratch
{
public:
	IndexScratch(MemoryPool& p, thread_db* tdbb, index_desc* idx, CompilerScratch::csb_repeat* csb_tail);
	IndexScratch(MemoryPool& p, const IndexScratch& scratch);
	~IndexScratch();

	index_desc*	idx;				// index descriptor
	double selectivity;				// calculated selectivity for this index
	bool candidate;					// used when deciding which indices to use
	bool scopeCandidate;			// used when making inversion based on scope
	int lowerCount;					//
	int upperCount;					//
	int nonFullMatchedSegments;		//
	bool fuzzy;						// Need to use INTL_KEY_PARTIAL in btr lookups
	double cardinality;				// Estimated cardinality when using the whole index

	Firebird::Array<IndexScratchSegment*> segments;
};

class InversionCandidate
{
public:
	explicit InversionCandidate(MemoryPool& p);

	double			selectivity;
	double			cost;
	USHORT			nonFullMatchedSegments;
	USHORT			matchedSegments;
	int				indexes;
	int				dependencies;
	BoolExprNode*	boolean;
	BoolExprNode*	condition;
	InversionNode*	inversion;
	IndexScratch*	scratch;
	bool			used;
	bool			unique;
	bool			navigated;

	Firebird::Array<BoolExprNode*> matches;
	SortedStreamList dependentFromStreams;
};

typedef Firebird::HalfStaticArray<InversionCandidate*, 16> InversionCandidateList;
typedef Firebird::ObjectsArray<IndexScratch> IndexScratchList;

class OptimizerRetrieval
{
public:
	OptimizerRetrieval(MemoryPool& p, OptimizerBlk* opt, StreamType streamNumber,
		bool outer, bool inner, SortNode* sortNode);
	~OptimizerRetrieval();

	InversionCandidate* getInversion()
	{
		createIndexScanNodes = true;
		setConjunctionsMatched = true;

		return generateInversion();
	}

	InversionCandidate* getCost()
	{
		createIndexScanNodes = false;
		setConjunctionsMatched = false;

		return generateInversion();
	}

	IndexTableScan* getNavigation();

protected:
	void analyzeNavigation();
	InversionNode* composeInversion(InversionNode* node1, InversionNode* node2,
		InversionNode::Type node_type) const;
	const Firebird::string& getAlias();
	InversionCandidate* generateInversion();
	void getInversionCandidates(InversionCandidateList* inversions,
		IndexScratchList* indexScratches, USHORT scope) const;
	InversionNode* makeIndexNode(const index_desc* idx) const;
	InversionNode* makeIndexScanNode(IndexScratch* indexScratch) const;
	InversionCandidate* makeInversion(InversionCandidateList* inversions) const;
	bool matchBoolean(IndexScratch* indexScratch, BoolExprNode* boolean, USHORT scope) const;
	InversionCandidate* matchDbKey(BoolExprNode* boolean) const;
	InversionCandidate* matchOnIndexes(IndexScratchList* indexScratches,
		BoolExprNode* boolean, USHORT scope) const;
	ValueExprNode* findDbKey(ValueExprNode* dbkey, SLONG* position) const;

#ifdef OPT_DEBUG_RETRIEVAL
	void printCandidate(const InversionCandidate* candidate) const;
	void printCandidates(const InversionCandidateList* inversions) const;
	void printFinalCandidate(const InversionCandidate* candidate) const;
#endif

	bool validateStarts(IndexScratch* indexScratch, ComparativeBoolNode* cmpNode,
		USHORT segment) const;

private:
	MemoryPool& pool;
	thread_db* tdbb;

public:
	StreamType stream;
	Firebird::string alias;
	SortNode* sort;
	jrd_rel* relation;
	CompilerScratch* csb;
	Database* database;
	OptimizerBlk* optimizer;
	IndexScratchList indexScratches;
	InversionCandidateList inversionCandidates;
	bool innerFlag;
	bool outerFlag;
	bool createIndexScanNodes;
	bool setConjunctionsMatched;
	IndexScratch* navigationCandidate;
};

class IndexRelationship
{
public:
	IndexRelationship();

	StreamType stream;
	bool unique;
	double cost;
	double cardinality;
};

typedef Firebird::Array<IndexRelationship*> IndexedRelationships;

class InnerJoinStreamInfo
{
public:
	explicit InnerJoinStreamInfo(MemoryPool& p);
	bool independent() const;

	StreamType stream;
	bool baseUnique;
	double baseCost;
	int baseIndexes;
	int baseConjunctionMatches;
	bool baseNavigated;
	bool used;

	IndexedRelationships indexedRelationships;
	int previousExpectedStreams;
};

typedef Firebird::HalfStaticArray<InnerJoinStreamInfo*, 8> StreamInfoList;

class OptimizerInnerJoin
{
public:
	OptimizerInnerJoin(MemoryPool& p, OptimizerBlk* opt, const StreamList& streams,
					   SortNode* sort_clause, PlanNode* plan_clause);
	~OptimizerInnerJoin();

	StreamType findJoinOrder();

protected:
	void calculateCardinalities();
	void calculateStreamInfo();
	bool cheaperRelationship(IndexRelationship* checkRelationship,
		IndexRelationship* withRelationship) const;
	void estimateCost(StreamType stream, double* cost, double* resulting_cardinality, bool start) const;
	void findBestOrder(StreamType position, InnerJoinStreamInfo* stream,
		IndexedRelationships* processList, double cost, double cardinality);
	void getIndexedRelationship(InnerJoinStreamInfo* baseStream, InnerJoinStreamInfo* testStream);
	InnerJoinStreamInfo* getStreamInfo(StreamType stream);
#ifdef OPT_DEBUG
	void printBestOrder() const;
	void printFoundOrder(StreamType position, double positionCost,
		double positionCardinality, double cost, double cardinality) const;
	void printProcessList(const IndexedRelationships* processList, StreamType stream) const;
	void printStartOrder() const;
#endif

private:
	MemoryPool& pool;
	thread_db* tdbb;
	SortNode* sort;
	PlanNode* plan;
	CompilerScratch* csb;
	Database* database;
	OptimizerBlk* optimizer;
	StreamInfoList innerStreams;
	StreamType remainingStreams;
};

class StreamStateHolder
{
public:
	explicit StreamStateHolder(CompilerScratch* csb)
		: m_csb(csb), m_streams(csb->csb_pool), m_flags(csb->csb_pool)
	{
		for (StreamType stream = 0; stream < csb->csb_n_stream; stream++)
			m_streams.add(stream);

		init();
	}

	StreamStateHolder(CompilerScratch* csb, const StreamList& streams)
		: m_csb(csb), m_streams(csb->csb_pool), m_flags(csb->csb_pool)
	{
		m_streams.assign(streams);

		init();
	}

	~StreamStateHolder()
	{
		for (size_t i = 0; i < m_streams.getCount(); i++)
		{
			const StreamType stream = m_streams[i];

			if (m_flags[i >> 3] & (1 << (i & 7)))
				m_csb->csb_rpt[stream].activate();
			else
				m_csb->csb_rpt[stream].deactivate();
		}
	}

	void activate()
	{
		for (const StreamType* iter = m_streams.begin(); iter != m_streams.end(); ++iter)
			m_csb->csb_rpt[*iter].activate();
	}

	void deactivate()
	{
		for (const StreamType* iter = m_streams.begin(); iter != m_streams.end(); ++iter)
			m_csb->csb_rpt[*iter].deactivate();
	}

private:
	void init()
	{
		m_flags.resize(FLAG_BYTES(m_streams.getCount()));

		for (size_t i = 0; i < m_streams.getCount(); i++)
		{
			const StreamType stream = m_streams[i];

			if (m_csb->csb_rpt[stream].csb_flags & csb_active)
				m_flags[i >> 3] |= (1 << (i & 7));
		}
	}

	CompilerScratch* const m_csb;
	StreamList m_streams;
	Firebird::HalfStaticArray<UCHAR, sizeof(SLONG)> m_flags;
};

} // namespace Jrd

#endif // OPTIMIZER_H
