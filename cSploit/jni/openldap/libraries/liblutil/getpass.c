/* getpass.c -- get password from user */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
 * Portions Copyright 1998-2003 Kurt D. Zeilenga.
 * Portions Copyright 2009 Howard Chu.
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
/* Portions Copyright (c) 1992, 1993  Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* This work was originally developed by the University of Michigan
 * and distributed as part of U-MICH LDAP.  It was adapted for use in
 * -llutil by Kurt D. Zeilenga and subsequently rewritten by Howard Chu.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#include <ac/ctype.h>
#include <ac/signal.h>
#include <ac/string.h>
#include <ac/termios.h>
#include <ac/time.h>
#include <ac/unistd.h>

#ifndef HAVE_GETPASSPHRASE

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_CONIO_H
#include <conio.h>
#endif

#include <lber.h>
#include <ldap.h>

#include "ldap_defaults.h"

#define PBUF	512

#ifdef HAVE_WINSOCK
#define TTY "con:"
#else
#define TTY "/dev/tty"
#endif

char *
lutil_getpass( const char *prompt )
{
	static char pbuf[PBUF];
	FILE *fi;
	int c;
	unsigned i;
#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	TERMIO_TYPE ttyb;
	TERMFLAG_TYPE flags;
	RETSIGTYPE (*sig)( int sig );
#endif

	if( prompt == NULL ) prompt = _("Password: ");

#ifdef DEBUG
	if (debug & D_TRACE)
		printf("->getpass(%s)\n", prompt);
#endif

#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	if ((fi = fopen(TTY, "r")) == NULL)
		fi = stdin;
	else
		setbuf(fi, (char *)NULL);
	if (fi != stdin) {
		if (GETATTR(fileno(fi), &ttyb) < 0)
			perror("GETATTR");
		sig = SIGNAL (SIGINT, SIG_IGN);
		flags = GETFLAGS( ttyb );
		SETFLAGS( ttyb, flags & ~ECHO );
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
	}
#else
	fi = stdin;
#endif
	fprintf(stderr, "%s", prompt); 
	fflush(stderr);
	i = 0;
	while ( (c = getc(fi)) != EOF && c != '\n' && c != '\r' )
		if ( i < (sizeof(pbuf)-1) )
			pbuf[i++] = c;
#if defined(HAVE_TERMIOS_H) || defined(HAVE_SGTTY_H)
	/* tidy up */
	if (fi != stdin) {
		fprintf(stderr, "\n"); 
		fflush(stderr);
		SETFLAGS( ttyb, flags );
		if (SETATTR(fileno(fi), &ttyb) < 0)
			perror("SETATTR");
		(void) SIGNAL (SIGINT, sig);
		(void) fclose(fi);
	}
#endif
	if ( c == EOF )
		return( NULL );
	pbuf[i] = '\0';
	return (pbuf);
}

#endif /* !NEED_GETPASSPHRASE */
