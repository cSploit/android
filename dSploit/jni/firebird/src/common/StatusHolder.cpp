/*
 *	PROGRAM:		Firebird exceptions classes
 *	MODULE:			StatusHolder.cpp
 *	DESCRIPTION:	Firebird's exception classes
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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Vlad Khorsun <hvlad at users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include "StatusHolder.h"
#include "gen/iberror.h"
#include "classes/alloc.h"

namespace Firebird {

ISC_STATUS StatusHolder::save(const ISC_STATUS* status)
{
	fb_assert(m_status_vector[1] == 0 || m_raised);
	if (m_raised) {
		clear();
	}

	const ISC_STATUS *from = status;
	ISC_STATUS *to = m_status_vector;
	while (true)
	{
		const ISC_STATUS type = *to++ = *from++;
		if (type == isc_arg_end)
			break;

		switch (type)
		{
		case isc_arg_cstring:
			{
				const size_t len = *to++ = *from++;
				char *string = FB_NEW(*getDefaultMemoryPool()) char[len];
				const char *temp = reinterpret_cast<const char*>(*from++);
				memcpy(string, temp, len);
				*to++ = (ISC_STATUS)(IPTR) string;
			}
			break;

		case isc_arg_string:
		case isc_arg_interpreted:
		case isc_arg_sql_state:
			{
				const char* temp = reinterpret_cast<const char*>(*from++);

				const size_t len = strlen(temp);
				char* string = FB_NEW(*getDefaultMemoryPool()) char[len + 1];
				memcpy(string, temp, len + 1);

				*to++ = (ISC_STATUS)(IPTR) string;
			}
			break;

		default:
			*to++ = *from++;
			break;
		}
	}
	return m_status_vector[1];
}

void StatusHolder::clear()
{
	ISC_STATUS *ptr = m_status_vector;
	while (true)
	{
		const ISC_STATUS type = *ptr++;
		if (type == isc_arg_end)
			break;

		switch (type)
		{
		case isc_arg_cstring:
			ptr++;
			delete[] reinterpret_cast<char*>(*ptr++);
			break;

		case isc_arg_string:
		case isc_arg_interpreted:
		case isc_arg_sql_state:
			delete[] reinterpret_cast<char*>(*ptr++);
			break;

		default:
			ptr++;
			break;
		}
	}
	memset(m_status_vector, 0, sizeof(m_status_vector));
	m_raised = false;
}

void StatusHolder::raise()
{
	if (getError())
	{
		m_raised = true;
		status_exception::raise(m_status_vector);
	}
}


} // namespace Firebird
