/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
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

#include "perl_back.h"


/**********************************************************
 *
 * Bind
 *
 **********************************************************/
int
perl_back_bind(
	Operation *op,
	SlapReply *rs )
{
	int count;

	PerlBackend *perl_back = (PerlBackend *) op->o_bd->be_private;

	/* allow rootdn as a means to auth without the need to actually
 	 * contact the proxied DSA */
	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		return rs->sr_err;
	}

	PERL_SET_CONTEXT( PERL_INTERPRETER );
	ldap_pvt_thread_mutex_lock( &perl_interpreter_mutex );	

	{
		dSP; ENTER; SAVETMPS;

		PUSHMARK(SP);
		XPUSHs( perl_back->pb_obj_ref );
		XPUSHs(sv_2mortal(newSVpv( op->o_req_dn.bv_val , op->o_req_dn.bv_len)));
		XPUSHs(sv_2mortal(newSVpv( op->orb_cred.bv_val , op->orb_cred.bv_len)));
		PUTBACK;

		count = call_method("bind", G_SCALAR);

		SPAGAIN;

		if (count != 1) {
			croak("Big trouble in back_bind\n");
		}

		rs->sr_err = POPi;
							 

		PUTBACK; FREETMPS; LEAVE;
	}

	ldap_pvt_thread_mutex_unlock( &perl_interpreter_mutex );	

	Debug( LDAP_DEBUG_ANY, "Perl BIND returned 0x%04x\n", rs->sr_err, 0, 0 );

	/* frontend will send result on success (0) */
	if( rs->sr_err != LDAP_SUCCESS )
		send_ldap_result( op, rs );

	return ( rs->sr_err );
}
