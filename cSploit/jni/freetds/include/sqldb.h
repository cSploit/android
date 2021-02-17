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

#ifndef SQLDB_h
#define SQLDB_h

#include "./sybdb.h"

#define SQLCHAR SYBCHAR
#define SQLVARCHAR SYBVARCHAR
#define SQLINTN SYBINTN
#define SQLINT1 SYBINT1
#define SQLINT2 SYBINT2
#define SQLINT4 SYBINT4
#define SQLINT8 SYBINT8
#define SQLFLT8 SYBFLT8
#define SQLDATETIME SYBDATETIME
#define SQLBIT SYBBIT
#define SQLTEXT SYBTEXT
#define SQLIMAGE SYBIMAGE
#define SQLMONEY4 SYBMONEY4
#define SQLMONEY SYBMONEY
#define SQLDATETIM4 SYBDATETIME4
#define SQLFLT4 SYBREAL
#define SQLBINARY SYBBINARY
#define SQLVARBINARY SYBVARBINARY
#define SQLNUMERIC SYBNUMERIC
#define SQLDECIMAL SYBDECIMAL
#define SQLFLTN SYBFLTN
#define SQLMONEYN SYBMONEYN
#define SQLDATETIMN SYBDATETIMN
#define SQLVOID	SYBVOID

#define SMALLDATETIBIND SMALLDATETIMEBIND

#define DBERRHANDLE_PROC EHANDLEFUNC 
#define DBMSGHANDLE_PROC MHANDLEFUNC 

/* DB-Library errors as defined by Microsoft */
#define SQLEMEM		SYBEMEM
#define SQLENULL	SYBENULL
#define SQLENLOG	SYBENLOG
#define SQLEPWD		SYBEPWD
#define SQLECONN	SYBECONN
#define SQLEDDNE	SYBEDDNE
#define SQLENULLO	SYBENULLO
#define SQLESMSG	SYBESMSG
#define SQLEBTOK	SYBEBTOK
#define SQLENSPE	SYBENSPE
#define SQLEREAD	SYBEREAD
#define SQLECNOR	SYBECNOR
#define SQLETSIT	SYBETSIT
#define SQLEPARM	SYBEPARM
#define SQLEAUTN	SYBEAUTN
#define SQLECOFL	SYBECOFL
#define SQLERDCN	SYBERDCN
#define SQLEICN		SYBEICN
#define SQLECLOS	SYBECLOS
#define SQLENTXT	SYBENTXT
#define SQLEDNTI	SYBEDNTI
#define SQLETMTD	SYBETMTD
#define SQLEASEC	SYBEASEC
#define SQLENTLL	SYBENTLL
#define SQLETIME	SYBETIME
#define SQLEWRIT	SYBEWRIT
#define SQLEMODE	SYBEMODE
#define SQLEOOB		SYBEOOB
#define SQLEITIM	SYBEITIM
#define SQLEDBPS	SYBEDBPS
#define SQLEIOPT	SYBEIOPT
#define SQLEASNL	SYBEASNL
#define SQLEASUL	SYBEASUL
#define SQLENPRM	SYBENPRM
#define SQLEDBOP	SYBEDBOP
#define SQLENSIP	SYBENSIP
#define SQLECNULL	SYBECNULL
#define SQLESEOF	SYBESEOF
#define SQLERPND	SYBERPND
#define SQLECSYN	SYBECSYN
#define SQLENONET	SYBENONET
#define SQLEBTYP	SYBEBTYP
#define SQLEABNC	SYBEABNC
#define SQLEABMT	SYBEABMT
#define SQLEABNP	SYBEABNP
#define SQLEBNCR	SYBEBNCR
#define SQLEAAMT	SYBEAAMT
#define SQLENXID	SYBENXID
#define SQLEIFNB	SYBEIFNB
#define SQLEKBCO	SYBEKBCO
#define SQLEBBCI	SYBEBBCI
#define SQLEKBCI	SYBEKBCI
#define SQLEBCWE	SYBEBCWE
#define SQLEBCNN	SYBEBCNN
#define SQLEBCOR	SYBEBCOR
#define SQLEBCPI	SYBEBCPI
#define SQLEBCPN	SYBEBCPN
#define SQLEBCPB	SYBEBCPB
#define SQLEVDPT	SYBEVDPT
#define SQLEBIVI	SYBEBIVI
#define SQLEBCBC	SYBEBCBC
#define SQLEBCFO	SYBEBCFO
#define SQLEBCVH	SYBEBCVH
#define SQLEBCUO	SYBEBCUO
#define SQLEBUOE	SYBEBUOE
#define SQLEBWEF	SYBEBWEF
#define SQLEBTMT	SYBEBTMT
#define SQLEBEOF	SYBEBEOF
#define SQLEBCSI	SYBEBCSI
#define SQLEPNUL	SYBEPNUL
#define SQLEBSKERR	SYBEBSKERR
#define SQLEBDIO	SYBEBDIO
#define SQLEBCNT	SYBEBCNT
#define SQLEMDBP	SYBEMDBP
#define SQLINIT		SYBINIT
#define SQLCRSINV	SYBCRSINV
#define SQLCRSCMD	SYBCRSCMD
#define SQLCRSNOIND	SYBCRSNOIND
#define SQLCRSDIS	SYBCRSDIS
#define SQLCRSAGR	SYBCRSAGR
#define SQLCRSORD	SYBCRSORD
#define SQLCRSMEM	SYBCRSMEM
#define SQLCRSBSKEY	SYBCRSBSKEY
#define SQLCRSNORES	SYBCRSNORES
#define SQLCRSVIEW	SYBCRSVIEW
#define SQLCRSBUFR	SYBCRSBUFR
#define SQLCRSFROWN	SYBCRSFROWN
#define SQLCRSBROL	SYBCRSBROL
#define SQLCRSFRAND	SYBCRSFRAND
#define SQLCRSFLAST	SYBCRSFLAST
#define SQLCRSRO	SYBCRSRO
#define SQLCRSTAB	SYBCRSTAB
#define SQLCRSUPDTAB	SYBCRSUPDTAB
#define SQLCRSUPDNB	SYBCRSUPDNB
#define SQLCRSVIIND	SYBCRSVIIND
#define SQLCRSNOUPD	SYBCRSNOUPD
#define SQLCRSOS	SYBCRSOS
#define SQLEBCSA	SYBEBCSA
#define SQLEBCRO	SYBEBCRO
#define SQLEBCNE	SYBEBCNE
#define SQLEBCSK	SYBEBCSK
#define SQLEUVBF	SYBEUVBF
#define SQLEBIHC	SYBEBIHC
#define SQLEBWFF	SYBEBWFF
#define SQLNUMVAL	SYBNUMVAL
#define SQLEOLDVR	SYBEOLDVR
#define SQLEBCPS	SYBEBCPS
#define SQLEDTC		SYBEDTC
#define SQLENOTIMPL	SYBENOTIMPL
#define SQLENONFLOAT	SYBENONFLOAT
#define SQLECONNFB	SYBECONNFB


#define dbfreelogin(x) dbloginfree((x))

#define dbprocerrhandle(p, h) dberrhandle((h))
#define dbprocmsghandle(p, h) dbmsghandle((h))

#define dbwinexit()

static const char rcsid_sqldb_h[] = "$Id: sqldb.h,v 1.6 2009-12-02 22:35:18 jklowden Exp $";
static const void *const no_unused_sqldb_h_warn[] = { rcsid_sqldb_h, no_unused_sqldb_h_warn };


#endif
