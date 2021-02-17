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

#ifndef JRD_EXT_ENGINE_MANAGER_H
#define JRD_EXT_ENGINE_MANAGER_H

#include "FirebirdApi.h"
#include "firebird/ExternalEngine.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/NestConst.h"
#include "../common/classes/auto.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/StatementMetadata.h"

struct dsc;

namespace Jrd {

class thread_db;
class jrd_prc;
class jrd_tra;
class Attachment;
class CompilerScratch;
class Database;
class Format;
class Trigger;
class Function;
class ValueExprNode;
struct impure_value;
struct record_param;


class ExtEngineManager FB_FINAL : public Firebird::PermanentStorage
{
private:
	class AttachmentImpl;
	template <typename T> class ContextManager;
	class TransactionImpl;

	class RoutineMetadata FB_FINAL :
		public Firebird::VersionedIface<Firebird::IRoutineMetadata, FB_ROUTINE_METADATA_VERSION>,
		public Firebird::PermanentStorage
	{
	public:
		explicit RoutineMetadata(MemoryPool& pool)
			: PermanentStorage(pool),
			  package(pool),
			  name(pool),
			  entryPoint(pool),
			  body(pool),
			  triggerTable(pool),
			  triggerType(Firebird::ExternalTrigger::Type(0))
		{
		}

		virtual const char* FB_CARG getPackage(Firebird::IStatus* /*status*/) const
		{
			return package.nullStr();
		}

		virtual const char* FB_CARG getName(Firebird::IStatus* /*status*/) const
		{
			return name.c_str();
		}

		virtual const char* FB_CARG getEntryPoint(Firebird::IStatus* /*status*/) const
		{
			return entryPoint.c_str();
		}

		virtual const char* FB_CARG getBody(Firebird::IStatus* /*status*/) const
		{
			return body.c_str();
		}

		virtual Firebird::IMessageMetadata* FB_CARG getInputMetadata(
			Firebird::IStatus* /*status*/) const
		{
			return getMetadata(inputParameters);
		}

		virtual Firebird::IMessageMetadata* FB_CARG getOutputMetadata(
			Firebird::IStatus* /*status*/) const
		{
			return getMetadata(outputParameters);
		}

		virtual Firebird::IMessageMetadata* FB_CARG getTriggerMetadata(
			Firebird::IStatus* /*status*/) const
		{
			return getMetadata(triggerFields);
		}

		virtual const char* FB_CARG getTriggerTable(Firebird::IStatus* /*status*/) const
		{
			return triggerTable.c_str();
		}

		virtual Firebird::ExternalTrigger::Type FB_CARG getTriggerType(Firebird::IStatus* /*status*/) const
		{
			return triggerType;
		}

	public:
		Firebird::MetaName package;
		Firebird::MetaName name;
		Firebird::string entryPoint;
		Firebird::string body;
		Firebird::RefPtr<Firebird::IMessageMetadata> inputParameters;
		Firebird::RefPtr<Firebird::IMessageMetadata> outputParameters;
		Firebird::RefPtr<Firebird::IMessageMetadata> triggerFields;
		Firebird::MetaName triggerTable;
		Firebird::ExternalTrigger::Type triggerType;

	private:
		static Firebird::IMessageMetadata* getMetadata(const Firebird::IMessageMetadata* par)
		{
			Firebird::IMessageMetadata* rc = const_cast<Firebird::IMessageMetadata*>(par);
			rc->addRef();
			return rc;
		}
	};

	class ExternalContextImpl : public Firebird::ExternalContext
	{
	friend class AttachmentImpl;

	public:
		ExternalContextImpl(thread_db* tdbb, Firebird::ExternalEngine* aEngine);
		virtual ~ExternalContextImpl();

		void releaseTransaction();
		void setTransaction(thread_db* tdbb);

		virtual Firebird::IMaster* FB_CARG getMaster();
		virtual Firebird::ExternalEngine* FB_CARG getEngine(Firebird::IStatus* status);
		virtual Firebird::IAttachment* FB_CARG getAttachment(Firebird::IStatus* status);
		virtual Firebird::ITransaction* FB_CARG getTransaction(Firebird::IStatus* status);
		virtual const char* FB_CARG getUserName();
		virtual const char* FB_CARG getDatabaseName();
		virtual const Firebird::Utf8* FB_CARG getClientCharSet();
		virtual int FB_CARG obtainInfoCode();
		virtual void* FB_CARG getInfo(int code);
		virtual void* FB_CARG setInfo(int code, void* value);

	private:
		Firebird::ExternalEngine* engine;
		Attachment* internalAttachment;
		jrd_tra* internalTransaction;
		Firebird::IAttachment* externalAttachment;
		Firebird::ITransaction* externalTransaction;
		Firebird::GenericMap<Firebird::NonPooled<int, void*> > miscInfo;
		Firebird::MetaName clientCharSet;
	};

	struct EngineAttachment
	{
		EngineAttachment(Firebird::ExternalEngine* aEngine, Jrd::Attachment* aAttachment)
			: engine(aEngine),
			  attachment(aAttachment)
		{
		}

		static bool greaterThan(const EngineAttachment& i1, const EngineAttachment& i2)
		{
			return (i1.engine > i2.engine) ||
				(i1.engine == i2.engine && i1.attachment > i2.attachment);
		}

		Firebird::ExternalEngine* engine;
		Jrd::Attachment* attachment;
	};

	struct EngineAttachmentInfo
	{
		EngineAttachmentInfo()
			: engine(NULL),
			  context(NULL),
			  adminCharSet(0)
		{
		}

		Firebird::ExternalEngine* engine;
		Firebird::AutoPtr<ExternalContextImpl> context;
		USHORT adminCharSet;
	};

public:
	class Function
	{
	public:
		Function(thread_db* tdbb, ExtEngineManager* aExtManager,
			Firebird::ExternalEngine* aEngine,
			RoutineMetadata* aMetadata,
			Firebird::ExternalFunction* aFunction,
			const Jrd::Function* aUdf);
		~Function();

		void execute(thread_db* tdbb, UCHAR* inMsg, UCHAR* outMsg) const;

	private:
		ExtEngineManager* extManager;
		Firebird::ExternalEngine* engine;
		Firebird::AutoPtr<RoutineMetadata> metadata;
		Firebird::ExternalFunction* function;
		const Jrd::Function* udf;
		Database* database;
	};

	class ResultSet;

	class Procedure
	{
	public:
		Procedure(thread_db* tdbb, ExtEngineManager* aExtManager,
			Firebird::ExternalEngine* aEngine,
			RoutineMetadata* aMetadata,
			Firebird::ExternalProcedure* aProcedure,
			const jrd_prc* aPrc);
		~Procedure();

		ResultSet* open(thread_db* tdbb, UCHAR* inMsg, UCHAR* outMsg) const;

	private:
		ExtEngineManager* extManager;
		Firebird::ExternalEngine* engine;
		Firebird::AutoPtr<RoutineMetadata> metadata;
		Firebird::ExternalProcedure* procedure;
		const jrd_prc* prc;
		Database* database;

	friend class ResultSet;
	};

	class ResultSet
	{
	public:
		ResultSet(thread_db* tdbb, UCHAR* inMsg, UCHAR* outMsg, const Procedure* aProcedure);
		~ResultSet();

		bool fetch(thread_db* tdbb);

	private:
		const Procedure* procedure;
		Attachment* attachment;
		bool firstFetch;
		EngineAttachmentInfo* attInfo;
		Firebird::ExternalResultSet* resultSet;
		USHORT charSet;
	};

	class Trigger
	{
	public:
		Trigger(thread_db* tdbb, MemoryPool& pool, ExtEngineManager* aExtManager,
			Firebird::ExternalEngine* aEngine, RoutineMetadata* aMetadata,
			Firebird::ExternalTrigger* aTrigger, const Jrd::Trigger* aTrg);
		~Trigger();

		void execute(thread_db* tdbb, Firebird::ExternalTrigger::Action action,
			record_param* oldRpb, record_param* newRpb) const;

	private:
		void setValues(thread_db* tdbb, Firebird::Array<UCHAR>& msgBuffer, record_param* rpb) const;

		ExtEngineManager* extManager;
		Firebird::ExternalEngine* engine;
		Firebird::AutoPtr<RoutineMetadata> metadata;
		Firebird::AutoPtr<Format> format;
		Firebird::ExternalTrigger* trigger;
		const Jrd::Trigger* trg;
		Firebird::Array<USHORT> fieldsPos;
		Database* database;
	};

public:
	explicit ExtEngineManager(MemoryPool& p)
		: PermanentStorage(p),
		  engines(p),
		  enginesAttachments(p)
	{
	}

	~ExtEngineManager();

public:
	static void initialize();

public:
	void closeAttachment(thread_db* tdbb, Attachment* attachment);

	void makeFunction(thread_db* tdbb, CompilerScratch* csb, Jrd::Function* udf,
		const Firebird::MetaName& engine, const Firebird::string& entryPoint,
		const Firebird::string& body);
	void makeProcedure(thread_db* tdbb, CompilerScratch* csb, jrd_prc* prc,
		const Firebird::MetaName& engine, const Firebird::string& entryPoint,
		const Firebird::string& body);
	void makeTrigger(thread_db* tdbb, CompilerScratch* csb, Jrd::Trigger* trg,
		const Firebird::MetaName& engine, const Firebird::string& entryPoint,
		const Firebird::string& body, Firebird::ExternalTrigger::Type type);

private:
	Firebird::ExternalEngine* getEngine(thread_db* tdbb,
		const Firebird::MetaName& name);
	EngineAttachmentInfo* getEngineAttachment(thread_db* tdbb,
		const Firebird::MetaName& name);
	EngineAttachmentInfo* getEngineAttachment(thread_db* tdbb,
		Firebird::ExternalEngine* engine, bool closing = false);
	void setupAdminCharSet(thread_db* tdbb, Firebird::ExternalEngine* engine,
		EngineAttachmentInfo* attInfo);

private:
	typedef Firebird::GenericMap<Firebird::Pair<
		Firebird::Left<Firebird::MetaName, Firebird::ExternalEngine*> > > EnginesMap;
	typedef Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<
		EngineAttachment, EngineAttachmentInfo*> >, EngineAttachment> EnginesAttachmentsMap;

	Firebird::RWLock enginesLock;
	EnginesMap engines;
	EnginesAttachmentsMap enginesAttachments;
};


}	// namespace Jrd

#endif	// JRD_EXT_ENGINE_MANAGER_H
