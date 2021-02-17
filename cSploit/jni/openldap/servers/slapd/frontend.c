/* frontend.c - routines for dealing with frontend */
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
#include "lutil.h"
#include "lber_pvt.h"

#include "ldap_rq.h"

static BackendInfo	slap_frontendInfo;
static BackendDB	slap_frontendDB;
BackendDB	*frontendDB;

static int
fe_entry_get_rw(
	Operation *op,
	struct berval *ndn,
	ObjectClass *oc,
	AttributeDescription *at,
	int rw,
	Entry **e )
{
	BackendDB	*bd;
	int		rc = LDAP_NO_SUCH_OBJECT;

	bd = op->o_bd;
	op->o_bd = select_backend( ndn, 0 );
	if ( op->o_bd != NULL ) {
		if ( op->o_bd->be_fetch ) {
			rc = op->o_bd->be_fetch( op, ndn, oc, at, rw, e );
		}
	}
	op->o_bd = bd;

	return rc;
}

static int
fe_entry_release_rw(
	Operation *op,
	Entry *e,
	int rw )
{
	BackendDB	*bd;
	int		rc = LDAP_NO_SUCH_OBJECT;

	bd = op->o_bd;
	op->o_bd = select_backend( &e->e_nname, 0 );
	if ( op->o_bd != NULL ) {
		if ( op->o_bd->be_release ) {
			rc = op->o_bd->be_release( op, e, rw );
		}
	}
	op->o_bd = bd;

	return rc;
}

int
frontend_init( void )
{
	/* data */
	frontendDB = &slap_frontendDB;
	frontendDB->bd_self = frontendDB;

	/* ACLs */
	frontendDB->be_dfltaccess = ACL_READ;

	/* limits */
	frontendDB->be_def_limit.lms_t_soft = SLAPD_DEFAULT_TIMELIMIT;	/* backward compatible limits */
	frontendDB->be_def_limit.lms_t_hard = 0;
	frontendDB->be_def_limit.lms_s_soft = SLAPD_DEFAULT_SIZELIMIT;	/* backward compatible limits */
	frontendDB->be_def_limit.lms_s_hard = 0;
	frontendDB->be_def_limit.lms_s_unchecked = -1;			/* no limit on unchecked size */
	frontendDB->be_def_limit.lms_s_pr = 0;				/* page limit */
	frontendDB->be_def_limit.lms_s_pr_hide = 0;			/* don't hide number of entries left */
	frontendDB->be_def_limit.lms_s_pr_total = 0;			/* number of total entries returned by pagedResults equal to hard limit */

	ldap_pvt_thread_mutex_init( &frontendDB->be_pcl_mutex );

	/* suffix */
	frontendDB->be_suffix = ch_calloc( 2, sizeof( struct berval ) );
	ber_str2bv( "", 0, 1, &frontendDB->be_suffix[0] );
	BER_BVZERO( &frontendDB->be_suffix[1] );
	frontendDB->be_nsuffix = ch_calloc( 2, sizeof( struct berval ) );
	ber_str2bv( "", 0, 1, &frontendDB->be_nsuffix[0] );
	BER_BVZERO( &frontendDB->be_nsuffix[1] );

	/* info */
	frontendDB->bd_info = &slap_frontendInfo;

	SLAP_BFLAGS(frontendDB) |= SLAP_BFLAG_FRONTEND;

	/* name */
	frontendDB->bd_info->bi_type = "frontend";

	/* known controls */
	{
		int	i;

		frontendDB->bd_info->bi_controls = slap_known_controls;

		for ( i = 0; slap_known_controls[ i ]; i++ ) {
			int	cid;

			if ( slap_find_control_id( slap_known_controls[ i ], &cid )
					== LDAP_CONTROL_NOT_FOUND )
			{
				assert( 0 );
				return -1;
			}

			frontendDB->bd_info->bi_ctrls[ cid ] = 1;
			frontendDB->be_ctrls[ cid ] = 1;
		}
	}

	/* calls */
	frontendDB->bd_info->bi_op_abandon = fe_op_abandon;
	frontendDB->bd_info->bi_op_add = fe_op_add;
	frontendDB->bd_info->bi_op_bind = fe_op_bind;
	frontendDB->bd_info->bi_op_compare = fe_op_compare;
	frontendDB->bd_info->bi_op_delete = fe_op_delete;
	frontendDB->bd_info->bi_op_modify = fe_op_modify;
	frontendDB->bd_info->bi_op_modrdn = fe_op_modrdn;
	frontendDB->bd_info->bi_op_search = fe_op_search;
	frontendDB->bd_info->bi_extended = fe_extended;
	frontendDB->bd_info->bi_operational = fe_aux_operational;
	frontendDB->bd_info->bi_entry_get_rw = fe_entry_get_rw;
	frontendDB->bd_info->bi_entry_release_rw = fe_entry_release_rw;
	frontendDB->bd_info->bi_access_allowed = fe_access_allowed;
	frontendDB->bd_info->bi_acl_group = fe_acl_group;
	frontendDB->bd_info->bi_acl_attribute = fe_acl_attribute;

#if 0
	/* FIXME: is this too early? */
	return backend_startup_one( frontendDB );
#endif

	return 0;
}

