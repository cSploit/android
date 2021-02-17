/*
    ncpsign.h
    Copyright (C) 1997  Arne de Bruijn <arne@knoware.nl>
  
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

	0.00  1997			Arne de Bruijn <arne@knoware.nl>
		Initial revision.

 */
 
#ifndef _NCPSIGN_H
#define _NCPSIGN_H

#include "ncplib_i.h"

#ifdef SIGNATURES
void sign_init(const unsigned char *logindata, unsigned char *sign_root);
int sign_verify_reply(struct ncp_conn *conn, const void *data, size_t size, u_int32_t totalsize, const void *sign_buf);
#else
static inline void sign_init(const unsigned char *logindata, unsigned char *sign_root) {
	(void)logindata;
	(void)sign_root;
}
int sign_verify_reply(struct ncp_conn *conn, const void *data, size_t size, u_int32_t totalsize, const void *sign_buf) {
	return 1;
	(void)conn;
	(void)data;
	(void)size;
	(void)totalsize;
	(void)sign_buf;
}
#endif

void __sign_packet(struct ncp_conn *conn, const void *data, size_t size, u_int32_t totalsize, void *sign_buf);

static inline size_t sign_packet(struct ncp_conn *conn, const void *data, size_t size, u_int32_t totalsize, void *sign_buf) {
	if (ncp_get_sign_active(conn)) {
		__sign_packet(conn, data, size, totalsize, sign_buf);
		return 8;
	}
	return 0;
}

#endif
