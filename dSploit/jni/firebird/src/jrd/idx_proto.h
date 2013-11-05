/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		idx_proto.h
 *	DESCRIPTION:	Prototype header file for idx.cpp
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

#ifndef JRD_IDX_PROTO_H
#define JRD_IDX_PROTO_H

#include "../jrd/btr.h"
#include "../jrd/exe.h"
#include "../jrd/req.h"

namespace Jrd {
	class jrd_rel;
	class jrd_tra;
	struct record_param;
	class IndexBlock;
	enum idx_e;
	struct index_desc;
	class CompilerScratch;
	class jrd_fld;
	class Record;
}

void IDX_check_access(Jrd::thread_db*, Jrd::CompilerScratch*, Jrd::jrd_rel*, Jrd::jrd_rel*);
bool IDX_check_master_types (Jrd::thread_db*, Jrd::index_desc&, Jrd::jrd_rel*, int&);
void IDX_create_index(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::index_desc*, const TEXT*,
					  USHORT*, Jrd::jrd_tra*, Jrd::SelectivityList&);
Jrd::IndexBlock* IDX_create_index_block(Jrd::thread_db*, Jrd::jrd_rel*, USHORT);
void IDX_delete_index(Jrd::thread_db*, Jrd::jrd_rel*, USHORT);
void IDX_delete_indices(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::RelationPages*);
Jrd::idx_e IDX_erase(Jrd::thread_db*, Jrd::record_param*, Jrd::jrd_tra*, Jrd::jrd_rel**, USHORT*);
void IDX_garbage_collect(Jrd::thread_db*, Jrd::record_param*, Jrd::RecordStack&, Jrd::RecordStack&);
Jrd::idx_e IDX_modify(Jrd::thread_db*, Jrd::record_param*, Jrd::record_param*,
							 Jrd::jrd_tra*, Jrd::jrd_rel**, USHORT *);
Jrd::idx_e IDX_modify_check_constraints(Jrd::thread_db*, Jrd::record_param*, Jrd::record_param*,
											  Jrd::jrd_tra*, Jrd::jrd_rel**, USHORT*);
void IDX_statistics(Jrd::thread_db*, Jrd::jrd_rel*, USHORT, Jrd::SelectivityList&);
Jrd::idx_e IDX_store(Jrd::thread_db*, Jrd::record_param*, Jrd::jrd_tra*, Jrd::jrd_rel**, USHORT*);
void IDX_modify_flag_uk_modified(Jrd::thread_db*, Jrd::record_param*, Jrd::record_param*, Jrd::jrd_tra*);


#endif // JRD_IDX_PROTO_H
