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

#include <string.h>
#include <stdlib.h>
#include <memory.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "../jrd/ibase.h"
#include "PathName.h"

#ifdef _WIN32
#include <windows.h>
#define SEPARATOR		'\\'
#define IS_SEPARATOR(c)	(c == '/' || c == '\\')
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

#ifndef SEPARATOR
#define SEPARATOR		'/'
#define IS_SEPARATOR(c)	(c == '/')
#endif

#define IS_LETTER(c)	((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
//#define UPPER(c)		((c >= 'a' && c <= 'z') ? c - 'a' + 'A' : c)

PathName::PathName()
{
}

PathName::~PathName()
{
}

const char* PathName::getWorkingDirectory()
{
	static char workingDirectory [MAXPATHLEN];

#ifdef _WIN32
	GetCurrentDirectory(sizeof (workingDirectory), workingDirectory);
#else
	if (workingDirectory [0])
		return workingDirectory;

#ifdef HAVE_GETCWD
	getcwd(workingDirectory, sizeof (workingDirectory));
#else
	getwd(workingDirectory);
#endif

#endif

	return workingDirectory;
}

int PathName::findWorkingDirectory(int dpbLength, const UCHAR* dpb, int bufferLength, char* buffer)
{
	const UCHAR* p = dpb;
	const UCHAR* const end = dpb + dpbLength;

	if (dpbLength <= 0 || *p++ != isc_dpb_version1)
		return 0;

	for (int length = 0; p < end; p += length)
	{
		const UCHAR verb = *p++;
		length = *p++;
		length += (*p++) << 8;

		if (verb == isc_dpb_working_directory)
		{
			const int l = MIN (bufferLength - 1, length);
			memcpy (buffer, p, l);
			buffer [l] = 0;
			return length;
		}
	}

	return 0;
}

Firebird::string PathName::expandFilename(const char* fileName, int dpbLength, const UCHAR* dpb)
{
	char workingDirectory [MAXPATHLEN];
	const char *directory;

	if (findWorkingDirectory (dpbLength, dpb, sizeof (workingDirectory), workingDirectory))
		directory = workingDirectory;
	else
		directory = getWorkingDirectory();

	return expandFilename (fileName, directory);
}

Firebird::string PathName::expandFilename(const char* fileName, const char* workingDirectory)
{
	char buffer [MAXPATHLEN];
	int length = merge (fileName, workingDirectory, sizeof (buffer), buffer);

#ifdef _WIN32
	for (char *p = buffer; *p; ++p)
	{
		if (*p == '/')
			*p = SEPARATOR;
	}
#endif

	return Firebird::string(buffer, length);
}

int PathName::merge(const char* fileName, const char* workingDirectory, int bufferLength, char* buffer)
{
	const char* const endBuffer = buffer + bufferLength - 1;

	if (isAbsolute (fileName))
	{
		char* rc = copyCanonical (fileName, buffer, endBuffer);
		return rc - buffer;
	}

	// Copy working directory, making slashes canonical

	char* q = copyCanonical (workingDirectory, buffer, endBuffer);

#ifdef _WIN32
	if (IS_SEPARATOR (fileName [0]))
	{
		for (q = buffer; *q && *q++ != ':';)
			;
		q = copyCanonical (fileName, q, endBuffer);
		return q - buffer;
	}
#endif

	// And add a trailing slash, if necessary

	if (q == buffer || q [-1] != '/')
		*q++ = '/';

	// Handle self relative segments

	const char* p = fileName;
	while (*p == '.')
	{
		if (IS_SEPARATOR (p[1]))
			p += 2;
		else if (p [1] == '.' && IS_SEPARATOR (p [2]))
		{
			p += 3;
			q -= 1;
			while (q > buffer && q [-1] != '/')
				--q;
			if (q == buffer || q [-1] != '/')
				*q++ = '/';
		}
		else if (!p [1])
			++p;
	}

	// skip over extra separators in the file name

	while (IS_SEPARATOR (*p))
		++p;

	// and move in the final filename

	q = copyCanonical (p, q, endBuffer);

	return q - buffer;
}

bool PathName::isAbsolute(const char* fileName)
{
#ifdef _WIN32
	return (IS_LETTER (fileName [0]) &&
			 fileName [1] == ':' && (fileName [2] == '/' || fileName [2] == '\\')) ||
		   (fileName[0] == '/' && fileName[1] == '/') ||
		   (fileName[0] == '\\' && fileName[1] == '\\');


#else
	return fileName [0] == '/';
#endif
}

char* PathName::copyCanonical(const char* fileName, char* buffer, const char* endBuffer)
{
	char* q = buffer;
	const char* p = fileName;

#ifdef _WIN32
	if (IS_SEPARATOR (*p))
	{
		*q++ = '/';
		++p;
	}
#endif

	while (*p && q < endBuffer)
	{
		char c = *p++;
		if (IS_SEPARATOR (c))
		{
			c = '/';
			while (IS_SEPARATOR (*p))
				++p;
		}
		*q++ = c;
	}

	*q = 0;

	return q;
}

Firebird::string PathName::expandFilename(const char* fileName)
{
	return expandFilename (fileName, getWorkingDirectory());
}

bool PathName::hasDirectory(const char* fileName)
{
	for (const char* p = fileName; *p; ++p)
	{
		if (IS_SEPARATOR (*p))
			return true;
	}

	return false;
}

bool PathName::pathsEquivalent(const char* path1, const char* path2)
{
#ifdef _WIN32
	return stricmp(path1, path2) == 0;
#else
	return strcmp (path1, path2) == 0;
#endif
}
