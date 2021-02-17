/*
 *	PROGRAM:	Interactive SQL utility
 *	MODULE:		iutils_proto.h
 *	DESCRIPTION:	Prototype header file for iutils.cpp
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

#ifndef ISQL_IUTILS_PROTO_H
#define ISQL_IUTILS_PROTO_H

#include "../common/classes/SafeArg.h"
#include <stdio.h>

#ifdef NOT_USED_OR_REPLACED
TEXT*	IUTILS_blankterm2(const TEXT* input, TEXT* output);
#endif
void	IUTILS_copy_SQL_id(const TEXT*, TEXT*, TEXT);
void	IUTILS_make_upper(TEXT*);
void	IUTILS_msg_get(USHORT number, TEXT* msg,
					 const MsgFormat::SafeArg& args = MsgFormat::SafeArg());
void	IUTILS_msg_get(USHORT number, USHORT size, TEXT* msg,
					 const MsgFormat::SafeArg& args = MsgFormat::SafeArg());
void	IUTILS_printf(FILE*, const char*);
void	IUTILS_printf2(FILE*, const char*, ...);
void	IUTILS_put_errmsg(USHORT number, const MsgFormat::SafeArg& args);
void	IUTILS_remove_and_unescape_quotes(TEXT* string, const char quote);
void	IUTILS_truncate_term(TEXT*, USHORT);

#endif // ISQL_IUTILS_PROTO_H
