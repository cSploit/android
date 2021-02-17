/*
 *	PROGRAM:	Client/Server Common Code
 *	MODULE:		fbsyslog.h
 *	DESCRIPTION:	System log facility (win32)
 *
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
 * Created by: Alex Peshkov <peshkoff@mail.ru>
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "../common/os/fbsyslog.h"
#include "../common/classes/init.h"

#include <Windows.h>

namespace {

typedef HANDLE WINAPI tRegisterEventSource(LPCTSTR lpUNCServerName, LPCTSTR lpSourceName);

typedef BOOL WINAPI tReportEvent(
		HANDLE hEventLog,
		WORD wType,
		WORD wCategory,
		DWORD dwEventID,
		PSID lpUserSid,
		WORD wNumStrings,
		DWORD dwDataSize,
		LPCTSTR *lpStrings,
		LPVOID lpRawData);

class SyslogAccess
{
private:
	CRITICAL_SECTION cs;
	HANDLE LogHandle;
	tReportEvent *fReportEvent;
	bool InitFlag;
public:
	explicit SyslogAccess(Firebird::MemoryPool&)
	{
		InitializeCriticalSection(&cs);
		InitFlag = false;
		LogHandle = 0;
	}
	~SyslogAccess()
	{
		DeleteCriticalSection(&cs);
	}
	void Record(WORD wType, const char* msg);
};

void SyslogAccess::Record(WORD wType, const char* msg)
{
	EnterCriticalSection(&cs);
	if (! InitFlag) {
		InitFlag = true;
		HINSTANCE hLib = LoadLibrary("Advapi32");
		tRegisterEventSource *fRegisterEventSource = hLib ?
			(tRegisterEventSource *) GetProcAddress(hLib, "RegisterEventSourceA") : 0;
		fReportEvent = hLib ?
			(tReportEvent *) GetProcAddress(hLib, "ReportEventA") : 0;
		LogHandle = fRegisterEventSource && fReportEvent ?
			fRegisterEventSource(0, "Firebird SQL Server") : 0;
	}
	bool use9x = true;
	if (LogHandle) {
		LPCTSTR sb[1];
		sb[0] = msg;
		if (fReportEvent(LogHandle, wType, 0, 0, 0, 1, 0, sb, 0)) {
			use9x = false;
		}
	}
	if (use9x) {
		::MessageBox(0, msg, "Firebird Error", MB_ICONSTOP);
	}
	LeaveCriticalSection(&cs);
}

Firebird::InitInstance<SyslogAccess> iSyslogAccess;

} // namespace

namespace Firebird {

void Syslog::Record(Severity level, const char* msg)
{
	WORD wType = EVENTLOG_ERROR_TYPE;
	switch (level)
	{
	case Warning:
		wType = EVENTLOG_INFORMATION_TYPE;
		break;
	case Error:
	default:
		wType = EVENTLOG_ERROR_TYPE;
		break;
	}
	iSyslogAccess().Record(wType, msg);
}

} // namespace Firebird
