/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		dyn_md_proto.h
 *	DESCRIPTION:	Prototype Header file for dyn_mod.epp
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

#ifndef JRD_DYN_MD_PROTO_H
#define JRD_DYN_MD_PROTO_H

void DYN_modify_charset(Jrd::Global*, const UCHAR**);
void DYN_modify_collation(Jrd::Global*, const UCHAR**);
void DYN_modify_database(Jrd::Global*, const UCHAR**);
void DYN_modify_exception(Jrd::Global*, const UCHAR**);
void DYN_modify_filter(Jrd::Global*, const UCHAR**);
void DYN_modify_function(Jrd::Global*, const UCHAR**);
void DYN_modify_generator(Jrd::Global*, const UCHAR**);
void DYN_modify_global_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_modify_index(Jrd::Global*, const UCHAR**);
void DYN_modify_local_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*);
void DYN_modify_parameter(Jrd::Global*, const UCHAR**);
void DYN_modify_procedure(Jrd::Global*, const UCHAR**);
void DYN_modify_relation(Jrd::Global*, const UCHAR**);
void DYN_modify_role(Jrd::Global*, const UCHAR**);
void DYN_modify_trigger(Jrd::Global*, const UCHAR**);
void DYN_modify_trigger_msg(Jrd::Global*, const UCHAR**, Firebird::MetaName*);
void DYN_modify_sql_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*);
void DYN_modify_view(Jrd::Global*, const UCHAR**);
void DYN_modify_mapping(Jrd::Global*, const UCHAR**);


#endif // JRD_DYN_MD_PROTO_H

