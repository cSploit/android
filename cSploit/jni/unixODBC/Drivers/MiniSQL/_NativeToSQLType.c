/**************************************************
 * _NativeToSQLType
 *
 **************************************************
 * This code was created by Peter Harvey @ CodeByDesign.
 * Released under LGPL 31.JAN.99
 *
 * Contributions from...
 * -----------------------------------------------
 * Peter Harvey		- pharvey@codebydesign.com
 **************************************************/
#include <config.h>
#include "driver.h"

int _NativeToSQLType( void *pNativeColumnHeader )
{
	m_field	*pField	= (m_field*)pNativeColumnHeader;

	switch ( pField->type )
	{
	case MONEY_TYPE:
        return SQL_REAL;

	case INT_TYPE:
        return SQL_INTEGER;

	case UINT_TYPE:
        return SQL_INTEGER;

	case TIME_TYPE:
        return SQL_CHAR;

	case DATE_TYPE:
        return SQL_CHAR;

	case REAL_TYPE:		
        return SQL_REAL; 	

	case CHAR_TYPE:
        return SQL_CHAR;

	case TEXT_TYPE:
		return SQL_CHAR;

	case IDENT_TYPE:
		return SQL_CHAR;

	default:
		return SQL_UNKNOWN_TYPE;
	}


	return SQL_CHAR;
}




