/* unbind.c - decode an ldap unbind operation and pass it to a backend db */
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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 *
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"

int
do_unbind( Operation *op, SlapReply *rs )
{
	Debug( LDAP_DEBUG_TRACE, "%s do_unbind\n",
		op->o_log_prefix, 0, 0 );

	/*
	 * Parse the unbind request.  It looks like this:
	 *
	 *	UnBindRequest ::= NULL
	 */

	Statslog( LDAP_DEBUG_STATS, "%s UNBIND\n", op->o_log_prefix,
		0, 0, 0, 0 );

	if ( frontendDB->be_unbind ) {
		op->o_bd = frontendDB;
		(void)frontendDB->be_unbind( op, rs );
		op->o_bd = NULL;
	}

	/* pass the unbind to all backends */
	(void)backend_unbind( op, rs );

	return 0;
}

