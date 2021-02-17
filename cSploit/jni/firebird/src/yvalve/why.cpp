/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		why.cpp
 *	DESCRIPTION:	Universal Y-valve
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
 * Dmitry Yemanov
 * Sean Leyne
 * Alex Peshkoff
 * Adriano dos Santos Fernandes
 *
 */

#define FB_COMPILING_WHY_CPP

#include "firebird.h"
#include "firebird/Provider.h"
#include "memory_routines.h"
#include "gen/iberror.h"
#include "gen/msg_facs.h"
#include "../yvalve/YObjects.h"
#include "../dsql/sqlda.h"
#include "../dsql/sqlda_pub.h"
#include "../common/gdsassert.h"
#include "../common/db_alias.h"
#include "../common/StatementMetadata.h"
#include "../common/StatusHolder.h"
#include "../common/ThreadStart.h"
#include "../common/isc_proto.h"
#include "../common/isc_f_proto.h"
#include "../common/utils_proto.h"
#include "../common/IntlParametersBlock.h"
#include "../common/os/isc_i_proto.h"
#include "../common/os/path_utils.h"
#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/stack.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/fb_atomic.h"
#include "../common/classes/init.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/FpeControl.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/GetPlugins.h"
#include "../common/classes/fb_tls.h"
#include "../common/classes/InternalMessageBuffer.h"
#include "../yvalve/utl_proto.h"
#include "../yvalve/why_proto.h"
#include "../yvalve/MasterImplementation.h"
#include "../yvalve/PluginManager.h"
#include "../jrd/acl.h"
#include "../jrd/align.h"
#include "../jrd/blr.h"
#include "../jrd/msg_encode.h"
#include "../jrd/inf_pub.h"

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

using namespace Firebird;
using namespace Why;


static void badHandle(ISC_STATUS code);
static bool isNetworkError(const IStatus* status);
static void nullCheck(const FB_API_HANDLE* ptr, ISC_STATUS code);
//static void saveErrorString(ISC_STATUS* status);
static void error804(ISC_STATUS err);
static void sqldaDescribeParameters(XSQLDA* sqlda, IMessageMetadata* parameters);
static ISC_STATUS openOrCreateBlob(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId,
	USHORT bpbLength, const UCHAR* bpb, bool createFlag);


//-------------------------------------


static const USHORT PREPARE_BUFFER_SIZE = 32768;	// size of buffer used in isc_dsql_prepare call
static const USHORT DESCRIBE_BUFFER_SIZE = 1024;	// size of buffer used in isc_dsql_describe_xxx call

namespace Why {
	class StatusVector;
	extern UtlInterface utlInterface;
};

namespace {

// Class-wrapper around external SQLDA.
// Can be used as local variable, but do it with care
class SQLDAMetadata FB_FINAL : public RefCntIface<IMessageMetadata, FB_MESSAGE_METADATA_VERSION>
{
friend class SQLDAMetadataLauncher;
private:
	int detach(); // is called from launcher class when it goes off

	// Fool-proof requested by Alex
	// Private memory operators to be sure that this class is used in heap only with launcher
	void* operator new (size_t s) { return MemoryPool::globalAlloc(s); }
	void operator delete (void* mem) { MemoryPool::globalFree(mem); }

public:
	typedef UCharBuffer DataBuffer;

	SQLDAMetadata(const XSQLDA* aSqlda);
	~SQLDAMetadata() { delete[] offsets; }

	virtual int FB_CARG release();

	unsigned FB_CARG getCount(IStatus* status);
	const char* FB_CARG getField(IStatus* status, unsigned index);
	const char* FB_CARG getRelation(IStatus* status, unsigned index);
	const char* FB_CARG getOwner(IStatus* status, unsigned index);
	const char* FB_CARG getAlias(IStatus* status, unsigned index);
	unsigned FB_CARG getType(IStatus* status, unsigned index);
	FB_BOOLEAN FB_CARG isNullable(IStatus* status, unsigned index);
	int FB_CARG getSubType(IStatus* status, unsigned index);
	unsigned FB_CARG getLength(IStatus* status, unsigned index);
	int FB_CARG getScale(IStatus* status, unsigned index);
	unsigned FB_CARG getCharSet(IStatus* status, unsigned index);
	unsigned FB_CARG getOffset(IStatus* status, unsigned index);
	unsigned FB_CARG getNullOffset(IStatus* status, unsigned index);

	IMetadataBuilder* FB_CARG getBuilder(IStatus* status);
	unsigned FB_CARG getMessageLength(IStatus* status);

	void gatherData(DataBuffer& to);	// Copy data from SQLDA into target buffer.
	void scatterData(DataBuffer& from);

private:
	void assign(); // copy metadata from sqlda and calculate length and offsets

	const XSQLDA* sqlda;
	unsigned count;

	// Internal structure to copy metadata from sqlda
	struct OffsetItem
	{
		unsigned type;
		int subType;
		unsigned length;
		int scale;
		unsigned charSet;
		unsigned offset;
		unsigned indOffset;
	} *offsets;

	unsigned length;
	bool speedHackEnabled; // May be user by stupid luck use right buffer format even with SQLDA interface?..
};

// Initial owner for SQLDAMetadata. It must not live longer that XSQLDA it is created for is know to be valid and fixed.
// Mostly it means that instance of this class has to be a local variable at the same or nested level with the original XSQLDA*.
// When launcher is dying, the rocket must either die too or switch to internal sources and take off
// Using of this class brings overhead of dynamic memory and pointers, but with current architecture (reference counted interfaces
// only plain buffers) - inevitable.
class SQLDAMetadataLauncher
{
public:
	SQLDAMetadata* metadata;

	explicit SQLDAMetadataLauncher(const XSQLDA* sqlda) : metadata(NULL)
	{
		if (sqlda)
		{
			metadata = new SQLDAMetadata(sqlda);
			metadata->addRef();
		}
	}

	~SQLDAMetadataLauncher()
	{
		if (metadata)
			metadata->detach();
	}

	UCHAR* getBuffer(); // Provide empty data buffer with the same live time

	void gatherData(SQLDAMetadata::DataBuffer& to)
	{
		if (metadata)
			metadata->gatherData(to);
		else
			to.clear();
	}

 	void scatterData()
	{
		if (metadata)
			metadata->scatterData(buffer);
	}

private:
	// Private operator new prevents this class from being allocated in heap, so it can be used only as stack or static variable
	void* operator new (size_t);

	SQLDAMetadata::DataBuffer buffer;
};

SQLDAMetadata::SQLDAMetadata(const XSQLDA* aSqlda)
	: sqlda(aSqlda), count(0), offsets(NULL), length(0), speedHackEnabled(false)
{
	if (sqlda && sqlda->version != SQLDA_VERSION1)
	{
		(Arg::Gds(isc_dsql_sqlda_err) <<
		 Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		 Arg::Gds(isc_random) << "Invalid SQLDA version"
		).raise();
	}
}

unsigned FB_CARG SQLDAMetadata::getCount(IStatus* status)
{
	if (sqlda)
		return sqlda->sqld;

	return count;
}

const char* FB_CARG SQLDAMetadata::getField(IStatus* status, unsigned index)
{
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int)index);
		return sqlda->sqlvar[index].sqlname;
	}

	// we are in free fly. It is only possible for input sqlda which usually have no names inside
	return "";	// Old conversion sqlda->BLR->metadata dropped them anyway
}

const char* FB_CARG SQLDAMetadata::getRelation(IStatus* status, unsigned index)
{
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int)index);
		return sqlda->sqlvar[index].relname;
	}

	return "";
}

const char* FB_CARG SQLDAMetadata::getOwner(IStatus* status, unsigned index)
{
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		return sqlda->sqlvar[index].ownname;
	}

	return "";
}

const char* FB_CARG SQLDAMetadata::getAlias(IStatus* status, unsigned index)
{
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		return sqlda->sqlvar[index].aliasname;
	}

	return "";
}

unsigned FB_CARG SQLDAMetadata::getType(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].type & ~1;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		return sqlda->sqlvar[index].sqltype & ~1;
	}

	fb_assert(false);
	return 0;
}

FB_BOOLEAN FB_CARG SQLDAMetadata::isNullable(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].type & 1;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		return sqlda->sqlvar[index].sqltype & 1;
	}

	fb_assert(false);
	return FB_FALSE;
}

int FB_CARG SQLDAMetadata::getSubType(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].subType;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		ISC_SHORT sqltype = sqlda->sqlvar[index].sqltype & ~1;
		if (sqltype == SQL_VARYING || sqltype == SQL_TEXT)
			return 0;
		return sqlda->sqlvar[index].sqlsubtype;
	}

	fb_assert(false);
	return 0;
}

unsigned FB_CARG SQLDAMetadata::getLength(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].length;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		return sqlda->sqlvar[index].sqllen;
	}

	fb_assert(false);
	return 0;
}

int FB_CARG SQLDAMetadata::getScale(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].scale;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		if ((sqlda->sqlvar[index].sqltype & ~1) == SQL_BLOB)
			return 0;
		return sqlda->sqlvar[index].sqlscale;
	}

	fb_assert(false);
	return 0;
}

unsigned FB_CARG SQLDAMetadata::getCharSet(IStatus* status, unsigned index)
{
	if (offsets)
	{
		fb_assert(count > index);
		return offsets[index].charSet;
	}
	if (sqlda)
	{
		fb_assert(sqlda->sqld > (int) index);
		switch (sqlda->sqlvar[index].sqltype & ~1)
		{
		case SQL_TEXT:
		case SQL_VARYING:
			return sqlda->sqlvar[index].sqlsubtype;
		case SQL_BLOB:
			return sqlda->sqlvar[index].sqlscale;
		default:
			return 0;
		}
	}

	fb_assert(false);
	return 0;
}

unsigned FB_CARG SQLDAMetadata::getOffset(IStatus* status, unsigned index)
{
	if (!offsets)
		assign();

	fb_assert(count > index);
	return offsets[index].offset;
}

unsigned FB_CARG SQLDAMetadata::getNullOffset(IStatus* status, unsigned index)
{
	if (!offsets)
		assign();

	fb_assert(count > index);
	return offsets[index].indOffset;
}

// ATTENTION!!! Keep this method in sync with MsgMetadata::MakeOffsets()
void SQLDAMetadata::assign()
{
	if (offsets)
		return; // already assigned

	if (sqlda->sqld <= 0) // malformed sqlda?
		return;

	count = (USHORT) sqlda->sqld;
	speedHackEnabled = true; // May be we are lucky...
	ISC_SCHAR* const base = sqlda->sqlvar[0].sqldata;

	offsets = new OffsetItem[count];
	for (unsigned i = 0; i < count; i++)
	{
		const XSQLVAR& var = sqlda->sqlvar[i];
		OffsetItem& it = offsets[i];

		it.type = var.sqltype;
		it.subType = var.sqlsubtype;
		it.length = var.sqllen;
		it.scale = var.sqlscale;
		switch (var.sqltype & ~1)
		{
		case SQL_TEXT:
		case SQL_VARYING:
			it.charSet = var.sqlsubtype;
			it.subType = 0;
			break;
		case SQL_BLOB:
			it.charSet = var.sqlscale;
			it.scale = 0;
			break;
		default:
			it.charSet = 0;
		}
		// No matter how good or bad is the way data is placed in message buffer, it cannot be changed
		// because changing of it on current codebase will completely kill remote module and may be the engine as well
		length = fb_utils::sqlTypeToDsc(length, var.sqltype, var.sqllen,
			NULL /*dtype*/, NULL /*length*/, &it.offset, &it.indOffset);
		if (it.offset != var.sqldata - base || it.indOffset != ((ISC_SCHAR*) (var.sqlind)) - base)
			speedHackEnabled = false; // No luck
	}
}

unsigned FB_CARG SQLDAMetadata::getMessageLength(IStatus* status)
{
	if (!offsets)
		assign();
	return length;
}

void SQLDAMetadata::gatherData(DataBuffer& to)
{
	fb_assert(sqlda); // Ensure that data is gathered before take off because later they can be already changed
	if (sqlda->sqld <= 0)
	{
		to.clear();
		return;
	}

	const unsigned l = getMessageLength(NULL);
	if (speedHackEnabled)
	{
		to.assign(reinterpret_cast<UCHAR*>(sqlda->sqlvar[0].sqldata), l);
		return;
	}

	UCHAR* result = to.getBuffer(l, false);
	for (unsigned i = 0; i < count; i++)
	{
		SSHORT* nullInd = (SSHORT*)(result + offsets[i].indOffset);
		UCHAR* data = result + offsets[i].offset;

		const XSQLVAR& var = sqlda->sqlvar[i];
		unsigned len = var.sqllen;
		if ((var.sqltype & ~1) == SQL_VARYING)
			len += sizeof(USHORT);

		if (var.sqltype & 1)
		{
			// Make sure user has specified a location for null indicator.
			if (!var.sqlind)
			{
				(Arg::Gds(isc_dsql_sqlda_value_err) <<
				 Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				 Arg::Gds(isc_random) << "empty pointer to NULL indicator variable" <<
				 Arg::Gds(isc_index_name) << Arg::Num(i)  // To be replaced later with proper message
				).raise();
			}

			*nullInd = *var.sqlind;
		}
		else
			*nullInd = 0;

		// Make sure user has specified a data location (unless NULL).
		if (!var.sqldata && !*nullInd && (var.sqltype & ~1) != SQL_NULL)
		{
			(Arg::Gds(isc_dsql_sqlda_value_err) <<
				Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
				Arg::Gds(isc_random) << "empty pointer to data" <<
				Arg::Gds(isc_index_name) << Arg::Num(i)  // To be replaced later with proper message
			).raise();
		}

		// This is assertion instead of error check because such situation is next to impossible: buffer size is calculated a routine above
		fb_assert(data + len < result + length);

		// Copy data - unless known to be NULL.
		if (!*nullInd)
			memcpy(data, var.sqldata, len);
	}
}

void SQLDAMetadata::scatterData(DataBuffer& from)
{
	fb_assert(sqlda);
	fb_assert(offsets); // Not reliable, but still check that input buffer can come from this metadata

	if (speedHackEnabled)
		return; // Data is already in user's buffer

	fb_assert(length == from.getCount());

	for (unsigned i = 0; i < count; i++)
	{
		const XSQLVAR& var = sqlda->sqlvar[i];

		if ((var.sqltype & ~1) != SQL_NULL)
		{
			unsigned len = var.sqllen;

			// Make sure user has specified a data location.
			if (!var.sqldata)
			{
				(Arg::Gds(isc_dsql_sqlda_value_err) <<
					Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					Arg::Gds(isc_random) << "empty pointer to data" <<
					Arg::Gds(isc_index_name) << Arg::Num(i)  // To be replaced later with proper message
				).raise();
			}

			if ((var.sqltype & ~1) == SQL_VARYING)
				len += sizeof(SSHORT);

			memcpy(var.sqldata, from.begin() + offsets[i].offset, len);
		}

		if (var.sqltype & 1)
		{
			// Make sure user has specified a location for null indicator.
			if (!var.sqlind)
			{
				(Arg::Gds(isc_dsql_sqlda_value_err) <<
					Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
					Arg::Gds(isc_random) << "empty pointer to NULL indicator variable" <<
					Arg::Gds(isc_index_name) << Arg::Num(i)  // To be replaced later with proper message
				).raise();
			}

			*var.sqlind = *reinterpret_cast<ISC_SHORT*>(from.begin() + offsets[i].indOffset);
		}
	}

}

UCHAR* SQLDAMetadataLauncher::getBuffer()
{
	if (metadata)
	{
		// ASF: It's important to call getMessageLength before check speedHackEnabled.
		unsigned length = metadata->getMessageLength(NULL);

		if (metadata->speedHackEnabled)
			return reinterpret_cast<UCHAR*>(metadata->sqlda->sqlvar[0].sqldata);

		return buffer.getBuffer(length, false);
	}
	else
	{
		buffer.clear();
		return NULL;
	}
}

IMetadataBuilder* FB_CARG SQLDAMetadata::getBuilder(IStatus* status)
{
	// no way to construct SQLDA
	fb_assert(false);
	return NULL;
}

int SQLDAMetadata::release()
{
	if (--refCounter != 0)
	{
		return 1;
	}

	delete this;
	return 0;
}

int SQLDAMetadata::detach()
{
	// This code can lead to unnecessary work but not to segfault
	if (refCounter.value() > 1)
	{
		if (!offsets) // in current codebase this is never true because offsets are allocated on buffer size calculation
			assign();
	}
	sqlda = NULL;
	speedHackEnabled = false;

	return release();
}


class IscStatement : public RefCounted, public GlobalStorage, public YObject
{
public:
	static const ISC_STATUS ERROR_CODE = isc_bad_stmt_handle;

	explicit IscStatement(YAttachment* aAttachment)
		: cursorName(getPool()),
		  attachment(aAttachment),
		  statement(NULL),
		  userHandle(NULL),
		  pseudoOpened(false),
		  delayedFormat(false)
	{ }

	FB_API_HANDLE& getHandle();
	void openCursor(IStatus* status, FB_API_HANDLE* traHandle,
					IMessageMetadata* inMetadata, UCHAR* buffer, IMessageMetadata* outMetadata);
	void closeCursor(Why::StatusVector* status, bool raise);
	void closeStatement(Why::StatusVector* status);

	void execute(IStatus* status, FB_API_HANDLE* traHandle,
				 IMessageMetadata* inMetadata, UCHAR* inBuffer, IMessageMetadata* outMetadata, UCHAR* outBuffer);
	FB_BOOLEAN fetch(IStatus* status, IMessageMetadata* outMetadata, UCHAR* outBuffer);

	void checkPrepared(ISC_STATUS code = isc_unprepared_stmt) const
	{
		if (!statement)
			Arg::Gds(code).raise();
	}

	void checkCursorOpened() const
	{
		if (!statement || !statement->cursor)
			Arg::Gds(isc_dsql_cursor_not_open).raise();
	}

	void checkCursorClosed() const
	{
		if (statement && statement->cursor)
			Arg::Gds(isc_dsql_cursor_open_err).raise();
	}

	string cursorName;
	YAttachment* attachment;
	YStatement* statement;
	FB_API_HANDLE* userHandle;
	bool pseudoOpened, delayedFormat;
};

GlobalPtr<RWLock> handleMappingLock;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YService*> > > > services;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YAttachment*> > > > attachments;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YEvents*> > > > events;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YTransaction*> > > > transactions;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, IscStatement*> > > > statements;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YRequest*> > > > requests;
GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, YBlob*> > > > blobs;

bool shutdownStarted = false;


//-------------------------------------


// CVC: I'm following types_pub.h here. If there's a better solution, commit it, please.
#if defined(_LP64) || defined(__LP64__) || defined(__arch64__) || defined(_WIN64)
inline FB_API_HANDLE ULONG_TO_FB_API_HANDLE(ULONG h) { return static_cast<FB_API_HANDLE>(h); }
inline ULONG FB_API_HANDLE_TO_ULONG(FB_API_HANDLE h) { return h; }
#else
inline FB_API_HANDLE ULONG_TO_FB_API_HANDLE(ULONG h) { return reinterpret_cast<FB_API_HANDLE>(h); }
inline ULONG FB_API_HANDLE_TO_ULONG(FB_API_HANDLE h) { return reinterpret_cast<ULONG>(h); }
#endif


template <typename T>
void makeHandle(GenericMap<Pair<NonPooled<FB_API_HANDLE, T*> > >* map, T* obj, FB_API_HANDLE& h)
{
	static ULONG sequenceNumber = 0;

	WriteLockGuard sync(handleMappingLock, FB_FUNCTION);
	if (h)		// Avoid 2 different handles for same object
		return;

	ULONG handle = 0;

	// Loop until we find an empty handle slot. This is to care of case when counter rolls over.
	do
	{
		// Generate handle number using rolling counter.
		// This way we tend to give out unique handle numbers and closed
		// handles do not appear as valid to our clients.
		handle = ++sequenceNumber;

		// Avoid generating NULL handle when sequence number wraps
		if (!handle)
			handle = ++sequenceNumber;
	} while (map->put(ULONG_TO_FB_API_HANDLE(handle), obj));

	h = ULONG_TO_FB_API_HANDLE(handle);
}

template <typename T>
void removeHandle(GenericMap<Pair<NonPooled<FB_API_HANDLE, T*> > >* map, FB_API_HANDLE& handle)
{
	if (handle)
	{
		WriteLockGuard sync(handleMappingLock, FB_FUNCTION);
		bool removed = map->remove(handle);

		fb_assert(removed);
		(void) removed;	// avoid warning in prod build
		handle = 0;
	}
}

template <typename T>
RefPtr<T> translateHandle(GlobalPtr<GenericMap<Pair<NonPooled<FB_API_HANDLE, T*> > > >& map,
	FB_API_HANDLE* handle)
{
	if (!handle)
		status_exception::raise(Arg::Gds(T::ERROR_CODE));

	ReadLockGuard sync(handleMappingLock, FB_FUNCTION);

	T** obj = map->get(*handle);

	if (!obj)
		status_exception::raise(Arg::Gds(T::ERROR_CODE));

	return RefPtr<T>(*obj);
}

FB_API_HANDLE& IscStatement::getHandle()
{
	if (!handle)
		makeHandle(&statements, this, handle);
	return handle;
}

//-------------------------------------

const int SHUTDOWN_TIMEOUT = 5000;	// 5 sec

class ShutdownInit
{
public:
	explicit ShutdownInit(MemoryPool&)
	{
		InstanceControl::registerShutdown(atExitShutdown);
	}

private:
	static void atExitShutdown()
	{
		fb_shutdown(SHUTDOWN_TIMEOUT, fb_shutrsn_exit_called);
	}
};

#ifdef UNIX
	int killed;
	bool procInt, procTerm;
	SignalSafeSemaphore* shutdownSemaphore = NULL;

	THREAD_ENTRY_DECLARE shutdownThread(THREAD_ENTRY_PARAM)
	{
		for (;;)
		{
			killed = 0;
			try
			{
				if (shutdownSemaphore)
					shutdownSemaphore->enter();
			}
			catch (status_exception& e)
			{
				TEXT buffer[BUFFER_LARGE];
				const ISC_STATUS* vector = e.value();

				if (! (vector && fb_interpret(buffer, sizeof(buffer), &vector)))
					strcpy(buffer, "Unknown failure in shutdown thread in shutdownSemaphore->enter()");

				fb_utils::logAndDie(buffer);
			}

			if (!killed)
				break;

			// perform shutdown
			if (fb_shutdown(SHUTDOWN_TIMEOUT, fb_shutrsn_signal) == FB_SUCCESS)
			{
				InstanceControl::registerShutdown(0);
				break;	//exit(0);
			}
		}

		return 0;
	}

	void handler(int sig)
	{
		if (!killed && shutdownSemaphore)
		{
			killed = sig;
			shutdownSemaphore->release();
		}
	}

	void handlerInt(void*)
	{
		handler(SIGINT);
	}

	void handlerTerm(void*)
	{
		handler(SIGTERM);
	}

	class CtrlCHandler : public ShutdownInit
	{
	public:
		SignalSafeSemaphore semaphore;

		explicit CtrlCHandler(MemoryPool& p)
			: ShutdownInit(p)
		{
			Thread::start(shutdownThread, 0, 0, &handle);

			procInt = ISC_signal(SIGINT, handlerInt, 0);
			procTerm = ISC_signal(SIGTERM, handlerTerm, 0);
			shutdownSemaphore = &semaphore;
		}

		~CtrlCHandler()
		{
			ISC_signal_cancel(SIGINT, handlerInt, 0);
			ISC_signal_cancel(SIGTERM, handlerTerm, 0);

			if (!killed)
			{
				// Must be done to let shutdownThread close.
				shutdownSemaphore->release();
				shutdownSemaphore = NULL;
				Thread::waitForCompletion(handle);
			}
		}
	private:
		ThreadId handle;
	};
#endif // UNIX

	void signalInit()
	{
#ifdef UNIX
		static GlobalPtr<CtrlCHandler> ctrlCHandler;
#else
		static GlobalPtr<ShutdownInit> shutdownInit;
#endif // UNIX
	}

} // anonymous namespace


//-------------------------------------

namespace Why
{
	// StatusVector:	Provides correct status vector for operation and init() it.
	class StatusVector : public AutoIface<IStatus, FB_STATUS_VERSION>
	{
	public:
		explicit StatusVector(ISC_STATUS* v) throw()
			: localVector(v ? v : localStatus)
		{
			init();
		}

		operator ISC_STATUS*()
		{
			return localVector;
		}

		~StatusVector()
		{
#ifdef DEV_BUILD
			checkStatusVector(localVector);
#endif
		}

		// IStatus implementation
		void FB_CARG dispose()
		{ }

		void FB_CARG set(unsigned int length, const ISC_STATUS* value)
		{
			fb_utils::copyStatus(localVector, FB_NELEM(localStatus), value, length);
		}

		void FB_CARG set(const ISC_STATUS* value)
		{
			set(fb_utils::statusLength(value), value);
		}

		void FB_CARG init()
		{
			fb_utils::init_status(localVector);
		}

		const ISC_STATUS* FB_CARG get() const
		{
			return localVector;
		}

		int FB_CARG isSuccess() const
		{
			switch (localVector[1])
			{
				case 0:
				case 100:
					return true;

				default:
					break;
			}

			return false;
		}

		void check()
		{
			if (!isSuccess())
				status_exception::raise(get());
		}

#ifdef DEV_BUILD
		// Validate that a status vector looks valid.
		static void checkStatusVector(const ISC_STATUS* status)
		{
#define SV_MSG(x)	\
	do {	\
		gds__log ("%s %d check_status_vector: %s", __FILE__, __LINE__, (x)); BREAKPOINT (__LINE__);	\
	} while (false)

			const ISC_STATUS* s = status;
			if (!s)
			{
				SV_MSG("Invalid status vector");
				return;
			}

			if (*s != isc_arg_gds)
			{
				SV_MSG("Must start with isc_arg_gds");
				return;
			}

			// Vector [2] could either end the vector, or start a warning
			// in either case the status vector is a success.
			if (s[1] == FB_SUCCESS && s[2] != isc_arg_end && s[2] != isc_arg_gds &&
				s[2] != isc_arg_warning)
			{
				SV_MSG("Bad success vector format");
			}

			ULONG length;

			while (*s != isc_arg_end)
			{
				const ISC_STATUS code = *s++;

				switch (code)
				{
					case isc_arg_warning:
					case isc_arg_gds:
						// The next element must either be 0 (indicating no error) or a
						// valid isc error message, let's check.
						if (*s && (*s & ISC_MASK) != ISC_MASK)
						{
							if (code == isc_arg_warning)
								SV_MSG("warning code not a valid ISC message");
							else
								SV_MSG("error code not a valid ISC message");
						}

						// If the error code is valid, then I better be able to retrieve a
						// proper facility code from it ... let's find out.
						if (*s && (*s & ISC_MASK) == ISC_MASK)
						{
							bool found = false;

							const _facilities* facs = facilities;
							const int fac_code = GET_FACILITY(*s);

							while (facs->facility)
							{
								if (facs->fac_code == fac_code)
								{
									found = true;
									break;
								}

								++facs;
							}

							if (!found)
							{
								if (code == isc_arg_warning)
									SV_MSG("warning code does not contain a valid facility");
								else
									SV_MSG("error code does not contain a valid facility");
							}
						}

						++s;
						break;

					case isc_arg_interpreted:
					case isc_arg_string:
					case isc_arg_sql_state:
						length = strlen((const char*) *s);
						// This check is heuristic, not deterministic.
						if (length > 1024 - 1)
							SV_MSG("suspect length value");
						if (*((const UCHAR*) *s) == 0xCB)
							SV_MSG("string in freed memory");
						++s;
						break;

					case isc_arg_cstring:
						length = (ULONG) *s;
						++s;
						// This check is heuristic, not deterministic.
						// Note: This can never happen anyway, as the length is coming from a byte value.
						if (length > 1024 - 1)
							SV_MSG("suspect length value");
						if (*((const UCHAR*) *s) == 0xCB)
							SV_MSG("string in freed memory");
						++s;
						break;

					case isc_arg_number:
					case isc_arg_vms:
					case isc_arg_unix:
					case isc_arg_win32:
						++s;
						break;

					default:
						SV_MSG("invalid status code");
						return;
				}

				if (s - status >= ISC_STATUS_LENGTH)
					SV_MSG("vector too long");
			}

#undef SV_MSG
		}
#endif

	private:
		ISC_STATUS_ARRAY localStatus;
		ISC_STATUS* localVector;
	};

	class ShutChain : public GlobalStorage
	{
	private:
		ShutChain(ShutChain* link, FB_SHUTDOWN_CALLBACK cb, const int m, void* a)
			: next(link),
			  callBack(cb),
			  mask(m),
			  arg(a)
		{
		}

	private:
		static ShutChain* list;
		static GlobalPtr<Mutex> shutdownCallbackMutex;

		ShutChain* next;
		FB_SHUTDOWN_CALLBACK callBack;
		int mask;
		void* arg;

	public:
		static void add(FB_SHUTDOWN_CALLBACK cb, const int m, void* a)
		{
			MutexLockGuard guard(shutdownCallbackMutex, FB_FUNCTION);

			for (ShutChain* chain = list; chain; chain = chain->next)
			{
				if (chain->callBack == cb && chain->arg == a)
				{
					if (m)
						chain->mask |= m;
					else
						chain->mask = 0;

					return;
				}
			}

			list = new ShutChain(list, cb, m, a);
		}

		static int run(const int m, const int reason)
		{
			int rc = FB_SUCCESS;
			MutexLockGuard guard(shutdownCallbackMutex, FB_FUNCTION);

			for (ShutChain* chain = list; chain; chain = chain->next)
			{
				if ((chain->mask & m) && (chain->callBack(reason, m, chain->arg) != FB_SUCCESS))
					rc = FB_FAILURE;
			}

			return rc;
		}
	};

	ShutChain* ShutChain::list = NULL;
	GlobalPtr<Mutex> ShutChain::shutdownCallbackMutex;

	class NoEntrypoint
	{
	public:
		virtual void FB_CARG noEntrypoint(IStatus* s)
		{
			s->set(Arg::Gds(isc_wish_list).value());
		}
	};

	MakeUpgradeInfo<NoEntrypoint> upInfo;

	template <typename T, typename CleanupRoutine>	// T = YAttachment or YTransaction
	class CleanupCallbackImpl : public CleanupCallback
	{
	public:
		CleanupCallbackImpl(T* aObject, CleanupRoutine* aRoutine, void* aArg)
			: object(aObject),
			  routine(aRoutine),
			  arg(aArg)
		{
		}

		virtual void FB_CARG cleanupCallbackFunction()
		{
			call(routine);
			delete this;
		}

	private:
		void call(AttachmentCleanupRoutine* routine)
		{
			routine(&object->getHandle(), arg);
		}

		void call(TransactionCleanupRoutine* routine)
		{
			routine(object->getHandle(), arg);
		}

	private:
		T* object;
		CleanupRoutine* routine;
		void* arg;
	};

	static AtomicCounter dispCounter;

	template <typename Y>
	class YEntry : public FpeControl	//// TODO: move FpeControl to the engine
	{
	public:
		YEntry(IStatus* aStatus, Y* object, int checkAttachment = 1)
			: ref(object->attachment), nextRef(NULL)
		{
			aStatus->init();
			init(object->next);

			if (checkAttachment && !(nextRef.hasData()))
			{
				fini();
				Arg::Gds(Y::ERROR_CODE).raise();
			}

			if (checkAttachment && ref && ref->savedStatus.getError())
			{
				fini();
				status_exception::raise(object->attachment->savedStatus.value());
			}
		}

		~YEntry()
		{
			fini();
		}

		void init(typename Y::NextInterface* nxt)
		{
			signalInit();

			if (ref)
			{
				MutexLockGuard guard(ref->enterMutex, FB_FUNCTION);
				++ref->enterCount;
			}
			else
			{
				++dispCounter;
			}

			if (shutdownStarted)
			{
				fini();
				Arg::Gds(isc_att_shutdown).raise();
			}

			nextRef = nxt;
		}

		void fini()
		{
			RefDeb(DEB_RLS_JATT, "YEntry::fini");
			nextRef = NULL;

			if (ref)
			{
				MutexLockGuard guard(ref->enterMutex, FB_FUNCTION);
				--ref->enterCount;
			}
			else
			{
				--dispCounter;
			}
		}

		typename Y::NextInterface* next()
		{
			return nextRef;
		}

	private:
		YEntry(const YEntry&);	// prohibit copy constructor

	private:
		RefPtr<typename Y::YRef> ref;
		RefPtr<typename Y::NextInterface> nextRef;
	};

	template <>
	YEntry<YAttachment>::YEntry(IStatus* aStatus, YAttachment* aAttachment, int checkAttachment)
		: ref(aAttachment), nextRef(NULL)
	{
		aStatus->init();
		init(aAttachment->next);

		if (checkAttachment && !(nextRef.hasData()))
		{
			fini();
			Arg::Gds(YAttachment::ERROR_CODE).raise();
		}

		if (checkAttachment && aAttachment->savedStatus.getError())
		{
			fini();
			status_exception::raise(aAttachment->savedStatus.value());
		}
	}

	template <>
	YEntry<YService>::YEntry(IStatus* aStatus, YService* aService, int checkService)
		: ref(aService), nextRef(NULL)
	{
		aStatus->init();
		init(aService->next);

		if (checkService && !nextRef.hasData())
		{
			fini();
			Arg::Gds(YService::ERROR_CODE).raise();
		}

	}

	class DispatcherEntry : public FpeControl	//// TODO: move FpeControl to the engine
	{
	public:
		explicit DispatcherEntry(IStatus* aStatus, bool p_shutdownMode = false)
			: shutdownMode(p_shutdownMode)
		{
			aStatus->init();
			signalInit();

			if (!shutdownMode)
			{
				++dispCounter;
				if (shutdownStarted)
				{
					--dispCounter;
					Arg::Gds(isc_att_shutdown).raise();
				}
			}
		}

		~DispatcherEntry()
		{
			if (!shutdownMode)
			{
				--dispCounter;
			}
		}

	private:
		DispatcherEntry(const DispatcherEntry&);	// prohibit copy constructor
		bool shutdownMode;
	};

}	// namespace Why

namespace {
	void IscStatement::openCursor(IStatus* status, FB_API_HANDLE* traHandle,
					IMessageMetadata* inMetadata, UCHAR* buffer, IMessageMetadata* outMetadata)
	{
		checkCursorClosed();

		// Transaction is not optional for statement returning result set
		RefPtr<YTransaction> transaction = translateHandle(transactions, traHandle);;

		statement->openCursor(status, transaction, inMetadata, buffer, outMetadata);

		if (!status->isSuccess())
			return;

		fb_assert(statement->cursor);

		if (cursorName.hasData())
		{
			statement->cursor->setCursorName(status, cursorName.c_str());

			if (status->isSuccess())
				cursorName = "";
		}

		delayedFormat = (outMetadata == DELAYED_OUT_FORMAT);
	}

	void IscStatement::closeCursor(Why::StatusVector* status, bool raise)
	{
		if (statement && statement->cursor)
		{
			fb_assert(!pseudoOpened);

			statement->cursor->close(status);
			if (!status->isSuccess())
				Arg::StatusVector(status->get()).raise();

			statement->cursor = NULL;
		}
		else if (pseudoOpened)
			pseudoOpened = false;
		else if (raise)
			Arg::Gds(isc_dsql_cursor_close_err).raise();
	}

	void IscStatement::closeStatement(Why::StatusVector* status)
	{
		if (statement)
		{
			statement->free(status);
			if (!status->isSuccess())
				Arg::StatusVector(status->get()).raise();

			statement = NULL;
			cursorName = "";
		}
	}

	void IscStatement::execute(IStatus* status, FB_API_HANDLE* traHandle,
				IMessageMetadata* inMetadata, UCHAR* inBuffer, IMessageMetadata* outMetadata,
				UCHAR* outBuffer)
	{
		checkCursorClosed();

		RefPtr<YTransaction> transaction;
		if (traHandle && *traHandle)
			transaction = translateHandle(transactions, traHandle);

		ITransaction* newTrans = statement->execute(status, transaction,
			inMetadata, inBuffer, outMetadata, outBuffer);

		if (status->isSuccess())
		{
			if (transaction && !newTrans)
			{
				transaction->destroy();
				*traHandle = 0;
			}
			else if (!transaction && newTrans)
			{
				// in this case we know for sure that newTrans points to YTransaction
				if (traHandle)
					*traHandle = static_cast<YTransaction*>(newTrans)->getHandle();
			}
		}
	}

	FB_BOOLEAN IscStatement::fetch(IStatus* status, IMessageMetadata* outMetadata, UCHAR* outBuffer)
	{
		checkCursorOpened();

		if (delayedFormat)
		{
			statement->cursor->setDelayedOutputFormat(status, outMetadata);

			if (!status->isSuccess())
				return FB_FALSE;

			delayedFormat = false;
		}

		return statement->cursor->fetchNext(status, outBuffer);
	}
}

struct TEB
{
	FB_API_HANDLE* teb_database;
	int teb_tpb_length;
	const UCHAR* teb_tpb;
};


//-------------------------------------


static void badHandle(ISC_STATUS code)
{
	status_exception::raise(Arg::Gds(code));
}

static bool isNetworkError(const IStatus* status)
{
	ISC_STATUS code = status->get()[1];
	return code == isc_network_error || code == isc_net_write_err || code == isc_net_read_err;
}

static void nullCheck(const FB_API_HANDLE* ptr, ISC_STATUS code)
{
	// This function is called for incoming API handles, proposed to be created by the call.
	if (!ptr || *ptr)
		badHandle(code);
}

// This is need because there are cases where the memory allocated for strings in the status vector
// is freed prior to surfacing them to the user. This is an attempt to save off 1 string to surface
// to the user. Any other strings will be set to a standard <Unknown> string.
/*
static void saveErrorString(ISC_STATUS* status)
{
	const int MAXERRORSTRINGLENGTH = 250;
	static TEXT glbstr1[MAXERRORSTRINGLENGTH];
	static const TEXT glbunknown[10] = "<unknown>";

	fb_assert(status != NULL);

	char* p = glbstr1;
	ULONG len = sizeof(glbstr1) - 1;

	while (*status != isc_arg_end)
	{
		ULONG l;

		switch (*status++)
		{
			case isc_arg_cstring:
				l = (ULONG) *status;
				if (l < len)
				{
					++status;		// Length is unchanged.

					// This strncpy should really be a memcpy.
					strncpy(p, reinterpret_cast<char*>(*status), l);
					*status++ = (ISC_STATUS) p;	// string in static memory
					p += l;
					len -= l;
				}
				else
				{
					*status++ = (ISC_STATUS) strlen(glbunknown);
					*status++ = (ISC_STATUS) glbunknown;
				}
				break;

			case isc_arg_interpreted:
			case isc_arg_string:
			case isc_arg_sql_state:
				l = (ULONG) strlen(reinterpret_cast<char*>(*status)) + 1;
				if (l < len)
				{
					strncpy(p, reinterpret_cast<char*>(*status), l);
					*status++ = (ISC_STATUS) p;	// string in static memory
					p += l;
					len -= l;
				}
				else
					*status++ = (ISC_STATUS) glbunknown;
				break;

			default:
				fb_assert(false);
				// fall into

			case isc_arg_gds:
			case isc_arg_number:
			case isc_arg_vms:
			case isc_arg_unix:
			case isc_arg_win32:
				status++;			// Skip parameter
				break;
		}
	}
}
 */
// Raises a DSQL -804 error message.
static void error804(ISC_STATUS err)
{
	status_exception::raise(
		Arg::Gds(isc_dsql_error) << Arg::Gds(isc_sqlerr) << Arg::Num(-804) <<
		Arg::Gds(err));
}

// Set charset info in SQLVAR according to legacy rules
static void setTextType(XSQLVAR* var, unsigned charSet)
{
	switch(var->sqltype & ~1)
	{
	case SQL_VARYING:
	case SQL_TEXT:
		var->sqlsubtype = charSet;
		break;
	case SQL_BLOB:
		var->sqlscale = charSet;
		break;
	}
}

// Describe parameters metadata in an sqlda.
static void sqldaDescribeParameters(XSQLDA* sqlda, IMessageMetadata* parameters)
{
	if (!sqlda)
		return;

	if (sqlda->version != SQLDA_VERSION1)
		error804(isc_dsql_sqlda_err);

	StatusVector status(NULL);

	unsigned parametersCount = parameters->getCount(&status);
	status.check();
	sqlda->sqld = (USHORT) parametersCount;

	// If necessary, inform the application that more sqlda items are needed.
	if (sqlda->sqld > sqlda->sqln)
		return;

	for (unsigned i = 0; i < parametersCount; ++i)
	{
		XSQLVAR* var = &sqlda->sqlvar[i];
		const char* s;

		var->sqltype = parameters->getType(&status, i);
		status.check();

		var->sqltype |= (parameters->isNullable(&status, i) ? 1 : 0);
		status.check();

		var->sqlsubtype = parameters->getSubType(&status, i);
		status.check();

		var->sqllen = parameters->getLength(&status, i);
		status.check();

		var->sqlscale = parameters->getScale(&status, i);
		status.check();

		unsigned charSet = parameters->getCharSet(&status, i);
		status.check();
		setTextType(var, charSet);

		s = parameters->getField(&status, i);
		status.check();
		var->sqlname_length = fb_utils::snprintf(var->sqlname, sizeof(var->sqlname), "%s", s);

		s = parameters->getRelation(&status, i);
		status.check();
		var->relname_length = fb_utils::snprintf(var->relname, sizeof(var->relname), "%s", s);

		s = parameters->getOwner(&status, i);
		status.check();
		var->ownname_length = fb_utils::snprintf(var->ownname, sizeof(var->ownname), "%s", s);

		s = parameters->getAlias(&status, i);
		status.check();
		var->aliasname_length = fb_utils::snprintf(var->aliasname, sizeof(var->aliasname), "%s", s);
	}
}

// Open or create an existing blob (extended edition).
static ISC_STATUS openOrCreateBlob(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId,
	USHORT bpbLength, const UCHAR* bpb, bool createFlag)
{
	StatusVector status(userStatus);

	try
	{
		nullCheck(blobHandle, isc_bad_segstr_handle);

		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		YBlob* blob = createFlag ?
			attachment->createBlob(&status, transaction, blobId, bpbLength, bpb) :
			attachment->openBlob(&status, transaction, blobId, bpbLength, bpb);

		if (!status.isSuccess())
			return status[1];

		*blobHandle = blob->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


//-------------------------------------


static TLS_DECLARE(ICryptKeyCallback*, legacyCryptCallback);

ISC_STATUS API_ROUTINE fb_database_crypt_callback(ISC_STATUS* userStatus, ICryptKeyCallback* cb)
{
	StatusVector status(userStatus);

	try
	{
		TLS_SET(legacyCryptCallback, cb);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


//-------------------------------------


Firebird::IAttachment* handleToIAttachment(Firebird::IStatus* status, FB_API_HANDLE* handle)
{
	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		attachment->addRef();
		return attachment;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}


Firebird::ITransaction* handleToITransaction(Firebird::IStatus* status, FB_API_HANDLE* handle)
{
	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, handle));
		transaction->addRef();
		return transaction;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}


//-------------------------------------


// Attach a database through the first subsystem that recognizes it.
ISC_STATUS API_ROUTINE isc_attach_database(ISC_STATUS* userStatus, SSHORT fileLength,
	const TEXT* filename, FB_API_HANDLE* publicHandle, SSHORT dpbLength, const SCHAR* dpb)
{
	StatusVector status(userStatus);

	try
	{
		nullCheck(publicHandle, isc_bad_db_handle);

		if (!filename)
			status_exception::raise(Arg::Gds(isc_bad_db_format) << Arg::Str(""));

		PathName pathName(filename, fileLength ? fileLength : strlen(filename));

		RefPtr<Dispatcher> dispatcher(new Dispatcher);

		dispatcher->setDbCryptCallback(&status, TLS_GET(legacyCryptCallback));
		if (!status.isSuccess())
			return status[1];

		YAttachment* attachment = dispatcher->attachDatabase(&status, pathName.c_str(),
			dpbLength, reinterpret_cast<const UCHAR*>(dpb));
		if (!status.isSuccess())
			return status[1];

		*publicHandle = attachment->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on blob object.
ISC_STATUS API_ROUTINE isc_blob_info(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle,
	SSHORT itemLength, const SCHAR* items, SSHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));

		blob->getInfo(&status, itemLength, reinterpret_cast<const UCHAR*>(items),
			bufferLength, reinterpret_cast<UCHAR*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Abort a partially completed blob.
ISC_STATUS API_ROUTINE isc_cancel_blob(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle)
{
	if (!*blobHandle)
	{
		if (userStatus)
			fb_utils::init_status(userStatus);

		return FB_SUCCESS;
	}

	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));

		blob->cancel(&status);

		if (status.isSuccess())
			*blobHandle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Try to cancel an event.
ISC_STATUS API_ROUTINE isc_cancel_events(ISC_STATUS* userStatus, FB_API_HANDLE* handle, SLONG* id)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		// This cast appears awful, but as long as handles are 32-bit entities it's OK.
		RefPtr<YEvents> event(translateHandle(events, (FB_API_HANDLE*) id));

		if (event->attachment != attachment)
			Arg::Gds(isc_bad_events_handle).raise();

		event->cancel(&status);

		if (status.isSuccess())
			*id = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Try to cancel an operation.
ISC_STATUS API_ROUTINE fb_cancel_operation(ISC_STATUS* userStatus, FB_API_HANDLE* handle,
	USHORT option)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		attachment->cancelOperation(&status, option);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Close a blob opened either for reading or writing (creation).
ISC_STATUS API_ROUTINE isc_close_blob(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));

		blob->close(&status);

		if (status.isSuccess())
			*blobHandle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Commit a transaction.
ISC_STATUS API_ROUTINE isc_commit_transaction(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->commit(&status);

		if (status.isSuccess())
			*traHandle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Do a commit retaining.
// N.B., the transaction cleanup handlers are NOT called here since, conceptually, the transaction
// is still running.
ISC_STATUS API_ROUTINE isc_commit_retaining(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->commitRetaining(&status);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_compile_request(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* reqHandle, USHORT blrLength, const SCHAR* blr)
{
	StatusVector status(userStatus);
	YRequest* request = NULL;

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		nullCheck(reqHandle, isc_bad_stmt_handle);

		request = attachment->compileRequest(&status, blrLength,
			reinterpret_cast<const UCHAR*>(blr));

		if (!status.isSuccess())
			return status[1];

		*reqHandle = request->getHandle();
	}
	catch (const Exception& e)
	{
		if (request)
		{
			*reqHandle = 0;
			StatusVector temp(NULL);
			request->free(&temp);
		}

		e.stuffException(&status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_compile_request2(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* reqHandle, USHORT blrLength, const SCHAR* blr)
{
	StatusVector status(userStatus);

	if (isc_compile_request(status, dbHandle, reqHandle, blrLength, blr))
		return status[1];

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		request->userHandle = reqHandle;
	}
	catch (const Exception& e)
	{
		StatusVector temp(NULL);
		isc_release_request(temp, reqHandle);

		e.stuffException(&status);
	}

	return status[1];
}


// Open an existing blob.
ISC_STATUS API_ROUTINE isc_create_blob(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId)
{
	return openOrCreateBlob(userStatus, dbHandle, traHandle, blobHandle, blobId, 0, NULL, true);
}


// Create a new blob.
ISC_STATUS API_ROUTINE isc_create_blob2(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId,
	SSHORT bpbLength, const UCHAR* bpb)
{
	return openOrCreateBlob(userStatus, dbHandle, traHandle, blobHandle, blobId,
		bpbLength, bpb, true);
}


// Create a nice, squeaky clean database, uncorrupted by user data.
ISC_STATUS API_ROUTINE isc_create_database(ISC_STATUS* userStatus, USHORT fileLength,
	const TEXT* filename, FB_API_HANDLE* publicHandle, SSHORT dpbLength, const UCHAR* dpb,
	USHORT /*db_type*/)
{
	StatusVector status(userStatus);

	try
	{
		nullCheck(publicHandle, isc_bad_db_handle);

		if (!filename)
			status_exception::raise(Arg::Gds(isc_bad_db_format) << Arg::Str(""));

		PathName pathName(filename, fileLength ? fileLength : strlen(filename));

		RefPtr<Dispatcher> dispatcher(new Dispatcher);

		dispatcher->setDbCryptCallback(&status, TLS_GET(legacyCryptCallback));
		if (!status.isSuccess())
			return status[1];

		ClumpletWriter newDpb(ClumpletReader::dpbList, MAX_DPB_SIZE, dpb, dpbLength);
		if (!newDpb.find(isc_dpb_sql_dialect))
		{
			newDpb.insertInt(isc_dpb_sql_dialect, 1);	// legacy behavior in legacy interface
			dpb = newDpb.getBuffer();
			dpbLength = newDpb.getBufferLength();
		}

		YAttachment* attachment = dispatcher->createDatabase(&status, pathName.c_str(),
			dpbLength, reinterpret_cast<const UCHAR*>(dpb));
		if (!status.isSuccess())
			return status[1];

		*publicHandle = attachment->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Register an attachment specific cleanup handler.
ISC_STATUS API_ROUTINE isc_database_cleanup(ISC_STATUS* userStatus, FB_API_HANDLE* handle,
	AttachmentCleanupRoutine* routine, void* arg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));

		AutoPtr<CleanupCallbackImpl<YAttachment, AttachmentCleanupRoutine> > callback(
			new CleanupCallbackImpl<YAttachment, AttachmentCleanupRoutine>(attachment, routine, arg));

		attachment->addCleanupHandler(&status, callback);

		if (status.isSuccess())
			callback.release();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on database object.
ISC_STATUS API_ROUTINE isc_database_info(ISC_STATUS* userStatus, FB_API_HANDLE* handle,
	SSHORT itemLength, const SCHAR* items, SSHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		attachment->getInfo(&status, itemLength, reinterpret_cast<const UCHAR*>(items),
			bufferLength, reinterpret_cast<UCHAR*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Do meta-data update.
ISC_STATUS API_ROUTINE isc_ddl(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, SSHORT length, const UCHAR* dyn)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		attachment->executeDyn(&status, transaction, length, dyn);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Close down an attachment.
ISC_STATUS API_ROUTINE isc_detach_database(ISC_STATUS* userStatus, FB_API_HANDLE* handle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		attachment->detach(&status);

		if (status.isSuccess())
			*handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Disable access to a specific subsystem. If no subsystem has been explicitly disabled, all are
// available.
int API_ROUTINE gds__disable_subsystem(TEXT* /*subsystem*/)
{
	return FALSE;
}


// Close down a database and then purge it.
ISC_STATUS API_ROUTINE isc_drop_database(ISC_STATUS* userStatus, FB_API_HANDLE* handle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, handle));
		attachment->dropDatabase(&status);

		if (status.isSuccess() || status[1] == isc_drdb_completed_with_errs)
			*handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_dsql_alloc_statement(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* stmtHandle)
{
	return isc_dsql_allocate_statement(userStatus, dbHandle, stmtHandle);
}


// Allocate a statement handle.
ISC_STATUS API_ROUTINE isc_dsql_alloc_statement2(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* stmtHandle)
{
	StatusVector status(userStatus);

	if (isc_dsql_alloc_statement(status, dbHandle, stmtHandle))
		return status[1];

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));
		statement->userHandle = stmtHandle;
	}
	catch (const Exception& e)
	{
		StatusVector temp(NULL);
		isc_dsql_free_statement(temp, stmtHandle, DSQL_drop);

		e.stuffException(&status);
	}

	return status[1];
}


// Allocate a statement handle.
ISC_STATUS API_ROUTINE isc_dsql_allocate_statement(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* stmtHandle)
{
	StatusVector status(userStatus);
	IscStatement* statement = NULL;

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		nullCheck(stmtHandle, isc_bad_stmt_handle);

		statement = new IscStatement(attachment);
		statement->addRef();
		*stmtHandle = statement->getHandle();
	}
	catch (const Exception& e)
	{
		if (statement)
		{
			*stmtHandle = 0;
			statement->release();
		}
		e.stuffException(&status);
	}

	return status[1];
}


// Describe output parameters (columns) for a prepared statement.
ISC_STATUS API_ROUTINE isc_dsql_describe(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	USHORT /*dialect*/, XSQLDA* sqlda)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		statement->checkPrepared(isc_info_unprepared_stmt);

		RefPtr<IMessageMetadata> columns(REF_NO_INCR, statement->statement->next->getOutputMetadata(&status));
		status.check();

		sqldaDescribeParameters(sqlda, columns);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Describe input parameters for a prepared statement.
ISC_STATUS API_ROUTINE isc_dsql_describe_bind(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	USHORT /*dialect*/, XSQLDA* sqlda)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		statement->checkPrepared(isc_info_unprepared_stmt);

		RefPtr<IMessageMetadata> parameters(REF_NO_INCR, statement->statement->next->getInputMetadata(&status));
		status.check();

		sqldaDescribeParameters(sqlda, parameters);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Execute a non-SELECT dynamic SQL statement.
ISC_STATUS API_ROUTINE isc_dsql_execute(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT dialect, const XSQLDA* sqlda)
{
	return isc_dsql_execute2(userStatus, traHandle, stmtHandle, dialect, sqlda, NULL);
}


// Execute a non-SELECT dynamic SQL statement.
ISC_STATUS API_ROUTINE isc_dsql_execute2(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT dialect, const XSQLDA* inSqlda, const XSQLDA* outSqlda)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		statement->checkPrepared();

		const unsigned flags = statement->statement->getFlags(&status);
		if (!status.isSuccess())
		{
			return status[1];
		}

		SQLDAMetadataLauncher inMessage(inSqlda);
		SQLDAMetadata::DataBuffer inMsgBuffer;
		inMessage.gatherData(inMsgBuffer);

		if ((flags & IStatement::FLAG_HAS_CURSOR) && !outSqlda)
		{
			statement->checkCursorClosed();

			statement->openCursor(&status, traHandle,
				inMessage.metadata, inMsgBuffer.begin(), DELAYED_OUT_FORMAT);
			if (!status.isSuccess())
			{
				return status[1];
			}

			fb_assert(statement->statement->cursor);

			if (statement->cursorName.hasData())
			{
				statement->statement->cursor->setCursorName(&status, statement->cursorName.c_str());
				if (status.isSuccess())
					statement->cursorName = "";
			}
		}
		else
		{
			SQLDAMetadataLauncher outMessage(outSqlda);

			statement->execute(&status, traHandle,
				inMessage.metadata, inMsgBuffer.begin(), outMessage.metadata, outMessage.getBuffer());
			if (status.isSuccess())
				outMessage.scatterData();
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Execute a non-SELECT dynamic SQL statement.
ISC_STATUS API_ROUTINE isc_dsql_execute_m(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT blrLength, const SCHAR* blr,
	USHORT msgType, USHORT msgLength, SCHAR* msg)
{
	return isc_dsql_execute2_m(userStatus, traHandle, stmtHandle, blrLength, blr,
		msgType, msgLength, msg, 0, NULL, 0, 0, NULL);
}


// Execute a non-SELECT dynamic SQL statement.
ISC_STATUS API_ROUTINE isc_dsql_execute2_m(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT inBlrLength, const SCHAR* inBlr, USHORT inMsgType,
	USHORT inMsgLength, const SCHAR* inMsg, USHORT outBlrLength,
	SCHAR* outBlr, USHORT /*outMsgType*/, USHORT outMsgLength, SCHAR* outMsg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		if (((SSHORT) inMsgType) == -1)	// dirty hack to support old esql code
		{
			statement->pseudoOpened = true;
			return status[1];
		}

		statement->checkPrepared();

		InternalMessageBuffer inMsgBuffer(inBlrLength, (const UCHAR*)inBlr, inMsgLength, (UCHAR*)inMsg);
		InternalMessageBuffer outMsgBuffer(outBlrLength, (const UCHAR*)outBlr, outMsgLength, (UCHAR*)outMsg);

		const unsigned flags = statement->statement->getFlags(&status);
		if (status.isSuccess())
		{
			if ((flags & IStatement::FLAG_HAS_CURSOR) && (outMsgLength == 0))
			{
				statement->openCursor(&status, traHandle,
					inMsgBuffer.metadata, inMsgBuffer.buffer,
					(outBlrLength ? outMsgBuffer.metadata : DELAYED_OUT_FORMAT));
			}
			else
			{
				statement->execute(&status, traHandle,
					inMsgBuffer.metadata, inMsgBuffer.buffer, outMsgBuffer.metadata, outMsgBuffer.buffer);
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Is this really API function? Where is it declared?
ISC_STATUS API_ROUTINE isc_dsql_exec_immediate(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	const XSQLDA* sqlda)
{
	return isc_dsql_execute_immediate(userStatus, dbHandle, traHandle, stmtLength, sqlStmt,
		dialect, sqlda);
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_execute_immediate(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	const XSQLDA* sqlda)
{
	return isc_dsql_exec_immed2(userStatus, dbHandle, traHandle, stmtLength, sqlStmt,
		dialect, sqlda, NULL);
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_exec_immed2(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	const XSQLDA* inSqlda, const XSQLDA* outSqlda)
{
	StatusVector status(userStatus);

	try
	{
		if (!sqlStmt)
			Arg::Gds(isc_command_end_err).raise();

		FB_BOOLEAN stmtIsCrDb = FB_FALSE;
		YAttachment* att = utlInterface.executeCreateDatabase(&status, stmtLength, sqlStmt, dialect, &stmtIsCrDb);

		if (stmtIsCrDb)
		{
			if (status.isSuccess())
				*dbHandle = att->getHandle();

			return status[1];
		}

		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		SQLDAMetadata::DataBuffer inMessageBuffer;

		SQLDAMetadataLauncher inMessage(inSqlda), outMessage(outSqlda);

		inMessage.gatherData(inMessageBuffer);

		attachment->execute(&status, traHandle, stmtLength, sqlStmt,
			dialect, inMessage.metadata, inMessageBuffer.begin(),
			outMessage.metadata, outMessage.getBuffer());

		if (status.isSuccess())
		{
			outMessage.scatterData();
		}

	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return 	status[1];
}


// Is this really API function? Where is it declared?
ISC_STATUS API_ROUTINE isc_dsql_exec_immediate_m(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	USHORT blrLength, USHORT msgType, USHORT msgLength, SCHAR* blr, SCHAR* msg)
{
	return isc_dsql_execute_immediate_m(userStatus, dbHandle, traHandle, stmtLength, sqlStmt,
		dialect, blrLength, blr, msgType, msgLength, msg);
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_execute_immediate_m(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	USHORT blrLength, SCHAR* blr, USHORT msgType, USHORT msgLength, SCHAR* msg)
{
	return isc_dsql_exec_immed2_m(userStatus, dbHandle, traHandle, stmtLength, sqlStmt, dialect,
		blrLength, blr, msgType, msgLength, msg, 0, NULL, 0, 0, NULL);
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_exec_immed2_m(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	USHORT inBlrLength, SCHAR* inBlr, USHORT inMsgType, USHORT inMsgLength, const SCHAR* inMsg,
	USHORT outBlrLength, SCHAR* outBlr, USHORT outMsgType, USHORT outMsgLength, SCHAR* outMsg)
{
	StatusVector status(userStatus);

	FB_BOOLEAN stmtIsCrDb = FB_FALSE;
	YAttachment* att = utlInterface.executeCreateDatabase(&status, stmtLength, sqlStmt, dialect, &stmtIsCrDb);

	if (stmtIsCrDb)
	{
		if (status.isSuccess())
			*dbHandle = att->getHandle();

		return status[1];
	}

	return isc_dsql_exec_immed3_m(userStatus, dbHandle, traHandle, stmtLength, sqlStmt, dialect,
		inBlrLength, inBlr, inMsgType, inMsgLength, inMsg,
		outBlrLength, outBlr, outMsgType, outMsgLength, outMsg);
}


// Execute statement immediately.
ISC_STATUS API_ROUTINE isc_dsql_exec_immed3_m(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	USHORT inBlrLength, const SCHAR* inBlr, USHORT inMsgType, USHORT inMsgLength, const SCHAR* inMsg,
	USHORT outBlrLength, SCHAR* outBlr, USHORT /*outMsgType*/, USHORT outMsgLength, SCHAR* outMsg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		InternalMessageBuffer inMsgBuffer(inBlrLength, reinterpret_cast<const unsigned char*>(inBlr),
										  inMsgLength, reinterpret_cast<UCHAR*>(const_cast<SCHAR*>(inMsg)));
		InternalMessageBuffer outMsgBuffer(outBlrLength, reinterpret_cast<unsigned char*>(outBlr),
										   outMsgLength, reinterpret_cast<unsigned char*>(outMsg));

		attachment->execute(&status, traHandle, stmtLength, sqlStmt,
			dialect, inMsgBuffer.metadata, inMsgBuffer.buffer,
			outMsgBuffer.metadata, outMsgBuffer.buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}

// Fetch next record from a dynamic SQL cursor
ISC_STATUS API_ROUTINE isc_dsql_fetch(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	USHORT dialect, const XSQLDA* sqlda)
{
	StatusVector status(userStatus);

	try
	{
		if (!sqlda)
			status_exception::raise(Arg::Gds(isc_dsql_sqlda_err) << Arg::Gds(isc_random) << "No output SQLDA for fetch");

		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		SQLDAMetadataLauncher outMessage(sqlda);

		if (statement->fetch(&status, outMessage.metadata, outMessage.getBuffer()))
			outMessage.scatterData();
		else if (status.isSuccess())
			return 100;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Fetch next record from a dynamic SQL cursor
ISC_STATUS API_ROUTINE isc_dsql_fetch_m(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	USHORT blrLength, SCHAR* blr, USHORT /*msgType*/, USHORT msgLength, SCHAR* msg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		InternalMessageBuffer msgBuffer(blrLength, reinterpret_cast<UCHAR*>(blr), msgLength, reinterpret_cast<UCHAR*>(msg));

		if (!msgBuffer.metadata)
		{
			(Arg::Gds(isc_sqlerr) << Arg::Num(-502) <<
				Arg::Gds(isc_dsql_cursor_open_err)).raise();
		}

		if (!statement->fetch(&status, msgBuffer.metadata, reinterpret_cast<UCHAR*>(msg)) &&
			status.isSuccess())
		{
			return 100;
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Release request for an sql statement
ISC_STATUS API_ROUTINE isc_dsql_free_statement(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	USHORT option)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		if (option & DSQL_drop)
		{
			// Release everything
			statement->closeCursor(&status, false);
			statement->closeStatement(&status);
			statement->release();
 			*stmtHandle = 0;
		}
		else if (option & DSQL_unprepare)
		{
			// Release everything but the request itself
			statement->closeCursor(&status, false);
			statement->closeStatement(&status);
		}
		else if (option & DSQL_close)
		{
			// Only close the cursor
			statement->closeCursor(&status, true);
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Insert a BLOB into a dynamic SQL cursor. (deprecated)
ISC_STATUS API_ROUTINE isc_dsql_insert(ISC_STATUS* userStatus, FB_API_HANDLE* /*stmtHandle*/,
	USHORT /*dialect*/, XSQLDA* /*sqlda*/)
{
	(Arg::Gds(isc_feature_removed) << Arg::Str("isc_dsql_insert")).copyTo(userStatus);
	return userStatus[1];
}


// Insert a BLOB into a dynamic SQL cursor. (deprecated)
ISC_STATUS API_ROUTINE isc_dsql_insert_m(ISC_STATUS* userStatus, FB_API_HANDLE* /*stmtHandle*/,
	USHORT /*blrLength*/, const SCHAR* /*blr*/,
	USHORT /*msgType*/, USHORT /*msgLength*/, const SCHAR* /*msg*/)
{
	(Arg::Gds(isc_feature_removed) << Arg::Str("isc_dsql_insert")).copyTo(userStatus);
	return userStatus[1];
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_prepare(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	XSQLDA* sqlda)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));
		RefPtr<YTransaction> transaction;

		if (statement->statement)
		{
			statement->closeStatement(&status);
			if (!status.isSuccess())
				return status[1];
		}

		statement->cursorName = "";

		if (traHandle && *traHandle)
			transaction = translateHandle(transactions, traHandle);

		statement->statement = statement->attachment->prepare(&status, transaction, stmtLength,
			sqlStmt, dialect, IStatement::PREPARE_PREFETCH_METADATA);

		if (status.isSuccess())
		{
			StatusVector tempStatus(NULL);

			RefPtr<IMessageMetadata> parameters(REF_NO_INCR,
				statement->statement->next->getOutputMetadata(&tempStatus));
			tempStatus.check();

			sqldaDescribeParameters(sqlda, parameters);
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Prepare a statement for execution.
ISC_STATUS API_ROUTINE isc_dsql_prepare_m(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	FB_API_HANDLE* stmtHandle, USHORT stmtLength, const SCHAR* sqlStmt, USHORT dialect,
	USHORT itemLength, const SCHAR* items, USHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));
		RefPtr<YTransaction> transaction;

		if (statement->statement)
		{
			statement->closeStatement(&status);
			if (!status.isSuccess())
				return status[1];
		}

		statement->cursorName = "";

		if (traHandle && *traHandle)
			transaction = translateHandle(transactions, traHandle);

		unsigned flags = StatementMetadata::buildInfoFlags(
			itemLength, reinterpret_cast<const UCHAR*>(items));

		statement->statement = statement->attachment->prepare(&status, transaction, stmtLength,
			sqlStmt, dialect, flags);

		if (status.isSuccess())
		{
			StatusVector tempStatus(NULL);
			statement->statement->getInfo(&tempStatus, itemLength, reinterpret_cast<const UCHAR*>(items),
				bufferLength, reinterpret_cast<UCHAR*>(buffer));
			tempStatus.check();
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Set a cursor name for a dynamic request.
ISC_STATUS API_ROUTINE isc_dsql_set_cursor_name(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	const SCHAR* cursorName, USHORT /*cursorType*/)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));

		if (statement->statement && statement->statement->cursor)
			statement->statement->cursor->setCursorName(&status, cursorName);
		else
		{
			if (statement->cursorName.hasData() && statement->cursorName != cursorName)
			{
				(Arg::Gds(isc_dsql_decl_err) <<
				 Arg::Gds(isc_dsql_cursor_redefined) << statement->cursorName).raise();
			}

			statement->cursorName = cursorName;
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on sql statement.
ISC_STATUS API_ROUTINE isc_dsql_sql_info(ISC_STATUS* userStatus, FB_API_HANDLE* stmtHandle,
	SSHORT itemLength, const SCHAR* items, SSHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<IscStatement> statement(translateHandle(statements, stmtHandle));
		statement->checkPrepared(isc_info_unprepared_stmt);

		statement->statement->getInfo(&status, USHORT(itemLength), reinterpret_cast<const UCHAR*>(items),
			USHORT(bufferLength), reinterpret_cast<UCHAR*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Enable access to a specific subsystem. If no subsystem has been explicitly enabled, all are
// available.
int API_ROUTINE gds__enable_subsystem(TEXT* /*subsystem*/)
{
	return FALSE;
}


namespace
{
	class WaitCallback FB_FINAL : public RefCntIface<IEventCallback, FB_EVENT_CALLBACK_VERSION>
	{
	public:
		explicit WaitCallback(UCHAR* aBuffer)
			: buffer(aBuffer)
		{
		}

		// IEventCallback implementation
		virtual void FB_CARG eventCallbackFunction(unsigned int length, const UCHAR* events)
		{
			memcpy(buffer, events, length);
			sem.release();
		}

		int FB_CARG release()
		{
			if (--refCounter == 0)
			{
				delete this;
				return 0;
			}

			return 1;
		}

		UCHAR* buffer;
		Semaphore sem;
	};

}

// Que request for event notification.
ISC_STATUS API_ROUTINE isc_wait_for_event(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	USHORT length, const UCHAR* eventsData, UCHAR* buffer)
{
	StatusVector status(userStatus);
	YEvents* events = NULL;

	RefPtr<WaitCallback> callback(new WaitCallback(buffer));

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		///nullCheck(id, isc_bad_events_handle);

		events = attachment->queEvents(&status, callback, length, eventsData);

		if (!status.isSuccess())
			return status[1];

		callback->sem.enter();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	if (events)
	{
		StatusVector temp(NULL);
		events->cancel(&temp);
	}

	return status[1];
}


namespace
{
	class QueCallback FB_FINAL : public RefCntIface<IEventCallback, FB_EVENT_CALLBACK_VERSION>
	{
	public:
		QueCallback(FPTR_EVENT_CALLBACK aAst, void* aArg)
			: ast(aAst),
			  arg(aArg)
		{
		}

		// IEventCallback implementation
		virtual void FB_CARG eventCallbackFunction(unsigned int length, const UCHAR* events)
		{
			ast(arg, length, events);
		}

		int FB_CARG release()
		{
			if (--refCounter == 0)
			{
				delete this;
				return 0;
			}

			return 1;
		}

		FPTR_EVENT_CALLBACK ast;
		void* arg;
	};
}

// Que request for event notification.
ISC_STATUS API_ROUTINE isc_que_events(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle, SLONG* id,
	USHORT length, const UCHAR* eventsData, FPTR_EVENT_CALLBACK ast, void* arg)
{
	StatusVector status(userStatus);
	YEvents* events = NULL;

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		///nullCheck(idFB_FINAL , isc_bad_events_handle);

		RefPtr<QueCallback> callback(new QueCallback(ast, arg));

		events = attachment->queEvents(&status, callback, length, eventsData);

		if (!status.isSuccess())
			return status[1];

		*id = FB_API_HANDLE_TO_ULONG(events->getHandle());
	}
	catch (const Exception& e)
	{
		if (events)
		{
			*id = 0;
			StatusVector temp(NULL);
			events->cancel(&temp);
		}

		e.stuffException(&status);
	}

	return status[1];
}


// Get a segment from a blob opened for reading.
ISC_STATUS API_ROUTINE isc_get_segment(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle,
	USHORT* returnLength, USHORT bufferLength, UCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));
		unsigned int length = blob->getSegment(&status, bufferLength, buffer);

		if ((status.isSuccess() || status[1] == isc_segment || status[1] == isc_segstr_eof) &&
			returnLength)
		{
			*returnLength = length;
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Snatch a slice of an array.
ISC_STATUS API_ROUTINE isc_get_slice(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, ISC_QUAD* arrayId, USHORT sdlLength, const UCHAR* sdl,
	USHORT paramLength, const UCHAR* param, SLONG sliceLength, UCHAR* slice, SLONG* returnLength)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		int length = attachment->getSlice(&status, transaction, arrayId, sdlLength, sdl,
			paramLength, param, sliceLength, slice);

		if (status.isSuccess() && returnLength)
			*returnLength = length;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Clean up a dangling transaction handle.
ISC_STATUS API_ROUTINE fb_disconnect_transaction(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->disconnect(&status);

		if (status.isSuccess())
			*traHandle = 0;	// ASF: Previous versions wasn't cleaning the handle.
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Open an existing blob.
ISC_STATUS API_ROUTINE isc_open_blob(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId)
{
	return openOrCreateBlob(userStatus, dbHandle, traHandle, blobHandle, blobId, 0, 0, false);
}


// Open an existing blob (extended edition).
ISC_STATUS API_ROUTINE isc_open_blob2(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, FB_API_HANDLE* blobHandle, ISC_QUAD* blobId,
	SSHORT bpbLength, const UCHAR* bpb)
{
	return openOrCreateBlob(userStatus, dbHandle, traHandle, blobHandle, blobId,
		bpbLength, bpb, false);
}


// Prepare a transaction for commit. First phase of a two phase commit.
ISC_STATUS API_ROUTINE isc_prepare_transaction(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	return isc_prepare_transaction2(userStatus, traHandle, 0, NULL);
}


// Prepare a transaction for commit. First phase of a two phase commit.
ISC_STATUS API_ROUTINE isc_prepare_transaction2(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	USHORT msgLength, const UCHAR* msg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->prepare(&status, msgLength, msg);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Put a segment in a blob opened for writing (creation).
ISC_STATUS API_ROUTINE isc_put_segment(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle,
	USHORT bufferLength, const UCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));
		blob->putSegment(&status, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Put a slice in an array.
ISC_STATUS API_ROUTINE isc_put_slice(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, ISC_QUAD* arrayId, USHORT sdlLength, const UCHAR* sdl,
	USHORT paramLength, const SLONG* param, SLONG sliceLength, UCHAR* slice)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		attachment->putSlice(&status, transaction, arrayId, sdlLength, sdl,
			paramLength, reinterpret_cast<const UCHAR*>(param), sliceLength, slice);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Send a record to the host program.
ISC_STATUS API_ROUTINE isc_receive(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle, USHORT msgType,
	USHORT msgLength, SCHAR* msg, SSHORT level)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		request->receive(&status, level, msgType, msgLength, reinterpret_cast<UCHAR*>(msg));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Connect to a transaction in limbo.
ISC_STATUS API_ROUTINE isc_reconnect_transaction(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, SSHORT length, const UCHAR* id)
{
	StatusVector status(userStatus);

	try
	{
		nullCheck(traHandle, isc_bad_trans_handle);

		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));

		YTransaction* transaction = attachment->reconnectTransaction(&status, length, id);

		if (!status.isSuccess())
			status_exception::raise(status);

		*traHandle = transaction->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Release a request.
ISC_STATUS API_ROUTINE isc_release_request(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		request->free(&status);

		if (status.isSuccess())
			*reqHandle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on blob object.
ISC_STATUS API_ROUTINE isc_request_info(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle,
	SSHORT level, SSHORT itemLength, const SCHAR* items, SSHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));

		request->getInfo(&status, level, itemLength, reinterpret_cast<const UCHAR*>(items),
			bufferLength, reinterpret_cast<UCHAR*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}

// API to be used to tell Firebird to reset it's FPE handler - eg: client has an FPE of it's own
// and just changed it. Returns prior setting of the FPE reset flag.
#if defined (SOLARIS) || defined (WIN_NT)
extern "C"
#endif
SLONG API_ROUTINE isc_reset_fpe(USHORT /*fpe_status*/)
{
	// A client-only API call, isc_reset_fpe() is deprecated - we do not use
	// the FPE handler anymore, it can't be used in multithreaded library.
	// Parameter is ignored, it always returns FPE_RESET_ALL_API_CALL,
	// this is the most close code to what we are doing now.

	//static const USHORT FPE_RESET_INIT_ONLY		= 0x0;	// Don't reset FPE after init
	//static const USHORT FPE_RESET_NEXT_API_CALL	= 0x1;	// Reset FPE on next gds call
	static const USHORT FPE_RESET_ALL_API_CALL		= 0x2;	// Reset FPE on all gds call

	return FPE_RESET_ALL_API_CALL;
}


// Abort a transaction, but keep all cursors open.
ISC_STATUS API_ROUTINE isc_rollback_retaining(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->rollbackRetaining(&status);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Abort a transaction.
ISC_STATUS API_ROUTINE isc_rollback_transaction(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));
		transaction->rollback(&status);

		if (status.isSuccess())
			*traHandle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Seek a blob.
ISC_STATUS API_ROUTINE isc_seek_blob(ISC_STATUS* userStatus, FB_API_HANDLE* blobHandle,
	SSHORT mode, SLONG offset, SLONG* result)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YBlob> blob(translateHandle(blobs, blobHandle));
		unsigned int pos = blob->seek(&status, mode, offset);

		if (status.isSuccess() && result)
			*result = pos;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Get a record from the host program.
ISC_STATUS API_ROUTINE isc_send(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle, USHORT msgType,
	USHORT msgLength, const SCHAR* msg, SSHORT level)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		request->send(&status, level, msgType, msgLength, reinterpret_cast<const UCHAR*>(msg));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Attach a service through the first subsystem that recognizes it.
ISC_STATUS API_ROUTINE isc_service_attach(ISC_STATUS* userStatus, USHORT serviceLength,
	const TEXT* serviceName, FB_API_HANDLE* publicHandle, USHORT spbLength, const SCHAR* spb)
{
	StatusVector status(userStatus);
	YService* service = NULL;

	try
	{
		nullCheck(publicHandle, isc_bad_svc_handle);

		if (!serviceName)
			status_exception::raise(Arg::Gds(isc_service_att_err) << Arg::Gds(isc_svc_name_missing));

		string svcName(serviceName, serviceLength ? serviceLength : strlen(serviceName));

		RefPtr<Dispatcher> dispatcher(new Dispatcher);

		dispatcher->setDbCryptCallback(&status, TLS_GET(legacyCryptCallback));
		if (!status.isSuccess())
			return status[1];

		service = dispatcher->attachServiceManager(&status, svcName.c_str(),
			spbLength, reinterpret_cast<const UCHAR*>(spb));
		if (!status.isSuccess())
			return status[1];

		*publicHandle = service->getHandle();
	}
	catch (const Exception& e)
	{
		if (service)
		{
			StatusVector temp(NULL);
			service->detach(&temp);
		}

		e.stuffException(&status);
	}

	return status[1];
}


// Close down a service.
ISC_STATUS API_ROUTINE isc_service_detach(ISC_STATUS* userStatus, FB_API_HANDLE* handle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YService> service(translateHandle(services, handle));
		service->detach(&status);

		if (status.isSuccess())
			*handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on service object.
// NOTE: The parameter RESERVED must not be used for any purpose as there are networking issues
// involved (as with any handle that goes over the network). This parameter will be implemented at
// a later date.
ISC_STATUS API_ROUTINE isc_service_query(ISC_STATUS* userStatus, FB_API_HANDLE* handle,
	ULONG* /*reserved*/, USHORT sendItemLength, const SCHAR* sendItems, USHORT recvItemLength,
	const SCHAR* recvItems, USHORT bufferLength, SCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YService> service(translateHandle(services, handle));

		service->query(&status, sendItemLength, reinterpret_cast<const unsigned char*>(sendItems),
			recvItemLength, reinterpret_cast<const unsigned char*>(recvItems),
			bufferLength, reinterpret_cast<unsigned char*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Starts a service thread.
// NOTE: The parameter RESERVED must not be used for any purpose as there are networking issues
// involved (as with any handle that goes over the network). This parameter will be implemented at
// a later date.
ISC_STATUS API_ROUTINE isc_service_start(ISC_STATUS* userStatus, FB_API_HANDLE* handle,
	ULONG* /*reserved*/, USHORT spbLength, const SCHAR* spb)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YService> service(translateHandle(services, handle));
		service->start(&status, spbLength, reinterpret_cast<const unsigned char*>(spb));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Get a record from the host program.
ISC_STATUS API_ROUTINE isc_start_and_send(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle,
	FB_API_HANDLE* traHandle, USHORT msgType, USHORT msgLength, const SCHAR* msg, SSHORT level)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		request->startAndSend(&status, transaction, level, msgType,
			msgLength, reinterpret_cast<const UCHAR*>(msg));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Get a record from the host program.
ISC_STATUS API_ROUTINE isc_start_request(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle,
	FB_API_HANDLE* traHandle, SSHORT level)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		request->start(&status, transaction, level);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Start a transaction.
ISC_STATUS API_ROUTINE isc_start_multiple(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	SSHORT count, void* vec)
{
	StatusVector status(userStatus);
	TEB* vector = (TEB*) vec;
	YTransaction* multiTrans = NULL;
	DtcStart* ds = NULL;

	try
	{
		nullCheck(traHandle, isc_bad_trans_handle);

		if (count <= 0 || !vector)
			status_exception::raise(Arg::Gds(isc_bad_teb_form));

		if (count == 1)
		{
			RefPtr<YAttachment> attachment(translateHandle(attachments, vector->teb_database));

			YTransaction* transaction = attachment->startTransaction(&status,
				vector->teb_tpb_length, vector->teb_tpb);

			if (transaction)
				*traHandle = transaction->getHandle();

			return status[1];
		}

		HalfStaticArray<DtcStart, 16> dtcStartBuffer;
		ds = dtcStartBuffer.getBuffer(count);
		memset(ds, 0, sizeof(DtcStart) * count);
		DtcStart* const end = ds + count;

		for (DtcStart* p = ds; p < end; ++p, ++vector)
		{
			RefPtr<YAttachment> attachment(translateHandle(attachments, vector->teb_database));
			p->attachment = attachment;
			attachment->addRef();
			p->tpbLength = vector->teb_tpb_length;
			p->tpb = reinterpret_cast<const unsigned char*>(vector->teb_tpb);
		}

		multiTrans = MasterImplementation::dtc->start(&status, count, ds);

		if (multiTrans)
			*traHandle = multiTrans->getHandle();
	}
	catch (const Exception& e)
	{
		StatusVector temp(NULL);

		if (multiTrans)
			multiTrans->rollback(&temp);

		e.stuffException(&status);
	}

	if (ds)
	{
		DtcStart* const end = ds + count;

		for (DtcStart* p = ds; p < end; ++p)
		{
			if (p->attachment)
				p->attachment->release();
		}
	}

	return status[1];
}


// Start a transaction..
ISC_STATUS API_ROUTINE_VARARG isc_start_transaction(ISC_STATUS* userStatus,
	FB_API_HANDLE* traHandle, SSHORT count, ...)
{
	StatusVector status(userStatus);

	try
	{
		HalfStaticArray<TEB, 16> tebs;
		TEB* teb = tebs.getBuffer(count);

		const TEB* const end = teb + count;
		va_list ptr;
		va_start(ptr, count);

		for (TEB* teb_iter = teb; teb_iter < end; teb_iter++)
		{
			teb_iter->teb_database = va_arg(ptr, FB_API_HANDLE*);
			teb_iter->teb_tpb_length = va_arg(ptr, int);
			teb_iter->teb_tpb = va_arg(ptr, UCHAR *);
		}

		va_end(ptr);

		isc_start_multiple(status, traHandle, count, teb);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Execute a procedure.
ISC_STATUS API_ROUTINE isc_transact_request(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle,
	FB_API_HANDLE* traHandle, USHORT blrLength, SCHAR* blr, USHORT inMsgLength, SCHAR* inMsg,
	USHORT outMsgLength, SCHAR* outMsg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		attachment->transactRequest(&status, transaction,
			blrLength, reinterpret_cast<unsigned char*>(blr),
			inMsgLength, reinterpret_cast<unsigned char*>(inMsg),
			outMsgLength, reinterpret_cast<unsigned char*>(outMsg));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Register a transaction specific cleanup handler.
ISC_STATUS API_ROUTINE gds__transaction_cleanup(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	TransactionCleanupRoutine* routine, void* arg)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		AutoPtr<CleanupCallbackImpl<YTransaction, TransactionCleanupRoutine> > callback(
			new CleanupCallbackImpl<YTransaction, TransactionCleanupRoutine>(transaction, routine, arg));

		transaction->addCleanupHandler(&status, callback);

		if (status.isSuccess())
			callback.release();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Provide information on transaction object.
ISC_STATUS API_ROUTINE isc_transaction_info(ISC_STATUS* userStatus, FB_API_HANDLE* traHandle,
	SSHORT itemLength, const SCHAR* items, SSHORT bufferLength, UCHAR* buffer)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YTransaction> transaction(translateHandle(transactions, traHandle));

		transaction->getInfo(&status, itemLength, reinterpret_cast<const UCHAR*>(items),
			bufferLength, reinterpret_cast<UCHAR*>(buffer));
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Unwind a running request. This is potentially nasty since it can be called asynchronously.
ISC_STATUS API_ROUTINE isc_unwind_request(ISC_STATUS* userStatus, FB_API_HANDLE* reqHandle,
	SSHORT level)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YRequest> request(translateHandle(requests, reqHandle));
		request->unwind(&status, level);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Shutdown firebird.
int API_ROUTINE fb_shutdown(unsigned int timeout, const int reason)
{
	StatusVector status(NULL);
	RefPtr<Dispatcher> dispatcher(new Dispatcher);

	dispatcher->shutdown(&status, timeout, reason);
	return status.isSuccess() ? FB_SUCCESS : FB_FAILURE;
}


// Register client callback to be called when FB is going down.
ISC_STATUS API_ROUTINE fb_shutdown_callback(ISC_STATUS* userStatus, FB_SHUTDOWN_CALLBACK callBack,
	const int mask, void* arg)
{
	StatusVector status(userStatus);

	try
	{
		ShutChain::add(callBack, mask, arg);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Check the attachment handle for persistent errors.
ISC_STATUS API_ROUTINE fb_ping(ISC_STATUS* userStatus, FB_API_HANDLE* dbHandle)
{
	StatusVector status(userStatus);

	try
	{
		RefPtr<YAttachment> attachment(translateHandle(attachments, dbHandle));
		attachment->ping(&status);
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Get the legacy handle of a database.
ISC_STATUS API_ROUTINE fb_get_database_handle(ISC_STATUS* userStatus, FB_API_HANDLE* handle, void* obj)
{
	StatusVector status(userStatus);

	try
	{
		YAttachment* yObject = static_cast<YAttachment*>(obj);
		*handle = yObject->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


// Get the legacy handle of a transaction.
ISC_STATUS API_ROUTINE fb_get_transaction_handle(ISC_STATUS* userStatus, FB_API_HANDLE* handle, void* obj)
{
	StatusVector status(userStatus);

	try
	{
		YTransaction* yObject = static_cast<YTransaction*>(obj);
		*handle = yObject->getHandle();
	}
	catch (const Exception& e)
	{
		e.stuffException(&status);
	}

	return status[1];
}


//-------------------------------------

namespace Why {

IAttachment* MasterImplementation::registerAttachment(IProvider* provider, IAttachment* attachment)
{
	return new YAttachment(provider, attachment, "");
}

ITransaction* MasterImplementation::registerTransaction(IAttachment* attachment,
	ITransaction* transaction)
{
	return new YTransaction(static_cast<YAttachment*>(attachment), transaction);
}

template <typename Impl, typename Intf, int Vers>
YHelper<Impl, Intf, Vers>::YHelper(Intf* aNext)
{
	MasterInterfacePtr()->upgradeInterface(aNext, Vers, upInfo);
	next = aNext;
	this->addRef();
}

//-------------------------------------


YEvents::YEvents(YAttachment* aAttachment, IEvents* aNext, IEventCallback* aCallback)
	: YHelper<YEvents, IEvents, FB_EVENTS_VERSION>(aNext),
	  attachment(aAttachment),
	  callback(aCallback)
{
	attachment->childEvents.add(this);
}

FB_API_HANDLE& YEvents::getHandle()
{
	if (!handle)
		makeHandle(&events, this, handle);
	return handle;
}

void YEvents::destroy()
{
	attachment->childEvents.remove(this);

	removeHandle(&events, handle);

	destroy2();
}

void YEvents::cancel(IStatus* status)
{
	try
	{
		YEntry<YEvents> entry(status, this);

		entry.next()->cancel(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}


//-------------------------------------


YRequest::YRequest(YAttachment* aAttachment, IRequest* aNext)
	: YHelper<YRequest, IRequest, FB_REQUEST_VERSION>(aNext),
	  attachment(aAttachment),
	  userHandle(NULL)
{
	attachment->childRequests.add(this);
}

FB_API_HANDLE& YRequest::getHandle()
{
	if (!handle)
		makeHandle(&requests, this, handle);
	return handle;
}

void YRequest::destroy()
{
	if (userHandle)
	{
		*userHandle = 0;
		userHandle = NULL;
	}

	attachment->childRequests.remove(this);

	removeHandle(&requests, handle);

	destroy2();
}

void YRequest::receive(IStatus* status, int level, unsigned int msgType,
	unsigned int length, unsigned char* message)
{
	try
	{
		YEntry<YRequest> entry(status, this);
		entry.next()->receive(status, level, msgType, length, message);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::send(IStatus* status, int level, unsigned int msgType,
	unsigned int length, const unsigned char* message)
{
	try
	{
		YEntry<YRequest> entry(status, this);
		entry.next()->send(status, level, msgType, length, message);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::getInfo(IStatus* status, int level, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		YEntry<YRequest> entry(status, this);
		entry.next()->getInfo(status, level, itemsLength, items, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::start(IStatus* status, ITransaction* transaction, int level)
{
	try
	{
		YEntry<YRequest> entry(status, this);

		NextTransaction trans;
		attachment->getNextTransaction(status, transaction, trans);
		entry.next()->start(status, trans, level);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::startAndSend(IStatus* status, ITransaction* transaction, int level,
	unsigned int msgType, unsigned int length, const unsigned char* message)
{
	try
	{
		YEntry<YRequest> entry(status, this);

		NextTransaction trans;
		attachment->getNextTransaction(status, transaction, trans);
		entry.next()->startAndSend(status, trans, level, msgType, length, message);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::unwind(IStatus* status, int level)
{
	try
	{
		YEntry<YRequest> entry(status, this);
		entry.next()->unwind(status, level);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YRequest::free(IStatus* status)
{
	try
	{
		YEntry<YRequest> entry(status, this);

		entry.next()->free(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}


//-------------------------------------


YBlob::YBlob(YAttachment* aAttachment, YTransaction* aTransaction, IBlob* aNext)
	: YHelper<YBlob, IBlob, FB_BLOB_VERSION>(aNext),
	  attachment(aAttachment),
	  transaction(aTransaction)
{
	attachment->childBlobs.add(this);
	transaction->childBlobs.add(this);
}

FB_API_HANDLE& YBlob::getHandle()
{
	if (!handle)
		makeHandle(&blobs, this, handle);
	return handle;
}

void YBlob::destroy()
{
	attachment->childBlobs.remove(this);
	transaction->childBlobs.remove(this);

	removeHandle(&blobs, handle);

	destroy2();
}

void YBlob::getInfo(IStatus* status, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		YEntry<YBlob> entry(status, this);
		entry.next()->getInfo(status, itemsLength, items, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

unsigned int YBlob::getSegment(IStatus* status, unsigned int length, void* buffer)
{
	try
	{
		YEntry<YBlob> entry(status, this);
		return entry.next()->getSegment(status, length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}

void YBlob::putSegment(IStatus* status, unsigned int length, const void* buffer)
{
	try
	{
		YEntry<YBlob> entry(status, this);
		entry.next()->putSegment(status, length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YBlob::cancel(IStatus* status)
{
	try
	{
		YEntry<YBlob> entry(status, this);

		entry.next()->cancel(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YBlob::close(IStatus* status)
{
	try
	{
		YEntry<YBlob> entry(status, this);

		entry.next()->close(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

int YBlob::seek(IStatus* status, int mode, int offset)
{
	try
	{
		YEntry<YBlob> entry(status, this);
		return entry.next()->seek(status, mode, offset);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}


//-------------------------------------


YStatement::YStatement(YAttachment* aAttachment, IStatement* aNext)
	: YHelper<YStatement, IStatement, FB_STATEMENT_VERSION>(aNext),
	  attachment(aAttachment), cursor(NULL), input(true), output(false)
{
	attachment->childStatements.add(this);
}

void YStatement::destroy()
{
	{	// scope
		MutexLockGuard guard(statementMutex, FB_FUNCTION);
		if (cursor)
		{
			cursor->destroy();
			cursor = NULL;
		}
	}

	attachment->childStatements.remove(this);

	removeHandle(&statements, handle);

	destroy2();
}

void YStatement::getInfo(IStatus* status, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		entry.next()->getInfo(status, itemsLength, items, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

unsigned YStatement::getType(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return entry.next()->getType(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}

unsigned YStatement::getFlags(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return entry.next()->getFlags(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}

const char* YStatement::getPlan(IStatus* status, FB_BOOLEAN detailed)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return entry.next()->getPlan(status, detailed);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

IMessageMetadata* YMetadata::get(IStatement* next, YStatement* statement)
{
	if (!flag)
	{
		MutexLockGuard guard(statement->statementMutex, FB_FUNCTION);
		if (!flag)
		{
			RefPtr<IMessageMetadata> nextMeta(REF_NO_INCR, statement->getMetadata(input, next));
			metadata = new MsgMetadata(nextMeta);

			flag = true;
		}
	}

	metadata->addRef();
	return metadata;
}

IMessageMetadata* YStatement::getMetadata(bool in, IStatement* next)
{
	LocalStatus status;
	IMessageMetadata* rc = in ? next->getInputMetadata(&status) : next->getOutputMetadata(&status);
	if (!status.isSuccess())
		status_exception::raise(status.get());
	return rc;
}

IMessageMetadata* YStatement::getInputMetadata(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return input.get(entry.next(), this);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

IMessageMetadata* YStatement::getOutputMetadata(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return output.get(entry.next(), this);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

FB_UINT64 YStatement::getAffectedRecords(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		return entry.next()->getAffectedRecords(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}

ITransaction* YStatement::execute(IStatus* status, ITransaction* transaction,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		NextTransaction trans;
		if (transaction)
			attachment->getNextTransaction(status, transaction, trans);

		ITransaction* newTrans = entry.next()->execute(status, trans, inMetadata, inBuffer,
			outMetadata, outBuffer);

		if (newTrans)
		{
			if (newTrans == trans)
				newTrans = transaction;
			else
			{
				transaction = NULL;		// Get ready for correct return in OOM case
				newTrans = new YTransaction(attachment, newTrans);
			}
		}

		return newTrans;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return transaction;
}

IResultSet* YStatement::openCursor(Firebird::IStatus* status, ITransaction* transaction,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		NextTransaction trans;
		if (transaction)
			attachment->getNextTransaction(status, transaction, trans);

		IResultSet* rs = entry.next()->openCursor(status, trans, inMetadata, inBuffer, outMetadata);
		if (!status->isSuccess())
		{
			return NULL;
		}
		fb_assert(rs);

		YTransaction* const yTra = attachment->getTransaction(status, transaction);

		return new YResultSet(attachment, yTra, this, rs);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

void YStatement::free(IStatus* status)
{
	try
	{
		YEntry<YStatement> entry(status, this);

		entry.next()->free(status);
		if (!status->isSuccess())
		{
			return;
		}

		destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}


//-------------------------------------


YResultSet::YResultSet(YAttachment* anAttachment, YTransaction* aTransaction, IResultSet* aNext)
	: YHelper<YResultSet, IResultSet, FB_RESULTSET_VERSION>(aNext),
	  attachment(anAttachment),
	  transaction(aTransaction),
	  statement(NULL)
{
	fb_assert(aTransaction && aNext);
	transaction->childCursors.add(this);
}

YResultSet::YResultSet(YAttachment* anAttachment, YTransaction* aTransaction,
			YStatement* aStatement, IResultSet* aNext)
	: YHelper<YResultSet, IResultSet, FB_RESULTSET_VERSION>(aNext),
	  attachment(anAttachment),
	  transaction(aTransaction),
	  statement(aStatement)
{
	fb_assert(aTransaction && aNext);
	transaction->childCursors.add(this);

	MutexLockGuard guard(statement->statementMutex, FB_FUNCTION);

	if (statement->cursor)
		Arg::Gds(isc_cursor_already_open).raise();

	statement->cursor = this;
}

void YResultSet::destroy()
{
	if (statement)
	{
		MutexLockGuard guard(statement->statementMutex, FB_FUNCTION);
		fb_assert(statement->cursor == this);
		statement->cursor = NULL;
	}

	fb_assert(transaction);
	transaction->childCursors.remove(this);

	destroy2();
}

void YResultSet::setCursorName(IStatus* status, const char* name)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		entry.next()->setCursorName(status, name);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YResultSet::setDelayedOutputFormat(IStatus* status, IMessageMetadata* format)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		entry.next()->setDelayedOutputFormat(status, format);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

FB_BOOLEAN YResultSet::fetchNext(IStatus* status, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchNext(status, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::fetchPrior(IStatus* status, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchPrior(status, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::fetchFirst(IStatus* status, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchFirst(status, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::fetchLast(IStatus* status, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchLast(status, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::fetchAbsolute(IStatus* status, unsigned int position, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchAbsolute(status, position, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::fetchRelative(IStatus* status, int offset, void* buffer)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->fetchRelative(status, offset, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::isEof(IStatus* status)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->isEof(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

FB_BOOLEAN YResultSet::isBof(IStatus* status)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->isBof(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return FB_FALSE;
}

IMessageMetadata* YResultSet::getMetadata(IStatus* status)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		return entry.next()->getMetadata(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

void YResultSet::close(IStatus* status)
{
	try
	{
		YEntry<YResultSet> entry(status, this);

		entry.next()->close(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}


//-------------------------------------


YTransaction::YTransaction(YAttachment* aAttachment, ITransaction* aNext)
	: YHelper<YTransaction, ITransaction, FB_TRANSACTION_VERSION>(aNext),
	  attachment(aAttachment),
	  childBlobs(getPool()),
	  childCursors(getPool()),
	  cleanupHandlers(getPool())
{
	if (attachment)
		attachment->childTransactions.add(this);
}

FB_API_HANDLE& YTransaction::getHandle()
{
	if (!handle)
		makeHandle(&transactions, this, handle);
	return handle;
}

void YTransaction::destroy()
{
	for (CleanupCallback** handler = cleanupHandlers.begin();
		 handler != cleanupHandlers.end();
		 ++handler)
	{
		(*handler)->cleanupCallbackFunction();
	}

	cleanupHandlers.clear();

	childBlobs.destroy();
	childCursors.destroy();

	if (attachment)
		attachment->childTransactions.remove(this);

	removeHandle(&transactions, handle);

	destroy2();
}

void YTransaction::getInfo(IStatus* status, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
	Array<unsigned char> newItemsBuffer;

	try
	{
		YEntry<YTransaction> entry(status, this);

		fb_utils::getDbPathInfo(itemsLength, items, bufferLength, buffer,
								newItemsBuffer, attachment->dbPath);

		entry.next()->getInfo(status, itemsLength, items, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::prepare(IStatus* status, unsigned int msgLength, const unsigned char* message)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		entry.next()->prepare(status, msgLength, message);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::commit(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		entry.next()->commit(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::commitRetaining(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		entry.next()->commitRetaining(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::rollback(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		entry.next()->rollback(status);
		if (isNetworkError(status))
			status->init();

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::rollbackRetaining(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		entry.next()->rollbackRetaining(status);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::disconnect(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		/*** ASF: We must call the provider, but this makes the shutdown to crash currently.
		for (YTransaction* i = this; i; i = i->sub)
		{
			if (i->next)
			{
				i->next->disconnect(status);

				if (!status->isSuccess())
					return;

				i->next = NULL;
			}
		}
		***/

		destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YTransaction::addCleanupHandler(IStatus* status, CleanupCallback* callback)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		cleanupHandlers.add(callback);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}

void YTransaction::selfCheck()
{
	if (!next)
		Arg::Gds(isc_bad_trans_handle).raise();
}

ITransaction* FB_CARG YTransaction::join(IStatus* status, ITransaction* transaction)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		return DtcInterfacePtr()->join(status, this, transaction);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

ITransaction* FB_CARG YTransaction::validate(IStatus* status, IAttachment* testAtt)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		// Do not raise error in status - just return NULL if attachment does not match
		if (attachment == testAtt)
			return this;

		return entry.next()->validate(status, testAtt);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}

YTransaction* FB_CARG YTransaction::enterDtc(IStatus* status)
{
	try
	{
		YEntry<YTransaction> entry(status, this);

		YTransaction* copy = new YTransaction(this);
		// copy is created with zero handle
		copy->addRef();

		if (attachment)
			attachment->childTransactions.remove(this);

		removeHandle(&transactions, handle);
		next = NULL;
		release();

		return copy;
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}

	return NULL;
}


//-------------------------------------


YAttachment::YAttachment(IProvider* aProvider, IAttachment* aNext, const PathName& aDbPath)
	: YHelper<YAttachment, IAttachment, FB_ATTACHMENT_VERSION>(aNext),
	  provider(aProvider),
	  dbPath(getPool(), aDbPath),
	  childBlobs(getPool()),
	  childEvents(getPool()),
	  childRequests(getPool()),
	  childStatements(getPool()),
	  childTransactions(getPool()),
	  cleanupHandlers(getPool())
{
	provider->addRef();
	makeHandle(&attachments, this, handle);
}

FB_API_HANDLE& YAttachment::getHandle()
{
	fb_assert(handle);
	return handle;
}

YAttachment::~YAttachment()
{
	if (provider)
		PluginManagerInterfacePtr()->releasePlugin(provider);
}

void YAttachment::destroy()
{
	for (CleanupCallback** handler = cleanupHandlers.begin();
		 handler != cleanupHandlers.end();
		 ++handler)
	{
		(*handler)->cleanupCallbackFunction();
	}

	cleanupHandlers.clear();

	childRequests.destroy();
	childStatements.destroy();
	childBlobs.destroy();
	childEvents.destroy();
	childTransactions.destroy();

	removeHandle(&attachments, handle);

	destroy2();
}

void YAttachment::shutdown()
{
	if (provider)
	{
		destroy();
		PluginManagerInterfacePtr()->releasePlugin(provider);
		provider = NULL;
	}
}

void YAttachment::getInfo(IStatus* status, unsigned int itemsLength,
	const unsigned char* items, unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		YEntry<YAttachment> entry(status, this);
		entry.next()->getInfo(status, itemsLength, items, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

YStatement* YAttachment::prepare(IStatus* status, ITransaction* transaction,
	unsigned int stmtLength, const char* sqlStmt, unsigned int dialect, unsigned int flags)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		if (!sqlStmt)
			Arg::Gds(isc_command_end_err).raise();

		NextTransaction trans;
		if (transaction)
			getNextTransaction(status, transaction, trans);

		IStatement* st = entry.next()->prepare(status, trans, stmtLength, sqlStmt, dialect, flags);
		if (status->isSuccess())
		{
			return new YStatement(this, st);
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

YTransaction* YAttachment::startTransaction(IStatus* status, unsigned int tpbLength,
	const unsigned char* tpb)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		ITransaction* transaction = entry.next()->startTransaction(status, tpbLength, tpb);
		if (transaction)
			return new YTransaction(this, transaction);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

YTransaction* YAttachment::reconnectTransaction(IStatus* status, unsigned int length,
	const unsigned char* id)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		ITransaction* transaction = entry.next()->reconnectTransaction(status, length, id);

		if (transaction)
			return new YTransaction(this, transaction);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

YRequest* YAttachment::compileRequest(IStatus* status, unsigned int blrLength,
	const unsigned char* blr)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		IRequest* request = entry.next()->compileRequest(status, blrLength, blr);
		return request ? new YRequest(this, request) : NULL;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

void YAttachment::transactRequest(IStatus* status, ITransaction* transaction,
	unsigned int blrLength, const unsigned char* blr,
	unsigned int inMsgLength, const unsigned char* inMsg,
	unsigned int outMsgLength, unsigned char* outMsg)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		getNextTransaction(status, transaction, trans);

		entry.next()->transactRequest(status, trans, blrLength, blr, inMsgLength, inMsg,
			outMsgLength, outMsg);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

YBlob* YAttachment::createBlob(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
	unsigned int bpbLength, const unsigned char* bpb)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		YTransaction* yTra = getTransaction(status, transaction);
		NextTransaction nextTra(yTra->next);

		IBlob* blob = entry.next()->createBlob(status, nextTra, id, bpbLength, bpb);
		return blob ? new YBlob(this, yTra, blob) : NULL;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

YBlob* YAttachment::openBlob(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
	unsigned int bpbLength, const unsigned char* bpb)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		YTransaction* yTra = getTransaction(status, transaction);
		NextTransaction nextTra(yTra->next);

		IBlob* blob = entry.next()->openBlob(status, nextTra, id, bpbLength, bpb);
		return blob ? new YBlob(this, yTra, blob) : NULL;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

int YAttachment::getSlice(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
	unsigned int sdlLength, const unsigned char* sdl, unsigned int paramLength,
	const unsigned char* param, int sliceLength, unsigned char* slice)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		getNextTransaction(status, transaction, trans);

		return entry.next()->getSlice(status, trans, id, sdlLength, sdl, paramLength, param,
			sliceLength, slice);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return 0;
}

void YAttachment::putSlice(IStatus* status, ITransaction* transaction, ISC_QUAD* id,
	unsigned int sdlLength, const unsigned char* sdl, unsigned int paramLength,
	const unsigned char* param, int sliceLength, unsigned char* slice)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		getNextTransaction(status, transaction, trans);

		entry.next()->putSlice(status, trans, id, sdlLength, sdl, paramLength, param, sliceLength, slice);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YAttachment::executeDyn(IStatus* status, ITransaction* transaction, unsigned int length,
	const unsigned char* dyn)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		getNextTransaction(status, transaction, trans);

		return entry.next()->executeDyn(status, trans, length, dyn);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

IResultSet* YAttachment::openCursor(IStatus* status, ITransaction* transaction,
	unsigned int length, const char* string, unsigned int dialect,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata)
{
	IResultSet* rs = NULL;
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		if (transaction)
			getNextTransaction(status, transaction, trans);

		rs = entry.next()->openCursor(status, trans, length, string, dialect,
			inMetadata, inBuffer, outMetadata);

		YTransaction* const yTra = getTransaction(status, transaction);

		return new YResultSet(this, yTra, rs);
	}
	catch (const Exception& e)
	{
		if (rs)
			rs->close(status);
		e.stuffException(status);
	}

	return NULL;
}

ITransaction* YAttachment::execute(IStatus* status, ITransaction* transaction,
	unsigned int length, const char* string, unsigned int dialect,
	IMessageMetadata* inMetadata, void* inBuffer, IMessageMetadata* outMetadata, void* outBuffer)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		NextTransaction trans;
		if (transaction)
			getNextTransaction(status, transaction, trans);

		ITransaction* newTrans = entry.next()->execute(status, trans, length, string, dialect,
			inMetadata, inBuffer, outMetadata, outBuffer);

		if (newTrans)
		{
			if (newTrans == trans)
				newTrans = transaction;
			else
			{
				transaction = NULL;		// Get ready for correct return in OOM case
				newTrans = new YTransaction(this, newTrans);
			}
		}

		return newTrans;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return transaction;
}


void YAttachment::execute(Firebird::IStatus* status, FB_API_HANDLE* traHandle,
		unsigned int stmtLength, const char* sqlStmt, unsigned int dialect,
		Firebird::IMessageMetadata* inMetadata, void* inBuffer,
		Firebird::IMessageMetadata* outMetadata, void* outBuffer)
{
	RefPtr<YTransaction> transaction;

	if (traHandle && *traHandle)
		transaction = translateHandle(transactions, traHandle);

	ITransaction* newTrans = execute(status, transaction, stmtLength, sqlStmt,
		dialect, inMetadata, inBuffer, outMetadata, outBuffer);

	if (status->isSuccess())
	{
		if (transaction && !newTrans)
		{
			transaction->destroy();
			*traHandle = 0;
		}
		else if (!transaction && newTrans)
		{
			// in this case we know for sure that newTrans points to YTransaction
			if (traHandle)
				*traHandle = static_cast<YTransaction*>(newTrans)->getHandle();
		}
	}
}

YEvents* YAttachment::queEvents(IStatus* status, IEventCallback* callback,
	unsigned int length, const unsigned char* eventsData)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		IEvents* events = entry.next()->queEvents(status, callback, length, eventsData);
		return events ? new YEvents(this, events, callback) : NULL;
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

void YAttachment::cancelOperation(IStatus* status, int option)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		// Mutex will be locked here for a really long time.
		MutexLockGuard guard(enterMutex, FB_FUNCTION);

		if (enterCount > 1 || option != fb_cancel_raise)
			entry.next()->cancelOperation(status, option);
		else
			status_exception::raise(Arg::Gds(isc_nothing_to_cancel));
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YAttachment::ping(IStatus* status)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		entry.next()->ping(status);

		if (!status->isSuccess())
		{
			if (!savedStatus.getError())
				savedStatus.save(status->get());

			StatusVector temp(NULL);
			entry.next()->detach(&temp);
			next = NULL;

			status_exception::raise(savedStatus.value());
		}
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YAttachment::detach(IStatus* status)
{
	try
	{
		YEntry<YAttachment> entry(status, this, 0);

		if (entry.next())
			entry.next()->detach(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YAttachment::dropDatabase(IStatus* status)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		entry.next()->dropDatabase(status);

		if (status->isSuccess())
			destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YAttachment::addCleanupHandler(IStatus* status, CleanupCallback* callback)
{
	try
	{
		YEntry<YAttachment> entry(status, this);

		cleanupHandlers.add(callback);
	}
	catch (const Exception& ex)
	{
		ex.stuffException(status);
	}
}


YTransaction* YAttachment::getTransaction(IStatus* status, ITransaction* tra)
{
	if (!tra)
		Arg::Gds(isc_bad_trans_handle).raise();

	status->init();

	// If validation is successfull, this means that this attachment and valid transaction
	// use same provider. I.e. the following cast is safe.
	YTransaction* yt = static_cast<YTransaction*>(tra->validate(status, this));
	if (!status->isSuccess())
		status_exception::raise(status->get());
	if (!yt)
		Arg::Gds(isc_bad_trans_handle).raise();

	yt->selfCheck();
	return yt;
}


void YAttachment::getNextTransaction(IStatus* status, ITransaction* tra, NextTransaction& next)
{
	next = getTransaction(status, tra)->next;
	if (!next.hasData())
		Arg::Gds(isc_bad_trans_handle).raise();
}


//-------------------------------------


YService::YService(IProvider* aProvider, IService* aNext, bool utf8)
	: YHelper<YService, Firebird::IService, FB_SERVICE_VERSION>(aNext),
	  provider(aProvider),
	  utf8Connection(utf8)
{
	provider->addRef();
	makeHandle(&services, this, handle);
}

FB_API_HANDLE& YService::getHandle()
{
	fb_assert(handle);
	return handle;
}

YService::~YService()
{
	if (provider)
		PluginManagerInterfacePtr()->releasePlugin(provider);
}

void YService::destroy()
{
	removeHandle(&services, handle);

	next = NULL;
	release();
}

void YService::shutdown()
{
	if (provider)
	{
		destroy();
		PluginManagerInterfacePtr()->releasePlugin(provider);
		provider = NULL;
	}
}

void YService::detach(IStatus* status)
{
	try
	{
		YEntry<YService> entry(status, this, 1);

		if (entry.next())
			entry.next()->detach(status);

		destroy();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YService::query(IStatus* status, unsigned int sendLength, const unsigned char* sendItems,
	unsigned int receiveLength, const unsigned char* receiveItems,
	unsigned int bufferLength, unsigned char* buffer)
{
	try
	{
		YEntry<YService> entry(status, this);
		entry.next()->query(status, sendLength, sendItems,
			receiveLength, receiveItems, bufferLength, buffer);
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}

void YService::start(IStatus* status, unsigned int spbLength, const unsigned char* spbItems)
{
	try
	{
		ClumpletWriter spb(ClumpletReader::SpbStart, MAX_DPB_SIZE, spbItems, spbLength);
		if (!utf8Connection)
		{
			IntlSpbStart().toUtf8(spb, 0);
		}

		YEntry<YService> entry(status, this);
		entry.next()->start(status, spb.getBufferLength(), spb.getBuffer());
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}
}


//-------------------------------------


// Attach a database through the first subsystem that recognizes it.
YAttachment* Dispatcher::attachDatabase(IStatus* status, const char* filename,
	unsigned int dpbLength, const unsigned char* dpb)
{
	return attachOrCreateDatabase(status, false, filename, dpbLength, dpb);
}

// Create new database using the first subsystem that can do it.
YAttachment* Dispatcher::createDatabase(IStatus* status, const char* filename,
	unsigned int dpbLength, const unsigned char* dpb)
{
	return attachOrCreateDatabase(status, true, filename, dpbLength, dpb);
}

YAttachment* Dispatcher::attachOrCreateDatabase(Firebird::IStatus* status, bool createFlag,
	const char* filename, unsigned int dpbLength, const unsigned char* dpb)
{
	try
	{
		DispatcherEntry entry(status);

		if (!filename)
			status_exception::raise(Arg::Gds(isc_bad_db_format) << Arg::Str(""));

		if (dpbLength > 0 && !dpb)
			status_exception::raise(Arg::Gds(isc_bad_dpb_form));

		ClumpletWriter newDpb(ClumpletReader::dpbList, MAX_DPB_SIZE, dpb, dpbLength);
		bool utfData = newDpb.find(isc_dpb_utf8_filename);

		// Take care about DPB
		setLogin(newDpb, false);
		if (!utfData)
		{
			IntlDpb().toUtf8(newDpb, isc_dpb_utf8_filename);
		}

		// Take care about filename
		PathName orgFilename(filename);
		if (utfData)
		{
			ISC_utf8ToSystem(orgFilename);
		}
		orgFilename.rtrim();

		PathName expandedFilename;
		RefPtr<Config> config;
		if (expandDatabaseName(orgFilename, expandedFilename, &config))
		{
			expandedFilename = orgFilename;
		}

		if (newDpb.find(isc_dpb_config))
		{
			string dpb_config;
			newDpb.getString(dpb_config);
			Config::merge(config, &dpb_config);
		}

		// Convert to UTF8
		ISC_systemToUtf8(orgFilename);
		ISC_systemToUtf8(expandedFilename);

		// Add original filename to DPB
		if (orgFilename != expandedFilename && !newDpb.find(isc_dpb_org_filename))
			newDpb.insertPath(isc_dpb_org_filename, orgFilename);

		StatusVector temp(NULL);
		IStatus* currentStatus = status;

		for (GetPlugins<IProvider> providerIterator(PluginType::Provider,
				FB_PROVIDER_VERSION, upInfo, config);
			 providerIterator.hasData();
			 providerIterator.next())
		{
			IProvider* provider = providerIterator.plugin();

			if (cryptCallback)
			{
				provider->setDbCryptCallback(currentStatus, cryptCallback);
				if (!currentStatus->isSuccess())
					continue;
			}

			IAttachment* attachment = createFlag ?
				provider->createDatabase(currentStatus,	expandedFilename.c_str(),
					newDpb.getBufferLength(), newDpb.getBuffer()) :
				provider->attachDatabase(currentStatus,	expandedFilename.c_str(),
					newDpb.getBufferLength(), newDpb.getBuffer());

			if (currentStatus->isSuccess())
			{
#ifdef WIN_NT
				if (createFlag)
				{
	            	// Now we can expand, the file exists
					ISC_utf8ToSystem(orgFilename);
					if (expandDatabaseName(orgFilename, expandedFilename, NULL))
					{
						expandedFilename = orgFilename;
					}
					ISC_systemToUtf8(expandedFilename);
				}
#endif

				status->set(currentStatus->get());
				return new YAttachment(provider, attachment, expandedFilename);
			}

			if (currentStatus->get()[1] != isc_unavailable)
				currentStatus = &temp;

			currentStatus->init();
		}

		if (status->get()[1] == 0)
			Arg::Gds(isc_unavailable).raise();
	}
	catch (const Exception& e)
	{
		e.stuffException(status);
	}

	return NULL;
}

// Attach a service through the first subsystem that recognizes it.
YService* Dispatcher::attachServiceManager(IStatus* status, const char* serviceName,
	unsigned int spbLength, const unsigned char* spb)
{
	IService* service = NULL;

	try
	{
		DispatcherEntry entry(status);

		if (!serviceName)
			status_exception::raise(Arg::Gds(isc_service_att_err) << Arg::Gds(isc_svc_name_missing));

		if (spbLength > 0 && !spb)
			status_exception::raise(Arg::Gds(isc_bad_spb_form) <<
 									Arg::Gds(isc_null_spb));

		PathName svcName(serviceName);
		svcName.trim();

		// Take care about SPB
		ClumpletWriter spbWriter(ClumpletReader::spbList, MAX_DPB_SIZE, spb, spbLength);
		setLogin(spbWriter, true);
		bool utfData = spbWriter.find(isc_spb_utf8_filename);
		if (!utfData)
		{
			IntlSpb().toUtf8(spbWriter, isc_spb_utf8_filename);
		}

		// Build correct config
		RefPtr<Config> config(Config::getDefaultConfig());
		if (spbWriter.find(isc_spb_config))
		{
			string spb_config;
			spbWriter.getString(spb_config);
			Config::merge(config, &spb_config);
		}

		for (GetPlugins<IProvider> providerIterator(PluginType::Provider,
				FB_PROVIDER_VERSION, upInfo, config);
			 providerIterator.hasData();
			 providerIterator.next())
		{
			IProvider* p = providerIterator.plugin();

			if (cryptCallback)
			{
				p->setDbCryptCallback(status, cryptCallback);
				if (!status->isSuccess())
					continue;
			}

			service = p->attachServiceManager(status, svcName.c_str(),
				spbWriter.getBufferLength(), spbWriter.getBuffer());

			if (status->isSuccess())
				return new YService(p, service, utfData);
		}

		if (status->isSuccess())
		{
			(Arg::Gds(isc_service_att_err) <<
			 Arg::Gds(isc_no_providers)).copyTo(status);
		}
	}
	catch (const Exception& e)
	{
		if (service)
		{
			StatusVector temp(NULL);
			service->detach(&temp);
		}

		e.stuffException(status);
	}

	return NULL;
}

void Dispatcher::shutdown(IStatus* userStatus, unsigned int timeout, const int reason)
{
	try
	{
		DispatcherEntry entry(userStatus, true);

		static GlobalPtr<Mutex> singleShutdown;
		MutexLockGuard guard(singleShutdown, FB_FUNCTION);

		if (shutdownStarted)
			return;

		StatusVector status(NULL);

#ifdef DEV_BUILD
		// Make timeout huge in debug build: hard to debug something during 5-10 sec
		if (timeout)
			timeout = 10 * 3600 * 1000;		// 10 hours
#endif

		// In some cases it's OK to stop plugin manager even now.
		if (reason == fb_shutrsn_exit_called)
			PluginManager::shutdown();

		const Arg::Gds error(isc_att_shutdown);	//// TODO: review

		// Ask clients about shutdown confirmation.
		if (ShutChain::run(fb_shut_confirmation, reason) != FB_SUCCESS)
		{
			userStatus->set(error.value());
			return;	// Do not perform former shutdown
		}

		// Shut known components, internal for libfbclient
		PluginManager::shutdown();
		shutdownTimers();

		// Shutdown clients before providers
		if (ShutChain::run(fb_shut_preproviders, reason) != FB_SUCCESS)
			userStatus->set(error.value());

		// Shutdown yValve
		// Since this moment no new thread will be able to enter yValve.
		// Existing threads continue to run inside it - later do our best to close them.
		shutdownStarted = true;

		// Shutdown providers (if any present).
		for (GetPlugins<IProvider> providerIterator(PluginType::Provider,
				FB_PROVIDER_VERSION, upInfo);
			 providerIterator.hasData();
			 providerIterator.next())
		{
			IProvider* provider = providerIterator.plugin();

			StatusVector status2(NULL);
			provider->shutdown(&status2, timeout, reason);

			if (status2[1])
				userStatus->set(error.value());
		}

		// Close all known interfaces from providers...
		for (bool hasThreads = true; hasThreads; )
		{
			THD_yield();

			if (dispCounter.value() > 0)
			{
				continue;
			}

			hasThreads = false;

			Stack<YService*, 64> svcStack;
			{
				WriteLockGuard sync(handleMappingLock, FB_FUNCTION);
				GenericMap<Pair<NonPooled<FB_API_HANDLE, YService*> > >::Accessor accessor(&services);

				if (accessor.getFirst())
				{
					do
					{
						YService* service = accessor.current()->second;
						if (service->enterCount)
							hasThreads = true;
						else
						{
							service->addRef();
							svcStack.push(service);
						}
					} while (accessor.getNext());
				}
			}

			while (svcStack.hasData())
			{
				YService* service = svcStack.pop();
				service->shutdown();
				service->release();
			}

			if (hasThreads)
				continue;

			Stack<YAttachment*, 64> attStack;
			{
				WriteLockGuard sync(handleMappingLock, FB_FUNCTION);
				GenericMap<Pair<NonPooled<FB_API_HANDLE, YAttachment*> > >::Accessor accessor(&attachments);

				if (accessor.getFirst())
				{
					do
					{
						YAttachment* attachment = accessor.current()->second;
						if (attachment->enterCount)
							hasThreads = true;
						else
						{
							attachment->addRef();
							attStack.push(attachment);
						}
					} while (accessor.getNext());
				}
			}

			while (attStack.hasData())
			{
				YAttachment* attachment = attStack.pop();
				attachment->shutdown();
				attachment->release();
			}

		}

		// ... and wait for all providers to go away
		PluginManager::waitForType(PluginType::Provider);

		// Shutdown clients after providers.
		if (ShutChain::run(fb_shut_postproviders, reason) != FB_SUCCESS)
			userStatus->set(error.value());

		// Finish shutdown.
		if (ShutChain::run(fb_shut_finish, reason) != FB_SUCCESS)
			userStatus->set(error.value());

		// At this step callbacks are welcome to exit (or take actions to make main thread do it).
		if (ShutChain::run(fb_shut_exit, reason) != FB_SUCCESS)
			userStatus->set(error.value());
	}
	catch (const Exception& e)
	{
		e.stuffException(userStatus);
		gds__log_status(0, userStatus->get());
	}
}

void Dispatcher::setDbCryptCallback(IStatus* status, ICryptKeyCallback* callback)
{
	status->init();
	cryptCallback = callback;
}

} // namespace Why
