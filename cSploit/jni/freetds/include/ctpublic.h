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

#ifndef _ctpublic_h_
#define _ctpublic_h_

#include <cspublic.h>

#undef TDS_STATIC_CAST
#ifdef __cplusplus
#define TDS_STATIC_CAST(type, a) static_cast<type>(a)
extern "C"
{
#if 0
}
#endif
#else
#define TDS_STATIC_CAST(type, a) ((type)(a))
#endif

/*
** define for each CT-Lib API
*/
#define CT_BIND         TDS_STATIC_CAST(CS_INT, 0)
#define CT_BR_COLUMN    TDS_STATIC_CAST(CS_INT, 1)
#define CT_BR_TABLE     TDS_STATIC_CAST(CS_INT, 2)
#define CT_CALLBACK     TDS_STATIC_CAST(CS_INT, 3)
#define CT_CANCEL       TDS_STATIC_CAST(CS_INT, 4)
#define CT_CAPABILITY   TDS_STATIC_CAST(CS_INT, 5)
#define CT_CLOSE        TDS_STATIC_CAST(CS_INT, 6)
#define CT_CMD_ALLOC    TDS_STATIC_CAST(CS_INT, 7)
#define CT_CMD_DROP     TDS_STATIC_CAST(CS_INT, 8)
#define CT_CMD_PROPS    TDS_STATIC_CAST(CS_INT, 9)
#define CT_COMMAND      TDS_STATIC_CAST(CS_INT, 10)
#define CT_COMPUTE_INFO TDS_STATIC_CAST(CS_INT, 11)
#define CT_CON_ALLOC    TDS_STATIC_CAST(CS_INT, 12)
#define CT_CON_DROP     TDS_STATIC_CAST(CS_INT, 13)
#define CT_CON_PROPS    TDS_STATIC_CAST(CS_INT, 14)
#define CT_CON_XFER     TDS_STATIC_CAST(CS_INT, 15)
#define CT_CONFIG       TDS_STATIC_CAST(CS_INT, 16)
#define CT_CONNECT      TDS_STATIC_CAST(CS_INT, 17)
#define CT_CURSOR       TDS_STATIC_CAST(CS_INT, 18)
#define CT_DATA_INFO    TDS_STATIC_CAST(CS_INT, 19)
#define CT_DEBUG        TDS_STATIC_CAST(CS_INT, 20)
#define CT_DESCRIBE     TDS_STATIC_CAST(CS_INT, 21)
#define CT_DIAG         TDS_STATIC_CAST(CS_INT, 22)
#define CT_DYNAMIC      TDS_STATIC_CAST(CS_INT, 23)
#define CT_DYNDESC      TDS_STATIC_CAST(CS_INT, 24)
#define CT_EXIT         TDS_STATIC_CAST(CS_INT, 25)
#define CT_FETCH        TDS_STATIC_CAST(CS_INT, 26)
#define CT_GET_DATA     TDS_STATIC_CAST(CS_INT, 27)
#define CT_GETFORMAT    TDS_STATIC_CAST(CS_INT, 28)
#define CT_GETLOGINFO   TDS_STATIC_CAST(CS_INT, 29)
#define CT_INIT         TDS_STATIC_CAST(CS_INT, 30)
#define CT_KEYDATA      TDS_STATIC_CAST(CS_INT, 31)
#define CT_OPTIONS      TDS_STATIC_CAST(CS_INT, 32)
#define CT_PARAM        TDS_STATIC_CAST(CS_INT, 33)
#define CT_POLL         TDS_STATIC_CAST(CS_INT, 34)
#define CT_RECVPASSTHRU TDS_STATIC_CAST(CS_INT, 35)
#define CT_REMOTE_PWD   TDS_STATIC_CAST(CS_INT, 36)
#define CT_RES_INFO     TDS_STATIC_CAST(CS_INT, 37)
#define CT_RESULTS      TDS_STATIC_CAST(CS_INT, 38)
#define CT_SEND         TDS_STATIC_CAST(CS_INT, 39)
#define CT_SEND_DATA    TDS_STATIC_CAST(CS_INT, 40)
#define CT_SENDPASSTHRU TDS_STATIC_CAST(CS_INT, 41)
#define CT_SETLOGINFO   TDS_STATIC_CAST(CS_INT, 42)
#define CT_WAKEUP       TDS_STATIC_CAST(CS_INT, 43)
#define CT_LABELS       TDS_STATIC_CAST(CS_INT, 44)
#define CT_DS_LOOKUP    TDS_STATIC_CAST(CS_INT, 45)
#define CT_DS_DROP      TDS_STATIC_CAST(CS_INT, 46)
#define CT_DS_OBJINFO   TDS_STATIC_CAST(CS_INT, 47)
#define CT_SETPARAM     TDS_STATIC_CAST(CS_INT, 48)
#define CT_DYNSQLDA     TDS_STATIC_CAST(CS_INT, 49)
#define CT_NOTIFICATION TDS_STATIC_CAST(CS_INT, 1000)

static const char rcsid_ctpublic_h[] = "$Id: ctpublic.h,v 1.14 2005-05-28 10:48:26 freddy77 Exp $";
static const void *const no_unused_ctpublic_h_warn[] = { rcsid_ctpublic_h, no_unused_ctpublic_h_warn };


CS_RETCODE ct_init(CS_CONTEXT * ctx, CS_INT version);
CS_RETCODE ct_con_alloc(CS_CONTEXT * ctx, CS_CONNECTION ** con);
CS_RETCODE ct_con_props(CS_CONNECTION * con, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * out_len);
CS_RETCODE ct_connect(CS_CONNECTION * con, CS_CHAR * servername, CS_INT snamelen);
CS_RETCODE ct_cmd_alloc(CS_CONNECTION * con, CS_COMMAND ** cmd);
CS_RETCODE ct_cancel(CS_CONNECTION * conn, CS_COMMAND * cmd, CS_INT type);
CS_RETCODE ct_cmd_drop(CS_COMMAND * cmd);
CS_RETCODE ct_close(CS_CONNECTION * con, CS_INT option);
CS_RETCODE ct_con_drop(CS_CONNECTION * con);
CS_RETCODE ct_exit(CS_CONTEXT * ctx, CS_INT unused);
CS_RETCODE ct_command(CS_COMMAND * cmd, CS_INT type, const CS_VOID * buffer, CS_INT buflen, CS_INT option);
CS_RETCODE ct_send(CS_COMMAND * cmd);
CS_RETCODE ct_results(CS_COMMAND * cmd, CS_INT * result_type);
CS_RETCODE ct_bind(CS_COMMAND * cmd, CS_INT item, CS_DATAFMT * datafmt, CS_VOID * buffer, CS_INT * copied, CS_SMALLINT * indicator);
CS_RETCODE ct_fetch(CS_COMMAND * cmd, CS_INT type, CS_INT offset, CS_INT option, CS_INT * rows_read);
CS_RETCODE ct_res_info_dyn(CS_COMMAND * cmd, CS_INT type, CS_VOID * buffer, CS_INT buflen, CS_INT * out_len);
CS_RETCODE ct_res_info(CS_COMMAND * cmd, CS_INT type, CS_VOID * buffer, CS_INT buflen, CS_INT * out_len);
CS_RETCODE ct_describe(CS_COMMAND * cmd, CS_INT item, CS_DATAFMT * datafmt);
CS_RETCODE ct_callback(CS_CONTEXT * ctx, CS_CONNECTION * con, CS_INT action, CS_INT type, CS_VOID * func);
CS_RETCODE ct_send_dyn(CS_COMMAND * cmd);
CS_RETCODE ct_results_dyn(CS_COMMAND * cmd, CS_INT * result_type);
CS_RETCODE ct_config(CS_CONTEXT * ctx, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE ct_cmd_props(CS_COMMAND * cmd, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE ct_compute_info(CS_COMMAND * cmd, CS_INT type, CS_INT colnum, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE ct_get_data(CS_COMMAND * cmd, CS_INT item, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE ct_send_data(CS_COMMAND * cmd, CS_VOID * buffer, CS_INT buflen);
CS_RETCODE ct_data_info(CS_COMMAND * cmd, CS_INT action, CS_INT colnum, CS_IODESC * iodesc);
CS_RETCODE ct_capability(CS_CONNECTION * con, CS_INT action, CS_INT type, CS_INT capability, CS_VOID * value);
CS_RETCODE ct_dynamic(CS_COMMAND * cmd, CS_INT type, CS_CHAR * id, CS_INT idlen, CS_CHAR * buffer, CS_INT buflen);
CS_RETCODE ct_param(CS_COMMAND * cmd, CS_DATAFMT * datafmt, CS_VOID * data, CS_INT datalen, CS_SMALLINT indicator);
CS_RETCODE ct_setparam(CS_COMMAND * cmd, CS_DATAFMT * datafmt, CS_VOID * data, CS_INT * datalen, CS_SMALLINT * indicator);
CS_RETCODE ct_options(CS_CONNECTION * con, CS_INT action, CS_INT option, CS_VOID * param, CS_INT paramlen, CS_INT * outlen);
CS_RETCODE ct_poll(CS_CONTEXT * ctx, CS_CONNECTION * connection, CS_INT milliseconds, CS_CONNECTION ** compconn,
		   CS_COMMAND ** compcmd, CS_INT * compid, CS_INT * compstatus);
CS_RETCODE ct_cursor(CS_COMMAND * cmd, CS_INT type, CS_CHAR * name, CS_INT namelen, CS_CHAR * text, CS_INT tlen, CS_INT option);
CS_RETCODE ct_diag(CS_CONNECTION * conn, CS_INT operation, CS_INT type, CS_INT idx, CS_VOID * buffer);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
