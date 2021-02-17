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
 * $Id: SQLGetDiagRecW.c,v 1.11 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLGetDiagRecW.c,v $
 * Revision 1.11  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.10  2009/02/04 09:30:02  lurcher
 * Fix some SQLINTEGER/SQLLEN conflicts
 *
 * Revision 1.9  2007/11/26 11:37:23  lurcher
 * Sync up before tag
 *
 * Revision 1.8  2007/02/28 15:37:48  lurcher
 * deal with drivers that call internal W functions and end up in the driver manager. controlled by the --enable-handlemap configure arg
 *
 * Revision 1.7  2002/12/05 17:44:31  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.6  2002/11/11 17:10:17  lurcher
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
 * Revision 1.3  2002/05/21 14:19:44  lurcher
 *
 * * Update libtool to escape from AIX build problem
 * * Add fix to avoid file handle limitations
 * * Add more UNICODE changes, it looks like it is native 16 representation
 *   the old way can be reproduced by defining UCS16BE
 * * Add iusql, its just the same as isql but uses the wide functions
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
 * Revision 1.3  2001/07/03 09:30:41  nick
 *
 * Add ability to alter size of displayed message in the log
 *
 * Revision 1.2  2001/04/12 17:43:36  nick
 *
 * Change logging and added autotest to odbctest
 *
 * Revision 1.1  2000/12/31 20:30:54  nick
 *
 * Add UNICODE support
 *
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLGetDiagRecW.c,v $";

static SQLRETURN extract_sql_error_rec_w( EHEAD *head,
        SQLWCHAR *sqlstate,
        SQLINTEGER rec_number,
        SQLINTEGER *native_error,
        SQLWCHAR *message_text,
        SQLSMALLINT buffer_length,
        SQLSMALLINT *text_length )
{
    SQLRETURN ret;

    if ( sqlstate )
    {
        SQLWCHAR *tmp;

        tmp = ansi_to_unicode_alloc((SQLCHAR*) "00000", SQL_NTS, __get_connection( head ));
        wide_strcpy( sqlstate, tmp );
        free( tmp );
    }

    if ( rec_number <= head -> sql_diag_head.internal_count )
    {
        ERROR *ptr;

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

        if ( sqlstate )
        {
            wide_strcpy( sqlstate, ptr -> sqlstate );
        }
        if ( buffer_length < wide_strlen( ptr -> msg ) + 1 )
        {
            ret = SQL_SUCCESS_WITH_INFO;
        }
        else
        {
            ret = SQL_SUCCESS;
        }

        if ( message_text )
        {
            if ( ret == SQL_SUCCESS )
            {
                wide_strcpy( message_text, ptr -> msg );
            }
            else
            {
                memcpy( message_text, ptr -> msg, buffer_length * 2 );
                message_text[ buffer_length - 1 ] = '\0';
            }
        }

        if ( text_length )
        {
            *text_length = wide_strlen( ptr -> msg );
        }

        if ( native_error )
        {
            *native_error = ptr -> native_error;
        }

        /*
         * map 3 to 2 if required
         */

        if ( SQL_SUCCEEDED( ret ) && sqlstate )
            __map_error_state((char*) sqlstate, __get_version( head ));

        return ret;
    }
    else if ( rec_number <= head -> sql_diag_head.internal_count + 
            head -> sql_diag_head.error_count )
    {
        ERROR *ptr;
        rec_number -= head -> sql_diag_head.internal_count;

        if ( __get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGRECW( __get_connection( head )))
        {
            ret = SQLGETDIAGRECW( __get_connection( head ),
                    head -> handle_type,
                    __get_driver_handle( head ),
                    rec_number,
                    sqlstate,
                    native_error,
                    message_text,
                    buffer_length,
                    text_length );

            /*
             * map 3 to 2 if required
             */

            if ( SQL_SUCCEEDED( ret ) && sqlstate )
            {
                __map_error_state_w( sqlstate, __get_version( head ));
            }

            return ret;
        }
        else if ( !__get_connection( head ) -> unicode_driver &&
            CHECK_SQLGETDIAGREC( __get_connection( head )))
        {
            SQLCHAR *as1 = NULL, *as2 = NULL;

            if ( sqlstate )
            {
                as1 = malloc( 7 );
            }

            if ( message_text && buffer_length > 0 )
            {
                as2 = malloc( buffer_length + 1 );
            }
                
            ret = SQLGETDIAGREC( __get_connection( head ),
                    head -> handle_type,
                    __get_driver_handle( head ),
                    rec_number,
                    as1 ? as1 : (SQLCHAR *)sqlstate,
                    native_error,
                    as2 ? as2 : (SQLCHAR *)message_text,
                    buffer_length,
                    text_length );

            /*
             * map 3 to 2 if required
             */

            if ( SQL_SUCCEEDED( ret ) && sqlstate )
            {
                if ( sqlstate )
                {
                    if ( as1 )
                    {
                        ansi_to_unicode_copy( sqlstate,(char*) as1, SQL_NTS, __get_connection( head ));
                        __map_error_state_w( sqlstate, __get_version( head ));
                    }
                }
                if ( message_text )
                {
                    if ( as2 )
                    {
                        ansi_to_unicode_copy( message_text,(char*) as2, SQL_NTS, __get_connection( head ));
                    }
                }
            }

            if ( as1 ) free( as1 );
            if ( as2 ) free( as2 );

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

            if ( sqlstate )
            {
                wide_strcpy( sqlstate, ptr -> sqlstate );
            }
            if ( buffer_length < wide_strlen( ptr -> msg ) + 1 )
            {
                ret = SQL_SUCCESS_WITH_INFO;
            }
            else
            {
                ret = SQL_SUCCESS;
            }

            if ( message_text )
            {
                if ( ret == SQL_SUCCESS )
                {
                    wide_strcpy( message_text, ptr -> msg );
                }
                else
                {
                    memcpy( message_text, ptr -> msg, buffer_length * 2 );
                    message_text[ buffer_length - 1 ] = '\0';
                }
            }

            if ( text_length )
            {
                *text_length = wide_strlen( ptr -> msg );
            }

            if ( native_error )
            {
                *native_error = ptr -> native_error;
            }

            /*
             * map 3 to 2 if required
             */

            if ( SQL_SUCCEEDED( ret ) && sqlstate )
                __map_error_state_w( sqlstate, __get_version( head ));

            return ret;
        }
    }
    else
    {
        return SQL_NO_DATA;
    }
}

SQLRETURN SQLGetDiagRecW( SQLSMALLINT handle_type,
        SQLHANDLE   handle,
        SQLSMALLINT rec_number,
        SQLWCHAR     *sqlstate,
        SQLINTEGER  *native,
        SQLWCHAR     *message_text,
        SQLSMALLINT buffer_length,
        SQLSMALLINT *text_length_ptr )
{
    SQLRETURN ret;
    SQLCHAR s0[ 32 ], s1[ 100 + LOG_MESSAGE_LEN ];
    SQLCHAR s2[ 100 + LOG_MESSAGE_LEN ];

    if ( rec_number < 1 )
    {
        return SQL_ERROR;
    }

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
\n\t\t\tSQLState = %p\
\n\t\t\tNative = %p\
\n\t\t\tMessage Text = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tText Len Ptr = %p",
                    environment,
                    rec_number,
                    sqlstate,
                    native,
                    message_text,
                    buffer_length,
                    text_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    environment -> msg );
        }

        ret = extract_sql_error_rec_w( &environment -> error,
                sqlstate,
                rec_number,
                native,
                message_text,
                buffer_length,
                text_length_ptr );

        if ( log_info.log_flag )
        {
            if ( SQL_SUCCEEDED( ret ))
            {
                char *ts1, *ts2;

                sprintf( environment -> msg, 
                    "\n\t\tExit:[%s]\
\n\t\t\tSQLState = %s\
\n\t\t\tNative = %s\
\n\t\t\tMessage Text = %s",
                        __get_return_status( ret, s2 ),
                        ts1 = unicode_to_ansi_alloc( sqlstate, SQL_NTS, NULL ),
                        __iptr_as_string( s0, native ),
                        __sdata_as_string( s1, SQL_CHAR, 
                            text_length_ptr, ts2 = unicode_to_ansi_alloc( message_text, SQL_NTS, NULL )));

                free( ts1 );
                free( ts2 );
            }
            else
            {
                sprintf( environment -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s2 ));
            }

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

				if ( CHECK_SQLGETDIAGRECW( parent_connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGRECW( parent_connection, 
							handle_type,
							connection, 
        					rec_number,
        					sqlstate,
        					native,
        					message_text,
        					buffer_length,
        					text_length_ptr );
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
\n\t\t\tSQLState = %p\
\n\t\t\tNative = %p\
\n\t\t\tMessage Text = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tText Len Ptr = %p",
                    connection,
                    rec_number,
                    sqlstate,
                    native,
                    message_text,
                    buffer_length,
                    text_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    connection -> msg );
        }

        ret = extract_sql_error_rec_w( &connection -> error,
                sqlstate,
                rec_number,
                native,
                message_text,
                buffer_length,
                text_length_ptr );

        if ( log_info.log_flag )
        {
            if ( SQL_SUCCEEDED( ret ))
            {
                char *ts1, *ts2;

                sprintf( connection -> msg, 
                    "\n\t\tExit:[%s]\
\n\t\t\tSQLState = %s\
\n\t\t\tNative = %s\
\n\t\t\tMessage Text = %s",
                        __get_return_status( ret, s2 ),
                        ts1 = unicode_to_ansi_alloc( sqlstate, SQL_NTS, connection ),
                        __iptr_as_string( s0, native ),
                        __sdata_as_string( s1, SQL_CHAR, 
                            text_length_ptr, ts2 = unicode_to_ansi_alloc( message_text, SQL_NTS, connection )));

                free( ts1 );
                free( ts2 );
            }
            else
            {
                sprintf( connection -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s2 ));
            }

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

				if ( CHECK_SQLGETDIAGRECW( parent_statement -> connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGRECW( parent_statement -> connection, 
							handle_type,
							statement,
        					rec_number,
        					sqlstate,
        					native,
        					message_text,
        					buffer_length,
        					text_length_ptr );
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
\n\t\t\tSQLState = %p\
\n\t\t\tNative = %p\
\n\t\t\tMessage Text = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tText Len Ptr = %p",
                    statement,
                    rec_number,
                    sqlstate,
                    native,
                    message_text,
                    buffer_length,
                    text_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    statement -> msg );
        }

        ret = extract_sql_error_rec_w( &statement -> error,
                sqlstate,
                rec_number,
                native,
                message_text,
                buffer_length,
                text_length_ptr );

        if ( log_info.log_flag )
        {
            if ( SQL_SUCCEEDED( ret ))
            {
                char *ts1, *ts2;

                sprintf( statement -> msg, 
                    "\n\t\tExit:[%s]\
\n\t\t\tSQLState = %s\
\n\t\t\tNative = %s\
\n\t\t\tMessage Text = %s",
                        __get_return_status( ret, s2 ),
                        ts1 = unicode_to_ansi_alloc( sqlstate, SQL_NTS, statement -> connection ),
                        __iptr_as_string( s0, native ),
                        __sdata_as_string( s1, SQL_CHAR, 
                            text_length_ptr, ts2 = unicode_to_ansi_alloc( message_text, SQL_NTS, statement -> connection )));

                free( ts1 );
                free( ts2 );
            }
            else
            {
                sprintf( statement -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s2 ));
            }

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

				if ( CHECK_SQLGETDIAGRECW( parent_desc -> connection ))
				{
        			dm_log_write( __FILE__, 
                		__LINE__, 
                   		 	LOG_INFO, 
                   		 	LOG_INFO, 
                   		 	"Info: calling redirected driver function" );

					return SQLGETDIAGRECW( parent_desc -> connection, 
							handle_type,
							descriptor,
        					rec_number,
        					sqlstate,
        					native,
        					message_text,
        					buffer_length,
        					text_length_ptr );
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
\n\t\t\tSQLState = %p\
\n\t\t\tNative = %p\
\n\t\t\tMessage Text = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tText Len Ptr = %p",
                    descriptor,
                    rec_number,
                    sqlstate,
                    native,
                    message_text,
                    buffer_length,
                    text_length_ptr );

            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    descriptor -> msg );
        }

        ret = extract_sql_error_rec_w( &descriptor -> error,
                sqlstate,
                rec_number,
                native,
                message_text,
                buffer_length,
                text_length_ptr );

        if ( log_info.log_flag )
        {
            if ( SQL_SUCCEEDED( ret ))
            {
                char *ts1, *ts2;

                sprintf( descriptor -> msg, 
                    "\n\t\tExit:[%s]\
\n\t\t\tSQLState = %s\
\n\t\t\tNative = %s\
\n\t\t\tMessage Text = %s",
                        __get_return_status( ret, s2 ),
                        ts1 = unicode_to_ansi_alloc( sqlstate, SQL_NTS, descriptor -> connection ),
                        __iptr_as_string( s0, native ),
                        __sdata_as_string( s1, SQL_CHAR, 
                            text_length_ptr, ts2 = unicode_to_ansi_alloc( message_text, SQL_NTS, descriptor -> connection )));

                free( ts1 );
                free( ts2 );
            }
            else
            {
                sprintf( descriptor -> msg, 
                    "\n\t\tExit:[%s]",
                        __get_return_status( ret, s2 ));
            }

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

