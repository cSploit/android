/*
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
 *
 */

#ifndef JRD_RELATION_H
#define JRD_RELATION_H

#include "../jrd/jrd.h"
#include "../jrd/pag.h"
#include "../jrd/val.h"

namespace Jrd
{

// view context block to cache view aliases

class ViewContext
{
public:
	explicit ViewContext(MemoryPool& p, const TEXT* context_name,
						 const TEXT* relation_name, USHORT context,
						 bool is_relation) :
		vcx_context_name(p, context_name, strlen(context_name)),
		vcx_relation_name(relation_name),
		vcx_context(context),
		vcx_is_relation(is_relation)
	{
	}

	static USHORT generate(const void*, const ViewContext* vc)
	{
		return vc->vcx_context;
	}

	const Firebird::string	vcx_context_name;
	const Firebird::MetaName	vcx_relation_name;
	const USHORT	vcx_context;
	const bool		vcx_is_relation;
};

typedef Firebird::SortedArray<ViewContext*, Firebird::EmptyStorage<ViewContext*>,
		USHORT, ViewContext> ViewContexts;

#ifdef GARBAGE_THREAD

class RelationGarbage
{
private:
	class TranGarbage
	{
	public:
		SLONG tran;
		PageBitmap *bm;

		TranGarbage(PageBitmap* aBm, SLONG aTran)
			: tran(aTran), bm(aBm)
		{}

		static inline const SLONG generate(void const*, const TranGarbage& Item)
		{
			return Item.tran;
		}
	};

	typedef	Firebird::SortedArray<
				TranGarbage,
				Firebird::EmptyStorage<TranGarbage>,
				SLONG,
				TranGarbage> TranGarbageArray;

	TranGarbageArray array;

public:
	explicit RelationGarbage(MemoryPool& p)
		: array(p)
	{}
	~RelationGarbage() { clear(); }

	void addPage(MemoryPool* pool, const SLONG pageno, const SLONG tranid);
	void clear();

	void getGarbage(const SLONG oldest_snapshot, PageBitmap **sbm);

	SLONG minTranID() const
	{
		return (array.getCount() > 0) ? array[0].tran : MAX_SLONG;
	}
};

#endif //GARBAGE_THREAD

class RelationPages
{
public:
	vcl*	rel_pages;			// vector of pointer page numbers
	SLONG	rel_instance_id;	// 0 or att_attachment_id or tra_number
	SLONG	rel_index_root;		// index root page number
	SLONG	rel_data_pages;		// count of relation data pages
	USHORT	rel_slot_space;		// lowest pointer page with slot space
	USHORT	rel_data_space;		// lowest pointer page with data page space
	USHORT	rel_pg_space_id;

	RelationPages()
	{
		rel_pages = 0;
		rel_index_root = rel_data_pages = rel_instance_id = 0;
		rel_slot_space = rel_data_space = 0;
		rel_pg_space_id = DB_PAGE_SPACE;
		rel_next_free = 0;
		useCount = 0;
	}

	inline SLONG addRef()
	{
		return useCount++;
	}

	void free(RelationPages*& nextFree);

	static inline SLONG generate(const void*, const RelationPages* item)
	{
		return item->rel_instance_id;
	}

private:
	RelationPages*	rel_next_free;
	SLONG	useCount;

friend class jrd_rel;
};


// Primary dependencies from all foreign references to relation's
// primary/unique keys

struct prim
{
	vec<int>* prim_reference_ids;
	vec<int>* prim_relations;
	vec<int>* prim_indexes;
};


// Foreign references to other relations' primary/unique keys

struct frgn
{
	vec<int>* frgn_reference_ids;
	vec<int>* frgn_relations;
	vec<int>* frgn_indexes;
};


// Relation block; one is created for each relation referenced
// in the database, though it is not really filled out until
// the relation is scanned

class jrd_rel : public pool_alloc<type_rel>
{
public:
	USHORT			rel_id;
	USHORT			rel_current_fmt;	// Current format number
	ULONG			rel_flags;
	Format*			rel_current_format;	// Current record format
	Firebird::MetaName	rel_name;		// ascii relation name
	vec<Format*>*	rel_formats;		// Known record formats
	Firebird::MetaName	rel_owner_name;	// ascii owner
	vec<jrd_fld*>*	rel_fields;			// vector of field blocks

	RecordSelExpr*	rel_view_rse;		// view record select expression
	ViewContexts	rel_view_contexts;	// sorted array of view contexts

	Firebird::MetaName	rel_security_name;	// security class name for relation
	ExternalFile* 	rel_file;			// external file name

	vec<Record*>*	rel_gc_rec;			// vector of records for garbage collection
#ifdef GARBAGE_THREAD
	PageBitmap*		rel_gc_bitmap;		// garbage collect bitmap of data page sequences
	RelationGarbage*	rel_garbage;	// deferred gc bitmap's by tran numbers
#endif

	USHORT		rel_use_count;		// requests compiled with relation
	USHORT		rel_sweep_count;	// sweep and/or garbage collector threads active
	SSHORT		rel_scan_count;		// concurrent sequential scan count

	Lock*		rel_existence_lock;	// existence lock, if any
	Lock*		rel_partners_lock;	// partners lock
	IndexLock*	rel_index_locks;	// index existence locks
	IndexBlock*	rel_index_blocks;	// index blocks for caching index info
	trig_vec*	rel_pre_erase; 		// Pre-operation erase trigger
	trig_vec*	rel_post_erase;		// Post-operation erase trigger
	trig_vec*	rel_pre_modify;		// Pre-operation modify trigger
	trig_vec*	rel_post_modify;	// Post-operation modify trigger
	trig_vec*	rel_pre_store;		// Pre-operation store trigger
	trig_vec*	rel_post_store;		// Post-operation store trigger
	prim		rel_primary_dpnds;	// foreign dependencies on this relation's primary key
	frgn		rel_foreign_refs;	// foreign references to other relations' primary keys

	Firebird::Mutex rel_drop_mutex;

	bool isSystem() const;
	bool isTemporary() const;
	bool isVirtual() const;
	bool isView() const;

	// global temporary relations attributes
	RelationPages* getPages(thread_db* tdbb, SLONG tran = -1, bool allocPages = true);

	RelationPages* getBasePages()
	{
		return &rel_pages_base;
	}

	bool			delPages(thread_db* tdbb, SLONG tran = -1, RelationPages* aPages = 0);

	void			getRelLockKey(thread_db* tdbb, UCHAR* key);
	SSHORT			getRelLockKeyLength() const;

	void			cleanUp();

	class RelPagesSnapshot : public Firebird::Array<RelationPages*>
	{
	public:
		typedef Firebird::Array<RelationPages*> inherited;

		RelPagesSnapshot(thread_db* tdbb, jrd_rel* relation)
		{
			spt_tdbb = tdbb;
			spt_relation = relation;
		}

		~RelPagesSnapshot() { clear(); }

		void clear();
	private:
		thread_db*	spt_tdbb;
		jrd_rel*	spt_relation;

	friend class jrd_rel;
	};

	void fillPagesSnapshot(RelPagesSnapshot&, const bool AttachmentOnly = false);

private:
	typedef Firebird::SortedArray<
				RelationPages*,
				Firebird::EmptyStorage<RelationPages*>,
				SLONG,
				RelationPages>
			RelationPagesInstances;

	RelationPagesInstances* rel_pages_inst;
	RelationPages			rel_pages_base;
	RelationPages*			rel_pages_free;

	RelationPages*	getPagesInternal(thread_db* tdbb, SLONG tran, bool allocPages);

public:
	explicit jrd_rel(MemoryPool& p)
		: rel_name(p), rel_owner_name(p), rel_view_contexts(p), rel_security_name(p)
	{ }

	bool hasTriggers() const;
};

// rel_flags

const ULONG REL_scanned					= 0x0001;	// Field expressions scanned (or being scanned)
const ULONG REL_system					= 0x0002;
const ULONG REL_deleted					= 0x0004;	// Relation known gonzo
const ULONG REL_get_dependencies		= 0x0008;	// New relation needs dependencies during scan
const ULONG REL_force_scan				= 0x0010;	// system relation has been updated since ODS change, force a scan
const ULONG REL_check_existence			= 0x0020;	// Existence lock released pending drop of relation
const ULONG REL_blocking				= 0x0040;	// Blocking someone from dropping relation
const ULONG REL_sys_triggers			= 0x0080;	// The relation has system triggers to compile
const ULONG REL_sql_relation			= 0x0100;	// Relation defined as sql table
const ULONG REL_check_partners			= 0x0200;	// Rescan primary dependencies and foreign references
const ULONG REL_being_scanned			= 0x0400;	// relation scan in progress
const ULONG REL_sys_trigs_being_loaded	= 0x0800;	// System triggers being loaded
const ULONG REL_deleting				= 0x1000;	// relation delete in progress
const ULONG REL_temp_tran				= 0x2000;	// relation is a GTT delete rows
const ULONG REL_temp_conn				= 0x4000;	// relation is a GTT preserve rows
const ULONG REL_virtual					= 0x8000;	// relation is virtual
const ULONG REL_jrd_view				= 0x10000;	// relation is VIEW


inline bool jrd_rel::isSystem() const
{
	return rel_flags & REL_system;
}

inline bool jrd_rel::isTemporary() const
{
	return (rel_flags & (REL_temp_tran | REL_temp_conn));
}

inline bool jrd_rel::isVirtual() const
{
	return (rel_flags & REL_virtual);
}

inline bool jrd_rel::isView() const
{
	return (rel_flags & REL_jrd_view);
}

inline RelationPages* jrd_rel::getPages(thread_db* tdbb, SLONG tran, bool allocPages)
{
	if (!isTemporary())
		return &rel_pages_base;

	return getPagesInternal(tdbb, tran, allocPages);
}

// Field block, one for each field in a scanned relation

class jrd_fld : public pool_alloc<type_fld>
{
public:
	jrd_nod*	fld_validation;		// validation clause, if any
	jrd_nod*	fld_not_null;		// if field cannot be NULL
	jrd_nod*	fld_missing_value;	// missing value, if any
	jrd_nod*	fld_computation;	// computation for virtual field
	jrd_nod*	fld_source;			// source for view fields
	jrd_nod*	fld_default_value;	// default value, if any
	ArrayField*	fld_array;			// array description, if array
	Firebird::MetaName	fld_name;	// Field name
	Firebird::MetaName	fld_security_name;	// security class name for field

public:
	explicit jrd_fld(MemoryPool& p)
		: fld_name(p), fld_security_name(p)
	{ }
};

}

#endif	// JRD_RELATION_H
