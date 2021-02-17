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

#ifndef _bkpublic_h_
#define _bkpublic_h_

static const char rcsid_bkpublic_h[] = "$Id: bkpublic.h,v 1.5 2004-10-28 12:42:11 freddy77 Exp $";
static const void *const no_unused_bkpublic_h_warn[] = { rcsid_bkpublic_h, no_unused_bkpublic_h_warn };

/* seperate this stuff out later */
#include <cspublic.h>

#ifdef __cplusplus
extern "C"
{
#if 0
}
#endif
#endif

/* buld properties start with 1 i guess */
#define BLK_IDENTITY 1

CS_RETCODE blk_alloc(CS_CONNECTION * connection, CS_INT version, CS_BLKDESC ** blk_pointer);
CS_RETCODE blk_bind(CS_BLKDESC * blkdesc, CS_INT colnum, CS_DATAFMT * datafmt, CS_VOID * buffer, CS_INT * datalen,
		    CS_SMALLINT * indicator);
CS_RETCODE blk_colval(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp, CS_INT colnum, CS_VOID * valuep,
		      CS_INT valuelen, CS_INT * outlenp);
CS_RETCODE blk_default(CS_BLKDESC * blkdesc, CS_INT colnum, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE blk_describe(CS_BLKDESC * blkdesc, CS_INT colnum, CS_DATAFMT * datafmt);
CS_RETCODE blk_done(CS_BLKDESC * blkdesc, CS_INT type, CS_INT * outrow);
CS_RETCODE blk_drop(CS_BLKDESC * blkdesc);
CS_RETCODE blk_getrow(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp);
CS_RETCODE blk_gettext(SRV_PROC * srvproc, CS_BLKDESC * blkdescp, CS_BLK_ROW * rowp, CS_INT bufsize, CS_INT * outlenp);
CS_RETCODE blk_init(CS_BLKDESC * blkdesc, CS_INT direction, CS_CHAR * tablename, CS_INT tnamelen);
CS_RETCODE blk_props(CS_BLKDESC * blkdesc, CS_INT action, CS_INT property, CS_VOID * buffer, CS_INT buflen, CS_INT * outlen);
CS_RETCODE blk_rowalloc(SRV_PROC * srvproc, CS_BLK_ROW ** row);
CS_RETCODE blk_rowdrop(SRV_PROC * srvproc, CS_BLK_ROW * row);
CS_RETCODE blk_rowxfer(CS_BLKDESC * blkdesc);
CS_RETCODE blk_rowxfer_mult(CS_BLKDESC * blkdesc, CS_INT * row_count);
CS_RETCODE blk_sendrow(CS_BLKDESC * blkdesc, CS_BLK_ROW * row);
CS_RETCODE blk_sendtext(CS_BLKDESC * blkdesc, CS_BLK_ROW * row, CS_BYTE * buffer, CS_INT buflen);
CS_RETCODE blk_srvinit(SRV_PROC * srvproc, CS_BLKDESC * blkdescp);
CS_RETCODE blk_textxfer(CS_BLKDESC * blkdesc, CS_BYTE * buffer, CS_INT buflen, CS_INT * outlen);

#ifdef __cplusplus
#if 0
{
#endif
}
#endif

#endif
