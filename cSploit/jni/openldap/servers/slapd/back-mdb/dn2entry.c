/* dn2entry.c - routines to deal with the dn2id / id2entry glue */
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

#include "back-mdb.h"

/*
 * dn2entry - look up dn in the cache/indexes and return the corresponding
 * entry. If the requested DN is not found and matched is TRUE, return info
 * for the closest ancestor of the DN. Otherwise e is NULL.
 */

int
mdb_dn2entry(
	Operation *op,
	MDB_txn *tid,
	MDB_cursor *m2,
	struct berval *dn,
	Entry **e,
	ID *nsubs,
	int matched )
{
	struct mdb_info *mdb = (struct mdb_info *) op->o_bd->be_private;
	int rc, rc2;
	ID id = NOID;
	struct berval mbv, nmbv;
	MDB_cursor *mc;

	Debug(LDAP_DEBUG_TRACE, "mdb_dn2entry(\"%s\")\n",
		dn->bv_val ? dn->bv_val : "", 0, 0 );

	*e = NULL;

	rc = mdb_dn2id( op, tid, m2, dn, &id, nsubs, &mbv, &nmbv );
	if ( rc ) {
		if ( matched ) {
			rc2 = mdb_cursor_open( tid, mdb->mi_id2entry, &mc );
			if ( rc2 == MDB_SUCCESS ) {
				rc2 = mdb_id2entry( op, mc, id, e );
				mdb_cursor_close( mc );
			}
		}

	} else {
		rc = mdb_cursor_open( tid, mdb->mi_id2entry, &mc );
		if ( rc == MDB_SUCCESS ) {
			rc = mdb_id2entry( op, mc, id, e );
			mdb_cursor_close(mc);
		}
	}
	if ( *e ) {
		(*e)->e_name = mbv;
		if ( rc == MDB_SUCCESS )
			ber_dupbv_x( &(*e)->e_nname, dn, op->o_tmpmemctx );
		else
			ber_dupbv_x( &(*e)->e_nname, &nmbv, op->o_tmpmemctx );
	}

	return rc;
}
