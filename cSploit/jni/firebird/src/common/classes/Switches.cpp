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
 *  The Original Code was created by Claudio Valderrama on 10-Jul-2009
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Claudio Valderrama
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include "Switches.h"
#include "fb_exception.h"

namespace
{
	const char switch_char = '-';
}

Switches::Switches(const in_sw_tab_t* table, size_t count, bool copy, bool minLength)
	: m_base(table), m_count(count), m_copy(copy), m_minLength(minLength),
	  m_table(0), m_opLengths(0)
{
	fb_assert(table && count > 1); // the last element is a terminator
	if (!table || count < 2)
		complain("Switches: invalid arguments for constructor");

	if (m_copy)
	{
		m_table = FB_NEW(*getDefaultMemoryPool()) in_sw_tab_t[m_count];
		for (size_t iter = 0; iter < m_count; ++iter)
			m_table[iter] = m_base[iter];
	}

	m_opLengths = FB_NEW(*getDefaultMemoryPool()) size_t[m_count];

	for (size_t iter = 0; iter < m_count; ++iter)
	{
		if (m_base[iter].in_sw_name)
		{
			m_opLengths[iter] = strlen(m_base[iter].in_sw_name);
			fb_assert(m_opLengths[iter] > 0);
			fb_assert(!m_minLength || m_opLengths[iter] >= m_base[iter].in_sw_min_length);
		}
		else
			m_opLengths[iter] = 0;
	}
}

Switches::~Switches()
{
	delete[] m_table;
	delete[] m_opLengths;
}

const Switches::in_sw_tab_t* Switches::findSwitch(Firebird::string sw, bool* invalidSwitchInd) const
{
/**************************************
 *
 *	f i n d S w i t c h
 *
 **************************************
 *
 * Functional description
 *	Returns pointer to in_sw_tab entry for current switch
 *	If not a switch, returns NULL.
 *	If no match, invalidSwitch is set to true (if the pointer is given)
 *
 **************************************/
	if (sw.isEmpty() || sw[0] != switch_char)
	{
		return 0;
	}
	if (sw.length() == 1)
	{
		if (invalidSwitchInd)
			*invalidSwitchInd = true;
		return 0;
	}

	sw.erase(0, 1);
	sw.upper();

	size_t iter = 0;
	for (const in_sw_tab_t* in_sw_tab = m_base; in_sw_tab->in_sw_name; ++in_sw_tab, ++iter)
	{
		if ((!m_minLength || sw.length() >= in_sw_tab->in_sw_min_length) &&
			matchSwitch(sw, in_sw_tab->in_sw_name, m_opLengths[iter]))
		{
			return in_sw_tab;
		}
	}
	fb_assert(iter == m_count - 1);
	if (invalidSwitchInd)
		*invalidSwitchInd = true;

	return 0;
}

Switches::in_sw_tab_t* Switches::findSwitchMod(Firebird::string& sw, bool* invalidSwitchInd)
{
/**************************************
 *
 *	f i n d S w i t c h M o d
 *
 **************************************
 *
 * Functional description
 *	Returns pointer to in_sw_tab entry for current switch
 *	If not a switch, returns NULL.
 *
 **************************************/
	fb_assert(m_copy && m_table);
	if (!m_copy || !m_table)
		complain("Switches: calling findSwitchMod for a const switch table");

	if (sw.isEmpty() || sw[0] != switch_char)
	{
		return 0;
	}
	if (sw.length() == 1)
	{
		if (invalidSwitchInd)
			*invalidSwitchInd = true;
		return 0;
	}

	sw.erase(0, 1);
	sw.upper();

	size_t iter = 0;
	for (in_sw_tab_t* in_sw_tab = m_table; in_sw_tab->in_sw_name; ++in_sw_tab, ++iter)
	{
		if ((!m_minLength || sw.length() >= in_sw_tab->in_sw_min_length) &&
			matchSwitch(sw, in_sw_tab->in_sw_name, m_opLengths[iter]))
		{
			return in_sw_tab;
		}
	}
	fb_assert(iter == m_count - 1);
	if (invalidSwitchInd)
		*invalidSwitchInd = true;

	return NULL;
}

const Switches::in_sw_tab_t* Switches::getTable() const
{
	return m_base;
}

Switches::in_sw_tab_t* Switches::getTableMod()
{
	fb_assert(m_copy && m_table);
	if (!m_copy || !m_table)
		complain("Switches: calling getTableMod for a const switch table");

	return m_table;
}

void Switches::activate(const int in_sw)
{
	fb_assert(m_copy && m_table && in_sw > 0);
	if (!m_copy || !m_table)
		complain("Switches: calling activate() for a const switch table");

	if (in_sw <= 0)
		complain("Switches: calling activate() with an element out of range");

	// There may be multiple copies due to different naming for the same var.
	// Therefore, we don't stop at the first match. This solves a problem with
	// isql and burp to be able to detect switches duplication.
	size_t matches = 0, iter = 0;
	for (in_sw_tab_t* in_sw_tab = m_table; in_sw_tab->in_sw_name; ++in_sw_tab, ++iter)
	{
		if (in_sw_tab->in_sw == in_sw)
		{
			in_sw_tab->in_sw_state = true;
			++matches;
		}
	}
	fb_assert(iter == m_count - 1);
	fb_assert(matches > 0);
	if (matches == 0)
		complain("Switches: activate cannot locate the element by Tag");
}

bool Switches::exists(const int in_sw, const char* const* argv, const int start, const int stop) const
{
	fb_assert(in_sw > 0);
	if (in_sw <= 0)
		complain("Switches: calling exists() with an element out of range");

	size_t pos = 0;
	const in_sw_tab_t* const rc = findByTag(in_sw, &pos);
	fb_assert(rc);
	const size_t rclen = m_opLengths[pos];
	fb_assert(rclen);

	for (int itr = start; itr < stop; ++itr)
	{
		Firebird::string sw(argv[itr]);
		if (sw.length() < 2 || sw[0] != switch_char)
			continue;
		sw.erase(0, 1);
		sw.upper();
		if ((!m_minLength || sw.length() >= rc->in_sw_min_length) &&
			matchSwitch(sw, rc->in_sw_name, rclen))
		{
			return true;
		}
	}

	return false;
}

const char* Switches::findNameByTag(const int in_sw) const
{
	const in_sw_tab_t* rc = findByTag(in_sw, NULL, false);
	fb_assert(rc);
	return rc->in_sw_name;
}


// Begin private functions.

// static function
bool Switches::matchSwitch(const Firebird::string& sw, const char* target, size_t n)
{
/**************************************
 *
 *	m a t c h S w i t c h
 *
 **************************************
 *
 * Functional description
 *	Returns true if switch matches target
 *
 **************************************/
	if (n < sw.length())
	{
		return false;
	}
	n = sw.length();
	return memcmp(sw.c_str(), target, n) == 0;
}

const Switches::in_sw_tab_t* Switches::findByTag(const int in_sw, size_t* pos, bool rejectAmbiguity) const
{
	fb_assert(in_sw > 0);
	if (in_sw <= 0)
		complain("Switches: calling findByTag with an element out of range");

	const in_sw_tab_t* rc = NULL;
	size_t iter = 0;
	for (const in_sw_tab_t* in_sw_tab = m_table; in_sw_tab->in_sw_name; ++in_sw_tab, ++iter)
	{
		if (in_sw_tab->in_sw == in_sw)
		{
			if (rc)
			{
				fb_assert(rejectAmbiguity);
				complain("Switches: findByTag found more than one item with the same Tag (key)");
			}

			if (pos)
				*pos = iter;

			fb_assert(!rc);
			rc = in_sw_tab;
			if (!rejectAmbiguity)
				return rc;
		}
	}
	fb_assert(iter == m_count - 1);
	if (!rc)
		complain("Switches: findByTag cannot locate the element");

	return rc;
}

// static function
void Switches::complain(const char* msg)
{
	Firebird::system_call_failed::raise(msg);
}
