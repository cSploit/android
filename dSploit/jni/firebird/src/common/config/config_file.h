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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2002 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef CONFIG_CONFIG_FILE_H
#define CONFIG_CONFIG_FILE_H

#include "../../common/classes/alloc.h"
#include "../../common/classes/fb_pair.h"
#include "../../common/classes/objects_array.h"
#include "../common/classes/fb_string.h"

/**
	Since the original (isc.cpp) code wasn't able to provide powerful and
	easy-to-use abilities to work with complex configurations, a decision
	has been made to create a completely new one.

	The below class implements generic file abstraction for new configuration
	manager. It allows "value-by-key" retrieval based on plain text files. Both
	keys and values are just strings that may be handled case-sensitively or
	case-insensitively, depending on host OS. The configuration file is loaded
	on demand, its current status can be checked with isLoaded() member function.
	All leading and trailing spaces are ignored. Comments (follow after a
	hash-mark) are ignored as well.

	Now this implementation is used by generic configuration manager
	(common/config/config.cpp) and server-side alias manager (jrd/db_alias.cpp).
**/

class ConfigFile : public Firebird::AutoStorage
{
	// config_file works with OS case-sensitivity
	typedef Firebird::PathName string;

	typedef Firebird::Pair<Firebird::Full<string, string> > Parameter;

    typedef Firebird::SortedObjectsArray <Parameter,
		Firebird::InlineStorage<Parameter *, 100>,
		string, Firebird::FirstPointerKey<Parameter> > mymap_t;

public:
	explicit ConfigFile(MemoryPool& p)
		: AutoStorage(p),
		  configFile(getPool()),
		  lastMessage(getPool()),
		  isLoadedFlg(false),
		  parsingAliases(false),
		  parameters(getPool())
		{ }
	explicit ConfigFile(const bool useForAliases)
		: AutoStorage(),
		  configFile(getPool()),
		  lastMessage(getPool()),
		  isLoadedFlg(false),
		  parsingAliases(useForAliases),
		  parameters(getPool())
		{ }

	// configuration file management
    const string getConfigFilePath() const { return configFile; }
    void setConfigFilePath(const string& newFile) { configFile = newFile; }

    bool isLoaded() const { return isLoadedFlg; }

    void loadConfig();
    void checkLoadConfig();

	// key and value management
    bool doesKeyExist(const string&);
    string getString(const string&);

	// utilities
	bool stripComments(string&) const;
	static string parseKeyFrom(const string&, string::size_type&);
	string parseValueFrom(string, string::size_type);

	// was there some error parsing config file?
	const char* getMessage();

private:
    string configFile;
    string lastMessage;
    bool isLoadedFlg;
	const bool parsingAliases;
    mymap_t parameters;
};

#endif	// CONFIG_CONFIG_FILE_H
