/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		nav_proto.h
 *	DESCRIPTION:	Prototype header file for nav.cpp
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

#ifndef JRD_NAV_PROTO_H
#define JRD_NAV_PROTO_H

#include "../jrd/rse.h"

struct exp_index_buf;

namespace Jrd {
	struct record_param;
	class jrd_nod;
	class RecordSource;
	struct win;
	struct irsb_nav;
}

#ifdef SCROLLABLE_CURSORS
exp_index_buf* NAV_expand_index(Jrd::win*, Jrd::irsb_nav*);
#endif
bool NAV_get_record(Jrd::thread_db* tdbb, Jrd::RecordSource*, Jrd::irsb_nav*,
					Jrd::record_param*, Jrd::rse_get_mode);

#endif // JRD_NAV_PROTO_H

