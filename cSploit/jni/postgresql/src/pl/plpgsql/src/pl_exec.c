/*-------------------------------------------------------------------------
 *
 * pl_exec.c		- Executor for the PL/pgSQL
 *			  procedural language
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/pl/plpgsql/src/pl_exec.c
 *
 *-------------------------------------------------------------------------
 */

#include "plpgsql.h"

#include <ctype.h>

#include "access/transam.h"
#include "access/tupconvert.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "executor/spi_priv.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/nodeFuncs.h"
#include "parser/scansup.h"
#include "storage/proc.h"
#include "tcop/tcopprot.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/datum.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/snapmgr.h"
#include "utils/typcache.h"


static const char *const raise_skip_msg = "RAISE";

typedef struct
{
	int			nargs;			/* number of arguments */
	Oid		   *types;			/* types of arguments */
	Datum	   *values;			/* evaluated argument values */
	char	   *nulls;			/* null markers (' '/'n' style) */
	bool	   *freevals;		/* which arguments are pfree-able */
} PreparedParamsData;

/*
 * All plpgsql function executions within a single transaction share the same
 * executor EState for evaluating "simple" expressions.  Each function call
 * creates its own "eval_econtext" ExprContext within this estate for
 * per-evaluation workspace.  eval_econtext is freed at normal function exit,
 * and the EState is freed at transaction end (in case of error, we assume
 * that the abort mechanisms clean it all up).	Furthermore, any exception
 * block within a function has to have its own eval_econtext separate from
 * the containing function's, so that we can clean up ExprContext callbacks
 * properly at subtransaction exit.  We maintain a stack that tracks the
 * individual econtexts so that we can clean up correctly at subxact exit.
 *
 * This arrangement is a bit tedious to maintain, but it's worth the trouble
 * so that we don't have to re-prepare simple expressions on each trip through
 * a function.	(We assume the case to optimize is many repetitions of a
 * function within a transaction.)
 */
typedef struct SimpleEcontextStackEntry
{
	ExprContext *stack_econtext;	/* a stacked econtext */
	SubTransactionId xact_subxid;		/* ID for current subxact */
	struct SimpleEcontextStackEntry *next;		/* next stack entry up */
} SimpleEcontextStackEntry;

static EState *simple_eval_estate = NULL;
static SimpleEcontextStackEntry *simple_econtext_stack = NULL;

/************************************************************
 * Local function forward declarations
 ************************************************************/
static void plpgsql_exec_error_callback(void *arg);
static PLpgSQL_datum *copy_plpgsql_datum(PLpgSQL_datum *datum);

static int exec_stmt_block(PLpgSQL_execstate *estate,
				PLpgSQL_stmt_block *block);
static int exec_stmts(PLpgSQL_execstate *estate,
		   List *stmts);
static int exec_stmt(PLpgSQL_execstate *estate,
		  PLpgSQL_stmt *stmt);
static int exec_stmt_assign(PLpgSQL_execstate *estate,
				 PLpgSQL_stmt_assign *stmt);
static int exec_stmt_perform(PLpgSQL_execstate *estate,
				  PLpgSQL_stmt_perform *stmt);
static int exec_stmt_getdiag(PLpgSQL_execstate *estate,
				  PLpgSQL_stmt_getdiag *stmt);
static int exec_stmt_if(PLpgSQL_execstate *estate,
			 PLpgSQL_stmt_if *stmt);
static int exec_stmt_case(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_case *stmt);
static int exec_stmt_loop(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_loop *stmt);
static int exec_stmt_while(PLpgSQL_execstate *estate,
				PLpgSQL_stmt_while *stmt);
static int exec_stmt_fori(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_fori *stmt);
static int exec_stmt_fors(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_fors *stmt);
static int exec_stmt_forc(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_forc *stmt);
static int exec_stmt_foreach_a(PLpgSQL_execstate *estate,
					PLpgSQL_stmt_foreach_a *stmt);
static int exec_stmt_open(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_open *stmt);
static int exec_stmt_fetch(PLpgSQL_execstate *estate,
				PLpgSQL_stmt_fetch *stmt);
static int exec_stmt_close(PLpgSQL_execstate *estate,
				PLpgSQL_stmt_close *stmt);
static int exec_stmt_exit(PLpgSQL_execstate *estate,
			   PLpgSQL_stmt_exit *stmt);
static int exec_stmt_return(PLpgSQL_execstate *estate,
				 PLpgSQL_stmt_return *stmt);
static int exec_stmt_return_next(PLpgSQL_execstate *estate,
					  PLpgSQL_stmt_return_next *stmt);
static int exec_stmt_return_query(PLpgSQL_execstate *estate,
					   PLpgSQL_stmt_return_query *stmt);
static int exec_stmt_raise(PLpgSQL_execstate *estate,
				PLpgSQL_stmt_raise *stmt);
static int exec_stmt_execsql(PLpgSQL_execstate *estate,
				  PLpgSQL_stmt_execsql *stmt);
static int exec_stmt_dynexecute(PLpgSQL_execstate *estate,
					 PLpgSQL_stmt_dynexecute *stmt);
static int exec_stmt_dynfors(PLpgSQL_execstate *estate,
				  PLpgSQL_stmt_dynfors *stmt);

static void plpgsql_estate_setup(PLpgSQL_execstate *estate,
					 PLpgSQL_function *func,
					 ReturnSetInfo *rsi);
static void exec_eval_cleanup(PLpgSQL_execstate *estate);

static void exec_prepare_plan(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr, int cursorOptions);
static bool exec_simple_check_node(Node *node);
static void exec_simple_check_plan(PLpgSQL_expr *expr);
static bool exec_eval_simple_expr(PLpgSQL_execstate *estate,
					  PLpgSQL_expr *expr,
					  Datum *result,
					  bool *isNull,
					  Oid *rettype);

static void exec_assign_expr(PLpgSQL_execstate *estate,
				 PLpgSQL_datum *target,
				 PLpgSQL_expr *expr);
static void exec_assign_value(PLpgSQL_execstate *estate,
				  PLpgSQL_datum *target,
				  Datum value, Oid valtype, bool *isNull);
static void exec_eval_datum(PLpgSQL_execstate *estate,
				PLpgSQL_datum *datum,
				Oid *typeid,
				int32 *typetypmod,
				Datum *value,
				bool *isnull);
static int exec_eval_integer(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr,
				  bool *isNull);
static bool exec_eval_boolean(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr,
				  bool *isNull);
static Datum exec_eval_expr(PLpgSQL_execstate *estate,
			   PLpgSQL_expr *expr,
			   bool *isNull,
			   Oid *rettype);
static int exec_run_select(PLpgSQL_execstate *estate,
				PLpgSQL_expr *expr, long maxtuples, Portal *portalP);
static int exec_for_query(PLpgSQL_execstate *estate, PLpgSQL_stmt_forq *stmt,
			   Portal portal, bool prefetch_ok);
static ParamListInfo setup_param_list(PLpgSQL_execstate *estate,
				 PLpgSQL_expr *expr);
static void plpgsql_param_fetch(ParamListInfo params, int paramid);
static void exec_move_row(PLpgSQL_execstate *estate,
			  PLpgSQL_rec *rec,
			  PLpgSQL_row *row,
			  HeapTuple tup, TupleDesc tupdesc);
static HeapTuple make_tuple_from_row(PLpgSQL_execstate *estate,
					PLpgSQL_row *row,
					TupleDesc tupdesc);
static char *convert_value_to_string(PLpgSQL_execstate *estate,
						Datum value, Oid valtype);
static Datum exec_cast_value(PLpgSQL_execstate *estate,
				Datum value, Oid valtype,
				Oid reqtype,
				FmgrInfo *reqinput,
				Oid reqtypioparam,
				int32 reqtypmod,
				bool isnull);
static Datum exec_simple_cast_value(PLpgSQL_execstate *estate,
					   Datum value, Oid valtype,
					   Oid reqtype, int32 reqtypmod,
					   bool isnull);
static void exec_init_tuple_store(PLpgSQL_execstate *estate);
static void exec_set_found(PLpgSQL_execstate *estate, bool state);
static void plpgsql_create_econtext(PLpgSQL_execstate *estate);
static void plpgsql_destroy_econtext(PLpgSQL_execstate *estate);
static void free_var(PLpgSQL_var *var);
static void assign_text_var(PLpgSQL_var *var, const char *str);
static PreparedParamsData *exec_eval_using_params(PLpgSQL_execstate *estate,
					   List *params);
static void free_params_data(PreparedParamsData *ppd);
static Portal exec_dynquery_with_params(PLpgSQL_execstate *estate,
						  PLpgSQL_expr *dynquery, List *params,
						  const char *portalname, int cursorOptions);


/* ----------
 * plpgsql_exec_function	Called by the call handler for
 *				function execution.
 * ----------
 */
Datum
plpgsql_exec_function(PLpgSQL_function *func, FunctionCallInfo fcinfo)
{
	PLpgSQL_execstate estate;
	ErrorContextCallback plerrcontext;
	int			i;
	int			rc;

	/*
	 * Setup the execution state
	 */
	plpgsql_estate_setup(&estate, func, (ReturnSetInfo *) fcinfo->resultinfo);

	/*
	 * Setup error traceback support for ereport()
	 */
	plerrcontext.callback = plpgsql_exec_error_callback;
	plerrcontext.arg = &estate;
	plerrcontext.previous = error_context_stack;
	error_context_stack = &plerrcontext;

	/*
	 * Make local execution copies of all the datums
	 */
	estate.err_text = gettext_noop("during initialization of execution state");
	for (i = 0; i < estate.ndatums; i++)
		estate.datums[i] = copy_plpgsql_datum(func->datums[i]);

	/*
	 * Store the actual call argument values into the appropriate variables
	 */
	estate.err_text = gettext_noop("while storing call arguments into local variables");
	for (i = 0; i < func->fn_nargs; i++)
	{
		int			n = func->fn_argvarnos[i];

		switch (estate.datums[n]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) estate.datums[n];

					var->value = fcinfo->arg[i];
					var->isnull = fcinfo->argnull[i];
					var->freeval = false;
				}
				break;

			case PLPGSQL_DTYPE_ROW:
				{
					PLpgSQL_row *row = (PLpgSQL_row *) estate.datums[n];

					if (!fcinfo->argnull[i])
					{
						HeapTupleHeader td;
						Oid			tupType;
						int32		tupTypmod;
						TupleDesc	tupdesc;
						HeapTupleData tmptup;

						td = DatumGetHeapTupleHeader(fcinfo->arg[i]);
						/* Extract rowtype info and find a tupdesc */
						tupType = HeapTupleHeaderGetTypeId(td);
						tupTypmod = HeapTupleHeaderGetTypMod(td);
						tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
						/* Build a temporary HeapTuple control structure */
						tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
						ItemPointerSetInvalid(&(tmptup.t_self));
						tmptup.t_tableOid = InvalidOid;
						tmptup.t_data = td;
						exec_move_row(&estate, NULL, row, &tmptup, tupdesc);
						ReleaseTupleDesc(tupdesc);
					}
					else
					{
						/* If arg is null, treat it as an empty row */
						exec_move_row(&estate, NULL, row, NULL, NULL);
					}
					/* clean up after exec_move_row() */
					exec_eval_cleanup(&estate);
				}
				break;

			default:
				elog(ERROR, "unrecognized dtype: %d", func->datums[i]->dtype);
		}
	}

	estate.err_text = gettext_noop("during function entry");

	/*
	 * Set the magic variable FOUND to false
	 */
	exec_set_found(&estate, false);

	/*
	 * Let the instrumentation plugin peek at this function
	 */
	if (*plugin_ptr && (*plugin_ptr)->func_beg)
		((*plugin_ptr)->func_beg) (&estate, func);

	/*
	 * Now call the toplevel block of statements
	 */
	estate.err_text = NULL;
	estate.err_stmt = (PLpgSQL_stmt *) (func->action);
	rc = exec_stmt_block(&estate, func->action);
	if (rc != PLPGSQL_RC_RETURN)
	{
		estate.err_stmt = NULL;
		estate.err_text = NULL;

		/*
		 * Provide a more helpful message if a CONTINUE or RAISE has been used
		 * outside the context it can work in.
		 */
		if (rc == PLPGSQL_RC_CONTINUE)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("CONTINUE cannot be used outside a loop")));
		else
			ereport(ERROR,
			   (errcode(ERRCODE_S_R_E_FUNCTION_EXECUTED_NO_RETURN_STATEMENT),
				errmsg("control reached end of function without RETURN")));
	}

	/*
	 * We got a return value - process it
	 */
	estate.err_stmt = NULL;
	estate.err_text = gettext_noop("while casting return value to function's return type");

	fcinfo->isnull = estate.retisnull;

	if (estate.retisset)
	{
		ReturnSetInfo *rsi = estate.rsi;

		/* Check caller can handle a set result */
		if (!rsi || !IsA(rsi, ReturnSetInfo) ||
			(rsi->allowedModes & SFRM_Materialize) == 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("set-valued function called in context that cannot accept a set")));
		rsi->returnMode = SFRM_Materialize;

		/* If we produced any tuples, send back the result */
		if (estate.tuple_store)
		{
			rsi->setResult = estate.tuple_store;
			if (estate.rettupdesc)
			{
				MemoryContext oldcxt;

				oldcxt = MemoryContextSwitchTo(estate.tuple_store_cxt);
				rsi->setDesc = CreateTupleDescCopy(estate.rettupdesc);
				MemoryContextSwitchTo(oldcxt);
			}
		}
		estate.retval = (Datum) 0;
		fcinfo->isnull = true;
	}
	else if (!estate.retisnull)
	{
		if (estate.retistuple)
		{
			/*
			 * We have to check that the returned tuple actually matches the
			 * expected result type.  XXX would be better to cache the tupdesc
			 * instead of repeating get_call_result_type()
			 */
			HeapTuple	rettup = (HeapTuple) DatumGetPointer(estate.retval);
			TupleDesc	tupdesc;
			TupleConversionMap *tupmap;

			switch (get_call_result_type(fcinfo, NULL, &tupdesc))
			{
				case TYPEFUNC_COMPOSITE:
					/* got the expected result rowtype, now check it */
					tupmap = convert_tuples_by_position(estate.rettupdesc,
														tupdesc,
														gettext_noop("returned record type does not match expected record type"));
					/* it might need conversion */
					if (tupmap)
						rettup = do_convert_tuple(rettup, tupmap);
					/* no need to free map, we're about to return anyway */
					break;
				case TYPEFUNC_RECORD:

					/*
					 * Failed to determine actual type of RECORD.  We could
					 * raise an error here, but what this means in practice is
					 * that the caller is expecting any old generic rowtype,
					 * so we don't really need to be restrictive. Pass back
					 * the generated result type, instead.
					 */
					tupdesc = estate.rettupdesc;
					if (tupdesc == NULL)		/* shouldn't happen */
						elog(ERROR, "return type must be a row type");
					break;
				default:
					/* shouldn't get here if retistuple is true ... */
					elog(ERROR, "return type must be a row type");
					break;
			}

			/*
			 * Copy tuple to upper executor memory, as a tuple Datum. Make
			 * sure it is labeled with the caller-supplied tuple type.
			 */
			estate.retval = PointerGetDatum(SPI_returntuple(rettup, tupdesc));
		}
		else
		{
			/* Cast value to proper type */
			estate.retval = exec_cast_value(&estate,
											estate.retval,
											estate.rettype,
											func->fn_rettype,
											&(func->fn_retinput),
											func->fn_rettypioparam,
											-1,
											fcinfo->isnull);

			/*
			 * If the function's return type isn't by value, copy the value
			 * into upper executor memory context.
			 */
			if (!fcinfo->isnull && !func->fn_retbyval)
			{
				Size		len;
				void	   *tmp;

				len = datumGetSize(estate.retval, false, func->fn_rettyplen);
				tmp = SPI_palloc(len);
				memcpy(tmp, DatumGetPointer(estate.retval), len);
				estate.retval = PointerGetDatum(tmp);
			}
		}
	}

	estate.err_text = gettext_noop("during function exit");

	/*
	 * Let the instrumentation plugin peek at this function
	 */
	if (*plugin_ptr && (*plugin_ptr)->func_end)
		((*plugin_ptr)->func_end) (&estate, func);

	/* Clean up any leftover temporary memory */
	plpgsql_destroy_econtext(&estate);
	exec_eval_cleanup(&estate);

	/*
	 * Pop the error context stack
	 */
	error_context_stack = plerrcontext.previous;

	/*
	 * Return the function's result
	 */
	return estate.retval;
}


/* ----------
 * plpgsql_exec_trigger		Called by the call handler for
 *				trigger execution.
 * ----------
 */
HeapTuple
plpgsql_exec_trigger(PLpgSQL_function *func,
					 TriggerData *trigdata)
{
	PLpgSQL_execstate estate;
	ErrorContextCallback plerrcontext;
	int			i;
	int			rc;
	PLpgSQL_var *var;
	PLpgSQL_rec *rec_new,
			   *rec_old;
	HeapTuple	rettup;

	/*
	 * Setup the execution state
	 */
	plpgsql_estate_setup(&estate, func, NULL);

	/*
	 * Setup error traceback support for ereport()
	 */
	plerrcontext.callback = plpgsql_exec_error_callback;
	plerrcontext.arg = &estate;
	plerrcontext.previous = error_context_stack;
	error_context_stack = &plerrcontext;

	/*
	 * Make local execution copies of all the datums
	 */
	estate.err_text = gettext_noop("during initialization of execution state");
	for (i = 0; i < estate.ndatums; i++)
		estate.datums[i] = copy_plpgsql_datum(func->datums[i]);

	/*
	 * Put the OLD and NEW tuples into record variables
	 *
	 * We make the tupdescs available in both records even though only one may
	 * have a value.  This allows parsing of record references to succeed in
	 * functions that are used for multiple trigger types.	For example, we
	 * might have a test like "if (TG_OP = 'INSERT' and NEW.foo = 'xyz')",
	 * which should parse regardless of the current trigger type.
	 */
	rec_new = (PLpgSQL_rec *) (estate.datums[func->new_varno]);
	rec_new->freetup = false;
	rec_new->tupdesc = trigdata->tg_relation->rd_att;
	rec_new->freetupdesc = false;
	rec_old = (PLpgSQL_rec *) (estate.datums[func->old_varno]);
	rec_old->freetup = false;
	rec_old->tupdesc = trigdata->tg_relation->rd_att;
	rec_old->freetupdesc = false;

	if (!TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
	{
		/*
		 * Per-statement triggers don't use OLD/NEW variables
		 */
		rec_new->tup = NULL;
		rec_old->tup = NULL;
	}
	else if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
	{
		rec_new->tup = trigdata->tg_trigtuple;
		rec_old->tup = NULL;
	}
	else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
	{
		rec_new->tup = trigdata->tg_newtuple;
		rec_old->tup = trigdata->tg_trigtuple;
	}
	else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
	{
		rec_new->tup = NULL;
		rec_old->tup = trigdata->tg_trigtuple;
	}
	else
		elog(ERROR, "unrecognized trigger action: not INSERT, DELETE, or UPDATE");

	/*
	 * Assign the special tg_ variables
	 */

	var = (PLpgSQL_var *) (estate.datums[func->tg_op_varno]);
	if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
		var->value = CStringGetTextDatum("INSERT");
	else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
		var->value = CStringGetTextDatum("UPDATE");
	else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
		var->value = CStringGetTextDatum("DELETE");
	else if (TRIGGER_FIRED_BY_TRUNCATE(trigdata->tg_event))
		var->value = CStringGetTextDatum("TRUNCATE");
	else
		elog(ERROR, "unrecognized trigger action: not INSERT, DELETE, UPDATE, or TRUNCATE");
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_name_varno]);
	var->value = DirectFunctionCall1(namein,
							  CStringGetDatum(trigdata->tg_trigger->tgname));
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_when_varno]);
	if (TRIGGER_FIRED_BEFORE(trigdata->tg_event))
		var->value = CStringGetTextDatum("BEFORE");
	else if (TRIGGER_FIRED_AFTER(trigdata->tg_event))
		var->value = CStringGetTextDatum("AFTER");
	else if (TRIGGER_FIRED_INSTEAD(trigdata->tg_event))
		var->value = CStringGetTextDatum("INSTEAD OF");
	else
		elog(ERROR, "unrecognized trigger execution time: not BEFORE, AFTER, or INSTEAD OF");
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_level_varno]);
	if (TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		var->value = CStringGetTextDatum("ROW");
	else if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
		var->value = CStringGetTextDatum("STATEMENT");
	else
		elog(ERROR, "unrecognized trigger event type: not ROW or STATEMENT");
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_relid_varno]);
	var->value = ObjectIdGetDatum(trigdata->tg_relation->rd_id);
	var->isnull = false;
	var->freeval = false;

	var = (PLpgSQL_var *) (estate.datums[func->tg_relname_varno]);
	var->value = DirectFunctionCall1(namein,
			CStringGetDatum(RelationGetRelationName(trigdata->tg_relation)));
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_table_name_varno]);
	var->value = DirectFunctionCall1(namein,
			CStringGetDatum(RelationGetRelationName(trigdata->tg_relation)));
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_table_schema_varno]);
	var->value = DirectFunctionCall1(namein,
									 CStringGetDatum(
													 get_namespace_name(
														RelationGetNamespace(
												   trigdata->tg_relation))));
	var->isnull = false;
	var->freeval = true;

	var = (PLpgSQL_var *) (estate.datums[func->tg_nargs_varno]);
	var->value = Int16GetDatum(trigdata->tg_trigger->tgnargs);
	var->isnull = false;
	var->freeval = false;

	var = (PLpgSQL_var *) (estate.datums[func->tg_argv_varno]);
	if (trigdata->tg_trigger->tgnargs > 0)
	{
		/*
		 * For historical reasons, tg_argv[] subscripts start at zero not one.
		 * So we can't use construct_array().
		 */
		int			nelems = trigdata->tg_trigger->tgnargs;
		Datum	   *elems;
		int			dims[1];
		int			lbs[1];

		elems = palloc(sizeof(Datum) * nelems);
		for (i = 0; i < nelems; i++)
			elems[i] = CStringGetTextDatum(trigdata->tg_trigger->tgargs[i]);
		dims[0] = nelems;
		lbs[0] = 0;

		var->value = PointerGetDatum(construct_md_array(elems, NULL,
														1, dims, lbs,
														TEXTOID,
														-1, false, 'i'));
		var->isnull = false;
		var->freeval = true;
	}
	else
	{
		var->value = (Datum) 0;
		var->isnull = true;
		var->freeval = false;
	}

	estate.err_text = gettext_noop("during function entry");

	/*
	 * Set the magic variable FOUND to false
	 */
	exec_set_found(&estate, false);

	/*
	 * Let the instrumentation plugin peek at this function
	 */
	if (*plugin_ptr && (*plugin_ptr)->func_beg)
		((*plugin_ptr)->func_beg) (&estate, func);

	/*
	 * Now call the toplevel block of statements
	 */
	estate.err_text = NULL;
	estate.err_stmt = (PLpgSQL_stmt *) (func->action);
	rc = exec_stmt_block(&estate, func->action);
	if (rc != PLPGSQL_RC_RETURN)
	{
		estate.err_stmt = NULL;
		estate.err_text = NULL;

		/*
		 * Provide a more helpful message if a CONTINUE or RAISE has been used
		 * outside the context it can work in.
		 */
		if (rc == PLPGSQL_RC_CONTINUE)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("CONTINUE cannot be used outside a loop")));
		else
			ereport(ERROR,
			   (errcode(ERRCODE_S_R_E_FUNCTION_EXECUTED_NO_RETURN_STATEMENT),
				errmsg("control reached end of trigger procedure without RETURN")));
	}

	estate.err_stmt = NULL;
	estate.err_text = gettext_noop("during function exit");

	if (estate.retisset)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("trigger procedure cannot return a set")));

	/*
	 * Check that the returned tuple structure has the same attributes, the
	 * relation that fired the trigger has. A per-statement trigger always
	 * needs to return NULL, so we ignore any return value the function itself
	 * produces (XXX: is this a good idea?)
	 *
	 * XXX This way it is possible, that the trigger returns a tuple where
	 * attributes don't have the correct atttypmod's length. It's up to the
	 * trigger's programmer to ensure that this doesn't happen. Jan
	 */
	if (estate.retisnull || !TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		rettup = NULL;
	else
	{
		TupleConversionMap *tupmap;

		rettup = (HeapTuple) DatumGetPointer(estate.retval);
		/* check rowtype compatibility */
		tupmap = convert_tuples_by_position(estate.rettupdesc,
											trigdata->tg_relation->rd_att,
											gettext_noop("returned row structure does not match the structure of the triggering table"));
		/* it might need conversion */
		if (tupmap)
			rettup = do_convert_tuple(rettup, tupmap);
		/* no need to free map, we're about to return anyway */

		/* Copy tuple to upper executor memory */
		rettup = SPI_copytuple(rettup);
	}

	/*
	 * Let the instrumentation plugin peek at this function
	 */
	if (*plugin_ptr && (*plugin_ptr)->func_end)
		((*plugin_ptr)->func_end) (&estate, func);

	/* Clean up any leftover temporary memory */
	plpgsql_destroy_econtext(&estate);
	exec_eval_cleanup(&estate);

	/*
	 * Pop the error context stack
	 */
	error_context_stack = plerrcontext.previous;

	/*
	 * Return the trigger's result
	 */
	return rettup;
}


/*
 * error context callback to let us supply a call-stack traceback
 */
static void
plpgsql_exec_error_callback(void *arg)
{
	PLpgSQL_execstate *estate = (PLpgSQL_execstate *) arg;

	/* if we are doing RAISE, don't report its location */
	if (estate->err_text == raise_skip_msg)
		return;

	if (estate->err_text != NULL)
	{
		/*
		 * We don't expend the cycles to run gettext() on err_text unless we
		 * actually need it.  Therefore, places that set up err_text should
		 * use gettext_noop() to ensure the strings get recorded in the
		 * message dictionary.
		 *
		 * If both err_text and err_stmt are set, use the err_text as
		 * description, but report the err_stmt's line number.  When err_stmt
		 * is not set, we're in function entry/exit, or some such place not
		 * attached to a specific line number.
		 */
		if (estate->err_stmt != NULL)
		{
			/*
			 * translator: last %s is a phrase such as "during statement block
			 * local variable initialization"
			 */
			errcontext("PL/pgSQL function \"%s\" line %d %s",
					   estate->func->fn_name,
					   estate->err_stmt->lineno,
					   _(estate->err_text));
		}
		else
		{
			/*
			 * translator: last %s is a phrase such as "while storing call
			 * arguments into local variables"
			 */
			errcontext("PL/pgSQL function \"%s\" %s",
					   estate->func->fn_name,
					   _(estate->err_text));
		}
	}
	else if (estate->err_stmt != NULL)
	{
		/* translator: last %s is a plpgsql statement type name */
		errcontext("PL/pgSQL function \"%s\" line %d at %s",
				   estate->func->fn_name,
				   estate->err_stmt->lineno,
				   plpgsql_stmt_typename(estate->err_stmt));
	}
	else
		errcontext("PL/pgSQL function \"%s\"",
				   estate->func->fn_name);
}


/* ----------
 * Support function for initializing local execution variables
 * ----------
 */
static PLpgSQL_datum *
copy_plpgsql_datum(PLpgSQL_datum *datum)
{
	PLpgSQL_datum *result;

	switch (datum->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			{
				PLpgSQL_var *new = palloc(sizeof(PLpgSQL_var));

				memcpy(new, datum, sizeof(PLpgSQL_var));
				/* Ensure the value is null (possibly not needed?) */
				new->value = 0;
				new->isnull = true;
				new->freeval = false;

				result = (PLpgSQL_datum *) new;
			}
			break;

		case PLPGSQL_DTYPE_REC:
			{
				PLpgSQL_rec *new = palloc(sizeof(PLpgSQL_rec));

				memcpy(new, datum, sizeof(PLpgSQL_rec));
				/* Ensure the value is null (possibly not needed?) */
				new->tup = NULL;
				new->tupdesc = NULL;
				new->freetup = false;
				new->freetupdesc = false;

				result = (PLpgSQL_datum *) new;
			}
			break;

		case PLPGSQL_DTYPE_ROW:
		case PLPGSQL_DTYPE_RECFIELD:
		case PLPGSQL_DTYPE_ARRAYELEM:

			/*
			 * These datum records are read-only at runtime, so no need to
			 * copy them
			 */
			result = datum;
			break;

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			result = NULL;		/* keep compiler quiet */
			break;
	}

	return result;
}


static bool
exception_matches_conditions(ErrorData *edata, PLpgSQL_condition *cond)
{
	for (; cond != NULL; cond = cond->next)
	{
		int			sqlerrstate = cond->sqlerrstate;

		/*
		 * OTHERS matches everything *except* query-canceled; if you're
		 * foolish enough, you can match that explicitly.
		 */
		if (sqlerrstate == 0)
		{
			if (edata->sqlerrcode != ERRCODE_QUERY_CANCELED)
				return true;
		}
		/* Exact match? */
		else if (edata->sqlerrcode == sqlerrstate)
			return true;
		/* Category match? */
		else if (ERRCODE_IS_CATEGORY(sqlerrstate) &&
				 ERRCODE_TO_CATEGORY(edata->sqlerrcode) == sqlerrstate)
			return true;
	}
	return false;
}


/* ----------
 * exec_stmt_block			Execute a block of statements
 * ----------
 */
static int
exec_stmt_block(PLpgSQL_execstate *estate, PLpgSQL_stmt_block *block)
{
	volatile int rc = -1;
	int			i;
	int			n;

	/*
	 * First initialize all variables declared in this block
	 */
	estate->err_text = gettext_noop("during statement block local variable initialization");

	for (i = 0; i < block->n_initvars; i++)
	{
		n = block->initvarnos[i];

		switch (estate->datums[n]->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) (estate->datums[n]);

					/* free any old value, in case re-entering block */
					free_var(var);

					/* Initially it contains a NULL */
					var->value = (Datum) 0;
					var->isnull = true;

					if (var->default_val == NULL)
					{
						/*
						 * If needed, give the datatype a chance to reject
						 * NULLs, by assigning a NULL to the variable. We
						 * claim the value is of type UNKNOWN, not the var's
						 * datatype, else coercion will be skipped. (Do this
						 * before the notnull check to be consistent with
						 * exec_assign_value.)
						 */
						if (!var->datatype->typinput.fn_strict)
						{
							bool		valIsNull = true;

							exec_assign_value(estate,
											  (PLpgSQL_datum *) var,
											  (Datum) 0,
											  UNKNOWNOID,
											  &valIsNull);
						}
						if (var->notnull)
							ereport(ERROR,
									(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
									 errmsg("variable \"%s\" declared NOT NULL cannot default to NULL",
											var->refname)));
					}
					else
					{
						exec_assign_expr(estate, (PLpgSQL_datum *) var,
										 var->default_val);
					}
				}
				break;

			case PLPGSQL_DTYPE_REC:
				{
					PLpgSQL_rec *rec = (PLpgSQL_rec *) (estate->datums[n]);

					if (rec->freetup)
					{
						heap_freetuple(rec->tup);
						rec->freetup = false;
					}
					if (rec->freetupdesc)
					{
						FreeTupleDesc(rec->tupdesc);
						rec->freetupdesc = false;
					}
					rec->tup = NULL;
					rec->tupdesc = NULL;
				}
				break;

			case PLPGSQL_DTYPE_RECFIELD:
			case PLPGSQL_DTYPE_ARRAYELEM:
				break;

			default:
				elog(ERROR, "unrecognized dtype: %d",
					 estate->datums[n]->dtype);
		}
	}

	if (block->exceptions)
	{
		/*
		 * Execute the statements in the block's body inside a sub-transaction
		 */
		MemoryContext oldcontext = CurrentMemoryContext;
		ResourceOwner oldowner = CurrentResourceOwner;
		ExprContext *old_eval_econtext = estate->eval_econtext;
		ErrorData  *save_cur_error = estate->cur_error;

		estate->err_text = gettext_noop("during statement block entry");

		BeginInternalSubTransaction(NULL);
		/* Want to run statements inside function's memory context */
		MemoryContextSwitchTo(oldcontext);

		PG_TRY();
		{
			/*
			 * We need to run the block's statements with a new eval_econtext
			 * that belongs to the current subtransaction; if we try to use
			 * the outer econtext then ExprContext shutdown callbacks will be
			 * called at the wrong times.
			 */
			plpgsql_create_econtext(estate);

			estate->err_text = NULL;

			/* Run the block's statements */
			rc = exec_stmts(estate, block->body);

			estate->err_text = gettext_noop("during statement block exit");

			/*
			 * If the block ended with RETURN, we may need to copy the return
			 * value out of the subtransaction eval_context.  This is
			 * currently only needed for scalar result types --- rowtype
			 * values will always exist in the function's own memory context.
			 */
			if (rc == PLPGSQL_RC_RETURN &&
				!estate->retisset &&
				!estate->retisnull &&
				estate->rettupdesc == NULL)
			{
				int16		resTypLen;
				bool		resTypByVal;

				get_typlenbyval(estate->rettype, &resTypLen, &resTypByVal);
				estate->retval = datumCopy(estate->retval,
										   resTypByVal, resTypLen);
			}

			/* Commit the inner transaction, return to outer xact context */
			ReleaseCurrentSubTransaction();
			MemoryContextSwitchTo(oldcontext);
			CurrentResourceOwner = oldowner;

			/*
			 * Revert to outer eval_econtext.  (The inner one was
			 * automatically cleaned up during subxact exit.)
			 */
			estate->eval_econtext = old_eval_econtext;

			/*
			 * AtEOSubXact_SPI() should not have popped any SPI context, but
			 * just in case it did, make sure we remain connected.
			 */
			SPI_restore_connection();
		}
		PG_CATCH();
		{
			ErrorData  *edata;
			ListCell   *e;

			estate->err_text = gettext_noop("during exception cleanup");

			/* Save error info */
			MemoryContextSwitchTo(oldcontext);
			edata = CopyErrorData();
			FlushErrorState();

			/* Abort the inner transaction */
			RollbackAndReleaseCurrentSubTransaction();
			MemoryContextSwitchTo(oldcontext);
			CurrentResourceOwner = oldowner;

			/* Revert to outer eval_econtext */
			estate->eval_econtext = old_eval_econtext;

			/*
			 * If AtEOSubXact_SPI() popped any SPI context of the subxact, it
			 * will have left us in a disconnected state.  We need this hack
			 * to return to connected state.
			 */
			SPI_restore_connection();

			/* Must clean up the econtext too */
			exec_eval_cleanup(estate);

			/* Look for a matching exception handler */
			foreach(e, block->exceptions->exc_list)
			{
				PLpgSQL_exception *exception = (PLpgSQL_exception *) lfirst(e);

				if (exception_matches_conditions(edata, exception->conditions))
				{
					/*
					 * Initialize the magic SQLSTATE and SQLERRM variables for
					 * the exception block. We needn't do this until we have
					 * found a matching exception.
					 */
					PLpgSQL_var *state_var;
					PLpgSQL_var *errm_var;

					state_var = (PLpgSQL_var *)
						estate->datums[block->exceptions->sqlstate_varno];
					errm_var = (PLpgSQL_var *)
						estate->datums[block->exceptions->sqlerrm_varno];

					assign_text_var(state_var,
									unpack_sql_state(edata->sqlerrcode));
					assign_text_var(errm_var, edata->message);

					/*
					 * Also set up cur_error so the error data is accessible
					 * inside the handler.
					 */
					estate->cur_error = edata;

					estate->err_text = NULL;

					rc = exec_stmts(estate, exception->action);

					free_var(state_var);
					state_var->value = (Datum) 0;
					state_var->isnull = true;
					free_var(errm_var);
					errm_var->value = (Datum) 0;
					errm_var->isnull = true;

					break;
				}
			}

			/*
			 * Restore previous state of cur_error, whether or not we executed
			 * a handler.  This is needed in case an error got thrown from
			 * some inner block's exception handler.
			 */
			estate->cur_error = save_cur_error;

			/* If no match found, re-throw the error */
			if (e == NULL)
				ReThrowError(edata);
			else
				FreeErrorData(edata);
		}
		PG_END_TRY();

		Assert(save_cur_error == estate->cur_error);
	}
	else
	{
		/*
		 * Just execute the statements in the block's body
		 */
		estate->err_text = NULL;

		rc = exec_stmts(estate, block->body);
	}

	estate->err_text = NULL;

	/*
	 * Handle the return code.
	 */
	switch (rc)
	{
		case PLPGSQL_RC_OK:
		case PLPGSQL_RC_RETURN:
		case PLPGSQL_RC_CONTINUE:
			return rc;

		case PLPGSQL_RC_EXIT:

			/*
			 * This is intentionally different from the handling of RC_EXIT
			 * for loops: to match a block, we require a match by label.
			 */
			if (estate->exitlabel == NULL)
				return PLPGSQL_RC_EXIT;
			if (block->label == NULL)
				return PLPGSQL_RC_EXIT;
			if (strcmp(block->label, estate->exitlabel) != 0)
				return PLPGSQL_RC_EXIT;
			estate->exitlabel = NULL;
			return PLPGSQL_RC_OK;

		default:
			elog(ERROR, "unrecognized rc: %d", rc);
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmts			Iterate over a list of statements
 *				as long as their return code is OK
 * ----------
 */
static int
exec_stmts(PLpgSQL_execstate *estate, List *stmts)
{
	ListCell   *s;

	if (stmts == NIL)
	{
		/*
		 * Ensure we do a CHECK_FOR_INTERRUPTS() even though there is no
		 * statement.  This prevents hangup in a tight loop if, for instance,
		 * there is a LOOP construct with an empty body.
		 */
		CHECK_FOR_INTERRUPTS();
		return PLPGSQL_RC_OK;
	}

	foreach(s, stmts)
	{
		PLpgSQL_stmt *stmt = (PLpgSQL_stmt *) lfirst(s);
		int			rc = exec_stmt(estate, stmt);

		if (rc != PLPGSQL_RC_OK)
			return rc;
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt			Distribute one statement to the statements
 *				type specific execution function.
 * ----------
 */
static int
exec_stmt(PLpgSQL_execstate *estate, PLpgSQL_stmt *stmt)
{
	PLpgSQL_stmt *save_estmt;
	int			rc = -1;

	save_estmt = estate->err_stmt;
	estate->err_stmt = stmt;

	/* Let the plugin know that we are about to execute this statement */
	if (*plugin_ptr && (*plugin_ptr)->stmt_beg)
		((*plugin_ptr)->stmt_beg) (estate, stmt);

	CHECK_FOR_INTERRUPTS();

	switch ((enum PLpgSQL_stmt_types) stmt->cmd_type)
	{
		case PLPGSQL_STMT_BLOCK:
			rc = exec_stmt_block(estate, (PLpgSQL_stmt_block *) stmt);
			break;

		case PLPGSQL_STMT_ASSIGN:
			rc = exec_stmt_assign(estate, (PLpgSQL_stmt_assign *) stmt);
			break;

		case PLPGSQL_STMT_PERFORM:
			rc = exec_stmt_perform(estate, (PLpgSQL_stmt_perform *) stmt);
			break;

		case PLPGSQL_STMT_GETDIAG:
			rc = exec_stmt_getdiag(estate, (PLpgSQL_stmt_getdiag *) stmt);
			break;

		case PLPGSQL_STMT_IF:
			rc = exec_stmt_if(estate, (PLpgSQL_stmt_if *) stmt);
			break;

		case PLPGSQL_STMT_CASE:
			rc = exec_stmt_case(estate, (PLpgSQL_stmt_case *) stmt);
			break;

		case PLPGSQL_STMT_LOOP:
			rc = exec_stmt_loop(estate, (PLpgSQL_stmt_loop *) stmt);
			break;

		case PLPGSQL_STMT_WHILE:
			rc = exec_stmt_while(estate, (PLpgSQL_stmt_while *) stmt);
			break;

		case PLPGSQL_STMT_FORI:
			rc = exec_stmt_fori(estate, (PLpgSQL_stmt_fori *) stmt);
			break;

		case PLPGSQL_STMT_FORS:
			rc = exec_stmt_fors(estate, (PLpgSQL_stmt_fors *) stmt);
			break;

		case PLPGSQL_STMT_FORC:
			rc = exec_stmt_forc(estate, (PLpgSQL_stmt_forc *) stmt);
			break;

		case PLPGSQL_STMT_FOREACH_A:
			rc = exec_stmt_foreach_a(estate, (PLpgSQL_stmt_foreach_a *) stmt);
			break;

		case PLPGSQL_STMT_EXIT:
			rc = exec_stmt_exit(estate, (PLpgSQL_stmt_exit *) stmt);
			break;

		case PLPGSQL_STMT_RETURN:
			rc = exec_stmt_return(estate, (PLpgSQL_stmt_return *) stmt);
			break;

		case PLPGSQL_STMT_RETURN_NEXT:
			rc = exec_stmt_return_next(estate, (PLpgSQL_stmt_return_next *) stmt);
			break;

		case PLPGSQL_STMT_RETURN_QUERY:
			rc = exec_stmt_return_query(estate, (PLpgSQL_stmt_return_query *) stmt);
			break;

		case PLPGSQL_STMT_RAISE:
			rc = exec_stmt_raise(estate, (PLpgSQL_stmt_raise *) stmt);
			break;

		case PLPGSQL_STMT_EXECSQL:
			rc = exec_stmt_execsql(estate, (PLpgSQL_stmt_execsql *) stmt);
			break;

		case PLPGSQL_STMT_DYNEXECUTE:
			rc = exec_stmt_dynexecute(estate, (PLpgSQL_stmt_dynexecute *) stmt);
			break;

		case PLPGSQL_STMT_DYNFORS:
			rc = exec_stmt_dynfors(estate, (PLpgSQL_stmt_dynfors *) stmt);
			break;

		case PLPGSQL_STMT_OPEN:
			rc = exec_stmt_open(estate, (PLpgSQL_stmt_open *) stmt);
			break;

		case PLPGSQL_STMT_FETCH:
			rc = exec_stmt_fetch(estate, (PLpgSQL_stmt_fetch *) stmt);
			break;

		case PLPGSQL_STMT_CLOSE:
			rc = exec_stmt_close(estate, (PLpgSQL_stmt_close *) stmt);
			break;

		default:
			estate->err_stmt = save_estmt;
			elog(ERROR, "unrecognized cmdtype: %d", stmt->cmd_type);
	}

	/* Let the plugin know that we have finished executing this statement */
	if (*plugin_ptr && (*plugin_ptr)->stmt_end)
		((*plugin_ptr)->stmt_end) (estate, stmt);

	estate->err_stmt = save_estmt;

	return rc;
}


/* ----------
 * exec_stmt_assign			Evaluate an expression and
 *					put the result into a variable.
 * ----------
 */
static int
exec_stmt_assign(PLpgSQL_execstate *estate, PLpgSQL_stmt_assign *stmt)
{
	Assert(stmt->varno >= 0);

	exec_assign_expr(estate, estate->datums[stmt->varno], stmt->expr);

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_perform		Evaluate query and discard result (but set
 *							FOUND depending on whether at least one row
 *							was returned).
 * ----------
 */
static int
exec_stmt_perform(PLpgSQL_execstate *estate, PLpgSQL_stmt_perform *stmt)
{
	PLpgSQL_expr *expr = stmt->expr;

	(void) exec_run_select(estate, expr, 0, NULL);
	exec_set_found(estate, (estate->eval_processed != 0));
	exec_eval_cleanup(estate);

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_getdiag					Put internal PG information into
 *										specified variables.
 * ----------
 */
static int
exec_stmt_getdiag(PLpgSQL_execstate *estate, PLpgSQL_stmt_getdiag *stmt)
{
	ListCell   *lc;

	foreach(lc, stmt->diag_items)
	{
		PLpgSQL_diag_item *diag_item = (PLpgSQL_diag_item *) lfirst(lc);
		PLpgSQL_datum *var = estate->datums[diag_item->target];
		bool		isnull = false;

		switch (diag_item->kind)
		{
			case PLPGSQL_GETDIAG_ROW_COUNT:

				exec_assign_value(estate, var,
								  UInt32GetDatum(estate->eval_processed),
								  INT4OID, &isnull);
				break;

			case PLPGSQL_GETDIAG_RESULT_OID:

				exec_assign_value(estate, var,
								  ObjectIdGetDatum(estate->eval_lastoid),
								  OIDOID, &isnull);
				break;

			default:
				elog(ERROR, "unrecognized attribute request: %d",
					 diag_item->kind);
		}
	}

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_if				Evaluate a bool expression and
 *					execute the true or false body
 *					conditionally.
 * ----------
 */
static int
exec_stmt_if(PLpgSQL_execstate *estate, PLpgSQL_stmt_if *stmt)
{
	bool		value;
	bool		isnull;

	value = exec_eval_boolean(estate, stmt->cond, &isnull);
	exec_eval_cleanup(estate);

	if (!isnull && value)
	{
		if (stmt->true_body != NIL)
			return exec_stmts(estate, stmt->true_body);
	}
	else
	{
		if (stmt->false_body != NIL)
			return exec_stmts(estate, stmt->false_body);
	}

	return PLPGSQL_RC_OK;
}


/*-----------
 * exec_stmt_case
 *-----------
 */
static int
exec_stmt_case(PLpgSQL_execstate *estate, PLpgSQL_stmt_case *stmt)
{
	PLpgSQL_var *t_var = NULL;
	bool		isnull;
	ListCell   *l;

	if (stmt->t_expr != NULL)
	{
		/* simple case */
		Datum		t_val;
		Oid			t_oid;

		t_val = exec_eval_expr(estate, stmt->t_expr, &isnull, &t_oid);

		t_var = (PLpgSQL_var *) estate->datums[stmt->t_varno];

		/*
		 * When expected datatype is different from real, change it. Note that
		 * what we're modifying here is an execution copy of the datum, so
		 * this doesn't affect the originally stored function parse tree.
		 */
		if (t_var->datatype->typoid != t_oid)
			t_var->datatype = plpgsql_build_datatype(t_oid,
													 -1,
										   estate->func->fn_input_collation);

		/* now we can assign to the variable */
		exec_assign_value(estate,
						  (PLpgSQL_datum *) t_var,
						  t_val,
						  t_oid,
						  &isnull);

		exec_eval_cleanup(estate);
	}

	/* Now search for a successful WHEN clause */
	foreach(l, stmt->case_when_list)
	{
		PLpgSQL_case_when *cwt = (PLpgSQL_case_when *) lfirst(l);
		bool		value;

		value = exec_eval_boolean(estate, cwt->expr, &isnull);
		exec_eval_cleanup(estate);
		if (!isnull && value)
		{
			/* Found it */

			/* We can now discard any value we had for the temp variable */
			if (t_var != NULL)
			{
				free_var(t_var);
				t_var->value = (Datum) 0;
				t_var->isnull = true;
			}

			/* Evaluate the statement(s), and we're done */
			return exec_stmts(estate, cwt->stmts);
		}
	}

	/* We can now discard any value we had for the temp variable */
	if (t_var != NULL)
	{
		free_var(t_var);
		t_var->value = (Datum) 0;
		t_var->isnull = true;
	}

	/* SQL2003 mandates this error if there was no ELSE clause */
	if (!stmt->have_else)
		ereport(ERROR,
				(errcode(ERRCODE_CASE_NOT_FOUND),
				 errmsg("case not found"),
				 errhint("CASE statement is missing ELSE part.")));

	/* Evaluate the ELSE statements, and we're done */
	return exec_stmts(estate, stmt->else_stmts);
}


/* ----------
 * exec_stmt_loop			Loop over statements until
 *					an exit occurs.
 * ----------
 */
static int
exec_stmt_loop(PLpgSQL_execstate *estate, PLpgSQL_stmt_loop *stmt)
{
	for (;;)
	{
		int			rc = exec_stmts(estate, stmt->body);

		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel) != 0)
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_CONTINUE:
				if (estate->exitlabel == NULL)
					/* anonymous continue, so re-run the loop */
					break;
				else if (stmt->label != NULL &&
						 strcmp(stmt->label, estate->exitlabel) == 0)
					/* label matches named continue, so re-run loop */
					estate->exitlabel = NULL;
				else
					/* label doesn't match named continue, so propagate upward */
					return PLPGSQL_RC_CONTINUE;
				break;

			case PLPGSQL_RC_RETURN:
				return rc;

			default:
				elog(ERROR, "unrecognized rc: %d", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_while			Loop over statements as long
 *					as an expression evaluates to
 *					true or an exit occurs.
 * ----------
 */
static int
exec_stmt_while(PLpgSQL_execstate *estate, PLpgSQL_stmt_while *stmt)
{
	for (;;)
	{
		int			rc;
		bool		value;
		bool		isnull;

		value = exec_eval_boolean(estate, stmt->cond, &isnull);
		exec_eval_cleanup(estate);

		if (isnull || !value)
			break;

		rc = exec_stmts(estate, stmt->body);

		switch (rc)
		{
			case PLPGSQL_RC_OK:
				break;

			case PLPGSQL_RC_EXIT:
				if (estate->exitlabel == NULL)
					return PLPGSQL_RC_OK;
				if (stmt->label == NULL)
					return PLPGSQL_RC_EXIT;
				if (strcmp(stmt->label, estate->exitlabel) != 0)
					return PLPGSQL_RC_EXIT;
				estate->exitlabel = NULL;
				return PLPGSQL_RC_OK;

			case PLPGSQL_RC_CONTINUE:
				if (estate->exitlabel == NULL)
					/* anonymous continue, so re-run loop */
					break;
				else if (stmt->label != NULL &&
						 strcmp(stmt->label, estate->exitlabel) == 0)
					/* label matches named continue, so re-run loop */
					estate->exitlabel = NULL;
				else
					/* label doesn't match named continue, propagate upward */
					return PLPGSQL_RC_CONTINUE;
				break;

			case PLPGSQL_RC_RETURN:
				return rc;

			default:
				elog(ERROR, "unrecognized rc: %d", rc);
		}
	}

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_fori			Iterate an integer variable
 *					from a lower to an upper value
 *					incrementing or decrementing by the BY value
 * ----------
 */
static int
exec_stmt_fori(PLpgSQL_execstate *estate, PLpgSQL_stmt_fori *stmt)
{
	PLpgSQL_var *var;
	Datum		value;
	bool		isnull;
	Oid			valtype;
	int32		loop_value;
	int32		end_value;
	int32		step_value;
	bool		found = false;
	int			rc = PLPGSQL_RC_OK;

	var = (PLpgSQL_var *) (estate->datums[stmt->var->dno]);

	/*
	 * Get the value of the lower bound
	 */
	value = exec_eval_expr(estate, stmt->lower, &isnull, &valtype);
	value = exec_cast_value(estate, value, valtype, var->datatype->typoid,
							&(var->datatype->typinput),
							var->datatype->typioparam,
							var->datatype->atttypmod, isnull);
	if (isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("lower bound of FOR loop cannot be null")));
	loop_value = DatumGetInt32(value);
	exec_eval_cleanup(estate);

	/*
	 * Get the value of the upper bound
	 */
	value = exec_eval_expr(estate, stmt->upper, &isnull, &valtype);
	value = exec_cast_value(estate, value, valtype, var->datatype->typoid,
							&(var->datatype->typinput),
							var->datatype->typioparam,
							var->datatype->atttypmod, isnull);
	if (isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("upper bound of FOR loop cannot be null")));
	end_value = DatumGetInt32(value);
	exec_eval_cleanup(estate);

	/*
	 * Get the step value
	 */
	if (stmt->step)
	{
		value = exec_eval_expr(estate, stmt->step, &isnull, &valtype);
		value = exec_cast_value(estate, value, valtype, var->datatype->typoid,
								&(var->datatype->typinput),
								var->datatype->typioparam,
								var->datatype->atttypmod, isnull);
		if (isnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("BY value of FOR loop cannot be null")));
		step_value = DatumGetInt32(value);
		exec_eval_cleanup(estate);
		if (step_value <= 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				  errmsg("BY value of FOR loop must be greater than zero")));
	}
	else
		step_value = 1;

	/*
	 * Now do the loop
	 */
	for (;;)
	{
		/*
		 * Check against upper bound
		 */
		if (stmt->reverse)
		{
			if (loop_value < end_value)
				break;
		}
		else
		{
			if (loop_value > end_value)
				break;
		}

		found = true;			/* looped at least once */

		/*
		 * Assign current value to loop var
		 */
		var->value = Int32GetDatum(loop_value);
		var->isnull = false;

		/*
		 * Execute the statements
		 */
		rc = exec_stmts(estate, stmt->body);

		if (rc == PLPGSQL_RC_RETURN)
			break;				/* break out of the loop */
		else if (rc == PLPGSQL_RC_EXIT)
		{
			if (estate->exitlabel == NULL)
				/* unlabelled exit, finish the current loop */
				rc = PLPGSQL_RC_OK;
			else if (stmt->label != NULL &&
					 strcmp(stmt->label, estate->exitlabel) == 0)
			{
				/* labelled exit, matches the current stmt's label */
				estate->exitlabel = NULL;
				rc = PLPGSQL_RC_OK;
			}

			/*
			 * otherwise, this is a labelled exit that does not match the
			 * current statement's label, if any: return RC_EXIT so that the
			 * EXIT continues to propagate up the stack.
			 */
			break;
		}
		else if (rc == PLPGSQL_RC_CONTINUE)
		{
			if (estate->exitlabel == NULL)
				/* unlabelled continue, so re-run the current loop */
				rc = PLPGSQL_RC_OK;
			else if (stmt->label != NULL &&
					 strcmp(stmt->label, estate->exitlabel) == 0)
			{
				/* label matches named continue, so re-run loop */
				estate->exitlabel = NULL;
				rc = PLPGSQL_RC_OK;
			}
			else
			{
				/*
				 * otherwise, this is a named continue that does not match the
				 * current statement's label, if any: return RC_CONTINUE so
				 * that the CONTINUE will propagate up the stack.
				 */
				break;
			}
		}

		/*
		 * Increase/decrease loop value, unless it would overflow, in which
		 * case exit the loop.
		 */
		if (stmt->reverse)
		{
			if ((int32) (loop_value - step_value) > loop_value)
				break;
			loop_value -= step_value;
		}
		else
		{
			if ((int32) (loop_value + step_value) < loop_value)
				break;
			loop_value += step_value;
		}
	}

	/*
	 * Set the FOUND variable to indicate the result of executing the loop
	 * (namely, whether we looped one or more times). This must be set here so
	 * that it does not interfere with the value of the FOUND variable inside
	 * the loop processing itself.
	 */
	exec_set_found(estate, found);

	return rc;
}


/* ----------
 * exec_stmt_fors			Execute a query, assign each
 *					tuple to a record or row and
 *					execute a group of statements
 *					for it.
 * ----------
 */
static int
exec_stmt_fors(PLpgSQL_execstate *estate, PLpgSQL_stmt_fors *stmt)
{
	Portal		portal;
	int			rc;

	/*
	 * Open the implicit cursor for the statement using exec_run_select
	 */
	exec_run_select(estate, stmt->query, 0, &portal);

	/*
	 * Execute the loop
	 */
	rc = exec_for_query(estate, (PLpgSQL_stmt_forq *) stmt, portal, true);

	/*
	 * Close the implicit cursor
	 */
	SPI_cursor_close(portal);

	return rc;
}


/* ----------
 * exec_stmt_forc			Execute a loop for each row from a cursor.
 * ----------
 */
static int
exec_stmt_forc(PLpgSQL_execstate *estate, PLpgSQL_stmt_forc *stmt)
{
	PLpgSQL_var *curvar;
	char	   *curname = NULL;
	PLpgSQL_expr *query;
	ParamListInfo paramLI;
	Portal		portal;
	int			rc;

	/* ----------
	 * Get the cursor variable and if it has an assigned name, check
	 * that it's not in use currently.
	 * ----------
	 */
	curvar = (PLpgSQL_var *) (estate->datums[stmt->curvar]);
	if (!curvar->isnull)
	{
		curname = TextDatumGetCString(curvar->value);
		if (SPI_cursor_find(curname) != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_CURSOR),
					 errmsg("cursor \"%s\" already in use", curname)));
	}

	/* ----------
	 * Open the cursor just like an OPEN command
	 *
	 * Note: parser should already have checked that statement supplies
	 * args iff cursor needs them, but we check again to be safe.
	 * ----------
	 */
	if (stmt->argquery != NULL)
	{
		/* ----------
		 * OPEN CURSOR with args.  We fake a SELECT ... INTO ...
		 * statement to evaluate the args and put 'em into the
		 * internal row.
		 * ----------
		 */
		PLpgSQL_stmt_execsql set_args;

		if (curvar->cursor_explicit_argrow < 0)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("arguments given for cursor without arguments")));

		memset(&set_args, 0, sizeof(set_args));
		set_args.cmd_type = PLPGSQL_STMT_EXECSQL;
		set_args.lineno = stmt->lineno;
		set_args.sqlstmt = stmt->argquery;
		set_args.into = true;
		/* XXX historically this has not been STRICT */
		set_args.row = (PLpgSQL_row *)
			(estate->datums[curvar->cursor_explicit_argrow]);

		if (exec_stmt_execsql(estate, &set_args) != PLPGSQL_RC_OK)
			elog(ERROR, "open cursor failed during argument processing");
	}
	else
	{
		if (curvar->cursor_explicit_argrow >= 0)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("arguments required for cursor")));
	}

	query = curvar->cursor_explicit_expr;
	Assert(query);

	if (query->plan == NULL)
		exec_prepare_plan(estate, query, curvar->cursor_options);

	/*
	 * Set up ParamListInfo (note this is only carrying a hook function, not
	 * any actual data values, at this point)
	 */
	paramLI = setup_param_list(estate, query);

	/*
	 * Open the cursor (the paramlist will get copied into the portal)
	 */
	portal = SPI_cursor_open_with_paramlist(curname, query->plan,
											paramLI,
											estate->readonly_func);
	if (portal == NULL)
		elog(ERROR, "could not open cursor: %s",
			 SPI_result_code_string(SPI_result));

	/* don't need paramlist any more */
	if (paramLI)
		pfree(paramLI);

	/*
	 * If cursor variable was NULL, store the generated portal name in it
	 */
	if (curname == NULL)
		assign_text_var(curvar, portal->name);

	/*
	 * Execute the loop.  We can't prefetch because the cursor is accessible
	 * to the user, for instance via UPDATE WHERE CURRENT OF within the loop.
	 */
	rc = exec_for_query(estate, (PLpgSQL_stmt_forq *) stmt, portal, false);

	/* ----------
	 * Close portal, and restore cursor variable if it was initially NULL.
	 * ----------
	 */
	SPI_cursor_close(portal);

	if (curname == NULL)
	{
		free_var(curvar);
		curvar->value = (Datum) 0;
		curvar->isnull = true;
	}

	if (curname)
		pfree(curname);

	return rc;
}


/* ----------
 * exec_stmt_foreach_a			Loop over elements or slices of an array
 *
 * When looping over elements, the loop variable is the same type that the
 * array stores (eg: integer), when looping through slices, the loop variable
 * is an array of size and dimensions to match the size of the slice.
 * ----------
 */
static int
exec_stmt_foreach_a(PLpgSQL_execstate *estate, PLpgSQL_stmt_foreach_a *stmt)
{
	ArrayType  *arr;
	Oid			arrtype;
	PLpgSQL_datum *loop_var;
	Oid			loop_var_elem_type;
	bool		found = false;
	int			rc = PLPGSQL_RC_OK;
	ArrayIterator array_iterator;
	Oid			iterator_result_type;
	Datum		value;
	bool		isnull;

	/* get the value of the array expression */
	value = exec_eval_expr(estate, stmt->expr, &isnull, &arrtype);
	if (isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("FOREACH expression must not be null")));

	/* check the type of the expression - must be an array */
	if (!OidIsValid(get_element_type(arrtype)))
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
				 errmsg("FOREACH expression must yield an array, not type %s",
						format_type_be(arrtype))));

	/*
	 * We must copy the array, else it will disappear in exec_eval_cleanup.
	 * This is annoying, but cleanup will certainly happen while running the
	 * loop body, so we have little choice.
	 */
	arr = DatumGetArrayTypePCopy(value);

	/* Clean up any leftover temporary memory */
	exec_eval_cleanup(estate);

	/* Slice dimension must be less than or equal to array dimension */
	if (stmt->slice < 0 || stmt->slice > ARR_NDIM(arr))
		ereport(ERROR,
				(errcode(ERRCODE_ARRAY_SUBSCRIPT_ERROR),
			   errmsg("slice dimension (%d) is out of the valid range 0..%d",
					  stmt->slice, ARR_NDIM(arr))));

	/* Set up the loop variable and see if it is of an array type */
	loop_var = estate->datums[stmt->varno];
	if (loop_var->dtype == PLPGSQL_DTYPE_REC ||
		loop_var->dtype == PLPGSQL_DTYPE_ROW)
	{
		/*
		 * Record/row variable is certainly not of array type, and might not
		 * be initialized at all yet, so don't try to get its type
		 */
		loop_var_elem_type = InvalidOid;
	}
	else
		loop_var_elem_type = get_element_type(exec_get_datum_type(estate,
																  loop_var));

	/*
	 * Sanity-check the loop variable type.  We don't try very hard here, and
	 * should not be too picky since it's possible that exec_assign_value can
	 * coerce values of different types.  But it seems worthwhile to complain
	 * if the array-ness of the loop variable is not right.
	 */
	if (stmt->slice > 0 && loop_var_elem_type == InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
		errmsg("FOREACH ... SLICE loop variable must be of an array type")));
	if (stmt->slice == 0 && loop_var_elem_type != InvalidOid)
		ereport(ERROR,
				(errcode(ERRCODE_DATATYPE_MISMATCH),
			  errmsg("FOREACH loop variable must not be of an array type")));

	/* Create an iterator to step through the array */
	array_iterator = array_create_iterator(arr, stmt->slice);

	/* Identify iterator result type */
	if (stmt->slice > 0)
	{
		/* When slicing, nominal type of result is same as array type */
		iterator_result_type = arrtype;
	}
	else
	{
		/* Without slicing, results are individual array elements */
		iterator_result_type = ARR_ELEMTYPE(arr);
	}

	/* Iterate over the array elements or slices */
	while (array_iterate(array_iterator, &value, &isnull))
	{
		found = true;			/* looped at least once */

		/* Assign current element/slice to the loop variable */
		exec_assign_value(estate, loop_var, value, iterator_result_type,
						  &isnull);

		/* In slice case, value is temporary; must free it to avoid leakage */
		if (stmt->slice > 0)
			pfree(DatumGetPointer(value));

		/*
		 * Execute the statements
		 */
		rc = exec_stmts(estate, stmt->body);

		/* Handle the return code */
		if (rc == PLPGSQL_RC_RETURN)
			break;				/* break out of the loop */
		else if (rc == PLPGSQL_RC_EXIT)
		{
			if (estate->exitlabel == NULL)
				/* unlabelled exit, finish the current loop */
				rc = PLPGSQL_RC_OK;
			else if (stmt->label != NULL &&
					 strcmp(stmt->label, estate->exitlabel) == 0)
			{
				/* labelled exit, matches the current stmt's label */
				estate->exitlabel = NULL;
				rc = PLPGSQL_RC_OK;
			}

			/*
			 * otherwise, this is a labelled exit that does not match the
			 * current statement's label, if any: return RC_EXIT so that the
			 * EXIT continues to propagate up the stack.
			 */
			break;
		}
		else if (rc == PLPGSQL_RC_CONTINUE)
		{
			if (estate->exitlabel == NULL)
				/* unlabelled continue, so re-run the current loop */
				rc = PLPGSQL_RC_OK;
			else if (stmt->label != NULL &&
					 strcmp(stmt->label, estate->exitlabel) == 0)
			{
				/* label matches named continue, so re-run loop */
				estate->exitlabel = NULL;
				rc = PLPGSQL_RC_OK;
			}
			else
			{
				/*
				 * otherwise, this is a named continue that does not match the
				 * current statement's label, if any: return RC_CONTINUE so
				 * that the CONTINUE will propagate up the stack.
				 */
				break;
			}
		}
	}

	/* Release temporary memory, including the array value */
	array_free_iterator(array_iterator);
	pfree(arr);

	/*
	 * Set the FOUND variable to indicate the result of executing the loop
	 * (namely, whether we looped one or more times). This must be set here so
	 * that it does not interfere with the value of the FOUND variable inside
	 * the loop processing itself.
	 */
	exec_set_found(estate, found);

	return rc;
}


/* ----------
 * exec_stmt_exit			Implements EXIT and CONTINUE
 *
 * This begins the process of exiting / restarting a loop.
 * ----------
 */
static int
exec_stmt_exit(PLpgSQL_execstate *estate, PLpgSQL_stmt_exit *stmt)
{
	/*
	 * If the exit / continue has a condition, evaluate it
	 */
	if (stmt->cond != NULL)
	{
		bool		value;
		bool		isnull;

		value = exec_eval_boolean(estate, stmt->cond, &isnull);
		exec_eval_cleanup(estate);
		if (isnull || value == false)
			return PLPGSQL_RC_OK;
	}

	estate->exitlabel = stmt->label;
	if (stmt->is_exit)
		return PLPGSQL_RC_EXIT;
	else
		return PLPGSQL_RC_CONTINUE;
}


/* ----------
 * exec_stmt_return			Evaluate an expression and start
 *					returning from the function.
 * ----------
 */
static int
exec_stmt_return(PLpgSQL_execstate *estate, PLpgSQL_stmt_return *stmt)
{
	/*
	 * If processing a set-returning PL/pgSQL function, the final RETURN
	 * indicates that the function is finished producing tuples.  The rest of
	 * the work will be done at the top level.
	 */
	if (estate->retisset)
		return PLPGSQL_RC_RETURN;

	/* initialize for null result (possibly a tuple) */
	estate->retval = (Datum) 0;
	estate->rettupdesc = NULL;
	estate->retisnull = true;

	if (stmt->retvarno >= 0)
	{
		PLpgSQL_datum *retvar = estate->datums[stmt->retvarno];

		switch (retvar->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) retvar;

					estate->retval = var->value;
					estate->retisnull = var->isnull;
					estate->rettype = var->datatype->typoid;
				}
				break;

			case PLPGSQL_DTYPE_REC:
				{
					PLpgSQL_rec *rec = (PLpgSQL_rec *) retvar;

					if (HeapTupleIsValid(rec->tup))
					{
						estate->retval = PointerGetDatum(rec->tup);
						estate->rettupdesc = rec->tupdesc;
						estate->retisnull = false;
					}
				}
				break;

			case PLPGSQL_DTYPE_ROW:
				{
					PLpgSQL_row *row = (PLpgSQL_row *) retvar;

					Assert(row->rowtupdesc);
					estate->retval =
						PointerGetDatum(make_tuple_from_row(estate, row,
															row->rowtupdesc));
					if (DatumGetPointer(estate->retval) == NULL)		/* should not happen */
						elog(ERROR, "row not compatible with its own tupdesc");
					estate->rettupdesc = row->rowtupdesc;
					estate->retisnull = false;
				}
				break;

			default:
				elog(ERROR, "unrecognized dtype: %d", retvar->dtype);
		}

		return PLPGSQL_RC_RETURN;
	}

	if (stmt->expr != NULL)
	{
		if (estate->retistuple)
		{
			exec_run_select(estate, stmt->expr, 1, NULL);
			if (estate->eval_processed > 0)
			{
				estate->retval = PointerGetDatum(estate->eval_tuptable->vals[0]);
				estate->rettupdesc = estate->eval_tuptable->tupdesc;
				estate->retisnull = false;
			}
		}
		else
		{
			/* Normal case for scalar results */
			estate->retval = exec_eval_expr(estate, stmt->expr,
											&(estate->retisnull),
											&(estate->rettype));
		}

		return PLPGSQL_RC_RETURN;
	}

	/*
	 * Special hack for function returning VOID: instead of NULL, return a
	 * non-null VOID value.  This is of dubious importance but is kept for
	 * backwards compatibility.  Note that the only other way to get here is
	 * to have written "RETURN NULL" in a function returning tuple.
	 */
	if (estate->fn_rettype == VOIDOID)
	{
		estate->retval = (Datum) 0;
		estate->retisnull = false;
		estate->rettype = VOIDOID;
	}

	return PLPGSQL_RC_RETURN;
}

/* ----------
 * exec_stmt_return_next		Evaluate an expression and add it to the
 *								list of tuples returned by the current
 *								SRF.
 * ----------
 */
static int
exec_stmt_return_next(PLpgSQL_execstate *estate,
					  PLpgSQL_stmt_return_next *stmt)
{
	TupleDesc	tupdesc;
	int			natts;
	HeapTuple	tuple = NULL;
	bool		free_tuple = false;

	if (!estate->retisset)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("cannot use RETURN NEXT in a non-SETOF function")));

	if (estate->tuple_store == NULL)
		exec_init_tuple_store(estate);

	/* rettupdesc will be filled by exec_init_tuple_store */
	tupdesc = estate->rettupdesc;
	natts = tupdesc->natts;

	if (stmt->retvarno >= 0)
	{
		PLpgSQL_datum *retvar = estate->datums[stmt->retvarno];

		switch (retvar->dtype)
		{
			case PLPGSQL_DTYPE_VAR:
				{
					PLpgSQL_var *var = (PLpgSQL_var *) retvar;
					Datum		retval = var->value;
					bool		isNull = var->isnull;

					if (natts != 1)
						ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
						errmsg("wrong result type supplied in RETURN NEXT")));

					/* coerce type if needed */
					retval = exec_simple_cast_value(estate,
													retval,
													var->datatype->typoid,
												 tupdesc->attrs[0]->atttypid,
												tupdesc->attrs[0]->atttypmod,
													isNull);

					tuplestore_putvalues(estate->tuple_store, tupdesc,
										 &retval, &isNull);
				}
				break;

			case PLPGSQL_DTYPE_REC:
				{
					PLpgSQL_rec *rec = (PLpgSQL_rec *) retvar;
					TupleConversionMap *tupmap;

					if (!HeapTupleIsValid(rec->tup))
						ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						errdetail("The tuple structure of a not-yet-assigned"
								  " record is indeterminate.")));
					tupmap = convert_tuples_by_position(rec->tupdesc,
														tupdesc,
														gettext_noop("wrong record type supplied in RETURN NEXT"));
					tuple = rec->tup;
					/* it might need conversion */
					if (tupmap)
					{
						tuple = do_convert_tuple(tuple, tupmap);
						free_conversion_map(tupmap);
						free_tuple = true;
					}
				}
				break;

			case PLPGSQL_DTYPE_ROW:
				{
					PLpgSQL_row *row = (PLpgSQL_row *) retvar;

					tuple = make_tuple_from_row(estate, row, tupdesc);
					if (tuple == NULL)
						ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
						errmsg("wrong record type supplied in RETURN NEXT")));
					free_tuple = true;
				}
				break;

			default:
				elog(ERROR, "unrecognized dtype: %d", retvar->dtype);
				break;
		}
	}
	else if (stmt->expr)
	{
		Datum		retval;
		bool		isNull;
		Oid			rettype;

		if (natts != 1)
			ereport(ERROR,
					(errcode(ERRCODE_DATATYPE_MISMATCH),
					 errmsg("wrong result type supplied in RETURN NEXT")));

		retval = exec_eval_expr(estate,
								stmt->expr,
								&isNull,
								&rettype);

		/* coerce type if needed */
		retval = exec_simple_cast_value(estate,
										retval,
										rettype,
										tupdesc->attrs[0]->atttypid,
										tupdesc->attrs[0]->atttypmod,
										isNull);

		tuplestore_putvalues(estate->tuple_store, tupdesc,
							 &retval, &isNull);
	}
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("RETURN NEXT must have a parameter")));
	}

	if (HeapTupleIsValid(tuple))
	{
		tuplestore_puttuple(estate->tuple_store, tuple);

		if (free_tuple)
			heap_freetuple(tuple);
	}

	exec_eval_cleanup(estate);

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_return_query		Evaluate a query and add it to the
 *								list of tuples returned by the current
 *								SRF.
 * ----------
 */
static int
exec_stmt_return_query(PLpgSQL_execstate *estate,
					   PLpgSQL_stmt_return_query *stmt)
{
	Portal		portal;
	uint32		processed = 0;
	TupleConversionMap *tupmap;

	if (!estate->retisset)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("cannot use RETURN QUERY in a non-SETOF function")));

	if (estate->tuple_store == NULL)
		exec_init_tuple_store(estate);

	if (stmt->query != NULL)
	{
		/* static query */
		exec_run_select(estate, stmt->query, 0, &portal);
	}
	else
	{
		/* RETURN QUERY EXECUTE */
		Assert(stmt->dynquery != NULL);
		portal = exec_dynquery_with_params(estate, stmt->dynquery,
										   stmt->params, NULL, 0);
	}

	tupmap = convert_tuples_by_position(portal->tupDesc,
										estate->rettupdesc,
	 gettext_noop("structure of query does not match function result type"));

	while (true)
	{
		int			i;

		SPI_cursor_fetch(portal, true, 50);
		if (SPI_processed == 0)
			break;

		for (i = 0; i < SPI_processed; i++)
		{
			HeapTuple	tuple = SPI_tuptable->vals[i];

			if (tupmap)
				tuple = do_convert_tuple(tuple, tupmap);
			tuplestore_puttuple(estate->tuple_store, tuple);
			if (tupmap)
				heap_freetuple(tuple);
			processed++;
		}

		SPI_freetuptable(SPI_tuptable);
	}

	if (tupmap)
		free_conversion_map(tupmap);

	SPI_freetuptable(SPI_tuptable);
	SPI_cursor_close(portal);

	estate->eval_processed = processed;
	exec_set_found(estate, processed != 0);

	return PLPGSQL_RC_OK;
}

static void
exec_init_tuple_store(PLpgSQL_execstate *estate)
{
	ReturnSetInfo *rsi = estate->rsi;
	MemoryContext oldcxt;
	ResourceOwner oldowner;

	/*
	 * Check caller can handle a set result in the way we want
	 */
	if (!rsi || !IsA(rsi, ReturnSetInfo) ||
		(rsi->allowedModes & SFRM_Materialize) == 0 ||
		rsi->expectedDesc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));

	/*
	 * Switch to the right memory context and resource owner for storing the
	 * tuplestore for return set. If we're within a subtransaction opened for
	 * an exception-block, for example, we must still create the tuplestore in
	 * the resource owner that was active when this function was entered, and
	 * not in the subtransaction resource owner.
	 */
	oldcxt = MemoryContextSwitchTo(estate->tuple_store_cxt);
	oldowner = CurrentResourceOwner;
	CurrentResourceOwner = estate->tuple_store_owner;

	estate->tuple_store =
		tuplestore_begin_heap(rsi->allowedModes & SFRM_Materialize_Random,
							  false, work_mem);

	CurrentResourceOwner = oldowner;
	MemoryContextSwitchTo(oldcxt);

	estate->rettupdesc = rsi->expectedDesc;
}

/* ----------
 * exec_stmt_raise			Build a message and throw it with elog()
 * ----------
 */
static int
exec_stmt_raise(PLpgSQL_execstate *estate, PLpgSQL_stmt_raise *stmt)
{
	int			err_code = 0;
	char	   *condname = NULL;
	char	   *err_message = NULL;
	char	   *err_detail = NULL;
	char	   *err_hint = NULL;
	ListCell   *lc;

	/* RAISE with no parameters: re-throw current exception */
	if (stmt->condname == NULL && stmt->message == NULL &&
		stmt->options == NIL)
	{
		if (estate->cur_error != NULL)
			ReThrowError(estate->cur_error);
		/* oops, we're not inside a handler */
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("RAISE without parameters cannot be used outside an exception handler")));
	}

	if (stmt->condname)
	{
		err_code = plpgsql_recognize_err_condition(stmt->condname, true);
		condname = pstrdup(stmt->condname);
	}

	if (stmt->message)
	{
		StringInfoData ds;
		ListCell   *current_param;
		char	   *cp;

		initStringInfo(&ds);
		current_param = list_head(stmt->params);

		for (cp = stmt->message; *cp; cp++)
		{
			/*
			 * Occurrences of a single % are replaced by the next parameter's
			 * external representation. Double %'s are converted to one %.
			 */
			if (cp[0] == '%')
			{
				Oid			paramtypeid;
				Datum		paramvalue;
				bool		paramisnull;
				char	   *extval;

				if (cp[1] == '%')
				{
					appendStringInfoChar(&ds, '%');
					cp++;
					continue;
				}

				if (current_param == NULL)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
						  errmsg("too few parameters specified for RAISE")));

				paramvalue = exec_eval_expr(estate,
									  (PLpgSQL_expr *) lfirst(current_param),
											&paramisnull,
											&paramtypeid);

				if (paramisnull)
					extval = "<NULL>";
				else
					extval = convert_value_to_string(estate,
													 paramvalue,
													 paramtypeid);
				appendStringInfoString(&ds, extval);
				current_param = lnext(current_param);
				exec_eval_cleanup(estate);
			}
			else
				appendStringInfoChar(&ds, cp[0]);
		}

		/*
		 * If more parameters were specified than were required to process the
		 * format string, throw an error
		 */
		if (current_param != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("too many parameters specified for RAISE")));

		err_message = ds.data;
		/* No pfree(ds.data), the pfree(err_message) does it */
	}

	foreach(lc, stmt->options)
	{
		PLpgSQL_raise_option *opt = (PLpgSQL_raise_option *) lfirst(lc);
		Datum		optionvalue;
		bool		optionisnull;
		Oid			optiontypeid;
		char	   *extval;

		optionvalue = exec_eval_expr(estate, opt->expr,
									 &optionisnull,
									 &optiontypeid);
		if (optionisnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("RAISE statement option cannot be null")));

		extval = convert_value_to_string(estate, optionvalue, optiontypeid);

		switch (opt->opt_type)
		{
			case PLPGSQL_RAISEOPTION_ERRCODE:
				if (err_code)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("RAISE option already specified: %s",
									"ERRCODE")));
				err_code = plpgsql_recognize_err_condition(extval, true);
				condname = pstrdup(extval);
				break;
			case PLPGSQL_RAISEOPTION_MESSAGE:
				if (err_message)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("RAISE option already specified: %s",
									"MESSAGE")));
				err_message = pstrdup(extval);
				break;
			case PLPGSQL_RAISEOPTION_DETAIL:
				if (err_detail)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("RAISE option already specified: %s",
									"DETAIL")));
				err_detail = pstrdup(extval);
				break;
			case PLPGSQL_RAISEOPTION_HINT:
				if (err_hint)
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("RAISE option already specified: %s",
									"HINT")));
				err_hint = pstrdup(extval);
				break;
			default:
				elog(ERROR, "unrecognized raise option: %d", opt->opt_type);
		}

		exec_eval_cleanup(estate);
	}

	/* Default code if nothing specified */
	if (err_code == 0 && stmt->elog_level >= ERROR)
		err_code = ERRCODE_RAISE_EXCEPTION;

	/* Default error message if nothing specified */
	if (err_message == NULL)
	{
		if (condname)
		{
			err_message = condname;
			condname = NULL;
		}
		else
			err_message = pstrdup(unpack_sql_state(err_code));
	}

	/*
	 * Throw the error (may or may not come back)
	 */
	estate->err_text = raise_skip_msg;	/* suppress traceback of raise */

	ereport(stmt->elog_level,
			(err_code ? errcode(err_code) : 0,
			 errmsg_internal("%s", err_message),
			 (err_detail != NULL) ? errdetail_internal("%s", err_detail) : 0,
			 (err_hint != NULL) ? errhint("%s", err_hint) : 0));

	estate->err_text = NULL;	/* un-suppress... */

	if (condname != NULL)
		pfree(condname);
	if (err_message != NULL)
		pfree(err_message);
	if (err_detail != NULL)
		pfree(err_detail);
	if (err_hint != NULL)
		pfree(err_hint);

	return PLPGSQL_RC_OK;
}


/* ----------
 * Initialize a mostly empty execution state
 * ----------
 */
static void
plpgsql_estate_setup(PLpgSQL_execstate *estate,
					 PLpgSQL_function *func,
					 ReturnSetInfo *rsi)
{
	/* this link will be restored at exit from plpgsql_call_handler */
	func->cur_estate = estate;

	estate->func = func;

	estate->retval = (Datum) 0;
	estate->retisnull = true;
	estate->rettype = InvalidOid;

	estate->fn_rettype = func->fn_rettype;
	estate->retistuple = func->fn_retistuple;
	estate->retisset = func->fn_retset;

	estate->readonly_func = func->fn_readonly;

	estate->rettupdesc = NULL;
	estate->exitlabel = NULL;
	estate->cur_error = NULL;

	estate->tuple_store = NULL;
	if (rsi)
	{
		estate->tuple_store_cxt = rsi->econtext->ecxt_per_query_memory;
		estate->tuple_store_owner = CurrentResourceOwner;
	}
	else
	{
		estate->tuple_store_cxt = NULL;
		estate->tuple_store_owner = NULL;
	}
	estate->rsi = rsi;

	estate->found_varno = func->found_varno;
	estate->ndatums = func->ndatums;
	estate->datums = palloc(sizeof(PLpgSQL_datum *) * estate->ndatums);
	/* caller is expected to fill the datums array */

	estate->eval_tuptable = NULL;
	estate->eval_processed = 0;
	estate->eval_lastoid = InvalidOid;
	estate->eval_econtext = NULL;
	estate->cur_expr = NULL;

	estate->err_stmt = NULL;
	estate->err_text = NULL;

	estate->plugin_info = NULL;

	/*
	 * Create an EState and ExprContext for evaluation of simple expressions.
	 */
	plpgsql_create_econtext(estate);

	/*
	 * Let the plugin see this function before we initialize any local
	 * PL/pgSQL variables - note that we also give the plugin a few function
	 * pointers so it can call back into PL/pgSQL for doing things like
	 * variable assignments and stack traces
	 */
	if (*plugin_ptr)
	{
		(*plugin_ptr)->error_callback = plpgsql_exec_error_callback;
		(*plugin_ptr)->assign_expr = exec_assign_expr;

		if ((*plugin_ptr)->func_setup)
			((*plugin_ptr)->func_setup) (estate, func);
	}
}

/* ----------
 * Release temporary memory used by expression/subselect evaluation
 *
 * NB: the result of the evaluation is no longer valid after this is done,
 * unless it is a pass-by-value datatype.
 *
 * NB: if you change this code, see also the hacks in exec_assign_value's
 * PLPGSQL_DTYPE_ARRAYELEM case.
 * ----------
 */
static void
exec_eval_cleanup(PLpgSQL_execstate *estate)
{
	/* Clear result of a full SPI_execute */
	if (estate->eval_tuptable != NULL)
		SPI_freetuptable(estate->eval_tuptable);
	estate->eval_tuptable = NULL;

	/* Clear result of exec_eval_simple_expr (but keep the econtext) */
	if (estate->eval_econtext != NULL)
		ResetExprContext(estate->eval_econtext);
}


/* ----------
 * Generate a prepared plan
 * ----------
 */
static void
exec_prepare_plan(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr, int cursorOptions)
{
	SPIPlanPtr	plan;

	/*
	 * The grammar can't conveniently set expr->func while building the parse
	 * tree, so make sure it's set before parser hooks need it.
	 */
	expr->func = estate->func;

	/*
	 * Generate and save the plan
	 */
	plan = SPI_prepare_params(expr->query,
							  (ParserSetupHook) plpgsql_parser_setup,
							  (void *) expr,
							  cursorOptions);
	if (plan == NULL)
	{
		/* Some SPI errors deserve specific error messages */
		switch (SPI_result)
		{
			case SPI_ERROR_COPY:
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("cannot COPY to/from client in PL/pgSQL")));
			case SPI_ERROR_TRANSACTION:
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("cannot begin/end transactions in PL/pgSQL"),
						 errhint("Use a BEGIN block with an EXCEPTION clause instead.")));
			default:
				elog(ERROR, "SPI_prepare_params failed for \"%s\": %s",
					 expr->query, SPI_result_code_string(SPI_result));
		}
	}
	expr->plan = SPI_saveplan(plan);
	SPI_freeplan(plan);
	exec_simple_check_plan(expr);
}


/* ----------
 * exec_stmt_execsql			Execute an SQL statement (possibly with INTO).
 * ----------
 */
static int
exec_stmt_execsql(PLpgSQL_execstate *estate,
				  PLpgSQL_stmt_execsql *stmt)
{
	ParamListInfo paramLI;
	long		tcount;
	int			rc;
	PLpgSQL_expr *expr = stmt->sqlstmt;

	/*
	 * On the first call for this statement generate the plan, and detect
	 * whether the statement is INSERT/UPDATE/DELETE
	 */
	if (expr->plan == NULL)
	{
		ListCell   *l;

		exec_prepare_plan(estate, expr, 0);
		stmt->mod_stmt = false;
		foreach(l, expr->plan->plancache_list)
		{
			CachedPlanSource *plansource = (CachedPlanSource *) lfirst(l);
			ListCell   *l2;

			foreach(l2, plansource->plan->stmt_list)
			{
				PlannedStmt *p = (PlannedStmt *) lfirst(l2);

				if (IsA(p, PlannedStmt) &&
					p->canSetTag)
				{
					if (p->commandType == CMD_INSERT ||
						p->commandType == CMD_UPDATE ||
						p->commandType == CMD_DELETE)
						stmt->mod_stmt = true;
				}
			}
		}
	}

	/*
	 * Set up ParamListInfo (note this is only carrying a hook function, not
	 * any actual data values, at this point)
	 */
	paramLI = setup_param_list(estate, expr);

	/*
	 * If we have INTO, then we only need one row back ... but if we have INTO
	 * STRICT, ask for two rows, so that we can verify the statement returns
	 * only one.  INSERT/UPDATE/DELETE are always treated strictly. Without
	 * INTO, just run the statement to completion (tcount = 0).
	 *
	 * We could just ask for two rows always when using INTO, but there are
	 * some cases where demanding the extra row costs significant time, eg by
	 * forcing completion of a sequential scan.  So don't do it unless we need
	 * to enforce strictness.
	 */
	if (stmt->into)
	{
		if (stmt->strict || stmt->mod_stmt)
			tcount = 2;
		else
			tcount = 1;
	}
	else
		tcount = 0;

	/*
	 * Execute the plan
	 */
	rc = SPI_execute_plan_with_paramlist(expr->plan, paramLI,
										 estate->readonly_func, tcount);

	/*
	 * Check for error, and set FOUND if appropriate (for historical reasons
	 * we set FOUND only for certain query types).	Also Assert that we
	 * identified the statement type the same as SPI did.
	 */
	switch (rc)
	{
		case SPI_OK_SELECT:
			Assert(!stmt->mod_stmt);
			exec_set_found(estate, (SPI_processed != 0));
			break;

		case SPI_OK_INSERT:
		case SPI_OK_UPDATE:
		case SPI_OK_DELETE:
		case SPI_OK_INSERT_RETURNING:
		case SPI_OK_UPDATE_RETURNING:
		case SPI_OK_DELETE_RETURNING:
			Assert(stmt->mod_stmt);
			exec_set_found(estate, (SPI_processed != 0));
			break;

		case SPI_OK_SELINTO:
		case SPI_OK_UTILITY:
			Assert(!stmt->mod_stmt);
			break;

		case SPI_OK_REWRITTEN:
			Assert(!stmt->mod_stmt);

			/*
			 * The command was rewritten into another kind of command. It's
			 * not clear what FOUND would mean in that case (and SPI doesn't
			 * return the row count either), so just set it to false.
			 */
			exec_set_found(estate, false);
			break;

			/* Some SPI errors deserve specific error messages */
		case SPI_ERROR_COPY:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot COPY to/from client in PL/pgSQL")));
		case SPI_ERROR_TRANSACTION:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot begin/end transactions in PL/pgSQL"),
			errhint("Use a BEGIN block with an EXCEPTION clause instead.")));

		default:
			elog(ERROR, "SPI_execute_plan_with_paramlist failed executing query \"%s\": %s",
				 expr->query, SPI_result_code_string(rc));
	}

	/* All variants should save result info for GET DIAGNOSTICS */
	estate->eval_processed = SPI_processed;
	estate->eval_lastoid = SPI_lastoid;

	/* Process INTO if present */
	if (stmt->into)
	{
		SPITupleTable *tuptab = SPI_tuptable;
		uint32		n = SPI_processed;
		PLpgSQL_rec *rec = NULL;
		PLpgSQL_row *row = NULL;

		/* If the statement did not return a tuple table, complain */
		if (tuptab == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("INTO used with a command that cannot return data")));

		/* Determine if we assign to a record or a row */
		if (stmt->rec != NULL)
			rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->dno]);
		else if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->dno]);
		else
			elog(ERROR, "unsupported target");

		/*
		 * If SELECT ... INTO specified STRICT, and the query didn't find
		 * exactly one row, throw an error.  If STRICT was not specified, then
		 * allow the query to find any number of rows.
		 */
		if (n == 0)
		{
			if (stmt->strict)
				ereport(ERROR,
						(errcode(ERRCODE_NO_DATA_FOUND),
						 errmsg("query returned no rows")));
			/* set the target to NULL(s) */
			exec_move_row(estate, rec, row, NULL, tuptab->tupdesc);
		}
		else
		{
			if (n > 1 && (stmt->strict || stmt->mod_stmt))
				ereport(ERROR,
						(errcode(ERRCODE_TOO_MANY_ROWS),
						 errmsg("query returned more than one row")));
			/* Put the first result row into the target */
			exec_move_row(estate, rec, row, tuptab->vals[0], tuptab->tupdesc);
		}

		/* Clean up */
		exec_eval_cleanup(estate);
		SPI_freetuptable(SPI_tuptable);
	}
	else
	{
		/* If the statement returned a tuple table, complain */
		if (SPI_tuptable != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("query has no destination for result data"),
					 (rc == SPI_OK_SELECT) ? errhint("If you want to discard the results of a SELECT, use PERFORM instead.") : 0));
	}

	if (paramLI)
		pfree(paramLI);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_dynexecute			Execute a dynamic SQL query
 *					(possibly with INTO).
 * ----------
 */
static int
exec_stmt_dynexecute(PLpgSQL_execstate *estate,
					 PLpgSQL_stmt_dynexecute *stmt)
{
	Datum		query;
	bool		isnull = false;
	Oid			restype;
	char	   *querystr;
	int			exec_res;

	/*
	 * First we evaluate the string expression after the EXECUTE keyword. Its
	 * result is the querystring we have to execute.
	 */
	query = exec_eval_expr(estate, stmt->query, &isnull, &restype);
	if (isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("query string argument of EXECUTE is null")));

	/* Get the C-String representation */
	querystr = convert_value_to_string(estate, query, restype);

	/* copy it out of the temporary context before we clean up */
	querystr = pstrdup(querystr);

	exec_eval_cleanup(estate);

	/*
	 * Execute the query without preparing a saved plan.
	 */
	if (stmt->params)
	{
		PreparedParamsData *ppd;

		ppd = exec_eval_using_params(estate, stmt->params);
		exec_res = SPI_execute_with_args(querystr,
										 ppd->nargs, ppd->types,
										 ppd->values, ppd->nulls,
										 estate->readonly_func, 0);
		free_params_data(ppd);
	}
	else
		exec_res = SPI_execute(querystr, estate->readonly_func, 0);

	switch (exec_res)
	{
		case SPI_OK_SELECT:
		case SPI_OK_INSERT:
		case SPI_OK_UPDATE:
		case SPI_OK_DELETE:
		case SPI_OK_INSERT_RETURNING:
		case SPI_OK_UPDATE_RETURNING:
		case SPI_OK_DELETE_RETURNING:
		case SPI_OK_UTILITY:
		case SPI_OK_REWRITTEN:
			break;

		case 0:

			/*
			 * Also allow a zero return, which implies the querystring
			 * contained no commands.
			 */
			break;

		case SPI_OK_SELINTO:

			/*
			 * We want to disallow SELECT INTO for now, because its behavior
			 * is not consistent with SELECT INTO in a normal plpgsql context.
			 * (We need to reimplement EXECUTE to parse the string as a
			 * plpgsql command, not just feed it to SPI_execute.) However,
			 * CREATE AS should be allowed ... and since it produces the same
			 * parsetree as SELECT INTO, there's no way to tell the difference
			 * except to look at the source text.  Wotta kluge!
			 */
			{
				char	   *ptr;

				for (ptr = querystr; *ptr; ptr++)
					if (!scanner_isspace(*ptr))
						break;
				if (*ptr == 'S' || *ptr == 's')
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("EXECUTE of SELECT ... INTO is not implemented"),
							 errhint("You might want to use EXECUTE ... INTO or EXECUTE CREATE TABLE ... AS instead.")));
				break;
			}

			/* Some SPI errors deserve specific error messages */
		case SPI_ERROR_COPY:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot COPY to/from client in PL/pgSQL")));
		case SPI_ERROR_TRANSACTION:
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("cannot begin/end transactions in PL/pgSQL"),
			errhint("Use a BEGIN block with an EXCEPTION clause instead.")));

		default:
			elog(ERROR, "SPI_execute failed executing query \"%s\": %s",
				 querystr, SPI_result_code_string(exec_res));
			break;
	}

	/* Save result info for GET DIAGNOSTICS */
	estate->eval_processed = SPI_processed;
	estate->eval_lastoid = SPI_lastoid;

	/* Process INTO if present */
	if (stmt->into)
	{
		SPITupleTable *tuptab = SPI_tuptable;
		uint32		n = SPI_processed;
		PLpgSQL_rec *rec = NULL;
		PLpgSQL_row *row = NULL;

		/* If the statement did not return a tuple table, complain */
		if (tuptab == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
				errmsg("INTO used with a command that cannot return data")));

		/* Determine if we assign to a record or a row */
		if (stmt->rec != NULL)
			rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->dno]);
		else if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->dno]);
		else
			elog(ERROR, "unsupported target");

		/*
		 * If SELECT ... INTO specified STRICT, and the query didn't find
		 * exactly one row, throw an error.  If STRICT was not specified, then
		 * allow the query to find any number of rows.
		 */
		if (n == 0)
		{
			if (stmt->strict)
				ereport(ERROR,
						(errcode(ERRCODE_NO_DATA_FOUND),
						 errmsg("query returned no rows")));
			/* set the target to NULL(s) */
			exec_move_row(estate, rec, row, NULL, tuptab->tupdesc);
		}
		else
		{
			if (n > 1 && stmt->strict)
				ereport(ERROR,
						(errcode(ERRCODE_TOO_MANY_ROWS),
						 errmsg("query returned more than one row")));
			/* Put the first result row into the target */
			exec_move_row(estate, rec, row, tuptab->vals[0], tuptab->tupdesc);
		}
		/* clean up after exec_move_row() */
		exec_eval_cleanup(estate);
	}
	else
	{
		/*
		 * It might be a good idea to raise an error if the query returned
		 * tuples that are being ignored, but historically we have not done
		 * that.
		 */
	}

	/* Release any result from SPI_execute, as well as the querystring */
	SPI_freetuptable(SPI_tuptable);
	pfree(querystr);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_dynfors			Execute a dynamic query, assign each
 *					tuple to a record or row and
 *					execute a group of statements
 *					for it.
 * ----------
 */
static int
exec_stmt_dynfors(PLpgSQL_execstate *estate, PLpgSQL_stmt_dynfors *stmt)
{
	Portal		portal;
	int			rc;

	portal = exec_dynquery_with_params(estate, stmt->query, stmt->params,
									   NULL, 0);

	/*
	 * Execute the loop
	 */
	rc = exec_for_query(estate, (PLpgSQL_stmt_forq *) stmt, portal, true);

	/*
	 * Close the implicit cursor
	 */
	SPI_cursor_close(portal);

	return rc;
}


/* ----------
 * exec_stmt_open			Execute an OPEN cursor statement
 * ----------
 */
static int
exec_stmt_open(PLpgSQL_execstate *estate, PLpgSQL_stmt_open *stmt)
{
	PLpgSQL_var *curvar;
	char	   *curname = NULL;
	PLpgSQL_expr *query;
	Portal		portal;
	ParamListInfo paramLI;

	/* ----------
	 * Get the cursor variable and if it has an assigned name, check
	 * that it's not in use currently.
	 * ----------
	 */
	curvar = (PLpgSQL_var *) (estate->datums[stmt->curvar]);
	if (!curvar->isnull)
	{
		curname = TextDatumGetCString(curvar->value);
		if (SPI_cursor_find(curname) != NULL)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_CURSOR),
					 errmsg("cursor \"%s\" already in use", curname)));
	}

	/* ----------
	 * Process the OPEN according to it's type.
	 * ----------
	 */
	if (stmt->query != NULL)
	{
		/* ----------
		 * This is an OPEN refcursor FOR SELECT ...
		 *
		 * We just make sure the query is planned. The real work is
		 * done downstairs.
		 * ----------
		 */
		query = stmt->query;
		if (query->plan == NULL)
			exec_prepare_plan(estate, query, stmt->cursor_options);
	}
	else if (stmt->dynquery != NULL)
	{
		/* ----------
		 * This is an OPEN refcursor FOR EXECUTE ...
		 * ----------
		 */
		portal = exec_dynquery_with_params(estate,
										   stmt->dynquery,
										   stmt->params,
										   curname,
										   stmt->cursor_options);

		/*
		 * If cursor variable was NULL, store the generated portal name in it
		 */
		if (curname == NULL)
			assign_text_var(curvar, portal->name);

		return PLPGSQL_RC_OK;
	}
	else
	{
		/* ----------
		 * This is an OPEN cursor
		 *
		 * Note: parser should already have checked that statement supplies
		 * args iff cursor needs them, but we check again to be safe.
		 * ----------
		 */
		if (stmt->argquery != NULL)
		{
			/* ----------
			 * OPEN CURSOR with args.  We fake a SELECT ... INTO ...
			 * statement to evaluate the args and put 'em into the
			 * internal row.
			 * ----------
			 */
			PLpgSQL_stmt_execsql set_args;

			if (curvar->cursor_explicit_argrow < 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
					errmsg("arguments given for cursor without arguments")));

			memset(&set_args, 0, sizeof(set_args));
			set_args.cmd_type = PLPGSQL_STMT_EXECSQL;
			set_args.lineno = stmt->lineno;
			set_args.sqlstmt = stmt->argquery;
			set_args.into = true;
			/* XXX historically this has not been STRICT */
			set_args.row = (PLpgSQL_row *)
				(estate->datums[curvar->cursor_explicit_argrow]);

			if (exec_stmt_execsql(estate, &set_args) != PLPGSQL_RC_OK)
				elog(ERROR, "open cursor failed during argument processing");
		}
		else
		{
			if (curvar->cursor_explicit_argrow >= 0)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("arguments required for cursor")));
		}

		query = curvar->cursor_explicit_expr;
		if (query->plan == NULL)
			exec_prepare_plan(estate, query, curvar->cursor_options);
	}

	/*
	 * Set up ParamListInfo (note this is only carrying a hook function, not
	 * any actual data values, at this point)
	 */
	paramLI = setup_param_list(estate, query);

	/*
	 * Open the cursor
	 */
	portal = SPI_cursor_open_with_paramlist(curname, query->plan,
											paramLI,
											estate->readonly_func);
	if (portal == NULL)
		elog(ERROR, "could not open cursor: %s",
			 SPI_result_code_string(SPI_result));

	/*
	 * If cursor variable was NULL, store the generated portal name in it
	 */
	if (curname == NULL)
		assign_text_var(curvar, portal->name);

	if (curname)
		pfree(curname);
	if (paramLI)
		pfree(paramLI);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_stmt_fetch			Fetch from a cursor into a target, or just
 *							move the current position of the cursor
 * ----------
 */
static int
exec_stmt_fetch(PLpgSQL_execstate *estate, PLpgSQL_stmt_fetch *stmt)
{
	PLpgSQL_var *curvar = NULL;
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_row *row = NULL;
	long		how_many = stmt->how_many;
	SPITupleTable *tuptab;
	Portal		portal;
	char	   *curname;
	uint32		n;

	/* ----------
	 * Get the portal of the cursor by name
	 * ----------
	 */
	curvar = (PLpgSQL_var *) (estate->datums[stmt->curvar]);
	if (curvar->isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("cursor variable \"%s\" is null", curvar->refname)));
	curname = TextDatumGetCString(curvar->value);

	portal = SPI_cursor_find(curname);
	if (portal == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_CURSOR),
				 errmsg("cursor \"%s\" does not exist", curname)));
	pfree(curname);

	/* Calculate position for FETCH_RELATIVE or FETCH_ABSOLUTE */
	if (stmt->expr)
	{
		bool		isnull;

		/* XXX should be doing this in LONG not INT width */
		how_many = exec_eval_integer(estate, stmt->expr, &isnull);

		if (isnull)
			ereport(ERROR,
					(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
					 errmsg("relative or absolute cursor position is null")));

		exec_eval_cleanup(estate);
	}

	if (!stmt->is_move)
	{
		/* ----------
		 * Determine if we fetch into a record or a row
		 * ----------
		 */
		if (stmt->rec != NULL)
			rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->dno]);
		else if (stmt->row != NULL)
			row = (PLpgSQL_row *) (estate->datums[stmt->row->dno]);
		else
			elog(ERROR, "unsupported target");

		/* ----------
		 * Fetch 1 tuple from the cursor
		 * ----------
		 */
		SPI_scroll_cursor_fetch(portal, stmt->direction, how_many);
		tuptab = SPI_tuptable;
		n = SPI_processed;

		/* ----------
		 * Set the target appropriately.
		 * ----------
		 */
		if (n == 0)
			exec_move_row(estate, rec, row, NULL, tuptab->tupdesc);
		else
			exec_move_row(estate, rec, row, tuptab->vals[0], tuptab->tupdesc);

		exec_eval_cleanup(estate);
		SPI_freetuptable(tuptab);
	}
	else
	{
		/* Move the cursor */
		SPI_scroll_cursor_move(portal, stmt->direction, how_many);
		n = SPI_processed;
	}

	/* Set the ROW_COUNT and the global FOUND variable appropriately. */
	estate->eval_processed = n;
	exec_set_found(estate, n != 0);

	return PLPGSQL_RC_OK;
}

/* ----------
 * exec_stmt_close			Close a cursor
 * ----------
 */
static int
exec_stmt_close(PLpgSQL_execstate *estate, PLpgSQL_stmt_close *stmt)
{
	PLpgSQL_var *curvar = NULL;
	Portal		portal;
	char	   *curname;

	/* ----------
	 * Get the portal of the cursor by name
	 * ----------
	 */
	curvar = (PLpgSQL_var *) (estate->datums[stmt->curvar]);
	if (curvar->isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("cursor variable \"%s\" is null", curvar->refname)));
	curname = TextDatumGetCString(curvar->value);

	portal = SPI_cursor_find(curname);
	if (portal == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_CURSOR),
				 errmsg("cursor \"%s\" does not exist", curname)));
	pfree(curname);

	/* ----------
	 * And close it.
	 * ----------
	 */
	SPI_cursor_close(portal);

	return PLPGSQL_RC_OK;
}


/* ----------
 * exec_assign_expr			Put an expression's result into
 *					a variable.
 * ----------
 */
static void
exec_assign_expr(PLpgSQL_execstate *estate, PLpgSQL_datum *target,
				 PLpgSQL_expr *expr)
{
	Datum		value;
	Oid			valtype;
	bool		isnull = false;

	value = exec_eval_expr(estate, expr, &isnull, &valtype);
	exec_assign_value(estate, target, value, valtype, &isnull);
	exec_eval_cleanup(estate);
}


/* ----------
 * exec_assign_value			Put a value into a target field
 *
 * Note: in some code paths, this will leak memory in the eval_econtext;
 * we assume that will be cleaned up later by exec_eval_cleanup.  We cannot
 * call exec_eval_cleanup here for fear of destroying the input Datum value.
 * ----------
 */
static void
exec_assign_value(PLpgSQL_execstate *estate,
				  PLpgSQL_datum *target,
				  Datum value, Oid valtype, bool *isNull)
{
	switch (target->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			{
				/*
				 * Target is a variable
				 */
				PLpgSQL_var *var = (PLpgSQL_var *) target;
				Datum		newvalue;

				newvalue = exec_cast_value(estate,
										   value,
										   valtype,
										   var->datatype->typoid,
										   &(var->datatype->typinput),
										   var->datatype->typioparam,
										   var->datatype->atttypmod,
										   *isNull);

				if (*isNull && var->notnull)
					ereport(ERROR,
							(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
							 errmsg("null value cannot be assigned to variable \"%s\" declared NOT NULL",
									var->refname)));

				/*
				 * If type is by-reference, copy the new value (which is
				 * probably in the eval_econtext) into the procedure's
				 * memory context.
				 */
				if (!var->datatype->typbyval && !*isNull)
					newvalue = datumCopy(newvalue,
										 false,
										 var->datatype->typlen);

				/*
				 * Now free the old value.	(We can't do this any earlier
				 * because of the possibility that we are assigning the var's
				 * old value to it, eg "foo := foo".  We could optimize out
				 * the assignment altogether in such cases, but it's too
				 * infrequent to be worth testing for.)
				 */
				free_var(var);

				var->value = newvalue;
				var->isnull = *isNull;
				if (!var->datatype->typbyval && !*isNull)
					var->freeval = true;
				break;
			}

		case PLPGSQL_DTYPE_ROW:
			{
				/*
				 * Target is a row variable
				 */
				PLpgSQL_row *row = (PLpgSQL_row *) target;

				if (*isNull)
				{
					/* If source is null, just assign nulls to the row */
					exec_move_row(estate, NULL, row, NULL, NULL);
				}
				else
				{
					HeapTupleHeader td;
					Oid			tupType;
					int32		tupTypmod;
					TupleDesc	tupdesc;
					HeapTupleData tmptup;

					/* Source must be of RECORD or composite type */
					if (!type_is_rowtype(valtype))
						ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
								 errmsg("cannot assign non-composite value to a row variable")));
					/* Source is a tuple Datum, so safe to do this: */
					td = DatumGetHeapTupleHeader(value);
					/* Extract rowtype info and find a tupdesc */
					tupType = HeapTupleHeaderGetTypeId(td);
					tupTypmod = HeapTupleHeaderGetTypMod(td);
					tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
					/* Build a temporary HeapTuple control structure */
					tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
					ItemPointerSetInvalid(&(tmptup.t_self));
					tmptup.t_tableOid = InvalidOid;
					tmptup.t_data = td;
					exec_move_row(estate, NULL, row, &tmptup, tupdesc);
					ReleaseTupleDesc(tupdesc);
				}
				break;
			}

		case PLPGSQL_DTYPE_REC:
			{
				/*
				 * Target is a record variable
				 */
				PLpgSQL_rec *rec = (PLpgSQL_rec *) target;

				if (*isNull)
				{
					/* If source is null, just assign nulls to the record */
					exec_move_row(estate, rec, NULL, NULL, NULL);
				}
				else
				{
					HeapTupleHeader td;
					Oid			tupType;
					int32		tupTypmod;
					TupleDesc	tupdesc;
					HeapTupleData tmptup;

					/* Source must be of RECORD or composite type */
					if (!type_is_rowtype(valtype))
						ereport(ERROR,
								(errcode(ERRCODE_DATATYPE_MISMATCH),
								 errmsg("cannot assign non-composite value to a record variable")));

					/* Source is a tuple Datum, so safe to do this: */
					td = DatumGetHeapTupleHeader(value);
					/* Extract rowtype info and find a tupdesc */
					tupType = HeapTupleHeaderGetTypeId(td);
					tupTypmod = HeapTupleHeaderGetTypMod(td);
					tupdesc = lookup_rowtype_tupdesc(tupType, tupTypmod);
					/* Build a temporary HeapTuple control structure */
					tmptup.t_len = HeapTupleHeaderGetDatumLength(td);
					ItemPointerSetInvalid(&(tmptup.t_self));
					tmptup.t_tableOid = InvalidOid;
					tmptup.t_data = td;
					exec_move_row(estate, rec, NULL, &tmptup, tupdesc);
					ReleaseTupleDesc(tupdesc);
				}
				break;
			}

		case PLPGSQL_DTYPE_RECFIELD:
			{
				/*
				 * Target is a field of a record
				 */
				PLpgSQL_recfield *recfield = (PLpgSQL_recfield *) target;
				PLpgSQL_rec *rec;
				int			fno;
				HeapTuple	newtup;
				int			natts;
				Datum	   *values;
				bool	   *nulls;
				bool	   *replaces;
				bool		attisnull;
				Oid			atttype;
				int32		atttypmod;

				rec = (PLpgSQL_rec *) (estate->datums[recfield->recparentno]);

				/*
				 * Check that there is already a tuple in the record. We need
				 * that because records don't have any predefined field
				 * structure.
				 */
				if (!HeapTupleIsValid(rec->tup))
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));

				/*
				 * Get the number of the records field to change and the
				 * number of attributes in the tuple.  Note: disallow system
				 * column names because the code below won't cope.
				 */
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno <= 0)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									rec->refname, recfield->fieldname)));
				fno--;
				natts = rec->tupdesc->natts;

				/*
				 * Set up values/control arrays for heap_modify_tuple. For all
				 * the attributes except the one we want to replace, use the
				 * value that's in the old tuple.
				 */
				values = palloc(sizeof(Datum) * natts);
				nulls = palloc(sizeof(bool) * natts);
				replaces = palloc(sizeof(bool) * natts);

				memset(replaces, false, sizeof(bool) * natts);
				replaces[fno] = true;

				/*
				 * Now insert the new value, being careful to cast it to the
				 * right type.
				 */
				atttype = SPI_gettypeid(rec->tupdesc, fno + 1);
				atttypmod = rec->tupdesc->attrs[fno]->atttypmod;
				attisnull = *isNull;
				values[fno] = exec_simple_cast_value(estate,
													 value,
													 valtype,
													 atttype,
													 atttypmod,
													 attisnull);
				nulls[fno] = attisnull;

				/*
				 * Now call heap_modify_tuple() to create a new tuple that
				 * replaces the old one in the record.
				 */
				newtup = heap_modify_tuple(rec->tup, rec->tupdesc,
										   values, nulls, replaces);

				if (rec->freetup)
					heap_freetuple(rec->tup);

				rec->tup = newtup;
				rec->freetup = true;

				pfree(values);
				pfree(nulls);
				pfree(replaces);

				break;
			}

		case PLPGSQL_DTYPE_ARRAYELEM:
			{
				/*
				 * Target is an element of an array
				 */
				int			nsubscripts;
				int			i;
				PLpgSQL_expr *subscripts[MAXDIM];
				int			subscriptvals[MAXDIM];
				bool		oldarrayisnull;
				Oid			arraytypeid,
							arrayelemtypeid;
				int32		arraytypmod;
				int16		arraytyplen,
							elemtyplen;
				bool		elemtypbyval;
				char		elemtypalign;
				Datum		oldarraydatum,
							coerced_value;
				ArrayType  *oldarrayval;
				ArrayType  *newarrayval;
				SPITupleTable *save_eval_tuptable;
				MemoryContext oldcontext;

				/*
				 * We need to do subscript evaluation, which might require
				 * evaluating general expressions; and the caller might have
				 * done that too in order to prepare the input Datum.  We have
				 * to save and restore the caller's SPI_execute result, if
				 * any.
				 */
				save_eval_tuptable = estate->eval_tuptable;
				estate->eval_tuptable = NULL;

				/*
				 * To handle constructs like x[1][2] := something, we have to
				 * be prepared to deal with a chain of arrayelem datums. Chase
				 * back to find the base array datum, and save the subscript
				 * expressions as we go.  (We are scanning right to left here,
				 * but want to evaluate the subscripts left-to-right to
				 * minimize surprises.)
				 */
				nsubscripts = 0;
				do
				{
					PLpgSQL_arrayelem *arrayelem = (PLpgSQL_arrayelem *) target;

					if (nsubscripts >= MAXDIM)
						ereport(ERROR,
								(errcode(ERRCODE_PROGRAM_LIMIT_EXCEEDED),
								 errmsg("number of array dimensions (%d) exceeds the maximum allowed (%d)",
										nsubscripts + 1, MAXDIM)));
					subscripts[nsubscripts++] = arrayelem->subscript;
					target = estate->datums[arrayelem->arrayparentno];
				} while (target->dtype == PLPGSQL_DTYPE_ARRAYELEM);

				/* Fetch current value of array datum */
				exec_eval_datum(estate, target,
								&arraytypeid, &arraytypmod,
								&oldarraydatum, &oldarrayisnull);

				/* If target is domain over array, reduce to base type */
				arraytypeid = getBaseTypeAndTypmod(arraytypeid, &arraytypmod);

				/* ... and identify the element type */
				arrayelemtypeid = get_element_type(arraytypeid);
				if (!OidIsValid(arrayelemtypeid))
					ereport(ERROR,
							(errcode(ERRCODE_DATATYPE_MISMATCH),
							 errmsg("subscripted object is not an array")));

				get_typlenbyvalalign(arrayelemtypeid,
									 &elemtyplen,
									 &elemtypbyval,
									 &elemtypalign);
				arraytyplen = get_typlen(arraytypeid);

				/*
				 * Evaluate the subscripts, switch into left-to-right order.
				 * Like ExecEvalArrayRef(), complain if any subscript is null.
				 */
				for (i = 0; i < nsubscripts; i++)
				{
					bool		subisnull;

					subscriptvals[i] =
						exec_eval_integer(estate,
										  subscripts[nsubscripts - 1 - i],
										  &subisnull);
					if (subisnull)
						ereport(ERROR,
								(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
								 errmsg("array subscript in assignment must not be null")));

					/*
					 * Clean up in case the subscript expression wasn't
					 * simple. We can't do exec_eval_cleanup, but we can do
					 * this much (which is safe because the integer subscript
					 * value is surely pass-by-value), and we must do it in
					 * case the next subscript expression isn't simple either.
					 */
					if (estate->eval_tuptable != NULL)
						SPI_freetuptable(estate->eval_tuptable);
					estate->eval_tuptable = NULL;
				}

				/* Now we can restore caller's SPI_execute result if any. */
				Assert(estate->eval_tuptable == NULL);
				estate->eval_tuptable = save_eval_tuptable;

				/* Coerce source value to match array element type. */
				coerced_value = exec_simple_cast_value(estate,
													   value,
													   valtype,
													   arrayelemtypeid,
													   arraytypmod,
													   *isNull);

				/*
				 * If the original array is null, cons up an empty array so
				 * that the assignment can proceed; we'll end with a
				 * one-element array containing just the assigned-to
				 * subscript.  This only works for varlena arrays, though; for
				 * fixed-length array types we skip the assignment.  We can't
				 * support assignment of a null entry into a fixed-length
				 * array, either, so that's a no-op too.  This is all ugly but
				 * corresponds to the current behavior of ExecEvalArrayRef().
				 */
				if (arraytyplen > 0 &&	/* fixed-length array? */
					(oldarrayisnull || *isNull))
					return;

				/* oldarrayval and newarrayval should be short-lived */
				oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);

				if (oldarrayisnull)
					oldarrayval = construct_empty_array(arrayelemtypeid);
				else
					oldarrayval = (ArrayType *) DatumGetPointer(oldarraydatum);

				/*
				 * Build the modified array value.
				 */
				newarrayval = array_set(oldarrayval,
										nsubscripts,
										subscriptvals,
										coerced_value,
										*isNull,
										arraytyplen,
										elemtyplen,
										elemtypbyval,
										elemtypalign);

				MemoryContextSwitchTo(oldcontext);

				/*
				 * Assign the new array to the base variable.  It's never NULL
				 * at this point.  Note that if the target is a domain,
				 * coercing the base array type back up to the domain will
				 * happen within exec_assign_value.
				 */
				*isNull = false;
				exec_assign_value(estate, target,
								  PointerGetDatum(newarrayval),
								  arraytypeid, isNull);
				break;
			}

		default:
			elog(ERROR, "unrecognized dtype: %d", target->dtype);
	}
}

/*
 * exec_eval_datum				Get current value of a PLpgSQL_datum
 *
 * The type oid, typmod, value in Datum format, and null flag are returned.
 *
 * At present this doesn't handle PLpgSQL_expr or PLpgSQL_arrayelem datums.
 *
 * NOTE: caller must not modify the returned value, since it points right
 * at the stored value in the case of pass-by-reference datatypes.	In some
 * cases we have to palloc a return value, and in such cases we put it into
 * the estate's short-term memory context.
 */
static void
exec_eval_datum(PLpgSQL_execstate *estate,
				PLpgSQL_datum *datum,
				Oid *typeid,
				int32 *typetypmod,
				Datum *value,
				bool *isnull)
{
	MemoryContext oldcontext;

	switch (datum->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			{
				PLpgSQL_var *var = (PLpgSQL_var *) datum;

				*typeid = var->datatype->typoid;
				*typetypmod = var->datatype->atttypmod;
				*value = var->value;
				*isnull = var->isnull;
				break;
			}

		case PLPGSQL_DTYPE_ROW:
			{
				PLpgSQL_row *row = (PLpgSQL_row *) datum;
				HeapTuple	tup;

				if (!row->rowtupdesc)	/* should not happen */
					elog(ERROR, "row variable has no tupdesc");
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(row->rowtupdesc);
				oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);
				tup = make_tuple_from_row(estate, row, row->rowtupdesc);
				if (tup == NULL)	/* should not happen */
					elog(ERROR, "row not compatible with its own tupdesc");
				MemoryContextSwitchTo(oldcontext);
				*typeid = row->rowtupdesc->tdtypeid;
				*typetypmod = row->rowtupdesc->tdtypmod;
				*value = HeapTupleGetDatum(tup);
				*isnull = false;
				break;
			}

		case PLPGSQL_DTYPE_REC:
			{
				PLpgSQL_rec *rec = (PLpgSQL_rec *) datum;
				HeapTupleData worktup;

				if (!HeapTupleIsValid(rec->tup))
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				Assert(rec->tupdesc != NULL);
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(rec->tupdesc);

				/*
				 * In a trigger, the NEW and OLD parameters are likely to be
				 * on-disk tuples that don't have the desired Datum fields.
				 * Copy the tuple body and insert the right values.
				 */
				oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);
				heap_copytuple_with_tuple(rec->tup, &worktup);
				HeapTupleHeaderSetDatumLength(worktup.t_data, worktup.t_len);
				HeapTupleHeaderSetTypeId(worktup.t_data, rec->tupdesc->tdtypeid);
				HeapTupleHeaderSetTypMod(worktup.t_data, rec->tupdesc->tdtypmod);
				MemoryContextSwitchTo(oldcontext);
				*typeid = rec->tupdesc->tdtypeid;
				*typetypmod = rec->tupdesc->tdtypmod;
				*value = HeapTupleGetDatum(&worktup);
				*isnull = false;
				break;
			}

		case PLPGSQL_DTYPE_RECFIELD:
			{
				PLpgSQL_recfield *recfield = (PLpgSQL_recfield *) datum;
				PLpgSQL_rec *rec;
				int			fno;

				rec = (PLpgSQL_rec *) (estate->datums[recfield->recparentno]);
				if (!HeapTupleIsValid(rec->tup))
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									rec->refname, recfield->fieldname)));
				*typeid = SPI_gettypeid(rec->tupdesc, fno);
				/* XXX there's no SPI_gettypmod, for some reason */
				if (fno > 0)
					*typetypmod = rec->tupdesc->attrs[fno - 1]->atttypmod;
				else
					*typetypmod = -1;
				*value = SPI_getbinval(rec->tup, rec->tupdesc, fno, isnull);
				break;
			}

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
	}
}

/*
 * exec_get_datum_type				Get datatype of a PLpgSQL_datum
 *
 * This is the same logic as in exec_eval_datum, except that it can handle
 * some cases where exec_eval_datum has to fail; specifically, we may have
 * a tupdesc but no row value for a record variable.  (This currently can
 * happen only for a trigger's NEW/OLD records.)
 */
Oid
exec_get_datum_type(PLpgSQL_execstate *estate,
					PLpgSQL_datum *datum)
{
	Oid			typeid;

	switch (datum->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			{
				PLpgSQL_var *var = (PLpgSQL_var *) datum;

				typeid = var->datatype->typoid;
				break;
			}

		case PLPGSQL_DTYPE_ROW:
			{
				PLpgSQL_row *row = (PLpgSQL_row *) datum;

				if (!row->rowtupdesc)	/* should not happen */
					elog(ERROR, "row variable has no tupdesc");
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(row->rowtupdesc);
				typeid = row->rowtupdesc->tdtypeid;
				break;
			}

		case PLPGSQL_DTYPE_REC:
			{
				PLpgSQL_rec *rec = (PLpgSQL_rec *) datum;

				if (rec->tupdesc == NULL)
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(rec->tupdesc);
				typeid = rec->tupdesc->tdtypeid;
				break;
			}

		case PLPGSQL_DTYPE_RECFIELD:
			{
				PLpgSQL_recfield *recfield = (PLpgSQL_recfield *) datum;
				PLpgSQL_rec *rec;
				int			fno;

				rec = (PLpgSQL_rec *) (estate->datums[recfield->recparentno]);
				if (rec->tupdesc == NULL)
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									rec->refname, recfield->fieldname)));
				typeid = SPI_gettypeid(rec->tupdesc, fno);
				break;
			}

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			typeid = InvalidOid;	/* keep compiler quiet */
			break;
	}

	return typeid;
}

/*
 * exec_get_datum_type_info			Get datatype etc of a PLpgSQL_datum
 *
 * An extended version of exec_get_datum_type, which also retrieves the
 * typmod and collation of the datum.
 */
void
exec_get_datum_type_info(PLpgSQL_execstate *estate,
						 PLpgSQL_datum *datum,
						 Oid *typeid, int32 *typmod, Oid *collation)
{
	switch (datum->dtype)
	{
		case PLPGSQL_DTYPE_VAR:
			{
				PLpgSQL_var *var = (PLpgSQL_var *) datum;

				*typeid = var->datatype->typoid;
				*typmod = var->datatype->atttypmod;
				*collation = var->datatype->collation;
				break;
			}

		case PLPGSQL_DTYPE_ROW:
			{
				PLpgSQL_row *row = (PLpgSQL_row *) datum;

				if (!row->rowtupdesc)	/* should not happen */
					elog(ERROR, "row variable has no tupdesc");
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(row->rowtupdesc);
				*typeid = row->rowtupdesc->tdtypeid;
				/* do NOT return the mutable typmod of a RECORD variable */
				*typmod = -1;
				/* composite types are never collatable */
				*collation = InvalidOid;
				break;
			}

		case PLPGSQL_DTYPE_REC:
			{
				PLpgSQL_rec *rec = (PLpgSQL_rec *) datum;

				if (rec->tupdesc == NULL)
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				/* Make sure we have a valid type/typmod setting */
				BlessTupleDesc(rec->tupdesc);
				*typeid = rec->tupdesc->tdtypeid;
				/* do NOT return the mutable typmod of a RECORD variable */
				*typmod = -1;
				/* composite types are never collatable */
				*collation = InvalidOid;
				break;
			}

		case PLPGSQL_DTYPE_RECFIELD:
			{
				PLpgSQL_recfield *recfield = (PLpgSQL_recfield *) datum;
				PLpgSQL_rec *rec;
				int			fno;

				rec = (PLpgSQL_rec *) (estate->datums[recfield->recparentno]);
				if (rec->tupdesc == NULL)
					ereport(ERROR,
						  (errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
						   errmsg("record \"%s\" is not assigned yet",
								  rec->refname),
						   errdetail("The tuple structure of a not-yet-assigned record is indeterminate.")));
				fno = SPI_fnumber(rec->tupdesc, recfield->fieldname);
				if (fno == SPI_ERROR_NOATTRIBUTE)
					ereport(ERROR,
							(errcode(ERRCODE_UNDEFINED_COLUMN),
							 errmsg("record \"%s\" has no field \"%s\"",
									rec->refname, recfield->fieldname)));
				*typeid = SPI_gettypeid(rec->tupdesc, fno);
				/* XXX there's no SPI_gettypmod, for some reason */
				if (fno > 0)
					*typmod = rec->tupdesc->attrs[fno - 1]->atttypmod;
				else
					*typmod = -1;
				/* XXX there's no SPI_getcollation either */
				if (fno > 0)
					*collation = rec->tupdesc->attrs[fno - 1]->attcollation;
				else	/* no system column types have collation */
					*collation = InvalidOid;
				break;
			}

		default:
			elog(ERROR, "unrecognized dtype: %d", datum->dtype);
			*typeid = InvalidOid;		/* keep compiler quiet */
			*typmod = -1;
			*collation = InvalidOid;
			break;
	}
}

/* ----------
 * exec_eval_integer		Evaluate an expression, coerce result to int4
 *
 * Note we do not do exec_eval_cleanup here; the caller must do it at
 * some later point.  (We do this because the caller may be holding the
 * results of other, pass-by-reference, expression evaluations, such as
 * an array value to be subscripted.  Also see notes in exec_eval_simple_expr
 * about allocation of the parameter array.)
 * ----------
 */
static int
exec_eval_integer(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr,
				  bool *isNull)
{
	Datum		exprdatum;
	Oid			exprtypeid;

	exprdatum = exec_eval_expr(estate, expr, isNull, &exprtypeid);
	exprdatum = exec_simple_cast_value(estate, exprdatum, exprtypeid,
									   INT4OID, -1,
									   *isNull);
	return DatumGetInt32(exprdatum);
}

/* ----------
 * exec_eval_boolean		Evaluate an expression, coerce result to bool
 *
 * Note we do not do exec_eval_cleanup here; the caller must do it at
 * some later point.
 * ----------
 */
static bool
exec_eval_boolean(PLpgSQL_execstate *estate,
				  PLpgSQL_expr *expr,
				  bool *isNull)
{
	Datum		exprdatum;
	Oid			exprtypeid;

	exprdatum = exec_eval_expr(estate, expr, isNull, &exprtypeid);
	exprdatum = exec_simple_cast_value(estate, exprdatum, exprtypeid,
									   BOOLOID, -1,
									   *isNull);
	return DatumGetBool(exprdatum);
}

/* ----------
 * exec_eval_expr			Evaluate an expression and return
 *					the result Datum.
 *
 * NOTE: caller must do exec_eval_cleanup when done with the Datum.
 * ----------
 */
static Datum
exec_eval_expr(PLpgSQL_execstate *estate,
			   PLpgSQL_expr *expr,
			   bool *isNull,
			   Oid *rettype)
{
	Datum		result = 0;
	int			rc;

	/*
	 * If first time through, create a plan for this expression.
	 */
	if (expr->plan == NULL)
		exec_prepare_plan(estate, expr, 0);

	/*
	 * If this is a simple expression, bypass SPI and use the executor
	 * directly
	 */
	if (exec_eval_simple_expr(estate, expr, &result, isNull, rettype))
		return result;

	/*
	 * Else do it the hard way via exec_run_select
	 */
	rc = exec_run_select(estate, expr, 2, NULL);
	if (rc != SPI_OK_SELECT)
		ereport(ERROR,
				(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				 errmsg("query \"%s\" did not return data", expr->query)));

	/*
	 * Check that the expression returns exactly one column...
	 */
	if (estate->eval_tuptable->tupdesc->natts != 1)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg_plural("query \"%s\" returned %d column",
							   "query \"%s\" returned %d columns",
							   estate->eval_tuptable->tupdesc->natts,
							   expr->query,
							   estate->eval_tuptable->tupdesc->natts)));

	/*
	 * ... and get the column's datatype.
	 */
	*rettype = SPI_gettypeid(estate->eval_tuptable->tupdesc, 1);

	/*
	 * If there are no rows selected, the result is a NULL of that type.
	 */
	if (estate->eval_processed == 0)
	{
		*isNull = true;
		return (Datum) 0;
	}

	/*
	 * Check that the expression returned no more than one row.
	 */
	if (estate->eval_processed != 1)
		ereport(ERROR,
				(errcode(ERRCODE_CARDINALITY_VIOLATION),
				 errmsg("query \"%s\" returned more than one row",
						expr->query)));

	/*
	 * Return the single result Datum.
	 */
	return SPI_getbinval(estate->eval_tuptable->vals[0],
						 estate->eval_tuptable->tupdesc, 1, isNull);
}


/* ----------
 * exec_run_select			Execute a select query
 * ----------
 */
static int
exec_run_select(PLpgSQL_execstate *estate,
				PLpgSQL_expr *expr, long maxtuples, Portal *portalP)
{
	ParamListInfo paramLI;
	int			rc;

	/*
	 * On the first call for this expression generate the plan
	 */
	if (expr->plan == NULL)
		exec_prepare_plan(estate, expr, 0);

	/*
	 * Set up ParamListInfo (note this is only carrying a hook function, not
	 * any actual data values, at this point)
	 */
	paramLI = setup_param_list(estate, expr);

	/*
	 * If a portal was requested, put the query into the portal
	 */
	if (portalP != NULL)
	{
		*portalP = SPI_cursor_open_with_paramlist(NULL, expr->plan,
												  paramLI,
												  estate->readonly_func);
		if (*portalP == NULL)
			elog(ERROR, "could not open implicit cursor for query \"%s\": %s",
				 expr->query, SPI_result_code_string(SPI_result));
		if (paramLI)
			pfree(paramLI);
		return SPI_OK_CURSOR;
	}

	/*
	 * Execute the query
	 */
	rc = SPI_execute_plan_with_paramlist(expr->plan, paramLI,
										 estate->readonly_func, maxtuples);
	if (rc != SPI_OK_SELECT)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("query \"%s\" is not a SELECT", expr->query)));

	/* Save query results for eventual cleanup */
	Assert(estate->eval_tuptable == NULL);
	estate->eval_tuptable = SPI_tuptable;
	estate->eval_processed = SPI_processed;
	estate->eval_lastoid = SPI_lastoid;

	if (paramLI)
		pfree(paramLI);

	return rc;
}


/*
 * exec_for_query --- execute body of FOR loop for each row from a portal
 *
 * Used by exec_stmt_fors, exec_stmt_forc and exec_stmt_dynfors
 */
static int
exec_for_query(PLpgSQL_execstate *estate, PLpgSQL_stmt_forq *stmt,
			   Portal portal, bool prefetch_ok)
{
	PLpgSQL_rec *rec = NULL;
	PLpgSQL_row *row = NULL;
	SPITupleTable *tuptab;
	bool		found = false;
	int			rc = PLPGSQL_RC_OK;
	int			n;

	/*
	 * Determine if we assign to a record or a row
	 */
	if (stmt->rec != NULL)
		rec = (PLpgSQL_rec *) (estate->datums[stmt->rec->dno]);
	else if (stmt->row != NULL)
		row = (PLpgSQL_row *) (estate->datums[stmt->row->dno]);
	else
		elog(ERROR, "unsupported target");

	/*
	 * Make sure the portal doesn't get closed by the user statements we
	 * execute.
	 */
	PinPortal(portal);

	/*
	 * Fetch the initial tuple(s).	If prefetching is allowed then we grab a
	 * few more rows to avoid multiple trips through executor startup
	 * overhead.
	 */
	SPI_cursor_fetch(portal, true, prefetch_ok ? 10 : 1);
	tuptab = SPI_tuptable;
	n = SPI_processed;

	/*
	 * If the query didn't return any rows, set the target to NULL and fall
	 * through with found = false.
	 */
	if (n <= 0)
	{
		exec_move_row(estate, rec, row, NULL, tuptab->tupdesc);
		exec_eval_cleanup(estate);
	}
	else
		found = true;			/* processed at least one tuple */

	/*
	 * Now do the loop
	 */
	while (n > 0)
	{
		int			i;

		for (i = 0; i < n; i++)
		{
			/*
			 * Assign the tuple to the target
			 */
			exec_move_row(estate, rec, row, tuptab->vals[i], tuptab->tupdesc);
			exec_eval_cleanup(estate);

			/*
			 * Execute the statements
			 */
			rc = exec_stmts(estate, stmt->body);

			if (rc != PLPGSQL_RC_OK)
			{
				if (rc == PLPGSQL_RC_EXIT)
				{
					if (estate->exitlabel == NULL)
					{
						/* unlabelled exit, so exit the current loop */
						rc = PLPGSQL_RC_OK;
					}
					else if (stmt->label != NULL &&
							 strcmp(stmt->label, estate->exitlabel) == 0)
					{
						/* label matches this loop, so exit loop */
						estate->exitlabel = NULL;
						rc = PLPGSQL_RC_OK;
					}

					/*
					 * otherwise, we processed a labelled exit that does not
					 * match the current statement's label, if any; return
					 * RC_EXIT so that the EXIT continues to recurse upward.
					 */
				}
				else if (rc == PLPGSQL_RC_CONTINUE)
				{
					if (estate->exitlabel == NULL)
					{
						/* unlabelled continue, so re-run the current loop */
						rc = PLPGSQL_RC_OK;
						continue;
					}
					else if (stmt->label != NULL &&
							 strcmp(stmt->label, estate->exitlabel) == 0)
					{
						/* label matches this loop, so re-run loop */
						estate->exitlabel = NULL;
						rc = PLPGSQL_RC_OK;
						continue;
					}

					/*
					 * otherwise, we process a labelled continue that does not
					 * match the current statement's label, if any; return
					 * RC_CONTINUE so that the CONTINUE will propagate up the
					 * stack.
					 */
				}

				/*
				 * We're aborting the loop.  Need a goto to get out of two
				 * levels of loop...
				 */
				goto loop_exit;
			}
		}

		SPI_freetuptable(tuptab);

		/*
		 * Fetch more tuples.  If prefetching is allowed, grab 50 at a time.
		 */
		SPI_cursor_fetch(portal, true, prefetch_ok ? 50 : 1);
		tuptab = SPI_tuptable;
		n = SPI_processed;
	}

loop_exit:

	/*
	 * Release last group of tuples (if any)
	 */
	SPI_freetuptable(tuptab);

	UnpinPortal(portal);

	/*
	 * Set the FOUND variable to indicate the result of executing the loop
	 * (namely, whether we looped one or more times). This must be set last so
	 * that it does not interfere with the value of the FOUND variable inside
	 * the loop processing itself.
	 */
	exec_set_found(estate, found);

	return rc;
}


/* ----------
 * exec_eval_simple_expr -		Evaluate a simple expression returning
 *								a Datum by directly calling ExecEvalExpr().
 *
 * If successful, store results into *result, *isNull, *rettype and return
 * TRUE.  If the expression cannot be handled by simple evaluation,
 * return FALSE.
 *
 * Because we only store one execution tree for a simple expression, we
 * can't handle recursion cases.  So, if we see the tree is already busy
 * with an evaluation in the current xact, we just return FALSE and let the
 * caller run the expression the hard way.	(Other alternatives such as
 * creating a new tree for a recursive call either introduce memory leaks,
 * or add enough bookkeeping to be doubtful wins anyway.)  Another case that
 * is covered by the expr_simple_in_use test is where a previous execution
 * of the tree was aborted by an error: the tree may contain bogus state
 * so we dare not re-use it.
 *
 * It is possible though unlikely for a simple expression to become non-simple
 * (consider for example redefining a trivial view).  We must handle that for
 * correctness; fortunately it's normally inexpensive to do
 * RevalidateCachedPlan on a simple expression.  We do not consider the other
 * direction (non-simple expression becoming simple) because we'll still give
 * correct results if that happens, and it's unlikely to be worth the cycles
 * to check.
 *
 * Note: if pass-by-reference, the result is in the eval_econtext's
 * temporary memory context.  It will be freed when exec_eval_cleanup
 * is done.
 * ----------
 */
static bool
exec_eval_simple_expr(PLpgSQL_execstate *estate,
					  PLpgSQL_expr *expr,
					  Datum *result,
					  bool *isNull,
					  Oid *rettype)
{
	ExprContext *econtext = estate->eval_econtext;
	LocalTransactionId curlxid = MyProc->lxid;
	CachedPlanSource *plansource;
	CachedPlan *cplan;
	ParamListInfo paramLI;
	PLpgSQL_expr *save_cur_expr;
	MemoryContext oldcontext;

	/*
	 * Forget it if expression wasn't simple before.
	 */
	if (expr->expr_simple_expr == NULL)
		return false;

	/*
	 * If expression is in use in current xact, don't touch it.
	 */
	if (expr->expr_simple_in_use && expr->expr_simple_lxid == curlxid)
		return false;

	/*
	 * Revalidate cached plan, so that we will notice if it became stale. (We
	 * also need to hold a refcount while using the plan.)	Note that even if
	 * replanning occurs, the length of plancache_list can't change, since it
	 * is a property of the raw parsetree generated from the query text.
	 */
	Assert(list_length(expr->plan->plancache_list) == 1);
	plansource = (CachedPlanSource *) linitial(expr->plan->plancache_list);
	cplan = RevalidateCachedPlan(plansource, true);
	if (cplan->generation != expr->expr_simple_generation)
	{
		/* It got replanned ... is it still simple? */
		exec_simple_check_plan(expr);
		if (expr->expr_simple_expr == NULL)
		{
			/* Ooops, release refcount and fail */
			ReleaseCachedPlan(cplan, true);
			return false;
		}
	}

	/*
	 * Pass back previously-determined result type.
	 */
	*rettype = expr->expr_simple_type;

	/*
	 * Prepare the expression for execution, if it's not been done already in
	 * the current transaction.  (This will be forced to happen if we called
	 * exec_simple_check_plan above.)
	 */
	if (expr->expr_simple_lxid != curlxid)
	{
		oldcontext = MemoryContextSwitchTo(simple_eval_estate->es_query_cxt);
		expr->expr_simple_state = ExecInitExpr(expr->expr_simple_expr, NULL);
		expr->expr_simple_in_use = false;
		expr->expr_simple_lxid = curlxid;
		MemoryContextSwitchTo(oldcontext);
	}

	/*
	 * We have to do some of the things SPI_execute_plan would do, in
	 * particular advance the snapshot if we are in a non-read-only function.
	 * Without this, stable functions within the expression would fail to see
	 * updates made so far by our own function.
	 */
	SPI_push();

	oldcontext = MemoryContextSwitchTo(econtext->ecxt_per_tuple_memory);
	if (!estate->readonly_func)
	{
		CommandCounterIncrement();
		PushActiveSnapshot(GetTransactionSnapshot());
	}

	/*
	 * Create the param list in econtext's temporary memory context. We won't
	 * need to free it explicitly, since it will go away at the next reset of
	 * that context.
	 *
	 * XXX think about avoiding repeated palloc's for param lists?  It should
	 * be possible --- this routine isn't re-entrant anymore.
	 *
	 * Just for paranoia's sake, save and restore the prior value of
	 * estate->cur_expr, which setup_param_list() sets.
	 */
	save_cur_expr = estate->cur_expr;

	paramLI = setup_param_list(estate, expr);
	econtext->ecxt_param_list_info = paramLI;

	/*
	 * Mark expression as busy for the duration of the ExecEvalExpr call.
	 */
	expr->expr_simple_in_use = true;

	/*
	 * Finally we can call the executor to evaluate the expression
	 */
	*result = ExecEvalExpr(expr->expr_simple_state,
						   econtext,
						   isNull,
						   NULL);

	/* Assorted cleanup */
	expr->expr_simple_in_use = false;

	estate->cur_expr = save_cur_expr;

	if (!estate->readonly_func)
		PopActiveSnapshot();

	MemoryContextSwitchTo(oldcontext);

	SPI_pop();

	/*
	 * Now we can release our refcount on the cached plan.
	 */
	ReleaseCachedPlan(cplan, true);

	/*
	 * That's it.
	 */
	return true;
}


/*
 * Create a ParamListInfo to pass to SPI
 *
 * The ParamListInfo array is initially all zeroes, in particular the
 * ptype values are all InvalidOid.  This causes the executor to call the
 * paramFetch hook each time it wants a value.	We thus evaluate only the
 * parameters actually demanded.
 *
 * The result is a locally palloc'd array that should be pfree'd after use;
 * but note it can be NULL.
 */
static ParamListInfo
setup_param_list(PLpgSQL_execstate *estate, PLpgSQL_expr *expr)
{
	ParamListInfo paramLI;

	/*
	 * Could we re-use these arrays instead of palloc'ing a new one each time?
	 * However, we'd have to zero the array each time anyway, since new values
	 * might have been assigned to the variables.
	 */
	if (estate->ndatums > 0)
	{
		/* sizeof(ParamListInfoData) includes the first array element */
		paramLI = (ParamListInfo)
			palloc0(sizeof(ParamListInfoData) +
					(estate->ndatums - 1) * sizeof(ParamExternData));
		paramLI->paramFetch = plpgsql_param_fetch;
		paramLI->paramFetchArg = (void *) estate;
		paramLI->parserSetup = (ParserSetupHook) plpgsql_parser_setup;
		paramLI->parserSetupArg = (void *) expr;
		paramLI->numParams = estate->ndatums;

		/*
		 * Set up link to active expr where the hook functions can find it.
		 * Callers must save and restore cur_expr if there is any chance that
		 * they are interrupting an active use of parameters.
		 */
		estate->cur_expr = expr;

		/*
		 * Also make sure this is set before parser hooks need it.	There is
		 * no need to save and restore, since the value is always correct once
		 * set.
		 */
		expr->func = estate->func;
	}
	else
		paramLI = NULL;
	return paramLI;
}

/*
 * plpgsql_param_fetch		paramFetch callback for dynamic parameter fetch
 */
static void
plpgsql_param_fetch(ParamListInfo params, int paramid)
{
	int			dno;
	PLpgSQL_execstate *estate;
	PLpgSQL_expr *expr;
	PLpgSQL_datum *datum;
	ParamExternData *prm;
	int32		prmtypmod;

	/* paramid's are 1-based, but dnos are 0-based */
	dno = paramid - 1;
	Assert(dno >= 0 && dno < params->numParams);

	/* fetch back the hook data */
	estate = (PLpgSQL_execstate *) params->paramFetchArg;
	expr = estate->cur_expr;
	Assert(params->numParams == estate->ndatums);

	/*
	 * Do nothing if asked for a value that's not supposed to be used by this
	 * SQL expression.	This avoids unwanted evaluations when functions such
	 * as copyParamList try to materialize all the values.
	 */
	if (!bms_is_member(dno, expr->paramnos))
		return;

	/* OK, evaluate the value and store into the appropriate paramlist slot */
	datum = estate->datums[dno];
	prm = &params->params[dno];
	exec_eval_datum(estate, datum,
					&prm->ptype, &prmtypmod,
					&prm->value, &prm->isnull);
}


/* ----------
 * exec_move_row			Move one tuple's values into a record or row
 *
 * Since this uses exec_assign_value, caller should eventually call
 * exec_eval_cleanup to prevent long-term memory leaks.
 * ----------
 */
static void
exec_move_row(PLpgSQL_execstate *estate,
			  PLpgSQL_rec *rec,
			  PLpgSQL_row *row,
			  HeapTuple tup, TupleDesc tupdesc)
{
	/*
	 * Record is simple - just copy the tuple and its descriptor into the
	 * record variable
	 */
	if (rec != NULL)
	{
		/*
		 * Copy input first, just in case it is pointing at variable's value
		 */
		if (HeapTupleIsValid(tup))
			tup = heap_copytuple(tup);
		else if (tupdesc)
		{
			/* If we have a tupdesc but no data, form an all-nulls tuple */
			bool	   *nulls;

			nulls = (bool *) palloc(tupdesc->natts * sizeof(bool));
			memset(nulls, true, tupdesc->natts * sizeof(bool));

			tup = heap_form_tuple(tupdesc, NULL, nulls);

			pfree(nulls);
		}

		if (tupdesc)
			tupdesc = CreateTupleDescCopy(tupdesc);

		/* Free the old value ... */
		if (rec->freetup)
		{
			heap_freetuple(rec->tup);
			rec->freetup = false;
		}
		if (rec->freetupdesc)
		{
			FreeTupleDesc(rec->tupdesc);
			rec->freetupdesc = false;
		}

		/* ... and install the new */
		if (HeapTupleIsValid(tup))
		{
			rec->tup = tup;
			rec->freetup = true;
		}
		else
			rec->tup = NULL;

		if (tupdesc)
		{
			rec->tupdesc = tupdesc;
			rec->freetupdesc = true;
		}
		else
			rec->tupdesc = NULL;

		return;
	}

	/*
	 * Row is a bit more complicated in that we assign the individual
	 * attributes of the tuple to the variables the row points to.
	 *
	 * NOTE: this code used to demand row->nfields ==
	 * HeapTupleHeaderGetNatts(tup->t_data), but that's wrong.  The tuple
	 * might have more fields than we expected if it's from an
	 * inheritance-child table of the current table, or it might have fewer if
	 * the table has had columns added by ALTER TABLE. Ignore extra columns
	 * and assume NULL for missing columns, the same as heap_getattr would do.
	 * We also have to skip over dropped columns in either the source or
	 * destination.
	 *
	 * If we have no tuple data at all, we'll assign NULL to all columns of
	 * the row variable.
	 */
	if (row != NULL)
	{
		int			td_natts = tupdesc ? tupdesc->natts : 0;
		int			t_natts;
		int			fnum;
		int			anum;

		if (HeapTupleIsValid(tup))
			t_natts = HeapTupleHeaderGetNatts(tup->t_data);
		else
			t_natts = 0;

		anum = 0;
		for (fnum = 0; fnum < row->nfields; fnum++)
		{
			PLpgSQL_var *var;
			Datum		value;
			bool		isnull;
			Oid			valtype;

			if (row->varnos[fnum] < 0)
				continue;		/* skip dropped column in row struct */

			var = (PLpgSQL_var *) (estate->datums[row->varnos[fnum]]);

			while (anum < td_natts && tupdesc->attrs[anum]->attisdropped)
				anum++;			/* skip dropped column in tuple */

			if (anum < td_natts)
			{
				if (anum < t_natts)
					value = SPI_getbinval(tup, tupdesc, anum + 1, &isnull);
				else
				{
					value = (Datum) 0;
					isnull = true;
				}
				valtype = SPI_gettypeid(tupdesc, anum + 1);
				anum++;
			}
			else
			{
				value = (Datum) 0;
				isnull = true;

				/*
				 * InvalidOid is OK because exec_assign_value doesn't care
				 * about the type of a source NULL
				 */
				valtype = InvalidOid;
			}

			exec_assign_value(estate, (PLpgSQL_datum *) var,
							  value, valtype, &isnull);
		}

		return;
	}

	elog(ERROR, "unsupported target");
}

/* ----------
 * make_tuple_from_row		Make a tuple from the values of a row object
 *
 * A NULL return indicates rowtype mismatch; caller must raise suitable error
 * ----------
 */
static HeapTuple
make_tuple_from_row(PLpgSQL_execstate *estate,
					PLpgSQL_row *row,
					TupleDesc tupdesc)
{
	int			natts = tupdesc->natts;
	HeapTuple	tuple;
	Datum	   *dvalues;
	bool	   *nulls;
	int			i;

	if (natts != row->nfields)
		return NULL;

	dvalues = (Datum *) palloc0(natts * sizeof(Datum));
	nulls = (bool *) palloc(natts * sizeof(bool));

	for (i = 0; i < natts; i++)
	{
		Oid			fieldtypeid;
		int32		fieldtypmod;

		if (tupdesc->attrs[i]->attisdropped)
		{
			nulls[i] = true;	/* leave the column as null */
			continue;
		}
		if (row->varnos[i] < 0) /* should not happen */
			elog(ERROR, "dropped rowtype entry for non-dropped column");

		exec_eval_datum(estate, estate->datums[row->varnos[i]],
						&fieldtypeid, &fieldtypmod,
						&dvalues[i], &nulls[i]);
		if (fieldtypeid != tupdesc->attrs[i]->atttypid)
			return NULL;
		/* XXX should we insist on typmod match, too? */
	}

	tuple = heap_form_tuple(tupdesc, dvalues, nulls);

	pfree(dvalues);
	pfree(nulls);

	return tuple;
}

/* ----------
 * convert_value_to_string			Convert a non-null Datum to C string
 *
 * Note: the result is in the estate's eval_econtext, and will be cleared
 * by the next exec_eval_cleanup() call.  The invoked output function might
 * leave additional cruft there as well, so just pfree'ing the result string
 * would not be enough to avoid memory leaks if we did not do it like this.
 * In most usages the Datum being passed in is also in that context (if
 * pass-by-reference) and so an exec_eval_cleanup() call is needed anyway.
 *
 * Note: not caching the conversion function lookup is bad for performance.
 * ----------
 */
static char *
convert_value_to_string(PLpgSQL_execstate *estate, Datum value, Oid valtype)
{
	char	   *result;
	MemoryContext oldcontext;
	Oid			typoutput;
	bool		typIsVarlena;

	oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);
	getTypeOutputInfo(valtype, &typoutput, &typIsVarlena);
	result = OidOutputFunctionCall(typoutput, value);
	MemoryContextSwitchTo(oldcontext);

	return result;
}

/* ----------
 * exec_cast_value			Cast a value if required
 *
 * Note: the estate's eval_econtext is used for temporary storage, and may
 * also contain the result Datum if we have to do a conversion to a pass-
 * by-reference data type.  Be sure to do an exec_eval_cleanup() call when
 * done with the result.
 * ----------
 */
static Datum
exec_cast_value(PLpgSQL_execstate *estate,
				Datum value, Oid valtype,
				Oid reqtype,
				FmgrInfo *reqinput,
				Oid reqtypioparam,
				int32 reqtypmod,
				bool isnull)
{
	/*
	 * If the type of the given value isn't what's requested, convert it.
	 */
	if (valtype != reqtype || reqtypmod != -1)
	{
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(estate->eval_econtext->ecxt_per_tuple_memory);
		if (!isnull)
		{
			char	   *extval;

			extval = convert_value_to_string(estate, value, valtype);
			value = InputFunctionCall(reqinput, extval,
									  reqtypioparam, reqtypmod);
		}
		else
		{
			value = InputFunctionCall(reqinput, NULL,
									  reqtypioparam, reqtypmod);
		}
		MemoryContextSwitchTo(oldcontext);
	}

	return value;
}

/* ----------
 * exec_simple_cast_value			Cast a value if required
 *
 * As above, but need not supply details about target type.  Note that this
 * is slower than exec_cast_value with cached type info, and so should be
 * avoided in heavily used code paths.
 * ----------
 */
static Datum
exec_simple_cast_value(PLpgSQL_execstate *estate,
					   Datum value, Oid valtype,
					   Oid reqtype, int32 reqtypmod,
					   bool isnull)
{
	if (valtype != reqtype || reqtypmod != -1)
	{
		Oid			typinput;
		Oid			typioparam;
		FmgrInfo	finfo_input;

		getTypeInputInfo(reqtype, &typinput, &typioparam);

		fmgr_info(typinput, &finfo_input);

		value = exec_cast_value(estate,
								value,
								valtype,
								reqtype,
								&finfo_input,
								typioparam,
								reqtypmod,
								isnull);
	}

	return value;
}


/* ----------
 * exec_simple_check_node -		Recursively check if an expression
 *								is made only of simple things we can
 *								hand out directly to ExecEvalExpr()
 *								instead of calling SPI.
 * ----------
 */
static bool
exec_simple_check_node(Node *node)
{
	if (node == NULL)
		return TRUE;

	switch (nodeTag(node))
	{
		case T_Const:
			return TRUE;

		case T_Param:
			return TRUE;

		case T_ArrayRef:
			{
				ArrayRef   *expr = (ArrayRef *) node;

				if (!exec_simple_check_node((Node *) expr->refupperindexpr))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->reflowerindexpr))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->refexpr))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->refassgnexpr))
					return FALSE;

				return TRUE;
			}

		case T_FuncExpr:
			{
				FuncExpr   *expr = (FuncExpr *) node;

				if (expr->funcretset)
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_OpExpr:
			{
				OpExpr	   *expr = (OpExpr *) node;

				if (expr->opretset)
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_DistinctExpr:
			{
				DistinctExpr *expr = (DistinctExpr *) node;

				if (expr->opretset)
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_NullIfExpr:
			{
				NullIfExpr *expr = (NullIfExpr *) node;

				if (expr->opretset)
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_ScalarArrayOpExpr:
			{
				ScalarArrayOpExpr *expr = (ScalarArrayOpExpr *) node;

				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_BoolExpr:
			{
				BoolExpr   *expr = (BoolExpr *) node;

				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_FieldSelect:
			return exec_simple_check_node((Node *) ((FieldSelect *) node)->arg);

		case T_FieldStore:
			{
				FieldStore *expr = (FieldStore *) node;

				if (!exec_simple_check_node((Node *) expr->arg))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->newvals))
					return FALSE;

				return TRUE;
			}

		case T_RelabelType:
			return exec_simple_check_node((Node *) ((RelabelType *) node)->arg);

		case T_CoerceViaIO:
			return exec_simple_check_node((Node *) ((CoerceViaIO *) node)->arg);

		case T_ArrayCoerceExpr:
			return exec_simple_check_node((Node *) ((ArrayCoerceExpr *) node)->arg);

		case T_ConvertRowtypeExpr:
			return exec_simple_check_node((Node *) ((ConvertRowtypeExpr *) node)->arg);

		case T_CaseExpr:
			{
				CaseExpr   *expr = (CaseExpr *) node;

				if (!exec_simple_check_node((Node *) expr->arg))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->defresult))
					return FALSE;

				return TRUE;
			}

		case T_CaseWhen:
			{
				CaseWhen   *when = (CaseWhen *) node;

				if (!exec_simple_check_node((Node *) when->expr))
					return FALSE;
				if (!exec_simple_check_node((Node *) when->result))
					return FALSE;

				return TRUE;
			}

		case T_CaseTestExpr:
			return TRUE;

		case T_ArrayExpr:
			{
				ArrayExpr  *expr = (ArrayExpr *) node;

				if (!exec_simple_check_node((Node *) expr->elements))
					return FALSE;

				return TRUE;
			}

		case T_RowExpr:
			{
				RowExpr    *expr = (RowExpr *) node;

				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_RowCompareExpr:
			{
				RowCompareExpr *expr = (RowCompareExpr *) node;

				if (!exec_simple_check_node((Node *) expr->largs))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->rargs))
					return FALSE;

				return TRUE;
			}

		case T_CoalesceExpr:
			{
				CoalesceExpr *expr = (CoalesceExpr *) node;

				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_MinMaxExpr:
			{
				MinMaxExpr *expr = (MinMaxExpr *) node;

				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_XmlExpr:
			{
				XmlExpr    *expr = (XmlExpr *) node;

				if (!exec_simple_check_node((Node *) expr->named_args))
					return FALSE;
				if (!exec_simple_check_node((Node *) expr->args))
					return FALSE;

				return TRUE;
			}

		case T_NullTest:
			return exec_simple_check_node((Node *) ((NullTest *) node)->arg);

		case T_BooleanTest:
			return exec_simple_check_node((Node *) ((BooleanTest *) node)->arg);

		case T_CoerceToDomain:
			return exec_simple_check_node((Node *) ((CoerceToDomain *) node)->arg);

		case T_CoerceToDomainValue:
			return TRUE;

		case T_List:
			{
				List	   *expr = (List *) node;
				ListCell   *l;

				foreach(l, expr)
				{
					if (!exec_simple_check_node(lfirst(l)))
						return FALSE;
				}

				return TRUE;
			}

		default:
			return FALSE;
	}
}


/* ----------
 * exec_simple_check_plan -		Check if a plan is simple enough to
 *								be evaluated by ExecEvalExpr() instead
 *								of SPI.
 * ----------
 */
static void
exec_simple_check_plan(PLpgSQL_expr *expr)
{
	CachedPlanSource *plansource;
	PlannedStmt *stmt;
	Plan	   *plan;
	TargetEntry *tle;

	/*
	 * Initialize to "not simple", and remember the plan generation number we
	 * last checked.  (If the query produces more or less than one parsetree
	 * we just leave expr_simple_generation set to 0.)
	 */
	expr->expr_simple_expr = NULL;
	expr->expr_simple_generation = 0;

	/*
	 * 1. We can only evaluate queries that resulted in one single execution
	 * plan
	 */
	if (list_length(expr->plan->plancache_list) != 1)
		return;
	plansource = (CachedPlanSource *) linitial(expr->plan->plancache_list);
	expr->expr_simple_generation = plansource->generation;
	if (list_length(plansource->plan->stmt_list) != 1)
		return;

	stmt = (PlannedStmt *) linitial(plansource->plan->stmt_list);

	/*
	 * 2. It must be a RESULT plan --> no scan's required
	 */
	if (!IsA(stmt, PlannedStmt))
		return;
	if (stmt->commandType != CMD_SELECT || stmt->intoClause)
		return;
	plan = stmt->planTree;
	if (!IsA(plan, Result))
		return;

	/*
	 * 3. Can't have any subplan or qual clause, either
	 */
	if (plan->lefttree != NULL ||
		plan->righttree != NULL ||
		plan->initPlan != NULL ||
		plan->qual != NULL ||
		((Result *) plan)->resconstantqual != NULL)
		return;

	/*
	 * 4. The plan must have a single attribute as result
	 */
	if (list_length(plan->targetlist) != 1)
		return;

	tle = (TargetEntry *) linitial(plan->targetlist);

	/*
	 * 5. Check that all the nodes in the expression are non-scary.
	 */
	if (!exec_simple_check_node((Node *) tle->expr))
		return;

	/*
	 * Yes - this is a simple expression.  Mark it as such, and initialize
	 * state to "not valid in current transaction".
	 */
	expr->expr_simple_expr = tle->expr;
	expr->expr_simple_state = NULL;
	expr->expr_simple_in_use = false;
	expr->expr_simple_lxid = InvalidLocalTransactionId;
	/* Also stash away the expression result type */
	expr->expr_simple_type = exprType((Node *) tle->expr);
}

/* ----------
 * exec_set_found			Set the global found variable
 *					to true/false
 * ----------
 */
static void
exec_set_found(PLpgSQL_execstate *estate, bool state)
{
	PLpgSQL_var *var;

	var = (PLpgSQL_var *) (estate->datums[estate->found_varno]);
	var->value = PointerGetDatum(state);
	var->isnull = false;
}

/*
 * plpgsql_create_econtext --- create an eval_econtext for the current function
 *
 * We may need to create a new simple_eval_estate too, if there's not one
 * already for the current transaction.  The EState will be cleaned up at
 * transaction end.
 */
static void
plpgsql_create_econtext(PLpgSQL_execstate *estate)
{
	SimpleEcontextStackEntry *entry;

	/*
	 * Create an EState for evaluation of simple expressions, if there's not
	 * one already in the current transaction.	The EState is made a child of
	 * TopTransactionContext so it will have the right lifespan.
	 */
	if (simple_eval_estate == NULL)
	{
		MemoryContext oldcontext;

		oldcontext = MemoryContextSwitchTo(TopTransactionContext);
		simple_eval_estate = CreateExecutorState();
		MemoryContextSwitchTo(oldcontext);
	}

	/*
	 * Create a child econtext for the current function.
	 */
	estate->eval_econtext = CreateExprContext(simple_eval_estate);

	/*
	 * Make a stack entry so we can clean up the econtext at subxact end.
	 * Stack entries are kept in TopTransactionContext for simplicity.
	 */
	entry = (SimpleEcontextStackEntry *)
		MemoryContextAlloc(TopTransactionContext,
						   sizeof(SimpleEcontextStackEntry));

	entry->stack_econtext = estate->eval_econtext;
	entry->xact_subxid = GetCurrentSubTransactionId();

	entry->next = simple_econtext_stack;
	simple_econtext_stack = entry;
}

/*
 * plpgsql_destroy_econtext --- destroy function's econtext
 *
 * We check that it matches the top stack entry, and destroy the stack
 * entry along with the context.
 */
static void
plpgsql_destroy_econtext(PLpgSQL_execstate *estate)
{
	SimpleEcontextStackEntry *next;

	Assert(simple_econtext_stack != NULL);
	Assert(simple_econtext_stack->stack_econtext == estate->eval_econtext);

	next = simple_econtext_stack->next;
	pfree(simple_econtext_stack);
	simple_econtext_stack = next;

	FreeExprContext(estate->eval_econtext, true);
	estate->eval_econtext = NULL;
}

/*
 * plpgsql_xact_cb --- post-transaction-commit-or-abort cleanup
 *
 * If a simple-expression EState was created in the current transaction,
 * it has to be cleaned up.
 */
void
plpgsql_xact_cb(XactEvent event, void *arg)
{
	/*
	 * If we are doing a clean transaction shutdown, free the EState (so that
	 * any remaining resources will be released correctly). In an abort, we
	 * expect the regular abort recovery procedures to release everything of
	 * interest.
	 */
	if (event != XACT_EVENT_ABORT)
	{
		/* Shouldn't be any econtext stack entries left at commit */
		Assert(simple_econtext_stack == NULL);

		if (simple_eval_estate)
			FreeExecutorState(simple_eval_estate);
		simple_eval_estate = NULL;
	}
	else
	{
		simple_econtext_stack = NULL;
		simple_eval_estate = NULL;
	}
}

/*
 * plpgsql_subxact_cb --- post-subtransaction-commit-or-abort cleanup
 *
 * Make sure any simple-expression econtexts created in the current
 * subtransaction get cleaned up.  We have to do this explicitly because
 * no other code knows which child econtexts of simple_eval_estate belong
 * to which level of subxact.
 */
void
plpgsql_subxact_cb(SubXactEvent event, SubTransactionId mySubid,
				   SubTransactionId parentSubid, void *arg)
{
	if (event == SUBXACT_EVENT_START_SUB)
		return;

	while (simple_econtext_stack != NULL &&
		   simple_econtext_stack->xact_subxid == mySubid)
	{
		SimpleEcontextStackEntry *next;

		FreeExprContext(simple_econtext_stack->stack_econtext,
						(event == SUBXACT_EVENT_COMMIT_SUB));
		next = simple_econtext_stack->next;
		pfree(simple_econtext_stack);
		simple_econtext_stack = next;
	}
}

/*
 * free_var --- pfree any pass-by-reference value of the variable.
 *
 * This should always be followed by some assignment to var->value,
 * as it leaves a dangling pointer.
 */
static void
free_var(PLpgSQL_var *var)
{
	if (var->freeval)
	{
		pfree(DatumGetPointer(var->value));
		var->freeval = false;
	}
}

/*
 * free old value of a text variable and assign new value from C string
 */
static void
assign_text_var(PLpgSQL_var *var, const char *str)
{
	free_var(var);
	var->value = CStringGetTextDatum(str);
	var->isnull = false;
	var->freeval = true;
}

/*
 * exec_eval_using_params --- evaluate params of USING clause
 */
static PreparedParamsData *
exec_eval_using_params(PLpgSQL_execstate *estate, List *params)
{
	PreparedParamsData *ppd;
	int			nargs;
	int			i;
	ListCell   *lc;

	ppd = (PreparedParamsData *) palloc(sizeof(PreparedParamsData));
	nargs = list_length(params);

	ppd->nargs = nargs;
	ppd->types = (Oid *) palloc(nargs * sizeof(Oid));
	ppd->values = (Datum *) palloc(nargs * sizeof(Datum));
	ppd->nulls = (char *) palloc(nargs * sizeof(char));
	ppd->freevals = (bool *) palloc(nargs * sizeof(bool));

	i = 0;
	foreach(lc, params)
	{
		PLpgSQL_expr *param = (PLpgSQL_expr *) lfirst(lc);
		bool		isnull;

		ppd->values[i] = exec_eval_expr(estate, param,
										&isnull,
										&ppd->types[i]);
		ppd->nulls[i] = isnull ? 'n' : ' ';
		ppd->freevals[i] = false;

		if (ppd->types[i] == UNKNOWNOID)
		{
			/*
			 * Treat 'unknown' parameters as text, since that's what most
			 * people would expect. SPI_execute_with_args can coerce unknown
			 * constants in a more intelligent way, but not unknown Params.
			 * This code also takes care of copying into the right context.
			 * Note we assume 'unknown' has the representation of C-string.
			 */
			ppd->types[i] = TEXTOID;
			if (!isnull)
			{
				ppd->values[i] = CStringGetTextDatum(DatumGetCString(ppd->values[i]));
				ppd->freevals[i] = true;
			}
		}
		/* pass-by-ref non null values must be copied into plpgsql context */
		else if (!isnull)
		{
			int16		typLen;
			bool		typByVal;

			get_typlenbyval(ppd->types[i], &typLen, &typByVal);
			if (!typByVal)
			{
				ppd->values[i] = datumCopy(ppd->values[i], typByVal, typLen);
				ppd->freevals[i] = true;
			}
		}

		exec_eval_cleanup(estate);

		i++;
	}

	return ppd;
}

/*
 * free_params_data --- pfree all pass-by-reference values used in USING clause
 */
static void
free_params_data(PreparedParamsData *ppd)
{
	int			i;

	for (i = 0; i < ppd->nargs; i++)
	{
		if (ppd->freevals[i])
			pfree(DatumGetPointer(ppd->values[i]));
	}

	pfree(ppd->types);
	pfree(ppd->values);
	pfree(ppd->nulls);
	pfree(ppd->freevals);

	pfree(ppd);
}

/*
 * Open portal for dynamic query
 */
static Portal
exec_dynquery_with_params(PLpgSQL_execstate *estate,
						  PLpgSQL_expr *dynquery,
						  List *params,
						  const char *portalname,
						  int cursorOptions)
{
	Portal		portal;
	Datum		query;
	bool		isnull;
	Oid			restype;
	char	   *querystr;

	/*
	 * Evaluate the string expression after the EXECUTE keyword. Its result is
	 * the querystring we have to execute.
	 */
	query = exec_eval_expr(estate, dynquery, &isnull, &restype);
	if (isnull)
		ereport(ERROR,
				(errcode(ERRCODE_NULL_VALUE_NOT_ALLOWED),
				 errmsg("query string argument of EXECUTE is null")));

	/* Get the C-String representation */
	querystr = convert_value_to_string(estate, query, restype);

	/* copy it out of the temporary context before we clean up */
	querystr = pstrdup(querystr);

	exec_eval_cleanup(estate);

	/*
	 * Open an implicit cursor for the query.  We use
	 * SPI_cursor_open_with_args even when there are no params, because this
	 * avoids making and freeing one copy of the plan.
	 */
	if (params)
	{
		PreparedParamsData *ppd;

		ppd = exec_eval_using_params(estate, params);
		portal = SPI_cursor_open_with_args(portalname,
										   querystr,
										   ppd->nargs, ppd->types,
										   ppd->values, ppd->nulls,
										   estate->readonly_func,
										   cursorOptions);
		free_params_data(ppd);
	}
	else
	{
		portal = SPI_cursor_open_with_args(portalname,
										   querystr,
										   0, NULL,
										   NULL, NULL,
										   estate->readonly_func,
										   cursorOptions);
	}

	if (portal == NULL)
		elog(ERROR, "could not open implicit cursor for query \"%s\": %s",
			 querystr, SPI_result_code_string(SPI_result));
	pfree(querystr);

	return portal;
}
