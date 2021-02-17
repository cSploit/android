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

#ifndef _cspublic_h_
#define _cspublic_h_

#include <cstypes.h>

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

static const char rcsid_cspublic_h[] = "$Id: cspublic.h,v 1.61 2008-09-08 17:50:25 jklowden Exp $";
static const void *const no_unused_cspublic_h_warn[] = { rcsid_cspublic_h, no_unused_cspublic_h_warn };

#define CS_PUBLIC
#define CS_STATIC static

#define CS_SUCCEED		1
#define CS_FAIL			0
#define CS_MEM_ERROR		-1
#define CS_PENDING 		-2
#define CS_QUIET 		-3
#define CS_BUSY			-4
#define CS_INTERRUPT 		-5
#define CS_BLK_HAS_TEXT		-6
#define CS_CONTINUE		-7
#define CS_FATAL		-8
#define CS_RET_HAFAILOVER	-9
#define CS_UNSUPPORTED		-10

#define CS_CANCELED	-202
#define CS_ROW_FAIL	-203
#define CS_END_DATA	-204
#define CS_END_RESULTS	-205
#define CS_END_ITEM	-206
#define CS_NOMSG	-207
#define CS_TIMED_OUT 	-208

#define CS_SIZEOF(x) sizeof(x)

#define CS_LAYER(x)    (((x) >> 24) & 0xFF)
#define CS_ORIGIN(x)   (((x) >> 16) & 0xFF)
#define CS_SEVERITY(x) (((x) >>  8) & 0xFF)
#define CS_NUMBER(x)   ((x) & 0xFF)

/* forward declarations */
typedef CS_RETCODE(*CS_CSLIBMSG_FUNC) (CS_CONTEXT *, CS_CLIENTMSG *);
typedef CS_RETCODE(*CS_CLIENTMSG_FUNC) (CS_CONTEXT *, CS_CONNECTION *, CS_CLIENTMSG *);
typedef CS_RETCODE(*CS_SERVERMSG_FUNC) (CS_CONTEXT *, CS_CONNECTION *, CS_SERVERMSG *);


#define CS_IODATA          TDS_STATIC_CAST(CS_INT, 1600)
#define CS_SRC_VALUE   -2562



/* status bits for CS_SERVERMSG */
#define CS_HASEED 0x01

typedef struct _cs_blkdesc CS_BLKDESC;

/* CS_CAP_REQUEST values */
#define CS_REQ_LANG	1
#define CS_REQ_RPC	2
#define CS_REQ_NOTIF	3
#define CS_REQ_MSTMT	4
#define CS_REQ_BCP	5
#define CS_REQ_CURSOR	6
#define CS_REQ_DYN	7
#define CS_REQ_MSG	8
#define CS_REQ_PARAM	9
#define CS_DATA_INT1	10
#define CS_DATA_INT2	11
#define CS_DATA_INT4	12
#define CS_DATA_BIT	13
#define CS_DATA_CHAR	14
#define CS_DATA_VCHAR	15
#define CS_DATA_BIN	16
#define CS_DATA_VBIN	17
#define CS_DATA_MNY8	18
#define CS_DATA_MNY4	19
#define CS_DATA_DATE8	20
#define CS_DATA_DATE4	21
#define CS_DATA_FLT4	22
#define CS_DATA_FLT8	23
#define CS_DATA_NUM	24
#define CS_DATA_TEXT	25
#define CS_DATA_IMAGE	26
#define CS_DATA_DEC	27
#define CS_DATA_LCHAR	28
#define CS_DATA_LBIN	29
#define CS_DATA_INTN	30
#define CS_DATA_DATETIMEN	31
#define CS_DATA_MONEYN	32
#define CS_CSR_PREV	33
#define CS_CSR_FIRST	34
#define CS_CSR_LAST	35
#define CS_CSR_ABS	36
#define CS_CSR_REL	37
#define CS_CSR_MULTI	38
#define CS_CON_OOB	39
#define CS_CON_INBAND	40
#define CS_CON_LOGICAL	41
#define CS_PROTO_TEXT	42
#define CS_PROTO_BULK	43
#define CS_REQ_URGNOTIF	44
#define CS_DATA_SENSITIVITY	45
#define CS_DATA_BOUNDARY	46
#define CS_PROTO_DYNAMIC	47
#define CS_PROTO_DYNPROC	48
#define CS_DATA_FLTN	49
#define CS_DATA_BITN	50
#define CS_OPTION_GET	51
#define CS_DATA_INT8	52
#define CS_DATA_VOID	53

/* CS_CAP_RESPONSE values */
#define CS_RES_NOMSG	1
#define CS_RES_NOEED	2
#define CS_RES_NOPARAM	3
#define CS_DATA_NOINT1	4
#define CS_DATA_NOINT2	5
#define CS_DATA_NOINT4	6
#define CS_DATA_NOBIT	7
#define CS_DATA_NOCHAR	8
#define CS_DATA_NOVCHAR	9
#define CS_DATA_NOBIN	10
#define CS_DATA_NOVBIN	11
#define CS_DATA_NOMNY8	12
#define CS_DATA_NOMNY4	13
#define CS_DATA_NODATE8	14
#define CS_DATA_NODATE4	15
#define CS_DATA_NOFLT4	16
#define CS_DATA_NOFLT8	17
#define CS_DATA_NONUM	18
#define CS_DATA_NOTEXT	19
#define CS_DATA_NOIMAGE	20
#define CS_DATA_NODEC	21
#define CS_DATA_NOLCHAR	22
#define CS_DATA_NOLBIN	23
#define CS_DATA_NOINTN	24
#define CS_DATA_NODATETIMEN	25
#define CS_DATA_NOMONEYN	26
#define CS_CON_NOOOB	27
#define CS_CON_NOINBAND	28
#define CS_PROTO_NOTEXT	29
#define CS_PROTO_NOBULK	30
#define CS_DATA_NOSENSITIVITY	31
#define CS_DATA_NOBOUNDARY	32
#define CS_RES_NOTDSDEBUG	33
#define CS_RES_NOSTRIPBLANKS	34
#define CS_DATA_NOINT8	35

/* Properties */
enum
{
/*
 * These defines looks weird but programs can test support for defines,
 * compiler can check enum and there are no define side effecs
 */
	CS_USERNAME = 9100,
#define CS_USERNAME CS_USERNAME
	CS_PASSWORD = 9101,
#define CS_PASSWORD CS_PASSWORD
	CS_APPNAME = 9102,
#define CS_APPNAME CS_APPNAME
	CS_HOSTNAME = 9103,
#define CS_HOSTNAME CS_HOSTNAME
	CS_LOGIN_STATUS = 9104,
#define CS_LOGIN_STATUS CS_LOGIN_STATUS
	CS_TDS_VERSION = 9105,
#define CS_TDS_VERSION CS_TDS_VERSION
	CS_CHARSETCNV = 9106,
#define CS_CHARSETCNV CS_CHARSETCNV
	CS_PACKETSIZE = 9107,
#define CS_PACKETSIZE CS_PACKETSIZE
	CS_USERDATA = 9108,
#define CS_USERDATA CS_USERDATA
	CS_NETIO = 9110,
#define CS_NETIO CS_NETIO
	CS_TEXTLIMIT = 9112,
#define CS_TEXTLIMIT CS_TEXTLIMIT
	CS_HIDDEN_KEYS = 9113,
#define CS_HIDDEN_KEYS CS_HIDDEN_KEYS
	CS_VERSION = 9114,
#define CS_VERSION CS_VERSION
	CS_IFILE = 9115,
#define CS_IFILE CS_IFILE
	CS_LOGIN_TIMEOUT = 9116,
#define CS_LOGIN_TIMEOUT CS_LOGIN_TIMEOUT
	CS_TIMEOUT = 9117,
#define CS_TIMEOUT CS_TIMEOUT
	CS_MAX_CONNECT = 9118,
#define CS_MAX_CONNECT CS_MAX_CONNECT
	CS_EXPOSE_FMTS = 9120,
#define CS_EXPOSE_FMTS CS_EXPOSE_FMTS
	CS_EXTRA_INF = 9121,
#define CS_EXTRA_INF CS_EXTRA_INF
	CS_ANSI_BINDS = 9123,
#define CS_ANSI_BINDS CS_ANSI_BINDS
	CS_BULK_LOGIN = 9124,
#define CS_BULK_LOGIN CS_BULK_LOGIN
	CS_LOC_PROP = 9125,
#define CS_LOC_PROP CS_LOC_PROP
	CS_PARENT_HANDLE = 9130,
#define CS_PARENT_HANDLE CS_PARENT_HANDLE
	CS_EED_CMD = 9131,
#define CS_EED_CMD CS_EED_CMD
	CS_DIAG_TIMEOUT = 9132,
#define CS_DIAG_TIMEOUT CS_DIAG_TIMEOUT
	CS_DISABLE_POLL = 9133,
#define CS_DISABLE_POLL CS_DISABLE_POLL
	CS_SEC_ENCRYPTION = 9135,
#define CS_SEC_ENCRYPTION CS_SEC_ENCRYPTION
	CS_SEC_CHALLENGE = 9136,
#define CS_SEC_CHALLENGE CS_SEC_CHALLENGE
	CS_SEC_NEGOTIATE = 9137,
#define CS_SEC_NEGOTIATE CS_SEC_NEGOTIATE
	CS_CON_STATUS = 9143,
#define CS_CON_STATUS CS_CON_STATUS
	CS_VER_STRING = 9144,
#define CS_VER_STRING CS_VER_STRING
	CS_SERVERNAME = 9146,
#define CS_SERVERNAME CS_SERVERNAME
	CS_SEC_APPDEFINED = 9149,
#define CS_SEC_APPDEFINED CS_SEC_APPDEFINED
	CS_STICKY_BINDS = 9151,
#define CS_STICKY_BINDS CS_STICKY_BINDS
	CS_SERVERADDR = 9206,
#define CS_SERVERADDR CS_SERVERADDR
	CS_PORT = 9300
#define CS_PORT CS_PORT
};

/* Arbitrary precision math operators */
enum
{
	CS_ADD = 1,
	CS_SUB,
	CS_MULT,
	CS_DIV
};

enum
{
	CS_TDS_40 = 7360,
	CS_TDS_42,
	CS_TDS_46,
	CS_TDS_495,
	CS_TDS_50,
	CS_TDS_70,
	CS_TDS_80
};

/* bit mask values used by CS_DATAFMT.status */
#define CS_HIDDEN      (1 <<  0)
#define CS_KEY         (1 <<  1)
#define CS_VERSION_KEY (1 <<  2)
#define CS_NODATA      (1 <<  3)
#define CS_UPDATABLE   (1 <<  4)
#define CS_CANBENULL   (1 <<  5)
#define CS_DESCIN      (1 <<  6)
#define CS_DESCOUT     (1 <<  7)
#define CS_INPUTVALUE  (1 <<  8)
#define CS_UPDATECOL   (1 <<  9)
#define CS_RETURN      (1 << 10)
#define CS_TIMESTAMP   (1 << 13)
#define CS_NODEFAULT   (1 << 14)
#define CS_IDENTITY    (1 << 15)

/*
 * DBD::Sybase compares indicator to CS_NULLDATA so this is -1
 * (the documentation states -1)
 */
#define CS_GOODDATA	0
#define CS_NULLDATA	(-1)

/* CS_CON_STATUS read-only property bit mask values */
#define CS_CONSTAT_CONNECTED	0x01
#define CS_CONSTAT_DEAD 	0x02

/*
 * Code added for CURSOR support
 * types accepted by ct_cursor
 */
#define CS_CURSOR_DECLARE  700
#define CS_CURSOR_OPEN     701
#define CS_CURSOR_ROWS     703
#define CS_CURSOR_UPDATE   704
#define CS_CURSOR_DELETE   705
#define CS_CURSOR_CLOSE    706
#define CS_CURSOR_DEALLOC  707
#define CS_CURSOR_OPTION   725

#define CS_FOR_UPDATE      TDS_STATIC_CAST(CS_INT, 0x1)
#define CS_READ_ONLY       TDS_STATIC_CAST(CS_INT, 0x2)
#define CS_RESTORE_OPEN    TDS_STATIC_CAST(CS_INT, 0x8)
#define CS_IMPLICIT_CURSOR TDS_STATIC_CAST(CS_INT, 0x40)


#define CS_CURSTAT_NONE      TDS_STATIC_CAST(CS_INT, 0x0)
#define CS_CURSTAT_DECLARED  TDS_STATIC_CAST(CS_INT, 0x1)
#define CS_CURSTAT_OPEN      TDS_STATIC_CAST(CS_INT, 0x2)
#define CS_CURSTAT_CLOSED    TDS_STATIC_CAST(CS_INT, 0x4)
#define CS_CURSTAT_RDONLY    TDS_STATIC_CAST(CS_INT, 0x8)
#define CS_CURSTAT_UPDATABLE TDS_STATIC_CAST(CS_INT, 0x10)
#define CS_CURSTAT_ROWCOUNT  TDS_STATIC_CAST(CS_INT, 0x20)
#define CS_CURSTAT_DEALLOC   TDS_STATIC_CAST(CS_INT, 0x40)

#define CS_CUR_STATUS        TDS_STATIC_CAST(CS_INT, 9126)
#define CS_CUR_ID            TDS_STATIC_CAST(CS_INT, 9127)
#define CS_CUR_NAME          TDS_STATIC_CAST(CS_INT, 9128)
#define CS_CUR_ROWCOUNT      TDS_STATIC_CAST(CS_INT, 9129)

/* options accepted by ct_options() */
#define CS_OPT_DATEFIRST	5001
#define CS_OPT_TEXTSIZE		5002
#define CS_OPT_STATS_TIME	5003
#define CS_OPT_STATS_IO		5004
#define CS_OPT_ROWCOUNT		5005
#define CS_OPT_DATEFORMAT	5007
#define CS_OPT_ISOLATION	5008
#define CS_OPT_AUTHON		5009
#define CS_OPT_SHOWPLAN		5013
#define CS_OPT_NOEXEC		5014
#define CS_OPT_ARITHIGNORE	5015
#define CS_OPT_TRUNCIGNORE	5016
#define CS_OPT_ARITHABORT	5017
#define CS_OPT_PARSEONLY	5018
#define CS_OPT_GETDATA		5020
#define CS_OPT_NOCOUNT		5021
#define CS_OPT_FORCEPLAN	5023
#define CS_OPT_FORMATONLY	5024
#define CS_OPT_CHAINXACTS	5025
#define CS_OPT_CURCLOSEONXACT	5026
#define CS_OPT_FIPSFLAG		5027
#define CS_OPT_RESTREES		5028
#define CS_OPT_IDENTITYON	5029
#define CS_OPT_CURREAD		5030
#define CS_OPT_CURWRITE		5031
#define CS_OPT_IDENTITYOFF	5032
#define CS_OPT_AUTHOFF		5033
#define CS_OPT_ANSINULL		5034
#define CS_OPT_QUOTED_IDENT	5035
#define CS_OPT_ANSIPERM		5036
#define CS_OPT_STR_RTRUNC	5037

/* options accepted by ct_command() */
enum ct_command_options
{
	CS_MORE = 16,
	CS_END = 32,
	CS_RECOMPILE = 188,
	CS_NO_RECOMPILE,
	CS_BULK_INIT,
	CS_BULK_CONT,
	CS_BULK_DATA,
	CS_COLUMN_DATA
};


/*
 * bind formats, should be mapped to TDS types
 * can be a combination of bit
 */
enum
{
	CS_FMT_UNUSED = 0,
#define CS_FMT_UNUSED CS_FMT_UNUSED
	CS_FMT_NULLTERM = 1,
#define CS_FMT_NULLTERM CS_FMT_NULLTERM
	CS_FMT_PADNULL = 2,
#define CS_FMT_PADBLANK CS_FMT_PADBLANK
	CS_FMT_PADBLANK = 4,
#define CS_FMT_PADNULL CS_FMT_PADNULL
	CS_FMT_JUSTIFY_RT = 8
#define CS_FMT_JUSTIFY_RT CS_FMT_JUSTIFY_RT
};

/* callbacks */
#define CS_COMPLETION_CB	1
#define CS_SERVERMSG_CB		2
#define CS_CLIENTMSG_CB		3
#define CS_NOTIF_CB		4
#define CS_ENCRYPT_CB		5
#define CS_CHALLENGE_CB		6
#define CS_DS_LOOKUP_CB		7
#define CS_SECSESSION_CB	8
#define CS_SIGNAL_CB		100
#define CS_MESSAGE_CB		9119

/* string types */
#define CS_NULLTERM	-9
#define CS_WILDCARD	-99
#define CS_NO_LIMIT	-9999
#define CS_UNUSED	-99999

/* other */
#define CS_GET		33
#define CS_SET		34
#define CS_CLEAR	35
#define CS_INIT         36
#define CS_STATUS       37
#define CS_MSGLIMIT     38
#define CS_SUPPORTED    40

#define CS_CMD_DONE	4046
#define CS_CMD_SUCCEED	4047
#define CS_CMD_FAIL	4048

/* commands */
#define CS_LANG_CMD	 148
#define CS_RPC_CMD	 149
#define CS_SEND_DATA_CMD 151
#define CS_SEND_BULK_CMD 153

#define CS_VERSION_100	112
#define CS_VERSION_110	1100
#define CS_VERSION_120	1100
#define CS_VERSION_125	12500
#define CS_VERSION_150	15000

#define BLK_VERSION_100 CS_VERSION_100
#define BLK_VERSION_110 CS_VERSION_110
#define BLK_VERSION_120 CS_VERSION_120
#define BLK_VERSION_125 CS_VERSION_125
#define BLK_VERSION_150 CS_VERSION_150

#define CS_FORCE_EXIT	300
#define CS_FORCE_CLOSE  301

#define CS_SYNC_IO	8111
#define CS_ASYNC_IO	8112
#define CS_DEFER_IO	8113

#define CS_CANCEL_CURRENT 6000
#define CS_CANCEL_ALL	  6001
#define CS_CANCEL_ATTN	  6002

#define CS_ROW_COUNT	800
#define CS_CMD_NUMBER	801
#define CS_NUM_COMPUTES	802
#define CS_NUMDATA	803
#define CS_NUMORDERCOLS	805
#define CS_MSGTYPE      806
#define CS_BROWSE_INFO	807
#define CS_TRANS_STATE	808

#define CS_TRAN_UNDEFINED   0
#define CS_TRAN_IN_PROGRESS 1
#define CS_TRAN_COMPLETED   2
#define CS_TRAN_FAIL        3
#define CS_TRAN_STMT_FAIL   4

#define CS_COMP_OP	5350
#define CS_COMP_ID	5351
#define CS_COMP_COLID	5352
#define CS_COMP_BYLIST	5353
#define CS_BYLIST_LEN	5354

#define CS_NO_COUNT	-1

#define CS_OP_SUM	5370
#define CS_OP_AVG	5371
#define CS_OP_COUNT	5372
#define CS_OP_MIN	5373
#define CS_OP_MAX	5374

#define CS_CAP_REQUEST	1
#define CS_CAP_RESPONSE	2

#define CS_PREPARE	717
#define CS_EXECUTE	718
#define CS_DESCRIBE_INPUT	720
#define CS_DESCRIBE_OUTPUT	721

#define CS_DEALLOC	711

#define CS_LC_ALL	     7
#define CS_SYB_LANG	     8
#define CS_SYB_CHARSET	     9
#define CS_SYB_SORTORDER     10
#define CS_SYB_COLLATE CS_SYB_SORTORDER
#define CS_SYB_LANG_CHARSET  11

#define CS_BLK_IN	1
#define CS_BLK_OUT	2

#define CS_BLK_BATCH	1
#define CS_BLK_ALL	2
#define CS_BLK_CANCEL	3

/* to do support these */

#define CS_BLK_ARRAY_MAXLEN 0x1000
#define CS_DEF_PREC     18

/* Error Severities  */
#define CS_SV_INFORM        TDS_STATIC_CAST(CS_INT, 0)
#define CS_SV_API_FAIL      TDS_STATIC_CAST(CS_INT, 1)
#define CS_SV_RETRY_FAIL    TDS_STATIC_CAST(CS_INT, 2)
#define CS_SV_RESOURCE_FAIL TDS_STATIC_CAST(CS_INT, 3)
#define CS_SV_CONFIG_FAIL   TDS_STATIC_CAST(CS_INT, 4)
#define CS_SV_COMM_FAIL     TDS_STATIC_CAST(CS_INT, 5)
#define CS_SV_INTERNAL_FAIL TDS_STATIC_CAST(CS_INT, 6)
#define CS_SV_FATAL         TDS_STATIC_CAST(CS_INT, 7)

/* result_types */
#define CS_COMPUTE_RESULT	4045
#define CS_CURSOR_RESULT	4041
#define CS_PARAM_RESULT		4042
#define CS_ROW_RESULT		4040
#define CS_STATUS_RESULT	4043
#define CS_COMPUTEFMT_RESULT	4050
#define CS_ROWFMT_RESULT	4049
#define CS_MSG_RESULT		4044
#define CS_DESCRIBE_RESULT	4051

/* bind types */
#define CS_ILLEGAL_TYPE     TDS_STATIC_CAST(CS_INT, -1)
#define CS_CHAR_TYPE        TDS_STATIC_CAST(CS_INT, 0)
#define CS_BINARY_TYPE      TDS_STATIC_CAST(CS_INT, 1)
#define CS_LONGCHAR_TYPE    TDS_STATIC_CAST(CS_INT, 2)
#define CS_LONGBINARY_TYPE  TDS_STATIC_CAST(CS_INT, 3)
#define CS_TEXT_TYPE        TDS_STATIC_CAST(CS_INT, 4)
#define CS_IMAGE_TYPE       TDS_STATIC_CAST(CS_INT, 5)
#define CS_TINYINT_TYPE     TDS_STATIC_CAST(CS_INT, 6)
#define CS_SMALLINT_TYPE    TDS_STATIC_CAST(CS_INT, 7)
#define CS_INT_TYPE         TDS_STATIC_CAST(CS_INT, 8)
#define CS_REAL_TYPE        TDS_STATIC_CAST(CS_INT, 9)
#define CS_FLOAT_TYPE       TDS_STATIC_CAST(CS_INT, 10)
#define CS_BIT_TYPE         TDS_STATIC_CAST(CS_INT, 11)
#define CS_DATETIME_TYPE    TDS_STATIC_CAST(CS_INT, 12)
#define CS_DATETIME4_TYPE   TDS_STATIC_CAST(CS_INT, 13)
#define CS_MONEY_TYPE       TDS_STATIC_CAST(CS_INT, 14)
#define CS_MONEY4_TYPE      TDS_STATIC_CAST(CS_INT, 15)
#define CS_NUMERIC_TYPE     TDS_STATIC_CAST(CS_INT, 16)
#define CS_DECIMAL_TYPE     TDS_STATIC_CAST(CS_INT, 17)
#define CS_VARCHAR_TYPE     TDS_STATIC_CAST(CS_INT, 18)
#define CS_VARBINARY_TYPE   TDS_STATIC_CAST(CS_INT, 19)
#define CS_LONG_TYPE        TDS_STATIC_CAST(CS_INT, 20)
#define CS_SENSITIVITY_TYPE TDS_STATIC_CAST(CS_INT, 21)
#define CS_BOUNDARY_TYPE    TDS_STATIC_CAST(CS_INT, 22)
#define CS_VOID_TYPE        TDS_STATIC_CAST(CS_INT, 23)
#define CS_USHORT_TYPE      TDS_STATIC_CAST(CS_INT, 24)
#define CS_UNICHAR_TYPE     TDS_STATIC_CAST(CS_INT, 25)
#define CS_BLOB_TYPE        TDS_STATIC_CAST(CS_INT, 26)
#define CS_DATE_TYPE        TDS_STATIC_CAST(CS_INT, 27)
#define CS_TIME_TYPE        TDS_STATIC_CAST(CS_INT, 28)
#define CS_UNITEXT_TYPE     TDS_STATIC_CAST(CS_INT, 29)
#define CS_BIGINT_TYPE      TDS_STATIC_CAST(CS_INT, 30)
#define CS_USMALLINT_TYPE   TDS_STATIC_CAST(CS_INT, 31)
#define CS_UINT_TYPE        TDS_STATIC_CAST(CS_INT, 32)
#define CS_UBIGINT_TYPE     TDS_STATIC_CAST(CS_INT, 33)
#define CS_XML_TYPE         TDS_STATIC_CAST(CS_INT, 34)
#define CS_UNIQUE_TYPE      TDS_STATIC_CAST(CS_INT, 40)

#define CS_USER_TYPE        TDS_STATIC_CAST(CS_INT, 100)
/* cs_dt_info type values */
enum
{
	CS_MONTH = 7340,
#define CS_MONTH CS_MONTH
	CS_SHORTMONTH,
#define CS_SHORTMONTH CS_SHORTMONTH
	CS_DAYNAME,
#define CS_DAYNAME CS_DAYNAME
	CS_DATEORDER,
#define CS_DATEORDER CS_DATEORDER
	CS_12HOUR,
#define CS_12HOUR CS_12HOUR
	CS_DT_CONVFMT
#define CS_DT_CONVFMT CS_DT_CONVFMT
};

/* DT_CONVFMT types */
enum
{
	CS_DATES_SHORT = 0,
#define CS_DATES_SHORT CS_DATES_SHORT
	CS_DATES_MDY1,
#define CS_DATES_MDY1 CS_DATES_MDY1
	CS_DATES_YMD1,
#define CS_DATES_YMD1 CS_DATES_YMD1
	CS_DATES_DMY1,
#define CS_DATES_DMY1 CS_DATES_DMY1
	CS_DATES_DMY2,
#define CS_DATES_DMY2 CS_DATES_DMY2
	CS_DATES_DMY3,
#define CS_DATES_DMY3 CS_DATES_DMY3
	CS_DATES_DMY4,
#define CS_DATES_DMY4 CS_DATES_DMY4
	CS_DATES_MDY2,
#define CS_DATES_MDY2 CS_DATES_MDY2
	CS_DATES_HMS,
#define CS_DATES_HMS CS_DATES_HMS
	CS_DATES_LONG,
#define CS_DATES_LONG CS_DATES_LONG
	CS_DATES_MDY3,
#define CS_DATES_MDY3 CS_DATES_MDY3
	CS_DATES_YMD2,
#define CS_DATES_YMD2 CS_DATES_YMD2
	CS_DATES_YMD3,
#define CS_DATES_YMD3 CS_DATES_YMD3
	CS_DATES_YDM1,
#define CS_DATES_YDM1 CS_DATES_YDM1
	CS_DATES_MYD1,
#define CS_DATES_MYD1 CS_DATES_MYD1
	CS_DATES_DYM1,
#define CS_DATES_DYM1 CS_DATES_DYM1
	CS_DATES_MDY1_YYYY = 101,
#define CS_DATES_MDY1_YYYY CS_DATES_MDY1_YYYY
	CS_DATES_YMD1_YYYY,
#define CS_DATES_YMD1_YYYY CS_DATES_YMD1_YYYY
	CS_DATES_DMY1_YYYY,
#define CS_DATES_DMY1_YYYY CS_DATES_DMY1_YYYY
	CS_DATES_DMY2_YYYY,
#define CS_DATES_DMY2_YYYY CS_DATES_DMY2_YYYY
	CS_DATES_DMY3_YYYY,
#define CS_DATES_DMY3_YYYY CS_DATES_DMY3_YYYY
	CS_DATES_DMY4_YYYY,
#define CS_DATES_DMY4_YYYY CS_DATES_DMY4_YYYY
	CS_DATES_MDY2_YYYY,
#define CS_DATES_MDY2_YYYY CS_DATES_MDY2_YYYY
	CS_DATES_MDY3_YYYY = 110,
#define CS_DATES_MDY3_YYYY CS_DATES_MDY3_YYYY
	CS_DATES_YMD2_YYYY,
#define CS_DATES_YMD2_YYYY CS_DATES_YMD2_YYYY
	CS_DATES_YMD3_YYYY
#define CS_DATES_YMD3_YYYY CS_DATES_YMD3_YYYY
};

typedef CS_RETCODE(*CS_CONV_FUNC) (CS_CONTEXT * context, CS_DATAFMT * srcfmt, CS_VOID * src, CS_DATAFMT * detsfmt, CS_VOID * dest,
				   CS_INT * destlen);

typedef struct _cs_objname
{
	CS_BOOL thinkexists;
	CS_INT object_type;
	CS_CHAR last_name[CS_MAX_NAME];
	CS_INT lnlen;
	CS_CHAR first_name[CS_MAX_NAME];
	CS_INT fnlen;
	CS_VOID *scope;
	CS_INT scopelen;
	CS_VOID *thread;
	CS_INT threadlen;
} CS_OBJNAME;

typedef struct _cs_objdata
{
	CS_BOOL actuallyexists;
	CS_CONNECTION *connection;
	CS_COMMAND *command;
	CS_VOID *buffer;
	CS_INT buflen;
} CS_OBJDATA;

/* Eventually, these should be in terms of TDS values */
enum
{
	CS_OPT_MONDAY = 1,
	CS_OPT_TUESDAY,
	CS_OPT_WEDNESDAY,
	CS_OPT_THURSDAY,
	CS_OPT_FRIDAY,
	CS_OPT_SATURDAY,
	CS_OPT_SUNDAY
};
enum
{
	CS_OPT_FMTMDY = 1,
	CS_OPT_FMTDMY,
	CS_OPT_FMTYMD,
	CS_OPT_FMTYDM,
	CS_OPT_FMTMYD,
	CS_OPT_FMTDYM
};
enum
{
	CS_OPT_LEVEL0 = 0,
	CS_OPT_LEVEL1,
	CS_OPT_LEVEL2,
	CS_OPT_LEVEL3
};

#define CS_FALSE	0
#define CS_TRUE	1

#define SRV_PROC	CS_VOID

/* constants required for ct_diag (not jet implemented) */
#define CS_CLIENTMSG_TYPE 4700
#define CS_SERVERMSG_TYPE 4701
#define CS_ALLMSG_TYPE 4702

CS_RETCODE cs_convert(CS_CONTEXT * ctx, CS_DATAFMT * srcfmt, CS_VOID * srcdata, CS_DATAFMT * destfmt, CS_VOID * destdata,
		      CS_INT * resultlen);
CS_RETCODE cs_ctx_alloc(CS_INT version, CS_CONTEXT ** ctx);
CS_RETCODE cs_ctx_global(CS_INT version, CS_CONTEXT ** ctx);
CS_RETCODE cs_ctx_drop(CS_CONTEXT * ctx);
CS_RETCODE cs_config(CS_CONTEXT * ctx, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE cs_strbuild(CS_CONTEXT * ctx, CS_CHAR * buffer, CS_INT buflen, CS_INT * resultlen, CS_CHAR * text, CS_INT textlen,
		       CS_CHAR * formats, CS_INT formatlen, ...);
CS_RETCODE cs_dt_crack(CS_CONTEXT * ctx, CS_INT datetype, CS_VOID * dateval, CS_DATEREC * daterec);
CS_RETCODE cs_loc_alloc(CS_CONTEXT * ctx, CS_LOCALE ** locptr);
CS_RETCODE cs_loc_drop(CS_CONTEXT * ctx, CS_LOCALE * locale);
CS_RETCODE cs_locale(CS_CONTEXT * ctx, CS_INT action, CS_LOCALE * locale, CS_INT type, CS_VOID * buffer, CS_INT buflen,
		     CS_INT * outlen);
CS_RETCODE cs_dt_info(CS_CONTEXT * ctx, CS_INT action, CS_LOCALE * locale, CS_INT type, CS_INT item, CS_VOID * buffer,
		      CS_INT buflen, CS_INT * outlen);

CS_RETCODE cs_calc(CS_CONTEXT * ctx, CS_INT op, CS_INT datatype, CS_VOID * var1, CS_VOID * var2, CS_VOID * dest);
CS_RETCODE cs_cmp(CS_CONTEXT * ctx, CS_INT datatype, CS_VOID * var1, CS_VOID * var2, CS_INT * result);
CS_RETCODE cs_conv_mult(CS_CONTEXT * ctx, CS_LOCALE * srcloc, CS_LOCALE * destloc, CS_INT * conv_multiplier);
CS_RETCODE cs_diag(CS_CONTEXT * ctx, CS_INT operation, CS_INT type, CS_INT idx, CS_VOID * buffer);
CS_RETCODE cs_manage_convert(CS_CONTEXT * ctx, CS_INT action, CS_INT srctype, CS_CHAR * srcname, CS_INT srcnamelen, CS_INT desttype,
			     CS_CHAR * destname, CS_INT destnamelen, CS_INT * conv_multiplier, CS_CONV_FUNC * func);
CS_RETCODE cs_objects(CS_CONTEXT * ctx, CS_INT action, CS_OBJNAME * objname, CS_OBJDATA * objdata);
CS_RETCODE cs_set_convert(CS_CONTEXT * ctx, CS_INT action, CS_INT srctype, CS_INT desttype, CS_CONV_FUNC * func);
CS_RETCODE cs_setnull(CS_CONTEXT * ctx, CS_DATAFMT * datafmt, CS_VOID * buffer, CS_INT buflen);
CS_RETCODE cs_strcmp(CS_CONTEXT * ctx, CS_LOCALE * locale, CS_INT type, CS_CHAR * str1, CS_INT len1, CS_CHAR * str2, CS_INT len2,
		     CS_INT * result);
CS_RETCODE cs_time(CS_CONTEXT * ctx, CS_LOCALE * locale, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen, CS_DATEREC * daterec);
CS_RETCODE cs_will_convert(CS_CONTEXT * ctx, CS_INT srctype, CS_INT desttype, CS_BOOL * result);

const char * cs_prretcode(int retcode);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
