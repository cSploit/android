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

// AdminException.cpp: implementation of the AdminException class.
//
//////////////////////////////////////////////////////////////////////

#include "firebird.h"
#include <stdio.h>
#include <stdarg.h>
#include "../common/classes/alloc.h"
#include "AdminException.h"


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

AdminException::AdminException(const char* txt, ...) :
	text(getPool()), fileName(getPool())
{
	va_list		args;
	va_start	(args, txt);
	text.vprintf(txt, args);
	va_end(args);
}

AdminException::~AdminException()
{

}

const char* AdminException::getText() const
{
	return text.c_str();
}

void AdminException::setLocation(const Firebird::PathName& file, int lineNumber)
{
	fileName = file;
	Firebird::string buffer;

	if (fileName.isEmpty())
		buffer.printf("line %d: %s", lineNumber, text.c_str());
	else
		buffer.printf("%s, line %d: %s", fileName.c_str(), lineNumber, text.c_str());
	text = buffer;
}
