/* denyop.c - Denies operations */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Pierangelo Masarati for inclusion in
 * OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_DENYOP

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"

/* This overlay provides a quick'n'easy way to deny selected operations
 * for a database whose backend implements the operations.  It is intended
 * to be less expensive than ACLs because its evaluation occurs before
 * any backend specific operation is actually even initiated.
 */

enum {
	denyop_add = 0,
	denyop_bind,
	denyop_compare,
	denyop_delete,
	denyop_extended,
	denyop_modify,
	denyop_modrdn,
	denyop_search,
	denyop_unbind
} denyop_e;

typedef struct denyop_info {
	int do_op[denyop_unbind + 1];
} denyop_info;

static int
denyop_func( Operation *op, SlapReply *rs )
{
	slap_overinst		*on = (slap_overinst *) op->o_bd->bd_info;
	denyop_info		*oi = (denyop_info *)on->on_bi.bi_private;
	int			deny = 0;

	switch( op->o_tag ) {
	case LDAP_REQ_BIND:
		deny = oi->do_op[denyop_bind];
		break;

	case LDAP_REQ_ADD:
		deny = oi->do_op[denyop_add];
		break;

	case LDAP_REQ_DELETE:
		deny = oi->do_op[denyop_delete];
		break;

	case LDAP_REQ_MODRDN:
		deny = oi->do_op[denyop_modrdn];
		break;

	case LDAP_REQ_MODIFY:
		deny = oi->do_op[denyop_modify];
		break;

	case LDAP_REQ_COMPARE:
		deny = oi->do_op[denyop_compare];
		break;

	case LDAP_REQ_SEARCH:
		deny = oi->do_op[denyop_search];
		break;

	case LDAP_REQ_EXTENDED:
		deny = oi->do_op[denyop_extended];
		break;

	case LDAP_REQ_UNBIND:
		deny = oi->do_op[denyop_unbind];
		break;
	}

	if ( !deny ) {
		return SLAP_CB_CONTINUE;
	}

	op->o_bd->bd_info = (BackendInfo *)on->on_info;
	send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"operation not allowed within namingContext" );

	return 0;
}

static int
denyop_over_init(
	BackendDB *be
)
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	denyop_info		*oi;

	oi = (denyop_info *)ch_malloc(sizeof(denyop_info));
	memset(oi, 0, sizeof(denyop_info));
	on->on_bi.bi_private = oi;

	return 0;
}

static int
denyop_config(
    BackendDB	*be,
    const char	*fname,
    int		lineno,
    int		argc,
    char	**argv
)
{
	slap_overinst		*on = (slap_overinst *) be->bd_info;
	denyop_info		*oi = (denyop_info *)on->on_bi.bi_private;

	if ( strcasecmp( argv[0], "denyop" ) == 0 ) {
		char *op;

		if ( argc != 2 ) {
			Debug( LDAP_DEBUG_ANY, "%s: line %d: "
				"operation list missing in "
				"\"denyop <op-list>\" line.\n",
				fname, lineno, 0 );
			return( 1 );
		}

		/* The on->on_bi.bi_private pointer can be used for
		 * anything this instance of the overlay needs.
		 */

		op = argv[1];
		do {
			char	*next = strchr( op, ',' );

			if ( next ) {
				next[0] = '\0';
				next++;
			}

			if ( strcmp( op, "add" ) == 0 ) {
				oi->do_op[denyop_add] = 1;

			} else if ( strcmp( op, "bind" ) == 0 ) {
				oi->do_op[denyop_bind] = 1;

			} else if ( strcmp( op, "compare" ) == 0 ) {
				oi->do_op[denyop_compare] = 1;

			} else if ( strcmp( op, "delete" ) == 0 ) {
				oi->do_op[denyop_delete] = 1;

			} else if ( strcmp( op, "extended" ) == 0 ) {
				oi->do_op[denyop_extended] = 1;

			} else if ( strcmp( op, "modify" ) == 0 ) {
				oi->do_op[denyop_modify] = 1;

			} else if ( strcmp( op, "modrdn" ) == 0 ) {
				oi->do_op[denyop_modrdn] = 1;

			} else if ( strcmp( op, "search" ) == 0 ) {
				oi->do_op[denyop_search] = 1;

			} else if ( strcmp( op, "unbind" ) == 0 ) {
				oi->do_op[denyop_unbind] = 1;

			} else {
				Debug( LDAP_DEBUG_ANY, "%s: line %d: "
					"unknown operation \"%s\" at "
					"\"denyop <op-list>\" line.\n",
					fname, lineno, op );
				return( 1 );
			}

			op = next;
		} while ( op );

	} else {
		return SLAP_CONF_UNKNOWN;
	}
	return 0;
}

static int
denyop_destroy(
	BackendDB *be
)
{
	slap_overinst	*on = (slap_overinst *) be->bd_info;
	denyop_info	*oi = (denyop_info *)on->on_bi.bi_private;

	if ( oi ) {
		ch_free( oi );
	}

	return 0;
}

/* This overlay is set up for dynamic loading via moduleload. For static
 * configuration, you'll need to arrange for the slap_overinst to be
 * initialized and registered by some other function inside slapd.
 */

static slap_overinst denyop;

int
denyop_initialize( void )
{
	memset( &denyop, 0, sizeof( slap_overinst ) );
	denyop.on_bi.bi_type = "denyop";
	denyop.on_bi.bi_db_init = denyop_over_init;
	denyop.on_bi.bi_db_config = denyop_config;
	denyop.on_bi.bi_db_destroy = denyop_destroy;

	denyop.on_bi.bi_op_bind = denyop_func;
	denyop.on_bi.bi_op_search = denyop_func;
	denyop.on_bi.bi_op_compare = denyop_func;
	denyop.on_bi.bi_op_modify = denyop_func;
	denyop.on_bi.bi_op_modrdn = denyop_func;
	denyop.on_bi.bi_op_add = denyop_func;
	denyop.on_bi.bi_op_delete = denyop_func;
	denyop.on_bi.bi_extended = denyop_func;
	denyop.on_bi.bi_op_unbind = denyop_func;

	denyop.on_response = NULL /* denyop_response */ ;

	return overlay_register( &denyop );
}

#if SLAPD_OVER_DENYOP == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return denyop_initialize();
}
#endif /* SLAPD_OVER_DENYOP == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_DENYOP) */
