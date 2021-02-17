//____________________________________________________________
//
//		PROGRAM:	C preprocessor
//		MODULE:		hsh.cpp
//		DESCRIPTION:	Hash table and symbol manager
//
//  The contents of this file are subject to the Interbase Public
//  License Version 1.0 (the "License"); you may not use this file
//  except in compliance with the License. You may obtain a copy
//  of the License at http://www.Inprise.com/IPL.html
//
//  Software distributed under the License is distributed on an
//  "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
//  or implied. See the License for the specific language governing
//  rights and limitations under the License.
//
//  The Original Code was created by Inprise Corporation
//  and its predecessors. Portions created by Inprise Corporation are
//  Copyright (C) Inprise Corporation.
//
//  All Rights Reserved.
//  Contributor(s): ______________________________________.
//  TMN (Mike Nordell) 11.APR.2001 - Reduce compiler warnings
//
//
//____________________________________________________________
//
//

#include "firebird.h"
#include "../gpre/gpre.h"
#include "../gpre/hsh_proto.h"
#include "../gpre/gpre_proto.h"
#include "../gpre/msc_proto.h"


static int hash(const SCHAR*);
static bool scompare(const SCHAR*, const SCHAR*);
static bool scompare2(const SCHAR*, const SCHAR*);

const int HASH_SIZE = 211;

static gpre_sym* hash_table[HASH_SIZE];
static gpre_sym* key_symbols;

static struct word
{
	const char* keyword;
	kwwords_t id;
}  keywords[] =
{
#include "../gpre/hsh.h"
};


//____________________________________________________________
//
//		Release space used by keywords.
//

void HSH_fini()
{
	while (key_symbols)
	{
		gpre_sym* symbol = key_symbols;
		key_symbols = (gpre_sym*) key_symbols->sym_object;
		HSH_remove(symbol);
		MSC_free(symbol);
	}
}


//____________________________________________________________
//
//		Initialize the hash table.  This mostly involves
//		inserting all known keywords.
//

void HSH_init()
{
	//const char *string;

	int i = 0;
	for (gpre_sym** ptr = hash_table; i < HASH_SIZE; i++)
		*ptr++ = NULL;

	fflush(stdout);
	const word* a_word;
	for (i = 0, a_word = keywords; i < FB_NELEM(keywords); i++, a_word++)
	{
		// Unused code: SYM_LEN is used always.
		//for (string = a_word->keyword; *string; string++);
		gpre_sym* symbol = (gpre_sym*) MSC_alloc(SYM_LEN);
		symbol->sym_type = SYM_keyword;
		symbol->sym_string = a_word->keyword;
		symbol->sym_keyword = (int) a_word->id;
		HSH_insert(symbol);
		symbol->sym_object = (gpre_ctx*) key_symbols;
		key_symbols = symbol;
	}
}


//____________________________________________________________
//
//		Insert a symbol into the hash table.
//

void HSH_insert( gpre_sym* symbol)
{
	const int h = hash(symbol->sym_string);

	for (gpre_sym** next = &hash_table[h]; *next; next = &(*next)->sym_collision)
	{
		for (const gpre_sym* ptr = *next; ptr; ptr = ptr->sym_homonym)
		{
			if (ptr == symbol)
				return;
		}

		if (scompare(symbol->sym_string, (*next)->sym_string))
		{
			// insert in most recently seen order;
			// This is important for alias resolution in subqueries.
			// BUT insert tokens AFTER keyword!
			// In a lookup, keyword should be found first.
			// This assumes that KEYWORDS are inserted before any other token.
			// No one should be using a keywords as an alias anyway.

			if ((*next)->sym_type == SYM_keyword)
			{
				symbol->sym_homonym = (*next)->sym_homonym;
				symbol->sym_collision = NULL;
				(*next)->sym_homonym = symbol;
			}
			else
			{
				symbol->sym_homonym = *next;
				symbol->sym_collision = (*next)->sym_collision;
				(*next)->sym_collision = NULL;
				*next = symbol;
			}
			return;
		}
	}

	symbol->sym_collision = hash_table[h];
	hash_table[h] = symbol;
}


//____________________________________________________________
//
//		Perform a string lookup against hash table.
//

gpre_sym* HSH_lookup(const SCHAR* string)
{
	for (gpre_sym* symbol = hash_table[hash(string)]; symbol; symbol = symbol->sym_collision)
	{
		if (scompare(string, symbol->sym_string))
			return symbol;
	}

	return NULL;
}

//____________________________________________________________
//
//		Perform a string lookup against hash table.
//       Calls scompare2 which performs case insensitive
//		compare.
//

gpre_sym* HSH_lookup2(const SCHAR* string)
{
	for (gpre_sym* symbol = hash_table[hash(string)]; symbol; symbol = symbol->sym_collision)
	{
		if (scompare2(string, symbol->sym_string))
			return symbol;
	}

	return NULL;
}


//____________________________________________________________
//
//		Remove a symbol from the hash table.
//

void HSH_remove( gpre_sym* symbol)
{
	const int h = hash(symbol->sym_string);

	for (gpre_sym** next = &hash_table[h]; *next; next = &(*next)->sym_collision)
	{
		if (symbol == *next)
		{
			gpre_sym* homonym = symbol->sym_homonym;
			if (homonym)
			{
				homonym->sym_collision = symbol->sym_collision;
				*next = homonym;
			}
			else {
				*next = symbol->sym_collision;
			}
			return;
		}

		for (gpre_sym** ptr = &(*next)->sym_homonym; *ptr; ptr = &(*ptr)->sym_homonym)
		{
			if (symbol == *ptr)
			{
				*ptr = symbol->sym_homonym;
				return;
			}
		}
	}

	CPR_error("HSH_remove failed");
}


//____________________________________________________________
//
//		Returns the hash function of a string.
//

static int hash(const SCHAR* string)
{
	SCHAR c;

	SLONG value = 0;

	while (c = *string++)
		value = (value << 1) + UPPER(c);

	return ((value >= 0) ? value : -value) % HASH_SIZE;
}


//____________________________________________________________
//
//		case sensitive Compare
//

static bool scompare(const SCHAR* string1, const SCHAR* string2)
{
	return strcmp(string1, string2) == 0;
}

//____________________________________________________________
//
//		Compare two strings
//

static bool scompare2(const SCHAR* string1, const SCHAR* string2)
{
	SCHAR c1;

	while (c1 = *string1++)
	{
		SCHAR c2;
		if (!(c2 = *string2++) || (UPPER(c1) != UPPER(c2)))
			return false;
	}

	if (*string2)
		return false;

	return true;
}

