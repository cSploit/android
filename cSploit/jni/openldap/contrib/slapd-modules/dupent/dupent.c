/* dupent.c - LDAP Control for a Duplicate Entry Representation of Search Results */
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
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

/*
 * LDAP Control for a Duplicate Entry Representation of Search Results
 * <draft-ietf-ldapext-ldapv3-dupent-08.txt> (EXPIRED)
 * <http://tools.ietf.org/id/draft-ietf-ldapext-ldapv3-dupent-08.txt>
 */

#include "portable.h"

/* define SLAPD_OVER_DUPENT=2 to build as run-time loadable module */
#ifdef SLAPD_OVER_DUPENT

/*
 * The macros
 *
 * LDAP_CONTROL_DUPENT_REQUEST		"2.16.840.1.113719.1.27.101.1"
 * LDAP_CONTROL_DUPENT_RESPONSE		"2.16.840.1.113719.1.27.101.2"
 * LDAP_CONTROL_DUPENT_ENTRY		"2.16.840.1.113719.1.27.101.3"
 *
 * are already defined in <ldap.h>
 */

/*
 * support for no attrs and "*" in AttributeDescriptionList is missing
 */

#include "slap.h"
#include "ac/string.h"

#define o_dupent			o_ctrlflag[dupent_cid]
#define o_ctrldupent		o_controls[dupent_cid]

static int dupent_cid;
static slap_overinst dupent;

typedef struct dupent_t {
	AttributeName	*ds_an;
	ber_len_t		ds_nattrs;
	slap_mask_t		ds_flags;
	ber_int_t		ds_paa;
} dupent_t;

static int
dupent_parseCtrl (
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	ber_tag_t tag;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_len_t len;
	BerVarray AttributeDescriptionList = NULL;
	ber_len_t cnt = sizeof(struct berval);
	ber_len_t off = 0;
	ber_int_t PartialApplicationAllowed = 1;
	dupent_t *ds = NULL;
	int i;

	if ( op->o_dupent != SLAP_CONTROL_NONE ) {
		rs->sr_text = "Dupent control specified multiple times";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISNULL( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Dupent control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value ) ) {
		rs->sr_text = "Dupent control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_init2( ber, &ctrl->ldctl_value, 0 );

	/*

   DuplicateEntryRequest ::= SEQUENCE { 
        AttributeDescriptionList, -- from [RFC2251] 
        PartialApplicationAllowed BOOLEAN DEFAULT TRUE } 

        AttributeDescriptionList ::= SEQUENCE OF 
                AttributeDescription 
    
        AttributeDescription ::= LDAPString 
    
        attributeDescription = AttributeType [ ";" <options> ] 

	 */

	tag = ber_skip_tag( ber, &len );
	if ( tag != LBER_SEQUENCE ) return LDAP_INVALID_SYNTAX;
	if ( ber_scanf( ber, "{M}", &AttributeDescriptionList, &cnt, off )
		== LBER_ERROR )
	{
		rs->sr_text = "Dupent control: dupentSpec decoding error";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}
	tag = ber_skip_tag( ber, &len );
	if ( tag == LBER_BOOLEAN ) {
		/* NOTE: PartialApplicationAllowed is ignored, since the control
		 * can always be honored
		 */
		if ( ber_scanf( ber, "b", &PartialApplicationAllowed ) == LBER_ERROR )
		{
			rs->sr_text = "Dupent control: dupentSpec decoding error";
			rs->sr_err = LDAP_PROTOCOL_ERROR;
			goto done;
		}
		tag = ber_skip_tag( ber, &len );
	}
	if ( len || tag != LBER_DEFAULT ) {
		rs->sr_text = "Dupent control: dupentSpec decoding error";
		rs->sr_err = LDAP_PROTOCOL_ERROR;
		goto done;
	}

	ds = (dupent_t *)op->o_tmpcalloc( 1,
		sizeof(dupent_t) + sizeof(AttributeName)*cnt,
		op->o_tmpmemctx );

	ds->ds_paa = PartialApplicationAllowed;

	if ( cnt == 0 ) {
		ds->ds_flags |= SLAP_USERATTRS_YES;

	} else {
		int c;

		ds->ds_an = (AttributeName *)&ds[ 1 ];

		for ( i = 0, c = 0; i < cnt; i++ ) {
			const char *text;
			int j;
			int rc;
			AttributeDescription *ad = NULL;

			if ( bvmatch( &AttributeDescriptionList[i],
				slap_bv_all_user_attrs ) )
			{
				if ( ds->ds_flags & SLAP_USERATTRS_YES ) {
					rs->sr_text = "Dupent control: AttributeDescription decoding error";
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					goto done;
				}

				ds->ds_flags |= SLAP_USERATTRS_YES;
				continue;
			}

			rc = slap_bv2ad( &AttributeDescriptionList[i], &ad, &text );
			if ( rc != LDAP_SUCCESS ) {
				continue;
			}

			ds->ds_an[c].an_desc = ad;
			ds->ds_an[c].an_name = ad->ad_cname;

			/* FIXME: not specified; consider this an error, just in case */
			for ( j = 0; j < c; j++ ) {
				if ( ds->ds_an[c].an_desc == ds->ds_an[j].an_desc ) {
					rs->sr_text = "Dupent control: AttributeDescription must be unique within AttributeDescriptionList";
					rs->sr_err = LDAP_PROTOCOL_ERROR;
					goto done;
				}
			}

			c++;
		}

		ds->ds_nattrs = c;

		if ( ds->ds_flags & SLAP_USERATTRS_YES ) {
			/* purge user attrs */
			for ( i = 0; i < ds->ds_nattrs;  ) {
				if ( is_at_operational( ds->ds_an[i].an_desc->ad_type ) ) {
					i++;
					continue;
				}

				ds->ds_nattrs--;
				if ( i < ds->ds_nattrs ) {
					ds->ds_an[i] = ds->ds_an[ds->ds_nattrs];
				}
			}
		}
	}

	op->o_ctrldupent = (void *)ds;

	op->o_dupent = ctrl->ldctl_iscritical
		? SLAP_CONTROL_CRITICAL
		: SLAP_CONTROL_NONCRITICAL;

	rs->sr_err = LDAP_SUCCESS;

done:;
	if ( rs->sr_err != LDAP_SUCCESS ) {
		op->o_tmpfree( ds, op->o_tmpmemctx );
	}

	if ( AttributeDescriptionList != NULL ) {
		ber_memfree_x( AttributeDescriptionList, op->o_tmpmemctx );
	}

	return rs->sr_err;
}

typedef struct dupent_cb_t {
	slap_overinst	*dc_on;
	dupent_t		*dc_ds;
	int		dc_skip;
} dupent_cb_t;

typedef struct valnum_t {
	Attribute *ap;
	Attribute a;
	struct berval vals[2];
	struct berval nvals[2];
	int cnt;
} valnum_t;

static int
dupent_response_done( Operation *op, SlapReply *rs )
{
	BerElementBuffer	berbuf;
	BerElement			*ber = (BerElement *) &berbuf;
	struct berval		ctrlval;
	LDAPControl			*ctrl, *ctrlsp[2];

	ber_init2( ber, NULL, LBER_USE_DER );

	/*

      DuplicateEntryResponseDone ::= SEQUENCE { 
         resultCode,     -- From [RFC2251] 
         errorMessage    [0] LDAPString OPTIONAL, 
         attribute       [1] AttributeDescription OPTIONAL } 

	 */

	ber_printf( ber, "{i}", rs->sr_err );
	if ( ber_flatten2( ber, &ctrlval, 0 ) == -1 ) {
		ber_free_buf( ber );
		if ( op->o_dupent == SLAP_CONTROL_CRITICAL ) {
			return LDAP_CONSTRAINT_VIOLATION;
		}
		return SLAP_CB_CONTINUE;
	}

	ctrl = op->o_tmpcalloc( 1,
		sizeof( LDAPControl ) + ctrlval.bv_len + 1,
		op->o_tmpmemctx );
	ctrl->ldctl_value.bv_val = (char *)&ctrl[ 1 ];
	ctrl->ldctl_oid = LDAP_CONTROL_DUPENT_RESPONSE;
	ctrl->ldctl_iscritical = 0;
	ctrl->ldctl_value.bv_len = ctrlval.bv_len;
	AC_MEMCPY( ctrl->ldctl_value.bv_val, ctrlval.bv_val, ctrlval.bv_len );
	ctrl->ldctl_value.bv_val[ ctrl->ldctl_value.bv_len ] = '\0';

	ber_free_buf( ber );

	ctrlsp[0] = ctrl;
	ctrlsp[1] = NULL;
	slap_add_ctrls( op, rs, ctrlsp );

	return SLAP_CB_CONTINUE;
}

static int
dupent_response_entry_1level(
	Operation *op,
	SlapReply *rs,
	Entry *e,
	valnum_t *valnum,
	int nattrs,
	int level )
{
	int i, rc = LDAP_SUCCESS;

	for ( i = 0; i < valnum[level].ap->a_numvals; i++ ) {
		LDAPControl	*ctrl = NULL, *ctrlsp[2];

		valnum[level].a.a_vals[0] = valnum[level].ap->a_vals[i];
		if ( valnum[level].ap->a_nvals != valnum[level].ap->a_vals ) {
			valnum[level].a.a_nvals[0] = valnum[level].ap->a_nvals[i];
		}

		if ( level < nattrs - 1 ) {
			rc = dupent_response_entry_1level( op, rs,
				e, valnum, nattrs, level + 1 );
			if ( rc != LDAP_SUCCESS ) {
				break;
			}

			continue;
		}

		/* NOTE: add the control all times, under the assumption
		 * send_search_entry() honors the REP_CTRLS_MUSTBEFREED
		 * set by slap_add_ctrls(); this is not true (ITS#6629)
		 */
		ctrl = op->o_tmpcalloc( 1, sizeof( LDAPControl ), op->o_tmpmemctx );
		ctrl->ldctl_oid = LDAP_CONTROL_DUPENT_ENTRY;
		ctrl->ldctl_iscritical = 0;

		ctrlsp[0] = ctrl;
		ctrlsp[1] = NULL;
		slap_add_ctrls( op, rs, ctrlsp );

		/* do the real send */
		rs->sr_entry = e;
		rc = send_search_entry( op, rs );
		if ( rc != LDAP_SUCCESS ) {
			break;
		}
	}

	return rc;
}

static int
dupent_attr_prepare( dupent_t *ds, Entry *e, valnum_t *valnum, int nattrs, int c, Attribute **app, Attribute **ap_listp )
{
	valnum[c].ap = *app;
	*app = (*app)->a_next;

	valnum[c].ap->a_next = *ap_listp;
	*ap_listp = valnum[c].ap;

	valnum[c].a = *valnum[c].ap;
	if ( c < nattrs - 1 ) {
		valnum[c].a.a_next = &valnum[c + 1].a;
	} else {
		valnum[c].a.a_next = NULL;
	}
	valnum[c].a.a_numvals = 1;
	valnum[c].a.a_vals = valnum[c].vals;
	BER_BVZERO( &valnum[c].vals[1] );
	if ( valnum[c].ap->a_nvals != valnum[c].ap->a_vals ) {
		valnum[c].a.a_nvals = valnum[c].nvals;
		BER_BVZERO( &valnum[c].nvals[1] );
	} else {
		valnum[c].a.a_nvals = valnum[c].a.a_vals;
	}
}

static int
dupent_response_entry( Operation *op, SlapReply *rs )
{
	dupent_cb_t	*dc = (dupent_cb_t *)op->o_callback->sc_private;
	int			nattrs = 0;
	valnum_t	*valnum = NULL;
	Attribute	**app, *ap_list = NULL;
	int			i, c;
	Entry		*e = NULL;
	int			rc;

	assert( rs->sr_type == REP_SEARCH );

	for ( i = 0; i < dc->dc_ds->ds_nattrs; i++ ) {
		Attribute *ap;

		ap = attr_find( rs->sr_entry->e_attrs,
			dc->dc_ds->ds_an[ i ].an_desc );
		if ( ap && ap->a_numvals > 1 ) {
			nattrs++;
		}
	}

	if ( dc->dc_ds->ds_flags & SLAP_USERATTRS_YES ) {
		Attribute *ap;

		for ( ap = rs->sr_entry->e_attrs; ap != NULL; ap = ap->a_next ) {
			if ( !is_at_operational( ap->a_desc->ad_type ) && ap->a_numvals > 1 ) {
				nattrs++;
			}
		}
	}

	if ( !nattrs ) {
		return SLAP_CB_CONTINUE;
	}

	rs_entry2modifiable( op, rs, dc->dc_on );
	rs->sr_flags &= ~(REP_ENTRY_MODIFIABLE | REP_ENTRY_MUSTBEFREED);
	e = rs->sr_entry;

	valnum = op->o_tmpcalloc( sizeof(valnum_t), nattrs, op->o_tmpmemctx );

	for ( c = 0, i = 0; i < dc->dc_ds->ds_nattrs; i++ ) {
		for ( app = &e->e_attrs; *app != NULL; app = &(*app)->a_next ) {
			if ( (*app)->a_desc == dc->dc_ds->ds_an[ i ].an_desc ) {
				break;
			}
		}

		if ( *app != NULL && (*app)->a_numvals > 1 ) {
			assert( c < nattrs );
			dupent_attr_prepare( dc->dc_ds, e, valnum, nattrs, c, app, &ap_list );
			c++;
		}
	}

	if ( dc->dc_ds->ds_flags & SLAP_USERATTRS_YES ) {
		for ( app = &e->e_attrs; *app != NULL; app = &(*app)->a_next ) {
			if ( !is_at_operational( (*app)->a_desc->ad_type ) && (*app)->a_numvals > 1 ) {
				assert( c < nattrs );
				dupent_attr_prepare( dc->dc_ds, e, valnum, nattrs, c, app, &ap_list );
				c++;
			}
		}
	}

	for ( app = &e->e_attrs; *app != NULL; app = &(*app)->a_next )
		/* goto tail */ ;

	*app = &valnum[0].a;

	/* NOTE: since send_search_entry() does not honor the
	 * REP_CTRLS_MUSTBEFREED flag set by slap_add_ctrls(),
	 * the control could be added here once for all (ITS#6629)
	 */

	dc->dc_skip = 1;
	rc = dupent_response_entry_1level( op, rs, e, valnum, nattrs, 0 );
	dc->dc_skip = 0;

	*app = ap_list;

	entry_free( e );

	op->o_tmpfree( valnum, op->o_tmpmemctx );

	return rc;
}

static int
dupent_response( Operation *op, SlapReply *rs )
{
	dupent_cb_t	*dc = (dupent_cb_t *)op->o_callback->sc_private;

	if ( dc->dc_skip ) {
		return SLAP_CB_CONTINUE;
	}
	
	switch ( rs->sr_type ) {
	case REP_RESULT:
		return dupent_response_done( op, rs );

	case REP_SEARCH:
		return dupent_response_entry( op, rs );

	case REP_SEARCHREF:
		break;

	default:
		assert( 0 );
	}

	return SLAP_CB_CONTINUE;
}

static int
dupent_cleanup( Operation *op, SlapReply *rs )
{
	if ( rs->sr_type == REP_RESULT || rs->sr_err == SLAPD_ABANDON ) {
		op->o_tmpfree( op->o_callback, op->o_tmpmemctx );
		op->o_callback = NULL;

		op->o_tmpfree( op->o_ctrldupent, op->o_tmpmemctx );
		op->o_ctrldupent = NULL;
	}

	return SLAP_CB_CONTINUE;
}

static int
dupent_op_search( Operation *op, SlapReply *rs )
{
	if ( op->o_dupent != SLAP_CONTROL_NONE ) {
		slap_callback *sc;
		dupent_cb_t *dc;

		sc = op->o_tmpcalloc( 1, sizeof( slap_callback ) + sizeof( dupent_cb_t ), op->o_tmpmemctx );

		dc = (dupent_cb_t *)&sc[ 1 ];
		dc->dc_on = (slap_overinst *)op->o_bd->bd_info;
		dc->dc_ds = (dupent_t *)op->o_ctrldupent;
		dc->dc_skip = 0;

		sc->sc_response = dupent_response;
		sc->sc_cleanup = dupent_cleanup;
		sc->sc_private = (void *)dc;

		sc->sc_next = op->o_callback->sc_next;
                op->o_callback->sc_next = sc;
	}
	
	return SLAP_CB_CONTINUE;
}

#if SLAPD_OVER_DUPENT == SLAPD_MOD_DYNAMIC
static
#endif /* SLAPD_OVER_DUPENT == SLAPD_MOD_DYNAMIC */
int
dupent_initialize( void )
{
	int rc;

	rc = register_supported_control( LDAP_CONTROL_DUPENT,
		SLAP_CTRL_SEARCH, NULL,
		dupent_parseCtrl, &dupent_cid );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY,
			"dupent_initialize: Failed to register control (%d)\n",
			rc, 0, 0 );
		return -1;
	}

	dupent.on_bi.bi_type = "dupent";

	dupent.on_bi.bi_op_search = dupent_op_search;

	return overlay_register( &dupent );
}

#if SLAPD_OVER_DUPENT == SLAPD_MOD_DYNAMIC
int
init_module( int argc, char *argv[] )
{
	return dupent_initialize();
}
#endif /* SLAPD_OVER_DUPENT == SLAPD_MOD_DYNAMIC */

#endif /* SLAPD_OVER_DUPENT */
