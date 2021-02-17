/* back-relay.h - relay backend header file */
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
 * A copy of this license is available in the file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */
/* ACKNOWLEDGEMENTS:
 * This work was initially developed by Pierangelo Masarati for inclusion
 * in OpenLDAP Software.
 */

#ifndef SLAPD_RELAY_H
#define SLAPD_RELAY_H

#include "proto-back-relay.h"

LDAP_BEGIN_DECL

typedef enum relay_operation_e {
	relay_op_entry_get = op_last,
	relay_op_entry_release,
	relay_op_has_subordinates,
	relay_op_last
} relay_operation_t;

typedef struct relay_back_info {
	BackendDB	*ri_bd;
	struct berval	ri_realsuffix;
	int		ri_massage;
} relay_back_info;

/* Pad relay_back_info if needed to create valid OpExtra key addresses */
#define	RELAY_INFO_SIZE \
	(sizeof(relay_back_info) > (size_t) relay_op_last ? \
	 sizeof(relay_back_info) : (size_t) relay_op_last   )

LDAP_END_DECL

#endif /* SLAPD_RELAY_H */
