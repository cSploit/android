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
/* Portions Copyright 1999, Juan C. Gomez, All rights reserved.
 * This software is not subject to any license of Silicon Graphics 
 * Inc. or Purdue University.
 *
 * Redistribution and use in source and binary forms are permitted
 * without restriction or fee of any kind as long as this notice
 * is preserved.
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
do_modrdn(
    Operation	*op,
    SlapReply	*rs
)
{
	struct berval	dn = BER_BVNULL;
	struct berval	newrdn = BER_BVNULL;
	struct berval	newSuperior = BER_BVNULL;
	ber_int_t	deloldrdn;

	struct berval pnewSuperior = BER_BVNULL;

	struct berval nnewSuperior = BER_BVNULL;

	ber_len_t	length;

	Debug( LDAP_DEBUG_TRACE, "%s do_modrdn\n",
			op->o_log_prefix, 0, 0 );
	/*
	 * Parse the modrdn request.  It looks like this:
	 *
	 *	ModifyRDNRequest := SEQUENCE {
	 *		entry	DistinguishedName,
	 *		newrdn	RelativeDistinguishedName
	 *		deleteoldrdn	BOOLEAN,
	 *		newSuperior	[0] LDAPDN OPTIONAL (v3 Only!)
	 *	}
	 */

	if ( ber_scanf( op->o_ber, "{mmb", &dn, &newrdn, &deloldrdn )
	    == LBER_ERROR )
	{
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	/* Check for newSuperior parameter, if present scan it */

	if ( ber_peek_tag( op->o_ber, &length ) == LDAP_TAG_NEWSUPERIOR ) {
		if ( op->o_protocol < LDAP_VERSION3 ) {
			/* Connection record indicates v2 but field 
			 * newSuperior is present: report error.
			 */
			Debug( LDAP_DEBUG_ANY,
				"%s do_modrdn: newSuperior requires LDAPv3\n",
				op->o_log_prefix, 0, 0 );

			send_ldap_discon( op, rs,
				LDAP_PROTOCOL_ERROR, "newSuperior requires LDAPv3" );
			rs->sr_err = SLAPD_DISCONNECT;
			goto cleanup;
		}

		if ( ber_scanf( op->o_ber, "m", &newSuperior ) 
		     == LBER_ERROR ) {

			Debug( LDAP_DEBUG_ANY, "%s do_modrdn: ber_scanf(\"m\") failed\n",
				op->o_log_prefix, 0, 0 );

			send_ldap_discon( op, rs,
				LDAP_PROTOCOL_ERROR, "decoding error" );
			rs->sr_err = SLAPD_DISCONNECT;
			goto cleanup;
		}
		op->orr_newSup = &pnewSuperior;
		op->orr_nnewSup = &nnewSuperior;
	}

	Debug( LDAP_DEBUG_ARGS,
	    "do_modrdn: dn (%s) newrdn (%s) newsuperior (%s)\n",
		dn.bv_val, newrdn.bv_val,
		newSuperior.bv_len ? newSuperior.bv_val : "" );

	if ( ber_scanf( op->o_ber, /*{*/ "}") == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs,
			LDAP_PROTOCOL_ERROR, "decoding error" );
		rs->sr_err = SLAPD_DISCONNECT;
		goto cleanup;
	}

	if( get_ctrls( op, rs, 1 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		/* get_ctrls has sent results.	Now clean up. */
		goto cleanup;
	} 

	rs->sr_err = dnPrettyNormal( NULL, &dn, &op->o_req_dn, &op->o_req_ndn, op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: invalid dn (%s)\n",
			op->o_log_prefix, dn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid DN" );
		goto cleanup;
	}

	/* FIXME: should have/use rdnPretty / rdnNormalize routines */

	rs->sr_err = dnPrettyNormal( NULL, &newrdn, &op->orr_newrdn, &op->orr_nnewrdn, op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: invalid newrdn (%s)\n",
			op->o_log_prefix, newrdn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid new RDN" );
		goto cleanup;
	}

	if( rdn_validate( &op->orr_newrdn ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: invalid rdn (%s)\n",
			op->o_log_prefix, op->orr_newrdn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid new RDN" );
		goto cleanup;
	}

	if( op->orr_newSup ) {
		rs->sr_err = dnPrettyNormal( NULL, &newSuperior, &pnewSuperior,
			&nnewSuperior, op->o_tmpmemctx );
		if( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"%s do_modrdn: invalid newSuperior (%s)\n",
				op->o_log_prefix, newSuperior.bv_val, 0 );
			send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid newSuperior" );
			goto cleanup;
		}
	}

	Statslog( LDAP_DEBUG_STATS, "%s MODRDN dn=\"%s\"\n",
	    op->o_log_prefix, op->o_req_dn.bv_val, 0, 0, 0 );

	op->orr_deleteoldrdn = deloldrdn;
	op->orr_modlist = NULL;

	/* prepare modlist of modifications from old/new RDN */
	rs->sr_err = slap_modrdn2mods( op, rs );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	op->o_bd = frontendDB;
	rs->sr_err = frontendDB->be_modrdn( op, rs );

#ifdef LDAP_X_TXN
	if( rs->sr_err == LDAP_X_TXN_SPECIFY_OKAY ) {
		/* skip cleanup */
	}
#endif

cleanup:
	op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );

	op->o_tmpfree( op->orr_newrdn.bv_val, op->o_tmpmemctx );	
	op->o_tmpfree( op->orr_nnewrdn.bv_val, op->o_tmpmemctx );	

	if ( op->orr_modlist != NULL )
		slap_mods_free( op->orr_modlist, 1 );

	if ( !BER_BVISNULL( &pnewSuperior ) ) {
		op->o_tmpfree( pnewSuperior.bv_val, op->o_tmpmemctx );
	}
	if ( !BER_BVISNULL( &nnewSuperior ) ) {
		op->o_tmpfree( nnewSuperior.bv_val, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

int
fe_op_modrdn( Operation *op, SlapReply *rs )
{
	struct berval	dest_ndn = BER_BVNULL, dest_pndn, pdn = BER_BVNULL;
	BackendDB	*op_be, *bd = op->o_bd;
	ber_slen_t	diff;
	
	if( op->o_req_ndn.bv_len == 0 ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: root dse!\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"cannot rename the root DSE" );
		goto cleanup;

	} else if ( bvmatch( &op->o_req_ndn, &frontendDB->be_schemandn ) ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modrdn: subschema subentry: %s (%ld)\n",
			op->o_log_prefix, frontendDB->be_schemandn.bv_val, (long)frontendDB->be_schemandn.bv_len );

		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"cannot rename subschema subentry" );
		goto cleanup;
	}

	if( op->orr_nnewSup ) {
		dest_pndn = *op->orr_nnewSup;
	} else {
		dnParent( &op->o_req_ndn, &dest_pndn );
	}
	build_new_dn( &dest_ndn, &dest_pndn, &op->orr_nnewrdn, op->o_tmpmemctx );

	diff = (ber_slen_t) dest_ndn.bv_len - (ber_slen_t) op->o_req_ndn.bv_len;
	if ( diff > 0 ? dnIsSuffix( &dest_ndn, &op->o_req_ndn )
		: diff < 0 && dnIsSuffix( &op->o_req_ndn, &dest_ndn ) )
	{
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			diff > 0 ? "cannot place an entry below itself"
			: "cannot place an entry above itself" );
		goto cleanup;
	}

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	op->o_bd = select_backend( &op->o_req_ndn, 1 );
	if ( op->o_bd == NULL ) {
		op->o_bd = bd;
		rs->sr_ref = referral_rewrite( default_referral,
			NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
		if (!rs->sr_ref) rs->sr_ref = default_referral;

		if ( rs->sr_ref != NULL ) {
			rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );

			if (rs->sr_ref != default_referral) ber_bvarray_free( rs->sr_ref );
		} else {
			send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
				"no global superior knowledge" );
		}
		goto cleanup;
	}

	/* If we've got a glued backend, check the real backend */
	op_be = op->o_bd;
	if ( SLAP_GLUE_INSTANCE( op->o_bd )) {
		op->o_bd = select_backend( &op->o_req_ndn, 0 );
	}

	/* check restrictions */
	if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* check for referrals */
	if ( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		goto cleanup;
	}

	/* check that destination DN is in the same backend as source DN */
	if ( select_backend( &dest_ndn, 0 ) != op->o_bd ) {
			send_ldap_error( op, rs, LDAP_AFFECTS_MULTIPLE_DSAS,
				"cannot rename between DSAs" );
			goto cleanup;
	}

	/*
	 * do the modrdn if 1 && (2 || 3)
	 * 1) there is a modrdn function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the update_ndn.
	 */
	if ( op->o_bd->be_modrdn ) {
		/* do the update here */
		int repl_user = be_isupdate( op );
		if ( !SLAP_SINGLE_SHADOW(op->o_bd) || repl_user )
		{
			op->o_bd = op_be;
			op->o_bd->be_modrdn( op, rs );

			if ( op->o_bd->be_delete ) {
				struct berval	org_req_dn = BER_BVNULL;
				struct berval	org_req_ndn = BER_BVNULL;
				struct berval	org_dn = BER_BVNULL;
				struct berval	org_ndn = BER_BVNULL;
				int		org_managedsait;

				org_req_dn = op->o_req_dn;
				org_req_ndn = op->o_req_ndn;
				org_dn = op->o_dn;
				org_ndn = op->o_ndn;
				org_managedsait = get_manageDSAit( op );
				op->o_dn = op->o_bd->be_rootdn;
				op->o_ndn = op->o_bd->be_rootndn;
				op->o_managedsait = SLAP_CONTROL_NONCRITICAL;

				while ( rs->sr_err == LDAP_SUCCESS &&
						op->o_delete_glue_parent ) {
					op->o_delete_glue_parent = 0;
					if ( !be_issuffix( op->o_bd, &op->o_req_ndn )) {
						slap_callback cb = { NULL };
						cb.sc_response = slap_null_cb;
						dnParent( &op->o_req_ndn, &pdn );
						op->o_req_dn = pdn;
						op->o_req_ndn = pdn;
						op->o_callback = &cb;
						op->o_bd->be_delete( op, rs );
					} else {
						break;
					}
				}
				op->o_managedsait = org_managedsait;
				op->o_dn = org_dn;
				op->o_ndn = org_ndn;
				op->o_req_dn = org_req_dn;
				op->o_req_ndn = org_req_ndn;
				op->o_delete_glue_parent = 0;
			}

		} else {
			BerVarray defref = op->o_bd->be_update_refs
				? op->o_bd->be_update_refs : default_referral;

			if ( defref != NULL ) {
				rs->sr_ref = referral_rewrite( defref,
					NULL, &op->o_req_dn, LDAP_SCOPE_DEFAULT );
				if (!rs->sr_ref) rs->sr_ref = defref;

				rs->sr_err = LDAP_REFERRAL;
				send_ldap_result( op, rs );

				if (rs->sr_ref != defref) ber_bvarray_free( rs->sr_ref );
			} else {
				send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
					"shadow context; no update referral" );
			}
		}
	} else {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"operation not supported within namingContext" );
	}

cleanup:;
	if ( dest_ndn.bv_val != NULL )
		ber_memfree_x( dest_ndn.bv_val, op->o_tmpmemctx );
	op->o_bd = bd;
	return rs->sr_err;
}

int
slap_modrdn2mods(
	Operation	*op,
	SlapReply	*rs )
{
	int		a_cnt, d_cnt;
	LDAPRDN		old_rdn = NULL;
	LDAPRDN		new_rdn = NULL;

	assert( !BER_BVISEMPTY( &op->oq_modrdn.rs_newrdn ) );

	/* if requestDN is empty, silently reset deleteOldRDN */
	if ( BER_BVISEMPTY( &op->o_req_dn ) ) op->orr_deleteoldrdn = 0;

	if ( ldap_bv2rdn_x( &op->oq_modrdn.rs_newrdn, &new_rdn,
		(char **)&rs->sr_text, LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx ) ) {
		Debug( LDAP_DEBUG_TRACE,
			"%s slap_modrdn2mods: can't figure out "
			"type(s)/value(s) of newrdn\n",
			op->o_log_prefix, 0, 0 );
		rs->sr_err = LDAP_INVALID_DN_SYNTAX;
		rs->sr_text = "unknown type(s)/value(s) used in RDN";
		goto done;
	}

	if ( op->oq_modrdn.rs_deleteoldrdn ) {
		if ( ldap_bv2rdn_x( &op->o_req_dn, &old_rdn,
			(char **)&rs->sr_text, LDAP_DN_FORMAT_LDAP, op->o_tmpmemctx ) ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s slap_modrdn2mods: can't figure out "
				"type(s)/value(s) of oldrdn\n",
				op->o_log_prefix, 0, 0 );
			rs->sr_err = LDAP_OTHER;
			rs->sr_text = "cannot parse RDN from old DN";
			goto done;
		}
	}
	rs->sr_text = NULL;

	/* Add new attribute values to the entry */
	for ( a_cnt = 0; new_rdn[a_cnt]; a_cnt++ ) {
		AttributeDescription	*desc = NULL;
		Modifications 		*mod_tmp;

		rs->sr_err = slap_bv2ad( &new_rdn[a_cnt]->la_attr, &desc, &rs->sr_text );

		if ( rs->sr_err != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s slap_modrdn2mods: %s: %s (new)\n",
				op->o_log_prefix,
				rs->sr_text,
				new_rdn[ a_cnt ]->la_attr.bv_val );
			goto done;		
		}

		if ( !desc->ad_type->sat_equality ) {
			Debug( LDAP_DEBUG_TRACE,
				"%s slap_modrdn2mods: %s: %s (new)\n",
				op->o_log_prefix,
				rs->sr_text,
				new_rdn[ a_cnt ]->la_attr.bv_val );
			rs->sr_text = "naming attribute has no equality matching rule";
			rs->sr_err = LDAP_NAMING_VIOLATION;
			goto done;
		}

		/* Apply modification */
		mod_tmp = ( Modifications * )ch_malloc( sizeof( Modifications ) );
		mod_tmp->sml_desc = desc;
		BER_BVZERO( &mod_tmp->sml_type );
		mod_tmp->sml_numvals = 1;
		mod_tmp->sml_values = ( BerVarray )ch_malloc( 2 * sizeof( struct berval ) );
		ber_dupbv( &mod_tmp->sml_values[0], &new_rdn[a_cnt]->la_value );
		mod_tmp->sml_values[1].bv_val = NULL;
		if( desc->ad_type->sat_equality->smr_normalize) {
			mod_tmp->sml_nvalues = ( BerVarray )ch_malloc( 2 * sizeof( struct berval ) );
			rs->sr_err = desc->ad_type->sat_equality->smr_normalize(
				SLAP_MR_EQUALITY|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
				desc->ad_type->sat_syntax,
				desc->ad_type->sat_equality,
				&mod_tmp->sml_values[0],
				&mod_tmp->sml_nvalues[0], NULL );
			if (rs->sr_err != LDAP_SUCCESS) {
				ch_free(mod_tmp->sml_nvalues);
				ch_free(mod_tmp->sml_values[0].bv_val);
				ch_free(mod_tmp->sml_values);
				ch_free(mod_tmp);
				goto done;
			}
			mod_tmp->sml_nvalues[1].bv_val = NULL;
		} else {
			mod_tmp->sml_nvalues = NULL;
		}
		mod_tmp->sml_op = SLAP_MOD_SOFTADD;
		mod_tmp->sml_flags = 0;
		mod_tmp->sml_next = op->orr_modlist;
		op->orr_modlist = mod_tmp;
	}

	/* Remove old rdn value if required */
	if ( op->orr_deleteoldrdn ) {
		for ( d_cnt = 0; old_rdn[d_cnt]; d_cnt++ ) {
			AttributeDescription	*desc = NULL;
			Modifications 		*mod_tmp;

			rs->sr_err = slap_bv2ad( &old_rdn[d_cnt]->la_attr, &desc, &rs->sr_text );
			if ( rs->sr_err != LDAP_SUCCESS ) {
				Debug( LDAP_DEBUG_TRACE,
					"%s slap_modrdn2mods: %s: %s (old)\n",
					op->o_log_prefix,
					rs->sr_text, 
					old_rdn[d_cnt]->la_attr.bv_val );
				goto done;		
			}

			/* Apply modification */
			mod_tmp = ( Modifications * )ch_malloc( sizeof( Modifications ) );
			mod_tmp->sml_desc = desc;
			BER_BVZERO( &mod_tmp->sml_type );
			mod_tmp->sml_numvals = 1;
			mod_tmp->sml_values = ( BerVarray )ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mod_tmp->sml_values[0], &old_rdn[d_cnt]->la_value );
			mod_tmp->sml_values[1].bv_val = NULL;
			if( desc->ad_type->sat_equality->smr_normalize) {
				mod_tmp->sml_nvalues = ( BerVarray )ch_malloc( 2 * sizeof( struct berval ) );
				(void) (*desc->ad_type->sat_equality->smr_normalize)(
					SLAP_MR_EQUALITY|SLAP_MR_VALUE_OF_ASSERTION_SYNTAX,
					desc->ad_type->sat_syntax,
					desc->ad_type->sat_equality,
					&mod_tmp->sml_values[0],
					&mod_tmp->sml_nvalues[0], NULL );
				mod_tmp->sml_nvalues[1].bv_val = NULL;
			} else {
				mod_tmp->sml_nvalues = NULL;
			}
			mod_tmp->sml_op = LDAP_MOD_DELETE;
			mod_tmp->sml_flags = 0;
			mod_tmp->sml_next = op->orr_modlist;
			op->orr_modlist = mod_tmp;
		}
	}
	
done:

	/* LDAP v2 supporting correct attribute handling. */
	if ( rs->sr_err != LDAP_SUCCESS && op->orr_modlist != NULL ) {
		Modifications *tmp;

		for ( ; op->orr_modlist != NULL; op->orr_modlist = tmp ) {
			tmp = op->orr_modlist->sml_next;
			ch_free( op->orr_modlist );
		}
	}

	if ( new_rdn != NULL ) {
		ldap_rdnfree_x( new_rdn, op->o_tmpmemctx );
	}
	if ( old_rdn != NULL ) {
		ldap_rdnfree_x( old_rdn, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

