/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		exe_proto.h
 *	DESCRIPTION:	Prototype header file for exe.cpp
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

#ifndef JRD_EXE_PROTO_H
#define JRD_EXE_PROTO_H

namespace Jrd {
	class jrd_req;
	class jrd_nod;
	class jrd_tra;
}

void EXE_assignment(Jrd::thread_db*, Jrd::jrd_nod*);
void EXE_assignment(Jrd::thread_db* tdbb, Jrd::jrd_nod* to, dsc* from_desc, bool from_null,
	Jrd::jrd_nod* missing_node, Jrd::jrd_nod* missing2_node);
void EXE_execute_db_triggers(Jrd::thread_db*, Jrd::jrd_tra*, enum Jrd::jrd_req::req_ta);
Jrd::jrd_nod* EXE_looper(Jrd::thread_db* tdbb, Jrd::jrd_req* request, Jrd::jrd_nod* in_node);
Jrd::jrd_req* EXE_find_request(Jrd::thread_db*, Jrd::jrd_req*, bool);
void EXE_receive(Jrd::thread_db*, Jrd::jrd_req*, USHORT, USHORT, UCHAR*, bool = false);
void EXE_send(Jrd::thread_db*, Jrd::jrd_req*, USHORT, USHORT, const UCHAR*);
void EXE_start(Jrd::thread_db*, Jrd::jrd_req*, Jrd::jrd_tra*);
void EXE_unwind(Jrd::thread_db*, Jrd::jrd_req*);
#ifdef SCROLLABLE_CURSORS
void EXE_seek(Jrd::thread_db*, Jrd::jrd_req*, USHORT, ULONG);
#endif

#endif // JRD_EXE_PROTO_H

