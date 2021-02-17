/* getpeereid.c */
/* $OpenLDAP$ */
/* This work is part of OpenLDAP Software <http://www.openldap.org/>.
 *
 * Copyright 2000-2014 The OpenLDAP Foundation.
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

#ifndef _GNU_SOURCE
#define _GNU_SOURCE 1			/* Needed for glibc struct ucred */
#endif

#include "portable.h"

#ifndef HAVE_GETPEEREID

#include <sys/types.h>
#include <ac/unistd.h>

#include <ac/socket.h>
#include <ac/errno.h>

#ifdef HAVE_GETPEERUCRED
#include <ucred.h>
#endif

#ifdef LDAP_PF_LOCAL_SENDMSG
#include <lber.h>
#ifdef HAVE_SYS_UIO_H
#include <sys/uio.h>
#endif
#include <sys/stat.h>
#endif

#ifdef HAVE_SYS_UCRED_H
#ifdef HAVE_GRP_H
#include <grp.h>	/* for NGROUPS on Tru64 5.1 */
#endif
#include <sys/ucred.h>
#endif

#include <stdlib.h>

int lutil_getpeereid( int s, uid_t *euid, gid_t *egid
#ifdef LDAP_PF_LOCAL_SENDMSG
	, struct berval *peerbv
#endif
	)
{
#ifdef LDAP_PF_LOCAL
#if defined( HAVE_GETPEERUCRED )
	ucred_t *uc = NULL;
	if( getpeerucred( s, &uc ) == 0 )  {
		*euid = ucred_geteuid( uc );
		*egid = ucred_getegid( uc );
		ucred_free( uc );
		return 0;
	}

#elif defined( SO_PEERCRED )
	struct ucred peercred;
	ber_socklen_t peercredlen = sizeof peercred;

	if(( getsockopt( s, SOL_SOCKET, SO_PEERCRED,
		(void *)&peercred, &peercredlen ) == 0 )
		&& ( peercredlen == sizeof peercred ))
	{
		*euid = peercred.uid;
		*egid = peercred.gid;
		return 0;
	}

#elif defined( LOCAL_PEERCRED )
	struct xucred peercred;
	ber_socklen_t peercredlen = sizeof peercred;

	if(( getsockopt( s, LOCAL_PEERCRED, 1,
		(void *)&peercred, &peercredlen ) == 0 )
		&& ( peercred.cr_version == XUCRED_VERSION ))
	{
		*euid = peercred.cr_uid;
		*egid = peercred.cr_gid;
		return 0;
	}
#elif defined( LDAP_PF_LOCAL_SENDMSG ) && defined( MSG_WAITALL )
	int err, fd;
	struct iovec iov;
	struct msghdr msg = {0};
# ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
# ifndef CMSG_SPACE
# define CMSG_SPACE(len)	(_CMSG_ALIGN(sizeof(struct cmsghdr)) + _CMSG_ALIGN(len))
# endif
# ifndef CMSG_LEN
# define CMSG_LEN(len)		(_CMSG_ALIGN(sizeof(struct cmsghdr)) + (len))
# endif
	struct {
		struct cmsghdr cm;
		int fd;
	} control_st;
	struct cmsghdr *cmsg;
# endif /* HAVE_STRUCT_MSGHDR_MSG_CONTROL */
	struct stat st;
	struct sockaddr_un lname, rname;
	ber_socklen_t llen, rlen;

	rlen = sizeof(rname);
	llen = sizeof(lname);
	memset( &lname, 0, sizeof( lname ));
	getsockname(s, (struct sockaddr *)&lname, &llen);

	iov.iov_base = peerbv->bv_val;
	iov.iov_len = peerbv->bv_len;
	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;
	peerbv->bv_len = 0;

# ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
	msg.msg_control = &control_st;
	msg.msg_controllen = sizeof( struct cmsghdr ) + sizeof( int );	/* no padding! */

	cmsg = CMSG_FIRSTHDR( &msg );
# else
	msg.msg_accrights = (char *)&fd;
	msg.msg_accrightslen = sizeof(fd);
# endif

	/*
	 * AIX returns a bogus file descriptor if recvmsg() is
	 * called with MSG_PEEK (is this a bug?). Hence we need
	 * to receive the Abandon PDU.
	 */
	err = recvmsg( s, &msg, MSG_WAITALL );
	if( err >= 0 &&
# ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
	    cmsg->cmsg_len == CMSG_LEN( sizeof(int) ) &&
	    cmsg->cmsg_level == SOL_SOCKET &&
	    cmsg->cmsg_type == SCM_RIGHTS
# else
		msg.msg_accrightslen == sizeof(int)
# endif /* HAVE_STRUCT_MSGHDR_MSG_CONTROL*/
	) {
		int mode = S_IFIFO|S_ISUID|S_IRWXU;

		/* We must receive a valid descriptor, it must be a pipe,
		 * it must only be accessible by its owner, and it must
		 * have the name of our socket written on it.
		 */
		peerbv->bv_len = err;
# ifdef HAVE_STRUCT_MSGHDR_MSG_CONTROL
		fd = (*(int *)CMSG_DATA( cmsg ));
# endif
		err = fstat( fd, &st );
		if ( err == 0 )
			rlen = read(fd, &rname, rlen);
		close(fd);
		if( err == 0 && st.st_mode == mode &&
			llen == rlen && !memcmp(&lname, &rname, llen))
		{
			*euid = st.st_uid;
			*egid = st.st_gid;
			return 0;
		}
	}
#elif defined(SOCKCREDSIZE)
	struct msghdr msg;
	ber_socklen_t crmsgsize;
	void *crmsg;
	struct cmsghdr *cmp;
	struct sockcred *sc;

	memset(&msg, 0, sizeof msg);
	crmsgsize = CMSG_SPACE(SOCKCREDSIZE(NGROUPS));
	if (crmsgsize == 0) goto sc_err;
	crmsg = malloc(crmsgsize);
	if (crmsg == NULL) goto sc_err;
	memset(crmsg, 0, crmsgsize);
	
	msg.msg_control = crmsg;
	msg.msg_controllen = crmsgsize;
	
	if (recvmsg(s, &msg, 0) < 0) {
		free(crmsg);
		goto sc_err;
	}	

	if (msg.msg_controllen == 0 || (msg.msg_flags & MSG_CTRUNC) != 0) {
		free(crmsg);
		goto sc_err;
	}	
	
	cmp = CMSG_FIRSTHDR(&msg);
	if (cmp->cmsg_level != SOL_SOCKET || cmp->cmsg_type != SCM_CREDS) {
		printf("nocreds\n");
		goto sc_err;
	}	
	
	sc = (struct sockcred *)(void *)CMSG_DATA(cmp);
	
	*euid = sc->sc_euid;
	*egid = sc->sc_egid;

	free(crmsg);
	return 0;

sc_err:	
#endif
#endif /* LDAP_PF_LOCAL */

	return -1;
}

#endif /* HAVE_GETPEEREID */
