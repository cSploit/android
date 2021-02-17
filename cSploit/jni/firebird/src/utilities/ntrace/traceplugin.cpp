/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		traceplugin.cpp
 *	DESCRIPTION:	Exported entrypoints for the plugin
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
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  2008 Khorsun Vladyslav
 */

#include "firebird.h"
#include "../common/classes/ImplementHelper.h"

#include "TraceConfiguration.h"
#include "TracePluginImpl.h"

class TraceFactoryImpl FB_FINAL : public Firebird::StdPlugin<TraceFactory, FB_TRACE_FACTORY_VERSION>
{
public:
	explicit TraceFactoryImpl(Firebird::IPluginConfig*)
	{ }

	// TraceFactory implementation
	ntrace_mask_t FB_CARG trace_needs();
	TracePlugin* FB_CARG trace_create(Firebird::IStatus* status, TraceInitInfo* init_info);
	int FB_CARG release();
};

int FB_CARG TraceFactoryImpl::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}
	return 1;
}

static Firebird::MakeUpgradeInfo<> upInfo;

ntrace_mask_t FB_CARG TraceFactoryImpl::trace_needs()
{
	return (1 << TRACE_EVENT_MAX) - 1;
}

TracePlugin* FB_CARG TraceFactoryImpl::trace_create(Firebird::IStatus* status, TraceInitInfo* initInfo)
{
	Firebird::MasterInterfacePtr master;
	const char* dbname = NULL;
	try
	{
		master->upgradeInterface(initInfo, FB_TRACE_INIT_INFO_VERSION, upInfo);

		dbname = initInfo->getDatabaseName();
		if (!dbname)
			dbname = "";

		TracePluginConfig config;
		TraceCfgReader::readTraceConfiguration(initInfo->getConfigText(), dbname, config);

		TraceDatabaseConnection* connection = initInfo->getConnection();
		if (connection)
			master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);

		if (!config.enabled ||
			(config.connection_id && connection &&
				(connection->getConnectionID() != SLONG(config.connection_id))))
		{
			return NULL; // Plugin is not needed, no error happened.
		}

		Firebird::AutoPtr<TraceLogWriter, Firebird::SimpleRelease<TraceLogWriter> >
			logWriter(initInfo->getLogWriter());

		if (logWriter)
			config.log_filename = "";

		return new TracePluginImpl(config, initInfo);	// Everything is ok, we created a plugin

	}
	catch (Firebird::Exception& ex)
	{
		// put error into trace log
		TraceLogWriter* logWriter = initInfo->getLogWriter();
		if (logWriter)
		{
			master->upgradeInterface(logWriter, FB_TRACE_LOG_WRITER_VERSION, upInfo);
			const char* strEx = TracePluginImpl::marshal_exception(ex);
			Firebird::string err;
			if (dbname)
				err.printf("Error creating trace session for database \"%s\":\n%s\n", dbname, strEx);
			else
				err.printf("Error creating trace session for service manager attachment:\n%s\n", strEx);

			logWriter->write(err.c_str(), err.length());
			logWriter->release();
		}
		else
			ex.stuffException(status);
	}

	return NULL;
}


static Firebird::SimpleFactory<TraceFactoryImpl> traceFactory;

void registerTrace(Firebird::IPluginManager* iPlugin)
{
	iPlugin->registerPluginFactory(Firebird::PluginType::Trace, "fbtrace", &traceFactory);
	Firebird::myModule->registerMe();
}


extern "C" void FB_DLL_EXPORT FB_PLUGIN_ENTRY_POINT(Firebird::IMaster* master)
{
	Firebird::CachedMasterInterface::set(master);
	registerTrace(Firebird::PluginManagerInterfacePtr());
}
