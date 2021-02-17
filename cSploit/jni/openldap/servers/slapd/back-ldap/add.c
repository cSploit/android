/* add.c - ldap backend add function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Pierangelo Masarati.
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
#include "back-ldap.h"

int
ldap_back_add(
	Operation	*op,
	SlapReply	*rs )
{
	ldapinfo_t		*li = (ldapinfo_t *)op->o_bd->be_private;

	ldapconn_t		*lc = NULL;
	int			i = 0,
				j = 0;
	Attribute		*a;
	LDAPMod			**attrs = NULL,
				*attrs2 = NULL;
	ber_int_t		msgid;
	int			isupdate;
	ldap_back_send_t	retrying = LDAP_BACK_RETRYING;
	LDAPControl		**ctrls = NULL;

	rs->sr_err = LDAP_SUCCESS;
	
	Debug( LDAP_DEBUG_ARGS, "==> ldap_back_add(\"%s\")\n",
			op->o_req_dn.bv_val, 0, 0 );

	if ( !ldap_back_dobind( &lc, op, rs, LDAP_BACK_SENDERR ) ) {
		lc = NULL;
		goto cleanup;
	}

	/* Count number of attributes in entry */
	for ( i = 1, a = op->oq_add.rs_e->e_attrs; a; i++, a = a->a_next )
		/* just count attrs */ ;
	
	/* Create array of LDAPMods for ldap_add() */
	attrs = (LDAPMod **)ch_malloc( sizeof( LDAPMod * )*i 
			+ sizeof( LDAPMod )*( i - 1 ) );
	attrs2 = ( LDAPMod * )&attrs[ i ];

	isupdate = be_shadow_update( op );
	for ( i = 0, a = op->oq_add.rs_e->e_attrs; a; a = a->a_next ) {
		if ( !isupdate && !get_relax( op ) && a->a_desc->ad_type->sat_no_user_mod  )
		{
			continue;
		}

		attrs[ i ] = &attrs2[ i ];
		attrs[ i ]->mod_op = LDAP_MOD_BVALUES;
		attrs[ i ]->mod_type = a->a_desc->ad_cname.bv_val;

		for ( j = 0; a->a_vals[ j ].bv_val; j++ )
			/* just count vals */ ;
		attrs[i]->mod_vals.modv_bvals = 
			ch_malloc( ( j + 1 )*sizeof( struct berval * ) );
		for ( j = 0; a->a_vals[ j ].bv_val; j++ ) {
			attrs[ i ]->mod_vals.modv_bvals[ j ] = &a->a_vals[ j ];
		}
		attrs[ i ]->mod_vals.modv_bvals[ j ] = NULL;
		i++;
	}
	attrs[ i ] = NULL;

retry:
	ctrls = op->o_ctrls;
	rs->sr_err = ldap_back_controls_add( op, rs, lc, &ctrls );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	rs->sr_err = ldap_add_ext( lc->lc_ld, op->o_req_dn.bv_val, attrs,
			ctrls, NULL, &msgid );
	rs->sr_err = ldap_back_op_result( lc, op, rs, msgid,
		li->li_timeout[ SLAP_OP_ADD ],
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
	ldap_pvt_mp_add( li->li_ops_completed[ SLAP_OP_ADD ], 1 );
	ldap_pvt_thread_mutex_unlock( &li->li_counter_mutex );

cleanup:
	(void)ldap_back_controls_free( op, rs, &ctrls );

	if ( attrs ) {
		for ( --i; i >= 0; --i ) {
			ch_free( attrs[ i ]->mod_vals.modv_bvals );
		}
		ch_free( attrs );
	}

	if ( lc ) {
		ldap_back_release_conn( li, lc );
	}

	Debug( LDAP_DEBUG_ARGS, "<== ldap_back_add(\"%s\"): %d\n",
			op->o_req_dn.bv_val, rs->sr_err, 0 );

	return rs->sr_err;
}

