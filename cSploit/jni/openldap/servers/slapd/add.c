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
#include <ac/string.h>
#include <ac/time.h>
#include <ac/socket.h>

#include "lutil.h"
#include "slap.h"

int
do_add( Operation *op, SlapReply *rs )
{
	BerElement	*ber = op->o_ber;
	char		*last;
	struct berval	dn = BER_BVNULL;
	ber_len_t	len;
	ber_tag_t	tag;
	Modifications	*modlist = NULL;
	Modifications	**modtail = &modlist;
	Modifications	tmp;
	char		textbuf[ SLAP_TEXT_BUFLEN ];
	size_t		textlen = sizeof( textbuf );
	int		rc = 0;
	int		freevals = 1;
	OpExtraDB oex;

	Debug( LDAP_DEBUG_TRACE, "%s do_add\n",
		op->o_log_prefix, 0, 0 );

	/*
	 * Parse the add request.  It looks like this:
	 *
	 *	AddRequest := [APPLICATION 14] SEQUENCE {
	 *		name	DistinguishedName,
	 *		attrs	SEQUENCE OF SEQUENCE {
	 *			type	AttributeType,
	 *			values	SET OF AttributeValue
	 *		}
	 *	}
	 */

	/* get the name */
	if ( ber_scanf( ber, "{m", /*}*/ &dn ) == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_add: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		return SLAPD_DISCONNECT;
	}

	Debug( LDAP_DEBUG_ARGS, "%s do_add: dn (%s)\n",
		op->o_log_prefix, dn.bv_val, 0 );

	/* get the attrs */
	for ( tag = ber_first_element( ber, &len, &last ); tag != LBER_DEFAULT;
	    tag = ber_next_element( ber, &len, last ) )
	{
		Modifications *mod;
		ber_tag_t rtag;

		tmp.sml_nvalues = NULL;

		rtag = ber_scanf( ber, "{m{W}}", &tmp.sml_type, &tmp.sml_values );

		if ( rtag == LBER_ERROR ) {
			Debug( LDAP_DEBUG_ANY, "%s do_add: decoding error\n",
				op->o_log_prefix, 0, 0 );
			send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
			rs->sr_err = SLAPD_DISCONNECT;
			goto done;
		}

		if ( tmp.sml_values == NULL ) {
			Debug( LDAP_DEBUG_ANY, "%s do_add: no values for type %s\n",
				op->o_log_prefix, tmp.sml_type.bv_val, 0 );
			send_ldap_error( op, rs, LDAP_PROTOCOL_ERROR,
				"no values for attribute type" );
			goto done;
		}

		mod  = (Modifications *) ch_malloc( sizeof(Modifications) );
		mod->sml_op = LDAP_MOD_ADD;
		mod->sml_flags = 0;
		mod->sml_next = NULL;
		mod->sml_desc = NULL;
		mod->sml_type = tmp.sml_type;
		mod->sml_values = tmp.sml_values;
		mod->sml_nvalues = NULL;

		*modtail = mod;
		modtail = &mod->sml_next;
	}

	if ( ber_scanf( ber, /*{*/ "}") == LBER_ERROR ) {
		Debug( LDAP_DEBUG_ANY, "%s do_add: ber_scanf failed\n",
			op->o_log_prefix, 0, 0 );
		send_ldap_discon( op, rs, LDAP_PROTOCOL_ERROR, "decoding error" );
		rs->sr_err = SLAPD_DISCONNECT;
		goto done;
	}

	if ( get_ctrls( op, rs, 1 ) != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_add: get_ctrls failed\n",
			op->o_log_prefix, 0, 0 );
		goto done;
	} 

	rs->sr_err = dnPrettyNormal( NULL, &dn, &op->o_req_dn, &op->o_req_ndn,
		op->o_tmpmemctx );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "%s do_add: invalid dn (%s)\n",
			op->o_log_prefix, dn.bv_val, 0 );
		send_ldap_error( op, rs, LDAP_INVALID_DN_SYNTAX, "invalid DN" );
		goto done;
	}

	op->ora_e = entry_alloc();
	ber_dupbv( &op->ora_e->e_name, &op->o_req_dn );
	ber_dupbv( &op->ora_e->e_nname, &op->o_req_ndn );

	Statslog( LDAP_DEBUG_STATS, "%s ADD dn=\"%s\"\n",
	    op->o_log_prefix, op->o_req_dn.bv_val, 0, 0, 0 );

	if ( modlist == NULL ) {
		send_ldap_error( op, rs, LDAP_PROTOCOL_ERROR,
			"no attributes provided" );
		goto done;
	}

	if ( dn_match( &op->ora_e->e_nname, &slap_empty_bv ) ) {
		/* protocolError may be a more appropriate error */
		send_ldap_error( op, rs, LDAP_ALREADY_EXISTS,
			"root DSE already exists" );
		goto done;

	} else if ( dn_match( &op->ora_e->e_nname, &frontendDB->be_schemandn ) ) {
		send_ldap_error( op, rs, LDAP_ALREADY_EXISTS,
			"subschema subentry already exists" );
		goto done;
	}

	rs->sr_err = slap_mods_check( op, modlist, &rs->sr_text,
		textbuf, textlen, NULL );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto done;
	}

	/* temporary; remove if not invoking backend function */
	op->ora_modlist = modlist;

	/* call this so global overlays/SLAPI have access to ora_e */
	rs->sr_err = slap_mods2entry( op->ora_modlist, &op->ora_e,
		1, 0, &rs->sr_text, textbuf, textlen );
	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto done;
	}

	freevals = 0;

	oex.oe.oe_key = (void *)do_add;
	oex.oe_db = NULL;
	LDAP_SLIST_INSERT_HEAD(&op->o_extra, &oex.oe, oe_next);

	op->o_bd = frontendDB;
	rc = frontendDB->be_add( op, rs );
	LDAP_SLIST_REMOVE(&op->o_extra, &oex.oe, OpExtra, oe_next);

#ifdef LDAP_X_TXN
	if ( rc == LDAP_X_TXN_SPECIFY_OKAY ) {
		/* skip cleanup */
		return rc;
	} else
#endif
	if ( rc == 0 ) {
		if ( op->ora_e != NULL && oex.oe_db != NULL ) {
			BackendDB	*bd = op->o_bd;

			op->o_bd = oex.oe_db;

			be_entry_release_w( op, op->ora_e );

			op->ora_e = NULL;
			op->o_bd = bd;
		}
	}

done:;
	if ( modlist != NULL ) {
		/* in case of error, free the values as well */
		slap_mods_free( modlist, freevals );
	}

	if ( op->ora_e != NULL ) {
		entry_free( op->ora_e );
	}
	op->o_tmpfree( op->o_req_dn.bv_val, op->o_tmpmemctx );
	op->o_tmpfree( op->o_req_ndn.bv_val, op->o_tmpmemctx );

	return rc;
}

int
fe_op_add( Operation *op, SlapReply *rs )
{
	Modifications	**modtail = &op->ora_modlist;
	int		rc = 0;
	BackendDB	*op_be, *bd = op->o_bd;
	char		textbuf[ SLAP_TEXT_BUFLEN ];
	size_t		textlen = sizeof( textbuf );

	/*
	 * We could be serving multiple database backends.  Select the
	 * appropriate one, or send a referral to our "referral server"
	 * if we don't hold it.
	 */
	op->o_bd = select_backend( &op->ora_e->e_nname, 1 );
	if ( op->o_bd == NULL ) {
		op->o_bd = bd;
		rs->sr_ref = referral_rewrite( default_referral,
			NULL, &op->ora_e->e_name, LDAP_SCOPE_DEFAULT );
		if ( !rs->sr_ref ) rs->sr_ref = default_referral;
		if ( rs->sr_ref ) {
			rs->sr_err = LDAP_REFERRAL;
			send_ldap_result( op, rs );

			if ( rs->sr_ref != default_referral ) {
				ber_bvarray_free( rs->sr_ref );
			}
		} else {
			send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
				"no global superior knowledge" );
		}
		goto done;
	}

	/* If we've got a glued backend, check the real backend */
	op_be = op->o_bd;
	if ( SLAP_GLUE_INSTANCE( op->o_bd )) {
		op->o_bd = select_backend( &op->ora_e->e_nname, 0 );
	}

	/* check restrictions */
	if( backend_check_restrictions( op, rs, NULL ) != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto done;
	}

	/* check for referrals */
	if( backend_check_referrals( op, rs ) != LDAP_SUCCESS ) {
		goto done;
	}

	rs->sr_err = slap_mods_obsolete_check( op, op->ora_modlist,
		&rs->sr_text, textbuf, textlen );

	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
		goto done;
	}

	/*
	 * do the add if 1 && (2 || 3)
	 * 1) there is an add function implemented in this backend;
	 * 2) this backend is master for what it holds;
	 * 3) it's a replica and the dn supplied is the updatedn.
	 */
	if ( op->o_bd->be_add ) {
		/* do the update here */
		int repl_user = be_isupdate( op );
		if ( !SLAP_SINGLE_SHADOW(op->o_bd) || repl_user ) {
			int		update = !BER_BVISEMPTY( &op->o_bd->be_update_ndn );

			op->o_bd = op_be;

			if ( !update ) {
				rs->sr_err = slap_mods_no_user_mod_check( op, op->ora_modlist,
					&rs->sr_text, textbuf, textlen );

				if ( rs->sr_err != LDAP_SUCCESS ) {
					send_ldap_result( op, rs );
					goto done;
				}
			}

			if ( !repl_user ) {
				/* go to the last mod */
				for ( modtail = &op->ora_modlist;
						*modtail != NULL;
						modtail = &(*modtail)->sml_next )
				{
					assert( (*modtail)->sml_op == LDAP_MOD_ADD );
					assert( (*modtail)->sml_desc != NULL );
				}


				/* check for unmodifiable attributes */
				rs->sr_err = slap_mods_no_repl_user_mod_check( op,
					op->ora_modlist, &rs->sr_text, textbuf, textlen );
				if ( rs->sr_err != LDAP_SUCCESS ) {
					send_ldap_result( op, rs );
					goto done;
				}
			}

			rc = op->o_bd->be_add( op, rs );
			if ( rc == LDAP_SUCCESS ) {
				OpExtra *oex;
				/* NOTE: be_entry_release_w() is
				 * called by do_add(), so that global
				 * overlays on the way back can
				 * at least read the entry */
				LDAP_SLIST_FOREACH(oex, &op->o_extra, oe_next) {
					if ( oex->oe_key == (void *)do_add ) {
						((OpExtraDB *)oex)->oe_db = op->o_bd;
						break;
					}
				}
			}

		} else {
			BerVarray defref = NULL;

			defref = op->o_bd->be_update_refs
				? op->o_bd->be_update_refs : default_referral;

			if ( defref != NULL ) {
				rs->sr_ref = referral_rewrite( defref,
					NULL, &op->ora_e->e_name, LDAP_SCOPE_DEFAULT );
				if ( rs->sr_ref == NULL ) rs->sr_ref = defref;
				rs->sr_err = LDAP_REFERRAL;
				if (!rs->sr_ref) rs->sr_ref = default_referral;
				send_ldap_result( op, rs );

				if ( rs->sr_ref != default_referral ) {
					ber_bvarray_free( rs->sr_ref );
				}
			} else {
				send_ldap_error( op, rs,
					LDAP_UNWILLING_TO_PERFORM,
					"shadow context; no update referral" );
			}
		}
	} else {
		Debug( LDAP_DEBUG_ARGS, "do_add: no backend support\n", 0, 0, 0 );
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
			"operation not supported within namingContext" );
	}

done:;
	op->o_bd = bd;
	return rc;
}

int
slap_mods2entry(
	Modifications *mods,
	Entry **e,
	int initial,
	int dup,
	const char **text,
	char *textbuf, size_t textlen )
{
	Attribute **tail;
	int i;

	if ( initial ) {
		assert( (*e)->e_attrs == NULL );
	}

	for ( tail = &(*e)->e_attrs; *tail != NULL; tail = &(*tail)->a_next )
		;

	*text = textbuf;

	for( ; mods != NULL; mods = mods->sml_next ) {
		Attribute *attr;

		assert( mods->sml_desc != NULL );

		attr = attr_find( (*e)->e_attrs, mods->sml_desc );

		if( attr != NULL ) {
#define SLURPD_FRIENDLY
#ifdef SLURPD_FRIENDLY
			int j;

			if ( !initial ) {
				/*	
				 * This check allows overlays to override operational
				 * attributes by setting them directly in the entry.
				 * We assume slap_mods_no_user_mod_check() was called
				 * with the user modifications.
				 */
				*text = NULL;
				return LDAP_SUCCESS;
			}

			i = attr->a_numvals;
			j = mods->sml_numvals;
			attr->a_numvals += j;
			j++;	/* NULL */
			
			attr->a_vals = ch_realloc( attr->a_vals,
				sizeof( struct berval ) * (i+j) );

			/* checked for duplicates in slap_mods_check */

			if ( dup ) {
				for ( j = 0; mods->sml_values[j].bv_val; j++ ) {
					ber_dupbv( &attr->a_vals[i+j], &mods->sml_values[j] );
				}
				BER_BVZERO( &attr->a_vals[i+j] );
				j++;
			} else {
				AC_MEMCPY( &attr->a_vals[i], mods->sml_values,
					sizeof( struct berval ) * j );
			}

			if( mods->sml_nvalues ) {
				attr->a_nvals = ch_realloc( attr->a_nvals,
					sizeof( struct berval ) * (i+j) );
				if ( dup ) {
					for ( j = 0; mods->sml_nvalues[j].bv_val; j++ ) {
						ber_dupbv( &attr->a_nvals[i+j], &mods->sml_nvalues[j] );
					}
					BER_BVZERO( &attr->a_nvals[i+j] );	
				} else {
					AC_MEMCPY( &attr->a_nvals[i], mods->sml_nvalues,
						sizeof( struct berval ) * j );
				}
			} else {
				attr->a_nvals = attr->a_vals;
			}

			continue;
#else
			snprintf( textbuf, textlen,
				"attribute '%s' provided more than once",
				mods->sml_desc->ad_cname.bv_val );
			*text = textbuf;
			return LDAP_TYPE_OR_VALUE_EXISTS;
#endif
		}

		attr = attr_alloc( mods->sml_desc );

		/* move values to attr structure */
		i = mods->sml_numvals;
		attr->a_numvals = mods->sml_numvals;
		if ( dup ) { 
			attr->a_vals = (BerVarray) ch_calloc( i+1, sizeof( BerValue ));
			for ( i = 0; mods->sml_values[i].bv_val; i++ ) {
				ber_dupbv( &attr->a_vals[i], &mods->sml_values[i] );
			}
			BER_BVZERO( &attr->a_vals[i] );
		} else {
			attr->a_vals = mods->sml_values;
		}

		if ( mods->sml_nvalues ) {
			if ( dup ) {
				i = mods->sml_numvals;
				attr->a_nvals = (BerVarray) ch_calloc( i+1, sizeof( BerValue ));
				for ( i = 0; mods->sml_nvalues[i].bv_val; i++ ) {
					ber_dupbv( &attr->a_nvals[i], &mods->sml_nvalues[i] );
				}
				BER_BVZERO( &attr->a_nvals[i] );
			} else {
				attr->a_nvals = mods->sml_nvalues;
			}
		} else {
			attr->a_nvals = attr->a_vals;
		}

		*tail = attr;
		tail = &attr->a_next;
	}

	*text = NULL;

	return LDAP_SUCCESS;
}

int
slap_entry2mods(
	Entry *e,
	Modifications **mods,
	const char **text,
	char *textbuf, size_t textlen )
{
	Modifications	*modhead = NULL;
	Modifications	*mod;
	Modifications	**modtail = &modhead;
	Attribute		*a_new;
	AttributeDescription	*a_new_desc;
	int				i, count;

	a_new = e->e_attrs;

	while ( a_new != NULL ) {
		a_new_desc = a_new->a_desc;
		mod = (Modifications *) ch_malloc( sizeof( Modifications ));
		
		mod->sml_op = LDAP_MOD_REPLACE;
		mod->sml_flags = 0;

		mod->sml_type = a_new_desc->ad_cname;

		count = a_new->a_numvals;
		mod->sml_numvals = a_new->a_numvals;

		mod->sml_values = (struct berval*) ch_malloc(
			(count+1) * sizeof( struct berval) );

		/* see slap_mods_check() comments...
		 * if a_vals == a_nvals, there is no normalizer.
		 * in this case, mod->sml_nvalues must be left NULL.
		 */
		if ( a_new->a_vals != a_new->a_nvals ) {
			mod->sml_nvalues = (struct berval*) ch_malloc(
				(count+1) * sizeof( struct berval) );
		} else {
			mod->sml_nvalues = NULL;
		}

		for ( i = 0; i < count; i++ ) {
			ber_dupbv(mod->sml_values+i, a_new->a_vals+i); 
			if ( mod->sml_nvalues ) {
				ber_dupbv( mod->sml_nvalues+i, a_new->a_nvals+i ); 
			} 
		}

		mod->sml_values[count].bv_val = NULL; 
		mod->sml_values[count].bv_len = 0; 

		if ( mod->sml_nvalues ) {
			mod->sml_nvalues[count].bv_val = NULL; 
			mod->sml_nvalues[count].bv_len = 0; 
		}

		mod->sml_desc = a_new_desc;
		mod->sml_next =NULL;
		*modtail = mod;
		modtail = &mod->sml_next;
		a_new = a_new->a_next; 
	}

	*mods = modhead;

	return LDAP_SUCCESS;
}

int slap_add_opattrs(
	Operation *op,
	const char **text,
	char *textbuf,
	size_t textlen,
	int manage_ctxcsn )
{
	struct berval name, timestamp, csn = BER_BVNULL;
	struct berval nname, tmp;
	char timebuf[ LDAP_LUTIL_GENTIME_BUFSIZE ];
	char csnbuf[ LDAP_PVT_CSNSTR_BUFSIZE ];
	Attribute *a;

	if ( SLAP_LASTMOD( op->o_bd ) ) {
		char *ptr;
		int gotcsn = 0;

		timestamp.bv_val = timebuf;
		a = attr_find( op->ora_e->e_attrs, slap_schema.si_ad_entryCSN );
		if ( a ) {
			gotcsn = 1;
			csn = a->a_vals[0];
		}
		if ( BER_BVISEMPTY( &op->o_csn )) {
			if ( !gotcsn ) {
				csn.bv_val = csnbuf;
				csn.bv_len = sizeof(csnbuf);
				slap_get_csn( op, &csn, manage_ctxcsn );
			} else {
				if ( manage_ctxcsn )
					slap_queue_csn( op, &csn );
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

		a = attr_find( op->ora_e->e_attrs,
			slap_schema.si_ad_entryUUID );
		if ( !a ) {
			char uuidbuf[ LDAP_LUTIL_UUIDSTR_BUFSIZE ];

			tmp.bv_len = lutil_uuidstr( uuidbuf, sizeof( uuidbuf ) );
			tmp.bv_val = uuidbuf;
			
			attr_merge_normalize_one( op->ora_e,
				slap_schema.si_ad_entryUUID, &tmp, op->o_tmpmemctx );
		}

		a = attr_find( op->ora_e->e_attrs,
			slap_schema.si_ad_creatorsName );
		if ( !a ) {
			attr_merge_one( op->ora_e,
				slap_schema.si_ad_creatorsName, &name, &nname );
		}

		a = attr_find( op->ora_e->e_attrs,
			slap_schema.si_ad_createTimestamp );
		if ( !a ) {
			attr_merge_one( op->ora_e,
				slap_schema.si_ad_createTimestamp, &timestamp, NULL );
		}

		if ( !gotcsn ) {
			attr_merge_one( op->ora_e,
				slap_schema.si_ad_entryCSN, &csn, NULL );
		}

		a = attr_find( op->ora_e->e_attrs,
			slap_schema.si_ad_modifiersName );
		if ( !a ) {
			attr_merge_one( op->ora_e,
				slap_schema.si_ad_modifiersName, &name, &nname );
		}

		a = attr_find( op->ora_e->e_attrs,
			slap_schema.si_ad_modifyTimestamp );
		if ( !a ) {
			attr_merge_one( op->ora_e,
				slap_schema.si_ad_modifyTimestamp, &timestamp, NULL );
		}
	}

	return LDAP_SUCCESS;
}
