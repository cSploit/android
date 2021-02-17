/* Generic termios.h */
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

#ifndef _AC_TERMIOS_H
#define _AC_TERMIOS_H

#ifdef HAVE_TERMIOS_H
#include <termios.h>

#ifdef GCWINSZ_IN_SYS_IOCTL
#include <sys/ioctl.h>
#endif

#define TERMIO_TYPE	struct termios
#define TERMFLAG_TYPE	tcflag_t
#define GETATTR( fd, tiop )	tcgetattr((fd), (tiop))
#define SETATTR( fd, tiop )	tcsetattr((fd), TCSANOW /* 0 */, (tiop))
#define GETFLAGS( tio )		((tio).c_lflag)
#define SETFLAGS( tio, flags )	((tio).c_lflag = (flags))

#elif defined( HAVE_SGTTY_H )
#include <sgtty.h>

#ifdef HAVE_SYS_IOCTL_H
#include <sys/ioctl.h>
#endif

#define TERMIO_TYPE	struct sgttyb
#define TERMFLAG_TYPE	int
#define GETATTR( fd, tiop )	ioctl((fd), TIOCGETP, (caddr_t)(tiop))
#define SETATTR( fd, tiop )	ioctl((fd), TIOCSETP, (caddr_t)(tiop))
#define GETFLAGS( tio )     ((tio).sg_flags)
#define SETFLAGS( tio, flags )  ((tio).sg_flags = (flags))

#endif /* HAVE_SGTTY_H */

#endif /* _AC_TERMIOS_H */
