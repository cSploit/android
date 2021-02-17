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
 *		Alex Peshkoff, 2010 - divided into class DataDump and the remaining part of DatabaseSnapshot
 */

#ifndef JRD_DATABASE_SNAPSHOT_H
#define JRD_DATABASE_SNAPSHOT_H

#include "../common/classes/array.h"
#include "../common/classes/init.h"
#include "../common/isc_s_proto.h"
#include "../common/classes/timestamp.h"
#include "../jrd/val.h"
#include "../jrd/recsrc/RecordSource.h"

namespace Jrd {

// forward declarations
class jrd_rel;
class Record;
class RecordBuffer;
class RuntimeStatistics;

class DataDump
{
public:
	struct RelationData
	{
		int rel_id;
		RecordBuffer* data;
	};
	typedef Firebird::Array<RelationData> Snapshot;

	enum ValueType {VALUE_GLOBAL_ID, VALUE_INTEGER, VALUE_TIMESTAMP, VALUE_STRING, VALUE_BOOLEAN};

	explicit DataDump(MemoryPool& pool)
		: idMap(pool), snapshot(pool), idCounter(0) { }
	~DataDump()
	{
		clearSnapshot();
	}

	struct DumpField
	{
		DumpField(USHORT p_id, ValueType p_type, USHORT p_length, const void* p_data)
			: id(p_id), type(p_type), length(p_length), data(p_data) { }
		DumpField()
			: id(0), type(VALUE_GLOBAL_ID), length(0), data(NULL) { }

		USHORT id;
		ValueType type;
		USHORT length;
		const void* data;
	};

	class DumpRecord
	{
	public:
		explicit DumpRecord(MemoryPool& pool)
			: buffer(pool), offset(0)
		{}

		DumpRecord(MemoryPool& pool, int rel_id)
			: buffer(pool), offset(1)
		{
			buffer.add(rel_id);
		}

		void reset(int rel_id)
		{
			offset = 1;
			buffer.clear();
			buffer.add(rel_id);
		}

		void assign(ULONG length, const UCHAR* ptr)
		{
			offset = 0;
			buffer.assign(ptr, length);
		}

		ULONG getLength() const
		{
			return offset;
		}

		const UCHAR* getData() const
		{
			return buffer.begin();
		}

		void storeGlobalId(int field_id, SINT64 value)
		{
			storeField(field_id, VALUE_GLOBAL_ID, sizeof(SINT64), &value);
		}

		void storeInteger(int field_id, SINT64 value)
		{
			storeField(field_id, VALUE_INTEGER, sizeof(SINT64), &value);
		}

		void storeTimestamp(int field_id, const Firebird::TimeStamp& value)
		{
			if (!value.isEmpty())
				storeField(field_id, VALUE_TIMESTAMP, sizeof(ISC_TIMESTAMP), &value.value());
		}

		void storeString(int field_id, const Firebird::string& value)
		{
			if (value.length())
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
		}

		void storeString(int field_id, const Firebird::PathName& value)
		{
			if (value.length())
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
		}

		void storeString(int field_id, const Firebird::MetaName& value)
		{
			if (value.length())
				storeField(field_id, VALUE_STRING, value.length(), value.c_str());
		}

		int getRelationId()
		{
			fb_assert(!offset);
			return (ULONG) buffer[offset++];
		}

		bool getField(DumpField& field)
		{
			fb_assert(offset);

			if (offset < buffer.getCount())
			{
				field.id = (USHORT) buffer[offset++];
				field.type = (ValueType) buffer[offset++];
				fb_assert(field.type >= VALUE_GLOBAL_ID && field.type <= VALUE_STRING);
				memcpy(&field.length, &buffer[offset], sizeof(USHORT));
				offset += sizeof(USHORT);
				field.data = &buffer[offset];
				offset += field.length;
				return true;
			}

			return false;
		}

	private:
		void storeField(int field_id, ValueType type, size_t length, const void* value)
		{
			const size_t delta = sizeof(UCHAR) + sizeof(UCHAR) + sizeof(USHORT) + length;
			buffer.resize(offset + delta);

			UCHAR* ptr = buffer.begin() + offset;
			fb_assert(field_id <= int(MAX_UCHAR));
			*ptr++ = (UCHAR) field_id;
			*ptr++ = (UCHAR) type;
			const USHORT adjusted_length = (USHORT) length;
			memcpy(ptr, &adjusted_length, sizeof(adjusted_length));
			ptr += sizeof(USHORT);
			memcpy(ptr, value, length);
			offset += (ULONG) delta;
		}

		Firebird::HalfStaticArray<UCHAR, 1024> buffer;
		ULONG offset;
	};

	void putField(thread_db*, Record*, const DumpField&, int);

	RecordBuffer* allocBuffer(thread_db*, MemoryPool&, int);
	RecordBuffer* getData(const jrd_rel*) const;
	RecordBuffer* getData(int) const;
	void clearSnapshot();

private:
	Firebird::GenericMap<Firebird::Pair<Firebird::NonPooled<SINT64, SLONG> > > idMap;
	Snapshot snapshot;
	int idCounter;
};


class MonitoringHeader : public Firebird::MemoryHeader
{
public:
	ULONG used;
	ULONG allocated;
};

class MonitoringData FB_FINAL : public Firebird::IpcObject
{
	static const ULONG MONITOR_VERSION = 3;
	static const ULONG DEFAULT_SIZE = 1048576;

	typedef MonitoringHeader Header;

	struct Element
	{
		SLONG processId;
		SLONG localId;
		ULONG length;
	};

	static ULONG alignOffset(ULONG absoluteOffset);

public:
	class Guard
	{
	public:
		explicit Guard(MonitoringData* ptr)
			: data(ptr)
		{
			data->acquire();
		}

		~Guard()
		{
			data->release();
		}

	private:
		Guard(const Guard&);
		Guard& operator=(const Guard&);

		MonitoringData* const data;
	};

	explicit MonitoringData(const Database*);
	~MonitoringData();

	bool initialize(Firebird::SharedMemoryBase*, bool);
	void mutexBug(int osErrorCode, const char* text);

	void acquire();
	void release();

	UCHAR* read(MemoryPool&, ULONG&);
	ULONG setup();
	void write(ULONG, ULONG, const void*);

	void cleanup();

private:
	// copying is prohibited
	MonitoringData(const MonitoringData&);
	MonitoringData& operator =(const MonitoringData&);

	void ensureSpace(ULONG);

	Firebird::AutoPtr<Firebird::SharedMemory<MonitoringHeader> > shared_memory;
	const SLONG process_id;
	const SLONG local_id;
};


class MonitoringTableScan: public VirtualTableScan
{
public:
	MonitoringTableScan(CompilerScratch* csb, const Firebird::string& name, StreamType stream)
		: VirtualTableScan(csb, name, stream)
	{}

protected:
	const Format* getFormat(thread_db* tdbb, jrd_rel* relation) const;
	bool retrieveRecord(thread_db* tdbb, jrd_rel* relation, FB_UINT64 position, Record* record) const;
};


class DatabaseSnapshot : public DataDump
{
private:
	class Writer
	{
	public:
		explicit Writer(MonitoringData* data)
			: dump(data)
		{
			fb_assert(dump);
			offset = dump->setup();
			fb_assert(offset);
		}

		void putRecord(const DumpRecord& record)
		{
			const ULONG length = record.getLength();
			dump->write(offset, sizeof(ULONG), &length);
			dump->write(offset, length, record.getData());
		}

	private:
		MonitoringData* dump;
		ULONG offset;
	};

	class Reader
	{
	public:
		Reader(ULONG length, UCHAR* ptr)
			: sizeLimit(length), buffer(ptr), offset(0)
		{}

		bool getRecord(DumpRecord& record)
		{
			if (offset < sizeLimit)
			{
				ULONG length;
				memcpy(&length, buffer + offset, sizeof(ULONG));
				offset += sizeof(ULONG);
				record.assign(length, buffer + offset);
				offset += length;
				return true;
			}

			return false;
		}

	private:
		ULONG sizeLimit;
		const UCHAR* buffer;
		ULONG offset;
	};

public:
	static DatabaseSnapshot* create(thread_db*);
	static int blockingAst(void*);
	static void initialize(thread_db*);
	static void shutdown(thread_db*);
	static void activate(thread_db*);
	static bool getRecord(thread_db* tdbb, jrd_rel* relation, FB_UINT64 position, Record* record);

protected:
	DatabaseSnapshot(thread_db*, MemoryPool&);

private:
	static void dumpData(Database*, int);
	static void dumpAttachment(DumpRecord&, const Attachment*, Writer&);

	static SINT64 getGlobalId(int);

	static void putDatabase(DumpRecord&, const Database*, Writer&, int, int);
	static void putAttachment(DumpRecord&, const Attachment*, Writer&, int);
	static void putTransaction(DumpRecord&, const jrd_tra*, Writer&, int);
	static void putRequest(DumpRecord&, const jrd_req*, Writer&, int);
	static void putCall(DumpRecord&, const jrd_req*, Writer&, int);
	static void putStatistics(DumpRecord&, const RuntimeStatistics&, Writer&, int, int);
	static void putContextVars(DumpRecord&, const Firebird::StringMap&, Writer&, int, bool);
	static void putMemoryUsage(DumpRecord&, const Firebird::MemoryStats&, Writer&, int, int);
};

} // namespace

#endif // JRD_DATABASE_SNAPSHOT_H
