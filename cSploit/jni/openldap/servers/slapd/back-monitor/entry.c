/* entry.c - monitor backend entry handling routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2001-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <slap.h>
#include "back-monitor.h"

int
monitor_entry_update(
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e
)
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	monitor_entry_t *mp;

	int		rc = SLAP_CB_CONTINUE;

	assert( mi != NULL );
	assert( e != NULL );
	assert( e->e_private != NULL );

	mp = ( monitor_entry_t * )e->e_private;

	if ( mp->mp_cb ) {
		struct monitor_callback_t	*mc;

		for ( mc = mp->mp_cb; mc; mc = mc->mc_next ) {
			if ( mc->mc_update ) {
				rc = mc->mc_update( op, rs, e, mc->mc_private );
				if ( rc != SLAP_CB_CONTINUE ) {
					break;
				}
			}
		}
	}

	if ( rc == SLAP_CB_CONTINUE && mp->mp_info && mp->mp_info->mss_update ) {
		rc = mp->mp_info->mss_update( op, rs, e );
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		rc = LDAP_SUCCESS;
	}

	return rc;
}

int
monitor_entry_create(
	Operation		*op,
	SlapReply		*rs,
	struct berval		*ndn,
	Entry			*e_parent,
	Entry			**ep )
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	monitor_entry_t *mp;

	int		rc = SLAP_CB_CONTINUE;

	assert( mi != NULL );
	assert( e_parent != NULL );
	assert( e_parent->e_private != NULL );
	assert( ep != NULL );

	mp = ( monitor_entry_t * )e_parent->e_private;

	if ( mp->mp_info && mp->mp_info->mss_create ) {
		rc = mp->mp_info->mss_create( op, rs, ndn, e_parent, ep );
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		rc = LDAP_SUCCESS;
	}
	
	return rc;
}

int
monitor_entry_modify(
	Operation		*op,
	SlapReply		*rs,
	Entry 			*e
)
{
	monitor_info_t	*mi = ( monitor_info_t * )op->o_bd->be_private;
	monitor_entry_t *mp;

	int		rc = SLAP_CB_CONTINUE;

	assert( mi != NULL );
	assert( e != NULL );
	assert( e->e_private != NULL );

	mp = ( monitor_entry_t * )e->e_private;

	if ( mp->mp_cb ) {
		struct monitor_callback_t	*mc;

		for ( mc = mp->mp_cb; mc; mc = mc->mc_next ) {
			if ( mc->mc_modify ) {
				rc = mc->mc_modify( op, rs, e, mc->mc_private );
				if ( rc != SLAP_CB_CONTINUE ) {
					break;
				}
			}
		}
	}

	if ( rc == SLAP_CB_CONTINUE && mp->mp_info && mp->mp_info->mss_modify ) {
		rc = mp->mp_info->mss_modify( op, rs, e );
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		rc = LDAP_SUCCESS;
	}

	return rc;
}

int
monitor_entry_test_flags(
	monitor_entry_t		*mp,
	int			cond
)
{
	assert( mp != NULL );

	return( ( mp->mp_flags & cond ) || ( mp->mp_info->mss_flags & cond ) );
}

monitor_entry_t *
monitor_back_entrypriv_create( void )
{
	monitor_entry_t	*mp;

	mp = ( monitor_entry_t * )ch_calloc( sizeof( monitor_entry_t ), 1 );

	mp->mp_next = NULL;
	mp->mp_children = NULL;
	mp->mp_info = NULL;
	mp->mp_flags = MONITOR_F_NONE;
	mp->mp_cb = NULL;

	ldap_pvt_thread_mutex_init( &mp->mp_mutex );

	return mp;
}

Entry *
monitor_entry_stub(
	struct berval *pdn,
	struct berval *pndn,
	struct berval *rdn,
	ObjectClass *oc,
	struct berval *create,
	struct berval *modify
)
{
	monitor_info_t *mi;
	AttributeDescription *nad = NULL;
	Entry *e;
	struct berval nat;
	char *ptr;
	const char *text;
	int rc;

	mi = ( monitor_info_t * )be_monitor->be_private;

	nat = *rdn;
	ptr = strchr( nat.bv_val, '=' );
	nat.bv_len = ptr - nat.bv_val;
	rc = slap_bv2ad( &nat, &nad, &text );
	if ( rc )
		return NULL;

	e = entry_alloc();
	if ( e ) {
		struct berval nrdn;

		rdnNormalize( 0, NULL, NULL, rdn, &nrdn, NULL );
		build_new_dn( &e->e_name, pdn, rdn, NULL );
		build_new_dn( &e->e_nname, pndn, &nrdn, NULL );
		ber_memfree( nrdn.bv_val );
		nat.bv_val = ptr + 1;
		nat.bv_len = rdn->bv_len - ( nat.bv_val - rdn->bv_val );
		attr_merge_normalize_one( e, slap_schema.si_ad_objectClass,
			&oc->soc_cname, NULL );
		attr_merge_normalize_one( e, slap_schema.si_ad_structuralObjectClass,
			&oc->soc_cname, NULL );
		attr_merge_normalize_one( e, nad, &nat, NULL );
		attr_merge_one( e, slap_schema.si_ad_creatorsName, &mi->mi_creatorsName,
			&mi->mi_ncreatorsName );
		attr_merge_one( e, slap_schema.si_ad_modifiersName, &mi->mi_creatorsName,
			&mi->mi_ncreatorsName );
		attr_merge_normalize_one( e, slap_schema.si_ad_createTimestamp,
			create ? create : &mi->mi_startTime, NULL );
		attr_merge_normalize_one( e, slap_schema.si_ad_modifyTimestamp,
			modify ? modify : &mi->mi_startTime, NULL );
	}
	return e;
}
