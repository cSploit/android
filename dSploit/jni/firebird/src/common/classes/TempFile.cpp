/*
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>

#if defined(WIN_NT)
#include <fcntl.h>
#include <io.h>
#include <process.h>
#include <share.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/timeb.h>
#include <windows.h>
#ifndef INVALID_SET_FILE_POINTER
#define INVALID_SET_FILE_POINTER	((DWORD)-1)
#endif
#elif defined(HAVE_UNISTD_H)
#include <unistd.h>
#endif

#include "../jrd/gdsassert.h"
#include "../jrd/os/path_utils.h"
#include "../common/classes/init.h"

#include "../common/classes/TempFile.h"

namespace Firebird {

// Const definitions

static const char* ENV_VAR = "FIREBIRD_TMP";
static const char* DEFAULT_PATH =
#if defined(UNIX)
	"/tmp/";
#elif defined(WIN_NT)
	"c:\\temp\\";
#else
	NULL;
#endif

static const char* const NAME_PATTERN = "XXXXXX";
static const char* const NAME_LETTERS = "abcdefghijklmnopqrstuvwxyz0123456789";
static const size_t MAX_TRIES = 256;

// we need a class here only to return memory on shutdown and avoid
// false memory leak reports
static Firebird::InitInstance<ZeroBuffer> zeros;

//
// TempFile::getTempPath
//
// Returns a pathname to the system temporary directory
//

PathName TempFile::getTempPath()
{
	const char* env_temp = getenv(ENV_VAR);
	PathName path = env_temp ? env_temp : "";
	if (path.empty())
	{
#if defined(WIN_NT)
		char temp_dir[MAXPATHLEN];
		// this checks "TEMP" and "TMP" environment variables
		const int len = GetTempPath(sizeof(temp_dir), temp_dir);
		if (len && len < sizeof(temp_dir))
		{
			path = temp_dir;
		}
#else
		env_temp = getenv("TMP");
		path = env_temp ? env_temp : "";
#endif
	}
	if (path.empty())
	{
		path = DEFAULT_PATH;
	}

	fb_assert(path.length());
	return path;
}

//
// TempFile::create
//
// Creates a temporary file and returns its name
//

PathName TempFile::create(const PathName& prefix, const PathName& directory)
{
	PathName filename;

	try {
		TempFile file(*getDefaultMemoryPool(), prefix, directory, false);
		filename = file.getName();
	}
	catch (const Exception&)
	{} // do nothing

	return filename;
}

//
// TempFile::init
//
// Creates temporary file with a unique filename
//

void TempFile::init(const PathName& directory, const PathName& prefix)
{
	// set up temporary directory, if not specified
	filename = directory;
	if (filename.empty())
	{
		filename = getTempPath();
	}
	PathUtils::ensureSeparator(filename);

#if defined(WIN_NT)
	_timeb t;
	_ftime(&t);
	__int64 randomness = t.time;
	randomness *= 1000;
	randomness += t.millitm;
	PathName suffix = NAME_PATTERN;
	for (int tries = 0; tries < MAX_TRIES; tries++)
	{
		PathName name = filename + prefix;
		__int64 temp = randomness;
		for (size_t i = 0; i < suffix.length(); i++)
		{
			suffix[i] = NAME_LETTERS[temp % (strlen(NAME_LETTERS))];
			temp /= strlen(NAME_LETTERS);
		}
		name += suffix;
		DWORD attributes = FILE_ATTRIBUTE_NORMAL | FILE_ATTRIBUTE_TEMPORARY;
		if (doUnlink)
		{
			attributes |= FILE_FLAG_DELETE_ON_CLOSE;
		}
		handle = CreateFile(name.c_str(), GENERIC_READ | GENERIC_WRITE,
							0, NULL, CREATE_NEW, attributes, NULL);
		if (handle != INVALID_HANDLE_VALUE)
		{
			filename = name;
			break;
		}
		randomness++;
	}
	if (handle == INVALID_HANDLE_VALUE)
	{
		system_error::raise("CreateFile");
	}
#else
	filename += prefix;
	filename += NAME_PATTERN;

#ifdef HAVE_MKSTEMP
	handle = (IPTR) mkstemp(filename.begin());
#else
	if (!mktemp(filename.begin()))
	{
		system_error::raise("mktemp");
	}

	do {
		handle = open(filename.c_str(), O_RDWR | O_EXCL | O_CREAT);
	} while (handle == -1 && errno == EINTR);
#endif

	if (handle == -1)
	{
		system_error::raise("open");
	}

	if (doUnlink)
	{
		::unlink(filename.c_str());
	}
#endif

	doUnlink = false;
}

//
// TempFile::~TempFile
//
// Destructor
//

TempFile::~TempFile()
{
#if defined(WIN_NT)
	CloseHandle(handle);
#else
	::close(handle);
#endif
	if (doUnlink)
	{
		::unlink(filename.c_str());
	}
}

//
// TempFile::seek
//
// Performs a positioning operation
//

void TempFile::seek(const offset_t offset)
{
	if (position == offset)
		return;

#if defined(WIN_NT)
	LARGE_INTEGER liOffset;
	liOffset.QuadPart = offset;
	const DWORD seek_result =
		SetFilePointer(handle, (LONG) liOffset.LowPart, &liOffset.HighPart, FILE_BEGIN);
	if (seek_result == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
	{
		system_error::raise("SetFilePointer");
	}
#else
	const off_t seek_result = ::lseek(handle, (off_t) offset, SEEK_SET);
	if (seek_result == (off_t) -1)
	{
		system_error::raise("lseek");
	}
#endif
	position = offset;
	if (position > size)
		size = position;
}

//
// TempFile::extend
//
// Increases the file size
//

void TempFile::extend(size_t delta)
{
	const char* const buffer = zeros().getBuffer();
	const size_t bufferSize = zeros().getSize();
	const offset_t newSize = size + delta;

	for (offset_t offset = size; offset < newSize; offset += bufferSize)
	{
		const size_t length = MIN(newSize - offset, bufferSize);
		write(offset, buffer, length);
	}
}

//
// TempFile::read
//
// Reads bytes from file
//

size_t TempFile::read(offset_t offset, void* buffer, size_t length)
{
	fb_assert(offset + length <= size);
	seek(offset);
#if defined(WIN_NT)
	DWORD bytes = 0;
	if (!ReadFile(handle, buffer, length, &bytes, NULL) || bytes != length)
	{
		system_error::raise("ReadFile");
	}
#else
	const int bytes = ::read(handle, buffer, length);
	if (bytes < 0 || size_t(bytes) != length)
	{
		system_error::raise("read");
	}
#endif
	position += bytes;
	return bytes;
}

//
// TempFile::write
//
// Writes bytes to file
//

size_t TempFile::write(offset_t offset, const void* buffer, size_t length)
{
	fb_assert(offset <= size);
	seek(offset);
#if defined(WIN_NT)
	DWORD bytes = 0;
	if (!WriteFile(handle, buffer, length, &bytes, NULL) || bytes != length)
	{
		system_error::raise("WriteFile");
	}
#else
	const int bytes = ::write(handle, buffer, length);
	if (bytes < 0 || size_t(bytes) != length)
	{
		system_error::raise("write");
	}
#endif
	position += bytes;
	if (position > size)
		size = position;
	return bytes;
}

//
// TempFile::unlink
//
// Unlinks the file
//

void TempFile::unlink()
{
#if defined(WIN_NT)
	doUnlink = true;
#else
	::unlink(filename.c_str());
#endif
}

}	// namespace Firebird
