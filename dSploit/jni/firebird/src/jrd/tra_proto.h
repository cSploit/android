/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		tra_proto.h
 *	DESCRIPTION:	Prototype header file for tra.cpp
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

#ifndef JRD_TRA_PROTO_H
#define JRD_TRA_PROTO_H

#include "../jrd/tra.h"

namespace Jrd {
	class Attachment;
	class Database;
	class TraceTransactionEnd;
}

bool	TRA_active_transactions(Jrd::thread_db*, Jrd::Database*);
void	TRA_cleanup(Jrd::thread_db*);
void	TRA_commit(Jrd::thread_db*, Jrd::jrd_tra*, const bool);
void	TRA_extend_tip(Jrd::thread_db*, ULONG /*, struct Jrd::win* */);
int		TRA_fetch_state(Jrd::thread_db*, SLONG);
void	TRA_get_inventory(Jrd::thread_db*, UCHAR*, ULONG, ULONG);
int		TRA_get_state(Jrd::thread_db*, SLONG);

#ifdef SUPERSERVER_V2
void	TRA_header_write(Jrd::thread_db*, Jrd::Database*, SLONG);
#endif
void	TRA_init(Jrd::Database*);
void	TRA_invalidate(Jrd::Database*, ULONG);
void	TRA_link_cursor(Jrd::jrd_tra*, Jrd::dsql_req*);
void	TRA_unlink_cursor(Jrd::jrd_tra*, Jrd::dsql_req*);
void	TRA_post_resources(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::ResourceList&);
bool	TRA_precommited(Jrd::thread_db*, SLONG, SLONG);
void	TRA_prepare(Jrd::thread_db*, Jrd::jrd_tra*, USHORT, const UCHAR*);
Jrd::jrd_tra*	TRA_reconnect(Jrd::thread_db*, const UCHAR*, USHORT);
void	TRA_release_transaction(Jrd::thread_db*, Jrd::jrd_tra*, Jrd::TraceTransactionEnd*);
void	TRA_rollback(Jrd::thread_db*, Jrd::jrd_tra*, const bool, const bool);
void	TRA_set_state(Jrd::thread_db*, Jrd::jrd_tra*, SLONG, SSHORT);
void	TRA_shutdown_attachment(Jrd::thread_db*, Jrd::Attachment*);
int		TRA_snapshot_state(Jrd::thread_db*, const Jrd::jrd_tra*, SLONG);
Jrd::jrd_tra*	TRA_start(Jrd::thread_db*, ULONG flags, SSHORT lock_timeout, Jrd::jrd_tra* outer = NULL);
Jrd::jrd_tra*	TRA_start(Jrd::thread_db*, int, const UCHAR*, Jrd::jrd_tra* outer = NULL);
int		TRA_state(const UCHAR*, ULONG, ULONG);
bool	TRA_sweep(Jrd::thread_db*, Jrd::jrd_tra*);
int		TRA_wait(Jrd::thread_db*, Jrd::jrd_tra*, SLONG, Jrd::jrd_tra::wait_t);
void	TRA_attach_request(Jrd::jrd_tra* transaction, Jrd::jrd_req* request);
void	TRA_detach_request(Jrd::jrd_req* request);

#endif // JRD_TRA_PROTO_H

