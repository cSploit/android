/**********************************************************************
 * SQLAllocEnv
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

SQLRETURN _AllocEnv( SQLHENV *phDrvEnv )
{
    HDRVENV *phEnv	= (HDRVENV*)phDrvEnv;
    

    /* SANITY CHECKS */
    if( NULL == phEnv )
        return SQL_INVALID_HANDLE;

    /* OK */

    /* allocate environment */
	*phEnv = malloc( sizeof(DRVENV) );
    if( SQL_NULL_HENV == *phEnv )
    {
        *phEnv = SQL_NULL_HENV;
        return SQL_ERROR;
    }

    /* initialise environment */
    memset( *phEnv, 0, sizeof(DRVENV) );
	(*phEnv)->hFirstDbc	= NULL;
    (*phEnv)->hLastDbc	= NULL;
    (*phEnv)->hLog 		= NULL;

	/* start logging		*/
    if ( !logOpen( &(*phEnv)->hLog, "[template]", NULL, 50 ) )
		(*phEnv)->hLog = NULL;
	logOn( (*phEnv)->hLog, 1 );

    /* ALLOCATE AND INIT DRIVER SPECIFIC STORAGE */
    (*phEnv)->hEnvExtras = malloc(sizeof(ENVEXTRAS));
    (*phEnv)->hEnvExtras->nDummy = -1;

  	logPushMsg( (*phEnv)->hLog, __FILE__, __FILE__, __LINE__, LOG_INFO, LOG_INFO, "SQL_SUCCESS" );

    return SQL_SUCCESS;
}

SQLRETURN SQLAllocEnv(	SQLHENV    *phDrvEnv )
{
  return _AllocEnv( phDrvEnv );
}

