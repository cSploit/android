/*
 *	PROGRAM:	Dynamic  SQL RUNTIME SUPPORT
 *	MODULE:		errd_proto.h
 *	DESCRIPTION:	Prototype Header file for errd.cpp
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

#ifndef DSQL_ERRD_PROTO_H
#define DSQL_ERRD_PROTO_H

#ifdef DEV_BUILD
void ERRD_assert_msg(const char*, const char*, ULONG);
#endif

void ERRD_bugcheck(const char*);
void ERRD_error(const char*);
void ERRD_post(const Firebird::Arg::StatusVector& v);
bool ERRD_post_warning(const Firebird::Arg::StatusVector& v);
void ERRD_punt(const ISC_STATUS* = 0);

#endif // DSQL_ERRD_PROTO_H

