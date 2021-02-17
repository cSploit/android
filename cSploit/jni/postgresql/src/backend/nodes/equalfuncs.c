/*-------------------------------------------------------------------------
 *
 * equalfuncs.c
 *	  Equality functions to compare node trees.
 *
 * NOTE: we currently support comparing all node types found in parse
 * trees.  We do not support comparing executor state trees; there
 * is no need for that, and no point in maintaining all the code that
 * would be needed.  We also do not support comparing Path trees, mainly
 * because the circular linkages between RelOptInfo and Path nodes can't
 * be handled easily in a simple depth-first traversal.
 *
 * Currently, in fact, equal() doesn't know how to compare Plan trees
 * either.	This might need to be fixed someday.
 *
 * NOTE: it is intentional that parse location fields (in nodes that have
 * one) are not compared.  This is because we want, for example, a variable
 * "x" to be considered equal() to another reference to "x" in the query.
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/nodes/equalfuncs.c
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "nodes/relation.h"
#include "utils/datum.h"


/*
 * Macros to simplify comparison of different kinds of fields.	Use these
 * wherever possible to reduce the chance for silly typos.	Note that these
 * hard-wire the convention that the local variables in an Equal routine are
 * named 'a' and 'b'.
 */

/* Compare a simple scalar field (int, float, bool, enum, etc) */
#define COMPARE_SCALAR_FIELD(fldname) \
	do { \
		if (a->fldname != b->fldname) \
			return false; \
	} while (0)

/* Compare a field that is a pointer to some kind of Node or Node tree */
#define COMPARE_NODE_FIELD(fldname) \
	do { \
		if (!equal(a->fldname, b->fldname)) \
			return false; \
	} while (0)

/* Compare a field that is a pointer to a Bitmapset */
#define COMPARE_BITMAPSET_FIELD(fldname) \
	do { \
		if (!bms_equal(a->fldname, b->fldname)) \
			return false; \
	} while (0)

/* Compare a field that is a pointer to a C string, or perhaps NULL */
#define COMPARE_STRING_FIELD(fldname) \
	do { \
		if (!equalstr(a->fldname, b->fldname)) \
			return false; \
	} while (0)

/* Macro for comparing string fields that might be NULL */
#define equalstr(a, b)	\
	(((a) != NULL && (b) != NULL) ? (strcmp(a, b) == 0) : (a) == (b))

/* Compare a field that is a pointer to a simple palloc'd object of size sz */
#define COMPARE_POINTER_FIELD(fldname, sz) \
	do { \
		if (memcmp(a->fldname, b->fldname, (sz)) != 0) \
			return false; \
	} while (0)

/* Compare a parse location field (this is a no-op, per note above) */
#define COMPARE_LOCATION_FIELD(fldname) \
	((void) 0)


/*
 *	Stuff from primnodes.h
 */

static bool
_equalAlias(Alias *a, Alias *b)
{
	COMPARE_STRING_FIELD(aliasname);
	COMPARE_NODE_FIELD(colnames);

	return true;
}

static bool
_equalRangeVar(RangeVar *a, RangeVar *b)
{
	COMPARE_STRING_FIELD(catalogname);
	COMPARE_STRING_FIELD(schemaname);
	COMPARE_STRING_FIELD(relname);
	COMPARE_SCALAR_FIELD(inhOpt);
	COMPARE_SCALAR_FIELD(relpersistence);
	COMPARE_NODE_FIELD(alias);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalIntoClause(IntoClause *a, IntoClause *b)
{
	COMPARE_NODE_FIELD(rel);
	COMPARE_NODE_FIELD(colNames);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(onCommit);
	COMPARE_STRING_FIELD(tableSpaceName);

	return true;
}

/*
 * We don't need an _equalExpr because Expr is an abstract supertype which
 * should never actually get instantiated.	Also, since it has no common
 * fields except NodeTag, there's no need for a helper routine to factor
 * out comparing the common fields...
 */

static bool
_equalVar(Var *a, Var *b)
{
	COMPARE_SCALAR_FIELD(varno);
	COMPARE_SCALAR_FIELD(varattno);
	COMPARE_SCALAR_FIELD(vartype);
	COMPARE_SCALAR_FIELD(vartypmod);
	COMPARE_SCALAR_FIELD(varcollid);
	COMPARE_SCALAR_FIELD(varlevelsup);
	COMPARE_SCALAR_FIELD(varnoold);
	COMPARE_SCALAR_FIELD(varoattno);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalConst(Const *a, Const *b)
{
	COMPARE_SCALAR_FIELD(consttype);
	COMPARE_SCALAR_FIELD(consttypmod);
	COMPARE_SCALAR_FIELD(constcollid);
	COMPARE_SCALAR_FIELD(constlen);
	COMPARE_SCALAR_FIELD(constisnull);
	COMPARE_SCALAR_FIELD(constbyval);
	COMPARE_LOCATION_FIELD(location);

	/*
	 * We treat all NULL constants of the same type as equal. Someday this
	 * might need to change?  But datumIsEqual doesn't work on nulls, so...
	 */
	if (a->constisnull)
		return true;
	return datumIsEqual(a->constvalue, b->constvalue,
						a->constbyval, a->constlen);
}

static bool
_equalParam(Param *a, Param *b)
{
	COMPARE_SCALAR_FIELD(paramkind);
	COMPARE_SCALAR_FIELD(paramid);
	COMPARE_SCALAR_FIELD(paramtype);
	COMPARE_SCALAR_FIELD(paramtypmod);
	COMPARE_SCALAR_FIELD(paramcollid);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalAggref(Aggref *a, Aggref *b)
{
	COMPARE_SCALAR_FIELD(aggfnoid);
	COMPARE_SCALAR_FIELD(aggtype);
	COMPARE_SCALAR_FIELD(aggcollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_NODE_FIELD(aggorder);
	COMPARE_NODE_FIELD(aggdistinct);
	COMPARE_SCALAR_FIELD(aggstar);
	COMPARE_SCALAR_FIELD(agglevelsup);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalWindowFunc(WindowFunc *a, WindowFunc *b)
{
	COMPARE_SCALAR_FIELD(winfnoid);
	COMPARE_SCALAR_FIELD(wintype);
	COMPARE_SCALAR_FIELD(wincollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(winref);
	COMPARE_SCALAR_FIELD(winstar);
	COMPARE_SCALAR_FIELD(winagg);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalArrayRef(ArrayRef *a, ArrayRef *b)
{
	COMPARE_SCALAR_FIELD(refarraytype);
	COMPARE_SCALAR_FIELD(refelemtype);
	COMPARE_SCALAR_FIELD(reftypmod);
	COMPARE_SCALAR_FIELD(refcollid);
	COMPARE_NODE_FIELD(refupperindexpr);
	COMPARE_NODE_FIELD(reflowerindexpr);
	COMPARE_NODE_FIELD(refexpr);
	COMPARE_NODE_FIELD(refassgnexpr);

	return true;
}

static bool
_equalFuncExpr(FuncExpr *a, FuncExpr *b)
{
	COMPARE_SCALAR_FIELD(funcid);
	COMPARE_SCALAR_FIELD(funcresulttype);
	COMPARE_SCALAR_FIELD(funcretset);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->funcformat != b->funcformat &&
		a->funcformat != COERCE_DONTCARE &&
		b->funcformat != COERCE_DONTCARE)
		return false;

	COMPARE_SCALAR_FIELD(funccollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalNamedArgExpr(NamedArgExpr *a, NamedArgExpr *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_STRING_FIELD(name);
	COMPARE_SCALAR_FIELD(argnumber);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalOpExpr(OpExpr *a, OpExpr *b)
{
	COMPARE_SCALAR_FIELD(opno);

	/*
	 * Special-case opfuncid: it is allowable for it to differ if one node
	 * contains zero and the other doesn't.  This just means that the one node
	 * isn't as far along in the parse/plan pipeline and hasn't had the
	 * opfuncid cache filled yet.
	 */
	if (a->opfuncid != b->opfuncid &&
		a->opfuncid != 0 &&
		b->opfuncid != 0)
		return false;

	COMPARE_SCALAR_FIELD(opresulttype);
	COMPARE_SCALAR_FIELD(opretset);
	COMPARE_SCALAR_FIELD(opcollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalDistinctExpr(DistinctExpr *a, DistinctExpr *b)
{
	COMPARE_SCALAR_FIELD(opno);

	/*
	 * Special-case opfuncid: it is allowable for it to differ if one node
	 * contains zero and the other doesn't.  This just means that the one node
	 * isn't as far along in the parse/plan pipeline and hasn't had the
	 * opfuncid cache filled yet.
	 */
	if (a->opfuncid != b->opfuncid &&
		a->opfuncid != 0 &&
		b->opfuncid != 0)
		return false;

	COMPARE_SCALAR_FIELD(opresulttype);
	COMPARE_SCALAR_FIELD(opretset);
	COMPARE_SCALAR_FIELD(opcollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalNullIfExpr(NullIfExpr *a, NullIfExpr *b)
{
	COMPARE_SCALAR_FIELD(opno);

	/*
	 * Special-case opfuncid: it is allowable for it to differ if one node
	 * contains zero and the other doesn't.  This just means that the one node
	 * isn't as far along in the parse/plan pipeline and hasn't had the
	 * opfuncid cache filled yet.
	 */
	if (a->opfuncid != b->opfuncid &&
		a->opfuncid != 0 &&
		b->opfuncid != 0)
		return false;

	COMPARE_SCALAR_FIELD(opresulttype);
	COMPARE_SCALAR_FIELD(opretset);
	COMPARE_SCALAR_FIELD(opcollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalScalarArrayOpExpr(ScalarArrayOpExpr *a, ScalarArrayOpExpr *b)
{
	COMPARE_SCALAR_FIELD(opno);

	/*
	 * Special-case opfuncid: it is allowable for it to differ if one node
	 * contains zero and the other doesn't.  This just means that the one node
	 * isn't as far along in the parse/plan pipeline and hasn't had the
	 * opfuncid cache filled yet.
	 */
	if (a->opfuncid != b->opfuncid &&
		a->opfuncid != 0 &&
		b->opfuncid != 0)
		return false;

	COMPARE_SCALAR_FIELD(useOr);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalBoolExpr(BoolExpr *a, BoolExpr *b)
{
	COMPARE_SCALAR_FIELD(boolop);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalSubLink(SubLink *a, SubLink *b)
{
	COMPARE_SCALAR_FIELD(subLinkType);
	COMPARE_NODE_FIELD(testexpr);
	COMPARE_NODE_FIELD(operName);
	COMPARE_NODE_FIELD(subselect);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalSubPlan(SubPlan *a, SubPlan *b)
{
	COMPARE_SCALAR_FIELD(subLinkType);
	COMPARE_NODE_FIELD(testexpr);
	COMPARE_NODE_FIELD(paramIds);
	COMPARE_SCALAR_FIELD(plan_id);
	COMPARE_STRING_FIELD(plan_name);
	COMPARE_SCALAR_FIELD(firstColType);
	COMPARE_SCALAR_FIELD(firstColTypmod);
	COMPARE_SCALAR_FIELD(firstColCollation);
	COMPARE_SCALAR_FIELD(useHashTable);
	COMPARE_SCALAR_FIELD(unknownEqFalse);
	COMPARE_NODE_FIELD(setParam);
	COMPARE_NODE_FIELD(parParam);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(startup_cost);
	COMPARE_SCALAR_FIELD(per_call_cost);

	return true;
}

static bool
_equalAlternativeSubPlan(AlternativeSubPlan *a, AlternativeSubPlan *b)
{
	COMPARE_NODE_FIELD(subplans);

	return true;
}

static bool
_equalFieldSelect(FieldSelect *a, FieldSelect *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(fieldnum);
	COMPARE_SCALAR_FIELD(resulttype);
	COMPARE_SCALAR_FIELD(resulttypmod);
	COMPARE_SCALAR_FIELD(resultcollid);

	return true;
}

static bool
_equalFieldStore(FieldStore *a, FieldStore *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_NODE_FIELD(newvals);
	COMPARE_NODE_FIELD(fieldnums);
	COMPARE_SCALAR_FIELD(resulttype);

	return true;
}

static bool
_equalRelabelType(RelabelType *a, RelabelType *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(resulttype);
	COMPARE_SCALAR_FIELD(resulttypmod);
	COMPARE_SCALAR_FIELD(resultcollid);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->relabelformat != b->relabelformat &&
		a->relabelformat != COERCE_DONTCARE &&
		b->relabelformat != COERCE_DONTCARE)
		return false;

	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCoerceViaIO(CoerceViaIO *a, CoerceViaIO *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(resulttype);
	COMPARE_SCALAR_FIELD(resultcollid);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->coerceformat != b->coerceformat &&
		a->coerceformat != COERCE_DONTCARE &&
		b->coerceformat != COERCE_DONTCARE)
		return false;

	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalArrayCoerceExpr(ArrayCoerceExpr *a, ArrayCoerceExpr *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(elemfuncid);
	COMPARE_SCALAR_FIELD(resulttype);
	COMPARE_SCALAR_FIELD(resulttypmod);
	COMPARE_SCALAR_FIELD(resultcollid);
	COMPARE_SCALAR_FIELD(isExplicit);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->coerceformat != b->coerceformat &&
		a->coerceformat != COERCE_DONTCARE &&
		b->coerceformat != COERCE_DONTCARE)
		return false;

	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalConvertRowtypeExpr(ConvertRowtypeExpr *a, ConvertRowtypeExpr *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(resulttype);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->convertformat != b->convertformat &&
		a->convertformat != COERCE_DONTCARE &&
		b->convertformat != COERCE_DONTCARE)
		return false;

	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCollateExpr(CollateExpr *a, CollateExpr *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(collOid);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCaseExpr(CaseExpr *a, CaseExpr *b)
{
	COMPARE_SCALAR_FIELD(casetype);
	COMPARE_SCALAR_FIELD(casecollid);
	COMPARE_NODE_FIELD(arg);
	COMPARE_NODE_FIELD(args);
	COMPARE_NODE_FIELD(defresult);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCaseWhen(CaseWhen *a, CaseWhen *b)
{
	COMPARE_NODE_FIELD(expr);
	COMPARE_NODE_FIELD(result);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCaseTestExpr(CaseTestExpr *a, CaseTestExpr *b)
{
	COMPARE_SCALAR_FIELD(typeId);
	COMPARE_SCALAR_FIELD(typeMod);
	COMPARE_SCALAR_FIELD(collation);

	return true;
}

static bool
_equalArrayExpr(ArrayExpr *a, ArrayExpr *b)
{
	COMPARE_SCALAR_FIELD(array_typeid);
	COMPARE_SCALAR_FIELD(array_collid);
	COMPARE_SCALAR_FIELD(element_typeid);
	COMPARE_NODE_FIELD(elements);
	COMPARE_SCALAR_FIELD(multidims);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalRowExpr(RowExpr *a, RowExpr *b)
{
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(row_typeid);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->row_format != b->row_format &&
		a->row_format != COERCE_DONTCARE &&
		b->row_format != COERCE_DONTCARE)
		return false;

	COMPARE_NODE_FIELD(colnames);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalRowCompareExpr(RowCompareExpr *a, RowCompareExpr *b)
{
	COMPARE_SCALAR_FIELD(rctype);
	COMPARE_NODE_FIELD(opnos);
	COMPARE_NODE_FIELD(opfamilies);
	COMPARE_NODE_FIELD(inputcollids);
	COMPARE_NODE_FIELD(largs);
	COMPARE_NODE_FIELD(rargs);

	return true;
}

static bool
_equalCoalesceExpr(CoalesceExpr *a, CoalesceExpr *b)
{
	COMPARE_SCALAR_FIELD(coalescetype);
	COMPARE_SCALAR_FIELD(coalescecollid);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalMinMaxExpr(MinMaxExpr *a, MinMaxExpr *b)
{
	COMPARE_SCALAR_FIELD(minmaxtype);
	COMPARE_SCALAR_FIELD(minmaxcollid);
	COMPARE_SCALAR_FIELD(inputcollid);
	COMPARE_SCALAR_FIELD(op);
	COMPARE_NODE_FIELD(args);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalXmlExpr(XmlExpr *a, XmlExpr *b)
{
	COMPARE_SCALAR_FIELD(op);
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(named_args);
	COMPARE_NODE_FIELD(arg_names);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(xmloption);
	COMPARE_SCALAR_FIELD(type);
	COMPARE_SCALAR_FIELD(typmod);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalNullTest(NullTest *a, NullTest *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(nulltesttype);
	COMPARE_SCALAR_FIELD(argisrow);

	return true;
}

static bool
_equalBooleanTest(BooleanTest *a, BooleanTest *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(booltesttype);

	return true;
}

static bool
_equalCoerceToDomain(CoerceToDomain *a, CoerceToDomain *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(resulttype);
	COMPARE_SCALAR_FIELD(resulttypmod);
	COMPARE_SCALAR_FIELD(resultcollid);

	/*
	 * Special-case COERCE_DONTCARE, so that planner can build coercion nodes
	 * that are equal() to both explicit and implicit coercions.
	 */
	if (a->coercionformat != b->coercionformat &&
		a->coercionformat != COERCE_DONTCARE &&
		b->coercionformat != COERCE_DONTCARE)
		return false;

	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCoerceToDomainValue(CoerceToDomainValue *a, CoerceToDomainValue *b)
{
	COMPARE_SCALAR_FIELD(typeId);
	COMPARE_SCALAR_FIELD(typeMod);
	COMPARE_SCALAR_FIELD(collation);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalSetToDefault(SetToDefault *a, SetToDefault *b)
{
	COMPARE_SCALAR_FIELD(typeId);
	COMPARE_SCALAR_FIELD(typeMod);
	COMPARE_SCALAR_FIELD(collation);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCurrentOfExpr(CurrentOfExpr *a, CurrentOfExpr *b)
{
	COMPARE_SCALAR_FIELD(cvarno);
	COMPARE_STRING_FIELD(cursor_name);
	COMPARE_SCALAR_FIELD(cursor_param);

	return true;
}

static bool
_equalTargetEntry(TargetEntry *a, TargetEntry *b)
{
	COMPARE_NODE_FIELD(expr);
	COMPARE_SCALAR_FIELD(resno);
	COMPARE_STRING_FIELD(resname);
	COMPARE_SCALAR_FIELD(ressortgroupref);
	COMPARE_SCALAR_FIELD(resorigtbl);
	COMPARE_SCALAR_FIELD(resorigcol);
	COMPARE_SCALAR_FIELD(resjunk);

	return true;
}

static bool
_equalRangeTblRef(RangeTblRef *a, RangeTblRef *b)
{
	COMPARE_SCALAR_FIELD(rtindex);

	return true;
}

static bool
_equalJoinExpr(JoinExpr *a, JoinExpr *b)
{
	COMPARE_SCALAR_FIELD(jointype);
	COMPARE_SCALAR_FIELD(isNatural);
	COMPARE_NODE_FIELD(larg);
	COMPARE_NODE_FIELD(rarg);
	COMPARE_NODE_FIELD(usingClause);
	COMPARE_NODE_FIELD(quals);
	COMPARE_NODE_FIELD(alias);
	COMPARE_SCALAR_FIELD(rtindex);

	return true;
}

static bool
_equalFromExpr(FromExpr *a, FromExpr *b)
{
	COMPARE_NODE_FIELD(fromlist);
	COMPARE_NODE_FIELD(quals);

	return true;
}


/*
 * Stuff from relation.h
 */

static bool
_equalPathKey(PathKey *a, PathKey *b)
{
	/*
	 * This is normally used on non-canonicalized PathKeys, so must chase up
	 * to the topmost merged EquivalenceClass and see if those are the same
	 * (by pointer equality).
	 */
	EquivalenceClass *a_eclass;
	EquivalenceClass *b_eclass;

	a_eclass = a->pk_eclass;
	while (a_eclass->ec_merged)
		a_eclass = a_eclass->ec_merged;
	b_eclass = b->pk_eclass;
	while (b_eclass->ec_merged)
		b_eclass = b_eclass->ec_merged;
	if (a_eclass != b_eclass)
		return false;
	COMPARE_SCALAR_FIELD(pk_opfamily);
	COMPARE_SCALAR_FIELD(pk_strategy);
	COMPARE_SCALAR_FIELD(pk_nulls_first);

	return true;
}

static bool
_equalRestrictInfo(RestrictInfo *a, RestrictInfo *b)
{
	COMPARE_NODE_FIELD(clause);
	COMPARE_SCALAR_FIELD(is_pushed_down);
	COMPARE_SCALAR_FIELD(outerjoin_delayed);
	COMPARE_BITMAPSET_FIELD(required_relids);
	COMPARE_BITMAPSET_FIELD(nullable_relids);

	/*
	 * We ignore all the remaining fields, since they may not be set yet, and
	 * should be derivable from the clause anyway.
	 */

	return true;
}

static bool
_equalPlaceHolderVar(PlaceHolderVar *a, PlaceHolderVar *b)
{
	/*
	 * We intentionally do not compare phexpr.	Two PlaceHolderVars with the
	 * same ID and levelsup should be considered equal even if the contained
	 * expressions have managed to mutate to different states.	One way in
	 * which that can happen is that initplan sublinks would get replaced by
	 * differently-numbered Params when sublink folding is done.  (The end
	 * result of such a situation would be some unreferenced initplans, which
	 * is annoying but not really a problem.)
	 *
	 * COMPARE_NODE_FIELD(phexpr);
	 */
	COMPARE_BITMAPSET_FIELD(phrels);
	COMPARE_SCALAR_FIELD(phid);
	COMPARE_SCALAR_FIELD(phlevelsup);

	return true;
}

static bool
_equalSpecialJoinInfo(SpecialJoinInfo *a, SpecialJoinInfo *b)
{
	COMPARE_BITMAPSET_FIELD(min_lefthand);
	COMPARE_BITMAPSET_FIELD(min_righthand);
	COMPARE_BITMAPSET_FIELD(syn_lefthand);
	COMPARE_BITMAPSET_FIELD(syn_righthand);
	COMPARE_SCALAR_FIELD(jointype);
	COMPARE_SCALAR_FIELD(lhs_strict);
	COMPARE_SCALAR_FIELD(delay_upper_joins);
	COMPARE_NODE_FIELD(join_quals);

	return true;
}

static bool
_equalAppendRelInfo(AppendRelInfo *a, AppendRelInfo *b)
{
	COMPARE_SCALAR_FIELD(parent_relid);
	COMPARE_SCALAR_FIELD(child_relid);
	COMPARE_SCALAR_FIELD(parent_reltype);
	COMPARE_SCALAR_FIELD(child_reltype);
	COMPARE_NODE_FIELD(translated_vars);
	COMPARE_SCALAR_FIELD(parent_reloid);

	return true;
}

static bool
_equalPlaceHolderInfo(PlaceHolderInfo *a, PlaceHolderInfo *b)
{
	COMPARE_SCALAR_FIELD(phid);
	COMPARE_NODE_FIELD(ph_var);
	COMPARE_BITMAPSET_FIELD(ph_eval_at);
	COMPARE_BITMAPSET_FIELD(ph_needed);
	COMPARE_BITMAPSET_FIELD(ph_may_need);
	COMPARE_SCALAR_FIELD(ph_width);

	return true;
}


/*
 * Stuff from parsenodes.h
 */

static bool
_equalQuery(Query *a, Query *b)
{
	COMPARE_SCALAR_FIELD(commandType);
	COMPARE_SCALAR_FIELD(querySource);
	COMPARE_SCALAR_FIELD(canSetTag);
	COMPARE_NODE_FIELD(utilityStmt);
	COMPARE_SCALAR_FIELD(resultRelation);
	COMPARE_NODE_FIELD(intoClause);
	COMPARE_SCALAR_FIELD(hasAggs);
	COMPARE_SCALAR_FIELD(hasWindowFuncs);
	COMPARE_SCALAR_FIELD(hasSubLinks);
	COMPARE_SCALAR_FIELD(hasDistinctOn);
	COMPARE_SCALAR_FIELD(hasRecursive);
	COMPARE_SCALAR_FIELD(hasModifyingCTE);
	COMPARE_SCALAR_FIELD(hasForUpdate);
	COMPARE_NODE_FIELD(cteList);
	COMPARE_NODE_FIELD(rtable);
	COMPARE_NODE_FIELD(jointree);
	COMPARE_NODE_FIELD(targetList);
	COMPARE_NODE_FIELD(returningList);
	COMPARE_NODE_FIELD(groupClause);
	COMPARE_NODE_FIELD(havingQual);
	COMPARE_NODE_FIELD(windowClause);
	COMPARE_NODE_FIELD(distinctClause);
	COMPARE_NODE_FIELD(sortClause);
	COMPARE_NODE_FIELD(limitOffset);
	COMPARE_NODE_FIELD(limitCount);
	COMPARE_NODE_FIELD(rowMarks);
	COMPARE_NODE_FIELD(setOperations);
	COMPARE_NODE_FIELD(constraintDeps);

	return true;
}

static bool
_equalInsertStmt(InsertStmt *a, InsertStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(cols);
	COMPARE_NODE_FIELD(selectStmt);
	COMPARE_NODE_FIELD(returningList);
	COMPARE_NODE_FIELD(withClause);

	return true;
}

static bool
_equalDeleteStmt(DeleteStmt *a, DeleteStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(usingClause);
	COMPARE_NODE_FIELD(whereClause);
	COMPARE_NODE_FIELD(returningList);
	COMPARE_NODE_FIELD(withClause);

	return true;
}

static bool
_equalUpdateStmt(UpdateStmt *a, UpdateStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(targetList);
	COMPARE_NODE_FIELD(whereClause);
	COMPARE_NODE_FIELD(fromClause);
	COMPARE_NODE_FIELD(returningList);
	COMPARE_NODE_FIELD(withClause);

	return true;
}

static bool
_equalSelectStmt(SelectStmt *a, SelectStmt *b)
{
	COMPARE_NODE_FIELD(distinctClause);
	COMPARE_NODE_FIELD(intoClause);
	COMPARE_NODE_FIELD(targetList);
	COMPARE_NODE_FIELD(fromClause);
	COMPARE_NODE_FIELD(whereClause);
	COMPARE_NODE_FIELD(groupClause);
	COMPARE_NODE_FIELD(havingClause);
	COMPARE_NODE_FIELD(windowClause);
	COMPARE_NODE_FIELD(withClause);
	COMPARE_NODE_FIELD(valuesLists);
	COMPARE_NODE_FIELD(sortClause);
	COMPARE_NODE_FIELD(limitOffset);
	COMPARE_NODE_FIELD(limitCount);
	COMPARE_NODE_FIELD(lockingClause);
	COMPARE_SCALAR_FIELD(op);
	COMPARE_SCALAR_FIELD(all);
	COMPARE_NODE_FIELD(larg);
	COMPARE_NODE_FIELD(rarg);

	return true;
}

static bool
_equalSetOperationStmt(SetOperationStmt *a, SetOperationStmt *b)
{
	COMPARE_SCALAR_FIELD(op);
	COMPARE_SCALAR_FIELD(all);
	COMPARE_NODE_FIELD(larg);
	COMPARE_NODE_FIELD(rarg);
	COMPARE_NODE_FIELD(colTypes);
	COMPARE_NODE_FIELD(colTypmods);
	COMPARE_NODE_FIELD(colCollations);
	COMPARE_NODE_FIELD(groupClauses);

	return true;
}

static bool
_equalAlterTableStmt(AlterTableStmt *a, AlterTableStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(cmds);
	COMPARE_SCALAR_FIELD(relkind);

	return true;
}

static bool
_equalAlterTableCmd(AlterTableCmd *a, AlterTableCmd *b)
{
	COMPARE_SCALAR_FIELD(subtype);
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(def);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalAlterDomainStmt(AlterDomainStmt *a, AlterDomainStmt *b)
{
	COMPARE_SCALAR_FIELD(subtype);
	COMPARE_NODE_FIELD(typeName);
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(def);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalGrantStmt(GrantStmt *a, GrantStmt *b)
{
	COMPARE_SCALAR_FIELD(is_grant);
	COMPARE_SCALAR_FIELD(targtype);
	COMPARE_SCALAR_FIELD(objtype);
	COMPARE_NODE_FIELD(objects);
	COMPARE_NODE_FIELD(privileges);
	COMPARE_NODE_FIELD(grantees);
	COMPARE_SCALAR_FIELD(grant_option);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalPrivGrantee(PrivGrantee *a, PrivGrantee *b)
{
	COMPARE_STRING_FIELD(rolname);

	return true;
}

static bool
_equalFuncWithArgs(FuncWithArgs *a, FuncWithArgs *b)
{
	COMPARE_NODE_FIELD(funcname);
	COMPARE_NODE_FIELD(funcargs);

	return true;
}

static bool
_equalAccessPriv(AccessPriv *a, AccessPriv *b)
{
	COMPARE_STRING_FIELD(priv_name);
	COMPARE_NODE_FIELD(cols);

	return true;
}

static bool
_equalGrantRoleStmt(GrantRoleStmt *a, GrantRoleStmt *b)
{
	COMPARE_NODE_FIELD(granted_roles);
	COMPARE_NODE_FIELD(grantee_roles);
	COMPARE_SCALAR_FIELD(is_grant);
	COMPARE_SCALAR_FIELD(admin_opt);
	COMPARE_STRING_FIELD(grantor);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalAlterDefaultPrivilegesStmt(AlterDefaultPrivilegesStmt *a, AlterDefaultPrivilegesStmt *b)
{
	COMPARE_NODE_FIELD(options);
	COMPARE_NODE_FIELD(action);

	return true;
}

static bool
_equalDeclareCursorStmt(DeclareCursorStmt *a, DeclareCursorStmt *b)
{
	COMPARE_STRING_FIELD(portalname);
	COMPARE_SCALAR_FIELD(options);
	COMPARE_NODE_FIELD(query);

	return true;
}

static bool
_equalClosePortalStmt(ClosePortalStmt *a, ClosePortalStmt *b)
{
	COMPARE_STRING_FIELD(portalname);

	return true;
}

static bool
_equalClusterStmt(ClusterStmt *a, ClusterStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_STRING_FIELD(indexname);
	COMPARE_SCALAR_FIELD(verbose);

	return true;
}

static bool
_equalCopyStmt(CopyStmt *a, CopyStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(query);
	COMPARE_NODE_FIELD(attlist);
	COMPARE_SCALAR_FIELD(is_from);
	COMPARE_STRING_FIELD(filename);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalCreateStmt(CreateStmt *a, CreateStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(tableElts);
	COMPARE_NODE_FIELD(inhRelations);
	COMPARE_NODE_FIELD(ofTypename);
	COMPARE_NODE_FIELD(constraints);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(oncommit);
	COMPARE_STRING_FIELD(tablespacename);
	COMPARE_SCALAR_FIELD(if_not_exists);

	return true;
}

static bool
_equalInhRelation(InhRelation *a, InhRelation *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_SCALAR_FIELD(options);

	return true;
}

static bool
_equalDefineStmt(DefineStmt *a, DefineStmt *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_SCALAR_FIELD(oldstyle);
	COMPARE_NODE_FIELD(defnames);
	COMPARE_NODE_FIELD(args);
	COMPARE_NODE_FIELD(definition);

	return true;
}

static bool
_equalDropStmt(DropStmt *a, DropStmt *b)
{
	COMPARE_NODE_FIELD(objects);
	COMPARE_SCALAR_FIELD(removeType);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalTruncateStmt(TruncateStmt *a, TruncateStmt *b)
{
	COMPARE_NODE_FIELD(relations);
	COMPARE_SCALAR_FIELD(restart_seqs);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalCommentStmt(CommentStmt *a, CommentStmt *b)
{
	COMPARE_SCALAR_FIELD(objtype);
	COMPARE_NODE_FIELD(objname);
	COMPARE_NODE_FIELD(objargs);
	COMPARE_STRING_FIELD(comment);

	return true;
}

static bool
_equalSecLabelStmt(SecLabelStmt *a, SecLabelStmt *b)
{
	COMPARE_SCALAR_FIELD(objtype);
	COMPARE_NODE_FIELD(objname);
	COMPARE_NODE_FIELD(objargs);
	COMPARE_STRING_FIELD(provider);
	COMPARE_STRING_FIELD(label);

	return true;
}

static bool
_equalFetchStmt(FetchStmt *a, FetchStmt *b)
{
	COMPARE_SCALAR_FIELD(direction);
	COMPARE_SCALAR_FIELD(howMany);
	COMPARE_STRING_FIELD(portalname);
	COMPARE_SCALAR_FIELD(ismove);

	return true;
}

static bool
_equalIndexStmt(IndexStmt *a, IndexStmt *b)
{
	COMPARE_STRING_FIELD(idxname);
	COMPARE_NODE_FIELD(relation);
	COMPARE_STRING_FIELD(accessMethod);
	COMPARE_STRING_FIELD(tableSpace);
	COMPARE_NODE_FIELD(indexParams);
	COMPARE_NODE_FIELD(options);
	COMPARE_NODE_FIELD(whereClause);
	COMPARE_NODE_FIELD(excludeOpNames);
	COMPARE_SCALAR_FIELD(indexOid);
	COMPARE_SCALAR_FIELD(unique);
	COMPARE_SCALAR_FIELD(primary);
	COMPARE_SCALAR_FIELD(isconstraint);
	COMPARE_SCALAR_FIELD(deferrable);
	COMPARE_SCALAR_FIELD(initdeferred);
	COMPARE_SCALAR_FIELD(concurrent);

	return true;
}

static bool
_equalCreateFunctionStmt(CreateFunctionStmt *a, CreateFunctionStmt *b)
{
	COMPARE_SCALAR_FIELD(replace);
	COMPARE_NODE_FIELD(funcname);
	COMPARE_NODE_FIELD(parameters);
	COMPARE_NODE_FIELD(returnType);
	COMPARE_NODE_FIELD(options);
	COMPARE_NODE_FIELD(withClause);

	return true;
}

static bool
_equalFunctionParameter(FunctionParameter *a, FunctionParameter *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(argType);
	COMPARE_SCALAR_FIELD(mode);
	COMPARE_NODE_FIELD(defexpr);

	return true;
}

static bool
_equalAlterFunctionStmt(AlterFunctionStmt *a, AlterFunctionStmt *b)
{
	COMPARE_NODE_FIELD(func);
	COMPARE_NODE_FIELD(actions);

	return true;
}

static bool
_equalRemoveFuncStmt(RemoveFuncStmt *a, RemoveFuncStmt *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_NODE_FIELD(name);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalDoStmt(DoStmt *a, DoStmt *b)
{
	COMPARE_NODE_FIELD(args);

	return true;
}

static bool
_equalRemoveOpClassStmt(RemoveOpClassStmt *a, RemoveOpClassStmt *b)
{
	COMPARE_NODE_FIELD(opclassname);
	COMPARE_STRING_FIELD(amname);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalRemoveOpFamilyStmt(RemoveOpFamilyStmt *a, RemoveOpFamilyStmt *b)
{
	COMPARE_NODE_FIELD(opfamilyname);
	COMPARE_STRING_FIELD(amname);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalRenameStmt(RenameStmt *a, RenameStmt *b)
{
	COMPARE_SCALAR_FIELD(renameType);
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(object);
	COMPARE_NODE_FIELD(objarg);
	COMPARE_STRING_FIELD(subname);
	COMPARE_STRING_FIELD(newname);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalAlterObjectSchemaStmt(AlterObjectSchemaStmt *a, AlterObjectSchemaStmt *b)
{
	COMPARE_SCALAR_FIELD(objectType);
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(object);
	COMPARE_NODE_FIELD(objarg);
	COMPARE_STRING_FIELD(addname);
	COMPARE_STRING_FIELD(newschema);

	return true;
}

static bool
_equalAlterOwnerStmt(AlterOwnerStmt *a, AlterOwnerStmt *b)
{
	COMPARE_SCALAR_FIELD(objectType);
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(object);
	COMPARE_NODE_FIELD(objarg);
	COMPARE_STRING_FIELD(addname);
	COMPARE_STRING_FIELD(newowner);

	return true;
}

static bool
_equalRuleStmt(RuleStmt *a, RuleStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_STRING_FIELD(rulename);
	COMPARE_NODE_FIELD(whereClause);
	COMPARE_SCALAR_FIELD(event);
	COMPARE_SCALAR_FIELD(instead);
	COMPARE_NODE_FIELD(actions);
	COMPARE_SCALAR_FIELD(replace);

	return true;
}

static bool
_equalNotifyStmt(NotifyStmt *a, NotifyStmt *b)
{
	COMPARE_STRING_FIELD(conditionname);
	COMPARE_STRING_FIELD(payload);

	return true;
}

static bool
_equalListenStmt(ListenStmt *a, ListenStmt *b)
{
	COMPARE_STRING_FIELD(conditionname);

	return true;
}

static bool
_equalUnlistenStmt(UnlistenStmt *a, UnlistenStmt *b)
{
	COMPARE_STRING_FIELD(conditionname);

	return true;
}

static bool
_equalTransactionStmt(TransactionStmt *a, TransactionStmt *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_NODE_FIELD(options);
	COMPARE_STRING_FIELD(gid);

	return true;
}

static bool
_equalCompositeTypeStmt(CompositeTypeStmt *a, CompositeTypeStmt *b)
{
	COMPARE_NODE_FIELD(typevar);
	COMPARE_NODE_FIELD(coldeflist);

	return true;
}

static bool
_equalCreateEnumStmt(CreateEnumStmt *a, CreateEnumStmt *b)
{
	COMPARE_NODE_FIELD(typeName);
	COMPARE_NODE_FIELD(vals);

	return true;
}

static bool
_equalAlterEnumStmt(AlterEnumStmt *a, AlterEnumStmt *b)
{
	COMPARE_NODE_FIELD(typeName);
	COMPARE_STRING_FIELD(newVal);
	COMPARE_STRING_FIELD(newValNeighbor);
	COMPARE_SCALAR_FIELD(newValIsAfter);

	return true;
}

static bool
_equalViewStmt(ViewStmt *a, ViewStmt *b)
{
	COMPARE_NODE_FIELD(view);
	COMPARE_NODE_FIELD(aliases);
	COMPARE_NODE_FIELD(query);
	COMPARE_SCALAR_FIELD(replace);

	return true;
}

static bool
_equalLoadStmt(LoadStmt *a, LoadStmt *b)
{
	COMPARE_STRING_FIELD(filename);

	return true;
}

static bool
_equalCreateDomainStmt(CreateDomainStmt *a, CreateDomainStmt *b)
{
	COMPARE_NODE_FIELD(domainname);
	COMPARE_NODE_FIELD(typeName);
	COMPARE_NODE_FIELD(collClause);
	COMPARE_NODE_FIELD(constraints);

	return true;
}

static bool
_equalCreateOpClassStmt(CreateOpClassStmt *a, CreateOpClassStmt *b)
{
	COMPARE_NODE_FIELD(opclassname);
	COMPARE_NODE_FIELD(opfamilyname);
	COMPARE_STRING_FIELD(amname);
	COMPARE_NODE_FIELD(datatype);
	COMPARE_NODE_FIELD(items);
	COMPARE_SCALAR_FIELD(isDefault);

	return true;
}

static bool
_equalCreateOpClassItem(CreateOpClassItem *a, CreateOpClassItem *b)
{
	COMPARE_SCALAR_FIELD(itemtype);
	COMPARE_NODE_FIELD(name);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(number);
	COMPARE_NODE_FIELD(order_family);
	COMPARE_NODE_FIELD(class_args);
	COMPARE_NODE_FIELD(storedtype);

	return true;
}

static bool
_equalCreateOpFamilyStmt(CreateOpFamilyStmt *a, CreateOpFamilyStmt *b)
{
	COMPARE_NODE_FIELD(opfamilyname);
	COMPARE_STRING_FIELD(amname);

	return true;
}

static bool
_equalAlterOpFamilyStmt(AlterOpFamilyStmt *a, AlterOpFamilyStmt *b)
{
	COMPARE_NODE_FIELD(opfamilyname);
	COMPARE_STRING_FIELD(amname);
	COMPARE_SCALAR_FIELD(isDrop);
	COMPARE_NODE_FIELD(items);

	return true;
}

static bool
_equalCreatedbStmt(CreatedbStmt *a, CreatedbStmt *b)
{
	COMPARE_STRING_FIELD(dbname);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterDatabaseStmt(AlterDatabaseStmt *a, AlterDatabaseStmt *b)
{
	COMPARE_STRING_FIELD(dbname);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterDatabaseSetStmt(AlterDatabaseSetStmt *a, AlterDatabaseSetStmt *b)
{
	COMPARE_STRING_FIELD(dbname);
	COMPARE_NODE_FIELD(setstmt);

	return true;
}

static bool
_equalDropdbStmt(DropdbStmt *a, DropdbStmt *b)
{
	COMPARE_STRING_FIELD(dbname);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalVacuumStmt(VacuumStmt *a, VacuumStmt *b)
{
	COMPARE_SCALAR_FIELD(options);
	COMPARE_SCALAR_FIELD(freeze_min_age);
	COMPARE_SCALAR_FIELD(freeze_table_age);
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(va_cols);

	return true;
}

static bool
_equalExplainStmt(ExplainStmt *a, ExplainStmt *b)
{
	COMPARE_NODE_FIELD(query);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalCreateSeqStmt(CreateSeqStmt *a, CreateSeqStmt *b)
{
	COMPARE_NODE_FIELD(sequence);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(ownerId);

	return true;
}

static bool
_equalAlterSeqStmt(AlterSeqStmt *a, AlterSeqStmt *b)
{
	COMPARE_NODE_FIELD(sequence);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalVariableSetStmt(VariableSetStmt *a, VariableSetStmt *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(is_local);

	return true;
}

static bool
_equalVariableShowStmt(VariableShowStmt *a, VariableShowStmt *b)
{
	COMPARE_STRING_FIELD(name);

	return true;
}

static bool
_equalDiscardStmt(DiscardStmt *a, DiscardStmt *b)
{
	COMPARE_SCALAR_FIELD(target);

	return true;
}

static bool
_equalCreateTableSpaceStmt(CreateTableSpaceStmt *a, CreateTableSpaceStmt *b)
{
	COMPARE_STRING_FIELD(tablespacename);
	COMPARE_STRING_FIELD(owner);
	COMPARE_STRING_FIELD(location);

	return true;
}

static bool
_equalDropTableSpaceStmt(DropTableSpaceStmt *a, DropTableSpaceStmt *b)
{
	COMPARE_STRING_FIELD(tablespacename);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalAlterTableSpaceOptionsStmt(AlterTableSpaceOptionsStmt *a,
								 AlterTableSpaceOptionsStmt *b)
{
	COMPARE_STRING_FIELD(tablespacename);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(isReset);

	return true;
}

static bool
_equalCreateExtensionStmt(CreateExtensionStmt *a, CreateExtensionStmt *b)
{
	COMPARE_STRING_FIELD(extname);
	COMPARE_SCALAR_FIELD(if_not_exists);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterExtensionStmt(AlterExtensionStmt *a, AlterExtensionStmt *b)
{
	COMPARE_STRING_FIELD(extname);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterExtensionContentsStmt(AlterExtensionContentsStmt *a, AlterExtensionContentsStmt *b)
{
	COMPARE_STRING_FIELD(extname);
	COMPARE_SCALAR_FIELD(action);
	COMPARE_SCALAR_FIELD(objtype);
	COMPARE_NODE_FIELD(objname);
	COMPARE_NODE_FIELD(objargs);

	return true;
}

static bool
_equalCreateFdwStmt(CreateFdwStmt *a, CreateFdwStmt *b)
{
	COMPARE_STRING_FIELD(fdwname);
	COMPARE_NODE_FIELD(func_options);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterFdwStmt(AlterFdwStmt *a, AlterFdwStmt *b)
{
	COMPARE_STRING_FIELD(fdwname);
	COMPARE_NODE_FIELD(func_options);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalDropFdwStmt(DropFdwStmt *a, DropFdwStmt *b)
{
	COMPARE_STRING_FIELD(fdwname);
	COMPARE_SCALAR_FIELD(missing_ok);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalCreateForeignServerStmt(CreateForeignServerStmt *a, CreateForeignServerStmt *b)
{
	COMPARE_STRING_FIELD(servername);
	COMPARE_STRING_FIELD(servertype);
	COMPARE_STRING_FIELD(version);
	COMPARE_STRING_FIELD(fdwname);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterForeignServerStmt(AlterForeignServerStmt *a, AlterForeignServerStmt *b)
{
	COMPARE_STRING_FIELD(servername);
	COMPARE_STRING_FIELD(version);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(has_version);

	return true;
}

static bool
_equalDropForeignServerStmt(DropForeignServerStmt *a, DropForeignServerStmt *b)
{
	COMPARE_STRING_FIELD(servername);
	COMPARE_SCALAR_FIELD(missing_ok);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalCreateUserMappingStmt(CreateUserMappingStmt *a, CreateUserMappingStmt *b)
{
	COMPARE_STRING_FIELD(username);
	COMPARE_STRING_FIELD(servername);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterUserMappingStmt(AlterUserMappingStmt *a, AlterUserMappingStmt *b)
{
	COMPARE_STRING_FIELD(username);
	COMPARE_STRING_FIELD(servername);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalDropUserMappingStmt(DropUserMappingStmt *a, DropUserMappingStmt *b)
{
	COMPARE_STRING_FIELD(username);
	COMPARE_STRING_FIELD(servername);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalCreateForeignTableStmt(CreateForeignTableStmt *a, CreateForeignTableStmt *b)
{
	if (!_equalCreateStmt(&a->base, &b->base))
		return false;

	COMPARE_STRING_FIELD(servername);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalCreateTrigStmt(CreateTrigStmt *a, CreateTrigStmt *b)
{
	COMPARE_STRING_FIELD(trigname);
	COMPARE_NODE_FIELD(relation);
	COMPARE_NODE_FIELD(funcname);
	COMPARE_NODE_FIELD(args);
	COMPARE_SCALAR_FIELD(row);
	COMPARE_SCALAR_FIELD(timing);
	COMPARE_SCALAR_FIELD(events);
	COMPARE_NODE_FIELD(columns);
	COMPARE_NODE_FIELD(whenClause);
	COMPARE_SCALAR_FIELD(isconstraint);
	COMPARE_SCALAR_FIELD(deferrable);
	COMPARE_SCALAR_FIELD(initdeferred);
	COMPARE_NODE_FIELD(constrrel);

	return true;
}

static bool
_equalDropPropertyStmt(DropPropertyStmt *a, DropPropertyStmt *b)
{
	COMPARE_NODE_FIELD(relation);
	COMPARE_STRING_FIELD(property);
	COMPARE_SCALAR_FIELD(removeType);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalCreatePLangStmt(CreatePLangStmt *a, CreatePLangStmt *b)
{
	COMPARE_SCALAR_FIELD(replace);
	COMPARE_STRING_FIELD(plname);
	COMPARE_NODE_FIELD(plhandler);
	COMPARE_NODE_FIELD(plinline);
	COMPARE_NODE_FIELD(plvalidator);
	COMPARE_SCALAR_FIELD(pltrusted);

	return true;
}

static bool
_equalDropPLangStmt(DropPLangStmt *a, DropPLangStmt *b)
{
	COMPARE_STRING_FIELD(plname);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalCreateRoleStmt(CreateRoleStmt *a, CreateRoleStmt *b)
{
	COMPARE_SCALAR_FIELD(stmt_type);
	COMPARE_STRING_FIELD(role);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterRoleStmt(AlterRoleStmt *a, AlterRoleStmt *b)
{
	COMPARE_STRING_FIELD(role);
	COMPARE_NODE_FIELD(options);
	COMPARE_SCALAR_FIELD(action);

	return true;
}

static bool
_equalAlterRoleSetStmt(AlterRoleSetStmt *a, AlterRoleSetStmt *b)
{
	COMPARE_STRING_FIELD(role);
	COMPARE_STRING_FIELD(database);
	COMPARE_NODE_FIELD(setstmt);

	return true;
}

static bool
_equalDropRoleStmt(DropRoleStmt *a, DropRoleStmt *b)
{
	COMPARE_NODE_FIELD(roles);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalLockStmt(LockStmt *a, LockStmt *b)
{
	COMPARE_NODE_FIELD(relations);
	COMPARE_SCALAR_FIELD(mode);
	COMPARE_SCALAR_FIELD(nowait);

	return true;
}

static bool
_equalConstraintsSetStmt(ConstraintsSetStmt *a, ConstraintsSetStmt *b)
{
	COMPARE_NODE_FIELD(constraints);
	COMPARE_SCALAR_FIELD(deferred);

	return true;
}

static bool
_equalReindexStmt(ReindexStmt *a, ReindexStmt *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_NODE_FIELD(relation);
	COMPARE_STRING_FIELD(name);
	COMPARE_SCALAR_FIELD(do_system);
	COMPARE_SCALAR_FIELD(do_user);

	return true;
}

static bool
_equalCreateSchemaStmt(CreateSchemaStmt *a, CreateSchemaStmt *b)
{
	COMPARE_STRING_FIELD(schemaname);
	COMPARE_STRING_FIELD(authid);
	COMPARE_NODE_FIELD(schemaElts);

	return true;
}

static bool
_equalCreateConversionStmt(CreateConversionStmt *a, CreateConversionStmt *b)
{
	COMPARE_NODE_FIELD(conversion_name);
	COMPARE_STRING_FIELD(for_encoding_name);
	COMPARE_STRING_FIELD(to_encoding_name);
	COMPARE_NODE_FIELD(func_name);
	COMPARE_SCALAR_FIELD(def);

	return true;
}

static bool
_equalCreateCastStmt(CreateCastStmt *a, CreateCastStmt *b)
{
	COMPARE_NODE_FIELD(sourcetype);
	COMPARE_NODE_FIELD(targettype);
	COMPARE_NODE_FIELD(func);
	COMPARE_SCALAR_FIELD(context);
	COMPARE_SCALAR_FIELD(inout);

	return true;
}

static bool
_equalDropCastStmt(DropCastStmt *a, DropCastStmt *b)
{
	COMPARE_NODE_FIELD(sourcetype);
	COMPARE_NODE_FIELD(targettype);
	COMPARE_SCALAR_FIELD(behavior);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalPrepareStmt(PrepareStmt *a, PrepareStmt *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(argtypes);
	COMPARE_NODE_FIELD(query);

	return true;
}

static bool
_equalExecuteStmt(ExecuteStmt *a, ExecuteStmt *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(into);
	COMPARE_NODE_FIELD(params);

	return true;
}

static bool
_equalDeallocateStmt(DeallocateStmt *a, DeallocateStmt *b)
{
	COMPARE_STRING_FIELD(name);

	return true;
}

static bool
_equalDropOwnedStmt(DropOwnedStmt *a, DropOwnedStmt *b)
{
	COMPARE_NODE_FIELD(roles);
	COMPARE_SCALAR_FIELD(behavior);

	return true;
}

static bool
_equalReassignOwnedStmt(ReassignOwnedStmt *a, ReassignOwnedStmt *b)
{
	COMPARE_NODE_FIELD(roles);
	COMPARE_STRING_FIELD(newrole);

	return true;
}

static bool
_equalAlterTSDictionaryStmt(AlterTSDictionaryStmt *a, AlterTSDictionaryStmt *b)
{
	COMPARE_NODE_FIELD(dictname);
	COMPARE_NODE_FIELD(options);

	return true;
}

static bool
_equalAlterTSConfigurationStmt(AlterTSConfigurationStmt *a,
							   AlterTSConfigurationStmt *b)
{
	COMPARE_NODE_FIELD(cfgname);
	COMPARE_NODE_FIELD(tokentype);
	COMPARE_NODE_FIELD(dicts);
	COMPARE_SCALAR_FIELD(override);
	COMPARE_SCALAR_FIELD(replace);
	COMPARE_SCALAR_FIELD(missing_ok);

	return true;
}

static bool
_equalAExpr(A_Expr *a, A_Expr *b)
{
	COMPARE_SCALAR_FIELD(kind);
	COMPARE_NODE_FIELD(name);
	COMPARE_NODE_FIELD(lexpr);
	COMPARE_NODE_FIELD(rexpr);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalColumnRef(ColumnRef *a, ColumnRef *b)
{
	COMPARE_NODE_FIELD(fields);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalParamRef(ParamRef *a, ParamRef *b)
{
	COMPARE_SCALAR_FIELD(number);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalAConst(A_Const *a, A_Const *b)
{
	if (!equal(&a->val, &b->val))		/* hack for in-line Value field */
		return false;
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalFuncCall(FuncCall *a, FuncCall *b)
{
	COMPARE_NODE_FIELD(funcname);
	COMPARE_NODE_FIELD(args);
	COMPARE_NODE_FIELD(agg_order);
	COMPARE_SCALAR_FIELD(agg_star);
	COMPARE_SCALAR_FIELD(agg_distinct);
	COMPARE_SCALAR_FIELD(func_variadic);
	COMPARE_NODE_FIELD(over);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalAStar(A_Star *a, A_Star *b)
{
	return true;
}

static bool
_equalAIndices(A_Indices *a, A_Indices *b)
{
	COMPARE_NODE_FIELD(lidx);
	COMPARE_NODE_FIELD(uidx);

	return true;
}

static bool
_equalA_Indirection(A_Indirection *a, A_Indirection *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_NODE_FIELD(indirection);

	return true;
}

static bool
_equalA_ArrayExpr(A_ArrayExpr *a, A_ArrayExpr *b)
{
	COMPARE_NODE_FIELD(elements);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalResTarget(ResTarget *a, ResTarget *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(indirection);
	COMPARE_NODE_FIELD(val);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalTypeName(TypeName *a, TypeName *b)
{
	COMPARE_NODE_FIELD(names);
	COMPARE_SCALAR_FIELD(typeOid);
	COMPARE_SCALAR_FIELD(setof);
	COMPARE_SCALAR_FIELD(pct_type);
	COMPARE_NODE_FIELD(typmods);
	COMPARE_SCALAR_FIELD(typemod);
	COMPARE_NODE_FIELD(arrayBounds);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalTypeCast(TypeCast *a, TypeCast *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_NODE_FIELD(typeName);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCollateClause(CollateClause *a, CollateClause *b)
{
	COMPARE_NODE_FIELD(arg);
	COMPARE_NODE_FIELD(collname);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalSortBy(SortBy *a, SortBy *b)
{
	COMPARE_NODE_FIELD(node);
	COMPARE_SCALAR_FIELD(sortby_dir);
	COMPARE_SCALAR_FIELD(sortby_nulls);
	COMPARE_NODE_FIELD(useOp);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalWindowDef(WindowDef *a, WindowDef *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_STRING_FIELD(refname);
	COMPARE_NODE_FIELD(partitionClause);
	COMPARE_NODE_FIELD(orderClause);
	COMPARE_SCALAR_FIELD(frameOptions);
	COMPARE_NODE_FIELD(startOffset);
	COMPARE_NODE_FIELD(endOffset);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalRangeSubselect(RangeSubselect *a, RangeSubselect *b)
{
	COMPARE_NODE_FIELD(subquery);
	COMPARE_NODE_FIELD(alias);

	return true;
}

static bool
_equalRangeFunction(RangeFunction *a, RangeFunction *b)
{
	COMPARE_NODE_FIELD(funccallnode);
	COMPARE_NODE_FIELD(alias);
	COMPARE_NODE_FIELD(coldeflist);

	return true;
}

static bool
_equalIndexElem(IndexElem *a, IndexElem *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_NODE_FIELD(expr);
	COMPARE_STRING_FIELD(indexcolname);
	COMPARE_NODE_FIELD(collation);
	COMPARE_NODE_FIELD(opclass);
	COMPARE_SCALAR_FIELD(ordering);
	COMPARE_SCALAR_FIELD(nulls_ordering);

	return true;
}

static bool
_equalColumnDef(ColumnDef *a, ColumnDef *b)
{
	COMPARE_STRING_FIELD(colname);
	COMPARE_NODE_FIELD(typeName);
	COMPARE_SCALAR_FIELD(inhcount);
	COMPARE_SCALAR_FIELD(is_local);
	COMPARE_SCALAR_FIELD(is_not_null);
	COMPARE_SCALAR_FIELD(is_from_type);
	COMPARE_SCALAR_FIELD(storage);
	COMPARE_NODE_FIELD(raw_default);
	COMPARE_NODE_FIELD(cooked_default);
	COMPARE_NODE_FIELD(collClause);
	COMPARE_SCALAR_FIELD(collOid);
	COMPARE_NODE_FIELD(constraints);

	return true;
}

static bool
_equalConstraint(Constraint *a, Constraint *b)
{
	COMPARE_SCALAR_FIELD(contype);
	COMPARE_STRING_FIELD(conname);
	COMPARE_SCALAR_FIELD(deferrable);
	COMPARE_SCALAR_FIELD(initdeferred);
	COMPARE_LOCATION_FIELD(location);
	COMPARE_NODE_FIELD(raw_expr);
	COMPARE_STRING_FIELD(cooked_expr);
	COMPARE_NODE_FIELD(keys);
	COMPARE_NODE_FIELD(exclusions);
	COMPARE_NODE_FIELD(options);
	COMPARE_STRING_FIELD(indexname);
	COMPARE_STRING_FIELD(indexspace);
	COMPARE_STRING_FIELD(access_method);
	COMPARE_NODE_FIELD(where_clause);
	COMPARE_NODE_FIELD(pktable);
	COMPARE_NODE_FIELD(fk_attrs);
	COMPARE_NODE_FIELD(pk_attrs);
	COMPARE_SCALAR_FIELD(fk_matchtype);
	COMPARE_SCALAR_FIELD(fk_upd_action);
	COMPARE_SCALAR_FIELD(fk_del_action);
	COMPARE_SCALAR_FIELD(skip_validation);
	COMPARE_SCALAR_FIELD(initially_valid);

	return true;
}

static bool
_equalDefElem(DefElem *a, DefElem *b)
{
	COMPARE_STRING_FIELD(defnamespace);
	COMPARE_STRING_FIELD(defname);
	COMPARE_NODE_FIELD(arg);
	COMPARE_SCALAR_FIELD(defaction);

	return true;
}

static bool
_equalLockingClause(LockingClause *a, LockingClause *b)
{
	COMPARE_NODE_FIELD(lockedRels);
	COMPARE_SCALAR_FIELD(forUpdate);
	COMPARE_SCALAR_FIELD(noWait);

	return true;
}

static bool
_equalRangeTblEntry(RangeTblEntry *a, RangeTblEntry *b)
{
	COMPARE_SCALAR_FIELD(rtekind);
	COMPARE_SCALAR_FIELD(relid);
	COMPARE_SCALAR_FIELD(relkind);
	COMPARE_NODE_FIELD(subquery);
	COMPARE_SCALAR_FIELD(jointype);
	COMPARE_NODE_FIELD(joinaliasvars);
	COMPARE_NODE_FIELD(funcexpr);
	COMPARE_NODE_FIELD(funccoltypes);
	COMPARE_NODE_FIELD(funccoltypmods);
	COMPARE_NODE_FIELD(funccolcollations);
	COMPARE_NODE_FIELD(values_lists);
	COMPARE_NODE_FIELD(values_collations);
	COMPARE_STRING_FIELD(ctename);
	COMPARE_SCALAR_FIELD(ctelevelsup);
	COMPARE_SCALAR_FIELD(self_reference);
	COMPARE_NODE_FIELD(ctecoltypes);
	COMPARE_NODE_FIELD(ctecoltypmods);
	COMPARE_NODE_FIELD(ctecolcollations);
	COMPARE_NODE_FIELD(alias);
	COMPARE_NODE_FIELD(eref);
	COMPARE_SCALAR_FIELD(inh);
	COMPARE_SCALAR_FIELD(inFromCl);
	COMPARE_SCALAR_FIELD(requiredPerms);
	COMPARE_SCALAR_FIELD(checkAsUser);
	COMPARE_BITMAPSET_FIELD(selectedCols);
	COMPARE_BITMAPSET_FIELD(modifiedCols);

	return true;
}

static bool
_equalSortGroupClause(SortGroupClause *a, SortGroupClause *b)
{
	COMPARE_SCALAR_FIELD(tleSortGroupRef);
	COMPARE_SCALAR_FIELD(eqop);
	COMPARE_SCALAR_FIELD(sortop);
	COMPARE_SCALAR_FIELD(nulls_first);
	COMPARE_SCALAR_FIELD(hashable);

	return true;
}

static bool
_equalWindowClause(WindowClause *a, WindowClause *b)
{
	COMPARE_STRING_FIELD(name);
	COMPARE_STRING_FIELD(refname);
	COMPARE_NODE_FIELD(partitionClause);
	COMPARE_NODE_FIELD(orderClause);
	COMPARE_SCALAR_FIELD(frameOptions);
	COMPARE_NODE_FIELD(startOffset);
	COMPARE_NODE_FIELD(endOffset);
	COMPARE_SCALAR_FIELD(winref);
	COMPARE_SCALAR_FIELD(copiedOrder);

	return true;
}

static bool
_equalRowMarkClause(RowMarkClause *a, RowMarkClause *b)
{
	COMPARE_SCALAR_FIELD(rti);
	COMPARE_SCALAR_FIELD(forUpdate);
	COMPARE_SCALAR_FIELD(noWait);
	COMPARE_SCALAR_FIELD(pushedDown);

	return true;
}

static bool
_equalWithClause(WithClause *a, WithClause *b)
{
	COMPARE_NODE_FIELD(ctes);
	COMPARE_SCALAR_FIELD(recursive);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

static bool
_equalCommonTableExpr(CommonTableExpr *a, CommonTableExpr *b)
{
	COMPARE_STRING_FIELD(ctename);
	COMPARE_NODE_FIELD(aliascolnames);
	COMPARE_NODE_FIELD(ctequery);
	COMPARE_LOCATION_FIELD(location);
	COMPARE_SCALAR_FIELD(cterecursive);
	COMPARE_SCALAR_FIELD(cterefcount);
	COMPARE_NODE_FIELD(ctecolnames);
	COMPARE_NODE_FIELD(ctecoltypes);
	COMPARE_NODE_FIELD(ctecoltypmods);
	COMPARE_NODE_FIELD(ctecolcollations);

	return true;
}

static bool
_equalXmlSerialize(XmlSerialize *a, XmlSerialize *b)
{
	COMPARE_SCALAR_FIELD(xmloption);
	COMPARE_NODE_FIELD(expr);
	COMPARE_NODE_FIELD(typeName);
	COMPARE_LOCATION_FIELD(location);

	return true;
}

/*
 * Stuff from pg_list.h
 */

static bool
_equalList(List *a, List *b)
{
	ListCell   *item_a;
	ListCell   *item_b;

	/*
	 * Try to reject by simple scalar checks before grovelling through all the
	 * list elements...
	 */
	COMPARE_SCALAR_FIELD(type);
	COMPARE_SCALAR_FIELD(length);

	/*
	 * We place the switch outside the loop for the sake of efficiency; this
	 * may not be worth doing...
	 */
	switch (a->type)
	{
		case T_List:
			forboth(item_a, a, item_b, b)
			{
				if (!equal(lfirst(item_a), lfirst(item_b)))
					return false;
			}
			break;
		case T_IntList:
			forboth(item_a, a, item_b, b)
			{
				if (lfirst_int(item_a) != lfirst_int(item_b))
					return false;
			}
			break;
		case T_OidList:
			forboth(item_a, a, item_b, b)
			{
				if (lfirst_oid(item_a) != lfirst_oid(item_b))
					return false;
			}
			break;
		default:
			elog(ERROR, "unrecognized list node type: %d",
				 (int) a->type);
			return false;		/* keep compiler quiet */
	}

	/*
	 * If we got here, we should have run out of elements of both lists
	 */
	Assert(item_a == NULL);
	Assert(item_b == NULL);

	return true;
}

/*
 * Stuff from value.h
 */

static bool
_equalValue(Value *a, Value *b)
{
	COMPARE_SCALAR_FIELD(type);

	switch (a->type)
	{
		case T_Integer:
			COMPARE_SCALAR_FIELD(val.ival);
			break;
		case T_Float:
		case T_String:
		case T_BitString:
			COMPARE_STRING_FIELD(val.str);
			break;
		case T_Null:
			/* nothing to do */
			break;
		default:
			elog(ERROR, "unrecognized node type: %d", (int) a->type);
			break;
	}

	return true;
}

/*
 * equal
 *	  returns whether two nodes are equal
 */
bool
equal(void *a, void *b)
{
	bool		retval;

	if (a == b)
		return true;

	/*
	 * note that a!=b, so only one of them can be NULL
	 */
	if (a == NULL || b == NULL)
		return false;

	/*
	 * are they the same type of nodes?
	 */
	if (nodeTag(a) != nodeTag(b))
		return false;

	switch (nodeTag(a))
	{
			/*
			 * PRIMITIVE NODES
			 */
		case T_Alias:
			retval = _equalAlias(a, b);
			break;
		case T_RangeVar:
			retval = _equalRangeVar(a, b);
			break;
		case T_IntoClause:
			retval = _equalIntoClause(a, b);
			break;
		case T_Var:
			retval = _equalVar(a, b);
			break;
		case T_Const:
			retval = _equalConst(a, b);
			break;
		case T_Param:
			retval = _equalParam(a, b);
			break;
		case T_Aggref:
			retval = _equalAggref(a, b);
			break;
		case T_WindowFunc:
			retval = _equalWindowFunc(a, b);
			break;
		case T_ArrayRef:
			retval = _equalArrayRef(a, b);
			break;
		case T_FuncExpr:
			retval = _equalFuncExpr(a, b);
			break;
		case T_NamedArgExpr:
			retval = _equalNamedArgExpr(a, b);
			break;
		case T_OpExpr:
			retval = _equalOpExpr(a, b);
			break;
		case T_DistinctExpr:
			retval = _equalDistinctExpr(a, b);
			break;
		case T_NullIfExpr:
			retval = _equalNullIfExpr(a, b);
			break;
		case T_ScalarArrayOpExpr:
			retval = _equalScalarArrayOpExpr(a, b);
			break;
		case T_BoolExpr:
			retval = _equalBoolExpr(a, b);
			break;
		case T_SubLink:
			retval = _equalSubLink(a, b);
			break;
		case T_SubPlan:
			retval = _equalSubPlan(a, b);
			break;
		case T_AlternativeSubPlan:
			retval = _equalAlternativeSubPlan(a, b);
			break;
		case T_FieldSelect:
			retval = _equalFieldSelect(a, b);
			break;
		case T_FieldStore:
			retval = _equalFieldStore(a, b);
			break;
		case T_RelabelType:
			retval = _equalRelabelType(a, b);
			break;
		case T_CoerceViaIO:
			retval = _equalCoerceViaIO(a, b);
			break;
		case T_ArrayCoerceExpr:
			retval = _equalArrayCoerceExpr(a, b);
			break;
		case T_ConvertRowtypeExpr:
			retval = _equalConvertRowtypeExpr(a, b);
			break;
		case T_CollateExpr:
			retval = _equalCollateExpr(a, b);
			break;
		case T_CaseExpr:
			retval = _equalCaseExpr(a, b);
			break;
		case T_CaseWhen:
			retval = _equalCaseWhen(a, b);
			break;
		case T_CaseTestExpr:
			retval = _equalCaseTestExpr(a, b);
			break;
		case T_ArrayExpr:
			retval = _equalArrayExpr(a, b);
			break;
		case T_RowExpr:
			retval = _equalRowExpr(a, b);
			break;
		case T_RowCompareExpr:
			retval = _equalRowCompareExpr(a, b);
			break;
		case T_CoalesceExpr:
			retval = _equalCoalesceExpr(a, b);
			break;
		case T_MinMaxExpr:
			retval = _equalMinMaxExpr(a, b);
			break;
		case T_XmlExpr:
			retval = _equalXmlExpr(a, b);
			break;
		case T_NullTest:
			retval = _equalNullTest(a, b);
			break;
		case T_BooleanTest:
			retval = _equalBooleanTest(a, b);
			break;
		case T_CoerceToDomain:
			retval = _equalCoerceToDomain(a, b);
			break;
		case T_CoerceToDomainValue:
			retval = _equalCoerceToDomainValue(a, b);
			break;
		case T_SetToDefault:
			retval = _equalSetToDefault(a, b);
			break;
		case T_CurrentOfExpr:
			retval = _equalCurrentOfExpr(a, b);
			break;
		case T_TargetEntry:
			retval = _equalTargetEntry(a, b);
			break;
		case T_RangeTblRef:
			retval = _equalRangeTblRef(a, b);
			break;
		case T_FromExpr:
			retval = _equalFromExpr(a, b);
			break;
		case T_JoinExpr:
			retval = _equalJoinExpr(a, b);
			break;

			/*
			 * RELATION NODES
			 */
		case T_PathKey:
			retval = _equalPathKey(a, b);
			break;
		case T_RestrictInfo:
			retval = _equalRestrictInfo(a, b);
			break;
		case T_PlaceHolderVar:
			retval = _equalPlaceHolderVar(a, b);
			break;
		case T_SpecialJoinInfo:
			retval = _equalSpecialJoinInfo(a, b);
			break;
		case T_AppendRelInfo:
			retval = _equalAppendRelInfo(a, b);
			break;
		case T_PlaceHolderInfo:
			retval = _equalPlaceHolderInfo(a, b);
			break;

		case T_List:
		case T_IntList:
		case T_OidList:
			retval = _equalList(a, b);
			break;

		case T_Integer:
		case T_Float:
		case T_String:
		case T_BitString:
		case T_Null:
			retval = _equalValue(a, b);
			break;

			/*
			 * PARSE NODES
			 */
		case T_Query:
			retval = _equalQuery(a, b);
			break;
		case T_InsertStmt:
			retval = _equalInsertStmt(a, b);
			break;
		case T_DeleteStmt:
			retval = _equalDeleteStmt(a, b);
			break;
		case T_UpdateStmt:
			retval = _equalUpdateStmt(a, b);
			break;
		case T_SelectStmt:
			retval = _equalSelectStmt(a, b);
			break;
		case T_SetOperationStmt:
			retval = _equalSetOperationStmt(a, b);
			break;
		case T_AlterTableStmt:
			retval = _equalAlterTableStmt(a, b);
			break;
		case T_AlterTableCmd:
			retval = _equalAlterTableCmd(a, b);
			break;
		case T_AlterDomainStmt:
			retval = _equalAlterDomainStmt(a, b);
			break;
		case T_GrantStmt:
			retval = _equalGrantStmt(a, b);
			break;
		case T_GrantRoleStmt:
			retval = _equalGrantRoleStmt(a, b);
			break;
		case T_AlterDefaultPrivilegesStmt:
			retval = _equalAlterDefaultPrivilegesStmt(a, b);
			break;
		case T_DeclareCursorStmt:
			retval = _equalDeclareCursorStmt(a, b);
			break;
		case T_ClosePortalStmt:
			retval = _equalClosePortalStmt(a, b);
			break;
		case T_ClusterStmt:
			retval = _equalClusterStmt(a, b);
			break;
		case T_CopyStmt:
			retval = _equalCopyStmt(a, b);
			break;
		case T_CreateStmt:
			retval = _equalCreateStmt(a, b);
			break;
		case T_InhRelation:
			retval = _equalInhRelation(a, b);
			break;
		case T_DefineStmt:
			retval = _equalDefineStmt(a, b);
			break;
		case T_DropStmt:
			retval = _equalDropStmt(a, b);
			break;
		case T_TruncateStmt:
			retval = _equalTruncateStmt(a, b);
			break;
		case T_CommentStmt:
			retval = _equalCommentStmt(a, b);
			break;
		case T_SecLabelStmt:
			retval = _equalSecLabelStmt(a, b);
			break;
		case T_FetchStmt:
			retval = _equalFetchStmt(a, b);
			break;
		case T_IndexStmt:
			retval = _equalIndexStmt(a, b);
			break;
		case T_CreateFunctionStmt:
			retval = _equalCreateFunctionStmt(a, b);
			break;
		case T_FunctionParameter:
			retval = _equalFunctionParameter(a, b);
			break;
		case T_AlterFunctionStmt:
			retval = _equalAlterFunctionStmt(a, b);
			break;
		case T_RemoveFuncStmt:
			retval = _equalRemoveFuncStmt(a, b);
			break;
		case T_DoStmt:
			retval = _equalDoStmt(a, b);
			break;
		case T_RemoveOpClassStmt:
			retval = _equalRemoveOpClassStmt(a, b);
			break;
		case T_RemoveOpFamilyStmt:
			retval = _equalRemoveOpFamilyStmt(a, b);
			break;
		case T_RenameStmt:
			retval = _equalRenameStmt(a, b);
			break;
		case T_AlterObjectSchemaStmt:
			retval = _equalAlterObjectSchemaStmt(a, b);
			break;
		case T_AlterOwnerStmt:
			retval = _equalAlterOwnerStmt(a, b);
			break;
		case T_RuleStmt:
			retval = _equalRuleStmt(a, b);
			break;
		case T_NotifyStmt:
			retval = _equalNotifyStmt(a, b);
			break;
		case T_ListenStmt:
			retval = _equalListenStmt(a, b);
			break;
		case T_UnlistenStmt:
			retval = _equalUnlistenStmt(a, b);
			break;
		case T_TransactionStmt:
			retval = _equalTransactionStmt(a, b);
			break;
		case T_CompositeTypeStmt:
			retval = _equalCompositeTypeStmt(a, b);
			break;
		case T_CreateEnumStmt:
			retval = _equalCreateEnumStmt(a, b);
			break;
		case T_AlterEnumStmt:
			retval = _equalAlterEnumStmt(a, b);
			break;
		case T_ViewStmt:
			retval = _equalViewStmt(a, b);
			break;
		case T_LoadStmt:
			retval = _equalLoadStmt(a, b);
			break;
		case T_CreateDomainStmt:
			retval = _equalCreateDomainStmt(a, b);
			break;
		case T_CreateOpClassStmt:
			retval = _equalCreateOpClassStmt(a, b);
			break;
		case T_CreateOpClassItem:
			retval = _equalCreateOpClassItem(a, b);
			break;
		case T_CreateOpFamilyStmt:
			retval = _equalCreateOpFamilyStmt(a, b);
			break;
		case T_AlterOpFamilyStmt:
			retval = _equalAlterOpFamilyStmt(a, b);
			break;
		case T_CreatedbStmt:
			retval = _equalCreatedbStmt(a, b);
			break;
		case T_AlterDatabaseStmt:
			retval = _equalAlterDatabaseStmt(a, b);
			break;
		case T_AlterDatabaseSetStmt:
			retval = _equalAlterDatabaseSetStmt(a, b);
			break;
		case T_DropdbStmt:
			retval = _equalDropdbStmt(a, b);
			break;
		case T_VacuumStmt:
			retval = _equalVacuumStmt(a, b);
			break;
		case T_ExplainStmt:
			retval = _equalExplainStmt(a, b);
			break;
		case T_CreateSeqStmt:
			retval = _equalCreateSeqStmt(a, b);
			break;
		case T_AlterSeqStmt:
			retval = _equalAlterSeqStmt(a, b);
			break;
		case T_VariableSetStmt:
			retval = _equalVariableSetStmt(a, b);
			break;
		case T_VariableShowStmt:
			retval = _equalVariableShowStmt(a, b);
			break;
		case T_DiscardStmt:
			retval = _equalDiscardStmt(a, b);
			break;
		case T_CreateTableSpaceStmt:
			retval = _equalCreateTableSpaceStmt(a, b);
			break;
		case T_DropTableSpaceStmt:
			retval = _equalDropTableSpaceStmt(a, b);
			break;
		case T_AlterTableSpaceOptionsStmt:
			retval = _equalAlterTableSpaceOptionsStmt(a, b);
			break;
		case T_CreateExtensionStmt:
			retval = _equalCreateExtensionStmt(a, b);
			break;
		case T_AlterExtensionStmt:
			retval = _equalAlterExtensionStmt(a, b);
			break;
		case T_AlterExtensionContentsStmt:
			retval = _equalAlterExtensionContentsStmt(a, b);
			break;
		case T_CreateFdwStmt:
			retval = _equalCreateFdwStmt(a, b);
			break;
		case T_AlterFdwStmt:
			retval = _equalAlterFdwStmt(a, b);
			break;
		case T_DropFdwStmt:
			retval = _equalDropFdwStmt(a, b);
			break;
		case T_CreateForeignServerStmt:
			retval = _equalCreateForeignServerStmt(a, b);
			break;
		case T_AlterForeignServerStmt:
			retval = _equalAlterForeignServerStmt(a, b);
			break;
		case T_DropForeignServerStmt:
			retval = _equalDropForeignServerStmt(a, b);
			break;
		case T_CreateUserMappingStmt:
			retval = _equalCreateUserMappingStmt(a, b);
			break;
		case T_AlterUserMappingStmt:
			retval = _equalAlterUserMappingStmt(a, b);
			break;
		case T_DropUserMappingStmt:
			retval = _equalDropUserMappingStmt(a, b);
			break;
		case T_CreateForeignTableStmt:
			retval = _equalCreateForeignTableStmt(a, b);
			break;
		case T_CreateTrigStmt:
			retval = _equalCreateTrigStmt(a, b);
			break;
		case T_DropPropertyStmt:
			retval = _equalDropPropertyStmt(a, b);
			break;
		case T_CreatePLangStmt:
			retval = _equalCreatePLangStmt(a, b);
			break;
		case T_DropPLangStmt:
			retval = _equalDropPLangStmt(a, b);
			break;
		case T_CreateRoleStmt:
			retval = _equalCreateRoleStmt(a, b);
			break;
		case T_AlterRoleStmt:
			retval = _equalAlterRoleStmt(a, b);
			break;
		case T_AlterRoleSetStmt:
			retval = _equalAlterRoleSetStmt(a, b);
			break;
		case T_DropRoleStmt:
			retval = _equalDropRoleStmt(a, b);
			break;
		case T_LockStmt:
			retval = _equalLockStmt(a, b);
			break;
		case T_ConstraintsSetStmt:
			retval = _equalConstraintsSetStmt(a, b);
			break;
		case T_ReindexStmt:
			retval = _equalReindexStmt(a, b);
			break;
		case T_CheckPointStmt:
			retval = true;
			break;
		case T_CreateSchemaStmt:
			retval = _equalCreateSchemaStmt(a, b);
			break;
		case T_CreateConversionStmt:
			retval = _equalCreateConversionStmt(a, b);
			break;
		case T_CreateCastStmt:
			retval = _equalCreateCastStmt(a, b);
			break;
		case T_DropCastStmt:
			retval = _equalDropCastStmt(a, b);
			break;
		case T_PrepareStmt:
			retval = _equalPrepareStmt(a, b);
			break;
		case T_ExecuteStmt:
			retval = _equalExecuteStmt(a, b);
			break;
		case T_DeallocateStmt:
			retval = _equalDeallocateStmt(a, b);
			break;
		case T_DropOwnedStmt:
			retval = _equalDropOwnedStmt(a, b);
			break;
		case T_ReassignOwnedStmt:
			retval = _equalReassignOwnedStmt(a, b);
			break;
		case T_AlterTSDictionaryStmt:
			retval = _equalAlterTSDictionaryStmt(a, b);
			break;
		case T_AlterTSConfigurationStmt:
			retval = _equalAlterTSConfigurationStmt(a, b);
			break;

		case T_A_Expr:
			retval = _equalAExpr(a, b);
			break;
		case T_ColumnRef:
			retval = _equalColumnRef(a, b);
			break;
		case T_ParamRef:
			retval = _equalParamRef(a, b);
			break;
		case T_A_Const:
			retval = _equalAConst(a, b);
			break;
		case T_FuncCall:
			retval = _equalFuncCall(a, b);
			break;
		case T_A_Star:
			retval = _equalAStar(a, b);
			break;
		case T_A_Indices:
			retval = _equalAIndices(a, b);
			break;
		case T_A_Indirection:
			retval = _equalA_Indirection(a, b);
			break;
		case T_A_ArrayExpr:
			retval = _equalA_ArrayExpr(a, b);
			break;
		case T_ResTarget:
			retval = _equalResTarget(a, b);
			break;
		case T_TypeCast:
			retval = _equalTypeCast(a, b);
			break;
		case T_CollateClause:
			retval = _equalCollateClause(a, b);
			break;
		case T_SortBy:
			retval = _equalSortBy(a, b);
			break;
		case T_WindowDef:
			retval = _equalWindowDef(a, b);
			break;
		case T_RangeSubselect:
			retval = _equalRangeSubselect(a, b);
			break;
		case T_RangeFunction:
			retval = _equalRangeFunction(a, b);
			break;
		case T_TypeName:
			retval = _equalTypeName(a, b);
			break;
		case T_IndexElem:
			retval = _equalIndexElem(a, b);
			break;
		case T_ColumnDef:
			retval = _equalColumnDef(a, b);
			break;
		case T_Constraint:
			retval = _equalConstraint(a, b);
			break;
		case T_DefElem:
			retval = _equalDefElem(a, b);
			break;
		case T_LockingClause:
			retval = _equalLockingClause(a, b);
			break;
		case T_RangeTblEntry:
			retval = _equalRangeTblEntry(a, b);
			break;
		case T_SortGroupClause:
			retval = _equalSortGroupClause(a, b);
			break;
		case T_WindowClause:
			retval = _equalWindowClause(a, b);
			break;
		case T_RowMarkClause:
			retval = _equalRowMarkClause(a, b);
			break;
		case T_WithClause:
			retval = _equalWithClause(a, b);
			break;
		case T_CommonTableExpr:
			retval = _equalCommonTableExpr(a, b);
			break;
		case T_PrivGrantee:
			retval = _equalPrivGrantee(a, b);
			break;
		case T_FuncWithArgs:
			retval = _equalFuncWithArgs(a, b);
			break;
		case T_AccessPriv:
			retval = _equalAccessPriv(a, b);
			break;
		case T_XmlSerialize:
			retval = _equalXmlSerialize(a, b);
			break;

		default:
			elog(ERROR, "unrecognized node type: %d",
				 (int) nodeTag(a));
			retval = false;		/* keep compiler quiet */
			break;
	}

	return retval;
}
