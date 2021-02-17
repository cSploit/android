/* modify.c - ldap backend modify function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999-2003 Howard Chu.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
#include "back-ldap.h"

int
ldap_back_modify(
		Operation	*op,
		SlapReply	*rs )
{
	ldapinfo_t		*li = (ldapinfo_t *)op->o_bd->be_private;

	ldapconn_t		*lc = NULL;
	LDAPMod			**modv = NULL,
				*mods = NULL;
	Modifications		*ml;
	int			i, j, rc;
	ber_int_t		msgid;
	int			isupdate;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	LDAPControl		**ctrls = NULL;

	if ( !ldap_back_dobind( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
		return rs->sr_err;
	}

	for ( i = 0, ml = op->orm_modlist; ml; i++, ml = ml->sml_next )
		/* just count mods */ ;

	modv = (LDAPMod **)ch_malloc( ( i + 1 )*sizeof( LDAPMod * )
			+ i*sizeof( LDAPMod ) );
	if ( modv == NULL ) {
		rc = LDAP_NO_MEMORY;
		goto cleanup;
	}
	mods = (LDAPMod *)&modv[ i + 1 ];

	isupdate = be_shadow_update( op );
	for ( i = 0, ml = op->orm_modlist; ml; ml = ml->sml_next ) {
		if ( !isupdate && !get_relax( op ) && ml->sml_desc->ad_type->sat_no_user_mod  )
		{
			continue;
		}

		modv[ i ] = &mods[ i ];
		mods[ i ].mod_op = ( ml->sml_op | LDAP_MOD_BVALUES );
		mods[ i ].mod_type = ml->sml_desc->ad_cname.bv_val;

		if ( ml->sml_values != NULL ) {
			if ( ml->sml_values == NULL ) {	
				continue;
			}

			for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ )
				/* just count mods */ ;
			mods[ i ].mod_bvalues =
				(struct berval **)ch_malloc( ( j + 1 )*sizeof( struct berval * ) );
			for ( j = 0; !BER_BVISNULL( &ml->sml_values[ j ] ); j++ )
			{
				mods[ i ].mod_bvalues[ j ] = &ml->sml_values[ j ];
			}
			mods[ i ].mod_bvalues[ j ] = NULL;

		} else {
			mods[ i ].mod_bvalues = NULL;
		}

		i++;
	}
	modv[ i ] = 0;

retry:;
	ctrls = op->o_ctrls;
	rc = ldap_back_controls_add( op, rs, lc, &ctrls );
	if ( rc != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		rc = -1;
		goto cleanup;
	}

	rs->sr_err = ldap_modify_ext( lc->lc_ld, op->o_req_dn.bv_val, modv,
			ctrls, NULL, &msgid );
	rc = ldap_back_op_result( lc, op, rs, msgid,
		li->li_timeout[ SLAP_OP_MODIFY ],
		( LDAP_BACK_SENDRESULT | retrying ) );
	if ( rs->sr_err == LDAP_UNAVAILABLE && retrying ) {
		retrying &= ~LDAP_BACK_RETRYING;
		if ( ldap_back_retry( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
			/* if the identity changed, there might be need to re-authz */
			(void)ldap_back_controls_free( op, rs, &ctrls );
			goto retry;
		}
	}

	ldap_pvt_thread_mutex_lock( &li->li_counter_mutex );
	ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_MODIFY ], 1 );
	ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

cleanup:;
	(void)ldap_back_controls_free( op, rs, &ctrls );

	for ( i = 0; modv[ i ]; i++ ) {
		ch_free( modv[ i ]->mod_bvalues );
	}
	ch_free( modv );

	if ( lc != NULL ) {
		ldap_back_release_conn( li, lc );
	}

	return rc;
}

