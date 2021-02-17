/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 Dmitry Kovalev.
 * Portions Copyright 2002 Pierangelo Masarati.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Dmitry Kovalev for inclusion
 * by OpenLDAP Software.  Additional significant contributors include
 * Pierangelo Masarati.
 */

#include "portable.h"

#include <stdio.h>
#include <sys/types.h>

#include "slap.h"
#include "proto-sql.h"

int 
backsql_bind( Operation *op, SlapReply *rs )
{
	SQLHDBC			dbh = SQL_NULL_HDBC;
	Entry			e = { 0 };
	Attribute		*a;
	backsql_srch_info	bsi = { 0 };
	AttributeName		anlist[2];
	int			rc;
 
 	Debug( LDAP_DEBUG_TRACE, "==>backsql_bind()\n", 0, 0, 0 );

	switch ( be_rootdn_bind( op, rs ) ) {
	case SLAP_CB_CONTINUE:
		break;

	default:
		/* in case of success, front end will send result;
		 * otherwise, be_rootdn_bind() did */
 		Debug( LDAP_DEBUG_TRACE, "<==backsql_bind(%d)\n",
			rs->sr_err, 0, 0 );
		return rs->sr_err;
	}

	rs->sr_err = backsql_get_db_conn( op, &dbh );
	if ( rs->sr_err != LDAP_SUCCESS ) {
     		Debug( LDAP_DEBUG_TRACE, "backsql_bind(): "
			"could not get connection handle - exiting\n",
			0, 0, 0 );

		rs->sr_text = ( rs->sr_err == LDAP_OTHER )
			? "SQL-backend error" : NULL;
		goto error_return;
	}

	anlist[0].an_name = slap_schema.si_ad_userPassword->ad_cname;
	anlist[0].an_desc = slap_schema.si_ad_userPassword;
	anlist[1].an_name.bv_val = NULL;

	bsi.bsi_e = &e;
	rc = backsql_init_search( &bsi, &op->o_req_ndn, LDAP_SCOPE_BASE, 
			(time_t)(-1), NULL, dbh, op, rs, anlist,
			BACKSQL_ISF_GET_ENTRY );
	if ( rc != LDAP_SUCCESS ) {
		Debug( LDAP_DEBUG_TRACE, "backsql_bind(): "
			"could not retrieve bindDN ID - no such entry\n", 
			0, 0, 0 );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto error_return;
	}

	a = attr_find( e.e_attrs, slap_schema.si_ad_userPassword );
	if ( a == NULL ) {
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto error_return;
	}

	if ( slap_passwd_check( op, &e, a, &op->oq_bind.rb_cred,
				&rs->sr_text ) != 0 )
	{
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto error_return;
	}

error_return:;
	if ( !BER_BVISNULL( &bsi.bsi_base_id.eid_ndn ) ) {
		(void)backsql_free_entryID( &bsi.bsi_base_id, 0, op->o_tmpmemctx );
	}

	if ( !BER_BVISNULL( &e.e_nname ) ) {
		backsql_entry_clean( op, &e );
	}

	if ( bsi.bsi_attrs != NULL ) {
		op->o_tmpfree( bsi.bsi_attrs, op->o_tmpmemctx );
	}

	if ( rs->sr_err != LDAP_SUCCESS ) {
		send_ldap_result( op, rs );
	}
	
	Debug( LDAP_DEBUG_TRACE,"<==backsql_bind()\n", 0, 0, 0 );

	return rs->sr_err;
}
 
