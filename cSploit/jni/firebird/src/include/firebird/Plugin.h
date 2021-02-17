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
 *  The Original Code was created by Adriano dos Santos Fernandes
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

/*
 * Firebird plugins are accessed using methods of PluginLoader interface.
 * For each plugin_module tag found, it constructs a Plugin object, reads the corresponding
 * plugin_config tag and inserts all config information in the object.
 *
 * When requested, the engine gets the attribute value of plugin_module/filename, load it as a
 * dynamic (shared) library and calls the exported function firebirdPlugin (FB_PLUGIN_ENTRY_POINT
 * definition, PluginEntrypoint prototype) passing the Plugin object as parameter.
 *
 * The plugin library may save the plugin object and call they methods later. The object and all
 * pointers returned by it are valid until the plugin is unloaded (done through OS unload of the
 * dynamic library) when Firebird is shutting down.
 *
 * Inside the plugin entry point (firebirdPlugin), the plugin may register extra functionality that
 * may be obtained by Firebird when required. Currently only External Engines may be registered
 * through Plugin::setExternalEngineFactory.
 *
 * Example plugin configuration file:
 *
 * <external_engine UDR>
 *     plugin_module UDR_engine
 * </external_engine>
 *
 * <plugin_module UDR_engine>
 *     filename $(this)/udr_engine
 *     plugin_config UDR_config
 * </plugin_module>
 *
 * <plugin_config UDR_config>
 *     path $(this)/udr
 * </plugin_config>
 *
 * Note that the external_engine tag is ignored at this stage. Only plugin_module and plugin_config
 * are read. The dynamic library extension may be ommitted, and $(this) expands to the directory of
 * the .conf file.
 *
 * Plugins may access Firebird API through the fbclient library.
 */

#ifndef FIREBIRD_PLUGIN_API_H
#define FIREBIRD_PLUGIN_API_H

#include "./Interface.h"

#define FB_PLUGIN_ENTRY_POINT		firebird_plugin


namespace Firebird {

// IPluginBase interface - base for master plugin interfaces (factories are registered for them)
class IPluginBase : public IRefCounted
{
public:
	// Additional (compared with Interface) functions getOwner() and setOwner()
	// are needed to release() owner of the plugin. This is done in releasePlugin()
	// function in IPluginManager. Such method is needed to make sure that owner is released
	// after plugin itself, and therefore module is unloaded after release of last plugin from it.
	// Releasing owner from release() of plugin will unload module and after returning control
	// to missing code segfault is unavoidable.
	virtual void FB_CARG setOwner(IRefCounted*) = 0;
	virtual IRefCounted* FB_CARG getOwner() = 0;
};
#define FB_PLUGIN_VERSION (FB_REFCOUNTED_VERSION + 2)

// IPluginSet - low level tool to access plugins according to parameter from firebird.conf
class IPluginSet : public IRefCounted
{
public:
	virtual const char* FB_CARG getName() const = 0;
	virtual const char* FB_CARG getModuleName() const = 0;
	virtual IPluginBase* FB_CARG getPlugin() = 0;
	virtual void FB_CARG next() = 0;
	virtual void FB_CARG set(const char*) = 0;
};
#define FB_PLUGIN_SET_VERSION (FB_REFCOUNTED_VERSION + 5)

// Interfaces to work with configuration data
class IConfig;

// Entry in configuration file
class IConfigEntry : public IRefCounted
{
public:
	virtual const char* FB_CARG getName() = 0;
	virtual const char* FB_CARG getValue() = 0;
	virtual IConfig* FB_CARG getSubConfig() = 0;
	virtual ISC_INT64 FB_CARG getIntValue() = 0;
	virtual FB_BOOLEAN FB_CARG getBoolValue() = 0;
};
#define FB_CONFIG_PARAMETER_VERSION (FB_REFCOUNTED_VERSION + 5)

// Generic form of access to configuration file - find specific entry in it
class IConfig : public IRefCounted
{
public:
	virtual IConfigEntry* FB_CARG find(const char* name) = 0;
	virtual IConfigEntry* FB_CARG findValue(const char* name, const char* value) = 0;
	virtual IConfigEntry* FB_CARG findPos(const char* name, unsigned int pos) = 0;
};
#define FB_CONFIG_VERSION (FB_REFCOUNTED_VERSION + 3)

// Used to access config values from firebird.conf (may be DB specific)
class IFirebirdConf : public IRefCounted
{
public:
	// Get integer key by it's name
	// Value ~0 means name is invalid
	// Keys are stable: one can use once obtained key in other instances of this interface
	virtual unsigned int FB_CARG getKey(const char* name) = 0;
	// Use to access integer values
	virtual ISC_INT64 FB_CARG asInteger(unsigned int key) = 0;
	// Use to access string values
	virtual const char* FB_CARG asString(unsigned int key) = 0;
	// Use to access boolean values
	virtual FB_BOOLEAN FB_CARG asBoolean(unsigned int key) = 0;
};
#define FB_FIREBIRD_CONF_VERSION (FB_REFCOUNTED_VERSION + 4)

// This interface is passed to plugin's factory as it's single parameter
// and contains methods to access specific plugin's configuration data
class IPluginConfig : public IRefCounted
{
public:
	virtual const char* FB_CARG getConfigFileName() = 0;
	virtual IConfig* FB_CARG getDefaultConfig() = 0;
	virtual IFirebirdConf* FB_CARG getFirebirdConf() = 0;
	virtual void FB_CARG setReleaseDelay(ISC_UINT64 microSeconds) = 0;
};
#define FB_PLUGIN_CONFIG_VERSION (FB_REFCOUNTED_VERSION + 4)

// Required to creat instances of given plugin
class IPluginFactory : public IVersioned
{
public:
	virtual IPluginBase* FB_CARG createPlugin(IPluginConfig* factoryParameter) = 0;
};
#define FB_PLUGIN_FACTORY_VERSION (FB_VERSIONED_VERSION + 1)

// Required to let plugins manager invoke module's cleanup routine before unloading it.
// For some OS/compiler this may be done in dtor of global variable in module itself.
// Others (Windows/VC) fail to create some very useful resources (threads) when module is unloading.
class IPluginModule : public IVersioned
{
public:
	virtual void FB_CARG doClean() = 0;
};
#define FB_PLUGIN_MODULE_VERSION (FB_VERSIONED_VERSION + 1)


// Interface to deal with plugins here and there, returned by master interface
class IPluginManager : public IVersioned
{
public:
	// Main function called by plugin modules in firebird_plugin()
	virtual void FB_CARG registerPluginFactory(unsigned int interfaceType, const char* defaultName,
										IPluginFactory* factory) = 0;
	// Sets cleanup for plugin module
	// Pay attention - this should be called at plugin-register time!
	// Only at this moment manager knows, which module sets his cleanup
	virtual void FB_CARG registerModule(IPluginModule* cleanup) = 0;
	// Remove registered module before cleanup routine.
	// This method must be called by module which detects that it's unloaded,
	// but not notified prior to it by PluginManager via IPluginModule.
	virtual void FB_CARG unregisterModule(IPluginModule* cleanup) = 0;
	// Main function called to access plugins registered in plugins manager
	// Has front-end in GetPlugins.h - template GetPlugins
	// In namesList parameter comma or space separated list of names of configured plugins is passed
	// UpgradeInfo is used to add functions "notImplemented" to the end of vtable
	// in case when plugin's version is less than desired
	// If caller already has an interface for firebird.conf, it may be passed here
	// If parameter is missing, plugins will get access to default (non database specific) config
	virtual IPluginSet* FB_CARG getPlugins(IStatus* status, unsigned int interfaceType,
						const char* namesList, int desiredVersion,
						UpgradeInfo* ui, IFirebirdConf* firebirdConf) = 0;
	// Get generic config interface for given file
	virtual IConfig* FB_CARG getConfig(const char* filename) = 0;
	// Plugins must be released using this function - use of plugin's release()
	// will cause resources leak
	virtual void FB_CARG releasePlugin(IPluginBase* plugin) = 0;
};
#define FB_PLUGIN_MANAGER_VERSION (FB_VERSIONED_VERSION + 6)


struct FbCryptKey
{
	const char* type;					// If NULL type is auth plugin name
	const void* encryptKey;
	const void* decryptKey;				// May be NULL for symmetric keys
	unsigned int encryptLength;
	unsigned int decryptLength;			// Ignored when decryptKey is NULL
};


typedef void PluginEntrypoint(IMaster* masterInterface);

namespace PluginType {
	static const unsigned int YValve = 1;
	static const unsigned int Provider = 2;
	// leave space for may be some more super-std plugins
	static const unsigned int FirstNonLibPlugin = 11;
	static const unsigned int AuthServer = 11;
	static const unsigned int AuthClient = 12;
	static const unsigned int AuthUserManagement = 13;
	static const unsigned int ExternalEngine = 14;
	static const unsigned int Trace = 15;
	static const unsigned int WireCrypt = 16;
	static const unsigned int DbCrypt = 17;
	static const unsigned int KeyHolder = 18;

	static const unsigned int MaxType = 19;	// keep in sync please
}


// Generic access to all config interfaces
class IConfigManager : public IVersioned
{
public:
	virtual const char* FB_CARG getDirectory(unsigned code) = 0;
	virtual IFirebirdConf* FB_CARG getFirebirdConf() = 0;
	virtual IFirebirdConf* FB_CARG getDatabaseConf(const char* dbName) = 0;
	virtual IConfig* FB_CARG getPluginConfig(const char* configuredPlugin) = 0;
};
#define FB_CONFIG_MANAGER_VERSION (FB_VERSIONED_VERSION + 4)

namespace DirType {
	// Codes for IConfigManager::getDirectory()

	static const unsigned int FB_DIR_BIN = 0;
	static const unsigned int FB_DIR_SBIN = 1;
	static const unsigned int FB_DIR_CONF = 2;
	static const unsigned int FB_DIR_LIB = 3;
	static const unsigned int FB_DIR_INC = 4;
	static const unsigned int FB_DIR_DOC = 5;
	static const unsigned int FB_DIR_UDF = 6;
	static const unsigned int FB_DIR_SAMPLE = 7;
	static const unsigned int FB_DIR_SAMPLEDB = 8;
	static const unsigned int FB_DIR_HELP = 9;
	static const unsigned int FB_DIR_INTL = 10;
	static const unsigned int FB_DIR_MISC = 11;
	static const unsigned int FB_DIR_SECDB = 12;
	static const unsigned int FB_DIR_MSG = 13;
	static const unsigned int FB_DIR_LOG = 14;
	static const unsigned int FB_DIR_GUARD = 15;
	static const unsigned int FB_DIR_PLUGINS = 16;

	static const unsigned int FB_DIRCOUNT = 17;
}

}	// namespace Firebird

#endif	// FIREBIRD_PLUGIN_API_H
