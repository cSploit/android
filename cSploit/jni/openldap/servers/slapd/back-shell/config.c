/* config.c - shell backend configuration file routine */
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

#include "portable.h"

#include <stdio.h>

#include <ac/string.h>
#include <ac/socket.h>

#include "slap.h"
#include "shell.h"
#include "config.h"

static ConfigDriver shell_cf;

enum {
	SHELL_BIND = 0,
	SHELL_UNBIND = 1,
	SHELL_SEARCH,
	SHELL_COMPARE,
	SHELL_MODIFY,
	SHELL_MODRDN,
	SHELL_ADD,
	SHELL_DELETE
};

static ConfigTable shellcfg[] = {
	{ "bind", "args", 2, 0, 0, ARG_MAGIC|SHELL_BIND, shell_cf,
		"( OLcfgDbAt:10.1 NAME 'olcShellBind' "
			"DESC 'Bind command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "unbind", "args", 2, 0, 0, ARG_MAGIC|SHELL_UNBIND, shell_cf,
		"( OLcfgDbAt:10.2 NAME 'olcShellUnbind' "
			"DESC 'Unbind command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "search", "args", 2, 0, 0, ARG_MAGIC|SHELL_SEARCH, shell_cf,
		"( OLcfgDbAt:10.3 NAME 'olcShellSearch' "
			"DESC 'Search command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "compare", "args", 2, 0, 0, ARG_MAGIC|SHELL_COMPARE, shell_cf,
		"( OLcfgDbAt:10.4 NAME 'olcShellCompare' "
			"DESC 'Compare command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "modify", "args", 2, 0, 0, ARG_MAGIC|SHELL_MODIFY, shell_cf,
		"( OLcfgDbAt:10.5 NAME 'olcShellModify' "
			"DESC 'Modify command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "modrdn", "args", 2, 0, 0, ARG_MAGIC|SHELL_MODRDN, shell_cf,
		"( OLcfgDbAt:10.6 NAME 'olcShellModRDN' "
			"DESC 'ModRDN command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "add", "args", 2, 0, 0, ARG_MAGIC|SHELL_ADD, shell_cf,
		"( OLcfgDbAt:10.7 NAME 'olcShellAdd' "
			"DESC 'Add command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ "delete", "args", 2, 0, 0, ARG_MAGIC|SHELL_DELETE, shell_cf,
		"( OLcfgDbAt:10.8 NAME 'olcShellDelete' "
			"DESC 'Delete command and arguments' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE ) ", NULL, NULL },
	{ NULL }
};

static ConfigOCs shellocs[] = {
	{ "( OLcfgDbOc:10.1 "
		"NAME 'olcShellConfig'  "
		"DESC 'Shell backend configuration' "
		"SUP olcDatabaseConfig "
		"MAY ( olcShellBind $ olcShellUnbind $ olcShellSearch $ "
			"olcShellCompare $ olcShellModify $ olcShellModRDN $ "
			"olcShellAdd $ olcShellDelete ) )",
				Cft_Database, shellcfg },
	{ NULL }
};

static int
shell_cf( ConfigArgs *c )
{
	struct shellinfo	*si = (struct shellinfo *) c->be->be_private;
	char ***arr = &si->si_bind;

	if ( c->op == SLAP_CONFIG_EMIT ) {
		struct berval bv;
		if ( !arr[c->type] ) return 1;
		bv.bv_val = ldap_charray2str( arr[c->type], " " );
		bv.bv_len = strlen( bv.bv_val );
		ber_bvarray_add( &c->rvalue_vals, &bv );
	} else if ( c->op == LDAP_MOD_DELETE ) {
		ldap_charray_free( arr[c->type] );
		arr[c->type] = NULL;
	} else {
		arr[c->type] = ldap_charray_dup( &c->argv[1] );
	}
	return 0;
}

int
shell_back_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = shellocs;
	return config_register_schema( shellcfg, shellocs );
}
