/* cloak.c - Overlay to hide some attribute except if explicitely requested */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 Emmanuel Dreyfus
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

#ifdef SLAPD_OVER_CLOAK

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "lutil.h"
#include "slap.h"
#include "config.h"

enum { CLOAK_ATTR = 1 };

typedef struct cloak_info_t {
	ObjectClass 		*ci_oc;	
	AttributeDescription	*ci_ad;
	struct cloak_info_t	*ci_next;
} cloak_info_t;

#define CLOAK_USAGE "\"cloak-attr <attr> [<class>]\": "

static int
cloak_cfgen( ConfigArgs *c )
{
	slap_overinst	*on = (slap_overinst *)c->bi;
	cloak_info_t	*ci = (cloak_info_t *)on->on_bi.bi_private;

	int		rc = 0, i;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case CLOAK_ATTR:
			for ( i = 0; ci; i++, ci = ci->ci_next ) {
				struct berval	bv;
				int len;

				assert( ci->ci_ad != NULL );

				if ( ci->ci_oc != NULL )
					len = snprintf( c->cr_msg, 
					sizeof( c->cr_msg ),
					SLAP_X_ORDERED_FMT "%s %s", i,
					ci->ci_ad->ad_cname.bv_val,
					ci->ci_oc->soc_cname.bv_val );
				else
					len = snprintf( c->cr_msg, 
					sizeof( c->cr_msg ),
					SLAP_X_ORDERED_FMT "%s", i,
					ci->ci_ad->ad_cname.bv_val );

				bv.bv_val = c->cr_msg;
				bv.bv_len = len;
				value_add_one( &c->rvalue_vals, &bv );
			}
			break;

		default:
			rc = 1;
			break;
		}

		return rc;

	} else if ( c->op == LDAP_MOD_DELETE ) {
		cloak_info_t	*ci_next;

		switch( c->type ) {
		case CLOAK_ATTR:
			for ( ci_next = ci, i = 0; 
			      ci_next, c->valx < 0 || i < c->valx; 
			      ci = ci_next, i++ ){

				ci_next = ci->ci_next;

				ch_free ( ci->ci_ad );
				if ( ci->ci_oc != NULL )
					ch_free ( ci->ci_oc );

				ch_free( ci );
			}
			ci = (cloak_info_t *)on->on_bi.bi_private;
			break;

		default:
			rc = 1;
			break;
		}

		return rc;
	}

	switch( c->type ) {
	case CLOAK_ATTR: {
		ObjectClass		*oc = NULL;
		AttributeDescription	*ad = NULL;
		const char		*text;
		cloak_info_t 	       **cip = NULL;
		cloak_info_t 	        *ci_next = NULL;

		if ( c->argc == 3 ) {
			oc = oc_find( c->argv[ 2 ] );
			if ( oc == NULL ) {
				snprintf( c->cr_msg, 
					  sizeof( c->cr_msg ), 
					  CLOAK_USAGE
					  "unable to find ObjectClass \"%s\"",
					  c->argv[ 2 ] );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				       c->log, c->cr_msg, 0 );
				return 1;
			}
		}

		rc = slap_str2ad( c->argv[ 1 ], &ad, &text );
		if ( rc != LDAP_SUCCESS ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), CLOAK_USAGE
				"unable to find AttributeDescription \"%s\"",
				c->argv[ 1 ] );
			Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
				c->log, c->cr_msg, 0 );
			return 1;
		}

		for ( i = 0, cip = (cloak_info_t **)&on->on_bi.bi_private;
		      c->valx < 0 || i < c->valx, *cip;
		      i++, cip = &(*cip)->ci_next ) {
			if ( c->valx >= 0 && *cip == NULL ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ),
					CLOAK_USAGE
					"invalid index {%d}\n",
					c->valx );
				Debug( LDAP_DEBUG_ANY, "%s: %s.\n",
					c->log, c->cr_msg, 0 );
				return 1;
			}
			ci_next = *cip;
		}

		*cip = (cloak_info_t *)SLAP_CALLOC( 1, sizeof( cloak_info_t ) );
		(*cip)->ci_oc = oc;
		(*cip)->ci_ad = ad;
		(*cip)->ci_next = ci_next;

		rc = 0;
		break;
	}

	default:
		rc = 1;
		break;
	}

	return rc;
}

static int
cloak_search_response_cb( Operation *op, SlapReply *rs )
{
	slap_callback   *sc;
	cloak_info_t	*ci;
	Entry		*e = NULL;
	Entry		*me = NULL;

	assert( op && op->o_callback && rs );

	if ( rs->sr_type != REP_SEARCH || !rs->sr_entry ) {
		return ( SLAP_CB_CONTINUE );
	}

	sc = op->o_callback;
	e = rs->sr_entry;

	/* 
	 * First perform a quick scan for an attribute to cloak
	 */
	for ( ci = (cloak_info_t *)sc->sc_private; ci; ci = ci->ci_next ) {
		Attribute *a;

		if ( ci->ci_oc != NULL &&
		     !is_entry_objectclass_or_sub( e, ci->ci_oc ) )
			continue;

		for ( a = e->e_attrs; a; a = a->a_next )
			if ( a->a_desc == ci->ci_ad )
				break;

		if ( a != NULL )
			break;
	}

	/*
	 * Nothing found to cloak
	 */
	if ( ci == NULL )
		return ( SLAP_CB_CONTINUE );

	/*
	 * We are now committed to cloak an attribute.
	 */
	rs_entry2modifiable( op, rs, (slap_overinst *) op->o_bd->bd_info );
	me = rs->sr_entry;
		
	for ( ci = (cloak_info_t *)sc->sc_private; ci; ci = ci->ci_next ) {
		Attribute *a;
		Attribute *pa;

		for ( pa = NULL, a = me->e_attrs;
		      a; 
		      pa = a, a = a->a_next ) {

			if ( a->a_desc != ci->ci_ad )
				continue;

			Debug( LDAP_DEBUG_TRACE, "cloak_search_response_cb: cloak %s\n", 
			       a->a_desc->ad_cname.bv_val,
			       0, 0 );

			if ( pa != NULL ) 
				pa->a_next = a->a_next;
			else
				me->e_attrs = a->a_next;

			attr_clean( a );
		}

	}

	return ( SLAP_CB_CONTINUE );
}

static int
cloak_search_cleanup_cb( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_RESULT || rs->sr_err != LDAP_SUCCESS ) {
		slap_freeself_cb( op, rs );
	}

	return SLAP_CB_CONTINUE;
}

static int
cloak_search( Operation *op, SlapReply *rs )
{
	slap_overinst   *on = (slap_overinst *)op->o_bd->bd_info;
	cloak_info_t    *ci = (cloak_info_t *)on->on_bi.bi_private; 
	slap_callback	*sc;

	if ( op->ors_attrsonly ||
	     op->ors_attrs ||
	     get_manageDSAit( op ) )
		return SLAP_CB_CONTINUE;

	sc = op->o_tmpcalloc( 1, sizeof( *sc ), op->o_tmpmemctx );
	sc->sc_response = cloak_search_response_cb;
	sc->sc_cleanup = cloak_search_cleanup_cb;
	sc->sc_next = op->o_callback;
	sc->sc_private = ci;
	op->o_callback = sc;

	return SLAP_CB_CONTINUE;
}

static slap_overinst cloak_ovl;

static ConfigTable cloakcfg[] = {
	{ "cloak-attr", "attribute [class]",
		2, 3, 0, ARG_MAGIC|CLOAK_ATTR, cloak_cfgen,
		"( OLcfgCtAt:4.1 NAME 'olcCloakAttribute' "
			"DESC 'Cloaked attribute: attribute [class]' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString "
			"X-ORDERED 'VALUES' )",
			NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static int
cloak_db_destroy(
	BackendDB *be,
	ConfigReply *cr )
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	cloak_info_t	*ci = (cloak_info_t *)on->on_bi.bi_private;

	for ( ; ci; ) {
		cloak_info_t *tmp = ci;
		ci = ci->ci_next;
		SLAP_FREE( tmp );
	}

	on->on_bi.bi_private = NULL;

	return 0;
}

static ConfigOCs cloakocs[] = {
	{ "( OLcfgCtOc:4.1 "
	  "NAME 'olcCloakConfig' "
	  "DESC 'Attribute cloak configuration' "
	  "SUP olcOverlayConfig "
	  "MAY ( olcCloakAttribute ) )", 
	  Cft_Overlay, cloakcfg },
	{ NULL, 0, NULL }
};

#if SLAPD_OVER_CLOAK == SLAPD_MOD_DYNAMIC
static
#endif
int
cloak_initialize( void ) {
	int rc;
	cloak_ovl.on_bi.bi_type = "cloak";
	cloak_ovl.on_bi.bi_db_destroy = cloak_db_destroy;
	cloak_ovl.on_bi.bi_op_search = cloak_search;
        cloak_ovl.on_bi.bi_cf_ocs = cloakocs;

	rc = config_register_schema ( cloakcfg, cloakocs );
	if ( rc ) 
		return rc;

	return overlay_register( &cloak_ovl );
}

#if SLAPD_OVER_CLOAK == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return cloak_initialize();
}
#endif

#endif /* defined(SLAPD_OVER_CLOAK) */

