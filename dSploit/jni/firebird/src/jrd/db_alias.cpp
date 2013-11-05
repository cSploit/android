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

#include "firebird.h"
#include "../common/classes/init.h"
#include "../common/config/config.h"
#include "../common/config/config_file.h"
#include "../common/config/dir_list.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/gds_proto.h"
#include "../common/utils_proto.h"

using namespace Firebird;

namespace
{
	class DatabaseDirectoryList : public DirectoryList
	{
	private:
		const PathName getConfigString() const
		{
			return PathName(Config::getDatabaseAccess());
		}
	public:
		explicit DatabaseDirectoryList(MemoryPool& p)
			: DirectoryList(p)
		{
			initialize();
		}
	};
	InitInstance<DatabaseDirectoryList> databaseDirectoryList;

	const char* const ALIAS_FILE = "aliases.conf";

	void replace_dir_sep(PathName& s)
	{
		const char correct_dir_sep = PathUtils::dir_sep;
		const char incorrect_dir_sep = (correct_dir_sep == '/') ? '\\' : '/';
		for (char* itr = s.begin(); itr < s.end(); ++itr)
		{
			if (*itr == incorrect_dir_sep)
			{
				*itr = correct_dir_sep;
			}
		}
	}
}

bool ResolveDatabaseAlias(const PathName& alias, PathName& database)
{
	PathName alias_filename = fb_utils::getPrefix(fb_utils::FB_DIR_CONF, ALIAS_FILE);
	ConfigFile aliasConfig(true);
	aliasConfig.setConfigFilePath(alias_filename);

	PathName corrected_alias = alias;
	replace_dir_sep(corrected_alias);

	database = aliasConfig.getString(corrected_alias);

	if (!database.empty())
	{
		replace_dir_sep(database);
		if (PathUtils::isRelative(database))
		{
			gds__log("Value %s configured for alias %s "
				"is not a fully qualified path name, ignored",
						database.c_str(), alias.c_str());
			return false;
		}
		return true;
	}

	// If file_name has no path part, expand it in DatabasesPath
	Firebird::PathName path, name;
	PathUtils::splitLastComponent(path, name, corrected_alias);

	// if path component not present in file_name
	if (path.isEmpty())
	{
		// try to expand to existing file
		if (databaseDirectoryList().expandFileName(database, name))
		{
			return true;
		}
		// try to use default path
		if (databaseDirectoryList().defaultName(database, name))
		{
			return true;
		}
	}

	return false;
}
