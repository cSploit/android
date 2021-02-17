/*-------------------------------------------------------------------------
 *
 * indxpath.c
 *	  Routines to determine which indexes are usable for scanning a
 *	  given relation, and create Paths accordingly.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/optimizer/path/indxpath.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <math.h>

#include "access/skey.h"
#include "catalog/pg_am.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_opfamily.h"
#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/predtest.h"
#include "optimizer/restrictinfo.h"
#include "optimizer/var.h"
#include "utils/builtins.h"
#include "utils/bytea.h"
#include "utils/lsyscache.h"
#include "utils/pg_locale.h"
#include "utils/selfuncs.h"


#define IsBooleanOpfamily(opfamily) \
	((opfamily) == BOOL_BTREE_FAM_OID || (opfamily) == BOOL_HASH_FAM_OID)

#define IndexCollMatchesExprColl(idxcollation, exprcollation) \
	((idxcollation) == InvalidOid || (idxcollation) == (exprcollation))

/* Whether to use ScalarArrayOpExpr to build index qualifications */
typedef enum
{
	SAOP_FORBID,				/* Do not use ScalarArrayOpExpr */
	SAOP_ALLOW,					/* OK to use ScalarArrayOpExpr */
	SAOP_REQUIRE				/* Require ScalarArrayOpExpr */
} SaOpControl;

/* Whether we are looking for plain indexscan, bitmap scan, or either */
typedef enum
{
	ST_INDEXSCAN,				/* must support amgettuple */
	ST_BITMAPSCAN,				/* must support amgetbitmap */
	ST_ANYSCAN					/* either is okay */
} ScanTypeControl;

/* Per-path data used within choose_bitmap_and() */
typedef struct
{
	Path	   *path;			/* IndexPath, BitmapAndPath, or BitmapOrPath */
	List	   *quals;			/* the WHERE clauses it uses */
	List	   *preds;			/* predicates of its partial index(es) */
	Bitmapset  *clauseids;		/* quals+preds represented as a bitmapset */
} PathClauseUsage;


static List *find_usable_indexes(PlannerInfo *root, RelOptInfo *rel,
					List *clauses, List *outer_clauses,
					bool istoplevel, RelOptInfo *outer_rel,
					SaOpControl saop_control, ScanTypeControl scantype);
static List *find_saop_paths(PlannerInfo *root, RelOptInfo *rel,
				List *clauses, List *outer_clauses,
				bool istoplevel, RelOptInfo *outer_rel);
static Path *choose_bitmap_and(PlannerInfo *root, RelOptInfo *rel,
				  List *paths, RelOptInfo *outer_rel);
static int	path_usage_comparator(const void *a, const void *b);
static Cost bitmap_scan_cost_est(PlannerInfo *root, RelOptInfo *rel,
					 Path *ipath, RelOptInfo *outer_rel);
static Cost bitmap_and_cost_est(PlannerInfo *root, RelOptInfo *rel,
					List *paths, RelOptInfo *outer_rel);
static PathClauseUsage *classify_index_clause_usage(Path *path,
							List **clauselist);
static void find_indexpath_quals(Path *bitmapqual, List **quals, List **preds);
static int	find_list_position(Node *node, List **nodelist);
static List *group_clauses_by_indexkey(IndexOptInfo *index,
						  List *clauses, List *outer_clauses,
						  Relids outer_relids,
						  SaOpControl saop_control,
						  bool *found_clause);
static bool match_clause_to_indexcol(IndexOptInfo *index,
						 int indexcol,
						 RestrictInfo *rinfo,
						 Relids outer_relids,
						 SaOpControl saop_control);
static bool is_indexable_operator(Oid expr_op, Oid opfamily,
					  bool indexkey_on_left);
static bool match_rowcompare_to_indexcol(IndexOptInfo *index,
							 int indexcol,
							 Oid opfamily,
							 Oid idxcollation,
							 RowCompareExpr *clause,
							 Relids outer_relids);
static List *match_index_to_pathkeys(IndexOptInfo *index, List *pathkeys);
static Expr *match_clause_to_ordering_op(IndexOptInfo *index,
							int indexcol, Expr *clause, Oid pk_opfamily);
static Relids indexable_outerrelids(PlannerInfo *root, RelOptInfo *rel);
static bool matches_any_index(RestrictInfo *rinfo, RelOptInfo *rel,
				  Relids outer_relids);
static List *find_clauses_for_join(PlannerInfo *root, RelOptInfo *rel,
					  Relids outer_relids, bool isouterjoin);
static bool match_boolean_index_clause(Node *clause, int indexcol,
						   IndexOptInfo *index);
static bool match_special_index_operator(Expr *clause,
							 Oid opfamily, Oid idxcollation,
							 bool indexkey_on_left);
static Expr *expand_boolean_index_clause(Node *clause, int indexcol,
							IndexOptInfo *index);
static List *expand_indexqual_opclause(RestrictInfo *rinfo,
						  Oid opfamily, Oid idxcollation);
static RestrictInfo *expand_indexqual_rowcompare(RestrictInfo *rinfo,
							IndexOptInfo *index,
							int indexcol);
static List *prefix_quals(Node *leftop, Oid opfamily, Oid collation,
			 Const *prefix, Pattern_Prefix_Status pstatus);
static List *network_prefix_quals(Node *leftop, Oid expr_op, Oid opfamily,
					 Datum rightop);
static Datum string_to_datum(const char *str, Oid datatype);
static Const *string_to_const(const char *str, Oid datatype);


/*
 * create_index_paths()
 *	  Generate all interesting index paths for the given relation.
 *	  Candidate paths are added to the rel's pathlist (using add_path).
 *
 * To be considered for an index scan, an index must match one or more
 * restriction clauses or join clauses from the query's qual condition,
 * or match the query's ORDER BY condition, or have a predicate that
 * matches the query's qual condition.
 *
 * There are two basic kinds of index scans.  A "plain" index scan uses
 * only restriction clauses (possibly none at all) in its indexqual,
 * so it can be applied in any context.  An "innerjoin" index scan uses
 * join clauses (plus restriction clauses, if available) in its indexqual.
 * Therefore it can only be used as the inner relation of a nestloop
 * join against an outer rel that includes all the other rels mentioned
 * in its join clauses.  In that context, values for the other rels'
 * attributes are available and fixed during any one scan of the indexpath.
 *
 * An IndexPath is generated and submitted to add_path() for each plain index
 * scan this routine deems potentially interesting for the current query.
 *
 * We also determine the set of other relids that participate in join
 * clauses that could be used with each index.	The actually best innerjoin
 * path will be generated for each outer relation later on, but knowing the
 * set of potential otherrels allows us to identify equivalent outer relations
 * and avoid repeated computation.
 *
 * 'rel' is the relation for which we want to generate index paths
 *
 * Note: check_partial_indexes() must have been run previously for this rel.
 */
void
create_index_paths(PlannerInfo *root, RelOptInfo *rel)
{
	List	   *indexpaths;
	List	   *bitindexpaths;
	ListCell   *l;

	/* Skip the whole mess if no indexes */
	if (rel->indexlist == NIL)
	{
		rel->index_outer_relids = NULL;
		return;
	}

	/*
	 * Examine join clauses to see which ones are potentially usable with
	 * indexes of this rel, and generate the set of all other relids that
	 * participate in such join clauses.  We'll use this set later to
	 * recognize outer rels that are equivalent for joining purposes.
	 */
	rel->index_outer_relids = indexable_outerrelids(root, rel);

	/*
	 * Find all the index paths that are directly usable for this relation
	 * (ie, are valid without considering OR or JOIN clauses).
	 */
	indexpaths = find_usable_indexes(root, rel,
									 rel->baserestrictinfo, NIL,
									 true, NULL, SAOP_FORBID, ST_ANYSCAN);

	/*
	 * Submit all the ones that can form plain IndexScan plans to add_path. (A
	 * plain IndexPath always represents a plain IndexScan plan; however some
	 * of the indexes might support only bitmap scans, and those we mustn't
	 * submit to add_path here.)  Also, pick out the ones that might be useful
	 * as bitmap scans.  For that, we must discard indexes that don't support
	 * bitmap scans, and we also are only interested in paths that have some
	 * selectivity; we should discard anything that was generated solely for
	 * ordering purposes.
	 */
	bitindexpaths = NIL;
	foreach(l, indexpaths)
	{
		IndexPath  *ipath = (IndexPath *) lfirst(l);

		if (ipath->indexinfo->amhasgettuple)
			add_path(rel, (Path *) ipath);

		if (ipath->indexinfo->amhasgetbitmap &&
			(ipath->path.pathkeys == NIL ||
			 ipath->indexselectivity < 1.0))
			bitindexpaths = lappend(bitindexpaths, ipath);
	}

	/*
	 * Generate BitmapOrPaths for any suitable OR-clauses present in the
	 * restriction list.  Add these to bitindexpaths.
	 */
	indexpaths = generate_bitmap_or_paths(root, rel,
										  rel->baserestrictinfo, NIL,
										  NULL);
	bitindexpaths = list_concat(bitindexpaths, indexpaths);

	/*
	 * Likewise, generate paths using ScalarArrayOpExpr clauses; these can't
	 * be simple indexscans but they can be used in bitmap scans.
	 */
	indexpaths = find_saop_paths(root, rel,
								 rel->baserestrictinfo, NIL,
								 true, NULL);
	bitindexpaths = list_concat(bitindexpaths, indexpaths);

	/*
	 * If we found anything usable, generate a BitmapHeapPath for the most
	 * promising combination of bitmap index paths.
	 */
	if (bitindexpaths != NIL)
	{
		Path	   *bitmapqual;
		BitmapHeapPath *bpath;

		bitmapqual = choose_bitmap_and(root, rel, bitindexpaths, NULL);
		bpath = create_bitmap_heap_path(root, rel, bitmapqual, NULL);
		add_path(rel, (Path *) bpath);
	}
}


/*----------
 * find_usable_indexes
 *	  Given a list of restriction clauses, find all the potentially usable
 *	  indexes for the given relation, and return a list of IndexPaths.
 *
 * The caller actually supplies two lists of restriction clauses: some
 * "current" ones and some "outer" ones.  Both lists can be used freely
 * to match keys of the index, but an index must use at least one of the
 * "current" clauses to be considered usable.  The motivation for this is
 * examples like
 *		WHERE (x = 42) AND (... OR (y = 52 AND z = 77) OR ....)
 * While we are considering the y/z subclause of the OR, we can use "x = 42"
 * as one of the available index conditions; but we shouldn't match the
 * subclause to any index on x alone, because such a Path would already have
 * been generated at the upper level.  So we could use an index on x,y,z
 * or an index on x,y for the OR subclause, but not an index on just x.
 * When dealing with a partial index, a match of the index predicate to
 * one of the "current" clauses also makes the index usable.
 *
 * If istoplevel is true (indicating we are considering the top level of a
 * rel's restriction clauses), we will include indexes in the result that
 * have an interesting sort order, even if they have no matching restriction
 * clauses.
 *
 * 'rel' is the relation for which we want to generate index paths
 * 'clauses' is the current list of clauses (RestrictInfo nodes)
 * 'outer_clauses' is the list of additional upper-level clauses
 * 'istoplevel' is true if clauses are the rel's top-level restriction list
 *		(outer_clauses must be NIL when this is true)
 * 'outer_rel' is the outer side of the join if forming an inner indexscan
 *		(so some of the given clauses are join clauses); NULL if not
 * 'saop_control' indicates whether ScalarArrayOpExpr clauses can be used
 * 'scantype' indicates whether we need plain or bitmap scan support
 *
 * Note: check_partial_indexes() must have been run previously.
 *----------
 */
static List *
find_usable_indexes(PlannerInfo *root, RelOptInfo *rel,
					List *clauses, List *outer_clauses,
					bool istoplevel, RelOptInfo *outer_rel,
					SaOpControl saop_control, ScanTypeControl scantype)
{
	Relids		outer_relids = outer_rel ? outer_rel->relids : NULL;
	bool		possibly_useful_pathkeys = has_useful_pathkeys(root, rel);
	List	   *result = NIL;
	List	   *all_clauses = NIL;		/* not computed till needed */
	ListCell   *ilist;

	foreach(ilist, rel->indexlist)
	{
		IndexOptInfo *index = (IndexOptInfo *) lfirst(ilist);
		IndexPath  *ipath;
		List	   *restrictclauses;
		List	   *orderbyclauses;
		List	   *index_pathkeys;
		List	   *useful_pathkeys;
		bool		useful_predicate;
		bool		found_clause;
		bool		index_is_ordered;

		/*
		 * Check that index supports the desired scan type(s)
		 */
		switch (scantype)
		{
			case ST_INDEXSCAN:
				if (!index->amhasgettuple)
					continue;
				break;
			case ST_BITMAPSCAN:
				if (!index->amhasgetbitmap)
					continue;
				break;
			case ST_ANYSCAN:
				/* either or both are OK */
				break;
		}

		/*
		 * Ignore partial indexes that do not match the query.	If a partial
		 * index is marked predOK then we know it's OK; otherwise, if we are
		 * at top level we know it's not OK (since predOK is exactly whether
		 * its predicate could be proven from the toplevel clauses).
		 * Otherwise, we have to test whether the added clauses are sufficient
		 * to imply the predicate.	If so, we could use the index in the
		 * current context.
		 *
		 * We set useful_predicate to true iff the predicate was proven using
		 * the current set of clauses.	This is needed to prevent matching a
		 * predOK index to an arm of an OR, which would be a legal but
		 * pointlessly inefficient plan.  (A better plan will be generated by
		 * just scanning the predOK index alone, no OR.)
		 */
		useful_predicate = false;
		if (index->indpred != NIL)
		{
			if (index->predOK)
			{
				if (istoplevel)
				{
					/* we know predicate was proven from these clauses */
					useful_predicate = true;
				}
			}
			else
			{
				if (istoplevel)
					continue;	/* no point in trying to prove it */

				/* Form all_clauses if not done already */
				if (all_clauses == NIL)
					all_clauses = list_concat(list_copy(clauses),
											  outer_clauses);

				if (!predicate_implied_by(index->indpred, all_clauses))
					continue;	/* can't use it at all */

				if (!predicate_implied_by(index->indpred, outer_clauses))
					useful_predicate = true;
			}
		}

		/*
		 * 1. Match the index against the available restriction clauses.
		 * found_clause is set true only if at least one of the current
		 * clauses was used (and, if saop_control is SAOP_REQUIRE, it has to
		 * have been a ScalarArrayOpExpr clause).
		 */
		restrictclauses = group_clauses_by_indexkey(index,
													clauses,
													outer_clauses,
													outer_relids,
													saop_control,
													&found_clause);

		/*
		 * Not all index AMs support scans with no restriction clauses. We
		 * can't generate a scan over an index with amoptionalkey = false
		 * unless there's at least one restriction clause.
		 */
		if (restrictclauses == NIL && !index->amoptionalkey)
			continue;

		/*
		 * 2. Compute pathkeys describing index's ordering, if any, then see
		 * how many of them are actually useful for this query.  This is not
		 * relevant unless we are at top level.
		 */
		index_is_ordered = (index->sortopfamily != NULL);
		if (index_is_ordered && possibly_useful_pathkeys &&
			istoplevel && outer_rel == NULL)
		{
			index_pathkeys = build_index_pathkeys(root, index,
												  ForwardScanDirection);
			useful_pathkeys = truncate_useless_pathkeys(root, rel,
														index_pathkeys);
			orderbyclauses = NIL;
		}
		else if (index->amcanorderbyop && possibly_useful_pathkeys &&
				 istoplevel && outer_rel == NULL && scantype != ST_BITMAPSCAN)
		{
			/* see if we can generate ordering operators for query_pathkeys */
			orderbyclauses = match_index_to_pathkeys(index,
													 root->query_pathkeys);
			if (orderbyclauses)
				useful_pathkeys = root->query_pathkeys;
			else
				useful_pathkeys = NIL;
		}
		else
		{
			useful_pathkeys = NIL;
			orderbyclauses = NIL;
		}

		/*
		 * 3. Generate an indexscan path if there are relevant restriction
		 * clauses in the current clauses, OR the index ordering is
		 * potentially useful for later merging or final output ordering, OR
		 * the index has a predicate that was proven by the current clauses.
		 */
		if (found_clause || useful_pathkeys != NIL || useful_predicate)
		{
			ipath = create_index_path(root, index,
									  restrictclauses,
									  orderbyclauses,
									  useful_pathkeys,
									  index_is_ordered ?
									  ForwardScanDirection :
									  NoMovementScanDirection,
									  outer_rel);
			result = lappend(result, ipath);
		}

		/*
		 * 4. If the index is ordered, a backwards scan might be interesting.
		 * Again, this is only interesting at top level.
		 */
		if (index_is_ordered && possibly_useful_pathkeys &&
			istoplevel && outer_rel == NULL)
		{
			index_pathkeys = build_index_pathkeys(root, index,
												  BackwardScanDirection);
			useful_pathkeys = truncate_useless_pathkeys(root, rel,
														index_pathkeys);
			if (useful_pathkeys != NIL)
			{
				ipath = create_index_path(root, index,
										  restrictclauses,
										  NIL,
										  useful_pathkeys,
										  BackwardScanDirection,
										  outer_rel);
				result = lappend(result, ipath);
			}
		}
	}

	return result;
}


/*
 * find_saop_paths
 *		Find all the potential indexpaths that make use of ScalarArrayOpExpr
 *		clauses.  The executor only supports these in bitmap scans, not
 *		plain indexscans, so we need to segregate them from the normal case.
 *		Otherwise, same API as find_usable_indexes().
 *		Returns a list of IndexPaths.
 */
static List *
find_saop_paths(PlannerInfo *root, RelOptInfo *rel,
				List *clauses, List *outer_clauses,
				bool istoplevel, RelOptInfo *outer_rel)
{
	bool		have_saop = false;
	ListCell   *l;

	/*
	 * Since find_usable_indexes is relatively expensive, don't bother to run
	 * it unless there are some top-level ScalarArrayOpExpr clauses.
	 */
	foreach(l, clauses)
	{
		RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);

		Assert(IsA(rinfo, RestrictInfo));
		if (IsA(rinfo->clause, ScalarArrayOpExpr))
		{
			have_saop = true;
			break;
		}
	}
	if (!have_saop)
		return NIL;

	return find_usable_indexes(root, rel,
							   clauses, outer_clauses,
							   istoplevel, outer_rel,
							   SAOP_REQUIRE, ST_BITMAPSCAN);
}


/*
 * generate_bitmap_or_paths
 *		Look through the list of clauses to find OR clauses, and generate
 *		a BitmapOrPath for each one we can handle that way.  Return a list
 *		of the generated BitmapOrPaths.
 *
 * outer_clauses is a list of additional clauses that can be assumed true
 * for the purpose of generating indexquals, but are not to be searched for
 * ORs.  (See find_usable_indexes() for motivation.)  outer_rel is the outer
 * side when we are considering a nestloop inner indexpath.
 */
List *
generate_bitmap_or_paths(PlannerInfo *root, RelOptInfo *rel,
						 List *clauses, List *outer_clauses,
						 RelOptInfo *outer_rel)
{
	List	   *result = NIL;
	List	   *all_clauses;
	ListCell   *l;

	/*
	 * We can use both the current and outer clauses as context for
	 * find_usable_indexes
	 */
	all_clauses = list_concat(list_copy(clauses), outer_clauses);

	foreach(l, clauses)
	{
		RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);
		List	   *pathlist;
		Path	   *bitmapqual;
		ListCell   *j;

		Assert(IsA(rinfo, RestrictInfo));
		/* Ignore RestrictInfos that aren't ORs */
		if (!restriction_is_or_clause(rinfo))
			continue;

		/*
		 * We must be able to match at least one index to each of the arms of
		 * the OR, else we can't use it.
		 */
		pathlist = NIL;
		foreach(j, ((BoolExpr *) rinfo->orclause)->args)
		{
			Node	   *orarg = (Node *) lfirst(j);
			List	   *indlist;

			/* OR arguments should be ANDs or sub-RestrictInfos */
			if (and_clause(orarg))
			{
				List	   *andargs = ((BoolExpr *) orarg)->args;

				indlist = find_usable_indexes(root, rel,
											  andargs,
											  all_clauses,
											  false,
											  outer_rel,
											  SAOP_ALLOW,
											  ST_BITMAPSCAN);
				/* Recurse in case there are sub-ORs */
				indlist = list_concat(indlist,
									  generate_bitmap_or_paths(root, rel,
															   andargs,
															   all_clauses,
															   outer_rel));
			}
			else
			{
				Assert(IsA(orarg, RestrictInfo));
				Assert(!restriction_is_or_clause((RestrictInfo *) orarg));
				indlist = find_usable_indexes(root, rel,
											  list_make1(orarg),
											  all_clauses,
											  false,
											  outer_rel,
											  SAOP_ALLOW,
											  ST_BITMAPSCAN);
			}

			/*
			 * If nothing matched this arm, we can't do anything with this OR
			 * clause.
			 */
			if (indlist == NIL)
			{
				pathlist = NIL;
				break;
			}

			/*
			 * OK, pick the most promising AND combination, and add it to
			 * pathlist.
			 */
			bitmapqual = choose_bitmap_and(root, rel, indlist, outer_rel);
			pathlist = lappend(pathlist, bitmapqual);
		}

		/*
		 * If we have a match for every arm, then turn them into a
		 * BitmapOrPath, and add to result list.
		 */
		if (pathlist != NIL)
		{
			bitmapqual = (Path *) create_bitmap_or_path(root, rel, pathlist);
			result = lappend(result, bitmapqual);
		}
	}

	return result;
}


/*
 * choose_bitmap_and
 *		Given a nonempty list of bitmap paths, AND them into one path.
 *
 * This is a nontrivial decision since we can legally use any subset of the
 * given path set.	We want to choose a good tradeoff between selectivity
 * and cost of computing the bitmap.
 *
 * The result is either a single one of the inputs, or a BitmapAndPath
 * combining multiple inputs.
 */
static Path *
choose_bitmap_and(PlannerInfo *root, RelOptInfo *rel,
				  List *paths, RelOptInfo *outer_rel)
{
	int			npaths = list_length(paths);
	PathClauseUsage **pathinfoarray;
	PathClauseUsage *pathinfo;
	List	   *clauselist;
	List	   *bestpaths = NIL;
	Cost		bestcost = 0;
	int			i,
				j;
	ListCell   *l;

	Assert(npaths > 0);			/* else caller error */
	if (npaths == 1)
		return (Path *) linitial(paths);		/* easy case */

	/*
	 * In theory we should consider every nonempty subset of the given paths.
	 * In practice that seems like overkill, given the crude nature of the
	 * estimates, not to mention the possible effects of higher-level AND and
	 * OR clauses.	Moreover, it's completely impractical if there are a large
	 * number of paths, since the work would grow as O(2^N).
	 *
	 * As a heuristic, we first check for paths using exactly the same sets of
	 * WHERE clauses + index predicate conditions, and reject all but the
	 * cheapest-to-scan in any such group.	This primarily gets rid of indexes
	 * that include the interesting columns but also irrelevant columns.  (In
	 * situations where the DBA has gone overboard on creating variant
	 * indexes, this can make for a very large reduction in the number of
	 * paths considered further.)
	 *
	 * We then sort the surviving paths with the cheapest-to-scan first, and
	 * for each path, consider using that path alone as the basis for a bitmap
	 * scan.  Then we consider bitmap AND scans formed from that path plus
	 * each subsequent (higher-cost) path, adding on a subsequent path if it
	 * results in a reduction in the estimated total scan cost. This means we
	 * consider about O(N^2) rather than O(2^N) path combinations, which is
	 * quite tolerable, especially given than N is usually reasonably small
	 * because of the prefiltering step.  The cheapest of these is returned.
	 *
	 * We will only consider AND combinations in which no two indexes use the
	 * same WHERE clause.  This is a bit of a kluge: it's needed because
	 * costsize.c and clausesel.c aren't very smart about redundant clauses.
	 * They will usually double-count the redundant clauses, producing a
	 * too-small selectivity that makes a redundant AND step look like it
	 * reduces the total cost.	Perhaps someday that code will be smarter and
	 * we can remove this limitation.  (But note that this also defends
	 * against flat-out duplicate input paths, which can happen because
	 * best_inner_indexscan will find the same OR join clauses that
	 * create_or_index_quals has pulled OR restriction clauses out of.)
	 *
	 * For the same reason, we reject AND combinations in which an index
	 * predicate clause duplicates another clause.	Here we find it necessary
	 * to be even stricter: we'll reject a partial index if any of its
	 * predicate clauses are implied by the set of WHERE clauses and predicate
	 * clauses used so far.  This covers cases such as a condition "x = 42"
	 * used with a plain index, followed by a clauseless scan of a partial
	 * index "WHERE x >= 40 AND x < 50".  The partial index has been accepted
	 * only because "x = 42" was present, and so allowing it would partially
	 * double-count selectivity.  (We could use predicate_implied_by on
	 * regular qual clauses too, to have a more intelligent, but much more
	 * expensive, check for redundancy --- but in most cases simple equality
	 * seems to suffice.)
	 */

	/*
	 * Extract clause usage info and detect any paths that use exactly the
	 * same set of clauses; keep only the cheapest-to-scan of any such groups.
	 * The surviving paths are put into an array for qsort'ing.
	 */
	pathinfoarray = (PathClauseUsage **)
		palloc(npaths * sizeof(PathClauseUsage *));
	clauselist = NIL;
	npaths = 0;
	foreach(l, paths)
	{
		Path	   *ipath = (Path *) lfirst(l);

		pathinfo = classify_index_clause_usage(ipath, &clauselist);
		for (i = 0; i < npaths; i++)
		{
			if (bms_equal(pathinfo->clauseids, pathinfoarray[i]->clauseids))
				break;
		}
		if (i < npaths)
		{
			/* duplicate clauseids, keep the cheaper one */
			Cost		ncost;
			Cost		ocost;
			Selectivity nselec;
			Selectivity oselec;

			cost_bitmap_tree_node(pathinfo->path, &ncost, &nselec);
			cost_bitmap_tree_node(pathinfoarray[i]->path, &ocost, &oselec);
			if (ncost < ocost)
				pathinfoarray[i] = pathinfo;
		}
		else
		{
			/* not duplicate clauseids, add to array */
			pathinfoarray[npaths++] = pathinfo;
		}
	}

	/* If only one surviving path, we're done */
	if (npaths == 1)
		return pathinfoarray[0]->path;

	/* Sort the surviving paths by index access cost */
	qsort(pathinfoarray, npaths, sizeof(PathClauseUsage *),
		  path_usage_comparator);

	/*
	 * For each surviving index, consider it as an "AND group leader", and see
	 * whether adding on any of the later indexes results in an AND path with
	 * cheaper total cost than before.	Then take the cheapest AND group.
	 */
	for (i = 0; i < npaths; i++)
	{
		Cost		costsofar;
		List	   *qualsofar;
		Bitmapset  *clauseidsofar;
		ListCell   *lastcell;

		pathinfo = pathinfoarray[i];
		paths = list_make1(pathinfo->path);
		costsofar = bitmap_scan_cost_est(root, rel, pathinfo->path, outer_rel);
		qualsofar = list_concat(list_copy(pathinfo->quals),
								list_copy(pathinfo->preds));
		clauseidsofar = bms_copy(pathinfo->clauseids);
		lastcell = list_head(paths);	/* for quick deletions */

		for (j = i + 1; j < npaths; j++)
		{
			Cost		newcost;

			pathinfo = pathinfoarray[j];
			/* Check for redundancy */
			if (bms_overlap(pathinfo->clauseids, clauseidsofar))
				continue;		/* consider it redundant */
			if (pathinfo->preds)
			{
				bool		redundant = false;

				/* we check each predicate clause separately */
				foreach(l, pathinfo->preds)
				{
					Node	   *np = (Node *) lfirst(l);

					if (predicate_implied_by(list_make1(np), qualsofar))
					{
						redundant = true;
						break;	/* out of inner foreach loop */
					}
				}
				if (redundant)
					continue;
			}
			/* tentatively add new path to paths, so we can estimate cost */
			paths = lappend(paths, pathinfo->path);
			newcost = bitmap_and_cost_est(root, rel, paths, outer_rel);
			if (newcost < costsofar)
			{
				/* keep new path in paths, update subsidiary variables */
				costsofar = newcost;
				qualsofar = list_concat(qualsofar,
										list_copy(pathinfo->quals));
				qualsofar = list_concat(qualsofar,
										list_copy(pathinfo->preds));
				clauseidsofar = bms_add_members(clauseidsofar,
												pathinfo->clauseids);
				lastcell = lnext(lastcell);
			}
			else
			{
				/* reject new path, remove it from paths list */
				paths = list_delete_cell(paths, lnext(lastcell), lastcell);
			}
			Assert(lnext(lastcell) == NULL);
		}

		/* Keep the cheapest AND-group (or singleton) */
		if (i == 0 || costsofar < bestcost)
		{
			bestpaths = paths;
			bestcost = costsofar;
		}

		/* some easy cleanup (we don't try real hard though) */
		list_free(qualsofar);
	}

	if (list_length(bestpaths) == 1)
		return (Path *) linitial(bestpaths);	/* no need for AND */
	return (Path *) create_bitmap_and_path(root, rel, bestpaths);
}

/* qsort comparator to sort in increasing index access cost order */
static int
path_usage_comparator(const void *a, const void *b)
{
	PathClauseUsage *pa = *(PathClauseUsage *const *) a;
	PathClauseUsage *pb = *(PathClauseUsage *const *) b;
	Cost		acost;
	Cost		bcost;
	Selectivity aselec;
	Selectivity bselec;

	cost_bitmap_tree_node(pa->path, &acost, &aselec);
	cost_bitmap_tree_node(pb->path, &bcost, &bselec);

	/*
	 * If costs are the same, sort by selectivity.
	 */
	if (acost < bcost)
		return -1;
	if (acost > bcost)
		return 1;

	if (aselec < bselec)
		return -1;
	if (aselec > bselec)
		return 1;

	return 0;
}

/*
 * Estimate the cost of actually executing a bitmap scan with a single
 * index path (no BitmapAnd, at least not at this level).
 */
static Cost
bitmap_scan_cost_est(PlannerInfo *root, RelOptInfo *rel,
					 Path *ipath, RelOptInfo *outer_rel)
{
	Path		bpath;

	cost_bitmap_heap_scan(&bpath, root, rel, ipath, outer_rel);

	return bpath.total_cost;
}

/*
 * Estimate the cost of actually executing a BitmapAnd scan with the given
 * inputs.
 */
static Cost
bitmap_and_cost_est(PlannerInfo *root, RelOptInfo *rel,
					List *paths, RelOptInfo *outer_rel)
{
	BitmapAndPath apath;
	Path		bpath;

	/* Set up a dummy BitmapAndPath */
	apath.path.type = T_BitmapAndPath;
	apath.path.parent = rel;
	apath.bitmapquals = paths;
	cost_bitmap_and_node(&apath, root);

	/* Now we can do cost_bitmap_heap_scan */
	cost_bitmap_heap_scan(&bpath, root, rel, (Path *) &apath, outer_rel);

	return bpath.total_cost;
}


/*
 * classify_index_clause_usage
 *		Construct a PathClauseUsage struct describing the WHERE clauses and
 *		index predicate clauses used by the given indexscan path.
 *		We consider two clauses the same if they are equal().
 *
 * At some point we might want to migrate this info into the Path data
 * structure proper, but for the moment it's only needed within
 * choose_bitmap_and().
 *
 * *clauselist is used and expanded as needed to identify all the distinct
 * clauses seen across successive calls.  Caller must initialize it to NIL
 * before first call of a set.
 */
static PathClauseUsage *
classify_index_clause_usage(Path *path, List **clauselist)
{
	PathClauseUsage *result;
	Bitmapset  *clauseids;
	ListCell   *lc;

	result = (PathClauseUsage *) palloc(sizeof(PathClauseUsage));
	result->path = path;

	/* Recursively find the quals and preds used by the path */
	result->quals = NIL;
	result->preds = NIL;
	find_indexpath_quals(path, &result->quals, &result->preds);

	/* Build up a bitmapset representing the quals and preds */
	clauseids = NULL;
	foreach(lc, result->quals)
	{
		Node	   *node = (Node *) lfirst(lc);

		clauseids = bms_add_member(clauseids,
								   find_list_position(node, clauselist));
	}
	foreach(lc, result->preds)
	{
		Node	   *node = (Node *) lfirst(lc);

		clauseids = bms_add_member(clauseids,
								   find_list_position(node, clauselist));
	}
	result->clauseids = clauseids;

	return result;
}


/*
 * find_indexpath_quals
 *
 * Given the Path structure for a plain or bitmap indexscan, extract lists
 * of all the indexquals and index predicate conditions used in the Path.
 * These are appended to the initial contents of *quals and *preds (hence
 * caller should initialize those to NIL).
 *
 * This is sort of a simplified version of make_restrictinfo_from_bitmapqual;
 * here, we are not trying to produce an accurate representation of the AND/OR
 * semantics of the Path, but just find out all the base conditions used.
 *
 * The result lists contain pointers to the expressions used in the Path,
 * but all the list cells are freshly built, so it's safe to destructively
 * modify the lists (eg, by concat'ing with other lists).
 */
static void
find_indexpath_quals(Path *bitmapqual, List **quals, List **preds)
{
	if (IsA(bitmapqual, BitmapAndPath))
	{
		BitmapAndPath *apath = (BitmapAndPath *) bitmapqual;
		ListCell   *l;

		foreach(l, apath->bitmapquals)
		{
			find_indexpath_quals((Path *) lfirst(l), quals, preds);
		}
	}
	else if (IsA(bitmapqual, BitmapOrPath))
	{
		BitmapOrPath *opath = (BitmapOrPath *) bitmapqual;
		ListCell   *l;

		foreach(l, opath->bitmapquals)
		{
			find_indexpath_quals((Path *) lfirst(l), quals, preds);
		}
	}
	else if (IsA(bitmapqual, IndexPath))
	{
		IndexPath  *ipath = (IndexPath *) bitmapqual;

		*quals = list_concat(*quals, get_actual_clauses(ipath->indexclauses));
		*preds = list_concat(*preds, list_copy(ipath->indexinfo->indpred));
	}
	else
		elog(ERROR, "unrecognized node type: %d", nodeTag(bitmapqual));
}


/*
 * find_list_position
 *		Return the given node's position (counting from 0) in the given
 *		list of nodes.	If it's not equal() to any existing list member,
 *		add it at the end, and return that position.
 */
static int
find_list_position(Node *node, List **nodelist)
{
	int			i;
	ListCell   *lc;

	i = 0;
	foreach(lc, *nodelist)
	{
		Node	   *oldnode = (Node *) lfirst(lc);

		if (equal(node, oldnode))
			return i;
		i++;
	}

	*nodelist = lappend(*nodelist, node);

	return i;
}


/****************************************************************************
 *				----  ROUTINES TO CHECK RESTRICTIONS  ----
 ****************************************************************************/


/*
 * group_clauses_by_indexkey
 *	  Find restriction clauses that can be used with an index.
 *
 * Returns a list of sublists of RestrictInfo nodes for clauses that can be
 * used with this index.  Each sublist contains clauses that can be used
 * with one index key (in no particular order); the top list is ordered by
 * index key.  (This is depended on by expand_indexqual_conditions().)
 *
 * We can use clauses from either the current clauses or outer_clauses lists,
 * but *found_clause is set TRUE only if we used at least one clause from
 * the "current clauses" list.	See find_usable_indexes() for motivation.
 *
 * outer_relids determines what Vars will be allowed on the other side
 * of a possible index qual; see match_clause_to_indexcol().
 *
 * 'saop_control' indicates whether ScalarArrayOpExpr clauses can be used.
 * When it's SAOP_REQUIRE, *found_clause is set TRUE only if we used at least
 * one ScalarArrayOpExpr from the current clauses list.
 *
 * If the index has amoptionalkey = false, we give up and return NIL when
 * there are no restriction clauses matching the first index key.  Otherwise,
 * we return NIL if there are no restriction clauses matching any index key.
 * A non-NIL result will have one (possibly empty) sublist for each index key.
 *
 * Example: given an index on (A,B,C), we would return ((C1 C2) () (C3 C4))
 * if we find that clauses C1 and C2 use column A, clauses C3 and C4 use
 * column C, and no clauses use column B.
 *
 * Note: in some circumstances we may find the same RestrictInfos coming
 * from multiple places.  Defend against redundant outputs by using
 * list_append_unique_ptr (pointer equality should be good enough).
 */
static List *
group_clauses_by_indexkey(IndexOptInfo *index,
						  List *clauses, List *outer_clauses,
						  Relids outer_relids,
						  SaOpControl saop_control,
						  bool *found_clause)
{
	List	   *clausegroup_list = NIL;
	bool		found_outer_clause = false;
	int			indexcol;

	*found_clause = false;		/* default result */

	if (clauses == NIL && outer_clauses == NIL)
		return NIL;				/* cannot succeed */

	for (indexcol = 0; indexcol < index->ncolumns; indexcol++)
	{
		List	   *clausegroup = NIL;
		ListCell   *l;

		/* check the current clauses */
		foreach(l, clauses)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);

			Assert(IsA(rinfo, RestrictInfo));
			if (match_clause_to_indexcol(index,
										 indexcol,
										 rinfo,
										 outer_relids,
										 saop_control))
			{
				clausegroup = list_append_unique_ptr(clausegroup, rinfo);
				if (saop_control != SAOP_REQUIRE ||
					IsA(rinfo->clause, ScalarArrayOpExpr))
					*found_clause = true;
			}
		}

		/* check the outer clauses */
		foreach(l, outer_clauses)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);

			Assert(IsA(rinfo, RestrictInfo));
			if (match_clause_to_indexcol(index,
										 indexcol,
										 rinfo,
										 outer_relids,
										 saop_control))
			{
				clausegroup = list_append_unique_ptr(clausegroup, rinfo);
				found_outer_clause = true;
			}
		}

		/*
		 * If no clauses match this key, check for amoptionalkey restriction.
		 */
		if (clausegroup == NIL && !index->amoptionalkey && indexcol == 0)
			return NIL;

		clausegroup_list = lappend(clausegroup_list, clausegroup);
	}

	if (!*found_clause && !found_outer_clause)
		return NIL;				/* no indexable clauses anywhere */

	return clausegroup_list;
}


/*
 * match_clause_to_indexcol()
 *	  Determines whether a restriction clause matches a column of an index.
 *
 *	  To match a normal index, the clause:
 *
 *	  (1)  must be in the form (indexkey op const) or (const op indexkey);
 *		   and
 *	  (2)  must contain an operator which is in the same family as the index
 *		   operator for this column, or is a "special" operator as recognized
 *		   by match_special_index_operator();
 *		   and
 *	  (3)  must match the collation of the index, if collation is relevant.
 *
 *	  Our definition of "const" is pretty liberal: we allow Vars belonging
 *	  to the caller-specified outer_relids relations (which had better not
 *	  include the relation whose index is being tested).  outer_relids should
 *	  be NULL when checking simple restriction clauses, and the outer side
 *	  of the join when building a join inner scan.	Other than that, the
 *	  only thing we don't like is volatile functions.
 *
 *	  Note: in most cases we already know that the clause as a whole uses
 *	  vars from the interesting set of relations.  The reason for the
 *	  outer_relids test is to reject clauses like (a.f1 OP (b.f2 OP a.f3));
 *	  that's not processable by an indexscan nestloop join on A, whereas
 *	  (a.f1 OP (b.f2 OP c.f3)) is.
 *
 *	  Presently, the executor can only deal with indexquals that have the
 *	  indexkey on the left, so we can only use clauses that have the indexkey
 *	  on the right if we can commute the clause to put the key on the left.
 *	  We do not actually do the commuting here, but we check whether a
 *	  suitable commutator operator is available.
 *
 *	  If the index has a collation, the clause must have the same collation.
 *	  For collation-less indexes, we assume it doesn't matter; this is
 *	  necessary for cases like "hstore ? text", wherein hstore's operators
 *	  don't care about collation but the clause will get marked with a
 *	  collation anyway because of the text argument.  (This logic is
 *	  embodied in the macro IndexCollMatchesExprColl.)
 *
 *	  It is also possible to match RowCompareExpr clauses to indexes (but
 *	  currently, only btree indexes handle this).  In this routine we will
 *	  report a match if the first column of the row comparison matches the
 *	  target index column.	This is sufficient to guarantee that some index
 *	  condition can be constructed from the RowCompareExpr --- whether the
 *	  remaining columns match the index too is considered in
 *	  expand_indexqual_rowcompare().
 *
 *	  It is also possible to match ScalarArrayOpExpr clauses to indexes, when
 *	  the clause is of the form "indexkey op ANY (arrayconst)".  Since the
 *	  executor can only handle these in the context of bitmap index scans,
 *	  our caller specifies whether to allow these or not.
 *
 *	  For boolean indexes, it is also possible to match the clause directly
 *	  to the indexkey; or perhaps the clause is (NOT indexkey).
 *
 * 'index' is the index of interest.
 * 'indexcol' is a column number of 'index' (counting from 0).
 * 'rinfo' is the clause to be tested (as a RestrictInfo node).
 * 'outer_relids' lists rels whose Vars can be considered pseudoconstant.
 * 'saop_control' indicates whether ScalarArrayOpExpr clauses can be used.
 *
 * Returns true if the clause can be used with this index key.
 *
 * NOTE:  returns false if clause is an OR or AND clause; it is the
 * responsibility of higher-level routines to cope with those.
 */
static bool
match_clause_to_indexcol(IndexOptInfo *index,
						 int indexcol,
						 RestrictInfo *rinfo,
						 Relids outer_relids,
						 SaOpControl saop_control)
{
	Expr	   *clause = rinfo->clause;
	Oid			opfamily = index->opfamily[indexcol];
	Oid			idxcollation = index->indexcollations[indexcol];
	Node	   *leftop,
			   *rightop;
	Relids		left_relids;
	Relids		right_relids;
	Oid			expr_op;
	Oid			expr_coll;
	bool		plain_op;

	/*
	 * Never match pseudoconstants to indexes.	(Normally this could not
	 * happen anyway, since a pseudoconstant clause couldn't contain a Var,
	 * but what if someone builds an expression index on a constant? It's not
	 * totally unreasonable to do so with a partial index, either.)
	 */
	if (rinfo->pseudoconstant)
		return false;

	/* First check for boolean-index cases. */
	if (IsBooleanOpfamily(opfamily))
	{
		if (match_boolean_index_clause((Node *) clause, indexcol, index))
			return true;
	}

	/*
	 * Clause must be a binary opclause, or possibly a ScalarArrayOpExpr
	 * (which is always binary, by definition).  Or it could be a
	 * RowCompareExpr, which we pass off to match_rowcompare_to_indexcol().
	 * Or, if the index supports it, we can handle IS NULL/NOT NULL clauses.
	 */
	if (is_opclause(clause))
	{
		leftop = get_leftop(clause);
		rightop = get_rightop(clause);
		if (!leftop || !rightop)
			return false;
		left_relids = rinfo->left_relids;
		right_relids = rinfo->right_relids;
		expr_op = ((OpExpr *) clause)->opno;
		expr_coll = ((OpExpr *) clause)->inputcollid;
		plain_op = true;
	}
	else if (saop_control != SAOP_FORBID &&
			 clause && IsA(clause, ScalarArrayOpExpr))
	{
		ScalarArrayOpExpr *saop = (ScalarArrayOpExpr *) clause;

		/* We only accept ANY clauses, not ALL */
		if (!saop->useOr)
			return false;
		leftop = (Node *) linitial(saop->args);
		rightop = (Node *) lsecond(saop->args);
		left_relids = NULL;		/* not actually needed */
		right_relids = pull_varnos(rightop);
		expr_op = saop->opno;
		expr_coll = saop->inputcollid;
		plain_op = false;
	}
	else if (clause && IsA(clause, RowCompareExpr))
	{
		return match_rowcompare_to_indexcol(index, indexcol,
											opfamily, idxcollation,
											(RowCompareExpr *) clause,
											outer_relids);
	}
	else if (index->amsearchnulls && IsA(clause, NullTest))
	{
		NullTest   *nt = (NullTest *) clause;

		if (!nt->argisrow &&
			match_index_to_operand((Node *) nt->arg, indexcol, index))
			return true;
		return false;
	}
	else
		return false;

	/*
	 * Check for clauses of the form: (indexkey operator constant) or
	 * (constant operator indexkey).  See above notes about const-ness.
	 */
	if (match_index_to_operand(leftop, indexcol, index) &&
		bms_is_subset(right_relids, outer_relids) &&
		!contain_volatile_functions(rightop))
	{
		if (IndexCollMatchesExprColl(idxcollation, expr_coll) &&
			is_indexable_operator(expr_op, opfamily, true))
			return true;

		/*
		 * If we didn't find a member of the index's opfamily, see whether it
		 * is a "special" indexable operator.
		 */
		if (plain_op &&
		  match_special_index_operator(clause, opfamily, idxcollation, true))
			return true;
		return false;
	}

	if (plain_op &&
		match_index_to_operand(rightop, indexcol, index) &&
		bms_is_subset(left_relids, outer_relids) &&
		!contain_volatile_functions(leftop))
	{
		if (IndexCollMatchesExprColl(idxcollation, expr_coll) &&
			is_indexable_operator(expr_op, opfamily, false))
			return true;

		/*
		 * If we didn't find a member of the index's opfamily, see whether it
		 * is a "special" indexable operator.
		 */
		if (match_special_index_operator(clause, opfamily, idxcollation, false))
			return true;
		return false;
	}

	return false;
}

/*
 * is_indexable_operator
 *	  Does the operator match the specified index opfamily?
 *
 * If the indexkey is on the right, what we actually want to know
 * is whether the operator has a commutator operator that matches
 * the opfamily.
 */
static bool
is_indexable_operator(Oid expr_op, Oid opfamily, bool indexkey_on_left)
{
	/* Get the commuted operator if necessary */
	if (!indexkey_on_left)
	{
		expr_op = get_commutator(expr_op);
		if (expr_op == InvalidOid)
			return false;
	}

	/* OK if the (commuted) operator is a member of the index's opfamily */
	return op_in_opfamily(expr_op, opfamily);
}

/*
 * match_rowcompare_to_indexcol()
 *	  Handles the RowCompareExpr case for match_clause_to_indexcol(),
 *	  which see for comments.
 */
static bool
match_rowcompare_to_indexcol(IndexOptInfo *index,
							 int indexcol,
							 Oid opfamily,
							 Oid idxcollation,
							 RowCompareExpr *clause,
							 Relids outer_relids)
{
	Node	   *leftop,
			   *rightop;
	Oid			expr_op;
	Oid			expr_coll;

	/* Forget it if we're not dealing with a btree index */
	if (index->relam != BTREE_AM_OID)
		return false;

	/*
	 * We could do the matching on the basis of insisting that the opfamily
	 * shown in the RowCompareExpr be the same as the index column's opfamily,
	 * but that could fail in the presence of reverse-sort opfamilies: it'd be
	 * a matter of chance whether RowCompareExpr had picked the forward or
	 * reverse-sort family.  So look only at the operator, and match if it is
	 * a member of the index's opfamily (after commutation, if the indexkey is
	 * on the right).  We'll worry later about whether any additional
	 * operators are matchable to the index.
	 */
	leftop = (Node *) linitial(clause->largs);
	rightop = (Node *) linitial(clause->rargs);
	expr_op = linitial_oid(clause->opnos);
	expr_coll = linitial_oid(clause->inputcollids);

	/* Collations must match, if relevant */
	if (!IndexCollMatchesExprColl(idxcollation, expr_coll))
		return false;

	/*
	 * These syntactic tests are the same as in match_clause_to_indexcol()
	 */
	if (match_index_to_operand(leftop, indexcol, index) &&
		bms_is_subset(pull_varnos(rightop), outer_relids) &&
		!contain_volatile_functions(rightop))
	{
		/* OK, indexkey is on left */
	}
	else if (match_index_to_operand(rightop, indexcol, index) &&
			 bms_is_subset(pull_varnos(leftop), outer_relids) &&
			 !contain_volatile_functions(leftop))
	{
		/* indexkey is on right, so commute the operator */
		expr_op = get_commutator(expr_op);
		if (expr_op == InvalidOid)
			return false;
	}
	else
		return false;

	/* We're good if the operator is the right type of opfamily member */
	switch (get_op_opfamily_strategy(expr_op, opfamily))
	{
		case BTLessStrategyNumber:
		case BTLessEqualStrategyNumber:
		case BTGreaterEqualStrategyNumber:
		case BTGreaterStrategyNumber:
			return true;
	}

	return false;
}


/****************************************************************************
 *				----  ROUTINES TO CHECK ORDERING OPERATORS	----
 ****************************************************************************/

/*
 * match_index_to_pathkeys
 *		Test whether an index can produce output ordered according to the
 *		given pathkeys using "ordering operators".
 *
 * If it can, return a list of suitable ORDER BY expressions, each of the form
 * "indexedcol operator pseudoconstant".  If not, return NIL.
 */
static List *
match_index_to_pathkeys(IndexOptInfo *index, List *pathkeys)
{
	List	   *orderbyexprs = NIL;
	ListCell   *lc1;

	/* Only indexes with the amcanorderbyop property are interesting here */
	if (!index->amcanorderbyop)
		return NIL;

	foreach(lc1, pathkeys)
	{
		PathKey    *pathkey = (PathKey *) lfirst(lc1);
		bool		found = false;
		ListCell   *lc2;

		/*
		 * Note: for any failure to match, we just return NIL immediately.
		 * There is no value in matching just some of the pathkeys.
		 */

		/* Pathkey must request default sort order for the target opfamily */
		if (pathkey->pk_strategy != BTLessStrategyNumber ||
			pathkey->pk_nulls_first)
			return NIL;

		/* If eclass is volatile, no hope of using an indexscan */
		if (pathkey->pk_eclass->ec_has_volatile)
			return NIL;

		/*
		 * Try to match eclass member expression(s) to index.  Note that child
		 * EC members are considered, but only when they belong to the target
		 * relation.  (Unlike regular members, the same expression could be a
		 * child member of more than one EC.  Therefore, the same index could
		 * be considered to match more than one pathkey list, which is OK
		 * here.  See also get_eclass_for_sort_expr.)
		 */
		foreach(lc2, pathkey->pk_eclass->ec_members)
		{
			EquivalenceMember *member = (EquivalenceMember *) lfirst(lc2);
			int			indexcol;

			/* No possibility of match if it references other relations */
			if (!bms_equal(member->em_relids, index->rel->relids))
				continue;

			for (indexcol = 0; indexcol < index->ncolumns; indexcol++)
			{
				Expr	   *expr;

				expr = match_clause_to_ordering_op(index,
												   indexcol,
												   member->em_expr,
												   pathkey->pk_opfamily);
				if (expr)
				{
					orderbyexprs = lappend(orderbyexprs, expr);
					found = true;
					break;
				}
			}

			if (found)			/* don't want to look at remaining members */
				break;
		}

		if (!found)				/* fail if no match for this pathkey */
			return NIL;
	}

	return orderbyexprs;		/* success! */
}

/*
 * match_clause_to_ordering_op
 *	  Determines whether an ordering operator expression matches an
 *	  index column.
 *
 *	  This is similar to, but simpler than, match_clause_to_indexcol.
 *	  We only care about simple OpExpr cases.  The input is a bare
 *	  expression that is being ordered by, which must be of the form
 *	  (indexkey op const) or (const op indexkey) where op is an ordering
 *	  operator for the column's opfamily.
 *
 * 'index' is the index of interest.
 * 'indexcol' is a column number of 'index' (counting from 0).
 * 'clause' is the ordering expression to be tested.
 * 'pk_opfamily' is the btree opfamily describing the required sort order.
 *
 * Note that we currently do not consider the collation of the ordering
 * operator's result.  In practical cases the result type will be numeric
 * and thus have no collation, and it's not very clear what to match to
 * if it did have a collation.	The index's collation should match the
 * ordering operator's input collation, not its result.
 *
 * If successful, return 'clause' as-is if the indexkey is on the left,
 * otherwise a commuted copy of 'clause'.  If no match, return NULL.
 */
static Expr *
match_clause_to_ordering_op(IndexOptInfo *index,
							int indexcol,
							Expr *clause,
							Oid pk_opfamily)
{
	Oid			opfamily = index->opfamily[indexcol];
	Oid			idxcollation = index->indexcollations[indexcol];
	Node	   *leftop,
			   *rightop;
	Oid			expr_op;
	Oid			expr_coll;
	Oid			sortfamily;
	bool		commuted;

	/*
	 * Clause must be a binary opclause.
	 */
	if (!is_opclause(clause))
		return NULL;
	leftop = get_leftop(clause);
	rightop = get_rightop(clause);
	if (!leftop || !rightop)
		return NULL;
	expr_op = ((OpExpr *) clause)->opno;
	expr_coll = ((OpExpr *) clause)->inputcollid;

	/*
	 * We can forget the whole thing right away if wrong collation.
	 */
	if (!IndexCollMatchesExprColl(idxcollation, expr_coll))
		return NULL;

	/*
	 * Check for clauses of the form: (indexkey operator constant) or
	 * (constant operator indexkey).
	 */
	if (match_index_to_operand(leftop, indexcol, index) &&
		!contain_var_clause(rightop) &&
		!contain_volatile_functions(rightop))
	{
		commuted = false;
	}
	else if (match_index_to_operand(rightop, indexcol, index) &&
			 !contain_var_clause(leftop) &&
			 !contain_volatile_functions(leftop))
	{
		/* Might match, but we need a commuted operator */
		expr_op = get_commutator(expr_op);
		if (expr_op == InvalidOid)
			return NULL;
		commuted = true;
	}
	else
		return NULL;

	/*
	 * Is the (commuted) operator an ordering operator for the opfamily? And
	 * if so, does it yield the right sorting semantics?
	 */
	sortfamily = get_op_opfamily_sortfamily(expr_op, opfamily);
	if (sortfamily != pk_opfamily)
		return NULL;

	/* We have a match.  Return clause or a commuted version thereof. */
	if (commuted)
	{
		OpExpr	   *newclause = makeNode(OpExpr);

		/* flat-copy all the fields of clause */
		memcpy(newclause, clause, sizeof(OpExpr));

		/* commute it */
		newclause->opno = expr_op;
		newclause->opfuncid = InvalidOid;
		newclause->args = list_make2(rightop, leftop);

		clause = (Expr *) newclause;
	}

	return clause;
}


/****************************************************************************
 *				----  ROUTINES TO DO PARTIAL INDEX PREDICATE TESTS	----
 ****************************************************************************/

/*
 * check_partial_indexes
 *		Check each partial index of the relation, and mark it predOK if
 *		the index's predicate is satisfied for this query.
 *
 * Note: it is possible for this to get re-run after adding more restrictions
 * to the rel; so we might be able to prove more indexes OK.  We assume that
 * adding more restrictions can't make an index not OK.
 */
void
check_partial_indexes(PlannerInfo *root, RelOptInfo *rel)
{
	List	   *restrictinfo_list = rel->baserestrictinfo;
	ListCell   *ilist;

	foreach(ilist, rel->indexlist)
	{
		IndexOptInfo *index = (IndexOptInfo *) lfirst(ilist);

		if (index->indpred == NIL)
			continue;			/* ignore non-partial indexes */

		if (index->predOK)
			continue;			/* don't repeat work if already proven OK */

		index->predOK = predicate_implied_by(index->indpred,
											 restrictinfo_list);
	}
}

/****************************************************************************
 *				----  ROUTINES TO CHECK JOIN CLAUSES  ----
 ****************************************************************************/

/*
 * indexable_outerrelids
 *	  Finds all other relids that participate in any indexable join clause
 *	  for the specified table.	Returns a set of relids.
 */
static Relids
indexable_outerrelids(PlannerInfo *root, RelOptInfo *rel)
{
	Relids		outer_relids = NULL;
	bool		is_child_rel = (rel->reloptkind == RELOPT_OTHER_MEMBER_REL);
	ListCell   *lc1;

	/*
	 * Examine each joinclause in the joininfo list to see if it matches any
	 * key of any index.  If so, add the clause's other rels to the result.
	 */
	foreach(lc1, rel->joininfo)
	{
		RestrictInfo *joininfo = (RestrictInfo *) lfirst(lc1);
		Relids		other_rels;

		other_rels = bms_difference(joininfo->required_relids, rel->relids);
		if (matches_any_index(joininfo, rel, other_rels))
			outer_relids = bms_join(outer_relids, other_rels);
		else
			bms_free(other_rels);
	}

	/*
	 * We also have to look through the query's EquivalenceClasses to see if
	 * any of them could generate indexable join conditions for this rel.
	 */
	if (rel->has_eclass_joins)
	{
		foreach(lc1, root->eq_classes)
		{
			EquivalenceClass *cur_ec = (EquivalenceClass *) lfirst(lc1);
			Relids		other_rels = NULL;
			bool		found_index = false;
			ListCell   *lc2;

			/*
			 * Won't generate joinclauses if const or single-member (the
			 * latter test covers the volatile case too)
			 */
			if (cur_ec->ec_has_const || list_length(cur_ec->ec_members) <= 1)
				continue;

			/*
			 * Note we don't test ec_broken; if we did, we'd need a separate
			 * code path to look through ec_sources.  Checking the members
			 * anyway is OK as a possibly-overoptimistic heuristic.
			 */

			/*
			 * No point in searching if rel not mentioned in eclass (but we
			 * can't tell that for a child rel).
			 */
			if (!is_child_rel &&
				!bms_is_subset(rel->relids, cur_ec->ec_relids))
				continue;

			/*
			 * Scan members, looking for both an index match and join
			 * candidates
			 */
			foreach(lc2, cur_ec->ec_members)
			{
				EquivalenceMember *cur_em = (EquivalenceMember *) lfirst(lc2);

				/* Join candidate? */
				if (!cur_em->em_is_child &&
					!bms_overlap(cur_em->em_relids, rel->relids))
				{
					other_rels = bms_add_members(other_rels,
												 cur_em->em_relids);
					continue;
				}

				/* Check for index match (only need one) */
				if (!found_index &&
					bms_equal(cur_em->em_relids, rel->relids) &&
					eclass_matches_any_index(cur_ec, cur_em, rel))
					found_index = true;
			}

			if (found_index)
				outer_relids = bms_join(outer_relids, other_rels);
			else
				bms_free(other_rels);
		}
	}

	return outer_relids;
}

/*
 * matches_any_index
 *	  Workhorse for indexable_outerrelids: see if a joinclause can be
 *	  matched to any index of the given rel.
 */
static bool
matches_any_index(RestrictInfo *rinfo, RelOptInfo *rel, Relids outer_relids)
{
	ListCell   *l;

	Assert(IsA(rinfo, RestrictInfo));

	if (restriction_is_or_clause(rinfo))
	{
		foreach(l, ((BoolExpr *) rinfo->orclause)->args)
		{
			Node	   *orarg = (Node *) lfirst(l);

			/* OR arguments should be ANDs or sub-RestrictInfos */
			if (and_clause(orarg))
			{
				ListCell   *j;

				/* Recurse to examine AND items and sub-ORs */
				foreach(j, ((BoolExpr *) orarg)->args)
				{
					RestrictInfo *arinfo = (RestrictInfo *) lfirst(j);

					if (matches_any_index(arinfo, rel, outer_relids))
						return true;
				}
			}
			else
			{
				/* Recurse to examine simple clause */
				Assert(IsA(orarg, RestrictInfo));
				Assert(!restriction_is_or_clause((RestrictInfo *) orarg));
				if (matches_any_index((RestrictInfo *) orarg, rel,
									  outer_relids))
					return true;
			}
		}

		return false;
	}

	/* Normal case for a simple restriction clause */
	foreach(l, rel->indexlist)
	{
		IndexOptInfo *index = (IndexOptInfo *) lfirst(l);
		int			indexcol;

		for (indexcol = 0; indexcol < index->ncolumns; indexcol++)
		{
			if (match_clause_to_indexcol(index,
										 indexcol,
										 rinfo,
										 outer_relids,
										 SAOP_ALLOW))
				return true;
		}
	}

	return false;
}

/*
 * eclass_matches_any_index
 *	  Workhorse for indexable_outerrelids: see if an EquivalenceClass member
 *	  can be matched to any index column of the given rel.
 *
 * This is also exported for use by find_eclass_clauses_for_index_join.
 */
bool
eclass_matches_any_index(EquivalenceClass *ec, EquivalenceMember *em,
						 RelOptInfo *rel)
{
	ListCell   *l;

	foreach(l, rel->indexlist)
	{
		IndexOptInfo *index = (IndexOptInfo *) lfirst(l);
		int			indexcol;

		for (indexcol = 0; indexcol < index->ncolumns; indexcol++)
		{
			Oid			curFamily = index->opfamily[indexcol];
			Oid			curCollation = index->indexcollations[indexcol];

			/*
			 * If it's a btree index, we can reject it if its opfamily isn't
			 * compatible with the EC, since no clause generated from the EC
			 * could be used with the index.  For non-btree indexes, we can't
			 * easily tell whether clauses generated from the EC could be used
			 * with the index, so only check for expression match.	This might
			 * mean we return "true" for a useless index, but that will just
			 * cause some wasted planner cycles; it's better than ignoring
			 * useful indexes.
			 *
			 * We insist on collation match for all index types, though.
			 */
			if ((index->relam != BTREE_AM_OID ||
				 list_member_oid(ec->ec_opfamilies, curFamily)) &&
				IndexCollMatchesExprColl(curCollation, ec->ec_collation) &&
				match_index_to_operand((Node *) em->em_expr, indexcol, index))
				return true;
		}
	}

	return false;
}


/*
 * best_inner_indexscan
 *	  Finds the best available inner indexscans for a nestloop join
 *	  with the given rel on the inside and the given outer_rel outside.
 *
 * *cheapest_startup gets the path with least startup cost
 * *cheapest_total gets the path with least total cost (often the same path)
 * Both are set to NULL if there are no possible inner indexscans.
 *
 * We ignore ordering considerations, since a nestloop's inner scan's order
 * is uninteresting.  Hence startup cost and total cost are the only figures
 * of merit to consider.
 *
 * Note: create_index_paths() must have been run previously for this rel,
 * else the results will always be NULL.
 */
void
best_inner_indexscan(PlannerInfo *root, RelOptInfo *rel,
					 RelOptInfo *outer_rel, JoinType jointype,
					 Path **cheapest_startup, Path **cheapest_total)
{
	Relids		outer_relids;
	bool		isouterjoin;
	List	   *clause_list;
	List	   *indexpaths;
	List	   *bitindexpaths;
	List	   *allindexpaths;
	ListCell   *l;
	InnerIndexscanInfo *info;
	MemoryContext oldcontext;

	/* Initialize results for failure returns */
	*cheapest_startup = *cheapest_total = NULL;

	/*
	 * Nestloop only supports inner, left, semi, and anti joins.
	 */
	switch (jointype)
	{
		case JOIN_INNER:
		case JOIN_SEMI:
			isouterjoin = false;
			break;
		case JOIN_LEFT:
		case JOIN_ANTI:
			isouterjoin = true;
			break;
		default:
			return;
	}

	/*
	 * If there are no indexable joinclauses for this rel, exit quickly.
	 */
	if (bms_is_empty(rel->index_outer_relids))
		return;

	/*
	 * Otherwise, we have to do path selection in the main planning context,
	 * so that any created path can be safely attached to the rel's cache of
	 * best inner paths.  (This is not currently an issue for normal planning,
	 * but it is an issue for GEQO planning.)
	 */
	oldcontext = MemoryContextSwitchTo(root->planner_cxt);

	/*
	 * Intersect the given outer relids with index_outer_relids to find the
	 * set of outer relids actually relevant for this rel. If there are none,
	 * again we can fail immediately.
	 */
	outer_relids = bms_intersect(rel->index_outer_relids, outer_rel->relids);
	if (bms_is_empty(outer_relids))
	{
		bms_free(outer_relids);
		MemoryContextSwitchTo(oldcontext);
		return;
	}

	/*
	 * Look to see if we already computed the result for this set of relevant
	 * outerrels.  (We include the isouterjoin status in the cache lookup key
	 * for safety.	In practice I suspect this is not necessary because it
	 * should always be the same for a given combination of rels.)
	 *
	 * NOTE: because we cache on outer_relids rather than outer_rel->relids,
	 * we will report the same paths and hence path cost for joins with
	 * different sets of irrelevant rels on the outside.  Now that cost_index
	 * is sensitive to outer_rel->rows, this is not really right.  However the
	 * error is probably not large.  Is it worth establishing a separate cache
	 * entry for each distinct outer_rel->relids set to get this right?
	 */
	foreach(l, rel->index_inner_paths)
	{
		info = (InnerIndexscanInfo *) lfirst(l);
		if (bms_equal(info->other_relids, outer_relids) &&
			info->isouterjoin == isouterjoin)
		{
			bms_free(outer_relids);
			MemoryContextSwitchTo(oldcontext);
			*cheapest_startup = info->cheapest_startup_innerpath;
			*cheapest_total = info->cheapest_total_innerpath;
			return;
		}
	}

	/*
	 * Find all the relevant restriction and join clauses.
	 *
	 * Note: because we include restriction clauses, we will find indexscans
	 * that could be plain indexscans, ie, they don't require the join context
	 * at all.	This may seem redundant, but we need to include those scans in
	 * the input given to choose_bitmap_and() to be sure we find optimal AND
	 * combinations of join and non-join scans.  Also, even if the "best inner
	 * indexscan" is just a plain indexscan, it will have a different cost
	 * estimate because of cache effects.
	 */
	clause_list = find_clauses_for_join(root, rel, outer_relids, isouterjoin);

	/*
	 * Find all the index paths that are usable for this join, except for
	 * stuff involving OR and ScalarArrayOpExpr clauses.
	 */
	allindexpaths = find_usable_indexes(root, rel,
										clause_list, NIL,
										false, outer_rel,
										SAOP_FORBID,
										ST_ANYSCAN);

	/*
	 * Include the ones that are usable as plain indexscans in indexpaths, and
	 * include the ones that are usable as bitmap scans in bitindexpaths.
	 */
	indexpaths = bitindexpaths = NIL;
	foreach(l, allindexpaths)
	{
		IndexPath  *ipath = (IndexPath *) lfirst(l);

		if (ipath->indexinfo->amhasgettuple)
			indexpaths = lappend(indexpaths, ipath);

		if (ipath->indexinfo->amhasgetbitmap)
			bitindexpaths = lappend(bitindexpaths, ipath);
	}

	/*
	 * Generate BitmapOrPaths for any suitable OR-clauses present in the
	 * clause list.
	 */
	bitindexpaths = list_concat(bitindexpaths,
								generate_bitmap_or_paths(root, rel,
														 clause_list, NIL,
														 outer_rel));

	/*
	 * Likewise, generate paths using ScalarArrayOpExpr clauses; these can't
	 * be simple indexscans but they can be used in bitmap scans.
	 */
	bitindexpaths = list_concat(bitindexpaths,
								find_saop_paths(root, rel,
												clause_list, NIL,
												false, outer_rel));

	/*
	 * If we found anything usable, generate a BitmapHeapPath for the most
	 * promising combination of bitmap index paths.
	 */
	if (bitindexpaths != NIL)
	{
		Path	   *bitmapqual;
		BitmapHeapPath *bpath;

		bitmapqual = choose_bitmap_and(root, rel, bitindexpaths, outer_rel);
		bpath = create_bitmap_heap_path(root, rel, bitmapqual, outer_rel);
		indexpaths = lappend(indexpaths, bpath);
	}

	/*
	 * Now choose the cheapest members of indexpaths.
	 */
	if (indexpaths != NIL)
	{
		*cheapest_startup = *cheapest_total = (Path *) linitial(indexpaths);

		for_each_cell(l, lnext(list_head(indexpaths)))
		{
			Path	   *path = (Path *) lfirst(l);

			if (compare_path_costs(path, *cheapest_startup, STARTUP_COST) < 0)
				*cheapest_startup = path;
			if (compare_path_costs(path, *cheapest_total, TOTAL_COST) < 0)
				*cheapest_total = path;
		}
	}

	/* Cache the results --- whether positive or negative */
	info = makeNode(InnerIndexscanInfo);
	info->other_relids = outer_relids;
	info->isouterjoin = isouterjoin;
	info->cheapest_startup_innerpath = *cheapest_startup;
	info->cheapest_total_innerpath = *cheapest_total;
	rel->index_inner_paths = lcons(info, rel->index_inner_paths);

	MemoryContextSwitchTo(oldcontext);
}

/*
 * find_clauses_for_join
 *	  Generate a list of clauses that are potentially useful for
 *	  scanning rel as the inner side of a nestloop join.
 *
 * We consider both join and restriction clauses.  Any joinclause that uses
 * only otherrels in the specified outer_relids is fair game.  But there must
 * be at least one such joinclause in the final list, otherwise we return NIL
 * indicating that there isn't any potential win here.
 */
static List *
find_clauses_for_join(PlannerInfo *root, RelOptInfo *rel,
					  Relids outer_relids, bool isouterjoin)
{
	List	   *clause_list = NIL;
	Relids		join_relids;
	ListCell   *l;

	/*
	 * Look for joinclauses that are usable with given outer_relids.  Note
	 * we'll take anything that's applicable to the join whether it has
	 * anything to do with an index or not; since we're only building a list,
	 * it's not worth filtering more finely here.
	 */
	join_relids = bms_union(rel->relids, outer_relids);

	foreach(l, rel->joininfo)
	{
		RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);

		/* Can't use pushed-down join clauses in outer join */
		if (isouterjoin && rinfo->is_pushed_down)
			continue;
		if (!bms_is_subset(rinfo->required_relids, join_relids))
			continue;

		clause_list = lappend(clause_list, rinfo);
	}

	bms_free(join_relids);

	/*
	 * Also check to see if any EquivalenceClasses can produce a relevant
	 * joinclause.	Since all such clauses are effectively pushed-down, this
	 * doesn't apply to outer joins.
	 */
	if (!isouterjoin && rel->has_eclass_joins)
		clause_list = list_concat(clause_list,
								  find_eclass_clauses_for_index_join(root,
																	 rel,
															  outer_relids));

	/* If no join clause was matched then forget it, per comments above */
	if (clause_list == NIL)
		return NIL;

	/* We can also use any plain restriction clauses for the rel */
	clause_list = list_concat(list_copy(rel->baserestrictinfo), clause_list);

	return clause_list;
}

/*
 * relation_has_unique_index_for
 *	  Determine whether the relation provably has at most one row satisfying
 *	  a set of equality conditions, because the conditions constrain all
 *	  columns of some unique index.
 *
 * The conditions are provided as a list of RestrictInfo nodes, where the
 * caller has already determined that each condition is a mergejoinable
 * equality with an expression in this relation on one side, and an
 * expression not involving this relation on the other.  The transient
 * outer_is_left flag is used to identify which side we should look at:
 * left side if outer_is_left is false, right side if it is true.
 */
bool
relation_has_unique_index_for(PlannerInfo *root, RelOptInfo *rel,
							  List *restrictlist)
{
	ListCell   *ic;

	/* Short-circuit the easy case */
	if (restrictlist == NIL)
		return false;

	/* Examine each index of the relation ... */
	foreach(ic, rel->indexlist)
	{
		IndexOptInfo *ind = (IndexOptInfo *) lfirst(ic);
		int			c;

		/*
		 * If the index is not unique, or not immediately enforced, or if it's
		 * a partial index that doesn't match the query, it's useless here.
		 */
		if (!ind->unique || !ind->immediate ||
			(ind->indpred != NIL && !ind->predOK))
			continue;

		/*
		 * Try to find each index column in the list of conditions.  This is
		 * O(n^2) or worse, but we expect all the lists to be short.
		 */
		for (c = 0; c < ind->ncolumns; c++)
		{
			ListCell   *lc;

			foreach(lc, restrictlist)
			{
				RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc);
				Node	   *rexpr;

				/*
				 * The condition's equality operator must be a member of the
				 * index opfamily, else it is not asserting the right kind of
				 * equality behavior for this index.  We check this first
				 * since it's probably cheaper than match_index_to_operand().
				 */
				if (!list_member_oid(rinfo->mergeopfamilies, ind->opfamily[c]))
					continue;

				/*
				 * XXX at some point we may need to check collations here too.
				 * For the moment we assume all collations reduce to the same
				 * notion of equality.
				 */

				/* OK, see if the condition operand matches the index key */
				if (rinfo->outer_is_left)
					rexpr = get_rightop(rinfo->clause);
				else
					rexpr = get_leftop(rinfo->clause);

				if (match_index_to_operand(rexpr, c, ind))
					break;		/* found a match; column is unique */
			}

			if (lc == NULL)
				break;			/* no match; this index doesn't help us */
		}

		/* Matched all columns of this index? */
		if (c == ind->ncolumns)
			return true;
	}

	return false;
}


/****************************************************************************
 *				----  PATH CREATION UTILITIES  ----
 ****************************************************************************/

/*
 * flatten_clausegroups_list
 *	  Given a list of lists of RestrictInfos, flatten it to a list
 *	  of RestrictInfos.
 *
 * This is used to flatten out the result of group_clauses_by_indexkey()
 * to produce an indexclauses list.  The original list structure mustn't
 * be altered, but it's OK to share copies of the underlying RestrictInfos.
 */
List *
flatten_clausegroups_list(List *clausegroups)
{
	List	   *allclauses = NIL;
	ListCell   *l;

	foreach(l, clausegroups)
		allclauses = list_concat(allclauses, list_copy((List *) lfirst(l)));
	return allclauses;
}


/****************************************************************************
 *				----  ROUTINES TO CHECK OPERANDS  ----
 ****************************************************************************/

/*
 * match_index_to_operand()
 *	  Generalized test for a match between an index's key
 *	  and the operand on one side of a restriction or join clause.
 *
 * operand: the nodetree to be compared to the index
 * indexcol: the column number of the index (counting from 0)
 * index: the index of interest
 *
 * Note that we aren't interested in collations here; the caller must check
 * for a collation match, if it's dealing with an operator where that matters.
 */
bool
match_index_to_operand(Node *operand,
					   int indexcol,
					   IndexOptInfo *index)
{
	int			indkey;

	/*
	 * Ignore any RelabelType node above the operand.	This is needed to be
	 * able to apply indexscanning in binary-compatible-operator cases. Note:
	 * we can assume there is at most one RelabelType node;
	 * eval_const_expressions() will have simplified if more than one.
	 */
	if (operand && IsA(operand, RelabelType))
		operand = (Node *) ((RelabelType *) operand)->arg;

	indkey = index->indexkeys[indexcol];
	if (indkey != 0)
	{
		/*
		 * Simple index column; operand must be a matching Var.
		 */
		if (operand && IsA(operand, Var) &&
			index->rel->relid == ((Var *) operand)->varno &&
			indkey == ((Var *) operand)->varattno)
			return true;
	}
	else
	{
		/*
		 * Index expression; find the correct expression.  (This search could
		 * be avoided, at the cost of complicating all the callers of this
		 * routine; doesn't seem worth it.)
		 */
		ListCell   *indexpr_item;
		int			i;
		Node	   *indexkey;

		indexpr_item = list_head(index->indexprs);
		for (i = 0; i < indexcol; i++)
		{
			if (index->indexkeys[i] == 0)
			{
				if (indexpr_item == NULL)
					elog(ERROR, "wrong number of index expressions");
				indexpr_item = lnext(indexpr_item);
			}
		}
		if (indexpr_item == NULL)
			elog(ERROR, "wrong number of index expressions");
		indexkey = (Node *) lfirst(indexpr_item);

		/*
		 * Does it match the operand?  Again, strip any relabeling.
		 */
		if (indexkey && IsA(indexkey, RelabelType))
			indexkey = (Node *) ((RelabelType *) indexkey)->arg;

		if (equal(indexkey, operand))
			return true;
	}

	return false;
}

/****************************************************************************
 *			----  ROUTINES FOR "SPECIAL" INDEXABLE OPERATORS  ----
 ****************************************************************************/

/*----------
 * These routines handle special optimization of operators that can be
 * used with index scans even though they are not known to the executor's
 * indexscan machinery.  The key idea is that these operators allow us
 * to derive approximate indexscan qual clauses, such that any tuples
 * that pass the operator clause itself must also satisfy the simpler
 * indexscan condition(s).	Then we can use the indexscan machinery
 * to avoid scanning as much of the table as we'd otherwise have to,
 * while applying the original operator as a qpqual condition to ensure
 * we deliver only the tuples we want.	(In essence, we're using a regular
 * index as if it were a lossy index.)
 *
 * An example of what we're doing is
 *			textfield LIKE 'abc%'
 * from which we can generate the indexscanable conditions
 *			textfield >= 'abc' AND textfield < 'abd'
 * which allow efficient scanning of an index on textfield.
 * (In reality, character set and collation issues make the transformation
 * from LIKE to indexscan limits rather harder than one might think ...
 * but that's the basic idea.)
 *
 * Another thing that we do with this machinery is to provide special
 * smarts for "boolean" indexes (that is, indexes on boolean columns
 * that support boolean equality).	We can transform a plain reference
 * to the indexkey into "indexkey = true", or "NOT indexkey" into
 * "indexkey = false", so as to make the expression indexable using the
 * regular index operators.  (As of Postgres 8.1, we must do this here
 * because constant simplification does the reverse transformation;
 * without this code there'd be no way to use such an index at all.)
 *
 * Three routines are provided here:
 *
 * match_special_index_operator() is just an auxiliary function for
 * match_clause_to_indexcol(); after the latter fails to recognize a
 * restriction opclause's operator as a member of an index's opfamily,
 * it asks match_special_index_operator() whether the clause should be
 * considered an indexqual anyway.
 *
 * match_boolean_index_clause() similarly detects clauses that can be
 * converted into boolean equality operators.
 *
 * expand_indexqual_conditions() converts a list of lists of RestrictInfo
 * nodes (with implicit AND semantics across list elements) into
 * a list of clauses that the executor can actually handle.  For operators
 * that are members of the index's opfamily this transformation is a no-op,
 * but clauses recognized by match_special_index_operator() or
 * match_boolean_index_clause() must be converted into one or more "regular"
 * indexqual conditions.
 *----------
 */

/*
 * match_boolean_index_clause
 *	  Recognize restriction clauses that can be matched to a boolean index.
 *
 * This should be called only when IsBooleanOpfamily() recognizes the
 * index's operator family.  We check to see if the clause matches the
 * index's key.
 */
static bool
match_boolean_index_clause(Node *clause,
						   int indexcol,
						   IndexOptInfo *index)
{
	/* Direct match? */
	if (match_index_to_operand(clause, indexcol, index))
		return true;
	/* NOT clause? */
	if (not_clause(clause))
	{
		if (match_index_to_operand((Node *) get_notclausearg((Expr *) clause),
								   indexcol, index))
			return true;
	}

	/*
	 * Since we only consider clauses at top level of WHERE, we can convert
	 * indexkey IS TRUE and indexkey IS FALSE to index searches as well. The
	 * different meaning for NULL isn't important.
	 */
	else if (clause && IsA(clause, BooleanTest))
	{
		BooleanTest *btest = (BooleanTest *) clause;

		if (btest->booltesttype == IS_TRUE ||
			btest->booltesttype == IS_FALSE)
			if (match_index_to_operand((Node *) btest->arg,
									   indexcol, index))
				return true;
	}
	return false;
}

/*
 * match_special_index_operator
 *	  Recognize restriction clauses that can be used to generate
 *	  additional indexscanable qualifications.
 *
 * The given clause is already known to be a binary opclause having
 * the form (indexkey OP pseudoconst) or (pseudoconst OP indexkey),
 * but the OP proved not to be one of the index's opfamily operators.
 * Return 'true' if we can do something with it anyway.
 */
static bool
match_special_index_operator(Expr *clause, Oid opfamily, Oid idxcollation,
							 bool indexkey_on_left)
{
	bool		isIndexable = false;
	Node	   *rightop;
	Oid			expr_op;
	Oid			expr_coll;
	Const	   *patt;
	Const	   *prefix = NULL;
	Pattern_Prefix_Status pstatus = Pattern_Prefix_None;

	/*
	 * Currently, all known special operators require the indexkey on the
	 * left, but this test could be pushed into the switch statement if some
	 * are added that do not...
	 */
	if (!indexkey_on_left)
		return false;

	/* we know these will succeed */
	rightop = get_rightop(clause);
	expr_op = ((OpExpr *) clause)->opno;
	expr_coll = ((OpExpr *) clause)->inputcollid;

	/* again, required for all current special ops: */
	if (!IsA(rightop, Const) ||
		((Const *) rightop)->constisnull)
		return false;
	patt = (Const *) rightop;

	switch (expr_op)
	{
		case OID_TEXT_LIKE_OP:
		case OID_BPCHAR_LIKE_OP:
		case OID_NAME_LIKE_OP:
			/* the right-hand const is type text for all of these */
			pstatus = pattern_fixed_prefix(patt, Pattern_Type_Like, expr_coll,
										   &prefix, NULL);
			isIndexable = (pstatus != Pattern_Prefix_None);
			break;

		case OID_BYTEA_LIKE_OP:
			pstatus = pattern_fixed_prefix(patt, Pattern_Type_Like, expr_coll,
										   &prefix, NULL);
			isIndexable = (pstatus != Pattern_Prefix_None);
			break;

		case OID_TEXT_ICLIKE_OP:
		case OID_BPCHAR_ICLIKE_OP:
		case OID_NAME_ICLIKE_OP:
			/* the right-hand const is type text for all of these */
			pstatus = pattern_fixed_prefix(patt, Pattern_Type_Like_IC, expr_coll,
										   &prefix, NULL);
			isIndexable = (pstatus != Pattern_Prefix_None);
			break;

		case OID_TEXT_REGEXEQ_OP:
		case OID_BPCHAR_REGEXEQ_OP:
		case OID_NAME_REGEXEQ_OP:
			/* the right-hand const is type text for all of these */
			pstatus = pattern_fixed_prefix(patt, Pattern_Type_Regex, expr_coll,
										   &prefix, NULL);
			isIndexable = (pstatus != Pattern_Prefix_None);
			break;

		case OID_TEXT_ICREGEXEQ_OP:
		case OID_BPCHAR_ICREGEXEQ_OP:
		case OID_NAME_ICREGEXEQ_OP:
			/* the right-hand const is type text for all of these */
			pstatus = pattern_fixed_prefix(patt, Pattern_Type_Regex_IC, expr_coll,
										   &prefix, NULL);
			isIndexable = (pstatus != Pattern_Prefix_None);
			break;

		case OID_INET_SUB_OP:
		case OID_INET_SUBEQ_OP:
			isIndexable = true;
			break;
	}

	if (prefix)
	{
		pfree(DatumGetPointer(prefix->constvalue));
		pfree(prefix);
	}

	/* done if the expression doesn't look indexable */
	if (!isIndexable)
		return false;

	/*
	 * Must also check that index's opfamily supports the operators we will
	 * want to apply.  (A hash index, for example, will not support ">=".)
	 * Currently, only btree supports the operators we need.
	 *
	 * Note: actually, in the Pattern_Prefix_Exact case, we only need "=" so a
	 * hash index would work.  Currently it doesn't seem worth checking for
	 * that, however.
	 *
	 * We insist on the opfamily being the specific one we expect, else we'd
	 * do the wrong thing if someone were to make a reverse-sort opfamily with
	 * the same operators.
	 *
	 * The non-pattern opclasses will not sort the way we need in most non-C
	 * locales.  We can use such an index anyway for an exact match (simple
	 * equality), but not for prefix-match cases.  Note that here we are
	 * looking at the index's collation, not the expression's collation --
	 * this test is *not* dependent on the LIKE/regex operator's collation.
	 */
	switch (expr_op)
	{
		case OID_TEXT_LIKE_OP:
		case OID_TEXT_ICLIKE_OP:
		case OID_TEXT_REGEXEQ_OP:
		case OID_TEXT_ICREGEXEQ_OP:
			isIndexable =
				(opfamily == TEXT_PATTERN_BTREE_FAM_OID) ||
				(opfamily == TEXT_BTREE_FAM_OID &&
				 (pstatus == Pattern_Prefix_Exact ||
				  lc_collate_is_c(idxcollation)));
			break;

		case OID_BPCHAR_LIKE_OP:
		case OID_BPCHAR_ICLIKE_OP:
		case OID_BPCHAR_REGEXEQ_OP:
		case OID_BPCHAR_ICREGEXEQ_OP:
			isIndexable =
				(opfamily == BPCHAR_PATTERN_BTREE_FAM_OID) ||
				(opfamily == BPCHAR_BTREE_FAM_OID &&
				 (pstatus == Pattern_Prefix_Exact ||
				  lc_collate_is_c(idxcollation)));
			break;

		case OID_NAME_LIKE_OP:
		case OID_NAME_ICLIKE_OP:
		case OID_NAME_REGEXEQ_OP:
		case OID_NAME_ICREGEXEQ_OP:
			/* name uses locale-insensitive sorting */
			isIndexable = (opfamily == NAME_BTREE_FAM_OID);
			break;

		case OID_BYTEA_LIKE_OP:
			isIndexable = (opfamily == BYTEA_BTREE_FAM_OID);
			break;

		case OID_INET_SUB_OP:
		case OID_INET_SUBEQ_OP:
			isIndexable = (opfamily == NETWORK_BTREE_FAM_OID);
			break;
	}

	return isIndexable;
}

/*
 * expand_indexqual_conditions
 *	  Given a list of sublists of RestrictInfo nodes, produce a flat list
 *	  of index qual clauses.  Standard qual clauses (those in the index's
 *	  opfamily) are passed through unchanged.  Boolean clauses and "special"
 *	  index operators are expanded into clauses that the indexscan machinery
 *	  will know what to do with.  RowCompare clauses are simplified if
 *	  necessary to create a clause that is fully checkable by the index.
 *
 * The input list is ordered by index key, and so the output list is too.
 * (The latter is not depended on by any part of the core planner, I believe,
 * but parts of the executor require it, and so do the amcostestimate
 * functions.)
 */
List *
expand_indexqual_conditions(IndexOptInfo *index, List *clausegroups)
{
	List	   *resultquals = NIL;
	ListCell   *lc;
	int			indexcol;

	if (clausegroups == NIL)
		return NIL;

	/* clausegroups must correspond to index columns */
	Assert(list_length(clausegroups) <= index->ncolumns);

	indexcol = 0;
	foreach(lc, clausegroups)
	{
		List	   *clausegroup = (List *) lfirst(lc);
		Oid			curFamily = index->opfamily[indexcol];
		Oid			curCollation = index->indexcollations[indexcol];
		ListCell   *lc2;

		foreach(lc2, clausegroup)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(lc2);
			Expr	   *clause = rinfo->clause;

			/* First check for boolean cases */
			if (IsBooleanOpfamily(curFamily))
			{
				Expr	   *boolqual;

				boolqual = expand_boolean_index_clause((Node *) clause,
													   indexcol,
													   index);
				if (boolqual)
				{
					resultquals = lappend(resultquals,
										  make_simple_restrictinfo(boolqual));
					continue;
				}
			}

			/*
			 * Else it must be an opclause (usual case), ScalarArrayOp,
			 * RowCompare, or NullTest
			 */
			if (is_opclause(clause))
			{
				resultquals = list_concat(resultquals,
										  expand_indexqual_opclause(rinfo,
																	curFamily,
															  curCollation));
			}
			else if (IsA(clause, ScalarArrayOpExpr))
			{
				/* no extra work at this time */
				resultquals = lappend(resultquals, rinfo);
			}
			else if (IsA(clause, RowCompareExpr))
			{
				resultquals = lappend(resultquals,
									  expand_indexqual_rowcompare(rinfo,
																  index,
																  indexcol));
			}
			else if (IsA(clause, NullTest))
			{
				Assert(index->amsearchnulls);
				resultquals = lappend(resultquals,
									  make_simple_restrictinfo(clause));
			}
			else
				elog(ERROR, "unsupported indexqual type: %d",
					 (int) nodeTag(clause));
		}

		indexcol++;
	}

	return resultquals;
}

/*
 * expand_boolean_index_clause
 *	  Convert a clause recognized by match_boolean_index_clause into
 *	  a boolean equality operator clause.
 *
 * Returns NULL if the clause isn't a boolean index qual.
 */
static Expr *
expand_boolean_index_clause(Node *clause,
							int indexcol,
							IndexOptInfo *index)
{
	/* Direct match? */
	if (match_index_to_operand(clause, indexcol, index))
	{
		/* convert to indexkey = TRUE */
		return make_opclause(BooleanEqualOperator, BOOLOID, false,
							 (Expr *) clause,
							 (Expr *) makeBoolConst(true, false),
							 InvalidOid, InvalidOid);
	}
	/* NOT clause? */
	if (not_clause(clause))
	{
		Node	   *arg = (Node *) get_notclausearg((Expr *) clause);

		/* It must have matched the indexkey */
		Assert(match_index_to_operand(arg, indexcol, index));
		/* convert to indexkey = FALSE */
		return make_opclause(BooleanEqualOperator, BOOLOID, false,
							 (Expr *) arg,
							 (Expr *) makeBoolConst(false, false),
							 InvalidOid, InvalidOid);
	}
	if (clause && IsA(clause, BooleanTest))
	{
		BooleanTest *btest = (BooleanTest *) clause;
		Node	   *arg = (Node *) btest->arg;

		/* It must have matched the indexkey */
		Assert(match_index_to_operand(arg, indexcol, index));
		if (btest->booltesttype == IS_TRUE)
		{
			/* convert to indexkey = TRUE */
			return make_opclause(BooleanEqualOperator, BOOLOID, false,
								 (Expr *) arg,
								 (Expr *) makeBoolConst(true, false),
								 InvalidOid, InvalidOid);
		}
		if (btest->booltesttype == IS_FALSE)
		{
			/* convert to indexkey = FALSE */
			return make_opclause(BooleanEqualOperator, BOOLOID, false,
								 (Expr *) arg,
								 (Expr *) makeBoolConst(false, false),
								 InvalidOid, InvalidOid);
		}
		/* Oops */
		Assert(false);
	}

	return NULL;
}

/*
 * expand_indexqual_opclause --- expand a single indexqual condition
 *		that is an operator clause
 *
 * The input is a single RestrictInfo, the output a list of RestrictInfos.
 *
 * In the base case this is just list_make1(), but we have to be prepared to
 * expand special cases that were accepted by match_special_index_operator().
 */
static List *
expand_indexqual_opclause(RestrictInfo *rinfo, Oid opfamily, Oid idxcollation)
{
	Expr	   *clause = rinfo->clause;

	/* we know these will succeed */
	Node	   *leftop = get_leftop(clause);
	Node	   *rightop = get_rightop(clause);
	Oid			expr_op = ((OpExpr *) clause)->opno;
	Oid			expr_coll = ((OpExpr *) clause)->inputcollid;
	Const	   *patt = (Const *) rightop;
	Const	   *prefix = NULL;
	Pattern_Prefix_Status pstatus;

	/*
	 * LIKE and regex operators are not members of any btree index opfamily,
	 * but they can be members of opfamilies for more exotic index types such
	 * as GIN.	Therefore, we should only do expansion if the operator is
	 * actually not in the opfamily.  But checking that requires a syscache
	 * lookup, so it's best to first see if the operator is one we are
	 * interested in.
	 */
	switch (expr_op)
	{
		case OID_TEXT_LIKE_OP:
		case OID_BPCHAR_LIKE_OP:
		case OID_NAME_LIKE_OP:
		case OID_BYTEA_LIKE_OP:
			if (!op_in_opfamily(expr_op, opfamily))
			{
				pstatus = pattern_fixed_prefix(patt, Pattern_Type_Like, expr_coll,
											   &prefix, NULL);
				return prefix_quals(leftop, opfamily, idxcollation, prefix, pstatus);
			}
			break;

		case OID_TEXT_ICLIKE_OP:
		case OID_BPCHAR_ICLIKE_OP:
		case OID_NAME_ICLIKE_OP:
			if (!op_in_opfamily(expr_op, opfamily))
			{
				/* the right-hand const is type text for all of these */
				pstatus = pattern_fixed_prefix(patt, Pattern_Type_Like_IC, expr_coll,
											   &prefix, NULL);
				return prefix_quals(leftop, opfamily, idxcollation, prefix, pstatus);
			}
			break;

		case OID_TEXT_REGEXEQ_OP:
		case OID_BPCHAR_REGEXEQ_OP:
		case OID_NAME_REGEXEQ_OP:
			if (!op_in_opfamily(expr_op, opfamily))
			{
				/* the right-hand const is type text for all of these */
				pstatus = pattern_fixed_prefix(patt, Pattern_Type_Regex, expr_coll,
											   &prefix, NULL);
				return prefix_quals(leftop, opfamily, idxcollation, prefix, pstatus);
			}
			break;

		case OID_TEXT_ICREGEXEQ_OP:
		case OID_BPCHAR_ICREGEXEQ_OP:
		case OID_NAME_ICREGEXEQ_OP:
			if (!op_in_opfamily(expr_op, opfamily))
			{
				/* the right-hand const is type text for all of these */
				pstatus = pattern_fixed_prefix(patt, Pattern_Type_Regex_IC, expr_coll,
											   &prefix, NULL);
				return prefix_quals(leftop, opfamily, idxcollation, prefix, pstatus);
			}
			break;

		case OID_INET_SUB_OP:
		case OID_INET_SUBEQ_OP:
			if (!op_in_opfamily(expr_op, opfamily))
			{
				return network_prefix_quals(leftop, expr_op, opfamily,
											patt->constvalue);
			}
			break;
	}

	/* Default case: just make a list of the unmodified indexqual */
	return list_make1(rinfo);
}

/*
 * expand_indexqual_rowcompare --- expand a single indexqual condition
 *		that is a RowCompareExpr
 *
 * It's already known that the first column of the row comparison matches
 * the specified column of the index.  We can use additional columns of the
 * row comparison as index qualifications, so long as they match the index
 * in the "same direction", ie, the indexkeys are all on the same side of the
 * clause and the operators are all the same-type members of the opfamilies.
 * If all the columns of the RowCompareExpr match in this way, we just use it
 * as-is.  Otherwise, we build a shortened RowCompareExpr (if more than one
 * column matches) or a simple OpExpr (if the first-column match is all
 * there is).  In these cases the modified clause is always "<=" or ">="
 * even when the original was "<" or ">" --- this is necessary to match all
 * the rows that could match the original.	(We are essentially building a
 * lossy version of the row comparison when we do this.)
 */
static RestrictInfo *
expand_indexqual_rowcompare(RestrictInfo *rinfo,
							IndexOptInfo *index,
							int indexcol)
{
	RowCompareExpr *clause = (RowCompareExpr *) rinfo->clause;
	bool		var_on_left;
	int			op_strategy;
	Oid			op_lefttype;
	Oid			op_righttype;
	int			matching_cols;
	Oid			expr_op;
	List	   *opfamilies;
	List	   *lefttypes;
	List	   *righttypes;
	List	   *new_ops;
	ListCell   *largs_cell;
	ListCell   *rargs_cell;
	ListCell   *opnos_cell;
	ListCell   *collids_cell;

	/* We have to figure out (again) how the first col matches */
	var_on_left = match_index_to_operand((Node *) linitial(clause->largs),
										 indexcol, index);
	Assert(var_on_left ||
		   match_index_to_operand((Node *) linitial(clause->rargs),
								  indexcol, index));
	expr_op = linitial_oid(clause->opnos);
	if (!var_on_left)
		expr_op = get_commutator(expr_op);
	get_op_opfamily_properties(expr_op, index->opfamily[indexcol], false,
							   &op_strategy,
							   &op_lefttype,
							   &op_righttype);
	/* Build lists of the opfamilies and operator datatypes in case needed */
	opfamilies = list_make1_oid(index->opfamily[indexcol]);
	lefttypes = list_make1_oid(op_lefttype);
	righttypes = list_make1_oid(op_righttype);

	/*
	 * See how many of the remaining columns match some index column in the
	 * same way.  A note about rel membership tests: we assume that the clause
	 * as a whole is already known to use only Vars from the indexed relation
	 * and possibly some acceptable outer relations. So the "other" side of
	 * any potential index condition is OK as long as it doesn't use Vars from
	 * the indexed relation.
	 */
	matching_cols = 1;
	largs_cell = lnext(list_head(clause->largs));
	rargs_cell = lnext(list_head(clause->rargs));
	opnos_cell = lnext(list_head(clause->opnos));
	collids_cell = lnext(list_head(clause->inputcollids));

	while (largs_cell != NULL)
	{
		Node	   *varop;
		Node	   *constop;
		int			i;

		expr_op = lfirst_oid(opnos_cell);
		if (var_on_left)
		{
			varop = (Node *) lfirst(largs_cell);
			constop = (Node *) lfirst(rargs_cell);
		}
		else
		{
			varop = (Node *) lfirst(rargs_cell);
			constop = (Node *) lfirst(largs_cell);
			/* indexkey is on right, so commute the operator */
			expr_op = get_commutator(expr_op);
			if (expr_op == InvalidOid)
				break;			/* operator is not usable */
		}
		if (bms_is_member(index->rel->relid, pull_varnos(constop)))
			break;				/* no good, Var on wrong side */
		if (contain_volatile_functions(constop))
			break;				/* no good, volatile comparison value */

		/*
		 * The Var side can match any column of the index.	If the user does
		 * something weird like having multiple identical index columns, we
		 * insist the match be on the first such column, to avoid confusing
		 * the executor.
		 */
		for (i = 0; i < index->ncolumns; i++)
		{
			if (match_index_to_operand(varop, i, index))
				break;
		}
		if (i >= index->ncolumns)
			break;				/* no match found */

		/* Now, do we have the right operator for this column? */
		if (get_op_opfamily_strategy(expr_op, index->opfamily[i])
			!= op_strategy)
			break;

		/* Does collation match? */
		if (!IndexCollMatchesExprColl(index->indexcollations[i],
									  lfirst_oid(collids_cell)))
			break;

		/* Add opfamily and datatypes to lists */
		get_op_opfamily_properties(expr_op, index->opfamily[i], false,
								   &op_strategy,
								   &op_lefttype,
								   &op_righttype);
		opfamilies = lappend_oid(opfamilies, index->opfamily[i]);
		lefttypes = lappend_oid(lefttypes, op_lefttype);
		righttypes = lappend_oid(righttypes, op_righttype);

		/* This column matches, keep scanning */
		matching_cols++;
		largs_cell = lnext(largs_cell);
		rargs_cell = lnext(rargs_cell);
		opnos_cell = lnext(opnos_cell);
		collids_cell = lnext(collids_cell);
	}

	/* Return clause as-is if it's all usable as index quals */
	if (matching_cols == list_length(clause->opnos))
		return rinfo;

	/*
	 * We have to generate a subset rowcompare (possibly just one OpExpr). The
	 * painful part of this is changing < to <= or > to >=, so deal with that
	 * first.
	 */
	if (op_strategy == BTLessEqualStrategyNumber ||
		op_strategy == BTGreaterEqualStrategyNumber)
	{
		/* easy, just use the same operators */
		new_ops = list_truncate(list_copy(clause->opnos), matching_cols);
	}
	else
	{
		ListCell   *opfamilies_cell;
		ListCell   *lefttypes_cell;
		ListCell   *righttypes_cell;

		if (op_strategy == BTLessStrategyNumber)
			op_strategy = BTLessEqualStrategyNumber;
		else if (op_strategy == BTGreaterStrategyNumber)
			op_strategy = BTGreaterEqualStrategyNumber;
		else
			elog(ERROR, "unexpected strategy number %d", op_strategy);
		new_ops = NIL;
		lefttypes_cell = list_head(lefttypes);
		righttypes_cell = list_head(righttypes);
		foreach(opfamilies_cell, opfamilies)
		{
			Oid			opfam = lfirst_oid(opfamilies_cell);
			Oid			lefttype = lfirst_oid(lefttypes_cell);
			Oid			righttype = lfirst_oid(righttypes_cell);

			expr_op = get_opfamily_member(opfam, lefttype, righttype,
										  op_strategy);
			if (!OidIsValid(expr_op))	/* should not happen */
				elog(ERROR, "could not find member %d(%u,%u) of opfamily %u",
					 op_strategy, lefttype, righttype, opfam);
			if (!var_on_left)
			{
				expr_op = get_commutator(expr_op);
				if (!OidIsValid(expr_op))		/* should not happen */
					elog(ERROR, "could not find commutator of member %d(%u,%u) of opfamily %u",
						 op_strategy, lefttype, righttype, opfam);
			}
			new_ops = lappend_oid(new_ops, expr_op);
			lefttypes_cell = lnext(lefttypes_cell);
			righttypes_cell = lnext(righttypes_cell);
		}
	}

	/* If we have more than one matching col, create a subset rowcompare */
	if (matching_cols > 1)
	{
		RowCompareExpr *rc = makeNode(RowCompareExpr);

		if (var_on_left)
			rc->rctype = (RowCompareType) op_strategy;
		else
			rc->rctype = (op_strategy == BTLessEqualStrategyNumber) ?
				ROWCOMPARE_GE : ROWCOMPARE_LE;
		rc->opnos = new_ops;
		rc->opfamilies = list_truncate(list_copy(clause->opfamilies),
									   matching_cols);
		rc->inputcollids = list_truncate(list_copy(clause->inputcollids),
										 matching_cols);
		rc->largs = list_truncate((List *) copyObject(clause->largs),
								  matching_cols);
		rc->rargs = list_truncate((List *) copyObject(clause->rargs),
								  matching_cols);
		return make_simple_restrictinfo((Expr *) rc);
	}
	else
	{
		Expr	   *opexpr;

		opexpr = make_opclause(linitial_oid(new_ops), BOOLOID, false,
							   copyObject(linitial(clause->largs)),
							   copyObject(linitial(clause->rargs)),
							   InvalidOid,
							   linitial_oid(clause->inputcollids));
		return make_simple_restrictinfo(opexpr);
	}
}

/*
 * Given a fixed prefix that all the "leftop" values must have,
 * generate suitable indexqual condition(s).  opfamily is the index
 * operator family; we use it to deduce the appropriate comparison
 * operators and operand datatypes.  collation is the input collation to use.
 */
static List *
prefix_quals(Node *leftop, Oid opfamily, Oid collation,
			 Const *prefix_const, Pattern_Prefix_Status pstatus)
{
	List	   *result;
	Oid			datatype;
	Oid			oproid;
	Expr	   *expr;
	FmgrInfo	ltproc;
	Const	   *greaterstr;

	Assert(pstatus != Pattern_Prefix_None);

	switch (opfamily)
	{
		case TEXT_BTREE_FAM_OID:
		case TEXT_PATTERN_BTREE_FAM_OID:
			datatype = TEXTOID;
			break;

		case BPCHAR_BTREE_FAM_OID:
		case BPCHAR_PATTERN_BTREE_FAM_OID:
			datatype = BPCHAROID;
			break;

		case NAME_BTREE_FAM_OID:
			datatype = NAMEOID;
			break;

		case BYTEA_BTREE_FAM_OID:
			datatype = BYTEAOID;
			break;

		default:
			/* shouldn't get here */
			elog(ERROR, "unexpected opfamily: %u", opfamily);
			return NIL;
	}

	/*
	 * If necessary, coerce the prefix constant to the right type. The given
	 * prefix constant is either text or bytea type.
	 */
	if (prefix_const->consttype != datatype)
	{
		char	   *prefix;

		switch (prefix_const->consttype)
		{
			case TEXTOID:
				prefix = TextDatumGetCString(prefix_const->constvalue);
				break;
			case BYTEAOID:
				prefix = DatumGetCString(DirectFunctionCall1(byteaout,
												  prefix_const->constvalue));
				break;
			default:
				elog(ERROR, "unexpected const type: %u",
					 prefix_const->consttype);
				return NIL;
		}
		prefix_const = string_to_const(prefix, datatype);
		pfree(prefix);
	}

	/*
	 * If we found an exact-match pattern, generate an "=" indexqual.
	 */
	if (pstatus == Pattern_Prefix_Exact)
	{
		oproid = get_opfamily_member(opfamily, datatype, datatype,
									 BTEqualStrategyNumber);
		if (oproid == InvalidOid)
			elog(ERROR, "no = operator for opfamily %u", opfamily);
		expr = make_opclause(oproid, BOOLOID, false,
							 (Expr *) leftop, (Expr *) prefix_const,
							 InvalidOid, collation);
		result = list_make1(make_simple_restrictinfo(expr));
		return result;
	}

	/*
	 * Otherwise, we have a nonempty required prefix of the values.
	 *
	 * We can always say "x >= prefix".
	 */
	oproid = get_opfamily_member(opfamily, datatype, datatype,
								 BTGreaterEqualStrategyNumber);
	if (oproid == InvalidOid)
		elog(ERROR, "no >= operator for opfamily %u", opfamily);
	expr = make_opclause(oproid, BOOLOID, false,
						 (Expr *) leftop, (Expr *) prefix_const,
						 InvalidOid, collation);
	result = list_make1(make_simple_restrictinfo(expr));

	/*-------
	 * If we can create a string larger than the prefix, we can say
	 * "x < greaterstr".  NB: we rely on make_greater_string() to generate
	 * a guaranteed-greater string, not just a probably-greater string.
	 * In general this is only guaranteed in C locale, so we'd better be
	 * using a C-locale index collation.
	 *-------
	 */
	oproid = get_opfamily_member(opfamily, datatype, datatype,
								 BTLessStrategyNumber);
	if (oproid == InvalidOid)
		elog(ERROR, "no < operator for opfamily %u", opfamily);
	fmgr_info(get_opcode(oproid), &ltproc);
	greaterstr = make_greater_string(prefix_const, &ltproc, collation);
	if (greaterstr)
	{
		expr = make_opclause(oproid, BOOLOID, false,
							 (Expr *) leftop, (Expr *) greaterstr,
							 InvalidOid, collation);
		result = lappend(result, make_simple_restrictinfo(expr));
	}

	return result;
}

/*
 * Given a leftop and a rightop, and a inet-family sup/sub operator,
 * generate suitable indexqual condition(s).  expr_op is the original
 * operator, and opfamily is the index opfamily.
 */
static List *
network_prefix_quals(Node *leftop, Oid expr_op, Oid opfamily, Datum rightop)
{
	bool		is_eq;
	Oid			datatype;
	Oid			opr1oid;
	Oid			opr2oid;
	Datum		opr1right;
	Datum		opr2right;
	List	   *result;
	Expr	   *expr;

	switch (expr_op)
	{
		case OID_INET_SUB_OP:
			datatype = INETOID;
			is_eq = false;
			break;
		case OID_INET_SUBEQ_OP:
			datatype = INETOID;
			is_eq = true;
			break;
		default:
			elog(ERROR, "unexpected operator: %u", expr_op);
			return NIL;
	}

	/*
	 * create clause "key >= network_scan_first( rightop )", or ">" if the
	 * operator disallows equality.
	 */
	if (is_eq)
	{
		opr1oid = get_opfamily_member(opfamily, datatype, datatype,
									  BTGreaterEqualStrategyNumber);
		if (opr1oid == InvalidOid)
			elog(ERROR, "no >= operator for opfamily %u", opfamily);
	}
	else
	{
		opr1oid = get_opfamily_member(opfamily, datatype, datatype,
									  BTGreaterStrategyNumber);
		if (opr1oid == InvalidOid)
			elog(ERROR, "no > operator for opfamily %u", opfamily);
	}

	opr1right = network_scan_first(rightop);

	expr = make_opclause(opr1oid, BOOLOID, false,
						 (Expr *) leftop,
						 (Expr *) makeConst(datatype, -1,
											InvalidOid, /* not collatable */
											-1, opr1right,
											false, false),
						 InvalidOid, InvalidOid);
	result = list_make1(make_simple_restrictinfo(expr));

	/* create clause "key <= network_scan_last( rightop )" */

	opr2oid = get_opfamily_member(opfamily, datatype, datatype,
								  BTLessEqualStrategyNumber);
	if (opr2oid == InvalidOid)
		elog(ERROR, "no <= operator for opfamily %u", opfamily);

	opr2right = network_scan_last(rightop);

	expr = make_opclause(opr2oid, BOOLOID, false,
						 (Expr *) leftop,
						 (Expr *) makeConst(datatype, -1,
											InvalidOid, /* not collatable */
											-1, opr2right,
											false, false),
						 InvalidOid, InvalidOid);
	result = lappend(result, make_simple_restrictinfo(expr));

	return result;
}

/*
 * Handy subroutines for match_special_index_operator() and friends.
 */

/*
 * Generate a Datum of the appropriate type from a C string.
 * Note that all of the supported types are pass-by-ref, so the
 * returned value should be pfree'd if no longer needed.
 */
static Datum
string_to_datum(const char *str, Oid datatype)
{
	/*
	 * We cheat a little by assuming that CStringGetTextDatum() will do for
	 * bpchar and varchar constants too...
	 */
	if (datatype == NAMEOID)
		return DirectFunctionCall1(namein, CStringGetDatum(str));
	else if (datatype == BYTEAOID)
		return DirectFunctionCall1(byteain, CStringGetDatum(str));
	else
		return CStringGetTextDatum(str);
}

/*
 * Generate a Const node of the appropriate type from a C string.
 */
static Const *
string_to_const(const char *str, Oid datatype)
{
	Datum		conval = string_to_datum(str, datatype);
	Oid			collation;
	int			constlen;

	/*
	 * We only need to support a few datatypes here, so hard-wire properties
	 * instead of incurring the expense of catalog lookups.
	 */
	switch (datatype)
	{
		case TEXTOID:
		case VARCHAROID:
		case BPCHAROID:
			collation = DEFAULT_COLLATION_OID;
			constlen = -1;
			break;

		case NAMEOID:
			collation = InvalidOid;
			constlen = NAMEDATALEN;
			break;

		case BYTEAOID:
			collation = InvalidOid;
			constlen = -1;
			break;

		default:
			elog(ERROR, "unexpected datatype in string_to_const: %u",
				 datatype);
			return NULL;
	}

	return makeConst(datatype, -1, collation, constlen,
					 conval, false, false);
}
