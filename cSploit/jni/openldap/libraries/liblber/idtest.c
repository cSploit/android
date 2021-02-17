/* idtest.c - ber decoding test program using isode libraries */
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
/* Portions Copyright (c) 1990 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/stdlib.h>

#ifdef HAVE_PSAP_H
#include <psap.h>
#include <quipu/attr.h>
#endif

int
main( int argc, char **argv )
{
#ifdef HAVE_PSAP_H
	PE	pe;
	PS	psin, psout, pserr;

	/* read the pe from standard in */
	if ( (psin = ps_alloc( std_open )) == NULLPS ) {
		perror( "ps_alloc" );
		exit( EXIT_FAILURE );
	}
	if ( std_setup( psin, stdin ) == NOTOK ) {
		perror( "std_setup" );
		exit( EXIT_FAILURE );
	}
	/* write the pe to standard out */
	if ( (psout = ps_alloc( std_open )) == NULLPS ) {
		perror( "ps_alloc" );
		exit( EXIT_FAILURE );
	}
	if ( std_setup( psout, stdout ) == NOTOK ) {
		perror( "std_setup" );
		exit( EXIT_FAILURE );
	}
	/* pretty print it to standard error */
	if ( (pserr = ps_alloc( std_open )) == NULLPS ) {
		perror( "ps_alloc" );
		exit( EXIT_FAILURE );
	}
	if ( std_setup( pserr, stderr ) == NOTOK ) {
		perror( "std_setup" );
		exit( EXIT_FAILURE );
	}

	while ( (pe = ps2pe( psin )) != NULLPE ) {
		pe2pl( pserr, pe );
		pe2ps( psout, pe );
	}

	exit( EXIT_SUCCESS );
#else
	fprintf(stderr, "requires ISODE X.500 distribution.\n");
	return( EXIT_FAILURE );
#endif
}
