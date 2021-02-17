/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

int
meta_back_open(
	BackendInfo	*bi )
{
	/* FIXME: need to remove the pagedResults, and likely more... */
	bi->bi_controls = slap_known_controls;

	return 0;
}

int
meta_back_initialize(
	BackendInfo	*bi )
{
	bi->bi_flags =
#if 0
	/* this is not (yet) set essentially because back-meta does not
	 * directly support extended operations... */
#ifdef LDAP_DYNAMIC_OBJECTS
		/* this is set because all the support a proxy has to provide
		 * is the capability to forward the refresh exop, and to
		 * pass thru entries that contain the dynamicObject class
		 * and the entryTtl attribute */
		SLAP_BFLAG_DYNAMIC |
#endif /* LDAP_DYNAMIC_OBJECTS */
#endif

		/* back-meta recognizes RFC4525 increment;
		 * let the remote server complain, if needed (ITS#5912) */
		SLAP_BFLAG_INCREMENT;

	bi->bi_open = meta_back_open;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = meta_back_db_init;
	bi->bi_db_config = config_generic_wrapper;
	bi->bi_db_open = meta_back_db_open;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = meta_back_db_destroy;

	bi->bi_op_bind = meta_back_bind;
	bi->bi_op_unbind = 0;
	bi->bi_op_search = meta_back_search;
	bi->bi_op_compare = meta_back_compare;
	bi->bi_op_modify = meta_back_modify;
	bi->bi_op_modrdn = meta_back_modrdn;
	bi->bi_op_add = meta_back_add;
	bi->bi_op_delete = meta_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = meta_back_conn_destroy;

	return meta_back_init_cf( bi );
}

int
meta_back_db_init(
	Backend		*be,
	ConfigReply	*cr)
{
	metainfo_t	*mi;
	int		i;
	BackendInfo	*bi;

	bi = backend_info( "ldap" );
	if ( !bi || !bi->bi_extra ) {
		Debug( LDAP_DEBUG_ANY,
			"meta_back_db_init: needs back-ldap\n",
			0, 0, 0 );
		return 1;
	}

	mi = ch_calloc( 1, sizeof( metainfo_t ) );
	if ( mi == NULL ) {
 		return -1;
 	}

	/* set default flags */
	mi->mi_flags =
		META_BACK_F_DEFER_ROOTDN_BIND
		| META_BACK_F_PROXYAUTHZ_ALWAYS
		| META_BACK_F_PROXYAUTHZ_ANON
		| META_BACK_F_PROXYAUTHZ_NOANON;

	/*
	 * At present the default is no default target;
	 * this may change
	 */
	mi->mi_defaulttarget = META_DEFAULT_TARGET_NONE;
	mi->mi_bind_timeout.tv_sec = 0;
	mi->mi_bind_timeout.tv_usec = META_BIND_TIMEOUT;

	mi->mi_rebind_f = meta_back_default_rebind;
	mi->mi_urllist_f = meta_back_default_urllist;

	ldap_pvt_thread_mutex_init( &mi->mi_conninfo.lai_mutex );
	ldap_pvt_thread_mutex_init( &mi->mi_cache.mutex );

	/* safe default */
	mi->mi_nretries = META_RETRY_DEFAULT;
	mi->mi_version = LDAP_VERSION3;

	for ( i = LDAP_BACK_PCONN_FIRST; i < LDAP_BACK_PCONN_LAST; i++ ) {
		mi->mi_conn_priv[ i ].mic_num = 0;
		LDAP_TAILQ_INIT( &mi->mi_conn_priv[ i ].mic_priv );
	}
	mi->mi_conn_priv_max = LDAP_BACK_CONN_PRIV_DEFAULT;
	
	mi->mi_ldap_extra = (ldap_extra_t *)bi->bi_extra;

	be->be_private = mi;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return 0;
}

int
meta_target_finish(
	metainfo_t *mi,
	metatarget_t *mt,
	const char *log,
	char *msg,
	size_t msize
)
{
	slap_bindconf	sb = { BER_BVNULL };
	struct berval mapped;
	int rc;

	ber_str2bv( mt->mt_uri, 0, 0, &sb.sb_uri );
	sb.sb_version = mt->mt_version;
	sb.sb_method = LDAP_AUTH_SIMPLE;
	BER_BVSTR( &sb.sb_binddn, "" );

	if ( META_BACK_TGT_T_F_DISCOVER( mt ) ) {
		rc = slap_discover_feature( &sb,
				slap_schema.si_ad_supportedFeatures->ad_cname.bv_val,
				LDAP_FEATURE_ABSOLUTE_FILTERS );
		if ( rc == LDAP_COMPARE_TRUE ) {
			mt->mt_flags |= LDAP_BACK_F_T_F;
		}
	}

	if ( META_BACK_TGT_CANCEL_DISCOVER( mt ) ) {
		rc = slap_discover_feature( &sb,
				slap_schema.si_ad_supportedExtension->ad_cname.bv_val,
				LDAP_EXOP_CANCEL );
		if ( rc == LDAP_COMPARE_TRUE ) {
			mt->mt_flags |= LDAP_BACK_F_CANCEL_EXOP;
		}
	}

	if ( !( mt->mt_idassert_flags & LDAP_BACK_AUTH_OVERRIDE )
		|| mt->mt_idassert_authz != NULL )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_ALWAYS;
	}

	if ( ( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL )
		&& !( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) )
	{
		snprintf( msg, msize,
			"%s: inconsistent idassert configuration "
			"(likely authz=\"*\" used with \"non-prescriptive\" flag)",
			log );
		Debug( LDAP_DEBUG_ANY, "%s (target %s)\n",
			msg, mt->mt_uri, 0 );
		return 1;
	}

	if ( !( mt->mt_idassert_flags & LDAP_BACK_AUTH_AUTHZ_ALL ) )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_ANON;
	}

	if ( ( mt->mt_idassert_flags & LDAP_BACK_AUTH_PRESCRIPTIVE ) )
	{
		mi->mi_flags &= ~META_BACK_F_PROXYAUTHZ_NOANON;
	}

	BER_BVZERO( &mapped );
	ldap_back_map( &mt->mt_rwmap.rwm_at,
		&slap_schema.si_ad_entryDN->ad_cname, &mapped,
		BACKLDAP_REMAP );
	if ( BER_BVISNULL( &mapped ) || mapped.bv_val[0] == '\0' ) {
		mt->mt_rep_flags |= REP_NO_ENTRYDN;
	}

	BER_BVZERO( &mapped );
	ldap_back_map( &mt->mt_rwmap.rwm_at,
		&slap_schema.si_ad_subschemaSubentry->ad_cname, &mapped,
		BACKLDAP_REMAP );
	if ( BER_BVISNULL( &mapped ) || mapped.bv_val[0] == '\0' ) {
		mt->mt_rep_flags |= REP_NO_SUBSCHEMA;
	}

	return 0;
}

int
meta_back_db_open(
	Backend		*be,
	ConfigReply	*cr )
{
	metainfo_t	*mi = (metainfo_t *)be->be_private;
	char msg[SLAP_TEXT_BUFLEN];

	int		i, rc;

	if ( mi->mi_ntargets == 0 ) {
		/* Dynamically added, nothing to check here until
		 * some targets get added
		 */
		if ( slapMode & SLAP_SERVER_RUNNING )
			return 0;

		Debug( LDAP_DEBUG_ANY,
			"meta_back_db_open: no targets defined\n",
			0, 0, 0 );
		return 1;
	}

	for ( i = 0; i < mi->mi_ntargets; i++ ) {
		metatarget_t	*mt = mi->mi_targets[ i ];

		if ( meta_target_finish( mi, mt,
			"meta_back_db_open", msg, sizeof( msg )))
			return 1;
	}

	return 0;
}

/*
 * meta_back_conn_free()
 *
 * actually frees a connection; the reference count must be 0,
 * and it must not (or no longer) be in the cache.
 */
void
meta_back_conn_free( 
	void 		*v_mc )
{
	metaconn_t		*mc = v_mc;
	int			ntargets;

	assert( mc != NULL );
	assert( mc->mc_refcnt == 0 );

	/* at least one must be present... */
	ntargets = mc->mc_info->mi_ntargets;
	assert( ntargets > 0 );

	for ( ; ntargets--; ) {
		(void)meta_clear_one_candidate( NULL, mc, ntargets );
	}

	if ( !BER_BVISNULL( &mc->mc_local_ndn ) ) {
		free( mc->mc_local_ndn.bv_val );
	}

	free( mc );
}

static void
mapping_free(
	void		*v_mapping )
{
	struct ldapmapping *mapping = v_mapping;
	ch_free( mapping->src.bv_val );
	ch_free( mapping->dst.bv_val );
	ch_free( mapping );
}

static void
mapping_dst_free(
	void		*v_mapping )
{
	struct ldapmapping *mapping = v_mapping;

	if ( BER_BVISEMPTY( &mapping->dst ) ) {
		mapping_free( &mapping[ -1 ] );
	}
}

void
meta_back_map_free( struct ldapmap *lm )
{
	avl_free( lm->remap, mapping_dst_free );
	avl_free( lm->map, mapping_free );
	lm->remap = NULL;
	lm->map = NULL;
}

static void
target_free(
	metatarget_t	*mt )
{
	if ( mt->mt_uri ) {
		free( mt->mt_uri );
		ldap_pvt_thread_mutex_destroy( &mt->mt_uri_mutex );
	}
	if ( mt->mt_subtree ) {
		meta_subtree_destroy( mt->mt_subtree );
		mt->mt_subtree = NULL;
	}
	if ( mt->mt_filter ) {
		meta_filter_destroy( mt->mt_filter );
		mt->mt_filter = NULL;
	}
	if ( !BER_BVISNULL( &mt->mt_psuffix ) ) {
		free( mt->mt_psuffix.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_nsuffix ) ) {
		free( mt->mt_nsuffix.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_binddn ) ) {
		free( mt->mt_binddn.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_bindpw ) ) {
		free( mt->mt_bindpw.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authcID ) ) {
		ch_free( mt->mt_idassert_authcID.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authcDN ) ) {
		ch_free( mt->mt_idassert_authcDN.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_passwd ) ) {
		ch_free( mt->mt_idassert_passwd.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_authzID ) ) {
		ch_free( mt->mt_idassert_authzID.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_sasl_mech ) ) {
		ch_free( mt->mt_idassert_sasl_mech.bv_val );
	}
	if ( !BER_BVISNULL( &mt->mt_idassert_sasl_realm ) ) {
		ch_free( mt->mt_idassert_sasl_realm.bv_val );
	}
	if ( mt->mt_idassert_authz != NULL ) {
		ber_bvarray_free( mt->mt_idassert_authz );
	}
	if ( mt->mt_rwmap.rwm_rw ) {
		rewrite_info_delete( &mt->mt_rwmap.rwm_rw );
		if ( mt->mt_rwmap.rwm_bva_rewrite )
			ber_bvarray_free( mt->mt_rwmap.rwm_bva_rewrite );
	}
	meta_back_map_free( &mt->mt_rwmap.rwm_oc );
	meta_back_map_free( &mt->mt_rwmap.rwm_at );
	ber_bvarray_free( mt->mt_rwmap.rwm_bva_map );

	free( mt );
}

int
meta_back_db_destroy(
	Backend		*be,
	ConfigReply	*cr )
{
	metainfo_t	*mi;

	if ( be->be_private ) {
		int i;

		mi = ( metainfo_t * )be->be_private;

		/*
		 * Destroy the connection tree
		 */
		ldap_pvt_thread_mutex_lock( &mi->mi_conninfo.lai_mutex );

		if ( mi->mi_conninfo.lai_tree ) {
			avl_free( mi->mi_conninfo.lai_tree, meta_back_conn_free );
		}
		for ( i = LDAP_BACK_PCONN_FIRST; i < LDAP_BACK_PCONN_LAST; i++ ) {
			while ( !LDAP_TAILQ_EMPTY( &mi->mi_conn_priv[ i ].mic_priv ) ) {
				metaconn_t	*mc = LDAP_TAILQ_FIRST( &mi->mi_conn_priv[ i ].mic_priv );

				LDAP_TAILQ_REMOVE( &mi->mi_conn_priv[ i ].mic_priv, mc, mc_q );
				meta_back_conn_free( mc );
			}
		}

		/*
		 * Destroy the per-target stuff (assuming there's at
		 * least one ...)
		 */
		if ( mi->mi_targets != NULL ) {
			for ( i = 0; i < mi->mi_ntargets; i++ ) {
				metatarget_t	*mt = mi->mi_targets[ i ];

				if ( META_BACK_TGT_QUARANTINE( mt ) ) {
					if ( mt->mt_quarantine.ri_num != mi->mi_quarantine.ri_num )
					{
						mi->mi_ldap_extra->retry_info_destroy( &mt->mt_quarantine );
					}

					ldap_pvt_thread_mutex_destroy( &mt->mt_quarantine_mutex );
				}

				target_free( mt );
			}

			free( mi->mi_targets );
		}

		ldap_pvt_thread_mutex_lock( &mi->mi_cache.mutex );
		if ( mi->mi_cache.tree ) {
			avl_free( mi->mi_cache.tree, meta_dncache_free );
		}
		
		ldap_pvt_thread_mutex_unlock( &mi->mi_cache.mutex );
		ldap_pvt_thread_mutex_destroy( &mi->mi_cache.mutex );

		ldap_pvt_thread_mutex_unlock( &mi->mi_conninfo.lai_mutex );
		ldap_pvt_thread_mutex_destroy( &mi->mi_conninfo.lai_mutex );

		if ( mi->mi_candidates != NULL ) {
			ber_memfree_x( mi->mi_candidates, NULL );
		}

		if ( META_BACK_QUARANTINE( mi ) ) {
			mi->mi_ldap_extra->retry_info_destroy( &mi->mi_quarantine );
		}
	}

	free( be->be_private );
	return 0;
}

#if SLAPD_META == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( meta )

#endif /* SLAPD_META == SLAPD_MOD_DYNAMIC */


