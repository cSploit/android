/*
 *	PROGRAM:	JRD Access method
 *	MODULE:		tpc_proto.h
 *	DESCRIPTION:	Prototype Header file for tpc.cpp
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

#ifndef JRD_TPC_PROTO_H
#define JRD_TPC_PROTO_H

int		TPC_cache_state(Jrd::thread_db*, SLONG);
void	TPC_initialize_tpc(Jrd::thread_db*, SLONG);
void	TPC_set_state(Jrd::thread_db*, SLONG, SSHORT);
int		TPC_snapshot_state(Jrd::thread_db*, SLONG);
void	TPC_update_cache(Jrd::thread_db*, const Ods::tx_inv_page*, SLONG);

#endif // JRD_TPC_PROTO_H

