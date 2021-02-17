/*-------------------------------------------------------------------------
 *
 * foreign.c
 *		  support for foreign-data wrappers, servers and user mappings.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 *
 * IDENTIFICATION
 *		  src/backend/foreign/foreign.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/reloptions.h"
#include "catalog/namespace.h"
#include "catalog/pg_foreign_data_wrapper.h"
#include "catalog/pg_foreign_server.h"
#include "catalog/pg_foreign_table.h"
#include "catalog/pg_type.h"
#include "catalog/pg_user_mapping.h"
#include "foreign/fdwapi.h"
#include "foreign/foreign.h"
#include "funcapi.h"
#include "miscadmin.h"
#include "nodes/parsenodes.h"
#include "utils/acl.h"
#include "utils/array.h"
#include "utils/builtins.h"
#include "utils/lsyscache.h"
#include "utils/memutils.h"
#include "utils/syscache.h"


extern Datum pg_options_to_table(PG_FUNCTION_ARGS);
extern Datum postgresql_fdw_validator(PG_FUNCTION_ARGS);



/*
 * GetForeignDataWrapper -	look up the foreign-data wrapper by OID.
 */
ForeignDataWrapper *
GetForeignDataWrapper(Oid fdwid)
{
	Form_pg_foreign_data_wrapper fdwform;
	ForeignDataWrapper *fdw;
	Datum		datum;
	HeapTuple	tp;
	bool		isnull;

	tp = SearchSysCache1(FOREIGNDATAWRAPPEROID, ObjectIdGetDatum(fdwid));

	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign-data wrapper %u", fdwid);

	fdwform = (Form_pg_foreign_data_wrapper) GETSTRUCT(tp);

	fdw = (ForeignDataWrapper *) palloc(sizeof(ForeignDataWrapper));
	fdw->fdwid = fdwid;
	fdw->owner = fdwform->fdwowner;
	fdw->fdwname = pstrdup(NameStr(fdwform->fdwname));
	fdw->fdwhandler = fdwform->fdwhandler;
	fdw->fdwvalidator = fdwform->fdwvalidator;

	/* Extract the fdwoptions */
	datum = SysCacheGetAttr(FOREIGNDATAWRAPPEROID,
							tp,
							Anum_pg_foreign_data_wrapper_fdwoptions,
							&isnull);
	if (isnull)
		fdw->options = NIL;
	else
		fdw->options = untransformRelOptions(datum);

	ReleaseSysCache(tp);

	return fdw;
}



/*
 * GetForeignDataWrapperByName - look up the foreign-data wrapper
 * definition by name.
 */
ForeignDataWrapper *
GetForeignDataWrapperByName(const char *fdwname, bool missing_ok)
{
	Oid			fdwId = get_foreign_data_wrapper_oid(fdwname, missing_ok);

	if (!OidIsValid(fdwId))
		return NULL;

	return GetForeignDataWrapper(fdwId);
}


/*
 * GetForeignServer - look up the foreign server definition.
 */
ForeignServer *
GetForeignServer(Oid serverid)
{
	Form_pg_foreign_server serverform;
	ForeignServer *server;
	HeapTuple	tp;
	Datum		datum;
	bool		isnull;

	tp = SearchSysCache1(FOREIGNSERVEROID, ObjectIdGetDatum(serverid));

	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign server %u", serverid);

	serverform = (Form_pg_foreign_server) GETSTRUCT(tp);

	server = (ForeignServer *) palloc(sizeof(ForeignServer));
	server->serverid = serverid;
	server->servername = pstrdup(NameStr(serverform->srvname));
	server->owner = serverform->srvowner;
	server->fdwid = serverform->srvfdw;

	/* Extract server type */
	datum = SysCacheGetAttr(FOREIGNSERVEROID,
							tp,
							Anum_pg_foreign_server_srvtype,
							&isnull);
	server->servertype = isnull ? NULL : pstrdup(TextDatumGetCString(datum));

	/* Extract server version */
	datum = SysCacheGetAttr(FOREIGNSERVEROID,
							tp,
							Anum_pg_foreign_server_srvversion,
							&isnull);
	server->serverversion = isnull ? NULL : pstrdup(TextDatumGetCString(datum));

	/* Extract the srvoptions */
	datum = SysCacheGetAttr(FOREIGNSERVEROID,
							tp,
							Anum_pg_foreign_server_srvoptions,
							&isnull);
	if (isnull)
		server->options = NIL;
	else
		server->options = untransformRelOptions(datum);

	ReleaseSysCache(tp);

	return server;
}


/*
 * GetForeignServerByName - look up the foreign server definition by name.
 */
ForeignServer *
GetForeignServerByName(const char *srvname, bool missing_ok)
{
	Oid			serverid = get_foreign_server_oid(srvname, missing_ok);

	if (!OidIsValid(serverid))
		return NULL;

	return GetForeignServer(serverid);
}


/*
 * GetUserMapping - look up the user mapping.
 *
 * If no mapping is found for the supplied user, we also look for
 * PUBLIC mappings (userid == InvalidOid).
 */
UserMapping *
GetUserMapping(Oid userid, Oid serverid)
{
	Datum		datum;
	HeapTuple	tp;
	bool		isnull;
	UserMapping *um;

	tp = SearchSysCache2(USERMAPPINGUSERSERVER,
						 ObjectIdGetDatum(userid),
						 ObjectIdGetDatum(serverid));

	if (!HeapTupleIsValid(tp))
	{
		/* Not found for the specific user -- try PUBLIC */
		tp = SearchSysCache2(USERMAPPINGUSERSERVER,
							 ObjectIdGetDatum(InvalidOid),
							 ObjectIdGetDatum(serverid));
	}

	if (!HeapTupleIsValid(tp))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("user mapping not found for \"%s\"",
						MappingUserName(userid))));

	um = (UserMapping *) palloc(sizeof(UserMapping));
	um->userid = userid;
	um->serverid = serverid;

	/* Extract the umoptions */
	datum = SysCacheGetAttr(USERMAPPINGUSERSERVER,
							tp,
							Anum_pg_user_mapping_umoptions,
							&isnull);
	if (isnull)
		um->options = NIL;
	else
		um->options = untransformRelOptions(datum);

	ReleaseSysCache(tp);

	return um;
}


/*
 * GetForeignTable - look up the foreign table definition by relation oid.
 */
ForeignTable *
GetForeignTable(Oid relid)
{
	Form_pg_foreign_table tableform;
	ForeignTable *ft;
	HeapTuple	tp;
	Datum		datum;
	bool		isnull;

	tp = SearchSysCache1(FOREIGNTABLEREL, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign table %u", relid);
	tableform = (Form_pg_foreign_table) GETSTRUCT(tp);

	ft = (ForeignTable *) palloc(sizeof(ForeignTable));
	ft->relid = relid;
	ft->serverid = tableform->ftserver;

	/* Extract the ftoptions */
	datum = SysCacheGetAttr(FOREIGNTABLEREL,
							tp,
							Anum_pg_foreign_table_ftoptions,
							&isnull);
	if (isnull)
		ft->options = NIL;
	else
		ft->options = untransformRelOptions(datum);

	ReleaseSysCache(tp);

	return ft;
}


/*
 * GetFdwRoutine - call the specified foreign-data wrapper handler routine
 * to get its FdwRoutine struct.
 */
FdwRoutine *
GetFdwRoutine(Oid fdwhandler)
{
	Datum		datum;
	FdwRoutine *routine;

	datum = OidFunctionCall0(fdwhandler);
	routine = (FdwRoutine *) DatumGetPointer(datum);

	if (routine == NULL || !IsA(routine, FdwRoutine))
		elog(ERROR, "foreign-data wrapper handler function %u did not return an FdwRoutine struct",
			 fdwhandler);

	return routine;
}


/*
 * GetFdwRoutineByRelId - look up the handler of the foreign-data wrapper
 * for the given foreign table, and retrieve its FdwRoutine struct.
 */
FdwRoutine *
GetFdwRoutineByRelId(Oid relid)
{
	HeapTuple	tp;
	Form_pg_foreign_data_wrapper fdwform;
	Form_pg_foreign_server serverform;
	Form_pg_foreign_table tableform;
	Oid			serverid;
	Oid			fdwid;
	Oid			fdwhandler;

	/* Get server OID for the foreign table. */
	tp = SearchSysCache1(FOREIGNTABLEREL, ObjectIdGetDatum(relid));
	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign table %u", relid);
	tableform = (Form_pg_foreign_table) GETSTRUCT(tp);
	serverid = tableform->ftserver;
	ReleaseSysCache(tp);

	/* Get foreign-data wrapper OID for the server. */
	tp = SearchSysCache1(FOREIGNSERVEROID, ObjectIdGetDatum(serverid));
	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign server %u", serverid);
	serverform = (Form_pg_foreign_server) GETSTRUCT(tp);
	fdwid = serverform->srvfdw;
	ReleaseSysCache(tp);

	/* Get handler function OID for the FDW. */
	tp = SearchSysCache1(FOREIGNDATAWRAPPEROID, ObjectIdGetDatum(fdwid));
	if (!HeapTupleIsValid(tp))
		elog(ERROR, "cache lookup failed for foreign-data wrapper %u", fdwid);
	fdwform = (Form_pg_foreign_data_wrapper) GETSTRUCT(tp);
	fdwhandler = fdwform->fdwhandler;

	/* Complain if FDW has been set to NO HANDLER. */
	if (!OidIsValid(fdwhandler))
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("foreign-data wrapper \"%s\" has no handler",
						NameStr(fdwform->fdwname))));

	ReleaseSysCache(tp);

	/* And finally, call the handler function. */
	return GetFdwRoutine(fdwhandler);
}


/*
 * deflist_to_tuplestore - Helper function to convert DefElem list to
 * tuplestore usable in SRF.
 */
static void
deflist_to_tuplestore(ReturnSetInfo *rsinfo, List *options)
{
	ListCell   *cell;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	Datum		values[2];
	bool		nulls[2];
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize) ||
		rsinfo->expectedDesc == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not allowed in this context")));

	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	/*
	 * Now prepare the result set.
	 */
	tupdesc = CreateTupleDescCopy(rsinfo->expectedDesc);
	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	foreach(cell, options)
	{
		DefElem    *def = lfirst(cell);

		values[0] = CStringGetTextDatum(def->defname);
		nulls[0] = false;
		if (def->arg)
		{
			values[1] = CStringGetTextDatum(((Value *) (def->arg))->val.str);
			nulls[1] = false;
		}
		else
		{
			values[1] = (Datum) 0;
			nulls[1] = true;
		}
		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	/* clean up and return the tuplestore */
	tuplestore_donestoring(tupstore);

	MemoryContextSwitchTo(oldcontext);
}


/*
 * Convert options array to name/value table.  Useful for information
 * schema and pg_dump.
 */
Datum
pg_options_to_table(PG_FUNCTION_ARGS)
{
	Datum		array = PG_GETARG_DATUM(0);

	deflist_to_tuplestore((ReturnSetInfo *) fcinfo->resultinfo,
						  untransformRelOptions(array));

	return (Datum) 0;
}


/*
 * Describes the valid options for postgresql FDW, server, and user mapping.
 */
struct ConnectionOption
{
	const char *optname;
	Oid			optcontext;		/* Oid of catalog in which option may appear */
};

/*
 * Copied from fe-connect.c PQconninfoOptions.
 *
 * The list is small - don't bother with bsearch if it stays so.
 */
static struct ConnectionOption libpq_conninfo_options[] = {
	{"authtype", ForeignServerRelationId},
	{"service", ForeignServerRelationId},
	{"user", UserMappingRelationId},
	{"password", UserMappingRelationId},
	{"connect_timeout", ForeignServerRelationId},
	{"dbname", ForeignServerRelationId},
	{"host", ForeignServerRelationId},
	{"hostaddr", ForeignServerRelationId},
	{"port", ForeignServerRelationId},
	{"tty", ForeignServerRelationId},
	{"options", ForeignServerRelationId},
	{"requiressl", ForeignServerRelationId},
	{"sslmode", ForeignServerRelationId},
	{"gsslib", ForeignServerRelationId},
	{NULL, InvalidOid}
};


/*
 * Check if the provided option is one of libpq conninfo options.
 * context is the Oid of the catalog the option came from, or 0 if we
 * don't care.
 */
static bool
is_conninfo_option(const char *option, Oid context)
{
	struct ConnectionOption *opt;

	for (opt = libpq_conninfo_options; opt->optname; opt++)
		if (context == opt->optcontext && strcmp(opt->optname, option) == 0)
			return true;
	return false;
}


/*
 * Validate the generic option given to SERVER or USER MAPPING.
 * Raise an ERROR if the option or its value is considered
 * invalid.
 *
 * Valid server options are all libpq conninfo options except
 * user and password -- these may only appear in USER MAPPING options.
 */
Datum
postgresql_fdw_validator(PG_FUNCTION_ARGS)
{
	List	   *options_list = untransformRelOptions(PG_GETARG_DATUM(0));
	Oid			catalog = PG_GETARG_OID(1);

	ListCell   *cell;

	foreach(cell, options_list)
	{
		DefElem    *def = lfirst(cell);

		if (!is_conninfo_option(def->defname, catalog))
		{
			struct ConnectionOption *opt;
			StringInfoData buf;

			/*
			 * Unknown option specified, complain about it. Provide a hint
			 * with list of valid options for the object.
			 */
			initStringInfo(&buf);
			for (opt = libpq_conninfo_options; opt->optname; opt++)
				if (catalog == opt->optcontext)
					appendStringInfo(&buf, "%s%s", (buf.len > 0) ? ", " : "",
									 opt->optname);

			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("invalid option \"%s\"", def->defname),
					 errhint("Valid options in this context are: %s",
							 buf.data)));

			PG_RETURN_BOOL(false);
		}
	}

	PG_RETURN_BOOL(true);
}

/*
 * get_foreign_data_wrapper_oid - given a FDW name, look up the OID
 *
 * If missing_ok is false, throw an error if name not found.  If true, just
 * return InvalidOid.
 */
Oid
get_foreign_data_wrapper_oid(const char *fdwname, bool missing_ok)
{
	Oid			oid;

	oid = GetSysCacheOid1(FOREIGNDATAWRAPPERNAME, CStringGetDatum(fdwname));
	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("foreign-data wrapper \"%s\" does not exist",
						fdwname)));
	return oid;
}

/*
 * get_foreign_server_oid - given a FDW name, look up the OID
 *
 * If missing_ok is false, throw an error if name not found.  If true, just
 * return InvalidOid.
 */
Oid
get_foreign_server_oid(const char *servername, bool missing_ok)
{
	Oid			oid;

	oid = GetSysCacheOid1(FOREIGNSERVERNAME, CStringGetDatum(servername));
	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("server \"%s\" does not exist", servername)));
	return oid;
}
