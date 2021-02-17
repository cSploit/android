/*
    ncp_fs.h
    Copyright (C) 1995, 1996 by Volker Lendecke
    Copyright (C) 1998, 1999  Petr Vandrovec

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


    Revision history:

	0.00  1995			Volker Lendecke
		Initial revision.

	0.01  1998			Petr Vandrovec <vandrove@vc.cvut.cz>
		New ioctl: LOCKUNLOCK, GETROOT, SETROOT, GETOBJECTNAME,
			SETOBJECTNAME, GETPRIVATEDATA, SETPRIVATEDATA

	0.02  1998			Wolfram Pienkoss <wp@bszh.de>
		New ioctl: GETCHARSETS, SETCHARSETS

	0.03  1999, September		Petr Vandrovec <vandrove@vc.cvut.cz>
		New ioctl: GETDENTRYTTL, SETDENTRYTTL

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
 */

/* TODO: These are all internal to ncpfs... Do something with them :-) */

#ifndef _KERNEL_NCP_FS_H
#define _KERNEL_NCP_FS_H

#include <ncp/kernel/fs.h>
#include <ncp/kernel/types.h>
#include <netinet/in.h>
#include <ncp/kernel/ipx.h>

struct ncp_sign_init
{
	char sign_root[8];
	char sign_last[16];
};

union ncp_sockaddr {
	struct sockaddr any;
#ifdef NCP_IPX_SUPPORT
	struct sockaddr_ipx ipx;
#endif
#ifdef NCP_IN_SUPPORT
	struct sockaddr_in inet;
#endif
};

#ifdef NCP_KERNEL_NCPFS_AVAILABLE
/*
 * ioctl commands
 */

struct ncp_ioctl_request {
	unsigned int function;
	unsigned int size;
	char *data;
};

struct ncp_fs_info {
	int version;
	union ncp_sockaddr addr;
	__kerXX_uid_t mounted_uid;
	int connection;		/* Connection number the server assigned us */
	int buffer_size;	/* The negotiated buffer size, to be
				   used for read/write requests! */

	int volume_number;
	u_int32_t directory_id;
};

struct ncp_lock_ioctl
{
#define NCP_LOCK_LOG	0
#define NCP_LOCK_SH	1
#define NCP_LOCK_EX	2
#define NCP_LOCK_CLEAR	256
	int		cmd;
	int		origin;
	unsigned int	offset;
	unsigned int	length;
#define NCP_LOCK_DEFAULT_TIMEOUT	18
#define NCP_LOCK_MAX_TIMEOUT		180
	int		timeout;
};

struct ncp_setroot_ioctl
{
	int		volNumber;
	int		name_space;
	u_int32_t	dirEntNum;
};

struct ncp_objectname_ioctl
{
#define NCP_AUTH_NONE	0x00
#define NCP_AUTH_BIND	0x31
#define NCP_AUTH_NDS	0x32
	int		auth_type;
	size_t		object_name_len;
	void*		object_name;
};

struct ncp_privatedata_ioctl
{
	size_t		len;
	void*		data;		/* ~1000 for NDS */
};

/* NLS charsets by ioctl */
#define NCP_IOCSNAME_LEN 20
struct ncp_nls_ioctl_old
{
	int codepage;
	unsigned char iocharset[NCP_IOCSNAME_LEN+1];
};

struct ncp_nls_ioctl
{
	unsigned char codepage[NCP_IOCSNAME_LEN+1];
	unsigned char iocharset[NCP_IOCSNAME_LEN+1];
};

#define	NCP_IOC_NCPREQUEST		_IOR('n', 1, struct ncp_ioctl_request)
#define	NCP_IOC_GETMOUNTUID		_IOW('n', 2, __kernel_uid_t)
#define NCP_IOC_GETMOUNTUID2		_IOW('n', 2, unsigned long)

#define NCP_IOC_CONN_LOGGED_IN          _IO('n', 3)

#define NCP_GET_FS_INFO_VERSION    (1)
#define NCP_IOC_GET_FS_INFO             _IOWR('n', 4, struct ncp_fs_info)

#define NCP_IOC_SIGN_INIT		_IOR('n', 5, struct ncp_sign_init)
#define NCP_IOC_SIGN_WANTED		_IOR('n', 6, int)
#define NCP_IOC_SET_SIGN_WANTED		_IOW('n', 6, int)

#define NCP_IOC_LOCKUNLOCK		_IOR('n', 7, struct ncp_lock_ioctl)

#define NCP_IOC_GETROOT			_IOW('n', 8, struct ncp_setroot_ioctl)
#define NCP_IOC_SETROOT			_IOR('n', 8, struct ncp_setroot_ioctl)

#define NCP_IOC_GETOBJECTNAME		_IOWR('n', 9, struct ncp_objectname_ioctl)
#define NCP_IOC_SETOBJECTNAME		_IOR('n', 9, struct ncp_objectname_ioctl)
#define NCP_IOC_GETPRIVATEDATA		_IOWR('n', 10, struct ncp_privatedata_ioctl)
#define NCP_IOC_SETPRIVATEDATA		_IOR('n', 10, struct ncp_privatedata_ioctl)

#define NCP_IOC_GETCHARSETS		_IOWR('n', 11, struct ncp_nls_ioctl)
#define NCP_IOC_SETCHARSETS		_IOR('n', 11, struct ncp_nls_ioctl)
/* never released to Linus, will wanish shortly */
#define NCP_IOC_GETCHARSETS_OLD		_IOWR('n', 11, struct ncp_nls_ioctl_old)
#define NCP_IOC_SETCHARSETS_OLD		_IOR('n', 11, struct ncp_nls_ioctl_old)

#define NCP_IOC_GETDENTRYTTL		_IOW('n', 12, u_int32_t)
#define NCP_IOC_SETDENTRYTTL		_IOR('n', 12, u_int32_t)

#else

struct ncp_fs_info {
	int version;
	union ncp_sockaddr addr;
	uid_t mounted_uid;
	int connection;		/* Connection number the server assigned us */
	size_t buffer_size;	/* The negotiated buffer size, to be
				   used for read/write requests! */

	int volume_number;
	u_int32_t directory_id;
};

#endif	/* NCP_KERNEL_NCPFS_AVAILABLE */

/*
 * The packet size to allocate. One page should be enough.
 */
#define NCP_PACKET_SIZE 65536

#define NCP_MAXPATHLEN 255
#define NCP_MAXNAMELEN 14

#endif				/* _LINUX_NCP_FS_H */

