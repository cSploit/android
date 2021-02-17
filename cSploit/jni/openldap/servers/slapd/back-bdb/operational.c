/* operational.c - bdb backend operational attributes function */
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
#include <ac/socket.h>

#include "slap.h"
#include "back-bdb.h"

/*
 * sets *hasSubordinates to LDAP_COMPARE_TRUE/LDAP_COMPARE_FALSE
 * if the entry has children or not.
 */
int
bdb_hasSubordinates(
	Operation	*op,
	Entry		*e,
	int		*hasSubordinates )
{
	struct bdb_info *bdb = (struct bdb_info *) op->o_bd->be_private;
	struct bdb_op_info	*opinfo;
	OpExtra *oex;
	DB_TXN		*rtxn;
	int		rc;
	int		release = 0;
	
	assert( e != NULL );

	/* NOTE: this should never happen, but it actually happens
	 * when using back-relay; until we find a better way to
	 * preserve entry's private information while rewriting it,
	 * let's disable the hasSubordinate feature for back-relay.
	 */
	if ( BEI( e ) == NULL ) {
		Entry *ee = NULL;
		rc = be_entry_get_rw( op, &e->e_nname, NULL, NULL, 0, &ee );
		if ( rc != LDAP_SUCCESS || ee == NULL ) {
			rc = LDAP_OTHER;
			goto done;
		}
		e = ee;
		release = 1;
		if ( BEI( ee ) == NULL ) {
			rc = LDAP_OTHER;
			goto done;
		}
	}

	/* Check for a txn in a parent op, otherwise use reader txn */
	LDAP_SLIST_FOREACH( oex, &op->o_extra, oe_next ) {
		if ( oex->oe_key == bdb )
			break;
	}
	opinfo = (struct bdb_op_info *) oex;
	if ( opinfo && opinfo->boi_txn ) {
		rtxn = opinfo->boi_txn;
	} else {
		rc = bdb_reader_get(op, bdb->bi_dbenv, &rtxn);
		if ( rc ) {
			rc = LDAP_OTHER;
			goto done;
		}
	}

retry:
	/* FIXME: we can no longer assume the entry's e_private
	 * field is correctly populated; so we need to reacquire
	 * it with reader lock */
	rc = bdb_cache_children( op, rtxn, e );

	switch( rc ) {
	case DB_LOCK_DEADLOCK:
	case DB_LOCK_NOTGRANTED:
		goto retry;

	case 0:
		*hasSubordinates = LDAP_COMPARE_TRUE;
		break;

	case DB_NOTFOUND:
		*hasSubordinates = LDAP_COMPARE_FALSE;
		rc = LDAP_SUCCESS;
		break;

	default:
		Debug(LDAP_DEBUG_ARGS, 
			"<=- " LDAP_XSTRING(bdb_hasSubordinates)
			": has_children failed: %s (%d)\n", 
			db_strerror(rc), rc, 0 );
		rc = LDAP_OTHER;
	}

done:;
	if ( release && e != NULL ) be_entry_release_r( op, e );
	return rc;
}

/*
 * sets the supported operational attributes (if required)
 */
int
bdb_operational(
	Operation	*op,
	SlapReply	*rs )
{
	Attribute	**ap;

	assert( rs->sr_entry != NULL );

	for ( ap = &rs->sr_operational_attrs; *ap; ap = &(*ap)->a_next ) {
		if ( (*ap)->a_desc == slap_schema.si_ad_hasSubordinates ) {
			break;
		}
	}

	if ( *ap == NULL &&
		attr_find( rs->sr_entry->e_attrs, slap_schema.si_ad_hasSubordinates ) == NULL &&
		( SLAP_OPATTRS( rs->sr_attr_flags ) ||
			ad_inlist( slap_schema.si_ad_hasSubordinates, rs->sr_attrs ) ) )
	{
		int	hasSubordinates, rc;

		rc = bdb_hasSubordinates( op, rs->sr_entry, &hasSubordinates );
		if ( rc == LDAP_SUCCESS ) {
			*ap = slap_operational_hasSubordinate( hasSubordinates == LDAP_COMPARE_TRUE );
			assert( *ap != NULL );

			ap = &(*ap)->a_next;
		}
	}

	return LDAP_SUCCESS;
}

