/* config.c - passwd backend configuration file routine */
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

#include <ac/socket.h>
#include <ac/string.h>
#include <ac/time.h>

#include "slap.h"
#include "back-passwd.h"
#include "config.h"

static ConfigTable passwdcfg[] = {
	{ "file", "filename", 2, 2, 0,
#ifdef HAVE_SETPWFILE
		ARG_STRING|ARG_OFFSET, NULL,
#else
		ARG_IGNORED, NULL,
#endif
		"( OLcfgDbAt:9.1 NAME 'olcPasswdFile' "
			"DESC 'File containing passwd records' "
			"EQUALITY caseExactMatch "
			"SYNTAX OMsDirectoryString SINGLE-VALUE )", NULL, NULL },
	{ NULL, NULL, 0, 0, 0, ARG_IGNORED,
		NULL, NULL, NULL, NULL }
};

static ConfigOCs passwdocs[] = {
	{ "( OLcfgDbOc:9.1 "
			"NAME 'olcPasswdConfig' "
			"DESC 'Passwd backend configuration' "
			"SUP olcDatabaseConfig "
			"MAY olcPasswdFile )",
			Cft_Database, passwdcfg },
	{ NULL, 0, NULL }
};

int
passwd_back_init_cf( BackendInfo *bi )
{
	bi->bi_cf_ocs = passwdocs;
	return config_register_schema( passwdcfg, passwdocs );
}
