/* nops.c - Overlay to filter idempotent operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>. 
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Copyright 2008 Emmanuel Dreyfus.
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
 * This work was originally developed by the Emmanuel Dreyfus for
 * inclusion in OpenLDAP Software.
 */
#include "portable.h"

#ifdef SLAPD_OVER_NOPS

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "config.h"

static ConfigDriver nops_cf_gen;

static int nops_cf_gen( ConfigArgs *c ) { return 0; }

static void
nops_rm_mod( Modifications **mods, Modifications *mod ) {
	Modifications *next, *m;

	next = mod->sml_next;
	if (*mods == mod) {
		*mods = next;
	} else {
		Modifications *m;

		for (m = *mods; m; m = m->sml_next) {
			if (m->sml_next == mod) {
				m->sml_next = next;
				break;
			}
		}
	}

	mod->sml_next = NULL;
	slap_mods_free(mod, 1);

	return;
}

static int
nops_modify( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	Backend *be = op->o_bd;
	Entry *target_entry = NULL;
	Modifications *m;
	int rc;
	
	if ((m = op->orm_modlist) == NULL) {
		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		send_ldap_error(op, rs, LDAP_INVALID_SYNTAX,
				"nops() got null orm_modlist");
		return(rs->sr_err);
	}

	op->o_bd = on->on_info->oi_origdb;
	rc = be_entry_get_rw(op, &op->o_req_ndn, NULL, NULL, 0,  &target_entry);
	op->o_bd = be;

	if (rc != 0 || target_entry == NULL)
		return 0;
        
	/* 
	 * For each attribute modification, check if the 
	 * modification and the old entry are the same.
	 */
	while (m) {
		int i, j;
		int found;
		Attribute *a;
		BerVarray bm;
		BerVarray bt;
		Modifications *mc;

		mc = m;
		m = m->sml_next;

		/* Check only replace sub-operations */
		if ((mc->sml_op & LDAP_MOD_OP) != LDAP_MOD_REPLACE)
			continue;

		/* If there is no values, skip */
		if (((bm = mc->sml_values ) == NULL ) || (bm[0].bv_val == NULL))
			continue;

		/* If the attribute does not exist in old entry, skip */
		if ((a = attr_find(target_entry->e_attrs, mc->sml_desc)) == NULL)
			continue;
		if ((bt = a->a_vals) == NULL)
			continue;

		/* For each value replaced, do we find it in old entry? */
		found = 0;
		for (i = 0; bm[i].bv_val; i++) {
			for (j = 0; bt[j].bv_val; j++) {
				if (bm[i].bv_len != bt[j].bv_len)
					continue;
				if (memcmp(bm[i].bv_val, bt[j].bv_val, bt[j].bv_len) != 0)
					continue;

				found++;
				break;
			}
		}

		/* Did we find as many values as we had in old entry? */
		if (i != a->a_numvals || found != a->a_numvals)
			continue;

		/* This is a nop, remove it */
		Debug(LDAP_DEBUG_TRACE, "removing nop on %s%s%s",
			a->a_desc->ad_cname.bv_val, "", "");

		nops_rm_mod(&op->orm_modlist, mc);
	}
	if (target_entry) {
		op->o_bd = on->on_info->oi_origdb;
		be_entry_release_r(op, target_entry);
		op->o_bd = be;
	}

	if ((m = op->orm_modlist) == NULL) {
		slap_callback *cb = op->o_callback;

		op->o_bd->bd_info = (BackendInfo *)(on->on_info);
		op->o_callback = NULL;
                send_ldap_error(op, rs, LDAP_SUCCESS, "");
		op->o_callback = cb;

		return (rs->sr_err);
	}

	return SLAP_CB_CONTINUE;
}

static slap_overinst nops_ovl;

#if SLAPD_OVER_NOPS == SLAPD_MOD_DYNAMIC
static
#endif
int
nops_initialize( void ) {
	nops_ovl.on_bi.bi_type = "nops";
	nops_ovl.on_bi.bi_op_modify = nops_modify;
	return overlay_register( &nops_ovl );
}

#if SLAPD_OVER_NOPS == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return nops_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_NOPS) */

