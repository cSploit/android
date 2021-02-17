/* Generic Regex */
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

#ifndef _AC_REGEX_H_
#define _AC_REGEX_H_

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifndef HAVE_REGEX_H
/*	NO POSIX REGEX!!
 *  You'll need to install a POSIX compatible REGEX library.
 *  Either Henry Spencer's or GNU regex will do.
 */
#error "No POSIX REGEX available."

#elif HAVE_GNUREGEX_H
	/* system has GNU gnuregex.h */
#	include <gnuregex.h>
#else
	/* have regex.h, assume it's POSIX compliant */
#	include <regex.h>
#endif /* regex.h */

#endif /* _AC_REGEX_H_ */
