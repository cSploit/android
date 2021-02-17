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
 *  for the Firebird Open Source RDBMS project, based on previous work done
 *  by Eugeney Putilin <evgeneyputilin at mail.ru>,
 *  Vlad Khorsun <hvlad at users.sourceforge.net> and
 *  Roman Rokytskyy <roman at rokytskyy.de>.
 *
 *  Copyright (c) 2008 Adriano dos Santos Fernandes <adrianosf@uol.com.br>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *  Eugeney Putilin <evgeneyputilin at mail.ru>
 *  Vlad Khorsun <hvlad at users.sourceforge.net>
 *  Roman Rokytskyy <roman at rokytskyy.de>
 */

#ifndef FIREBIRD_EXTERNAL_API_H
#define FIREBIRD_EXTERNAL_API_H

#include "FirebirdApi.h"
#include "./Plugin.h"
#include "./Provider.h"

namespace Firebird {

class ExternalEngine;


// Connection to current database in external engine.
// Context passed to ExternalEngine has SYSDBA privileges.
// Context passed to ExternalFunction, ExternalProcedure and ExternalTrigger
// has user privileges.
// There is one ExternalContext per attachment. The privileges and character
// set properties are changed during the calls.
class ExternalContext
{
public:
	// Gets the IMaster associated with this context.
	virtual IMaster* FB_CARG getMaster() = 0;

	// Gets the ExternalEngine associated with this context.
	virtual ExternalEngine* FB_CARG getEngine(IStatus* status) = 0;

	// Gets the Attachment associated with this context.
	virtual IAttachment* FB_CARG getAttachment(IStatus* status) = 0;

	// Obtained transaction is valid only before control is returned to the engine
	// or in ExternalResultSet::fetch calls of correspondent ExternalProcedure::open.
	virtual ITransaction* FB_CARG getTransaction(IStatus* status) = 0;

	virtual const char* FB_CARG getUserName() = 0;
	virtual const char* FB_CARG getDatabaseName() = 0;

	// Get user attachment character set.
	virtual const Utf8* FB_CARG getClientCharSet() = 0;

	// Misc info associated with a context. The pointers are never accessed or freed by Firebird.

	// Obtains an unique (across all contexts) code to associate plugin and/or user information.
	virtual int FB_CARG obtainInfoCode() = 0;
	// Gets a value associated with this code or FB_NULL if no value was set.
	virtual void* FB_CARG getInfo(int code) = 0;
	// Sets a value associated with this code and returns the last value.
	virtual void* FB_CARG setInfo(int code, void* value) = 0;
};


// To return set of rows in selectable procedures.
class ExternalResultSet : public IDisposable
{
public:
	virtual FB_BOOLEAN FB_CARG fetch(IStatus* status) = 0;
};

#define FB_EXTERNAL_RESULT_SET_VERSION (FB_DISPOSABLE_VERSION + 1)


class ExternalFunction : public IDisposable
{
public:
	// This method is called just before execute and informs the engine our requested character
	// set for data exchange inside that method.
	// During this call, the context uses the character set obtained from ExternalEngine::getCharSet.
	virtual void FB_CARG getCharSet(IStatus* status, ExternalContext* context,
		Utf8* name, uint nameSize) = 0;

	virtual void FB_CARG execute(IStatus* status, ExternalContext* context,
		void* inMsg, void* outMsg) = 0;
};

#define FB_EXTERNAL_FUNCTION_VERSION (FB_DISPOSABLE_VERSION + 2)


class ExternalProcedure : public IDisposable
{
public:
	// This method is called just before open and informs the engine our requested character
	// set for data exchange inside that method and ExternalResultSet::fetch.
	// During this call, the context uses the character set obtained from ExternalEngine::getCharSet.
	virtual void FB_CARG getCharSet(IStatus* status, ExternalContext* context,
		Utf8* name, uint nameSize) = 0;

	// Returns a ExternalResultSet for selectable procedures.
	// Returning NULL results in a result set of one record.
	// Procedures without output parameters should return NULL.
	virtual ExternalResultSet* FB_CARG open(IStatus* status, ExternalContext* context,
		void* inMsg, void* outMsg) = 0;
};

#define FB_EXTERNAL_PROCEDURE_VERSION (FB_DISPOSABLE_VERSION + 2)


class ExternalTrigger : public IDisposable
{
public:
	enum Type
	{
		TYPE_BEFORE = 1,
		TYPE_AFTER,
		TYPE_DATABASE
	};

	enum Action
	{
		ACTION_INSERT = 1,
		ACTION_UPDATE,
		ACTION_DELETE,
		ACTION_CONNECT,
		ACTION_DISCONNECT,
		ACTION_TRANS_START,
		ACTION_TRANS_COMMIT,
		ACTION_TRANS_ROLLBACK,
		ACTION_DDL
	};

public:
	// This method is called just before execute and informs the engine our requested character
	// set for data exchange inside that method.
	// During this call, the context uses the character set obtained from ExternalEngine::getCharSet.
	virtual void FB_CARG getCharSet(IStatus* status, ExternalContext* context,
		Utf8* name, uint nameSize) = 0;

	virtual void FB_CARG execute(IStatus* status, ExternalContext* context,
		Action action, void* oldMsg, void* newMsg) = 0;
};

#define FB_EXTERNAL_TRIGGER_VERSION (FB_DISPOSABLE_VERSION + 2)


class IRoutineMetadata : public IVersioned
{
public:
	virtual const char* FB_CARG getPackage(IStatus* status) const = 0;
	virtual const char* FB_CARG getName(IStatus* status) const = 0;
	virtual const char* FB_CARG getEntryPoint(IStatus* status) const = 0;
	virtual const char* FB_CARG getBody(IStatus* status) const = 0;
	virtual IMessageMetadata* FB_CARG getInputMetadata(IStatus* status) const = 0;
	virtual IMessageMetadata* FB_CARG getOutputMetadata(IStatus* status) const = 0;
	virtual IMessageMetadata* FB_CARG getTriggerMetadata(IStatus* status) const = 0;
	virtual const char* FB_CARG getTriggerTable(IStatus* status) const = 0;
	virtual ExternalTrigger::Type FB_CARG getTriggerType(IStatus* status) const = 0;
};
#define FB_ROUTINE_METADATA_VERSION (FB_VERSIONED_VERSION + 9)


// In SuperServer, shared by all attachments to one database and disposed when last (non-external)
// user attachment to the database is closed.
class ExternalEngine : public IPluginBase
{
public:
	// This method is called once (per ExternalEngine instance) before any following methods.
	// The requested character set for data exchange inside methods of this interface should
	// be copied to charSet parameter.
	// During this call, the context uses the UTF-8 character set.
	virtual void FB_CARG open(IStatus* status, ExternalContext* context,
		Utf8* charSet, uint charSetSize) = 0;

	// Attachment is being opened.
	virtual void FB_CARG openAttachment(IStatus* status, ExternalContext* context) = 0;

	// Attachment is being closed.
	virtual void FB_CARG closeAttachment(IStatus* status, ExternalContext* context) = 0;

	// Called when engine wants to load object in the cache. Objects are disposed when
	// going out of the cache.
	virtual ExternalFunction* FB_CARG makeFunction(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata,
		IMetadataBuilder* inBuilder, IMetadataBuilder* outBuilder) = 0;
	virtual ExternalProcedure* FB_CARG makeProcedure(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata,
		IMetadataBuilder* inBuilder, IMetadataBuilder* outBuilder) = 0;
	virtual ExternalTrigger* FB_CARG makeTrigger(IStatus* status, ExternalContext* context,
		const IRoutineMetadata* metadata, IMetadataBuilder* fieldsBuilder) = 0;
};
#define FB_EXTERNAL_ENGINE_VERSION (FB_PLUGIN_VERSION + 6)

}	// namespace Firebird


#endif	// FIREBIRD_EXTERNAL_API_H
