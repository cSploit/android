/* valsort.c - sort attribute values */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions copyright 2005 Symas Corporation.
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
 * This work was initially developed by Howard Chu for inclusion in
 * OpenLDAP Software. This work was sponsored by Stanford University.
 */

/*
 * This overlay sorts the values of multi-valued attributes when returning
 * them in a search response.
 */
#include "portable.h"

#ifdef SLAPD_OVER_VALSORT

#include <stdio.h>

#include <ac/string.h>
#include <ac/ctype.h>

#include "slap.h"
#include "config.h"
#include "lutil.h"

#define	VALSORT_ASCEND	0
#define	VALSORT_DESCEND	1

#define	VALSORT_ALPHA	2
#define	VALSORT_NUMERIC	4

#define	VALSORT_WEIGHTED	8

typedef struct valsort_info {
	struct valsort_info *vi_next;
	struct berval vi_dn;
	AttributeDescription *vi_ad;
	slap_mask_t vi_sort;
} valsort_info;

static int valsort_cid;

static ConfigDriver valsort_cf_func;

static ConfigTable valsort_cfats[] = {
	{ "valsort-attr", "attribute> <dn> <sort-type", 4, 5, 0, ARG_MAGIC,
		valsort_cf_func, "( OLcfgOvAt:5.1 NAME 'olcValSortAttr' "
			"DESC 'Sorting rule for attribute under given DN' "
			"EQUALITY caseIgnoreMatch "
			"SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL }
};

static ConfigOCs valsort_cfocs[] = {
	{ "( OLcfgOvOc:5.1 "
		"NAME 'olcValSortConfig' "
		"DESC 'Value Sorting configuration' "
		"SUP olcOverlayConfig "
		"MUST olcValSortAttr )",
			Cft_Overlay, valsort_cfats },
	{ NULL }
};

static slap_verbmasks sorts[] = {
	{ BER_BVC("alpha-ascend"), VALSORT_ASCEND|VALSORT_ALPHA },
	{ BER_BVC("alpha-descend"), VALSORT_DESCEND|VALSORT_ALPHA },
	{ BER_BVC("numeric-ascend"), VALSORT_ASCEND|VALSORT_NUMERIC },
	{ BER_BVC("numeric-descend"), VALSORT_DESCEND|VALSORT_NUMERIC },
	{ BER_BVC("weighted"), VALSORT_WEIGHTED },
	{ BER_BVNULL, 0 }
};

static Syntax *syn_numericString;

static int
valsort_cf_func(ConfigArgs *c) {
	slap_overinst *on = (slap_overinst *)c->bi;
	valsort_info vitmp, *vi;
	const char *text = NULL;
	int i, is_numeric;
	struct berval bv = BER_BVNULL;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		for ( vi = on->on_bi.bi_private; vi; vi = vi->vi_next ) {
			struct berval bv2 = BER_BVNULL, bvret;
			char *ptr;
			int len;
			
			len = vi->vi_ad->ad_cname.bv_len + 1 + vi->vi_dn.bv_len + 2;
			i = vi->vi_sort;
			if ( i & VALSORT_WEIGHTED ) {
				enum_to_verb( sorts, VALSORT_WEIGHTED, &bv2 );
				len += bv2.bv_len + 1;
				i ^= VALSORT_WEIGHTED;
			}
			if ( i ) {
				enum_to_verb( sorts, i, &bv );
				len += bv.bv_len + 1;
			}
			bvret.bv_val = ch_malloc( len+1 );
			bvret.bv_len = len;

			ptr = lutil_strcopy( bvret.bv_val, vi->vi_ad->ad_cname.bv_val );
			*ptr++ = ' ';
			*ptr++ = '"';
			ptr = lutil_strcopy( ptr, vi->vi_dn.bv_val );
			*ptr++ = '"';
			if ( vi->vi_sort & VALSORT_WEIGHTED ) {
				*ptr++ = ' ';
				ptr = lutil_strcopy( ptr, bv2.bv_val );
			}
			if ( i ) {
				*ptr++ = ' ';
				strcpy( ptr, bv.bv_val );
			}
			ber_bvarray_add( &c->rvalue_vals, &bvret );
		}
		i = ( c->rvalue_vals != NULL ) ? 0 : 1;
		return i;
	} else if ( c->op == LDAP_MOD_DELETE ) {
		if ( c->valx < 0 ) {
			for ( vi = on->on_bi.bi_private; vi; vi = on->on_bi.bi_private ) {
				on->on_bi.bi_private = vi->vi_next;
				ch_free( vi->vi_dn.bv_val );
				ch_free( vi );
			}
		} else {
			valsort_info **prev;

			for (i=0, prev = (valsort_info **)&on->on_bi.bi_private,
				vi = *prev; vi && i<c->valx;
				prev = &vi->vi_next, vi = vi->vi_next, i++ );
			(*prev)->vi_next = vi->vi_next;
			ch_free( vi->vi_dn.bv_val );
			ch_free( vi );
		}
		return 0;
	}
	vitmp.vi_ad = NULL;
	i = slap_str2ad( c->argv[1], &vitmp.vi_ad, &text );
	if ( i ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg), "<%s> %s", c->argv[0], text );
		Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
			c->log, c->cr_msg, c->argv[1] );
		return(1);
	}
	if ( is_at_single_value( vitmp.vi_ad->ad_type )) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> %s is single-valued, ignoring", c->argv[0],
			vitmp.vi_ad->ad_cname.bv_val );
		Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
			c->log, c->cr_msg, c->argv[1] );
		return(0);
	}
	is_numeric = ( vitmp.vi_ad->ad_type->sat_syntax == syn_numericString ||
		vitmp.vi_ad->ad_type->sat_syntax == slap_schema.si_syn_integer ) ? 1
		: 0;
	ber_str2bv( c->argv[2], 0, 0, &bv );
	i = dnNormalize( 0, NULL, NULL, &bv, &vitmp.vi_dn, NULL );
	if ( i ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unable to normalize DN", c->argv[0] );
		Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
			c->log, c->cr_msg, c->argv[2] );
		return(1);
	}
	i = verb_to_mask( c->argv[3], sorts );
	if ( BER_BVISNULL( &sorts[i].word )) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unrecognized sort type", c->argv[0] );
		Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
			c->log, c->cr_msg, c->argv[3] );
		return(1);
	}
	vitmp.vi_sort = sorts[i].mask;
	if ( sorts[i].mask == VALSORT_WEIGHTED && c->argc == 5 ) {
		i = verb_to_mask( c->argv[4], sorts );
		if ( BER_BVISNULL( &sorts[i].word )) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> unrecognized sort type", c->argv[0] );
			Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
				c->log, c->cr_msg, c->argv[4] );
			return(1);
		}
		vitmp.vi_sort |= sorts[i].mask;
	}
	if (( vitmp.vi_sort & VALSORT_NUMERIC ) && !is_numeric ) {
		snprintf( c->cr_msg, sizeof( c->cr_msg ), "<%s> numeric sort specified for non-numeric syntax",
			c->argv[0] );
		Debug( LDAP_DEBUG_ANY, "%s: %s (%s)!\n",
			c->log, c->cr_msg, c->argv[1] );
		return(1);
	}
	vi = ch_malloc( sizeof(valsort_info) );
	*vi = vitmp;
	vi->vi_next = on->on_bi.bi_private;
	on->on_bi.bi_private = vi;
	return 0;
}

/* Use Insertion Sort algorithm on selected values */
static void
do_sort( Operation *op, Attribute *a, int beg, int num, slap_mask_t sort )
{
	int i, j, gotnvals;
	struct berval tmp, ntmp, *vals = NULL, *nvals;

	gotnvals = (a->a_vals != a->a_nvals );

	nvals = a->a_nvals + beg;
	if ( gotnvals )
		vals = a->a_vals + beg;

	if ( sort & VALSORT_NUMERIC ) {
		long *numbers = op->o_tmpalloc( num * sizeof(long), op->o_tmpmemctx ),
			idx;
		for (i=0; i<num; i++)
			numbers[i] = strtol( nvals[i].bv_val, NULL, 0 );

		for (i=1; i<num; i++) {
			idx = numbers[i];
			ntmp = nvals[i];
			if ( gotnvals ) tmp = vals[i];
			j = i;
			while ( j>0 ) {
				int cmp = (sort & VALSORT_DESCEND) ? numbers[j-1] < idx :
					numbers[j-1] > idx;
				if ( !cmp ) break;
				numbers[j] = numbers[j-1];
				nvals[j] = nvals[j-1];
				if ( gotnvals ) vals[j] = vals[j-1];
				j--;
			}
			numbers[j] = idx;
			nvals[j] = ntmp;
			if ( gotnvals ) vals[j] = tmp;
		}
		op->o_tmpfree( numbers, op->o_tmpmemctx );
	} else {
		for (i=1; i<num; i++) {
			ntmp = nvals[i];
			if ( gotnvals ) tmp = vals[i];
			j = i;
			while ( j>0 ) {
				int cmp = strcmp( nvals[j-1].bv_val, ntmp.bv_val );
				cmp = (sort & VALSORT_DESCEND) ? (cmp < 0) : (cmp > 0);
				if ( !cmp ) break;

				nvals[j] = nvals[j-1];
				if ( gotnvals ) vals[j] = vals[j-1];
				j--;
			}
			nvals[j] = ntmp;
			if ( gotnvals ) vals[j] = tmp;
		}
	}
}

static int
valsort_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on;
	valsort_info *vi;
	Attribute *a;

	/* If this is not a search response, or it is a syncrepl response,
	 * or the valsort control wants raw results, pass thru unmodified.
	 */
	if ( rs->sr_type != REP_SEARCH ||
		( _SCM(op->o_sync) > SLAP_CONTROL_IGNORED ) ||
		( op->o_ctrlflag[valsort_cid] & SLAP_CONTROL_DATA0))
		return SLAP_CB_CONTINUE;
		
	on = (slap_overinst *) op->o_bd->bd_info;
	vi = on->on_bi.bi_private;

	/* And we must have something configured */
	if ( !vi ) return SLAP_CB_CONTINUE;

	/* Find a rule whose baseDN matches this entry */
	for (; vi; vi = vi->vi_next ) {
		int i, n;

		if ( !dnIsSuffix( &rs->sr_entry->e_nname, &vi->vi_dn ))
			continue;

		/* Find attr that this rule affects */
		a = attr_find( rs->sr_entry->e_attrs, vi->vi_ad );
		if ( !a ) continue;

		if ( rs_entry2modifiable( op, rs, on )) {
			a = attr_find( rs->sr_entry->e_attrs, vi->vi_ad );
		}

		n = a->a_numvals;
		if ( vi->vi_sort & VALSORT_WEIGHTED ) {
			int j, gotnvals;
			long *index = op->o_tmpalloc( n * sizeof(long), op->o_tmpmemctx );

			gotnvals = (a->a_vals != a->a_nvals );

			for (i=0; i<n; i++) {
				char *ptr = ber_bvchr( &a->a_nvals[i], '{' );
				char *end = NULL;
				if ( !ptr ) {
					Debug(LDAP_DEBUG_TRACE, "weights missing from attr %s "
						"in entry %s\n", vi->vi_ad->ad_cname.bv_val,
						rs->sr_entry->e_name.bv_val, 0 );
					break;
				}
				index[i] = strtol( ptr+1, &end, 0 );
				if ( *end != '}' ) {
					Debug(LDAP_DEBUG_TRACE, "weights misformatted "
						"in entry %s\n", 
						rs->sr_entry->e_name.bv_val, 0, 0 );
					break;
				}
				/* Strip out weights */
				ptr = a->a_nvals[i].bv_val;
				end++;
				for (;*end;)
					*ptr++ = *end++;
				*ptr = '\0';
				a->a_nvals[i].bv_len = ptr - a->a_nvals[i].bv_val;

				if ( a->a_vals != a->a_nvals ) {
					ptr = a->a_vals[i].bv_val;
					end = ber_bvchr( &a->a_vals[i], '}' );
					assert( end != NULL );
					end++;
					for (;*end;)
						*ptr++ = *end++;
					*ptr = '\0';
					a->a_vals[i].bv_len = ptr - a->a_vals[i].bv_val;
				}
			}
			/* An attr was missing weights here, ignore it */
			if ( i<n ) {
				op->o_tmpfree( index, op->o_tmpmemctx );
				continue;
			}
			/* Insertion sort */
			for ( i=1; i<n; i++) {
				long idx = index[i];
				struct berval tmp = a->a_vals[i], ntmp;
				if ( gotnvals ) ntmp = a->a_nvals[i];
				j = i;
				while (( j>0 ) && (index[j-1] > idx )) {
					index[j] = index[j-1];
					a->a_vals[j] = a->a_vals[j-1];
					if ( gotnvals ) a->a_nvals[j] = a->a_nvals[j-1];
					j--;
				}
				index[j] = idx;
				a->a_vals[j] = tmp;
				if ( gotnvals ) a->a_nvals[j] = ntmp;
			}
			/* Check for secondary sort */
			if ( vi->vi_sort ^ VALSORT_WEIGHTED ) {
				for ( i=0; i<n;) {
					for (j=i+1; j<n; j++) {
						if (index[i] != index[j])
							break;
					}
					if( j-i > 1 )
						do_sort( op, a, i, j-i, vi->vi_sort );
					i = j;
				}
			}
			op->o_tmpfree( index, op->o_tmpmemctx );
		} else {
			do_sort( op, a, 0, n, vi->vi_sort );
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
valsort_add( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	valsort_info *vi = on->on_bi.bi_private;

	Attribute *a;
	int i;
	char *ptr, *end;

	/* See if any weighted sorting applies to this entry */
	for ( ;vi;vi=vi->vi_next ) {
		if ( !dnIsSuffix( &op->o_req_ndn, &vi->vi_dn ))
			continue;
		if ( !(vi->vi_sort & VALSORT_WEIGHTED ))
			continue;
		a = attr_find( op->ora_e->e_attrs, vi->vi_ad );
		if ( !a )
			continue;
		for (i=0; !BER_BVISNULL( &a->a_vals[i] ); i++) {
			ptr = ber_bvchr(&a->a_vals[i], '{' );
			if ( !ptr ) {
				Debug(LDAP_DEBUG_TRACE, "weight missing from attribute %s\n",
					vi->vi_ad->ad_cname.bv_val, 0, 0);
				send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION,
					"weight missing from attribute" );
				return rs->sr_err;
			}
			strtol( ptr+1, &end, 0 );
			if ( *end != '}' ) {
				Debug(LDAP_DEBUG_TRACE, "weight is misformatted in %s\n",
					vi->vi_ad->ad_cname.bv_val, 0, 0);
				send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION,
					"weight is misformatted" );
				return rs->sr_err;
			}
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
valsort_modify( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *)op->o_bd->bd_info;
	valsort_info *vi = on->on_bi.bi_private;

	Modifications *ml;
	int i;
	char *ptr, *end;

	/* See if any weighted sorting applies to this entry */
	for ( ;vi;vi=vi->vi_next ) {
		if ( !dnIsSuffix( &op->o_req_ndn, &vi->vi_dn ))
			continue;
		if ( !(vi->vi_sort & VALSORT_WEIGHTED ))
			continue;
		for (ml = op->orm_modlist; ml; ml=ml->sml_next ) {
			/* Must be a Delete Attr op, so no values to consider */
			if ( !ml->sml_values )
				continue;
			if ( ml->sml_desc == vi->vi_ad )
				break;
		}
		if ( !ml )
			continue;
		for (i=0; !BER_BVISNULL( &ml->sml_values[i] ); i++) {
			ptr = ber_bvchr(&ml->sml_values[i], '{' );
			if ( !ptr ) {
				Debug(LDAP_DEBUG_TRACE, "weight missing from attribute %s\n",
					vi->vi_ad->ad_cname.bv_val, 0, 0);
				send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION,
					"weight missing from attribute" );
				return rs->sr_err;
			}
			strtol( ptr+1, &end, 0 );
			if ( *end != '}' ) {
				Debug(LDAP_DEBUG_TRACE, "weight is misformatted in %s\n",
					vi->vi_ad->ad_cname.bv_val, 0, 0);
				send_ldap_error( op, rs, LDAP_CONSTRAINT_VIOLATION,
					"weight is misformatted" );
				return rs->sr_err;
			}
		}
	}
	return SLAP_CB_CONTINUE;
}

static int
valsort_db_open(
	BackendDB *be,
	ConfigReply *cr
)
{
	return overlay_register_control( be, LDAP_CONTROL_VALSORT );
}

static int
valsort_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	valsort_info *vi = on->on_bi.bi_private, *next;

#ifdef SLAP_CONFIG_DELETE
	overlay_unregister_control( be, LDAP_CONTROL_VALSORT );
#endif /* SLAP_CONFIG_DELETE */

	for (; vi; vi = next) {
		next = vi->vi_next;
		ch_free( vi->vi_dn.bv_val );
		ch_free( vi );
	}

	return 0;
}

static int
valsort_parseCtrl(
	Operation *op,
	SlapReply *rs,
	LDAPControl *ctrl )
{
	ber_tag_t tag;
	BerElementBuffer berbuf;
	BerElement *ber = (BerElement *)&berbuf;
	ber_int_t flag = 0;

	if ( BER_BVISNULL( &ctrl->ldctl_value )) {
		rs->sr_text = "valSort control value is absent";
		return LDAP_PROTOCOL_ERROR;
	}

	if ( BER_BVISEMPTY( &ctrl->ldctl_value )) {
		rs->sr_text = "valSort control value is empty";
		return LDAP_PROTOCOL_ERROR;
	}

	ber_init2( ber, &ctrl->ldctl_value, 0 );
	if (( tag = ber_scanf( ber, "{b}", &flag )) == LBER_ERROR ) {
		rs->sr_text = "valSort control: flag decoding error";
		return LDAP_PROTOCOL_ERROR;
	}

	op->o_ctrlflag[valsort_cid] = ctrl->ldctl_iscritical ?
		SLAP_CONTROL_CRITICAL : SLAP_CONTROL_NONCRITICAL;
	if ( flag )
		op->o_ctrlflag[valsort_cid] |= SLAP_CONTROL_DATA0;

	return LDAP_SUCCESS;
}

static slap_overinst valsort;

int valsort_initialize( void )
{
	int rc;

	valsort.on_bi.bi_type = "valsort";
	valsort.on_bi.bi_db_destroy = valsort_destroy;
	valsort.on_bi.bi_db_open = valsort_db_open;

	valsort.on_bi.bi_op_add = valsort_add;
	valsort.on_bi.bi_op_modify = valsort_modify;

	valsort.on_response = valsort_response;

	valsort.on_bi.bi_cf_ocs = valsort_cfocs;

	rc = register_supported_control( LDAP_CONTROL_VALSORT,
		SLAP_CTRL_SEARCH | SLAP_CTRL_HIDE, NULL, valsort_parseCtrl,
		&valsort_cid );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_ANY, "Failed to register control %d\n", rc, 0, 0 );
		return rc;
	}

	syn_numericString = syn_find( "1.3.6.1.4.1.1466.115.121.1.36" );

	rc = config_register_schema( valsort_cfats, valsort_cfocs );
	if ( rc ) return rc;

	return overlay_register(&valsort);
}

#if SLAPD_OVER_VALSORT == SLAPD_MOD_DYNAMIC
int init_module( int argc, char *argv[]) {
	return valsort_initialize();
}
#endif

#endif /* SLAPD_OVER_VALSORT */
