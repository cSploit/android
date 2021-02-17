/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		win9x_nt.h
 *	DESCRIPTION:	Windows 9X IO support module
 *
 *  This file needs to be included in winnt.cpp. It is designed to
 *  mimic Windows NT overlapped API when application runs on Win9X.
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Nikolay Samofatov <skidder at users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

// This file needs to be explicitly included in winnt.cpp


#include "../common/classes/fb_pair.h"

namespace Jrd {

using Firebird::MutexLockGuard;

Firebird::Mutex file_locks_mutex;

struct LocksArrayItem {
	HANDLE first;
	Firebird::Mutex* second;

	LocksArrayItem() {}
	LocksArrayItem(HANDLE handle, Firebird::Mutex* mutex) : first(handle), second(mutex) { }
	static HANDLE generate(LocksArrayItem value) {
		return value.first;
	}
};

Firebird::SortedArray<
	LocksArrayItem,
	Firebird::InlineStorage<LocksArrayItem, 16>,
	HANDLE,
	LocksArrayItem > file_locks_9X(*getDefaultMemoryPool());

// This header can be included in winnt.h
// It converts Win32 overlapped API calls to synchronized regular API calls
HANDLE CreateFile_9X(
  LPCSTR lpFileName,
  DWORD dwDesiredAccess,
  DWORD dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD dwCreationDisposition,
  DWORD dwFlagsAndAttributes,
  HANDLE hTemplateFile)
{
	if (!ISC_is_WinNT())
		dwShareMode &= ~FILE_SHARE_DELETE;

	HANDLE file = CreateFileA(
		lpFileName,
		dwDesiredAccess,
		dwShareMode,
		lpSecurityAttributes,
		dwCreationDisposition,
		dwFlagsAndAttributes,
		hTemplateFile);

    DWORD dwLastError = GetLastError();

    if (!ISC_is_WinNT() && file != INVALID_HANDLE_VALUE) {
        MutexLockGuard sync(file_locks_mutex);
        file_locks_9X.add(
            LocksArrayItem(
                file,
                new Firebird::Mutex()));
    }

    SetLastError(dwLastError);
    return file;
}

BOOL CloseHandle_9X(HANDLE hObject)
{
	if (!ISC_is_WinNT()) {
		MutexLockGuard sync(file_locks_mutex);
		size_t pos;
		if (file_locks_9X.find(hObject, pos)) {
			delete file_locks_9X[pos].second;
			file_locks_9X.remove(pos);
		}
	}

	return CloseHandle(hObject);
}

BOOL ReadFile_9X(
  HANDLE hFile,
  LPVOID lpBuffer,
  DWORD nNumberOfBytesToRead,
  LPDWORD lpNumberOfBytesRead,
  LPOVERLAPPED lpOverlapped)
{
	if (ISC_is_WinNT())
		return ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, lpOverlapped);

	Firebird::Mutex* fileMutex = NULL;
	{
		MutexLockGuard sync(file_locks_mutex);
		size_t pos;
		if (file_locks_9X.find(hFile, pos))
			fileMutex = file_locks_9X[pos].second;
	}

	BOOL result;
    DWORD dwLastError;
	{
		MutexLockGuard sync(*fileMutex);

		if (lpOverlapped != NULL) {
			const DWORD ret = SetFilePointer(hFile, lpOverlapped->Offset,
				(PLONG)&lpOverlapped->OffsetHigh, FILE_BEGIN);
			if (ret == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
				return 0;
		}

		result = ReadFile(hFile, lpBuffer, nNumberOfBytesToRead, lpNumberOfBytesRead, NULL);
		dwLastError = GetLastError();
	}

    SetLastError(dwLastError);
	return result;
}

BOOL WriteFile_9X(
  HANDLE hFile,
  LPCVOID lpBuffer,
  DWORD nNumberOfBytesToWrite,
  LPDWORD lpNumberOfBytesWritten,
  LPOVERLAPPED lpOverlapped
)
{
	if (ISC_is_WinNT())
		return WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, lpOverlapped);

	Firebird::Mutex* fileMutex = NULL;
	{
		MutexLockGuard sync(file_locks_mutex);
		size_t pos;
		if (file_locks_9X.find(hFile, pos))
			fileMutex = file_locks_9X[pos].second;
	}

	BOOL result;
    DWORD dwLastError;
	{
		MutexLockGuard sync(*fileMutex);

		if (lpOverlapped != NULL) {
			const DWORD ret = SetFilePointer(hFile, lpOverlapped->Offset,
				(PLONG)&lpOverlapped->OffsetHigh, FILE_BEGIN);
			if (ret == INVALID_SET_FILE_POINTER && GetLastError() != NO_ERROR)
				return 0;
			lpOverlapped = NULL;
		}
		result = WriteFile(hFile, lpBuffer, nNumberOfBytesToWrite, lpNumberOfBytesWritten, NULL);
		dwLastError = GetLastError();
	}

    SetLastError(dwLastError);
	return result;
}


}

#if defined CreateFile
#undef CreateFile
#endif

#define CreateFile CreateFile_9X
#define CloseHandle CloseHandle_9X
#define ReadFile ReadFile_9X
#define WriteFile WriteFile_9X
