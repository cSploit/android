/*********************************************************************
 *
 * unixODBC Cursor Library
 *
 * Created by Nick Gorham
 * (nick@lurcher.org).
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
 * $Id: SQLAllocStmt.c,v 1.3 2009/02/18 17:59:17 lurcher Exp $
 *
 * $Log: SQLAllocStmt.c,v $
 * Revision 1.3  2009/02/18 17:59:17  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.2  2002/11/19 18:52:28  lurcher
 *
 * Alter the cursor lib to not require linking to the driver manager.
 *
 * Revision 1.1.1.1  2001/10/17 16:40:15  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.2  2001/04/12 17:43:36  nick
 *
 * Change logging and added autotest to odbctest
 *
 * Revision 1.1.1.1  2000/09/04 16:42:52  nick
 * Imported Sources
 *
 * Revision 1.2  1999/11/20 20:54:00  ngorham
 *
 * Asorted portability fixes
 *
 * Revision 1.1  1999/09/19 22:22:50  ngorham
 *
 *
 * Added first cursor library work, read only at the moment and only works
 * with selects with no where clause
 *
 *
 **********************************************************************/

#include <config.h>
#include "cursorlibrary.h"

SQLRETURN CLAllocStmt( SQLHDBC connection_handle,
           SQLHSTMT *statement_handle,
           SQLHANDLE dm_handle ) 
{
    CLHDBC cl_connection = (CLHDBC) connection_handle;
    CLHSTMT cl_statement;
    DMHDBC connection = cl_connection -> dm_connection;
    SQLRETURN ret;

    /*
     * allocate a cursor lib statement
     */

    cl_statement = malloc( sizeof( *cl_statement ));

    if ( !cl_statement )
    {
        cl_connection -> dh.dm_log_write( "CL " __FILE__,
                __LINE__,
                LOG_INFO,
                LOG_INFO,
                "Error: IM001" );

        cl_statement -> cl_connection -> dh.__post_internal_error( &connection -> error,
                ERROR_HY001, NULL,
                connection -> environment -> requested_version );

        return SQL_ERROR;
    }

    memset( cl_statement, 0, sizeof( *cl_statement ));

    cl_statement -> cl_connection = cl_connection;
    cl_statement -> dm_statement = ( DMHSTMT ) dm_handle; 

    ret = SQLALLOCSTMT( cl_connection, 
            cl_connection -> driver_dbc,
            &cl_statement -> driver_stmt,
            NULL );

    if ( SQL_SUCCEEDED( ret ))
    {
        *statement_handle = ( SQLHSTMT ) cl_statement;
    }
    else
    {
        free( cl_statement );
    }
    return ret;
}
