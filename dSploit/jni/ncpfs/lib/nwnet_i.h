/*
    nwnet_i.h
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

 */

#ifndef __NWNET_I_H__
#define __NWNET_I_H__

#include "config.h"

#include <string.h>
#include <ncp/nwnet.h>
#include "private/list.h"
#include "private/libncp-lock.h"
/*
#include <unicode.h>
*/
typedef u_int16_t unicode;

#ifdef HAVE_WCHAR_H
#include <wchar.h>
#endif
#ifndef HAVE_WCSCMP
int wcscmp(const wchar_t*, const wchar_t*);
#endif
#ifndef HAVE_WCSNCMP
int wcsncmp(const wchar_t*, const wchar_t*, size_t);
#endif
#ifndef HAVE_WCSCASECMP
int wcscasecmp(const wchar_t*, const wchar_t*);
#endif
#ifndef HAVE_WCSNCASECMP
int wcsncasecmp(const wchar_t*, const wchar_t*, size_t);
#endif
#ifndef HAVE_WCSCPY
wchar_t* wcscpy(wchar_t*, const wchar_t*);
#endif
#ifndef HAVE_WCSLEN
size_t wcslen(const wchar_t*);
#endif
#ifndef HAVE_WCSDUP
wchar_t* wcsdup(const wchar_t*);
#endif
#ifndef HAVE_WCSREV
wchar_t* wcsrev(wchar_t*);
#endif

size_t unilen(const unicode*);

#ifdef HAVE_ICONV
#include <iconv.h>
#endif

typedef struct {
#define MY_ICONV_INTERNAL	0
#define MY_ICONV_LIBC		1
	int	type;
	union {
#ifdef HAVE_ICONV
		iconv_t h;
#endif
		int (*proc)(const char** fa, size_t* fl, char** ta, size_t* tl);
	} lowlevel;
} *my_iconv_t;

my_iconv_t my_iconv_open(const char* a, const char* b);
int my_iconv_close(my_iconv_t handle);
int my_iconv(my_iconv_t, const char** fa, size_t* fl, char** ta, size_t* tl);
int iconv_external_to_wchar_t(const char* src, wchar_t* dst, size_t maxl);
int iconv_wchar_t_to_external(const wchar_t* src, char* dst, size_t maxl);

union __NWDSAuthInfo {
	struct {
		size_t		total;
		size_t		version;
		size_t		hdrlen;
		nuint8		logindata[8];
		size_t		name_len;
		size_t		privkey_len;
			      } header;
	nuint8			data[4];
};

typedef struct NWDSConnection {
	struct {
		wchar_t*	tree_name;
			      } dck;
	struct list_head	conns;
	struct list_head	contexts;
	union __NWDSAuthInfo*	authinfo;
} *NWDS_HANDLE;

struct RDNInfo {
	struct RDNEntry*	end;
	size_t			depth;
};

struct TreeList;

#define DCK_RDN		6	/* internal flag for NWDSGetContext */

#define DCK_TREELIST	17	/* internal... */

struct __NWDSContextHandle {
	struct {
		nuint32		flags;
		nuint32		confidence;
		struct {
		NWCONN_HANDLE	conn;
		u_int32_t	state;
		              } last_connection;
		char*		local_charset;
		nuint32		name_form;
		size_t		transports;
		nuint32*	transport_types;
		struct RDNInfo	rdn;
		wchar_t*	namectx;
		nuint32		dsi_flags;
		struct TreeList* tree_list;
		      } dck;
	struct {
		my_iconv_t	to;
		my_iconv_t	from;
		ncpt_mutex_t	tolock;
		ncpt_mutex_t	fromlock;
		      } xlate;
	NWDS_HANDLE		ds_connection;
	struct list_head	context_ring;
#define DCV_PRIV_AUTHENTICATING	0x00000001
	nuint32			priv_flags;	/* no DCK_* ... */
};

/************************************************************************
 *									*
 *  struct RDNInfo* internal functions					*
 *									*
 ************************************************************************/
NWDSCCODE __NWDSCreateRDN(struct RDNInfo* rdn, const wchar_t* dn,
		size_t* trailingDots);
void __NWDSDestroyRDN(struct RDNInfo* rdn);

/************************************************************************
 *									*
 *  NWDSContextHandle internal functions				*
 *									*
 ************************************************************************/
static inline NWDSCCODE NWDXIsValid(NWDS_HANDLE dsh) {
	return dsh ? 0 : ERR_NULL_POINTER;
}

static inline NWDSCCODE NWDSIsContextValid(NWDSContextHandle ctx) {
	return ctx ? 0 : ERR_BAD_CONTEXT;
}

NWDSCCODE __NWDSGetConnection(NWDSContextHandle ctx, NWCONN_HANDLE *conn);

/************************************************************************
 *									*
 *  Buf_T internal functions						*
 *									*
 ************************************************************************/
#define ROUNDBUFF(x) (((x) + 3) & ~3)
#define ROUNDPKT(x)  (((x) + 3) & ~3)

#define NWDSBUFT_AUTOSIZE	0x01000000
#define NWDSBUFT_ALLOCATED	0x02000000
#define NWDSBUFT_INPUT		0x04000000
#define NWDSBUFT_OUTPUT		0x08000000
void NWDSSetupBuf(Buf_T* buf, void* ptr, size_t len);
NWDSCCODE NWDSCreateBuf(Buf_T** buff, void* ptr, size_t len);
NWDSCCODE NWDSClearFreeBuf(Buf_T* buf);

static inline void NWDSBufStartPut(Buf_T* buf, nuint32 operation) {
	buf->operation = operation;
	buf->cmdFlags = 0;
	buf->dsiFlags = 0;
	buf->bufFlags |= NWDSBUFT_OUTPUT;
	buf->bufFlags &= ~NWDSBUFT_INPUT;
	buf->dataend = buf->allocend;
	buf->curPos = buf->data;
}

static inline void NWDSBufFinishPut(Buf_T* buf) {
	buf->dataend = buf->curPos;
	buf->curPos = buf->data;
}
	
static inline void NWDSBufSetDSIFlags(Buf_T* buf, nuint32 dsiFlags) {
	buf->dsiFlags = dsiFlags;
}

NWDSCCODE NWDSBufSetInfoType(Buf_T* buf, nuint32 type);

static inline NWDSCCODE NWDSBufGetLE32(Buf_T* buf, nuint32* val) {
	if (buf->curPos + sizeof(*val) <= buf->dataend) {
		*val = DVAL_LH(buf->curPos, 0);
		buf->curPos += sizeof(*val);
		return 0;
	} else {
		buf->curPos = buf->dataend;
		return ERR_BUFFER_EMPTY;
	}	
}

static inline NWDSCCODE NWDSBufPeekLE32(Buf_T* buf, size_t off, nuint32* val) {
	nuint8* q;
	
	q = buf->curPos + off;
	if (q + sizeof(*val) <= buf->dataend) {
		*val = DVAL_LH(q, 0);
		return 0;
	} else {
		return ERR_BUFFER_EMPTY;
	}
}

static inline void* NWDSBufPeekPtrLen(Buf_T* buf, size_t off, size_t len) {
	nuint8* q;

	q = buf->curPos + off;
	return (q + len <= buf->dataend)?q:NULL;
}
		
/* User ID is an abstract type :-) It is 4 bytes long, big-endian
   (to preserve 0x00000001 = [Supervisor]) */
static inline NWDSCCODE NWDSBufGetID(Buf_T* buf, NWObjectID* ID) {
	if (buf->curPos + 4 <= buf->dataend) {
		*ID = DVAL_HL(buf->curPos, 0);
		buf->curPos += 4;
		return 0;
	} else {
		buf->curPos = buf->dataend;
		return ERR_BUFFER_EMPTY;
	}
}

static inline NWDSCCODE NWDSBufGet(Buf_T* buf, void* buffer, size_t len) {
	if (buf->curPos + len <= buf->dataend) {
		memcpy(buffer, buf->curPos, len);
		buf->curPos += ROUNDPKT(len);
		return 0;
	} else {
		buf->curPos = buf->dataend;
		return ERR_BUFFER_EMPTY;
	}
}

static inline void* NWDSBufGetPtr(Buf_T* buf, size_t len) {
	if (buf->curPos + len <= buf->dataend) {
		void* r = buf->curPos;
		buf->curPos += ROUNDPKT(len);
		return r;
	} else {
		buf->curPos = buf->dataend;
		return NULL;
	}
}

static inline void* NWDSBufPeekPtr(Buf_T* buf) {
	if (buf->curPos <= buf->dataend)
		return buf->curPos;
	else
		return NULL;
}

static inline void NWDSBufSeek(Buf_T* buf, void* ptr) {
	buf->curPos = ptr;
}

static inline void* NWDSBufTell(Buf_T* buf) {
	return buf->curPos;
}

static inline NWDSCCODE NWDSBufPutLE32(Buf_T* buf, nuint32 val) {
	if (buf->curPos + sizeof(val) <= buf->dataend) {
		DSET_LH(buf->curPos, 0, val);
		buf->curPos += sizeof(val);
		return 0;
	} else {
		/* TODO: honor NWDSBUFT_AUTOSIZE */
		buf->curPos = buf->dataend;
		return ERR_BUFFER_FULL;
	}	
}

static inline void* NWDSBufPutPtr(Buf_T* buf, size_t len) {
	return NWDSBufGetPtr(buf, len);
}

static inline void* NWDSBufPutPtrLen(Buf_T* buf, size_t* len) {
	*len = buf->dataend - buf->curPos;
	return buf->curPos;
}

static inline void* NWDSBufRetrievePtrAndLen(Buf_T* buf, size_t* len) {
	return NWDSBufPutPtrLen(buf, len);
}

static inline void* NWDSBufRetrieve(Buf_T* buf, size_t* len) {
	*len = buf->curPos - buf->data;
	return buf->data;
}

/* len <= free space !!! */
static inline void NWDSBufPutSkip(Buf_T* buf, size_t len) {
	buf->curPos += ROUNDPKT(len);
}

static inline void NWDSBufGetSkip(Buf_T* buf, size_t len) {
	buf->curPos += ROUNDPKT(len);
}

static inline NWDSCCODE NWDSBufPut(Buf_T* buf, const void* buffer, size_t len) {
	if (buf->curPos + len <= buf->dataend) {
		if (len & 3)
			*(nuint32*)(buf->curPos + (len & ~3)) = 0;
		memcpy(buf->curPos, buffer, len);
		buf->curPos += ROUNDPKT(len);
		return 0;
	} else {
		/* TODO: honor NWDSBUFT_AUTOSIZE */
		return ERR_BUFFER_FULL;
	}
}

/* put buffer length & buffer */
NWDSCCODE NWDSBufPutBuffer(Buf_T* buf, const void* buffer, size_t len);

/* skips length-preceeded item in buf */
NWDSCCODE NWDSBufSkipBuffer(Buf_T* buf);

/************************************************************************
 *                                                                      *
 *  NWDS internal functions                                             *
 *                                                                      *
 ************************************************************************/
 
/* Converts name from context format to wchar_t and canonicalizes it */
NWDSCCODE NWDSGetCanonicalizedName(
		NWDSContextHandle ctx,
		const NWDSChar* src,
		wchar_t* dst);
		
/* Converts name from unicode to wchar_t; srclen must be correct */
NWDSCCODE NWDSPtrDN(
		const unicode* src,
		size_t srclen,
		wchar_t* dst,
		size_t maxlen);

NWDSCCODE NWDSBufDN(
		Buf_T* buffer,
		wchar_t* name,
		size_t maxlen);

NWDSCCODE NWDSBufCtxDN(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		void* dst,
		size_t* ln);

NWDSCCODE NWDSBufCtxString(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		void* dst,
		size_t maxLen,
		size_t* realLen);

NWDSCCODE NWDSCtxBufDN(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		const NWDSChar* objectName);

NWDSCCODE NWDSCtxBufString(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		const NWDSChar* string);

NWDSCCODE NWDSPutAttrVal_XX_STRING(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		const NWDSChar* name);
		
NWDSCCODE NWDSPutAttrVal_TIMESTAMP(
		NWDSContextHandle ctx,
		Buf_T* buffer,
		const TimeStamp_T* stamp);
						
NWDSCCODE NWDSXlateFromCtx(NWDSContextHandle ctx, wchar_t* dst,
		size_t maxlen, const void* src);
		
NWDSCCODE NWDSXlateToCtx(NWDSContextHandle ctx, void* data, 
		size_t maxlen, const wchar_t* src, size_t* ln);

NWDSCCODE NWDSSetTreeNameW(NWDSContextHandle ctx, const wchar_t* treename);

/************************************************************************
 *									*
 *  NWDS internal server calls						*
 *									*
 ************************************************************************/
#define DS_RESOLVE_V0			     0

#define DS_RESOLVE_ENTRY_ID		0x0001
#define DS_RESOLVE_READABLE		0x0002
#define DS_RESOLVE_WRITEABLE		0x0004
#define DS_RESOLVE_MASTER		0x0008
#define DS_RESOLVE_CREATE_ID		0x0010
#define DS_RESOLVE_WALK_TREE		0x0020
#define DS_RESOLVE_DEREF_ALIASES	0x0040
/*                                      0x0080 */
#define DS_RESOLVE_ENTRY_EXISTS		0x0100
#define DS_RESOLVE_DELAYED_CREATE_ID	0x0200
#define DS_RESOLVE_EXHAUST_REPLICAS	0x0400
#define DS_RESOLVE_ACCEPT_NOT_PRESENT	0x0800
#define DS_RESOLVE_FORCE_REFERRALS	0x1000
#define DS_RESOLVE_PREFFER_REFERRALS	0x2000
#define DS_RESOLVE_ONLY_REFERRALS	0x4000
/* Entry ID?                            0x8000 */

#define DS_RESOLVE_REPLY_LOCAL_ENTRY	0x0001
#define DS_RESOLVE_REPLY_REMOTE_ENTRY	0x0002
#define DS_RESOLVE_REPLY_ALIAS		0x0003
#define DS_RESOLVE_REPLY_REFERRAL	0x0004
#define DS_RESOLVE_REPLY_REFERRAL_AND_ENTRY 0x0006

/* one step of resolving name */
NWDSCCODE NWDSResolveNameInt(
		NWDSContextHandle ctx,
		NWCONN_HANDLE conn,
		u_int32_t version,
		u_int32_t flag,
		const NWDSChar* name,
		Buf_T* reply);

/* resolve name, do not deref aliases */
NWDSCCODE NWDSResolveName2DR(
		NWDSContextHandle ctx,
		const NWDSChar* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID);

/* resolve name */
NWDSCCODE NWDSResolveName2(
		NWDSContextHandle ctx, 
		const NWDSChar* name, 
		u_int32_t flag,
		NWCONN_HANDLE* resconn, 
		NWObjectID* ID);

/* resolve unicode name */
NWDSCCODE __NWDSResolveName2u(
		NWDSContextHandle ctx,
		const unicode* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID);

/* resolve wchar_t name */
NWDSCCODE __NWDSResolveName2w(
		NWDSContextHandle ctx,
		const wchar_t* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID);

/* split name to parent and RDN, resolve parent */
NWDSCCODE __NWDSResolveName2p(
		NWDSContextHandle ctx,
		const NWDSChar* name,
		u_int32_t flag,
		NWCONN_HANDLE* resconn,
		NWObjectID* ID,
		wchar_t* childName);

/* read object attributes/attribute values */
NWDSCCODE __NWDSReadV1(
		NWCONN_HANDLE conn,
		nuint32 qflags,
		NWObjectID objID,
		nuint infoType,
		nuint allAttrs,
		Buf_T* attrNames,
		nuint32* iterhandle,
		Buf_T* subjectName,
		Buf_T* objectInfo);

/* retrieve server DN in wchar_t */
NWDSCCODE __NWDSGetServerDN(
		NWCONN_HANDLE conn,
		wchar_t* name,
		size_t maxlen);

/* retrieve object DN in wchar_t */
NWDSCCODE __NWDSGetObjectDN(
		NWCONN_HANDLE conn,
		NWObjectID id,
		wchar_t* name,
		size_t maxlen);

/* retrieve object DN in unicode */
NWDSCCODE __NWDSGetObjectDNUnicode(
		NWCONN_HANDLE conn,
		NWObjectID id,
		unicode* name,
		size_t* len);

/* begin login */
NWDSCCODE __NWDSBeginLoginV0(
		NWCONN_HANDLE conn,
		NWObjectID objID,
		NWObjectID *p1,
		void *p2);

/* finish login */
NWDSCCODE __NWDSFinishLoginV2(
		NWCONN_HANDLE conn,
		nuint32 flag,
		NWObjectID objID,
		Buf_T* rqb,
		nuint8 p1[8],
		Buf_T* rpb);

/* begin connection authentication */
NWDSCCODE __NWDSBeginAuthenticationV0(
		NWCONN_HANDLE conn,
		NWObjectID objID,
		const nuint8 seed[4],
		nuint8 authid[4],
		Buf_T* rpb);

/* end connection authntication */
NWDSCCODE __NWDSFinishAuthenticationV0(
		NWCONN_HANDLE	conn,
		Buf_T*		md5_key,
		const void*	login_identity,
		size_t		login_identity_len,
		Buf_T*		auth_key);

/* closes lowlevel iteration handle */
NWDSCCODE __NWDSCloseIterationV0(
		NWCONN_HANDLE	conn,
		nuint32		iterHandle,
		nuint32		verb);

NWDSCCODE __NWDSReadObjectDSIInfo(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		NWObjectID		objectID,
		nuint32			dsiFlags,
		Buf_T*			dsiInfo);

NWDSCCODE __NWDSReadObjectInfo(
		NWDSContextHandle	ctx,
		NWCONN_HANDLE		conn,
		NWObjectID		id,
		NWDSChar*		distname,
		Object_Info_T*		oi);

NWDSCCODE NWDSDuplicateContextHandleInt(
		NWDSContextHandle	srcctx,
		NWDSContextHandle*	wctx);

#include "iterhandle.h"

#endif	/* __NWNET_I_H__ */
