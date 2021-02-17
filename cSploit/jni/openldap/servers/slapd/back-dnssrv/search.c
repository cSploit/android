/* search.c - DNS SRV backend search function */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "proto-dnssrv.h"

int
dnssrv_back_search(
    Operation	*op,
    SlapReply	*rs )
{
	int i;
	int rc;
	char *domain = NULL;
	char *hostlist = NULL;
	char **hosts = NULL;
	char *refdn;
	struct berval nrefdn = BER_BVNULL;
	BerVarray urls = NULL;
	int manageDSAit;

	rs->sr_ref = NULL;

	if ( BER_BVISEMPTY( &op->o_req_ndn ) ) {
		/* FIXME: need some means to determine whether the database
		 * is a glue instance; if we got here with empty DN, then
		 * we passed this same test in dnssrv_back_referrals() */
		if ( !SLAP_GLUE_INSTANCE( op->o_bd ) ) {
			rs->sr_err = LDAP_UNWILLING_TO_PERFORM;
			rs->sr_text = "DNS SRV operation upon null (empty) DN disallowed";

		} else {
			rs->sr_err = LDAP_SUCCESS;
		}
		goto done;
	}

	manageDSAit = get_manageDSAit( op );
	/*
	 * FIXME: we may return a referral if manageDSAit is not set
	 */
	if ( !manageDSAit ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
				"manageDSAit must be set" );
		goto done;
	}

	if( ldap_dn2domain( op->o_req_dn.bv_val, &domain ) || domain == NULL ) {
		rs->sr_err = LDAP_REFERRAL;
		rs->sr_ref = default_referral;
		send_ldap_result( op, rs );
		rs->sr_ref = NULL;
		goto done;
	}

	Debug( LDAP_DEBUG_TRACE, "DNSSRV: dn=\"%s\" -> domain=\"%s\"\n",
		op->o_req_dn.bv_len ? op->o_req_dn.bv_val : "", domain, 0 );

	if( ( rc = ldap_domain2hostlist( domain, &hostlist ) ) ) {
		Debug( LDAP_DEBUG_TRACE, "DNSSRV: domain2hostlist returned %d\n",
			rc, 0, 0 );
		send_ldap_error( op, rs, LDAP_NO_SUCH_OBJECT,
			"no DNS SRV RR available for DN" );
		goto done;
	}

	hosts = ldap_str2charray( hostlist, " " );

	if( hosts == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "DNSSRV: str2charrary error\n", 0, 0, 0 );
		send_ldap_error( op, rs, LDAP_OTHER,
			"problem processing DNS SRV records for DN" );
		goto done;
	}

	for( i=0; hosts[i] != NULL; i++) {
		struct berval url;

		url.bv_len = STRLENOF( "ldap://" ) + strlen(hosts[i]);
		url.bv_val = ch_malloc( url.bv_len + 1 );

		strcpy( url.bv_val, "ldap://" );
		strcpy( &url.bv_val[STRLENOF( "ldap://" )], hosts[i] );

		if( ber_bvarray_add( &urls, &url ) < 0 ) {
			free( url.bv_val );
			send_ldap_error( op, rs, LDAP_OTHER,
			"problem processing DNS SRV records for DN" );
			goto done;
		}
	}

	Statslog( LDAP_DEBUG_STATS,
	    "%s DNSSRV p=%d dn=\"%s\" url=\"%s\"\n",
	    op->o_log_prefix, op->o_protocol,
		op->o_req_dn.bv_len ? op->o_req_dn.bv_val : "", urls[0].bv_val, 0 );

	Debug( LDAP_DEBUG_TRACE,
		"DNSSRV: ManageDSAit scope=%d dn=\"%s\" -> url=\"%s\"\n",
		op->oq_search.rs_scope,
		op->o_req_dn.bv_len ? op->o_req_dn.bv_val : "",
		urls[0].bv_val );

	rc = ldap_domain2dn(domain, &refdn);

	if( rc != LDAP_SUCCESS ) {
		send_ldap_error( op, rs, LDAP_OTHER,
			"DNS SRV problem processing manageDSAit control" );
		goto done;

	} else {
		struct berval bv;
		bv.bv_val = refdn;
		bv.bv_len = strlen( refdn );

		rc = dnNormalize( 0, NULL, NULL, &bv, &nrefdn, op->o_tmpmemctx );
		if( rc != LDAP_SUCCESS ) {
			send_ldap_error( op, rs, LDAP_OTHER,
				"DNS SRV problem processing manageDSAit control" );
			goto done;
		}
	}

	if( !dn_match( &nrefdn, &op->o_req_ndn ) ) {
		/* requested dn is subordinate */

		Debug( LDAP_DEBUG_TRACE,
			"DNSSRV: dn=\"%s\" subordinate to refdn=\"%s\"\n",
			op->o_req_dn.bv_len ? op->o_req_dn.bv_val : "",
			refdn == NULL ? "" : refdn,
			NULL );

		rs->sr_matched = refdn;
		rs->sr_err = LDAP_NO_SUCH_OBJECT;
		send_ldap_result( op, rs );
		rs->sr_matched = NULL;

	} else if ( op->oq_search.rs_scope == LDAP_SCOPE_ONELEVEL ) {
		send_ldap_error( op, rs, LDAP_SUCCESS, NULL );

	} else {
		Entry e = { 0 };
		AttributeDescription *ad_objectClass
			= slap_schema.si_ad_objectClass;
		AttributeDescription *ad_ref = slap_schema.si_ad_ref;
		e.e_name.bv_val = ch_strdup( op->o_req_dn.bv_val );
		e.e_name.bv_len = op->o_req_dn.bv_len;
		e.e_nname.bv_val = ch_strdup( op->o_req_ndn.bv_val );
		e.e_nname.bv_len = op->o_req_ndn.bv_len;

		e.e_attrs = NULL;
		e.e_private = NULL;

		attr_merge_one( &e, ad_objectClass, &slap_schema.si_oc_referral->soc_cname, NULL );
		attr_merge_one( &e, ad_objectClass, &slap_schema.si_oc_extensibleObject->soc_cname, NULL );

		if ( ad_dc ) {
			char		*p;
			struct berval	bv;

			bv.bv_val = domain;

			p = strchr( bv.bv_val, '.' );
					
			if ( p == bv.bv_val ) {
				bv.bv_len = 1;

			} else if ( p != NULL ) {
				bv.bv_len = p - bv.bv_val;

			} else {
				bv.bv_len = strlen( bv.bv_val );
			}

			attr_merge_normalize_one( &e, ad_dc, &bv, NULL );
		}

		if ( ad_associatedDomain ) {
			struct berval	bv;

			ber_str2bv( domain, 0, 0, &bv );
			attr_merge_normalize_one( &e, ad_associatedDomain, &bv, NULL );
		}

		attr_merge_normalize_one( &e, ad_ref, urls, NULL );

		rc = test_filter( op, &e, op->oq_search.rs_filter ); 

		if( rc == LDAP_COMPARE_TRUE ) {
			rs->sr_entry = &e;
			rs->sr_attrs = op->oq_search.rs_attrs;
			rs->sr_flags = REP_ENTRY_MODIFIABLE;
			send_search_entry( op, rs );
			rs->sr_entry = NULL;
			rs->sr_attrs = NULL;
			rs->sr_flags = 0;
		}

		entry_clean( &e );

		rs->sr_err = LDAP_SUCCESS;
		send_ldap_result( op, rs );
	}

	if ( refdn ) free( refdn );
	if ( nrefdn.bv_val ) free( nrefdn.bv_val );

done:
	if( domain != NULL ) ch_free( domain );
	if( hostlist != NULL ) ch_free( hostlist );
	if( hosts != NULL ) ldap_charray_free( hosts );
	if( urls != NULL ) ber_bvarray_free( urls );
	return 0;
}
