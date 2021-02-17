/*
 *  ncp_mount.h
 *
 *  Copyright (C) 1995, 1996 by Volker Lendecke
 *  Copyright (C) 1998, 1999 by Petr Vandrovec
 *
 */

#ifndef __NCPMOUNT_H__
#define __NCPMOUNT_H__

#include <ncp/ncp.h>

#define NCP_MOUNT_VERSION_V2	(2)

/* Values for flags */
#define NCP_MOUNT2_SOFT		0x0001
#define NCP_MOUNT2_INTR		0x0002
#define NCP_MOUNT2_STRONG	0x0004
#define NCP_MOUNT2_NO_OS2	0x0008
#define NCP_MOUNT2_NO_NFS	0x0010

#define PATH_MAX_V20	1024	/* PATH_MAX for 2.0 kernel */

struct ncp_mount_data_v2 {
	int version;
	unsigned int ncp_fd;	/* The socket to the ncp port */
	unsigned int wdog_fd;	/* Watchdog packets come here */
	unsigned int message_fd; /* Message notifications come here */
        __ker20_uid_t mounted_uid;      /* Who may umount() this filesystem? */

	struct sockaddr_ipx serv_addr;
	unsigned char server_name[NCP_BINDERY_NAME_LEN];

	unsigned char mount_point[PATH_MAX_V20+1];
	unsigned char mounted_vol[NCP_VOLNAME_LEN+1];

	unsigned int time_out;	/* How long should I wait after
				   sending a NCP request? */
	unsigned int retry_count; /* And how often should I retry? */
	unsigned int flags;

        __ker20_uid_t uid;
        __ker20_gid_t gid;
        __ker20_mode_t file_mode;
        __ker20_mode_t dir_mode;
};

#define NCP_MOUNT_VERSION_V3	(3)

/* Values for flags */
#define NCP_MOUNT3_SOFT		0x0001
#define NCP_MOUNT3_INTR		0x0002
#define NCP_MOUNT3_STRONG	0x0004
#define NCP_MOUNT3_NO_OS2	0x0008
#define NCP_MOUNT3_NO_NFS	0x0010
#define NCP_MOUNT3_EXTRAS	0x0020
#define NCP_MOUNT3_SYMLINKS	0x0040
#define NCP_MOUNT3_NFS_EXTRAS	0x0080

struct ncp_mount_data_v3 {
	int version;
	unsigned int ncp_fd;	/* The socket to the ncp port */
	__ker21_uid_t mounted_uid;	/* Who may umount() this filesystem? */
	__ker21_pid_t wdog_pid;		/* Who cares for our watchdog packets? */

	unsigned char mounted_vol[NCP_VOLNAME_LEN + 1];
	unsigned int time_out;	/* How long should I wait after
				   sending a NCP request? */
	unsigned int retry_count;	/* And how often should I retry? */
	unsigned int flags;

	__ker21_uid_t uid;
	__ker21_gid_t gid;
	__ker21_mode_t file_mode;
	__ker21_mode_t dir_mode;
};

#define NCP_MOUNT_VERSION_V4	(4)

struct ncp_mount_data_v4 {
	int version;
	unsigned long flags;	/* NCP_MOUNT_* flags */
	/* MIPS uses long __kernel_uid_t, but... */
	/* we neever pass -1, so it is safe */
	unsigned long mounted_uid;	/* Who may umount() this filesystem? */
	/* MIPS uses long __kernel_pid_t */
	long wdog_pid;		/* Who cares for our watchdog packets? */

	unsigned int ncp_fd;	/* The socket to the ncp port */
	unsigned int time_out;	/* How long should I wait after
				   sending a NCP request? */
	unsigned int retry_count;	/* And how often should I retry? */

	/* MIPS uses long __kernel_uid_t... */
	/* we never pass -1, so it is safe */
	unsigned long uid;
	unsigned long gid;
	/* MIPS uses unsigned long __kernel_mode_t */
	unsigned long file_mode;
	unsigned long dir_mode;
};

#define NCP_MOUNT_VERSION_V5	(5)

#endif /* __NCPMOUNT_H__ */
