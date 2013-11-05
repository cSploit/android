/*
    eas.h
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

	1.01  2001, December 31		Petr Vandrovec <vandrove@vc.cvut.cz>
		Updated for SWIG.

 */

#ifndef __NCP_EAS_H__
#define __NCP_EAS_H__

#include <ncp/nwcalls.h>

#define NWEA_FL_CLOSE_IMM	0x0080
#define NWEA_FL_INFO(X)		((X) << 4)
#define NWEA_FL_INFO0		NWEA_FL_INFO(0)
#define NWEA_FL_INFO1		NWEA_FL_INFO(1)
#define NWEA_FL_INFO6		NWEA_FL_INFO(6)
#define NWEA_FL_INFO7		NWEA_FL_INFO(7)
#define NWEA_FL_CLOSE_ERR	0x0004
#define NWEA_FL_SRC_DIRENT	0x0000	/* volume + DOS dirEnt */
#define NWEA_FL_SRC_NWHANDLE	0x0001	/* handle from open */
#define NWEA_FL_SRC_EAHANDLE	0x0002	/* handle from previous EA op */

struct ncp_ea_enumerate_info {
	uint_fast32_t errorCode;
	uint_fast32_t totalEAs;
	uint_fast32_t totalEAsDataSize;
	uint_fast32_t totalEAsKeySize;
	uint_fast32_t newEAhandle;
	uint_fast16_t enumSequence;
	uint_fast16_t returnedItems;
};

struct ncp_ea_read_info {
	uint_fast32_t errorCode;
	uint_fast32_t totalValuesLength;
	uint_fast32_t newEAhandle;
	uint_fast32_t accessFlag;
};

struct ncp_ea_write_info {
	uint_fast32_t errorCode;
	uint_fast32_t written;
	uint_fast32_t newEAhandle;
};

/* level0 has no special info */
struct ncp_ea_info_level1 {
	uint_fast32_t valueLength;
	uint_fast32_t accessFlag;
#ifdef SWIG
%pragma(swig) readonly
	uint_fast16_t keyLength;
	uint_fast16_tLenPrefixCharArray key[256];
%pragma(swig) readwrite
#else
	uint_fast16_t keyLength;
	u_int8_t key[256];
#endif
};

struct ncp_ea_info_level6 {
	uint_fast32_t valueLength;
	uint_fast32_t accessFlag;
	uint_fast32_t valueExtants;
	uint_fast32_t keyExtants;
#ifdef SWIG
%pragma(swig) readonly
	uint_fast16_t keyLength;
	uint_fast16_tLenPrefixCharArray key[256];
%pragma(swig) readwrite
#else
	uint_fast16_t keyLength;
	u_int8_t key[256];
#endif
};

/* level7 is returned as char array */

NWCCODE ncp_ea_close(NWCONN_HANDLE conn, uint_fast32_t EAHandle);
NWCCODE ncp_ea_duplicate(NWCONN_HANDLE conn,
		uint_fast16_t srcFlags,
		uint_fast32_t srcHandleOrVol, uint_fast32_t srcZeroOrDir,
		uint_fast16_t dstFlags,
		uint_fast32_t dstHandleOrVol, uint_fast32_t dstZeroOrDir,
		u_int32_t* duplicateCount,
		u_int32_t* dataSizeDuplicated,
		u_int32_t* keySizeDuplicated);
NWCCODE ncp_ea_enumerate(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t inspectSize,
		const void* key, size_t keyLen,
		struct ncp_ea_enumerate_info* info, 
		void* data, size_t datalen, size_t* rdatalen);
NWCCODE ncp_ea_read(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t inspectSize,
		const void* key, size_t keyLen,
		uint_fast32_t readOffset,
		struct ncp_ea_read_info* info,
		void* data, size_t datalen, size_t* rdatalen);
NWCCODE ncp_ea_write(NWCONN_HANDLE conn,
		uint_fast16_t flags,
		uint_fast32_t handleOrVol, uint_fast32_t zeroOrDir,
		uint_fast32_t totalWriteSize,
		const void* key, size_t keyLen,
		uint_fast32_t writeOffset,
		uint_fast32_t accessFlag,
		struct ncp_ea_write_info* info,
		const void* data, size_t datalen);

NWCCODE ncp_ea_extract_info_level1(const unsigned char* buffer,
		const unsigned char* endbuf,
		struct ncp_ea_info_level1* info, size_t maxsize, 
		size_t* needsize, const unsigned char** next);
NWCCODE ncp_ea_extract_info_level6(const unsigned char* buffer,
		const unsigned char* endbuf,
		struct ncp_ea_info_level6* info, size_t maxsize, 
		size_t* needsize, const unsigned char** next);
NWCCODE ncp_ea_extract_info_level7(const unsigned char* buffer,
		const unsigned char* endbuf,
		char* key, size_t maxsize, size_t* needsize,
		const unsigned char** next);

#endif	/* __NCP_EAS_H__ */
