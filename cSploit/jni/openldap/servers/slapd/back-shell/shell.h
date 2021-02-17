/* shell.h - shell backend header file */
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

#ifndef SLAPD_SHELL_H
#define SLAPD_SHELL_H

#include "proto-shell.h"

LDAP_BEGIN_DECL

struct shellinfo {
	char	**si_bind;		/* cmd + args to exec for bind	  */
	char	**si_unbind;	/* cmd + args to exec for unbind  */
	char	**si_search;	/* cmd + args to exec for search  */
	char	**si_compare;	/* cmd + args to exec for compare */
	char	**si_modify;	/* cmd + args to exec for modify  */
	char	**si_modrdn;	/* cmd + args to exec for modrdn  */
	char	**si_add;		/* cmd + args to exec for add	  */
	char	**si_delete;	/* cmd + args to exec for delete  */
};

extern pid_t forkandexec LDAP_P((
	char **args,
	FILE **rfp,
	FILE **wfp));

extern void print_suffixes LDAP_P((
	FILE *fp,
	BackendDB *bd));

extern int read_and_send_results LDAP_P((
	Operation *op,
	SlapReply *rs,
	FILE *fp));

LDAP_END_DECL

#endif
