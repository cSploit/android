/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Mararati.
 * Portions Copyright 2004 Mark Adamson.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati and Mark Adamson.
 */
/*
 * The following changes have been addressed:
 *	 
 * Enhancements:
 *   - re-styled code for better readability
 *   - upgraded backend API to reflect recent changes
 *   - LDAP schema is checked when loading SQL/LDAP mapping
 *   - AttributeDescription/ObjectClass pointers used for more efficient
 *     mapping lookup
 *   - bervals used where string length is required often
 *   - atomized write operations by committing at the end of each operation
 *     and defaulting connection closure to rollback
 *   - added LDAP access control to write operations
 *   - fully implemented modrdn (with rdn attrs change, deleteoldrdn,
 *     access check, parent/children check and more)
 *   - added parent access control, children control to delete operation
 *   - added structuralObjectClass operational attribute check and
 *     value return on search
 *   - added hasSubordinate operational attribute on demand
 *   - search limits are appropriately enforced
 *   - function backsql_strcat() has been made more efficient
 *   - concat function has been made configurable by means of a pattern
 *   - added config switches:
 *       - fail_if_no_mapping	write operations fail if there is no mapping
 *       - has_ldapinfo_dn_ru	overrides autodetect
 *       - concat_pattern	a string containing two '?' is used
 * 				(note that "?||?" should be more portable
 * 				than builtin function "CONCAT(?,?)")
 *       - strcast_func		cast of string constants in "SELECT DISTINCT
 *				statements (needed by PostgreSQL)
 *       - upper_needs_cast	cast the argument of upper when required
 * 				(basically when building dn substring queries)
 *   - added noop control
 *   - added values return filter control
 *   - hasSubordinate can be used in search filters (with limitations)
 *   - eliminated oc->name; use oc->oc->soc_cname instead
 * 
 * Todo:
 *   - add security checks for SQL statements that can be injected (?)
 *   - re-test with previously supported RDBMs
 *   - replace dn_ru and so with normalized dn (no need for upper() and so
 *     in dn match)
 *   - implement a backsql_normalize() function to replace the upper()
 *     conversion routines
 *   - note that subtree deletion, subtree renaming and so could be easily
 *     implemented (rollback and consistency checks are available :)
 *   - implement "lastmod" and other operational stuff (ldap_entries table ?)
 *   - check how to allow multiple operations with one statement, to remove
 *     BACKSQL_REALLOC_STMT from modify.c (a more recent unixODBC lib?)
 */
/*
 * Improvements submitted by (ITS#3432)
 *
 * 1. id_query.patch		applied (with changes)
 * 2. shortcut.patch		applied (reworked)
 * 3. create_hint.patch		applied
 * 4. count_query.patch		applied (reworked)
 * 5. returncodes.patch		applied (with sanity checks)
 * 6. connpool.patch		under evaluation
 * 7. modoc.patch		under evaluation (requires
 * 				manageDSAit and "manage"
 * 				access privileges)
 * 8. miscfixes.patch		applied (reworked; other
 *				operations need to load the
 *				entire entry for ACL purposes;
 *				see ITS#3480, now fixed)
 *
 * original description:

         Changes that were made to the SQL backend.

The patches were made against 2.2.18 and can be applied individually,
but would best be applied in the numerical order of the file names.
A synopsis of each patch is given here:


1. Added an option to set SQL query for the "id_query" operation.

2. Added an option to the SQL backend called "use_subtree_shortcut".
When a search is performed, the SQL query includes a WHERE clause
which says the DN must be "LIKE %<searchbase>".  The LIKE operation
can be slow in an RDBM. This shortcut option says that if the
searchbase of the LDAP search is the root DN of the SQL backend,
and thus all objects will match the LIKE operator, do not include
the "LIKE %<searchbase>" clause in the SQL query (it is replaced
instead by the always true "1=1" clause to keep the "AND"'s 
working correctly).  This option is off by default, and should be
turned on only if all objects to be found in the RDBM are under the
same root DN. Multiple backends working within the same RDBM table
space would encounter problems. LDAP searches whose searchbase are
not at the root DN will bypass this shortcut and employ the LIKE 
clause.

3. Added a "create_hint" column to ldap_oc_mappings table. Allows
taking the value of an attr named in "create_hint" and passing it to
the create_proc procedure.  This is necessary for when an objectClass's
table is partition indexed by some indexing column and thus the value
in that indexing column cannot change after the row is created. The
value for the indexed column is passed into the create_proc, which
uses it to fill in the indexed column as the new row is created.

4. When loading the values of an attribute, the count(*) of the number
of values is fetched first and memory is allocated for the array of
values and normalized values. The old system of loading the values one
by one and running realloc() on the array of values and normalized
values each time was badly fragmenting memory. The array of values and
normalized values would be side by side in memory, and realloc()'ing
them over and over would force them to leapfrog each other through all
of available memory. Attrs with a large number of values could not be
loaded without crashing the slapd daemon.

5. Added code to interpret the value returned by stored procedures
which have expect_return set. Returned value is interpreted as an LDAP
return code. This allows the distinction between the SQL failing to
execute and the SQL running to completion and returning an error code
which can indicate a policy violation.

6. Added RDBM connection pooling. Once an operation is finished the
connection to the RDBM is returned to a pool rather than closing.
Allows the next operation to skip the initialization and authentication
phases of contacting the RDBM. Also, if licensing with ODBC places
a limit on the number of connections, an LDAP thread can block waiting
for another thread to finish, so that no LDAP errors are returned
for having more LDAP connections than allowed RDBM connections. An
RDBM connection which receives an SQL error is marked as "tainted"
so that it will be closed rather than returned to the pool.
  Also, RDBM connections must be bound to a given LDAP connection AND
operation number, and NOT just the connection number.  Asynchronous
LDAP clients can have multiple simultaneous LDAP operations which
should not share the same RDBM connection.  A given LDAP operation can
even make multiple SQL operations (e.g. a BIND operation which
requires SASL to perform an LDAP search to convert the SASL ID to an
LDAP DN), so each RDBM connection now has a refcount that must reach
zero before the connection is returned to the free pool.

7. Added ability to change the objectClass of an object. Required 
considerable work to copy all attributes out of old object and into
new object.  Does a schema check before proceeding.  Creates a new
object, fills it in, deletes the old object, then changes the 
oc_map_id and keyval of the entry in the "ldap_entries" table.

8.  Generic fixes. Includes initializing pointers before they
get used in error branch cases, pointer checks before dereferencing,
resetting a return code to success after a COMPARE op, sealing
memory leaks, and in search.c, changing some of the "1=1" tests to
"2=2", "3=3", etc so that when reading slapd trace output, the 
location in the source code where the x=x test was added to the SQL
can be easily distinguished.
 */

#ifndef __BACKSQL_H__
#define __BACKSQL_H__

/* former sql-types.h */
#include <sql.h>
#include <sqlext.h>

typedef struct {
	SWORD		ncols;
	BerVarray	col_names;
	UDWORD		*col_prec;
	SQLSMALLINT	*col_type;
	char		**cols;
	SQLLEN		*value_len;
} BACKSQL_ROW_NTS;

/*
 * Better use the standard length of 8192 (as of slap.h)?
 *
 * NOTE: must be consistent with definition in ldap_entries table
 */
/* #define BACKSQL_MAX_DN_LEN	SLAP_LDAPDN_MAXLEN */
#define BACKSQL_MAX_DN_LEN	255

/*
 * define to enable very extensive trace logging (debug only)
 */
#undef BACKSQL_TRACE

/*
 * define if using MS SQL and workaround needed (see sql-wrap.c)
 */
#undef BACKSQL_MSSQL_WORKAROUND

/*
 * define to enable values counting for attributes
 */
#define BACKSQL_COUNTQUERY

/*
 * define to enable prettification/validation of values
 */
#define BACKSQL_PRETTY_VALIDATE

/*
 * define to enable varchars as unique keys in user tables
 *
 * by default integers are used (and recommended)
 * for performances.  Integers are used anyway in back-sql
 * related tables.
 */
#undef BACKSQL_ARBITRARY_KEY

/*
 * type used for keys
 */
#if defined(HAVE_LONG_LONG) && defined(SQL_C_UBIGINT) && \
	( defined(HAVE_STRTOULL) || defined(HAVE_STRTOUQ) )
typedef unsigned long long backsql_key_t;
#define BACKSQL_C_NUMID	SQL_C_UBIGINT
#define BACKSQL_IDNUMFMT "%llu"
#define BACKSQL_STR2ID lutil_atoullx
#else /* ! HAVE_LONG_LONG || ! SQL_C_UBIGINT */
typedef unsigned long backsql_key_t;
#define BACKSQL_C_NUMID	SQL_C_ULONG
#define BACKSQL_IDNUMFMT "%lu"
#define BACKSQL_STR2ID lutil_atoulx
#endif /* ! HAVE_LONG_LONG */

/*
 * define to enable support for syncprov overlay
 */
#define BACKSQL_SYNCPROV

/*
 * define to the appropriate aliasing string
 *
 * some RDBMSes tolerate (or require) that " AS " is not used
 * when aliasing tables/columns
 */
#define BACKSQL_ALIASING	"AS "
/* #define	BACKSQL_ALIASING	"" */

/*
 * define to the appropriate quoting char
 *
 * some RDBMSes tolerate/require that the aliases be enclosed
 * in quotes.  This is especially true for those that do not
 * allow keywords used as aliases.
 */
#define BACKSQL_ALIASING_QUOTE	""
/* #define BACKSQL_ALIASING_QUOTE	"\"" */
/* #define BACKSQL_ALIASING_QUOTE	"'" */

/*
 * API
 *
 * a simple mechanism to allow DN mucking between the LDAP
 * and the stored string representation.
 */
typedef struct backsql_api {
	char			*ba_name;
	int 			(*ba_config)( struct backsql_api *self, int argc, char *argv[] );
	int			(*ba_destroy)( struct backsql_api *self );

	int 			(*ba_dn2odbc)( Operation *op, SlapReply *rs, struct berval *dn );
	int 			(*ba_odbc2dn)( Operation *op, SlapReply *rs, struct berval *dn );

	void			*ba_private;
	struct backsql_api	*ba_next;
	char **ba_argv;
	int	ba_argc;
} backsql_api;

/*
 * "structural" objectClass mapping structure
 */
typedef struct backsql_oc_map_rec {
	/*
	 * Structure of corresponding LDAP objectClass definition
	 */
	ObjectClass		*bom_oc;
#define BACKSQL_OC_NAME(ocmap)	((ocmap)->bom_oc->soc_cname.bv_val)
	
	struct berval		bom_keytbl;
	struct berval		bom_keycol;
	/* expected to return keyval of newly created entry */
	char			*bom_create_proc;
	/* in case create_proc does not return the keyval of the newly
	 * created row */
	char			*bom_create_keyval;
	/* supposed to expect keyval as parameter and delete 
	 * all the attributes as well */
	char			*bom_delete_proc;
	/* flags whether delete_proc is a function (whether back-sql 
	 * should bind first parameter as output for return code) */
	int			bom_expect_return;
	backsql_key_t		bom_id;
	Avlnode			*bom_attrs;
	AttributeDescription	*bom_create_hint;
} backsql_oc_map_rec;

/*
 * attributeType mapping structure
 */
typedef struct backsql_at_map_rec {
	/* Description of corresponding LDAP attribute type */
	AttributeDescription	*bam_ad;
	AttributeDescription	*bam_true_ad;
	/* ObjectClass if bam_ad is objectClass */
	ObjectClass		*bam_oc;

	struct berval	bam_from_tbls;
	struct berval	bam_join_where;
	struct berval	bam_sel_expr;

	/* TimesTen, or, if a uppercase function is defined,
	 * an uppercased version of bam_sel_expr */
	struct berval	bam_sel_expr_u;

	/* supposed to expect 2 binded values: entry keyval 
	 * and attr. value to add, like "add_name(?,?,?)" */
	char		*bam_add_proc;
	/* supposed to expect 2 binded values: entry keyval 
	 * and attr. value to delete */
	char		*bam_delete_proc;
	/* for optimization purposes attribute load query 
	 * is preconstructed from parts on schemamap load time */
	char		*bam_query;
#ifdef BACKSQL_COUNTQUERY
	char		*bam_countquery;
#endif /* BACKSQL_COUNTQUERY */
	/* following flags are bitmasks (first bit used for add_proc, 
	 * second - for delete_proc) */
	/* order of parameters for procedures above; 
	 * 1 means "data then keyval", 0 means "keyval then data" */
	int 		bam_param_order;
	/* flags whether one or more of procedures is a function 
	 * (whether back-sql should bind first parameter as output 
	 * for return code) */
	int 		bam_expect_return;

	/* next mapping for attribute */
	struct backsql_at_map_rec	*bam_next;
} backsql_at_map_rec;

#define BACKSQL_AT_MAP_REC_INIT { NULL, NULL, BER_BVC(""), BER_BVC(""), BER_BVNULL, BER_BVNULL, NULL, NULL, NULL, 0, 0, NULL }

/* define to uppercase filters only if the matching rule requires it
 * (currently broken) */
/* #define	BACKSQL_UPPERCASE_FILTER */

#define	BACKSQL_AT_CANUPPERCASE(at)	( !BER_BVISNULL( &(at)->bam_sel_expr_u ) )

/* defines to support bitmasks above */
#define BACKSQL_ADD	0x1
#define BACKSQL_DEL	0x2

#define BACKSQL_IS_ADD(x)	( ( BACKSQL_ADD & (x) ) == BACKSQL_ADD )
#define BACKSQL_IS_DEL(x)	( ( BACKSQL_DEL & (x) ) == BACKSQL_DEL )

#define BACKSQL_NCMP(v1,v2)	ber_bvcmp((v1),(v2))

#define BACKSQL_CONCAT
/*
 * berbuf structure: a berval with a buffer size associated
 */
typedef struct berbuf {
	struct berval	bb_val;
	ber_len_t	bb_len;
} BerBuffer;

#define BB_NULL		{ BER_BVNULL, 0 }

/*
 * Entry ID structure
 */
typedef struct backsql_entryID {
	/* #define BACKSQL_ARBITRARY_KEY to allow a non-numeric key.
	 * It is required by some special applications that use
	 * strings as keys for the main table.
	 * In this case, #define BACKSQL_MAX_KEY_LEN consistently
	 * with the key size definition */
#ifdef BACKSQL_ARBITRARY_KEY
	struct berval		eid_id;
	struct berval		eid_keyval;
#define BACKSQL_MAX_KEY_LEN	64
#else /* ! BACKSQL_ARBITRARY_KEY */
	/* The original numeric key is maintained as default. */
	backsql_key_t		eid_id;
	backsql_key_t		eid_keyval;
#endif /* ! BACKSQL_ARBITRARY_KEY */

	backsql_key_t		eid_oc_id;
	backsql_oc_map_rec	*eid_oc;
	struct berval		eid_dn;
	struct berval		eid_ndn;
	struct backsql_entryID	*eid_next;
} backsql_entryID;

#ifdef BACKSQL_ARBITRARY_KEY
#define BACKSQL_ENTRYID_INIT { BER_BVNULL, BER_BVNULL, 0, NULL, BER_BVNULL, BER_BVNULL, NULL }
#else /* ! BACKSQL_ARBITRARY_KEY */
#define BACKSQL_ENTRYID_INIT { 0, 0, 0, NULL, BER_BVNULL, BER_BVNULL, NULL }
#endif /* BACKSQL_ARBITRARY_KEY */

/* the function must collect the entry associated to nbase */
#define BACKSQL_ISF_GET_ID	0x1U
#define BACKSQL_ISF_GET_ENTRY	( 0x2U | BACKSQL_ISF_GET_ID )
#define BACKSQL_ISF_GET_OC	( 0x4U | BACKSQL_ISF_GET_ID )
#define BACKSQL_ISF_MATCHED	0x8U
#define BACKSQL_IS_GET_ID(f) \
	( ( (f) & BACKSQL_ISF_GET_ID ) == BACKSQL_ISF_GET_ID )
#define BACKSQL_IS_GET_ENTRY(f) \
	( ( (f) & BACKSQL_ISF_GET_ENTRY ) == BACKSQL_ISF_GET_ENTRY )
#define BACKSQL_IS_GET_OC(f) \
	( ( (f) & BACKSQL_ISF_GET_OC ) == BACKSQL_ISF_GET_OC )
#define BACKSQL_IS_MATCHED(f) \
	( ( (f) & BACKSQL_ISF_MATCHED ) == BACKSQL_ISF_MATCHED )
typedef struct backsql_srch_info {
	Operation		*bsi_op;
	SlapReply		*bsi_rs;

	unsigned		bsi_flags;
#define	BSQL_SF_NONE			0x0000U
#define	BSQL_SF_ALL_USER		0x0001U
#define	BSQL_SF_ALL_OPER		0x0002U
#define	BSQL_SF_ALL_ATTRS		(BSQL_SF_ALL_USER|BSQL_SF_ALL_OPER)
#define BSQL_SF_FILTER_HASSUBORDINATE	0x0010U
#define BSQL_SF_FILTER_ENTRYUUID	0x0020U
#define BSQL_SF_FILTER_ENTRYCSN		0x0040U
#define BSQL_SF_RETURN_ENTRYUUID	(BSQL_SF_FILTER_ENTRYUUID << 8)
#define	BSQL_ISF(bsi, f)		( ( (bsi)->bsi_flags & f ) == f )
#define	BSQL_ISF_ALL_USER(bsi)		BSQL_ISF(bsi, BSQL_SF_ALL_USER)
#define	BSQL_ISF_ALL_OPER(bsi)		BSQL_ISF(bsi, BSQL_SF_ALL_OPER)
#define	BSQL_ISF_ALL_ATTRS(bsi)		BSQL_ISF(bsi, BSQL_SF_ALL_ATTRS)

	struct berval		*bsi_base_ndn;
	int			bsi_use_subtree_shortcut;
	backsql_entryID		bsi_base_id;
	int			bsi_scope;
/* BACKSQL_SCOPE_BASE_LIKE can be set by API in ors_scope
 * whenever the search base DN contains chars that cannot
 * be mapped into the charset used in the RDBMS; so they're
 * turned into '%' and an approximate ('LIKE') condition
 * is used */
#define BACKSQL_SCOPE_BASE_LIKE		( LDAP_SCOPE_BASE | 0x1000 )
	Filter			*bsi_filter;
	time_t			bsi_stoptime;

	backsql_entryID		*bsi_id_list,
				**bsi_id_listtail,
				*bsi_c_eid;
	int			bsi_n_candidates;
	int			bsi_status;

	backsql_oc_map_rec	*bsi_oc;
	struct berbuf		bsi_sel,
				bsi_from,
				bsi_join_where,
				bsi_flt_where;
	ObjectClass		*bsi_filter_oc;
	SQLHDBC			bsi_dbh;
	AttributeName		*bsi_attrs;

	Entry			*bsi_e;
} backsql_srch_info;

/*
 * Backend private data structure
 */
typedef struct backsql_info {
	char		*sql_dbhost;
	int		sql_dbport;
	char		*sql_dbuser;
	char		*sql_dbpasswd;
	char		*sql_dbname;

 	/*
	 * SQL condition for subtree searches differs in syntax:
	 * "LIKE CONCAT('%',?)" or "LIKE '%'+?" or "LIKE '%'||?"
	 * or smtg else 
	 */
	struct berval	sql_subtree_cond;
	struct berval	sql_children_cond;
	struct berval	sql_dn_match_cond;
	char		*sql_oc_query;
	char		*sql_at_query;
	char		*sql_insentry_stmt;
	char		*sql_delentry_stmt;
	char		*sql_renentry_stmt;
	char		*sql_delobjclasses_stmt;
	char		*sql_id_query;
	char		*sql_has_children_query;
	char		*sql_list_children_query;

	MatchingRule	*sql_caseIgnoreMatch;
	MatchingRule	*sql_telephoneNumberMatch;

	struct berval	sql_upper_func;
	struct berval	sql_upper_func_open;
	struct berval	sql_upper_func_close;
	struct berval	sql_strcast_func;
	BerVarray	sql_concat_func;
	char		*sql_concat_patt;

	struct berval	sql_aliasing;
	struct berval	sql_aliasing_quote;
	struct berval	sql_dn_oc_aliasing;

	AttributeName	*sql_anlist;

	unsigned int	sql_flags;
#define	BSQLF_SCHEMA_LOADED		0x0001
#define	BSQLF_UPPER_NEEDS_CAST		0x0002
#define	BSQLF_CREATE_NEEDS_SELECT	0x0004
#define	BSQLF_FAIL_IF_NO_MAPPING	0x0008
#define BSQLF_HAS_LDAPINFO_DN_RU	0x0010
#define BSQLF_DONTCHECK_LDAPINFO_DN_RU	0x0020
#define BSQLF_USE_REVERSE_DN		0x0040
#define BSQLF_ALLOW_ORPHANS		0x0080
#define BSQLF_USE_SUBTREE_SHORTCUT	0x0100
#define BSQLF_FETCH_ALL_USERATTRS	0x0200
#define BSQLF_FETCH_ALL_OPATTRS		0x0400
#define	BSQLF_FETCH_ALL_ATTRS		(BSQLF_FETCH_ALL_USERATTRS|BSQLF_FETCH_ALL_OPATTRS)
#define BSQLF_CHECK_SCHEMA		0x0800
#define BSQLF_AUTOCOMMIT_ON		0x1000

#define BACKSQL_ISF(si, f) \
	(((si)->sql_flags & f) == f)

#define	BACKSQL_SCHEMA_LOADED(si) \
	BACKSQL_ISF(si, BSQLF_SCHEMA_LOADED)
#define BACKSQL_UPPER_NEEDS_CAST(si) \
	BACKSQL_ISF(si, BSQLF_UPPER_NEEDS_CAST)
#define BACKSQL_CREATE_NEEDS_SELECT(si) \
	BACKSQL_ISF(si, BSQLF_CREATE_NEEDS_SELECT)
#define BACKSQL_FAIL_IF_NO_MAPPING(si) \
	BACKSQL_ISF(si, BSQLF_FAIL_IF_NO_MAPPING)
#define BACKSQL_HAS_LDAPINFO_DN_RU(si) \
	BACKSQL_ISF(si, BSQLF_HAS_LDAPINFO_DN_RU)
#define BACKSQL_DONTCHECK_LDAPINFO_DN_RU(si) \
	BACKSQL_ISF(si, BSQLF_DONTCHECK_LDAPINFO_DN_RU)
#define BACKSQL_USE_REVERSE_DN(si) \
	BACKSQL_ISF(si, BSQLF_USE_REVERSE_DN)
#define BACKSQL_CANUPPERCASE(si) \
	(!BER_BVISNULL( &(si)->sql_upper_func ))
#define BACKSQL_ALLOW_ORPHANS(si) \
	BACKSQL_ISF(si, BSQLF_ALLOW_ORPHANS)
#define BACKSQL_USE_SUBTREE_SHORTCUT(si) \
	BACKSQL_ISF(si, BSQLF_USE_SUBTREE_SHORTCUT)
#define BACKSQL_FETCH_ALL_USERATTRS(si) \
	BACKSQL_ISF(si, BSQLF_FETCH_ALL_USERATTRS)
#define BACKSQL_FETCH_ALL_OPATTRS(si) \
	BACKSQL_ISF(si, BSQLF_FETCH_ALL_OPATTRS)
#define BACKSQL_FETCH_ALL_ATTRS(si) \
	BACKSQL_ISF(si, BSQLF_FETCH_ALL_ATTRS)
#define BACKSQL_CHECK_SCHEMA(si) \
	BACKSQL_ISF(si, BSQLF_CHECK_SCHEMA)
#define BACKSQL_AUTOCOMMIT_ON(si) \
	BACKSQL_ISF(si, BSQLF_AUTOCOMMIT_ON)

	Entry		*sql_baseObject;
	char		*sql_base_ob_file;
#ifdef BACKSQL_ARBITRARY_KEY
#define BACKSQL_BASEOBJECT_IDSTR	"baseObject"
#define BACKSQL_BASEOBJECT_KEYVAL	BACKSQL_BASEOBJECT_IDSTR
#define	BACKSQL_IS_BASEOBJECT_ID(id)	(bvmatch((id), &backsql_baseObject_bv))
#else /* ! BACKSQL_ARBITRARY_KEY */
#define BACKSQL_BASEOBJECT_ID		0
#define BACKSQL_BASEOBJECT_IDSTR	LDAP_XSTRING(BACKSQL_BASEOBJECT_ID)
#define BACKSQL_BASEOBJECT_KEYVAL	0
#define	BACKSQL_IS_BASEOBJECT_ID(id)	(*(id) == BACKSQL_BASEOBJECT_ID)
#endif /* ! BACKSQL_ARBITRARY_KEY */
#define BACKSQL_BASEOBJECT_OC		0
	
	Avlnode		*sql_db_conns;
	SQLHDBC		sql_dbh;
	ldap_pvt_thread_mutex_t		sql_dbconn_mutex;
	Avlnode		*sql_oc_by_oc;
	Avlnode		*sql_oc_by_id;
	ldap_pvt_thread_mutex_t		sql_schema_mutex;
 	SQLHENV		sql_db_env;

	backsql_api	*sql_api;
} backsql_info;

#define BACKSQL_SUCCESS( rc ) \
	( (rc) == SQL_SUCCESS || (rc) == SQL_SUCCESS_WITH_INFO )

#define BACKSQL_AVL_STOP		0
#define BACKSQL_AVL_CONTINUE		1

/* see ldap.h for the meaning of the macros and of the values */
#define BACKSQL_LEGAL_ERROR( rc ) \
	( LDAP_RANGE( (rc), 0x00, 0x0e ) \
	  || LDAP_ATTR_ERROR( (rc) ) \
	  || LDAP_NAME_ERROR( (rc) ) \
	  || LDAP_SECURITY_ERROR( (rc) ) \
	  || LDAP_SERVICE_ERROR( (rc) ) \
	  || LDAP_UPDATE_ERROR( (rc) ) )
#define BACKSQL_SANITIZE_ERROR( rc ) \
	( BACKSQL_LEGAL_ERROR( (rc) ) ? (rc) : LDAP_OTHER )

#define BACKSQL_IS_BINARY(ct) \
	( (ct) == SQL_BINARY \
	  || (ct) == SQL_VARBINARY \
	  || (ct) == SQL_LONGVARBINARY)

#ifdef BACKSQL_ARBITRARY_KEY
#define BACKSQL_IDFMT "%s"
#define BACKSQL_IDARG(arg) ((arg).bv_val)
#else /* ! BACKSQL_ARBITRARY_KEY */
#define BACKSQL_IDFMT BACKSQL_IDNUMFMT
#define BACKSQL_IDARG(arg) (arg)
#endif /* ! BACKSQL_ARBITRARY_KEY */

#endif /* __BACKSQL_H__ */

