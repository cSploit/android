/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dbg_proto.h
 *	DESCRIPTION:	Prototype header file for dbg.cpp
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

#ifndef JRD_DBG_PROTO_H
#define JRD_DBG_PROTO_H

/* Don't declare DBG_supervisor in _ANSI_PROTOTYPES_, it screws up val.cpp */

namespace Jrd
{
	class jrd_nod;
	struct record_param;
	class SortMap;
}

namespace Firebird
{
	class MemoryPool;
}
struct blk;

int DBG_supervisor(int);

int DBG_all();
int DBG_analyze(int);
int DBG_bdbs();
int DBG_precedence();
int DBG_block(blk*);
int DBG_check(int);
int DBG_close();
int DBG_eval(int);
int DBG_examine(int *);
int DBG_init();
int DBG_open();
int DBG_pool(Firebird::MemoryPool*);
int DBG_pretty(const Jrd::jrd_nod*, int);
int DBG_rpb(Jrd::record_param*);
int DBG_smb(Jrd::SortMap*, int);
int DBG_verify();
int DBG_window(int *);
int DBG_memory();

#endif // JRD_DBG_PROTO_H

