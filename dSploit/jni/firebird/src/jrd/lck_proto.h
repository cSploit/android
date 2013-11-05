/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		lck_proto.h
 *	DESCRIPTION:	Prototype header file for lck.cpp
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

#ifndef JRD_LCK_PROTO_H
#define JRD_LCK_PROTO_H

#include "../jrd/lck.h"
#include "../lock/lock_proto.h"

namespace Jrd {
	enum lck_t;
}

void	LCK_assert(Jrd::thread_db*, Jrd::Lock*);
bool	LCK_cancel_wait(Jrd::Attachment*);
bool	LCK_convert(Jrd::thread_db*, Jrd::Lock*, USHORT, SSHORT);
bool	LCK_convert_opt(Jrd::thread_db*, Jrd::Lock*, USHORT);
void	LCK_downgrade(Jrd::thread_db*, Jrd::Lock*);
void	LCK_fini(Jrd::thread_db*, Jrd::lck_owner_t);
SLONG	LCK_get_owner_handle(Jrd::thread_db*, Jrd::lck_t);
SLONG	LCK_get_owner_handle_by_type(Jrd::thread_db*, Jrd::lck_owner_t);
void	LCK_init(Jrd::thread_db*, Jrd::lck_owner_t);
bool	LCK_lock(Jrd::thread_db*, Jrd::Lock*, USHORT, SSHORT);
bool	LCK_lock_opt(Jrd::thread_db*, Jrd::Lock*, USHORT, SSHORT);
SLONG	LCK_query_data(Jrd::thread_db*, Jrd::Lock*, Jrd::lck_t, USHORT);
SLONG	LCK_read_data(Jrd::thread_db*, Jrd::Lock*);
void	LCK_release(Jrd::thread_db*, Jrd::Lock*);
void	LCK_re_post(Jrd::thread_db*, Jrd::Lock*);
void	LCK_write_data(Jrd::thread_db*, Jrd::Lock*, SLONG);

#endif // JRD_LCK_PROTO_H
