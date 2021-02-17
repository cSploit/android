/* dyngroup.c - Demonstration of overlay code */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Copyright 2003 by Howard Chu.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_DYNGROUP

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"
#include "config.h"

/* This overlay extends the Compare operation to detect members of a
 * dynamic group. It has no effect on any other operations. It must
 * be configured with a pair of attributes to trigger on, e.g.
 *	attrpair member memberURL
 * will cause compares on "member" to trigger a compare on "memberURL".
 */

typedef struct adpair {
	struct adpair *ap_next;
	AttributeDescription *ap_mem;
	AttributeDescription *ap_uri;
} adpair;

static int dgroup_cf( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	int rc = 1;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		{
		adpair *ap;
		for ( ap = on->on_bi.bi_private; ap; ap = ap->ap_next ) {
			struct berval bv;
			char *ptr;
			bv.bv_len = ap->ap_mem->ad_cname.bv_len + 1 +
				ap->ap_uri->ad_cname.bv_len;
			bv.bv_val = ch_malloc( bv.bv_len + 1 );
			ptr = lutil_strcopy( bv.bv_val, ap->ap_mem->ad_cname.bv_val );
			*ptr++ = ' ';
			strcpy( ptr, ap->ap_uri->ad_cname.bv_val );
			ber_bvarray_add( &c->rvalue_vals, &bv );
			rc = 0;
		}
		}
		break;
	case LDAP_MOD_DELETE:
		if ( c->valx == -1 ) {
			adpair *ap;
			while (( ap = on->on_bi.bi_private )) {
				on->on_bi.bi_private = ap->ap_next;
				ch_free( ap );
			}
		} else {
			adpair **app, *ap;
			int i;
			app = (adpair **)&on->on_bi.bi_private;
			for (i=0; i<=c->valx; i++, app = &ap->ap_next) {
				ap = *app;
			}
			*app = ap->ap_next;
			ch_free( ap );
		}
		rc = 0;
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		{
		adpair ap = { NULL, NULL, NULL }, *a2;
		const char *text;
		if ( slap_str2ad( c->argv[1], &ap.ap_mem, &text ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s attribute description unknown: \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		if ( slap_str2ad( c->argv[2], &ap.ap_uri, &text ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s attribute description unknown: \"%s\"",
				c->argv[0], c->argv[2] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}
		/* The on->on_bi.bi_private pointer can be used for
		 * anything this instance of the overlay needs.
		 */
		a2 = ch_malloc( sizeof(adpair) );
		a2->ap_next = on->on_bi.bi_private;
		a2->ap_mem = ap.ap_mem;
		a2->ap_uri = ap.ap_uri;
		on->on_bi.bi_private = a2;
		rc = 0;
		}
	}
	return rc;
}

static ConfigTable dgroupcfg[] = {
	{ "attrpair", "member-attribute> <URL-attribute", 3, 3, 0,
	  ARG_MAGIC, dgroup_cf,
	  "( OLcfgOvAt:17.1 NAME 'olcDGAttrPair' "
	  "DESC 'Member and MemberURL attribute pair' "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs dgroupocs[] = {
	{ "( OLcfgOvOc:17.1 "
	  "NAME 'olcDGConfig' "
	  "DESC 'Dynamic Group configuration' "
	  "SUP olcOverlayConfig "
	  "MAY olcDGAttrPair )",
	  Cft_Overlay, dgroupcfg },
	{ NULL, 0, NULL }
};

static int
dyngroup_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	adpair *ap = on->on_bi.bi_private;

	/* If we've been configured and the current response is
	 * what we're looking for...
	 */
	if ( ap && op->o_tag == LDAP_REQ_COMPARE &&
		rs->sr_err == LDAP_NO_SUCH_ATTRIBUTE ) {

		for (;ap;ap=ap->ap_next) {
			if ( op->oq_compare.rs_ava->aa_desc == ap->ap_mem ) {
				/* This compare is for one of the attributes we're
				 * interested in. We'll use slapd's existing dyngroup
				 * evaluator to get the answer we want.
				 */
				int cache = op->o_do_not_cache;
				
				op->o_do_not_cache = 1;
				rs->sr_err = backend_group( op, NULL, &op->o_req_ndn,
					&op->oq_compare.rs_ava->aa_value, NULL, ap->ap_uri );
				op->o_do_not_cache = cache;
				switch ( rs->sr_err ) {
				case LDAP_SUCCESS:
					rs->sr_err = LDAP_COMPARE_TRUE;
					break;

				case LDAP_NO_SUCH_OBJECT:
					rs->sr_err = LDAP_COMPARE_FALSE;
					break;
				}
				break;
			}
		}
	}
	/* Default is to just fall through to the normal processing */
	return SLAP_CB_CONTINUE;
}

static int
dyngroup_close(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *) be->bd_info;
	adpair *ap, *a2;

	for ( ap = on->on_bi.bi_private; ap; ap = a2 ) {
		a2 = ap->ap_next;
		ch_free( ap );
	}
	return 0;
}

static slap_overinst dyngroup;

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

int dyngroup_initialize() {
	int code;

	dyngroup.on_bi.bi_type = "dyngroup";
	dyngroup.on_bi.bi_db_close = dyngroup_close;
	dyngroup.on_response = dyngroup_response;

	dyngroup.on_bi.bi_cf_ocs = dgroupocs;
	code = config_register_schema( dgroupcfg, dgroupocs );
	if ( code ) return code;

	return overlay_register( &dyngroup );
}

#if SLAPD_OVER_DYNGROUP == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return dyngroup_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_DYNGROUP) */
