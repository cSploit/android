/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		hsh.cpp
 *	DESCRIPTION:	Hash table and symbol manager
 *
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../qli/dtr.h"
#include "../qli/parse.h"
#include "../qli/all_proto.h"
#include "../qli/err_proto.h"
#include "../qli/hsh_proto.h"

typedef bool (*scompare_t)(const SCHAR*, int, const SCHAR*, int);

static int hash(const SCHAR*, int);
static bool scompare_ins(const SCHAR*, int, const SCHAR*, int);
static bool scompare_sens(const SCHAR*, int, const SCHAR*, int);

const int HASH_SIZE = 224;
static qli_symbol* hash_table[HASH_SIZE];
static qli_symbol* key_symbols;

struct qli_kword
{
	kwwords id;
	const char* keyword;
};

const qli_kword keywords[] =
{
#include "../qli/words.h"
   {KW_continuation, "-\n"}
};


void HSH_fini()
{
/**************************************
 *
 *	H S H _ f i n i
 *
 **************************************
 *
 * Functional description
 *	Release space used by keywords.
 *
 **************************************/
	while (key_symbols)
	{
		qli_symbol* symbol = key_symbols;
		key_symbols = (qli_symbol*) key_symbols->sym_object;
		HSH_remove(symbol);
		ALLQ_release((qli_frb*) symbol);
	}
}


void HSH_init()
{
/**************************************
 *
 *	H S H _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize the hash table.  This mostly involves
 *	inserting all known keywords.
 *
 **************************************/
	const qli_kword* qword = keywords;

	for (int i = 0; i < FB_NELEM(keywords); i++, qword++)
	{
		qli_symbol* symbol = (qli_symbol*) ALLOCPV(type_sym, 0);
		symbol->sym_type = SYM_keyword;
		symbol->sym_length = strlen(qword->keyword);
		symbol->sym_string = qword->keyword;
		symbol->sym_keyword = qword->id;
		HSH_insert(symbol, true);
		symbol->sym_object = (BLK) key_symbols;
		key_symbols = symbol;
	}
}


void HSH_insert( qli_symbol* symbol, bool ignore_case)
{
/**************************************
 *
 *	H S H _ i n s e r t
 *
 **************************************
 *
 * Functional description
 *	Insert a symbol into the hash table.
 *
 **************************************/
	const int h = hash(symbol->sym_string, symbol->sym_length);
	scompare_t scompare = ignore_case ? scompare_ins : scompare_sens;

	for (qli_symbol* old = hash_table[h]; old; old = old->sym_collision)
	{
		if (scompare(symbol->sym_string, symbol->sym_length, old->sym_string, old->sym_length))
		{
			symbol->sym_homonym = old->sym_homonym;
			old->sym_homonym = symbol;
			return;
		}
	}

	symbol->sym_collision = hash_table[h];
	hash_table[h] = symbol;
}


qli_symbol* HSH_lookup(const SCHAR* string, int length)
{
/**************************************
 *
 *	H S H _ l o o k u p
 *
 **************************************
 *
 * Functional description
 *	Perform a string lookup against hash table.
 *
 **************************************/
	scompare_t scompare = scompare_ins;

	if (length > 1 && string[0] == '"')
	{
		// This logic differs from DSQL. See how LEX_token works.
		length -= 2;
		++string;
		scompare = scompare_sens;
	}
	for (qli_symbol* symbol = hash_table[hash(string, length)]; symbol;
		symbol = symbol->sym_collision)
	{
		if (scompare(string, length, symbol->sym_string, symbol->sym_length))
			return symbol;
	}

	return NULL;
}


void HSH_remove( qli_symbol* symbol)
{
/**************************************
 *
 *	H S H _ r e m o v e
 *
 **************************************
 *
 * Functional description
 *	Remove a symbol from the hash table.
 *
 **************************************/
	const int h = hash(symbol->sym_string, symbol->sym_length);

	for (qli_symbol** next = &hash_table[h]; *next; next = &(*next)->sym_collision)
	{
		if (symbol == *next)
		{
			qli_symbol* homonym = symbol->sym_homonym;
			if (homonym)
			{
				homonym->sym_collision = symbol->sym_collision;
				*next = homonym;
			}
			else
				*next = symbol->sym_collision;

			return;
		}

		for (qli_symbol** ptr = &(*next)->sym_homonym; *ptr; ptr = &(*ptr)->sym_homonym)
		{
			if (symbol == *ptr)
			{
				*ptr = symbol->sym_homonym;
				return;
			}
		}
	}

	ERRQ_error(27);	// Msg 27 HSH_remove failed
}


static int hash(const SCHAR* string, int length)
{
/**************************************
 *
 *	h a s h
 *
 **************************************
 *
 * Functional description
 *	Returns the hash function of a string.
 *
 **************************************/
	int value = 0;

	while (length--)
	{
		const UCHAR c = *string++;
		value = (value << 1) + UPPER(c);
	}

	return ((value >= 0) ? value : -value) % HASH_SIZE;
}


static bool scompare_ins(const SCHAR* string1, int length1, const SCHAR* string2, int length2)
{
/**************************************
 *
 *	s c o m p a r e _ i n s
 *
 **************************************
 *
 * Functional description
 *	Compare two strings, case insensitive.
 *
 **************************************/
	if (length1 != length2)
		return false;

	while (length1--)
	{
		const SCHAR c1 = *string1++;
		const SCHAR c2 = *string2++;
		if (c1 != c2 && UPPER(c1) != UPPER(c2))
			return false;
	}

	return true;
}


static bool scompare_sens(const SCHAR* string1, int length1, const SCHAR* string2, int length2)
{
/**************************************
 *
 *	s c o m p a r e _ s e n s
 *
 **************************************
 *
 * Functional description
 *	Compare two strings, case sensitive: quotes identifiers.
 *
 **************************************/
	if (length1 != length2)
		return false;

	return !memcmp(string1, string2, length1);
}

