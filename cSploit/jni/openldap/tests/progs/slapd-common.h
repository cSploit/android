/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted only as authorized by the OpenLDAP
 * Public License.
 *
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software.
 */

#ifndef SLAPD_COMMON_H
#define SLAPD_COMMON_H

typedef enum {
	TESTER_TESTER,
	TESTER_ADDEL,
	TESTER_BIND,
	TESTER_MODIFY,
	TESTER_MODRDN,
	TESTER_READ,
	TESTER_SEARCH,
	TESTER_LAST
} tester_t;

extern void tester_init( const char *pname, tester_t ptype );
extern char * tester_uri( char *uri, char *host, int port );
extern void tester_error( const char *msg );
extern void tester_perror( const char *fname, const char *msg );
extern void tester_ldap_error( LDAP *ld, const char *fname, const char *msg );
extern int tester_ignore_str2errlist( const char *err );
extern int tester_ignore_err( int err );

extern pid_t		pid;

#endif /* SLAPD_COMMON_H */
