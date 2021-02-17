/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		gpre_proto.h
 *	DESCRIPTION:	Prototype header file for gpre.cpp
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

#ifndef GPRE_GPRE_PROTO_H
#define GPRE_GPRE_PROTO_H

#include "../gpre/parse.h"

void	CPR_abort();
#ifdef DEV_BUILD
void	CPR_assert(const TEXT*, int);
#endif
void	CPR_bugcheck(const TEXT*);
void	CPR_end_text(gpre_txt*);
int		CPR_error(const TEXT*);
void	CPR_exit(int);
void	CPR_warn(const TEXT*);
tok*	CPR_eol_token();
void	CPR_get_text(TEXT*, const gpre_txt*);

#ifdef NOT_USED_OR_REPLACED
void	CPR_raw_read();
#endif

void	CPR_s_error(const TEXT*);
gpre_txt*	CPR_start_text();
tok*	CPR_token();

#endif // GPRE_GPRE_PROTO_H

