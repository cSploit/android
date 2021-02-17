/*-------------------------------------------------------------------------
 *
 * parse_func.c
 *		handle function calls in parser
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/parser/parse_func.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "funcapi.h"
#include "nodes/makefuncs.h"
#include "nodes/nodeFuncs.h"
#include "parser/parse_agg.h"
#include "parser/parse_coerce.h"
#include "parser/parse_func.h"
#include "parser/parse_relation.h"
#include "parser/parse_target.h"
#include "parser/parse_type.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/syscache.h"


static Oid	FuncNameAsType(List *funcname);
static Node *ParseComplexProjection(ParseState *pstate, char *funcname,
					   Node *first_arg, int location);


/*
 *	Parse a function call
 *
 *	For historical reasons, Postgres tries to treat the notations tab.col
 *	and col(tab) as equivalent: if a single-argument function call has an
 *	argument of complex type and the (unqualified) function name matches
 *	any attribute of the type, we take it as a column projection.  Conversely
 *	a function of a single complex-type argument can be written like a
 *	column reference, allowing functions to act like computed columns.
 *
 *	Hence, both cases come through here.  The is_column parameter tells us
 *	which syntactic construct is actually being dealt with, but this is
 *	intended to be used only to deliver an appropriate error message,
 *	not to affect the semantics.  When is_column is true, we should have
 *	a single argument (the putative table), unqualified function name
 *	equal to the column name, and no aggregate or variadic decoration.
 *	Also, when is_column is true, we return NULL on failure rather than
 *	reporting a no-such-function error.
 *
 *	The argument expressions (in fargs) must have been transformed already.
 *	But the agg_order expressions, if any, have not been.
 */
Node *
ParseFuncOrColumn(ParseState *pstate, List *funcname, List *fargs,
				  List *agg_order, bool agg_star, bool agg_distinct,
				  bool func_variadic,
				  WindowDef *over, bool is_column, int location)
{
	Oid			rettype;
	Oid			funcid;
	ListCell   *l;
	ListCell   *nextl;
	Node	   *first_arg = NULL;
	int			nargs;
	int			nargsplusdefs;
	Oid			actual_arg_types[FUNC_MAX_ARGS];
	Oid		   *declared_arg_types;
	List	   *argnames;
	List	   *argdefaults;
	Node	   *retval;
	bool		retset;
	int			nvargs;
	FuncDetailCode fdresult;

	/*
	 * Most of the rest of the parser just assumes that functions do not have
	 * more than FUNC_MAX_ARGS parameters.	We have to test here to protect
	 * against array overruns, etc.  Of course, this may not be a function,
	 * but the test doesn't hurt.
	 */
	if (list_length(fargs) > FUNC_MAX_ARGS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
			 errmsg_plural("cannot pass more than %d argument to a function",
						   "cannot pass more than %d arguments to a function",
						   FUNC_MAX_ARGS,
						   FUNC_MAX_ARGS),
				 parser_errposition(pstate, location)));

	/*
	 * Extract arg type info in preparation for function lookup.
	 *
	 * If any arguments are Param markers of type VOID, we discard them from
	 * the parameter list.	This is a hack to allow the JDBC driver to not
	 * have to distinguish "input" and "output" parameter symbols while
	 * parsing function-call constructs.  We can't use foreach() because we
	 * may modify the list ...
	 */
	nargs = 0;
	for (l = list_head(fargs); l != NULL; l = nextl)
	{
		Node	   *arg = lfirst(l);
		Oid			argtype = exprType(arg);

		nextl = lnext(l);

		if (argtype == VOIDOID && IsA(arg, Param) &&!is_column)
		{
			fargs = list_delete_ptr(fargs, arg);
			continue;
		}

		actual_arg_types[nargs++] = argtype;
	}

	/*
	 * Check for named arguments; if there are any, build a list of names.
	 *
	 * We allow mixed notation (some named and some not), but only with all
	 * the named parameters after all the unnamed ones.  So the name list
	 * corresponds to the last N actual parameters and we don't need any extra
	 * bookkeeping to match things up.
	 */
	argnames = NIL;
	foreach(l, fargs)
	{
		Node	   *arg = lfirst(l);

		if (IsA(arg, NamedArgExpr))
		{
			NamedArgExpr *na = (NamedArgExpr *) arg;
			ListCell   *lc;

			/* Reject duplicate arg names */
			foreach(lc, argnames)
			{
				if (strcmp(na->name, (char *) lfirst(lc)) == 0)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
						   errmsg("argument name \"%s\" used more than once",
								  na->name),
							 parser_errposition(pstate, na->location)));
			}
			argnames = lappend(argnames, na->name);
		}
		else
		{
			if (argnames != NIL)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
				  errmsg("positional argument cannot follow named argument"),
						 parser_errposition(pstate, exprLocation(arg))));
		}
	}

	if (fargs)
	{
		first_arg = linitial(fargs);
		Assert(first_arg != NULL);
	}

	/*
	 * Check for column projection: if function has one argument, and that
	 * argument is of complex type, and function name is not qualified, then
	 * the "function call" could be a projection.  We also check that there
	 * wasn't any aggregate or variadic decoration, nor an argument name.
	 */
	if (nargs == 1 && agg_order == NIL && !agg_star && !agg_distinct &&
		over == NULL && !func_variadic && argnames == NIL &&
		list_length(funcname) == 1)
	{
		Oid			argtype = actual_arg_types[0];

		if (argtype == RECORDOID || ISCOMPLEX(argtype))
		{
			retval = ParseComplexProjection(pstate,
											strVal(linitial(funcname)),
											first_arg,
											location);
			if (retval)
				return retval;

			/*
			 * If ParseComplexProjection doesn't recognize it as a projection,
			 * just press on.
			 */
		}
	}

	/*
	 * Okay, it's not a column projection, so it must really be a function.
	 * func_get_detail looks up the function in the catalogs, does
	 * disambiguation for polymorphic functions, handles inheritance, and
	 * returns the funcid and type and set or singleton status of the
	 * function's return value.  It also returns the true argument types to
	 * the function.
	 *
	 * Note: for a named-notation or variadic function call, the reported
	 * "true" types aren't really what is in pg_proc: the types are reordered
	 * to match the given argument order of named arguments, and a variadic
	 * argument is replaced by a suitable number of copies of its element
	 * type.  We'll fix up the variadic case below.  We may also have to deal
	 * with default arguments.
	 */
	fdresult = func_get_detail(funcname, fargs, argnames, nargs,
							   actual_arg_types,
							   !func_variadic, true,
							   &funcid, &rettype, &retset, &nvargs,
							   &declared_arg_types, &argdefaults);
	if (fdresult == FUNCDETAIL_COERCION)
	{
		/*
		 * We interpreted it as a type coercion. coerce_type can handle these
		 * cases, so why duplicate code...
		 */
		return coerce_type(pstate, linitial(fargs),
						   actual_arg_types[0], rettype, -1,
						   COERCION_EXPLICIT, COERCE_EXPLICIT_CALL, location);
	}
	else if (fdresult == FUNCDETAIL_NORMAL)
	{
		/*
		 * Normal function found; was there anything indicating it must be an
		 * aggregate?
		 */
		if (agg_star)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
			   errmsg("%s(*) specified, but %s is not an aggregate function",
					  NameListToString(funcname),
					  NameListToString(funcname)),
					 parser_errposition(pstate, location)));
		if (agg_distinct)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
			errmsg("DISTINCT specified, but %s is not an aggregate function",
				   NameListToString(funcname)),
					 parser_errposition(pstate, location)));
		if (agg_order != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
			errmsg("ORDER BY specified, but %s is not an aggregate function",
				   NameListToString(funcname)),
					 parser_errposition(pstate, location)));
		if (over)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("OVER specified, but %s is not a window function nor an aggregate function",
							NameListToString(funcname)),
					 parser_errposition(pstate, location)));
	}
	else if (!(fdresult == FUNCDETAIL_AGGREGATE ||
			   fdresult == FUNCDETAIL_WINDOWFUNC))
	{
		/*
		 * Oops.  Time to die.
		 *
		 * If we are dealing with the attribute notation rel.function, let the
		 * caller handle failure.
		 */
		if (is_column)
			return NULL;

		/*
		 * Else generate a detailed complaint for a function
		 */
		if (fdresult == FUNCDETAIL_MULTIPLE)
			ereport(ERROR,
					(errcode(ERRCODE_AMBIGUOUS_FUNCTION),
					 errmsg("function %s is not unique",
							func_signature_string(funcname, nargs, argnames,
												  actual_arg_types)),
					 errhint("Could not choose a best candidate function. "
							 "You might need to add explicit type casts."),
					 parser_errposition(pstate, location)));
		else if (list_length(agg_order) > 1)
		{
			/* It's agg(x, ORDER BY y,z) ... perhaps misplaced ORDER BY */
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("function %s does not exist",
							func_signature_string(funcname, nargs, argnames,
												  actual_arg_types)),
					 errhint("No aggregate function matches the given name and argument types. "
					  "Perhaps you misplaced ORDER BY; ORDER BY must appear "
							 "after all regular arguments of the aggregate."),
					 parser_errposition(pstate, location)));
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("function %s does not exist",
							func_signature_string(funcname, nargs, argnames,
												  actual_arg_types)),
			errhint("No function matches the given name and argument types. "
					"You might need to add explicit type casts."),
					 parser_errposition(pstate, location)));
	}

	/*
	 * If there are default arguments, we have to include their types in
	 * actual_arg_types for the purpose of checking generic type consistency.
	 * However, we do NOT put them into the generated parse node, because
	 * their actual values might change before the query gets run.	The
	 * planner has to insert the up-to-date values at plan time.
	 */
	nargsplusdefs = nargs;
	foreach(l, argdefaults)
	{
		Node	   *expr = (Node *) lfirst(l);

		/* probably shouldn't happen ... */
		if (nargsplusdefs >= FUNC_MAX_ARGS)
			ereport(ERROR,
					(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
			 errmsg_plural("cannot pass more than %d argument to a function",
						   "cannot pass more than %d arguments to a function",
						   FUNC_MAX_ARGS,
						   FUNC_MAX_ARGS),
					 parser_errposition(pstate, location)));

		actual_arg_types[nargsplusdefs++] = exprType(expr);
	}

	/*
	 * enforce consistency with polymorphic argument and return types,
	 * possibly adjusting return type or declared_arg_types (which will be
	 * used as the cast destination by make_fn_arguments)
	 */
	rettype = enforce_generic_type_consistency(actual_arg_types,
											   declared_arg_types,
											   nargsplusdefs,
											   rettype,
											   false);

	/* perform the necessary typecasting of arguments */
	make_fn_arguments(pstate, fargs, actual_arg_types, declared_arg_types);

	/*
	 * If it's a variadic function call, transform the last nvargs arguments
	 * into an array --- unless it's an "any" variadic.
	 */
	if (nvargs > 0 && declared_arg_types[nargs - 1] != ANYOID)
	{
		ArrayExpr  *newa = makeNode(ArrayExpr);
		int			non_var_args = nargs - nvargs;
		List	   *vargs;

		Assert(non_var_args >= 0);
		vargs = list_copy_tail(fargs, non_var_args);
		fargs = list_truncate(fargs, non_var_args);

		newa->elements = vargs;
		/* assume all the variadic arguments were coerced to the same type */
		newa->element_typeid = exprType((Node *) linitial(vargs));
		newa->array_typeid = get_array_type(newa->element_typeid);
		if (!OidIsValid(newa->array_typeid))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("could not find array type for data type %s",
							format_type_be(newa->element_typeid)),
				  parser_errposition(pstate, exprLocation((Node *) vargs))));
		/* array_collid will be set by parse_collate.c */
		newa->multidims = false;
		newa->location = exprLocation((Node *) vargs);

		fargs = lappend(fargs, newa);
	}

	/* build the appropriate output structure */
	if (fdresult == FUNCDETAIL_NORMAL)
	{
		FuncExpr   *funcexpr = makeNode(FuncExpr);

		funcexpr->funcid = funcid;
		funcexpr->funcresulttype = rettype;
		funcexpr->funcretset = retset;
		funcexpr->funcformat = COERCE_EXPLICIT_CALL;
		/* funccollid and inputcollid will be set by parse_collate.c */
		funcexpr->args = fargs;
		funcexpr->location = location;

		retval = (Node *) funcexpr;
	}
	else if (fdresult == FUNCDETAIL_AGGREGATE && !over)
	{
		/* aggregate function */
		Aggref	   *aggref = makeNode(Aggref);

		aggref->aggfnoid = funcid;
		aggref->aggtype = rettype;
		/* aggcollid and inputcollid will be set by parse_collate.c */
		/* args, aggorder, aggdistinct will be set by transformAggregateCall */
		aggref->aggstar = agg_star;
		/* agglevelsup will be set by transformAggregateCall */
		aggref->location = location;

		/*
		 * Reject attempt to call a parameterless aggregate without (*)
		 * syntax.	This is mere pedantry but some folks insisted ...
		 */
		if (fargs == NIL && !agg_star)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("%s(*) must be used to call a parameterless aggregate function",
							NameListToString(funcname)),
					 parser_errposition(pstate, location)));

		if (retset)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("aggregates cannot return sets"),
					 parser_errposition(pstate, location)));

		/*
		 * Currently it's not possible to define an aggregate with named
		 * arguments, so this case should be impossible.  Check anyway because
		 * the planner and executor wouldn't cope with NamedArgExprs in an
		 * Aggref node.
		 */
		if (argnames != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("aggregates cannot use named arguments"),
					 parser_errposition(pstate, location)));

		/* parse_agg.c does additional aggregate-specific processing */
		transformAggregateCall(pstate, aggref, fargs, agg_order, agg_distinct);

		retval = (Node *) aggref;
	}
	else
	{
		/* window function */
		WindowFunc *wfunc = makeNode(WindowFunc);

		/*
		 * True window functions must be called with a window definition.
		 */
		if (!over)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("window function call requires an OVER clause"),
					 parser_errposition(pstate, location)));

		wfunc->winfnoid = funcid;
		wfunc->wintype = rettype;
		/* wincollid and inputcollid will be set by parse_collate.c */
		wfunc->args = fargs;
		/* winref will be set by transformWindowFuncCall */
		wfunc->winstar = agg_star;
		wfunc->winagg = (fdresult == FUNCDETAIL_AGGREGATE);
		wfunc->location = location;

		/*
		 * agg_star is allowed for aggregate functions but distinct isn't
		 */
		if (agg_distinct)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				  errmsg("DISTINCT is not implemented for window functions"),
					 parser_errposition(pstate, location)));

		/*
		 * Reject attempt to call a parameterless aggregate without (*)
		 * syntax.	This is mere pedantry but some folks insisted ...
		 */
		if (wfunc->winagg && fargs == NIL && !agg_star)
			ereport(ERROR,
					(errcode(ERRCODE_WRONG_OBJECT_TYPE),
					 errmsg("%s(*) must be used to call a parameterless aggregate function",
							NameListToString(funcname)),
					 parser_errposition(pstate, location)));

		/*
		 * ordered aggs not allowed in windows yet
		 */
		if (agg_order != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("aggregate ORDER BY is not implemented for window functions"),
					 parser_errposition(pstate, location)));

		if (retset)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_FUNCTION_DEFINITION),
					 errmsg("window functions cannot return sets"),
					 parser_errposition(pstate, location)));

		/*
		 * We might want to support this later, but for now reject it because
		 * the planner and executor wouldn't cope with NamedArgExprs in a
		 * WindowFunc node.
		 */
		if (argnames != NIL)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("window functions cannot use named arguments"),
					 parser_errposition(pstate, location)));

		/* parse_agg.c does additional window-func-specific processing */
		transformWindowFuncCall(pstate, wfunc, over);

		retval = (Node *) wfunc;
	}

	return retval;
}


/* func_match_argtypes()
 *
 * Given a list of candidate functions (having the right name and number
 * of arguments) and an array of input datatype OIDs, produce a shortlist of
 * those candidates that actually accept the input datatypes (either exactly
 * or by coercion), and return the number of such candidates.
 *
 * Note that can_coerce_type will assume that UNKNOWN inputs are coercible to
 * anything, so candidates will not be eliminated on that basis.
 *
 * NB: okay to modify input list structure, as long as we find at least
 * one match.  If no match at all, the list must remain unmodified.
 */
int
func_match_argtypes(int nargs,
					Oid *input_typeids,
					FuncCandidateList raw_candidates,
					FuncCandidateList *candidates)		/* return value */
{
	FuncCandidateList current_candidate;
	FuncCandidateList next_candidate;
	int			ncandidates = 0;

	*candidates = NULL;

	for (current_candidate = raw_candidates;
		 current_candidate != NULL;
		 current_candidate = next_candidate)
	{
		next_candidate = current_candidate->next;
		if (can_coerce_type(nargs, input_typeids, current_candidate->args,
							COERCION_IMPLICIT))
		{
			current_candidate->next = *candidates;
			*candidates = current_candidate;
			ncandidates++;
		}
	}

	return ncandidates;
}	/* func_match_argtypes() */


/* func_select_candidate()
 *		Given the input argtype array and more than one candidate
 *		for the function, attempt to resolve the conflict.
 *
 * Returns the selected candidate if the conflict can be resolved,
 * otherwise returns NULL.
 *
 * Note that the caller has already determined that there is no candidate
 * exactly matching the input argtypes, and has pruned away any "candidates"
 * that aren't actually coercion-compatible with the input types.
 *
 * This is also used for resolving ambiguous operator references.  Formerly
 * parse_oper.c had its own, essentially duplicate code for the purpose.
 * The following comments (formerly in parse_oper.c) are kept to record some
 * of the history of these heuristics.
 *
 * OLD COMMENTS:
 *
 * This routine is new code, replacing binary_oper_select_candidate()
 * which dates from v4.2/v1.0.x days. It tries very hard to match up
 * operators with types, including allowing type coercions if necessary.
 * The important thing is that the code do as much as possible,
 * while _never_ doing the wrong thing, where "the wrong thing" would
 * be returning an operator when other better choices are available,
 * or returning an operator which is a non-intuitive possibility.
 * - thomas 1998-05-21
 *
 * The comments below came from binary_oper_select_candidate(), and
 * illustrate the issues and choices which are possible:
 * - thomas 1998-05-20
 *
 * current wisdom holds that the default operator should be one in which
 * both operands have the same type (there will only be one such
 * operator)
 *
 * 7.27.93 - I have decided not to do this; it's too hard to justify, and
 * it's easy enough to typecast explicitly - avi
 * [the rest of this routine was commented out since then - ay]
 *
 * 6/23/95 - I don't complete agree with avi. In particular, casting
 * floats is a pain for users. Whatever the rationale behind not doing
 * this is, I need the following special case to work.
 *
 * In the WHERE clause of a query, if a float is specified without
 * quotes, we treat it as float8. I added the float48* operators so
 * that we can operate on float4 and float8. But now we have more than
 * one matching operator if the right arg is unknown (eg. float
 * specified with quotes). This break some stuff in the regression
 * test where there are floats in quotes not properly casted. Below is
 * the solution. In addition to requiring the operator operates on the
 * same type for both operands [as in the code Avi originally
 * commented out], we also require that the operators be equivalent in
 * some sense. (see equivalentOpersAfterPromotion for details.)
 * - ay 6/95
 */
FuncCandidateList
func_select_candidate(int nargs,
					  Oid *input_typeids,
					  FuncCandidateList candidates)
{
	FuncCandidateList current_candidate;
	FuncCandidateList last_candidate;
	Oid		   *current_typeids;
	Oid			current_type;
	int			i;
	int			ncandidates;
	int			nbestMatch,
				nmatch;
	Oid			input_base_typeids[FUNC_MAX_ARGS];
	TYPCATEGORY slot_category[FUNC_MAX_ARGS],
				current_category;
	bool		current_is_preferred;
	bool		slot_has_preferred_type[FUNC_MAX_ARGS];
	bool		resolved_unknowns;

	/* protect local fixed-size arrays */
	if (nargs > FUNC_MAX_ARGS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
			 errmsg_plural("cannot pass more than %d argument to a function",
						   "cannot pass more than %d arguments to a function",
						   FUNC_MAX_ARGS,
						   FUNC_MAX_ARGS)));

	/*
	 * If any input types are domains, reduce them to their base types. This
	 * ensures that we will consider functions on the base type to be "exact
	 * matches" in the exact-match heuristic; it also makes it possible to do
	 * something useful with the type-category heuristics. Note that this
	 * makes it difficult, but not impossible, to use functions declared to
	 * take a domain as an input datatype.	Such a function will be selected
	 * over the base-type function only if it is an exact match at all
	 * argument positions, and so was already chosen by our caller.
	 */
	for (i = 0; i < nargs; i++)
		input_base_typeids[i] = getBaseType(input_typeids[i]);

	/*
	 * Run through all candidates and keep those with the most matches on
	 * exact types. Keep all candidates if none match.
	 */
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_base_typeids[i] != UNKNOWNOID &&
				current_typeids[i] == input_base_typeids[i])
				nmatch++;
		}

		/* take this one as the best choice so far? */
		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		/* no worse than the last choice, so keep this one too? */
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
		/* otherwise, don't bother keeping this one... */
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates;

	/*
	 * Still too many candidates? Now look for candidates which have either
	 * exact matches or preferred types at the args that will require
	 * coercion. (Restriction added in 7.4: preferred type must be of same
	 * category as input type; give no preference to cross-category
	 * conversions to preferred types.)  Keep all candidates if none match.
	 */
	for (i = 0; i < nargs; i++) /* avoid multiple lookups */
		slot_category[i] = TypeCategory(input_base_typeids[i]);
	ncandidates = 0;
	nbestMatch = 0;
	last_candidate = NULL;
	for (current_candidate = candidates;
		 current_candidate != NULL;
		 current_candidate = current_candidate->next)
	{
		current_typeids = current_candidate->args;
		nmatch = 0;
		for (i = 0; i < nargs; i++)
		{
			if (input_base_typeids[i] != UNKNOWNOID)
			{
				if (current_typeids[i] == input_base_typeids[i] ||
					IsPreferredType(slot_category[i], current_typeids[i]))
					nmatch++;
			}
		}

		if ((nmatch > nbestMatch) || (last_candidate == NULL))
		{
			nbestMatch = nmatch;
			candidates = current_candidate;
			last_candidate = current_candidate;
			ncandidates = 1;
		}
		else if (nmatch == nbestMatch)
		{
			last_candidate->next = current_candidate;
			last_candidate = current_candidate;
			ncandidates++;
		}
	}

	if (last_candidate)			/* terminate rebuilt list */
		last_candidate->next = NULL;

	if (ncandidates == 1)
		return candidates;

	/*
	 * Still too many candidates? Try assigning types for the unknown columns.
	 *
	 * NOTE: for a binary operator with one unknown and one non-unknown input,
	 * we already tried the heuristic of looking for a candidate with the
	 * known input type on both sides (see binary_oper_exact()). That's
	 * essentially a special case of the general algorithm we try next.
	 *
	 * We do this by examining each unknown argument position to see if we can
	 * determine a "type category" for it.	If any candidate has an input
	 * datatype of STRING category, use STRING category (this bias towards
	 * STRING is appropriate since unknown-type literals look like strings).
	 * Otherwise, if all the candidates agree on the type category of this
	 * argument position, use that category.  Otherwise, fail because we
	 * cannot determine a category.
	 *
	 * If we are able to determine a type category, also notice whether any of
	 * the candidates takes a preferred datatype within the category.
	 *
	 * Having completed this examination, remove candidates that accept the
	 * wrong category at any unknown position.	Also, if at least one
	 * candidate accepted a preferred type at a position, remove candidates
	 * that accept non-preferred types.
	 *
	 * If we are down to one candidate at the end, we win.
	 */
	resolved_unknowns = false;
	for (i = 0; i < nargs; i++)
	{
		bool		have_conflict;

		if (input_base_typeids[i] != UNKNOWNOID)
			continue;
		resolved_unknowns = true;		/* assume we can do it */
		slot_category[i] = TYPCATEGORY_INVALID;
		slot_has_preferred_type[i] = false;
		have_conflict = false;
		for (current_candidate = candidates;
			 current_candidate != NULL;
			 current_candidate = current_candidate->next)
		{
			current_typeids = current_candidate->args;
			current_type = current_typeids[i];
			get_type_category_preferred(current_type,
										&current_category,
										&current_is_preferred);
			if (slot_category[i] == TYPCATEGORY_INVALID)
			{
				/* first candidate */
				slot_category[i] = current_category;
				slot_has_preferred_type[i] = current_is_preferred;
			}
			else if (current_category == slot_category[i])
			{
				/* more candidates in same category */
				slot_has_preferred_type[i] |= current_is_preferred;
			}
			else
			{
				/* category conflict! */
				if (current_category == TYPCATEGORY_STRING)
				{
					/* STRING always wins if available */
					slot_category[i] = current_category;
					slot_has_preferred_type[i] = current_is_preferred;
				}
				else
				{
					/*
					 * Remember conflict, but keep going (might find STRING)
					 */
					have_conflict = true;
				}
			}
		}
		if (have_conflict && slot_category[i] != TYPCATEGORY_STRING)
		{
			/* Failed to resolve category conflict at this position */
			resolved_unknowns = false;
			break;
		}
	}

	if (resolved_unknowns)
	{
		/* Strip non-matching candidates */
		ncandidates = 0;
		last_candidate = NULL;
		for (current_candidate = candidates;
			 current_candidate != NULL;
			 current_candidate = current_candidate->next)
		{
			bool		keepit = true;

			current_typeids = current_candidate->args;
			for (i = 0; i < nargs; i++)
			{
				if (input_base_typeids[i] != UNKNOWNOID)
					continue;
				current_type = current_typeids[i];
				get_type_category_preferred(current_type,
											&current_category,
											&current_is_preferred);
				if (current_category != slot_category[i])
				{
					keepit = false;
					break;
				}
				if (slot_has_preferred_type[i] && !current_is_preferred)
				{
					keepit = false;
					break;
				}
			}
			if (keepit)
			{
				/* keep this candidate */
				last_candidate = current_candidate;
				ncandidates++;
			}
			else
			{
				/* forget this candidate */
				if (last_candidate)
					last_candidate->next = current_candidate->next;
				else
					candidates = current_candidate->next;
			}
		}
		if (last_candidate)		/* terminate rebuilt list */
			last_candidate->next = NULL;
	}

	if (ncandidates == 1)
		return candidates;

	return NULL;				/* failed to select a best candidate */
}	/* func_select_candidate() */


/* func_get_detail()
 *
 * Find the named function in the system catalogs.
 *
 * Attempt to find the named function in the system catalogs with
 * arguments exactly as specified, so that the normal case (exact match)
 * is as quick as possible.
 *
 * If an exact match isn't found:
 *	1) check for possible interpretation as a type coercion request
 *	2) apply the ambiguous-function resolution rules
 *
 * Return values *funcid through *true_typeids receive info about the function.
 * If argdefaults isn't NULL, *argdefaults receives a list of any default
 * argument expressions that need to be added to the given arguments.
 *
 * When processing a named- or mixed-notation call (ie, fargnames isn't NIL),
 * the returned true_typeids and argdefaults are ordered according to the
 * call's argument ordering: first any positional arguments, then the named
 * arguments, then defaulted arguments (if needed and allowed by
 * expand_defaults).  Some care is needed if this information is to be compared
 * to the function's pg_proc entry, but in practice the caller can usually
 * just work with the call's argument ordering.
 *
 * We rely primarily on fargnames/nargs/argtypes as the argument description.
 * The actual expression node list is passed in fargs so that we can check
 * for type coercion of a constant.  Some callers pass fargs == NIL indicating
 * they don't need that check made.  Note also that when fargnames isn't NIL,
 * the fargs list must be passed if the caller wants actual argument position
 * information to be returned into the NamedArgExpr nodes.
 */
FuncDetailCode
func_get_detail(List *funcname,
				List *fargs,
				List *fargnames,
				int nargs,
				Oid *argtypes,
				bool expand_variadic,
				bool expand_defaults,
				Oid *funcid,	/* return value */
				Oid *rettype,	/* return value */
				bool *retset,	/* return value */
				int *nvargs,	/* return value */
				Oid **true_typeids,		/* return value */
				List **argdefaults)		/* optional return value */
{
	FuncCandidateList raw_candidates;
	FuncCandidateList best_candidate;

	/* initialize output arguments to silence compiler warnings */
	*funcid = InvalidOid;
	*rettype = InvalidOid;
	*retset = false;
	*nvargs = 0;
	*true_typeids = NULL;
	if (argdefaults)
		*argdefaults = NIL;

	/* Get list of possible candidates from namespace search */
	raw_candidates = FuncnameGetCandidates(funcname, nargs, fargnames,
										   expand_variadic, expand_defaults);

	/*
	 * Quickly check if there is an exact match to the input datatypes (there
	 * can be only one)
	 */
	for (best_candidate = raw_candidates;
		 best_candidate != NULL;
		 best_candidate = best_candidate->next)
	{
		if (memcmp(argtypes, best_candidate->args, nargs * sizeof(Oid)) == 0)
			break;
	}

	if (best_candidate == NULL)
	{
		/*
		 * If we didn't find an exact match, next consider the possibility
		 * that this is really a type-coercion request: a single-argument
		 * function call where the function name is a type name.  If so, and
		 * if the coercion path is RELABELTYPE or COERCEVIAIO, then go ahead
		 * and treat the "function call" as a coercion.
		 *
		 * This interpretation needs to be given higher priority than
		 * interpretations involving a type coercion followed by a function
		 * call, otherwise we can produce surprising results. For example, we
		 * want "text(varchar)" to be interpreted as a simple coercion, not as
		 * "text(name(varchar))" which the code below this point is entirely
		 * capable of selecting.
		 *
		 * We also treat a coercion of a previously-unknown-type literal
		 * constant to a specific type this way.
		 *
		 * The reason we reject COERCION_PATH_FUNC here is that we expect the
		 * cast implementation function to be named after the target type.
		 * Thus the function will be found by normal lookup if appropriate.
		 *
		 * The reason we reject COERCION_PATH_ARRAYCOERCE is mainly that you
		 * can't write "foo[] (something)" as a function call.  In theory
		 * someone might want to invoke it as "_foo (something)" but we have
		 * never supported that historically, so we can insist that people
		 * write it as a normal cast instead.
		 *
		 * We also reject the specific case of COERCEVIAIO for a composite
		 * source type and a string-category target type.  This is a case that
		 * find_coercion_pathway() allows by default, but experience has shown
		 * that it's too commonly invoked by mistake.  So, again, insist that
		 * people use cast syntax if they want to do that.
		 *
		 * NB: it's important that this code does not exceed what coerce_type
		 * can do, because the caller will try to apply coerce_type if we
		 * return FUNCDETAIL_COERCION.	If we return that result for something
		 * coerce_type can't handle, we'll cause infinite recursion between
		 * this module and coerce_type!
		 */
		if (nargs == 1 && fargs != NIL && fargnames == NIL)
		{
			Oid			targetType = FuncNameAsType(funcname);

			if (OidIsValid(targetType))
			{
				Oid			sourceType = argtypes[0];
				Node	   *arg1 = linitial(fargs);
				bool		iscoercion;

				if (sourceType == UNKNOWNOID && IsA(arg1, Const))
				{
					/* always treat typename('literal') as coercion */
					iscoercion = true;
				}
				else
				{
					CoercionPathType cpathtype;
					Oid			cfuncid;

					cpathtype = find_coercion_pathway(targetType, sourceType,
													  COERCION_EXPLICIT,
													  &cfuncid);
					switch (cpathtype)
					{
						case COERCION_PATH_RELABELTYPE:
							iscoercion = true;
							break;
						case COERCION_PATH_COERCEVIAIO:
							if ((sourceType == RECORDOID ||
								 ISCOMPLEX(sourceType)) &&
							  TypeCategory(targetType) == TYPCATEGORY_STRING)
								iscoercion = false;
							else
								iscoercion = true;
							break;
						default:
							iscoercion = false;
							break;
					}
				}

				if (iscoercion)
				{
					/* Treat it as a type coercion */
					*funcid = InvalidOid;
					*rettype = targetType;
					*retset = false;
					*nvargs = 0;
					*true_typeids = argtypes;
					return FUNCDETAIL_COERCION;
				}
			}
		}

		/*
		 * didn't find an exact match, so now try to match up candidates...
		 */
		if (raw_candidates != NULL)
		{
			FuncCandidateList current_candidates;
			int			ncandidates;

			ncandidates = func_match_argtypes(nargs,
											  argtypes,
											  raw_candidates,
											  &current_candidates);

			/* one match only? then run with it... */
			if (ncandidates == 1)
				best_candidate = current_candidates;

			/*
			 * multiple candidates? then better decide or throw an error...
			 */
			else if (ncandidates > 1)
			{
				best_candidate = func_select_candidate(nargs,
													   argtypes,
													   current_candidates);

				/*
				 * If we were able to choose a best candidate, we're done.
				 * Otherwise, ambiguous function call.
				 */
				if (!best_candidate)
					return FUNCDETAIL_MULTIPLE;
			}
		}
	}

	if (best_candidate)
	{
		HeapTuple	ftup;
		Form_pg_proc pform;
		FuncDetailCode result;

		/*
		 * If processing named args or expanding variadics or defaults, the
		 * "best candidate" might represent multiple equivalently good
		 * functions; treat this case as ambiguous.
		 */
		if (!OidIsValid(best_candidate->oid))
			return FUNCDETAIL_MULTIPLE;

		/*
		 * We disallow VARIADIC with named arguments unless the last argument
		 * (the one with VARIADIC attached) actually matched the variadic
		 * parameter.  This is mere pedantry, really, but some folks insisted.
		 */
		if (fargnames != NIL && !expand_variadic && nargs > 0 &&
			best_candidate->argnumbers[nargs - 1] != nargs - 1)
			return FUNCDETAIL_NOTFOUND;

		*funcid = best_candidate->oid;
		*nvargs = best_candidate->nvargs;
		*true_typeids = best_candidate->args;

		/*
		 * If processing named args, return actual argument positions into
		 * NamedArgExpr nodes in the fargs list.  This is a bit ugly but not
		 * worth the extra notation needed to do it differently.
		 */
		if (best_candidate->argnumbers != NULL)
		{
			int			i = 0;
			ListCell   *lc;

			foreach(lc, fargs)
			{
				NamedArgExpr *na = (NamedArgExpr *) lfirst(lc);

				if (IsA(na, NamedArgExpr))
					na->argnumber = best_candidate->argnumbers[i];
				i++;
			}
		}

		ftup = SearchSysCache1(PROCOID,
							   ObjectIdGetDatum(best_candidate->oid));
		if (!HeapTupleIsValid(ftup))	/* should not happen */
			elog(ERROR, "cache lookup failed for function %u",
				 best_candidate->oid);
		pform = (Form_pg_proc) GETSTRUCT(ftup);
		*rettype = pform->prorettype;
		*retset = pform->proretset;
		/* fetch default args if caller wants 'em */
		if (argdefaults && best_candidate->ndargs > 0)
		{
			Datum		proargdefaults;
			bool		isnull;
			char	   *str;
			List	   *defaults;

			/* shouldn't happen, FuncnameGetCandidates messed up */
			if (best_candidate->ndargs > pform->pronargdefaults)
				elog(ERROR, "not enough default arguments");

			proargdefaults = SysCacheGetAttr(PROCOID, ftup,
											 Anum_pg_proc_proargdefaults,
											 &isnull);
			Assert(!isnull);
			str = TextDatumGetCString(proargdefaults);
			defaults = (List *) stringToNode(str);
			Assert(IsA(defaults, List));
			pfree(str);

			/* Delete any unused defaults from the returned list */
			if (best_candidate->argnumbers != NULL)
			{
				/*
				 * This is a bit tricky in named notation, since the supplied
				 * arguments could replace any subset of the defaults.	We
				 * work by making a bitmapset of the argnumbers of defaulted
				 * arguments, then scanning the defaults list and selecting
				 * the needed items.  (This assumes that defaulted arguments
				 * should be supplied in their positional order.)
				 */
				Bitmapset  *defargnumbers;
				int		   *firstdefarg;
				List	   *newdefaults;
				ListCell   *lc;
				int			i;

				defargnumbers = NULL;
				firstdefarg = &best_candidate->argnumbers[best_candidate->nargs - best_candidate->ndargs];
				for (i = 0; i < best_candidate->ndargs; i++)
					defargnumbers = bms_add_member(defargnumbers,
												   firstdefarg[i]);
				newdefaults = NIL;
				i = pform->pronargs - pform->pronargdefaults;
				foreach(lc, defaults)
				{
					if (bms_is_member(i, defargnumbers))
						newdefaults = lappend(newdefaults, lfirst(lc));
					i++;
				}
				Assert(list_length(newdefaults) == best_candidate->ndargs);
				bms_free(defargnumbers);
				*argdefaults = newdefaults;
			}
			else
			{
				/*
				 * Defaults for positional notation are lots easier; just
				 * remove any unwanted ones from the front.
				 */
				int			ndelete;

				ndelete = list_length(defaults) - best_candidate->ndargs;
				while (ndelete-- > 0)
					defaults = list_delete_first(defaults);
				*argdefaults = defaults;
			}
		}
		if (pform->proisagg)
			result = FUNCDETAIL_AGGREGATE;
		else if (pform->proiswindow)
			result = FUNCDETAIL_WINDOWFUNC;
		else
			result = FUNCDETAIL_NORMAL;
		ReleaseSysCache(ftup);
		return result;
	}

	return FUNCDETAIL_NOTFOUND;
}


/*
 * make_fn_arguments()
 *
 * Given the actual argument expressions for a function, and the desired
 * input types for the function, add any necessary typecasting to the
 * expression tree.  Caller should already have verified that casting is
 * allowed.
 *
 * Caution: given argument list is modified in-place.
 *
 * As with coerce_type, pstate may be NULL if no special unknown-Param
 * processing is wanted.
 */
void
make_fn_arguments(ParseState *pstate,
				  List *fargs,
				  Oid *actual_arg_types,
				  Oid *declared_arg_types)
{
	ListCell   *current_fargs;
	int			i = 0;

	foreach(current_fargs, fargs)
	{
		/* types don't match? then force coercion using a function call... */
		if (actual_arg_types[i] != declared_arg_types[i])
		{
			Node	   *node = (Node *) lfirst(current_fargs);

			/*
			 * If arg is a NamedArgExpr, coerce its input expr instead --- we
			 * want the NamedArgExpr to stay at the top level of the list.
			 */
			if (IsA(node, NamedArgExpr))
			{
				NamedArgExpr *na = (NamedArgExpr *) node;

				node = coerce_type(pstate,
								   (Node *) na->arg,
								   actual_arg_types[i],
								   declared_arg_types[i], -1,
								   COERCION_IMPLICIT,
								   COERCE_IMPLICIT_CAST,
								   -1);
				na->arg = (Expr *) node;
			}
			else
			{
				node = coerce_type(pstate,
								   node,
								   actual_arg_types[i],
								   declared_arg_types[i], -1,
								   COERCION_IMPLICIT,
								   COERCE_IMPLICIT_CAST,
								   -1);
				lfirst(current_fargs) = node;
			}
		}
		i++;
	}
}

/*
 * FuncNameAsType -
 *	  convenience routine to see if a function name matches a type name
 *
 * Returns the OID of the matching type, or InvalidOid if none.  We ignore
 * shell types and complex types.
 */
static Oid
FuncNameAsType(List *funcname)
{
	Oid			result;
	Type		typtup;

	typtup = LookupTypeName(NULL, makeTypeNameFromNameList(funcname), NULL);
	if (typtup == NULL)
		return InvalidOid;

	if (((Form_pg_type) GETSTRUCT(typtup))->typisdefined &&
		!OidIsValid(typeTypeRelid(typtup)))
		result = typeTypeId(typtup);
	else
		result = InvalidOid;

	ReleaseSysCache(typtup);
	return result;
}

/*
 * ParseComplexProjection -
 *	  handles function calls with a single argument that is of complex type.
 *	  If the function call is actually a column projection, return a suitably
 *	  transformed expression tree.	If not, return NULL.
 */
static Node *
ParseComplexProjection(ParseState *pstate, char *funcname, Node *first_arg,
					   int location)
{
	TupleDesc	tupdesc;
	int			i;

	/*
	 * Special case for whole-row Vars so that we can resolve (foo.*).bar even
	 * when foo is a reference to a subselect, join, or RECORD function. A
	 * bonus is that we avoid generating an unnecessary FieldSelect; our
	 * result can omit the whole-row Var and just be a Var for the selected
	 * field.
	 *
	 * This case could be handled by expandRecordVariable, but it's more
	 * efficient to do it this way when possible.
	 */
	if (IsA(first_arg, Var) &&
		((Var *) first_arg)->varattno == InvalidAttrNumber)
	{
		RangeTblEntry *rte;

		rte = GetRTEByRangeTablePosn(pstate,
									 ((Var *) first_arg)->varno,
									 ((Var *) first_arg)->varlevelsup);
		/* Return a Var if funcname matches a column, else NULL */
		return scanRTEForColumn(pstate, rte, funcname, location);
	}

	/*
	 * Else do it the hard way with get_expr_result_type().
	 *
	 * If it's a Var of type RECORD, we have to work even harder: we have to
	 * find what the Var refers to, and pass that to get_expr_result_type.
	 * That task is handled by expandRecordVariable().
	 */
	if (IsA(first_arg, Var) &&
		((Var *) first_arg)->vartype == RECORDOID)
		tupdesc = expandRecordVariable(pstate, (Var *) first_arg, 0);
	else if (get_expr_result_type(first_arg, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		return NULL;			/* unresolvable RECORD type */
	Assert(tupdesc);

	for (i = 0; i < tupdesc->natts; i++)
	{
		Form_pg_attribute att = tupdesc->attrs[i];

		if (strcmp(funcname, NameStr(att->attname)) == 0 &&
			!att->attisdropped)
		{
			/* Success, so generate a FieldSelect expression */
			FieldSelect *fselect = makeNode(FieldSelect);

			fselect->arg = (Expr *) first_arg;
			fselect->fieldnum = i + 1;
			fselect->resulttype = att->atttypid;
			fselect->resulttypmod = att->atttypmod;
			/* save attribute's collation for parse_collate.c */
			fselect->resultcollid = att->attcollation;
			return (Node *) fselect;
		}
	}

	return NULL;				/* funcname does not match any column */
}

/*
 * funcname_signature_string
 *		Build a string representing a function name, including arg types.
 *		The result is something like "foo(integer)".
 *
 * If argnames isn't NIL, it is a list of C strings representing the actual
 * arg names for the last N arguments.	This must be considered part of the
 * function signature too, when dealing with named-notation function calls.
 *
 * This is typically used in the construction of function-not-found error
 * messages.
 */
const char *
funcname_signature_string(const char *funcname, int nargs,
						  List *argnames, const Oid *argtypes)
{
	StringInfoData argbuf;
	int			numposargs;
	ListCell   *lc;
	int			i;

	initStringInfo(&argbuf);

	appendStringInfo(&argbuf, "%s(", funcname);

	numposargs = nargs - list_length(argnames);
	lc = list_head(argnames);

	for (i = 0; i < nargs; i++)
	{
		if (i)
			appendStringInfoString(&argbuf, ", ");
		if (i >= numposargs)
		{
			appendStringInfo(&argbuf, "%s := ", (char *) lfirst(lc));
			lc = lnext(lc);
		}
		appendStringInfoString(&argbuf, format_type_be(argtypes[i]));
	}

	appendStringInfoChar(&argbuf, ')');

	return argbuf.data;			/* return palloc'd string buffer */
}

/*
 * func_signature_string
 *		As above, but function name is passed as a qualified name list.
 */
const char *
func_signature_string(List *funcname, int nargs,
					  List *argnames, const Oid *argtypes)
{
	return funcname_signature_string(NameListToString(funcname),
									 nargs, argnames, argtypes);
}

/*
 * LookupFuncName
 *		Given a possibly-qualified function name and a set of argument types,
 *		look up the function.
 *
 * If the function name is not schema-qualified, it is sought in the current
 * namespace search path.
 *
 * If the function is not found, we return InvalidOid if noError is true,
 * else raise an error.
 */
Oid
LookupFuncName(List *funcname, int nargs, const Oid *argtypes, bool noError)
{
	FuncCandidateList clist;

	clist = FuncnameGetCandidates(funcname, nargs, NIL, false, false);

	while (clist)
	{
		if (memcmp(argtypes, clist->args, nargs * sizeof(Oid)) == 0)
			return clist->oid;
		clist = clist->next;
	}

	if (!noError)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_FUNCTION),
				 errmsg("function %s does not exist",
						func_signature_string(funcname, nargs,
											  NIL, argtypes))));

	return InvalidOid;
}

/*
 * LookupTypeNameOid
 *		Convenience routine to look up a type, silently accepting shell types
 */
static Oid
LookupTypeNameOid(const TypeName *typename)
{
	Oid			result;
	Type		typtup;

	typtup = LookupTypeName(NULL, typename, NULL);
	if (typtup == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("type \"%s\" does not exist",
						TypeNameToString(typename))));
	result = typeTypeId(typtup);
	ReleaseSysCache(typtup);
	return result;
}

/*
 * LookupFuncNameTypeNames
 *		Like LookupFuncName, but the argument types are specified by a
 *		list of TypeName nodes.
 */
Oid
LookupFuncNameTypeNames(List *funcname, List *argtypes, bool noError)
{
	Oid			argoids[FUNC_MAX_ARGS];
	int			argcount;
	int			i;
	ListCell   *args_item;

	argcount = list_length(argtypes);
	if (argcount > FUNC_MAX_ARGS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
				 errmsg_plural("functions cannot have more than %d argument",
							   "functions cannot have more than %d arguments",
							   FUNC_MAX_ARGS,
							   FUNC_MAX_ARGS)));

	args_item = list_head(argtypes);
	for (i = 0; i < argcount; i++)
	{
		TypeName   *t = (TypeName *) lfirst(args_item);

		argoids[i] = LookupTypeNameOid(t);
		args_item = lnext(args_item);
	}

	return LookupFuncName(funcname, argcount, argoids, noError);
}

/*
 * LookupAggNameTypeNames
 *		Find an aggregate function given a name and list of TypeName nodes.
 *
 * This is almost like LookupFuncNameTypeNames, but the error messages refer
 * to aggregates rather than plain functions, and we verify that the found
 * function really is an aggregate.
 */
Oid
LookupAggNameTypeNames(List *aggname, List *argtypes, bool noError)
{
	Oid			argoids[FUNC_MAX_ARGS];
	int			argcount;
	int			i;
	ListCell   *lc;
	Oid			oid;
	HeapTuple	ftup;
	Form_pg_proc pform;

	argcount = list_length(argtypes);
	if (argcount > FUNC_MAX_ARGS)
		ereport(ERROR,
				(errcode(ERRCODE_TOO_MANY_ARGUMENTS),
				 errmsg_plural("functions cannot have more than %d argument",
							   "functions cannot have more than %d arguments",
							   FUNC_MAX_ARGS,
							   FUNC_MAX_ARGS)));

	i = 0;
	foreach(lc, argtypes)
	{
		TypeName   *t = (TypeName *) lfirst(lc);

		argoids[i] = LookupTypeNameOid(t);
		i++;
	}

	oid = LookupFuncName(aggname, argcount, argoids, true);

	if (!OidIsValid(oid))
	{
		if (noError)
			return InvalidOid;
		if (argcount == 0)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("aggregate %s(*) does not exist",
							NameListToString(aggname))));
		else
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_FUNCTION),
					 errmsg("aggregate %s does not exist",
							func_signature_string(aggname, argcount,
												  NIL, argoids))));
	}

	/* Make sure it's an aggregate */
	ftup = SearchSysCache1(PROCOID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(ftup))	/* should not happen */
		elog(ERROR, "cache lookup failed for function %u", oid);
	pform = (Form_pg_proc) GETSTRUCT(ftup);

	if (!pform->proisagg)
	{
		ReleaseSysCache(ftup);
		if (noError)
			return InvalidOid;
		/* we do not use the (*) notation for functions... */
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("function %s is not an aggregate",
						func_signature_string(aggname, argcount,
											  NIL, argoids))));
	}

	ReleaseSysCache(ftup);

	return oid;
}
