/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		dyn_ut_proto.h
 *	DESCRIPTION:	Prototype Header file for dyn_util.epp
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


#ifndef JRD_DYN_UT_PROTO_H
#define JRD_DYN_UT_PROTO_H

void	DYN_UTIL_store_check_constraints(Jrd::thread_db*, Jrd::Global*,
			const Firebird::MetaName&, const Firebird::MetaName&);
bool	DYN_UTIL_find_field_source(Jrd::thread_db* tdbb, Jrd::Global* gbl,
			const Firebird::MetaName& view_name, USHORT context, const TEXT* local_name,
			TEXT* output_field_name);
bool	DYN_UTIL_get_prot(Jrd::thread_db*, Jrd::Global*, const SCHAR*,
			const SCHAR*, Jrd::SecurityClass::flags_t*);
void	DYN_UTIL_generate_trigger_name(Jrd::thread_db*, Jrd::Global*, Firebird::MetaName&);
void	DYN_UTIL_generate_index_name(Jrd::thread_db*, Jrd::Global*, Firebird::MetaName&, UCHAR);
void	DYN_UTIL_generate_field_position(Jrd::thread_db*, Jrd::Global*,
			const Firebird::MetaName&, SLONG*);
void	DYN_UTIL_generate_field_name(Jrd::thread_db*, Jrd::Global*, TEXT*);
void	DYN_UTIL_generate_field_name(Jrd::thread_db*, Jrd::Global*, Firebird::MetaName&);
void	DYN_UTIL_generate_constraint_name(Jrd::thread_db*, Jrd::Global*, Firebird::MetaName&);
SINT64	DYN_UTIL_gen_unique_id(Jrd::thread_db*, Jrd::Global*, SSHORT, const char*);
bool    DYN_UTIL_is_array(Jrd::thread_db*, Jrd::Global*, const Firebird::MetaName&);
void	DYN_UTIL_copy_domain(Jrd::thread_db*, Jrd::Global* gbl,
			const Firebird::MetaName&, const Firebird::MetaName&);

#endif // JRD_DYN_UT_PROTO_H

