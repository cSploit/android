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
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#include <fcntl.h>

using namespace Firebird;
Firebird::Mutex open_mutex;

void FileObject::open(int flags, int pflags)
{
	open_mutex.enter();
	int oflags = 0;

	switch (flags & (fo_rdonly | fo_wronly | fo_rdwr))
	{
	case fo_rdonly:
		oflags = O_RDONLY;
		break;
	case fo_wronly:
		oflags = O_WRONLY;
		break;
	default:
		oflags = O_RDWR;
		break;
	}

	if (flags & fo_append)
		oflags |= O_APPEND;

	if (flags & fo_creat)
		oflags |= O_CREAT;

	if (flags & fo_trunc)
		oflags |= O_TRUNC;

	if (flags & fo_excl)
		oflags |= O_EXCL;

	file = ::open(filename.c_str(), oflags, pflags);
	open_mutex.leave();

	if (file < 0)
		fatal_exception::raiseFmt("Error (%d) opening file: %s", errno, filename.c_str());

	if (flags & fo_temporary)
		unlink(filename.c_str());
}

FileObject::~FileObject()
{
	close(file);
}

//Size of file, given by descriptor
FB_UINT64 FileObject::size()
{
	off_t nFileLen = 0;
	struct stat file_stat;
	if (!fstat(file, &file_stat))
		nFileLen = file_stat.st_size;
	else
		fatal_exception::raiseFmt("IO error (%d) file stat: %s", errno, filename.c_str());

	return nFileLen;
}

size_t FileObject::blockRead(void* buffer, size_t bytesToRead)
{
	ssize_t bytesDone = read(file, buffer, bytesToRead);
	if (bytesDone < 0)
	{
		fatal_exception::raiseFmt("IO error (%d) reading file: %s", errno, filename.c_str());
	}

	return bytesDone;
}

void FileObject::blockWrite(const void* buffer, size_t bytesToWrite)
{
	ssize_t bytesDone = write(file, buffer, bytesToWrite);
	if (bytesDone != static_cast<ssize_t>(bytesToWrite))
	{
		fatal_exception::raiseFmt("IO error (%d) writing file: %s", errno, filename.c_str());
	}
}

// Write data to header only if file is empty
void FileObject::writeHeader(const void* buffer, size_t bytesToWrite)
{
	if (seek(0, so_from_end) != 0)
		return;

	ssize_t bytesDone = write(file, buffer, bytesToWrite);
	if (bytesDone != static_cast<ssize_t>(bytesToWrite))
	{
		fatal_exception::raiseFmt("IO error (%d) writing file: %s", errno, filename.c_str());
	}
}

void FileObject::reopen()
{
	if (file >= 0)
		close(file);
	open(fo_rdwr | fo_append | fo_creat, 0666);
	//fchmod(file, PMASK);
}

bool FileObject::renameFile(const Firebird::PathName new_filename)
{
	if (rename(filename.c_str(), new_filename.c_str()))
	{
		int rename_err = errno;
		if (rename_err == ENOENT || rename_err == EEXIST) {
			// Another process renames our file just now. Open it again.
			reopen();
			return false;
		}
		fatal_exception::raiseFmt("IO error (%d) renaming file: %s", rename_err,	filename.c_str());
	}
	else
		reopen();
	return true;
}

SINT64 FileObject::seek(SINT64 newOffset, SeekOrigin origin)
{
    if (newOffset != (SINT64) LSEEK_OFFSET_CAST newOffset)
	{
		fatal_exception::raiseFmt("Attempt to seek file %s past platform size limit", filename.c_str());
	}

	int moveMethod;

	switch(origin)
	{
		case so_from_beginning:
			moveMethod = SEEK_SET;
			break;
		case so_from_current:
			moveMethod = SEEK_CUR;
			break;
		case so_from_end:
			moveMethod = SEEK_END;
			break;
	}

	off_t result = lseek(file, newOffset, moveMethod);

	if (result == (off_t) -1)
	{
		fatal_exception::raiseFmt("IO error (%d) seeking file: %s", errno, filename.c_str());
	}

	return result;
}
