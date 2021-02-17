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

void	DYN_UTIL_store_check_constraints(Jrd::thread_db*, Jrd::jrd_tra*,
			const Firebird::MetaName&, const Firebird::MetaName&);
bool	DYN_UTIL_find_field_source(Jrd::thread_db* tdbb, Jrd::jrd_tra* transaction,
			const Firebird::MetaName& view_name, USHORT context, const TEXT* local_name,
			TEXT* output_field_name);
void	DYN_UTIL_generate_generator_name(Jrd::thread_db*, Firebird::MetaName&);
void	DYN_UTIL_generate_trigger_name(Jrd::thread_db*, Jrd::jrd_tra*, Firebird::MetaName&);
void	DYN_UTIL_generate_index_name(Jrd::thread_db*, Jrd::jrd_tra*, Firebird::MetaName&, UCHAR);
void	DYN_UTIL_generate_field_position(Jrd::thread_db*, const Firebird::MetaName&, SLONG*);
void	DYN_UTIL_generate_field_name(Jrd::thread_db*, TEXT*);
void	DYN_UTIL_generate_field_name(Jrd::thread_db*, Firebird::MetaName&);
void	DYN_UTIL_generate_constraint_name(Jrd::thread_db*, Firebird::MetaName&);
void	DYN_UTIL_check_unique_name(Jrd::thread_db* tdbb, Jrd::jrd_tra* transaction,
								   const Firebird::MetaName& object_name, int object_type);
SINT64	DYN_UTIL_gen_unique_id(Jrd::thread_db*, SSHORT, const char*);

#endif // JRD_DYN_UT_PROTO_H
