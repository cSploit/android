/*
 *	PROGRAM:		Firebird samples.
 *	MODULE:			CryptKeyHolder.cpp
 *	DESCRIPTION:	Sample of how key holder may be written.
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
 *  Copyright (c) 2012 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include <stdio.h>
#include <string.h>

#include "firebird.h"
#include "firebird/Crypt.h"

#include "../common/classes/fb_atomic.h"

using namespace Firebird;

namespace
{

IMaster* master = NULL;
IPluginManager* pluginManager = NULL;

class PluginModule : public IPluginModule
{
public:
	PluginModule()
		: flag(false)
	{ }

	void registerMe()
	{
		pluginManager->registerModule(this);
		flag = true;
	}

	~PluginModule()
	{
		if (flag)
		{
			pluginManager->unregisterModule(this);
			doClean();
		}
	}

	int FB_CARG getVersion()
	{
		return FB_PLUGIN_MODULE_VERSION;
	}

	IPluginModule* FB_CARG getModule()
	{
		return this;
	}

	void FB_CARG doClean()
	{
		flag = false;
	}

private:
	bool flag;
};

PluginModule module;

class CryptKeyHolder : public IKeyHolderPlugin
{
public:
	explicit CryptKeyHolder(IPluginConfig* cnf)
		: callbackInterface(this), config(cnf), key(0), owner(NULL)
	{
		config->addRef();
	}

	~CryptKeyHolder()
	{
		config->release();
	}

	// IKeyHolderPlugin implementation
	virtual int FB_CARG keyCallback(IStatus* status, ICryptKeyCallback* callback);
	virtual ICryptKeyCallback* FB_CARG keyHandle(IStatus* status, const char* keyName);

	int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}
		return 1;
	}

	void FB_CARG addRef()
	{
		++refCounter;
	}

	int FB_CARG getVersion()
	{
		return FB_KEYHOLDER_PLUGIN_VERSION;
	}

	IPluginModule* FB_CARG getModule()
	{
		return &module;
	}

	void FB_CARG setOwner(Firebird::IRefCounted* o)
	{
		owner = o;
	}

	IRefCounted* FB_CARG getOwner()
	{
		return owner;
	}

	UCHAR getKey()
	{
		return key;
	}

private:
	class CallbackInterface : public ICryptKeyCallback
	{
	public:
		explicit CallbackInterface(CryptKeyHolder* p)
			: parent(p)
		{ }

		unsigned int FB_CARG callback(unsigned int, const void*, unsigned int length, void* buffer)
		{
			UCHAR k = parent->getKey();
			if (!k)
			{
				return 0;
			}

			if (length > 0 && buffer)
			{
				memcpy(buffer, &k, 1);
			}
			return 1;
		}

		int FB_CARG getVersion()
		{
			return FB_CRYPT_CALLBACK_VERSION;
		}

		IPluginModule* FB_CARG getModule()
		{
			return &module;
		}

	private:
		CryptKeyHolder* parent;
	};

	CallbackInterface callbackInterface;

	IPluginConfig* config;
	UCHAR key;

	AtomicCounter refCounter;
	IRefCounted* owner;

	void noKeyError(IStatus* status);
};

void CryptKeyHolder::noKeyError(IStatus* status)
{
	ISC_STATUS_ARRAY vector;
	vector[0] = isc_arg_gds;
	vector[1] = isc_random;
	vector[2] = isc_arg_string;
	vector[3] = (ISC_STATUS) "Key not set";
	vector[4] = isc_arg_end;
	status->set(vector);
}

int FB_CARG CryptKeyHolder::keyCallback(IStatus* status, ICryptKeyCallback* callback)
{
	status->init();

	if (key != 0)
	{
		return 1;
	}

	IConfig* def = config->getDefaultConfig();
	IConfigEntry* confEntry = def->find("Auto");
	def->release();

	if (confEntry)
	{
		char v = *(confEntry->getValue());
		confEntry->release();
		if (v == '1' || v == 'y' || v == 'Y' || v == 't' || v == 'T')
		{
			key = 0x5a;
			return 1;
		}
	}

	if (callback && callback->callback(0, NULL, 1, &key) != 1)
	{
		key = 0;
		return 0;
	}

	return 1;
}

ICryptKeyCallback* FB_CARG CryptKeyHolder::keyHandle(IStatus* status, const char* keyName)
{
	if (strcmp(keyName, "sample") != 0)
	{
		return NULL;
	}

	return &callbackInterface;
}

class Factory : public IPluginFactory
{
public:
	int FB_CARG getVersion()
	{
		return FB_PLUGIN_FACTORY_VERSION;
	}

	IPluginModule* FB_CARG getModule()
	{
		return &module;
	}

	IPluginBase* FB_CARG createPlugin(IPluginConfig* factoryParameter)
	{
		CryptKeyHolder* p = new CryptKeyHolder(factoryParameter);
		p->addRef();
		return p;
	}
};

Factory factory;

} // anonymous namespace

extern "C" void FB_PLUGIN_ENTRY_POINT(IMaster* m)
{
	master = m;
	pluginManager = master->getPluginManager();

	module.registerMe();
	pluginManager->registerPluginFactory(PluginType::KeyHolder, "CryptKeyHolder_example",
		&factory);
}
