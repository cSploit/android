/*********************************************************************
 *
 * This is based on code created by Peter Harvey,
 * (pharvey@codebydesign.com).
 *
 * Modified and extended by Nick Gorham
 * (nick@lurcher.org).
 *
 * Any bugs or problems should be considered the fault of Nick and not
 * Peter.
 *
 * copyright (c) 1999 Nick Gorham
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 **********************************************************************
 *
 * $Id: SQLCancel.c,v 1.4 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLCancel.c,v $
 * Revision 1.4  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.3  2003/10/30 18:20:45  lurcher
 *
 * Fix broken thread protection
 * Remove SQLNumResultCols after execute, lease S4/S% to driver
 * Fix string overrun in SQLDriverConnect
 * Add initial support for Interix
 *
 * Revision 1.2  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.1.1.1  2001/10/17 16:40:05  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.2  2001/04/12 17:43:35  nick
 *
 * Change logging and added autotest to odbctest
 *
 * Revision 1.1.1.1  2000/09/04 16:42:52  nick
 * Imported Sources
 *
 * Revision 1.7  1999/11/13 23:40:58  ngorham
 *
 * Alter the way DM logging works
 * Upgrade the Postgres driver to 6.4.6
 *
 * Revision 1.6  1999/10/24 23:54:17  ngorham
 *
 * First part of the changes to the error reporting
 *
 * Revision 1.5  1999/09/21 22:34:24  ngorham
 *
 * Improve performance by removing unneeded logging calls when logging is
 * disabled
 *
 * Revision 1.4  1999/07/10 21:10:15  ngorham
 *
 * Adjust error sqlstate from driver manager, depending on requested
 * version (ODBC2/3)
 *
 * Revision 1.3  1999/07/04 21:05:06  ngorham
 *
 * Add LGPL Headers to code
 *
 * Revision 1.2  1999/06/30 23:56:54  ngorham
 *
 * Add initial thread safety code
 *
 * Revision 1.1.1.1  1999/05/29 13:41:05  sShandyb
 * first go at it
 *
 * Revision 1.1.1.1  1999/05/27 18:23:17  pharvey
 * Imported sources
 *
 * Revision 1.2  1999/05/04 22:41:12  nick
 * and another night ends
 *
 * Revision 1.1  1999/04/25 23:02:41  nick
 * Initial revision
 *
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLCancel.c,v $ $Revision: 1.4 $";

SQLRETURN SQLCancel( SQLHSTMT statement_handle )
{
    DMHSTMT statement = (DMHSTMT) statement_handle;
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ];

    /*
     * check statement
     */

    if ( !__validate_stmt( statement ))
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

        return SQL_INVALID_HANDLE;
    }

    function_entry( statement );

    if ( log_info.log_flag )
    {
        sprintf( statement -> msg, "\n\t\tEntry:\
\n\t\t\tStatement = %p",
                statement );

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                statement -> msg );
    }

#if defined( HAVE_LIBPTH ) || defined( HAVE_LIBPTHREAD ) || defined( HAVE_LIBTHREAD )
    /*
     * Allow this past the thread checks if the driver is at all thread safe, as SQLCancel can 
     * be called across threads
     */
    if ( statement -> connection -> protection_level == 3 ) 
    {
        thread_protect( SQL_HANDLE_STMT, statement ); 
    }
#endif

    /*
     * check states
     */

    if ( !CHECK_SQLCANCEL( statement -> connection ))
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: IM001" );

        __post_internal_error( &statement -> error,
                ERROR_IM001, NULL,
                statement -> connection -> environment -> requested_version );

#if defined( HAVE_LIBPTH ) || defined( HAVE_LIBPTHREAD ) || defined( HAVE_LIBTHREAD )
        if ( statement -> connection -> protection_level == 3 ) 
        {
            return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
        }
        else 
        {
            return function_return( IGNORE_THREAD, statement, SQL_ERROR );
        }
#else 
        return function_return( IGNORE_THREAD, statement, SQL_ERROR );
#endif
    }

    ret = SQLCANCEL( statement -> connection,
            statement -> driver_stmt );

    if ( SQL_SUCCEEDED( ret ))
    {
        if ( statement -> state == STATE_S8 ||
            statement -> state == STATE_S9 ||
            statement -> state == STATE_S10 )
        {
            if ( statement -> interupted_func == SQL_API_SQLEXECDIRECT )
            {
                statement -> state = STATE_S1;
            }
            else if ( statement -> interupted_func == SQL_API_SQLEXECUTE )
            {
                if ( statement -> hascols )
                {
                    statement -> state = STATE_S3;
                }
                else
                {
                    statement -> state = STATE_S2;
                }
            }
            else if ( statement -> interupted_func ==
                    SQL_API_SQLBULKOPERATIONS )
            {
                if ( statement -> interupted_state == STATE_S5 ||
                        statement -> interupted_state == STATE_S6 ||
                        statement -> interupted_state == STATE_S7 )
                {
                    statement -> state = STATE_S6;
                    statement -> eod = 0;
                }
                else
                {
                    statement -> state = STATE_S6;
                    statement -> eod = 0;
                }
            }
            else if ( statement -> interupted_func ==
                    SQL_API_SQLSETPOS )
            {
                if ( statement -> interupted_state == STATE_S5 ||
                        statement -> interupted_state == STATE_S6 )
                {
                    statement -> state = STATE_S6;
                    statement -> eod = 0;
                }
                else if ( statement -> interupted_state == STATE_S7 )
                {
                    statement -> state = STATE_S7;
                }
            }
        }
        else if ( statement -> state == STATE_S11 ||
                statement -> state == STATE_S12 )
        {
            statement -> state = STATE_S12;
        }
    }

    if ( log_info.log_flag )
    {
        sprintf( statement -> msg, 
                "\n\t\tExit:[%s]",
                    __get_return_status( ret, s1 ));

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                statement -> msg );
    }

#if defined( HAVE_LIBPTH ) || defined( HAVE_LIBPTHREAD ) || defined( HAVE_LIBTHREAD )
    if ( statement -> connection -> protection_level == 3 ) 
    {
        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }
    else 
    {
        return function_return( IGNORE_THREAD, statement, ret );
    }
#else
    return function_return( IGNORE_THREAD, statement, ret );
#endif
}
