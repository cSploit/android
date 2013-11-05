/*
    nwnet.h
    Copyright (C) 1999  Petr Vandrovec

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

	0.00  1999			Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial revision.

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, February 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSSearch* function group.

	1.02  2000, April		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGenerateObjectKeyPair, NWDSVerifyObjectPassword,
			NWDSChangeObjectPassword functions group.
		
	1.03  2000, April 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetNDSStatistics, NWDSAbortPartitionOperation,
			NWDSGetCountByClassAndName.

	1.04  2000, April 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added DSPING* definitions.
		Added NWDSGetDSVerInfo, NWDSJoinPartitions, 
			NWDSListPartitions, NWDSGetServerName.

	1.05  2000, April 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added DSP_* definitions.
		Added NWDSListPartitionsExtInfo, NWDSGetPartitionInfo, 
			NWDSGetPartitionExtInfoPtr, NWDSGetPartitionExtInfo,
			NWDSSplitPartition, NWDSRemovePartition.
			
	1.06  2000, April 29		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSAddReplica, NWDSRemoveReplica, NWDSChangeReplicaType.
		
	1.07  2000, April 30		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSPartitionReceiveAllUpdates, 
			NWDSPartitionSendAllUpdates, NWDSSyncPartition,
			NWDSGetPartitionRoot.
		Added NWDSGetEffectiveRights, NWDSListAttrsEffectiveRights.
		Added NWDSExtSyncList, NWDSExtSyncRead.
		
	1.08  2000, May 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetBinderyContext, NWDSReloadDS, 
			NWDSResetNDSStatistics.
		Added NWDSRepairTimeStamps.
		Added NWGetFileServerUTCTime.

	1.09  2000, May 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSReadClassDef, NWDSGetClassDefCount, NWDSGetClassDef.

	1.10  2000, May 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSGetClassItemCount, NWDSGetClassItem.
		
	1.11  2000, May 7		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSPutClassItem, NWDSPutClassName, NWDSBeginClassItem,
			NWDSDefineClass, NWDSRemoveClassDef, 
			NWDSListContainableClasses, NWDSModifyClassDef,
			NWDSReadAttrDef, NWDSGetAttrDef, NWDSDefineAttr,
			NWDSRemoveAttrDef.

	1.12  2000, May 8		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSSyncSchema.

	1.13  2001, January 15		Patrick Pollet <patrick.pollet@insa-lyon.fr>
		Added NWDSReadSyntaxes, NWDSPutSyntaxName,NWDSGetSyntaxDef,
			NWDSGetSyntaxCount,NWDSReadSyntaxDef, NWDSGetSyntaxID,
			NWDSScanConnsForTrees, NWDSScanForAvailableTrees,
			NWDSReturnBlockOfAvailableTrees, NWDSWhoAmI

	1.14  2001, March 10		Petr Vandrovec <vandrove@vc.cvut.cz>
		Moved several defs from nwclient here.

	1.15  2001, June 2		Petr Vandrovec <vandrove@vc.cvut.cz>
		Add NWDSPutFilter and NWDSFreeFilter.

	1.16  2001, December 11		Hans Grobler <grobh@sun.ac.za>
		Added DS_ATTR_* definitions.

	1.17  2001, December 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWDSDuplicateContext.

	1.18  2002, January 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added __NWDSOpenStream.
	
 */

#ifndef __NWNET_H__
#define __NWNET_H__

#include <ncp/ndslib.h>
#include <ncp/nwcalls.h>
#include <wchar.h>

#ifndef NWDSChar
typedef char NWDSChar;
#define NWDSChar NWDSChar
#endif

#ifdef SWIG
struct __NWDSContextHandle;
#endif

typedef struct __NWDSContextHandle *NWDSContextHandle;
typedef int NWDSCCODE;

#define DCK_FLAGS		1
#define DCK_CONFIDENCE		2
#define DCK_NAME_CONTEXT	3
#define DCK_LAST_CONNECTION	8
#define DCK_TREE_NAME		11
#define DCK_DSI_FLAGS		12
#define DCK_NAME_FORM		13
#define DCK_LOCAL_CHARSET	32

#define DCV_DEREF_ALIASES	0x00000001
#define DCV_XLATE_STRINGS	0x00000002
#define DCV_TYPELESS_NAMES	0x00000004
#define DCV_CANONICALIZE_NAMES	0x00000010
#define DCV_DEREF_BASE_CLASS	0x00000040
#define DCV_DISALLOW_REFERRALS	0x00000080

#define DCV_LOW_CONF		0
#define DCV_MED_CONF		1
#define DCV_HIGH_CONF		2

#define MAX_RDN_CHARS		128
#define MAX_DN_CHARS		256
#define MAX_SCHEMA_NAME_CHARS	32
#define MAX_TREE_NAME_CHARS	32
#define MAX_ASN1_NAME		32

#define MAX_RDN_BYTES		(4*(MAX_RDN_CHARS+1))
#define MAX_DN_BYTES		(4*(MAX_DN_CHARS+1))
#define MAX_SCHEMA_NAME_BYTES	(4*(MAX_SCHEMA_NAME_CHARS+1))
#define MAX_TREE_NAME_BYTES	(4*(MAX_TREE_NAME_CHARS+1))

#define DS_NCP_VERB		104

#define DS_NCP_PING		1
#define DS_NCP_FRAGMENT		2

#define DS_NCP_BINDERY_CONTEXT	4

#define DS_NCP_GET_DS_STATISTICS	6
#define DS_NCP_RESET_DS_COUNTERS	7
#define DS_NCP_RELOAD		8


#define DSPING_SUPPORTED_FIELDS	0x00000001
#define DSPING_DEPTH		0x00000002
#define DSPING_BUILD_NUMBER	0x00000004
#define DSPING_FLAGS		0x00000008
/* also 0x3F0 flags are defined, but as they are not exposed through any NW API...
   BTW, you must use ping v1 for them */
#define DSPING_SAP_NAME		0x00010000
#define DSPING_TREE_NAME	0x00020000
/* also 0x001C0000 flags are defined, but as they are not exposed through any NW API...
   BTW, you must use ping v1 for them */


#define DSV_RESOLVE_NAME	1
#define DSV_READ_ENTRY_INFO	2
#define DSV_READ		3
#define DSV_COMPARE		4
#define DSV_LIST		5
#define DSV_SEARCH		6
#define DSV_ADD_ENTRY		7
#define DSV_REMOVE_ENTRY	8
#define DSV_MODIFY_ENTRY	9
#define DSV_MODIFY_RDN		10
#define DSV_DEFINE_ATTR		11
#define DSV_READ_ATTR_DEF	12
#define DSV_REMOVE_ATTR_DEF	13
#define DSV_DEFINE_CLASS	14
#define DSV_READ_CLASS_DEF	15
#define DSV_MODIFY_CLASS_DEF	16
#define DSV_REMOVE_CLASS_DEF	17
#define DSV_LIST_CONTAINABLE_CLASSES	18
#define DSV_GET_EFFECTIVE_RIGHTS	19

#define DSV_REMOVE_PARTITION	21
#define DSV_LIST_PARTITIONS	22
#define DSV_SPLIT_PARTITION	23
#define DSV_JOIN_PARTITIONS	24
#define DSV_ADD_REPLICA		25
#define DSV_REMOVE_REPLICA	26
#define DSV_OPEN_STREAM		27
#define DSV_SEARCH_FILTER	28

#define DSV_CHANGE_REPLICA_TYPE	31

#define DSV_SYNC_PARTITION	38
#define DSV_SYNC_SCHEMA		39
#define DSV_READ_SYNTAXES	40

#define DSV_BEGIN_MOVE_ENTRY	42
#define DSV_FINISH_MOVE_ENTRY	43

#define DSV_CLOSE_ITERATION	50

#define DSV_GET_SERVER_ADDRESS	53
#define DSV_SET_KEYS		54
#define DSV_CHANGE_PASSWORD	55
#define DSV_VERIFY_PASSWORD	56
#define DSV_BEGIN_LOGIN		57
#define DSV_FINISH_LOGIN	58
#define DSV_BEGIN_AUTHENTICATION	59
#define DSV_FINISH_AUTHENTICATION	60

#define DSV_REPAIR_TIMESTAMPS	63

#define DSV_ABORT_PARTITION_OPERATION	76

#define DSV_PARTITION_FUNCTIONS	78

#define DSV_GET_BINDERY_CONTEXTS	95

#define DSI_OUTPUT_FIELDS		0x00000001
#define DSI_ENTRY_ID			0x00000002
#define DSI_ENTRY_FLAGS			0x00000004
#define DSI_SUBORDINATE_COUNT		0x00000008
#define DSI_MODIFICATION_TIME		0x00000010
#define DSI_MODIFICATION_TIMESTAMP	0x00000020
#define DSI_CREATION_TIMESTAMP		0x00000040
#define DSI_PARTITION_ROOT_ID		0x00000080
#define DSI_PARENT_ID			0x00000100
#define DSI_REVISION_COUNT		0x00000200
#define DSI_REPLICA_TYPE		0x00000400
#define DSI_BASE_CLASS			0x00000800
#define DSI_ENTRY_RDN			0x00001000
#define DSI_ENTRY_DN			0x00002000
#define DSI_PARTITION_ROOT_DN		0x00004000
#define DSI_PARENT_DN			0x00008000
#define DSI_PURGE_TIME			0x00010000
#define DSI_DEREFERENCE_BASE_CLASS	0x00020000
#define DSI_REPLICA_NUMBER		0x00040000
#define DSI_REPLICA_STATE		0x00080000

#define DCV_NF_PARTIAL_DOT	1
#define DCV_NF_FULL_DOT		2
#define DCV_NF_SLASH		3

#define DEFAULT_MESSAGE_LEN	4096

#ifdef SWIG
#define NO_MORE_ITERATIONS	(0xFFFFFFFFUL)
#else
#define NO_MORE_ITERATIONS	((nuint32)-1)
#endif

/* DSV_READ */
#define DS_ATTRIBUTE_NAMES	0x00
#define DS_ATTRIBUTE_VALUES	0x01
#define DS_EFFECTIVE_PRIVILEGES	0x02	/* only for NWDSListAttrsEffectiveRights */
#define DS_VALUE_INFO		0x03
#define DS_ABBREVIATED_VALUE	0x04

/* DSV_MODIFY_ENTRY */
#define DS_ADD_ATTRIBUTE	0x00
#define DS_REMOVE_ATTRIBUTE	0x01	/* does not have value... */
#define DS_ADD_VALUE		0x02
#define DS_REMOVE_VALUE		0x03
#define DS_ADDITIONAL_VALUE	0x04
#define DS_OVERWRITE_VALUE	0x05
#define DS_CLEAR_ATTRIBUTE	0x06	/* does not have value... */
#define DS_CLEAR_VALUE		0x07

/* DSV_SEARCH */
#define DS_SEARCH_ENTRY		0x00
#define DS_SEARCH_SUBORDINATES	0x01
#define DS_SEARCH_SUBTREE	0x02
#define DS_SEARCH_PARTITION	0x03

/* DSV_READ_CLASS_DEF */
#define DS_CLASS_DEF_NAMES	0
#define DS_CLASS_DEFS		1
#define DS_EXPANDED_CLASSS_DEFS	2
#define DS_INFO_CLASS_DEFS	3
#define DS_FULL_CLASS_DEFS	4
/* #define DS_FULL_CLASS_DEFS_AND_TIMESTAMPS  5 */

/* NWDSGenerateObjectKeyPair* */
#define NDS_PASSWORD		1

/* DSV_LIST_PARTITIONS */
#define DSP_OUTPUT_FIELDS		0x00000001
#define DSP_PARTITION_ID		0x00000002
#define DSP_REPLICA_STATE		0x00000004
#define DSP_MODIFICATION_TIMESTAMP	0x00000008
#define DSP_PURGE_TIME			0x00000010
#define DSP_LOCAL_PARTITION_ID		0x00000020
#define DSP_PARTITION_DN		0x00000040
#define DSP_REPLICA_TYPE		0x00000080
#define DSP_PARTITION_BUSY		0x00000100
/* #define DSP_UNKNOWN_00000200		0x00000200    NDS 8.17 does not have it, beta of DirXML NDS 8400 has it */

/* NDS Attribute Rights */
#define DS_ENTRY_BROWSE		0x00000001
#define DS_ENTRY_ADD		0x00000002
#define DS_ENTRY_DELETE		0x00000004
#define DS_ENTRY_RENAME		0x00000008
#define DS_ENTRY_SUPERVISOR	0x00000010
#define DS_ENTRY_INHERIT_CTL	0x00000040

#define DS_ENTRY_MASK		(DS_ENTRY_BROWSE | DS_ENTRY_ADD | DS_ENTRY_DELETE | \
				DS_ENTRY_RENAME | DS_ENTRY_SUPERVISOR | \
				DS_ENTRY_INHERIT_CTL)

#define DS_ATTR_COMPARE		0x00000001
#define DS_ATTR_READ		0x00000002
#define DS_ATTR_WRITE		0x00000004
#define DS_ATTR_SELF		0x00000008
#define DS_ATTR_SUPERVISOR	0x00000020
#define DS_ATTR_INHERIT_CTL	0x00000040

#define DS_ATTR_MASK		(DS_ATTR_COMPARE | DS_ATTR_READ | DS_ATTR_WRITE | \
				DS_ATTR_SELF | DS_ATTR_SUPERVISOR | \
				DS_ATTR_INHERIT_CTL)

typedef enum SYNTAX {
	SYN_UNKNOWN = 0,	/*  0 */
	SYN_DIST_NAME,		/*  1 */
	SYN_CE_STRING,		/*  2 */
	SYN_CI_STRING,		/*  3 */
	SYN_PR_STRING,		/*  4 */
	SYN_NU_STRING,		/*  5 */
	SYN_CI_LIST,		/*  6 */
	SYN_BOOLEAN,		/*  7 */
	SYN_INTEGER,		/*  8 */
	SYN_OCTET_STRING,	/*  9 */
	SYN_TEL_NUMBER,		/* 10 */
	SYN_FAX_NUMBER,		/* 11 */
	SYN_NET_ADDRESS,	/* 12 */
	SYN_OCTET_LIST,		/* 13 */
	SYN_EMAIL_ADDRESS,	/* 14 */
	SYN_PATH,		/* 15 */
	SYN_REPLICA_POINTER,	/* 16 */
	SYN_OBJECT_ACL,		/* 17 */
	SYN_PO_ADDRESS,		/* 18 */
	SYN_TIMESTAMP,		/* 19 */
	SYN_CLASS_NAME,		/* 20 */
	SYN_STREAM,		/* 21 */
	SYN_COUNTER,		/* 22 */
	SYN_BACK_LINK,		/* 23 */
	SYN_TIME,		/* 24 */
	SYN_TYPED_NAME,		/* 25 */
	SYN_HOLD,		/* 26 */
	SYN_INTERVAL,		/* 27 */
	SYNTAX_COUNT
} SYNTAX;

#ifndef SWIG

typedef struct {
	size_t			numOfBits;
	u_int8_t*		data;
} Bit_String_T;

typedef NWDSChar *		DN_T;		/*  1, SYN_DIST_NAME */
typedef NWDSChar *		CE_String_T;	/*  2, SYN_CE_STRING */
typedef NWDSChar *		CI_String_T;	/*  3, SYN_CI_STRING */
typedef NWDSChar *		PR_String_T;	/*  4, SYN_PR_STRING */
typedef NWDSChar *		NU_String_T;	/*  5, SYN_NU_STRING */
typedef struct _ci_list {
	struct _ci_list*	next;
	NWDSChar*		s;
} CI_List_T;					/*  6, SYN_CI_LIST */
typedef nuint8			Boolean_T;	/*  7, SYN_BOOLEAN */
typedef nint32a			Integer_T;	/*  8, SYN_INTEGER */
typedef struct {
	size_t			length;
	u_int8_t*		data;
} Octet_String_T;				/*  9, SYN_OCTET_STRING */
typedef NWDSChar *		TN_String_T;	/* 10, SYN_TEL_NUMBER */
typedef struct {
	NWDSChar*		telephoneNumber;
	Bit_String_T		parameters;
} Fax_Number_T;					/* 11, SYN_FAX_NUMBER */
typedef struct {
	enum NET_ADDRESS_TYPE	addressType;
	size_t			addressLength;
	u_int8_t*		address;
} Net_Address_T;				/* 12, SYN_NET_ADDRESS */
typedef struct _octet_list {
	struct _octet_list*	next;
	size_t			length;
	u_int8_t*		data;
} Octet_List_T;					/* 13, SYN_OCTET_LIST */
typedef struct {
	nuint32a		type;
	NWDSChar*		address;
} EMail_Address_T;				/* 14, SYN_EMAIL_ADDRESS */
typedef struct {
	nuint32a		nameSpaceType;
	NWDSChar*		volumeName;
	NWDSChar*		path;
} Path_T;					/* 15, SYN_PATH */
typedef struct {
	NWDSChar*		serverName;
	nint32a			replicaType;
	nint32a			replicaNumber;
	size_t			count;
	Net_Address_T		replicaAddressHint[1];
} Replica_Pointer_T;				/* 16, SYN_REPLICA_POINTER */
typedef struct {
	NWDSChar*		protectedAttrName;
	NWDSChar*		subjectName;
	nuint32a		privileges;
} Object_ACL_T;					/* 17, SYN_OBJECT_ACL */
typedef NWDSChar *
Postal_Address_T[6];				/* 18, SYN_PO_ADDRESS */
#endif	/* not SWIG */

typedef struct {
	nuint32a		wholeSeconds;
	nuint16a		replicaNum;
	nuint16a		eventID;
} TimeStamp_T;					/* 19, SYN_TIMESTAMP */

#ifndef SWIG
typedef NWDSChar *		Class_Name_T;	/* 20, SYN_CLASS_NAME */
typedef Octet_String_T		Stream_T;	/* 21, SYN_STREAM */
typedef nuint32a		Counter_T;	/* 22, SYN_COUNTER */
typedef struct {
	NWObjectID		remoteID;
	NWDSChar*		objectName;
} Back_Link_T;					/* 23, SYN_BACK_LINK */
typedef time_t			Time_T;		/* 24, SYN_TIME */
typedef struct {
	NWDSChar*		objectName;
	nuint32a		level;
	nuint32a		interval;
} Typed_Name_T;					/* 25, SYN_TYPED_NAME */
typedef struct {
	NWDSChar*		objectName;
	nuint32a		amount;
} Hold_T;					/* 26, SYN_HOLD */
#if 0	/* SYN_INTERVAL uses SYN_INTEGER directly ?! */
typedef nuint32a		Interval_T;	/* 27, SYN_INTERVAL */
#endif

#endif	/* not SWIG */

#ifdef SWIG
struct tagAsn1ID_T {
%pragma(swig) readonly
	size_t			length;
%pragma(swig) readwrite
	size_tLenPrefixCharArray data[MAX_ASN1_NAME];
};
typedef struct tagAsn1ID_T Asn1ID_T;
#else
typedef struct 
#ifdef SWIG_BUILD
tagAsn1ID_T
#endif
{
	size_t			length;
	nuint8			data[MAX_ASN1_NAME];
} Asn1ID_T;
#endif

typedef struct {
	nuint32a		attrFlags;
	enum SYNTAX		attrSyntaxID;
	nuint32			attrLower;
	nuint32			attrUpper;
	Asn1ID_T		asn1ID;
} Attr_Info_T;

typedef struct {
	nuint32a		classFlags;
	Asn1ID_T		asn1ID;
} Class_Info_T;

typedef struct {
	nuint32a		objectFlags;
	nuint32a		subordinateCount;
	time_t			modificationTime;
	/* we cannot make this type NWDSChar... */
	char			baseClass[MAX_SCHEMA_NAME_BYTES + 2];
} Object_Info_T;

struct tagBuf_T {
	nuint	operation;
	nuint	bufFlags;
	nuint8*	dataend;
	nuint8*	curPos;
	nuint8*	data;
	nuint8* allocend;
	nuint	cmdFlags;	/* NWDSRead, NWDSSearch */
	nuint32	dsiFlags;	/* NWDSSearch, NWDSReadObjectInfo */
	nuint8* attrCountPtr;	/* used by NWDSPutAttrName/NWDSPutChange */
	nuint8* valCountPtr;	/* used by NWDSPutAttrVal */
};

typedef struct tagBuf_T Buf_T;
typedef Buf_T *pBuf_T;
typedef pBuf_T *ppBuf_T;

typedef struct {
		nuint32 statsVersion;
		nuint32 noSuchEntry;
		nuint32 localEntry;
		nuint32 typeReferral;
		nuint32 aliasReferral;
		nuint32 requestCount;
		nuint32 requestDataSize;
		nuint32 replyDataSize;
		nuint32 resetTime;
		nuint32 transportReferral;
		nuint32 upReferral;
		nuint32 downReferral;
} NDSStatsInfo_T;

typedef nuint32a NWObjectCount;

/* we are using the same env names as the Caldera client (hope they won't mind) */
#define PREFER_TREE_ENV         "NWCLIENT_PREFERRED_TREE"
#define PREFER_CTX_ENV          "NWCLIENT_DEFAULT_NAME_CONTEXT"
#define PREFER_SRV_ENV          "NWCLIENT_PREFERRED_SERVER"
#define PREFER_USER_ENV         "NWCLIENT_DEFAULT_USER"

/*this one is an extra by PP for a possible autologon*/
#define PREFER_PWD_ENV          "NWCLIENT_DEFAULT_PASSWORD"

#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE NWDSInitRequester(void); /* temporary */ /* D */
NWDSCCODE NWCFragmentRequest(NWCONN_HANDLE conn, nuint32 verb, 
		nuint numRq, const NW_FRAGMENT* rq, 
		nuint numRp, NW_FRAGMENT* rp, size_t *replyLen);

NWDSCCODE NWDSCreateContextHandle(NWDSContextHandle* ctx);	/* D */
NWDSContextHandle NWDSCreateContext(void); /* obsolete */	/* D */
NWDSCCODE NWDSDuplicateContextHandle(NWDSContextHandle ctx,
		NWDSContextHandle* newctx);			/* D */
NWDSContextHandle NWDSDuplicateContext(NWDSContextHandle ctx); /* obsolete */	/* D */
NWDSCCODE NWDSFreeContext(NWDSContextHandle ctxToFree);		/* D */

NWDSCCODE NWDSGetContext2(NWDSContextHandle ctx, int key, 
		void* ptr, size_t len);
NWDSCCODE NWDSGetContext(NWDSContextHandle ctx, int key, void* ptr);
NWDSCCODE NWDSSetContext(NWDSContextHandle ctx, int key, const void* ptr);
NWDSCCODE NWDSSetTransport(NWDSContextHandle, size_t, const NET_ADDRESS_TYPE*);

NWDSCCODE NWDSAllocBuf(size_t Buf_T_size, pBuf_T *buf);		/* D */
NWDSCCODE NWDSFreeBuf(pBuf_T freebuf);				/* D */
NWDSCCODE NWDSInitBuf(NWDSContextHandle ctx, nuint verb, Buf_T* initBuf);	/* D */

NWDSCCODE NWDSComputeAttrValSize(NWDSContextHandle ctx, Buf_T* buf, 
		enum SYNTAX synt, size_t* size);		/* D */
NWDSCCODE NWDSGetAttrVal(NWDSContextHandle ctx, Buf_T* buf, enum SYNTAX synt,
		void* val);					/* D */
NWDSCCODE NWDSGetAttrCount(NWDSContextHandle ctx, Buf_T* buf, 
		NWObjectCount* count);				/* D */
NWDSCCODE NWDSGetAttrName(NWDSContextHandle ctx, Buf_T* buf, NWDSChar* name,
		NWObjectCount* valcount, enum SYNTAX* syntaxID);	/* D */
NWDSCCODE NWDSGetAttrValFlags(NWDSContextHandle ctx, Buf_T* buf, 
		nuint32* flags);					/* D */
NWDSCCODE NWDSGetAttrValModTime(NWDSContextHandle ctx, Buf_T* buf, 
		TimeStamp_T* stamp);				/* D */
NWDSCCODE NWDSPutAttrVal(NWDSContextHandle ctx, Buf_T* buf, 
		enum SYNTAX syntaxID, const void* attrVal_syn3);	/* Partially */
NWDSCCODE NWDSPutAttrName(NWDSContextHandle ctx, Buf_T* buf, 
		const NWDSChar* name);				/* D */
NWDSCCODE NWDSPutAttrNameAndVal(NWDSContextHandle ctx, Buf_T* buf, 
		const NWDSChar* name, enum SYNTAX syntaxID,
		const void* attrVal_syn4);				/* Partially */
NWDSCCODE NWDSPutChange(NWDSContextHandle ctx, Buf_T* buf, 
		nuint changeType, const NWDSChar* name);	/* D */
NWDSCCODE NWDSPutChangeAndVal(NWDSContextHandle ctx, Buf_T* buf,
		nuint changeType, const NWDSChar* name,
		enum SYNTAX syntaxID, const void* attrVal_syn5);	/* Partially */

NWDSCCODE NWDSGetObjectCount(NWDSContextHandle ctx, Buf_T* buf, 
		NWObjectCount* count);					/* D */
NWDSCCODE NWDSGetObjectName(NWDSContextHandle ctx, Buf_T* buf, NWDSChar* name, 
		NWObjectCount* attrs, Object_Info_T* oit);		/* D */
NWDSCCODE NWDSGetObjectNameAndInfo(NWDSContextHandle ctx, Buf_T* buf, 
		NWDSChar* name, NWObjectCount* attrs, char** info);
NWDSCCODE NWDSGetPartitionInfo(NWDSContextHandle ctx, Buf_T* buf,
		NWDSChar* partitionRoot, nuint32* replicaType);
NWDSCCODE NWDSGetPartitionExtInfoPtr(NWDSContextHandle ctx, Buf_T* buf,
		char** infoPtr, char** infoPtrEnd);
NWDSCCODE NWDSGetPartitionExtInfo(NWDSContextHandle ctx, char* infoPtr,
		char* infoPtrEnd, nflag32 infoFlag, 
		size_t* length, void* data);
NWDSCCODE NWDSGetServerName(NWDSContextHandle ctx, Buf_T* buf, 
		NWDSChar* serverDN, NWObjectCount* partCount);
NWDSCCODE NWDSGetDSIInfo(NWDSContextHandle ctx, void* buffer, size_t blen,
		nuint32 dsiflag, void* dst);

NWDSCCODE NWDSRemoveAllTypesW(NWDSContextHandle ctx, const wchar_t* src, 
		wchar_t* dst);
NWDSCCODE NWDSRemoveAllTypes(NWDSContextHandle ctx, const NWDSChar* src, 
		NWDSChar* dst);					/* D */
NWDSCCODE NWDSCanonicalizeNameW(NWDSContextHandle ctx, const wchar_t* src, 
		wchar_t* dst);
NWDSCCODE NWDSCanonicalizeName(NWDSContextHandle ctx, const NWDSChar* src, 
		NWDSChar* dst);					/* D */
NWDSCCODE NWDSAbbreviateNameW(NWDSContextHandle ctx, const wchar_t* src, 
		wchar_t* dst);
NWDSCCODE NWDSAbbreviateName(NWDSContextHandle ctx, const NWDSChar* src, 
		NWDSChar* dst);					/* D */

NWDSCCODE NWDSGetBinderyContext(NWDSContextHandle ctx,
		NWCONN_HANDLE conn, NWDSChar* binderyEmulationContext);	/* D */
NWDSCCODE __NWGetFileServerUTCTime(NWCONN_HANDLE conn,
		nuint32* timev, nuint32* OUTPUT, nuint32* flags, 
		nuint32* OUTPUT2, nuint32* OUTPUT3, nuint32* OUTPUT4, 
		nuint32* OUTPUT5);					/* D */
NWDSCCODE NWGetFileServerUTCTime(NWCONN_HANDLE conn, nuint32* timev);	/* D */
NWDSCCODE __NWTimeGetVersion(NWCONN_HANDLE conn, int req,
		void* buffer, size_t* len, size_t maxlen);
NWDSCCODE NWDSGetObjectHostServerAddress(NWDSContextHandle ctx, 
		const NWDSChar* objectName, NWDSChar* serverDN, 
		Buf_T* serverAddresses);				/* D */
NWDSCCODE NWDSGetServerDN(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWDSChar* name);					/* D */
NWDSCCODE NWDSGetServerAddress(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWObjectCount* addrcnt, Buf_T* addrBuf);		/* D */
NWDSCCODE NWDSGetServerAddress2(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWObjectCount* addrcnt, Buf_T* addrBuf);		/* D */
NWDSCCODE NWDSMapIDToName(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWObjectID ID, NWDSChar* name);				/* D */
NWDSCCODE NWDSMapNameToID(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		const NWDSChar* name, NWObjectID* ID);			/* D */

NWDSCCODE NWDSGetDSVerInfo(NWCONN_HANDLE conn, nuint32* dsVersion,
		nuint32* rootMostEntryDepth, char* sapName, nuint32* flags,
		wchar_t* treeName);					/* D */
NWDSCCODE NWDSGetNDSStatistics(NWDSContextHandle ctx, 
		const NWDSChar* serverDN, size_t statsInfoLen,
		NDSStatsInfo_T* statsInfo);				/* D */
NWDSCCODE NWDSReloadDS(NWDSContextHandle ctx, const NWDSChar* serverDN);	/* D */
NWDSCCODE NWDSResetNDSStatistics(NWDSContextHandle ctx, 
		const NWDSChar* serverDN);

NWDSCCODE __NWDSCompare(NWDSContextHandle ctx, NWCONN_HANDLE conn,
		NWObjectID objectID, Buf_T* buf, nbool8* matched);
NWDSCCODE __NWDSOpenStream(NWDSContextHandle ctx, const NWDSChar* objectName,
                const NWDSChar* attrName, nflag32 flags, NWCONN_HANDLE* rconn,
                char fileHandle[6], ncp_off64_t* fileSize);

NWDSCCODE NWDSAddObject(NWDSContextHandle ctx, const NWDSChar* name,
		nuint32* iterHandle, nbool8 more, Buf_T* buf);		/* D */
NWDSCCODE NWDSAuthenticateConn(NWDSContextHandle ctx, NWCONN_HANDLE conn);
NWDSCCODE NWDSChangeObjectPassword(NWDSContextHandle ctx, nflag32 flags,
		const NWDSChar* objectName, const char* oldPassword,
		const char* newPassword);
NWDSCCODE NWDSCloseIteration(NWDSContextHandle ctx, nuint32 iterHandle,
		nuint32 verb);
NWDSCCODE NWDSCompare(NWDSContextHandle ctx, const NWDSChar* name,
		Buf_T* buf, nbool8* matched);
NWDSCCODE NWDSExtSyncList(NWDSContextHandle ctx, const NWDSChar* objectName,
		const NWDSChar* className, const NWDSChar* subordinateName,
		nuint32* iterHandle, const TimeStamp_T* timeStamp,
		nuint onlyContainers, Buf_T* objectList);
NWDSCCODE NWDSExtSyncRead(NWDSContextHandle ctx, const NWDSChar* objectName,
		nuint infoType, nuint allAttrs, Buf_T* in_attrNames,
		nuint32* iterHandle, const TimeStamp_T* timeStamp,
		Buf_T* objectInfo);
NWDSCCODE NWDSExtSyncSearch(NWDSContextHandle ctx, const NWDSChar* objectName,
		nint scope, nuint searchAliases, Buf_T* in_filter, 
		const TimeStamp_T* timeStamp, nuint infoType, 
		nuint allAttrs, Buf_T* in_attrNames, nuint32* iterHandle, 
		NWObjectCount countObjectsToSearch, 
		NWObjectCount* countObjectsSearched,
		Buf_T* objectInfo);
NWDSCCODE NWDSGenerateObjectKeyPair2(NWDSContextHandle ctx, 
		const NWDSChar* objectName, NWObjectID pseudoID, size_t	pwdLen,
		const nuint8* pwdHash, nflag32 optionsFlag);
NWDSCCODE NWDSGenerateObjectKeyPair(NWDSContextHandle ctx, 
		const NWDSChar* objectName, const char* objectPassword, 
		nflag32 optionsFlag);
NWDSCCODE NWDSGetCountByClassAndName(NWDSContextHandle ctx,
		const NWDSChar* objectName, const NWDSChar* className,
		const NWDSChar* subordinateName, NWObjectCount* count);
NWDSCCODE NWDSGetEffectiveRights(NWDSContextHandle ctx,
		const NWDSChar* subjectName, const NWDSChar* objectName,
		const NWDSChar* attrName, nuint32* privileges);
NWDSCCODE NWDSList(NWDSContextHandle ctx, const NWDSChar* name,
		nuint32* iterHandle, Buf_T* objectList);		/* D */
NWDSCCODE NWDSListAttrsEffectiveRights(NWDSContextHandle ctx, 
		const NWDSChar* objectName, const NWDSChar* subjectName,
		nuint allAttrs, Buf_T* in_attrNames, nuint32* iterHandle,
		Buf_T* privilegeInfo);
NWDSCCODE NWDSListByClassAndName(NWDSContextHandle ctx, const NWDSChar* name,
		const NWDSChar* className, const NWDSChar* subordinateName,
		nuint32* iterHandle, Buf_T* objectList);
NWDSCCODE NWDSListContainers(NWDSContextHandle ctx, const NWDSChar* name,
		nuint32* iterHandle, Buf_T* objectList);
NWDSCCODE NWDSModifyDN(NWDSContextHandle ctx, const NWDSChar* oldName,
		const NWDSChar* newName, nuint deleteOldRDN);		/* D */
NWDSCCODE NWDSModifyObject(NWDSContextHandle ctx, const NWDSChar* name,
		nuint32* iterHandle, nbool8 more, Buf_T* buf);		/* D */
NWDSCCODE NWDSModifyRDN(NWDSContextHandle ctx, const NWDSChar* oldName,
		const NWDSChar* newRDN, nuint deleteOldRDN);		/* D */
NWDSCCODE NWDSMoveObject(NWDSContextHandle ctx, const NWDSChar* oldName,
		const NWDSChar* newParent, const NWDSChar* newRDN);	/* D */
NWDSCCODE NWDSOpenConnToNDSServer(NWDSContextHandle ctx, 
		const NWDSChar* serverDN, NWCONN_HANDLE* pconn);	/* D */
NWDSCCODE NWDSRead(NWDSContextHandle ctx, const NWDSChar* name,
		nuint infoType, nuint allAttrs, Buf_T* in_attrNames,
		nuint32* iterHandle, Buf_T* objectInfo);		/* D */
NWDSCCODE NWDSReadObjectDSIInfo(NWDSContextHandle ctx, const NWDSChar* name,
		size_t bufflen, void* buffer);
NWDSCCODE NWDSReadObjectInfo(NWDSContextHandle ctx, const NWDSChar* name,
		NWDSChar* distName, Object_Info_T* oit);		/* D */
NWDSCCODE NWDSRemoveObject(NWDSContextHandle ctx, const NWDSChar* name);	/* D */
NWDSCCODE NWDSResolveName(NWDSContextHandle ctx, const NWDSChar* name,
		NWCONN_HANDLE* conn, NWObjectID* ID);			/* D */
NWDSCCODE NWDSSearch(NWDSContextHandle ctx, const NWDSChar* objectName,
		nint scope, nuint searchAliases, Buf_T* in_filter, 
		nuint infoType, nuint allAttrs, Buf_T* in_attrNames,
		nuint32* iterHandle, NWObjectCount countObjectsToSearch,
		NWObjectCount* countObjectsSearched, Buf_T* objectInfo);
NWDSCCODE NWDSVerifyObjectPassword(NWDSContextHandle ctx, nflag32 flags,
		const NWDSChar* objectName, const char* objectPassword);

/* NWIsDSServer returns true/false instead of error code! */
NWDSCCODE NWIsDSServer(NWCONN_HANDLE conn, char* treename);		/* D */
NWDSCCODE NWIsDSServerW(NWCONN_HANDLE conn, wchar_t* treename);

NWDSCCODE NWDSAbortPartitionOperation(NWDSContextHandle ctx, 
		const NWDSChar* partitionRoot);
NWDSCCODE NWDSAddReplica(NWDSContextHandle ctx, const NWDSChar* serverDN,
		const NWDSChar* partitionRoot, nuint32 replicaType);
NWDSCCODE NWDSChangeReplicaType(NWDSContextHandle ctx, 
		const NWDSChar* partitionRoot, const NWDSChar* serverDN,
		nuint32 newReplicaType);
NWDSCCODE NWDSGetPartitionRoot(NWDSContextHandle ctx, 
		const NWDSChar* objectName, NWDSChar* partitionRoot);	/* D */
NWDSCCODE NWDSJoinPartitions(NWDSContextHandle ctx,
		const NWDSChar* subordinatePartition, nflag32 flags);	/* D */
NWDSCCODE NWDSListPartitions(NWDSContextHandle ctx, nuint32* iterHandle, 
		const NWDSChar* serverDN, Buf_T* partitions);
NWDSCCODE NWDSListPartitionsExtInfo(NWDSContextHandle ctx, nuint32* iterHandle, 
		const NWDSChar* serverDN, nflag32 dspFlags,
		Buf_T* partitions);
NWDSCCODE NWDSPartitionReceiveAllUpdates(NWDSContextHandle ctx,
		const NWDSChar* partitionRoot, const NWDSChar* serverDN);
NWDSCCODE NWDSPartitionSendAllUpdates(NWDSContextHandle ctx,
		const NWDSChar* partitionRoot, const NWDSChar* serverDN);
NWDSCCODE NWDSRemovePartition(NWDSContextHandle ctx, 
		const NWDSChar* partitionRoot);
NWDSCCODE NWDSRemoveReplica(NWDSContextHandle ctx, const NWDSChar* serverDN,
		const NWDSChar* partitionRoot);
NWDSCCODE NWDSRepairTimeStamps(NWDSContextHandle ctx, 
		const NWDSChar* partitionRoot);
NWDSCCODE NWDSSplitPartition(NWDSContextHandle ctx,
		const NWDSChar* subordinatePartition, nflag32 flags);	/* D */
NWDSCCODE NWDSSyncPartition(NWDSContextHandle ctx, const NWDSChar* serverDN,
		const NWDSChar* partitionRoot, nuint32 seconds);

NWDSCCODE NWDSBeginClassItem(NWDSContextHandle ctx, Buf_T* in_classItems);
NWDSCCODE NWDSDefineAttr(NWDSContextHandle ctx, const NWDSChar* attrName,
		const Attr_Info_T* attrInfo);
NWDSCCODE NWDSDefineClass(NWDSContextHandle ctx, const NWDSChar* className,
		const Class_Info_T* classInfo, Buf_T* in_classItems);
NWDSCCODE NWDSGetAttrDef(NWDSContextHandle ctx, Buf_T* in_attrDefs,
		NWDSChar* attrName, Attr_Info_T* attrInfo);		/* D */
NWDSCCODE NWDSGetClassDef(NWDSContextHandle ctx, Buf_T* in_classDefs,
		NWDSChar* className, Class_Info_T* classInfo);
NWDSCCODE NWDSGetClassDefCount(NWDSContextHandle ctx, Buf_T* in_classDefs,
		NWObjectCount* classDefCount);
NWDSCCODE NWDSGetClassItem(NWDSContextHandle ctx, Buf_T* in_classDefs,
		NWDSChar* item);
NWDSCCODE NWDSGetClassItemCount(NWDSContextHandle ctx, Buf_T* in_classDefs,
		NWObjectCount* itemCount);
NWDSCCODE NWDSListContainableClasses(NWDSContextHandle ctx,
		const NWDSChar* parentName, nuint32* iterHandle,
		Buf_T* containableClasses);
NWDSCCODE NWDSModifyClassDef(NWDSContextHandle ctx, const NWDSChar* className,
		Buf_T* in_optionalAttrs);
NWDSCCODE NWDSPutClassItem(NWDSContextHandle ctx, Buf_T* in_classDefs,
		const NWDSChar* itemName);
#ifdef SWIG
NWDSCCODE NWDSPutClassName(NWDSContextHandle ctx, Buf_T* in_classDefs,
		const NWDSChar* itemName);				/* D */
#else
#define NWDSPutClassName(ctx, classDefs, itemName) \
				NWDSPutClassItem(ctx, classDefs, itemName)
#endif
NWDSCCODE NWDSReadAttrDef(NWDSContextHandle ctx, nuint infoType,
		nuint allAttrs, Buf_T* in_attrNames, nuint32* iterHandle,
		Buf_T* attrDefs);					/* D */
NWDSCCODE NWDSReadClassDef(NWDSContextHandle ctx, nuint infoType, 
		nuint allClasses, Buf_T* in_classNames, nuint32* iterHandle,
		Buf_T* classDefs);
NWDSCCODE NWDSRemoveAttrDef(NWDSContextHandle ctx, const NWDSChar* attrName);
NWDSCCODE NWDSRemoveClassDef(NWDSContextHandle ctx, const NWDSChar* className);
NWDSCCODE NWDSSyncSchema(NWDSContextHandle ctx, const NWDSChar* serverDN,
		nuint32a delaySeconds);

struct _filter_node {
	struct _filter_node*	parent;
	struct _filter_node*	left;
	struct _filter_node*	right;
	void*			value;
	enum SYNTAX		syntax;
	nuint			token;
};
	
typedef struct {
	struct _filter_node*	fn;
	nuint			level;
	nuint32			expect;
} Filter_Cursor_T;

#define FTOK_END	0
#define FTOK_OR		1
#define FTOK_AND	2
#define FTOK_NOT	3
#define FTOK_LPAREN	4
#define FTOK_RPAREN	5
#define FTOK_AVAL	6
#define FTOK_EQ		7
#define FTOK_GE		8
#define FTOK_LE		9
#define FTOK_APPROX	10
#define FTOK_ANAME	14
#define FTOK_PRESENT	15
#define FTOK_RDN	16
#define FTOK_BASECLS	17
#define FTOK_MODTIME	18
#define FTOK_VALTIME	19

#define FBIT_END	(1 << FTOK_END)
#define FBIT_OR		(1 << FTOK_OR)
#define FBIT_AND	(1 << FTOK_AND)
#define FBIT_NOT	(1 << FTOK_NOT)
#define FBIT_LPAREN	(1 << FTOK_LPAREN)
#define FBIT_RPAREN	(1 << FTOK_RPAREN)
#define FBIT_AVAL	(1 << FTOK_AVAL)
#define FBIT_EQ		(1 << FTOK_EQ)
#define FBIT_GE		(1 << FTOK_GE)
#define FBIT_LE		(1 << FTOK_LE)
#define FBIT_APPROX	(1 << FTOK_APPROX)
#define FBIT_ANAME	(1 << FTOK_ANAME)
#define FBIT_PRESENT	(1 << FTOK_PRESENT)
#define FBIT_RDN	(1 << FTOK_RDN)
#define FBIT_BASECLS	(1 << FTOK_BASECLS)
#define FBIT_MODTIME	(1 << FTOK_MODTIME)
#define FBIT_VALTIME	(1 << FTOK_VALTIME)

#define FBIT_OPERAND	(FBIT_NOT | FBIT_LPAREN | FBIT_ANAME | FBIT_PRESENT | FBIT_RDN | FBIT_BASECLS | FBIT_MODTIME | FBIT_VALTIME)
#define FBIT_RELOP	(FBIT_EQ | FBIT_GE | FBIT_LE | FBIT_APPROX)
#define FBIT_BOOLOP	(FBIT_OR | FBIT_AND)

NWDSCCODE NWDSAddFilterToken(Filter_Cursor_T* cur, nuint token, const void* value, 
		enum SYNTAX syntax);
NWDSCCODE NWDSAllocFilter(Filter_Cursor_T** cur);
NWDSCCODE NWDSDelFilterToken(Filter_Cursor_T* cur, 
		void (*freeVal)(enum SYNTAX, void*));
NWDSCCODE NWDSPutFilter(NWDSContextHandle ctx, Buf_T* buf, 
		Filter_Cursor_T* cur, void (*freeVal)(enum SYNTAX, void*));
NWDSCCODE NWDSFreeFilter(Filter_Cursor_T* cur, 
		void (*freeVal)(enum SYNTAX, void*));

NWDSCCODE NWDSAddConnection(NWDSContextHandle __ctx, NWCONN_HANDLE __conn);


/***************************PP */

#define DS_SYNTAX_NAMES 0
#define DS_SYNTAX_DEFS 1

#define DS_STRING           0x0001
#define DS_SINGLE_VALUED    0x0002
#define DS_SUPPORT_ORDER    0x0004
#define DS_SUPPORT_EQUALS   0x0008
#define DS_IGNORE_CASE      0x0010
#define DS_IGNORE_SPACE     0x0020
#define DS_IGNORE_DASH      0x0040
#define DS_ONLY_DIGITS      0x0080
#define DS_ONLY_PRINTABLE   0x0100
#define DS_SIZEABLE         0x0200
#define DS_BITWISE_EQUALS   0x0400




typedef struct {
   enum SYNTAX	ID;
   char		defStr[MAX_SCHEMA_NAME_BYTES + 2];
   nuint16	flags;
} Syntax_Info_T;




NWDSCCODE NWDSReadSyntaxes(NWDSContextHandle ctx, nuint infoType, 
		nuint allSyntaxes, Buf_T* in_syntaxNames, 
		nuint32* iterHandle, Buf_T* syntaxDefs);

NWDSCCODE NWDSPutSyntaxName(NWDSContextHandle ctx, Buf_T* buf,
		const NWDSChar* syntaxName);

NWDSCCODE NWDSGetSyntaxDef(NWDSContextHandle ctx, Buf_T* buf,
		NWDSChar* syntaxName, Syntax_Info_T* syntaxDef);

NWDSCCODE NWDSGetSyntaxCount(NWDSContextHandle ctx, Buf_T* buf,
		NWObjectCount* syntaxCount);

NWDSCCODE NWDSReadSyntaxDef(NWDSContextHandle context, enum SYNTAX syntaxID,
		Syntax_Info_T* syntaxDef);

NWDSCCODE NWDSGetSyntaxID(NWDSContextHandle ctx, const NWDSChar* attrName,
		enum SYNTAX* syntaxID);

/************* end syntaxes**************/



NWDSCCODE NWDSScanConnsForTrees(NWDSContextHandle context, nuint numOfPtrs,
		nuint* numOfTrees, NWDSChar** treeBufPtrs);

NWDSCCODE NWDSScanForAvailableTrees(NWDSContextHandle context, 
		NWCONN_HANDLE connHandle, const char* scanFilter, 
		nint32* scanIndex, NWDSChar* treeName);

NWDSCCODE NWDSReturnBlockOfAvailableTrees(NWDSContextHandle context,
		NWCONN_HANDLE connHandle, const char* scanFilter,
		const void* lastBlocksString, const NWDSChar* endBoundString,
		nuint32 maxTreeNames, NWDSChar** arrayOfNames,
		nuint32* numberOfTrees, nuint32* totalUniqueTrees);

NWDSCCODE NWDSWhoAmI(NWDSContextHandle context, NWDSChar* myDN);

#ifdef __cplusplus
}
#endif
		
#endif	/* __NWNET_H__ */
