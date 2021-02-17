/*
 * The contents of this file are subject to the Interbase Public
 * License Version 1.0 (the "License"); you may not use this file
 * except in compliance with the License. You may obtain a copy
 * of the License at http://www.Inprise.com/IPL.html
 *
 * Software distributed under the License is distributed on an
 * "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either express
 * or implied. See the License for the specific language governing
 * rights and limitations under the License.
 *
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * Dmitry Yemanov
 * Sean Leyne
 * Alex Peshkoff
 * Adriano dos Santos Fernandes
 *
 */

#ifndef YVALVE_Y_OBJECTS_H
#define YVALVE_Y_OBJECTS_H

#include "firebird.h"
#include "firebird/Provider.h"
#include "gen/iberror.h"
#include "../common/StatusHolder.h"
#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/MsgMetadata.h"

namespace Firebird
{
	class ClumpletWriter;
}

namespace Why
{


class YAttachment;
class YBlob;
class YRequest;
class YResultSet;
class YService;
class YStatement;
class YTransaction;

class YObject
{
public:
	YObject()
		: handle(0)
	{
	}

protected:
	FB_API_HANDLE handle;
};

class CleanupCallback
{
public:
	virtual void FB_CARG cleanupCallbackFunction() = 0;
	virtual ~CleanupCallback() { }
};

template <typename T>
class HandleArray
{
public:
	explicit HandleArray(Firebird::MemoryPool& pool)
		: array(pool)
	{
	}

	void add(T* obj)
	{
		Firebird::MutexLockGuard guard(mtx, FB_FUNCTION);

		array.add(obj);
	}

	void remove(T* obj)
	{
		Firebird::MutexLockGuard guard(mtx, FB_FUNCTION);
		size_t pos;

		if (array.find(obj, pos))
			array.remove(pos);
	}

	void destroy()
	{
		Firebird::MutexLockGuard guard(mtx, FB_FUNCTION);
		size_t i;

		while ((i = array.getCount()) > 0)
			array[i - 1]->destroy();
	}

	void assign(HandleArray& from)
	{
		clear();
		array.assign(from.array);
	}

	void clear()
	{
		array.clear();
	}

private:
	Firebird::Mutex mtx;
	Firebird::SortedArray<T*> array;
};

template <typename Impl, typename Intf, int Vers>
class YHelper : public Firebird::StdPlugin<Intf, Vers>, public YObject
{
public:
	explicit YHelper(Intf* aNext);

	int FB_CARG release()
	{
		if (--this->refCounter == 0)
		{
			Impl* impl = static_cast<Impl*>(this);

			if (next)
			{
				++this->refCounter;		// to be decremented in destroy()
				++this->refCounter;		// to avoid recursion
				next->release();		// reference normally released by detach/rollback/free etc.
				impl->destroy();		// destroy() must call release()
				--this->refCounter;
			}

			delete impl; // call correct destructor !
			return 0;
		}

		return 1;
	}

	void destroy2()
	{
		RefDeb(Firebird::DEB_RLS_JATT, "YValve");
		next = NULL;
		RefDeb(Firebird::DEB_RLS_YATT, "destroy2");
		release();
	}

	typedef Intf NextInterface;
	typedef YAttachment YRef;
	Firebird::RefPtr<NextInterface> next;
};

class YEvents FB_FINAL : public YHelper<YEvents, Firebird::IEvents, FB_EVENTS_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_events_handle;

	YEvents(YAttachment* aAttachment, IEvents* aNext, Firebird::IEventCallback* aCallback);

	void destroy();
	FB_API_HANDLE& getHandle();

	// IEvents implementation
	virtual void FB_CARG cancel(Firebird::IStatus* status);

public:
	YAttachment* attachment;
	Firebird::RefPtr<Firebird::IEventCallback> callback;
};

class YRequest FB_FINAL : public YHelper<YRequest, Firebird::IRequest, FB_REQUEST_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_req_handle;

	YRequest(YAttachment* aAttachment, Firebird::IRequest* aNext);

	void destroy();
	FB_API_HANDLE& getHandle();

	// IRequest implementation
	virtual void FB_CARG receive(Firebird::IStatus* status, int level, unsigned int msgType,
		unsigned int length, unsigned char* message);
	virtual void FB_CARG send(Firebird::IStatus* status, int level, unsigned int msgType,
		unsigned int length, const unsigned char* message);
	virtual void FB_CARG getInfo(Firebird::IStatus* status, int level, unsigned int itemsLength,
		const unsigned char* items, unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG start(Firebird::IStatus* status, Firebird::ITransaction* transaction, int level);
	virtual void FB_CARG startAndSend(Firebird::IStatus* status, Firebird::ITransaction* transaction, int level,
		unsigned int msgType, unsigned int length, const unsigned char* message);
	virtual void FB_CARG unwind(Firebird::IStatus* status, int level);
	virtual void FB_CARG free(Firebird::IStatus* status);

public:
	YAttachment* attachment;
	FB_API_HANDLE* userHandle;
};

class YTransaction FB_FINAL : public YHelper<YTransaction, Firebird::ITransaction, FB_TRANSACTION_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_trans_handle;

	YTransaction(YAttachment* aAttachment, Firebird::ITransaction* aNext);

	void destroy();
	FB_API_HANDLE& getHandle();

	// ITransaction implementation
	virtual void FB_CARG getInfo(Firebird::IStatus* status, unsigned int itemsLength,
		const unsigned char* items, unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG prepare(Firebird::IStatus* status, unsigned int msgLength,
		const unsigned char* message);
	virtual void FB_CARG commit(Firebird::IStatus* status);
	virtual void FB_CARG commitRetaining(Firebird::IStatus* status);
	virtual void FB_CARG rollback(Firebird::IStatus* status);
	virtual void FB_CARG rollbackRetaining(Firebird::IStatus* status);
	virtual void FB_CARG disconnect(Firebird::IStatus* status);
	virtual ITransaction* FB_CARG join(Firebird::IStatus* status, Firebird::ITransaction* transaction);
	virtual ITransaction* FB_CARG validate(Firebird::IStatus* status, Firebird::IAttachment* testAtt);
	virtual YTransaction* FB_CARG enterDtc(Firebird::IStatus* status);

	void addCleanupHandler(Firebird::IStatus* status, CleanupCallback* callback);
	void selfCheck();

public:
	YAttachment* attachment;
	HandleArray<YBlob> childBlobs;
	HandleArray<YResultSet> childCursors;
	Firebird::Array<CleanupCallback*> cleanupHandlers;

private:
	YTransaction(YTransaction* from)
		: YHelper<YTransaction, Firebird::ITransaction, FB_TRANSACTION_VERSION>(from->next),
		  attachment(from->attachment),
		  childBlobs(getPool()),
		  childCursors(getPool()),
		  cleanupHandlers(getPool())
	{
		childBlobs.assign(from->childBlobs);
		from->childBlobs.clear();
		childCursors.assign(from->childCursors);
		from->childCursors.clear();
		cleanupHandlers.assign(from->cleanupHandlers);
		from->cleanupHandlers.clear();
	}
};

typedef Firebird::RefPtr<Firebird::ITransaction> NextTransaction;

class YBlob FB_FINAL : public YHelper<YBlob, Firebird::IBlob, FB_BLOB_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_segstr_handle;

	YBlob(YAttachment* aAttachment, YTransaction* aTransaction, Firebird::IBlob* aNext);

	void destroy();
	FB_API_HANDLE& getHandle();

	// IBlob implementation
	virtual void FB_CARG getInfo(Firebird::IStatus* status, unsigned int itemsLength,
		const unsigned char* items, unsigned int bufferLength, unsigned char* buffer);
	virtual unsigned int FB_CARG getSegment(Firebird::IStatus* status, unsigned int length, void* buffer);
	virtual void FB_CARG putSegment(Firebird::IStatus* status, unsigned int length, const void* buffer);
	virtual void FB_CARG cancel(Firebird::IStatus* status);
	virtual void FB_CARG close(Firebird::IStatus* status);
	virtual int FB_CARG seek(Firebird::IStatus* status, int mode, int offset);

public:
	YAttachment* attachment;
	YTransaction* transaction;
};

class YResultSet FB_FINAL : public YHelper<YResultSet, Firebird::IResultSet, FB_RESULTSET_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_result_set;

	YResultSet(YAttachment* anAttachment, YTransaction* aTransaction, Firebird::IResultSet* aNext);
	YResultSet(YAttachment* anAttachment, YTransaction* aTransaction, YStatement* aStatement,
		Firebird::IResultSet* aNext);

	void destroy();

	// IResultSet implementation
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
	YAttachment* attachment;
	YTransaction* transaction;
	YStatement* statement;
};

class YMetadata
{
public:
	explicit YMetadata(bool in)
		: flag(false), input(in)
	{ }

	Firebird::IMessageMetadata* get(Firebird::IStatement* next, YStatement* statement);

private:
	Firebird::RefPtr<Firebird::MsgMetadata> metadata;
	volatile bool flag;
	bool input;
};

class YStatement FB_FINAL : public YHelper<YStatement, Firebird::IStatement, FB_STATEMENT_VERSION>
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_stmt_handle;

	YStatement(YAttachment* aAttachment, Firebird::IStatement* aNext);

	void destroy();

	// IStatement implementation
	virtual void FB_CARG getInfo(Firebird::IStatus* status,
								 unsigned int itemsLength, const unsigned char* items,
								 unsigned int bufferLength, unsigned char* buffer);
	virtual unsigned FB_CARG getType(Firebird::IStatus* status);
	virtual const char* FB_CARG getPlan(Firebird::IStatus* status, FB_BOOLEAN detailed);
	virtual ISC_UINT64 FB_CARG getAffectedRecords(Firebird::IStatus* status);
	virtual Firebird::IMessageMetadata* FB_CARG getInputMetadata(Firebird::IStatus* status);
	virtual Firebird::IMessageMetadata* FB_CARG getOutputMetadata(Firebird::IStatus* status);
	virtual Firebird::ITransaction* FB_CARG execute(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer);
	virtual Firebird::IResultSet* FB_CARG openCursor(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer, Firebird::IMessageMetadata* outMetadata);
	virtual void FB_CARG free(Firebird::IStatus* status);
	virtual unsigned FB_CARG getFlags(Firebird::IStatus* status);

public:
	Firebird::Mutex statementMutex;
	YAttachment* attachment;
	YResultSet* cursor;

	Firebird::IMessageMetadata* getMetadata(bool in, Firebird::IStatement* next);

private:
	YMetadata input, output;
};

class EnterCount
{
public:
	EnterCount()
		: enterCount(0)
	{}

	~EnterCount()
	{
		fb_assert(enterCount == 0);
	}

	int enterCount;
	Firebird::Mutex enterMutex;
};

class YAttachment FB_FINAL : public YHelper<YAttachment, Firebird::IAttachment, FB_ATTACHMENT_VERSION>, public EnterCount
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_db_handle;

	YAttachment(Firebird::IProvider* aProvider, Firebird::IAttachment* aNext,
		const Firebird::PathName& aDbPath);
	~YAttachment();

	void destroy();
	void shutdown();
	FB_API_HANDLE& getHandle();

	// IAttachment implementation
	virtual void FB_CARG getInfo(Firebird::IStatus* status, unsigned int itemsLength,
		const unsigned char* items, unsigned int bufferLength, unsigned char* buffer);
	virtual YTransaction* FB_CARG startTransaction(Firebird::IStatus* status, unsigned int tpbLength,
		const unsigned char* tpb);
	virtual YTransaction* FB_CARG reconnectTransaction(Firebird::IStatus* status, unsigned int length,
		const unsigned char* id);
	virtual YRequest* FB_CARG compileRequest(Firebird::IStatus* status, unsigned int blrLength,
		const unsigned char* blr);
	virtual void FB_CARG transactRequest(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		unsigned int blrLength, const unsigned char* blr, unsigned int inMsgLength,
		const unsigned char* inMsg, unsigned int outMsgLength, unsigned char* outMsg);
	virtual YBlob* FB_CARG createBlob(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
		unsigned int bpbLength, const unsigned char* bpb);
	virtual YBlob* FB_CARG openBlob(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
		unsigned int bpbLength, const unsigned char* bpb);
	virtual int FB_CARG getSlice(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
		unsigned int sdlLength, const unsigned char* sdl, unsigned int paramLength,
		const unsigned char* param, int sliceLength, unsigned char* slice);
	virtual void FB_CARG putSlice(Firebird::IStatus* status, Firebird::ITransaction* transaction, ISC_QUAD* id,
		unsigned int sdlLength, const unsigned char* sdl, unsigned int paramLength,
		const unsigned char* param, int sliceLength, unsigned char* slice);
	virtual void FB_CARG executeDyn(Firebird::IStatus* status, Firebird::ITransaction* transaction, unsigned int length,
		const unsigned char* dyn);
	virtual YStatement* FB_CARG prepare(Firebird::IStatus* status, Firebird::ITransaction* tra,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect, unsigned int flags);
	virtual Firebird::ITransaction* FB_CARG execute(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer);
	virtual Firebird::IResultSet* FB_CARG openCursor(Firebird::IStatus* status, Firebird::ITransaction* transaction,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer, Firebird::IMessageMetadata* outMetadata);
	virtual YEvents* FB_CARG queEvents(Firebird::IStatus* status, Firebird::IEventCallback* callback,
		unsigned int length, const unsigned char* eventsData);
	virtual void FB_CARG cancelOperation(Firebird::IStatus* status, int option);
	virtual void FB_CARG ping(Firebird::IStatus* status);
	virtual void FB_CARG detach(Firebird::IStatus* status);
	virtual void FB_CARG dropDatabase(Firebird::IStatus* status);

	void addCleanupHandler(Firebird::IStatus* status, CleanupCallback* callback);
	YTransaction* getTransaction(Firebird::IStatus* status, Firebird::ITransaction* tra);
	void getNextTransaction(Firebird::IStatus* status, Firebird::ITransaction* tra, NextTransaction& next);
	void execute(Firebird::IStatus* status, FB_API_HANDLE* traHandle,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer);

public:
	Firebird::IProvider* provider;
	Firebird::PathName dbPath;
	HandleArray<YBlob> childBlobs;
	HandleArray<YEvents> childEvents;
	HandleArray<YRequest> childRequests;
	HandleArray<YStatement> childStatements;
	HandleArray<YTransaction> childTransactions;
	Firebird::Array<CleanupCallback*> cleanupHandlers;
	Firebird::StatusHolder savedStatus;	// Do not use raise() method of this class in yValve.
};

class YService FB_FINAL : public YHelper<YService, Firebird::IService, FB_SERVICE_VERSION>, public EnterCount
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_svc_handle;

	YService(Firebird::IProvider* aProvider, Firebird::IService* aNext, bool utf8);
	~YService();

	void shutdown();
	void destroy();
	FB_API_HANDLE& getHandle();

	// IService implementation
	virtual void FB_CARG detach(Firebird::IStatus* status);
	virtual void FB_CARG query(Firebird::IStatus* status,
		unsigned int sendLength, const unsigned char* sendItems,
		unsigned int receiveLength, const unsigned char* receiveItems,
		unsigned int bufferLength, unsigned char* buffer);
	virtual void FB_CARG start(Firebird::IStatus* status,
		unsigned int spbLength, const unsigned char* spb);

public:
	typedef IService NextInterface;
	typedef YService YRef;

private:
	Firebird::IProvider* provider;
	bool utf8Connection;		// Client talks to us using UTF8, else - system default charset
};

class Dispatcher FB_FINAL : public Firebird::StdPlugin<Firebird::IProvider, FB_PROVIDER_VERSION>
{
public:
	Dispatcher()
		: cryptCallback(NULL)
	{ }

	// IProvider implementation
	virtual YAttachment* FB_CARG attachDatabase(Firebird::IStatus* status, const char* filename,
		unsigned int dpbLength, const unsigned char* dpb);
	virtual YAttachment* FB_CARG createDatabase(Firebird::IStatus* status, const char* filename,
		unsigned int dpbLength, const unsigned char* dpb);
	virtual YService* FB_CARG attachServiceManager(Firebird::IStatus* status, const char* serviceName,
		unsigned int spbLength, const unsigned char* spb);
	virtual void FB_CARG shutdown(Firebird::IStatus* status, unsigned int timeout, const int reason);
	virtual void FB_CARG setDbCryptCallback(Firebird::IStatus* status,
		Firebird::ICryptKeyCallback* cryptCallback);

	virtual int FB_CARG release()
	{
		if (--refCounter == 0)
		{
			delete this;
			return 0;
		}

		return 1;
	}

private:
	YAttachment* attachOrCreateDatabase(Firebird::IStatus* status, bool createFlag,
		const char* filename, unsigned int dpbLength, const unsigned char* dpb);

	Firebird::ICryptKeyCallback* cryptCallback;
};


}	// namespace Why

#endif	// YVALVE_Y_OBJECTS_H
