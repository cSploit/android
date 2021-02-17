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

#include "portable.h"
#include <ac/socket.h>
#include <ac/unistd.h>

#include <lutil.h>

/* Return a pair of socket descriptors that are connected to each other.
 * The returned descriptors are suitable for use with select(). The two
 * descriptors may or may not be identical; the function may return
 * the same descriptor number in both slots. It is guaranteed that
 * data written on sds[1] will be readable on sds[0]. The returned
 * descriptors may be datagram oriented, so data should be written
 * in reasonably small pieces and read all at once. On Unix systems
 * this function is best implemented using a single pipe() call.
 */

int lutil_pair( ber_socket_t sds[2] )
{
#ifdef USE_PIPE
	return pipe( sds );
#else
	struct sockaddr_in si;
	int rc;
	ber_socklen_t len = sizeof(si);
	ber_socket_t sd;

	sd = socket( AF_INET, SOCK_DGRAM, 0 );
	if ( sd == AC_SOCKET_INVALID ) {
		return sd;
	}
	
	(void) memset( (void*) &si, '\0', len );
	si.sin_family = AF_INET;
	si.sin_port = 0;
	si.sin_addr.s_addr = htonl( INADDR_LOOPBACK );

	rc = bind( sd, (struct sockaddr *)&si, len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	rc = getsockname( sd, (struct sockaddr *)&si, &len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	rc = connect( sd, (struct sockaddr *)&si, len );
	if ( rc == AC_SOCKET_ERROR ) {
		tcp_close(sd);
		return rc;
	}

	sds[0] = sd;
#if !HAVE_WINSOCK
	sds[1] = dup( sds[0] );
#else
	sds[1] = sds[0];
#endif
	return 0;
#endif
}
