/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdarg.h>
#include <stdio.h>
#include <assert.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif /* HAVE_SYS_TYPES_H */

#if HAVE_SYS_SOCKET_H
#include <sys/socket.h>
#endif /* HAVE_SYS_SOCKET_H */

#if HAVE_NETINET_IN_H
#include <netinet/in.h>
#endif /* HAVE_NETINET_IN_H */

#if HAVE_ARPA_INET_H
#include <arpa/inet.h>
#endif /* HAVE_ARPA_INET_H */

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/server.h>
#include <freetds/string.h>

static char software_version[] = "$Id: login.c,v 1.59 2012-03-11 15:52:22 freddy77 Exp $";
static void *no_unused_var_warn[] = { software_version, no_unused_var_warn };

unsigned char *
tds7_decrypt_pass(const unsigned char *crypt_pass, int len, unsigned char *clear_pass)
{
	int i;
	const unsigned char xormask = 0x5A;
	unsigned char hi_nibble, lo_nibble;

	for (i = 0; i < len; i++) {
		lo_nibble = (crypt_pass[i] << 4) ^ (xormask & 0xF0);
		hi_nibble = (crypt_pass[i] >> 4) ^ (xormask & 0x0F);
		clear_pass[i] = hi_nibble | lo_nibble;
	}
	return clear_pass;
}

TDSSOCKET *
tds_listen(TDSCONTEXT * ctx, int ip_port)
{
	TDSSOCKET *tds;
	struct sockaddr_in sin;
	TDS_SYS_SOCKET fd, s;
	socklen_t len;

	sin.sin_addr.s_addr = INADDR_ANY;
	sin.sin_port = htons((short) ip_port);
	sin.sin_family = AF_INET;

	s = socket(AF_INET, SOCK_STREAM, 0);
	if (TDS_IS_SOCKET_INVALID(s)) {
		perror("socket");
		return NULL;
	}
	if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		CLOSESOCKET(s);
		perror("bind");
		return NULL;
	}
	listen(s, 5);
	len = sizeof(sin);
	fd = tds_accept(s, (struct sockaddr *) &sin, &len);
	if (TDS_IS_SOCKET_INVALID(fd)) {
		CLOSESOCKET(s);
		perror("accept");
		return NULL;
	}
	CLOSESOCKET(s);
	tds = tds_alloc_socket(ctx, 8192);
	tds_set_s(tds, fd);
	tds->out_flag = TDS_LOGIN;
	/* TODO proper charset */
	tds_iconv_open(tds->conn, "ISO8859-1");
	/* get_incoming(tds->s); */
	return tds;
}

static void tds_read_string(TDSSOCKET * tds, DSTR * s, int size);

void
tds_read_login(TDSSOCKET * tds, TDSLOGIN * login)
{
	DSTR blockstr;
	TDS_USMALLINT major;

/*
	while (len = tds_read_packet(tds)) {
		for (i=0;i<len;i++)
			printf("%d %d %c\n",i, tds->in_buf[i], (tds->in_buf[i]>=' ' && tds->in_buf[i]<='z') ? tds->in_buf[i] : ' ');
	}	
*/
	tds_dstr_init(&blockstr);
	tds_read_string(tds, &login->client_host_name, 30);
	tds_read_string(tds, &login->user_name, 30);
	tds_read_string(tds, &login->password, 30);
	tds_get_n(tds, NULL, 31);	/* host process, junk for now */
	tds_get_n(tds, NULL, 16);	/* magic */
	tds_read_string(tds, &login->app_name, 30);
	tds_read_string(tds, &login->server_name, 30);
	tds_get_n(tds, NULL, 256);	/* secondary passwd...encryption? */
	major = tds_get_byte(tds);
	login->tds_version = (major << 8) | tds_get_byte(tds);
	tds_get_smallint(tds);	/* unused part of protocol field */
	tds_read_string(tds, &login->library, 10);
	tds_get_byte(tds);	/* program version, junk it */
	tds_get_byte(tds);
	tds_get_smallint(tds);
	tds_get_n(tds, NULL, 3);	/* magic */
	tds_read_string(tds, &login->language, 30);
	tds_get_n(tds, NULL, 14);	/* magic */
	tds_read_string(tds, &login->server_charset, 30);
	tds_get_n(tds, NULL, 1);	/* magic */
	tds_read_string(tds, &blockstr, 6);
	printf("block size %s\n", tds_dstr_cstr(&blockstr));
	login->block_size = atoi(tds_dstr_cstr(&blockstr));
	tds_dstr_free(&blockstr);
	tds_get_n(tds, NULL, tds->in_len - tds->in_pos);	/* read junk at end */
}

static void
tds7_read_string(TDSSOCKET * tds, DSTR *s, int len)
{
	tds_dstr_alloc(s, len);
	/* FIXME possible truncation on char conversion ? */
	len = tds_get_string(tds, len, tds_dstr_buf(s), len);
	tds_dstr_setlen(s, len);
}

int
tds7_read_login(TDSSOCKET * tds, TDSLOGIN * login)
{
	int a;
	int host_name_len, user_name_len, app_name_len, server_name_len;
	int library_name_len, language_name_len;
	int auth_len, database_name_len;
	size_t unicode_len, password_len;
	char *unicode_string, *psrc;
	char *pbuf;
	DSTR database;

	tds_dstr_init(&database);

	a = tds_get_int(tds);	/*total packet size */
	a = tds_get_int(tds);	/*TDS version */
	a &= 0xff;
	login->tds_version = ((a << 4) & 0xff00) | (a & 0xf);
	a = tds_get_int(tds);	/*desired packet size being requested by client */
	tds_get_n(tds, NULL, 24);	/*magic1 */
	a = tds_get_smallint(tds);	/*current position */
	host_name_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	user_name_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	password_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	app_name_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	server_name_len = tds_get_smallint(tds);
	tds_get_smallint(tds);
	tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	library_name_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);	/*current position */
	language_name_len = tds_get_smallint(tds);
	a = tds_get_smallint(tds);
	database_name_len = tds_get_smallint(tds);
	tds_get_n(tds, NULL, 6);	/* client mac address */
	a = tds_get_smallint(tds);	/*partial packet size */
	auth_len = tds_get_smallint(tds);	/*authentication len */
	a = tds_get_smallint(tds);	/*total packet size */
	tds_get_smallint(tds);

	tds7_read_string(tds, &login->client_host_name, host_name_len);
	tds7_read_string(tds, &login->user_name, user_name_len);

	unicode_len = password_len * 2;
	unicode_string = (char *) malloc(unicode_len);
	tds_dstr_alloc(&login->password, password_len);
	tds_get_n(tds, unicode_string, unicode_len);
	tds7_decrypt_pass((unsigned char *) unicode_string, unicode_len, (unsigned char *) unicode_string);
	pbuf = tds_dstr_buf(&login->password);
	
	memset(&tds->conn->char_convs[client2ucs2]->suppress, 0, sizeof(tds->conn->char_convs[client2ucs2]->suppress));
	psrc = unicode_string;
	a = tds_iconv(tds, tds->conn->char_convs[client2ucs2], to_client, (const char **) &psrc, &unicode_len, &pbuf,
			 &password_len);
	if (a < 0 ) {
		fprintf(stderr, "error: %s:%d: tds7_read_login: tds_iconv() failed\n", __FILE__, __LINE__);
		assert(-1 != a);
	}
	tds_dstr_setlen(&login->password, pbuf - tds_dstr_buf(&login->password));
	free(unicode_string);

	tds7_read_string(tds, &login->app_name, app_name_len);
	tds7_read_string(tds, &login->server_name, server_name_len);
	tds7_read_string(tds, &login->library, library_name_len);
	tds7_read_string(tds, &login->language, language_name_len);
	/* TODO use it */
	tds7_read_string(tds, &database, database_name_len);
	tds_dstr_free(&database);

	tds_get_n(tds, NULL, auth_len);

	tds_dstr_copy(&login->server_charset, "");	/*empty char_set for TDS 7.0 */
	login->block_size = 0;	/*0 block size for TDS 7.0 */
	login->encryption_level = TDS_ENCRYPTION_OFF;
	return (0);

}
static void
tds_read_string(TDSSOCKET * tds, DSTR * s, int size)
{
	int len;

	/* FIXME this can fails... */
	tds_dstr_alloc(s, size);
	tds_get_n(tds, tds_dstr_buf(s), size);
	len = tds_get_byte(tds);
	if (len <= size)
		tds_dstr_setlen(s, len);
}


/**
 * Allocate a TDSLOGIN structure, read a login packet into it, and return it.
 * This is smart enough to distinguish between TDS4/5 or TDS7.  The calling
 * function should call tds_free_login() on the returned structure when it is
 * no longer needed.
 * \param tds  The socket to read from
 * \return  Returns NULL if no login was received.  The calling function can
 * use IS_TDSDEAD(tds) to distinguish between an error/shutdown on the socket,
 * or the receipt of an unexpected packet type.  In the latter case,
 * tds->in_flag will indicate the return type.
 */
TDSLOGIN *
tds_alloc_read_login(TDSSOCKET * tds)
{
	TDSLOGIN * login;

	/*
	 * This should only be done on a server connection, and the server
	 * always sends 0x04 packets.
	 */
	tds->out_flag = TDS_REPLY;

	/* Pre-read the next packet so we know what kind of packet it is */
	if (tds_read_packet(tds) < 1) {
		return NULL;
	}

	/* Allocate the login packet */
	login = tds_alloc_login(1);
	if (!login)
		return NULL;

	/* Use the packet type to determine which login format to expect */
	switch (tds->in_flag) {
	case 0x02: /* TDS4/5 login */
		tds->conn->tds_version = 0x402;
		tds_read_login(tds, login);
		if (login->block_size == 0) {
			login->block_size = 512;
		}
		break;

	case 0x10: /* TDS7+ login */
		tds->conn->tds_version = 0x700;
		tds7_read_login(tds, login);
		break;

	case 0x12: /* TDS7.1+ prelogin, hopefully followed by a login */
		tds->conn->tds_version = 0x701;
		/* ignore client and just send our reply TODO... finish */
		tds71_send_prelogin(tds);
		tds_flush_packet(tds);
		if (tds_read_packet(tds) < 0 || tds->in_flag != 0x10) {
			tds_free_login(login);
			return NULL;
		}
		tds7_read_login(tds, login);
		break;

	default:
		/* unexpected packet */
		tds_free_login(login);
		return NULL;
	}

	/* Return it */
	return login;
}
