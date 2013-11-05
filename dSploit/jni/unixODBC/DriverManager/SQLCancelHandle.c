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
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

SQLRETURN SQLCancelHandle( SQLSMALLINT handle_type, SQLHANDLE handle )
{
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ];

    if ( handle_type == SQL_HANDLE_DBC )
    {
        DMHDBC connection = ( DMHDBC ) handle;

        if ( !__validate_dbc( connection ))
        {
            return SQL_INVALID_HANDLE;
        }

        thread_protect( SQL_HANDLE_DBC, connection );

        if ( log_info.log_flag )
        {
            sprintf( connection -> msg, 
                "\n\t\tEntry:\
\n\t\t\tConnection = %p",
                    connection );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    connection -> msg );
        }

        if ( CHECK_SQLCANCELHANDLE( connection ))
        {
            ret = SQLCANCELHANDLE( connection,
                 handle_type,
                 connection -> driver_dbc );
        }
        else
        {
            dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: IM001" );

            __post_internal_error( &connection -> error,
                    ERROR_IM001, NULL,
                    connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_DBC, connection, SQL_ERROR );
        }

        if ( SQL_SUCCEEDED( ret )) {
        }

        if ( log_info.log_flag )
        {
            sprintf( connection -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s1 ));
    
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    connection -> msg );
        }
    
        return function_return( SQL_HANDLE_DBC, connection, ret );
    }
    else if ( handle_type == SQL_HANDLE_STMT )
    {
        DMHSTMT statement = ( DMHSTMT ) handle;

        if ( !__validate_stmt( statement ))
        {
            return SQL_INVALID_HANDLE;
        }

        thread_protect( SQL_HANDLE_STMT, statement );

        if ( log_info.log_flag )
        {
            sprintf( statement -> msg, 
                "\n\t\tEntry:\
                \n\t\t\tStatement = %p",
                    statement );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    statement -> msg );
        }

        if ( CHECK_SQLCANCELHANDLE( statement -> connection ))
        {
            ret = SQLCANCELHANDLE( statement -> connection,
                handle_type,
                statement -> driver_stmt );
        }
        else if ( CHECK_SQLCANCEL( statement -> connection ))
        {
            ret = SQLCANCEL( statement -> connection,
                statement -> driver_stmt );
        }
        else
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
    else {
        return SQL_INVALID_HANDLE;
    }
}
