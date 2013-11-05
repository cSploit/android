#ifndef _PRIVATE_NCP_FS_H
#define _PRIVATE_NCP_FS_H

#include <ncp/kernel/types.h>

struct ncp_fs_info_v2 {
	int version;
	unsigned long mounted_uid;
	unsigned int connection;
	unsigned int buffer_size;

	unsigned int volume_number;
	u_int32_t directory_id;

	u_int32_t dummy1;
	u_int32_t dummy2;
	u_int32_t dummy3;
};

#define NCP_GET_FS_INFO_VERSION_V2 (2)
#define NCP_IOC_GET_FS_INFO_V2		_IOWR('n', 4, struct ncp_fs_info_v2)

#endif
