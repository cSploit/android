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
 *  The Original Code was created by Claudio Valderrama on 25-Sept-2007
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
#include "ColList.h"
#include "../common/utils_proto.h"


ColList::item::item(const char* name, unsigned len)
	: col_len(len), next(0)
{
	fb_utils::copy_terminate(col_name, name, sizeof(col_name));
}


// Deletes all items in the list.
void ColList::clear()
{
	while (m_head)
	{
		item* p = m_head;
		m_head = m_head->next;
		delete p;
		--m_count;
	}
	fb_assert(m_count == 0);
	m_head = 0;
	m_count = 0;
}

// Put an item in the list. If the name already exists, replace the item's length.
bool ColList::put(const char* name, unsigned len)
{
	if (!m_head)
	{
		fb_assert(m_count == 0);
		m_head = new item(name, len);
	}
	else
	{
		fb_assert(m_count > 0);
		item* p = m_head;
		while (p->next && strcmp(p->col_name, name))
			p = p->next;

		// If there is a match on name, replace the length
		if (!strcmp(p->col_name, name))
		{
			p->col_len = len;
			return false;
		}
		fb_assert(p->next == 0);
		p->next = new item(name, len);
	}
	++m_count;
	return true;
}

// Try to delete an item by name. Returns true if found and removed, false otherwise.
bool ColList::remove(const char* name)
{
	item* pold = NULL;
	item* p = m_head;
	while (p && strcmp(p->col_name, name))
	{
		pold = p;
		p = p->next;
	}

	// If there is a match on name, delete the entry
	if (p)
	{
		fb_assert(m_count > 0);
		if (pold)
			pold->next = p->next;
		else
			m_head = NULL;

		delete p;
		--m_count;
		return true;
	}
	return false;
}

// Locate the item by name and return it or return NULL.
// The return is const data because it doesn't make sense to modify it beyond the
// put() method in this same class.
const ColList::item* ColList::find(const char* name) const
{
	for (const item* pc = m_head; pc; pc = pc->next)
	{
		if (!strcmp(name, pc->col_name))
			return pc;
	}
	return 0;
}

// Locate the item by name and return true if found; false otherwise.
// If found, put the item's length in the second (output) argument.
bool ColList::find(const char* name, unsigned* out_len) const
{
	for (const item* pc = m_head; pc; pc = pc->next)
	{
		if (!strcmp(name, pc->col_name))
		{
			*out_len = pc->col_len;
			return true;
		}
	}
	return false;
}

