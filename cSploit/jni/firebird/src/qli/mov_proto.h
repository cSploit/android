/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		mov_proto.h
 *	DESCRIPTION:	Prototype header file for mov.cpp
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

#ifndef QLI_MOV_PROTO_H
#define QLI_MOV_PROTO_H

int			MOVQ_compare(const dsc*, const dsc*);
double		MOVQ_date_to_double(const dsc*);
int			MOVQ_decompose (const TEXT*, USHORT, SLONG *);
void		MOVQ_double_to_date (double, SLONG [2]);
double		MOVQ_get_double(const dsc*);
SLONG		MOVQ_get_long(const dsc*, SSHORT);
int			MOVQ_get_string(const dsc*, const TEXT**, vary*, USHORT);
void		MOVQ_move(const dsc*, dsc*);
void		MOVQ_terminate (const SCHAR*, SCHAR*, USHORT, USHORT);

#endif // QLI_MOV_PROTO_H

