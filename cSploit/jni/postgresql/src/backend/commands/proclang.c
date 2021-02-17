/*-------------------------------------------------------------------------
 *
 * proclang.c
 *	  PostgreSQL PROCEDURAL LANGUAGE support code.
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * IDENTIFICATION
 *	  src/backend/commands/proclang.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include "access/genam.h"
#include "access/heapam.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_language.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_pltemplate.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_proc_fn.h"
#include "catalog/pg_type.h"
#include "commands/dbcommands.h"
#include "commands/defrem.h"
#include "commands/proclang.h"
#include "miscadmin.h"
#include "parser/parse_func.h"
#include "parser/parser.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/tqual.h"


typedef struct
{
	bool		tmpltrusted;	/* trusted? */
	bool		tmpldbacreate;	/* db owner allowed to create? */
	char	   *tmplhandler;	/* name of handler function */
	char	   *tmplinline;		/* name of anonymous-block handler, or NULL */
	char	   *tmplvalidator;	/* name of validator function, or NULL */
	char	   *tmpllibrary;	/* path of shared library */
} PLTemplate;

static void create_proc_lang(const char *languageName, bool replace,
				 Oid languageOwner, Oid handlerOid, Oid inlineOid,
				 Oid valOid, bool trusted);
static PLTemplate *find_language_template(const char *languageName);
static void AlterLanguageOwner_internal(HeapTuple tup, Relation rel,
							Oid newOwnerId);


/* ---------------------------------------------------------------------
 * CREATE PROCEDURAL LANGUAGE
 * ---------------------------------------------------------------------
 */
void
CreateProceduralLanguage(CreatePLangStmt *stmt)
{
	char	   *languageName;
	PLTemplate *pltemplate;
	Oid			handlerOid,
				inlineOid,
				valOid;
	Oid			funcrettype;
	Oid			funcargtypes[1];

	/*
	 * Translate the language name to lower case
	 */
	languageName = case_translate_language_name(stmt->plname);

	/*
	 * If we have template information for the language, ignore the supplied
	 * parameters (if any) and use the template information.
	 */
	if ((pltemplate = find_language_template(languageName)) != NULL)
	{
		List	   *funcname;

		/*
		 * Give a notice if we are ignoring supplied parameters.
		 */
		if (stmt->plhandler)
			ereport(NOTICE,
					(errmsg("using pg_pltemplate information instead of CREATE LANGUAGE parameters")));

		/*
		 * Check permission
		 */
		if (!superuser())
		{
			if (!pltemplate->tmpldbacreate)
				ereport(ERROR,
						(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
						 errmsg("must be superuser to create procedural language \"%s\"",
								languageName)));
			if (!pg_database_ownercheck(MyDatabaseId, GetUserId()))
				aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_DATABASE,
							   get_database_name(MyDatabaseId));
		}

		/*
		 * Find or create the handler function, which we force to be in the
		 * pg_catalog schema.  If already present, it must have the correct
		 * return type.
		 */
		funcname = SystemFuncName(pltemplate->tmplhandler);
		handlerOid = LookupFuncName(funcname, 0, funcargtypes, true);
		if (OidIsValid(handlerOid))
		{
			funcrettype = get_func_rettype(handlerOid);
			if (funcrettype != LANGUAGE_HANDLEROID)
				ereport(ERROR,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				  errmsg("function %s must return type \"language_handler\"",
						 NameListToString(funcname))));
		}
		else
		{
			handlerOid = ProcedureCreate(pltemplate->tmplhandler,
										 PG_CATALOG_NAMESPACE,
										 false, /* replace */
										 false, /* returnsSet */
										 LANGUAGE_HANDLEROID,
										 ClanguageId,
										 F_FMGR_C_VALIDATOR,
										 pltemplate->tmplhandler,
										 pltemplate->tmpllibrary,
										 false, /* isAgg */
										 false, /* isWindowFunc */
										 false, /* security_definer */
										 false, /* isStrict */
										 PROVOLATILE_VOLATILE,
										 buildoidvector(funcargtypes, 0),
										 PointerGetDatum(NULL),
										 PointerGetDatum(NULL),
										 PointerGetDatum(NULL),
										 NIL,
										 PointerGetDatum(NULL),
										 1,
										 0);
		}

		/*
		 * Likewise for the anonymous block handler, if required; but we don't
		 * care about its return type.
		 */
		if (pltemplate->tmplinline)
		{
			funcname = SystemFuncName(pltemplate->tmplinline);
			funcargtypes[0] = INTERNALOID;
			inlineOid = LookupFuncName(funcname, 1, funcargtypes, true);
			if (!OidIsValid(inlineOid))
			{
				inlineOid = ProcedureCreate(pltemplate->tmplinline,
											PG_CATALOG_NAMESPACE,
											false,		/* replace */
											false,		/* returnsSet */
											VOIDOID,
											ClanguageId,
											F_FMGR_C_VALIDATOR,
											pltemplate->tmplinline,
											pltemplate->tmpllibrary,
											false,		/* isAgg */
											false,		/* isWindowFunc */
											false,		/* security_definer */
											true,		/* isStrict */
											PROVOLATILE_VOLATILE,
											buildoidvector(funcargtypes, 1),
											PointerGetDatum(NULL),
											PointerGetDatum(NULL),
											PointerGetDatum(NULL),
											NIL,
											PointerGetDatum(NULL),
											1,
											0);
			}
		}
		else
			inlineOid = InvalidOid;

		/*
		 * Likewise for the validator, if required; but we don't care about
		 * its return type.
		 */
		if (pltemplate->tmplvalidator)
		{
			funcname = SystemFuncName(pltemplate->tmplvalidator);
			funcargtypes[0] = OIDOID;
			valOid = LookupFuncName(funcname, 1, funcargtypes, true);
			if (!OidIsValid(valOid))
			{
				valOid = ProcedureCreate(pltemplate->tmplvalidator,
										 PG_CATALOG_NAMESPACE,
										 false, /* replace */
										 false, /* returnsSet */
										 VOIDOID,
										 ClanguageId,
										 F_FMGR_C_VALIDATOR,
										 pltemplate->tmplvalidator,
										 pltemplate->tmpllibrary,
										 false, /* isAgg */
										 false, /* isWindowFunc */
										 false, /* security_definer */
										 true,	/* isStrict */
										 PROVOLATILE_VOLATILE,
										 buildoidvector(funcargtypes, 1),
										 PointerGetDatum(NULL),
										 PointerGetDatum(NULL),
										 PointerGetDatum(NULL),
										 NIL,
										 PointerGetDatum(NULL),
										 1,
										 0);
			}
		}
		else
			valOid = InvalidOid;

		/* ok, create it */
		create_proc_lang(languageName, stmt->replace, GetUserId(),
						 handlerOid, inlineOid,
						 valOid, pltemplate->tmpltrusted);
	}
	else
	{
		/*
		 * No template, so use the provided information.  If there's no
		 * handler clause, the user is trying to rely on a template that we
		 * don't have, so complain accordingly.
		 */
		if (!stmt->plhandler)
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("unsupported language \"%s\"",
							languageName),
					 errhint("The supported languages are listed in the pg_pltemplate system catalog.")));

		/*
		 * Check permission
		 */
		if (!superuser())
			ereport(ERROR,
					(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
					 errmsg("must be superuser to create custom procedural language")));

		/*
		 * Lookup the PL handler function and check that it is of the expected
		 * return type
		 */
		handlerOid = LookupFuncName(stmt->plhandler, 0, funcargtypes, false);
		funcrettype = get_func_rettype(handlerOid);
		if (funcrettype != LANGUAGE_HANDLEROID)
		{
			/*
			 * We allow OPAQUE just so we can load old dump files.	When we
			 * see a handler function declared OPAQUE, change it to
			 * LANGUAGE_HANDLER.  (This is probably obsolete and removable?)
			 */
			if (funcrettype == OPAQUEOID)
			{
				ereport(WARNING,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
						 errmsg("changing return type of function %s from \"opaque\" to \"language_handler\"",
								NameListToString(stmt->plhandler))));
				SetFunctionReturnType(handlerOid, LANGUAGE_HANDLEROID);
			}
			else
				ereport(ERROR,
						(errcode(ERRCODE_WRONG_OBJECT_TYPE),
				  errmsg("function %s must return type \"language_handler\"",
						 NameListToString(stmt->plhandler))));
		}

		/* validate the inline function */
		if (stmt->plinline)
		{
			funcargtypes[0] = INTERNALOID;
			inlineOid = LookupFuncName(stmt->plinline, 1, funcargtypes, false);
			/* return value is ignored, so we don't check the type */
		}
		else
			inlineOid = InvalidOid;

		/* validate the validator function */
		if (stmt->plvalidator)
		{
			funcargtypes[0] = OIDOID;
			valOid = LookupFuncName(stmt->plvalidator, 1, funcargtypes, false);
			/* return value is ignored, so we don't check the type */
		}
		else
			valOid = InvalidOid;

		/* ok, create it */
		create_proc_lang(languageName, stmt->replace, GetUserId(),
						 handlerOid, inlineOid,
						 valOid, stmt->pltrusted);
	}
}

/*
 * Guts of language creation.
 */
static void
create_proc_lang(const char *languageName, bool replace,
				 Oid languageOwner, Oid handlerOid, Oid inlineOid,
				 Oid valOid, bool trusted)
{
	Relation	rel;
	TupleDesc	tupDesc;
	Datum		values[Natts_pg_language];
	bool		nulls[Natts_pg_language];
	bool		replaces[Natts_pg_language];
	NameData	langname;
	HeapTuple	oldtup;
	HeapTuple	tup;
	bool		is_update;
	ObjectAddress myself,
				referenced;

	rel = heap_open(LanguageRelationId, RowExclusiveLock);
	tupDesc = RelationGetDescr(rel);

	/* Prepare data to be inserted */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));
	memset(replaces, true, sizeof(replaces));

	namestrcpy(&langname, languageName);
	values[Anum_pg_language_lanname - 1] = NameGetDatum(&langname);
	values[Anum_pg_language_lanowner - 1] = ObjectIdGetDatum(languageOwner);
	values[Anum_pg_language_lanispl - 1] = BoolGetDatum(true);
	values[Anum_pg_language_lanpltrusted - 1] = BoolGetDatum(trusted);
	values[Anum_pg_language_lanplcallfoid - 1] = ObjectIdGetDatum(handlerOid);
	values[Anum_pg_language_laninline - 1] = ObjectIdGetDatum(inlineOid);
	values[Anum_pg_language_lanvalidator - 1] = ObjectIdGetDatum(valOid);
	nulls[Anum_pg_language_lanacl - 1] = true;

	/* Check for pre-existing definition */
	oldtup = SearchSysCache1(LANGNAME, PointerGetDatum(languageName));

	if (HeapTupleIsValid(oldtup))
	{
		/* There is one; okay to replace it? */
		if (!replace)
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("language \"%s\" already exists", languageName)));
		if (!pg_language_ownercheck(HeapTupleGetOid(oldtup), languageOwner))
			aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_LANGUAGE,
						   languageName);

		/*
		 * Do not change existing ownership or permissions.  Note
		 * dependency-update code below has to agree with this decision.
		 */
		replaces[Anum_pg_language_lanowner - 1] = false;
		replaces[Anum_pg_language_lanacl - 1] = false;

		/* Okay, do it... */
		tup = heap_modify_tuple(oldtup, tupDesc, values, nulls, replaces);
		simple_heap_update(rel, &tup->t_self, tup);

		ReleaseSysCache(oldtup);
		is_update = true;
	}
	else
	{
		/* Creating a new language */
		tup = heap_form_tuple(tupDesc, values, nulls);
		simple_heap_insert(rel, tup);
		is_update = false;
	}

	/* Need to update indexes for either the insert or update case */
	CatalogUpdateIndexes(rel, tup);

	/*
	 * Create dependencies for the new language.  If we are updating an
	 * existing language, first delete any existing pg_depend entries.
	 * (However, since we are not changing ownership or permissions, the
	 * shared dependencies do *not* need to change, and we leave them alone.)
	 */
	myself.classId = LanguageRelationId;
	myself.objectId = HeapTupleGetOid(tup);
	myself.objectSubId = 0;

	if (is_update)
		deleteDependencyRecordsFor(myself.classId, myself.objectId, true);

	/* dependency on owner of language */
	if (!is_update)
		recordDependencyOnOwner(myself.classId, myself.objectId,
								languageOwner);

	/* dependency on extension */
	recordDependencyOnCurrentExtension(&myself, is_update);

	/* dependency on the PL handler function */
	referenced.classId = ProcedureRelationId;
	referenced.objectId = handlerOid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on the inline handler function, if any */
	if (OidIsValid(inlineOid))
	{
		referenced.classId = ProcedureRelationId;
		referenced.objectId = inlineOid;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
	}

	/* dependency on the validator function, if any */
	if (OidIsValid(valOid))
	{
		referenced.classId = ProcedureRelationId;
		referenced.objectId = valOid;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
	}

	/* Post creation hook for new procedural language */
	InvokeObjectAccessHook(OAT_POST_CREATE,
						   LanguageRelationId, myself.objectId, 0);

	heap_close(rel, RowExclusiveLock);
}

/*
 * Look to see if we have template information for the given language name.
 */
static PLTemplate *
find_language_template(const char *languageName)
{
	PLTemplate *result;
	Relation	rel;
	SysScanDesc scan;
	ScanKeyData key;
	HeapTuple	tup;

	rel = heap_open(PLTemplateRelationId, AccessShareLock);

	ScanKeyInit(&key,
				Anum_pg_pltemplate_tmplname,
				BTEqualStrategyNumber, F_NAMEEQ,
				NameGetDatum(languageName));
	scan = systable_beginscan(rel, PLTemplateNameIndexId, true,
							  SnapshotNow, 1, &key);

	tup = systable_getnext(scan);
	if (HeapTupleIsValid(tup))
	{
		Form_pg_pltemplate tmpl = (Form_pg_pltemplate) GETSTRUCT(tup);
		Datum		datum;
		bool		isnull;

		result = (PLTemplate *) palloc0(sizeof(PLTemplate));
		result->tmpltrusted = tmpl->tmpltrusted;
		result->tmpldbacreate = tmpl->tmpldbacreate;

		/* Remaining fields are variable-width so we need heap_getattr */
		datum = heap_getattr(tup, Anum_pg_pltemplate_tmplhandler,
							 RelationGetDescr(rel), &isnull);
		if (!isnull)
			result->tmplhandler = TextDatumGetCString(datum);

		datum = heap_getattr(tup, Anum_pg_pltemplate_tmplinline,
							 RelationGetDescr(rel), &isnull);
		if (!isnull)
			result->tmplinline = TextDatumGetCString(datum);

		datum = heap_getattr(tup, Anum_pg_pltemplate_tmplvalidator,
							 RelationGetDescr(rel), &isnull);
		if (!isnull)
			result->tmplvalidator = TextDatumGetCString(datum);

		datum = heap_getattr(tup, Anum_pg_pltemplate_tmpllibrary,
							 RelationGetDescr(rel), &isnull);
		if (!isnull)
			result->tmpllibrary = TextDatumGetCString(datum);

		/* Ignore template if handler or library info is missing */
		if (!result->tmplhandler || !result->tmpllibrary)
			result = NULL;
	}
	else
		result = NULL;

	systable_endscan(scan);

	heap_close(rel, AccessShareLock);

	return result;
}


/*
 * This just returns TRUE if we have a valid template for a given language
 */
bool
PLTemplateExists(const char *languageName)
{
	return (find_language_template(languageName) != NULL);
}


/* ---------------------------------------------------------------------
 * DROP PROCEDURAL LANGUAGE
 * ---------------------------------------------------------------------
 */
void
DropProceduralLanguage(DropPLangStmt *stmt)
{
	char	   *languageName;
	Oid			oid;
	ObjectAddress object;

	/*
	 * Translate the language name, check that the language exists
	 */
	languageName = case_translate_language_name(stmt->plname);

	oid = get_language_oid(languageName, stmt->missing_ok);
	if (!OidIsValid(oid))
	{
		ereport(NOTICE,
				(errmsg("language \"%s\" does not exist, skipping",
						languageName)));
		return;
	}

	/*
	 * Check permission
	 */
	if (!pg_language_ownercheck(oid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_LANGUAGE,
					   languageName);

	object.classId = LanguageRelationId;
	object.objectId = oid;
	object.objectSubId = 0;

	/*
	 * Do the deletion
	 */
	performDeletion(&object, stmt->behavior);
}

/*
 * Guts of language dropping.
 */
void
DropProceduralLanguageById(Oid langOid)
{
	Relation	rel;
	HeapTuple	langTup;

	rel = heap_open(LanguageRelationId, RowExclusiveLock);

	langTup = SearchSysCache1(LANGOID, ObjectIdGetDatum(langOid));
	if (!HeapTupleIsValid(langTup))		/* should not happen */
		elog(ERROR, "cache lookup failed for language %u", langOid);

	simple_heap_delete(rel, &langTup->t_self);

	ReleaseSysCache(langTup);

	heap_close(rel, RowExclusiveLock);
}

/*
 * Rename language
 */
void
RenameLanguage(const char *oldname, const char *newname)
{
	HeapTuple	tup;
	Relation	rel;

	/* Translate both names for consistency with CREATE */
	oldname = case_translate_language_name(oldname);
	newname = case_translate_language_name(newname);

	rel = heap_open(LanguageRelationId, RowExclusiveLock);

	tup = SearchSysCacheCopy1(LANGNAME, CStringGetDatum(oldname));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("language \"%s\" does not exist", oldname)));

	/* make sure the new name doesn't exist */
	if (SearchSysCacheExists1(LANGNAME, CStringGetDatum(newname)))
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("language \"%s\" already exists", newname)));

	/* must be owner of PL */
	if (!pg_language_ownercheck(HeapTupleGetOid(tup), GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_LANGUAGE,
					   oldname);

	/* rename */
	namestrcpy(&(((Form_pg_language) GETSTRUCT(tup))->lanname), newname);
	simple_heap_update(rel, &tup->t_self, tup);
	CatalogUpdateIndexes(rel, tup);

	heap_close(rel, NoLock);
	heap_freetuple(tup);
}

/*
 * Change language owner
 */
void
AlterLanguageOwner(const char *name, Oid newOwnerId)
{
	HeapTuple	tup;
	Relation	rel;

	/* Translate name for consistency with CREATE */
	name = case_translate_language_name(name);

	rel = heap_open(LanguageRelationId, RowExclusiveLock);

	tup = SearchSysCache1(LANGNAME, CStringGetDatum(name));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("language \"%s\" does not exist", name)));

	AlterLanguageOwner_internal(tup, rel, newOwnerId);

	ReleaseSysCache(tup);

	heap_close(rel, RowExclusiveLock);

}

/*
 * Change language owner, specified by OID
 */
void
AlterLanguageOwner_oid(Oid oid, Oid newOwnerId)
{
	HeapTuple	tup;
	Relation	rel;

	rel = heap_open(LanguageRelationId, RowExclusiveLock);

	tup = SearchSysCache1(LANGOID, ObjectIdGetDatum(oid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for language %u", oid);

	AlterLanguageOwner_internal(tup, rel, newOwnerId);

	ReleaseSysCache(tup);

	heap_close(rel, RowExclusiveLock);
}

/*
 * Workhorse for AlterLanguageOwner variants
 */
static void
AlterLanguageOwner_internal(HeapTuple tup, Relation rel, Oid newOwnerId)
{
	Form_pg_language lanForm;

	lanForm = (Form_pg_language) GETSTRUCT(tup);

	/*
	 * If the new owner is the same as the existing owner, consider the
	 * command to have succeeded.  This is for dump restoration purposes.
	 */
	if (lanForm->lanowner != newOwnerId)
	{
		Datum		repl_val[Natts_pg_language];
		bool		repl_null[Natts_pg_language];
		bool		repl_repl[Natts_pg_language];
		Acl		   *newAcl;
		Datum		aclDatum;
		bool		isNull;
		HeapTuple	newtuple;

		/* Otherwise, must be owner of the existing object */
		if (!pg_language_ownercheck(HeapTupleGetOid(tup), GetUserId()))
			aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_LANGUAGE,
						   NameStr(lanForm->lanname));

		/* Must be able to become new owner */
		check_is_member_of_role(GetUserId(), newOwnerId);

		memset(repl_null, false, sizeof(repl_null));
		memset(repl_repl, false, sizeof(repl_repl));

		repl_repl[Anum_pg_language_lanowner - 1] = true;
		repl_val[Anum_pg_language_lanowner - 1] = ObjectIdGetDatum(newOwnerId);

		/*
		 * Determine the modified ACL for the new owner.  This is only
		 * necessary when the ACL is non-null.
		 */
		aclDatum = SysCacheGetAttr(LANGNAME, tup,
								   Anum_pg_language_lanacl,
								   &isNull);
		if (!isNull)
		{
			newAcl = aclnewowner(DatumGetAclP(aclDatum),
								 lanForm->lanowner, newOwnerId);
			repl_repl[Anum_pg_language_lanacl - 1] = true;
			repl_val[Anum_pg_language_lanacl - 1] = PointerGetDatum(newAcl);
		}

		newtuple = heap_modify_tuple(tup, RelationGetDescr(rel),
									 repl_val, repl_null, repl_repl);

		simple_heap_update(rel, &newtuple->t_self, newtuple);
		CatalogUpdateIndexes(rel, newtuple);

		heap_freetuple(newtuple);

		/* Update owner dependency reference */
		changeDependencyOnOwner(LanguageRelationId, HeapTupleGetOid(tup),
								newOwnerId);
	}
}

/*
 * get_language_oid - given a language name, look up the OID
 *
 * If missing_ok is false, throw an error if language name not found.  If
 * true, just return InvalidOid.
 */
Oid
get_language_oid(const char *langname, bool missing_ok)
{
	Oid			oid;

	oid = GetSysCacheOid1(LANGNAME, CStringGetDatum(langname));
	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("language \"%s\" does not exist", langname)));
	return oid;
}
