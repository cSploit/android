/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		ext_proto.h
 *	DESCRIPTION:	Prototype header file for ext.cpp & extvms.cpp
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

#ifndef JRD_EXT_PROTO_H
#define JRD_EXT_PROTO_H

namespace Jrd {
	class ExternalFile;
	class jrd_tra;
	class RecordSource;
	class jrd_rel;
	class OptimizerBlk;
	struct record_param;
	struct bid;
}

double	EXT_cardinality(Jrd::thread_db*, Jrd::jrd_rel*);
void	EXT_erase(Jrd::record_param*, Jrd::jrd_tra*);
Jrd::ExternalFile*	EXT_file(Jrd::jrd_rel*, const TEXT*); //, Jrd::bid*);
void	EXT_fini(Jrd::jrd_rel*, bool);
bool	EXT_get(Jrd::thread_db*, Jrd::record_param*, FB_UINT64&);
void	EXT_modify(Jrd::record_param*, Jrd::record_param*, Jrd::jrd_tra*);

void	EXT_open(Jrd::Database*, Jrd::ExternalFile*);
void	EXT_store(Jrd::thread_db*, Jrd::record_param*);

void EXT_tra_attach(Jrd::ExternalFile*, Jrd::jrd_tra*);
void EXT_tra_detach(Jrd::ExternalFile*, Jrd::jrd_tra*);

#endif // JRD_EXT_PROTO_H
