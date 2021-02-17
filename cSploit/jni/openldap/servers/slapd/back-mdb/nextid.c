/* init.c - initialize mdb backend */
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

int mdb_next_id( BackendDB *be, MDB_cursor *mc, ID *out )
{
	struct mdb_info *mdb = (struct mdb_info *) be->be_private;
	int rc;
	ID id = 0;
	MDB_val key;

	rc = mdb_cursor_get(mc, &key, NULL, MDB_LAST);

	switch(rc) {
	case MDB_NOTFOUND:
		rc = 0;
		*out = 1;
		break;
	case 0:
		memcpy( &id, key.mv_data, sizeof( id ));
		*out = ++id;
		break;

	default:
		Debug( LDAP_DEBUG_ANY,
			"=> mdb_next_id: get failed: %s (%d)\n",
			mdb_strerror(rc), rc, 0 );
		goto done;
	}
	mdb->mi_nextid = *out;

done:
	return rc;
}
