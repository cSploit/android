/*
    filelock.c
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

    Revision History:

	1.00  2002, July 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Initial release.

 */

#include "ncplib_i.h"

#include <ncp/nwnet.h>
#include <errno.h>

static NWCCODE ncp_log_physical_record32(NWCONN_HANDLE conn, const char fh[6],
		u_int32_t startOffset, u_int32_t length, unsigned int flags,
		unsigned int timeout) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, flags);
	ncp_add_mem(conn, fh, 6);
	ncp_add_dword_hl(conn, startOffset);
	ncp_add_dword_hl(conn, length);
	ncp_add_word_hl(conn, timeout);
	err = ncp_request(conn, 109);
	ncp_unlock_conn(conn);
	return err;
}

static NWCCODE ncp_log_physical_record64(NWCONN_HANDLE conn, u_int32_t fh,
		ncp_off64_t startOffset, u_int64_t length, unsigned int flags,
		unsigned int timeout) {
	NWCCODE err;

	ncp_init_request(conn);
	ncp_add_byte(conn, 67);
	ncp_add_dword_lh(conn, flags);
	ncp_add_dword_lh(conn, fh);
	ncp_add_qword_hl(conn, startOffset);
	ncp_add_qword_hl(conn, length);
	ncp_add_dword_hl(conn, timeout);
	err = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return err;
}

NWCCODE ncp_log_physical_record(NWCONN_HANDLE conn, const char file_handle[6],
		ncp_off64_t startOffset, u_int64_t length, unsigned int flags,
		unsigned int timeout) {
	NWCCODE err;
	
	if (!conn || !file_handle) {
		return ERR_NULL_POINTER;
	}
	err = __NWReadFileServerInfo(conn);
	if (err) {
		return err;
	}
	if (conn->serverInfo.ncp64bit) {
		err = ncp_log_physical_record64(conn, ConvertToDWORDfromNW(file_handle),
				startOffset, length, flags, timeout);
	} else {
		if ((flags & ~0xFF) || (timeout & ~0xFFFF)) {
			return EINVAL;
		}
		if (startOffset >= 0x100000000ULL ||
		    length >= 0x100000000ULL ||
		    startOffset + length >= 0x100000000ULL) {
			return EFBIG;
		}
		err = ncp_log_physical_record32(conn, file_handle,
				startOffset, length, flags, timeout);
	}
	return err;
}


static NWCCODE ncp_clear_release_physical_record32(NWCONN_HANDLE conn, const char fh[6],
		u_int32_t startOffset, u_int32_t length, int release) {
	NWCCODE err;
	
	ncp_init_request(conn);
	ncp_add_mem(conn, fh, 6);
	ncp_add_dword_hl(conn, startOffset);
	ncp_add_dword_hl(conn, length);
	if (release) {
		err = ncp_request(conn, 28);
	} else {
		err = ncp_request(conn, 30);
	}
	ncp_unlock_conn(conn);
	return err;
}

static NWCCODE ncp_clear_release_physical_record64(NWCONN_HANDLE conn, u_int32_t fh,
		ncp_off64_t startOffset, u_int64_t length, int release) {
	NWCCODE err;
	
	ncp_init_request(conn);
	if (release) {
		ncp_add_byte(conn, 68);
	} else {
		ncp_add_byte(conn, 69);
	}
	ncp_add_dword_lh(conn, fh);
	ncp_add_qword_hl(conn, startOffset);
	ncp_add_qword_hl(conn, length);
	err = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return err;
}

static NWCCODE ncp_clear_release_physical_record(NWCONN_HANDLE conn, const char file_handle[6],
		ncp_off64_t startOffset, u_int64_t length, int release) {
	NWCCODE err;

	if (!conn || !file_handle) {
		return ERR_NULL_POINTER;
	}
	err = __NWReadFileServerInfo(conn);
	if (err) {
		return err;
	}
	if (conn->serverInfo.ncp64bit) {
		err = ncp_clear_release_physical_record64(conn, ConvertToDWORDfromNW(file_handle),
				startOffset, length, release);
	} else {
		if (startOffset >= 0x100000000ULL ||
		    length >= 0x100000000ULL ||
		    startOffset + length >= 0x100000000ULL) {
		    	return EFBIG;
		}
		err = ncp_clear_release_physical_record32(conn, file_handle,
				startOffset, length, release);
	}
	return err;
}

NWCCODE ncp_clear_physical_record(NWCONN_HANDLE conn, const char file_handle[6],
		ncp_off64_t startOffset, u_int64_t length) {
	return ncp_clear_release_physical_record(conn, file_handle, startOffset, length, 0);
}

NWCCODE ncp_release_physical_record(NWCONN_HANDLE conn, const char file_handle[6],
		ncp_off64_t startOffset, u_int64_t length) {
	return ncp_clear_release_physical_record(conn, file_handle, startOffset, length, 1);
}
