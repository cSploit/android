/* 
    resolve.c - Resolve names to addresses
    Copyright (C) 2002  Petr Vandrovec

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

	1.00  2002, May 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.
 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <ctype.h>
#include <sys/time.h>
#include <sys/wait.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncplib_i.h"
#include "ncpcode.h"
#include "ncpi.h"

static NWCCODE NWCCOpenConnByNameTran(NWCONN_HANDLE startConn, const char *serverName,
		nuint nameFormat, nuint openState, nuint reserved, 
		nuint transport, NWCONN_HANDLE* pconn);

struct resolver {
	const char* name;
	NWCCODE (*start)(void **rh, NWCONN_HANDLE conn,
			 const char *name, nuint nameFormat, nuint transport);
	void (*stop)(void *rh);
	NWCCODE (*get)(void *rh, union ncp_sockaddr *addr, 
		       enum NET_ADDRESS_TYPE *addrType);
};

static int safe_read(int fd, void* ptr, size_t ln) {
	unsigned char* d = ptr;
	
	while (ln != 0) {
		int l;
		
		l = read(fd, d, ln);
		if (l == 0) {
			return ln;
		}
		if (l < 0) {
			if (errno == EINTR || errno == EAGAIN) {
				continue;
			}
			return -1;
		}
		if ((size_t)l > ln) {
			errno = EIO;
			return -1;
		}
		ln -= l;
	}
	return 0;
}

static void memcpy_toupper(char* dst, const char* src, size_t ln) {
	while (ln--) {
		*dst++ = toupper(*src++);
	}
}

/* -------- */

#ifdef CONFIG_NATIVE_IPX
/* Bindery resolver can return only IPX addresses, so do not
   use it in non-ipx environment at all */

struct bind_rh {
	NWCONN_HANDLE startConn;
	NWObjectType objType;
	char objName[NW_MAX_SERVER_NAME_LEN];
	NWObjectID lastObj;
	unsigned int scan:1;
	unsigned int done:1;
};

static NWCCODE bind_start(void **prh, NWCONN_HANDLE startConn,
			  const char *name, nuint nameFormat, nuint transport) {
	struct bind_rh* rh;
	NWCCODE err;

	switch (transport) {
		case NWCC_TRAN_TYPE_WILD:
		case NWCC_TRAN_TYPE_IPX_old:
		case NT_IPX:
			break;
		default:
			return NWE_UNSUPPORTED_TRAN_TYPE;
	}
	if (!startConn) {
		err = ncp_next_conn(NULL, &startConn);
		if (err) {
			if (name != NULL || nameFormat != NWCC_NAME_FORMAT_BIND || transport != NT_IPX) {
				err = NWCCOpenConnByNameTran(NULL, NULL, NWCC_NAME_FORMAT_BIND,
					NWCC_OPEN_NEW_CONN, NWCC_RESERVED, NT_IPX, &startConn);
			}
			if (err) {
				return err;
			}
		}
	} else {
		ncp_conn_use(startConn);
	}
	rh = malloc(sizeof(*rh));
	if (!rh) {
		NWCCCloseConn(startConn);
		return ENOMEM;
	}
	rh->startConn = startConn;
	rh->lastObj = 0xFFFFFFFF;
	rh->scan = 0;
	rh->done = 0;
	if (nameFormat == NWCC_NAME_FORMAT_BIND) {
		size_t ln;
		
		if (!name) {
			err = ERR_NULL_POINTER;
			goto qRH;
		}
		ln = strnlen(name, NW_MAX_SERVER_NAME_LEN);
		if (ln >= NW_MAX_SERVER_NAME_LEN) {
			err = ENAMETOOLONG;
			goto qRH;
		}
		memcpy_toupper(rh->objName, name, ln + 1);
		rh->objType = OT_FILE_SERVER;
		*prh = rh;
		return 0;
	} else if (nameFormat == NWCC_NAME_FORMAT_NDS_TREE) {
		rh->objType = OT_TREE_NAME;
		if (name) {
			size_t ln;
			
			ln = strnlen(name, NW_MAX_SERVER_NAME_LEN);
			if (ln >= NW_MAX_SERVER_NAME_LEN) {
				err = ENAMETOOLONG;
				goto qRH;
			}
			memcpy_toupper(rh->objName, name, ln + 1);
			if (ln <= 32) {
				if (ln < 32) {
					memset(rh->objName + ln, '_', 32 - ln);
				}
				strcpy(rh->objName + 32, "*");
				rh->scan = 1;
			}
		} else {
			strcpy(rh->objName, "*");
		}
		*prh = rh;
		return 0;
	} else {
		err = NWE_UNSUPPORTED_NAME_FORMAT_TYP;
	}
qRH:;
	free(rh);
	NWCCCloseConn(startConn);
	return err;
}

static void bind_stop(void *arh) {
	struct bind_rh *rh = arh;

	if (rh) {
		if (rh->startConn) {
			NWCCCloseConn(rh->startConn);
			rh->startConn = NULL;
		}
		free(rh);
	}
}

static NWCCODE bind_get(void *arh, union ncp_sockaddr *addr,
		enum NET_ADDRESS_TYPE *addrType) {
	struct bind_rh *rh = arh;
	NWCCODE err;
	union {
		unsigned char data[128];
		struct prop_net_address addr;
			} prop;

	if (rh->done) {
		return NWE_SCAN_COMPLETE;
	}
	if (rh->scan) {
		char name[NCP_BINDERY_NAME_LEN + 1];

		while ((err = NWScanObject(rh->startConn, rh->objName, rh->objType,
				&rh->lastObj, name, NULL, NULL, NULL, NULL)) == 0) {
			err = NWReadPropertyValue(rh->startConn, name, rh->objType,
				"NET_ADDRESS", 1, prop.data, NULL, NULL);
			if (!err) {
				addr->ipx.sipx_family = AF_IPX;
				addr->ipx.sipx_network = prop.addr.network;
				addr->ipx.sipx_port = prop.addr.port;
				addr->ipx.sipx_type = 0x11;	/* NCP/IPX */
				ipx_assign_node(addr->ipx.sipx_node, prop.addr.node);
				*addrType = NT_IPX;
				return 0;
			}
		}
	} else {
		rh->done = 1;
		err = NWReadPropertyValue(rh->startConn, rh->objName, rh->objType,
			"NET_ADDRESS", 1, prop.data, NULL, NULL);
		if (!err) {
			addr->ipx.sipx_family = AF_IPX;
			addr->ipx.sipx_network = prop.addr.network;
			addr->ipx.sipx_port = prop.addr.port;
			addr->ipx.sipx_type = 0x11;	/* NCP/IPX */
			ipx_assign_node(addr->ipx.sipx_node, prop.addr.node);
			*addrType = NT_IPX;
		}
	}
	rh->done = 1;
	NWCCCloseConn(rh->startConn);
	rh->startConn = NULL;
	return err;
}

static struct resolver bind_resolver = {
	"BIND",
	bind_start,
	bind_stop,
	bind_get
};

#endif	/* CONFIG_NATIVE_IPX */

/* -------- */

#ifdef CONFIG_NATIVE_IP
/* DNS resolver can currently return only IPv4 addresses (because of
   IPv6 NCP stack is unavailable to public)... */

struct dns_rh {
	size_t pos;
	size_t size;
	nuint transport;
	unsigned int state:1;
	u_int16_t port;
	u_int32_t addresses[0];
};

static NWCCODE dns_start(void **prh, UNUSED(NWCONN_HANDLE startConn),
			 const char *name, nuint nameFormat, nuint transport) {
	struct dns_rh* rh;
	struct hostent* he;
	u_int32_t** addresses;
	u_int32_t** ptr;
	u_int32_t* dst;
	size_t addrcnt;
	const char* portid;
	int port;

	switch (transport) {
		case NWCC_TRAN_TYPE_WILD:
		case NT_UDP:
		case NT_TCP:
			break;
		default:
			return NWE_UNSUPPORTED_TRAN_TYPE;
	}
	if (nameFormat != NWCC_NAME_FORMAT_BIND) {
		return NWE_UNSUPPORTED_NAME_FORMAT_TYP;
	}
	if (!name) {
		return ERR_NULL_POINTER;
	}
	portid = strrchr(name, ':');
	if (portid) {
		size_t nln;
		char* n;

		port = strtoul(portid + 1, NULL, 10);
		if (!port) {
			return NWE_PARAM_INVALID;
		}
		nln = portid - name;
		n = malloc(nln + 1);
		if (!n) {
			return ENOMEM;
		}
		memcpy(n, name, nln);
		n[nln] = 0;
		he = gethostbyname(n);
		free(n);
	} else {
		port = 524;
		he = gethostbyname(name);
	}
	if (!he) {
		return NWE_SERVER_UNKNOWN;
	}
	if (he->h_addrtype != AF_INET || he->h_length != 4) {
		return NWE_SERVER_UNKNOWN;
	}
	addresses = (u_int32_t**)he->h_addr_list;
	for (ptr = addresses; *ptr; ptr++) ;
	addrcnt = ptr - addresses;
	rh = malloc(sizeof(*rh) + sizeof(u_int32_t) * addrcnt);
	if (!rh) {
		return ENOMEM;
	}
	rh->pos = 0;
	rh->size = addrcnt;
	rh->transport = transport;
	rh->state = 0;
	rh->port = htons(port);
	for (ptr = addresses, dst = rh->addresses; *ptr; ) {
		*dst++ = **ptr++;
	}
	*prh = rh;
	return 0;
}

static void dns_stop(void *arh) {
	struct dns_rh *rh = arh;

	if (rh) {
		free(rh);
	}
}

static NWCCODE dns_get(void *arh, union ncp_sockaddr *addr,
		enum NET_ADDRESS_TYPE *addrType) {
	struct dns_rh *rh = arh;

	if (rh->pos >= rh->size) {
		return NWE_SCAN_COMPLETE;
	}
	addr->inet.sin_family = AF_INET;
	addr->inet.sin_addr.s_addr = rh->addresses[rh->pos];
	addr->inet.sin_port = rh->port;
	if (rh->transport != NWCC_TRAN_TYPE_WILD) {
		rh->pos++;
		*addrType = rh->transport;
		return 0;
	}
	if (rh->state) {
		rh->state = 0;
		rh->pos++;
		*addrType = NT_UDP;
		return 0;
	}
	rh->state = 1;
	*addrType = NT_TCP;
	return 0;
}

static struct resolver dns_resolver = {
	"DNS",
	dns_start,
	dns_stop,
	dns_get
};

#endif	/* CONFIG_NATIVE_IP */

/* -------- */

#ifdef CONFIG_NATIVE_IPX
/* SAP resolver can return only IPX addresses, so do not
   use it in non-ipx environment at all */

struct sap_rh {
	int fd;
	pid_t pid;
	unsigned int done:1;
};

static void sap_stop(void *arh) {
	struct sap_rh *rh = arh;

	if (rh) {
		close(rh->fd);
		rh->fd = -1;
		/* send TERM to the resolver... */
		kill(rh->pid, SIGTERM);
		waitpid(rh->pid, NULL, 0);
		free(rh);
	}
}

static void sap_report(int fd, const struct sap_server_ident *ident) {
	unsigned char answer[4 + 4 + 4 + (4 + 6 + 2)];
	
	DSET_HL(answer,  0, 0);
	DSET_HL(answer,  4, NT_IPX);
	DSET_HL(answer,  8, 12);
	memcpy(answer + 12, &ident->server_network, 4);
	memcpy(answer + 16, ident->server_node, 6);
	/* NDS reports some internal socket instead of NCP socket back */
	if (WVAL_HL(ident, 0) == OT_TREE_NAME) {
		WSET_HL(answer, 22, 0x0451);
	} else {
		memcpy(answer + 22, &ident->server_port, 2);
	}
	/* Immediately abort if write fails */
	if (write(fd, answer, sizeof(answer)) != sizeof(answer)) {
		exit(1);
	}
}

static int sap_name_cmp(const unsigned char *my, const unsigned char *net, size_t cmplen) {
	if (memcmp(my, net, cmplen)) {
		size_t ln;

		/* If it is exact match, allow trailing spaces and garbage after
		   terminating zero. */
		ln = strnlen(my, cmplen);
		if (memcmp(my, net, ln)) {
			return 1;
		}
		while (ln < cmplen && net[ln] != 0) {
			if (net[ln] != ' ') {
				return 1;
			}
			ln++;
		}
	}
	return 0;
}

static void sap_process(int fd, unsigned char *sap_request, size_t cmplen) {
	struct sockaddr_ipx addr;
	char rx_data[1024];
	int sock;
	size_t sendlen;
	unsigned int expectedType;
	static int one = 1;
	u_int32_t u32;
	size_t len;
	struct timeval tv;
	int retry;
	int found = 0;
	
	struct sap_server_ident *ident;

	if ((sock = socket(PF_IPX, SOCK_DGRAM, IPXPROTO_IPX)) < 0) {
		u32 = htonl(errno);
		goto quit;
	}
	/* Permit broadcast output */
	if (setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &one, sizeof(one)) == -1) {
		u32 = htonl(errno);
		goto quitSock;
	}
	memset(&addr, 0, sizeof(addr));
	addr.sipx_family = AF_IPX;
	addr.sipx_type = IPX_SAP_PTYPE;

	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) == -1) {
		u32 = htonl(errno);
		goto quitSock;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sipx_family = AF_IPX;
	addr.sipx_port = htons(IPX_SAP_PORT);
	addr.sipx_type = IPX_SAP_PTYPE;
	addr.sipx_network = htonl(0x0);
	ipx_assign_node(addr.sipx_node, IPX_BROADCAST_NODE);

	ident = (struct sap_server_ident *) (rx_data + 2);
	
	if (cmplen == 0) {
		sendlen = 4;
		expectedType = IPX_SAP_NEAREST_RESPONSE;
		WSET_HL(sap_request, 0, IPX_SAP_NEAREST_QUERY);
	} else {
		sendlen = 60;
		expectedType = IPX_SAP_SPECIFIC_RESPONSE;
		WSET_HL(sap_request, 0, IPX_SAP_SPECIFIC_QUERY);
	}
	for (retry = 5; retry; retry--) {
		unsigned int sendtime;

		if (gettimeofday(&tv, NULL))
			goto quitSock;
		sendtime = tv.tv_sec * 1000 + tv.tv_usec / 1000 + 100;	/* 100ms timeout */

		if ((size_t)sendto(sock, sap_request, sendlen, 0,
			   (struct sockaddr *) &addr, sizeof(addr)) != sendlen) {
			goto quitSock;
		}

		while (1) {
			long err;
			int recvtime;

			if (gettimeofday(&tv, NULL))
				goto quitSock;

			recvtime = sendtime - tv.tv_sec * 1000 - tv.tv_usec / 1000;

			if (recvtime < 0)
				break;

			err = x_recv(sock, rx_data, sizeof(rx_data), 0, recvtime, &len);
			if (err) {
				if (err == ETIMEDOUT)
					break;
				continue;
			}
			if (len < 66)
				continue;
			if (WVAL_HL(rx_data, 0) != expectedType)
				continue;
			if (memcmp(sap_request + 2, rx_data + 2, 2))
				continue;
			if (sap_name_cmp(sap_request + 4, ident->server_name, cmplen))
				continue;
			found = 1;
			sap_report(fd, ident);
		}
		if (found) {
			break;
		}
	}
	if (!found) {
		/* Specific/nearest query failed, try general one ... */
		WSET_HL(sap_request, 0, IPX_SAP_GENERAL_QUERY);

		for (retry = 5; retry; retry--) {
			unsigned int sendtime;

			if (gettimeofday(&tv, NULL))
				goto quitSock;
			sendtime = tv.tv_sec * 1000 + tv.tv_usec / 1000 + 100;

			if (sendto(sock, sap_request, 4, 0,
				   (struct sockaddr *) &addr, sizeof(addr)) != 4) {
				goto quitSock;
			}

			while (1) {
				long err;
				int recvtime;
				size_t offs;

				if (gettimeofday(&tv, NULL))
					goto quitSock;

				recvtime = sendtime - tv.tv_sec * 1000 - tv.tv_usec / 1000;

				if (recvtime < 0)
					break;

				err = x_recv(sock, rx_data, sizeof(rx_data), 0, recvtime, &len);
				if (err) {
					if (err == ETIMEDOUT)
						break;
					continue;
				}
				if (len < 66)
					continue;
				if (WVAL_HL(rx_data, 0) != IPX_SAP_GENERAL_RESPONSE)
					continue;
				found = 1;
				for (offs = 2; offs + 64 <= len; offs += 64) {
					ident = (struct sap_server_ident *)(rx_data + offs);
					if (memcmp(sap_request + 2, &ident->server_type, 2))
						continue;
					if (sap_name_cmp(sap_request + 4, ident->server_name, cmplen))
						continue;
					sap_report(fd, ident);
				}
			}
			if (found) {
				break;
			}
		}
	}
	close(sock);
	return;
quitSock:
	close(sock);
quit:
	write(fd, &u32, sizeof(u32));
}

static NWCCODE sap_start(void **prh, UNUSED(NWCONN_HANDLE startConn),
			 const char *name, nuint nameFormat, nuint transport) {
	struct sap_rh* rh;
	unsigned char sap_request[60];
	size_t cmplen;
	int fds[2];
	int ec;

	switch (transport) {
		case NWCC_TRAN_TYPE_WILD:
		case NWCC_TRAN_TYPE_IPX_old:
		case NT_IPX:
			break;
		default:
			return NWE_UNSUPPORTED_TRAN_TYPE;
	}
	memset(sap_request + 52, 0xFF, 8);
	if (nameFormat == NWCC_NAME_FORMAT_BIND) {
		WSET_HL(sap_request, 2, OT_FILE_SERVER);
		if (!name) {
			cmplen = 0;
		} else {
			size_t ln;
	
			ln = strnlen(name, NW_MAX_SERVER_NAME_LEN);
			if (ln >= NW_MAX_SERVER_NAME_LEN) {
				return ENAMETOOLONG;
			}
			memcpy_toupper(sap_request + 4, name, ln);
			if (ln < 48) {
				memset(sap_request + 4 + ln, 0, 48 - ln);
			}
			cmplen = 48;
		}
	} else if (nameFormat == NWCC_NAME_FORMAT_NDS_TREE) {
		WSET_HL(sap_request, 2, OT_TREE_NAME);
		if (name) {
			size_t ln;
			
			ln = strnlen(name, NW_MAX_SERVER_NAME_LEN);
			if (ln >= NW_MAX_SERVER_NAME_LEN) {
				return ENAMETOOLONG;
			}
			memcpy_toupper(sap_request + 4, name, ln);
			if (ln <= 32) {
				if (ln < 32) {
					memset(sap_request + 4 + ln, '_', 32 - ln);
				}
				sap_request[4 + 32] = '*';
				memset(sap_request + 4 + 33, 0, 48 - 33);
				cmplen = 32;
			} else {
				if (ln < 48) {
					memset(sap_request + 4 + ln, 0, 48 - ln);
				}
				cmplen = 48;
			}
		} else {
			cmplen = 0;
		}
	} else {
		return NWE_UNSUPPORTED_NAME_FORMAT_TYP;
	}
	rh = malloc(sizeof(*rh));
	if (!rh) {
		return ENOMEM;
	}
	rh->done = 0;
	if (pipe(fds)) {
		free(rh);
		return errno;
	}
	rh->pid = fork();
	if (rh->pid == (pid_t)-1) {
		close(fds[0]);
		close(fds[1]);
		free(rh);
		return errno;
	}
	if (!rh->pid) {
		int i;
		sigset_t newsigset;
		sigset_t oldsigset;
		
		for (i = 3; i < 1024; i++) {
			if (i != fds[1]) {
				close(i);
			}
		}

		sigemptyset(&newsigset);
		sigaddset(&newsigset, SIGTERM);
		sigaddset(&newsigset, SIGPIPE);
		sigprocmask(SIG_UNBLOCK, &newsigset, &oldsigset);
		signal(SIGTERM, SIG_DFL);
		signal(SIGPIPE, SIG_DFL);

		write(fds[1], "R", 1);

		sap_process(fds[1], sap_request, cmplen);
		exit(0);		
	}
	rh->fd = fds[0];
	close(fds[1]);
	ec = safe_read(fds[0], sap_request, 1);
	if (ec != 0 || sap_request[0] != 'R') {
		sap_stop(rh);
		return EIO;
	}
	*prh = rh;
	return 0;
}

static NWCCODE sap_get(void *arh, union ncp_sockaddr *addr,
		enum NET_ADDRESS_TYPE *addrType) {
	struct sap_rh *rh = arh;
	NWCCODE err;
	unsigned char data[64];
	int e;
	u_int32_t len;
	enum NET_ADDRESS_TYPE at;

nextOne:;
	if (rh->done) {
		return NWE_SCAN_COMPLETE;
	}
	e = safe_read(rh->fd, data, 4);
	if (e == -1) {
		rh->done = 1;
		return errno;
	}
	if (e) {
		rh->done = 1;
		return NWE_SCAN_COMPLETE;
	}
	err = DVAL_HL(data, 0);
	if (err) {
		return err;
	}
	e = safe_read(rh->fd, data, 8);
	if (e == -1) {
		rh->done = 1;
		return errno;
	}
	if (e) {
		rh->done = 1;
		return NWE_SCAN_COMPLETE;
	}
	at = DVAL_HL(data, 0);
	len = DVAL_HL(data, 4);
	if (len > sizeof(data)) {
		rh->done = 1;
		return NWE_BUFFER_OVERFLOW;
	}
	e = safe_read(rh->fd, data, len);
	if (e == -1) {
		rh->done = 1;
		return errno;
	}
	if (e) {
		rh->done = 1;
		return NWE_SCAN_COMPLETE;
	}
	if (at == NT_IPX) {
		addr->ipx.sipx_family = AF_IPX;
		memcpy(&addr->ipx.sipx_network, data, 4);
		memcpy(&addr->ipx.sipx_port, data + 10, 2);
		addr->ipx.sipx_type = 0x11;	/* NCP/IPX */
		ipx_assign_node(addr->ipx.sipx_node, data + 4);
	} else {
		goto nextOne;
	}
	*addrType = at;
	return 0;
}

static struct resolver sap_resolver = {
	"SAP",
	sap_start,
	sap_stop,
	sap_get
};

#endif	/* CONFIG_NATIVE_IPX */

/* -------- */

static struct resolver* resolvers[] = {
#ifdef CONFIG_NATIVE_IPX
	&bind_resolver,
#endif
#ifdef CONFIG_NATIVE_IP
	&dns_resolver,
#endif
#ifdef CONFIG_NATIVE_IPX
	&sap_resolver,
#endif
	NULL
};

static NWCCODE NWCCOpenConnByNameTran(NWCONN_HANDLE startConn, const char *serverName,
		nuint nameFormat, nuint openState, nuint reserved, 
		nuint transport, NWCONN_HANDLE* pconn) {
	NWCCODE err;
	struct resolver** res;
	
	if (reserved != NWCC_RESERVED)
		return NWE_PARAM_INVALID;
	for (res = resolvers; *res; res++) {
		struct resolver *r = *res;
		void* rdata;

		err = r->start(&rdata, startConn, serverName, nameFormat, transport);
		if (!err) {
			while (1) {
				union ncp_sockaddr server_addr;
				enum NET_ADDRESS_TYPE at;

				err = r->get(rdata, &server_addr, &at);
				if (err == NWE_SCAN_COMPLETE) {
					break;
				}
				if (err) {
					continue;
				}
				err = NWCCOpenConnBySockAddr(&server_addr.any, at, openState, reserved, pconn);
				if (!err) {
					r->stop(rdata);
					return 0;
				}
			}
			r->stop(rdata);
		}
	}
	return NWE_SERVER_NOT_FOUND;
}

NWCCODE NWCCOpenConnByName(NWCONN_HANDLE startConn, const char *serverName,
		nuint nameFormat, nuint openState,
		nuint reserved, NWCONN_HANDLE* pconn) {
	return NWCCOpenConnByNameTran(startConn, serverName, nameFormat, openState, reserved, NWCC_TRAN_TYPE_WILD,
		pconn);
}

NWCCODE ncp_find_server_addr(const char **serverName, int object_type, struct sockaddr* addr, socklen_t len, nuint transport) {
	NWCCODE err;
	NWCONN_HANDLE conn;

	if (!serverName) {
		return ERR_NULL_POINTER;
	}
	if (object_type == OT_TREE_NAME) {
		object_type = NWCC_NAME_FORMAT_NDS_TREE;
	} else {
		object_type = NWCC_NAME_FORMAT_BIND;
	}
	err = NWCCOpenConnByNameTran(NULL, *serverName, object_type, NWCC_OPEN_NEW_CONN,
		NWCC_RESERVED, transport, &conn);
	if (!err) {
		NWCCTranAddr ta;
		unsigned char buffer[24];
		static char sname[NCP_BINDERY_NAME_LEN + 1];
		
		ta.len = sizeof(buffer);
		ta.buffer = buffer;
		
		err = NWCCGetConnInfo(conn, NWCC_INFO_TRAN_ADDR, sizeof(ta), &ta);
		if (!err) {
			if (ta.type == NT_IPX) {
				struct sockaddr_ipx* ipx = (struct sockaddr_ipx*)addr;
			
				if (len < sizeof(*ipx)) {
					err = NWE_BUFFER_OVERFLOW;
				} else {
					ipx->sipx_family = AF_IPX;
					memcpy(ipx->sipx_node, buffer + 4, 6);
					memcpy(&ipx->sipx_network, buffer, 4);
					memcpy(&ipx->sipx_port, buffer + 10, 2);
					ipx->sipx_type = 0x11;
				}
			} else if (ta.type == NT_UDP || ta.type == NT_TCP) {
				struct sockaddr_in* in = (struct sockaddr_in*)addr;

				if (len < sizeof(*in)) {
					err = NWE_BUFFER_OVERFLOW;
				} else {
					in->sin_family = AF_INET;
					memcpy(&in->sin_addr.s_addr, buffer + 2, 4);
					memcpy(&in->sin_port, buffer, 2);
				}
			} else {
				err = EINVAL;
			}
			if (!err) {
				err = NWGetFileServerName(conn, sname);
				if (!err) {
					*serverName = sname;
				}
			}
		}
		NWCCCloseConn(conn);
	}
	return err;
}

long ncp_find_server(const char **serverName, int object_type, struct sockaddr* addr, socklen_t len) {
	return ncp_find_server_addr(serverName, object_type, addr, len, NT_IPX);
}
