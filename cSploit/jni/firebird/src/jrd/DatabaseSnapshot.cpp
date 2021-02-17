/*
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
 *  The Original Code was created by Dmitry Yemanov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2006 Dmitry Yemanov <dimitr@users.sf.net>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 */

#include "firebird.h"
#include "ids.h"

#include "../common/classes/auto.h"
#include "../common/classes/locks.h"
#include "../common/classes/fb_string.h"

#include "../common/gdsassert.h"
#include "../jrd/jrd.h"
#include "../jrd/cch.h"
#include "../jrd/ini.h"
#include "../jrd/nbak.h"
#include "../common/os/guid.h"
#include "../jrd/os/pio.h"
#include "../jrd/req.h"
#include "../jrd/tra.h"
#include "../jrd/blb_proto.h"
#include "../common/isc_proto.h"
#include "../common/isc_f_proto.h"
#include "../common/isc_s_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/CryptoManager.h"

#include "../jrd/Relation.h"
#include "../jrd/RecordBuffer.h"
#include "../jrd/DatabaseSnapshot.h"
#include "../jrd/Function.h"

#include "../common/utils_proto.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef UNIX
#include <signal.h>
#endif

#ifdef WIN_NT
#include <process.h>
#include <windows.h>
#endif


using namespace Firebird;
using namespace Jrd;


const Format* MonitoringTableScan::getFormat(thread_db* tdbb, jrd_rel* relation) const
{
	DatabaseSnapshot* const snapshot = DatabaseSnapshot::create(tdbb);
	return snapshot->getData(relation)->getFormat();
}


bool MonitoringTableScan::retrieveRecord(thread_db* tdbb, jrd_rel* relation,
										 FB_UINT64 position, Record* record) const
{
	DatabaseSnapshot* const snapshot = DatabaseSnapshot::create(tdbb);
	return snapshot->getData(relation)->fetch(position, record);
}


// MonitoringData class

MonitoringData::MonitoringData(const Database* dbb)
	: process_id(getpid()), local_id(dbb->dbb_monitoring_id)
{
	string name;
	name.printf(MONITOR_FILE, dbb->getUniqueFileId().c_str());

	Arg::StatusVector statusVector;
	try
	{
		shared_memory.reset(FB_NEW(*dbb->dbb_permanent) SharedMemory<MonitoringHeader>(
			name.c_str(), DEFAULT_SIZE, this));
	}
	catch (const Exception& ex)
	{
		iscLogException("MonitoringData: Cannot initialize the shared memory region", ex);
		throw;
	}

	fb_assert(shared_memory->getHeader()->mhb_version == MONITOR_VERSION);
}


MonitoringData::~MonitoringData()
{
	Guard guard(this);
	cleanup();

	if (shared_memory->getHeader()->used == sizeof(Header))
		shared_memory->removeMapFile();
}


void MonitoringData::acquire()
{
	shared_memory->mutexLock();

	if (shared_memory->getHeader()->allocated > shared_memory->sh_mem_length_mapped)
	{
#ifdef HAVE_OBJECT_MAP
		Arg::StatusVector statusVector;
		shared_memory->remapFile(statusVector, shared_memory->getHeader()->allocated, false);
		if (!shared_memory->remapFile(statusVector, shared_memory->getHeader()->allocated, false))
		{
			status_exception::raise(statusVector);
		}
#else
		status_exception::raise(Arg::Gds(isc_montabexh));
#endif
	}
}


void MonitoringData::release()
{
	shared_memory->mutexUnlock();
}


UCHAR* MonitoringData::read(MemoryPool& pool, ULONG& resultSize)
{
	ULONG self_dbb_offset = 0;

	// Garbage collect elements belonging to dead processes.
	// This is done in two passes. First, we compact the data
	// and calculate the total size of the resulting data.
	// Second, we create a resulting buffer of the necessary size
	// and copy the data there, starting with our own dbb.

	// First pass
	for (ULONG offset = alignOffset(sizeof(Header)); offset < shared_memory->getHeader()->used;)
	{
		UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + offset;
		const Element* const element = (Element*) ptr;
		const ULONG length = alignOffset(sizeof(Element) + element->length);

		if (element->processId == process_id && element->localId == local_id)
		{
			self_dbb_offset = offset;
		}

		if (ISC_check_process_existence(element->processId))
		{
			resultSize += element->length;
			offset += length;
		}
		else
		{
			fb_assert(shared_memory->getHeader()->used >= offset + length);
			memmove(ptr, ptr + length, shared_memory->getHeader()->used - offset - length);
			shared_memory->getHeader()->used -= length;
		}
	}

	// Second pass
	UCHAR* const buffer = FB_NEW(pool) UCHAR[resultSize];
	UCHAR* bufferPtr(buffer);

	fb_assert(self_dbb_offset);

	UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + self_dbb_offset;
	const Element* const element = (Element*) ptr;
	memcpy(bufferPtr, ptr + sizeof(Element), element->length);
	bufferPtr += element->length;

	for (ULONG offset = alignOffset(sizeof(Header)); offset < shared_memory->getHeader()->used;)
	{
		UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + offset;
		const Element* const element = (Element*) ptr;
		const ULONG length = alignOffset(sizeof(Element) + element->length);

		if (offset != self_dbb_offset)
		{
			memcpy(bufferPtr, ptr + sizeof(Element), element->length);
			bufferPtr += element->length;
		}

		offset += length;
	}

	fb_assert(buffer + resultSize == bufferPtr);
	return buffer;
}


ULONG MonitoringData::setup()
{
	ensureSpace(sizeof(Element));

	// Put an up-to-date element at the tail
	const ULONG offset = shared_memory->getHeader()->used;
	UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + offset;
	Element* const element = (Element*) ptr;
	element->processId = process_id;
	element->localId = local_id;
	element->length = 0;
	shared_memory->getHeader()->used += alignOffset(sizeof(Element));
	return offset;
}


void MonitoringData::write(ULONG offset, ULONG length, const void* buffer)
{
	ensureSpace(length);

	// Put an up-to-date element at the tail
	UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + offset;
	Element* const element = (Element*) ptr;
	memcpy(ptr + sizeof(Element) + element->length, buffer, length);
	ULONG previous = alignOffset(sizeof(Element) + element->length);
	element->length += length;
	ULONG current = alignOffset(sizeof(Element) + element->length);
	shared_memory->getHeader()->used += (current - previous);
}


void MonitoringData::cleanup()
{
	// Remove information about our dbb
	for (ULONG offset = alignOffset(sizeof(Header)); offset < shared_memory->getHeader()->used;)
	{
		UCHAR* const ptr = (UCHAR*) shared_memory->getHeader() + offset;
		const Element* const element = (Element*) ptr;
		const ULONG length = alignOffset(sizeof(Element) + element->length);

		if (element->processId == process_id && element->localId == local_id)
		{
			fb_assert(shared_memory->getHeader()->used >= offset + length);
			memmove(ptr, ptr + length, shared_memory->getHeader()->used - offset - length);
			shared_memory->getHeader()->used -= length;
		}
		else
		{
			offset += length;
		}
	}
}


void MonitoringData::ensureSpace(ULONG length)
{
	ULONG newSize = shared_memory->getHeader()->used + length;

	if (newSize > shared_memory->getHeader()->allocated)
	{
		newSize = FB_ALIGN(newSize, DEFAULT_SIZE);

#ifdef HAVE_OBJECT_MAP
		Arg::StatusVector statusVector;
		if (!shared_memory->remapFile(statusVector, newSize, true))
		{
			status_exception::raise(statusVector);
		}
		shared_memory->getHeader()->allocated = shared_memory->sh_mem_length_mapped;
#else
		status_exception::raise(Arg::Gds(isc_montabexh));
#endif
	}
}


void MonitoringData::mutexBug(int osErrorCode, const char* s)
{
	string msg;
	msg.printf("MONITOR: mutex %s error, status = %d", s, osErrorCode);
	fb_utils::logAndDie(msg.c_str());
}


bool MonitoringData::initialize(SharedMemoryBase* sm, bool initialize)
{
	if (initialize)
	{
		MonitoringHeader* header = reinterpret_cast<MonitoringHeader*>(sm->sh_mem_header);

		// Initialize the shared data header
		header->mhb_type = SharedMemoryBase::SRAM_DATABASE_SNAPSHOT;
		header->mhb_version = MONITOR_VERSION;
		header->mhb_timestamp = TimeStamp::getCurrentTimeStamp().value();

		header->used = alignOffset(sizeof(Header));
		header->allocated = sm->sh_mem_length_mapped;
	}

	return true;
}


ULONG MonitoringData::alignOffset(ULONG unaligned)
{
	return (ULONG) Firebird::MEM_ALIGN(unaligned);
}


// DatabaseSnapshot class


DatabaseSnapshot* DatabaseSnapshot::create(thread_db* tdbb)
{
	SET_TDBB(tdbb);

	jrd_tra* transaction = tdbb->getTransaction();
	fb_assert(transaction);

	if (!transaction->tra_db_snapshot)
	{
		// Create a database snapshot and store it
		// in the transaction block
		MemoryPool& pool = *transaction->tra_pool;
		transaction->tra_db_snapshot = FB_NEW(pool) DatabaseSnapshot(tdbb, pool);
	}

	return transaction->tra_db_snapshot;
}


int DatabaseSnapshot::blockingAst(void* ast_object)
{
	Database* dbb = static_cast<Database*>(ast_object);

	try
	{
		Lock* const lock = dbb->dbb_monitor_lock;

		if (!(dbb->dbb_ast_flags & DBB_monitor_off))
		{
			SyncLockGuard monGuard(&dbb->dbb_mon_sync, SYNC_EXCLUSIVE, FB_FUNCTION);

			if (!(dbb->dbb_ast_flags & DBB_monitor_off))
			{
				AsyncContextHolder tdbb(dbb, FB_FUNCTION);

				// Write the data to the shared memory
				try
				{
					dumpData(dbb, backup_state_unknown);
				}
				catch (const Exception& ex)
				{
					iscLogException("Cannot dump the monitoring data", ex);
				}

				// Release the lock and mark dbb as requesting a new one
				LCK_release(tdbb, lock);
				dbb->dbb_ast_flags |= DBB_monitor_off;
			}
		}
	}
	catch (const Exception&)
	{} // no-op

	return 0;
}


void DatabaseSnapshot::initialize(thread_db* tdbb)
{
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);

	if (!dbb->dbb_monitor_lock)
	{
		dbb->dbb_monitor_lock = FB_NEW_RPT(*dbb->dbb_permanent, 0)
				Lock(tdbb, 0, LCK_monitor, dbb, DatabaseSnapshot::blockingAst);
		LCK_lock(tdbb, dbb->dbb_monitor_lock, LCK_SR, LCK_WAIT);
	}
}


void DatabaseSnapshot::shutdown(thread_db* tdbb)
{
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);

	if (dbb->dbb_monitor_lock)
		LCK_release(tdbb, dbb->dbb_monitor_lock);
}


void DatabaseSnapshot::activate(thread_db* tdbb)
{
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);
	Jrd::Attachment* const attachment = tdbb->getAttachment();
	fb_assert(attachment);

	if (dbb->dbb_ast_flags & DBB_monitor_off)
	{
		// Enable signal handler for the monitoring stuff

		Attachment::CheckoutSyncGuard monGuard(attachment, dbb->dbb_mon_sync,
											   SYNC_EXCLUSIVE, FB_FUNCTION);

		if (dbb->dbb_ast_flags & DBB_monitor_off)
		{
			dbb->dbb_ast_flags &= ~DBB_monitor_off;
			LCK_lock(tdbb, dbb->dbb_monitor_lock, LCK_SR, LCK_WAIT);

			fb_assert(!(dbb->dbb_ast_flags & DBB_monitor_off));

			// While waiting for return from LCK_lock call above, the blocking AST (see
			// DatabaseSnapshot::blockingAst) was called and set DBB_monitor_off flag
			// again. But it do not released lock as lck_id was unknown at that moment.
			// Do it now to not block another process waiting for a monitoring lock.

			if (dbb->dbb_ast_flags & DBB_monitor_off)
				LCK_release(tdbb, dbb->dbb_monitor_lock);
		}
	}
}


DatabaseSnapshot::DatabaseSnapshot(thread_db* tdbb, MemoryPool& pool)
	: DataDump(pool)
{
	SET_TDBB(tdbb);

	PAG_header(tdbb, true);

	Database* const dbb = tdbb->getDatabase();
	fb_assert(dbb);

	Attachment* const attachment = tdbb->getAttachment();
	fb_assert(attachment);

	// Initialize record buffers
	RecordBuffer* const dbb_buffer = allocBuffer(tdbb, pool, rel_mon_database);
	RecordBuffer* const att_buffer = allocBuffer(tdbb, pool, rel_mon_attachments);
	RecordBuffer* const tra_buffer = allocBuffer(tdbb, pool, rel_mon_transactions);
	RecordBuffer* const stmt_buffer = allocBuffer(tdbb, pool, rel_mon_statements);
	RecordBuffer* const call_buffer = allocBuffer(tdbb, pool, rel_mon_calls);
	RecordBuffer* const io_stat_buffer = allocBuffer(tdbb, pool, rel_mon_io_stats);
	RecordBuffer* const rec_stat_buffer = allocBuffer(tdbb, pool, rel_mon_rec_stats);
	RecordBuffer* const ctx_var_buffer = allocBuffer(tdbb, pool, rel_mon_ctx_vars);
	RecordBuffer* const mem_usage_buffer = allocBuffer(tdbb, pool, rel_mon_mem_usage);

	// Determine the backup state
	int backup_state = backup_state_unknown;

	BackupManager* const bm = dbb->dbb_backup_manager;
	if (bm && !bm->isShutDown())
	{
		BackupManager::StateReadGuard holder(tdbb);

		switch (bm->getState())
		{
		case nbak_state_normal:
			backup_state = backup_state_normal;
			break;
		case nbak_state_stalled:
			backup_state = backup_state_stalled;
			break;
		case nbak_state_merge:
			backup_state = backup_state_merge;
			break;
		}
	}

	{ // scope
		Attachment::Checkout cout(attachment, FB_FUNCTION);
		SyncLockGuard monGuard(&dbb->dbb_mon_sync, SYNC_EXCLUSIVE, FB_FUNCTION);

		// Release our own lock
		LCK_release(tdbb, dbb->dbb_monitor_lock);
		dbb->dbb_ast_flags &= ~DBB_monitor_off;

		// Dump our own data
		dumpData(dbb, backup_state);
	}

	// Signal other processes to dump their data
	Lock temp_lock(tdbb, 0, LCK_monitor), *lock = &temp_lock;

	if (LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT))
		LCK_release(tdbb, lock);

	{ // scope
		Attachment::CheckoutSyncGuard monGuard(attachment, dbb->dbb_mon_sync,
											   SYNC_EXCLUSIVE, FB_FUNCTION);
		// Mark dbb as requesting a new lock
		dbb->dbb_ast_flags |= DBB_monitor_off;
	}

	// Read the shared memory
	ULONG dataSize = 0;
	UCHAR* dataPtr = NULL;

	{ // scope
		fb_assert(dbb->dbb_monitoring_data);
		MonitoringData::Guard guard(dbb->dbb_monitoring_data);
		dataPtr = dbb->dbb_monitoring_data->read(pool, dataSize);
	}

	fb_assert(dataSize && dataPtr);
	AutoPtr<UCHAR, ArrayDelete<UCHAR> > data(dataPtr);

	Reader reader(dataSize, data);

	string databaseName(dbb->dbb_database_name.c_str());
	ISC_systemToUtf8(databaseName);

	const string& userName = attachment->att_user->usr_user_name;
	const bool locksmith = attachment->locksmith();

	// Parse the dump
	RecordBuffer* buffer = NULL;
	Record* record = NULL;

	bool dbb_processed = false, fields_processed = false;
	bool dbb_allowed = false, att_allowed = false;

	DumpRecord dumpRecord(*tdbb->getDefaultPool());
	while (reader.getRecord(dumpRecord))
	{
		const int rid = dumpRecord.getRelationId();

		switch (rid)
		{
		case rel_mon_database:
			buffer = dbb_buffer;
			break;
		case rel_mon_attachments:
			buffer = att_buffer;
			break;
		case rel_mon_transactions:
			buffer = tra_buffer;
			break;
		case rel_mon_statements:
			buffer = stmt_buffer;
			break;
		case rel_mon_calls:
			buffer = call_buffer;
			break;
		case rel_mon_io_stats:
			buffer = io_stat_buffer;
			break;
		case rel_mon_rec_stats:
			buffer = rec_stat_buffer;
			break;
		case rel_mon_ctx_vars:
			buffer = ctx_var_buffer;
			break;
		case rel_mon_mem_usage:
			buffer = mem_usage_buffer;
			break;
		default:
			fb_assert(false);
		}

		if (buffer)
		{
			record = buffer->getTempRecord();
			record->nullify();
		}
		else
		{
			record = NULL;
		}

		DumpField dumpField;
		while (dumpRecord.getField(dumpField))
		{
			const USHORT fid = dumpField.id;
			const size_t length = dumpField.length;
			const char* source = (const char*) dumpField.data;

			// All the strings that may require transliteration (i.e. the target charset is not NONE)
			// are known to be in the metadata charset or ASCII (which is binary compatible).
			const int charset = ttype_metadata;

			// special case for MON$DATABASE
			if (rid == rel_mon_database)
			{
				if (fid == f_mon_db_name)
					dbb_allowed = !databaseName.compare(source, length);

				if (record && dbb_allowed && !dbb_processed)
				{
					putField(tdbb, record, dumpField, charset);
					fields_processed = true;
				}

				att_allowed = (dbb_allowed && !dbb_processed);
			}
			// special case for MON$ATTACHMENTS
			else if (rid == rel_mon_attachments)
			{
				if (fid == f_mon_att_user)
					att_allowed = locksmith || !userName.compare(source, length);

				if (record && dbb_allowed && att_allowed)
				{
					putField(tdbb, record, dumpField, charset);
					fields_processed = true;
					dbb_processed = true;
				}
			}
			// generic logic that covers all other relations
			else if (record && dbb_allowed && att_allowed)
			{
				putField(tdbb, record, dumpField, charset);
				fields_processed = true;
				dbb_processed = true;
			}
		}

		if (fields_processed)
		{
			buffer->store(record);
			fields_processed = false;
		}
	}
}


void DataDump::clearSnapshot()
{
	for (size_t i = 0; i < snapshot.getCount(); i++)
		delete snapshot[i].data;
	snapshot.clear();
}


RecordBuffer* DataDump::getData(const jrd_rel* relation) const
{
	fb_assert(relation);

	return getData(relation->rel_id);
}


RecordBuffer* DataDump::getData(int id) const
{
	for (size_t i = 0; i < snapshot.getCount(); i++)
	{
		if (snapshot[i].rel_id == id)
			return snapshot[i].data;
	}

	return NULL;
}


RecordBuffer* DataDump::allocBuffer(thread_db* tdbb, MemoryPool& pool, int rel_id)
{
	jrd_rel* const relation = MET_lookup_relation_id(tdbb, rel_id, false);
	fb_assert(relation);
	MET_scan_relation(tdbb, relation);
	fb_assert(relation->isVirtual());

	const Format* const format = MET_current(tdbb, relation);
	fb_assert(format);

	RecordBuffer* const buffer = FB_NEW(pool) RecordBuffer(pool, format);
	RelationData data = {relation->rel_id, buffer};
	snapshot.add(data);

	return buffer;
}


void DataDump::putField(thread_db* tdbb, Record* record, const DumpField& field, int charset)
{
	fb_assert(record);

	const Format* const format = record->rec_format;
	fb_assert(format);

	dsc to_desc;

	if (field.id < format->fmt_count)
		to_desc = format->fmt_desc[field.id];

	if (to_desc.isUnknown())
		return;

	to_desc.dsc_address += (IPTR) record->rec_data;

	if (field.type == VALUE_GLOBAL_ID)
	{
		// special case: translate 64-bit global ID into 32-bit local ID
		fb_assert(field.length == sizeof(SINT64));
		SINT64 global_id;
		memcpy(&global_id, field.data, field.length);
		SLONG local_id;
		if (!idMap.get(global_id, local_id))
		{
			local_id = ++idCounter;
			idMap.put(global_id, local_id);
		}
		dsc from_desc;
		from_desc.makeLong(0, &local_id);
		MOV_move(tdbb, &from_desc, &to_desc);
	}
	else if (field.type == VALUE_INTEGER)
	{
		fb_assert(field.length == sizeof(SINT64));
		SINT64 value;
		memcpy(&value, field.data, field.length);
		dsc from_desc;
		from_desc.makeInt64(0, &value);
		MOV_move(tdbb, &from_desc, &to_desc);
	}
	else if (field.type == VALUE_TIMESTAMP)
	{
		fb_assert(field.length == sizeof(ISC_TIMESTAMP));
		ISC_TIMESTAMP value;
		memcpy(&value, field.data, field.length);
		dsc from_desc;
		from_desc.makeTimestamp(&value);
		MOV_move(tdbb, &from_desc, &to_desc);
	}
	else if (field.type == VALUE_STRING)
	{
		dsc from_desc;
		MoveBuffer buffer;

		if (charset == CS_NONE && to_desc.getCharSet() == CS_METADATA)
		{
			// ASF: If an attachment using NONE charset has a string using non-ASCII characters,
			// nobody will be able to select them in a system field. So we change these characters to
			// question marks here - CORE-2602.

			UCHAR* p = buffer.getBuffer(field.length);
			const UCHAR* s = (const UCHAR*) field.data;

			for (const UCHAR* end = buffer.end(); p < end; ++p, ++s)
				*p = (*s > 0x7F ? '?' : *s);

			from_desc.makeText(field.length, CS_ASCII, buffer.begin());
		}
		else
			from_desc.makeText(field.length, charset, (UCHAR*) field.data);

		MOV_move(tdbb, &from_desc, &to_desc);
	}
	else if (field.type == VALUE_BOOLEAN)
	{
		fb_assert(field.length == sizeof(UCHAR));
		UCHAR value;
		memcpy(&value, field.data, field.length);
		dsc from_desc;
		from_desc.makeBoolean(&value);
		MOV_move(tdbb, &from_desc, &to_desc);
	}
	else
	{
		fb_assert(false);
	}

	// hvlad: detach just created temporary blob from request to bound its
	// lifetime to transaction. This is necessary as this blob belongs to
	// the MON$ table and must be accessible until transaction ends.
	if (to_desc.isBlob())
	{
		bid* blob_id = reinterpret_cast<bid*>(to_desc.dsc_address);
		jrd_tra* tran = tdbb->getTransaction();

		if (!tran->tra_blobs->locate(blob_id->bid_temp_id()))
			fb_assert(false);

		BlobIndex& blobIdx = tran->tra_blobs->current();
		fb_assert(!blobIdx.bli_materialized);

		if (blobIdx.bli_request)
		{
			if (!blobIdx.bli_request->req_blobs.locate(blobIdx.bli_temp_id))
				fb_assert(false);

			blobIdx.bli_request->req_blobs.fastRemove();
			blobIdx.bli_request = NULL;
		}
	}

	record->clearNull(field.id);
}


void DatabaseSnapshot::dumpData(Database* dbb, int backup_state)
{
	MemoryPool& pool = *dbb->dbb_permanent;

	if (!dbb->dbb_monitoring_data)
		dbb->dbb_monitoring_data = FB_NEW(pool) MonitoringData(dbb);

	MonitoringData::Guard guard(dbb->dbb_monitoring_data);
	dbb->dbb_monitoring_data->cleanup();

	Writer writer(dbb->dbb_monitoring_data);
	DumpRecord record(pool);

	// Database information

	putDatabase(record, dbb, writer, fb_utils::genUniqueId(), backup_state);

	// Attachment information

	AttachmentsRefHolder attachments(pool);

	{ // scope
		SyncLockGuard attGuard(&dbb->dbb_sync, SYNC_SHARED, FB_FUNCTION);

		for (Attachment* attachment = dbb->dbb_attachments; attachment;
			 attachment = attachment->att_next)
		{
			attachments.add(attachment->att_interface);
		}
	}

	{ // scope
		SyncLockGuard sysAttGuard(&dbb->dbb_sys_attach, SYNC_SHARED, FB_FUNCTION);

		for (Attachment* attachment = dbb->dbb_sys_attachments; attachment;
			 attachment = attachment->att_next)
		{
			attachments.add(attachment->att_interface);
		}
	}

	for (AttachmentsRefHolder::Iterator iter(attachments); *iter; )
	{
		{ // scope
			JAttachment* const jAtt = *iter;
			MutexLockGuard guard(*jAtt->getMutex(), FB_FUNCTION);

			Attachment* const attachment = jAtt->getHandle();

			if (attachment && attachment->att_user)
				dumpAttachment(record, attachment, writer);
		}

		iter.remove();
	}
}

void DatabaseSnapshot::dumpAttachment(DumpRecord& record, const Attachment* attachment,
									  Writer& writer)
{
	putAttachment(record, attachment, writer, fb_utils::genUniqueId());

	jrd_tra* transaction = NULL;
	jrd_req* request = NULL;

	// Transaction information

	for (transaction = attachment->att_transactions; transaction;
		 transaction = transaction->tra_next)
	{
		putTransaction(record, transaction, writer, fb_utils::genUniqueId());
	}

	// Call stack information

	for (transaction = attachment->att_transactions; transaction;
		 transaction = transaction->tra_next)
	{
		for (request = transaction->tra_requests;
			 request && (request->req_flags & req_active);
			 request = request->req_caller)
		{
			request->adjustCallerStats();

			if (!(request->getStatement()->flags &
					(JrdStatement::FLAG_INTERNAL | JrdStatement::FLAG_SYS_TRIGGER)) &&
				request->req_caller)
			{
				putCall(record, request, writer, fb_utils::genUniqueId());
			}
		}
	}

	// Request information

	for (const jrd_req* const* i = attachment->att_requests.begin();
		 i != attachment->att_requests.end();
		 ++i)
	{
		const jrd_req* request = *i;

		if (!(request->getStatement()->flags &
				(JrdStatement::FLAG_INTERNAL | JrdStatement::FLAG_SYS_TRIGGER)))
		{
			putRequest(record, request, writer, fb_utils::genUniqueId());
		}
	}
}


SINT64 DatabaseSnapshot::getGlobalId(int value)
{
	return ((SINT64) getpid() << BITS_PER_LONG) + value;
}


void DatabaseSnapshot::putDatabase(DumpRecord& record, const Database* database,
								   Writer& writer, int stat_id, int backup_state)
{
	fb_assert(database);

	record.reset(rel_mon_database);

	PathName databaseName(database->dbb_database_name);
	ISC_systemToUtf8(databaseName);

	// database name or alias (MUST BE ALWAYS THE FIRST ITEM PASSED!)
	record.storeString(f_mon_db_name, databaseName);
	// page size
	record.storeInteger(f_mon_db_page_size, database->dbb_page_size);
	// major ODS version
	record.storeInteger(f_mon_db_ods_major, database->dbb_ods_version);
	// minor ODS version
	record.storeInteger(f_mon_db_ods_minor, database->dbb_minor_version);
	// oldest interesting transaction
	record.storeInteger(f_mon_db_oit, database->dbb_oldest_transaction);
	// oldest active transaction
	record.storeInteger(f_mon_db_oat, database->dbb_oldest_active);
	// oldest snapshot transaction
	record.storeInteger(f_mon_db_ost, database->dbb_oldest_snapshot);
	// next transaction
	record.storeInteger(f_mon_db_nt, database->dbb_next_transaction);
	// number of page buffers
	record.storeInteger(f_mon_db_page_bufs, database->dbb_bcb->bcb_count);

	int temp;

	// SQL dialect
	temp = (database->dbb_flags & DBB_DB_SQL_dialect_3) ? 3 : 1;
	record.storeInteger(f_mon_db_dialect, temp);

	// shutdown mode
	if (database->dbb_ast_flags & DBB_shutdown_full)
		temp = shut_mode_full;
	else if (database->dbb_ast_flags & DBB_shutdown_single)
		temp = shut_mode_single;
	else if (database->dbb_ast_flags & DBB_shutdown)
		temp = shut_mode_multi;
	else
		temp = shut_mode_online;
	record.storeInteger(f_mon_db_shut_mode, temp);

	// sweep interval
	record.storeInteger(f_mon_db_sweep_int, database->dbb_sweep_interval);
	// read only flag
	temp = database->readOnly() ? 1 : 0;
	record.storeInteger(f_mon_db_read_only, temp);
	// forced writes flag
	temp = (database->dbb_flags & DBB_force_write) ? 1 : 0;
	record.storeInteger(f_mon_db_forced_writes, temp);
	// reserve space flag
	temp = (database->dbb_flags & DBB_no_reserve) ? 0 : 1;
	record.storeInteger(f_mon_db_res_space, temp);
	// creation date
	record.storeTimestamp(f_mon_db_created, database->dbb_creation_date);
	// database size
	record.storeInteger(f_mon_db_pages, PageSpace::actAlloc(database));
	// database backup state
	record.storeInteger(f_mon_db_backup_state, backup_state);

	// crypt thread status
	if (database->dbb_crypto_manager)
		record.storeInteger(f_mon_db_crypt_page, database->dbb_crypto_manager->getCurrentPage());

	// database owner
	record.storeString(f_mon_db_owner, database->dbb_owner);

	// statistics
	record.storeGlobalId(f_mon_db_stat_id, getGlobalId(stat_id));
	writer.putRecord(record);

	if (Config::getSharedCache())
	{
		putStatistics(record, database->dbb_stats, writer, stat_id, stat_database);
		putMemoryUsage(record, database->dbb_memory_stats, writer, stat_id, stat_database);
	}
	else
	{
		RuntimeStatistics zero_rt_stats;
		MemoryStats zero_mem_stats;
		putStatistics(record, zero_rt_stats, writer, stat_id, stat_database);
		putMemoryUsage(record, zero_mem_stats, writer, stat_id, stat_database);
	}
}


void DatabaseSnapshot::putAttachment(DumpRecord& record, const Jrd::Attachment* attachment,
									 Writer& writer, int stat_id)
{
	fb_assert(attachment && attachment->att_user);

	record.reset(rel_mon_attachments);

	int temp = mon_state_idle;

	for (const jrd_tra* transaction_itr = attachment->att_transactions;
		 transaction_itr; transaction_itr = transaction_itr->tra_next)
	{
		if (transaction_itr->tra_requests)
		{
			temp = mon_state_active;
			break;
		}
	}

	PathName attName(attachment->att_filename);
	ISC_systemToUtf8(attName);

	// user (MUST BE ALWAYS THE FIRST ITEM PASSED!)
	record.storeString(f_mon_att_user, attachment->att_user->usr_user_name);
	// attachment id
	record.storeInteger(f_mon_att_id, attachment->att_attachment_id);
	// process id
	record.storeInteger(f_mon_att_server_pid, getpid());
	// state
	record.storeInteger(f_mon_att_state, temp);
	// attachment name
	record.storeString(f_mon_att_name, attName);
	// role
	record.storeString(f_mon_att_role, attachment->att_user->usr_sql_role_name);
	// remote protocol
	record.storeString(f_mon_att_remote_proto, attachment->att_network_protocol);
	// remote address
	record.storeString(f_mon_att_remote_addr, attachment->att_remote_address);
	// remote process id
	if (attachment->att_remote_pid)
	{
		record.storeInteger(f_mon_att_remote_pid, attachment->att_remote_pid);
	}
	// remote process name
	record.storeString(f_mon_att_remote_process, attachment->att_remote_process);
	// charset
	record.storeInteger(f_mon_att_charset_id, attachment->att_charset);
	// timestamp
	record.storeTimestamp(f_mon_att_timestamp, attachment->att_timestamp);
	// garbage collection flag
	temp = (attachment->att_flags & ATT_no_cleanup) ? 0 : 1;
	record.storeInteger(f_mon_att_gc, temp);
	// client library version
	record.storeString(f_mon_att_client_version, attachment->att_client_version);
	// remote protocol version
	record.storeString(f_mon_att_remote_version, attachment->att_remote_protocol);
	// remote host name
	record.storeString(f_mon_att_remote_host, attachment->att_remote_host);
	// OS user name
	record.storeString(f_mon_att_remote_os_user, attachment->att_remote_os_user);
	// authentication method
	record.storeString(f_mon_att_auth_method, attachment->att_user->usr_auth_method);
	// statistics
	record.storeGlobalId(f_mon_att_stat_id, getGlobalId(stat_id));
	// system flag
	temp = (attachment->att_flags & ATT_system) ? 1 : 0;
	record.storeInteger(f_mon_att_sys_flag, temp);

	writer.putRecord(record);

	if (Config::getSharedCache())
	{
		putStatistics(record, attachment->att_stats, writer, stat_id, stat_attachment);
		putMemoryUsage(record, attachment->att_memory_stats, writer, stat_id, stat_attachment);
	}
	else
	{
		putStatistics(record, attachment->att_database->dbb_stats, writer, stat_id, stat_attachment);
		putMemoryUsage(record, attachment->att_database->dbb_memory_stats, writer, stat_id, stat_attachment);
	}

	// context vars
	putContextVars(record, attachment->att_context_vars, writer, attachment->att_attachment_id, true);
}


void DatabaseSnapshot::putTransaction(DumpRecord& record, const jrd_tra* transaction,
									  Writer& writer, int stat_id)
{
	fb_assert(transaction);

	record.reset(rel_mon_transactions);

	int temp;

	// transaction id
	record.storeInteger(f_mon_tra_id, transaction->tra_number);
	// attachment id
	record.storeInteger(f_mon_tra_att_id, transaction->tra_attachment->att_attachment_id);
	// state
	temp = transaction->tra_requests ? mon_state_active : mon_state_idle;
	record.storeInteger(f_mon_tra_state, temp);
	// timestamp
	record.storeTimestamp(f_mon_tra_timestamp, transaction->tra_timestamp);
	// top transaction
	record.storeInteger(f_mon_tra_top, transaction->tra_top);
	// oldest transaction
	record.storeInteger(f_mon_tra_oit, transaction->tra_oldest);
	// oldest active transaction
	record.storeInteger(f_mon_tra_oat, transaction->tra_oldest_active);
	// isolation mode
	if (transaction->tra_flags & TRA_degree3)
	{
		temp = iso_mode_consistency;
	}
	else if (transaction->tra_flags & TRA_read_committed)
	{
		temp = (transaction->tra_flags & TRA_rec_version) ?
			iso_mode_rc_version : iso_mode_rc_no_version;
	}
	else
	{
		temp = iso_mode_concurrency;
	}
	record.storeInteger(f_mon_tra_iso_mode, temp);
	// lock timeout
	record.storeInteger(f_mon_tra_lock_timeout, transaction->tra_lock_timeout);
	// read only flag
	temp = (transaction->tra_flags & TRA_readonly) ? 1 : 0;
	record.storeInteger(f_mon_tra_read_only, temp);
	// autocommit flag
	temp = (transaction->tra_flags & TRA_autocommit) ? 1 : 0;
	record.storeInteger(f_mon_tra_auto_commit, temp);
	// auto undo flag
	temp = (transaction->tra_flags & TRA_no_auto_undo) ? 0 : 1;
	record.storeInteger(f_mon_tra_auto_undo, temp);

	// statistics
	record.storeGlobalId(f_mon_tra_stat_id, getGlobalId(stat_id));
	writer.putRecord(record);
	putStatistics(record, transaction->tra_stats, writer, stat_id, stat_transaction);
	putMemoryUsage(record, transaction->tra_memory_stats, writer, stat_id, stat_transaction);

	// context vars
	putContextVars(record, transaction->tra_context_vars, writer, transaction->tra_number, false);
}


void DatabaseSnapshot::putRequest(DumpRecord& record, const jrd_req* request,
								  Writer& writer, int stat_id)
{
	fb_assert(request);

	record.reset(rel_mon_statements);

	// request id
	record.storeInteger(f_mon_stmt_id, request->req_id);
	// attachment id
	if (request->req_attachment)
	{
		record.storeInteger(f_mon_stmt_att_id, request->req_attachment->att_attachment_id);
	}
	// state, transaction ID, timestamp
	if (request->req_flags & req_active)
	{
		const bool is_stalled = (request->req_flags & req_stall);
		record.storeInteger(f_mon_stmt_state, is_stalled ? mon_state_stalled : mon_state_active);
		if (request->req_transaction)
		{
			record.storeInteger(f_mon_stmt_tra_id, request->req_transaction->tra_number);
		}
		record.storeTimestamp(f_mon_stmt_timestamp, request->req_timestamp);
	}
	else
	{
		record.storeInteger(f_mon_stmt_state, mon_state_idle);
	}
	// sql text
	if (request->getStatement()->sqlText)
	{
		record.storeString(f_mon_stmt_sql_text, *request->getStatement()->sqlText);
	}

	// statistics
	record.storeGlobalId(f_mon_stmt_stat_id, getGlobalId(stat_id));
	writer.putRecord(record);
	putStatistics(record, request->req_stats, writer, stat_id, stat_statement);
	putMemoryUsage(record, request->req_memory_stats, writer, stat_id, stat_statement);
}


void DatabaseSnapshot::putCall(DumpRecord& record, const jrd_req* request,
							   Writer& writer, int stat_id)
{
	fb_assert(request);

	const jrd_req* initialRequest = request->req_caller;
	while (initialRequest->req_caller)
	{
		initialRequest = initialRequest->req_caller;
	}
	fb_assert(initialRequest);

	record.reset(rel_mon_calls);

	// call id
	record.storeInteger(f_mon_call_id, request->req_id);
	// statement id
	record.storeInteger(f_mon_call_stmt_id, initialRequest->req_id);
	// caller id
	if (initialRequest != request->req_caller)
	{
		record.storeInteger(f_mon_call_caller_id, request->req_caller->req_id);
	}

	const JrdStatement* statement = request->getStatement();
	const Routine* routine = statement->getRoutine();

	// object name/type
	if (routine)
	{
		if (routine->getName().package.hasData())
			record.storeString(f_mon_call_pkg_name, routine->getName().package);

		record.storeString(f_mon_call_name, routine->getName().identifier);
		record.storeInteger(f_mon_call_type, routine->getObjectType());
	}
	else if (!statement->triggerName.isEmpty())
	{
		record.storeString(f_mon_call_name, statement->triggerName);
		record.storeInteger(f_mon_call_type, obj_trigger);
	}
	else
	{
		// we should never be here...
		fb_assert(false);
	}

	// timestamp
	record.storeTimestamp(f_mon_call_timestamp, request->req_timestamp);
	// source line/column
	if (request->req_src_line)
	{
		record.storeInteger(f_mon_call_src_line, request->req_src_line);
		record.storeInteger(f_mon_call_src_column, request->req_src_column);
	}

	// statistics
	record.storeGlobalId(f_mon_call_stat_id, getGlobalId(stat_id));
	writer.putRecord(record);
	putStatistics(record, request->req_stats, writer, stat_id, stat_call);
	putMemoryUsage(record, request->req_memory_stats, writer, stat_id, stat_call);
}

void DatabaseSnapshot::putStatistics(DumpRecord& record, const RuntimeStatistics& statistics,
									 Writer& writer, int stat_id, int stat_group)
{
	// statistics id
	const SINT64 id = getGlobalId(stat_id);

	// physical I/O statistics
	record.reset(rel_mon_io_stats);
	record.storeGlobalId(f_mon_io_stat_id, id);
	record.storeInteger(f_mon_io_stat_group, stat_group);
	record.storeInteger(f_mon_io_page_reads, statistics.getValue(RuntimeStatistics::PAGE_READS));
	record.storeInteger(f_mon_io_page_writes, statistics.getValue(RuntimeStatistics::PAGE_WRITES));
	record.storeInteger(f_mon_io_page_fetches, statistics.getValue(RuntimeStatistics::PAGE_FETCHES));
	record.storeInteger(f_mon_io_page_marks, statistics.getValue(RuntimeStatistics::PAGE_MARKS));
	writer.putRecord(record);

	// logical I/O statistics
	record.reset(rel_mon_rec_stats);
	record.storeGlobalId(f_mon_rec_stat_id, id);
	record.storeInteger(f_mon_rec_stat_group, stat_group);
	record.storeInteger(f_mon_rec_seq_reads, statistics.getValue(RuntimeStatistics::RECORD_SEQ_READS));
	record.storeInteger(f_mon_rec_idx_reads, statistics.getValue(RuntimeStatistics::RECORD_IDX_READS));
	record.storeInteger(f_mon_rec_inserts, statistics.getValue(RuntimeStatistics::RECORD_INSERTS));
	record.storeInteger(f_mon_rec_updates, statistics.getValue(RuntimeStatistics::RECORD_UPDATES));
	record.storeInteger(f_mon_rec_deletes, statistics.getValue(RuntimeStatistics::RECORD_DELETES));
	record.storeInteger(f_mon_rec_backouts, statistics.getValue(RuntimeStatistics::RECORD_BACKOUTS));
	record.storeInteger(f_mon_rec_purges, statistics.getValue(RuntimeStatistics::RECORD_PURGES));
	record.storeInteger(f_mon_rec_expunges, statistics.getValue(RuntimeStatistics::RECORD_EXPUNGES));
	writer.putRecord(record);
}

void DatabaseSnapshot::putContextVars(DumpRecord& record, const StringMap& variables,
									  Writer& writer, int object_id, bool is_attachment)
{
	StringMap::ConstAccessor accessor(&variables);

	for (bool found = accessor.getFirst(); found; found = accessor.getNext())
	{
		record.reset(rel_mon_ctx_vars);

		if (is_attachment)
			record.storeInteger(f_mon_ctx_var_att_id, object_id);
		else
			record.storeInteger(f_mon_ctx_var_tra_id, object_id);

		record.storeString(f_mon_ctx_var_name, accessor.current()->first);
		record.storeString(f_mon_ctx_var_value, accessor.current()->second);

		writer.putRecord(record);
	}
}

void DatabaseSnapshot::putMemoryUsage(DumpRecord& record, const MemoryStats& stats,
									  Writer& writer, int stat_id, int stat_group)
{
	// statistics id
	const SINT64 id = getGlobalId(stat_id);

	// memory usage
	record.reset(rel_mon_mem_usage);
	record.storeGlobalId(f_mon_mem_stat_id, id);
	record.storeInteger(f_mon_mem_stat_group, stat_group);
	record.storeInteger(f_mon_mem_cur_used, stats.getCurrentUsage());
	record.storeInteger(f_mon_mem_cur_alloc, stats.getCurrentMapping());
	record.storeInteger(f_mon_mem_max_used, stats.getMaximumUsage());
	record.storeInteger(f_mon_mem_max_alloc, stats.getMaximumMapping());

	writer.putRecord(record);
}
