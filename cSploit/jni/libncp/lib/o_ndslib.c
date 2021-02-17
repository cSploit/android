/*
    NDS client for ncpfs
    Copyright (C) 1997  Arne de Bruijn

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
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Revision history:
    
	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
    						Mark these calls obsolete.
						Modify them to use new API.

*/

#define NCP_OBSOLETE

extern int bindery_only;

#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/time.h>
#include <fcntl.h>
#include <errno.h>

#include "nwnet_i.h"

int strlen_u(const uni_char *s) {
	return unilen(s);
}

uni_char getchr_u(const uni_char* s) {
	return WVAL_LH((const void*)s, 0);
}

void strcpy_uc(char *d, const uni_char *s) {
	while ((*d++ = getchr_u(s++)) != 0);
}

void strcpy_cu(uni_char *d, const char *s) {
	do {
		WSET_LH((void*)(d++), 0, *s);
	} while (*s++);
}

long nds_get_server_name(struct ncp_conn *conn, uni_char **server_name) {
	NWDSContextHandle ctx;
	NWDSCCODE err;
	void* outbuf;
	u_int32_t flags;
	
	outbuf = malloc(4096);
	if (!outbuf)
		return ENOMEM;
	err = NWDSCreateContextHandle(&ctx);
	if (err)
		goto q_mem;
	flags = 0;
	err = NWDSSetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		goto q_ctx;
	err = NWDSGetServerDN(ctx, conn, outbuf);
	if (!err) {
		*server_name = outbuf;
		NWDSFreeContext(ctx);
		return 0;
	}
q_ctx:;
	NWDSFreeContext(ctx);
q_mem:;
	free(outbuf);
	return err;
}

long nds_get_tree_name(struct ncp_conn *conn, char *name, int name_buf_len) {
	char buf[MAX_TREE_NAME_CHARS+1];
   
	if (bindery_only) return -1;
 
 	if (!NWIsDSServer(conn, buf))
		return -1;
	if (name) {
		size_t size;
		char* p;
		
		p = strchr(buf, '\0') - 1;
		while ((p >= buf) && (*p == '_'))
			p--;
		size = p - buf + 1;
		if (size >= (size_t)name_buf_len)
			return -1;
		memcpy(name, buf, size);
		name[size] = 0;
	}
	return 0;
}

/* for login */
long nds_resolve_name(struct ncp_conn *conn, int flags, uni_char *entry_name, 
 int *entry_id, int *remote, UNUSED(struct sockaddr *serv_addr), size_t *addr_len) {
	NWDSCCODE err;
	NWDSContextHandle ctx;
	u_int32_t ctxflags;
	u_int32_t replytype;
	Buf_T* rp;
	
	err = NWDSCreateContextHandle(&ctx);
	if (err)
		return err;
	flags = 0;
	err = NWDSSetContext(ctx, DCK_FLAGS, &ctxflags);
	if (err)
		goto q_ctx;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &rp);
	if (err)
		goto q_ctx;
	err = NWDSResolveNameInt(ctx, conn, DS_RESOLVE_V0, flags, (const NWDSChar*)entry_name, rp);
	if (err)
		goto q_buf;
	err = NWDSBufGetLE32(rp, &replytype);
	if (err)
		goto q_buf;
	switch (replytype) {
		case DS_RESOLVE_REPLY_LOCAL_ENTRY:
			{
				NWObjectID ID;
				
				err = NWDSBufGetID(rp, &ID);
				if (!err) {
					if (entry_id)
						*entry_id = ID;
					if (remote)
						*remote = 0;
				}
			}
			break;
		case DS_RESOLVE_REPLY_REMOTE_ENTRY:
			{
				NWObjectID ID;
				
				err = NWDSBufGetID(rp, &ID);
				if (err)
					goto q_buf;
				if (entry_id)
					*entry_id = ID;
				err = NWDSBufGetLE32(rp, &flags);
				if (err)
					goto q_buf;
				if (remote)
					*remote = 1;
				/* we do not support address passing... use NWDSResolveName instead */
				if (addr_len)
					*addr_len = 0;
				break;
			}
			break;
		default:
			err = ERR_INVALID_SERVER_RESPONSE;
			break;
	}
q_buf:;
	NWDSFreeBuf(rp);
q_ctx:;
	NWDSFreeContext(ctx);
	return err;
}

long nds_read(struct ncp_conn *conn, u_int32_t obj_id, uni_char *propname, 
		u_int32_t* type, void **outbuf, size_t *outlen) {
	NWDSContextHandle ctx;
	NWDSCCODE err;
	u_int32_t flags;
	Buf_T* attr;
	Buf_T* rp;
	u_int32_t iterHandle = NO_MORE_ITERATIONS;
	NWObjectCount cnt;
	enum SYNTAX synt;
	size_t size;
	Octet_String_T* buf;

	err = NWDSCreateContextHandle(&ctx);
	if (err)
		return err;
	flags = 0;
	err = NWDSSetContext(ctx, DCK_FLAGS, &flags);
	if (err)
		goto q_ctx;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &attr);
	if (err)
		goto q_ctx;
	err = NWDSInitBuf(ctx, DSV_READ, attr);
	if (err)
		goto q_attr;
	err = NWDSPutAttrName(ctx, attr, (const NWDSChar*)propname);
	if (err)
		goto q_attr;
	err = NWDSAllocBuf(DEFAULT_MESSAGE_LEN, &rp);
	if (err)
		goto q_attr;
	err = __NWDSReadV1(conn, 0, obj_id, DS_ATTRIBUTE_VALUES, 0, attr, &iterHandle, NULL, rp);
	if (err)
		goto q_rp;
	err = NWDSGetAttrCount(ctx, rp, &cnt);
	if (err)
		goto q_rp;
	/* we asked for one attribute, so we expect one attribute */
	if (cnt != 1) {
		err = ERR_INVALID_SERVER_RESPONSE;
		goto q_rp;
	}
	err = NWDSGetAttrName(ctx, rp, NULL, &cnt, &synt);
	if (err)
		goto q_rp;
	/* we do not support multivalued attributes */
	if (cnt != 1) {
		err = ERR_INVALID_SERVER_RESPONSE;
		goto q_rp;
	}
	if (type)
		*type = synt;
	/* retrieve it as raw string */
	err = NWDSComputeAttrValSize(ctx, rp, SYN_OCTET_STRING, &size);
	if (err)
		goto q_rp;
	buf = (Octet_String_T*)malloc(size);
	if (!buf) {
		err = ENOMEM;
		goto q_rp;
	}
	err = NWDSGetAttrVal(ctx, rp, SYN_OCTET_STRING, buf);
	if (err) {
		free(buf);
		goto q_rp;
	}
	if (outlen)
		*outlen = buf->length;
	/* move buffer to the start of allocated region */
	if (outbuf) {
		*outbuf = buf;
		memmove(buf, buf->data, buf->length);
	} else {
		free(buf);
	}
q_rp:;
	NWDSFreeBuf(rp);
q_attr:
	NWDSFreeBuf(attr);
q_ctx:
	NWDSFreeContext(ctx);
	return err;
}

/* this lives in ndslib.c... for now...
long nds_login_auth(
		struct ncp_conn *conn,
		const char *user,
		const char *pwd);
*/

