/* uuid.c -- Universally Unique Identifier routines */
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
/* Portions Copyright 2000, John E. Schimmel, All rights reserved.
 * This software is not subject to any license of Mirapoint, Inc.
 *
 * This is free software; you can redistribute and use it
 * under the same terms as OpenLDAP itself.
 */
/* This work was initially developed by John E. Schimmel and adapted
 * for inclusion in OpenLDAP Software by Kurt D. Zeilenga.
 */

/*
 * Sorry this file is so scary, but it needs to run on a wide range of
 * platforms.  The only exported routine is lutil_uuidstr() which is all
 * that LDAP cares about.  It generates a new uuid and returns it in
 * in string form.
 */
#include "portable.h"

#include <limits.h>
#include <stdio.h>
#include <sys/types.h>

#include <ac/stdlib.h>
#include <ac/string.h>	/* get memcmp() */

#ifdef HAVE_UUID_TO_STR
#  include <sys/uuid.h>
#elif defined( HAVE_UUID_GENERATE )
#  include <uuid/uuid.h>
#elif defined( _WIN32 )
#  include <rpc.h>
#else
#  include <ac/socket.h>
#  include <ac/time.h>
#  ifdef HAVE_SYS_SYSCTL_H
#    include <net/if.h>
#    include <sys/sysctl.h>
#    include <net/route.h>
#  endif
#endif

#include <lutil.h>

/* not needed for Windows */
#if !defined(HAVE_UUID_TO_STR) && !defined(HAVE_UUID_GENERATE) && !defined(_WIN32)
static unsigned char *
lutil_eaddr( void )
{
	static unsigned char zero[6];
	static unsigned char eaddr[6];

#ifdef HAVE_SYS_SYSCTL_H
	size_t needed;
	int mib[6];
	char *buf, *next, *lim;
	struct if_msghdr *ifm;
	struct sockaddr_dl *sdl;

	if (memcmp(eaddr, zero, sizeof(eaddr))) {
		return eaddr;
	}

	mib[0] = CTL_NET;
	mib[1] = PF_ROUTE;
	mib[3] = 0;
	mib[3] = 0;
	mib[4] = NET_RT_IFLIST;
	mib[5] = 0;

	if (sysctl(mib, sizeof(mib), NULL, &needed, NULL, 0) < 0) {
		return NULL;
	}

	buf = malloc(needed);
	if( buf == NULL ) return NULL;

	if (sysctl(mib, sizeof(mib), buf, &needed, NULL, 0) < 0) {
		free(buf);
		return NULL;
	}

	lim = buf + needed;
	for (next = buf; next < lim; next += ifm->ifm_msglen) {
		ifm = (struct if_msghdr *)next;
		sdl = (struct sockaddr_dl *)(ifm + 1);

		if ( sdl->sdl_family != AF_LINK || sdl->sdl_alen == 6 ) {
			AC_MEMCPY(eaddr,
				(unsigned char *)sdl->sdl_data + sdl->sdl_nlen,
				sizeof(eaddr));
			free(buf);
			return eaddr;
		}
	}

	free(buf);
	return NULL;

#elif defined( SIOCGIFADDR ) && defined( AFLINK )
	char buf[sizeof(struct ifreq) * 32];
	struct ifconf ifc;
	struct ifreq *ifr;
	struct sockaddr *sa;
	struct sockaddr_dl *sdl;
	unsigned char *p;
	int s, i;

	if (memcmp(eaddr, zero, sizeof(eaddr))) {
		return eaddr;
	}

	s = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( s < 0 ) {
		return NULL;
	}

	ifc.ifc_len = sizeof( buf );
	ifc.ifc_buf = buf;
	memset( buf, 0, sizeof( buf ) );

	i = ioctl( s, SIOCGIFCONF, (char *)&ifc );
	close( s );

	if( i < 0 ) {
		return NULL;
	}

	for ( i = 0; i < ifc.ifc_len; ) {
		ifr = (struct ifreq *)&ifc.ifc_buf[i];
		sa = &ifr->ifr_addr;

		if ( sa->sa_len > sizeof( ifr->ifr_addr ) ) {
			i += sizeof( ifr->ifr_name ) + sa->sa_len;
		} else {
			i += sizeof( *ifr );
		}

		if ( sa->sa_family != AF_LINK ) {
			continue;
		}

		sdl = (struct sockaddr_dl *)sa;

		if ( sdl->sdl_alen == 6 ) {
			AC_MEMCPY(eaddr,
				(unsigned char *)sdl->sdl_data + sdl->sdl_nlen,
				sizeof(eaddr));
			return eaddr;
		}
	}

	return NULL;

#else
	if (memcmp(eaddr, zero, sizeof(eaddr)) == 0) {
		/* XXX - who knows? */
		lutil_entropy( eaddr, sizeof(eaddr) );
		eaddr[0] |= 0x01; /* turn it into a multicast address */
	}

	return eaddr;
#endif
}

#if (ULONG_MAX >> 31 >> 31) > 1 || defined HAVE_LONG_LONG

#if (ULONG_MAX >> 31 >> 31) > 1
    typedef unsigned long       UI64;
	/* 100 usec intervals from 10/10/1582 to 1/1/1970 */
#   define UUID_TPLUS           0x01B21DD2138140ul
#else
    typedef unsigned long long  UI64;
#   define UUID_TPLUS           0x01B21DD2138140ull
#endif

#define high32(i)           ((unsigned long) ((i) >> 32))
#define low32(i)            ((unsigned long) (i) & 0xFFFFFFFFul)
#define set_add64(res, i)   ((res) += (i))
#define set_add64l(res, i)  ((res) += (i))
#define mul64ll(i1, i2)     ((UI64) (i1) * (i2))

#else /* ! (ULONG_MAX >= 64 bits || HAVE_LONG_LONG) */

typedef struct {
	unsigned long high, low;
} UI64;

static const UI64 UUID_TPLUS = { 0x01B21Dul, 0xD2138140ul };

#define high32(i)			 ((i).high)
#define low32(i)			 ((i).low)

/* res += ui64 */
#define set_add64(res, ui64) \
{ \
	res.high += ui64.high; \
	res.low	 = (res.low + ui64.low) & 0xFFFFFFFFul; \
	if (res.low < ui64.low) res.high++; \
}

/* res += ul32 */
#define set_add64l(res, ul32) \
{ \
	res.low	= (res.low + ul32) & 0xFFFFFFFFul; \
	if (res.low < ul32) res.high++; \
}

/* compute i1 * i2 */
static UI64
mul64ll(unsigned long i1, unsigned long i2)
{
	const unsigned int high1 = (i1 >> 16), low1 = (i1 & 0xffff);
	const unsigned int high2 = (i2 >> 16), low2 = (i2 & 0xffff);

	UI64 res;
	unsigned long tmp;

	res.high = (unsigned long) high1 * high2;
	res.low	 = (unsigned long) low1	 * low2;

	tmp = (unsigned long) low1 * high2;
	res.high += (tmp >> 16);
	tmp = (tmp << 16) & 0xFFFFFFFFul;
	res.low = (res.low + tmp) & 0xFFFFFFFFul;
	if (res.low < tmp)
		res.high++;

	tmp = (unsigned long) low2 * high1;
	res.high += (tmp >> 16);
	tmp = (tmp << 16) & 0xFFFFFFFFul;
	res.low = (res.low + tmp) & 0xFFFFFFFFul;
	if (res.low < tmp)
		res.high++;

	return res;
}

#endif /* ULONG_MAX >= 64 bits || HAVE_LONG_LONG */

#endif /* !HAVE_UUID_TO_STR && !HAVE_UUID_GENERATE && !_WIN32 */

/*
** All we really care about is an ISO UUID string.  The format of a UUID is:
**	field			octet		note
**	time_low		0-3		low field of the timestamp
**	time_mid		4-5		middle field of timestamp
**	time_hi_and_version	6-7		high field of timestamp and
**						version number
**	clock_seq_hi_and_resv	8		high field of clock sequence
**						and variant
**	clock_seq_low		9		low field of clock sequence
**	node			10-15		spacially unique identifier
**
** We use DCE version one, and the DCE variant.  Our unique identifier is
** the first ethernet address on the system.
*/
size_t
lutil_uuidstr( char *buf, size_t len )
{
#ifdef HAVE_UUID_TO_STR
	uuid_t uu = {0};
	unsigned rc;
	char *s;
	size_t l;

	uuid_create( &uu, &rc );
	if ( rc != uuid_s_ok ) {
		return 0;
	}

	uuid_to_str( &uu, &s, &rc );
	if ( rc != uuid_s_ok ) {
		return 0;
	}

	l = strlen( s );
	if ( l >= len ) {
		free( s );
		return 0;
	}

	strncpy( buf, s, len );
	free( s );

	return l;

#elif defined( HAVE_UUID_GENERATE )
	uuid_t uu;

	uuid_generate( uu );
	uuid_unparse_lower( uu, buf );
	return strlen( buf );
	
#elif defined( _WIN32 )
	UUID uuid;
	unsigned char *uuidstr;
	size_t uuidlen;

	if( UuidCreate( &uuid ) != RPC_S_OK ) {
		return 0;
	}
 
	if( UuidToString( &uuid, &uuidstr ) !=  RPC_S_OK ) {
		return 0;
	}

	uuidlen = strlen( uuidstr );
	if( uuidlen >= len ) {
		return 0;
	}

	strncpy( buf, uuidstr, len );
	RpcStringFree( &uuidstr );

	return uuidlen;
 
#else
	struct timeval tv;
	UI64 tl;
	unsigned char *nl;
	unsigned short t2, t3, s1;
	unsigned long t1, tl_high;
	unsigned int rc;

	/*
	 * Theoretically we should delay if seq wraps within 100usec but for now
	 * systems are not fast enough to worry about it.
	 */
	static int inited = 0;
	static unsigned short seq;
	
	if (!inited) {
		lutil_entropy( (unsigned char *) &seq, sizeof(seq) );
		inited++;
	}

#ifdef HAVE_GETTIMEOFDAY
	gettimeofday( &tv, 0 );
#else
	time( &tv.tv_sec );
	tv.tv_usec = 0;
#endif

	tl = mul64ll(tv.tv_sec, 10000000UL);
	set_add64l(tl, tv.tv_usec * 10UL);
	set_add64(tl, UUID_TPLUS);

	nl = lutil_eaddr();

	t1 = low32(tl);				/* time_low */
	tl_high = high32(tl);
	t2 = tl_high & 0xffff;		/* time_mid */
	t3 = ((tl_high >> 16) & 0x0fff) | 0x1000;	/* time_hi_and_version */
	s1 = ( ++seq & 0x1fff ) | 0x8000;		/* clock_seq_and_reserved */

	rc = snprintf( buf, len,
		"%08lx-%04x-%04x-%04x-%02x%02x%02x%02x%02x%02x",
		t1, (unsigned) t2, (unsigned) t3, (unsigned) s1,
		(unsigned) nl[0], (unsigned) nl[1],
		(unsigned) nl[2], (unsigned) nl[3],
		(unsigned) nl[4], (unsigned) nl[5] );

	return rc < len ? rc : 0;
#endif
}

int
lutil_uuidstr_from_normalized(
	char		*uuid,
	size_t		uuidlen,
	char		*buf,
	size_t		buflen )
{
	unsigned char nibble;
	int i, d = 0;

	assert( uuid != NULL );
	assert( buf != NULL );

	if ( uuidlen != 16 ) return -1;
	if ( buflen < 36 ) return -1;

	for ( i = 0; i < 16; i++ ) {
		if ( i == 4 || i == 6 || i == 8 || i == 10 ) {
			buf[(i<<1)+d] = '-';
			d += 1;
		}

		nibble = (uuid[i] >> 4) & 0xF;
		if ( nibble < 10 ) {
			buf[(i<<1)+d] = nibble + '0';
		} else {
			buf[(i<<1)+d] = nibble - 10 + 'a';
		}

		nibble = (uuid[i]) & 0xF;
		if ( nibble < 10 ) {
			buf[(i<<1)+d+1] = nibble + '0';
		} else {
			buf[(i<<1)+d+1] = nibble - 10 + 'a';
		}
	}

	if ( buflen > 36 ) buf[36] = '\0';
	return 36;
}

#ifdef TEST
int
main(int argc, char **argv)
{
	char buf1[8], buf2[64];

#ifndef HAVE_UUID_TO_STR
	unsigned char *p = lutil_eaddr();

	if( p ) {
		printf( "Ethernet Address: %02x:%02x:%02x:%02x:%02x:%02x\n",
			(unsigned) p[0], (unsigned) p[1], (unsigned) p[2],
			(unsigned) p[3], (unsigned) p[4], (unsigned) p[5]);
	}
#endif

	if ( lutil_uuidstr( buf1, sizeof( buf1 ) ) ) {
		printf( "UUID: %s\n", buf1 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf1 ) );
	}

	if ( lutil_uuidstr( buf2, sizeof( buf2 ) ) ) {
		printf( "UUID: %s\n", buf2 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf2 ) );
	}

	if ( lutil_uuidstr( buf2, sizeof( buf2 ) ) ) {
		printf( "UUID: %s\n", buf2 );
	} else {
		fprintf( stderr, "too short: %ld\n", (long) sizeof( buf2 ) );
	}

	return 0;
}
#endif
