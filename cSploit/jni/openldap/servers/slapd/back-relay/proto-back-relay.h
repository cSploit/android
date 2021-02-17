/* proto-back-relay.h - relay backend header file */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2004-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004 Pierangelo Masarati.
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
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#ifndef PROTO_BACK_RELAY
#define PROTO_BACK_RELAY

#include <ldap_cdefs.h>

LDAP_BEGIN_DECL

extern BI_init			relay_back_initialize;

extern BI_db_init		relay_back_db_init;
extern BI_db_open		relay_back_db_open;
extern BI_db_close		relay_back_db_close;
extern BI_db_destroy		relay_back_db_destroy;

extern BI_op_bind		relay_back_op_bind;
extern BI_op_search		relay_back_op_search;
extern BI_op_compare		relay_back_op_compare;
extern BI_op_modify		relay_back_op_modify;
extern BI_op_modrdn		relay_back_op_modrdn;
extern BI_op_add		relay_back_op_add;
extern BI_op_delete		relay_back_op_delete;
extern BI_op_extended		relay_back_op_extended;
extern BI_entry_release_rw	relay_back_entry_release_rw;
extern BI_entry_get_rw		relay_back_entry_get_rw;
extern BI_operational		relay_back_operational;
extern BI_has_subordinates	relay_back_has_subordinates;

LDAP_END_DECL

#endif /* PROTO_BACK_RELAY */

