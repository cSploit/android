/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		exp_proto.h
 *	DESCRIPTION:	Prototype header file for exp.cpp
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

#ifndef GPRE_EXP_PROTO_H
#define GPRE_EXP_PROTO_H

gpre_nod*	EXP_array(gpre_req*, gpre_fld*, bool, bool);
gpre_fld*	EXP_cast(gpre_fld*);
gpre_ctx*	EXP_context(gpre_req*, gpre_sym*);
gpre_fld*	EXP_field(gpre_ctx**);
void		EXP_left_paren(const TEXT*);
gpre_nod*	EXP_literal();
void		EXP_post_array(ref*);
ref*		EXP_post_field(gpre_fld*, gpre_ctx*, bool);
bool		EXP_match_paren();
gpre_rel*	EXP_relation();
gpre_rse*	EXP_rse(gpre_req*, gpre_sym*);
void		EXP_rse_cleanup(gpre_rse*);
gpre_nod*	EXP_subscript(gpre_req*);
SLONG		EXP_SLONG_ordinal(bool);
SINT64		EXP_SINT64_ordinal(bool);
SSHORT		EXP_SSHORT_ordinal(bool);
ULONG		EXP_ULONG_ordinal(bool);
USHORT		EXP_USHORT_ordinal(bool);
USHORT		EXP_pos_USHORT_ordinal(bool);

#endif // GPRE_EXP_PROTO_H

