/*
 *	PROGRAM:	SQL Trace plugin
 *	MODULE:		TracePluginImpl.cpp
 *	DESCRIPTION:	Plugin implementation
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

#include <time.h>
#include <math.h>

#include "TracePluginImpl.h"
#include "PluginLogWriter.h"
#include "os/platform.h"
#include "../../common/isc_f_proto.h"
#include "../../jrd/req.h"
#include "../../jrd/svc.h"
#include "../../common/os/path_utils.h"
#include "../../jrd/inf_pub.h"
#include "../../dsql/sqlda_pub.h"
#include "../common/classes/ImplementHelper.h"


using namespace Firebird;
using namespace Jrd;

static const char* const DEFAULT_LOG_NAME = "default_trace.log";
static MakeUpgradeInfo<> upInfo;

#ifdef WIN_NT
#define NEWLINE "\r\n"
#else
#define NEWLINE "\n"
#endif


/// TracePluginImpl

const char* TracePluginImpl::marshal_exception(const Firebird::Exception& ex)
{
	ISC_STATUS_ARRAY status = {0};
	ex.stuff_exception(status);

	char buff[1024];
	char* p = buff;
	char* const end = buff + sizeof(buff) - 1;

	const ISC_STATUS* s = status;
	while (end > p && fb_interpret(p, end - p, &s))
	{
		p += strlen(p);
		if (p < end)
			*p++ = '\n';
	}
	*p = 0;

	set_error_string(buff);
	return get_error_string();
}

TracePluginImpl::TracePluginImpl(const TracePluginConfig &configuration, TraceInitInfo* initInfo) :
	operational(false),
	session_id(initInfo->getTraceSessionID()),
	session_name(*getDefaultMemoryPool()),
	logWriter(initInfo->getLogWriter()),
	config(configuration),
	record(*getDefaultMemoryPool()),
	connections(getDefaultMemoryPool()),
	transactions(getDefaultMemoryPool()),
	statements(getDefaultMemoryPool()),
	services(getDefaultMemoryPool()),
	unicodeCollation(*getDefaultMemoryPool())
{
	const char* ses_name = initInfo->getTraceSessionName();
	session_name = ses_name && *ses_name ? ses_name : " ";

	if (!logWriter)
	{
		PathName logname(configuration.log_filename);
		if (logname.empty()) {
			logname = DEFAULT_LOG_NAME;
		}

		if (PathUtils::isRelative(logname))
		{
			PathName root(initInfo->getFirebirdRootDirectory());
			PathUtils::ensureSeparator(root);
			logname.insert(0, root);
		}

		logWriter = new PluginLogWriter(logname.c_str(), config.max_log_size * 1024 * 1024);
		logWriter->addRef();
	}
	else
	{
		MasterInterfacePtr()->upgradeInterface(logWriter, FB_TRACE_LOG_WRITER_VERSION, upInfo);
	}

	Jrd::TextType* textType = unicodeCollation.getTextType();

	// Compile filtering regular expressions
	const char* str = NULL;
	try
	{
		if (config.include_filter.hasData())
		{
			str = config.include_filter.c_str();
			string filter(config.include_filter);
			ISC_systemToUtf8(filter);

			include_matcher = new SimilarToMatcher<UCHAR, UpcaseConverter<> >(
				*getDefaultMemoryPool(), textType, (const UCHAR*) filter.c_str(),
				filter.length(), '\\', true);
		}

		if (config.exclude_filter.hasData())
		{
			str = config.exclude_filter.c_str();
			string filter(config.exclude_filter);
			ISC_systemToUtf8(filter);

			exclude_matcher = new SimilarToMatcher<UCHAR, UpcaseConverter<> >(
				*getDefaultMemoryPool(), textType, (const UCHAR*) filter.c_str(),
				filter.length(), '\\', true);
		}
	}
	catch (const Exception&)
	{
		if (config.db_filename.empty())
		{
			fatal_exception::raiseFmt(
				"error while compiling regular expression \"%s\"", str);
		}
		else
		{
			fatal_exception::raiseFmt(
				"error while compiling regular expression \"%s\" for database \"%s\"",
				str, config.db_filename.c_str());
		}
	}

	operational = true;
	log_init();
}

TracePluginImpl::~TracePluginImpl()
{
	// All objects must have been free already, but in case something remained
	// deallocate tracking objects now.

	if (operational)
	{
		if (connections.getFirst())
		{
			do {
				connections.current().deallocate_references();
			} while (connections.getNext());
		}

		if (transactions.getFirst())
		{
			do {
				transactions.current().deallocate_references();
			} while (transactions.getNext());
		}

		if (statements.getFirst())
		{
			do {
				delete statements.current().description;
			} while (statements.getNext());
		}

		if (services.getFirst())
		{
			do {
				services.current().deallocate_references();
			} while (services.getNext());
		}

		log_finalize();
	}
}

void TracePluginImpl::logRecord(const char* action)
{
	// We use atomic file appends for logging. Do not try to break logging
	// to multiple separate file operations
	const Firebird::TimeStamp stamp(Firebird::TimeStamp::getCurrentTimeStamp());
	struct tm times;
	stamp.decode(&times);

	char buffer[100];
	SNPRINTF(buffer, sizeof(buffer), "%04d-%02d-%02dT%02d:%02d:%02d.%04d (%d:%p) %s" NEWLINE,
		times.tm_year + 1900, times.tm_mon + 1, times.tm_mday, times.tm_hour,
		times.tm_min, times.tm_sec, (int) (stamp.value().timestamp_time % ISC_TIME_SECONDS_PRECISION),
		get_process_id(), this, action);

	record.insert(0, buffer);
	record.append(NEWLINE);
	// TODO: implement adjusting of line breaks
	// line.adjustLineBreaks();

	logWriter->write(record.c_str(), record.length());

	record = "";
}

void TracePluginImpl::logRecordConn(const char* action, TraceDatabaseConnection* connection)
{
	// Lookup connection description
	const int conn_id = connection->getConnectionID();
	bool reg = false;

	while (true)
	{
		{
			ReadLockGuard lock(connectionsLock, FB_FUNCTION);
			ConnectionsTree::Accessor accessor(&connections);
			if (accessor.locate(conn_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\t%s (ATT_%d, <unknown, bug?>)" NEWLINE,
				config.db_filename.c_str(), conn_id);
			record.insert(0, temp);
			break;
		}

		register_connection(connection);
		reg = true;
	}

	// don't keep failed connection
	if (!conn_id)
	{
		WriteLockGuard lock(connectionsLock, FB_FUNCTION);
		ConnectionsTree::Accessor accessor(&connections);
		if (accessor.locate(conn_id))
		{
			accessor.current().deallocate_references();
			accessor.fastRemove();
		}
	}

	logRecord(action);
}

void TracePluginImpl::logRecordTrans(const char* action, TraceDatabaseConnection* connection,
	TraceTransaction* transaction)
{
	const unsigned tra_id = transaction->getTransactionID();
	bool reg = false;
	while (true)
	{
		// Lookup transaction description
		{
			ReadLockGuard lock(transactionsLock, FB_FUNCTION);
			TransactionsTree::Accessor accessor(&transactions);
			if (accessor.locate(tra_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\t\t(TRA_%lu, <unknown, bug?>)" NEWLINE, transaction->getTransactionID());
			record.insert(0, temp);
			break;
		}

		register_transaction(transaction);
		reg = true;
	}

	logRecordConn(action, connection);
}

void TracePluginImpl::logRecordProcFunc(const char* action, TraceDatabaseConnection* connection,
	TraceTransaction* transaction, const char* obj_type, const char* obj_name)
{
	string temp;
	temp.printf(NEWLINE "%s %s:" NEWLINE, obj_type, obj_name);
	record.insert(0, temp);

	if (!transaction) {
		logRecordConn(action, connection);
	}
	else {
		logRecordTrans(action, connection, transaction);
	}
}

void TracePluginImpl::logRecordStmt(const char* action, TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceStatement* statement, bool isSQL)
{
	const int stmt_id = statement->getStmtID();
	bool reg = false;
	bool log = true;

	while (true)
	{
		// Lookup description for statement
		{
			ReadLockGuard lock(statementsLock, FB_FUNCTION);

			StatementsTree::Accessor accessor(&statements);
			if (accessor.locate(stmt_id))
			{
				const string* description = accessor.current().description;

				log = (description != NULL);
				// Do not say anything about statements which do not fall under filter criteria
				if (log) {
					record.insert(0, *description);
				}
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf(NEWLINE "Statement %d, <unknown, bug?>:" NEWLINE, stmt_id);
			record.insert(0, temp);
			break;
		}

		if (isSQL) {
			register_sql_statement((TraceSQLStatement*) statement);
		}
		else {
			register_blr_statement((TraceBLRStatement*) statement);
		}
		reg = true;
	}

	// don't need to keep failed statement
	if (!stmt_id)
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		StatementsTree::Accessor accessor(&statements);
		if (accessor.locate(stmt_id))
		{
			delete accessor.current().description;
			accessor.fastRemove();
		}
	}

	if (!log)
	{
		record = "";
		return;
	}

	if (!transaction) {
		logRecordConn(action, connection);
	}
	else {
		logRecordTrans(action, connection, transaction);
	}
}

void TracePluginImpl::logRecordServ(const char* action, TraceServiceConnection* service)
{
	const ntrace_service_t svc_id = service->getServiceID();
	bool reg = false;

	while (true)
	{
		// Lookup service description
		{
			ReadLockGuard lock(servicesLock, FB_FUNCTION);

			ServicesTree::Accessor accessor(&services);
			if (accessor.locate(svc_id))
			{
				record.insert(0, *accessor.current().description);
				break;
			}
		}

		if (reg)
		{
			string temp;
			temp.printf("\tService %p, <unknown, bug?>" NEWLINE, svc_id);
			record.insert(0, temp);
			break;
		}

		register_service(service);
		reg = true;
	}

	logRecord(action);
}

void TracePluginImpl::logRecordError(const char* action, TraceBaseConnection* connection,
	TraceStatusVector* status)
{
	const char* err = status->getText();

	record.insert(0, err);

	if (connection)
	{
		switch (connection->getKind())
		{
		case connection_database:
			logRecordConn(action, (TraceDatabaseConnection*) connection);
			break;

		case connection_service:
			logRecordServ(action, (TraceServiceConnection*) connection);
			break;

		default:
			break;
		}
	}
	else
		logRecord(action);
}

void TracePluginImpl::appendGlobalCounts(const PerformanceInfo* info)
{
	string temp;

	temp.printf("%7"QUADFORMAT"d ms", info->pin_time);
	record.append(temp);

	ntrace_counter_t cnt;

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_READS]) != 0)
	{
		temp.printf(", %"QUADFORMAT"d read(s)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_WRITES]) != 0)
	{
		temp.printf(", %"QUADFORMAT"d write(s)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_FETCHES]) != 0)
	{
		temp.printf(", %"QUADFORMAT"d fetch(es)", cnt);
		record.append(temp);
	}

	if ((cnt = info->pin_counters[RuntimeStatistics::PAGE_MARKS]) != 0)
	{
		temp.printf(", %"QUADFORMAT"d mark(s)", cnt);
		record.append(temp);
	}

	record.append(NEWLINE);
}

void TracePluginImpl::appendTableCounts(const PerformanceInfo *info)
{
	if (!config.print_perf || info->pin_count == 0)
		return;

	record.append(NEWLINE
		"Table                             Natural     Index    Update    Insert    Delete   Backout     Purge   Expunge" NEWLINE
		"***************************************************************************************************************" NEWLINE );

	const TraceCounts* trc;
	const TraceCounts* trc_end;

	string temp;
	for (trc = info->pin_tables, trc_end = trc + info->pin_count; trc < trc_end; trc++)
	{
		record.append(trc->trc_relation_name);
		record.append(MAX_SQL_IDENTIFIER_LEN - strlen(trc->trc_relation_name), ' ');
		for (int j = 0; j < DBB_max_rel_count; j++)
		{
			if (trc->trc_counters[j] == 0)
			{
				record.append(10, ' ');
			}
			else
			{
				//fb_utils::exactNumericToStr(trc->trc_counters[j], 0, temp);
				//record.append(' ', 10 - temp.length());
				temp.printf("%10"QUADFORMAT"d", trc->trc_counters[j]);
				record.append(temp);
			}
		}
		record.append(NEWLINE);
	}
}


void TracePluginImpl::formatStringArgument(string& result, const UCHAR* str, size_t len)
{
	if (config.max_arg_length && len > config.max_arg_length)
	{
		/* CVC: We will wrap with the original code.
		len = config.max_arg_length - 3;
		if (len < 0)
			len = 0;
		*/
		if (config.max_arg_length < 3)
			len = 0;
		else
			len = config.max_arg_length - 3;

		result.printf("%.*s...", len, str);
		return;
	}
	result.printf("%.*s", len, str);
}


void TracePluginImpl::appendParams(TraceParams* params)
{
	const size_t paramcount = params->getCount();
	if (!paramcount)
		return;

	// NS: Please, do not move strings inside the loop. This is performance-sensitive piece of code.
	string paramtype;
	string paramvalue;
	string temp;

	for (size_t i = 0; i < paramcount; i++)
	{
		const struct dsc* parameters = params->getParam(i);

		// See if we need to print any more arguments
		if (config.max_arg_count && i >= config.max_arg_count)
		{
			temp.printf("...%d more arguments skipped..." NEWLINE, paramcount - i);
			record.append(temp);
			break;
		}

		// Assign type name
		switch (parameters->dsc_dtype)
		{
			case dtype_text:
				paramtype.printf("char(%d)", parameters->dsc_length);
				break;
			case dtype_cstring:
				paramtype.printf("cstring(%d)", parameters->dsc_length - 1);
				break;
			case dtype_varying:
				paramtype.printf("varchar(%d)", parameters->dsc_length - 2);
				break;
			case dtype_blob:
				paramtype = "blob";
				break;
			case dtype_array:
				paramtype = "array";
				break;
			case dtype_quad:
				paramtype = "quad";
				break;

			case dtype_short:
				if (parameters->dsc_scale)
					paramtype.printf("smallint(*, %d)", parameters->dsc_scale);
				else
					paramtype = "smallint";
				break;

			case dtype_long:
				if (parameters->dsc_scale)
					paramtype.printf("integer(*, %d)", parameters->dsc_scale);
				else
					paramtype = "integer";
				break;

			case dtype_double:
				if (parameters->dsc_scale)
					paramtype.printf("double precision(*, %d)", parameters->dsc_scale);
				else
					paramtype = "double precision";
				break;

			case dtype_int64:
				if (parameters->dsc_scale)
					paramtype.printf("bigint(*, %d)", parameters->dsc_scale);
				else
					paramtype = "bigint";
				break;

			case dtype_real:
				paramtype = "float";
				break;
			case dtype_sql_date:
				paramtype = "date";
				break;
			case dtype_sql_time:
				paramtype = "time";
				break;
			case dtype_timestamp:
				paramtype = "timestamp";
				break;
			case dtype_dbkey:
				paramtype = "db_key";
				break;
			case dtype_boolean:
				paramtype = "boolean";
				break;

			default:
				paramtype.printf("<type%d>", parameters->dsc_dtype);
				break;
		}

		if (parameters->dsc_flags & DSC_null)
		{
			paramvalue = "<NULL>";
		}
		else
		{
			// Assign value
			switch (parameters->dsc_dtype)
			{
				// Handle potentially long string values
				case dtype_text:
					formatStringArgument(paramvalue,
						parameters->dsc_address, parameters->dsc_length);
					break;
				case dtype_cstring:
					formatStringArgument(paramvalue,
						parameters->dsc_address,
						strlen(reinterpret_cast<const char*>(parameters->dsc_address)));
					break;
				case dtype_varying:
					formatStringArgument(paramvalue,
						parameters->dsc_address + 2,
						*(USHORT*)parameters->dsc_address);
					break;

				// Handle quad
				case dtype_quad:
				case dtype_blob:
				case dtype_array:
				case dtype_dbkey:
				{
					ISC_QUAD *quad = (ISC_QUAD*) parameters->dsc_address;
					paramvalue.printf("%08X%08X", quad->gds_quad_high, quad->gds_quad_low);
					break;
				}

				case dtype_short:
					fb_utils::exactNumericToStr(*(SSHORT*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_long:
					fb_utils::exactNumericToStr(*(SLONG*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_int64:
					fb_utils::exactNumericToStr(*(SINT64*) parameters->dsc_address, parameters->dsc_scale, paramvalue);
					break;

				case dtype_real:
					if (!parameters->dsc_scale) {
						paramvalue.printf("%.7g", *(float*) parameters->dsc_address);
					}
					else {
						paramvalue.printf("%.7g",
							*(float*) parameters->dsc_address * pow(10.0f, -parameters->dsc_scale));
					}
					break;

				case dtype_double:
					if (!parameters->dsc_scale) {
						paramvalue.printf("%.15g", *(double*) parameters->dsc_address);
					}
					else {
						paramvalue.printf("%.15g",
							*(double*) parameters->dsc_address * pow(10.0, -parameters->dsc_scale));
					}
					break;

				case dtype_sql_date:
				{
					struct tm times;
					Firebird::TimeStamp::decode_date(*(ISC_DATE*)parameters->dsc_address, &times);
					paramvalue.printf("%04d-%02d-%02d", times.tm_year + 1900, times.tm_mon + 1, times.tm_mday);
					break;
				}
				case dtype_sql_time:
				{
					int hours, minutes, seconds, fractions;
					Firebird::TimeStamp::decode_time(*(ISC_TIME*) parameters->dsc_address,
						&hours, &minutes, &seconds, &fractions);

					paramvalue.printf("%02d:%02d:%02d.%04d", hours,	minutes, seconds, fractions);
					break;
				}
				case dtype_timestamp:
				{
					Firebird::TimeStamp ts(*(ISC_TIMESTAMP*) parameters->dsc_address);
					struct tm times;

					ts.decode(&times);

					paramvalue.printf("%04d-%02d-%02dT%02d:%02d:%02d.%04d",
						times.tm_year + 1900, times.tm_mon + 1, times.tm_mday,
						times.tm_hour, times.tm_min, times.tm_sec,
						ts.value().timestamp_time % ISC_TIME_SECONDS_PRECISION);
					break;
				}

				case dtype_boolean:
					paramvalue = *parameters->dsc_address ? "<true>" : "<false>";
					break;

				default:
					paramvalue = "<unknown>";
			}
		}
		temp.printf("param%d = %s, \"%s\"" NEWLINE, i, paramtype.c_str(), paramvalue.c_str());
		record.append(temp);
	}
}

void TracePluginImpl::appendServiceQueryParams(size_t send_item_length,
		const ntrace_byte_t* send_items, size_t recv_item_length,
		const ntrace_byte_t* recv_items)
{
	string send_query;
	string recv_query;
	USHORT l;
	UCHAR item;
	//USHORT timeout = 0; // Unused

	const UCHAR* items = send_items;
	const UCHAR* const end_items = items + send_item_length;
	while (items < end_items && *items != isc_info_end)
	{
		switch ((item = *items++))
		{
		case isc_info_end:
			break;

		default:
			if (items + 2 <= end_items)
			{
				l = (USHORT) gds__vax_integer(items, 2);
				items += 2;
				if (items + l <= end_items)
				{
					switch (item)
					{
					case isc_info_svc_line:
						send_query.printf(NEWLINE "\t\t send line: %.*s", l, items);
						break;
					case isc_info_svc_message:
						send_query.printf(NEWLINE "\t\t send message: %.*s", l + 3, items - 3);
						break;
					case isc_info_svc_timeout:
						send_query.printf(NEWLINE "\t\t set timeout: %d",
							(USHORT) gds__vax_integer(items, l));
						break;
					case isc_info_svc_version:
						send_query.printf(NEWLINE "\t\t set version: %d",
							(USHORT) gds__vax_integer(items, l));
						break;
					}
				}
				items += l;
			}
			else
				items += 2;
			break;
		}
	}

	if (send_query.hasData())
	{
		record.append("\t Send portion of the query:");
		record.append(send_query);
	}

	items = recv_items;
	const UCHAR* const end_items2 = items + recv_item_length;

	if (*items == isc_info_length) {
		items++;
	}

	while (items < end_items2 && *items != isc_info_end)
	{
		switch ((item = *items++))
		{
			case isc_info_end:
				break;

			case isc_info_svc_svr_db_info:
				recv_query.printf(NEWLINE "\t\t retrieve number of attachments and databases");
				break;

			case isc_info_svc_svr_online:
				recv_query.printf(NEWLINE "\t\t set service online");
				break;

			case isc_info_svc_svr_offline:
				recv_query.printf(NEWLINE "\t\t set service offline");
				break;

			case isc_info_svc_get_env:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD");
				break;

			case isc_info_svc_get_env_lock:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD_LCK");
				break;

			case isc_info_svc_get_env_msg:
				recv_query.printf(NEWLINE "\t\t retrieve the setting of $FIREBIRD_MSG");
				break;

			case isc_info_svc_dump_pool_info:
				recv_query.printf(NEWLINE "\t\t print memory counters");
				break;

			case isc_info_svc_get_config:
				recv_query.printf(NEWLINE "\t\t retrieve the parameters and values for IB_CONFIG");
				break;

			case isc_info_svc_default_config:
				recv_query.printf(NEWLINE "\t\t reset the config values to defaults");
				break;

			case isc_info_svc_set_config:
				recv_query.printf(NEWLINE "\t\t set the config values");
				break;

			case isc_info_svc_version:
				recv_query.printf(NEWLINE "\t\t retrieve the version of the service manager");
				break;

			case isc_info_svc_capabilities:
				recv_query.printf(NEWLINE "\t\t retrieve a bitmask representing the server's capabilities");
				break;

			case isc_info_svc_server_version:
				recv_query.printf(NEWLINE "\t\t retrieve the version of the server engine");
				break;

			case isc_info_svc_implementation:
				recv_query.printf(NEWLINE "\t\t retrieve the implementation of the Firebird server");
				break;

			case isc_info_svc_user_dbpath:
				recv_query.printf(NEWLINE "\t\t retrieve the path to the security database in use by the server");
				break;

			case isc_info_svc_response:
				recv_query.printf(NEWLINE "\t\t retrieve service response");
				break;

			case isc_info_svc_response_more:
				recv_query.printf(NEWLINE "\t\t retrieve service response more");
				break;

			case isc_info_svc_total_length:
				recv_query.printf(NEWLINE "\t\t retrieve total length");
				break;

			case isc_info_svc_line:
				recv_query.printf(NEWLINE "\t\t retrieve 1 line of service output per call");
				break;

			case isc_info_svc_to_eof:
				recv_query.printf(NEWLINE "\t\t retrieve as much of the server output as will fit in the supplied buffer");
				break;

			case isc_info_svc_limbo_trans:
				recv_query.printf(NEWLINE "\t\t retrieve the limbo transactions");
				break;

			case isc_info_svc_get_users:
				recv_query.printf(NEWLINE "\t\t retrieve the user information");
				break;

			case isc_info_svc_stdin:
				recv_query.printf(NEWLINE "\t\t retrieve the size of data to send to the server");
				break;
		}
	}

	if (recv_query.hasData())
	{
		record.append("\t Receive portion of the query:");
		record.append(recv_query);
	}
}

void TracePluginImpl::log_init()
{
	record.printf("\tSESSION_%d %s" NEWLINE "\t%s" NEWLINE, session_id, session_name.c_str(), config.db_filename.c_str());
	logRecord("TRACE_INIT");
}

void TracePluginImpl::log_finalize()
{
	record.printf("\tSESSION_%d %s" NEWLINE "\t%s" NEWLINE, session_id, session_name.c_str(), config.db_filename.c_str());
	logRecord("TRACE_FINI");

	logWriter->release();
	logWriter = NULL;
}

void TracePluginImpl::register_connection(TraceDatabaseConnection* connection)
{
	ConnectionData conn_data;
	conn_data.id = connection->getConnectionID();
	conn_data.description = FB_NEW(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

	string tmp(*getDefaultMemoryPool());

	conn_data.description->printf("\t%s (ATT_%d",  connection->getDatabaseName(), connection->getConnectionID());

	const char* user = connection->getUserName();
	if (user)
	{
		const char* role = connection->getRoleName();
		if (role && *role) {
			tmp.printf(", %s:%s", user, role);
		}
		else {
			tmp.printf(", %s", user);
		}
		conn_data.description->append(tmp);
	}
	else
	{
		conn_data.description->append(", <unknown_user>");
	}

	const char* charSet = connection->getCharSet();
	tmp.printf(", %s", charSet && *charSet ? charSet : "NONE");
	conn_data.description->append(tmp);

	const char* remProto = connection->getRemoteProtocol();
	const char* remAddr = connection->getRemoteAddress();
	if (remProto && *remProto)
	{
		tmp.printf(", %s:%s)", remProto, remAddr);
		conn_data.description->append(tmp);
	}
	else
	{
		conn_data.description->append(", <internal>)");
	}

	const char *prc_name = connection->getRemoteProcessName();
	if (prc_name && *prc_name)
	{
		tmp.printf(NEWLINE "\t%s:%d", prc_name, connection->getRemoteProcessID());
		conn_data.description->append(tmp);
	}
	conn_data.description->append(NEWLINE);

	// Adjust the list of connections
	{
		WriteLockGuard lock(connectionsLock, FB_FUNCTION);
		connections.add(conn_data);
	}
}

void TracePluginImpl::log_event_attach(TraceDatabaseConnection* connection,
	ntrace_boolean_t create_db, ntrace_result_t att_result)
{
	if (config.log_connections)
	{
		const char* event_type;
		switch (att_result)
		{
			case res_successful:
				event_type = create_db ? "CREATE_DATABASE" : "ATTACH_DATABASE";
				break;
			case res_failed:
				event_type = create_db ? "FAILED CREATE_DATABASE" : "FAILED ATTACH_DATABASE";
				break;
			case res_unauthorized:
				event_type = create_db ? "UNAUTHORIZED CREATE_DATABASE" : "UNAUTHORIZED ATTACH_DATABASE";
				break;
			default:
				event_type = create_db ?
					"Unknown event in CREATE DATABASE ": "Unknown event in ATTACH_DATABASE";
				break;
		}

		logRecordConn(event_type, connection);
	}
}

void TracePluginImpl::log_event_detach(TraceDatabaseConnection* connection, ntrace_boolean_t drop_db)
{
	if (config.log_connections)
	{
		logRecordConn(drop_db ? "DROP_DATABASE" : "DETACH_DATABASE", connection);
	}

	// Get rid of connection descriptor
	WriteLockGuard lock(connectionsLock, FB_FUNCTION);
	if (connections.locate(connection->getConnectionID()))
	{
		connections.current().deallocate_references();
		connections.fastRemove();
	}
}

void TracePluginImpl::register_transaction(TraceTransaction* transaction)
{
	TransactionData trans_data;
	trans_data.id = transaction->getTransactionID();
	trans_data.description = FB_NEW(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());
	trans_data.description->printf("\t\t(TRA_%lu, ", trans_data.id);

	switch (transaction->getIsolation())
	{
	case tra_iso_consistency:
		trans_data.description->append("CONSISTENCY");
		break;

	case tra_iso_concurrency:
		trans_data.description->append("CONCURRENCY");
		break;

	case tra_iso_read_committed_recver:
		trans_data.description->append("READ_COMMITTED | REC_VERSION");
		break;

	case tra_iso_read_committed_norecver:
		trans_data.description->append("READ_COMMITTED | NO_REC_VERSION");
		break;

	default:
		trans_data.description->append("<unknown>");
	}

	const int wait = transaction->getWait();
	if (wait < 0) {
		trans_data.description->append(" | WAIT");
	}
	else if (wait == 0) {
		trans_data.description->append(" | NOWAIT");
	}
	else
	{
		string s;
		s.printf(" | WAIT %d", wait);
		trans_data.description->append(s);
	}

	if (transaction->getReadOnly()) {
		trans_data.description->append(" | READ_ONLY");
	}
	else {
		trans_data.description->append(" | READ_WRITE");
	}

	trans_data.description->append(")" NEWLINE);

	// Remember transaction
	{
		WriteLockGuard lock(transactionsLock, FB_FUNCTION);
		transactions.add(trans_data);
	}
}


void TracePluginImpl::log_event_transaction_start(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, size_t /*tpb_length*/,
		const ntrace_byte_t* /*tpb*/, ntrace_result_t tra_result)
{
	if (config.log_transactions)
	{
		const char* event_type;
		switch (tra_result)
		{
			case res_successful:
				event_type = "START_TRANSACTION";
				break;
			case res_failed:
				event_type = "FAILED START_TRANSACTION";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED START_TRANSACTION";
				break;
			default:
				event_type = "Unknown event in START_TRANSACTION";
				break;
		}
		logRecordTrans(event_type, connection, transaction);
	}
}

void TracePluginImpl::log_event_transaction_end(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, ntrace_boolean_t commit,
		ntrace_boolean_t retain_context, ntrace_result_t tra_result)
{
	if (config.log_transactions)
	{
		PerformanceInfo* info = transaction->getPerf();
		if (info)
		{
			appendGlobalCounts(info);
			appendTableCounts(info);
		}

		const char* event_type;
		switch (tra_result)
		{
			case res_successful:
				event_type = commit ?
					(retain_context ? "COMMIT_RETAINING"   : "COMMIT_TRANSACTION") :
					(retain_context ? "ROLLBACK_RETAINING" : "ROLLBACK_TRANSACTION");
				break;
			case res_failed:
				event_type = commit ?
					(retain_context ? "FAILED COMMIT_RETAINING"   : "FAILED COMMIT_TRANSACTION") :
					(retain_context ? "FAILED ROLLBACK_RETAINING" : "FAILED ROLLBACK_TRANSACTION");
				break;
			case res_unauthorized:
				event_type = commit ?
					(retain_context ? "UNAUTHORIZED COMMIT_RETAINING"   : "UNAUTHORIZED COMMIT_TRANSACTION") :
					(retain_context ? "UNAUTHORIZED ROLLBACK_RETAINING" : "UNAUTHORIZED ROLLBACK_TRANSACTION");
				break;
			default:
				event_type = "Unknown event at transaction end";
				break;
		}
		logRecordTrans(event_type, connection, transaction);
	}

	if (!retain_context)
	{
		// Forget about the transaction
		WriteLockGuard lock(transactionsLock, FB_FUNCTION);
		if (transactions.locate(transaction->getTransactionID()))
		{
			transactions.current().deallocate_references();
			transactions.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_set_context(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceContextVariable* variable)
{
	const char* ns = variable->getNameSpace();
	const char* name = variable->getVarName();
	const char* value = variable->getVarValue();

	const size_t ns_len = strlen(ns);
	const size_t name_len = strlen(name);
	const size_t value_len = value ? strlen(value) : 0;

	if (config.log_context)
	{
		if (value == NULL) {
			record.printf("[%.*s] %.*s = NULL" NEWLINE, ns_len, ns, name_len, name);
		}
		else {
			record.printf("[%.*s] %.*s = \"%.*s\"" NEWLINE, ns_len, ns, name_len, name, value_len, value);
		}
		logRecordTrans("SET_CONTEXT", connection, transaction);
	}
}

void TracePluginImpl::log_event_proc_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceProcedure* procedure, bool started, ntrace_result_t proc_result)
{
	if (!config.log_procedure_start && started)
		return;

	if (!config.log_procedure_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : procedure->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	TraceParams* params = procedure->getInputs();
	if (params && params->getCount())
	{
		appendParams(params);
		record.append(NEWLINE);
	}

	if (info)
	{
		if (info->pin_records_fetched)
		{
			string temp;
			temp.printf("%"QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
			record.append(temp);
		}
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (proc_result)
	{
		case res_successful:
			event_type = started ? "EXECUTE_PROCEDURE_START" :
								   "EXECUTE_PROCEDURE_FINISH";
			break;
		case res_failed:
			event_type = started ? "FAILED EXECUTE_PROCEDURE_START" :
								   "FAILED EXECUTE_PROCEDURE_FINISH";
			break;
		case res_unauthorized:
			event_type = started ? "UNAUTHORIZED EXECUTE_PROCEDURE_START" :
								   "UNAUTHORIZED EXECUTE_PROCEDURE_FINISH";
			break;
		default:
			event_type = "Unknown event at executing procedure";
			break;
	}

	logRecordProcFunc(event_type, connection, transaction, "Procedure", procedure->getProcName());
}

void TracePluginImpl::log_event_func_execute(TraceDatabaseConnection* connection, TraceTransaction* transaction,
		TraceFunction* function, bool started, ntrace_result_t func_result)
{
	if (!config.log_function_start && started)
		return;

	if (!config.log_function_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : function->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	TraceParams* params = function->getInputs();
	if (params && params->getCount())
	{
		appendParams(params);
		record.append(NEWLINE);
	}

	if (!started && func_result == res_successful)
	{
		params = function->getResult();
		{
			record.append("returns:"NEWLINE);
			appendParams(params);
			record.append(NEWLINE);
		}
	}

	if (info)
	{
		if (info->pin_records_fetched)
		{
			string temp;
			temp.printf("%"QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
			record.append(temp);
		}
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (func_result)
	{
		case res_successful:
			event_type = started ? "EXECUTE_FUNCTION_START" :
								   "EXECUTE_FUNCTION_FINISH";
			break;
		case res_failed:
			event_type = started ? "FAILED EXECUTE_FUNCTION_START" :
								   "FAILED EXECUTE_FUNCTION_FINISH";
			break;
		case res_unauthorized:
			event_type = started ? "UNAUTHORIZED EXECUTE_FUNCTION_START" :
								   "UNAUTHORIZED EXECUTE_FUNCTION_FINISH";
			break;
		default:
			event_type = "Unknown event at executing function";
			break;
	}

	logRecordProcFunc(event_type, connection, transaction, "Function", function->getFuncName());
}

void TracePluginImpl::register_sql_statement(TraceSQLStatement* statement)
{
	StatementData stmt_data;
	stmt_data.id = statement->getStmtID();

	bool need_statement = true;

	const char* sql = statement->getText();
	if (!sql)
		return;

	size_t sql_length = strlen(sql);
	if (!sql_length)
		return;

	if (config.include_filter.hasData() || config.exclude_filter.hasData())
	{
		const char* sqlUtf8 = statement->getTextUTF8();
		size_t utf8_length = strlen(sqlUtf8);

		if (config.include_filter.hasData())
		{
			include_matcher->reset();
			include_matcher->process((const UCHAR*) sqlUtf8, utf8_length);
			need_statement = include_matcher->result();
		}

		if (need_statement && config.exclude_filter.hasData())
		{
			exclude_matcher->reset();
			exclude_matcher->process((const UCHAR*) sqlUtf8, utf8_length);
			need_statement = !exclude_matcher->result();
		}
	}

	if (need_statement)
	{
		stmt_data.description = FB_NEW(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

		if (stmt_data.id) {
			stmt_data.description->printf(NEWLINE "Statement %d:", stmt_data.id);
		}

		string temp(*getDefaultMemoryPool());
		if (config.max_sql_length && sql_length > config.max_sql_length)
		{
			// Truncate too long SQL printing it out with ellipsis
			sql_length = (config.max_sql_length < 3) ? 0 : (config.max_sql_length - 3);
			temp.printf(NEWLINE
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s...", sql_length, sql);
		}
		else
		{
			temp.printf(NEWLINE
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s", sql_length, sql);
		}
		*stmt_data.description += temp;

		const char* access_path = config.print_plan ? statement->getPlan() : NULL;
		if (access_path && *access_path)
		{
			const size_t access_path_length = strlen(access_path);
			temp.printf(NEWLINE
				"^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^"
				"%.*s" NEWLINE, access_path_length, access_path);

			*stmt_data.description += temp;
		}
		else
		{
			*stmt_data.description += NEWLINE;
		}
	}
	else
	{
		stmt_data.description = NULL;
	}

	// Remember statement
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		statements.add(stmt_data);
	}
}

void TracePluginImpl::log_event_dsql_prepare(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceSQLStatement* statement,
		ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_statement_prepare)
	{
		const char* event_type;
		switch (req_result)
		{
			case res_successful:
				event_type = "PREPARE_STATEMENT";
				break;
			case res_failed:
				event_type = "FAILED PREPARE_STATEMENT";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED PREPARE_STATEMENT";
				break;
			default:
				event_type = "Unknown event in PREPARE_STATEMENT";
				break;
		}
		record.printf("%7d ms" NEWLINE, time_millis);
		logRecordStmt(event_type, connection, transaction, statement, true);
	}
}

void TracePluginImpl::log_event_dsql_free(TraceDatabaseConnection* connection,
		TraceSQLStatement* statement, unsigned short option)
{
	if (config.log_statement_free)
	{
		logRecordStmt(option == DSQL_drop ? "FREE_STATEMENT" : "CLOSE_CURSOR",
			connection, 0, statement, true);
	}

	if (option == DSQL_drop)
	{
		WriteLockGuard lock(statementsLock, FB_FUNCTION);
		if (statements.locate(statement->getStmtID()))
		{
			delete statements.current().description;
			statements.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_dsql_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceSQLStatement* statement,
		bool started, ntrace_result_t req_result)
{
	if (started && !config.log_statement_start)
		return;

	if (!started && !config.log_statement_finish)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : statement->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	TraceParams *params = statement->getInputs();
	if (params && params->getCount())
	{
		record.append(NEWLINE);
		appendParams(params);
		record.append(NEWLINE);
	}

	if (info)
	{
		string temp;
		temp.printf("%"QUADFORMAT"d records fetched" NEWLINE, info->pin_records_fetched);
		record.append(temp);

		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (req_result)
	{
		case res_successful:
			event_type = started ? "EXECUTE_STATEMENT_START" :
								   "EXECUTE_STATEMENT_FINISH";
			break;
		case res_failed:
			event_type = started ? "FAILED EXECUTE_STATEMENT_START" :
								   "FAILED EXECUTE_STATEMENT_FINISH";
			break;
		case res_unauthorized:
			event_type = started ? "UNAUTHORIZED EXECUTE_STATEMENT_START" :
								   "UNAUTHORIZED EXECUTE_STATEMENT_FINISH";
			break;
		default:
			event_type = "Unknown event at executing statement";
			break;
	}
	logRecordStmt(event_type, connection, transaction, statement, true);
}


void TracePluginImpl::register_blr_statement(TraceBLRStatement* statement)
{
	string* description = FB_NEW(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());

	if (statement->getStmtID()) {
		description->printf(NEWLINE "Statement %d:" NEWLINE, statement->getStmtID());
	}

	if (config.print_blr)
	{
		const char *text_blr = statement->getText();
		size_t text_blr_length = text_blr ? strlen(text_blr) : 0;
		if (!text_blr)
			text_blr = "";

		if (config.max_blr_length && text_blr_length > config.max_blr_length)
		{
			// Truncate too long BLR printing it out with ellipsis
			text_blr_length = config.max_blr_length < 3 ? 0 : config.max_blr_length - 3;
			description->printf(
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s..." NEWLINE,
				text_blr_length, text_blr);
		}
		else
		{
			description->printf(
				"-------------------------------------------------------------------------------" NEWLINE
				"%.*s" NEWLINE,
				text_blr_length, text_blr);
		}
	}

	StatementData stmt_data;
	stmt_data.id = statement->getStmtID();
	stmt_data.description = description;
	WriteLockGuard lock(statementsLock, FB_FUNCTION);

	statements.add(stmt_data);
}

void TracePluginImpl::log_event_blr_compile(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceBLRStatement* statement,
	ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_blr_requests)
	{
		{
			ReadLockGuard lock(statementsLock, FB_FUNCTION);
			StatementsTree::Accessor accessor(&statements);
			if (accessor.locate(statement->getStmtID()))
				return;
		}

		const char* event_type;
		switch (req_result)
		{
			case res_successful:
				event_type = "COMPILE_BLR";
				break;
			case res_failed:
				event_type = "FAILED COMPILE_BLR";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED COMPILE_BLR";
				break;
			default:
				event_type = "Unknown event in COMPILE_BLR";
				break;
		}

		record.printf("%7d ms", time_millis);

		logRecordStmt(event_type, connection, transaction, statement, false);
	}
}

void TracePluginImpl::log_event_blr_execute(TraceDatabaseConnection* connection,
		TraceTransaction* transaction, TraceBLRStatement* statement,
		ntrace_result_t req_result)
{
	PerformanceInfo *info = statement->getPerf();

	// Do not log operation if it is below time threshold
	if (config.time_threshold && info->pin_time < config.time_threshold)
		return;

	if (config.log_blr_requests)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);

		const char* event_type;
		switch (req_result)
		{
			case res_successful:
				event_type = "EXECUTE_BLR";
				break;
			case res_failed:
				event_type = "FAILED EXECUTE_BLR";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED EXECUTE_BLR";
				break;
			default:
				event_type = "Unknown event in EXECUTE_BLR";
				break;
		}

		logRecordStmt(event_type, connection, transaction, statement, false);
	}
}

void TracePluginImpl::log_event_dyn_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceDYNRequest* request,
	ntrace_counter_t time_millis, ntrace_result_t req_result)
{
	if (config.log_dyn_requests)
	{
		string description;

		if (config.print_dyn)
		{
			const char *text_dyn = request->getText();
			size_t text_dyn_length = text_dyn ? strlen(text_dyn) : 0;
			if (!text_dyn) {
				text_dyn = "";
			}

			if (config.max_dyn_length && text_dyn_length > config.max_dyn_length)
			{
				// Truncate too long DDL printing it out with ellipsis
				text_dyn_length = config.max_dyn_length < 3 ? 0 : config.max_dyn_length - 3;
				description.printf(
					"-------------------------------------------------------------------------------" NEWLINE
					"%.*s...",
					text_dyn_length, text_dyn);
			}
			else
			{
				description.printf(
					"-------------------------------------------------------------------------------" NEWLINE
					"%.*s",
					text_dyn_length, text_dyn);
			}
		}

		const char* event_type;
		switch (req_result)
		{
			case res_successful:
				event_type = "EXECUTE_DYN";
				break;
			case res_failed:
				event_type = "FAILED EXECUTE_DYN";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED EXECUTE_DYN";
				break;
			default:
				event_type = "Unknown event in EXECUTE_DYN";
				break;
		}

		record.printf("%7d ms", time_millis);
		record.insert(0, description);

		logRecordTrans(event_type, connection, transaction);
	}
}


void TracePluginImpl::register_service(TraceServiceConnection* service)
{
	string username(service->getUserName());
	string remote_address;
	string remote_process;

	const char* tmp = service->getRemoteAddress();
	if (tmp && *tmp) {
		remote_address.printf("%s:%s", service->getRemoteProtocol(), service->getRemoteAddress());
	}
	else
	{
		tmp = service->getRemoteProtocol();
		if (tmp && *tmp)
			remote_address = tmp;
		else
			remote_address = "internal";
	}

	if (username.isEmpty())
		username = "<user is unknown>";

	tmp = service->getRemoteProcessName();
	if (tmp && *tmp) {
		remote_process.printf(", %s:%d", tmp, service->getRemoteProcessID());
	}

	ServiceData serv_data;
	serv_data.id = service->getServiceID();
	serv_data.description = FB_NEW(*getDefaultMemoryPool()) string(*getDefaultMemoryPool());
	serv_data.description->printf("\t%s, (Service %p, %s, %s%s)" NEWLINE,
		service->getServiceMgr(), serv_data.id,
		username.c_str(), remote_address.c_str(), remote_process.c_str());
	serv_data.enabled = true;

	// Adjust the list of services
	{
		WriteLockGuard lock(servicesLock, FB_FUNCTION);
		services.add(serv_data);
	}
}


bool TracePluginImpl::checkServiceFilter(TraceServiceConnection* service, bool started)
{
	ReadLockGuard lock(servicesLock, FB_FUNCTION);

	ServiceData* data = NULL;
	ServicesTree::Accessor accessor(&services);
	if (accessor.locate(service->getServiceID()))
		data = &accessor.current();

	if (data && !started)
		return data->enabled;

	const char* svcName = service->getServiceName();
	const int svcNameLen = strlen(svcName);
	bool enabled = true;

	if (config.include_filter.hasData())
	{
		include_matcher->reset();
		include_matcher->process((const UCHAR*) svcName, svcNameLen);
		enabled = include_matcher->result();
	}

	if (enabled && config.exclude_filter.hasData())
	{
		exclude_matcher->reset();
		exclude_matcher->process((const UCHAR*) svcName, svcNameLen);
		enabled = !exclude_matcher->result();
	}

	if (data) {
		data->enabled = enabled;
	}

	return enabled;
}


void TracePluginImpl::log_event_service_attach(TraceServiceConnection* service,
	ntrace_result_t att_result)
{
	if (config.log_services)
	{
		const char* event_type;
		switch (att_result)
		{
			case res_successful:
				event_type = "ATTACH_SERVICE";
				break;
			case res_failed:
				event_type = "FAILED ATTACH_SERVICE";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED ATTACH_SERVICE";
				break;
			default:
				event_type = "Unknown evnt in ATTACH_SERVICE";
				break;
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_start(TraceServiceConnection* service,
	size_t switches_length, const char* switches, ntrace_result_t start_result)
{
	if (config.log_services)
	{
		if (!checkServiceFilter(service, true))
			return;

		const char* event_type;
		switch (start_result)
		{
			case res_successful:
				event_type = "START_SERVICE";
				break;
			case res_failed:
				event_type = "FAILED START_SERVICE";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED START_SERVICE";
				break;
			default:
				event_type = "Unknown event in START_SERVICE";
				break;
		}

		const char* tmp = service->getServiceName();
		if (tmp && *tmp) {
			record.printf("\t\"%s\"" NEWLINE, tmp);
		}

		if (switches_length)
		{
			string sw;
			sw.printf("\t%.*s" NEWLINE, switches_length, switches);

			// Delete terminator symbols from service switches
			for (size_t i = 0; i < sw.length(); ++i)
			{
				if (sw[i] == Firebird::SVC_TRMNTR)
				{
					sw.erase(i, 1);
					if ((i < sw.length()) && (sw[i] != Firebird::SVC_TRMNTR))
						--i;
				}
			}
			record.append(sw);
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_query(TraceServiceConnection* service,
	size_t send_item_length, const ntrace_byte_t* send_items,
	size_t recv_item_length, const ntrace_byte_t* recv_items,
	ntrace_result_t query_result)
{
	if (config.log_services && config.log_service_query)
	{
		if (!checkServiceFilter(service, false))
			return;

		const char* tmp = service->getServiceName();
		if (tmp && *tmp) {
			record.printf("\t\"%s\"" NEWLINE, tmp);
		}
		appendServiceQueryParams(send_item_length, send_items, recv_item_length, recv_items);
		record.append(NEWLINE);

		const char* event_type;
		switch (query_result)
		{
			case res_successful:
				event_type = "QUERY_SERVICE";
				break;
			case res_failed:
				event_type = "FAILED QUERY_SERVICE";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED QUERY_SERVICE";
				break;
			default:
				event_type = "Unknown event in QUERY_SERVICE";
				break;
		}

		logRecordServ(event_type, service);
	}
}

void TracePluginImpl::log_event_service_detach(TraceServiceConnection* service,
	ntrace_result_t detach_result)
{
	if (config.log_services)
	{
		const char* event_type;
		switch (detach_result)
		{
			case res_successful:
				event_type = "DETACH_SERVICE";
				break;
			case res_failed:
				event_type = "FAILED DETACH_SERVICE";
				break;
			case res_unauthorized:
				event_type = "UNAUTHORIZED DETACH_SERVICE";
				break;
			default:
				event_type = "Unknown event in DETACH_SERVICE";
				break;
		}
		logRecordServ(event_type, service);
	}

	// Get rid of connection descriptor
	{
		WriteLockGuard lock(servicesLock, FB_FUNCTION);
		if (services.locate(service->getServiceID()))
		{
			services.current().deallocate_references();
			services.fastRemove();
		}
	}
}

void TracePluginImpl::log_event_trigger_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceTrigger* trigger, bool started, ntrace_result_t trig_result)
{
	if (!config.log_trigger_start && started)
		return;

	if (!config.log_trigger_finish && !started)
		return;

	// Do not log operation if it is below time threshold
	const PerformanceInfo* info = started ? NULL : trigger->getPerf();
	if (config.time_threshold && info && info->pin_time < config.time_threshold)
		return;

	string trgname(trigger->getTriggerName());

	if (trgname.empty())
		trgname = "<unknown>";

	if ((trigger->getWhich() != trg_all) && trigger->getRelationName())
	{
		string relation;
		relation.printf(" FOR %s", trigger->getRelationName());
		trgname.append(relation);
	}

	string action;
	switch (trigger->getWhich())
	{
		case trg_all:
			action = "ON ";
			break;
		case trg_before:
			action = "BEFORE ";
			break;
		case trg_after:
			action = "AFTER ";
			break;
		default:
			action = "<unknown> ";
			break;
	}

	switch (trigger->getAction())
	{
		case jrd_req::req_trigger_insert:
			action.append("INSERT");
			break;
		case jrd_req::req_trigger_update:
			action.append("UPDATE");
			break;
		case jrd_req::req_trigger_delete:
			action.append("DELETE");
			break;
		case jrd_req::req_trigger_connect:
			action.append("CONNECT");
			break;
		case jrd_req::req_trigger_disconnect:
			action.append("DISCONNECT");
			break;
		case jrd_req::req_trigger_trans_start:
			action.append("TRANSACTION_START");
			break;
		case jrd_req::req_trigger_trans_commit:
			action.append("TRANSACTION_COMMIT");
			break;
		case jrd_req::req_trigger_trans_rollback:
			action.append("TRANSACTION_ROLLBACK");
			break;
		case jrd_req::req_trigger_ddl:
			action.append("DDL");
			break;
		default:
			action.append("Unknown trigger action");
			break;
	}

	record.printf("\t%s (%s) " NEWLINE, trgname.c_str(), action.c_str());

	if (info)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type;
	switch (trig_result)
	{
		case res_successful:
			event_type = started ? "EXECUTE_TRIGGER_START" :
								   "EXECUTE_TRIGGER_FINISH";
			break;
		case res_failed:
			event_type = started ? "FAILED EXECUTE_TRIGGER_START" :
								   "FAILED EXECUTE_TRIGGER_FINISH";
			break;
		case res_unauthorized:
			event_type = started ? "UNAUTHORIZED EXECUTE_TRIGGER_START" :
								   "UNAUTHORIZED EXECUTE_TRIGGER_FINISH";
			break;
		default:
			event_type = "Unknown event at executing trigger";
			break;
	}

	logRecordTrans(event_type, connection, transaction);
}

void TracePluginImpl::log_event_error(TraceBaseConnection* connection, TraceStatusVector* status, const char* function)
{
	if (!config.log_errors)
		return;

	string event_type;
	if (status->hasError())
		event_type.printf("ERROR AT %s", function);
	else if (status->hasWarning())
		event_type.printf("WARNING AT %s", function);
	else
		return;

	logRecordError(event_type.c_str(), connection, status);
}

void TracePluginImpl::log_event_sweep(TraceDatabaseConnection* connection, TraceSweepInfo* sweep,
	ntrace_process_state_t sweep_state)
{
	if (!config.log_sweep)
		return;

	if (sweep_state == process_state_started ||
		sweep_state == process_state_finished)
	{
		record.printf("\nTransaction counters:\n"
			"\tOldest interesting %10ld\n"
			"\tOldest active      %10ld\n"
			"\tOldest snapshot    %10ld\n"
			"\tNext transaction   %10ld\n",
			sweep->getOIT(),
			sweep->getOAT(),
			sweep->getOST(),
			sweep->getNext()
			);
	}

	PerformanceInfo* info = sweep->getPerf();
	if (info)
	{
		appendGlobalCounts(info);
		appendTableCounts(info);
	}

	const char* event_type = NULL;
	switch (sweep_state)
	{
	case process_state_started:
		event_type = "SWEEP_START";
		break;

	case process_state_finished:
		event_type = "SWEEP_FINISH";
		break;

	case process_state_failed:
		event_type = "SWEEP_FAILED";
		break;

	case process_state_progress:
		event_type = "SWEEP_PROGRESS";
		break;

	default:
		fb_assert(false);
		event_type = "Unknown SWEEP process state";
		break;
	}

	logRecordConn(event_type, connection);
}

//***************************** PLUGIN INTERFACE ********************************

int TracePluginImpl::release()
{
	if (--refCounter == 0)
	{
		delete this;
		return 0;
	}
	return 1;
}

const char* TracePluginImpl::trace_get_error()
{
	return get_error_string();
}

// Create/close attachment
ntrace_boolean_t TracePluginImpl::trace_attach(TraceDatabaseConnection* connection,
	ntrace_boolean_t create_db, ntrace_result_t att_result)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);

		log_event_attach(connection, create_db, att_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_detach(TraceDatabaseConnection* connection, ntrace_boolean_t drop_db)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);

		log_event_detach(connection, drop_db);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Start/end transaction
ntrace_boolean_t TracePluginImpl::trace_transaction_start(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, size_t tpb_length, const ntrace_byte_t* tpb,
	ntrace_result_t tra_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);

		log_event_transaction_start(connection, transaction, tpb_length, tpb, tra_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_transaction_end(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, ntrace_boolean_t commit, ntrace_boolean_t retain_context,
	ntrace_result_t tra_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);

		log_event_transaction_end(connection, transaction, commit, retain_context, tra_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Assignment to context variables
ntrace_boolean_t TracePluginImpl::trace_set_context(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceContextVariable* variable)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(variable, FB_TRACE_CONTEXT_VARIABLE_VERSION, upInfo);

		log_event_set_context(connection, transaction, variable);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Stored procedure executing
ntrace_boolean_t TracePluginImpl::trace_proc_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceProcedure* procedure,
	bool started, ntrace_result_t proc_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(procedure, FB_TRACE_PROCEDURE_VERSION, upInfo);

		log_event_proc_execute(connection, transaction, procedure, started, proc_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Stored function executing
ntrace_boolean_t TracePluginImpl::trace_func_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceFunction* function,
	bool started, ntrace_result_t func_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(function, FB_TRACE_FUNCTION_VERSION, upInfo);

		log_event_func_execute(connection, transaction, function, started, func_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_trigger_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceTrigger* trigger,
	bool started, ntrace_result_t trig_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(trigger, FB_TRACE_TRIGGER_VERSION, upInfo);

		log_event_trigger_execute(connection, transaction, trigger, started, trig_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}


// DSQL statement lifecycle
ntrace_boolean_t TracePluginImpl::trace_dsql_prepare(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceSQLStatement* statement, ntrace_counter_t time_millis,
	ntrace_result_t req_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(statement, FB_TRACE_SQL_STATEMENT_VERSION, upInfo);

		log_event_dsql_prepare(connection, transaction, statement, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_dsql_free(TraceDatabaseConnection* connection,
	TraceSQLStatement* statement, unsigned short option)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(statement, FB_TRACE_SQL_STATEMENT_VERSION, upInfo);

		log_event_dsql_free(connection, statement, option);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_dsql_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceSQLStatement* statement,
	bool started, ntrace_result_t req_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(statement, FB_TRACE_SQL_STATEMENT_VERSION, upInfo);

		log_event_dsql_execute(connection, transaction, statement, started, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}


// BLR requests
ntrace_boolean_t TracePluginImpl::trace_blr_compile(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceBLRStatement* statement, ntrace_counter_t time_millis,
	ntrace_result_t req_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		if (transaction)
			master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(statement, FB_TRACE_BLR_STATEMENT_VERSION, upInfo);

		log_event_blr_compile(connection, transaction, statement, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_blr_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceBLRStatement* statement, ntrace_result_t req_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(statement, FB_TRACE_BLR_STATEMENT_VERSION, upInfo);

		log_event_blr_execute(connection, transaction, statement, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// DYN requests
ntrace_boolean_t TracePluginImpl::trace_dyn_execute(TraceDatabaseConnection* connection,
	TraceTransaction* transaction, TraceDYNRequest* request, ntrace_counter_t time_millis,
	ntrace_result_t req_result)
{
	try
	{
		MasterInterfacePtr master;
		master->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		master->upgradeInterface(transaction, FB_TRACE_TRANSACTION_VERSION, upInfo);
		master->upgradeInterface(request, FB_TRACE_DYN_REQUEST_VERSION, upInfo);

		log_event_dyn_execute(connection, transaction, request, time_millis, req_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

// Using the services
ntrace_boolean_t TracePluginImpl::trace_service_attach(TraceServiceConnection* service,
	ntrace_result_t att_result)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(service, FB_TRACE_SERVICE_VERSION, upInfo);
		log_event_service_attach(service, att_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_service_start(TraceServiceConnection* service,
	size_t switches_length, const char* switches, ntrace_result_t start_result)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(service, FB_TRACE_SERVICE_VERSION, upInfo);
		log_event_service_start(service, switches_length, switches, start_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_service_query(TraceServiceConnection* service,
	size_t send_item_length, const ntrace_byte_t* send_items, size_t recv_item_length,
	const ntrace_byte_t* recv_items, ntrace_result_t query_result)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(service, FB_TRACE_SERVICE_VERSION, upInfo);
		log_event_service_query(service, send_item_length, send_items,
								recv_item_length, recv_items, query_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}

}

ntrace_boolean_t TracePluginImpl::trace_service_detach(TraceServiceConnection* service,
	ntrace_result_t detach_result)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(service, FB_TRACE_SERVICE_VERSION, upInfo);
		log_event_service_detach(service, detach_result);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_event_error(TraceBaseConnection* connection,
	TraceStatusVector* status, const char* function)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(connection, FB_TRACE_BASE_CONNECTION_VERSION, upInfo);
		log_event_error(connection, status, function);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}

ntrace_boolean_t TracePluginImpl::trace_event_sweep(TraceDatabaseConnection* connection,
	TraceSweepInfo* sweep, ntrace_process_state_t sweep_state)
{
	try
	{
		MasterInterfacePtr()->upgradeInterface(connection, FB_TRACE_CONNECTION_VERSION, upInfo);
		MasterInterfacePtr()->upgradeInterface(sweep, FB_TRACE_SWEEP_INFO_VERSION, upInfo);
		log_event_sweep(connection, sweep, sweep_state);
		return true;
	}
	catch (const Firebird::Exception& ex)
	{
		marshal_exception(ex);
		return false;
	}
}
