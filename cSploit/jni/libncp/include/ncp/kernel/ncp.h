/*
    ncp.h
    Copyright (C) 1995 by Volker Lendecke
    Copyright (C) 1996  J.F. Chadima
    Copyright (C) 1997, 1998, 1999  Petr Vandrovec

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

	0.00  1995			Volker Lendecke
		Initial revision.

	0.01  1996			J.F. Chadima
		Modified for sparc.

	0.02  1998			Petr Vandrovec <vandrove@vc.cvut.cz>
		New flags...

	1.00  1999, November 20		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.
  
	1.01  2001, November 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NW_NS_LONG definition.
		Added couple of SWIG fixes.

 */

#ifndef _LINUX_NCP_H
#define _LINUX_NCP_H

#include <ncp/kernel/types.h>

#define NCP_PTYPE                (0x11)
#define NCP_PORT                 (0x0451)

#define NCP_ALLOC_SLOT_REQUEST   (0x1111)
#define NCP_REQUEST              (0x2222)
#define NCP_DEALLOC_SLOT_REQUEST (0x5555)

struct ncp_request_header {
	u_int16_t type __attribute__((packed));
	u_int8_t  sequence;
	u_int8_t  conn_low;
	u_int8_t  task;
	u_int8_t  conn_high;
	u_int8_t  function;
	u_int8_t  data[0];
};

#define NCP_REPLY                (0x3333)
#define NCP_POSITIVE_ACK         (0x9999)

struct ncp_reply_header {
	u_int16_t type __attribute__((packed));
	u_int8_t sequence;
	u_int8_t conn_low;
	u_int8_t task;
	u_int8_t conn_high;
	u_int8_t completion_code;
	u_int8_t connection_state;
	u_int8_t data[0];
};

#define NCP_VOLNAME_LEN (16)
#define NCP_NUMBER_OF_VOLUMES (64)
#ifdef SWIG
struct ncp_volume_info {
	u_int32_t total_blocks;
	u_int32_t free_blocks;
	u_int32_t purgeable_blocks;
	u_int32_t not_yet_purgeable_blocks;
	u_int32_t total_dir_entries;
	u_int32_t available_dir_entries;
	u_int8_t sectors_per_block;
	fixedCharArray volume_name[NCP_VOLNAME_LEN + 1];
};
#else
struct ncp_volume_info {
	u_int32_t total_blocks;
	u_int32_t free_blocks;
	u_int32_t purgeable_blocks;
	u_int32_t not_yet_purgeable_blocks;
	u_int32_t total_dir_entries;
	u_int32_t available_dir_entries;
	u_int8_t sectors_per_block;
	char volume_name[NCP_VOLNAME_LEN + 1];
};
#endif

#define NCP_FILE_ID_LEN 6

/* Defines for Name Spaces */
#define NW_NS_DOS	0
#define NW_NS_MAC	1
#define NW_NS_NFS	2
#define NW_NS_FTAM	3
#define NW_NS_OS2	4
#define NW_NS_LONG	NW_NS_OS2

#define	RIM_NAME		0x00000001
#define	RIM_SPACE_ALLOCATED	0x00000002
#define RIM_ATTRIBUTES		0x00000004
#define RIM_DATA_SIZE		0x00000008
#define RIM_TOTAL_SIZE		0x00000010
#define RIM_EXT_ATTR_INFO	0x00000020
#define RIM_ARCHIVE		0x00000040
#define RIM_MODIFY		0x00000080
#define RIM_CREATION		0x00000100
#define RIM_OWNING_NAMESPACE	0x00000200
#define RIM_DIRECTORY		0x00000400
#define RIM_RIGHTS		0x00000800
#define RIM_ALL			0x00000FFF
#define RIM_REFERENCE_ID	0x00001000
#define RIM_NS_ATTRIBUTES	0x00002000
#define RIM_DATASTREAM_SIZES	0x00004000
#define RIM_DATASTREAM_LOGICALS	0x00008000
#define RIM_UPDATE_TIME		0x00010000
#define RIM_DOS_NAME		0x00020000
#define RIM_FLUSH_TIME		0x00040000
#define RIM_PARENT_BASE_ID	0x00080000
#define RIM_MAC_FINDER_INFO	0x00100000
#define RIM_SIBLING_COUNT	0x00200000
#define RIM_EFFECTIVE_RIGHTS	0x00400000
#define RIM_MAC_TIMES		0x00800000
#define RIM_LAST_ACCESS_TIME	0x01000000
#ifdef MAKE_NCPLIB
#define RIM_UNKNOWN25		0x02000000	/* fixme! */
#endif
#define RIM_SIZE64		0x04000000
#define RIM_NSS_LARGE_SIZES	0x40000000
#define RIM_COMPRESSED_INFO	0x80000000U

/* open/create modes */
#define OC_MODE_OPEN			0x01
#define OC_MODE_TRUNCATE		0x02
#define OC_MODE_REPLACE 		0x02
#define OC_MODE_CREATE			0x08
#define OC_MODE_OPEN_64BIT_ACCESS	0x20
#define OC_MODE_READONLY_OK		0x40
#define OC_MODE_CALLBACK		0x80

/* open/create results */
#define OC_ACTION_NONE		0x00
#define OC_ACTION_OPEN		0x01
#define OC_ACTION_CREATE	0x02
#define OC_ACTION_TRUNCATE	0x04
#define OC_ACTION_REPLACE	0x04
#define OC_ACTION_COMPRESSED	0x08
#define OC_ACTION_READONLY	0x80

/* access rights attributes */
#ifndef AR_READ_ONLY
#define AR_READ_ONLY	   0x0001
#define AR_WRITE_ONLY	   0x0002
#define AR_DENY_READ	   0x0004
#define AR_DENY_WRITE	   0x0008
#define AR_COMPATIBILITY   0x0010
#define AR_WRITE_THROUGH   0x0040
#define AR_OPEN_COMPRESSED 0x0100
#endif

#ifdef SWIG
struct nw_info_struct {
	u_int32_t spaceAlloc __attribute__((packed));
	u_int32_t attributes __attribute__((packed));
	u_int16_t flags __attribute__((packed));
	u_int32_t dataStreamSize __attribute__((packed));
	u_int32_t totalStreamSize __attribute__((packed));
	u_int16_t numberOfStreams __attribute__((packed));
	u_int16_t creationTime __attribute__((packed));
	u_int16_t creationDate __attribute__((packed));
	u_int32_t creatorID __attribute__((packed));
	u_int16_t modifyTime __attribute__((packed));
	u_int16_t modifyDate __attribute__((packed));
	u_int32_t modifierID __attribute__((packed));
	u_int16_t lastAccessDate __attribute__((packed));
	u_int16_t archiveTime __attribute__((packed));
	u_int16_t archiveDate __attribute__((packed));
	u_int32_t archiverID __attribute__((packed));
	u_int16_t inheritedRightsMask __attribute__((packed));
	u_int32_t dirEntNum __attribute__((packed));
	u_int32_t DosDirNum __attribute__((packed));
	u_int32_t volNumber __attribute__((packed));
	u_int32_t EADataSize __attribute__((packed));
	u_int32_t EAKeyCount __attribute__((packed));
	u_int32_t EAKeySize __attribute__((packed));
	u_int32_t NSCreator __attribute__((packed));
%pragma(swig) readonly
	u_int8_t nameLen __attribute__((packed));
%pragma(swig) readwrite
	byteLenPrefixCharArray entryName[255] __attribute__((packed));
};
#else
struct nw_info_struct {
	u_int32_t spaceAlloc __attribute__((packed));
	u_int32_t attributes __attribute__((packed));
	u_int16_t flags __attribute__((packed));
	u_int32_t dataStreamSize __attribute__((packed));
	u_int32_t totalStreamSize __attribute__((packed));
	u_int16_t numberOfStreams __attribute__((packed));
	u_int16_t creationTime __attribute__((packed));
	u_int16_t creationDate __attribute__((packed));
	u_int32_t creatorID __attribute__((packed));
	u_int16_t modifyTime __attribute__((packed));
	u_int16_t modifyDate __attribute__((packed));
	u_int32_t modifierID __attribute__((packed));
	u_int16_t lastAccessDate __attribute__((packed));
	u_int16_t archiveTime __attribute__((packed));
	u_int16_t archiveDate __attribute__((packed));
	u_int32_t archiverID __attribute__((packed));
	u_int16_t inheritedRightsMask __attribute__((packed));
	u_int32_t dirEntNum __attribute__((packed));
	u_int32_t DosDirNum __attribute__((packed));
	u_int32_t volNumber __attribute__((packed));
	u_int32_t EADataSize __attribute__((packed));
	u_int32_t EAKeyCount __attribute__((packed));
	u_int32_t EAKeySize __attribute__((packed));
	u_int32_t NSCreator __attribute__((packed));
	u_int8_t nameLen;
	u_int8_t entryName[256];
};
#endif

/* modify mask - use with MODIFY_DOS_INFO structure */
#define DM_ATTRIBUTES		  (0x00000002L)
#define DM_CREATE_DATE		  (0x00000004L)
#define DM_CREATE_TIME		  (0x00000008L)
#define DM_CREATOR_ID		  (0x00000010L)
#define DM_ARCHIVE_DATE 	  (0x00000020L)
#define DM_ARCHIVE_TIME 	  (0x00000040L)
#define DM_ARCHIVER_ID		  (0x00000080L)
#define DM_MODIFY_DATE		  (0x00000100L)
#define DM_MODIFY_TIME		  (0x00000200L)
#define DM_MODIFIER_ID		  (0x00000400L)
#define DM_LAST_ACCESS_DATE	  (0x00000800L)
#define DM_INHERITED_RIGHTS_MASK  (0x00001000L)
#define DM_MAXIMUM_SPACE	  (0x00002000L)

struct nw_modify_dos_info {
	u_int32_t attributes __attribute__((packed));
	u_int16_t creationDate __attribute__((packed));
	u_int16_t creationTime __attribute__((packed));
	u_int32_t creatorID __attribute__((packed));
	u_int16_t modifyDate __attribute__((packed));
	u_int16_t modifyTime __attribute__((packed));
	u_int32_t modifierID __attribute__((packed));
	u_int16_t archiveDate __attribute__((packed));
	u_int16_t archiveTime __attribute__((packed));
	u_int32_t archiverID __attribute__((packed));
	u_int16_t lastAccessDate __attribute__((packed));
	u_int16_t inheritanceGrantMask __attribute__((packed));
	u_int16_t inheritanceRevokeMask __attribute__((packed));
	u_int32_t maximumSpace __attribute__((packed));
};

#ifdef SWIG
struct nw_file_info {
	struct nw_info_struct i;
	int opened;
	int access;
	u_int32_t server_file_handle __attribute__((packed));
	u_int8_t open_create_action __attribute__((packed));
	fixedArray file_handle[6] __attribute__((packed));
};
#else
struct nw_file_info {
	struct nw_info_struct i;
	int opened;
	int access;
	u_int32_t server_file_handle __attribute__((packed));
	u_int8_t open_create_action;
	u_int8_t file_handle[6];
};
#endif

struct nw_search_sequence {
	u_int8_t volNumber;
	u_int32_t dirBase __attribute__((packed));
	u_int32_t sequence __attribute__((packed));
};

#endif				/* _LINUX_NCP_H */
