/*
 *	MODULE:		TraceService.cpp
 *	DESCRIPTION:
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
 *  The Original Code was created by Khorsun Vladyslav
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2008 Khorsun Vladyslav <hvlad@users.sourceforge.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */

#include "firebird.h"
#include "consts_pub.h"
#include "fb_exception.h"
#include "iberror.h"
#include "../../jrd/ibase.h"
#include "../../common/classes/fb_string.h"
#include "../../common/classes/timestamp.h"
#include "../../common/config/config.h"
#include "../../common/StatusArg.h"
#include "../../common/thd.h"
#include "../../jrd/svc.h"
#include "../../jrd/os/guid.h"
#include "../../jrd/trace/TraceLog.h"
#include "../../jrd/trace/TraceManager.h"
#include "../../jrd/trace/TraceService.h"

using namespace Firebird;
using namespace Jrd;


class TraceSvcJrd : public TraceSvcIntf
{
public:
	explicit TraceSvcJrd(Service& svc) :
		m_svc(svc),
		m_admin(false),
		m_chg_number(0)
	{};

	virtual ~TraceSvcJrd() {};

	virtual void setAttachInfo(const string& service_name, const string& user,
		const string& pwd, bool isAdmin);

	virtual void startSession(TraceSession& session, bool interactive);
	virtual void stopSession(ULONG id);
	virtual void setActive(ULONG id, bool active);
	virtual void listSessions();

private:
	void readSession(TraceSession& session);
	bool changeFlags(ULONG id, int setFlags, int clearFlags);
	bool checkAliveAndFlags(ULONG sesId, int& flags);

	Service& m_svc;
	string m_user;
	bool   m_admin;
	ULONG  m_chg_number;
};

void TraceSvcJrd::setAttachInfo(const string& /*service_name*/, const string& user,
	const string& /*pwd*/, bool isAdmin)
{
	m_user = user;
	m_admin = isAdmin || (m_user == SYSDBA_USER_NAME);
}

void TraceSvcJrd::startSession(TraceSession& session, bool interactive)
{
	if (!TraceManager::pluginsCount())
	{
		m_svc.printf(false, "Can not start trace session. There are no trace plugins loaded\n");
		return;
	}
	
	ConfigStorage* storage = TraceManager::getStorage();

	{	// scope
		StorageGuard guard(storage);

		session.ses_user = m_user;

		session.ses_flags = trs_active;
		if (m_admin) {
			session.ses_flags |= trs_admin;
		}

		if (interactive)
		{
			FB_GUID guid;
			GenerateGuid(&guid);

			char* buff = session.ses_logfile.getBuffer(GUID_BUFF_SIZE);
			GuidToString(buff, &guid);

			session.ses_logfile.insert(0, "fb_trace.");
		}

		storage->addSession(session);
		m_chg_number = storage->getChangeNumber();
	}

	m_svc.started();
	m_svc.printf(false, "Trace session ID %ld started\n", session.ses_id);

	if (interactive)
	{
		readSession(session);
		{
			StorageGuard guard(storage);
			storage->removeSession(session.ses_id);
		}
	}
}

void TraceSvcJrd::stopSession(ULONG id)
{
	m_svc.started();

	ConfigStorage* storage = TraceManager::getStorage();
	StorageGuard guard(storage);

	storage->restart();

	TraceSession session(*getDefaultMemoryPool());
	while (storage->getNextSession(session))
	{
		if (id != session.ses_id)
			continue;

		if (m_admin || m_user == session.ses_user)
		{
			storage->removeSession(id);
			m_svc.printf(false, "Trace session ID %ld stopped\n", id);
		}
		else
			m_svc.printf(false, "No permissions to stop other user trace session\n");

		return;
	}

	m_svc.printf(false, "Trace session ID %d not found\n", id);
}

void TraceSvcJrd::setActive(ULONG id, bool active)
{
	if (active)
	{
		if (changeFlags(id, trs_active, 0)) {
			m_svc.printf(false, "Trace session ID %ld resumed\n", id);
		}
	}
	else
	{
		if (changeFlags(id, 0, trs_active)) {
			m_svc.printf(false, "Trace session ID %ld paused\n", id);
		}
	}
}

bool TraceSvcJrd::changeFlags(ULONG id, int setFlags, int clearFlags)
{
	ConfigStorage* storage = TraceManager::getStorage();
	StorageGuard guard(storage);

	storage->restart();

	TraceSession session(*getDefaultMemoryPool());
	while (storage->getNextSession(session))
	{
		if (id != session.ses_id)
			continue;

		if (m_admin || m_user == session.ses_user)
		{
			const int saveFlags = session.ses_flags;

			session.ses_flags |= setFlags;
			session.ses_flags &= ~clearFlags;

			if (saveFlags != session.ses_flags) {
				storage->updateSession(session);
			}

			return true;
		}

		m_svc.printf(false, "No permissions to change other user trace session\n");
		return false;
	}

	m_svc.printf(false, "Trace session ID %d not found\n", id);
	return false;
}

void TraceSvcJrd::listSessions()
{
	m_svc.started();

	ConfigStorage* storage = TraceManager::getStorage();
	StorageGuard guard(storage);

	storage->restart();

	TraceSession session(*getDefaultMemoryPool());
	while (storage->getNextSession(session))
	{
		if (m_admin || m_user == session.ses_user)
		{
			m_svc.printf(false, "\nSession ID: %d\n", session.ses_id);
			if (!session.ses_name.empty()) {
				m_svc.printf(false, "  name:  %s\n", session.ses_name.c_str());
			}
			m_svc.printf(false, "  user:  %s\n", session.ses_user.c_str());

			struct tm* t = localtime(&session.ses_start);
			m_svc.printf(false, "  date:  %04d-%02d-%02d %02d:%02d:%02d\n",
					t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
					t->tm_hour, t->tm_min, t->tm_sec);

			string flags;
			if (session.ses_flags & trs_active) {
				flags = "active";
			}
			else {
				flags = "suspend";
			}
			if (session.ses_flags & trs_admin) {
				flags += ", admin";
			}
			if (session.ses_flags & trs_system) {
				flags += ", system";
			}
			if (session.ses_logfile.empty()) {
				flags += ", audit";
			}
			else {
				flags += ", trace";
			}
			if (session.ses_flags & trs_log_full) {
				flags += ", log full";
			}
			m_svc.printf(false, "  flags: %s\n", flags.c_str());
		}
	}
}

void TraceSvcJrd::readSession(TraceSession& session)
{
	const size_t maxLogSize = Config::getMaxUserTraceLogSize(); // in MB

	if (session.ses_logfile.empty())
	{
		m_svc.printf(false, "Can't open trace data log file");
		return;
	}

	MemoryPool& pool = *getDefaultMemoryPool();
	AutoPtr<TraceLog> log(FB_NEW(pool) TraceLog(pool, session.ses_logfile, true));

	UCHAR buff[1024];
	int flags = session.ses_flags;
	while (!m_svc.finished() && checkAliveAndFlags(session.ses_id, flags))
	{
		const size_t len = log->read(buff, sizeof(buff));
		if (!len)
		{
			if (!checkAliveAndFlags(session.ses_id, flags))
				break;

			if (m_svc.svc_detach_sem.tryEnter(0, 250))
				break;
		}
		else
		{
			m_svc.putBytes(buff, len);

			const bool logFull = (flags & trs_log_full);
			if (logFull && log->getApproxLogSize() <= maxLogSize)
			{
				// resume session
				changeFlags(session.ses_id, 0, trs_log_full);
			}
		}
	}
}

bool TraceSvcJrd::checkAliveAndFlags(ULONG sesId, int& flags)
{
	ConfigStorage* storage = TraceManager::getStorage();

	bool alive = (m_chg_number == storage->getChangeNumber());
	if (!alive)
	{
		// look if our session still alive
		StorageGuard guard(storage);

		TraceSession readSession(*getDefaultMemoryPool());
		storage->restart();
		while (storage->getNextSession(readSession))
		{
			if (readSession.ses_id == sesId)
			{
				alive = true;
				flags = readSession.ses_flags;
				break;
			}
		}

		m_chg_number = storage->getChangeNumber();
	}

	return alive;
}


// service entrypoint
THREAD_ENTRY_DECLARE TRACE_main(THREAD_ENTRY_PARAM arg)
{
	Service* svc = (Service*) arg;
	int exit_code = FB_SUCCESS;

	TraceSvcJrd traceSvc(*svc);
	try
	{
		fbtrace(svc, &traceSvc);
	}
	catch (const Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		svc->initStatus();
		svc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	svc->started();
	svc->finish();

	return (THREAD_ENTRY_RETURN)(IPTR) exit_code;
}
