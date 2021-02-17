/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1998-2014 The OpenLDAP Foundation.
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

#ifndef PROTO_PASSWD_H
#define PROTO_PASSWD_H

LDAP_BEGIN_DECL

extern BI_init		passwd_back_initialize;
extern BI_open		passwd_back_open;
extern BI_destroy	passwd_back_destroy;
extern BI_op_search	passwd_back_search;

extern int passwd_back_init_cf( BackendInfo *bi );

extern AttributeDescription	*ad_sn;
extern AttributeDescription	*ad_desc;

LDAP_END_DECL

#endif /* PROTO_PASSWD_H */
