/*
 *	PROGRAM:	JRD Access Method
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

#ifndef JRD_MOV_PROTO_H
#define JRD_MOV_PROTO_H

#include "../jrd/dsc.h"
#include "../jrd/jrd.h"
#include "../jrd/val.h"

struct dsc;
struct vary;

int		MOV_compare(const dsc*, const dsc*);
double	MOV_date_to_double(const dsc*);
void	MOV_double_to_date(double, SLONG[2]);
double	MOV_get_double(const dsc*);
SLONG	MOV_get_long(const dsc*, SSHORT);
void	MOV_get_metadata_str(const dsc*, TEXT*, USHORT);
void	MOV_get_name(const dsc*, TEXT*);
SQUAD	MOV_get_quad(const dsc*, SSHORT);
SINT64	MOV_get_int64(const dsc*, SSHORT);
int		MOV_get_string_ptr(const dsc*, USHORT*, UCHAR**, vary*,
							  USHORT);
int		MOV_get_string(const dsc*, UCHAR**, vary*, USHORT);
GDS_DATE	MOV_get_sql_date(const dsc*);
GDS_TIME	MOV_get_sql_time(const dsc*);
GDS_TIMESTAMP	MOV_get_timestamp(const dsc*);
int		MOV_make_string(const dsc*, USHORT, const char**, vary*, USHORT);
int		MOV_make_string2(Jrd::thread_db*, const dsc*, USHORT, UCHAR**, Jrd::MoveBuffer&, bool = true);
void	MOV_move(Jrd::thread_db*, /*const*/ dsc*, dsc*);

#endif // JRD_MOV_PROTO_H
