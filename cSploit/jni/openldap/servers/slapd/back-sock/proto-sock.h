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

#ifndef _PROTO_SOCK_H
#define _PROTO_SOCK_H

LDAP_BEGIN_DECL

extern BI_init		sock_back_initialize;

extern BI_open		sock_back_open;
extern BI_close		sock_back_close;
extern BI_destroy	sock_back_destroy;

extern BI_db_init	sock_back_db_init;
extern BI_db_destroy	sock_back_db_destroy;

extern BI_op_bind	sock_back_bind;
extern BI_op_unbind	sock_back_unbind;
extern BI_op_search	sock_back_search;
extern BI_op_compare	sock_back_compare;
extern BI_op_modify	sock_back_modify;
extern BI_op_modrdn	sock_back_modrdn;
extern BI_op_add	sock_back_add;
extern BI_op_delete	sock_back_delete;

extern int sock_back_init_cf( BackendInfo *bi );

LDAP_END_DECL

#endif /* _PROTO_SOCK_H */
