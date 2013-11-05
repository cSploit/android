/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		dyn_df_proto.h
 *	DESCRIPTION:	Prototype Header file for dyn_def.epp
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

#ifndef JRD_DYN_DF_PROTO_H
#define JRD_DYN_DF_PROTO_H

void DYN_define_collation(Jrd::Global*, const UCHAR**);
void DYN_define_constraint(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_define_dimension(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_define_exception(Jrd::Global*, const UCHAR**);
void DYN_define_file(Jrd::Global*, const UCHAR**, SLONG, SLONG*, USHORT);
void DYN_define_filter(Jrd::Global*, const UCHAR**);
void DYN_define_function(Jrd::Global*, const UCHAR**);
void DYN_define_function_arg(Jrd::Global*, const UCHAR**, Firebird::MetaName*);
void DYN_define_generator(Jrd::Global*, const UCHAR**);
void DYN_define_global_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_define_index(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, UCHAR,
					  Firebird::MetaName*, Firebird::MetaName*, Firebird::MetaName*, UCHAR*);
void DYN_define_local_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_define_parameter(Jrd::Global*, const UCHAR**, Firebird::MetaName*);
void DYN_define_procedure(Jrd::Global*, const UCHAR**);
void DYN_define_relation(Jrd::Global*, const UCHAR**);
void DYN_define_role(Jrd::Global*, const UCHAR**);
void DYN_define_security_class(Jrd::Global*, const UCHAR**);
void DYN_define_shadow(Jrd::Global*, const UCHAR**);
void DYN_define_sql_field(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*);
void DYN_define_trigger(Jrd::Global*, const UCHAR**, const Firebird::MetaName*, Firebird::MetaName*, const bool);
void DYN_define_trigger_msg(Jrd::Global*, const UCHAR**, const Firebird::MetaName*);
void DYN_define_view_relation(Jrd::Global*, const UCHAR**, const Firebird::MetaName*);
void DYN_define_difference(Jrd::Global*, const UCHAR**);

#endif // JRD_DYN_DF_PROTO_H

