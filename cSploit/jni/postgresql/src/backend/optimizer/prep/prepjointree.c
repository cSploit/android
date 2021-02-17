/*-------------------------------------------------------------------------
 *
 * prepjointree.c
 *	  Planner preprocessing for subqueries and join tree manipulation.
 *
 * NOTE: the intended sequence for invoking these operations is
 *		pull_up_sublinks
 *		inline_set_returning_functions
 *		pull_up_subqueries
 *		flatten_simple_union_all
 *		do expression preprocessing (including flattening JOIN alias vars)
 *		reduce_outer_joins
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/optimizer/prep/prepjointree.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_type.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/clauses.h"
#include "optimizer/placeholder.h"
#include "optimizer/prep.h"
#include "optimizer/subselect.h"
#include "optimizer/tlist.h"
#include "optimizer/var.h"
#include "parser/parse_relation.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteManip.h"


typedef struct pullup_replace_vars_context
{
	PlannerInfo *root;
	List	   *targetlist;		/* tlist of subquery being pulled up */
	RangeTblEntry *target_rte;	/* RTE of subquery */
	bool	   *outer_hasSubLinks;		/* -> outer query's hasSubLinks */
	int			varno;			/* varno of subquery */
	bool		need_phvs;		/* do we need PlaceHolderVars? */
	bool		wrap_non_vars;	/* do we need 'em on *all* non-Vars? */
	Node	  **rv_cache;		/* cache for results with PHVs */
} pullup_replace_vars_context;

typedef struct reduce_outer_joins_state
{
	Relids		relids;			/* base relids within this subtree */
	bool		contains_outer; /* does subtree contain outer join(s)? */
	List	   *sub_states;		/* List of states for subtree components */
} reduce_outer_joins_state;

static Node *pull_up_sublinks_jointree_recurse(PlannerInfo *root, Node *jtnode,
								  Relids *relids);
static Node *pull_up_sublinks_qual_recurse(PlannerInfo *root, Node *node,
							  Relids available_rels, Node **jtlink);
static Node *pull_up_simple_subquery(PlannerInfo *root, Node *jtnode,
						RangeTblEntry *rte,
						JoinExpr *lowest_outer_join,
						AppendRelInfo *containing_appendrel);
static Node *pull_up_simple_union_all(PlannerInfo *root, Node *jtnode,
						 RangeTblEntry *rte);
static void pull_up_union_leaf_queries(Node *setOp, PlannerInfo *root,
						   int parentRTindex, Query *setOpQuery,
						   int childRToffset);
static void make_setop_translation_list(Query *query, Index newvarno,
							List **translated_vars);
static bool is_simple_subquery(Query *subquery);
static bool is_simple_union_all(Query *subquery);
static bool is_simple_union_all_recurse(Node *setOp, Query *setOpQuery,
							List *colTypes);
static bool is_safe_append_member(Query *subquery);
static void replace_vars_in_jointree(Node *jtnode,
						 pullup_replace_vars_context *context,
						 JoinExpr *lowest_outer_join);
static Node *pullup_replace_vars(Node *expr,
					pullup_replace_vars_context *context);
static Node *pullup_replace_vars_callback(Var *var,
							 replace_rte_variables_context *context);
static reduce_outer_joins_state *reduce_outer_joins_pass1(Node *jtnode);
static void reduce_outer_joins_pass2(Node *jtnode,
						 reduce_outer_joins_state *state,
						 PlannerInfo *root,
						 Relids nonnullable_rels,
						 List *nonnullable_vars,
						 List *forced_null_vars);
static void substitute_multiple_relids(Node *node,
						   int varno, Relids subrelids);
static void fix_append_rel_relids(List *append_rel_list, int varno,
					  Relids subrelids);
static Node *find_jointree_node_for_rel(Node *jtnode, int relid);


/*
 * pull_up_sublinks
 *		Attempt to pull up ANY and EXISTS SubLinks to be treated as
 *		semijoins or anti-semijoins.
 *
 * A clause "foo op ANY (sub-SELECT)" can be processed by pulling the
 * sub-SELECT up to become a rangetable entry and treating the implied
 * comparisons as quals of a semijoin.	However, this optimization *only*
 * works at the top level of WHERE or a JOIN/ON clause, because we cannot
 * distinguish whether the ANY ought to return FALSE or NULL in cases
 * involving NULL inputs.  Also, in an outer join's ON clause we can only
 * do this if the sublink is degenerate (ie, references only the nullable
 * side of the join).  In that case it is legal to push the semijoin
 * down into the nullable side of the join.  If the sublink references any
 * nonnullable-side variables then it would have to be evaluated as part
 * of the outer join, which makes things way too complicated.
 *
 * Under similar conditions, EXISTS and NOT EXISTS clauses can be handled
 * by pulling up the sub-SELECT and creating a semijoin or anti-semijoin.
 *
 * This routine searches for such clauses and does the necessary parsetree
 * transformations if any are found.
 *
 * This routine has to run before preprocess_expression(), so the quals
 * clauses are not yet reduced to implicit-AND format.	That means we need
 * to recursively search through explicit AND clauses, which are
 * probably only binary ANDs.  We stop as soon as we hit a non-AND item.
 */
void
pull_up_sublinks(PlannerInfo *root)
{
	Node	   *jtnode;
	Relids		relids;

	/* Begin recursion through the jointree */
	jtnode = pull_up_sublinks_jointree_recurse(root,
											   (Node *) root->parse->jointree,
											   &relids);

	/*
	 * root->parse->jointree must always be a FromExpr, so insert a dummy one
	 * if we got a bare RangeTblRef or JoinExpr out of the recursion.
	 */
	if (IsA(jtnode, FromExpr))
		root->parse->jointree = (FromExpr *) jtnode;
	else
		root->parse->jointree = makeFromExpr(list_make1(jtnode), NULL);
}

/*
 * Recurse through jointree nodes for pull_up_sublinks()
 *
 * In addition to returning the possibly-modified jointree node, we return
 * a relids set of the contained rels into *relids.
 */
static Node *
pull_up_sublinks_jointree_recurse(PlannerInfo *root, Node *jtnode,
								  Relids *relids)
{
	if (jtnode == NULL)
	{
		*relids = NULL;
	}
	else if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		*relids = bms_make_singleton(varno);
		/* jtnode is returned unmodified */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		List	   *newfromlist = NIL;
		Relids		frelids = NULL;
		FromExpr   *newf;
		Node	   *jtlink;
		ListCell   *l;

		/* First, recurse to process children and collect their relids */
		foreach(l, f->fromlist)
		{
			Node	   *newchild;
			Relids		childrelids;

			newchild = pull_up_sublinks_jointree_recurse(root,
														 lfirst(l),
														 &childrelids);
			newfromlist = lappend(newfromlist, newchild);
			frelids = bms_join(frelids, childrelids);
		}
		/* Build the replacement FromExpr; no quals yet */
		newf = makeFromExpr(newfromlist, NULL);
		/* Set up a link representing the rebuilt jointree */
		jtlink = (Node *) newf;
		/* Now process qual --- all children are available for use */
		newf->quals = pull_up_sublinks_qual_recurse(root, f->quals, frelids,
													&jtlink);

		/*
		 * Note that the result will be either newf, or a stack of JoinExprs
		 * with newf at the base.  We rely on subsequent optimization steps to
		 * flatten this and rearrange the joins as needed.
		 *
		 * Although we could include the pulled-up subqueries in the returned
		 * relids, there's no need since upper quals couldn't refer to their
		 * outputs anyway.
		 */
		*relids = frelids;
		jtnode = jtlink;
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j;
		Relids		leftrelids;
		Relids		rightrelids;
		Node	   *jtlink;

		/*
		 * Make a modifiable copy of join node, but don't bother copying its
		 * subnodes (yet).
		 */
		j = (JoinExpr *) palloc(sizeof(JoinExpr));
		memcpy(j, jtnode, sizeof(JoinExpr));
		jtlink = (Node *) j;

		/* Recurse to process children and collect their relids */
		j->larg = pull_up_sublinks_jointree_recurse(root, j->larg,
													&leftrelids);
		j->rarg = pull_up_sublinks_jointree_recurse(root, j->rarg,
													&rightrelids);

		/*
		 * Now process qual, showing appropriate child relids as available,
		 * and attach any pulled-up jointree items at the right place. In the
		 * inner-join case we put new JoinExprs above the existing one (much
		 * as for a FromExpr-style join).  In outer-join cases the new
		 * JoinExprs must go into the nullable side of the outer join. The
		 * point of the available_rels machinations is to ensure that we only
		 * pull up quals for which that's okay.
		 *
		 * XXX for the moment, we refrain from pulling up IN/EXISTS clauses
		 * appearing in LEFT or RIGHT join conditions.	Although it is
		 * semantically valid to do so under the above conditions, we end up
		 * with a query in which the semijoin or antijoin must be evaluated
		 * below the outer join, which could perform far worse than leaving it
		 * as a sublink that is executed only for row pairs that meet the
		 * other join conditions.  Fixing this seems to require considerable
		 * restructuring of the executor, but maybe someday it can happen.
		 * (See also the comparable case in pull_up_sublinks_qual_recurse.)
		 *
		 * We don't expect to see any pre-existing JOIN_SEMI or JOIN_ANTI
		 * nodes here.
		 */
		switch (j->jointype)
		{
			case JOIN_INNER:
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 bms_union(leftrelids,
																rightrelids),
														 &jtlink);
				break;
			case JOIN_LEFT:
#ifdef NOT_USED					/* see XXX comment above */
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 rightrelids,
														 &j->rarg);
#endif
				break;
			case JOIN_FULL:
				/* can't do anything with full-join quals */
				break;
			case JOIN_RIGHT:
#ifdef NOT_USED					/* see XXX comment above */
				j->quals = pull_up_sublinks_qual_recurse(root, j->quals,
														 leftrelids,
														 &j->larg);
#endif
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) j->jointype);
				break;
		}

		/*
		 * Although we could include the pulled-up subqueries in the returned
		 * relids, there's no need since upper quals couldn't refer to their
		 * outputs anyway.	But we *do* need to include the join's own rtindex
		 * because we haven't yet collapsed join alias variables, so upper
		 * levels would mistakenly think they couldn't use references to this
		 * join.
		 */
		*relids = bms_join(leftrelids, rightrelids);
		if (j->rtindex)
			*relids = bms_add_member(*relids, j->rtindex);
		jtnode = jtlink;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return jtnode;
}

/*
 * Recurse through top-level qual nodes for pull_up_sublinks()
 *
 * jtlink points to the link in the jointree where any new JoinExprs should be
 * inserted.  If we find multiple pull-up-able SubLinks, they'll get stacked
 * there in the order we encounter them.  We rely on subsequent optimization
 * to rearrange the stack if appropriate.
 */
static Node *
pull_up_sublinks_qual_recurse(PlannerInfo *root, Node *node,
							  Relids available_rels, Node **jtlink)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, SubLink))
	{
		SubLink    *sublink = (SubLink *) node;
		JoinExpr   *j;
		Relids		child_rels;

		/* Is it a convertible ANY or EXISTS clause? */
		if (sublink->subLinkType == ANY_SUBLINK)
		{
			j = convert_ANY_sublink_to_join(root, sublink,
											available_rels);
			if (j)
			{
				/* Yes; recursively process what we pulled up */
				j->rarg = pull_up_sublinks_jointree_recurse(root,
															j->rarg,
															&child_rels);
				/* Any inserted joins get stacked onto j->rarg */
				j->quals = pull_up_sublinks_qual_recurse(root,
														 j->quals,
														 child_rels,
														 &j->rarg);
				/* Now insert the new join node into the join tree */
				j->larg = *jtlink;
				*jtlink = (Node *) j;
				/* and return NULL representing constant TRUE */
				return NULL;
			}
		}
		else if (sublink->subLinkType == EXISTS_SUBLINK)
		{
			j = convert_EXISTS_sublink_to_join(root, sublink, false,
											   available_rels);
			if (j)
			{
				/* Yes; recursively process what we pulled up */
				j->rarg = pull_up_sublinks_jointree_recurse(root,
															j->rarg,
															&child_rels);
				/* Any inserted joins get stacked onto j->rarg */
				j->quals = pull_up_sublinks_qual_recurse(root,
														 j->quals,
														 child_rels,
														 &j->rarg);
				/* Now insert the new join node into the join tree */
				j->larg = *jtlink;
				*jtlink = (Node *) j;
				/* and return NULL representing constant TRUE */
				return NULL;
			}
		}
		/* Else return it unmodified */
		return node;
	}
	if (not_clause(node))
	{
		/* If the immediate argument of NOT is EXISTS, try to convert */
		SubLink    *sublink = (SubLink *) get_notclausearg((Expr *) node);
		JoinExpr   *j;

		if (sublink && IsA(sublink, SubLink))
		{
			if (sublink->subLinkType == EXISTS_SUBLINK)
			{
				j = convert_EXISTS_sublink_to_join(root, sublink, true,
												   available_rels);
				if (j)
				{
					/*
					 * For the moment, refrain from recursing underneath NOT.
					 * As in pull_up_sublinks_jointree_recurse, recursing here
					 * would result in inserting a join underneath an ANTI
					 * join with which it could not commute, and that could
					 * easily lead to a worse plan than what we've
					 * historically generated.
					 */
#ifdef NOT_USED
					/* Yes; recursively process what we pulled up */
					Relids		child_rels;

					j->rarg = pull_up_sublinks_jointree_recurse(root,
																j->rarg,
																&child_rels);
					/* Any inserted joins get stacked onto j->rarg */
					j->quals = pull_up_sublinks_qual_recurse(root,
															 j->quals,
															 child_rels,
															 &j->rarg);
#endif
					/* Now insert the new join node into the join tree */
					j->larg = *jtlink;
					*jtlink = (Node *) j;
					/* and return NULL representing constant TRUE */
					return NULL;
				}
			}
		}
		/* Else return it unmodified */
		return node;
	}
	if (and_clause(node))
	{
		/* Recurse into AND clause */
		List	   *newclauses = NIL;
		ListCell   *l;

		foreach(l, ((BoolExpr *) node)->args)
		{
			Node	   *oldclause = (Node *) lfirst(l);
			Node	   *newclause;

			newclause = pull_up_sublinks_qual_recurse(root,
													  oldclause,
													  available_rels,
													  jtlink);
			if (newclause)
				newclauses = lappend(newclauses, newclause);
		}
		/* We might have got back fewer clauses than we started with */
		if (newclauses == NIL)
			return NULL;
		else if (list_length(newclauses) == 1)
			return (Node *) linitial(newclauses);
		else
			return (Node *) make_andclause(newclauses);
	}
	/* Stop if not an AND */
	return node;
}

/*
 * inline_set_returning_functions
 *		Attempt to "inline" set-returning functions in the FROM clause.
 *
 * If an RTE_FUNCTION rtable entry invokes a set-returning function that
 * contains just a simple SELECT, we can convert the rtable entry to an
 * RTE_SUBQUERY entry exposing the SELECT directly.  This is especially
 * useful if the subquery can then be "pulled up" for further optimization,
 * but we do it even if not, to reduce executor overhead.
 *
 * This has to be done before we have started to do any optimization of
 * subqueries, else any such steps wouldn't get applied to subqueries
 * obtained via inlining.  However, we do it after pull_up_sublinks
 * so that we can inline any functions used in SubLink subselects.
 *
 * Like most of the planner, this feels free to scribble on its input data
 * structure.
 */
void
inline_set_returning_functions(PlannerInfo *root)
{
	ListCell   *rt;

	foreach(rt, root->parse->rtable)
	{
		RangeTblEntry *rte = (RangeTblEntry *) lfirst(rt);

		if (rte->rtekind == RTE_FUNCTION)
		{
			Query	   *funcquery;

			/* Check safety of expansion, and expand if possible */
			funcquery = inline_set_returning_function(root, rte);
			if (funcquery)
			{
				/* Successful expansion, replace the rtable entry */
				rte->rtekind = RTE_SUBQUERY;
				rte->subquery = funcquery;
				rte->funcexpr = NULL;
				rte->funccoltypes = NIL;
				rte->funccoltypmods = NIL;
				rte->funccolcollations = NIL;
			}
		}
	}
}

/*
 * pull_up_subqueries
 *		Look for subqueries in the rangetable that can be pulled up into
 *		the parent query.  If the subquery has no special features like
 *		grouping/aggregation then we can merge it into the parent's jointree.
 *		Also, subqueries that are simple UNION ALL structures can be
 *		converted into "append relations".
 *
 * If this jointree node is within the nullable side of an outer join, then
 * lowest_outer_join references the lowest such JoinExpr node; otherwise it
 * is NULL.  This forces use of the PlaceHolderVar mechanism for references
 * to non-nullable targetlist items, but only for references above that join.
 *
 * If we are looking at a member subquery of an append relation,
 * containing_appendrel describes that relation; else it is NULL.
 * This forces use of the PlaceHolderVar mechanism for all non-Var targetlist
 * items, and puts some additional restrictions on what can be pulled up.
 *
 * A tricky aspect of this code is that if we pull up a subquery we have
 * to replace Vars that reference the subquery's outputs throughout the
 * parent query, including quals attached to jointree nodes above the one
 * we are currently processing!  We handle this by being careful not to
 * change the jointree structure while recursing: no nodes other than
 * subquery RangeTblRef entries will be replaced.  Also, we can't turn
 * pullup_replace_vars loose on the whole jointree, because it'll return a
 * mutated copy of the tree; we have to invoke it just on the quals, instead.
 * This behavior is what makes it reasonable to pass lowest_outer_join as a
 * pointer rather than some more-indirect way of identifying the lowest OJ.
 * Likewise, we don't replace append_rel_list members but only their
 * substructure, so the containing_appendrel reference is safe to use.
 */
Node *
pull_up_subqueries(PlannerInfo *root, Node *jtnode,
				   JoinExpr *lowest_outer_join,
				   AppendRelInfo *containing_appendrel)
{
	if (jtnode == NULL)
		return NULL;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;
		RangeTblEntry *rte = rt_fetch(varno, root->parse->rtable);

		/*
		 * Is this a subquery RTE, and if so, is the subquery simple enough to
		 * pull up?
		 *
		 * If we are looking at an append-relation member, we can't pull it up
		 * unless is_safe_append_member says so.
		 */
		if (rte->rtekind == RTE_SUBQUERY &&
			is_simple_subquery(rte->subquery) &&
			(containing_appendrel == NULL ||
			 is_safe_append_member(rte->subquery)))
			return pull_up_simple_subquery(root, jtnode, rte,
										   lowest_outer_join,
										   containing_appendrel);

		/*
		 * Alternatively, is it a simple UNION ALL subquery?  If so, flatten
		 * into an "append relation".
		 *
		 * It's safe to do this regardless of whether this query is itself an
		 * appendrel member.  (If you're thinking we should try to flatten the
		 * two levels of appendrel together, you're right; but we handle that
		 * in set_append_rel_pathlist, not here.)
		 */
		if (rte->rtekind == RTE_SUBQUERY &&
			is_simple_union_all(rte->subquery))
			return pull_up_simple_union_all(root, jtnode, rte);

		/* Otherwise, do nothing at this node. */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		Assert(containing_appendrel == NULL);
		foreach(l, f->fromlist)
			lfirst(l) = pull_up_subqueries(root, lfirst(l),
										   lowest_outer_join, NULL);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		Assert(containing_appendrel == NULL);
		/* Recurse, being careful to tell myself when inside outer join */
		switch (j->jointype)
		{
			case JOIN_INNER:
				j->larg = pull_up_subqueries(root, j->larg,
											 lowest_outer_join, NULL);
				j->rarg = pull_up_subqueries(root, j->rarg,
											 lowest_outer_join, NULL);
				break;
			case JOIN_LEFT:
			case JOIN_SEMI:
			case JOIN_ANTI:
				j->larg = pull_up_subqueries(root, j->larg,
											 lowest_outer_join, NULL);
				j->rarg = pull_up_subqueries(root, j->rarg,
											 j, NULL);
				break;
			case JOIN_FULL:
				j->larg = pull_up_subqueries(root, j->larg,
											 j, NULL);
				j->rarg = pull_up_subqueries(root, j->rarg,
											 j, NULL);
				break;
			case JOIN_RIGHT:
				j->larg = pull_up_subqueries(root, j->larg,
											 j, NULL);
				j->rarg = pull_up_subqueries(root, j->rarg,
											 lowest_outer_join, NULL);
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) j->jointype);
				break;
		}
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return jtnode;
}

/*
 * pull_up_simple_subquery
 *		Attempt to pull up a single simple subquery.
 *
 * jtnode is a RangeTblRef that has been tentatively identified as a simple
 * subquery by pull_up_subqueries.	We return the replacement jointree node,
 * or jtnode itself if we determine that the subquery can't be pulled up after
 * all.
 *
 * rte is the RangeTblEntry referenced by jtnode.  Remaining parameters are
 * as for pull_up_subqueries.
 */
static Node *
pull_up_simple_subquery(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte,
						JoinExpr *lowest_outer_join,
						AppendRelInfo *containing_appendrel)
{
	Query	   *parse = root->parse;
	int			varno = ((RangeTblRef *) jtnode)->rtindex;
	Query	   *subquery;
	PlannerInfo *subroot;
	int			rtoffset;
	pullup_replace_vars_context rvcontext;
	ListCell   *lc;

	/*
	 * Need a modifiable copy of the subquery to hack on.  Even if we didn't
	 * sometimes choose not to pull up below, we must do this to avoid
	 * problems if the same subquery is referenced from multiple jointree
	 * items (which can't happen normally, but might after rule rewriting).
	 */
	subquery = copyObject(rte->subquery);

	/*
	 * Create a PlannerInfo data structure for this subquery.
	 *
	 * NOTE: the next few steps should match the first processing in
	 * subquery_planner().	Can we refactor to avoid code duplication, or
	 * would that just make things uglier?
	 */
	subroot = makeNode(PlannerInfo);
	subroot->parse = subquery;
	subroot->glob = root->glob;
	subroot->query_level = root->query_level;
	subroot->parent_root = root->parent_root;
	subroot->plan_params = NIL;
	subroot->planner_cxt = CurrentMemoryContext;
	subroot->init_plans = NIL;
	subroot->cte_plan_ids = NIL;
	subroot->eq_classes = NIL;
	subroot->append_rel_list = NIL;
	subroot->rowMarks = NIL;
	subroot->hasRecursion = false;
	subroot->wt_param_id = -1;
	subroot->non_recursive_plan = NULL;

	/* No CTEs to worry about */
	Assert(subquery->cteList == NIL);

	/*
	 * Pull up any SubLinks within the subquery's quals, so that we don't
	 * leave unoptimized SubLinks behind.
	 */
	if (subquery->hasSubLinks)
		pull_up_sublinks(subroot);

	/*
	 * Similarly, inline any set-returning functions in its rangetable.
	 */
	inline_set_returning_functions(subroot);

	/*
	 * Recursively pull up the subquery's subqueries, so that
	 * pull_up_subqueries' processing is complete for its jointree and
	 * rangetable.
	 *
	 * Note: we should pass NULL for containing-join info even if we are
	 * within an outer join in the upper query; the lower query starts with a
	 * clean slate for outer-join semantics.  Likewise, we say we aren't
	 * handling an appendrel member.
	 */
	subquery->jointree = (FromExpr *)
		pull_up_subqueries(subroot, (Node *) subquery->jointree, NULL, NULL);

	/*
	 * Now we must recheck whether the subquery is still simple enough to pull
	 * up.	If not, abandon processing it.
	 *
	 * We don't really need to recheck all the conditions involved, but it's
	 * easier just to keep this "if" looking the same as the one in
	 * pull_up_subqueries.
	 */
	if (is_simple_subquery(subquery) &&
		(containing_appendrel == NULL || is_safe_append_member(subquery)))
	{
		/* good to go */
	}
	else
	{
		/*
		 * Give up, return unmodified RangeTblRef.
		 *
		 * Note: The work we just did will be redone when the subquery gets
		 * planned on its own.	Perhaps we could avoid that by storing the
		 * modified subquery back into the rangetable, but I'm not gonna risk
		 * it now.
		 */
		return jtnode;
	}

	/*
	 * Adjust level-0 varnos in subquery so that we can append its rangetable
	 * to upper query's.  We have to fix the subquery's append_rel_list as
	 * well.
	 */
	rtoffset = list_length(parse->rtable);
	OffsetVarNodes((Node *) subquery, rtoffset, 0);
	OffsetVarNodes((Node *) subroot->append_rel_list, rtoffset, 0);

	/*
	 * Upper-level vars in subquery are now one level closer to their parent
	 * than before.
	 */
	IncrementVarSublevelsUp((Node *) subquery, -1, 1);
	IncrementVarSublevelsUp((Node *) subroot->append_rel_list, -1, 1);

	/*
	 * The subquery's targetlist items are now in the appropriate form to
	 * insert into the top query, but if we are under an outer join then
	 * non-nullable items may have to be turned into PlaceHolderVars.  If we
	 * are dealing with an appendrel member then anything that's not a simple
	 * Var has to be turned into a PlaceHolderVar.	Set up appropriate context
	 * data for pullup_replace_vars.
	 */
	rvcontext.root = root;
	rvcontext.targetlist = subquery->targetList;
	rvcontext.target_rte = rte;
	rvcontext.outer_hasSubLinks = &parse->hasSubLinks;
	rvcontext.varno = varno;
	rvcontext.need_phvs = (lowest_outer_join != NULL ||
						   containing_appendrel != NULL);
	rvcontext.wrap_non_vars = (containing_appendrel != NULL);
	/* initialize cache array with indexes 0 .. length(tlist) */
	rvcontext.rv_cache = palloc0((list_length(subquery->targetList) + 1) *
								 sizeof(Node *));

	/*
	 * Replace all of the top query's references to the subquery's outputs
	 * with copies of the adjusted subtlist items, being careful not to
	 * replace any of the jointree structure. (This'd be a lot cleaner if we
	 * could use query_tree_mutator.)  We have to use PHVs in the targetList,
	 * returningList, and havingQual, since those are certainly above any
	 * outer join.	replace_vars_in_jointree tracks its location in the
	 * jointree and uses PHVs or not appropriately.
	 */
	parse->targetList = (List *)
		pullup_replace_vars((Node *) parse->targetList, &rvcontext);
	parse->returningList = (List *)
		pullup_replace_vars((Node *) parse->returningList, &rvcontext);
	replace_vars_in_jointree((Node *) parse->jointree, &rvcontext,
							 lowest_outer_join);
	Assert(parse->setOperations == NULL);
	parse->havingQual = pullup_replace_vars(parse->havingQual, &rvcontext);

	/*
	 * Replace references in the translated_vars lists of appendrels. When
	 * pulling up an appendrel member, we do not need PHVs in the list of the
	 * parent appendrel --- there isn't any outer join between. Elsewhere, use
	 * PHVs for safety.  (This analysis could be made tighter but it seems
	 * unlikely to be worth much trouble.)
	 */
	foreach(lc, root->append_rel_list)
	{
		AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(lc);
		bool		save_need_phvs = rvcontext.need_phvs;

		if (appinfo == containing_appendrel)
			rvcontext.need_phvs = false;
		appinfo->translated_vars = (List *)
			pullup_replace_vars((Node *) appinfo->translated_vars, &rvcontext);
		rvcontext.need_phvs = save_need_phvs;
	}

	/*
	 * Replace references in the joinaliasvars lists of join RTEs.
	 *
	 * You might think that we could avoid using PHVs for alias vars of joins
	 * below lowest_outer_join, but that doesn't work because the alias vars
	 * could be referenced above that join; we need the PHVs to be present in
	 * such references after the alias vars get flattened.	(It might be worth
	 * trying to be smarter here, someday.)
	 */
	foreach(lc, parse->rtable)
	{
		RangeTblEntry *otherrte = (RangeTblEntry *) lfirst(lc);

		if (otherrte->rtekind == RTE_JOIN)
			otherrte->joinaliasvars = (List *)
				pullup_replace_vars((Node *) otherrte->joinaliasvars,
									&rvcontext);
	}

	/*
	 * Now append the adjusted rtable entries to upper query. (We hold off
	 * until after fixing the upper rtable entries; no point in running that
	 * code on the subquery ones too.)
	 */
	parse->rtable = list_concat(parse->rtable, subquery->rtable);

	/*
	 * Pull up any FOR UPDATE/SHARE markers, too.  (OffsetVarNodes already
	 * adjusted the marker rtindexes, so just concat the lists.)
	 */
	parse->rowMarks = list_concat(parse->rowMarks, subquery->rowMarks);

	/*
	 * We also have to fix the relid sets of any PlaceHolderVar nodes in the
	 * parent query.  (This could perhaps be done by pullup_replace_vars(),
	 * but it seems cleaner to use two passes.)  Note in particular that any
	 * PlaceHolderVar nodes just created by pullup_replace_vars() will be
	 * adjusted, so having created them with the subquery's varno is correct.
	 *
	 * Likewise, relids appearing in AppendRelInfo nodes have to be fixed. We
	 * already checked that this won't require introducing multiple subrelids
	 * into the single-slot AppendRelInfo structs.
	 */
	if (parse->hasSubLinks || root->glob->lastPHId != 0 ||
		root->append_rel_list)
	{
		Relids		subrelids;

		subrelids = get_relids_in_jointree((Node *) subquery->jointree, false);
		substitute_multiple_relids((Node *) parse, varno, subrelids);
		fix_append_rel_relids(root->append_rel_list, varno, subrelids);
	}

	/*
	 * And now add subquery's AppendRelInfos to our list.
	 */
	root->append_rel_list = list_concat(root->append_rel_list,
										subroot->append_rel_list);

	/*
	 * We don't have to do the equivalent bookkeeping for outer-join info,
	 * because that hasn't been set up yet.  placeholder_list likewise.
	 */
	Assert(root->join_info_list == NIL);
	Assert(subroot->join_info_list == NIL);
	Assert(root->placeholder_list == NIL);
	Assert(subroot->placeholder_list == NIL);

	/*
	 * Miscellaneous housekeeping.
	 *
	 * Although replace_rte_variables() faithfully updated parse->hasSubLinks
	 * if it copied any SubLinks out of the subquery's targetlist, we still
	 * could have SubLinks added to the query in the expressions of FUNCTION
	 * and VALUES RTEs copied up from the subquery.  So it's necessary to copy
	 * subquery->hasSubLinks anyway.  Perhaps this can be improved someday.
	 */
	parse->hasSubLinks |= subquery->hasSubLinks;

	/*
	 * subquery won't be pulled up if it hasAggs or hasWindowFuncs, so no work
	 * needed on those flags
	 */

	/*
	 * Return the adjusted subquery jointree to replace the RangeTblRef entry
	 * in parent's jointree.
	 */
	return (Node *) subquery->jointree;
}

/*
 * pull_up_simple_union_all
 *		Pull up a single simple UNION ALL subquery.
 *
 * jtnode is a RangeTblRef that has been identified as a simple UNION ALL
 * subquery by pull_up_subqueries.	We pull up the leaf subqueries and
 * build an "append relation" for the union set.  The result value is just
 * jtnode, since we don't actually need to change the query jointree.
 */
static Node *
pull_up_simple_union_all(PlannerInfo *root, Node *jtnode, RangeTblEntry *rte)
{
	int			varno = ((RangeTblRef *) jtnode)->rtindex;
	Query	   *subquery = rte->subquery;
	int			rtoffset;
	List	   *rtable;

	/*
	 * Append child RTEs to parent rtable.
	 *
	 * Upper-level vars in subquery are now one level closer to their parent
	 * than before.  We don't have to worry about offsetting varnos, though,
	 * because any such vars must refer to stuff above the level of the query
	 * we are pulling into.
	 */
	rtoffset = list_length(root->parse->rtable);
	rtable = copyObject(subquery->rtable);
	IncrementVarSublevelsUp_rtable(rtable, -1, 1);
	root->parse->rtable = list_concat(root->parse->rtable, rtable);

	/*
	 * Recursively scan the subquery's setOperations tree and add
	 * AppendRelInfo nodes for leaf subqueries to the parent's
	 * append_rel_list.  Also apply pull_up_subqueries to the leaf subqueries.
	 */
	Assert(subquery->setOperations);
	pull_up_union_leaf_queries(subquery->setOperations, root, varno, subquery,
							   rtoffset);

	/*
	 * Mark the parent as an append relation.
	 */
	rte->inh = true;

	return jtnode;
}

/*
 * pull_up_union_leaf_queries -- recursive guts of pull_up_simple_union_all
 *
 * Build an AppendRelInfo for each leaf query in the setop tree, and then
 * apply pull_up_subqueries to the leaf query.
 *
 * Note that setOpQuery is the Query containing the setOp node, whose tlist
 * contains references to all the setop output columns.  When called from
 * pull_up_simple_union_all, this is *not* the same as root->parse, which is
 * the parent Query we are pulling up into.
 *
 * parentRTindex is the appendrel parent's index in root->parse->rtable.
 *
 * The child RTEs have already been copied to the parent.  childRToffset
 * tells us where in the parent's range table they were copied.  When called
 * from flatten_simple_union_all, childRToffset is 0 since the child RTEs
 * were already in root->parse->rtable and no RT index adjustment is needed.
 */
static void
pull_up_union_leaf_queries(Node *setOp, PlannerInfo *root, int parentRTindex,
						   Query *setOpQuery, int childRToffset)
{
	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		int			childRTindex;
		AppendRelInfo *appinfo;

		/*
		 * Calculate the index in the parent's range table
		 */
		childRTindex = childRToffset + rtr->rtindex;

		/*
		 * Build a suitable AppendRelInfo, and attach to parent's list.
		 */
		appinfo = makeNode(AppendRelInfo);
		appinfo->parent_relid = parentRTindex;
		appinfo->child_relid = childRTindex;
		appinfo->parent_reltype = InvalidOid;
		appinfo->child_reltype = InvalidOid;
		make_setop_translation_list(setOpQuery, childRTindex,
									&appinfo->translated_vars);
		appinfo->parent_reloid = InvalidOid;
		root->append_rel_list = lappend(root->append_rel_list, appinfo);

		/*
		 * Recursively apply pull_up_subqueries to the new child RTE.  (We
		 * must build the AppendRelInfo first, because this will modify it.)
		 * Note that we can pass NULL for containing-join info even if we're
		 * actually under an outer join, because the child's expressions
		 * aren't going to propagate up above the join.
		 */
		rtr = makeNode(RangeTblRef);
		rtr->rtindex = childRTindex;
		(void) pull_up_subqueries(root, (Node *) rtr, NULL, appinfo);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		/* Recurse to reach leaf queries */
		pull_up_union_leaf_queries(op->larg, root, parentRTindex, setOpQuery,
								   childRToffset);
		pull_up_union_leaf_queries(op->rarg, root, parentRTindex, setOpQuery,
								   childRToffset);
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
	}
}

/*
 * make_setop_translation_list
 *	  Build the list of translations from parent Vars to child Vars for
 *	  a UNION ALL member.  (At this point it's just a simple list of
 *	  referencing Vars, but if we succeed in pulling up the member
 *	  subquery, the Vars will get replaced by pulled-up expressions.)
 */
static void
make_setop_translation_list(Query *query, Index newvarno,
							List **translated_vars)
{
	List	   *vars = NIL;
	ListCell   *l;

	foreach(l, query->targetList)
	{
		TargetEntry *tle = (TargetEntry *) lfirst(l);

		if (tle->resjunk)
			continue;

		vars = lappend(vars, makeVarFromTargetEntry(newvarno, tle));
	}

	*translated_vars = vars;
}

/*
 * is_simple_subquery
 *	  Check a subquery in the range table to see if it's simple enough
 *	  to pull up into the parent query.
 */
static bool
is_simple_subquery(Query *subquery)
{
	/*
	 * Let's just make sure it's a valid subselect ...
	 */
	if (!IsA(subquery, Query) ||
		subquery->commandType != CMD_SELECT ||
		subquery->utilityStmt != NULL ||
		subquery->intoClause != NULL)
		elog(ERROR, "subquery is bogus");

	/*
	 * Can't currently pull up a query with setops (unless it's simple UNION
	 * ALL, which is handled by a different code path). Maybe after querytree
	 * redesign...
	 */
	if (subquery->setOperations)
		return false;

	/*
	 * Can't pull up a subquery involving grouping, aggregation, sorting,
	 * limiting, or WITH.  (XXX WITH could possibly be allowed later)
	 *
	 * We also don't pull up a subquery that has explicit FOR UPDATE/SHARE
	 * clauses, because pullup would cause the locking to occur semantically
	 * higher than it should.  Implicit FOR UPDATE/SHARE is okay because in
	 * that case the locking was originally declared in the upper query
	 * anyway.
	 */
	if (subquery->hasAggs ||
		subquery->hasWindowFuncs ||
		subquery->groupClause ||
		subquery->havingQual ||
		subquery->sortClause ||
		subquery->distinctClause ||
		subquery->limitOffset ||
		subquery->limitCount ||
		subquery->hasForUpdate ||
		subquery->cteList)
		return false;

	/*
	 * Don't pull up a subquery that has any set-returning functions in its
	 * targetlist.	Otherwise we might well wind up inserting set-returning
	 * functions into places where they mustn't go, such as quals of higher
	 * queries.
	 */
	if (expression_returns_set((Node *) subquery->targetList))
		return false;

	/*
	 * Don't pull up a subquery that has any volatile functions in its
	 * targetlist.	Otherwise we might introduce multiple evaluations of these
	 * functions, if they get copied to multiple places in the upper query,
	 * leading to surprising results.  (Note: the PlaceHolderVar mechanism
	 * doesn't quite guarantee single evaluation; else we could pull up anyway
	 * and just wrap such items in PlaceHolderVars ...)
	 */
	if (contain_volatile_functions((Node *) subquery->targetList))
		return false;

	/*
	 * Hack: don't try to pull up a subquery with an empty jointree.
	 * query_planner() will correctly generate a Result plan for a jointree
	 * that's totally empty, but I don't think the right things happen if an
	 * empty FromExpr appears lower down in a jointree.  It would pose a
	 * problem for the PlaceHolderVar mechanism too, since we'd have no way to
	 * identify where to evaluate a PHV coming out of the subquery. Not worth
	 * working hard on this, just to collapse SubqueryScan/Result into Result;
	 * especially since the SubqueryScan can often be optimized away by
	 * setrefs.c anyway.
	 */
	if (subquery->jointree->fromlist == NIL)
		return false;

	return true;
}

/*
 * is_simple_union_all
 *	  Check a subquery to see if it's a simple UNION ALL.
 *
 * We require all the setops to be UNION ALL (no mixing) and there can't be
 * any datatype coercions involved, ie, all the leaf queries must emit the
 * same datatypes.
 */
static bool
is_simple_union_all(Query *subquery)
{
	SetOperationStmt *topop;

	/* Let's just make sure it's a valid subselect ... */
	if (!IsA(subquery, Query) ||
		subquery->commandType != CMD_SELECT ||
		subquery->utilityStmt != NULL ||
		subquery->intoClause != NULL)
		elog(ERROR, "subquery is bogus");

	/* Is it a set-operation query at all? */
	topop = (SetOperationStmt *) subquery->setOperations;
	if (!topop)
		return false;
	Assert(IsA(topop, SetOperationStmt));

	/* Can't handle ORDER BY, LIMIT/OFFSET, locking, or WITH */
	if (subquery->sortClause ||
		subquery->limitOffset ||
		subquery->limitCount ||
		subquery->rowMarks ||
		subquery->cteList)
		return false;

	/* Recursively check the tree of set operations */
	return is_simple_union_all_recurse((Node *) topop, subquery,
									   topop->colTypes);
}

static bool
is_simple_union_all_recurse(Node *setOp, Query *setOpQuery, List *colTypes)
{
	if (IsA(setOp, RangeTblRef))
	{
		RangeTblRef *rtr = (RangeTblRef *) setOp;
		RangeTblEntry *rte = rt_fetch(rtr->rtindex, setOpQuery->rtable);
		Query	   *subquery = rte->subquery;

		Assert(subquery != NULL);

		/* Leaf nodes are OK if they match the toplevel column types */
		/* We don't have to compare typmods or collations here */
		return tlist_same_datatypes(subquery->targetList, colTypes, true);
	}
	else if (IsA(setOp, SetOperationStmt))
	{
		SetOperationStmt *op = (SetOperationStmt *) setOp;

		/* Must be UNION ALL */
		if (op->op != SETOP_UNION || !op->all)
			return false;

		/* Recurse to check inputs */
		return is_simple_union_all_recurse(op->larg, setOpQuery, colTypes) &&
			is_simple_union_all_recurse(op->rarg, setOpQuery, colTypes);
	}
	else
	{
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(setOp));
		return false;			/* keep compiler quiet */
	}
}

/*
 * is_safe_append_member
 *	  Check a subquery that is a leaf of a UNION ALL appendrel to see if it's
 *	  safe to pull up.
 */
static bool
is_safe_append_member(Query *subquery)
{
	FromExpr   *jtnode;

	/*
	 * It's only safe to pull up the child if its jointree contains exactly
	 * one RTE, else the AppendRelInfo data structure breaks. The one base RTE
	 * could be buried in several levels of FromExpr, however.
	 *
	 * Also, the child can't have any WHERE quals because there's no place to
	 * put them in an appendrel.  (This is a bit annoying...) If we didn't
	 * need to check this, we'd just test whether get_relids_in_jointree()
	 * yields a singleton set, to be more consistent with the coding of
	 * fix_append_rel_relids().
	 */
	jtnode = subquery->jointree;
	while (IsA(jtnode, FromExpr))
	{
		if (jtnode->quals != NULL)
			return false;
		if (list_length(jtnode->fromlist) != 1)
			return false;
		jtnode = linitial(jtnode->fromlist);
	}
	if (!IsA(jtnode, RangeTblRef))
		return false;

	return true;
}

/*
 * Helper routine for pull_up_subqueries: do pullup_replace_vars on every
 * expression in the jointree, without changing the jointree structure itself.
 * Ugly, but there's no other way...
 *
 * If we are at or below lowest_outer_join, we can suppress use of
 * PlaceHolderVars wrapped around the replacement expressions.
 */
static void
replace_vars_in_jointree(Node *jtnode,
						 pullup_replace_vars_context *context,
						 JoinExpr *lowest_outer_join)
{
	if (jtnode == NULL)
		return;
	if (IsA(jtnode, RangeTblRef))
	{
		/* nothing to do here */
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
			replace_vars_in_jointree(lfirst(l), context, lowest_outer_join);
		f->quals = pullup_replace_vars(f->quals, context);
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		bool		save_need_phvs = context->need_phvs;

		if (j == lowest_outer_join)
		{
			/* no more PHVs in or below this join */
			context->need_phvs = false;
			lowest_outer_join = NULL;
		}
		replace_vars_in_jointree(j->larg, context, lowest_outer_join);
		replace_vars_in_jointree(j->rarg, context, lowest_outer_join);
		j->quals = pullup_replace_vars(j->quals, context);

		/*
		 * We don't bother to update the colvars list, since it won't be used
		 * again ...
		 */
		context->need_phvs = save_need_phvs;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
}

/*
 * Apply pullup variable replacement throughout an expression tree
 *
 * Returns a modified copy of the tree, so this can't be used where we
 * need to do in-place replacement.
 */
static Node *
pullup_replace_vars(Node *expr, pullup_replace_vars_context *context)
{
	return replace_rte_variables(expr,
								 context->varno, 0,
								 pullup_replace_vars_callback,
								 (void *) context,
								 context->outer_hasSubLinks);
}

static Node *
pullup_replace_vars_callback(Var *var,
							 replace_rte_variables_context *context)
{
	pullup_replace_vars_context *rcon = (pullup_replace_vars_context *) context->callback_arg;
	int			varattno = var->varattno;
	Node	   *newnode;

	/*
	 * If PlaceHolderVars are needed, we cache the modified expressions in
	 * rcon->rv_cache[].  This is not in hopes of any material speed gain
	 * within this function, but to avoid generating identical PHVs with
	 * different IDs.  That would result in duplicate evaluations at runtime,
	 * and possibly prevent optimizations that rely on recognizing different
	 * references to the same subquery output as being equal().  So it's worth
	 * a bit of extra effort to avoid it.
	 */
	if (rcon->need_phvs &&
		varattno >= InvalidAttrNumber &&
		varattno <= list_length(rcon->targetlist) &&
		rcon->rv_cache[varattno] != NULL)
	{
		/* Just copy the entry and fall through to adjust its varlevelsup */
		newnode = copyObject(rcon->rv_cache[varattno]);
	}
	else if (varattno == InvalidAttrNumber)
	{
		/* Must expand whole-tuple reference into RowExpr */
		RowExpr    *rowexpr;
		List	   *colnames;
		List	   *fields;
		bool		save_need_phvs = rcon->need_phvs;
		int			save_sublevelsup = context->sublevels_up;

		/*
		 * If generating an expansion for a var of a named rowtype (ie, this
		 * is a plain relation RTE), then we must include dummy items for
		 * dropped columns.  If the var is RECORD (ie, this is a JOIN), then
		 * omit dropped columns. Either way, attach column names to the
		 * RowExpr for use of ruleutils.c.
		 *
		 * In order to be able to cache the results, we always generate the
		 * expansion with varlevelsup = 0, and then adjust if needed.
		 */
		expandRTE(rcon->target_rte,
				  var->varno, 0 /* not varlevelsup */ , var->location,
				  (var->vartype != RECORDOID),
				  &colnames, &fields);
		/* Adjust the generated per-field Vars, but don't insert PHVs */
		rcon->need_phvs = false;
		context->sublevels_up = 0;		/* to match the expandRTE output */
		fields = (List *) replace_rte_variables_mutator((Node *) fields,
														context);
		rcon->need_phvs = save_need_phvs;
		context->sublevels_up = save_sublevelsup;

		rowexpr = makeNode(RowExpr);
		rowexpr->args = fields;
		rowexpr->row_typeid = var->vartype;
		rowexpr->row_format = COERCE_IMPLICIT_CAST;
		rowexpr->colnames = colnames;
		rowexpr->location = var->location;
		newnode = (Node *) rowexpr;

		/*
		 * Insert PlaceHolderVar if needed.  Notice that we are wrapping one
		 * PlaceHolderVar around the whole RowExpr, rather than putting one
		 * around each element of the row.	This is because we need the
		 * expression to yield NULL, not ROW(NULL,NULL,...) when it is forced
		 * to null by an outer join.
		 */
		if (rcon->need_phvs)
		{
			/* RowExpr is certainly not strict, so always need PHV */
			newnode = (Node *)
				make_placeholder_expr(rcon->root,
									  (Expr *) newnode,
									  bms_make_singleton(rcon->varno));
			/* cache it with the PHV, and with varlevelsup still zero */
			rcon->rv_cache[InvalidAttrNumber] = copyObject(newnode);
		}
	}
	else
	{
		/* Normal case referencing one targetlist element */
		TargetEntry *tle = get_tle_by_resno(rcon->targetlist, varattno);

		if (tle == NULL)		/* shouldn't happen */
			elog(ERROR, "could not find attribute %d in subquery targetlist",
				 varattno);

		/* Make a copy of the tlist item to return */
		newnode = copyObject(tle->expr);

		/* Insert PlaceHolderVar if needed */
		if (rcon->need_phvs)
		{
			bool		wrap;

			if (newnode && IsA(newnode, Var) &&
				((Var *) newnode)->varlevelsup == 0)
			{
				/* Simple Vars always escape being wrapped */
				wrap = false;
			}
			else if (newnode && IsA(newnode, PlaceHolderVar) &&
					 ((PlaceHolderVar *) newnode)->phlevelsup == 0)
			{
				/* No need to wrap a PlaceHolderVar with another one, either */
				wrap = false;
			}
			else if (rcon->wrap_non_vars)
			{
				/* Wrap all non-Vars in a PlaceHolderVar */
				wrap = true;
			}
			else
			{
				/*
				 * If it contains a Var of current level, and does not contain
				 * any non-strict constructs, then it's certainly nullable and
				 * we don't need to insert a PlaceHolderVar.  (Note: in future
				 * maybe we should insert PlaceHolderVars anyway, when a tlist
				 * item is expensive to evaluate?
				 */
				if (contain_vars_of_level((Node *) newnode, 0) &&
					!contain_nonstrict_functions((Node *) newnode))
				{
					/* No wrap needed */
					wrap = false;
				}
				else
				{
					/* Else wrap it in a PlaceHolderVar */
					wrap = true;
				}
			}

			if (wrap)
				newnode = (Node *)
					make_placeholder_expr(rcon->root,
										  (Expr *) newnode,
										  bms_make_singleton(rcon->varno));

			/*
			 * Cache it if possible (ie, if the attno is in range, which it
			 * probably always should be).	We can cache the value even if we
			 * decided we didn't need a PHV, since this result will be
			 * suitable for any request that has need_phvs.
			 */
			if (varattno > InvalidAttrNumber &&
				varattno <= list_length(rcon->targetlist))
				rcon->rv_cache[varattno] = copyObject(newnode);
		}
	}

	/* Must adjust varlevelsup if tlist item is from higher query */
	if (var->varlevelsup > 0)
		IncrementVarSublevelsUp(newnode, var->varlevelsup, 0);

	return newnode;
}


/*
 * flatten_simple_union_all
 *		Try to optimize top-level UNION ALL structure into an appendrel
 *
 * If a query's setOperations tree consists entirely of simple UNION ALL
 * operations, flatten it into an append relation, which we can process more
 * intelligently than the general setops case.	Otherwise, do nothing.
 *
 * In most cases, this can succeed only for a top-level query, because for a
 * subquery in FROM, the parent query's invocation of pull_up_subqueries would
 * already have flattened the UNION via pull_up_simple_union_all.  But there
 * are a few cases we can support here but not in that code path, for example
 * when the subquery also contains ORDER BY.
 */
void
flatten_simple_union_all(PlannerInfo *root)
{
	Query	   *parse = root->parse;
	SetOperationStmt *topop;
	Node	   *leftmostjtnode;
	int			leftmostRTI;
	RangeTblEntry *leftmostRTE;
	int			childRTI;
	RangeTblEntry *childRTE;
	RangeTblRef *rtr;

	/* Shouldn't be called unless query has setops */
	topop = (SetOperationStmt *) parse->setOperations;
	Assert(topop && IsA(topop, SetOperationStmt));

	/* Can't optimize away a recursive UNION */
	if (root->hasRecursion)
		return;

	/*
	 * Recursively check the tree of set operations.  If not all UNION ALL
	 * with identical column types, punt.
	 */
	if (!is_simple_union_all_recurse((Node *) topop, parse, topop->colTypes))
		return;

	/*
	 * Locate the leftmost leaf query in the setops tree.  The upper query's
	 * Vars all refer to this RTE (see transformSetOperationStmt).
	 */
	leftmostjtnode = topop->larg;
	while (leftmostjtnode && IsA(leftmostjtnode, SetOperationStmt))
		leftmostjtnode = ((SetOperationStmt *) leftmostjtnode)->larg;
	Assert(leftmostjtnode && IsA(leftmostjtnode, RangeTblRef));
	leftmostRTI = ((RangeTblRef *) leftmostjtnode)->rtindex;
	leftmostRTE = rt_fetch(leftmostRTI, parse->rtable);
	Assert(leftmostRTE->rtekind == RTE_SUBQUERY);

	/*
	 * Make a copy of the leftmost RTE and add it to the rtable.  This copy
	 * will represent the leftmost leaf query in its capacity as a member of
	 * the appendrel.  The original will represent the appendrel as a whole.
	 * (We must do things this way because the upper query's Vars have to be
	 * seen as referring to the whole appendrel.)
	 */
	childRTE = copyObject(leftmostRTE);
	parse->rtable = lappend(parse->rtable, childRTE);
	childRTI = list_length(parse->rtable);

	/* Modify the setops tree to reference the child copy */
	((RangeTblRef *) leftmostjtnode)->rtindex = childRTI;

	/* Modify the formerly-leftmost RTE to mark it as an appendrel parent */
	leftmostRTE->inh = true;

	/*
	 * Form a RangeTblRef for the appendrel, and insert it into FROM.  The top
	 * Query of a setops tree should have had an empty FromClause initially.
	 */
	rtr = makeNode(RangeTblRef);
	rtr->rtindex = leftmostRTI;
	Assert(parse->jointree->fromlist == NIL);
	parse->jointree->fromlist = list_make1(rtr);

	/*
	 * Now pretend the query has no setops.  We must do this before trying to
	 * do subquery pullup, because of Assert in pull_up_simple_subquery.
	 */
	parse->setOperations = NULL;

	/*
	 * Build AppendRelInfo information, and apply pull_up_subqueries to the
	 * leaf queries of the UNION ALL.  (We must do that now because they
	 * weren't previously referenced by the jointree, and so were missed by
	 * the main invocation of pull_up_subqueries.)
	 */
	pull_up_union_leaf_queries((Node *) topop, root, leftmostRTI, parse, 0);
}


/*
 * reduce_outer_joins
 *		Attempt to reduce outer joins to plain inner joins.
 *
 * The idea here is that given a query like
 *		SELECT ... FROM a LEFT JOIN b ON (...) WHERE b.y = 42;
 * we can reduce the LEFT JOIN to a plain JOIN if the "=" operator in WHERE
 * is strict.  The strict operator will always return NULL, causing the outer
 * WHERE to fail, on any row where the LEFT JOIN filled in NULLs for b's
 * columns.  Therefore, there's no need for the join to produce null-extended
 * rows in the first place --- which makes it a plain join not an outer join.
 * (This scenario may not be very likely in a query written out by hand, but
 * it's reasonably likely when pushing quals down into complex views.)
 *
 * More generally, an outer join can be reduced in strength if there is a
 * strict qual above it in the qual tree that constrains a Var from the
 * nullable side of the join to be non-null.  (For FULL joins this applies
 * to each side separately.)
 *
 * Another transformation we apply here is to recognize cases like
 *		SELECT ... FROM a LEFT JOIN b ON (a.x = b.y) WHERE b.y IS NULL;
 * If the join clause is strict for b.y, then only null-extended rows could
 * pass the upper WHERE, and we can conclude that what the query is really
 * specifying is an anti-semijoin.	We change the join type from JOIN_LEFT
 * to JOIN_ANTI.  The IS NULL clause then becomes redundant, and must be
 * removed to prevent bogus selectivity calculations, but we leave it to
 * distribute_qual_to_rels to get rid of such clauses.
 *
 * Also, we get rid of JOIN_RIGHT cases by flipping them around to become
 * JOIN_LEFT.  This saves some code here and in some later planner routines,
 * but the main reason to do it is to not need to invent a JOIN_REVERSE_ANTI
 * join type.
 *
 * To ease recognition of strict qual clauses, we require this routine to be
 * run after expression preprocessing (i.e., qual canonicalization and JOIN
 * alias-var expansion).
 */
void
reduce_outer_joins(PlannerInfo *root)
{
	reduce_outer_joins_state *state;

	/*
	 * To avoid doing strictness checks on more quals than necessary, we want
	 * to stop descending the jointree as soon as there are no outer joins
	 * below our current point.  This consideration forces a two-pass process.
	 * The first pass gathers information about which base rels appear below
	 * each side of each join clause, and about whether there are outer
	 * join(s) below each side of each join clause. The second pass examines
	 * qual clauses and changes join types as it descends the tree.
	 */
	state = reduce_outer_joins_pass1((Node *) root->parse->jointree);

	/* planner.c shouldn't have called me if no outer joins */
	if (state == NULL || !state->contains_outer)
		elog(ERROR, "so where are the outer joins?");

	reduce_outer_joins_pass2((Node *) root->parse->jointree,
							 state, root, NULL, NIL, NIL);
}

/*
 * reduce_outer_joins_pass1 - phase 1 data collection
 *
 * Returns a state node describing the given jointree node.
 */
static reduce_outer_joins_state *
reduce_outer_joins_pass1(Node *jtnode)
{
	reduce_outer_joins_state *result;

	result = (reduce_outer_joins_state *)
		palloc(sizeof(reduce_outer_joins_state));
	result->relids = NULL;
	result->contains_outer = false;
	result->sub_states = NIL;

	if (jtnode == NULL)
		return result;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		result->relids = bms_make_singleton(varno);
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
		{
			reduce_outer_joins_state *sub_state;

			sub_state = reduce_outer_joins_pass1(lfirst(l));
			result->relids = bms_add_members(result->relids,
											 sub_state->relids);
			result->contains_outer |= sub_state->contains_outer;
			result->sub_states = lappend(result->sub_states, sub_state);
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		reduce_outer_joins_state *sub_state;

		/* join's own RT index is not wanted in result->relids */
		if (IS_OUTER_JOIN(j->jointype))
			result->contains_outer = true;

		sub_state = reduce_outer_joins_pass1(j->larg);
		result->relids = bms_add_members(result->relids,
										 sub_state->relids);
		result->contains_outer |= sub_state->contains_outer;
		result->sub_states = lappend(result->sub_states, sub_state);

		sub_state = reduce_outer_joins_pass1(j->rarg);
		result->relids = bms_add_members(result->relids,
										 sub_state->relids);
		result->contains_outer |= sub_state->contains_outer;
		result->sub_states = lappend(result->sub_states, sub_state);
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return result;
}

/*
 * reduce_outer_joins_pass2 - phase 2 processing
 *
 *	jtnode: current jointree node
 *	state: state data collected by phase 1 for this node
 *	root: toplevel planner state
 *	nonnullable_rels: set of base relids forced non-null by upper quals
 *	nonnullable_vars: list of Vars forced non-null by upper quals
 *	forced_null_vars: list of Vars forced null by upper quals
 */
static void
reduce_outer_joins_pass2(Node *jtnode,
						 reduce_outer_joins_state *state,
						 PlannerInfo *root,
						 Relids nonnullable_rels,
						 List *nonnullable_vars,
						 List *forced_null_vars)
{
	/*
	 * pass 2 should never descend as far as an empty subnode or base rel,
	 * because it's only called on subtrees marked as contains_outer.
	 */
	if (jtnode == NULL)
		elog(ERROR, "reached empty jointree");
	if (IsA(jtnode, RangeTblRef))
		elog(ERROR, "reached base rel");
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;
		ListCell   *s;
		Relids		pass_nonnullable_rels;
		List	   *pass_nonnullable_vars;
		List	   *pass_forced_null_vars;

		/* Scan quals to see if we can add any constraints */
		pass_nonnullable_rels = find_nonnullable_rels(f->quals);
		pass_nonnullable_rels = bms_add_members(pass_nonnullable_rels,
												nonnullable_rels);
		/* NB: we rely on list_concat to not damage its second argument */
		pass_nonnullable_vars = find_nonnullable_vars(f->quals);
		pass_nonnullable_vars = list_concat(pass_nonnullable_vars,
											nonnullable_vars);
		pass_forced_null_vars = find_forced_null_vars(f->quals);
		pass_forced_null_vars = list_concat(pass_forced_null_vars,
											forced_null_vars);
		/* And recurse --- but only into interesting subtrees */
		Assert(list_length(f->fromlist) == list_length(state->sub_states));
		forboth(l, f->fromlist, s, state->sub_states)
		{
			reduce_outer_joins_state *sub_state = lfirst(s);

			if (sub_state->contains_outer)
				reduce_outer_joins_pass2(lfirst(l), sub_state, root,
										 pass_nonnullable_rels,
										 pass_nonnullable_vars,
										 pass_forced_null_vars);
		}
		bms_free(pass_nonnullable_rels);
		/* can't so easily clean up var lists, unfortunately */
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;
		int			rtindex = j->rtindex;
		JoinType	jointype = j->jointype;
		reduce_outer_joins_state *left_state = linitial(state->sub_states);
		reduce_outer_joins_state *right_state = lsecond(state->sub_states);
		List	   *local_nonnullable_vars = NIL;
		bool		computed_local_nonnullable_vars = false;

		/* Can we simplify this join? */
		switch (jointype)
		{
			case JOIN_INNER:
				break;
			case JOIN_LEFT:
				if (bms_overlap(nonnullable_rels, right_state->relids))
					jointype = JOIN_INNER;
				break;
			case JOIN_RIGHT:
				if (bms_overlap(nonnullable_rels, left_state->relids))
					jointype = JOIN_INNER;
				break;
			case JOIN_FULL:
				if (bms_overlap(nonnullable_rels, left_state->relids))
				{
					if (bms_overlap(nonnullable_rels, right_state->relids))
						jointype = JOIN_INNER;
					else
						jointype = JOIN_LEFT;
				}
				else
				{
					if (bms_overlap(nonnullable_rels, right_state->relids))
						jointype = JOIN_RIGHT;
				}
				break;
			case JOIN_SEMI:
			case JOIN_ANTI:

				/*
				 * These could only have been introduced by pull_up_sublinks,
				 * so there's no way that upper quals could refer to their
				 * righthand sides, and no point in checking.
				 */
				break;
			default:
				elog(ERROR, "unrecognized join type: %d",
					 (int) jointype);
				break;
		}

		/*
		 * Convert JOIN_RIGHT to JOIN_LEFT.  Note that in the case where we
		 * reduced JOIN_FULL to JOIN_RIGHT, this will mean the JoinExpr no
		 * longer matches the internal ordering of any CoalesceExpr's built to
		 * represent merged join variables.  We don't care about that at
		 * present, but be wary of it ...
		 */
		if (jointype == JOIN_RIGHT)
		{
			Node	   *tmparg;

			tmparg = j->larg;
			j->larg = j->rarg;
			j->rarg = tmparg;
			jointype = JOIN_LEFT;
			right_state = linitial(state->sub_states);
			left_state = lsecond(state->sub_states);
		}

		/*
		 * See if we can reduce JOIN_LEFT to JOIN_ANTI.  This is the case if
		 * the join's own quals are strict for any var that was forced null by
		 * higher qual levels.	NOTE: there are other ways that we could
		 * detect an anti-join, in particular if we were to check whether Vars
		 * coming from the RHS must be non-null because of table constraints.
		 * That seems complicated and expensive though (in particular, one
		 * would have to be wary of lower outer joins). For the moment this
		 * seems sufficient.
		 */
		if (jointype == JOIN_LEFT)
		{
			List	   *overlap;

			local_nonnullable_vars = find_nonnullable_vars(j->quals);
			computed_local_nonnullable_vars = true;

			/*
			 * It's not sufficient to check whether local_nonnullable_vars and
			 * forced_null_vars overlap: we need to know if the overlap
			 * includes any RHS variables.
			 */
			overlap = list_intersection(local_nonnullable_vars,
										forced_null_vars);
			if (overlap != NIL &&
				bms_overlap(pull_varnos((Node *) overlap),
							right_state->relids))
				jointype = JOIN_ANTI;
		}

		/* Apply the jointype change, if any, to both jointree node and RTE */
		if (rtindex && jointype != j->jointype)
		{
			RangeTblEntry *rte = rt_fetch(rtindex, root->parse->rtable);

			Assert(rte->rtekind == RTE_JOIN);
			Assert(rte->jointype == j->jointype);
			rte->jointype = jointype;
		}
		j->jointype = jointype;

		/* Only recurse if there's more to do below here */
		if (left_state->contains_outer || right_state->contains_outer)
		{
			Relids		local_nonnullable_rels;
			List	   *local_forced_null_vars;
			Relids		pass_nonnullable_rels;
			List	   *pass_nonnullable_vars;
			List	   *pass_forced_null_vars;

			/*
			 * If this join is (now) inner, we can add any constraints its
			 * quals provide to those we got from above.  But if it is outer,
			 * we can pass down the local constraints only into the nullable
			 * side, because an outer join never eliminates any rows from its
			 * non-nullable side.  Also, there is no point in passing upper
			 * constraints into the nullable side, since if there were any
			 * we'd have been able to reduce the join.  (In the case of upper
			 * forced-null constraints, we *must not* pass them into the
			 * nullable side --- they either applied here, or not.) The upshot
			 * is that we pass either the local or the upper constraints,
			 * never both, to the children of an outer join.
			 *
			 * Note that a SEMI join works like an inner join here: it's okay
			 * to pass down both local and upper constraints.  (There can't be
			 * any upper constraints affecting its inner side, but it's not
			 * worth having a separate code path to avoid passing them.)
			 *
			 * At a FULL join we just punt and pass nothing down --- is it
			 * possible to be smarter?
			 */
			if (jointype != JOIN_FULL)
			{
				local_nonnullable_rels = find_nonnullable_rels(j->quals);
				if (!computed_local_nonnullable_vars)
					local_nonnullable_vars = find_nonnullable_vars(j->quals);
				local_forced_null_vars = find_forced_null_vars(j->quals);
				if (jointype == JOIN_INNER || jointype == JOIN_SEMI)
				{
					/* OK to merge upper and local constraints */
					local_nonnullable_rels = bms_add_members(local_nonnullable_rels,
														   nonnullable_rels);
					local_nonnullable_vars = list_concat(local_nonnullable_vars,
														 nonnullable_vars);
					local_forced_null_vars = list_concat(local_forced_null_vars,
														 forced_null_vars);
				}
			}
			else
			{
				/* no use in calculating these */
				local_nonnullable_rels = NULL;
				local_forced_null_vars = NIL;
			}

			if (left_state->contains_outer)
			{
				if (jointype == JOIN_INNER || jointype == JOIN_SEMI)
				{
					/* pass union of local and upper constraints */
					pass_nonnullable_rels = local_nonnullable_rels;
					pass_nonnullable_vars = local_nonnullable_vars;
					pass_forced_null_vars = local_forced_null_vars;
				}
				else if (jointype != JOIN_FULL) /* ie, LEFT or ANTI */
				{
					/* can't pass local constraints to non-nullable side */
					pass_nonnullable_rels = nonnullable_rels;
					pass_nonnullable_vars = nonnullable_vars;
					pass_forced_null_vars = forced_null_vars;
				}
				else
				{
					/* no constraints pass through JOIN_FULL */
					pass_nonnullable_rels = NULL;
					pass_nonnullable_vars = NIL;
					pass_forced_null_vars = NIL;
				}
				reduce_outer_joins_pass2(j->larg, left_state, root,
										 pass_nonnullable_rels,
										 pass_nonnullable_vars,
										 pass_forced_null_vars);
			}

			if (right_state->contains_outer)
			{
				if (jointype != JOIN_FULL)		/* ie, INNER/LEFT/SEMI/ANTI */
				{
					/* pass appropriate constraints, per comment above */
					pass_nonnullable_rels = local_nonnullable_rels;
					pass_nonnullable_vars = local_nonnullable_vars;
					pass_forced_null_vars = local_forced_null_vars;
				}
				else
				{
					/* no constraints pass through JOIN_FULL */
					pass_nonnullable_rels = NULL;
					pass_nonnullable_vars = NIL;
					pass_forced_null_vars = NIL;
				}
				reduce_outer_joins_pass2(j->rarg, right_state, root,
										 pass_nonnullable_rels,
										 pass_nonnullable_vars,
										 pass_forced_null_vars);
			}
			bms_free(local_nonnullable_rels);
		}
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
}

/*
 * substitute_multiple_relids - adjust node relid sets after pulling up
 * a subquery
 *
 * Find any PlaceHolderVar nodes in the given tree that reference the
 * pulled-up relid, and change them to reference the replacement relid(s).
 *
 * NOTE: although this has the form of a walker, we cheat and modify the
 * nodes in-place.	This should be OK since the tree was copied by
 * pullup_replace_vars earlier.  Avoid scribbling on the original values of
 * the bitmapsets, though, because expression_tree_mutator doesn't copy those.
 */

typedef struct
{
	int			varno;
	int			sublevels_up;
	Relids		subrelids;
} substitute_multiple_relids_context;

static bool
substitute_multiple_relids_walker(Node *node,
								  substitute_multiple_relids_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, PlaceHolderVar))
	{
		PlaceHolderVar *phv = (PlaceHolderVar *) node;

		if (phv->phlevelsup == context->sublevels_up &&
			bms_is_member(context->varno, phv->phrels))
		{
			phv->phrels = bms_union(phv->phrels,
									context->subrelids);
			phv->phrels = bms_del_member(phv->phrels,
										 context->varno);
		}
		/* fall through to examine children */
	}
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   substitute_multiple_relids_walker,
								   (void *) context, 0);
		context->sublevels_up--;
		return result;
	}
	/* Shouldn't need to handle planner auxiliary nodes here */
	Assert(!IsA(node, SpecialJoinInfo));
	Assert(!IsA(node, AppendRelInfo));
	Assert(!IsA(node, PlaceHolderInfo));
	Assert(!IsA(node, MinMaxAggInfo));

	return expression_tree_walker(node, substitute_multiple_relids_walker,
								  (void *) context);
}

static void
substitute_multiple_relids(Node *node, int varno, Relids subrelids)
{
	substitute_multiple_relids_context context;

	context.varno = varno;
	context.sublevels_up = 0;
	context.subrelids = subrelids;

	/*
	 * Must be prepared to start with a Query or a bare expression tree.
	 */
	query_or_expression_tree_walker(node,
									substitute_multiple_relids_walker,
									(void *) &context,
									0);
}

/*
 * fix_append_rel_relids: update RT-index fields of AppendRelInfo nodes
 *
 * When we pull up a subquery, any AppendRelInfo references to the subquery's
 * RT index have to be replaced by the substituted relid (and there had better
 * be only one).  We also need to apply substitute_multiple_relids to their
 * translated_vars lists, since those might contain PlaceHolderVars.
 *
 * We assume we may modify the AppendRelInfo nodes in-place.
 */
static void
fix_append_rel_relids(List *append_rel_list, int varno, Relids subrelids)
{
	ListCell   *l;
	int			subvarno = -1;

	/*
	 * We only want to extract the member relid once, but we mustn't fail
	 * immediately if there are multiple members; it could be that none of the
	 * AppendRelInfo nodes refer to it.  So compute it on first use. Note that
	 * bms_singleton_member will complain if set is not singleton.
	 */
	foreach(l, append_rel_list)
	{
		AppendRelInfo *appinfo = (AppendRelInfo *) lfirst(l);

		/* The parent_relid shouldn't ever be a pullup target */
		Assert(appinfo->parent_relid != varno);

		if (appinfo->child_relid == varno)
		{
			if (subvarno < 0)
				subvarno = bms_singleton_member(subrelids);
			appinfo->child_relid = subvarno;
		}

		/* Also finish fixups for its translated vars */
		substitute_multiple_relids((Node *) appinfo->translated_vars,
								   varno, subrelids);
	}
}

/*
 * get_relids_in_jointree: get set of RT indexes present in a jointree
 *
 * If include_joins is true, join RT indexes are included; if false,
 * only base rels are included.
 */
Relids
get_relids_in_jointree(Node *jtnode, bool include_joins)
{
	Relids		result = NULL;

	if (jtnode == NULL)
		return result;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		result = bms_make_singleton(varno);
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
		{
			result = bms_join(result,
							  get_relids_in_jointree(lfirst(l),
													 include_joins));
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		result = get_relids_in_jointree(j->larg, include_joins);
		result = bms_join(result,
						  get_relids_in_jointree(j->rarg, include_joins));
		if (include_joins && j->rtindex)
			result = bms_add_member(result, j->rtindex);
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return result;
}

/*
 * get_relids_for_join: get set of base RT indexes making up a join
 */
Relids
get_relids_for_join(PlannerInfo *root, int joinrelid)
{
	Node	   *jtnode;

	jtnode = find_jointree_node_for_rel((Node *) root->parse->jointree,
										joinrelid);
	if (!jtnode)
		elog(ERROR, "could not find join node %d", joinrelid);
	return get_relids_in_jointree(jtnode, false);
}

/*
 * find_jointree_node_for_rel: locate jointree node for a base or join RT index
 *
 * Returns NULL if not found
 */
static Node *
find_jointree_node_for_rel(Node *jtnode, int relid)
{
	if (jtnode == NULL)
		return NULL;
	if (IsA(jtnode, RangeTblRef))
	{
		int			varno = ((RangeTblRef *) jtnode)->rtindex;

		if (relid == varno)
			return jtnode;
	}
	else if (IsA(jtnode, FromExpr))
	{
		FromExpr   *f = (FromExpr *) jtnode;
		ListCell   *l;

		foreach(l, f->fromlist)
		{
			jtnode = find_jointree_node_for_rel(lfirst(l), relid);
			if (jtnode)
				return jtnode;
		}
	}
	else if (IsA(jtnode, JoinExpr))
	{
		JoinExpr   *j = (JoinExpr *) jtnode;

		if (relid == j->rtindex)
			return jtnode;
		jtnode = find_jointree_node_for_rel(j->larg, relid);
		if (jtnode)
			return jtnode;
		jtnode = find_jointree_node_for_rel(j->rarg, relid);
		if (jtnode)
			return jtnode;
	}
	else
		elog(ERROR, "unrecognized node type: %d",
			 (int) nodeTag(jtnode));
	return NULL;
}
