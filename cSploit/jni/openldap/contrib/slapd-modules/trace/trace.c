/* trace.c - traces overlay invocation */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2006-2014 The OpenLDAP Foundation.
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

#ifdef SLAPD_OVER_TRACE

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "lutil.h"

static int
trace_op2str( Operation *op, char **op_strp )
{
	switch ( op->o_tag ) {
	case LDAP_REQ_BIND:
		*op_strp = "BIND";
		break;

	case LDAP_REQ_UNBIND:
		*op_strp = "UNBIND";
		break;

	case LDAP_REQ_SEARCH:
		*op_strp = "SEARCH";
		break;

	case LDAP_REQ_MODIFY:
		*op_strp = "MODIFY";
		break;

	case LDAP_REQ_ADD:
		*op_strp = "ADD";
		break;

	case LDAP_REQ_DELETE:
		*op_strp = "DELETE";
		break;

	case LDAP_REQ_MODRDN:
		*op_strp = "MODRDN";
		break;

	case LDAP_REQ_COMPARE:
		*op_strp = "COMPARE";
		break;

	case LDAP_REQ_ABANDON:
		*op_strp = "ABANDON";
		break;

	case LDAP_REQ_EXTENDED:
		*op_strp = "EXTENDED";
		break;

	default:
		assert( 0 );
	}

	return 0;
}

static int
trace_op_func( Operation *op, SlapReply *rs )
{
	char	*op_str = NULL;

	(void)trace_op2str( op, &op_str );

	switch ( op->o_tag ) {
	case LDAP_REQ_EXTENDED:
		Log3( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
			"%s trace op=EXTENDED dn=\"%s\" reqoid=%s\n",
			op->o_log_prefix, 
			BER_BVISNULL( &op->o_req_ndn ) ? "(null)" : op->o_req_ndn.bv_val,
			BER_BVISNULL( &op->ore_reqoid ) ? "" : op->ore_reqoid.bv_val );
		break;

	default:
		Log3( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
			"%s trace op=%s dn=\"%s\"\n",
			op->o_log_prefix, op_str,
			BER_BVISNULL( &op->o_req_ndn ) ? "(null)" : op->o_req_ndn.bv_val );
		break;
	}

	return SLAP_CB_CONTINUE;
}

static int
trace_response( Operation *op, SlapReply *rs )
{
	char	*op_str = NULL;

	(void)trace_op2str( op, &op_str );

	switch ( op->o_tag ) {
	case LDAP_REQ_EXTENDED:
		Log5( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
			"%s trace op=EXTENDED RESPONSE dn=\"%s\" reqoid=%s rspoid=%s err=%d\n",
			op->o_log_prefix,
			BER_BVISNULL( &op->o_req_ndn ) ? "(null)" : op->o_req_ndn.bv_val,
			BER_BVISNULL( &op->ore_reqoid ) ? "" : op->ore_reqoid.bv_val,
			rs->sr_rspoid == NULL ? "" : rs->sr_rspoid,
			rs->sr_err );
		break;

	case LDAP_REQ_SEARCH:
		switch ( rs->sr_type ) {
		case REP_SEARCH:
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
				"%s trace op=SEARCH ENTRY dn=\"%s\"\n",
				op->o_log_prefix,
				rs->sr_entry->e_name.bv_val );
			goto done;

		case REP_SEARCHREF:
			Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
				"%s trace op=SEARCH REFERENCE ref=\"%s\"\n",
				op->o_log_prefix,
				rs->sr_ref[ 0 ].bv_val );
			goto done;

		case REP_RESULT:
			break;

		default:
			assert( 0 );
		}
		/* fallthru */

	default:
		Log4( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
			"%s trace op=%s RESPONSE dn=\"%s\" err=%d\n",
			op->o_log_prefix,
			op_str,
			BER_BVISNULL( &op->o_req_ndn ) ? "(null)" : op->o_req_ndn.bv_val,
			rs->sr_err );
		break;
	}

done:;
	return SLAP_CB_CONTINUE;
}

static int
trace_db_init(
	BackendDB *be )
{
	Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
		"trace DB_INIT\n" );

	return 0;
}

static int
trace_db_config(
	BackendDB	*be,
	const char	*fname,
	int		lineno,
	int		argc,
	char		**argv )
{
	Log2( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
		"trace DB_CONFIG argc=%d argv[0]=\"%s\"\n",
		argc, argv[ 0 ] );

	return 0;
}

static int
trace_db_open(
	BackendDB *be )
{
	Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
		"trace DB_OPEN\n" );

	return 0;
}

static int
trace_db_close(
	BackendDB *be )
{
	Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
		"trace DB_CLOSE\n" );

	return 0;
}

static int
trace_db_destroy(
	BackendDB *be )
{
	Log0( LDAP_DEBUG_ANY, LDAP_LEVEL_INFO,
		"trace DB_DESTROY\n" );

	return 0;
}

static slap_overinst 		trace;

int
trace_initialize()
{
	trace.on_bi.bi_type = "trace";

	trace.on_bi.bi_db_init = trace_db_init;
	trace.on_bi.bi_db_open = trace_db_open;
	trace.on_bi.bi_db_config = trace_db_config;
	trace.on_bi.bi_db_close = trace_db_close;
	trace.on_bi.bi_db_destroy = trace_db_destroy;

	trace.on_bi.bi_op_add = trace_op_func;
	trace.on_bi.bi_op_bind = trace_op_func;
	trace.on_bi.bi_op_unbind = trace_op_func;
	trace.on_bi.bi_op_compare = trace_op_func;
	trace.on_bi.bi_op_delete = trace_op_func;
	trace.on_bi.bi_op_modify = trace_op_func;
	trace.on_bi.bi_op_modrdn = trace_op_func;
	trace.on_bi.bi_op_search = trace_op_func;
	trace.on_bi.bi_op_abandon = trace_op_func;
	trace.on_bi.bi_extended = trace_op_func;

	trace.on_response = trace_response;

	return overlay_register( &trace );
}

#if SLAPD_OVER_TRACE == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return trace_initialize();
}
#endif /* SLAPD_OVER_TRACE == SLAPD_MOD_DYNAMIC */

#endif /* defined(SLAPD_OVER_TRACE) */
