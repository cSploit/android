/*
 * poll(2) implemented with select(2), for systems without poll(2). 
 * Warning: a call to this poll() takes about 4K of stack space.
 *
 * This file and the accompanying fakepoll.h  
 * are based on fakepoll.h in C++ by 
 * 
 * Greg Parker	 gparker-web@sealiesoftware.com     December 2000
 * This code is in the public domain. 
 *
 * Updated May 2002: 
 * * fix crash when an fd is less than 0
 * * set errno=EINVAL if an fd is greater or equal to FD_SETSIZE
 * * don't set POLLIN or POLLOUT in revents if it wasn't requested 
 *   in events (only happens when an fd is in the poll set twice)
 *
 * Converted to C and spruced up by James K. Lowden December 2008. 
 */

#include <config.h>

#ifndef HAVE_POLL

static char software_version[] = "$Id: fakepoll.c,v 1.12 2011-05-16 08:51:40 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

#include <stdarg.h>
#include <stdio.h>

#include "replacements.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_ERRNO_H
#include <errno.h>
#endif /* HAVE_ERRNO_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if TIME_WITH_SYS_TIME
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# endif
# include <time.h>
#else
# if HAVE_SYS_TIME_H
#  include <sys/time.h>
# else
#  include <time.h>
# endif
#endif

#include <string.h>
#include <assert.h>

int
fakepoll(struct pollfd fds[], int nfds, int timeout)
{
	struct timeval tv, *tvp;
	fd_set fdsr, fdsw, fdsp;
	struct pollfd *p;
	const struct pollfd *endp = fds? fds + nfds : NULL;
	int selected, polled = 0, maxfd = 0;

#if defined(_WIN32)
	typedef int (WSAAPI *WSAPoll_t)(struct pollfd fds[], ULONG nfds, INT timeout);
	static WSAPoll_t poll_p = (WSAPoll_t) -1;
	if (poll_p == (WSAPoll_t) -1) {
		HMODULE mod;

		poll_p = NULL;
		mod = GetModuleHandle("ws2_32");
		if (mod)
			poll_p = (WSAPoll_t) GetProcAddress(mod, "WSAPoll");
	}
	/* Windows 2008 have WSAPoll which is semantically equal to poll */
	if (poll_p != NULL)
		return poll_p(fds, nfds, timeout);
#endif

	if (fds == NULL) {
		errno = EFAULT;
		return -1;
	}

	FD_ZERO(&fdsr);
	FD_ZERO(&fdsw);
	FD_ZERO(&fdsp);

	/* 
	 * Transcribe flags from the poll set to the fd sets. 
	 * Also ensure we don't exceed select's capabilities. 
	 */
	for (p = fds; p < endp; p++) {
		/* Negative fd checks nothing and always reports zero */
		if (p->fd < 0) {
			continue;
		} 

#if defined(_WIN32)
		/* Win32 cares about the number of descriptors, not the highest one. */
		++maxfd;
#else
		if (p->fd > maxfd)
			maxfd = p->fd;
#endif

		/* POLLERR is never set coming in; poll(2) always reports errors */
		/* But don't report if we're not listening to anything at all. */
		if (p->events & POLLIN)
			FD_SET(p->fd, &fdsr);
		if (p->events & POLLOUT)
			FD_SET(p->fd, &fdsw);
		if (p->events != 0)
			FD_SET(p->fd, &fdsp);
	}

	/* 
	 * If any FD is too big for select(2), we need to return an error. 
	 * Which one, though, is debatable.  There's no defined errno for 
	 * this for poll(2) because it's an "impossible" condition; 
	 * there's no such thing as "too many" FD's to check.  
	 * select(2) returns EINVAL, and so do we.  
	 * EFAULT might be better.
	 */ 
#if !defined(_WIN32)
	if (maxfd > FD_SETSIZE) {
		assert(FD_SETSIZE > 0);
		errno = EINVAL;
		return -1;
	}
#endif

	/*
	 * poll timeout is in milliseconds. Convert to struct timeval.
	 *      timeout == -1: wait forever : select timeout of NULL
	 *      timeout ==  0: return immediately : select timeout of zero
	 */
	if (timeout >= 0) {
		tv.tv_sec = timeout / 1000;
		tv.tv_usec = (timeout % 1000) * 1000;
		tvp = &tv;
	} else {
		tvp = NULL;
	}

	/* 
	 * call select(2) 
	 */
	if ((selected = select(maxfd+1, &fdsr, &fdsw, &fdsp, tvp)) < 0) {
		return -1; /* error */
	}
	
	/* timeout, clear all result bits and return zero. */
	if (selected == 0) {	
		for (p = fds; p < endp; p++) {
			p->revents = 0;
		}
		return 0;
	}

	/*
	 * Select found something
	 * Transcribe result from fd sets to poll set.
	 * Return the number of ready fds.
	 */
	for (polled=0, p=fds; p < endp; p++) {
		p->revents = 0;
		/* Negative fd always reports zero */
		if (p->fd < 0) {
			continue;
		}
		
		if ((p->events & POLLIN) && FD_ISSET(p->fd, &fdsr)) {
			p->revents |= POLLIN;
		}
		if ((p->events & POLLOUT) && FD_ISSET(p->fd, &fdsw)) {
			p->revents |= POLLOUT;
		}
		if ((p->events != 0) && FD_ISSET(p->fd, &fdsp)) {
			p->revents |= POLLERR;
		}

		if (p->revents)
			polled++;
	}
	
	assert(polled == selected);

	return polled;
}
#endif /* HAVE_POLL */
