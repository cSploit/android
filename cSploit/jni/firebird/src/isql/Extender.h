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
 *  The Original Code was created by Claudio Valderrama on 25-Feb-2007
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#ifndef FB_EXTENDER_H
#define FB_EXTENDER_H

// Helper class for the bulk insertion tricks.
// It does automatic resource management
// and appends starting with newline when there's anything already in the buffer.
// Calling alloc() again with a bigger buffer size simply forgets the old data.
// Calling alloc() always resets the usage counter to zero, hence the first
// call to append() after alloc() always work as a simple copy.
// Using the object is only valid after the first append() call.
// This class can be used with binary data that includes the null terminator,
// although it was designed for text (see the newline inserted by append()).
// It only puts the null terminator on allocation; no warranties getBuffer()
// will return a null terminated string unless append() received it.

#include <memory.h>

class Extender
{
public:
	Extender();
	~Extender();
	void alloc(size_t n);
	void allocFill(size_t n, char c);
	size_t append(const char* s, size_t s_size, bool newline = true);
	char* getBuffer();
	const char* getBuffer() const;
	size_t getUsed() const;
protected:
	void grow(size_t n);
private:
	char* m_buf;
	size_t m_size;
	char* m_pos;
};


inline Extender::Extender()
	: m_buf(0), m_size(0), m_pos(0)
{
}

inline Extender::~Extender()
{
	delete[] m_buf;
}

inline char* Extender::getBuffer()
{
	return m_buf;
}

// Probably won't be used as I can't imagine a const Extender object except
// when passed to another function as const param with content already stored.
inline const char* Extender::getBuffer() const
{
	return m_buf;
}

// It uses the current insertion point to calculate an offset from the base address.
// This is in turn the number of effective bytes in use from the whole allocation.
inline size_t Extender::getUsed() const
{
	return m_pos - m_buf;
}

#endif // FB_EXTENDER_H

