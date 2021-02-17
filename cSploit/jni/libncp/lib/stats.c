/* 
    stats.c - Server statistics
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

	1.00  2002, July 21		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

 */

#include "config.h"

//#include <errno.h>
//#include <string.h>
//#include <strings.h>
//#include <stdlib.h>
//#include <stddef.h>
//#include <unistd.h>
//#include <sys/stat.h>
//#include <sys/un.h>
//#include <sys/uio.h>
//#include <sys/time.h>

#include <ncp/nwfse.h>
//#include <ncp/nwnet.h>
#include "ncplib_i.h"
#include "ncpcode.h"
#include "ncpi.h"

static inline void vconsole_ver(NWSERVER_AND_VCONSOLE_INFO* svi, const unsigned char* packet) {
	svi->currentServerTime = DVAL_LH(packet, 0);
	svi->vconsoleVersion = BVAL(packet, 4);
	svi->vconsoleRevision = BVAL(packet, 5);
}

static inline void fetch_userinfo(NWUSER_INFO* ui, const unsigned char* packet) {
	ui->connNum		 = DVAL_LH(packet, 0);
	ui->useCount		 = DVAL_LH(packet, 4);
	ui->connServiceType	 = BVAL(packet, 8);
	memcpy(ui->loginTime, packet + 9, 7);
	ui->status		 = DVAL_LH(packet, 16);
	ui->expirationTime	 = DVAL_LH(packet, 20);
	ui->objType		 = DVAL_LH(packet, 24);
	ui->transactionFlag	 = BVAL(packet, 28);
	ui->logicalLockThreshold = BVAL(packet, 29);
	ui->recordLockThreshold	 = BVAL(packet, 30);
	ui->fileWriteFlags	 = BVAL(packet, 31);
	ui->fileWriteState	 = BVAL(packet, 32);
	ui->filler		 = BVAL(packet, 33);
	ui->fileLockCount	 = WVAL_LH(packet, 34);
	ui->recordLockCount	 = WVAL_LH(packet, 36);
	ui->totalBytesRead	 = SVAL_LH(packet, 38);
	ui->totalBytesWritten	 = SVAL_LH(packet, 44);
	ui->totalRequests	 = DVAL_LH(packet, 50);
	ui->heldRequests	 = DVAL_LH(packet, 54);
	ui->heldBytesRead	 = SVAL_LH(packet, 58);
	ui->heldBytesWritten	 = SVAL_LH(packet, 64);
}

static NWCCODE NWGetUserInfo2(NWCONN_HANDLE conn, NWCONN_NUM connNum, char* userName,
		size_t userNameLen, NWFSE_USER_INFO *fseUserInfo) {
	unsigned char rq[4];
	NWCCODE err;
	NW_FRAGMENT rpf;
	unsigned int nameLen;
	
	rpf.fragAddress = NULL;
	rpf.fragSize = 0;
	DSET_LH(rq, 0, connNum);
	err = NWRequestSimple(conn, NCPC_SFN(0x7B, 0x04), rq, sizeof(rq), &rpf);
	if (err) {
		return err;
	}
	if (rpf.fragSize < 8 + 70 + 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nameLen = BVAL(rpf.fragAddress, 8 + 70);
	if (rpf.fragSize < 8 + 70 + 1 + nameLen) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (userName) {
		if (nameLen >= userNameLen) {
			ncp_unlock_conn(conn);
			return NWE_BUFFER_OVERFLOW;
		}
		memcpy(userName, (unsigned char*)rpf.fragAddress + 8 + 70 + 1, nameLen);
		userName[nameLen] = 0;
	}
	if (fseUserInfo) {
		vconsole_ver(&fseUserInfo->serverTimeAndVConsoleInfo, rpf.fragAddress);
		fseUserInfo->reserved = WVAL_LH(rpf.fragAddress, 6);
		fetch_userinfo(&fseUserInfo->userInfo, (unsigned char*)rpf.fragAddress + 8);
	}
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE NWGetUserInfo(NWCONN_HANDLE conn, NWCONN_NUM connNum, char* userName, 
		NWFSE_USER_INFO *fseUserInfo) {
	return NWGetUserInfo2(conn, connNum, userName, 256, fseUserInfo);
}

NWCCODE NWGetNLMLoadedList(NWCONN_HANDLE conn, u_int32_t startNum, NWFSE_NLM_LOADED_LIST *fseNLMLoadedList) {
	unsigned char rq[4];
	NWCCODE err;
	NW_FRAGMENT rpf;
	unsigned int nlmsInList;
	
	rpf.fragAddress = NULL;
	rpf.fragSize = 0;
	DSET_LH(rq, 0, startNum);
	err = NWRequestSimple(conn, NCPC_SFN(0x7B, 0x0A), rq, sizeof(rq), &rpf);
	if (err) {
		return err;
	}
	if (rpf.fragSize < 16) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nlmsInList = DVAL_LH(rpf.fragAddress, 12);
	if (nlmsInList > NWFSE_NLM_NUMS_MAX) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	if (rpf.fragSize < 16 + nlmsInList * 4) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (fseNLMLoadedList) {
		unsigned int cnt;
		
		vconsole_ver(&fseNLMLoadedList->serverTimeAndVConsoleInfo, rpf.fragAddress);
		fseNLMLoadedList->reserved = WVAL_LH(rpf.fragAddress, 6);
		fseNLMLoadedList->numberNLMsLoaded = DVAL_LH(rpf.fragAddress, 8);
		fseNLMLoadedList->NLMsInList = nlmsInList;
		for (cnt = 0; cnt < nlmsInList; cnt++) {
			fseNLMLoadedList->NLMNums[cnt] = DVAL_LH(rpf.fragAddress, 16 + cnt * 4);
		}
	}
	ncp_unlock_conn(conn);
	return 0;
}
