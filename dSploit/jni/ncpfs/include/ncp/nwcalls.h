/*
    nwcalls.h
    Copyright (C) 1998-2001  Petr Vandrovec

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

	0.00  1998			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, June 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetBinderyAccessLevel.

        1.02  2001, January 7		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added NWCC_INFO_TREE_NAME, NWCCOpenConnByName and
		NWCC_NAME_FORMAT_*.

	1.03  2001, January 8		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added disk retrictions API calls.

	1.04  2001, January 9		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added semaphores API calls.

	1.05  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWScanOpenFilesByConn2, ncp_ns_scan_connections_using_file.
		Added NWGetNumberNCPExtensions, NWScanNCPExtensions.
		
	1.06  2001, February 7		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added Broadcast API calls & defines

	1.07  2001, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ncp_ns_scan_physical_locks_by_file.

	1.08  2001, February 10		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetVolumeName to header.

	1.09  2001, February 25		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added NWIsObjectInSet.

	1.10  2001, March 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added OT_TREE_NAME.

	1.11  2001, June 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetDirSpaceLimitList, NWGetDirSpaceLimitList2, 
			ncp_get_directory_info.
		Added NW_LIMIT_LIST, NW_MAX_VOLUME_NAME_LEN, DIR_SPACE_INFO.

	1.12  2001, July 16		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added __NWDisableBroadcasts and __NWEnableBroadcasts.
		
	1.13  2001, September 15	Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixes for SWIG. Unwind nested structs so that names are defined
		in this header and not by SWIG.

	1.14  2001, December 12		Hans Grobler <grobh@sun.ac.za>
		Added TR_ALL define.

	1.15  2001, December 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Updates for SWIG.

 */

#ifndef __NWCALLS_H__
#define __NWCALLS_H__

#include <ncp/ncplib.h>
#include <ncp/ext/stdint.h>

typedef unsigned int	nuint;
typedef u_int8_t	nuint8;
typedef u_int16_t	nuint16;
typedef u_int32_t	nuint32;

typedef int		nint;
typedef int16_t		nint16;
typedef int32_t		nint32;

typedef uint_least16_t	nuint16a;
typedef uint_least32_t	nuint32a;
typedef int_least16_t	nint16a;
typedef int_least32_t	nint32a;

typedef u_int8_t	nbool8;

typedef nuint32a	nflag32;

typedef struct nw_info_struct NW_ENTRY_INFO;
typedef nuint		NWCONN_NUM;	/* nuint16 ? */
#define nameLength nameLen

union NW_FRAGMENT_fragAddr {
	void*		rw;
	const void*	ro;
};
	
typedef struct {
	union NW_FRAGMENT_fragAddr fragAddr;
#define fragAddress fragAddr.rw
	size_t	fragSize;
		} NW_FRAGMENT;

/* including null byte... */
#define NW_MAX_VOLUME_NAME_LEN	17

#define TR_READ		NCP_PERM_READ
#define TR_WRITE	NCP_PERM_WRITE
#define TR_CREATE	NCP_PERM_CREATE
#define TR_ERASE	NCP_PERM_DELETE
#define TR_ACCESS_CTRL	NCP_PERM_OWNER
#define TR_SEARCH	NCP_PERM_SEARCH
#define TR_MODIFY	NCP_PERM_MODIFY
#define TR_SUPERVISOR	NCP_PERM_SUPER
#define TR_ALL		NCP_PERM_ALL

#define OT_USER		NCP_BINDERY_USER
#define OT_USER_GROUP	NCP_BINDERY_UGROUP
#define OT_FILE_SERVER	NCP_BINDERY_FSERVER
#define OT_TREE_NAME	NCP_BINDERY_TREE

#define IM_NAME		RIM_NAME
#define IM_DIRECTORY	RIM_DIRECTORY
#define IM_ALL		RIM_ALL

#define A_READ_ONLY		0x00000001	/* aRDONLY... but it is htonl-ed... fixme... */
#define A_HIDDEN		0x00000002
#define A_SYSTEM		0x00000004
#define A_EXECUTE_ONLY		0x00000008
#define A_DIRECTORY		0x00000010
#define A_NEEDS_ARCHIVED	0x00000020
#define A_EXECUTE_CONFIRM	0x00000040	/* does not occur in flags */
#define A_SHAREABLE		0x00000080
#define A_SEARCH_MODE		0x00000700
#define A_DONT_SUBALLOCATE	0x00000800
#define A_TRANSACTIONAL		0x00001000
#define A_INDEXED		0x00002000
#define A_READ_AUDIT		0x00004000
#define A_WRITE_AUDIT		0x00008000
#define A_IMMEDIATE_PURGE	0x00010000
#define A_RENAME_INHIBIT	0x00020000
#define A_DELETE_INHIBIT	0x00040000
#define A_COPY_INHIBIT		0x00080000
#define A_FILE_AUDITING		0x00100000
/* reserved			0x00200000 */
#define A_FILE_MIGRATED		0x00400000
#define A_DONT_MIGRATE		0x00800000
#define A_DATA_MIGRATION_SAVE_KEY \
				0x01000000
#define A_IMMEDIATE_COMPRESS	0x02000000
#define A_FILE_COMPRESSED	0x04000000
#define A_DONT_COMPRESS		0x08000000
#define A_CREATE_HARDLINK	0x10000000
#define A_CANT_COMPRESS		0x20000000
#define A_ATTRIBUTES_ARCHIVE	0x40000000
/* reserved			0x80000000 */

#define NWCC_RESERVED		0x00000000

/* note that NWCC_TRAN_TYPE_x must match NT_x */
/* in past NWCC_TRAN_TYPE_IPX was equal to 1, but no API
   returned it, they returned NT_IPX instead */
#define NWCC_TRAN_TYPE_IPX	0x00000000
#define NWCC_TRAN_TYPE_IPX_old	0x00000001
#define NWCC_TRAN_TYPE_UDP	0x00000008
#define NWCC_TRAN_TYPE_TCP	0x00000009
#define NWCC_TRAN_TYPE_WILD	0x00008000

#define NWCC_OPEN_PRIVATE	0x00000004
#define NWCC_OPEN_PUBLIC	0x00000008

#define NWCC_OPEN_NEW_CONN	0x00080000	/* ncpfs specific */

#define NWCC_INFO_AUTHENT_STATE		     1
#define NWCC_INFO_BCAST_STATE		     2
#define NWCC_INFO_TREE_NAME                  4
#define NWCC_INFO_CONN_NUMBER		     5
#define NWCC_INFO_USER_ID		     6
#define NWCC_INFO_SERVER_NAME		     7
#define NWCC_INFO_MAX_PACKET_SIZE	     9
#define NWCC_INFO_SERVER_VERSION	    12
#define NWCC_INFO_TRAN_ADDR		    13
#define NWCC_INFO_USER_NAME		0x4000
#define NWCC_INFO_ROOT_ENTRY		0x4001
#define NWCC_INFO_MOUNT_UID		0x4002
#define NWCC_INFO_SECURITY		0x4003
#define NWCC_INFO_MOUNT_POINT		0x4004

#define NW_MAX_SERVER_NAME_LEN		49

#define NWCC_SECUR_SIGNING_IN_USE	0x0001
#define NWCC_SECUR_LEVEL_CHECKSUM	0x0100
#define NWCC_SECUR_LEVEL_SIGN_HEADERS	0x0200
#define NWCC_SECUR_LEVEL_SIGN_ALL	0x0400
#define NWCC_SECUR_LEVEL_ENCRYPT	0x0800


/* Authentication States */
#define NWCC_AUTHENT_STATE_NONE     0x0000
#define NWCC_AUTHENT_STATE_BIND     0x0001
#define NWCC_AUTHENT_STATE_NDS      0x0002

/* Broadcast States */
#define NWCC_BCAST_PERMIT_ALL       0x0000
#define NWCC_BCAST_PERMIT_SYSTEM    0x0001
#define NWCC_BCAST_PERMIT_NONE      0x0002
#define NWCC_BCAST_PERMIT_POLL      0x0003

/* Name Format types */
#define NWCC_NAME_FORMAT_NDS        0x0001  
#define NWCC_NAME_FORMAT_BIND       0x0002
#define NWCC_NAME_FORMAT_NDS_TREE   0x0008
#define NWCC_NAME_FORMAT_WILD       0x8000
typedef struct {
	unsigned int major;
	unsigned int minor;
	unsigned int revision;
} NWCCVersion;

typedef struct {
	nuint32	type;
#ifdef SWIG
%pragma(swig) readonly
	size_t	len;
%pragma(swig) readwrite
	NWCCTranAddr_buffer buffer[32];
#else
	size_t	len;
	void*	buffer;
#endif
#if defined(SWIG_BUILD)
	nuint8	bufferdata[32];
#endif
} NWCCTranAddr;

struct NWCCRootEntry {
	NWVOL_NUM	volume;
	NWDIR_ENTRY	dirEnt;
};

#ifdef __cplusplus
extern "C" {
#endif

/* misc */
NWCCODE NWCallsInit(void* __NULL1, void* __NULL2);
NWCCODE NWRequest(NWCONN_HANDLE __conn, nuint, nuint, const NW_FRAGMENT*, nuint, NW_FRAGMENT*);
NWCCODE NWRequestSimple(NWCONN_HANDLE __conn, nuint, const void*, size_t, NW_FRAGMENT*);

/* bindery */
NWCCODE NWGetObjectName(NWCONN_HANDLE __conn, NWObjectID __objectid, 
		char* __objectname, NWObjectType* __objecttype);
NWCCODE NWGetObjectID(NWCONN_HANDLE __conn, const char* __objectname, 
		NWObjectType __objecttype, NWObjectID* __objectid);
NWCCODE NWGetBinderyAccessLevel(NWCONN_HANDLE __conn, nuint8* __rights,
		NWObjectID* __objectid);
NWCCODE NWLogoutFromFileServer(NWCONN_HANDLE __conn);	/* ncplib.c */
NWCCODE NWVerifyObjectPassword(NWCONN_HANDLE __conn, const char* __objectname, 
		NWObjectType __objecttype, const char* __objectpassword); /* ncplib.c */

/* filesystem */
NWCCODE NWGetNSEntryInfo(NWCONN_HANDLE __conn, nuint __dirhandle,
		const char* __path, nuint __srcns, nuint __dstns, 
		nuint16 __attr, nuint32 __rim, NW_ENTRY_INFO* target);
NWCCODE NWParsePath(const char* __path, char* __server, 
		NWCONN_HANDLE* __pconn, char* __volume, char* __volpath);

NWCCODE ncp_get_volume_name(NWCONN_HANDLE __conn, NWVOL_NUM __vol, char* __name, size_t __maxlen);	/* filemgmt.c */
NWCCODE NWGetVolumeNumber(NWCONN_HANDLE __conn, const char* __name, NWVOL_NUM* __vol);
NWCCODE NWGetVolumeName(NWCONN_HANDLE __conn, NWVOL_NUM __vol, char* __volume);

NWCCODE NWGetNSLoadedList(NWCONN_HANDLE __conn,
			  NWVOL_NUM __vol,
			  size_t __maxlen,
			  nuint8* __nslist, 
			  size_t* __nslen);	/* filemgmt.c */

typedef struct {
	u_int16_t	nextRequest;
	u_int16_t	openCount;
#ifdef SWIG
	fixedArray	buffer[512];
#else
	u_int8_t	buffer[512];
#endif
	u_int16_t	curRecord;
} OPEN_FILE_CONN_CTRL;

typedef struct {
	u_int16_t	taskNumber;
	u_int8_t	lockType;
	u_int8_t	accessControl;
	u_int8_t	lockFlag;
	NWVOL_NUM	volNumber;
	NWDIR_ENTRY	parent;
	NWDIR_ENTRY	dirEntry;
	u_int8_t	forkCount;
	u_int8_t	nameSpace;
#ifdef SWIG
%pragma(swig) readonly
	size_t		nameLen;
%pragma(swig) readwrite
	size_tLenPrefixCharArray	fileName[255];
#else
	size_t		nameLen;
	char		fileName[255];
#endif
} OPEN_FILE_CONN;

NWCCODE
NWScanOpenFilesByConn2(NWCONN_HANDLE	conn,
		       NWCONN_NUM	connNum,
		       u_int16_t	*iterHandle,
		       OPEN_FILE_CONN_CTRL *openCtrl,
		       OPEN_FILE_CONN	*openFile);	

typedef struct
#ifndef SWIG
__CONN_USING_FILE
#endif
{
	NWCONN_NUM	connNumber;
	u_int16_t	taskNumber;
	u_int8_t	lockType;
	u_int8_t	accessControl;
	u_int8_t	lockFlag;
} CONN_USING_FILE;

typedef struct {
	u_int16_t	nextRequest;
	u_int16_t	useCount;
	u_int16_t	openCount;
	u_int16_t	openForReadCount;
	u_int16_t	openForWriteCount;
	u_int16_t	denyReadCount;
	u_int16_t	denyWriteCount;
	u_int8_t	locked;
	u_int8_t	forkCount;
	u_int16_t	connCount;
	CONN_USING_FILE	connInfo[70];
} CONNS_USING_FILE;

NWCCODE
ncp_ns_scan_connections_using_file(
			NWCONN_HANDLE	conn,
			NWVOL_NUM	vol,
			NWDIR_ENTRY	dirent,
			int		datastream,
			u_int16_t	*iterHandle,
			CONN_USING_FILE	*cf,
			CONNS_USING_FILE *cfa);

typedef struct 
#ifndef SWIG
__PHYSICAL_LOCK 
#endif
{
	u_int16_t	loggedCount;
	u_int16_t	shareableLockCount;
	ncp_off64_t	recordStart;
	ncp_off64_t	recordEnd;
	NWCONN_NUM	connNumber;
	u_int16_t	taskNumber;
	u_int8_t	lockType;
} PHYSICAL_LOCK;

typedef struct {
	u_int16_t	nextRequest;
	u_int16_t	numRecords;
	PHYSICAL_LOCK	locks[32];
	u_int16_t	curRecord;
#ifdef SWIG
	fixedArray	reserved[8];
#else
	u_int8_t	reserved[8];
#endif
} PHYSICAL_LOCKS;

NWCCODE
ncp_ns_scan_physical_locks_by_file(
			NWCONN_HANDLE	conn,
			NWVOL_NUM	vol,
			NWDIR_ENTRY	dirent,
			int		datastream,
			u_int16_t	*iterHandle,
			PHYSICAL_LOCK	*lock,
			PHYSICAL_LOCKS	*locks);

typedef struct {
	u_int16_t	openCount;
	u_int16_t	semaphoreValue;
	u_int16_t	taskNumber;
#ifdef SWIG
	fixedCharArray	semaphoreName[128];
#else
	char		semaphoreName[128];
#endif
} CONN_SEMAPHORE;

typedef struct {
	u_int16_t	nextRequest;
	u_int16_t	numRecords;
#ifdef SWIG
	fixedArray	records[508];
#else
	u_int8_t	records[508];
#endif
	u_int16_t	curOffset;
	u_int16_t	curRecord;
} CONN_SEMAPHORES;

NWCCODE
NWScanSemaphoresByConn(
			NWCONN_HANDLE	conn,
			NWCONN_NUM	connNum,
			u_int16_t	*iterHandle,
			CONN_SEMAPHORE	*semaphore,
			CONN_SEMAPHORES	*semaphores);
				
/* management services */
NWCCODE NWOpenBindery(NWCONN_HANDLE __conn);
NWCCODE NWCloseBindery(NWCONN_HANDLE __conn);
NWCCODE NWDownFileServer(NWCONN_HANDLE __conn, nuint __force);
NWCCODE NWEnableFileServerLogin(NWCONN_HANDLE __conn);
NWCCODE NWDisableFileServerLogin(NWCONN_HANDLE __conn);
NWCCODE NWDisableTTS(NWCONN_HANDLE __conn);
NWCCODE NWEnableTTS(NWCONN_HANDLE __conn);

/* RPC services */
NWCCODE NWSMLoadNLM(NWCONN_HANDLE __conn, const char* __nlm);
NWCCODE NWSMUnloadNLM(NWCONN_HANDLE __conn, const char* __nlm);
NWCCODE NWSMMountVolume(NWCONN_HANDLE __conn, const char* __volume,
		nuint32* __volnumber);
NWCCODE NWSMDismountVolumeByName(NWCONN_HANDLE __conn, const char* __volume);
NWCCODE NWSMDismountVolumeByNumber(NWCONN_HANDLE __conn, nuint32 __volnumber);
NWCCODE NWSMExecuteNCFFile(NWCONN_HANDLE __conn, const char* __ncf);
NWCCODE NWSMSetDynamicCmdStrValue(NWCONN_HANDLE __conn, const char* __param, 
		const char* __value);
NWCCODE NWSMSetDynamicCmdIntValue(NWCONN_HANDLE __conn, const char* __param,
		nuint32 __value);

/* connection services */
NWCCODE NWClearConnectionNumber(NWCONN_HANDLE __conn, NWCONN_NUM __connnum);

NWCCODE NWGetObjectConnectionNumbers(NWCONN_HANDLE __conn, 
		const char* __objectname, NWObjectType __objecttype,
		size_t* noOfReturnedConns, NWCONN_NUM* conns, size_t maxConns);
NWCCODE NWGetConnListFromObject(NWCONN_HANDLE __conn, NWObjectID objID, 
		NWCONN_NUM firstConn,
		size_t* noOfReturnedConns, NWCONN_NUM* conns125); /* returned max. 125 */
		
/* message services */
NWCCODE NWSendBroadcastMessage(NWCONN_HANDLE __conn, const char* message,
		size_t conns, NWCONN_NUM* connArray, nuint8* deliveryStatus);

/* local connection services */
NWCCODE NWGetConnectionNumber(NWCONN_HANDLE __conn, NWCONN_NUM* __connnum);
NWCCODE NWGetFileServerName(NWCONN_HANDLE __conn, char* __server);
NWCCODE NWGetFileServerVersion(NWCONN_HANDLE __conn, u_int16_t* version);

NWCCODE NWCCGetConnInfo(NWCONN_HANDLE __conn, nuint info,
		size_t conninfolen, void* conninfoaddr);
NWCCODE NWCCOpenConnBySockAddr(const struct sockaddr* addr,
		enum NET_ADDRESS_TYPE type, nuint openState,
		nuint reserved, NWCONN_HANDLE* __pconn);
NWCCODE NWCCOpenConnByAddr(const NWCCTranAddr* addr, nuint openState,
		nuint reserved, NWCONN_HANDLE* __pconn);
NWCCODE NWCCCloseConn(NWCONN_HANDLE __conn);

/* ncp extensions */
#define MAX_NCP_EXTENSION_NAME_BYTES	33

NWCCODE NWGetNumberNCPExtensions(NWCONN_HANDLE __conn, nuint* __exts);
NWCCODE NWScanNCPExtensions(NWCONN_HANDLE __conn, nuint32* __iter, 
		char* __extname, nuint8* __majorVersion, 
		nuint8* __minorVersion, nuint8* __revision,
		nuint8 __queryData[32]);
NWCCODE NWFragNCPExtensionRequest(NWCONN_HANDLE conn, nuint32 NCPExtensionID, 
		nuint reqFragCount, NW_FRAGMENT* reqFragList, 
		nuint replyFragCount, NW_FRAGMENT* replyFragList);
NWCCODE NWNCPExtensionRequest(NWCONN_HANDLE conn, nuint32 NCPExtensionID,
                const void* requestData, size_t requestDataLen,
		void* replyData, size_t* replyDataLen);

NWCCODE NWEnableBroadcasts(NWCONN_HANDLE __conn);
NWCCODE NWDisableBroadcasts(NWCONN_HANDLE __conn);
#ifdef NCPFS_VERSION
NWCCODE __NWEnableBroadcasts(NWCONN_HANDLE __conn);
NWCCODE __NWDisableBroadcasts(NWCONN_HANDLE __conn);
#endif
NWCCODE NWGetBroadcastMode(NWCONN_HANDLE __conn, nuint16* __bcstmode) ;
NWCCODE NWSetBroadcastMode(NWCONN_HANDLE __conn, nuint16 __bcstmode);

NWCCODE NWCCOpenConnByName(NWCONN_HANDLE startConn, const char *serverName,
                nuint nameFormat,nuint openState,
		nuint reserved, NWCONN_HANDLE* __pconn);

typedef struct
#ifndef SWIG
__NWOBJ_REST
#endif
{
	NWObjectID objectID;
	nuint32 restriction;
} NWOBJ_REST;

typedef struct {
	nuint8  numberOfEntries;
	NWOBJ_REST resInfo[12];
} NWVolumeRestrictions;


typedef struct {
	nuint8  numberOfEntries;
	NWOBJ_REST resInfo[16];
} NWVOL_RESTRICTIONS;

NWCCODE NWGetObjDiskRestrictions(
	NWCONN_HANDLE   conn,
	NWVOL_NUM       volNumber,
	NWObjectID      objectID,
	nuint32*        restriction,
	nuint32*        inUse
);

NWCCODE NWScanVolDiskRestrictions(
	NWCONN_HANDLE   conn,
	NWVOL_NUM       volNum,
	nuint32*        iterhandle,
	NWVolumeRestrictions * volInfo
);

NWCCODE NWScanVolDiskRestrictions2( 
	NWCONN_HANDLE   conn,
	NWVOL_NUM       volNum,
	nuint32*        iterhandle,
	NWVOL_RESTRICTIONS* volInfo
);

NWCCODE NWRemoveObjectDiskRestrictions(
	NWCONN_HANDLE   conn,
	NWVOL_NUM       volNum,
	NWObjectID      objID
);

NWCCODE NWSetObjectVolSpaceLimit(
	NWCONN_HANDLE   conn,
	NWVOL_NUM       volNum,
	NWObjectID      objID,
	nuint32         restriction
);

NWCCODE NWGetDirSpaceLimitList(NWCONN_HANDLE conn, NWDIR_HANDLE dirhandle,
		nuint8* __buffer512);

typedef struct
#ifndef SWIG
__tag_NW_LIMIT_LIST_list
#endif
{
	nuint32		level;
	nuint32		max;
	nuint32		current;
} __NW_LIMIT_LIST_list;

typedef struct {
	size_t numEntries;
	__NW_LIMIT_LIST_list list[102];
} NW_LIMIT_LIST;
		
NWCCODE NWGetDirSpaceLimitList2(NWCONN_HANDLE conn, NWDIR_HANDLE dirhandle,
		NW_LIMIT_LIST* limitlist);

typedef struct {
	nuint32	totalBlocks;
	nuint32	availableBlocks;
	nuint32	purgeableBlocks;
	nuint32	notYetPurgeableBlocks;
	nuint32	totalDirEntries;
	nuint32	availableDirEntries;
	nuint32	reserved;
	nuint8	sectorsPerBlock;
#ifdef SWIG
%pragma(swig) readonly
	nuint8	volLen;
%pragma(swig) readwrite
	byteLenPrefixCharArray	volName[NW_MAX_VOLUME_NAME_LEN];
#else
	nuint8	volLen;
	char	volName[NW_MAX_VOLUME_NAME_LEN];
#endif
} DIR_SPACE_INFO;

NWCCODE ncp_get_directory_info(NWCONN_HANDLE conn, NWDIR_HANDLE dirhandle,
		                DIR_SPACE_INFO* target);


typedef struct {
	NWCONN_NUM connNumber;
	nuint16 taskNumber;
} SEMAPHORE;

typedef struct {
	nuint16 nextRequest;
	nuint16 openCount;
	nuint16 semaphoreValue;
	nuint16 semaphoreCount;
	SEMAPHORE semaphores[170];
	nuint16 curRecord;
} SEMAPHORES;

/********************* not yet
NWCCODE NWScanSemaphoresByName(
   NWCONN_HANDLE conn,
   const char*   semName,
   nint16*      iterHandle,
   SEMAPHORE  * semaphore,
   SEMAPHORES  * semaphores
);
******************************/

NWCCODE NWSignalSemaphore(NWCONN_HANDLE conn, nuint32 semHandle);
NWCCODE NWCloseSemaphore(NWCONN_HANDLE conn, nuint32 semHandle);
NWCCODE NWOpenSemaphore(NWCONN_HANDLE conn, const char* semName, 
		nint16 maxCount, nuint32* semHandle, nuint16* semOpenCount);
NWCCODE NWExamineSemaphore(NWCONN_HANDLE conn, nuint32 semHandle, 
		nint16* semValue, nuint16* semOpenCount);
NWCCODE NWWaitOnSemaphore(NWCONN_HANDLE conn, nuint32 semHandle, 
		nuint16 timeOutValue);
NWCCODE NWCancelWait(NWCONN_HANDLE conn);

NWCCODE NWCCGetConnAddress(NWCONN_HANDLE conn, nuint32 bufLen, 
		NWCCTranAddr* tranAddr);
NWCCODE NWCCGetConnAddressLength(NWCONN_HANDLE conn, nuint32* bufLen);

NWCCODE NWIsObjectInSet(NWCONN_HANDLE conn, const char* objectName, 
		NWObjectType objectType, const char* propertyName,
		const char* memberName, NWObjectType memberType);

#ifdef __cplusplus
}
#endif

#endif	/* __NWCALLS_H__ */
