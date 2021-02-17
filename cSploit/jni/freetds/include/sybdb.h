/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003, 2004  Brian Bruns
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

/** \file ../include/sybdb.h
 *  \brief Primary include file for db-lib applications.
 *  
 *  Declares macros, functions, enumerated types, and defined tokens 
 *  used by the db-lib API.
 */

#ifndef _sybdb_h_
#define _sybdb_h_

#include "tds_sysdep_public.h"

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

static const char rcsid_sybdb_h[] = "$Id: sybdb.h,v 1.106 2011-12-05 02:26:31 jklowden Exp $";
static const void *const no_unused_sybdb_h_warn[] = { rcsid_sybdb_h, no_unused_sybdb_h_warn };

#ifdef FALSE
#undef FALSE
#endif
#ifdef TRUE
#undef TRUE
#endif
#define FALSE 0
#define TRUE  1

#define DBSAVE   1
#define DBNOSAVE 0
#define DBNOERR  -1

#define INT_EXIT	0
#define INT_CONTINUE	1
#define INT_CANCEL	2
#define INT_TIMEOUT	3

#define DBMAXNUMLEN 33
#define DBMAXNAME   30

/**
 * DBVERSION_xxx are used with dbsetversion()
 */
#define DBVERSION_UNKNOWN 0
#define DBVERSION_46      1
#define DBVERSION_100     2
#define DBVERSION_42      3
#define DBVERSION_70      4
#define DBVERSION_71      5
#define DBVERSION_80      DBVERSION_71
#define DBVERSION_72      6
#define DBVERSION_73      7

/* these two are defined by Microsoft for dbsetlversion() */
#define DBVER42 	  DBVERSION_42
#define DBVER60 	  DBVERSION_70	/* our best approximation */

/**
 * DBTDS_xxx are returned by DBTDS()
 * The integer values of the constants are poorly chosen.  
 */
#define DBTDS_UNKNOWN           0
#define DBTDS_2_0               1	/* pre 4.0 SQL Server */
#define DBTDS_3_4               2	/* Microsoft SQL Server (3.0) */
#define DBTDS_4_0               3	/* 4.0 SQL Server */
#define DBTDS_4_2               4	/* 4.2 SQL Server */
#define DBTDS_4_6               5	/* 2.0 OpenServer and 4.6 SQL Server. */
#define DBTDS_4_9_5             6	/* 4.9.5 (NCR) SQL Server */
#define DBTDS_5_0               7	/* 5.0 SQL Server */
#define DBTDS_7_0               8	/* Microsoft SQL Server 7.0 */
#define DBTDS_8_0               9	/* Microsoft SQL Server 2000 */
#define DBTDS_9_0               10	/* Microsoft SQL Server 2005 */
#define DBTDS_7_1               9	/* Microsoft SQL Server 2000 */
#define DBTDS_7_2               10	/* Microsoft SQL Server 2005 */
#define DBTDS_7_3               11	/* Microsoft SQL Server 2008 */

#define DBTXPLEN 16

#define BCPMAXERRS 1
#define BCPFIRST 2
#define BCPLAST 3
#define BCPBATCH 4
#define BCPKEEPIDENTITY	8

#define BCPLABELED 5
#define BCPHINTS 6

#define DBCMDNONE 0
#define DBCMDPEND 1
#define DBCMDSENT 2

typedef int RETCODE;

typedef void DBCURSOR;
typedef void DBXLATE;
typedef void DBSORTORDER;
typedef void DBLOGINFO;
typedef void *DBVOIDPTR;
typedef short SHORT;
typedef unsigned short USHORT;
typedef int (*INTFUNCPTR) (void *, ...);
typedef int (*DBWAITFUNC) (void);
typedef DBWAITFUNC(*DB_DBBUSY_FUNC) (void *dbproc);
typedef void (*DB_DBIDLE_FUNC) (DBWAITFUNC dfunc, void *dbproc);
typedef int (*DB_DBCHKINTR_FUNC) (void *dbproc);
typedef int (*DB_DBHNDLINTR_FUNC) (void *dbproc);

#ifndef __INCvxWorksh
/* VxWorks already defines STATUS and BOOL. Compiler gets mad if you 
** redefine them. */
/* __INCvxWorksh will get #defined by std. include files included from tds.h
*/
#ifdef STATUS
/* On DU4.0d we get a conflicting STATUS definition from arpa/nameser.h
   when _REENTRANT is defined.
*/
#undef STATUS
#endif
typedef int STATUS;
#if !defined(_WINDEF_) && !defined(_WINDEF_H) && !defined(DOS32X)
typedef int BOOL;
#endif
#endif

#ifndef _tds_h_
/* copied from tds.h */
/* TODO find a best way... */
enum
{
	SYBCHAR = 47,		/* 0x2F */
#define SYBCHAR	SYBCHAR
	SYBVARCHAR = 39,	/* 0x27 */
#define SYBVARCHAR	SYBVARCHAR
	SYBINTN = 38,		/* 0x26 */
#define SYBINTN	SYBINTN
	SYBINT1 = 48,		/* 0x30 */
#define SYBINT1	SYBINT1
	SYBINT2 = 52,		/* 0x34 */
#define SYBINT2	SYBINT2
	SYBINT4 = 56,		/* 0x38 */
#define SYBINT4	SYBINT4
	SYBINT8 = 127,		/* 0x7F */
#define SYBINT8	SYBINT8
	SYBFLT8 = 62,		/* 0x3E */
#define SYBFLT8	SYBFLT8
	SYBDATETIME = 61,	/* 0x3D */
#define SYBDATETIME	SYBDATETIME
	SYBBIT = 50,		/* 0x32 */
#define SYBBIT	SYBBIT
	SYBBITN = 104,		/* 0x68 */
#define SYBBITN	SYBBITN
	SYBTEXT = 35,		/* 0x23 */
#define SYBTEXT	SYBTEXT
	SYBNTEXT = 99,		/* 0x63 */
#define SYBNTEXT	SYBNTEXT
	SYBIMAGE = 34,		/* 0x22 */
#define SYBIMAGE	SYBIMAGE
	SYBMONEY4 = 122,	/* 0x7A */
#define SYBMONEY4	SYBMONEY4
	SYBMONEY = 60,		/* 0x3C */
#define SYBMONEY	SYBMONEY
	SYBDATETIME4 = 58,	/* 0x3A */
#define SYBDATETIME4	SYBDATETIME4
	SYBREAL = 59,		/* 0x3B */
#define SYBREAL	SYBREAL
	SYBBINARY = 45,		/* 0x2D */
#define SYBBINARY	SYBBINARY
	SYBVOID = 31,		/* 0x1F */
#define SYBVOID	SYBVOID
	SYBVARBINARY = 37,	/* 0x25 */
#define SYBVARBINARY	SYBVARBINARY
	SYBNUMERIC = 108,	/* 0x6C */
#define SYBNUMERIC	SYBNUMERIC
	SYBDECIMAL = 106,	/* 0x6A */
#define SYBDECIMAL	SYBDECIMAL
	SYBFLTN = 109,		/* 0x6D */
#define SYBFLTN	SYBFLTN
	SYBMONEYN = 110,	/* 0x6E */
#define SYBMONEYN	SYBMONEYN
	SYBDATETIMN = 111,	/* 0x6F */
#define SYBDATETIMN	SYBDATETIMN
	SYBNVARCHAR = 103	/* 0x67 */
#define SYBNVARCHAR	SYBNVARCHAR
};

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

#endif

typedef unsigned char DBBOOL;
typedef char DBCHAR;
typedef unsigned char DBBIT;
typedef unsigned char DBTINYINT;
typedef tds_sysdep_int16_type DBSMALLINT;
typedef tds_sysdep_int32_type DBINT;
typedef tds_sysdep_int64_type DBBIGINT;
typedef unsigned char DBBINARY;
typedef tds_sysdep_real32_type DBREAL;
typedef tds_sysdep_real64_type DBFLT8;
typedef unsigned tds_sysdep_int16_type DBUSMALLINT;

typedef struct 
{
	DBSMALLINT len;
	char  str[256];
} DBVARYCHAR;

typedef struct
{
	DBSMALLINT len;
	unsigned char  array[256];
} DBVARYBIN;

typedef struct
{
	unsigned char precision;
	unsigned char scale;
	unsigned char array[33];
} DBNUMERIC;

typedef DBNUMERIC DBDECIMAL;

typedef struct
{
	DBINT mnyhigh;
	unsigned tds_sysdep_int32_type mnylow;
} DBMONEY;

typedef struct
{
	DBINT mny4;
} DBMONEY4;

typedef struct
{
	DBINT dtdays;
	DBINT dttime;
} DBDATETIME;

typedef struct
{
	DBUSMALLINT days;	/* days since Jan-1-1900 */
	DBUSMALLINT minutes;	/* minutes since midnight */
} DBDATETIME4;

#ifdef MSDBLIB
# define SQLCHAR SYBCHAR
	typedef struct
	{
		DBUSMALLINT numdays;
		DBUSMALLINT nummins;
	} DBDATETIM4;
#endif

typedef struct tds_dblib_loginrec LOGINREC;

#ifndef DOS32X
typedef unsigned char BYTE;
#endif

typedef struct dbtypeinfo
{
	DBINT precision;
	DBINT scale;
} DBTYPEINFO;

struct dbstring
{
	BYTE *strtext;
	DBINT strtotlen;
	struct dbstring *strnext;
};
typedef struct dbstring DBSTRING;

/* Used by dbcolinfo */
enum
{ MAXCOLNAMELEN = 512 }; /* duplicates TDS_SYSNAME_SIZE */
typedef enum { CI_REGULAR=1, CI_ALTERNATE=2, CI_CURSOR=3 } CI_TYPE;

typedef struct 
{ 
    DBINT SizeOfStruct; 
    DBCHAR  Name[MAXCOLNAMELEN+2]; 
    DBCHAR  ActualName[MAXCOLNAMELEN+2]; 
    DBCHAR  TableName[MAXCOLNAMELEN+2]; 
    SHORT Type; 
    DBINT UserType; 
    DBINT MaxLength; 
    BYTE  Precision; 
    BYTE  Scale; 
    BOOL  VarLength; 
    BYTE  Null; 
    BYTE  CaseSensitive; 
    BYTE  Updatable; 
    BOOL  Identity; 

} DBCOL; 
/* end dbcolinfo stuff */



/* a large list of options, DBTEXTSIZE is needed by sybtcl */
#define DBPARSEONLY      0
#define DBESTIMATE       1
#define DBSHOWPLAN       2
#define DBNOEXEC         3
#define DBARITHIGNORE    4
#define DBNOCOUNT        5
#define DBARITHABORT     6
#define DBTEXTLIMIT      7
#define DBBROWSE         8
#define DBOFFSET         9
#define DBSTAT          10
#define DBERRLVL        11
#define DBCONFIRM       12
#define DBSTORPROCID    13
#define DBBUFFER        14
#define DBNOAUTOFREE    15
#define DBROWCOUNT      16
#define DBTEXTSIZE      17
#define DBNATLANG       18
#define DBDATEFORMAT    19
#define DBPRPAD         20
#define DBPRCOLSEP      21
#define DBPRLINELEN     22
#define DBPRLINESEP     23
#define DBLFCONVERT     24
#define DBDATEFIRST	25
#define DBCHAINXACTS	26
#define DBFIPSFLAG	27
#define DBISOLATION	28
#define DBAUTH		29
#define DBIDENTITY	30
#define DBNOIDCOL       31
#define DBDATESHORT	32
#define DBCLIENTCURSORS	33
#define DBSETTIME 	34
#define DBQUOTEDIDENT 	35

#define DBNUMOPTIONS  36

#define DBPADOFF       0
#define DBPADON        1

#define OFF            0
#define ON             1

#define NOSUCHOPTION   2

#define MAXOPTTEXT    32

typedef struct tds_dblib_dbprocess DBPROCESS;

/*
 * Sybase & Microsoft use different names for the dbdaterec members. 
 * Keep these two structures physically identical in memory.  
 * dbdatecrack() casts one to the other for ease of implementation. 
 *
 * Giving credit where credit is due, we can acknowledge that
 * Microsoft chose the better names here, hands down.  ("datedmonth"?!)
 */
struct tds_microsoft_dbdaterec
{
	DBINT year;		/* 1753 - 9999  	   */
	DBINT quarter;		/* 1 - 4 		   */
	DBINT month;		/* 1 - 12 		   */
	DBINT day;		/* 1 - 31 		   */
	DBINT dayofyear;	/* 1 - 366 		   */
	DBINT week;            	/* 1 - 54 (for leap years) */
	DBINT weekday;		/* 1 - 7 (Mon. - Sun.)     */
	DBINT hour;		/* 0 - 23 		   */
	DBINT minute;		/* 0 - 59 		   */
	DBINT second;		/* 0 - 59 		   */
	DBINT millisecond;	/* 0 - 999 		   */
	DBINT tzone;		/* 0 - 127  (Sybase only)  */	
};					

struct tds_sybase_dbdaterec
{
	DBINT dateyear;		/* 1900 and counting	  */ 
	DBINT quarter;		/* 0 - 3 (Microsoft only) */
	DBINT datemonth;	/* 0 - 11   	     	  */
	DBINT datedmonth;	/* 1 - 31   	     	  */
	DBINT datedyear;	/* 1 - 366  	     	  */
	DBINT week;            	/* 1 - 54 (Microsoft only) */
	DBINT datedweek;	/* 0 - 6    	     	  */
	DBINT datehour; 	/* 0 - 23   	     	  */
	DBINT dateminute;	/* 0 - 59   	     	  */
	DBINT datesecond;	/* 0 - 59   	     	  */
	DBINT datemsecond;	/* 0 - 997  	     	  */
	DBINT datetzone;	/* 0 - 127  	     	  */
};

#ifdef MSDBLIB
typedef struct tds_microsoft_dbdaterec DBDATEREC;
#else
typedef struct tds_sybase_dbdaterec DBDATEREC;
#endif

typedef int (*EHANDLEFUNC) (DBPROCESS * dbproc, int severity, int dberr, int oserr, char *dberrstr, char *oserrstr);

typedef int (*MHANDLEFUNC) (DBPROCESS * dbproc, DBINT msgno, int msgstate, int severity, char *msgtext, char *srvname,
			    char *proc, int line);

/* dbpoll() result codes, sybtcl needs DBRESULT */
#define DBRESULT       1
#define DBNOTIFICATION 2
#define DBTIMEOUT      3
#define DBINTERRUPT    4

/* more sybtcl needs: */
#define DBTXTSLEN    8

/* bind types */
#define CHARBIND          0
#define STRINGBIND        1
#define NTBSTRINGBIND     2
#define VARYCHARBIND      3
#define VARYBINBIND	  4
#define TINYBIND          6
#define SMALLBIND         7
#define INTBIND           8
#define FLT8BIND          9
#define REALBIND          10
#define DATETIMEBIND      11
#define SMALLDATETIMEBIND 12
#define MONEYBIND         13
#define SMALLMONEYBIND    14
#define BINARYBIND        15
#define BITBIND           16
#define NUMERICBIND       17
#define DECIMALBIND       18
#define BIGINTBIND        30
#define MAXBINDTYPES      31	/* keep last */

#define DBPRCOLSEP  21
#define DBPRLINELEN 22
#define DBRPCRETURN 1
#define DBRPCDEFAULT 2

#define REG_ROW         -1
#define MORE_ROWS       -1
#define NO_MORE_ROWS    -2
#define BUF_FULL        -3
#define NO_MORE_RESULTS 2
#define SUCCEED         1
#define FAIL            0

#define DB_IN  1
#define DB_OUT 2
#define DB_QUERYOUT 3

#define DBSINGLE 0
#define DBDOUBLE 1
#define DBBOTH   2

/* remote procedure call (rpc) options */
#define DBRPCRECOMPILE	TDS_STATIC_CAST(DBSMALLINT, 0x0001)
#define DBRPCRESET 	TDS_STATIC_CAST(DBSMALLINT, 0x0002)
#define DBRPCCURSOR 	TDS_STATIC_CAST(DBSMALLINT, 0x0008)

#if defined(DBLIB_UNIMPLEMENTED)
DBBOOL db12hour(DBPROCESS * dbprocess, const char language[]);

DBBOOL dbcolbrowse(DBPROCESS * dbprocess, int colnum);

RETCODE dbcursor(DBCURSOR * hc, DBINT optype, DBINT bufno, BYTE * table, BYTE * values);
RETCODE dbcursorbind(DBCURSOR * hc, int col, int vartype, DBINT varlen, DBINT * poutlen, BYTE * pvaraddr, DBTYPEINFO * typeinfo);
void dbcursorclose(DBCURSOR * hc);
RETCODE dbcursorcolinfo(DBCURSOR * hc, DBINT column, DBCHAR * colname, DBINT * coltype, DBINT * collen, DBINT * usertype);
RETCODE dbcursorfetch(DBCURSOR * hc, DBINT fetchtype, DBINT rownum);
RETCODE dbcursorinfo(DBCURSOR * hc, DBINT * ncols, DBINT * nrows);
DBCURSOR *dbcursoropen(DBPROCESS * dbprocess, BYTE * stmt, SHORT scollopt, SHORT concuropt, USHORT nrows, DBINT * pstatus);

int dbdate4cmp(DBPROCESS * dbprocess, DBDATETIME4 * d1, DBDATETIME4 * d2);
RETCODE dbdate4zero(DBPROCESS * dbprocess, DBDATETIME4 * d1);
RETCODE dbdatechar(DBPROCESS * dbprocess, char *buf, int datepart, int value);
int dbdatename(DBPROCESS * dbprocess, char *buf, int date, DBDATETIME * datetime);
char *dateorder(DBPROCESS * dbprocess, char *language);
DBINT dbdatepart(DBPROCESS * dbprocess, int datepart, DBDATETIME * datetime);
RETCODE dbdatezero(DBPROCESS * dbprocess, DBDATETIME * d1);
char *dbdayname(DBPROCESS * dbprocess, char *language, int daynum);

int dbgetoff(DBPROCESS * dbprocess, DBUSMALLINT offtype, int startfrom);

char *dbqual(DBPROCESS * dbprocess, int tabnum, char *tabname);
void dbfreequal(char *qualptr);

DBSORTORDER *dbloadsort(DBPROCESS * dbprocess);
RETCODE dbfreesort(DBPROCESS * dbprocess, DBSORTORDER * sortorder);

RETCODE dbload_xlate(DBPROCESS * dbprocess, char *srv_charset, char *clt_name, DBXLATE ** xlt_tosrv, DBXLATE ** xlt_todisp);

RETCODE dbmny4divide(DBPROCESS * dbproc, DBMONEY4 * m1, DBMONEY4 * m2, DBMONEY4 * quotient);
RETCODE dbmny4mul(DBPROCESS * dbproc, DBMONEY4 * m1, DBMONEY4 * m2, DBMONEY4 * prod);
RETCODE dbmnyadd(DBPROCESS * dbproc, DBMONEY * m1, DBMONEY * m2, DBMONEY * sum);
RETCODE dbmnydivide(DBPROCESS * dbproc, DBMONEY * m1, DBMONEY * m2, DBMONEY * quotient);
RETCODE dbmnydown(DBPROCESS * dbproc, DBMONEY * mnyptr, int divisor, int *remainder);
RETCODE dbmnyinit(DBPROCESS * dbproc, DBMONEY * mnyptr, int trim, DBBOOL * negative);
RETCODE dbmnyndigit(DBPROCESS * dbproc, DBMONEY * mnyptr, DBCHAR * value, DBBOOL * zero);
RETCODE dbmnymul(DBPROCESS * dbproc, DBMONEY * m1, DBMONEY * m2, DBMONEY * prod);
RETCODE dbmnydigit(DBPROCESS * dbprocess, DBMONEY * m1, DBCHAR * value, DBBOOL * zero);
RETCODE dbmnyscale(DBPROCESS * dbproc, DBMONEY * dest, int multiplier, int addend);


RETCODE dbnpcreate(DBPROCESS * dbprocess);
RETCODE dbnpdefine(DBPROCESS * dbprocess, DBCHAR * procedure_name, DBSMALLINT namelen);

int DBNUMORDERS(DBPROCESS * dbprocess);

RETCODE dbpoll(DBPROCESS * dbproc, long milliseconds, DBPROCESS ** ready_dbproc, int *return_reason);
int dbordercol(DBPROCESS * dbprocess, int order);

RETCODE dbregdrop(DBPROCESS * dbprocess, DBCHAR * procnm, DBSMALLINT namelen);
RETCODE dbregexec(DBPROCESS * dbproc, DBUSMALLINT options);
RETCODE dbreghandle(DBPROCESS * dbprocess, DBCHAR * procnm, DBSMALLINT namelen, INTFUNCPTR handler);
RETCODE dbreginit(DBPROCESS * dbproc, DBCHAR * procedure_name, DBSMALLINT namelen);
RETCODE dbreglist(DBPROCESS * dbproc);
RETCODE dbregnowatch(DBPROCESS * dbprocess, DBCHAR * procnm, DBSMALLINT namelen);
RETCODE dbregparam(DBPROCESS * dbproc, char *param_name, int type, DBINT datalen, BYTE * data);
RETCODE dbregwatch(DBPROCESS * dbprocess, DBCHAR * procnm, DBSMALLINT namelen, DBUSMALLINT options);
RETCODE dbregwatchlist(DBPROCESS * dbprocess);

void dbrpwclr(LOGINREC * login);
RETCODE dbrpwset(LOGINREC * login, char *srvname, char *password, int pwlen);

DBINT dbreadpage(DBPROCESS * dbprocess, char *p_dbname, DBINT pageno, BYTE * buf);
RETCODE dbwritepage(DBPROCESS * dbprocess, char *p_dbname, DBINT pageno, DBINT size, BYTE * buf);

RETCODE dbsetdeflang(char *language);

int dbstrcmp(DBPROCESS * dbprocess, char *s1, int l1, char *s2, int l2, DBSORTORDER * sort);
int dbstrsort(DBPROCESS * dbprocess, char *s1, int l1, char *s2, int l2, DBSORTORDER * sort);

DBBOOL dbtabbrowse(DBPROCESS * dbprocess, int tabnum);
int dbtabcount(DBPROCESS * dbprocess);
char *dbtabname(DBPROCESS * dbprocess, int tabnum);
char *dbtabsoruce(DBPROCESS * dbprocess, int colnum, int *tabnum);

RETCODE dbsetlshort(LOGINREC * login, int value, int which);

RETCODE dbsendpassthru(DBPROCESS * dbprocess, DBVOIDPTR bufp);
RETCODE dbrecvpassthru(DBPROCESS * dbprocess, DBVOIDPTR * bufp);

RETCODE dbgetloginfo(DBPROCESS * dbprocess, DBLOGINFO ** loginfo);
RETCODE dbsetloginfo(LOGINREC * loginrec, DBLOGINFO * loginfo);

int dbtsnewlen(DBPROCESS * dbprocess);
DBBINARY *dbtsnewval(DBPROCESS * dbprocess);
RETCODE dbtsput(DBPROCESS * dbprocess, DBBINARY * newts, int newtslen, int tabnum, char *tabname);

RETCODE dbfree_xlate(DBPROCESS * dbprocess, DBXLATE * xlt_tosrv, DBXLATE * clt_todisp);
int dbxlate(DBPROCESS * dbprocess, char *src, int srclen, char *dest, int destlen, DBXLATE * xlt, int *srcbytes_used,
	    DBBOOL srcend, int status);

RETCODE bcp_moretext(DBPROCESS * dbproc, DBINT size, BYTE * text);
RETCODE bcp_writefmt(DBPROCESS * dbproc, const char filename[]);

void build_xact_string(char *xact_name, char *service_name, DBINT commid, char *result);
RETCODE remove_xact(DBPROCESS * connect, DBINT commid, int n);
RETCODE abort_xact(DBPROCESS * connect, DBINT commid);
void close_commit(DBPROCESS * connect);
RETCODE commit_xact(DBPROCESS * connect, DBINT commid);
DBPROCESS *open_commit(LOGINREC * login, char *servername);
RETCODE scan_xact(DBPROCESS * connect, DBINT commid);
DBINT start_xact(DBPROCESS * connect, char *application_name, char *xact_name, int site_count);
DBINT stat_xact(DBPROCESS * connect, DBINT commid);

#endif /* define unimplemented */

BYTE *dbadata(DBPROCESS * dbproc, int computeid, int column);
DBINT dbadlen(DBPROCESS * dbproc, int computeid, int column);
RETCODE dbaltbind(DBPROCESS * dbprocess, int computeid, int column, int vartype, DBINT varlen, BYTE * varaddr);
RETCODE dbaltbind_ps(DBPROCESS * dbprocess, int computeid, int column, int vartype, DBINT varlen, BYTE * varaddr,
		     DBTYPEINFO * typeinfo);
int dbaltcolid(DBPROCESS * dbproc, int computeid, int column);
DBINT dbaltlen(DBPROCESS * dbproc, int computeid, int column);
int dbaltop(DBPROCESS * dbproc, int computeid, int column);
int dbalttype(DBPROCESS * dbproc, int computeid, int column);
DBINT dbaltutype(DBPROCESS * dbproc, int computeid, int column);
RETCODE dbanullbind(DBPROCESS * dbprocess, int computeid, int column, DBINT * indicator);
RETCODE dbbind(DBPROCESS * dbproc, int column, int vartype, DBINT varlen, BYTE * varaddr);
RETCODE dbbind_ps(DBPROCESS * dbprocess, int column, int vartype, DBINT varlen, BYTE * varaddr, DBTYPEINFO * typeinfo);
int dbbufsize(DBPROCESS * dbprocess);
BYTE *dbbylist(DBPROCESS * dbproc, int computeid, int *size);
RETCODE dbcancel(DBPROCESS * dbproc);
RETCODE dbcanquery(DBPROCESS * dbproc);
char *dbchange(DBPROCESS * dbprocess);
DBBOOL dbcharsetconv(DBPROCESS * dbprocess);
void dbclose(DBPROCESS * dbproc);
void dbclrbuf(DBPROCESS * dbproc, DBINT n);
RETCODE dbclropt(DBPROCESS * dbproc, int option, const char param[]);
RETCODE dbcmd(DBPROCESS * dbproc, const char cmdstring[]);
RETCODE dbcmdrow(DBPROCESS * dbproc);

#define DBCMDROW(x) dbcmdrow((x))
RETCODE dbtablecolinfo (DBPROCESS *dbproc, DBINT column, DBCOL *pdbcol );
RETCODE	dbcolinfo (DBPROCESS *dbproc, CI_TYPE type, DBINT column, DBINT computeid, DBCOL *pdbcol);
DBINT dbcollen(DBPROCESS * dbproc, int column);
char *dbcolname(DBPROCESS * dbproc, int column);
char *dbcolsource(DBPROCESS * dbproc, int colnum);
int dbcoltype(DBPROCESS * dbproc, int column);
DBTYPEINFO *dbcoltypeinfo(DBPROCESS * dbproc, int column);
DBINT dbcolutype(DBPROCESS * dbprocess, int column);
DBINT dbconvert(DBPROCESS * dbproc, int srctype, const BYTE * src, DBINT srclen, int desttype, BYTE * dest, DBINT destlen);
DBINT dbconvert_ps(DBPROCESS * dbprocess, int srctype, BYTE * src, DBINT srclen, int desttype, BYTE * dest, DBINT destlen,
		   DBTYPEINFO * typeinfo);
BOOL dbiscount(DBPROCESS * dbproc);
DBINT dbcount(DBPROCESS * dbproc);

#define DBCOUNT(x) dbcount((x))
int dbcurcmd(DBPROCESS * dbproc);

#define DBCURCMD(x) dbcurcmd((x))
DBINT dbcurrow(DBPROCESS * dbproc);

#define DBCURROW(x) dbcurrow((x))
BYTE *dbdata(DBPROCESS * dbproc, int column);
int dbdatecmp(DBPROCESS * dbproc, DBDATETIME * d1, DBDATETIME * d2);
RETCODE dbdatecrack(DBPROCESS * dbproc, DBDATEREC * di, DBDATETIME * dt);
DBINT dbdatlen(DBPROCESS * dbproc, int column);
DBBOOL dbdead(DBPROCESS * dbproc);

#define DBDEAD(x) dbdead((x))
EHANDLEFUNC dberrhandle(EHANDLEFUNC handler);
void dbexit(void);
RETCODE dbfcmd(DBPROCESS * dbproc, const char *fmt, ...);
DBINT dbfirstrow(DBPROCESS * dbproc);

#define DBFIRSTROW(x) dbfirstrow((x))
void dbfreebuf(DBPROCESS * dbproc);
char *dbgetchar(DBPROCESS * dbprocess, int n);
char *dbgetcharset(DBPROCESS * dbprocess);
int dbgetlusername(LOGINREC * login, BYTE * name_buffer, int buffer_len);
int dbgetmaxprocs(void);
char *dbgetnatlanf(DBPROCESS * dbprocess);
int dbgetpacket(DBPROCESS * dbproc);
RETCODE dbgetrow(DBPROCESS * dbproc, DBINT row);
int dbgettime(void);
#define DBGETTIME dbgettime
BYTE *dbgetuserdata(DBPROCESS * dbproc);
DBBOOL dbhasretstat(DBPROCESS * dbproc);
RETCODE dbinit(void);
int dbiordesc(DBPROCESS * dbproc);

#define DBIORDESC(x) dbiordesc((x))
int dbiowdesc(DBPROCESS * dbproc);

#define DBIOWDESC(x) dbiowdesc((x))
DBBOOL dbisavail(DBPROCESS * dbprocess);

#define DBISAVAIL(x) dbisavail((x))
DBBOOL dbisopt(DBPROCESS * dbproc, int option, const char param[]);
DBINT dblastrow(DBPROCESS * dbproc);

#define DBLASTROW(x) dblastrow((x))
LOGINREC *dblogin(void);
void dbloginfree(LOGINREC * login);

int dbmny4cmp(DBPROCESS * dbproc, DBMONEY4 * m1, DBMONEY4 * m2);
int dbmnycmp(DBPROCESS * dbproc, DBMONEY * m1, DBMONEY * m2);

RETCODE dbmny4add(DBPROCESS * dbproc, DBMONEY4 * m1, DBMONEY4 * m2, DBMONEY4 * sum);
RETCODE dbmnydec(DBPROCESS * dbproc, DBMONEY * mnyptr);
RETCODE dbmnyinc(DBPROCESS * dbproc, DBMONEY * mnyptr);
RETCODE dbmnymaxpos(DBPROCESS * dbproc, DBMONEY * dest);
RETCODE dbmnymaxneg(DBPROCESS * dbproc, DBMONEY * dest);
RETCODE dbmny4minus(DBPROCESS * dbproc, DBMONEY4 * src, DBMONEY4 * dest);
RETCODE dbmnyminus(DBPROCESS * dbproc, DBMONEY * src, DBMONEY * dest);
RETCODE dbmny4sub(DBPROCESS * dbproc, DBMONEY4 * m1, DBMONEY4 * m2, DBMONEY4 * diff);
RETCODE dbmnysub(DBPROCESS * dbproc, DBMONEY * m1, DBMONEY * m2, DBMONEY * diff);

RETCODE dbmny4copy(DBPROCESS * dbprocess, DBMONEY4 * m1, DBMONEY4 * m2);
RETCODE dbmnycopy(DBPROCESS * dbproc, DBMONEY * src, DBMONEY * dest);

RETCODE dbmny4zero(DBPROCESS * dbproc, DBMONEY4 * dest);
RETCODE dbmnyzero(DBPROCESS * dbproc, DBMONEY * dest);

const char *dbmonthname(DBPROCESS * dbproc, char *language, int monthnum, DBBOOL shortform);
RETCODE dbmorecmds(DBPROCESS * dbproc);

#define DBMORECMDS(x) dbmorecmds((x))
RETCODE dbmoretext(DBPROCESS * dbproc, DBINT size, const BYTE text[]);
MHANDLEFUNC dbmsghandle(MHANDLEFUNC handler);
char *dbname(DBPROCESS * dbproc);
STATUS dbnextrow(DBPROCESS * dbproc);
RETCODE dbnullbind(DBPROCESS * dbproc, int column, DBINT * indicator);
int dbnumalts(DBPROCESS * dbproc, int computeid);
int dbnumcols(DBPROCESS * dbproc);
int dbnumcompute(DBPROCESS * dbprocess);
int dbnumrets(DBPROCESS * dbproc);
DBPROCESS *tdsdbopen(LOGINREC * login, const char *server, int msdblib);
DBPROCESS *dbopen(LOGINREC * login, const char *server);

/* pivot functions */
struct col_t;
void dbpivot_count (struct col_t *output, const struct col_t *input);
void dbpivot_sum (struct col_t *output, const struct col_t *input);
void dbpivot_min (struct col_t *output, const struct col_t *input);
void dbpivot_max (struct col_t *output, const struct col_t *input);

struct pivot_t;
typedef void (*DBPIVOT_FUNC)(struct col_t *output, const struct col_t *input);
struct pivot_t * dbrows_pivoted(DBPROCESS *dbproc);
STATUS dbnextrow_pivoted(DBPROCESS *dbproc, struct pivot_t *pp);
RETCODE dbpivot(DBPROCESS *dbproc, int nkeys, int *keys, int ncols, int *cols, DBPIVOT_FUNC func, int val);

DBPIVOT_FUNC dbpivot_lookup_name( const char name[] );

#ifdef MSDBLIB
#define   dbopen(x,y) tdsdbopen((x),(y), 1)
#else
#define   dbopen(x,y) tdsdbopen((x),(y), 0)
#endif

/* fix PHP problem */
#ifdef PHP_SYBASE_DBOPEN
#undef PHP_SYBASE_DBOPEN
#define PHP_SYBASE_DBOPEN dbopen
#endif

void dbprhead(DBPROCESS * dbproc);
RETCODE dbprrow(DBPROCESS * dbproc);
const char *dbprtype(int token);
DBBOOL DRBUF(DBPROCESS * dbprocess);
STATUS dbreadtext(DBPROCESS * dbproc, void *buf, DBINT bufsize);
void dbrecftos(const char filename[]);
RETCODE dbresults(DBPROCESS * dbproc);
RETCODE dbresults_r(DBPROCESS * dbproc, int recursive);
BYTE *dbretdata(DBPROCESS * dbproc, int retnum);
int dbretlen(DBPROCESS * dbproc, int retnum);
char *dbretname(DBPROCESS * dbproc, int retnum);
DBINT dbretstatus(DBPROCESS * dbproc);
int dbrettype(DBPROCESS * dbproc, int retnum);
RETCODE dbrows(DBPROCESS * dbproc);

#define DBROWS(x) dbrows((x))
STATUS dbrowtype(DBPROCESS * dbprocess);

#define DBROWTYPE(x) dbrowtype((x))
RETCODE dbrpcinit(DBPROCESS * dbproc, const char rpcname[], DBSMALLINT options);
RETCODE dbrpcparam(DBPROCESS * dbproc, const char paramname[], BYTE status, int type, DBINT maxlen, DBINT datalen, BYTE * value);
RETCODE dbrpcsend(DBPROCESS * dbproc);
RETCODE dbsafestr(DBPROCESS * dbproc, const char *src, DBINT srclen, char *dest, DBINT destlen, int quotetype);
RETCODE *dbsechandle(DBINT type, INTFUNCPTR handler);
char *dbservcharset(DBPROCESS * dbprocess);
void dbsetavail(DBPROCESS * dbprocess);
void dbsetbusy(DBPROCESS * dbprocess, DB_DBBUSY_FUNC busyfunc);
RETCODE dbsetdefcharset(char *charset);
void dbsetidle(DBPROCESS * dbprocess, DB_DBIDLE_FUNC idlefunc);
void dbsetifile(char *filename);
void dbsetinterrupt(DBPROCESS * dbproc, DB_DBCHKINTR_FUNC chkintr, DB_DBHNDLINTR_FUNC hndlintr);
RETCODE dbsetlogintime(int seconds);
RETCODE dbsetmaxprocs(int maxprocs);
RETCODE dbsetnull(DBPROCESS * dbprocess, int bindtype, int bindlen, BYTE * bindval);
RETCODE dbsetopt(DBPROCESS * dbproc, int option, const char *char_param, int int_param);
STATUS dbsetrow(DBPROCESS * dbprocess, DBINT row);
RETCODE dbsettime(int seconds);
void dbsetuserdata(DBPROCESS * dbproc, BYTE * ptr);
RETCODE dbsetversion(DBINT version);

int dbspid(DBPROCESS * dbproc);
RETCODE dbspr1row(DBPROCESS * dbproc, char *buffer, DBINT buf_len);
DBINT dbspr1rowlen(DBPROCESS * dbproc);
RETCODE dbsprhead(DBPROCESS * dbproc, char *buffer, DBINT buf_len);
RETCODE dbsprline(DBPROCESS * dbproc, char *buffer, DBINT buf_len, DBCHAR line_char);
RETCODE dbsqlexec(DBPROCESS * dbproc);
RETCODE dbsqlok(DBPROCESS * dbproc);
RETCODE dbsqlsend(DBPROCESS * dbproc);
int dbstrbuild(DBPROCESS * dbproc, char *charbuf, int bufsize, char *text, char *formats, ...);
RETCODE dbstrcpy(DBPROCESS * dbproc, int start, int numbytes, char *dest);
int dbstrlen(DBPROCESS * dbproc);
DBINT dbvarylen(DBPROCESS * dbproc, int column);

#define SYBEVERDOWN	  100	/* TDS version downgraded . */
#define SYBEICONVIU	 2400	/* Some character(s) could not be converted into client's character set. */
#define SYBEICONVAVAIL	 2401	/* Character set conversion is not available between client character set '%.*s' and server character set '%.*s'.*/
#define SYBEICONVO	 2402	/* Error converting characters into server's character set. Some character(s) could not be converted.*/
#define SYBEICONVI	 2403	/* Some character(s) could not be converted into client's character set.  Unconverted bytes were changed to question marks ('?').*/
#define SYBEICONV2BIG	 2404	/* Buffer overflow converting characters from client into server's character set.*/
				/* cf. doc/dblib_errors.txt for more iconv error values. */
				/* Reserve a few slots for other iconv-related issues. */
#define SYBETDSVER	 2410 	/* Cannot bcp with TDSVER < 5.0 */
#define SYBEPORT	 2500	/* Both port and instance specified */
#define SYBESYNC        20001	/* Read attempted while out of synchronization with SQL Server. */
#define SYBEFCON        20002	/* SQL Server connection failed. */
#define SYBETIME        20003	/* SQL Server connection timed out. */
#define SYBEREAD        20004	/* Read from SQL Server failed. */
#define SYBEBUFL        20005	/* DB-LIBRARY internal error - send buffer length corrupted. */
#define SYBEWRIT        20006	/* Write to SQL Server failed. */
#define SYBEVMS         20007	/* Sendflush: VMS I/O error. */
#define SYBESOCK        20008	/* Unable to open socket */
#define SYBECONN        20009	/* Unable to connect socket -- SQL Server is unavailable or does not exist. */
#define SYBEMEM         20010	/* Unable to allocate sufficient memory */
#define SYBEDBPS        20011	/* Maximum number of DBPROCESSes already allocated. */
#define SYBEINTF        20012	/* Server name not found in interface file */
#define SYBEUHST        20013	/* Unknown host machine name */
#define SYBEPWD         20014	/* Incorrect password. */
#define SYBEOPIN        20015	/* Could not open interface file. */
#define SYBEINLN        20016	/* Interface file: unexpected end-of-line. */
#define SYBESEOF        20017	/* Unexpected EOF from SQL Server. */
#define SYBESMSG        20018	/* General SQL Server error: Check messages from the SQL Server. */
#define SYBERPND        20019	/* Attempt to initiate a new SQL Server operation with results pending. */
#define SYBEBTOK        20020	/* Bad token from SQL Server: Data-stream processing out of sync. */
#define SYBEITIM        20021	/* Illegal timeout value specified. */
#define SYBEOOB         20022	/* Error in sending out-of-band data to SQL Server. */
#define SYBEBTYP        20023	/* Unknown bind type passed to DB-LIBRARY function. */
#define SYBEBNCR        20024	/* Attempt to bind user variable to a non-existent compute row. */
#define SYBEIICL        20025	/* Illegal integer column length returned by SQL Server. Legal integer lengths are 1, 2, and 4 bytes. */
#define SYBECNOR        20026	/* Column number out of range. */
#define SYBENPRM        20027	/* NULL parameter not allowed for this dboption. */
#define SYBEUVDT        20028	/* Unknown variable-length datatype encountered. */
#define SYBEUFDT        20029	/* Unknown fixed-length datatype encountered. */
#define SYBEWAID        20030	/* DB-LIBRARY internal error: ALTFMT following ALTNAME has wrong id. */
#define SYBECDNS        20031	/* Datastream indicates that a compute column is derived from a non-existent select-list member. */
#define SYBEABNC        20032	/* Attempt to bind to a non-existent column. */
#define SYBEABMT        20033	/* User attempted a dbbind() with mismatched column and variable types. */
#define SYBEABNP        20034	/* Attempt to bind using NULL pointers. */
#define SYBEAAMT        20035	/* User attempted a dbaltbind() with mismatched column and variable types. */
#define SYBENXID        20036	/* The Server did not grant us a distributed-transaction ID. */
#define SYBERXID        20037	/* The Server did not recognize our distributed-transaction ID. */
#define SYBEICN         20038	/* Invalid computeid or compute column number. */
#define SYBENMOB        20039	/* No such member of 'order by' clause. */
#define SYBEAPUT        20040	/* Attempt to print unknown token. */
#define SYBEASNL        20041	/* Attempt to set fields in a null loginrec. */
#define SYBENTLL        20042	/* Name too long for loginrec field. */
#define SYBEASUL        20043	/* Attempt to set unknown loginrec field. */
#define SYBERDNR        20044	/* Attempt to retrieve data from a non-existent row. */
#define SYBENSIP        20045	/* Negative starting index passed to dbstrcpy(). */
#define SYBEABNV        20046	/* Attempt to bind to a NULL program variable. */
#define SYBEDDNE        20047	/* DBPROCESS is dead or not enabled. */
#define SYBECUFL        20048	/* Data-conversion resulted in underflow. */
#define SYBECOFL        20049	/* Data-conversion resulted in overflow. */
#define SYBECSYN        20050	/* Attempt to convert data stopped by syntax error in source field. */
#define SYBECLPR        20051	/* Data-conversion resulted in loss of precision. */
#define SYBECNOV        20052	/* Attempt to set variable to NULL resulted in overflow. */
#define SYBERDCN        20053	/* Requested data-conversion does not exist. */
#define SYBESFOV        20054	/* dbsafestr() overflowed its destination buffer. */
#define SYBEUNT         20055	/* Unknown network type found in interface file. */
#define SYBECLOS        20056	/* Error in closing network connection. */
#define SYBEUAVE        20057	/* Unable to allocate VMS event flag. */
#define SYBEUSCT        20058	/* Unable to set communications timer. */
#define SYBEEQVA        20059	/* Error in queueing VMS AST routine. */
#define SYBEUDTY        20060	/* Unknown datatype encountered. */
#define SYBETSIT        20061	/* Attempt to call dbtsput() with an invalid timestamp. */
#define SYBEAUTN        20062	/* Attempt to update the timestamp of a table which has no timestamp column. */
#define SYBEBDIO        20063	/* Bad bulk-copy direction.  Must be either IN or OUT. */
#define SYBEBCNT        20064	/* Attempt to use Bulk Copy with a non-existent Server table. */
#define SYBEIFNB        20065	/* Illegal field number passed to bcp_control(). */
#define SYBETTS         20066	/* The table which bulk-copy is attempting to copy to a host-file is shorter than the number of rows which bulk-copy was instructed to skip. */
#define SYBEKBCO        20067	/* 1000 rows successfully bulk-copied to host-file. */
#define SYBEBBCI        20068	/* Batch successfully bulk-copied to SQL Server. */
#define SYBEKBCI        20069	/* Bcp: 1000 rows sent to SQL Server. */
#define SYBEBCRE        20070	/* I/O error while reading bcp data-file. */
#define SYBETPTN        20071	/* Syntax error: only two periods are permitted in table names. */
#define SYBEBCWE        20072	/* I/O error while writing bcp data-file. */
#define SYBEBCNN        20073	/* Attempt to bulk-copy a NULL value into Server column %d,  which does not accept NULL values. */
#define SYBEBCOR        20074	/* Attempt to bulk-copy an oversized row to the SQL Server. */
#define SYBEBCIS        20075	/* Attempt to bulk-copy an illegally-sized column value to the SQL Server. */
#define SYBEBCPI        20076	/* bcp_init() must be called before any other bcp routines. */
#define SYBEBCPN        20077	/* bcp_bind(), bcp_collen(), bcp_colptr(), bcp_moretext() and bcp_sendrow() may be used only after bcp_init() has been called with the copy direction set to DB_IN. */
#define SYBEBCPB        20078	/* bcp_bind(), bcp_moretext() and bcp_sendrow() may NOT be used after bcp_init() has been passed a non-NULL input file name. */
#define SYBEVDPT        20079	/* For bulk copy, all variable-length data must have either a length-prefix or a terminator specified. */
#define SYBEBIVI        20080	/* bcp_columns(), bcp_colfmt() and bcp_colfmt_ps() may be used only after bcp_init() has been passed a valid input file. */
#define SYBEBCBC        20081	/* bcp_columns() must be called before bcp_colfmt() and bcp_colfmt_ps(). */
#define SYBEBCFO        20082	/* Bcp host-files must contain at least one column. */
#define SYBEBCVH        20083	/* bcp_exec() may be called only after bcp_init() has been passed a valid host file. */
#define SYBEBCUO        20084	/* Bcp: Unable to open host data-file. */
#define SYBEBCUC        20085	/* Bcp: Unable to close host data-file. */
#define SYBEBUOE        20086	/* Bcp: Unable to open error-file. */
#define SYBEBUCE        20087	/* Bcp: Unable to close error-file. */
#define SYBEBWEF        20088	/* I/O error while writing bcp error-file. */
#define SYBEASTF        20089	/* VMS: Unable to setmode for control_c ast. */
#define SYBEUACS        20090	/* VMS: Unable to assign channel to sys$command. */
#define SYBEASEC        20091	/* Attempt to send an empty command buffer to the SQL Server. */
#define SYBETMTD        20092	/* Attempt to send too much TEXT data via the dbmoretext() call. */
#define SYBENTTN        20093	/* Attempt to use dbtxtsput() to put a new text-timestamp into a non-existent data row. */
#define SYBEDNTI        20094	/* Attempt to use dbtxtsput() to put a new text-timestamp into a column whose datatype is neither SYBTEXT nor SYBIMAGE. */
#define SYBEBTMT        20095	/* Attempt to send too much TEXT data via the bcp_moretext() call. */
#define SYBEORPF        20096	/* Attempt to set remote password would overflow the login-record's remote-password field. */
#define SYBEUVBF        20097	/* Attempt to read an unknown version of BCP format-file. */
#define SYBEBUOF        20098	/* Bcp: Unable to open format-file. */
#define SYBEBUCF        20099	/* Bcp: Unable to close format-file. */
#define SYBEBRFF        20100	/* I/O error while reading bcp format-file. */
#define SYBEBWFF        20101	/* I/O error while writing bcp format-file. */
#define SYBEBUDF        20102	/* Bcp: Unrecognized datatype found in format-file. */
#define SYBEBIHC        20103	/* Incorrect host-column number found in bcp format-file. */
#define SYBEBEOF        20104	/* Unexpected EOF encountered in BCP data-file. */
#define SYBEBCNL        20105	/* Negative length-prefix found in BCP data-file. */
#define SYBEBCSI        20106	/* Host-file columns may be skipped only when copying INto the Server. */
#define SYBEBCIT        20107	/* It's illegal to use BCP terminators with program variables other than SYBCHAR, SYBBINARY, SYBTEXT, or SYBIMAGE. */
#define SYBEBCSA        20108	/* The BCP hostfile '%s' contains only %ld rows. Skipping all of these rows is not allowed. */
#define SYBENULL        20109	/* NULL DBPROCESS pointer passed to DB-Library. */
#define SYBEUNAM        20110	/* Unable to get current username from operating system. */
#define SYBEBCRO        20111	/* The BCP hostfile '%s' contains only %ld rows. It was impossible to read the requested %ld rows. */
#define SYBEMPLL        20112	/* Attempt to set maximum number of DBPROCESSes lower than 1. */
#define SYBERPIL        20113	/* It is illegal to pass -1 to dbrpcparam() for the datalen of parameters which are of type SYBCHAR, SYBVARCHAR, SYBBINARY, or SYBVARBINARY. */
#define SYBERPUL        20114	/* When passing a SYBINTN, SYBDATETIMN, SYBMONEYN, or SYBFLTN parameter via dbrpcparam(), it's necessary to specify the parameter's maximum or actual length, so that DB-Library can recognize it as a SYBINT1, SYBINT2, SYBINT4, SYBMONEY, or SYBMONEY4, etc. */
#define SYBEUNOP        20115	/* Unknown option passed to dbsetopt(). */
#define SYBECRNC        20116	/* The current row is not a result of compute clause %d, so it is illegal to attempt to extract that data from this row. */
#define SYBERTCC        20117	/* dbreadtext() may not be used to receive the results of a query which contains a COMPUTE clause. */
#define SYBERTSC        20118	/* dbreadtext() may only be used to receive the results of a query which contains a single result column. */
#define SYBEUCRR        20119	/* Internal software error: Unknown connection result reported by                                                 * dbpasswd(). */
#define SYBERPNA        20120	/* The RPC facility is available only when using a SQL Server whose version number is 4.0 or greater. */
#define SYBEOPNA        20121	/* The text/image facility is available only when using a SQL Server whose version number is 4.0 or greater. */
#define SYBEFGTL        20122	/* Bcp: Row number of the first row to be copied cannot be greater than the row number for the last row to be copied.  */
#define SYBECWLL        20123	/* Attempt to set column width less than 1.  */
#define SYBEUFDS        20124	/* Unrecognized format encountered in dbstrbuild(). */
#define SYBEUCPT        20125	/* Unrecognized custom-format parameter-type encountered in dbstrbuild(). */
#define SYBETMCF        20126	/* Attempt to install too many custom formats via dbfmtinstall(). */
#define SYBEAICF        20127	/* Error in attempting to install custom format. */
#define SYBEADST        20128	/* Error in attempting to determine the size of a pair of translation tables. */
#define SYBEALTT        20129	/* Error in attempting to load a pair of translation tables. */
#define SYBEAPCT        20130	/* Error in attempting to perform a character-set translation. */
#define SYBEXOCI        20131	/* A character-set translation overflowed its destination buffer while using bcp to copy data from a host-file to the SQL Server. */
#define SYBEFSHD        20132	/* Error in attempting to find the Sybase home directory. */
#define SYBEAOLF        20133	/* Error in attempting to open a localization file. */
#define SYBEARDI        20134	/* Error in attempting to read datetime information from a localization file. */
#define SYBEURCI        20135	/* Unable to read copyright information from the dblib localization file. */
#define SYBEARDL        20136	/* Error in attempting to read the dblib.loc localization file. */
#define SYBEURMI        20137	/* Unable to read money-format information from the dblib localization file. */
#define SYBEUREM        20138	/* Unable to read error mnemonic from the dblib localization file. */
#define SYBEURES        20139	/* Unable to read error string from the dblib localization file. */
#define SYBEUREI        20140	/* Unable to read error information from the dblib localization file. */
#define SYBEOREN        20141	/* Warning: an out-of-range error-number was encountered in dblib.loc. The maximum permissible error-number is defined as DBERRCOUNT in sybdb.h. */
#define SYBEISOI        20142	/* Invalid sort-order information found. */
#define SYBEIDCL        20143	/* Illegal datetime column length returned by DataServer. Legal datetime lengths are 4 and 8 bytes. */
#define SYBEIMCL        20144	/* Illegal money column length returned by DataServer. Legal money lengths are 4 and 8 bytes. */
#define SYBEIFCL        20145	/* Illegal floating-point column length returned by DataServer. Legal floating-point lengths are 4 and 8 bytes. */
#define SYBEUTDS        20146	/* Unrecognized TDS version received from SQL Server. */
#define SYBEBUFF        20147	/* Bcp: Unable to create format-file. */
#define SYBEACNV        20148	/* Attemp to do conversion with NULL destination variable. */
#define SYBEDPOR        20149	/* Out-of-range datepart constant. */
#define SYBENDC         20150	/* Cannot have negative component in date in numeric form. */
#define SYBEMVOR        20151	/* Month values must be between 1 and 12. */
#define SYBEDVOR        20152	/* Day values must be between 1 and 7. */
#define SYBENBVP        20153	/* Cannot pass dbsetnull() a NULL bindval pointer. */
#define SYBESPID        20154	/* Called dbspid() with a NULL dbproc. */
#define SYBENDTP        20155	/* Called dbdatecrack() with a NULL datetime  parameter. */
#define SYBEXTN         20156	/* The xlt_todisp and xlt_tosrv parameters to dbfree_xlate() were NULL. */
#define SYBEXTDN        20157	/* Warning:  the xlt_todisp parameter to dbfree_xlate() was NULL.  The space associated with the xlt_tosrv parameter has been freed. */
#define SYBEXTSN        20158	/* Warning:  the xlt_tosrv parameter to dbfree_xlate() was NULL.  The space associated with the xlt_todisp parameter has been freed. */
#define SYBENUM         20159	/* Incorrect number of arguments given  to DB-Library.  */
#define SYBETYPE        20160	/* Invalid argument type given to DB-Library. */
#define SYBEGENOS       20161	/* General Operating System Error. */
#define SYBEPAGE        20162	/* wrong resource type or length given for  dbpage() operation.  */
#define SYBEOPTNO       20163	/* Option is not allowed or is unreconized */
#define SYBEETD         20164	/* Failure to send the expected amount of  TEXT or IMAGE data via dbmoretext(). */
#define SYBERTYPE       20165	/* Invalid resource type given to DB-Library. */
#define SYBERFILE       20166	/* "Can not open resource file." */
#define SYBEFMODE       20167	/* Read/Write/Append mode denied on file. */
#define SYBESLCT        20168	/* Could not select or copy field specified */
#define SYBEZTXT        20169	/* Attempt to send zero length TEXT or  IMAGE to dataserver via dbwritetext(). */
#define SYBENTST        20170	/* The file being opened must be a stream_lf. */
#define SYBEOSSL        20171	/* Operating system login level not in range of Secure SQL Server */
#define SYBEESSL        20172	/* Login security level entered does not agree with operating system level */
#define SYBENLNL        20173	/* Program not linked with specified network library. */
#define SYBENHAN        20174	/* called dbrecvpassthru() with a NULL handler parameter. */
#define SYBENBUF        20175	/* called dbsendpassthru() with a NULL buf pointer. */
#define SYBENULP        20176	/* Called %s with a NULL %s parameter. */
#define SYBENOTI        20177	/* No event handler installed. */
#define SYBEEVOP        20178	/* Called dbregwatch() with a bad options parameter. */
#define SYBENEHA        20179	/* Called dbreghandle() with a NULL handler parameter. */
#define SYBETRAN        20180	/* DBPROCESS is being used for another transaction. */
#define SYBEEVST        20181	/* Must initiate a transaction before calling dbregparam(). */
#define SYBEEINI        20182	/* Must call dbreginit() before dbregraise(). */
#define SYBEECRT        20183	/* Must call dbregdefine() before dbregcreate(). */
#define SYBEECAN        20184	/* Attempted to cancel unrequested event notification. */
#define SYBEEUNR        20185	/* Unsolicited event notification received. */
#define SYBERPCS        20186	/* Must call dbrpcinit() before dbrpcparam(). */
#define SYBETPAR        20187	/* No SYBTEXT or SYBIMAGE parameters were defined. */
#define SYBETEXS        20188	/* Called dbmoretext() with a bad size parameter. */
#define SYBETRAC        20189	/* Attempted to turn off a trace flag that was not on. */
#define SYBETRAS        20190	/* DB-Library internal error - trace structure not found. */
#define SYBEPRTF        20191	/* dbtracestring() may only be called from a printfunc(). */
#define SYBETRSN        20192	/* Bad numbytes parameter passed to dbtracestring(). */
#define SYBEBPKS        20193	/* In DBSETLPACKET(), the packet size parameter must be between 0 and 999999. */
#define SYBEIPV         20194	/* %1! is an illegal value for the %2! parameter of %3!. */
#define SYBEMOV         20195	/* Money arithmetic resulted in overflow in function %1!. */
#define SYBEDIVZ        20196	/* Attempt to divide by $0.00 in function %1!. */
#define SYBEASTL        20197	/* Synchronous I/O attempted at AST level. */
#define SYBESEFA        20198	/* DBSETNOTIFS cannot be called if connections are present. */
#define SYBEPOLL        20199	/* Only one dbpoll() can be active at a time. */
#define SYBENOEV        20200	/* dbpoll() cannot be called if registered procedure notifications have been disabled. */
#define SYBEBADPK       20201	/* Packet size of %1! not supported. -- size of %2! used instead. */
#define SYBESECURE      20202	/* Secure Server function not supported in this version. */
#define SYBECAP         20203	/* DB-Library capabilities not accepted by the Server. */
#define SYBEFUNC        20204	/* Functionality not supported at the specified version level. */
#define SYBERESP        20205	/* Response function address passed to dbresponse() must be non-NULL. */
#define SYBEIVERS       20206	/* Illegal version level specified. */
#define SYBEONCE        20207	/* Function can be called only once. */
#define SYBERPNULL      20208	/* value parameter for dbprcparam() can be NULL, only if the datalen parameter is 0 */
#define SYBERPTXTIM     20209	/* RPC parameters cannot be of type Text/Image. */
#define SYBENEG         20210	/* Negotiated login attempt failed. */
#define SYBELBLEN       20211	/* Security labels should be less than 256 characters long. */
#define SYBEUMSG        20212	/* Unknown message-id in MSG datastream. */
#define SYBECAPTYP      20213	/* Unexpected capability type in CAPABILITY datastream. */
#define SYBEBNUM        20214	/* Bad numbytes parameter passed to dbstrcpy() */
#define SYBEBBL         20215	/* Bad bindlen parameter passed to dbsetnull() */
#define SYBEBPREC       20216	/* Illegal precision specified */
#define SYBEBSCALE      20217	/* Illegal scale specified */
#define SYBECDOMAIN     20218	/* Source field value is not within the domain of legal values. */
#define SYBECINTERNAL   20219	/* Internal Conversion error. */
#define SYBEBTYPSRV     20220	/* Datatype is not supported by the server. */
#define SYBEBCSET       20221	/* Unknown character-set encountered." */
#define SYBEFENC        20222	/* Password Encryption failed." */
#define SYBEFRES        20223	/* Challenge-Response function failed.", */
#define SYBEISRVPREC    20224	/* Illegal precision value returned by the server. */
#define SYBEISRVSCL     20225	/* Illegal scale value returned by the server. */
#define SYBEINUMCL      20226	/* Invalid numeric column length returned by the server. */
#define SYBEIDECCL      20227	/* Invalid decimal column length returned by the server. */
#define SYBEBCMTXT      20228	/* bcp_moretext() may be used only when there is at least one text or image column in the server table. */
#define SYBEBCPREC      20229	/* Column %1!: Illegal precision value encountered. */
#define SYBEBCBNPR      20230	/* bcp_bind(): if varaddr is NULL, prefixlen must be 0 and no terminator should be specified. */
#define SYBEBCBNTYP     20231	/* bcp_bind(): if varaddr is NULL and varlen greater than 0, the table column type must be SYBTEXT or SYBIMAGE and the program variable type must be SYBTEXT, SYBCHAR, SYBIMAGE or SYBBINARY. */
#define SYBEBCSNTYP     20232	/* column number %1!: if varaddr is NULL and varlen greater than 0, the table column type must be SYBTEXT or SYBIMAGE and the program variable type must be SYBTEXT, SYBCHAR, SYBIMAGE or SYBBINARY. */
#define SYBEBCPCTYP     20233	/* bcp_colfmt(): If table_colnum is 0, host_type cannot be 0. */
#define SYBEBCVLEN      20234	/* varlen should be greater than or equal to -1. */
#define SYBEBCHLEN      20235	/* host_collen should be greater than or equal to -1. */
#define SYBEBCBPREF     20236	/* Illegal prefix length. Legal values are 0, 1, 2 or 4. */
#define SYBEBCPREF      20237	/* Illegal prefix length. Legal values are -1, 0, 1, 2 or 4. */
#define SYBEBCITBNM     20238	/* bcp_init(): tblname parameter cannot be NULL. */
#define SYBEBCITBLEN    20239	/* bcp_init(): tblname parameter is too long. */
#define SYBEBCSNDROW    20240	/* bcp_sendrow() may NOT be called unless all text data for the previous row has been sent using bcp_moretext(). */
#define SYBEBPROCOL     20241	/* bcp protocol error: returned column count differs from the actual number of columns received. */
#define SYBEBPRODEF     20242	/* bcp protocol error: expected default information and got none. */
#define SYBEBPRONUMDEF  20243	/* bcp protocol error: expected number of defaults differs from the actual number of defaults received. */
#define SYBEBPRODEFID   20244	/* bcp protocol error: default column id and actual column id are not same */
#define SYBEBPRONODEF   20245	/* bcp protocol error:  default value received for column that does not have default. */
#define SYBEBPRODEFTYP  20246	/* bcp protocol error:  default value datatype differs from column datatype. */
#define SYBEBPROEXTDEF  20247	/* bcp protocol error: more than one row of default information received. */
#define SYBEBPROEXTRES  20248	/* bcp protocol error: unexpected set of results received. */
#define SYBEBPROBADDEF  20249	/* bcp protocol error: illegal default column id received. */
#define SYBEBPROBADTYP  20250	/* bcp protocol error: unknown column datatype. */
#define SYBEBPROBADLEN  20251	/* bcp protocol error: illegal datatype length received. */
#define SYBEBPROBADPREC 20252	/* bcp protocol error: illegal precision value received. */
#define SYBEBPROBADSCL  20253	/* bcp protocol error: illegal scale value received. */
#define SYBEBADTYPE     20254	/* Illegal value for type parameter  given to %1!. */
#define SYBECRSNORES    20255	/* Cursor statement generated no results. */
#define SYBECRSNOIND    20256	/* One of the tables involved in the cursor  statement does not have a unique index. */
#define SYBECRSVIEW     20257	/* A view cannot be joined with another table  or a view in a cursor statement. */
#define SYBECRSVIIND    20258	/* The view used in the cursor statement does  not include all the unique index columns of  the underlying tables. */
#define SYBECRSORD      20259	/* Only fully keyset driven cursors can have 'order by', ' group by', or 'having' phrases. */
#define SYBECRSBUFR     20260	/* Row buffering should not be turned on when  using cursor APIs. */
#define SYBECRSNOFREE   20261	/* The DBNOAUTOFREE option should not be  turned on when using cursor APIs. */
#define SYBECRSDIS      20262	/* Cursor statement contains one of the  disallowed phrases 'compute', 'union', 'for browse', or 'select into'. */
#define SYBECRSAGR      20263	/* Aggregate functions are not allowed in a  cursor statement. */
#define SYBECRSFRAND    20264	/* Fetch types RANDOM and RELATIVE can only be  used within the keyset of keyset driven  cursors. */
#define SYBECRSFLAST    20265	/* Fetch type LAST requires fully keyset  driven cursors. */
#define SYBECRSBROL     20266	/* Backward scrolling cannot be used in a  forward scrolling cursor. */
#define SYBECRSFROWN    20267	/* Row number to be fetched is outside valid  range. */
#define SYBECRSBSKEY    20268	/* Keyset cannot be scrolled backward in mixed  cursors with a previous fetch type. */
#define SYBECRSRO       20269	/* Data locking or modifications cannot be  made in a READONLY cursor. */
#define SYBECRSNOCOUNT  20270	/* The DBNOCOUNT option should not be  turned on when doing updates or deletes with  dbcursor(). */
#define SYBECRSTAB      20271	/* Table name must be determined in operations  involving data locking or modifications. */
#define SYBECRSUPDNB    20272	/* Update or insert operations cannot use bind  variables when binding type is NOBIND. */
#define SYBECRSNOWHERE  20273	/* A WHERE clause is not allowed in a cursor  update or insert. */
#define SYBECRSSET      20274	/* A SET clause is required for a cursor  update or insert.  */
#define SYBECRSUPDTAB   20275	/* Update or insert operations using bind  variables require single table cursors. */
#define SYBECRSNOUPD    20276	/* Update or delete operation did not affect  any rows. */
#define SYBECRSINV      20277	/* Invalid cursor statement. */
#define SYBECRSNOKEYS   20278	/* The entire keyset must be defined for KEYSET cursors. */
#define SYBECRSNOBIND   20279	/* Cursor bind must be called prior to updating cursor */
#define SYBECRSFTYPE    20280	/* Unknown fetch type. */
#define SYBECRSINVALID  20281	/* The cursor handle is invalid. */
#define SYBECRSMROWS    20282	/* Multiple rows are returned, only one is expected. */
#define SYBECRSNROWS    20283	/* No rows returned, at least one is expected. */
#define SYBECRSNOLEN    20284	/* No unique index found. */
#define SYBECRSNOPTCC   20285	/* No OPTCC was found. */
#define SYBECRSNORDER   20286	/* The order of clauses must be from, where, and order by. */
#define SYBECRSNOTABLE  20287	/* Table name is NULL. */
#define SYBECRSNUNIQUE  20288	/* No unique keys associated with this view. */
#define SYBECRSVAR      20289	/* There is no valid address associated with this bind. */
#define SYBENOVALUE     20290	/* Security labels require both a name and a value */
#define SYBEVOIDRET     20291	/* Parameter of type SYBVOID cannot  be a return parameter. */
#define SYBECLOSEIN     20292	/* Unable to close interface file. */
#define SYBEBOOL        20293	/* Boolean parameters must be TRUE or FALSE. */
#define SYBEBCPOPT      20294	/* The  option cannot be called while a bulk copy operation is progress. */
#define SYBEERRLABEL    20295	/* An illegal value was returned from the security label handler. */
#define SYBEATTNACK     20296	/* Timed out waiting for server to acknowledge attention." */
#define SYBEBBFL        20297	/* -001- Batch failed in bulk-copy to SQL Server */
#define SYBEDCL         20298	/* -004- DCL Error */
#define SYBECS          20299	/* -004- cs context Error */
#define SYBEBULKINSERT  20599	/* cannot build bulk insert statement */

int dbtds(DBPROCESS * dbprocess);

#define DBTDS(a)                dbtds(a)
DBINT dbtextsize(DBPROCESS * dbprocess);
DBBINARY *dbtxptr(DBPROCESS * dbproc, int column);
DBBINARY *dbtxtimestamp(DBPROCESS * dbproc, int column);
DBBINARY *dbtxtsnewval(DBPROCESS * dbprocess);
RETCODE dbtxtsput(DBPROCESS * dbprocess, DBBINARY newtxts, int colnum);
RETCODE dbuse(DBPROCESS * dbproc, const char *name);
const char *dbversion(void);
DBBOOL dbwillconvert(int srctype, int desttype);
RETCODE dbwritetext(DBPROCESS * dbproc, char *objname, DBBINARY * textptr, DBTINYINT textptrlen, DBBINARY * timestamp,
		    DBBOOL log, DBINT size, BYTE * text);

/* LOGINREC manipulation */
RETCODE dbsetlname(LOGINREC * login, const char *value, int which);
RETCODE dbsetlbool(LOGINREC * login, int value, int which);
RETCODE dbsetllong(LOGINREC * login, long value, int which);
RETCODE dbsetlversion (LOGINREC * login, BYTE version);

#define DBSETHOST		1
#define DBSETLHOST(x,y)		dbsetlname((x), (y), DBSETHOST)
#define dbsetlhost(x,y)		dbsetlname((x), (y), DBSETHOST)
#define DBSETUSER		2
#define DBSETLUSER(x,y)		dbsetlname((x), (y), DBSETUSER)
#define dbsetluser(x,y)		dbsetlname((x), (y), DBSETUSER)
#define DBSETPWD		3
#define DBSETLPWD(x,y)		dbsetlname((x), (y), DBSETPWD)
#define dbsetlpwd(x,y)		dbsetlname((x), (y), DBSETPWD)
#if defined(DBLIB_UNIMPLEMENTED)
# define DBSETHID		4	/* not implemented */
# define DBSETLHID(x,y)		dbsetlname((x), (y), DBSETHID)
#endif
#define DBSETAPP		5
#define DBSETLAPP(x,y)		dbsetlname((x), (y), DBSETAPP)
#define dbsetlapp(x,y)		dbsetlname((x), (y), DBSETAPP)
#define DBSETBCP		6
#define BCP_SETL(x,y)		dbsetlbool((x), (y), DBSETBCP)
#define DBSETLSECURE(x)		dbsetlbool((x), (1), DBSETBCP)
#define DBSETNATLANG		7	
#define DBSETLNATLANG(x,y)	dbsetlname((x), (y), DBSETNATLANG)
#define dbsetlnatlang(x,y)	dbsetlname((x), (y), DBSETNATLANG)
#if defined(DBLIB_UNIMPLEMENTED)
# define DBSETNOSHORT		8	/* not implemented */
# define DBSETLNOSHORT(x,y)	dbsetlbool((x), (y), DBSETNOSHORT)
# define DBSETHIER		9	/* not implemented */
# define DBSETLHIER(x,y)	dbsetlshort((x), (y), DBSETHIER)
#endif
#define DBSETCHARSET		10
#define DBSETLCHARSET(x,y)	dbsetlname((x), (y), DBSETCHARSET)
#define DBSETPACKET		11
#define DBSETLPACKET(x,y)	dbsetllong((x), (y), DBSETPACKET)
#define dbsetlpacket(x,y)	dbsetllong((x), (y), DBSETPACKET)
#define DBSETENCRYPT		12
#define DBSETLENCRYPT(x,y)	dbsetlbool((x), (y), DBSETENCRYPT)
#define DBSETLABELED		13
#define DBSETLLABELED(x,y)	dbsetlbool((x), (y), DBSETLABELED)
#define BCP_SETLABELED(x,y)	dbsetlbool((x), (y), DBSETLABELED)
#define DBSETDBNAME		14
#define DBSETLDBNAME(x,y)	dbsetlname((x), (y), DBSETDBNAME)
#define DBSETLVERSION(login, version) dbsetlversion((login), (version))

RETCODE bcp_init(DBPROCESS * dbproc, const char *tblname, const char *hfile, const char *errfile, int direction);
DBINT bcp_done(DBPROCESS * dbproc);

DBINT bcp_batch(DBPROCESS * dbproc);
RETCODE bcp_bind(DBPROCESS * dbproc, BYTE * varaddr, int prefixlen, DBINT varlen, BYTE * terminator, int termlen, int type,
		 int table_column);
RETCODE bcp_collen(DBPROCESS * dbproc, DBINT varlen, int table_column);
RETCODE bcp_columns(DBPROCESS * dbproc, int host_colcount);
RETCODE bcp_colfmt(DBPROCESS * dbproc, int host_column, int host_type, int host_prefixlen, DBINT host_collen,
		   const BYTE * host_term, int host_termlen, int colnum);
RETCODE bcp_colfmt_ps(DBPROCESS * dbproc, int host_column, int host_type, int host_prefixlen, DBINT host_collen,
		      BYTE * host_term, int host_termlen, int colnum, DBTYPEINFO * typeinfo);
RETCODE bcp_colptr(DBPROCESS * dbproc, BYTE * colptr, int table_column);
RETCODE bcp_control(DBPROCESS * dbproc, int field, DBINT value);
int bcp_getbatchsize(DBPROCESS * dbproc); /* FreeTDS only */
RETCODE bcp_exec(DBPROCESS * dbproc, DBINT * rows_copied);
DBBOOL bcp_getl(LOGINREC * login);
RETCODE bcp_options(DBPROCESS * dbproc, int option, BYTE * value, int valuelen);
RETCODE bcp_readfmt(DBPROCESS * dbproc, const char filename[]);
RETCODE bcp_sendrow(DBPROCESS * dbproc);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
