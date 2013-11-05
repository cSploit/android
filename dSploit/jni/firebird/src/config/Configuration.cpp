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
#include "../common/classes/alloc.h"
#include "Configuration.h"
#include "ConfigFile.h"

// This var could be a static member of the class.
static ConfigFile* configFile;

// Non MT-safe code follows, maybe it was intended?

Configuration::Configuration()
{
	fb_assert(!configFile); // only one instance of Configuration at a given time.
	configFile = NULL;
}

Configuration::~Configuration()
{
	if (configFile)
	{
		configFile->release();
		configFile = NULL;
	}
}

ConfObject* Configuration::findObject(const char* objectType, const char* objectName)
{
	if (!configFile)
		loadConfigFile();

	return configFile->findObject (objectType, objectName);
}

const char* Configuration::getRootDirectory()
{
	if (!configFile)
		loadConfigFile();

	return configFile->getRootDirectory();
}

void Configuration::loadConfigFile()
{
	if (!configFile)
		configFile = new ConfigFile (ConfigFile::LEX_none);
}

void Configuration::setConfigFilePath(const char* filename)
{
	if (!configFile)
		configFile = new ConfigFile (filename, ConfigFile::LEX_none);
}

ConfObject* Configuration::getObject(const char* objectType)
{
	if (!configFile)
		loadConfigFile();

	return configFile->getObject (objectType);
}

ConfObject* Configuration::getObject(const char* objectType, const char* objectName)
{
	ConfObject* object = findObject (objectType, objectName);

	if (!object)
		object = getObject (objectType);

	return object;
}
