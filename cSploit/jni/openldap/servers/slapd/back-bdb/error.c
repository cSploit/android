/* error.c - BDB errcall routine */
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

#include "slap.h"
#include "back-bdb.h"

#if DB_VERSION_FULL < 0x04030000
void bdb_errcall( const char *pfx, char * msg )
#else
void bdb_errcall( const DB_ENV *env, const char *pfx, const char * msg )
#endif
{
#ifdef HAVE_EBCDIC
	if ( msg[0] > 0x7f )
		__etoa( msg );
#endif
	Debug( LDAP_DEBUG_ANY, "bdb(%s): %s\n", pfx, msg, 0 );
}

#if DB_VERSION_FULL >= 0x04030000
void bdb_msgcall( const DB_ENV *env, const char *msg )
{
#ifdef HAVE_EBCDIC
	if ( msg[0] > 0x7f )
		__etoa( msg );
#endif
	Debug( LDAP_DEBUG_TRACE, "bdb: %s\n", msg, 0, 0 );
}
#endif

#ifdef HAVE_EBCDIC

#undef db_strerror

/* Not re-entrant! */
char *ebcdic_dberror( int rc )
{
	static char msg[1024];

	strcpy( msg, db_strerror( rc ) );
	__etoa( msg );
	return msg;
}
#endif
