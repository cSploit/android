/*
 *	PROGRAM:	JRD Backup and Restore program
 *	MODULE:		burp_proto.h
 *	DESCRIPTION:	Prototype header file for burp.cpp
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

#ifndef BURP_BURP_PROTO_H
#define BURP_BURP_PROTO_H

#include "../jrd/ThreadData.h"
#include "../common/classes/MsgPrint.h"
#include "../common/UtilSvc.h"

THREAD_ENTRY_DECLARE BURP_main(THREAD_ENTRY_PARAM);
int		gbak(Firebird::UtilSvc*);

void	BURP_abort();
void	BURP_error(USHORT, bool, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_error(USHORT, bool, const char* str);
void	BURP_error_redirect(const ISC_STATUS*, USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_msg_partial(bool, USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_msg_put(bool, USHORT, const MsgFormat::SafeArg& arg);
const int BURP_MSG_GET_SIZE = 128; // Use it for buffers passed to this function.
void	BURP_msg_get(USHORT, TEXT*, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_output_version(void*, const TEXT*);
void	BURP_print(bool err, USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_print(bool err, USHORT, const char* str);
void	BURP_print_status(bool err, const ISC_STATUS* status);
void	BURP_print_warning(const ISC_STATUS*);
void	BURP_verbose(USHORT, const MsgFormat::SafeArg& arg = MsgFormat::SafeArg());
void	BURP_verbose(USHORT, const char* str);

#endif	//  BURP_BURP_PROTO_H

