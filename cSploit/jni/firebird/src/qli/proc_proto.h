/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		proc_proto.h
 *	DESCRIPTION:	Prototype header file for proc.cpp
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

#ifndef QLI_PROC_PROTO_H
#define QLI_PROC_PROTO_H

void	PRO_close(qli_dbb*, FB_API_HANDLE);
void	PRO_commit(qli_dbb*);
void	PRO_copy_procedure(qli_dbb*, const TEXT*, qli_dbb*, const TEXT*);
void	PRO_create(qli_dbb*, const TEXT*);
int		PRO_delete_procedure(qli_dbb*, const TEXT*);
void	PRO_edit_procedure(qli_dbb*, const TEXT*);
FB_API_HANDLE	PRO_fetch_procedure(qli_dbb*, const TEXT*);
bool	PRO_get_line (FB_API_HANDLE, TEXT*, USHORT);
void	PRO_invoke(qli_dbb*, const TEXT*);
FB_API_HANDLE	PRO_open_blob(qli_dbb*, ISC_QUAD&);
int		PRO_rename_procedure(qli_dbb*, const TEXT*, const TEXT*);
void	PRO_rollback(qli_dbb*);

typedef void (*extract_fn_t)(void* file, const TEXT* name,
							  USHORT length, qli_dbb* database, ISC_QUAD& blob_id);
void	PRO_scan(qli_dbb*, extract_fn_t, void*);

void	PRO_setup(qli_dbb*);
FB_API_HANDLE	PRO_transaction(qli_dbb*, bool);

#endif // QLI_PROC_PROTO_H

