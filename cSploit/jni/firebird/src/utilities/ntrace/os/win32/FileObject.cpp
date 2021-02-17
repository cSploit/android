/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		FileObject.cpp
 *	DESCRIPTION:	Wrapper class for platform IO
 *
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include "../FileObject.h"
//#include "../common/classes/locks.h"

using namespace Firebird;
Firebird::Mutex open_mutex;

void FileObject::open(int flags, int pflags)
{
	MutexLockGuard guard(open_mutex, FB_FUNCTION);
	DWORD flagsAndAttributes = FILE_ATTRIBUTE_NORMAL;
	DWORD filecreate = 0;
	DWORD desiredAccess = 0;

	switch (flags & (fo_rdonly | fo_wronly | fo_rdwr))
	{
		case fo_rdonly:
			desiredAccess = GENERIC_READ;
			break;
		case fo_wronly:
			desiredAccess = GENERIC_WRITE;
			break;
		default:
			desiredAccess = GENERIC_READ | GENERIC_WRITE;
	}

	if (flags & fo_append)
	{
		// This is going to be tricky. Need to use global named mutex to achieve
		// multi-process happiness
		string temp(filename.c_str());
		for (string::size_type i = 0; i < temp.length(); i++)
		{
			switch (temp[i])
			{
				case '\\':
				case '/':
				case ':':
					temp[i] = '_';
			}
		}

		temp.append("_mutex");
		append_mutex = CreateMutex(NULL, FALSE, temp.c_str());
		if (append_mutex == NULL)
		{
			append_mutex = INVALID_HANDLE_VALUE;
			system_call_failed::raise("CreateMutex");
		}
	}

	// decode open/create method flags
	switch ( flags & (fo_creat | fo_excl | fo_trunc) )
	{
		case 0:
		case fo_excl:                   // ignore EXCL w/o CREAT
			filecreate = OPEN_EXISTING;
			break;

		case fo_creat:
			filecreate = OPEN_ALWAYS;
			break;

		case fo_creat | fo_excl:
		case fo_creat | fo_trunc | fo_excl:
			filecreate = CREATE_NEW;
			break;

		case fo_trunc:
		case fo_trunc | fo_excl:        // ignore EXCL w/o CREAT
			filecreate = TRUNCATE_EXISTING;
			break;

		case fo_creat | fo_trunc:
			filecreate = CREATE_ALWAYS;
			break;

		default:
			// this can't happen ... all cases are covered
			fb_assert(false);
	}

	if (flags & fo_temporary)
		flagsAndAttributes |= FILE_FLAG_DELETE_ON_CLOSE;

	if (flags & fo_short_lived)
		flagsAndAttributes |= FILE_ATTRIBUTE_TEMPORARY;

	if (flags & fo_sequential)
		flagsAndAttributes |= FILE_FLAG_SEQUENTIAL_SCAN;

	if (flags & fo_random)
		flagsAndAttributes |= FILE_FLAG_RANDOM_ACCESS;

	file = CreateFile(filename.c_str(),
		desiredAccess,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		filecreate,
		flagsAndAttributes, NULL);

	if (file == INVALID_HANDLE_VALUE)
		fatal_exception::raiseFmt("Error (%d) opening file: %s", GetLastError(), filename.c_str());
}

FileObject::~FileObject()
{
	CloseHandle(file);
	CloseHandle(append_mutex);
}

UINT64 FileObject::size()
{
	UINT64 nFileLen = 0;
	if (file != INVALID_HANDLE_VALUE)
	{
		LARGE_INTEGER size;
		if (GetFileSizeEx(file, &size))
			nFileLen = size.QuadPart;
	}
	return nFileLen;
}

void FileObject::reopen()
{
	CloseHandle(file);
	if (append_mutex != INVALID_HANDLE_VALUE)
	{
		ReleaseMutex(append_mutex);
		CloseHandle(append_mutex);
	}
	open(fo_rdwr | fo_append | fo_creat, 0);
	FlushFileBuffers(file);
}

size_t FileObject::blockRead(void* buffer, size_t bytesToRead)
{
	DWORD bytesDone;
	if (!ReadFile(file, buffer, bytesToRead, &bytesDone, NULL))
	{
		fatal_exception::raiseFmt("IO error (%d) reading file: %s",
			GetLastError(),
			filename.c_str());
	}

	return bytesDone;
}

void FileObject::blockWrite(const void* buffer, size_t bytesToWrite)
{
	if (append_mutex != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(append_mutex, INFINITE) != WAIT_OBJECT_0)
			system_call_failed::raise("WaitForSingleObject");
		seek(0, so_from_end);
	}

	DWORD bytesDone;
	if (!WriteFile(file, buffer, bytesToWrite, &bytesDone, NULL) ||
		bytesDone != bytesToWrite)
	{
		if (append_mutex != INVALID_HANDLE_VALUE)
			ReleaseMutex(append_mutex);
		fatal_exception::raiseFmt("IO error (%d) writing file: %s",
			GetLastError(),
			filename.c_str());
	}

	if (append_mutex != INVALID_HANDLE_VALUE)
		ReleaseMutex(append_mutex);
}

// Write data to header only if file is empty
void FileObject::writeHeader(const void* buffer, size_t bytesToWrite)
{
	if (append_mutex != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(append_mutex, INFINITE) != WAIT_OBJECT_0)
			system_call_failed::raise("WaitForSingleObject");
	}

	if (seek(0, so_from_end) != 0)
		return;

	DWORD bytesDone;
	if (!WriteFile(file, buffer, bytesToWrite, &bytesDone, NULL) ||
		bytesDone != bytesToWrite)
	{
		if (append_mutex != INVALID_HANDLE_VALUE)
			ReleaseMutex(append_mutex);

		fatal_exception::raiseFmt("IO error (%d) writing file: %s",
			GetLastError(),
			filename.c_str());
	}

	if (append_mutex != INVALID_HANDLE_VALUE)
		ReleaseMutex(append_mutex);
}

bool FileObject::renameFile(const Firebird::PathName new_filename)
{
	if (append_mutex != INVALID_HANDLE_VALUE)
	{
		if (WaitForSingleObject(append_mutex, INFINITE) != WAIT_OBJECT_0)
			system_call_failed::raise("WaitForSingleObject");
	}

	if (!MoveFile(filename.c_str(), new_filename.c_str()))
	{
		DWORD rename_err = GetLastError();
		if (rename_err == ERROR_ALREADY_EXISTS || rename_err == ERROR_FILE_NOT_FOUND)
		{
			// Another process renames our file just now. Open it again.
			reopen();
			return false;
		}

		if (append_mutex != INVALID_HANDLE_VALUE)
			ReleaseMutex(append_mutex);

		fatal_exception::raiseFmt("IO error (%d) renaming file: %s",
			rename_err,
			filename.c_str());
	}
	else
		reopen();

	return true;
}

SINT64 FileObject::seek(SINT64 newOffset, SeekOrigin origin)
{
	LARGE_INTEGER offset;
	offset.QuadPart = newOffset;
	DWORD moveMethod;

	switch(origin)
	{
		case so_from_beginning:
			moveMethod = FILE_BEGIN;
			break;
		case so_from_current:
			moveMethod = FILE_CURRENT;
			break;
		case so_from_end:
			moveMethod = FILE_END;
			break;
	}

	DWORD error;
	if ((offset.LowPart = SetFilePointer(file, offset.LowPart,
			&offset.HighPart, moveMethod)) == INVALID_SET_FILE_POINTER &&
		(error = GetLastError()) != NO_ERROR)
	{
		fatal_exception::raiseFmt("IO error (%d) seeking file: %s",
			error,
			filename.c_str());
	}

	return offset.QuadPart;
}
