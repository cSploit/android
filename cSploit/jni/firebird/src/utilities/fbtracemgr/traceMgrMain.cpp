/*
 *	PROGRAM:		Firebird utilities
 *	MODULE:			traceMgrMain.cpp
 *	DESCRIPTION:	Trace Manager utility
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
 *  The Original Code was created by Vladyslav Khorsun
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Vladyslav Khorsun <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#include "firebird.h"
#include <signal.h>

#include "../../common/classes/auto.h"
#include "../../common/classes/ClumpletWriter.h"
#include "../../common/utils_proto.h"
#include "../../jrd/trace/TraceService.h"
#include "../../jrd/ibase.h"

namespace Firebird {

class TraceSvcUtil : public TraceSvcIntf
{
public:
	TraceSvcUtil();
	virtual ~TraceSvcUtil();

	virtual void setAttachInfo(const string& service_name, const string& user,
		const string& pwd, bool isAdmin);

	virtual void startSession(TraceSession& session, bool interactive);
	virtual void stopSession(ULONG id);
	virtual void setActive(ULONG id, bool active);
	virtual void listSessions();

	static void stopRead();

private:
	void runService(size_t spbSize, const UCHAR* spb);

	isc_svc_handle m_svcHandle;
	static bool m_stop;
};


const int MAXBUF = 16384;

bool TraceSvcUtil::m_stop = true;

TraceSvcUtil::TraceSvcUtil()
{
	m_svcHandle = 0;
}

TraceSvcUtil::~TraceSvcUtil()
{
	if (m_svcHandle)
	{
		ISC_STATUS_ARRAY status = {0};
		isc_service_detach(status, &m_svcHandle);
	}
}

void TraceSvcUtil::setAttachInfo(const string& service_name, const string& /*user*/,
	const string& /*pwd*/, bool isAdmin)
{
	ISC_STATUS_ARRAY status = {0};

	ClumpletWriter spb(ClumpletWriter::spbList, MAXBUF);

	if (isAdmin) {
		spb.insertTag(isc_spb_trusted_auth);
	}

	if (isc_service_attach(status, 0, service_name.c_str(), &m_svcHandle,
			static_cast<USHORT>(spb.getBufferLength()),
			reinterpret_cast<const char*>(spb.getBuffer())))
	{
		status_exception::raise(status);
	}
}

void TraceSvcUtil::startSession(TraceSession& session, bool /*interactive*/)
{
	m_stop = false;

	HalfStaticArray<UCHAR, 1024> buff(*getDefaultMemoryPool());
	UCHAR* p = NULL;
	long len = 0;

	FILE* file = NULL;

	try
	{
		const char* fileName = session.ses_config.c_str();
		file = fopen(fileName, "rb");
		if (!file)
		{
			(Arg::Gds(isc_io_error) << Arg::Str("fopen") << Arg::Str(fileName) <<
				Arg::Gds(isc_io_open_err) << Arg::OsError()).raise();
		}

		fseek(file, 0, SEEK_END);
		len = ftell(file);
		if (len == 0)
		{
			(Arg::Gds(isc_io_error) << Arg::Str("fread") << Arg::Str(fileName) <<
				Arg::Gds(isc_io_read_err) << Arg::OsError()).raise();
		}

		fseek(file, 0, SEEK_SET);
		p = buff.getBuffer(len);

		if (fread(p, 1, len, file) != size_t(len))
		{
			(Arg::Gds(isc_io_error) << Arg::Str("fread") << Arg::Str(fileName) <<
				Arg::Gds(isc_io_read_err) << Arg::OsError()).raise();
		}
		fclose(file);
	}
	catch (const Exception&)
	{
		if (file)
			fclose(file);

		throw;
	}

	ClumpletWriter spb(ClumpletWriter::SpbStart, MAXBUF);

	spb.insertTag(isc_action_svc_trace_start);
	spb.insertBytes(isc_spb_trc_cfg, p, len);

	if (session.ses_name.hasData())
	{
		spb.insertBytes(isc_spb_trc_name,
			reinterpret_cast<const UCHAR*> (session.ses_name.c_str()),
			session.ses_name.length());
	}

	runService(spb.getBufferLength(), spb.getBuffer());
}

void TraceSvcUtil::stopSession(ULONG id)
{
	ClumpletWriter spb(ClumpletWriter::SpbStart, MAXBUF);

	spb.insertTag(isc_action_svc_trace_stop);
	spb.insertInt(isc_spb_trc_id, id);

	runService(spb.getBufferLength(), spb.getBuffer());
}

void TraceSvcUtil::setActive(ULONG id, bool active)
{
	ClumpletWriter spb(ClumpletWriter::SpbStart, MAXBUF);

	spb.insertTag(active ? isc_action_svc_trace_resume : isc_action_svc_trace_suspend);
	spb.insertInt(isc_spb_trc_id, id);

	runService(spb.getBufferLength(), spb.getBuffer());
}

void TraceSvcUtil::listSessions()
{
	ClumpletWriter spb(ClumpletWriter::SpbStart, MAXBUF);

	spb.insertTag(isc_action_svc_trace_list);

	runService(spb.getBufferLength(), spb.getBuffer());
}

void TraceSvcUtil::stopRead()
{
	m_stop = true;
}

void TraceSvcUtil::runService(size_t spbSize, const UCHAR* spb)
{
	ISC_STATUS_ARRAY status;

	if (isc_service_start(status, &m_svcHandle, 0,
			static_cast<USHORT>(spbSize),
			reinterpret_cast<const char*>(spb)))
	{
		status_exception::raise(status);
	}

	const char query[] = {isc_info_svc_to_eof, isc_info_end};

	// use one second timeout to poll service
	char send[16];
	char* p = send;
	*p++ = isc_info_svc_timeout;
	ADD_SPB_LENGTH(p, 4);
	ADD_SPB_NUMERIC(p, 1);
	*p++ = isc_info_end;

	const USHORT sendSize = (p - send);

	char results[MAXBUF];
	bool noData;
	do
	{
		if (isc_service_query(status, &m_svcHandle, 0,
				sendSize, send,
				sizeof(query), query,
				sizeof(results) - 1, results))
		{
			status_exception::raise(status);
		}

		p = results;
		bool ignoreTruncation = false;
		bool dirty = false;
		noData = true;

		while (*p != isc_info_end)
		{
			const UCHAR item = *p++;
			switch (item)
			{
			case isc_info_svc_to_eof:
				ignoreTruncation = true;

			case isc_info_svc_line:
				{
					const unsigned short l = isc_vax_integer(p, sizeof(l));
					p += sizeof(l);
					if (l)
					{
						const char ch = p[l];
						p[l] = 0;
						fprintf(stdout, "%s", p);
						p[l] = ch;
						p += l;
						dirty = true;
					}
					noData = (l == 0);
				}
				break;

			case isc_info_truncated:
				if (!ignoreTruncation)
					return;
				break;

			case isc_info_svc_timeout:
			case isc_info_data_not_ready:
				noData = false;
				if (dirty)
				{
					fflush(stdout);
					dirty = false;
				}
				break;

			default:
				status_exception::raise(Arg::Gds(isc_fbsvcmgr_query_err) <<
										Arg::Num(static_cast<unsigned char>(p[-1])));
			}
		}
	} while (!(m_stop || noData));
}

} // namespace Firebird


using namespace Firebird;


typedef void (*SignalHandlerPointer)(int);

static SignalHandlerPointer prevCtrlCHandler = NULL;
static bool terminated = false;

static void ctrl_c_handler(int signal)
{
	if (signal == SIGINT)
		TraceSvcUtil::stopRead();

	if (prevCtrlCHandler)
		prevCtrlCHandler(signal);
}


int CLIB_ROUTINE main(int argc, char* argv[])
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Invoke real trace main function
 *
 **************************************/

	prevCtrlCHandler = signal(SIGINT, ctrl_c_handler);

	AutoPtr<UtilSvc> uSvc(UtilSvc::createStandalone(argc, argv));
	try
	{
		TraceSvcUtil traceUtil;

 		fbtrace(uSvc, &traceUtil);
	}
	catch (const Firebird::Exception& ex)
	{
		ISC_STATUS_ARRAY temp;

		ex.stuff_exception(temp);
		isc_print_status(temp);

		return FINI_ERROR;
	}

	return FINI_OK;
}
