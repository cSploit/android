/*
    eas.c
    Copyright (C) 2000  Petr Vandrovec

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

    Revision History:

	1.00  2000, August 26		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

 */

#include <ncp/eas.h>
#include "ncplib_i.h"

#include <string.h>
#include <errno.h>
#include <stdlib.h>

NWCCODE ncp_ea_close(NWCONN_HANDLE conn, uint_fast32_t EAHandle) {
	nuint8	rq[7];
	
	BSET   (rq, 0, 1);
	WSET_LH(rq, 1, 0);
	DSET_LH(rq, 3, EAHandle);
	
	return NWRequestSimple(conn, 0x56, rq, sizeof(rq), NULL);
}

NWCCODE ncp_ea_duplicate(NWCONN_HANDLE conn,
		uint_fast16_t srcFlags,
		uint_fast32_t srcHandleOrVol, uint_fast32_t srcZeroOrDir,
		uint_fast16_t dstFlags,
		uint_fast32_t dstHandleOrVol, uint_fast32_t dstZeroOrDir,
		u_int32_t* duplicateCount,
		u_int32_t* dataSizeDuplicated,
		u_int32_t* keySizeDuplicated) {
	NWCCODE err;
	NW_FRAGMENT rp_frag;
	nuint8	rq[1+4+8+8];
	nuint8  rp[12];
	
	BSET   (rq,  0, 5);
	WSET_LH(rq,  1, srcFlags);
	WSET_LH(rq,  3, dstFlags);
	DSET_LH(rq,  5, srcHandleOrVol);
	DSET_LH(rq,  9, srcZeroOrDir);
	DSET_LH(rq, 13, dstHandleOrVol);
	DSET_LH(rq, 17, dstZeroOrDir);
	rp_frag.fragSize = sizeof(rp);
	rp_frag.fragAddr.rw = rp;
	
	err = NWRequestSimple(conn, 0x56, rq, sizeof(rq), &rp_frag);
	if (err)
		return err;
	if (rp_frag.fragSize < 12)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (duplicateCount)
		*duplicateCount = DVAL_LH(rp, 0);
	if (dataSizeDuplicated)
		*dataSizeDuplicated = DVAL_LH(rp, 4);
	if (keySizeDuplicated)
		*keySizeDuplicated = DVAL_LH(rp, 8);
	return 0;
}

NWCCODE ncp_ea_enumerate(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t inspectSize,
		const void* key, size_t keyLen,
		struct ncp_ea_enumerate_info* info,
		void* data, size_t datalen, size_t* rdatalen) {
	NWCCODE err;
	void* ptr;

	if (keyLen && !key)
		return NWE_PARAM_INVALID;
	if (!info)
		return NWE_PARAM_INVALID;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 4);
	ncp_add_word_lh(conn, flags);
	ncp_add_dword_lh(conn, handleOrVol);
	ncp_add_dword_lh(conn, zeroOrDir);
	ncp_add_dword_lh(conn, inspectSize);
	ncp_add_word_lh(conn, info->enumSequence);
	ncp_add_word_lh(conn, keyLen);
	if (keyLen)
		ncp_add_mem(conn, key, keyLen);
	err = ncp_request(conn, 0x56);
	if (!err) {
		if (conn->ncp_reply_size < 24) {
			err = NWE_INVALID_NCP_PACKET_LENGTH;
		} else {
			size_t ln;
			
			ptr = ncp_reply_data(conn, 0);
			info->errorCode = DVAL_LH(ptr, 0);
			info->totalEAs = DVAL_LH(ptr, 4);
			info->totalEAsDataSize = DVAL_LH(ptr, 8);
			info->totalEAsKeySize = DVAL_LH(ptr, 12);
			info->newEAhandle = DVAL_LH(ptr, 16);
			info->enumSequence = WVAL_LH(ptr, 20);
			info->returnedItems = WVAL_LH(ptr, 22);
			
			ln = conn->ncp_reply_size - 24;
			if (data) {
				if (ln > datalen) {
					ln = datalen;
					err = NWE_BUFFER_OVERFLOW;
				}
				memcpy(data, ncp_reply_data(conn, 24), ln);
			}
			if (rdatalen)
				*rdatalen = ln;
		}
	}
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE ncp_ea_read(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t inspectSize,
		const void* key, size_t keyLen,
		uint_fast32_t readOffset,
		struct ncp_ea_read_info* info,
		void* data, size_t datalen, size_t* rdatalen) {
	NWCCODE err;
	void* ptr;

	if (keyLen && !key)
		return NWE_PARAM_INVALID;
	if (!info)
		return NWE_PARAM_INVALID;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 3);
	ncp_add_word_lh(conn, flags);
	ncp_add_dword_lh(conn, handleOrVol);
	ncp_add_dword_lh(conn, zeroOrDir);
	ncp_add_dword_lh(conn, readOffset);
	ncp_add_dword_lh(conn, inspectSize);
	ncp_add_word_lh(conn, keyLen);
	if (keyLen)
		ncp_add_mem(conn, key, keyLen);
	err = ncp_request(conn, 0x56);
	if (!err) {
		if (conn->ncp_reply_size < 18) {
			err = NWE_INVALID_NCP_PACKET_LENGTH;
		} else {
			size_t ln;
			
			ptr = ncp_reply_data(conn, 0);
			info->errorCode = DVAL_LH(ptr, 0);
			info->totalValuesLength = DVAL_LH(ptr, 4);
			info->newEAhandle = DVAL_LH(ptr, 8);
			info->accessFlag = DVAL_LH(ptr, 12);
			ln = WVAL_LH(ptr, 16);
			if (ln + 18 > conn->ncp_reply_size) {
				err = NWE_INVALID_NCP_PACKET_LENGTH;
			} else {
				if (data) {
					if (ln > datalen) {
						ln = datalen;
						err = NWE_BUFFER_OVERFLOW;
					}
					memcpy(data, ncp_reply_data(conn, 18), ln);
				}
				if (rdatalen)
					*rdatalen = ln;
			}
		}
	}
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE ncp_ea_write(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t totalWriteSize,
		const void* key, size_t keyLen,
		uint_fast32_t writeOffset,
		uint_fast32_t accessFlag,
		struct ncp_ea_write_info* info,
		const void* data, size_t datalen) {
	NWCCODE err;
	void* ptr;

	if (keyLen && !key)
		return NWE_PARAM_INVALID;
	if (!info)
		return NWE_PARAM_INVALID;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 2);
	ncp_add_word_lh(conn, flags);
	ncp_add_dword_lh(conn, handleOrVol);
	ncp_add_dword_lh(conn, zeroOrDir);
	ncp_add_dword_lh(conn, totalWriteSize);
	ncp_add_dword_lh(conn, writeOffset);
	ncp_add_dword_lh(conn, accessFlag);
	ncp_add_word_lh(conn, datalen);
	ncp_add_word_lh(conn, keyLen);
	if (keyLen)
		ncp_add_mem(conn, key, keyLen);
	if (datalen)
		ncp_add_mem(conn, data, datalen);
	err = ncp_request(conn, 0x56);
	if (!err) {
		if (conn->ncp_reply_size < 12) {
			err = NWE_INVALID_NCP_PACKET_LENGTH;
		} else {
			ptr = ncp_reply_data(conn, 0);
			info->errorCode = DVAL_LH(ptr, 0);
			info->written = DVAL_LH(ptr, 4);
			info->newEAhandle = DVAL_LH(ptr, 8);
		}
	}
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE ncp_ea_extract_info_level1(const unsigned char* buffer,
		const unsigned char* endbuf,
		struct ncp_ea_info_level1* info, size_t maxsize, 
		size_t* needsize, const unsigned char** next) {
	size_t len;
	size_t usedsize;
	const unsigned char* nptr;
		
	if (next)
		*next = NULL;
	if (!buffer)
		return NWE_PARAM_INVALID;
	if (buffer + 10 > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	len = WVAL_LH(buffer, 4);
	nptr = buffer + 10 + len;
	if (nptr > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (next)
		*next = nptr;
	usedsize = sizeof(*info) - 256 + len + 1;
	if (needsize)
		*needsize = usedsize;
	if (info) {
		if (maxsize < usedsize)
			return NWE_BUFFER_OVERFLOW;
		info->keyLength = len;
		info->valueLength = DVAL_LH(buffer, 0);
		info->accessFlag = DVAL_LH(buffer, 6);
		memcpy(info->key, buffer + 10, len);
		info->key[len] = 0;
	}
	return 0;
}

NWCCODE ncp_ea_extract_info_level6(const unsigned char* buffer,
		const unsigned char* endbuf,
		struct ncp_ea_info_level6* info, size_t maxsize, 
		size_t* needsize, const unsigned char** next) {
	size_t len;
	size_t usedsize;
	const unsigned char* nptr;
		
	if (next)
		*next = NULL;
	if (!buffer)
		return NWE_PARAM_INVALID;
	if (buffer + 18 > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	len = WVAL_LH(buffer, 4);
	nptr = buffer + 18 + len;
	if (nptr > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (next)
		*next = nptr;
	usedsize = sizeof(*info) - 256 + len + 1;
	if (needsize)
		*needsize = usedsize;
	if (info) {
		if (maxsize < usedsize)
			return NWE_BUFFER_OVERFLOW;
		info->keyLength = len;
		info->valueLength = DVAL_LH(buffer, 0);
		info->accessFlag = DVAL_LH(buffer, 6);
		info->keyExtants = DVAL_LH(buffer, 10);
		info->valueExtants = DVAL_LH(buffer, 14);
		memcpy(info->key, buffer + 18, len);
		info->key[len] = 0;
	}
	return 0;
}

NWCCODE ncp_ea_extract_info_level7(const unsigned char* buffer,
		const unsigned char* endbuf,
		char* key, size_t maxsize, size_t* needsize,
		const unsigned char** next) {
	size_t len;
	size_t usedsize;
	const unsigned char* nptr;
		
	if (next)
		*next = NULL;
	if (!buffer)
		return NWE_PARAM_INVALID;
	if (buffer + 2 > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	len = BVAL(buffer, 0);
	nptr = buffer + 2 + len;
	if (nptr > endbuf)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (next)
		*next = nptr;
	usedsize = len + 1;
	if (needsize)
		*needsize = usedsize;
	if (key) {
		if (maxsize < usedsize)
			return NWE_BUFFER_OVERFLOW;
		memcpy(key, buffer + 1, len);
		key[len] = 0;
	}
	return 0;
}
