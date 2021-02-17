/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 1999-2014 The OpenLDAP Foundation.
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

/* This file is probably obsolete.  If it is not,        */
/* #inclusion of it may have to be moved.  See ITS#2513. */

/* This file is necessary because both PERL headers */
/* and OpenLDAP define a number of macros without   */
/* checking wether they're already defined */

#ifndef ASPERL_UNDEFS_H
#define ASPERL_UNDEFS_H

/* ActiveState Win32 PERL port support */
/* set in ldap/include/portable.h */
#  ifdef HAVE_WIN32_ASPERL
/* The following macros are undefined to prevent */
/* redefinition in PERL headers*/
#    undef gid_t
#    undef uid_t
#    undef mode_t
#    undef caddr_t
#    undef WIN32_LEAN_AND_MEAN
#  endif
#endif

