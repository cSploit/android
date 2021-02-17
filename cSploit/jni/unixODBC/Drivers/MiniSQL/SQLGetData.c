/**********************************************************************
 * SQLGetData
 *
 * 1. MiniSQL server sends all data as ascii strings so things are
 * simplified. We always convert from string to nTargetType.
 *
 **********************************************************************
 *
 * This code was created by Peter Harvey (mostly during Christmas 98/99).
 * This code is LGPL. Please ensure that this message remains in future
 * distributions and uses of this code (thats about all I get out of it).
 * - Peter Harvey pharvey@codebydesign.com
 *
 **********************************************************************/

#include <config.h>
#include "driver.h"

SQLRETURN SQLGetData( SQLHSTMT      hDrvStmt,
					  SQLUSMALLINT  nCol,
					  SQLSMALLINT   nTargetType,				/* C DATA TYPE */
					  SQLPOINTER    pTarget,
					  SQLINTEGER    nTargetLength,
					  SQLINTEGER    *pnLengthOrIndicator )
{
	return _GetData(	hDrvStmt,
						nCol,
						nTargetType,		
						pTarget,
						nTargetLength,
						pnLengthOrIndicator );
}


