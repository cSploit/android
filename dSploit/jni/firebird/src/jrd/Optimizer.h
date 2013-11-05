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
const double REDUCE_SELECTIVITY_FACTOR_INEQUALITY = 0.5;

const double MAXIMUM_SELECTIVITY = 1.0;
const double DEFAULT_SELECTIVITY = 0.1;

const double MINIMUM_CARDINALITY = 1.0;
const double THRESHOLD_CARDINALITY = 5.0;

// Default (Minimum) cost (nr. of pages) for an index.
const int DEFAULT_INDEX_COST = 1;


class jrd_nod;
struct index_desc;
class OptimizerBlk;
class jrd_rel;

bool OPT_computable(CompilerScratch*, const jrd_nod*, SSHORT, const bool, const bool);
void OPT_compute_rse_streams(const RecordSelExpr*, UCHAR*);
bool OPT_expression_equal(thread_db*, OptimizerBlk*, const index_desc*, jrd_nod*, USHORT);
bool OPT_expression_equal2(thread_db*, OptimizerBlk*, jrd_nod*, jrd_nod*, USHORT);
void OPT_get_expression_streams(const jrd_nod*, Firebird::SortedArray<int>&);
double OPT_getRelationCardinality(thread_db*, jrd_rel*, const Format*);
VaryingString* OPT_make_alias(thread_db*, const CompilerScratch*, const CompilerScratch::csb_repeat*);
jrd_nod* OPT_make_binary_node(nod_t, jrd_nod*, jrd_nod*, bool);
USHORT OPT_nav_rsb_size(RecordSource*, USHORT, USHORT);

inline int STREAM_INDEX(const jrd_nod* node)
{
	switch (node->nod_type)
	{
		case nod_relation:
			return e_rel_stream;
		case nod_procedure:
			return e_prc_stream;
		case nod_union:
			return e_uni_stream;
		case nod_aggregate:
			return e_agg_stream;
		default:
			fb_assert(false);
			return 0; // silence compiler warning.
	}
}

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


	jrd_nod* lowerValue;		// lower bound on index value
	jrd_nod* upperValue;		// upper bound on index value
	bool excludeLower;			// exclude lower bound value from scan
	bool excludeUpper;			// exclude upper bound value from scan
	int scope;					// highest scope level
	segmentScanType scanType;	// scan type

	Firebird::Array<jrd_nod*> matches;
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

typedef Firebird::SortedArray<int> SortedStreamList;

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
	jrd_nod*		boolean;
	jrd_nod*		inversion;
	IndexScratch*	scratch;
	bool			used;
	bool			unique;

	Firebird::Array<jrd_nod*> matches;
	SortedStreamList dependentFromStreams;
};

typedef Firebird::HalfStaticArray<InversionCandidate*, 16> InversionCandidateList;
typedef Firebird::ObjectsArray<IndexScratch> IndexScratchList;

class OptimizerRetrieval
{
public:
	OptimizerRetrieval(MemoryPool& p, OptimizerBlk* opt, SSHORT streamNumber,
		bool outer, bool inner, jrd_nod** sortNode);
	~OptimizerRetrieval();

	InversionCandidate* getInversion(RecordSource** rsb);

	InversionCandidate* getCost()
	{
		return getInversion(NULL);
	}

protected:
	jrd_nod* composeInversion(jrd_nod* node1, jrd_nod* node2, nod_t node_type) const;
	void findDependentFromStreams(const jrd_nod* node, SortedStreamList* streamList) const;
	VaryingString* getAlias();
	InversionCandidate* generateInversion(RecordSource** rsb);
	RecordSource* generateNavigation();
	void getInversionCandidates(InversionCandidateList* inversions,
		IndexScratchList* indexScratches, USHORT scope) const;
	jrd_nod* makeIndexNode(const index_desc* idx) const;
	jrd_nod* makeIndexScanNode(IndexScratch* indexScratch) const;
	InversionCandidate* makeInversion(InversionCandidateList* inversions) const;
	bool matchBoolean(IndexScratch* indexScratch, jrd_nod* boolean, USHORT scope) const;
	InversionCandidate* matchOnIndexes(IndexScratchList* indexScratches,
		jrd_nod* boolean, USHORT scope) const;

#ifdef OPT_DEBUG_RETRIEVAL
	void printCandidate(const InversionCandidate* candidate) const;
	void printCandidates(const InversionCandidateList* inversions) const;
	void printFinalCandidate(const InversionCandidate* candidate) const;
#endif

	bool validateStarts(IndexScratch* indexScratch, jrd_nod* boolean, USHORT segment) const;
private:
	MemoryPool& pool;
	thread_db* tdbb;
	SSHORT stream;
	VaryingString* alias;
	jrd_nod** sort;
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
};

class IndexRelationship
{
public:
	IndexRelationship();

	int		stream;
	bool	unique;
	double	cost;
	double	cardinality;
};

typedef Firebird::Array<IndexRelationship*> IndexedRelationships;

class InnerJoinStreamInfo
{
public:
	explicit InnerJoinStreamInfo(MemoryPool& p);
	bool independent() const;

	int		stream;
	bool	baseUnique;
	double	baseCost;
	int		baseIndexes;
	int		baseConjunctionMatches;
	bool	used;

	IndexedRelationships indexedRelationships;
	int previousExpectedStreams;
};

typedef Firebird::HalfStaticArray<InnerJoinStreamInfo*, 8> StreamInfoList;

class OptimizerInnerJoin
{
public:
	OptimizerInnerJoin(MemoryPool& p, OptimizerBlk* opt, const UCHAR* streams,
		/*RiverStack& river_stack,*/ jrd_nod** sort_clause, jrd_nod** project_clause, jrd_nod* plan_clause);
	~OptimizerInnerJoin();

	int findJoinOrder();

protected:
	void calculateCardinalities();
	void calculateStreamInfo();
	bool cheaperRelationship(IndexRelationship* checkRelationship,
		IndexRelationship* withRelationship) const;
	void estimateCost(USHORT stream, double* cost, double* resulting_cardinality) const;
	void findBestOrder(int position, InnerJoinStreamInfo* stream,
		IndexedRelationships* processList, double cost, double cardinality);
	void getIndexedRelationship(InnerJoinStreamInfo* baseStream, InnerJoinStreamInfo* testStream);
	InnerJoinStreamInfo* getStreamInfo(int stream);
#ifdef OPT_DEBUG
	void printBestOrder() const;
	void printFoundOrder(int position, double positionCost,
		double positionCardinality, double cost, double cardinality) const;
	void printProcessList(const IndexedRelationships* processList, int stream) const;
	void printStartOrder() const;
#endif

private:
	MemoryPool& pool;
	thread_db* tdbb;
	jrd_nod** sort;
	jrd_nod** project;
	jrd_nod* plan;
	CompilerScratch* csb;
	Database* database;
	OptimizerBlk* optimizer;
	StreamInfoList innerStreams;
	int remainingStreams;
};

typedef UCHAR stream_array_t[MAX_STREAMS + 1];

} // namespace Jrd

#endif // OPTIMIZER_H
