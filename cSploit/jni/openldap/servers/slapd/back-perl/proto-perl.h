/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
 * Portions Copyright 1999 John C. Quillan.
 * Portions Copyright 2002 myinternet Limited.
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

#ifndef PROTO_PERL_H
#define PROTO_PERL_H

LDAP_BEGIN_DECL

extern BI_init		perl_back_initialize;

extern BI_close		perl_back_close;

extern BI_db_init	perl_back_db_init;
extern BI_db_open	perl_back_db_open;
extern BI_db_destroy	perl_back_db_destroy;
extern BI_db_config	perl_back_db_config;

extern BI_op_bind	perl_back_bind;
extern BI_op_search	perl_back_search;
extern BI_op_compare	perl_back_compare;
extern BI_op_modify	perl_back_modify;
extern BI_op_modrdn	perl_back_modrdn;
extern BI_op_add	perl_back_add;
extern BI_op_delete	perl_back_delete;

extern int perl_back_init_cf( BackendInfo *bi );
LDAP_END_DECL

#endif /* PROTO_PERL_H */
