/* backend.c - routines for dealing with back-end databases */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */


#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>
#include <sys/stat.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"
#include "lber_pvt.h"

/*
 * If a module is configured as dynamic, its header should not
 * get included into slapd. While this is a general rule and does
 * not have much of an effect in UNIX, this rule should be adhered
 * to for Windows, where dynamic object code should not be implicitly
 * imported into slapd without appropriate __declspec(dllimport) directives.
 */

int			nBackendInfo = 0;
slap_bi_head backendInfo = LDAP_STAILQ_HEAD_INITIALIZER(backendInfo);

int			nBackendDB = 0; 
slap_be_head backendDB = LDAP_STAILQ_HEAD_INITIALIZER(backendDB);

static int
backend_init_controls( BackendInfo *bi )
{
	if ( bi->bi_controls ) {
		int	i;

		for ( i = 0; bi->bi_controls[ i ]; i++ ) {
			int	cid;

			if ( slap_find_control_id( bi->bi_controls[ i ], &cid )
					== LDAP_CONTROL_NOT_FOUND )
			{
				if ( !( slapMode & SLAP_TOOL_MODE ) ) {
					assert( 0 );
				}

				return -1;
			}

			bi->bi_ctrls[ cid ] = 1;
		}
	}

	return 0;
}

int backend_init(void)
{
	int rc = -1;
	BackendInfo *bi;

	if((nBackendInfo != 0) || !LDAP_STAILQ_EMPTY(&backendInfo)) {
		/* already initialized */
		Debug( LDAP_DEBUG_ANY,
			"backend_init: already initialized\n", 0, 0, 0 );
		return -1;
	}

	for( bi=slap_binfo; bi->bi_type != NULL; bi++,nBackendInfo++ ) {
		assert( bi->bi_init != 0 );

		rc = bi->bi_init( bi );

		if(rc != 0) {
			Debug( LDAP_DEBUG_ANY,
				"backend_init: initialized for type \"%s\"\n",
				bi->bi_type, 0, 0 );
			/* destroy those we've already inited */
			for( nBackendInfo--;
				nBackendInfo >= 0 ;
				nBackendInfo-- )
			{ 
				if ( slap_binfo[nBackendInfo].bi_destroy ) {
					slap_binfo[nBackendInfo].bi_destroy(
						&slap_binfo[nBackendInfo] );
				}
			}
			return rc;
		}

		LDAP_STAILQ_INSERT_TAIL(&backendInfo, bi, bi_next);
	}

	if ( nBackendInfo > 0) {
		return 0;
	}

#ifdef SLAPD_MODULES	
	return 0;
#else

	Debug( LDAP_DEBUG_ANY,
		"backend_init: failed\n",
		0, 0, 0 );

	return rc;
#endif /* SLAPD_MODULES */
}

int backend_add(BackendInfo *aBackendInfo)
{
	int rc = 0;

	if ( aBackendInfo->bi_init == NULL ) {
		Debug( LDAP_DEBUG_ANY, "backend_add: "
			"backend type \"%s\" does not have the (mandatory)init function\n",
			aBackendInfo->bi_type, 0, 0 );
		return -1;
	}

	rc = aBackendInfo->bi_init(aBackendInfo);
	if ( rc != 0) {
		Debug( LDAP_DEBUG_ANY,
			"backend_add:  initialization for type \"%s\" failed\n",
			aBackendInfo->bi_type, 0, 0 );
		return rc;
	}

	(void)backend_init_controls( aBackendInfo );

	/* now add the backend type to the Backend Info List */
	LDAP_STAILQ_INSERT_TAIL( &backendInfo, aBackendInfo, bi_next );
	nBackendInfo++;
	return 0;
}

static int
backend_set_controls( BackendDB *be )
{
	BackendInfo	*bi = be->bd_info;

	/* back-relay takes care of itself; so may do other */
	if ( overlay_is_over( be ) ) {
		bi = ((slap_overinfo *)be->bd_info->bi_private)->oi_orig;
	}

	if ( bi->bi_controls ) {
		if ( be->be_ctrls[ SLAP_MAX_CIDS ] == 0 ) {
			AC_MEMCPY( be->be_ctrls, bi->bi_ctrls,
					sizeof( be->be_ctrls ) );
			be->be_ctrls[ SLAP_MAX_CIDS ] = 1;
			
		} else {
			int	i;
			
			for ( i = 0; i < SLAP_MAX_CIDS; i++ ) {
				if ( bi->bi_ctrls[ i ] ) {
					be->be_ctrls[ i ] = bi->bi_ctrls[ i ];
				}
			}
		}

	}

	return 0;
}

/* startup a specific backend database */
int backend_startup_one(Backend *be, ConfigReply *cr)
{
	int		rc = 0;

	assert( be != NULL );

	be->be_pending_csn_list = (struct be_pcl *)
		ch_calloc( 1, sizeof( struct be_pcl ) );

	LDAP_TAILQ_INIT( be->be_pending_csn_list );

	Debug( LDAP_DEBUG_TRACE,
		"backend_startup_one: starting \"%s\"\n",
		be->be_suffix ? be->be_suffix[0].bv_val : "(unknown)",
		0, 0 );

	/* set database controls */
	(void)backend_set_controls( be );

#if 0
	if ( !BER_BVISEMPTY( &be->be_rootndn )
		&& select_backend( &be->be_rootndn, 0 ) == be
		&& BER_BVISNULL( &be->be_rootpw ) )
	{
		/* warning: if rootdn entry is created,
		 * it can take rootdn privileges;
		 * set empty rootpw to prevent */
	}
#endif

	if ( be->bd_info->bi_db_open ) {
		rc = be->bd_info->bi_db_open( be, cr );
		if ( rc == 0 ) {
			(void)backend_set_controls( be );

		} else {
			char *type = be->bd_info->bi_type;
			char *suffix = "(null)";

			if ( overlay_is_over( be ) ) {
				slap_overinfo	*oi = (slap_overinfo *)be->bd_info->bi_private;
				type = oi->oi_orig->bi_type;
			}

			if ( be->be_suffix != NULL && !BER_BVISNULL( &be->be_suffix[0] ) ) {
				suffix = be->be_suffix[0].bv_val;
			}

			Debug( LDAP_DEBUG_ANY,
				"backend_startup_one (type=%s, suffix=\"%s\"): "
				"bi_db_open failed! (%d)\n",
				type, suffix, rc );
		}
	}

	return rc;
}

int backend_startup(Backend *be)
{
	int i;
	int rc = 0;
	BackendInfo *bi;
	ConfigReply cr={0, ""};

	if( ! ( nBackendDB > 0 ) ) {
		/* no databases */
		Debug( LDAP_DEBUG_ANY,
			"backend_startup: %d databases to startup.\n",
			nBackendDB, 0, 0 );
		return 1;
	}

	if(be != NULL) {
		/* silent noop if disabled */
		if ( SLAP_DBDISABLED( be ))
			return 0;
		if ( be->bd_info->bi_open ) {
			rc = be->bd_info->bi_open( be->bd_info );
			if ( rc != 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"backend_startup: bi_open failed!\n",
					0, 0, 0 );

				return rc;
			}
		}

		return backend_startup_one( be, &cr );
	}

	/* open frontend, if required */
	if ( frontendDB->bd_info->bi_db_open ) {
		rc = frontendDB->bd_info->bi_db_open( frontendDB, &cr );
		if ( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"backend_startup: bi_db_open(frontend) failed! (%d)\n",
				rc, 0, 0 );
			return rc;
		}
	}

	/* open each backend type */
	i = -1;
	LDAP_STAILQ_FOREACH(bi, &backendInfo, bi_next) {
		i++;
		if( bi->bi_nDB == 0) {
			/* no database of this type, don't open */
			continue;
		}

		if( bi->bi_open ) {
			rc = bi->bi_open( bi );
			if ( rc != 0 ) {
				Debug( LDAP_DEBUG_ANY,
					"backend_startup: bi_open %d (%s) failed!\n",
					i, bi->bi_type, 0 );
				return rc;
			}
		}

		(void)backend_init_controls( bi );
	}

	/* open each backend database */
	i = -1;
	LDAP_STAILQ_FOREACH(be, &backendDB, be_next) {
		i++;
		if ( SLAP_DBDISABLED( be ))
			continue;
		if ( be->be_suffix == NULL ) {
			Debug( LDAP_DEBUG_ANY,
				"backend_startup: warning, database %d (%s) "
				"has no suffix\n",
				i, be->bd_info->bi_type, 0 );
		}

		rc = backend_startup_one( be, &cr );

		if ( rc ) return rc;
	}

	return rc;
}

int backend_num( Backend *be )
{
	int i = 0;
	BackendDB *b2;

	if( be == NULL ) return -1;

	LDAP_STAILQ_FOREACH( b2, &backendDB, be_next ) {
		if( be == b2 ) return i;
		i++;
	}
	return -1;
}

int backend_shutdown( Backend *be )
{
	int rc = 0;
	BackendInfo *bi;

	if( be != NULL ) {
		/* shutdown a specific backend database */

		if ( be->bd_info->bi_nDB == 0 ) {
			/* no database of this type, we never opened it */
			return 0;
		}

		if ( be->bd_info->bi_db_close ) {
			rc = be->bd_info->bi_db_close( be, NULL );
			if ( rc ) return rc;
		}

		if( be->bd_info->bi_close ) {
			rc = be->bd_info->bi_close( be->bd_info );
			if ( rc ) return rc;
		}

		return 0;
	}

	/* close each backend database */
	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( SLAP_DBDISABLED( be ))
			continue;
		if ( be->bd_info->bi_db_close ) {
			be->bd_info->bi_db_close( be, NULL );
		}

		if(rc != 0) {
			Debug( LDAP_DEBUG_ANY,
				"backend_close: bi_db_close %s failed!\n",
				be->be_type, 0, 0 );
		}
	}

	/* close each backend type */
	LDAP_STAILQ_FOREACH( bi, &backendInfo, bi_next ) {
		if( bi->bi_nDB == 0 ) {
			/* no database of this type */
			continue;
		}

		if( bi->bi_close ) {
			bi->bi_close( bi );
		}
	}

	/* close frontend, if required */
	if ( frontendDB->bd_info->bi_db_close ) {
		rc = frontendDB->bd_info->bi_db_close ( frontendDB, NULL );
		if ( rc != 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"backend_startup: bi_db_close(frontend) failed! (%d)\n",
				rc, 0, 0 );
		}
	}

	return 0;
}

/*
 * This function is supposed to be the exact counterpart
 * of backend_startup_one(), although this one calls bi_db_destroy()
 * while backend_startup_one() calls bi_db_open().
 *
 * Make sure backend_stopdown_one() destroys resources allocated
 * by backend_startup_one(); only call backend_destroy_one() when
 * all stuff in a BackendDB needs to be destroyed
 */
void
backend_stopdown_one( BackendDB *bd )
{
	if ( bd->be_pending_csn_list ) {
		struct slap_csn_entry *csne;
		csne = LDAP_TAILQ_FIRST( bd->be_pending_csn_list );
		while ( csne ) {
			struct slap_csn_entry *tmp_csne = csne;

			LDAP_TAILQ_REMOVE( bd->be_pending_csn_list, csne, ce_csn_link );
			ch_free( csne->ce_csn.bv_val );
			csne = LDAP_TAILQ_NEXT( csne, ce_csn_link );
			ch_free( tmp_csne );
		}
		ch_free( bd->be_pending_csn_list );
	}

	if ( bd->bd_info->bi_db_destroy ) {
		bd->bd_info->bi_db_destroy( bd, NULL );
	}
}

void backend_destroy_one( BackendDB *bd, int dynamic )
{
	if ( dynamic ) {
		LDAP_STAILQ_REMOVE(&backendDB, bd, BackendDB, be_next );
	}

	if ( bd->be_syncinfo ) {
		syncinfo_free( bd->be_syncinfo, 1 );
	}

	backend_stopdown_one( bd );

	ber_bvarray_free( bd->be_suffix );
	ber_bvarray_free( bd->be_nsuffix );
	if ( !BER_BVISNULL( &bd->be_rootdn ) ) {
		free( bd->be_rootdn.bv_val );
	}
	if ( !BER_BVISNULL( &bd->be_rootndn ) ) {
		free( bd->be_rootndn.bv_val );
	}
	if ( !BER_BVISNULL( &bd->be_rootpw ) ) {
		free( bd->be_rootpw.bv_val );
	}
	acl_destroy( bd->be_acl );
	limits_destroy( bd->be_limits );
	if ( bd->be_extra_anlist ) {
		anlist_free( bd->be_extra_anlist, 1, NULL );
	}
	if ( !BER_BVISNULL( &bd->be_update_ndn ) ) {
		ch_free( bd->be_update_ndn.bv_val );
	}
	if ( bd->be_update_refs ) {
		ber_bvarray_free( bd->be_update_refs );
	}

	ldap_pvt_thread_mutex_destroy( &bd->be_pcl_mutex );

	if ( dynamic ) {
		free( bd );
	}
}

int backend_destroy(void)
{
	BackendDB *bd;
	BackendInfo *bi;

	/* destroy each backend database */
	while (( bd = LDAP_STAILQ_FIRST(&backendDB))) {
		backend_destroy_one( bd, 1 );
	}

	/* destroy each backend type */
	LDAP_STAILQ_FOREACH( bi, &backendInfo, bi_next ) {
		if( bi->bi_destroy ) {
			bi->bi_destroy( bi );
		}
	}

	nBackendInfo = 0;
	LDAP_STAILQ_INIT(&backendInfo);

	/* destroy frontend database */
	bd = frontendDB;
	if ( bd ) {
		if ( bd->bd_info->bi_db_destroy ) {
			bd->bd_info->bi_db_destroy( bd, NULL );
		}
		ber_bvarray_free( bd->be_suffix );
		ber_bvarray_free( bd->be_nsuffix );
		if ( !BER_BVISNULL( &bd->be_rootdn ) ) {
			free( bd->be_rootdn.bv_val );
		}
		if ( !BER_BVISNULL( &bd->be_rootndn ) ) {
			free( bd->be_rootndn.bv_val );
		}
		if ( !BER_BVISNULL( &bd->be_rootpw ) ) {
			free( bd->be_rootpw.bv_val );
		}
		acl_destroy( bd->be_acl );
		frontendDB = NULL;
	}

	return 0;
}

BackendInfo* backend_info(const char *type)
{
	BackendInfo *bi;

	/* search for the backend type */
	LDAP_STAILQ_FOREACH(bi,&backendInfo,bi_next) {
		if( strcasecmp(bi->bi_type, type) == 0 ) {
			return bi;
		}
	}

	return NULL;
}

void
backend_db_insert(
	BackendDB *be,
	int idx
)
{
	/* If idx < 0, just add to end of list */
	if ( idx < 0 ) {
		LDAP_STAILQ_INSERT_TAIL(&backendDB, be, be_next);
	} else if ( idx == 0 ) {
		LDAP_STAILQ_INSERT_HEAD(&backendDB, be, be_next);
	} else {
		int i;
		BackendDB *b2;

		b2 = LDAP_STAILQ_FIRST(&backendDB);
		idx--;
		for (i=0; i<idx; i++) {
			b2 = LDAP_STAILQ_NEXT(b2, be_next);
		}
		LDAP_STAILQ_INSERT_AFTER(&backendDB, b2, be, be_next);
	}
}

void
backend_db_move(
	BackendDB *be,
	int idx
)
{
	LDAP_STAILQ_REMOVE(&backendDB, be, BackendDB, be_next);
	backend_db_insert(be, idx);
}

BackendDB *
backend_db_init(
    const char	*type,
	BackendDB *b0,
	int idx,
	ConfigReply *cr)
{
	BackendInfo *bi = backend_info(type);
	BackendDB *be = b0;
	int	rc = 0;

	if( bi == NULL ) {
		fprintf( stderr, "Unrecognized database type (%s)\n", type );
		return NULL;
	}

	/* If be is provided, treat it as private. Otherwise allocate
	 * one and add it to the global list.
	 */
	if ( !be ) {
		be = ch_calloc( 1, sizeof(Backend) );
		/* Just append */
		if ( idx >= nbackends )
			idx = -1;
		nbackends++;
		backend_db_insert( be, idx );
	}

	be->bd_info = bi;
	be->bd_self = be;

	be->be_def_limit = frontendDB->be_def_limit;
	be->be_dfltaccess = frontendDB->be_dfltaccess;

	be->be_restrictops = frontendDB->be_restrictops;
	be->be_requires = frontendDB->be_requires;
	be->be_ssf_set = frontendDB->be_ssf_set;

	ldap_pvt_thread_mutex_init( &be->be_pcl_mutex );

 	/* assign a default depth limit for alias deref */
	be->be_max_deref_depth = SLAPD_DEFAULT_MAXDEREFDEPTH; 

	if ( bi->bi_db_init ) {
		rc = bi->bi_db_init( be, cr );
	}

	if ( rc != 0 ) {
		fprintf( stderr, "database init failed (%s)\n", type );
		/* If we created and linked this be, remove it and free it */
		if ( !b0 ) {
			LDAP_STAILQ_REMOVE(&backendDB, be, BackendDB, be_next);
			ldap_pvt_thread_mutex_destroy( &be->be_pcl_mutex );
			ch_free( be );
			be = NULL;
			nbackends--;
		}
	} else {
		if ( !bi->bi_nDB ) {
			backend_init_controls( bi );
		}
		bi->bi_nDB++;
	}
	return( be );
}

void
be_db_close( void )
{
	BackendDB *be;

	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( be->bd_info->bi_db_close ) {
			be->bd_info->bi_db_close( be, NULL );
		}
	}

	if ( frontendDB->bd_info->bi_db_close ) {
		frontendDB->bd_info->bi_db_close( frontendDB, NULL );
	}

}

Backend *
select_backend(
	struct berval * dn,
	int noSubs )
{
	int		j;
	ber_len_t	len, dnlen = dn->bv_len;
	Backend		*be;

	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( be->be_nsuffix == NULL || SLAP_DBHIDDEN( be ) || SLAP_DBDISABLED( be )) {
			continue;
		}

		for ( j = 0; !BER_BVISNULL( &be->be_nsuffix[j] ); j++ )
		{
			if ( ( SLAP_GLUE_SUBORDINATE( be ) ) && noSubs )
			{
			  	continue;
			}

			len = be->be_nsuffix[j].bv_len;

			if ( len > dnlen ) {
				/* suffix is longer than DN */
				continue;
			}
			
			/*
			 * input DN is normalized, so the separator check
			 * need not look at escaping
			 */
			if ( len && len < dnlen &&
				!DN_SEPARATOR( dn->bv_val[(dnlen-len)-1] ))
			{
				continue;
			}

			if ( strcmp( be->be_nsuffix[j].bv_val,
				&dn->bv_val[dnlen-len] ) == 0 )
			{
				return be;
			}
		}
	}

	return be;
}

int
be_issuffix(
    Backend *be,
    struct berval *bvsuffix )
{
	int	i;

	if ( be->be_nsuffix == NULL ) {
		return 0;
	}

	for ( i = 0; !BER_BVISNULL( &be->be_nsuffix[i] ); i++ ) {
		if ( bvmatch( &be->be_nsuffix[i], bvsuffix ) ) {
			return 1;
		}
	}

	return 0;
}

int
be_issubordinate(
    Backend *be,
    struct berval *bvsubordinate )
{
	int	i;

	if ( be->be_nsuffix == NULL ) {
		return 0;
	}

	for ( i = 0; !BER_BVISNULL( &be->be_nsuffix[i] ); i++ ) {
		if ( dnIsSuffix( bvsubordinate, &be->be_nsuffix[i] ) ) {
			return 1;
		}
	}

	return 0;
}

int
be_isroot_dn( Backend *be, struct berval *ndn )
{
	if ( BER_BVISEMPTY( ndn ) || BER_BVISEMPTY( &be->be_rootndn ) ) {
		return 0;
	}

	return dn_match( &be->be_rootndn, ndn );
}

int
be_slurp_update( Operation *op )
{
	return ( SLAP_SLURP_SHADOW( op->o_bd ) &&
		be_isupdate_dn( op->o_bd, &op->o_ndn ) );
}

int
be_shadow_update( Operation *op )
{
	/* This assumes that all internal ops (connid <= -1000) on a syncrepl
	 * database are syncrepl operations.
	 */
	return ( ( SLAP_SYNC_SHADOW( op->o_bd ) && SLAPD_SYNC_IS_SYNCCONN( op->o_connid ) ) ||
		( SLAP_SHADOW( op->o_bd ) && be_isupdate_dn( op->o_bd, &op->o_ndn ) ) );
}

int
be_isupdate_dn( Backend *be, struct berval *ndn )
{
	if ( BER_BVISEMPTY( ndn ) || BER_BVISEMPTY( &be->be_update_ndn ) ) {
		return 0;
	}

	return dn_match( &be->be_update_ndn, ndn );
}

struct berval *
be_root_dn( Backend *be )
{
	return &be->be_rootdn;
}

int
be_isroot( Operation *op )
{
	return be_isroot_dn( op->o_bd, &op->o_ndn );
}

int
be_isroot_pw( Operation *op )
{
	return be_rootdn_bind( op, NULL ) == LDAP_SUCCESS;
}

/*
 * checks if binding as rootdn
 *
 * return value:
 *	SLAP_CB_CONTINUE		if not the rootdn, or if rootpw is null
 *	LDAP_SUCCESS			if rootdn & rootpw
 *	LDAP_INVALID_CREDENTIALS	if rootdn & !rootpw
 *
 * if rs != NULL
 *	if LDAP_SUCCESS, op->orb_edn is set
 *	if LDAP_INVALID_CREDENTIALS, response is sent to client
 */
int
be_rootdn_bind( Operation *op, SlapReply *rs )
{
	int		rc;
#ifdef SLAPD_SPASSWD
	void	*old_authctx = NULL;
#endif

	assert( op->o_tag == LDAP_REQ_BIND );
	assert( op->orb_method == LDAP_AUTH_SIMPLE );

	if ( !be_isroot_dn( op->o_bd, &op->o_req_ndn ) ) {
		return SLAP_CB_CONTINUE;
	}

	if ( BER_BVISNULL( &op->o_bd->be_rootpw ) ) {
		/* give the database a chance */
		return SLAP_CB_CONTINUE;
	}

	if ( BER_BVISEMPTY( &op->o_bd->be_rootpw ) ) {
		/* rootdn bind explicitly disallowed */
		rc = LDAP_INVALID_CREDENTIALS;
		if ( rs ) {
			goto send_result;
		}

		return rc;
	}

#ifdef SLAPD_SPASSWD
	ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)slap_sasl_bind,
		op->o_conn->c_sasl_authctx, 0, &old_authctx, NULL );
#endif

	rc = lutil_passwd( &op->o_bd->be_rootpw, &op->orb_cred, NULL, NULL );

#ifdef SLAPD_SPASSWD
	ldap_pvt_thread_pool_setkey( op->o_threadctx, (void *)slap_sasl_bind,
		old_authctx, 0, NULL, NULL );
#endif

	rc = ( rc == 0 ? LDAP_SUCCESS : LDAP_INVALID_CREDENTIALS );
	if ( rs ) {
send_result:;
		rs->sr_err = rc;

		Debug( LDAP_DEBUG_TRACE, "%s: rootdn=\"%s\" bind%s\n", 
			op->o_log_prefix, op->o_bd->be_rootdn.bv_val,
			rc == LDAP_SUCCESS ? " succeeded" : " failed" );

		if ( rc == LDAP_SUCCESS ) {
			/* Set to the pretty rootdn */
     			ber_dupbv( &op->orb_edn, &op->o_bd->be_rootdn );

		} else {
			send_ldap_result( op, rs );
		}
	}

	return rc;
}

/* Inlined in proto-slap.h, sans assertions, when !(USE_RS_ASSERT) */
int
(slap_bi_op)(
	BackendInfo *bi,
	slap_operation_t which,
	Operation *op,
	SlapReply *rs )
{
	int rc;
#ifndef slap_bi_op
	void (*rsCheck)( const SlapReply *rs ) =
		which < op_aux_operational ? rs_assert_ready : rs_assert_ok;
#else
#	define rsCheck(rs) ((void) 0)
#endif
	BI_op_func *fn;

	assert( bi != NULL );
	assert( (unsigned) which < (unsigned) op_last );

	fn = (&bi->bi_op_bind)[ which ];

	assert( op != NULL );
	assert( rs != NULL );
	assert( fn != 0 );
	rsCheck( rs );

	rc = fn( op, rs );

#ifndef slap_bi_op
	if ( rc != SLAP_CB_CONTINUE && rc != SLAP_CB_BYPASS ) {
		int err = rs->sr_err;

		if ( 0 )	/* TODO */
		if ( err == LDAP_COMPARE_TRUE || err == LDAP_COMPARE_FALSE ) {
			assert( which == op_compare );
			assert( rc == LDAP_SUCCESS );
		}

		rsCheck = which < op_extended ? rs_assert_done : rs_assert_ok;
		if ( which == op_aux_chk_referrals ) {
			if      ( rc == LDAP_SUCCESS  ) rsCheck = rs_assert_ready;
			else if ( rc == LDAP_REFERRAL ) rsCheck = rs_assert_done;
		} else if ( which == op_bind ) {
			if      ( rc == LDAP_SUCCESS  ) rsCheck = rs_assert_ok;
		}

		/* TODO: Just what is the relation between rc and rs->sr_err? */
		if ( rc != err &&
			(rc != LDAP_SUCCESS ||
			 (err != LDAP_COMPARE_TRUE && err != LDAP_COMPARE_FALSE)) )
		{
			rs->sr_err = rc;
			rsCheck( rs );
			rs->sr_err = err;
		}
	}
	rsCheck( rs );
#endif

	return rc;
}

int
be_entry_release_rw(
	Operation *op,
	Entry *e,
	int rw )
{
	if ( op->o_bd->be_release ) {
		/* free and release entry from backend */
		return op->o_bd->be_release( op, e, rw );
	} else {
		/* free entry */
		entry_free( e );
		return 0;
	}
}

int
backend_unbind( Operation *op, SlapReply *rs )
{
	BackendDB *be;

	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( be->be_unbind ) {
			op->o_bd = be;
			be->be_unbind( op, rs );
		}
	}

	return 0;
}

int
backend_connection_init(
	Connection   *conn )
{
	BackendDB *be;

	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( be->be_connection_init ) {
			be->be_connection_init( be, conn );
		}
	}

	return 0;
}

int
backend_connection_destroy(
	Connection   *conn )
{
	BackendDB *be;

	LDAP_STAILQ_FOREACH( be, &backendDB, be_next ) {
		if ( be->be_connection_destroy ) {
			be->be_connection_destroy( be, conn);
		}
	}

	return 0;
}

int
backend_check_controls(
	Operation *op,
	SlapReply *rs )
{
	LDAPControl **ctrls = op->o_ctrls;
	rs->sr_err = LDAP_SUCCESS;

	if( ctrls ) {
		for( ; *ctrls != NULL ; ctrls++ ) {
			int cid;

			switch ( slap_global_control( op, (*ctrls)->ldctl_oid, &cid ) ) {
			case LDAP_CONTROL_NOT_FOUND:
				/* unrecognized control */ 
				if ( (*ctrls)->ldctl_iscritical ) {
					/* should not be reachable */ 
					Debug( LDAP_DEBUG_ANY, "backend_check_controls: "
						"unrecognized critical control: %s\n",
						(*ctrls)->ldctl_oid, 0, 0 );
					assert( 0 );
				} else {
					Debug( LDAP_DEBUG_TRACE, "backend_check_controls: "
						"unrecognized non-critical control: %s\n",
						(*ctrls)->ldctl_oid, 0, 0 );
				}
				break;

			case LDAP_COMPARE_FALSE:
				if ( !op->o_bd->be_ctrls[cid] && (*ctrls)->ldctl_iscritical ) {
#ifdef SLAP_CONTROL_X_WHATFAILED
					if ( get_whatFailed( op ) ) {
						char *oids[ 2 ];
						oids[ 0 ] = (*ctrls)->ldctl_oid;
						oids[ 1 ] = NULL;
						slap_ctrl_whatFailed_add( op, rs, oids );
					}
#endif
					/* RFC 4511 allows unavailableCriticalExtension to be
					 * returned when the server is unwilling to perform
					 * an operation extended by a recognized critical
					 * control.
					 */
					rs->sr_text = "critical control unavailable in context";
					rs->sr_err = LDAP_UNAVAILABLE_CRITICAL_EXTENSION;
					goto done;
				}
				break;

			case LDAP_COMPARE_TRUE:
				break;

			default:
				/* unreachable */
				Debug( LDAP_DEBUG_ANY,
					"backend_check_controls: unable to check control: %s\n",
					(*ctrls)->ldctl_oid, 0, 0 );
				assert( 0 );

				rs->sr_text = "unable to check control";
				rs->sr_err = LDAP_OTHER;
				goto done;
			}
		}
	}

#if 0 /* temporarily removed */
	/* check should be generalized */
	if( get_relax(op) && !be_isroot(op)) {
		rs->sr_text = "requires manager authorization";
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
	}
#endif

done:;
	return rs->sr_err;
}

int
backend_check_restrictions(
	Operation *op,
	SlapReply *rs,
	struct berval *opdata )
{
	slap_mask_t restrictops;
	slap_mask_t requires;
	slap_mask_t opflag;
	slap_mask_t exopflag = 0;
	slap_ssf_set_t ssfs, *ssf;
	int updateop = 0;
	int starttls = 0;
	int session = 0;

	restrictops = frontendDB->be_restrictops;
	requires = frontendDB->be_requires;
	ssfs = frontendDB->be_ssf_set;
	ssf = &ssfs;

	if ( op->o_bd ) {
		slap_ssf_t *fssf, *bssf;
		int	rc = SLAP_CB_CONTINUE, i;

		if ( op->o_bd->be_chk_controls ) {
			rc = ( *op->o_bd->be_chk_controls )( op, rs );
		}

		if ( rc == SLAP_CB_CONTINUE ) {
			rc = backend_check_controls( op, rs );
		}

		if ( rc != LDAP_SUCCESS ) {
			return rs->sr_err;
		}

		restrictops |= op->o_bd->be_restrictops;
		requires |= op->o_bd->be_requires;
		bssf = &op->o_bd->be_ssf_set.sss_ssf;
		fssf = &ssfs.sss_ssf;
		for ( i=0; i < (int)(sizeof(ssfs)/sizeof(slap_ssf_t)); i++ ) {
			if ( bssf[i] ) fssf[i] = bssf[i];
		}
	}

	switch( op->o_tag ) {
	case LDAP_REQ_ADD:
		opflag = SLAP_RESTRICT_OP_ADD;
		updateop++;
		break;
	case LDAP_REQ_BIND:
		opflag = SLAP_RESTRICT_OP_BIND;
		session++;
		break;
	case LDAP_REQ_COMPARE:
		opflag = SLAP_RESTRICT_OP_COMPARE;
		break;
	case LDAP_REQ_DELETE:
		updateop++;
		opflag = SLAP_RESTRICT_OP_DELETE;
		break;
	case LDAP_REQ_EXTENDED:
		opflag = SLAP_RESTRICT_OP_EXTENDED;

		if( !opdata ) {
			/* treat unspecified as a modify */
			opflag = SLAP_RESTRICT_OP_MODIFY;
			updateop++;
			break;
		}

		if( bvmatch( opdata, &slap_EXOP_START_TLS ) ) {
			session++;
			starttls++;
			exopflag = SLAP_RESTRICT_EXOP_START_TLS;
			break;
		}

		if( bvmatch( opdata, &slap_EXOP_WHOAMI ) ) {
			exopflag = SLAP_RESTRICT_EXOP_WHOAMI;
			break;
		}

		if ( bvmatch( opdata, &slap_EXOP_CANCEL ) ) {
			exopflag = SLAP_RESTRICT_EXOP_CANCEL;
			break;
		}

		if ( bvmatch( opdata, &slap_EXOP_MODIFY_PASSWD ) ) {
			exopflag = SLAP_RESTRICT_EXOP_MODIFY_PASSWD;
			updateop++;
			break;
		}

		/* treat everything else as a modify */
		opflag = SLAP_RESTRICT_OP_MODIFY;
		updateop++;
		break;

	case LDAP_REQ_MODIFY:
		updateop++;
		opflag = SLAP_RESTRICT_OP_MODIFY;
		break;
	case LDAP_REQ_RENAME:
		updateop++;
		opflag = SLAP_RESTRICT_OP_RENAME;
		break;
	case LDAP_REQ_SEARCH:
		opflag = SLAP_RESTRICT_OP_SEARCH;
		break;
	case LDAP_REQ_UNBIND:
		session++;
		opflag = 0;
		break;
	default:
		rs->sr_text = "restrict operations internal error";
		rs->sr_err = LDAP_OTHER;
		return rs->sr_err;
	}

	if ( !starttls ) {
		/* these checks don't apply to StartTLS */

		rs->sr_err = LDAP_CONFIDENTIALITY_REQUIRED;
		if( op->o_transport_ssf < ssf->sss_transport ) {
			rs->sr_text = op->o_transport_ssf
				? "stronger transport confidentiality required"
				: "transport confidentiality required";
			return rs->sr_err;
		}

		if( op->o_tls_ssf < ssf->sss_tls ) {
			rs->sr_text = op->o_tls_ssf
				? "stronger TLS confidentiality required"
				: "TLS confidentiality required";
			return rs->sr_err;
		}


		if( op->o_tag == LDAP_REQ_BIND && opdata == NULL ) {
			/* simple bind specific check */
			if( op->o_ssf < ssf->sss_simple_bind ) {
				rs->sr_text = op->o_ssf
					? "stronger confidentiality required"
					: "confidentiality required";
				return rs->sr_err;
			}
		}

		if( op->o_tag != LDAP_REQ_BIND || opdata == NULL ) {
			/* these checks don't apply to SASL bind */

			if( op->o_sasl_ssf < ssf->sss_sasl ) {
				rs->sr_text = op->o_sasl_ssf
					? "stronger SASL confidentiality required"
					: "SASL confidentiality required";
				return rs->sr_err;
			}

			if( op->o_ssf < ssf->sss_ssf ) {
				rs->sr_text = op->o_ssf
					? "stronger confidentiality required"
					: "confidentiality required";
				return rs->sr_err;
			}
		}

		if( updateop ) {
			if( op->o_transport_ssf < ssf->sss_update_transport ) {
				rs->sr_text = op->o_transport_ssf
					? "stronger transport confidentiality required for update"
					: "transport confidentiality required for update";
				return rs->sr_err;
			}

			if( op->o_tls_ssf < ssf->sss_update_tls ) {
				rs->sr_text = op->o_tls_ssf
					? "stronger TLS confidentiality required for update"
					: "TLS confidentiality required for update";
				return rs->sr_err;
			}

			if( op->o_sasl_ssf < ssf->sss_update_sasl ) {
				rs->sr_text = op->o_sasl_ssf
					? "stronger SASL confidentiality required for update"
					: "SASL confidentiality required for update";
				return rs->sr_err;
			}

			if( op->o_ssf < ssf->sss_update_ssf ) {
				rs->sr_text = op->o_ssf
					? "stronger confidentiality required for update"
					: "confidentiality required for update";
				return rs->sr_err;
			}

			if( !( global_allows & SLAP_ALLOW_UPDATE_ANON ) &&
				BER_BVISEMPTY( &op->o_ndn ) )
			{
				rs->sr_text = "modifications require authentication";
				rs->sr_err = LDAP_STRONG_AUTH_REQUIRED;
				return rs->sr_err;
			}

#ifdef SLAP_X_LISTENER_MOD
			if ( op->o_conn->c_listener &&
				! ( op->o_conn->c_listener->sl_perms & ( !BER_BVISEMPTY( &op->o_ndn )
					? (S_IWUSR|S_IWOTH) : S_IWOTH ) ) )
			{
				/* no "w" mode means readonly */
				rs->sr_text = "modifications not allowed on this listener";
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				return rs->sr_err;
			}
#endif /* SLAP_X_LISTENER_MOD */
		}
	}

	if ( !session ) {
		/* these checks don't apply to Bind, StartTLS, or Unbind */

		if( requires & SLAP_REQUIRE_STRONG ) {
			/* should check mechanism */
			if( ( op->o_transport_ssf < ssf->sss_transport
				&& op->o_authtype == LDAP_AUTH_SIMPLE )
				|| BER_BVISEMPTY( &op->o_dn ) )
			{
				rs->sr_text = "strong(er) authentication required";
				rs->sr_err = LDAP_STRONG_AUTH_REQUIRED;
				return rs->sr_err;
			}
		}

		if( requires & SLAP_REQUIRE_SASL ) {
			if( op->o_authtype != LDAP_AUTH_SASL || BER_BVISEMPTY( &op->o_dn ) ) {
				rs->sr_text = "SASL authentication required";
				rs->sr_err = LDAP_STRONG_AUTH_REQUIRED;
				return rs->sr_err;
			}
		}
			
		if( requires & SLAP_REQUIRE_AUTHC ) {
			if( BER_BVISEMPTY( &op->o_dn ) ) {
				rs->sr_text = "authentication required";
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				return rs->sr_err;
			}
		}

		if( requires & SLAP_REQUIRE_BIND ) {
			int version;
			ldap_pvt_thread_mutex_lock( &op->o_conn->c_mutex );
			version = op->o_conn->c_protocol;
			ldap_pvt_thread_mutex_unlock( &op->o_conn->c_mutex );

			if( !version ) {
				/* no bind has occurred */
				rs->sr_text = "BIND required";
				rs->sr_err = LDAP_OPERATIONS_ERROR;
				return rs->sr_err;
			}
		}

		if( requires & SLAP_REQUIRE_LDAP_V3 ) {
			if( op->o_protocol < LDAP_VERSION3 ) {
				/* no bind has occurred */
				rs->sr_text = "operation restricted to LDAPv3 clients";
				rs->sr_err = LDAP_OPERATIONS_ERROR;
				return rs->sr_err;
			}
		}

#ifdef SLAP_X_LISTENER_MOD
		if ( !starttls && BER_BVISEMPTY( &op->o_dn ) ) {
			if ( op->o_conn->c_listener &&
				!( op->o_conn->c_listener->sl_perms & S_IXOTH ))
		{
				/* no "x" mode means bind required */
				rs->sr_text = "bind required on this listener";
				rs->sr_err = LDAP_STRONG_AUTH_REQUIRED;
				return rs->sr_err;
			}
		}

		if ( !starttls && !updateop ) {
			if ( op->o_conn->c_listener &&
				!( op->o_conn->c_listener->sl_perms &
					( !BER_BVISEMPTY( &op->o_dn )
						? (S_IRUSR|S_IROTH) : S_IROTH )))
			{
				/* no "r" mode means no read */
				rs->sr_text = "read not allowed on this listener";
				rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
				return rs->sr_err;
			}
		}
#endif /* SLAP_X_LISTENER_MOD */

	}

	if( ( restrictops & opflag )
			|| ( exopflag && ( restrictops & exopflag ) )
			|| (( restrictops & SLAP_RESTRICT_READONLY ) && updateop )) {
		if( ( restrictops & SLAP_RESTRICT_OP_MASK) == SLAP_RESTRICT_OP_READS ) {
			rs->sr_text = "read operations restricted";
		} else if ( restrictops & exopflag ) {
			rs->sr_text = "extended operation restricted";
		} else {
			rs->sr_text = "operation restricted";
		}
		rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
		return rs->sr_err;
 	}

	rs->sr_err = LDAP_SUCCESS;
	return rs->sr_err;
}

int backend_check_referrals( Operation *op, SlapReply *rs )
{
	rs->sr_err = LDAP_SUCCESS;

	if( op->o_bd->be_chk_referrals ) {
		rs->sr_err = op->o_bd->be_chk_referrals( op, rs );

		if( rs->sr_err != LDAP_SUCCESS && rs->sr_err != LDAP_REFERRAL ) {
			send_ldap_result( op, rs );
		}
	}

	return rs->sr_err;
}

int
be_entry_get_rw(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **e )
{
	*e = NULL;

	if ( op->o_bd == NULL ) {
		return LDAP_NO_SUCH_OBJECT;
	}

	if ( op->o_bd->be_fetch ) {
		return op->o_bd->be_fetch( op, ndn, oc, at, rw, e );
	}

	return LDAP_UNWILLING_TO_PERFORM;
}

int 
fe_acl_group(
	Operation *op,
	Entry	*target,
	struct berval *gr_ndn,
	struct berval *op_ndn,
	ObjectClass *group_oc,
	AttributeDescription *group_at )
{
	Entry *e;
	void *o_priv = op->o_private, *e_priv = NULL;
	Attribute *a;
	int rc;
	GroupAssertion *g;
	Backend *be = op->o_bd;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH(oex, &op->o_extra, oe_next) {
		if ( oex->oe_key == (void *)backend_group )
			break;
	}

	if ( oex && ((OpExtraDB *)oex)->oe_db )
		op->o_bd = ((OpExtraDB *)oex)->oe_db;

	if ( !op->o_bd || !SLAP_DBHIDDEN( op->o_bd ))
		op->o_bd = select_backend( gr_ndn, 0 );

	for ( g = op->o_groups; g; g = g->ga_next ) {
		if ( g->ga_be != op->o_bd || g->ga_oc != group_oc ||
			g->ga_at != group_at || g->ga_len != gr_ndn->bv_len )
		{
			continue;
		}
		if ( strcmp( g->ga_ndn, gr_ndn->bv_val ) == 0 ) {
			break;
		}
	}

	if ( g ) {
		rc = g->ga_res;
		goto done;
	}

	if ( target && dn_match( &target->e_nname, gr_ndn ) ) {
		e = target;
		rc = 0;

	} else {
		op->o_private = NULL;
		rc = be_entry_get_rw( op, gr_ndn, group_oc, group_at, 0, &e );
		e_priv = op->o_private;
		op->o_private = o_priv;
	}

	if ( e ) {
		a = attr_find( e->e_attrs, group_at );
		if ( a ) {
			/* If the attribute is a subtype of labeledURI,
			 * treat this as a dynamic group ala groupOfURLs
			 */
			if ( is_at_subtype( group_at->ad_type,
				slap_schema.si_ad_labeledURI->ad_type ) )
			{
				int i;
				LDAPURLDesc *ludp;
				struct berval bv, nbase;
				Filter *filter;
				Entry *user = NULL;
				void *user_priv = NULL;
				Backend *b2 = op->o_bd;

				if ( target && dn_match( &target->e_nname, op_ndn ) ) {
					user = target;
				}
				
				rc = LDAP_COMPARE_FALSE;
				for ( i = 0; !BER_BVISNULL( &a->a_vals[i] ); i++ ) {
					if ( ldap_url_parse( a->a_vals[i].bv_val, &ludp ) !=
						LDAP_URL_SUCCESS )
					{
						continue;
					}

					BER_BVZERO( &nbase );

					/* host, attrs and extensions parts must be empty */
					if ( ( ludp->lud_host && *ludp->lud_host )
						|| ludp->lud_attrs
						|| ludp->lud_exts )
					{
						goto loopit;
					}

					ber_str2bv( ludp->lud_dn, 0, 0, &bv );
					if ( dnNormalize( 0, NULL, NULL, &bv, &nbase,
						op->o_tmpmemctx ) != LDAP_SUCCESS )
					{
						goto loopit;
					}

					switch ( ludp->lud_scope ) {
					case LDAP_SCOPE_BASE:
						if ( !dn_match( &nbase, op_ndn ) ) {
							goto loopit;
						}
						break;
					case LDAP_SCOPE_ONELEVEL:
						dnParent( op_ndn, &bv );
						if ( !dn_match( &nbase, &bv ) ) {
							goto loopit;
						}
						break;
					case LDAP_SCOPE_SUBTREE:
						if ( !dnIsSuffix( op_ndn, &nbase ) ) {
							goto loopit;
						}
						break;
					case LDAP_SCOPE_SUBORDINATE:
						if ( dn_match( &nbase, op_ndn ) ||
							!dnIsSuffix( op_ndn, &nbase ) )
						{
							goto loopit;
						}
					}

					/* NOTE: this could be NULL
					 * if no filter is provided,
					 * or if filter parsing fails.
					 * In the latter case,
					 * we should give up. */
					if ( ludp->lud_filter != NULL && ludp->lud_filter != '\0') {
						filter = str2filter_x( op, ludp->lud_filter );
						if ( filter == NULL ) {
							/* give up... */
							rc = LDAP_OTHER;
							goto loopit;
						}

						/* only get user if required
						 * and not available yet */
						if ( user == NULL ) {	
							int rc2;

							op->o_bd = select_backend( op_ndn, 0 );
							op->o_private = NULL;
							rc2 = be_entry_get_rw( op, op_ndn, NULL, NULL, 0, &user );
							user_priv = op->o_private;
							op->o_private = o_priv;
							if ( rc2 != 0 ) {
								/* give up... */
								rc = LDAP_OTHER;
								goto loopit;
							}
						}

						if ( test_filter( NULL, user, filter ) ==
							LDAP_COMPARE_TRUE )
						{
							rc = 0;
						}
						filter_free_x( op, filter, 1 );
					}
loopit:
					ldap_free_urldesc( ludp );
					if ( !BER_BVISNULL( &nbase ) ) {
						op->o_tmpfree( nbase.bv_val, op->o_tmpmemctx );
					}
					if ( rc != LDAP_COMPARE_FALSE ) {
						break;
					}
				}

				if ( user != NULL && user != target ) {
					op->o_private = user_priv;
					be_entry_release_r( op, user );
					op->o_private = o_priv;
				}
				op->o_bd = b2;

			} else {
				rc = attr_valfind( a,
					SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
					op_ndn, NULL, op->o_tmpmemctx );
				if ( rc == LDAP_NO_SUCH_ATTRIBUTE ) {
					rc = LDAP_COMPARE_FALSE;
				}
			}

		} else {
			rc = LDAP_NO_SUCH_ATTRIBUTE;
		}

		if ( e != target ) {
			op->o_private = e_priv;
			be_entry_release_r( op, e );
			op->o_private = o_priv;
		}

	} else {
		rc = LDAP_NO_SUCH_OBJECT;
	}

	if ( op->o_tag != LDAP_REQ_BIND && !op->o_do_not_cache ) {
		g = op->o_tmpalloc( sizeof( GroupAssertion ) + gr_ndn->bv_len,
			op->o_tmpmemctx );
		g->ga_be = op->o_bd;
		g->ga_oc = group_oc;
		g->ga_at = group_at;
		g->ga_res = rc;
		g->ga_len = gr_ndn->bv_len;
		strcpy( g->ga_ndn, gr_ndn->bv_val );
		g->ga_next = op->o_groups;
		op->o_groups = g;
	}

done:
	op->o_bd = be;
	return rc;
}

int 
backend_group(
	Operation *op,
	Entry	*target,
	struct berval *gr_ndn,
	struct berval *op_ndn,
	ObjectClass *group_oc,
	AttributeDescription *group_at )
{
	int			rc;
	BackendDB *be_orig;
	OpExtraDB	oex;

	if ( op->o_abandon ) {
		return SLAPD_ABANDON;
	}

	oex.oe_db = op->o_bd;
	oex.oe.oe_key = (void *)backend_group;
	LDAP_SLIST_INSERT_HEAD(&op->o_extra, &oex.oe, oe_next);

	be_orig = op->o_bd;
	op->o_bd = frontendDB;
	rc = frontendDB->be_group( op, target, gr_ndn,
		op_ndn, group_oc, group_at );
	op->o_bd = be_orig;
	LDAP_SLIST_REMOVE(&op->o_extra, &oex.oe, OpExtra, oe_next);

	return rc;
}

int 
fe_acl_attribute(
	Operation *op,
	Entry	*target,
	struct berval	*edn,
	AttributeDescription *entry_at,
	BerVarray *vals,
	slap_access_t access )
{
	Entry			*e = NULL;
	void			*o_priv = op->o_private, *e_priv = NULL;
	Attribute		*a = NULL;
	int			freeattr = 0, i, j, rc = LDAP_SUCCESS;
	AccessControlState	acl_state = ACL_STATE_INIT;
	Backend			*be = op->o_bd;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH(oex, &op->o_extra, oe_next) {
		if ( oex->oe_key == (void *)backend_attribute )
			break;
	}

	if ( oex && ((OpExtraDB *)oex)->oe_db )
		op->o_bd = ((OpExtraDB *)oex)->oe_db;

	if ( !op->o_bd || !SLAP_DBHIDDEN( op->o_bd ))
		op->o_bd = select_backend( edn, 0 );

	if ( target && dn_match( &target->e_nname, edn ) ) {
		e = target;

	} else {
		op->o_private = NULL;
		rc = be_entry_get_rw( op, edn, NULL, entry_at, 0, &e );
		e_priv = op->o_private;
		op->o_private = o_priv;
	} 

	if ( e ) {
		if ( entry_at == slap_schema.si_ad_entry || entry_at == slap_schema.si_ad_children ) {
			assert( vals == NULL );

			rc = LDAP_SUCCESS;
			if ( op->o_conn && access > ACL_NONE &&
				access_allowed( op, e, entry_at, NULL,
						access, &acl_state ) == 0 )
			{
				rc = LDAP_INSUFFICIENT_ACCESS;
			}
			goto freeit;
		}

		a = attr_find( e->e_attrs, entry_at );
		if ( a == NULL ) {
			SlapReply	rs = { REP_SEARCH };
			AttributeName	anlist[ 2 ];

			anlist[ 0 ].an_name = entry_at->ad_cname;
			anlist[ 0 ].an_desc = entry_at;
			BER_BVZERO( &anlist[ 1 ].an_name );
			rs.sr_attrs = anlist;
			
 			/* NOTE: backend_operational() is also called
 			 * when returning results, so it's supposed
 			 * to do no harm to entries */
 			rs.sr_entry = e;
  			rc = backend_operational( op, &rs );

			if ( rc == LDAP_SUCCESS ) {
				if ( rs.sr_operational_attrs ) {
					freeattr = 1;
					a = rs.sr_operational_attrs;

				} else {
					rc = LDAP_NO_SUCH_ATTRIBUTE;
				}
			}
		}

		if ( a ) {
			BerVarray v;

			if ( op->o_conn && access > ACL_NONE &&
				access_allowed( op, e, entry_at, NULL,
						access, &acl_state ) == 0 )
			{
				rc = LDAP_INSUFFICIENT_ACCESS;
				goto freeit;
			}

			i = a->a_numvals;
			v = op->o_tmpalloc( sizeof(struct berval) * ( i + 1 ),
				op->o_tmpmemctx );
			for ( i = 0, j = 0; !BER_BVISNULL( &a->a_vals[i] ); i++ )
			{
				if ( op->o_conn && access > ACL_NONE && 
					access_allowed( op, e, entry_at,
							&a->a_nvals[i],
							access,
							&acl_state ) == 0 )
				{
					continue;
				}
				ber_dupbv_x( &v[j], &a->a_nvals[i],
						op->o_tmpmemctx );
				if ( !BER_BVISNULL( &v[j] ) ) {
					j++;
				}
			}
			if ( j == 0 ) {
				op->o_tmpfree( v, op->o_tmpmemctx );
				*vals = NULL;
				rc = LDAP_INSUFFICIENT_ACCESS;

			} else {
				BER_BVZERO( &v[j] );
				*vals = v;
				rc = LDAP_SUCCESS;
			}
		}
freeit:		if ( e != target ) {
			op->o_private = e_priv;
			be_entry_release_r( op, e );
			op->o_private = o_priv;
		}
		if ( freeattr ) {
			attr_free( a );
		}
	}

	op->o_bd = be;
	return rc;
}

int 
backend_attribute(
	Operation *op,
	Entry	*target,
	struct berval	*edn,
	AttributeDescription *entry_at,
	BerVarray *vals,
	slap_access_t access )
{
	int			rc;
	BackendDB *be_orig;
	OpExtraDB	oex;

	oex.oe_db = op->o_bd;
	oex.oe.oe_key = (void *)backend_attribute;
	LDAP_SLIST_INSERT_HEAD(&op->o_extra, &oex.oe, oe_next);

	be_orig = op->o_bd;
	op->o_bd = frontendDB;
	rc = frontendDB->be_attribute( op, target, edn,
		entry_at, vals, access );
	op->o_bd = be_orig;
	LDAP_SLIST_REMOVE(&op->o_extra, &oex.oe, OpExtra, oe_next);

	return rc;
}

int 
backend_access(
	Operation		*op,
	Entry			*target,
	struct berval		*edn,
	AttributeDescription	*entry_at,
	struct berval		*nval,
	slap_access_t		access,
	slap_mask_t		*mask )
{
	Entry		*e = NULL;
	void		*o_priv, *e_priv = NULL;
	int		rc = LDAP_INSUFFICIENT_ACCESS;
	Backend		*be;

	/* pedantic */
	assert( op != NULL );
	assert( op->o_conn != NULL );
	assert( edn != NULL );
	assert( access > ACL_NONE );

	be = op->o_bd;
	o_priv = op->o_private;

	if ( !op->o_bd ) {
		op->o_bd = select_backend( edn, 0 );
	}

	if ( target && dn_match( &target->e_nname, edn ) ) {
		e = target;

	} else {
		op->o_private = NULL;
		rc = be_entry_get_rw( op, edn, NULL, entry_at, 0, &e );
		e_priv = op->o_private;
		op->o_private = o_priv;
	} 

	if ( e ) {
		Attribute	*a = NULL;
		int		freeattr = 0;

		if ( entry_at == NULL ) {
			entry_at = slap_schema.si_ad_entry;
		}

		if ( entry_at == slap_schema.si_ad_entry || entry_at == slap_schema.si_ad_children )
		{
			if ( access_allowed_mask( op, e, entry_at,
					NULL, access, NULL, mask ) == 0 )
			{
				rc = LDAP_INSUFFICIENT_ACCESS;

			} else {
				rc = LDAP_SUCCESS;
			}

		} else {
			a = attr_find( e->e_attrs, entry_at );
			if ( a == NULL ) {
				SlapReply	rs = { REP_SEARCH };
				AttributeName	anlist[ 2 ];

				anlist[ 0 ].an_name = entry_at->ad_cname;
				anlist[ 0 ].an_desc = entry_at;
				BER_BVZERO( &anlist[ 1 ].an_name );
				rs.sr_attrs = anlist;
			
				rs.sr_attr_flags = slap_attr_flags( rs.sr_attrs );

				/* NOTE: backend_operational() is also called
				 * when returning results, so it's supposed
				 * to do no harm to entries */
				rs.sr_entry = e;
				rc = backend_operational( op, &rs );

				if ( rc == LDAP_SUCCESS ) {
					if ( rs.sr_operational_attrs ) {
						freeattr = 1;
						a = rs.sr_operational_attrs;

					} else {
						rc = LDAP_NO_SUCH_OBJECT;
					}
				}
			}

			if ( a ) {
				if ( access_allowed_mask( op, e, entry_at,
						nval, access, NULL, mask ) == 0 )
				{
					rc = LDAP_INSUFFICIENT_ACCESS;
					goto freeit;
				}
				rc = LDAP_SUCCESS;
			}
		}
freeit:		if ( e != target ) {
			op->o_private = e_priv;
			be_entry_release_r( op, e );
			op->o_private = o_priv;
		}
		if ( freeattr ) {
			attr_free( a );
		}
	}

	op->o_bd = be;
	return rc;
}

int
fe_aux_operational(
	Operation *op,
	SlapReply *rs )
{
	Attribute		**ap;
	int			rc = LDAP_SUCCESS;
	BackendDB		*be_orig = op->o_bd;
	OpExtra		*oex;

	LDAP_SLIST_FOREACH(oex, &op->o_extra, oe_next) {
		if ( oex->oe_key == (void *)backend_operational )
			break;
	}

	for ( ap = &rs->sr_operational_attrs; *ap; ap = &(*ap)->a_next )
		/* just count them */ ;

	/*
	 * If operational attributes (allegedly) are required, 
	 * and the backend supports specific operational attributes, 
	 * add them to the attribute list
	 */
	if ( !( rs->sr_flags & REP_NO_ENTRYDN )
		&& ( SLAP_OPATTRS( rs->sr_attr_flags ) || ( rs->sr_attrs &&
		ad_inlist( slap_schema.si_ad_entryDN, rs->sr_attrs ) ) ) )
	{
		*ap = slap_operational_entryDN( rs->sr_entry );
		ap = &(*ap)->a_next;
	}

	if ( !( rs->sr_flags & REP_NO_SUBSCHEMA)
		&& ( SLAP_OPATTRS( rs->sr_attr_flags ) || ( rs->sr_attrs &&
		ad_inlist( slap_schema.si_ad_subschemaSubentry, rs->sr_attrs ) ) ) )
	{
		*ap = slap_operational_subschemaSubentry( op->o_bd );
		ap = &(*ap)->a_next;
	}

	/* Let the overlays have a chance at this */
	if ( oex && ((OpExtraDB *)oex)->oe_db )
		op->o_bd = ((OpExtraDB *)oex)->oe_db;

	if ( !op->o_bd || !SLAP_DBHIDDEN( op->o_bd ))
		op->o_bd = select_backend( &op->o_req_ndn, 0 );

	if ( op->o_bd != NULL && !be_match( op->o_bd, frontendDB ) &&
		( SLAP_OPATTRS( rs->sr_attr_flags ) || rs->sr_attrs ) &&
		op->o_bd->be_operational != NULL )
	{
		rc = op->o_bd->be_operational( op, rs );
	}
	op->o_bd = be_orig;

	return rc;
}

int backend_operational( Operation *op, SlapReply *rs )
{
	int rc;
	BackendDB *be_orig;
	OpExtraDB	oex;

	oex.oe_db = op->o_bd;
	oex.oe.oe_key = (void *)backend_operational;
	LDAP_SLIST_INSERT_HEAD(&op->o_extra, &oex.oe, oe_next);

	/* Moved this into the frontend so global overlays are called */

	be_orig = op->o_bd;
	op->o_bd = frontendDB;
	rc = frontendDB->be_operational( op, rs );
	op->o_bd = be_orig;
	LDAP_SLIST_REMOVE(&op->o_extra, &oex.oe, OpExtra, oe_next);

	return rc;
}

/* helper that calls the bi_tool_entry_first_x() variant with default args;
 * use to initialize a backend's bi_tool_entry_first() when appropriate
 */
ID
backend_tool_entry_first( BackendDB *be )
{
	return be->bd_info->bi_tool_entry_first_x( be,
		NULL, LDAP_SCOPE_DEFAULT, NULL );
}
