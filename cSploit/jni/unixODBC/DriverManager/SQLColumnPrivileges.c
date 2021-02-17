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
 * $Id: SQLColumnPrivileges.c,v 1.8 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLColumnPrivileges.c,v $
 * Revision 1.8  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.7  2005/11/21 17:25:43  lurcher
 * A few DM fixes for Oracle's ODBC driver
 *
 * Revision 1.6  2004/01/12 09:54:39  lurcher
 *
 * Fix problem where STATE_S5 stops metadata calls
 *
 * Revision 1.5  2003/10/30 18:20:45  lurcher
 *
 * Fix broken thread protection
 * Remove SQLNumResultCols after execute, lease S4/S% to driver
 * Fix string overrun in SQLDriverConnect
 * Add initial support for Interix
 *
 * Revision 1.4  2003/02/27 12:19:39  lurcher
 *
 * Add the A functions as well as the W
 *
 * Revision 1.3  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.2  2002/07/24 08:49:51  lurcher
 *
 * Alter UNICODE support to use iconv for UNICODE-ANSI conversion
 *
 * Revision 1.1.1.1  2001/10/17 16:40:05  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.4  2001/07/03 09:30:41  nick
 *
 * Add ability to alter size of displayed message in the log
 *
 * Revision 1.3  2001/04/12 17:43:35  nick
 *
 * Change logging and added autotest to odbctest
 *
 * Revision 1.2  2000/12/31 20:30:54  nick
 *
 * Add UNICODE support
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
 * Revision 1.3  1999/07/04 21:05:07  ngorham
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
 * Revision 1.3  1999/05/03 19:50:43  nick
 * Another check point
 *
 * Revision 1.2  1999/04/30 16:22:47  nick
 * Another checkpoint
 *
 * Revision 1.1  1999/04/25 23:06:11  nick
 * Initial revision
 *
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLColumnPrivileges.c,v $ $Revision: 1.8 $";

SQLRETURN SQLColumnPrivilegesA(
    SQLHSTMT            statement_handle,
    SQLCHAR             *catalog_name,
    SQLSMALLINT         name_length1,
    SQLCHAR             *schema_name,
    SQLSMALLINT         name_length2,
    SQLCHAR             *table_name,
    SQLSMALLINT         name_length3,
    SQLCHAR             *column_name,
    SQLSMALLINT         name_length4 )
{
    return SQLColumnPrivileges( statement_handle,
                                catalog_name,
                                name_length1,
                                schema_name,
                                name_length2,
                                table_name,
                                name_length3,
                                column_name,
                                name_length4 );
}

SQLRETURN SQLColumnPrivileges(
    SQLHSTMT            statement_handle,
    SQLCHAR             *catalog_name,
    SQLSMALLINT         name_length1,
    SQLCHAR             *schema_name,
    SQLSMALLINT         name_length2,
    SQLCHAR             *table_name,
    SQLSMALLINT         name_length3,
    SQLCHAR             *column_name,
    SQLSMALLINT         name_length4 )
{
    DMHSTMT statement = (DMHSTMT) statement_handle;
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ], s2[ 100 + LOG_MESSAGE_LEN ], s3[ 100 + LOG_MESSAGE_LEN ], s4[ 100 + LOG_MESSAGE_LEN ];

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
\n\t\t\tStatement = %p\
\n\t\t\tCatalog Name = %s\
\n\t\t\tSchema Name = %s\
\n\t\t\tTable Name = %s\
\n\t\t\tColumn Name = %s", 
                statement,
                __string_with_length( s1, catalog_name, name_length1 ), 
                __string_with_length( s2, schema_name, name_length2 ), 
                __string_with_length( s3, table_name, name_length3 ), 
                __string_with_length( s4, column_name, name_length4 ));

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                statement -> msg );
    }

    thread_protect( SQL_HANDLE_STMT, statement );

    if (( name_length1 < 0 && name_length1 != SQL_NTS ) ||
            ( name_length2 < 0 && name_length2 != SQL_NTS ) ||
            ( name_length3 < 0 && name_length3 != SQL_NTS ) ||
            ( name_length4 < 0 && name_length4 != SQL_NTS ))
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY090" );

        __post_internal_error( &statement -> error,
                ERROR_HY090, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    /*
     * check states
     */

#ifdef NR_PROBE
    if ( statement -> state == STATE_S5 ||
            statement -> state == STATE_S6 ||
            statement -> state == STATE_S7 )
#else
    if (( statement -> state == STATE_S6 && statement -> eod == 0 ) ||
            statement -> state == STATE_S7 )
#endif
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: 24000" );

        __post_internal_error( &statement -> error,
                ERROR_24000, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }
    else if ( statement -> state == STATE_S8 ||
            statement -> state == STATE_S9 ||
            statement -> state == STATE_S10 )
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY010" );

        __post_internal_error( &statement -> error,
                ERROR_HY010, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    if ( statement -> state == STATE_S11 ||
            statement -> state == STATE_S12 )
    {
        if ( statement -> interupted_func != SQL_API_SQLCOLUMNPRIVILEGES )
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: HY010" );

            __post_internal_error( &statement -> error,
                    ERROR_HY010, NULL,
                    statement -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
        }
    }

    if ( table_name == NULL ) 
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY009" );

        __post_internal_error( &statement -> error,
                ERROR_HY009, NULL,
                statement -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
    }

    /*
     * TO_DO Check the SQL_ATTR_METADATA_ID settings
     */

    if ( statement -> connection -> unicode_driver )
    {
        SQLWCHAR *s1, *s2, *s3, *s4;

        if ( !CHECK_SQLCOLUMNPRIVILEGESW( statement -> connection ))
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: IM001" );

            __post_internal_error( &statement -> error,
                    ERROR_IM001, NULL,
                    statement -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
        }

        s1 = ansi_to_unicode_alloc( catalog_name, name_length1, statement -> connection );
        s2 = ansi_to_unicode_alloc( schema_name, name_length2, statement -> connection );
        s3 = ansi_to_unicode_alloc( table_name, name_length3, statement -> connection );
        s4 = ansi_to_unicode_alloc( column_name, name_length4, statement -> connection );

        ret = SQLCOLUMNPRIVILEGESW( statement -> connection ,
                statement -> driver_stmt,
                s1,
                name_length1,
                s2,
                name_length2,
                s3,
                name_length3,
                s4,
                name_length4 );

        if( s1 )
            free( s1 );
        if( s2 )
            free( s2 );
        if( s3 )
            free( s3 );
        if( s4 )
            free( s4 );
    }
    else
    {
        if ( !CHECK_SQLCOLUMNPRIVILEGES( statement -> connection ))
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: IM001" );

            __post_internal_error( &statement -> error,
                    ERROR_IM001, NULL,
                    statement -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_STMT, statement, SQL_ERROR );
        }

        ret = SQLCOLUMNPRIVILEGES( statement -> connection ,
                statement -> driver_stmt,
                catalog_name,
                name_length1,
                schema_name,
                name_length2,
                table_name,
                name_length3,
                column_name,
                name_length4 );
    }

    if ( SQL_SUCCEEDED( ret ))
    {
        statement -> state = STATE_S5;
        statement -> prepared = 0;
    }
    else if ( ret == SQL_STILL_EXECUTING )
    {
        statement -> interupted_func = SQL_API_SQLCOLUMNPRIVILEGES;
        if ( statement -> state != STATE_S11 &&
                statement -> state != STATE_S12 )
            statement -> state = STATE_S11;
    }
    else
    {
        statement -> state = STATE_S1;
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

    return function_return( SQL_HANDLE_STMT, statement, ret );
}
