/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 2001-2003 Pierangelo Masarati.
 * Portions Copyright 1999-2003 Howard Chu.
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
 * This work was initially developed by the Howard Chu for inclusion
 * in OpenLDAP Software and subsequently enhanced by Pierangelo
 * Masarati.
 */

#ifndef PROTO_META_H
#define PROTO_META_H

LDAP_BEGIN_DECL

extern BI_init			meta_back_initialize;

extern BI_open			meta_back_open;
extern BI_close			meta_back_close;
extern BI_destroy		meta_back_destroy;

extern BI_db_init		meta_back_db_init;
extern BI_db_open		meta_back_db_open;
extern BI_db_destroy		meta_back_db_destroy;
extern BI_db_config		meta_back_db_config;

extern BI_op_bind		meta_back_bind;
extern BI_op_search		meta_back_search;
extern BI_op_compare		meta_back_compare;
extern BI_op_modify		meta_back_modify;
extern BI_op_modrdn		meta_back_modrdn;
extern BI_op_add		meta_back_add;
extern BI_op_delete		meta_back_delete;
extern BI_op_abandon		meta_back_abandon;

extern BI_connection_destroy	meta_back_conn_destroy;

int meta_back_init_cf( BackendInfo *bi );

LDAP_END_DECL

#endif /* PROTO_META_H */
