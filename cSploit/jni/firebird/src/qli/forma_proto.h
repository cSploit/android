/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		forma_proto.h
 *	DESCRIPTION:	Prototype header file for format.cpp
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

#ifndef QLI_FORMA_PROTO_H
#define QLI_FORMA_PROTO_H

int			FMT_expression(qli_nod*);
TEXT*		FMT_format(qli_lls*);
qli_nod*	FMT_list(qli_nod*);
void		FMT_print(qli_nod*, qli_prt*);
void		FMT_put(const TEXT*, qli_prt*);
void		FMT_report(qli_rpt*);

#endif // QLI_FORMA_PROTO_H

