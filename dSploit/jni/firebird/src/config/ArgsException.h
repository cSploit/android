/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */
// ArgsException.h: interface for the ArgsException class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ARGSEXCEPTION_H__0546C1B7_E895_4AC1_A951_AF1812A505B0__INCLUDED_)
#define AFX_ARGSEXCEPTION_H__0546C1B7_E895_4AC1_A951_AF1812A505B0__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/fb_string.h"

class ArgsException : public Firebird::GlobalStorage
{
public:
	ArgsException(const char* txt, ...);
	virtual ~ArgsException();
	const char* getText() const;

private:
	Firebird::string	text;
};

#endif // !defined(AFX_ARGSEXCEPTION_H__0546C1B7_E895_4AC1_A951_AF1812A505B0__INCLUDED_)
