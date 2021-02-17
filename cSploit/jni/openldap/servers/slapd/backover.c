/* backover.c - backend overlay routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
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

/* Functions to overlay other modules over a backend. */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#define SLAPD_TOOLS
#include "slap.h"
#include "config.h"

static slap_overinst *overlays;

static int
over_db_config(
	BackendDB *be,
	const char *fname,
	int lineno,
	int argc,
	char **argv
)
{
	slap_overinfo *oi = be->bd_info->bi_private;
	slap_overinst *on = oi->oi_list;
	BackendInfo *bi_orig = be->bd_info;
	struct ConfigOCs *be_cf_ocs = be->be_cf_ocs;
	ConfigArgs ca = {0};
	int rc = 0;

	if ( oi->oi_orig->bi_db_config ) {
		be->bd_info = oi->oi_orig;
		be->be_cf_ocs = oi->oi_orig->bi_cf_ocs;
		rc = oi->oi_orig->bi_db_config( be, fname, lineno,
			argc, argv );

		if ( be->bd_info != oi->oi_orig ) {
			slap_overinfo	*oi2;
			slap_overinst	*on2, **onp;
			BackendDB	be2 = *be;
			int		i;

			/* a database added an overlay;
			 * work it around... */
			assert( overlay_is_over( be ) );
			
			oi2 = ( slap_overinfo * )be->bd_info->bi_private;
			on2 = oi2->oi_list;

			/* need to put a uniqueness check here as well;
			 * note that in principle there could be more than
			 * one overlay as a result of multiple calls to
			 * overlay_config() */
			be2.bd_info = (BackendInfo *)oi;

			for ( i = 0, onp = &on2; *onp; i++, onp = &(*onp)->on_next ) {
				if ( overlay_is_inst( &be2, (*onp)->on_bi.bi_type ) ) {
					Debug( LDAP_DEBUG_ANY, "over_db_config(): "
							"warning, freshly added "
							"overlay #%d \"%s\" is already in list\n",
							i, (*onp)->on_bi.bi_type, 0 );

					/* NOTE: if the overlay already exists,
					 * there is no way to merge the results
					 * of the configuration that may have 
					 * occurred during bi_db_config(); we
					 * just issue a warning, and the 
					 * administrator should deal with this */
				}
			}
			*onp = oi->oi_list;

			oi->oi_list = on2;

			ch_free( be->bd_info );
		}

		be->bd_info = (BackendInfo *)oi;
		if ( rc != SLAP_CONF_UNKNOWN ) return rc;
	}

	ca.argv = argv;
	ca.argc = argc;
	ca.fname = fname;
	ca.lineno = lineno;
	ca.be = be;
	snprintf( ca.log, sizeof( ca.log ), "%s: line %d",
			ca.fname, ca.lineno );
	ca.op = SLAP_CONFIG_ADD;
	ca.valx = -1;

	for (; on; on=on->on_next) {
		rc = SLAP_CONF_UNKNOWN;
		if (on->on_bi.bi_cf_ocs) {
			ConfigTable *ct;
			ca.bi = &on->on_bi;
			ct = config_find_keyword( on->on_bi.bi_cf_ocs->co_table, &ca );
			if ( ct ) {
				ca.table = on->on_bi.bi_cf_ocs->co_type;
				rc = config_add_vals( ct, &ca );
				if ( rc != SLAP_CONF_UNKNOWN )
					break;
			}
		}
		if (on->on_bi.bi_db_config && rc == SLAP_CONF_UNKNOWN) {
			be->bd_info = &on->on_bi;
			rc = on->on_bi.bi_db_config( be, fname, lineno,
				argc, argv );
			if ( rc != SLAP_CONF_UNKNOWN ) break;
		}
	}
	be->bd_info = bi_orig;
	be->be_cf_ocs = be_cf_ocs;
	
	return rc;
}

static int
over_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinfo *oi = be->bd_info->bi_private;
	slap_overinst *on = oi->oi_list;
	BackendDB db = *be;
	int rc = 0;

	db.be_flags |= SLAP_DBFLAG_OVERLAY;
	db.bd_info = oi->oi_orig;
	if ( db.bd_info->bi_db_open ) {
		rc = db.bd_info->bi_db_open( &db, cr );
	}

	for (; on && rc == 0; on=on->on_next) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		db.bd_info = &on->on_bi;
		if ( db.bd_info->bi_db_open ) {
			rc = db.bd_info->bi_db_open( &db, cr );
		}
	}

	return rc;
}

static int
over_db_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinfo *oi = be->bd_info->bi_private;
	slap_overinst *on = oi->oi_list;
	BackendInfo *bi_orig = be->bd_info;
	int rc = 0;

	for (; on && rc == 0; on=on->on_next) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		be->bd_info = &on->on_bi;
		if ( be->bd_info->bi_db_close ) {
			rc = be->bd_info->bi_db_close( be, cr );
		}
	}

	if ( oi->oi_orig->bi_db_close ) {
		be->bd_info = oi->oi_orig;
		rc = be->bd_info->bi_db_close( be, cr );
	}

	be->bd_info = bi_orig;
	return rc;
}

static int
over_db_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinfo *oi = be->bd_info->bi_private;
	slap_overinst *on = oi->oi_list, *next;
	BackendInfo *bi_orig = be->bd_info;
	int rc = 0;

	be->bd_info = oi->oi_orig;
	if ( be->bd_info->bi_db_destroy ) {
		rc = be->bd_info->bi_db_destroy( be, cr );
	}

	for (; on && rc == 0; on=on->on_next) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		be->bd_info = &on->on_bi;
		if ( be->bd_info->bi_db_destroy ) {
			rc = be->bd_info->bi_db_destroy( be, cr );
		}
	}

	on = oi->oi_list;
	if ( on ) {
		for (next = on->on_next; on; on=next) {
			next = on->on_next;
			free( on );
		}
	}
	be->bd_info = bi_orig;
	free( oi );
	return rc;
}

static int
over_back_response ( Operation *op, SlapReply *rs )
{
	slap_overinfo *oi = op->o_callback->sc_private;
	slap_overinst *on = oi->oi_list;
	int rc = SLAP_CB_CONTINUE;
	BackendDB *be = op->o_bd, db = *op->o_bd;

	db.be_flags |= SLAP_DBFLAG_OVERLAY;
	op->o_bd = &db;
	for (; on; on=on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_response ) {
			db.bd_info = (BackendInfo *)on;
			rc = on->on_response( op, rs );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}
	/* Bypass the remaining on_response layers, but allow
	 * normal execution to continue.
	 */
	if ( rc == SLAP_CB_BYPASS )
		rc = SLAP_CB_CONTINUE;
	op->o_bd = be;
	return rc;
}

static int
over_access_allowed(
	Operation		*op,
	Entry			*e,
	AttributeDescription	*desc,
	struct berval		*val,
	slap_access_t		access,
	AccessControlState	*state,
	slap_mask_t		*maskp )
{
	slap_overinfo *oi;
	slap_overinst *on;
	BackendInfo *bi;
	BackendDB *be = op->o_bd, db;
	int rc = SLAP_CB_CONTINUE;

	/* FIXME: used to happen for instance during abandon
	 * when global overlays are used... */
	assert( op->o_bd != NULL );

	bi = op->o_bd->bd_info;
	/* Were we invoked on the frontend? */
	if ( !bi->bi_access_allowed ) {
		oi = frontendDB->bd_info->bi_private;
	} else {
		oi = op->o_bd->bd_info->bi_private;
	}
	on = oi->oi_list;

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_bi.bi_access_allowed ) {
			/* NOTE: do not copy the structure until required */
		 	if ( !SLAP_ISOVERLAY( op->o_bd ) ) {
 				db = *op->o_bd;
				db.be_flags |= SLAP_DBFLAG_OVERLAY;
				op->o_bd = &db;
			}

			op->o_bd->bd_info = (BackendInfo *)on;
			rc = on->on_bi.bi_access_allowed( op, e,
				desc, val, access, state, maskp );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		BI_access_allowed	*bi_access_allowed;

		/* if the database structure was changed, o_bd points to a
		 * copy of the structure; put the original bd_info in place */
		if ( SLAP_ISOVERLAY( op->o_bd ) ) {
			op->o_bd->bd_info = oi->oi_orig;
		}

		if ( oi->oi_orig->bi_access_allowed ) {
			bi_access_allowed = oi->oi_orig->bi_access_allowed;
		} else {
			bi_access_allowed = slap_access_allowed;
		}

		rc = bi_access_allowed( op, e,
			desc, val, access, state, maskp );
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		/* access not allowed */
		rc = 0;
	}

	op->o_bd = be;
	op->o_bd->bd_info = bi;

	return rc;
}

int
overlay_entry_get_ov(
	Operation		*op,
	struct berval	*dn,
	ObjectClass		*oc,
	AttributeDescription	*ad,
	int	rw,
	Entry	**e,
	slap_overinst *on )
{
	slap_overinfo *oi = on->on_info;
	BackendDB *be = op->o_bd, db;
	BackendInfo *bi = op->o_bd->bd_info;
	int rc = SLAP_CB_CONTINUE;

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_bi.bi_entry_get_rw ) {
			/* NOTE: do not copy the structure until required */
		 	if ( !SLAP_ISOVERLAY( op->o_bd ) ) {
 				db = *op->o_bd;
				db.be_flags |= SLAP_DBFLAG_OVERLAY;
				op->o_bd = &db;
			}

			op->o_bd->bd_info = (BackendInfo *)on;
			rc = on->on_bi.bi_entry_get_rw( op, dn,
				oc, ad, rw, e );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		/* if the database structure was changed, o_bd points to a
		 * copy of the structure; put the original bd_info in place */
		if ( SLAP_ISOVERLAY( op->o_bd ) ) {
			op->o_bd->bd_info = oi->oi_orig;
		}

		if ( oi->oi_orig->bi_entry_get_rw ) {
			rc = oi->oi_orig->bi_entry_get_rw( op, dn,
				oc, ad, rw, e );
		}
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		rc = LDAP_UNWILLING_TO_PERFORM;
	}

	op->o_bd = be;
	op->o_bd->bd_info = bi;

	return rc;
}

static int
over_entry_get_rw(
	Operation		*op,
	struct berval	*dn,
	ObjectClass		*oc,
	AttributeDescription	*ad,
	int	rw,
	Entry	**e )
{
	slap_overinfo *oi;
	slap_overinst *on;

	assert( op->o_bd != NULL );

	oi = op->o_bd->bd_info->bi_private;
	on = oi->oi_list;

	return overlay_entry_get_ov( op, dn, oc, ad, rw, e, on );
}

int
overlay_entry_release_ov(
	Operation	*op,
	Entry	*e,
	int rw,
	slap_overinst *on )
{
	slap_overinfo *oi = on->on_info;
	BackendDB *be = op->o_bd, db;
	BackendInfo *bi = op->o_bd->bd_info;
	int rc = SLAP_CB_CONTINUE;

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_bi.bi_entry_release_rw ) {
			/* NOTE: do not copy the structure until required */
		 	if ( !SLAP_ISOVERLAY( op->o_bd ) ) {
 				db = *op->o_bd;
				db.be_flags |= SLAP_DBFLAG_OVERLAY;
				op->o_bd = &db;
			}

			op->o_bd->bd_info = (BackendInfo *)on;
			rc = on->on_bi.bi_entry_release_rw( op, e, rw );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		/* if the database structure was changed, o_bd points to a
		 * copy of the structure; put the original bd_info in place */
		if ( SLAP_ISOVERLAY( op->o_bd ) ) {
			op->o_bd->bd_info = oi->oi_orig;
		}

		if ( oi->oi_orig->bi_entry_release_rw ) {
			rc = oi->oi_orig->bi_entry_release_rw( op, e, rw );
		}
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		entry_free( e );
		rc = 0;
	}

	op->o_bd = be;
	op->o_bd->bd_info = bi;

	return rc;
}

static int
over_entry_release_rw(
	Operation	*op,
	Entry	*e,
	int rw )
{
	slap_overinfo *oi;
	slap_overinst *on;

	assert( op->o_bd != NULL );

	oi = op->o_bd->bd_info->bi_private;
	on = oi->oi_list;

	return overlay_entry_release_ov( op, e, rw, on );
}

static int
over_acl_group(
	Operation		*op,
	Entry			*e,
	struct berval		*gr_ndn,
	struct berval		*op_ndn,
	ObjectClass		*group_oc,
	AttributeDescription	*group_at )
{
	slap_overinfo *oi;
	slap_overinst *on;
	BackendInfo *bi;
	BackendDB *be = op->o_bd, db;
	int rc = SLAP_CB_CONTINUE;

	/* FIXME: used to happen for instance during abandon
	 * when global overlays are used... */
	assert( be != NULL );

	bi = be->bd_info;
	oi = bi->bi_private;
	on = oi->oi_list;

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_bi.bi_acl_group ) {
			/* NOTE: do not copy the structure until required */
		 	if ( !SLAP_ISOVERLAY( op->o_bd ) ) {
 				db = *op->o_bd;
				db.be_flags |= SLAP_DBFLAG_OVERLAY;
				op->o_bd = &db;
			}

			op->o_bd->bd_info = (BackendInfo *)on;
			rc = on->on_bi.bi_acl_group( op, e,
				gr_ndn, op_ndn, group_oc, group_at );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		BI_acl_group		*bi_acl_group;

		/* if the database structure was changed, o_bd points to a
		 * copy of the structure; put the original bd_info in place */
		if ( SLAP_ISOVERLAY( op->o_bd ) ) {
			op->o_bd->bd_info = oi->oi_orig;
		}

		if ( oi->oi_orig->bi_acl_group ) {
			bi_acl_group = oi->oi_orig->bi_acl_group;
		} else {
			bi_acl_group = backend_group;
		}

		rc = bi_acl_group( op, e,
			gr_ndn, op_ndn, group_oc, group_at );
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		/* access not allowed */
		rc = 0;
	}

	op->o_bd = be;
	op->o_bd->bd_info = bi;

	return rc;
}

static int
over_acl_attribute(
	Operation		*op,
	Entry			*target,
	struct berval		*entry_ndn,
	AttributeDescription	*entry_at,
	BerVarray		*vals,
	slap_access_t		access )
{
	slap_overinfo *oi;
	slap_overinst *on;
	BackendInfo *bi;
	BackendDB *be = op->o_bd, db;
	int rc = SLAP_CB_CONTINUE;

	/* FIXME: used to happen for instance during abandon
	 * when global overlays are used... */
	assert( be != NULL );

	bi = be->bd_info;
	oi = bi->bi_private;
	on = oi->oi_list;

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		if ( on->on_bi.bi_acl_attribute ) {
			/* NOTE: do not copy the structure until required */
		 	if ( !SLAP_ISOVERLAY( op->o_bd ) ) {
 				db = *op->o_bd;
				db.be_flags |= SLAP_DBFLAG_OVERLAY;
				op->o_bd = &db;
			}

			op->o_bd->bd_info = (BackendInfo *)on;
			rc = on->on_bi.bi_acl_attribute( op, target,
				entry_ndn, entry_at, vals, access );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	if ( rc == SLAP_CB_CONTINUE ) {
		BI_acl_attribute		*bi_acl_attribute;

		/* if the database structure was changed, o_bd points to a
		 * copy of the structure; put the original bd_info in place */
		if ( SLAP_ISOVERLAY( op->o_bd ) ) {
			op->o_bd->bd_info = oi->oi_orig;
		}

		if ( oi->oi_orig->bi_acl_attribute ) {
			bi_acl_attribute = oi->oi_orig->bi_acl_attribute;
		} else {
			bi_acl_attribute = backend_attribute;
		}

		rc = bi_acl_attribute( op, target,
			entry_ndn, entry_at, vals, access );
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		/* access not allowed */
		rc = 0;
	}

	op->o_bd = be;
	op->o_bd->bd_info = bi;

	return rc;
}

int
overlay_callback_after_backover( Operation *op, slap_callback *sc, int append )
{
	slap_callback **scp;

	for ( scp = &op->o_callback; *scp != NULL; scp = &(*scp)->sc_next ) {
		if ( (*scp)->sc_response == over_back_response ) {
			sc->sc_next = (*scp)->sc_next;
			(*scp)->sc_next = sc;
			return 0;
		}
	}

	if ( append ) {
		*scp = sc;
		return 0;
	}

	return 1;
}

/*
 * default return code in case of missing backend function
 * and overlay stack returning SLAP_CB_CONTINUE
 */
static int op_rc[ op_last ] = {
	LDAP_UNWILLING_TO_PERFORM,	/* bind */
	LDAP_UNWILLING_TO_PERFORM,	/* unbind */
	LDAP_UNWILLING_TO_PERFORM,	/* search */
	SLAP_CB_CONTINUE,		/* compare; pass to frontend */
	LDAP_UNWILLING_TO_PERFORM,	/* modify */
	LDAP_UNWILLING_TO_PERFORM,	/* modrdn */
	LDAP_UNWILLING_TO_PERFORM,	/* add */
	LDAP_UNWILLING_TO_PERFORM,	/* delete */
	LDAP_UNWILLING_TO_PERFORM,	/* abandon */
	LDAP_UNWILLING_TO_PERFORM,	/* cancel */
	LDAP_UNWILLING_TO_PERFORM,	/* extended */
	LDAP_SUCCESS,			/* aux_operational */
	LDAP_SUCCESS,			/* aux_chk_referrals */
	SLAP_CB_CONTINUE		/* aux_chk_controls; pass to frontend */
};

int overlay_op_walk(
	Operation *op,
	SlapReply *rs,
	slap_operation_t which,
	slap_overinfo *oi,
	slap_overinst *on
)
{
	BackendInfo *bi;
	int rc = SLAP_CB_CONTINUE;

	for (; on; on=on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		bi = &on->on_bi;
		if ( (&bi->bi_op_bind)[ which ] ) {
			op->o_bd->bd_info = (BackendInfo *)on;
			rc = (&bi->bi_op_bind)[ which ]( op, rs );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}
	if ( rc == SLAP_CB_BYPASS )
		rc = SLAP_CB_CONTINUE;

	bi = oi->oi_orig;
	if ( (&bi->bi_op_bind)[ which ] && rc == SLAP_CB_CONTINUE ) {
		op->o_bd->bd_info = bi;
		rc = (&bi->bi_op_bind)[ which ]( op, rs );
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		rc = op_rc[ which ];
	}

	/* The underlying backend didn't handle the request, make sure
	 * overlay cleanup is processed.
	 */
	if ( rc == LDAP_UNWILLING_TO_PERFORM ) {
		slap_callback *sc_next;
		for ( ; op->o_callback && op->o_callback->sc_response !=
			over_back_response; op->o_callback = sc_next ) {
			sc_next = op->o_callback->sc_next;
			if ( op->o_callback->sc_cleanup ) {
				op->o_callback->sc_cleanup( op, rs );
			}
		}
	}
	return rc;
}

static int
over_op_func(
	Operation *op,
	SlapReply *rs,
	slap_operation_t which
)
{
	slap_overinfo *oi;
	slap_overinst *on;
	BackendDB *be = op->o_bd, db;
	slap_callback cb = {NULL, over_back_response, NULL, NULL}, **sc;
	int rc = SLAP_CB_CONTINUE;

	/* FIXME: used to happen for instance during abandon
	 * when global overlays are used... */
	assert( op->o_bd != NULL );

	oi = op->o_bd->bd_info->bi_private;
	on = oi->oi_list;

 	if ( !SLAP_ISOVERLAY( op->o_bd )) {
 		db = *op->o_bd;
		db.be_flags |= SLAP_DBFLAG_OVERLAY;
		op->o_bd = &db;
	}
	cb.sc_next = op->o_callback;
	cb.sc_private = oi;
	op->o_callback = &cb;

	rc = overlay_op_walk( op, rs, which, oi, on );
	for ( sc = &op->o_callback; *sc; sc = &(*sc)->sc_next ) {
		if ( *sc == &cb ) {
			*sc = cb.sc_next;
			break;
		}
	}

	op->o_bd = be;
	return rc;
}

static int
over_op_bind( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_bind );
}

static int
over_op_unbind( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_unbind );
}

static int
over_op_search( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_search );
}

static int
over_op_compare( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_compare );
}

static int
over_op_modify( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_modify );
}

static int
over_op_modrdn( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_modrdn );
}

static int
over_op_add( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_add );
}

static int
over_op_delete( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_delete );
}

static int
over_op_abandon( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_abandon );
}

static int
over_op_cancel( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_cancel );
}

static int
over_op_extended( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_extended );
}

static int
over_aux_operational( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_aux_operational );
}

static int
over_aux_chk_referrals( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_aux_chk_referrals );
}

static int
over_aux_chk_controls( Operation *op, SlapReply *rs )
{
	return over_op_func( op, rs, op_aux_chk_controls );
}

enum conn_which {
	conn_init = 0,
	conn_destroy,
	conn_last
};

static int
over_connection_func(
	BackendDB	*bd,
	Connection	*conn,
	enum conn_which	which
)
{
	slap_overinfo		*oi;
	slap_overinst		*on;
	BackendDB		db;
	int			rc = SLAP_CB_CONTINUE;
	BI_connection_init	**func;

	/* FIXME: used to happen for instance during abandon
	 * when global overlays are used... */
	assert( bd != NULL );

	oi = bd->bd_info->bi_private;
	on = oi->oi_list;

 	if ( !SLAP_ISOVERLAY( bd ) ) {
 		db = *bd;
		db.be_flags |= SLAP_DBFLAG_OVERLAY;
		bd = &db;
	}

	for ( ; on; on = on->on_next ) {
		if ( on->on_bi.bi_flags & SLAPO_BFLAG_DISABLED )
			continue;
		func = &on->on_bi.bi_connection_init;
		if ( func[ which ] ) {
			bd->bd_info = (BackendInfo *)on;
			rc = func[ which ]( bd, conn );
			if ( rc != SLAP_CB_CONTINUE ) break;
		}
	}

	func = &oi->oi_orig->bi_connection_init;
	if ( func[ which ] && rc == SLAP_CB_CONTINUE ) {
		bd->bd_info = oi->oi_orig;
		rc = func[ which ]( bd, conn );
	}
	/* should not fall thru this far without anything happening... */
	if ( rc == SLAP_CB_CONTINUE ) {
		rc = LDAP_UNWILLING_TO_PERFORM;
	}

	return rc;
}

static int
over_connection_init(
	BackendDB	*bd,
	Connection	*conn
)
{
	return over_connection_func( bd, conn, conn_init );
}

static int
over_connection_destroy(
	BackendDB	*bd,
	Connection	*conn
)
{
	return over_connection_func( bd, conn, conn_destroy );
}

int
overlay_register(
	slap_overinst *on
)
{
	slap_overinst	*tmp;

	/* FIXME: check for duplicates? */
	for ( tmp = overlays; tmp != NULL; tmp = tmp->on_next ) {
		if ( strcmp( on->on_bi.bi_type, tmp->on_bi.bi_type ) == 0 ) {
			Debug( LDAP_DEBUG_ANY,
				"overlay_register(\"%s\"): "
				"name already in use.\n",
				on->on_bi.bi_type, 0, 0 );
			return -1;
		}

		if ( on->on_bi.bi_obsolete_names != NULL ) {
			int	i;

			for ( i = 0; on->on_bi.bi_obsolete_names[ i ] != NULL; i++ ) {
				if ( strcmp( on->on_bi.bi_obsolete_names[ i ], tmp->on_bi.bi_type ) == 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"overlay_register(\"%s\"): "
						"obsolete name \"%s\" already in use "
						"by overlay \"%s\".\n",
						on->on_bi.bi_type,
						on->on_bi.bi_obsolete_names[ i ],
						tmp->on_bi.bi_type );
					return -1;
				}
			}
		}

		if ( tmp->on_bi.bi_obsolete_names != NULL ) {
			int	i;

			for ( i = 0; tmp->on_bi.bi_obsolete_names[ i ] != NULL; i++ ) {
				int	j;

				if ( strcmp( on->on_bi.bi_type, tmp->on_bi.bi_obsolete_names[ i ] ) == 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"overlay_register(\"%s\"): "
						"name already in use "
						"as obsolete by overlay \"%s\".\n",
						on->on_bi.bi_type,
						tmp->on_bi.bi_obsolete_names[ i ], 0 );
					return -1;
				}

				if ( on->on_bi.bi_obsolete_names != NULL ) {
					for ( j = 0; on->on_bi.bi_obsolete_names[ j ] != NULL; j++ ) {
						if ( strcmp( on->on_bi.bi_obsolete_names[ j ], tmp->on_bi.bi_obsolete_names[ i ] ) == 0 ) {
							Debug( LDAP_DEBUG_ANY,
								"overlay_register(\"%s\"): "
								"obsolete name \"%s\" already in use "
								"as obsolete by overlay \"%s\".\n",
								on->on_bi.bi_type,
								on->on_bi.bi_obsolete_names[ j ],
								tmp->on_bi.bi_type );
							return -1;
						}
					}
				}
			}
		}
	}

	on->on_next = overlays;
	overlays = on;
	return 0;
}

/*
 * iterator on registered overlays; overlay_next( NULL ) returns the first
 * overlay; subsequent calls with the previously returned value allow to 
 * iterate over the entire list; returns NULL when no more overlays are 
 * registered.
 */

slap_overinst *
overlay_next(
	slap_overinst *on
)
{
	if ( on == NULL ) {
		return overlays;
	}

	return on->on_next;
}

/*
 * returns a specific registered overlay based on the type; NULL if not
 * registered.
 */

slap_overinst *
overlay_find( const char *over_type )
{
	slap_overinst *on = overlays;

	assert( over_type != NULL );

	for ( ; on; on = on->on_next ) {
		if ( strcmp( on->on_bi.bi_type, over_type ) == 0 ) {
			goto foundit;
		}

		if ( on->on_bi.bi_obsolete_names != NULL ) {
			int	i;

			for ( i = 0; on->on_bi.bi_obsolete_names[ i ] != NULL; i++ ) {
				if ( strcmp( on->on_bi.bi_obsolete_names[ i ], over_type ) == 0 ) {
					Debug( LDAP_DEBUG_ANY,
						"overlay_find(\"%s\"): "
						"obsolete name for \"%s\".\n",
						on->on_bi.bi_obsolete_names[ i ],
						on->on_bi.bi_type, 0 );
					goto foundit;
				}
			}
		}
	}

foundit:;
	return on;
}

static const char overtype[] = "over";

/*
 * returns TRUE (1) if the database is actually an overlay instance;
 * FALSE (0) otherwise.
 */

int
overlay_is_over( BackendDB *be )
{
	return be->bd_info->bi_type == overtype;
}

/*
 * returns TRUE (1) if the given database is actually an overlay
 * instance and, somewhere in the list, contains the requested overlay;
 * FALSE (0) otherwise.
 */

int
overlay_is_inst( BackendDB *be, const char *over_type )
{
	slap_overinst	*on;

	assert( be != NULL );

	if ( !overlay_is_over( be ) ) {
		return 0;
	}
	
	on = ((slap_overinfo *)be->bd_info->bi_private)->oi_list;
	for ( ; on; on = on->on_next ) {
		if ( strcmp( on->on_bi.bi_type, over_type ) == 0 ) {
			return 1;
		}
	}

	return 0;
}

int
overlay_register_control( BackendDB *be, const char *oid )
{
	int		gotit = 0;
	int		cid;

	if ( slap_find_control_id( oid, &cid ) == LDAP_CONTROL_NOT_FOUND ) {
		return -1;
	}

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		BackendDB *bd;
		
		/* add to all backends... */
		LDAP_STAILQ_FOREACH( bd, &backendDB, be_next ) {
			if ( bd == be->bd_self ) {
				gotit = 1;
			}

			/* overlays can be instanciated multiple times, use
			 * be_ctrls[ cid ] as an instance counter, so that the
			 * overlay's controls are only really disabled after the
			 * last instance called overlay_register_control() */
			bd->be_ctrls[ cid ]++;
			bd->be_ctrls[ SLAP_MAX_CIDS ] = 1;
		}

	}
	
	if ( !gotit ) {
		/* overlays can be instanciated multiple times, use
		 * be_ctrls[ cid ] as an instance counter, so that the
		 * overlay's controls are only really unregistered after the
		 * last instance called overlay_register_control() */
		be->bd_self->be_ctrls[ cid ]++;
		be->bd_self->be_ctrls[ SLAP_MAX_CIDS ] = 1;
	}

	return 0;
}

#ifdef SLAP_CONFIG_DELETE
void
overlay_unregister_control( BackendDB *be, const char *oid )
{
	int		gotit = 0;
	int		cid;

	if ( slap_find_control_id( oid, &cid ) == LDAP_CONTROL_NOT_FOUND ) {
		return;
	}

	if ( SLAP_ISGLOBALOVERLAY( be ) ) {
		BackendDB *bd;

		/* remove from all backends... */
		LDAP_STAILQ_FOREACH( bd, &backendDB, be_next ) {
			if ( bd == be->bd_self ) {
				gotit = 1;
			}

			bd->be_ctrls[ cid ]--;
		}
	}

	if ( !gotit ) {
		be->bd_self->be_ctrls[ cid ]--;
	}
}
#endif /* SLAP_CONFIG_DELETE */

void
overlay_destroy_one( BackendDB *be, slap_overinst *on )
{
	slap_overinfo *oi = on->on_info;
	slap_overinst **oidx;

	for ( oidx = &oi->oi_list; *oidx; oidx = &(*oidx)->on_next ) {
		if ( *oidx == on ) {
			*oidx = on->on_next;
			if ( on->on_bi.bi_db_destroy ) {
				BackendInfo *bi_orig = be->bd_info;
				be->bd_info = (BackendInfo *)on;
				on->on_bi.bi_db_destroy( be, NULL );
				be->bd_info = bi_orig;
			}
			free( on );
			break;
		}
	}
}

#ifdef SLAP_CONFIG_DELETE
typedef struct ov_remove_ctx {
	BackendDB *be;
	slap_overinst *on;
} ov_remove_ctx;

int
overlay_remove_cb( Operation *op, SlapReply *rs )
{
	ov_remove_ctx *rm_ctx = (ov_remove_ctx*) op->o_callback->sc_private;
	BackendInfo *bi_orig = rm_ctx->be->bd_info;

	rm_ctx->be->bd_info = (BackendInfo*) rm_ctx->on;

	if ( rm_ctx->on->on_bi.bi_db_close ) {
		rm_ctx->on->on_bi.bi_db_close( rm_ctx->be, NULL );
	}
	if ( rm_ctx->on->on_bi.bi_db_destroy ) {
		rm_ctx->on->on_bi.bi_db_destroy( rm_ctx->be, NULL );
	}

	/* clean up after removing last overlay */
	if ( ! rm_ctx->on->on_info->oi_list ) {
		/* reset db flags and bd_info to orig */
		SLAP_DBFLAGS( rm_ctx->be ) &= ~SLAP_DBFLAG_GLOBAL_OVERLAY;
		rm_ctx->be->bd_info = rm_ctx->on->on_info->oi_orig;
		ch_free(rm_ctx->on->on_info);
	} else {
		rm_ctx->be->bd_info = bi_orig;
	}
	free( rm_ctx->on );
	op->o_tmpfree(rm_ctx, op->o_tmpmemctx);
	return SLAP_CB_CONTINUE;
}

void
overlay_remove( BackendDB *be, slap_overinst *on, Operation *op )
{
	slap_overinfo *oi = on->on_info;
	slap_overinst **oidx;
	ov_remove_ctx *rm_ctx;
	slap_callback *rm_cb, *cb;

	/* remove overlay from oi_list */
	for ( oidx = &oi->oi_list; *oidx; oidx = &(*oidx)->on_next ) {
		if ( *oidx == on ) {
			*oidx = on->on_next;
			break;
		}
	}

	/* The db_close and db_destroy handlers to cleanup a release
	 * the overlay's resources are called from the cleanup callback
	 */
	rm_ctx = op->o_tmpalloc( sizeof( ov_remove_ctx ), op->o_tmpmemctx );
	rm_ctx->be = be;
	rm_ctx->on = on;

	rm_cb = op->o_tmpalloc( sizeof( slap_callback ), op->o_tmpmemctx );
	rm_cb->sc_next = NULL;
	rm_cb->sc_cleanup = overlay_remove_cb;
	rm_cb->sc_response = NULL;
	rm_cb->sc_private = (void*) rm_ctx;

	/* Append callback to the end of the list */
	if ( !op->o_callback ) {
		op->o_callback = rm_cb;
	} else {
		for ( cb = op->o_callback; cb->sc_next; cb = cb->sc_next );
		cb->sc_next = rm_cb;
	}
}
#endif /* SLAP_CONFIG_DELETE */

void
overlay_insert( BackendDB *be, slap_overinst *on2, slap_overinst ***prev,
	int idx )
{
	slap_overinfo *oi = (slap_overinfo *)be->bd_info;

	if ( idx == -1 ) {
		on2->on_next = oi->oi_list;
		oi->oi_list = on2;
	} else {
		int i, novs;
		slap_overinst *on, **prev;

		/* Since the list is in reverse order and is singly linked,
		 * we have to count the overlays and then insert backwards.
		 * Adding on overlay at a specific point should be a pretty
		 * infrequent occurrence.
		 */
		novs = 0;
		for ( on = oi->oi_list; on; on=on->on_next )
			novs++;

		if (idx > novs)
			idx = 0;
		else
			idx = novs - idx;

		/* advance to insertion point */
		prev = &oi->oi_list;
		for ( i=0; i<idx; i++ ) {
			on = *prev;
			prev = &on->on_next;
		}
		/* insert */
		on2->on_next = *prev;
		*prev = on2;
	}
}

void
overlay_move( BackendDB *be, slap_overinst *on, int idx )
{
	slap_overinfo *oi = (slap_overinfo *)be->bd_info;
	slap_overinst **onp;

	for (onp = &oi->oi_list; *onp; onp= &(*onp)->on_next) {
		if ( *onp == on ) {
			*onp = on->on_next;
			break;
		}
	}
	overlay_insert( be, on, &onp, idx );
}

/* add an overlay to a particular backend. */
int
overlay_config( BackendDB *be, const char *ov, int idx, BackendInfo **res, ConfigReply *cr )
{
	slap_overinst *on = NULL, *on2 = NULL, **prev;
	slap_overinfo *oi = NULL;
	BackendInfo *bi = NULL;

	if ( res )
		*res = NULL;

	on = overlay_find( ov );
	if ( !on ) {
		Debug( LDAP_DEBUG_ANY, "overlay \"%s\" not found\n", ov, 0, 0 );
		return 1;
	}

	/* If this is the first overlay on this backend, set up the
	 * overlay info structure
	 */
	if ( !overlay_is_over( be ) ) {
		int	isglobal = 0;

		/* NOTE: the first time a global overlay is configured,
		 * frontendDB gets this flag; it is used later by overlays
		 * to determine if they're stacked on top of the frontendDB */
		if ( be->bd_info == frontendDB->bd_info || SLAP_ISGLOBALOVERLAY( be ) ) {
			isglobal = 1;
			if ( on->on_bi.bi_flags & SLAPO_BFLAG_DBONLY ) {
				Debug( LDAP_DEBUG_ANY, "overlay_config(): "
					"overlay \"%s\" cannot be global.\n",
					ov, 0, 0 );
				return 1;
			}

		} else if ( on->on_bi.bi_flags & SLAPO_BFLAG_GLOBONLY ) {
			Debug( LDAP_DEBUG_ANY, "overlay_config(): "
				"overlay \"%s\" can only be global.\n",
				ov, 0, 0 );
			return 1;
		}

		oi = ch_malloc( sizeof( slap_overinfo ) );
		oi->oi_orig = be->bd_info;
		oi->oi_bi = *be->bd_info;
		oi->oi_origdb = be;

		if ( isglobal ) {
			SLAP_DBFLAGS( be ) |= SLAP_DBFLAG_GLOBAL_OVERLAY;
		}

		/* Save a pointer to ourself in bi_private.
		 */
		oi->oi_bi.bi_private = oi;
		oi->oi_list = NULL;
		bi = (BackendInfo *)oi;

		bi->bi_type = (char *)overtype;

		bi->bi_db_config = over_db_config;
		bi->bi_db_open = over_db_open;
		bi->bi_db_close = over_db_close;
		bi->bi_db_destroy = over_db_destroy;

		bi->bi_op_bind = over_op_bind;
		bi->bi_op_unbind = over_op_unbind;
		bi->bi_op_search = over_op_search;
		bi->bi_op_compare = over_op_compare;
		bi->bi_op_modify = over_op_modify;
		bi->bi_op_modrdn = over_op_modrdn;
		bi->bi_op_add = over_op_add;
		bi->bi_op_delete = over_op_delete;
		bi->bi_op_abandon = over_op_abandon;
		bi->bi_op_cancel = over_op_cancel;

		bi->bi_extended = over_op_extended;

		/*
		 * this is fine because it has the same
		 * args of the operations; we need to rework
		 * all the hooks to share the same args
		 * of the operations...
		 */
		bi->bi_operational = over_aux_operational;
		bi->bi_chk_referrals = over_aux_chk_referrals;
		bi->bi_chk_controls = over_aux_chk_controls;

		/* these have specific arglists */
		bi->bi_entry_get_rw = over_entry_get_rw;
		bi->bi_entry_release_rw = over_entry_release_rw;
		bi->bi_access_allowed = over_access_allowed;
		bi->bi_acl_group = over_acl_group;
		bi->bi_acl_attribute = over_acl_attribute;
		
		bi->bi_connection_init = over_connection_init;
		bi->bi_connection_destroy = over_connection_destroy;

		be->bd_info = bi;

	} else {
		if ( overlay_is_inst( be, ov ) ) {
			if ( SLAPO_SINGLE( be ) ) {
				Debug( LDAP_DEBUG_ANY, "overlay_config(): "
					"overlay \"%s\" already in list\n",
					ov, 0, 0 );
				return 1;
			}
		}

		oi = be->bd_info->bi_private;
	}

	/* Insert new overlay into list. By default overlays are
	 * added to head of list and executed in LIFO order.
	 */
	on2 = ch_calloc( 1, sizeof(slap_overinst) );
	*on2 = *on;
	on2->on_info = oi;

	prev = &oi->oi_list;
	/* Do we need to find the insertion point? */
	if ( idx >= 0 ) {
		int i;

		/* count current overlays */
		for ( i=0, on=oi->oi_list; on; on=on->on_next, i++ );

		/* are we just appending a new one? */
		if ( idx >= i )
			idx = -1;
	}
	overlay_insert( be, on2, &prev, idx );

	/* Any initialization needed? */
	if ( on2->on_bi.bi_db_init ) {
		int rc;
		be->bd_info = (BackendInfo *)on2;
		rc = on2->on_bi.bi_db_init( be, cr);
		be->bd_info = (BackendInfo *)oi;
		if ( rc ) {
			*prev = on2->on_next;
			ch_free( on2 );
			on2 = NULL;
			return rc;
		}
	}

	if ( res )
		*res = &on2->on_bi;

	return 0;
}
