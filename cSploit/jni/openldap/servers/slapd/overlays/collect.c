/* collect.c - Demonstration of overlay code */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2003-2014 The OpenLDAP Foundation.
 * Portions Copyright 2003 Howard Chu.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#ifdef SLAPD_OVER_COLLECT

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "config.h"

#include "lutil.h"

/* This is a cheap hack to implement a collective attribute.
 *
 * This demonstration overlay looks for a specified attribute in an
 * ancestor of a given entry and adds that attribute to the given
 * entry when it is returned in a search response. It takes no effect
 * for any other operations. If the ancestor does not exist, there
 * is no effect. If no attribute was configured, there is no effect.
 */

typedef struct collect_info {
	struct collect_info *ci_next;
	struct berval ci_dn;
	int ci_ad_num;
	AttributeDescription *ci_ad[1];
} collect_info;

static int collect_cf( ConfigArgs *c );

static ConfigTable collectcfg[] = {
	{ "collectinfo", "dn> <attribute", 3, 3, 0,
	  ARG_MAGIC, collect_cf,
	  "( OLcfgOvAt:19.1 NAME 'olcCollectInfo' "
	  "DESC 'DN of entry and attribute to distribute' "
	  "EQUALITY caseIgnoreMatch "
	  "SYNTAX OMsDirectoryString )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED }
};

static ConfigOCs collectocs[] = {
	{ "( OLcfgOvOc:19.1 "
	  "NAME 'olcCollectConfig' "
	  "DESC 'Collective Attribute configuration' "
	  "SUP olcOverlayConfig "
	  "MAY olcCollectInfo )",
	  Cft_Overlay, collectcfg },
	{ NULL, 0, NULL }
};

/*
 * inserts a collect_info into on->on_bi.bi_private taking into account
 * order. this means longer dn's (i.e. more specific dn's) will be found
 * first when searching, allowing some limited overlap of dn's
 */
static void
insert_ordered( slap_overinst *on, collect_info *ci ) {
	collect_info *find = on->on_bi.bi_private;
	collect_info *prev = NULL;
	int found = 0;

	while (!found) {
		if (find == NULL) {
			if (prev == NULL) {
				/* base case - empty list */
				on->on_bi.bi_private = ci;
				ci->ci_next = NULL;
			} else {
				/* final case - end of list */
				prev->ci_next = ci;
				ci->ci_next = NULL;
			}
			found = 1;
		} else if (find->ci_dn.bv_len < ci->ci_dn.bv_len) { 
			/* insert into list here */
			if (prev == NULL) {
				/* entry is head of list */
				ci->ci_next = on->on_bi.bi_private;
				on->on_bi.bi_private = ci;
			} else {
				/* entry is not head of list */
				prev->ci_next = ci;
				ci->ci_next = find;
			}
			found = 1;
		} else {
			/* keep looking */
			prev = find;
			find = find->ci_next;
		}
	}
}

static int
collect_cf( ConfigArgs *c )
{
	slap_overinst *on = (slap_overinst *)c->bi;
	int rc = 1, idx;

	switch( c->op ) {
	case SLAP_CONFIG_EMIT:
		{
		collect_info *ci;
		for ( ci = on->on_bi.bi_private; ci; ci = ci->ci_next ) {
			struct berval bv;
			char *ptr;
			int len;

			/* calculate the length & malloc memory */
			bv.bv_len = ci->ci_dn.bv_len + STRLENOF("\"\" ");
			for (idx=0; idx<ci->ci_ad_num; idx++) {
				bv.bv_len += ci->ci_ad[idx]->ad_cname.bv_len;
				if (idx<(ci->ci_ad_num-1)) { 
					bv.bv_len++;
				}
			}
			bv.bv_val = ch_malloc( bv.bv_len + 1 );

			/* copy the value and update len */
			len = snprintf( bv.bv_val, bv.bv_len + 1, "\"%s\" ", 
				ci->ci_dn.bv_val);
			ptr = bv.bv_val + len;
			for (idx=0; idx<ci->ci_ad_num; idx++) {
				ptr = lutil_strncopy( ptr,
					ci->ci_ad[idx]->ad_cname.bv_val,
					ci->ci_ad[idx]->ad_cname.bv_len);
				if (idx<(ci->ci_ad_num-1)) {
					*ptr++ = ',';
				}
			}
			*ptr = '\0';
			bv.bv_len = ptr - bv.bv_val;

			ber_bvarray_add( &c->rvalue_vals, &bv );
			rc = 0;
		}
		}
		break;
	case LDAP_MOD_DELETE:
		if ( c->valx == -1 ) {
		/* Delete entire attribute */
			collect_info *ci;
			while (( ci = on->on_bi.bi_private )) {
				on->on_bi.bi_private = ci->ci_next;
				ch_free( ci->ci_dn.bv_val );
				ch_free( ci );
			}
		} else {
		/* Delete just one value */
			collect_info **cip, *ci;
			int i;
			cip = (collect_info **)&on->on_bi.bi_private;
			ci = *cip;
			for ( i=0; i < c->valx; i++ ) {
				cip = &ci->ci_next;
				ci = *cip;
			}
			*cip = ci->ci_next;
			ch_free( ci->ci_dn.bv_val );
			ch_free( ci );
		}
		rc = 0;
		break;
	case SLAP_CONFIG_ADD:
	case LDAP_MOD_ADD:
		{
		collect_info *ci;
		struct berval bv, dn;
		const char *text;
		int idx, count=0;
		char *arg;

		/* count delimiters in attribute argument */
		arg = strtok(c->argv[2], ",");
		while (arg!=NULL) {
			count++;
			arg = strtok(NULL, ",");
		}

		/* validate and normalize dn */
		ber_str2bv( c->argv[1], 0, 0, &bv );
		if ( dnNormalize( 0, NULL, NULL, &bv, &dn, NULL ) ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s invalid DN: \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		/* check for duplicate DNs */
		for ( ci = (collect_info *)on->on_bi.bi_private; ci;
			ci = ci->ci_next ) {
			/* If new DN is longest, there are no possible matches */
			if ( dn.bv_len > ci->ci_dn.bv_len ) {
				ci = NULL;
				break;
			}
			if ( bvmatch( &dn, &ci->ci_dn )) {
				break;
			}
		}
		if ( ci ) {
			snprintf( c->cr_msg, sizeof( c->cr_msg ), "%s DN already configured: \"%s\"",
				c->argv[0], c->argv[1] );
			Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
				"%s: %s\n", c->log, c->cr_msg, 0 );
			return ARG_BAD_CONF;
		}

		/* allocate config info with room for attribute array */
		ci = ch_malloc( sizeof( collect_info ) +
			sizeof( AttributeDescription * ) * count );

		/* load attribute description for attribute list */
		arg = c->argv[2];
		for( idx=0; idx<count; idx++) {
			ci->ci_ad[idx] = NULL;

			if ( slap_str2ad( arg, &ci->ci_ad[idx], &text ) ) {
				snprintf( c->cr_msg, sizeof( c->cr_msg ), 
					"%s attribute description unknown: \"%s\"",
					c->argv[0], arg);
				Debug( LDAP_DEBUG_CONFIG|LDAP_DEBUG_NONE,
					"%s: %s\n", c->log, c->cr_msg, 0 );
				ch_free( ci );
				return ARG_BAD_CONF;
			}
			while(*arg!='\0') {
				arg++; /* skip to end of argument */
			}
			if (idx<count-1) {
				arg++; /* skip inner delimiters */
			}
		}

		/* The on->on_bi.bi_private pointer can be used for
		 * anything this instance of the overlay needs.
		 */
		ci->ci_ad[count] = NULL;
		ci->ci_ad_num = count;
		ci->ci_dn = dn;

		/* creates list of ci's ordered by dn length */ 
		insert_ordered ( on, ci );

		/* New ci wasn't simply appended to end, adjust its
		 * position in the config entry's a_vals
		 */
		if ( c->ca_entry && ci->ci_next ) {
			Attribute *a = attr_find( c->ca_entry->e_attrs,
				collectcfg[0].ad );
			if ( a ) {
				struct berval bv, nbv;
				collect_info *c2 = (collect_info *)on->on_bi.bi_private;
				int i, j;
				for ( i=0; c2 != ci; i++, c2 = c2->ci_next );
				bv = a->a_vals[a->a_numvals-1];
				nbv = a->a_nvals[a->a_numvals-1];
				for ( j=a->a_numvals-1; j>i; j-- ) {
					a->a_vals[j] = a->a_vals[j-1];
					a->a_nvals[j] = a->a_nvals[j-1];
				}
				a->a_vals[j] = bv;
				a->a_nvals[j] = nbv;
			}
		}

		rc = 0;
		}
	}
	return rc;
}

static int
collect_destroy(
	BackendDB *be,
	ConfigReply *cr
)
{
	slap_overinst *on = (slap_overinst *)be->bd_info;
	collect_info *ci;

	while (( ci = on->on_bi.bi_private )) {
		on->on_bi.bi_private = ci->ci_next;
		ch_free( ci->ci_dn.bv_val );
		ch_free( ci );
	}
	return 0;
}

static int
collect_modify( Operation *op, SlapReply *rs)
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	collect_info *ci = on->on_bi.bi_private;
	Modifications *ml;
	char errMsg[100];
	int idx;

	for ( ml = op->orm_modlist; ml != NULL; ml = ml->sml_next) {
		for (; ci; ci=ci->ci_next ) {
			/* Is this entry an ancestor of this collectinfo ? */
			if (!dnIsSuffix(&op->o_req_ndn, &ci->ci_dn)) {
				/* this collectinfo does not match */
				continue;
			}

			/* Is this entry the same as the template DN ? */
			if ( dn_match(&op->o_req_ndn, &ci->ci_dn)) {
				/* all changes in this ci are allowed */
				continue;
			}

			/* check for collect attributes - disallow modify if present */
			for(idx=0; idx<ci->ci_ad_num; idx++) {
				if (ml->sml_desc == ci->ci_ad[idx]) {
					rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
					snprintf( errMsg, sizeof( errMsg ), 
						"cannot change virtual attribute '%s'",
						ci->ci_ad[idx]->ad_cname.bv_val);
					rs->sr_text = errMsg;
					send_ldap_result( op, rs );
					return rs->sr_err;
				}
			}
		}

	}

	return SLAP_CB_CONTINUE;
}

static int
collect_response( Operation *op, SlapReply *rs )
{
	slap_overinst *on = (slap_overinst *) op->o_bd->bd_info;
	collect_info *ci = on->on_bi.bi_private;

	/* If we've been configured and the current response is
	 * a search entry
	 */
	if ( ci && rs->sr_type == REP_SEARCH ) {
		int rc;

		op->o_bd->bd_info = (BackendInfo *)on->on_info;

		for (; ci; ci=ci->ci_next ) {
			int idx=0;

			/* Is this entry an ancestor of this collectinfo ? */
			if (!dnIsSuffix(&rs->sr_entry->e_nname, &ci->ci_dn)) {
				/* collectinfo does not match */
				continue;
			}

			/* Is this entry the same as the template DN ? */
			if ( dn_match(&rs->sr_entry->e_nname, &ci->ci_dn)) {
				/* dont apply change to parent */
				continue;
			}

			/* The current entry may live in a cache, so
			* don't modify it directly. Make a copy and
			* work with that instead.
			*/
			rs_entry2modifiable( op, rs, on );

			/* Loop for each attribute in this collectinfo */
			for(idx=0; idx<ci->ci_ad_num; idx++) {
				BerVarray vals = NULL;

				/* Extract the values of the desired attribute from
			 	 * the ancestor entry */
				rc = backend_attribute( op, NULL, &ci->ci_dn, 
					ci->ci_ad[idx], &vals, ACL_READ );

				/* If there are any values, merge them into the
			 	 * current search result
			 	 */
				if ( vals ) {
					attr_merge( rs->sr_entry, ci->ci_ad[idx], 
						vals, NULL );
					ber_bvarray_free_x( vals, op->o_tmpmemctx );
				}
			}
		}
	}

	/* Default is to just fall through to the normal processing */
	return SLAP_CB_CONTINUE;
}

static slap_overinst collect;

int collect_initialize() {
	int code;

	collect.on_bi.bi_type = "collect";
	collect.on_bi.bi_db_destroy = collect_destroy;
	collect.on_bi.bi_op_modify = collect_modify;
	collect.on_response = collect_response;

	collect.on_bi.bi_cf_ocs = collectocs;
	code = config_register_schema( collectcfg, collectocs );
	if ( code ) return code;

	return overlay_register( &collect );
}

#if SLAPD_OVER_COLLECT == SLAPD_MOD_DYNAMIC
int init_module(int argc, char *argv[]) {
	return collect_initialize();
}
#endif

#endif /* SLAPD_OVER_COLLECT */
