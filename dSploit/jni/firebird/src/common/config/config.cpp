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

#include "../../common/config/config.h"
#include "../../common/config/config_impl.h"
#include "../../common/config/config_file.h"
#include "../../common/classes/init.h"

#ifdef HAVE_STDLIB_H
#include <stdlib.h>
#endif

// config_file works with OS case-sensitivity
typedef Firebird::PathName string;

/******************************************************************************
 *
 *	Configuration entries
 */

const char*	GCPolicyCooperative	= "cooperative";
const char*	GCPolicyBackground	= "background";
const char*	GCPolicyCombined	= "combined";
#ifdef SUPERSERVER
const char*	GCPolicyDefault	= GCPolicyCombined;
#else
const char*	GCPolicyDefault	= GCPolicyCooperative;
#endif

const char*	AmNative	= "native";
const char*	AmTrusted	= "trusted";
const char*	AmMixed		= "mixed";

const ConfigImpl::ConfigEntry ConfigImpl::entries[] =
{
	{TYPE_STRING,		"RootDirectory",			(ConfigValue) 0},
	{TYPE_INTEGER,		"TempBlockSize",			(ConfigValue) 1048576},		// bytes
#ifdef SUPERSERVER
	{TYPE_INTEGER,		"TempCacheLimit",			(ConfigValue) 67108864},	// bytes
#elif defined(WIN_NT) // win32 CS
	{TYPE_INTEGER,		"TempCacheLimit",			(ConfigValue) 8388608},		// bytes
#else // non-win32 CS
	{TYPE_INTEGER,		"TempCacheLimit",			(ConfigValue) 0},			// bytes
#endif
#ifdef BOOT_BUILD
	{TYPE_BOOLEAN,		"RemoteFileOpenAbility",	(ConfigValue) true},
#else
	{TYPE_BOOLEAN,		"RemoteFileOpenAbility",	(ConfigValue) false},
#endif
	{TYPE_INTEGER,		"GuardianOption",			(ConfigValue) 1},
	{TYPE_INTEGER,		"CpuAffinityMask",			(ConfigValue) 1},
	{TYPE_INTEGER,		"TcpRemoteBufferSize",		(ConfigValue) 8192},		// bytes
	{TYPE_BOOLEAN,		"TcpNoNagle",				(ConfigValue) true},
#ifdef SUPERSERVER
	{TYPE_INTEGER,		"DefaultDbCachePages",		(ConfigValue) 2048},		// pages
#else
	{TYPE_INTEGER,		"DefaultDbCachePages",		(ConfigValue) 75},			// pages
#endif
	{TYPE_INTEGER,		"ConnectionTimeout",		(ConfigValue) 180},			// seconds
	{TYPE_INTEGER,		"DummyPacketInterval",		(ConfigValue) 0},			// seconds
	{TYPE_INTEGER,		"LockMemSize",				(ConfigValue) 1048576},		// bytes
	{TYPE_BOOLEAN,		"LockGrantOrder",			(ConfigValue) true},
	{TYPE_INTEGER,		"LockHashSlots",			(ConfigValue) 1009},		// slots
	{TYPE_INTEGER,		"LockAcquireSpins",			(ConfigValue) 0},
	{TYPE_INTEGER,		"EventMemSize",				(ConfigValue) 65536},		// bytes
	{TYPE_INTEGER,		"DeadlockTimeout",			(ConfigValue) 10},			// seconds
	{TYPE_INTEGER,		"PrioritySwitchDelay",		(ConfigValue) 100},			// milliseconds
	{TYPE_BOOLEAN,		"UsePriorityScheduler",		(ConfigValue) true},
	{TYPE_INTEGER,		"PriorityBoost",			(ConfigValue) 5},			// ratio oh high- to low-priority thread ticks in jrd.cpp
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
	{TYPE_BOOLEAN,		"CompleteBooleanEvaluation", (ConfigValue) false},
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
	{TYPE_BOOLEAN,		"LegacyHash",				(ConfigValue) true},	// let use old passwd hash verification
	{TYPE_STRING,		"GCPolicy",					(ConfigValue) GCPolicyDefault},	// garbage collection policy
	{TYPE_BOOLEAN,		"Redirection",				(ConfigValue) false},
	{TYPE_BOOLEAN,		"OldColumnNaming",			(ConfigValue) false},	// if true use old style concatenation
	{TYPE_STRING,		"Authentication",			(ConfigValue) AmNative},	// use native, trusted or mixed
	{TYPE_INTEGER,		"DatabaseGrowthIncrement",	(ConfigValue) 128 * 1048576},	// bytes
	{TYPE_INTEGER,		"FileSystemCacheThreshold",	(ConfigValue) 65536},	// page buffers
	{TYPE_BOOLEAN,		"RelaxedAliasChecking",		(ConfigValue) false},	// if true relax strict alias checking rules in DSQL a bit
	{TYPE_BOOLEAN,		"OldSetClauseSemantics",	(ConfigValue) false},	// if true disallow SET A = B, B = A to exchange column values
	{TYPE_STRING,		"AuditTraceConfigFile",		(ConfigValue) ""},		// location of audit trace configuration file
	{TYPE_INTEGER,		"MaxUserTraceLogSize",		(ConfigValue) 10},		// maximum size of user session trace log
	{TYPE_INTEGER,		"FileSystemCacheSize",		(ConfigValue) 0}		// percent
};

/******************************************************************************
 *
 *	Static instance of the system configuration file
 */

static Firebird::InitInstance<ConfigImpl> sysConfig;

/******************************************************************************
 *
 *	Implementation interface
 */

ConfigImpl::ConfigImpl(MemoryPool& p) : ConfigRoot(p), confMessage(p)
{
	// Prepare some stuff

	ConfigFile file(p);
	root_dir = getRootDirectory();
	const int size = FB_NELEM(entries);
	values = FB_NEW(p) ConfigValue[size];

	//string val_sep = ",";
	file.setConfigFilePath(getConfigFilePath());

	// Iterate through the known configuration entries

	for (int i = 0; i < size; i++)
	{
		const ConfigEntry entry = entries[i];
		const string value = getValue(file, entries[i].key);

		if (!value.length())
		{
			// Assign the default value

			values[i] = entries[i].default_value;
			continue;
		}

		// Assign the actual value

		switch (entry.data_type)
		{
		case TYPE_BOOLEAN:
			values[i] = (ConfigValue) asBoolean(value);
			break;
		case TYPE_INTEGER:
			values[i] = (ConfigValue) asInteger(value);
			break;
		case TYPE_STRING:
			{
				const char* src = asString(value);
				char* dst = FB_NEW(p) char[strlen(src) + 1];
				strcpy(dst, src);
				values[i] = (ConfigValue) dst;
			}
			break;
		//case TYPE_STRING_VECTOR:
		//	break;
		}
	}

	if (file.getMessage())
	{
		confMessage = file.getMessage();
	}
}

ConfigImpl::~ConfigImpl()
{
	const int size = FB_NELEM(entries);

	// Free allocated memory

	for (int i = 0; i < size; i++)
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
	delete[] values;
}

string ConfigImpl::getValue(ConfigFile& file, const ConfigKey key)
{
	return file.doesKeyExist(key) ? file.getString(key) : "";
}

int ConfigImpl::asInteger(const string &value)
{
	return atoi(value.data());
}

bool ConfigImpl::asBoolean(const string &value)
{
	return (atoi(value.data()) != 0);
}

const char* ConfigImpl::asString(const string &value)
{
	return value.c_str();
}

/******************************************************************************
 *
 *	Public interface
 */

const char* Config::getMessage()
{
	return sysConfig().getMessage();
}

const char* Config::getInstallDirectory()
{
	return sysConfig().getInstallDirectory();
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
	// must check it here - command line must override any other root settings, including firebird.conf
	if (rootFromCommandLine)
	{
		return rootFromCommandLine->c_str();
	}

	const char* result = (char*) sysConfig().values[KEY_ROOT_DIRECTORY];
	return result ? result : sysConfig().root_dir;
}

int Config::getTempBlockSize()
{
	return (int) sysConfig().values[KEY_TEMP_BLOCK_SIZE];
}

int Config::getTempCacheLimit()
{
	int v = (int) sysConfig().values[KEY_TEMP_CACHE_LIMIT];
	return v < 0 ? 0 : v;
}

bool Config::getRemoteFileOpenAbility()
{
	return (bool) sysConfig().values[KEY_REMOTE_FILE_OPEN_ABILITY];
}

int Config::getGuardianOption()
{
	return (int) sysConfig().values[KEY_GUARDIAN_OPTION];
}

int Config::getCpuAffinityMask()
{
	return (int) sysConfig().values[KEY_CPU_AFFINITY_MASK];
}

int Config::getTcpRemoteBufferSize()
{
	int rc = (int) sysConfig().values[KEY_TCP_REMOTE_BUFFER_SIZE];
	if (rc < 1448)
		rc = 1448;
	if (rc > MAX_SSHORT)
		rc = MAX_SSHORT;
	return rc;
}

bool Config::getTcpNoNagle()
{
	return (bool) sysConfig().values[KEY_TCP_NO_NAGLE];
}

int Config::getDefaultDbCachePages()
{
	return (int) sysConfig().values[KEY_DEFAULT_DB_CACHE_PAGES];
}

int Config::getConnectionTimeout()
{
	return (int) sysConfig().values[KEY_CONNECTION_TIMEOUT];
}

int Config::getDummyPacketInterval()
{
	return (int) sysConfig().values[KEY_DUMMY_PACKET_INTERVAL];
}

int Config::getLockMemSize()
{
	return (int) sysConfig().values[KEY_LOCK_MEM_SIZE];
}

bool Config::getLockGrantOrder()
{
	return (bool) sysConfig().values[KEY_LOCK_GRANT_ORDER];
}

int Config::getLockHashSlots()
{
	return (int) sysConfig().values[KEY_LOCK_HASH_SLOTS];
}

int Config::getLockAcquireSpins()
{
	return (int) sysConfig().values[KEY_LOCK_ACQUIRE_SPINS];
}

int Config::getEventMemSize()
{
	return (int) sysConfig().values[KEY_EVENT_MEM_SIZE];
}

int Config::getDeadlockTimeout()
{
	return (int) sysConfig().values[KEY_DEADLOCK_TIMEOUT];
}

int Config::getPrioritySwitchDelay()
{
	int rc = (int) sysConfig().values[KEY_PRIORITY_SWITCH_DELAY];
	if (rc < 1)
		rc = 1;
	return rc;
}

int Config::getPriorityBoost()
{
	int rc = (int) sysConfig().values[KEY_PRIORITY_BOOST];
	if (rc < 1)
		rc = 1;
	if (rc > 1000)
		rc = 1000;
	return rc;
}

bool Config::getUsePriorityScheduler()
{
	return (bool) sysConfig().values[KEY_USE_PRIORITY_SCHEDULER];
}

const char *Config::getRemoteServiceName()
{
	return (const char*) sysConfig().values[KEY_REMOTE_SERVICE_NAME];
}

unsigned short Config::getRemoteServicePort()
{
	return (unsigned short) sysConfig().values[KEY_REMOTE_SERVICE_PORT];
}

const char *Config::getRemotePipeName()
{
	return (const char*) sysConfig().values[KEY_REMOTE_PIPE_NAME];
}

const char *Config::getIpcName()
{
	return (const char*) sysConfig().values[KEY_IPC_NAME];
}

int Config::getMaxUnflushedWrites()
{
	return (int) sysConfig().values[KEY_MAX_UNFLUSHED_WRITES];
}

int Config::getMaxUnflushedWriteTime()
{
	return (int) sysConfig().values[KEY_MAX_UNFLUSHED_WRITE_TIME];
}

int Config::getProcessPriorityLevel()
{
	return (int) sysConfig().values[KEY_PROCESS_PRIORITY_LEVEL];
}

bool Config::getCompleteBooleanEvaluation()
{
	return (bool) sysConfig().values[KEY_COMPLETE_BOOLEAN_EVALUATION];
}

int Config::getRemoteAuxPort()
{
	return (int) sysConfig().values[KEY_REMOTE_AUX_PORT];
}

const char *Config::getRemoteBindAddress()
{
	return (const char*) sysConfig().values[KEY_REMOTE_BIND_ADDRESS];
}

const char *Config::getExternalFileAccess()
{
	return (const char*) sysConfig().values[KEY_EXTERNAL_FILE_ACCESS];
}

const char *Config::getDatabaseAccess()
{
	return (const char*) sysConfig().values[KEY_DATABASE_ACCESS];
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

	Firebird::MutexLockGuard guard(udfMutex);

	if (value)
	{
		return value;
	}

	const char* v = (const char*) sysConfig().values[KEY_UDF_ACCESS];
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
	return (const char*) sysConfig().values[KEY_TEMP_DIRECTORIES];
}

bool Config::getBugcheckAbort()
{
	return (bool) sysConfig().values[KEY_BUGCHECK_ABORT];
}

bool Config::getLegacyHash()
{
	return (bool) sysConfig().values[KEY_LEGACY_HASH];
}

const char *Config::getGCPolicy()
{
	return (const char *) sysConfig().values[KEY_GC_POLICY];
}

bool Config::getRedirection()
{
	return (bool) sysConfig().values[KEY_REDIRECTION];
}

bool Config::getOldColumnNaming()
{
	return (bool) sysConfig().values[KEY_OLD_COLUMN_NAMING];
}

const char *Config::getAuthMethod()
{
	return (const char *) sysConfig().values[KEY_AUTH_METHOD];
}

int Config::getDatabaseGrowthIncrement()
{
	return (int) sysConfig().values[KEY_DATABASE_GROWTH_INCREMENT];
}

int Config::getFileSystemCacheThreshold()
{
	return (int) sysConfig().values[KEY_FILESYSTEM_CACHE_THRESHOLD];
}

bool Config::getRelaxedAliasChecking()
{
	return (bool) sysConfig().values[KEY_RELAXED_ALIAS_CHECKING];
}

bool Config::getOldSetClauseSemantics()
{
	return (bool) sysConfig().values[KEY_OLD_SET_CLAUSE_SEMANTICS];
}

int Config::getFileSystemCacheSize()
{
	return (int) sysConfig().values[KEY_FILESYSTEM_CACHE_SIZE];
}

const char *Config::getAuditTraceConfigFile()
{
	return (const char*) sysConfig().values[KEY_TRACE_CONFIG];
}

int Config::getMaxUserTraceLogSize()
{
	return (int) sysConfig().values[KEY_MAX_TRACELOG_SIZE];
}
