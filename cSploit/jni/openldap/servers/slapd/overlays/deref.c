/* deref.c - dereference overlay */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 2008 Pierangelo Masarati.
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
 * This work was initially developed by Pierangelo Masarati
 * for inclusion in OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_DEREF

#include <stdio.h>

#include "ac/string.h"
#include "ac/socket.h"

#include "slap.h"
#include "config.h"

#include "lutil.h"

/*
 * 1. Specification
 *
 * 1.1. Request
 *
 *  controlValue ::= SEQUENCE OF derefSpec DerefSpec
 *
 *  DerefSpec ::= SEQUENCE {
 *      derefAttr       attributeDescription,    ; DN-valued
 *      attributes      AttributeList }
 *
 *  AttributeList ::= SEQUENCE OF attr AttributeDescription
 *
 *  derefAttr MUST be unique within controlValue
 *
 *
 * 1.2. Response
 *
 *  controlValue ::= SEQUENCE OF DerefRes
 *
 * From RFC 4511:
 *      PartialAttribute ::= SEQUENCE {
 *           type       AttributeDescription,
 *           vals       SET OF value AttributeValue }
 *
 *      PartialAttributeList ::= SEQUENCE OF
 *                           partialAttribute PartialAttribute
 *
 *  DerefRes ::= SEQUENCE {
 *      derefAttr       AttributeDescription,
 *      derefVal        LDAPDN,
 *      attrVals        [0] PartialAttributeList OPTIONAL }
 *
 *  If vals is empty, partialAttribute is omitted.
 *  If all vals in attrVals are empty, attrVals is omitted.
 *      
 * 2. Examples
 *
 * 2.1. Example
 *
 * 2.1.1. Request
 *
 * { { member, { GUID, SID } }, { memberOf, { GUID, SID } } }
 *
 * 2.1.2. Response
 *
 * { { memberOf, "cn=abartlet,cn=users,dc=abartlet,dc=net",
 *     { { GUID, [ "0bc11d00-e431-40a0-8767-344a320142fa" ] },
 *       { SID, [ "S-1-2-3-2345" ] } } },
 *   { memberOf, "cn=ando,cn=users,dc=sys-net,dc=it",
 *     { { GUID, [ "0bc11d00-e431-40a0-8767-344a320142fb" ] },
 *       { SID, [ "S-1-2-3-2346" ] } } } }
 *
 * 2.2. Example
 *
 * 2.2.1. Request
 *
 * { { member, { cn, uid, drink } } }
 *
 * 2.2.2. Response
 *
 * { { member, "cn=ando,cn=users,dc=sys-net,dc=it",
 *     { { cn, [ "ando", "Pierangelo Masarati" ] },
 *       { uid, [ "ando" ] } } },
 *   { member, "dc=sys-net,dc=it" } }
 *
 *
 * 3. Security considerations
 *
 * The control result must not disclose information the client's
 * identity could not have accessed directly by performing the related
 * search operations.  The presence of a derefVal in the control
 * response does not imply neither the existence of nor any access
 * privilege to the corresponding entry.  It is merely a consequence
 * of the read access the client's identity has on the corresponding
 * attribute's value.
 */

#define o_deref			o_ctrlflag[deref_cid]
#define o_ctrlderef		o_controls[deref_cid]

typedef struct DerefSpec {
	AttributeDescription	*ds_derefAttr;
	AttributeDescription	**ds_attributes;
	int			ds_nattrs;
	struct DerefSpec	*ds_next;
} DerefSpec;

typedef struct DerefVal {
	struct berval	dv_derefSpecVal;
	BerVarray	*dv_attrVals;
} DerefVal;

typedef struct DerefRes {
	DerefSpec		dr_spec;
	DerefVal		*dr_vals;
	struct DerefRes		*dr_next;
} DerefRes;

typedef struct deref_cb_t {
	slap_overinst *dc_on;
	DerefSpec *dc_ds;
} deref_cb_t;

static int			deref_cid;
static slap_overinst 		deref;
static int ov_count;

static int
deref_parseCtrl (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	ber_tag_t tag;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_len_t len;
	char *last;
	DerefSpec *dshead = NULL, **dsp = &dshead;
	BerVarray attributes = NULL;

	if ( op->o_deref != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Dereference control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Dereference control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Dereference control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_init2( ber, &ctrl->ldctl_value, 0 );

	for ( tag = ber_first_element( ber, &len, &last );
		tag != LBER_DEFAULT;
		tag = ber_next_element( ber, &len, last ) )
	{
		struct berval derefAttr;
		DerefSpec *ds, *dstmp;
		const char *text;
		int rc;
		ber_len_t cnt = sizeof(struct berval);
		ber_len_t off = 0;

		if ( ber_scanf( ber, "{m{M}}", &derefAttr, &attributes, &cnt, off ) == LBER_ERROR )
		{
			rs->sr_text = "Dereference control: derefSpec decoding error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		ds = (DerefSpec *)op->o_tmpcalloc( 1,
			sizeof(DerefSpec) + sizeof(AttributeDescription *)*(cnt + 1),
			op->o_tmpmemctx );
		ds->ds_attributes = (AttributeDescription **)&ds[ 1 ];
		ds->ds_nattrs = cnt;

		rc = slap_bv2ad( &derefAttr, &ds->ds_derefAttr, &text );
		if ( rc != LDAP_SUCCESS ) {
			rs->sr_text = "Dereference control: derefAttr decoding error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}

		for ( dstmp = dshead; dstmp && dstmp != ds; dstmp = dstmp->ds_next ) {
			if ( dstmp->ds_derefAttr == ds->ds_derefAttr ) {
				rs->sr_text = "Dereference control: derefAttr must be unique within control";
				rs->sr_err = LDAP_PROTOCOL_ERROR;
				goto done;
			}
		}

		if ( !( ds->ds_derefAttr->ad_type->sat_syntax->ssyn_flags & SLAP_SYNTAX_DN )) {
			if ( ctrl->ldctl_iscritical ) {
				rs->sr_text = "Dereference control: derefAttr syntax not distinguishedName";
				rs->sr_err = LDAP_PROTOCOL_ERROR;
				goto done;
			}

			rs->sr_err = LDAP_SUCCESS;
			goto justcleanup;
		}

		for ( cnt = 0; !BER_BVISNULL( &attributes[ cnt ] ); cnt++ ) {
			rc = slap_bv2ad( &attributes[ cnt ], &ds->ds_attributes[ cnt ], &text );
			if ( rc != LDAP_SUCCESS ) {
				rs->sr_text = "Dereference control: attribute decoding error";
				rs->sr_err = LDAP_PROTOCOL_ERROR;
				goto done;
			}
		}

		ber_memfree_x( attributes, op->o_tmpmemctx );
		attributes = NULL;

		*dsp = ds;
		dsp = &ds->ds_next;
	}

	op->o_ctrlderef = (void *)dshead;

	op->o_deref = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	rs->sr_err = LDAP_SUCCESS;

done:;
	if ( rs->sr_err != LDAP_SUCCESS ) {
justcleanup:;
		for ( ; dshead; ) {
			DerefSpec *dsnext = dshead->ds_next;
			op->o_tmpfree( dshead, op->o_tmpmemctx );
			dshead = dsnext;
		}
	}

	if ( attributes != NULL ) {
		ber_memfree_x( attributes, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

static int
deref_cleanup( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_RESULT || rs->sr_err == SLAPD_ABANDON ) {
		op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
		op->o_callback = NULL;

		op->o_tmpfree( op->o_ctrlderef, op->o_tmpmemctx );
		op->o_ctrlderef = NULL;
	}

	return SLAP_CB_CONTINUE;
}

static int
deref_response( Operation *op, SlapReply *rs )
{
	int rc = SLAP_CB_CONTINUE;

	if ( rs->sr_type == REP_SEARCH ) {
		BerElementBuffer berbuf;
		BerElement *ber = (BerElement *) &berbuf;
		deref_cb_t *dc = (deref_cb_t *)op->o_callback->sc_private;
		DerefSpec *ds;
		DerefRes *dr, *drhead = NULL, **drp = &drhead;
		struct berval bv = BER_BVNULL;
		int nDerefRes = 0, nDerefVals = 0, nAttrs = 0, nVals = 0;
		struct berval ctrlval;
		LDAPControl *ctrl, *ctrlsp[2];
		AccessControlState acl_state = ACL_STATE_INIT;
		static char dummy = '\0';
		Entry *ebase;
		int i;

		rc = overlay_entry_get_ov( op, &rs->sr_entry->e_nname, NULL, NULL, 0, &ebase, dc->dc_on );
		if ( rc != LDAP_SUCCESS || ebase == NULL ) {
			return SLAP_CB_CONTINUE;
		}

		for ( ds = dc->dc_ds; ds; ds = ds->ds_next ) {
			Attribute *a = attr_find( ebase->e_attrs, ds->ds_derefAttr );

			if ( a != NULL ) {
				DerefVal *dv;
				BerVarray *bva;

				if ( !access_allowed( op, rs->sr_entry, a->a_desc,
						NULL, ACL_READ, &acl_state ) )
				{
					continue;
				}

				dr = op->o_tmpcalloc( 1,
					sizeof( DerefRes ) + ( sizeof( DerefVal ) + sizeof( BerVarray * ) * ds->ds_nattrs ) * ( a->a_numvals + 1 ),
					op->o_tmpmemctx );
				dr->dr_spec = *ds;
				dv = dr->dr_vals = (DerefVal *)&dr[ 1 ];
				bva = (BerVarray *)&dv[ a->a_numvals + 1 ];

				bv.bv_len += ds->ds_derefAttr->ad_cname.bv_len;
				nAttrs++;
				nDerefRes++;

				for ( i = 0; !BER_BVISNULL( &a->a_nvals[ i ] ); i++ ) {
					Entry *e = NULL;

					dv[ i ].dv_attrVals = bva;
					bva += ds->ds_nattrs;


					if ( !access_allowed( op, rs->sr_entry, a->a_desc,
							&a->a_nvals[ i ], ACL_READ, &acl_state ) )
					{
						dv[ i ].dv_derefSpecVal.bv_val = &dummy;
						continue;
					}

					ber_dupbv_x( &dv[ i ].dv_derefSpecVal, &a->a_vals[ i ], op->o_tmpmemctx );
					bv.bv_len += dv[ i ].dv_derefSpecVal.bv_len;
					nVals++;
					nDerefVals++;

					rc = overlay_entry_get_ov( op, &a->a_nvals[ i ], NULL, NULL, 0, &e, dc->dc_on );
					if ( rc == LDAP_SUCCESS && e != NULL ) {
						int j;

						if ( access_allowed( op, e, slap_schema.si_ad_entry,
							NULL, ACL_READ, NULL ) )
						{
							for ( j = 0; j < ds->ds_nattrs; j++ ) {
								Attribute *aa;

								if ( !access_allowed( op, e, ds->ds_attributes[ j ], NULL,
									ACL_READ, &acl_state ) )
								{
									continue;
								}

								aa = attr_find( e->e_attrs, ds->ds_attributes[ j ] );
								if ( aa != NULL ) {
									unsigned k, h, last = aa->a_numvals;

									ber_bvarray_dup_x( &dv[ i ].dv_attrVals[ j ],
										aa->a_vals, op->o_tmpmemctx );

									bv.bv_len += ds->ds_attributes[ j ]->ad_cname.bv_len;

									for ( k = 0, h = 0; k < aa->a_numvals; k++ ) {
										if ( !access_allowed( op, e,
											aa->a_desc,
											&aa->a_nvals[ k ],
											ACL_READ, &acl_state ) )
										{
											op->o_tmpfree( dv[ i ].dv_attrVals[ j ][ h ].bv_val,
												op->o_tmpmemctx );
											dv[ i ].dv_attrVals[ j ][ h ] = dv[ i ].dv_attrVals[ j ][ --last ];
											BER_BVZERO( &dv[ i ].dv_attrVals[ j ][ last ] );
											continue;
										}
										bv.bv_len += dv[ i ].dv_attrVals[ j ][ h ].bv_len;
										nVals++;
										h++;
									}
									nAttrs++;
								}
							}
						}

						overlay_entry_release_ov( op, e, 0, dc->dc_on );
					}
				}

				*drp = dr;
				drp = &dr->dr_next;
			}
		}
		overlay_entry_release_ov( op, ebase, 0, dc->dc_on );

		if ( drhead == NULL ) {
			return SLAP_CB_CONTINUE;
		}

		/* cook the control value */
		bv.bv_len += nVals * sizeof(struct berval)
			+ nAttrs * sizeof(struct berval)
			+ nDerefVals * sizeof(DerefVal)
			+ nDerefRes * sizeof(DerefRes);
		bv.bv_val = op->o_tmpalloc( bv.bv_len, op->o_tmpmemctx );

		ber_init2( ber, &bv, LBER_USE_DER );
		ber_set_option( ber, LBER_OPT_BER_MEMCTX, &op->o_tmpmemctx );

		rc = ber_printf( ber, "{" /*}*/ );
		for ( dr = drhead; dr != NULL; dr = dr->dr_next ) {
			for ( i = 0; !BER_BVISNULL( &dr->dr_vals[ i ].dv_derefSpecVal ); i++ ) {
				int j, first = 1;

				if ( dr->dr_vals[ i ].dv_derefSpecVal.bv_val == &dummy ) {
					continue;
				}

				rc = ber_printf( ber, "{OO" /*}*/,
					&dr->dr_spec.ds_derefAttr->ad_cname,
					&dr->dr_vals[ i ].dv_derefSpecVal );
				op->o_tmpfree( dr->dr_vals[ i ].dv_derefSpecVal.bv_val, op->o_tmpmemctx );
				for ( j = 0; j < dr->dr_spec.ds_nattrs; j++ ) {
					if ( dr->dr_vals[ i ].dv_attrVals[ j ] != NULL ) {
						if ( first ) {
							rc = ber_printf( ber, "t{" /*}*/,
								(LBER_CONSTRUCTED|LBER_CLASS_CONTEXT) );
							first = 0;
						}
						rc = ber_printf( ber, "{O[W]}",
							&dr->dr_spec.ds_attributes[ j ]->ad_cname,
							dr->dr_vals[ i ].dv_attrVals[ j ] );
						op->o_tmpfree( dr->dr_vals[ i ].dv_attrVals[ j ],
							op->o_tmpmemctx );
					}
				}
				if ( !first ) {
					rc = ber_printf( ber, /*{{*/ "}N}" );
				} else {
					rc = ber_printf( ber, /*{*/ "}" );
				}
			}
		}
		rc = ber_printf( ber, /*{*/ "}" );
		if ( ber_flatten2( ber, &ctrlval, 0 ) == -1 ) {
			if ( op->o_deref == SLAP_CONTROL_CRITICAL ) {
				rc = LDAP_CONSTRAINT_VIOLATION;

			} else {
				rc = SLAP_CB_CONTINUE;
			}
			goto cleanup;
		}

		ctrl = op->o_tmpcalloc( 1,
			sizeof( LDAPControl ) + ctrlval.bv_len + 1,
			op->o_tmpmemctx );
		ctrl->ldctl_value.bv_val = (char *)&ctrl[ 1 ];
		ctrl->ldctl_oid = LDAP_CONTROL_X_DEREF;
		ctrl->ldctl_iscritical = 0;
		ctrl->ldctl_value.bv_len = ctrlval.bv_len;
		AC_MEMCPY( ctrl->ldctl_value.bv_val, ctrlval.bv_val, ctrlval.bv_len );
		ctrl->ldctl_value.bv_val[ ctrl->ldctl_value.bv_len ] = '\0';

		ber_free_buf( ber );

		ctrlsp[0] = ctrl;
		ctrlsp[1] = NULL;
		slap_add_ctrls( op, rs, ctrlsp );

		rc = SLAP_CB_CONTINUE;

cleanup:;
		/* release all */
		for ( ; drhead != NULL; ) {
			DerefRes *drnext = drhead->dr_next;
			op->o_tmpfree( drhead, op->o_tmpmemctx );
			drhead = drnext;
		}

	} else if ( rs->sr_type == REP_RESULT ) {
		rc = deref_cleanup( op, rs );
	}

	return rc;
}

static int
deref_op_search( Operation *op, SlapReply *rs )
{
	if ( op->o_deref ) {
		slap_callback *sc;
		deref_cb_t *dc;

		sc = op->o_tmpcalloc( 1, sizeof( slap_callback ) + sizeof( deref_cb_t ), op->o_tmpmemctx );

		dc = (deref_cb_t *)&sc[ 1 ];
		dc->dc_on = (slap_overinst *)op->o_bd->bd_info;
		dc->dc_ds = (DerefSpec *)op->o_ctrlderef;

		sc->sc_response = deref_response;
		sc->sc_cleanup = deref_cleanup;
		sc->sc_private = (void *)dc;

		sc->sc_next = op->o_callback->sc_next;
                op->o_callback->sc_next = sc;
	}

	return SLAP_CB_CONTINUE;
}

static int
deref_db_init( BackendDB *be, ConfigReply *cr)
{
	if ( ov_count == 0 ) {
		int rc;

		rc = register_supported_control2( LDAP_CONTROL_X_DEREF,
			SLAP_CTRL_SEARCH,
			NULL,
			deref_parseCtrl,
			1, /* replace */
			&deref_cid );
		if ( rc != LDAP_SUCCESS ) {
			Debug( LDAP_DEBUG_ANY,
				"deref_init: Failed to register control (%d)\n",
				rc, 0, 0 );
			return rc;
		}
	}
	ov_count++;
	return LDAP_SUCCESS;
}

static int
deref_db_open( BackendDB *be, ConfigReply *cr)
{
	return overlay_register_control( be, LDAP_CONTROL_X_DEREF );
}

#ifdef SLAP_CONFIG_DELETE
static int
deref_db_destroy( BackendDB *be, ConfigReply *cr)
{
	ov_count--;
	overlay_unregister_control( be, LDAP_CONTROL_X_DEREF );
	if ( ov_count == 0 ) {
		unregister_supported_control( LDAP_CONTROL_X_DEREF );
	}
	return 0;
}
#endif /* SLAP_CONFIG_DELETE */

int
deref_initialize(void)
{
	deref.on_bi.bi_type = "deref";
	deref.on_bi.bi_db_init = deref_db_init;
	deref.on_bi.bi_db_open = deref_db_open;
#ifdef SLAP_CONFIG_DELETE
	deref.on_bi.bi_db_destroy = deref_db_destroy;
#endif /* SLAP_CONFIG_DELETE */
	deref.on_bi.bi_op_search = deref_op_search;

	return overlay_register( &deref );
}

#if SLAPD_OVER_DEREF == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return deref_initialize();
}
#endif /* SLAPD_OVER_DEREF == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_DEREF */
