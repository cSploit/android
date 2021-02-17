/*
 *	PROGRAM:		Firebird samples.
 *	MODULE:			DbCrypt.cpp
 *	DESCRIPTION:	Sample of how diskcrypt may be written.
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

class DbCrypt : public IDbCryptPlugin
{
public:
	explicit DbCrypt(IPluginConfig* cnf)
		: config(cnf), key(0), owner(NULL)
	{
		config->addRef();
	}

	~DbCrypt()
	{
		config->release();
	}

	// ICryptPlugin implementation
	void FB_CARG encrypt(IStatus* status, unsigned int length, const void* from, void* to);
	void FB_CARG decrypt(IStatus* status, unsigned int length, const void* from, void* to);
	void FB_CARG setKey(IStatus* status, unsigned int length, IKeyHolderPlugin** sources);

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
		return FB_DBCRYPT_PLUGIN_VERSION;
	}

	IPluginModule* FB_CARG getModule()
	{
		return &module;
	}

	void FB_CARG setOwner(IRefCounted* o)
	{
		owner = o;
	}

	IRefCounted* FB_CARG getOwner()
	{
		return owner;
	}

private:
	IPluginConfig* config;
	UCHAR key;

	AtomicCounter refCounter;
	IRefCounted* owner;

	void noKeyError(IStatus* status);
};

void DbCrypt::noKeyError(IStatus* status)
{
	ISC_STATUS_ARRAY vector;
	vector[0] = isc_arg_gds;
	vector[1] = isc_random;
	vector[2] = isc_arg_string;
	vector[3] = (ISC_STATUS)"Key not set";
	vector[4] = isc_arg_end;
	status->set(vector);
}

void FB_CARG DbCrypt::encrypt(IStatus* status, unsigned int length, const void* from, void* to)
{
	status->init();

	if (!key)
	{
		noKeyError(status);
		return;
	}

	const UCHAR* f = static_cast<const UCHAR*>(from);
	UCHAR* t = static_cast<UCHAR*>(to);

	while (length--)
	{
		*t++ = (*f++) + key;
	}
}

void FB_CARG DbCrypt::decrypt(IStatus* status, unsigned int length, const void* from, void* to)
{
	status->init();

	if (!key)
	{
		noKeyError(status);
		return;
	}

	const UCHAR* f = static_cast<const UCHAR*>(from);
	UCHAR* t = static_cast<UCHAR*>(to);

	while (length--)
	{
		*t++ = (*f++) - key;
	}
}

void FB_CARG DbCrypt::setKey(IStatus* status, unsigned int length, IKeyHolderPlugin** sources)
{
	status->init();

	if (key != 0)
		return;

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
			return;
		}
	}

	for (unsigned n = 0; n < length; ++n)
	{
		ICryptKeyCallback* callback = sources[n]->keyHandle(status, "sample");
		if (!status->isSuccess())
		{
			return;
		}

		if (callback && callback->callback(0, NULL, 1, &key) == 1)
		{
			return;
		}
	}

	key = 0;
	noKeyError(status);
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
		DbCrypt* p = new DbCrypt(factoryParameter);
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
	pluginManager->registerPluginFactory(PluginType::DbCrypt, "DbCrypt_example", &factory);
}
