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

#include "firebird.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>
#include "../common/classes/alloc.h"
#include "ConfigFile.h"
#include "ConfObject.h"
#include "InputFile.h"
#include "Element.h"
#include "AdminException.h"
#include "ScanDir.h"
#include "../common/config/config.h"

#ifdef _WIN32

#include <windows.h>
#define IS_SEPARATOR(c)		(c == '\\' || c == '/')

#ifndef strcasecmp
#define strcasecmp		stricmp
//#define strncasecmp		strnicmp
#endif

//#define KEY		"SOFTWARE\\Firebird Project\\Firebird Server\\Instances"
//#define SUB_KEY	NULL
//#define PATH_KEY "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\App Paths\\Firebird"

#else
#define IS_SEPARATOR(c)		(c == '/')

#endif

ConfigFile::ConfigFile(const LEX_flags configFlags) :
	Lex ("/<>=", configFlags),
	rootDirectory(getPool()), installDirectory(getPool()), currentDirectory(getPool())
{
	init (configFlags);
}


ConfigFile::ConfigFile(const char* configFile, const LEX_flags configFlags) :
	Lex ("/<>=", configFlags),
	rootDirectory(getPool()), installDirectory(getPool()), currentDirectory(getPool())
{
	init (configFlags);
	InputFile *inputFile = new InputFile;
	Firebird::PathName expandedFile (expand (configFile));

	if (!inputFile->openInputFile(expandedFile.c_str()))
	{
		delete inputFile;
		throw AdminException ("can't open configuration file \"%s\"", configFile);
	}

	pushStream (inputFile);
	parse();
}

void ConfigFile::init(const LEX_flags configFlags)
{
	flags = configFlags;
	setLineComment ("#");
	setContinuationChar ('\\');
	objects = NULL;
	memset (hashTable, 0, sizeof (hashTable));
}

ConfigFile::~ConfigFile()
{
	delete objects;

	for (int n = 0; n < HASH_SIZE; ++n)
	{
		for (Element *element; element = hashTable [n];)
		{
			hashTable [n] = element->sibling;
			delete element;
		}
	}
}

InputFile* ConfigFile::openConfigFile()
{
	fb_assert(false);
	// Vulcan specific code removed
	return NULL;
}

void ConfigFile::addText(const char* text)
{
	InputStream *stream = new InputStream(text);
	pushStream(stream);
	stream->release();
}

void ConfigFile::parse()
{
	objects = new Element ("ConfObjects");
	getToken();

	while (tokenType != END_OF_STREAM)
	{
		while (match ("include"))
		{
			Firebird::PathName fileName = expand (reparseFilename());
			if (fileName.find('*') != Firebird::PathName::npos)
				wildCardInclude(fileName.c_str());
			else
				pushStream (new InputFile (fileName.c_str()));
			getToken();
		}
		if (match ("<"))
			objects->addChild (parseObject());
		else
		{
			Element *element = parseAttribute();
			const int slot = element->name.hash (HASH_SIZE);
			element->sibling = hashTable [slot];
			hashTable [slot] = element;
		}
	}

}

Element* ConfigFile::parseObject()
{
	const Firebird::string name = getName();
	Element *element = new Element (name);
	element->setSource (priorLineNumber, priorInputStream);

	while (!match (">"))
	{
		element->addAttribute (new Element (reparseFilename().ToString()));
		getToken();
	}

	for (;;)
	{
		if (match ("<"))
		{
			if (match ("/"))
			{
				if (!match (element->name.c_str()))
					syntaxError ("closing element");
				if (!match (">"))
					syntaxError ("\">\"");
				element->numberLines = priorLineNumber - element->lineNumber + 1;
				return element;
			}
			element->addChild (parseObject());
		}
		else
			element->addChild (parseAttribute());
	}
}

Element* ConfigFile::parseAttribute()
{
	Element *element = new Element (getName());
	element->setSource (priorLineNumber, priorInputStream);
	match ("=");

	while (!eol)
	{
		element->addAttribute (new Element (reparseFilename().ToString()));
		getToken();
	}

	element->numberLines = priorLineNumber - element->lineNumber + 1;

	return element;
}

ConfObject* ConfigFile::findObject(const char* objectType, const char* objectName)
{
	if (!objects)
		return NULL;

	ConfObject *object = new ConfObject (this);

	for (Element *child = objects->children; child; child = child->sibling)
	{
		if (object->matches (child, objectType, objectName))
			return object;
	}

	object->release();

	return NULL;
}

const char* ConfigFile::getRootDirectory()
{
	return Config::getRootDirectory();
}

ConfObject* ConfigFile::getObject(const char* /*objectType*/)
{
	return new ConfObject (this);
}

Element* ConfigFile::findGlobalAttribute(const char *attributeName)
{
	for (Element *element = hashTable [Firebird::string::hash (attributeName, HASH_SIZE)]; element; element = element->sibling)
	{
		if (element->name == attributeName)
			return element;
	}

	return NULL;
}

const char* ConfigFile::getInstallDirectory()
{
	return Config::getInstallDirectory();
}

Firebird::PathName ConfigFile::expand(const Firebird::PathName& rawString)
{
	const char* const errstr = "filename expansion reached implementation limit at %d";
	char temp [1024];
	char* p = temp;
	const char* const temp_end = temp + sizeof(temp) - 1;
	bool change = false;

	for (const char *s = rawString.c_str(); *s;)
	{
		char c = *s++;
		if (c == '$')
		{
			if (*s == '(')
			{
				++s;
				change = true;
				char name [256];
				const char* const name_end = name + sizeof(name) - 1;
				char* n = name;
				bool overflow = false;
				while (*s && (c = *s++) != ')' && !overflow)
				{
					if (n < name_end)
						*n++ = c;
					else
						overflow = true;
				}
				*n = 0;
				if (overflow)
				{
					n[-3] = n[-2] = n[-1] = '.';
					throw AdminException("name to be substituted \"%s\" is too long", name);
				}
				const char *subst = translate (name, NULL);
				if (!subst)
					throw AdminException ("can't substitute for \"%s\"", name);
				for (const char *t = subst; *t;)
				{
					if (p < temp_end)
						*p++ = *t++;
					else
						throw AdminException(errstr, sizeof(temp) - 1);
				}
			}
		}
		else if (p < temp_end)
			*p++ = c;
		else
			throw AdminException(errstr, sizeof(temp) - 1);
	}

	if (!change)
		return rawString;

	*p = 0;

	return temp;
}

const char* ConfigFile::translate(const char* value, const Element* object)
{
	if (strcasecmp (value, "root") == 0)
		return getRootDirectory();

	if (strcasecmp (value, "install") == 0)
		return getInstallDirectory();

	if (strcasecmp (value, "this") == 0)
	{
		const char *source = NULL;

		if (object && object->inputStream)
			source = object->inputStream->getFileName();

		if (!source && inputStream)
			source = inputStream->getFileName();

		if (!source)
			throw AdminException("no context for $(this)");

		const Firebird::PathName currentFile = expand (source);
		const char *fileName = currentFile.c_str();
		const char *p = NULL;

		for (const char *q = fileName; *q; ++q)
		{
			if (IS_SEPARATOR(*q))
				p = q;
		}

		if (p)
			currentDirectory = Firebird::PathName(fileName, p - fileName);
		else
			currentDirectory = ".";

		return currentDirectory.c_str();
	}

	return NULL;
}


void ConfigFile::wildCardInclude(const char* fileName)
{
	char directory [256];

	if (strlen(fileName) >= sizeof(directory))
		throw AdminException("Too long filename in wildCardInclude()");

	strcpy (directory, fileName);
	const char *wildcard = fileName;
	char *p = strrchr (directory, '/');

	if (p)
	{
		*p = 0;
		wildcard = p + 1;
	}
	else
		directory [0] = 0;

	// OK, expand wildcard

	ScanDir scanDir (directory, wildcard);

	while (scanDir.next())
		pushStream (new InputFile (scanDir.getFilePath()));
}
