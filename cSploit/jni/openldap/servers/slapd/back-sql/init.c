/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
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
#include <sys/types.h>
#include "ac/string.h"

#include "slap.h"
#include "config.h"
#include "proto-sql.h"

int
sql_back_initialize(
	BackendInfo	*bi )
{ 
	static char *controls[] = {
		LDAP_CONTROL_ASSERT,
		LDAP_CONTROL_MANAGEDSAIT,
		LDAP_CONTROL_NOOP,
#ifdef SLAP_CONTROL_X_TREE_DELETE
		SLAP_CONTROL_X_TREE_DELETE,
#endif /* SLAP_CONTROL_X_TREE_DELETE */
#ifndef BACKSQL_ARBITRARY_KEY
		LDAP_CONTROL_PAGEDRESULTS,
#endif /* ! BACKSQL_ARBITRARY_KEY */
		NULL
	};
	int rc;

	bi->bi_controls = controls;

	bi->bi_flags |=
#if 0
		SLAP_BFLAG_INCREMENT |
#endif
		SLAP_BFLAG_REFERRALS;

	Debug( LDAP_DEBUG_TRACE,"==>sql_back_initialize()\n", 0, 0, 0 );
	
	bi->bi_db_init = backsql_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = backsql_db_open;
	bi->bi_db_close = backsql_db_close;
	bi->bi_db_destroy = backsql_db_destroy;

	bi->bi_op_abandon = 0;
	bi->bi_op_compare = backsql_compare;
	bi->bi_op_bind = backsql_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = backsql_search;
	bi->bi_op_modify = backsql_modify;
	bi->bi_op_modrdn = backsql_modrdn;
	bi->bi_op_add = backsql_add;
	bi->bi_op_delete = backsql_delete;
	
	bi->bi_chk_referrals = 0;
	bi->bi_operational = backsql_operational;
	bi->bi_entry_get_rw = backsql_entry_get;
	bi->bi_entry_release_rw = backsql_entry_release;
 
	bi->bi_connection_init = 0;

	rc = backsql_init_cf( bi );
	Debug( LDAP_DEBUG_TRACE,"<==sql_back_initialize()\n", 0, 0, 0 );
	return rc;
}

int
backsql_destroy( 
	BackendInfo 	*bi )
{
	Debug( LDAP_DEBUG_TRACE, "==>backsql_destroy()\n", 0, 0, 0 );
	Debug( LDAP_DEBUG_TRACE, "<==backsql_destroy()\n", 0, 0, 0 );
	return 0;
}

int
backsql_db_init(
	BackendDB 	*bd,
	ConfigReply	*cr )
{
	backsql_info	*bi;
	int		rc = 0;
 
	Debug( LDAP_DEBUG_TRACE, "==>backsql_db_init()\n", 0, 0, 0 );

	bi = (backsql_info *)ch_calloc( 1, sizeof( backsql_info ) );
	ldap_pvt_thread_mutex_init( &bi->sql_dbconn_mutex );
	ldap_pvt_thread_mutex_init( &bi->sql_schema_mutex );

	if ( backsql_init_db_env( bi ) != SQL_SUCCESS ) {
		rc = -1;
	}

	bd->be_private = bi;
	bd->be_cf_ocs = bd->bd_info->bi_cf_ocs;

	Debug( LDAP_DEBUG_TRACE, "<==backsql_db_init()\n", 0, 0, 0 );

	return rc;
}

int
backsql_db_destroy(
	BackendDB 	*bd,
	ConfigReply	*cr )
{
	backsql_info	*bi = (backsql_info*)bd->be_private;
 
	Debug( LDAP_DEBUG_TRACE, "==>backsql_db_destroy()\n", 0, 0, 0 );

	backsql_free_db_env( bi );
	ldap_pvt_thread_mutex_destroy( &bi->sql_dbconn_mutex );
	backsql_destroy_schema_map( bi );
	ldap_pvt_thread_mutex_destroy( &bi->sql_schema_mutex );

	if ( bi->sql_dbname ) {
		ch_free( bi->sql_dbname );
	}
	if ( bi->sql_dbuser ) {
		ch_free( bi->sql_dbuser );
	}
	if ( bi->sql_dbpasswd ) {
		ch_free( bi->sql_dbpasswd );
	}
	if ( bi->sql_dbhost ) {
		ch_free( bi->sql_dbhost );
	}
	if ( bi->sql_upper_func.bv_val ) {
		ch_free( bi->sql_upper_func.bv_val );
		ch_free( bi->sql_upper_func_open.bv_val );
		ch_free( bi->sql_upper_func_close.bv_val );
	}
	if ( bi->sql_concat_func ) {
		ber_bvarray_free( bi->sql_concat_func );
	}
	if ( !BER_BVISNULL( &bi->sql_strcast_func ) ) {
		ch_free( bi->sql_strcast_func.bv_val );
	}
	if ( !BER_BVISNULL( &bi->sql_children_cond ) ) {
		ch_free( bi->sql_children_cond.bv_val );
	}
	if ( !BER_BVISNULL( &bi->sql_dn_match_cond ) ) {
		ch_free( bi->sql_dn_match_cond.bv_val );
	}
	if ( !BER_BVISNULL( &bi->sql_subtree_cond ) ) {
		ch_free( bi->sql_subtree_cond.bv_val );
	}
	if ( !BER_BVISNULL( &bi->sql_dn_oc_aliasing ) ) {
		ch_free( bi->sql_dn_oc_aliasing.bv_val );
	}
	if ( bi->sql_oc_query ) {
		ch_free( bi->sql_oc_query );
	}
	if ( bi->sql_at_query ) {
		ch_free( bi->sql_at_query );
	}
	if ( bi->sql_id_query ) {
		ch_free( bi->sql_id_query );
	}
	if ( bi->sql_has_children_query ) {
		ch_free( bi->sql_has_children_query );
	}
	if ( bi->sql_insentry_stmt ) {
		ch_free( bi->sql_insentry_stmt );
	}
	if ( bi->sql_delentry_stmt ) {
		ch_free( bi->sql_delentry_stmt );
	}
	if ( bi->sql_renentry_stmt ) {
		ch_free( bi->sql_renentry_stmt );
	}
	if ( bi->sql_delobjclasses_stmt ) {
		ch_free( bi->sql_delobjclasses_stmt );
	}
	if ( !BER_BVISNULL( &bi->sql_aliasing ) ) {
		ch_free( bi->sql_aliasing.bv_val );
	}
	if ( !BER_BVISNULL( &bi->sql_aliasing_quote ) ) {
		ch_free( bi->sql_aliasing_quote.bv_val );
	}

	if ( bi->sql_anlist ) {
		int	i;

		for ( i = 0; !BER_BVISNULL( &bi->sql_anlist[ i ].an_name ); i++ )
		{
			ch_free( bi->sql_anlist[ i ].an_name.bv_val );
		}
		ch_free( bi->sql_anlist );
	}

	if ( bi->sql_baseObject ) {
		entry_free( bi->sql_baseObject );
	}
	
	ch_free( bi );
	
	Debug( LDAP_DEBUG_TRACE, "<==backsql_db_destroy()\n", 0, 0, 0 );
	return 0;
}

int
backsql_db_open(
	BackendDB 	*bd,
	ConfigReply	*cr )
{
	backsql_info 	*bi = (backsql_info*)bd->be_private;
	struct berbuf	bb = BB_NULL;

	Connection	conn = { 0 };
	OperationBuffer opbuf;
	Operation*	op;
	SQLHDBC		dbh = SQL_NULL_HDBC;
	void		*thrctx = ldap_pvt_thread_pool_context();

	Debug( LDAP_DEBUG_TRACE, "==>backsql_db_open(): "
		"testing RDBMS connection\n", 0, 0, 0 );
	if ( bi->sql_dbname == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"datasource name not specified "
			"(use \"dbname\" directive in slapd.conf)\n", 0, 0, 0 );
		return 1;
	}

	if ( bi->sql_concat_func == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"concat func not specified (use \"concat_pattern\" "
			"directive in slapd.conf)\n", 0, 0, 0 );

		if ( backsql_split_pattern( backsql_def_concat_func, 
				&bi->sql_concat_func, 2 ) ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
				"unable to parse pattern \"%s\"",
				backsql_def_concat_func, 0, 0 );
			return 1;
		}
	}

	/*
	 * see back-sql.h for default values
	 */
	if ( BER_BVISNULL( &bi->sql_aliasing ) ) {
		ber_str2bv( BACKSQL_ALIASING,
			STRLENOF( BACKSQL_ALIASING ),
			1, &bi->sql_aliasing );
	}

	if ( BER_BVISNULL( &bi->sql_aliasing_quote ) ) {
		ber_str2bv( BACKSQL_ALIASING_QUOTE,
			STRLENOF( BACKSQL_ALIASING_QUOTE ),
			1, &bi->sql_aliasing_quote );
	}

	/*
	 * Prepare cast string as required
	 */
	if ( bi->sql_upper_func.bv_val ) {
		char buf[1024];

		if ( BACKSQL_UPPER_NEEDS_CAST( bi ) ) {
			snprintf( buf, sizeof( buf ), 
				"%s(cast (" /* ? as varchar(%d))) */ , 
				bi->sql_upper_func.bv_val );
			ber_str2bv( buf, 0, 1, &bi->sql_upper_func_open );

			snprintf( buf, sizeof( buf ),
				/* (cast(? */ " as varchar(%d)))",
				BACKSQL_MAX_DN_LEN );
			ber_str2bv( buf, 0, 1, &bi->sql_upper_func_close );

		} else {
			snprintf( buf, sizeof( buf ), "%s(" /* ?) */ ,
					bi->sql_upper_func.bv_val );
			ber_str2bv( buf, 0, 1, &bi->sql_upper_func_open );

			ber_str2bv( /* (? */ ")", 0, 1, &bi->sql_upper_func_close );
		}
	}

	/* normalize filter values only if necessary */
	bi->sql_caseIgnoreMatch = mr_find( "caseIgnoreMatch" );
	assert( bi->sql_caseIgnoreMatch != NULL );

	bi->sql_telephoneNumberMatch = mr_find( "telephoneNumberMatch" );
	assert( bi->sql_telephoneNumberMatch != NULL );

	if ( bi->sql_dbuser == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"user name not specified "
			"(use \"dbuser\" directive in slapd.conf)\n", 0, 0, 0 );
		return 1;
	}
	
	if ( BER_BVISNULL( &bi->sql_subtree_cond ) ) {
		/*
		 * Prepare concat function for subtree search condition
		 */
		struct berval	concat;
		struct berval	values[] = {
			BER_BVC( "'%'" ),
			BER_BVC( "?" ),
			BER_BVNULL
		};
		struct berbuf	bb = BB_NULL;

		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"subtree search SQL condition not specified "
			"(use \"subtree_cond\" directive in slapd.conf); "
			"preparing default\n", 
			0, 0, 0);

		if ( backsql_prepare_pattern( bi->sql_concat_func, values, 
				&concat ) ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
				"unable to prepare CONCAT pattern for subtree search",
				0, 0, 0 );
			return 1;
		}
			
		if ( bi->sql_upper_func.bv_val ) {

			/*
			 * UPPER(ldap_entries.dn) LIKE UPPER(CONCAT('%',?))
			 */

			backsql_strfcat_x( &bb, NULL, "blbbb",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn) LIKE " ),
						"(ldap_entries.dn) LIKE ",
					&bi->sql_upper_func_open,
					&concat,
					&bi->sql_upper_func_close );

		} else {

			/*
			 * ldap_entries.dn LIKE CONCAT('%',?)
			 */

			backsql_strfcat_x( &bb, NULL, "lb",
					(ber_len_t)STRLENOF( "ldap_entries.dn LIKE " ),
						"ldap_entries.dn LIKE ",
					&concat );
		}

		ch_free( concat.bv_val );

		bi->sql_subtree_cond = bb.bb_val;
			
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" as default \"subtree_cond\"\n",
			bi->sql_subtree_cond.bv_val, 0, 0 );
	}

	if ( bi->sql_children_cond.bv_val == NULL ) {
		/*
		 * Prepare concat function for children search condition
		 */
		struct berval	concat;
		struct berval	values[] = {
			BER_BVC( "'%,'" ),
			BER_BVC( "?" ),
			BER_BVNULL
		};
		struct berbuf	bb = BB_NULL;

		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"children search SQL condition not specified "
			"(use \"children_cond\" directive in slapd.conf); "
			"preparing default\n", 
			0, 0, 0);

		if ( backsql_prepare_pattern( bi->sql_concat_func, values, 
				&concat ) ) {
			Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
				"unable to prepare CONCAT pattern for children search", 0, 0, 0 );
			return 1;
		}
			
		if ( bi->sql_upper_func.bv_val ) {

			/*
			 * UPPER(ldap_entries.dn) LIKE UPPER(CONCAT('%,',?))
			 */

			backsql_strfcat_x( &bb, NULL, "blbbb",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn) LIKE " ),
						"(ldap_entries.dn) LIKE ",
					&bi->sql_upper_func_open,
					&concat,
					&bi->sql_upper_func_close );

		} else {

			/*
			 * ldap_entries.dn LIKE CONCAT('%,',?)
			 */

			backsql_strfcat_x( &bb, NULL, "lb",
					(ber_len_t)STRLENOF( "ldap_entries.dn LIKE " ),
						"ldap_entries.dn LIKE ",
					&concat );
		}

		ch_free( concat.bv_val );

		bi->sql_children_cond = bb.bb_val;
			
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" as default \"children_cond\"\n",
			bi->sql_children_cond.bv_val, 0, 0 );
	}

	if ( bi->sql_dn_match_cond.bv_val == NULL ) {
		/*
		 * Prepare concat function for dn match search condition
		 */
		struct berbuf	bb = BB_NULL;

		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"DN match search SQL condition not specified "
			"(use \"dn_match_cond\" directive in slapd.conf); "
			"preparing default\n", 
			0, 0, 0);

		if ( bi->sql_upper_func.bv_val ) {

			/*
			 * UPPER(ldap_entries.dn)=?
			 */

			backsql_strfcat_x( &bb, NULL, "blbcb",
					&bi->sql_upper_func,
					(ber_len_t)STRLENOF( "(ldap_entries.dn)=" ),
						"(ldap_entries.dn)=",
					&bi->sql_upper_func_open,
					'?',
					&bi->sql_upper_func_close );

		} else {

			/*
			 * ldap_entries.dn=?
			 */

			backsql_strfcat_x( &bb, NULL, "l",
					(ber_len_t)STRLENOF( "ldap_entries.dn=?" ),
						"ldap_entries.dn=?" );
		}

		bi->sql_dn_match_cond = bb.bb_val;
			
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" as default \"dn_match_cond\"\n",
			bi->sql_dn_match_cond.bv_val, 0, 0 );
	}

	if ( bi->sql_oc_query == NULL ) {
		if ( BACKSQL_CREATE_NEEDS_SELECT( bi ) ) {
			bi->sql_oc_query =
				ch_strdup( backsql_def_needs_select_oc_query );

		} else {
			bi->sql_oc_query = ch_strdup( backsql_def_oc_query );
		}

		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"objectclass mapping SQL statement not specified "
			"(use \"oc_query\" directive in slapd.conf)\n", 
			0, 0, 0 );
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n", bi->sql_oc_query, 0, 0 );
	}
	
	if ( bi->sql_at_query == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"attribute mapping SQL statement not specified "
			"(use \"at_query\" directive in slapd.conf)\n",
			0, 0, 0 );
		Debug(LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n",
			backsql_def_at_query, 0, 0 );
		bi->sql_at_query = ch_strdup( backsql_def_at_query );
	}
	
	if ( bi->sql_insentry_stmt == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"entry insertion SQL statement not specified "
			"(use \"insentry_stmt\" directive in slapd.conf)\n",
			0, 0, 0 );
		Debug(LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n",
			backsql_def_insentry_stmt, 0, 0 );
		bi->sql_insentry_stmt = ch_strdup( backsql_def_insentry_stmt );
	}
	
	if ( bi->sql_delentry_stmt == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"entry deletion SQL statement not specified "
			"(use \"delentry_stmt\" directive in slapd.conf)\n",
			0, 0, 0 );
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n",
			backsql_def_delentry_stmt, 0, 0 );
		bi->sql_delentry_stmt = ch_strdup( backsql_def_delentry_stmt );
	}

	if ( bi->sql_renentry_stmt == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"entry deletion SQL statement not specified "
			"(use \"renentry_stmt\" directive in slapd.conf)\n",
			0, 0, 0 );
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n",
			backsql_def_renentry_stmt, 0, 0 );
		bi->sql_renentry_stmt = ch_strdup( backsql_def_renentry_stmt );
	}

	if ( bi->sql_delobjclasses_stmt == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"objclasses deletion SQL statement not specified "
			"(use \"delobjclasses_stmt\" directive in slapd.conf)\n",
			0, 0, 0 );
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"setting \"%s\" by default\n",
			backsql_def_delobjclasses_stmt, 0, 0 );
		bi->sql_delobjclasses_stmt = ch_strdup( backsql_def_delobjclasses_stmt );
	}

	/* This should just be to force schema loading */
	connection_fake_init2( &conn, &opbuf, thrctx, 0 );
	op = &opbuf.ob_op;
	op->o_bd = bd;
	if ( backsql_get_db_conn( op, &dbh ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"connection failed, exiting\n", 0, 0, 0 );
		return 1;
	}
	if ( backsql_load_schema_map( bi, dbh ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"schema mapping failed, exiting\n", 0, 0, 0 );
		return 1;
	}
	if ( backsql_free_db_conn( op, dbh ) != SQL_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"connection free failed\n", 0, 0, 0 );
	}
	if ( !BACKSQL_SCHEMA_LOADED( bi ) ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_db_open(): "
			"test failed, schema map not loaded - exiting\n",
			0, 0, 0 );
		return 1;
	}

	/*
	 * Prepare ID selection query
	 */
	if ( bi->sql_id_query == NULL ) {
		/* no custom id_query provided */
		if ( bi->sql_upper_func.bv_val == NULL ) {
			backsql_strcat_x( &bb, NULL, backsql_id_query, "dn=?", NULL );

		} else {
			if ( BACKSQL_HAS_LDAPINFO_DN_RU( bi ) ) {
				backsql_strcat_x( &bb, NULL, backsql_id_query,
						"dn_ru=?", NULL );
			} else {
				if ( BACKSQL_USE_REVERSE_DN( bi ) ) {
					backsql_strfcat_x( &bb, NULL, "sbl",
							backsql_id_query,
							&bi->sql_upper_func, 
							(ber_len_t)STRLENOF( "(dn)=?" ), "(dn)=?" );
				} else {
					backsql_strfcat_x( &bb, NULL, "sblbcb",
							backsql_id_query,
							&bi->sql_upper_func, 
							(ber_len_t)STRLENOF( "(dn)=" ), "(dn)=",
							&bi->sql_upper_func_open, 
							'?', 
							&bi->sql_upper_func_close );
				}
			}
		}
		bi->sql_id_query = bb.bb_val.bv_val;
	}

	/*
	 * Prepare children count query
	 */
	BER_BVZERO( &bb.bb_val );
	bb.bb_len = 0;
	backsql_strfcat_x( &bb, NULL, "sbsb",
			"SELECT COUNT(distinct subordinates.id) "
			"FROM ldap_entries,ldap_entries ",
			&bi->sql_aliasing, "subordinates "
			"WHERE subordinates.parent=ldap_entries.id AND ",
			&bi->sql_dn_match_cond );
	bi->sql_has_children_query = bb.bb_val.bv_val;
 
	/*
	 * Prepare DN and objectClass aliasing bit of query
	 */
	BER_BVZERO( &bb.bb_val );
	bb.bb_len = 0;
	backsql_strfcat_x( &bb, NULL, "sbbsbsbbsb",
			" ", &bi->sql_aliasing, &bi->sql_aliasing_quote,
			"objectClass", &bi->sql_aliasing_quote,
			",ldap_entries.dn ", &bi->sql_aliasing,
			&bi->sql_aliasing_quote, "dn", &bi->sql_aliasing_quote );
	bi->sql_dn_oc_aliasing = bb.bb_val;
 
	/* should never happen! */
	assert( bd->be_nsuffix != NULL );
	
	if ( BER_BVISNULL( &bd->be_nsuffix[ 1 ] ) ) {
		/* enable if only one suffix is defined */
		bi->sql_flags |= BSQLF_USE_SUBTREE_SHORTCUT;
	}

	bi->sql_flags |= BSQLF_CHECK_SCHEMA;
	
	Debug( LDAP_DEBUG_TRACE, "<==backsql_db_open(): "
		"test succeeded, schema map loaded\n", 0, 0, 0 );
	return 0;
}

int
backsql_db_close(
	BackendDB	*bd,
	ConfigReply	*cr )
{
	backsql_info 	*bi = (backsql_info*)bd->be_private;

	Debug( LDAP_DEBUG_TRACE, "==>backsql_db_close()\n", 0, 0, 0 );

	backsql_conn_destroy( bi );

	Debug( LDAP_DEBUG_TRACE, "<==backsql_db_close()\n", 0, 0, 0 );

	return 0;
}

#if SLAPD_SQL == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( sql )

#endif /* SLAPD_SQL == SLAPD_MOD_DYNAMIC */

