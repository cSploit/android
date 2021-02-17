/*-------------------------------------------------------------------------
 *
 * parse_agg.h
 *	  handle aggregates and window functions in parser
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/parser/parse_agg.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef PARSE_AGG_H
#define PARSE_AGG_H

#include "parser/parse_node.h"

extern void transformAggregateCall(ParseState *pstate, Aggref *agg,
					   List *args, List *aggorder,
					   bool agg_distinct);
extern void transformWindowFuncCall(ParseState *pstate, WindowFunc *wfunc,
						WindowDef *windef);

extern void parseCheckAggregates(ParseState *pstate, Query *qry);
extern void parseCheckWindowFuncs(ParseState *pstate, Query *qry);

extern void build_aggregate_fnexprs(Oid *agg_input_types,
						int agg_num_inputs,
						Oid agg_state_type,
						Oid agg_result_type,
						Oid agg_input_collation,
						Oid transfn_oid,
						Oid finalfn_oid,
						Expr **transfnexpr,
						Expr **finalfnexpr);

#endif   /* PARSE_AGG_H */
