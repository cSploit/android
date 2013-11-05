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

#include "firebird.h"
#include "fb_types.h"
#include "../common.h"
#include "../../include/fb_blk.h"
#include "fb_exception.h"
#include "iberror.h"

#include "../align.h"
#include "../dsc.h"
#include "../exe.h"
#include "IscDS.h"
#include "../tra.h"

#include "../blb_proto.h"
#include "../evl_proto.h"
#include "../exe_proto.h"
#include "../intl_proto.h"
#include "../mov_proto.h"


using namespace Jrd;
using namespace Firebird;

namespace EDS
{

const char* FIREBIRD_PROVIDER_NAME = "Firebird";

class RegisterFBProvider
{
public:
	RegisterFBProvider()
	{
		Provider* provider = new FBProvider(FIREBIRD_PROVIDER_NAME);
		Manager::addProvider(provider);
	}
};

static RegisterFBProvider reg;

static bool isConnectionBrokenError(ISC_STATUS status);
static void parseSQLDA(XSQLDA* xsqlda, UCharBuffer& buff, Firebird::Array<dsc>& descs);
static UCHAR sqlTypeToDscType(SSHORT sqlType);

// 	IscProvider

void IscProvider::getRemoteError(ISC_STATUS* status, string& err) const
{
	err = "";

	// We can't use safe fb_interpret here as we have no idea what implementation
	// of ISC API is used by current provider. We can test for existence of
	// fb_interpret and use it if present, but i don't want to complicate code.
	// So, buffer should be big enough to please old isc_interprete.
	// Probably in next version we should use fb_interpret only.

	char buff[1024];
	ISC_STATUS* p = status;
	const ISC_STATUS* const end = status + ISC_STATUS_LENGTH;

	while (p < end)
	{
		const ISC_STATUS code = *p ? p[1] : 0;
		if (!m_api.isc_interprete(buff, &p))
			break;

		string rem_err;
		rem_err.printf("%lu : %s\n", code, buff);
		err += rem_err;
	}
}

Connection* IscProvider::doCreateConnection()
{
	return new IscConnection(*this);
}

// IscConnection

IscConnection::IscConnection(IscProvider& prov) :
	Connection(prov),
	m_iscProvider(prov),
	m_handle(0)
{
}

IscConnection::~IscConnection()
{
}

void IscConnection::attach(thread_db* tdbb, const string& dbName, const string& user,
	const string& pwd, const string& role)
{
	m_dbName = dbName;
	generateDPB(tdbb, m_dpb, user, pwd, role);

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, *this);
		m_iscProvider.isc_attach_database(status, m_dbName.length(), m_dbName.c_str(),
			&m_handle, m_dpb.getBufferLength(),
			reinterpret_cast<const char*>(m_dpb.getBuffer()));
	}
	if (status[1]) {
		raise(status, tdbb, "attach");
	}

	char buff[16];
	{
		EngineCallbackGuard guard(tdbb, *this);

		const char info[] = {isc_info_db_sql_dialect, isc_info_end};
		m_iscProvider.isc_database_info(status, &m_handle, sizeof(info), info, sizeof(buff), buff);
	}
	if (status[1]) {
		raise(status, tdbb, "isc_database_info");
	}

	const char* p = buff, *end = buff + sizeof(buff);
	while (p < end)
	{
		const UCHAR item = *p++;
		const USHORT len = m_iscProvider.isc_vax_integer(p, sizeof(USHORT));
		p += sizeof(USHORT);

		switch (item)
		{
			case isc_info_db_sql_dialect:
				m_sqlDialect = m_iscProvider.isc_vax_integer(p, len);
				break;

			case isc_info_error:
				if (*p == isc_info_db_sql_dialect)
				{
					const ULONG err = m_iscProvider.isc_vax_integer(p + 1, len - 1);
					if (err == isc_infunk)
					{
						// Remote server don't understand isc_info_db_sql_dialect.
						// Consider it as pre-IB6 server and use SQL dialect 1 to work with it.
						m_sqlDialect = 1;	
						break;
					}
				}
			// fall thru

			case isc_info_truncated:
				ERR_post(Arg::Gds(isc_random) << Arg::Str("Unexpected error in isc_database_info"));

			case isc_info_end:
				p = end;
				break;
		}
		p += len;
	}
}

void IscConnection::doDetach(thread_db* tdbb)
{
	ISC_STATUS_ARRAY status = {0};
	if (m_handle)
	{
		EngineCallbackGuard guard(tdbb, *this);

		FB_API_HANDLE h = m_handle;
		m_handle = 0;
		m_iscProvider.isc_detach_database(status, &h);
		m_handle = h;
	}

	if (status[1] && !isConnectionBrokenError(status[1]))
		raise(status, tdbb, "detach");
}

bool IscConnection::cancelExecution(thread_db* /*tdbb*/)
{
	ISC_STATUS_ARRAY status = {0, 0, 0};
	if (m_handle)
	{
		m_iscProvider.fb_cancel_operation(status, &m_handle, fb_cancel_raise);

		if (m_handle && status[1] == isc_wish_list)
		{
			fb_utils::init_status(status);
			m_iscProvider.fb_cancel_operation(status, &m_handle, fb_cancel_abort);
		}
	}
	return (status[1] == 0);
}

// this ISC connection instance is available for the current execution context if it
// a) has no active statements or supports many active statements
//    and
// b) has no active transactions or has active transaction of given
//	  TraScope bound to current jrd transaction or supports many active
//    transactions
bool IscConnection::isAvailable(thread_db* tdbb, TraScope traScope) const
{
	const int flags = m_provider.getFlags();
	if (m_used_stmts && !(flags & prvMultyStmts))
		return false;

	if (m_transactions.getCount() && !(flags & prvMultyTrans) && !findTransaction(tdbb, traScope))
	{
		return false;
	}

	return true;
}

Blob* IscConnection::createBlob()
{
	return new IscBlob(*this);
}

Transaction* IscConnection::doCreateTransaction()
{
	return new IscTransaction(*this);
}

Statement* IscConnection::doCreateStatement()
{
	return new IscStatement(*this);
}

// IscTransaction

void IscTransaction::doStart(ISC_STATUS* status, thread_db* tdbb, Firebird::ClumpletWriter& tpb)
{
	fb_assert(!m_handle);
	FB_API_HANDLE& db_handle = m_iscConnection.getAPIHandle();

	EngineCallbackGuard guard(tdbb, *this);
	m_iscProvider.isc_start_transaction(status, &m_handle, 1, &db_handle,
		tpb.getBufferLength(), tpb.getBuffer());
}

void IscTransaction::doPrepare(ISC_STATUS* /*status*/, thread_db* /*tdbb*/, int /*info_len*/, const char* /*info*/)
{
}

void IscTransaction::doCommit(ISC_STATUS* status, thread_db* tdbb, bool retain)
{
	EngineCallbackGuard guard(tdbb, *this);
	if (retain)
		m_iscProvider.isc_commit_retaining(status, &m_handle);
	else
		m_iscProvider.isc_commit_transaction(status, &m_handle);

	fb_assert(retain && m_handle || !retain && !m_handle || status[1] && m_handle);
}

void IscTransaction::doRollback(ISC_STATUS* status, thread_db* tdbb, bool retain)
{
	EngineCallbackGuard guard(tdbb, *this);
	if (retain)
		m_iscProvider.isc_rollback_retaining(status, &m_handle);
	else
		m_iscProvider.isc_rollback_transaction(status, &m_handle);

	if (status[1] && isConnectionBrokenError(status[1]) && !retain)
	{
		m_handle = 0;
		fb_utils::init_status(status);
	}

	fb_assert(retain && m_handle || !retain && !m_handle || status[1] && m_handle);
}


// IscStatement

IscStatement::IscStatement(IscConnection& conn) :
	Statement(conn),
	m_iscProvider(*(IscProvider*) conn.getProvider()),
	m_iscConnection(conn),
	m_handle(0),
	m_in_xsqlda(NULL),
	m_out_xsqlda(NULL)
{
}

IscStatement::~IscStatement()
{
	delete[] (char*) m_in_xsqlda;
	delete[] (char*) m_out_xsqlda;
}

void IscStatement::doPrepare(thread_db* tdbb, const string& sql)
{
	FB_API_HANDLE& h_conn = m_iscConnection.getAPIHandle();
	FB_API_HANDLE& h_tran = getIscTransaction()->getAPIHandle();

	ISC_STATUS_ARRAY status = {0};

	// prepare and get output parameters
	if (!m_out_xsqlda)
	{
		m_out_xsqlda = (XSQLDA*) FB_NEW (getPool()) char [XSQLDA_LENGTH(1)];
		m_out_xsqlda->sqln = 1;
		m_out_xsqlda->version = 1;
	}

	const char* sWhereError = NULL;
	{
		EngineCallbackGuard guard(tdbb, *this);

		if (!m_handle)
		{
			fb_assert(!m_allocated);
			if (m_iscProvider.isc_dsql_allocate_statement(status, &h_conn, &m_handle)) {
				sWhereError = "isc_dsql_allocate_statement";
			}
			m_allocated = (m_handle != 0);
		}

		if (!sWhereError) {
			if (m_iscProvider.isc_dsql_prepare(status, &h_tran, &m_handle, sql.length(),
				sql.c_str(), m_connection.getSqlDialect(), m_out_xsqlda))
			{
				sWhereError = "isc_dsql_prepare";
			}
		}
	}
	if (sWhereError) {
		raise(status, tdbb, sWhereError, &sql);
	}

	// adjust output parameters
	if (m_out_xsqlda->sqld > m_out_xsqlda->sqln)
	{
		const int n = m_out_xsqlda->sqld;
		delete[] (char*) m_out_xsqlda;

		m_out_xsqlda = (XSQLDA*) FB_NEW (getPool()) char [XSQLDA_LENGTH(n)];
		m_out_xsqlda->sqln = n;
		m_out_xsqlda->version = 1;

		EngineCallbackGuard guard(tdbb, *this);
		if (m_iscProvider.isc_dsql_describe(status, &m_handle, 1, m_out_xsqlda))
		{
			sWhereError = "isc_dsql_describe";
		}
	}
	if (sWhereError) {
		raise(status, tdbb, sWhereError, &sql);
	}

	parseSQLDA(m_out_xsqlda, m_out_buffer, m_outDescs);
	m_outputs = m_out_xsqlda ? m_out_xsqlda->sqld : 0;

	// get input parameters
	if (!m_in_xsqlda)
	{
		m_in_xsqlda = (XSQLDA*) FB_NEW (getPool()) char [XSQLDA_LENGTH(1)];
		m_in_xsqlda->sqln = 1;
		m_in_xsqlda->version = 1;
	}

	{
		EngineCallbackGuard guard(tdbb, *this);
		if (m_iscProvider.isc_dsql_describe_bind(status, &m_handle, 1, m_in_xsqlda))
		{
			sWhereError = "isc_dsql_describe_bind";
		}
	}
	if (sWhereError) {
		raise(status, tdbb, sWhereError, &sql);
	}

	// adjust input parameters
	if (m_in_xsqlda->sqld > m_in_xsqlda->sqln)
	{
		const int n = m_in_xsqlda->sqld;
		delete[] (char*) m_in_xsqlda;

		m_in_xsqlda = (XSQLDA*) FB_NEW (getPool()) char [XSQLDA_LENGTH(n)];
		m_in_xsqlda->sqln = n;
		m_in_xsqlda->version = 1;

		EngineCallbackGuard guard(tdbb, *this);
		if (m_iscProvider.isc_dsql_describe_bind(status, &m_handle, 1, m_in_xsqlda))
		{
			sWhereError = "isc_dsql_describe_bind";
		}
	}
	if (sWhereError) {
		raise(status, tdbb, sWhereError, &sql);
	}

	parseSQLDA(m_in_xsqlda, m_in_buffer, m_inDescs);
	m_inputs = m_in_xsqlda ? m_in_xsqlda->sqld : 0;

	// get statement type
	const char stmt_info[] = {isc_info_sql_stmt_type};
	char info_buff[16];
	{
		EngineCallbackGuard guard(tdbb, *this);
		if (m_iscProvider.isc_dsql_sql_info(status, &m_handle, sizeof(stmt_info), stmt_info,
			sizeof(info_buff), info_buff))
		{
			sWhereError = "isc_dsql_sql_info";
		}
	}
	if (sWhereError) {
		raise(status, tdbb, sWhereError, &sql);
	}

	if (info_buff[0] != stmt_info[0])
	{
		ERR_build_status(status, Arg::Gds(isc_random) << "Unknown statement type");

		sWhereError = "isc_dsql_sql_info";
		raise(status, tdbb, sWhereError, &sql);
	}

	{
		EngineCallbackGuard guard(tdbb, *this);
		const int len = m_iscProvider.isc_vax_integer(&info_buff[1], 2);
		const int stmt_type = m_iscProvider.isc_vax_integer(&info_buff[3], len);

		m_stmt_selectable = (stmt_type == isc_info_sql_stmt_select ||
			stmt_type == isc_info_sql_stmt_select_for_upd);

		if (stmt_type == isc_info_sql_stmt_start_trans ||
			stmt_type == isc_info_sql_stmt_commit ||
			stmt_type == isc_info_sql_stmt_rollback)
		{
			ERR_build_status(status, Arg::Gds(isc_eds_expl_tran_ctrl));

			sWhereError = "isc_dsql_prepare";
			raise(status, tdbb, sWhereError, &sql);
		}
	}
}

void IscStatement::doExecute(thread_db* tdbb)
{
	FB_API_HANDLE& h_tran = getIscTransaction()->getAPIHandle();

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, *this);
		m_iscProvider.isc_dsql_execute2(status, &h_tran, &m_handle, 1, m_in_xsqlda, m_out_xsqlda);
	}
	if (status[1]) {
		raise(status, tdbb, "isc_dsql_execute2");
	}
}

void IscStatement::doOpen(thread_db* tdbb)
{
	FB_API_HANDLE& h_tran = getIscTransaction()->getAPIHandle();
	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, *this);
		m_iscProvider.isc_dsql_execute(status, &h_tran, &m_handle, 1, m_in_xsqlda);
	}
	if (status[1]) {
		raise(status, tdbb, "isc_dsql_execute");
	}
}

bool IscStatement::doFetch(thread_db* tdbb)
{
	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, *this);
		const ISC_STATUS res = m_iscProvider.isc_dsql_fetch(status, &m_handle, 1, m_out_xsqlda);
		if (res == 100) {
			return false;
		}
	}
	if (status[1]) {
		raise(status, tdbb, "isc_dsql_fetch");
	}
	return true;
}

void IscStatement::doClose(thread_db* tdbb, bool drop)
{
	fb_assert(m_handle);
	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, *this);
		m_iscProvider.isc_dsql_free_statement(status, &m_handle, drop ? DSQL_drop : DSQL_close);
		m_allocated = (m_handle != 0);
	}
	if (status[1])
	{
		// we can do nothing else with this statement after this point
		m_allocated = m_handle = 0;
		raise(status, tdbb, "isc_dsql_free_statement");
	}
}

void IscStatement::doSetInParams(thread_db* tdbb, int count, const string* const* names, jrd_nod** params)
{
	Statement::doSetInParams(tdbb, count, names, params);

	if (names)
	{
		XSQLVAR* xVar = m_in_xsqlda->sqlvar;
		for (int i = 0; i < count; i++, xVar++)
		{
			const int max_len = sizeof(xVar->sqlname);
			const int len = MIN(names[i]->length(), max_len - 1);
			xVar->sqlname_length = len;
			strncpy(xVar->sqlname, names[i]->c_str(), len);
			xVar->sqlname[max_len-1] = 0;
		}
	}
}

//  IscBlob

IscBlob::IscBlob(IscConnection& conn) :
	Blob(conn),
	m_iscProvider(*(IscProvider*) conn.getProvider()),
	m_iscConnection(conn),
	m_handle(0)
{
	memset(&m_blob_id, 0, sizeof(m_blob_id));
}

IscBlob::~IscBlob()
{
	fb_assert(!m_handle);
}

void IscBlob::open(thread_db* tdbb, Transaction& tran, const dsc& desc, const UCharBuffer* bpb)
{
	fb_assert(!m_handle);
	fb_assert(sizeof(m_blob_id) == desc.dsc_length);

	FB_API_HANDLE& h_db = m_iscConnection.getAPIHandle();
	FB_API_HANDLE& h_tran = ((IscTransaction&) tran).getAPIHandle();

	memcpy(&m_blob_id, desc.dsc_address, sizeof(m_blob_id));

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);

		ISC_USHORT bpb_len = bpb ? bpb->getCount() : 0;
		const ISC_UCHAR* bpb_buff = bpb ? bpb->begin() : NULL;

		m_iscProvider.isc_open_blob2(status, &h_db, &h_tran, &m_handle, &m_blob_id,
			bpb_len, bpb_buff);
	}
	if (status[1]) {
		m_iscConnection.raise(status, tdbb, "isc_open_blob2");
	}
	fb_assert(m_handle);
}

void IscBlob::create(thread_db* tdbb, Transaction& tran, dsc& desc, const UCharBuffer* bpb)
{
	fb_assert(!m_handle);
	fb_assert(sizeof(m_blob_id) == desc.dsc_length);

	FB_API_HANDLE& h_db = m_iscConnection.getAPIHandle();
	FB_API_HANDLE& h_tran = ((IscTransaction&) tran).getAPIHandle();

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);

		ISC_USHORT bpb_len = bpb ? bpb->getCount() : 0;
		const char* bpb_buff = bpb ? reinterpret_cast<const char*>(bpb->begin()) : NULL;

		m_iscProvider.isc_create_blob2(status, &h_db, &h_tran, &m_handle, &m_blob_id,
			bpb_len, bpb_buff);
		memcpy(desc.dsc_address, &m_blob_id, sizeof(m_blob_id));
	}
	if (status[1]) {
		m_iscConnection.raise(status, tdbb, "isc_create_blob2");
	}
	fb_assert(m_handle);
}

USHORT IscBlob::read(thread_db* tdbb, UCHAR* buff, USHORT len)
{
	fb_assert(m_handle);

	USHORT result = 0;
	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);
		m_iscProvider.isc_get_segment(status, &m_handle, &result, len, reinterpret_cast<SCHAR*>(buff));
	}
	switch (status[1])
	{
	case isc_segstr_eof:
		fb_assert(result == 0);
		break;
	case isc_segment:
	case 0:
		break;
	default:
		m_iscConnection.raise(status, tdbb, "isc_get_segment");
	}

	return result;
}

void IscBlob::write(thread_db* tdbb, const UCHAR* buff, USHORT len)
{
	fb_assert(m_handle);

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);
		m_iscProvider.isc_put_segment(status, &m_handle, len, reinterpret_cast<const SCHAR*>(buff));
	}
	if (status[1]) {
		m_iscConnection.raise(status, tdbb, "isc_put_segment");
	}
}

void IscBlob::close(thread_db* tdbb)
{
	fb_assert(m_handle);
	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);
		m_iscProvider.isc_close_blob(status, &m_handle);
	}
	if (status[1]) {
		m_iscConnection.raise(status, tdbb, "isc_close_blob");
	}
	fb_assert(!m_handle);
}


void IscBlob::cancel(thread_db* tdbb)
{
	if (!m_handle)
		return;

	ISC_STATUS_ARRAY status = {0};
	{
		EngineCallbackGuard guard(tdbb, m_iscConnection);
		m_iscProvider.isc_cancel_blob(status, &m_handle);
	}
	if (status[1]) {
		m_iscConnection.raise(status, tdbb, "isc_close_blob");
	}
	fb_assert(!m_handle);
}


// IscProvider
// isc api

ISC_STATUS IscProvider::notImplemented(ISC_STATUS* status) const
{
	Arg::Gds(isc_unavailable).copyTo(status);

	return status[1];
}

ISC_STATUS ISC_EXPORT IscProvider::isc_attach_database(ISC_STATUS* user_status,
	short file_length, const char* file_name, isc_db_handle* public_handle,
	short dpb_length, const char* dpb)
{
	if (!m_api.isc_attach_database)
		return notImplemented(user_status);

	return (*m_api.isc_attach_database) (user_status, file_length, file_name,
			public_handle, dpb_length, dpb);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_gen_sdl(ISC_STATUS* user_status,
	const ISC_ARRAY_DESC*, short*, char*, short*)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_get_slice(ISC_STATUS* user_status,
									  isc_db_handle *,
									  isc_tr_handle *,
									  ISC_QUAD *,
									  const ISC_ARRAY_DESC*,
									  void *,
									  ISC_LONG *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_lookup_bounds(ISC_STATUS* user_status,
										  isc_db_handle *,
										  isc_tr_handle *,
										  const char*,
										  const char*,
										  ISC_ARRAY_DESC *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_lookup_desc(ISC_STATUS* user_status,
										isc_db_handle *,
										isc_tr_handle *,
										const char*,
										const char*,
										ISC_ARRAY_DESC *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_set_desc(ISC_STATUS* user_status,
									 const char*,
									 const char*,
									 const short*,
									 const short*,
									 const short*,
									 ISC_ARRAY_DESC *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_array_put_slice(ISC_STATUS* user_status,
									  isc_db_handle *,
									  isc_tr_handle *,
									  ISC_QUAD *,
									  const ISC_ARRAY_DESC*,
									  void *,
									  ISC_LONG *)
{
	return notImplemented(user_status);
}

void ISC_EXPORT IscProvider::isc_blob_default_desc(ISC_BLOB_DESC *,
								  const unsigned char*,
								  const unsigned char*)
{
	return;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_blob_gen_bpb(ISC_STATUS* user_status,
								   const ISC_BLOB_DESC*,
								   const ISC_BLOB_DESC*,
								   unsigned short,
								   unsigned char *,
								   unsigned short *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_blob_info(ISC_STATUS* user_status,
								isc_blob_handle* blob_handle,
								short item_length,
								const char* items,
								short buffer_length,
								char* buffer)
{
	if (!m_api.isc_blob_info)
		return notImplemented(user_status);

	return (*m_api.isc_blob_info) (user_status, blob_handle,
			item_length, items, buffer_length, buffer);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_blob_lookup_desc(ISC_STATUS* user_status,
									   isc_db_handle *,
									   isc_tr_handle *,
									   const unsigned char*,
									   const unsigned char*,
									   ISC_BLOB_DESC *,
									   unsigned char *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_blob_set_desc(ISC_STATUS* user_status,
									const unsigned char*,
									const unsigned char*,
									short,
									short,
									short,
									ISC_BLOB_DESC *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_cancel_blob(ISC_STATUS* user_status,
								  isc_blob_handle* blob_handle)
{
	if (!m_api.isc_cancel_blob)
		return notImplemented(user_status);

	return (*m_api.isc_cancel_blob) (user_status, blob_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_cancel_events(ISC_STATUS* user_status,
									isc_db_handle *,
									ISC_LONG *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_close_blob(ISC_STATUS* user_status,
								 isc_blob_handle* blob_handle)
{
	if (!m_api.isc_close_blob)
		return notImplemented(user_status);

	return (*m_api.isc_close_blob) (user_status, blob_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_commit_retaining(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle)
{
	if (!m_api.isc_commit_retaining)
		return notImplemented(user_status);

	return (*m_api.isc_commit_retaining) (user_status, tra_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_commit_transaction(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle)
{
	if (!m_api.isc_commit_transaction)
		return notImplemented(user_status);

	return (*m_api.isc_commit_transaction) (user_status, tra_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_create_blob(ISC_STATUS* user_status,
								  isc_db_handle* db_handle,
								  isc_tr_handle* tr_handle,
								  isc_blob_handle* blob_handle,
								  ISC_QUAD* blob_id)
{
	if (!m_api.isc_create_blob)
		return notImplemented(user_status);

	return (*m_api.isc_create_blob) (user_status, db_handle, tr_handle,
			blob_handle, blob_id);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_create_blob2(ISC_STATUS* user_status,
								  isc_db_handle* db_handle,
								  isc_tr_handle* tr_handle,
								  isc_blob_handle* blob_handle,
								  ISC_QUAD* blob_id,
								  short bpb_length,
								  const char* bpb)
{
	if (!m_api.isc_create_blob2)
		return notImplemented(user_status);

	return (*m_api.isc_create_blob2) (user_status, db_handle, tr_handle,
			blob_handle, blob_id, bpb_length, bpb);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_create_database(ISC_STATUS* user_status,
									  short,
									  const char*,
									  isc_db_handle *,
									  short,
									  const char*,
									  short)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_database_info(ISC_STATUS* user_status,
									isc_db_handle* db_handle,
									short info_len,
									const char* info,
									short res_len,
									char* res)
{
	if (!m_api.isc_database_info)
		return notImplemented(user_status);

	return (*m_api.isc_database_info) (user_status, db_handle,
			info_len, info, res_len, res);
}

void ISC_EXPORT IscProvider::isc_decode_date(const ISC_QUAD*,
							void *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_decode_sql_date(const ISC_DATE*,
								void *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_decode_sql_time(const ISC_TIME*,
								void *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_decode_timestamp(const ISC_TIMESTAMP*,
								 void *)
{
	return;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_detach_database(ISC_STATUS* user_status,
									  isc_db_handle* public_handle)
{
	if (!m_api.isc_detach_database)
		return notImplemented(user_status);

	return (*m_api.isc_detach_database) (user_status, public_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_drop_database(ISC_STATUS* user_status,
									isc_db_handle *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_allocate_statement(ISC_STATUS* user_status,
	isc_db_handle* db_handle, isc_stmt_handle* stmt_handle)
{
	if (!m_api.isc_dsql_allocate_statement)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_allocate_statement) (user_status, db_handle, stmt_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_alloc_statement2(ISC_STATUS* user_status,
	isc_db_handle* db_handle, isc_stmt_handle* stmt_handle)
{
	if (!m_api.isc_dsql_alloc_statement2)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_alloc_statement2) (user_status, db_handle, stmt_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_describe(ISC_STATUS* user_status,
	isc_stmt_handle* stmt_handle, unsigned short dialect, XSQLDA* sqlda)
{
	if (!m_api.isc_dsql_describe)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_describe) (user_status, stmt_handle, dialect, sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_describe_bind(ISC_STATUS* user_status,
	isc_stmt_handle* stmt_handle, unsigned short dialect, XSQLDA* sqlda)
{
	if (!m_api.isc_dsql_describe_bind)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_describe_bind) (user_status, stmt_handle, dialect, sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_exec_immed2(ISC_STATUS* user_status,
									   isc_db_handle *,
									   isc_tr_handle *,
									   unsigned short,
									   const char*,
									   unsigned short,
									   const XSQLDA *,
									   const XSQLDA *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_execute(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle, isc_stmt_handle* stmt_handle, unsigned short dialect,
	const XSQLDA* sqlda)
{
	if (!m_api.isc_dsql_execute)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_execute) (user_status, tra_handle, stmt_handle, dialect, sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_execute2(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle, isc_stmt_handle* stmt_handle, unsigned short dialect,
	const XSQLDA* in_sqlda, const XSQLDA* out_sqlda)
{
	if (!m_api.isc_dsql_execute2)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_execute2) (user_status, tra_handle, stmt_handle, dialect,
			in_sqlda, out_sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_execute_immediate(ISC_STATUS* user_status,
											 isc_db_handle *,
											 isc_tr_handle *,
											 unsigned short,
											 const char*,
											 unsigned short,
											 const XSQLDA *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_fetch(ISC_STATUS* user_status,
	isc_stmt_handle* stmt_handle, unsigned short da_version, const XSQLDA* sqlda)
{
	if (!m_api.isc_dsql_fetch)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_fetch) (user_status, stmt_handle, da_version, sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_finish(isc_db_handle *)
{
	return isc_unavailable;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_free_statement(ISC_STATUS* user_status,
	isc_stmt_handle* stmt_handle, unsigned short option)
{
	if (!m_api.isc_dsql_free_statement)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_free_statement) (user_status, stmt_handle, option);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_insert(ISC_STATUS* user_status,
								  isc_stmt_handle *,
								  unsigned short,
								  XSQLDA *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_prepare(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle, isc_stmt_handle* stmt_handle, unsigned short length,
	const char* str, unsigned short dialect, XSQLDA* sqlda)
{
	if (!m_api.isc_dsql_prepare)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_prepare) (user_status, tra_handle, stmt_handle,
				length, str, dialect, sqlda);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_set_cursor_name(ISC_STATUS* user_status,
										   isc_stmt_handle *,
										   const char*,
										   unsigned short)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_dsql_sql_info(ISC_STATUS* user_status,
	isc_stmt_handle* stmt_handle, short items_len, const char* items,
	short buffer_len, char* buffer)
{
	if (!m_api.isc_dsql_sql_info)
		return notImplemented(user_status);

	return (*m_api.isc_dsql_sql_info) (user_status, stmt_handle, items_len, items,
				buffer_len, buffer);
}

void ISC_EXPORT IscProvider::isc_encode_date(const void*,
							ISC_QUAD *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_encode_sql_date(const void*,
								ISC_DATE *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_encode_sql_time(const void*,
								ISC_TIME *)
{
	return;
}

void ISC_EXPORT IscProvider::isc_encode_timestamp(const void*,
								 ISC_TIMESTAMP *)
{
	return;
}

ISC_LONG ISC_EXPORT_VARARG IscProvider::isc_event_block(char * *,
									   char * *,
									   unsigned short, ...)
{
	return 0;
}

void ISC_EXPORT IscProvider::isc_event_counts(ISC_ULONG *,
							 short,
							 char *,
							 const char*)
{
	return;
}

/* 17 May 2001 - IscProvider::isc_expand_dpb is DEPRECATED */
void ISC_EXPORT_VARARG IscProvider::isc_expand_dpb(char * *,
								  short *, ...)
{
	return;
}

int ISC_EXPORT IscProvider::isc_modify_dpb(char * *,
						  short *,
						  unsigned short,
						  const char*,
						  short)
{
	return 0;
}

ISC_LONG ISC_EXPORT IscProvider::isc_free(char *)
{
	return 0;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_get_segment(ISC_STATUS* user_status,
								  isc_blob_handle* blob_handle,
								  unsigned short* length,
								  unsigned short buffer_length,
								  char* buffer)
{
	if (!m_api.isc_get_segment)
		return notImplemented(user_status);

	return (*m_api.isc_get_segment) (user_status, blob_handle, length,
			buffer_length, buffer);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_get_slice(ISC_STATUS* user_status,
								isc_db_handle *,
								isc_tr_handle *,
								ISC_QUAD *,
								short,
								const char*,
								short,
								const ISC_LONG*,
								ISC_LONG,
								void *,
								ISC_LONG *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_interprete(char *,
								 ISC_STATUS * *)
{
	return isc_unavailable;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_open_blob(ISC_STATUS* user_status,
								isc_db_handle* db_handle,
								isc_tr_handle* tr_handle,
								isc_blob_handle* blob_handle,
								ISC_QUAD* blob_id)
{
	if (!m_api.isc_open_blob)
		return notImplemented(user_status);

	return (*m_api.isc_open_blob) (user_status, db_handle, tr_handle,
			blob_handle, blob_id);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_open_blob2(ISC_STATUS* user_status,
								isc_db_handle* db_handle,
								isc_tr_handle* tr_handle,
								isc_blob_handle* blob_handle,
								ISC_QUAD* blob_id,
								ISC_USHORT bpb_length,
								const ISC_UCHAR* bpb)
{
	if (!m_api.isc_open_blob2)
		return notImplemented(user_status);

	return (*m_api.isc_open_blob2) (user_status, db_handle, tr_handle,
			blob_handle, blob_id, bpb_length, bpb);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_prepare_transaction2(ISC_STATUS* user_status,
										   isc_tr_handle *,
										   ISC_USHORT,
										   const ISC_UCHAR*)
{
	return notImplemented(user_status);
}

void ISC_EXPORT IscProvider::isc_print_sqlerror(ISC_SHORT,
							   const ISC_STATUS*)
{
	return;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_print_status(const ISC_STATUS*)
{
	return isc_unavailable;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_put_segment(ISC_STATUS* user_status,
								  isc_blob_handle* blob_handle,
								  unsigned short buffer_length,
								  const char* buffer)
{
	if (!m_api.isc_put_segment)
		return notImplemented(user_status);

	return (*m_api.isc_put_segment) (user_status, blob_handle,
			buffer_length, buffer);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_put_slice(ISC_STATUS* user_status,
								isc_db_handle *,
								isc_tr_handle *,
								ISC_QUAD *,
								short,
								const char*,
								short,
								const ISC_LONG*,
								ISC_LONG,
								void *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_que_events(ISC_STATUS* user_status,
								 isc_db_handle *,
								 ISC_LONG *,
								 ISC_USHORT,
								 const ISC_UCHAR*,
								 isc_callback,
								 void *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_rollback_retaining(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle)
{
	if (!m_api.isc_rollback_retaining)
		return notImplemented(user_status);

	return (*m_api.isc_rollback_retaining) (user_status, tra_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_rollback_transaction(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle)
{
	if (!m_api.isc_rollback_transaction)
		return notImplemented(user_status);

	return (*m_api.isc_rollback_transaction) (user_status, tra_handle);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_start_multiple(ISC_STATUS* user_status,
	isc_tr_handle* tra_handle, short count, void* vec)
{
	if (!m_api.isc_start_multiple)
		return notImplemented(user_status);

	return (*m_api.isc_start_multiple) (user_status, tra_handle, count, vec);
}


struct why_teb
{
	FB_API_HANDLE* teb_database;
	int teb_tpb_length;
	const UCHAR* teb_tpb;
};

ISC_STATUS ISC_EXPORT_VARARG IscProvider::isc_start_transaction(ISC_STATUS* user_status,
											   isc_tr_handle* tra_handle,
											   short count, ...)
{
	if (!m_api.isc_start_multiple)				// !!!
		return notImplemented(user_status);

	Firebird::HalfStaticArray<why_teb, 16> tebs;
	why_teb* teb = tebs.getBuffer(count);

	const why_teb* const end = teb + count;
	va_list ptr;
	va_start(ptr, count);

	for (why_teb* teb_iter = teb; teb_iter < end; teb_iter++)
	{
		teb_iter->teb_database = va_arg(ptr, FB_API_HANDLE*);
		teb_iter->teb_tpb_length = va_arg(ptr, int);
		teb_iter->teb_tpb = va_arg(ptr, UCHAR *);
	}
	va_end(ptr);

	return (*m_api.isc_start_multiple) (user_status, tra_handle, count, teb);
}

ISC_STATUS ISC_EXPORT_VARARG IscProvider::isc_reconnect_transaction(ISC_STATUS* user_status,
                                               isc_db_handle *,
                                               isc_tr_handle *,
                                               short,
                                               const char*)
{
	return notImplemented(user_status);
}

ISC_LONG ISC_EXPORT IscProvider::isc_sqlcode(const ISC_STATUS*)
{
	return isc_unavailable;
}

void ISC_EXPORT IscProvider::isc_sql_interprete(short,
							   char *,
							   short)
{
	return;
}

ISC_STATUS ISC_EXPORT IscProvider::isc_transaction_info(ISC_STATUS* user_status,
									   isc_tr_handle *,
									   short,
									   const char*,
									   short,
									   char *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_transact_request(ISC_STATUS* user_status,
									   isc_db_handle *,
									   isc_tr_handle *,
									   unsigned short,
									   char *,
									   unsigned short,
									   char *,
									   unsigned short,
									   char *)
{
	return notImplemented(user_status);
}

ISC_LONG ISC_EXPORT IscProvider::isc_vax_integer(const char* p, short len)
{
	return ::isc_vax_integer(p, len);
}

ISC_INT64 ISC_EXPORT IscProvider::isc_portable_integer(const unsigned char* p, short len)
{
	return ::isc_portable_integer(p, len);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_seek_blob(ISC_STATUS* user_status,
								isc_blob_handle *,
								short,
								ISC_LONG,
								ISC_LONG *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_service_attach(ISC_STATUS* user_status,
									 unsigned short,
									 const char*,
									 isc_svc_handle *,
									 unsigned short,
									 const char*)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_service_detach(ISC_STATUS* user_status,
									 isc_svc_handle *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_service_query(ISC_STATUS* user_status,
									isc_svc_handle *,
									isc_resv_handle *,
									unsigned short,
									const char*,
									unsigned short,
									const char*,
									unsigned short,
									char *)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::isc_service_start(ISC_STATUS* user_status,
									isc_svc_handle *,
									isc_resv_handle *,
									unsigned short,
									const char*)
{
	return notImplemented(user_status);
}

ISC_STATUS ISC_EXPORT IscProvider::fb_cancel_operation(ISC_STATUS* user_status,
										isc_db_handle* db_handle,
										USHORT option)
{
	if (m_api.fb_cancel_operation)
		return m_api.fb_cancel_operation(user_status, db_handle, option);
	else
		return notImplemented(user_status);
}

void IscProvider::loadAPI()
{
	ISC_STATUS_ARRAY status;
	notImplemented(status);
	status_exception::raise(status);
}


// FBProvider

#define PROTO(X) (prototype_##X*) &X

static FirebirdApiPointers isc_callbacks =
{
	PROTO(isc_attach_database),
	PROTO(isc_array_gen_sdl),
	PROTO(isc_array_get_slice),
	PROTO(isc_array_lookup_bounds),
	PROTO(isc_array_lookup_desc),
	PROTO(isc_array_set_desc),
	PROTO(isc_array_put_slice),
	PROTO(isc_blob_default_desc),
	PROTO(isc_blob_gen_bpb),
	PROTO(isc_blob_info),
	PROTO(isc_blob_lookup_desc),
	PROTO(isc_blob_set_desc),
	PROTO(isc_cancel_blob),
	PROTO(isc_cancel_events),
	PROTO(isc_close_blob),
	PROTO(isc_commit_retaining),
	PROTO(isc_commit_transaction),
	PROTO(isc_create_blob),
	PROTO(isc_create_blob2),
	PROTO(isc_create_database),
	PROTO(isc_database_info),
	PROTO(isc_decode_date),
	PROTO(isc_decode_sql_date),
	PROTO(isc_decode_sql_time),
	PROTO(isc_decode_timestamp),
	PROTO(isc_detach_database),
	PROTO(isc_drop_database),
	PROTO(isc_dsql_allocate_statement),
	PROTO(isc_dsql_alloc_statement2),
	PROTO(isc_dsql_describe),
	PROTO(isc_dsql_describe_bind),
	PROTO(isc_dsql_exec_immed2),
	PROTO(isc_dsql_execute),
	PROTO(isc_dsql_execute2),
	PROTO(isc_dsql_execute_immediate),
	PROTO(isc_dsql_fetch),
	PROTO(isc_dsql_finish),
	PROTO(isc_dsql_free_statement),
	PROTO(isc_dsql_insert),
	PROTO(isc_dsql_prepare),
	PROTO(isc_dsql_set_cursor_name),
	PROTO(isc_dsql_sql_info),
	PROTO(isc_encode_date),
	PROTO(isc_encode_sql_date),
	PROTO(isc_encode_sql_time),
	PROTO(isc_encode_timestamp),
	PROTO(isc_event_block),
	PROTO(isc_event_counts),
	PROTO(isc_expand_dpb),
	PROTO(isc_modify_dpb),
	PROTO(isc_free),
	PROTO(isc_get_segment),
	PROTO(isc_get_slice),
	PROTO(isc_interprete),
	PROTO(isc_open_blob),
	PROTO(isc_open_blob2),
	PROTO(isc_prepare_transaction2),
	PROTO(isc_print_sqlerror),
	PROTO(isc_print_status),
	PROTO(isc_put_segment),
	PROTO(isc_put_slice),
	PROTO(isc_que_events),
	PROTO(isc_rollback_retaining),
	PROTO(isc_rollback_transaction),
	PROTO(isc_start_multiple),
	PROTO(isc_start_transaction),
	PROTO(isc_reconnect_transaction),
	PROTO(isc_sqlcode),
	PROTO(isc_sql_interprete),
	PROTO(isc_transaction_info),
	PROTO(isc_transact_request),
	PROTO(isc_vax_integer),
	PROTO(isc_seek_blob),
	PROTO(isc_service_attach),
	PROTO(isc_service_detach),
	PROTO(isc_service_query),
	PROTO(isc_service_start),
	PROTO(fb_cancel_operation)
};


void FBProvider::loadAPI()
{
	m_api = isc_callbacks;
	m_api_loaded = true;
}


static bool isConnectionBrokenError(ISC_STATUS status)
{
	switch (status)
	{
	case isc_att_shutdown:
	case isc_network_error:
	case isc_net_read_err:
	case isc_net_write_err:
		return true;
	}
	return false;
}


static void parseSQLDA(XSQLDA* xsqlda, UCharBuffer& buff, Firebird::Array<dsc> &descs)
{
	size_t offset = 0;
	XSQLVAR* xVar = xsqlda->sqlvar;

    for (int i = 0; i < xsqlda->sqld; xVar++, i++)
	{
		const UCHAR dtype = sqlTypeToDscType(xVar->sqltype & ~1);
		xVar->sqltype |= 1;

		if (type_alignments[dtype])
			offset = FB_ALIGN(offset, type_alignments[dtype]);

        offset += xVar->sqllen;
        const int type = xVar->sqltype & (~1);
		if (type == SQL_VARYING) {
            offset += sizeof(SSHORT);
		}

		// null indicator
		offset = FB_ALIGN(offset, type_alignments[dtype_short]);
		offset += sizeof(SSHORT);
    }

	descs.resize(xsqlda->sqld * 2);
	UCHAR* p = buff.getBuffer(offset);

	offset = 0;
	xVar = xsqlda->sqlvar;

    for (int i = 0; i < xsqlda->sqld; xVar++, i++)
	{
		const UCHAR dtype = sqlTypeToDscType(xVar->sqltype & ~1);
		if (type_alignments[dtype])
			offset = FB_ALIGN(offset, type_alignments[dtype]);

		xVar->sqldata = (ISC_SCHAR*) (p + offset);

		// build the src descriptor
		dsc& src = descs[i * 2];
		src.dsc_dtype = dtype;
		src.dsc_length = xVar->sqllen;
		src.dsc_scale = xVar->sqlscale;
		src.dsc_sub_type = xVar->sqlsubtype;
		src.dsc_address = (UCHAR*) xVar->sqldata;

		offset += xVar->sqllen;
        const int type = xVar->sqltype & (~1);
		if (type == SQL_VARYING)
		{
            offset += sizeof(SSHORT);
			src.dsc_length += sizeof(SSHORT);
		}
		else if (type == SQL_NULL) {
			src.dsc_flags |= DSC_null;
		}

		// null indicator
		offset = FB_ALIGN(offset, type_alignments[dtype_short]);
		xVar->sqlind = (SSHORT*) (p + offset);

		dsc& null = descs[i * 2 + 1];
		null.makeShort(0, xVar->sqlind);

		offset += sizeof(SSHORT);
	}
}


static UCHAR sqlTypeToDscType(SSHORT sqlType)
{
	switch (sqlType)
	{
	case SQL_VARYING:
		return dtype_varying;
	case SQL_TEXT:
		return dtype_text;
	case SQL_NULL:
		return dtype_text;
	case SQL_DOUBLE:
		return dtype_double;
	case SQL_FLOAT:
		return dtype_real;
	case SQL_D_FLOAT:
		return dtype_d_float;
	case SQL_TYPE_DATE:
		return dtype_sql_date;
	case SQL_TYPE_TIME:
		return dtype_sql_time;
	case SQL_TIMESTAMP:
		return dtype_timestamp;
	case SQL_BLOB:
		return dtype_blob;
	case SQL_ARRAY:
		return dtype_array;
	case SQL_LONG:
		return dtype_long;
	case SQL_SHORT:
		return dtype_short;
	case SQL_INT64:
		return dtype_int64;
	case SQL_QUAD:
		return dtype_quad;
	default:
		return dtype_unknown;
	}
}



} // namespace EDS
