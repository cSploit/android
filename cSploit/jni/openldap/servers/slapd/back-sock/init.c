/* init.c - initialize sock backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2007-2014 The OpenLDAP Foundation.
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
 * This work was initially developed by Brian Candler for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>

#include "slap.h"
#include "back-sock.h"

int
sock_back_initialize(
    BackendInfo	*bi
)
{
	bi->bi_open = 0;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = sock_back_db_init;
	bi->bi_db_config = 0;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;
	bi->bi_db_destroy = sock_back_db_destroy;

	bi->bi_op_bind = sock_back_bind;
	bi->bi_op_unbind = sock_back_unbind;
	bi->bi_op_search = sock_back_search;
	bi->bi_op_compare = sock_back_compare;
	bi->bi_op_modify = sock_back_modify;
	bi->bi_op_modrdn = sock_back_modrdn;
	bi->bi_op_add = sock_back_add;
	bi->bi_op_delete = sock_back_delete;
	bi->bi_op_abandon = 0;

	bi->bi_extended = 0;

	bi->bi_chk_referrals = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	return sock_back_init_cf( bi );
}

int
sock_back_db_init(
    Backend	*be,
	struct config_reply_s *cr
)
{
	struct sockinfo	*si;

	si = (struct sockinfo *) ch_calloc( 1, sizeof(struct sockinfo) );

	be->be_private = si;
	be->be_cf_ocs = be->bd_info->bi_cf_ocs;

	return si == NULL;
}

int
sock_back_db_destroy(
    Backend	*be,
	struct config_reply_s *cr
)
{
	free( be->be_private );
	return 0;
}

#if SLAPD_SOCK == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( sock )

#endif /* SLAPD_SOCK == SLAPD_MOD_DYNAMIC */
