/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_s_proto.h
 *	DESCRIPTION:	Prototype header file for isc_sync.cpp
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
 * The Original Code was created by Inprise Corporation
 * and its predecessors. Portions created by Inprise Corporation are
 * Copyright (C) Inprise Corporation.
 *
 * All Rights Reserved.
 * Contributor(s): ______________________________________.
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_ISC_S_PROTO_H
#define JRD_ISC_S_PROTO_H

#include "../common/classes/alloc.h"
#include "../common/classes/RefCounted.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/timestamp.h"

// Firebird platform-specific synchronization data structures

/*
#if defined(DARWIN) || defined(ANDROID)
#define USE_FILELOCKS
#endif
*/

#if defined(DARWIN)
#define USE_FILELOCKS
#endif

#if defined(FREEBSD)
#define USE_SYS5SEMAPHORE
#endif

#ifdef LINUX
// This hack fixes CORE-2896 - embedded connections fail on linux.
// Looks like a lot of linux kernels are buggy when working with PRIO_INHERIT mutexes.
//#undef HAVE_PTHREAD_MUTEXATTR_SETPROTOCOL
#endif


#if defined(USE_FILELOCKS) && (!defined(USE_POSIX_SEMAPHORE))
#define USE_SYS5SEMAPHORE
#endif

#if defined(HAVE_MMAP) || defined(WIN_NT)
#define HAVE_OBJECT_MAP
#endif

#if defined(HAVE_MMAP) && (!defined(USE_SYS5SEMAPHORE))
#define USE_MUTEX_MAP
#endif

#ifndef USE_POSIX_SEMAPHORE
#ifndef USE_FILELOCKS
#ifndef USE_SYS5SEMAPHORE
#define USE_SHARED_FUTEX
#endif
#endif
#endif

#ifdef UNIX

#if defined(USE_SHARED_FUTEX)
#if defined(HAVE_PTHREAD_MUTEXATTR_SETROBUST_NP) && defined(HAVE_PTHREAD_MUTEX_CONSISTENT_NP)
#define USE_ROBUST_MUTEX
#endif // ROBUST mutex

#include "fb_pthread.h"
#endif // USE_SHARED_FUTEX

#ifndef USE_FILELOCKS
#define HAVE_SHARED_MUTEX_SECTION
#endif

namespace Firebird {

#ifdef USE_SYS5SEMAPHORE

struct Sys5Semaphore
{
	int semSet;				// index in shared memory table
	unsigned short semNum;	// number in semset
	int getId();
};

#ifndef USE_FILELOCKS
struct mtx : public Sys5Semaphore
{
};
#endif

struct event_t : public Sys5Semaphore
{
	SLONG event_count;
};

#endif // USE_SYS5SEMAPHORE

#ifdef USE_SHARED_FUTEX

struct mtx
{
	pthread_mutex_t mtx_mutex[1];
};

struct event_t
{
	SLONG event_count;
	int pid;
	pthread_mutex_t event_mutex[1];
	pthread_cond_t event_cond[1];
};

#endif // USE_SHARED_FUTEX

#endif // UNIX


#ifdef WIN_NT
#include <windows.h>

namespace Firebird {

struct FAST_MUTEX_SHARED_SECTION
{
	SLONG fInitialized;
	SLONG lSpinLock;
	SLONG lThreadsWaiting;
	SLONG lAvailable;
	SLONG lOwnerPID;
#ifdef DEV_BUILD
	SLONG lThreadId;
#endif
};

struct FAST_MUTEX
{
	HANDLE hEvent;
	HANDLE hFileMap;
	SLONG lSpinCount;
	volatile FAST_MUTEX_SHARED_SECTION* lpSharedInfo;
};

struct mtx
{
	FAST_MUTEX mtx_fast;
};

struct event_t
{
	SLONG event_pid;
	SLONG event_id;
	SLONG event_count;
	void* event_handle;
};

#endif // WIN_NT

class MemoryHeader
{
public:
	USHORT mhb_type;
	USHORT mhb_version;
	GDS_TIMESTAMP mhb_timestamp;
#ifdef HAVE_SHARED_MUTEX_SECTION
	struct mtx mhb_mutex;
#endif
};


#ifdef UNIX

#if defined(USE_FILELOCKS) || (!defined(HAVE_FLOCK))
#define USE_FCNTL
#endif

class CountedFd;

class FileLock
{
public:
	enum LockMode {FLM_EXCLUSIVE, FLM_TRY_EXCLUSIVE, FLM_SHARED, FLM_TRY_SHARED};

	typedef void InitFunction(int fd);
	explicit FileLock(const char* fileName, InitFunction* init = NULL);		// main ctor
	FileLock(const FileLock* main, int s);	// creates additional lock for existing file
	~FileLock();

	// Main function to lock file
	int setlock(const LockMode mode);

	// Alternative locker is using status vector to report errors
	bool setlock(Firebird::Arg::StatusVector& status, const LockMode mode);

	// unlocking can only put error into log file - we can't throw in dtors
	void unlock();

	int getFd();

private:
	enum LockLevel {LCK_NONE, LCK_SHARED, LCK_EXCL};

	LockLevel level;
	CountedFd* oFile;
#ifdef USE_FCNTL
	int lStart;
#endif
	class CountedRWLock* rwcl;		// Due to order of init in ctor rwcl must go after fd & start

	Firebird::string getLockId();
	class CountedRWLock* getRw();
	void rwUnlock();
};

#endif // UNIX


class SharedMemoryBase;		// forward

class IpcObject
{
public:
	virtual bool initialize(SharedMemoryBase*, bool) = 0;
	virtual void mutexBug(int osErrorCode, const char* text) = 0;
	//virtual void eventBug(int osErrorCode, const char* text) = 0;
};


class SharedMemoryBase
{
public:
	SharedMemoryBase(const TEXT* fileName, ULONG size, IpcObject* cb);
	~SharedMemoryBase();

#ifdef HAVE_OBJECT_MAP
	UCHAR* mapObject(Firebird::Arg::StatusVector& status, ULONG offset, ULONG size);
	void unmapObject(Firebird::Arg::StatusVector& status, UCHAR** object, ULONG size);
#endif
	bool remapFile(Firebird::Arg::StatusVector& status, ULONG newSize, bool truncateFlag);
	void removeMapFile();
#ifdef UNIX
	void internalUnmap();
#endif

	void mutexLock();
	bool mutexLockCond();
	void mutexUnlock();

	int eventInit(event_t* event);
	void eventFini(event_t* event);
	SLONG eventClear(event_t* event);
	int eventWait(event_t* event, const SLONG value, const SLONG micro_seconds);
	int eventPost(event_t* event);

public:
#ifdef UNIX
	Firebird::AutoPtr<FileLock> mainLock;
#endif
#ifdef WIN_NT
	struct mtx sh_mem_winMutex;
	struct mtx* sh_mem_mutex;
#endif
#ifdef HAVE_SHARED_MUTEX_SECTION
	struct mtx* sh_mem_mutex;
#endif
#ifdef USE_FILELOCKS
	Firebird::AutoPtr<FileLock> sh_mem_fileMutex;
	Firebird::Mutex localMutex;
#endif

#ifdef UNIX
	Firebird::AutoPtr<FileLock> initFile;
#endif
#ifdef USE_SYS5SEMAPHORE
	Firebird::AutoPtr<FileLock> semFile;
#endif

	ULONG	sh_mem_length_mapped;
#ifdef WIN_NT
	void*	sh_mem_handle;
	void*	sh_mem_object;
	void*	sh_mem_interest;
	void*	sh_mem_hdr_object;
	ULONG*	sh_mem_hdr_address;
#endif
	TEXT	sh_mem_name[MAXPATHLEN];
	MemoryHeader* volatile sh_mem_header;
#ifdef USE_SYS5SEMAPHORE
private:
	int		fileNum;	// file number in shared table of shared files
	bool	getSem5(Sys5Semaphore* sem);
	void	freeSem5(Sys5Semaphore* sem);
#endif

private:
	IpcObject* sh_mem_callback;
#ifdef WIN_NT
	bool sh_mem_unlink;
#endif
	void unlinkFile();

public:
	enum MemoryTypes
	{
		SRAM_LOCK_MANAGER = 0xFF,		// To avoid mixing with old files no matter of endianness
		SRAM_DATABASE_SNAPSHOT = 0xFE,	// use downcount for shared memory types
		SRAM_EVENT_MANAGER = 0xFD,
		SRAM_TRACE_CONFIG = 0xFC,
		SRAM_TRACE_LOG = 0xFB,
		SRAM_MAPPING_RESET = 0xFA,
	};

protected:
	void logError(const char* text, const Firebird::Arg::StatusVector& status);
};

template <class Header>		// Header must be "public MemoryHeader"
class SharedMemory : public SharedMemoryBase
{
public:
	SharedMemory(const TEXT* fileName, ULONG size, IpcObject* cb)
		: SharedMemoryBase(fileName, size, cb)
	{ }

#ifdef HAVE_OBJECT_MAP
	template <class Object> Object* mapObject(Firebird::Arg::StatusVector& status, ULONG offset)
	{
		return (Object*) SharedMemoryBase::mapObject(status, offset, sizeof(Object));
	}

	template <class Object> void unmapObject(Firebird::Arg::StatusVector& status, Object** object)
	{
		SharedMemoryBase::unmapObject(status, (UCHAR**) object, sizeof(Object));
	}
#endif

public:
	void setHeader(Header* hdr)
	{
		sh_mem_header = hdr;	// This implicit cast ensures that Header is "public MemoryHeader"
	}

	const Header* getHeader() const
	{
		return (const Header*) sh_mem_header;
	}

	Header* getHeader()
	{
		return (Header*) sh_mem_header;
	}

};

} // namespace Firebird

#ifdef WIN_NT
int		ISC_mutex_init(struct Firebird::mtx*, const TEXT*);
void	ISC_mutex_fini(struct Firebird::mtx*);
int		ISC_mutex_lock(struct Firebird::mtx*);
int		ISC_mutex_unlock(struct Firebird::mtx*);
#endif

ULONG	ISC_exception_post(ULONG, const TEXT*);

#endif // JRD_ISC_S_PROTO_H
