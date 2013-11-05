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

// Element.h: interface for the Element class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_ELEMENT_H__36AF5EE7_6842_4000_B3C5_DB8DECEC79B7__INCLUDED_)
#define AFX_ELEMENT_H__36AF5EE7_6842_4000_B3C5_DB8DECEC79B7__INCLUDED_

#if defined _MSC_VER  && _MSC_VER >= 1000
#pragma once
#endif // _MSC_VER > 1000

#include "../common/classes/fb_string.h"

START_NAMESPACE

class Stream;
class InputStream;

class Element : public Firebird::GlobalStorage
{
public:
	explicit Element(const Firebird::string& elementName);
	Element (const Firebird::string& elementName, const Firebird::string& elementValue);
	virtual ~Element();

	void addChild (Element *child);
	Element* addChild (const Firebird::string& name);

	void addAttribute (Element *child);
	void addAttribute (const Firebird::string& name);
	Element* addAttribute (const Firebird::string& name, const Firebird::string& value);
	Element* addAttribute (const Firebird::string& name, int value);

	Element* findChild (const char *name, const char *attribute, const char *value);
	void gen (int level, Stream *stream) const;
	void setSource (int line, InputStream *stream);
	const char* getAttributeValue (const char *name, const char *defaultValue);
	const char* getAttributeValue (const char *name);
	const char* getAttributeName (int position) const;
	Element* findChildIgnoreCase (const char *name);
	void indent (int level, Stream *stream) const;
	void genXML (int level, Stream *stream) const;
	Element* findAttribute (const char *name);
	const Element* findAttribute (const char *name) const;
	Element* findAttribute (int seq);
	const Element* findAttribute (int seq) const;
	Element* findChild (const char *name);
	const Element* findChild (const char *name) const;
	void print (int level) const;
	const Element* getAttributes() const { return attributes; }

	Firebird::string	name;
	Firebird::string	value;

	Element		*sibling;
	Element		*children;

	int			lineNumber;
	int			numberLines;
	InputStream	*inputStream;
	static int analyseText(const char* text);
	void putQuotedText(const char* text, Stream* stream) const;
	static int analyzeData(const UCHAR* data);
private:
	void init(const Firebird::string& elementName);
	Firebird::string	innerText;
	Element		*parent; // assigned but never used.
	Element		*attributes;
};

END_NAMESPACE

#endif // !defined(AFX_ELEMENT_H__36AF5EE7_6842_4000_B3C5_DB8DECEC79B7__INCLUDED_)
