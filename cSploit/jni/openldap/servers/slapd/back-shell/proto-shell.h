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
/* Portions Copyright (c) 1995 Regents of the University of Michigan.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that this notice is preserved and that due credit is given
 * to the University of Michigan at Ann Arbor. The name of the University
 * may not be used to endorse or promote products derived from this
 * software without specific prior written permission. This software
 * is provided ``as is'' without express or implied warranty.
 */
/* ACKNOWLEDGEMENTS:
 * This work was originally developed by the University of Michigan
 * (as part of U-MICH LDAP).
 */

#ifndef PROTO_SHELL_H
#define PROTO_SHELL_H

LDAP_BEGIN_DECL

extern BI_init		shell_back_initialize;

extern BI_open		shell_back_open;
extern BI_close		shell_back_close;
extern BI_destroy	shell_back_destroy;

extern BI_db_init	shell_back_db_init;
extern BI_db_destroy	shell_back_db_destroy;

extern BI_op_bind	shell_back_bind;
extern BI_op_unbind	shell_back_unbind;
extern BI_op_search	shell_back_search;
extern BI_op_compare	shell_back_compare;
extern BI_op_modify	shell_back_modify;
extern BI_op_modrdn	shell_back_modrdn;
extern BI_op_add	shell_back_add;
extern BI_op_delete	shell_back_delete;

extern int shell_back_init_cf( BackendInfo *bi );
LDAP_END_DECL

#endif /* PROTO_SHELL_H */
