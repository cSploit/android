/* Generic time.h */
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
 * A copy of this license is available in file LICENSE in the
 * top-level directory of the distribution or, alternatively, at
 * <http://www.OpenLDAP.org/license.html>.
 */

#ifndef _AC_TIME_H
#define _AC_TIME_H

#ifdef TIME_WITH_SYS_TIME
# include <sys/time.h>
# include <time.h>
#elif defined(HAVE_SYS_TIME_H)
# include <sys/time.h>
# ifdef HAVE_SYS_TIMEB_H
#  include <sys/timeb.h>
# endif
#else
# include <time.h>
#endif

#endif /* _AC_TIME_H */
