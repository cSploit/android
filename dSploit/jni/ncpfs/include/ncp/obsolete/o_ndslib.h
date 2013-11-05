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
*/

#ifndef _O_NDSLIB_H_
#define _O_NDSLIB_H_

typedef unsigned short uni_char;

/* old compat code */
#define NDS_GRACE_PERIOD 0x89DF

#ifdef __cplusplus
extern "C" {
#endif

int strlen_u(const uni_char *s);
uni_char getchr_u(const uni_char *s);
void strcpy_uc(char *d, const uni_char *s);
void strcpy_cu(uni_char *d, const char *s);
long nds_get_server_name(NWCONN_HANDLE conn, uni_char **server_name);
long nds_get_tree_name(NWCONN_HANDLE conn, char *name, int name_buf_len);
long nds_resolve_name(struct ncp_conn *conn, int flags, uni_char *entry_name,
	int *entry_id, int *remote, struct sockaddr *serv_addr, size_t *addr_len);
long nds_read(NWCONN_HANDLE conn, u_int32_t object_id, uni_char *prop_name,
	u_int32_t *syntax_id, void **outbuf, size_t *outlen);

#ifdef __cplusplus
}
#endif

#endif /* ifndef _O_NDSLIB_H_ */
