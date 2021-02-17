/* config.c - sock backend configuration file routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software. Dynamic config support by Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"
#include "back-sock.h"

static ConfigDriver bs_cf_gen;
static int sock_over_setup();
static slap_response sock_over_response;

enum {
	BS_EXT = 1,
	BS_OPS,
	BS_RESP
};

/* The number of overlay-only config attrs */
#define NUM_OV_ATTRS	2

static ConfigTable bscfg[] = {
	{ "sockops", "ops", 2, 0, 0, ARG_MAGIC|BS_OPS,
		bs_cf_gen, "( OLcfgDbAt:7.3 NAME 'olcOvSocketOps' "
			"DESC 'Operation types to forward' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ "sockresps", "resps", 2, 0, 0, ARG_MAGIC|BS_RESP,
		bs_cf_gen, "( OLcfgDbAt:7.4 NAME 'olcOvSocketResps' "
			"DESC 'Response types to forward' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },

	{ "socketpath", "pathname", 2, 2, 0, ARG_STRING|ARG_OFFSET,
		(void *)offsetof(struct sockinfo, si_sockpath),
		"( OLcfgDbAt:7.1 NAME 'olcDbSocketPath' "
			"DESC 'Pathname for Unix domain socket' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ "extensions", "ext", 2, 0, 0, ARG_MAGIC|BS_EXT,
		bs_cf_gen, "( OLcfgDbAt:7.2 NAME 'olcDbSocketExtensions' "
			"DESC 'binddn, peername, or ssf' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL }
};

static ConfigOCs bsocs[] = {
	{ "( OLcfgDbOc:7.1 "
		"NAME 'olcDbSocketConfig' "
		"DESC 'Socket backend configuration' "
		"SUP olcDatabaseConfig "
		"MUST olcDbSocketPath "
		"MAY olcDbSocketExtensions )",
			Cft_Database, bscfg+NUM_OV_ATTRS },
	{ NULL, 0, NULL }
};

static ConfigOCs osocs[] = {
	{ "( OLcfgDbOc:7.2 "
		"NAME 'olcOvSocketConfig' "
		"DESC 'Socket overlay configuration' "
		"SUP olcOverlayConfig "
		"MUST olcDbSocketPath "
		"MAY ( olcDbSocketExtensions $ "
			" olcOvSocketOps $ olcOvSocketResps ) )",
			Cft_Overlay, bscfg },
	{ NULL, 0, NULL }
};

#define SOCK_OP_BIND	0x001
#define SOCK_OP_UNBIND	0x002
#define SOCK_OP_SEARCH	0x004
#define SOCK_OP_COMPARE	0x008
#define SOCK_OP_MODIFY	0x010
#define SOCK_OP_MODRDN	0x020
#define SOCK_OP_ADD		0x040
#define SOCK_OP_DELETE	0x080

#define SOCK_REP_RESULT	0x001
#define SOCK_REP_SEARCH	0x002

static slap_verbmasks bs_exts[] = {
	{ BER_BVC("binddn"), SOCK_EXT_BINDDN },
	{ BER_BVC("peername"), SOCK_EXT_PEERNAME },
	{ BER_BVC("ssf"), SOCK_EXT_SSF },
	{ BER_BVC("connid"), SOCK_EXT_CONNID },
	{ BER_BVNULL, 0 }
};

static slap_verbmasks ov_ops[] = {
	{ BER_BVC("bind"), SOCK_OP_BIND },
	{ BER_BVC("unbind"), SOCK_OP_UNBIND },
	{ BER_BVC("search"), SOCK_OP_SEARCH },
	{ BER_BVC("compare"), SOCK_OP_COMPARE },
	{ BER_BVC("modify"), SOCK_OP_MODIFY },
	{ BER_BVC("modrdn"), SOCK_OP_MODRDN },
	{ BER_BVC("add"), SOCK_OP_ADD },
	{ BER_BVC("delete"), SOCK_OP_DELETE },
	{ BER_BVNULL, 0 }
};

static slap_verbmasks ov_resps[] = {
	{ BER_BVC("result"), SOCK_REP_RESULT },
	{ BER_BVC("search"), SOCK_REP_SEARCH },
	{ BER_BVNULL, 0 }
};

static int
bs_cf_gen( ConfigArgs *c )
{
	struct sockinfo	*si;
	int rc;

	if ( c->be && c->table == Cft_Database )
		si = c->be->be_private;
	else if ( c->bi )
		si = c->bi->bi_private;
	else
		return ARG_BAD_CONF;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		switch( c->type ) {
		case BS_EXT:
			return mask_to_verbs( bs_exts, si->si_extensions, &c->rvalue_vals );
		case BS_OPS:
			return mask_to_verbs( ov_ops, si->si_ops, &c->rvalue_vals );
		case BS_RESP:
			return mask_to_verbs( ov_resps, si->si_resps, &c->rvalue_vals );
		}
	} else if ( c->op == LDAP_MOD_DELETE ) {
		switch( c->type ) {
		case BS_EXT:
			if ( c->valx < 0 ) {
				si->si_extensions = 0;
				rc = 0;
			} else {
				slap_mask_t dels = 0;
				rc = verbs_to_mask( c->argc, c->argv, bs_exts, &dels );
				if ( rc == 0 )
					si->si_extensions ^= dels;
			}
			return rc;
		case BS_OPS:
			if ( c->valx < 0 ) {
				si->si_ops = 0;
				rc = 0;
			} else {
				slap_mask_t dels = 0;
				rc = verbs_to_mask( c->argc, c->argv, ov_ops, &dels );
				if ( rc == 0 )
					si->si_ops ^= dels;
			}
			return rc;
		case BS_RESP:
			if ( c->valx < 0 ) {
				si->si_resps = 0;
				rc = 0;
			} else {
				slap_mask_t dels = 0;
				rc = verbs_to_mask( c->argc, c->argv, ov_resps, &dels );
				if ( rc == 0 )
					si->si_resps ^= dels;
			}
			return rc;
		}

	} else {
		switch( c->type ) {
		case BS_EXT:
			return verbs_to_mask( c->argc, c->argv, bs_exts, &si->si_extensions );
		case BS_OPS:
			return verbs_to_mask( c->argc, c->argv, ov_ops, &si->si_ops );
		case BS_RESP:
			return verbs_to_mask( c->argc, c->argv, ov_resps, &si->si_resps );
		}
	}
	return 1;
}

int
sock_back_init_cf( BackendInfo *bi )
{
	int rc;
	bi->bi_cf_ocs = bsocs;

	rc = config_register_schema( bscfg, bsocs );
	if ( !rc )
		rc = sock_over_setup();
	return rc;
}

/* sock overlay wrapper */
static slap_overinst sockover;

static int sock_over_db_init( Backend *be, struct config_reply_s *cr );
static int sock_over_db_destroy( Backend *be, struct config_reply_s *cr );

static BI_op_bind *sockfuncs[] = {
	sock_back_bind,
	sock_back_unbind,
	sock_back_search,
	sock_back_compare,
	sock_back_modify,
	sock_back_modrdn,
	sock_back_add,
	sock_back_delete
};

static const int sockopflags[] = {
	SOCK_OP_BIND,
	SOCK_OP_UNBIND,
	SOCK_OP_SEARCH,
	SOCK_OP_COMPARE,
	SOCK_OP_MODIFY,
	SOCK_OP_MODRDN,
	SOCK_OP_ADD,
	SOCK_OP_DELETE
};

static int sock_over_op(
	Operation *op,
	SlapReply *rs
)
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	void *private = op->o_bd->be_private;
	slap_callback *sc;
	struct sockinfo	*si;
	slap_operation_t which;

	switch (op->o_tag) {
	case LDAP_REQ_BIND:	which = op_bind; break;
	case LDAP_REQ_UNBIND:	which = op_unbind; break;
	case LDAP_REQ_SEARCH:	which = op_search; break;
	case LDAP_REQ_COMPARE:	which = op_compare; break;
	case LDAP_REQ_MODIFY:	which = op_modify; break;
	case LDAP_REQ_MODRDN:	which = op_modrdn; break;
	case LDAP_REQ_ADD:	which = op_add; break;
	case LDAP_REQ_DELETE:	which = op_delete; break;
	default:
		return SLAP_CB_CONTINUE;
	}
	si = on->on_bi.bi_private;
	if ( !(si->si_ops & sockopflags[which]))
		return SLAP_CB_CONTINUE;

	op->o_bd->be_private = si;
	sc = op->o_callback;
	op->o_callback = NULL;
	sockfuncs[which]( op, rs );
	op->o_bd->be_private = private;
	op->o_callback = sc;
	return rs->sr_err;
}

static int
sock_over_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	struct sockinfo *si = (struct sockinfo *)on->on_bi.bi_private;
	FILE *fp;

	if ( rs->sr_type == REP_RESULT ) {
		if ( !( si->si_resps & SOCK_REP_RESULT ))
			return SLAP_CB_CONTINUE;
	} else if ( rs->sr_type == REP_SEARCH ) {
		if ( !( si->si_resps & SOCK_REP_SEARCH ))
			return SLAP_CB_CONTINUE;
	} else
		return SLAP_CB_CONTINUE;

	if (( fp = opensock( si->si_sockpath )) == NULL )
		return SLAP_CB_CONTINUE;

	if ( rs->sr_type == REP_RESULT ) {
		/* write out the result */
		fprintf( fp, "RESULT\n" );
		fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
		sock_print_conn( fp, op->o_conn, si );
		fprintf( fp, "code: %d\n", rs->sr_err );
		if ( rs->sr_matched )
			fprintf( fp, "matched: %s\n", rs->sr_matched );
		if (rs->sr_text )
			fprintf( fp, "info: %s\n", rs->sr_text );
	} else {
		/* write out the search entry */
		int len;
		fprintf( fp, "ENTRY\n" );
		fprintf( fp, "msgid: %ld\n", (long) op->o_msgid );
		sock_print_conn( fp, op->o_conn, si );
		ldap_pvt_thread_mutex_lock( &entry2str_mutex );
		fprintf( fp, "%s", entry2str( rs->sr_entry, &len ) );
		ldap_pvt_thread_mutex_unlock( &entry2str_mutex );
	}
	fprintf( fp, "\n" );
	fclose( fp );

	return SLAP_CB_CONTINUE;
}

static int
sock_over_setup()
{
	int rc;

	sockover.on_bi.bi_type = "sock";
	sockover.on_bi.bi_db_init = sock_over_db_init;
	sockover.on_bi.bi_db_destroy = sock_over_db_destroy;

	sockover.on_bi.bi_op_bind = sock_over_op;
	sockover.on_bi.bi_op_unbind = sock_over_op;
	sockover.on_bi.bi_op_search = sock_over_op;
	sockover.on_bi.bi_op_compare = sock_over_op;
	sockover.on_bi.bi_op_modify = sock_over_op;
	sockover.on_bi.bi_op_modrdn = sock_over_op;
	sockover.on_bi.bi_op_add = sock_over_op;
	sockover.on_bi.bi_op_delete = sock_over_op;
	sockover.on_response = sock_over_response;

	sockover.on_bi.bi_cf_ocs = osocs;

	rc = config_register_schema( bscfg, osocs );
	if ( rc ) return rc;

	return overlay_register( &sockover );
}

static int
sock_over_db_init(
    Backend	*be,
	struct config_reply_s *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	void *private = be->be_private;
	int rc;

	be->be_private = NULL;
	rc = sock_back_db_init( be, cr );
	on->on_bi.bi_private = be->be_private;
	be->be_private = private;
	return rc;
}

static int
sock_over_db_destroy(
    Backend	*be,
	struct config_reply_s *cr
)
{
	slap_overinst	*on = (slap_overinst *)be->bd_info;
	void *private = be->be_private;
	int rc;

	be->be_private = on->on_bi.bi_private;
	rc = sock_back_db_destroy( be, cr );
	on->on_bi.bi_private = be->be_private;
	be->be_private = private;
	return rc;
}
