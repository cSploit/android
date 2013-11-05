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
 *  Copyright (c) 2004 James A. Starkey
 *  All Rights Reserved.
 */

#ifndef VULCAN_PATHNAME_H_
#define VULCAN_PATHNAME_H_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/fb_string.h"


class PathName : public Firebird::GlobalStorage
{
public:
	PathName();
	virtual ~PathName();

	static const char* getWorkingDirectory();
	static int findWorkingDirectory(int dpbLength, const UCHAR* dpb, int bufferLength, char* buffer);
	static Firebird::string expandFilename(const char* fileName, int dpbLength, const UCHAR* dpb);
	static Firebird::string expandFilename(const char* fileName, const char* workingDirectory);
	static int merge(const char* fileName, const char* workingDirectory, int bufferLength, char* buffer);
	static bool isAbsolute(const char* fileName);
	static char* copyCanonical(const char* fileName, char* buffer, const char* endBuffer);
	static Firebird::string expandFilename(const char* fileName);
	static bool hasDirectory(const char* fileName);
	static bool pathsEquivalent(const char* path1, const char* path2);
};

#endif	// VULCAN_PATHNAME_H_
