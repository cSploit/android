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
#include <ac/socket.h>
#include <ac/string.h>

#include "slap.h"

int
do_compare(
    Operation	*op,
    SlapReply	*rs )
{
	struct berval dn = BER_BVNULL;
	struct berval desc = BER_BVNULL;
	struct berval value = BER_BVNULL;
	AttributeAssertion ava = ATTRIBUTEASSERTION_INIT;

	Debug( LDAP_DEBUG_TRACE, "%s do_compare\n",
		op->o_log_prefix, 0, 0 );
	/*
	 * Parse the compare request.  It looks like this:
	 *
	 *	CompareRequest := [APPLICATION 14] SEQUENCE {
	 *		entry	DistinguishedName,
	 *		ava	SEQUENCE {
	 *			type	AttributeType,
	 *			value	AttributeValue
	 *		}
	 *	}
	 */

	if ( ber_scanf( op->o_ber, "{m" /*}*/, &dn ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_compare: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	if ( ber_scanf( op->o_ber, "{mm}", &desc, &value ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_compare: get ava failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	if ( ber_scanf( op->o_ber, /*{*/ "}" ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_compare: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	if( get_ctrls( op, rs, 1 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_compare: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		goto cleanup;
	} 

	rs->sr_err = dnPrettyNormal( NULL, &dn, &op->o_req_dn, &op->o_req_ndn,
		op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_compare: invalid dn (%s)\n",
			op->o_log_prefix, dn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid DN" );
		goto cleanup;
	}

	Statslog( LDAP_DEBUG_STATS,
		"%s CMP dn=\"%s\" attr=\"%s\"\n",
		op->o_log_prefix, op->o_req_dn.bv_val,
		desc.bv_val, 0, 0 );

	rs->sr_err = slap_bv2ad( &desc, &ava.aa_desc, &rs->sr_text );
	if( rs->sr_err != LDAP_SUCCESS ) {
		rs->sr_err = slap_bv2undef_ad( &desc, &ava.aa_desc,
				&rs->sr_text,
				SLAP_AD_PROXIED|SLAP_AD_NOINSERT );
		if( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			goto cleanup;
		}
	}

	rs->sr_err = asserted_value_validate_normalize( ava.aa_desc,
		ava.aa_desc->ad_type->sat_equality,
		SLAP_MR_EQUALITY|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
		&value, &ava.aa_value, &rs->sr_text, op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	op->orc_ava = &ava;

	Debug( LDAP_DEBUG_ARGS,
		"do_compare: dn (%s) attr (%s) value (%s)\n",
		op->o_req_dn.bv_val,
		ava.aa_desc->ad_cname.bv_val, ava.aa_value.bv_val );

	op->o_bd = frontendDB;
	rs->sr_err = frontendDB->be_compare( op, rs );

cleanup:;
	op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
	if ( !BER_BVISNULL( &ava.aa_value ) ) {
		op->o_tmpfree( ava.aa_value.bv_val, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

int
fe_op_compare( Operation *op, SlapReply *rs )
{
	Entry			*entry = NULL;
	AttributeAssertion	*ava = op->orc_ava;
	BackendDB		*bd = op->o_bd;

	if( strcasecmp( op->o_req_ndn.bv_val, LDAP_ROOT_DSE ) == 0 ) {
		if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			goto cleanup;
		}

		rs->sr_err = root_dse_info( op->o_conn, &entry, &rs->sr_text );
		if( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			goto cleanup;
		}

	} else if ( bvmatch( &op->o_req_ndn, &frontendDB->be_schemandn ) ) {
		if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			rs->sr_err = 0;
			goto cleanup;
		}

		rs->sr_err = schema_info( &entry, &rs->sr_text );
		if( rs->sr_err != LDAP_SUCCESS ) {
			send_ldap_result( op, rs );
			rs->sr_err = 0;
			goto cleanup;
		}
	}

	if( entry ) {
		rs->sr_err = slap_compare_entry( op, entry, ava );
		entry_free( entry );

		send_ldap_result( op, rs );

		if( rs->sr_err == LDAP_COMPARE_TRUE ||
			rs->sr_err == LDAP_COMPARE_FALSE )
		{
			rs->sr_err = LDAP_SUCCESS;
		}

		goto cleanup;
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	op->o_bd = select_backend( &op->o_req_ndn, 0 );
	if ( op->o_bd == NULL ) {
		rs->sr_ref = referral_rewrite( default_referral,
			NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );

		rs->sr_err = LDAP_REFERRAL;
		if (!rs->sr_ref) rs->sr_ref = default_referral;
		op->o_bd = bd;
		send_ldap_result( op, rs );

		if (rs->sr_ref != default_referral) ber_bvarray_free( rs->sr_ref );
		rs->sr_err = 0;
		goto cleanup;
	}

	/* check restrictions */
	if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* check for referrals */
	if( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		goto cleanup;
	}

	if ( SLAP_SHADOW(op->o_bd) && get_dontUseCopy(op) ) {
		/* don't use shadow copy */
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"copy not used" );

	} else if ( ava->aa_desc == slap_schema.si_ad_entryDN ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"entryDN compare not supported" );

	} else if ( ava->aa_desc == slap_schema.si_ad_subschemaSubentry ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"subschemaSubentry compare not supported" );

#ifndef SLAP_COMPARE_IN_FRONTEND
	} else if ( ava->aa_desc == slap_schema.si_ad_hasSubordinates
		&& op->o_bd->be_has_subordinates )
	{
		int	rc, hasSubordinates = LDAP_SUCCESS;

		rc = be_entry_get_rw( op, &op->o_req_ndn, NULL, NULL, 0, &entry );
		if ( rc == 0 && entry ) {
			if ( ! access_allowed( op, entry,
				ava->aa_desc, &ava->aa_value, ACL_COMPARE, NULL ) )
			{	
				rc = rs->sr_err = LDAP_INSUFFICIENT_ACCESS;
				
			} else {
				rc = rs->sr_err = op->o_bd->be_has_subordinates( op,
						entry, &hasSubordinates );
				be_entry_release_r( op, entry );
			}
		}

		if ( rc == 0 ) {
			int	asserted;

			asserted = bvmatch( &ava->aa_value, &slap_true_bv )
				? LDAP_COMPARE_TRUE : LDAP_COMPARE_FALSE;
			if ( hasSubordinates == asserted ) {
				rs->sr_err = LDAP_COMPARE_TRUE;

			} else {
				rs->sr_err = LDAP_COMPARE_FALSE;
			}

		} else {
			/* return error only if "disclose"
			 * is granted on the object */
			if ( backend_access( op, NULL, &op->o_req_ndn,
					slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL ) == LDAP_INSUFFICIENT_ACCESS )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}
		}

		send_ldap_result( op, rs );

		if ( rc == 0 ) {
			rs->sr_err = LDAP_SUCCESS;
		}

	} else if ( op->o_bd->be_compare ) {
		rs->sr_err = op->o_bd->be_compare( op, rs );

#endif /* ! SLAP_COMPARE_IN_FRONTEND */
	} else {
		rs->sr_err = SLAP_CB_CONTINUE;
	}

	if ( rs->sr_err == SLAP_CB_CONTINUE ) {
		/* do our best to compare that AVA
		 * 
		 * NOTE: this code is used only
		 * if SLAP_COMPARE_IN_FRONTEND 
		 * is #define'd (it's not by default)
		 * or if op->o_bd->be_compare is NULL.
		 * 
		 * FIXME: one potential issue is that
		 * if SLAP_COMPARE_IN_FRONTEND overlays
		 * are not executed for compare. */
		BerVarray	vals = NULL;
		int		rc = LDAP_OTHER;

		rs->sr_err = backend_attribute( op, NULL, &op->o_req_ndn,
				ava->aa_desc, &vals, ACL_COMPARE );
		switch ( rs->sr_err ) {
		default:
			/* return error only if "disclose"
			 * is granted on the object */
			if ( backend_access( op, NULL, &op->o_req_ndn,
					slap_schema.si_ad_entry,
					NULL, ACL_DISCLOSE, NULL )
					== LDAP_INSUFFICIENT_ACCESS )
			{
				rs->sr_err = LDAP_NO_SUCH_OBJECT;
			}
			break;

		case LDAP_SUCCESS:
			if ( value_find_ex( op->oq_compare.rs_ava->aa_desc,
				SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
					SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
				vals, &ava->aa_value, op->o_tmpmemctx ) == 0 )
			{
				rs->sr_err = LDAP_COMPARE_TRUE;
				break;

			} else {
				rs->sr_err = LDAP_COMPARE_FALSE;
			}
			rc = LDAP_SUCCESS;
			break;
		}

		send_ldap_result( op, rs );

		if ( rc == 0 ) {
			rs->sr_err = LDAP_SUCCESS;
		}
		
		if ( vals ) {
			ber_bvarray_free_x( vals, op->o_tmpmemctx );
		}
	}

cleanup:;
	op->o_bd = bd;
	return rs->sr_err;
}

int slap_compare_entry(
	Operation *op,
	Entry *e,
	AttributeAssertion *ava )
{
	int rc = LDAP_COMPARE_FALSE;
	Attribute *a;

	if ( ! access_allowed( op, e,
		ava->aa_desc, &ava->aa_value, ACL_COMPARE, NULL ) )
	{	
		rc = LDAP_INSUFFICIENT_ACCESS;
		goto done;
	}

	if ( get_assert( op ) &&
		( test_filter( op, e, get_assertion( op )) != LDAP_COMPARE_TRUE ))
	{
		rc = LDAP_ASSERTION_FAILED;
		goto done;
	}

	a = attrs_find( e->e_attrs, ava->aa_desc );
	if( a == NULL ) {
		rc = LDAP_NO_SUCH_ATTRIBUTE;
		goto done;
	}

	for(;
		a != NULL;
		a = attrs_find( a->a_next, ava->aa_desc ))
	{
		if (( ava->aa_desc != a->a_desc ) && ! access_allowed( op,
			e, a->a_desc, &ava->aa_value, ACL_COMPARE, NULL ) )
		{	
			rc = LDAP_INSUFFICIENT_ACCESS;
			break;
		}

		if ( attr_valfind( a, 
			SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH |
				SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH,
			&ava->aa_value, NULL, op->o_tmpmemctx ) == 0 )
		{
			rc = LDAP_COMPARE_TRUE;
			break;
		}
	}

done:
	if( rc != LDAP_COMPARE_TRUE && rc != LDAP_COMPARE_FALSE ) {
		if ( ! access_allowed( op, e,
			slap_schema.si_ad_entry, NULL, ACL_DISCLOSE, NULL ) )
		{
			rc = LDAP_NO_SUCH_OBJECT;
		}
	}

	return rc;
}
