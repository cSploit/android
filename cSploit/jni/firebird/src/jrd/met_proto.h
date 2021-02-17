/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		met_proto.h
 *	DESCRIPTION:	Prototype header file for met.cpp
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
 */

#ifndef JRD_MET_PROTO_H
#define JRD_MET_PROTO_H

#include "../common/classes/MetaName.h"

struct dsc;

namespace Jrd
{
	class jrd_tra;
	class jrd_req;
	class JrdStatement;
	class jrd_prc;
	class Format;
	class jrd_rel;
	class CompilerScratch;
	class DmlNode;
	class Database;
	struct bid;
	struct index_desc;
	class jrd_fld;
	class Shadow;
	class DeferredWork;
	struct FieldInfo;
	class ExceptionItem;

	// index status
	enum IndexStatus
	{
		MET_object_active,
		MET_object_inactive,
		MET_object_unknown
	};
}

struct SubtypeInfo
{
	SubtypeInfo()
		: attributes(0),
		  ignoreAttributes(true)
	{
	}

	Firebird::MetaName charsetName;
	Firebird::MetaName collationName;
	Firebird::MetaName baseCollationName;
	USHORT attributes;
	bool ignoreAttributes;
	Firebird::UCharBuffer specificAttributes;
};

void		MET_activate_shadow(Jrd::thread_db*);
ULONG		MET_align(const dsc*, ULONG);
Jrd::DeferredWork*	MET_change_fields(Jrd::thread_db*, Jrd::jrd_tra*, const dsc*);
Jrd::Format*	MET_current(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_delete_dependencies(Jrd::thread_db*, const Firebird::MetaName&, int, Jrd::jrd_tra*);
void		MET_delete_shadow(Jrd::thread_db*, USHORT);
bool		MET_dsql_cache_use(Jrd::thread_db* tdbb, int type, const Firebird::MetaName& name, const Firebird::MetaName& package = "");
void		MET_dsql_cache_release(Jrd::thread_db* tdbb, int type, const Firebird::MetaName& name, const Firebird::MetaName& package = "");
void		MET_error(const TEXT*, ...);
Jrd::Format*	MET_format(Jrd::thread_db*, Jrd::jrd_rel*, USHORT);
bool		MET_get_char_coll_subtype(Jrd::thread_db*, USHORT*, const UCHAR*, USHORT);
bool		MET_get_char_coll_subtype_info(Jrd::thread_db*, USHORT, SubtypeInfo* info);
Jrd::DmlNode*	MET_get_dependencies(Jrd::thread_db*, Jrd::jrd_rel*, const UCHAR*, const ULONG,
								Jrd::CompilerScratch*, Jrd::bid*, Jrd::JrdStatement**,
								Jrd::CompilerScratch**, const Firebird::MetaName&, int, USHORT,
								Jrd::jrd_tra*, const Firebird::MetaName& = Firebird::MetaName());
Jrd::jrd_fld*	MET_get_field(const Jrd::jrd_rel*, USHORT);
ULONG		MET_get_rel_flags_from_TYPE(USHORT);
void		MET_get_shadow_files(Jrd::thread_db*, bool);
void		MET_load_db_triggers(Jrd::thread_db*, int);
void		MET_load_ddl_triggers(Jrd::thread_db* tdbb);
bool		MET_load_exception(Jrd::thread_db*, Jrd::ExceptionItem&);
void		MET_load_trigger(Jrd::thread_db*, Jrd::jrd_rel*, const Firebird::MetaName&, Jrd::trig_vec**);
void		MET_lookup_cnstrt_for_index(Jrd::thread_db*, Firebird::MetaName& constraint, const Firebird::MetaName& index_name);
void		MET_lookup_cnstrt_for_trigger(Jrd::thread_db*, Firebird::MetaName&, Firebird::MetaName&, const Firebird::MetaName&);
void		MET_lookup_exception(Jrd::thread_db*, SLONG, /* OUT */ Firebird::MetaName&, /* OUT */ Firebird::string*);
int			MET_lookup_field(Jrd::thread_db*, Jrd::jrd_rel*, const Firebird::MetaName&);
Jrd::BlobFilter*	MET_lookup_filter(Jrd::thread_db*, SSHORT, SSHORT);
bool		MET_load_generator(Jrd::thread_db*, Jrd::GeneratorItem&, bool* sysGen = 0, SLONG* step = 0);
SLONG		MET_lookup_generator(Jrd::thread_db*, const Firebird::MetaName&, bool* sysGen = 0, SLONG* step = 0);
bool		MET_lookup_generator_id(Jrd::thread_db*, SLONG, Firebird::MetaName&, bool* sysGen = 0);
void		MET_update_generator_increment(Jrd::thread_db* tdbb, SLONG gen_id, SLONG step);
void		MET_lookup_index(Jrd::thread_db*, Firebird::MetaName&, const Firebird::MetaName&, USHORT);
SLONG		MET_lookup_index_name(Jrd::thread_db*, const Firebird::MetaName&, SLONG*, Jrd::IndexStatus* status);
bool		MET_lookup_partner(Jrd::thread_db*, Jrd::jrd_rel*, struct Jrd::index_desc*, const TEXT*);
Jrd::jrd_prc*	MET_lookup_procedure(Jrd::thread_db*, const Firebird::QualifiedName&, bool);
Jrd::jrd_prc*	MET_lookup_procedure_id(Jrd::thread_db*, USHORT, bool, bool, USHORT);
Jrd::jrd_rel*	MET_lookup_relation(Jrd::thread_db*, const Firebird::MetaName&);
Jrd::jrd_rel*	MET_lookup_relation_id(Jrd::thread_db*, SLONG, bool);
Jrd::DmlNode*	MET_parse_blob(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::bid*, Jrd::CompilerScratch**,
							   Jrd::JrdStatement**, bool, bool);
void		MET_parse_sys_trigger(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_post_existence(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_prepare(Jrd::thread_db*, Jrd::jrd_tra*, USHORT, const UCHAR*);
Jrd::jrd_prc*	MET_procedure(Jrd::thread_db*, USHORT, bool, USHORT);
Jrd::jrd_rel*	MET_relation(Jrd::thread_db*, USHORT);
void		MET_release_existence(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_release_trigger(Jrd::thread_db*, Jrd::trig_vec**, const Firebird::MetaName&);
void		MET_release_triggers(Jrd::thread_db*, Jrd::trig_vec**);
#ifdef DEV_BUILD
void		MET_verify_cache(Jrd::thread_db*);
#endif
void		MET_clear_cache(Jrd::thread_db*);
bool		MET_routine_in_use(Jrd::thread_db*, Jrd::Routine*);
void		MET_revoke(Jrd::thread_db*, Jrd::jrd_tra*, const Firebird::MetaName&,
	const Firebird::MetaName&, const Firebird::string&);
void		MET_scan_partners(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_scan_relation(Jrd::thread_db*, Jrd::jrd_rel*);
void		MET_trigger_msg(Jrd::thread_db*, Firebird::string&, const Firebird::MetaName&, USHORT);
void		MET_update_shadow(Jrd::thread_db*, Jrd::Shadow*, USHORT);
void		MET_update_transaction(Jrd::thread_db*, Jrd::jrd_tra*, const bool);
void		MET_get_domain(Jrd::thread_db*, MemoryPool& csbPool, const Firebird::MetaName&, dsc*,
	Jrd::FieldInfo*);
Firebird::MetaName MET_get_relation_field(Jrd::thread_db*, MemoryPool& csbPool,
	const Firebird::MetaName&, const Firebird::MetaName&, dsc*, Jrd::FieldInfo*);
void		MET_update_partners(Jrd::thread_db*);
int			MET_get_linger(Jrd::thread_db*);
#endif // JRD_MET_PROTO_H
