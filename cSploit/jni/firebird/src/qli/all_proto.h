/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		all_proto.h
 *	DESCRIPTION:	Prototype header file for all.cpp
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

#ifndef QLI_ALL_PROTO_H
#define QLI_ALL_PROTO_H

blk*		ALLQ_alloc(qli_plb*, UCHAR, int);
blk*		ALLQ_extend(blk**, int);
void		ALLQ_fini();
void		ALLQ_free(void*);
void		ALLQ_init();
SCHAR*		ALLQ_malloc(SLONG);
qli_plb*	ALLQ_pool();
void		ALLQ_push(blk*, qli_lls**);
blk*		ALLQ_pop(qli_lls**);
void		ALLQ_release(qli_frb*);
void		ALLQ_rlpool(qli_plb*);

#endif // QLI_ALL_PROTO_H

