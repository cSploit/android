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
#include <ac/time.h>

#include "slap.h"
#include "lutil.h"


int
do_modify(
    Operation	*op,
    SlapReply	*rs )
{
	struct berval dn = BER_BVNULL;
	char		textbuf[ SLAP_TEXT_BUFLEN ];
	size_t		textlen = sizeof( textbuf );
#ifdef LDAP_DEBUG
	Modifications	*tmp;
#endif

	Debug( LDAP_DEBUG_TRACE, "%s do_modify\n",
		op->o_log_prefix, 0, 0 );
	/*
	 * Parse the modify request.  It looks like this:
	 *
	 *	ModifyRequest := [APPLICATION 6] SEQUENCE {
	 *		name	DistinguishedName,
	 *		mods	SEQUENCE OF SEQUENCE {
	 *			operation	ENUMERATED {
	 *				add	(0),
	 *				delete	(1),
	 *				replace	(2)
	 *			},
	 *			modification	SEQUENCE {
	 *				type	AttributeType,
	 *				values	SET OF AttributeValue
	 *			}
	 *		}
	 *	}
	 */

	if ( ber_scanf( op->o_ber, "{m" /*}*/, &dn ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	Debug( LDAP_DEBUG_ARGS, "%s do_modify: dn (%s)\n",
		op->o_log_prefix, dn.bv_val, 0 );

	rs->sr_err = slap_parse_modlist( op, rs, op->o_ber, &op->oq_modify );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: slap_parse_modlist failed err=%d msg=%s\n",
			op->o_log_prefix, rs->sr_err, rs->sr_text );
		send_ldap_result( op, rs );
		goto cleanup;
	}

	if( get_ctrls( op, rs, 1 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		/* get_ctrls has sent results.	Now clean up. */
		goto cleanup;
	}

	rs->sr_err = dnPrettyNormal( NULL, &dn, &op->o_req_dn, &op->o_req_ndn,
		op->o_tmpmemctx );
	if( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: invalid dn (%s)\n",
			op->o_log_prefix, dn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid DN" );
		goto cleanup;
	}

	op->orm_no_opattrs = 0;

#ifdef LDAP_DEBUG
	Debug( LDAP_DEBUG_ARGS, "%s modifications:\n",
			op->o_log_prefix, 0, 0 );

	for ( tmp = op->orm_modlist; tmp != NULL; tmp = tmp->sml_next ) {
		Debug( LDAP_DEBUG_ARGS, "\t%s: %s\n",
			tmp->sml_op == LDAP_MOD_ADD ? "add" :
				(tmp->sml_op == LDAP_MOD_INCREMENT ? "increment" :
				(tmp->sml_op == LDAP_MOD_DELETE ? "delete" :
					"replace")), tmp->sml_type.bv_val, 0 );

		if ( tmp->sml_values == NULL ) {
			Debug( LDAP_DEBUG_ARGS, "%s\n",
			   "\t\tno values", NULL, NULL );
		} else if ( BER_BVISNULL( &tmp->sml_values[ 0 ] ) ) {
			Debug( LDAP_DEBUG_ARGS, "%s\n",
			   "\t\tzero values", NULL, NULL );
		} else if ( BER_BVISNULL( &tmp->sml_values[ 1 ] ) ) {
			Debug( LDAP_DEBUG_ARGS, "%s, length %ld\n",
			   "\t\tone value", (long) tmp->sml_values[0].bv_len, NULL );
		} else {
			Debug( LDAP_DEBUG_ARGS, "%s\n",
			   "\t\tmultiple values", NULL, NULL );
		}
	}

	if ( StatslogTest( LDAP_DEBUG_STATS ) ) {
		char abuf[BUFSIZ/2], *ptr = abuf;
		int len = 0;

		Statslog( LDAP_DEBUG_STATS, "%s MOD dn=\"%s\"\n",
			op->o_log_prefix, op->o_req_dn.bv_val, 0, 0, 0 );

		for ( tmp = op->orm_modlist; tmp != NULL; tmp = tmp->sml_next ) {
			if (len + 1 + tmp->sml_type.bv_len > sizeof(abuf)) {
				Statslog( LDAP_DEBUG_STATS, "%s MOD attr=%s\n",
				    op->o_log_prefix, abuf, 0, 0, 0 );

				len = 0;
				ptr = abuf;

				if( 1 + tmp->sml_type.bv_len > sizeof(abuf)) {
					Statslog( LDAP_DEBUG_STATS, "%s MOD attr=%s\n",
						op->o_log_prefix, tmp->sml_type.bv_val, 0, 0, 0 );
					continue;
				}
			}
			if (len) {
				*ptr++ = ' ';
				len++;
			}
			ptr = lutil_strcopy(ptr, tmp->sml_type.bv_val);
			len += tmp->sml_type.bv_len;
		}
		if (len) {
			Statslog( LDAP_DEBUG_STATS, "%s MOD attr=%s\n",
	    			op->o_log_prefix, abuf, 0, 0, 0 );
		}
	}
#endif	/* LDAP_DEBUG */

	rs->sr_err = slap_mods_check( op, op->orm_modlist,
		&rs->sr_text, textbuf, textlen, NULL );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	op->o_bd = frontendDB;
	rs->sr_err = frontendDB->be_modify( op, rs );

#ifdef LDAP_X_TXN
	if( rs->sr_err == LDAP_X_TXN_SPECIFY_OKAY ) {
		/* skip cleanup */
		return rs->sr_err;
	}
#endif

cleanup:
	op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );
	if ( op->orm_modlist != NULL ) slap_mods_free( op->orm_modlist, 1 );

	return rs->sr_err;
}

int
fe_op_modify( Operation *op, SlapReply *rs )
{
	BackendDB	*op_be, *bd = op->o_bd;
	char		textbuf[ SLAP_TEXT_BUFLEN ];
	size_t		textlen = sizeof( textbuf );
	
	if ( BER_BVISEMPTY( &op->o_req_ndn ) ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: root dse!\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"modify upon the root DSE not supported" );
		goto cleanup;

	} else if ( bvmatch( &op->o_req_ndn, &frontendDB->be_schemandn ) ) {
		Debug( LDAP_DEBUG_ANY, "%s do_modify: subschema subentry!\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"modification of subschema subentry not supported" );
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
		if ( !rs->sr_ref ) {
			rs->sr_ref = default_referral;
		}

		if ( rs->sr_ref != NULL ) {
			rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );

			if ( rs->sr_ref != default_referral ) {
				ber_bvarray_free( rs->sr_ref );
			}

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
	if ( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* check for referrals */
	if ( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		goto cleanup;
	}

	rs->sr_err = slap_mods_obsolete_check( op, op->orm_modlist,
		&rs->sr_text, textbuf, textlen );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto cleanup;
	}

	/* check for modify/increment support */
	if ( op->orm_increment && !SLAP_INCREMENT( op->o_bd ) ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"modify/increment not supported in context" );
		goto cleanup;
	}

	/*
	 * do the modify if 1 && (2 || 3)
	 * 1) there is a modify function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the update_ndn.
	 */
	if ( op->o_bd->be_modify ) {
		/* do the update here */
		int repl_user = be_isupdate( op );

		/*
		 * Multimaster slapd does not have to check for replicator dn
		 * because it accepts each modify request
		 */
		if ( !SLAP_SINGLE_SHADOW(op->o_bd) || repl_user ) {
			int update = !BER_BVISEMPTY( &op->o_bd->be_update_ndn );

			op->o_bd = op_be;

			if ( !update ) {
				rs->sr_err = slap_mods_no_user_mod_check( op, op->orm_modlist,
					&rs->sr_text, textbuf, textlen );
				if ( rs->sr_err != LDAP_SUCCESS ) {
					send_ldap_result( op, rs );
					goto cleanup;
				}
			}
			op->o_bd->be_modify( op, rs );

		} else { /* send a referral */
			BerVarray defref = op->o_bd->be_update_refs
				? op->o_bd->be_update_refs : default_referral;
			if ( defref != NULL ) {
				rs->sr_ref = referral_rewrite( defref,
					NULL, &op->o_req_dn,
					LDAP_SCOPE_DEFAULT );
				if ( rs->sr_ref == NULL ) {
					/* FIXME: must duplicate, because
					 * overlays may muck with it */
					rs->sr_ref = defref;
				}
				rs->sr_err = LDAP_REFERRAL;
				send_ldap_result( op, rs );
				if ( rs->sr_ref != defref ) {
					ber_bvarray_free( rs->sr_ref );
				}

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
	op->o_bd = bd;
	return rs->sr_err;
}

/*
 * Obsolete constraint checking.
 */
int
slap_mods_obsolete_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	if( get_relax( op ) ) return LDAP_SUCCESS;

	for ( ; ml != NULL; ml = ml->sml_next ) {
		if ( is_at_obsolete( ml->sml_desc->ad_type ) &&
			(( ml->sml_op != LDAP_MOD_REPLACE &&
				ml->sml_op != LDAP_MOD_DELETE ) ||
					ml->sml_values != NULL ))
		{
			/*
			 * attribute is obsolete,
			 * only allow replace/delete with no values
			 */
			snprintf( textbuf, textlen,
				"%s: attribute is obsolete",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_CONSTRAINT_VIOLATION;
		}
	}

	return LDAP_SUCCESS;
}

/*
 * No-user-modification constraint checking.
 */
int
slap_mods_no_user_mod_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	for ( ; ml != NULL; ml = ml->sml_next ) {
		if ( !is_at_no_user_mod( ml->sml_desc->ad_type ) ) {
			continue;
		}

		if ( ml->sml_flags & SLAP_MOD_INTERNAL ) {
			continue;
		}

		if ( get_relax( op ) ) {
			if ( ml->sml_desc->ad_type->sat_flags & SLAP_AT_MANAGEABLE ) {
				ml->sml_flags |= SLAP_MOD_MANAGING;
				continue;
			}

			/* attribute not manageable */
			snprintf( textbuf, textlen,
				"%s: no-user-modification attribute not manageable",
				ml->sml_type.bv_val );

		} else {
			/* user modification disallowed */
			snprintf( textbuf, textlen,
				"%s: no user modification allowed",
				ml->sml_type.bv_val );
		}

		*text = textbuf;
		return LDAP_CONSTRAINT_VIOLATION;
	}

	return LDAP_SUCCESS;
}

int
slap_mods_no_repl_user_mod_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf,
	size_t textlen )
{
	Modifications *mods;
	Modifications *modp;

	for ( mods = ml; mods != NULL; mods = mods->sml_next ) {
		assert( mods->sml_op == LDAP_MOD_ADD );

		/* check doesn't already appear */
		for ( modp = ml; modp != NULL; modp = modp->sml_next ) {
			if ( mods->sml_desc == modp->sml_desc && mods != modp ) {
				snprintf( textbuf, textlen,
					"attribute '%s' provided more than once",
					mods->sml_desc->ad_cname.bv_val );
				*text = textbuf;
				return LDAP_TYPE_OR_VALUE_EXISTS;
			}
		}
	}

	return LDAP_SUCCESS;
}

/*
 * Do basic attribute type checking and syntax validation.
 */
int slap_mods_check(
	Operation *op,
	Modifications *ml,
	const char **text,
	char *textbuf,
	size_t textlen,
	void *ctx )
{
	int rc;

	for( ; ml != NULL; ml = ml->sml_next ) {
		AttributeDescription *ad = NULL;

		/* convert to attribute description */
		if ( ml->sml_desc == NULL ) {
			rc = slap_bv2ad( &ml->sml_type, &ml->sml_desc, text );
			if( rc != LDAP_SUCCESS ) {
				if ( get_no_schema_check( op )) {
					rc = slap_bv2undef_ad( &ml->sml_type, &ml->sml_desc,
						text, 0 );
				}
			}
			if( rc != LDAP_SUCCESS ) {
				snprintf( textbuf, textlen, "%s: %s",
					ml->sml_type.bv_val, *text );
				*text = textbuf;
				return rc;
			}
		}

		ad = ml->sml_desc;

		if( slap_syntax_is_binary( ad->ad_type->sat_syntax )
			&& !slap_ad_is_binary( ad ))
		{
			/* attribute requires binary transfer */
			snprintf( textbuf, textlen,
				"%s: requires ;binary transfer",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_UNDEFINED_TYPE;
		}

		if( !slap_syntax_is_binary( ad->ad_type->sat_syntax )
			&& slap_ad_is_binary( ad ))
		{
			/* attribute does not require binary transfer */
			snprintf( textbuf, textlen,
				"%s: disallows ;binary transfer",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_UNDEFINED_TYPE;
		}

		if( slap_ad_is_tag_range( ad )) {
			/* attribute requires binary transfer */
			snprintf( textbuf, textlen,
				"%s: inappropriate use of tag range option",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_UNDEFINED_TYPE;
		}

#if 0
		if ( is_at_obsolete( ad->ad_type ) &&
			(( ml->sml_op != LDAP_MOD_REPLACE &&
				ml->sml_op != LDAP_MOD_DELETE ) ||
					ml->sml_values != NULL ))
		{
			/*
			 * attribute is obsolete,
			 * only allow replace/delete with no values
			 */
			snprintf( textbuf, textlen,
				"%s: attribute is obsolete",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_CONSTRAINT_VIOLATION;
		}
#endif

		if ( ml->sml_op == LDAP_MOD_INCREMENT &&
#ifdef SLAPD_REAL_SYNTAX
			!is_at_syntax( ad->ad_type, SLAPD_REAL_SYNTAX ) &&
#endif
			!is_at_syntax( ad->ad_type, SLAPD_INTEGER_SYNTAX ) )
		{
			/*
			 * attribute values must be INTEGER or REAL
			 */
			snprintf( textbuf, textlen,
				"%s: attribute syntax inappropriate for increment",
				ml->sml_type.bv_val );
			*text = textbuf;
			return LDAP_CONSTRAINT_VIOLATION;
		}

		/*
		 * check values
		 */
		if( ml->sml_values != NULL ) {
			ber_len_t nvals;
			slap_syntax_validate_func *validate =
				ad->ad_type->sat_syntax->ssyn_validate;
			slap_syntax_transform_func *pretty =
				ad->ad_type->sat_syntax->ssyn_pretty;
 
			if( !pretty && !validate ) {
				*text = "no validator for syntax";
				snprintf( textbuf, textlen,
					"%s: no validator for syntax %s",
					ml->sml_type.bv_val,
					ad->ad_type->sat_syntax->ssyn_oid );
				*text = textbuf;
				return LDAP_INVALID_SYNTAX;
			}

			/*
			 * check that each value is valid per syntax
			 *	and pretty if appropriate
			 */
			for ( nvals = 0; !BER_BVISNULL( &ml->sml_values[nvals] ); nvals++ ) {
				struct berval pval;

				if ( pretty ) {
					rc = ordered_value_pretty( ad,
						&ml->sml_values[nvals], &pval, ctx );
				} else {
					rc = ordered_value_validate( ad,
						&ml->sml_values[nvals], ml->sml_op );
				}

				if( rc != 0 ) {
					snprintf( textbuf, textlen,
						"%s: value #%ld invalid per syntax",
						ml->sml_type.bv_val, (long) nvals );
					*text = textbuf;
					return LDAP_INVALID_SYNTAX;
				}

				if( pretty ) {
					ber_memfree_x( ml->sml_values[nvals].bv_val, ctx );
					ml->sml_values[nvals] = pval;
				}
			}
			ml->sml_values[nvals].bv_len = 0;
			ml->sml_numvals = nvals;

			/*
			 * a rough single value check... an additional check is needed
			 * to catch add of single value to existing single valued attribute
			 */
			if ((ml->sml_op == LDAP_MOD_ADD || ml->sml_op == LDAP_MOD_REPLACE)
				&& nvals > 1 && is_at_single_value( ad->ad_type ))
			{
				snprintf( textbuf, textlen,
					"%s: multiple values provided",
					ml->sml_type.bv_val );
				*text = textbuf;
				return LDAP_CONSTRAINT_VIOLATION;
			}

			/* if the type has a normalizer, generate the
			 * normalized values. otherwise leave them NULL.
			 *
			 * this is different from the rule for attributes
			 * in an entry - in an attribute list, the normalized
			 * value is set equal to the non-normalized value
			 * when there is no normalizer.
			 */
			if( nvals && ad->ad_type->sat_equality &&
				ad->ad_type->sat_equality->smr_normalize )
			{
				ml->sml_nvalues = ber_memalloc_x(
					(nvals+1)*sizeof(struct berval), ctx );

				for ( nvals = 0; !BER_BVISNULL( &ml->sml_values[nvals] ); nvals++ ) {
					rc = ordered_value_normalize(
						SLAP_MR_VALUE_OF_ATTRIBUTE_SYNTAX,
						ad,
						ad->ad_type->sat_equality,
						&ml->sml_values[nvals], &ml->sml_nvalues[nvals], ctx );
					if ( rc ) {
						Debug( LDAP_DEBUG_ANY,
							"<= str2entry NULL (ssyn_normalize %d)\n",
							rc, 0, 0 );
						snprintf( textbuf, textlen,
							"%s: value #%ld normalization failed",
							ml->sml_type.bv_val, (long) nvals );
						*text = textbuf;
						BER_BVZERO( &ml->sml_nvalues[nvals] );
						return rc;
					}
				}

				BER_BVZERO( &ml->sml_nvalues[nvals] );
			}

			/* check for duplicates, but ignore Deletes.
			 */
			if( nvals > 1 && ml->sml_op != LDAP_MOD_DELETE ) {
				int i;
				rc = slap_sort_vals( ml, text, &i, ctx );
				if ( rc == LDAP_TYPE_OR_VALUE_EXISTS ) {
					/* value exists already */
					snprintf( textbuf, textlen,
						"%s: value #%d provided more than once",
						ml->sml_desc->ad_cname.bv_val, i );
					*text = textbuf;
				}
				if ( rc )
					return rc;
			}
		} else {
			ml->sml_numvals = 0;
		}
	}

	return LDAP_SUCCESS;
}

/* Sort a set of values. An (Attribute *) may be used interchangeably here
 * instead of a (Modifications *) structure.
 *
 * Uses Quicksort + Insertion sort for small arrays
 */

int
slap_sort_vals(
	Modifications *ml,
	const char **text,
	int *dup,
	void *ctx )
{
	AttributeDescription *ad;
	MatchingRule *mr;
	int istack[sizeof(int)*16];
	int i, j, k, l, ir, jstack, match, *ix, itmp, nvals, rc = LDAP_SUCCESS;
	int is_norm;
	struct berval a, *cv;

#define SMALL	8
#define	SWAP(a,b,tmp)	tmp=(a);(a)=(b);(b)=tmp
#define	COMP(a,b)	match=0; rc = ordered_value_match( &match, \
						ad, mr, SLAP_MR_EQUALITY \
								| SLAP_MR_VALUE_OF_ASSERTION_SYNTAX \
								| SLAP_MR_ASSERTED_VALUE_NORMALIZED_MATCH \
								| SLAP_MR_ATTRIBUTE_VALUE_NORMALIZED_MATCH, \
								&(a), &(b), text );

#define	IX(x)	ix[x]
#define	EXCH(x,y)	SWAP(ix[x],ix[y],itmp)
#define	SETA(x)	itmp = ix[x]; a = cv[itmp]
#define	GETA(x)	ix[x] = itmp;
#define	SET(x,y)	ix[x] = ix[y]

	ad = ml->sml_desc;
	nvals = ml->sml_numvals;
	if ( nvals <= 1 )
		goto ret;

	/* For Modifications, sml_nvalues is NULL if normalization wasn't needed.
	 * For Attributes, sml_nvalues == sml_values when normalization isn't needed.
	 */
	if ( ml->sml_nvalues && ml->sml_nvalues != ml->sml_values ) {
		cv = ml->sml_nvalues;
		is_norm = 1;
	} else {
		cv = ml->sml_values;
		is_norm = 0;
	}

	if ( ad == slap_schema.si_ad_objectClass )
		mr = NULL;	/* shortcut matching */
	else
		mr = ad->ad_type->sat_equality;

	/* record indices to preserve input ordering */
	ix = slap_sl_malloc( nvals * sizeof(int), ctx );
	for (i=0; i<nvals; i++) ix[i] = i;

	ir = nvals-1;
	l = 0;
	jstack = 0;

	for(;;) {
		if (ir - l < SMALL) {	/* Insertion sort */
			match=1;
			for (j=l+1;j<=ir;j++) {
				SETA(j);
				for (i=j-1;i>=0;i--) {
					COMP(cv[IX(i)], a);
					if ( match <= 0 )
						break;
					SET(i+1,i);
				}
				GETA(i+1);
				if ( match == 0 ) goto done;
			}
			if ( jstack == 0 ) break;
			if ( match == 0 ) break;
			ir = istack[jstack--];
			l = istack[jstack--];
		} else {
			k = (l + ir) >> 1;	/* Choose median of left, center, right */
			EXCH(k, l+1);
			COMP( cv[IX(l)], cv[IX(ir)] );
			if ( match > 0 ) {
				EXCH(l, ir);
			} else if ( match == 0 ) {
				i = ir;
				break;
			}
			COMP( cv[IX(l+1)], cv[IX(ir)] );
			if ( match > 0 ) {
				EXCH(l+1, ir);
			} else if ( match == 0 ) {
				i = ir;
				break;
			}
			COMP( cv[IX(l)], cv[IX(l+1)] );
			if ( match > 0 ) {
				EXCH(l, l+1);
			} else if ( match == 0 ) {
				i = l;
				break;
			}
			i = l+1;
			j = ir;
			a = cv[IX(i)];
			for(;;) {
				do {
					i++;
					COMP( cv[IX(i)], a );
				} while( match < 0 );
				while( match > 0 ) {
					j--;
					COMP( cv[IX(j)], a );
				}
				if (j < i) {
					match = 1;
					break;
				}
				if ( match == 0 ) {
					i = l+1;
					break;
				}
				EXCH(i,j);
			}
			if ( match == 0 )
				break;
			EXCH(l+1,j);
			jstack += 2;
			if (ir-i+1 > j-l) {
				istack[jstack] = ir;
				istack[jstack-1] = i;
				ir = j;
			} else {
				istack[jstack] = j;
				istack[jstack-1] = l;
				l = i;
			}
		}
	}
	done:
	if ( match == 0 && i >= 0 )
		*dup = ix[i];

	/* For sorted attributes, put the values in index order */
	if ( rc == LDAP_SUCCESS && match &&
		( ad->ad_type->sat_flags & SLAP_AT_SORTED_VAL )) {
		BerVarray tmpv = slap_sl_malloc( sizeof( struct berval ) * nvals, ctx );
		for ( i = 0; i<nvals; i++ )
			tmpv[i] = cv[ix[i]];
		for ( i = 0; i<nvals; i++ )
			cv[i] = tmpv[i];
		/* Check if the non-normalized array needs to move too */
		if ( is_norm ) {
			cv = ml->sml_values;
			for ( i = 0; i<nvals; i++ )
				tmpv[i] = cv[ix[i]];
			for ( i = 0; i<nvals; i++ )
				cv[i] = tmpv[i];
		}
		slap_sl_free( tmpv, ctx );
	}

	slap_sl_free( ix, ctx );

	if ( rc == LDAP_SUCCESS && match == 0 ) {
		/* value exists already */
		assert( i >= 0 );
		assert( i < nvals );
		rc = LDAP_TYPE_OR_VALUE_EXISTS;
	}
 ret:
	return rc;
}

/* Enter with bv->bv_len = sizeof buffer, returns with
 * actual length of string
 */
void slap_timestamp( time_t *tm, struct berval *bv )
{
	struct tm ltm;

	ldap_pvt_gmtime( tm, &ltm );

	bv->bv_len = lutil_gentime( bv->bv_val, bv->bv_len, &ltm );
}

/* Called for all modify and modrdn ops. If the current op was replicated
 * from elsewhere, all of the attrs should already be present.
 */
void slap_mods_opattrs(
	Operation *op,
	Modifications **modsp,
	int manage_ctxcsn )
{
	struct berval name, timestamp, csn = BER_BVNULL;
	struct berval nname;
	char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
	Modifications *mod, **modtail, *modlast;
	int gotcsn = 0, gotmname = 0, gotmtime = 0;

	if ( SLAP_LASTMOD( op->o_bd ) && !op->orm_no_opattrs ) {
		char *ptr;
		timestamp.bv_val = timebuf;
		for ( modtail = modsp; *modtail; modtail = &(*modtail)->sml_next ) {
			if ( (*modtail)->sml_op != LDAP_MOD_ADD &&
				(*modtail)->sml_op != SLAP_MOD_SOFTADD &&
				(*modtail)->sml_op != SLAP_MOD_ADD_IF_NOT_PRESENT &&
				(*modtail)->sml_op != LDAP_MOD_REPLACE )
			{
				continue;
			}

			if ( (*modtail)->sml_desc == slap_schema.si_ad_entryCSN )
			{
				csn = (*modtail)->sml_values[0];
				gotcsn = 1;

			} else if ( (*modtail)->sml_desc == slap_schema.si_ad_modifiersName )
			{
				gotmname = 1;

			} else if ( (*modtail)->sml_desc == slap_schema.si_ad_modifyTimestamp )
			{
				gotmtime = 1;
			}
		}

		if ( BER_BVISEMPTY( &op->o_csn )) {
			if ( !gotcsn ) {
				csn.bv_val = csnbuf;
				csn.bv_len = sizeof( csnbuf );
				slap_get_csn( op, &csn, manage_ctxcsn );

			} else {
				if ( manage_ctxcsn ) {
					slap_queue_csn( op, &csn );
				}
			}

		} else {
			csn = op->o_csn;
		}

		ptr = ber_bvchr( &csn, '#' );
		if ( ptr ) {
			timestamp.bv_len = STRLENOF("YYYYMMDDHHMMSSZ");
			AC_MEMCPY( timebuf, csn.bv_val, timestamp.bv_len );
			timebuf[timestamp.bv_len-1] = 'Z';
			timebuf[timestamp.bv_len] = '\0';

		} else {
			time_t now = slap_get_time();

			timestamp.bv_len = sizeof(timebuf);

			slap_timestamp( &now, &timestamp );
		}

		if ( BER_BVISEMPTY( &op->o_dn ) ) {
			BER_BVSTR( &name, SLAPD_ANONYMOUS );
			nname = name;

		} else {
			name = op->o_dn;
			nname = op->o_ndn;
		}

		if ( !gotcsn ) {
			mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
			mod->sml_op = LDAP_MOD_REPLACE;
			mod->sml_flags = SLAP_MOD_INTERNAL;
			mod->sml_next = NULL;
			BER_BVZERO( &mod->sml_type );
			mod->sml_desc = slap_schema.si_ad_entryCSN;
			mod->sml_numvals = 1;
			mod->sml_values = (BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mod->sml_values[0], &csn );
			BER_BVZERO( &mod->sml_values[1] );
			assert( !BER_BVISNULL( &mod->sml_values[0] ) );
			mod->sml_nvalues = NULL;
			*modtail = mod;
			modlast = mod;
			modtail = &mod->sml_next;
		}

		if ( !gotmname ) {
			mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
			mod->sml_op = LDAP_MOD_REPLACE;
			mod->sml_flags = SLAP_MOD_INTERNAL;
			mod->sml_next = NULL;
			BER_BVZERO( &mod->sml_type );
			mod->sml_desc = slap_schema.si_ad_modifiersName;
			mod->sml_numvals = 1;
			mod->sml_values = (BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mod->sml_values[0], &name );
			BER_BVZERO( &mod->sml_values[1] );
			assert( !BER_BVISNULL( &mod->sml_values[0] ) );
			mod->sml_nvalues =
				(BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mod->sml_nvalues[0], &nname );
			BER_BVZERO( &mod->sml_nvalues[1] );
			assert( !BER_BVISNULL( &mod->sml_nvalues[0] ) );
			*modtail = mod;
			modtail = &mod->sml_next;
		}

		if ( !gotmtime ) {
			mod = (Modifications *) ch_malloc( sizeof( Modifications ) );
			mod->sml_op = LDAP_MOD_REPLACE;
			mod->sml_flags = SLAP_MOD_INTERNAL;
			mod->sml_next = NULL;
			BER_BVZERO( &mod->sml_type );
			mod->sml_desc = slap_schema.si_ad_modifyTimestamp;
			mod->sml_numvals = 1;
			mod->sml_values = (BerVarray) ch_malloc( 2 * sizeof( struct berval ) );
			ber_dupbv( &mod->sml_values[0], &timestamp );
			BER_BVZERO( &mod->sml_values[1] );
			assert( !BER_BVISNULL( &mod->sml_values[0] ) );
			mod->sml_nvalues = NULL;
			*modtail = mod;
			modtail = &mod->sml_next;
		}
	}
}

int
slap_parse_modlist(
	Operation *op,
	SlapReply *rs,
	BerElement *ber,
	req_modify_s *ms )
{
	ber_tag_t	tag;
	ber_len_t	len;
	char		*last;
	Modifications	**modtail = &ms->rs_mods.rs_modlist;

	ms->rs_mods.rs_modlist = NULL;
	ms->rs_increment = 0;

	rs->sr_err = LDAP_SUCCESS;

	/* collect modifications & save for later */
	for ( tag = ber_first_element( ber, &len, &last );
		tag != LBER_DEFAULT;
		tag = ber_next_element( ber, &len, last ) )
	{
		ber_int_t mop;
		Modifications tmp, *mod;

		tmp.sml_nvalues = NULL;

		if ( ber_scanf( ber, "{e{m[W]}}", &mop,
		    &tmp.sml_type, &tmp.sml_values ) == LBER_ERROR )
		{
			rs->sr_text = "decoding modlist error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		mod = (Modifications *) ch_malloc( sizeof(Modifications) );
		mod->sml_op = mop;
		mod->sml_flags = 0;
		mod->sml_type = tmp.sml_type;
		mod->sml_values = tmp.sml_values;
		mod->sml_nvalues = NULL;
		mod->sml_desc = NULL;
		mod->sml_next = NULL;
		*modtail = mod;

		switch( mop ) {
		case LDAP_MOD_ADD:
			if ( mod->sml_values == NULL ) {
				rs->sr_text = "modify/add operation requires values";
				rs->sr_err = LDAP_PROTOCOL_ERROR;
				goto done;
			}

			/* fall through */

		case LDAP_MOD_DELETE:
		case LDAP_MOD_REPLACE:
			break;

		case LDAP_MOD_INCREMENT:
			if( op->o_protocol >= LDAP_VERSION3 ) {
				ms->rs_increment++;
				if ( mod->sml_values == NULL ) {
					rs->sr_text = "modify/increment operation requires value";
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					goto done;
				}

				if ( !BER_BVISNULL( &mod->sml_values[ 1 ] ) ) {
					rs->sr_text = "modify/increment operation requires single value";
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					goto done;
				}

				break;
			}
			/* fall thru */

		default:
			rs->sr_text = "unrecognized modify operation";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		modtail = &mod->sml_next;
	}
	*modtail = NULL;

done:
	if ( rs->sr_err != LDAP_SUCCESS ) {
		slap_mods_free( ms->rs_mods.rs_modlist, 1 );
		ms->rs_mods.rs_modlist = NULL;
		ms->rs_increment = 0;
	}

	return rs->sr_err;
}

