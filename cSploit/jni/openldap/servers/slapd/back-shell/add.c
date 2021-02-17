/* add.c - shell backend add function */
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
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"

int
shell_back_add(
    Operation	*op,
    SlapReply	*rs )
{
	struct shellinfo	*si = (struct shellinfo *) op->o_bd->be_private;
	AttributeDescription *entry = slap_schema.si_ad_entry;
	FILE			*rfp, *wfp;
	int			len;

	if ( si->si_add == NULL ) {
		send_ldap_error( op, rs, LDAP_UNWILLING_TO_PERFORM,
		    "add not implemented" );
		return( -1 );
	}

	if ( ! access_allowed( op, op->oq_add.rs_e,
		entry, NULL, ACL_WADD, NULL ) )
	{
		send_ldap_error( op, rs, LDAP_INSUFFICIENT_ACCESS, NULL );
		return -1;
	}

	if ( forkandexec( si->si_add, &rfp, &wfp ) == (pid_t)-1 ) {
		send_ldap_error( op, rs, LDAP_OTHER,
		    "could not fork/exec" );
		return( -1 );
	}

	/* write out the request to the add process */
	fprintf( wfp, "ADD\n" );
	fprintf( wfp, "msgid: %ld\n", (long) op->o_msgid );
	print_suffixes( wfp, op->o_bd );
	ldap_pvt_thread_mutex_lock( &entry2str_mutex );
	fprintf( wfp, "%s", entry2str( op->oq_add.rs_e, &len ) );
	ldap_pvt_thread_mutex_unlock( &entry2str_mutex );
	fclose( wfp );

	/* read in the result and send it along */
	read_and_send_results( op, rs, rfp );

	fclose( rfp );
	return( 0 );
}
