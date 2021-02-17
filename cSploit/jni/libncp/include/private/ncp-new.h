#ifndef _LINUX_NCP_NEW_H
#define _LINUX_NCP_NEW_H

#include <ncp/kernel/types.h>

#define NCP_MINOR	222

#define NCP_IOC_NEWCONN	0xDDDD0001	/* ncp_ioc_newconn */
#define NCP_IOC_DELCONN 0xDDDD0002	/* none */
#define NCP_IOC_REQUEST_REPLY	0xDDDD0003	/* ncp_ioc_request_reply */

#define NCP_MAX_RPC_TIMEOUT 100

#define DDPRINTK(X...)
#define DPRINTK(X...)

struct ncp_ioc_newconn {
	int fd;
#define NCP_FLAGS_SIGN_NEGOTIATED	0x0001
#define NCP_FLAGS_SIGN_ACTIVE		0x0002	
	int flags;
	int sequence;
	int connection;
	struct ncp_sign_init sign;
};

struct ncp_ioc_request_reply {
	unsigned int function;
	struct {
		unsigned int size;
		u_int8_t* addr;
	} request;
	struct {
		unsigned int size;
		u_int8_t* addr;
	} reply;
};

#endif
