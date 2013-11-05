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

#ifndef _CONFOBJECT_H_
#define _CONFOBJECT_H_

#include "../common/classes/fb_string.h"
#include "../vulcan/RefObject.h"

START_NAMESPACE

class Element;
class ConfigFile;


class ConfObject : public RefObject, public Firebird::GlobalStorage
{
public:
	explicit ConfObject(ConfigFile *confFile);
	virtual ~ConfObject();

	virtual const char*	getValue(const char* option, const char *defaultValue);
	virtual int			getValue(const char* option, int defaultValue);
	virtual bool		getValue(const char* option, bool defaultValue);
	virtual const char*	getValue(int instanceNumber, const char* attributeName);
	virtual bool		matches(Element* element, const char* type, const char* string);
	virtual void		setChain(ConfObject* object);
	virtual const char* getName();
	virtual const char*	getConcatenatedValues(const char* attributeName);
	virtual Firebird::string	expand(const char* rawValue);
	virtual ConfObject*	findObject(const char* objectType, const char* objectName);
	virtual ConfObject* getChain();

protected:
	virtual void		putString(int position, const char* string, int stringLength);
	virtual bool		match(int position, const char* p1, const char* p2);
	virtual Firebird::string	getValue(const char* attributeName);
	virtual Element*	findAttribute(const char* attributeName);
	virtual const char* getValue(const Element* attribute);

public:
	Element*			object;

private:
	enum { MAX_STRINGS = 32 };
	ConfObject*			chain;
	ConfigFile*			configFile;
	Firebird::string	source;
	Firebird::string	tempValue;
	int					numberStrings;
	const char*			strings [MAX_STRINGS];
	char				buffer [1024];
	char*				next;
	const char*			end;
};

END_NAMESPACE

#endif
