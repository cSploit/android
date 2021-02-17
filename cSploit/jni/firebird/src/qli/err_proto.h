/*
 *	PROGRAM:	JRD Command Oriented Query Language
 *	MODULE:		err_proto.h
 *	DESCRIPTION:	Prototype header file for err.cpp
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

#ifndef QLI_ERR_PROTO_H
#define QLI_ERR_PROTO_H

#include "../common/classes/SafeArg.h"

void	ERRQ_bugcheck(USHORT);
void	ERRQ_database_error(qli_dbb*, ISC_STATUS*);
void	ERRQ_error(USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	ERRQ_error(USHORT, const char* str);
void	ERRQ_error_format(USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	ERRQ_exit (int);
void	ERRQ_msg_format(USHORT, USHORT, TEXT*, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
int		ERRQ_msg_get(USHORT, TEXT*, size_t s_size);
void	ERRQ_msg_partial (USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	ERRQ_msg_put (USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	ERRQ_msg_put(USHORT number, const char* str);
void	ERRQ_pending ();
void	ERRQ_print_error(USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	ERRQ_print_error(USHORT number, const char* str);
void	ERRQ_syntax (USHORT);

#endif // QLI_ERR_PROTO_H

