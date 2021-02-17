/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		gener_proto.h
 *	DESCRIPTION:	Prototype header file for gener.cpp
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

#ifndef QLI_GENER_PROTO_H
#define QLI_GENER_PROTO_H

qli_nod*	GEN_generate(qli_nod*);
void		GEN_release();
qli_rlb*	GEN_rlb_extend(qli_rlb*);
void		GEN_rlb_release(qli_rlb*);

inline qli_rlb* CHECK_RLB(qli_rlb*& in)
{
	if (!in || (in->rlb_data > in->rlb_limit))
		in = GEN_rlb_extend(in);
	return in;
}

#endif // QLI_GENER_PROTO_H

