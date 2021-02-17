/*
 *	PROGRAM:	Firebird Trace Services
 *	MODULE:		TraceConfigStorage.h
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

#ifndef JRD_TRACECONFIGSTORAGE_H
#define JRD_TRACECONFIGSTORAGE_H

#include "../../common/classes/array.h"
#include "../../common/classes/fb_string.h"
#include "../../common/classes/init.h"
#include "../../common/isc_s_proto.h"
#include "../../jrd/trace/TraceSession.h"
#include "../../common/classes/RefCounted.h"

namespace Jrd {

struct TraceCSHeader : public Firebird::MemoryHeader
{
	volatile ULONG change_number;
	volatile ULONG session_number;
	ULONG cnt_uses;
	char cfg_file_name[MAXPATHLEN];
};

class ConfigStorage FB_FINAL : public Firebird::GlobalStorage, public Firebird::IpcObject, public Firebird::Reasons
{
public:
	ConfigStorage();
	~ConfigStorage();

	void addSession(Firebird::TraceSession& session);
	bool getNextSession(Firebird::TraceSession& session);
	void removeSession(ULONG id);
	void restart();
	void updateSession(Firebird::TraceSession& session);

	ULONG getChangeNumber() const
	{ return m_sharedMemory && m_sharedMemory->getHeader() ? m_sharedMemory->getHeader()->change_number : 0; }

	void acquire();
	void release();

private:
	void mutexBug(int osErrorCode, const char* text);
	bool initialize(Firebird::SharedMemoryBase*, bool);

	void checkFile();
	void touchFile();

	class TouchFile FB_FINAL : public Firebird::RefCntIface<Firebird::ITimer, FB_TIMER_VERSION>
	{
	public:
		void FB_CARG handler();
		void start(const char* fName);
		void stop();
		int FB_CARG release();
	private:
		const char* fileName;
	};
	Firebird::RefPtr<TouchFile> m_timer;

	void checkDirty()
	{
		if (m_dirty)
		{
			//_commit(m_cfg_file);
			m_dirty = false;
		}
	}

	void setDirty()
	{
		if (!m_dirty)
		{
			if (m_sharedMemory && m_sharedMemory->getHeader())
				m_sharedMemory->getHeader()->change_number++;
			m_dirty = true;
		}
	}

	// items in every session record at sessions file
	enum ITEM
	{
		tagID = 1,			// session ID
		tagName,			// session Name
		tagUserName,		// creator user name
		tagFlags,			// session flags
		tagConfig,			// configuration
		tagStartTS,			// date+time when started
		tagLogFile,			// log file name, if any
		tagEnd
	};

	void putItem(ITEM tag, ULONG len, const void* data);
	bool getItemLength(ITEM& tag, ULONG& len);

	Firebird::AutoPtr<Firebird::SharedMemory<TraceCSHeader> > m_sharedMemory;
	int m_recursive;
	ThreadId m_mutexTID;
	int m_cfg_file;
	bool m_dirty;
};


class StorageInstance
{
private:
	Firebird::Mutex initMtx;
	ConfigStorage* storage;

public:
	explicit StorageInstance(Firebird::MemoryPool&) :
		storage(NULL)
	{}

	~StorageInstance()
	{
		delete storage;
	}

	ConfigStorage* getStorage()
	{
		if (!storage)
		{
			Firebird::MutexLockGuard guard(initMtx, FB_FUNCTION);
			if (!storage)
			{
				storage = new ConfigStorage;
			}
		}
		return storage;
	}
};



class StorageGuard
{
public:
	explicit StorageGuard(ConfigStorage* storage) :
		m_storage(storage)
	{
		m_storage->acquire();
	}

	~StorageGuard()
	{
		m_storage->release();
	}
private:
	ConfigStorage* m_storage;
};

}

#endif
