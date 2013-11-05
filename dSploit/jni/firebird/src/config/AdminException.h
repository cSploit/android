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

// AdminException.h: interface for the AdminException class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ADMINEXCEPTION_H__ED22B745_F780_416D_8291_0A2D49EF5B43__INCLUDED_)
#define AFX_ADMINEXCEPTION_H__ED22B745_F780_416D_8291_0A2D49EF5B43__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/fb_string.h"

class AdminException : public Firebird::GlobalStorage
{
public:
	AdminException(const char* txt, ...);
	virtual ~AdminException();
	void setLocation (const Firebird::PathName& fileName, int lineNumber);
	const char* getText() const;

private:
	Firebird::string	text;
	Firebird::PathName	fileName;
};

#endif // !defined(AFX_ADMINEXCEPTION_H__ED22B745_F780_416D_8291_0A2D49EF5B43__INCLUDED_)
