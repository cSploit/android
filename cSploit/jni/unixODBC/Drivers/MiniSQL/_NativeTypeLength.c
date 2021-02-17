/**************************************************
 * _NativeTypeLength
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

int _NativeTypeLength( void *pNativeColumnHeader )
{
	m_field	*pField	= (m_field*)pNativeColumnHeader;
	int nLength;

	/****************************
	 * DEFAULT
	 ***************************/
	nLength = pField->length;

	/****************************
	 * NON-DEFAULT
	 ***************************/
	switch ( pField->type )
	{
	case MONEY_TYPE:
		return 16;

	case INT_TYPE:
		return 5;

	case UINT_TYPE:
        return 5;

	case TIME_TYPE:
		return nLength;

	case DATE_TYPE:
		return nLength;

	case REAL_TYPE:		
		return nLength;

	case CHAR_TYPE:
		return nLength;

	case TEXT_TYPE:
		return nLength;

	case IDENT_TYPE:
		return nLength;

	default:
		return nLength;
	}

	return nLength;
}




