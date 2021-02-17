/* $Id: fakepoll.h,v 1.5 2011-05-16 08:51:40 freddy77 Exp $ */

/** \file
 *  \brief Provide poll call where missing
 */

#if !defined(_FAKE_POLL_H) && !defined(HAVE_POLL)
#define _FAKE_POLL_H

#include <config.h>

#if HAVE_LIMITS_H
#include <limits.h>
#endif 

#if HAVE_SYS_SELECT_H
#include <sys/select.h>
#endif 

#if defined(_WIN32)
#include <winsock2.h>
#endif

#if defined(__VMS)
#include <time.h> /* FD_SETSIZE is in here */
#endif

#if !defined(FD_SETSIZE)
# if !defined(OPEN_MAX)
# error cannot establish FD_SETSIZE
# endif
#define FD_SETSIZE OPEN_MAX
#endif

#ifndef _WIN32
/* poll flags */
# define POLLIN  0x0001
# define POLLOUT 0x0004
# define POLLERR 0x0008

/* synonyms */
# define POLLNORM POLLIN
# define POLLPRI POLLIN
# define POLLRDNORM POLLIN
# define POLLRDBAND POLLIN
# define POLLWRNORM POLLOUT
# define POLLWRBAND POLLOUT

/* ignored */
# define POLLHUP 0x0010
# define POLLNVAL 0x0020
typedef struct pollfd {
    int fd;		/* file descriptor to poll */
    short events;	/* events of interest on fd */
    short revents;	/* events that occurred on fd */
} pollfd_t;

#else /* Windows */
/*
 * Windows use different constants then Unix
 * Newer version have a WSAPoll which is equal to Unix poll
 */
# if !defined(POLLRDNORM) && !defined(POLLWRNORM)
#  define POLLIN  0x0300
#  define POLLOUT 0x0010
#  define POLLERR 0x0001
#  define POLLRDNORM 0x0100
#  define POLLWRNORM 0x0010
typedef struct pollfd {
    SOCKET fd;	/* file descriptor to poll */
    short events;	/* events of interest on fd */
    short revents;	/* events that occurred on fd */
} pollfd_t;
# else
typedef struct pollfd pollfd_t;
# endif
#endif

int fakepoll(struct pollfd fds[], int nfds, int timeout);

#endif
