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

// Element.cpp: implementation of the Element class.
//
//////////////////////////////////////////////////////////////////////

#include <stdio.h>
#include <string.h>
//#include <stdlib.h>
//#include "Engine.h"
#include "firebird.h"
#include "../common/classes/alloc.h"
#include "Element.h"
#include "Stream.h"
#include "InputStream.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

static const int quoted = 1;
static const int illegal = 2;

static int charTable [256];

inline void setCharTable(UCHAR pos, int val)
{
	charTable[pos] = val;
}

static int init();
static int foo = init();

int init()
{
	setCharTable('<', quoted);
	setCharTable('>', quoted);
	setCharTable('&', quoted);

	for (int n = 0; n < 10; ++n)
		charTable[n] = illegal;

	return 0;
}

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Element::Element(const Firebird::string& elementName) :
	name(getPool()), value(getPool()), innerText(getPool())
{
	init (elementName);
}

Element::Element(const Firebird::string& elementName, const Firebird::string& elementValue) :
	name(getPool()), value(getPool()), innerText(getPool())
{
	init (elementName);
	value = elementValue;
}


void Element::init(const Firebird::string& elementName)
{
	name = elementName;
	attributes = NULL;
	sibling = NULL;
	children = NULL;
	parent = NULL;
	lineNumber = 0;
	numberLines = 0;
	inputStream = NULL;
}

Element::~Element()
{
	Element *child;

	while (child = children)
	{
		children = child->sibling;
		delete child;
	}

	while (child = attributes)
	{
		attributes = child->sibling;
		delete child;
	}

	if (inputStream)
		inputStream->release();
}


void Element::addChild(Element *child)
{
	child->parent = this;
	child->sibling = NULL;
	Element** ptr = &children;
	while (*ptr)
		ptr = &(*ptr)->sibling;

	*ptr = child;
}

Element* Element::addChild(const Firebird::string& childName)
{
	Element *element = new Element (childName);
	addChild (element);

	return element;
}

void Element::addAttribute(Element *child)
{
	child->parent = this;
	child->sibling = NULL;
	Element** ptr = &attributes;
	while (*ptr)
		ptr = &(*ptr)->sibling;

	*ptr = child;
}

void Element::addAttribute(const Firebird::string& attributeName)
{
	addAttribute (new Element (attributeName));
}

Element* Element::addAttribute(const Firebird::string& attributeName, const Firebird::string& attributeValue)
{
	Element *attribute = new Element (attributeName, attributeValue);
	addAttribute (attribute);

	return attribute;
}

Element* Element::addAttribute(const Firebird::string& attributeName, int attributeValue)
{
	Firebird::string buffer;
	buffer.printf ("%d", attributeValue);

	return addAttribute (attributeName, buffer);
}

void Element::print(int level) const
{
	printf ("%*s%s", level * 3, "", name.c_str());
	const Element *element;

	for (element = attributes; element; element = element->sibling)
	{
		printf (" %s", element->name.c_str());
		if (element->value != "")
			printf ("=%s", (const char*) element->value.c_str());
	}

	printf ("\n");
	++level;
	for (element = children; element; element = element->sibling)
		element->print (level);
}

Element* Element::findChild(const char *childName)
{
	for (Element *child = children; child; child = child->sibling)
	{
		if (child->name == childName)
			return child;
	}

	return NULL;
}

const Element* Element::findChild(const char *childName) const
{
	for (const Element *child = children; child; child = child->sibling)
	{
		if (child->name == childName)
			return child;
	}

	return NULL;
}

Element* Element::findAttribute(const char *childName)
{
	for (Element *child = attributes; child; child = child->sibling)
	{
		if (child->name == childName)
			return child;
	}

	return NULL;
}

const Element* Element::findAttribute(const char *childName) const
{
	for (const Element *child = attributes; child; child = child->sibling)
	{
		if (child->name == childName)
			return child;
	}

	return NULL;
}

Element* Element::findAttribute(int seq)
{
	int n = 0;

	for (Element *attribute = attributes; attribute; attribute = attribute->sibling)
	{
		if (n++ == seq)
			return attribute;
	}

	return NULL;
}

const Element* Element::findAttribute(int seq) const
{
	int n = 0;

	for (const Element *attribute = attributes; attribute; attribute = attribute->sibling)
	{
		if (n++ == seq)
			return attribute;
	}

	return NULL;
}

void Element::genXML(int level, Stream *stream) const
{
	indent (level, stream);
	stream->putCharacter ('<');
	stream->putSegment (name.c_str());

	for (const Element *attribute = attributes; attribute; attribute = attribute->sibling)
	{
		stream->putCharacter (' ');
		stream->putSegment (attribute->name.c_str());
		stream->putSegment ("=\"");
		for (const char *p = attribute->value.c_str(); *p; ++p)
		{
			switch (*p)
			{
				case '"':	stream->putSegment ("&quot;"); break;
				case '\'':	stream->putSegment ("&apos;"); break;
				case '&':	stream->putSegment ("&amp;"); break;
				case '<':	stream->putSegment ("&lt;"); break;
				case '>':	stream->putSegment ("&gt;"); break;
				default:	stream->putCharacter (*p); break;
			}
		}
		//stream->putSegment (attribute->value);
		stream->putCharacter ('"');
	}

	if (innerText.hasData())
	{
		stream->putCharacter('>');
		putQuotedText(innerText.c_str(), stream);
	}
	else if (!children)
	{
		if (name.at(0) == '?')
			stream->putSegment ("?>\n");
		else
			stream->putSegment ("/>\n");
		return;
	}
	else
		stream->putSegment (">\n");

	++level;

	for (const Element *child = children; child; child = child->sibling)
		child->genXML (level, stream);

	if (innerText.isEmpty())
		indent (level - 1, stream);

	stream->putSegment ("</");
	stream->putSegment (name.c_str());
	stream->putSegment (">\n");
}

void Element::indent(int level, Stream *stream) const
{
	int count = level * 3;

	for (int n = 0; n < count; ++n)
		stream->putCharacter (' ');
}

Element* Element::findChildIgnoreCase(const char *childName)
{
	for (Element *child = children; child; child = child->sibling)
	{
		if (child->name.equalsNoCase (childName))
			return child;
	}

	return NULL;
}

const char* Element::getAttributeName(int position) const
{
	const Element *element = findAttribute (position);

	if (!element)
		return NULL;

	return element->name.c_str();
}

const char* Element::getAttributeValue(const char *attributeName)
{
	return getAttributeValue (attributeName, NULL);
}

const char* Element::getAttributeValue(const char *attributeName, const char *defaultValue)
{
	Element *attribute = findAttribute (attributeName);

	if (!attribute)
		return defaultValue;

	return attribute->value.c_str();
}

void Element::setSource(int line, InputStream *stream)
{
	lineNumber = line;
	inputStream = stream;
	inputStream->addRef();
}

void Element::gen(int level, Stream *stream) const
{
	for (int n = 0; n < level; ++n)
		stream->putSegment ("   ");

	if (children)
		stream->putCharacter ('<');

	stream->putSegment (name.c_str());
	const Element *element;

	for (element = attributes; element; element = element->sibling)
	{
		stream->putCharacter (' ');
		stream->putSegment (element->name.c_str());
		if (element->value != "")
		{
			stream->putCharacter ('=');
			stream->putSegment (element->value.c_str());
		}
	}

	if (!children)
	{
		stream->putCharacter ('\n');
		return;
	}

	stream->putSegment (">\n");
	++level;

	for (element = children; element; element = element->sibling)
		element->gen (level, stream);

	stream->putSegment ("</");
	stream->putSegment (name.c_str());
	stream->putSegment (">\n");
}

Element* Element::findChild(const char *childName, const char *attribute, const char *attributeValue)
{
	for (Element *child = children; child; child = child->sibling)
	{
		if (child->name == childName)
		{
			const char *p = child->getAttributeValue (attribute, NULL);
			if (p && strcmp (p, attributeValue) == 0)
				return child;
		}
	}

	return NULL;
}

int Element::analyseText(const char* text)
{
	int count = 0;

	for (const char *p = text; *p; p++)
	{
		const int n = charTable[(UCHAR) *p];

		if (n)
		{
			if (n & illegal)
				return -1;

			++count;
		}
	}

	return count;
}

void Element::putQuotedText(const char* text, Stream* stream) const
{
	const char *start = text;
	const char *p;

	for (p = text; *p; p++)
	{
		if (charTable[(UCHAR) *p])
		{
			const char* escape = NULL;
			switch (*p)
			{
			case '>':
				escape = "&gt;";
				break;
			case '<':
				escape = "&lt;";
				break;
			case '&':
				escape = "&amp;";
				break;
			default:
				continue;
			}

			if (p > start)
				stream->putSegment(p - start, start, true);

			stream->putSegment(escape);
			start = p + 1;
		}
	}

	if (p > start)
		stream->putSegment(p - start, start, true);
}

int Element::analyzeData(const UCHAR* bytes)
{
	int count = 0;

	for (const UCHAR *p = bytes; *p; p++)
	{
		const int n = charTable[*p];

		if (n)
		{
			if (n & illegal)
				return -1;

			++count;
		}
	}

	return count;
}
