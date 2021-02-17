/*
 *
 *     The contents of this file are subject to the Initial
 *     Developer's Public License Version 1.0 (the "License");
 *     you may not use this file except in compliance with the
 *     License. You may obtain a copy of the License at
 *     http://www.ibphoenix.com/idpl.html.
 *
 *     Software distributed under the License is distributed on
 *     an "AS IS" basis, WITHOUT WARRANTY OF ANY KIND, either
 *     express or implied.  See the License for the specific
 *     language governing rights and limitations under the License.
 *
 *     The contents of this file or any work derived from this file
 *     may not be distributed under any other license whatsoever
 *     without the express prior written permission of the original
 *     author.
 *
 *
 *  The Original Code was created by James A. Starkey for IBPhoenix.
 *
 *  Copyright (c) 1997 - 2000, 2001, 2003 James A. Starkey
 *  Copyright (c) 1997 - 2000, 2001, 2003 Netfrastructure, Inc.
 *  All Rights Reserved.
 *
 *  The Code was ported into Firebird Open Source RDBMS project by
 *  Vladyslav Khorsun at 2010
 *
 *  Contributor(s):
 */

#ifndef CLASSES_SYNCHRONIZE_H
#define CLASSES_SYNCHRONIZE_H

#include "../common/classes/SyncObject.h"

#ifndef WIN_NT
#include "fb_pthread.h"
#endif


namespace Firebird {

class Synchronize
{
public:
	Synchronize();
	virtual ~Synchronize();

	virtual void shutdown();
	void wake();
	bool sleep(int milliseconds);
	void sleep();

protected:
	bool shutdownInProgress;
	bool sleeping;
	volatile bool wakeup;

#ifdef WIN_NT
	void* evnt;
#else
	pthread_cond_t condition;
	pthread_mutex_t mutex;
#endif
};


class Sync;
class SyncObject;

class ThreadSync : public Synchronize
{
friend class SyncObject;

public:
	explicit ThreadSync(const char* desc);
	virtual ~ThreadSync();

	static ThreadSync* findThread();
	static ThreadSync* getThread(const char* desc);
	static ThreadId getCurrentThreadId();

	const char* getWhere() const;

private:
	void grantLock(SyncObject* lock);

	static void setThread(ThreadSync* thread);

	ThreadId threadId;
	ThreadSync* nextWaiting;	// next thread in sleep que (see SyncObject)
	ThreadSync* prevWaiting;	// previous thread in sleep que (see SyncObject)
	SyncType lockType;			// requested lock type (see SyncObject)
	volatile bool lockGranted;
	Sync* lockPending;
	Sync* locks;
	const char* description;
};


} // namespace Firebird

#endif // CLASSES_SYNCHRONIZE_H
