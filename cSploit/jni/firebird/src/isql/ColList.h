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


#ifndef FB_COLLIST_H
#define FB_COLLIST_H

#include "../jrd/constants.h"

// This is a linked list implementing a seldom used feature: the ability to
// asssign a maximum width to each column, by name. The name should match the
// one that appears in a result set (from a SELECT statement).
// Each "item" represents a column name and its associated length.

class ColList
{
public:
	struct item
	{
		TEXT col_name[MAX_SQL_IDENTIFIER_SIZE];
		unsigned col_len;
		item* next;
		item(const char* name, unsigned len);
	};

	ColList();
	~ColList();
	void clear();
	item* getHead();
	bool put(const char* name, unsigned len);
	bool remove(const char* name);
	const item* find(const char* name) const;
	bool find(const char* name, unsigned* out_len) const;
	size_t count() const;

private:
	size_t m_count;
	item* m_head;
};

inline ColList::ColList()
	: m_count(0), m_head(0)
{
}

inline ColList::~ColList()
{
	clear();
}

inline ColList::item* ColList::getHead()
{
	return m_head;
}

inline size_t ColList::count() const
{
	return m_count;
}

#endif // FB_COLLIST_H

