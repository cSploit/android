/* init.cpp - initialize ndb backend */
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

#include <stdio.h>
#include <ac/string.h>
#include <ac/unistd.h>
#include <ac/stdlib.h>
#include <ac/errno.h>
#include <sys/stat.h>
#include "back-ndb.h"
#include <lutil.h>
#include "config.h"

extern "C" {
	static BI_db_init ndb_db_init;
	static BI_db_close ndb_db_close;
	static BI_db_open ndb_db_open;
	static BI_db_destroy ndb_db_destroy;
}

static struct berval ndb_optable = BER_BVC("OL_opattrs");

static struct berval ndb_opattrs[] = {
	BER_BVC("structuralObjectClass"),
	BER_BVC("entryUUID"),
	BER_BVC("creatorsName"),
	BER_BVC("createTimestamp"),
	BER_BVC("entryCSN"),
	BER_BVC("modifiersName"),
	BER_BVC("modifyTimestamp"),
	BER_BVNULL
};

static int ndb_oplens[] = {
	0,	/* structuralOC, default */
	36,	/* entryUUID */
	0,	/* creatorsName, default */
	26,	/* createTimestamp */
	40,	/* entryCSN */
	0,	/* modifiersName, default */
	26,	/* modifyTimestamp */
	-1
};

static Uint32 ndb_lastrow[1];
NdbInterpretedCode *ndb_lastrow_code;

static int
ndb_db_init( BackendDB *be, ConfigReply *cr )
{
	struct ndb_info	*ni;
	int rc = 0;

	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_db_init) ": Initializing ndb database\n",
		0, 0, 0 );

	/* allocate backend-database-specific stuff */
	ni = (struct ndb_info *) ch_calloc( 1, sizeof(struct ndb_info) );

	be->be_private = ni;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	ni->ni_search_stack_depth = DEFAULT_SEARCH_STACK_DEPTH;

	ldap_pvt_thread_rdwr_init( &ni->ni_ai_rwlock );
	ldap_pvt_thread_rdwr_init( &ni->ni_oc_rwlock );
	ldap_pvt_thread_mutex_init( &ni->ni_conn_mutex );

#ifdef DO_MONITORING
	rc = ndb_monitor_db_init( be );
#endif

	return rc;
}

static int
ndb_db_close( BackendDB *be, ConfigReply *cr );

static int
ndb_db_open( BackendDB *be, ConfigReply *cr )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;
	char sqlbuf[BUFSIZ], *ptr;
	int rc, i;

	if ( be->be_suffix == NULL ) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: need suffix" );
		Debug( LDAP_DEBUG_ANY, "%s\n",
			cr->msg, 0, 0 );
		return -1;
	}

	Debug( LDAP_DEBUG_ARGS,
		LDAP_XSTRING(ndb_db_open) ": \"%s\"\n",
		be->be_suffix[0].bv_val, 0, 0 );

	if ( ni->ni_nconns < 1 )
		ni->ni_nconns = 1;

	ni->ni_cluster = (Ndb_cluster_connection **)ch_calloc( ni->ni_nconns, sizeof( Ndb_cluster_connection *));
	for ( i=0; i<ni->ni_nconns; i++ ) {
		ni->ni_cluster[i] = new Ndb_cluster_connection( ni->ni_connectstr );
		rc = ni->ni_cluster[i]->connect( 20, 5, 1 );
		if ( rc ) {
			snprintf( cr->msg, sizeof( cr->msg ),
				"ndb_db_open: ni_cluster[%d]->connect failed (%d)",
				i, rc );
			goto fail;
		}
	}
	for ( i=0; i<ni->ni_nconns; i++ ) {
		rc = ni->ni_cluster[i]->wait_until_ready( 30, 30 );
		if ( rc ) {
			snprintf( cr->msg, sizeof( cr->msg ),
				"ndb_db_open: ni_cluster[%d]->wait failed (%d)",
				i, rc );
			goto fail;
		}
	}

	mysql_init( &ni->ni_sql );
	if ( !mysql_real_connect( &ni->ni_sql, ni->ni_hostname, ni->ni_username, ni->ni_password,
		"", ni->ni_port, ni->ni_socket, ni->ni_clflag )) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: mysql_real_connect failed, %s (%d)",
			mysql_error(&ni->ni_sql), mysql_errno(&ni->ni_sql) );
		rc = -1;
		goto fail;
	}

	sprintf( sqlbuf, "CREATE DATABASE IF NOT EXISTS %s", ni->ni_dbname );
	rc = mysql_query( &ni->ni_sql, sqlbuf );
	if ( rc ) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: CREATE DATABASE %s failed, %s (%d)",
			ni->ni_dbname, mysql_error(&ni->ni_sql), mysql_errno(&ni->ni_sql) );
		goto fail;
	}

	sprintf( sqlbuf, "USE %s", ni->ni_dbname );
	rc = mysql_query( &ni->ni_sql, sqlbuf );
	if ( rc ) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: USE DATABASE %s failed, %s (%d)",
			ni->ni_dbname, mysql_error(&ni->ni_sql), mysql_errno(&ni->ni_sql) );
		goto fail;
	}

	ptr = sqlbuf;
	ptr += sprintf( ptr, "CREATE TABLE IF NOT EXISTS " DN2ID_TABLE " ("
		"eid bigint unsigned NOT NULL, "
		"object_classes VARCHAR(1024) NOT NULL, "
		"a0 VARCHAR(128) NOT NULL DEFAULT '', "
		"a1 VARCHAR(128) NOT NULL DEFAULT '', "
		"a2 VARCHAR(128) NOT NULL DEFAULT '', "
		"a3 VARCHAR(128) NOT NULL DEFAULT '', "
		"a4 VARCHAR(128) NOT NULL DEFAULT '', "
		"a5 VARCHAR(128) NOT NULL DEFAULT '', "
		"a6 VARCHAR(128) NOT NULL DEFAULT '', "
		"a7 VARCHAR(128) NOT NULL DEFAULT '', "
		"a8 VARCHAR(128) NOT NULL DEFAULT '', "
		"a9 VARCHAR(128) NOT NULL DEFAULT '', "
		"a10 VARCHAR(128) NOT NULL DEFAULT '', "
		"a11 VARCHAR(128) NOT NULL DEFAULT '', "
		"a12 VARCHAR(128) NOT NULL DEFAULT '', "
		"a13 VARCHAR(128) NOT NULL DEFAULT '', "
		"a14 VARCHAR(128) NOT NULL DEFAULT '', "
		"a15 VARCHAR(128) NOT NULL DEFAULT '', "
		"PRIMARY KEY (a0, a1, a2, a3, a4, a5, a6, a7, a8, a9, a10, a11, a12, a13, a14, a15), "
		"UNIQUE KEY eid (eid) USING HASH" );
	/* Create index columns */
	if ( ni->ni_attridxs ) {
		ListNode *ln;
		int newcol = 0;

		*ptr++ = ',';
		*ptr++ = ' ';
		for ( ln = ni->ni_attridxs; ln; ln=ln->ln_next ) {
			NdbAttrInfo *ai = (NdbAttrInfo *)ln->ln_data;
			ptr += sprintf( ptr, "`%s` VARCHAR(%d), ",
				ai->na_name.bv_val, ai->na_len );
		}
		ptr = lutil_strcopy(ptr, "KEY " INDEX_NAME " (" );

		for ( ln = ni->ni_attridxs; ln; ln=ln->ln_next ) {
			NdbAttrInfo *ai = (NdbAttrInfo *)ln->ln_data;
			if ( newcol ) *ptr++ = ',';
			*ptr++ = '`';
			ptr = lutil_strcopy( ptr, ai->na_name.bv_val );
			*ptr++ = '`';
			ai->na_ixcol = newcol + 18;
			newcol++;
		}
		*ptr++ = ')';
	}
	strcpy( ptr, ") ENGINE=ndb" );
	rc = mysql_query( &ni->ni_sql, sqlbuf );
	if ( rc ) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: CREATE TABLE " DN2ID_TABLE " failed, %s (%d)",
			mysql_error(&ni->ni_sql), mysql_errno(&ni->ni_sql) );
		goto fail;
	}

	rc = mysql_query( &ni->ni_sql, "CREATE TABLE IF NOT EXISTS " NEXTID_TABLE " ("
		"a bigint unsigned AUTO_INCREMENT PRIMARY KEY ) ENGINE=ndb" );
	if ( rc ) {
		snprintf( cr->msg, sizeof( cr->msg ),
			"ndb_db_open: CREATE TABLE " NEXTID_TABLE " failed, %s (%d)",
			mysql_error(&ni->ni_sql), mysql_errno(&ni->ni_sql) );
		goto fail;
	}

	{
		NdbOcInfo *oci;

		rc = ndb_aset_get( ni, &ndb_optable, ndb_opattrs, &oci );
		if ( rc ) {
			snprintf( cr->msg, sizeof( cr->msg ),
				"ndb_db_open: ndb_aset_get( %s ) failed (%d)",
				ndb_optable.bv_val, rc );
			goto fail;
		}
		for ( i=0; ndb_oplens[i] >= 0; i++ ) {
			if ( ndb_oplens[i] )
				oci->no_attrs[i]->na_len = ndb_oplens[i];
		}
		rc = ndb_aset_create( ni, oci );
		if ( rc ) {
			snprintf( cr->msg, sizeof( cr->msg ),
				"ndb_db_open: ndb_aset_create( %s ) failed (%d)",
				ndb_optable.bv_val, rc );
			goto fail;
		}
		ni->ni_opattrs = oci;
	}
	/* Create attribute sets */
	{
		ListNode *ln;

		for ( ln = ni->ni_attrsets; ln; ln=ln->ln_next ) {
			NdbOcInfo *oci = (NdbOcInfo *)ln->ln_data;
			rc = ndb_aset_create( ni, oci );
			if ( rc ) {
				snprintf( cr->msg, sizeof( cr->msg ),
					"ndb_db_open: ndb_aset_create( %s ) failed (%d)",
					oci->no_name.bv_val, rc );
				goto fail;
			}
		}
	}
	/* Initialize any currently used objectClasses */
	{
		Ndb *ndb;
		const NdbDictionary::Dictionary *myDict;

		ndb = new Ndb( ni->ni_cluster[0], ni->ni_dbname );
		ndb->init(1024);

		myDict = ndb->getDictionary();
		ndb_oc_read( ni, myDict );
		delete ndb;
	}

#ifdef DO_MONITORING
	/* monitor setup */
	rc = ndb_monitor_db_open( be );
	if ( rc != 0 ) {
		goto fail;
	}
#endif

	return 0;

fail:
	Debug( LDAP_DEBUG_ANY, "%s\n",
		cr->msg, 0, 0 );
	ndb_db_close( be, NULL );
	return rc;
}

static int
ndb_db_close( BackendDB *be, ConfigReply *cr )
{
	int i;
	struct ndb_info *ni = (struct ndb_info *) be->be_private;

	mysql_close( &ni->ni_sql );
	if ( ni->ni_cluster ) {
		for ( i=0; i<ni->ni_nconns; i++ ) {
			if ( ni->ni_cluster[i] ) {
				delete ni->ni_cluster[i];
				ni->ni_cluster[i] = NULL;
			}
		}
		ch_free( ni->ni_cluster );
		ni->ni_cluster = NULL;
	}

#ifdef DO_MONITORING
	/* monitor handling */
	(void)ndb_monitor_db_close( be );
#endif

	return 0;
}

static int
ndb_db_destroy( BackendDB *be, ConfigReply *cr )
{
	struct ndb_info *ni = (struct ndb_info *) be->be_private;

#ifdef DO_MONITORING
	/* monitor handling */
	(void)ndb_monitor_db_destroy( be );
#endif

	ldap_pvt_thread_mutex_destroy( &ni->ni_conn_mutex );
	ldap_pvt_thread_rdwr_destroy( &ni->ni_ai_rwlock );
	ldap_pvt_thread_rdwr_destroy( &ni->ni_oc_rwlock );

	ch_free( ni );
	be->be_private = NULL;

	return 0;
}

extern "C" int
ndb_back_initialize(
	BackendInfo	*bi )
{
	static char *controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
		LDAP_CONTROL_PAGEDRESULTS,
		LDAP_CONTROL_PRE_READ,
		LDAP_CONTROL_POST_READ,
		LDAP_CONTROL_SUBENTRIES,
		LDAP_CONTROL_X_PERMISSIVE_MODIFY,
#ifdef LDAP_X_TXN
		LDAP_CONTROL_X_TXN_SPEC,
#endif
		NULL
	};

	int rc = 0;

	/* initialize the underlying database system */
	Debug( LDAP_DEBUG_TRACE,
		LDAP_XSTRING(ndb_back_initialize) ": initialize ndb backend\n", 0, 0, 0 );

	ndb_init();

	ndb_lastrow_code = new NdbInterpretedCode( NULL, ndb_lastrow, 1 );
	ndb_lastrow_code->interpret_exit_last_row();
	ndb_lastrow_code->finalise();

	bi->bi_flags |=
		SLAP_BFLAG_INCREMENT |
		SLAP_BFLAG_SUBENTRIES |
		SLAP_BFLAG_ALIASES |
		SLAP_BFLAG_REFERRALS;

	bi->bi_controls = controls;

	bi->bi_open = 0;
	bi->bi_close = 0;
	bi->bi_config = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = ndb_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = ndb_db_open;
	bi->bi_db_close = ndb_db_close;
	bi->bi_db_destroy = ndb_db_destroy;

	bi->bi_op_add = ndb_back_add;
	bi->bi_op_bind = ndb_back_bind;
	bi->bi_op_compare = ndb_back_compare;
	bi->bi_op_delete = ndb_back_delete;
	bi->bi_op_modify = ndb_back_modify;
	bi->bi_op_modrdn = ndb_back_modrdn;
	bi->bi_op_search = ndb_back_search;

	bi->bi_op_unbind = 0;

#if 0
	bi->bi_extended = ndb_extended;

	bi->bi_chk_referrals = ndb_referrals;
#endif
	bi->bi_operational = ndb_operational;
	bi->bi_has_subordinates = ndb_has_subordinates;
	bi->bi_entry_release_rw = 0;
	bi->bi_entry_get_rw = ndb_entry_get;

	/*
	 * hooks for slap tools
	 */
	bi->bi_tool_entry_open = ndb_tool_entry_open;
	bi->bi_tool_entry_close = ndb_tool_entry_close;
	bi->bi_tool_entry_first = ndb_tool_entry_first;
	bi->bi_tool_entry_next = ndb_tool_entry_next;
	bi->bi_tool_entry_get = ndb_tool_entry_get;
	bi->bi_tool_entry_put = ndb_tool_entry_put;
#if 0
	bi->bi_tool_entry_reindex = ndb_tool_entry_reindex;
	bi->bi_tool_sync = 0;
	bi->bi_tool_dn2id_get = ndb_tool_dn2id_get;
	bi->bi_tool_entry_modify = ndb_tool_entry_modify;
#endif

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	rc = ndb_back_init_cf( bi );

	return rc;
}

#if	SLAPD_NDB == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
extern "C" { int init_module( int argc, char *argv[] ); }

SLAP_BACKEND_INIT_MODULE( ndb )

#endif /* SLAPD_NDB == SLAPD_MOD_DYNAMIC */

