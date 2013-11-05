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

#ifndef COMMON_CONFIG_H
#define COMMON_CONFIG_H

#include "../common/classes/fb_string.h"
#include "../jrd/os/path_utils.h"

/**
	Since the original (isc.cpp) code wasn't able to provide powerful and
	easy-to-use abilities to work with complex configurations, a decision
	has been made to create a completely new one.

	This class is a public interface for our generic configuration manager
	and allows to access all configuration values by its getXXX() member
	functions. Each of these functions corresponds to one and only one key
	and has one input argument - default value, which is used when the
	requested key is missing or the configuration file is not found. Supported
	value datatypes are "const char*", "int" and "bool". Usual default values for
	these datatypes are empty string, zero and false respectively. There are
	two types of member functions - scalar and vector. The former ones return
	single value of the given type. The latter ones return vector which
	contains an ordered array of values.

	There's one exception - getRootDirectory() member function, which returns
	root pathname of the current installation. This value isn't stored in the
	configuration file, but is managed by the code itself. But there's a way
	to override this value via the configuration file as well.

	To add new configuration item, you have to take the following steps:

		1. Add key description to ConfigImpl::entries[] array (config.cpp)
		2. Add logical key to Config::ConfigKey enumeration (config.h)
		   (note: both physical and logical keys MUST have the same ordinal
				  position within appropriate structures)
		3. Add member function to Config class (config.h) and implement it
		   in config.cpp module.
**/

extern const char*	GCPolicyCooperative;
extern const char*	GCPolicyBackground;
extern const char*	GCPolicyCombined;
extern const char*	GCPolicyDefault;

extern const char*	AmNative;
extern const char*	AmTrusted;
extern const char*	AmMixed;

enum AmCache {AM_UNKNOWN, AM_DISABLED, AM_ENABLED};

class Config
{
	enum ConfigKey
	{
		KEY_ROOT_DIRECTORY,							// 0
		KEY_TEMP_BLOCK_SIZE,						// 1
		KEY_TEMP_CACHE_LIMIT,						// 2
		KEY_REMOTE_FILE_OPEN_ABILITY,				// 3
		KEY_GUARDIAN_OPTION,						// 4
		KEY_CPU_AFFINITY_MASK,						// 5
		KEY_TCP_REMOTE_BUFFER_SIZE,					// 6
		KEY_TCP_NO_NAGLE,							// 7
		KEY_DEFAULT_DB_CACHE_PAGES,					// 8
		KEY_CONNECTION_TIMEOUT,						// 9
		KEY_DUMMY_PACKET_INTERVAL,					// 10
		KEY_LOCK_MEM_SIZE,							// 11
		KEY_LOCK_GRANT_ORDER,						// 12
		KEY_LOCK_HASH_SLOTS,						// 13
		KEY_LOCK_ACQUIRE_SPINS,						// 14
		KEY_EVENT_MEM_SIZE,							// 15
		KEY_DEADLOCK_TIMEOUT,						// 16
		KEY_PRIORITY_SWITCH_DELAY,					// 17
		KEY_USE_PRIORITY_SCHEDULER,					// 18
		KEY_PRIORITY_BOOST,							// 19
		KEY_REMOTE_SERVICE_NAME,					// 20
		KEY_REMOTE_SERVICE_PORT,					// 21
		KEY_REMOTE_PIPE_NAME,						// 22
		KEY_IPC_NAME,								// 23
		KEY_MAX_UNFLUSHED_WRITES,					// 24
		KEY_MAX_UNFLUSHED_WRITE_TIME,				// 25
		KEY_PROCESS_PRIORITY_LEVEL,					// 26
		KEY_COMPLETE_BOOLEAN_EVALUATION,			// 27
		KEY_REMOTE_AUX_PORT,						// 28
		KEY_REMOTE_BIND_ADDRESS,					// 29
		KEY_EXTERNAL_FILE_ACCESS,					// 30
		KEY_DATABASE_ACCESS,						// 31
		KEY_UDF_ACCESS,								// 32
		KEY_TEMP_DIRECTORIES,						// 33
 		KEY_BUGCHECK_ABORT,							// 34
		KEY_LEGACY_HASH,							// 35
		KEY_GC_POLICY,								// 36
		KEY_REDIRECTION,							// 37
		KEY_OLD_COLUMN_NAMING,						// 38
		KEY_AUTH_METHOD,							// 39
		KEY_DATABASE_GROWTH_INCREMENT,				// 40
		KEY_FILESYSTEM_CACHE_THRESHOLD,				// 41
		KEY_RELAXED_ALIAS_CHECKING,					// 42
		KEY_OLD_SET_CLAUSE_SEMANTICS,				// 43
		KEY_TRACE_CONFIG,							// 44
		KEY_MAX_TRACELOG_SIZE,						// 45
		KEY_FILESYSTEM_CACHE_SIZE					// 46
	};

public:

	// Check for errors in .conf file

	static const char* getMessage();

	// Interface to support command line root specification.

	// This ugly solution was required to make it possible to specify root
	// in command line to load firebird.conf from that root, though in other
	// cases firebird.conf may be also used to specify root.

	static void setRootDirectoryFromCommandLine(const Firebird::PathName& newRoot);
	static const Firebird::PathName* getCommandLineRootDirectory();

	// Installation directory
	static const char* getInstallDirectory();

	// Root directory of current installation
	static const char* getRootDirectory();

	// Allocation chunk for the temporary spaces
	static int getTempBlockSize();

	// Caching limit for the temporary data
	static int getTempCacheLimit();

	// Whether remote (NFS) files can be opened
	static bool getRemoteFileOpenAbility();

	// Startup option for the guardian
	static int getGuardianOption();

	// CPU affinity mask
	static int getCpuAffinityMask();

	// XDR buffer size
	static int getTcpRemoteBufferSize();

	// Disable Nagle algorithm
	static bool getTcpNoNagle();

	// Default database cache size
	static int getDefaultDbCachePages();

	// Connection timeout
	static int getConnectionTimeout();

	// Dummy packet interval
	static int getDummyPacketInterval();

	// Lock manager memory size
	static int getLockMemSize();

	// Lock manager grant order
	static bool getLockGrantOrder();

	// Lock manager hash slots
	static int getLockHashSlots();

	// Lock manager acquire spins
	static int getLockAcquireSpins();

	// Event manager memory size
	static int getEventMemSize();

	// Deadlock timeout
	static int getDeadlockTimeout();

	// Priority switch delay
	static int getPrioritySwitchDelay();

	// Use priority scheduler
	static bool getUsePriorityScheduler();

	// Priority boost
	static int getPriorityBoost();

	// Service name for remote protocols
	static const char *getRemoteServiceName();

	// Service port for INET
	static unsigned short getRemoteServicePort();

	// Pipe name for WNET
	static const char *getRemotePipeName();

	// Name for IPC-related objects
	static const char *getIpcName();

	// Unflushed writes number
	static int getMaxUnflushedWrites();

	// Unflushed write time
	static int getMaxUnflushedWriteTime();

	// Process priority level
	static int getProcessPriorityLevel();

	// Complete boolean evaluation
	static bool getCompleteBooleanEvaluation();

	// Port for event processing
	static int getRemoteAuxPort();

	// Server binding NIC address
	static const char *getRemoteBindAddress();

	// Directory list for external tables
	static const char *getExternalFileAccess();

	// Directory list for databases
	static const char *getDatabaseAccess();

	// Directory list for UDF libraries
	static const char *getUdfAccess();

	// Temporary directories list
	static const char *getTempDirectories();

	// Abort on BUGCHECK and structured exceptions
 	static bool getBugcheckAbort();

	// Let use of des hash to verify passwords
	static bool getLegacyHash();

	// GC policy
	static const char *getGCPolicy();

	// Redirection
	static bool getRedirection();

	// Use old column naming rules (does not conform to SQL standard)
	static bool getOldColumnNaming();

	// Use native, trusted or mixed authentication
	static const char *getAuthMethod();

	static int getDatabaseGrowthIncrement();

	static int getFileSystemCacheThreshold();

	static int getFileSystemCacheSize();

	static bool getRelaxedAliasChecking();

	static bool getOldSetClauseSemantics();

	static const char *getAuditTraceConfigFile();

	static int getMaxUserTraceLogSize();
};

#endif // COMMON_CONFIG_H
