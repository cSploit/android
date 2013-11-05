/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceConfigStorage.cpp
 *	DESCRIPTION:	Trace API shared configurations storage
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
#include "../../common/classes/TempFile.h"
#include "../../common/StatusArg.h"
#include "../../common/utils_proto.h"
#include "../../jrd/common.h"
#include "../../jrd/err_proto.h"
#include "../../jrd/isc_proto.h"
#include "../../jrd/isc_s_proto.h"
#include "../../jrd/jrd.h"
#include "../../jrd/os/path_utils.h"
#include "../../jrd/os/config_root.h"
#include "../../jrd/os/os_utils.h"
#include "../../jrd/trace/TraceConfigStorage.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_IO_H
#include <io.h>
#endif
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>

#ifndef O_BINARY
#define O_BINARY 0
#endif

using namespace Firebird;

namespace Jrd {

static const int TOUCH_INTERVAL = 60 * 60;		// in seconds, one hour should be enough

void checkFileError(const char* filename, const char* operation, ISC_STATUS iscError)
{
	if (errno == 0)
		return;

#ifdef WIN_NT
	// we can't use SYS_ERR(errno) on Windows as errno codes is not
	// the same as GetLastError() codes
	const char* strErr = strerror(errno);

	(Arg::Gds(isc_io_error) << Arg::Str(operation) << Arg::Str(filename) <<
		Arg::Gds(iscError) << Arg::Str(strErr)).raise();
#else
	(Arg::Gds(isc_io_error) << Arg::Str(operation) << Arg::Str(filename) <<
		Arg::Gds(iscError) << SYS_ERR(errno)).raise();
#endif
}

ConfigStorage::ConfigStorage() :
	m_base(NULL),
	m_recursive(0),
	m_mutexTID(0),
	m_cfg_file(-1),
	m_dirty(false),
	m_touchSemaphore(FB_NEW(*getDefaultMemoryPool()) AnyRef<Semaphore>),
	m_touchSemRef(*m_touchSemaphore)
{
	PathName filename;
#ifdef WIN_NT
	DWORD sesID = 0;

	typedef BOOL (WINAPI *PFnProcessIdToSessionId) (DWORD, DWORD *);

	HMODULE hmodKernel32 = GetModuleHandle("kernel32.dll");

	PFnProcessIdToSessionId pfnProcessIdToSessionId =
		(PFnProcessIdToSessionId) GetProcAddress(hmodKernel32, "ProcessIdToSessionId");

	if (fb_utils::isGlobalKernelPrefix() ||
		!pfnProcessIdToSessionId ||
		pfnProcessIdToSessionId(GetCurrentProcessId(), &sesID) == 0 ||
		sesID == 0)
	{
		filename.printf(TRACE_FILE); // TODO: it must be per engine instance
	}
	else
	{
		filename.printf("%s.%u", TRACE_FILE, sesID);
	}
#else
	filename.printf(TRACE_FILE); // TODO: it must be per engine instance
#endif

	ISC_STATUS_ARRAY status;
	(void)	// errors are checked indirectly using m_base
		ISC_map_file(status, filename.c_str(), initShMem, this, sizeof(ShMemHeader), &m_handle);
	if (!m_base)
	{
		iscLogStatus("ConfigStorage: Cannot initialize the shared memory region", status);
		status_exception::raise(status);
	}

	fb_assert(m_base->version == 1 || m_base->version == 2);

	StorageGuard guard(this);
	checkFile();
	++m_base->cnt_uses;

	if (m_base->version == 2) 
	{
		if (gds__thread_start(touchThread, (void*) this, THREAD_medium, 0, NULL))
			gds__log("Trace facility: can't start touch thread");
		else
			m_touchStartStop.tryEnter(3);
	}
}

ConfigStorage::~ConfigStorage()
{
	// signal touchThread to finish
	m_touchSemaphore->Semaphore::release();
	m_touchStartStop.tryEnter(3);

	::close(m_cfg_file);
	m_cfg_file = -1;

	{
		StorageGuard guard(this);
		--m_base->cnt_uses;
		if (m_base->cnt_uses == 0)
		{
			unlink(m_base->cfg_file_name);
			memset(m_base->cfg_file_name, 0, sizeof(m_base->cfg_file_name));

			ISC_remove_map_file(&m_handle);
		}
	}

	ISC_STATUS_ARRAY status;
	ISC_unmap_file(status, &m_handle);
}

void ConfigStorage::checkMutex(const TEXT* string, int state)
{
	if (state)
	{
		TEXT msg[BUFFER_TINY];

		sprintf(msg, "ConfigStorage: mutex %s error, status = %d", string, state);
		gds__log(msg);

		fprintf(stderr, "%s\n", msg);
		exit(FINI_ERROR);
	}
}

void ConfigStorage::initShMem(void* arg, sh_mem* shmemData, bool initialize)
{
	ConfigStorage* const storage = (ConfigStorage*) arg;
	fb_assert(storage);

#ifdef WIN_NT
	checkMutex("init", ISC_mutex_init(&storage->m_winMutex, shmemData->sh_mem_name));
	storage->m_mutex = &storage->m_winMutex;
#endif

	ShMemHeader* const header = (ShMemHeader*) shmemData->sh_mem_address;
	storage->m_base = header;

	// Initialize the shared data header
	if (initialize)
	{
		header->version = 2;
		header->change_number = 0;
		header->session_number = 1;
		header->cnt_uses = 0;
		header->touch_time = 0;
		memset(header->cfg_file_name, 0, sizeof(header->cfg_file_name));
#ifndef WIN_NT
		checkMutex("init", ISC_mutex_init(shmemData, &header->mutex, &storage->m_mutex));
	}
	else
	{
		checkMutex("map", ISC_map_mutex(shmemData, &header->mutex, &storage->m_mutex));
#endif
	}
}

void ConfigStorage::checkFile()
{
	if (m_cfg_file >= 0)
		return;

	char* cfg_file_name = m_base->cfg_file_name;

	if (!(*cfg_file_name))
	{
		fb_assert(m_base->cnt_uses == 0);

		char dir[MAXPATHLEN];
		gds__prefix_lock(dir, "");

		PathName filename = TempFile::create("fb_trace_", dir);
		filename.copyTo(cfg_file_name, sizeof(m_base->cfg_file_name));
		m_cfg_file = os_utils::openCreateSharedFile(cfg_file_name, O_BINARY);
	}
	else
	{
		m_cfg_file = ::open(cfg_file_name, O_RDWR | O_BINARY);
	}

	if (m_cfg_file < 0) {
		checkFileError(cfg_file_name, "open", isc_io_open_err);
	}

	// put default (audit) trace file contents into storage
	if (m_base->change_number == 0)
	{
		FILE* cfgFile = NULL;

		try
		{
			PathName configFileName(Config::getAuditTraceConfigFile());

			// remove quotes around path if present
			{ // scope
				const size_t pathLen = configFileName.length();
				if (pathLen > 1 && configFileName[0] == '"' &&
					configFileName[pathLen - 1] == '"')
				{
					configFileName.erase(0, 1);
					configFileName.erase(pathLen - 2, 1);
				}
			}

			if (configFileName.empty())
				return;

			if (PathUtils::isRelative(configFileName))
			{
				PathName root(Config::getRootDirectory());
				PathUtils::ensureSeparator(root);
				configFileName.insert(0, root);
			}

			cfgFile = fopen(configFileName.c_str(), "rb");
			if (!cfgFile) {
				checkFileError(configFileName.c_str(), "fopen", isc_io_open_err);
			}

			TraceSession session(*getDefaultMemoryPool());

			fseek(cfgFile, 0, SEEK_END);
			const long len = ftell(cfgFile);
			if (len)
			{
				fseek(cfgFile, 0, SEEK_SET);
				char* p = session.ses_config.getBuffer(len + 1);

				if (fread(p, 1, len, cfgFile) != size_t(len)) {
					checkFileError(configFileName.c_str(), "fread", isc_io_read_err);
				}
				p[len] = 0;
			}
			else {
				gds__log("Audit configuration file \"%s\" is empty", configFileName.c_str());
			}

			session.ses_user = SYSDBA_USER_NAME;
			session.ses_name = "Firebird Audit";
			session.ses_flags = trs_admin | trs_system;

			addSession(session);
		}
		catch(const Exception& ex)
		{
			ISC_STATUS_ARRAY temp;
			ex.stuff_exception(temp);
			iscLogStatus("Cannot open audit configuration file", temp);
		}

		if (cfgFile) {
			fclose(cfgFile);
		}
	}

	touchFile();
}


void ConfigStorage::touchFile()
{
	os_utils::touchFile(m_base->cfg_file_name);
}


THREAD_ENTRY_DECLARE ConfigStorage::touchThread(THREAD_ENTRY_PARAM arg)
{
	ConfigStorage* storage = (ConfigStorage*) arg;
	storage->touchThreadFunc();

	// release start/stop semaphore only here to avoid problems 
	// with dtors of local varoables in touchThreadFunc()
	storage->m_touchStartStop.release();
	return 0;
}


void ConfigStorage::touchThreadFunc()
{
	AnyRef<Semaphore>* semaphore = m_touchSemaphore;
	Reference semRef(*semaphore);

	m_touchStartStop.release();

	int delay = TOUCH_INTERVAL / 2;
	while (!semaphore->tryEnter(delay))
	{
		StorageGuard guard(this);

		time_t now;
		time(&now);

		if (!m_base->touch_time || m_base->touch_time < now)
		{
			touchFile();
			m_base->touch_time = now + TOUCH_INTERVAL;
		}

		delay = difftime(m_base->touch_time, now);
	} 
}


void ConfigStorage::acquire()
{
	fb_assert(m_recursive >= 0);
	const FB_THREAD_ID currTID = getThreadId();

	if (m_mutexTID == currTID)
		m_recursive++;
	else
	{
		checkMutex("lock", ISC_mutex_lock(m_mutex));

		fb_assert(m_recursive == 0);
		m_recursive = 1;

		fb_assert(m_mutexTID == 0);
		m_mutexTID = currTID;
	}
}

void ConfigStorage::release()
{
	fb_assert(m_recursive > 0);

	const FB_THREAD_ID currTID = getThreadId();
	fb_assert(m_mutexTID == currTID);
	
	if (--m_recursive == 0)
	{
		checkDirty();
		m_mutexTID = 0;
		checkMutex("unlock", ISC_mutex_unlock(m_mutex));
	}
}

void ConfigStorage::addSession(TraceSession& session)
{
	setDirty();
	session.ses_id = m_base->session_number++;
	session.ses_flags |= trs_active;
	time(&session.ses_start);

	const long pos1 = lseek(m_cfg_file, 0, SEEK_END);
	if (pos1 < 0)
	{
		const char* fn = m_base->cfg_file_name;
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("lseek") << Arg::Str(fn) <<
			Arg::Gds(isc_io_read_err) << SYS_ERR(errno));
	}

	putItem(tagID, sizeof(session.ses_id), &session.ses_id);
	if (!session.ses_name.empty()) {
		putItem(tagName, session.ses_name.length(), session.ses_name.c_str());
	}
	putItem(tagUserName, session.ses_user.length(), session.ses_user.c_str());
	putItem(tagFlags, sizeof(session.ses_flags), &session.ses_flags);
	putItem(tagConfig, session.ses_config.length(), session.ses_config.c_str());
	putItem(tagStartTS, sizeof(session.ses_start), &session.ses_start);
	if (!session.ses_logfile.empty()) {
		putItem(tagLogFile, session.ses_logfile.length(), session.ses_logfile.c_str());
	}
	putItem(tagEnd, 0, NULL);

	// const long pos2 = lseek(m_cfg_file, 0, SEEK_END);
	// m_base->used_space += pos2 - pos1;
}

bool ConfigStorage::getNextSession(TraceSession& session)
{
	ITEM tag = tagID;
	ULONG len;
	session.clear();

	while (true)
	{
		if (!getItemLength(tag, len))
			return false;

		if (tag == tagEnd)
		{
			if (session.ses_id != 0)
				return true;

			continue;
		}

		void* p = NULL;

		switch (tag)
		{
			case tagID:
				fb_assert(len == sizeof(session.ses_id));
				p = &session.ses_id;
				break;

			case tagName:
				if (session.ses_id)
					p = session.ses_name.getBuffer(len);
				break;

			case tagUserName:
				if (session.ses_id)
					p = session.ses_user.getBuffer(len);
				break;

			case tagFlags:
				fb_assert(len == sizeof(session.ses_flags));
				if (session.ses_id)
					p = &session.ses_flags;
				break;

			case tagConfig:
				if (session.ses_id)
					p = session.ses_config.getBuffer(len);
				break;

			case tagStartTS:
				fb_assert(len == sizeof(session.ses_start));
				if (session.ses_id)
					p = &session.ses_start;
				break;

			case tagLogFile:
				if (session.ses_id)
					p = session.ses_logfile.getBuffer(len);
				break;

			default:
				fb_assert(false);
		}

		if (p)
		{
			if (::read(m_cfg_file, p, len) != len)
				checkFileError(m_base->cfg_file_name, "read", isc_io_read_err);
		}
		else
		{
			if (lseek(m_cfg_file, len, SEEK_CUR) < 0)
				checkFileError(m_base->cfg_file_name, "lseek", isc_io_read_err);
		}
	}

	return true;
}

void ConfigStorage::removeSession(ULONG id)
{
	ITEM tag = tagID;
	ULONG len;

	restart();
	while (true)
	{
		if (!getItemLength(tag, len))
			return;

		if (tag == tagID)
		{
			ULONG currID;
			fb_assert(len == sizeof(currID));

			bool err = (::read(m_cfg_file, &currID, len) != len);
			if (!err && currID == id)
			{
				setDirty();

				currID = 0;
				// Do not delete this temporary signed var, otherwise we get
				// warning C4146: unary minus operator applied to unsigned type, result still unsigned
				// but we need a negative offset here.
				const long local_len = len;
				if (lseek(m_cfg_file, -local_len, SEEK_CUR) < 0)
					checkFileError(m_base->cfg_file_name, "lseek", isc_io_read_err);

				if (write(m_cfg_file, &currID, len) != len)
					checkFileError(m_base->cfg_file_name, "write", isc_io_write_err);

				break;
			}
		}
		else
		{
			if (lseek(m_cfg_file, len, SEEK_CUR) < 0)
				checkFileError(m_base->cfg_file_name, "lseek", isc_io_read_err);
		}
	}
}


void ConfigStorage::restart()
{
	checkDirty();

	if (lseek(m_cfg_file, 0, SEEK_SET) < 0)
		checkFileError(m_base->cfg_file_name, "lseek", isc_io_read_err);
}


void ConfigStorage::updateSession(TraceSession& session)
{
	restart();

	ITEM tag;
	ULONG len;
	ULONG currID = 0;

	while (true)
	{
		if (!getItemLength(tag, len))
			return;

		void* p = NULL;
		switch (tag)
		{
			case tagID:
				fb_assert(len == sizeof(currID));
				read(m_cfg_file, &currID, len);
				continue;

			case tagFlags:
				fb_assert(len == sizeof(session.ses_flags));
				if (currID == session.ses_id)
					p = &session.ses_flags;
				break;

			case tagEnd:
				if (currID == session.ses_id)
					return;
				len = 0;
				break;
		}

		if (p)
		{
			setDirty();
			if (write(m_cfg_file, p, len) != len)
				checkFileError(m_base->cfg_file_name, "write", isc_io_write_err);
		}
		else if (len)
		{
			if (lseek(m_cfg_file, len, SEEK_CUR) < 0)
				checkFileError(m_base->cfg_file_name, "lseek", isc_io_read_err);
		}
	}
}


void ConfigStorage::putItem(ITEM tag, ULONG len, const void* data)
{
	const char tag_data = (char) tag;
	ULONG to_write = sizeof(tag_data);
	if (write(m_cfg_file, &tag_data, to_write) != to_write)
		checkFileError(m_base->cfg_file_name, "write", isc_io_write_err);

	if (tag == tagEnd)
		return;

	to_write = sizeof(len);
	if (write(m_cfg_file, &len, to_write) != to_write)
		checkFileError(m_base->cfg_file_name, "write", isc_io_write_err);

	if (len)
	{
		if (write(m_cfg_file, data, len) != len)
			checkFileError(m_base->cfg_file_name, "write", isc_io_write_err);
	}
}

bool ConfigStorage::getItemLength(ITEM& tag, ULONG& len)
{
	char data;
	const int cnt_read = read(m_cfg_file, &data, sizeof(data));

	if (cnt_read == 0)
		return false;

	if (cnt_read < 0)
		checkFileError(m_base->cfg_file_name, "read", isc_io_read_err);

	tag = (ITEM) data;

	if (tag == tagEnd)
		len = 0;
	else
	{
		if (read(m_cfg_file, &len, sizeof(ULONG)) != sizeof(ULONG))
			checkFileError(m_base->cfg_file_name, "read", isc_io_read_err);
	}

	return true;
}

} // namespace Jrd
