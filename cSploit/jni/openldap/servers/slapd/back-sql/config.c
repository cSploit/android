/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
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
 * Pierangelo Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include "ac/string.h"
#include <sys/types.h>

#include "slap.h"
#include "config.h"
#include "ldif.h"
#include "lutil.h"
#include "proto-sql.h"

static int
create_baseObject(
	BackendDB	*be,
	const char	*fname,
	int		lineno );

static int
read_baseObject(
	BackendDB	*be,
	const char	*fname );

static ConfigDriver sql_cf_gen;

enum {
	BSQL_CONCAT_PATT = 1,
	BSQL_CREATE_NEEDS_SEL,
	BSQL_UPPER_NEEDS_CAST,
	BSQL_HAS_LDAPINFO_DN_RU,
	BSQL_FAIL_IF_NO_MAPPING,
	BSQL_ALLOW_ORPHANS,
	BSQL_BASE_OBJECT,
	BSQL_LAYER,
	BSQL_SUBTREE_SHORTCUT,
	BSQL_FETCH_ALL_ATTRS,
	BSQL_FETCH_ATTRS,
	BSQL_CHECK_SCHEMA,
	BSQL_ALIASING_KEYWORD,
	BSQL_AUTOCOMMIT
};

static ConfigTable sqlcfg[] = {
	{ "dbhost", "hostname", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_dbhost),
		"( OLcfgDbAt:6.1 NAME 'olcDbHost' "
			"DESC 'Hostname of SQL server' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbname", "name", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_dbname),
		"( OLcfgDbAt:6.2 NAME 'olcDbName' "
			"DESC 'Name of SQL database' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbuser", "username", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_dbuser),
		"( OLcfgDbAt:6.3 NAME 'olcDbUser' "
			"DESC 'Username for SQL session' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbpasswd", "password", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_dbpasswd),
		"( OLcfgDbAt:6.4 NAME 'olcDbPass' "
			"DESC 'Password for SQL session' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "concat_pattern", "pattern", 2, 2, 0,
		ARG_STRING|ARG_MAGIC|BSQL_CONCAT_PATT, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.20 NAME 'olcSqlConcatPattern' "
			"DESC 'Pattern used to concatenate strings' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "subtree_cond", "SQL expression", 2, 0, 0, ARG_BERVAL|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_subtree_cond),
		"( OLcfgDbAt:6.21 NAME 'olcSqlSubtreeCond' "
			"DESC 'Where-clause template for a subtree search condition' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "children_cond", "SQL expression", 2, 0, 0, ARG_BERVAL|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_children_cond),
		"( OLcfgDbAt:6.22 NAME 'olcSqlChildrenCond' "
			"DESC 'Where-clause template for a children search condition' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dn_match_cond", "SQL expression", 2, 0, 0, ARG_BERVAL|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_dn_match_cond),
		"( OLcfgDbAt:6.23 NAME 'olcSqlDnMatchCond' "
			"DESC 'Where-clause template for a DN match search condition' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "oc_query", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_oc_query),
		"( OLcfgDbAt:6.24 NAME 'olcSqlOcQuery' "
			"DESC 'Query used to collect objectClass mapping data' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "at_query", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_at_query),
		"( OLcfgDbAt:6.25 NAME 'olcSqlAtQuery' "
			"DESC 'Query used to collect attributeType mapping data' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "insentry_stmt", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_insentry_stmt),
		"( OLcfgDbAt:6.26 NAME 'olcSqlInsEntryStmt' "
			"DESC 'Statement used to insert a new entry' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "create_needs_select", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_CREATE_NEEDS_SEL, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.27 NAME 'olcSqlCreateNeedsSelect' "
			"DESC 'Whether entry creation needs a subsequent select' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "upper_func", "SQL function name", 2, 2, 0, ARG_BERVAL|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_upper_func),
		"( OLcfgDbAt:6.28 NAME 'olcSqlUpperFunc' "
			"DESC 'Function that converts a value to uppercase' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "upper_needs_cast", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_UPPER_NEEDS_CAST, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.29 NAME 'olcSqlUpperNeedsCast' "
			"DESC 'Whether olcSqlUpperFunc needs an explicit cast' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "strcast_func", "SQL function name", 2, 2, 0, ARG_BERVAL|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_strcast_func),
		"( OLcfgDbAt:6.30 NAME 'olcSqlStrcastFunc' "
			"DESC 'Function that converts a value to a string' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "delentry_stmt", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_delentry_stmt),
		"( OLcfgDbAt:6.31 NAME 'olcSqlDelEntryStmt' "
			"DESC 'Statement used to delete an existing entry' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "renentry_stmt", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_renentry_stmt),
		"( OLcfgDbAt:6.32 NAME 'olcSqlRenEntryStmt' "
			"DESC 'Statement used to rename an entry' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "delobjclasses_stmt", "SQL expression", 2, 0, 0, ARG_STRING|ARG_QUOTE|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_delobjclasses_stmt),
		"( OLcfgDbAt:6.33 NAME 'olcSqlDelObjclassesStmt' "
			"DESC 'Statement used to delete the ID of an entry' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "has_ldapinfo_dn_ru", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_HAS_LDAPINFO_DN_RU, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.34 NAME 'olcSqlHasLDAPinfoDnRu' "
			"DESC 'Whether the dn_ru column is present' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "fail_if_no_mapping", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_FAIL_IF_NO_MAPPING, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.35 NAME 'olcSqlFailIfNoMapping' "
			"DESC 'Whether to fail on unknown attribute mappings' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "allow_orphans", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_ALLOW_ORPHANS, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.36 NAME 'olcSqlAllowOrphans' "
			"DESC 'Whether to allow adding entries with no parent' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "baseobject", "[file]", 1, 2, 0,
		ARG_STRING|ARG_MAGIC|BSQL_BASE_OBJECT, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.37 NAME 'olcSqlBaseObject' "
			"DESC 'Manage an in-memory baseObject entry' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "sqllayer", "name", 2, 0, 0,
		ARG_MAGIC|BSQL_LAYER, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.38 NAME 'olcSqlLayer' "
			"DESC 'Helper used to map DNs between LDAP and SQL' "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "use_subtree_shortcut", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_SUBTREE_SHORTCUT, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.39 NAME 'olcSqlUseSubtreeShortcut' "
			"DESC 'Collect all entries when searchBase is DB suffix' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "fetch_all_attrs", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_FETCH_ALL_ATTRS, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.40 NAME 'olcSqlFetchAllAttrs' "
			"DESC 'Require all attributes to always be loaded' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "fetch_attrs", "attrlist", 2, 0, 0,
		ARG_MAGIC|BSQL_FETCH_ATTRS, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.41 NAME 'olcSqlFetchAttrs' "
			"DESC 'Set of attributes to always fetch' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "check_schema", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|BSQL_CHECK_SCHEMA, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.42 NAME 'olcSqlCheckSchema' "
			"DESC 'Check schema after modifications' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ "aliasing_keyword", "string", 2, 2, 0,
		ARG_STRING|ARG_MAGIC|BSQL_ALIASING_KEYWORD, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.43 NAME 'olcSqlAliasingKeyword' "
			"DESC 'The aliasing keyword' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "aliasing_quote", "string", 2, 2, 0, ARG_BERVAL|ARG_OFFSET,
		(void *)offsetof(struct backsql_info, sql_aliasing_quote),
		"( OLcfgDbAt:6.44 NAME 'olcSqlAliasingQuote' "
			"DESC 'Quoting char of the aliasing keyword' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "autocommit", "yes|no", 2, 2, 0,
		ARG_ON_OFF|ARG_MAGIC|SQL_AUTOCOMMIT, (void *)sql_cf_gen,
		"( OLcfgDbAt:6.45 NAME 'olcSqlAutocommit' "
			"SYNTAX OMsBoolean SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs sqlocs[] = {
	{
		"( OLcfgDbOc:6.1 "
		"NAME 'olcSqlConfig' "
		"DESC 'SQL backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST olcDbName "
		"MAY ( olcDbHost $ olcDbUser $ olcDbPass $ olcSqlConcatPattern $ "
		"olcSqlSubtreeCond $ olcsqlChildrenCond $ olcSqlDnMatchCond $ "
		"olcSqlOcQuery $ olcSqlAtQuery $ olcSqlInsEntryStmt $ "
		"olcSqlCreateNeedsSelect $ olcSqlUpperFunc $ olcSqlUpperNeedsCast $ "
		"olcSqlStrCastFunc $ olcSqlDelEntryStmt $ olcSqlRenEntryStmt $ "
		"olcSqlDelObjClassesStmt $ olcSqlHasLDAPInfoDnRu $ "
		"olcSqlFailIfNoMapping $ olcSqlAllowOrphans $ olcSqlBaseObject $ "
		"olcSqlLayer $ olcSqlUseSubtreeShortcut $ olcSqlFetchAllAttrs $ "
		"olcSqlFetchAttrs $ olcSqlCheckSchema $ olcSqlAliasingKeyword $ "
		"olcSqlAliasingQuote $ olcSqlAutocommit ) )",
			Cft_Database, sqlcfg },
	{ NULL, Cft_Abstract, NULL }
};

static int
sql_cf_gen( ConfigArgs *c )
{
	backsql_info 	*bi = (backsql_info *)c->be->be_private;
	int rc = 0;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case BSQL_CONCAT_PATT:
			if ( bi->sql_concat_patt ) {
				c->value_string = ch_strdup( bi->sql_concat_patt );
			} else {
				rc = 1;
			}
			break;
		case BSQL_CREATE_NEEDS_SEL:
			if ( bi->sql_flags & BSQLF_CREATE_NEEDS_SELECT )
				c->value_int = 1;
			break;
		case BSQL_UPPER_NEEDS_CAST:
			if ( bi->sql_flags & BSQLF_UPPER_NEEDS_CAST )
				c->value_int = 1;
			break;
		case BSQL_HAS_LDAPINFO_DN_RU:
			if ( !(bi->sql_flags & BSQLF_DONTCHECK_LDAPINFO_DN_RU) )
				return 1;
			if ( bi->sql_flags & BSQLF_HAS_LDAPINFO_DN_RU )
				c->value_int = 1;
			break;
		case BSQL_FAIL_IF_NO_MAPPING:
			if ( bi->sql_flags & BSQLF_FAIL_IF_NO_MAPPING )
				c->value_int = 1;
			break;
		case BSQL_ALLOW_ORPHANS:
			if ( bi->sql_flags & BSQLF_ALLOW_ORPHANS )
				c->value_int = 1;
			break;
		case BSQL_SUBTREE_SHORTCUT:
			if ( bi->sql_flags & BSQLF_USE_SUBTREE_SHORTCUT )
				c->value_int = 1;
			break;
		case BSQL_FETCH_ALL_ATTRS:
			if ( bi->sql_flags & BSQLF_FETCH_ALL_ATTRS )
				c->value_int = 1;
			break;
		case BSQL_CHECK_SCHEMA:
			if ( bi->sql_flags & BSQLF_CHECK_SCHEMA )
				c->value_int = 1;
			break;
		case BSQL_AUTOCOMMIT:
			if ( bi->sql_flags & BSQLF_AUTOCOMMIT_ON )
				c->value_int = 1;
			break;
		case BSQL_BASE_OBJECT:
			if ( bi->sql_base_ob_file ) {
				c->value_string = ch_strdup( bi->sql_base_ob_file );
			} else if ( bi->sql_baseObject ) {
				c->value_string = ch_strdup( "TRUE" );
			} else {
				rc = 1;
			}
			break;
		case BSQL_LAYER:
			if ( bi->sql_api ) {
				backsql_api *ba;
				struct berval bv;
				char *ptr;
				int i;
				for ( ba = bi->sql_api; ba; ba = ba->ba_next ) {
					bv.bv_len = strlen( ba->ba_name );
					if ( ba->ba_argc ) {
						for ( i = 0; i<ba->ba_argc; i++ )
							bv.bv_len += strlen( ba->ba_argv[i] ) + 3;
					}
					bv.bv_val = ch_malloc( bv.bv_len + 1 );
					ptr = lutil_strcopy( bv.bv_val, ba->ba_name );
					if ( ba->ba_argc ) {
						for ( i = 0; i<ba->ba_argc; i++ ) {
							*ptr++ = ' ';
							*ptr++ = '"';
							ptr = lutil_strcopy( ptr, ba->ba_argv[i] );
							*ptr++ = '"';
						}
					}
					ber_bvarray_add( &c->rvalue_vals, &bv );
				}
			} else {
				rc = 1;
			}
			break;
		case BSQL_ALIASING_KEYWORD:
			if ( !BER_BVISNULL( &bi->sql_aliasing )) {
				struct berval bv;
				bv = bi->sql_aliasing;
				bv.bv_len--;
				value_add_one( &c->rvalue_vals, &bv );
			} else {
				rc = 1;
			}
			break;
		case BSQL_FETCH_ATTRS:
			if ( bi->sql_anlist ||
				( bi->sql_flags & (BSQLF_FETCH_ALL_USERATTRS|
								   BSQLF_FETCH_ALL_OPATTRS)))
			{
				char buf[BUFSIZ*2], *ptr;
				struct berval bv;
#   define WHATSLEFT    ((ber_len_t) (&buf[sizeof( buf )] - ptr))
				ptr = buf;
				if ( bi->sql_anlist ) {
					ptr = anlist_unparse( bi->sql_anlist, ptr, WHATSLEFT );
					if ( ptr == NULL )
						return 1;
				}
				if ( bi->sql_flags & BSQLF_FETCH_ALL_USERATTRS ) {
					if ( WHATSLEFT <= STRLENOF( ",*" )) return 1;
					if ( ptr != buf ) *ptr++ = ',';
					*ptr++ = '*';
				}
				if ( bi->sql_flags & BSQLF_FETCH_ALL_OPATTRS ) {
					if ( WHATSLEFT <= STRLENOF( ",+" )) return 1;
					if ( ptr != buf ) *ptr++ = ',';
					*ptr++ = '+';
				}
				bv.bv_val = buf;
				bv.bv_len = ptr - buf;
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;
		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) {	/* FIXME */
		return -1;
	}

	switch( c->type ) {
	case BSQL_CONCAT_PATT:
		if ( backsql_split_pattern( c->argv[ 1 ], &bi->sql_concat_func, 2 ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s: unable to parse pattern \"%s\"",
				c->log, c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		bi->sql_concat_patt = c->value_string;
		break;
	case BSQL_CREATE_NEEDS_SEL:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_CREATE_NEEDS_SELECT;
		else
			bi->sql_flags &= ~BSQLF_CREATE_NEEDS_SELECT;
		break;
	case BSQL_UPPER_NEEDS_CAST:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_UPPER_NEEDS_CAST;
		else
			bi->sql_flags &= ~BSQLF_UPPER_NEEDS_CAST;
		break;
	case BSQL_HAS_LDAPINFO_DN_RU:
		bi->sql_flags |= BSQLF_DONTCHECK_LDAPINFO_DN_RU;
		if ( c->value_int )
			bi->sql_flags |= BSQLF_HAS_LDAPINFO_DN_RU;
		else
			bi->sql_flags &= ~BSQLF_HAS_LDAPINFO_DN_RU;
		break;
	case BSQL_FAIL_IF_NO_MAPPING:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_FAIL_IF_NO_MAPPING;
		else
			bi->sql_flags &= ~BSQLF_FAIL_IF_NO_MAPPING;
		break;
	case BSQL_ALLOW_ORPHANS:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_ALLOW_ORPHANS;
		else
			bi->sql_flags &= ~BSQLF_ALLOW_ORPHANS;
		break;
	case BSQL_SUBTREE_SHORTCUT:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_USE_SUBTREE_SHORTCUT;
		else
			bi->sql_flags &= ~BSQLF_USE_SUBTREE_SHORTCUT;
		break;
	case BSQL_FETCH_ALL_ATTRS:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_FETCH_ALL_ATTRS;
		else
			bi->sql_flags &= ~BSQLF_FETCH_ALL_ATTRS;
		break;
	case BSQL_CHECK_SCHEMA:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_CHECK_SCHEMA;
		else
			bi->sql_flags &= ~BSQLF_CHECK_SCHEMA;
		break;
	case BSQL_AUTOCOMMIT:
		if ( c->value_int )
			bi->sql_flags |= BSQLF_AUTOCOMMIT_ON;
		else
			bi->sql_flags &= ~BSQLF_AUTOCOMMIT_ON;
		break;
	case BSQL_BASE_OBJECT:
		if ( c->be->be_nsuffix == NULL ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s: suffix must be set", c->log );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			rc = ARG_BAD_CONF;
			break;
		}
		if ( bi->sql_baseObject ) {
			Debug( LDAP_DEBUG_CONFIG,
				"%s: "
				"\"baseObject\" already provided (will be overwritten)\n",
				c->log, 0, 0 );
			entry_free( bi->sql_baseObject );
		}
		if ( c->argc == 2 && !strcmp( c->argv[1], "TRUE" ))
			c->argc = 1;
		switch( c->argc ) {
		case 1:
			return create_baseObject( c->be, c->fname, c->lineno );

		case 2:
			rc = read_baseObject( c->be, c->argv[ 1 ] );
			if ( rc == 0 ) {
				ch_free( bi->sql_base_ob_file );
				bi->sql_base_ob_file = c->value_string;
			}
			return rc;

		default:
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s: trailing values in directive", c->log );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return 1;
		}
		break;
	case BSQL_LAYER:
		if ( backsql_api_config( bi, c->argv[ 1 ], c->argc - 2, &c->argv[ 2 ] ) )
		{
			snprintf( c->cr_msg, sizeof( c->cr_msg ),
				"%s: unable to load sql layer", c->log );
			Debug( LDAP_DEBUG_ANY, "%s \"%s\"\n",
				c->cr_msg, c->argv[1], 0 );
			return 1;
		}
		break;
	case BSQL_ALIASING_KEYWORD:
		if ( ! BER_BVISNULL( &bi->sql_aliasing ) ) {
			ch_free( bi->sql_aliasing.bv_val );
		}

		ber_str2bv( c->argv[ 1 ], strlen( c->argv[ 1 ] ) + 1, 1,
			&bi->sql_aliasing );
		/* add a trailing space... */
		bi->sql_aliasing.bv_val[ bi->sql_aliasing.bv_len - 1] = ' ';
		break;
	case BSQL_FETCH_ATTRS: {
		char		*str, *s, *next;
		const char	*delimstr = ",";

		str = ch_strdup( c->argv[ 1 ] );
		for ( s = ldap_pvt_strtok( str, delimstr, &next );
				s != NULL;
				s = ldap_pvt_strtok( NULL, delimstr, &next ) )
		{
			if ( strlen( s ) == 1 ) {
				if ( *s == '*' ) {
					bi->sql_flags |= BSQLF_FETCH_ALL_USERATTRS;
					c->argv[ 1 ][ s - str ] = ',';

				} else if ( *s == '+' ) {
					bi->sql_flags |= BSQLF_FETCH_ALL_OPATTRS;
					c->argv[ 1 ][ s - str ] = ',';
				}
			}
		}
		ch_free( str );
		bi->sql_anlist = str2anlist( bi->sql_anlist, c->argv[ 1 ], delimstr );
		if ( bi->sql_anlist == NULL ) {
			return -1;
		}
		}
		break;
	}
	return rc;
}

/*
 * Read the entries specified in fname and merge the attributes
 * to the user defined baseObject entry. Note that if we find any errors
 * what so ever, we will discard the entire entries, print an
 * error message and return.
 */
static int
read_baseObject( 
	BackendDB	*be,
	const char	*fname )
{
	backsql_info 	*bi = (backsql_info *)be->be_private;
	LDIFFP		*fp;
	int		rc = 0, lmax = 0, ldifrc;
	unsigned long	lineno = 0;
	char		*buf = NULL;

	assert( fname != NULL );

	fp = ldif_open( fname, "r" );
	if ( fp == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"could not open back-sql baseObject "
			"attr file \"%s\" - absolute path?\n",
			fname, 0, 0 );
		perror( fname );
		return LDAP_OTHER;
	}

	bi->sql_baseObject = entry_alloc();
	if ( bi->sql_baseObject == NULL ) {
		Debug( LDAP_DEBUG_ANY,
			"read_baseObject_file: entry_alloc failed", 0, 0, 0 );
		ldif_close( fp );
		return LDAP_NO_MEMORY;
	}
	bi->sql_baseObject->e_name = be->be_suffix[0];
	bi->sql_baseObject->e_nname = be->be_nsuffix[0];
	bi->sql_baseObject->e_attrs = NULL;

	while (( ldifrc = ldif_read_record( fp, &lineno, &buf, &lmax )) > 0 ) {
		Entry		*e = str2entry( buf );
		Attribute	*a;

		if( e == NULL ) {
			fprintf( stderr, "back-sql baseObject: "
					"could not parse entry (line=%lu)\n",
					lineno );
			rc = LDAP_OTHER;
			break;
		}

		/* make sure the DN is the database's suffix */
		if ( !be_issuffix( be, &e->e_nname ) ) {
			fprintf( stderr,
				"back-sql: invalid baseObject - "
				"dn=\"%s\" (line=%lu)\n",
				e->e_name.bv_val, lineno );
			entry_free( e );
			rc = LDAP_OTHER;
			break;
		}

		/*
		 * we found a valid entry, so walk thru all the attributes in the
		 * entry, and add each attribute type and description to baseObject
		 */
		for ( a = e->e_attrs; a != NULL; a = a->a_next ) {
			if ( attr_merge( bi->sql_baseObject, a->a_desc,
						a->a_vals,
						( a->a_nvals == a->a_vals ) ?
						NULL : a->a_nvals ) )
			{
				rc = LDAP_OTHER;
				break;
			}
		}

		entry_free( e );
		if ( rc ) {
			break;
		}
	}

	if ( ldifrc < 0 )
		rc = LDAP_OTHER;

	if ( rc ) {
		entry_free( bi->sql_baseObject );
		bi->sql_baseObject = NULL;
	}

	ch_free( buf );

	ldif_close( fp );

	Debug( LDAP_DEBUG_CONFIG, "back-sql baseObject file \"%s\" read.\n",
			fname, 0, 0 );

	return rc;
}

static int
create_baseObject(
	BackendDB	*be,
	const char	*fname,
	int		lineno )
{
	backsql_info 	*bi = (backsql_info *)be->be_private;
	LDAPRDN		rdn;
	char		*p;
	int		rc, iAVA;
	char		buf[1024];

	snprintf( buf, sizeof(buf),
			"dn: %s\n"
			"objectClass: extensibleObject\n"
			"description: builtin baseObject for back-sql\n"
			"description: all entries mapped "
				"in table \"ldap_entries\" "
				"must have "
				"\"" BACKSQL_BASEOBJECT_IDSTR "\" "
				"in the \"parent\" column",
			be->be_suffix[0].bv_val );

	bi->sql_baseObject = str2entry( buf );
	if ( bi->sql_baseObject == NULL ) {
		Debug( LDAP_DEBUG_TRACE,
			"<==backsql_db_config (%s line %d): "
			"unable to parse baseObject entry\n",
			fname, lineno, 0 );
		return 1;
	}

	if ( BER_BVISEMPTY( &be->be_suffix[ 0 ] ) ) {
		return 0;
	}

	rc = ldap_bv2rdn( &be->be_suffix[ 0 ], &rdn, (char **)&p,
			LDAP_DN_FORMAT_LDAP );
	if ( rc != LDAP_SUCCESS ) {
		snprintf( buf, sizeof(buf),
			"unable to extract RDN "
			"from baseObject DN \"%s\" (%d: %s)",
			be->be_suffix[ 0 ].bv_val,
			rc, ldap_err2string( rc ) );
		Debug( LDAP_DEBUG_TRACE,
			"<==backsql_db_config (%s line %d): %s\n",
			fname, lineno, buf );
		return 1;
	}

	for ( iAVA = 0; rdn[ iAVA ]; iAVA++ ) {
		LDAPAVA				*ava = rdn[ iAVA ];
		AttributeDescription		*ad = NULL;
		slap_syntax_transform_func	*transf = NULL;
		struct berval			bv = BER_BVNULL;
		const char			*text = NULL;

		assert( ava != NULL );

		rc = slap_bv2ad( &ava->la_attr, &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( buf, sizeof(buf),
				"AttributeDescription of naming "
				"attribute #%d from baseObject "
				"DN \"%s\": %d: %s",
				iAVA, be->be_suffix[ 0 ].bv_val,
				rc, ldap_err2string( rc ) );
			Debug( LDAP_DEBUG_TRACE,
				"<==backsql_db_config (%s line %d): %s\n",
				fname, lineno, buf );
			return 1;
		}
		
		transf = ad->ad_type->sat_syntax->ssyn_pretty;
		if ( transf ) {
			/*
	 		 * transform value by pretty function
			 *	if value is empty, use empty_bv
			 */
			rc = ( *transf )( ad->ad_type->sat_syntax,
				ava->la_value.bv_len
					? &ava->la_value
					: (struct berval *) &slap_empty_bv,
				&bv, NULL );
	
			if ( rc != LDAP_SUCCESS ) {
				snprintf( buf, sizeof(buf),
					"prettying of attribute #%d "
					"from baseObject "
					"DN \"%s\" failed: %d: %s",
					iAVA, be->be_suffix[ 0 ].bv_val,
					rc, ldap_err2string( rc ) );
				Debug( LDAP_DEBUG_TRACE,
					"<==backsql_db_config (%s line %d): "
					"%s\n",
					fname, lineno, buf );
				return 1;
			}
		}

		if ( !BER_BVISNULL( &bv ) ) {
			if ( ava->la_flags & LDAP_AVA_FREE_VALUE ) {
				ber_memfree( ava->la_value.bv_val );
			}
			ava->la_value = bv;
			ava->la_flags |= LDAP_AVA_FREE_VALUE;
		}

		attr_merge_normalize_one( bi->sql_baseObject,
				ad, &ava->la_value, NULL );
	}

	ldap_rdnfree( rdn );

	return 0;
}

int backsql_init_cf( BackendInfo *bi )
{
	int rc;

	bi->bi_cf_ocs = sqlocs;
	rc = config_register_schema( sqlcfg, sqlocs );
	if ( rc ) return rc;
	return 0;
}
