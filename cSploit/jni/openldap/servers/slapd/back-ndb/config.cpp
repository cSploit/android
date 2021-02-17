/* config.cpp - ndb backend configuration file routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software. This work was sponsored by MySQL.
 */

#include "portable.h"
#include "lutil.h"

#include "back-ndb.h"

#include "config.h"

extern "C" {
	static ConfigDriver ndb_cf_gen;
};

enum {
	NDB_ATLEN = 1,
	NDB_ATSET,
	NDB_INDEX,
	NDB_ATBLOB
};

static ConfigTable ndbcfg[] = {
	{ "dbhost", "hostname", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_hostname),
		"( OLcfgDbAt:6.1 NAME 'olcDbHost' "
			"DESC 'Hostname of SQL server' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbname", "name", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_dbname),
		"( OLcfgDbAt:6.2 NAME 'olcDbName' "
			"DESC 'Name of SQL database' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbuser", "username", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_username),
		"( OLcfgDbAt:6.3 NAME 'olcDbUser' "
			"DESC 'Username for SQL session' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbpass", "password", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_password),
		"( OLcfgDbAt:6.4 NAME 'olcDbPass' "
			"DESC 'Password for SQL session' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbport", "port", 2, 2, 0, ARG_UINT|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_port),
		"( OLcfgDbAt:6.5 NAME 'olcDbPort' "
			"DESC 'Port number of SQL server' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "dbsocket", "path", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_socket),
		"( OLcfgDbAt:6.6 NAME 'olcDbSocket' "
			"DESC 'Local socket path of SQL server' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbflag", "flag", 2, 2, 0, ARG_LONG|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_clflag),
		"( OLcfgDbAt:6.7 NAME 'olcDbFlag' "
			"DESC 'Flags for SQL session' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "dbconnect", "hostname", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_connectstr),
		"( OLcfgDbAt:6.8 NAME 'olcDbConnect' "
			"DESC 'Hostname of NDB server' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "dbconnections", "number", 2, 2, 0, ARG_INT|ARG_OFFSET,
		(void *)offsetof(struct ndb_info, ni_nconns),
		"( OLcfgDbAt:6.9 NAME 'olcDbConnections' "
			"DESC 'Number of cluster connections to open' "
			"SYNTAX OMsInteger SINGLE-VALUE )", NULL, NULL },
	{ "attrlen", "attr> <len", 3, 3, 0, ARG_MAGIC|NDB_ATLEN,
		(void *)ndb_cf_gen,
		"( OLcfgDbAt:6.10 NAME 'olcNdbAttrLen' "
			"DESC 'Column length of a specific attribute' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "attrset", "set> <attrs", 3, 3, 0, ARG_MAGIC|NDB_ATSET,
		(void *)ndb_cf_gen,
		"( OLcfgDbAt:6.11 NAME 'olcNdbAttrSet' "
			"DESC 'Set of common attributes' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "index", "attr", 2, 2, 0, ARG_MAGIC|NDB_INDEX,
		(void *)ndb_cf_gen, "( OLcfgDbAt:0.2 NAME 'olcDbIndex' "
		"DESC 'Attribute to index' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "attrblob", "attr", 2, 2, 0, ARG_MAGIC|NDB_ATBLOB,
		(void *)ndb_cf_gen, "( OLcfgDbAt:6.12 NAME 'olcNdbAttrBlob' "
		"DESC 'Attribute to treat as a BLOB' "
		"EQUALITY caseIgnoreMatch "
		"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "directory", "dir", 2, 2, 0, ARG_IGNORED,
		NULL, "( OLcfgDbAt:0.1 NAME 'olcDbDirectory' "
			"DESC 'Dummy keyword' "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs ndbocs[] = {
	{
		"( OLcfgDbOc:6.2 "
		"NAME 'olcNdbConfig' "
		"DESC 'NDB backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST ( olcDbHost $ olcDbName $ olcDbConnect ) "
		"MAY ( olcDbUser $ olcDbPass $ olcDbPort $ olcDbSocket $ "
		"olcDbFlag $ olcDbConnections $ olcNdbAttrLen $ "
		"olcDbIndex $ olcNdbAttrSet $ olcNdbAttrBlob ) )",
			Cft_Database, ndbcfg },
	{ NULL, Cft_Abstract, NULL }
};

static int
ndb_cf_gen( ConfigArgs *c )
{
	struct ndb_info *ni = (struct ndb_info *)c->be->be_private;
	int i, rc;
	NdbAttrInfo *ai;
	NdbOcInfo *oci;
	ListNode *ln, **l2;
	struct berval bv, *bva;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		char buf[BUFSIZ];
		rc = 0;
		bv.bv_val = buf;
		switch( c->type ) {
		case NDB_ATLEN:
			if ( ni->ni_attrlens ) {
				for ( ln = ni->ni_attrlens; ln; ln=ln->ln_next ) {
					ai = (NdbAttrInfo *)ln->ln_data;
					bv.bv_len = snprintf( buf, sizeof(buf),
						"%s %d", ai->na_name.bv_val,
							ai->na_len );
					value_add_one( &c->rvalue_vals, &bv );
				}
			} else {
				rc = 1;
			}
			break;

		case NDB_ATSET:
			if ( ni->ni_attrsets ) {
				char *ptr, *end = buf+sizeof(buf);
				for ( ln = ni->ni_attrsets; ln; ln=ln->ln_next ) {
					oci = (NdbOcInfo *)ln->ln_data;
					ptr = lutil_strcopy( buf, oci->no_name.bv_val );
					*ptr++ = ' ';
					for ( i=0; i<oci->no_nattrs; i++ ) {
						if ( end - ptr < oci->no_attrs[i]->na_name.bv_len+1 )
							break;
						if ( i )
							*ptr++ = ',';
						ptr = lutil_strcopy(ptr,
							oci->no_attrs[i]->na_name.bv_val );
					}
					bv.bv_len = ptr - buf;
					value_add_one( &c->rvalue_vals, &bv );
				}
			} else {
				rc = 1;
			}
			break;

		case NDB_INDEX:
			if ( ni->ni_attridxs ) {
				for ( ln = ni->ni_attridxs; ln; ln=ln->ln_next ) {
					ai = (NdbAttrInfo *)ln->ln_data;
					value_add_one( &c->rvalue_vals, &ai->na_name );
				}
			} else {
				rc = 1;
			}
			break;

		case NDB_ATBLOB:
			if ( ni->ni_attrblobs ) {
				for ( ln = ni->ni_attrblobs; ln; ln=ln->ln_next ) {
					ai = (NdbAttrInfo *)ln->ln_data;
					value_add_one( &c->rvalue_vals, &ai->na_name );
				}
			} else {
				rc = 1;
			}
			break;

		}
		return rc;
	} else if ( c->op == LDAP_MOD_DELETE ) { /* FIXME */
		rc = 0;
		switch( c->type ) {
		case NDB_INDEX:
			if ( c->valx == -1 ) {

				/* delete all */

			} else {

			}
			break;
		}
		return rc;
	}

	switch( c->type ) {
	case NDB_ATLEN:
		ber_str2bv( c->argv[1], 0, 0, &bv );
		ai = ndb_ai_get( ni, &bv );
		if ( !ai ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: invalid attr %s",
				c->log, c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		for ( ln = ni->ni_attrlens; ln; ln = ln->ln_next ) {
			if ( ln->ln_data == (void *)ai ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: attr len already set for %s",
					c->log, c->argv[1] );
				Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
				return -1;
			}
		}
		ai->na_len = atoi( c->argv[2] );
		ai->na_flag |= NDB_INFO_ATLEN;
		ln = (ListNode *)ch_malloc( sizeof(ListNode));
		ln->ln_data = ai;
		ln->ln_next = NULL;
		for ( l2 = &ni->ni_attrlens; *l2; l2 = &(*l2)->ln_next );
		*l2 = ln;
		break;
		
	case NDB_INDEX:
		ber_str2bv( c->argv[1], 0, 0, &bv );
		ai = ndb_ai_get( ni, &bv );
		if ( !ai ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: invalid attr %s",
				c->log, c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		for ( ln = ni->ni_attridxs; ln; ln = ln->ln_next ) {
			if ( ln->ln_data == (void *)ai ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: attr index already set for %s",
					c->log, c->argv[1] );
				Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
				return -1;
			}
		}
		ai->na_flag |= NDB_INFO_INDEX;
		ln = (ListNode *)ch_malloc( sizeof(ListNode));
		ln->ln_data = ai;
		ln->ln_next = NULL;
		for ( l2 = &ni->ni_attridxs; *l2; l2 = &(*l2)->ln_next );
		*l2 = ln;
		break;

	case NDB_ATSET:
		ber_str2bv( c->argv[1], 0, 0, &bv );
		bva = ndb_str2bvarray( c->argv[2], strlen( c->argv[2] ), ',', NULL );
		rc = ndb_aset_get( ni, &bv, bva, &oci );
		ber_bvarray_free( bva );
		if ( rc ) {
			if ( rc == LDAP_ALREADY_EXISTS ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%s: attrset %s already defined",
					c->log, c->argv[1] );
			} else {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					"%s: invalid attrset %s (%d)",
					c->log, c->argv[1], rc );
			}
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		ln = (ListNode *)ch_malloc( sizeof(ListNode));
		ln->ln_data = oci;
		ln->ln_next = NULL;
		for ( l2 = &ni->ni_attrsets; *l2; l2 = &(*l2)->ln_next );
		*l2 = ln;
		break;

	case NDB_ATBLOB:
		ber_str2bv( c->argv[1], 0, 0, &bv );
		ai = ndb_ai_get( ni, &bv );
		if ( !ai ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: invalid attr %s",
				c->log, c->argv[1] );
			Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
			return -1;
		}
		for ( ln = ni->ni_attrblobs; ln; ln = ln->ln_next ) {
			if ( ln->ln_data == (void *)ai ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s: attr blob already set for %s",
					c->log, c->argv[1] );
				Debug( LDAP_DEBUG_ANY, "%s\n", c->cr_msg, 0, 0 );
				return -1;
			}
		}
		ai->na_flag |= NDB_INFO_ATBLOB;
		ln = (ListNode *)ch_malloc( sizeof(ListNode));
		ln->ln_data = ai;
		ln->ln_next = NULL;
		for ( l2 = &ni->ni_attrblobs; *l2; l2 = &(*l2)->ln_next );
		*l2 = ln;
		break;

	}
	return 0;
}

extern "C"
int ndb_back_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = ndbocs;

	return config_register_schema( ndbcfg, ndbocs );
}
