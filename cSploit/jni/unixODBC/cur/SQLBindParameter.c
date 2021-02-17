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
 * $Id: SQLBindParameter.c,v 1.3 2009/02/18 17:59:17 lurcher Exp $
 *
 * $Log: SQLBindParameter.c,v $
 * Revision 1.3  2009/02/18 17:59:17  lurcher
 * Shift to using config.h, the compile lines were making it hard to spot warnings
 *
 * Revision 1.2  2007/11/13 15:04:57  lurcher
 * Fix 64 bit cursor lib issues
 *
 * Revision 1.1.1.1  2001/10/17 16:40:15  lurcher
 *
 * First upload to SourceForge
 *
 * Revision 1.1.1.1  2000/09/04 16:42:52  nick
 * Imported Sources
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

SQLRETURN CLBindParameter(
    SQLHSTMT           statement_handle,
    SQLUSMALLINT       ipar,
    SQLSMALLINT        f_param_type,
    SQLSMALLINT        f_c_type,
    SQLSMALLINT        f_sql_type,
    SQLULEN            cb_col_def,
    SQLSMALLINT        ib_scale,
    SQLPOINTER         rgb_value,
    SQLLEN             cb_value_max,
    SQLLEN             *pcb_value )
{
    CLHSTMT cl_statement = (CLHSTMT) statement_handle; 

    return SQLBINDPARAMETER( cl_statement -> cl_connection,
            cl_statement -> driver_stmt,
            ipar,
            f_param_type,
            f_c_type,
            f_sql_type,
            cb_col_def,
            ib_scale,
            rgb_value,
            cb_value_max,
            pcb_value );
}
