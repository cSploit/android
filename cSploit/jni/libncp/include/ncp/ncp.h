/*
    ncp.h
    Copyright (C) 1995 by Volker Lendecke

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

	1.00  2001, March 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NCP_BINDERY_TREE.

 */

#ifndef _NCP_H
#define _NCP_H

#include <ncp/kernel/types.h>
#include <ncp/kernel/ipx.h>
#include <ncp/kernel/ncp.h>
#include <ncp/kernel/ncp_fs.h>

typedef u_int16_t NWObjectType;
typedef u_int32_t NWObjectID;

#define NCP_BINDERY_WILD        (0xffff)
#define NCP_BINDERY_UNKNOWN     (0x0000)
#define NCP_BINDERY_USER	(0x0001)
#define NCP_BINDERY_UGROUP	(0x0002)	/* User Group */
#define NCP_BINDERY_PQUEUE	(0x0003)	/* Print Queue */
#define NCP_BINDERY_FSERVER	(0x0004)	/* File Server */
#define NCP_BINDERY_JSERVER     (0x0005)        /* Job Server */
#define NCP_BINDERY_GATEWAY     (0x0006)
#define NCP_BINDERY_PSERVER     (0x0007)        /* Print Server */
#define NCP_BINDERY_ARCHQ       (0x0008)        /* Archive Queue */
#define NCP_BINDERY_ARCHSERVER  (0x0009)        /* Archive Server */
#define NCP_BINDERY_JQUEUE      (0x000a)        /* Job Queue */
#define NCP_BINDERY_ADMIN       (0x000b)
#define NCP_BINDERY_BSERVER     (0x0026)        /* Bridge Server */
#define NCP_BINDERY_ADVPSERVER  (0x0047)        /* Advertising Print Server */
#define NCP_BINDERY_TREE	(0x0278)	/* NDS Tree */
#define NCP_BINDERY_NTSERVER    (0x0640)        /* NT Server */

#define NCP_BINDERY_ID_WILDCARD (0xffffffff)

#define NCP_BINDERY_NAME_LEN (48)

#ifndef IPX_NODE_LEN
#define IPX_NODE_LEN	(6)
#endif

struct ncp_bindery_object {
	NWObjectID object_id;
	NWObjectType object_type;
#ifdef SWIG
	fixedCharArray object_name[NCP_BINDERY_NAME_LEN];
#else
	u_int8_t object_name[NCP_BINDERY_NAME_LEN];
#endif
	u_int8_t object_flags;
	u_int8_t object_security;
	u_int8_t object_has_prop;
};

struct nw_property {
#ifdef SWIG
	fixedArray value[128];
#else
	u_int8_t value[128];
#endif
	u_int8_t more_flag;
	u_int8_t property_flag;
};

struct prop_net_address {
	u_int32_t network __attribute__((packed));
#ifdef SWIG
	fixedArray node[IPX_NODE_LEN];
#else
	u_int8_t node[IPX_NODE_LEN];
#endif
	u_int16_t port __attribute__((packed));
};

struct ncp_filesearch_info {
	u_int8_t volume_number;
	u_int16_t directory_id;
	u_int16_t sequence_no;
	u_int8_t access_rights;
};

#define NCP_MAX_FILENAME (14)
struct ncp_file_info {
#ifdef SWIG
	fixedArray file_id[NCP_FILE_ID_LEN];
	fixedCharArray file_name[NCP_MAX_FILENAME + 1];
#else	
	u_int8_t file_id[NCP_FILE_ID_LEN];
	char file_name[NCP_MAX_FILENAME + 1];
#endif
	u_int8_t file_attributes;
	u_int8_t file_mode;
	u_int32_t file_length;
	u_int16_t creation_date;
	u_int16_t access_date;
	u_int16_t update_date;
	u_int16_t update_time;
};

#ifdef SWIG
struct nw_queue_job_entry {
	u_int16_t InUse __attribute__((packed));
	u_int32_t prev __attribute__((packed));
	u_int32_t next __attribute__((packed));
	u_int32_t ClientStation __attribute__((packed));
	u_int32_t ClientTask __attribute__((packed));
	u_int32_t ClientObjectID __attribute__((packed));
	u_int32_t TargetServerID __attribute__((packed));
	fixedArray TargetExecTime[6] __attribute__((packed));
	fixedArray JobEntryTime[6] __attribute__((packed));
	u_int32_t JobNumber __attribute__((packed));
	u_int16_t JobType __attribute__((packed));
	u_int16_t JobPosition __attribute__((packed));
	u_int16_t JobControlFlags __attribute__((packed));
	u_int8_t FileNameLen __attribute__((packed));
	byteLenPrefixCharArray JobFileName[13] __attribute__((packed));
	u_int32_t JobFileHandle __attribute__((packed));
	u_int32_t ServerStation __attribute__((packed));
	u_int32_t ServerTaskNumber __attribute__((packed));
	u_int32_t ServerObjectID __attribute__((packed));
	fixedCharArray JobTextDescription[50] __attribute__((packed));
	fixedArray ClientRecordArea[152] __attribute__((packed));
};

struct queue_job {
	struct nw_queue_job_entry j;
	fixedArray file_handle[6];
};
#else
struct nw_queue_job_entry {
	u_int16_t InUse __attribute__((packed));
	u_int32_t prev __attribute__((packed));
	u_int32_t next __attribute__((packed));
	u_int32_t ClientStation __attribute__((packed));
	u_int32_t ClientTask __attribute__((packed));
	u_int32_t ClientObjectID __attribute__((packed));
	u_int32_t TargetServerID __attribute__((packed));
	u_int8_t TargetExecTime[6];
	u_int8_t JobEntryTime[6];
	u_int32_t JobNumber __attribute__((packed));
	u_int16_t JobType __attribute__((packed));
	u_int16_t JobPosition __attribute__((packed));
	u_int16_t JobControlFlags __attribute__((packed));
	u_int8_t FileNameLen;
	char JobFileName[13];
	u_int32_t JobFileHandle __attribute__((packed));
	u_int32_t ServerStation __attribute__((packed));
	u_int32_t ServerTaskNumber __attribute__((packed));
	u_int32_t ServerObjectID __attribute__((packed));
	char JobTextDescription[50];
	char ClientRecordArea[152];
};

struct queue_job {
	struct nw_queue_job_entry j;
	u_int8_t file_handle[6];
};
#endif

#define QJE_OPER_HOLD	0x80
#define QJE_USER_HOLD	0x40
#define QJE_ENTRYOPEN	0x20
#define QJE_SERV_RESTART    0x10
#define QJE_SERV_AUTO	    0x08

/* ClientRecordArea for print jobs */

#define   KEEP_ON        0x0400
#define   NO_FORM_FEED   0x0800
#define   NOTIFICATION   0x1000
#define   DELETE_FILE    0x2000
#define   EXPAND_TABS    0x4000
#define   PRINT_BANNER   0x8000

#ifdef SWIG
struct print_job_record {
	u_int8_t Version __attribute__((packed));
	u_int8_t TabSize __attribute__((packed));
	u_int16_t Copies __attribute__((packed));
	u_int16_t CtrlFlags __attribute__((packed));
	u_int16_t Lines __attribute__((packed));
	u_int16_t Rows __attribute__((packed));
	fixedCharArray FormName[16] __attribute__((packed));
	fixedArray Reserved[6] __attribute__((packed));
	fixedCharArray BannerName[13] __attribute__((packed));
	fixedCharArray FnameBanner[13] __attribute__((packed));
	fixedCharArray FnameHeader[14] __attribute__((packed));
	fixedCharArray Path[80] __attribute__((packed));
};
#else
struct print_job_record {
	u_int8_t Version;
	u_int8_t TabSize;
	u_int16_t Copies __attribute__((packed));
	u_int16_t CtrlFlags __attribute__((packed));
	u_int16_t Lines __attribute__((packed));
	u_int16_t Rows __attribute__((packed));
	char FormName[16];
	u_int8_t Reserved[6];
	char BannerName[13];
	char FnameBanner[13];
	char FnameHeader[14];
	char Path[80];
};
#endif

#endif				/* _NCP_H */
