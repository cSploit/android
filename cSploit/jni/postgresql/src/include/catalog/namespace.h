/*-------------------------------------------------------------------------
 *
 * namespace.h
 *	  prototypes for functions in backend/catalog/namespace.c
 *
 *
 * Portions Copyright (c) 1996-2011, PostgreSQL Global Development Group
 * Portions Copyright (c) 1994, Regents of the University of California
 *
 * src/include/catalog/namespace.h
 *
 *-------------------------------------------------------------------------
 */
#ifndef NAMESPACE_H
#define NAMESPACE_H

#include "nodes/primnodes.h"


/*
 *	This structure holds a list of possible functions or operators
 *	found by namespace lookup.	Each function/operator is identified
 *	by OID and by argument types; the list must be pruned by type
 *	resolution rules that are embodied in the parser, not here.
 *	See FuncnameGetCandidates's comments for more info.
 */
typedef struct _FuncCandidateList
{
	struct _FuncCandidateList *next;
	int			pathpos;		/* for internal use of namespace lookup */
	Oid			oid;			/* the function or operator's OID */
	int			nargs;			/* number of arg types returned */
	int			nvargs;			/* number of args to become variadic array */
	int			ndargs;			/* number of defaulted args */
	int		   *argnumbers;		/* args' positional indexes, if named call */
	Oid			args[1];		/* arg types --- VARIABLE LENGTH ARRAY */
}	*FuncCandidateList;	/* VARIABLE LENGTH STRUCT */

/*
 *	Structure for xxxOverrideSearchPath functions
 */
typedef struct OverrideSearchPath
{
	List	   *schemas;		/* OIDs of explicitly named schemas */
	bool		addCatalog;		/* implicitly prepend pg_catalog? */
	bool		addTemp;		/* implicitly prepend temp schema? */
} OverrideSearchPath;


extern Oid	RangeVarGetRelid(const RangeVar *relation, bool failOK);
extern Oid	RangeVarGetCreationNamespace(const RangeVar *newRelation);
extern Oid	RangeVarGetAndCheckCreationNamespace(const RangeVar *newRelation);
extern void RangeVarAdjustRelationPersistence(RangeVar *newRelation, Oid nspid);
extern Oid	RelnameGetRelid(const char *relname);
extern bool RelationIsVisible(Oid relid);

extern Oid	TypenameGetTypid(const char *typname);
extern bool TypeIsVisible(Oid typid);

extern FuncCandidateList FuncnameGetCandidates(List *names,
					  int nargs, List *argnames,
					  bool expand_variadic,
					  bool expand_defaults);
extern bool FunctionIsVisible(Oid funcid);

extern Oid	OpernameGetOprid(List *names, Oid oprleft, Oid oprright);
extern FuncCandidateList OpernameGetCandidates(List *names, char oprkind);
extern bool OperatorIsVisible(Oid oprid);

extern Oid	OpclassnameGetOpcid(Oid amid, const char *opcname);
extern bool OpclassIsVisible(Oid opcid);

extern Oid	OpfamilynameGetOpfid(Oid amid, const char *opfname);
extern bool OpfamilyIsVisible(Oid opfid);

extern Oid	CollationGetCollid(const char *collname);
extern bool CollationIsVisible(Oid collid);

extern Oid	ConversionGetConid(const char *conname);
extern bool ConversionIsVisible(Oid conid);

extern Oid	get_ts_parser_oid(List *names, bool missing_ok);
extern bool TSParserIsVisible(Oid prsId);

extern Oid	get_ts_dict_oid(List *names, bool missing_ok);
extern bool TSDictionaryIsVisible(Oid dictId);

extern Oid	get_ts_template_oid(List *names, bool missing_ok);
extern bool TSTemplateIsVisible(Oid tmplId);

extern Oid	get_ts_config_oid(List *names, bool missing_ok);
extern bool TSConfigIsVisible(Oid cfgid);

extern void DeconstructQualifiedName(List *names,
						 char **nspname_p,
						 char **objname_p);
extern Oid	LookupNamespaceNoError(const char *nspname);
extern Oid	LookupExplicitNamespace(const char *nspname);
extern Oid	get_namespace_oid(const char *nspname, bool missing_ok);

extern Oid	LookupCreationNamespace(const char *nspname);
extern void CheckSetNamespace(Oid oldNspOid, Oid nspOid, Oid classid,
				  Oid objid);
extern Oid	QualifiedNameGetCreationNamespace(List *names, char **objname_p);
extern RangeVar *makeRangeVarFromNameList(List *names);
extern char *NameListToString(List *names);
extern char *NameListToQuotedString(List *names);

extern bool isTempNamespace(Oid namespaceId);
extern bool isTempToastNamespace(Oid namespaceId);
extern bool isTempOrToastNamespace(Oid namespaceId);
extern bool isAnyTempNamespace(Oid namespaceId);
extern bool isOtherTempNamespace(Oid namespaceId);
extern int	GetTempNamespaceBackendId(Oid namespaceId);
extern Oid	GetTempToastNamespace(void);
extern void ResetTempTableNamespace(void);

extern OverrideSearchPath *GetOverrideSearchPath(MemoryContext context);
extern void PushOverrideSearchPath(OverrideSearchPath *newpath);
extern void PopOverrideSearchPath(void);

extern Oid	get_collation_oid(List *collname, bool missing_ok);
extern Oid	get_conversion_oid(List *conname, bool missing_ok);
extern Oid	FindDefaultConversionProc(int4 for_encoding, int4 to_encoding);

/* initialization & transaction cleanup code */
extern void InitializeSearchPath(void);
extern void AtEOXact_Namespace(bool isCommit);
extern void AtEOSubXact_Namespace(bool isCommit, SubTransactionId mySubid,
					  SubTransactionId parentSubid);

/* stuff for search_path GUC variable */
extern char *namespace_search_path;

extern List *fetch_search_path(bool includeImplicit);
extern int	fetch_search_path_array(Oid *sarray, int sarray_len);

#endif   /* NAMESPACE_H */
