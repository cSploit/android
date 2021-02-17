/* init.c - initialize bdb backend */
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

#include "back-bdb.h"

int bdb_next_id( BackendDB *be, ID *out )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;

	ldap_pvt_thread_mutex_lock( &bdb->bi_lastid_mutex );
	*out = ++bdb->bi_lastid;
	ldap_pvt_thread_mutex_unlock( &bdb->bi_lastid_mutex );

	return 0;
}

int bdb_last_id( BackendDB *be, DB_TXN *tid )
{
	struct bdb_info *bdb = (struct bdb_info *) be->be_private;
	int rc;
	ID id = 0;
	unsigned char idbuf[sizeof(ID)];
	DBT key, data;
	DBC *cursor;

	DBTzero( &key );
	key.flags = DB_DBT_USERMEM;
	key.data = (char *) idbuf;
	key.ulen = sizeof( idbuf );

	DBTzero( &data );
	data.flags = DB_DBT_USERMEM | DB_DBT_PARTIAL;

	/* Get a read cursor */
	rc = bdb->bi_id2entry->bdi_db->cursor( bdb->bi_id2entry->bdi_db,
		tid, &cursor, 0 );

	if (rc == 0) {
		rc = cursor->c_get(cursor, &key, &data, DB_LAST);
		cursor->c_close(cursor);
	}

	switch(rc) {
	case DB_NOTFOUND:
		rc = 0;
		break;
	case 0:
		BDB_DISK2ID( idbuf, &id );
		break;

	default:
		Debug( LDAP_DEBUG_ANY,
			"=> bdb_last_id: get failed: %s (%d)\n",
			db_strerror(rc), rc, 0 );
		goto done;
	}

	bdb->bi_lastid = id;

done:
	return rc;
}
