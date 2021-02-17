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

#include "firebird.h"
#include "BaseStream.h"
#include <string.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

namespace MsgFormat
{

int RawStream::write(const void* str, unsigned int n)
{
	return ::write(m_stream, str, n);
}

/////////////////////

int StdioStream::write(const void* str, unsigned int n)
{
	return ::fwrite(str, 1, n, m_stream);
}

StdioStream::~StdioStream()
{
	if (m_flush)
		::fflush(m_stream);
}

/////////////////////


// We cater for the special case where a wicked soul used a string even smaller
// than the ellipsis!
StringStream::StringStream(char* const stream, unsigned int s_size)
	: m_size(s_size), m_max_pos(s_size ? stream + s_size - 1 : stream),
	m_ellipsis(s_size > 3 ? stream + s_size - 4 : stream)
{
	m_current_pos = stream;
}

// The count of written bytes does not include the null terminator.
// The count is adjusted when there's not enough room in the target string.
// With not enough room, an ellipsis will be print at the end to show failure.
// The ellipsis may rewrite some positions to be printed. The rewritten bytes
// aren't counted in the returned value.
int StringStream::write(const void* str, unsigned int n)
{
	if (m_current_pos >= m_max_pos)
		return 0;

	unsigned int avail = n;
	if (m_current_pos + n >= m_max_pos)
	{
		if (m_current_pos >= m_ellipsis)
			avail = 0;
		else
			avail = m_ellipsis - m_current_pos;
	}
	memcpy(m_current_pos, str, avail);
	if (avail < n)
	{
		memcpy(m_ellipsis, "...", m_size > 3 ? 4 : m_size);
		avail = m_max_pos - m_current_pos;
		m_current_pos = m_max_pos;
	}
	else
		m_current_pos += avail;

	m_current_pos[0] = 0;

	return avail;
}

} // namespace
