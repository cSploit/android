/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		opt_proto.h
 *	DESCRIPTION:	Prototype header file for opt.cpp
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

#ifndef JRD_OPT_PROTO_H
#define JRD_OPT_PROTO_H

#include "../jrd/jrd.h"
#include "../jrd/btr.h"
#include "../jrd/rse.h"
#include "../jrd/lls.h"

namespace Jrd {
	class jrd_req;
	class jrd_rel;
	class jrd_nod;
	class RecordSelExpr;
	class RecordSource;
	struct index_desc;
	class CompilerScratch;
	class OptimizerBlk;
}

bool OPT_access_path(const Jrd::jrd_req*, UCHAR*, SLONG, ULONG*);
Jrd::RecordSource* OPT_compile(Jrd::thread_db*, Jrd::CompilerScratch*,
							   Jrd::RecordSelExpr*, Jrd::NodeStack*);

// Begin only exported for VMS
Jrd::jrd_nod* OPT_make_dbkey(Jrd::OptimizerBlk*, Jrd::jrd_nod*,
								  USHORT);
Jrd::jrd_nod* OPT_make_index(Jrd::thread_db*, Jrd::OptimizerBlk*, Jrd::jrd_rel*,
								  Jrd::index_desc*);
int OPT_match_index(Jrd::OptimizerBlk*, USHORT, Jrd::index_desc*);
// End only exported for VMS

#endif // JRD_OPT_PROTO_H

