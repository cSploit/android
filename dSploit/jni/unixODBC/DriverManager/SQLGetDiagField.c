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
 * $Id: SQLGetDiagField.c,v 1.17 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLGetDiagField.c,v $
 * Revision 1.17  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.16  2009/02/17 09:47:44  lurcher
 * Clear up a number of bugs
 *
 * Revision 1.15  2009/02/04 09:30:01  lurcher
 * Fix some SQLINTEGER/SQLLEN conflicts
 *
 * Revision 1.14  2008/09/29 14:02:45  lurcher
 * Fix missing dlfcn group option
 *
 * Revision 1.13  2007/03/05 09:49:24  lurcher
 * Get it to build on VMS again
 *
 * Revision 1.12  2006/11/27 14:08:33  lurcher
 * Sync up dirs
 *
 * Revision 1.11  2006/05/31 17:35:34  lurcher
 * Add unicode ODBCINST entry points
 *
 * Revision 1.10  2006/03/08 09:18:41  lurcher
 * fix silly typo that was using sizeof( SQL_WCHAR ) instead of SQLWCHAR
 *
 * Revision 1.9  2005/12/19 18:43:26  lurcher
 * Add new parts to contrib and alter how the errors are returned from the driver
 *
 * Revision 1.8  2003/02/27 12:19:39  lurcher
 *
 * Add the A functions as well as the W
 *
 * Revision 1.7  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.6  2002/11/11 17:10:12  lurcher
 *
 * VMS changes
 *
 * Revision 1.5  2002/07/24 08:49:52  lurcher
 *
 * Alter UNICODE support to use iconv for UNICODE-ANSI conversion
 *
 * Revision 1.4  2002/01/30 12:20:02  lurcher
 *
 * Add MyODBC 3 driver source
 *
 * Revision 1.3  2002/01/21 18:00:51  lurcher
 *
 * Assorted fixed and changes, mainly UNICODE/bug fixes
 *
 * Revision 1.2  2001/12/13 13:00:32  lurcher
 *
 * Remove most if not all warnings on 64 bit platforms
 * Add support for new MS 3.52 64 bit changes
 * Add override to disable the stopping of tracing
 * Add MAX_ROWS support in postgres driver
 *
 * Revision 1.1.1.1  2001/10/17 16:40:05  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.4  2001/07/03 09:30:41  nick
 *
 * Add ability to alter size of displayed message in the log
 *
 * Revision 1.3  2001/04/12 17:43:36  nick
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
 * Revision 1.15  2000/08/22 22:56:27  ngorham
 *
 * Add fix to SQLGetDiagField to return the server name on statements and
 * descriptors
 *
 * Revision 1.14  2000/08/21 10:31:58  ngorham
 *
 * Add missing line continuation char
 *
 * Revision 1.13  2000/08/09 08:48:18  ngorham
 *
 * Fix for SQLGetDiagField(SQL_DIAG_SUBCLASS_ORIGIN) returning a null string
 *
 * Revision 1.12  2000/08/03 10:49:34  ngorham
 *
 * Fix buffer overrun problem in GetDiagField
 *
 * Revision 1.11  2000/07/31 20:48:01  ngorham
 *
 * Fix bugs in SQLGetDiagField and with SQLColAttributes
 *
 * Revision 1.10  2000/06/23 16:11:35  ngorham
 *
 * Map ODBC 2 SQLSTATE values to ODBC 3
 *
 * Revision 1.9  2000/05/21 21:49:19  ngorham
 *
 * Assorted fixes
 *
 * Revision 1.8  1999/11/17 21:08:58  ngorham
 *
 * Fix Bug with the ODBC 3 error handling
 *
 * Revision 1.7  1999/11/13 23:40:59  ngorham
 *
 * Alter the way DM logging works
 * Upgrade the Postgres driver to 6.4.6
 *
 * Revision 1.6  1999/11/10 03:51:33  ngorham
 *
 * Update the error reporting in the DM to enable ODBC 3 and 2 calls to
 * work at the same time
 *
 * Revision 1.5  1999/09/21 22:34:25  ngorham
 *
 * Improve performance by removing unneeded logging calls when logging is
 * disabled
 *
 * Revision 1.4  1999/07/12 19:42:06  ngorham
 *
 * Finished off SQLGetDiagField.c and fixed a but that caused SQLError to
 * fail with Perl and PHP, connect errors were not being returned because
 * I was checking to the environment being set, they were setting the
 * statement and the environment. The order of checking has been changed.
 *
 * Revision 1.3  1999/07/04 21:05:07  ngorham
 *
 * Add LGPL Headers to code
 *
 * Revision 1.2  1999/06/30 23:56:55  ngorham
 *
 * Add initial thread safety code
 *
 * Revision 1.1.1.1  1999/05/29 13:41:07  sShandyb
 * first go at it
 *
 * Revision 1.1.1.1  1999/05/27 18:23:17  pharvey
 * Imported sources
 *
 * Revision 1.5  1999/05/09 23:27:11  nick
 * All the API done now
 *
 * Revision 1.4  1999/05/04 22:41:12  nick
 * and another night ends
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

static char const rcsid[]= "$RCSfile: SQLGetDiagField.c,v $ $Revision: 1.17 $";

#define ODBC30_SUBCLASS        "01S00,01S01,01S02,01S06,01S07,07S01,08S01,21S01,\
21S02,25S01,25S02,25S03,42S01,42S02,42S11,42S12,42S21,42S22,HY095,HY097,HY098,\
HY099,HY100,HY101,HY105,HY107,HY109,HY110,HY111,HYT00,HYT01,IM001,IM002,IM003,\
IM004,IM005,IM006,IM007,IM008,IM010,IM011,IM012"

extern int __is_env( EHEAD * head );

/*
 * is it a diag identifier that we have to convert from unicode to ansi
 */

static int is_char_diag( int diag_identifier )
{
    switch( diag_identifier ) {
        case SQL_DIAG_CLASS_ORIGIN:
        case SQL_DIAG_CONNECTION_NAME:
        case SQL_DIAG_MESSAGE_TEXT:
        case SQL_DIAG_SERVER_NAME:
        case SQL_DIAG_SQLSTATE:
        case SQL_DIAG_SUBCLASS_ORIGIN:
            return 1;

        default:
            return 0;
    }
}

static SQLRETURN extract_sql_error_field( EHEAD *head,
                SQLSMALLINT rec_number,
                SQLSMALLINT diag_identifier,
                SQLPOINTER diag_info_ptr,
                SQLSMALLINT buffer_length,
                SQLSMALLINT *string_length_ptr )
{
    ERROR *ptr;

    /*
     * check the header fields first
     */

    switch( diag_identifier )
    {
      case SQL_DIAG_CURSOR_ROW_COUNT:
      case SQL_DIAG_ROW_COUNT:
        {
            SQLLEN val;
            SQLRETURN ret;

            if ( head -> handle_type != SQL_HANDLE_STMT )
            {
                return SQL_ERROR;
            }
            else if ( head -> header_set )
            {
                switch( diag_identifier )
                {
                  case SQL_DIAG_CURSOR_ROW_COUNT:
                    if ( SQL_SUCCEEDED( head -> diag_cursor_row_count_ret ) && diag_info_ptr )
                    {
                        *((SQLLEN*)diag_info_ptr) = head -> diag_cursor_row_count;
                    }
                    return head -> diag_cursor_row_count_ret;

                  case SQL_DIAG_ROW_COUNT:
                    if ( SQL_SUCCEEDED( head -> diag_row_count_ret ) && diag_info_ptr )
                    {
                        *((SQLLEN*)diag_info_ptr) = head -> diag_row_count;
                    }
                    return head -> diag_row_count_ret;
                }
            }
            else if ( __get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELDW( __get_connection( head )))
            {
                ret = SQLGETDIAGFIELDW( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                return ret;
            }
            else if ( !__get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELD( __get_connection( head )))
            {
                ret = SQLGETDIAGFIELD( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                return ret;
            }
            else if ( CHECK_SQLROWCOUNT( __get_connection( head )))
            {
                ret = DEF_SQLROWCOUNT( __get_connection( head ),
                    __get_driver_handle( head ),
                    &val );

                if ( !SQL_SUCCEEDED( ret ))
                {
                    return ret;
                }
            }
            else
            {
                val = 0;
            }

            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &val, sizeof( val ));
            }
        }
        return SQL_SUCCESS;

      case SQL_DIAG_DYNAMIC_FUNCTION:
        {
            SQLRETURN ret;

            if ( head -> handle_type != SQL_HANDLE_STMT )
            {
                return SQL_ERROR;
            }
            else if ( head -> header_set )
            {
                if ( SQL_SUCCEEDED( head -> diag_dynamic_function_ret ) && diag_info_ptr )
                {
                    unicode_to_ansi_copy( diag_info_ptr, buffer_length, head -> diag_dynamic_function, buffer_length, __get_connection( head ));
                    if ( string_length_ptr )
                    {
                        *string_length_ptr = wide_strlen( head -> diag_dynamic_function );
                    }
                }
                return head -> diag_dynamic_function_ret;
            }
            else if ( __get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELDW( __get_connection( head )))
            {
                SQLWCHAR *s1 = NULL;

                if ( buffer_length > 0 )
                {
                    s1 = malloc( sizeof( SQLWCHAR ) * ( buffer_length + 1 ));
                }
                
                ret = SQLGETDIAGFIELDW( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        s1 ? s1 : diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                if ( SQL_SUCCEEDED( ret ) && diag_info_ptr && s1 )
                {
                    unicode_to_ansi_copy( diag_info_ptr, buffer_length, s1, buffer_length, __get_connection( head ));
                }

                if ( s1 )
                {
                    free( s1 );
                }

                return ret;
            }
            else if ( !__get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELD( __get_connection( head )))
            {
                ret = SQLGETDIAGFIELD( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                return ret;
            }
            if ( diag_info_ptr )
            {
                strcpy( diag_info_ptr, "" );
            }
        }
        return SQL_SUCCESS;

      case SQL_DIAG_DYNAMIC_FUNCTION_CODE:
        {
            SQLINTEGER val;
            SQLRETURN ret;

            if ( head -> handle_type != SQL_HANDLE_STMT )
            {
                return SQL_ERROR;
            }
            else if ( head -> header_set )
            {
                if ( SQL_SUCCEEDED( head -> diag_dynamic_function_code_ret ) && diag_info_ptr )
                {
                    *((SQLINTEGER*)diag_info_ptr) = head -> diag_dynamic_function_code;
                }
                return head -> diag_dynamic_function_code_ret;
            }
            else if ( __get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELDW( __get_connection( head )))
            {
                ret = SQLGETDIAGFIELDW( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                return ret;
            }
            else if ( !__get_connection( head ) -> unicode_driver &&
                CHECK_SQLGETDIAGFIELD( __get_connection( head )))
            {
                ret = SQLGETDIAGFIELD( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );

                return ret;
            }
            else
            {
                val = SQL_DIAG_UNKNOWN_STATEMENT;
            }

            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &val, sizeof( val ));
            }
        }
        return SQL_SUCCESS;

      case SQL_DIAG_NUMBER:
        {
            SQLINTEGER val;
            
            val = head -> sql_diag_head.internal_count + 
                head -> sql_diag_head.error_count;

            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &val, sizeof( val ));
            }
        }
        return SQL_SUCCESS;

      case SQL_DIAG_RETURNCODE:
        {
            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &head -> return_code, 
                        sizeof( head -> return_code ));
            }
        }
        return SQL_SUCCESS;
    }

    /*
     * else check the records
     */

    if ( rec_number < 1 )
    {
        return SQL_ERROR;
    }

    if ( rec_number <= head -> sql_diag_head.internal_count )
    {
        /*
         * local errors
         */

        ptr = head -> sql_diag_head.internal_list_head;
        while( rec_number > 1 )
        {
            ptr = ptr -> next;
            rec_number --;
        }
		if ( !ptr ) 
		{
			return SQL_NO_DATA;
		}
    }
    else if ( !__is_env( head ) && __get_connection( head ) -> state != STATE_C2 )
    {
		rec_number -= head -> sql_diag_head.internal_count;

        if ( __get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGFIELDW( __get_connection( head )))
        {
            SQLRETURN ret;
            SQLWCHAR *s1 = NULL;
			int char_buffer_len = sizeof( SQLWCHAR ) * buffer_length;

            if ( is_char_diag( diag_identifier ) && diag_info_ptr && buffer_length > 0 )
            {
                s1 = malloc( char_buffer_len + sizeof( SQLWCHAR ));
            }

            ret = SQLGETDIAGFIELDW( __get_connection( head ),
                    head -> handle_type,
                    __get_driver_handle( head ),
                    rec_number,
                    diag_identifier,
                    s1 ? s1 : diag_info_ptr,
                    s1 ? char_buffer_len + sizeof( SQLWCHAR ) : buffer_length,
                    string_length_ptr );

            if ( SQL_SUCCEEDED( ret ) && s1 && diag_info_ptr )
            {
                unicode_to_ansi_copy( diag_info_ptr, buffer_length, s1, SQL_NTS, __get_connection( head ));
            }

            if ( s1 )
            {
                free( s1 );
            }

			if ( string_length_ptr && *string_length_ptr > 0 ) 
			{
				*string_length_ptr /= sizeof( SQLWCHAR );
			}

            if ( SQL_SUCCEEDED( ret ) && diag_identifier == SQL_DIAG_SQLSTATE )
            {
                /*
                 * map 3 to 2 if required
                 */
                if ( diag_info_ptr )
                {
                    if ( diag_info_ptr )
                        __map_error_state( diag_info_ptr, __get_version( head ));
                }
            }

            return ret;
        }
		else if ( !__get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGFIELD( __get_connection( head )))
        {
            SQLRETURN ret;

            ret = SQLGETDIAGFIELD( __get_connection( head ),
                    head -> handle_type,
                    __get_driver_handle( head ),
                    rec_number,
                    diag_identifier,
                    diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            if ( SQL_SUCCEEDED( ret ) && diag_identifier == SQL_DIAG_SQLSTATE )
            {
                /*
                 * map 3 to 2 if required
                 */
                if ( diag_info_ptr )
                {
                    if ( diag_info_ptr )
                        __map_error_state( diag_info_ptr, __get_version( head ));
                }
            }

            return ret;
        }
        else
        {
            ptr = head -> sql_diag_head.error_list_head;
            while( rec_number > 1 )
            {
                ptr = ptr -> next;
                rec_number --;
            }
			if ( !ptr ) 
			{
	    		return SQL_NO_DATA;
			}
        }
    }
    else 
    {
	    return SQL_NO_DATA;
    }

    /*
     * if we are here ptr should point to the local error
     * record
     */

    switch( diag_identifier )
    {
      case SQL_DIAG_CLASS_ORIGIN:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_class_origin_ret ))
            {
                unicode_to_ansi_copy( diag_info_ptr, buffer_length, ptr -> diag_class_origin, buffer_length, __get_connection( head ));
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_class_origin );
                }
                return ptr -> diag_class_origin_ret;
            }
            else
            {
                return ptr -> diag_class_origin_ret;
            }
        }
        break;

      case SQL_DIAG_COLUMN_NUMBER:
        {
            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &ptr -> diag_column_number, sizeof( SQLINTEGER ));
            }
            return SQL_SUCCESS;
        }
        break;

      case SQL_DIAG_CONNECTION_NAME:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_connection_name_ret ))
            {
                unicode_to_ansi_copy( diag_info_ptr, buffer_length, ptr -> diag_connection_name, buffer_length, __get_connection( head ));
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_connection_name );
                }
                return ptr -> diag_connection_name_ret;
            }
            else
            {
                return ptr -> diag_connection_name_ret;
            }
        }
        break;

      case SQL_DIAG_MESSAGE_TEXT:
        {
            char *str;
            int ret = SQL_SUCCESS;

            str = unicode_to_ansi_alloc( ptr -> msg, SQL_NTS, __get_connection( head ));

            if ( diag_info_ptr )
            {
                if ( buffer_length >= strlen( str ) + 1 )
                {
                    strcpy( diag_info_ptr, str );
                }
                else
                {
                    ret = SQL_SUCCESS_WITH_INFO;
                    memcpy( diag_info_ptr, str, buffer_length - 1 );
                    (( char * ) diag_info_ptr )[ buffer_length - 1 ] = '\0';
                }
            }
            if ( string_length_ptr )
            {
                *string_length_ptr = strlen( str );
            }

            if ( str )
            {
                free( str );
            }

            return ret;
        }
        break;

      case SQL_DIAG_NATIVE:
        {
            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &ptr -> native_error, sizeof( SQLINTEGER ));
            }
            return SQL_SUCCESS;
        }
        break;

      case SQL_DIAG_ROW_NUMBER:
        {
            if ( diag_info_ptr )
            {
                memcpy( diag_info_ptr, &ptr -> diag_row_number, sizeof( SQLLEN ));
            }
            return SQL_SUCCESS;
        }
        break;

      case SQL_DIAG_SERVER_NAME:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_server_name_ret ))
            {
                unicode_to_ansi_copy( diag_info_ptr, buffer_length, ptr -> diag_server_name, buffer_length, __get_connection( head ));
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_server_name );
                }
                return ptr -> diag_server_name_ret;
            }
            else
            {
                return ptr -> diag_server_name_ret;
            }
        }
        break;

      case SQL_DIAG_SQLSTATE:
        {
            char *str;
            int ret = SQL_SUCCESS;

            str = unicode_to_ansi_alloc( ptr -> sqlstate, SQL_NTS, __get_connection( head ));

            if ( diag_info_ptr )
            {
                if ( buffer_length >= strlen( str ) + 1 )
                {
                    strcpy( diag_info_ptr, str );
                }
                else
                {
                    ret = SQL_SUCCESS_WITH_INFO;
                    memcpy( diag_info_ptr, str, buffer_length - 1 );
                    (( char * ) diag_info_ptr )[ buffer_length - 1 ] = '\0';
                }

                /*
                 * map 3 to 2 if required
                 */

                if ( diag_info_ptr )
                    __map_error_state( diag_info_ptr, __get_version( head ));
            }
            if ( string_length_ptr )
            {
                *string_length_ptr = strlen( str );
            }

            if ( str )
            {
                free( str );
            }

            return ret;
        }
        break;

      case SQL_DIAG_SUBCLASS_ORIGIN:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_subclass_origin_ret ))
            {
                unicode_to_ansi_copy( diag_info_ptr, buffer_length, ptr -> diag_subclass_origin, buffer_length, __get_connection( head ));
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_subclass_origin );
                }
                return ptr -> diag_subclass_origin_ret;
            }
            else
            {
                return ptr -> diag_subclass_origin_ret;
            }
        }
        break;
    }

    return SQL_SUCCESS;
}

SQLRETURN SQLGetDiagFieldA( SQLSMALLINT handle_type,
        SQLHANDLE handle,
        SQLSMALLINT rec_number,
        SQLSMALLINT diag_identifier,
        SQLPOINTER diag_info_ptr,
        SQLSMALLINT buffer_length,
        SQLSMALLINT *string_length_ptr )
{
    return SQLGetDiagField( handle_type,
                        handle,
                        rec_number,
                        diag_identifier,
                        diag_info_ptr,
                        buffer_length,
                        string_length_ptr );
}

SQLRETURN SQLGetDiagField( SQLSMALLINT handle_type,
        SQLHANDLE handle,
        SQLSMALLINT rec_number,
        SQLSMALLINT diag_identifier,
        SQLPOINTER diag_info_ptr,
        SQLSMALLINT buffer_length,
        SQLSMALLINT *string_length_ptr )
{
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ];

    if ( handle_type == SQL_HANDLE_ENV )
    {
        DMHENV environment = ( DMHENV ) handle;

        if ( !__validate_env( environment ))
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

            return SQL_INVALID_HANDLE;
        }

        thread_protect( SQL_HANDLE_ENV, environment );

        if ( log_info.log_flag )
        {
            sprintf( environment -> msg, 
                "\n\t\tEntry:\
\n\t\t\tEnvironment = %p\
\n\t\t\tRec Number = %d\
\n\t\t\tDiag Ident = %d\
\n\t\t\tDiag Info Ptr = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tString Len Ptr = %p",
                    environment,
                    rec_number,
                    diag_identifier,
                    diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    environment -> msg );
        }

        ret = extract_sql_error_field( &environment -> error,
                rec_number,
                diag_identifier,
                diag_info_ptr,
                buffer_length,
                string_length_ptr );

        if ( log_info.log_flag )
        {
            sprintf( environment -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s1 ));

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    environment -> msg );
        }

        thread_release( SQL_HANDLE_ENV, environment );

        return ret;
    }
    else if ( handle_type == SQL_HANDLE_DBC )
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
\n\t\t\tConnection = %p\
\n\t\t\tRec Number = %d\
\n\t\t\tDiag Ident = %d\
\n\t\t\tDiag Info Ptr = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tString Len Ptr = %p",
                    connection,
                    rec_number,
                    diag_identifier,
                    diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    connection -> msg );
        }

        ret = extract_sql_error_field( &connection -> error,
                rec_number,
                diag_identifier,
                diag_info_ptr,
                buffer_length,
                string_length_ptr );

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

        thread_release( SQL_HANDLE_DBC, connection );

        return ret;
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
\n\t\t\tStatement = %p\
\n\t\t\tRec Number = %d\
\n\t\t\tDiag Ident = %d\
\n\t\t\tDiag Info Ptr = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tString Len Ptr = %p",
                    statement,
                    rec_number,
                    diag_identifier,
                    diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    statement -> msg );
        }

        ret = extract_sql_error_field( &statement -> error,
                rec_number,
                diag_identifier,
                diag_info_ptr,
                buffer_length,
                string_length_ptr );

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

        thread_release( SQL_HANDLE_STMT, statement );

        return ret;
    }
    else if ( handle_type == SQL_HANDLE_DESC )
    {
        DMHDESC descriptor = ( DMHDESC ) handle;

        if ( !__validate_desc( descriptor ))
        {
            return SQL_INVALID_HANDLE;
        }

        thread_protect( SQL_HANDLE_DESC, descriptor );

        if ( log_info.log_flag )
        {
            sprintf( descriptor -> msg, 
                "\n\t\tEntry:\
\n\t\t\tDescriptor = %p\
\n\t\t\tRec Number = %d\
\n\t\t\tDiag Ident = %d\
\n\t\t\tDiag Info Ptr = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tString Len Ptr = %p",
                    descriptor,
                    rec_number,
                    diag_identifier,
                    diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    descriptor -> msg );
        }

        ret = extract_sql_error_field( &descriptor -> error,
                rec_number,
                diag_identifier,
                diag_info_ptr,
                buffer_length,
                string_length_ptr );

        if ( log_info.log_flag )
        {
            sprintf( descriptor -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s1 ));

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    descriptor -> msg );
        }

        thread_release( SQL_HANDLE_DESC, descriptor );

        return ret;
    }
    return SQL_NO_DATA;
}

