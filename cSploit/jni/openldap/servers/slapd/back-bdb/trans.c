/* trans.c - bdb backend transaction routines */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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

#include <stdio.h>
#include <ac/string.h>

#include "back-bdb.h"
#include "lber_pvt.h"
#include "lutil.h"


/* Congestion avoidance code
 * for Deadlock Rollback
 */

void
bdb_trans_backoff( int num_retries )
{
	int i;
	int delay = 0;
	int pow_retries = 1;
	unsigned long key = 0;
	unsigned long max_key = -1;
	struct timeval timeout;

	lutil_entropy( (unsigned char *) &key, sizeof( unsigned long ));

	for ( i = 0; i < num_retries; i++ ) {
		if ( i >= 5 ) break;
		pow_retries *= 4;
	}

	delay = 16384 * (key * (double) pow_retries / (double) max_key);
	delay = delay ? delay : 1;

	Debug( LDAP_DEBUG_TRACE,  "delay = %d, num_retries = %d\n", delay, num_retries, 0 );

	timeout.tv_sec = delay / 1000000;
	timeout.tv_usec = delay % 1000000;
	select( 0, NULL, NULL, NULL, &timeout );
}
