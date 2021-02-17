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

#include "firebird.h"
#include "Extender.h"
#include "../common/classes/alloc.h"

// Dynamic allocation. This is a destructive operation if the requested size
// is bigger than the existing one. Otherwise, no reallocation happens.
// The insertion point is always reset. This means getUsed() returns zero after
// this call. The first byte is initialized to zero to aid in debugging.
// If size zero is passed, no allocation happens but the insertion point is reset.
void Extender::alloc(size_t n)
{
	if (m_size < n)
	{
		delete[] m_buf;

		m_buf = new char[m_size = n];
		m_buf[0] = 0;
	}
	m_pos = m_buf;
}

// Allocation plus initialization. Same as alloc(), but initializes the internal
// buffer with the desired byte.
// This means the first byte intialization to zero in alloc() is wiped out.
void Extender::allocFill(size_t n, char c)
{
	alloc(n);
	memset(m_buf, c, n);
}

// Append to the string. Note second parameter is the size, not the length.
// This means that a null terminator is not added unless the input contains it.
// If there's already a null terminator, it's not discarded before appending!
// If there's space used, a newline is optionally used as separator between
// the existing content and the new input.
// The internal buffer is not grown; the input will be truncated.
// The amount of written bytes (the ones that fit) is returned.
size_t Extender::append(const char* s, size_t s_size, bool newline)
{
	if (m_pos >= m_buf + m_size) // Full buffer?
		return 0;

	const size_t extra = m_buf < m_pos && newline ? 1 : 0;

	if (m_pos + s_size + extra > m_buf + m_size) // Adjust the argument, truncating it.
		s_size = m_buf + m_size - m_pos - extra;

	// Do not append newline if there's nothing before or the caller doesn't want it.
	if (extra)
		*m_pos++ = '\n';

	memcpy(m_pos, s, s_size);
	m_pos += s_size;
	return s_size + extra;
}

// Unlike alloc() and allocFill(), this is not a destructive operation.
// If the requested size is not bigger than the existing one, nothing happens and
// the insertion point remains in its current position.
// Otherwise, reallocation happens and the old contents are copied into the
// new buffer. Notice only getUsed() bytes are copied. This means a fill pattern
// will not be preserved. The insertion point is updated after the copy to point
// to the same position in the string where it was before growing the buffer.
// The position at the insertion point is initialized to zero to aid in debugging.
void Extender::grow(size_t n)
{
	if (m_pos == m_buf)
	{
		// We don't have data to preserve, go faster.
		alloc(n);
		return;
	}

	if (m_size < n)
	{
		const size_t old_pos = getUsed();
		char* const old_buf = m_buf;

		m_buf = new char[m_size = n];
		memcpy(m_buf, old_buf, old_pos); // Copy only the used bytes.
		m_pos = m_buf + old_pos; // Reposition the current insertion point.
		m_pos[0] = 0; // Same as alloc().

		delete[] old_buf;
	}
}

