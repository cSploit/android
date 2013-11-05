/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dpm_proto.h
 *	DESCRIPTION:	Prototype header file for dpm.cpp
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

#ifndef JRD_DPM_PROTO_H
#define JRD_DPM_PROTO_H

#include "../jrd/RecordNumber.h"
#include "../jrd/sbm.h"

// fwd. decl.
namespace Jrd {
	class blb;
	class jrd_rel;
	struct record_param;
	class Record;
	class jrd_tra;
	struct win;
}
namespace Ods {
	struct pag;
	struct data_page;
}

Ods::pag* DPM_allocate(Jrd::thread_db*, Jrd::win*);
void	DPM_backout(Jrd::thread_db*, Jrd::record_param*);
void	DPM_backout_mark(Jrd::thread_db*, Jrd::record_param*, const Jrd::jrd_tra*);
double	DPM_cardinality(Jrd::thread_db*, Jrd::jrd_rel*, const Jrd::Format*);
bool	DPM_chain(Jrd::thread_db*, Jrd::record_param*, Jrd::record_param*);
int		DPM_compress(Jrd::thread_db*, Ods::data_page*);
void	DPM_create_relation(Jrd::thread_db*, Jrd::jrd_rel*);
SLONG	DPM_data_pages(Jrd::thread_db*, Jrd::jrd_rel*);
void	DPM_delete(Jrd::thread_db*, Jrd::record_param*, SLONG);
void	DPM_delete_relation(Jrd::thread_db*, Jrd::jrd_rel*);
bool	DPM_fetch(Jrd::thread_db*, Jrd::record_param*, USHORT);
SSHORT	DPM_fetch_back(Jrd::thread_db*, Jrd::record_param*, USHORT, SSHORT);
void	DPM_fetch_fragment(Jrd::thread_db*, Jrd::record_param*, USHORT);
SINT64	DPM_gen_id(Jrd::thread_db*, SLONG, bool, SINT64);
bool	DPM_get(Jrd::thread_db*, Jrd::record_param*, SSHORT);
ULONG	DPM_get_blob(Jrd::thread_db*, Jrd::blb*, RecordNumber, bool, SLONG);
bool	DPM_next(Jrd::thread_db*, Jrd::record_param*, USHORT,
#ifdef SCROLLABLE_CURSORS
				bool,
#endif
				bool);
void	DPM_pages(Jrd::thread_db*, SSHORT, int, ULONG, SLONG);
SLONG	DPM_prefetch_bitmap(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::PageBitmap*, SLONG);
void	DPM_scan_pages(Jrd::thread_db*);
void	DPM_store(Jrd::thread_db*, Jrd::record_param*, Jrd::PageStack&, USHORT);
RecordNumber DPM_store_blob(Jrd::thread_db*, Jrd::blb*, Jrd::Record*);
void	DPM_rewrite_header(Jrd::thread_db*, Jrd::record_param*);
void	DPM_update(Jrd::thread_db*, Jrd::record_param*, Jrd::PageStack*, const Jrd::jrd_tra*);

void DPM_create_relation_pages(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::RelationPages*);
void DPM_delete_relation_pages(Jrd::thread_db*, Jrd::jrd_rel*, Jrd::RelationPages*);

#endif // JRD_DPM_PROTO_H

