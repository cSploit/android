/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		dmp_proto.h
 *	DESCRIPTION:	Prototype header file for dmp.cpp
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

#ifndef JRD_DMP_PROTO_H
#define JRD_DMP_PROTO_H

namespace Ods {
	struct pag;
}

void DMP_active();
void DMP_btc();
void DMP_btc_errors();
void DMP_btc_ordered();
void DMP_dirty();
void DMP_page(SLONG, USHORT);
void DMP_fetched_page(const struct Ods::pag*, ULONG, ULONG, USHORT);

#endif // JRD_DMP_PROTO_H

