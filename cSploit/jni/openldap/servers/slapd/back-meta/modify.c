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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "../back-ldap/back-ldap.h"
#include "back-meta.h"

int
meta_back_modify( Operation *op, SlapReply *rs )
{
	metainfo_t	*mi = ( metainfo_t * )op->o_bd->be_private;
	metatarget_t	*mt;
	metaconn_t	*mc;
	int		rc = 0;
	LDAPMod		**modv = NULL;
	LDAPMod		*mods = NULL;
	Modifications	*ml;
	int		candidate = -1, i;
	int		isupdate;
	struct berval	mdn = BER_BVNULL;
	struct berval	mapped;
	dncookie	dc;
	int		msgid;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	LDAPControl	**ctrls = NULL;

	mc = meta_back_getconn( op, rs, &candidate, LDAP_BACK_SENDERR );
	if ( !mc || !meta_back_dobind( op, rs, mc, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	assert( mc->mc_conns[ candidate ].msc_ld != NULL );

	/*
	 * Rewrite the modify dn, if needed
	 */
	mt = mi->mi_targets[ candidate ];
	dc.target = mt;
	dc.conn = op->o_conn;
	dc.rs = rs;
	dc.ctx = "modifyDN";

	if ( ldap_back_dn_massage( &dc, &op->o_req_dn, &mdn ) ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	for ( i = 0, ml = op->orm_modlist; ml; i++ ,ml = ml->sml_next )
		;

	mods = ch_malloc( sizeof( LDAPMod )*i );
	if ( mods == NULL ) {
		rs->sr_err = LDAP_OTHER;
		send_ldap_result( op, rs );
		goto cleanup;
	}
	modv = ( LDAPMod ** )ch_malloc( ( i + 1 )*sizeof( LDAPMod * ) );
	if ( modv == NULL ) {
		rs->sr_err = LDAP_OTHER;
		send_ldap_result( op, rs );
		goto cleanup;
	}

	dc.ctx = "modifyAttrDN";
	isupdate = be_shadow_update( op );
	for ( i = 0, ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		int	j, is_oc = 0;

		if ( !isupdate && !get_relax( op ) && ml->sml_desc->ad_type->sat_no_user_mod  )
		{
			continue;
		}

		if ( ml->sml_desc == slap_schema.si_ad_objectClass 
				|| ml->sml_desc == slap_schema.si_ad_structuralObjectClass )
		{
			is_oc = 1;
			mapped = ml->sml_desc->ad_cname;

		} else {
			ldap_back_map( &mt->mt_rwmap.rwm_at,
					&ml->sml_desc->ad_cname, &mapped,
					BACKLDAP_MAP );
			if ( BER_BVISNULL( &mapped ) || BER_BVISEMPTY( &mapped ) ) {
				continue;
			}
		}

		modv[ i ] = &mods[ i ];
		mods[ i ].mod_op = ml->sml_op | LDAP_MOD_BVALUES;
		mods[ i ].mod_type = mapped.bv_val;

		/*
		 * FIXME: dn-valued attrs should be rewritten
		 * to allow their use in ACLs at the back-ldap
		 * level.
		 */
		if ( ml->sml_values != NULL ) {
			if ( is_oc ) {
				for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ )
					;
				mods[ i ].mod_bvalues =
					(struct berval **)ch_malloc( ( j + 1 ) *
					sizeof( struct berval * ) );
				for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); ) {
					struct ldapmapping	*mapping;

					ldap_back_mapping( &mt->mt_rwmap.rwm_oc,
							&ml->sml_values[ j ], &mapping, BACKLDAP_MAP );

					if ( mapping == NULL ) {
						if ( mt->mt_rwmap.rwm_oc.drop_missing ) {
							continue;
						}
						mods[ i ].mod_bvalues[ j ] = &ml->sml_values[ j ];

					} else {
						mods[ i ].mod_bvalues[ j ] = &mapping->dst;
					}
					j++;
				}
				mods[ i ].mod_bvalues[ j ] = NULL;

			} else {
				if ( ml->sml_desc->ad_type->sat_syntax ==
						slap_schema.si_syn_distinguishedName )
				{
					( void )ldap_dnattr_rewrite( &dc, ml->sml_values );
					if ( ml->sml_values == NULL ) {
						continue;
					}
				}

				for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ )
					;
				mods[ i ].mod_bvalues =
					(struct berval **)ch_malloc( ( j + 1 ) *
					sizeof( struct berval * ) );
				for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ ) {
					mods[ i ].mod_bvalues[ j ] = &ml->sml_values[ j ];
				}
				mods[ i ].mod_bvalues[ j ] = NULL;
			}

		} else {
			mods[ i ].mod_bvalues = NULL;
		}

		i++;
	}
	modv[ i ] = 0;

retry:;
	ctrls = op->o_ctrls;
	rc = meta_back_controls_add( op, rs, mc, candidate, &ctrls );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	rs->sr_err = ldap_modify_ext( mc->mc_conns[ candidate ].msc_ld, mdn.bv_val,
			modv, ctrls, NULL, &msgid );
	rs->sr_err = meta_back_op_result( mc, op, rs, candidate, msgid,
		mt->mt_timeout[ SLAP_OP_MODIFY ], ( LDAP_BACK_SENDRESULT | retrying ) );
	if ( rs->sr_err == LDAP_UNAVAILABLE && retrying ) {
		retrying &= ~LDAP_BACK_RETRYING;
		if ( meta_back_retry( op, rs, &mc, candidate, LDAP_BACK_SENDERR ) ) {
			/* if the identity changed, there might be need to re-authz */
			(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );
			goto retry;
		}
	}

cleanup:;
	(void)mi->mi_ldap_extra->controls_free( op, rs, &ctrls );

	if ( mdn.bv_val != op->o_req_dn.bv_val ) {
		free( mdn.bv_val );
		BER_BVZERO( &mdn );
	}
	if ( modv != NULL ) {
		for ( i = 0; modv[ i ]; i++ ) {
			free( modv[ i ]->mod_bvalues );
		}
	}
	free( mods );
	free( modv );

	if ( mc ) {
		meta_back_release_conn( mi, mc );
	}

	return rs->sr_err;
}

