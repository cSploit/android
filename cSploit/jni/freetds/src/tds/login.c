/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2005-2010  Ziglio Frediano
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include <assert.h>

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#if HAVE_UNISTD_H
#include <unistd.h>
#endif /* HAVE_UNISTD_H */

#ifdef _WIN32
#include <process.h>
#endif

#include <freetds/tds.h>
#include <freetds/iconv.h>
#include <freetds/string.h>
#include "replacements.h"
#ifdef DMALLOC
#include <dmalloc.h>
#endif

TDS_RCSID(var, "$Id: login.c,v 1.224 2011-09-25 11:40:45 freddy77 Exp $");

static TDSRET tds_send_login(TDSSOCKET * tds, TDSLOGIN * login);
static TDSRET tds71_do_login(TDSSOCKET * tds, TDSLOGIN * login);
static TDSRET tds7_send_login(TDSSOCKET * tds, TDSLOGIN * login);

void
tds_set_version(TDSLOGIN * tds_login, TDS_TINYINT major_ver, TDS_TINYINT minor_ver)
{
	tds_login->tds_version = ((TDS_USMALLINT) major_ver << 8) + minor_ver;
}

void
tds_set_packet(TDSLOGIN * tds_login, int packet_size)
{
	tds_login->block_size = packet_size;
}

void
tds_set_port(TDSLOGIN * tds_login, int port)
{
	tds_login->port = port;
}

void
tds_set_passwd(TDSLOGIN * tds_login, const char *password)
{
	if (password) {
		tds_dstr_zero(&tds_login->password);
		tds_dstr_copy(&tds_login->password, password);
	}
}
void
tds_set_bulk(TDSLOGIN * tds_login, TDS_TINYINT enabled)
{
	tds_login->bulk_copy = enabled ? 0 : 1;
}

void
tds_set_user(TDSLOGIN * tds_login, const char *username)
{
	tds_dstr_copy(&tds_login->user_name, username);
}

void
tds_set_host(TDSLOGIN * tds_login, const char *hostname)
{
	tds_dstr_copy(&tds_login->client_host_name, hostname);
}

void
tds_set_app(TDSLOGIN * tds_login, const char *application)
{
	tds_dstr_copy(&tds_login->app_name, application);
}

/**
 * \brief Set the servername in a TDSLOGIN structure
 *
 * Normally copies \a server into \a tds_login.  If \a server does not point to a plausible name, the environment 
 * variables TDSQUERY and DSQUERY are used, in that order.  If they don't exist, the "default default" servername
 * is "SYBASE" (although the utility of that choice is a bit murky).  
 *
 * \param tds_login	points to a TDSLOGIN structure
 * \param server	the servername, or NULL, or a zero-length string
 * \todo open the log file earlier, so these messages can be seen.  
 */
void
tds_set_server(TDSLOGIN * tds_login, const char *server)
{
#if 0
	/* Doing this in tds_alloc_login instead */
	static const char *names[] = { "TDSQUERY", "DSQUERY", "SYBASE" };
	int i;
	
	for (i=0; i < TDS_VECTOR_SIZE(names) && (!server || strlen(server) == 0); i++) {
		const char *source;
		if (i + 1 == TDS_VECTOR_SIZE(names)) {
			server = names[i];
			source = "compiled-in default";
		} else {
			server = getenv(names[i]);
			source = names[i];
		}
		if (server) {
			tdsdump_log(TDS_DBG_INFO1, "Setting TDSLOGIN::server_name to '%s' from %s.\n", server, source);
		}
	}
#endif
	if (server)
		tds_dstr_copy(&tds_login->server_name, server);
}

void
tds_set_library(TDSLOGIN * tds_login, const char *library)
{
	tds_dstr_copy(&tds_login->library, library);
}

void
tds_set_client_charset(TDSLOGIN * tds_login, const char *charset)
{
	tds_dstr_copy(&tds_login->client_charset, charset);
}

void
tds_set_language(TDSLOGIN * tds_login, const char *language)
{
	tds_dstr_copy(&tds_login->language, language);
}

void
tds_set_database_name(TDSLOGIN * tds_login, const char *dbname)
{
	tds_dstr_copy(&tds_login->database, dbname);
}

struct tds_save_msg
{
	TDSMESSAGE msg;
	char type;
};

struct tds_save_env
{
	char *oldval;
	char *newval;
	int type;
};

typedef struct tds_save_context
{
	/* must be first !!! */
	TDSCONTEXT ctx;

	unsigned num_msg;
	struct tds_save_msg msgs[10];

	unsigned num_env;
	struct tds_save_env envs[10];
} TDSSAVECONTEXT;

static void
tds_save(TDSSAVECONTEXT *ctx, char type, TDSMESSAGE *msg)
{
	struct tds_save_msg *dest_msg;

	if (ctx->num_msg >= TDS_VECTOR_SIZE(ctx->msgs))
		return;

	dest_msg = &ctx->msgs[ctx->num_msg];
	dest_msg->type = type;
	dest_msg->msg = *msg;
#define COPY(name) if (msg->name) dest_msg->msg.name = strdup(msg->name);
	COPY(server);
	COPY(message);
	COPY(proc_name);
	COPY(sql_state);
#undef COPY
	++ctx->num_msg;
}

static int
tds_save_msg(const TDSCONTEXT *ctx, TDSSOCKET *tds, TDSMESSAGE *msg)
{
	tds_save((TDSSAVECONTEXT *) ctx, 0, msg);
	return 0;
}

static int
tds_save_err(const TDSCONTEXT *ctx, TDSSOCKET *tds, TDSMESSAGE *msg)
{
	tds_save((TDSSAVECONTEXT *) ctx, 1, msg);
	return TDS_INT_CANCEL;
}

static void
tds_save_env(TDSSOCKET * tds, int type, char *oldval, char *newval)
{
	TDSSAVECONTEXT *ctx;
	struct tds_save_env *env;

	if (tds_get_ctx(tds)->msg_handler != tds_save_msg)
		return;

	ctx = (TDSSAVECONTEXT *) tds_get_ctx(tds);
	if (ctx->num_env >= TDS_VECTOR_SIZE(ctx->envs))
		return;

	env = &ctx->envs[ctx->num_env];
	env->type = type;
	env->oldval = oldval ? strdup(oldval) : NULL;
	env->newval = newval ? strdup(newval) : NULL;
	++ctx->num_env;
}

static void
init_save_context(TDSSAVECONTEXT *ctx, const TDSCONTEXT *old_ctx)
{
	memset(ctx, 0, sizeof(*ctx));
	ctx->ctx.locale = old_ctx->locale;
	ctx->ctx.msg_handler = tds_save_msg;
	ctx->ctx.err_handler = tds_save_err;
}

static void
replay_save_context(TDSSOCKET *tds, TDSSAVECONTEXT *ctx)
{
	unsigned n;

	/* replay all recorded messages */
	for (n = 0; n < ctx->num_msg; ++n)
		if (ctx->msgs[n].type == 0) {
			if (tds_get_ctx(tds)->msg_handler)
				tds_get_ctx(tds)->msg_handler(tds_get_ctx(tds), tds, &ctx->msgs[n].msg);
		} else {
			if (tds_get_ctx(tds)->err_handler)
				tds_get_ctx(tds)->err_handler(tds_get_ctx(tds), tds, &ctx->msgs[n].msg);
		}

	/* replay all recorded envs */
	for (n = 0; n < ctx->num_env; ++n)
		if (tds->env_chg_func)
			tds->env_chg_func(tds, ctx->envs[n].type, ctx->envs[n].oldval, ctx->envs[n].newval);
}

static void
reset_save_context(TDSSAVECONTEXT *ctx)
{
	unsigned n;

	/* free all messages */
	for (n = 0; n < ctx->num_msg; ++n)
		tds_free_msg(&ctx->msgs[n].msg);
	ctx->num_msg = 0;

	/* free all envs */
	for (n = 0; n < ctx->num_env; ++n) {
		free(ctx->envs[n].oldval);
		free(ctx->envs[n].newval);
	}
	ctx->num_env = 0;
}

static void
free_save_context(TDSSAVECONTEXT *ctx)
{
	reset_save_context(ctx);
}

/**
 * Do a connection to socket
 * @param tds connection structure. This should be a non-connected connection.
 * @return TDS_FAIL or TDS_SUCCESS if a connection was made to the server's port.
 * @return TDSERROR enumerated type if no TCP/IP connection could be formed. 
 * @param login info for login
 * @remark Possible error conditions:
 *		- TDSESOCK: socket(2) failed: insufficient local resources
 * 		- TDSECONN: connect(2) failed: invalid hostname or port (ETIMEDOUT, ECONNREFUSED, ENETUNREACH)
 * 		- TDSEFCON: connect(2) succeeded, login packet not acknowledged.  
 *		- TDS_FAIL: connect(2) succeeded, login failed.  
 */
static int
tds_connect(TDSSOCKET * tds, TDSLOGIN * login, int *p_oserr)
{
	int erc = -TDSEFCON;
	int connect_timeout = 0;
	int db_selected = 0;
	struct tds_addrinfo *addrs;
	int orig_port;

	/*
	 * A major version of 0 means try to guess the TDS version. 
	 * We try them in an order that should work. 
	 */
	const static TDS_USMALLINT versions[] =
		{ 0x703
		, 0x702
		, 0x701
		, 0x700
		, 0x500
		, 0x402
		};

	if (!login->valid_configuration) {
		tdserror(tds_get_ctx(tds), tds, TDSECONF, 0);
		return TDS_FAIL;
	}

	if (TDS_MAJOR(login) == 0) {
		unsigned int i;
		TDSSAVECONTEXT save_ctx;
		const TDSCONTEXT *old_ctx = tds_get_ctx(tds);
		typedef void (*env_chg_func_t) (TDSSOCKET * tds, int type, char *oldval, char *newval);
		env_chg_func_t old_env_chg = tds->env_chg_func;
		/* the context of a socket is const; we have to modify it to suppress error messages during multiple tries. */
		TDSCONTEXT *mod_ctx = (TDSCONTEXT *) tds_get_ctx(tds);
		err_handler_t err_handler = tds_get_ctx(tds)->err_handler;

		init_save_context(&save_ctx, old_ctx);
		tds_set_ctx(tds, &save_ctx.ctx);
		tds->env_chg_func = tds_save_env;
		mod_ctx->err_handler = NULL;

		for (i = 0; i < TDS_VECTOR_SIZE(versions); ++i) {
			login->tds_version = versions[i];
			reset_save_context(&save_ctx);

			erc = tds_connect(tds, login, p_oserr);
			if (TDS_FAILED(erc)) {
				tds_close_socket(tds);
			}
			
			if (erc != -TDSEFCON)	/* TDSEFCON indicates wrong TDS version */
				break;
		}
		
		mod_ctx->err_handler = err_handler;
		tds->env_chg_func = old_env_chg;
		tds_set_ctx(tds, old_ctx);
		replay_save_context(tds, &save_ctx);
		free_save_context(&save_ctx);
		
		if (TDS_FAILED(erc))
			tdserror(tds_get_ctx(tds), tds, -erc, *p_oserr);

		return erc;
	}
	

	/*
	 * If a dump file has been specified, start logging
	 */
	if (!tds_dstr_isempty(&login->dump_file) && !tdsdump_isopen()) {
		if (login->debug_flags)
			tds_debug_flags = login->debug_flags;
		tdsdump_open(tds_dstr_cstr(&login->dump_file));
	}

	tds->login = login;

	tds->conn->tds_version = login->tds_version;
	tds_conn(tds)->emul_little_endian = login->emul_little_endian;
#ifdef WORDS_BIGENDIAN
	if (IS_TDS7_PLUS(tds->conn)) {
		/* TDS 7/8 only supports little endian */
		tds_conn(tds)->emul_little_endian = 1;
	}
#endif

	/* set up iconv if not already initialized*/
	if (tds->conn->char_convs[client2ucs2]->to.cd == (iconv_t) -1) {
		if (!tds_dstr_isempty(&login->client_charset)) {
			tds_iconv_open(tds->conn, tds_dstr_cstr(&login->client_charset));
		}
	}

	connect_timeout = login->connect_timeout;

	/* Jeff's hack - begin */
	tds->query_timeout = connect_timeout ? connect_timeout : login->query_timeout;
	/* end */

	/* verify that ip_addr is not empty */
	if (login->ip_addrs == NULL) {
		tdserror(tds_get_ctx(tds), tds, TDSEUHST, 0 );
		tdsdump_log(TDS_DBG_ERROR, "IP address pointer is empty\n");
		if (!tds_dstr_isempty(&login->server_name)) {
			tdsdump_log(TDS_DBG_ERROR, "Server %s not found!\n", tds_dstr_cstr(&login->server_name));
		} else {
			tdsdump_log(TDS_DBG_ERROR, "No server specified!\n");
		}
		return -TDSECONN;
	}

	tds_conn(tds)->capabilities = login->capabilities;

	erc = TDSEINTF;
	orig_port = login->port;
	for (addrs = login->ip_addrs; addrs != NULL; addrs = addrs->ai_next) {
		login->port = orig_port;

		if (!IS_TDS50(tds->conn) && !tds_dstr_isempty(&login->instance_name) && !login->port)
			login->port = tds7_get_instance_port(addrs, tds_dstr_cstr(&login->instance_name));

		if (login->port >= 1) {
			if ((erc = tds_open_socket(tds, addrs, login->port, connect_timeout, p_oserr)) == TDSEOK) {
				login->connected_addr = addrs;
				break;
			}
		} else {
			erc = TDSECONN;
		}
	}

	if (erc != TDSEOK) {
		if (login->port < 1)
			tdsdump_log(TDS_DBG_ERROR, "invalid port number\n");

		tdserror(tds_get_ctx(tds), tds, erc, *p_oserr);
		return -erc;
	}
		
	/*
	 * Beyond this point, we're connected to the server.  We know we have a valid TCP/IP address+socket pair.  
	 * Although network errors *might* happen, most problems from here on out will be TDS-level errors, 
	 * either TDS version problems or authentication problems.  
	 */
		
	tds_set_state(tds, TDS_IDLE);

	if (IS_TDS71_PLUS(tds->conn)) {
		erc = tds71_do_login(tds, login);
		db_selected = 1;
	} else if (IS_TDS7_PLUS(tds->conn)) {
		erc = tds7_send_login(tds, login);
		db_selected = 1;
	} else {
		tds->out_flag = TDS_LOGIN;
		erc = tds_send_login(tds, login);
	}
	if (TDS_FAILED(erc) || TDS_FAILED(tds_process_login_tokens(tds))) {
		tdsdump_log(TDS_DBG_ERROR, "login packet %s\n", TDS_SUCCEED(erc)? "accepted":"rejected");
		tds_close_socket(tds);
		tdserror(tds_get_ctx(tds), tds, TDSEFCON, 0); 	/* "Adaptive Server connection failed" */
		return -TDSEFCON;
	}

#if ENABLE_ODBC_MARS
	/* initialize SID */
	if (IS_TDS72_PLUS(tds->conn) && login->mars) {
		tds->conn->sessions[0] = NULL;
		tds->conn->mars = 1;
		tds->sid = -1;
		tds_init_write_buf(tds);
	}
#endif

	if (login->text_size || (!db_selected && !tds_dstr_isempty(&login->database))) {
		char *str;
		int len;

		len = 64 + tds_quote_id(tds, NULL, tds_dstr_cstr(&login->database),-1);
		if ((str = (char *) malloc(len)) == NULL)
			return TDS_FAIL;

		str[0] = 0;
		if (login->text_size) {
			sprintf(str, "set textsize %d ", login->text_size);
		}
		if (!db_selected && !tds_dstr_isempty(&login->database)) {
			strcat(str, "use ");
			tds_quote_id(tds, strchr(str, 0), tds_dstr_cstr(&login->database), -1);
		}
		erc = tds_submit_query(tds, str);
		free(str);
		if (TDS_FAILED(erc))
			return erc;

		erc = tds_process_simple_query(tds);
		if (TDS_FAILED(erc))
			return erc;
	}

	tds->query_timeout = login->query_timeout;
	tds->login = NULL;
	return TDS_SUCCESS;
}

int
tds_connect_and_login(TDSSOCKET * tds, TDSLOGIN * login)
{
	int oserr = 0;
	return tds_connect(tds, login, &oserr);
}

static int
tds_put_login_string(TDSSOCKET * tds, const char *buf, int n)
{
	const int buf_len = buf ? (int)strlen(buf) : 0;
	return tds_put_buf(tds, (const unsigned char *) buf, n, buf_len);
}

static TDSRET
tds_send_login(TDSSOCKET * tds, TDSLOGIN * login)
{
#ifdef WORDS_BIGENDIAN
	static const unsigned char be1[] = { 0x02, 0x00, 0x06, 0x04, 0x08, 0x01 };
#endif
	static const unsigned char le1[] = { 0x03, 0x01, 0x06, 0x0a, 0x09, 0x01 };
	static const unsigned char magic2[] = { 0x00, 0x00 };

	static const unsigned char magic3[] = { 0x00, 0x00, 0x00 };

	/* these seem to endian flags as well 13,17 on intel/alpha 12,16 on power */

#ifdef WORDS_BIGENDIAN
	static const unsigned char be2[] = { 0x00, 12, 16 };
#endif
	static const unsigned char le2[] = { 0x00, 13, 17 };

	/* 
	 * the former byte 0 of magic5 causes the language token and message to be 
	 * absent from the login acknowledgement if set to 1. There must be a way 
	 * of setting this in the client layer, but I am not aware of any thing of
	 * the sort -- bsb 01/17/99
	 */
	static const unsigned char magic5[] = { 0x00, 0x00 };
	static const unsigned char magic6[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };

	static const unsigned char magic42[] = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
	static const unsigned char magic50[] = { 0x00, 0x00, 0x00, 0x00 };

	/*
	 * capabilities are now part of the tds structure.
	 * unsigned char capabilities[]= {0x01,0x07,0x03,109,127,0xFF,0xFF,0xFF,0xFE,0x02,0x07,0x00,0x00,0x0A,104,0x00,0x00,0x00};
	 */
	/*
	 * This is the original capabilities packet we were working with (sqsh)
	 * unsigned char capabilities[]= {0x01,0x07,0x03,109,127,0xFF,0xFF,0xFF,0xFE,0x02,0x07,0x00,0x00,0x0A,104,0x00,0x00,0x00};
	 * original with 4.x messages
	 * unsigned char capabilities[]= {0x01,0x07,0x03,109,127,0xFF,0xFF,0xFF,0xFE,0x02,0x07,0x00,0x00,0x00,120,192,0x00,0x0D};
	 * This is isql 11.0.3
	 * unsigned char capabilities[]= {0x01,0x07,0x00,96, 129,207, 0xFF,0xFE,62,  0x02,0x07,0x00,0x00,0x00,120,192,0x00,0x0D};
	 * like isql but with 5.0 messages
	 * unsigned char capabilities[]= {0x01,0x07,0x00,96, 129,207, 0xFF,0xFE,62,  0x02,0x07,0x00,0x00,0x00,120,192,0x00,0x00};
	 */

	unsigned char protocol_version[4];
	unsigned char program_version[4];

	int len;
	char blockstr[16];

	/* override lservname field for ASA servers */	
	const char *lservname = getenv("ASA_DATABASE")? getenv("ASA_DATABASE") : tds_dstr_cstr(&login->server_name);

	if (strchr(tds_dstr_cstr(&login->user_name), '\\') != NULL) {
		tdsdump_log(TDS_DBG_ERROR, "NT login not support using TDS 4.x or 5.0\n");
		return TDS_FAIL;
	}

	if (IS_TDS42(tds->conn)) {
		memcpy(protocol_version, "\004\002\000\000", 4);
		memcpy(program_version, "\004\002\000\000", 4);
	} else if (IS_TDS46(tds->conn)) {
		memcpy(protocol_version, "\004\006\000\000", 4);
		memcpy(program_version, "\004\002\000\000", 4);
	} else if (IS_TDS50(tds->conn)) {
		memcpy(protocol_version, "\005\000\000\000", 4);
		memcpy(program_version, "\005\000\000\000", 4);
	} else {
		tdsdump_log(TDS_DBG_SEVERE, "Unknown protocol version!\n");
		return TDS_FAIL;
	}
	/*
	 * the following code is adapted from  Arno Pedusaar's 
	 * (psaar@fenar.ee) MS-SQL Client. His was a much better way to
	 * do this, (well...mine was a kludge actually) so here's mostly his
	 */

	tds_put_login_string(tds, tds_dstr_cstr(&login->client_host_name), TDS_MAXNAME);	/* client host name */
	tds_put_login_string(tds, tds_dstr_cstr(&login->user_name), TDS_MAXNAME);	/* account name */
	tds_put_login_string(tds, tds_dstr_cstr(&login->password), TDS_MAXNAME);	/* account password */
	sprintf(blockstr, "%d", (int) getpid());
	tds_put_login_string(tds, blockstr, TDS_MAXNAME);	/* host process */
#ifdef WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian) {
		tds_put_n(tds, le1, 6);
	} else {
		tds_put_n(tds, be1, 6);
	}
#else
	tds_put_n(tds, le1, 6);
#endif
	tds_put_byte(tds, login->bulk_copy);
	tds_put_n(tds, magic2, 2);
	if (IS_TDS42(tds->conn)) {
		tds_put_int(tds, 512);
	} else {
		tds_put_int(tds, 0);
	}
	tds_put_n(tds, magic3, 3);
	tds_put_login_string(tds, tds_dstr_cstr(&login->app_name), TDS_MAXNAME);
	tds_put_login_string(tds, lservname, TDS_MAXNAME);
	if (IS_TDS42(tds->conn)) {
		tds_put_login_string(tds, tds_dstr_cstr(&login->password), 255);
	} else {
		len = (int)tds_dstr_len(&login->password);
		if (len > 253)
			len = 0;
		tds_put_byte(tds, 0);
		tds_put_byte(tds, len);
		tds_put_n(tds, tds_dstr_cstr(&login->password), len);
		tds_put_n(tds, NULL, 253 - len);
		tds_put_byte(tds, len + 2);
	}

	tds_put_n(tds, protocol_version, 4);	/* TDS version; { 0x04,0x02,0x00,0x00 } */
	tds_put_login_string(tds, tds_dstr_cstr(&login->library), TDS_PROGNLEN);	/* client program name */
	if (IS_TDS42(tds->conn)) {
		tds_put_int(tds, 0);
	} else {
		tds_put_n(tds, program_version, 4);	/* program version ? */
	}
#ifdef WORDS_BIGENDIAN
	if (tds_conn(tds)->emul_little_endian) {
		tds_put_n(tds, le2, 3);
	} else {
		tds_put_n(tds, be2, 3);
	}
#else
	tds_put_n(tds, le2, 3);
#endif
	tds_put_login_string(tds, tds_dstr_cstr(&login->language), TDS_MAXNAME);	/* language */
	tds_put_byte(tds, login->suppress_language);
	tds_put_n(tds, magic5, 2);
	tds_put_byte(tds, login->encryption_level ? 1 : 0);
	tds_put_n(tds, magic6, 10);

	/* use empty charset to handle conversions on client */
	tds_put_login_string(tds, "", TDS_MAXNAME);	/* charset */
	/* this is a flag, mean that server should use character set provided by client */
	/* TODO notify charset change ?? what's correct meaning ?? -- freddy77 */
	tds_put_byte(tds, 1);

	/* network packet size */
	if (login->block_size < 65536 && login->block_size > 0)
		sprintf(blockstr, "%d", login->block_size);
	else
		strcpy(blockstr, "512");
	tds_put_login_string(tds, blockstr, TDS_PKTLEN);

	if (IS_TDS42(tds->conn)) {
		tds_put_n(tds, magic42, 8);
	} else if (IS_TDS46(tds->conn)) {
		tds_put_n(tds, magic42, 4);
	} else if (IS_TDS50(tds->conn)) {
		tds_put_n(tds, magic50, 4);
		tds_put_byte(tds, TDS_CAPABILITY_TOKEN);
		tds_put_smallint(tds, sizeof(tds_conn(tds)->capabilities));
		tds_put_n(tds, &tds_conn(tds)->capabilities, sizeof(tds_conn(tds)->capabilities));
	}

	return tds_flush_packet(tds);
}

/**
 * tds7_send_login() -- Send a TDS 7.0 login packet
 * TDS 7.0 login packet is vastly different and so gets its own function
 * \returns the return value is ignored by the caller. :-/
 */
static TDSRET
tds7_send_login(TDSSOCKET * tds, TDSLOGIN * login)
{
	static const unsigned char 
		client_progver[] = {   6, 0x83, 0xf2, 0xf8 }, 

		tds70Version[] = { 0x00, 0x00, 0x00, 0x70 },
		tds71Version[] = { 0x01, 0x00, 0x00, 0x71 },
		tds72Version[] = { 0x02, 0x00, 0x09, 0x72 },
		tds73Version[] = { 0x03, 0x00, 0x0B, 0x73 }, 
		connection_id[] = { 0x00, 0x00, 0x00, 0x00 }, 
		time_zone[] = { 0x88, 0xff, 0xff, 0xff }, 
		collation[] = { 0x36, 0x04, 0x00, 0x00 }, 
		
		sql_type_flag = 0x00, 
		reserved_flag = 0x00;

	const unsigned char *ptds7version = tds70Version;
	
	TDS_INT block_size = 4096;
	
	unsigned char option_flag1 = TDS_SET_LANG_ON | TDS_USE_DB_NOTIFY | TDS_INIT_DB_FATAL;
	unsigned char option_flag2 = login->option_flag2;
	unsigned char option_flag3 = TDS_UNKNOWN_COLLATION_HANDLING;

	unsigned char hwaddr[6];
	size_t unicode_left, packet_size, current_pos;
	TDSRET rc;

	const char *user_name = tds_dstr_cstr(&login->user_name);

	/* FIXME: These are defined as size_t, but should be TDS_SMALLINT. */
	size_t user_name_len = strlen(user_name);
	size_t host_name_len = tds_dstr_len(&login->client_host_name);
	size_t app_name_len = tds_dstr_len(&login->app_name);
	size_t password_len = tds_dstr_len(&login->password);
	size_t server_name_len = tds_dstr_len(&login->server_name);
	size_t library_len = tds_dstr_len(&login->library);
	size_t language_len = tds_dstr_len(&login->language);
	size_t database_len = tds_dstr_len(&login->database);
	size_t auth_len = 0;

	tds->out_flag = TDS7_LOGIN;

	/* discard possible previous authentication */
	if (tds_conn(tds)->authentication) {
		tds_conn(tds)->authentication->free(tds_conn(tds), tds_conn(tds)->authentication);
		tds_conn(tds)->authentication = NULL;
	}

	/* avoid overflow limiting password */
	if (password_len > 128)
		password_len = 128;

	current_pos = IS_TDS72_PLUS(tds->conn) ? 86 + 8 : 86;	/* ? */

	packet_size = current_pos + (host_name_len + app_name_len + server_name_len + library_len + language_len + database_len) * 2;

	/* check ntlm */
#ifdef HAVE_SSPI
	if (strchr(user_name, '\\') != NULL || user_name_len == 0) {
		tdsdump_log(TDS_DBG_INFO2, "using SSPI authentication for '%s' account\n", user_name);
		tds_conn(tds)->authentication = tds_sspi_get_auth(tds);
		if (!tds_conn(tds)->authentication)
			return TDS_FAIL;
		auth_len = tds_conn(tds)->authentication->packet_len;
		packet_size += auth_len;
#else
	if (strchr(user_name, '\\') != NULL) {
		tdsdump_log(TDS_DBG_INFO2, "using NTLM authentication for '%s' account\n", user_name);
		tds_conn(tds)->authentication = tds_ntlm_get_auth(tds);
		if (!tds_conn(tds)->authentication)
			return TDS_FAIL;
		auth_len = tds_conn(tds)->authentication->packet_len;
		packet_size += auth_len;
	} else if (user_name_len == 0) {
# ifdef ENABLE_KRB5
		/* try kerberos */
		tdsdump_log(TDS_DBG_INFO2, "using GSS authentication\n");
		tds_conn(tds)->authentication = tds_gss_get_auth(tds);
		if (!tds_conn(tds)->authentication)
			return TDS_FAIL;
		auth_len = tds_conn(tds)->authentication->packet_len;
		packet_size += auth_len;
# else
		tdsdump_log(TDS_DBG_ERROR, "requested GSS authentication but not compiled in\n");
		return TDS_FAIL;
# endif
#endif
	} else
		packet_size += (user_name_len + password_len) * 2;

#if !defined(TDS_DEBUG_LOGIN)
	tdsdump_log(TDS_DBG_INFO2, "quietly sending TDS 7+ login packet\n");
	tdsdump_off();
#endif
	TDS_PUT_INT(tds, packet_size);
	switch (login->tds_version) {
	case 0x700:
		ptds7version = tds70Version;
		break;
	case 0x701:
		ptds7version = tds71Version;
		break;
	case 0x702:
		ptds7version = tds72Version;
		break;
	case 0x703:
		ptds7version = tds73Version;
		break;
	default:
		assert(0 && 0x700 <= login->tds_version && login->tds_version <= 0x703);
	}
	
	tds_put_n(tds, ptds7version, sizeof(tds70Version));

	if (512 <= login->block_size && login->block_size < 1000000)
		block_size = login->block_size;

	tds_put_int(tds, block_size);	/* desired packet size being requested by client */

	if (block_size > tds->out_buf_max)
		tds_realloc_socket(tds, block_size);

	tds_put_n(tds, client_progver, sizeof(client_progver));	/* client program version ? */

	tds_put_int(tds, getpid());	/* process id of this process */

	tds_put_n(tds, connection_id, sizeof(connection_id));

	if (!login->bulk_copy)
		option_flag1 |= TDS_DUMPLOAD_OFF;
		
	tds_put_byte(tds, option_flag1);

	if (tds_conn(tds)->authentication)
		option_flag2 |= TDS_INTEGRATED_SECURITY_ON;

	tds_put_byte(tds, option_flag2);

	tds_put_byte(tds, sql_type_flag);
	tds_put_byte(tds, IS_TDS73_PLUS(tds->conn) ? option_flag3 : reserved_flag);

	tds_put_n(tds, time_zone, sizeof(time_zone));
	tds_put_n(tds, collation, sizeof(collation));

	/* host name */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, host_name_len);
	current_pos += host_name_len * 2;
	if (tds_conn(tds)->authentication) {
		tds_put_smallint(tds, 0);
		tds_put_smallint(tds, 0);
		tds_put_smallint(tds, 0);
		tds_put_smallint(tds, 0);
	} else {
		/* username */
		TDS_PUT_SMALLINT(tds, current_pos);
		TDS_PUT_SMALLINT(tds, user_name_len);
		current_pos += user_name_len * 2;
		/* password */
		TDS_PUT_SMALLINT(tds, current_pos);
		TDS_PUT_SMALLINT(tds, password_len);
		current_pos += password_len * 2;
	}
	/* app name */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, app_name_len);
	current_pos += app_name_len * 2;
	/* server name */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, server_name_len);
	current_pos += server_name_len * 2;
	/* unknown */
	tds_put_smallint(tds, 0);
	tds_put_smallint(tds, 0);
	/* library name */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, library_len);
	current_pos += library_len * 2;
	/* language  - kostya@warmcat.excom.spb.su */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, language_len);
	current_pos += language_len * 2;
	/* database name */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, database_len);
	current_pos += database_len * 2;

	/* MAC address */
	tds_getmac(tds_get_s(tds), hwaddr);
	tds_put_n(tds, hwaddr, 6);

	/* authentication stuff */
	TDS_PUT_SMALLINT(tds, current_pos);
	TDS_PUT_SMALLINT(tds, auth_len);	/* this matches numbers at end of packet */
	current_pos += auth_len;

	/* db file */
	TDS_PUT_SMALLINT(tds, current_pos);
	tds_put_smallint(tds, 0);

	if (IS_TDS72_PLUS(tds->conn)) {
		/* new password */
		TDS_PUT_SMALLINT(tds, current_pos);
		tds_put_smallint(tds, 0);

		/* SSPI long */
		tds_put_int(tds, 0);
	}

	/* FIXME here we assume single byte, do not use *2 to compute bytes, convert before !!! */
	tds_put_string(tds, tds_dstr_cstr(&login->client_host_name), (int)host_name_len);
	if (!tds_conn(tds)->authentication) {
		char unicode_string[256], *punicode = unicode_string;
		const char *p;
		TDSICONV *char_conv = tds->conn->char_convs[client2ucs2];

		tds_put_string(tds, tds_dstr_cstr(&login->user_name), (int)user_name_len);
		p = tds_dstr_cstr(&login->password);
		unicode_left = sizeof(unicode_string);

		memset(&char_conv->suppress, 0, sizeof(char_conv->suppress));
		if (tds_iconv(tds, tds->conn->char_convs[client2ucs2], to_server, &p, &password_len, &punicode, &unicode_left) ==
		    (size_t) - 1) {
			tdsdump_log(TDS_DBG_INFO1, "password \"%s\" could not be converted to UCS-2\n", p);
			assert(0);
		}
		password_len = punicode - unicode_string;
		tds7_crypt_pass((unsigned char *) unicode_string, password_len, (unsigned char *) unicode_string);
		tds_put_n(tds, unicode_string, password_len);
	}
	tds_put_string(tds, tds_dstr_cstr(&login->app_name), (int)app_name_len);
	tds_put_string(tds, tds_dstr_cstr(&login->server_name), (int)server_name_len);
	tds_put_string(tds, tds_dstr_cstr(&login->library), (int)library_len);
	tds_put_string(tds, tds_dstr_cstr(&login->language), (int)language_len);
	tds_put_string(tds, tds_dstr_cstr(&login->database), (int)database_len);

	if (tds_conn(tds)->authentication)
		tds_put_n(tds, tds_conn(tds)->authentication->packet, auth_len);

	rc = tds_flush_packet(tds);
	tdsdump_on();

	return rc;
}

/**
 * tds7_crypt_pass() -- 'encrypt' TDS 7.0 style passwords.
 * the calling function is responsible for ensuring crypt_pass is at least 
 * 'len' characters
 */
unsigned char *
tds7_crypt_pass(const unsigned char *clear_pass, size_t len, unsigned char *crypt_pass)
{
	size_t i;

	for (i = 0; i < len; i++)
		crypt_pass[i] = ((clear_pass[i] << 4) | (clear_pass[i] >> 4)) ^ 0xA5;
	return crypt_pass;
}

static TDSRET
tds71_do_login(TDSSOCKET * tds, TDSLOGIN* login)
{
	int i, len;
	const char *instance_name = tds_dstr_isempty(&login->instance_name) ? "MSSQLServer" : tds_dstr_cstr(&login->instance_name);
	int instance_name_len = strlen(instance_name) + 1;
	TDS_CHAR crypt_flag;
	unsigned int start_pos = 21;
	TDSRET ret;

#define START_POS 21
#define UI16BE(n) ((n) >> 8), ((n) & 0xffu)
#define SET_UI16BE(i,n) do { buf[i] = ((n) >> 8); buf[i+1] = ((n) & 0xffu); } while(0)
	TDS_UCHAR buf[] = {
		/* netlib version */
		0, UI16BE(START_POS), UI16BE(6),
		/* encryption */
		1, UI16BE(START_POS + 6), UI16BE(1),
		/* instance */
		2, UI16BE(START_POS + 6 + 1), UI16BE(0),
		/* process id */
		3, UI16BE(0), UI16BE(4),
		/* MARS enables */
		4, UI16BE(0), UI16BE(1),
		/* end */
		0xff
	};
	static const TDS_UCHAR netlib8[] = { 8, 0, 1, 0x55, 0, 0 };
	static const TDS_UCHAR netlib9[] = { 9, 0, 0,    0, 0, 0 };

	TDS_UCHAR *p;

	SET_UI16BE(13, instance_name_len);
	if (!IS_TDS72_PLUS(tds->conn)) {
		SET_UI16BE(16, START_POS + 6 + 1 + instance_name_len);
		buf[20] = 0xff;
	} else {
		start_pos += 5;
#undef  START_POS
#define START_POS 26
		SET_UI16BE(1, START_POS);
		SET_UI16BE(6, START_POS + 6);
		SET_UI16BE(11, START_POS + 6 + 1);
		SET_UI16BE(16, START_POS + 6 + 1 + instance_name_len);
		SET_UI16BE(21, START_POS + 6 + 1 + instance_name_len + 4);
	}

	assert(start_pos >= 21 && start_pos <= sizeof(buf));
	assert(buf[start_pos-1] == 0xff);

	/*
	 * fix a problem with mssql2k which doesn't like
	 * packet splitted during SSL handshake
	 */
	if (tds->out_buf_max < 4096)
		tds_realloc_socket(tds, 4096);

	/* do prelogin */
	tds->out_flag = TDS71_PRELOGIN;

	tds_put_n(tds, buf, start_pos);
	/* netlib version */
	tds_put_n(tds, IS_TDS72_PLUS(tds->conn) ? netlib9 : netlib8, 6);
	/* encryption */
#if !defined(HAVE_GNUTLS) && !defined(HAVE_OPENSSL)
	/* not supported */
	tds_put_byte(tds, 2);
#else
	tds_put_byte(tds, login->encryption_level >= TDS_ENCRYPTION_REQUIRE ? 1 : 0);
#endif
	/* instance */
	tds_put_n(tds, instance_name, instance_name_len);
	/* pid */
	tds_put_int(tds, getpid());
	/* MARS (1 enabled) */
	if (IS_TDS72_PLUS(tds->conn))
#if ENABLE_ODBC_MARS
		tds_put_byte(tds, login->mars);
#else
		tds_put_byte(tds, 0);
#endif
	ret = tds_flush_packet(tds);
	if (TDS_FAILED(ret))
		return ret;

	/* now process reply from server */
	len = tds_read_packet(tds);
	if (len <= 0 || tds->in_flag != 4)
		return TDS_FAIL;
	len = tds->in_len - tds->in_pos;

	/* the only thing we care is flag */
	p = tds->in_buf + tds->in_pos;
	/* default 2, no certificate, no encryption */
	crypt_flag = 2;
	for (i = 0;; i += 5) {
		TDS_UCHAR type;
		int off, l;

		if (i >= len)
			return TDS_FAIL;
		type = p[i];
		if (type == 0xff)
			break;
		/* check packet */
		if (i+4 >= len)
			return TDS_FAIL;
		off = (((int) p[i+1]) << 8) | p[i+2];
		l = (((int) p[i+3]) << 8) | p[i+4];
		if (off > len || (off+l) > len)
			return TDS_FAIL;
		if (type == 1 && l >= 1) {
			crypt_flag = p[off];
		}
	}
	/* we readed all packet */
	tds->in_pos += len;
	/* TODO some mssql version do not set last packet, update tds according */

	tdsdump_log(TDS_DBG_INFO1, "detected flag %d\n", crypt_flag);

	/* if server do not has certificate do normal login */
	if (crypt_flag == 2) {
		/* unless we wanted encryption and got none, then fail */
		if (login->encryption_level >= TDS_ENCRYPTION_REQUIRE)
			return TDS_FAIL;

		return tds7_send_login(tds, login);
	}
#if !defined(HAVE_GNUTLS) && !defined(HAVE_OPENSSL)
	tdsdump_log(TDS_DBG_ERROR, "server required encryption but support is not compiled in\n");
	return TDS_FAIL;
#else
	/*
	 * if server has a certificate it require at least a crypted login
	 * (even if data is not encrypted)
	 */

	/* here we have to do encryption ... */

	ret = tds_ssl_init(tds);
	if (TDS_FAILED(ret))
		return ret;

	ret = tds7_send_login(tds, login);

	/* if flag is 0 it means that after login server continue not encrypted */
	if (crypt_flag == 0 || TDS_FAILED(ret))
		tds_ssl_deinit(tds_conn(tds));

	return ret;
#endif
}

