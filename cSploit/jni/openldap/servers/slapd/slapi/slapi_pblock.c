/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2002-2014 The OpenLDAP Foundation.
 * Portions Copyright 1997,2002-2003 IBM Corporation.
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
 * This work was initially developed by IBM Corporation for use in
 * IBM products and subsequently ported to OpenLDAP Software by
 * Steve Omrani.  Additional significant contributors include:
 *   Luke Howard
 */

#include "portable.h"
#include <slap.h>
#include <slapi.h>

#ifdef LDAP_SLAPI

/* some parameters require a valid connection and operation */
#define PBLOCK_LOCK_CONN( _pb )		do { \
		ldap_pvt_thread_mutex_lock( &(_pb)->pb_conn->c_mutex ); \
	} while (0)

#define PBLOCK_UNLOCK_CONN( _pb )	do { \
		ldap_pvt_thread_mutex_unlock( &(_pb)->pb_conn->c_mutex ); \
	} while (0)

/* some parameters are only settable for internal operations */
#define PBLOCK_VALIDATE_IS_INTOP( _pb )	do { if ( (_pb)->pb_intop == 0 ) break; } while ( 0 )

static slapi_pblock_class_t 
pblock_get_param_class( int param ) 
{
	switch ( param ) {
	case SLAPI_PLUGIN_TYPE:
	case SLAPI_PLUGIN_ARGC:
	case SLAPI_PLUGIN_OPRETURN:
	case SLAPI_PLUGIN_INTOP_RESULT:
	case SLAPI_CONFIG_LINENO:
	case SLAPI_CONFIG_ARGC:
	case SLAPI_BIND_METHOD:
	case SLAPI_MODRDN_DELOLDRDN:
	case SLAPI_SEARCH_SCOPE:
	case SLAPI_SEARCH_DEREF:
	case SLAPI_SEARCH_SIZELIMIT:
	case SLAPI_SEARCH_TIMELIMIT:
	case SLAPI_SEARCH_ATTRSONLY:
	case SLAPI_NENTRIES:
	case SLAPI_CHANGENUMBER:
	case SLAPI_DBSIZE:
	case SLAPI_REQUESTOR_ISROOT:
	case SLAPI_BE_READONLY:
	case SLAPI_BE_LASTMOD:
	case SLAPI_DB2LDIF_PRINTKEY:
	case SLAPI_LDIF2DB_REMOVEDUPVALS:
	case SLAPI_MANAGEDSAIT:
	case SLAPI_X_RELAX:
	case SLAPI_X_OPERATION_NO_SCHEMA_CHECK:
	case SLAPI_IS_REPLICATED_OPERATION:
	case SLAPI_X_CONN_IS_UDP:
	case SLAPI_X_CONN_SSF:
	case SLAPI_RESULT_CODE:
	case SLAPI_LOG_OPERATION:
	case SLAPI_IS_INTERNAL_OPERATION:
		return PBLOCK_CLASS_INTEGER;
		break;

	case SLAPI_CONN_ID:
	case SLAPI_OPERATION_ID:
	case SLAPI_OPINITIATED_TIME:
	case SLAPI_ABANDON_MSGID:
	case SLAPI_X_OPERATION_DELETE_GLUE_PARENT:
	case SLAPI_OPERATION_MSGID:
		return PBLOCK_CLASS_LONG_INTEGER;
		break;

	case SLAPI_PLUGIN_DESTROY_FN:
	case SLAPI_PLUGIN_DB_BIND_FN:
	case SLAPI_PLUGIN_DB_UNBIND_FN:
	case SLAPI_PLUGIN_DB_SEARCH_FN:
	case SLAPI_PLUGIN_DB_COMPARE_FN:
	case SLAPI_PLUGIN_DB_MODIFY_FN:
	case SLAPI_PLUGIN_DB_MODRDN_FN:
	case SLAPI_PLUGIN_DB_ADD_FN:
	case SLAPI_PLUGIN_DB_DELETE_FN:
	case SLAPI_PLUGIN_DB_ABANDON_FN:
	case SLAPI_PLUGIN_DB_CONFIG_FN:
	case SLAPI_PLUGIN_CLOSE_FN:
	case SLAPI_PLUGIN_DB_FLUSH_FN:
	case SLAPI_PLUGIN_START_FN:
	case SLAPI_PLUGIN_DB_SEQ_FN:
	case SLAPI_PLUGIN_DB_ENTRY_FN:
	case SLAPI_PLUGIN_DB_REFERRAL_FN:
	case SLAPI_PLUGIN_DB_RESULT_FN:
	case SLAPI_PLUGIN_DB_LDIF2DB_FN:
	case SLAPI_PLUGIN_DB_DB2LDIF_FN:
	case SLAPI_PLUGIN_DB_BEGIN_FN:
	case SLAPI_PLUGIN_DB_COMMIT_FN:
	case SLAPI_PLUGIN_DB_ABORT_FN:
	case SLAPI_PLUGIN_DB_ARCHIVE2DB_FN:
	case SLAPI_PLUGIN_DB_DB2ARCHIVE_FN:
	case SLAPI_PLUGIN_DB_NEXT_SEARCH_ENTRY_FN:
	case SLAPI_PLUGIN_DB_FREE_RESULT_SET_FN:
	case SLAPI_PLUGIN_DB_SIZE_FN:
	case SLAPI_PLUGIN_DB_TEST_FN:
	case SLAPI_PLUGIN_DB_NO_ACL:
	case SLAPI_PLUGIN_EXT_OP_FN:
	case SLAPI_PLUGIN_EXT_OP_OIDLIST:
	case SLAPI_PLUGIN_PRE_BIND_FN:
	case SLAPI_PLUGIN_PRE_UNBIND_FN:
	case SLAPI_PLUGIN_PRE_SEARCH_FN:
	case SLAPI_PLUGIN_PRE_COMPARE_FN:
	case SLAPI_PLUGIN_PRE_MODIFY_FN:
	case SLAPI_PLUGIN_PRE_MODRDN_FN:
	case SLAPI_PLUGIN_PRE_ADD_FN:
	case SLAPI_PLUGIN_PRE_DELETE_FN:
	case SLAPI_PLUGIN_PRE_ABANDON_FN:
	case SLAPI_PLUGIN_PRE_ENTRY_FN:
	case SLAPI_PLUGIN_PRE_REFERRAL_FN:
	case SLAPI_PLUGIN_PRE_RESULT_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_ADD_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_MODIFY_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_MODRDN_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_DELETE_FN:
	case SLAPI_PLUGIN_BE_PRE_ADD_FN:
	case SLAPI_PLUGIN_BE_PRE_MODIFY_FN:
	case SLAPI_PLUGIN_BE_PRE_MODRDN_FN:
	case SLAPI_PLUGIN_BE_PRE_DELETE_FN:
	case SLAPI_PLUGIN_POST_BIND_FN:
	case SLAPI_PLUGIN_POST_UNBIND_FN:
	case SLAPI_PLUGIN_POST_SEARCH_FN:
	case SLAPI_PLUGIN_POST_COMPARE_FN:
	case SLAPI_PLUGIN_POST_MODIFY_FN:
	case SLAPI_PLUGIN_POST_MODRDN_FN:
	case SLAPI_PLUGIN_POST_ADD_FN:
	case SLAPI_PLUGIN_POST_DELETE_FN:
	case SLAPI_PLUGIN_POST_ABANDON_FN:
	case SLAPI_PLUGIN_POST_ENTRY_FN:
	case SLAPI_PLUGIN_POST_REFERRAL_FN:
	case SLAPI_PLUGIN_POST_RESULT_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_ADD_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_MODIFY_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_MODRDN_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_DELETE_FN:
	case SLAPI_PLUGIN_BE_POST_ADD_FN:
	case SLAPI_PLUGIN_BE_POST_MODIFY_FN:
	case SLAPI_PLUGIN_BE_POST_MODRDN_FN:
	case SLAPI_PLUGIN_BE_POST_DELETE_FN:
	case SLAPI_PLUGIN_MR_FILTER_CREATE_FN:
	case SLAPI_PLUGIN_MR_INDEXER_CREATE_FN:
	case SLAPI_PLUGIN_MR_FILTER_MATCH_FN:
	case SLAPI_PLUGIN_MR_FILTER_INDEX_FN:
	case SLAPI_PLUGIN_MR_FILTER_RESET_FN:
	case SLAPI_PLUGIN_MR_INDEX_FN:
	case SLAPI_PLUGIN_COMPUTE_EVALUATOR_FN:
	case SLAPI_PLUGIN_COMPUTE_SEARCH_REWRITER_FN:
	case SLAPI_PLUGIN_ACL_ALLOW_ACCESS:
	case SLAPI_X_PLUGIN_PRE_GROUP_FN:
	case SLAPI_X_PLUGIN_POST_GROUP_FN:
	case SLAPI_PLUGIN_AUDIT_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_BIND_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_UNBIND_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_SEARCH_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_COMPARE_FN:
	case SLAPI_PLUGIN_INTERNAL_PRE_ABANDON_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_BIND_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_UNBIND_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_SEARCH_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_COMPARE_FN:
	case SLAPI_PLUGIN_INTERNAL_POST_ABANDON_FN:
		return PBLOCK_CLASS_FUNCTION_POINTER;
		break;

	case SLAPI_BACKEND:
	case SLAPI_CONNECTION:
	case SLAPI_OPERATION:
	case SLAPI_OPERATION_PARAMETERS:
	case SLAPI_OPERATION_TYPE:
	case SLAPI_OPERATION_AUTHTYPE:
	case SLAPI_BE_MONITORDN:
	case SLAPI_BE_TYPE:
	case SLAPI_REQUESTOR_DN:
	case SLAPI_CONN_DN:
	case SLAPI_CONN_CLIENTIP:
	case SLAPI_CONN_SERVERIP:
	case SLAPI_CONN_AUTHTYPE:
	case SLAPI_CONN_AUTHMETHOD:
	case SLAPI_CONN_CERT:
	case SLAPI_X_CONN_CLIENTPATH:
	case SLAPI_X_CONN_SERVERPATH:
	case SLAPI_X_CONN_SASL_CONTEXT:
	case SLAPI_X_CONFIG_ARGV:
	case SLAPI_X_INTOP_FLAGS:
	case SLAPI_X_INTOP_RESULT_CALLBACK:
	case SLAPI_X_INTOP_SEARCH_ENTRY_CALLBACK:
	case SLAPI_X_INTOP_REFERRAL_ENTRY_CALLBACK:
	case SLAPI_X_INTOP_CALLBACK_DATA:
	case SLAPI_PLUGIN_MR_OID:
	case SLAPI_PLUGIN_MR_TYPE:
	case SLAPI_PLUGIN_MR_VALUE:
	case SLAPI_PLUGIN_MR_VALUES:
	case SLAPI_PLUGIN_MR_KEYS:
	case SLAPI_PLUGIN:
	case SLAPI_PLUGIN_PRIVATE:
	case SLAPI_PLUGIN_ARGV:
	case SLAPI_PLUGIN_OBJECT:
	case SLAPI_PLUGIN_DESCRIPTION:
	case SLAPI_PLUGIN_IDENTITY:
	case SLAPI_PLUGIN_INTOP_SEARCH_ENTRIES:
	case SLAPI_PLUGIN_INTOP_SEARCH_REFERRALS:
	case SLAPI_PLUGIN_MR_FILTER_REUSABLE:
	case SLAPI_PLUGIN_MR_QUERY_OPERATOR:
	case SLAPI_PLUGIN_MR_USAGE:
	case SLAPI_OP_LESS:
	case SLAPI_OP_LESS_OR_EQUAL:
	case SLAPI_PLUGIN_MR_USAGE_INDEX:
	case SLAPI_PLUGIN_SYNTAX_FILTER_AVA:
	case SLAPI_PLUGIN_SYNTAX_FILTER_SUB:
	case SLAPI_PLUGIN_SYNTAX_VALUES2KEYS:
	case SLAPI_PLUGIN_SYNTAX_ASSERTION2KEYS_AVA:
	case SLAPI_PLUGIN_SYNTAX_ASSERTION2KEYS_SUB:
	case SLAPI_PLUGIN_SYNTAX_NAMES:
	case SLAPI_PLUGIN_SYNTAX_OID:
	case SLAPI_PLUGIN_SYNTAX_FLAGS:
	case SLAPI_PLUGIN_SYNTAX_COMPARE:
	case SLAPI_CONFIG_FILENAME:
	case SLAPI_CONFIG_ARGV:
	case SLAPI_TARGET_ADDRESS:
	case SLAPI_TARGET_UNIQUEID:
	case SLAPI_TARGET_DN:
	case SLAPI_REQCONTROLS:
	case SLAPI_ENTRY_PRE_OP:
	case SLAPI_ENTRY_POST_OP:
	case SLAPI_RESCONTROLS:
	case SLAPI_X_OLD_RESCONTROLS:
	case SLAPI_ADD_RESCONTROL:
	case SLAPI_CONTROLS_ARG:
	case SLAPI_ADD_ENTRY:
	case SLAPI_ADD_EXISTING_DN_ENTRY:
	case SLAPI_ADD_PARENT_ENTRY:
	case SLAPI_ADD_PARENT_UNIQUEID:
	case SLAPI_ADD_EXISTING_UNIQUEID_ENTRY:
	case SLAPI_BIND_CREDENTIALS:
	case SLAPI_BIND_SASLMECHANISM:
	case SLAPI_BIND_RET_SASLCREDS:
	case SLAPI_COMPARE_TYPE:
	case SLAPI_COMPARE_VALUE:
	case SLAPI_MODIFY_MODS:
	case SLAPI_MODRDN_NEWRDN:
	case SLAPI_MODRDN_NEWSUPERIOR:
	case SLAPI_MODRDN_PARENT_ENTRY:
	case SLAPI_MODRDN_NEWPARENT_ENTRY:
	case SLAPI_MODRDN_TARGET_ENTRY:
	case SLAPI_MODRDN_NEWSUPERIOR_ADDRESS:
	case SLAPI_SEARCH_FILTER:
	case SLAPI_SEARCH_STRFILTER:
	case SLAPI_SEARCH_ATTRS:
	case SLAPI_SEQ_TYPE:
	case SLAPI_SEQ_ATTRNAME:
	case SLAPI_SEQ_VAL:
	case SLAPI_EXT_OP_REQ_OID:
	case SLAPI_EXT_OP_REQ_VALUE:
	case SLAPI_EXT_OP_RET_OID:
	case SLAPI_EXT_OP_RET_VALUE:
	case SLAPI_MR_FILTER_ENTRY:
	case SLAPI_MR_FILTER_TYPE:
	case SLAPI_MR_FILTER_VALUE:
	case SLAPI_MR_FILTER_OID:
	case SLAPI_MR_FILTER_DNATTRS:
	case SLAPI_LDIF2DB_FILE:
	case SLAPI_PARENT_TXN:
	case SLAPI_TXN:
	case SLAPI_SEARCH_RESULT_SET:
	case SLAPI_SEARCH_RESULT_ENTRY:
	case SLAPI_SEARCH_REFERRALS:
	case SLAPI_RESULT_TEXT:
	case SLAPI_RESULT_MATCHED:
	case SLAPI_X_GROUP_ENTRY:
	case SLAPI_X_GROUP_ATTRIBUTE:
	case SLAPI_X_GROUP_OPERATION_DN:
	case SLAPI_X_GROUP_TARGET_ENTRY:
	case SLAPI_X_ADD_STRUCTURAL_CLASS:
	case SLAPI_PLUGIN_AUDIT_DATA:
	case SLAPI_IBM_PBLOCK:
	case SLAPI_PLUGIN_VERSION:
		return PBLOCK_CLASS_POINTER;
		break;
	default:
		break;
	}

	return PBLOCK_CLASS_INVALID;
}

static void
pblock_lock( Slapi_PBlock *pb )
{
	ldap_pvt_thread_mutex_lock( &pb->pb_mutex );
}

static void
pblock_unlock( Slapi_PBlock *pb )
{
	ldap_pvt_thread_mutex_unlock( &pb->pb_mutex );
}

static int 
pblock_get_default( Slapi_PBlock *pb, int param, void **value ) 
{	
	int i;
	slapi_pblock_class_t pbClass;

	pbClass = pblock_get_param_class( param );
	if ( pbClass == PBLOCK_CLASS_INVALID ) {
		return PBLOCK_ERROR;
	}
	
	switch ( pbClass ) {
	case PBLOCK_CLASS_INTEGER:
		*((int *)value) = 0;
		break;
	case PBLOCK_CLASS_LONG_INTEGER:
		*((long *)value) = 0L;
		break;
	case PBLOCK_CLASS_POINTER:
	case PBLOCK_CLASS_FUNCTION_POINTER:
		*value = NULL;
		break;
	case PBLOCK_CLASS_INVALID:
		return PBLOCK_ERROR;
	}

	for ( i = 0; i < pb->pb_nParams; i++ ) {
		if ( pb->pb_params[i] == param ) {
			switch ( pbClass ) {
			case PBLOCK_CLASS_INTEGER:
				*((int *)value) = pb->pb_values[i].pv_integer;
				break;
			case PBLOCK_CLASS_LONG_INTEGER:
				*((long *)value) = pb->pb_values[i].pv_long_integer;
				break;
			case PBLOCK_CLASS_POINTER:
				*value = pb->pb_values[i].pv_pointer;
				break;
			case PBLOCK_CLASS_FUNCTION_POINTER:
				*value = pb->pb_values[i].pv_function_pointer;
				break;
			default:
				break;
			}
			break;
	  	}
	}

	return PBLOCK_SUCCESS;
}

static char *
pblock_get_authtype( AuthorizationInformation *authz, int is_tls )
{
	char *authType;

	switch ( authz->sai_method ) {
	case LDAP_AUTH_SASL:
		authType = SLAPD_AUTH_SASL;
		break;
	case LDAP_AUTH_SIMPLE:
		authType = SLAPD_AUTH_SIMPLE;
		break;
	case LDAP_AUTH_NONE:
		authType = SLAPD_AUTH_NONE;
		break;
	default:
		authType = NULL;
		break;
	}

	if ( is_tls && authType == NULL ) {
		authType = SLAPD_AUTH_SSL;
	}

	return authType;
}

static int 
pblock_set_default( Slapi_PBlock *pb, int param, void *value ) 
{
	slapi_pblock_class_t pbClass;
	int i;

	pbClass = pblock_get_param_class( param );
	if ( pbClass == PBLOCK_CLASS_INVALID ) {
		return PBLOCK_ERROR;
	}

	if ( pb->pb_nParams == PBLOCK_MAX_PARAMS ) {
		return PBLOCK_ERROR;
	}

	for ( i = 0; i < pb->pb_nParams; i++ ) {
		if ( pb->pb_params[i] == param )
			break;
	}
	if ( i >= pb->pb_nParams ) {
		pb->pb_params[i] = param;
	  	pb->pb_nParams++;
	}

	switch ( pbClass ) {
	case PBLOCK_CLASS_INTEGER:
		pb->pb_values[i].pv_integer = (*((int *)value));
		break;
	case PBLOCK_CLASS_LONG_INTEGER:
		pb->pb_values[i].pv_long_integer = (*((long *)value));
		break;
	case PBLOCK_CLASS_POINTER:
		pb->pb_values[i].pv_pointer = value;
		break;
	case PBLOCK_CLASS_FUNCTION_POINTER:
		pb->pb_values[i].pv_function_pointer = value;
		break;
	default:
		break;
	}

	return PBLOCK_SUCCESS;
}

static int
pblock_be_call( Slapi_PBlock *pb, int (*bep)(Operation *) )
{
	BackendDB *be_orig;
	Operation *op;
	int rc;

	PBLOCK_ASSERT_OP( pb, 0 );
	op = pb->pb_op;

	be_orig = op->o_bd;
	op->o_bd = select_backend( &op->o_req_ndn, 0 );
	rc = (*bep)( op );
	op->o_bd = be_orig;

	return rc;
}

static int 
pblock_get( Slapi_PBlock *pb, int param, void **value ) 
{
	int rc = PBLOCK_SUCCESS;

	pblock_lock( pb );

	switch ( param ) {
	case SLAPI_OPERATION:
		*value = pb->pb_op;
		break;
	case SLAPI_OPINITIATED_TIME:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((long *)value) = pb->pb_op->o_time;
		break;
	case SLAPI_OPERATION_ID:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((long *)value) = pb->pb_op->o_opid;
		break;
	case SLAPI_OPERATION_TYPE:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((ber_tag_t *)value) = pb->pb_op->o_tag;
		break;
	case SLAPI_OPERATION_MSGID:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((long *)value) = pb->pb_op->o_msgid;
		break;
	case SLAPI_X_OPERATION_DELETE_GLUE_PARENT:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = pb->pb_op->o_delete_glue_parent;
		break;
	case SLAPI_X_OPERATION_NO_SCHEMA_CHECK:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = get_no_schema_check( pb->pb_op );
		break;
	case SLAPI_X_ADD_STRUCTURAL_CLASS:
		PBLOCK_ASSERT_OP( pb, 0 );

		if ( pb->pb_op->o_tag == LDAP_REQ_ADD ) {
			struct berval tmpval = BER_BVNULL;

			rc = mods_structural_class( pb->pb_op->ora_modlist,
				&tmpval, &pb->pb_rs->sr_text,
				pb->pb_textbuf, sizeof( pb->pb_textbuf ),
				pb->pb_op->o_tmpmemctx );
			*((char **)value) = tmpval.bv_val;
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_X_OPERATION_NO_SUBORDINATE_GLUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = pb->pb_op->o_no_subordinate_glue;
		break;
	case SLAPI_REQCONTROLS:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((LDAPControl ***)value) = pb->pb_op->o_ctrls;
		break;
	case SLAPI_REQUESTOR_DN:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((char **)value) = pb->pb_op->o_dn.bv_val;
		break;
	case SLAPI_MANAGEDSAIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = get_manageDSAit( pb->pb_op );
		break;
	case SLAPI_X_RELAX:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = get_relax( pb->pb_op );
		break;
	case SLAPI_BACKEND:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((BackendDB **)value) = select_backend( &pb->pb_op->o_req_ndn, 0 );
		break;
	case SLAPI_BE_TYPE:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_bd != NULL )
			*((char **)value) = pb->pb_op->o_bd->bd_info->bi_type;
		else
			*value = NULL;
		break;
	case SLAPI_CONNECTION:
		*value = pb->pb_conn;
		break;
	case SLAPI_X_CONN_SSF:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((slap_ssf_t *)value) = pb->pb_conn->c_ssf;
		break;
	case SLAPI_X_CONN_SASL_CONTEXT:
		PBLOCK_ASSERT_CONN( pb );
		if ( pb->pb_conn->c_sasl_authctx != NULL )
			*value = pb->pb_conn->c_sasl_authctx;
		else
			*value = pb->pb_conn->c_sasl_sockctx;
		break;
	case SLAPI_TARGET_DN:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((char **)value) = pb->pb_op->o_req_dn.bv_val;
		break;
	case SLAPI_REQUESTOR_ISROOT:
		*((int *)value) = pblock_be_call( pb, be_isroot );
		break;
	case SLAPI_IS_REPLICATED_OPERATION:
		*((int *)value) = pblock_be_call( pb, be_slurp_update );
		break;
	case SLAPI_CONN_AUTHTYPE:
	case SLAPI_CONN_AUTHMETHOD: /* XXX should return SASL mech */
		PBLOCK_ASSERT_CONN( pb );
		*((char **)value) = pblock_get_authtype( &pb->pb_conn->c_authz,
#ifdef HAVE_TLS
							 pb->pb_conn->c_is_tls
#else
							 0
#endif
							 );
		break;
	case SLAPI_IS_INTERNAL_OPERATION:
		*((int *)value) = pb->pb_intop;
		break;
	case SLAPI_X_CONN_IS_UDP:
		PBLOCK_ASSERT_CONN( pb );
#ifdef LDAP_CONNECTIONLESS
		*((int *)value) = pb->pb_conn->c_is_udp;
#else
		*((int *)value) = 0;
#endif
		break;
	case SLAPI_CONN_ID:
		PBLOCK_ASSERT_CONN( pb );
		*((long *)value) = pb->pb_conn->c_connid;
		break;
	case SLAPI_CONN_DN:
		PBLOCK_ASSERT_CONN( pb );
#if 0
		/* This would be necessary to keep plugin compat after the fix in ITS#4158 */
		if ( pb->pb_op->o_tag == LDAP_REQ_BIND && pb->pb_rs->sr_err == LDAP_SUCCESS )
			*((char **)value) = pb->pb_op->orb_edn.bv_val;
		else
#endif
		*((char **)value) = pb->pb_conn->c_dn.bv_val;
		break;
	case SLAPI_CONN_CLIENTIP:
		PBLOCK_ASSERT_CONN( pb );
		if ( strncmp( pb->pb_conn->c_peer_name.bv_val, "IP=", 3 ) == 0 )
			*((char **)value) = &pb->pb_conn->c_peer_name.bv_val[3];
		else
			*value = NULL;
		break;
	case SLAPI_X_CONN_CLIENTPATH:
		PBLOCK_ASSERT_CONN( pb );
		if ( strncmp( pb->pb_conn->c_peer_name.bv_val, "PATH=", 3 ) == 0 )
			*((char **)value) = &pb->pb_conn->c_peer_name.bv_val[5];
		else
			*value = NULL;
		break;
	case SLAPI_CONN_SERVERIP:
		PBLOCK_ASSERT_CONN( pb );
		if ( strncmp( pb->pb_conn->c_sock_name.bv_val, "IP=", 3 ) == 0 )
			*((char **)value) = &pb->pb_conn->c_sock_name.bv_val[3];
		else
			*value = NULL;
		break;
	case SLAPI_X_CONN_SERVERPATH:
		PBLOCK_ASSERT_CONN( pb );
		if ( strncmp( pb->pb_conn->c_sock_name.bv_val, "PATH=", 3 ) == 0 )
			*((char **)value) = &pb->pb_conn->c_sock_name.bv_val[5];
		else
			*value = NULL;
		break;
	case SLAPI_RESULT_CODE:
	case SLAPI_PLUGIN_INTOP_RESULT:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((int *)value) = pb->pb_rs->sr_err;
		break;
        case SLAPI_RESULT_TEXT:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((const char **)value) = pb->pb_rs->sr_text;
		break;
        case SLAPI_RESULT_MATCHED:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((const char **)value) = pb->pb_rs->sr_matched;
		break;
	case SLAPI_ADD_ENTRY:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_ADD )
			*((Slapi_Entry **)value) = pb->pb_op->ora_e;
		else
			*value = NULL;
		break;
	case SLAPI_MODIFY_MODS: {
		LDAPMod **mods = NULL;
		Modifications *ml = NULL;

		pblock_get_default( pb, param, (void **)&mods );
		if ( mods == NULL && pb->pb_intop == 0 ) {
			switch ( pb->pb_op->o_tag ) {
			case LDAP_REQ_MODIFY:
				ml = pb->pb_op->orm_modlist;
				break;
			case LDAP_REQ_MODRDN:
				ml = pb->pb_op->orr_modlist;
				break;
			default:
				rc = PBLOCK_ERROR;
				break;
			}
			if ( rc != PBLOCK_ERROR ) {
				mods = slapi_int_modifications2ldapmods( ml );
				pblock_set_default( pb, param, (void *)mods );
			}
		}
		*((LDAPMod ***)value) = mods;
		break;
	}
	case SLAPI_MODRDN_NEWRDN:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN )
			*((char **)value) = pb->pb_op->orr_newrdn.bv_val;
		else
			*value = NULL;
		break;
	case SLAPI_MODRDN_NEWSUPERIOR:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN && pb->pb_op->orr_newSup != NULL )
			*((char **)value) = pb->pb_op->orr_newSup->bv_val;
		else
			*value = NULL;
		break;
	case SLAPI_MODRDN_DELOLDRDN:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN )
			*((int *)value) = pb->pb_op->orr_deleteoldrdn;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_SCOPE:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((int *)value) = pb->pb_op->ors_scope;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_DEREF:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((int *)value) = pb->pb_op->ors_deref;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_SIZELIMIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((int *)value) = pb->pb_op->ors_slimit;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_TIMELIMIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((int *)value) = pb->pb_op->ors_tlimit;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_FILTER:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((Slapi_Filter **)value) = pb->pb_op->ors_filter;
		else
			*((Slapi_Filter **)value) = NULL;
		break;
	case SLAPI_SEARCH_STRFILTER:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((char **)value) = pb->pb_op->ors_filterstr.bv_val;
		else
			*((char **)value) = NULL;
		break;
	case SLAPI_SEARCH_ATTRS: {
		char **attrs = NULL;

		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag != LDAP_REQ_SEARCH ) {
			rc = PBLOCK_ERROR;
			break;
		}
		pblock_get_default( pb, param, (void **)&attrs );
		if ( attrs == NULL && pb->pb_intop == 0 ) {
			attrs = anlist2charray_x( pb->pb_op->ors_attrs, 0, pb->pb_op->o_tmpmemctx );
			pblock_set_default( pb, param, (void *)attrs );
		}
		*((char ***)value) = attrs;
		break;
	}
	case SLAPI_SEARCH_ATTRSONLY:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			*((int *)value) = pb->pb_op->ors_attrsonly;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_SEARCH_RESULT_ENTRY:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((Slapi_Entry **)value) = pb->pb_rs->sr_entry;
		break;
	case SLAPI_BIND_RET_SASLCREDS:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((struct berval **)value) = pb->pb_rs->sr_sasldata;
		break;
	case SLAPI_EXT_OP_REQ_OID:
		*((const char **)value) = pb->pb_op->ore_reqoid.bv_val;
		break;
	case SLAPI_EXT_OP_REQ_VALUE:
		*((struct berval **)value) = pb->pb_op->ore_reqdata;
		break;
	case SLAPI_EXT_OP_RET_OID:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((const char **)value) = pb->pb_rs->sr_rspoid;
		break;
	case SLAPI_EXT_OP_RET_VALUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		*((struct berval **)value) = pb->pb_rs->sr_rspdata;
		break;
	case SLAPI_BIND_METHOD:
		if ( pb->pb_op->o_tag == LDAP_REQ_BIND )
			*((int *)value) = pb->pb_op->orb_method;
		else
			*((int *)value) = 0;
		break;
	case SLAPI_BIND_CREDENTIALS:
		if ( pb->pb_op->o_tag == LDAP_REQ_BIND )
			*((struct berval **)value) = &pb->pb_op->orb_cred;
		else
			*value = NULL;
		break;
	case SLAPI_COMPARE_TYPE:
		if ( pb->pb_op->o_tag == LDAP_REQ_COMPARE )
			*((char **)value) = pb->pb_op->orc_ava->aa_desc->ad_cname.bv_val;
		else
			*value = NULL;
		break;
	case SLAPI_COMPARE_VALUE:
		if ( pb->pb_op->o_tag == LDAP_REQ_COMPARE )
			*((struct berval **)value) = &pb->pb_op->orc_ava->aa_value;
		else
			*value = NULL;
		break;
	case SLAPI_ABANDON_MSGID:
		if ( pb->pb_op->o_tag == LDAP_REQ_ABANDON )
			*((int *)value) = pb->pb_op->orn_msgid;
		else
			*((int *)value) = 0;
		break;
	default:
		rc = pblock_get_default( pb, param, value );
		break;
	}

	pblock_unlock( pb );

	return rc;
}

static int
pblock_add_control( Slapi_PBlock *pb, LDAPControl *control )
{
	LDAPControl **controls = NULL;
	size_t i;

	pblock_get_default( pb, SLAPI_RESCONTROLS, (void **)&controls );

	if ( controls != NULL ) {
		for ( i = 0; controls[i] != NULL; i++ )
			;
	} else {
		i = 0;
	}

	controls = (LDAPControl **)slapi_ch_realloc( (char *)controls,
		( i + 2 ) * sizeof(LDAPControl *));
	controls[i++] = slapi_dup_control( control );
	controls[i] = NULL;

	return pblock_set_default( pb, SLAPI_RESCONTROLS, (void *)controls );
}

static int
pblock_set_dn( void *value, struct berval *dn, struct berval *ndn, void *memctx )
{
	struct berval bv;

	if ( !BER_BVISNULL( dn )) {
		slap_sl_free( dn->bv_val, memctx );
		BER_BVZERO( dn );
	}
	if ( !BER_BVISNULL( ndn )) {
		slap_sl_free( ndn->bv_val, memctx );
		BER_BVZERO( ndn );
	}

	bv.bv_val = (char *)value;
	bv.bv_len = ( value != NULL ) ? strlen( bv.bv_val ) : 0;

	return dnPrettyNormal( NULL, &bv, dn, ndn, memctx );
}

static int 
pblock_set( Slapi_PBlock *pb, int param, void *value ) 
{
	int rc = PBLOCK_SUCCESS;

	pblock_lock( pb );	

	switch ( param ) {
	case SLAPI_OPERATION:
		pb->pb_op = (Operation *)value;
		break;
	case SLAPI_OPINITIATED_TIME:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_time = *((long *)value);
		break;
	case SLAPI_OPERATION_ID:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_opid = *((long *)value);
		break;
	case SLAPI_OPERATION_TYPE:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_tag = *((ber_tag_t *)value);
		break;
	case SLAPI_OPERATION_MSGID:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_msgid = *((long *)value);
		break;
	case SLAPI_X_OPERATION_DELETE_GLUE_PARENT:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_delete_glue_parent = *((int *)value);
		break;
	case SLAPI_X_OPERATION_NO_SCHEMA_CHECK:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_no_schema_check = *((int *)value);
		break;
	case SLAPI_X_OPERATION_NO_SUBORDINATE_GLUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_no_subordinate_glue = *((int *)value);
		break;
	case SLAPI_REQCONTROLS:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_ctrls = (LDAPControl **)value;
		break;
	case SLAPI_RESCONTROLS: {
		LDAPControl **ctrls = NULL;

		pblock_get_default( pb, param, (void **)&ctrls );
		if ( ctrls != NULL ) {
			/* free old ones first */
			ldap_controls_free( ctrls );
		}
		rc = pblock_set_default( pb, param, value );
		break;
	}
	case SLAPI_ADD_RESCONTROL:
		PBLOCK_ASSERT_OP( pb, 0 );
		rc = pblock_add_control( pb, (LDAPControl *)value );
		break;
	case SLAPI_REQUESTOR_DN:
		PBLOCK_ASSERT_OP( pb, 0 );
		rc = pblock_set_dn( value, &pb->pb_op->o_dn, &pb->pb_op->o_ndn, pb->pb_op->o_tmpmemctx );
		break;
	case SLAPI_MANAGEDSAIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_managedsait = *((int *)value);
		break;
	case SLAPI_X_RELAX:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_relax = *((int *)value);
		break;
	case SLAPI_BACKEND:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_op->o_bd = (BackendDB *)value;
		break;
	case SLAPI_CONNECTION:
		pb->pb_conn = (Connection *)value;
		break;
	case SLAPI_X_CONN_SSF:
		PBLOCK_ASSERT_CONN( pb );
		PBLOCK_LOCK_CONN( pb );
		pb->pb_conn->c_ssf = (slap_ssf_t)(long)value;
		PBLOCK_UNLOCK_CONN( pb );
		break;
	case SLAPI_X_CONN_SASL_CONTEXT:
		PBLOCK_ASSERT_CONN( pb );
		PBLOCK_LOCK_CONN( pb );
		pb->pb_conn->c_sasl_authctx = value;
		PBLOCK_UNLOCK_CONN( pb );
		break;
	case SLAPI_TARGET_DN:
		PBLOCK_ASSERT_OP( pb, 0 );
		rc = pblock_set_dn( value, &pb->pb_op->o_req_dn, &pb->pb_op->o_req_ndn, pb->pb_op->o_tmpmemctx );
		break;
	case SLAPI_CONN_ID:
		PBLOCK_ASSERT_CONN( pb );
		PBLOCK_LOCK_CONN( pb );
		pb->pb_conn->c_connid = *((long *)value);
		PBLOCK_UNLOCK_CONN( pb );
		break;
	case SLAPI_CONN_DN:
		PBLOCK_ASSERT_CONN( pb );
		PBLOCK_LOCK_CONN( pb );
		rc = pblock_set_dn( value, &pb->pb_conn->c_dn, &pb->pb_conn->c_ndn, NULL );
		PBLOCK_UNLOCK_CONN( pb );
		break;
	case SLAPI_RESULT_CODE:
	case SLAPI_PLUGIN_INTOP_RESULT:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_rs->sr_err = *((int *)value);
		break;
	case SLAPI_RESULT_TEXT:
		PBLOCK_ASSERT_OP( pb, 0 );
		snprintf( pb->pb_textbuf, sizeof( pb->pb_textbuf ), "%s", (char *)value );
		pb->pb_rs->sr_text = pb->pb_textbuf;
		break;
	case SLAPI_RESULT_MATCHED:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_rs->sr_matched = (char *)value; /* XXX should dup? */
		break;
	case SLAPI_ADD_ENTRY:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_ADD )
			pb->pb_op->ora_e = (Slapi_Entry *)value;
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_MODIFY_MODS: {
		Modifications **mlp;
		Modifications *newmods;

		PBLOCK_ASSERT_OP( pb, 0 );
		rc = pblock_set_default( pb, param, value );
		if ( rc != PBLOCK_SUCCESS ) {
			break;
		}

		if ( pb->pb_op->o_tag == LDAP_REQ_MODIFY ) {
			mlp = &pb->pb_op->orm_modlist;
		} else if ( pb->pb_op->o_tag == LDAP_REQ_ADD ) {
			mlp = &pb->pb_op->ora_modlist;
		} else if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN ) {
			mlp = &pb->pb_op->orr_modlist;
		} else {
			break;
		}

		newmods = slapi_int_ldapmods2modifications( pb->pb_op, (LDAPMod **)value );
		if ( newmods != NULL ) {
			slap_mods_free( *mlp, 1 );
			*mlp = newmods;
		}
		break;
	}
	case SLAPI_MODRDN_NEWRDN:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN ) {
			rc = pblock_set_dn( value, &pb->pb_op->orr_newrdn, &pb->pb_op->orr_nnewrdn, pb->pb_op->o_tmpmemctx );
			if ( rc == LDAP_SUCCESS )
				rc = rdn_validate( &pb->pb_op->orr_nnewrdn );
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_MODRDN_NEWSUPERIOR:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN ) {
			if ( value == NULL ) {
				if ( pb->pb_op->orr_newSup != NULL ) {
					pb->pb_op->o_tmpfree( pb->pb_op->orr_newSup, pb->pb_op->o_tmpmemctx );
					BER_BVZERO( pb->pb_op->orr_newSup );
					pb->pb_op->orr_newSup = NULL;
				}
				if ( pb->pb_op->orr_newSup != NULL ) {
					pb->pb_op->o_tmpfree( pb->pb_op->orr_nnewSup, pb->pb_op->o_tmpmemctx );
					BER_BVZERO( pb->pb_op->orr_nnewSup );
					pb->pb_op->orr_nnewSup = NULL;
				}
			} else {
				if ( pb->pb_op->orr_newSup == NULL ) {
					pb->pb_op->orr_newSup = (struct berval *)pb->pb_op->o_tmpalloc(
						sizeof(struct berval), pb->pb_op->o_tmpmemctx );
					BER_BVZERO( pb->pb_op->orr_newSup );
				}
				if ( pb->pb_op->orr_nnewSup == NULL ) {
					pb->pb_op->orr_nnewSup = (struct berval *)pb->pb_op->o_tmpalloc(
						sizeof(struct berval), pb->pb_op->o_tmpmemctx );
					BER_BVZERO( pb->pb_op->orr_nnewSup );
				}
				rc = pblock_set_dn( value, pb->pb_op->orr_newSup, pb->pb_op->orr_nnewSup, pb->pb_op->o_tmpmemctx );
			}
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_MODRDN_DELOLDRDN:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );
		if ( pb->pb_op->o_tag == LDAP_REQ_MODRDN )
			pb->pb_op->orr_deleteoldrdn = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_SCOPE: {
		int scope = *((int *)value);

		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH ) {
			switch ( *((int *)value) ) {
			case LDAP_SCOPE_BASE:
			case LDAP_SCOPE_ONELEVEL:
			case LDAP_SCOPE_SUBTREE:
			case LDAP_SCOPE_SUBORDINATE:
				pb->pb_op->ors_scope = scope;
				break;
			default:
				rc = PBLOCK_ERROR;
				break;
			}
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	}
	case SLAPI_SEARCH_DEREF:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			pb->pb_op->ors_deref = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_SIZELIMIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			pb->pb_op->ors_slimit = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_TIMELIMIT:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			pb->pb_op->ors_tlimit = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_FILTER:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			pb->pb_op->ors_filter = (Slapi_Filter *)value;
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_STRFILTER:
		PBLOCK_ASSERT_OP( pb, 0 );
		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH ) {
			pb->pb_op->ors_filterstr.bv_val = (char *)value;
			pb->pb_op->ors_filterstr.bv_len = strlen((char *)value);
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_SEARCH_ATTRS: {
		AttributeName *an = NULL;
		size_t i = 0, j = 0;
		char **attrs = (char **)value;

		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag != LDAP_REQ_SEARCH ) {
			rc = PBLOCK_ERROR;
			break;
		}
		/* also set mapped attrs */
		rc = pblock_set_default( pb, param, value );
		if ( rc != PBLOCK_SUCCESS ) {
			break;
		}
		if ( pb->pb_op->ors_attrs != NULL ) {
			pb->pb_op->o_tmpfree( pb->pb_op->ors_attrs, pb->pb_op->o_tmpmemctx );
			pb->pb_op->ors_attrs = NULL;
		}
		if ( attrs != NULL ) {
			for ( i = 0; attrs[i] != NULL; i++ )
				;
		}
		if ( i ) {
			an = (AttributeName *)pb->pb_op->o_tmpcalloc( i + 1,
				sizeof(AttributeName), pb->pb_op->o_tmpmemctx );
			for ( i = 0; attrs[i] != NULL; i++ ) {
				an[j].an_desc = NULL;
				an[j].an_oc = NULL;
				an[j].an_flags = 0;
				an[j].an_name.bv_val = attrs[i];
				an[j].an_name.bv_len = strlen( attrs[i] );
				if ( slap_bv2ad( &an[j].an_name, &an[j].an_desc, &pb->pb_rs->sr_text ) == LDAP_SUCCESS ) {
					j++;
				}
			}
			an[j].an_name.bv_val = NULL;
			an[j].an_name.bv_len = 0;
		}	
		pb->pb_op->ors_attrs = an;
		break;
	}
	case SLAPI_SEARCH_ATTRSONLY:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_SEARCH )
			pb->pb_op->ors_attrsonly = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_SEARCH_RESULT_ENTRY:
		PBLOCK_ASSERT_OP( pb, 0 );
		rs_replace_entry( pb->pb_op, pb->pb_rs, NULL, (Slapi_Entry *)value );
		/* TODO: Should REP_ENTRY_MODIFIABLE be set? */
		pb->pb_rs->sr_flags |= REP_ENTRY_MUSTBEFREED;
		break;
	case SLAPI_BIND_RET_SASLCREDS:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_rs->sr_sasldata = (struct berval *)value;
		break;
	case SLAPI_EXT_OP_REQ_OID:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_EXTENDED ) {
			pb->pb_op->ore_reqoid.bv_val = (char *)value;
			pb->pb_op->ore_reqoid.bv_len = strlen((char *)value);
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_EXT_OP_REQ_VALUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_EXTENDED )
			pb->pb_op->ore_reqdata = (struct berval *)value;
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_EXT_OP_RET_OID:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_rs->sr_rspoid = (char *)value;
		break;
	case SLAPI_EXT_OP_RET_VALUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		pb->pb_rs->sr_rspdata = (struct berval *)value;
		break;
	case SLAPI_BIND_METHOD:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_BIND )
			pb->pb_op->orb_method = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_BIND_CREDENTIALS:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_BIND )
			pb->pb_op->orb_cred = *((struct berval *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_COMPARE_TYPE:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_COMPARE ) {
			const char *text;

			pb->pb_op->orc_ava->aa_desc = NULL;
			rc = slap_str2ad( (char *)value, &pb->pb_op->orc_ava->aa_desc, &text );
		} else {
			rc = PBLOCK_ERROR;
		}
		break;
	case SLAPI_COMPARE_VALUE:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_COMPARE )
			pb->pb_op->orc_ava->aa_value = *((struct berval *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_ABANDON_MSGID:
		PBLOCK_ASSERT_OP( pb, 0 );
		PBLOCK_VALIDATE_IS_INTOP( pb );

		if ( pb->pb_op->o_tag == LDAP_REQ_ABANDON)
			pb->pb_op->orn_msgid = *((int *)value);
		else
			rc = PBLOCK_ERROR;
		break;
	case SLAPI_REQUESTOR_ISROOT:
	case SLAPI_IS_REPLICATED_OPERATION:
	case SLAPI_CONN_AUTHTYPE:
	case SLAPI_CONN_AUTHMETHOD:
	case SLAPI_IS_INTERNAL_OPERATION:
	case SLAPI_X_CONN_IS_UDP:
	case SLAPI_CONN_CLIENTIP:
	case SLAPI_X_CONN_CLIENTPATH:
	case SLAPI_CONN_SERVERIP:
	case SLAPI_X_CONN_SERVERPATH:
	case SLAPI_X_ADD_STRUCTURAL_CLASS:
		/* These parameters cannot be set */
		rc = PBLOCK_ERROR;
		break;
	default:
		rc = pblock_set_default( pb, param, value );
		break;
	}

	pblock_unlock( pb );

	return rc;
}

static void
pblock_clear( Slapi_PBlock *pb ) 
{
	pb->pb_nParams = 1;
}

static int
pblock_delete_param( Slapi_PBlock *p, int param ) 
{
	int i;

	pblock_lock(p);

	for ( i = 0; i < p->pb_nParams; i++ ) { 
		if ( p->pb_params[i] == param ) {
			break;
		}
	}
    
	if (i >= p->pb_nParams ) {
		pblock_unlock( p );
		return PBLOCK_ERROR;
	}

	/* move last parameter to index of deleted parameter */
	if ( p->pb_nParams > 1 ) {
		p->pb_params[i] = p->pb_params[p->pb_nParams - 1];
		p->pb_values[i] = p->pb_values[p->pb_nParams - 1];
	}
	p->pb_nParams--;

	pblock_unlock( p );	

	return PBLOCK_SUCCESS;
}

Slapi_PBlock *
slapi_pblock_new(void) 
{
	Slapi_PBlock *pb;

	pb = (Slapi_PBlock *) ch_calloc( 1, sizeof(Slapi_PBlock) );
	if ( pb != NULL ) {
		ldap_pvt_thread_mutex_init( &pb->pb_mutex );

		pb->pb_params[0] = SLAPI_IBM_PBLOCK;
		pb->pb_values[0].pv_pointer = NULL;
		pb->pb_nParams = 1;
		pb->pb_conn = NULL;
		pb->pb_op = NULL;
		pb->pb_rs = NULL;
		pb->pb_intop = 0;
	}
	return pb;
}

static void
pblock_destroy( Slapi_PBlock *pb )
{
	LDAPControl **controls = NULL;
	LDAPMod **mods = NULL;
	char **attrs = NULL;

	assert( pb != NULL );

	pblock_get_default( pb, SLAPI_RESCONTROLS, (void **)&controls );
	if ( controls != NULL ) {
		ldap_controls_free( controls );
	}

	if ( pb->pb_intop ) {
		slapi_int_connection_done_pb( pb );
	} else {
		pblock_get_default( pb, SLAPI_MODIFY_MODS, (void **)&mods );
		ldap_mods_free( mods, 1 );

		pblock_get_default( pb, SLAPI_SEARCH_ATTRS, (void **)&attrs );
		if ( attrs != NULL )
			pb->pb_op->o_tmpfree( attrs, pb->pb_op->o_tmpmemctx );
	}

	ldap_pvt_thread_mutex_destroy( &pb->pb_mutex );
	slapi_ch_free( (void **)&pb ); 
}

void 
slapi_pblock_destroy( Slapi_PBlock *pb ) 
{
	if ( pb != NULL ) {
		pblock_destroy( pb );
	}
}

int 
slapi_pblock_get( Slapi_PBlock *pb, int arg, void *value ) 
{
	return pblock_get( pb, arg, (void **)value );
}

int 
slapi_pblock_set( Slapi_PBlock *pb, int arg, void *value ) 
{
	return pblock_set( pb, arg, value );
}

void
slapi_pblock_clear( Slapi_PBlock *pb ) 
{
	pblock_clear( pb );
}

int 
slapi_pblock_delete_param( Slapi_PBlock *p, int param ) 
{
	return pblock_delete_param( p, param );
}

/*
 * OpenLDAP extension
 */
int
slapi_int_pblock_get_first( Backend *be, Slapi_PBlock **pb )
{
	assert( pb != NULL );
	*pb = SLAPI_BACKEND_PBLOCK( be );
	return (*pb == NULL ? LDAP_OTHER : LDAP_SUCCESS);
}

/*
 * OpenLDAP extension
 */
int
slapi_int_pblock_get_next( Slapi_PBlock **pb )
{
	assert( pb != NULL );
	return slapi_pblock_get( *pb, SLAPI_IBM_PBLOCK, pb );
}

#endif /* LDAP_SLAPI */
