/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		msc_proto.h
 *	DESCRIPTION:	Prototype header file for msc.cpp
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

#ifndef GPRE_MSC_PROTO_H
#define GPRE_MSC_PROTO_H

#include "../gpre/parse.h"

act*		MSC_action(gpre_req*, act_t);
UCHAR*		MSC_alloc(int);
UCHAR*		MSC_alloc_permanent(int);
gpre_nod*	MSC_binary(nod_t, gpre_nod*, gpre_nod*);
gpre_ctx*	MSC_context(gpre_req*);
void		MSC_copy(const char*, int, char*);
void		MSC_copy_cat(const char*, int, const char*, int, char*);
gpre_sym*	MSC_find_symbol(gpre_sym*, sym_t);
void		MSC_free(void*);
void		MSC_free_request(gpre_req*);
void		MSC_init();
bool		MSC_match(const kwwords_t);
gpre_nod*	MSC_node(nod_t, SSHORT);
gpre_nod*	MSC_pop(gpre_lls**);
PRV			MSC_privilege_block();
void		MSC_push(gpre_nod*, gpre_lls**);
ref*		MSC_reference(ref**);
gpre_req*	MSC_request(req_t);
SCHAR*		MSC_string(const TEXT*);
gpre_sym*	MSC_symbol(sym_t, const TEXT*, USHORT, gpre_ctx*);
gpre_nod*	MSC_unary(nod_t, gpre_nod*);
gpre_usn*	MSC_username(const SCHAR*, USHORT);

#endif // GPRE_MSC_PROTO_H

