/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004, 2005  Brian Bruns
 * Copyright (C) 2010, 2011  Frediano Ziglio
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

/*
 * This file contains defines and structures strictly related to TDS protocol
 */

typedef struct tdsnumeric
{
	unsigned char precision;
	unsigned char scale;
	unsigned char array[33];
} TDS_NUMERIC;

typedef struct tdsoldmoney
{
	TDS_INT mnyhigh;
	TDS_UINT mnylow;
} TDS_OLD_MONEY;

typedef union tdsmoney
{
	TDS_OLD_MONEY tdsoldmoney;
	TDS_INT8 mny;
} TDS_MONEY;

typedef struct tdsmoney4
{
	TDS_INT mny4;
} TDS_MONEY4;

typedef struct tdsdatetime
{
	TDS_INT dtdays;
	TDS_INT dttime;
} TDS_DATETIME;

typedef struct tdsdatetime4
{
	TDS_USMALLINT days;
	TDS_USMALLINT minutes;
} TDS_DATETIME4;

typedef struct tdsunique
{
	TDS_UINT Data1;
	TDS_USMALLINT Data2;
	TDS_USMALLINT Data3;
	TDS_UCHAR Data4[8];
} TDS_UNIQUE;

#define TDS5_PARAMFMT2_TOKEN       32	/* 0x20 */
#define TDS_LANGUAGE_TOKEN         33	/* 0x21    TDS 5.0 only              */
#define TDS_ORDERBY2_TOKEN         34	/* 0x22 */
#define TDS_ROWFMT2_TOKEN          97	/* 0x61    TDS 5.0 only              */
#define TDS_LOGOUT_TOKEN          113	/* 0x71    TDS 5.0 only? ct_close()  */
#define TDS_RETURNSTATUS_TOKEN    121	/* 0x79                              */
#define TDS_PROCID_TOKEN          124	/* 0x7C    TDS 4.2 only - TDS_PROCID */
#define TDS7_RESULT_TOKEN         129	/* 0x81    TDS 7.0 only              */
#define TDS7_COMPUTE_RESULT_TOKEN 136	/* 0x88    TDS 7.0 only              */
#define TDS_COLNAME_TOKEN         160	/* 0xA0    TDS 4.2 only              */
#define TDS_COLFMT_TOKEN          161	/* 0xA1    TDS 4.2 only - TDS_COLFMT */
#define TDS_DYNAMIC2_TOKEN        163	/* 0xA3 */
#define TDS_TABNAME_TOKEN         164	/* 0xA4 */
#define TDS_COLINFO_TOKEN         165	/* 0xA5 */
#define TDS_OPTIONCMD_TOKEN   	  166	/* 0xA6 */
#define TDS_COMPUTE_NAMES_TOKEN   167	/* 0xA7 */
#define TDS_COMPUTE_RESULT_TOKEN  168	/* 0xA8 */
#define TDS_ORDERBY_TOKEN         169	/* 0xA9    TDS_ORDER                 */
#define TDS_ERROR_TOKEN           170	/* 0xAA                              */
#define TDS_INFO_TOKEN            171	/* 0xAB                              */
#define TDS_PARAM_TOKEN           172	/* 0xAC    RETURNVALUE?              */
#define TDS_LOGINACK_TOKEN        173	/* 0xAD                              */
#define TDS_CONTROL_TOKEN         174	/* 0xAE    TDS_CONTROL               */
#define TDS_ROW_TOKEN             209	/* 0xD1                              */
#define TDS_NBC_ROW_TOKEN         210	/* 0xD2    as of TDS 7.3.B           */ /* not implemented */
#define TDS_CMP_ROW_TOKEN         211	/* 0xD3                              */
#define TDS5_PARAMS_TOKEN         215	/* 0xD7    TDS 5.0 only              */
#define TDS_CAPABILITY_TOKEN      226	/* 0xE2                              */
#define TDS_ENVCHANGE_TOKEN       227	/* 0xE3                              */
#define TDS_EED_TOKEN             229	/* 0xE5                              */
#define TDS_DBRPC_TOKEN           230	/* 0xE6                              */
#define TDS5_DYNAMIC_TOKEN        231	/* 0xE7    TDS 5.0 only              */
#define TDS5_PARAMFMT_TOKEN       236	/* 0xEC    TDS 5.0 only              */
#define TDS_AUTH_TOKEN            237	/* 0xED    TDS 7.0 only              */
#define TDS_RESULT_TOKEN          238	/* 0xEE                              */
#define TDS_DONE_TOKEN            253	/* 0xFD    TDS_DONE                  */
#define TDS_DONEPROC_TOKEN        254	/* 0xFE    TDS_DONEPROC              */
#define TDS_DONEINPROC_TOKEN      255	/* 0xFF    TDS_DONEINPROC            */

/* CURSOR support: TDS 5.0 only*/
#define TDS_CURCLOSE_TOKEN        128  /* 0x80    TDS 5.0 only              */
#define TDS_CURDELETE_TOKEN       129  /* 0x81    TDS 5.0 only              */
#define TDS_CURFETCH_TOKEN        130  /* 0x82    TDS 5.0 only              */
#define TDS_CURINFO_TOKEN         131  /* 0x83    TDS 5.0 only              */
#define TDS_CUROPEN_TOKEN         132  /* 0x84    TDS 5.0 only              */
#define TDS_CURDECLARE_TOKEN      134  /* 0x86    TDS 5.0 only              */


/* environment type field */
#define TDS_ENV_DATABASE  	1
#define TDS_ENV_LANG      	2
#define TDS_ENV_CHARSET   	3
#define TDS_ENV_PACKSIZE  	4
#define TDS_ENV_LCID        	5
#define TDS_ENV_SQLCOLLATION	7
#define TDS_ENV_BEGINTRANS	8
#define TDS_ENV_COMMITTRANS	9
#define TDS_ENV_ROLLBACKTRANS	10

/* Microsoft internal stored procedure id's */
#define TDS_SP_CURSOR           1
#define TDS_SP_CURSOROPEN       2
#define TDS_SP_CURSORPREPARE    3
#define TDS_SP_CURSOREXECUTE    4
#define TDS_SP_CURSORPREPEXEC   5
#define TDS_SP_CURSORUNPREPARE  6
#define TDS_SP_CURSORFETCH      7
#define TDS_SP_CURSOROPTION     8
#define TDS_SP_CURSORCLOSE      9
#define TDS_SP_EXECUTESQL      10
#define TDS_SP_PREPARE         11
#define TDS_SP_EXECUTE         12
#define TDS_SP_PREPEXEC        13
#define TDS_SP_PREPEXECRPC     14
#define TDS_SP_UNPREPARE       15

/* 
 * <rant> Sybase does an awful job of this stuff, non null ints of size 1 2 
 * and 4 have there own codes but nullable ints are lumped into INTN
 * sheesh! </rant>
 */
typedef enum
{
	SYBCHAR = 47,		/* 0x2F */
	SYBVARCHAR = 39,	/* 0x27 */
	SYBINTN = 38,		/* 0x26 */
	SYBINT1 = 48,		/* 0x30 */
	SYBINT2 = 52,		/* 0x34 */
	SYBINT4 = 56,		/* 0x38 */
	SYBFLT8 = 62,		/* 0x3E */
	SYBDATETIME = 61,	/* 0x3D */
	SYBBIT = 50,		/* 0x32 */
	SYBTEXT = 35,		/* 0x23 */
	SYBNTEXT = 99,		/* 0x63 */
	SYBIMAGE = 34,		/* 0x22 */
	SYBMONEY4 = 122,	/* 0x7A */
	SYBMONEY = 60,		/* 0x3C */
	SYBDATETIME4 = 58,	/* 0x3A */
	SYBREAL = 59,		/* 0x3B */
	SYBBINARY = 45,		/* 0x2D */
	SYBVOID = 31,		/* 0x1F */
	SYBVARBINARY = 37,	/* 0x25 */
	SYBBITN = 104,		/* 0x68 */
	SYBNUMERIC = 108,	/* 0x6C */
	SYBDECIMAL = 106,	/* 0x6A */
	SYBFLTN = 109,		/* 0x6D */
	SYBMONEYN = 110,	/* 0x6E */
	SYBDATETIMN = 111,	/* 0x6F */

/*
 * MS only types
 */
	SYBNVARCHAR = 103,	/* 0x67 */
	SYBINT8 = 127,		/* 0x7F */
	XSYBCHAR = 175,		/* 0xAF */
	XSYBVARCHAR = 167,	/* 0xA7 */
	XSYBNVARCHAR = 231,	/* 0xE7 */
	XSYBNCHAR = 239,	/* 0xEF */
	XSYBVARBINARY = 165,	/* 0xA5 */
	XSYBBINARY = 173,	/* 0xAD */
	SYBUNIQUE = 36,		/* 0x24 */
	SYBVARIANT = 98, 	/* 0x62 */
	SYBMSUDT = 240,		/* 0xF0 */
	SYBMSXML = 241,		/* 0xF1 */
	SYBMSDATE = 40,  	/* 0x28 */
	SYBMSTIME = 41,  	/* 0x29 */
	SYBMSDATETIME2 = 42,  	/* 0x2a */
	SYBMSDATETIMEOFFSET = 43,/* 0x2b */

/*
 * Sybase only types
 */
	SYBLONGBINARY = 225,	/* 0xE1 */
	SYBUINT1 = 64,		/* 0x40 */
	SYBUINT2 = 65,		/* 0x41 */
	SYBUINT4 = 66,		/* 0x42 */
	SYBUINT8 = 67,		/* 0x43 */
	SYBBLOB = 36,		/* 0x24 */
	SYBBOUNDARY = 104,	/* 0x68 */
	SYBDATE = 49,		/* 0x31 */
	SYBDATEN = 123,		/* 0x7B */
	SYB5INT8 = 191,		/* 0xBF */
	SYBINTERVAL = 46,	/* 0x2E */
	SYBLONGCHAR = 175,	/* 0xAF */
	SYBSENSITIVITY = 103,	/* 0x67 */
	SYBSINT1 = 176,		/* 0xB0 */
	SYBTIME = 51,		/* 0x33 */
	SYBTIMEN = 147,		/* 0x93 */
	SYBUINTN = 68,		/* 0x44 */
	SYBUNITEXT = 174,	/* 0xAE */
	SYBXML = 163,		/* 0xA3 */

} TDS_SERVER_TYPE;

typedef enum
{
	USER_UNICHAR_TYPE = 34,		/* 0x22 */
	USER_UNIVARCHAR_TYPE = 35	/* 0x23 */
} TDS_USER_TYPE;

/* compute operator */
#define SYBAOPCNT  0x4b
#define SYBAOPCNTU 0x4c
#define SYBAOPSUM  0x4d
#define SYBAOPSUMU 0x4e
#define SYBAOPAVG  0x4f
#define SYBAOPAVGU 0x50
#define SYBAOPMIN  0x51
#define SYBAOPMAX  0x52

/* mssql2k compute operator */
#define SYBAOPCNT_BIG		0x09
#define SYBAOPSTDEV		0x30
#define SYBAOPSTDEVP		0x31
#define SYBAOPVAR		0x32
#define SYBAOPVARP		0x33
#define SYBAOPCHECKSUM_AGG	0x72

/** 
 * options that can be sent with a TDS_OPTIONCMD token
 */
typedef enum
{
	  TDS_OPT_SET = 1	/**< Set an option. */
	, TDS_OPT_DEFAULT = 2	/**< Set option to its default value. */
	, TDS_OPT_LIST = 3	/**< Request current setting of a specific option. */
	, TDS_OPT_INFO = 4	/**< Report current setting of a specific option. */
} TDS_OPTION_CMD;

typedef enum
{
	  TDS_OPT_DATEFIRST = 1		/* 0x01 */
	, TDS_OPT_TEXTSIZE = 2		/* 0x02 */
	, TDS_OPT_STAT_TIME = 3		/* 0x03 */
	, TDS_OPT_STAT_IO = 4		/* 0x04 */
	, TDS_OPT_ROWCOUNT = 5		/* 0x05 */
	, TDS_OPT_NATLANG = 6		/* 0x06 */
	, TDS_OPT_DATEFORMAT = 7	/* 0x07 */
	, TDS_OPT_ISOLATION = 8		/* 0x08 */
	, TDS_OPT_AUTHON = 9		/* 0x09 */
	, TDS_OPT_CHARSET = 10		/* 0x0a */
	, TDS_OPT_SHOWPLAN = 13		/* 0x0d */
	, TDS_OPT_NOEXEC = 14		/* 0x0e */
	, TDS_OPT_ARITHIGNOREON = 15	/* 0x0f */
	, TDS_OPT_ARITHABORTON = 17	/* 0x11 */
	, TDS_OPT_PARSEONLY = 18	/* 0x12 */
	, TDS_OPT_GETDATA = 20		/* 0x14 */
	, TDS_OPT_NOCOUNT = 21		/* 0x15 */
	, TDS_OPT_FORCEPLAN = 23	/* 0x17 */
	, TDS_OPT_FORMATONLY = 24	/* 0x18 */
	, TDS_OPT_CHAINXACTS = 25	/* 0x19 */
	, TDS_OPT_CURCLOSEONXACT = 26	/* 0x1a */
	, TDS_OPT_FIPSFLAG = 27		/* 0x1b */
	, TDS_OPT_RESTREES = 28		/* 0x1c */
	, TDS_OPT_IDENTITYON = 29	/* 0x1d */
	, TDS_OPT_CURREAD = 30		/* 0x1e */
	, TDS_OPT_CURWRITE = 31		/* 0x1f */
	, TDS_OPT_IDENTITYOFF = 32	/* 0x20 */
	, TDS_OPT_AUTHOFF = 33		/* 0x21 */
	, TDS_OPT_ANSINULL = 34		/* 0x22 */
	, TDS_OPT_QUOTED_IDENT = 35	/* 0x23 */
	, TDS_OPT_ARITHIGNOREOFF = 36	/* 0x24 */
	, TDS_OPT_ARITHABORTOFF = 37	/* 0x25 */
	, TDS_OPT_TRUNCABORT = 38	/* 0x26 */
} TDS_OPTION;

enum {
	TDS_OPT_ARITHOVERFLOW = 0x01,
	TDS_OPT_NUMERICTRUNC = 0x02
};

enum TDS_OPT_DATEFIRST_CHOICE
{
	TDS_OPT_MONDAY = 1, TDS_OPT_TUESDAY = 2, TDS_OPT_WEDNESDAY = 3, TDS_OPT_THURSDAY = 4, TDS_OPT_FRIDAY = 5, TDS_OPT_SATURDAY =
		6, TDS_OPT_SUNDAY = 7
};

enum TDS_OPT_DATEFORMAT_CHOICE
{
	TDS_OPT_FMTMDY = 1, TDS_OPT_FMTDMY = 2, TDS_OPT_FMTYMD = 3, TDS_OPT_FMTYDM = 4, TDS_OPT_FMTMYD = 5, TDS_OPT_FMTDYM = 6
};
enum TDS_OPT_ISOLATION_CHOICE
{
	TDS_OPT_LEVEL1 = 1, TDS_OPT_LEVEL3 = 3
};


typedef enum tds_packet_type
{
	TDS_QUERY = 1,
	TDS_LOGIN = 2,
	TDS_RPC = 3,
	TDS_REPLY = 4,
	TDS_CANCEL = 6,
	TDS_BULK = 7,
	TDS7_TRANS = 14,	/* transaction management */
	TDS_NORMAL = 15,
	TDS7_LOGIN = 16,
	TDS7_AUTH = 17,
	TDS71_PRELOGIN = 18,
	TDS72_SMP = 0x53
} TDS_PACKET_TYPE;

/** 
 * TDS 7.1 collation informations.
 */
typedef struct
{
	TDS_USMALLINT locale_id;	/* master..syslanguages.lcid */
	TDS_USMALLINT flags;
	TDS_UCHAR charset_id;		/* or zero */
} TDS71_COLLATION;

/**
 * TDS 7.2 SMP packet header
 */
typedef struct
{
	TDS_UCHAR signature;	/* TDS72_SMP */
	TDS_UCHAR type;
	TDS_USMALLINT sid;
	TDS_UINT size;
	TDS_UINT seq;
	TDS_UINT wnd;
} TDS72_SMP_HEADER;

enum {
	TDS_SMP_SYN = 1,
	TDS_SMP_ACK = 2,
	TDS_SMP_FIN = 4,
	TDS_SMP_DATA = 8,
};

/* SF stands for "sort flag" */
#define TDS_SF_BIN                   (TDS_USMALLINT) 0x100
#define TDS_SF_WIDTH_INSENSITIVE     (TDS_USMALLINT) 0x080
#define TDS_SF_KATATYPE_INSENSITIVE  (TDS_USMALLINT) 0x040
#define TDS_SF_ACCENT_SENSITIVE      (TDS_USMALLINT) 0x020
#define TDS_SF_CASE_INSENSITIVE      (TDS_USMALLINT) 0x010

/* UT stands for user type */
#define TDS_UT_TIMESTAMP             80


/* mssql login options flags */
enum option_flag1_values {
	TDS_BYTE_ORDER_X86		= 0, 
	TDS_CHARSET_ASCII		= 0, 
	TDS_DUMPLOAD_ON 		= 0, 
	TDS_FLOAT_IEEE_754		= 0, 
	TDS_INIT_DB_WARN		= 0, 
	TDS_SET_LANG_OFF		= 0, 
	TDS_USE_DB_SILENT		= 0, 
	TDS_BYTE_ORDER_68000	= 0x01, 
	TDS_CHARSET_EBDDIC		= 0x02, 
	TDS_FLOAT_VAX		= 0x04, 
	TDS_FLOAT_ND5000		= 0x08, 
	TDS_DUMPLOAD_OFF		= 0x10,	/* prevent BCP */ 
	TDS_USE_DB_NOTIFY		= 0x20, 
	TDS_INIT_DB_FATAL		= 0x40, 
	TDS_SET_LANG_ON		= 0x80
};

enum option_flag2_values {
	TDS_INIT_LANG_WARN		= 0, 
	TDS_INTEGRATED_SECURTY_OFF	= 0, 
	TDS_ODBC_OFF		= 0, 
	TDS_USER_NORMAL		= 0,	/* SQL Server login */
	TDS_INIT_LANG_REQUIRED	= 0x01, 
	TDS_ODBC_ON			= 0x02, 
	TDS_TRANSACTION_BOUNDARY71	= 0x04,	/* removed in TDS 7.2 */
	TDS_CACHE_CONNECT71		= 0x08,	/* removed in TDS 7.2 */
	TDS_USER_SERVER		= 0x10,	/* reserved */
	TDS_USER_REMUSER		= 0x20,	/* DQ login */
	TDS_USER_SQLREPL		= 0x40,	/* replication login */
	TDS_INTEGRATED_SECURITY_ON	= 0x80
};

enum option_flag3_values {	/* TDS 7.3+ */
	TDS_RESTRICTED_COLLATION	= 0, 
	TDS_CHANGE_PASSWORD		= 0x01, 
	TDS_SEND_YUKON_BINARY_XML	= 0x02, 
	TDS_REQUEST_USER_INSTANCE	= 0x04, 
	TDS_UNKNOWN_COLLATION_HANDLING	= 0x08, 
	TDS_ANY_COLLATION		= 0x10
};

/* Sybase dynamic types */
enum dynamic_types {
	TDS_DYN_PREPARE		= 0x01,
	TDS_DYN_EXEC		= 0x02,
	TDS_DYN_DEALLOC		= 0x04,
	TDS_DYN_EXEC_IMMED	= 0x08,
	TDS_DYN_PROCNAME	= 0x10,
	TDS_DYN_ACK		= 0x20,
	TDS_DYN_DESCIN		= 0x40,
	TDS_DYN_DESCOUT		= 0x80,
};

/* http://jtds.sourceforge.net/apiCursors.html */
/* Cursor scroll option, must be one of 0x01 - 0x10, OR'd with other bits */
enum {
	TDS_CUR_TYPE_KEYSET          = 0x0001, /* default */
	TDS_CUR_TYPE_DYNAMIC         = 0x0002,
	TDS_CUR_TYPE_FORWARD         = 0x0004,
	TDS_CUR_TYPE_STATIC          = 0x0008,
	TDS_CUR_TYPE_FASTFORWARDONLY = 0x0010,
	TDS_CUR_TYPE_PARAMETERIZED   = 0x1000,
	TDS_CUR_TYPE_AUTO_FETCH      = 0x2000
};

enum {
	TDS_CUR_CONCUR_READ_ONLY         = 1,
	TDS_CUR_CONCUR_SCROLL_LOCKS      = 2,
	TDS_CUR_CONCUR_OPTIMISTIC        = 4, /* default */
	TDS_CUR_CONCUR_OPTIMISTIC_VALUES = 8
};

/* TDS 4/5 login*/
#define TDS_MAXNAME 30	/* maximum login name lenghts */
#define TDS_PROGNLEN 10	/* maximum program lenght */
#define TDS_PKTLEN 6	/* maximum packet lenght in login */

