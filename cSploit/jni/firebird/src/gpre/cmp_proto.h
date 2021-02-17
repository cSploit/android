/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		cmp_proto.h
 *	DESCRIPTION:	Prototype header file for cmp.cpp
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

#ifndef GPRE_CMP_PROTO_H
#define GPRE_CMP_PROTO_H

void	CMP_check(gpre_req*, SSHORT);
void	CMP_compile_request(gpre_req*);
void	CMP_external_field(gpre_req*, const gpre_fld*);
void	CMP_init();
ULONG	CMP_next_ident();
void	CMP_stuff_symbol(gpre_req*, const gpre_sym*);
void	CMP_t_start(gpre_tra*);

#endif // GPRE_CMP_PROTO_H

