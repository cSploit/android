/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		remot_proto.h
 *	DESCRIPTION:	Prototpe header file for remote.cpp
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

#ifndef REMOTE_REMOT_PROTO_H
#define REMOTE_REMOT_PROTO_H

namespace Firebird
{
	class ClumpletReader;
}

void		REMOTE_cleanup_transaction (struct Rtr *);
ULONG		REMOTE_compute_batch_size (rem_port*, USHORT, P_OP, const rem_fmt*);
void		REMOTE_get_timeout_params(rem_port* port, Firebird::ClumpletReader* pb);
struct Rrq*	REMOTE_find_request (struct Rrq *, USHORT);
void		REMOTE_free_packet (rem_port*, struct packet *, bool = false);
struct rem_str*	REMOTE_make_string (const SCHAR*);
void		REMOTE_release_messages (struct RMessage*);
void		REMOTE_release_request (struct Rrq *);
void		REMOTE_reset_request (struct Rrq *, struct RMessage*);
void		REMOTE_reset_statement (struct Rsr *);
void		REMOTE_save_status_strings (ISC_STATUS *);
bool_t		REMOTE_getbytes (XDR*, SCHAR*, u_int);

#endif // REMOTE_REMOT_PROTO_H

