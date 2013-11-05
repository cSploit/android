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

// ScanDir.h: interface for the ScanDir class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_SCANDIR_H__D3AE9FF3_1295_11D6_B8F7_00E0180AC49E__INCLUDED_)
#define AFX_SCANDIR_H__D3AE9FF3_1295_11D6_B8F7_00E0180AC49E__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#ifdef _WIN32
#ifndef _WINDOWS_
#undef ERROR
#include <windows.h>
#endif
#else
#include <sys/types.h>
#include <dirent.h>
#endif

#include "../common/classes/fb_string.h"

class ScanDir : public Firebird::GlobalStorage
{
public:
	ScanDir(const char *dir, const char *pattern);
	virtual ~ScanDir();

	bool isDots();
	const char* getFilePath();
	bool isDirectory();
	static bool match (const char *pattern, const char *name);
	const char* getFileName();
	bool next();

	Firebird::PathName	directory;
	Firebird::PathName	pattern;
	Firebird::PathName	fileName;
	Firebird::PathName	filePath;
private:
#ifdef _WIN32
	WIN32_FIND_DATA	data;
	HANDLE			handle;
#else
	DIR				*dir;
	dirent			*data;
#endif
};

#endif // !defined(AFX_SCANDIR_H__D3AE9FF3_1295_11D6_B8F7_00E0180AC49E__INCLUDED_)
