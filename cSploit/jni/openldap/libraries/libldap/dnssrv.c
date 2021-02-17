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

/*
 * locate LDAP servers using DNS SRV records.
 * Location code based on MIT Kerberos KDC location code.
 */
#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/param.h>
#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "ldap-int.h"

#ifdef HAVE_ARPA_NAMESER_H
#include <arpa/nameser.h>
#endif
#ifdef HAVE_RESOLV_H
#include <resolv.h>
#endif

int ldap_dn2domain(
	LDAP_CONST char *dn_in,
	char **domainp)
{
	int i, j;
	char *ndomain;
	LDAPDN dn = NULL;
	LDAPRDN rdn = NULL;
	LDAPAVA *ava = NULL;
	struct berval domain = BER_BVNULL;
	static const struct berval DC = BER_BVC("DC");
	static const struct berval DCOID = BER_BVC("0.9.2342.19200300.100.1.25");

	assert( dn_in != NULL );
	assert( domainp != NULL );

	*domainp = NULL;

	if ( ldap_str2dn( dn_in, &dn, LDAP_DN_FORMAT_LDAP ) != LDAP_SUCCESS ) {
		return -2;
	}

	if( dn ) for( i=0; dn[i] != NULL; i++ ) {
		rdn = dn[i];

		for( j=0; rdn[j] != NULL; j++ ) {
			ava = rdn[j];

			if( rdn[j+1] == NULL &&
				(ava->la_flags & LDAP_AVA_STRING) &&
				ava->la_value.bv_len &&
				( ber_bvstrcasecmp( &ava->la_attr, &DC ) == 0
				|| ber_bvcmp( &ava->la_attr, &DCOID ) == 0 ) )
			{
				if( domain.bv_len == 0 ) {
					ndomain = LDAP_REALLOC( domain.bv_val,
						ava->la_value.bv_len + 1);

					if( ndomain == NULL ) {
						goto return_error;
					}

					domain.bv_val = ndomain;

					AC_MEMCPY( domain.bv_val, ava->la_value.bv_val,
						ava->la_value.bv_len );

					domain.bv_len = ava->la_value.bv_len;
					domain.bv_val[domain.bv_len] = '\0';

				} else {
					ndomain = LDAP_REALLOC( domain.bv_val,
						ava->la_value.bv_len + sizeof(".") + domain.bv_len );

					if( ndomain == NULL ) {
						goto return_error;
					}

					domain.bv_val = ndomain;
					domain.bv_val[domain.bv_len++] = '.';
					AC_MEMCPY( &domain.bv_val[domain.bv_len],
						ava->la_value.bv_val, ava->la_value.bv_len );
					domain.bv_len += ava->la_value.bv_len;
					domain.bv_val[domain.bv_len] = '\0';
				}
			} else {
				domain.bv_len = 0;
			}
		} 
	}


	if( domain.bv_len == 0 && domain.bv_val != NULL ) {
		LDAP_FREE( domain.bv_val );
		domain.bv_val = NULL;
	}

	ldap_dnfree( dn );
	*domainp = domain.bv_val;
	return 0;

return_error:
	ldap_dnfree( dn );
	LDAP_FREE( domain.bv_val );
	return -1;
}

int ldap_domain2dn(
	LDAP_CONST char *domain_in,
	char **dnp)
{
	char *domain, *s, *tok_r, *dn, *dntmp;
	size_t loc;

	assert( domain_in != NULL );
	assert( dnp != NULL );

	domain = LDAP_STRDUP(domain_in);
	if (domain == NULL) {
		return LDAP_NO_MEMORY;
	}
	dn = NULL;
	loc = 0;

	for (s = ldap_pvt_strtok(domain, ".", &tok_r);
		s != NULL;
		s = ldap_pvt_strtok(NULL, ".", &tok_r))
	{
		size_t len = strlen(s);

		dntmp = (char *) LDAP_REALLOC(dn, loc + sizeof(",dc=") + len );
		if (dntmp == NULL) {
		    if (dn != NULL)
			LDAP_FREE(dn);
		    LDAP_FREE(domain);
		    return LDAP_NO_MEMORY;
		}

		dn = dntmp;

		if (loc > 0) {
		    /* not first time. */
		    strcpy(dn + loc, ",");
		    loc++;
		}
		strcpy(dn + loc, "dc=");
		loc += sizeof("dc=")-1;

		strcpy(dn + loc, s);
		loc += len;
    }

	LDAP_FREE(domain);
	*dnp = dn;
	return LDAP_SUCCESS;
}

/*
 * Lookup and return LDAP servers for domain (using the DNS
 * SRV record _ldap._tcp.domain).
 */
int ldap_domain2hostlist(
	LDAP_CONST char *domain,
	char **list )
{
#ifdef HAVE_RES_QUERY
#define DNSBUFSIZ (64*1024)
    char *request;
    char *hostlist = NULL;
    int rc, len, cur = 0;
    unsigned char reply[DNSBUFSIZ];

	assert( domain != NULL );
	assert( list != NULL );

	if( *domain == '\0' ) {
		return LDAP_PARAM_ERROR;
	}

    request = LDAP_MALLOC(strlen(domain) + sizeof("_ldap._tcp."));
    if (request == NULL) {
		return LDAP_NO_MEMORY;
    }
    sprintf(request, "_ldap._tcp.%s", domain);

    LDAP_MUTEX_LOCK(&ldap_int_resolv_mutex);

    rc = LDAP_UNAVAILABLE;
#ifdef NS_HFIXEDSZ
	/* Bind 8/9 interface */
    len = res_query(request, ns_c_in, ns_t_srv, reply, sizeof(reply));
#	ifndef T_SRV
#		define T_SRV ns_t_srv
#	endif
#else
	/* Bind 4 interface */
#	ifndef T_SRV
#		define T_SRV 33
#	endif

    len = res_query(request, C_IN, T_SRV, reply, sizeof(reply));
#endif
    if (len >= 0) {
	unsigned char *p;
	char host[DNSBUFSIZ];
	int status;
	u_short port;
	/* int priority, weight; */

	/* Parse out query */
	p = reply;

#ifdef NS_HFIXEDSZ
	/* Bind 8/9 interface */
	p += NS_HFIXEDSZ;
#elif defined(HFIXEDSZ)
	/* Bind 4 interface w/ HFIXEDSZ */
	p += HFIXEDSZ;
#else
	/* Bind 4 interface w/o HFIXEDSZ */
	p += sizeof(HEADER);
#endif

	status = dn_expand(reply, reply + len, p, host, sizeof(host));
	if (status < 0) {
	    goto out;
	}
	p += status;
	p += 4;

	while (p < reply + len) {
	    int type, class, ttl, size;
	    status = dn_expand(reply, reply + len, p, host, sizeof(host));
	    if (status < 0) {
		goto out;
	    }
	    p += status;
	    type = (p[0] << 8) | p[1];
	    p += 2;
	    class = (p[0] << 8) | p[1];
	    p += 2;
	    ttl = (p[0] << 24) | (p[1] << 16) | (p[2] << 8) | p[3];
	    p += 4;
	    size = (p[0] << 8) | p[1];
	    p += 2;
	    if (type == T_SRV) {
		int buflen;
		status = dn_expand(reply, reply + len, p + 6, host, sizeof(host));
		if (status < 0) {
		    goto out;
		}
		/* ignore priority and weight for now */
		/* priority = (p[0] << 8) | p[1]; */
		/* weight = (p[2] << 8) | p[3]; */
		port = (p[4] << 8) | p[5];

		if ( port == 0 || host[ 0 ] == '\0' ) {
		    goto add_size;
		}

		buflen = strlen(host) + STRLENOF(":65355 ");
		hostlist = (char *) LDAP_REALLOC(hostlist, cur + buflen + 1);
		if (hostlist == NULL) {
		    rc = LDAP_NO_MEMORY;
		    goto out;
		}
		if (cur > 0) {
		    /* not first time around */
		    hostlist[cur++] = ' ';
		}
		cur += sprintf(&hostlist[cur], "%s:%hu", host, port);
	    }
add_size:;
	    p += size;
	}
    }
    if (hostlist == NULL) {
	/* No LDAP servers found in DNS. */
	rc = LDAP_UNAVAILABLE;
	goto out;
    }

    rc = LDAP_SUCCESS;
	*list = hostlist;

  out:
    LDAP_MUTEX_UNLOCK(&ldap_int_resolv_mutex);

    if (request != NULL) {
	LDAP_FREE(request);
    }
    if (rc != LDAP_SUCCESS && hostlist != NULL) {
	LDAP_FREE(hostlist);
    }
    return rc;
#else
    return LDAP_NOT_SUPPORTED;
#endif /* HAVE_RES_QUERY */
}
