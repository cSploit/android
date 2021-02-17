/* bind.cpp - ndb backend bind routine */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2008-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Howard Chu for inclusion
 * in OpenLDAP Software. This work was sponsored by MySQL.
 */

#include "portable.h"

#include <stdio.h>
#include <ac/string.h>
#include <ac/unistd.h>

#include "back-ndb.h"

extern "C" int
ndb_back_bind( Operation *op, SlapReply *rs )
{
	struct ndb_info *ni = (struct ndb_info *) op->o_bd->be_private;
	Entry		e = {0};
	Attribute	*a;

	AttributeDescription *password = slap_schema.si_ad_userPassword;

	NdbArgs NA;

	Debug( LDAP_DEBUG_ARGS,
		"==> " LDAP_XSTRING(ndb_back_bind) ": dn: %s\n",
		op->o_req_dn.bv_val, 0, 0);

	/* allow noauth binds */
	switch ( be_rootdn_bind( op, NULL ) ) {
	case LDAP_SUCCESS:
		/* frontend will send result */
		return rs->sr_err = LDAP_SUCCESS;

	default:
		/* give the database a chance */
		break;
	}

	/* Get our NDB handle */
	rs->sr_err = ndb_thread_handle( op, &NA.ndb );

	e.e_name = op->o_req_dn;
	e.e_nname = op->o_req_ndn;
	NA.e = &e;

dn2entry_retry:
	NA.txn = NA.ndb->startTransaction();
	rs->sr_text = NULL;
	if( !NA.txn ) {
		Debug( LDAP_DEBUG_TRACE,
			LDAP_XSTRING(ndb_back_bind) ": startTransaction failed: %s (%d)\n",
			NA.ndb->getNdbError().message, NA.ndb->getNdbError().code, 0 );
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto done;
	}

	/* get entry */
	{
		NdbRdns rdns;
		rdns.nr_num = 0;
		NA.rdns = &rdns;
		NA.ocs = NULL;
		rs->sr_err = ndb_entry_get_info( op, &NA, 0, NULL );
	}
	switch(rs->sr_err) {
	case 0:
		break;
	case LDAP_NO_SUCH_OBJECT:
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	case LDAP_BUSY:
		rs->sr_text = "ldap_server_busy";
		goto done;
#if 0
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto dn2entry_retry;
#endif
	default:
		rs->sr_err = LDAP_OTHER;
		rs->sr_text = "internal error";
		goto done;
	}

	rs->sr_err = ndb_entry_get_data( op, &NA, 0 );
	ber_bvarray_free_x( NA.ocs, op->o_tmpmemctx );
	ber_dupbv( &op->oq_bind.rb_edn, &e.e_name );

	/* check for deleted */
	if ( is_entry_subentry( &e ) ) {
		/* entry is an subentry, don't allow bind */
		Debug( LDAP_DEBUG_TRACE, "entry is subentry\n", 0,
			0, 0 );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	if ( is_entry_alias( &e ) ) {
		/* entry is an alias, don't allow bind */
		Debug( LDAP_DEBUG_TRACE, "entry is alias\n", 0, 0, 0 );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	if ( is_entry_referral( &e ) ) {
		Debug( LDAP_DEBUG_TRACE, "entry is referral\n", 0,
			0, 0 );
		rs->sr_err = LDAP_INVALID_CREDENTIALS;
		goto done;
	}

	switch ( op->oq_bind.rb_method ) {
	case LDAP_AUTH_SIMPLE:
		a = attr_find( e.e_attrs, password );
		if ( a == NULL ) {
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			goto done;
		}

		if ( slap_passwd_check( op, &e, a, &op->oq_bind.rb_cred,
					&rs->sr_text ) != 0 )
		{
			/* failure; stop front end from sending result */
			rs->sr_err = LDAP_INVALID_CREDENTIALS;
			goto done;
		}
			
		rs->sr_err = 0;
		break;

	default:
		assert( 0 ); /* should not be reachable */
		rs->sr_err = LDAP_STRONG_AUTH_NOT_SUPPORTED;
		rs->sr_text = "authentication method not supported";
	}

done:
	NA.txn->close();
	if ( e.e_attrs ) {
		attrs_free( e.e_attrs );
		e.e_attrs = NULL;
	}
	if ( rs->sr_err ) {
		send_ldap_result( op, rs );
	}
	/* front end will send result on success (rs->sr_err==0) */
	return rs->sr_err;
}
