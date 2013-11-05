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
 *  The Original Code was created by Vladyslav Khorsun for the
 *  Firebird Open Source RDBMS project and based on execute_statement
 *	module by Alexander Peshkoff.
 *
 *  Copyright (c) 2007 Vladyslav Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "fb_types.h"
#include "../common.h"
#include "../../include/fb_blk.h"
#include "fb_exception.h"
#include "iberror.h"

#include "../../dsql/chars.h"
#include "../dsc.h"
#include "../exe.h"
#include "ExtDS.h"
#include "../jrd.h"
#include "../tra.h"

#include "../blb_proto.h"
#include "../exe_proto.h"
#include "../err_proto.h"
#include "../evl_proto.h"
#include "../intl_proto.h"
#include "../mov_proto.h"

#include "../jrd/ibase.h"

using namespace Jrd;
using namespace Firebird;


namespace EDS {
// Manager

GlobalPtr<Manager> Manager::manager;
Mutex Manager::m_mutex;
Provider* Manager::m_providers = NULL;
volatile bool Manager::m_initialized = false;

Manager::Manager(MemoryPool& pool) :
	PermanentStorage(pool)
{
}

Manager::~Manager()
{
	while (m_providers)
	{
		Provider* to_delete = m_providers;
		m_providers = m_providers->m_next;
		delete to_delete;
	}
}

void Manager::init()
{
	fb_shutdown_callback(0, shutdown, fb_shut_preproviders, 0);
}

void Manager::addProvider(Provider* provider)
{
	for (const Provider* prv = m_providers; prv; prv = prv->m_next)
	{
		if (prv->m_name == provider->m_name) {
			return;
		}
	}

	provider->m_next = m_providers;
	m_providers = provider;
	provider->initialize();
}

Provider* Manager::getProvider(const string& prvName)
{
	for (Provider* prv = m_providers; prv; prv = prv->m_next)
	{
		if (prv->m_name == prvName) {
			return prv;
		}
	}

	// External Data Source provider ''@1'' not found
	ERR_post(Arg::Gds(isc_eds_provider_not_found) << Arg::Str(prvName));
	return NULL;
}

Connection* Manager::getConnection(thread_db* tdbb, const string& dataSource,
	const string& user, const string& pwd, const string& role, TraScope tra_scope)
{
	if (!m_initialized)
	{
		Database::CheckoutLockGuard guard(tdbb->getDatabase(), m_mutex);
		if (!m_initialized)
		{
			init();
			m_initialized = true;
		}
	}

	// dataSource : registered data source name
	// or connection string : provider::database
	string prvName, dbName;

	if (dataSource.isEmpty())
	{
		prvName = INTERNAL_PROVIDER_NAME;
		dbName = tdbb->getDatabase()->dbb_database_name.c_str();
	}
	else
	{
		size_t pos = dataSource.find("::");
		if (pos != string::npos)
		{
			prvName = dataSource.substr(0, pos);
			dbName = dataSource.substr(pos + 2);
		}
		else
		{
			// search dataSource at registered data sources and get connection
			// string, user and password from this info if found

			// if not found - treat dataSource as Firebird's connection string
			prvName = FIREBIRD_PROVIDER_NAME;
			dbName = dataSource;
		}
	}

	Provider* prv = getProvider(prvName);
	return prv->getConnection(tdbb, dbName, user, pwd, role, tra_scope);
}

void Manager::jrdAttachmentEnd(thread_db* tdbb, Jrd::Attachment* att)
{
	for (Provider* prv = m_providers; prv; prv = prv->m_next) {
		prv->jrdAttachmentEnd(tdbb, att);
	}
}

int Manager::shutdown(const int /*reason*/, const int /*mask*/, void* /*arg*/)
{
	thread_db* tdbb = JRD_get_thread_data();
	for (Provider* prv = m_providers; prv; prv = prv->m_next) {
		prv->cancelConnections(tdbb);
	}
	return 0;
}


// Provider

Provider::Provider(const char* prvName) :
	m_name(getPool()),
	m_connections(getPool()),
	m_flags(0)
{
	m_name = prvName;
}

Provider::~Provider()
{
	thread_db* tdbb = JRD_get_thread_data();
	clearConnections(tdbb);
}

Connection* Provider::getConnection(thread_db* tdbb, const string& dbName,
	const string& user, const string& pwd, const string& role, TraScope tra_scope)
{
	const Attachment* attachment = tdbb->getAttachment();

	if (attachment->att_ext_call_depth >= MAX_CALLBACKS)
		ERR_post(Arg::Gds(isc_exec_sql_max_call_exceeded));

	{ // m_mutex scope
		Database::CheckoutLockGuard guard(tdbb->getDatabase(), m_mutex);

		Connection** conn_ptr = m_connections.begin();
		Connection** end = m_connections.end();

		for (; conn_ptr < end; conn_ptr++)
		{
			Connection* conn = *conn_ptr;
			if (conn->m_boundAtt == attachment &&
				conn->isSameDatabase(tdbb, dbName, user, pwd, role) &&
				conn->isAvailable(tdbb, tra_scope))
			{
				return conn;
			}
		}
	}

	Connection* conn = doCreateConnection();
	try
	{
		conn->attach(tdbb, dbName, user, pwd, role);
		conn->m_boundAtt = attachment;
	}
	catch (...)
	{
		Connection::deleteConnection(tdbb, conn);
		throw;
	}

	{ // m_mutex scope
		Database::CheckoutLockGuard guard(tdbb->getDatabase(), m_mutex);
		m_connections.add(conn);
	}

	return conn;
}

// hvlad: in current implementation I didn't return connections in pool as
// I have not implemented way to delete long idle connections.
void Provider::releaseConnection(thread_db* tdbb, Connection& conn, bool /*inPool*/)
{
	{ // m_mutex scope
		Database::CheckoutLockGuard guard(tdbb->getDatabase(), m_mutex);

		conn.m_boundAtt = NULL;

		size_t pos;
		if (!m_connections.find(&conn, pos))
		{
			fb_assert(false);
			return;
		}

		m_connections.remove(pos);
	}
	Connection::deleteConnection(tdbb, &conn);
}

void Provider::clearConnections(thread_db* tdbb)
{
	fb_assert(!tdbb || !tdbb->getDatabase());

	MutexLockGuard guard(m_mutex);

	Connection** ptr = m_connections.begin();
	Connection** end = m_connections.end();

	for (; ptr < end; ptr++)
	{
		Connection::deleteConnection(tdbb, *ptr);
		*ptr = NULL;
	}

	m_connections.clear();
}

void Provider::cancelConnections(thread_db* tdbb)
{
	fb_assert(!tdbb || !tdbb->getDatabase());

	MutexLockGuard guard(m_mutex);

	Connection** ptr = m_connections.begin();
	Connection** end = m_connections.end();

	for (; ptr < end; ptr++) {
		(*ptr)->cancelExecution(tdbb);
	}
}

// Connection

Connection::Connection(Provider& prov) :
	PermanentStorage(prov.getPool()),
	m_provider(prov),
	m_dbName(getPool()),
	m_dpb(getPool(), ClumpletReader::Tagged, MAX_DPB_SIZE),
	m_transactions(getPool()),
	m_statements(getPool()),
	m_freeStatements(NULL),
	m_boundAtt(NULL),
	m_used_stmts(0),
	m_free_stmts(0),
	m_deleting(false),
	m_sqlDialect(0),
	m_wrapErrors(true)
{
}

void Connection::deleteConnection(thread_db* tdbb, Connection* conn)
{
	conn->m_deleting = true;

	if (conn->isConnected())
		conn->detach(tdbb);

	delete conn;
}

Connection::~Connection()
{
}

void Connection::generateDPB(thread_db* tdbb, ClumpletWriter& dpb,
	const string& user, const string& pwd, const string& role) const
{
	dpb.reset(isc_dpb_version1);

	const Attachment *attachment = tdbb->getAttachment();
	dpb.insertInt(isc_dpb_ext_call_depth, attachment->att_ext_call_depth + 1);

	const string& attUser = attachment->att_user->usr_user_name;
	const string& attRole = attachment->att_user->usr_sql_role_name;

	if ((m_provider.getFlags() & prvTrustedAuth) &&
		(user.isEmpty() || user == attUser) && pwd.isEmpty() &&
		(role.isEmpty() || role == attRole))
	{
		dpb.insertString(isc_dpb_trusted_auth, attUser);
		dpb.insertString(isc_dpb_trusted_role, attRole);
	}
	else
	{
		if (!user.isEmpty()) {
			dpb.insertString(isc_dpb_user_name, user);
		}
		if (!pwd.isEmpty()) {
			dpb.insertString(isc_dpb_password, pwd);
		}
		if (!role.isEmpty()) {
			dpb.insertString(isc_dpb_sql_role_name, role);
		}
	}

	CharSet* const cs = INTL_charset_lookup(tdbb, attachment->att_charset);
	if (cs) {
		dpb.insertString(isc_dpb_lc_ctype, cs->getName());
	}
}

bool Connection::isSameDatabase(thread_db* tdbb, const string& dbName,
	const string& user, const string& pwd, const string& role) const
{
	if (m_dbName != dbName)
		return false;

	ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);
	generateDPB(tdbb, dpb, user, pwd, role);

	return m_dpb.simpleCompare(dpb);
}


Transaction* Connection::createTransaction()
{
	Transaction* tran = doCreateTransaction();
	m_transactions.add(tran);
	return tran;
}

void Connection::deleteTransaction(Transaction* tran)
{
	size_t pos;
	if (m_transactions.find(tran, pos))
	{
		m_transactions.remove(pos);
		delete tran;
	}
	else {
		fb_assert(false);
	}

	if (!m_used_stmts && m_transactions.getCount() == 0 && !m_deleting)
		m_provider.releaseConnection(JRD_get_thread_data(), *this);
}

Statement* Connection::createStatement(const string& sql)
{
	m_used_stmts++;

	for (Statement** stmt_ptr = &m_freeStatements; *stmt_ptr; stmt_ptr = &(*stmt_ptr)->m_nextFree)
	{
		Statement* stmt = *stmt_ptr;
		if (stmt->getSql() == sql)
		{
			*stmt_ptr = stmt->m_nextFree;
			stmt->m_nextFree = NULL;
			m_free_stmts--;
			return stmt;
		}
	}

	if (m_free_stmts >= MAX_CACHED_STMTS)
	{
		Statement* stmt = m_freeStatements;
		m_freeStatements = stmt->m_nextFree;
		stmt->m_nextFree = NULL;
		m_free_stmts--;
		return stmt;
	}

	Statement* stmt = doCreateStatement();
	m_statements.add(stmt);
	return stmt;
}

void Connection::releaseStatement(Jrd::thread_db* tdbb, Statement* stmt)
{
	fb_assert(stmt && !stmt->isActive());

	if (stmt->isAllocated() && m_free_stmts < MAX_CACHED_STMTS)
	{
		stmt->m_nextFree = m_freeStatements;
		m_freeStatements = stmt;
		m_free_stmts++;
	}
	else
	{
		size_t pos;
		if (m_statements.find(stmt, pos))
		{
			m_statements.remove(pos);
			Statement::deleteStatement(tdbb, stmt);
		}
		else {
			fb_assert(false);
		}
	}

	m_used_stmts--;

	if (!m_used_stmts && m_transactions.getCount() == 0 && !m_deleting)
		m_provider.releaseConnection(tdbb, *this);
}

void Connection::clearTransactions(Jrd::thread_db* tdbb)
{
	while (m_transactions.getCount())
	{
		Transaction* tran = m_transactions[0];
		tran->rollback(tdbb, false);
	}
}

void Connection::clearStatements(thread_db* tdbb)
{
	Statement** stmt_ptr = m_statements.begin();
	Statement** end = m_statements.end();

	for (; stmt_ptr < end; stmt_ptr++)
	{
		Statement* stmt = *stmt_ptr;
		if (stmt->isActive())
			stmt->close(tdbb);
		Statement::deleteStatement(tdbb, stmt);
	}

	m_statements.clear();

	m_freeStatements = NULL;
	m_free_stmts = m_used_stmts = 0;
}

void Connection::detach(thread_db* tdbb)
{
	const bool was_deleting = m_deleting;
	m_deleting = true;

	try
	{
		clearStatements(tdbb);
		clearTransactions(tdbb);
		m_deleting = was_deleting;
	}
	catch (...)
	{
		m_deleting = was_deleting;
		throw;
	}

	fb_assert(m_used_stmts == 0);
	fb_assert(m_transactions.getCount() == 0);

	doDetach(tdbb);
}

Transaction* Connection::findTransaction(thread_db* tdbb, TraScope traScope) const
{
	jrd_tra* tran = tdbb->getTransaction();
	Transaction* ext_tran = NULL;

	switch (traScope)
	{
	case traCommon :
		ext_tran = tran->tra_ext_common;
		while (ext_tran)
		{
			if (ext_tran->getConnection() == this)
				break;
			ext_tran = ext_tran->m_nextTran;
		}
		break;

	case traTwoPhase :
		ERR_post(Arg::Gds(isc_random) << Arg::Str("2PC transactions not implemented"));

		//ext_tran = tran->tra_ext_two_phase;
		// join transaction if not already joined
		break;
	}

	return ext_tran;
}

void Connection::raise(ISC_STATUS* status, thread_db* tdbb, const char* sWhere)
{
	if (!getWrapErrors())
	{
		ERR_post(Arg::StatusVector(status));
	}

	string rem_err;
	m_provider.getRemoteError(status, rem_err);

	// Execute statement error at @1 :\n@2Data source : @3
	ERR_post(Arg::Gds(isc_eds_connection) << Arg::Str(sWhere) <<
											 Arg::Str(rem_err) <<
											 Arg::Str(getDataSourceName()));
}


// Transaction

Transaction::Transaction(Connection& conn) :
	PermanentStorage(conn.getProvider()->getPool()),
	m_provider(*conn.getProvider()),
	m_connection(conn),
	m_scope(traCommon),
	m_nextTran(0)
{
}

Transaction::~Transaction()
{
}

void Transaction::generateTPB(thread_db* tdbb, ClumpletWriter& tpb,
		TraModes traMode, bool readOnly, bool wait, int lockTimeout) const
{
	switch (traMode)
	{
	case traReadCommited:
		tpb.insertTag(isc_tpb_read_committed);
		break;

	case traReadCommitedRecVersions:
		tpb.insertTag(isc_tpb_read_committed);
		tpb.insertTag(isc_tpb_rec_version);
		break;

	case traConcurrency:
		tpb.insertTag(isc_tpb_concurrency);
		break;

	case traConsistency:
		tpb.insertTag(isc_tpb_consistency);
		break;
	}

	tpb.insertTag(readOnly ? isc_tpb_read : isc_tpb_write);
	tpb.insertTag(wait ? isc_tpb_wait : isc_tpb_nowait);

	if (wait && lockTimeout && lockTimeout != DEFAULT_LOCK_TIMEOUT)
		tpb.insertInt(isc_tpb_lock_timeout, lockTimeout);
}

void Transaction::start(thread_db* tdbb, TraScope traScope, TraModes traMode,
	bool readOnly, bool wait, int lockTimeout)
{
	m_scope = traScope;

	ClumpletWriter tpb(ClumpletReader::Tpb, 64, isc_tpb_version3);
	generateTPB(tdbb, tpb, traMode, readOnly, wait, lockTimeout);

	ISC_STATUS_ARRAY status = {0};
	doStart(status, tdbb, tpb);

	if (status[1]) {
		m_connection.raise(status, tdbb, "transaction start");
	}

	jrd_tra* tran = tdbb->getTransaction();
	switch (m_scope)
	{
	case traCommon:
		this->m_nextTran = tran->tra_ext_common;
		this->m_jrdTran = tran;
		tran->tra_ext_common = this;
		break;

	case traTwoPhase:
		// join transaction
		// this->m_jrdTran = tran;
		// tran->tra_ext_two_phase = ext_tran;
		break;
	}
}

void Transaction::prepare(thread_db* tdbb, int info_len, const char* info)
{
	ISC_STATUS_ARRAY status = {0};
	doPrepare(status, tdbb, info_len, info);

	if (status[1]) {
		m_connection.raise(status, tdbb, "transaction prepare");
	}
}

void Transaction::commit(thread_db* tdbb, bool retain)
{
	ISC_STATUS_ARRAY status = {0};
	doCommit(status, tdbb, retain);

	if (status[1]) {
		m_connection.raise(status, tdbb, "transaction commit");
	}

	if (!retain)
	{
		detachFromJrdTran();
		m_connection.deleteTransaction(this);
	}
}

void Transaction::rollback(thread_db* tdbb, bool retain)
{
	ISC_STATUS_ARRAY status = {0};
	doRollback(status, tdbb, retain);

	Connection& conn = m_connection;
	if (!retain)
	{
		detachFromJrdTran();
		m_connection.deleteTransaction(this);
	}

	if (status[1]) {
		conn.raise(status, tdbb, "transaction rollback");
	}
}

Transaction* Transaction::getTransaction(thread_db* tdbb, Connection* conn, TraScope tra_scope)
{
	jrd_tra* tran = tdbb->getTransaction();
	Transaction* ext_tran = conn->findTransaction(tdbb, tra_scope);

	if (!ext_tran)
	{
		ext_tran = conn->createTransaction();

		TraModes traMode = traConcurrency;
		if (tran->tra_flags & TRA_read_committed)
		{
			if (tran->tra_flags & TRA_rec_version)
				traMode = traReadCommitedRecVersions;
			else
				traMode = traReadCommited;
		}
		else if (tran->tra_flags & TRA_degree3) {
			traMode = traConsistency;
		}

		try {
			ext_tran->start(tdbb,
				tra_scope,
				traMode,
				tran->tra_flags & TRA_readonly,
				tran->getLockWait() != 0,
				-tran->getLockWait()
			);
		}
		catch (const Exception&)
		{
			conn->deleteTransaction(ext_tran);
			throw;
		}
	}

	return ext_tran;
}

void Transaction::detachFromJrdTran()
{
	if (m_scope != traCommon)
		return;

	fb_assert(m_jrdTran);
	if (!m_jrdTran)
		return;

	Transaction** tran_ptr = &m_jrdTran->tra_ext_common;
	for (; *tran_ptr; tran_ptr = &(*tran_ptr)->m_nextTran)
	{
		if (*tran_ptr == this)
		{
			*tran_ptr = this->m_nextTran;
			this->m_nextTran = NULL;
			return;
		}
	}

	fb_assert(false);
}

void Transaction::jrdTransactionEnd(thread_db* tdbb, jrd_tra* transaction,
		bool commit, bool retain, bool force)
{
	Transaction* tran = transaction->tra_ext_common;
	while (tran)
	{
		Transaction* next = tran->m_nextTran;
		try
		{
			if (commit)
				tran->commit(tdbb, retain);
			else
				tran->rollback(tdbb, retain);
		}
		catch (const Exception&)
		{
			if (!force || commit)
				throw;

			// ignore rollback error
			fb_utils::init_status(tdbb->tdbb_status_vector);
			tran->detachFromJrdTran();
		}
		tran = next;
	}
}

// Statement

Statement::Statement(Connection& conn) :
	PermanentStorage(conn.getProvider()->getPool()),
	m_provider(*conn.getProvider()),
	m_connection(conn),
	m_transaction(NULL),
	m_nextFree(NULL),
	m_boundReq(NULL),
	m_ReqImpure(NULL),
	m_nextInReq(NULL),
	m_prevInReq(NULL),
	m_sql(getPool()),
	m_singleton(false),
	m_active(false),
	m_error(false),
	m_allocated(false),
	m_stmt_selectable(false),
	m_inputs(0),
	m_outputs(0),
	m_callerPrivileges(false),
	m_preparedByReq(NULL),
	m_sqlParamNames(getPool()),
	m_sqlParamsMap(getPool()),
	m_in_buffer(getPool()),
	m_out_buffer(getPool()),
	m_inDescs(getPool()),
	m_outDescs(getPool())
{
}

Statement::~Statement()
{
	clearNames();
}

void Statement::deleteStatement(Jrd::thread_db* tdbb, Statement* stmt)
{
	stmt->deallocate(tdbb);
	delete stmt;
}

void Statement::prepare(thread_db* tdbb, Transaction* tran, const string& sql, bool named)
{
	fb_assert(!m_active);

	// already prepared the same non-empty statement
	if (isAllocated() && (m_sql == sql) && (m_sql != "") &&
		m_preparedByReq == (m_callerPrivileges ? tdbb->getRequest() : NULL))
	{
		return;
	}

	m_error = false;
	m_transaction = tran;
	m_sql = "";
	m_preparedByReq = NULL;

	m_in_buffer.clear();
	m_out_buffer.clear();
	m_inDescs.clear();
	m_outDescs.clear();
	clearNames();

	string sql2(getPool());
	const string* readySql = &sql;

	if (named && !(m_provider.getFlags() & prvNamedParams))
	{
		preprocess(sql, sql2);
		readySql = &sql2;
	}

	doPrepare(tdbb, *readySql);

	m_sql = sql;
	m_sql.trim();
	m_preparedByReq = m_callerPrivileges ? tdbb->getRequest() : NULL;
}

void Statement::execute(thread_db* tdbb, Transaction* tran, int in_count,
	const string* const* in_names, jrd_nod** in_params, int out_count, jrd_nod** out_params)
{
	fb_assert(isAllocated() && !m_stmt_selectable);
	fb_assert(!m_error);
	fb_assert(!m_active);

	m_transaction = tran;
	setInParams(tdbb, in_count, in_names, in_params);
	doExecute(tdbb);
	getOutParams(tdbb, out_count, out_params);
}

void Statement::open(thread_db* tdbb, Transaction* tran, int in_count,
	const string* const* in_names, jrd_nod** in_params, bool singleton)
{
	fb_assert(isAllocated() && m_stmt_selectable);
	fb_assert(!m_error);
	fb_assert(!m_active);

	m_singleton = singleton;
	m_transaction = tran;

	setInParams(tdbb, in_count, in_names, in_params);
	doOpen(tdbb);
	m_active = true;
}

bool Statement::fetch(thread_db* tdbb, int out_count, jrd_nod** out_params)
{
	fb_assert(isAllocated() && m_stmt_selectable);
	fb_assert(!m_error);
	fb_assert(m_active);

	if (!doFetch(tdbb))
		return false;

	getOutParams(tdbb, out_count, out_params);

	if (m_singleton)
	{
		if (doFetch(tdbb))
		{
			ISC_STATUS_ARRAY status;
			Arg::Gds(isc_sing_select_err).copyTo(status);
			raise(status, tdbb, "isc_dsql_fetch");
		}
		return false;
	}

	return true;
}

void Statement::close(thread_db* tdbb)
{
	// we must stuff exception if and only if this is the first time it occurs
	// once we stuff exception we must punt

	const bool wasError = m_error;
	bool doPunt = false;

	if (isAllocated() && m_active)
	{
		fb_assert(isAllocated() && m_stmt_selectable);
		try {
			doClose(tdbb, false);
		}
		catch (const Exception& ex)
		{
			if (!doPunt && !wasError)
			{
				doPunt = true;
				stuff_exception(tdbb->tdbb_status_vector, ex);
			}
		}
		m_active = false;
	}

	if (m_boundReq) {
		unBindFromRequest();
	}

	if (m_transaction && m_transaction->getScope() == traAutonomous)
	{
		bool commitFailed = false;
		if (!m_error)
		{
			try {
				m_transaction->commit(tdbb, false);
			}
			catch (const Exception& ex)
			{
				commitFailed = true;
				if (!doPunt && !wasError)
				{
					doPunt = true;
					stuff_exception(tdbb->tdbb_status_vector, ex);
				}
			}
		}

		if (m_error || commitFailed)
		{
			try {
				m_transaction->rollback(tdbb, false);
			}
			catch (const Exception& ex)
			{
				if (!doPunt && !wasError)
				{
					doPunt = true;
					stuff_exception(tdbb->tdbb_status_vector, ex);
				}
			}
		}
	}

	m_error = false;
	m_transaction = NULL;
	m_connection.releaseStatement(tdbb, this);

	if (doPunt) {
		ERR_punt();
	}
}

void Statement::deallocate(thread_db* tdbb)
{
	if (isAllocated())
	{
		try {
			doClose(tdbb, true);
		}
		catch (const Exception&) {
			// ignore
			fb_utils::init_status(tdbb->tdbb_status_vector);
		}
	}
	fb_assert(!isAllocated());
}


enum TokenType {ttNone, ttWhite, ttComment, ttBrokenComment, ttString, ttParamMark, ttIdent, ttOther};

static TokenType getToken(const char** begin, const char* end)
{
	TokenType ret = ttNone;
	const char* p = *begin;

	char c = *p++;
	switch (c)
	{
	case ':':
	case '?':
		ret = ttParamMark;
	break;

	case '\'':
	case '"':
		while (p < end)
		{
			if (*p++ == c)
			{
				ret = ttString;
				break;
			}
		}
		break;

	case '/':
		if (p < end && *p == '*')
		{
			ret = ttBrokenComment;
			p++;
			while (p < end)
			{
				if (*p++ == '*' && p < end && *p == '/')
				{
					p++;
					ret = ttComment;
					break;
				}
			}
		}
		else {
			ret = ttOther;
		}
		break;

	case '-':
		if (p < end && *p == '-')
		{
			while (p < end)
			{
				if (*p++ == '\n')
				{
					p--;
					ret = ttComment;
					break;
				}
			}
		}
		else {
			ret = ttOther;
		}
		break;

	default:
		if (classes(c) & CHR_DIGIT)
		{
			while (p < end && classes(*p) & CHR_DIGIT)
				p++;
			ret = ttOther;
		}
		else if (classes(c) & CHR_IDENT)
		{
			while (p < end && classes(*p) & CHR_IDENT)
				p++;
			ret = ttIdent;
		}
		else if (classes(c) & CHR_WHITE)
		{
			while (p < end && (classes(*p) & CHR_WHITE))
				p++;
			ret = ttWhite;
		}
		else
		{
			while (p < end && !( classes(*p) & (CHR_DIGIT | CHR_IDENT | CHR_WHITE) ) &&
				   (*p != '/') && (*p != '-') && (*p != ':') && (*p != '?') &&
				   (*p != '\'') && (*p != '"') )
			{
				p++;
			}
			ret = ttOther;
		}
	}

	*begin = p;
	return ret;
}

void Statement::preprocess(const string& sql, string& ret)
{
	bool passAsIs = true, execBlock = false;
	const char* p = sql.begin(), *end = sql.end();
	const char* start = p;
	TokenType tok = getToken(&p, end);

	const char* i = start;
	while (p < end && (tok == ttComment || tok == ttWhite))
	{
		i = p;
		tok = getToken(&p, end);
	}

	if (p >= end || tok != ttIdent)
	{
		// Execute statement preprocess SQL error
		// Statement expected
		ERR_post(Arg::Gds(isc_eds_preprocess) <<
				 Arg::Gds(isc_eds_stmt_expected));
	}

	start = i; // skip leading comments ??
	string ident(i, p - i);
	ident.upper();

	if (ident == "EXECUTE")
	{
		const char* i2 = p;
		tok = getToken(&p, end);
		while (p < end && (tok == ttComment || tok == ttWhite))
		{
			i2 = p;
			tok = getToken(&p, end);
		}
		if (p >= end || tok != ttIdent)
		{
			// Execute statement preprocess SQL error
			// Statement expected
			ERR_post(Arg::Gds(isc_eds_preprocess) <<
					 Arg::Gds(isc_eds_stmt_expected));
		}
		string ident2(i2, p - i2);
		ident2.upper();

		execBlock = (ident2 == "BLOCK");
		passAsIs = false;
	}
	else {
		passAsIs = !(ident == "INSERT" || ident == "UPDATE" ||  ident == "DELETE" ||
			ident == "MERGE" || ident == "SELECT" || ident == "WITH");
	}

	if (passAsIs)
	{
		ret = sql;
		return;
	}

	ret += string(start, p - start);

	while (p < end)
	{
		start = p;
		tok = getToken(&p, end);
		switch (tok)
		{
		case ttParamMark:
			tok = getToken(&p, end);
			if (tok == ttIdent /*|| tok == ttString*/)
			{
				// hvlad: TODO check quoted param names
				ident.assign(start + 1, p - start - 1);
				if (tok == ttIdent)
					ident.upper();

				size_t n = 0;
				for (; n < m_sqlParamNames.getCount(); n++)
				{
					if ((*m_sqlParamNames[n]) == ident)
						break;
				}

				if (n >= m_sqlParamNames.getCount())
				{
					n = m_sqlParamNames.getCount();
					m_sqlParamNames.add(FB_NEW(getPool()) string(getPool(), ident));
				}
				m_sqlParamsMap.add(m_sqlParamNames[n]);
			}
			else
			{
				// Execute statement preprocess SQL error
				// Parameter name expected
				ERR_post(Arg::Gds(isc_eds_preprocess) <<
						 Arg::Gds(isc_eds_prm_name_expected));
			}
			ret += '?';
			break;

		case ttIdent:
			if (execBlock)
			{
				ident.assign(start, p - start);
				ident.upper();
				if (ident == "AS")
				{
					ret += string(start, end - start);
					return;
				}
			}
			// fall thru

		case ttWhite:
		case ttComment:
		case ttString:
		case ttOther:
			ret += string(start, p - start);
			break;

		case ttBrokenComment:
			{
				// Execute statement preprocess SQL error
				// Unclosed comment found near ''@1''
				string s(start, MIN(16, end - start));
				ERR_post(Arg::Gds(isc_eds_preprocess) <<
						 Arg::Gds(isc_eds_unclosed_comment) << Arg::Str(s));
			}
			break;


		case ttNone:
			// Execute statement preprocess SQL error
			ERR_post(Arg::Gds(isc_eds_preprocess));
			break;
		}
	}
	return;
}

void Statement::setInParams(thread_db* tdbb, int count, const string* const* names, jrd_nod** params)
{
	m_error = (names && ((int) m_sqlParamNames.getCount() != count || !count)) ||
		(!names && m_sqlParamNames.getCount());

	if (m_error)
	{
		// Input parameters mismatch
		status_exception::raise(Arg::Gds(isc_eds_input_prm_mismatch));
	}

	if (m_sqlParamNames.getCount())
	{
		const int sqlCount = m_sqlParamsMap.getCount();
		Array<jrd_nod*> sqlParamsArray(getPool(), 16);
		jrd_nod** sqlParams = sqlParamsArray.getBuffer(sqlCount);

		for (int sqlNum = 0; sqlNum < sqlCount; sqlNum++)
		{
			const string* sqlName = m_sqlParamsMap[sqlNum];

			int num = 0;
			for (; num < count; num++)
			{
				if (*names[num] == *sqlName)
					break;
			}

			if (num == count)
			{
				// Input parameter ''@1'' have no value set
				status_exception::raise(Arg::Gds(isc_eds_input_prm_not_set) << Arg::Str(*sqlName));
			}

			sqlParams[sqlNum] = params[num];
		}

		doSetInParams(tdbb, sqlCount, m_sqlParamsMap.begin(), sqlParams);
	}
	else
	{
		doSetInParams(tdbb, count, names, params);
	}
}

void Statement::doSetInParams(thread_db* tdbb, int count, const string* const* /*names*/, jrd_nod** params)
{
	if (count != getInputs())
	{
		m_error = true;
		// Input parameters mismatch
		status_exception::raise(Arg::Gds(isc_eds_input_prm_mismatch));
	}

	if (!count)
		return;

	jrd_nod** jrdVar = params;
	GenericMap<Pair<NonPooled<jrd_nod*, dsc*> > > paramDescs(getPool());

	const jrd_req* request = tdbb->getRequest();

	for (int i = 0; i < count; i++, jrdVar++)
	{
		dsc* src = NULL;
		dsc& dst = m_inDescs[i * 2];
		dsc& null = m_inDescs[i * 2 + 1];

		if (!paramDescs.get(*jrdVar, src))
		{
			src = EVL_expr(tdbb, *jrdVar);
			paramDescs.put(*jrdVar, src);

			if (src)
			{
				if (request->req_flags & req_null)
					src->setNull();
				else
					src->dsc_flags &= ~DSC_null;
			}
		}

		const bool srcNull = !src || src->isNull();
		*((SSHORT*) null.dsc_address) = (srcNull ? -1 : 0);

		if (srcNull) {
			memset(dst.dsc_address, 0, dst.dsc_length);
		}
		else if (!dst.isNull())
		{
			if (dst.isBlob())
			{
				dsc srcBlob;
				srcBlob.clear();
				ISC_QUAD srcBlobID;

				if (src->isBlob())
				{
					srcBlob.makeBlob(src->getBlobSubType(), src->getTextType(), &srcBlobID);
					memmove(srcBlob.dsc_address, src->dsc_address, src->dsc_length);
				}
				else
				{
					srcBlob.makeBlob(dst.getBlobSubType(), dst.getTextType(), &srcBlobID);
					MOV_move(tdbb, src, &srcBlob);
				}

				putExtBlob(tdbb, srcBlob, dst);
			}
			else
				MOV_move(tdbb, src, &dst);
		}
	}
}


// m_outDescs -> jrd_nod
void Statement::getOutParams(thread_db* tdbb, int count, jrd_nod** params)
{
	if (count != getOutputs())
	{
		m_error = true;
		// Output parameters mismatch
		status_exception::raise(Arg::Gds(isc_eds_output_prm_mismatch));
	}

	if (!count)
		return;

	jrd_nod** jrdVar = params;
	for (int i = 0; i < count; i++, jrdVar++)
	{
/*
		dsc* d = EVL_assign_to(tdbb, *jrdVar);
		if (d->dsc_dtype >= FB_NELEM(sqlType) || sqlType[d->dsc_dtype] < 0)
		{
			m_error = true;
			status_exception::raise(
				Arg::Gds(isc_exec_sql_invalid_var) << Arg::Num(i + 1) << Arg::Str(m_sql.substr(0, 31)));
		}
*/

		// build the src descriptor
		dsc& src = m_outDescs[i * 2];
		const dsc& null = m_outDescs[i * 2 + 1];
		dsc* local = &src;
		dsc localDsc;
		bid localBlobID;

		const bool srcNull = (*(SSHORT*) null.dsc_address) == -1;
		if (src.isBlob() && !srcNull)
		{
			localDsc = src;
			localDsc.dsc_address = (UCHAR*) &localBlobID;
			getExtBlob(tdbb, src, localDsc);
			local = &localDsc;
		}

		// and assign to the target
		EXE_assignment(tdbb, *jrdVar, local, srcNull, NULL, NULL);
	}
}

// read external blob (src), store it as temporary local blob and put local blob_id into dst
void Statement::getExtBlob(thread_db* tdbb, const dsc& src, dsc& dst)
{
	blb* destBlob = NULL;
	AutoPtr<Blob> extBlob(m_connection.createBlob());
	try
	{
		extBlob->open(tdbb, *m_transaction, src, NULL);

		jrd_req* request = tdbb->getRequest();
		const UCHAR bpb[] = {isc_bpb_version1, isc_bpb_storage, isc_bpb_storage_temp};
		bid* localBlobID = (bid*) dst.dsc_address;
		destBlob = BLB_create2(tdbb, request->req_transaction, localBlobID, sizeof(bpb), bpb);

		// hvlad ?
		destBlob->blb_sub_type = src.getBlobSubType();
		destBlob->blb_charset = src.getCharSet();

		Firebird::Array<UCHAR> buffer;
		const int bufSize = 32 * 1024 - 2/*input->blb_max_segment*/;
		UCHAR* buff = buffer.getBuffer(bufSize);

		while (true)
		{
			const USHORT length = extBlob->read(tdbb, buff, bufSize);
			if (!length)
				break;

			BLB_put_segment(tdbb, destBlob, buff, length);
		}

		extBlob->close(tdbb);
		BLB_close(tdbb, destBlob);
	}
	catch (const Exception&)
	{
		extBlob->close(tdbb);
		if (destBlob) {
			BLB_cancel(tdbb, destBlob);
		}
		throw;
	}
}

// read local blob, store it as external blob and put external blob_id in dst
void Statement::putExtBlob(thread_db* tdbb, dsc& src, dsc& dst)
{
	blb* srcBlob = NULL;
	AutoPtr<Blob> extBlob(m_connection.createBlob());
	try
	{
		extBlob->create(tdbb, *m_transaction, dst, NULL);

		jrd_req* request = tdbb->getRequest();
		bid* srcBid = (bid*) src.dsc_address;

		UCharBuffer bpb;
		BLB_gen_bpb_from_descs(&src, &dst, bpb);
		srcBlob = BLB_open2(tdbb, request->req_transaction, srcBid, bpb.getCount(), bpb.begin());

		Firebird::HalfStaticArray<UCHAR, 2048> buffer;
		const int bufSize = srcBlob->blb_max_segment;
		UCHAR* buff = buffer.getBuffer(bufSize);

		while (true)
		{
			USHORT length = BLB_get_segment(tdbb, srcBlob, buff, srcBlob->blb_max_segment);
			if (srcBlob->blb_flags & BLB_eof) {
				break;
			}

			extBlob->write(tdbb, buff, length);
		}

		BLB_close(tdbb, srcBlob);
		extBlob->close(tdbb);
	}
	catch (const Exception&)
	{
		extBlob->cancel(tdbb);
		if (srcBlob) {
			BLB_close(tdbb, srcBlob);
		}
		throw;
	}
}

void Statement::clearNames()
{
	string** s = m_sqlParamNames.begin(), **end = m_sqlParamNames.end();
	for (; s < end; s++)
	{
		delete *s;
		*s = NULL;
	}

	m_sqlParamNames.clear();
	m_sqlParamsMap.clear();
}


void Statement::raise(ISC_STATUS* status, thread_db* tdbb, const char* sWhere,
		const string* sQuery)
{
	m_error = true;

	if (!m_connection.getWrapErrors())
	{
		ERR_post(Arg::StatusVector(status));
	}

	string rem_err;
	if (status)
	{
		m_provider.getRemoteError(status, rem_err);

		if (status == tdbb->tdbb_status_vector)
		{
			fb_utils::init_status(status);
		}
	}

	// Execute statement error at @1 :\n@2Statement : @3\nData source : @4
 	ERR_post(Arg::Gds(isc_eds_statement) << Arg::Str(sWhere) <<
											Arg::Str(rem_err) <<
 											Arg::Str(sQuery ? sQuery->substr(0, 255) : m_sql.substr(0, 255)) <<
											Arg::Str(m_connection.getDataSourceName()));
}

void Statement::bindToRequest(jrd_req* request, Statement** impure)
{
	fb_assert(!m_boundReq);
	fb_assert(!m_prevInReq);
	fb_assert(!m_nextInReq);

	if (request->req_ext_stmt)
	{
		this->m_nextInReq = request->req_ext_stmt;
		request->req_ext_stmt->m_prevInReq = this;
	}

	request->req_ext_stmt = this;
	m_boundReq = request;
	m_ReqImpure = impure;
	*m_ReqImpure = this;
}

void Statement::unBindFromRequest()
{
	fb_assert(m_boundReq);
	fb_assert(*m_ReqImpure == this);

	if (m_boundReq->req_ext_stmt == this)
		m_boundReq->req_ext_stmt = m_nextInReq;

	if (m_nextInReq)
		m_nextInReq->m_prevInReq = this->m_prevInReq;

	if (m_prevInReq)
		m_prevInReq->m_nextInReq = this->m_nextInReq;

	*m_ReqImpure = NULL;
	m_ReqImpure = NULL;
	m_boundReq = NULL;
	m_prevInReq = m_nextInReq = NULL;
}


//  EngineCallbackGuard

void EngineCallbackGuard::init(thread_db* tdbb, Connection& conn)
{
	m_tdbb = tdbb;
	m_mutex = conn.isConnected() ? &conn.m_mutex : &conn.m_provider.m_mutex;
	m_saveConnection = NULL;

	if (m_tdbb)
	{
		jrd_tra *transaction = m_tdbb->getTransaction();
		if (transaction) 
		{
			if (transaction->tra_callback_count >= MAX_CALLBACKS)
				ERR_post(Arg::Gds(isc_exec_sql_max_call_exceeded));

			transaction->tra_callback_count++;
		}

		Attachment *attachment = m_tdbb->getAttachment();
		if (attachment)
		{
			m_saveConnection = attachment->att_ext_connection;
			attachment->att_ext_connection = &conn;
		}

		m_tdbb->getDatabase()->dbb_sync->unlock();
	}

	if (m_mutex) {
		m_mutex->enter();
	}
}

EngineCallbackGuard::~EngineCallbackGuard()
{
	if (m_mutex) {
		m_mutex->leave();
	}

	if (m_tdbb)
	{
		m_tdbb->getDatabase()->dbb_sync->lock();

		jrd_tra *transaction = m_tdbb->getTransaction();
		if (transaction) {
			transaction->tra_callback_count--;
		}

		Attachment *attachment = m_tdbb->getAttachment();
		if (attachment) {
			attachment->att_ext_connection = m_saveConnection;
		}
	}
}

} // namespace EDS
