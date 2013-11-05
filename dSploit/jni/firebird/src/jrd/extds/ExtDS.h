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
 *  The Original Code was created by Vlad Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2007 Vlad Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#ifndef EXTDS_H
#define EXTDS_H

#include "../../common/classes/fb_string.h"
#include "../../common/classes/array.h"
#include "../../common/classes/ClumpletWriter.h"
#include "../../common/classes/locks.h"
#include "../../common/utils_proto.h"


namespace Jrd
{
	class jrd_nod;
	class jrd_tra;
	class thread_db;
}

namespace EDS {

class Manager;
class Provider;
class Connection;
class Transaction;
class Statement;
class Blob;

enum TraModes {traReadCommited, traReadCommitedRecVersions, traConcurrency, traConsistency};
enum TraScope {traAutonomous = 1, traCommon, traTwoPhase};

// Known built-in provider's names
extern const char* FIREBIRD_PROVIDER_NAME;
extern const char* INTERNAL_PROVIDER_NAME;


// Manage providers
class Manager : public Firebird::PermanentStorage
{
public:
	explicit Manager(Firebird::MemoryPool& pool);
	~Manager();

	static void addProvider(Provider* provider);
	static Provider* getProvider(const Firebird::string& prvName);
	static Connection* getConnection(Jrd::thread_db* tdbb,
		const Firebird::string& dataSource, const Firebird::string& user,
		const Firebird::string& pwd, const Firebird::string& role, TraScope tra_scope);

	// Notify providers when some jrd attachment is about to be released
	static void jrdAttachmentEnd(Jrd::thread_db* tdbb, Jrd::Attachment* att);

private:
	static void init();
	static int shutdown(const int reason, const int mask, void* arg);

	static Firebird::GlobalPtr<Manager> manager;
	static Firebird::Mutex m_mutex;
	static Provider* m_providers;
	static volatile bool m_initialized;
};


// manages connections\connection pool

class Provider : public Firebird::GlobalStorage
{
	friend class Manager;
	friend class EngineCallbackGuard;

public:
	explicit Provider(const char* prvName);
	virtual ~Provider();

	// return existing or create new Connection
	virtual Connection* getConnection(Jrd::thread_db* tdbb, const Firebird::string& dbName,
		const Firebird::string& user, const Firebird::string& pwd, const Firebird::string& role,
		TraScope tra_scope);

	// Connection gets unused, release it into pool or delete it completely
	virtual void releaseConnection(Jrd::thread_db* tdbb, Connection& conn, bool inPool = true);

	// Notify provider when some jrd attachment is about to be released
	virtual void jrdAttachmentEnd(Jrd::thread_db* tdbb, Jrd::Attachment* att) = 0;

	// cancel execution of every connection
	void cancelConnections(Jrd::thread_db* tdbb);

	const Firebird::string& getName() const { return m_name; }

	virtual void initialize() = 0;

	// Provider properties
	int getFlags() const { return m_flags; }

	// Interprete status and put error description into passed string
	virtual void getRemoteError(ISC_STATUS* status, Firebird::string& err) const = 0;

	static const Firebird::string* generate(const void*, const Provider* item)
	{
		return &item->m_name;
	}

protected:
	void clearConnections(Jrd::thread_db* tdbb);
	virtual Connection* doCreateConnection() = 0;

	// Protection against simultaneous attach database calls. Not sure we still
	// need it, but i believe it will not harm
	Firebird::Mutex m_mutex;

	Firebird::string m_name;
	Provider* m_next;

	Firebird::Array<Connection*> m_connections;
	int m_flags;
};

// Provider flags
const int prvMultyStmts		= 0x0001;	// supports many active statements per connection
const int prvMultyTrans		= 0x0002;	// supports many active transactions per connection
const int prvNamedParams	= 0x0004;	// supports named parameters
const int prvTrustedAuth	= 0x0008;	// supports trusted authentication


class Connection : public Firebird::PermanentStorage
{
protected:
	friend class EngineCallbackGuard;
	friend class Provider;

	explicit Connection(Provider& prov);
	virtual ~Connection();

public:
	static void deleteConnection(Jrd::thread_db* tdbb, Connection* conn);

	Provider* getProvider() { return &m_provider; }

	virtual void attach(Jrd::thread_db* tdbb, const Firebird::string& dbName,
		const Firebird::string& user, const Firebird::string& pwd,
		const Firebird::string& role) = 0;
	virtual void detach(Jrd::thread_db* tdbb);

	virtual bool cancelExecution(Jrd::thread_db* tdbb) = 0;

	int getSqlDialect() const { return m_sqlDialect; }

	// Is this connections can be used by current needs ? Not every DBMS
	// allows to use same connection in more than one transaction and\or
	// to have more than on active statement at time. See also provider
	// flags above.
	virtual bool isAvailable(Jrd::thread_db* tdbb, TraScope traScope) const = 0;

	virtual bool isConnected() const = 0;

	virtual bool isSameDatabase(Jrd::thread_db* tdbb, const Firebird::string& dbName,
		const Firebird::string& user, const Firebird::string& pwd,
		const Firebird::string& role) const;

	// Search for existing transaction of given scope, may return NULL.
	Transaction* findTransaction(Jrd::thread_db* tdbb, TraScope traScope) const;

	const Firebird::string getDataSourceName() const
	{
		return m_provider.getName() + "::" + m_dbName;
	}

	// Get error description from provider and put it with additional context
	// info into locally raised exception
	void raise(ISC_STATUS* status, Jrd::thread_db* tdbb, const char* sWhere);

	// will we wrap external errors into our ones (isc_eds_xxx) or pass them as is
	bool getWrapErrors() const	{ return m_wrapErrors; }
	void setWrapErrors(bool val) { m_wrapErrors = val; }

	// Transactions management within connection scope : put newly created
	// transaction into m_transactions array and delete not needed transaction
	// immediately (as we didn't pool transactions)
	Transaction* createTransaction();
	void deleteTransaction(Transaction* tran);

	// Statements management within connection scope : put newly created
	// statement into m_statements array, but don't delete freed statement
	// immediately (as we did pooled statements). Instead keep it in
	// m_freeStatements list for reuse later
	Statement* createStatement(const Firebird::string& sql);
	void releaseStatement(Jrd::thread_db* tdbb, Statement* stmt);

	virtual Blob* createBlob() = 0;

protected:
	void generateDPB(Jrd::thread_db* tdbb, Firebird::ClumpletWriter& dpb,
		const Firebird::string& user, const Firebird::string& pwd,
		const Firebird::string& role) const;

	virtual Transaction* doCreateTransaction() = 0;
	virtual Statement* doCreateStatement() = 0;

	void clearTransactions(Jrd::thread_db* tdbb);
	void clearStatements(Jrd::thread_db* tdbb);

	virtual void doDetach(Jrd::thread_db* tdbb) = 0;

	// Protection against simultaneous ISC API calls for the same connection
	Firebird::Mutex m_mutex;

	Provider& m_provider;
	Firebird::string m_dbName;
	Firebird::ClumpletWriter m_dpb;

	Firebird::Array<Transaction*> m_transactions;
	Firebird::Array<Statement*> m_statements;
	Statement* m_freeStatements;

	const Jrd::Attachment* m_boundAtt;

	static const int MAX_CACHED_STMTS = 16;
	int	m_used_stmts;
	int	m_free_stmts;
	bool m_deleting;
	int m_sqlDialect;	// must be filled in attach call
	bool m_wrapErrors;
};


class Transaction : public Firebird::PermanentStorage
{
protected:
	friend class Connection;

	// Create and delete only via parent Connection
	explicit Transaction(Connection& conn);
	virtual ~Transaction();

public:

	Provider* getProvider() { return &m_provider; }

	Connection* getConnection() { return &m_connection; }

	TraScope getScope() const { return m_scope; }

	virtual void start(Jrd::thread_db* tdbb, TraScope traScope, TraModes traMode,
		bool readOnly, bool wait, int lockTimeout);
	virtual void prepare(Jrd::thread_db* tdbb, int info_len, const char* info);
	virtual void commit(Jrd::thread_db* tdbb, bool retain);
	virtual void rollback(Jrd::thread_db* tdbb, bool retain);

	static Transaction* getTransaction(Jrd::thread_db* tdbb,
		Connection* conn, TraScope tra_scope);

	// Notification about end of some jrd transaction. Bound external transaction
	// (with traCommon scope) must be ended the same way as local jrd transaction
	static void jrdTransactionEnd(Jrd::thread_db* tdbb, Jrd::jrd_tra* tran,
		bool commit, bool retain, bool force);

protected:
	virtual void generateTPB(Jrd::thread_db* tdbb, Firebird::ClumpletWriter& tpb,
		TraModes traMode, bool readOnly, bool wait, int lockTimeout) const;
	void detachFromJrdTran();

	virtual void doStart(ISC_STATUS* status, Jrd::thread_db* tdbb, Firebird::ClumpletWriter& tpb) = 0;
	virtual void doPrepare(ISC_STATUS* status, Jrd::thread_db* tdbb, int info_len, const char* info) = 0;
	virtual void doCommit(ISC_STATUS* status, Jrd::thread_db* tdbb, bool retain) = 0;
	virtual void doRollback(ISC_STATUS* status, Jrd::thread_db* tdbb, bool retain) = 0;

	Provider& m_provider;
	Connection& m_connection;
	TraScope m_scope;
	Transaction* m_nextTran;		// next common transaction
	Jrd::jrd_tra* m_jrdTran;		// parent JRD transaction
};


typedef Firebird::Array<Firebird::string*> ParamNames;

class Statement : public Firebird::PermanentStorage
{
protected:
	friend class Connection;

	// Create and delete only via parent Connection
	explicit Statement(Connection& conn);
	virtual ~Statement();

public:
	static void deleteStatement(Jrd::thread_db* tdbb, Statement* stmt);

	Provider* getProvider() { return &m_provider; }

	Connection* getConnection() { return &m_connection; }

	Transaction* getTransaction() { return m_transaction; }

	void prepare(Jrd::thread_db* tdbb, Transaction* tran, const Firebird::string& sql, bool named);
	void execute(Jrd::thread_db* tdbb, Transaction* tran, int in_count,
		const Firebird::string* const* in_names, Jrd::jrd_nod** in_params,
		int out_count, Jrd::jrd_nod** out_params);
	void open(Jrd::thread_db* tdbb, Transaction* tran, int in_count,
		const Firebird::string* const* in_names, Jrd::jrd_nod** in_params, bool singleton);
	bool fetch(Jrd::thread_db* tdbb, int out_count, Jrd::jrd_nod** out_params);
	void close(Jrd::thread_db* tdbb);
	void deallocate(Jrd::thread_db* tdbb);

	const Firebird::string& getSql() const { return m_sql; }

	void setCallerPrivileges(bool use) { m_callerPrivileges = use; }

	bool isActive() const { return m_active; }

	bool isAllocated() const { return m_allocated; }

	bool isSelectable() const { return m_stmt_selectable; }

	int getInputs() const { return m_inputs; }

	int getOutputs() const { return m_outputs; }

	// Get error description from provider and put it with additional contex
	// info into locally raised exception
	void raise(ISC_STATUS* status, Jrd::thread_db* tdbb, const char* sWhere,
		const Firebird::string* sQuery = NULL);

	// Active statement must be bound to parent jrd request
	void bindToRequest(Jrd::jrd_req* request, Statement** impure);
	void unBindFromRequest();

protected:
	virtual void doPrepare(Jrd::thread_db* tdbb, const Firebird::string& sql) = 0;
	virtual void doExecute(Jrd::thread_db* tdbb) = 0;
	virtual void doOpen(Jrd::thread_db* tdbb) = 0;
	virtual bool doFetch(Jrd::thread_db* tdbb) = 0;
	virtual void doClose(Jrd::thread_db* tdbb, bool drop) = 0;

	void setInParams(Jrd::thread_db* tdbb, int count, const Firebird::string* const* names,
		Jrd::jrd_nod** params);
	virtual void getOutParams(Jrd::thread_db* tdbb, int count, Jrd::jrd_nod** params);

	virtual void doSetInParams(Jrd::thread_db* tdbb, int count, const Firebird::string* const* names,
		Jrd::jrd_nod** params);

	virtual void putExtBlob(Jrd::thread_db* tdbb, dsc& src, dsc& dst);
	virtual void getExtBlob(Jrd::thread_db* tdbb, const dsc& src, dsc& dst);

	// Preprocess user sql string : replace parameter names by placeholders (?)
	// and remember correspondence between logical parameter names and unnamed
	// placeholders numbers. This is needed only if provider didn't support
	// named parameters natively.
	void preprocess(const Firebird::string& sql, Firebird::string& ret);
	void clearNames();


	Provider	&m_provider;
	Connection	&m_connection;
	Transaction	*m_transaction;

	Statement* m_nextFree;		// next free statement

	Jrd::jrd_req* m_boundReq;
	Statement** m_ReqImpure;
	Statement* m_nextInReq;
	Statement* m_prevInReq;

	Firebird::string m_sql;

	// passed in open()
	bool	m_singleton;

	// set in open()
	bool	m_active;

	// if statement executed in autonomous transaction, it must be rolled back,
	// so track the error condition of a statement
	bool	m_error;

	// set in prepare()
	bool	m_allocated;
	bool	m_stmt_selectable;
	int		m_inputs;
	int		m_outputs;

	bool	m_callerPrivileges;
	Jrd::jrd_req* m_preparedByReq;

	// set in preprocess
	ParamNames m_sqlParamNames;
	ParamNames m_sqlParamsMap;

	// set in prepare()
	Firebird::UCharBuffer m_in_buffer;
	Firebird::UCharBuffer m_out_buffer;
	Firebird::Array<dsc> m_inDescs;
	Firebird::Array<dsc> m_outDescs;
};


class Blob : public Firebird::PermanentStorage
{
	friend class Connection;
protected:
	explicit Blob(Connection& conn) :
		 PermanentStorage(conn.getProvider()->getPool())
	{}

public:
	virtual ~Blob() {}

	virtual void open(Jrd::thread_db* tdbb, Transaction& tran, const dsc& desc,
		const Firebird::UCharBuffer* bpb) = 0;
	virtual void create(Jrd::thread_db* tdbb, Transaction& tran, dsc& desc,
		const Firebird::UCharBuffer* bpb) = 0;
	virtual USHORT read(Jrd::thread_db* tdbb, UCHAR* buff, USHORT len) = 0;
	virtual void write(Jrd::thread_db* tdbb, const UCHAR* buff, USHORT len) = 0;
	virtual void close(Jrd::thread_db* tdbb) = 0;
	virtual void cancel(Jrd::thread_db* tdbb) = 0;
};


class EngineCallbackGuard
{
public:
	EngineCallbackGuard(Jrd::thread_db* tdbb, Connection& conn)
	{
		init(tdbb, conn);
	}

	EngineCallbackGuard(Jrd::thread_db* tdbb, Transaction& tran)
	{
		init(tdbb, *tran.getConnection());
	}

	EngineCallbackGuard(Jrd::thread_db* tdbb, Statement& stmt)
	{
		init(tdbb, *stmt.getConnection());
	}

	~EngineCallbackGuard();

private:
	void init(Jrd::thread_db* tdbb, Connection& conn);

	Jrd::thread_db* m_tdbb;
	Firebird::Mutex* m_mutex;
	Connection* m_saveConnection;
};

} // namespace EDS

#endif // EXTDS_H
