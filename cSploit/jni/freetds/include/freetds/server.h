/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998-1999  Brian Bruns
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

#ifndef _tdsserver_h_
#define _tdsserver_h_
#endif

#ifdef __cplusplus
extern "C"
{
#endif
#if 0
}
#endif

/* login.c */
unsigned char *tds7_decrypt_pass(const unsigned char *crypt_pass, int len, unsigned char *clear_pass);
TDSSOCKET *tds_listen(TDSCONTEXT * ctx, int ip_port);
void tds_read_login(TDSSOCKET * tds, TDSLOGIN * login);
int tds7_read_login(TDSSOCKET * tds, TDSLOGIN * login);
TDSLOGIN *tds_alloc_read_login(TDSSOCKET * tds);

/* query.c */
char *tds_get_query(TDSSOCKET * tds);
char *tds_get_generic_query(TDSSOCKET * tds);

/* server.c */
void tds_env_change(TDSSOCKET * tds, int type, const char *oldvalue, const char *newvalue);
void tds_send_msg(TDSSOCKET * tds, int msgno, int msgstate, int severity, const char *msgtext, const char *srvname,
		  const char *procname, int line);
void tds_send_login_ack(TDSSOCKET * tds, const char *progname);
void tds_send_eed(TDSSOCKET * tds, int msgno, int msgstate, int severity, char *msgtext, char *srvname, char *procname, int line);
void tds_send_err(TDSSOCKET * tds, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);
void tds_send_capabilities_token(TDSSOCKET * tds);
/* TODO remove, use tds_send_done */
void tds_send_done_token(TDSSOCKET * tds, TDS_SMALLINT flags, TDS_INT numrows);
void tds_send_done(TDSSOCKET * tds, int token, TDS_SMALLINT flags, TDS_INT numrows);
void tds_send_control_token(TDSSOCKET * tds, TDS_SMALLINT numcols);
void tds_send_col_name(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds_send_col_info(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds_send_result(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds7_send_result(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds_send_table_header(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds_send_row(TDSSOCKET * tds, TDSRESULTINFO * resinfo);
void tds71_send_prelogin(TDSSOCKET * tds);

#if 0
{
#endif
#ifdef __cplusplus
}
#endif
