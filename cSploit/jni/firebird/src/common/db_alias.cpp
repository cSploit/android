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
#include "../common/config/ConfigCache.h"
#include "../common/config/dir_list.h"
#include "../common/os/path_utils.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/utils_proto.h"
#include "../common/classes/Hash.h"
#include "../common/isc_f_proto.h"
#include <ctype.h>

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

	const char* const ALIAS_FILE = "databases.conf";

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

	template <typename T>
	struct PathHash
	{
		static const PathName& generate(const T& item)
		{
			return item.name;
		}

		static size_t hash(const PathName& value, size_t hashSize)
		{
			return hash(value.c_str(), value.length(), hashSize);
		}

		static void upcpy(size_t* toPar, const char* from, size_t length)
		{
			char* to = reinterpret_cast<char*>(toPar);
			while (length--)
			{
				if (CASE_SENSITIVITY)
				{
					*to++ = *from++;
				}
				else
				{
					*to++ = toupper(*from++);
				}
			}
		}

		static size_t hash(const char* value, size_t length, size_t hashSize)
		{
			size_t sum = 0;
			size_t val;

			while (length >= sizeof(size_t))
			{
				upcpy(&val, value, sizeof(size_t));
				sum += val;
				value += sizeof(size_t);
				length -= sizeof(size_t);
			}

			if (length)
			{
				val = 0;
				upcpy(&val, value, length);
				sum += val;
			}

			size_t rc = 0;
			while (sum)
			{
				rc += (sum % hashSize);
				sum /= hashSize;
			}

			return rc % hashSize;
		}
	};

	struct DbName;
	typedef Hash<DbName, 127, PathName, PathHash<DbName>, PathHash<DbName> > DbHash;
	struct DbName : public DbHash::Entry
	{
		DbName(MemoryPool& p, const PathName& db)
			: name(p, db)
		{ }

		DbName* get()
		{
			return this;
		}

		bool isEqual(const PathName& val) const
		{
			return val == name;
		}

		PathName name;
		RefPtr<Config> config;
	};

	struct AliasName;
	typedef Hash<AliasName, 251, PathName, PathHash<AliasName>, PathHash<AliasName> > AliasHash;

	struct AliasName : public AliasHash::Entry
	{
		AliasName(MemoryPool& p, const PathName& al, DbName* db)
			: name(p, al), database(db)
		{ }

		AliasName* get()
		{
			return this;
		}

		bool isEqual(const PathName& val) const
		{
			return val == name;
		}

		PathName name;
		DbName* database;
	};

	class AliasesConf : public ConfigCache
	{
	public:
		explicit AliasesConf(MemoryPool& p)
			: ConfigCache(p, fb_utils::getPrefix(Firebird::DirType::FB_DIR_CONF, ALIAS_FILE)),
			  databases(getPool()), aliases(getPool())
		{ }

		void loadConfig()
		{
			size_t n;

			// clean old data
			for (n = 0; n < aliases.getCount(); ++n)
			{
				delete aliases[n];
			}
			aliases.clear();
			for (n = 0; n < databases.getCount(); ++n)
			{
				delete databases[n];
			}
			databases.clear();

			ConfigFile aliasConfig(getFileName(), ConfigFile::HAS_SUB_CONF, this);
			const ConfigFile::Parameters& params = aliasConfig.getParameters();

			for (n = 0; n < params.getCount(); ++n)
			{
				const ConfigFile::Parameter* par = &params[n];

				PathName file(par->value.ToPathName());
				replace_dir_sep(file);
				if (PathUtils::isRelative(file))
				{
					gds__log("Value %s configured for alias %s "
						"is not a fully qualified path name, ignored",
								file.c_str(), par->name.c_str());
					continue;
				}
				DbName* db = dbHash.lookup(file);
				if (! db)
				{
					db = FB_NEW(getPool()) DbName(getPool(), file);
					databases.add(db);
					dbHash.add(db);
				}
				else
				{
					// check for duplicated config
					if (par->sub && db->config.hasData())
					{
						fatal_exception::raiseFmt("Duplicated configuration for database %s\n",
												  file.c_str());
					}
				}
				if (par->sub)
				{
					// load per-database configuration
					db->config = new Config(*par->sub, *Config::getDefaultConfig());
				}

				PathName correctedAlias(par->name.ToPathName());
				AliasName* alias = aliasHash.lookup(correctedAlias);
				if (alias)
				{
					fatal_exception::raiseFmt("Duplicated alias %s\n", correctedAlias.c_str());
				}

				alias = FB_NEW(getPool()) AliasName(getPool(), correctedAlias, db);
				aliases.add(alias);
				aliasHash.add(alias);
			}
		}

	private:
		HalfStaticArray<DbName*, 100> databases;
		HalfStaticArray<AliasName*, 200> aliases;

	public:
		DbHash dbHash;
		AliasHash aliasHash;
	};

	InitInstance<AliasesConf> aliasesConf;
}

// Checks that argument doesn't contain colon or directory separator
static inline bool hasSeparator(const PathName& name)
{
	for (const char* p = name.c_str(); *p; p++)
	{
		if (*p == ':' || *p == '/' || *p == '\\')
			return true;
	}
	return false;
}

// Search for 'alias' in databases.conf, return its value in 'file' if found. Else set file to alias.
// Returns true if alias is found in databases.conf.
static bool resolveAlias(const PathName& alias, PathName& file, RefPtr<Config>* config)
{
	PathName correctedAlias = alias;
	replace_dir_sep(correctedAlias);

	AliasName* a = aliasesConf().aliasHash.lookup(correctedAlias);
	DbName* db = a ? a->database : NULL;
	if (db)
	{
		file = db->name;
		if (config)
		{
			*config = db->config.hasData() ? db->config : Config::getDefaultConfig();
		}

		return true;
	}

	return false;
}

// Search for filenames, containing no path component, in dirs from DatabaseAccess list
// of firebird.conf. If not found try first entry in that list as default entry.
// Returns true if expanded successfully.
static bool resolveDatabaseAccess(const PathName& alias, PathName& file)
{
	file = alias;

	if (hasSeparator(alias))
		return false;

	// try to expand to existing file
	if (!databaseDirectoryList().expandFileName(file, alias))
	{
		// try to use default path
		if (!databaseDirectoryList().defaultName(file, alias))
		{
			return false;
		}
	}

	return true;
}

// Set a prefix to a filename based on the ISC_PATH user variable.
// Returns true if database name is expanded using ISC_PATH.
static bool setPath(const PathName& filename, PathName& expandedName)
{
	// Look for the environment variables to tack onto the beginning of the database path.
	PathName pathname;
	if (!fb_utils::readenv("ISC_PATH", pathname))
		return false;

	// If the file already contains a remote node or any path at all forget it.
	if (hasSeparator(filename))
		return false;

	// concatenate the strings

	expandedName = pathname;

	// CVC: Make the concatenation work if no slash is present.
	char lastChar = expandedName[expandedName.length() - 1];
	if (lastChar != ':' && lastChar != '/' && lastChar != '\\')
		expandedName.append(1, PathUtils::dir_sep);

	expandedName.append(filename);

	return true;
}

// Full processing of database name
// Returns true if alias was found in databases.conf
bool expandDatabaseName(Firebird::PathName alias,
						Firebird::PathName& file,
						Firebird::RefPtr<Config>* config)
{
	try
	{
		aliasesConf().checkLoadConfig();
	}
	catch (const fatal_exception& ex)
	{
		gds__log("File databases.conf contains bad data: %s", ex.what());
		Arg::Gds(isc_server_misconfigured).raise();
	}

	// remove whitespaces from database name
	alias.trim();

	ReadLockGuard guard(aliasesConf().rwLock, "expandDatabaseName");

	// First of all check in databases.conf
	if (resolveAlias(alias, file, config))
	{
		return true;
	}

	// Now try ISC_PATH environment variable
	if (!setPath(alias, file))
	{
		// At this step check DatabaseAccess paths in firebird.conf
		if (!resolveDatabaseAccess(alias, file))
		{
			// Last chance - regular filename expansion
			file = alias;

			ISC_systemToUtf8(file);
			ISC_unescape(file);
			ISC_utf8ToSystem(file);

			ISC_expand_filename(file, true);

			ISC_systemToUtf8(file);
			ISC_escape(file);
			ISC_utf8ToSystem(file);
		}
	}

	// Search for correct config in databases.conf
	if (config)
	{
		DbName* db = aliasesConf().dbHash.lookup(file);
		*config = (db && db->config.hasData()) ? db->config : Config::getDefaultConfig();
	}

	return false;
}
