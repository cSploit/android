/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		FileObject.h
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

#ifndef OS_FILEOBJECT_H
#define OS_FILEOBJECT_H

#include "../../../common/classes/fb_string.h"
#include "../../../common/classes/locks.h"

#ifdef WIN_NT
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif

enum FileOpenFlags
{
	fo_rdonly = 0x0000,			// open for reading only
	fo_wronly = 0x0001,			// open for writing only
	fo_rdwr   = 0x0002,			// open for reading and writing
	fo_append = 0x0008,			// writes done at eof

	fo_creat  = 0x0100,			// create and open file
	fo_trunc  = 0x0200,			// open and truncate
	fo_excl   = 0x0400,			// open only if file doesn't already exist

	// Temporary file bit - file is deleted when last handle is closed
	fo_temporary = 0x0040,		// temporary file bit

	// temporary access hint
	fo_short_lived = 0x1000,	// temporary storage file, try not to flush

	// sequential/random access hints
	fo_sequential = 0x0020,		// file access is primarily sequential
	fo_random = 0x0010			// file access is primarily random
};

enum SeekOrigin
{
	so_from_beginning,
	so_from_current,
	so_from_end
};

class FileObject : public Firebird::AutoStorage
{
public:
	FileObject(const Firebird::PathName& afilename, int flags, int pflags = 0666) :
		filename(getPool(), afilename),
#ifdef WIN_NT
		file(INVALID_HANDLE_VALUE),
		append_mutex(INVALID_HANDLE_VALUE)
#else
	  file(-1)
#endif
	{
		open(flags, pflags);
	}

	FileObject(Firebird::MemoryPool& pool, const Firebird::PathName& afilename, int flags, int pflags = 0666) :
		Firebird::AutoStorage(pool), filename(getPool(), afilename),
#ifdef WIN_NT
		file(INVALID_HANDLE_VALUE),
		append_mutex(INVALID_HANDLE_VALUE)
#else
		file(-1)
#endif
	{
		open(flags, pflags);
	}

	~FileObject();

	// Platform-specific stuff
	size_t blockRead(void* buffer, size_t bytesToRead);
	void blockWrite(const void* buffer, size_t bytesToWrite);
	void writeHeader(const void* buffer, size_t bytesToWrite);
	SINT64 seek(SINT64 offset, SeekOrigin origin);
	FB_UINT64 size();

	//This method used when log file was renamed by another process
	void reopen();
	bool renameFile(const Firebird::PathName new_filename);

	// Generic stuff. Let it be inline for the moment.
	// If there will be a more than a few such methods we need to use inheritance
	bool readLine(Firebird::string& dest)
	{
		// This method is not very efficient, but is still much better than
		// reading characters one-by-one. Plus it can handle line breaks in
		// Windows, Linux and Macintosh format nicely on all platforms
		char buffer[100];
		size_t bytesRead;
		dest.resize(0);
		bool prevCR = false;

		do
		{
			bytesRead = blockRead(buffer, sizeof(buffer));
			for (int pos = 0; pos < bytesRead; pos++)
			{
				switch (buffer[pos])
				{
					case '\n':
						dest.append(buffer, pos);
						// Adjust file pointer
						seek(pos - bytesRead + 1, so_from_current);
						// Kill trailing CR if present (Windows)
						if (prevCR)
							dest.resize(dest.length() - 1);
						return true;
					case '\r':
						prevCR = true;
						break;
					default:
						if (prevCR)
						{
							dest.append(buffer, pos);
							// Adjust file pointer
							seek(pos - bytesRead, so_from_current);
							return true;
						}
				}
			}
			dest.append(buffer, bytesRead);
		} while (bytesRead == sizeof(buffer));

		// Kill trailing CR if present
		if (prevCR)
			dest.resize(dest.length() - 1);

		return dest.length() || bytesRead;
	}

	void writeLine(const Firebird::string& from)
	{
		// Line should be written in a single BlockWrite call to handle file append properly
		Firebird::string temp = from.substr(0, from.max_length() - 2) + NEWLINE;
		blockWrite(temp.c_str(), temp.length());
	}

private:
	// Forbid copy and assignment operators
	FileObject(const FileObject&);
	FileObject& operator =(const FileObject&);

	void open(int flags, int pflags);

	Firebird::PathName filename;
#ifdef WIN_NT
	HANDLE file;
	HANDLE append_mutex;
#else
	int file;
#endif
};

#endif // OS_FILEOBJECT_H
