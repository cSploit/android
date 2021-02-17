/*-------------------------------------------------------------------------
 *
 * extension.c
 *	  Commands to manipulate extensions
 *
 * Extensions in PostgreSQL allow management of collections of SQL objects.
 *
 * All we need internally to manage an extension is an OID so that the
 * dependent objects can be associated with it.  An extension is created by
 * populating the pg_extension catalog from a "control" file.
 * The extension control file is parsed with the same parser we use for
 * postgresql.conf and recovery.conf.  An extension also has an installation
 * script file, containing SQL commands to create the extension's objects.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/commands/extension.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <dirent.h>
#include <limits.h>
#include <unistd.h>

#include "access/sysattr.h"
#include "access/xact.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/namespace.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_collation.h"
#include "catalog/pg_depend.h"
#include "catalog/pg_extension.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_type.h"
#include "commands/alter.h"
#include "commands/comment.h"
#include "commands/extension.h"
#include "commands/schemacmds.h"
#include "commands/trigger.h"
#include "executor/executor.h"
#include "funcapi.h"
#include "mb/pg_wchar.h"
#include "miscadmin.h"
#include "tcop/tcopprot.h"
#include "tcop/utility.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/guc.h"
#include "utils/lsyscache.h"
#include "utils/snapmgr.h"
#include "utils/tqual.h"


/* Globally visible state variables */
bool		creating_extension = false;
Oid			CurrentExtensionObject = InvalidOid;

/*
 * Internal data structure to hold the results of parsing a control file
 */
typedef struct ExtensionControlFile
{
	char	   *name;			/* name of the extension */
	char	   *directory;		/* directory for script files */
	char	   *default_version;	/* default install target version, if any */
	char	   *module_pathname;	/* string to substitute for MODULE_PATHNAME */
	char	   *comment;		/* comment, if any */
	char	   *schema;			/* target schema (allowed if !relocatable) */
	bool		relocatable;	/* is ALTER EXTENSION SET SCHEMA supported? */
	bool		superuser;		/* must be superuser to install? */
	int			encoding;		/* encoding of the script file, or -1 */
	List	   *requires;		/* names of prerequisite extensions */
} ExtensionControlFile;

/*
 * Internal data structure for update path information
 */
typedef struct ExtensionVersionInfo
{
	char	   *name;			/* name of the starting version */
	List	   *reachable;		/* List of ExtensionVersionInfo's */
	bool		installable;	/* does this version have an install script? */
	/* working state for Dijkstra's algorithm: */
	bool		distance_known; /* is distance from start known yet? */
	int			distance;		/* current worst-case distance estimate */
	struct ExtensionVersionInfo *previous;		/* current best predecessor */
} ExtensionVersionInfo;

/* Local functions */
static List *find_update_path(List *evi_list,
				 ExtensionVersionInfo *evi_start,
				 ExtensionVersionInfo *evi_target,
				 bool reinitialize);
static void get_available_versions_for_extension(ExtensionControlFile *pcontrol,
									 Tuplestorestate *tupstore,
									 TupleDesc tupdesc);
static void ApplyExtensionUpdates(Oid extensionOid,
					  ExtensionControlFile *pcontrol,
					  const char *initialVersion,
					  List *updateVersions);


/*
 * get_extension_oid - given an extension name, look up the OID
 *
 * If missing_ok is false, throw an error if extension name not found.	If
 * true, just return InvalidOid.
 */
Oid
get_extension_oid(const char *extname, bool missing_ok)
{
	Oid			result;
	Relation	rel;
	SysScanDesc scandesc;
	HeapTuple	tuple;
	ScanKeyData entry[1];

	rel = heap_open(ExtensionRelationId, AccessShareLock);

	ScanKeyInit(&entry[0],
				Anum_pg_extension_extname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(extname));

	scandesc = systable_beginscan(rel, ExtensionNameIndexId, true,
								  SnapshotNow, 1, entry);

	tuple = systable_getnext(scandesc);

	/* We assume that there can be at most one matching tuple */
	if (HeapTupleIsValid(tuple))
		result = HeapTupleGetOid(tuple);
	else
		result = InvalidOid;

	systable_endscan(scandesc);

	heap_close(rel, AccessShareLock);

	if (!OidIsValid(result) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("extension \"%s\" does not exist",
						extname)));

	return result;
}

/*
 * get_extension_name - given an extension OID, look up the name
 *
 * Returns a palloc'd string, or NULL if no such extension.
 */
char *
get_extension_name(Oid ext_oid)
{
	char	   *result;
	Relation	rel;
	SysScanDesc scandesc;
	HeapTuple	tuple;
	ScanKeyData entry[1];

	rel = heap_open(ExtensionRelationId, AccessShareLock);

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(ext_oid));

	scandesc = systable_beginscan(rel, ExtensionOidIndexId, true,
								  SnapshotNow, 1, entry);

	tuple = systable_getnext(scandesc);

	/* We assume that there can be at most one matching tuple */
	if (HeapTupleIsValid(tuple))
		result = pstrdup(NameStr(((Form_pg_extension) GETSTRUCT(tuple))->extname));
	else
		result = NULL;

	systable_endscan(scandesc);

	heap_close(rel, AccessShareLock);

	return result;
}

/*
 * get_extension_schema - given an extension OID, fetch its extnamespace
 *
 * Returns InvalidOid if no such extension.
 */
static Oid
get_extension_schema(Oid ext_oid)
{
	Oid			result;
	Relation	rel;
	SysScanDesc scandesc;
	HeapTuple	tuple;
	ScanKeyData entry[1];

	rel = heap_open(ExtensionRelationId, AccessShareLock);

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(ext_oid));

	scandesc = systable_beginscan(rel, ExtensionOidIndexId, true,
								  SnapshotNow, 1, entry);

	tuple = systable_getnext(scandesc);

	/* We assume that there can be at most one matching tuple */
	if (HeapTupleIsValid(tuple))
		result = ((Form_pg_extension) GETSTRUCT(tuple))->extnamespace;
	else
		result = InvalidOid;

	systable_endscan(scandesc);

	heap_close(rel, AccessShareLock);

	return result;
}

/*
 * Utility functions to check validity of extension and version names
 */
static void
check_valid_extension_name(const char *extensionname)
{
	int			namelen = strlen(extensionname);

	/*
	 * Disallow empty names (the parser rejects empty identifiers anyway, but
	 * let's check).
	 */
	if (namelen == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid extension name: \"%s\"", extensionname),
				 errdetail("Extension names must not be empty.")));

	/*
	 * No double dashes, since that would make script filenames ambiguous.
	 */
	if (strstr(extensionname, "--"))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid extension name: \"%s\"", extensionname),
				 errdetail("Extension names must not contain \"--\".")));

	/*
	 * No leading or trailing dash either.	(We could probably allow this, but
	 * it would require much care in filename parsing and would make filenames
	 * visually if not formally ambiguous.	Since there's no real-world use
	 * case, let's just forbid it.)
	 */
	if (extensionname[0] == '-' || extensionname[namelen - 1] == '-')
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid extension name: \"%s\"", extensionname),
			errdetail("Extension names must not begin or end with \"-\".")));

	/*
	 * No directory separators either (this is sufficient to prevent ".."
	 * style attacks).
	 */
	if (first_dir_separator(extensionname) != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("invalid extension name: \"%s\"", extensionname),
				 errdetail("Extension names must not contain directory separator characters.")));
}

static void
check_valid_version_name(const char *versionname)
{
	int			namelen = strlen(versionname);

	/*
	 * Disallow empty names (we could possibly allow this, but there seems
	 * little point).
	 */
	if (namelen == 0)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("invalid extension version name: \"%s\"", versionname),
				 errdetail("Version names must not be empty.")));

	/*
	 * No double dashes, since that would make script filenames ambiguous.
	 */
	if (strstr(versionname, "--"))
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("invalid extension version name: \"%s\"", versionname),
				 errdetail("Version names must not contain \"--\".")));

	/*
	 * No leading or trailing dash either.
	 */
	if (versionname[0] == '-' || versionname[namelen - 1] == '-')
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("invalid extension version name: \"%s\"", versionname),
			  errdetail("Version names must not begin or end with \"-\".")));

	/*
	 * No directory separators either (this is sufficient to prevent ".."
	 * style attacks).
	 */
	if (first_dir_separator(versionname) != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
			   errmsg("invalid extension version name: \"%s\"", versionname),
				 errdetail("Version names must not contain directory separator characters.")));
}

/*
 * Utility functions to handle extension-related path names
 */
static bool
is_extension_control_filename(const char *filename)
{
	const char *extension = strrchr(filename, '.');

	return (extension != NULL) && (strcmp(extension, ".control") == 0);
}

static bool
is_extension_script_filename(const char *filename)
{
	const char *extension = strrchr(filename, '.');

	return (extension != NULL) && (strcmp(extension, ".sql") == 0);
}

static char *
get_extension_control_directory(void)
{
	char		sharepath[MAXPGPATH];
	char	   *result;

	get_share_path(my_exec_path, sharepath);
	result = (char *) palloc(MAXPGPATH);
	snprintf(result, MAXPGPATH, "%s/extension", sharepath);

	return result;
}

static char *
get_extension_control_filename(const char *extname)
{
	char		sharepath[MAXPGPATH];
	char	   *result;

	get_share_path(my_exec_path, sharepath);
	result = (char *) palloc(MAXPGPATH);
	snprintf(result, MAXPGPATH, "%s/extension/%s.control",
			 sharepath, extname);

	return result;
}

static char *
get_extension_script_directory(ExtensionControlFile *control)
{
	char		sharepath[MAXPGPATH];
	char	   *result;

	/*
	 * The directory parameter can be omitted, absolute, or relative to the
	 * installation's share directory.
	 */
	if (!control->directory)
		return get_extension_control_directory();

	if (is_absolute_path(control->directory))
		return pstrdup(control->directory);

	get_share_path(my_exec_path, sharepath);
	result = (char *) palloc(MAXPGPATH);
	snprintf(result, MAXPGPATH, "%s/%s", sharepath, control->directory);

	return result;
}

static char *
get_extension_aux_control_filename(ExtensionControlFile *control,
								   const char *version)
{
	char	   *result;
	char	   *scriptdir;

	scriptdir = get_extension_script_directory(control);

	result = (char *) palloc(MAXPGPATH);
	snprintf(result, MAXPGPATH, "%s/%s--%s.control",
			 scriptdir, control->name, version);

	pfree(scriptdir);

	return result;
}

static char *
get_extension_script_filename(ExtensionControlFile *control,
							  const char *from_version, const char *version)
{
	char	   *result;
	char	   *scriptdir;

	scriptdir = get_extension_script_directory(control);

	result = (char *) palloc(MAXPGPATH);
	if (from_version)
		snprintf(result, MAXPGPATH, "%s/%s--%s--%s.sql",
				 scriptdir, control->name, from_version, version);
	else
		snprintf(result, MAXPGPATH, "%s/%s--%s.sql",
				 scriptdir, control->name, version);

	pfree(scriptdir);

	return result;
}


/*
 * Parse contents of primary or auxiliary control file, and fill in
 * fields of *control.	We parse primary file if version == NULL,
 * else the optional auxiliary file for that version.
 *
 * Control files are supposed to be very short, half a dozen lines,
 * so we don't worry about memory allocation risks here.  Also we don't
 * worry about what encoding it's in; all values are expected to be ASCII.
 */
static void
parse_extension_control_file(ExtensionControlFile *control,
							 const char *version)
{
	char	   *filename;
	FILE	   *file;
	ConfigVariable *item,
			   *head = NULL,
			   *tail = NULL;

	/*
	 * Locate the file to read.  Auxiliary files are optional.
	 */
	if (version)
		filename = get_extension_aux_control_filename(control, version);
	else
		filename = get_extension_control_filename(control->name);

	if ((file = AllocateFile(filename, "r")) == NULL)
	{
		if (version && errno == ENOENT)
		{
			/* no auxiliary file for this version */
			pfree(filename);
			return;
		}
		ereport(ERROR,
				(errcode_for_file_access(),
				 errmsg("could not open extension control file \"%s\": %m",
						filename)));
	}

	/*
	 * Parse the file content, using GUC's file parsing code
	 */
	ParseConfigFp(file, filename, 0, ERROR, &head, &tail);

	FreeFile(file);

	/*
	 * Convert the ConfigVariable list into ExtensionControlFile entries.
	 */
	for (item = head; item != NULL; item = item->next)
	{
		if (strcmp(item->name, "directory") == 0)
		{
			if (version)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("parameter \"%s\" cannot be set in a secondary extension control file",
								item->name)));

			control->directory = pstrdup(item->value);
		}
		else if (strcmp(item->name, "default_version") == 0)
		{
			if (version)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("parameter \"%s\" cannot be set in a secondary extension control file",
								item->name)));

			control->default_version = pstrdup(item->value);
		}
		else if (strcmp(item->name, "module_pathname") == 0)
		{
			control->module_pathname = pstrdup(item->value);
		}
		else if (strcmp(item->name, "comment") == 0)
		{
			control->comment = pstrdup(item->value);
		}
		else if (strcmp(item->name, "schema") == 0)
		{
			control->schema = pstrdup(item->value);
		}
		else if (strcmp(item->name, "relocatable") == 0)
		{
			if (!parse_bool(item->value, &control->relocatable))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("parameter \"%s\" requires a Boolean value",
								item->name)));
		}
		else if (strcmp(item->name, "superuser") == 0)
		{
			if (!parse_bool(item->value, &control->superuser))
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
						 errmsg("parameter \"%s\" requires a Boolean value",
								item->name)));
		}
		else if (strcmp(item->name, "encoding") == 0)
		{
			control->encoding = pg_valid_server_encoding(item->value);
			if (control->encoding < 0)
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("\"%s\" is not a valid encoding name",
								item->value)));
		}
		else if (strcmp(item->name, "requires") == 0)
		{
			/* Need a modifiable copy of string */
			char	   *rawnames = pstrdup(item->value);

			/* Parse string into list of identifiers */
			if (!SplitIdentifierString(rawnames, ',', &control->requires))
			{
				/* syntax error in name list */
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("parameter \"%s\" must be a list of extension names",
						item->name)));
			}
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("unrecognized parameter \"%s\" in file \"%s\"",
							item->name, filename)));
	}

	FreeConfigVariables(head);

	if (control->relocatable && control->schema != NULL)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("parameter \"schema\" cannot be specified when \"relocatable\" is true")));

	pfree(filename);
}

/*
 * Read the primary control file for the specified extension.
 */
static ExtensionControlFile *
read_extension_control_file(const char *extname)
{
	ExtensionControlFile *control;

	/*
	 * Set up default values.  Pointer fields are initially null.
	 */
	control = (ExtensionControlFile *) palloc0(sizeof(ExtensionControlFile));
	control->name = pstrdup(extname);
	control->relocatable = false;
	control->superuser = true;
	control->encoding = -1;

	/*
	 * Parse the primary control file.
	 */
	parse_extension_control_file(control, NULL);

	return control;
}

/*
 * Read the auxiliary control file for the specified extension and version.
 *
 * Returns a new modified ExtensionControlFile struct; the original struct
 * (reflecting just the primary control file) is not modified.
 */
static ExtensionControlFile *
read_extension_aux_control_file(const ExtensionControlFile *pcontrol,
								const char *version)
{
	ExtensionControlFile *acontrol;

	/*
	 * Flat-copy the struct.  Pointer fields share values with original.
	 */
	acontrol = (ExtensionControlFile *) palloc(sizeof(ExtensionControlFile));
	memcpy(acontrol, pcontrol, sizeof(ExtensionControlFile));

	/*
	 * Parse the auxiliary control file, overwriting struct fields
	 */
	parse_extension_control_file(acontrol, version);

	return acontrol;
}

/*
 * Read an SQL script file into a string, and convert to database encoding
 */
static char *
read_extension_script_file(const ExtensionControlFile *control,
						   const char *filename)
{
	int			src_encoding;
	int			dest_encoding = GetDatabaseEncoding();
	bytea	   *content;
	char	   *src_str;
	char	   *dest_str;
	int			len;

	content = read_binary_file(filename, 0, -1);

	/* use database encoding if not given */
	if (control->encoding < 0)
		src_encoding = dest_encoding;
	else
		src_encoding = control->encoding;

	/* make sure that source string is valid in the expected encoding */
	len = VARSIZE_ANY_EXHDR(content);
	src_str = VARDATA_ANY(content);
	pg_verify_mbstr_len(src_encoding, src_str, len, false);

	/* convert the encoding to the database encoding */
	dest_str = (char *) pg_do_encoding_conversion((unsigned char *) src_str,
												  len,
												  src_encoding,
												  dest_encoding);

	/* if no conversion happened, we have to arrange for null termination */
	if (dest_str == src_str)
	{
		dest_str = (char *) palloc(len + 1);
		memcpy(dest_str, src_str, len);
		dest_str[len] = '\0';
	}

	return dest_str;
}

/*
 * Execute given SQL string.
 *
 * filename is used only to report errors.
 *
 * Note: it's tempting to just use SPI to execute the string, but that does
 * not work very well.	The really serious problem is that SPI will parse,
 * analyze, and plan the whole string before executing any of it; of course
 * this fails if there are any plannable statements referring to objects
 * created earlier in the script.  A lesser annoyance is that SPI insists
 * on printing the whole string as errcontext in case of any error, and that
 * could be very long.
 */
static void
execute_sql_string(const char *sql, const char *filename)
{
	List	   *raw_parsetree_list;
	DestReceiver *dest;
	ListCell   *lc1;

	/*
	 * Parse the SQL string into a list of raw parse trees.
	 */
	raw_parsetree_list = pg_parse_query(sql);

	/* All output from SELECTs goes to the bit bucket */
	dest = CreateDestReceiver(DestNone);

	/*
	 * Do parse analysis, rule rewrite, planning, and execution for each raw
	 * parsetree.  We must fully execute each query before beginning parse
	 * analysis on the next one, since there may be interdependencies.
	 */
	foreach(lc1, raw_parsetree_list)
	{
		Node	   *parsetree = (Node *) lfirst(lc1);
		List	   *stmt_list;
		ListCell   *lc2;

		stmt_list = pg_analyze_and_rewrite(parsetree,
										   sql,
										   NULL,
										   0);
		stmt_list = pg_plan_queries(stmt_list, 0, NULL);

		foreach(lc2, stmt_list)
		{
			Node	   *stmt = (Node *) lfirst(lc2);

			if (IsA(stmt, TransactionStmt))
				ereport(ERROR,
						(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
						 errmsg("transaction control statements are not allowed within an extension script")));

			CommandCounterIncrement();

			PushActiveSnapshot(GetTransactionSnapshot());

			if (IsA(stmt, PlannedStmt) &&
				((PlannedStmt *) stmt)->utilityStmt == NULL)
			{
				QueryDesc  *qdesc;

				qdesc = CreateQueryDesc((PlannedStmt *) stmt,
										sql,
										GetActiveSnapshot(), NULL,
										dest, NULL, 0);

				ExecutorStart(qdesc, 0);
				ExecutorRun(qdesc, ForwardScanDirection, 0);
				ExecutorFinish(qdesc);
				ExecutorEnd(qdesc);

				FreeQueryDesc(qdesc);
			}
			else
			{
				ProcessUtility(stmt,
							   sql,
							   NULL,
							   false,	/* not top level */
							   dest,
							   NULL);
			}

			PopActiveSnapshot();
		}
	}

	/* Be sure to advance the command counter after the last script command */
	CommandCounterIncrement();
}

/*
 * Execute the appropriate script file for installing or updating the extension
 *
 * If from_version isn't NULL, it's an update
 */
static void
execute_extension_script(Oid extensionOid, ExtensionControlFile *control,
						 const char *from_version,
						 const char *version,
						 List *requiredSchemas,
						 const char *schemaName, Oid schemaOid)
{
	char	   *filename;
	int			save_nestlevel;
	StringInfoData pathbuf;
	ListCell   *lc;

	/*
	 * Enforce superuser-ness if appropriate.  We postpone this check until
	 * here so that the flag is correctly associated with the right script(s)
	 * if it's set in secondary control files.
	 */
	if (control->superuser && !superuser())
	{
		if (from_version == NULL)
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied to create extension \"%s\"",
							control->name),
					 errhint("Must be superuser to create this extension.")));
		else
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("permission denied to update extension \"%s\"",
							control->name),
					 errhint("Must be superuser to update this extension.")));
	}

	filename = get_extension_script_filename(control, from_version, version);

	/*
	 * Force client_min_messages and log_min_messages to be at least WARNING,
	 * so that we won't spam the user with useless NOTICE messages from common
	 * script actions like creating shell types.
	 *
	 * We use the equivalent of a function SET option to allow the setting to
	 * persist for exactly the duration of the script execution.  guc.c also
	 * takes care of undoing the setting on error.
	 */
	save_nestlevel = NewGUCNestLevel();

	if (client_min_messages < WARNING)
		(void) set_config_option("client_min_messages", "warning",
								 PGC_USERSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true);
	if (log_min_messages < WARNING)
		(void) set_config_option("log_min_messages", "warning",
								 PGC_SUSET, PGC_S_SESSION,
								 GUC_ACTION_SAVE, true);

	/*
	 * Set up the search path to contain the target schema, then the schemas
	 * of any prerequisite extensions, and nothing else.  In particular this
	 * makes the target schema be the default creation target namespace.
	 *
	 * Note: it might look tempting to use PushOverrideSearchPath for this,
	 * but we cannot do that.  We have to actually set the search_path GUC in
	 * case the extension script examines or changes it.  In any case, the
	 * GUC_ACTION_SAVE method is just as convenient.
	 */
	initStringInfo(&pathbuf);
	appendStringInfoString(&pathbuf, quote_identifier(schemaName));
	foreach(lc, requiredSchemas)
	{
		Oid			reqschema = lfirst_oid(lc);
		char	   *reqname = get_namespace_name(reqschema);

		if (reqname)
			appendStringInfo(&pathbuf, ", %s", quote_identifier(reqname));
	}

	(void) set_config_option("search_path", pathbuf.data,
							 PGC_USERSET, PGC_S_SESSION,
							 GUC_ACTION_SAVE, true);

	/*
	 * Set creating_extension and related variables so that
	 * recordDependencyOnCurrentExtension and other functions do the right
	 * things.	On failure, ensure we reset these variables.
	 */
	creating_extension = true;
	CurrentExtensionObject = extensionOid;
	PG_TRY();
	{
		char	   *c_sql = read_extension_script_file(control, filename);
		Datum		t_sql;

		/* We use various functions that want to operate on text datums */
		t_sql = CStringGetTextDatum(c_sql);

		/*
		 * Reduce any lines beginning with "\echo" to empty.  This allows
		 * scripts to contain messages telling people not to run them via
		 * psql, which has been found to be necessary due to old habits.
		 */
		t_sql = DirectFunctionCall4Coll(textregexreplace,
										C_COLLATION_OID,
										t_sql,
										CStringGetTextDatum("^\\\\echo.*$"),
										CStringGetTextDatum(""),
										CStringGetTextDatum("ng"));

		/*
		 * If it's not relocatable, substitute the target schema name for
		 * occcurrences of @extschema@.
		 *
		 * For a relocatable extension, we needn't do this.  There cannot be
		 * any need for @extschema@, else it wouldn't be relocatable.
		 */
		if (!control->relocatable)
		{
			const char *qSchemaName = quote_identifier(schemaName);

			t_sql = DirectFunctionCall3(replace_text,
										t_sql,
										CStringGetTextDatum("@extschema@"),
										CStringGetTextDatum(qSchemaName));
		}

		/*
		 * If module_pathname was set in the control file, substitute its
		 * value for occurrences of MODULE_PATHNAME.
		 */
		if (control->module_pathname)
		{
			t_sql = DirectFunctionCall3(replace_text,
										t_sql,
										CStringGetTextDatum("MODULE_PATHNAME"),
							CStringGetTextDatum(control->module_pathname));
		}

		/* And now back to C string */
		c_sql = text_to_cstring(DatumGetTextPP(t_sql));

		execute_sql_string(c_sql, filename);
	}
	PG_CATCH();
	{
		creating_extension = false;
		CurrentExtensionObject = InvalidOid;
		PG_RE_THROW();
	}
	PG_END_TRY();

	creating_extension = false;
	CurrentExtensionObject = InvalidOid;

	/*
	 * Restore the GUC variables we set above.
	 */
	AtEOXact_GUC(true, save_nestlevel);
}

/*
 * Find or create an ExtensionVersionInfo for the specified version name
 *
 * Currently, we just use a List of the ExtensionVersionInfo's.  Searching
 * for them therefore uses about O(N^2) time when there are N versions of
 * the extension.  We could change the data structure to a hash table if
 * this ever becomes a bottleneck.
 */
static ExtensionVersionInfo *
get_ext_ver_info(const char *versionname, List **evi_list)
{
	ExtensionVersionInfo *evi;
	ListCell   *lc;

	foreach(lc, *evi_list)
	{
		evi = (ExtensionVersionInfo *) lfirst(lc);
		if (strcmp(evi->name, versionname) == 0)
			return evi;
	}

	evi = (ExtensionVersionInfo *) palloc(sizeof(ExtensionVersionInfo));
	evi->name = pstrdup(versionname);
	evi->reachable = NIL;
	evi->installable = false;
	/* initialize for later application of Dijkstra's algorithm */
	evi->distance_known = false;
	evi->distance = INT_MAX;
	evi->previous = NULL;

	*evi_list = lappend(*evi_list, evi);

	return evi;
}

/*
 * Locate the nearest unprocessed ExtensionVersionInfo
 *
 * This part of the algorithm is also about O(N^2).  A priority queue would
 * make it much faster, but for now there's no need.
 */
static ExtensionVersionInfo *
get_nearest_unprocessed_vertex(List *evi_list)
{
	ExtensionVersionInfo *evi = NULL;
	ListCell   *lc;

	foreach(lc, evi_list)
	{
		ExtensionVersionInfo *evi2 = (ExtensionVersionInfo *) lfirst(lc);

		/* only vertices whose distance is still uncertain are candidates */
		if (evi2->distance_known)
			continue;
		/* remember the closest such vertex */
		if (evi == NULL ||
			evi->distance > evi2->distance)
			evi = evi2;
	}

	return evi;
}

/*
 * Obtain information about the set of update scripts available for the
 * specified extension.  The result is a List of ExtensionVersionInfo
 * structs, each with a subsidiary list of the ExtensionVersionInfos for
 * the versions that can be reached in one step from that version.
 */
static List *
get_ext_ver_list(ExtensionControlFile *control)
{
	List	   *evi_list = NIL;
	int			extnamelen = strlen(control->name);
	char	   *location;
	DIR		   *dir;
	struct dirent *de;

	location = get_extension_script_directory(control);
	dir = AllocateDir(location);
	while ((de = ReadDir(dir, location)) != NULL)
	{
		char	   *vername;
		char	   *vername2;
		ExtensionVersionInfo *evi;
		ExtensionVersionInfo *evi2;

		/* must be a .sql file ... */
		if (!is_extension_script_filename(de->d_name))
			continue;

		/* ... matching extension name followed by separator */
		if (strncmp(de->d_name, control->name, extnamelen) != 0 ||
			de->d_name[extnamelen] != '-' ||
			de->d_name[extnamelen + 1] != '-')
			continue;

		/* extract version name(s) from 'extname--something.sql' filename */
		vername = pstrdup(de->d_name + extnamelen + 2);
		*strrchr(vername, '.') = '\0';
		vername2 = strstr(vername, "--");
		if (!vername2)
		{
			/* It's an install, not update, script; record its version name */
			evi = get_ext_ver_info(vername, &evi_list);
			evi->installable = true;
			continue;
		}
		*vername2 = '\0';		/* terminate first version */
		vername2 += 2;			/* and point to second */

		/* if there's a third --, it's bogus, ignore it */
		if (strstr(vername2, "--"))
			continue;

		/* Create ExtensionVersionInfos and link them together */
		evi = get_ext_ver_info(vername, &evi_list);
		evi2 = get_ext_ver_info(vername2, &evi_list);
		evi->reachable = lappend(evi->reachable, evi2);
	}
	FreeDir(dir);

	return evi_list;
}

/*
 * Given an initial and final version name, identify the sequence of update
 * scripts that have to be applied to perform that update.
 *
 * Result is a List of names of versions to transition through (the initial
 * version is *not* included).
 */
static List *
identify_update_path(ExtensionControlFile *control,
					 const char *oldVersion, const char *newVersion)
{
	List	   *result;
	List	   *evi_list;
	ExtensionVersionInfo *evi_start;
	ExtensionVersionInfo *evi_target;

	/* Extract the version update graph from the script directory */
	evi_list = get_ext_ver_list(control);

	/* Initialize start and end vertices */
	evi_start = get_ext_ver_info(oldVersion, &evi_list);
	evi_target = get_ext_ver_info(newVersion, &evi_list);

	/* Find shortest path */
	result = find_update_path(evi_list, evi_start, evi_target, false);

	if (result == NIL)
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("extension \"%s\" has no update path from version \"%s\" to version \"%s\"",
						control->name, oldVersion, newVersion)));

	return result;
}

/*
 * Apply Dijkstra's algorithm to find the shortest path from evi_start to
 * evi_target.
 *
 * If reinitialize is false, assume the ExtensionVersionInfo list has not
 * been used for this before, and the initialization done by get_ext_ver_info
 * is still good.
 *
 * Result is a List of names of versions to transition through (the initial
 * version is *not* included).	Returns NIL if no such path.
 */
static List *
find_update_path(List *evi_list,
				 ExtensionVersionInfo *evi_start,
				 ExtensionVersionInfo *evi_target,
				 bool reinitialize)
{
	List	   *result;
	ExtensionVersionInfo *evi;
	ListCell   *lc;

	/* Caller error if start == target */
	Assert(evi_start != evi_target);

	if (reinitialize)
	{
		foreach(lc, evi_list)
		{
			evi = (ExtensionVersionInfo *) lfirst(lc);
			evi->distance_known = false;
			evi->distance = INT_MAX;
			evi->previous = NULL;
		}
	}

	evi_start->distance = 0;

	while ((evi = get_nearest_unprocessed_vertex(evi_list)) != NULL)
	{
		if (evi->distance == INT_MAX)
			break;				/* all remaining vertices are unreachable */
		evi->distance_known = true;
		if (evi == evi_target)
			break;				/* found shortest path to target */
		foreach(lc, evi->reachable)
		{
			ExtensionVersionInfo *evi2 = (ExtensionVersionInfo *) lfirst(lc);
			int			newdist;

			newdist = evi->distance + 1;
			if (newdist < evi2->distance)
			{
				evi2->distance = newdist;
				evi2->previous = evi;
			}
			else if (newdist == evi2->distance &&
					 evi2->previous != NULL &&
					 strcmp(evi->name, evi2->previous->name) < 0)
			{
				/*
				 * Break ties in favor of the version name that comes first
				 * according to strcmp().  This behavior is undocumented and
				 * users shouldn't rely on it.  We do it just to ensure that
				 * if there is a tie, the update path that is chosen does not
				 * depend on random factors like the order in which directory
				 * entries get visited.
				 */
				evi2->previous = evi;
			}
		}
	}

	/* Return NIL if target is not reachable from start */
	if (!evi_target->distance_known)
		return NIL;

	/* Build and return list of version names representing the update path */
	result = NIL;
	for (evi = evi_target; evi != evi_start; evi = evi->previous)
		result = lcons(evi->name, result);

	return result;
}

/*
 * CREATE EXTENSION
 */
void
CreateExtension(CreateExtensionStmt *stmt)
{
	DefElem    *d_schema = NULL;
	DefElem    *d_new_version = NULL;
	DefElem    *d_old_version = NULL;
	char	   *schemaName;
	Oid			schemaOid;
	char	   *versionName;
	char	   *oldVersionName;
	Oid			extowner = GetUserId();
	ExtensionControlFile *pcontrol;
	ExtensionControlFile *control;
	List	   *updateVersions;
	List	   *requiredExtensions;
	List	   *requiredSchemas;
	Oid			extensionOid;
	ListCell   *lc;

	/* Check extension name validity before any filesystem access */
	check_valid_extension_name(stmt->extname);

	/*
	 * Check for duplicate extension name.	The unique index on
	 * pg_extension.extname would catch this anyway, and serves as a backstop
	 * in case of race conditions; but this is a friendlier error message, and
	 * besides we need a check to support IF NOT EXISTS.
	 */
	if (get_extension_oid(stmt->extname, true) != InvalidOid)
	{
		if (stmt->if_not_exists)
		{
			ereport(NOTICE,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("extension \"%s\" already exists, skipping",
							stmt->extname)));
			return;
		}
		else
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("extension \"%s\" already exists",
							stmt->extname)));
	}

	/*
	 * We use global variables to track the extension being created, so we can
	 * create only one extension at the same time.
	 */
	if (creating_extension)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("nested CREATE EXTENSION is not supported")));

	/*
	 * Read the primary control file.  Note we assume that it does not contain
	 * any non-ASCII data, so there is no need to worry about encoding at this
	 * point.
	 */
	pcontrol = read_extension_control_file(stmt->extname);

	/*
	 * Read the statement option list
	 */
	foreach(lc, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(lc);

		if (strcmp(defel->defname, "schema") == 0)
		{
			if (d_schema)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			d_schema = defel;
		}
		else if (strcmp(defel->defname, "new_version") == 0)
		{
			if (d_new_version)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			d_new_version = defel;
		}
		else if (strcmp(defel->defname, "old_version") == 0)
		{
			if (d_old_version)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			d_old_version = defel;
		}
		else
			elog(ERROR, "unrecognized option: %s", defel->defname);
	}

	/*
	 * Determine the version to install
	 */
	if (d_new_version && d_new_version->arg)
		versionName = strVal(d_new_version->arg);
	else if (pcontrol->default_version)
		versionName = pcontrol->default_version;
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("version to install must be specified")));
		versionName = NULL;		/* keep compiler quiet */
	}
	check_valid_version_name(versionName);

	/*
	 * Determine the (unpackaged) version to update from, if any, and then
	 * figure out what sequence of update scripts we need to apply.
	 */
	if (d_old_version && d_old_version->arg)
	{
		oldVersionName = strVal(d_old_version->arg);
		check_valid_version_name(oldVersionName);

		if (strcmp(oldVersionName, versionName) == 0)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
					 errmsg("FROM version must be different from installation target version \"%s\"",
							versionName)));

		updateVersions = identify_update_path(pcontrol,
											  oldVersionName,
											  versionName);

		if (list_length(updateVersions) == 1)
		{
			/*
			 * Simple case where there's just one update script to run. We
			 * will not need any follow-on update steps.
			 */
			Assert(strcmp((char *) linitial(updateVersions), versionName) == 0);
			updateVersions = NIL;
		}
		else
		{
			/*
			 * Multi-step sequence.  We treat this as installing the version
			 * that is the target of the first script, followed by successive
			 * updates to the later versions.
			 */
			versionName = (char *) linitial(updateVersions);
			updateVersions = list_delete_first(updateVersions);
		}
	}
	else
	{
		oldVersionName = NULL;
		updateVersions = NIL;
	}

	/*
	 * Fetch control parameters for installation target version
	 */
	control = read_extension_aux_control_file(pcontrol, versionName);

	/*
	 * Determine the target schema to install the extension into
	 */
	if (d_schema && d_schema->arg)
	{
		/*
		 * User given schema, CREATE EXTENSION ... WITH SCHEMA ...
		 *
		 * It's an error to give a schema different from control->schema if
		 * control->schema is specified.
		 */
		schemaName = strVal(d_schema->arg);

		if (control->schema != NULL &&
			strcmp(control->schema, schemaName) != 0)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("extension \"%s\" must be installed in schema \"%s\"",
					   control->name,
					   control->schema)));

		/* If the user is giving us the schema name, it must exist already */
		schemaOid = get_namespace_oid(schemaName, false);
	}
	else if (control->schema != NULL)
	{
		/*
		 * The extension is not relocatable and the author gave us a schema
		 * for it.	We create the schema here if it does not already exist.
		 */
		schemaName = control->schema;
		schemaOid = get_namespace_oid(schemaName, true);

		if (schemaOid == InvalidOid)
		{
			CreateSchemaStmt *csstmt = makeNode(CreateSchemaStmt);

			csstmt->schemaname = schemaName;
			csstmt->authid = NULL;		/* will be created by current user */
			csstmt->schemaElts = NIL;
			CreateSchemaCommand(csstmt, NULL);

			/*
			 * CreateSchemaCommand includes CommandCounterIncrement, so new
			 * schema is now visible
			 */
			schemaOid = get_namespace_oid(schemaName, false);
		}
	}
	else
	{
		/*
		 * Else, use the current default creation namespace, which is the
		 * first explicit entry in the search_path.
		 */
		List	   *search_path = fetch_search_path(false);

		if (search_path == NIL)	/* nothing valid in search_path? */
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_SCHEMA),
					 errmsg("no schema has been selected to create in")));
		schemaOid = linitial_oid(search_path);
		schemaName = get_namespace_name(schemaOid);
		if (schemaName == NULL) /* recently-deleted namespace? */
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_SCHEMA),
					 errmsg("no schema has been selected to create in")));

		list_free(search_path);
	}

	/*
	 * We don't check creation rights on the target namespace here.  If the
	 * extension script actually creates any objects there, it will fail if
	 * the user doesn't have such permissions.  But there are cases such as
	 * procedural languages where it's convenient to set schema = pg_catalog
	 * yet we don't want to restrict the command to users with ACL_CREATE for
	 * pg_catalog.
	 */

	/*
	 * Look up the prerequisite extensions, and build lists of their OIDs and
	 * the OIDs of their target schemas.
	 */
	requiredExtensions = NIL;
	requiredSchemas = NIL;
	foreach(lc, control->requires)
	{
		char	   *curreq = (char *) lfirst(lc);
		Oid			reqext;
		Oid			reqschema;

		/*
		 * We intentionally don't use get_extension_oid's default error
		 * message here, because it would be confusing in this context.
		 */
		reqext = get_extension_oid(curreq, true);
		if (!OidIsValid(reqext))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("required extension \"%s\" is not installed",
							curreq)));
		reqschema = get_extension_schema(reqext);
		requiredExtensions = lappend_oid(requiredExtensions, reqext);
		requiredSchemas = lappend_oid(requiredSchemas, reqschema);
	}

	/*
	 * Insert new tuple into pg_extension, and create dependency entries.
	 */
	extensionOid = InsertExtensionTuple(control->name, extowner,
										schemaOid, control->relocatable,
										versionName,
										PointerGetDatum(NULL),
										PointerGetDatum(NULL),
										requiredExtensions);

	/*
	 * Apply any control-file comment on extension
	 */
	if (control->comment != NULL)
		CreateComments(extensionOid, ExtensionRelationId, 0, control->comment);

	/*
	 * Execute the installation script file
	 */
	execute_extension_script(extensionOid, control,
							 oldVersionName, versionName,
							 requiredSchemas,
							 schemaName, schemaOid);

	/*
	 * If additional update scripts have to be executed, apply the updates as
	 * though a series of ALTER EXTENSION UPDATE commands were given
	 */
	ApplyExtensionUpdates(extensionOid, pcontrol,
						  versionName, updateVersions);
}

/*
 * InsertExtensionTuple
 *
 * Insert the new pg_extension row, and create extension's dependency entries.
 * Return the OID assigned to the new row.
 *
 * This is exported for the benefit of pg_upgrade, which has to create a
 * pg_extension entry (and the extension-level dependencies) without
 * actually running the extension's script.
 *
 * extConfig and extCondition should be arrays or PointerGetDatum(NULL).
 * We declare them as plain Datum to avoid needing array.h in extension.h.
 */
Oid
InsertExtensionTuple(const char *extName, Oid extOwner,
					 Oid schemaOid, bool relocatable, const char *extVersion,
					 Datum extConfig, Datum extCondition,
					 List *requiredExtensions)
{
	Oid			extensionOid;
	Relation	rel;
	Datum		values[Natts_pg_extension];
	bool		nulls[Natts_pg_extension];
	HeapTuple	tuple;
	ObjectAddress myself;
	ObjectAddress nsp;
	ListCell   *lc;

	/*
	 * Build and insert the pg_extension tuple
	 */
	rel = heap_open(ExtensionRelationId, RowExclusiveLock);

	memset(values, 0, sizeof(values));
	memset(nulls, 0, sizeof(nulls));

	values[Anum_pg_extension_extname - 1] =
		DirectFunctionCall1(namein, CStringGetDatum(extName));
	values[Anum_pg_extension_extowner - 1] = ObjectIdGetDatum(extOwner);
	values[Anum_pg_extension_extnamespace - 1] = ObjectIdGetDatum(schemaOid);
	values[Anum_pg_extension_extrelocatable - 1] = BoolGetDatum(relocatable);
	values[Anum_pg_extension_extversion - 1] = CStringGetTextDatum(extVersion);

	if (extConfig == PointerGetDatum(NULL))
		nulls[Anum_pg_extension_extconfig - 1] = true;
	else
		values[Anum_pg_extension_extconfig - 1] = extConfig;

	if (extCondition == PointerGetDatum(NULL))
		nulls[Anum_pg_extension_extcondition - 1] = true;
	else
		values[Anum_pg_extension_extcondition - 1] = extCondition;

	tuple = heap_form_tuple(rel->rd_att, values, nulls);

	extensionOid = simple_heap_insert(rel, tuple);
	CatalogUpdateIndexes(rel, tuple);

	heap_freetuple(tuple);
	heap_close(rel, RowExclusiveLock);

	/*
	 * Record dependencies on owner, schema, and prerequisite extensions
	 */
	recordDependencyOnOwner(ExtensionRelationId, extensionOid, extOwner);

	myself.classId = ExtensionRelationId;
	myself.objectId = extensionOid;
	myself.objectSubId = 0;

	nsp.classId = NamespaceRelationId;
	nsp.objectId = schemaOid;
	nsp.objectSubId = 0;

	recordDependencyOn(&myself, &nsp, DEPENDENCY_NORMAL);

	foreach(lc, requiredExtensions)
	{
		Oid			reqext = lfirst_oid(lc);
		ObjectAddress otherext;

		otherext.classId = ExtensionRelationId;
		otherext.objectId = reqext;
		otherext.objectSubId = 0;

		recordDependencyOn(&myself, &otherext, DEPENDENCY_NORMAL);
	}
	/* Post creation hook for new extension */
	InvokeObjectAccessHook(OAT_POST_CREATE,
						   ExtensionRelationId, extensionOid, 0);

	return extensionOid;
}


/*
 *	RemoveExtensions
 *		Implements DROP EXTENSION.
 */
void
RemoveExtensions(DropStmt *drop)
{
	ObjectAddresses *objects;
	ListCell   *cell;

	/*
	 * First we identify all the extensions, then we delete them in a single
	 * performMultipleDeletions() call.  This is to avoid unwanted DROP
	 * RESTRICT errors if one of the extensions depends on another.
	 */
	objects = new_object_addresses();

	foreach(cell, drop->objects)
	{
		List	   *names = (List *) lfirst(cell);
		char	   *extensionName;
		Oid			extensionId;
		ObjectAddress object;

		if (list_length(names) != 1)
			ereport(ERROR,
					(errcode(ERRCODE_SYNTAX_ERROR),
					 errmsg("extension name cannot be qualified")));
		extensionName = strVal(linitial(names));

		extensionId = get_extension_oid(extensionName, drop->missing_ok);

		if (!OidIsValid(extensionId))
		{
			ereport(NOTICE,
					(errmsg("extension \"%s\" does not exist, skipping",
							extensionName)));
			continue;
		}

		/* Permission check: must own extension */
		if (!pg_extension_ownercheck(extensionId, GetUserId()))
			aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_EXTENSION,
						   extensionName);

		object.classId = ExtensionRelationId;
		object.objectId = extensionId;
		object.objectSubId = 0;

		add_exact_object_address(&object, objects);
	}

	/*
	 * Do the deletions.  Objects contained in the extension(s) are removed by
	 * means of their dependency links to the extensions.
	 */
	performMultipleDeletions(objects, drop->behavior);

	free_object_addresses(objects);
}


/*
 * Guts of extension deletion.
 *
 * All we need do here is remove the pg_extension tuple itself.  Everything
 * else is taken care of by the dependency infrastructure.
 */
void
RemoveExtensionById(Oid extId)
{
	Relation	rel;
	SysScanDesc scandesc;
	HeapTuple	tuple;
	ScanKeyData entry[1];

	/*
	 * Disallow deletion of any extension that's currently open for insertion;
	 * else subsequent executions of recordDependencyOnCurrentExtension()
	 * could create dangling pg_depend records that refer to a no-longer-valid
	 * pg_extension OID.  This is needed not so much because we think people
	 * might write "DROP EXTENSION foo" in foo's own script files, as because
	 * errors in dependency management in extension script files could give
	 * rise to cases where an extension is dropped as a result of recursing
	 * from some contained object.  Because of that, we must test for the case
	 * here, not at some higher level of the DROP EXTENSION command.
	 */
	if (extId == CurrentExtensionObject)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("cannot drop extension \"%s\" because it is being modified",
						get_extension_name(extId))));

	rel = heap_open(ExtensionRelationId, RowExclusiveLock);

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(extId));
	scandesc = systable_beginscan(rel, ExtensionOidIndexId, true,
								  SnapshotNow, 1, entry);

	tuple = systable_getnext(scandesc);

	/* We assume that there can be at most one matching tuple */
	if (HeapTupleIsValid(tuple))
		simple_heap_delete(rel, &tuple->t_self);

	systable_endscan(scandesc);

	heap_close(rel, RowExclusiveLock);
}

/*
 * This function lists the available extensions (one row per primary control
 * file in the control directory).	We parse each control file and report the
 * interesting fields.
 *
 * The system view pg_available_extensions provides a user interface to this
 * SRF, adding information about whether the extensions are installed in the
 * current DB.
 */
Datum
pg_available_extensions(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	char	   *location;
	DIR		   *dir;
	struct dirent *de;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	/* Build tuplestore to hold the result rows */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	location = get_extension_control_directory();
	dir = AllocateDir(location);

	/*
	 * If the control directory doesn't exist, we want to silently return an
	 * empty set.  Any other error will be reported by ReadDir.
	 */
	if (dir == NULL && errno == ENOENT)
	{
		/* do nothing */
	}
	else
	{
		while ((de = ReadDir(dir, location)) != NULL)
		{
			ExtensionControlFile *control;
			char	   *extname;
			Datum		values[3];
			bool		nulls[3];

			if (!is_extension_control_filename(de->d_name))
				continue;

			/* extract extension name from 'name.control' filename */
			extname = pstrdup(de->d_name);
			*strrchr(extname, '.') = '\0';

			/* ignore it if it's an auxiliary control file */
			if (strstr(extname, "--"))
				continue;

			control = read_extension_control_file(extname);

			memset(values, 0, sizeof(values));
			memset(nulls, 0, sizeof(nulls));

			/* name */
			values[0] = DirectFunctionCall1(namein,
											CStringGetDatum(control->name));
			/* default_version */
			if (control->default_version == NULL)
				nulls[1] = true;
			else
				values[1] = CStringGetTextDatum(control->default_version);
			/* comment */
			if (control->comment == NULL)
				nulls[2] = true;
			else
				values[2] = CStringGetTextDatum(control->comment);

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}

		FreeDir(dir);
	}

	/* clean up and return the tuplestore */
	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * This function lists the available extension versions (one row per
 * extension installation script).	For each version, we parse the related
 * control file(s) and report the interesting fields.
 *
 * The system view pg_available_extension_versions provides a user interface
 * to this SRF, adding information about which versions are installed in the
 * current DB.
 */
Datum
pg_available_extension_versions(PG_FUNCTION_ARGS)
{
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	char	   *location;
	DIR		   *dir;
	struct dirent *de;

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	/* Build tuplestore to hold the result rows */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	location = get_extension_control_directory();
	dir = AllocateDir(location);

	/*
	 * If the control directory doesn't exist, we want to silently return an
	 * empty set.  Any other error will be reported by ReadDir.
	 */
	if (dir == NULL && errno == ENOENT)
	{
		/* do nothing */
	}
	else
	{
		while ((de = ReadDir(dir, location)) != NULL)
		{
			ExtensionControlFile *control;
			char	   *extname;

			if (!is_extension_control_filename(de->d_name))
				continue;

			/* extract extension name from 'name.control' filename */
			extname = pstrdup(de->d_name);
			*strrchr(extname, '.') = '\0';

			/* ignore it if it's an auxiliary control file */
			if (strstr(extname, "--"))
				continue;

			/* read the control file */
			control = read_extension_control_file(extname);

			/* scan extension's script directory for install scripts */
			get_available_versions_for_extension(control, tupstore, tupdesc);
		}

		FreeDir(dir);
	}

	/* clean up and return the tuplestore */
	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * Inner loop for pg_available_extension_versions:
 *		read versions of one extension, add rows to tupstore
 */
static void
get_available_versions_for_extension(ExtensionControlFile *pcontrol,
									 Tuplestorestate *tupstore,
									 TupleDesc tupdesc)
{
	int			extnamelen = strlen(pcontrol->name);
	char	   *location;
	DIR		   *dir;
	struct dirent *de;

	location = get_extension_script_directory(pcontrol);
	dir = AllocateDir(location);
	/* Note this will fail if script directory doesn't exist */
	while ((de = ReadDir(dir, location)) != NULL)
	{
		ExtensionControlFile *control;
		char	   *vername;
		Datum		values[7];
		bool		nulls[7];

		/* must be a .sql file ... */
		if (!is_extension_script_filename(de->d_name))
			continue;

		/* ... matching extension name followed by separator */
		if (strncmp(de->d_name, pcontrol->name, extnamelen) != 0 ||
			de->d_name[extnamelen] != '-' ||
			de->d_name[extnamelen + 1] != '-')
			continue;

		/* extract version name from 'extname--something.sql' filename */
		vername = pstrdup(de->d_name + extnamelen + 2);
		*strrchr(vername, '.') = '\0';

		/* ignore it if it's an update script */
		if (strstr(vername, "--"))
			continue;

		/*
		 * Fetch parameters for specific version (pcontrol is not changed)
		 */
		control = read_extension_aux_control_file(pcontrol, vername);

		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));

		/* name */
		values[0] = DirectFunctionCall1(namein,
										CStringGetDatum(control->name));
		/* version */
		values[1] = CStringGetTextDatum(vername);
		/* superuser */
		values[2] = BoolGetDatum(control->superuser);
		/* relocatable */
		values[3] = BoolGetDatum(control->relocatable);
		/* schema */
		if (control->schema == NULL)
			nulls[4] = true;
		else
			values[4] = DirectFunctionCall1(namein,
											CStringGetDatum(control->schema));
		/* requires */
		if (control->requires == NIL)
			nulls[5] = true;
		else
		{
			Datum	   *datums;
			int			ndatums;
			ArrayType  *a;
			ListCell   *lc;

			ndatums = list_length(control->requires);
			datums = (Datum *) palloc(ndatums * sizeof(Datum));
			ndatums = 0;
			foreach(lc, control->requires)
			{
				char	   *curreq = (char *) lfirst(lc);

				datums[ndatums++] =
					DirectFunctionCall1(namein, CStringGetDatum(curreq));
			}
			a = construct_array(datums, ndatums,
								NAMEOID,
								NAMEDATALEN, false, 'c');
			values[5] = PointerGetDatum(a);
		}
		/* comment */
		if (control->comment == NULL)
			nulls[6] = true;
		else
			values[6] = CStringGetTextDatum(control->comment);

		tuplestore_putvalues(tupstore, tupdesc, values, nulls);
	}

	FreeDir(dir);
}

/*
 * This function reports the version update paths that exist for the
 * specified extension.
 */
Datum
pg_extension_update_paths(PG_FUNCTION_ARGS)
{
	Name		extname = PG_GETARG_NAME(0);
	ReturnSetInfo *rsinfo = (ReturnSetInfo *) fcinfo->resultinfo;
	TupleDesc	tupdesc;
	Tuplestorestate *tupstore;
	MemoryContext per_query_ctx;
	MemoryContext oldcontext;
	List	   *evi_list;
	ExtensionControlFile *control;
	ListCell   *lc1;

	/* Check extension name validity before any filesystem access */
	check_valid_extension_name(NameStr(*extname));

	/* check to see if caller supports us returning a tuplestore */
	if (rsinfo == NULL || !IsA(rsinfo, ReturnSetInfo))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("set-valued function called in context that cannot accept a set")));
	if (!(rsinfo->allowedModes & SFRM_Materialize))
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("materialize mode required, but it is not " \
						"allowed in this context")));

	/* Build a tuple descriptor for our result type */
	if (get_call_result_type(fcinfo, NULL, &tupdesc) != TYPEFUNC_COMPOSITE)
		elog(ERROR, "return type must be a row type");

	/* Build tuplestore to hold the result rows */
	per_query_ctx = rsinfo->econtext->ecxt_per_query_memory;
	oldcontext = MemoryContextSwitchTo(per_query_ctx);

	tupstore = tuplestore_begin_heap(true, false, work_mem);
	rsinfo->returnMode = SFRM_Materialize;
	rsinfo->setResult = tupstore;
	rsinfo->setDesc = tupdesc;

	MemoryContextSwitchTo(oldcontext);

	/* Read the extension's control file */
	control = read_extension_control_file(NameStr(*extname));

	/* Extract the version update graph from the script directory */
	evi_list = get_ext_ver_list(control);

	/* Iterate over all pairs of versions */
	foreach(lc1, evi_list)
	{
		ExtensionVersionInfo *evi1 = (ExtensionVersionInfo *) lfirst(lc1);
		ListCell   *lc2;

		foreach(lc2, evi_list)
		{
			ExtensionVersionInfo *evi2 = (ExtensionVersionInfo *) lfirst(lc2);
			List	   *path;
			Datum		values[3];
			bool		nulls[3];

			if (evi1 == evi2)
				continue;

			/* Find shortest path from evi1 to evi2 */
			path = find_update_path(evi_list, evi1, evi2, true);

			/* Emit result row */
			memset(values, 0, sizeof(values));
			memset(nulls, 0, sizeof(nulls));

			/* source */
			values[0] = CStringGetTextDatum(evi1->name);
			/* target */
			values[1] = CStringGetTextDatum(evi2->name);
			/* path */
			if (path == NIL)
				nulls[2] = true;
			else
			{
				StringInfoData pathbuf;
				ListCell   *lcv;

				initStringInfo(&pathbuf);
				/* The path doesn't include start vertex, but show it */
				appendStringInfoString(&pathbuf, evi1->name);
				foreach(lcv, path)
				{
					char	   *versionName = (char *) lfirst(lcv);

					appendStringInfoString(&pathbuf, "--");
					appendStringInfoString(&pathbuf, versionName);
				}
				values[2] = CStringGetTextDatum(pathbuf.data);
				pfree(pathbuf.data);
			}

			tuplestore_putvalues(tupstore, tupdesc, values, nulls);
		}
	}

	/* clean up and return the tuplestore */
	tuplestore_donestoring(tupstore);

	return (Datum) 0;
}

/*
 * pg_extension_config_dump
 *
 * Record information about a configuration table that belongs to an
 * extension being created, but whose contents should be dumped in whole
 * or in part during pg_dump.
 */
Datum
pg_extension_config_dump(PG_FUNCTION_ARGS)
{
	Oid			tableoid = PG_GETARG_OID(0);
	text	   *wherecond = PG_GETARG_TEXT_P(1);
	char	   *tablename;
	Relation	extRel;
	ScanKeyData key[1];
	SysScanDesc extScan;
	HeapTuple	extTup;
	Datum		arrayDatum;
	Datum		elementDatum;
	int			arrayLength;
	int			arrayIndex;
	bool		isnull;
	Datum		repl_val[Natts_pg_extension];
	bool		repl_null[Natts_pg_extension];
	bool		repl_repl[Natts_pg_extension];
	ArrayType  *a;

	/*
	 * We only allow this to be called from an extension's SQL script. We
	 * shouldn't need any permissions check beyond that.
	 */
	if (!creating_extension)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("pg_extension_config_dump() can only be called "
						"from an SQL script executed by CREATE EXTENSION")));

	/*
	 * Check that the table exists and is a member of the extension being
	 * created.  This ensures that we don't need to register an additional
	 * dependency to protect the extconfig entry.
	 */
	tablename = get_rel_name(tableoid);
	if (tablename == NULL)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_TABLE),
				 errmsg("OID %u does not refer to a table", tableoid)));
	if (getExtensionOfObject(RelationRelationId, tableoid) !=
		CurrentExtensionObject)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
		errmsg("table \"%s\" is not a member of the extension being created",
			   tablename)));

	/*
	 * Add the table OID and WHERE condition to the extension's extconfig and
	 * extcondition arrays.
	 *
	 * If the table is already in extconfig, treat this as an update of the
	 * WHERE condition.
	 */

	/* Find the pg_extension tuple */
	extRel = heap_open(ExtensionRelationId, RowExclusiveLock);

	ScanKeyInit(&key[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(CurrentExtensionObject));

	extScan = systable_beginscan(extRel, ExtensionOidIndexId, true,
								 SnapshotNow, 1, key);

	extTup = systable_getnext(extScan);

	if (!HeapTupleIsValid(extTup))		/* should not happen */
		elog(ERROR, "extension with oid %u does not exist",
			 CurrentExtensionObject);

	memset(repl_val, 0, sizeof(repl_val));
	memset(repl_null, false, sizeof(repl_null));
	memset(repl_repl, false, sizeof(repl_repl));

	/* Build or modify the extconfig value */
	elementDatum = ObjectIdGetDatum(tableoid);

	arrayDatum = heap_getattr(extTup, Anum_pg_extension_extconfig,
							  RelationGetDescr(extRel), &isnull);
	if (isnull)
	{
		/* Previously empty extconfig, so build 1-element array */
		arrayLength = 0;
		arrayIndex = 1;

		a = construct_array(&elementDatum, 1,
							OIDOID,
							sizeof(Oid), true, 'i');
	}
	else
	{
		/* Modify or extend existing extconfig array */
		Oid		   *arrayData;
		int			i;

		a = DatumGetArrayTypeP(arrayDatum);

		arrayLength = ARR_DIMS(a)[0];
		if (ARR_NDIM(a) != 1 ||
			ARR_LBOUND(a)[0] != 1 ||
			arrayLength < 0 ||
			ARR_HASNULL(a) ||
			ARR_ELEMTYPE(a) != OIDOID)
			elog(ERROR, "extconfig is not a 1-D Oid array");
		arrayData = (Oid *) ARR_DATA_PTR(a);

		arrayIndex = arrayLength + 1;	/* set up to add after end */

		for (i = 0; i < arrayLength; i++)
		{
			if (arrayData[i] == tableoid)
			{
				arrayIndex = i + 1;		/* replace this element instead */
				break;
			}
		}

		a = array_set(a, 1, &arrayIndex,
					  elementDatum,
					  false,
					  -1 /* varlena array */ ,
					  sizeof(Oid) /* OID's typlen */ ,
					  true /* OID's typbyval */ ,
					  'i' /* OID's typalign */ );
	}
	repl_val[Anum_pg_extension_extconfig - 1] = PointerGetDatum(a);
	repl_repl[Anum_pg_extension_extconfig - 1] = true;

	/* Build or modify the extcondition value */
	elementDatum = PointerGetDatum(wherecond);

	arrayDatum = heap_getattr(extTup, Anum_pg_extension_extcondition,
							  RelationGetDescr(extRel), &isnull);
	if (isnull)
	{
		if (arrayLength != 0)
			elog(ERROR, "extconfig and extcondition arrays do not match");

		a = construct_array(&elementDatum, 1,
							TEXTOID,
							-1, false, 'i');
	}
	else
	{
		a = DatumGetArrayTypeP(arrayDatum);

		if (ARR_NDIM(a) != 1 ||
			ARR_LBOUND(a)[0] != 1 ||
			ARR_HASNULL(a) ||
			ARR_ELEMTYPE(a) != TEXTOID)
			elog(ERROR, "extcondition is not a 1-D text array");
		if (ARR_DIMS(a)[0] != arrayLength)
			elog(ERROR, "extconfig and extcondition arrays do not match");

		/* Add or replace at same index as in extconfig */
		a = array_set(a, 1, &arrayIndex,
					  elementDatum,
					  false,
					  -1 /* varlena array */ ,
					  -1 /* TEXT's typlen */ ,
					  false /* TEXT's typbyval */ ,
					  'i' /* TEXT's typalign */ );
	}
	repl_val[Anum_pg_extension_extcondition - 1] = PointerGetDatum(a);
	repl_repl[Anum_pg_extension_extcondition - 1] = true;

	extTup = heap_modify_tuple(extTup, RelationGetDescr(extRel),
							   repl_val, repl_null, repl_repl);

	simple_heap_update(extRel, &extTup->t_self, extTup);
	CatalogUpdateIndexes(extRel, extTup);

	systable_endscan(extScan);

	heap_close(extRel, RowExclusiveLock);

	PG_RETURN_VOID();
}

/*
 * extension_config_remove
 *
 * Remove the specified table OID from extension's extconfig, if present.
 * This is not currently exposed as a function, but it could be;
 * for now, we just invoke it from ALTER EXTENSION DROP.
 */
static void
extension_config_remove(Oid extensionoid, Oid tableoid)
{
	Relation	extRel;
	ScanKeyData key[1];
	SysScanDesc extScan;
	HeapTuple	extTup;
	Datum		arrayDatum;
	int			arrayLength;
	int			arrayIndex;
	bool		isnull;
	Datum		repl_val[Natts_pg_extension];
	bool		repl_null[Natts_pg_extension];
	bool		repl_repl[Natts_pg_extension];
	ArrayType  *a;

	/* Find the pg_extension tuple */
	extRel = heap_open(ExtensionRelationId, RowExclusiveLock);

	ScanKeyInit(&key[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(extensionoid));

	extScan = systable_beginscan(extRel, ExtensionOidIndexId, true,
								 SnapshotNow, 1, key);

	extTup = systable_getnext(extScan);

	if (!HeapTupleIsValid(extTup))		/* should not happen */
		elog(ERROR, "extension with oid %u does not exist",
			 extensionoid);

	/* Search extconfig for the tableoid */
	arrayDatum = heap_getattr(extTup, Anum_pg_extension_extconfig,
							  RelationGetDescr(extRel), &isnull);
	if (isnull)
	{
		/* nothing to do */
		a = NULL;
		arrayLength = 0;
		arrayIndex = -1;
	}
	else
	{
		Oid		   *arrayData;
		int			i;

		a = DatumGetArrayTypeP(arrayDatum);

		arrayLength = ARR_DIMS(a)[0];
		if (ARR_NDIM(a) != 1 ||
			ARR_LBOUND(a)[0] != 1 ||
			arrayLength < 0 ||
			ARR_HASNULL(a) ||
			ARR_ELEMTYPE(a) != OIDOID)
			elog(ERROR, "extconfig is not a 1-D Oid array");
		arrayData = (Oid *) ARR_DATA_PTR(a);

		arrayIndex = -1;		/* flag for no deletion needed */

		for (i = 0; i < arrayLength; i++)
		{
			if (arrayData[i] == tableoid)
			{
				arrayIndex = i; /* index to remove */
				break;
			}
		}
	}

	/* If tableoid is not in extconfig, nothing to do */
	if (arrayIndex < 0)
	{
		systable_endscan(extScan);
		heap_close(extRel, RowExclusiveLock);
		return;
	}

	/* Modify or delete the extconfig value */
	memset(repl_val, 0, sizeof(repl_val));
	memset(repl_null, false, sizeof(repl_null));
	memset(repl_repl, false, sizeof(repl_repl));

	if (arrayLength <= 1)
	{
		/* removing only element, just set array to null */
		repl_null[Anum_pg_extension_extconfig - 1] = true;
	}
	else
	{
		/* squeeze out the target element */
		Datum	   *dvalues;
		bool	   *dnulls;
		int			nelems;
		int			i;

		deconstruct_array(a, OIDOID, sizeof(Oid), true, 'i',
						  &dvalues, &dnulls, &nelems);

		/* We already checked there are no nulls, so ignore dnulls */
		for (i = arrayIndex; i < arrayLength - 1; i++)
			dvalues[i] = dvalues[i + 1];

		a = construct_array(dvalues, arrayLength - 1,
							OIDOID, sizeof(Oid), true, 'i');

		repl_val[Anum_pg_extension_extconfig - 1] = PointerGetDatum(a);
	}
	repl_repl[Anum_pg_extension_extconfig - 1] = true;

	/* Modify or delete the extcondition value */
	arrayDatum = heap_getattr(extTup, Anum_pg_extension_extcondition,
							  RelationGetDescr(extRel), &isnull);
	if (isnull)
	{
		elog(ERROR, "extconfig and extcondition arrays do not match");
	}
	else
	{
		a = DatumGetArrayTypeP(arrayDatum);

		if (ARR_NDIM(a) != 1 ||
			ARR_LBOUND(a)[0] != 1 ||
			ARR_HASNULL(a) ||
			ARR_ELEMTYPE(a) != TEXTOID)
			elog(ERROR, "extcondition is not a 1-D text array");
		if (ARR_DIMS(a)[0] != arrayLength)
			elog(ERROR, "extconfig and extcondition arrays do not match");
	}

	if (arrayLength <= 1)
	{
		/* removing only element, just set array to null */
		repl_null[Anum_pg_extension_extcondition - 1] = true;
	}
	else
	{
		/* squeeze out the target element */
		Datum	   *dvalues;
		bool	   *dnulls;
		int			nelems;
		int			i;

		deconstruct_array(a, TEXTOID, -1, false, 'i',
						  &dvalues, &dnulls, &nelems);

		/* We already checked there are no nulls, so ignore dnulls */
		for (i = arrayIndex; i < arrayLength - 1; i++)
			dvalues[i] = dvalues[i + 1];

		a = construct_array(dvalues, arrayLength - 1,
							TEXTOID, -1, false, 'i');

		repl_val[Anum_pg_extension_extcondition - 1] = PointerGetDatum(a);
	}
	repl_repl[Anum_pg_extension_extcondition - 1] = true;

	extTup = heap_modify_tuple(extTup, RelationGetDescr(extRel),
							   repl_val, repl_null, repl_repl);

	simple_heap_update(extRel, &extTup->t_self, extTup);
	CatalogUpdateIndexes(extRel, extTup);

	systable_endscan(extScan);

	heap_close(extRel, RowExclusiveLock);
}

/*
 * Execute ALTER EXTENSION SET SCHEMA
 */
void
AlterExtensionNamespace(List *names, const char *newschema)
{
	char	   *extensionName;
	Oid			extensionOid;
	Oid			nspOid;
	Oid			oldNspOid = InvalidOid;
	AclResult	aclresult;
	Relation	extRel;
	ScanKeyData key[2];
	SysScanDesc extScan;
	HeapTuple	extTup;
	Form_pg_extension extForm;
	Relation	depRel;
	SysScanDesc depScan;
	HeapTuple	depTup;
	ObjectAddresses *objsMoved;

	if (list_length(names) != 1)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("extension name cannot be qualified")));
	extensionName = strVal(linitial(names));

	extensionOid = get_extension_oid(extensionName, false);

	nspOid = LookupCreationNamespace(newschema);

	/*
	 * Permission check: must own extension.  Note that we don't bother to
	 * check ownership of the individual member objects ...
	 */
	if (!pg_extension_ownercheck(extensionOid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_EXTENSION,
					   extensionName);

	/* Permission check: must have creation rights in target namespace */
	aclresult = pg_namespace_aclcheck(nspOid, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE, newschema);

	/*
	 * If the schema is currently a member of the extension, disallow moving
	 * the extension into the schema.  That would create a dependency loop.
	 */
	if (getExtensionOfObject(NamespaceRelationId, nspOid) == extensionOid)
		ereport(ERROR,
				(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
				 errmsg("cannot move extension \"%s\" into schema \"%s\" "
						"because the extension contains the schema",
						extensionName, newschema)));

	/* Locate the pg_extension tuple */
	extRel = heap_open(ExtensionRelationId, RowExclusiveLock);

	ScanKeyInit(&key[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(extensionOid));

	extScan = systable_beginscan(extRel, ExtensionOidIndexId, true,
								 SnapshotNow, 1, key);

	extTup = systable_getnext(extScan);

	if (!HeapTupleIsValid(extTup))		/* should not happen */
		elog(ERROR, "extension with oid %u does not exist", extensionOid);

	/* Copy tuple so we can modify it below */
	extTup = heap_copytuple(extTup);
	extForm = (Form_pg_extension) GETSTRUCT(extTup);

	systable_endscan(extScan);

	/*
	 * If the extension is already in the target schema, just silently do
	 * nothing.
	 */
	if (extForm->extnamespace == nspOid)
	{
		heap_close(extRel, RowExclusiveLock);
		return;
	}

	/* Check extension is supposed to be relocatable */
	if (!extForm->extrelocatable)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("extension \"%s\" does not support SET SCHEMA",
						NameStr(extForm->extname))));

	objsMoved = new_object_addresses();

	/*
	 * Scan pg_depend to find objects that depend directly on the extension,
	 * and alter each one's schema.
	 */
	depRel = heap_open(DependRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_depend_refclassid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(ExtensionRelationId));
	ScanKeyInit(&key[1],
				Anum_pg_depend_refobjid,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(extensionOid));

	depScan = systable_beginscan(depRel, DependReferenceIndexId, true,
								 SnapshotNow, 2, key);

	while (HeapTupleIsValid(depTup = systable_getnext(depScan)))
	{
		Form_pg_depend pg_depend = (Form_pg_depend) GETSTRUCT(depTup);
		ObjectAddress dep;
		Oid			dep_oldNspOid;

		/*
		 * Ignore non-membership dependencies.	(Currently, the only other
		 * case we could see here is a normal dependency from another
		 * extension.)
		 */
		if (pg_depend->deptype != DEPENDENCY_EXTENSION)
			continue;

		dep.classId = pg_depend->classid;
		dep.objectId = pg_depend->objid;
		dep.objectSubId = pg_depend->objsubid;

		if (dep.objectSubId != 0)		/* should not happen */
			elog(ERROR, "extension should not have a sub-object dependency");

		/* Relocate the object */
		dep_oldNspOid = AlterObjectNamespace_oid(dep.classId,
												 dep.objectId,
												 nspOid,
												 objsMoved);

		/*
		 * Remember previous namespace of first object that has one
		 */
		if (oldNspOid == InvalidOid && dep_oldNspOid != InvalidOid)
			oldNspOid = dep_oldNspOid;

		/*
		 * If not all the objects had the same old namespace (ignoring any
		 * that are not in namespaces), complain.
		 */
		if (dep_oldNspOid != InvalidOid && dep_oldNspOid != oldNspOid)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					 errmsg("extension \"%s\" does not support SET SCHEMA",
							NameStr(extForm->extname)),
					 errdetail("%s is not in the extension's schema \"%s\"",
							   getObjectDescription(&dep),
							   get_namespace_name(oldNspOid))));
	}

	systable_endscan(depScan);

	relation_close(depRel, AccessShareLock);

	/* Now adjust pg_extension.extnamespace */
	extForm->extnamespace = nspOid;

	simple_heap_update(extRel, &extTup->t_self, extTup);
	CatalogUpdateIndexes(extRel, extTup);

	heap_close(extRel, RowExclusiveLock);

	/* update dependencies to point to the new schema */
	changeDependencyFor(ExtensionRelationId, extensionOid,
						NamespaceRelationId, oldNspOid, nspOid);
}

/*
 * Execute ALTER EXTENSION UPDATE
 */
void
ExecAlterExtensionStmt(AlterExtensionStmt *stmt)
{
	DefElem    *d_new_version = NULL;
	char	   *versionName;
	char	   *oldVersionName;
	ExtensionControlFile *control;
	Oid			extensionOid;
	Relation	extRel;
	ScanKeyData key[1];
	SysScanDesc extScan;
	HeapTuple	extTup;
	List	   *updateVersions;
	Datum		datum;
	bool		isnull;
	ListCell   *lc;

	/*
	 * We use global variables to track the extension being created, so we can
	 * create/update only one extension at the same time.
	 */
	if (creating_extension)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				 errmsg("nested ALTER EXTENSION is not supported")));

	/*
	 * Look up the extension --- it must already exist in pg_extension
	 */
	extRel = heap_open(ExtensionRelationId, AccessShareLock);

	ScanKeyInit(&key[0],
				Anum_pg_extension_extname,
				BTEqualStrategyNumber, F_NAMEEQ,
				CStringGetDatum(stmt->extname));

	extScan = systable_beginscan(extRel, ExtensionNameIndexId, true,
								 SnapshotNow, 1, key);

	extTup = systable_getnext(extScan);

	if (!HeapTupleIsValid(extTup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("extension \"%s\" does not exist",
						stmt->extname)));

	extensionOid = HeapTupleGetOid(extTup);

	/*
	 * Determine the existing version we are updating from
	 */
	datum = heap_getattr(extTup, Anum_pg_extension_extversion,
						 RelationGetDescr(extRel), &isnull);
	if (isnull)
		elog(ERROR, "extversion is null");
	oldVersionName = text_to_cstring(DatumGetTextPP(datum));

	systable_endscan(extScan);

	heap_close(extRel, AccessShareLock);

	/* Permission check: must own extension */
	if (!pg_extension_ownercheck(extensionOid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_EXTENSION,
					   stmt->extname);

	/*
	 * Read the primary control file.  Note we assume that it does not contain
	 * any non-ASCII data, so there is no need to worry about encoding at this
	 * point.
	 */
	control = read_extension_control_file(stmt->extname);

	/*
	 * Read the statement option list
	 */
	foreach(lc, stmt->options)
	{
		DefElem    *defel = (DefElem *) lfirst(lc);

		if (strcmp(defel->defname, "new_version") == 0)
		{
			if (d_new_version)
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("conflicting or redundant options")));
			d_new_version = defel;
		}
		else
			elog(ERROR, "unrecognized option: %s", defel->defname);
	}

	/*
	 * Determine the version to update to
	 */
	if (d_new_version && d_new_version->arg)
		versionName = strVal(d_new_version->arg);
	else if (control->default_version)
		versionName = control->default_version;
	else
	{
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_PARAMETER_VALUE),
				 errmsg("version to install must be specified")));
		versionName = NULL;		/* keep compiler quiet */
	}
	check_valid_version_name(versionName);

	/*
	 * If we're already at that version, just say so
	 */
	if (strcmp(oldVersionName, versionName) == 0)
	{
		ereport(NOTICE,
		   (errmsg("version \"%s\" of extension \"%s\" is already installed",
				   versionName, stmt->extname)));
		return;
	}

	/*
	 * Identify the series of update script files we need to execute
	 */
	updateVersions = identify_update_path(control,
										  oldVersionName,
										  versionName);

	/*
	 * Update the pg_extension row and execute the update scripts, one at a
	 * time
	 */
	ApplyExtensionUpdates(extensionOid, control,
						  oldVersionName, updateVersions);
}

/*
 * Apply a series of update scripts as though individual ALTER EXTENSION
 * UPDATE commands had been given, including altering the pg_extension row
 * and dependencies each time.
 *
 * This might be more work than necessary, but it ensures that old update
 * scripts don't break if newer versions have different control parameters.
 */
static void
ApplyExtensionUpdates(Oid extensionOid,
					  ExtensionControlFile *pcontrol,
					  const char *initialVersion,
					  List *updateVersions)
{
	const char *oldVersionName = initialVersion;
	ListCell   *lcv;

	foreach(lcv, updateVersions)
	{
		char	   *versionName = (char *) lfirst(lcv);
		ExtensionControlFile *control;
		char	   *schemaName;
		Oid			schemaOid;
		List	   *requiredExtensions;
		List	   *requiredSchemas;
		Relation	extRel;
		ScanKeyData key[1];
		SysScanDesc extScan;
		HeapTuple	extTup;
		Form_pg_extension extForm;
		Datum		values[Natts_pg_extension];
		bool		nulls[Natts_pg_extension];
		bool		repl[Natts_pg_extension];
		ObjectAddress myself;
		ListCell   *lc;

		/*
		 * Fetch parameters for specific version (pcontrol is not changed)
		 */
		control = read_extension_aux_control_file(pcontrol, versionName);

		/* Find the pg_extension tuple */
		extRel = heap_open(ExtensionRelationId, RowExclusiveLock);

		ScanKeyInit(&key[0],
					ObjectIdAttributeNumber,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(extensionOid));

		extScan = systable_beginscan(extRel, ExtensionOidIndexId, true,
									 SnapshotNow, 1, key);

		extTup = systable_getnext(extScan);

		if (!HeapTupleIsValid(extTup))	/* should not happen */
			elog(ERROR, "extension with oid %u does not exist",
				 extensionOid);

		extForm = (Form_pg_extension) GETSTRUCT(extTup);

		/*
		 * Determine the target schema (set by original install)
		 */
		schemaOid = extForm->extnamespace;
		schemaName = get_namespace_name(schemaOid);

		/*
		 * Modify extrelocatable and extversion in the pg_extension tuple
		 */
		memset(values, 0, sizeof(values));
		memset(nulls, 0, sizeof(nulls));
		memset(repl, 0, sizeof(repl));

		values[Anum_pg_extension_extrelocatable - 1] =
			BoolGetDatum(control->relocatable);
		repl[Anum_pg_extension_extrelocatable - 1] = true;
		values[Anum_pg_extension_extversion - 1] =
			CStringGetTextDatum(versionName);
		repl[Anum_pg_extension_extversion - 1] = true;

		extTup = heap_modify_tuple(extTup, RelationGetDescr(extRel),
								   values, nulls, repl);

		simple_heap_update(extRel, &extTup->t_self, extTup);
		CatalogUpdateIndexes(extRel, extTup);

		systable_endscan(extScan);

		heap_close(extRel, RowExclusiveLock);

		/*
		 * Look up the prerequisite extensions for this version, and build
		 * lists of their OIDs and the OIDs of their target schemas.
		 */
		requiredExtensions = NIL;
		requiredSchemas = NIL;
		foreach(lc, control->requires)
		{
			char	   *curreq = (char *) lfirst(lc);
			Oid			reqext;
			Oid			reqschema;

			/*
			 * We intentionally don't use get_extension_oid's default error
			 * message here, because it would be confusing in this context.
			 */
			reqext = get_extension_oid(curreq, true);
			if (!OidIsValid(reqext))
				ereport(ERROR,
						(errcode(ERRCODE_UNDEFINED_OBJECT),
						 errmsg("required extension \"%s\" is not installed",
								curreq)));
			reqschema = get_extension_schema(reqext);
			requiredExtensions = lappend_oid(requiredExtensions, reqext);
			requiredSchemas = lappend_oid(requiredSchemas, reqschema);
		}

		/*
		 * Remove and recreate dependencies on prerequisite extensions
		 */
		deleteDependencyRecordsForClass(ExtensionRelationId, extensionOid,
										ExtensionRelationId,
										DEPENDENCY_NORMAL);

		myself.classId = ExtensionRelationId;
		myself.objectId = extensionOid;
		myself.objectSubId = 0;

		foreach(lc, requiredExtensions)
		{
			Oid			reqext = lfirst_oid(lc);
			ObjectAddress otherext;

			otherext.classId = ExtensionRelationId;
			otherext.objectId = reqext;
			otherext.objectSubId = 0;

			recordDependencyOn(&myself, &otherext, DEPENDENCY_NORMAL);
		}

		/*
		 * Finally, execute the update script file
		 */
		execute_extension_script(extensionOid, control,
								 oldVersionName, versionName,
								 requiredSchemas,
								 schemaName, schemaOid);

		/*
		 * Update prior-version name and loop around.  Since
		 * execute_sql_string did a final CommandCounterIncrement, we can
		 * update the pg_extension row again.
		 */
		oldVersionName = versionName;
	}
}

/*
 * Execute ALTER EXTENSION ADD/DROP
 */
void
ExecAlterExtensionContentsStmt(AlterExtensionContentsStmt *stmt)
{
	ObjectAddress extension;
	ObjectAddress object;
	Relation	relation;
	Oid			oldExtension;

	extension.classId = ExtensionRelationId;
	extension.objectId = get_extension_oid(stmt->extname, false);
	extension.objectSubId = 0;

	/* Permission check: must own extension */
	if (!pg_extension_ownercheck(extension.objectId, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_EXTENSION,
					   stmt->extname);

	/*
	 * Translate the parser representation that identifies the object into an
	 * ObjectAddress.  get_object_address() will throw an error if the object
	 * does not exist, and will also acquire a lock on the object to guard
	 * against concurrent DROP and ALTER EXTENSION ADD/DROP operations.
	 */
	object = get_object_address(stmt->objtype, stmt->objname, stmt->objargs,
								&relation, ShareUpdateExclusiveLock);

	/* Permission check: must own target object, too */
	check_object_ownership(GetUserId(), stmt->objtype, object,
						   stmt->objname, stmt->objargs, relation);

	/*
	 * Check existing extension membership.
	 */
	oldExtension = getExtensionOfObject(object.classId, object.objectId);

	if (stmt->action > 0)
	{
		/*
		 * ADD, so complain if object is already attached to some extension.
		 */
		if (OidIsValid(oldExtension))
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					 errmsg("%s is already a member of extension \"%s\"",
							getObjectDescription(&object),
							get_extension_name(oldExtension))));

		/*
		 * Prevent a schema from being added to an extension if the schema
		 * contains the extension.	That would create a dependency loop.
		 */
		if (object.classId == NamespaceRelationId &&
			object.objectId == get_extension_schema(extension.objectId))
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					 errmsg("cannot add schema \"%s\" to extension \"%s\" "
							"because the schema contains the extension",
							get_namespace_name(object.objectId),
							stmt->extname)));

		/*
		 * OK, add the dependency.
		 */
		recordDependencyOn(&object, &extension, DEPENDENCY_EXTENSION);
	}
	else
	{
		/*
		 * DROP, so complain if it's not a member.
		 */
		if (oldExtension != extension.objectId)
			ereport(ERROR,
					(errcode(ERRCODE_OBJECT_NOT_IN_PREREQUISITE_STATE),
					 errmsg("%s is not a member of extension \"%s\"",
							getObjectDescription(&object),
							stmt->extname)));

		/*
		 * OK, drop the dependency.
		 */
		if (deleteDependencyRecordsForClass(object.classId, object.objectId,
											ExtensionRelationId,
											DEPENDENCY_EXTENSION) != 1)
			elog(ERROR, "unexpected number of extension dependency records");

		/*
		 * If it's a relation, it might have an entry in the extension's
		 * extconfig array, which we must remove.
		 */
		if (object.classId == RelationRelationId)
			extension_config_remove(extension.objectId, object.objectId);
	}

	/*
	 * If get_object_address() opened the relation for us, we close it to keep
	 * the reference count correct - but we retain any locks acquired by
	 * get_object_address() until commit time, to guard against concurrent
	 * activity.
	 */
	if (relation != NULL)
		relation_close(relation, NoLock);
}

/*
 * AlterExtensionOwner_internal
 *
 * Internal routine for changing the owner of an extension.  rel must be
 * pg_extension, already open and suitably locked; it will not be closed.
 *
 * Note that this only changes ownership of the extension itself; it doesn't
 * change the ownership of objects it contains.  Since this function is
 * currently only called from REASSIGN OWNED, this restriction is okay because
 * said objects would also be affected by our caller.  But it's not enough for
 * a full-fledged ALTER OWNER implementation, so beware.
 */
static void
AlterExtensionOwner_internal(Relation rel, Oid extensionOid, Oid newOwnerId)
{
	Form_pg_extension extForm;
	HeapTuple	tup;
	SysScanDesc scandesc;
	ScanKeyData entry[1];

	Assert(RelationGetRelid(rel) == ExtensionRelationId);

	ScanKeyInit(&entry[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(extensionOid));

	scandesc = systable_beginscan(rel, ExtensionOidIndexId, true,
								  SnapshotNow, 1, entry);

	/* We assume that there can be at most one matching tuple */
	tup = systable_getnext(scandesc);
	if (!HeapTupleIsValid(tup)) /* should not happen */
		elog(ERROR, "cache lookup failed for extension %u", extensionOid);

	tup = heap_copytuple(tup);
	systable_endscan(scandesc);

	extForm = (Form_pg_extension) GETSTRUCT(tup);

	/*
	 * If the new owner is the same as the existing owner, consider the
	 * command to have succeeded.  This is for dump restoration purposes.
	 */
	if (extForm->extowner != newOwnerId)
	{
		/* Superusers can always do it */
		if (!superuser())
		{
			/* Otherwise, must be owner of the existing object */
			if (!pg_extension_ownercheck(HeapTupleGetOid(tup), GetUserId()))
				aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_EXTENSION,
							   NameStr(extForm->extname));

			/* Must be able to become new owner */
			check_is_member_of_role(GetUserId(), newOwnerId);

			/* no privilege checks on namespace are required */
		}

		/*
		 * Modify the owner --- okay to scribble on tup because it's a copy
		 */
		extForm->extowner = newOwnerId;

		simple_heap_update(rel, &tup->t_self, tup);

		CatalogUpdateIndexes(rel, tup);

		/* Update owner dependency reference */
		changeDependencyOnOwner(ExtensionRelationId, extensionOid,
								newOwnerId);
	}

	heap_freetuple(tup);
}

/*
 * Change extension owner, by OID
 */
void
AlterExtensionOwner_oid(Oid extensionOid, Oid newOwnerId)
{
	Relation	rel;

	rel = heap_open(ExtensionRelationId, RowExclusiveLock);

	AlterExtensionOwner_internal(rel, extensionOid, newOwnerId);

	heap_close(rel, NoLock);
}
