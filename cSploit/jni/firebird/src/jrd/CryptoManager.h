/*
 *	PROGRAM:		JRD access method
 *	MODULE:			CryptoManager.h
 *	DESCRIPTION:	Database encryption
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
 *  The Original Code was created by Alex Peshkov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2012 Alex Peshkov <peshkoff at mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *
 */

#ifndef JRD_CRYPTO_MANAGER
#define JRD_CRYPTO_MANAGER

#include "../common/classes/alloc.h"
#include "../common/thd.h"
#include "../common/classes/fb_atomic.h"
#include "../common/classes/SyncObject.h"
#include "../common/classes/fb_string.h"
#include "../common/classes/objects_array.h"
#include "../common/classes/stack.h"
#include "../common/ThreadStart.h"
#include "../jrd/ods.h"

// forward

class Config;

namespace Firebird {
	class IDbCryptPlugin;
	class IKeyHolderPlugin;
}

namespace Ods {
	struct pag;
}

namespace Jrd {

class Database;
class Attachment;
class jrd_file;
class BufferDesc;
class thread_db;
class Lock;

class CryptoManager : public Firebird::PermanentStorage
{
public:
	explicit CryptoManager(thread_db* tdbb);
	~CryptoManager();

	void shutdown(thread_db* tdbb);

	void changeCryptState(thread_db* tdbb, const Firebird::string& plugName);
	void attach(thread_db* tdbb, Attachment* att);
	void detach(thread_db* tdbb, Attachment* att);

	void startCryptThread(thread_db* tdbb);
	void terminateCryptThread(thread_db* tdbb);

	bool decrypt(ISC_STATUS* sv, Ods::pag* page);
	Ods::pag* encrypt(ISC_STATUS* sv, Ods::pag* from, Ods::pag* to);

	void cryptThread();

	ULONG getCurrentPage();

	class Buffer
	{
	public:
		operator Ods::pag*()
		{
			return reinterpret_cast<Ods::pag*> (FB_ALIGN(reinterpret_cast<U_IPTR>(buf),
				MIN_PAGE_SIZE));
		}

	private:
		char buf[MAX_PAGE_SIZE + MIN_PAGE_SIZE - 1];
	};

private:
	class HolderAttachments
	{
	public:
		explicit HolderAttachments(Firebird::MemoryPool& p);
		~HolderAttachments();

		void registerAttachment(Attachment* att);
		bool unregisterAttachment(Attachment* att);

		void setPlugin(Firebird::IKeyHolderPlugin* kh);
		Firebird::IKeyHolderPlugin* getPlugin() const
		{
			return keyHolder;
		}

		bool operator==(Firebird::IKeyHolderPlugin* kh) const;

	private:
		Firebird::IKeyHolderPlugin* keyHolder;
		Firebird::HalfStaticArray<Attachment*, 32> attachments;
	};

	class KeyHolderPlugins
	{
	public:
		explicit KeyHolderPlugins(Firebird::MemoryPool& p)
			: knownHolders(p)
		{ }

		void attach(Attachment* att, Config* config);
		void detach(Attachment* att);
		void init(Firebird::IDbCryptPlugin* crypt);

	private:
		Firebird::Mutex holdersMutex;
		Firebird::ObjectsArray<HolderAttachments> knownHolders;
	};

	static int blockingAstChangeCryptState(void*);
	void blockingAstChangeCryptState();

	void takeStateLock(thread_db* tdbb);
	void loadPlugin(const char* pluginName);
	ULONG getLastPage(thread_db* tdbb);
	void writeDbHeader(thread_db* tdbb, ULONG runpage, Firebird::Stack<ULONG>& pages);

	Firebird::AtomicCounter currentPage;
	Firebird::Mutex pluginLoadMtx, cryptThreadMtx;
	KeyHolderPlugins keyHolderPlugins;
	Thread::Handle cryptThreadId;
	Firebird::IDbCryptPlugin* cryptPlugin;
	Database& dbb;
	Lock* stateLock;
	Lock* threadLock;
	bool needLock, crypt, process, down;
};

} // namespace Jrd


#endif // JRD_CRYPTO_MANAGER
