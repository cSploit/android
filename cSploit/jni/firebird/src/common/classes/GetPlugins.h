/*
 *	PROGRAM:		Firebird interface.
 *	MODULE:			ImplementHelper.h
 *	DESCRIPTION:	Tools to help access plugins.
 *
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2010 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef FB_COMMON_CLASSES_GET_PLUGINS
#define FB_COMMON_CLASSES_GET_PLUGINS

#include "../common/classes/ImplementHelper.h"
#include "../common/config/config.h"
#include "../common/StatusHolder.h"

namespace Firebird {

// Template to help with loop in the set of plugins
template <typename P>
class GetPlugins
{
public:
	GetPlugins(unsigned int interfaceType, unsigned int desiredVersion,
			   UpgradeInfo* ui, const char* namesList = NULL)
		: masterInterface(), pluginInterface(),
		  pluginSet(NULL), currentPlugin(NULL)
	{
		LocalStatus status;
		pluginSet.assignRefNoIncr(pluginInterface->getPlugins(&status, interfaceType,
			(namesList ? namesList : Config::getDefaultConfig()->getPlugins(interfaceType)),
			desiredVersion, ui, NULL));

		if (!pluginSet)
		{
			fb_assert(!status.isSuccess());
			status_exception::raise(status.get());
		}

		getPlugin();
	}

	GetPlugins(unsigned int interfaceType, unsigned int desiredVersion, UpgradeInfo* ui,
			   Config* knownConfig, const char* namesList = NULL)
		: masterInterface(), pluginInterface(),
		  pluginSet(NULL), currentPlugin(NULL)
	{
		LocalStatus status;
		pluginSet.assignRefNoIncr(pluginInterface->getPlugins(&status, interfaceType,
			(namesList ? namesList : knownConfig->getPlugins(interfaceType)),
			desiredVersion, ui, new FirebirdConf(knownConfig)));

		if (!pluginSet)
		{
			fb_assert(!status.isSuccess());
			status_exception::raise(status.get());
		}

		getPlugin();
	}

	bool hasData()
	{
		return currentPlugin;
	}

	const char* name()
	{
		return hasData() ? pluginSet->getName() : NULL;
	}

	const char* module()
	{
		return hasData() ? pluginSet->getModule() : NULL;
	}

	P* plugin()
	{
		return currentPlugin;
	}

	void next()
	{
		if (hasData())
		{
			pluginInterface->releasePlugin(currentPlugin);
			currentPlugin = NULL;
			pluginSet->next();
			getPlugin();
		}
	}

	void set(const char* newName)
	{
		if (hasData())
		{
			pluginInterface->releasePlugin(currentPlugin);
			currentPlugin = NULL;
		}
		pluginSet->set(newName);
		getPlugin();
	}

	~GetPlugins()
	{
		if (hasData())
		{
			pluginInterface->releasePlugin(currentPlugin);
			currentPlugin = NULL;
		}
	}

private:
	MasterInterfacePtr masterInterface;
	PluginManagerInterfacePtr pluginInterface;
	RefPtr<IPluginSet> pluginSet;
	P* currentPlugin;

	void getPlugin()
	{
		currentPlugin = (P*) pluginSet->getPlugin();
	}
};

} // namespace Firebird


#endif // FB_COMMON_CLASSES_GET_PLUGINS
