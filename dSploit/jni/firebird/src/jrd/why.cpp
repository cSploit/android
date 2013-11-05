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
 * 23-Feb-2002 Dmitry Yemanov - Events wildcarding
 *
 *
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old Win3.1 port (Windows_Only)
 * 2002-02-23 Sean Leyne - Code Cleanup, removed old M88K and NCR3000 port
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "IMP" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 * 2002.12.10 Alex Peshkoff: 1. Moved struct hndl declaration to h-file
 * 							 2. renamed all specific objects to WHY_*
 *
 */

#include "firebird.h"
#include "memory_routines.h"	// needed for get_long
#include "consts_pub.h"

#include <stdlib.h>
#include <string.h>
#include "../jrd/common.h"
#include <stdarg.h>

#include <stdio.h>
#include "../jrd/gdsassert.h"

/* includes specific for DSQL */

#include "../dsql/sqlda.h"
#include "../dsql/sqlda_pub.h"
#include "../dsql/prepa_proto.h"
#include "../dsql/utld_proto.h"

/* end DSQL-specific includes */

#include "../jrd/why_proto.h"
#include "../common/classes/alloc.h"
#include "../common/classes/array.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/RefCounted.h"
#include "../jrd/thread_proto.h"
#include "gen/iberror.h"
#include "../jrd/msg_encode.h"
#include "gen/msg_facs.h"
#include "../jrd/acl.h"
#include "../jrd/inf_pub.h"
#include "../jrd/fil.h"
#include "../jrd/db_alias.h"
#include "../jrd/os/path_utils.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/utils_proto.h"
#include "../common/StatusHolder.h"

#include "../jrd/flu_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/isc_s_proto.h"
#include "../jrd/utl_proto.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/auto.h"
#include "../common/classes/init.h"
#include "../common/classes/semaphore.h"
#include "../common/classes/fb_atomic.h"
#include "../common/classes/FpeControl.h"
#include "../jrd/constants.h"
#include "../jrd/ThreadStart.h"
#ifdef SCROLLABLE_CURSORS
#include "../jrd/blr.h"
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SIGNAL_H
#include <signal.h>
#endif

#ifdef HAVE_FLOCK
#include <sys/file.h>			/* for flock() */
#endif

#ifdef WIN_NT
#include <windows.h>
#endif

#ifdef HAVE_SYS_TIMEB_H
#include <sys/timeb.h>
#endif

using namespace Firebird;

const int IO_RETRY	= 20;

inline bool is_network_error(const ISC_STATUS* vector)
{
	return vector[1] == isc_network_error || vector[1] == isc_net_write_err ||
		vector[1] == isc_net_read_err;
}

inline void bad_handle(ISC_STATUS code)
{
	status_exception::raise(Arg::Gds(code));
}

inline void nullCheck(const FB_API_HANDLE* ptr, ISC_STATUS code)
{
	// this function is called for incoming API handles,
	// proposed to be created by the call
	if ((!ptr) || (*ptr))
	{
		bad_handle(code);
	}
}

#if !defined (SUPERCLIENT)
static bool disableConnections = false;
#endif

typedef ISC_STATUS(*PTR) (ISC_STATUS* user_status, ...);

/* Transaction element block */

struct teb
{
	FB_API_HANDLE *teb_database;
	int teb_tpb_length;
	const UCHAR *teb_tpb;
};
typedef teb TEB;

#ifdef DEBUG_GDS_ALLOC
#define alloc(x) alloc_debug(x, __FILE__, __LINE__)
static SCHAR *alloc_debug(SLONG, const char*, int);
#else
static SCHAR *alloc(SLONG);
#endif
static void free_block(void*);
static ISC_STATUS detach_or_drop_database(ISC_STATUS * user_status, FB_API_HANDLE * handle,
										  const int proc, const ISC_STATUS specCode = 0);

namespace Jrd {
	class Attachment;
	class jrd_tra;
	class jrd_req;
	class dsql_req;
}

namespace Why
{
	// process shutdown flag
	bool shutdownStarted = false;

	// flags
	const UCHAR HANDLE_TRANSACTION_limbo	= 0x01;
	const UCHAR HANDLE_STATEMENT_prepared	= 0x02;

	// forwards
	class CAttachment;
	class CTransaction;
	class CRequest;
	class CBlob;
	class CStatement;
	class CService;

	typedef RefPtr<CAttachment> Attachment;
	typedef RefPtr<CTransaction> Transaction;
	typedef RefPtr<CRequest> Request;
	typedef RefPtr<CBlob> Blob;
	typedef RefPtr<CStatement> Statement;
	typedef RefPtr<CService> Service;

	// force use of default memory pool for Y-Valve objects
	typedef GlobalStorage DefaultMemory;

	// stored handle types
	typedef Jrd::jrd_tra StoredTra;
	typedef void StoredReq;
	typedef void StoredBlb;
	typedef Jrd::Attachment StoredAtt;
	typedef Jrd::dsql_req StoredStm;
	typedef void StoredSvc;

	template <typename CleanupRoutine, typename CleanupArg>
	class Clean : public DefaultMemory
	{
	private:
		struct st_clean
		{
			CleanupRoutine *Routine;
			void* clean_arg;
			st_clean(CleanupRoutine *r, void* a)
				: Routine(r), clean_arg(a)
			{ }
			st_clean()
				: Routine(0), clean_arg(0)
			{ }
		};
		HalfStaticArray<st_clean, 1> calls;
		Mutex mutex;

	public:
		Clean() : calls(*getDefaultMemoryPool()) { }

		void add(CleanupRoutine *r, void* a)
		{
			MutexLockGuard guard(mutex);
			for (size_t i = 0; i < calls.getCount(); ++i)
			{
				if (calls[i].Routine == r && calls[i].clean_arg == a)
				{
					return;
				}
			}
			calls.add(st_clean(r, a));
		}

		void call(CleanupArg public_handle)
		{
			MutexLockGuard guard(mutex);
			for (size_t i = 0; i < calls.getCount(); ++i)
			{
				if (calls[i].Routine)
				{
					calls[i].Routine(public_handle, calls[i].clean_arg);
				}
			}
		}
	};

	class BaseHandle : public DefaultMemory, public RefCounted
	{
	public:
		UCHAR			type;
		UCHAR			flags;
		USHORT			implementation;
		FB_API_HANDLE	public_handle;
		Attachment		parent;
    	FB_API_HANDLE*	user_handle;

	protected:
		BaseHandle(UCHAR t, FB_API_HANDLE* pub, Attachment par, USHORT imp = ~0);

	public:
		static BaseHandle* translate(FB_API_HANDLE);

		void release_user_handle()
		{
			if (user_handle)
			{
				*user_handle = 0;
			}
		}

		// required to put pointers to it into the tree
		static const FB_API_HANDLE& generate(const void* /*sender*/, const BaseHandle* value)
		{
			return value->public_handle;
		}

		static void drop(BaseHandle*);

	protected:
		~BaseHandle();
	};

	template <typename T>
	class HandleArray
	{
	public:
		explicit HandleArray(MemoryPool& p) : arr(p) { }
		HandleArray() : arr(*getDefaultMemoryPool()) { }

		void destroy()
		{
			MutexLockGuard guard(mtx);

			size_t i;
			while ((i = arr.getCount()))
			{
				T::destroy(arr[i - 1]);
			}
		}

		void toParent(T* newMember)
		{
			MutexLockGuard guard(mtx);

			arr.add(newMember);
		}

		void fromParent(T* oldMember)
		{
			MutexLockGuard guard(mtx);

			size_t pos;
			if (arr.find(oldMember, pos))
			{
				arr.remove(pos);
			}
#ifdef DEV_BUILD
			else
			{
				//Attempt to deregister not registered member
				fb_assert(false);
			}
#endif
		}

		FB_API_HANDLE getPublicHandle(const void* handle)
		{
			if (handle)
			{
				MutexLockGuard guard(mtx);

				for (T** itr = arr.begin(); itr < arr.end(); itr++)
				{
					T* const member = *itr;

					if (member->handle == handle)
					{
						return member->public_handle;
					}
				}
			}

			return 0;
		}

	private:
		SortedArray<T*> arr;
		Mutex mtx;
	};

	class CAttachment : public BaseHandle
	{
	public:
		HandleArray<CTransaction> transactions;
		HandleArray<CRequest> requests;
		HandleArray<CBlob> blobs;
		HandleArray<CStatement> statements;

		int enterCount;
		Mutex enterMutex;

		Clean<AttachmentCleanupRoutine, FB_API_HANDLE*> cleanup;
		StoredAtt* handle;
		StatusHolder status;		// Do not use raise() method of this class in yValve
		PathName db_path;

		static ISC_STATUS hError()
		{
			return isc_bad_db_handle;
		}

		static UCHAR hType()
		{
			return 1;
		}

	public:
		CAttachment(StoredAtt*, FB_API_HANDLE*, USHORT);

		static void destroy(CAttachment*);

		bool destroying() const
		{
			return flagDestroying;
		}

	private:
		~CAttachment() { }

		bool flagDestroying;
	};

	class CTransaction : public BaseHandle
	{
	public:
		Clean<TransactionCleanupRoutine, FB_API_HANDLE> cleanup;
		Transaction next;
		StoredTra* handle;
		HandleArray<CBlob> blobs;

		static ISC_STATUS hError()
		{
			return isc_bad_trans_handle;
		}

		static UCHAR hType()
		{
			return 2;
		}

	public:
		CTransaction(StoredTra* h, FB_API_HANDLE* pub, Attachment par)
			: BaseHandle(hType(), pub, par), next(0), handle(h),
			blobs(getPool())
		{
			parent->transactions.toParent(this);
		}

		CTransaction(FB_API_HANDLE* pub, USHORT a_implementation)
			: BaseHandle(hType(), pub, Attachment(0), a_implementation), next(0), handle(0),
			blobs(getPool())
		{
		}

		static void destroy(CTransaction*);

	private:
		~CTransaction() { }
	};

	class CRequest : public BaseHandle
	{
	public:
		StoredReq* handle;

		static ISC_STATUS hError()
		{
			return isc_bad_req_handle;
		}

		static UCHAR hType()
		{
			return 3;
		}

	public:
		CRequest(StoredReq* h, FB_API_HANDLE* pub, Attachment par)
			: BaseHandle(hType(), pub, par), handle(h)
		{
			parent->requests.toParent(this);
		}

		static void destroy(CRequest* h)
		{
			h->release_user_handle();
			h->parent->requests.fromParent(h);
			drop(h);
		}

	private:
		~CRequest() { }
	};

	class CBlob : public BaseHandle
	{
	public:
		StoredBlb* handle;
		Transaction tra;

		static ISC_STATUS hError()
		{
			return isc_bad_segstr_handle;
		}

		static UCHAR hType()
		{
			return 4;
		}

	public:
		CBlob(StoredBlb* h, FB_API_HANDLE* pub, Attachment par, Transaction t)
			: BaseHandle(hType(), pub, par), handle(h), tra(t)
		{
			parent->blobs.toParent(this);
			tra->blobs.toParent(this);
		}

		static void destroy(CBlob* h)
		{
			h->tra->blobs.fromParent(h);
			h->parent->blobs.fromParent(h);
			drop(h);
		}

	private:
		~CBlob() { }
	};

	class CStatement : public BaseHandle
	{
	public:
		StoredStm* handle;
		struct sqlda_sup das;

		static ISC_STATUS hError()
		{
			return isc_bad_stmt_handle;
		}

		static UCHAR hType()
		{
			return 5;
		}

	public:
		CStatement(StoredStm* h, FB_API_HANDLE* pub, Attachment par)
			: BaseHandle(hType(), pub, par), handle(h)
		{
			parent->statements.toParent(this);
		}

		void checkPrepared() const
		{
			if (!(flags & HANDLE_STATEMENT_prepared))
			{
				status_exception::raise(Arg::Gds(isc_unprepared_stmt));
			}
		}

		static void destroy(CStatement* h)
		{
			h->release_user_handle();
			h->parent->statements.fromParent(h);
			drop(h);
		}
	};

	class CService : public BaseHandle
	{
	public:
		Clean<AttachmentCleanupRoutine, FB_API_HANDLE*> cleanup;
		StoredSvc* handle;

		static ISC_STATUS hError()
		{
			return isc_bad_svc_handle;
		}

		static UCHAR hType()
		{
			return 6;
		}

	public:
		CService(StoredSvc* h, FB_API_HANDLE* pub, USHORT impl)
			: BaseHandle(hType(), pub, Attachment(0), impl), handle(h)
		{
		}

		static void destroy(CService* h)
		{
			h->cleanup.call(&h->public_handle);
			drop(h);
		}

	private:
		~CService() { }
	};

	typedef BePlusTree<BaseHandle*, FB_API_HANDLE, MemoryPool, BaseHandle> HandleMapping;

	GlobalPtr<HandleMapping> handleMapping;
	ULONG handle_sequence_number = 0;
	GlobalPtr<RWLock> handleMappingLock;

	InitInstance<HandleArray<CAttachment> > attachments;
	GlobalPtr<Mutex> shutdownCallbackMutex;

	class ShutChain : public GlobalStorage
	{
	private:
		ShutChain(ShutChain* link, FB_SHUTDOWN_CALLBACK cb, const int m, void* a)
			: next(link), callBack(cb), mask(m), arg(a)
		{ }

		~ShutChain() { }

	private:
		static ShutChain* list;
		ShutChain* next;
		FB_SHUTDOWN_CALLBACK callBack;
		int mask;
		void* arg;

	public:
		static void add(FB_SHUTDOWN_CALLBACK cb, const int m, void* a)
		{
			MutexLockGuard guard(shutdownCallbackMutex);

			for (const ShutChain* chain = list; chain; chain = chain->next)
			{
				if (chain->callBack == cb && chain->mask == m && chain->arg == a)
				{
					return;
				}
			}

			list = new ShutChain(list, cb, m, a);
		}

		static int run(const int m, const int reason)
		{
			int rc = FB_SUCCESS;
			MutexLockGuard guard(shutdownCallbackMutex);

			for (ShutChain* chain = list; chain; chain = chain->next)
			{
				if ((chain->mask & m) && (chain->callBack(reason, m, chain->arg) != FB_SUCCESS))
				{
					rc = FB_FAILURE;
				}
			}

			return rc;
		}
	};

	ShutChain* ShutChain::list = 0;


	BaseHandle::BaseHandle(UCHAR t, FB_API_HANDLE* pub, Attachment par, USHORT imp)
		: type(t), flags(0), implementation(par ? par->implementation : imp),
		  parent(par), user_handle(0)
	{
		fb_assert(par || (imp != USHORT(~0)));

		addRef();

		{ // scope for write lock on handleMappingLock
			WriteLockGuard sync(handleMappingLock);
			// Loop until we find an empty handle slot.
			// This is to care of case when counter rolls over
			do {
				// Generate handle number using rolling counter.
				// This way we tend to give out unique handle numbers and closed
				// handles do not appear as valid to our clients.
				ULONG temp = ++handle_sequence_number;

				// Avoid generating NULL handle when sequence number wraps
				if (!temp)
					temp = ++handle_sequence_number;
				public_handle = (FB_API_HANDLE)(IPTR)temp;
			} while (!handleMapping->add(this));
		}

		if (pub)
		{
			*pub = public_handle;
		}
	}

	BaseHandle* BaseHandle::translate(FB_API_HANDLE handle)
	{
		HandleMapping::Accessor accessor(&handleMapping);
		if (accessor.locate(handle))
		{
			return accessor.current();
		}

		return 0;
	}

	template <typename ToHandle> RefPtr<ToHandle> translate(FB_API_HANDLE* handle, bool checkAttachment = true)
	{
		if (shutdownStarted)
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}

		if (handle && *handle)
		{
			ReadLockGuard sync(handleMappingLock);

			BaseHandle* rc = BaseHandle::translate(*handle);
			if (rc && rc->type == ToHandle::hType())
			{
				if (checkAttachment)
				{
					Attachment attachment = rc->parent;
					if (attachment && attachment->status.getError())
					{
						status_exception::raise(attachment->status.value());
					}
				}

				return RefPtr<ToHandle>(static_cast<ToHandle*>(rc));
			}
		}

		status_exception::raise(Arg::Gds(ToHandle::hError()));
		// compiler warning silencer
		return RefPtr<ToHandle>(0);
	}

	BaseHandle::~BaseHandle() { }

	void BaseHandle::drop(BaseHandle* h)
	{
		WriteLockGuard sync(handleMappingLock);

		// Silently ignore bad handles for PROD_BUILD
		if (handleMapping->locate(h->public_handle))
		{
			handleMapping->fastRemove();
		}
#ifdef DEV_BUILD
		else
		{
			//Attempt to release bad handle
			fb_assert(false);
		}
#endif
		h->release();
	}

	CAttachment::CAttachment(StoredAtt* h, FB_API_HANDLE* pub, USHORT impl)
		: BaseHandle(hType(), pub, Attachment(0), impl),
		  transactions(getPool()),
		  requests(getPool()),
		  blobs(getPool()),
		  statements(getPool()),
		  enterCount(0),
		  handle(h),
		  db_path(getPool()),
		  flagDestroying(false)
	{
		attachments().toParent(this);
		parent = this;
	}

	void CAttachment::destroy(CAttachment* h)
	{
		h->cleanup.call(&h->public_handle);

		// cleanup
		try
		{
			h->flagDestroying = true;

			h->requests.destroy();
			h->statements.destroy();
			h->blobs.destroy();
			// There should not be transactions at this point,
			// but it's no danger in cleaning empty array
			h->transactions.destroy();
			h->parent = NULL;

			h->flagDestroying = false;
		}
		catch(const Exception&)
		{
			h->flagDestroying = false;
			throw;
		}

		attachments().fromParent(h);
		drop(h);
	}

	void CTransaction::destroy(CTransaction* h)
	{
		h->cleanup.call(h->public_handle);
		h->blobs.destroy();

		if (h->parent)
		{
			h->parent->transactions.fromParent(h);
		}

		CTransaction* sub = h->next;

		drop(h);

		if (sub)
		{
			CTransaction::destroy(sub);
		}
	}

	template <typename T>
	void destroy(RefPtr<T> h)
	{
		if (h)
		{
			T::destroy(h);
		}
	}
} // namespace Why

#ifdef DEV_BUILD
static void check_status_vector(const ISC_STATUS*);
#endif

using namespace Why;

static void event_ast(void*, USHORT, const UCHAR*);
static void exit_handler(void*);

static Transaction find_transaction(Attachment, Transaction);

inline Transaction findTransaction(FB_API_HANDLE* public_handle, Attachment a)
{
	Transaction t = find_transaction(a, translate<CTransaction>(public_handle));
	if (! t)
	{
		bad_handle(isc_bad_trans_handle);
	}

	return t;
}

static int get_database_info(Transaction, TEXT**);
static PTR get_entrypoint(int, int);
static USHORT sqlda_buffer_size(USHORT, const XSQLDA*, USHORT);
static ISC_STATUS get_transaction_info(ISC_STATUS*, Transaction, TEXT**);

static void iterative_sql_info(ISC_STATUS*, FB_API_HANDLE*, USHORT, const SCHAR*, SSHORT,
							   SCHAR*, USHORT, XSQLDA*);
static ISC_STATUS open_blob(ISC_STATUS*, FB_API_HANDLE*, FB_API_HANDLE*, FB_API_HANDLE*, SLONG*,
						USHORT, const UCHAR*, SSHORT, SSHORT);
static ISC_STATUS prepare(ISC_STATUS *, Transaction);
static void save_error_string(ISC_STATUS*);
static bool set_path(const PathName&, PathName&);

GlobalPtr<Semaphore> why_sem;
static bool why_initialized = false;

/*
 * A client-only API call, isc_reset_fpe() is deprecated - we do not use
 * the FPE handler anymore, it can't be used in multithreaded library.
 * Parameter is ignored, it always returns FPE_RESET_ALL_API_CALL,
 * this is the most close code to what we are doing now.
 */

//static const USHORT FPE_RESET_INIT_ONLY		= 0x0;	/* Don't reset FPE after init */
//static const USHORT FPE_RESET_NEXT_API_CALL	= 0x1;	/* Reset FPE on next gds call */
static const USHORT FPE_RESET_ALL_API_CALL		= 0x2;	/* Reset FPE on all gds call */

/*
 * Global array to store string from the status vector in
 * save_error_string.
 */

const int MAXERRORSTRINGLENGTH	= 250;
static TEXT glbstr1[MAXERRORSTRINGLENGTH];
static const TEXT glbunknown[10] = "<unknown>";

const USHORT PREPARE_BUFFER_SIZE	= 32768;	// size of buffer used in isc_dsql_prepare call
const USHORT DESCRIBE_BUFFER_SIZE	= 1024;		// size of buffer used in isc_dsql_describe_xxx call

namespace
{
	// Status:	Provides correct status vector for operation and init() it.
	class Status
	{
	public:
		explicit Status(ISC_STATUS* v) throw()
			: local_vector(v ? v : local_status)
		{
			fb_utils::init_status(local_vector);
		}

		operator ISC_STATUS*()
		{
			return local_vector;
		}

		~Status()
		{
#ifdef DEV_BUILD
			check_status_vector(local_vector);
#endif
		}

	private:
		ISC_STATUS_ARRAY local_status;
		ISC_STATUS* local_vector;
	};

	const int SHUTDOWN_TIMEOUT = 5000;	// 5 sec

	void atExitShutdown()
	{
		fb_shutdown(SHUTDOWN_TIMEOUT, fb_shutrsn_exit_called);
	}

#ifdef UNIX
	int killed;
	bool procInt, procTerm;

	GlobalPtr<SignalSafeSemaphore> shutdownSemaphore;

	THREAD_ENTRY_DECLARE shutdownThread(THREAD_ENTRY_PARAM)
	{
		for (;;)
		{
			killed = 0;
			try {
				shutdownSemaphore->enter();
			}
			catch (status_exception& e)
			{
				TEXT buffer[BUFFER_LARGE];
				const ISC_STATUS* vector = e.value();
				if (! (vector && fb_interpret(buffer, sizeof(buffer), &vector)))
				{
					strcpy(buffer, "Unknown failure in shutdown thread in shutSem:enter()");
				}
				gds__log("%s", buffer);
				exit(0);
			}

			if (! killed)
			{
				break;
			}

			// perform shutdown
			if (fb_shutdown(SHUTDOWN_TIMEOUT, fb_shutrsn_signal) == FB_SUCCESS)
			{
				InstanceControl::registerShutdown(0);
				exit(0);
			}
		}

		return 0;
	}

	void handler(int sig)
	{
		if (killed)
		{
			return;
		}
		killed = sig;
		shutdownSemaphore->release();
	}

	void handlerInt(void*)
	{
		handler(SIGINT);
	}

	void handlerTerm(void*)
	{
		handler(SIGTERM);
	}

	class CtrlCHandler
	{
	public:
		explicit CtrlCHandler(MemoryPool&)
		{
			InstanceControl::registerShutdown(atExitShutdown);

			gds__thread_start(shutdownThread, 0, 0, 0, &handle);

			procInt = ISC_signal(SIGINT, handlerInt, 0);
			procTerm = ISC_signal(SIGTERM, handlerTerm, 0);
		}

		~CtrlCHandler()
		{
			ISC_signal_cancel(SIGINT, handlerInt, 0);
			ISC_signal_cancel(SIGTERM, handlerTerm, 0);

			if (! killed)
			{
				// must be done to let shutdownThread close
				shutdownSemaphore->release();
				THD_wait_for_completion(handle);
			}
		}
	private:
		ThreadHandle handle;
	};
#endif //UNIX

	// YEntry:	Tracks FP exceptions state (via FpeControl).
	//			Accounts activity per different attachments.
	class YEntry : public FpeControl
	{
	public:
		explicit YEntry(Status& _status, BaseHandle* primary)
			: att(primary->parent), status(&_status)
		{
			init();
		}

		explicit YEntry()
			: att(0), status(0)
		{
			init();
		}

		void init()
		{
#ifdef UNIX
			static GlobalPtr<CtrlCHandler> ctrlCHandler;
#elif defined WIN_NT
			static volatile bool registered = false;
			if (!registered)
			{
				registered = true;
				InstanceControl::registerShutdown(atExitShutdown);
			}
#endif //UNIX
			if (att)
			{
				MutexLockGuard guard(att->enterMutex);
				att->enterCount++;
			}
		}

		~YEntry()
		{
			if (att)
			{
				MutexLockGuard guard(att->enterMutex);
				att->enterCount--;

				if (!att->status.getError())
				{
					const ISC_STATUS err = (*status)[1];
					if (err == isc_shutdown || err == isc_att_shutdown)
					{
						// Save exact error to report it at all following calls
						// of this attachment (except of detach). This is correct
						// as server already closed its connection with client at
						// this time.
						att->status.save(*status);
					}
				}
			}
		}

	private:
		YEntry(const YEntry&);	// prohibit copy constructor
		Attachment att;
		Status*	status;
	};

} // anonymous namespace


#define CALL(proc, handle)	(get_entrypoint(proc, handle))


#define GDS_ATTACH_DATABASE		isc_attach_database
#define GDS_BLOB_INFO			isc_blob_info
#define GDS_CANCEL_BLOB			isc_cancel_blob
#define GDS_CANCEL_EVENTS		isc_cancel_events
#define FB_CANCEL_OPERATION		fb_cancel_operation
#define GDS_CLOSE_BLOB			isc_close_blob
#define GDS_COMMIT				isc_commit_transaction
#define GDS_COMMIT_RETAINING	isc_commit_retaining
#define GDS_COMPILE				isc_compile_request
#define GDS_COMPILE2			isc_compile_request2
#define GDS_CREATE_BLOB			isc_create_blob
#define GDS_CREATE_BLOB2		isc_create_blob2
#define GDS_CREATE_DATABASE		isc_create_database
#define GDS_DATABASE_INFO		isc_database_info
#define GDS_DDL					isc_ddl
#define GDS_DETACH				isc_detach_database
#define GDS_DROP_DATABASE		isc_drop_database
//#define GDS_EVENT_WAIT			gds__event_wait
#define GDS_GET_SEGMENT			isc_get_segment
#define GDS_GET_SLICE			isc_get_slice
#define GDS_OPEN_BLOB			isc_open_blob
#define GDS_OPEN_BLOB2			isc_open_blob2
#define GDS_PREPARE				isc_prepare_transaction
#define GDS_PREPARE2			isc_prepare_transaction2
#define GDS_PUT_SEGMENT			isc_put_segment
#define GDS_PUT_SLICE			isc_put_slice
#define GDS_QUE_EVENTS			isc_que_events
#define GDS_RECONNECT			isc_reconnect_transaction
#define GDS_RECEIVE				isc_receive

#ifdef SCROLLABLE_CURSORS
#define GDS_RECEIVE2			isc_receive2
#endif

#define GDS_RELEASE_REQUEST		isc_release_request
#define GDS_REQUEST_INFO		isc_request_info
#define GDS_ROLLBACK			isc_rollback_transaction
#define GDS_ROLLBACK_RETAINING	isc_rollback_retaining
#define GDS_SEEK_BLOB			isc_seek_blob
#define GDS_SEND				isc_send
#define GDS_START_AND_SEND		isc_start_and_send
#define GDS_START				isc_start_request
#define GDS_START_MULTIPLE		isc_start_multiple
#define GDS_START_TRANSACTION	isc_start_transaction
#define GDS_TRANSACT_REQUEST	isc_transact_request
#define GDS_TRANSACTION_INFO	isc_transaction_info
#define GDS_UNWIND				isc_unwind_request

#define GDS_DSQL_ALLOCATE		isc_dsql_allocate_statement
#define GDS_DSQL_ALLOC			isc_dsql_alloc_statement
#define GDS_DSQL_ALLOC2			isc_dsql_alloc_statement2
#define GDS_DSQL_EXECUTE		isc_dsql_execute
#define GDS_DSQL_EXECUTE2		isc_dsql_execute2
#define GDS_DSQL_EXECUTE_M		isc_dsql_execute_m
#define GDS_DSQL_EXECUTE2_M		isc_dsql_execute2_m
#define GDS_DSQL_EXECUTE_IMMED	isc_dsql_execute_immediate
#define GDS_DSQL_EXECUTE_IMM_M	isc_dsql_execute_immediate_m
#define GDS_DSQL_EXEC_IMMED		isc_dsql_exec_immediate
#define GDS_DSQL_EXEC_IMMED2	isc_dsql_exec_immed2
#define GDS_DSQL_EXEC_IMM_M		isc_dsql_exec_immediate_m
#define GDS_DSQL_EXEC_IMM2_M	isc_dsql_exec_immed2_m
#define GDS_DSQL_EXEC_IMM3_M    isc_dsql_exec_immed3_m
#define GDS_DSQL_FETCH			isc_dsql_fetch
#define GDS_DSQL_FETCH2			isc_dsql_fetch2
#define GDS_DSQL_FETCH_M		isc_dsql_fetch_m
#define GDS_DSQL_FETCH2_M		isc_dsql_fetch2_m
#define GDS_DSQL_FREE			isc_dsql_free_statement
#define GDS_DSQL_INSERT			isc_dsql_insert
#define GDS_DSQL_INSERT_M		isc_dsql_insert_m
#define GDS_DSQL_PREPARE		isc_dsql_prepare
#define GDS_DSQL_PREPARE_M		isc_dsql_prepare_m
#define GDS_DSQL_SET_CURSOR		isc_dsql_set_cursor_name
#define GDS_DSQL_SQL_INFO		isc_dsql_sql_info

#define GDS_SERVICE_ATTACH		isc_service_attach
#define GDS_SERVICE_DETACH		isc_service_detach
#define GDS_SERVICE_QUERY		isc_service_query
#define GDS_SERVICE_START		isc_service_start

/*****************************************************
 *  IMPORTANT IMPORTANT IMPORTANT IMPORTANT IMPORTANT
 *
 *  The order in which these defines appear MUST match
 *  the order in which the entrypoint appears in
 *  source/jrd/entry.h.  Failure to do so will result in
 *  much frustration
 ******************************************************/

const int PROC_ATTACH_DATABASE	= 0;
const int PROC_BLOB_INFO		= 1;
const int PROC_CANCEL_BLOB		= 2;
const int PROC_CLOSE_BLOB		= 3;
const int PROC_COMMIT			= 4;
const int PROC_COMPILE			= 5;
const int PROC_CREATE_BLOB		= 6;
const int PROC_CREATE_DATABASE	= 7;
const int PROC_DATABASE_INFO	= 8;
const int PROC_DETACH			= 9;
const int PROC_GET_SEGMENT		= 10;
const int PROC_OPEN_BLOB		= 11;
const int PROC_PREPARE			= 12;
const int PROC_PUT_SEGMENT		= 13;
const int PROC_RECONNECT		= 14;
const int PROC_RECEIVE			= 15;
const int PROC_RELEASE_REQUEST	= 16;
const int PROC_REQUEST_INFO		= 17;
const int PROC_ROLLBACK			= 18;
const int PROC_SEND				= 19;
const int PROC_START_AND_SEND	= 20;
const int PROC_START			= 21;
//const int PROC_START_MULTIPLE	= 22;
const int PROC_START_TRANSACTION= 23;
const int PROC_TRANSACTION_INFO	= 24;
const int PROC_UNWIND			= 25;
const int PROC_COMMIT_RETAINING	= 26;
const int PROC_QUE_EVENTS		= 27;
const int PROC_CANCEL_EVENTS	= 28;
const int PROC_DDL				= 29;
const int PROC_OPEN_BLOB2		= 30;
const int PROC_CREATE_BLOB2		= 31;
const int PROC_GET_SLICE		= 32;
const int PROC_PUT_SLICE		= 33;
const int PROC_SEEK_BLOB		= 34;
const int PROC_TRANSACT_REQUEST	= 35;
const int PROC_DROP_DATABASE	= 36;

const int PROC_DSQL_ALLOCATE	= 37;
//const int PROC_DSQL_EXECUTE		= 38;
const int PROC_DSQL_EXECUTE2	= 39;
//const int PROC_DSQL_EXEC_IMMED	= 40;
const int PROC_DSQL_EXEC_IMMED2	= 41;
const int PROC_DSQL_FETCH		= 42;
const int PROC_DSQL_FREE		= 43;
const int PROC_DSQL_INSERT		= 44;
const int PROC_DSQL_PREPARE		= 45;
const int PROC_DSQL_SET_CURSOR	= 46;
const int PROC_DSQL_SQL_INFO	= 47;

const int PROC_SERVICE_ATTACH	= 48;
const int PROC_SERVICE_DETACH	= 49;
const int PROC_SERVICE_QUERY	= 50;
const int PROC_SERVICE_START	= 51;

const int PROC_ROLLBACK_RETAINING	= 52;
const int PROC_CANCEL_OPERATION	= 53;

const int PROC_SHUTDOWN			= 54;
const int PROC_PING				= 55;

const int PROC_count			= 56;

/* Define complicated table for multi-subsystem world */

namespace
{
	const char* const subsystems[] = {"REMINT", "GDSSHR"};
	const size_t SUBSYSTEMS = FB_NELEM(subsystems);
	int enabledSubsystems = 0;
}

extern "C" {

static ISC_STATUS no_entrypoint(ISC_STATUS * user_status, ...);

#ifdef SUPERCLIENT
#define ENTRYPOINT(cur, rem)	ISC_STATUS rem(ISC_STATUS* user_status, ...);
#else
#define ENTRYPOINT(cur, rem)	ISC_STATUS rem(ISC_STATUS* user_status, ...), cur(ISC_STATUS* user_status, ...);
#endif

#include "../jrd/entry.h"

static PTR entrypoints[PROC_count * SUBSYSTEMS] =
{
#define ENTRYPOINT(cur, rem)	rem,
#include "../jrd/entry.h"

#if !defined(SUPERCLIENT)
#define ENTRYPOINT(cur, rem)	cur,
#include "../jrd/entry.h"
#endif
};

} // extern "C"

/* Information items for two phase commit */

static const UCHAR prepare_tr_info[] =
{
	isc_info_tra_id,
	isc_info_end
};

/* Information items for DSQL prepare */

static const SCHAR sql_prepare_info[] =
{
	isc_info_sql_select,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
};

/* Information items for SQL info */

static const SCHAR describe_select_info[] =
{
	isc_info_sql_select,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
};

static const SCHAR describe_bind_info[] =
{
	isc_info_sql_bind,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
};

static const SCHAR sql_prepare_info2[] =
{
	isc_info_sql_stmt_type,

	// describe_select_info
	isc_info_sql_select,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end,

	// describe_bind_info
	isc_info_sql_bind,
	isc_info_sql_describe_vars,
	isc_info_sql_sqlda_seq,
	isc_info_sql_type,
	isc_info_sql_sub_type,
	isc_info_sql_scale,
	isc_info_sql_length,
	isc_info_sql_field,
	isc_info_sql_relation,
	isc_info_sql_owner,
	isc_info_sql_alias,
	isc_info_sql_describe_end
};


ISC_STATUS API_ROUTINE GDS_ATTACH_DATABASE(ISC_STATUS* user_status,
										   SSHORT file_length,
										   const TEXT* file_name,
										   FB_API_HANDLE* public_handle,
										   SSHORT dpb_length,
										   const SCHAR* dpb)
{
/**************************************
 *
 *	g d s _ $ a t t a c h _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Attach a database through the first subsystem
 *	that recognizes it.
 *
 **************************************/
	ISC_STATUS *ptr;
	ISC_STATUS_ARRAY temp;
	StoredAtt* handle = 0;
	Attachment attachment(0);
	USHORT n = 0;

	Status status(user_status);

	try
	{
		YEntry entryGuard;

		nullCheck(public_handle, isc_bad_db_handle);

		if (shutdownStarted)
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}

		if (!file_name)
		{
			status_exception::raise(Arg::Gds(isc_bad_db_format) << Arg::Str(""));
		}

		if (dpb_length > 0 && !dpb)
		{
			status_exception::raise(Arg::Gds(isc_bad_dpb_form));
		}

#if !defined (SUPERCLIENT)
		if (disableConnections)
		{
			status_exception::raise(Arg::Gds(isc_shutwarn));
		}
#endif // !SUPERCLIENT

		ptr = status;

/* copy the file name to a temp buffer, since some of the following
   utilities can modify it */

		PathName org_filename(file_name, file_length ? file_length : strlen(file_name));
		ClumpletWriter newDpb(ClumpletReader::Tagged, MAX_DPB_SIZE,
			reinterpret_cast<const UCHAR*>(dpb), dpb_length, isc_dpb_version1);

		bool utfFilename = newDpb.find(isc_dpb_utf8_filename);

		if (utfFilename)
			ISC_utf8ToSystem(org_filename);
		else
		{
			newDpb.insertTag(isc_dpb_utf8_filename);

			for (newDpb.rewind(); !newDpb.isEof(); newDpb.moveNext())
			{
				UCHAR tag = newDpb.getClumpTag();
				switch (tag)
				{
					case isc_dpb_sys_user_name:
					case isc_dpb_user_name:
					case isc_dpb_password:
					case isc_dpb_sql_role_name:
					case isc_dpb_trusted_auth:
					case isc_dpb_trusted_role:
					case isc_dpb_working_directory:
					case isc_dpb_set_db_charset:
					case isc_dpb_process_name:
					{
						string s;
						newDpb.getString(s);
						ISC_systemToUtf8(s);
						newDpb.deleteClumplet();
						newDpb.insertString(tag, s);
						break;
					}
				}
			}
		}

		setLogin(newDpb);
		org_filename.rtrim();

		PathName expanded_filename;
		bool unescaped = false;

		if (!set_path(org_filename, expanded_filename))
		{
			expanded_filename = org_filename;
			ISC_systemToUtf8(expanded_filename);
			ISC_unescape(expanded_filename);
			unescaped = true;
			ISC_utf8ToSystem(expanded_filename);
			ISC_expand_filename(expanded_filename, true);
		}

		ISC_systemToUtf8(org_filename);
		ISC_systemToUtf8(expanded_filename);

		if (unescaped)
			ISC_escape(expanded_filename);

		if (org_filename != expanded_filename && !newDpb.find(isc_dpb_org_filename))
		{
			newDpb.insertPath(isc_dpb_org_filename, org_filename);
		}

		for (n = 0; n < SUBSYSTEMS; n++)
		{
			if (enabledSubsystems && !(enabledSubsystems & (1 << n)))
			{
				continue;
			}

			if (!CALL(PROC_ATTACH_DATABASE, n) (ptr, expanded_filename.c_str(),
												&handle, newDpb.getBufferLength(),
												reinterpret_cast<const char*>(newDpb.getBuffer())))
			{
				attachment = new CAttachment(handle, public_handle, n);
				attachment->db_path = expanded_filename;

				status[0] = isc_arg_gds;
				status[1] = 0;

				/* Check to make sure that status[2] is not a warning.  If it is, then
				 * the rest of the vector should be untouched.  If it is not, then make
				 * this element isc_arg_end
				 *
				 * This cleanup is done to remove any erroneous errors from trying multiple
				 * protocols for a database attach
				 */
				if (status[2] != isc_arg_warning) {
					status[2] = isc_arg_end;
				}

				return status[1];
			}
			if (ptr[1] != isc_unavailable)
			{
				ptr = temp;
			}
		}
	}
	catch (const Exception& e)
	{
		if (handle)
		{
			CALL(PROC_DETACH, n) (temp, &handle);
		}
		destroy(attachment);

  		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_BLOB_INFO(ISC_STATUS*	user_status,
									 FB_API_HANDLE*		blob_handle,
									 SSHORT		item_length,
									 const SCHAR*		items,
									 SSHORT		buffer_length,
									 SCHAR*		buffer)
{
/**************************************
 *
 *	g d s _ $ b l o b _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on blob object.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);
		CALL(PROC_BLOB_INFO, blob->implementation) (status, &blob->handle,
													item_length, items,
													buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_CANCEL_BLOB(ISC_STATUS * user_status,
									   FB_API_HANDLE * blob_handle)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Abort a partially completed blob.
 *
 **************************************/
	if (!*blob_handle)
	{
		if (user_status)
		{
			fb_utils::init_status(user_status);
		}
		return FB_SUCCESS;
	}

	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);

		if (! CALL(PROC_CANCEL_BLOB, blob->implementation) (status, &blob->handle))
		{
			destroy(blob);
			*blob_handle = 0;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_CANCEL_EVENTS(ISC_STATUS * user_status,
										 FB_API_HANDLE * handle,
										 SLONG* id)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Try to cancel an event.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(handle);
		YEntry entryGuard(status, attachment);
		CALL(PROC_CANCEL_EVENTS, attachment->implementation) (status, &attachment->handle, id);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE FB_CANCEL_OPERATION(ISC_STATUS * user_status,
											FB_API_HANDLE * handle,
											USHORT option)
{
/**************************************
 *
 *	g d s _ $ c a n c e l _ o p e r a t i o n
 *
 **************************************
 *
 * Functional description
 *	Try to cancel an operation.
 *
 **************************************/
	Status status(user_status);

	try
	{
		YEntry entryGuard;
		Attachment attachment = translate<CAttachment>(handle);
		// mutex will be locked here for a really long time
		MutexLockGuard guard(attachment->enterMutex);
		if (attachment->enterCount || option != fb_cancel_raise)
		{
			CALL(PROC_CANCEL_OPERATION, attachment->implementation) (status,
																	 &attachment->handle,
																	 option);
		}
		else
		{
			status_exception::raise(Arg::Gds(isc_nothing_to_cancel));
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_CLOSE_BLOB(ISC_STATUS * user_status,
									  FB_API_HANDLE * blob_handle)
{
/**************************************
 *
 *	g d s _ $ c l o s e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Close a blob opened either for reading or writing (creation).
 *
 **************************************/
	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);

		if (CALL(PROC_CLOSE_BLOB, blob->implementation) (status, &blob->handle))
		{
			return status[1];
		}

		destroy(blob);
		*blob_handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_COMMIT(ISC_STATUS * user_status,
								  FB_API_HANDLE * tra_handle)
{
/**************************************
 *
 *	g d s _ $ c o m m i t
 *
 **************************************
 *
 * Functional description
 *	Commit a transaction.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		Transaction sub;
		YEntry entryGuard(status, transaction);

		if (transaction->implementation != SUBSYSTEMS) {
			// Handle single transaction case
			if (CALL(PROC_COMMIT, transaction->implementation) (status, &transaction->handle))
			{
				return status[1];
			}
		}
		else {
			// Handle two phase commit.  Start by putting everybody into
			// limbo.  If anybody fails, punt
			if (!(transaction->flags & HANDLE_TRANSACTION_limbo))
			{
				if (prepare(status, transaction))
				{
					return status[1];
				}
			}

			// Everybody is in limbo, now commit everybody.
			// In theory, this can't fail

			for (sub = transaction->next; sub; sub = sub->next)
			{
				if (CALL(PROC_COMMIT, sub->implementation) (status, &sub->handle))
				{
					return status[1];
				}
			}
		}

		destroy(transaction);

		*tra_handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_COMMIT_RETAINING(ISC_STATUS * user_status,
											FB_API_HANDLE * tra_handle)
{
/**************************************
 *
 *	g d s _ $ c o m m i t _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Do a commit retaining.
 *
 * N.B., the transaction cleanup handlers are NOT
 * called here since, conceptually, the transaction
 * is still running.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		YEntry entryGuard(status, transaction);

		for (Transaction sub = transaction; sub; sub = sub->next)
		{
			if (sub->implementation != SUBSYSTEMS &&
				CALL(PROC_COMMIT_RETAINING, sub->implementation) (status, &sub->handle))
			{
				return status[1];
			}
		}

		transaction->flags |= HANDLE_TRANSACTION_limbo;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_COMPILE(ISC_STATUS* user_status,
								   FB_API_HANDLE* db_handle,
								   FB_API_HANDLE* req_handle,
								   USHORT blr_length,
								   const SCHAR* blr)
{
/**************************************
 *
 *	g d s _ $ c o m p i l e
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	Status status(user_status);
	Attachment attachment(NULL);
	StoredReq* rq = NULL;

	try
	{
		attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		nullCheck(req_handle, isc_bad_req_handle);

		if (CALL(PROC_COMPILE, attachment->implementation) (status, &attachment->handle, &rq, blr_length, blr))
		{
			return status[1];
		}

		new CRequest(rq, req_handle, attachment);
	}
	catch (const Exception& e)
	{
		if (attachment && rq)
		{
			*req_handle = 0;
			CALL(PROC_RELEASE_REQUEST, attachment->implementation) (status, rq);
		}
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_COMPILE2(ISC_STATUS* user_status,
									FB_API_HANDLE* db_handle,
									FB_API_HANDLE* req_handle,
									USHORT blr_length,
									const SCHAR* blr)
{
/**************************************
 *
 *	g d s _ $ c o m p i l e 2
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (GDS_COMPILE(status, db_handle, req_handle, blr_length, blr))
		{
			return status[1];
		}

		Request request = translate<CRequest>(req_handle);
		request->user_handle = req_handle;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_CREATE_BLOB(ISC_STATUS* user_status,
									   FB_API_HANDLE* db_handle,
									   FB_API_HANDLE* tra_handle,
									   FB_API_HANDLE* blob_handle,
									   SLONG* blob_id)
{
/**************************************
 *
 *	g d s _ $ c r e a t e _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob.
 *
 **************************************/

	return open_blob(user_status, db_handle, tra_handle, blob_handle, blob_id,
					 0, 0, PROC_CREATE_BLOB, PROC_CREATE_BLOB2);
}


ISC_STATUS API_ROUTINE GDS_CREATE_BLOB2(ISC_STATUS* user_status,
										FB_API_HANDLE* db_handle,
										FB_API_HANDLE* tra_handle,
										FB_API_HANDLE* blob_handle,
										SLONG* blob_id,
										SSHORT bpb_length,
										const UCHAR* bpb)
{
/**************************************
 *
 *	g d s _ $ c r e a t e _ b l o b 2
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob.
 *
 **************************************/

	return open_blob(user_status, db_handle, tra_handle, blob_handle, blob_id,
					 bpb_length, bpb, PROC_CREATE_BLOB,
					 PROC_CREATE_BLOB2);
}



ISC_STATUS API_ROUTINE GDS_CREATE_DATABASE(ISC_STATUS* user_status,
										   USHORT file_length,
										   const TEXT* file_name,
										   FB_API_HANDLE* public_handle,
										   SSHORT dpb_length,
										   const UCHAR* dpb,
										   USHORT /*db_type*/)
{
/**************************************
 *
 *	g d s _ $ c r e a t e _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Create a nice, squeaky clean database, uncorrupted by user data.
 *
 **************************************/
	ISC_STATUS *ptr;
	ISC_STATUS_ARRAY temp;
	StoredAtt* handle = 0;
	Attachment attachment(0);
	USHORT n = 0;

	Status status(user_status);

	try
	{
		YEntry entryGuard;

		nullCheck(public_handle, isc_bad_db_handle);

		if (shutdownStarted)
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}

		if (!file_name)
		{
			status_exception::raise(Arg::Gds(isc_bad_db_format) << Arg::Str(""));
		}

		if (dpb_length > 0 && !dpb)
		{
			status_exception::raise(Arg::Gds(isc_bad_dpb_form));
		}

#if !defined (SUPERCLIENT)
		if (disableConnections)
		{
			status_exception::raise(Arg::Gds(isc_shutwarn));
		}
#endif // !SUPERCLIENT

		ptr = status;

/* copy the file name to a temp buffer, since some of the following
   utilities can modify it */

		PathName org_filename(file_name, file_length ? file_length : strlen(file_name));
		ClumpletWriter newDpb(ClumpletReader::Tagged, MAX_DPB_SIZE,
				dpb, dpb_length, isc_dpb_version1);

		bool utfFilename = newDpb.find(isc_dpb_utf8_filename);

		if (utfFilename)
			ISC_utf8ToSystem(org_filename);
		else
			newDpb.insertTag(isc_dpb_utf8_filename);

		setLogin(newDpb);
		org_filename.rtrim();

		PathName expanded_filename;
		bool unescaped = false;

		if (!set_path(org_filename, expanded_filename))
		{
			expanded_filename = org_filename;
			ISC_systemToUtf8(expanded_filename);
			ISC_unescape(expanded_filename);
			unescaped = true;
			ISC_utf8ToSystem(expanded_filename);
			ISC_expand_filename(expanded_filename, true);
		}

		ISC_systemToUtf8(org_filename);
		ISC_systemToUtf8(expanded_filename);

		if (unescaped)
			ISC_escape(expanded_filename);

		if (org_filename != expanded_filename && !newDpb.find(isc_dpb_org_filename))
		{
			newDpb.insertPath(isc_dpb_org_filename, org_filename);
		}

		for (n = 0; n < SUBSYSTEMS; n++)
		{
			if (enabledSubsystems && !(enabledSubsystems & (1 << n)))
			{
				continue;
			}

			if (!CALL(PROC_CREATE_DATABASE, n) (ptr, expanded_filename.c_str(),
												&handle, newDpb.getBufferLength(),
												reinterpret_cast<const char*>(newDpb.getBuffer())))
			{
#ifdef WIN_NT
            	// Now we can expand, the file exists
				expanded_filename = org_filename;
				ISC_unescape(expanded_filename);
				ISC_utf8ToSystem(expanded_filename);
				ISC_expand_filename(expanded_filename, true);
				ISC_systemToUtf8(expanded_filename);
#endif

				attachment = new CAttachment(handle, public_handle, n);
#ifdef WIN_NT
				attachment->db_path = expanded_filename;
#else
				attachment->db_path = org_filename;
#endif

				status[0] = isc_arg_gds;
				status[1] = 0;
				if (status[2] != isc_arg_warning)
					status[2] = isc_arg_end;

				return status[1];
			}
			if (ptr[1] != isc_unavailable)
				ptr = temp;
		}
	}
	catch (const Exception& e)
	{
  		e.stuff_exception(status);
		if (handle)
		{
			CALL(PROC_DROP_DATABASE, n) (temp, &handle);
		}

		destroy(attachment);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_database_cleanup(ISC_STATUS * user_status,
											 FB_API_HANDLE * handle,
											 AttachmentCleanupRoutine * routine,
											 void* arg)
{
/**************************************
 *
 *	g d s _ $ d a t a b a s e _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Register an attachment specific cleanup handler.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(handle);
		YEntry entryGuard(status, attachment);

		attachment->cleanup.add(routine, arg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DATABASE_INFO(ISC_STATUS* user_status,
										 FB_API_HANDLE* handle,
										 SSHORT item_length,
										 const SCHAR* items,
										 SSHORT buffer_length,
										 SCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ d a t a b a s e _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on database object.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(handle);
		YEntry entryGuard(status, attachment);
		CALL(PROC_DATABASE_INFO, attachment->implementation) (status, &attachment->handle,
															item_length, items,
															buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DDL(ISC_STATUS* user_status,
							   FB_API_HANDLE* db_handle,
							   FB_API_HANDLE* tra_handle,
							   SSHORT length,
							   const UCHAR* ddl)
{
/**************************************
 *
 *	g d s _ $ d d l
 *
 **************************************
 *
 * Functional description
 *	Do meta-data update.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		Transaction transaction = findTransaction(tra_handle, attachment);

		CALL(PROC_DDL, attachment->implementation) (status, &attachment->handle, &transaction->handle,
												  length, ddl);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DETACH(ISC_STATUS * user_status,
								  FB_API_HANDLE * handle)
{
/**************************************
 *
 *	g d s _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down an attachment.
 *
 **************************************/
	return detach_or_drop_database(user_status, handle, PROC_DETACH);
}


static ISC_STATUS detach_or_drop_database(ISC_STATUS * user_status, FB_API_HANDLE * handle,
										  const int proc, const ISC_STATUS specCode)
{
/**************************************
 *
 *	d e t a c h _ o r _ d r o p _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Common code for that calls.
 *
 **************************************/
	Status status(user_status);

	const bool dropping = (proc == PROC_DROP_DATABASE);

	try
	{
		YEntry entryGuard;

		Attachment attachment = translate<CAttachment>(handle, dropping);

		if (attachment->handle)
		{
			if (CALL(proc, attachment->implementation) (status, &attachment->handle) &&
		    	status[1] != specCode)
			{
				return status[1];
			}
		}

		destroy(attachment);
		attachment = NULL;
		*handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}

int API_ROUTINE gds__disable_subsystem(TEXT* subsystem)
{
/**************************************
 *
 *	g d s _ $ d i s a b l e _ s u b s y s t e m
 *
 **************************************
 *
 * Functional description
 *	Disable access to a specific subsystem.  If no subsystem
 *	has been explicitly disabled, all are available.
 *
 **************************************/
	for (size_t i = 0; i < SUBSYSTEMS; i++)
	{
		if (!strcmp(subsystems[i], subsystem))
		{
			if (!enabledSubsystems)
				enabledSubsystems = ~enabledSubsystems;
			enabledSubsystems &= ~(1 << i);
			return TRUE;
		}
	}

	return FALSE;
}


ISC_STATUS API_ROUTINE GDS_DROP_DATABASE(ISC_STATUS * user_status,
										 FB_API_HANDLE * handle)
{
/**************************************
 *
 *	i s c _ d r o p _ d a t a b a s e
 *
 **************************************
 *
 * Functional description
 *	Close down a database and then purge it.
 *
 **************************************/
	return detach_or_drop_database(user_status, handle, PROC_DROP_DATABASE, isc_drdb_completed_with_errs);
}


ISC_STATUS API_ROUTINE GDS_DSQL_ALLOC(ISC_STATUS * user_status,
									  FB_API_HANDLE * db_handle,
									  FB_API_HANDLE * stmt_handle)
{
/**************************************
 *
 *	i s c _ d s q l _ a l l o c _ s t a t e m e n t
 *
 **************************************/

	return GDS_DSQL_ALLOCATE(user_status, db_handle, stmt_handle);
}


ISC_STATUS API_ROUTINE GDS_DSQL_ALLOC2(ISC_STATUS * user_status,
									   FB_API_HANDLE * db_handle,
									   FB_API_HANDLE * stmt_handle)
{
/**************************************
 *
 *	i s c _ d s q l _ a l l o c _ s t a t e m e n t 2
 *
 **************************************
 *
 * Functional description
 *	Allocate a statement handle.
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (GDS_DSQL_ALLOCATE(status, db_handle, stmt_handle))
		{
			return status[1];
		}

		Statement statement = translate<CStatement>(stmt_handle);
		statement->user_handle = stmt_handle;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_ALLOCATE(ISC_STATUS * user_status,
										 FB_API_HANDLE * db_handle,
										 FB_API_HANDLE * public_stmt_handle)
{
/**************************************
 *
 *	i s c _ d s q l _ a l l o c a t e _ s t a t e m e n t
 *
 **************************************
 *
 * Functional description
 *	Allocate a statement handle.
 *
 **************************************/
	Status status(user_status);
	Attachment attachment(NULL);
	StoredStm* stmt_handle = NULL;

	try
	{
		attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		// check the statement handle to make sure it's NULL and then initialize it.
		nullCheck(public_stmt_handle, isc_bad_stmt_handle);

		if (CALL(PROC_DSQL_ALLOCATE, attachment->implementation) (status, &attachment->handle, &stmt_handle))
		{
			return status[1];
		}

		//Statement statement =
		new CStatement(stmt_handle, public_stmt_handle, attachment);
	}
	catch (const Exception& e)
	{
		if (attachment && stmt_handle)
		{
			*public_stmt_handle = 0;
			CALL(PROC_DSQL_FREE, attachment->implementation) (status, &stmt_handle, DSQL_drop);
		}
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_dsql_describe(ISC_STATUS * user_status,
										 FB_API_HANDLE * stmt_handle,
										 USHORT dialect,
										 XSQLDA * sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ d e s c r i b e
 *
 **************************************
 *
 * Functional description
 *	Describe output parameters for a prepared statement.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);

		statement->checkPrepared();
		sqlda_sup::dasup_clause& clause = statement->das.dasup_clauses[DASUP_CLAUSE_select];

		if (clause.dasup_info_len && clause.dasup_info_buf)
		{
			iterative_sql_info(	status,
								stmt_handle,
								sizeof(describe_select_info),
								describe_select_info,
								clause.dasup_info_len,
								clause.dasup_info_buf,
								dialect,
								sqlda);
		}
		else
		{
			HalfStaticArray<SCHAR, DESCRIBE_BUFFER_SIZE> local_buffer;
			const USHORT buffer_len = sqlda_buffer_size(DESCRIBE_BUFFER_SIZE, sqlda, dialect);
			SCHAR *buffer = local_buffer.getBuffer(buffer_len);

			if (!GDS_DSQL_SQL_INFO(	status,
									stmt_handle,
									sizeof(describe_select_info),
									describe_select_info,
									buffer_len,
									buffer))
			{
				iterative_sql_info(	status,
									stmt_handle,
									sizeof(describe_select_info),
									describe_select_info,
									buffer_len,
									buffer,
									dialect,
									sqlda);
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE isc_dsql_describe_bind(ISC_STATUS * user_status,
											  FB_API_HANDLE * stmt_handle,
											  USHORT dialect,
											  XSQLDA * sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ d e s c r i b e _ b i n d
 *
 **************************************
 *
 * Functional description
 *	Describe input parameters for a prepared statement.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);

		sqlda_sup::dasup_clause& clause = statement->das.dasup_clauses[DASUP_CLAUSE_bind];

		if (clause.dasup_info_len && clause.dasup_info_buf)
		{
			iterative_sql_info(	status,
								stmt_handle,
								sizeof(describe_bind_info),
								describe_bind_info,
								clause.dasup_info_len,
								clause.dasup_info_buf,
								dialect,
								sqlda);
		}
		else
		{
			HalfStaticArray<SCHAR, DESCRIBE_BUFFER_SIZE> local_buffer;
			const USHORT buffer_len = sqlda_buffer_size(DESCRIBE_BUFFER_SIZE, sqlda, dialect);
			SCHAR *buffer = local_buffer.getBuffer(buffer_len);

			if (!GDS_DSQL_SQL_INFO(	status,
									stmt_handle,
									sizeof(describe_bind_info),
									describe_bind_info,
									buffer_len,
									buffer))
			{
				iterative_sql_info(	status,
									stmt_handle,
									sizeof(describe_bind_info),
									describe_bind_info,
									buffer_len,
									buffer,
									dialect,
									sqlda);
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE(ISC_STATUS* user_status,
										FB_API_HANDLE* tra_handle,
										FB_API_HANDLE* stmt_handle,
										USHORT dialect,
										const XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e
 *
 **************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 **************************************/

	return GDS_DSQL_EXECUTE2(user_status, tra_handle, stmt_handle, dialect,
							 sqlda, NULL);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE2(ISC_STATUS* user_status,
										 FB_API_HANDLE* tra_handle,
										 FB_API_HANDLE* stmt_handle,
										 USHORT dialect,
										 const XSQLDA* in_sqlda,
										 const XSQLDA* out_sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e 2
 *
 **************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 **************************************/
	Status status(user_status);

	try
	{
		USHORT in_blr_length, in_msg_type, in_msg_length,
			out_blr_length, out_msg_type, out_msg_length;

		Statement statement = translate<CStatement>(stmt_handle);

		sqlda_sup* dasup = &(statement->das);
		statement->checkPrepared();

		if (UTLD_parse_sqlda(status, dasup, &in_blr_length, &in_msg_type, &in_msg_length,
							 dialect, in_sqlda, DASUP_CLAUSE_bind))
		{
			return status[1];
		}

		if (UTLD_parse_sqlda(status, dasup, &out_blr_length, &out_msg_type, &out_msg_length,
							 dialect, out_sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}

		if (GDS_DSQL_EXECUTE2_M(status, tra_handle, stmt_handle,
								in_blr_length,
								dasup->dasup_clauses[DASUP_CLAUSE_bind].dasup_blr,
								in_msg_type, in_msg_length,
								dasup->dasup_clauses[DASUP_CLAUSE_bind].dasup_msg,
								out_blr_length,
								dasup->dasup_clauses[DASUP_CLAUSE_select].
								dasup_blr, out_msg_type, out_msg_length,
								dasup->dasup_clauses[DASUP_CLAUSE_select].
								dasup_msg))
		{
			return status[1];
		}

		if (UTLD_parse_sqlda(status, dasup, NULL, NULL, NULL, dialect, out_sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE_M(ISC_STATUS* user_status,
										  FB_API_HANDLE* tra_handle,
										  FB_API_HANDLE* stmt_handle,
										  USHORT blr_length,
										  const SCHAR* blr,
										  USHORT msg_type,
										  USHORT msg_length,
										  SCHAR* msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e _ m
 *
 **************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 **************************************/

	return GDS_DSQL_EXECUTE2_M(user_status, tra_handle, stmt_handle, blr_length, blr,
							   msg_type, msg_length, msg, 0, NULL, 0, 0, NULL);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE2_M(ISC_STATUS* user_status,
										   FB_API_HANDLE* tra_handle,
										   FB_API_HANDLE* stmt_handle,
										   USHORT in_blr_length,
										   const SCHAR* in_blr,
										   USHORT in_msg_type,
										   USHORT in_msg_length,
										   SCHAR* in_msg,
										   USHORT out_blr_length,
										   SCHAR* out_blr,
										   USHORT out_msg_type,
										   USHORT out_msg_length,
										   SCHAR* out_msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e 2 _ m
 *
 **************************************
 *
 * Functional description
 *	Execute a non-SELECT dynamic SQL statement.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		Transaction transaction(NULL);
		StoredTra* handle = NULL;

		if (tra_handle && *tra_handle)
		{
			transaction = translate<CTransaction>(tra_handle);
			Transaction t = find_transaction(statement->parent, transaction);
			if (!t)
			{
				bad_handle(isc_bad_trans_handle);
			}
			handle = t->handle;
		}

		CALL(PROC_DSQL_EXECUTE2, statement->implementation) (status,
														     &handle,
														     &statement->handle,
														     in_blr_length, in_blr,
														     in_msg_type, in_msg_length, in_msg,
														     out_blr_length, out_blr,
														     out_msg_type, out_msg_length, out_msg);

		if (!status[1])
		{
			if (transaction && !handle)
			{
				destroy(transaction);
				*tra_handle = 0;
			}
			else if (!transaction && handle)
			{
				transaction = new CTransaction(handle, tra_handle, statement->parent);
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


// Is this really API function? Where is it declared?
ISC_STATUS API_ROUTINE GDS_DSQL_EXEC_IMMED(ISC_STATUS* user_status,
										   FB_API_HANDLE* db_handle,
										   FB_API_HANDLE* tra_handle,
										   USHORT length,
										   const SCHAR* string,
										   USHORT dialect,
										   const XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c _ i m m e d
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return GDS_DSQL_EXECUTE_IMMED(user_status, db_handle, tra_handle, length, string,
								  dialect, sqlda);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE_IMMED(ISC_STATUS* user_status,
											  FB_API_HANDLE* db_handle,
											  FB_API_HANDLE* tra_handle,
											  USHORT length,
											  const SCHAR* string,
											  USHORT dialect,
											  const XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e _ i m m e d i a t e
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/

	return GDS_DSQL_EXEC_IMMED2(user_status, db_handle, tra_handle, length, string,
								dialect, sqlda, NULL);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXEC_IMMED2(ISC_STATUS* user_status,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* tra_handle,
											USHORT length,
											const SCHAR* string,
											USHORT dialect,
											const XSQLDA* in_sqlda,
											const XSQLDA* out_sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c _ i m m e d 2
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/
	Status status(user_status);
	ISC_STATUS s = 0;
	sqlda_sup dasup;

	try
	{
		if (!string)
		{
			Arg::Gds(isc_command_end_err).raise();
		}

		USHORT in_blr_length, in_msg_type, in_msg_length,
			out_blr_length, out_msg_type, out_msg_length;

		if (UTLD_parse_sqlda(status, &dasup, &in_blr_length, &in_msg_type, &in_msg_length,
							 dialect, in_sqlda, DASUP_CLAUSE_bind))
		{
			return status[1];
		}

		if (UTLD_parse_sqlda(status, &dasup, &out_blr_length, &out_msg_type, &out_msg_length,
							 dialect, out_sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}

		s = GDS_DSQL_EXEC_IMM2_M(status, db_handle, tra_handle,
								 length, string, dialect,
								 in_blr_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_bind].dasup_blr,
								 in_msg_type, in_msg_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_bind].dasup_msg,
								 out_blr_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_blr,
								 out_msg_type, out_msg_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_msg);
		if (!s)
		{
			s =	UTLD_parse_sqlda(status, &dasup, NULL, NULL, NULL, dialect,
								 out_sqlda, DASUP_CLAUSE_select);
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		s = status[1];
	}

	return s;
}


// Is this really API function? Where is it declared?
ISC_STATUS API_ROUTINE GDS_DSQL_EXEC_IMM_M(ISC_STATUS* user_status,
										   FB_API_HANDLE* db_handle,
										   FB_API_HANDLE* tra_handle,
										   USHORT length,
										   const SCHAR* string,
										   USHORT dialect,
										   USHORT blr_length,
										   USHORT msg_type,
										   USHORT msg_length,
										   SCHAR* blr,
										   SCHAR* msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c _ i m m e d _ m
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return GDS_DSQL_EXECUTE_IMM_M(user_status, db_handle, tra_handle,
								  length, string, dialect, blr_length, blr,
								  msg_type, msg_length, msg);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXECUTE_IMM_M(ISC_STATUS* user_status,
											  FB_API_HANDLE* db_handle,
											  FB_API_HANDLE* tra_handle,
											  USHORT length,
											  const SCHAR* string,
											  USHORT dialect,
											  USHORT blr_length,
											  SCHAR* blr,
											  USHORT msg_type,
											  USHORT msg_length,
											  SCHAR* msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c u t e _ i m m e d i a t e _ m
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/

	return GDS_DSQL_EXEC_IMM2_M(user_status, db_handle, tra_handle,
								length, string, dialect, blr_length, blr,
								msg_type, msg_length, msg, 0, NULL, 0, 0, NULL);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXEC_IMM2_M(ISC_STATUS* user_status,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* tra_handle,
											USHORT length,
											const SCHAR* string,
											USHORT dialect,
											USHORT in_blr_length,
											SCHAR* in_blr,
											USHORT in_msg_type,
											USHORT in_msg_length,
											const SCHAR* in_msg,
											USHORT out_blr_length,
											SCHAR* out_blr,
											USHORT out_msg_type,
											USHORT out_msg_length,
											SCHAR* out_msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c _ i m m 2 _ m
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/
	Status status(user_status);

	bool stmt_eaten;
	if (PREPARSE_execute(status, db_handle, tra_handle, length, string, &stmt_eaten, dialect))
	{
		if (status[1])
		{
			return status[1];
		}

		ISC_STATUS_ARRAY temp_status;
		FB_API_HANDLE crdb_trans_handle = 0;
		if (GDS_START_TRANSACTION(status, &crdb_trans_handle, 1, db_handle, 0, 0))
		{
			save_error_string(status);
			GDS_DROP_DATABASE(temp_status, db_handle);
			*db_handle = 0;
			return status[1];
		}

		bool ret_v3_error = false;
		if (!stmt_eaten) {
			/* Check if against < 4.0 database */

			const SCHAR ch = isc_info_base_level;
			SCHAR buffer[16];
			if (!GDS_DATABASE_INFO(status, db_handle, 1, &ch, sizeof(buffer), buffer))
			{
				if ((buffer[0] != isc_info_base_level) || (buffer[4] > 3))
				{
					GDS_DSQL_EXEC_IMM3_M(status, db_handle, &crdb_trans_handle, length, string,
										 dialect, in_blr_length, in_blr,
										 in_msg_type, in_msg_length, in_msg,
										 out_blr_length, out_blr,
										 out_msg_type, out_msg_length, out_msg);
				}
				else
					ret_v3_error = true;
			}
		}

		if (status[1]) {
			GDS_ROLLBACK(temp_status, &crdb_trans_handle);
			save_error_string(status);
			GDS_DROP_DATABASE(temp_status, db_handle);
			*db_handle = 0;
			return status[1];
		}

		if (GDS_COMMIT(status, &crdb_trans_handle)) {
			GDS_ROLLBACK(temp_status, &crdb_trans_handle);
			save_error_string(status);
			GDS_DROP_DATABASE(temp_status, db_handle);
			*db_handle = 0;
			return status[1];
		}

		if (ret_v3_error) {
			Firebird::Arg::Gds(isc_srvr_version_too_old).copyTo(status);
			return status[1];
		}

		return status[1];
	}

	return GDS_DSQL_EXEC_IMM3_M(user_status, db_handle, tra_handle, length, string, dialect,
								in_blr_length, in_blr, in_msg_type, in_msg_length, in_msg,
								out_blr_length, out_blr, out_msg_type, out_msg_length, out_msg);
}


ISC_STATUS API_ROUTINE GDS_DSQL_EXEC_IMM3_M(ISC_STATUS* user_status,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* tra_handle,
											USHORT length,
											const SCHAR* string,
											USHORT dialect,
											USHORT in_blr_length,
											SCHAR* in_blr,
											USHORT in_msg_type,
											USHORT in_msg_length,
											const SCHAR* in_msg,
											USHORT out_blr_length,
											SCHAR* out_blr,
											USHORT out_msg_type,
											USHORT out_msg_length,
											SCHAR* out_msg)
{
/**************************************
 *
 *	i s c _ d s q l _ e x e c _ i m m 3 _ m
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (!string)
		{
			Arg::Gds(isc_command_end_err).raise();
		}

		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);

		Transaction transaction(NULL);
		StoredTra* handle = NULL;

		if (tra_handle && *tra_handle)
		{
			transaction = translate<CTransaction>(tra_handle);
			Transaction t = find_transaction(attachment, transaction);
			if (!t)
			{
				bad_handle(isc_bad_trans_handle);
			}
			handle = t->handle;
		}

		CALL(PROC_DSQL_EXEC_IMMED2, attachment->implementation) (status,
														  &attachment->handle, &handle,
														  length, string, dialect,
														  in_blr_length, in_blr,
														  in_msg_type, in_msg_length, in_msg,
														  out_blr_length, out_blr,
														  out_msg_type, out_msg_length, out_msg);

		if (!status[1])
		{
			if (transaction && !handle)
			{
				destroy(transaction);
				*tra_handle = 0;
			}
			else if (!transaction && handle)
			{
				transaction = new CTransaction(handle, tra_handle, attachment);
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_FETCH(ISC_STATUS* user_status,
									  FB_API_HANDLE* stmt_handle,
									  USHORT dialect,
									  const XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ f e t c h
 *
 **************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (!sqlda)
		{
			status_exception::raise(Arg::Gds(isc_dsql_sqlda_err));
		}

		Statement statement = translate<CStatement>(stmt_handle);

		statement->checkPrepared();
		sqlda_sup& dasup = statement->das;

		USHORT blr_length, msg_type, msg_length;
		if (UTLD_parse_sqlda(status, &dasup, &blr_length, &msg_type, &msg_length,
							 dialect, sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}

		ISC_STATUS s = GDS_DSQL_FETCH_M(status, stmt_handle, blr_length,
								dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_blr,
								0, msg_length,
								dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_msg);
		if (s && s != 101)
		{
			return s;
		}

		if (UTLD_parse_sqlda(status, &dasup, NULL, NULL, NULL, dialect, sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


#ifdef SCROLLABLE_CURSORS
ISC_STATUS API_ROUTINE GDS_DSQL_FETCH2(ISC_STATUS* user_status,
									   FB_API_HANDLE* stmt_handle,
									   USHORT dialect,
									   XSQLDA* sqlda,
									   USHORT direction,
									   SLONG offset)
{
/**************************************
 *
 *	i s c _ d s q l _ f e t c h 2
 *
 **************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (!sqlda)
		{
			status_exception::raise(Arg::Gds(isc_dsql_sqlda_err));
		}

		Statement statement = translate<CStatement>(stmt_handle);

		statement->checkPrepared();
		sqlda_sup& dasup = statement->das;

		USHORT blr_length, msg_type, msg_length;

		if (UTLD_parse_sqlda(status, &dasup, &blr_length, &msg_type, &msg_length,
							 dialect, sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}

		ISC_STATUS s = GDS_DSQL_FETCH2_M(status, stmt_handle, blr_length,
										 dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_blr,
										 0, msg_length,
										 dasup.dasup_clauses[DASUP_CLAUSE_select].dasup_msg,
										 direction, offset);
		if (s && s != 101)
		{
			return s;
		}

		if (UTLD_parse_sqlda(status, &dasup, NULL, NULL, NULL, dialect, sqlda, DASUP_CLAUSE_select))
		{
			return status[1];
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}
#endif


ISC_STATUS API_ROUTINE GDS_DSQL_FETCH_M(ISC_STATUS* user_status,
										FB_API_HANDLE* stmt_handle,
										USHORT blr_length,
										SCHAR* blr,
										USHORT msg_type,
										USHORT msg_length,
										SCHAR* msg)
{
/**************************************
 *
 *	i s c _ d s q l _ f e t c h _ m
 *
 **************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		ISC_STATUS s =
			CALL(PROC_DSQL_FETCH, statement->implementation) (status, &statement->handle,
															  blr_length, blr,
															  msg_type, msg_length, msg
#ifdef SCROLLABLE_CURSORS
															  ,
															  (USHORT) 0, (ULONG) 1
#endif // SCROLLABLE_CURSORS
															  );

		if (s == 100 || s == 101)
		{
			return s;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


#ifdef SCROLLABLE_CURSORS
ISC_STATUS API_ROUTINE GDS_DSQL_FETCH2_M(ISC_STATUS* user_status,
										 FB_API_HANDLE* stmt_handle,
										 USHORT blr_length,
										 SCHAR* blr,
										 USHORT msg_type,
										 USHORT msg_length,
										 SCHAR* msg,
										 USHORT direction,
										 SLONG offset)
{
/**************************************
 *
 *	i s c _ d s q l _ f e t c h 2 _ m
 *
 **************************************
 *
 * Functional description
 *	Fetch next record from a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(statement);

		ISC_STATUS s =
			CALL(PROC_DSQL_FETCH, statement->implementation) (status, &statement->handle,
															  blr_length, blr,
															  msg_type, msg_length, msg,
															  direction, offset);

		if (s == 100 || s == 101)
		{
			return s;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}
#endif


ISC_STATUS API_ROUTINE GDS_DSQL_FREE(ISC_STATUS* user_status,
									 FB_API_HANDLE* stmt_handle,
									 USHORT option)
{
/*****************************************
 *
 *	i s c _ d s q l _ f r e e _ s t a t e m e n t
 *
 *****************************************
 *
 * Functional Description
 *	release request for an sql statement
 *
 *****************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		if (CALL(PROC_DSQL_FREE, statement->implementation) (status, &statement->handle, option))
		{
			return status[1];
		}

		if (option & DSQL_drop)
		{
			destroy(statement);
			*stmt_handle = 0;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_INSERT(ISC_STATUS* user_status,
									   FB_API_HANDLE* stmt_handle,
									   USHORT dialect,
									   XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ i n s e r t
 *
 **************************************
 *
 * Functional description
 *	Insert next record into a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);

		statement->checkPrepared();
		sqlda_sup& dasup = statement->das;
		USHORT blr_length, msg_type, msg_length;
		if (UTLD_parse_sqlda(status, &dasup, &blr_length, &msg_type, &msg_length,
							 dialect, sqlda, DASUP_CLAUSE_bind))
		{
			return status[1];
		}

		return GDS_DSQL_INSERT_M(status, stmt_handle, blr_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_bind].
								 dasup_blr, 0, msg_length,
								 dasup.dasup_clauses[DASUP_CLAUSE_bind].
								 dasup_msg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_INSERT_M(ISC_STATUS* user_status,
										 FB_API_HANDLE* stmt_handle,
										 USHORT blr_length,
										 const SCHAR* blr,
										 USHORT msg_type,
										 USHORT msg_length,
										 const SCHAR* msg)
{
/**************************************
 *
 *	i s c _ d s q l _ i n s e r t _ m
 *
 **************************************
 *
 * Functional description
 *	Insert next record into a dynamic SQL cursor
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		statement->checkPrepared();

		CALL(PROC_DSQL_INSERT, statement->implementation) (status, &statement->handle,
														   blr_length, blr,
														   msg_type, msg_length, msg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_PREPARE(ISC_STATUS* user_status,
										FB_API_HANDLE* tra_handle,
										FB_API_HANDLE* stmt_handle,
										USHORT length,
										const SCHAR* string,
										USHORT dialect,
										XSQLDA* sqlda)
{
/**************************************
 *
 *	i s c _ d s q l _ p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		sqlda_sup& dasup = statement->das;

		const USHORT buffer_len = sqlda_buffer_size(PREPARE_BUFFER_SIZE, sqlda, dialect);
		//Attachment attachment = statement->parent;
		Array<SCHAR> db_prepare_buffer;
		SCHAR* const buffer = db_prepare_buffer.getBuffer(buffer_len);

		if (!GDS_DSQL_PREPARE_M(status,
								tra_handle,
								stmt_handle,
								length,
								string,
								dialect,
								sizeof(sql_prepare_info2),
								sql_prepare_info2,
								buffer_len,
								buffer))
		{
			statement->flags &= ~HANDLE_STATEMENT_prepared;
			dasup.reset();

			dasup.dasup_dialect = dialect;

			SCHAR* p = buffer;

			dasup.dasup_stmt_type = 0;
			if (*p == isc_info_sql_stmt_type)
			{
				const USHORT len = gds__vax_integer((UCHAR*)p + 1, 2);
				dasup.dasup_stmt_type = gds__vax_integer((UCHAR*)p + 3, len);
				p += 3 + len;
			}

			sqlda_sup::dasup_clause &das_select = dasup.dasup_clauses[DASUP_CLAUSE_select];
			sqlda_sup::dasup_clause &das_bind = dasup.dasup_clauses[DASUP_CLAUSE_bind];
			das_select.dasup_info_buf = das_bind.dasup_info_buf = 0;
			das_select.dasup_info_len = das_bind.dasup_info_len = 0;

			SCHAR* buf_select = 0; // pointer in the buffer where isc_info_sql_select starts
			USHORT len_select = 0; // length of isc_info_sql_select part
			if (*p == isc_info_sql_select)
			{
				das_select.dasup_info_buf = p;
				buf_select = p;
				len_select = buffer_len - (buf_select - buffer);
			}

			das_bind.dasup_info_buf = UTLD_skip_sql_info(p);

			p = das_select.dasup_info_buf;
			if (p)
			{
				SCHAR* p2 = das_bind.dasup_info_buf;
				if (p2)
				{
					const SLONG len =  p2 - p;

					p2 = alloc(len + 1);
					memmove(p2, p, len);
					p2[len] = isc_info_end;
					das_select.dasup_info_buf = p2;
					das_select.dasup_info_len = len + 1;

					buf_select = das_select.dasup_info_buf;
					len_select = das_select.dasup_info_len;
				}
				else
				{
					das_select.dasup_info_buf = 0;
					das_select.dasup_info_len = 0;
				}
			}

			p = das_bind.dasup_info_buf;
			if (p)
			{
				SCHAR* p2 = UTLD_skip_sql_info(p);
				if (p2)
				{
					const SLONG len =  p2 - p;

					p2 = alloc(len + 1);
					memmove(p2, p, len);
					p2[len] = isc_info_end;
					das_bind.dasup_info_buf = p2;
					das_bind.dasup_info_len = len + 1;
				}
				else
				{
					das_bind.dasup_info_buf = 0;
					das_bind.dasup_info_len = 0;
				}
			}

			iterative_sql_info(status, stmt_handle, sizeof(sql_prepare_info),
				sql_prepare_info, len_select, buf_select, dialect, sqlda);

			// statement prepared OK
			statement->flags |= HANDLE_STATEMENT_prepared;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_PREPARE_M(ISC_STATUS* user_status,
										  FB_API_HANDLE* tra_handle,
										  FB_API_HANDLE* stmt_handle,
										  USHORT length,
										  const SCHAR* string,
										  USHORT dialect,
										  USHORT item_length,
										  const SCHAR* items,
										  USHORT buffer_length,
										  SCHAR* buffer)
{
/**************************************
 *
 *	i s c _ d s q l _ p r e p a r e _ m
 *
 **************************************
 *
 * Functional description
 *	Prepare a statement for execution.
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (!string)
		{
			Arg::Gds(isc_command_end_err).raise();
		}

		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		StoredTra* handle = NULL;

		if (tra_handle && *tra_handle)
		{
			Transaction transaction = translate<CTransaction>(tra_handle);
			transaction = find_transaction(statement->parent, transaction);
			if (!transaction)
			{
				bad_handle (isc_bad_trans_handle);
			}
			handle = transaction->handle;
		}

		CALL(PROC_DSQL_PREPARE, statement->implementation) (status, &handle, &statement->handle,
															length, string, dialect,
															item_length, items,
															buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_SET_CURSOR(ISC_STATUS* user_status,
										   FB_API_HANDLE* stmt_handle,
										   const SCHAR* cursor,
										   USHORT type)
{
/**************************************
 *
 *	i s c _ d s q l _ s e t _ c u r s o r
 *
 **************************************
 *
 * Functional description
 *	Set a cursor name for a dynamic request.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		CALL(PROC_DSQL_SET_CURSOR, statement->implementation) (status, &statement->handle,
															   cursor, type);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_DSQL_SQL_INFO(ISC_STATUS* user_status,
										 FB_API_HANDLE* stmt_handle,
										 SSHORT item_length,
										 const SCHAR* items,
										 SSHORT buffer_length,
										 SCHAR* buffer)
{
/**************************************
 *
 *	i s c _ d s q l _ s q l _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on sql statement.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Statement statement = translate<CStatement>(stmt_handle);
		YEntry entryGuard(status, statement);

		if (( (item_length == 1) && (items[0] == isc_info_sql_stmt_type) ||
				(item_length == 2) && (items[0] == isc_info_sql_stmt_type) &&
				(items[1] == isc_info_end || items[1] == 0) ) &&
			(statement->flags & HANDLE_STATEMENT_prepared) && statement->das.dasup_stmt_type)
		{
			if (USHORT(buffer_length) >= 8)
			{
				*buffer++ = isc_info_sql_stmt_type;
				put_vax_short((UCHAR*) buffer, 4);
				buffer += 2;
				put_vax_long((UCHAR*) buffer, statement->das.dasup_stmt_type);
				buffer += 4;
				*buffer = isc_info_end;
			}
			else
			{
				*buffer = isc_info_truncated;
			}
		}
		else
		{
			CALL(PROC_DSQL_SQL_INFO, statement->implementation) (status,
																 &statement->handle,
																 item_length, items,
																 buffer_length, buffer);
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


int API_ROUTINE gds__enable_subsystem(TEXT* subsystem)
{
/**************************************
 *
 *	g d s _ $ e n a b l e _ s u b s y s t e m
 *
 **************************************
 *
 * Functional description
 *	Enable access to a specific subsystem.  If no subsystem
 *	has been explicitly enabled, all are available.
 *
 **************************************/
	for (size_t i = 0; i < SUBSYSTEMS; i++)
	{
		if (!strcmp(subsystems[i], subsystem))
		{
			if (!~enabledSubsystems)
				enabledSubsystems = 0;
			enabledSubsystems |= (1 << i);
			return TRUE;
		}
	}

	return FALSE;
}


ISC_STATUS API_ROUTINE isc_wait_for_event(ISC_STATUS* user_status,
									  FB_API_HANDLE* handle,
									  USHORT length,
									  const UCHAR* events,
									  UCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ e v e n t _ w a i t
 *
 **************************************
 *
 * Functional description
 *	Que request for event notification.
 *
 **************************************/
	Status status(user_status);

	try
	{
		if (!why_initialized)
		{
			gds__register_cleanup(exit_handler, 0);
			why_initialized = true;
		}

		SLONG id;
		if (GDS_QUE_EVENTS(status, handle, &id, length, events, event_ast, buffer))
		{
			return status[1];
		}

		why_sem->enter();
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_GET_SEGMENT(ISC_STATUS* user_status,
									   FB_API_HANDLE* blob_handle,
									   USHORT* length,
									   USHORT buffer_length,
									   UCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ g e t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Get a segment from a blob opened for reading.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);

		ISC_STATUS code =
			CALL(PROC_GET_SEGMENT, blob->implementation) (status, &blob->handle,
														  length,
														  buffer_length, buffer);

		if (code == isc_segstr_eof || code == isc_segment)
		{
			return code;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_GET_SLICE(ISC_STATUS* user_status,
									 FB_API_HANDLE* db_handle,
									 FB_API_HANDLE* tra_handle,
									 ISC_QUAD* array_id,
									 USHORT sdl_length,
									 const UCHAR* sdl,
									 USHORT param_length,
									 const UCHAR* param,
									 SLONG slice_length,
									 UCHAR* slice,
									 SLONG* return_length)
{
/**************************************
 *
 *	g d s _ $ g e t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Snatch a slice of an array.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		Transaction transaction = findTransaction(tra_handle, attachment);

		CALL(PROC_GET_SLICE, attachment->implementation) (status, &attachment->handle,
			&transaction->handle, array_id, sdl_length, sdl, param_length, param,
			slice_length, slice, return_length);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE fb_disconnect_transaction(ISC_STATUS* user_status,
												 FB_API_HANDLE* tra_handle)
{
/**************************************
 *
 *	g d s _ $ h a n d l e _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Clean up a dangling transaction handle.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);

		if (!(transaction->flags & HANDLE_TRANSACTION_limbo))
		{
			status_exception::raise(Arg::Gds(isc_no_recon));
		}

		destroy(transaction);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_OPEN_BLOB(ISC_STATUS* user_status,
									 FB_API_HANDLE* db_handle,
									 FB_API_HANDLE* tra_handle,
									 FB_API_HANDLE* blob_handle,
									 SLONG* blob_id)
{
/**************************************
 *
 *	g d s _ $ o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob.
 *
 **************************************/

	return open_blob(user_status, db_handle, tra_handle, blob_handle, blob_id,
					 0, 0, PROC_OPEN_BLOB, PROC_OPEN_BLOB2);
}


ISC_STATUS API_ROUTINE GDS_OPEN_BLOB2(ISC_STATUS* user_status,
									  FB_API_HANDLE* db_handle,
									  FB_API_HANDLE* tra_handle,
									  FB_API_HANDLE* blob_handle,
									  SLONG* blob_id,
									  SSHORT bpb_length,
									  const UCHAR* bpb)
{
/**************************************
 *
 *	g d s _ $ o p e n _ b l o b 2
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob (extended edition).
 *
 **************************************/

	return open_blob(user_status, db_handle, tra_handle, blob_handle, blob_id,
					 bpb_length, bpb, PROC_OPEN_BLOB, PROC_OPEN_BLOB2);
}


ISC_STATUS API_ROUTINE GDS_PREPARE(ISC_STATUS* user_status,
								   FB_API_HANDLE* tra_handle)
{
/**************************************
 *
 *	g d s _ $ p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	Prepare a transaction for commit.  First phase of a two
 *	phase commit.
 *
 **************************************/
	return GDS_PREPARE2(user_status, tra_handle, 0, 0);
}


ISC_STATUS API_ROUTINE GDS_PREPARE2(ISC_STATUS* user_status,
									FB_API_HANDLE* tra_handle,
									USHORT msg_length,
									const UCHAR* msg)
{
/**************************************
 *
 *	g d s _ $ p r e p a r e 2
 *
 **************************************
 *
 * Functional description
 *	Prepare a transaction for commit.  First phase of a two
 *	phase commit.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		YEntry entryGuard(status, transaction);

		for (Transaction sub = transaction; sub; sub = sub->next)
		{
			if (sub->implementation != SUBSYSTEMS &&
				CALL(PROC_PREPARE, sub->implementation) (status, &sub->handle, msg_length, msg))
			{
				return status[1];
			}
		}

		transaction->flags |= HANDLE_TRANSACTION_limbo;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_PUT_SEGMENT(ISC_STATUS* user_status,
									   FB_API_HANDLE* blob_handle,
									   USHORT buffer_length,
									   const UCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ p u t _ s e g m e n t
 *
 **************************************
 *
 * Functional description
 *	Put a segment in a blob opened for writing (creation).
 *
 **************************************/
	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);

		CALL(PROC_PUT_SEGMENT, blob->implementation) (status, &blob->handle, buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_PUT_SLICE(ISC_STATUS* user_status,
									 FB_API_HANDLE* db_handle,
									 FB_API_HANDLE* tra_handle,
									 ISC_QUAD* array_id,
									 USHORT sdl_length,
									 const UCHAR* sdl,
									 USHORT param_length,
									 const SLONG* param,
									 SLONG slice_length,
									 UCHAR* slice)
{
/**************************************
 *
 *	g d s _ $ p u t _ s l i c e
 *
 **************************************
 *
 * Functional description
 *	Put a slice in an array.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		Transaction transaction = findTransaction(tra_handle, attachment);

		CALL(PROC_PUT_SLICE, attachment->implementation) (status, &attachment->handle,
			&transaction->handle, array_id, sdl_length, sdl, param_length, param,
			 slice_length, slice);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_QUE_EVENTS(ISC_STATUS* user_status,
									  FB_API_HANDLE* handle,
									  SLONG* id,
									  USHORT length,
									  const UCHAR* events,
									  FPTR_EVENT_CALLBACK ast,
									  void* arg)
{
/**************************************
 *
 *	g d s _ $ q u e _ e v e n t s
 *
 **************************************
 *
 * Functional description
 *	Que request for event notification.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(handle);
		YEntry entryGuard(status, attachment);

		CALL(PROC_QUE_EVENTS, attachment->implementation) (status, &attachment->handle,
			id, length, events, ast, arg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_RECEIVE(ISC_STATUS* user_status,
								   FB_API_HANDLE* req_handle,
								   USHORT msg_type,
								   USHORT msg_length,
								   SCHAR* msg,
								   SSHORT level)
{
/**************************************
 *
 *	g d s _ $ r e c e i v e
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/

#ifdef SCROLLABLE_CURSORS
	return GDS_RECEIVE2(user_status, req_handle, msg_type, msg_length,
						msg, level, (USHORT) blr_continue,	/* means continue in same direction as before */
						(ULONG) 1);
#else
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);

		CALL(PROC_RECEIVE, request->implementation) (status, &request->handle,
													 msg_type, msg_length, msg,
													 level);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
#endif
}


#ifdef SCROLLABLE_CURSORS
ISC_STATUS API_ROUTINE GDS_RECEIVE2(ISC_STATUS* user_status,
									FB_API_HANDLE* req_handle,
									USHORT msg_type,
									USHORT msg_length,
									SCHAR* msg,
									SSHORT level,
									USHORT direction,
									ULONG offset)
{
/**************************************
 *
 *	i s c _ r e c e i v e 2
 *
 **************************************
 *
 * Functional description
 *	Scroll through the request output stream,
 *	then get a record from the host program.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(request);

		CALL(PROC_RECEIVE, request->implementation) (status, &request->handle,
													 msg_type, msg_length, msg,
													 level, direction, offset);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}
#endif


ISC_STATUS API_ROUTINE GDS_RECONNECT(ISC_STATUS* user_status,
									 FB_API_HANDLE* db_handle,
									 FB_API_HANDLE* tra_handle,
									 SSHORT length,
									 const UCHAR* id)
{
/**************************************
 *
 *	g d s _ $ r e c o n n e c t
 *
 **************************************
 *
 * Functional description
 *	Connect to a transaction in limbo.
 *
 **************************************/
	Status status(user_status);
	StoredTra* handle = 0;

	try
	{
		nullCheck(tra_handle, isc_bad_trans_handle);
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);

		if (CALL(PROC_RECONNECT, attachment->implementation) (status, &attachment->handle,
				&handle, length, id))
		{
			return status[1];
		}

		Transaction transaction(new CTransaction(handle, tra_handle, attachment));
		transaction->flags |= HANDLE_TRANSACTION_limbo;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		if (handle)
		{
			*tra_handle = 0;
		}
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_RELEASE_REQUEST(ISC_STATUS* user_status,
										   FB_API_HANDLE* req_handle)
{
/**************************************
 *
 *	g d s _ $ r e l e a s e _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Release a request.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);

		if (!CALL(PROC_RELEASE_REQUEST, request->implementation) (status, &request->handle))
		{
			destroy(request);
			*req_handle = 0;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_REQUEST_INFO(ISC_STATUS* user_status,
										FB_API_HANDLE* req_handle,
										SSHORT level,
										SSHORT item_length,
										const SCHAR* items,
										SSHORT buffer_length,
										SCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ r e q u e s t _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on blob object.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);

		CALL(PROC_REQUEST_INFO, request->implementation) (status, &request->handle,
														  level,
														  item_length, items,
														  buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}

#if defined (SOLARIS) || defined (WIN_NT)
extern "C"
#endif

SLONG API_ROUTINE isc_reset_fpe(USHORT /*fpe_status*/)
{
/**************************************
 *
 *	i s c _ r e s e t _ f p e
 *
 **************************************
 *
 * Functional description
 *	API to be used to tell Firebird to reset it's
 *	FPE handler - eg: client has an FPE of it's own
 *	and just changed it.
 *
 * Returns
 *	Prior setting of the FPE reset flag
 *
 **************************************/
	return FPE_RESET_ALL_API_CALL;
}


ISC_STATUS API_ROUTINE GDS_ROLLBACK_RETAINING(ISC_STATUS* user_status,
											  FB_API_HANDLE* tra_handle)
{
/**************************************
 *
 *	i s c _ r o l l b a c k _ r e t a i n i n g
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction, but keep all cursors open.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		YEntry entryGuard(status, transaction);

		for (Transaction sub = transaction; sub; sub = sub->next)
		{
			if (sub->implementation != SUBSYSTEMS &&
				CALL(PROC_ROLLBACK_RETAINING, sub->implementation) (status, &sub->handle))
			{
				return status[1];
			}
		}

		transaction->flags |= HANDLE_TRANSACTION_limbo;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_ROLLBACK(ISC_STATUS* user_status,
									FB_API_HANDLE* tra_handle)
{
/**************************************
 *
 *	g d s _ $ r o l l b a c k
 *
 **************************************
 *
 * Functional description
 *	Abort a transaction.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		YEntry entryGuard(status, transaction);

		for (Transaction sub = transaction; sub; sub = sub->next)
			if (sub->implementation != SUBSYSTEMS &&
				CALL(PROC_ROLLBACK, sub->implementation) (status, &sub->handle))
			{
				if (!is_network_error(status) || (transaction->flags & HANDLE_TRANSACTION_limbo) )
				{
					return status[1];
				}
			}

		if (is_network_error(status))
		{
			fb_utils::init_status(status);
		}

		destroy(transaction);

		*tra_handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SEEK_BLOB(ISC_STATUS* user_status,
									 FB_API_HANDLE* blob_handle,
									 SSHORT mode,
									 SLONG offset,
									 SLONG* result)
{
/**************************************
 *
 *	g d s _ $ s e e k _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Seek a blob.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Blob blob = translate<CBlob>(blob_handle);
		YEntry entryGuard(status, blob);

		CALL(PROC_SEEK_BLOB, blob->implementation) (status, &blob->handle, mode, offset, result);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SEND(ISC_STATUS* user_status,
								FB_API_HANDLE* req_handle,
								USHORT msg_type,
								USHORT msg_length,
								const SCHAR* msg,
								SSHORT level)
{
/**************************************
 *
 *	g d s _ $ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);

		CALL(PROC_SEND, request->implementation) (status, &request->handle,
												  msg_type, msg_length, msg,
												  level);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SERVICE_ATTACH(ISC_STATUS* user_status,
										  USHORT service_length,
										  const TEXT* service_name,
										  FB_API_HANDLE* public_handle,
										  USHORT spb_length,
										  const SCHAR* spb)
{
/**************************************
 *
 *	i s c _ s e r v i c e _ a t t a c h
 *
 **************************************
 *
 * Functional description
 *	Attach a service through the first subsystem
 *	that recognizes it.
 *
 **************************************/
	StoredSvc* handle = 0;
	Service service(0);
	ISC_STATUS_ARRAY temp;
	USHORT n = 0;
	Status status(user_status);

	try
	{
		YEntry entryGuard;

		nullCheck(public_handle, isc_bad_svc_handle);

		if (shutdownStarted)
		{
			status_exception::raise(Arg::Gds(isc_att_shutdown));
		}

		if (!service_name)
		{
			status_exception::raise(Arg::Gds(isc_service_att_err) << Arg::Gds(isc_svc_name_missing));
		}

		if (spb_length > 0 && !spb)
		{
			status_exception::raise(Arg::Gds(isc_bad_spb_form));
		}

#if !defined (SUPERCLIENT)
		if (disableConnections)
		{
			status_exception::raise(Arg::Gds(isc_shutwarn));
		}
#endif // !SUPERCLIENT

		string svcname(service_name, service_length ? service_length : strlen(service_name));
		svcname.rtrim();

		ISC_STATUS* ptr = status;
		for (n = 0; n < SUBSYSTEMS; n++)
		{
			if (enabledSubsystems && !(enabledSubsystems & (1 << n)))
			{
				continue;
			}

			if (!CALL(PROC_SERVICE_ATTACH, n) (ptr, svcname.c_str(), &handle, spb_length, spb))
			{
				service = new CService(handle, public_handle, n);
				status[0] = isc_arg_gds;
				status[1] = 0;
				if (status[2] != isc_arg_warning)
				{
					status[2] = isc_arg_end;
				}
				return status[1];
			}
			if (ptr[1] != isc_unavailable)
			{
				ptr = temp;
			}
		}

		if (status[1] == isc_unavailable)
		{
			status[1] = isc_service_att_err;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		if (handle)
		{
			CALL(PROC_SERVICE_DETACH, n) (temp, &handle);
			*public_handle = 0;
			destroy(service);
		}
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SERVICE_DETACH(ISC_STATUS* user_status, FB_API_HANDLE* handle)
{
/**************************************
 *
 *	i s c _ s e r v i c e _ d e t a c h
 *
 **************************************
 *
 * Functional description
 *	Close down a service.
 *
 **************************************/
	Status status(user_status);

	try
	{
		YEntry entryGuard;
		Service service = translate<CService>(handle);

		if (CALL(PROC_SERVICE_DETACH, service->implementation) (status, &service->handle))
		{
			return status[1];
		}

		destroy(service);
		*handle = 0;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SERVICE_QUERY(ISC_STATUS* user_status,
										 FB_API_HANDLE* handle,
										 ULONG* /*reserved*/,
										 USHORT send_item_length,
										 const SCHAR* send_items,
										 USHORT recv_item_length,
										 const SCHAR* recv_items,
										 USHORT buffer_length,
										 SCHAR* buffer)
{
/**************************************
 *
 *	i s c _ s e r v i c e _ q u e r y
 *
 **************************************
 *
 * Functional description
 *	Provide information on service object.
 *
 *	NOTE: The parameter RESERVED must not be used
 *	for any purpose as there are networking issues
 *	involved (as with any handle that goes over the
 *	network).  This parameter will be implemented at
 *	a later date.
 **************************************/
	Status status(user_status);

	try
	{
		YEntry entryGuard;
		Service service = translate<CService>(handle);

		CALL(PROC_SERVICE_QUERY, service->implementation) (status,
														   &service->handle,
														   0,	/* reserved */
														   send_item_length, send_items,
														   recv_item_length, recv_items,
														   buffer_length, buffer);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_SERVICE_START(ISC_STATUS* user_status,
										 FB_API_HANDLE* handle,
										 ULONG* /*reserved*/,
										 USHORT spb_length,
										 const SCHAR* spb)
{
/**************************************
 *
 *	i s c _ s e r v i c e _ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Starts a service thread
 *
 *	NOTE: The parameter RESERVED must not be used
 *	for any purpose as there are networking issues
 *	involved (as with any handle that goes over the
 *	network).  This parameter will be implemented at
 *	a later date.
 **************************************/
	Status status(user_status);

	try
	{
		YEntry entryGuard;
		Service service = translate<CService>(handle);

		CALL(PROC_SERVICE_START, service->implementation) (status, &service->handle,
														   NULL,
														   spb_length, spb);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_START_AND_SEND(ISC_STATUS* user_status,
										  FB_API_HANDLE* req_handle,
										  FB_API_HANDLE* tra_handle,
										  USHORT msg_type,
										  USHORT msg_length,
										  const SCHAR* msg,
										  SSHORT level)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ a n d _ s e n d
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);
		Transaction transaction = findTransaction(tra_handle, request->parent);

		CALL(PROC_START_AND_SEND, request->implementation) (status,
															&request->handle, &transaction->handle,
															msg_type, msg_length, msg,
															level);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_START(ISC_STATUS* user_status,
								 FB_API_HANDLE* req_handle,
								 FB_API_HANDLE* tra_handle,
								 SSHORT level)
{
/**************************************
 *
 *	g d s _ $ s t a r t
 *
 **************************************
 *
 * Functional description
 *	Get a record from the host program.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);
		Transaction transaction = findTransaction(tra_handle, request->parent);

		CALL(PROC_START, request->implementation) (status, &request->handle, &transaction->handle,
												   level);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_START_MULTIPLE(ISC_STATUS* user_status,
										  FB_API_HANDLE* tra_handle,
										  SSHORT count,
//										  TEB* vector)
										  void* vec)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ m u l t i p l e
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	TEB* vector = (TEB*) vec;
	ISC_STATUS_ARRAY temp;
	Transaction transaction(NULL);
	Attachment attachment(NULL);
	StoredTra* handle = NULL;

	Status status(user_status);

	try
	{
		YEntry entryGuard;
		nullCheck(tra_handle, isc_bad_trans_handle);

		if (count <= 0 || !vector)
		{
			status_exception::raise(Arg::Gds(isc_bad_teb_form));
		}

		Transaction* ptr;
		USHORT n;
		for (n = 0, ptr = &transaction; n < count; n++, ptr = &(*ptr)->next, vector++)
		{
			if (vector->teb_tpb_length < 0 || (vector->teb_tpb_length > 0 && !vector->teb_tpb))
			{
				status_exception::raise(Arg::Gds(isc_bad_tpb_form));
			}

			attachment = translate<CAttachment>(vector->teb_database);
			YEntry attGuard(status, attachment);

			if (CALL(PROC_START_TRANSACTION, attachment->implementation) (status, &handle, 1,
					&attachment->handle, vector->teb_tpb_length, vector->teb_tpb))
			{
				status_exception::raise(status);
			}

			*ptr = new CTransaction(handle, 0, attachment);
			handle = 0;
		}

		if (transaction->next)
		{
			Transaction sub(new CTransaction(tra_handle, SUBSYSTEMS));
			sub->next = transaction;
		}
		else {
			*tra_handle = transaction->public_handle;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);

		if (handle || transaction)
		{
			*tra_handle = 0;
		}

		while (transaction)
		{
			Transaction sub = transaction;
			transaction = sub->next;
			if (sub->handle)
			{
				CALL(PROC_ROLLBACK, sub->implementation) (temp, &sub->handle);
			}
		}

		if (transaction)
		{
			destroy(transaction);
		}

		if (handle && attachment)
		{
			CALL(PROC_ROLLBACK, attachment->implementation) (temp, &handle);
		}
	}

	return status[1];
}


ISC_STATUS API_ROUTINE_VARARG GDS_START_TRANSACTION(ISC_STATUS* user_status,
													FB_API_HANDLE* tra_handle,
													SSHORT count, ...)
{
/**************************************
 *
 *	g d s _ $ s t a r t _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Start a transaction.
 *
 **************************************/
	Status status(user_status);

	try
	{
		HalfStaticArray<TEB, 16> tebs;
		TEB* teb = tebs.getBuffer(count);

		const TEB* const end = teb + count;
		va_list ptr;
		va_start(ptr, count);

		for (TEB* teb_iter = teb; teb_iter < end; teb_iter++) {
			teb_iter->teb_database = va_arg(ptr, FB_API_HANDLE*);
			teb_iter->teb_tpb_length = va_arg(ptr, int);
			teb_iter->teb_tpb = va_arg(ptr, UCHAR *);
		}
		va_end(ptr);

		GDS_START_MULTIPLE(status, tra_handle, count, teb);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_TRANSACT_REQUEST(ISC_STATUS* user_status,
											FB_API_HANDLE* db_handle,
											FB_API_HANDLE* tra_handle,
											USHORT blr_length,
											SCHAR* blr,
											USHORT in_msg_length,
											SCHAR* in_msg,
											USHORT out_msg_length,
											SCHAR* out_msg)
{
/**************************************
 *
 *	i s c _ t r a n s a c t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *	Execute a procedure.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		Transaction transaction = findTransaction(tra_handle, attachment);

		CALL(PROC_TRANSACT_REQUEST, attachment->implementation) (status, &attachment->handle,
			&transaction->handle, blr_length, blr, in_msg_length, in_msg, out_msg_length,
			out_msg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE gds__transaction_cleanup(ISC_STATUS* user_status,
												FB_API_HANDLE* tra_handle,
												TransactionCleanupRoutine *routine,
												void* arg)
{
/**************************************
 *
 *	g d s _ $ t r a n s a c t i o n _ c l e a n u p
 *
 **************************************
 *
 * Functional description
 *	Register a transaction specific cleanup handler.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		transaction->cleanup.add(routine, arg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_TRANSACTION_INFO(ISC_STATUS* user_status,
											FB_API_HANDLE* tra_handle,
											SSHORT item_length,
											const SCHAR* items,
											SSHORT buffer_length,
											UCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ t r a n s a c t i o n _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Provide information on transaction object.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Transaction transaction = translate<CTransaction>(tra_handle);
		YEntry entryGuard(status, transaction);

		if (transaction->implementation != SUBSYSTEMS) {
			CALL(PROC_TRANSACTION_INFO, transaction->implementation) (status, &transaction->handle,
																	  item_length, items,
																	  buffer_length, buffer);
		}
		else
		{
			SSHORT item_len = item_length;
			SSHORT buffer_len = buffer_length;
			for (Transaction sub = transaction->next; sub; sub = sub->next)
			{
				if (CALL(PROC_TRANSACTION_INFO, sub->implementation) (status, &sub->handle,
																	  item_len, items,
																	  buffer_len, buffer))
				{
					return status[1];
				}

				UCHAR* ptr = buffer;
				const UCHAR* const end = buffer + buffer_len;
				while (ptr < end && *ptr == isc_info_tra_id)
				{
					ptr += 3 + gds__vax_integer(ptr + 1, 2);
				}

				if (ptr >= end || *ptr != isc_info_end)
				{
					return status[1];
				}

				buffer_len = end - ptr;
				buffer = ptr;
			}
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE GDS_UNWIND(ISC_STATUS* user_status,
								  FB_API_HANDLE* req_handle,
								  SSHORT level)
{
/**************************************
 *
 *	g d s _ $ u n w i n d
 *
 **************************************
 *
 * Functional description
 *	Unwind a running request.  This is potentially nasty since it can be called
 *	asynchronously.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Request request = translate<CRequest>(req_handle);
		YEntry entryGuard(status, request);

		CALL(PROC_UNWIND, request->implementation) (status, &request->handle, level);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}

#ifdef DEBUG_GDS_ALLOC
static SCHAR *alloc_debug(SLONG length, const char* file, int line)
#else
static SCHAR *alloc(SLONG length)
#endif
{
/**************************************
 *
 *	a l l o c
 *
 **************************************
 *
 * Functional description
 *	Allocate some memory.
 *
 **************************************/
	SCHAR *block;

#ifdef DEBUG_GDS_ALLOC
	if (block = static_cast<SCHAR*>(gds__alloc_debug((SLONG) (sizeof(SCHAR) * length), file, line)))
#else
	if (block = static_cast<SCHAR*>(gds__alloc((SLONG) (sizeof(SCHAR) * length))))
#endif
		memset(block, 0, length);
	else
		BadAlloc::raise();
	return block;
}


#ifdef DEV_BUILD
static void check_status_vector(const ISC_STATUS* status)
 {
/**************************************
 *
 *	c h e c k _ s t a t u s _ v e c t o r
 *
 **************************************
 *
 * Functional description
 *	Validate that a status vector looks valid.
 *
 **************************************/

#define SV_MSG(x)	{ gds__log ("%s %d check_status_vector: %s", __FILE__, __LINE__, (x)); BREAKPOINT (__LINE__); }

	const ISC_STATUS* s = status;
	if (!s) {
		SV_MSG("Invalid status vector");
		return;
	}

	if (*s != isc_arg_gds) {
		SV_MSG("Must start with isc_arg_gds");
		return;
	}

/* Vector [2] could either end the vector, or start a warning
   in either case the status vector is a success */
	if ((s[1] == FB_SUCCESS) && (s[2] != isc_arg_end) && (s[2] != isc_arg_gds) &&
		(s[2] != isc_arg_warning))
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
			/* The next element must either be 0 (indicating no error) or a
			 * valid isc error message, let's check */
			if (*s && (*s & ISC_MASK) != ISC_MASK) {
				if (code == isc_arg_warning) {
					SV_MSG("warning code not a valid ISC message");
				}
				else {
					SV_MSG("error code not a valid ISC message");
				}
			}

			/* If the error code is valid, then I better be able to retrieve a
			 * proper facility code from it ... let's find out */
			if (*s && (*s & ISC_MASK) == ISC_MASK) {
				bool found = false;

				const struct _facilities* facs = facilities;
				const int fac_code = GET_FACILITY(*s);
				while (facs->facility) {
					if (facs->fac_code == fac_code) {
						found = true;
						break;
					}
					facs++;
				}
				if (!found)
					if (code == isc_arg_warning) {
						SV_MSG("warning code does not contain a valid facility");
					}
					else {
						SV_MSG("error code does not contain a valid facility");
					}
			}
			s++;
			break;

		case isc_arg_interpreted:
		case isc_arg_string:
		case isc_arg_sql_state:
			length = strlen((const char*) *s);
			/* This check is heuristic, not deterministic */
			if (length > 1024 - 1)
				SV_MSG("suspect length value");
			if (*((const UCHAR *) * s) == 0xCB)
				SV_MSG("string in freed memory");
			s++;
			break;

		case isc_arg_cstring:
			length = (ULONG) * s;
			s++;
			/* This check is heuristic, not deterministic */
			/* Note: This can never happen anyway, as the length is coming
			   from a byte value */
			if (length > 1024 - 1)
				SV_MSG("suspect length value");
			if (*((const UCHAR *) * s) == 0xCB)
				SV_MSG("string in freed memory");
			s++;
			break;

		case isc_arg_number:
		case isc_arg_vms:
		case isc_arg_unix:
		case isc_arg_win32:
			s++;
			break;

		default:
			SV_MSG("invalid status code");
			return;
		}
		if ((s - status) >= ISC_STATUS_LENGTH)
			SV_MSG("vector too long");
	}

#undef SV_MSG

}
#endif


static void event_ast(void* buffer_void, USHORT length, const UCHAR* items)
{
/**************************************
 *
 *	e v e n t _ a s t
 *
 **************************************
 *
 * Functional description
 *	We're had an event complete.
 *
 **************************************/
	memcpy(buffer_void, items, length);
	why_sem->release();
}


static void exit_handler(void*)
{
/**************************************
 *
 *	e x i t _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *	Cleanup shared image.
 *
 **************************************/

	why_initialized = false;
}


static Transaction find_transaction(Attachment attachment, Transaction transaction)
{
/**************************************
 *
 *	f i n d _ t r a n s a c t i o n
 *
 **************************************
 *
 * Functional description
 *	Find the element of a possible multiple database transaction
 *	that corresponds to the current attachment.
 *
 **************************************/

	for (; transaction; transaction = transaction->next)
	{
		if (transaction->parent == attachment)
			return transaction;
	}

	return Transaction(NULL);
}


static void free_block(void* block)
{
/**************************************
 *
 *	f r e e _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Release some memory.
 *
 **************************************/

	gds__free(block);
}


static int get_database_info(Transaction transaction, TEXT** ptr)
{
/**************************************
 *
 *	g e t _ d a t a b a s e _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Get the full database pathname
 *	and put it in the transaction
 *	description record.
 *
 **************************************/

	// Look at the changed code: we don't support here more than 254 bytes in the path
	// so it's better to truncate or we'll have corrupt data in the trans desc record:
	// the length in one byte would wrap and we would copy more bytes that expected.
	// Our caller (prepare) assumed each call consumes at most 256 bytes (item, len, data)
	// hence if we don't check here, we have a B.O.
	TEXT* p = *ptr;
	Attachment attachment = transaction->parent;
	*p++ = TDR_DATABASE_PATH;
	const TEXT* q = attachment->db_path.c_str();
	size_t len = strlen(q);
	if (len > 254)
		len = 254;

	*p++ = (TEXT) len;
	memcpy(p, q, len);
	*ptr = p + len;

	return FB_SUCCESS;
}


static PTR get_entrypoint(int proc, int implementation)
{
/**************************************
 *
 *	g e t _ e n t r y p o i n t
 *
 **************************************
 *
 * Functional description
 *	Lookup entrypoint for procedure.
 *
 **************************************/

	const PTR* const entry = entrypoints + implementation * PROC_count + proc;

	// static qualifier on &noentrypoint disallows use of conditional operator on Solaris
	// return *entry ? *entry : &no_entrypoint;
	if (*entry)
		return *entry;

	return &no_entrypoint;

}


static USHORT sqlda_buffer_size(USHORT min_buffer_size, const XSQLDA* sqlda, USHORT dialect)
{
/**************************************
 *
 *	s q l d a _ b u f f e r _ s i z e
 *
 **************************************
 *
 * Functional description
 *	Calculate size of a buffer that is large enough
 *	to store the info items relating to an SQLDA.
 *
 **************************************/
	USHORT n_variables;

/* If dialect / 10 == 0, then it has not been combined with the
   parser version for a prepare statement.  If it has been combined, then
   the dialect needs to be pulled out to compare to DIALECT_xsqlda
*/

	USHORT sql_dialect = dialect / 10;
	if (sql_dialect == 0)
		sql_dialect = dialect;

	if (!sqlda)
		n_variables = 0;
	else if (sql_dialect >= DIALECT_xsqlda)
		n_variables = sqlda->sqln;
	else
		n_variables = ((SQLDA *) sqlda)->sqln;

	ULONG length = 32 + ULONG(n_variables) * 172;
	if (length < min_buffer_size)
		length = min_buffer_size;

	return (USHORT)((length > 65500L) ? 65500L : length);
}


static ISC_STATUS get_transaction_info(ISC_STATUS* user_status,
									   Transaction transaction,
									   TEXT** ptr)
{
/**************************************
 *
 *	g e t _ t r a n s a c t i o n _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Put a transaction's id into the transaction
 *	description record.
 *
 **************************************/
	Status status(user_status);

	try
	{
		TEXT buffer[16];
		TEXT* p = *ptr;

		if (CALL(PROC_TRANSACTION_INFO, transaction->implementation) (status,
																	  &transaction->handle,
																	  sizeof(prepare_tr_info),
																	  prepare_tr_info,
																	  sizeof(buffer),
																	  buffer))
		{
			return status[1];
		}

		const TEXT* q = buffer + 3;
		*p++ = TDR_TRANSACTION_ID;

		USHORT length = (USHORT)gds__vax_integer(reinterpret_cast<UCHAR*>(buffer + 1), 2);

		// Prevent information out of sync.
		if (length > MAX_UCHAR)
			length = MAX_UCHAR;

		*p++ = length;
		memcpy(p, q, length);
		*ptr = p + length;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


static void iterative_sql_info(ISC_STATUS* user_status,
							   FB_API_HANDLE* stmt_handle,
							   USHORT item_length,
							   const SCHAR* items,
							   SSHORT buffer_length,
							   SCHAR* buffer,
							   USHORT dialect,
							   XSQLDA* sqlda)
{
/**************************************
 *
 *	i t e r a t i v e _ s q l _ i n f o
 *
 **************************************
 *
 * Functional description
 *	Turn an sql info buffer into an SQLDA.  If the info
 *	buffer was incomplete, make another request, beginning
 *	where the previous info call left off.
 *
 **************************************/
	USHORT last_index;
	SCHAR new_items[32];

	while (UTLD_parse_sql_info(user_status, dialect, buffer, sqlda, &last_index) && last_index)
	{
		char* p = new_items;
		*p++ = isc_info_sql_sqlda_start;
		*p++ = 2;
		*p++ = last_index;
		*p++ = last_index >> 8;
		fb_assert(p + item_length <= new_items + sizeof(new_items));
		memcpy(p, items, item_length);
		p += item_length;
		if (GDS_DSQL_SQL_INFO(user_status, stmt_handle, (SSHORT) (p - new_items), new_items,
								buffer_length, buffer))
		{
			break;
		}
	}
}


static ISC_STATUS open_blob(ISC_STATUS* user_status,
							FB_API_HANDLE* db_handle,
							FB_API_HANDLE* tra_handle,
							FB_API_HANDLE* public_blob_handle,
							SLONG* blob_id,
							USHORT bpb_length,
							const UCHAR* bpb,
							SSHORT proc,
							SSHORT proc2)
{
/**************************************
 *
 *	o p e n _ b l o b
 *
 **************************************
 *
 * Functional description
 *	Open an existing blob (extended edition).
 *
 **************************************/
	Status status(user_status);
	StoredBlb* blob_handle = 0;

	try
	{
		nullCheck(public_blob_handle, isc_bad_segstr_handle);

		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);
		Transaction transaction = findTransaction(tra_handle, attachment);

		USHORT flags = 0;
		USHORT from, to;
		gds__parse_bpb(bpb_length, bpb, &from, &to);

		if (get_entrypoint(proc2, attachment->implementation) != no_entrypoint &&
			CALL(proc2, attachment->implementation) (status, &attachment->handle,
				&transaction->handle, &blob_handle, blob_id, bpb_length,
				bpb) != isc_unavailable)
		{
			flags = 0;
		}
		else if (!to || from == to)
		{
			// This code has no effect because jrd8_create_blob, jrd8_open_blob,
			// REM_create_blob and REM_open_blob are defined as no_entrypoint in entry.h
			CALL(proc, attachment->implementation) (status, &attachment->handle,
				&transaction->handle, &blob_handle, blob_id);
		}

		if (status[1]) {
			return status[1];
		}

		Blob blob(new CBlob(blob_handle, public_blob_handle, attachment, transaction));
		blob->flags |= flags;
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


extern "C" {

static ISC_STATUS no_entrypoint(ISC_STATUS * user_status, ...)
{
/**************************************
 *
 *	n o _ e n t r y p o i n t
 *
 **************************************
 *
 * Functional description
 *	No_entrypoint is called if there is not entrypoint for a given routine.
 *
 **************************************/

	Firebird::Arg::Gds(isc_unavailable).copyTo(user_status);

	return isc_unavailable;
}

} // extern "C"

static ISC_STATUS prepare(ISC_STATUS* user_status, Transaction transaction)
{
/**************************************
 *
 *	p r e p a r e
 *
 **************************************
 *
 * Functional description
 *	Perform the first phase of a two-phase commit
 *	for a multi-database transaction.
 *
 **************************************/
	Status status(user_status);

	Transaction sub;
	TEXT tdr_buffer[1024];
	size_t length = 0;
	int transcount = 0;

	for (sub = transaction->next; sub; sub = sub->next)
	{
		length += 256;
		++transcount;
	}
	// To do: use transcount to check the maximum allowed dbs in a two phase commit.

	TEXT host[64];
	ISC_get_host(host, sizeof(host));
	const size_t hostlen = strlen(host);
	length += hostlen + 3; // TDR_version + TDR_host_site + UCHAR(strlen(host))

	TEXT* const description = (length > sizeof(tdr_buffer)) ?
		(TEXT *) gds__alloc(length) : tdr_buffer;

/* build a transaction description record containing
   the host site and database/transaction
   information for the target databases. */

	TEXT* p = description;
	if (!p)
	{
		Firebird::Arg::Gds(isc_virmemexh).copyTo(status);
		return status[1];
	}

	*p++ = TDR_VERSION;
	*p++ = TDR_HOST_SITE;
	*p++ = UCHAR(hostlen);
	memcpy(p, host, hostlen);
	p += hostlen;

/* Get database and transaction stuff for each sub-transaction */

	for (sub = transaction->next; sub; sub = sub->next) {
		get_database_info(sub, &p);
		get_transaction_info(status, sub, &p);
	}

/* So far so good -- prepare each sub-transaction */

	length = p - description;

	for (sub = transaction->next; sub; sub = sub->next)
	{
		if (CALL(PROC_PREPARE, sub->implementation) (status, &sub->handle, length, description))
		{
			if (description != tdr_buffer) {
				free_block(description);
			}
			return status[1];
		}
	}

	if (description != tdr_buffer)
		free_block(description);

	return FB_SUCCESS;
}


static void save_error_string(ISC_STATUS* status)
{
/**************************************
 *
 *	s a v e _ e r r o r _ s t r i n g
 *
 **************************************
 *
 * Functional description
 *	This is need because there are cases
 *	where the memory allocated for strings
 *	in the status vector is freed prior to
 *	surfacing them to the user.  This is an
 *	attempt to save off 1 string to surface to
 *  	the user.  Any other strings will be set to
 *	a standard <Unknown> string.
 *
 **************************************/
	fb_assert(status != NULL);

	TEXT* p = glbstr1;
	ULONG len = sizeof(glbstr1) - 1;

	while (*status != isc_arg_end)
	{
		ULONG l;
		switch (*status++)
		{
		case isc_arg_cstring:
			l = (ULONG) * status;
			if (l < len)
			{
				status++;		/* Length is unchanged */
				/*
				 * This strncpy should really be a memcpy
				 */
				strncpy(p, reinterpret_cast<char*>(*status), l);
				*status++ = (ISC_STATUS) p;	/* string in static memory */
				p += l;
				len -= l;
			}
			else {
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
				*status++ = (ISC_STATUS) p;	/* string in static memory */
				p += l;
				len -= l;
			}
			else
			{
				*status++ = (ISC_STATUS) glbunknown;
			}
			break;

		default:
			fb_assert(FALSE);
		case isc_arg_gds:
		case isc_arg_number:
		case isc_arg_vms:
		case isc_arg_unix:
		case isc_arg_win32:
			status++;			/* Skip parameter */
			break;
		}
	}
}


static bool set_path(const PathName& file_name, PathName& expanded_name)
{
/**************************************
 *
 *	s e t _ p a t h
 *
 **************************************
 *
 * Functional description
 *	Set a prefix to a filename based on
 *	the ISC_PATH user variable.
 *
 **************************************/

	// look for the environment variables to tack
	// onto the beginning of the database path
	PathName pathname;
	if (!fb_utils::readenv("ISC_PATH", pathname))
		return false;

	// if the file already contains a remote node
	// or any path at all forget it
	for (const TEXT* p = file_name.c_str(); *p; p++)
	{
		if (*p == ':' || *p == '/' || *p == '\\')
			return false;
	}

	// concatenate the strings

	expanded_name = pathname;

	// CVC: Make the concatenation work if no slash is present.
	char lastChar = expanded_name[expanded_name.length() - 1];
	if (lastChar != ':' && lastChar != '/' && lastChar != '\\') {
		expanded_name.append(1, PathUtils::dir_sep);
	}

	expanded_name.append(file_name);

	return true;
}


#if !defined (SUPERCLIENT)
bool WHY_set_shutdown(bool flag)
{
/**************************************
 *
 *	W H Y _ s e t _ s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *	Set disableConnections to either true or false.
 *		true  = refuse new connections
 *		false = accept new connections
 *	Returns the prior state of the flag (server).
 *
 **************************************/

	const bool old_flag = disableConnections;
	disableConnections = flag;
	return old_flag;
}

bool WHY_get_shutdown()
{
/**************************************
 *
 *	W H Y _ g e t _ s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *	Returns the current value of disableConnections.
 *
 **************************************/

	return disableConnections;
}

// dimitr: to be removed in FB 3.0
FB_API_HANDLE WHY_get_public_attachment_handle(const void* handle)
{
/**************************************
 *
 *	W H Y _ g e t _ p u b l i c _ a t t a c h m e n t _ h a n d l e
 *
 **************************************
 *
 * Functional description
 *	Returns public attachment handle for a given private handle.
 *
 **************************************/

	return attachments().getPublicHandle(handle);
}
#endif // !SUPERCLIENT


static GlobalPtr<Mutex> singleShutdown;

int API_ROUTINE fb_shutdown(unsigned int timeout, const int reason)
{
/**************************************
 *
 *	f b _ s h u t d o w n
 *
 **************************************
 *
 * Functional description
 *	Shutdown firebird.
 *
 **************************************/
	MutexLockGuard guard(singleShutdown);

	if (shutdownStarted)
	{
		return FB_SUCCESS;
	}

	Status status(NULL);
	int rc = FB_SUCCESS;

#ifdef DEV_BUILD
	// ignore timeout in debug build: hard to debug something during 5-10 sec
	timeout = 0;
#endif

	try
	{
		// Ask clients about shutdown confirmation
		if (ShutChain::run(fb_shut_confirmation, reason) != FB_SUCCESS)
		{
			return FB_FAILURE;	// Do not perform former shutdown
		}

		// Shutdown clients before providers
		if (ShutChain::run(fb_shut_preproviders, reason) != FB_SUCCESS)
		{
			rc = FB_FAILURE;
		}

		// shutdown yValve
		shutdownStarted = true;	// since this moment no new thread will be able to enter yValve

		// Shutdown providers
		for (int n = 0; n < SUBSYSTEMS; ++n)
		{
			typedef int ShutType(unsigned int);
			PTR entry = get_entrypoint(PROC_SHUTDOWN, n);
			if (entry != no_entrypoint)
			{
				// this awful cast will be gone as soon as we will have
				// pure virtual functions based provider interface
				if (((ShutType*) entry)(timeout) != FB_SUCCESS)
				{
					rc = FB_FAILURE;
				}
			}
		}

		// Shutdown clients after providers
		if (ShutChain::run(fb_shut_postproviders, reason) != FB_SUCCESS)
		{
			rc = FB_FAILURE;
		}

		// Finish shutdown
		if (ShutChain::run(fb_shut_finish, reason) != FB_SUCCESS)
		{
			rc = FB_FAILURE;
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
		gds__log_status(0, status);
		return FB_SUCCESS;	// This seems to be a strange logic, but we should better
							// let it exit() when unexpected errors happen
	}

	return rc;
}


ISC_STATUS API_ROUTINE fb_shutdown_callback(ISC_STATUS* user_status,
											FB_SHUTDOWN_CALLBACK callBack,
											const int mask,
											void* arg)
{
/**************************************
 *
 *	f b _ s h u t d o w n _ c a l l b a c k
 *
 **************************************
 *
 * Functional description
 *	Register client callback to be called when FB is going down.
 *
 **************************************/
	Status status(user_status);

	try
	{
		ShutChain::add(callBack, mask, arg);
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}


ISC_STATUS API_ROUTINE fb_ping(ISC_STATUS* user_status, FB_API_HANDLE* db_handle)
{
/**************************************
 *
 *	f b _ p i n g
 *
 **************************************
 *
 * Functional description
 *	Check the attachment handle for persistent errors.
 *
 **************************************/
	Status status(user_status);

	try
	{
		Attachment attachment = translate<CAttachment>(db_handle);
		YEntry entryGuard(status, attachment);

		if (CALL(PROC_PING, attachment->implementation) (status, &attachment->handle))
		{
			if (!attachment->status.getError()) {
				attachment->status.save(status);
			}
			CALL(PROC_DETACH, attachment->implementation) (status, &attachment->handle);
			status_exception::raise(attachment->status.value());
		}
	}
	catch (const Exception& e)
	{
		e.stuff_exception(status);
	}

	return status[1];
}
