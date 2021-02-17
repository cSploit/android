/*-------------------------------------------------------------------------
 *
 * var.c
 *	  Var node manipulation routines
 *
 * Note: for most purposes, PlaceHolderVar is considered a Var too,
 * even if its contained expression is variable-free.  Also, CurrentOfExpr
 * is treated as a Var for purposes of determining whether an expression
 * contains variables.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/optimizer/util/var.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/sysattr.h"
#include "nodes/nodeFuncs.h"
#include "optimizer/prep.h"
#include "optimizer/var.h"
#include "parser/parsetree.h"
#include "rewrite/rewriteManip.h"


typedef struct
{
	Relids		varnos;
	int			sublevels_up;
} pull_varnos_context;

typedef struct
{
	int			var_location;
	int			sublevels_up;
} locate_var_of_level_context;

typedef struct
{
	int			var_location;
	int			relid;
	int			sublevels_up;
} locate_var_of_relation_context;

typedef struct
{
	int			min_varlevel;
	int			sublevels_up;
} find_minimum_var_level_context;

typedef struct
{
	List	   *varlist;
	PVCAggregateBehavior aggbehavior;
	PVCPlaceHolderBehavior phbehavior;
} pull_var_clause_context;

typedef struct
{
	PlannerInfo *root;
	int			sublevels_up;
	bool		possible_sublink;		/* could aliases include a SubLink? */
	bool		inserted_sublink;		/* have we inserted a SubLink? */
} flatten_join_alias_vars_context;

static bool pull_varnos_walker(Node *node,
				   pull_varnos_context *context);
static bool pull_varattnos_walker(Node *node, Bitmapset **varattnos);
static bool contain_var_clause_walker(Node *node, void *context);
static bool contain_vars_of_level_walker(Node *node, int *sublevels_up);
static bool locate_var_of_level_walker(Node *node,
						   locate_var_of_level_context *context);
static bool locate_var_of_relation_walker(Node *node,
							  locate_var_of_relation_context *context);
static bool find_minimum_var_level_walker(Node *node,
							  find_minimum_var_level_context *context);
static bool pull_var_clause_walker(Node *node,
					   pull_var_clause_context *context);
static Node *flatten_join_alias_vars_mutator(Node *node,
								flatten_join_alias_vars_context *context);
static Relids alias_relid_set(PlannerInfo *root, Relids relids);


/*
 * pull_varnos
 *		Create a set of all the distinct varnos present in a parsetree.
 *		Only varnos that reference level-zero rtable entries are considered.
 *
 * NOTE: this is used on not-yet-planned expressions.  It may therefore find
 * bare SubLinks, and if so it needs to recurse into them to look for uplevel
 * references to the desired rtable level!	But when we find a completed
 * SubPlan, we only need to look at the parameters passed to the subplan.
 */
Relids
pull_varnos(Node *node)
{
	pull_varnos_context context;

	context.varnos = NULL;
	context.sublevels_up = 0;

	/*
	 * Must be prepared to start with a Query or a bare expression tree; if
	 * it's a Query, we don't want to increment sublevels_up.
	 */
	query_or_expression_tree_walker(node,
									pull_varnos_walker,
									(void *) &context,
									0);

	return context.varnos;
}

static bool
pull_varnos_walker(Node *node, pull_varnos_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		if (var->varlevelsup == context->sublevels_up)
			context->varnos = bms_add_member(context->varnos, var->varno);
		return false;
	}
	if (IsA(node, CurrentOfExpr))
	{
		CurrentOfExpr *cexpr = (CurrentOfExpr *) node;

		if (context->sublevels_up == 0)
			context->varnos = bms_add_member(context->varnos, cexpr->cvarno);
		return false;
	}
	if (IsA(node, PlaceHolderVar))
	{
		/*
		 * Normally, we can just take the varnos in the contained expression.
		 * But if it is variable-free, use the PHV's syntactic relids.
		 */
		PlaceHolderVar *phv = (PlaceHolderVar *) node;
		pull_varnos_context subcontext;

		subcontext.varnos = NULL;
		subcontext.sublevels_up = context->sublevels_up;
		(void) pull_varnos_walker((Node *) phv->phexpr, &subcontext);

		if (bms_is_empty(subcontext.varnos) &&
			phv->phlevelsup == context->sublevels_up)
			context->varnos = bms_add_members(context->varnos, phv->phrels);
		else
			context->varnos = bms_join(context->varnos, subcontext.varnos);
		return false;
	}
	if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node, pull_varnos_walker,
								   (void *) context, 0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node, pull_varnos_walker,
								  (void *) context);
}


/*
 * pull_varattnos
 *		Find all the distinct attribute numbers present in an expression tree,
 *		and add them to the initial contents of *varattnos.
 *		Only Vars that reference RTE 1 of rtable level zero are considered.
 *
 * Attribute numbers are offset by FirstLowInvalidHeapAttributeNumber so that
 * we can include system attributes (e.g., OID) in the bitmap representation.
 *
 * Currently, this does not support subqueries nor expressions containing
 * references to multiple tables; not needed since it's only applied to
 * index expressions and predicates.
 */
void
pull_varattnos(Node *node, Bitmapset **varattnos)
{
	(void) pull_varattnos_walker(node, varattnos);
}

static bool
pull_varattnos_walker(Node *node, Bitmapset **varattnos)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		Assert(var->varno == 1);
		*varattnos = bms_add_member(*varattnos,
						 var->varattno - FirstLowInvalidHeapAttributeNumber);
		return false;
	}
	/* Should not find a subquery or subplan */
	Assert(!IsA(node, Query));
	Assert(!IsA(node, SubPlan));

	return expression_tree_walker(node, pull_varattnos_walker,
								  (void *) varattnos);
}


/*
 * contain_var_clause
 *	  Recursively scan a clause to discover whether it contains any Var nodes
 *	  (of the current query level).
 *
 *	  Returns true if any varnode found.
 *
 * Does not examine subqueries, therefore must only be used after reduction
 * of sublinks to subplans!
 */
bool
contain_var_clause(Node *node)
{
	return contain_var_clause_walker(node, NULL);
}

static bool
contain_var_clause_walker(Node *node, void *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		if (((Var *) node)->varlevelsup == 0)
			return true;		/* abort the tree traversal and return true */
		return false;
	}
	if (IsA(node, CurrentOfExpr))
		return true;
	if (IsA(node, PlaceHolderVar))
	{
		if (((PlaceHolderVar *) node)->phlevelsup == 0)
			return true;		/* abort the tree traversal and return true */
		/* else fall through to check the contained expr */
	}
	return expression_tree_walker(node, contain_var_clause_walker, context);
}


/*
 * contain_vars_of_level
 *	  Recursively scan a clause to discover whether it contains any Var nodes
 *	  of the specified query level.
 *
 *	  Returns true if any such Var found.
 *
 * Will recurse into sublinks.	Also, may be invoked directly on a Query.
 */
bool
contain_vars_of_level(Node *node, int levelsup)
{
	int			sublevels_up = levelsup;

	return query_or_expression_tree_walker(node,
										   contain_vars_of_level_walker,
										   (void *) &sublevels_up,
										   0);
}

static bool
contain_vars_of_level_walker(Node *node, int *sublevels_up)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		if (((Var *) node)->varlevelsup == *sublevels_up)
			return true;		/* abort tree traversal and return true */
		return false;
	}
	if (IsA(node, CurrentOfExpr))
	{
		if (*sublevels_up == 0)
			return true;
		return false;
	}
	if (IsA(node, PlaceHolderVar))
	{
		if (((PlaceHolderVar *) node)->phlevelsup == *sublevels_up)
			return true;		/* abort the tree traversal and return true */
		/* else fall through to check the contained expr */
	}
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		(*sublevels_up)++;
		result = query_tree_walker((Query *) node,
								   contain_vars_of_level_walker,
								   (void *) sublevels_up,
								   0);
		(*sublevels_up)--;
		return result;
	}
	return expression_tree_walker(node,
								  contain_vars_of_level_walker,
								  (void *) sublevels_up);
}


/*
 * locate_var_of_level
 *	  Find the parse location of any Var of the specified query level.
 *
 * Returns -1 if no such Var is in the querytree, or if they all have
 * unknown parse location.	(The former case is probably caller error,
 * but we don't bother to distinguish it from the latter case.)
 *
 * Will recurse into sublinks.	Also, may be invoked directly on a Query.
 *
 * Note: it might seem appropriate to merge this functionality into
 * contain_vars_of_level, but that would complicate that function's API.
 * Currently, the only uses of this function are for error reporting,
 * and so shaving cycles probably isn't very important.
 */
int
locate_var_of_level(Node *node, int levelsup)
{
	locate_var_of_level_context context;

	context.var_location = -1;	/* in case we find nothing */
	context.sublevels_up = levelsup;

	(void) query_or_expression_tree_walker(node,
										   locate_var_of_level_walker,
										   (void *) &context,
										   0);

	return context.var_location;
}

static bool
locate_var_of_level_walker(Node *node,
						   locate_var_of_level_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		if (var->varlevelsup == context->sublevels_up &&
			var->location >= 0)
		{
			context->var_location = var->location;
			return true;		/* abort tree traversal and return true */
		}
		return false;
	}
	if (IsA(node, CurrentOfExpr))
	{
		/* since CurrentOfExpr doesn't carry location, nothing we can do */
		return false;
	}
	/* No extra code needed for PlaceHolderVar; just look in contained expr */
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   locate_var_of_level_walker,
								   (void *) context,
								   0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node,
								  locate_var_of_level_walker,
								  (void *) context);
}


/*
 * locate_var_of_relation
 *	  Find the parse location of any Var of the specified relation.
 *
 * Returns -1 if no such Var is in the querytree, or if they all have
 * unknown parse location.
 *
 * Will recurse into sublinks.	Also, may be invoked directly on a Query.
 */
int
locate_var_of_relation(Node *node, int relid, int levelsup)
{
	locate_var_of_relation_context context;

	context.var_location = -1;	/* in case we find nothing */
	context.relid = relid;
	context.sublevels_up = levelsup;

	(void) query_or_expression_tree_walker(node,
										   locate_var_of_relation_walker,
										   (void *) &context,
										   0);

	return context.var_location;
}

static bool
locate_var_of_relation_walker(Node *node,
							  locate_var_of_relation_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;

		if (var->varno == context->relid &&
			var->varlevelsup == context->sublevels_up &&
			var->location >= 0)
		{
			context->var_location = var->location;
			return true;		/* abort tree traversal and return true */
		}
		return false;
	}
	if (IsA(node, CurrentOfExpr))
	{
		/* since CurrentOfExpr doesn't carry location, nothing we can do */
		return false;
	}
	/* No extra code needed for PlaceHolderVar; just look in contained expr */
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   locate_var_of_relation_walker,
								   (void *) context,
								   0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node,
								  locate_var_of_relation_walker,
								  (void *) context);
}


/*
 * find_minimum_var_level
 *	  Recursively scan a clause to find the lowest variable level it
 *	  contains --- for example, zero is returned if there are any local
 *	  variables, one if there are no local variables but there are
 *	  one-level-up outer references, etc.  Subqueries are scanned to see
 *	  if they possess relevant outer references.  (But any local variables
 *	  within subqueries are not relevant.)
 *
 *	  -1 is returned if the clause has no variables at all.
 *
 * Will recurse into sublinks.	Also, may be invoked directly on a Query.
 */
int
find_minimum_var_level(Node *node)
{
	find_minimum_var_level_context context;

	context.min_varlevel = -1;	/* signifies nothing found yet */
	context.sublevels_up = 0;

	(void) query_or_expression_tree_walker(node,
										   find_minimum_var_level_walker,
										   (void *) &context,
										   0);

	return context.min_varlevel;
}

static bool
find_minimum_var_level_walker(Node *node,
							  find_minimum_var_level_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		int			varlevelsup = ((Var *) node)->varlevelsup;

		/* convert levelsup to frame of reference of original query */
		varlevelsup -= context->sublevels_up;
		/* ignore local vars of subqueries */
		if (varlevelsup >= 0)
		{
			if (context->min_varlevel < 0 ||
				context->min_varlevel > varlevelsup)
			{
				context->min_varlevel = varlevelsup;

				/*
				 * As soon as we find a local variable, we can abort the tree
				 * traversal, since min_varlevel is then certainly 0.
				 */
				if (varlevelsup == 0)
					return true;
			}
		}
	}
	if (IsA(node, CurrentOfExpr))
	{
		int			varlevelsup = 0;

		/* convert levelsup to frame of reference of original query */
		varlevelsup -= context->sublevels_up;
		/* ignore local vars of subqueries */
		if (varlevelsup >= 0)
		{
			if (context->min_varlevel < 0 ||
				context->min_varlevel > varlevelsup)
			{
				context->min_varlevel = varlevelsup;

				/*
				 * As soon as we find a local variable, we can abort the tree
				 * traversal, since min_varlevel is then certainly 0.
				 */
				if (varlevelsup == 0)
					return true;
			}
		}
	}

	/*
	 * An Aggref must be treated like a Var of its level.  Normally we'd get
	 * the same result from looking at the Vars in the aggregate's argument,
	 * but this fails in the case of a Var-less aggregate call (COUNT(*)).
	 */
	if (IsA(node, Aggref))
	{
		int			agglevelsup = ((Aggref *) node)->agglevelsup;

		/* convert levelsup to frame of reference of original query */
		agglevelsup -= context->sublevels_up;
		/* ignore local aggs of subqueries */
		if (agglevelsup >= 0)
		{
			if (context->min_varlevel < 0 ||
				context->min_varlevel > agglevelsup)
			{
				context->min_varlevel = agglevelsup;

				/*
				 * As soon as we find a local aggregate, we can abort the tree
				 * traversal, since min_varlevel is then certainly 0.
				 */
				if (agglevelsup == 0)
					return true;
			}
		}
	}
	/* Likewise, make sure PlaceHolderVar is treated correctly */
	if (IsA(node, PlaceHolderVar))
	{
		int			phlevelsup = ((PlaceHolderVar *) node)->phlevelsup;

		/* convert levelsup to frame of reference of original query */
		phlevelsup -= context->sublevels_up;
		/* ignore local vars of subqueries */
		if (phlevelsup >= 0)
		{
			if (context->min_varlevel < 0 ||
				context->min_varlevel > phlevelsup)
			{
				context->min_varlevel = phlevelsup;

				/*
				 * As soon as we find a local variable, we can abort the tree
				 * traversal, since min_varlevel is then certainly 0.
				 */
				if (phlevelsup == 0)
					return true;
			}
		}
	}
	if (IsA(node, Query))
	{
		/* Recurse into subselects */
		bool		result;

		context->sublevels_up++;
		result = query_tree_walker((Query *) node,
								   find_minimum_var_level_walker,
								   (void *) context,
								   0);
		context->sublevels_up--;
		return result;
	}
	return expression_tree_walker(node,
								  find_minimum_var_level_walker,
								  (void *) context);
}


/*
 * pull_var_clause
 *	  Recursively pulls all Var nodes from an expression clause.
 *
 *	  Aggrefs are handled according to 'aggbehavior':
 *		PVC_REJECT_AGGREGATES		throw error if Aggref found
 *		PVC_INCLUDE_AGGREGATES		include Aggrefs in output list
 *		PVC_RECURSE_AGGREGATES		recurse into Aggref arguments
 *	  Vars within an Aggref's expression are included only in the last case.
 *
 *	  PlaceHolderVars are handled according to 'phbehavior':
 *		PVC_REJECT_PLACEHOLDERS		throw error if PlaceHolderVar found
 *		PVC_INCLUDE_PLACEHOLDERS	include PlaceHolderVars in output list
 *		PVC_RECURSE_PLACEHOLDERS	recurse into PlaceHolderVar arguments
 *	  Vars within a PHV's expression are included only in the last case.
 *
 *	  CurrentOfExpr nodes are ignored in all cases.
 *
 *	  Upper-level vars (with varlevelsup > 0) should not be seen here,
 *	  likewise for upper-level Aggrefs and PlaceHolderVars.
 *
 *	  Returns list of nodes found.	Note the nodes themselves are not
 *	  copied, only referenced.
 *
 * Does not examine subqueries, therefore must only be used after reduction
 * of sublinks to subplans!
 */
List *
pull_var_clause(Node *node, PVCAggregateBehavior aggbehavior,
				PVCPlaceHolderBehavior phbehavior)
{
	pull_var_clause_context context;

	context.varlist = NIL;
	context.aggbehavior = aggbehavior;
	context.phbehavior = phbehavior;

	pull_var_clause_walker(node, &context);
	return context.varlist;
}

static bool
pull_var_clause_walker(Node *node, pull_var_clause_context *context)
{
	if (node == NULL)
		return false;
	if (IsA(node, Var))
	{
		if (((Var *) node)->varlevelsup != 0)
			elog(ERROR, "Upper-level Var found where not expected");
		context->varlist = lappend(context->varlist, node);
		return false;
	}
	else if (IsA(node, Aggref))
	{
		if (((Aggref *) node)->agglevelsup != 0)
			elog(ERROR, "Upper-level Aggref found where not expected");
		switch (context->aggbehavior)
		{
			case PVC_REJECT_AGGREGATES:
				elog(ERROR, "Aggref found where not expected");
				break;
			case PVC_INCLUDE_AGGREGATES:
				context->varlist = lappend(context->varlist, node);
				/* we do NOT descend into the contained expression */
				return false;
			case PVC_RECURSE_AGGREGATES:
				/* ignore the aggregate, look at its argument instead */
				break;
		}
	}
	else if (IsA(node, PlaceHolderVar))
	{
		if (((PlaceHolderVar *) node)->phlevelsup != 0)
			elog(ERROR, "Upper-level PlaceHolderVar found where not expected");
		switch (context->phbehavior)
		{
			case PVC_REJECT_PLACEHOLDERS:
				elog(ERROR, "PlaceHolderVar found where not expected");
				break;
			case PVC_INCLUDE_PLACEHOLDERS:
				context->varlist = lappend(context->varlist, node);
				/* we do NOT descend into the contained expression */
				return false;
			case PVC_RECURSE_PLACEHOLDERS:
				/* ignore the placeholder, look at its argument instead */
				break;
		}
	}
	return expression_tree_walker(node, pull_var_clause_walker,
								  (void *) context);
}


/*
 * flatten_join_alias_vars
 *	  Replace Vars that reference JOIN outputs with references to the original
 *	  relation variables instead.  This allows quals involving such vars to be
 *	  pushed down.	Whole-row Vars that reference JOIN relations are expanded
 *	  into RowExpr constructs that name the individual output Vars.  This
 *	  is necessary since we will not scan the JOIN as a base relation, which
 *	  is the only way that the executor can directly handle whole-row Vars.
 *
 * This also adjusts relid sets found in some expression node types to
 * substitute the contained base rels for any join relid.
 *
 * If a JOIN contains sub-selects that have been flattened, its join alias
 * entries might now be arbitrary expressions, not just Vars.  This affects
 * this function in one important way: we might find ourselves inserting
 * SubLink expressions into subqueries, and we must make sure that their
 * Query.hasSubLinks fields get set to TRUE if so.	If there are any
 * SubLinks in the join alias lists, the outer Query should already have
 * hasSubLinks = TRUE, so this is only relevant to un-flattened subqueries.
 *
 * NOTE: this is used on not-yet-planned expressions.  We do not expect it
 * to be applied directly to a Query node.
 */
Node *
flatten_join_alias_vars(PlannerInfo *root, Node *node)
{
	flatten_join_alias_vars_context context;

	context.root = root;
	context.sublevels_up = 0;
	/* flag whether join aliases could possibly contain SubLinks */
	context.possible_sublink = root->parse->hasSubLinks;
	/* if hasSubLinks is already true, no need to work hard */
	context.inserted_sublink = root->parse->hasSubLinks;

	return flatten_join_alias_vars_mutator(node, &context);
}

static Node *
flatten_join_alias_vars_mutator(Node *node,
								flatten_join_alias_vars_context *context)
{
	if (node == NULL)
		return NULL;
	if (IsA(node, Var))
	{
		Var		   *var = (Var *) node;
		RangeTblEntry *rte;
		Node	   *newvar;

		/* No change unless Var belongs to a JOIN of the target level */
		if (var->varlevelsup != context->sublevels_up)
			return node;		/* no need to copy, really */
		rte = rt_fetch(var->varno, context->root->parse->rtable);
		if (rte->rtekind != RTE_JOIN)
			return node;
		if (var->varattno == InvalidAttrNumber)
		{
			/* Must expand whole-row reference */
			RowExpr    *rowexpr;
			List	   *fields = NIL;
			AttrNumber	attnum;
			ListCell   *l;

			attnum = 0;
			foreach(l, rte->joinaliasvars)
			{
				newvar = (Node *) lfirst(l);
				attnum++;
				/* Ignore dropped columns */
				if (newvar == NULL)
					continue;

				/*
				 * If we are expanding an alias carried down from an upper
				 * query, must adjust its varlevelsup fields.
				 */
				if (context->sublevels_up != 0)
				{
					newvar = copyObject(newvar);
					IncrementVarSublevelsUp(newvar, context->sublevels_up, 0);
				}
				/* Recurse in case join input is itself a join */
				/* (also takes care of setting inserted_sublink if needed) */
				newvar = flatten_join_alias_vars_mutator(newvar, context);
				fields = lappend(fields, newvar);
			}
			rowexpr = makeNode(RowExpr);
			rowexpr->args = fields;
			rowexpr->row_typeid = var->vartype;
			rowexpr->row_format = COERCE_IMPLICIT_CAST;
			rowexpr->colnames = NIL;
			rowexpr->location = -1;

			return (Node *) rowexpr;
		}

		/* Expand join alias reference */
		Assert(var->varattno > 0);
		newvar = (Node *) list_nth(rte->joinaliasvars, var->varattno - 1);
		Assert(newvar != NULL);

		/*
		 * If we are expanding an alias carried down from an upper query, must
		 * adjust its varlevelsup fields.
		 */
		if (context->sublevels_up != 0)
		{
			newvar = copyObject(newvar);
			IncrementVarSublevelsUp(newvar, context->sublevels_up, 0);
		}

		/* Recurse in case join input is itself a join */
		newvar = flatten_join_alias_vars_mutator(newvar, context);

		/* Detect if we are adding a sublink to query */
		if (context->possible_sublink && !context->inserted_sublink)
			context->inserted_sublink = checkExprHasSubLink(newvar);

		return newvar;
	}
	if (IsA(node, PlaceHolderVar))
	{
		/* Copy the PlaceHolderVar node with correct mutation of subnodes */
		PlaceHolderVar *phv;

		phv = (PlaceHolderVar *) expression_tree_mutator(node,
											 flatten_join_alias_vars_mutator,
														 (void *) context);
		/* now fix PlaceHolderVar's relid sets */
		if (phv->phlevelsup == context->sublevels_up)
		{
			phv->phrels = alias_relid_set(context->root,
										  phv->phrels);
		}
		return (Node *) phv;
	}

	if (IsA(node, Query))
	{
		/* Recurse into RTE subquery or not-yet-planned sublink subquery */
		Query	   *newnode;
		bool		save_inserted_sublink;

		context->sublevels_up++;
		save_inserted_sublink = context->inserted_sublink;
		context->inserted_sublink = ((Query *) node)->hasSubLinks;
		newnode = query_tree_mutator((Query *) node,
									 flatten_join_alias_vars_mutator,
									 (void *) context,
									 QTW_IGNORE_JOINALIASES);
		newnode->hasSubLinks |= context->inserted_sublink;
		context->inserted_sublink = save_inserted_sublink;
		context->sublevels_up--;
		return (Node *) newnode;
	}
	/* Already-planned tree not supported */
	Assert(!IsA(node, SubPlan));
	/* Shouldn't need to handle these planner auxiliary nodes here */
	Assert(!IsA(node, SpecialJoinInfo));
	Assert(!IsA(node, PlaceHolderInfo));
	Assert(!IsA(node, MinMaxAggInfo));

	return expression_tree_mutator(node, flatten_join_alias_vars_mutator,
								   (void *) context);
}

/*
 * alias_relid_set: in a set of RT indexes, replace joins by their
 * underlying base relids
 */
static Relids
alias_relid_set(PlannerInfo *root, Relids relids)
{
	Relids		result = NULL;
	Relids		tmprelids;
	int			rtindex;

	tmprelids = bms_copy(relids);
	while ((rtindex = bms_first_member(tmprelids)) >= 0)
	{
		RangeTblEntry *rte = rt_fetch(rtindex, root->parse->rtable);

		if (rte->rtekind == RTE_JOIN)
			result = bms_join(result, get_relids_for_join(root, rtindex));
		else
			result = bms_add_member(result, rtindex);
	}
	bms_free(tmprelids);
	return result;
}
