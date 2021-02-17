/**********************************************************************
 * pltcl.c		- PostgreSQL support for Tcl as
 *				  procedural language (PL)
 *
 *	  src/pl/tcl/pltcl.c
 *
 **********************************************************************/

#include "postgres.h"

#include <tcl.h>

#include <unistd.h>
#include <fcntl.h>

/* Hack to deal with Tcl 8.4 const-ification without losing compatibility */
#ifndef CONST84
#define CONST84
#endif

#include "access/xact.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/trigger.h"
#include "executor/spi.h"
#include "fmgr.h"
#include "miscadmin.h"
#include "nodes/makefuncs.h"
#include "parser/parse_type.h"
#include "tcop/tcopprot.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/syscache.h"
#include "utils/typcache.h"


#define HAVE_TCL_VERSION(maj,min) \
	((TCL_MAJOR_VERSION > maj) || \
	 (TCL_MAJOR_VERSION == maj && TCL_MINOR_VERSION >= min))

/* In Tcl >= 8.0, really not supposed to touch interp->result directly */
#if !HAVE_TCL_VERSION(8,0)
#define Tcl_GetStringResult(interp)  ((interp)->result)
#endif

/* define our text domain for translations */
#undef TEXTDOMAIN
#define TEXTDOMAIN PG_TEXTDOMAIN("pltcl")

#if defined(UNICODE_CONVERSION) && HAVE_TCL_VERSION(8,1)

#include "mb/pg_wchar.h"

static unsigned char *
utf_u2e(unsigned char *src)
{
	return pg_do_encoding_conversion(src, strlen(src), PG_UTF8, GetDatabaseEncoding());
}

static unsigned char *
utf_e2u(unsigned char *src)
{
	return pg_do_encoding_conversion(src, strlen(src), GetDatabaseEncoding(), PG_UTF8);
}

#define PLTCL_UTF
#define UTF_BEGIN	 do { \
					unsigned char *_pltcl_utf_src; \
					unsigned char *_pltcl_utf_dst
#define UTF_END		 if (_pltcl_utf_src!=_pltcl_utf_dst) \
					pfree(_pltcl_utf_dst); } while (0)
#define UTF_U2E(x)	 (_pltcl_utf_dst=utf_u2e(_pltcl_utf_src=(x)))
#define UTF_E2U(x)	 (_pltcl_utf_dst=utf_e2u(_pltcl_utf_src=(x)))
#else							/* !PLTCL_UTF */

#define  UTF_BEGIN
#define  UTF_END
#define  UTF_U2E(x)  (x)
#define  UTF_E2U(x)  (x)
#endif   /* PLTCL_UTF */

PG_MODULE_MAGIC;


/**********************************************************************
 * Information associated with a Tcl interpreter.  We have one interpreter
 * that is used for all pltclu (untrusted) functions.  For pltcl (trusted)
 * functions, there is a separate interpreter for each effective SQL userid.
 * (This is needed to ensure that an unprivileged user can't inject Tcl code
 * that'll be executed with the privileges of some other SQL user.)
 *
 * The pltcl_interp_desc structs are kept in a Postgres hash table indexed
 * by userid OID, with OID 0 used for the single untrusted interpreter.
 **********************************************************************/
typedef struct pltcl_interp_desc
{
	Oid			user_id;		/* Hash key (must be first!) */
	Tcl_Interp *interp;			/* The interpreter */
	Tcl_HashTable query_hash;	/* pltcl_query_desc structs */
} pltcl_interp_desc;


/**********************************************************************
 * The information we cache about loaded procedures
 **********************************************************************/
typedef struct pltcl_proc_desc
{
	char	   *user_proname;
	char	   *internal_proname;
	TransactionId fn_xmin;
	ItemPointerData fn_tid;
	bool		fn_readonly;
	bool		lanpltrusted;
	pltcl_interp_desc *interp_desc;
	FmgrInfo	result_in_func;
	Oid			result_typioparam;
	int			nargs;
	FmgrInfo	arg_out_func[FUNC_MAX_ARGS];
	bool		arg_is_rowtype[FUNC_MAX_ARGS];
} pltcl_proc_desc;


/**********************************************************************
 * The information we cache about prepared and saved plans
 **********************************************************************/
typedef struct pltcl_query_desc
{
	char		qname[20];
	void	   *plan;
	int			nargs;
	Oid		   *argtypes;
	FmgrInfo   *arginfuncs;
	Oid		   *argtypioparams;
} pltcl_query_desc;


/**********************************************************************
 * For speedy lookup, we maintain a hash table mapping from
 * function OID + trigger flag + user OID to pltcl_proc_desc pointers.
 * The reason the pltcl_proc_desc struct isn't directly part of the hash
 * entry is to simplify recovery from errors during compile_pltcl_function.
 *
 * Note: if the same function is called by multiple userIDs within a session,
 * there will be a separate pltcl_proc_desc entry for each userID in the case
 * of pltcl functions, but only one entry for pltclu functions, because we
 * set user_id = 0 for that case.
 **********************************************************************/
typedef struct pltcl_proc_key
{
	Oid			proc_id;		/* Function OID */

	/*
	 * is_trigger is really a bool, but declare as Oid to ensure this struct
	 * contains no padding
	 */
	Oid			is_trigger;		/* is it a trigger function? */
	Oid			user_id;		/* User calling the function, or 0 */
} pltcl_proc_key;

typedef struct pltcl_proc_ptr
{
	pltcl_proc_key proc_key;	/* Hash key (must be first!) */
	pltcl_proc_desc *proc_ptr;
} pltcl_proc_ptr;


/**********************************************************************
 * Global data
 **********************************************************************/
static bool pltcl_pm_init_done = false;
static Tcl_Interp *pltcl_hold_interp = NULL;
static HTAB *pltcl_interp_htab = NULL;
static HTAB *pltcl_proc_htab = NULL;

/* these are saved and restored by pltcl_handler */
static FunctionCallInfo pltcl_current_fcinfo = NULL;
static pltcl_proc_desc *pltcl_current_prodesc = NULL;

/**********************************************************************
 * Forward declarations
 **********************************************************************/
Datum		pltcl_call_handler(PG_FUNCTION_ARGS);
Datum		pltclu_call_handler(PG_FUNCTION_ARGS);
void		_PG_init(void);

static void pltcl_init_interp(pltcl_interp_desc *interp_desc, bool pltrusted);
static pltcl_interp_desc *pltcl_fetch_interp(bool pltrusted);
static void pltcl_init_load_unknown(Tcl_Interp *interp);

static Datum pltcl_handler(PG_FUNCTION_ARGS, bool pltrusted);

static Datum pltcl_func_handler(PG_FUNCTION_ARGS, bool pltrusted);

static HeapTuple pltcl_trigger_handler(PG_FUNCTION_ARGS, bool pltrusted);

static void throw_tcl_error(Tcl_Interp *interp, const char *proname);

static pltcl_proc_desc *compile_pltcl_function(Oid fn_oid, Oid tgreloid,
					   bool pltrusted);

static int pltcl_elog(ClientData cdata, Tcl_Interp *interp,
		   int argc, CONST84 char *argv[]);
static int pltcl_quote(ClientData cdata, Tcl_Interp *interp,
			int argc, CONST84 char *argv[]);
static int pltcl_argisnull(ClientData cdata, Tcl_Interp *interp,
				int argc, CONST84 char *argv[]);
static int pltcl_returnnull(ClientData cdata, Tcl_Interp *interp,
				 int argc, CONST84 char *argv[]);

static int pltcl_SPI_execute(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[]);
static int pltcl_process_SPI_result(Tcl_Interp *interp,
						 CONST84 char *arrayname,
						 CONST84 char *loop_body,
						 int spi_rc,
						 SPITupleTable *tuptable,
						 int ntuples);
static int pltcl_SPI_prepare(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[]);
static int pltcl_SPI_execute_plan(ClientData cdata, Tcl_Interp *interp,
					   int argc, CONST84 char *argv[]);
static int pltcl_SPI_lastoid(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[]);

static void pltcl_set_tuple_values(Tcl_Interp *interp, CONST84 char *arrayname,
					   int tupno, HeapTuple tuple, TupleDesc tupdesc);
static void pltcl_build_tuple_argument(HeapTuple tuple, TupleDesc tupdesc,
						   Tcl_DString *retval);


/*
 * Hack to override Tcl's builtin Notifier subsystem.  This prevents the
 * backend from becoming multithreaded, which breaks all sorts of things.
 * That happens in the default version of Tcl_InitNotifier if the TCL library
 * has been compiled with multithreading support (i.e. when TCL_THREADS is
 * defined under Unix, and in all cases under Windows).
 * It's okay to disable the notifier because we never enter the Tcl event loop
 * from Postgres, so the notifier capabilities are initialized, but never
 * used.  Only InitNotifier and DeleteFileHandler ever seem to get called
 * within Postgres, but we implement all the functions for completeness.
 * We can only fix this with Tcl >= 8.4, when Tcl_SetNotifier() appeared.
 */
#if HAVE_TCL_VERSION(8,4)

static ClientData
pltcl_InitNotifier(void)
{
	static int	fakeThreadKey;	/* To give valid address for ClientData */

	return (ClientData) &(fakeThreadKey);
}

static void
pltcl_FinalizeNotifier(ClientData clientData)
{
}

static void
pltcl_SetTimer(Tcl_Time *timePtr)
{
}

static void
pltcl_AlertNotifier(ClientData clientData)
{
}

static void
pltcl_CreateFileHandler(int fd, int mask,
						Tcl_FileProc *proc, ClientData clientData)
{
}

static void
pltcl_DeleteFileHandler(int fd)
{
}

static void
pltcl_ServiceModeHook(int mode)
{
}

static int
pltcl_WaitForEvent(Tcl_Time *timePtr)
{
	return 0;
}
#endif   /* HAVE_TCL_VERSION(8,4) */


/*
 * This routine is a crock, and so is everyplace that calls it.  The problem
 * is that the cached form of pltcl functions/queries is allocated permanently
 * (mostly via malloc()) and never released until backend exit.  Subsidiary
 * data structures such as fmgr info records therefore must live forever
 * as well.  A better implementation would store all this stuff in a per-
 * function memory context that could be reclaimed at need.  In the meantime,
 * fmgr_info_cxt must be called specifying TopMemoryContext so that whatever
 * it might allocate, and whatever the eventual function might allocate using
 * fn_mcxt, will live forever too.
 */
static void
perm_fmgr_info(Oid functionId, FmgrInfo *finfo)
{
	fmgr_info_cxt(functionId, finfo, TopMemoryContext);
}

/*
 * _PG_init()			- library load-time initialization
 *
 * DO NOT make this static nor change its name!
 *
 * The work done here must be safe to do in the postmaster process,
 * in case the pltcl library is preloaded in the postmaster.
 */
void
_PG_init(void)
{
	HASHCTL		hash_ctl;

	/* Be sure we do initialization only once (should be redundant now) */
	if (pltcl_pm_init_done)
		return;

	pg_bindtextdomain(TEXTDOMAIN);

#ifdef WIN32
	/* Required on win32 to prevent error loading init.tcl */
	Tcl_FindExecutable("");
#endif

#if HAVE_TCL_VERSION(8,4)

	/*
	 * Override the functions in the Notifier subsystem.  See comments above.
	 */
	{
		Tcl_NotifierProcs notifier;

		notifier.setTimerProc = pltcl_SetTimer;
		notifier.waitForEventProc = pltcl_WaitForEvent;
		notifier.createFileHandlerProc = pltcl_CreateFileHandler;
		notifier.deleteFileHandlerProc = pltcl_DeleteFileHandler;
		notifier.initNotifierProc = pltcl_InitNotifier;
		notifier.finalizeNotifierProc = pltcl_FinalizeNotifier;
		notifier.alertNotifierProc = pltcl_AlertNotifier;
		notifier.serviceModeHookProc = pltcl_ServiceModeHook;
		Tcl_SetNotifier(&notifier);
	}
#endif

	/************************************************************
	 * Create the dummy hold interpreter to prevent close of
	 * stdout and stderr on DeleteInterp
	 ************************************************************/
	if ((pltcl_hold_interp = Tcl_CreateInterp()) == NULL)
		elog(ERROR, "could not create master Tcl interpreter");
	if (Tcl_Init(pltcl_hold_interp) == TCL_ERROR)
		elog(ERROR, "could not initialize master Tcl interpreter");

	/************************************************************
	 * Create the hash table for working interpreters
	 ************************************************************/
	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(Oid);
	hash_ctl.entrysize = sizeof(pltcl_interp_desc);
	hash_ctl.hash = oid_hash;
	pltcl_interp_htab = hash_create("PL/Tcl interpreters",
									8,
									&hash_ctl,
									HASH_ELEM | HASH_FUNCTION);

	/************************************************************
	 * Create the hash table for function lookup
	 ************************************************************/
	memset(&hash_ctl, 0, sizeof(hash_ctl));
	hash_ctl.keysize = sizeof(pltcl_proc_key);
	hash_ctl.entrysize = sizeof(pltcl_proc_ptr);
	hash_ctl.hash = tag_hash;
	pltcl_proc_htab = hash_create("PL/Tcl functions",
								  100,
								  &hash_ctl,
								  HASH_ELEM | HASH_FUNCTION);

	pltcl_pm_init_done = true;
}

/**********************************************************************
 * pltcl_init_interp() - initialize a new Tcl interpreter
 **********************************************************************/
static void
pltcl_init_interp(pltcl_interp_desc *interp_desc, bool pltrusted)
{
	Tcl_Interp *interp;
	char		interpname[32];

	/************************************************************
	 * Create the Tcl interpreter as a slave of pltcl_hold_interp.
	 * Note: Tcl automatically does Tcl_Init in the untrusted case,
	 * and it's not wanted in the trusted case.
	 ************************************************************/
	snprintf(interpname, sizeof(interpname), "slave_%u", interp_desc->user_id);
	if ((interp = Tcl_CreateSlave(pltcl_hold_interp, interpname,
								  pltrusted ? 1 : 0)) == NULL)
		elog(ERROR, "could not create slave Tcl interpreter");
	interp_desc->interp = interp;

	/************************************************************
	 * Initialize the query hash table associated with interpreter
	 ************************************************************/
	Tcl_InitHashTable(&interp_desc->query_hash, TCL_STRING_KEYS);

	/************************************************************
	 * Install the commands for SPI support in the interpreter
	 ************************************************************/
	Tcl_CreateCommand(interp, "elog",
					  pltcl_elog, NULL, NULL);
	Tcl_CreateCommand(interp, "quote",
					  pltcl_quote, NULL, NULL);
	Tcl_CreateCommand(interp, "argisnull",
					  pltcl_argisnull, NULL, NULL);
	Tcl_CreateCommand(interp, "return_null",
					  pltcl_returnnull, NULL, NULL);

	Tcl_CreateCommand(interp, "spi_exec",
					  pltcl_SPI_execute, NULL, NULL);
	Tcl_CreateCommand(interp, "spi_prepare",
					  pltcl_SPI_prepare, NULL, NULL);
	Tcl_CreateCommand(interp, "spi_execp",
					  pltcl_SPI_execute_plan, NULL, NULL);
	Tcl_CreateCommand(interp, "spi_lastoid",
					  pltcl_SPI_lastoid, NULL, NULL);

	/************************************************************
	 * Try to load the unknown procedure from pltcl_modules
	 ************************************************************/
	pltcl_init_load_unknown(interp);
}

/**********************************************************************
 * pltcl_fetch_interp() - fetch the Tcl interpreter to use for a function
 *
 * This also takes care of any on-first-use initialization required.
 * Note: we assume caller has already connected to SPI.
 **********************************************************************/
static pltcl_interp_desc *
pltcl_fetch_interp(bool pltrusted)
{
	Oid			user_id;
	pltcl_interp_desc *interp_desc;
	bool		found;

	/* Find or create the interpreter hashtable entry for this userid */
	if (pltrusted)
		user_id = GetUserId();
	else
		user_id = InvalidOid;

	interp_desc = hash_search(pltcl_interp_htab, &user_id,
							  HASH_ENTER,
							  &found);
	if (!found)
		pltcl_init_interp(interp_desc, pltrusted);

	return interp_desc;
}

/**********************************************************************
 * pltcl_init_load_unknown()	- Load the unknown procedure from
 *				  table pltcl_modules (if it exists)
 **********************************************************************/
static void
pltcl_init_load_unknown(Tcl_Interp *interp)
{
	Relation	pmrel;
	char	   *pmrelname,
			   *nspname;
	char	   *buf;
	int			buflen;
	int			spi_rc;
	int			tcl_rc;
	Tcl_DString unknown_src;
	char	   *part;
	int			i;
	int			fno;

	/************************************************************
	 * Check if table pltcl_modules exists
	 *
	 * We allow the table to be found anywhere in the search_path.
	 * This is for backwards compatibility.  To ensure that the table
	 * is trustworthy, we require it to be owned by a superuser.
	 ************************************************************/
	pmrel = try_relation_openrv(makeRangeVar(NULL, "pltcl_modules", -1),
								AccessShareLock);
	if (pmrel == NULL)
		return;
	/* must be table or view, else ignore */
	if (!(pmrel->rd_rel->relkind == RELKIND_RELATION ||
		  pmrel->rd_rel->relkind == RELKIND_VIEW))
	{
		relation_close(pmrel, AccessShareLock);
		return;
	}
	/* must be owned by superuser, else ignore */
	if (!superuser_arg(pmrel->rd_rel->relowner))
	{
		relation_close(pmrel, AccessShareLock);
		return;
	}
	/* get fully qualified table name for use in select command */
	nspname = get_namespace_name(RelationGetNamespace(pmrel));
	if (!nspname)
		elog(ERROR, "cache lookup failed for namespace %u",
			 RelationGetNamespace(pmrel));
	pmrelname = quote_qualified_identifier(nspname,
										   RelationGetRelationName(pmrel));

	/************************************************************
	 * Read all the rows from it where modname = 'unknown',
	 * in the order of modseq
	 ************************************************************/
	buflen = strlen(pmrelname) + 100;
	buf = (char *) palloc(buflen);
	snprintf(buf, buflen,
		   "select modsrc from %s where modname = 'unknown' order by modseq",
			 pmrelname);

	spi_rc = SPI_execute(buf, false, 0);
	if (spi_rc != SPI_OK_SELECT)
		elog(ERROR, "select from pltcl_modules failed");

	pfree(buf);

	/************************************************************
	 * If there's nothing, module unknown doesn't exist
	 ************************************************************/
	if (SPI_processed == 0)
	{
		SPI_freetuptable(SPI_tuptable);
		elog(WARNING, "module \"unknown\" not found in pltcl_modules");
		relation_close(pmrel, AccessShareLock);
		return;
	}

	/************************************************************
	 * There is a module named unknown. Reassemble the
	 * source from the modsrc attributes and evaluate
	 * it in the Tcl interpreter
	 ************************************************************/
	fno = SPI_fnumber(SPI_tuptable->tupdesc, "modsrc");

	Tcl_DStringInit(&unknown_src);

	for (i = 0; i < SPI_processed; i++)
	{
		part = SPI_getvalue(SPI_tuptable->vals[i],
							SPI_tuptable->tupdesc, fno);
		if (part != NULL)
		{
			UTF_BEGIN;
			Tcl_DStringAppend(&unknown_src, UTF_E2U(part), -1);
			UTF_END;
			pfree(part);
		}
	}
	tcl_rc = Tcl_GlobalEval(interp, Tcl_DStringValue(&unknown_src));

	Tcl_DStringFree(&unknown_src);
	SPI_freetuptable(SPI_tuptable);

	if (tcl_rc != TCL_OK)
	{
		UTF_BEGIN;
		elog(ERROR, "could not load module \"unknown\": %s",
			 UTF_U2E(Tcl_GetStringResult(interp)));
		UTF_END;
	}

	relation_close(pmrel, AccessShareLock);
}


/**********************************************************************
 * pltcl_call_handler		- This is the only visible function
 *				  of the PL interpreter. The PostgreSQL
 *				  function manager and trigger manager
 *				  call this function for execution of
 *				  PL/Tcl procedures.
 **********************************************************************/
PG_FUNCTION_INFO_V1(pltcl_call_handler);

/* keep non-static */
Datum
pltcl_call_handler(PG_FUNCTION_ARGS)
{
	return pltcl_handler(fcinfo, true);
}

/*
 * Alternative handler for unsafe functions
 */
PG_FUNCTION_INFO_V1(pltclu_call_handler);

/* keep non-static */
Datum
pltclu_call_handler(PG_FUNCTION_ARGS)
{
	return pltcl_handler(fcinfo, false);
}


static Datum
pltcl_handler(PG_FUNCTION_ARGS, bool pltrusted)
{
	Datum		retval;
	FunctionCallInfo save_fcinfo;
	pltcl_proc_desc *save_prodesc;

	/*
	 * Ensure that static pointers are saved/restored properly
	 */
	save_fcinfo = pltcl_current_fcinfo;
	save_prodesc = pltcl_current_prodesc;

	PG_TRY();
	{
		/*
		 * Determine if called as function or trigger and call appropriate
		 * subhandler
		 */
		if (CALLED_AS_TRIGGER(fcinfo))
		{
			pltcl_current_fcinfo = NULL;
			retval = PointerGetDatum(pltcl_trigger_handler(fcinfo, pltrusted));
		}
		else
		{
			pltcl_current_fcinfo = fcinfo;
			retval = pltcl_func_handler(fcinfo, pltrusted);
		}
	}
	PG_CATCH();
	{
		pltcl_current_fcinfo = save_fcinfo;
		pltcl_current_prodesc = save_prodesc;
		PG_RE_THROW();
	}
	PG_END_TRY();

	pltcl_current_fcinfo = save_fcinfo;
	pltcl_current_prodesc = save_prodesc;

	return retval;
}


/**********************************************************************
 * pltcl_func_handler()		- Handler for regular function calls
 **********************************************************************/
static Datum
pltcl_func_handler(PG_FUNCTION_ARGS, bool pltrusted)
{
	pltcl_proc_desc *prodesc;
	Tcl_Interp *volatile interp;
	Tcl_DString tcl_cmd;
	Tcl_DString list_tmp;
	int			i;
	int			tcl_rc;
	Datum		retval;

	/* Connect to SPI manager */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect to SPI manager");

	/* Find or compile the function */
	prodesc = compile_pltcl_function(fcinfo->flinfo->fn_oid, InvalidOid,
									 pltrusted);

	pltcl_current_prodesc = prodesc;

	interp = prodesc->interp_desc->interp;

	/************************************************************
	 * Create the tcl command to call the internal
	 * proc in the Tcl interpreter
	 ************************************************************/
	Tcl_DStringInit(&tcl_cmd);
	Tcl_DStringInit(&list_tmp);
	Tcl_DStringAppendElement(&tcl_cmd, prodesc->internal_proname);

	/************************************************************
	 * Add all call arguments to the command
	 ************************************************************/
	PG_TRY();
	{
		for (i = 0; i < prodesc->nargs; i++)
		{
			if (prodesc->arg_is_rowtype[i])
			{
				/**************************************************
				 * For tuple values, add a list for 'array set ...'
				 **************************************************/
				if (fcinfo->argnull[i])
					Tcl_DStringAppendElement(&tcl_cmd, "");
				else
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
					tmptup.t_data = td;

					Tcl_DStringSetLength(&list_tmp, 0);
					pltcl_build_tuple_argument(&tmptup, tupdesc, &list_tmp);
					Tcl_DStringAppendElement(&tcl_cmd,
											 Tcl_DStringValue(&list_tmp));
					ReleaseTupleDesc(tupdesc);
				}
			}
			else
			{
				/**************************************************
				 * Single values are added as string element
				 * of their external representation
				 **************************************************/
				if (fcinfo->argnull[i])
					Tcl_DStringAppendElement(&tcl_cmd, "");
				else
				{
					char	   *tmp;

					tmp = OutputFunctionCall(&prodesc->arg_out_func[i],
											 fcinfo->arg[i]);
					UTF_BEGIN;
					Tcl_DStringAppendElement(&tcl_cmd, UTF_E2U(tmp));
					UTF_END;
					pfree(tmp);
				}
			}
		}
	}
	PG_CATCH();
	{
		Tcl_DStringFree(&tcl_cmd);
		Tcl_DStringFree(&list_tmp);
		PG_RE_THROW();
	}
	PG_END_TRY();
	Tcl_DStringFree(&list_tmp);

	/************************************************************
	 * Call the Tcl function
	 *
	 * We assume no PG error can be thrown directly from this call.
	 ************************************************************/
	tcl_rc = Tcl_GlobalEval(interp, Tcl_DStringValue(&tcl_cmd));
	Tcl_DStringFree(&tcl_cmd);

	/************************************************************
	 * Check for errors reported by Tcl.
	 ************************************************************/
	if (tcl_rc != TCL_OK)
		throw_tcl_error(interp, prodesc->user_proname);

	/************************************************************
	 * Disconnect from SPI manager and then create the return
	 * value datum (if the input function does a palloc for it
	 * this must not be allocated in the SPI memory context
	 * because SPI_finish would free it).  But don't try to call
	 * the result_in_func if we've been told to return a NULL;
	 * the Tcl result may not be a valid value of the result type
	 * in that case.
	 ************************************************************/
	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish() failed");

	if (fcinfo->isnull)
		retval = InputFunctionCall(&prodesc->result_in_func,
								   NULL,
								   prodesc->result_typioparam,
								   -1);
	else
	{
		UTF_BEGIN;
		retval = InputFunctionCall(&prodesc->result_in_func,
							   UTF_U2E((char *) Tcl_GetStringResult(interp)),
								   prodesc->result_typioparam,
								   -1);
		UTF_END;
	}

	return retval;
}


/**********************************************************************
 * pltcl_trigger_handler()	- Handler for trigger calls
 **********************************************************************/
static HeapTuple
pltcl_trigger_handler(PG_FUNCTION_ARGS, bool pltrusted)
{
	pltcl_proc_desc *prodesc;
	Tcl_Interp *volatile interp;
	TriggerData *trigdata = (TriggerData *) fcinfo->context;
	char	   *stroid;
	TupleDesc	tupdesc;
	volatile HeapTuple rettup;
	Tcl_DString tcl_cmd;
	Tcl_DString tcl_trigtup;
	Tcl_DString tcl_newtup;
	int			tcl_rc;
	int			i;
	int		   *modattrs;
	Datum	   *modvalues;
	char	   *modnulls;
	int			ret_numvals;
	CONST84 char *result;
	CONST84 char **ret_values;

	/* Connect to SPI manager */
	if (SPI_connect() != SPI_OK_CONNECT)
		elog(ERROR, "could not connect to SPI manager");

	/* Find or compile the function */
	prodesc = compile_pltcl_function(fcinfo->flinfo->fn_oid,
									 RelationGetRelid(trigdata->tg_relation),
									 pltrusted);

	pltcl_current_prodesc = prodesc;

	interp = prodesc->interp_desc->interp;

	tupdesc = trigdata->tg_relation->rd_att;

	/************************************************************
	 * Create the tcl command to call the internal
	 * proc in the interpreter
	 ************************************************************/
	Tcl_DStringInit(&tcl_cmd);
	Tcl_DStringInit(&tcl_trigtup);
	Tcl_DStringInit(&tcl_newtup);
	PG_TRY();
	{
		/* The procedure name */
		Tcl_DStringAppendElement(&tcl_cmd, prodesc->internal_proname);

		/* The trigger name for argument TG_name */
		Tcl_DStringAppendElement(&tcl_cmd, trigdata->tg_trigger->tgname);

		/* The oid of the trigger relation for argument TG_relid */
		stroid = DatumGetCString(DirectFunctionCall1(oidout,
							ObjectIdGetDatum(trigdata->tg_relation->rd_id)));
		Tcl_DStringAppendElement(&tcl_cmd, stroid);
		pfree(stroid);

		/* The name of the table the trigger is acting on: TG_table_name */
		stroid = SPI_getrelname(trigdata->tg_relation);
		Tcl_DStringAppendElement(&tcl_cmd, stroid);
		pfree(stroid);

		/* The schema of the table the trigger is acting on: TG_table_schema */
		stroid = SPI_getnspname(trigdata->tg_relation);
		Tcl_DStringAppendElement(&tcl_cmd, stroid);
		pfree(stroid);

		/* A list of attribute names for argument TG_relatts */
		Tcl_DStringAppendElement(&tcl_trigtup, "");
		for (i = 0; i < tupdesc->natts; i++)
		{
			if (tupdesc->attrs[i]->attisdropped)
				Tcl_DStringAppendElement(&tcl_trigtup, "");
			else
				Tcl_DStringAppendElement(&tcl_trigtup,
										 NameStr(tupdesc->attrs[i]->attname));
		}
		Tcl_DStringAppendElement(&tcl_cmd, Tcl_DStringValue(&tcl_trigtup));
		Tcl_DStringFree(&tcl_trigtup);
		Tcl_DStringInit(&tcl_trigtup);

		/* The when part of the event for TG_when */
		if (TRIGGER_FIRED_BEFORE(trigdata->tg_event))
			Tcl_DStringAppendElement(&tcl_cmd, "BEFORE");
		else if (TRIGGER_FIRED_AFTER(trigdata->tg_event))
			Tcl_DStringAppendElement(&tcl_cmd, "AFTER");
		else if (TRIGGER_FIRED_INSTEAD(trigdata->tg_event))
			Tcl_DStringAppendElement(&tcl_cmd, "INSTEAD OF");
		else
			elog(ERROR, "unrecognized WHEN tg_event: %u", trigdata->tg_event);

		/* The level part of the event for TG_level */
		if (TRIGGER_FIRED_FOR_ROW(trigdata->tg_event))
		{
			Tcl_DStringAppendElement(&tcl_cmd, "ROW");

			/* Build the data list for the trigtuple */
			pltcl_build_tuple_argument(trigdata->tg_trigtuple,
									   tupdesc, &tcl_trigtup);

			/*
			 * Now the command part of the event for TG_op and data for NEW
			 * and OLD
			 */
			if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
			{
				Tcl_DStringAppendElement(&tcl_cmd, "INSERT");

				Tcl_DStringAppendElement(&tcl_cmd, Tcl_DStringValue(&tcl_trigtup));
				Tcl_DStringAppendElement(&tcl_cmd, "");

				rettup = trigdata->tg_trigtuple;
			}
			else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
			{
				Tcl_DStringAppendElement(&tcl_cmd, "DELETE");

				Tcl_DStringAppendElement(&tcl_cmd, "");
				Tcl_DStringAppendElement(&tcl_cmd, Tcl_DStringValue(&tcl_trigtup));

				rettup = trigdata->tg_trigtuple;
			}
			else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
			{
				Tcl_DStringAppendElement(&tcl_cmd, "UPDATE");

				pltcl_build_tuple_argument(trigdata->tg_newtuple,
										   tupdesc, &tcl_newtup);

				Tcl_DStringAppendElement(&tcl_cmd, Tcl_DStringValue(&tcl_newtup));
				Tcl_DStringAppendElement(&tcl_cmd, Tcl_DStringValue(&tcl_trigtup));

				rettup = trigdata->tg_newtuple;
			}
			else
				elog(ERROR, "unrecognized OP tg_event: %u", trigdata->tg_event);
		}
		else if (TRIGGER_FIRED_FOR_STATEMENT(trigdata->tg_event))
		{
			Tcl_DStringAppendElement(&tcl_cmd, "STATEMENT");

			if (TRIGGER_FIRED_BY_INSERT(trigdata->tg_event))
				Tcl_DStringAppendElement(&tcl_cmd, "INSERT");
			else if (TRIGGER_FIRED_BY_DELETE(trigdata->tg_event))
				Tcl_DStringAppendElement(&tcl_cmd, "DELETE");
			else if (TRIGGER_FIRED_BY_UPDATE(trigdata->tg_event))
				Tcl_DStringAppendElement(&tcl_cmd, "UPDATE");
			else if (TRIGGER_FIRED_BY_TRUNCATE(trigdata->tg_event))
				Tcl_DStringAppendElement(&tcl_cmd, "TRUNCATE");
			else
				elog(ERROR, "unrecognized OP tg_event: %u", trigdata->tg_event);

			Tcl_DStringAppendElement(&tcl_cmd, "");
			Tcl_DStringAppendElement(&tcl_cmd, "");

			rettup = (HeapTuple) NULL;
		}
		else
			elog(ERROR, "unrecognized LEVEL tg_event: %u", trigdata->tg_event);

		/* Finally append the arguments from CREATE TRIGGER */
		for (i = 0; i < trigdata->tg_trigger->tgnargs; i++)
			Tcl_DStringAppendElement(&tcl_cmd, trigdata->tg_trigger->tgargs[i]);

	}
	PG_CATCH();
	{
		Tcl_DStringFree(&tcl_cmd);
		Tcl_DStringFree(&tcl_trigtup);
		Tcl_DStringFree(&tcl_newtup);
		PG_RE_THROW();
	}
	PG_END_TRY();
	Tcl_DStringFree(&tcl_trigtup);
	Tcl_DStringFree(&tcl_newtup);

	/************************************************************
	 * Call the Tcl function
	 *
	 * We assume no PG error can be thrown directly from this call.
	 ************************************************************/
	tcl_rc = Tcl_GlobalEval(interp, Tcl_DStringValue(&tcl_cmd));
	Tcl_DStringFree(&tcl_cmd);

	/************************************************************
	 * Check for errors reported by Tcl.
	 ************************************************************/
	if (tcl_rc != TCL_OK)
		throw_tcl_error(interp, prodesc->user_proname);

	/************************************************************
	 * The return value from the procedure might be one of
	 * the magic strings OK or SKIP or a list from array get.
	 * We can check for OK or SKIP without worrying about encoding.
	 ************************************************************/
	if (SPI_finish() != SPI_OK_FINISH)
		elog(ERROR, "SPI_finish() failed");

	result = Tcl_GetStringResult(interp);

	if (strcmp(result, "OK") == 0)
		return rettup;
	if (strcmp(result, "SKIP") == 0)
		return (HeapTuple) NULL;

	/************************************************************
	 * Convert the result value from the Tcl interpreter
	 * and setup structures for SPI_modifytuple();
	 ************************************************************/
	if (Tcl_SplitList(interp, result,
					  &ret_numvals, &ret_values) != TCL_OK)
	{
		UTF_BEGIN;
		elog(ERROR, "could not split return value from trigger: %s",
			 UTF_U2E(Tcl_GetStringResult(interp)));
		UTF_END;
	}

	/* Use a TRY to ensure ret_values will get freed */
	PG_TRY();
	{
		if (ret_numvals % 2 != 0)
			elog(ERROR, "invalid return list from trigger - must have even # of elements");

		modattrs = (int *) palloc(tupdesc->natts * sizeof(int));
		modvalues = (Datum *) palloc(tupdesc->natts * sizeof(Datum));
		for (i = 0; i < tupdesc->natts; i++)
		{
			modattrs[i] = i + 1;
			modvalues[i] = (Datum) NULL;
		}

		modnulls = palloc(tupdesc->natts);
		memset(modnulls, 'n', tupdesc->natts);

		for (i = 0; i < ret_numvals; i += 2)
		{
			CONST84 char *ret_name = ret_values[i];
			CONST84 char *ret_value = ret_values[i + 1];
			int			attnum;
			HeapTuple	typeTup;
			Oid			typinput;
			Oid			typioparam;
			FmgrInfo	finfo;

			/************************************************************
			 * Ignore ".tupno" pseudo elements (see pltcl_set_tuple_values)
			 ************************************************************/
			if (strcmp(ret_name, ".tupno") == 0)
				continue;

			/************************************************************
			 * Get the attribute number
			 ************************************************************/
			attnum = SPI_fnumber(tupdesc, ret_name);
			if (attnum == SPI_ERROR_NOATTRIBUTE)
				elog(ERROR, "invalid attribute \"%s\"", ret_name);
			if (attnum <= 0)
				elog(ERROR, "cannot set system attribute \"%s\"", ret_name);

			/************************************************************
			 * Ignore dropped columns
			 ************************************************************/
			if (tupdesc->attrs[attnum - 1]->attisdropped)
				continue;

			/************************************************************
			 * Lookup the attribute type in the syscache
			 * for the input function
			 ************************************************************/
			typeTup = SearchSysCache1(TYPEOID,
					 ObjectIdGetDatum(tupdesc->attrs[attnum - 1]->atttypid));
			if (!HeapTupleIsValid(typeTup))
				elog(ERROR, "cache lookup failed for type %u",
					 tupdesc->attrs[attnum - 1]->atttypid);
			typinput = ((Form_pg_type) GETSTRUCT(typeTup))->typinput;
			typioparam = getTypeIOParam(typeTup);
			ReleaseSysCache(typeTup);

			/************************************************************
			 * Set the attribute to NOT NULL and convert the contents
			 ************************************************************/
			modnulls[attnum - 1] = ' ';
			fmgr_info(typinput, &finfo);
			UTF_BEGIN;
			modvalues[attnum - 1] = InputFunctionCall(&finfo,
												 (char *) UTF_U2E(ret_value),
													  typioparam,
									  tupdesc->attrs[attnum - 1]->atttypmod);
			UTF_END;
		}

		rettup = SPI_modifytuple(trigdata->tg_relation, rettup, tupdesc->natts,
								 modattrs, modvalues, modnulls);

		pfree(modattrs);
		pfree(modvalues);
		pfree(modnulls);

		if (rettup == NULL)
			elog(ERROR, "SPI_modifytuple() failed - RC = %d", SPI_result);

	}
	PG_CATCH();
	{
		ckfree((char *) ret_values);
		PG_RE_THROW();
	}
	PG_END_TRY();
	ckfree((char *) ret_values);

	return rettup;
}


/**********************************************************************
 * throw_tcl_error	- ereport an error returned from the Tcl interpreter
 **********************************************************************/
static void
throw_tcl_error(Tcl_Interp *interp, const char *proname)
{
	/*
	 * Caution is needed here because Tcl_GetVar could overwrite the
	 * interpreter result (even though it's not really supposed to), and we
	 * can't control the order of evaluation of ereport arguments. Hence, make
	 * real sure we have our own copy of the result string before invoking
	 * Tcl_GetVar.
	 */
	char	   *emsg;
	char	   *econtext;

	UTF_BEGIN;
	emsg = pstrdup(UTF_U2E(Tcl_GetStringResult(interp)));
	UTF_END;
	UTF_BEGIN;
	econtext = UTF_U2E((char *) Tcl_GetVar(interp, "errorInfo",
										   TCL_GLOBAL_ONLY));
	ereport(ERROR,
			(errmsg("%s", emsg),
			 errcontext("%s\nin PL/Tcl function \"%s\"",
						econtext, proname)));
	UTF_END;
}


/**********************************************************************
 * compile_pltcl_function	- compile (or hopefully just look up) function
 *
 * tgreloid is the OID of the relation when compiling a trigger, or zero
 * (InvalidOid) when compiling a plain function.
 **********************************************************************/
static pltcl_proc_desc *
compile_pltcl_function(Oid fn_oid, Oid tgreloid, bool pltrusted)
{
	HeapTuple	procTup;
	Form_pg_proc procStruct;
	pltcl_proc_key proc_key;
	pltcl_proc_ptr *proc_ptr;
	bool		found;
	pltcl_proc_desc *prodesc;

	/* We'll need the pg_proc tuple in any case... */
	procTup = SearchSysCache1(PROCOID, ObjectIdGetDatum(fn_oid));
	if (!HeapTupleIsValid(procTup))
		elog(ERROR, "cache lookup failed for function %u", fn_oid);
	procStruct = (Form_pg_proc) GETSTRUCT(procTup);

	/* Try to find function in pltcl_proc_htab */
	proc_key.proc_id = fn_oid;
	proc_key.is_trigger = OidIsValid(tgreloid);
	proc_key.user_id = pltrusted ? GetUserId() : InvalidOid;

	proc_ptr = hash_search(pltcl_proc_htab, &proc_key,
						   HASH_ENTER,
						   &found);
	if (!found)
		proc_ptr->proc_ptr = NULL;

	prodesc = proc_ptr->proc_ptr;

	/************************************************************
	 * If it's present, must check whether it's still up to date.
	 * This is needed because CREATE OR REPLACE FUNCTION can modify the
	 * function's pg_proc entry without changing its OID.
	 ************************************************************/
	if (prodesc != NULL)
	{
		bool		uptodate;

		uptodate = (prodesc->fn_xmin == HeapTupleHeaderGetXmin(procTup->t_data) &&
					ItemPointerEquals(&prodesc->fn_tid, &procTup->t_self));

		if (!uptodate)
		{
			proc_ptr->proc_ptr = NULL;
			prodesc = NULL;
		}
	}

	/************************************************************
	 * If we haven't found it in the hashtable, we analyze
	 * the functions arguments and returntype and store
	 * the in-/out-functions in the prodesc block and create
	 * a new hashtable entry for it.
	 *
	 * Then we load the procedure into the Tcl interpreter.
	 ************************************************************/
	if (prodesc == NULL)
	{
		bool		is_trigger = OidIsValid(tgreloid);
		char		internal_proname[128];
		HeapTuple	typeTup;
		Form_pg_type typeStruct;
		Tcl_DString proc_internal_def;
		Tcl_DString proc_internal_body;
		char		proc_internal_args[33 * FUNC_MAX_ARGS];
		Datum		prosrcdatum;
		bool		isnull;
		char	   *proc_source;
		char		buf[32];
		Tcl_Interp *interp;
		int			i;
		int			tcl_rc;

		/************************************************************
		 * Build our internal proc name from the function's Oid.  Append
		 * "_trigger" when appropriate to ensure the normal and trigger
		 * cases are kept separate.
		 ************************************************************/
		if (!is_trigger)
			snprintf(internal_proname, sizeof(internal_proname),
					 "__PLTcl_proc_%u", fn_oid);
		else
			snprintf(internal_proname, sizeof(internal_proname),
					 "__PLTcl_proc_%u_trigger", fn_oid);

		/************************************************************
		 * Allocate a new procedure description block
		 ************************************************************/
		prodesc = (pltcl_proc_desc *) malloc(sizeof(pltcl_proc_desc));
		if (prodesc == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		MemSet(prodesc, 0, sizeof(pltcl_proc_desc));
		prodesc->user_proname = strdup(NameStr(procStruct->proname));
		prodesc->internal_proname = strdup(internal_proname);
		if (prodesc->user_proname == NULL || prodesc->internal_proname == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_OUT_OF_MEMORY),
					 errmsg("out of memory")));
		prodesc->fn_xmin = HeapTupleHeaderGetXmin(procTup->t_data);
		prodesc->fn_tid = procTup->t_self;

		/* Remember if function is STABLE/IMMUTABLE */
		prodesc->fn_readonly =
			(procStruct->provolatile != PROVOLATILE_VOLATILE);
		/* And whether it is trusted */
		prodesc->lanpltrusted = pltrusted;

		/************************************************************
		 * Identify the interpreter to use for the function
		 ************************************************************/
		prodesc->interp_desc = pltcl_fetch_interp(prodesc->lanpltrusted);
		interp = prodesc->interp_desc->interp;

		/************************************************************
		 * Get the required information for input conversion of the
		 * return value.
		 ************************************************************/
		if (!is_trigger)
		{
			typeTup =
				SearchSysCache1(TYPEOID,
								ObjectIdGetDatum(procStruct->prorettype));
			if (!HeapTupleIsValid(typeTup))
			{
				free(prodesc->user_proname);
				free(prodesc->internal_proname);
				free(prodesc);
				elog(ERROR, "cache lookup failed for type %u",
					 procStruct->prorettype);
			}
			typeStruct = (Form_pg_type) GETSTRUCT(typeTup);

			/* Disallow pseudotype result, except VOID */
			if (typeStruct->typtype == TYPTYPE_PSEUDO)
			{
				if (procStruct->prorettype == VOIDOID)
					 /* okay */ ;
				else if (procStruct->prorettype == TRIGGEROID)
				{
					free(prodesc->user_proname);
					free(prodesc->internal_proname);
					free(prodesc);
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("trigger functions can only be called as triggers")));
				}
				else
				{
					free(prodesc->user_proname);
					free(prodesc->internal_proname);
					free(prodesc);
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("PL/Tcl functions cannot return type %s",
									format_type_be(procStruct->prorettype))));
				}
			}

			if (typeStruct->typtype == TYPTYPE_COMPOSITE)
			{
				free(prodesc->user_proname);
				free(prodesc->internal_proname);
				free(prodesc);
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				  errmsg("PL/Tcl functions cannot return composite types")));
			}

			perm_fmgr_info(typeStruct->typinput, &(prodesc->result_in_func));
			prodesc->result_typioparam = getTypeIOParam(typeTup);

			ReleaseSysCache(typeTup);
		}

		/************************************************************
		 * Get the required information for output conversion
		 * of all procedure arguments
		 ************************************************************/
		if (!is_trigger)
		{
			prodesc->nargs = procStruct->pronargs;
			proc_internal_args[0] = '\0';
			for (i = 0; i < prodesc->nargs; i++)
			{
				typeTup = SearchSysCache1(TYPEOID,
						ObjectIdGetDatum(procStruct->proargtypes.values[i]));
				if (!HeapTupleIsValid(typeTup))
				{
					free(prodesc->user_proname);
					free(prodesc->internal_proname);
					free(prodesc);
					elog(ERROR, "cache lookup failed for type %u",
						 procStruct->proargtypes.values[i]);
				}
				typeStruct = (Form_pg_type) GETSTRUCT(typeTup);

				/* Disallow pseudotype argument */
				if (typeStruct->typtype == TYPTYPE_PSEUDO)
				{
					free(prodesc->user_proname);
					free(prodesc->internal_proname);
					free(prodesc);
					ereport(ERROR,
							(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
							 errmsg("PL/Tcl functions cannot accept type %s",
						format_type_be(procStruct->proargtypes.values[i]))));
				}

				if (typeStruct->typtype == TYPTYPE_COMPOSITE)
				{
					prodesc->arg_is_rowtype[i] = true;
					snprintf(buf, sizeof(buf), "__PLTcl_Tup_%d", i + 1);
				}
				else
				{
					prodesc->arg_is_rowtype[i] = false;
					perm_fmgr_info(typeStruct->typoutput,
								   &(prodesc->arg_out_func[i]));
					snprintf(buf, sizeof(buf), "%d", i + 1);
				}

				if (i > 0)
					strcat(proc_internal_args, " ");
				strcat(proc_internal_args, buf);

				ReleaseSysCache(typeTup);
			}
		}
		else
		{
			/* trigger procedure has fixed args */
			strcpy(proc_internal_args,
				   "TG_name TG_relid TG_table_name TG_table_schema TG_relatts TG_when TG_level TG_op __PLTcl_Tup_NEW __PLTcl_Tup_OLD args");
		}

		/************************************************************
		 * Create the tcl command to define the internal
		 * procedure
		 ************************************************************/
		Tcl_DStringInit(&proc_internal_def);
		Tcl_DStringInit(&proc_internal_body);
		Tcl_DStringAppendElement(&proc_internal_def, "proc");
		Tcl_DStringAppendElement(&proc_internal_def, internal_proname);
		Tcl_DStringAppendElement(&proc_internal_def, proc_internal_args);

		/************************************************************
		 * prefix procedure body with
		 * upvar #0 <internal_procname> GD
		 * and with appropriate setting of arguments
		 ************************************************************/
		Tcl_DStringAppend(&proc_internal_body, "upvar #0 ", -1);
		Tcl_DStringAppend(&proc_internal_body, internal_proname, -1);
		Tcl_DStringAppend(&proc_internal_body, " GD\n", -1);
		if (!is_trigger)
		{
			for (i = 0; i < prodesc->nargs; i++)
			{
				if (prodesc->arg_is_rowtype[i])
				{
					snprintf(buf, sizeof(buf),
							 "array set %d $__PLTcl_Tup_%d\n",
							 i + 1, i + 1);
					Tcl_DStringAppend(&proc_internal_body, buf, -1);
				}
			}
		}
		else
		{
			Tcl_DStringAppend(&proc_internal_body,
							  "array set NEW $__PLTcl_Tup_NEW\n", -1);
			Tcl_DStringAppend(&proc_internal_body,
							  "array set OLD $__PLTcl_Tup_OLD\n", -1);

			Tcl_DStringAppend(&proc_internal_body,
							  "set i 0\n"
							  "set v 0\n"
							  "foreach v $args {\n"
							  "  incr i\n"
							  "  set $i $v\n"
							  "}\n"
							  "unset i v\n\n", -1);
		}

		/************************************************************
		 * Add user's function definition to proc body
		 ************************************************************/
		prosrcdatum = SysCacheGetAttr(PROCOID, procTup,
									  Anum_pg_proc_prosrc, &isnull);
		if (isnull)
			elog(ERROR, "null prosrc");
		proc_source = TextDatumGetCString(prosrcdatum);
		UTF_BEGIN;
		Tcl_DStringAppend(&proc_internal_body, UTF_E2U(proc_source), -1);
		UTF_END;
		pfree(proc_source);
		Tcl_DStringAppendElement(&proc_internal_def,
								 Tcl_DStringValue(&proc_internal_body));
		Tcl_DStringFree(&proc_internal_body);

		/************************************************************
		 * Create the procedure in the interpreter
		 ************************************************************/
		tcl_rc = Tcl_GlobalEval(interp,
								Tcl_DStringValue(&proc_internal_def));
		Tcl_DStringFree(&proc_internal_def);
		if (tcl_rc != TCL_OK)
		{
			free(prodesc->user_proname);
			free(prodesc->internal_proname);
			free(prodesc);
			UTF_BEGIN;
			elog(ERROR, "could not create internal procedure \"%s\": %s",
				 internal_proname, UTF_U2E(Tcl_GetStringResult(interp)));
			UTF_END;
		}

		/************************************************************
		 * Add the proc description block to the hashtable.  Note we do not
		 * attempt to free any previously existing prodesc block.  This is
		 * annoying, but necessary since there could be active calls using
		 * the old prodesc.
		 ************************************************************/
		proc_ptr->proc_ptr = prodesc;
	}

	ReleaseSysCache(procTup);

	return prodesc;
}


/**********************************************************************
 * pltcl_elog()		- elog() support for PLTcl
 **********************************************************************/
static int
pltcl_elog(ClientData cdata, Tcl_Interp *interp,
		   int argc, CONST84 char *argv[])
{
	volatile int level;
	MemoryContext oldcontext;

	if (argc != 3)
	{
		Tcl_SetResult(interp, "syntax error - 'elog level msg'", TCL_STATIC);
		return TCL_ERROR;
	}

	if (strcmp(argv[1], "DEBUG") == 0)
		level = DEBUG2;
	else if (strcmp(argv[1], "LOG") == 0)
		level = LOG;
	else if (strcmp(argv[1], "INFO") == 0)
		level = INFO;
	else if (strcmp(argv[1], "NOTICE") == 0)
		level = NOTICE;
	else if (strcmp(argv[1], "WARNING") == 0)
		level = WARNING;
	else if (strcmp(argv[1], "ERROR") == 0)
		level = ERROR;
	else if (strcmp(argv[1], "FATAL") == 0)
		level = FATAL;
	else
	{
		Tcl_AppendResult(interp, "Unknown elog level '", argv[1],
						 "'", NULL);
		return TCL_ERROR;
	}

	if (level == ERROR)
	{
		/*
		 * We just pass the error back to Tcl.	If it's not caught, it'll
		 * eventually get converted to a PG error when we reach the call
		 * handler.
		 */
		Tcl_SetResult(interp, (char *) argv[2], TCL_VOLATILE);
		return TCL_ERROR;
	}

	/*
	 * For non-error messages, just pass 'em to elog().  We do not expect that
	 * this will fail, but just on the off chance it does, report the error
	 * back to Tcl.  Note we are assuming that elog() can't have any internal
	 * failures that are so bad as to require a transaction abort.
	 *
	 * This path is also used for FATAL errors, which aren't going to come
	 * back to us at all.
	 */
	oldcontext = CurrentMemoryContext;
	PG_TRY();
	{
		UTF_BEGIN;
		elog(level, "%s", UTF_U2E(argv[2]));
		UTF_END;
	}
	PG_CATCH();
	{
		ErrorData  *edata;

		/* Must reset elog.c's state */
		MemoryContextSwitchTo(oldcontext);
		edata = CopyErrorData();
		FlushErrorState();

		/* Pass the error message to Tcl */
		UTF_BEGIN;
		Tcl_SetResult(interp, UTF_E2U(edata->message), TCL_VOLATILE);
		UTF_END;
		FreeErrorData(edata);

		return TCL_ERROR;
	}
	PG_END_TRY();

	return TCL_OK;
}


/**********************************************************************
 * pltcl_quote()	- quote literal strings that are to
 *			  be used in SPI_execute query strings
 **********************************************************************/
static int
pltcl_quote(ClientData cdata, Tcl_Interp *interp,
			int argc, CONST84 char *argv[])
{
	char	   *tmp;
	const char *cp1;
	char	   *cp2;

	/************************************************************
	 * Check call syntax
	 ************************************************************/
	if (argc != 2)
	{
		Tcl_SetResult(interp, "syntax error - 'quote string'", TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Allocate space for the maximum the string can
	 * grow to and initialize pointers
	 ************************************************************/
	tmp = palloc(strlen(argv[1]) * 2 + 1);
	cp1 = argv[1];
	cp2 = tmp;

	/************************************************************
	 * Walk through string and double every quote and backslash
	 ************************************************************/
	while (*cp1)
	{
		if (*cp1 == '\'')
			*cp2++ = '\'';
		else
		{
			if (*cp1 == '\\')
				*cp2++ = '\\';
		}
		*cp2++ = *cp1++;
	}

	/************************************************************
	 * Terminate the string and set it as result
	 ************************************************************/
	*cp2 = '\0';
	Tcl_SetResult(interp, tmp, TCL_VOLATILE);
	pfree(tmp);
	return TCL_OK;
}


/**********************************************************************
 * pltcl_argisnull()	- determine if a specific argument is NULL
 **********************************************************************/
static int
pltcl_argisnull(ClientData cdata, Tcl_Interp *interp,
				int argc, CONST84 char *argv[])
{
	int			argno;
	FunctionCallInfo fcinfo = pltcl_current_fcinfo;

	/************************************************************
	 * Check call syntax
	 ************************************************************/
	if (argc != 2)
	{
		Tcl_SetResult(interp, "syntax error - 'argisnull argno'",
					  TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Check that we're called as a normal function
	 ************************************************************/
	if (fcinfo == NULL)
	{
		Tcl_SetResult(interp, "argisnull cannot be used in triggers",
					  TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Get the argument number
	 ************************************************************/
	if (Tcl_GetInt(interp, argv[1], &argno) != TCL_OK)
		return TCL_ERROR;

	/************************************************************
	 * Check that the argno is valid
	 ************************************************************/
	argno--;
	if (argno < 0 || argno >= fcinfo->nargs)
	{
		Tcl_SetResult(interp, "argno out of range", TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Get the requested NULL state
	 ************************************************************/
	if (PG_ARGISNULL(argno))
		Tcl_SetResult(interp, "1", TCL_STATIC);
	else
		Tcl_SetResult(interp, "0", TCL_STATIC);

	return TCL_OK;
}


/**********************************************************************
 * pltcl_returnnull()	- Cause a NULL return from a function
 **********************************************************************/
static int
pltcl_returnnull(ClientData cdata, Tcl_Interp *interp,
				 int argc, CONST84 char *argv[])
{
	FunctionCallInfo fcinfo = pltcl_current_fcinfo;

	/************************************************************
	 * Check call syntax
	 ************************************************************/
	if (argc != 1)
	{
		Tcl_SetResult(interp, "syntax error - 'return_null'", TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Check that we're called as a normal function
	 ************************************************************/
	if (fcinfo == NULL)
	{
		Tcl_SetResult(interp, "return_null cannot be used in triggers",
					  TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Set the NULL return flag and cause Tcl to return from the
	 * procedure.
	 ************************************************************/
	fcinfo->isnull = true;

	return TCL_RETURN;
}


/*----------
 * Support for running SPI operations inside subtransactions
 *
 * Intended usage pattern is:
 *
 *	MemoryContext oldcontext = CurrentMemoryContext;
 *	ResourceOwner oldowner = CurrentResourceOwner;
 *
 *	...
 *	pltcl_subtrans_begin(oldcontext, oldowner);
 *	PG_TRY();
 *	{
 *		do something risky;
 *		pltcl_subtrans_commit(oldcontext, oldowner);
 *	}
 *	PG_CATCH();
 *	{
 *		pltcl_subtrans_abort(interp, oldcontext, oldowner);
 *		return TCL_ERROR;
 *	}
 *	PG_END_TRY();
 *	return TCL_OK;
 *----------
 */
static void
pltcl_subtrans_begin(MemoryContext oldcontext, ResourceOwner oldowner)
{
	BeginInternalSubTransaction(NULL);

	/* Want to run inside function's memory context */
	MemoryContextSwitchTo(oldcontext);
}

static void
pltcl_subtrans_commit(MemoryContext oldcontext, ResourceOwner oldowner)
{
	/* Commit the inner transaction, return to outer xact context */
	ReleaseCurrentSubTransaction();
	MemoryContextSwitchTo(oldcontext);
	CurrentResourceOwner = oldowner;

	/*
	 * AtEOSubXact_SPI() should not have popped any SPI context, but just in
	 * case it did, make sure we remain connected.
	 */
	SPI_restore_connection();
}

static void
pltcl_subtrans_abort(Tcl_Interp *interp,
					 MemoryContext oldcontext, ResourceOwner oldowner)
{
	ErrorData  *edata;

	/* Save error info */
	MemoryContextSwitchTo(oldcontext);
	edata = CopyErrorData();
	FlushErrorState();

	/* Abort the inner transaction */
	RollbackAndReleaseCurrentSubTransaction();
	MemoryContextSwitchTo(oldcontext);
	CurrentResourceOwner = oldowner;

	/*
	 * If AtEOSubXact_SPI() popped any SPI context of the subxact, it will
	 * have left us in a disconnected state.  We need this hack to return to
	 * connected state.
	 */
	SPI_restore_connection();

	/* Pass the error message to Tcl */
	UTF_BEGIN;
	Tcl_SetResult(interp, UTF_E2U(edata->message), TCL_VOLATILE);
	UTF_END;
	FreeErrorData(edata);
}


/**********************************************************************
 * pltcl_SPI_execute()		- The builtin SPI_execute command
 *				  for the Tcl interpreter
 **********************************************************************/
static int
pltcl_SPI_execute(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[])
{
	int			my_rc;
	int			spi_rc;
	int			query_idx;
	int			i;
	int			count = 0;
	CONST84 char *volatile arrayname = NULL;
	CONST84 char *volatile loop_body = NULL;
	MemoryContext oldcontext = CurrentMemoryContext;
	ResourceOwner oldowner = CurrentResourceOwner;

	char	   *usage = "syntax error - 'SPI_exec "
	"?-count n? "
	"?-array name? query ?loop body?";

	/************************************************************
	 * Check the call syntax and get the options
	 ************************************************************/
	if (argc < 2)
	{
		Tcl_SetResult(interp, usage, TCL_STATIC);
		return TCL_ERROR;
	}

	i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "-array") == 0)
		{
			if (++i >= argc)
			{
				Tcl_SetResult(interp, usage, TCL_STATIC);
				return TCL_ERROR;
			}
			arrayname = argv[i++];
			continue;
		}

		if (strcmp(argv[i], "-count") == 0)
		{
			if (++i >= argc)
			{
				Tcl_SetResult(interp, usage, TCL_STATIC);
				return TCL_ERROR;
			}
			if (Tcl_GetInt(interp, argv[i++], &count) != TCL_OK)
				return TCL_ERROR;
			continue;
		}

		break;
	}

	query_idx = i;
	if (query_idx >= argc || query_idx + 2 < argc)
	{
		Tcl_SetResult(interp, usage, TCL_STATIC);
		return TCL_ERROR;
	}
	if (query_idx + 1 < argc)
		loop_body = argv[query_idx + 1];

	/************************************************************
	 * Execute the query inside a sub-transaction, so we can cope with
	 * errors sanely
	 ************************************************************/

	pltcl_subtrans_begin(oldcontext, oldowner);

	PG_TRY();
	{
		UTF_BEGIN;
		spi_rc = SPI_execute(UTF_U2E(argv[query_idx]),
							 pltcl_current_prodesc->fn_readonly, count);
		UTF_END;

		my_rc = pltcl_process_SPI_result(interp,
										 arrayname,
										 loop_body,
										 spi_rc,
										 SPI_tuptable,
										 SPI_processed);

		pltcl_subtrans_commit(oldcontext, oldowner);
	}
	PG_CATCH();
	{
		pltcl_subtrans_abort(interp, oldcontext, oldowner);
		return TCL_ERROR;
	}
	PG_END_TRY();

	return my_rc;
}

/*
 * Process the result from SPI_execute or SPI_execute_plan
 *
 * Shared code between pltcl_SPI_execute and pltcl_SPI_execute_plan
 */
static int
pltcl_process_SPI_result(Tcl_Interp *interp,
						 CONST84 char *arrayname,
						 CONST84 char *loop_body,
						 int spi_rc,
						 SPITupleTable *tuptable,
						 int ntuples)
{
	int			my_rc = TCL_OK;
	char		buf[64];
	int			i;
	int			loop_rc;
	HeapTuple  *tuples;
	TupleDesc	tupdesc;

	switch (spi_rc)
	{
		case SPI_OK_SELINTO:
		case SPI_OK_INSERT:
		case SPI_OK_DELETE:
		case SPI_OK_UPDATE:
			snprintf(buf, sizeof(buf), "%d", ntuples);
			Tcl_SetResult(interp, buf, TCL_VOLATILE);
			break;

		case SPI_OK_UTILITY:
		case SPI_OK_REWRITTEN:
			if (tuptable == NULL)
			{
				Tcl_SetResult(interp, "0", TCL_STATIC);
				break;
			}
			/* FALL THRU for utility returning tuples */

		case SPI_OK_SELECT:
		case SPI_OK_INSERT_RETURNING:
		case SPI_OK_DELETE_RETURNING:
		case SPI_OK_UPDATE_RETURNING:

			/*
			 * Process the tuples we got
			 */
			tuples = tuptable->vals;
			tupdesc = tuptable->tupdesc;

			if (loop_body == NULL)
			{
				/*
				 * If there is no loop body given, just set the variables from
				 * the first tuple (if any)
				 */
				if (ntuples > 0)
					pltcl_set_tuple_values(interp, arrayname, 0,
										   tuples[0], tupdesc);
			}
			else
			{
				/*
				 * There is a loop body - process all tuples and evaluate the
				 * body on each
				 */
				for (i = 0; i < ntuples; i++)
				{
					pltcl_set_tuple_values(interp, arrayname, i,
										   tuples[i], tupdesc);

					loop_rc = Tcl_Eval(interp, loop_body);

					if (loop_rc == TCL_OK)
						continue;
					if (loop_rc == TCL_CONTINUE)
						continue;
					if (loop_rc == TCL_RETURN)
					{
						my_rc = TCL_RETURN;
						break;
					}
					if (loop_rc == TCL_BREAK)
						break;
					my_rc = TCL_ERROR;
					break;
				}
			}

			if (my_rc == TCL_OK)
			{
				snprintf(buf, sizeof(buf), "%d", ntuples);
				Tcl_SetResult(interp, buf, TCL_VOLATILE);
			}
			break;

		default:
			Tcl_AppendResult(interp, "pltcl: SPI_execute failed: ",
							 SPI_result_code_string(spi_rc), NULL);
			my_rc = TCL_ERROR;
			break;
	}

	SPI_freetuptable(tuptable);

	return my_rc;
}


/**********************************************************************
 * pltcl_SPI_prepare()		- Builtin support for prepared plans
 *				  The Tcl command SPI_prepare
 *				  always saves the plan using
 *				  SPI_saveplan and returns a key for
 *				  access. There is no chance to prepare
 *				  and not save the plan currently.
 **********************************************************************/
static int
pltcl_SPI_prepare(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[])
{
	int			nargs;
	CONST84 char **args;
	pltcl_query_desc *qdesc;
	void	   *plan;
	int			i;
	Tcl_HashEntry *hashent;
	int			hashnew;
	Tcl_HashTable *query_hash;
	MemoryContext oldcontext = CurrentMemoryContext;
	ResourceOwner oldowner = CurrentResourceOwner;

	/************************************************************
	 * Check the call syntax
	 ************************************************************/
	if (argc != 3)
	{
		Tcl_SetResult(interp, "syntax error - 'SPI_prepare query argtypes'",
					  TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Split the argument type list
	 ************************************************************/
	if (Tcl_SplitList(interp, argv[2], &nargs, &args) != TCL_OK)
		return TCL_ERROR;

	/************************************************************
	 * Allocate the new querydesc structure
	 ************************************************************/
	qdesc = (pltcl_query_desc *) malloc(sizeof(pltcl_query_desc));
	snprintf(qdesc->qname, sizeof(qdesc->qname), "%p", qdesc);
	qdesc->nargs = nargs;
	qdesc->argtypes = (Oid *) malloc(nargs * sizeof(Oid));
	qdesc->arginfuncs = (FmgrInfo *) malloc(nargs * sizeof(FmgrInfo));
	qdesc->argtypioparams = (Oid *) malloc(nargs * sizeof(Oid));

	/************************************************************
	 * Execute the prepare inside a sub-transaction, so we can cope with
	 * errors sanely
	 ************************************************************/

	pltcl_subtrans_begin(oldcontext, oldowner);

	PG_TRY();
	{
		/************************************************************
		 * Resolve argument type names and then look them up by oid
		 * in the system cache, and remember the required information
		 * for input conversion.
		 ************************************************************/
		for (i = 0; i < nargs; i++)
		{
			Oid			typId,
						typInput,
						typIOParam;
			int32		typmod;

			parseTypeString(args[i], &typId, &typmod);

			getTypeInputInfo(typId, &typInput, &typIOParam);

			qdesc->argtypes[i] = typId;
			perm_fmgr_info(typInput, &(qdesc->arginfuncs[i]));
			qdesc->argtypioparams[i] = typIOParam;
		}

		/************************************************************
		 * Prepare the plan and check for errors
		 ************************************************************/
		UTF_BEGIN;
		plan = SPI_prepare(UTF_U2E(argv[1]), nargs, qdesc->argtypes);
		UTF_END;

		if (plan == NULL)
			elog(ERROR, "SPI_prepare() failed");

		/************************************************************
		 * Save the plan into permanent memory (right now it's in the
		 * SPI procCxt, which will go away at function end).
		 ************************************************************/
		qdesc->plan = SPI_saveplan(plan);
		if (qdesc->plan == NULL)
			elog(ERROR, "SPI_saveplan() failed");

		/* Release the procCxt copy to avoid within-function memory leak */
		SPI_freeplan(plan);

		pltcl_subtrans_commit(oldcontext, oldowner);
	}
	PG_CATCH();
	{
		pltcl_subtrans_abort(interp, oldcontext, oldowner);

		free(qdesc->argtypes);
		free(qdesc->arginfuncs);
		free(qdesc->argtypioparams);
		free(qdesc);
		ckfree((char *) args);

		return TCL_ERROR;
	}
	PG_END_TRY();

	/************************************************************
	 * Insert a hashtable entry for the plan and return
	 * the key to the caller
	 ************************************************************/
	query_hash = &pltcl_current_prodesc->interp_desc->query_hash;

	hashent = Tcl_CreateHashEntry(query_hash, qdesc->qname, &hashnew);
	Tcl_SetHashValue(hashent, (ClientData) qdesc);

	ckfree((char *) args);

	/* qname is ASCII, so no need for encoding conversion */
	Tcl_SetResult(interp, qdesc->qname, TCL_VOLATILE);
	return TCL_OK;
}


/**********************************************************************
 * pltcl_SPI_execute_plan()		- Execute a prepared plan
 **********************************************************************/
static int
pltcl_SPI_execute_plan(ClientData cdata, Tcl_Interp *interp,
					   int argc, CONST84 char *argv[])
{
	int			my_rc;
	int			spi_rc;
	int			i;
	int			j;
	Tcl_HashEntry *hashent;
	pltcl_query_desc *qdesc;
	const char *volatile nulls = NULL;
	CONST84 char *volatile arrayname = NULL;
	CONST84 char *volatile loop_body = NULL;
	int			count = 0;
	int			callnargs;
	CONST84 char **callargs = NULL;
	Datum	   *argvalues;
	MemoryContext oldcontext = CurrentMemoryContext;
	ResourceOwner oldowner = CurrentResourceOwner;
	Tcl_HashTable *query_hash;

	char	   *usage = "syntax error - 'SPI_execp "
	"?-nulls string? ?-count n? "
	"?-array name? query ?args? ?loop body?";

	/************************************************************
	 * Get the options and check syntax
	 ************************************************************/
	i = 1;
	while (i < argc)
	{
		if (strcmp(argv[i], "-array") == 0)
		{
			if (++i >= argc)
			{
				Tcl_SetResult(interp, usage, TCL_STATIC);
				return TCL_ERROR;
			}
			arrayname = argv[i++];
			continue;
		}
		if (strcmp(argv[i], "-nulls") == 0)
		{
			if (++i >= argc)
			{
				Tcl_SetResult(interp, usage, TCL_STATIC);
				return TCL_ERROR;
			}
			nulls = argv[i++];
			continue;
		}
		if (strcmp(argv[i], "-count") == 0)
		{
			if (++i >= argc)
			{
				Tcl_SetResult(interp, usage, TCL_STATIC);
				return TCL_ERROR;
			}
			if (Tcl_GetInt(interp, argv[i++], &count) != TCL_OK)
				return TCL_ERROR;
			continue;
		}

		break;
	}

	/************************************************************
	 * Get the prepared plan descriptor by its key
	 ************************************************************/
	if (i >= argc)
	{
		Tcl_SetResult(interp, usage, TCL_STATIC);
		return TCL_ERROR;
	}

	query_hash = &pltcl_current_prodesc->interp_desc->query_hash;

	hashent = Tcl_FindHashEntry(query_hash, argv[i]);
	if (hashent == NULL)
	{
		Tcl_AppendResult(interp, "invalid queryid '", argv[i], "'", NULL);
		return TCL_ERROR;
	}
	qdesc = (pltcl_query_desc *) Tcl_GetHashValue(hashent);
	i++;

	/************************************************************
	 * If a nulls string is given, check for correct length
	 ************************************************************/
	if (nulls != NULL)
	{
		if (strlen(nulls) != qdesc->nargs)
		{
			Tcl_SetResult(interp,
					   "length of nulls string doesn't match # of arguments",
						  TCL_STATIC);
			return TCL_ERROR;
		}
	}

	/************************************************************
	 * If there was a argtype list on preparation, we need
	 * an argument value list now
	 ************************************************************/
	if (qdesc->nargs > 0)
	{
		if (i >= argc)
		{
			Tcl_SetResult(interp, "missing argument list", TCL_STATIC);
			return TCL_ERROR;
		}

		/************************************************************
		 * Split the argument values
		 ************************************************************/
		if (Tcl_SplitList(interp, argv[i++], &callnargs, &callargs) != TCL_OK)
			return TCL_ERROR;

		/************************************************************
		 * Check that the # of arguments matches
		 ************************************************************/
		if (callnargs != qdesc->nargs)
		{
			Tcl_SetResult(interp,
			   "argument list length doesn't match # of arguments for query",
						  TCL_STATIC);
			ckfree((char *) callargs);
			return TCL_ERROR;
		}
	}
	else
		callnargs = 0;

	/************************************************************
	 * Get loop body if present
	 ************************************************************/
	if (i < argc)
		loop_body = argv[i++];

	if (i != argc)
	{
		Tcl_SetResult(interp, usage, TCL_STATIC);
		return TCL_ERROR;
	}

	/************************************************************
	 * Execute the plan inside a sub-transaction, so we can cope with
	 * errors sanely
	 ************************************************************/

	pltcl_subtrans_begin(oldcontext, oldowner);

	PG_TRY();
	{
		/************************************************************
		 * Setup the value array for SPI_execute_plan() using
		 * the type specific input functions
		 ************************************************************/
		argvalues = (Datum *) palloc(callnargs * sizeof(Datum));

		for (j = 0; j < callnargs; j++)
		{
			if (nulls && nulls[j] == 'n')
			{
				argvalues[j] = InputFunctionCall(&qdesc->arginfuncs[j],
												 NULL,
												 qdesc->argtypioparams[j],
												 -1);
			}
			else
			{
				UTF_BEGIN;
				argvalues[j] = InputFunctionCall(&qdesc->arginfuncs[j],
											   (char *) UTF_U2E(callargs[j]),
												 qdesc->argtypioparams[j],
												 -1);
				UTF_END;
			}
		}

		if (callargs)
			ckfree((char *) callargs);
		callargs = NULL;

		/************************************************************
		 * Execute the plan
		 ************************************************************/
		spi_rc = SPI_execute_plan(qdesc->plan, argvalues, nulls,
								  pltcl_current_prodesc->fn_readonly, count);

		my_rc = pltcl_process_SPI_result(interp,
										 arrayname,
										 loop_body,
										 spi_rc,
										 SPI_tuptable,
										 SPI_processed);

		pltcl_subtrans_commit(oldcontext, oldowner);
	}
	PG_CATCH();
	{
		pltcl_subtrans_abort(interp, oldcontext, oldowner);

		if (callargs)
			ckfree((char *) callargs);

		return TCL_ERROR;
	}
	PG_END_TRY();

	return my_rc;
}


/**********************************************************************
 * pltcl_SPI_lastoid()	- return the last oid. To
 *		  be used after insert queries
 **********************************************************************/
static int
pltcl_SPI_lastoid(ClientData cdata, Tcl_Interp *interp,
				  int argc, CONST84 char *argv[])
{
	char		buf[64];

	snprintf(buf, sizeof(buf), "%u", SPI_lastoid);
	Tcl_SetResult(interp, buf, TCL_VOLATILE);
	return TCL_OK;
}


/**********************************************************************
 * pltcl_set_tuple_values() - Set variables for all attributes
 *				  of a given tuple
 **********************************************************************/
static void
pltcl_set_tuple_values(Tcl_Interp *interp, CONST84 char *arrayname,
					   int tupno, HeapTuple tuple, TupleDesc tupdesc)
{
	int			i;
	char	   *outputstr;
	char		buf[64];
	Datum		attr;
	bool		isnull;

	CONST84 char *attname;
	HeapTuple	typeTup;
	Oid			typoutput;

	CONST84 char **arrptr;
	CONST84 char **nameptr;
	CONST84 char *nullname = NULL;

	/************************************************************
	 * Prepare pointers for Tcl_SetVar2() below and in array
	 * mode set the .tupno element
	 ************************************************************/
	if (arrayname == NULL)
	{
		arrptr = &attname;
		nameptr = &nullname;
	}
	else
	{
		arrptr = &arrayname;
		nameptr = &attname;
		snprintf(buf, sizeof(buf), "%d", tupno);
		Tcl_SetVar2(interp, arrayname, ".tupno", buf, 0);
	}

	for (i = 0; i < tupdesc->natts; i++)
	{
		/* ignore dropped attributes */
		if (tupdesc->attrs[i]->attisdropped)
			continue;

		/************************************************************
		 * Get the attribute name
		 ************************************************************/
		attname = NameStr(tupdesc->attrs[i]->attname);

		/************************************************************
		 * Get the attributes value
		 ************************************************************/
		attr = heap_getattr(tuple, i + 1, tupdesc, &isnull);

		/************************************************************
		 * Lookup the attribute type in the syscache
		 * for the output function
		 ************************************************************/
		typeTup = SearchSysCache1(TYPEOID,
							  ObjectIdGetDatum(tupdesc->attrs[i]->atttypid));
		if (!HeapTupleIsValid(typeTup))
			elog(ERROR, "cache lookup failed for type %u",
				 tupdesc->attrs[i]->atttypid);

		typoutput = ((Form_pg_type) GETSTRUCT(typeTup))->typoutput;
		ReleaseSysCache(typeTup);

		/************************************************************
		 * If there is a value, set the variable
		 * If not, unset it
		 *
		 * Hmmm - Null attributes will cause functions to
		 *		  crash if they don't expect them - need something
		 *		  smarter here.
		 ************************************************************/
		if (!isnull && OidIsValid(typoutput))
		{
			outputstr = OidOutputFunctionCall(typoutput, attr);
			UTF_BEGIN;
			Tcl_SetVar2(interp, *arrptr, *nameptr, UTF_E2U(outputstr), 0);
			UTF_END;
			pfree(outputstr);
		}
		else
			Tcl_UnsetVar2(interp, *arrptr, *nameptr, 0);
	}
}


/**********************************************************************
 * pltcl_build_tuple_argument() - Build a string usable for 'array set'
 *				  from all attributes of a given tuple
 **********************************************************************/
static void
pltcl_build_tuple_argument(HeapTuple tuple, TupleDesc tupdesc,
						   Tcl_DString *retval)
{
	int			i;
	char	   *outputstr;
	Datum		attr;
	bool		isnull;

	char	   *attname;
	HeapTuple	typeTup;
	Oid			typoutput;

	for (i = 0; i < tupdesc->natts; i++)
	{
		/* ignore dropped attributes */
		if (tupdesc->attrs[i]->attisdropped)
			continue;

		/************************************************************
		 * Get the attribute name
		 ************************************************************/
		attname = NameStr(tupdesc->attrs[i]->attname);

		/************************************************************
		 * Get the attributes value
		 ************************************************************/
		attr = heap_getattr(tuple, i + 1, tupdesc, &isnull);

		/************************************************************
		 * Lookup the attribute type in the syscache
		 * for the output function
		 ************************************************************/
		typeTup = SearchSysCache1(TYPEOID,
							  ObjectIdGetDatum(tupdesc->attrs[i]->atttypid));
		if (!HeapTupleIsValid(typeTup))
			elog(ERROR, "cache lookup failed for type %u",
				 tupdesc->attrs[i]->atttypid);

		typoutput = ((Form_pg_type) GETSTRUCT(typeTup))->typoutput;
		ReleaseSysCache(typeTup);

		/************************************************************
		 * If there is a value, append the attribute name and the
		 * value to the list
		 *
		 * Hmmm - Null attributes will cause functions to
		 *		  crash if they don't expect them - need something
		 *		  smarter here.
		 ************************************************************/
		if (!isnull && OidIsValid(typoutput))
		{
			outputstr = OidOutputFunctionCall(typoutput, attr);
			Tcl_DStringAppendElement(retval, attname);
			UTF_BEGIN;
			Tcl_DStringAppendElement(retval, UTF_E2U(outputstr));
			UTF_END;
			pfree(outputstr);
		}
	}
}
