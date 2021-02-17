/* init.c - initialize ldap backend */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
 * Portions Copyright 2000-2003 Kurt D. Zeilenga.
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
 * This work was originally developed by Kurt D. Zeilenga for inclusion
 * in OpenLDAP Software.
 */

#include "portable.h"

#include <stdio.h>

#include <ac/socket.h>
#include <ac/param.h>
#include <ac/string.h>

#include "slap.h"
#include "config.h"
#include "proto-dnssrv.h"

int
dnssrv_back_initialize(
    BackendInfo	*bi )
{
	static char *controls[] = {
		LDAP_CONTROL_MANAGEDSAIT,
		NULL
	};

	bi->bi_controls = controls;

	bi->bi_open = dnssrv_back_open;
	bi->bi_config = 0;
	bi->bi_close = 0;
	bi->bi_destroy = 0;

	bi->bi_db_init = 0;
	bi->bi_db_destroy = 0;
	bi->bi_db_config = 0 /* dnssrv_back_db_config */;
	bi->bi_db_open = 0;
	bi->bi_db_close = 0;

	bi->bi_chk_referrals = dnssrv_back_referrals;

	bi->bi_op_bind = dnssrv_back_bind;
	bi->bi_op_search = dnssrv_back_search;
	bi->bi_op_compare = 0 /* dnssrv_back_compare */;
	bi->bi_op_modify = 0;
	bi->bi_op_modrdn = 0;
	bi->bi_op_add = 0;
	bi->bi_op_delete = 0;
	bi->bi_op_abandon = 0;
	bi->bi_op_unbind = 0;

	bi->bi_extended = 0;

	bi->bi_connection_init = 0;
	bi->bi_connection_destroy = 0;

	bi->bi_access_allowed = slap_access_always_allowed;

	return 0;
}

AttributeDescription	*ad_dc;
AttributeDescription	*ad_associatedDomain;

int
dnssrv_back_open(
    BackendInfo *bi )
{
	const char *text;

	(void)slap_str2ad( "dc", &ad_dc, &text );
	(void)slap_str2ad( "associatedDomain", &ad_associatedDomain, &text );

	return 0;
}

int
dnssrv_back_db_init(
	Backend	*be,
	ConfigReply *cr)
{
	return 0;
}

int
dnssrv_back_db_destroy(
	Backend	*be,
	ConfigReply *cr )
{
	return 0;
}

#if SLAPD_DNSSRV == SLAPD_MOD_DYNAMIC

/* conditionally define the init_module() function */
SLAP_BACKEND_INIT_MODULE( dnssrv )

#endif /* SLAPD_DNSSRV == SLAPD_MOD_DYNAMIC */

