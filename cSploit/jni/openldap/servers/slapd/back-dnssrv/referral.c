/* referral.c - DNS SRV backend referral handler */
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

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "proto-dnssrv.h"

int
dnssrv_back_referrals(
    Operation	*op,
    SlapReply	*rs )
{
	int i;
	int rc = LDAP_OTHER;
	char *domain = NULL;
	char *hostlist = NULL;
	char **hosts = NULL;
	BerVarray urls = NULL;

	if ( BER_BVISEMPTY( &op->o_req_dn ) ) {
		/* FIXME: need some means to determine whether the database
		 * is a glue instance */
		if ( SLAP_GLUE_INSTANCE( op->o_bd ) ) {
			return LDAP_SUCCESS;
		}

		rs->sr_text = "DNS SRV operation upon null (empty) DN disallowed";
		return LDAP_UNWILLING_TO_PERFORM;
	}

	if( get_manageDSAit( op ) ) {
		if( op->o_tag == LDAP_REQ_SEARCH ) {
			return LDAP_SUCCESS;
		}

		rs->sr_text = "DNS SRV problem processing manageDSAit control";
		return LDAP_OTHER;
	} 

	if( ldap_dn2domain( op->o_req_dn.bv_val, &domain ) || domain == NULL ) {
		rs->sr_err = LDAP_REFERRAL;
		rs->sr_ref = default_referral;
		send_ldap_result( op, rs );
		rs->sr_ref = NULL;
		return LDAP_REFERRAL;
	}

	Debug( LDAP_DEBUG_TRACE, "DNSSRV: dn=\"%s\" -> domain=\"%s\"\n",
		op->o_req_dn.bv_val, domain, 0 );

	i = ldap_domain2hostlist( domain, &hostlist );
	if ( i ) {
		Debug( LDAP_DEBUG_TRACE,
			"DNSSRV: domain2hostlist(%s) returned %d\n",
			domain, i, 0 );
		rs->sr_text = "no DNS SRV RR available for DN";
		rc = LDAP_NO_SUCH_OBJECT;
		goto done;
	}

	hosts = ldap_str2charray( hostlist, " " );

	if( hosts == NULL ) {
		Debug( LDAP_DEBUG_TRACE, "DNSSRV: str2charrary error\n", 0, 0, 0 );
		rs->sr_text = "problem processing DNS SRV records for DN";
		goto done;
	}

	for( i=0; hosts[i] != NULL; i++) {
		struct berval url;

		url.bv_len = STRLENOF( "ldap://" ) + strlen( hosts[i] );
		url.bv_val = ch_malloc( url.bv_len + 1 );

		strcpy( url.bv_val, "ldap://" );
		strcpy( &url.bv_val[STRLENOF( "ldap://" )], hosts[i] );

		if ( ber_bvarray_add( &urls, &url ) < 0 ) {
			free( url.bv_val );
			rs->sr_text = "problem processing DNS SRV records for DN";
			goto done;
		}
	}

	Statslog( LDAP_DEBUG_STATS,
	    "%s DNSSRV p=%d dn=\"%s\" url=\"%s\"\n",
	    op->o_log_prefix, op->o_protocol,
		op->o_req_dn.bv_val, urls[0].bv_val, 0 );

	Debug( LDAP_DEBUG_TRACE, "DNSSRV: dn=\"%s\" -> url=\"%s\"\n",
		op->o_req_dn.bv_val, urls[0].bv_val, 0 );

	rs->sr_ref = urls;
	send_ldap_error( op, rs, LDAP_REFERRAL,
		"DNS SRV generated referrals" );
	rs->sr_ref = NULL;
	rc = LDAP_REFERRAL;

done:
	if( domain != NULL ) ch_free( domain );
	if( hostlist != NULL ) ch_free( hostlist );
	if( hosts != NULL ) ldap_charray_free( hosts );
	ber_bvarray_free( urls );
	return rc;
}
