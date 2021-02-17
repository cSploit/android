/*-------------------------------------------------------------------------
 *
 * opclasscmds.c
 *
 *	  Routines for opclass (and opfamily) manipulation commands
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 *
 * IDENTIFICATION
 *	  src/backend/commands/opclasscmds.c
 *
 *-------------------------------------------------------------------------
 */
#include "postgres.h"

#include <limits.h>

#include "access/genam.h"
#include "access/heapam.h"
#include "access/sysattr.h"
#include "catalog/dependency.h"
#include "catalog/indexing.h"
#include "catalog/objectaccess.h"
#include "catalog/pg_amop.h"
#include "catalog/pg_amproc.h"
#include "catalog/pg_namespace.h"
#include "catalog/pg_opclass.h"
#include "catalog/pg_operator.h"
#include "catalog/pg_opfamily.h"
#include "catalog/pg_proc.h"
#include "catalog/pg_type.h"
#include "commands/alter.h"
#include "commands/defrem.h"
#include "miscadmin.h"
#include "parser/parse_func.h"
#include "parser/parse_oper.h"
#include "parser/parse_type.h"
#include "utils/acl.h"
#include "utils/builtins.h"
#include "utils/fmgroids.h"
#include "utils/lsyscache.h"
#include "utils/rel.h"
#include "utils/syscache.h"
#include "utils/tqual.h"


/*
 * We use lists of this struct type to keep track of both operators and
 * procedures while building or adding to an opfamily.
 */
typedef struct
{
	Oid			object;			/* operator or support proc's OID */
	int			number;			/* strategy or support proc number */
	Oid			lefttype;		/* lefttype */
	Oid			righttype;		/* righttype */
	Oid			sortfamily;		/* ordering operator's sort opfamily, or 0 */
} OpFamilyMember;


static void AlterOpFamilyAdd(List *opfamilyname, Oid amoid, Oid opfamilyoid,
				 int maxOpNumber, int maxProcNumber,
				 List *items);
static void AlterOpFamilyDrop(List *opfamilyname, Oid amoid, Oid opfamilyoid,
				  int maxOpNumber, int maxProcNumber,
				  List *items);
static void processTypesSpec(List *args, Oid *lefttype, Oid *righttype);
static void assignOperTypes(OpFamilyMember *member, Oid amoid, Oid typeoid);
static void assignProcTypes(OpFamilyMember *member, Oid amoid, Oid typeoid);
static void addFamilyMember(List **list, OpFamilyMember *member, bool isProc);
static void storeOperators(List *opfamilyname, Oid amoid,
			   Oid opfamilyoid, Oid opclassoid,
			   List *operators, bool isAdd);
static void storeProcedures(List *opfamilyname, Oid amoid,
				Oid opfamilyoid, Oid opclassoid,
				List *procedures, bool isAdd);
static void dropOperators(List *opfamilyname, Oid amoid, Oid opfamilyoid,
			  List *operators);
static void dropProcedures(List *opfamilyname, Oid amoid, Oid opfamilyoid,
			   List *procedures);
static void AlterOpClassOwner_internal(Relation rel, HeapTuple tuple,
						   Oid newOwnerId);
static void AlterOpFamilyOwner_internal(Relation rel, HeapTuple tuple,
							Oid newOwnerId);


/*
 * OpFamilyCacheLookup
 *		Look up an existing opfamily by name.
 *
 * Returns a syscache tuple reference, or NULL if not found.
 */
static HeapTuple
OpFamilyCacheLookup(Oid amID, List *opfamilyname, bool missing_ok)
{
	char	   *schemaname;
	char	   *opfname;
	HeapTuple	htup;

	/* deconstruct the name list */
	DeconstructQualifiedName(opfamilyname, &schemaname, &opfname);

	if (schemaname)
	{
		/* Look in specific schema only */
		Oid			namespaceId;

		namespaceId = LookupExplicitNamespace(schemaname);
		htup = SearchSysCache3(OPFAMILYAMNAMENSP,
							   ObjectIdGetDatum(amID),
							   PointerGetDatum(opfname),
							   ObjectIdGetDatum(namespaceId));
	}
	else
	{
		/* Unqualified opfamily name, so search the search path */
		Oid			opfID = OpfamilynameGetOpfid(amID, opfname);

		if (!OidIsValid(opfID))
			htup = NULL;
		else
			htup = SearchSysCache1(OPFAMILYOID, ObjectIdGetDatum(opfID));
	}

	if (!HeapTupleIsValid(htup) && !missing_ok)
	{
		HeapTuple	amtup;

		amtup = SearchSysCache1(AMOID, ObjectIdGetDatum(amID));
		if (!HeapTupleIsValid(amtup))
			elog(ERROR, "cache lookup failed for access method %u", amID);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("operator family \"%s\" does not exist for access method \"%s\"",
						NameListToString(opfamilyname),
						NameStr(((Form_pg_am) GETSTRUCT(amtup))->amname))));
	}

	return htup;
}

/*
 * get_opfamily_oid
 *	  find an opfamily OID by possibly qualified name
 *
 * If not found, returns InvalidOid if missing_ok, else throws error.
 */
Oid
get_opfamily_oid(Oid amID, List *opfamilyname, bool missing_ok)
{
	HeapTuple	htup;
	Oid			opfID;

	htup = OpFamilyCacheLookup(amID, opfamilyname, missing_ok);
	if (!HeapTupleIsValid(htup))
		return InvalidOid;
	opfID = HeapTupleGetOid(htup);
	ReleaseSysCache(htup);

	return opfID;
}

/*
 * OpClassCacheLookup
 *		Look up an existing opclass by name.
 *
 * Returns a syscache tuple reference, or NULL if not found.
 */
static HeapTuple
OpClassCacheLookup(Oid amID, List *opclassname, bool missing_ok)
{
	char	   *schemaname;
	char	   *opcname;
	HeapTuple	htup;

	/* deconstruct the name list */
	DeconstructQualifiedName(opclassname, &schemaname, &opcname);

	if (schemaname)
	{
		/* Look in specific schema only */
		Oid			namespaceId;

		namespaceId = LookupExplicitNamespace(schemaname);
		htup = SearchSysCache3(CLAAMNAMENSP,
							   ObjectIdGetDatum(amID),
							   PointerGetDatum(opcname),
							   ObjectIdGetDatum(namespaceId));
	}
	else
	{
		/* Unqualified opclass name, so search the search path */
		Oid			opcID = OpclassnameGetOpcid(amID, opcname);

		if (!OidIsValid(opcID))
			htup = NULL;
		else
			htup = SearchSysCache1(CLAOID, ObjectIdGetDatum(opcID));
	}

	if (!HeapTupleIsValid(htup) && !missing_ok)
	{
		HeapTuple	amtup;

		amtup = SearchSysCache1(AMOID, ObjectIdGetDatum(amID));
		if (!HeapTupleIsValid(amtup))
			elog(ERROR, "cache lookup failed for access method %u", amID);
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("operator class \"%s\" does not exist for access method \"%s\"",
						NameListToString(opclassname),
						NameStr(((Form_pg_am) GETSTRUCT(amtup))->amname))));
	}

	return htup;
}

/*
 * get_opclass_oid
 *	  find an opclass OID by possibly qualified name
 *
 * If not found, returns InvalidOid if missing_ok, else throws error.
 */
Oid
get_opclass_oid(Oid amID, List *opclassname, bool missing_ok)
{
	HeapTuple	htup;
	Oid			opcID;

	htup = OpClassCacheLookup(amID, opclassname, missing_ok);
	if (!HeapTupleIsValid(htup))
		return InvalidOid;
	opcID = HeapTupleGetOid(htup);
	ReleaseSysCache(htup);

	return opcID;
}

/*
 * CreateOpFamily
 *		Internal routine to make the catalog entry for a new operator family.
 *
 * Caller must have done permissions checks etc. already.
 */
static Oid
CreateOpFamily(char *amname, char *opfname, Oid namespaceoid, Oid amoid)
{
	Oid			opfamilyoid;
	Relation	rel;
	HeapTuple	tup;
	Datum		values[Natts_pg_opfamily];
	bool		nulls[Natts_pg_opfamily];
	NameData	opfName;
	ObjectAddress myself,
				referenced;

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	/*
	 * Make sure there is no existing opfamily of this name (this is just to
	 * give a more friendly error message than "duplicate key").
	 */
	if (SearchSysCacheExists3(OPFAMILYAMNAMENSP,
							  ObjectIdGetDatum(amoid),
							  CStringGetDatum(opfname),
							  ObjectIdGetDatum(namespaceoid)))
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("operator family \"%s\" for access method \"%s\" already exists",
						opfname, amname)));

	/*
	 * Okay, let's create the pg_opfamily entry.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_opfamily_opfmethod - 1] = ObjectIdGetDatum(amoid);
	namestrcpy(&opfName, opfname);
	values[Anum_pg_opfamily_opfname - 1] = NameGetDatum(&opfName);
	values[Anum_pg_opfamily_opfnamespace - 1] = ObjectIdGetDatum(namespaceoid);
	values[Anum_pg_opfamily_opfowner - 1] = ObjectIdGetDatum(GetUserId());

	tup = heap_form_tuple(rel->rd_att, values, nulls);

	opfamilyoid = simple_heap_insert(rel, tup);

	CatalogUpdateIndexes(rel, tup);

	heap_freetuple(tup);

	/*
	 * Create dependencies for the opfamily proper.  Note: we do not create a
	 * dependency link to the AM, because we don't currently support DROP
	 * ACCESS METHOD.
	 */
	myself.classId = OperatorFamilyRelationId;
	myself.objectId = opfamilyoid;
	myself.objectSubId = 0;

	/* dependency on namespace */
	referenced.classId = NamespaceRelationId;
	referenced.objectId = namespaceoid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on owner */
	recordDependencyOnOwner(OperatorFamilyRelationId, opfamilyoid, GetUserId());

	/* dependency on extension */
	recordDependencyOnCurrentExtension(&myself, false);

	/* Post creation hook for new operator family */
	InvokeObjectAccessHook(OAT_POST_CREATE,
						   OperatorFamilyRelationId, opfamilyoid, 0);

	heap_close(rel, RowExclusiveLock);

	return opfamilyoid;
}

/*
 * DefineOpClass
 *		Define a new index operator class.
 */
void
DefineOpClass(CreateOpClassStmt *stmt)
{
	char	   *opcname;		/* name of opclass we're creating */
	Oid			amoid,			/* our AM's oid */
				typeoid,		/* indexable datatype oid */
				storageoid,		/* storage datatype oid, if any */
				namespaceoid,	/* namespace to create opclass in */
				opfamilyoid,	/* oid of containing opfamily */
				opclassoid;		/* oid of opclass we create */
	int			maxOpNumber,	/* amstrategies value */
				maxProcNumber;	/* amsupport value */
	bool		amstorage;		/* amstorage flag */
	List	   *operators;		/* OpFamilyMember list for operators */
	List	   *procedures;		/* OpFamilyMember list for support procs */
	ListCell   *l;
	Relation	rel;
	HeapTuple	tup;
	Form_pg_am	pg_am;
	Datum		values[Natts_pg_opclass];
	bool		nulls[Natts_pg_opclass];
	AclResult	aclresult;
	NameData	opcName;
	ObjectAddress myself,
				referenced;

	/* Convert list of names to a name and namespace */
	namespaceoid = QualifiedNameGetCreationNamespace(stmt->opclassname,
													 &opcname);

	/* Check we have creation rights in target namespace */
	aclresult = pg_namespace_aclcheck(namespaceoid, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
					   get_namespace_name(namespaceoid));

	/* Get necessary info about access method */
	tup = SearchSysCache1(AMNAME, CStringGetDatum(stmt->amname));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("access method \"%s\" does not exist",
						stmt->amname)));

	amoid = HeapTupleGetOid(tup);
	pg_am = (Form_pg_am) GETSTRUCT(tup);
	maxOpNumber = pg_am->amstrategies;
	/* if amstrategies is zero, just enforce that op numbers fit in int16 */
	if (maxOpNumber <= 0)
		maxOpNumber = SHRT_MAX;
	maxProcNumber = pg_am->amsupport;
	amstorage = pg_am->amstorage;

	/* XXX Should we make any privilege check against the AM? */

	ReleaseSysCache(tup);

	/*
	 * The question of appropriate permissions for CREATE OPERATOR CLASS is
	 * interesting.  Creating an opclass is tantamount to granting public
	 * execute access on the functions involved, since the index machinery
	 * generally does not check access permission before using the functions.
	 * A minimum expectation therefore is that the caller have execute
	 * privilege with grant option.  Since we don't have a way to make the
	 * opclass go away if the grant option is revoked, we choose instead to
	 * require ownership of the functions.	It's also not entirely clear what
	 * permissions should be required on the datatype, but ownership seems
	 * like a safe choice.
	 *
	 * Currently, we require superuser privileges to create an opclass. This
	 * seems necessary because we have no way to validate that the offered set
	 * of operators and functions are consistent with the AM's expectations.
	 * It would be nice to provide such a check someday, if it can be done
	 * without solving the halting problem :-(
	 *
	 * XXX re-enable NOT_USED code sections below if you remove this test.
	 */
	if (!superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to create an operator class")));

	/* Look up the datatype */
	typeoid = typenameTypeId(NULL, stmt->datatype);

#ifdef NOT_USED
	/* XXX this is unnecessary given the superuser check above */
	/* Check we have ownership of the datatype */
	if (!pg_type_ownercheck(typeoid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_TYPE,
					   format_type_be(typeoid));
#endif

	/*
	 * Look up the containing operator family, or create one if FAMILY option
	 * was omitted and there's not a match already.
	 */
	if (stmt->opfamilyname)
	{
		opfamilyoid = get_opfamily_oid(amoid, stmt->opfamilyname, false);
	}
	else
	{
		/* Lookup existing family of same name and namespace */
		tup = SearchSysCache3(OPFAMILYAMNAMENSP,
							  ObjectIdGetDatum(amoid),
							  PointerGetDatum(opcname),
							  ObjectIdGetDatum(namespaceoid));
		if (HeapTupleIsValid(tup))
		{
			opfamilyoid = HeapTupleGetOid(tup);

			/*
			 * XXX given the superuser check above, there's no need for an
			 * ownership check here
			 */
			ReleaseSysCache(tup);
		}
		else
		{
			/*
			 * Create it ... again no need for more permissions ...
			 */
			opfamilyoid = CreateOpFamily(stmt->amname, opcname,
										 namespaceoid, amoid);
		}
	}

	operators = NIL;
	procedures = NIL;

	/* Storage datatype is optional */
	storageoid = InvalidOid;

	/*
	 * Scan the "items" list to obtain additional info.
	 */
	foreach(l, stmt->items)
	{
		CreateOpClassItem *item = lfirst(l);
		Oid			operOid;
		Oid			funcOid;
		Oid			sortfamilyOid;
		OpFamilyMember *member;

		Assert(IsA(item, CreateOpClassItem));
		switch (item->itemtype)
		{
			case OPCLASS_ITEM_OPERATOR:
				if (item->number <= 0 || item->number > maxOpNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid operator number %d,"
									" must be between 1 and %d",
									item->number, maxOpNumber)));
				if (item->args != NIL)
				{
					TypeName   *typeName1 = (TypeName *) linitial(item->args);
					TypeName   *typeName2 = (TypeName *) lsecond(item->args);

					operOid = LookupOperNameTypeNames(NULL, item->name,
													  typeName1, typeName2,
													  false, -1);
				}
				else
				{
					/* Default to binary op on input datatype */
					operOid = LookupOperName(NULL, item->name,
											 typeoid, typeoid,
											 false, -1);
				}

				if (item->order_family)
					sortfamilyOid = get_opfamily_oid(BTREE_AM_OID,
													 item->order_family,
													 false);
				else
					sortfamilyOid = InvalidOid;

#ifdef NOT_USED
				/* XXX this is unnecessary given the superuser check above */
				/* Caller must own operator and its underlying function */
				if (!pg_oper_ownercheck(operOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPER,
								   get_opname(operOid));
				funcOid = get_opcode(operOid);
				if (!pg_proc_ownercheck(funcOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_PROC,
								   get_func_name(funcOid));
#endif

				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->object = operOid;
				member->number = item->number;
				member->sortfamily = sortfamilyOid;
				assignOperTypes(member, amoid, typeoid);
				addFamilyMember(&operators, member, false);
				break;
			case OPCLASS_ITEM_FUNCTION:
				if (item->number <= 0 || item->number > maxProcNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid procedure number %d,"
									" must be between 1 and %d",
									item->number, maxProcNumber)));
				funcOid = LookupFuncNameTypeNames(item->name, item->args,
												  false);
#ifdef NOT_USED
				/* XXX this is unnecessary given the superuser check above */
				/* Caller must own function */
				if (!pg_proc_ownercheck(funcOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_PROC,
								   get_func_name(funcOid));
#endif

				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->object = funcOid;
				member->number = item->number;

				/* allow overriding of the function's actual arg types */
				if (item->class_args)
					processTypesSpec(item->class_args,
									 &member->lefttype, &member->righttype);

				assignProcTypes(member, amoid, typeoid);
				addFamilyMember(&procedures, member, true);
				break;
			case OPCLASS_ITEM_STORAGETYPE:
				if (OidIsValid(storageoid))
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						   errmsg("storage type specified more than once")));
				storageoid = typenameTypeId(NULL, item->storedtype);

#ifdef NOT_USED
				/* XXX this is unnecessary given the superuser check above */
				/* Check we have ownership of the datatype */
				if (!pg_type_ownercheck(storageoid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_TYPE,
								   format_type_be(storageoid));
#endif
				break;
			default:
				elog(ERROR, "unrecognized item type: %d", item->itemtype);
				break;
		}
	}

	/*
	 * If storagetype is specified, make sure it's legal.
	 */
	if (OidIsValid(storageoid))
	{
		/* Just drop the spec if same as column datatype */
		if (storageoid == typeoid)
			storageoid = InvalidOid;
		else if (!amstorage)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("storage type cannot be different from data type for access method \"%s\"",
							stmt->amname)));
	}

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	/*
	 * Make sure there is no existing opclass of this name (this is just to
	 * give a more friendly error message than "duplicate key").
	 */
	if (SearchSysCacheExists3(CLAAMNAMENSP,
							  ObjectIdGetDatum(amoid),
							  CStringGetDatum(opcname),
							  ObjectIdGetDatum(namespaceoid)))
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("operator class \"%s\" for access method \"%s\" already exists",
						opcname, stmt->amname)));

	/*
	 * If we are creating a default opclass, check there isn't one already.
	 * (Note we do not restrict this test to visible opclasses; this ensures
	 * that typcache.c can find unique solutions to its questions.)
	 */
	if (stmt->isDefault)
	{
		ScanKeyData skey[1];
		SysScanDesc scan;

		ScanKeyInit(&skey[0],
					Anum_pg_opclass_opcmethod,
					BTEqualStrategyNumber, F_OIDEQ,
					ObjectIdGetDatum(amoid));

		scan = systable_beginscan(rel, OpclassAmNameNspIndexId, true,
								  SnapshotNow, 1, skey);

		while (HeapTupleIsValid(tup = systable_getnext(scan)))
		{
			Form_pg_opclass opclass = (Form_pg_opclass) GETSTRUCT(tup);

			if (opclass->opcintype == typeoid && opclass->opcdefault)
				ereport(ERROR,
						(errcode(ERRCODE_DUPLICATE_OBJECT),
						 errmsg("could not make operator class \"%s\" be default for type %s",
								opcname,
								TypeNameToString(stmt->datatype)),
				   errdetail("Operator class \"%s\" already is the default.",
							 NameStr(opclass->opcname))));
		}

		systable_endscan(scan);
	}

	/*
	 * Okay, let's create the pg_opclass entry.
	 */
	memset(values, 0, sizeof(values));
	memset(nulls, false, sizeof(nulls));

	values[Anum_pg_opclass_opcmethod - 1] = ObjectIdGetDatum(amoid);
	namestrcpy(&opcName, opcname);
	values[Anum_pg_opclass_opcname - 1] = NameGetDatum(&opcName);
	values[Anum_pg_opclass_opcnamespace - 1] = ObjectIdGetDatum(namespaceoid);
	values[Anum_pg_opclass_opcowner - 1] = ObjectIdGetDatum(GetUserId());
	values[Anum_pg_opclass_opcfamily - 1] = ObjectIdGetDatum(opfamilyoid);
	values[Anum_pg_opclass_opcintype - 1] = ObjectIdGetDatum(typeoid);
	values[Anum_pg_opclass_opcdefault - 1] = BoolGetDatum(stmt->isDefault);
	values[Anum_pg_opclass_opckeytype - 1] = ObjectIdGetDatum(storageoid);

	tup = heap_form_tuple(rel->rd_att, values, nulls);

	opclassoid = simple_heap_insert(rel, tup);

	CatalogUpdateIndexes(rel, tup);

	heap_freetuple(tup);

	/*
	 * Now add tuples to pg_amop and pg_amproc tying in the operators and
	 * functions.  Dependencies on them are inserted, too.
	 */
	storeOperators(stmt->opfamilyname, amoid, opfamilyoid,
				   opclassoid, operators, false);
	storeProcedures(stmt->opfamilyname, amoid, opfamilyoid,
					opclassoid, procedures, false);

	/*
	 * Create dependencies for the opclass proper.	Note: we do not create a
	 * dependency link to the AM, because we don't currently support DROP
	 * ACCESS METHOD.
	 */
	myself.classId = OperatorClassRelationId;
	myself.objectId = opclassoid;
	myself.objectSubId = 0;

	/* dependency on namespace */
	referenced.classId = NamespaceRelationId;
	referenced.objectId = namespaceoid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on opfamily */
	referenced.classId = OperatorFamilyRelationId;
	referenced.objectId = opfamilyoid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);

	/* dependency on indexed datatype */
	referenced.classId = TypeRelationId;
	referenced.objectId = typeoid;
	referenced.objectSubId = 0;
	recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

	/* dependency on storage datatype */
	if (OidIsValid(storageoid))
	{
		referenced.classId = TypeRelationId;
		referenced.objectId = storageoid;
		referenced.objectSubId = 0;
		recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
	}

	/* dependency on owner */
	recordDependencyOnOwner(OperatorClassRelationId, opclassoid, GetUserId());

	/* dependency on extension */
	recordDependencyOnCurrentExtension(&myself, false);

	/* Post creation hook for new operator class */
	InvokeObjectAccessHook(OAT_POST_CREATE,
						   OperatorClassRelationId, opclassoid, 0);

	heap_close(rel, RowExclusiveLock);
}


/*
 * DefineOpFamily
 *		Define a new index operator family.
 */
void
DefineOpFamily(CreateOpFamilyStmt *stmt)
{
	char	   *opfname;		/* name of opfamily we're creating */
	Oid			amoid,			/* our AM's oid */
				namespaceoid;	/* namespace to create opfamily in */
	AclResult	aclresult;

	/* Convert list of names to a name and namespace */
	namespaceoid = QualifiedNameGetCreationNamespace(stmt->opfamilyname,
													 &opfname);

	/* Check we have creation rights in target namespace */
	aclresult = pg_namespace_aclcheck(namespaceoid, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
					   get_namespace_name(namespaceoid));

	/* Get access method OID, throwing an error if it doesn't exist. */
	amoid = get_am_oid(stmt->amname, false);

	/* XXX Should we make any privilege check against the AM? */

	/*
	 * Currently, we require superuser privileges to create an opfamily. See
	 * comments in DefineOpClass.
	 */
	if (!superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to create an operator family")));

	/* Insert pg_opfamily catalog entry */
	(void) CreateOpFamily(stmt->amname, opfname, namespaceoid, amoid);
}


/*
 * AlterOpFamily
 *		Add or remove operators/procedures within an existing operator family.
 *
 * Note: this implements only ALTER OPERATOR FAMILY ... ADD/DROP.  Some
 * other commands called ALTER OPERATOR FAMILY exist, but go through
 * different code paths.
 */
void
AlterOpFamily(AlterOpFamilyStmt *stmt)
{
	Oid			amoid,			/* our AM's oid */
				opfamilyoid;	/* oid of opfamily */
	int			maxOpNumber,	/* amstrategies value */
				maxProcNumber;	/* amsupport value */
	HeapTuple	tup;
	Form_pg_am	pg_am;

	/* Get necessary info about access method */
	tup = SearchSysCache1(AMNAME, CStringGetDatum(stmt->amname));
	if (!HeapTupleIsValid(tup))
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("access method \"%s\" does not exist",
						stmt->amname)));

	amoid = HeapTupleGetOid(tup);
	pg_am = (Form_pg_am) GETSTRUCT(tup);
	maxOpNumber = pg_am->amstrategies;
	/* if amstrategies is zero, just enforce that op numbers fit in int16 */
	if (maxOpNumber <= 0)
		maxOpNumber = SHRT_MAX;
	maxProcNumber = pg_am->amsupport;

	/* XXX Should we make any privilege check against the AM? */

	ReleaseSysCache(tup);

	/* Look up the opfamily */
	opfamilyoid = get_opfamily_oid(amoid, stmt->opfamilyname, false);

	/*
	 * Currently, we require superuser privileges to alter an opfamily.
	 *
	 * XXX re-enable NOT_USED code sections below if you remove this test.
	 */
	if (!superuser())
		ereport(ERROR,
				(errcode(ERRCODE_INSUFFICIENT_PRIVILEGE),
				 errmsg("must be superuser to alter an operator family")));

	/*
	 * ADD and DROP cases need separate code from here on down.
	 */
	if (stmt->isDrop)
		AlterOpFamilyDrop(stmt->opfamilyname, amoid, opfamilyoid,
						  maxOpNumber, maxProcNumber,
						  stmt->items);
	else
		AlterOpFamilyAdd(stmt->opfamilyname, amoid, opfamilyoid,
						 maxOpNumber, maxProcNumber,
						 stmt->items);
}

/*
 * ADD part of ALTER OP FAMILY
 */
static void
AlterOpFamilyAdd(List *opfamilyname, Oid amoid, Oid opfamilyoid,
				 int maxOpNumber, int maxProcNumber,
				 List *items)
{
	List	   *operators;		/* OpFamilyMember list for operators */
	List	   *procedures;		/* OpFamilyMember list for support procs */
	ListCell   *l;

	operators = NIL;
	procedures = NIL;

	/*
	 * Scan the "items" list to obtain additional info.
	 */
	foreach(l, items)
	{
		CreateOpClassItem *item = lfirst(l);
		Oid			operOid;
		Oid			funcOid;
		Oid			sortfamilyOid;
		OpFamilyMember *member;

		Assert(IsA(item, CreateOpClassItem));
		switch (item->itemtype)
		{
			case OPCLASS_ITEM_OPERATOR:
				if (item->number <= 0 || item->number > maxOpNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid operator number %d,"
									" must be between 1 and %d",
									item->number, maxOpNumber)));
				if (item->args != NIL)
				{
					TypeName   *typeName1 = (TypeName *) linitial(item->args);
					TypeName   *typeName2 = (TypeName *) lsecond(item->args);

					operOid = LookupOperNameTypeNames(NULL, item->name,
													  typeName1, typeName2,
													  false, -1);
				}
				else
				{
					ereport(ERROR,
							(errcode(ERRCODE_SYNTAX_ERROR),
							 errmsg("operator argument types must be specified in ALTER OPERATOR FAMILY")));
					operOid = InvalidOid;		/* keep compiler quiet */
				}

				if (item->order_family)
					sortfamilyOid = get_opfamily_oid(BTREE_AM_OID,
													 item->order_family,
													 false);
				else
					sortfamilyOid = InvalidOid;

#ifdef NOT_USED
				/* XXX this is unnecessary given the superuser check above */
				/* Caller must own operator and its underlying function */
				if (!pg_oper_ownercheck(operOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPER,
								   get_opname(operOid));
				funcOid = get_opcode(operOid);
				if (!pg_proc_ownercheck(funcOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_PROC,
								   get_func_name(funcOid));
#endif

				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->object = operOid;
				member->number = item->number;
				member->sortfamily = sortfamilyOid;
				assignOperTypes(member, amoid, InvalidOid);
				addFamilyMember(&operators, member, false);
				break;
			case OPCLASS_ITEM_FUNCTION:
				if (item->number <= 0 || item->number > maxProcNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid procedure number %d,"
									" must be between 1 and %d",
									item->number, maxProcNumber)));
				funcOid = LookupFuncNameTypeNames(item->name, item->args,
												  false);
#ifdef NOT_USED
				/* XXX this is unnecessary given the superuser check above */
				/* Caller must own function */
				if (!pg_proc_ownercheck(funcOid, GetUserId()))
					aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_PROC,
								   get_func_name(funcOid));
#endif

				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->object = funcOid;
				member->number = item->number;

				/* allow overriding of the function's actual arg types */
				if (item->class_args)
					processTypesSpec(item->class_args,
									 &member->lefttype, &member->righttype);

				assignProcTypes(member, amoid, InvalidOid);
				addFamilyMember(&procedures, member, true);
				break;
			case OPCLASS_ITEM_STORAGETYPE:
				ereport(ERROR,
						(errcode(ERRCODE_SYNTAX_ERROR),
						 errmsg("STORAGE cannot be specified in ALTER OPERATOR FAMILY")));
				break;
			default:
				elog(ERROR, "unrecognized item type: %d", item->itemtype);
				break;
		}
	}

	/*
	 * Add tuples to pg_amop and pg_amproc tying in the operators and
	 * functions.  Dependencies on them are inserted, too.
	 */
	storeOperators(opfamilyname, amoid, opfamilyoid,
				   InvalidOid, operators, true);
	storeProcedures(opfamilyname, amoid, opfamilyoid,
					InvalidOid, procedures, true);
}

/*
 * DROP part of ALTER OP FAMILY
 */
static void
AlterOpFamilyDrop(List *opfamilyname, Oid amoid, Oid opfamilyoid,
				  int maxOpNumber, int maxProcNumber,
				  List *items)
{
	List	   *operators;		/* OpFamilyMember list for operators */
	List	   *procedures;		/* OpFamilyMember list for support procs */
	ListCell   *l;

	operators = NIL;
	procedures = NIL;

	/*
	 * Scan the "items" list to obtain additional info.
	 */
	foreach(l, items)
	{
		CreateOpClassItem *item = lfirst(l);
		Oid			lefttype,
					righttype;
		OpFamilyMember *member;

		Assert(IsA(item, CreateOpClassItem));
		switch (item->itemtype)
		{
			case OPCLASS_ITEM_OPERATOR:
				if (item->number <= 0 || item->number > maxOpNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid operator number %d,"
									" must be between 1 and %d",
									item->number, maxOpNumber)));
				processTypesSpec(item->args, &lefttype, &righttype);
				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->number = item->number;
				member->lefttype = lefttype;
				member->righttype = righttype;
				addFamilyMember(&operators, member, false);
				break;
			case OPCLASS_ITEM_FUNCTION:
				if (item->number <= 0 || item->number > maxProcNumber)
					ereport(ERROR,
							(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
							 errmsg("invalid procedure number %d,"
									" must be between 1 and %d",
									item->number, maxProcNumber)));
				processTypesSpec(item->args, &lefttype, &righttype);
				/* Save the info */
				member = (OpFamilyMember *) palloc0(sizeof(OpFamilyMember));
				member->number = item->number;
				member->lefttype = lefttype;
				member->righttype = righttype;
				addFamilyMember(&procedures, member, true);
				break;
			case OPCLASS_ITEM_STORAGETYPE:
				/* grammar prevents this from appearing */
			default:
				elog(ERROR, "unrecognized item type: %d", item->itemtype);
				break;
		}
	}

	/*
	 * Remove tuples from pg_amop and pg_amproc.
	 */
	dropOperators(opfamilyname, amoid, opfamilyoid, operators);
	dropProcedures(opfamilyname, amoid, opfamilyoid, procedures);
}


/*
 * Deal with explicit arg types used in ALTER ADD/DROP
 */
static void
processTypesSpec(List *args, Oid *lefttype, Oid *righttype)
{
	TypeName   *typeName;

	Assert(args != NIL);

	typeName = (TypeName *) linitial(args);
	*lefttype = typenameTypeId(NULL, typeName);

	if (list_length(args) > 1)
	{
		typeName = (TypeName *) lsecond(args);
		*righttype = typenameTypeId(NULL, typeName);
	}
	else
		*righttype = *lefttype;

	if (list_length(args) > 2)
		ereport(ERROR,
				(errcode(ERRCODE_SYNTAX_ERROR),
				 errmsg("one or two argument types must be specified")));
}


/*
 * Determine the lefttype/righttype to assign to an operator,
 * and do any validity checking we can manage.
 */
static void
assignOperTypes(OpFamilyMember *member, Oid amoid, Oid typeoid)
{
	Operator	optup;
	Form_pg_operator opform;

	/* Fetch the operator definition */
	optup = SearchSysCache1(OPEROID, ObjectIdGetDatum(member->object));
	if (optup == NULL)
		elog(ERROR, "cache lookup failed for operator %u", member->object);
	opform = (Form_pg_operator) GETSTRUCT(optup);

	/*
	 * Opfamily operators must be binary.
	 */
	if (opform->oprkind != 'b')
		ereport(ERROR,
				(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
				 errmsg("index operators must be binary")));

	if (OidIsValid(member->sortfamily))
	{
		/*
		 * Ordering op, check index supports that.	(We could perhaps also
		 * check that the operator returns a type supported by the sortfamily,
		 * but that seems more trouble than it's worth here.  If it does not,
		 * the operator will never be matchable to any ORDER BY clause, but no
		 * worse consequences can ensue.  Also, trying to check that would
		 * create an ordering hazard during dump/reload: it's possible that
		 * the family has been created but not yet populated with the required
		 * operators.)
		 */
		HeapTuple	amtup;
		Form_pg_am	pg_am;

		amtup = SearchSysCache1(AMOID, ObjectIdGetDatum(amoid));
		if (amtup == NULL)
			elog(ERROR, "cache lookup failed for access method %u", amoid);
		pg_am = (Form_pg_am) GETSTRUCT(amtup);

		if (!pg_am->amcanorderbyop)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
			errmsg("access method \"%s\" does not support ordering operators",
				   NameStr(pg_am->amname))));

		ReleaseSysCache(amtup);
	}
	else
	{
		/*
		 * Search operators must return boolean.
		 */
		if (opform->oprresult != BOOLOID)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("index search operators must return boolean")));
	}

	/*
	 * If lefttype/righttype isn't specified, use the operator's input types
	 */
	if (!OidIsValid(member->lefttype))
		member->lefttype = opform->oprleft;
	if (!OidIsValid(member->righttype))
		member->righttype = opform->oprright;

	ReleaseSysCache(optup);
}

/*
 * Determine the lefttype/righttype to assign to a support procedure,
 * and do any validity checking we can manage.
 */
static void
assignProcTypes(OpFamilyMember *member, Oid amoid, Oid typeoid)
{
	HeapTuple	proctup;
	Form_pg_proc procform;

	/* Fetch the procedure definition */
	proctup = SearchSysCache1(PROCOID, ObjectIdGetDatum(member->object));
	if (proctup == NULL)
		elog(ERROR, "cache lookup failed for function %u", member->object);
	procform = (Form_pg_proc) GETSTRUCT(proctup);

	/*
	 * btree support procs must be 2-arg procs returning int4; hash support
	 * procs must be 1-arg procs returning int4; otherwise we don't know.
	 */
	if (amoid == BTREE_AM_OID)
	{
		if (procform->pronargs != 2)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("btree procedures must have two arguments")));
		if (procform->prorettype != INT4OID)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("btree procedures must return integer")));

		/*
		 * If lefttype/righttype isn't specified, use the proc's input types
		 */
		if (!OidIsValid(member->lefttype))
			member->lefttype = procform->proargtypes.values[0];
		if (!OidIsValid(member->righttype))
			member->righttype = procform->proargtypes.values[1];
	}
	else if (amoid == HASH_AM_OID)
	{
		if (procform->pronargs != 1)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("hash procedures must have one argument")));
		if (procform->prorettype != INT4OID)
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("hash procedures must return integer")));

		/*
		 * If lefttype/righttype isn't specified, use the proc's input type
		 */
		if (!OidIsValid(member->lefttype))
			member->lefttype = procform->proargtypes.values[0];
		if (!OidIsValid(member->righttype))
			member->righttype = procform->proargtypes.values[0];
	}
	else
	{
		/*
		 * The default for GiST and GIN in CREATE OPERATOR CLASS is to use the
		 * class' opcintype as lefttype and righttype.  In CREATE or ALTER
		 * OPERATOR FAMILY, opcintype isn't available, so make the user
		 * specify the types.
		 */
		if (!OidIsValid(member->lefttype))
			member->lefttype = typeoid;
		if (!OidIsValid(member->righttype))
			member->righttype = typeoid;
		if (!OidIsValid(member->lefttype) || !OidIsValid(member->righttype))
			ereport(ERROR,
					(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
					 errmsg("associated data types must be specified for index support procedure")));
	}

	ReleaseSysCache(proctup);
}

/*
 * Add a new family member to the appropriate list, after checking for
 * duplicated strategy or proc number.
 */
static void
addFamilyMember(List **list, OpFamilyMember *member, bool isProc)
{
	ListCell   *l;

	foreach(l, *list)
	{
		OpFamilyMember *old = (OpFamilyMember *) lfirst(l);

		if (old->number == member->number &&
			old->lefttype == member->lefttype &&
			old->righttype == member->righttype)
		{
			if (isProc)
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						 errmsg("procedure number %d for (%s,%s) appears more than once",
								member->number,
								format_type_be(member->lefttype),
								format_type_be(member->righttype))));
			else
				ereport(ERROR,
						(errcode(ERRCODE_INVALID_OBJECT_DEFINITION),
						 errmsg("operator number %d for (%s,%s) appears more than once",
								member->number,
								format_type_be(member->lefttype),
								format_type_be(member->righttype))));
		}
	}
	*list = lappend(*list, member);
}

/*
 * Dump the operators to pg_amop
 *
 * We also make dependency entries in pg_depend for the opfamily entries.
 * If opclassoid is valid then make an INTERNAL dependency on that opclass,
 * else make an AUTO dependency on the opfamily.
 */
static void
storeOperators(List *opfamilyname, Oid amoid,
			   Oid opfamilyoid, Oid opclassoid,
			   List *operators, bool isAdd)
{
	Relation	rel;
	Datum		values[Natts_pg_amop];
	bool		nulls[Natts_pg_amop];
	HeapTuple	tup;
	Oid			entryoid;
	ObjectAddress myself,
				referenced;
	ListCell   *l;

	rel = heap_open(AccessMethodOperatorRelationId, RowExclusiveLock);

	foreach(l, operators)
	{
		OpFamilyMember *op = (OpFamilyMember *) lfirst(l);
		char		oppurpose;

		/*
		 * If adding to an existing family, check for conflict with an
		 * existing pg_amop entry (just to give a nicer error message)
		 */
		if (isAdd &&
			SearchSysCacheExists4(AMOPSTRATEGY,
								  ObjectIdGetDatum(opfamilyoid),
								  ObjectIdGetDatum(op->lefttype),
								  ObjectIdGetDatum(op->righttype),
								  Int16GetDatum(op->number)))
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("operator %d(%s,%s) already exists in operator family \"%s\"",
							op->number,
							format_type_be(op->lefttype),
							format_type_be(op->righttype),
							NameListToString(opfamilyname))));

		oppurpose = OidIsValid(op->sortfamily) ? AMOP_ORDER : AMOP_SEARCH;

		/* Create the pg_amop entry */
		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		values[Anum_pg_amop_amopfamily - 1] = ObjectIdGetDatum(opfamilyoid);
		values[Anum_pg_amop_amoplefttype - 1] = ObjectIdGetDatum(op->lefttype);
		values[Anum_pg_amop_amoprighttype - 1] = ObjectIdGetDatum(op->righttype);
		values[Anum_pg_amop_amopstrategy - 1] = Int16GetDatum(op->number);
		values[Anum_pg_amop_amoppurpose - 1] = CharGetDatum(oppurpose);
		values[Anum_pg_amop_amopopr - 1] = ObjectIdGetDatum(op->object);
		values[Anum_pg_amop_amopmethod - 1] = ObjectIdGetDatum(amoid);
		values[Anum_pg_amop_amopsortfamily - 1] = ObjectIdGetDatum(op->sortfamily);

		tup = heap_form_tuple(rel->rd_att, values, nulls);

		entryoid = simple_heap_insert(rel, tup);

		CatalogUpdateIndexes(rel, tup);

		heap_freetuple(tup);

		/* Make its dependencies */
		myself.classId = AccessMethodOperatorRelationId;
		myself.objectId = entryoid;
		myself.objectSubId = 0;

		referenced.classId = OperatorRelationId;
		referenced.objectId = op->object;
		referenced.objectSubId = 0;

		if (OidIsValid(opclassoid))
		{
			/* if contained in an opclass, use a NORMAL dep on operator */
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

			/* ... and an INTERNAL dep on the opclass */
			referenced.classId = OperatorClassRelationId;
			referenced.objectId = opclassoid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);
		}
		else
		{
			/* if "loose" in the opfamily, use a AUTO dep on operator */
			recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);

			/* ... and an AUTO dep on the opfamily */
			referenced.classId = OperatorFamilyRelationId;
			referenced.objectId = opfamilyoid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
		}

		/* A search operator also needs a dep on the referenced opfamily */
		if (OidIsValid(op->sortfamily))
		{
			referenced.classId = OperatorFamilyRelationId;
			referenced.objectId = op->sortfamily;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);
		}
	}

	heap_close(rel, RowExclusiveLock);
}

/*
 * Dump the procedures (support routines) to pg_amproc
 *
 * We also make dependency entries in pg_depend for the opfamily entries.
 * If opclassoid is valid then make an INTERNAL dependency on that opclass,
 * else make an AUTO dependency on the opfamily.
 */
static void
storeProcedures(List *opfamilyname, Oid amoid,
				Oid opfamilyoid, Oid opclassoid,
				List *procedures, bool isAdd)
{
	Relation	rel;
	Datum		values[Natts_pg_amproc];
	bool		nulls[Natts_pg_amproc];
	HeapTuple	tup;
	Oid			entryoid;
	ObjectAddress myself,
				referenced;
	ListCell   *l;

	rel = heap_open(AccessMethodProcedureRelationId, RowExclusiveLock);

	foreach(l, procedures)
	{
		OpFamilyMember *proc = (OpFamilyMember *) lfirst(l);

		/*
		 * If adding to an existing family, check for conflict with an
		 * existing pg_amproc entry (just to give a nicer error message)
		 */
		if (isAdd &&
			SearchSysCacheExists4(AMPROCNUM,
								  ObjectIdGetDatum(opfamilyoid),
								  ObjectIdGetDatum(proc->lefttype),
								  ObjectIdGetDatum(proc->righttype),
								  Int16GetDatum(proc->number)))
			ereport(ERROR,
					(errcode(ERRCODE_DUPLICATE_OBJECT),
					 errmsg("function %d(%s,%s) already exists in operator family \"%s\"",
							proc->number,
							format_type_be(proc->lefttype),
							format_type_be(proc->righttype),
							NameListToString(opfamilyname))));

		/* Create the pg_amproc entry */
		memset(values, 0, sizeof(values));
		memset(nulls, false, sizeof(nulls));

		values[Anum_pg_amproc_amprocfamily - 1] = ObjectIdGetDatum(opfamilyoid);
		values[Anum_pg_amproc_amproclefttype - 1] = ObjectIdGetDatum(proc->lefttype);
		values[Anum_pg_amproc_amprocrighttype - 1] = ObjectIdGetDatum(proc->righttype);
		values[Anum_pg_amproc_amprocnum - 1] = Int16GetDatum(proc->number);
		values[Anum_pg_amproc_amproc - 1] = ObjectIdGetDatum(proc->object);

		tup = heap_form_tuple(rel->rd_att, values, nulls);

		entryoid = simple_heap_insert(rel, tup);

		CatalogUpdateIndexes(rel, tup);

		heap_freetuple(tup);

		/* Make its dependencies */
		myself.classId = AccessMethodProcedureRelationId;
		myself.objectId = entryoid;
		myself.objectSubId = 0;

		referenced.classId = ProcedureRelationId;
		referenced.objectId = proc->object;
		referenced.objectSubId = 0;

		if (OidIsValid(opclassoid))
		{
			/* if contained in an opclass, use a NORMAL dep on procedure */
			recordDependencyOn(&myself, &referenced, DEPENDENCY_NORMAL);

			/* ... and an INTERNAL dep on the opclass */
			referenced.classId = OperatorClassRelationId;
			referenced.objectId = opclassoid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_INTERNAL);
		}
		else
		{
			/* if "loose" in the opfamily, use a AUTO dep on procedure */
			recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);

			/* ... and an AUTO dep on the opfamily */
			referenced.classId = OperatorFamilyRelationId;
			referenced.objectId = opfamilyoid;
			referenced.objectSubId = 0;
			recordDependencyOn(&myself, &referenced, DEPENDENCY_AUTO);
		}
	}

	heap_close(rel, RowExclusiveLock);
}


/*
 * Remove operator entries from an opfamily.
 *
 * Note: this is only allowed for "loose" members of an opfamily, hence
 * behavior is always RESTRICT.
 */
static void
dropOperators(List *opfamilyname, Oid amoid, Oid opfamilyoid,
			  List *operators)
{
	ListCell   *l;

	foreach(l, operators)
	{
		OpFamilyMember *op = (OpFamilyMember *) lfirst(l);
		Oid			amopid;
		ObjectAddress object;

		amopid = GetSysCacheOid4(AMOPSTRATEGY,
								 ObjectIdGetDatum(opfamilyoid),
								 ObjectIdGetDatum(op->lefttype),
								 ObjectIdGetDatum(op->righttype),
								 Int16GetDatum(op->number));
		if (!OidIsValid(amopid))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("operator %d(%s,%s) does not exist in operator family \"%s\"",
							op->number,
							format_type_be(op->lefttype),
							format_type_be(op->righttype),
							NameListToString(opfamilyname))));

		object.classId = AccessMethodOperatorRelationId;
		object.objectId = amopid;
		object.objectSubId = 0;

		performDeletion(&object, DROP_RESTRICT);
	}
}

/*
 * Remove procedure entries from an opfamily.
 *
 * Note: this is only allowed for "loose" members of an opfamily, hence
 * behavior is always RESTRICT.
 */
static void
dropProcedures(List *opfamilyname, Oid amoid, Oid opfamilyoid,
			   List *procedures)
{
	ListCell   *l;

	foreach(l, procedures)
	{
		OpFamilyMember *op = (OpFamilyMember *) lfirst(l);
		Oid			amprocid;
		ObjectAddress object;

		amprocid = GetSysCacheOid4(AMPROCNUM,
								   ObjectIdGetDatum(opfamilyoid),
								   ObjectIdGetDatum(op->lefttype),
								   ObjectIdGetDatum(op->righttype),
								   Int16GetDatum(op->number));
		if (!OidIsValid(amprocid))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("function %d(%s,%s) does not exist in operator family \"%s\"",
							op->number,
							format_type_be(op->lefttype),
							format_type_be(op->righttype),
							NameListToString(opfamilyname))));

		object.classId = AccessMethodProcedureRelationId;
		object.objectId = amprocid;
		object.objectSubId = 0;

		performDeletion(&object, DROP_RESTRICT);
	}
}


/*
 * RemoveOpClass
 *		Deletes an opclass.
 */
void
RemoveOpClass(RemoveOpClassStmt *stmt)
{
	Oid			amID,
				opcID;
	HeapTuple	tuple;
	ObjectAddress object;

	/* Get the access method's OID. */
	amID = get_am_oid(stmt->amname, false);

	/* Look up the opclass. */
	tuple = OpClassCacheLookup(amID, stmt->opclassname, stmt->missing_ok);
	if (!HeapTupleIsValid(tuple))
	{
		ereport(NOTICE,
				(errmsg("operator class \"%s\" does not exist for access method \"%s\"",
						NameListToString(stmt->opclassname), stmt->amname)));
		return;
	}

	opcID = HeapTupleGetOid(tuple);

	/* Permission check: must own opclass or its namespace */
	if (!pg_opclass_ownercheck(opcID, GetUserId()) &&
		!pg_namespace_ownercheck(((Form_pg_opclass) GETSTRUCT(tuple))->opcnamespace,
								 GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPCLASS,
					   NameListToString(stmt->opclassname));

	ReleaseSysCache(tuple);

	/*
	 * Do the deletion
	 */
	object.classId = OperatorClassRelationId;
	object.objectId = opcID;
	object.objectSubId = 0;

	performDeletion(&object, stmt->behavior);
}

/*
 * RemoveOpFamily
 *		Deletes an opfamily.
 */
void
RemoveOpFamily(RemoveOpFamilyStmt *stmt)
{
	Oid			amID,
				opfID;
	HeapTuple	tuple;
	ObjectAddress object;

	/*
	 * Get the access method's OID.
	 */
	amID = get_am_oid(stmt->amname, false);

	/*
	 * Look up the opfamily.
	 */
	tuple = OpFamilyCacheLookup(amID, stmt->opfamilyname, stmt->missing_ok);
	if (!HeapTupleIsValid(tuple))
	{
		ereport(NOTICE,
				(errmsg("operator family \"%s\" does not exist for access method \"%s\", skipping",
				   NameListToString(stmt->opfamilyname), stmt->amname)));
		return;
	}

	opfID = HeapTupleGetOid(tuple);

	/* Permission check: must own opfamily or its namespace */
	if (!pg_opfamily_ownercheck(opfID, GetUserId()) &&
		!pg_namespace_ownercheck(((Form_pg_opfamily) GETSTRUCT(tuple))->opfnamespace,
								 GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPFAMILY,
					   NameListToString(stmt->opfamilyname));

	ReleaseSysCache(tuple);

	/*
	 * Do the deletion
	 */
	object.classId = OperatorFamilyRelationId;
	object.objectId = opfID;
	object.objectSubId = 0;

	performDeletion(&object, stmt->behavior);
}


/*
 * Deletion subroutines for use by dependency.c.
 */
void
RemoveOpFamilyById(Oid opfamilyOid)
{
	Relation	rel;
	HeapTuple	tup;

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	tup = SearchSysCache1(OPFAMILYOID, ObjectIdGetDatum(opfamilyOid));
	if (!HeapTupleIsValid(tup)) /* should not happen */
		elog(ERROR, "cache lookup failed for opfamily %u", opfamilyOid);

	simple_heap_delete(rel, &tup->t_self);

	ReleaseSysCache(tup);

	heap_close(rel, RowExclusiveLock);
}

void
RemoveOpClassById(Oid opclassOid)
{
	Relation	rel;
	HeapTuple	tup;

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	tup = SearchSysCache1(CLAOID, ObjectIdGetDatum(opclassOid));
	if (!HeapTupleIsValid(tup)) /* should not happen */
		elog(ERROR, "cache lookup failed for opclass %u", opclassOid);

	simple_heap_delete(rel, &tup->t_self);

	ReleaseSysCache(tup);

	heap_close(rel, RowExclusiveLock);
}

void
RemoveAmOpEntryById(Oid entryOid)
{
	Relation	rel;
	HeapTuple	tup;
	ScanKeyData skey[1];
	SysScanDesc scan;

	ScanKeyInit(&skey[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(entryOid));

	rel = heap_open(AccessMethodOperatorRelationId, RowExclusiveLock);

	scan = systable_beginscan(rel, AccessMethodOperatorOidIndexId, true,
							  SnapshotNow, 1, skey);

	/* we expect exactly one match */
	tup = systable_getnext(scan);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "could not find tuple for amop entry %u", entryOid);

	simple_heap_delete(rel, &tup->t_self);

	systable_endscan(scan);
	heap_close(rel, RowExclusiveLock);
}

void
RemoveAmProcEntryById(Oid entryOid)
{
	Relation	rel;
	HeapTuple	tup;
	ScanKeyData skey[1];
	SysScanDesc scan;

	ScanKeyInit(&skey[0],
				ObjectIdAttributeNumber,
				BTEqualStrategyNumber, F_OIDEQ,
				ObjectIdGetDatum(entryOid));

	rel = heap_open(AccessMethodProcedureRelationId, RowExclusiveLock);

	scan = systable_beginscan(rel, AccessMethodProcedureOidIndexId, true,
							  SnapshotNow, 1, skey);

	/* we expect exactly one match */
	tup = systable_getnext(scan);
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "could not find tuple for amproc entry %u", entryOid);

	simple_heap_delete(rel, &tup->t_self);

	systable_endscan(scan);
	heap_close(rel, RowExclusiveLock);
}


/*
 * Rename opclass
 */
void
RenameOpClass(List *name, const char *access_method, const char *newname)
{
	Oid			opcOid;
	Oid			amOid;
	Oid			namespaceOid;
	HeapTuple	origtup;
	HeapTuple	tup;
	Relation	rel;
	AclResult	aclresult;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	/* Look up the opclass. */
	origtup = OpClassCacheLookup(amOid, name, false);
	tup = heap_copytuple(origtup);
	ReleaseSysCache(origtup);
	opcOid = HeapTupleGetOid(tup);
	namespaceOid = ((Form_pg_opclass) GETSTRUCT(tup))->opcnamespace;

	/* make sure the new name doesn't exist */
	if (SearchSysCacheExists3(CLAAMNAMENSP,
							  ObjectIdGetDatum(amOid),
							  CStringGetDatum(newname),
							  ObjectIdGetDatum(namespaceOid)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("operator class \"%s\" for access method \"%s\" already exists in schema \"%s\"",
						newname, access_method,
						get_namespace_name(namespaceOid))));
	}

	/* must be owner */
	if (!pg_opclass_ownercheck(opcOid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPCLASS,
					   NameListToString(name));

	/* must have CREATE privilege on namespace */
	aclresult = pg_namespace_aclcheck(namespaceOid, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
					   get_namespace_name(namespaceOid));

	/* rename */
	namestrcpy(&(((Form_pg_opclass) GETSTRUCT(tup))->opcname), newname);
	simple_heap_update(rel, &tup->t_self, tup);
	CatalogUpdateIndexes(rel, tup);

	heap_close(rel, NoLock);
	heap_freetuple(tup);
}

/*
 * Rename opfamily
 */
void
RenameOpFamily(List *name, const char *access_method, const char *newname)
{
	Oid			opfOid;
	Oid			amOid;
	Oid			namespaceOid;
	char	   *schemaname;
	char	   *opfname;
	HeapTuple	tup;
	Relation	rel;
	AclResult	aclresult;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	/*
	 * Look up the opfamily
	 */
	DeconstructQualifiedName(name, &schemaname, &opfname);

	if (schemaname)
	{
		namespaceOid = LookupExplicitNamespace(schemaname);

		tup = SearchSysCacheCopy3(OPFAMILYAMNAMENSP,
								  ObjectIdGetDatum(amOid),
								  PointerGetDatum(opfname),
								  ObjectIdGetDatum(namespaceOid));
		if (!HeapTupleIsValid(tup))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("operator family \"%s\" does not exist for access method \"%s\"",
							opfname, access_method)));

		opfOid = HeapTupleGetOid(tup);
	}
	else
	{
		opfOid = OpfamilynameGetOpfid(amOid, opfname);
		if (!OidIsValid(opfOid))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("operator family \"%s\" does not exist for access method \"%s\"",
							opfname, access_method)));

		tup = SearchSysCacheCopy1(OPFAMILYOID, ObjectIdGetDatum(opfOid));
		if (!HeapTupleIsValid(tup))		/* should not happen */
			elog(ERROR, "cache lookup failed for opfamily %u", opfOid);

		namespaceOid = ((Form_pg_opfamily) GETSTRUCT(tup))->opfnamespace;
	}

	/* make sure the new name doesn't exist */
	if (SearchSysCacheExists3(OPFAMILYAMNAMENSP,
							  ObjectIdGetDatum(amOid),
							  CStringGetDatum(newname),
							  ObjectIdGetDatum(namespaceOid)))
	{
		ereport(ERROR,
				(errcode(ERRCODE_DUPLICATE_OBJECT),
				 errmsg("operator family \"%s\" for access method \"%s\" already exists in schema \"%s\"",
						newname, access_method,
						get_namespace_name(namespaceOid))));
	}

	/* must be owner */
	if (!pg_opfamily_ownercheck(opfOid, GetUserId()))
		aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPFAMILY,
					   NameListToString(name));

	/* must have CREATE privilege on namespace */
	aclresult = pg_namespace_aclcheck(namespaceOid, GetUserId(), ACL_CREATE);
	if (aclresult != ACLCHECK_OK)
		aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
					   get_namespace_name(namespaceOid));

	/* rename */
	namestrcpy(&(((Form_pg_opfamily) GETSTRUCT(tup))->opfname), newname);
	simple_heap_update(rel, &tup->t_self, tup);
	CatalogUpdateIndexes(rel, tup);

	heap_close(rel, NoLock);
	heap_freetuple(tup);
}

/*
 * Change opclass owner by name
 */
void
AlterOpClassOwner(List *name, const char *access_method, Oid newOwnerId)
{
	Oid			amOid;
	Relation	rel;
	HeapTuple	tup;
	HeapTuple	origtup;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	/* Look up the opclass. */
	origtup = OpClassCacheLookup(amOid, name, false);
	tup = heap_copytuple(origtup);
	ReleaseSysCache(origtup);

	AlterOpClassOwner_internal(rel, tup, newOwnerId);

	heap_freetuple(tup);
	heap_close(rel, NoLock);
}

/*
 * Change operator class owner, specified by OID
 */
void
AlterOpClassOwner_oid(Oid opclassOid, Oid newOwnerId)
{
	HeapTuple	tup;
	Relation	rel;

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	tup = SearchSysCacheCopy1(CLAOID, ObjectIdGetDatum(opclassOid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for opclass %u", opclassOid);

	AlterOpClassOwner_internal(rel, tup, newOwnerId);

	heap_freetuple(tup);
	heap_close(rel, NoLock);
}

/*
 * The first parameter is pg_opclass, opened and suitably locked.  The second
 * parameter is a copy of the tuple from pg_opclass we want to modify.
 */
static void
AlterOpClassOwner_internal(Relation rel, HeapTuple tup, Oid newOwnerId)
{
	Oid			namespaceOid;
	AclResult	aclresult;
	Form_pg_opclass opcForm;

	Assert(tup->t_tableOid == OperatorClassRelationId);
	Assert(RelationGetRelid(rel) == OperatorClassRelationId);

	opcForm = (Form_pg_opclass) GETSTRUCT(tup);

	namespaceOid = opcForm->opcnamespace;

	/*
	 * If the new owner is the same as the existing owner, consider the
	 * command to have succeeded.  This is for dump restoration purposes.
	 */
	if (opcForm->opcowner != newOwnerId)
	{
		/* Superusers can always do it */
		if (!superuser())
		{
			/* Otherwise, must be owner of the existing object */
			if (!pg_opclass_ownercheck(HeapTupleGetOid(tup), GetUserId()))
				aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPCLASS,
							   NameStr(opcForm->opcname));

			/* Must be able to become new owner */
			check_is_member_of_role(GetUserId(), newOwnerId);

			/* New owner must have CREATE privilege on namespace */
			aclresult = pg_namespace_aclcheck(namespaceOid, newOwnerId,
											  ACL_CREATE);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
							   get_namespace_name(namespaceOid));
		}

		/*
		 * Modify the owner --- okay to scribble on tup because it's a copy
		 */
		opcForm->opcowner = newOwnerId;

		simple_heap_update(rel, &tup->t_self, tup);

		CatalogUpdateIndexes(rel, tup);

		/* Update owner dependency reference */
		changeDependencyOnOwner(OperatorClassRelationId, HeapTupleGetOid(tup),
								newOwnerId);
	}
}

/*
 * ALTER OPERATOR CLASS any_name USING access_method SET SCHEMA name
 */
void
AlterOpClassNamespace(List *name, char *access_method, const char *newschema)
{
	Oid			amOid;
	Relation	rel;
	Oid			opclassOid;
	Oid			nspOid;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	/* Look up the opclass */
	opclassOid = get_opclass_oid(amOid, name, false);

	/* get schema OID */
	nspOid = LookupCreationNamespace(newschema);

	AlterObjectNamespace(rel, CLAOID, -1,
						 opclassOid, nspOid,
						 Anum_pg_opclass_opcname,
						 Anum_pg_opclass_opcnamespace,
						 Anum_pg_opclass_opcowner,
						 ACL_KIND_OPCLASS);

	heap_close(rel, RowExclusiveLock);
}

Oid
AlterOpClassNamespace_oid(Oid opclassOid, Oid newNspOid)
{
	Oid			oldNspOid;
	Relation	rel;

	rel = heap_open(OperatorClassRelationId, RowExclusiveLock);

	oldNspOid =
		AlterObjectNamespace(rel, CLAOID, -1,
							 opclassOid, newNspOid,
							 Anum_pg_opclass_opcname,
							 Anum_pg_opclass_opcnamespace,
							 Anum_pg_opclass_opcowner,
							 ACL_KIND_OPCLASS);

	heap_close(rel, RowExclusiveLock);

	return oldNspOid;
}

/*
 * Change opfamily owner by name
 */
void
AlterOpFamilyOwner(List *name, const char *access_method, Oid newOwnerId)
{
	Oid			amOid;
	Relation	rel;
	HeapTuple	tup;
	char	   *opfname;
	char	   *schemaname;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	/*
	 * Look up the opfamily
	 */
	DeconstructQualifiedName(name, &schemaname, &opfname);

	if (schemaname)
	{
		Oid			namespaceOid;

		namespaceOid = LookupExplicitNamespace(schemaname);

		tup = SearchSysCacheCopy3(OPFAMILYAMNAMENSP,
								  ObjectIdGetDatum(amOid),
								  PointerGetDatum(opfname),
								  ObjectIdGetDatum(namespaceOid));
		if (!HeapTupleIsValid(tup))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("operator family \"%s\" does not exist for access method \"%s\"",
							opfname, access_method)));
	}
	else
	{
		Oid			opfOid;

		opfOid = OpfamilynameGetOpfid(amOid, opfname);
		if (!OidIsValid(opfOid))
			ereport(ERROR,
					(errcode(ERRCODE_UNDEFINED_OBJECT),
					 errmsg("operator family \"%s\" does not exist for access method \"%s\"",
							opfname, access_method)));

		tup = SearchSysCacheCopy1(OPFAMILYOID, ObjectIdGetDatum(opfOid));
		if (!HeapTupleIsValid(tup))		/* should not happen */
			elog(ERROR, "cache lookup failed for opfamily %u", opfOid);
	}

	AlterOpFamilyOwner_internal(rel, tup, newOwnerId);

	heap_freetuple(tup);
	heap_close(rel, NoLock);
}

/*
 * Change operator family owner, specified by OID
 */
void
AlterOpFamilyOwner_oid(Oid opfamilyOid, Oid newOwnerId)
{
	HeapTuple	tup;
	Relation	rel;

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	tup = SearchSysCacheCopy1(OPFAMILYOID, ObjectIdGetDatum(opfamilyOid));
	if (!HeapTupleIsValid(tup))
		elog(ERROR, "cache lookup failed for opfamily %u", opfamilyOid);

	AlterOpFamilyOwner_internal(rel, tup, newOwnerId);

	heap_freetuple(tup);
	heap_close(rel, NoLock);
}

/*
 * The first parameter is pg_opfamily, opened and suitably locked.	The second
 * parameter is a copy of the tuple from pg_opfamily we want to modify.
 */
static void
AlterOpFamilyOwner_internal(Relation rel, HeapTuple tup, Oid newOwnerId)
{
	Oid			namespaceOid;
	AclResult	aclresult;
	Form_pg_opfamily opfForm;

	Assert(tup->t_tableOid == OperatorFamilyRelationId);
	Assert(RelationGetRelid(rel) == OperatorFamilyRelationId);

	opfForm = (Form_pg_opfamily) GETSTRUCT(tup);

	namespaceOid = opfForm->opfnamespace;

	/*
	 * If the new owner is the same as the existing owner, consider the
	 * command to have succeeded.  This is for dump restoration purposes.
	 */
	if (opfForm->opfowner != newOwnerId)
	{
		/* Superusers can always do it */
		if (!superuser())
		{
			/* Otherwise, must be owner of the existing object */
			if (!pg_opfamily_ownercheck(HeapTupleGetOid(tup), GetUserId()))
				aclcheck_error(ACLCHECK_NOT_OWNER, ACL_KIND_OPFAMILY,
							   NameStr(opfForm->opfname));

			/* Must be able to become new owner */
			check_is_member_of_role(GetUserId(), newOwnerId);

			/* New owner must have CREATE privilege on namespace */
			aclresult = pg_namespace_aclcheck(namespaceOid, newOwnerId,
											  ACL_CREATE);
			if (aclresult != ACLCHECK_OK)
				aclcheck_error(aclresult, ACL_KIND_NAMESPACE,
							   get_namespace_name(namespaceOid));
		}

		/*
		 * Modify the owner --- okay to scribble on tup because it's a copy
		 */
		opfForm->opfowner = newOwnerId;

		simple_heap_update(rel, &tup->t_self, tup);

		CatalogUpdateIndexes(rel, tup);

		/* Update owner dependency reference */
		changeDependencyOnOwner(OperatorFamilyRelationId, HeapTupleGetOid(tup),
								newOwnerId);
	}
}

/*
 * get_am_oid - given an access method name, look up the OID
 *
 * If missing_ok is false, throw an error if access method not found.  If
 * true, just return InvalidOid.
 */
Oid
get_am_oid(const char *amname, bool missing_ok)
{
	Oid			oid;

	oid = GetSysCacheOid1(AMNAME, CStringGetDatum(amname));
	if (!OidIsValid(oid) && !missing_ok)
		ereport(ERROR,
				(errcode(ERRCODE_UNDEFINED_OBJECT),
				 errmsg("access method \"%s\" does not exist", amname)));
	return oid;
}

/*
 * ALTER OPERATOR FAMILY any_name USING access_method SET SCHEMA name
 */
void
AlterOpFamilyNamespace(List *name, char *access_method, const char *newschema)
{
	Oid			amOid;
	Relation	rel;
	Oid			opfamilyOid;
	Oid			nspOid;

	amOid = get_am_oid(access_method, false);

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	/* Look up the opfamily */
	opfamilyOid = get_opfamily_oid(amOid, name, false);

	/* get schema OID */
	nspOid = LookupCreationNamespace(newschema);

	AlterObjectNamespace(rel, OPFAMILYOID, -1,
						 opfamilyOid, nspOid,
						 Anum_pg_opfamily_opfname,
						 Anum_pg_opfamily_opfnamespace,
						 Anum_pg_opfamily_opfowner,
						 ACL_KIND_OPFAMILY);

	heap_close(rel, RowExclusiveLock);
}

Oid
AlterOpFamilyNamespace_oid(Oid opfamilyOid, Oid newNspOid)
{
	Oid			oldNspOid;
	Relation	rel;

	rel = heap_open(OperatorFamilyRelationId, RowExclusiveLock);

	oldNspOid =
		AlterObjectNamespace(rel, OPFAMILYOID, -1,
							 opfamilyOid, newNspOid,
							 Anum_pg_opfamily_opfname,
							 Anum_pg_opfamily_opfnamespace,
							 Anum_pg_opfamily_opfowner,
							 ACL_KIND_OPFAMILY);

	heap_close(rel, RowExclusiveLock);

	return oldNspOid;
}
