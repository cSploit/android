/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		PluginLogWriter.cpp
 *	DESCRIPTION:	Plugin log writer implementation
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
*/

#include "PluginLogWriter.h"
#include "../common/classes/init.h"

using namespace Firebird;

// seems to only be Solaris 9 that doesn't have strerror_r,
// maybe we can remove this in the future
#ifndef HAVE_STRERROR_R
void strerror_r(int err, char* buf, size_t bufSize)
{
	static Firebird::GlobalPtr<Firebird::Mutex> mutex;
	Firebird::MutexLockGuard guard(mutex);
	strncpy(buf, strerror(err), bufSize);
}
#endif

PluginLogWriter::PluginLogWriter(const char* fileName, size_t maxSize) :
	m_fileName(*getDefaultMemoryPool()),
	m_fileHandle(-1),
	m_maxSize(maxSize)
{
	m_fileName = fileName;

#ifdef WIN_NT
	PathName mutexName("fb_mutex_");
	mutexName.append(m_fileName);

	checkMutex("init", ISC_mutex_init(&m_mutex, mutexName.c_str()));
#endif
}

PluginLogWriter::~PluginLogWriter()
{
	if (m_fileHandle != -1)
		::close(m_fileHandle);

#ifdef WIN_NT
	ISC_mutex_fini(&m_mutex);
#endif
}

SINT64 PluginLogWriter::seekToEnd()
{
#ifdef WIN_NT
	SINT64 nFileLen = _lseeki64(m_fileHandle, 0, SEEK_END);
#else
	off_t nFileLen = lseek(m_fileHandle, 0, SEEK_END);
#endif

	if (nFileLen < 0)
		checkErrno("lseek");

	return nFileLen;
}

void PluginLogWriter::reopen()
{
	if (m_fileHandle >= 0)
		::close(m_fileHandle);

#ifdef WIN_NT
	HANDLE hFile = CreateFile(
		m_fileName.c_str(),
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL,
		OPEN_ALWAYS,
		0, // FILE_FLAG_SEQUENTIAL_SCAN,
		NULL
		);
	m_fileHandle = _open_osfhandle((intptr_t) hFile, 0);
#else
	m_fileHandle = ::open(m_fileName.c_str(), O_CREAT | O_APPEND | O_RDWR, S_IRUSR | S_IWUSR);
#endif

	if (m_fileHandle < 0)
		checkErrno("open");
}

size_t PluginLogWriter::write(const void* buf, size_t size)
{
#ifdef WIN_NT
	Guard guard(this);
#endif

	if (m_fileHandle < 0)
		reopen();

	FB_UINT64 fileSize = seekToEnd();
	if (m_maxSize && (fileSize > m_maxSize))
	{
		reopen();
		fileSize = seekToEnd();
	}

	if (m_maxSize && (fileSize > m_maxSize))
	{
		const TimeStamp stamp(TimeStamp::getCurrentTimeStamp());
		struct tm times;
		stamp.decode(&times);

		PathName newName;
		const size_t last_dot_pos = m_fileName.rfind(".");
		if (last_dot_pos > 0)
		{
			PathName log_name = m_fileName.substr(0, last_dot_pos);
			PathName log_ext = m_fileName.substr(last_dot_pos + 1, m_fileName.length());
			newName.printf("%s.%04d-%02d-%02dT%02d-%02d-%02d.%s", log_name.c_str(), times.tm_year + 1900,
				times.tm_mon + 1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec, log_ext.c_str());
		}
		else
		{
			newName.printf("%s.%04d-%02d-%02dT%02d-%02d-%02d", m_fileName.c_str(), times.tm_year + 1900,
				times.tm_mon + 1, times.tm_mday, times.tm_hour, times.tm_min, times.tm_sec);
		}

#ifdef WIN_NT
		// hvlad: sad, but MSDN said "rename" returns EACCES when newName already
		// exists. Therefore we can't just check "rename" result for EEXIST and need
		// to write platform-dependent code. In reality, "rename" returns EEXIST to
		// me, not EACCES, strange...
		if (!MoveFile(m_fileName.c_str(), newName.c_str()))
		{
			const DWORD dwError = GetLastError();
			if (dwError != ERROR_ALREADY_EXISTS && dwError != ERROR_FILE_NOT_FOUND)
			{
				fatal_exception::raiseFmt("PluginLogWriter: MoveFile failed on file \"%s\". Error is : %d",
					m_fileName.c_str(), dwError);
			}
		}
#else
		if (rename(m_fileName.c_str(), newName.c_str()))
		{
			const int iErr = errno;
			if (iErr != ENOENT && iErr != EEXIST)
				checkErrno("rename");
		}
#endif

		reopen();
		seekToEnd();
	}

	const size_t written = ::write(m_fileHandle, buf, size);
	if (written != size)
		checkErrno("write");

	return written;
}

void PluginLogWriter::checkErrno(const char* operation)
{
	if (errno == 0)
		return;

	const char* strErr;
#ifdef WIN_NT
	strErr = strerror(errno);
#else
	char buff[256];
	strerror_r(errno, buff, sizeof(buff));
	strErr = buff;
#endif
	fatal_exception::raiseFmt("PluginLogWriter: operation \"%s\" failed on file \"%s\". Error is : %s",
		operation, m_fileName.c_str(), strErr);
}

#ifdef WIN_NT
void PluginLogWriter::checkMutex(const TEXT* string, int state)
{
	if (state)
	{
		TEXT msg[BUFFER_TINY];

		sprintf(msg, "PluginLogWriter: mutex %s error, status = %d", string, state);
		gds__log(msg);

		fprintf(stderr, "%s\n", msg);
		exit(FINI_ERROR);
	}
}

void PluginLogWriter::lock()
{
	checkMutex("lock", ISC_mutex_lock(&m_mutex));
}

void PluginLogWriter::unlock()
{
	checkMutex("unlock", ISC_mutex_unlock(&m_mutex));
}
#endif // WIN_NT
