/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		MetaName.h
 *	DESCRIPTION:	metadata name holder
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
 *  The Original Code was created by Alexander Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2005 Alexander Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef METANAME_H
#define METANAME_H

#include "../common/classes/fb_string.h"
#include "../common/classes/fb_pair.h"
#include "../jrd/constants.h"

#ifdef SFIO
#include <stdio.h>
#endif

namespace Firebird {

class MetaName
{
private:
	char data[MAX_SQL_IDENTIFIER_SIZE];
	unsigned int count;

	void init()
	{
		memset(data, 0, MAX_SQL_IDENTIFIER_SIZE);
	}
	MetaName& set(const MetaName& m)
	{
		memcpy(data, m.data, MAX_SQL_IDENTIFIER_SIZE);
		count = m.count;
		return *this;
	}

public:
	MetaName() { init(); count = 0; }
	MetaName(const char* s) { assign(s); }
	MetaName(const char* s, size_t l) { assign(s, l); }
	MetaName(const MetaName& m) { set(m); }
	MetaName(const AbstractString& s) { assign(s.c_str(), s.length()); }
	explicit MetaName(MemoryPool&) { init(); count = 0; }
	MetaName(MemoryPool&, const char* s) { assign(s); }
	MetaName(MemoryPool&, const char* s, size_t l) { assign(s, l); }
	MetaName(MemoryPool&, const MetaName& m) { set(m); }
	MetaName(MemoryPool&, const AbstractString& s) { assign(s.c_str(), s.length()); }

	MetaName& assign(const char* s, size_t l);
	MetaName& assign(const char* s) { return assign(s, s ? strlen(s) : 0); }
	MetaName& operator=(const char* s) { return assign(s); }
	MetaName& operator=(const AbstractString& s) { return assign(s.c_str(), s.length()); }
	MetaName& operator=(const MetaName& m) { return set(m); }
	char* getBuffer(const size_t l);

	size_t length() const { return count; }
	const char* c_str() const { return data; }
	const char* nullStr() const { return (count == 0 ? NULL : data); }
	bool isEmpty() const { return count == 0; }
	bool hasData() const { return count != 0; }

	const char* begin() const { return data; }
	const char* end() const { return data + count; }

	int compare(const char* s, size_t l) const;
	int compare(const char* s) const { return compare(s, s ? strlen(s) : 0); }
	int compare(const AbstractString& s) const { return compare(s.c_str(), s.length()); }
	int compare(const MetaName& m) const { return memcmp(data, m.data, MAX_SQL_IDENTIFIER_SIZE); }

	bool operator==(const char* s) const { return compare(s) == 0; }
	bool operator!=(const char* s) const { return compare(s) != 0; }
	bool operator==(const AbstractString& s) const { return compare(s) == 0; }
	bool operator!=(const AbstractString& s) const { return compare(s) != 0; }
	bool operator==(const MetaName& m) const { return compare(m) == 0; }
	bool operator!=(const MetaName& m) const { return compare(m) != 0; }
	bool operator<=(const MetaName& m) const { return compare(m) <= 0; }
	bool operator>=(const MetaName& m) const { return compare(m) >= 0; }
	bool operator< (const MetaName& m) const { return compare(m) <  0; }
	bool operator> (const MetaName& m) const { return compare(m) >  0; }

	void upper7();
	void lower7();
	void printf(const char*, ...);
	size_t copyTo(char* to, size_t toSize) const;

protected:
	static void adjustLength(const char* const s, size_t& l);
};

typedef Pair<Full<MetaName, MetaName> > MetaNamePair;

} // namespace Firebird

#endif // METANAME_H
