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

ISC_STATUS DynamicStatusVector::save(const ISC_STATUS* status)
{
	m_status_vector.clear();

	const ISC_STATUS* from = status;

	while (true)
	{
		const ISC_STATUS type = *from++;
		m_status_vector.push(type == isc_arg_cstring ? isc_arg_string : type);
		if (type == isc_arg_end)
			break;

		switch (type)
		{
		case isc_arg_cstring:
			{
				const size_t len = *from++;

				char* string = FB_NEW(*getDefaultMemoryPool()) char[len + 1];
				const char *temp = reinterpret_cast<const char*>(*from++);
				memcpy(string, temp, len);
				string[len] = 0;

				m_status_vector.push((ISC_STATUS)(IPTR) string);
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

				m_status_vector.push((ISC_STATUS)(IPTR) string);
			}
			break;

		default:
			m_status_vector.push(*from++);
			break;
		}
	}

	// Sanity check
	if (m_status_vector.getCount() < 3)
	{
		fb_utils::init_status(m_status_vector.getBuffer(3));
	}

	return m_status_vector[1];
}

void DynamicStatusVector::clear()
{
	ISC_STATUS *ptr = m_status_vector.begin();

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
			fb_assert(false); // CVC: according to the new logic, this case cannot happen
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

	// Sanity check
	if (m_status_vector.getCount() < 3)
	{
		m_status_vector.getBuffer(3);
	}

	fb_utils::init_status(m_status_vector.begin());
}

ISC_STATUS StatusHolder::save(const ISC_STATUS* status)
{
	fb_assert(isSuccess() || m_raised);
	if (m_raised)
	{
		clear();
	}

	return m_status_vector.save(status);
}

void StatusHolder::clear()
{
	m_status_vector.clear();
	m_raised = false;
}

void StatusHolder::raise()
{
	if (getError())
	{
		m_raised = true;
		status_exception::raise(m_status_vector.value());
	}
}


} // namespace Firebird
