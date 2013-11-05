/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 */

#include <string.h>
#include <memory.h>
#include "firebird.h"
#include "../jrd/common.h"
#include "ConfObject.h"
#include "Element.h"
#include "AdminException.h"
#include "ConfigFile.h"
#include "PathName.h"

#ifdef _WIN32
#ifndef strcasecmp
#define strcasecmp		stricmp
//#define strncasecmp		strnicmp
#endif
#endif

#define IS_DIGIT(c)		(c >= '0' && c <= '9')

struct BooleanName
{
	const char	*string;
	bool		value;
};

static const BooleanName booleanNames [] =
{
	{"yes",		true},
	{"true",	true},
	{"false",	false},
	{"no",		false},
	{NULL}
};

ConfObject::ConfObject(ConfigFile *confFile) :
	source(getPool()), tempValue(getPool())
{
	configFile = confFile;
	configFile->addRef();
	object = NULL;
	chain = NULL;
}

ConfObject::~ConfObject()
{
	configFile->release();

	if (chain)
		chain->release();
}

bool ConfObject::matches(Element* element, const char* type, const char* string)
{
	if (element->name != type)
		return false;

	const Element *attribute = element->findAttribute (0);

	if (!attribute)
		return false;

	//const char *name = configFile->translate (attribute->name, attribute->name);
	const Firebird::string name = expand (attribute->name.c_str());
	numberStrings = 0;
	end = buffer + sizeof (buffer);
	next = buffer;

	if (!match (0, name.c_str(), string))
		return false;

	object = element;
	source = string;

	return true;
}

void ConfObject::putString(int position, const char* string, int stringLength)
{
	if (position >= MAX_STRINGS)
		throw AdminException("ConfObject: string segments overflow");

	strings [position] = next;

	if (next + stringLength + 1 >= end)
		throw AdminException ("ConfObject: string overflow");

	memcpy (next, string, stringLength);
	next [stringLength] = 0;
	next += stringLength + 1;

	if (position >= numberStrings)
		numberStrings = position + 1;
}

bool ConfObject::match(int position, const char* pattern, const char* string)
{
	char c;
	const char *s = string;

	for (const char *p = pattern; (c = *p++); ++s)
	{
		if (c == '*')
		{
			if (!*p)
			{
				putString (position, string, (int) strlen (string));
				return true;
			}
			for (; *s; ++s)
			{
				if (match (position + 1,  pattern + 1, s))
				{
					putString (position, string, (int) (s - string));
					return true;
				}
			}
			return false;
		}

		if (!*s)
			return false;

		if (c != '%' && c != *s)
		{
#ifdef _WIN32
			if (UPPER (c) == UPPER (*s))
				continue;
			if ((c == '/' || c == '\\') && (*s == '/' || *s == '\\'))
				continue;
#endif
			return false;
		}
	}

	if (c || *s)
		return false;

	putString (position, string, (int) strlen (string));

	return true;
}

const char* ConfObject::getValue (const char* option, const char *defaultValue)
{
	const Element *element = findAttribute (option);

	if (!element)
		return defaultValue;

	tempValue = expand (getValue (element));

	return tempValue.c_str();
}


int ConfObject::getValue(const char* option, int defaultValue)
{
	Element *element = findAttribute (option);

	if (!element)
		return defaultValue;

	const Firebird::string value (expand (getValue (element)));
	int n = 0;

	for (const char *p = value.c_str(); *p;)
	{
		const char c = *p++;
		if (c >= '0' && c <= '9')
			n = n * 10 + c - '0';
		else
			throw AdminException ("expected numeric value for option \"%s\", got \"%s\"", option, value.c_str());
	}

	return n;
}

bool ConfObject::getValue(const char* option, bool defaultValue)
{
	const Element *element = findAttribute (option);

	if (!element)
		return defaultValue;

	const Firebird::string value = expand (getValue (element));
	const char* valueName = value.c_str();

	for (const BooleanName *name = booleanNames; name->string; ++name)
	{
		if (strcasecmp (name->string, valueName) == 0)
			return name->value;
	}

	throw AdminException ("expected boolean value for option \"%s\", got \"%s\"", option, value.c_str());
}

Firebird::string ConfObject::expand(const char* rawValue)
{
	if (!rawValue)
		return "";

	char temp [1024];
	char *p = temp;
	const char* const temp_end = temp + sizeof(temp) - 1;
	bool changed = false;

	// FIX THIS: detect overflow instead of silenty truncating content, as it's done in ConfigFile::expand
	for (const char *s = rawValue; *s;)
	{
		char c = *s++;
		if (c == '$')
		{
			if (*s == '(')
			{
				++s;
				char name [256];
				char* n = name;
				while (*s && (c = *s++) != ')' && n < name + sizeof(name) - 1)
					*n++ = c;
				*n = 0;
				const char *subst = configFile->translate (name, object);
				if (!subst)
					throw AdminException ("can't substitute for \"%s\"", name);
				changed = true;
				for (const char *t = subst; *t && p < temp_end;)
					*p++ = *t++;
			}
			else
			{
				int n = 0;
				while (IS_DIGIT (*s))
					n = n * 10 + *s++ - '0';
				if (n > numberStrings)
					throw AdminException ("substitution index exceeds available segments");
				for (const char *t = (n == 0) ? source.c_str() : strings [n - 1]; *t && p < temp_end;)
					*p++ = *t++;
			}
		}
		else if (p < temp_end)
			*p++ = c;
	}

	*p = 0;

	if (!changed)
		return temp;

	return PathName::expandFilename (temp);
}

Firebird::string ConfObject::getValue(const char* attributeName)
{
	const Element *attribute = findAttribute (attributeName);

	if (!attribute)
		return "";

	return expand (getValue (attribute));
}

const char* ConfObject::getValue(int instanceNumber, const char* attributeName)
{
	const Element *attribute = findAttribute (attributeName);

	if (!attribute)
		return "";

	const Element *val = attribute->findAttribute (instanceNumber);

	if (!val)
		return "";

	tempValue = expand (val->name.c_str());

	return tempValue.c_str();
}

const char* ConfObject::getConcatenatedValues(const char* attributeName)
{
	const Element *attribute = findAttribute (attributeName);

	if (!attribute)
		return "";

	Firebird::string value;

	for (const Element *att = attribute->getAttributes(); att; att = att->sibling)
	{
		if (value.hasData())
			value += " ";
		value += att->name;
	}

	tempValue = value;

	return tempValue.c_str();
}

Element* ConfObject::findAttribute(const char* attributeName)
{
	if (object)
	{
		Element *element = object->findChild (attributeName);
		if (element)
			return element;
	}

	if (chain)
		return chain->findAttribute (attributeName);

	return configFile->findGlobalAttribute (attributeName);
}

ConfObject* ConfObject::getChain()
{
	return chain;
}

void ConfObject::setChain(ConfObject* obj)
{
	if (chain)
		chain->release();

	if (chain = obj)
		chain->addRef();
}

const char* ConfObject::getValue(const Element* attribute)
{
	if (!attribute)
		return NULL;

	const Element *value = attribute->findAttribute (0);

	if (!value)
		return NULL;

	return value->name.c_str();
}

const char* ConfObject::getName()
{
	if (!object)
		return NULL;

	const Element *attribute = object->findAttribute (0);

	if (!attribute)
		return NULL;

	return attribute->name.c_str();
}

ConfObject* ConfObject::findObject(const char* objectType, const char* objectName)
{
	return configFile->findObject(objectType, objectName);
}

