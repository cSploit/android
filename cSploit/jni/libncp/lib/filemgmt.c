/*
    filemgmt.c
    Copyright (C) 1995, 1996 by Volker Lendecke
    Copyright (C) 1999-2001  Petr Vandrovec
    Copyright (C) 1999       Roumen Petrov

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

	0.00  1995			Volker Lendecke
		Initial release.

	0.01  1999, June 1		Petr Vandrovec <vandrove@vc.cvut.cz>
		Splitted from ncplib.c.
		See ncplib.c for other contributors.

	0.02  1999, July		Petr Vandrovec <vandrove@vc.cvut.cz>
		New ncp_ns_* function group.

	0.03  1999, September		Roumen Petrov <rpetrov@usa.net>
		Added ncp tracing code.
		Different bugfixes.

	1.00  1999, November, 20	Petr Vandrovec <vandrove@vc.cvut.cz>
		Added license.

	1.01  2000, May 17		Bruce Richardson <brichardson@lineone.net>
		Added ncp_perms_to_str and ncp_str_to_perms.

	1.02  2000, August 23		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added nw_info_struct3 and ncp_ns_extract_info_field.
	
	1.03  2000, October 28		Petr Vandrovec <vandrove@vc.cvut.cz>
		Changed some constants to symbolic names (SA_*, NCP_DIRSTYLE_*,
			NW_NS_*)

	1.04  2001, January 27		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWScanOpenFilesByConn2, ncp_ns_scan_connections_using_file.

	1.05  2001, February 4		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added ncp_ns_scan_physical_locks_by_file.
		
	1.06  2001, June 3		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NWGetDirSpaceLimitList, NWGetDirSpaceLimitList2.

	1.07  2001, September 23	Petr Vandrovec <vandrove@vc.cvut.cz>
		Initialize file_id in ncp_file_search_continue. Fix
		file_name braindamage on same place.

	1.08  2001, September 26	Petr Vandrovec <vandrove@vc.cvut.cz>
		Fixed ncp_read when reading with odd offset.

	1.09  2001, December 12		Hans Grobler <grobh@sun.ac.za>
		Added ncp_ns_delete_entry.

	1.10  2002, January 6		Petr Vandrovec <vandrove@vc.cvut.cz>
		Added NULL parameter checks.
		Added checks for legal reply sizes from server.

 */

#include "config.h"

#include <stddef.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>

#include "ncplib_i.h"
#include <ncp/nwcalls.h>
#include <ncp/ndslib.h>

#include "private/libintl.h"
#define _(X) dgettext(NCPFS_PACKAGE, (X))
#define N_(X) (X)

#ifndef ENOPKG
#define ENOPKG	ENOSYS
#endif

/* Here the ncp calls begin
 */

long
ncp_get_volume_info_with_number(struct ncp_conn *conn, int n,
				struct ncp_volume_info *target)
{
	long result;
	unsigned int len;

	if (n < 0 || n > 255) {
		return NWE_VOL_INVALID;
	}

	ncp_init_request_s(conn, 44);
	ncp_add_byte(conn, n);

	if ((result = ncp_request(conn, 22)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 30) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	len = ncp_reply_byte(conn, 29);
	if (conn->ncp_reply_size < 30 + len) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (len > NCP_VOLNAME_LEN)
	{
		ncp_printf(_("ncpfs: volume name too long: %d\n"), len);
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	if (target) {
		target->total_blocks = ncp_reply_dword_lh(conn, 0);
		target->free_blocks = ncp_reply_dword_lh(conn, 4);
		target->purgeable_blocks = ncp_reply_dword_lh(conn, 8);
		target->not_yet_purgeable_blocks = ncp_reply_dword_lh(conn, 12);
		target->total_dir_entries = ncp_reply_dword_lh(conn, 16);
		target->available_dir_entries = ncp_reply_dword_lh(conn, 20);
		target->sectors_per_block = ncp_reply_byte(conn, 28);
		memset(target->volume_name, 0, sizeof(target->volume_name));
		memcpy(&(target->volume_name), ncp_reply_data(conn, 30), len);
	}
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE ncp_get_directory_info(NWCONN_HANDLE conn, NWDIR_HANDLE dirhandle,
		DIR_SPACE_INFO* target) {
	NWCCODE err;
	size_t volnamelen;
	
	ncp_init_request_s(conn, 45);
	ncp_add_byte(conn, dirhandle);
	err = ncp_request(conn, 22);
	if (err) {
		ncp_unlock_conn(conn);
		return err;
	}
	if (conn->ncp_reply_size < 22) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	volnamelen = ncp_reply_byte(conn, 21);
	if (conn->ncp_reply_size < 22 + volnamelen) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (volnamelen >= NW_MAX_VOLUME_NAME_LEN) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	if (target) {
		target->totalBlocks = ncp_reply_dword_lh(conn, 0);
		target->availableBlocks = ncp_reply_dword_lh(conn, 4);
		target->purgeableBlocks = 0;
		target->notYetPurgeableBlocks = 0;
		target->totalDirEntries = ncp_reply_dword_lh(conn, 8);
		target->availableDirEntries = ncp_reply_dword_lh(conn, 12);
		target->reserved = ncp_reply_dword_lh(conn, 16);
		target->sectorsPerBlock = ncp_reply_byte(conn, 20);
		target->volLen = volnamelen;
		memcpy(target->volName, ncp_reply_data(conn, 22), volnamelen);
		target->volName[volnamelen] = 0;
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_volume_number(struct ncp_conn *conn, const char *name, int *target)
{
	long result;

	ncp_init_request_s(conn, 5);
	ncp_add_pstring(conn, name);

	if ((result = ncp_request(conn, 22)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (target) {
		*target = ncp_reply_byte(conn, 0);
	}
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE
ncp_get_volume_name(struct ncp_conn *conn,
		    NWVOL_NUM volume,
		    char* name, size_t nlen) {
	NWCCODE result;
	size_t len;
	
	if (volume > 255) {
		return NWE_VOL_INVALID;
	}

	ncp_init_request_s(conn, 6);
	ncp_add_byte(conn, volume);
	if ((result = ncp_request(conn, 22)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	len = ncp_reply_byte(conn, 0);
	if (conn->ncp_reply_size < 1 + len) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (name) {
		if (len >= nlen) {
			ncp_unlock_conn(conn);
			return NWE_BUFFER_OVERFLOW;
		}
		memcpy(name, ncp_reply_data(conn, 1), len);
		name[len] = 0;
	}
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE
NWGetVolumeName(struct ncp_conn *conn,
		NWVOL_NUM volume,
		char* name) {
	return ncp_get_volume_name(conn, volume, name, 17);
}

long
ncp_file_search_init(struct ncp_conn *conn,
		     int dir_handle, const char *path,
		     struct ncp_filesearch_info *target)
{
	long result;

	if (!target) {
		return ERR_NULL_POINTER;
	}
	if (dir_handle < 0 || dir_handle > 255) {
		return NWE_DIRHANDLE_INVALID;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, dir_handle);
	ncp_add_pstring(conn, path);

	if ((result = ncp_request(conn, 62)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 6) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	target->volume_number = ncp_reply_byte(conn, 0);
	target->directory_id = ncp_reply_word_hl(conn, 1);
	target->sequence_no = ncp_reply_word_hl(conn, 3);
	target->access_rights = ncp_reply_byte(conn, 5);
	ncp_unlock_conn(conn);
	return 0;
}


long
ncp_file_search_continue(struct ncp_conn *conn,
			 struct ncp_filesearch_info *fsinfo,
			 int attributes, const char *name,
			 struct ncp_file_info *target)
{
	long result;

	if (!fsinfo || !target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);

	ncp_add_byte(conn, fsinfo->volume_number);
	ncp_add_word_hl(conn, fsinfo->directory_id);
	ncp_add_word_hl(conn, fsinfo->sequence_no);

	ncp_add_byte(conn, attributes);
	ncp_add_pstring(conn, name);

	if ((result = ncp_request(conn, 63)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	fsinfo->sequence_no = ncp_reply_word_hl(conn, 0);

	memcpy(target->file_name, ncp_reply_data(conn, 4),
	       NCP_MAX_FILENAME);
	target->file_name[NCP_MAX_FILENAME] = 0;

	target->file_attributes = ncp_reply_byte(conn, 18);
	target->file_mode = ncp_reply_byte(conn, 19);
	target->file_length = ncp_reply_dword_hl(conn, 20);
	target->creation_date = ncp_reply_word_hl(conn, 24);
	target->access_date = ncp_reply_word_hl(conn, 26);
	target->update_date = ncp_reply_word_hl(conn, 28);
	target->update_time = ncp_reply_word_hl(conn, 30);
	memset(target->file_id, 0, sizeof(target->file_id));

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_get_finfo(struct ncp_conn *conn,
	      int dir_handle, const char *path, const char *name,
	      struct ncp_file_info *target)
{
	long result;

	struct ncp_filesearch_info fsinfo;

	if ((result = ncp_file_search_init(conn, dir_handle, path,
					   &fsinfo)) != 0)
	{
		return result;
	}
	if ((result = ncp_file_search_continue(conn, &fsinfo, 0, name,
					       target)) == 0)
	{
		return result;
	}
	if ((result = ncp_file_search_init(conn, dir_handle, path,
					   &fsinfo)) != 0)
	{
		return result;
	}
	return ncp_file_search_continue(conn, &fsinfo, A_DIRECTORY, name, target);
}

long
ncp_open_file(struct ncp_conn *conn,
	      int dir_handle, const char *path,
	      int attr, int access,
	      struct ncp_file_info *target)
{
	long result;

	if (!target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, dir_handle);
	ncp_add_byte(conn, attr);
	ncp_add_byte(conn, access);
	ncp_add_pstring(conn, path);

	if ((result = ncp_request(conn, 76)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 36) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	memcpy(&(target->file_id), ncp_reply_data(conn, 0),
	       NCP_FILE_ID_LEN);

	memset(target->file_name, 0, sizeof(target->file_name));
	memcpy(&(target->file_name), ncp_reply_data(conn, 8),
	       NCP_MAX_FILENAME);

	target->file_attributes = ncp_reply_byte(conn, 22);
	target->file_mode = ncp_reply_byte(conn, 23);
	target->file_length = ncp_reply_dword_hl(conn, 24);
	target->creation_date = ncp_reply_word_hl(conn, 28);
	target->access_date = ncp_reply_word_hl(conn, 30);
	target->update_date = ncp_reply_word_hl(conn, 32);
	target->update_time = ncp_reply_word_hl(conn, 34);

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_close_file(struct ncp_conn *conn, const char *file_id)
{
	long result;

	if (!file_id) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 0);
	ncp_add_mem(conn, file_id, 6);

	result = ncp_request(conn, 66);
	ncp_unlock_conn(conn);
	return result;
}

static int
ncp_do_create(struct ncp_conn *conn,
	      int dir_handle, const char *path,
	      int attr,
	      struct ncp_file_info *target,
	      int function)
{
	long result;

	if (!target) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, dir_handle);
	ncp_add_byte(conn, attr);
	ncp_add_pstring(conn, path);

	if ((result = ncp_request(conn, function)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 36) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	memcpy(&(target->file_id), ncp_reply_data(conn, 0),
	       NCP_FILE_ID_LEN);

	memset(target->file_name, 0, sizeof(target->file_name));
	memcpy(&(target->file_name), ncp_reply_data(conn, 8),
	       NCP_MAX_FILENAME);

	target->file_attributes = ncp_reply_byte(conn, 22);
	target->file_mode = ncp_reply_byte(conn, 23);
	target->file_length = ncp_reply_dword_hl(conn, 24);
	target->creation_date = ncp_reply_word_hl(conn, 28);
	target->access_date = ncp_reply_word_hl(conn, 30);
	target->update_date = ncp_reply_word_hl(conn, 32);
	target->update_time = ncp_reply_word_hl(conn, 34);

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_create_newfile(struct ncp_conn *conn,
		   int dir_handle, const char *path,
		   int attr,
		   struct ncp_file_info *target)
{
	return ncp_do_create(conn, dir_handle, path, attr, target, 77);
}

long
ncp_create_file(struct ncp_conn *conn,
		int dir_handle, const char *path,
		int attr,
		struct ncp_file_info *target)
{
	return ncp_do_create(conn, dir_handle, path, attr, target, 67);
}

long
ncp_erase_file(struct ncp_conn *conn,
	       int dir_handle, const char *path,
	       int attr)
{
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, dir_handle);
	ncp_add_byte(conn, attr);
	ncp_add_pstring(conn, path);

	result = ncp_request(conn, 68);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_rename_file(struct ncp_conn *conn,
		int old_handle, const char *old_path,
		int attr,
		int new_handle, const char *new_path)
{
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, old_handle);
	ncp_add_byte(conn, attr);
	ncp_add_pstring(conn, old_path);
	ncp_add_byte(conn, new_handle);
	ncp_add_pstring(conn, new_path);

	if ((result = ncp_request(conn, 69)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_create_directory(struct ncp_conn *conn,
		     int dir_handle, const char *path,
		     int inherit_mask)
{
	long result;

	ncp_init_request_s(conn, 10);
	ncp_add_byte(conn, dir_handle);
	ncp_add_byte(conn, inherit_mask);
	ncp_add_pstring(conn, path);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_delete_directory(struct ncp_conn *conn,
		     int dir_handle, const char *path)
{
	long result;

	ncp_init_request_s(conn, 11);
	ncp_add_byte(conn, dir_handle);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_pstring(conn, path);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_add_trustee(struct ncp_conn *conn,
		int dir_handle, const char *path,
		u_int32_t object_id, u_int8_t rights)
{
	long result;

	ncp_init_request_s(conn, 13);
	ncp_add_byte(conn, dir_handle);
	ncp_add_dword_hl(conn, object_id);
	ncp_add_byte(conn, rights);
	ncp_add_pstring(conn, path);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_delete_trustee(struct ncp_conn *conn,
		   int dir_handle, const char *path, u_int32_t object_id)
{
	long result;

	ncp_init_request_s(conn, 14);
	ncp_add_byte(conn, dir_handle);
	ncp_add_dword_hl(conn, object_id);
	ncp_add_byte(conn, 0);
	ncp_add_pstring(conn, path);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_get_trustee(struct ncp_conn *conn, u_int32_t object_id,
		u_int8_t vol, char *path,
		u_int16_t * trustee, u_int16_t * contin)
{
	long result;
	unsigned int len;
	
	if (!contin || !trustee || !path) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 71);
	ncp_add_byte(conn, vol);
	ncp_add_word_hl(conn, *contin);
	ncp_add_dword_hl(conn, object_id);

	if ((result = ncp_request(conn, 23)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 8) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	len = ncp_reply_byte(conn, 7);
	if (conn->ncp_reply_size < 8 + len) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*contin = ncp_reply_word_hl(conn, 0);
	*trustee = ncp_reply_byte(conn, 6);
	strncpy(path, ncp_reply_data(conn, 8), len);
	path[len] = 0;
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_rename_directory(struct ncp_conn *conn,
		     int dir_handle,
		     const char *old_path, const char *new_path)
{
	long result;

	ncp_init_request_s(conn, 15);
	ncp_add_byte(conn, dir_handle);
	ncp_add_pstring(conn, old_path);
	ncp_add_pstring(conn, new_path);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

static void
ncp_add_handle_path(struct ncp_conn *conn,
		    u_int8_t vol_num,
		    u_int32_t dir_base, u_int8_t dir_style,
		    const char *path)
{
	ncp_add_byte(conn, vol_num);
	ncp_add_dword_lh(conn, dir_base);
	ncp_add_byte(conn, dir_style);

	if (path) {
		ncp_add_byte(conn, 1);
		ncp_add_pstring(conn, path);
	} else
		ncp_add_byte(conn, 0);	
}

int
ncp_path_to_NW_format(const char* path, unsigned char* buff, int buffsize) 
{
	int components = 0;
	unsigned char* pos = buff+1;
	buffsize--;

	if (!buff) {
		return -EFAULT;
	}
	if (path != NULL) {
		if (*path == '/') path++;	/* skip optional leading / */
		while (*path) {
			const char *c;
			const char *d;
			int   l;

			c = strchr(path, '/');
			if (!c) c=path+strlen(path);
			l = c-path;
			if (components == 0) {			/* volume */
				d = strchr(path, ':');	/* can be separated by :, / or :/ */
				if (!d) d=path+strlen(path);
				if (d < c) {
					c=d;
					if (c[1]=='/') c++;	/* skip optional / after : */
					l = d-path;
				}
			}
			if (l == 0) 
				return -EINVAL;
			if (l > 255) 
				return -ENAMETOOLONG;
			if ((l != 1)||(*path!='.')) {
				if (buffsize <= l) return -ENOBUFS;
				buffsize -= l+1;
				*pos++ = l;
				memcpy(pos, path, l);
				pos+=l;
				components++;
			}
			path = c;
			if (!*c) break;
			path++;
		}
	}
	*buff = components;
	return pos-buff;
}

static int
ncp_path_to_NW_format2(const char* path, int dirstyle, unsigned char* buff, int buffsize) 
{
	int components = 0;
	unsigned char* pos = buff+1;
	unsigned char* end = buff+buffsize;
	int mayvolumesep;

	if (!buff) {
		return -EFAULT;
	}
	mayvolumesep = dirstyle == NCP_DIRSTYLE_NOHANDLE;
	if (path != NULL) {
		if (*path == '/') path++;	/* skip optional leading / */
		while (*path) {
			unsigned char* lenptr;
			char p;

			lenptr = NULL;
			while ((p = *path) != 0) {
				path++;
				if (p == '/' || (mayvolumesep && p == ':')) {
					break;
				}
				if (!lenptr) {
					if (buff >= end)
						return -ENOBUFS;
					lenptr = pos++;
				}
				if (p == '\xFF') {
					if (buff + 2 > end)
						return -ENOBUFS;
					*pos++ = 0xFF;
					*pos++ = 0xFF;
				} else {
					if (buff >= end)
						return -ENOBUFS;
					*pos++ = p;
				}
				if (pos - lenptr - 1 > 255)
					return -ENAMETOOLONG;
			}
			if (!lenptr)
				return -EINVAL;
			if (p == ':' && *path == '/')
				path++;
			mayvolumesep = 0;
			if (pos - lenptr == 3 && lenptr[1] == '.' && lenptr[2] == '.') {
				pos = lenptr + 1;
			} else if (pos - lenptr == 2 && lenptr[1] == '.') {
				pos = lenptr;
				continue;
			}
			*lenptr = pos - lenptr - 1;
			components++;
			if (components > 255)
				return -ENAMETOOLONG;
		}
	}
	*buff = components;
	return pos-buff;
}

static int
ncp_add_handle_path2(struct ncp_conn *conn,
		     unsigned int vol_num,
		     u_int32_t dir_base, int dir_style,
		     const unsigned char *encpath, int pathlen)
{
	if (dir_style == NCP_DIRSTYLE_DIRBASE) {
		if (vol_num > 255) {
			return NWE_VOL_INVALID;
		}
	}
	ncp_add_byte(conn, vol_num);
	ncp_add_dword_lh(conn, dir_base);
	ncp_add_byte(conn, dir_style);	/* 1 = dir_base, 0xFF = no handle, 0 = handle */
	if (encpath) {
		if (pathlen == NCP_PATH_STD) {
			int p = ncp_path_to_NW_format2(encpath, dir_style, conn->current_point, conn->packet + sizeof(conn->packet) - conn->current_point);
			if (p < 0) {
				return p;
			}
			conn->current_point += p;
		} else {
			ncp_add_mem(conn, encpath, pathlen);
		}
	} else {
		ncp_add_byte(conn, 0);	/* empty path */
	}
	return 0;
}

static void
ncp_extract_file_info(void *structure, struct nw_info_struct *target)
{
	if (target) {
		u_int8_t *name_len;
		const int info_struct_size = sizeof(*target) - 257;

		memcpy(target, structure, info_struct_size);
		name_len = (u_int8_t*)structure + info_struct_size;
		target->nameLen = *name_len;
		strncpy(target->entryName, name_len + 1, *name_len);
		target->entryName[*name_len] = '\0';
	}
	return;
}

long
ncp_obtain_file_or_subdir_info(struct ncp_conn *conn,
			       u_int8_t source_ns,
			       u_int8_t target_ns,
			       u_int16_t search_attribs,
			       u_int32_t rim,
			       u_int8_t vol,
			       u_int32_t dirent,
			       const char* path,
			       struct nw_info_struct *target) {
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 6);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_word_lh(conn, search_attribs);
	ncp_add_dword_lh(conn, rim);
	ncp_add_handle_path(conn, vol, dirent, NCP_DIRSTYLE_DIRBASE, path);
	/* fn: 87 , subfn: 6 */
	if ((result = ncp_request(conn, 87)) == 0)
	{
		ncp_extract_file_info(ncp_reply_data(conn, 0), target);
	}
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_get_eff_directory_rights(struct ncp_conn *conn,
			     u_int8_t source_ns, u_int8_t target_ns,
			     u_int16_t search_attribs,
			     u_int8_t vol, u_int32_t dirent, const char *path,
			     u_int16_t * my_effective_rights)
{
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 29);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_word_lh(conn, search_attribs);
	ncp_add_dword_lh(conn, 0);
	ncp_add_handle_path(conn, vol, dirent, NCP_DIRSTYLE_DIRBASE, path);
	/* fn: 87 , subfn: 29 */
	if ((result = ncp_request(conn, 87)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (my_effective_rights) {
		*my_effective_rights = ncp_reply_word_lh(conn, 0);
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_do_lookup2(struct ncp_conn *conn,
	       u_int8_t _source_ns,
	       const struct nw_info_struct *dir,
	       const char *path,       /* may only be one component */
	       u_int8_t _target_ns,
	       struct nw_info_struct *target)
{
	u_int8_t vol_num;
	u_int32_t dir_base;
	long result;

	if (target == NULL)
	{
		return EINVAL;
	}
	if (dir == NULL)
	{

		/* Access a volume's root directory */
		ncp_init_request(conn);
		ncp_add_byte(conn, 22);		/* subfunction */
		ncp_add_byte(conn, _source_ns);
		ncp_add_byte(conn, _target_ns);
		ncp_add_byte(conn, 0);	/* reserved */
		ncp_add_byte(conn, 0);	/* reserved */
		/* path must be volume_name */
		ncp_add_handle_path(conn, 0, 0, NCP_DIRSTYLE_NOHANDLE, path);
		/* fn: 87 , subfn: 22 */
		if ((result = ncp_request(conn, 87)) != 0)
		{
			ncp_unlock_conn(conn);
			return result;
		}
		dir_base = ncp_reply_dword_lh(conn, 4);
		vol_num = ncp_reply_byte(conn, 8);
		ncp_unlock_conn(conn);
		path = NULL;
	} else
	{
		vol_num = dir->volNumber;
		dir_base = dir->dirEntNum;
	}

	return ncp_obtain_file_or_subdir_info(
		conn, _source_ns, _target_ns,
		0xFF, RIM_ALL,
		vol_num, dir_base, path,
		target);
}

long
ncp_do_lookup(struct ncp_conn *conn,
	      const struct nw_info_struct *dir,
	      const char* path,
	      struct nw_info_struct *target) {
	return ncp_do_lookup2(conn, NW_NS_DOS, dir, path, NW_NS_DOS, target);
}

long
ncp_modify_file_or_subdir_dos_info(struct ncp_conn *conn,
				   const struct nw_info_struct *file,
				   u_int32_t info_mask,
				   const struct nw_modify_dos_info *info)
{
	long result;

	if (!info) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 7);	/* subfunction */
	ncp_add_byte(conn, NW_NS_DOS);	/* dos name space */
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, SA_ALL);	/* search attribs: all */

	ncp_add_dword_lh(conn, info_mask);
	ncp_add_mem(conn, info, sizeof(*info));
	ncp_add_handle_path(conn, file->volNumber,
			    file->DosDirNum, NCP_DIRSTYLE_DIRBASE, NULL);
	/* fn: 87 , subfn: 7 */
	result = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_delete_entry(struct ncp_conn *conn,
		    /* input */
		    unsigned int ns,
                    unsigned int search_attributes,
		    int dirstyle,
		    unsigned int vol,
		    u_int32_t dirent,
		    const unsigned char* encpath, size_t pathlen)
{
	NWCCODE result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 8);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, search_attributes);	/* Search attribs */
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	/* fn: 87 , subfn: 8 */
	if (result) {
		goto quit;
	}
	result = ncp_request(conn, 87);
quit:;
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_del_file_or_subdir(struct ncp_conn *conn,
		       const struct nw_info_struct *dir, const char *name)
{
	unsigned char namebuffer[257];
	size_t ln;
	unsigned char* encname;

	if (!dir) {
		return ERR_NULL_POINTER;
	}
	if (name) {
		ln = strlen(name);
		if (ln > 255) {
			return ENAMETOOLONG;
		}
		namebuffer[0] = 1;
		namebuffer[1] = ln;
		memcpy(namebuffer + 2, name, ln);
		ln += 2;
		encname = namebuffer;
	} else {
		encname = NULL;
		ln = 0;
	}
	return ncp_ns_delete_entry(conn, NW_NS_DOS, SA_ALL, NCP_DIRSTYLE_DIRBASE, 
		dir->volNumber, dir->DosDirNum, encname, ln);
}

long
ncp_open_create_file_or_subdir(struct ncp_conn *conn,
			       const struct nw_info_struct *dir, const char *name,
			       int open_create_mode,
			       u_int32_t create_attributes,
			       int desired_acc_rights,
			       struct nw_file_info *target)
{
	long result;

	if (!target || !dir) {
		return ERR_NULL_POINTER;
	}
	target->opened = 0;

	ncp_init_request(conn);
	ncp_add_byte(conn, 1);	/* subfunction */
	ncp_add_byte(conn, NW_NS_DOS);	/* dos name space */
	ncp_add_byte(conn, open_create_mode);
	ncp_add_word_lh(conn, SA_ALL);
	ncp_add_dword_lh(conn, RIM_ALL);
	ncp_add_dword_lh(conn, create_attributes);
	/* The desired acc rights seem to be the inherited rights mask
	   for directories */
	ncp_add_word_lh(conn, desired_acc_rights);
	ncp_add_handle_path(conn, dir->volNumber,
			    dir->DosDirNum, NCP_DIRSTYLE_DIRBASE, name);
	/* fn: 87 , subfn: 1 */
	if ((result = ncp_request(conn, 87)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	target->opened = 1;
	target->server_file_handle = ncp_reply_dword_lh(conn, 0);
	target->open_create_action = ncp_reply_byte(conn, 4);
	ncp_extract_file_info(ncp_reply_data(conn, 6), &(target->i));
	ConvertToNWfromDWORD(target->server_file_handle, target->file_handle);

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_initialize_search(struct ncp_conn *conn,
		      const struct nw_info_struct *dir,
		      int name_space,
		      struct ncp_search_seq *target)
{
	return ncp_initialize_search2(conn, dir, name_space, NULL, 0, target);
}

long
ncp_initialize_search2(struct ncp_conn *conn,
		       const struct nw_info_struct *dir,
		       int name_space,
		       const unsigned char *enc_subpath, int subpathlen,
		       struct ncp_search_seq *target)
{
	long result;

	if ((name_space < 0) || (name_space > 255))
	{
		return EINVAL;
	}
	if (!target || !dir) {
		return ERR_NULL_POINTER;
	}
	memset(target, 0, sizeof(*target));

	ncp_init_request(conn);
	ncp_add_byte(conn, 2);	/* subfunction */
	ncp_add_byte(conn, name_space);
	ncp_add_byte(conn, 0);	/* reserved */
	result = ncp_add_handle_path2(conn, dir->volNumber,
			    dir->dirEntNum, NCP_DIRSTYLE_DIRBASE,
			    enc_subpath, subpathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 2 */
	if ((result = ncp_request(conn, 87)) != 0) {
quit:;
		ncp_unlock_conn(conn);
		return result;
	}
	memcpy(&(target->s), ncp_reply_data(conn, 0), 9);
	target->name_space = name_space;

	ncp_unlock_conn(conn);
	return 0;
}

/* Search for everything */
long
ncp_search_for_file_or_subdir2(struct ncp_conn *conn,
			       int search_attributes,
			       u_int32_t RIM,
			       struct ncp_search_seq *seq,
			       struct nw_info_struct *target)
{
	long result;

	if (!seq) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 3);	/* subfunction */
	ncp_add_byte(conn, seq->name_space);
	ncp_add_byte(conn, 0);	/* data stream (???) */
	ncp_add_word_lh(conn, search_attributes);	/* Search attribs */
	ncp_add_dword_lh(conn, RIM);	/* return info mask */
	ncp_add_mem(conn, &(seq->s), 9);
	if ((seq->name_space == NW_NS_NFS) || (seq->name_space == NW_NS_MAC))
		ncp_add_byte(conn, 0);
	else {
		ncp_add_byte(conn, 2);	/* 2 byte pattern */
		ncp_add_byte(conn, 0xff);	/* following is a wildcard */
		ncp_add_byte(conn, '*');
	}
	/* fn: 87 , subfn: 3 */
	if ((result = ncp_request(conn, 87)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	memcpy(&(seq->s), ncp_reply_data(conn, 0), sizeof(seq->s));
	ncp_extract_file_info(ncp_reply_data(conn, 10), target);

	ncp_unlock_conn(conn);
	return 0;
}

/* Search for everything */
long
ncp_search_for_file_or_subdir(struct ncp_conn *conn,
			      struct ncp_search_seq *seq,
			      struct nw_info_struct *target)
{
	return ncp_search_for_file_or_subdir2(conn, SA_ALL, RIM_ALL, seq, target);
}

long
ncp_ren_or_mov_file_or_subdir(struct ncp_conn *conn,
			      const struct nw_info_struct *old_dir, const char *old_name,
			      const struct nw_info_struct *new_dir, const char *new_name)
{
	long result;

	if ((old_dir == NULL) || (old_name == NULL)
	    || (new_dir == NULL) || (new_name == NULL))
		return EINVAL;

	ncp_init_request(conn);
	ncp_add_byte(conn, 4);	/* subfunction */
	ncp_add_byte(conn, NW_NS_DOS);	/* dos name space */
	ncp_add_byte(conn, 1);	/* rename flag */
	ncp_add_word_lh(conn, SA_ALL);	/* search attributes */

	/* source Handle Path */
	ncp_add_byte(conn, old_dir->volNumber);
	ncp_add_dword_lh(conn, old_dir->DosDirNum);
	ncp_add_byte(conn, NCP_DIRSTYLE_DIRBASE);
	ncp_add_byte(conn, 1);	/* 1 source component */

	/* dest Handle Path */
	ncp_add_byte(conn, new_dir->volNumber);
	ncp_add_dword_lh(conn, new_dir->DosDirNum);
	ncp_add_byte(conn, NCP_DIRSTYLE_DIRBASE);
	ncp_add_byte(conn, 1);	/* 1 destination component */

	/* source path string */
	ncp_add_pstring(conn, old_name);
	/* dest path string */
	ncp_add_pstring(conn, new_name);
	/* fn: 87 , subfn: 4 */
	result = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return result;
}

static int
ncp_do_read(struct ncp_conn *conn, const char *file_id,
	    u_int32_t offset, u_int16_t to_read,
	    char *target, int *bytes_read)
{
	long result;
	unsigned int off;
	unsigned int len;

	ncp_init_request(conn);
	ncp_add_byte(conn, 0);
	ncp_add_mem(conn, file_id, 6);
	ncp_add_dword_hl(conn, offset);
	ncp_add_word_hl(conn, to_read);

	if ((result = ncp_request(conn, 72)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	len = ncp_reply_word_hl(conn, 0);
	off = 2 + (offset & 1);
	if (conn->ncp_reply_size < off + len) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*bytes_read = len;
	memcpy(target, ncp_reply_data(conn, off), len);

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_read(struct ncp_conn *conn, const char *file_id,
	 off_t offset, size_t count, char *target)
{
	int bufsize = conn->i.buffer_size;
	size_t already_read = 0;

	if (!file_id || !target) {
		return ERR_NULL_POINTER;
	}
	if (bufsize > NCP_PACKET_SIZE - 40)
		bufsize = NCP_PACKET_SIZE - 40;

	while (already_read < count)
	{
		int read_this_time;
		int to_read = min(bufsize - (offset % bufsize),
				  count - already_read);

		if (ncp_do_read(conn, file_id, offset, to_read,
				target, &read_this_time) != 0)
		{
			return -1;
		}
		offset += read_this_time;
		target += read_this_time;
		already_read += read_this_time;

		if (read_this_time < to_read)
		{
			break;
		}
	}
	return already_read;
}

static NWCCODE
ncp_do_read_64(struct ncp_conn *conn, u_int32_t fh,
	    ncp_off64_t offset, size_t to_read,
	    void *target, size_t *bytes_read)
{
	long result;
	unsigned int off;
	unsigned int len;

	ncp_init_request(conn);
	ncp_add_byte(conn, 64);
	ncp_add_dword_lh(conn, fh);
	ncp_add_qword_hl(conn, offset);
	ncp_add_word_hl(conn, to_read);

	if ((result = ncp_request(conn, 87)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	len = ncp_reply_word_hl(conn, 0);
	off = 2 + (offset & 1);
	if (conn->ncp_reply_size < off + len) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (len > to_read) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*bytes_read = len;
	memcpy(target, ncp_reply_data(conn, off), len);

	ncp_unlock_conn(conn);
	return 0;
}

static NWCCODE
ncp_read_64(struct ncp_conn *conn, u_int32_t fh, ncp_off64_t offset, 
		size_t count, char *target, size_t* readbytes) {
	size_t bufsize = conn->i.buffer_size;
	size_t already_read = 0;

	if (bufsize > NCP_PACKET_SIZE - 40)
		bufsize = NCP_PACKET_SIZE - 40;

	while (already_read < count) {
		size_t read_this_time;
		size_t to_read = count - already_read;
		NWCCODE err;
		
		if (to_read > bufsize) {
			/* Align offset to WORD ASAP */
			to_read = bufsize - (offset & 1);
		}

		err = ncp_do_read_64(conn, fh, offset, to_read,
				     target, &read_this_time);
		if (err) {
			if (already_read) {
				break;
			}
			return err;
		}
		offset += read_this_time;
		target += read_this_time;
		already_read += read_this_time;

		if (read_this_time < to_read) {
			break;
		}
	}
	*readbytes = already_read;
	return 0;
}

NWCCODE ncp_read64(struct ncp_conn *conn, const char file_handle[6],
		ncp_off64_t offset, size_t count, void *target, size_t *readbytes) {
	NWCCODE result;

	if (!conn || !file_handle || !target) {
		return ERR_NULL_POINTER;
	}	
	result = __NWReadFileServerInfo(conn);
	if (result) {
		return result;
	}
	if (conn->serverInfo.ncp64bit) {
		u_int32_t fh = ConvertToDWORDfromNW(file_handle);
		return ncp_read_64(conn, fh, offset, count, target, readbytes);
	} else {
		long rv;

		if (offset >= 0x100000000ULL) {
			return EFBIG;
		}
		if (offset + count > 0x100000000ULL) {
			count = 0x100000000ULL - offset;
		}
		rv = ncp_read(conn, file_handle, offset, count, target);
		if (rv > 0) {
			*readbytes = rv;
			rv = 0;
		}
		return rv;
	}
}

static int
ncp_do_write(struct ncp_conn *conn, const char *file_id,
	     u_int32_t offset, u_int16_t to_write,
	     const char *source, int *bytes_written)
{
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 0);
	ncp_add_mem(conn, file_id, 6);
	ncp_add_dword_hl(conn, offset);
	ncp_add_word_hl(conn, to_write);
	ncp_add_mem(conn, source, to_write);

	if ((result = ncp_request(conn, 73)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	*bytes_written = to_write;

	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_write(struct ncp_conn *conn, const char *file_id,
	  off_t offset, size_t count, const char *source)
{
	int bufsize = conn->i.buffer_size;
	size_t already_written = 0;

	if (!file_id || !source) {
		return ERR_NULL_POINTER;
	}
	if (bufsize > NCP_PACKET_SIZE - 40)
		bufsize = NCP_PACKET_SIZE - 40;

	while (already_written < count)
	{
		int written_this_time;
		int to_write = min(bufsize - (offset % bufsize),
				   count - already_written);

		if (ncp_do_write(conn, file_id, offset, to_write,
				 source, &written_this_time) != 0)
		{
			return -1;
		}
		offset += written_this_time;
		source += written_this_time;
		already_written += written_this_time;

		if (written_this_time < to_write)
		{
			break;
		}
	}
	return already_written;
}

static NWCCODE ncp_do_write_64(struct ncp_conn *conn, u_int32_t fh,
		ncp_off64_t offset, size_t to_write, const void *source, 
		size_t *bytes_written) {
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 65);
	ncp_add_dword_lh(conn, fh);
	ncp_add_qword_hl(conn, offset);
	ncp_add_word_hl(conn, to_write);
	ncp_add_mem(conn, source, to_write);

	if ((result = ncp_request(conn, 87)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	*bytes_written = to_write;
	ncp_unlock_conn(conn);
	return 0;
}

static NWCCODE ncp_write_64(struct ncp_conn *conn, u_int32_t fh,
		ncp_off64_t offset, size_t count, const char *source,
		size_t *bytes_written)
{
	size_t bufsize = conn->i.buffer_size;
	size_t already_written = 0;

	if (bufsize > NCP_PACKET_SIZE - 40)
		bufsize = NCP_PACKET_SIZE - 40;

	while (already_written < count)	{
		size_t written_this_time;
		size_t to_write = count - already_written;
		NWCCODE err;

		if (to_write > bufsize) {
			to_write = bufsize - (offset & 1);
		}
		err = ncp_do_write_64(conn, fh, offset, to_write,
				 source, &written_this_time);
		if (err) {
			if (already_written) {
				break;
			}
			return err;
		}
		offset += written_this_time;
		source += written_this_time;
		already_written += written_this_time;

		if (written_this_time < to_write) {
			break;
		}
	}
	*bytes_written = already_written;
	return 0;
}

NWCCODE ncp_write64(struct ncp_conn *conn, const char file_handle[6],
		ncp_off64_t offset, size_t count, const void *target, size_t *bytes) {
	NWCCODE result;

	if (!conn || !file_handle || !target) {
		return ERR_NULL_POINTER;
	}	
	result = __NWReadFileServerInfo(conn);
	if (result) {
		return result;
	}
	if (conn->serverInfo.ncp64bit) {
		u_int32_t fh = ConvertToDWORDfromNW(file_handle);
		return ncp_write_64(conn, fh, offset, count, target, bytes);
	} else {
		long rv;

		if (offset >= 0x100000000ULL) {
			return EFBIG;
		}
		if (offset + count > 0x100000000ULL) {
			count = 0x100000000ULL - offset;
		}
		rv = ncp_write(conn, file_handle, offset, count, target);
		if (rv > 0) {
			*bytes = rv;
			rv = 0;
		}
		return rv;
	}
}

long
ncp_copy_file(struct ncp_conn *conn,
	      const char source_file[6],
	      const char target_file[6],
	      u_int32_t source_offset,
	      u_int32_t target_offset,
	      u_int32_t count,
	      u_int32_t * copied_count)
{
	long result;

	ncp_init_request(conn);

	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_mem(conn, source_file, 6);
	ncp_add_mem(conn, target_file, 6);
	ncp_add_dword_hl(conn, source_offset);
	ncp_add_dword_hl(conn, target_offset);
	ncp_add_dword_hl(conn, count);

	if ((result = ncp_request(conn, 74)) != 0)
	{
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 4) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (copied_count) {
		*copied_count = ncp_reply_dword_hl(conn, 0);
	}
	ncp_unlock_conn(conn);
	return 0;
}

long
ncp_dealloc_dir_handle(struct ncp_conn *conn, u_int8_t dir_handle)
{
	long result;

	ncp_init_request_s(conn, 20);
	ncp_add_byte(conn, dir_handle);

	result = ncp_request(conn, 22);
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_alloc_short_dir_handle2(struct ncp_conn *conn,
			    u_int8_t ns,
			    const struct nw_info_struct *dir,
			    word alloc_mode,
			    byte * target)
{
	NWDIR_HANDLE dh;
	NWCCODE err;
	
	err = ncp_ns_alloc_short_dir_handle(conn, ns,
		NCP_DIRSTYLE_DIRBASE, dir->volNumber,
		dir->DosDirNum, NULL, 0,
		alloc_mode, &dh, NULL);
	if (!err) {
		if (target)
			*target = dh;
	}
	return err;
}

long
ncp_alloc_short_dir_handle(struct ncp_conn *conn,
			   const struct nw_info_struct *dir,
			   u_int16_t alloc_mode,
			   u_int8_t* target) {
	return ncp_alloc_short_dir_handle2(conn, NW_NS_DOS, dir, alloc_mode, target);
}

long
ncp_ns_scan_salvageable_file(struct ncp_conn* conn, u_int8_t src_ns,
			     int dirstyle, 
			     u_int8_t vol_num, u_int32_t dir_base,
			     const unsigned char *encpath, int pathlen,
			     struct ncp_deleted_file* finfo,
			     char* name, int maxnamelen)
{
	long result;
	u_int8_t namelen;

	ncp_init_request(conn);
	ncp_add_byte(conn, 0x10);
	ncp_add_byte(conn, src_ns);
	ncp_add_byte(conn, 0);
	ncp_add_dword_lh(conn, RIM_NAME);
	ncp_add_dword_lh(conn, finfo->seq);
	result = ncp_add_handle_path2(conn, vol_num, dir_base, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	result = ncp_request(conn, 0x57);
	if (result) {
quit:;
		ncp_unlock_conn(conn);
		return result;
	}
	/* reply format:                        * == returned by RIM_NAME
		+00 u_int32_t lh	next sequence   *
		+04 u_int16_t		deletion time   *
		+06 u_int16_t		deletion date   *
                +08 u_int32_t lh	deletor ID      *
                +0C u_int32_t lh	volume          *
		+10 u_int32_t lh	directory base  *
		+14 u_int32_t		?
		+18 u_int32_t lh	attributes
		+1C u_int16_t hl	flags ?
		+1E u_int32_t lh	size
		+22 u_int32_t		?
		+26 u_int16_t		?
		+28 u_int16_t		creation time
		+2A u_int16_t		creation date
		+2C u_int32_t lh	creator ID
		+30 u_int16_t		modify time
		+32 u_int16_t		modify date
		+34 u_int32_t lh	modifier ID
		+38 u_int16_t		last access date
		+3A u_int16_t		last archive time
		+3C u_int16_t		last archive date
		+3E u_int32_t lh	last archiver ID
		+42 u_int16_t		inherited right mask
		+44 u_int8_t[0x18]	?
		+5C u_int32_t lh	owning namespace
		+60 u_int8_t[var]	name, length preceeded  *
	*/

	if (conn->ncp_reply_size < 0x61) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	finfo->seq = ncp_reply_dword_lh(conn, 0x00);
	finfo->vol = ncp_reply_dword_lh(conn, 0x0C);
	finfo->base = ncp_reply_dword_lh(conn, 0x10);
	if (name) {
		namelen = ncp_reply_byte(conn, 0x60);
		if (namelen >= maxnamelen) {
			result = ENAMETOOLONG;
			namelen = maxnamelen-1;
		}
		memcpy(name, ncp_reply_data(conn, 0x61), namelen);
		name[namelen] = 0;
	}
	ncp_unlock_conn(conn);
	return result;
}

long
ncp_ns_purge_file(struct ncp_conn* conn,
	          const struct ncp_deleted_file* finfo)
{
	long result;

	if (!finfo) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 18);
	ncp_add_byte(conn, NW_NS_DOS);
	ncp_add_byte(conn, 0);		/* reserved? */
	ncp_add_dword_lh(conn, finfo->seq);
	ncp_add_dword_lh(conn, finfo->vol);
	ncp_add_dword_lh(conn, finfo->base);
	/* fn: 87 , subfn: 18 */
	result = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return result;
}

#if 0

/* This function can crash NW3.12 (even with latest patches as of
   August 24, 2000) when vol_num = 66, dir_base = 0xFFFFFFFF */
static NWCCODE ncp_map_directory_number_to_path(
		struct ncp_conn* conn, u_int8_t ns,
		u_int8_t vol_num, u_int32_t dir_base,
		unsigned char* name, size_t maxnamelen, unsigned char** begin)
{
	NWCCODE err;
	
	ncp_init_request_s(conn, 243);
	ncp_add_byte(conn, vol_num);
	ncp_add_dword_lh(conn, dir_base);
	ncp_add_byte(conn, ns);
	/* fn: 23, subfn: 243 */
	err = ncp_request(conn, 23);
	if (!err) {
		size_t rplen = conn->ncp_reply_size;

		if (rplen > maxnamelen)
			err = ENAMETOOLONG;
		else {
			unsigned char* start = name + maxnamelen - rplen;
			
			memcpy(start, ncp_reply_data(conn, 0), rplen);
			*begin = start;
		}
	}
	ncp_unlock_conn(conn);
	return err;
}
#endif

struct ncpi_gfn_cookies {
		int	flag;
		int32_t	cookie1;
		int32_t	cookie2;
			};
static long
ncp_ns_get_full_name_int(struct ncp_conn* conn, u_int8_t src_ns, u_int8_t dst_ns,
		int dirstyle, u_int8_t vol_num, u_int32_t dir_base,
		const unsigned char* encpath, size_t pathlen,
		struct ncpi_gfn_cookies* cookies,
		unsigned char* name, size_t maxnamelen, unsigned char** begin)
{
	long result;
	size_t comps;
	unsigned char* putpos;
	unsigned char* getpos;
	unsigned char* getend;

	ncp_init_request(conn);
	ncp_add_byte(conn, 0x1C);
	ncp_add_byte(conn, src_ns);
	ncp_add_byte(conn, dst_ns);
	ncp_add_word_lh(conn, cookies->flag);
	ncp_add_dword_lh(conn, cookies->cookie1);
	ncp_add_dword_lh(conn, cookies->cookie2);
	result = ncp_add_handle_path2(conn, vol_num, dir_base, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	result = ncp_request(conn, 0x57);
	if (result) {
quit:;	
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 14) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	cookies->flag = ncp_reply_word_lh(conn, 0);
	cookies->cookie1 = ncp_reply_dword_lh(conn, 2);
	cookies->cookie2 = ncp_reply_dword_lh(conn, 6);
	comps = ncp_reply_word_lh(conn, 12);
	getpos = ncp_reply_data(conn, 14);
	getend = getpos + ncp_reply_word_lh(conn, 10);
	putpos = name + maxnamelen;
	while (comps--) {
		size_t partl;

		if (getpos >= getend) {
			ncp_unlock_conn(conn);
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		partl = *getpos++;
		if (getpos + partl > getend) {
			ncp_unlock_conn(conn);
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		putpos -= partl+1;
		if (putpos < name) {
			ncp_unlock_conn(conn);
			return ENAMETOOLONG;
		}
		memcpy(putpos + 1, getpos, partl);
		*putpos = partl;
		getpos += partl;
	}
	ncp_unlock_conn(conn);
	*begin = putpos;
	return 0;
}

static long
ncp_ns_NW_to_path(char* name, size_t maxnamelen,
		  const unsigned char* encpath, const unsigned char* encend)
{
	char* nameend = name + maxnamelen;
	int pos = 0;

	if (!name) {
		return ERR_NULL_POINTER;
	}
	while (encpath < encend) {
		int namel;

		if (pos >= 2) {
			if (name >= nameend) return ENAMETOOLONG;
			*name++ = '/';
		}	
		namel = *encpath++;
		if (encpath + namel > encend) {
			return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		if (name + namel >= nameend) {
			return ENAMETOOLONG;
		}
		memcpy(name, encpath, namel);
		encpath += namel;
		name += namel;
		if (pos == 0) {
			if (name >= nameend) return ENAMETOOLONG;
			*name++ = ':';
		}
		pos++;
	}
	if (name >= nameend) return ENAMETOOLONG;
	*name = 0;
	return 0;
}

static NWCCODE
ncp_ns_get_full_name_loop(struct ncp_conn* conn, u_int8_t src_ns, 
		u_int8_t dst_ns, int dirstyle, u_int8_t vol_num,
		u_int32_t dir_base,
		const unsigned char* encpath, size_t enclen,
		unsigned char* buffer, size_t maxlen, unsigned char** begin) {
	struct ncpi_gfn_cookies cookie = { 0, -1, -1};
	NWCCODE err;
	unsigned char* npos;

	do {
		err = ncp_ns_get_full_name_int(conn, src_ns, dst_ns,
			dirstyle, vol_num, dir_base,
			encpath, enclen,
			&cookie, buffer, maxlen, &npos);
		if (err)
			return err;
		maxlen = npos - buffer;
	} while (cookie.cookie2 != -1);
	*begin = npos;
	return 0;
}

static inline void ncpi_putname(unsigned char* base, size_t* len, 
		const struct NSI_Name* name) {
	*len = *len - name->NameLength - 1;
	base[*len] = name->NameLength;
	memcpy(base + *len + 1, name->Name, name->NameLength);
}

long
ncp_ns_get_full_name(struct ncp_conn* conn, u_int8_t src_ns, u_int8_t dst_ns, 
		     int dirstyle, u_int8_t vol_num, u_int32_t dir_base, 
		     const unsigned char* encpath, size_t pathlen,
		     char* name, size_t maxnamelen)
{
	unsigned char space[1024];
	size_t len = sizeof(space);
	NWCCODE err;
	unsigned char* npos;
	struct nw_info_struct2 info;
	static const unsigned char up[] = { 1, 0 };
			
	/* NCP 87,28: Retrieve full name is broken for files:
	              NW3.12:                    return 0x899C
		      NW4.10, NW4.11 before SP4: works
		      NW4.11 after SP4, NW5.x:   only directory without
		                                 filename is returned
	   To workaround this, we first retrieve info about entry.
	   If it is directory, do simple call to NCP 87,28, otherwise
	   retrieve filename from info about entry and execute NCP 87,28
	   for parent directory (i.e. encpath = 1 item, 0 length).
	   
	   Do NOT try using NCP 23,243 instead - it can abend server. */
	err = ncp_ns_obtain_entry_info(conn, src_ns,
		SA_ALL, dirstyle, vol_num, dir_base,
		encpath, pathlen,
		dst_ns,
		RIM_NAME|RIM_ATTRIBUTES|RIM_DIRECTORY,
		&info, sizeof(info));
	if (err)
		return err;
	if (info.Attributes.Attributes & A_DIRECTORY) {
		err = ncp_ns_get_full_name_loop(conn, dst_ns, dst_ns,
			1, info.Directory.volNumber, info.Directory.dirEntNum,
			NULL, 0,
			space, len, &npos);
	} else {
		ncpi_putname(space, &len, &info.Name);
		/* retrieve directory name */
		err = ncp_ns_get_full_name_loop(conn, dst_ns, dst_ns,
			1, info.Directory.volNumber, info.Directory.dirEntNum,
			up, sizeof(up),
			space, len, &npos);
	}
	if (err)
		return err;
	return ncp_ns_NW_to_path(name, maxnamelen, npos, space+sizeof(space));
}

long
ncp_obtain_file_or_subdir_info2(struct ncp_conn *conn,
			        u_int8_t source_ns, u_int8_t target_ns,
			        u_int16_t search_attribs, u_int32_t rim,
				int dir_style,
			        u_int8_t vol, u_int32_t dirent, const unsigned char *encpath, 
				int pathlen, struct nw_info_struct *target)
{
	long result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 6);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_word_lh(conn, search_attribs);
	ncp_add_dword_lh(conn, rim);
	result = ncp_add_handle_path2(conn, vol, dirent, dir_style, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 6 */
	if ((result = ncp_request(conn, 87)) == 0)
	{
		ncp_extract_file_info(ncp_reply_data(conn, 0), target);
	}
quit:;	
	ncp_unlock_conn(conn);
	return result;
}

static NWCCODE get_file_info_size(
		const u_int8_t** end,
		u_int32_t rim, 
		const u_int8_t* src, 
		size_t srclen) {
	const u_int8_t* srcend = src + srclen;

	if (rim & RIM_COMPRESSED_INFO) {
		if (rim & RIM_SPACE_ALLOCATED) {
			src += 4;
		}
		if (rim & RIM_ATTRIBUTES) {
			src += 6;
		}
		if (rim & RIM_DATA_SIZE) {
			src += 4;
		}
		if (rim & RIM_TOTAL_SIZE) {
			src += 6;
		}
		if (rim & RIM_EXT_ATTR_INFO) {
			src += 12;
		}
		if (rim & RIM_ARCHIVE) {
			src += 8;
		}
		if (rim & RIM_MODIFY) {
			src += 10;
		}
		if (rim & RIM_CREATION) {
			src += 8;
		}
		if (rim & RIM_OWNING_NAMESPACE) {
			src += 4;
		}
		if (rim & RIM_DIRECTORY) {
			src += 12;
		}
		if (rim & RIM_RIGHTS) {
			src += 2;
		}
		if (rim & RIM_REFERENCE_ID) {
			src += 2;
		}
		if (rim & RIM_NS_ATTRIBUTES) {
			src += 4;
		}
		if (rim & RIM_DATASTREAM_SIZES) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 8 * DVAL_LH(src, 0) + 4;
		}
		if (rim & RIM_DATASTREAM_LOGICALS) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 8 * DVAL_LH(src, 0) + 4;
		}
		if (rim & RIM_UPDATE_TIME) {
			src += 4;
		}
		if (rim & RIM_DOS_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 1 + BVAL(src, 0);
		}
		if (rim & RIM_FLUSH_TIME) {
			src += 4;
		}
		if (rim & RIM_PARENT_BASE_ID) {
			src += 4;
		}
		if (rim & RIM_MAC_FINDER_INFO) {
			src += 32;
		}
		if (rim & RIM_SIBLING_COUNT) {
			src += 4;
		}
		if (rim & RIM_EFFECTIVE_RIGHTS) {
			src += 4;
		}
		if (rim & RIM_MAC_TIMES) {
			src += 8;
		}
		if (rim & RIM_LAST_ACCESS_TIME) {
			src += 2;
		}
		if (rim & RIM_UNKNOWN25) {
			src += 2;
		}
		if (rim & RIM_SIZE64) {
			src += 8;
		}
		if (rim & RIM_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 1 + BVAL(src, 0);
		}
	} else if (rim & 0x3FFFFFFF) {
#define gpt(Q) ((size_t)&((struct nw_info_struct*)NULL)->Q)
		src += gpt(nameLen);
		if (rim & RIM_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 1 + BVAL(src, 0);
		}
#undef gpt
	}
	if (src > srcend)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (end)
		*end = src;
	return 0;
}

struct NSI_Field {
	size_t start;
	size_t length;
	         };

static NWCCODE get_fields(
		const u_int8_t** end,
		u_int32_t rim, 
		const u_int8_t* src, 
		size_t srclen,
		struct NSI_Field fields[32]) {
	const u_int8_t* srcend = src + srclen;
	const u_int8_t* srcbase = src;

	memset(fields, 0, sizeof(struct NSI_Field) * 32);
#define PUTFIELD(x,len) do { \
				const size_t tmplen = len; \
				fields[NSIF_##x].start = src - srcbase; \
				fields[NSIF_##x].length = tmplen; \
				src += tmplen; \
			} while (0)
#define SIMPLEITEM(x,len) do { \
				if (rim & RIM_##x) \
					PUTFIELD(x,len); \
			} while (0)
#define gpt(Q) ((size_t)&((struct nw_info_struct*)NULL)->Q)
#define PUTFIELDFIELD(x,len,field) do { \
				fields[NSIF_##x].start = gpt(field); \
				fields[NSIF_##x].length = len; \
			} while (0)
#define SIMPLEITEMFIELD(x,len,field) do { \
				if (rim & RIM_##x) \
					PUTFIELDFIELD(x,len,field); \
			} while (0)
	if (rim & RIM_COMPRESSED_INFO) {
		SIMPLEITEM(SPACE_ALLOCATED, 4);
		SIMPLEITEM(ATTRIBUTES, 6);
		SIMPLEITEM(DATA_SIZE, 4);
		SIMPLEITEM(TOTAL_SIZE, 6);
		SIMPLEITEM(EXT_ATTR_INFO, 12);
		SIMPLEITEM(ARCHIVE, 8);
		SIMPLEITEM(MODIFY, 10);
		SIMPLEITEM(CREATION, 8);
		SIMPLEITEM(OWNING_NAMESPACE, 4);
		SIMPLEITEM(DIRECTORY, 12);
		SIMPLEITEM(RIGHTS, 2);
		SIMPLEITEM(REFERENCE_ID, 2);
		SIMPLEITEM(NS_ATTRIBUTES, 4);
		if (rim & RIM_DATASTREAM_SIZES) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			PUTFIELD(DATASTREAM_SIZES, 4 + 8 * DVAL_LH(src, 0));
		}
		if (rim & RIM_DATASTREAM_LOGICALS) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			PUTFIELD(DATASTREAM_LOGICALS, 4 + 8 * DVAL_LH(src, 0));
		}
		SIMPLEITEM(UPDATE_TIME, 4);
		if (rim & RIM_DOS_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			PUTFIELD(DOS_NAME, 1 + BVAL(src, 0));
		}
		SIMPLEITEM(FLUSH_TIME, 4);
		SIMPLEITEM(PARENT_BASE_ID, 4);
		SIMPLEITEM(MAC_FINDER_INFO, 32);
		SIMPLEITEM(SIBLING_COUNT, 4);
		SIMPLEITEM(EFFECTIVE_RIGHTS, 4);
		SIMPLEITEM(MAC_TIMES, 8);
		SIMPLEITEM(LAST_ACCESS_TIME, 2);
		SIMPLEITEM(UNKNOWN25, 2);
		SIMPLEITEM(SIZE64, 8);
		if (rim & RIM_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			PUTFIELD(NAME, 1 + BVAL(src, 0));
		}
	} else if (rim & 0x3FFFFFFF) {
		SIMPLEITEMFIELD(SPACE_ALLOCATED, 4, spaceAlloc);
		SIMPLEITEMFIELD(ATTRIBUTES, 6, attributes);
		SIMPLEITEMFIELD(DATA_SIZE, 4, dataStreamSize);
		SIMPLEITEMFIELD(TOTAL_SIZE, 6, totalStreamSize);
		SIMPLEITEMFIELD(EXT_ATTR_INFO, 12, EADataSize);
		SIMPLEITEMFIELD(ARCHIVE, 8, archiveTime);
		SIMPLEITEMFIELD(MODIFY, 10, modifyTime);
		SIMPLEITEMFIELD(CREATION, 8, creationTime);
		SIMPLEITEMFIELD(OWNING_NAMESPACE, 4, NSCreator);
		SIMPLEITEMFIELD(DIRECTORY, 12, dirEntNum);
		SIMPLEITEMFIELD(RIGHTS, 2, inheritedRightsMask);
		src += gpt(nameLen);
		if (rim & RIM_NAME) {
			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			PUTFIELD(NAME, 1 + BVAL(src, 0));
		}
	}
	if (src > srcend)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (end)
		*end = src;
	return 0;
#undef SIMPLEITEMFIELD
#undef PUTFIELDFIELD
#undef gpt
#undef SIMPLEITEM
#undef PUTFIELD
}

static NWCCODE get_file_info2(
		const u_int8_t** end,
		u_int32_t rim, 
		const u_int8_t* src, size_t srclen,
		struct nw_info_struct2* dest) {
	const u_int8_t* srcend = src + srclen;

	if (rim & RIM_COMPRESSED_INFO) {
		if (rim & RIM_SPACE_ALLOCATED) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->SpaceAllocated = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_ATTRIBUTES) {
			if (src + 6 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Attributes.Attributes = DVAL_LH(src, 0);
			dest->Attributes.Flags = WVAL_LH(src, 4);
			src += 6;
		}
		if (rim & RIM_DATA_SIZE) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->DataSize = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_TOTAL_SIZE) {
			if (src + 6 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->TotalSize.TotalAllocated = DVAL_LH(src, 0);
			dest->TotalSize.Datastreams = WVAL_LH(src, 4);
			src += 6;
		}
		if (rim & RIM_EXT_ATTR_INFO) {
			if (src + 12 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->ExtAttrInfo.DataSize = DVAL_LH(src, 0);
			dest->ExtAttrInfo.Count = DVAL_LH(src, 4);
			dest->ExtAttrInfo.KeySize = DVAL_LH(src, 8);
			src += 12;
		}
		if (rim & RIM_ARCHIVE) {
			if (src + 8 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Archive.Time = WVAL_LH(src, 0);
			dest->Archive.Date = WVAL_LH(src, 2);
			dest->Archive.ID = DVAL_HL(src, 4);
			src += 8;
		}
		if (rim & RIM_MODIFY) {
			if (src + 10 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Modify.Time = WVAL_LH(src, 0);
			dest->Modify.Date = WVAL_LH(src, 2);
			dest->Modify.ID = DVAL_HL(src, 4);
			dest->LastAccess.Date = WVAL_LH(src, 8);
			dest->LastAccess.Time = 0;
			src += 10;
		}
		if (rim & RIM_CREATION) {
			if (src + 8 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Creation.Time = WVAL_LH(src, 0);
			dest->Creation.Date = WVAL_LH(src, 2);
			dest->Creation.ID = DVAL_HL(src, 4);
			src += 8;
		}
		if (rim & RIM_OWNING_NAMESPACE) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->OwningNamespace = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_DIRECTORY) {
			if (src + 12 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Directory.dirEntNum = DVAL_LH(src, 0);
			dest->Directory.DosDirNum = DVAL_LH(src, 4);
			dest->Directory.volNumber = DVAL_LH(src, 8);
			src += 12;
		}
		if (rim & RIM_RIGHTS) {
			if (src + 2 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Rights = WVAL_LH(src, 0);
			src += 2;
		}
		if (rim & RIM_REFERENCE_ID) {
			if (src + 2 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->ReferenceID = WVAL_LH(src, 0);
			src += 2;
		}
		if (rim & RIM_NS_ATTRIBUTES) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->NSAttributes = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_DATASTREAM_SIZES) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 8 * DVAL_LH(src, 0) + 4;
			if (src > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		if (rim & RIM_DATASTREAM_LOGICALS) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			src += 8 * DVAL_LH(src, 0) + 4;
			if (src > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
		}
		if (rim & RIM_UPDATE_TIME) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->UpdateTime = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_DOS_NAME) {
			size_t len;

			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			len = dest->DOSName.NameLength = BVAL(src, 0);
			src++;
			if (src + len > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			memcpy(dest->DOSName.Name, src, len);
			dest->DOSName.Name[len] = 0;
			src += len;
		}
		if (rim & RIM_FLUSH_TIME) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->FlushTime = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_PARENT_BASE_ID) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->ParentBaseID = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_MAC_FINDER_INFO) {
			if (src + 32 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			memcpy(dest->MacFinderInfo, src, 32);
			src += 32;
		}
		if (rim & RIM_SIBLING_COUNT) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->SiblingCount = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_EFFECTIVE_RIGHTS) {
			if (src + 4 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->EffectiveRights = DVAL_LH(src, 0);
			src += 4;
		}
		if (rim & RIM_MAC_TIMES) {
			if (src + 8 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->MacTimes.CreateTime = DVAL_LH(src, 0);
			dest->MacTimes.BackupTime = DVAL_LH(src, 4);
			src += 8;
		}
		if (rim & RIM_LAST_ACCESS_TIME) {
			if (src + 2 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->LastAccess.Time = WVAL_LH(src, 0);
			src += 2;
		}
		if (rim & RIM_UNKNOWN25) {
			if (src + 2 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->Unknown25 = WVAL_LH(src, 0);
			src += 2;
		}
		if (rim & RIM_SIZE64) {
			if (src + 8 > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->DataSize = QVAL_LH(src, 0);
			src += 8;
		}
		if (rim & RIM_NAME) {
			size_t len;

			if (src >= srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			len = BVAL(src, 0);
			src++;
			if (src + len > srcend)
				return NWE_INVALID_NCP_PACKET_LENGTH;
			memcpy(dest->Name.Name, src, dest->Name.NameLength = len);
			dest->Name.Name[len] = 0;
			src += len;
		}
	} else {
#define gpt(Q) ((size_t)&((struct nw_info_struct*)NULL)->Q)
		if (rim & 0x3FFFFFFF) {	/* anything requested... */
			if (srclen < gpt(nameLen))
				return NWE_INVALID_NCP_PACKET_LENGTH;
			dest->SpaceAllocated = DVAL_LH(src, gpt(spaceAlloc));
			dest->Attributes.Attributes = DVAL_LH(src, gpt(attributes));
			dest->Attributes.Flags = WVAL_LH(src, gpt(flags));
			dest->DataSize = DVAL_LH(src, gpt(dataStreamSize));
			dest->TotalSize.TotalAllocated = DVAL_LH(src, gpt(totalStreamSize));
			dest->TotalSize.Datastreams = WVAL_LH(src, gpt(numberOfStreams));
			dest->Creation.Time = WVAL_LH(src, gpt(creationTime));
			dest->Creation.Date = WVAL_LH(src, gpt(creationDate));
			dest->Creation.ID = DVAL_HL(src, gpt(creatorID));
			dest->Modify.Time = WVAL_LH(src, gpt(modifyTime));
			dest->Modify.Date = WVAL_LH(src, gpt(modifyDate));
			dest->Modify.ID = DVAL_HL(src, gpt(modifierID));
			dest->LastAccess.Date = WVAL_LH(src, gpt(lastAccessDate));
			dest->LastAccess.Time = 0;
			dest->Archive.Time = WVAL_LH(src, gpt(archiveTime));
			dest->Archive.Date = WVAL_LH(src, gpt(archiveDate));
			dest->Archive.ID = DVAL_HL(src, gpt(archiverID));
			dest->Rights = WVAL_LH(src, gpt(inheritedRightsMask));
			dest->Directory.dirEntNum = DVAL_LH(src, gpt(dirEntNum));
			dest->Directory.DosDirNum = DVAL_LH(src, gpt(DosDirNum));
			dest->Directory.volNumber = DVAL_LH(src, gpt(volNumber));
			dest->ExtAttrInfo.DataSize = DVAL_LH(src, gpt(EADataSize));
			dest->ExtAttrInfo.Count = DVAL_LH(src, gpt(EAKeyCount));
			dest->ExtAttrInfo.KeySize = DVAL_LH(src, gpt(EAKeySize));
			dest->OwningNamespace = DVAL_LH(src, gpt(NSCreator));
			src += gpt(nameLen);
			if (rim & RIM_NAME) {
				size_t len;

				if (src >= srcend)
					return NWE_INVALID_NCP_PACKET_LENGTH;
				len = BVAL(src, 0);
				src++;
				if (src + len > srcend)
					return NWE_INVALID_NCP_PACKET_LENGTH;
				memcpy(dest->Name.Name, src, dest->Name.NameLength = len);
				dest->Name.Name[len] = 0;
				src += len;
			}
		}
#undef gpt
	}
	if (end)
		*end = src;
	return 0;
}

struct NSI_FileInfoReply {
	u_int32_t	 flag;
	struct NSI_Field fields[32];
	u_int8_t	 data[0];
		    };

static NWCCODE get_file_info3(
		const u_int8_t** end,
		u_int32_t rim,
		const u_int8_t* src, size_t srclen,
		struct nw_info_struct3* dest) {
	const u_int8_t* srcend;
	NWCCODE err;
	struct NSI_Field fields[32];
	struct NSI_FileInfoReply *rp;
	size_t needlen;
	
	err = get_fields(&srcend, rim, src, srclen, fields);
	if (err)
		return err;
	srclen = srcend - src;
	needlen = srclen + sizeof(struct NSI_FileInfoReply);
	if (dest->data) {
		if (dest->len < needlen) {
			return E2BIG;
		}
		rp = dest->data;
	} else {
		rp = malloc(needlen);
		if (!rp)
			return ENOMEM;
		dest->data = rp;
	}
	dest->len = needlen;
	rp->flag = 0x0000F120;
	memcpy(rp->fields, fields, sizeof(rp->fields));
	memcpy(rp->data, src, srclen);
	if (end)
		*end = srcend;
	return err;
}

/* It MUST be possible to call this with rim==0 && dest==NULL (&& sizedest==0) ! */
static NWCCODE ncp_ns_extract_file_info(
		const u_int8_t** end,
		u_int32_t rim, 
		const u_int8_t* src, size_t srclen,
		void* dest, size_t sizedest) {
	if (dest == NULL || sizedest == 0) {
		return get_file_info_size(end, rim, src, srclen);
	}
	switch (sizedest) {
		case sizeof(struct nw_info_struct2):
			return get_file_info2(end, rim, src, srclen,
				dest);
		case sizeof(struct nw_info_struct3):
			return get_file_info3(end, rim, src, srclen,
				dest);
	}
	return NWE_BUFFER_INVALID_LEN;
}

static const size_t field_sizes[32] = {
	/* NSIF_NAME */			sizeof(size_t),  /* + */
	/* NSIF_SPACE_ALLOCATED */	sizeof(u_int32_t),
	/* NSIF_ATTRIBUTES */		sizeof(struct NSI_Attributes),
	/* NSIF_DATA_SIZE */		sizeof(u_int32_t),
	/* NSIF_TOTAL_SIZE */		sizeof(struct NSI_TotalSize),
	/* NSIF_EXT_ATTR_INFO */	sizeof(struct NSI_ExtAttrInfo),
	/* NSIF_ARCHIVE */		sizeof(struct NSI_Change),
	/* NSIF_MODIFY */		sizeof(struct NSI_Modify),
	/* NSIF_CREATION */		sizeof(struct NSI_Change),
	/* NSIF_OWNING_NAMESPACE */	sizeof(u_int8_t),
	/* NSIF_DIRECTORY */		sizeof(struct NSI_Directory),
	/* NSIF_RIGHTS */		sizeof(u_int16_t),
	/* NSIF_REFERENCE_ID */		sizeof(u_int16_t),
	/* NSIF_NS_ATTRIBUTES */	sizeof(u_int32_t),
	/* NSIF_DATASTREAM_SIZES */	sizeof(u_int32_t), /* + */
	/* NSIF_DATASTREAM_LOGICALS */	sizeof(u_int32_t), /* + */
	/* NSIF_UPDATE_TIME */		sizeof(int32_t),
	/* NSIF_DOS_NAME */		sizeof(size_t),  /* + */
	/* NSIF_FLUSH_TIME */		sizeof(u_int32_t),
	/* NSIF_PARENT_BASE_ID */	sizeof(u_int32_t),
	/* NSIF_MAC_FINDER_INFO */	32,
	/* NSIF_SIBLING_COUNT */	sizeof(u_int32_t),
	/* NSIF_EFFECTIVE_RIGHTS */	sizeof(u_int16_t),
	/* NSIF_MAC_TIMES */		sizeof(struct NSI_MacTimes),
	/* NSIF_LAST_ACCESS_TIME */	sizeof(struct NSI_Modify),
	/* NSIF_UNKNOWN25 */		sizeof(u_int16_t),
	/* NSIF_SIZE64 */		sizeof(u_int64_t),
};

NWCCODE ncp_ns_extract_info_field_size(
		const struct nw_info_struct3* rq,
		u_int32_t field,
		size_t* destlen) {
	size_t needsize;
	struct NSI_FileInfoReply* rp;
	const u_int8_t* src;
	size_t ln = 0;
		
	if (!rq)
		return NWE_PARAM_INVALID;
	if (!rq->data)
		return NWE_PARAM_INVALID;
	if (rq->len < sizeof(struct NSI_FileInfoReply))
		return NWE_PARAM_INVALID;
	if (field >= 32)
		return NWE_PARAM_INVALID;
	rp = rq->data;
	if (rp->flag != 0x0000F120)
		return NWE_PARAM_INVALID;

	if (field == NSIF_LAST_ACCESS_TIME)
		field = NSIF_MODIFY;
	if (!rp->fields[field].length)
		return NCPLIB_INFORMATION_NOT_KNOWN;
	src = rp->data + rp->fields[field].start;
	needsize = field_sizes[field];
	switch (field) {
		case NSIF_NAME:
		case NSIF_DOS_NAME:
			ln = BVAL(src, 0);
			needsize = sizeof(struct NSI_Name) - 256 + 1 + ln;
			break;
		case NSIF_DATASTREAM_SIZES:
			ln = DVAL_LH(src, 0);
			needsize = offsetof(struct NSI_DatastreamSizes, ds[ln]);
			break;
		case NSIF_DATASTREAM_LOGICALS:
			ln = DVAL_LH(src, 0);
			needsize = offsetof(struct NSI_DatastreamLogicals, ds[ln]);
			break;
	}
	if (destlen)
		*destlen = needsize;
	return 0;
}

NWCCODE ncp_ns_extract_info_field(
		const struct nw_info_struct3* rq,
		u_int32_t field,
		void* dest, size_t destlen) {
	size_t needsize;
	struct NSI_FileInfoReply* rp;
	const u_int8_t* src;
	u_int64_t val;
	size_t ln = 0;
		
	if (!rq)
		return NWE_PARAM_INVALID;
	if (!rq->data)
		return NWE_PARAM_INVALID;
	if (rq->len < sizeof(struct NSI_FileInfoReply))
		return NWE_PARAM_INVALID;
	if (field >= 32)
		return NWE_PARAM_INVALID;
	rp = rq->data;
	if (rp->flag != 0x0000F120)
		return NWE_PARAM_INVALID;

	if (field == NSIF_LAST_ACCESS_TIME)
		field = NSIF_MODIFY;
	if (!rp->fields[field].length)
		return NCPLIB_INFORMATION_NOT_KNOWN;
	src = rp->data + rp->fields[field].start;
	needsize = field_sizes[field];
	switch (field) {
		case NSIF_NAME:
		case NSIF_DOS_NAME:
			ln = BVAL(src, 0);
			needsize = sizeof(struct NSI_Name) - 256 + 1 + ln;
			break;
		case NSIF_DATASTREAM_SIZES:
			ln = DVAL_LH(src, 0);
			needsize = offsetof(struct NSI_DatastreamSizes, ds[ln]);
			break;
		case NSIF_DATASTREAM_LOGICALS:
			ln = DVAL_LH(src, 0);
			needsize = offsetof(struct NSI_DatastreamLogicals, ds[ln]);
			break;
	}
	if (destlen < needsize)
		return NWE_BUFFER_OVERFLOW;

	switch (field) {
		case NSIF_NAME:
		case NSIF_DOS_NAME:
			((struct NSI_Name*)dest)->NameLength = ln;
			memcpy(((struct NSI_Name*)dest)->Name, src + 1, ln);
			((struct NSI_Name*)dest)->Name[ln] = 0;
			return 0;
		case NSIF_SPACE_ALLOCATED:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_ATTRIBUTES:
			((struct NSI_Attributes*)dest)->Attributes =
				DVAL_LH(src, 0);
			((struct NSI_Attributes*)dest)->Flags =
				WVAL_LH(src, 4);
			return 0;
		case NSIF_DATA_SIZE:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_TOTAL_SIZE:
			((struct NSI_TotalSize*)dest)->TotalAllocated = 
				DVAL_LH(src, 0);
			((struct NSI_TotalSize*)dest)->Datastreams =
				WVAL_LH(src, 4);
			return 0;
		case NSIF_EXT_ATTR_INFO:
			((struct NSI_ExtAttrInfo*)dest)->DataSize =
				DVAL_LH(src, 0);
			((struct NSI_ExtAttrInfo*)dest)->Count =
				DVAL_LH(src, 4);
			((struct NSI_ExtAttrInfo*)dest)->KeySize =
				DVAL_LH(src, 8);
			return 0;
		case NSIF_ARCHIVE:
		case NSIF_CREATION:
			((struct NSI_Change*)dest)->Time =
				WVAL_LH(src, 0);
			((struct NSI_Change*)dest)->Date =
				WVAL_LH(src, 2);
			((struct NSI_Change*)dest)->ID =
				DVAL_HL(src, 4);
			return 0;
		case NSIF_MODIFY:
			((struct NSI_Modify*)dest)->Modify.Time =
				WVAL_LH(src, 0);
			((struct NSI_Modify*)dest)->Modify.Date =
				WVAL_LH(src, 2);
			((struct NSI_Modify*)dest)->Modify.ID =
				DVAL_HL(src, 4);
			((struct NSI_Modify*)dest)->LastAccess.Date =
				WVAL_LH(src, 8);
			((struct NSI_Modify*)dest)->LastAccess.ID = 0;
			if (rp->fields[NSIF_LAST_ACCESS_TIME].length) {
				((struct NSI_Modify*)dest)->LastAccess.Time =
					WVAL_LH(rp->data, rp->fields[NSIF_LAST_ACCESS_TIME].start);
			} else {
				((struct NSI_Modify*)dest)->LastAccess.Time = 0;
			}
			return 0;
		case NSIF_OWNING_NAMESPACE:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_DIRECTORY:
			((struct NSI_Directory*)dest)->dirEntNum = 
				DVAL_LH(src, 0);
			((struct NSI_Directory*)dest)->DosDirNum = 
				DVAL_LH(src, 4);
			((struct NSI_Directory*)dest)->volNumber = 
				DVAL_LH(src, 8);
			return 0;
		case NSIF_RIGHTS:
			val = WVAL_LH(src, 0);
			break;
		case NSIF_REFERENCE_ID:
			val = WVAL_LH(src, 0);
			break;
		case NSIF_NS_ATTRIBUTES:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_DATASTREAM_SIZES:
			{
				struct NSI_DatastreamSizes* si = dest;
				struct NSI_DatastreamFATInfo* ds;
				
				si->NumberOfDatastreams = ln;
				ds = si->ds;
				src += 4;
				while (ln--) {
					ds->Number = DVAL_LH(src, 0);
					ds->FATBlockSize = DVAL_LH(src, 4);
					ds++;
					src += 8;
				}
			}
			return 0;
		case NSIF_DATASTREAM_LOGICALS:
			{
				struct NSI_DatastreamLogicals* si = dest;
				struct NSI_DatastreamInfo* ds;
				
				si->NumberOfDatastreams = ln;
				ds = si->ds;
				src += 4;
				while (ln--) {
					ds->Number = DVAL_LH(src, 0);
					ds->Size = DVAL_LH(src, 4);
					ds++;
					src += 8;
				}
			}
			return 0;
		case NSIF_UPDATE_TIME:
			val = (int64_t)(int32_t)(DVAL_LH(src, 0));
			break;
		case NSIF_FLUSH_TIME:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_PARENT_BASE_ID:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_MAC_FINDER_INFO:
			memcpy(dest, src, 32);
			return 0;
		case NSIF_SIBLING_COUNT:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_EFFECTIVE_RIGHTS:
			val = DVAL_LH(src, 0);
			break;
		case NSIF_MAC_TIMES:
			((struct NSI_MacTimes*)dest)->CreateTime =
				DVAL_LH(src, 0);
			((struct NSI_MacTimes*)dest)->BackupTime =
				DVAL_LH(src, 4);
			return 0;
		case NSIF_UNKNOWN25:
			val = WVAL_LH(src, 0);
			break;
		case NSIF_SIZE64:
			val = QVAL_LH(src, 0);
			break;
		default:
			return NWE_PARAM_INVALID;
	}
	switch (destlen) {
		case 1:
			*(u_int8_t*)dest = val;
			break;
		case 2:
			*(u_int16_t*)dest = val;
			break;
		case 4:
			*(u_int32_t*)dest = val;
			break;
		case 8:
			*(u_int64_t*)dest = val;
			break;
		default:
			return NWE_PARAM_INVALID;
	}
	return 0;
}

NWCCODE
ncp_ns_obtain_entry_info(struct ncp_conn *conn,
			 unsigned int source_ns,
			 unsigned int search_attribs,
			 int dir_style,
			 unsigned int vol, 
			 u_int32_t dirent, 
			 const unsigned char *encpath, size_t pathlen, 
			 unsigned int target_ns,
			 u_int32_t rim,
			 /* struct nw_info_struct2 */ void *target, size_t sizeoftarget)
{
	NWCCODE result;

	ncp_init_request(conn);
	ncp_add_byte(conn, 6);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_word_lh(conn, search_attribs);
	ncp_add_dword_lh(conn, rim);
	result = ncp_add_handle_path2(conn, vol, dirent, dir_style, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 6 */
	if ((result = ncp_request(conn, 87)) != 0) {
		goto quit;
	}
	result = ncp_ns_extract_file_info(NULL, rim, 
			ncp_reply_data(conn, 0), conn->ncp_reply_size, 
			target, sizeoftarget);
quit:;
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_open_create_entry(struct ncp_conn *conn,
				/* input */
				unsigned int ns,
				unsigned int search_attributes,
				int dirstyle,
				unsigned int vol,
				u_int32_t dirent,
				const unsigned char* encpath, size_t pathlen,
				/* open specific */
				int datastream,
				int open_create_mode,
				u_int32_t create_attributes,
				u_int16_t desired_access_rights,
				/* what to return */
				u_int32_t rim,
				/* returned */
				/* struct nw_info_struct2 */ void* target, size_t sizeoftarget,
				u_int8_t* oc_action,
				u_int8_t* oc_callback,
				char file_handle[6])
{
	NWCCODE result;
	u_int32_t fhandle;

	ncp_init_request(conn);
	if (datastream == -1) {
		if (open_create_mode & OC_MODE_CALLBACK) {
			ncp_add_byte(conn, 32);
		} else {
			ncp_add_byte(conn, 1);
		}
		ncp_add_byte(conn, ns);
		ncp_add_byte(conn, open_create_mode);
		ncp_add_word_lh(conn, search_attributes);
	} else {
		if (open_create_mode & OC_MODE_CALLBACK) {
			ncp_add_byte(conn, 33);
		} else {
			ncp_add_byte(conn, 30);
		}
		ncp_add_byte(conn, ns);
		ncp_add_byte(conn, datastream);
		ncp_add_byte(conn, open_create_mode);		/* word_lh? */
		ncp_add_byte(conn, 0);				/* reserved */
		ncp_add_word_lh(conn, search_attributes);	/* dword_lh? */
		ncp_add_word_lh(conn, 0);			/* reserved */
	}
	ncp_add_dword_lh(conn, rim);
	ncp_add_dword_lh(conn, create_attributes);
	ncp_add_word_lh(conn, desired_access_rights);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 32/1 or 33/30 */
	if ((result = ncp_request(conn, 87)) != 0) {
quit:;	
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 6) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	fhandle = ncp_reply_dword_lh(conn, 0);
	if (oc_action)
		*oc_action = ncp_reply_byte(conn, 4);
	if (oc_callback)
		*oc_callback = (open_create_mode & OC_MODE_CALLBACK) ? ncp_reply_byte(conn, 5) : 0;
	result = ncp_ns_extract_file_info(NULL, rim, 
		ncp_reply_data(conn, 6), conn->ncp_reply_size - 6, 
		target, sizeoftarget);
	ncp_unlock_conn(conn);
	if (file_handle)
		ConvertToNWfromDWORD(fhandle, file_handle);
	return result;
}

NWCCODE
ncp_ns_modify_entry_dos_info(struct ncp_conn *conn,
				    /* input */
				    unsigned int ns,
				    unsigned int search_attributes,
				    int dirstyle,
				    unsigned int vol,
				    u_int32_t dirent,
				    const unsigned char* encpath, size_t pathlen,
				    /* what to do with entry */
				    u_int32_t mim,
				    const struct ncp_dos_info* info) 
{
	NWCCODE result;

	if (!info) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 7);
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_dword_lh(conn, mim);
	ncp_add_dword_lh(conn, info->Attributes);	/* do it faster?! */
	ncp_add_word_lh(conn, info->Creation.Date);
	ncp_add_word_lh(conn, info->Creation.Time);
	ncp_add_dword_hl(conn, info->Creation.ID);
	ncp_add_word_lh(conn, info->Modify.Date);
	ncp_add_word_lh(conn, info->Modify.Time);
	ncp_add_dword_hl(conn, info->Modify.ID);
	ncp_add_word_lh(conn, info->Archive.Date);
	ncp_add_word_lh(conn, info->Archive.Time);
	ncp_add_dword_hl(conn, info->Archive.ID);
	ncp_add_word_lh(conn, info->LastAccess.Date);
	ncp_add_word_lh(conn, info->Rights.Grant);
	ncp_add_word_lh(conn, info->Rights.Revoke);
	ncp_add_dword_lh(conn, info->MaximumSpace);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 7 */
	result = ncp_request(conn, 87);
quit:;	
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_obtain_namespace_info_format(struct ncp_conn *conn,
				  /* input */
				  unsigned int vol,
				  /* where to act */
				  unsigned int target_ns,
				  /* return buffer */
				  struct ncp_namespace_format* format, UNUSED(size_t sizeofformat)) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 23);
	ncp_add_byte(conn, target_ns);
	ncp_add_byte(conn, vol);
	/* fn: 87 , subfn: 23 */
	result = ncp_request(conn, 87);
	if (!result) {
		if (conn->ncp_reply_size < 146) {
			result = NWE_INVALID_NCP_PACKET_LENGTH;
		} else {
			void* data = ncp_reply_data(conn, 0);
			int i;

			format->Version = 0;
			format->BitMask.fixed = DVAL_LH(data, 0);
			format->BitMask.variable = DVAL_LH(data, 4);
			format->BitMask.huge = DVAL_LH(data, 8);
			format->BitsDefined.fixed = WVAL_LH(data, 12);
			format->BitsDefined.variable = WVAL_LH(data, 14);
			format->BitsDefined.huge = WVAL_LH(data, 16);
			for (i = 0; i < 32; i++)
				format->FieldsLength[i] = DVAL_LH(data, 18+i*4);
		}
	}
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_obtain_entry_namespace_info(struct ncp_conn *conn,
					    /* input */
					    unsigned int source_ns,
					    unsigned int vol,
					    u_int32_t dirent,
					    /* where to act */
					    unsigned int target_ns,
					    /* what to return */
					    u_int32_t nsrim,
					    /* return buffer */
					    void* buffer, size_t* len, size_t maxlen) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 19);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_byte(conn, 0);		/* ? */
	ncp_add_byte(conn, vol);
	ncp_add_dword_lh(conn, dirent);
	ncp_add_dword_lh(conn, nsrim);
	/* fn: 87 , subfn: 19 */	
	result = ncp_request(conn, 87);
	if (!result) {
		/* do this... */
		if (conn->ncp_reply_size > maxlen)
			result = NWE_BUFFER_OVERFLOW;
		else {
			if (len)
				*len = conn->ncp_reply_size;
			if (buffer)
				memcpy(buffer, ncp_reply_data(conn, 0), conn->ncp_reply_size);
		}
	}
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_modify_entry_namespace_info(struct ncp_conn *conn,
					  /* input */
					  unsigned int source_ns,
					  unsigned int vol,
					  u_int32_t dirent,
					  /* where to act */
					  unsigned int target_ns,
					  /* what to set */
					  u_int32_t nsrim,
					  /* data buffer */
					  const void* buffer, size_t buflen) {
	
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 25);
	ncp_add_byte(conn, source_ns);
	ncp_add_byte(conn, target_ns);
	ncp_add_byte(conn, vol);
	ncp_add_dword_lh(conn, dirent);
	ncp_add_dword_lh(conn, nsrim);
	ncp_add_mem(conn, buffer, buflen);
	/* fn: 87 , subfn: 25 */	
	result = ncp_request(conn, 87);
	ncp_unlock_conn(conn);
	return result;
}
				       
NWCCODE
ncp_ns_get_namespace_info_element(const struct ncp_namespace_format* nsformat,
			       u_int32_t nsrim,
			       const void* buffer,
			       size_t bufferlen,
			       unsigned int itemid,
			       void* item, size_t* itemlen, size_t itemmaxlen) {
	u_int32_t mask;
	size_t pos;
	u_int32_t mask2;
	const size_t* entlen;
	
	if (!nsformat) {
		return ERR_NULL_POINTER;
	}
	if (nsformat->Version != 0)
		return NWE_INVALID_LEVEL;
	if (itemid >= 32)
		return NWE_PARAM_INVALID;
	mask = 1 << itemid;
	if (!(mask & nsrim))
		return NCPLIB_INFORMATION_NOT_KNOWN;
	entlen = nsformat->FieldsLength;
	if (!entlen) {
		return ERR_NULL_POINTER;
	}
	pos = 0;
	for (mask2 = 1; mask2 < mask; entlen++, mask2 <<= 1) {
		if (mask2 & nsrim) {
			if (nsformat->BitMask.variable & mask2) {
				if (pos >= bufferlen)
					return NWE_BUFFER_INVALID_LEN;
				if (!buffer)
					return ERR_NULL_POINTER;
				pos += BVAL(buffer, 0) + 1;
			} else if (nsformat->BitMask.huge & mask2)
				return NCPLIB_NSFORMAT_INVALID;
			else
				pos += *entlen;	/* not defined and fixed */
			if (pos > bufferlen)
				return NWE_BUFFER_INVALID_LEN;
		}
	}
	if (nsformat->BitMask.huge & mask)
		return NCPLIB_NSFORMAT_INVALID;
	else {
		size_t len;
		
		if (nsformat->BitMask.variable & mask) {
			if (pos >= bufferlen)
				return NWE_BUFFER_INVALID_LEN;
			if (!buffer)
				return ERR_NULL_POINTER;
			len = BVAL(buffer, 0) + 1;
		} else
			len = *entlen;
		if (pos + len > bufferlen)
			return NWE_BUFFER_INVALID_LEN;
		if (itemmaxlen < len)
			return NWE_BUFFER_OVERFLOW;
		if (itemlen)
			*itemlen = len;
		if (item) {
			if (!buffer)
				return ERR_NULL_POINTER;
			memcpy(item, ((const u_int8_t*)buffer)+pos, len);
		}
	}
	return 0;
}

NWCCODE
ncp_ns_trustee_add(struct ncp_conn *conn,
		   /* input */
		   unsigned int ns,
		   unsigned int search_attributes,
		   int dirstyle,
		   unsigned int vol,
		   u_int32_t dirent,
		   const unsigned char* encpath, size_t pathlen,
		   /* trustee_add specific */
		   const TRUSTEE_INFO* trustees,
		   unsigned int object_count,
		   u_int16_t rights_mask)
{
	NWCCODE result;

	if (object_count && !trustees) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 10);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_word_lh(conn, rights_mask);
	ncp_add_word_lh(conn, object_count);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	if (ncp_add_seek(conn, 16 + 307)) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	while (object_count--)
	{
		ncp_add_dword_hl(conn, trustees->objectID);
		ncp_add_word_lh(conn, trustees->objectRights);
		trustees += 1;
	}
	/* fn: 87 , subfn: 10 */
	result = ncp_request(conn, 87);
quit:;	
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_trustee_del(struct ncp_conn *conn,
		   /* input */
		   unsigned int ns,
		   int dirstyle,
		   unsigned int vol,
		   u_int32_t dirent,
		   const unsigned char* encpath, size_t pathlen,
		   /* trustee_del specific */
		   const TRUSTEE_INFO* trustees,
		   unsigned int object_count)
{
	NWCCODE result;

	if (object_count && !trustees) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 11);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, object_count);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	if (ncp_add_seek(conn, 12 + 307)) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	while (object_count--)
	{
		ncp_add_dword_hl(conn, trustees->objectID);
		ncp_add_word_lh(conn, trustees->objectRights);
		trustees += 1;
	}
	/* fn: 87 , subfn: 11 */
	result = ncp_request(conn, 87);
quit:;	
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_ns_trustee_scan(struct ncp_conn *conn,
		    /* input */
		    unsigned int ns,
		    unsigned int search_attributes,
		    int dirstyle,
		    unsigned int vol,
		    u_int32_t dirent,
		    const unsigned char* encpath, size_t pathlen,
		    /* trustee_scan specific */
		    u_int32_t* iter,
		    TRUSTEE_INFO* trustees,
		    unsigned int *object_count)
{
	NWCCODE result;
	unsigned int objcnt;
	int pos;
	
	if (!iter || !object_count || !trustees) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request(conn);
	ncp_add_byte(conn, 5);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_dword_lh(conn, *iter);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 5 */
	if ((result = ncp_request(conn, 87)) != 0) {
		goto quit;
	}
	if (conn->ncp_reply_size < 6) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	objcnt = ncp_reply_word_lh(conn, 4);
	if (conn->ncp_reply_size < 6 * objcnt + 6) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*iter = ncp_reply_dword_lh(conn, 0);
	if (objcnt > *object_count) {
		objcnt = *object_count;
		result = NWE_BUFFER_OVERFLOW;
	} else
		*object_count = objcnt;
	pos = 6;
	while (objcnt) {
		trustees->objectID = ncp_reply_dword_hl(conn, pos);
		trustees->objectRights = ncp_reply_word_lh(conn, pos + 4);
		trustees++;
		pos += 6;
		objcnt--;
	}
quit:;
	ncp_unlock_conn(conn);
	return result;
}

/* obsolete */
long
ncp_add_trustee_set(struct ncp_conn *conn,
		    u_int8_t volume_number, 
		    u_int32_t dir_entry,
		    u_int16_t rights_mask,
		    int object_count, 
		    const struct ncp_trustee_struct *rights)
{
	return ncp_ns_trustee_add(conn, NW_NS_DOS, SA_ALL, NCP_DIRSTYLE_DIRBASE,
		volume_number, dir_entry, NULL, 0, 
		(const TRUSTEE_INFO*)rights, object_count, rights_mask);
}

NWCCODE ncp_ns_alloc_short_dir_handle(struct ncp_conn *conn,
				      /* input */
				      unsigned int ns,
				      int dirstyle,
				      unsigned int vol,
		 		      u_int32_t dirent,
				      const unsigned char* encpath, size_t pathlen,
				      /* alloc short dir handle specific */
				      unsigned int allocate_mode,
				      /* output */
				      NWDIR_HANDLE *dirhandle,
				      NWVOL_NUM *ovol) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 12);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_word_lh(conn, allocate_mode);
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, pathlen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 12 */
	if ((result = ncp_request(conn, 87)) != 0) {
		goto quit;
	}
	/* NCP call returns 6 bytes, but we use first 2 only... */
	if (conn->ncp_reply_size < 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (dirhandle)
		*dirhandle = ncp_reply_byte(conn, 0);
	if (ovol)
		*ovol = ncp_reply_byte(conn, 1);
quit:;		
	ncp_unlock_conn(conn);
	return result;
}

/* volume services */
NWCCODE NWGetNSLoadedList(struct ncp_conn *conn,
			  NWVOL_NUM volume,
			  size_t maxNamespaces,
			  unsigned char* namespaces,
			  size_t *actual) {
	NWCCODE result;
	size_t nses;
	
	if (volume > 255) {
		return NWE_VOL_INVALID;
	}	
	ncp_init_request(conn);
	ncp_add_byte(conn, 24);
	ncp_add_word_lh(conn, 0);
	ncp_add_byte(conn, volume);
	/* fn: 87 , subfn: 24 */
	result = ncp_request(conn, 87);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 2) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	nses = ncp_reply_word_lh(conn, 0);
	if (conn->ncp_reply_size < 2 + nses) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (namespaces) {
		if (nses > maxNamespaces)
			result = NWE_BUFFER_OVERFLOW;
		else
			memcpy(namespaces, ncp_reply_data(conn, 2), nses);
	}
	ncp_unlock_conn(conn);
	if (actual)
		*actual = nses;
	return result;
}

static NWCCODE
ncp_get_mount_volume_list_compat(struct ncp_conn *conn,
				 unsigned int ns,
				 unsigned int flags,
				 unsigned int *volnum,
				 unsigned int *itemcnt,
				 void* b, size_t* blen) {
	unsigned int vol;
	unsigned char* buffer = b;
	unsigned int items = 0;
	size_t needSize;
	unsigned char* bend = buffer + *blen;

	if (flags & 1) {
		needSize = 4 + 1 + 17;
	} else {
		needSize = 4;
	}

	if (buffer + needSize > bend) {
		return NWE_BUFFER_OVERFLOW;
	}
				 
	for (vol = *volnum; buffer + needSize <= bend && vol < 256; vol++) {
		unsigned char nspc[256];
		size_t real;
		NWCCODE err;
		
		err = NWGetNSLoadedList(conn, vol, sizeof(nspc), nspc, &real);
		if (!err) {
			if (memchr(nspc, ns, real)) {
				DSET_LH(buffer, 0, vol);
				if (flags & 1) {
					err = ncp_get_volume_name(conn, vol, buffer+5, 17);
					if (!err) {
						real = strlen(buffer+5);
						buffer[4] = real;
						buffer += 4 + 1 + real;
						items++;
					}
				} else {
					buffer += 4;
					items++;
				}
			}
		}
	}
	if (items == 0) {
		return NWE_SERVER_FAILURE;
	}
	*itemcnt = items;
	*blen = buffer - (unsigned char*)b;
	return 0;
}

static NWCCODE
ncp_get_mount_volume_list(struct ncp_conn *conn,
			  unsigned int ns,
			  unsigned int flags,
			  unsigned int *volnum,
			  unsigned int *itemcnt,
			  void* buffer, size_t* blen) {
	NWCCODE result;
	unsigned int items;
	size_t slen;
	
	ncp_init_request_s(conn, 52);
	ncp_add_dword_lh(conn, *volnum);
	ncp_add_dword_lh(conn, flags);
	ncp_add_dword_lh(conn, ns);
	result = ncp_request(conn, 22);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 8) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	slen = conn->ncp_reply_size - 8;
	items = ncp_reply_dword_lh(conn, 0);
	if (slen < ((flags & 1)?6:4) * items) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	*itemcnt = items;
	*volnum = ncp_reply_dword_lh(conn, 4);
	if (slen > *blen) {
		slen = *blen;
		result = NWE_BUFFER_OVERFLOW;
	} else
		*blen = slen;
	if (buffer)
		memcpy(buffer, ncp_reply_data(conn, 8), slen);
	ncp_unlock_conn(conn);
	return result;
}

/* volume listing functions */

struct ncp_volume_list_handle {
	struct ncp_conn *conn;
	u_int32_t nextvol;
	unsigned int ns;
	unsigned int flags;
	NWCCODE err;
	unsigned int simple;
	unsigned int itemcnt;
	unsigned char* bufpos;
	unsigned char* buffer;
	unsigned char* bufend;
	ncpt_mutex_t mutex;
};
	
NWCCODE
ncp_volume_list_init(struct ncp_conn *conn,
		     unsigned int ns,
		     unsigned int flags,
		     NWVOL_HANDLE* handle) {
	NWCCODE result;
	NWVOL_HANDLE h;
	u_int16_t ver;
	
	if (!handle) {
		return ERR_NULL_POINTER;
	}	
	h = (NWVOL_HANDLE)malloc(sizeof(struct ncp_volume_list_handle));
	if (!h)
		return ENOMEM;
	ncp_conn_store(conn);	/* prevent release connection, but allow disconnect... */
	h->conn = conn;
	h->nextvol = 0;
	h->ns = ns;
	h->flags = flags;
	h->itemcnt = 0;
	h->err = 0;
	result = NWGetFileServerVersion(conn, &ver);
	h->simple = result || (ver < 0x0400);
	ncpt_mutex_init(&h->mutex);
	*handle = h;
	return 0;
}

static NWCCODE
__ncp_volume_list_one(NWVOL_HANDLE h,
		      unsigned char** cptr,
		      unsigned char* end,
		      unsigned int* volnum,
		      char* volume,
		      size_t maxlen) {
	unsigned char* curr = *cptr;
	
	if (curr + 4 > end)
		return NWE_INVALID_NCP_PACKET_LENGTH;
	if (volnum)
		*volnum = DVAL_LH(curr, 0);
	curr += 4;
	if (h->flags & 1) {
		size_t namel;
		
		if (curr >= end)
			return NWE_INVALID_NCP_PACKET_LENGTH;
		namel = *curr++;
		if (curr + namel > end)
			return NWE_INVALID_NCP_PACKET_LENGTH;
		if (namel >= maxlen)
			return NWE_BUFFER_OVERFLOW;
		if (volume) {
			memcpy(volume, curr, namel);
			volume[namel] = 0;
		}
		curr += namel;
	}
	*cptr = curr;
	return 0;		
}

NWCCODE
ncp_volume_list_next(NWVOL_HANDLE h, 
		     unsigned int* volnum,
		     char* volume, size_t maxlen) {
	NWCCODE result;

	if (!h) {
		return ERR_NULL_POINTER;
	}
	ncpt_mutex_lock(&h->mutex);
	if (!h->itemcnt) {
		unsigned int itemcnt;
		unsigned char buffer[1024];
		size_t blen = sizeof(buffer);
		char* b;
				
		if (h->err) {
			result = h->err;
			goto quit;
		}
		if (h->simple)
			result = ncp_get_mount_volume_list_compat(h->conn,
				h->ns, h->flags & 0x0001,
				&h->nextvol,
				&itemcnt,
				buffer, &blen);
		else
			result = ncp_get_mount_volume_list(h->conn, 
				h->ns, 
				h->flags & 0x0001, 
				&h->nextvol,
				&itemcnt,
				buffer, &blen);
		if (result) {
			h->err = result;
			goto quit;
		}
		if (!itemcnt) {
			result = NWE_SERVER_FAILURE;
			goto quit;
		}
			
		/* let's build buffer */
		b = (char*)malloc(blen);
		if (!b) {
			result = ENOMEM;
			goto quit;
		}
		memcpy(b, buffer, blen);
		h->bufpos = h->buffer = b;
		h->bufend = b + blen;
		h->itemcnt = itemcnt;
		if (!h->nextvol)
			h->err = NWE_SERVER_FAILURE;
	}
	result = __ncp_volume_list_one(h, &h->bufpos, h->bufend, volnum, volume, maxlen);
	if (result)
		goto quit;
	if (!--h->itemcnt)
		free(h->buffer);
quit:;	
	ncpt_mutex_unlock(&h->mutex);
	return result;
}

NWCCODE
ncp_volume_list_end(NWVOL_HANDLE h) {
	if (h) {
		ncpt_mutex_lock(&h->mutex);
		if (h->itemcnt)
			free(h->buffer);
		ncp_conn_release(h->conn);
		ncpt_mutex_destroy(&h->mutex);
		free(h);
	}
	return 0;
}

/* directory listing functions */

static NWCCODE
__ncp_ns_search_init(struct ncp_conn* conn,
		     /* input */
		     unsigned int ns,
		     unsigned int dirstyle,
		     unsigned int vol,
		     NWDIR_ENTRY dirent,
		     const unsigned char* encpath, size_t enclen,
		     /* output */
		     struct nw_search_sequence* seq) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 2);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, 0);	/* reserved */
	result = ncp_add_handle_path2(conn, vol, dirent, dirstyle, encpath, enclen);
	if (result) {
		goto quit;
	}
	/* fn: 87 , subfn: 2 */
	if ((result = ncp_request(conn, 87)) != 0) {
		goto quit;
	}
	if (conn->ncp_reply_size < 9) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (seq)
		memcpy(seq, ncp_reply_data(conn, 0), 9);
quit:;		
	ncp_unlock_conn(conn);
	return result;
}

static NWCCODE
__ncp_ns_search_next(struct ncp_conn* conn,
		     /* input */
		     unsigned int ns,
		     int datastream,
		     unsigned int search_attributes,
		     /* search next specific */
		     struct nw_search_sequence* seq,
		     u_int32_t rim,
		     const unsigned char* pattern, size_t patlen,
		     /* output */
		     void* buffer, size_t* blen) {
	NWCCODE result;
	size_t ulen;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 3);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, datastream);
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_dword_lh(conn, rim);
	ncp_add_mem(conn, seq, 9);
	ncp_add_mem(conn, pattern, patlen);
	/* fn: 87 , subfn: 3 */
	if ((result = ncp_request(conn, 87)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 10) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	ulen = conn->ncp_reply_size - 10;
	if (buffer) {
		if (*blen < ulen) {
			ncp_unlock_conn(conn);
			return NWE_BUFFER_OVERFLOW;
		}
		memcpy(buffer, ncp_reply_data(conn, 10), ulen);
	}
	*blen = ulen;
	memcpy(seq, ncp_reply_data(conn, 0), 9);
	ncp_unlock_conn(conn);
	return result;
}		     
		     
static NWCCODE
__ncp_ns_search_next_set(struct ncp_conn* conn,
			 /* input */
			 unsigned int ns,
			 int datastream,
			 unsigned int search_attributes,
			 /* search next specific */
			 struct nw_search_sequence* seq,
			 u_int32_t rim,
			 const unsigned char* pattern, size_t patlen,
			 /* output */
			 unsigned int *itemcnt,
			 void* buffer, size_t* blen, unsigned char* more) {
	NWCCODE result;
	size_t ulen;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 20);	/* subfunction */
	ncp_add_byte(conn, ns);
	ncp_add_byte(conn, datastream);
	ncp_add_word_lh(conn, search_attributes);
	ncp_add_dword_lh(conn, rim);
	ncp_add_word_lh(conn, *itemcnt);
	ncp_add_mem(conn, seq, 9);
	ncp_add_mem(conn, pattern, patlen);
	/* fn: 87 , subfn: 20 */
	if ((result = ncp_request(conn, 87)) != 0) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 12) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	ulen = conn->ncp_reply_size - 12;
	if (buffer) {
		if (*blen < ulen) {
			ncp_unlock_conn(conn);
			return NWE_BUFFER_OVERFLOW;
		}
		memcpy(buffer, ncp_reply_data(conn, 12), ulen);
	}
	*blen = ulen;
	*itemcnt = ncp_reply_word_lh(conn, 10);
	if (more)
		*more = ncp_reply_byte(conn, 9);
	memcpy(seq, ncp_reply_data(conn, 0), 9);
	ncp_unlock_conn(conn);
	return result;
}		     
		     
struct ncp_directory_list_handle {
	struct ncp_conn *conn;
	ncpt_mutex_t mutex;
	struct nw_search_sequence searchseq;
	NWCCODE err;
	unsigned int ns;
	unsigned int search_attr;
	int datastream;
	u_int32_t rim;
	unsigned int searchset;
	unsigned int itemcnt;
	const unsigned char* bufpos;
	unsigned char buffer[65536];
	const unsigned char* bufend;
	unsigned char noteof;
	size_t patlen;
	unsigned char pattern[1];
};

NWCCODE
ncp_ns_search_init(struct ncp_conn* conn,
		   /* input */
		   unsigned int ns,
		   unsigned int search_attributes,
		   unsigned int dirstyle,
		   unsigned int vol,
		   NWDIR_ENTRY dirent,
		   const unsigned char* encpath, size_t enclen,
		   /* search specific */
		   int datastream,
		   const unsigned char* pattern, size_t patlen,
		   u_int32_t rim,
		   /* handle */
		   NWDIRLIST_HANDLE* rhandle) {
	NWDIRLIST_HANDLE h;
	NWCCODE err;
	struct nw_search_sequence sseq;
	
	if (!rhandle) {
		return ERR_NULL_POINTER;
	}
	err = __ncp_ns_search_init(conn, ns, dirstyle, vol, dirent,
			encpath, enclen, &sseq);
	if (err)
		return err;
	if (!pattern)
		patlen = 0;
	h = (NWDIRLIST_HANDLE)malloc(sizeof(*h) + patlen);
	if (!h)
		return ENOMEM;
	ncp_conn_store(conn);
	ncpt_mutex_init(&h->mutex);
	h->conn = conn;
	h->searchseq = sseq;
	h->err = 0;
	h->ns = ns;
	h->search_attr = search_attributes;
	h->datastream = datastream;
	h->patlen = patlen + 1;
	h->pattern[0] = patlen;
	h->itemcnt = 0;
	h->searchset = 1;
//	h->searchset = 0;
	if (h->searchset) {
		/* searchset always returns NAME */
		rim |= RIM_NAME;
	}
	h->rim = rim;
	h->noteof = 1;
	if (patlen)
		memcpy(h->pattern + 1, pattern, patlen);
	*rhandle = h;
	return 0;
}

NWCCODE
ncp_ns_search_next(NWDIRLIST_HANDLE h,
		   /* struct nw_info_struct2 */ void* target, size_t sizeoftarget) {
	NWCCODE err;
	const unsigned char* p;
	
	if (!h) {
		return ERR_NULL_POINTER;
	}
	ncpt_mutex_lock(&h->mutex);
	if (!h->itemcnt) {
		size_t blen;
		
		if (!h->noteof) {
			err = NWE_SERVER_FAILURE;
			goto q;
		}
		blen = sizeof(h->buffer);
		if (h->searchset) {
			unsigned int itemcnt = 200;
			
			err = __ncp_ns_search_next_set(h->conn, h->ns, h->datastream, h->search_attr,
				&h->searchseq, h->rim, h->pattern, h->patlen,
				&itemcnt, h->buffer, &blen, &h->noteof);
			if (err)
				goto q;
			if (!itemcnt) {
				err = NWE_SERVER_FAILURE;
				goto q;
			}
			h->itemcnt = itemcnt;
		} else {
			h->rim |= RIM_NAME;
			err = __ncp_ns_search_next(h->conn, h->ns, h->datastream, h->search_attr,
				&h->searchseq, h->rim, h->pattern, h->patlen,
				h->buffer, &blen);
			if (err)
				goto q;
			h->itemcnt = 1;
		}
		h->bufpos = h->buffer;
		h->bufend = h->buffer + blen;
	}
	err = ncp_ns_extract_file_info(&p, h->rim, 
			h->bufpos, h->bufend - h->bufpos, target, sizeoftarget);
	switch (err) {
		case E2BIG:
		case ENOMEM:
			/* leave buffer intouch for retry */
			break;
		case 0:
			/* item successfully returned */
			h->bufpos = p;
			h->itemcnt--;
			break;
		default:
			/* invalid reply from server - skip to next one */
			h->itemcnt = 0;
			break;
	}
q:;
	ncpt_mutex_unlock(&h->mutex);
	return err;
}

NWCCODE
ncp_ns_search_end(NWDIRLIST_HANDLE h) {
	if (h) {
		ncpt_mutex_lock(&h->mutex);
		ncp_conn_release(h->conn);
		ncpt_mutex_destroy(&h->mutex);
		free(h);
	}
	return 0;
}

static NWCCODE ncp_get_file_size_32(
		NWCONN_HANDLE conn,
		const char fileHandle[6],
		ncp_off64_t* fileSize) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 0);	/* reserved */
	ncp_add_mem(conn, fileHandle, 6);
	/* fn: 71 */
	result = ncp_request(conn, 71);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 4) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (fileSize)
		*fileSize = ncp_reply_dword_hl(conn, 0);
	ncp_unlock_conn(conn);
	return result;
}

static NWCCODE ncp_get_file_size_64(
		NWCONN_HANDLE conn,
		u_int32_t fileHandle,
		ncp_off64_t* fileSize) {
	NWCCODE result;
	
	ncp_init_request(conn);
	ncp_add_byte(conn, 66);
	ncp_add_dword_lh(conn, fileHandle);
	/* fn: 87,66 */
	result = ncp_request(conn, 87);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 8) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (fileSize)
		*fileSize = ncp_reply_qword_lh(conn, 0);
	ncp_unlock_conn(conn);
	return result;
}

NWCCODE
ncp_get_file_size(
		NWCONN_HANDLE conn,
		const char fileHandle[6],
		ncp_off64_t* fileSize) {
	NWCCODE result;
	
	result = __NWReadFileServerInfo(conn);
	if (result) {
		return result;
	}
	if (conn->serverInfo.ncp64bit) {
		u_int32_t fh = ConvertToDWORDfromNW(fileHandle);
		return ncp_get_file_size_64(conn, fh, fileSize);
	} else {
		return ncp_get_file_size_32(conn, fileHandle, fileSize);
	}
}

static u_int8_t*
__ncp_openfile_fetch(OPEN_FILE_CONN *openFile, u_int8_t* finfo, u_int8_t* end) {
	size_t nm;

	if (finfo + 17 > end) {
		return NULL;
	}
	nm = BVAL(finfo, 16);
	if (finfo + 17 + nm > end) {
		return NULL;
	}
	if (nm < 1) {
		return NULL;
	}
	openFile->taskNumber = WVAL_LH(finfo, 0);
	openFile->lockType = BVAL(finfo, 2); 
	openFile->accessControl = BVAL(finfo, 3); 
	openFile->lockFlag = BVAL(finfo, 4);
	openFile->volNumber = BVAL(finfo, 5);
	openFile->parent = DVAL_LH(finfo, 6);
	openFile->dirEntry = DVAL_LH(finfo, 10);
	openFile->forkCount = BVAL(finfo, 14);
	openFile->nameSpace = BVAL(finfo, 15);
	openFile->nameLen = nm;
	memcpy(openFile->fileName, finfo + 17, nm);
	openFile->fileName[nm] = 0;
	return finfo + 17 + nm;
}

NWCCODE
NWScanOpenFilesByConn2(
		NWCONN_HANDLE conn,
		NWCONN_NUM connNum,
		u_int16_t* iterHandle,
		OPEN_FILE_CONN_CTRL *openCtrl,
		OPEN_FILE_CONN *openFile) {
	NWCCODE result;

	if (!iterHandle || !openCtrl || !openFile) {
		return NWE_PARAM_INVALID;
	}
	if (*iterHandle == 0) {
		openCtrl->nextRequest = 0;
		openCtrl->openCount = 0;
		openCtrl->curRecord = 0;
	} else if (openCtrl->openCount > 0) {
		u_int8_t* np;
		u_int8_t* finfo = openCtrl->buffer + openCtrl->curRecord;
		u_int8_t* maxf = openCtrl->buffer + sizeof(openCtrl->buffer);
			
		np = __ncp_openfile_fetch(openFile, finfo, maxf);
		if (!np) {
			goto baddata;
		}
		openCtrl->curRecord = np - openCtrl->buffer;
		goto finish;
	} else if (openCtrl->nextRequest == 0) {
		return NWE_REQUESTER_FAILURE;
	}
	ncp_init_request_s(conn, 235);
	ncp_add_word_lh(conn, connNum);
	ncp_add_word_lh(conn, openCtrl->nextRequest);
	result = ncp_request(conn, 23);
	if (result) {
		ncp_unlock_conn(conn);
		goto abrt;
	}
	if (conn->ncp_reply_size < 4) {
		ncp_unlock_conn(conn);
		result = NWE_INVALID_NCP_PACKET_LENGTH;
		goto abrt;
	}
	openCtrl->nextRequest = ncp_reply_word_lh(conn, 0);
	openCtrl->openCount = ncp_reply_word_lh(conn, 2);
	if (openCtrl->openCount == 0) {
		ncp_unlock_conn(conn);
		result = NWE_REQUESTER_FAILURE;
		goto abrt;
	}
	{
		size_t tocopy;
		u_int8_t* np;
		u_int8_t* maxf = ncp_reply_data(conn, 0) + conn->ncp_reply_size;
		
		np = __ncp_openfile_fetch(openFile, ncp_reply_data(conn, 4), maxf);
		if (!np) {
			ncp_unlock_conn(conn);
			goto baddata;
		}
		tocopy = maxf - np;
		if (tocopy > sizeof(openCtrl->buffer)) {
			ncp_unlock_conn(conn);
			goto baddata;
		}
		memcpy(openCtrl->buffer, np, tocopy);
	}
	openCtrl->curRecord = 0;
	ncp_unlock_conn(conn);
finish:;
	openCtrl->openCount--;
	if (openCtrl->openCount == 0 && openCtrl->nextRequest == 0) {
		*iterHandle = 0xFFFF;
	} else {
		*iterHandle = 0x0001;
	}
	return 0;
baddata:;
	result = NWE_BUFFER_INVALID_LEN;
abrt:;
	openCtrl->nextRequest = 0;
	openCtrl->openCount = 0;
	*iterHandle = 0xFFFF;
	return result;
}

NWCCODE
ncp_ns_scan_connections_using_file(
		NWCONN_HANDLE conn,
		NWVOL_NUM vol,
		NWDIR_ENTRY dirent,
		int datastream,
		u_int16_t *iterHandle,
		CONN_USING_FILE *cf,
		CONNS_USING_FILE *cfa) {
	unsigned int ih;
	NWCCODE result;
	unsigned int pos;
	unsigned int cnt;
	CONN_USING_FILE* cuf;

	if (!iterHandle || !cfa) {
		return NWE_PARAM_INVALID;
	}
	ih = *iterHandle;
	if (ih == 0) {
		cfa->nextRequest = 0;
		cfa->connCount = 0;
	} else if (ih < cfa->connCount) {
		if (!cf)
			return NWE_PARAM_INVALID;
		*cf = cfa->connInfo[ih];
		ih++;
		if (ih >= cfa->connCount && cfa->nextRequest == 0) {
			ih = 0xFFFF;
		}
		*iterHandle = ih;
		return 0;
	} else if (cfa->nextRequest == 0) {
		*iterHandle = 0xFFFF;
		return NWE_REQUESTER_FAILURE;
	}
	ncp_init_request_s(conn, 236);
	ncp_add_byte(conn, datastream);
	ncp_add_byte(conn, vol);
	ncp_add_dword_lh(conn, dirent);
	ncp_add_word_lh(conn, cfa->nextRequest);
	result = ncp_request(conn, 23);
	if (result) {
		ncp_unlock_conn(conn);
		goto abrt;
	}
	if (conn->ncp_reply_size < 18) {
		ncp_unlock_conn(conn);
		result = NWE_INVALID_NCP_PACKET_LENGTH;
		goto abrt;
	}
	cfa->nextRequest = ncp_reply_word_lh(conn, 0);
	cfa->useCount = ncp_reply_word_lh(conn, 2);
	cfa->openCount = ncp_reply_word_lh(conn, 4);
	cfa->openForReadCount = ncp_reply_word_lh(conn, 6);
	cfa->openForWriteCount = ncp_reply_word_lh(conn, 8);
	cfa->denyReadCount = ncp_reply_word_lh(conn, 10);
	cfa->denyWriteCount = ncp_reply_word_lh(conn, 12);
	cfa->locked = ncp_reply_byte(conn, 14);
	cfa->forkCount = ncp_reply_byte(conn, 15);
	cnt = cfa->connCount = ncp_reply_word_lh(conn, 16);
	if (cnt) {
		if (cnt > 70) {
			ncp_unlock_conn(conn);
			result = NWE_BUFFER_OVERFLOW;
			goto abrt;
		}
		pos = 18;
		cuf = cfa->connInfo;
		while (cnt--) {
			if (conn->ncp_reply_size < pos + 7) {
				ncp_unlock_conn(conn);
				result = NWE_INVALID_NCP_PACKET_LENGTH;
				goto abrt;
			}
			cuf->connNumber = ncp_reply_word_lh(conn, pos);
			cuf->taskNumber = ncp_reply_word_lh(conn, pos + 2);
			cuf->lockType = ncp_reply_byte(conn, pos + 4);
			cuf->accessControl = ncp_reply_byte(conn, pos + 5);
			cuf->lockFlag = ncp_reply_byte(conn, pos + 6);
			cuf++;
			pos += 7;
		}
		cnt = 1;
	}
	ncp_unlock_conn(conn);
	if (cnt && cf) {
		*cf = cfa->connInfo[0];
		ih = 1;
	} else if (cnt) {
		ih = cfa->connCount;
	} else {
		cfa->nextRequest = 0;
		ih = 0xFFFF;
	}
	if (ih >= cfa->connCount && cfa->nextRequest == 0) {
		ih = 0xFFFF;
	}
	*iterHandle = ih;
	return 0;
abrt:;
	cfa->nextRequest = 0;
	cfa->openCount = 0;
	*iterHandle = 0xFFFF;
	return result;
}

NWCCODE
ncp_ns_scan_physical_locks_by_file(
			NWCONN_HANDLE	conn,
			NWVOL_NUM	vol,
			NWDIR_ENTRY	dirent,
			int		datastream,
			u_int16_t	*iterHandle,
			PHYSICAL_LOCK	*lock,
			PHYSICAL_LOCKS	*locks) {
	unsigned int ih;
	NWCCODE result;
	unsigned int pos;
	unsigned int cnt;
	PHYSICAL_LOCK* currlock;

	if (!iterHandle || !locks) {
		return NWE_PARAM_INVALID;
	}
	ih = *iterHandle;
	if (ih == 0) {
		locks->nextRequest = 0;
		locks->numRecords = 0;
	} else if (ih < locks->numRecords) {
		if (!lock)
			return NWE_PARAM_INVALID;
		*lock = locks->locks[ih];
		ih++;
		if (ih >= locks->numRecords && locks->nextRequest == 0) {
			ih = 0xFFFF;
		}
		*iterHandle = ih;
		return 0;
	} else if (locks->nextRequest == 0) {
		*iterHandle = 0xFFFF;
		return NWE_REQUESTER_FAILURE;
	}
	ncp_init_request_s(conn, 238);
	ncp_add_byte(conn, datastream);
	ncp_add_byte(conn, vol);
	ncp_add_dword_lh(conn, dirent);
	ncp_add_word_lh(conn, locks->nextRequest);
	result = ncp_request(conn, 23);
	if (result) {
		ncp_unlock_conn(conn);
		goto abrt;
	}
	if (conn->ncp_reply_size < 4) {
		ncp_unlock_conn(conn);
		result = NWE_INVALID_NCP_PACKET_LENGTH;
		goto abrt;
	}
	locks->nextRequest = ncp_reply_word_lh(conn, 0);
	cnt = locks->numRecords = ncp_reply_word_lh(conn, 2);
	if (cnt) {
		if (cnt > 32) {
			ncp_unlock_conn(conn);
			result = NWE_BUFFER_OVERFLOW;
			goto abrt;
		}
		pos = 4;
		currlock = locks->locks;
		while (cnt--) {
			if (conn->ncp_reply_size < pos + 17) {
				ncp_unlock_conn(conn);
				result = NWE_INVALID_NCP_PACKET_LENGTH;
				goto abrt;
			}
			currlock->loggedCount = ncp_reply_word_lh(conn, pos);
			currlock->shareableLockCount = ncp_reply_word_lh(conn, pos + 2);
			currlock->recordStart = ncp_reply_dword_lh(conn, pos + 4);
			currlock->recordEnd = ncp_reply_dword_lh(conn, pos + 8);
			currlock->connNumber = ncp_reply_word_lh(conn, pos + 12);
			currlock->taskNumber = ncp_reply_word_lh(conn, pos + 14);
			currlock->lockType = ncp_reply_byte(conn, pos + 16);
			currlock++;
			pos += 17;
		}
		cnt = 1;
	}
	ncp_unlock_conn(conn);
	if (cnt && lock) {
		*lock = locks->locks[0];
		ih = 1;
	} else if (cnt) {
		ih = locks->numRecords;
	} else {
		locks->nextRequest = 0;
		ih = 0xFFFF;
	}
	if (ih >= locks->numRecords && locks->nextRequest == 0) {
		ih = 0xFFFF;
	}
	*iterHandle = ih;
	return 0;
abrt:;
	locks->nextRequest = 0;
	locks->numRecords = 0;
	*iterHandle = 0xFFFF;
	return result;
}

NWCCODE NWGetDirSpaceLimitList(
		NWCONN_HANDLE conn,
		NWDIR_HANDLE dir_handle,
		nuint8* buffer) {
	NWCCODE result;
	size_t items;

	if (!buffer) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 35);
	ncp_add_byte(conn, dir_handle);
	result = ncp_request(conn, 22);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	items = ncp_reply_byte(conn, 0) * 9 + 1;
	if (conn->ncp_reply_size < items) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (items > 512) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	memcpy(buffer, ncp_reply_data(conn, 0), items);
	ncp_unlock_conn(conn);
	return 0;
}

NWCCODE NWGetDirSpaceLimitList2(
		NWCONN_HANDLE conn,
		NWDIR_HANDLE dir_handle,
		NW_LIMIT_LIST* limitlist) {
	NWCCODE result;
	unsigned int items;
	unsigned int cnt;

	if (!limitlist) {
		return ERR_NULL_POINTER;
	}
	ncp_init_request_s(conn, 35);
	ncp_add_byte(conn, dir_handle);
	result = ncp_request(conn, 22);
	if (result) {
		ncp_unlock_conn(conn);
		return result;
	}
	if (conn->ncp_reply_size < 1) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	items = ncp_reply_byte(conn, 0);
	if (conn->ncp_reply_size < 1 + 9 * items) {
		ncp_unlock_conn(conn);
		return NWE_INVALID_NCP_PACKET_LENGTH;
	}
	if (items > 102) {
		ncp_unlock_conn(conn);
		return NWE_BUFFER_OVERFLOW;
	}
	limitlist->numEntries = items;
	for (cnt = 0; cnt < items; cnt++) {
		limitlist->list[cnt].level = ncp_reply_byte(conn, cnt * 9 + 1);
		limitlist->list[cnt].max = ncp_reply_dword_lh(conn, cnt * 9 + 1 + 1);
		limitlist->list[cnt].current = ncp_reply_dword_lh(conn, cnt * 9 + 1 + 5);
	}	
	ncp_unlock_conn(conn);
	return 0;
}

