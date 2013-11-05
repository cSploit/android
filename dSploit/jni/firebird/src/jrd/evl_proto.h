/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		evl_proto.h
 *	DESCRIPTION:	Prototype header file for evl.cpp
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

#ifndef JRD_EVL_PROTO_H
#define JRD_EVL_PROTO_H

#include "../jrd/intl_classes.h"

// Implemented in evl.cpp
void		EVL_adjust_text_descriptor(Jrd::thread_db*, dsc*);
dsc*		EVL_assign_to(Jrd::thread_db* tdbb, Jrd::jrd_nod*);
Jrd::RecordBitmap**	EVL_bitmap(Jrd::thread_db* tdbb, Jrd::jrd_nod*, Jrd::RecordBitmap*);
bool		EVL_boolean(Jrd::thread_db* tdbb, Jrd::jrd_nod*);
dsc*		EVL_expr(Jrd::thread_db* tdbb, Jrd::jrd_nod* const);
bool		EVL_field(Jrd::jrd_rel*, Jrd::Record*, USHORT, dsc*);
USHORT		EVL_group(Jrd::thread_db* tdbb, Jrd::RecordSource*, Jrd::jrd_nod* const, USHORT);
void		EVL_make_value(Jrd::thread_db* tdbb, const dsc*, Jrd::impure_value*);
void		EVL_validate(Jrd::thread_db*, const Jrd::Item&, const Jrd::ItemInfo*, dsc*, bool);


#endif // JRD_EVL_PROTO_H

