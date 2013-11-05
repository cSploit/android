/*
    nwfse.h
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
		Initial revision.

 */

#ifndef __NWFSE_H__
#define __NWFSE_H__

#include <ncp/nwcalls.h>

typedef struct {
	u_int32_t	currentServerTime;
	u_int8_t	vconsoleVersion;
	u_int8_t	vconsoleRevision;
} NWSERVER_AND_VCONSOLE_INFO;

#define FSE_LOGGED_IN			0x00000001
#define FSE_BEING_ABORTED		0x00000002
#define FSE_AUDITED			0x00000004
#define FSE_NEEDS_SECURITY_CHANGE	0x00000008
#define FSE_MAC_STATION			0x00000010
#define FSE_AUTHENTICATED_TEMPORARY	0x00000020
#define FSE_AUDIT_CONNECTION_RECORDED	0x00000040
#define FSE_DSAUDIT_CONNECTION_RECORDED	0x00000080

#define FSE_WRITE		1
#define FSE_WRITE_ABORTED	2

#define FSE_NOT_WRITING		0
#define FSE_WRITE_IN_PROGRESS	1
#define FSE_WRITE_BEING_STOPPED	2

#define FSE_NCP_CONNECTION_TYPE		2
#define FSE_NLM_CONNECTION_TYPE		3
#define FSE_AFP_CONNECTION_TYPE		4
#define FSE_FTAM_CONNECTION_TYPE	5
#define FSE_ANCP_CONNECTION_TYPE	6
#define FSE_ACP_CONNECTION_TYPE		7
#define FSE_SMB_CONNECTION_TYPE		8
#define FSE_WINSOCK_CONNECTION_TYPE	9
#define FSE_HTTP_CONNECTION_TYPE	10
#define FSE_UDP_CONNECTION_TYPE		11

typedef struct {
	u_int32_t	connNum;
	u_int32_t	useCount;
	u_int8_t	connServiceType;
	u_int8_t	loginTime[7];
	u_int32_t	status;
	u_int32_t	expirationTime;
	u_int32_t	objType;
	u_int8_t	transactionFlag;
	u_int8_t	logicalLockThreshold;
	u_int8_t	recordLockThreshold;
	u_int8_t	fileWriteFlags;
	u_int8_t	fileWriteState;
	u_int8_t	filler;
	u_int16_t	fileLockCount;
	u_int16_t	recordLockCount;
	u_int64_t	totalBytesRead;
	u_int64_t	totalBytesWritten;
	u_int32_t	totalRequests;
	u_int32_t	heldRequests;
	u_int64_t	heldBytesRead;
	u_int64_t	heldBytesWritten;
} NWUSER_INFO;

typedef struct {
	NWSERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
	u_int16_t	reserved;
	NWUSER_INFO userInfo;
} NWFSE_USER_INFO;

#define NWFSE_NLM_NUMS_MAX	130

typedef struct {
	NWSERVER_AND_VCONSOLE_INFO serverTimeAndVConsoleInfo;
	u_int16_t	reserved;
	u_int32_t	numberNLMsLoaded;
	u_int32_t	NLMsInList;
	u_int32_t	NLMNums[NWFSE_NLM_NUMS_MAX];
} NWFSE_NLM_LOADED_LIST;

#ifdef __NWCOMPAT__
typedef NWSERVER_AND_VCONSOLE_INFO SERVER_AND_VCONSOLE_INFO;
typedef NWUSER_INFO USER_INFO;
#define FSE_NLM_NUMS_MAX	NWFSE_NLM_NUMS_MAX
#endif

#ifdef __cplusplus
extern "C" {
#endif

NWCCODE NWGetUserInfo(NWCONN_HANDLE conn, NWCONN_NUM connNum, char *userName, NWFSE_USER_INFO *userInfo);
NWCCODE NWGetNLMLoadedList(NWCONN_HANDLE conn, u_int32_t startNum, NWFSE_NLM_LOADED_LIST *fseNLMLoadedList);

#ifdef __cplusplus
}
#endif

#endif	/* __NWFSE_H__ */
