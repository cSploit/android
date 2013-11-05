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
 * $Id: SQLGetDescField.c,v 1.10 2009/02/18 17:59:08 lurcher Exp $
 *
 * $Log: SQLGetDescField.c,v $
 * Revision 1.10  2009/02/18 17:59:08  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.9  2008/09/29 14:02:45  lurcher
 * Fix missing dlfcn group option
 *
 * Revision 1.8  2006/03/08 09:18:41  lurcher
 * fix silly typo that was using sizeof( SQL_WCHAR ) instead of SQLWCHAR
 *
 * Revision 1.7  2004/11/22 17:02:49  lurcher
 * Fix unicode/ansi conversion in the SQLGet functions
 *
 * Revision 1.6  2003/10/30 18:20:46  lurcher
 *
 * Fix broken thread protection
 * Remove SQLNumResultCols after execute, lease S4/S% to driver
 * Fix string overrun in SQLDriverConnect
 * Add initial support for Interix
 *
 * Revision 1.5  2003/02/27 12:19:39  lurcher
 *
 * Add the A functions as well as the W
 *
 * Revision 1.4  2002/12/05 17:44:30  lurcher
 *
 * Display unknown return values in return logging
 *
 * Revision 1.3  2002/07/24 08:49:52  lurcher
 *
 * Alter UNICODE support to use iconv for UNICODE-ANSI conversion
 *
 * Revision 1.2  2002/01/21 18:00:51  lurcher
 *
 * Assorted fixed and changes, mainly UNICODE/bug fixes
 *
 * Revision 1.1.1.1  2001/10/17 16:40:05  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.5  2001/07/03 09:30:41  nick
 *
 * Add ability to alter size of displayed message in the log
 *
 * Revision 1.4  2001/04/17 16:29:39  nick
 *
 * More checks and autotest fixes
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
 * Revision 1.7  1999/11/13 23:40:59  ngorham
 *
 * Alter the way DM logging works
 * Upgrade the Postgres driver to 6.4.6
 *
 * Revision 1.6  1999/10/24 23:54:18  ngorham
 *
 * First part of the changes to the error reporting
 *
 * Revision 1.5  1999/09/21 22:34:25  ngorham
 *
 * Improve performance by removing unneeded logging calls when logging is
 * disabled
 *
 * Revision 1.4  1999/07/10 21:10:16  ngorham
 *
 * Adjust error sqlstate from driver manager, depending on requested
 * version (ODBC2/3)
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
 * Revision 1.3  1999/05/04 22:41:12  nick
 * and another night ends
 *
 * Revision 1.2  1999/05/03 19:50:43  nick
 * Another check point
 *
 * Revision 1.1  1999/04/25 23:06:11  nick
 * Initial revision
 *
 *
 **********************************************************************/

#include <config.h>
#include "drivermanager.h"

static char const rcsid[]= "$RCSfile: SQLGetDescField.c,v $ $Revision: 1.10 $";

SQLRETURN SQLGetDescFieldA( SQLHDESC descriptor_handle,
           SQLSMALLINT rec_number, 
           SQLSMALLINT field_identifier,
           SQLPOINTER value, 
           SQLINTEGER buffer_length,
           SQLINTEGER *string_length )
{
    return SQLGetDescField( descriptor_handle,
                        rec_number, 
                        field_identifier,
                        value, 
                        buffer_length,
                        string_length );
}

SQLRETURN SQLGetDescField( SQLHDESC descriptor_handle,
           SQLSMALLINT rec_number, 
           SQLSMALLINT field_identifier,
           SQLPOINTER value, 
           SQLINTEGER buffer_length,
           SQLINTEGER *string_length )
{
    /*
     * not quite sure how the descriptor can be
     * allocated to a statement, all the documentation talks
     * about state transitions on statement states, but the
     * descriptor may be allocated with more than one statement
     * at one time. Which one should I check ?
     */
    DMHDESC descriptor = (DMHDESC) descriptor_handle;
    SQLRETURN ret;
    SQLCHAR s1[ 100 + LOG_MESSAGE_LEN ];

    /*
     * check descriptor
     */

    if ( !__validate_desc( descriptor ))
    {
        dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: SQL_INVALID_HANDLE" );

        return SQL_INVALID_HANDLE;
    }

    function_entry( descriptor );

    if ( log_info.log_flag )
    {
        sprintf( descriptor -> msg, "\n\t\tEntry:\
\n\t\t\tDescriptor = %p\
\n\t\t\tRec Number = %d\
\n\t\t\tField Attr = %s\
\n\t\t\tValue = %p\
\n\t\t\tBuffer Length = %d\
\n\t\t\tStrLen = %p",
                descriptor,
                rec_number,
                __desc_attr_as_string( s1, field_identifier ),
                value, 
                (int)buffer_length,
                (void*)string_length );

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                descriptor -> msg );
    }

    thread_protect( SQL_HANDLE_DESC, descriptor );

    if ( descriptor -> connection -> state < STATE_C4 )
    {
        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY010" );

        __post_internal_error( &descriptor -> error,
                ERROR_HY010, NULL,
                descriptor -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_DESC, descriptor, SQL_ERROR );
    }

    /*
     * check status of statements associated with this descriptor
     */

    if( __check_stmt_from_desc( descriptor, STATE_S8 ) ||
        __check_stmt_from_desc( descriptor, STATE_S9 ) ||
        __check_stmt_from_desc( descriptor, STATE_S10 ) ||
        __check_stmt_from_desc( descriptor, STATE_S11 ) ||
        __check_stmt_from_desc( descriptor, STATE_S12 )) {

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY010" );

        __post_internal_error( &descriptor -> error,
                ERROR_HY010, NULL,
                descriptor -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_DESC, descriptor, SQL_ERROR );
    }

    if( __check_stmt_from_desc_ird( descriptor, STATE_S1 )) {

        dm_log_write( __FILE__, 
                __LINE__, 
                LOG_INFO, 
                LOG_INFO, 
                "Error: HY007" );

        __post_internal_error( &descriptor -> error,
                ERROR_HY007, NULL,
                descriptor -> connection -> environment -> requested_version );

        return function_return( SQL_HANDLE_DESC, descriptor, SQL_ERROR );
    }

    if ( descriptor -> connection -> unicode_driver )
    {
        SQLWCHAR *s1 = NULL;

        if ( !CHECK_SQLGETDESCFIELDW( descriptor -> connection ))
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: IM001" );

            __post_internal_error( &descriptor -> error,
                    ERROR_IM001, NULL,
                    descriptor -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_DESC, descriptor, SQL_ERROR );
        }

        switch( field_identifier )
        {
          case SQL_DESC_BASE_COLUMN_NAME:
          case SQL_DESC_BASE_TABLE_NAME:
          case SQL_DESC_CATALOG_NAME:
          case SQL_DESC_LABEL:
          case SQL_DESC_LITERAL_PREFIX:
          case SQL_DESC_LITERAL_SUFFIX:
          case SQL_DESC_LOCAL_TYPE_NAME:
          case SQL_DESC_NAME:
          case SQL_DESC_SCHEMA_NAME:
          case SQL_DESC_TABLE_NAME:
          case SQL_DESC_TYPE_NAME:
            if ( buffer_length > 0 && value )
            {
                s1 = malloc( sizeof( SQLWCHAR ) * ( buffer_length + 1 ));
            }
            break;
        }

        ret = SQLGETDESCFIELDW( descriptor -> connection,
                descriptor -> driver_desc,
                rec_number, 
                field_identifier,
                s1 ? s1 : value, 
                buffer_length,
                string_length );

        switch( field_identifier )
        {
          case SQL_DESC_BASE_COLUMN_NAME:
          case SQL_DESC_BASE_TABLE_NAME:
          case SQL_DESC_CATALOG_NAME:
          case SQL_DESC_LABEL:
          case SQL_DESC_LITERAL_PREFIX:
          case SQL_DESC_LITERAL_SUFFIX:
          case SQL_DESC_LOCAL_TYPE_NAME:
          case SQL_DESC_NAME:
          case SQL_DESC_SCHEMA_NAME:
          case SQL_DESC_TABLE_NAME:
          case SQL_DESC_TYPE_NAME:
            if ( SQL_SUCCEEDED( ret ) && value && s1 )
            {
                unicode_to_ansi_copy( value, buffer_length, s1, SQL_NTS, descriptor -> connection );
            }
			if ( SQL_SUCCEEDED( ret ) && string_length ) 
			{
				*string_length /= sizeof( SQLWCHAR );	
			}
            break;
        }

        if ( s1 )
        {
            free( s1 );
        }
    }
    else
    {
        if ( !CHECK_SQLGETDESCFIELD( descriptor -> connection ))
        {
            dm_log_write( __FILE__, 
                    __LINE__, 
                    LOG_INFO, 
                    LOG_INFO, 
                    "Error: IM001" );

            __post_internal_error( &descriptor -> error,
                    ERROR_IM001, NULL,
                    descriptor -> connection -> environment -> requested_version );

            return function_return( SQL_HANDLE_DESC, descriptor, SQL_ERROR );
        }

        ret = SQLGETDESCFIELD( descriptor -> connection,
                descriptor -> driver_desc,
                rec_number, 
                field_identifier,
                value, 
                buffer_length,
                string_length );
    }

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

    return function_return( SQL_HANDLE_DESC, descriptor, ret );
}
