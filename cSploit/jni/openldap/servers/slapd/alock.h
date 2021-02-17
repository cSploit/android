/* alock.h - access lock header */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2005-2014 The OpenLDAP Foundation.
 * Portions Copyright 2004-2005 Symas Corporation.
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
 * This work was initially developed by Emily Backes at Symas
 * Corporation for inclusion in OpenLDAP Software.
 */

#ifndef _ALOCK_H_
#define _ALOCK_H_

#include "portable.h"
#include <ac/time.h>
#include <ac/unistd.h>

/* environment states (all the slots together) */
#define ALOCK_CLEAN		(0)
#define ALOCK_RECOVER	(1)
#define ALOCK_BUSY		(2)
#define ALOCK_UNSTABLE	(3)

/* lock user types and states */
#define ALOCK_UNLOCKED	(0)
#define ALOCK_LOCKED	(1)
#define ALOCK_UNIQUE	(2)
#define ALOCK_DIRTY		(3)

#define ALOCK_SMASK		3

/* lock/state where recovery is not available */
#define	ALOCK_NOSAVE	4

/* constants */
#define ALOCK_SLOT_SIZE		(1024)
#define ALOCK_SLOT_IATTRS	(4)
#define ALOCK_MAX_APPNAME	(ALOCK_SLOT_SIZE - 8 * ALOCK_SLOT_IATTRS)
#define ALOCK_MAGIC			(0x12345678)

LDAP_BEGIN_DECL

typedef struct alock_info {
	int al_fd;
	int al_slot;
} alock_info_t;

typedef struct alock_slot {
	unsigned int al_lock;
	time_t al_stamp;
	pid_t al_pid;
	char * al_appname;
} alock_slot_t;

LDAP_SLAPD_F (int) alock_open LDAP_P(( alock_info_t * info, const char * appname,
	const char * envdir, int locktype ));
LDAP_SLAPD_F (int) alock_scan LDAP_P(( alock_info_t * info ));
LDAP_SLAPD_F (int) alock_close LDAP_P(( alock_info_t * info, int nosave ));
LDAP_SLAPD_F (int) alock_recover LDAP_P(( alock_info_t * info ));

LDAP_END_DECL

#endif
