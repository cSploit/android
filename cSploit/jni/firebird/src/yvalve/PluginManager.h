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

#ifndef YVALVE_PLUGIN_MANAGER_H
#define YVALVE_PLUGIN_MANAGER_H

#include "firebird/Plugin.h"
#include "../common/classes/ImplementHelper.h"

#include "../common/os/mod_loader.h"
#include "../common/classes/array.h"
#include "../common/classes/auto.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/locks.h"
#include "../common/config/config_file.h"
#include "../common/config/config.h"

namespace Firebird {

class PluginManager : public AutoIface<IPluginManager, FB_PLUGIN_MANAGER_VERSION>
{
public:
	// IPluginManager implementation
	IPluginSet* FB_CARG getPlugins(IStatus* status, unsigned int interfaceType,
					const char* namesList, int desiredVersion, UpgradeInfo* ui,
					IFirebirdConf* firebirdConf);
	void FB_CARG registerPluginFactory(unsigned int interfaceType, const char* defaultName,
					IPluginFactory* factory);
	IConfig* FB_CARG getConfig(const char* filename);
	void FB_CARG releasePlugin(IPluginBase* plugin);
	void FB_CARG registerModule(IPluginModule* module);
	void FB_CARG unregisterModule(IPluginModule* module);

	PluginManager();

	static void shutdown();
	static void waitForType(unsigned int typeThatMustGoAway);
};

}	// namespace Firebird

#endif	// YVALVE_PLUGIN_MANAGER_H
