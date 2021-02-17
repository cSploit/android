/*
 *	PROGRAM:		JRD access method
 *	MODULE:			Mapping.cpp
 *	DESCRIPTION:	Maps names in authentication block
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

#include "firebird.h"
#include "firebird/Provider.h"
#include "../auth/SecureRemotePassword/Message.h"
#include "gen/iberror.h"

#include "../jrd/constants.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/classes/init.h"
#include "../common/classes/Hash.h"
#include "../common/classes/GenericMap.h"
#include "../common/classes/RefMutex.h"
#include "../common/classes/SyncObject.h"
#include "../common/classes/MetaName.h"
#include "../common/isc_s_proto.h"
#include "../common/isc_proto.h"
#include "../common/ThreadStart.h"
#include "../common/db_alias.h"

#include "../jrd/Mapping.h"
#include "../jrd/tra.h"
#include "../jrd/ini.h"
#include "gen/ids.h"

#ifdef WIN_NT
#include <process.h>
#define getpid _getpid
#endif

#define MAP_DEBUG(A)

using namespace Firebird;

namespace {

const unsigned FLAG_DB = 1;
const unsigned FLAG_SEC = 2;

const unsigned FLAG_USER = 1;
const unsigned FLAG_ROLE = 2;

const char* NM_ROLE = "Role";
const char* NM_USER = "User";

void check(const char* s, IStatus* st)
{
	if (st->isSuccess())
		return;

	Arg::StatusVector newStatus(st->get());
	newStatus << Arg::Gds(isc_map_load) << s;
	newStatus.raise();
}

class AuthWriter : public ClumpletWriter
{
public:
	AuthWriter()
		: ClumpletWriter(WideUnTagged, MAX_DPB_SIZE), sequence(0)
	{ }

	void append(AuthWriter& w)
	{
		internalAppend(w);
	}

	void append(const AuthReader::AuthBlock& b)
	{
		ClumpletReader r(WideUnTagged, b.begin(), b.getCount());
		internalAppend(r);
	}

	void add(const AuthReader::Info& info)
	{
		ClumpletWriter to(WideUnTagged, MAX_DPB_SIZE);

		add(to, AuthReader::AUTH_TYPE, info.type);
		add(to, AuthReader::AUTH_NAME, info.name);
		add(to, AuthReader::AUTH_PLUGIN, info.plugin);
		add(to, AuthReader::AUTH_SECURE_DB, info.secDb);

		if (to.getBufferLength())
		{
			moveNext();
			insertBytes(sequence++, to.getBuffer(), to.getBufferLength());
		}
	}

private:
	void add(ClumpletWriter& to, const unsigned char tag, const NoCaseString& str)
	{
		if (str.hasData())
		{
			to.insertString(tag, str.c_str(), str.length());
		}
	}

	void internalAppend(ClumpletReader& w)
	{
		while (!isEof())
		{
			moveNext();
		}

		for(w.rewind(); !w.isEof(); w.moveNext())
		{
			SingleClumplet sc = w.getClumplet();
			sc.tag = sequence++;
			insertClumplet(sc);
			moveNext();
		}
	}

	unsigned char sequence;
};

class Map;
typedef Hash<Map, DEFAULT_HASH_SIZE, Map, DefaultKeyValue<Map>, Map> MapHash;

class Map : public MapHash::Entry, public GlobalStorage
{
public:
	static size_t hash(const Map& value, size_t hashSize)
	{
		NoCaseString key = value.makeHashKey();
		return DefaultHash<Map>::hash(key.c_str(), key.length(), hashSize);
	}

	NoCaseString makeHashKey() const
	{
		NoCaseString key;
		key += usng;
		MAP_DEBUG(key += ':');
		key += plugin;
		MAP_DEBUG(key += ':');
		key += db;
		MAP_DEBUG(key += ':');
		key += fromType;
		MAP_DEBUG(key += ':');
		key += from;

		key.upper();
		return key;
	}

	NoCaseString plugin, db, fromType, from, to;
	bool toRole;
	char usng;

	Map(const char* aUsing, const char* aPlugin, const char* aDb,
		const char* aFromType, const char* aFrom,
		SSHORT aRole, const char* aTo)
		: plugin(getPool()), db(getPool()), fromType(getPool()),
		  from(getPool()), to(getPool()), toRole(aRole ? true : false), usng(aUsing[0])
	{
		plugin = aPlugin;
		db = aDb;
		fromType = aFromType;
		from = aFrom;
		to = aTo;

		trimAll();
	}

	explicit Map(AuthReader::Info& info)   //type, name, plugin, secDb
		: plugin(getPool()), db(getPool()),
		  fromType(getPool()), from(getPool()), to(getPool()),
		  toRole(false), usng(info.plugin.hasData() ? 'P' : 'M')
	{
		plugin = info.plugin.hasData() ? info.plugin.c_str() : "*";
		db = info.secDb.hasData() ? info.secDb.c_str() : "*";
		fromType = info.type;
		from = info.name.hasData() ? info.name.c_str() : "*";

		trimAll();
	}

	void trimAll()
	{
		plugin.rtrim();
		db.rtrim();
		fromType.rtrim();
		from.rtrim();
		to.rtrim();
	}

	virtual bool isEqual(const Map& k) const
	{
		return usng == k.usng &&
			plugin == k.plugin &&
			db == k.db &&
			fromType == k.fromType &&
			from == k.from ;
	}

	virtual Map* get()
	{
		return this;
	}
};

class Cache : public MapHash, public GlobalStorage
{
public:
	Cache(const NoCaseString& aliasDb, const NoCaseString& db)
		: alias(getPool(), aliasDb), name(getPool(), db), dataFlag(false)
	{
		enableDuplicates();
	}

	void populate(IAttachment *att)
	{
		LocalStatus st;

		if (dataFlag)
		{
			if (att)
				att->detach(&st);
			return;
		}

		if (!att)
		{
			dataFlag = true;
			return;
		}

		MAP_DEBUG(fprintf(stderr, "Populate cache for %s\n", name.c_str()));

		ITransaction* tra = NULL;
		IResultSet* curs = NULL;

		try
		{
			cleanup(eraseEntry);

			ClumpletWriter readOnly(ClumpletWriter::Tpb, MAX_DPB_SIZE, isc_tpb_version1);
			readOnly.insertTag(isc_tpb_read);
			readOnly.insertTag(isc_tpb_wait);
			tra = att->startTransaction(&st, readOnly.getBufferLength(), readOnly.getBuffer());
			check("IAttachment::startTransaction", &st);

			Message mMap;
			Field<Text> usng(mMap, 1);
			Field<Varying> plugin(mMap, MAX_SQL_IDENTIFIER_SIZE);
			Field<Varying> db(mMap, MAX_SQL_IDENTIFIER_SIZE);
			Field<Varying> fromType(mMap, MAX_SQL_IDENTIFIER_SIZE);
			Field<Varying> from(mMap, 255);
			Field<SSHORT> role(mMap);
			Field<Varying> to(mMap, MAX_SQL_IDENTIFIER_SIZE);

			curs = att->openCursor(&st, tra, 0,
				"SELECT RDB$MAP_USING, RDB$MAP_PLUGIN, RDB$MAP_DB, RDB$MAP_FROM_TYPE, "
				"	RDB$MAP_FROM, RDB$MAP_TO_TYPE, RDB$MAP_TO "
				"FROM RDB$MAP",
				3, NULL, NULL, mMap.getMetadata());
			if (!st.isSuccess())
			{
				if (fb_utils::containsErrorCode(st.get(), isc_dsql_relation_err))
				{
					// isc_dsql_relation_err when opening cursor - sooner of all table RDB$MAP
					// is missing due to non-FB3 security DB
					tra->release();
					att->detach(&st);
					dataFlag = true;
					return;
				}
				check("IAttachment::openCursor", &st);
			}

			while (curs->fetchNext(&st, mMap.getBuffer()))
			{
				const char* expandedDb = "*";
				PathName target;
				if (!db.null)
				{
					expandedDb = db;
					MAP_DEBUG(fprintf(stderr, "non-expandedDb '%s'\n", expandedDb));
					expandDatabaseName(expandedDb, target, NULL);
					expandedDb = target.c_str();
					MAP_DEBUG(fprintf(stderr, "expandedDb '%s'\n", expandedDb));
				}

				Map* map = new Map(usng, plugin.null ? "*" : plugin, expandedDb,
					fromType, from, role, to.null ? "*" : to);
				MAP_DEBUG(fprintf(stderr, "Add = %s\n", map->makeHashKey().c_str()));
				add(map);
			}
			check("IResultSet::fetchNext", &st);

			curs->close(&st);
			check("IResultSet::close", &st);
			curs = NULL;

			tra->rollback(&st);
			check("ITransaction::rollback", &st);
			tra = NULL;

			att->detach(&st);
			check("IAttachment::detach", &st);
			att = NULL;

			dataFlag = true;
		}
		catch (const Exception&)
		{
			if (curs)
				curs->release();
			if (tra)
				tra->release();
			if (att)
				att->detach(&st);
			throw;
		}
	}

	void map(bool flagWild, AuthReader::Info& info, AuthWriter& newBlock)
	{
		Map from(info);

		if (from.from == "*")
			Arg::Gds(isc_map_aster).raise();

		if (!flagWild)
			search(info, from, newBlock, from.from);
		else
			varUsing(info, from, newBlock);
	}

	void search(AuthReader::Info& info, const Map& from, AuthWriter& newBlock,
		const NoCaseString& originalUserName)
	{
		MAP_DEBUG(fprintf(stderr, "Key = %s\n", from.makeHashKey().c_str()));
		if (!dataFlag)
			return;

		for (Map* to = lookup(from); to; to = to->next(from))
		{
			MAP_DEBUG(fprintf(stderr, "Match!!\n"));
			unsigned flagRole = to->toRole ? FLAG_ROLE : FLAG_USER;
			if (info.found & flagRole)
				continue;
			if (info.current & flagRole)
				(Arg::Gds(isc_map_multi) << originalUserName).raise();

			info.current |= flagRole;

			AuthReader::Info newInfo;
			newInfo.type = to->toRole ? NM_ROLE : NM_USER;
			newInfo.name = to->to == "*" ? originalUserName : to->to;
	        newInfo.secDb = this->name;
			newBlock.add(newInfo);
		}
	}

	void varPlugin(AuthReader::Info& info, Map from, AuthWriter& newBlock)
	{
		varDb(info, from, newBlock);
		if (from.plugin != "*")
		{
			from.plugin = "*";
			varDb(info, from, newBlock);
		}
	}

	void varDb(AuthReader::Info& info, Map from, AuthWriter& newBlock)
	{
		varFrom(info, from, newBlock);
		if (from.db != "*")
		{
			from.db = "*";
			varFrom(info, from, newBlock);
		}
	}

	void varFrom(AuthReader::Info& info, Map from, AuthWriter& newBlock)
	{
		NoCaseString originalUserName = from.from;
		search(info, from, newBlock, originalUserName);
		from.from = "*";
		search(info, from, newBlock, originalUserName);
	}

	void varUsing(AuthReader::Info& info, Map from, AuthWriter& newBlock)
	{
		if (from.usng == 'P')
		{
			varPlugin(info, from, newBlock);

			from.usng = '*';
			varPlugin(info, from, newBlock);

			if (!info.secDb.hasData())
			{
				from.usng = 'S';
				from.plugin = "*";
				varDb(info, from, newBlock);
			}
		}
		else if (from.usng == 'M')
		{
			varDb(info, from, newBlock);

			from.usng = '*';
			varDb(info, from, newBlock);
		}
		else
			fb_assert(false);
	}

	bool map4(bool flagWild, unsigned flagSet, AuthReader& rdr, AuthReader::Info& info, AuthWriter& newBlock)
	{
		if (!flagSet)
		{
			AuthWriter workBlock;

			for (rdr.rewind(); rdr.getInfo(info); rdr.moveNext())
			{
				map(flagWild, info, workBlock);
			}

			info.found |= info.current;
			info.current = 0;
			newBlock.append(workBlock);
		}

		unsigned mapMask = FLAG_USER | FLAG_ROLE;
		return (info.found & mapMask) == mapMask;
	}

	static void eraseEntry(Map* m)
	{
		delete m;
	}

	void makeEmpty()
	{
		if (!dataFlag)
			return;

		dataFlag = false;
		cleanup(eraseEntry);
	}

public:
	SyncObject syncObject;
	NoCaseString alias, name;
	bool dataFlag;
};

typedef GenericMap<Pair<Left<NoCaseString, Cache*> > > CacheTree;

InitInstance<CacheTree> tree;
GlobalPtr<Mutex> treeMutex;

void setupIpc();

Cache* locate(const NoCaseString& target)
{
	fb_assert(treeMutex->locked());
	Cache* c;
	return tree().get(target, c) ? c : NULL;
}

Cache* locate(const NoCaseString& alias, const NoCaseString& target)
{
	fb_assert(treeMutex->locked());
	Cache* c = locate(target);
	if (!c)
	{
		c = new Cache(alias, target);
		*(tree().put(target)) = c;

		setupIpc();
	}
	return c;
}

class Found
{
public:
	enum What {FND_NOTHING, FND_SEC, FND_DB};

	Found()
		: found(FND_NOTHING)
	{ }

	void set(What find, NoCaseString& val, NoCaseString& m)
	{
		if (find == found && value != val)
			Arg::Gds(isc_map_undefined).raise();
		if (find > found)
		{
			found = find;
			value = val;
			method = m;
		}
	}

	NoCaseString value;
	NoCaseString method;
	What found;
};

void resetMap(const char* securityDb)
{
	MutexLockGuard g(treeMutex, FB_FUNCTION);

	Cache* cache = locate(securityDb);
	if (!cache)
	{
		MAP_DEBUG(fprintf(stderr, "Cache not found for %s\n", securityDb));
		return;
	}

	Sync sync(&cache->syncObject, FB_FUNCTION);
	sync.lock(SYNC_EXCLUSIVE);
	cache->makeEmpty();
	MAP_DEBUG(fprintf(stderr, "Empty cache for %s\n", securityDb));
}


// ----------------------------------------------------

class MappingHeader : public Firebird::MemoryHeader
{
public:
	SLONG currentProcess;
	ULONG processes;
	char databaseForReset[1024];

	struct Process
	{
		event_t notifyEvent;
		event_t callbackEvent;
		SLONG id;
		SLONG flags;
	};
	Process process[1];

	static const ULONG FLAG_ACTIVE = 0x1;
	static const ULONG FLAG_DELIVER = 0x2;
};

class MappingIpc FB_FINAL : public Firebird::IpcObject
{
	static const ULONG MAPPING_VERSION = 1;
	static const size_t DEFAULT_SIZE = 1024 * 1024;

public:
	explicit MappingIpc(MemoryPool&)
		: processId(getpid())
	{ }

	~MappingIpc()
	{
		if (!sharedMemory)
			return;

		Guard gShared(this);

		MappingHeader* sMem = sharedMemory->getHeader();

		sMem->process[process].flags &= ~MappingHeader::FLAG_ACTIVE;
		(void)  // Ignore errors in cleanup
            sharedMemory->eventPost(&sMem->process[process].notifyEvent);
		Thread::waitForCompletion(threadHandle);

		// Ignore errors in cleanup
		sharedMemory->eventFini(&sMem->process[process].notifyEvent);
		sharedMemory->eventFini(&sMem->process[process].callbackEvent);

		if (sharedMemory->getHeader()->processes == 1)
			sharedMemory->removeMapFile();
	}

	void clearMap(const char* dbName)
	{
		PathName target;
		expandDatabaseName(dbName, target, NULL);

		setup();

		Guard gShared(this);

		MappingHeader* sMem = sharedMemory->getHeader();
		target.copyTo(sMem->databaseForReset, sizeof(sMem->databaseForReset));

		// Set currentProcess
		sMem->currentProcess = -1;
		for (unsigned n = 0; n < sMem->processes; ++n)
		{
			MappingHeader::Process* p = &sMem->process[n];
			if (!(p->flags & MappingHeader::FLAG_ACTIVE))
				continue;

			if (p->id == processId)
			{
				sMem->currentProcess = n;
				break;
			}
		}

		if (sMem->currentProcess < 0)
		{
			// did not find current process
			// better ignore delivery than fail in it
			gds__log("MappingIpc::clearMap() failed to find current process %d in shared memory", processId);
			return;
		}
		MappingHeader::Process* current = &sMem->process[sMem->currentProcess];

		// Deliver
		for (unsigned n = 0; n < sMem->processes; ++n)
		{
			MappingHeader::Process* p = &sMem->process[n];
			if (!(p->flags & MappingHeader::FLAG_ACTIVE))
				continue;

			if (p->id == processId)
			{
				MAP_DEBUG(fprintf(stderr, "Internal resetMap(%s)\n", sMem->databaseForReset));
				resetMap(sMem->databaseForReset);
				continue;
			}

			SLONG value = sharedMemory->eventClear(&current->callbackEvent);
			p->flags |= MappingHeader::FLAG_DELIVER;
			if (sharedMemory->eventPost(&p->notifyEvent) != FB_SUCCESS)
			{
				(Arg::Gds(isc_random) << "Error posting notifyEvent in mapping shared memory").raise();
			}
			while (sharedMemory->eventWait(&current->callbackEvent, value, 10000) != FB_SUCCESS)
			{
				if (!ISC_check_process_existence(p->id))
				{
					p->flags &= ~MappingHeader::FLAG_ACTIVE;
					sharedMemory->eventFini(&sMem->process[process].notifyEvent);
					sharedMemory->eventFini(&sMem->process[process].callbackEvent);
					break;
				}
			}
			MAP_DEBUG(fprintf(stderr, "Notified pid %d about reset map %s\n", p->id, sMem->databaseForReset));
		}
	}

	void setup()
	{
		if (sharedMemory)
			return;
		MutexLockGuard gLocal(initMutex, FB_FUNCTION);
		if (sharedMemory)
			return;

		Arg::StatusVector statusVector;
		try
		{
			sharedMemory.reset(FB_NEW(*getDefaultMemoryPool())
				SharedMemory<MappingHeader>("fb_user_mapping", DEFAULT_SIZE, this));
		}
		catch (const Exception& ex)
		{
			iscLogException("MonitoringData: Cannot initialize the shared memory region", ex);
			throw;
		}
		fb_assert(sharedMemory->getHeader()->mhb_version == MAPPING_VERSION);

		Guard gShared(this);

		MappingHeader* sMem = sharedMemory->getHeader();

		for (process = 0; process < sMem->processes; ++process)
		{
			if (!(sMem->process[process].flags & MappingHeader::FLAG_ACTIVE))
				break;
			if (!ISC_check_process_existence(processId))
			{
				sharedMemory->eventFini(&sMem->process[process].notifyEvent);
				sharedMemory->eventFini(&sMem->process[process].callbackEvent);
				break;
			}
		}

		if (process >= sMem->processes)
		{
			sMem->processes++;
			if (((U_IPTR) &sMem->process[sMem->processes]) - ((U_IPTR) sMem) > DEFAULT_SIZE)
			{
				sMem->processes--;
				(Arg::Gds(isc_random) << "Global mapping memory overflow").raise();
			}
		}

		sMem->process[process].id = processId;
		sMem->process[process].flags = MappingHeader::FLAG_ACTIVE;
		if (sharedMemory->eventInit(&sMem->process[process].notifyEvent) != FB_SUCCESS)
		{
			(Arg::Gds(isc_random) << "Error initializing notifyEvent in mapping shared memory").raise();
		}
		if (sharedMemory->eventInit(&sMem->process[process].callbackEvent) != FB_SUCCESS)
		{
			(Arg::Gds(isc_random) << "Error initializing callbackEvent in mapping shared memory").raise();
		}

		try
		{
			Thread::start(clearDelivery, this, THREAD_high, &threadHandle);
		}
		catch (const Exception&)
		{
			sMem->process[process].flags &= ~MappingHeader::FLAG_ACTIVE;
			throw;
		}
	}

private:
	void clearDeliveryThread()
	{
		try
		{
			MappingHeader::Process* p = &sharedMemory->getHeader()->process[process];
			while (p->flags & MappingHeader::FLAG_ACTIVE)
			{
				SLONG value = sharedMemory->eventClear(&p->notifyEvent);

				if (p->flags & MappingHeader::FLAG_DELIVER)
				{
					resetMap(sharedMemory->getHeader()->databaseForReset);

					MappingHeader* sMem = sharedMemory->getHeader();
					MappingHeader::Process* cur = &sMem->process[sMem->currentProcess];
					if (sharedMemory->eventPost(&cur->callbackEvent) != FB_SUCCESS)
					{
						(Arg::Gds(isc_random) << "Error posting callbackEvent in mapping shared memory").raise();
					}
					p->flags &= ~MappingHeader::FLAG_DELIVER;
				}

				if (sharedMemory->eventWait(&p->notifyEvent, value, 0) != FB_SUCCESS)
				{
					(Arg::Gds(isc_random) << "Error waiting for notifyEvent in mapping shared memory").raise();
				}
			}
		}
		catch (const Exception& ex)
		{
			iscLogException("Fatal error in clearDeliveryThread", ex);
			fb_utils::logAndDie("Fatal error in clearDeliveryThread");
		}
	}

	// implement pure virtual functions
	bool initialize(SharedMemoryBase* sm, bool initFlag)
	{
		if (initFlag)
		{
			MappingHeader* header = reinterpret_cast<MappingHeader*>(sm->sh_mem_header);

			// Initialize the shared data header
			header->mhb_type = SharedMemoryBase::SRAM_MAPPING_RESET;
			header->mhb_version = MAPPING_VERSION;
			header->mhb_timestamp = TimeStamp::getCurrentTimeStamp().value();

			header->processes = 0;
			header->currentProcess = -1;
		}

		return true;
	}

	void mutexBug(int osErrorCode, const char* text)
	{
		iscLogStatus("Error when working with user mapping shared memory",
			(Arg::Gds(isc_sys_request) << text << Arg::OsError(osErrorCode)).value());
	}

	// copying is prohibited
	MappingIpc(const MappingIpc&);
	MappingIpc& operator =(const MappingIpc&);

	class Guard;
	friend class Guard;

	class Guard
	{
	public:
		explicit Guard(MappingIpc* ptr)
			: data(ptr)
		{
			data->sharedMemory->mutexLock();
		}

		~Guard()
		{
			data->sharedMemory->mutexUnlock();
		}

	private:
		Guard(const Guard&);
		Guard& operator=(const Guard&);

		MappingIpc* const data;
	};

	static THREAD_ENTRY_DECLARE clearDelivery(THREAD_ENTRY_PARAM par)
	{
		MappingIpc* m = (MappingIpc*)par;
		m->clearDeliveryThread();
		return 0;
	}

	AutoPtr<SharedMemory<MappingHeader> > sharedMemory;
	Mutex initMutex;
	Thread::Handle threadHandle;
	const SLONG processId;
	unsigned process;
};

GlobalPtr<MappingIpc, InstanceControl::PRIORITY_DELETE_FIRST> mappingIpc;

void setupIpc()
{
	mappingIpc->setup();
}

} // anonymous namespace

namespace Jrd {

void mapUser(string& name, string& trusted_role, Firebird::string* auth_method,
	AuthReader::AuthBlock* newAuthBlock, const AuthReader::AuthBlock& authBlock,
	const char* alias, const char* db, const char* securityAlias)
{
	AuthReader::Info info;

	if (!securityAlias)
	{
		// We are in the error handler - perform minimum processing
		trusted_role = "";
		name = "<Unknown>";

		for (AuthReader rdr(authBlock); rdr.getInfo(info); rdr.moveNext())
		{
			if (info.type == NM_USER && info.name.hasData())
			{
				name = info.name.ToString();
				break;
			}
		}

		return;
	}

	// expand security database name (db is expected to be expanded, alias - original)
	PathName secExpanded;
	expandDatabaseName(securityAlias, secExpanded, NULL);
	const char* securityDb = secExpanded.c_str();

	// Create new writer
	AuthWriter newBlock;

	// detect presence of this databases mapping in authBlock
	// in that case mapUser was already invoked for it
	unsigned flags = db ? 0 : FLAG_DB;
	for (AuthReader rdr(authBlock); rdr.getInfo(info); rdr.moveNext())
	{
		if (db && info.secDb == db)
			flags |= FLAG_DB;
		if (info.secDb == securityDb)
			flags |= FLAG_SEC;
	}

	// Perform lock & map only when needed
	if (flags != (FLAG_DB | FLAG_SEC))
	{
		AuthReader::Info info;
		SyncType syncType = SYNC_SHARED;
		IAttachment* iDb = NULL;
		IAttachment* iSec = NULL;

		for (;;)
		{
			if (syncType == SYNC_EXCLUSIVE)
			{
				LocalStatus st;
				DispatcherPtr prov;

				ClumpletWriter embeddedSysdba(ClumpletWriter::Tagged,
					MAX_DPB_SIZE, isc_dpb_version1);
				embeddedSysdba.insertString(isc_dpb_user_name, SYSDBA_USER_NAME,
					strlen(SYSDBA_USER_NAME));
				embeddedSysdba.insertByte(isc_dpb_sec_attach, TRUE);
				embeddedSysdba.insertByte(isc_dpb_no_db_triggers, TRUE);

				if (!iSec)
				{
					iSec = prov->attachDatabase(&st, securityAlias,
						embeddedSysdba.getBufferLength(), embeddedSysdba.getBuffer());
					if (!st.isSuccess())
					{
						if (!fb_utils::containsErrorCode(st.get(), isc_io_error))
							check("IProvider::attachDatabase", &st);

						// missing security DB is not a reason to fail mapping
						iSec = NULL;
					}
				}

				if (db && !iDb)
				{
					const char* conf = "Providers=" CURRENT_ENGINE;
					embeddedSysdba.insertString(isc_dpb_config, conf, strlen(conf));

					if (!iDb)
					{
						iDb = prov->attachDatabase(&st, alias,
							embeddedSysdba.getBufferLength(), embeddedSysdba.getBuffer());
					}

					if (!st.isSuccess())
					{
						if (!fb_utils::containsErrorCode(st.get(), isc_io_error))
							check("IProvider::attachDatabase", &st);

						// missing DB is not a reason to fail mapping
						iDb = NULL;
					}
				}
			}

			MutexEnsureUnlock g(treeMutex, FB_FUNCTION);
			g.enter();

			Cache* cDb = NULL;
			if (db)
				cDb = locate(alias, db);
			Cache* cSec = locate(securityAlias, securityDb);

			SyncObject dummySync;
			Sync sDb((!(flags & FLAG_DB)) ? &cDb->syncObject : &dummySync, FB_FUNCTION);
			Sync sSec((!(flags & FLAG_SEC)) ? &cSec->syncObject : &dummySync, FB_FUNCTION);

			sSec.lock(syncType);
			if (!sDb.lockConditional(syncType))
			{
				// Avoid deadlocks cause hell knows which db is security for which
				sSec.unlock();
				// Now safely wait for sSec
				sDb.lock(syncType);
				// and repeat whole operation
				continue;
			}

			// Required cache(s) are locked somehow - release treeMutex
			g.leave();

			// Check is it required to populate caches from DB
			if ((cDb && !cDb->dataFlag) || !cSec->dataFlag)
			{
				if (syncType != SYNC_EXCLUSIVE)
				{
					syncType = SYNC_EXCLUSIVE;
					sSec.unlock();
					sDb.unlock();

					continue;
				}

				if (cDb)
					cDb->populate(iDb);
				cSec->populate(iSec);

				sSec.downgrade(SYNC_SHARED);
				sDb.downgrade(SYNC_SHARED);
			}

			// Caches are ready somehow - proceed with analysis
			AuthReader auth(authBlock);

			// Map in simple mode first main, next security db
			if (cDb && cDb->map4(false, flags & FLAG_DB, auth, info, newBlock))
				break;
			if (cSec->map4(false, flags & FLAG_SEC, auth, info, newBlock))
				break;

			// Map in wildcard mode first main, next security db
			if (cDb && cDb->map4(true, flags & FLAG_DB, auth, info, newBlock))
				break;
			cSec->map4(true, flags & FLAG_SEC, auth, info, newBlock);

			break;
	  	}

		for (AuthReader rdr(newBlock); rdr.getInfo(info); rdr.moveNext())
		{
			if (db && info.secDb == db)
				flags |= FLAG_DB;
			if (info.secDb == securityDb)
				flags |= FLAG_SEC;
		}

		// mark both DBs as 'seen'
		info.plugin = "";
		info.name = "";
		info.type = "Seen";

		if (!(flags & FLAG_DB))
		{
			info.secDb = db;
			newBlock.add(info);
		}

		if (!(flags & FLAG_SEC))
		{
			info.secDb = securityDb;
			newBlock.add(info);
		}
	}

	newBlock.append(authBlock);

	Found fName, fRole;
	MAP_DEBUG(fprintf(stderr, "Starting newblock scan\n"));
	for (AuthReader scan(newBlock); scan.getInfo(info); scan.moveNext())
	{
		MAP_DEBUG(fprintf(stderr, "Newblock info: secDb=%s plugin=%s type=%s name=%s\n",
			info.secDb.c_str(), info.plugin.c_str(), info.type.c_str(), info.name.c_str()));

		Found::What recordWeight =
			(db && info.secDb == db) ? Found::FND_DB :
			(info.secDb == securityDb) ? Found::FND_SEC :
			Found::FND_NOTHING;

		if (recordWeight != Found::FND_NOTHING)
		{
			if (info.type == NM_USER)
				fName.set(recordWeight, info.name, info.plugin);
			else if (info.type == NM_ROLE)
				fRole.set(recordWeight, info.name, info.plugin);
		}
	}

	if (fName.found == Found::FND_NOTHING)
		(Arg::Gds(isc_sec_context) << alias).raise();

	name = fName.value.ToString();
	trusted_role = fRole.value.ToString();
	MAP_DEBUG(fprintf(stderr, "login=%s tr=%s\n", name.c_str(), trusted_role.c_str()));
	if (auth_method)
		*auth_method = fName.method.ToString();

	if (newAuthBlock)
	{
		newAuthBlock->shrink(0);
		newAuthBlock->push(newBlock.getBuffer(), newBlock.getBufferLength());
		MAP_DEBUG(fprintf(stderr, "Saved to newAuthBlock %" SIZEFORMAT " bytes\n", newAuthBlock->getCount()));
	}
}

void clearMap(const char* dbName)
{
	mappingIpc->clearMap(dbName);
}

const Format* GlobalMappingScan::getFormat(thread_db* tdbb, jrd_rel* relation) const
{
	jrd_tra* const transaction = tdbb->getTransaction();
	return transaction->getMappingList()->getList(tdbb, relation)->getFormat();
}

bool GlobalMappingScan::retrieveRecord(thread_db* tdbb, jrd_rel* relation,
									FB_UINT64 position, Record* record) const
{
	jrd_tra* const transaction = tdbb->getTransaction();
	return transaction->getMappingList()->getList(tdbb, relation)->fetch(position, record);
}

MappingList::MappingList(jrd_tra* tra)
	: DataDump(*tra->tra_pool)
{ }

RecordBuffer* MappingList::makeBuffer(thread_db* tdbb)
{
	MemoryPool* const pool = tdbb->getTransaction()->tra_pool;
	allocBuffer(tdbb, *pool, rel_sec_global_map);
	return getData(rel_sec_global_map);
}

RecordBuffer* MappingList::getList(thread_db* tdbb, jrd_rel* relation)
{
	fb_assert(relation);
	fb_assert(relation->rel_id == rel_sec_global_map);

	RecordBuffer* buffer = getData(relation);
	if (buffer)
	{
		return buffer;
	}

	LocalStatus st;
	DispatcherPtr prov;
	IAttachment* att = NULL;
	ITransaction* tra = NULL;
	IResultSet* curs = NULL;

	try
	{
		ClumpletWriter embeddedSysdba(ClumpletWriter::Tagged,
			MAX_DPB_SIZE, isc_dpb_version1);
		embeddedSysdba.insertString(isc_dpb_user_name, SYSDBA_USER_NAME,
			strlen(SYSDBA_USER_NAME));
		embeddedSysdba.insertByte(isc_dpb_sec_attach, TRUE);
		embeddedSysdba.insertByte(isc_dpb_no_db_triggers, TRUE);

		const char* dbName = tdbb->getDatabase()->dbb_config->getSecurityDatabase();
		att = prov->attachDatabase(&st, dbName,
			embeddedSysdba.getBufferLength(), embeddedSysdba.getBuffer());
		if (!st.isSuccess())
		{
			if (!fb_utils::containsErrorCode(st.get(), isc_io_error))
				check("IProvider::attachDatabase", &st);

			// In embedded mode we are not raising any errors - silent return
			if (MasterInterfacePtr()->serverMode(-1) < 0)
				return makeBuffer(tdbb);

			(Arg::Gds(isc_map_nodb) << dbName).raise();
		}

		ClumpletWriter readOnly(ClumpletWriter::Tpb, MAX_DPB_SIZE, isc_tpb_version1);
		readOnly.insertTag(isc_tpb_read);
		readOnly.insertTag(isc_tpb_wait);
		tra = att->startTransaction(&st, readOnly.getBufferLength(), readOnly.getBuffer());
		check("IAttachment::startTransaction", &st);

		Message mMap;
		Field<Varying> name(mMap, MAX_SQL_IDENTIFIER_SIZE);
		Field<Text> usng(mMap, 1);
		Field<Varying> plugin(mMap, MAX_SQL_IDENTIFIER_SIZE);
		Field<Varying> db(mMap, MAX_SQL_IDENTIFIER_SIZE);
		Field<Varying> fromType(mMap, MAX_SQL_IDENTIFIER_SIZE);
		Field<Varying> from(mMap, 255);
		Field<SSHORT> role(mMap);
		Field<Varying> to(mMap, MAX_SQL_IDENTIFIER_SIZE);

		curs = att->openCursor(&st, tra, 0,
			"SELECT RDB$MAP_NAME, RDB$MAP_USING, RDB$MAP_PLUGIN, RDB$MAP_DB, "
			"	RDB$MAP_FROM_TYPE, RDB$MAP_FROM, RDB$MAP_TO_TYPE, RDB$MAP_TO "
			"FROM RDB$MAP",
			3, NULL, NULL, mMap.getMetadata());
		if (!st.isSuccess())
		{
			if (!fb_utils::containsErrorCode(st.get(), isc_dsql_relation_err))
				check("IAttachment::openCursor", &st);

			// isc_dsql_relation_err when opening cursor - sooner of all table RDB$MAP
			// is missing due to non-FB3 security DB
			tra->release();
			att->detach(&st);

			// In embedded mode we are not raising any errors - silent return
			if (MasterInterfacePtr()->serverMode(-1) < 0)
				return makeBuffer(tdbb);

			(Arg::Gds(isc_map_notable) << dbName).raise();
		}

		buffer = makeBuffer(tdbb);
		Record* record = buffer->getTempRecord();

		while (curs->fetchNext(&st, mMap.getBuffer()))
		{
			int charset = CS_METADATA;
			record->nullify();

			putField(tdbb, record,
					 DumpField(f_sec_map_name, VALUE_STRING, name->len, name->data),
					 charset);

			putField(tdbb, record,
					 DumpField(f_sec_map_using, VALUE_STRING, 1, usng->data),
					 charset);

			if (!plugin.null)
			{
				putField(tdbb, record,
						 DumpField(f_sec_map_plugin, VALUE_STRING, plugin->len, plugin->data),
						 charset);
			}

			if (!db.null)
			{
				putField(tdbb, record,
						 DumpField(f_sec_map_db, VALUE_STRING, db->len, db->data),
						 charset);
			}

			if (!fromType.null)
			{
				putField(tdbb, record,
						 DumpField(f_sec_map_from_type, VALUE_STRING, fromType->len, fromType->data),
						 charset);
			}

			if (!from.null)
			{
				putField(tdbb, record,
						 DumpField(f_sec_map_from, VALUE_STRING, from->len, from->data),
						 charset);
			}

			if (!role.null)
			{
				SINT64 v = role;
				putField(tdbb, record,
						 DumpField(f_sec_map_to_type, VALUE_INTEGER, sizeof(v), &v),
						 charset);
			}

			if (!to.null)
			{
				putField(tdbb, record,
						 DumpField(f_sec_map_to, VALUE_STRING, to->len, to->data),
						 charset);
			}

			buffer->store(record);
		}
		check("IResultSet::fetchNext", &st);

		curs->close(&st);
		check("IResultSet::close", &st);
		curs = NULL;

		tra->rollback(&st);
		check("ITransaction::rollback", &st);
		tra = NULL;

		att->detach(&st);
		check("IAttachment::detach", &st);
		att = NULL;
	}
	catch (const Exception&)
	{
		if (curs)
			curs->release();
		if (tra)
			tra->release();
		if (att)
			att->detach(&st);

		clearSnapshot();
		throw;
	}

	return getData(relation);
}

} // namespace Jrd
