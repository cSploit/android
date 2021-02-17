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

#include "../common/config/config.h"
#include "../common/config/config_file.h"
#include "../common/config/os/config_root.h"
#include "../common/classes/init.h"
#include "../common/dllinst.h"
#include "../common/os/fbsyslog.h"
#include "../jrd/EngineInterface.h"
#include "firebird/Plugin.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

namespace {

/******************************************************************************
 *
 *	firebird.conf implementation
 */

class ConfigImpl : public Firebird::PermanentStorage
{
public:
	explicit ConfigImpl(Firebird::MemoryPool& p)
		: Firebird::PermanentStorage(p), missConf(false)
	{
		try
		{
			ConfigFile file(fb_utils::getPrefix(Firebird::DirType::FB_DIR_CONF, CONFIG_FILE),
				ConfigFile::ERROR_WHEN_MISS);
			defaultConfig = new Config(file);
		}
		catch (const Firebird::status_exception& ex)
		{
			if (ex.value()[1] != isc_miss_config)
			{
				throw;
			}

			missConf = true;

			ConfigFile file(ConfigFile::USE_TEXT, "");
			defaultConfig = new Config(file);
		}
	}

/*	void changeDefaultConfig(Config* newConfig)
	{
		defaultConfig = newConfig;
	}
 */
	const Firebird::RefPtr<Config>& getDefaultConfig() const
	{
		return defaultConfig;
	}

	bool missFirebirdConf() const
	{
		return missConf;
	}

	Firebird::IFirebirdConf* getFirebirdConf()
	{
		Firebird::IFirebirdConf* rc = new FirebirdConf(defaultConfig);
		rc->addRef();
		return rc;
	}

private:
	Firebird::RefPtr<Config> defaultConfig;

    ConfigImpl(const ConfigImpl&);
    void operator=(const ConfigImpl&);

	bool missConf;
};

/******************************************************************************
 *
 *	Static instance of the system configuration file
 */

Firebird::InitInstance<ConfigImpl> firebirdConf;

/******************************************************************************
 *
 *	Static instance of the root and install directories detector
 */

Firebird::InitInstance<ConfigRoot> rootDetector;

}	// anonymous namespace


Firebird::IFirebirdConf* getFirebirdConfig()
{
	return firebirdConf().getFirebirdConf();
}

/******************************************************************************
 *
 *	Configuration entries
 */

const char*	GCPolicyCooperative	= "cooperative";
const char*	GCPolicyBackground	= "background";
const char*	GCPolicyCombined	= "combined";


const Config::ConfigEntry Config::entries[MAX_CONFIG_KEY] =
{
	{TYPE_INTEGER,		"TempBlockSize",			(ConfigValue) 1048576},		// bytes
	{TYPE_INTEGER,		"TempCacheLimit",			(ConfigValue) -1},			// bytes
	{TYPE_BOOLEAN,		"RemoteFileOpenAbility",	(ConfigValue) false},
	{TYPE_INTEGER,		"GuardianOption",			(ConfigValue) 1},
	{TYPE_INTEGER,		"CpuAffinityMask",			(ConfigValue) 0},
	{TYPE_INTEGER,		"TcpRemoteBufferSize",		(ConfigValue) 8192},		// bytes
	{TYPE_BOOLEAN,		"TcpNoNagle",				(ConfigValue) true},
	{TYPE_INTEGER,		"DefaultDbCachePages",		(ConfigValue) -1},			// pages
	{TYPE_INTEGER,		"ConnectionTimeout",		(ConfigValue) 180},			// seconds
	{TYPE_INTEGER,		"DummyPacketInterval",		(ConfigValue) 0},			// seconds
	{TYPE_INTEGER,		"LockMemSize",				(ConfigValue) 1048576},		// bytes
	{TYPE_INTEGER,		"LockHashSlots",			(ConfigValue) 8191},		// slots
	{TYPE_INTEGER,		"LockAcquireSpins",			(ConfigValue) 0},
	{TYPE_INTEGER,		"EventMemSize",				(ConfigValue) 65536},		// bytes
	{TYPE_INTEGER,		"DeadlockTimeout",			(ConfigValue) 10},			// seconds
	{TYPE_STRING,		"RemoteServiceName",		(ConfigValue) FB_SERVICE_NAME},
	{TYPE_INTEGER,		"RemoteServicePort",		(ConfigValue) 0},
	{TYPE_STRING,		"RemotePipeName",			(ConfigValue) FB_PIPE_NAME},
	{TYPE_STRING,		"IpcName",					(ConfigValue) FB_IPC_NAME},
#ifdef WIN_NT
	{TYPE_INTEGER,		"MaxUnflushedWrites",		(ConfigValue) 100},
	{TYPE_INTEGER,		"MaxUnflushedWriteTime",	(ConfigValue) 5},
#else
	{TYPE_INTEGER,		"MaxUnflushedWrites",		(ConfigValue) -1},
	{TYPE_INTEGER,		"MaxUnflushedWriteTime",	(ConfigValue) -1},
#endif
	{TYPE_INTEGER,		"ProcessPriorityLevel",		(ConfigValue) 0},
	{TYPE_INTEGER,		"RemoteAuxPort",			(ConfigValue) 0},
	{TYPE_STRING,		"RemoteBindAddress",		(ConfigValue) 0},
	{TYPE_STRING,		"ExternalFileAccess",		(ConfigValue) "None"},	// location(s) of external files for tables
	{TYPE_STRING,		"DatabaseAccess",			(ConfigValue) "Full"},	// location(s) of databases
#define UDF_DEFAULT_CONFIG_VALUE "Restrict UDF"
	{TYPE_STRING,		"UdfAccess",				(ConfigValue) UDF_DEFAULT_CONFIG_VALUE},	// location(s) of UDFs
	{TYPE_STRING,		"TempDirectories",			(ConfigValue) 0},
#ifdef DEV_BUILD
 	{TYPE_BOOLEAN,		"BugcheckAbort",			(ConfigValue) true},	// whether to abort() engine when internal error is found
#else
 	{TYPE_BOOLEAN,		"BugcheckAbort",			(ConfigValue) false},	// whether to abort() engine when internal error is found
#endif
	{TYPE_INTEGER,		"TraceDSQL",				(ConfigValue) 0},		// bitmask
	{TYPE_BOOLEAN,		"LegacyHash",				(ConfigValue) true},	// let use old passwd hash verification
	{TYPE_STRING,		"GCPolicy",					(ConfigValue) NULL},	// garbage collection policy
	{TYPE_BOOLEAN,		"Redirection",				(ConfigValue) false},
	{TYPE_INTEGER,		"DatabaseGrowthIncrement",	(ConfigValue) 128 * 1048576},	// bytes
	{TYPE_INTEGER,		"FileSystemCacheThreshold",	(ConfigValue) 65536},	// page buffers
	{TYPE_BOOLEAN,		"RelaxedAliasChecking",		(ConfigValue) false},	// if true relax strict alias checking rules in DSQL a bit
	{TYPE_STRING,		"AuditTraceConfigFile",		(ConfigValue) ""},		// location of audit trace configuration file
	{TYPE_INTEGER,		"MaxUserTraceLogSize",		(ConfigValue) 10},		// maximum size of user session trace log
	{TYPE_INTEGER,		"FileSystemCacheSize",		(ConfigValue) 0},		// percent
	{TYPE_STRING,		"Providers",				(ConfigValue) "Remote, " CURRENT_ENGINE ", Loopback"},
	{TYPE_STRING,		"AuthServer",				(ConfigValue) "Srp"},
	{TYPE_STRING,		"AuthClient",				(ConfigValue) "Srp, Win_Sspi, Legacy_Auth"},
	{TYPE_STRING,		"UserManager",				(ConfigValue) "Srp"},
	{TYPE_STRING,		"TracePlugin",				(ConfigValue) "fbtrace"},
	{TYPE_STRING,		"SecurityDatabase",			(ConfigValue) "$(root)/security3.fdb"},	// security database name
	{TYPE_BOOLEAN,		"SharedCache",				(ConfigValue) true},
	{TYPE_BOOLEAN,		"SharedDatabase",			(ConfigValue) false},
	{TYPE_STRING,		"WireCrypt",				(ConfigValue) NULL},
	{TYPE_STRING,		"WireCryptPlugin",			(ConfigValue) "Arc4"},
	{TYPE_STRING,		"KeyHolderPlugin",			(ConfigValue) ""},
	{TYPE_BOOLEAN,		"RemoteAccess",				(ConfigValue) true}
};

/******************************************************************************
 *
 *	Config routines
 */

Config::Config(const ConfigFile& file)
{
	// Array to save string temporarily
	// Will be finally saved by loadValues() in the end of ctor
	Firebird::ObjectsArray<ConfigFile::String> tempStrings(getPool());

	// Iterate through the known configuration entries
	for (unsigned int i = 0; i < MAX_CONFIG_KEY; i++)
	{
		values[i] = entries[i].default_value;
		if (entries[i].data_type == TYPE_STRING && values[i])
		{
			ConfigFile::String expand((const char*)values[i]);
			if (file.macroParse(expand, NULL) && expand != (const char*) values[i])
			{
				ConfigFile::String& saved(tempStrings.add());
				saved = expand;
				values[i] = (ConfigValue) saved.c_str();
			}
		}
	}

	loadValues(file);
}

Config::Config(const ConfigFile& file, const Config& base)
{
	// Iterate through the known configuration entries

	for (unsigned int i = 0; i < MAX_CONFIG_KEY; i++)
	{
		values[i] = base.values[i];
	}

	loadValues(file);
}

void Config::merge(Firebird::RefPtr<Config>& config, const Firebird::string* dpbConfig)
{
	if (dpbConfig && dpbConfig->hasData())
	{
		ConfigFile txtStream(ConfigFile::USE_TEXT, dpbConfig->c_str());
		config = new Config(txtStream, *(config.hasData() ? config : getDefaultConfig()));
	}
}

void Config::loadValues(const ConfigFile& file)
{
	// Iterate through the known configuration entries

	for (int i = 0; i < MAX_CONFIG_KEY; i++)
	{
		const ConfigEntry& entry = entries[i];
		const ConfigFile::Parameter* par = file.findParameter(entry.key);

		if (par)
		{
			// Assign the actual value

			switch (entry.data_type)
			{
			case TYPE_BOOLEAN:
				values[i] = (ConfigValue) par->asBoolean();
				break;
			case TYPE_INTEGER:
				values[i] = (ConfigValue) par->asInteger();
				break;
			case TYPE_STRING:
				values[i] = (ConfigValue) par->value.c_str();
				break;
			//case TYPE_STRING_VECTOR:
			//	break;
			}
		}

		if (entry.data_type == TYPE_STRING && values[i] != entry.default_value)
		{
			const char* src = (const char*) values[i];
			char* dst = FB_NEW(getPool()) char[strlen(src) + 1];
			strcpy(dst, src);
			values[i] = (ConfigValue) dst;
		}
	}
}

Config::~Config()
{
	// Free allocated memory

	for (int i = 0; i < MAX_CONFIG_KEY; i++)
	{
		if (values[i] == entries[i].default_value)
			continue;

		switch (entries[i].data_type)
		{
		case TYPE_STRING:
			delete[] (char*) values[i];
			break;
		//case TYPE_STRING_VECTOR:
		//	break;
		}
	}
}


/******************************************************************************
 *
 *	Public interface
 */

const Firebird::RefPtr<Config>& Config::getDefaultConfig()
{
	return firebirdConf().getDefaultConfig();
}

bool Config::missFirebirdConf()
{
	return firebirdConf().missFirebirdConf();
}

const char* Config::getInstallDirectory()
{
	return rootDetector().getInstallDirectory();
}

static Firebird::PathName* rootFromCommandLine = 0;

void Config::setRootDirectoryFromCommandLine(const Firebird::PathName& newRoot)
{
	delete rootFromCommandLine;
	rootFromCommandLine = FB_NEW(*getDefaultMemoryPool())
		Firebird::PathName(*getDefaultMemoryPool(), newRoot);
}

const Firebird::PathName* Config::getCommandLineRootDirectory()
{
	return rootFromCommandLine;
}

const char* Config::getRootDirectory()
{
	// must check it here - command line must override any other root settings
	if (rootFromCommandLine)
	{
		return rootFromCommandLine->c_str();
	}

	return rootDetector().getRootDirectory();;
}


unsigned int Config::getKeyByName(ConfigName nm)
{
	ConfigFile::KeyType name(nm);
	for (unsigned int i = 0; i < MAX_CONFIG_KEY; i++)
	{
		if (name == entries[i].key)
		{
			return i;
		}
	}

	return ~0;
}

SINT64 Config::getInt(unsigned int key) const
{
	if (key >= MAX_CONFIG_KEY)
		return 0;
	return get<SINT64>(static_cast<ConfigKey>(key));
}

const char* Config::getString(unsigned int key) const
{
	if (key >= MAX_CONFIG_KEY)
		return NULL;
	return get<const char*>(static_cast<ConfigKey>(key));
}

bool Config::getBoolean(unsigned int key) const
{
	if (key >= MAX_CONFIG_KEY)
		return false;
	return get<bool>(static_cast<ConfigKey>(key));
}


int Config::getTempBlockSize()
{
	return (int) getDefaultConfig()->values[KEY_TEMP_BLOCK_SIZE];
}

FB_UINT64 Config::getTempCacheLimit()
{
	SINT64 v = (SINT64) getDefaultConfig()->values[KEY_TEMP_CACHE_LIMIT];
	if (v < 0)
	{
		v = getSharedDatabase() ? 8388608 : 67108864;	// bytes
	}
	return v;
}

bool Config::getRemoteFileOpenAbility()
{
	return fb_utils::bootBuild() ? true : ((bool) getDefaultConfig()->values[KEY_REMOTE_FILE_OPEN_ABILITY]);
}

int Config::getGuardianOption()
{
	return (int) getDefaultConfig()->values[KEY_GUARDIAN_OPTION];
}

int Config::getCpuAffinityMask()
{
	return (int) getDefaultConfig()->values[KEY_CPU_AFFINITY_MASK];
}

int Config::getTcpRemoteBufferSize()
{
	int rc = (int) getDefaultConfig()->values[KEY_TCP_REMOTE_BUFFER_SIZE];
	if (rc < 1448)
		rc = 1448;
	if (rc > MAX_SSHORT)
		rc = MAX_SSHORT;
	return rc;
}

bool Config::getTcpNoNagle() const
{
	return get<bool>(KEY_TCP_NO_NAGLE);
}

int Config::getDefaultDbCachePages() const
{
	int rc = get<int>(KEY_DEFAULT_DB_CACHE_PAGES);
	if (rc < 0)
	{
		rc = getSharedDatabase() ? 256 : 2048;	// pages
	}
	return rc;
}

int Config::getConnectionTimeout() const
{
	return get<int>(KEY_CONNECTION_TIMEOUT);
}

int Config::getDummyPacketInterval() const
{
	return get<int>(KEY_DUMMY_PACKET_INTERVAL);
}

int Config::getLockMemSize() const
{
	int size = get<int>(KEY_LOCK_MEM_SIZE);
	if (size < 64 * 1024)
		size = 64 * 1024;
	return size;
}

int Config::getLockHashSlots() const
{
	return get<int>(KEY_LOCK_HASH_SLOTS);
}

int Config::getLockAcquireSpins() const
{
	return get<int>(KEY_LOCK_ACQUIRE_SPINS);
}

int Config::getEventMemSize() const
{
	return get<int>(KEY_EVENT_MEM_SIZE);
}

int Config::getDeadlockTimeout() const
{
	return get<int>(KEY_DEADLOCK_TIMEOUT);
}

const char *Config::getRemoteServiceName() const
{
	return get<const char*>(KEY_REMOTE_SERVICE_NAME);
}

unsigned short Config::getRemoteServicePort() const
{
	return get<unsigned short>(KEY_REMOTE_SERVICE_PORT);
}

const char *Config::getRemotePipeName() const
{
	return get<const char*>(KEY_REMOTE_PIPE_NAME);
}

const char *Config::getIpcName() const
{
	return get<const char*>(KEY_IPC_NAME);
}

int Config::getMaxUnflushedWrites() const
{
	return get<int>(KEY_MAX_UNFLUSHED_WRITES);
}

int Config::getMaxUnflushedWriteTime() const
{
	return get<int>(KEY_MAX_UNFLUSHED_WRITE_TIME);
}

int Config::getProcessPriorityLevel()
{
	return (int) getDefaultConfig()->values[KEY_PROCESS_PRIORITY_LEVEL];
}

int Config::getRemoteAuxPort() const
{
	return get<int>(KEY_REMOTE_AUX_PORT);
}

const char *Config::getRemoteBindAddress()
{
	return (const char*) getDefaultConfig()->values[KEY_REMOTE_BIND_ADDRESS];
}

const char *Config::getExternalFileAccess() const
{
	return get<const char*>(KEY_EXTERNAL_FILE_ACCESS);
}

const char *Config::getDatabaseAccess()
{
	return (const char*) getDefaultConfig()->values[KEY_DATABASE_ACCESS];
}

const char *Config::getUdfAccess()
{
	static Firebird::GlobalPtr<Firebird::Mutex> udfMutex;
	static Firebird::GlobalPtr<Firebird::string> udfValue;
	static const char* volatile value = 0;

	if (value)
	{
		return value;
	}

	Firebird::MutexLockGuard guard(udfMutex, "Config::getUdfAccess");

	if (value)
	{
		return value;
	}

	const char* v = (const char*) getDefaultConfig()->values[KEY_UDF_ACCESS];
	if (CASE_SENSITIVITY ? (! strcmp(v, UDF_DEFAULT_CONFIG_VALUE) && FB_UDFDIR[0]) :
						   (! fb_utils::stricmp(v, UDF_DEFAULT_CONFIG_VALUE) && FB_UDFDIR[0]))
	{
		udfValue->printf("Restrict %s", FB_UDFDIR);
		value = udfValue->c_str();
	}
	else
	{
		value = v;
	}
	return value;
}

const char *Config::getTempDirectories()
{
	return (const char*) getDefaultConfig()->values[KEY_TEMP_DIRECTORIES];
}

bool Config::getBugcheckAbort()
{
	return (bool) getDefaultConfig()->values[KEY_BUGCHECK_ABORT];
}

int Config::getTraceDSQL()
{
	return (int) getDefaultConfig()->values[KEY_TRACE_DSQL];
}

bool Config::getLegacyHash()
{
	return (bool) getDefaultConfig()->values[KEY_LEGACY_HASH];
}

const char *Config::getGCPolicy() const
{
	const char* rc = get<const char*>(KEY_GC_POLICY);

	if (rc)
	{
		if (strcmp(rc, GCPolicyCooperative) != 0 &&
			strcmp(rc, GCPolicyBackground) != 0 &&
			strcmp(rc, GCPolicyCombined) != 0)
		{
			// user-provided value is invalid - fail to default
			rc = NULL;
		}
	}

	if (! rc)
	{
		rc = getSharedCache() ? GCPolicyCombined : GCPolicyCooperative;
	}

	return rc;
}

bool Config::getRedirection()
{
	return (bool) getDefaultConfig()->values[KEY_REDIRECTION];
}

int Config::getDatabaseGrowthIncrement() const
{
	return get<int>(KEY_DATABASE_GROWTH_INCREMENT);
}

int Config::getFileSystemCacheThreshold() const
{
	int rc = get<int>(KEY_FILESYSTEM_CACHE_THRESHOLD);
	return rc < 0 ? 0 : rc;
}

bool Config::getRelaxedAliasChecking()
{
	return (bool) getDefaultConfig()->values[KEY_RELAXED_ALIAS_CHECKING];
}

FB_UINT64 Config::getFileSystemCacheSize()
{
	return (FB_UINT64)(SINT64) getDefaultConfig()->values[KEY_FILESYSTEM_CACHE_SIZE];
}

const char *Config::getAuditTraceConfigFile()
{
	return (const char*) getDefaultConfig()->values[KEY_TRACE_CONFIG];
}

FB_UINT64 Config::getMaxUserTraceLogSize()
{
	return (FB_UINT64)(SINT64) getDefaultConfig()->values[KEY_MAX_TRACELOG_SIZE];
}

bool Config::getSharedCache()
{
	return (bool) getDefaultConfig()->values[KEY_SHARED_CACHE];
}

bool Config::getSharedDatabase()
{
	return (bool) getDefaultConfig()->values[KEY_SHARED_DATABASE];
}

const char* Config::getPlugins(unsigned int type) const
{
	switch (type)
	{
		case Firebird::PluginType::Provider:
			return (const char*) values[KEY_PLUG_PROVIDERS];
		case Firebird::PluginType::AuthServer:
			return (const char*) values[KEY_PLUG_AUTH_SERVER];
		case Firebird::PluginType::AuthClient:
			return (const char*) values[KEY_PLUG_AUTH_CLIENT];
		case Firebird::PluginType::AuthUserManagement:
			return (const char*) values[KEY_PLUG_AUTH_MANAGE];
		case Firebird::PluginType::Trace:
			return (const char*) values[KEY_PLUG_TRACE];
		case Firebird::PluginType::WireCrypt:
			return (const char*) values[KEY_PLUG_WIRE_CRYPT];
		case Firebird::PluginType::KeyHolder:
			return (const char*) values[KEY_PLUG_KEY_HOLDER];
	}

	(Firebird::Arg::Gds(isc_random) << "Internal error in Config::getPlugins(): unknown plugin type requested").raise();
	return NULL;		// compiler warning silencer
}

unsigned int FB_CARG FirebirdConf::getKey(const char* name)
{
	return Config::getKeyByName(name);
}

ISC_INT64 FB_CARG FirebirdConf::asInteger(unsigned int key)
{
	return config->getInt(key);
}

const char* FB_CARG FirebirdConf::asString(unsigned int key)
{
	return config->getString(key);
}

FB_BOOLEAN FB_CARG FirebirdConf::asBoolean(unsigned int key)
{
	return config->getBoolean(key);
}

int FB_CARG FirebirdConf::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}

	return 1;
}

const char* Config::getSecurityDatabase() const
{
	return get<const char*>(KEY_SECURITY_DATABASE);
}

int Config::getWireCrypt(WireCryptMode wcMode) const
{
	const char* wc = get<const char*>(KEY_WIRE_CRYPT);
	if (!wc)
	{
		return wcMode == WC_CLIENT ? WIRE_CRYPT_ENABLED : WIRE_CRYPT_REQUIRED;
	}

	Firebird::NoCaseString wireCrypt(wc);
	if (wireCrypt == "DISABLED")
		return WIRE_CRYPT_DISABLED;
	if (wireCrypt == "ENABLED")
		return WIRE_CRYPT_ENABLED;

	// the safest choice
	return WIRE_CRYPT_REQUIRED;
}

bool Config::getRemoteAccess() const
{
	return get<bool>(KEY_REMOTE_ACCESS);
}
