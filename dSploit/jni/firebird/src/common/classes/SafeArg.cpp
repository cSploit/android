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
 *  The author thanks specially Fred Polizo Jr and Adriano dos Santos Fernandes
 *  for comments and corrections.
 *
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// Localized messages type-safe printing facility.

#include "firebird.h"
#include "../jrd/common.h"
#include "SafeArg.h"

namespace MsgFormat
{

// This is just a silly convenience in case all arguments are of type int.
SafeArg::SafeArg(const int val[], size_t v_size)
	: m_extras(0)
{
	if (v_size > SAFEARG_MAX_ARG)
		v_size = SAFEARG_MAX_ARG; // Simply truncate.

	m_count = v_size;

	for (size_t a_count = 0; a_count < m_count; ++a_count)
	{
		m_arguments[a_count].i_value = val[a_count];
		m_arguments[a_count].type = safe_cell::at_int64;
	}
}

// Here follows the list of overloads to insert the basic data types.
SafeArg& SafeArg::operator<<(char c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].c_value = c;
		m_arguments[m_count].type = safe_cell::at_char;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(unsigned char c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].c_value = c;
		m_arguments[m_count].type = safe_cell::at_uchar;
		++m_count;
	}
	return *this;
}

/*
SafeArg& SafeArg::operator<<(_int16 c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_int16;
		++m_count;
	}
	return *this;
}
*/

SafeArg& SafeArg::operator<<(short c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_int64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(unsigned short c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_uint64;
		++m_count;
	}
	return *this;
}

/*
SafeArg& SafeArg::operator<<(_int32 c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_int32;
		++m_count;
	}
	return *this;
}
*/

SafeArg& SafeArg::operator<<(int c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_int64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(unsigned int c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = static_cast<int64_t>(c);
		m_arguments[m_count].type = safe_cell::at_uint64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(long int c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::at_int64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(unsigned long int c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = static_cast<int64_t>(c);
		m_arguments[m_count].type = safe_cell::at_uint64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(SINT64 c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = static_cast<int64_t>(c);
		m_arguments[m_count].type = safe_cell::at_int64;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(FB_UINT64 c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = static_cast<int64_t>(c);
		m_arguments[m_count].type = safe_cell::at_uint64;
		++m_count;
	}
	return *this;
}


/*
SafeArg& SafeArg::operator<<(long c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i_value = c;
		m_arguments[m_count].type = safe_cell::atint64_t;
		++m_count;
	}
	return *this;
}
*/

SafeArg& SafeArg::operator<<(SINT128 c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].i128_value = c;
		m_arguments[m_count].type = safe_cell::at_int128;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(double c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].d_value = c;
		m_arguments[m_count].type = safe_cell::at_double;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(const char* c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].st_value.s_string = c;
		m_arguments[m_count].type = safe_cell::at_str;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(const unsigned char* c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].st_value.s_string = reinterpret_cast<const char*>(c);
		m_arguments[m_count].type = safe_cell::at_str;
		++m_count;
	}
	return *this;
}

SafeArg& SafeArg::operator<<(void* c)
{
	if (m_count < SAFEARG_MAX_ARG)
	{
		m_arguments[m_count].p_value = c;
		m_arguments[m_count].type = safe_cell::at_ptr;
		++m_count;
	}
	return *this;
}

// Dump the parameters to the target buffer.
// Note that parameters of type pointer (void*) and counted_string may crash
// the caller if not prepared to handle them. Therefore, counted_string is being
// converted to null pointer and void* to TEXT*. Supposedly, the caller has
// information on the real types of the values. This can be done with a loop
// using getCount() and getCell() and looking at the safe_cell's type data member.
void SafeArg::dump(const TEXT* target[], size_t v_size) const
{
	for (size_t i = 0; i < v_size; ++i)
	{
		if (i >= m_count)
		{
			target[i] = 0;
			continue;
		}

		switch (m_arguments[i].type)
		{
		case safe_cell::at_char:
			target[i] = reinterpret_cast<TEXT*>((IPTR) m_arguments[i].c_value);
			break;
		case safe_cell::at_uchar:
			target[i] = reinterpret_cast<TEXT*>((U_IPTR) m_arguments[i].c_value);
			break;
		case safe_cell::at_int64:
			target[i] = reinterpret_cast<TEXT*>((IPTR) m_arguments[i].i_value);
			break;
		case safe_cell::at_uint64:
			target[i] = reinterpret_cast<TEXT*>((U_IPTR) m_arguments[i].i_value);
			break;
		case safe_cell::at_int128:
			target[i] = reinterpret_cast<TEXT*>((IPTR) m_arguments[i].i128_value.high);
			break;
		case safe_cell::at_double:
			target[i] = reinterpret_cast<TEXT*>((IPTR) m_arguments[i].d_value);
			break;
		case safe_cell::at_str:
			target[i] = m_arguments[i].st_value.s_string;
			break;
		case safe_cell::at_ptr:
			target[i] = static_cast<TEXT*>(m_arguments[i].p_value);
			break;
		default: // safe_cell::at_none and whatever out of range.
			target[i] = 0;
			break;
		}
	}
}

// Get one specific cell. If out of range, a cell with invalid type (at_none)
// is returned.
const safe_cell& SafeArg::getCell(size_t index) const
{
	static safe_cell aux_cell = {safe_cell::at_none, {0}};

	if (index < m_count)
		return m_arguments[index];

	return aux_cell;
}

} // namespace

