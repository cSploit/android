/*-------------------------------------------------------------------------
 *
 * allpaths.c
 *	  Routines to find possible search paths for processing a query
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/optimizer/path/allpaths.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include <math.h>

#include "catalog/pg_class.h"
#include "nodes/nodeFuncs.h"
#ifdef OPTIMIZER_DEBUG
#include "nodes/print.h"
#endif
#include "optimizer/clauses.h"
#include "optimizer/cost.h"
#include "optimizer/geqo.h"
#include "optimizer/pathnode.h"
#include "optimizer/paths.h"
#include "optimizer/plancat.h"
#include "optimizer/planner.h"
#include "optimizer/prep.h"
#include "optimizer/restrictinfo.h"
#include "optimizer/var.h"
#include "parser/parse_clause.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteManip.h"
#include "utils/lsyscache.h"


/* These parameters are set by GUC */
bool		enable_geqo = false;	/* just in case GUC doesn't set it */
int			geqo_threshold;

/* Hook for plugins to replace standard_join_search() */
join_search_hook_type join_search_hook = NULL;


static void set_base_rel_pathlists(PlannerInfo *root);
static void set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
				 Index rti, RangeTblEntry *rte);
static void set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
					   RangeTblEntry *rte);
static void set_append_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
						Index rti, RangeTblEntry *rte);
static List *accumulate_append_subpath(List *subpaths, Path *path);
static void set_dummy_rel_pathlist(RelOptInfo *rel);
static void set_subquery_pathlist(PlannerInfo *root, RelOptInfo *rel,
					  Index rti, RangeTblEntry *rte);
static void set_function_pathlist(PlannerInfo *root, RelOptInfo *rel,
					  RangeTblEntry *rte);
static void set_values_pathlist(PlannerInfo *root, RelOptInfo *rel,
					RangeTblEntry *rte);
static void set_cte_pathlist(PlannerInfo *root, RelOptInfo *rel,
				 RangeTblEntry *rte);
static void set_worktable_pathlist(PlannerInfo *root, RelOptInfo *rel,
					   RangeTblEntry *rte);
static void set_foreign_pathlist(PlannerInfo *root, RelOptInfo *rel,
					 RangeTblEntry *rte);
static RelOptInfo *make_rel_from_joinlist(PlannerInfo *root, List *joinlist);
static bool subquery_is_pushdown_safe(Query *subquery, Query *topquery,
						  bool *unsafeColumns);
static bool recurse_pushdown_safe(Node *setOp, Query *topquery,
					  bool *unsafeColumns);
static void check_output_expressions(Query *subquery, bool *unsafeColumns);
static void compare_tlist_datatypes(List *tlist, List *colTypes,
						bool *unsafeColumns);
static bool qual_is_pushdown_safe(Query *subquery, Index rti, Node *qual,
					  bool *unsafeColumns);
static void subquery_push_qual(Query *subquery,
				   RangeTblEntry *rte, Index rti, Node *qual);
static void recurse_push_qual(Node *setOp, Query *topquery,
				  RangeTblEntry *rte, Index rti, Node *qual);


/*
 * make_one_rel
 *	  Finds all possible access paths for executing a query, returning a
 *	  single rel that represents the join of all base rels in the query.
 */
RelOptInfo *
make_one_rel(PlannerInfo *root, List *joinlist)
{
	RelOptInfo *rel;

	/*
	 * Generate access paths for the base rels.
	 */
	set_base_rel_pathlists(root);

	/*
	 * Generate access paths for the entire join tree.
	 */
	rel = make_rel_from_joinlist(root, joinlist);

	/*
	 * The result should join all and only the query's base rels.
	 */
#ifdef USE_ASSERT_CHECKING
	{
		int			num_base_rels = 0;
		Index		rti;

		for (rti = 1; rti < root->simple_rel_array_size; rti++)
		{
			RelOptInfo *brel = root->simple_rel_array[rti];

			if (brel == NULL)
				continue;

			Assert(brel->relid == rti); /* sanity check on array */

			/* ignore RTEs that are "other rels" */
			if (brel->reloptkind != RELOPT_BASEREL)
				continue;

			Assert(bms_is_member(rti, rel->relids));
			num_base_rels++;
		}

		Assert(bms_num_members(rel->relids) == num_base_rels);
	}
#endif

	return rel;
}

/*
 * set_base_rel_pathlists
 *	  Finds all paths available for scanning each base-relation entry.
 *	  Sequential scan and any available indices are considered.
 *	  Each useful path is attached to its relation's 'pathlist' field.
 */
static void
set_base_rel_pathlists(PlannerInfo *root)
{
	Index		rti;

	for (rti = 1; rti < root->simple_rel_array_size; rti++)
	{
		RelOptInfo *rel = root->simple_rel_array[rti];

		/* there may be empty slots corresponding to non-baserel RTEs */
		if (rel == NULL)
			continue;

		Assert(rel->relid == rti);		/* sanity check on array */

		/* ignore RTEs that are "other rels" */
		if (rel->reloptkind != RELOPT_BASEREL)
			continue;

		set_rel_pathlist(root, rel, rti, root->simple_rte_array[rti]);
	}
}

/*
 * set_rel_pathlist
 *	  Build access paths for a base relation
 */
static void
set_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
				 Index rti, RangeTblEntry *rte)
{
	if (rte->inh)
	{
		/* It's an "append relation", process accordingly */
		set_append_rel_pathlist(root, rel, rti, rte);
	}
	else
	{
		switch (rel->rtekind)
		{
			case RTE_RELATION:
				if (rte->relkind == RELKIND_FOREIGN_TABLE)
				{
					/* Foreign table */
					set_foreign_pathlist(root, rel, rte);
				}
				else
				{
					/* Plain relation */
					set_plain_rel_pathlist(root, rel, rte);
				}
				break;
			case RTE_SUBQUERY:
				/* Subquery --- generate a separate plan for it */
				set_subquery_pathlist(root, rel, rti, rte);
				break;
			case RTE_FUNCTION:
				/* RangeFunction --- generate a suitable path for it */
				set_function_pathlist(root, rel, rte);
				break;
			case RTE_VALUES:
				/* Values list --- generate a suitable path for it */
				set_values_pathlist(root, rel, rte);
				break;
			case RTE_CTE:
				/* CTE reference --- generate a suitable path for it */
				if (rte->self_reference)
					set_worktable_pathlist(root, rel, rte);
				else
					set_cte_pathlist(root, rel, rte);
				break;
			default:
				elog(ERROR, "unexpected rtekind: %d", (int) rel->rtekind);
				break;
		}
	}

#ifdef OPTIMIZER_DEBUG
	debug_print_rel(root, rel);
#endif
}

/*
 * set_plain_rel_pathlist
 *	  Build access paths for a plain relation (no subquery, no inheritance)
 */
static void
set_plain_rel_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	/*
	 * If we can prove we don't need to scan the rel via constraint exclusion,
	 * set up a single dummy path for it.  We only need to check for regular
	 * baserels; if it's an otherrel, CE was already checked in
	 * set_append_rel_pathlist().
	 */
	if (rel->reloptkind == RELOPT_BASEREL &&
		relation_excluded_by_constraints(root, rel, rte))
	{
		set_dummy_rel_pathlist(rel);
		return;
	}

	/*
	 * Test any partial indexes of rel for applicability.  We must do this
	 * first since partial unique indexes can affect size estimates.
	 */
	check_partial_indexes(root, rel);

	/* Mark rel with estimated output rows, width, etc */
	set_baserel_size_estimates(root, rel);

	/*
	 * Check to see if we can extract any restriction conditions from join
	 * quals that are OR-of-AND structures.  If so, add them to the rel's
	 * restriction list, and redo the above steps.
	 */
	if (create_or_index_quals(root, rel))
	{
		check_partial_indexes(root, rel);
		set_baserel_size_estimates(root, rel);
	}

	/*
	 * Generate paths and add them to the rel's pathlist.
	 *
	 * Note: add_path() will discard any paths that are dominated by another
	 * available path, keeping only those paths that are superior along at
	 * least one dimension of cost or sortedness.
	 */

	/* Consider sequential scan */
	add_path(rel, create_seqscan_path(root, rel));

	/* Consider index scans */
	create_index_paths(root, rel);

	/* Consider TID scans */
	create_tidscan_paths(root, rel);

	/* Now find the cheapest of the paths for this rel */
	set_cheapest(rel);
}

/*
 * set_append_rel_pathlist
 *	  Build access paths for an "append relation"
 *
 * The passed-in rel and RTE represent the entire append relation.	The
 * relation's contents are computed by appending together the output of
 * the individual member relations.  Note that in the inheritance case,
 * the first member relation is actually the same table as is mentioned in
 * the parent RTE ... but it has a different RTE and RelOptInfo.  This is
 * a good thing because their outputs are not the same size.
 */
static void
set_append_rel_pathlist(PlannerInfo *root, RelOptInfo *rel,
						Index rti, RangeTblEntry *rte)
{
	int			parentRTindex = rti;
	List	   *live_childrels = NIL;
	List	   *subpaths = NIL;
	List	   *all_child_pathkeys = NIL;
	double		parent_rows;
	double		parent_size;
	double	   *parent_attrsizes;
	int			nattrs;
	ListCell   *l;

	/*
	 * Initialize to compute size estimates for whole append relation.
	 *
	 * We handle width estimates by weighting the widths of different child
	 * rels proportionally to their number of rows.  This is sensible because
	 * the use of width estimates is mainly to compute the total relation
	 * "footprint" if we have to sort or hash it.  To do this, we sum the
	 * total equivalent size (in "double" arithmetic) and then divide by the
	 * total rowcount estimate.  This is done separately for the total rel
	 * width and each attribute.
	 *
	 * Note: if you consider changing this logic, beware that child rels could
	 * have zero rows and/or width, if they were excluded by constraints.
	 */
	parent_rows = 0;
	parent_size = 0;
	nattrs = rel->max_attr - rel->min_attr + 1;
	parent_attrsizes = (double *) palloc0(nattrs * sizeof(double));

	/*
	 * Generate access paths for each member relation, and pick the cheapest
	 * path for each one.
	 */
	foreach(l, root->append_rel_list)
	{
		AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(l);
		int			childRTindex;
		RangeTblEntry *childRTE;
		RelOptInfo *childrel;
		List	   *childquals;
		Node	   *childqual;
		ListCell   *lcp;
		ListCell   *parentvars;
		ListCell   *childvars;

		/* append_rel_list contains all append rels; ignore others */
		if (appinfo->parent_relid != parentRTindex)
			continue;

		childRTindex = appinfo->child_relid;
		childRTE = root->simple_rte_array[childRTindex];

		/*
		 * The child rel's RelOptInfo was already created during
		 * add_base_rels_to_query.
		 */
		childrel = find_base_rel(root, childRTindex);
		Assert(childrel->reloptkind == RELOPT_OTHER_MEMBER_REL);

		/*
		 * We have to copy the parent's targetlist and quals to the child,
		 * with appropriate substitution of variables.	However, only the
		 * baserestrictinfo quals are needed before we can check for
		 * constraint exclusion; so do that first and then check to see if we
		 * can disregard this child.
		 *
		 * As of 8.4, the child rel's targetlist might contain non-Var
		 * expressions, which means that substitution into the quals could
		 * produce opportunities for const-simplification, and perhaps even
		 * pseudoconstant quals.  To deal with this, we strip the RestrictInfo
		 * nodes, do the substitution, do const-simplification, and then
		 * reconstitute the RestrictInfo layer.
		 */
		childquals = get_all_actual_clauses(rel->baserestrictinfo);
		childquals = (List *) adjust_appendrel_attrs((Node *) childquals,
													 appinfo);
		childqual = eval_const_expressions(root, (Node *)
										   make_ands_explicit(childquals));
		if (childqual && IsA(childqual, Const) &&
			(((Const *) childqual)->constisnull ||
			 !DatumGetBool(((Const *) childqual)->constvalue)))
		{
			/*
			 * Restriction reduces to constant FALSE or constant NULL after
			 * substitution, so this child need not be scanned.
			 */
			set_dummy_rel_pathlist(childrel);
			continue;
		}
		childquals = make_ands_implicit((Expr *) childqual);
		childquals = make_restrictinfos_from_actual_clauses(root,
															childquals);
		childrel->baserestrictinfo = childquals;

		if (relation_excluded_by_constraints(root, childrel, childRTE))
		{
			/*
			 * This child need not be scanned, so we can omit it from the
			 * appendrel.  Mark it with a dummy cheapest-path though, in case
			 * best_appendrel_indexscan() looks at it later.
			 */
			set_dummy_rel_pathlist(childrel);
			continue;
		}

		/*
		 * CE failed, so finish copying/modifying targetlist and join quals.
		 *
		 * Note: the resulting childrel->reltargetlist may contain arbitrary
		 * expressions, which normally would not occur in a reltargetlist.
		 * That is okay because nothing outside of this routine will look at
		 * the child rel's reltargetlist.  We do have to cope with the case
		 * while constructing attr_widths estimates below, though.
		 */
		childrel->joininfo = (List *)
			adjust_appendrel_attrs((Node *) rel->joininfo,
								   appinfo);
		childrel->reltargetlist = (List *)
			adjust_appendrel_attrs((Node *) rel->reltargetlist,
								   appinfo);

		/*
		 * We have to make child entries in the EquivalenceClass data
		 * structures as well.	This is needed either if the parent
		 * participates in some eclass joins (because we will want to consider
		 * inner-indexscan joins on the individual children) or if the parent
		 * has useful pathkeys (because we should try to build MergeAppend
		 * paths that produce those sort orderings).
		 */
		if (rel->has_eclass_joins || has_useful_pathkeys(root, rel))
			add_child_rel_equivalences(root, appinfo, rel, childrel);
		childrel->has_eclass_joins = rel->has_eclass_joins;

		/*
		 * Note: we could compute appropriate attr_needed data for the child's
		 * variables, by transforming the parent's attr_needed through the
		 * translated_vars mapping.  However, currently there's no need
		 * because attr_needed is only examined for base relations not
		 * otherrels.  So we just leave the child's attr_needed empty.
		 */

		/* Remember which childrels are live, for MergeAppend logic below */
		live_childrels = lappend(live_childrels, childrel);

		/*
		 * Compute the child's access paths, and add the cheapest one to the
		 * Append path we are constructing for the parent.
		 */
		set_rel_pathlist(root, childrel, childRTindex, childRTE);

		subpaths = accumulate_append_subpath(subpaths,
											 childrel->cheapest_total_path);

		/*
		 * Collect a list of all the available path orderings for all the
		 * children.  We use this as a heuristic to indicate which sort
		 * orderings we should build MergeAppend paths for.
		 */
		foreach(lcp, childrel->pathlist)
		{
			Path	   *childpath = (Path *) lfirst(lcp);
			List	   *childkeys = childpath->pathkeys;
			ListCell   *lpk;
			bool		found = false;

			/* Ignore unsorted paths */
			if (childkeys == NIL)
				continue;

			/* Have we already seen this ordering? */
			foreach(lpk, all_child_pathkeys)
			{
				List	   *existing_pathkeys = (List *) lfirst(lpk);

				if (compare_pathkeys(existing_pathkeys,
									 childkeys) == PATHKEYS_EQUAL)
				{
					found = true;
					break;
				}
			}
			if (!found)
			{
				/* No, so add it to all_child_pathkeys */
				all_child_pathkeys = lappend(all_child_pathkeys, childkeys);
			}
		}

		/*
		 * Accumulate size information from each child.
		 */
		if (childrel->rows > 0)
		{
			parent_rows += childrel->rows;
			parent_size += childrel->width * childrel->rows;

			/*
			 * Accumulate per-column estimates too.  We need not do anything
			 * for PlaceHolderVars in the parent list.  If child expression
			 * isn't a Var, or we didn't record a width estimate for it, we
			 * have to fall back on a datatype-based estimate.
			 *
			 * By construction, child's reltargetlist is 1-to-1 with parent's.
			 */
			forboth(parentvars, rel->reltargetlist,
					childvars, childrel->reltargetlist)
			{
				Var		   *parentvar = (Var *) lfirst(parentvars);
				Node	   *childvar = (Node *) lfirst(childvars);

				if (IsA(parentvar, Var))
				{
					int			pndx = parentvar->varattno - rel->min_attr;
					int32		child_width = 0;

					if (IsA(childvar, Var))
					{
						int		cndx = ((Var *) childvar)->varattno - childrel->min_attr;

						child_width = childrel->attr_widths[cndx];
					}
					if (child_width <= 0)
						child_width = get_typavgwidth(exprType(childvar),
													  exprTypmod(childvar));
					Assert(child_width > 0);
					parent_attrsizes[pndx] += child_width * childrel->rows;
				}
			}
		}
	}

	/*
	 * Save the finished size estimates.
	 */
	rel->rows = parent_rows;
	if (parent_rows > 0)
	{
		int			i;

		rel->width = rint(parent_size / parent_rows);
		for (i = 0; i < nattrs; i++)
			rel->attr_widths[i] = rint(parent_attrsizes[i] / parent_rows);
	}
	else
		rel->width = 0;			/* attr_widths should be zero already */

	/*
	 * Set "raw tuples" count equal to "rows" for the appendrel; needed
	 * because some places assume rel->tuples is valid for any baserel.
	 */
	rel->tuples = parent_rows;

	pfree(parent_attrsizes);

	/*
	 * Next, build an unordered Append path for the rel.  (Note: this is
	 * correct even if we have zero or one live subpath due to constraint
	 * exclusion.)
	 */
	add_path(rel, (Path *) create_append_path(rel, subpaths));

	/*
	 * Next, build MergeAppend paths based on the collected list of child
	 * pathkeys.  We consider both cheapest-startup and cheapest-total cases,
	 * ie, for each interesting ordering, collect all the cheapest startup
	 * subpaths and all the cheapest total paths, and build a MergeAppend path
	 * for each list.
	 */
	foreach(l, all_child_pathkeys)
	{
		List	   *pathkeys = (List *) lfirst(l);
		List	   *startup_subpaths = NIL;
		List	   *total_subpaths = NIL;
		bool		startup_neq_total = false;
		ListCell   *lcr;

		/* Select the child paths for this ordering... */
		foreach(lcr, live_childrels)
		{
			RelOptInfo *childrel = (RelOptInfo *) lfirst(lcr);
			Path	   *cheapest_startup,
					   *cheapest_total;

			/* Locate the right paths, if they are available. */
			cheapest_startup =
				get_cheapest_path_for_pathkeys(childrel->pathlist,
											   pathkeys,
											   STARTUP_COST);
			cheapest_total =
				get_cheapest_path_for_pathkeys(childrel->pathlist,
											   pathkeys,
											   TOTAL_COST);

			/*
			 * If we can't find any paths with the right order just add the
			 * cheapest-total path; we'll have to sort it.
			 */
			if (cheapest_startup == NULL)
				cheapest_startup = childrel->cheapest_total_path;
			if (cheapest_total == NULL)
				cheapest_total = childrel->cheapest_total_path;

			/*
			 * Notice whether we actually have different paths for the
			 * "cheapest" and "total" cases; frequently there will be no point
			 * in two create_merge_append_path() calls.
			 */
			if (cheapest_startup != cheapest_total)
				startup_neq_total = true;

			startup_subpaths =
				accumulate_append_subpath(startup_subpaths, cheapest_startup);
			total_subpaths =
				accumulate_append_subpath(total_subpaths, cheapest_total);
		}

		/* ... and build the MergeAppend paths */
		add_path(rel, (Path *) create_merge_append_path(root,
														rel,
														startup_subpaths,
														pathkeys));
		if (startup_neq_total)
			add_path(rel, (Path *) create_merge_append_path(root,
															rel,
															total_subpaths,
															pathkeys));
	}

	/* Select cheapest path */
	set_cheapest(rel);
}

/*
 * accumulate_append_subpath
 *		Add a subpath to the list being built for an Append or MergeAppend
 *
 * It's possible that the child is itself an Append path, in which case
 * we can "cut out the middleman" and just add its child paths to our
 * own list.  (We don't try to do this earlier because we need to
 * apply both levels of transformation to the quals.)
 */
static List *
accumulate_append_subpath(List *subpaths, Path *path)
{
	if (IsA(path, AppendPath))
	{
		AppendPath *apath = (AppendPath *) path;

		/* list_copy is important here to avoid sharing list substructure */
		return list_concat(subpaths, list_copy(apath->subpaths));
	}
	else
		return lappend(subpaths, path);
}

/*
 * set_dummy_rel_pathlist
 *	  Build a dummy path for a relation that's been excluded by constraints
 *
 * Rather than inventing a special "dummy" path type, we represent this as an
 * AppendPath with no members (see also IS_DUMMY_PATH macro).
 */
static void
set_dummy_rel_pathlist(RelOptInfo *rel)
{
	/* Set dummy size estimates --- we leave attr_widths[] as zeroes */
	rel->rows = 0;
	rel->width = 0;

	add_path(rel, (Path *) create_append_path(rel, NIL));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/* quick-and-dirty test to see if any joining is needed */
static bool
has_multiple_baserels(PlannerInfo *root)
{
	int			num_base_rels = 0;
	Index		rti;

	for (rti = 1; rti < root->simple_rel_array_size; rti++)
	{
		RelOptInfo *brel = root->simple_rel_array[rti];

		if (brel == NULL)
			continue;

		/* ignore RTEs that are "other rels" */
		if (brel->reloptkind == RELOPT_BASEREL)
			if (++num_base_rels > 1)
				return true;
	}
	return false;
}

/*
 * set_subquery_pathlist
 *		Build the (single) access path for a subquery RTE
 */
static void
set_subquery_pathlist(PlannerInfo *root, RelOptInfo *rel,
					  Index rti, RangeTblEntry *rte)
{
	Query	   *parse = root->parse;
	Query	   *subquery = rte->subquery;
	bool	   *unsafeColumns;
	double		tuple_fraction;
	PlannerInfo *subroot;
	List	   *pathkeys;

	/*
	 * Must copy the Query so that planning doesn't mess up the RTE contents
	 * (really really need to fix the planner to not scribble on its input,
	 * someday).
	 */
	subquery = copyObject(subquery);

	/*
	 * We need a workspace for keeping track of unsafe-to-reference columns.
	 * unsafeColumns[i] is set TRUE if we've found that output column i of the
	 * subquery is unsafe to use in a pushed-down qual.
	 */
	unsafeColumns = (bool *)
		palloc0((list_length(subquery->targetList) + 1) * sizeof(bool));

	/*
	 * If there are any restriction clauses that have been attached to the
	 * subquery relation, consider pushing them down to become WHERE or HAVING
	 * quals of the subquery itself.  This transformation is useful because it
	 * may allow us to generate a better plan for the subquery than evaluating
	 * all the subquery output rows and then filtering them.
	 *
	 * There are several cases where we cannot push down clauses. Restrictions
	 * involving the subquery are checked by subquery_is_pushdown_safe().
	 * Restrictions on individual clauses are checked by
	 * qual_is_pushdown_safe().  Also, we don't want to push down
	 * pseudoconstant clauses; better to have the gating node above the
	 * subquery.
	 *
	 * Non-pushed-down clauses will get evaluated as qpquals of the
	 * SubqueryScan node.
	 *
	 * XXX Are there any cases where we want to make a policy decision not to
	 * push down a pushable qual, because it'd result in a worse plan?
	 */
	if (rel->baserestrictinfo != NIL &&
		subquery_is_pushdown_safe(subquery, subquery, unsafeColumns))
	{
		/* OK to consider pushing down individual quals */
		List	   *upperrestrictlist = NIL;
		ListCell   *l;

		foreach(l, rel->baserestrictinfo)
		{
			RestrictInfo *rinfo = (RestrictInfo *) lfirst(l);
			Node	   *clause = (Node *) rinfo->clause;

			if (!rinfo->pseudoconstant &&
				qual_is_pushdown_safe(subquery, rti, clause, unsafeColumns))
			{
				/* Push it down */
				subquery_push_qual(subquery, rte, rti, clause);
			}
			else
			{
				/* Keep it in the upper query */
				upperrestrictlist = lappend(upperrestrictlist, rinfo);
			}
		}
		rel->baserestrictinfo = upperrestrictlist;
	}

	pfree(unsafeColumns);

	/*
	 * We can safely pass the outer tuple_fraction down to the subquery if the
	 * outer level has no joining, aggregation, or sorting to do. Otherwise
	 * we'd better tell the subquery to plan for full retrieval. (XXX This
	 * could probably be made more intelligent ...)
	 */
	if (parse->hasAggs ||
		parse->groupClause ||
		parse->havingQual ||
		parse->distinctClause ||
		parse->sortClause ||
		has_multiple_baserels(root))
		tuple_fraction = 0.0;	/* default case */
	else
		tuple_fraction = root->tuple_fraction;

	/* plan_params should not be in use in current query level */
	Assert(root->plan_params == NIL);

	/* Generate the plan for the subquery */
	rel->subplan = subquery_planner(root->glob, subquery,
									root,
									false, tuple_fraction,
									&subroot);
	rel->subrtable = subroot->parse->rtable;
	rel->subrowmark = subroot->rowMarks;

	/*
	 * Since we don't yet support LATERAL, it should not be possible for the
	 * sub-query to have requested parameters of this level.
	 */
	if (root->plan_params)
		elog(ERROR, "unexpected outer reference in subquery in FROM");

	/* Mark rel with estimated output rows, width, etc */
	set_subquery_size_estimates(root, rel, subroot);

	/* Convert subquery pathkeys to outer representation */
	pathkeys = convert_subquery_pathkeys(root, rel, subroot->query_pathkeys);

	/* Generate appropriate path */
	add_path(rel, create_subqueryscan_path(rel, pathkeys));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * set_function_pathlist
 *		Build the (single) access path for a function RTE
 */
static void
set_function_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	/* Mark rel with estimated output rows, width, etc */
	set_function_size_estimates(root, rel);

	/* Generate appropriate path */
	add_path(rel, create_functionscan_path(root, rel));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * set_values_pathlist
 *		Build the (single) access path for a VALUES RTE
 */
static void
set_values_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	/* Mark rel with estimated output rows, width, etc */
	set_values_size_estimates(root, rel);

	/* Generate appropriate path */
	add_path(rel, create_valuesscan_path(root, rel));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * set_cte_pathlist
 *		Build the (single) access path for a non-self-reference CTE RTE
 */
static void
set_cte_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	Plan	   *cteplan;
	PlannerInfo *cteroot;
	Index		levelsup;
	int			ndx;
	ListCell   *lc;
	int			plan_id;

	/*
	 * Find the referenced CTE, and locate the plan previously made for it.
	 */
	levelsup = rte->ctelevelsup;
	cteroot = root;
	while (levelsup-- > 0)
	{
		cteroot = cteroot->parent_root;
		if (!cteroot)			/* shouldn't happen */
			elog(ERROR, "bad levelsup for CTE \"%s\"", rte->ctename);
	}

	/*
	 * Note: cte_plan_ids can be shorter than cteList, if we are still working
	 * on planning the CTEs (ie, this is a side-reference from another CTE).
	 * So we mustn't use forboth here.
	 */
	ndx = 0;
	foreach(lc, cteroot->parse->cteList)
	{
		CommonTableExpr *cte = (CommonTableExpr *) lfirst(lc);

		if (strcmp(cte->ctename, rte->ctename) == 0)
			break;
		ndx++;
	}
	if (lc == NULL)				/* shouldn't happen */
		elog(ERROR, "could not find CTE \"%s\"", rte->ctename);
	if (ndx >= list_length(cteroot->cte_plan_ids))
		elog(ERROR, "could not find plan for CTE \"%s\"", rte->ctename);
	plan_id = list_nth_int(cteroot->cte_plan_ids, ndx);
	Assert(plan_id > 0);
	cteplan = (Plan *) list_nth(root->glob->subplans, plan_id - 1);

	/* Mark rel with estimated output rows, width, etc */
	set_cte_size_estimates(root, rel, cteplan);

	/* Generate appropriate path */
	add_path(rel, create_ctescan_path(root, rel));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * set_worktable_pathlist
 *		Build the (single) access path for a self-reference CTE RTE
 */
static void
set_worktable_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	Plan	   *cteplan;
	PlannerInfo *cteroot;
	Index		levelsup;

	/*
	 * We need to find the non-recursive term's plan, which is in the plan
	 * level that's processing the recursive UNION, which is one level *below*
	 * where the CTE comes from.
	 */
	levelsup = rte->ctelevelsup;
	if (levelsup == 0)			/* shouldn't happen */
		elog(ERROR, "bad levelsup for CTE \"%s\"", rte->ctename);
	levelsup--;
	cteroot = root;
	while (levelsup-- > 0)
	{
		cteroot = cteroot->parent_root;
		if (!cteroot)			/* shouldn't happen */
			elog(ERROR, "bad levelsup for CTE \"%s\"", rte->ctename);
	}
	cteplan = cteroot->non_recursive_plan;
	if (!cteplan)				/* shouldn't happen */
		elog(ERROR, "could not find plan for CTE \"%s\"", rte->ctename);

	/* Mark rel with estimated output rows, width, etc */
	set_cte_size_estimates(root, rel, cteplan);

	/* Generate appropriate path */
	add_path(rel, create_worktablescan_path(root, rel));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * set_foreign_pathlist
 *		Build the (single) access path for a foreign table RTE
 */
static void
set_foreign_pathlist(PlannerInfo *root, RelOptInfo *rel, RangeTblEntry *rte)
{
	/* Mark rel with estimated output rows, width, etc */
	set_foreign_size_estimates(root, rel);

	/* Generate appropriate path */
	add_path(rel, (Path *) create_foreignscan_path(root, rel));

	/* Select cheapest path (pretty easy in this case...) */
	set_cheapest(rel);
}

/*
 * make_rel_from_joinlist
 *	  Build access paths using a "joinlist" to guide the join path search.
 *
 * See comments for deconstruct_jointree() for definition of the joinlist
 * data structure.
 */
static RelOptInfo *
make_rel_from_joinlist(PlannerInfo *root, List *joinlist)
{
	int			levels_needed;
	List	   *initial_rels;
	ListCell   *jl;

	/*
	 * Count the number of child joinlist nodes.  This is the depth of the
	 * dynamic-programming algorithm we must employ to consider all ways of
	 * joining the child nodes.
	 */
	levels_needed = list_length(joinlist);

	if (levels_needed <= 0)
		return NULL;			/* nothing to do? */

	/*
	 * Construct a list of rels corresponding to the child joinlist nodes.
	 * This may contain both base rels and rels constructed according to
	 * sub-joinlists.
	 */
	initial_rels = NIL;
	foreach(jl, joinlist)
	{
		Node	   *jlnode = (Node *) lfirst(jl);
		RelOptInfo *thisrel;

		if (IsA(jlnode, RangeTblRef))
		{
			int			varno = ((RangeTblRef *) jlnode)->rtindex;

			thisrel = find_base_rel(root, varno);
		}
		else if (IsA(jlnode, List))
		{
			/* Recurse to handle subproblem */
			thisrel = make_rel_from_joinlist(root, (List *) jlnode);
		}
		else
		{
			elog(ERROR, "unrecognized joinlist node type: %d",
				 (int) nodeTag(jlnode));
			thisrel = NULL;		/* keep compiler quiet */
		}

		initial_rels = lappend(initial_rels, thisrel);
	}

	if (levels_needed == 1)
	{
		/*
		 * Single joinlist node, so we're done.
		 */
		return (RelOptInfo *) linitial(initial_rels);
	}
	else
	{
		/*
		 * Consider the different orders in which we could join the rels,
		 * using a plugin, GEQO, or the regular join search code.
		 *
		 * We put the initial_rels list into a PlannerInfo field because
		 * has_legal_joinclause() needs to look at it (ugly :-().
		 */
		root->initial_rels = initial_rels;

		if (join_search_hook)
			return (*join_search_hook) (root, levels_needed, initial_rels);
		else if (enable_geqo && levels_needed >= geqo_threshold)
			return geqo(root, levels_needed, initial_rels);
		else
			return standard_join_search(root, levels_needed, initial_rels);
	}
}

/*
 * standard_join_search
 *	  Find possible joinpaths for a query by successively finding ways
 *	  to join component relations into join relations.
 *
 * 'levels_needed' is the number of iterations needed, ie, the number of
 *		independent jointree items in the query.  This is > 1.
 *
 * 'initial_rels' is a list of RelOptInfo nodes for each independent
 *		jointree item.	These are the components to be joined together.
 *		Note that levels_needed == list_length(initial_rels).
 *
 * Returns the final level of join relations, i.e., the relation that is
 * the result of joining all the original relations together.
 * At least one implementation path must be provided for this relation and
 * all required sub-relations.
 *
 * To support loadable plugins that modify planner behavior by changing the
 * join searching algorithm, we provide a hook variable that lets a plugin
 * replace or supplement this function.  Any such hook must return the same
 * final join relation as the standard code would, but it might have a
 * different set of implementation paths attached, and only the sub-joinrels
 * needed for these paths need have been instantiated.
 *
 * Note to plugin authors: the functions invoked during standard_join_search()
 * modify root->join_rel_list and root->join_rel_hash.	If you want to do more
 * than one join-order search, you'll probably need to save and restore the
 * original states of those data structures.  See geqo_eval() for an example.
 */
RelOptInfo *
standard_join_search(PlannerInfo *root, int levels_needed, List *initial_rels)
{
	int			lev;
	RelOptInfo *rel;

	/*
	 * This function cannot be invoked recursively within any one planning
	 * problem, so join_rel_level[] can't be in use already.
	 */
	Assert(root->join_rel_level == NULL);

	/*
	 * We employ a simple "dynamic programming" algorithm: we first find all
	 * ways to build joins of two jointree items, then all ways to build joins
	 * of three items (from two-item joins and single items), then four-item
	 * joins, and so on until we have considered all ways to join all the
	 * items into one rel.
	 *
	 * root->join_rel_level[j] is a list of all the j-item rels.  Initially we
	 * set root->join_rel_level[1] to represent all the single-jointree-item
	 * relations.
	 */
	root->join_rel_level = (List **) palloc0((levels_needed + 1) * sizeof(List *));

	root->join_rel_level[1] = initial_rels;

	for (lev = 2; lev <= levels_needed; lev++)
	{
		ListCell   *lc;

		/*
		 * Determine all possible pairs of relations to be joined at this
		 * level, and build paths for making each one from every available
		 * pair of lower-level relations.
		 */
		join_search_one_level(root, lev);

		/*
		 * Do cleanup work on each just-processed rel.
		 */
		foreach(lc, root->join_rel_level[lev])
		{
			rel = (RelOptInfo *) lfirst(lc);

			/* Find and save the cheapest paths for this rel */
			set_cheapest(rel);

#ifdef OPTIMIZER_DEBUG
			debug_print_rel(root, rel);
#endif
		}
	}

	/*
	 * We should have a single rel at the final level.
	 */
	if (root->join_rel_level[levels_needed] == NIL)
		elog(ERROR, "failed to build any %d-way joins", levels_needed);
	Assert(list_length(root->join_rel_level[levels_needed]) == 1);

	rel = (RelOptInfo *) linitial(root->join_rel_level[levels_needed]);

	root->join_rel_level = NULL;

	return rel;
}

/*****************************************************************************
 *			PUSHING QUALS DOWN INTO SUBQUERIES
 *****************************************************************************/

/*
 * subquery_is_pushdown_safe - is a subquery safe for pushing down quals?
 *
 * subquery is the particular component query being checked.  topquery
 * is the top component of a set-operations tree (the same Query if no
 * set-op is involved).
 *
 * Conditions checked here:
 *
 * 1. If the subquery has a LIMIT clause, we must not push down any quals,
 * since that could change the set of rows returned.
 *
 * 2. If the subquery contains any window functions, we can't push quals
 * into it, because that could change the results.
 *
 * 3. If the subquery contains EXCEPT or EXCEPT ALL set ops we cannot push
 * quals into it, because that could change the results.
 *
 * In addition, we make several checks on the subquery's output columns
 * to see if it is safe to reference them in pushed-down quals.  If output
 * column k is found to be unsafe to reference, we set unsafeColumns[k] to
 * TRUE, but we don't reject the subquery overall since column k might
 * not be referenced by some/all quals.  The unsafeColumns[] array will be
 * consulted later by qual_is_pushdown_safe().	It's better to do it this
 * way than to make the checks directly in qual_is_pushdown_safe(), because
 * when the subquery involves set operations we have to check the output
 * expressions in each arm of the set op.
 */
static bool
subquery_is_pushdown_safe(Query *subquery, Query *topquery,
						  bool *unsafeColumns)
{
	SetOperationStmt *topop;

	/* Check point 1 */
	if (subquery->limitOffset != NULL || subquery->limitCount != NULL)
		return false;

	/* Check point 2 */
	if (subquery->hasWindowFuncs)
		return false;

	/*
	 * If we're at a leaf query, check for unsafe expressions in its target
	 * list, and mark any unsafe ones in unsafeColumns[].  (Non-leaf nodes in
	 * setop trees have only simple Vars in their tlists, so no need to check
	 * them.)
	 */
	if (subquery->setOperations == NULL)
		check_output_expressions(subquery, unsafeColumns);

	/* Are we at top level, or looking at a setop component? */
	if (subquery == topquery)
	{
		/* Top level, so check any component queries */
		if (subquery->setOperations != NULL)
			if (!recurse_pushdown_safe(subquery->setOperations, topquery,
									   unsafeColumns))
				return false;
	}
	else
	{
		/* Setop component must not have more components (too weird) */
		if (subquery->setOperations != NULL)
			return false;
		/* Check whether setop component output types match top level */
		topop = (SetOperationStmt *) topquery->setOperations;
		Assert(topop && IsA(topop, SetOperationStmt));
		compare_tlist_datatypes(subquery->targetList,
								topop->colTypes,
								unsafeColumns);
	}
	return true;
}

/*
 * Helper routine to recurse through setOperations tree
 */
static bool
recurse_pushdown_safe(Node *setOp, Query *topquery,
					  bool *unsafeColumns)
{
	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		RangeTblEntry *rte = rt_fetch(rtr->rtindex, topquery->rtable);
		Query	   *subquery = rte->subquery;

		Assert(subquery != NULL);
		return subquery_is_pushdown_safe(subquery, topquery, unsafeColumns);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		/* EXCEPT is no good (point 3 for subquery_is_pushdown_safe) */
		if (op->op == SETOP_EXCEPT)
			return false;
		/* Else recurse */
		if (!recurse_pushdown_safe(op->larg, topquery, unsafeColumns))
			return false;
		if (!recurse_pushdown_safe(op->rarg, topquery, unsafeColumns))
			return false;
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
	}
	return true;
}

/*
 * check_output_expressions - check subquery's output expressions for safety
 *
 * There are several cases in which it's unsafe to push down an upper-level
 * qual if it references a particular output column of a subquery.	We check
 * each output column of the subquery and set unsafeColumns[k] to TRUE if
 * that column is unsafe for a pushed-down qual to reference.  The conditions
 * checked here are:
 *
 * 1. We must not push down any quals that refer to subselect outputs that
 * return sets, else we'd introduce functions-returning-sets into the
 * subquery's WHERE/HAVING quals.
 *
 * 2. We must not push down any quals that refer to subselect outputs that
 * contain volatile functions, for fear of introducing strange results due
 * to multiple evaluation of a volatile function.
 *
 * 3. If the subquery uses DISTINCT ON, we must not push down any quals that
 * refer to non-DISTINCT output columns, because that could change the set
 * of rows returned.  (This condition is vacuous for DISTINCT, because then
 * there are no non-DISTINCT output columns, so we needn't check.  But note
 * we are assuming that the qual can't distinguish values that the DISTINCT
 * operator sees as equal.	This is a bit shaky but we have no way to test
 * for the case, and it's unlikely enough that we shouldn't refuse the
 * optimization just because it could theoretically happen.)
 */
static void
check_output_expressions(Query *subquery, bool *unsafeColumns)
{
	ListCell   *lc;

	foreach(lc, subquery->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(lc);

		if (tle->resjunk)
			continue;			/* ignore resjunk columns */

		/* We need not check further if output col is already known unsafe */
		if (unsafeColumns[tle->resno])
			continue;

		/* Functions returning sets are unsafe (point 1) */
		if (expression_returns_set((Node *) tle->expr))
		{
			unsafeColumns[tle->resno] = true;
			continue;
		}

		/* Volatile functions are unsafe (point 2) */
		if (contain_volatile_functions((Node *) tle->expr))
		{
			unsafeColumns[tle->resno] = true;
			continue;
		}

		/* If subquery uses DISTINCT ON, check point 3 */
		if (subquery->hasDistinctOn &&
			!targetIsInSortList(tle, InvalidOid, subquery->distinctClause))
		{
			/* non-DISTINCT column, so mark it unsafe */
			unsafeColumns[tle->resno] = true;
			continue;
		}
	}
}

/*
 * For subqueries using UNION/UNION ALL/INTERSECT/INTERSECT ALL, we can
 * push quals into each component query, but the quals can only reference
 * subquery columns that suffer no type coercions in the set operation.
 * Otherwise there are possible semantic gotchas.  So, we check the
 * component queries to see if any of them have output types different from
 * the top-level setop outputs.  unsafeColumns[k] is set true if column k
 * has different type in any component.
 *
 * We don't have to care about typmods here: the only allowed difference
 * between set-op input and output typmods is input is a specific typmod
 * and output is -1, and that does not require a coercion.
 *
 * tlist is a subquery tlist.
 * colTypes is an OID list of the top-level setop's output column types.
 * unsafeColumns[] is the result array.
 */
static void
compare_tlist_datatypes(List *tlist, List *colTypes,
						bool *unsafeColumns)
{
	ListCell   *l;
	ListCell   *colType = list_head(colTypes);

	foreach(l, tlist)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(l);

		if (tle->resjunk)
			continue;			/* ignore resjunk columns */
		if (colType == NULL)
			elog(ERROR, "wrong number of tlist entries");
		if (exprType((Node *) tle->expr) != lfirst_oid(colType))
			unsafeColumns[tle->resno] = true;
		colType = lnext(colType);
	}
	if (colType != NULL)
		elog(ERROR, "wrong number of tlist entries");
}

/*
 * qual_is_pushdown_safe - is a particular qual safe to push down?
 *
 * qual is a restriction clause applying to the given subquery (whose RTE
 * has index rti in the parent query).
 *
 * Conditions checked here:
 *
 * 1. The qual must not contain any subselects (mainly because I'm not sure
 * it will work correctly: sublinks will already have been transformed into
 * subplans in the qual, but not in the subquery).
 *
 * 2. The qual must not refer to the whole-row output of the subquery
 * (since there is no easy way to name that within the subquery itself).
 *
 * 3. The qual must not refer to any subquery output columns that were
 * found to be unsafe to reference by subquery_is_pushdown_safe().
 */
static bool
qual_is_pushdown_safe(Query *subquery, Index rti, Node *qual,
					  bool *unsafeColumns)
{
	bool		safe = true;
	List	   *vars;
	ListCell   *vl;

	/* Refuse subselects (point 1) */
	if (contain_subplans(qual))
		return false;

	/*
	 * It would be unsafe to push down window function calls, but at least for
	 * the moment we could never see any in a qual anyhow.  (The same applies
	 * to aggregates, which we check for in pull_var_clause below.)
	 */
	Assert(!contain_window_function(qual));

	/*
	 * Examine all Vars used in clause; since it's a restriction clause, all
	 * such Vars must refer to subselect output columns.
	 */
	vars = pull_var_clause(qual,
						   PVC_REJECT_AGGREGATES,
						   PVC_INCLUDE_PLACEHOLDERS);
	foreach(vl, vars)
	{
		Var		   *var = (Var *) lfirst(vl);

		/*
		 * XXX Punt if we find any PlaceHolderVars in the restriction clause.
		 * It's not clear whether a PHV could safely be pushed down, and even
		 * less clear whether such a situation could arise in any cases of
		 * practical interest anyway.  So for the moment, just refuse to push
		 * down.
		 */
		if (!IsA(var, Var))
		{
			safe = false;
			break;
		}

		Assert(var->varno == rti);
		Assert(var->varattno >= 0);

		/* Check point 2 */
		if (var->varattno == 0)
		{
			safe = false;
			break;
		}

		/* Check point 3 */
		if (unsafeColumns[var->varattno])
		{
			safe = false;
			break;
		}
	}

	list_free(vars);

	return safe;
}

/*
 * subquery_push_qual - push down a qual that we have determined is safe
 */
static void
subquery_push_qual(Query *subquery, RangeTblEntry *rte, Index rti, Node *qual)
{
	if (subquery->setOperations != NULL)
	{
		/* Recurse to push it separately to each component query */
		recurse_push_qual(subquery->setOperations, subquery,
						  rte, rti, qual);
	}
	else
	{
		/*
		 * We need to replace Vars in the qual (which must refer to outputs of
		 * the subquery) with copies of the subquery's targetlist expressions.
		 * Note that at this point, any uplevel Vars in the qual should have
		 * been replaced with Params, so they need no work.
		 *
		 * This step also ensures that when we are pushing into a setop tree,
		 * each component query gets its own copy of the qual.
		 */
		qual = ResolveNew(qual, rti, 0, rte,
						  subquery->targetList,
						  CMD_SELECT, 0,
						  &subquery->hasSubLinks);

		/*
		 * Now attach the qual to the proper place: normally WHERE, but if the
		 * subquery uses grouping or aggregation, put it in HAVING (since the
		 * qual really refers to the group-result rows).
		 */
		if (subquery->hasAggs || subquery->groupClause || subquery->havingQual)
			subquery->havingQual = make_and_qual(subquery->havingQual, qual);
		else
			subquery->jointree->quals =
				make_and_qual(subquery->jointree->quals, qual);

		/*
		 * We need not change the subquery's hasAggs or hasSublinks flags,
		 * since we can't be pushing down any aggregates that weren't there
		 * before, and we don't push down subselects at all.
		 */
	}
}

/*
 * Helper routine to recurse through setOperations tree
 */
static void
recurse_push_qual(Node *setOp, Query *topquery,
				  RangeTblEntry *rte, Index rti, Node *qual)
{
	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		RangeTblEntry *subrte = rt_fetch(rtr->rtindex, topquery->rtable);
		Query	   *subquery = subrte->subquery;

		Assert(subquery != NULL);
		subquery_push_qual(subquery, rte, rti, qual);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		recurse_push_qual(op->larg, topquery, rte, rti, qual);
		recurse_push_qual(op->rarg, topquery, rte, rti, qual);
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
	}
}

/*****************************************************************************
 *			DEBUG SUPPORT
 *****************************************************************************/

#ifdef OPTIMIZER_DEBUG

static void
print_relids(Relids relids)
{
	Relids		tmprelids;
	int			x;
	bool		first = true;

	tmprelids = bms_copy(relids);
	while ((x = bms_first_member(tmprelids)) >= 0)
	{
		if (!first)
			printf(" ");
		printf("%d", x);
		first = false;
	}
	bms_free(tmprelids);
}

static void
print_restrictclauses(PlannerInfo *root, List *clauses)
{
	ListCell   *l;

	foreach(l, clauses)
	{
		RestrictInfo *c = lfirst(l);

		print_expr((Node *) c->clause, root->parse->rtable);
		if (lnext(l))
			printf(", ");
	}
}

static void
print_path(PlannerInfo *root, Path *path, int indent)
{
	const char *ptype;
	bool		join = false;
	Path	   *subpath = NULL;
	int			i;

	switch (nodeTag(path))
	{
		case T_Path:
			ptype = "SeqScan";
			break;
		case T_IndexPath:
			ptype = "IdxScan";
			break;
		case T_BitmapHeapPath:
			ptype = "BitmapHeapScan";
			break;
		case T_BitmapAndPath:
			ptype = "BitmapAndPath";
			break;
		case T_BitmapOrPath:
			ptype = "BitmapOrPath";
			break;
		case T_TidPath:
			ptype = "TidScan";
			break;
		case T_ForeignPath:
			ptype = "ForeignScan";
			break;
		case T_AppendPath:
			ptype = "Append";
			break;
		case T_MergeAppendPath:
			ptype = "MergeAppend";
			break;
		case T_ResultPath:
			ptype = "Result";
			break;
		case T_MaterialPath:
			ptype = "Material";
			subpath = ((MaterialPath *) path)->subpath;
			break;
		case T_UniquePath:
			ptype = "Unique";
			subpath = ((UniquePath *) path)->subpath;
			break;
		case T_NestPath:
			ptype = "NestLoop";
			join = true;
			break;
		case T_MergePath:
			ptype = "MergeJoin";
			join = true;
			break;
		case T_HashPath:
			ptype = "HashJoin";
			join = true;
			break;
		default:
			ptype = "???Path";
			break;
	}

	for (i = 0; i < indent; i++)
		printf("\t");
	printf("%s", ptype);

	if (path->parent)
	{
		printf("(");
		print_relids(path->parent->relids);
		printf(") rows=%.0f", path->parent->rows);
	}
	printf(" cost=%.2f..%.2f\n", path->startup_cost, path->total_cost);

	if (path->pathkeys)
	{
		for (i = 0; i < indent; i++)
			printf("\t");
		printf("  pathkeys: ");
		print_pathkeys(path->pathkeys, root->parse->rtable);
	}

	if (join)
	{
		JoinPath   *jp = (JoinPath *) path;

		for (i = 0; i < indent; i++)
			printf("\t");
		printf("  clauses: ");
		print_restrictclauses(root, jp->joinrestrictinfo);
		printf("\n");

		if (IsA(path, MergePath))
		{
			MergePath  *mp = (MergePath *) path;

			for (i = 0; i < indent; i++)
				printf("\t");
			printf("  sortouter=%d sortinner=%d materializeinner=%d\n",
				   ((mp->outersortkeys) ? 1 : 0),
				   ((mp->innersortkeys) ? 1 : 0),
				   ((mp->materialize_inner) ? 1 : 0));
		}

		print_path(root, jp->outerjoinpath, indent + 1);
		print_path(root, jp->innerjoinpath, indent + 1);
	}

	if (subpath)
		print_path(root, subpath, indent + 1);
}

void
debug_print_rel(PlannerInfo *root, RelOptInfo *rel)
{
	ListCell   *l;

	printf("RELOPTINFO (");
	print_relids(rel->relids);
	printf("): rows=%.0f width=%d\n", rel->rows, rel->width);

	if (rel->baserestrictinfo)
	{
		printf("\tbaserestrictinfo: ");
		print_restrictclauses(root, rel->baserestrictinfo);
		printf("\n");
	}

	if (rel->joininfo)
	{
		printf("\tjoininfo: ");
		print_restrictclauses(root, rel->joininfo);
		printf("\n");
	}

	printf("\tpath list:\n");
	foreach(l, rel->pathlist)
		print_path(root, lfirst(l), 1);
	printf("\n\tcheapest startup path:\n");
	print_path(root, rel->cheapest_startup_path, 1);
	printf("\n\tcheapest total path:\n");
	print_path(root, rel->cheapest_total_path, 1);
	printf("\n");
	fflush(stdout);
}

#endif   /* OPTIMIZER_DEBUG */
