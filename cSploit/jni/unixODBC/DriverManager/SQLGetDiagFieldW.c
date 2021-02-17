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
 * $Id: SQLGetDiagFieldW.c,v 1.10 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLGetDiagFieldW.c,v $
 * Revision 1.10  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.9  2009/02/04 09:30:02  lurcher
 * Fix some SQLINTEGER/SQLLEN conflicts
 *
 * Revision 1.8  2007/02/28 15:37:48  lurcher
 * deal with drivers that call internal W functions and end up in the driver manager. controlled by the --enable-handlemap configure arg
 *
 * Revision 1.7  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.6  2002/11/11 17:10:14  lurcher
 *
 * VMS changes
 *
 * Revision 1.5  2002/08/23 09:42:37  lurcher
 *
 * Fix some build warnings with casts, and a AIX linker mod, to include
 * deplib's on the link line, but not the libtool generated ones
 *
 * Revision 1.4  2002/07/24 08:49:52  lurcher
 *
 * Alter UNICODE support to use iconv for UNICODE-ANSI conversion
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
 * Revision 1.2  2001/01/04 13:16:25  nick
 *
 * Add support for GNU portable threads and tidy up some UNICODE compile
 * warnings
 *
 * Revision 1.1  2000/12/31 20:30:54  nick
 *
 * Add UNICODE support
 *
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLGetDiagFieldW.c,v $";

#define ODBC30_SUBCLASS        "01S00,01S01,01S02,01S06,01S07,07S01,08S01,21S01,\
21S02,25S01,25S02,25S03,42S01,42S02,42S11,42S12,42S21,42S22,HY095,HY097,HY098,\
HY099,HY100,HY101,HY105,HY107,HY109,HY110,HY111,HYT00,HYT01,IM001,IM002,IM003,\
IM004,IM005,IM006,IM007,IM008,IM010,IM011,IM012"

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

static SQLRETURN extract_sql_error_field_w( EHEAD *head,
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
            SQLINTEGER val;
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
                        *((SQLINTEGER*)diag_info_ptr) = head -> diag_cursor_row_count;
                    }
                    return head -> diag_cursor_row_count_ret;

                  case SQL_DIAG_ROW_COUNT:
                    if ( SQL_SUCCEEDED( head -> diag_row_count_ret ) && diag_info_ptr )
                    {
                        *((SQLINTEGER*)diag_info_ptr) = head -> diag_row_count;
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
                    wide_strncpy( diag_info_ptr, head -> diag_dynamic_function, buffer_length );
                    if ( string_length_ptr )
                    {
                        *string_length_ptr = wide_strlen( head -> diag_dynamic_function ) * sizeof( SQLWCHAR );
                    }
                }
                return head -> diag_dynamic_function_ret;
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
                SQLCHAR *as1 = NULL;

                if ( buffer_length > 0 && diag_info_ptr )
                {
                    as1 = malloc( buffer_length + 1 );
                }

                ret = SQLGETDIAGFIELD( __get_connection( head ),
                        SQL_HANDLE_STMT,
                        __get_driver_handle( head ),
                        0,
                        diag_identifier,
                        as1 ? as1 : diag_info_ptr,
                        buffer_length / 2,
                        string_length_ptr );

                if ( SQL_SUCCEEDED( ret ) && as1 && diag_info_ptr )
                {
                    ansi_to_unicode_copy( diag_info_ptr, (char*) as1, SQL_NTS, __get_connection( head ));
                }
                if ( SQL_SUCCEEDED( ret ) && string_length_ptr )
                {
                    *string_length_ptr *= sizeof( SQLWCHAR );
                }

                if ( as1 )
                {
                    free( as1 );
                }

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
    else if ( rec_number <= head -> sql_diag_head.internal_count + 
            head -> sql_diag_head.error_count )
    {
        rec_number -= head -> sql_diag_head.internal_count;

        if ( __get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGFIELDW( __get_connection( head )))
        {
            SQLRETURN ret;

            ret = SQLGETDIAGFIELDW( __get_connection( head ),
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
                    __map_error_state_w( diag_info_ptr, __get_version( head ));
            }

            return ret;
        }
        else if ( !__get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGFIELD( __get_connection( head )))
        {
            SQLRETURN ret;
            SQLCHAR *as1 = NULL;

            if ( is_char_diag( diag_identifier ) && diag_info_ptr && buffer_length > 0 )
            {
                as1 = malloc( buffer_length + 1 );
            }

            ret = SQLGETDIAGFIELD( __get_connection( head ),
                    head -> handle_type,
                    __get_driver_handle( head ),
                    rec_number,
                    diag_identifier,
                    as1 ? as1 : diag_info_ptr,
                    buffer_length,
                    string_length_ptr );

            if ( SQL_SUCCEEDED( ret ) && diag_identifier == SQL_DIAG_SQLSTATE )
            {
                /*
                 * map 3 to 2 if required
                 */

                if ( diag_info_ptr && as1 )
                {
                    __map_error_state( (char*) as1, __get_version( head ));
                    ansi_to_unicode_copy( diag_info_ptr, (char*) as1, SQL_NTS, __get_connection( head ));
                }
            }

            if ( as1 )
            {
                free( as1 );
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
     * if we are here ptr should point to the error
     * record
     */

    switch( diag_identifier )
    {
      case SQL_DIAG_CLASS_ORIGIN:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_class_origin_ret ))
            {
                wide_strncpy( diag_info_ptr, ptr -> diag_class_origin, buffer_length );
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_class_origin ) * sizeof( SQLWCHAR );
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
                wide_strcpy( diag_info_ptr, ptr -> diag_connection_name );
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_connection_name ) * sizeof( SQLWCHAR );
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
            SQLWCHAR *str;
            int ret = SQL_SUCCESS;

            str = ptr -> msg;

            if ( diag_info_ptr )
            {
                if ( buffer_length >= wide_strlen( str ) + 1 )
                {
                    wide_strcpy( diag_info_ptr, str );
                }
                else
                {
                    ret = SQL_SUCCESS_WITH_INFO;
                    memcpy( diag_info_ptr, str, ( buffer_length - 1 ) * 2 );
                    (( SQLWCHAR * ) diag_info_ptr )[ buffer_length - 1 ] = '\0';
                }
            }
            if ( string_length_ptr )
            {
                *string_length_ptr = wide_strlen( str ) * sizeof( SQLWCHAR );
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
                memcpy( diag_info_ptr, &ptr -> diag_row_number, sizeof( SQLINTEGER ));
            }
            return SQL_SUCCESS;
        }
        break;

      case SQL_DIAG_SERVER_NAME:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_server_name_ret ))
            {
                wide_strcpy( diag_info_ptr, ptr -> diag_server_name );
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_server_name ) * sizeof( SQLWCHAR );
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
            SQLWCHAR *str;
            int ret = SQL_SUCCESS;

            str = ptr -> sqlstate;

            if ( diag_info_ptr )
            {
                if ( buffer_length >= wide_strlen( str ) + 1 )
                {
                    wide_strcpy( diag_info_ptr, str );
                }
                else
                {
                    ret = SQL_SUCCESS_WITH_INFO;
                    memcpy( diag_info_ptr, str, ( buffer_length - 1 ) * 2 );
                    (( SQLWCHAR * ) diag_info_ptr )[ buffer_length - 1 ] = '\0';
                }

                /*
                 * map 3 to 2 if required
                 */

                if ( diag_info_ptr )
                    __map_error_state_w( diag_info_ptr, __get_version( head ));
            }
            if ( string_length_ptr )
            {
                *string_length_ptr = wide_strlen( str ) * sizeof( SQLWCHAR );
            }
            return ret;
        }
        break;

      case SQL_DIAG_SUBCLASS_ORIGIN:
        {
            if ( SQL_SUCCEEDED( ptr -> diag_subclass_origin_ret ))
            {
                wide_strcpy( diag_info_ptr, ptr -> diag_subclass_origin );
                if ( string_length_ptr )
                {
                    *string_length_ptr = wide_strlen( ptr -> diag_subclass_origin ) * sizeof( SQLWCHAR );
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

SQLRETURN SQLGetDiagFieldW( SQLSMALLINT handle_type,
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

        ret = extract_sql_error_field_w( &environment -> error,
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
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

#ifdef WITH_HANDLE_REDIRECT
		{
			DMHDBC parent_connection;

			parent_connection = find_parent_handle( connection, SQL_HANDLE_DBC );

			if ( parent_connection ) {
        		dm_log_write( __FILE__, 
                	__LINE__, 
                    	LOG_INFO, 
                    	LOG_INFO, 
                    	"Info: found parent handle" );

				if ( CHECK_SQLGETDIAGFIELDW( parent_connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGFIELDW( parent_connection, 
							handle_type,
							connection, 
        					rec_number,
        					diag_identifier,
        					diag_info_ptr,
        					buffer_length,
        					string_length_ptr );
				}
			}
		}
#endif
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

        ret = extract_sql_error_field_w( &connection -> error,
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
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

#ifdef WITH_HANDLE_REDIRECT
		{
			DMHSTMT parent_statement;

			parent_statement = find_parent_handle( statement, SQL_HANDLE_STMT );

			if ( parent_statement ) {
        		dm_log_write( __FILE__, 
                	__LINE__, 
                    	LOG_INFO, 
                    	LOG_INFO, 
                    	"Info: found parent handle" );

				if ( CHECK_SQLGETDIAGFIELDW( parent_statement -> connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGFIELDW( parent_statement -> connection, 
							handle_type,
							statement, 
        					rec_number,
        					diag_identifier,
        					diag_info_ptr,
        					buffer_length,
        					string_length_ptr );
				}
			}
		}
#endif
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

        ret = extract_sql_error_field_w( &statement -> error,
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
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

#ifdef WITH_HANDLE_REDIRECT
		{
			DMHDESC parent_desc;

			parent_desc = find_parent_handle( descriptor, SQL_HANDLE_DESC );

			if ( parent_desc ) {
        		dm_log_write( __FILE__, 
                	__LINE__, 
                    	LOG_INFO, 
                    	LOG_INFO, 
                    	"Info: found parent handle" );

				if ( CHECK_SQLGETDIAGFIELDW( parent_desc -> connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGFIELDW( parent_desc -> connection, 
							handle_type,
							descriptor,
        					rec_number,
        					diag_identifier,
        					diag_info_ptr,
        					buffer_length,
        					string_length_ptr );
				}
			}
		}
#endif
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

        ret = extract_sql_error_field_w( &descriptor -> error,
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

