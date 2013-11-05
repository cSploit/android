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

#ifndef CONFIG_IMPL_H
#define CONFIG_IMPL_H

#include "fb_types.h"
#include "../../common/classes/fb_string.h"

#include "../../common/config/config.h"
#include "../../common/config/config_file.h"
#include "../jrd/os/config_root.h"

/******************************************************************************
 *
 *	Main implementation class
 */

class ConfigImpl : public ConfigRoot
{
	friend class Config;
	// config_file works with OS case-sensitivity
	typedef Firebird::PathName string;

	enum ConfigType
	{
		TYPE_BOOLEAN,
		TYPE_INTEGER,
		TYPE_STRING
		//TYPE_STRING_VECTOR // CVC: Unused
	};

	typedef const char* ConfigKey;
	typedef IPTR ConfigValue;

	struct ConfigEntry
	{
		ConfigType data_type;
		ConfigKey key;
		ConfigValue default_value;
	};

public:
	explicit ConfigImpl(MemoryPool& p);
    ~ConfigImpl();

	static string getValue(ConfigFile&, const ConfigKey);

	static int asInteger(const string&);
	static bool asBoolean(const string&);
	static const char* asString(const string&);

	const char* getMessage()
	{
		return confMessage.nullStr();
	}

private:
	static const ConfigEntry entries[];
	const char *root_dir;
	ConfigValue *values;

    ConfigImpl(const ConfigImpl&);
    void operator=(const ConfigImpl&);

	Firebird::string confMessage;
};

#endif // CONFIG_IMPL_H
