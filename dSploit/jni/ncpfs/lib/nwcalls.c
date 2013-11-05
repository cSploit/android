/* 
    nwcalls.c - Connection oriented calls
    Copyright (C) 1999, 2000  Petr Vandrovec

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

	0.00  1998, December 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added copyright and license.

	1.01  2000, January 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changes for 32bit uids.
		
	1.02  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetFileServerUTCTime.

	1.03  2000, May 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		NWGetFileServerUTCTime moved to nwtime.c.

	1.04  2000, May 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Make NWParsePath return ERR_NULL_POINTER if path==NULL.

	1.05  2000, June 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetBinderyAccessLevel.

        1.06  2001, January 7		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Make NWCCConnGetInfo ERR_NULL_POINTER if buffer==NULL.
		Added answer to NWCC_INFO_TREE_NAME in NWCCConnGetInfo.
		Added NWCCOpenConnByName (only IPX transport supported).

        1.07  2001, January 8		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added disk retrictions API calls.

        1.08  2001, January 9		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added semaphores API calls.

        1.09  2001, January 10		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added NWCCGetConnAddress,NWCCGetConnAddressLength,
		Fixed NWGetVolumeNumber prototype in nwcalls.h and added it here.

	1.10  2001, February 11		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWScanSemaphoresByConn.

	1.11  2001, February 25		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added __NWExecuteGlobal code, implemented NW{Set,Get}BroadcastsMode for
		permanent connections.

	1.12  2001, February 25		Patrick Pollet <Patrick.Pollet@cipcinsa.insa-lyon.fr>
		Added NWIsObjectInSet.

	1.13  2001, March 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed meaning of iterHandle parameter in
		NWScanVolDiskRestrictions/NWScanVolDiskRestrictions2.

	1.14  2001, May 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Include strings.h, do not use IPX in IPX-less config, hack
		around missing MSG_DONTWAIT.

	1.15  2001, June 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added missing #include <stddef.h>. Discovered by Daniel Greenberg <daneli@umich.edu>

	1.16  2001, December 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Forget about NWCC_TRAN_TYPE_* crap and use NT_* everywhere.

	1.17  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Move NWCC_INFO_MOUNT_POINT code from nwclient.c here.

	1.18  2002, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Handle correctly kernels reporting volume number > 255.

	1.19  2002, May 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Move NWCCOpenConnByName to resolve.c.

 */

#include "config.h"

#include <errno.h>
#include <string.h>
#include <strings.h>
#include <stdlib.h>
#include <stddef.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <sys/uio.h>
#include <sys/time.h>

#include <ncp/nwcalls.h>
#include <ncp/nwnet.h>
#include "ncplib_i.h"
#include "ncpcode.h"
#include "ncpi.h"

NWCCODE NWCallsInit(void* dummy1, void* dummy2) {
	(void)dummy1;
	(void)dummy2;
	return 0;
}

NWCCODE NWRequest(NWCONN_HANDLE conn, nuint function, 
	nuint numRq, const NW_FRAGMENT* rq, 
	nuint numRp, NW_FRAGMENT* rp) {
	long result;
	nuint i;
	size_t rest;
	unsigned char* ptr;

	ncp_init_request(conn);
	if (function & NCPC_SUBFUNCTION) {
		ncp_add_word_hl(conn, 0);
		ncp_add_byte(conn, NCPC_SUBFN(function));
		conn->has_subfunction = 1;
	}
	for (i=numRq; i--; rq++) {
		ncp_add_mem(conn, rq->fragAddr.ro, rq->fragSize);
	}
	result = ncp_request(conn, NCPC_FN(function));
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	rest = conn->ncp_reply_size;
	ptr = ncp_reply_data(conn, 0);
	for (i=numRp; i--; rp++) {
		size_t spc = rp->fragSize;
		if (spc > rest) {
			memcpy(rp->fragAddr.rw, ptr, rest);
			/* Do we want to modify fragList ? */
			/* I think so because of we want to propagate */
			/* reply length back to client */
			/* maybe we could do "return conn->ncp_reply_size" */
			/* instead of "return 0". */
			rp->fragSize = rest;
			rest = 0;
		} else {
			memcpy(rp->fragAddr.rw, ptr, spc);
			rest -= spc;
		}
	}
	ncp_unlock_conn(conn);
	return 0;
}
	
/* NWRequestSimple: Simple request, 1 request fragment, max. 1 reply fragment
 * !!! check carefully whether you call this function with rp->fragAddress ?= NULL !!!
 * conn = connection handle
 * function = NCP function (0x00 - 0xFF), subfunction 0x00xx - 0xFFxx, sfn flag 0x1xxxx
 * rq = pointer to request, if rqlen == 0 then address can be NULL
 * rqlen = request length, if rqlen == 0, rq can be NULL
 * rp = pointer to reply fragment
 *	on error: return nonzero code, unlock conn
 *	on success: return 0;
 *			if rp == NULL: throw away reply
 *			if rp != NULL && rp->fragAddress == NULL: fill rp with pointer
 *					to data and with size, do NOT unlock conn(!)
 *		if rp != NULL && rp->fragAddress != NULL: fill buffer pointed by
 *					rp->fragAddress with reply up to rp->fragSize
 *					rp->fragSize is reply length (can be greater than
 *					fragSize on input), unlock conn
 */
NWCCODE NWRequestSimple(NWCONN_HANDLE conn, nuint function,
	const void* rq, size_t rqlen, NW_FRAGMENT* rp) {
	long result;
	
	if ((rp && rp->fragSize && !rp->fragAddr.rw) || (rqlen && !rq))
		return NWE_PARAM_INVALID;
	ncp_init_request(conn);
	if (function & NCPC_SUBFUNCTION) {
		ncp_add_word_hl(conn, rqlen+1);
		ncp_add_byte(conn, NCPC_SUBFN(function));
	}
	if (rqlen) 
		ncp_add_mem(conn, rq, rqlen);
	result = ncp_request(conn, NCPC_FN(function));
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (rp) {
		size_t rest;
		void* ptr;

		rest = conn->ncp_reply_size;
		ptr = ncp_reply_data(conn, 0);
		if (rp->fragAddr.rw) {
			size_t maxsize = rp->fragSize;
			if (maxsize > rest)
				maxsize = rest;
			rp->fragSize = rest;
			memcpy(rp->fragAddr.rw, ptr, maxsize);
		} else {
			rp->fragAddr.rw = ptr;
			rp->fragSize = rest;
			return 0;
		}
	}
	ncp_unlock_conn(conn);
	return 0;
}

/* FIXME: Mark socket O_NONBLOCK instead */
#ifndef MSG_DONTWAIT
#define MSG_DONTWAIT 0
#endif
static int timedRecv(int fd, void* b, size_t len, int timeout) {
	struct timeval tv;
	unsigned char* buff = b;

	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;
	do {
		fd_set rdset;
		int rd;

		do {
			FD_ZERO(&rdset);
			FD_SET(fd, &rdset);
			rd = select(fd + 1, &rdset, NULL, NULL, &tv);
		} while (rd == -1 && errno == EINTR);
		if (rd == -1)
			break;
		if (!FD_ISSET(fd, &rdset))
			break;
		rd = recv(fd, buff, len, MSG_DONTWAIT);
		if (rd == 0)
			break;
		if (rd > 0) {
			size_t rd2 = rd;

			if (rd2 > len)
				break;
			buff += rd2;
			len -= rd2;
			if (!len)
				return 0;
		}
	} while (1);
	return -1;
}

static int timedRecvMsg(int fd, struct iovec* v, size_t vnum, int timeout) {
	struct timeval tv;

	tv.tv_sec = timeout / 1000000;
	tv.tv_usec = timeout % 1000000;
	do {
		fd_set rdset;
		int rd;
		struct msghdr m;

		do {
			FD_ZERO(&rdset);
			FD_SET(fd, &rdset);
			rd = select(fd + 1, &rdset, NULL, NULL, &tv);
		} while (rd == -1 && errno == EINTR);
		if (rd == -1)
			break;
		if (!FD_ISSET(fd, &rdset))
			break;
		m.msg_name = NULL;
		m.msg_namelen = 0;
		m.msg_iov = v;
		m.msg_iovlen = vnum;
		m.msg_control = NULL;
		m.msg_controllen = 0;
		m.msg_flags = 0;
		rd = recvmsg(fd, &m, MSG_DONTWAIT);
		if (rd == 0)
			break;
		if (rd > 0) {
			size_t rd2 = rd;
			while (rd2 >= v->iov_len) {
				rd2 -= v++->iov_len;
				if (!--vnum) {
					if (rd2)
						return -1;
					return 0;
				}
			}
			v->iov_base = (unsigned char*)v->iov_base + rd2;
			v->iov_len -= rd2;
		}
	} while (1);
	return -1;
}

static NWCCODE __NWExecuteGlobal(NWCONN_HANDLE conn, nuint function,
		nuint numRq, const NW_FRAGMENT* rq,
		nuint numRp, const NW_FRAGMENT* rp,
		size_t* rplen) {
	NWCCODE err = NCPLIB_NCPD_DEAD;
	int fd;
	struct msghdr msg;
	struct iovec iov[17];
	struct iovec* i;
	int sndlen;
	int rd;
	int retry = 0;
	size_t exprpl;
	size_t maxrpl;
	unsigned char rqb[20];
	nuint j;
	size_t rdu;

	if (rplen)
		*rplen = 0;

	if ((numRq > 16) || (numRp > 17)) {
		return NWE_REQ_TOO_MANY_REQ_FRAGS;
	}
	ncp_lock_conn(conn);
retryLoop:;	
	fd = conn->global_fd;
	if (fd == -1) {
		int fsfd;
		struct stat stb;
		struct sockaddr_un sun;
		socklen_t sunlen;

		fsfd = ncp_get_fid(conn);
		if (fsfd == -1) {
			goto errquit;
		}
		if (fstat(fsfd, &stb)) {
			goto errquit;
		}
		fd = socket(PF_UNIX, SOCK_STREAM, 0);
		if (fd < 0) {
			goto errquit;
		}
		sunlen = offsetof(struct sockaddr_un, sun_path) + sprintf(sun.sun_path, "%cncpfs.permanent.mount.%lu", 0,
				(unsigned long)stb.st_dev) + 1;
		sun.sun_family = AF_UNIX;
		if (connect(fd, (struct sockaddr*)&sun, sunlen)) {
			close(fd);
			goto errquit;
		}
		conn->global_fd = fd;
	}
	msg.msg_name = NULL;
	msg.msg_namelen = 0;
	msg.msg_iov = iov;
	msg.msg_iovlen = numRq + 1;
	msg.msg_control = NULL;
	msg.msg_controllen = 0;
	msg.msg_flags = 0;
	iov[0].iov_base = rqb;
	iov[0].iov_len = 20;
	sndlen = 20;
	i = iov + 1;
	{
		nuint numr = numRq;
		const NW_FRAGMENT* nwf = rq;
		
		while (numr--) {
			i->iov_base = nwf->fragAddr.rw;
			i->iov_len = nwf->fragSize;
			sndlen += nwf->fragSize;
			i++;
			rq++;
		}
		numr = numRp;
		nwf = rp;
		maxrpl = 12;
		while (numr--) {
			maxrpl += nwf++->fragSize;
		}
	}
	DSET_LH(rqb, 0, NCPI_REQUEST_SIGNATURE);
	DSET_LH(rqb, 4, sndlen);
	DSET_LH(rqb, 8, 1);
	DSET_LH(rqb, 12, maxrpl);
	DSET_LH(rqb, 16, function);
	rd = sendmsg(fd, &msg, MSG_NOSIGNAL);
	if (rd != sndlen) {
		close(fd);
		conn->global_fd = -1;
		if (retry) {
			goto errquit;
		}
		retry = 1;
		goto retryLoop;
	}
	rd = timedRecv(fd, rqb, 12, 30000000);
	if (rd < 0) {
		goto abort;
	}
	if (DVAL_LH(rqb, 0) != NCPI_REPLY_SIGNATURE) {
		goto abort;
	}
	exprpl = DVAL_LH(rqb, 4);
	if (exprpl < 12) {
		goto abort;
	}
	exprpl -= 12;
	rdu = exprpl;
	i = iov;
	for (j = 0; j < numRp; j++) {
		size_t r = rp[j].fragSize;
		if (r > rdu)
			r = rdu;
		if (r) {
			i->iov_base = rp[j].fragAddr.rw;
			i->iov_len = r;
			rdu -= r;
			i++;
		}
	}
	if (i != iov) {
		if (timedRecvMsg(fd, iov, i-iov, 10000000)) {
			goto abort;
		}
	}
	if (rplen) {
		*rplen = exprpl - rdu;
	}
	if (rdu) {
		timedRecv(fd, conn->packet, rdu, 10000000);
	}
	err = DVAL_LH(rqb, 8);
quit:;	
	ncp_unlock_conn(conn);
	return err;
abort:;
	close(fd);
	conn->global_fd = -1;
errquit:;
	goto quit;
}

NWCCODE NWGetObjectName(NWCONN_HANDLE conn, NWObjectID ID, char* name, NWObjectType* type) {
	long result;
	struct ncp_bindery_object spc;

	result = ncp_get_bindery_object_name(conn, ID, &spc);
	if (result)
		return result;
	if (name) strncpy(name, spc.object_name, NCP_BINDERY_NAME_LEN);
	if (type) *type = spc.object_type;
	return 0;
}

NWCCODE NWGetObjectID(NWCONN_HANDLE conn, const char* name, NWObjectType type, NWObjectID* ID) {
	long result;
	struct ncp_bindery_object spc;
	
	result = ncp_get_bindery_object_id(conn, type, name, &spc);
	if (result)
		return result;
	if (ID)
		*ID = spc.object_id;
	return 0;
}

NWCCODE NWGetBinderyAccessLevel(NWCONN_HANDLE conn,
		nuint8* level, NWObjectID* id) {
	nuint8 reply[16];
	NW_FRAGMENT rp;
	NWCCODE err;
	
	rp.fragAddr.rw = reply;
	rp.fragSize = sizeof(reply);
	
	err = NWRequestSimple(conn, NCPC_SFN(0x17, 0x46), NULL, 0, &rp);
	if (err)
		return err;
	if (rp.fragSize < 5)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (level)
		*level = BVAL(reply, 0);
	if (id)
		*id = DVAL_HL(reply, 1);
	return 0;
} 

NWCCODE NWGetNSEntryInfo(NWCONN_HANDLE conn, nuint dirHandle, const char* path, 
	nuint srcNS, nuint dstNS, nuint16 attr, nuint32 rim, NW_ENTRY_INFO* info) {
	unsigned char buff[1024];
	int rslt;

	rslt = ncp_path_to_NW_format(path, buff, sizeof(buff));
	if (rslt < 0)
		return -rslt;
	rslt = ncp_obtain_file_or_subdir_info2(conn, srcNS, dstNS, attr, rim, 
		dirHandle?0:0xFF, 0, dirHandle, buff, rslt, info);
	return rslt;
}

NWCCODE NWParsePath(const char* path, char* server, NWCONN_HANDLE* conn, 
	char* volume, char* volpath) {
	long result;
	NWCONN_HANDLE cn;
	struct NWCCRootEntry rent;
	char tmp[1000];
	char* ptr;

	if (!path)
		return ERR_NULL_POINTER;

	result = ncp_open_mount(path, &cn);
	if (result) {
		if (volume)
			*volume = 0;
		if (volpath)
			strcpy(volpath, path);
		if (conn)
			*conn = NULL;
		if (server)
			*server = 0;
		return 0;
	}
	result = NWCCGetConnInfo(cn, NWCC_INFO_ROOT_ENTRY, sizeof(rent), &rent);
	if (result) {
		ncp_close(cn);
		return NWE_REQUESTER_FAILURE;
	}
	if (rent.volume > 255) {
		*tmp = 0;
	} else {
		result = ncp_ns_get_full_name(cn, NW_NS_DOS, NW_NS_DOS, 1,
			rent.volume, rent.dirEnt, NULL, 0, tmp, sizeof(tmp));
		if (result) {
			ncp_close(cn);
			return result;
		}
	}
	ptr = strchr(tmp, ':');
	if (ptr) {
		if (volume) {
			memcpy(volume, tmp, ptr-tmp);
			volume[ptr-tmp]=0;
		}
		if (volpath)
			strcpy(volpath, ptr+1);
	} else {
		if (volume)
			*volume = 0;
		if (volpath)
			strcpy(volpath, tmp);
	}
	if (server) {
		if (NWGetFileServerName(cn, server)) {
			*server = 0;
		}
	}
	if (conn)
		*conn = cn;
	else
		ncp_close(cn);
	return 0;
}

NWCCODE NWDownFileServer(NWCONN_HANDLE conn, nuint force) {
	nuint8 flag = force?0:0xFF;

	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xD3), &flag, sizeof(flag), NULL);
}

NWCCODE NWCloseBindery(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0x44), NULL, 0, NULL);
}

NWCCODE NWOpenBindery(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0x45), NULL, 0, NULL);
}

NWCCODE NWDisableTTS(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xCF), NULL, 0, NULL);
}

NWCCODE NWEnableTTS(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xD0), NULL, 0, NULL);
}

NWCCODE NWDisableFileServerLogin(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xCB), NULL, 0, NULL);
}

NWCCODE NWEnableFileServerLogin(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xCC), NULL, 0, NULL);
}

static NWCCODE NWSMExec(NWCONN_HANDLE conn, int scode, 
		const nuint8* rqd, const char* txt, const char* txt2, 
	nuint32* rpd) {
	NW_FRAGMENT rq[4];
	NW_FRAGMENT rp;
	static const char here20[20] = { 0 };
	nuint8 rpl[24];
	nuint8 hdr[3];
	NWCCODE err;

	if (!rqd) rq[1].fragAddr.ro = here20; else rq[1].fragAddr.ro = rqd;
	rq[1].fragSize = 20;
	rq[0].fragAddr.ro = hdr;
	rq[0].fragSize = sizeof(hdr);
	rq[2].fragAddr.ro = txt;
	rq[2].fragSize = strlen(txt) + 1;
	rq[3].fragAddr.ro = txt2;
	rq[3].fragSize = txt2?(strlen(txt2) + 1):0;
	WSET_HL(hdr, 0, rq[3].fragSize + rq[2].fragSize + 1 + 20);
	BSET(hdr, 2, scode);
	rp.fragAddr.rw = rpl;
	rp.fragSize = sizeof(rpl);
	err = NWRequest(conn, 0x83, 4, rq, 1, &rp);
	if (err)
		return err;
	if (rp.fragSize < 4)
		return 0x897E;	/* buffer mismatch */
	err = DVAL_LH(rpl, 0);
	if (err)
		return err;
	if (rpd) {
		if (rp.fragSize < 24)
			return 0x897E;
		*rpd = DVAL_LH(rpl, 20);
	}
	return err;
}

NWCCODE NWSMLoadNLM(NWCONN_HANDLE conn, const char* cmd) {
	return NWSMExec(conn, 0x01, NULL, cmd, NULL, NULL);
}

NWCCODE NWSMUnloadNLM(NWCONN_HANDLE conn, const char* cmd) {
	return NWSMExec(conn, 0x02, NULL, cmd, NULL, NULL);
}

NWCCODE NWSMMountVolume(NWCONN_HANDLE conn, const char* vol, nuint32* volnum) {
	return NWSMExec(conn, 0x03, NULL, vol, NULL, volnum);
}

NWCCODE NWSMDismountVolumeByName(NWCONN_HANDLE conn, const char* vol) {
	return NWSMExec(conn, 0x04, NULL, vol, NULL, NULL);
}

NWCCODE NWSMDismountVolumeByNumber(NWCONN_HANDLE conn, nuint32 vol) {
	char volname[NCP_VOLNAME_LEN + 1];
	NWCCODE err;

	err = ncp_get_volume_name(conn, vol, volname, sizeof(volname));
	if (err)
		return err;
	return NWSMDismountVolumeByName(conn, volname);
}

NWCCODE NWSMSetDynamicCmdIntValue(NWCONN_HANDLE conn, const char* set,
		nuint32 value) {
	unsigned char hdr[20];

	memset(hdr, 0, sizeof(hdr));
	DSET_LH(hdr, 0, 1);
	DSET_LH(hdr, 4, value);
	return NWSMExec(conn, 0x06, hdr, set, NULL, NULL);
}

NWCCODE NWSMSetDynamicCmdStrValue(NWCONN_HANDLE conn, const char* set,
		const char* value) {
	return NWSMExec(conn, 0x06, NULL, set, value, NULL);
}

NWCCODE NWSMExecuteNCFFile(NWCONN_HANDLE conn, const char* cmd) {
	return NWSMExec(conn, 0x07, NULL, cmd, NULL, NULL);
}

/* Pre 3.11 NetWare */
static NWCCODE NWClearConnectionNumberV0(NWCONN_HANDLE conn, NWCONN_NUM connNumber) {
	nuint8 connN;

	if (connNumber >= 256)
		return NWE_CONN_NUM_INVALID;
	connN = connNumber;
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xD2), &connN, 1, NULL);
}

/* Post (including) 3.11 NetWare... */
static NWCCODE NWClearConnectionNumberV1(NWCONN_HANDLE conn, NWCONN_NUM connNumber) {
	nuint8 connN[4];

	DSET_LH(connN, 0, connNumber);
	return NWRequestSimple(conn, NCPC_SFN(0x17, 0xFE), &connN, 4, NULL);
}

NWCCODE NWClearConnectionNumber(NWCONN_HANDLE conn, NWCONN_NUM connNumber) {
	NWCCODE err;

	err = NWClearConnectionNumberV1(conn, connNumber);
	if (err == NWE_NCP_NOT_SUPPORTED) {
		err = NWClearConnectionNumberV0(conn, connNumber);
	}
	return err;
}

/* -------------------------------- */
/* Get object connection list
 * name must not be NULL (otherwise core)
 * name must be shorter than 256 chars (otherwise NWE_SERVER_FAILURE)
 * conns can be NULL
 * numConns can be NULL
 * returned numConns can be greater than maxConns, but connections
   above maxConns are lost
 * that's all? */
static NWCCODE NCPGetObjectConnectionList215(NWCONN_HANDLE conn,
		const char* name, nuint16 type, size_t* numConns,
		NWCONN_NUM* conns, size_t maxConns) {
	nuint8 rq_array[2+1];
	nuint8 rp_array[1+256];
	NW_FRAGMENT rq[2];
	NW_FRAGMENT rp;
	size_t strl;
	NWCCODE err;
	size_t nconns;
		
	strl = strlen(name);
	if (strl > 255)
		return NWE_SERVER_FAILURE;
	WSET_HL(rq_array, 0, type);
	BSET(rq_array, 2, strl);
	rq[0].fragAddr.ro = rq_array;
	rq[0].fragSize = 3;
	rq[1].fragAddr.ro = name;
	rq[1].fragSize = strl;
	rp.fragAddr.rw = rp_array;
	rp.fragSize = sizeof(rp_array);
	err = NWRequest(conn, NCPC_SFN(0x17, 0x15), 2, rq, 1, &rp);
	if (err)
		return err;
	if (rp.fragSize < 1) {
		/* at least one byte must be returned, do not you think? */
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nconns = BVAL(rp_array, 0);
	if (rp.fragSize <= nconns) {
		/* reply shorter than returned values... */
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (maxConns > nconns)
		maxConns = nconns;
	if (conns) {
		size_t i;
		
		for (i = 1; i <= maxConns; i++) {
			*conns++ = BVAL(rp_array, i);
		}
	}
	if (numConns)
		*numConns = nconns;
	return 0;
}

static NWCCODE NCPGetObjectConnectionList(NWCONN_HANDLE conn, NWCONN_NUM first,
		const char* name, nuint16 type, size_t* numConns,
		NWCONN_NUM* conns, size_t maxConns) {
	nuint8 rq_array[4+2+1];
	nuint8 rp_array[1+1024];
	NW_FRAGMENT rq[2];
	NW_FRAGMENT rp;
	size_t strl;
	NWCCODE err;
	size_t nconns;
		
	strl = strlen(name);
	if (strl > 255)
		return NWE_SERVER_FAILURE;
	DSET_LH(rq_array, 0, first);
	WSET_HL(rq_array, 4, type);
	BSET(rq_array, 6, strl);
	rq[0].fragAddr.ro = rq_array;
	rq[0].fragSize = 7;
	rq[1].fragAddr.ro = name;
	rq[1].fragSize = strl;
	rp.fragAddr.rw = rp_array;
	rp.fragSize = sizeof(rp_array);
	err = NWRequest(conn, NCPC_SFN(0x17, 0x1B), 2, rq, 1, &rp);
	if (err)
		return err;
	if (rp.fragSize < 1) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nconns = BVAL(rp_array, 0);
	if (rp.fragSize < nconns * 4 + 1) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (maxConns > nconns)
		maxConns = nconns;
	if (conns) {
		size_t i;
		
		for (i = 0; i < maxConns; i++) {
			*conns++ = DVAL_LH(rp_array, 1 + i * 4);
		}
	}
	if (numConns)
		*numConns = nconns;
	return 0;
}

static NWCCODE NCPGetConnListFromObject(NWCONN_HANDLE conn, NWObjectID objID,
		NWCONN_NUM firstConn, size_t* conns, NWCONN_NUM* connlist) {
	nuint8 rq_array[4+4];
	nuint8 rp_array[2+4*125]; /* 125 is magic NWServer constant... */
				  /* ncphdr+2+4*125 is just below 512... maybe? */
	NW_FRAGMENT rq;
	NW_FRAGMENT rp;
	NWCCODE err;
	size_t nconns;
		
	DSET_HL(rq_array, 0, objID);
	DSET_LH(rq_array, 4, firstConn);
	rq.fragAddr.ro = rq_array;
	rq.fragSize = 8;
	rp.fragAddr.rw = rp_array;
	rp.fragSize = sizeof(rp_array);
	err = NWRequest(conn, NCPC_SFN(0x17, 0x1F), 1, &rq, 1, &rp);
	if (err)
		return err;
	if (rp.fragSize < 2) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nconns = WVAL_LH(rp_array, 0);
	if (rp.fragSize < nconns * 4 + 2) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (nconns > 125)
		nconns = 125; /* ugh... Houston, we have a problem... */
	if (connlist) {
		size_t i;
		
		for (i = 0; i < nconns; i++) {
			*connlist++ = DVAL_LH(rp_array, 2 + i * 4);
		}
	}
	if (conns)
		*conns = nconns;
	return 0;
}

NWCCODE NWGetObjectConnectionNumbers(NWCONN_HANDLE conn, const char* name,
		nuint16 type, size_t* numConns, NWCONN_NUM* conns,
		size_t maxConns) {
	size_t returned;
	size_t complete;
	NWCCODE err;
	
	err = NCPGetObjectConnectionList(conn, 0, name, type, &returned, 
			conns, maxConns);
	if (err == NWE_NCP_NOT_SUPPORTED) {		
		return NCPGetObjectConnectionList215(conn, name, type, 
			numConns, conns, maxConns);
	}
	if (err)
		return err;
	complete = 0;
	/* TODO: do it better... */
	while (!err && (returned == 255) && (maxConns > 255) && conns) {
		conns += returned;
		maxConns -= returned;
		complete += returned;
		returned = 0;
		err = NCPGetObjectConnectionList(conn, conns[-1], name,
			type, &returned, conns, maxConns);
	}
	if (numConns)
		*numConns = complete + returned;
	return 0;	
}

NWCCODE NWGetConnListFromObject(NWCONN_HANDLE conn, NWObjectID objID,
		NWCONN_NUM firstConn, size_t* conns, NWCONN_NUM* connlist) {
	char name[NCP_BINDERY_NAME_LEN+1];
	nuint16 type;
	NWCCODE err;
	
	err = NCPGetConnListFromObject(conn, objID, firstConn, conns, connlist);
	if (err != NWE_NCP_NOT_SUPPORTED)
		return err;
	err = NWGetObjectName(conn, objID, name, &type);
	if (err)
		return err;
	if (firstConn) {
		if (conns) *conns = 0;
	} else
		err = NWGetObjectConnectionNumbers(conn, name, type, conns, connlist, 125);
	return err;
}

/* caller must not modify connArray during run ! */
static NWCCODE NCPSendBroadcastMessageOld(NWCONN_HANDLE conn, const char* message,
		size_t conns, NWCONN_NUM* connArray, nuint8* deliveryStatus) {
	nuint8 rq_array[1+255+1];
	nuint8 rp_array[1+255];
	size_t strl;
	NWCCODE err;
	size_t nconns;
	size_t rconns;
	size_t cnt;
	NWCONN_NUM* tmp;
	
	strl = strlen(message);
	if (strl > 58)
		strl = 58;
	if (conns > 255)
		return NWE_SERVER_FAILURE;
	if (!conns)
		return 0;
	nconns = 1;
	tmp = connArray;
	for (cnt = conns; cnt; cnt--) {
		NWCONN_NUM r = *tmp++;
		if (r < 256)
			BSET(rq_array, nconns++, r);
	}
	BSET(rq_array, 0, nconns-1);
	BSET(rq_array, nconns, strl);

	if (nconns != 1) {
		NW_FRAGMENT rq[2];
		NW_FRAGMENT rp;
		
		/* something to do... */
		rq[0].fragAddr.ro = rq_array;
		rq[0].fragSize = nconns + 1;
		rq[1].fragAddr.ro = message;
		rq[1].fragSize = strl;
		rp.fragAddr.rw = rp_array;
		rp.fragSize = sizeof(rp_array);
		err = NWRequest(conn, NCPC_SFN(0x15, 0x00), 2, rq, 1, &rp);
		if (err)
			return err;
		if (rp.fragSize < 1) {
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		rconns = BVAL(rp_array, 0);
		if (rp.fragSize < rconns + 1) {
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		if (rconns != nconns)
			return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (deliveryStatus) {
		size_t r = 1;
		for (; conns--;) {
			if (*connArray++ < 256)
				*deliveryStatus++ = BVAL(rp_array, r++);
			else
				*deliveryStatus++ = 0x01;
		}
	}
	return 0;
}

static NWCCODE NCPSendBroadcastMessage(NWCONN_HANDLE conn, const char* message,
		size_t conns, NWCONN_NUM* connArray, nuint8* deliveryStatus) {
#define __MAX_PER_REQUEST 512
	nuint8 rq_array[2+4*__MAX_PER_REQUEST+1];
	nuint8 rp_array[2+4*__MAX_PER_REQUEST];
	NW_FRAGMENT rq[2];
	NW_FRAGMENT rp;
	size_t strl;
	NWCCODE err;
	size_t nconns;
	size_t cnt;
		
	strl = strlen(message);
	if (strl > 255)
		strl = 255;
	if (conns > __MAX_PER_REQUEST)
		return NWE_SERVER_FAILURE;
	if (!conns)
		return 0;
	WSET_LH(rq_array, 0, conns);
	nconns = 2;
	for (cnt = conns; cnt; cnt--) {
		DSET_LH(rq_array, nconns, *connArray++);
		nconns += 4;
	}
	BSET(rq_array, nconns, strl);
	rq[0].fragAddr.ro = rq_array;
	rq[0].fragSize = nconns + 1;
	rq[1].fragAddr.ro = message;
	rq[1].fragSize = strl;
	rp.fragAddr.rw = rp_array;
	rp.fragSize = sizeof(rp_array);
	err = NWRequest(conn, NCPC_SFN(0x15, 0x0A), 2, rq, 1, &rp);
	if (err)
		return err;
	if (rp.fragSize < 2) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nconns = WVAL_LH(rp_array, 0);
	if (rp.fragSize < nconns * 4 + 2) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (nconns != conns)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (deliveryStatus) {
		size_t i;
		
		for (i = 0; i < nconns; i++) {
			*deliveryStatus++ = DVAL_LH(rp_array, 2 + i * 4);
		}
	}
	return 0;
#undef __MAX_PER_REQUEST	
}

NWCCODE NWSendBroadcastMessage(NWCONN_HANDLE conn, const char* message,
		size_t conns, NWCONN_NUM* connArray, nuint8* deliveryStatus) {
	NWCCODE err;
	
	err = NCPSendBroadcastMessage(conn, message, conns, connArray,
		deliveryStatus);
	if (err != NWE_NCP_NOT_SUPPORTED)
		return err;
	return NCPSendBroadcastMessageOld(conn, message, conns, connArray,
		deliveryStatus);
}

NWCCODE __NWReadFileServerInfo(NWCONN_HANDLE conn) {
	NWCCODE err;
	struct ncp_file_server_info_2 target;

	if (conn->serverInfo.valid)
		return 0;
	err = ncp_get_file_server_information_2(conn, &target, sizeof(target));
	if (err) {
		return err;
	}
	ncpt_mutex_lock(&conn->serverInfo.mutex);
	if (!conn->serverInfo.valid) {
		char* sn;
		conn->serverInfo.version.major = target.FileServiceVersion;
		conn->serverInfo.version.minor = target.FileServiceSubVersion;
		conn->serverInfo.version.revision = target.Revision;
		conn->serverInfo.ncp64bit = target._64BitOffsetsSupportedFlag != 0;
		sn = strdup(target.ServerName);
		if (sn) {
			if (conn->serverInfo.serverName)
				free(conn->serverInfo.serverName);
			conn->serverInfo.serverName = sn;
		} else
			err = ENOMEM;
		if (!err)
			conn->serverInfo.valid = 1;
	}
	ncpt_mutex_unlock(&conn->serverInfo.mutex);
	return err;
}

static NWCCODE NWGetFileServerNameInt(NWCONN_HANDLE conn, size_t maxlen, char* name) {
	NWCCODE err;

	err = __NWReadFileServerInfo(conn);
	if (err)
		return err;
	if (name) {
		size_t len = strlen(conn->serverInfo.serverName) + 1;
		if (len > maxlen)
			return NWE_BUFFER_OVERFLOW;
		memcpy(name, conn->serverInfo.serverName, len);
	}
	return 0;	
}

NWCCODE NWGetFileServerName(NWCONN_HANDLE conn, char* name) {
	return NWGetFileServerNameInt(conn, NCP_BINDERY_NAME_LEN+1, name);
}

NWCCODE NWGetFileServerVersion(NWCONN_HANDLE conn, u_int16_t* version) {
	NWCCODE err;

	err = __NWReadFileServerInfo(conn);
	if (err)
		return err;
	if (version) {
		*version = (conn->serverInfo.version.major << 8) 
			 | conn->serverInfo.version.minor;
	}
	return 0;
}

NWCCODE NWGetConnectionNumber(NWCONN_HANDLE conn, NWCONN_NUM* number) {
	int err = ncp_get_conn_number(conn);
	if (err < 0)
		return NWE_REQUESTER_FAILURE;
	*number = err;
	return 0;
}

NWCCODE ncp_put_req_size_unsigned(void* buffer, size_t reqlen, unsigned /* long */ long val) {
	switch (reqlen) {
		case 1:	if (val & ~0xFF)
				return E2BIG;
			*(u_int8_t*)buffer = val;
			return 0;
		case 2:	if (val & ~0xFFFF)
				return E2BIG;
			*(u_int16_t*)buffer = val;
			return 0;
		case 4:	/* if (val & ~0xFFFFFFFFUL)
				return E2BIG;
			*/
			*(u_int32_t*)buffer = val;
			return 0;
	}
	return NWE_PARAM_INVALID;
}

/* should be in ndslib.h ? */
#define NWE_BIND_NO_SUCH_PROP NWE_NCP_NOT_SUPPORTED

NWCCODE NWCCGetConnInfo(NWCONN_HANDLE conn, nuint info, size_t len,
		void* buffer) {
	if (!buffer) /* just in case */
		return ERR_NULL_POINTER;

	switch (info) {
		case NWCC_INFO_CONN_NUMBER:
			if (len == sizeof(NWCONN_NUM))
				return NWGetConnectionNumber(conn, (NWCONN_NUM*)buffer);
			{
				NWCONN_NUM cn;
				NWCCODE err;
				
				err = NWGetConnectionNumber(conn, &cn);
				if (err)
					return err;
				return ncp_put_req_size_unsigned(buffer, len, cn);
			}
		case NWCC_INFO_USER_ID:
			{
				if (!conn->user_id_valid) {
					NWCCODE err;
					struct ncp_bindery_object obj;
					NWCONN_NUM cn;

					err = NWGetConnectionNumber(conn, &cn);
					if (err)
						return err;
					err = ncp_get_stations_logged_info(conn, cn, &obj, NULL);
					if (err)
						return err;
					conn->user_id = obj.object_id;
					conn->user_id_valid = 1;
				}
				if (len != sizeof(NWObjectID))
					return NWE_BUFFER_OVERFLOW;
				*(NWObjectID*)buffer = conn->user_id;
				return 0;
			}
		case NWCC_INFO_SERVER_NAME:
			return NWGetFileServerNameInt(conn, len, buffer);
		case NWCC_INFO_MAX_PACKET_SIZE:
			return ncp_put_req_size_unsigned(buffer, len, conn->i.buffer_size);
		case NWCC_INFO_SERVER_VERSION:
			{
				NWCCODE err;

				err = __NWReadFileServerInfo(conn);
				if (err)
					return err;
				if (len != sizeof(NWCCVersion))
					return NWE_BUFFER_OVERFLOW;
				((NWCCVersion*)buffer)->major = conn->serverInfo.version.major;
				((NWCCVersion*)buffer)->minor = conn->serverInfo.version.minor;
				((NWCCVersion*)buffer)->revision = conn->serverInfo.version.revision;
#if 0
				printf ("NWCC_INFO_SERVER_VERSION %d %d %d %d %s\n", conn->serverInfo.valid,
					conn->serverInfo.version.major, conn->serverInfo.version.minor,
					conn->serverInfo.version.revision, conn->serverInfo.serverName);
#endif

			}
			return 0;
		case NWCC_INFO_TRAN_ADDR:
			{
				NWCCTranAddr* p = buffer;
				if (len < sizeof(NWCCTranAddr))
					return NWE_BUFFER_OVERFLOW;
				switch (conn->addr.any.sa_family) {
#ifdef CONFIG_NATIVE_IP
					case AF_INET:
						if (p->len < 6)
							return NWE_BUFFER_OVERFLOW;
						p->len = 6;
						p->type = NT_UDP;
						memcpy(((char*)(p->buffer))+2, &conn->addr.inet.sin_addr.s_addr, 4);
						memcpy(p->buffer, &conn->addr.inet.sin_port, 2);
						break;
#endif
#ifdef CONFIG_NATIVE_IPX
					case AF_IPX:
						if (p->len < 12)
							return NWE_BUFFER_OVERFLOW;
						p->len = 12;
						p->type = NT_IPX;
						memcpy(p->buffer, &conn->addr.ipx.sipx_network, 4);
						memcpy(((char*)(p->buffer))+4, conn->addr.ipx.sipx_node, 6);
						memcpy(((char*)(p->buffer))+10, &conn->addr.ipx.sipx_port, 2);
						break;
#endif
					default:
						return NWE_REQUESTER_FAILURE;
				}				
			}
			return 0;
		case NWCC_INFO_USER_NAME:
			{
				const char* user = "";
				size_t usrlen;
				NWCCODE err;
				struct ncp_bindery_object obj;

				if (!conn->user) {
					err = ncp_get_stations_logged_info(conn, conn->i.connection, &obj, NULL);
					if (!err)
						conn->user = strdup(obj.object_name);
				}
				if (conn->user)
					user = conn->user;
				usrlen = strlen(user) + 1;
				if (usrlen > len)
					return NWE_BUFFER_OVERFLOW;
				memcpy(buffer, user, usrlen);
			}
			return 0;
		case NWCC_INFO_MOUNT_UID:
			{
				uid_t tmpuid;
				NWCCODE err;

				if (ncp_get_conn_type(conn) != NCP_CONN_PERMANENT)
					return NWE_REQUESTER_FAILURE;
				if (len == sizeof(uid_t))
					return ncp_get_mount_uid(conn->mount_fid,
						buffer);
				err = ncp_get_mount_uid(conn->mount_fid,
					&tmpuid);
				if (err)
					return err;
				return ncp_put_req_size_unsigned(buffer, len, tmpuid);
			}
		case NWCC_INFO_SECURITY:
			{
				unsigned int val = 0;
				
				if (ncp_get_sign_wanted(conn))
					val |= NWCC_SECUR_LEVEL_SIGN_HEADERS;
				if (ncp_get_sign_active(conn))
					val |= NWCC_SECUR_SIGNING_IN_USE;
				return ncp_put_req_size_unsigned(buffer, len, val);
			}
		case NWCC_INFO_BCAST_STATE:
			{
				unsigned char rpb[4];
				NW_FRAGMENT rp;
				size_t tln;
				NWCCODE err;

				rp.fragAddr.rw = rpb;
				rp.fragSize = sizeof(rpb);
				
				err = __NWExecuteGlobal(conn, NCPI_OP_GETBCASTSTATE, 0, NULL, 1, &rp, &tln);
				if (err != NCPLIB_NCPD_DEAD) {
					if (err)
						return err;
					if (tln < 4)
						return NWE_INVALID_NCP_PACKET_LENGTH;
					return ncp_put_req_size_unsigned(buffer, len, DVAL_LH(rpb, 0));
				}
				if (conn->bcast_state == NWCC_BCAST_PERMIT_UNKNOWN)
					return NWE_REQUESTER_FAILURE;
				return ncp_put_req_size_unsigned(buffer, len, conn->bcast_state);
			}
		case NWCC_INFO_TREE_NAME:
#ifdef NDS_SUPPORT
			{

				char tn[MAX_TREE_NAME_CHARS+1];
				size_t treelen;

				if (!NWIsDSServer(conn, tn))
					return NWE_BIND_NO_SUCH_PROP;
				if (buffer) {
					char* p;
					p = strchr(tn, '\0') - 1;
					while ((p >= tn) && (*p == '_'))
						p--;
					treelen = p - tn + 1;
					tn[treelen] = 0;
					if (treelen >= len)
						return NWE_BUFFER_OVERFLOW;
					strcpy(buffer,tn);
				}
			}
			return 0;
#else
			return NWE_BIND_NO_SUCH_PROP;
#endif
		case NWCC_INFO_ROOT_ENTRY:
			{
				if (ncp_get_conn_type(conn) != NCP_CONN_PERMANENT)
					return NWE_REQUESTER_FAILURE;
				if (len < sizeof(struct NWCCRootEntry))
					return NWE_BUFFER_OVERFLOW;
				((struct NWCCRootEntry*)buffer)->volume = conn->i.volume_number;
				((struct NWCCRootEntry*)buffer)->dirEnt = conn->i.directory_id;
			}
			return 0;
		case NWCC_INFO_MOUNT_POINT:
			{
				size_t need_len;
				
				if (ncp_get_conn_type(conn) != NCP_CONN_PERMANENT)
					return NWE_REQUESTER_FAILURE;
				if (!conn->mount_point)
					return NWE_REQUESTER_FAILURE;
				need_len = strlen(conn->mount_point) + 1;
				if (len < need_len)
					return NWE_BUFFER_OVERFLOW;
				memcpy(buffer, conn->mount_point, need_len);
			}
			return 0;
		default:
			return NWE_INVALID_LEVEL;
	}
}

NWCCODE NWCCOpenConnByAddr(const NWCCTranAddr* addr, nuint openState,
		nuint reserved, NWCONN_HANDLE* conn) {
	nuint32 t;
	union ncp_sockaddr server;
	const nuint8* buffer;
	enum NET_ADDRESS_TYPE addrType;
	
	buffer = addr->buffer;
	if (!buffer)
		return NWE_PARAM_INVALID; /* ERR_NULL_POINTER */
	t = addr->type;
#ifdef CONFIG_NATIVE_IPX
	if (t == NWCC_TRAN_TYPE_IPX_old || t == NT_IPX) {
		if (addr->len < 12)
			return NWE_BUFFER_OVERFLOW;
		server.ipx.sipx_family = AF_IPX;
		memcpy(&server.ipx.sipx_network, buffer, 4);
		memcpy( server.ipx.sipx_node, buffer+4, 6);
		memcpy(&server.ipx.sipx_port, buffer+4+6, 2);
		server.ipx.sipx_type = 0x11;	/* NCP/IPX */
		addrType = NT_IPX;
	} else 
#endif
#ifdef CONFIG_NATIVE_IP
	if (t == NT_UDP || t == NT_TCP) {
		if (addr->len < 6)
			return NWE_BUFFER_OVERFLOW;
		server.inet.sin_family = AF_INET;
		memcpy(&server.inet.sin_addr.s_addr, buffer+2, 4);
		memcpy(&server.inet.sin_port, buffer, 2);
		addrType = t;
	} else
#endif
		return NWE_UNSUPPORTED_TRAN_TYPE;
	return NWCCOpenConnBySockAddr(&server.any, addrType, openState,
		reserved, conn);
}

NWCCODE NWCCCloseConn(NWCONN_HANDLE conn) {
	return ncp_close(conn);
}


/******************************************************** PP*/

NWCCODE NWScanVolDiskRestrictions(
		NWCONN_HANDLE	conn,
		NWVOL_NUM	volNum,
		nuint32*	sequence,
		NWVolumeRestrictions * volInfo) {
/* old API ??? by blocks of 12
   picke them up by blocks of 16 by return them by 12 */
	NWVOL_RESTRICTIONS myBuffer16;
	NWCCODE err;

	if (!volInfo)
		return ERR_NULL_POINTER;
	err = NWScanVolDiskRestrictions2(conn, volNum, sequence, &myBuffer16);
	if (err)
		return err;
	if (myBuffer16.numberOfEntries > 12)
		myBuffer16.numberOfEntries = 12;
	volInfo->numberOfEntries = myBuffer16.numberOfEntries;
	if (myBuffer16.numberOfEntries) 
		memcpy(volInfo->resInfo, myBuffer16.resInfo, myBuffer16.numberOfEntries * sizeof(NWOBJ_REST));
	return 0;
}

NWCCODE NWScanVolDiskRestrictions2(
		NWCONN_HANDLE	conn,
		NWVOL_NUM	volNum,
		nuint32*	sequence,
		NWVOL_RESTRICTIONS * volInfo) {
/* by blocks of 16 */
	nuint8 rq[1+4];
	nuint8 rp_b[sizeof (NWVOL_RESTRICTIONS)];
	NW_FRAGMENT rp;
	NWCCODE err;
	unsigned int tmp;
	unsigned int i;

	if (!sequence || !volInfo)
		return ERR_NULL_POINTER;

	rp.fragAddr.rw = rp_b;
	rp.fragSize = sizeof(rp_b);

	BSET(rq,0,volNum);
	DSET_LH(rq,1,*sequence);

	err = NWRequestSimple(conn, NCPC_SFN(22,32), &rq, 5, &rp);
	if (err)
		return err;
	if (rp.fragSize < 1)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	tmp = BVAL(rp_b,0);
	/* tmp =0 means no more entries, done */
	if (tmp > 16)
		return NWE_INVALID_NCP_PACKET_LENGTH; /* or a better code ??? */
	if (rp.fragSize < 1+tmp*8)
		return NWE_INVALID_NCP_PACKET_LENGTH;

	volInfo->numberOfEntries = tmp;
	for (i = 0; i < tmp; i++) {
		volInfo->resInfo[i].objectID = DVAL_HL(rp_b, 1+i*8);
		volInfo->resInfo[i].restriction = DVAL_LH(rp_b, 1+i*8+4);
	}
	for (; i < 16; i++) {
		volInfo->resInfo[i].objectID = 0;
		volInfo->resInfo[i].restriction = 0;
	}
	return 0;
}


NWCCODE NWRemoveObjectDiskRestrictions(
		NWCONN_HANDLE	conn,
		NWVOL_NUM	volNum,
		NWObjectID	objectID) {
	nuint8 rq[5];

	BSET(rq, 0, volNum);
	DSET_HL(rq, 1, objectID);
	return NWRequestSimple(conn, NCPC_SFN(22,34), &rq, 5, NULL);
}

NWCCODE NWSetObjectVolSpaceLimit(
		NWCONN_HANDLE	conn,
		NWVOL_NUM	volNum,
		NWObjectID	objectID,
		nuint32		restriction) {
	nuint8 rq[1+4+4];

	BSET(rq,0,volNum);
	DSET_HL(rq,1,objectID);
	DSET_LH(rq,5,restriction);
	return NWRequestSimple(conn, NCPC_SFN(22,33), &rq, 1+4+4, NULL);
}

NWCCODE NWGetObjDiskRestrictions(
		NWCONN_HANDLE	conn,
		NWVOL_NUM	volNum,
		NWObjectID	objectID,
		nuint32*	restriction,
		nuint32*	inUse) {
	nuint8 rq[1+4];
	nuint8 rp_b[4+4];
	NW_FRAGMENT rp;
	NWCCODE err;

	rp.fragAddr.rw = rp_b;
	rp.fragSize = sizeof(rp_b);

	BSET(rq, 0, volNum);
	DSET_HL(rq, 1, objectID);

	err = NWRequestSimple(conn, NCPC_SFN(22,41), &rq, 5, &rp);
	if (err)
		return err;
	if (rp.fragSize < 8)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (restriction)
		*restriction = DVAL_LH(rp_b, 0);
	if (inUse)
		*inUse = DVAL_LH(rp_b, 4);
	return 0;
}

/*****************************************************************************/
NWCCODE __NWEnableBroadcasts(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(21,3), NULL, 0, NULL);
}

NWCCODE __NWDisableBroadcasts(NWCONN_HANDLE conn) {
	return NWRequestSimple(conn, NCPC_SFN(21,2), NULL, 0, NULL);
}

NWCCODE NWGetBroadcastMode(NWCONN_HANDLE conn, nuint16* mode) {
	return NWCCGetConnInfo(conn, NWCC_INFO_BCAST_STATE, sizeof(*mode), mode);
}

NWCCODE NWSetBroadcastMode(NWCONN_HANDLE conn, nuint16 mode) {
	NWCCODE err;
	nuint8 rqb[4];
	NW_FRAGMENT rq;

	switch (mode) {
		case NWCC_BCAST_PERMIT_ALL:
		case NWCC_BCAST_PERMIT_SYSTEM:
		case NWCC_BCAST_PERMIT_NONE:
		case NWCC_BCAST_PERMIT_POLL:
			break;
		default:
			return NWE_PARAM_INVALID;
	}
	DSET_LH(rqb, 0, mode);
	rq.fragAddr.ro = rqb;
	rq.fragSize = sizeof(rqb);
	
	err = __NWExecuteGlobal(conn, NCPI_OP_SETBCASTSTATE, 1, &rq, 0, NULL, NULL);
	if (err != NCPLIB_NCPD_DEAD) {
		return err;
	}
	switch (mode) {
		case NWCC_BCAST_PERMIT_ALL:
			err = __NWEnableBroadcasts(conn); 
			break;
		case NWCC_BCAST_PERMIT_SYSTEM:
		case NWCC_BCAST_PERMIT_NONE:
			err = __NWDisableBroadcasts(conn);
			break;
		case NWCC_BCAST_PERMIT_POLL:
			err = __NWDisableBroadcasts(conn);
			break;
		default:
			err = NWE_PARAM_INVALID;
	}
	if (err)
		return err;
	conn->bcast_state = mode;
	return 0;
}

NWCCODE NWEnableBroadcasts(NWCONN_HANDLE conn) {
	return NWSetBroadcastMode(conn, NWCC_BCAST_PERMIT_ALL);
}

NWCCODE NWDisableBroadcasts(NWCONN_HANDLE conn) {
	return NWSetBroadcastMode(conn, NWCC_BCAST_PERMIT_NONE);
}

/********************************************************************************/


NWCCODE NWSignalSemaphore(
		NWCONN_HANDLE	conn,
		nuint32		semHandle) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, 3);	/* sub_func */
	ncp_add_dword_lh(conn, semHandle);
	err = ncp_request(conn, 111);
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE NWCloseSemaphore(
		NWCONN_HANDLE	conn,
		nuint32		semHandle) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, 4);	/* sub_func */
	ncp_add_dword_lh(conn, semHandle);
	err = ncp_request(conn, 111);
	/* this NCP returns openCount and shareCount , but no way to return them */
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE NWOpenSemaphore(
		NWCONN_HANDLE	conn,
		const char*	semName,
		nint16		initValue,
		nuint32*	semHandle,
		nuint16*	semOpenCount){
	NWCCODE err;
	size_t len;
	nuint8 nameBuf[512];

	if (!semName)
		return ERR_NULL_POINTER;
	if (!semHandle)
		return ERR_NULL_POINTER;

	len = strlen(semName);
	if (len > 255)
		len = 255;

	memset(nameBuf, 0, sizeof(nameBuf));
	memcpy(nameBuf, semName, len);
	ncp_init_request(conn);
	ncp_add_byte(conn, 0);	/* sub func 0 */
	ncp_add_byte(conn, initValue);
	ncp_add_byte(conn, len);
	ncp_add_mem(conn, nameBuf, sizeof(nameBuf));
	/* the string must be exactly 512 bytes
	   else server complains about a NCP packet incomplete */
	err = ncp_request(conn, 111);
	if (err) {
		ncp_unlock_conn(conn);
		return err;
	}

	*semHandle = ncp_reply_dword_lh(conn, 0);
	if (semOpenCount)
		*semOpenCount = ncp_reply_byte(conn, 4); /* NCP returns byte not word? */
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE NWExamineSemaphore(
		NWCONN_HANDLE	conn,
		nuint32		semHandle,
		nint16*		semValue,
		nuint16*	semOpenCount) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, 1);	/* sub_func */
	ncp_add_dword_lh(conn, semHandle);
	err = ncp_request(conn, 111);
	if (!err) {
		if (semValue) {
			*semValue = ncp_reply_byte(conn, 0); /* NCP returns byte not word? */
		}
		if (semOpenCount){
			*semOpenCount = ncp_reply_byte(conn, 1); /* NCP returns byte not word? */
		}
	}
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE NWWaitOnSemaphore(
		NWCONN_HANDLE	conn,
		nuint32		semHandle,
		nuint16		timeOutValue) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, 2);	/* sub_func */
	ncp_add_dword_lh(conn, semHandle);
	ncp_add_word_lh(conn, timeOutValue);
	err = ncp_request(conn, 111);
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE NWCancelWait(
		NWCONN_HANDLE	conn) {
	return NWRequestSimple(conn, NCPC_FN(112), NULL, 0, NULL);
}

NWCCODE NWGetVolumeNumber (
		NWCONN_HANDLE	conn,
		const char*	volumeName,
		NWVOL_NUM*	volumeNumber) {
	NWCCODE err;
	int v;

	/* nothing fancy but make code portable from MSWIN_PLAT to UNIX_PLAT ;-) */
	if (!volumeName)
		return ERR_NULL_POINTER;
	if (!volumeNumber)
		return ERR_NULL_POINTER;
	err = ncp_get_volume_number(conn, volumeName, &v);
	if (err)
		return err;
	*volumeNumber = v;
	return 0;
}



static NWCCODE __NWCCGetConnAddressInt(	/* internal */
		NWCONN_HANDLE	conn,
		nuint32*	bufLen,	/* return both */
		NWCCTranAddr*	tranAddr) {
	NWCCODE err;
	nuint8 myBuffer[32];
	NWCCTranAddr myTranAddr;

	myTranAddr.buffer = myBuffer;
	myTranAddr.len = sizeof(myBuffer);

	err = NWCCGetConnInfo(conn, NWCC_INFO_TRAN_ADDR, sizeof(myTranAddr), &myTranAddr);
	if (err)
		return err;
	if (bufLen)
		*bufLen = myTranAddr.len;
	if (tranAddr) {
		tranAddr->type = myTranAddr.type;
		tranAddr->len = myTranAddr.len;
		if (tranAddr->buffer)
			memcpy(tranAddr->buffer, myTranAddr.buffer, myTranAddr.len);
	}
	return 0;
}

NWCCODE NWCCGetConnAddress(
		NWCONN_HANDLE	conn,
		nuint32		bufLen,
		NWCCTranAddr*	tranAddr) {

	NWCCODE err;
	nuint32 aux;

	err = __NWCCGetConnAddressInt(conn, &aux, NULL);
	if (err)
		return err;
	if (aux > bufLen)
		return NWE_BUFFER_OVERFLOW;
	return __NWCCGetConnAddressInt(conn, NULL, tranAddr);
}

NWCCODE NWCCGetConnAddressLength(
		NWCONN_HANDLE	conn,
		nuint32*	bufLen) {
	return __NWCCGetConnAddressInt(conn, bufLen, NULL);
}

static NWCCODE __ncp_semaphore_fetch(CONN_SEMAPHORE *semaphore, CONN_SEMAPHORES *semaphores) {
	size_t nm;
	u_int8_t* base = semaphores->records + semaphores->curOffset;
	u_int8_t* end = semaphores->records + sizeof(semaphores->records);
	

	if (base + 7 > end) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nm = BVAL(base, 7);
	if (base + 7 + nm > end) {
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (nm >= sizeof(semaphore->semaphoreName)) {
		return NWE_BUFFER_OVERFLOW;
	}
	semaphore->openCount = WVAL_LH(base, 0);
	semaphore->semaphoreValue = WVAL_LH(base, 2);
	semaphore->taskNumber = WVAL_LH(base, 4);
	memcpy(semaphore->semaphoreName, base + 7, nm);
	semaphore->semaphoreName[nm] = 0;
	semaphores->curOffset += 7 + nm;
	semaphores->curRecord++;
	return 0;
}

NWCCODE
NWScanSemaphoresByConn(
		NWCONN_HANDLE	conn,
		NWCONN_NUM	connNum,
		u_int16_t*	iterHandle,
		CONN_SEMAPHORE*	semaphore,
		CONN_SEMAPHORES* semaphores) {
	NWCCODE result;
	u_int16_t ih;
	
	if (!iterHandle || !semaphores) {
		return NWE_PARAM_INVALID;
	}
	ih = *iterHandle;
	if (!ih) {
		semaphores->nextRequest = 0;
		semaphores->numRecords = 0;
		semaphores->curRecord = 0;
	} else if (ih < semaphores->numRecords) {
		if (!semaphore)
			return NWE_PARAM_INVALID;
		if (semaphores->curRecord != ih)
			return NWE_PARAM_INVALID;
		result = __ncp_semaphore_fetch(semaphore, semaphores);
		if (result)
			goto abrt;
		ih = semaphores->curRecord;
		goto finish;
	} else if (semaphores->nextRequest == 0) {
		return NWE_REQUESTER_FAILURE;
	}
	ncp_init_request_s(conn, 241);
	ncp_add_word_lh(conn, connNum);
	ncp_add_word_lh(conn, semaphores->nextRequest);
	result = ncp_request(conn, 23);
	if (result) {
		ncp_unlock_conn(conn);
		goto abrt;
	}
	if (conn->ncp_reply_size < 4) {
		ncp_unlock_conn(conn);
		result = NWE_INVALID_NCP_PACKET_LENGTH;
		goto abrt;
	}
	semaphores->nextRequest = ncp_reply_word_lh(conn, 0);
	semaphores->numRecords = ncp_reply_word_lh(conn, 2);
	if (semaphores->numRecords == 0) {
		ncp_unlock_conn(conn);
		result = NWE_REQUESTER_FAILURE;
		goto abrt;
	}
	{
		size_t tocopy;
		
		tocopy = conn->ncp_reply_size - 4;
		if (tocopy > sizeof(semaphores->records)) {
			tocopy = sizeof(semaphores->records);
		}
		memcpy(semaphores->records, ncp_reply_data(conn, 4), tocopy);
	}
	ncp_unlock_conn(conn);
	semaphores->curRecord = 0;
	semaphores->curOffset = 0;
	if (semaphores->numRecords && semaphore) {
		result = __ncp_semaphore_fetch(semaphore, semaphores);
		if (result)
			goto abrt;
		ih = 1;
	} else if (semaphores->numRecords) {
		ih = semaphores->numRecords;
	} else {
		semaphores->nextRequest = 0;
		ih = 0xFFFF;
	}
finish:;
	if (ih >= semaphores->numRecords && semaphores->nextRequest == 0) {
		ih = 0xFFFF;
	}
	*iterHandle = ih;
	return 0;
abrt:;
	semaphores->nextRequest = 0;
	semaphores->numRecords = 0;
	*iterHandle = 0xFFFF;
	return result;
}

NWCCODE
NWIsObjectInSet(NWCONN_HANDLE conn,
		const char* object_name,
		NWObjectType object_type,
		const char* property_name,
		const char* member_name,
		NWObjectType member_type) {
	NWCCODE result;

	ncp_init_request_s(conn, 67);
	ncp_add_word_hl(conn, object_type);
	ncp_add_pstring(conn, object_name);
	ncp_add_pstring(conn, property_name);
	ncp_add_word_hl(conn, member_type);
	ncp_add_pstring(conn, member_name);
	result = ncp_request(conn, 23);
	ncp_unlock_conn(conn);
	return result;
}
