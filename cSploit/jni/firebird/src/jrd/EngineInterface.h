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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2011 Alex Peshkov <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef JRD_ENGINE_INTERFACE_H
#define JRD_ENGINE_INTERFACE_H

#include "firebird/Provider.h"
#include "../common/classes/ImplementHelper.h"
#include "../common/StatementMetadata.h"
#include "../common/classes/RefCounted.h"

#define CURRENT_ENGINE "Engine12"

namespace Jrd {

// Engine objects used by interface objects
class blb;
class jrd_tra;
class dsql_req;
class JrdStatement;
class Attachment;
class Service;

// forward declarations
class JStatement;
class JAttachment;
class JProvider;

class JBlob FB_FINAL : public Firebird::RefCntIface<Firebird::IBlob, FB_BLOB_VERSION>
{
public:
	// IBlob implementation
	virtual int FB_CARG release();
	virtual void FB_CARG getInfo(Firebird::IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer);
	virtual unsigned int FB_CARG getSegment(Firebird::IStatus* status, unsigned int length, void* buffer);	// returns real length
	virtual void FB_CARG putSegment(Firebird::IStatus* status, unsigned int length, const void* buffer);
	virtual void FB_CARG cancel(Firebird::IStatus* status);
	virtual void FB_CARG close(Firebird::IStatus* status);
	virtual int FB_CARG seek(Firebird::IStatus* status, int mode, int offset);			// returns position

public:
	JBlob(blb* handle, JAttachment* ja)
		: blob(handle), jAtt(ja)
	{
	}

	JAttachment* getAttachment()
	{
		return jAtt;
	}

	blb* getHandle() throw()
	{
		return blob;
	}

private:
	blb* blob;
	Firebird::RefPtr<JAttachment> jAtt;

	void freeEngineData(Firebird::IStatus* status);
};

class JTransaction FB_FINAL : public Firebird::RefCntIface<Firebird::ITransaction, FB_TRANSACTION_VERSION>
{
public:
	// ITransaction implementation
	virtual int FB_CARG release();
	virtual void FB_CARG getInfo(Firebird::IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG prepare(Firebird::IStatus* status,
						 unsigned int msg_length = 0, const unsigned char* message = 0);
	virtual void FB_CARG commit(Firebird::IStatus* status);
	virtual void FB_CARG commitRetaining(Firebird::IStatus* status);
	virtual void FB_CARG rollback(Firebird::IStatus* status);
	virtual void FB_CARG rollbackRetaining(Firebird::IStatus* status);
	virtual void FB_CARG disconnect(Firebird::IStatus* status);
	virtual Firebird::ITransaction* FB_CARG join(Firebird::IStatus* status, Firebird::ITransaction* transaction);
	virtual JTransaction* FB_CARG validate(Firebird::IStatus* status, Firebird::IAttachment* testAtt);
	virtual JTransaction* FB_CARG enterDtc(Firebird::IStatus* status);

public:
	JTransaction(jrd_tra* handle, JAttachment* ja)
		: transaction(handle), jAtt(ja)
	{
	}

	jrd_tra* getHandle() throw()
	{
		return transaction;
	}

	void setHandle(jrd_tra* handle)
	{
		transaction = handle;
	}

	JAttachment* getAttachment()
	{
		return jAtt;
	}

	void clear()
	{
		transaction = NULL;
		release();
	}

private:
	jrd_tra* transaction;
	Firebird::RefPtr<JAttachment> jAtt;

	JTransaction(JTransaction* from)
		: transaction(from->transaction), jAtt(from->jAtt)
	{
	}

	void freeEngineData(Firebird::IStatus* status);
};

class JResultSet FB_FINAL : public Firebird::RefCntIface<Firebird::IResultSet, FB_RESULTSET_VERSION>
{
public:
	// IResultSet implementation
	virtual int FB_CARG release();
	virtual FB_BOOLEAN FB_CARG fetchNext(Firebird::IStatus* status, void* message);
	virtual FB_BOOLEAN FB_CARG fetchPrior(Firebird::IStatus* status, void* message);
	virtual FB_BOOLEAN FB_CARG fetchFirst(Firebird::IStatus* status, void* message);
	virtual FB_BOOLEAN FB_CARG fetchLast(Firebird::IStatus* status, void* message);
	virtual FB_BOOLEAN FB_CARG fetchAbsolute(Firebird::IStatus* status, unsigned int position, void* message);
	virtual FB_BOOLEAN FB_CARG fetchRelative(Firebird::IStatus* status, int offset, void* message);
	virtual FB_BOOLEAN FB_CARG isEof(Firebird::IStatus* status);
	virtual FB_BOOLEAN FB_CARG isBof(Firebird::IStatus* status);
	virtual Firebird::IMessageMetadata* FB_CARG getMetadata(Firebird::IStatus* status);
	virtual void FB_CARG setCursorName(Firebird::IStatus* status, const char* name);
	virtual void FB_CARG close(Firebird::IStatus* status);
	virtual void FB_CARG setDelayedOutputFormat(Firebird::IStatus* status, Firebird::IMessageMetadata* format);

public:
	JResultSet(JStatement* aStatement)
		: statement(aStatement), eof(false)
	{
	}

	JStatement* getStatement()
	{
		return statement;
	}

	JAttachment* getAttachment();

	// Change after adding separate handle for cursor in dsql
	dsql_req* getHandle() throw();

private:
	Firebird::RefPtr<JStatement> statement;
	bool eof;

	void freeEngineData(Firebird::IStatus* status);
};

class JStatement FB_FINAL : public Firebird::RefCntIface<Firebird::IStatement, FB_STATEMENT_VERSION>
{
public:
	// IStatement implementation
	virtual int FB_CARG release();
	virtual void FB_CARG getInfo(Firebird::IStatus* status,
								 unsigned int itemsLength, const unsigned char* items,
								 unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG free(Firebird::IStatus* status);
	virtual ISC_UINT64 FB_CARG getAffectedRecords(Firebird::IStatus* userStatus);
	virtual Firebird::IMessageMetadata* FB_CARG getOutputMetadata(Firebird::IStatus* userStatus);
	virtual Firebird::IMessageMetadata* FB_CARG getInputMetadata(Firebird::IStatus* userStatus);
	virtual unsigned FB_CARG getType(Firebird::IStatus* status);
    virtual const char* FB_CARG getPlan(Firebird::IStatus* status, FB_BOOLEAN detailed);
	virtual Firebird::ITransaction* FB_CARG execute(Firebird::IStatus* status,
		Firebird::ITransaction* transaction, Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer);
	virtual JResultSet* FB_CARG openCursor(Firebird::IStatus* status,
		Firebird::ITransaction* transaction, Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata);
	virtual unsigned FB_CARG getFlags(Firebird::IStatus* status);

public:
	JStatement(dsql_req* handle, JAttachment* ja, Firebird::Array<UCHAR>& meta);

	JAttachment* getAttachment()
	{
		return jAtt;
	}

	dsql_req* getHandle() throw()
	{
		return statement;
	}

private:
	dsql_req* statement;
	Firebird::RefPtr<JAttachment> jAtt;
	Firebird::StatementMetadata metadata;

	void freeEngineData(Firebird::IStatus* status);
};

inline JAttachment* JResultSet::getAttachment()
{
	return statement->getAttachment();
}

// Change after adding separate handle for cursor in dsql
inline dsql_req* JResultSet::getHandle() throw()
{
	return statement->getHandle();
}

class JRequest FB_FINAL : public Firebird::RefCntIface<Firebird::IRequest, FB_REQUEST_VERSION>
{
public:
	// IRequest implementation
	virtual int FB_CARG release();
	virtual void FB_CARG receive(Firebird::IStatus* status, int level, unsigned int msg_type,
						 unsigned int length, unsigned char* message);
	virtual void FB_CARG send(Firebird::IStatus* status, int level, unsigned int msg_type,
					  unsigned int length, const unsigned char* message);
	virtual void FB_CARG getInfo(Firebird::IStatus* status, int level,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG start(Firebird::IStatus* status, Firebird::ITransaction* tra, int level);
	virtual void FB_CARG startAndSend(Firebird::IStatus* status, Firebird::ITransaction* tra, int level, unsigned int msg_type,
							  unsigned int length, const unsigned char* message);
	virtual void FB_CARG unwind(Firebird::IStatus* status, int level);
	virtual void FB_CARG free(Firebird::IStatus* status);

public:
	JRequest(JrdStatement* handle, JAttachment* ja)
		: rq(handle), jAtt(ja)
	{
	}

	JAttachment* getAttachment()
	{
		return jAtt;
	}

	JrdStatement* getHandle() throw()
	{
		return rq;
	}

private:
	JrdStatement* rq;
	Firebird::RefPtr<JAttachment> jAtt;

	void freeEngineData(Firebird::IStatus* status);
};

class JEvents FB_FINAL : public Firebird::RefCntIface<Firebird::IEvents, FB_EVENTS_VERSION>
{
public:
	// IEvents implementation
	virtual int FB_CARG release();
	virtual void FB_CARG cancel(Firebird::IStatus* status);

public:
	JEvents(int aId, JAttachment* ja, Firebird::IEventCallback* aCallback)
		: id(aId), jAtt(ja), callback(aCallback)
	{
	}

	JEvents* getHandle() throw()
	{
		return this;
	}

	JAttachment* getAttachment()
	{
		return jAtt;
	}

private:
	int id;
	Firebird::RefPtr<JAttachment> jAtt;
	Firebird::RefPtr<Firebird::IEventCallback> callback;

	void freeEngineData(Firebird::IStatus* status);
};

class JAttachment : public Firebird::RefCntIface<Firebird::IAttachment, FB_ATTACHMENT_VERSION>
{
public:
	// IAttachment implementation
	virtual int FB_CARG release();
	virtual void FB_CARG getInfo(Firebird::IStatus* status,
						 unsigned int itemsLength, const unsigned char* items,
						 unsigned int bufferLength, unsigned char* buffer);
	virtual JTransaction* FB_CARG startTransaction(Firebird::IStatus* status,
		unsigned int tpbLength, const unsigned char* tpb);
	virtual JTransaction* FB_CARG reconnectTransaction(Firebird::IStatus* status, unsigned int length, const unsigned char* id);
	virtual JRequest* FB_CARG compileRequest(Firebird::IStatus* status, unsigned int blr_length, const unsigned char* blr);
	virtual void FB_CARG transactRequest(Firebird::IStatus* status, Firebird::ITransaction* transaction,
								 unsigned int blr_length, const unsigned char* blr,
								 unsigned int in_msg_length, const unsigned char* in_msg,
								 unsigned int out_msg_length, unsigned char* out_msg);
	virtual JBlob* FB_CARG createBlob(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		ISC_QUAD* id, unsigned int bpbLength = 0, const unsigned char* bpb = 0);
	virtual JBlob* FB_CARG openBlob(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		ISC_QUAD* id, unsigned int bpbLength = 0, const unsigned char* bpb = 0);
	virtual int FB_CARG getSlice(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
						 unsigned int sdl_length, const unsigned char* sdl,
						 unsigned int param_length, const unsigned char* param,
						 int sliceLength, unsigned char* slice);
	virtual void FB_CARG putSlice(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
						  unsigned int sdl_length, const unsigned char* sdl,
						  unsigned int param_length, const unsigned char* param,
						  int sliceLength, unsigned char* slice);
	virtual void FB_CARG executeDyn(Firebird::IStatus* status, Firebird::ITransaction* transaction, unsigned int length,
		const unsigned char* dyn);
	virtual JStatement* FB_CARG prepare(Firebird::IStatus* status, Firebird::ITransaction* tra,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect, unsigned int flags);
	virtual Firebird::ITransaction* FB_CARG execute(Firebird::IStatus* status,
		Firebird::ITransaction* transaction, unsigned int stmtLength, const char* sqlStmt,
		unsigned int dialect, Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer);
	virtual Firebird::IResultSet* FB_CARG openCursor(Firebird::IStatus* status,
		Firebird::ITransaction* transaction, unsigned int stmtLength, const char* sqlStmt,
		unsigned int dialect, Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata);
	virtual JEvents* FB_CARG queEvents(Firebird::IStatus* status, Firebird::IEventCallback* callback,
											unsigned int length, const unsigned char* events);
	virtual void FB_CARG cancelOperation(Firebird::IStatus* status, int option);
	virtual void FB_CARG ping(Firebird::IStatus* status);
	virtual void FB_CARG detach(Firebird::IStatus* status);
	virtual void FB_CARG dropDatabase(Firebird::IStatus* status);

public:
	explicit JAttachment(Attachment* handle);

	Attachment* getHandle() throw()
	{
		return att;
	}

	JAttachment* getAttachment()
	{
		return this;
	}

	Firebird::Mutex* getMutex(bool useAsync = false, bool forceAsync = false)
	{
		if (useAsync && !forceAsync)
		{
			fb_assert(!mainMutex.locked());
		}
		return useAsync ? &asyncMutex : &mainMutex;
	}

	void cancel()
	{
		fb_assert(asyncMutex.locked());
		fb_assert(mainMutex.locked());

		att = NULL;
	}

	JTransaction* getTransactionInterface(Firebird::IStatus* status, Firebird::ITransaction* tra);
	jrd_tra* getEngineTransaction(Firebird::IStatus* status, Firebird::ITransaction* tra);

	void manualLock(ULONG& flags);
	void manualUnlock(ULONG& flags);
	void manualAsyncUnlock(ULONG& flags);

private:
	Attachment* att;
	// These mutexes guarantee attachment existence. After releasing both of them with possibly
	// zero att_use_count one should check does attachment still exists calling getHandle().
	Firebird::Mutex mainMutex, asyncMutex;

	void freeEngineData(Firebird::IStatus* status);
};

// internal class used in system background threads
class SysAttachment FB_FINAL : public JAttachment
{
public:
	explicit SysAttachment(Attachment* handle);

	void initDone();

	virtual int FB_CARG release()
	{
		if (--refCounter != 0)
			return 1;

		Attachment* attachment = getHandle();
		if (attachment)
		{
			destroy(attachment);
		}

		delete this;
		return 0;
	}

private:
	void destroy(Attachment* attachment);
};

class JService FB_FINAL : public Firebird::RefCntIface<Firebird::IService, FB_SERVICE_VERSION>
{
public:
	// IService implementation
	virtual int FB_CARG release();
	virtual void FB_CARG detach(Firebird::IStatus* status);
	virtual void FB_CARG query(Firebird::IStatus* status,
					   unsigned int sendLength, const unsigned char* sendItems,
					   unsigned int receiveLength, const unsigned char* receiveItems,
					   unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG start(Firebird::IStatus* status,
					   unsigned int spbLength, const unsigned char* spb);

public:
	explicit JService(Service* handle);
	Firebird::Mutex mutex;
	Service* svc;

private:
	void freeEngineData(Firebird::IStatus* status);
};

class JProvider FB_FINAL : public Firebird::StdPlugin<Firebird::IProvider, FB_PROVIDER_VERSION>
{
public:
	explicit JProvider(Firebird::IPluginConfig* pConf)
		: cryptCallback(NULL), pluginConfig(pConf)
	{
	}

	static Firebird::RefPtr<JProvider> getInstance()
	{
		Firebird::RefPtr<JProvider> p(new JProvider(NULL));
		return p;
	}

	// IProvider implementation
	virtual JAttachment* FB_CARG attachDatabase(Firebird::IStatus* status, const char* fileName,
		unsigned int dpbLength, const unsigned char* dpb);
	virtual JAttachment* FB_CARG createDatabase(Firebird::IStatus* status, const char* fileName,
		unsigned int dpbLength, const unsigned char* dpb);
	virtual JService* FB_CARG attachServiceManager(Firebird::IStatus* status, const char* service,
		unsigned int spbLength, const unsigned char* spb);
	virtual void FB_CARG shutdown(Firebird::IStatus* status, unsigned int timeout, const int reason);
	virtual void FB_CARG setDbCryptCallback(Firebird::IStatus* status,
		Firebird::ICryptKeyCallback* cryptCallback);

	virtual int FB_CARG release();

private:
	Firebird::ICryptKeyCallback* cryptCallback;
	Firebird::IPluginConfig* pluginConfig;
};

inline JStatement::JStatement(dsql_req* handle, JAttachment* ja, Firebird::Array<UCHAR>& meta)
	: statement(handle), jAtt(ja), metadata(getPool(), this, ja)
{
	metadata.parse(meta.getCount(), meta.begin());
}

} // namespace Jrd

#endif // JRD_ENGINE_INTERFACE_H
