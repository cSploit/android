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
 *  The Original Code was created by Claudio Valderrama on 3-Mar-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// Localized messages type-safe printing facility.

#ifndef FB_BASESTREAM_H
#define FB_BASESTREAM_H

#ifdef HAVE_IO_H
#include <io.h>
#endif

#include <stdio.h>

namespace MsgFormat
{

// This is the abstract base class that is used by the MsgPrint routines.
// It doesn't own the stream, thus it doesn't open, check or close it.
// The desired functionality can be added in the derived classes.
// This approach is taken because the most sought usage is to work with the
// predefined output streams and these aren't closed typically by a program.
// The write() method takes void* to avoid issues with char v/s UCHAR strings.
// The value returned is the number of bytes, ignoring the null terminator in
// the case of strings.
class BaseStream
{
public:
	virtual int write(const void* str, unsigned int n) = 0;
	virtual ~BaseStream() {}
};


// This class represents that raw, typically unbuffered access to the operating
// system level file handles.
// The stream should be opened already and should be closed elsewhere.
class RawStream : public BaseStream
{
public:
	explicit RawStream(int stream = 1); // stdout
	int write(const void* str, unsigned int n);
private:
	int m_stream;
};

inline RawStream::RawStream(int stream)
	: m_stream(stream)
{
}


// This class represents the high-level, typically buffered access to the files
// through the FILE* structure in the stdio headers. Since there may be the need
// to flush the buffers immediately when the object is being destructed, an
// optional (false by default) do_flush parameter is provided in the constructor.
// The stream should be opened already and should be closed elsewhere.
class StdioStream : public BaseStream
{
public:
	explicit StdioStream(FILE* stream, bool do_flush = false);
	int write(const void* str, unsigned int n);
	~StdioStream();
private:
	FILE* m_stream;
	const bool m_flush;
};

inline StdioStream::StdioStream(FILE* stream, bool do_flush)
	: m_stream(stream), m_flush(do_flush)
{
}


// This class wraps a fixed size null terminated string and avoid buffers overruns.
// The parameters are the existing C string and its size in bytes.
// Notice the write() method will print an ellipsis if the format string with its
// arguments try to use more space than string size and the terminator is always
// appended (at most at the last valid position). As in a file stream, with each
// call to write, the position inside the string is updated and \0 is written.
class StringStream : public BaseStream
{
public:
	StringStream(char* const stream, unsigned int s_size);
	int write(const void* str, unsigned int n);
private:
	const unsigned int m_size;
	char* const m_max_pos;
	char* const m_ellipsis;
	char* m_current_pos;
};

} // namespace

#endif // FB_BASESTREAM_H
