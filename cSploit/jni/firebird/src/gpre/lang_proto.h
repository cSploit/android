/*
 *	PROGRAM:	Preprocessor
 *	MODULE:		lang_proto.h
 *	DESCRIPTION:	Prototype header file for all of the code generators
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
 * Modified to support RM/Cobol
 * Stephen W. Boyd 31.Aug.06
 */

#ifndef GPRE_LANG_PROTO_H
#define GPRE_LANG_PROTO_H

void	ADA_action(const act*, int);
void	ADA_print_buffer(TEXT*, const int);
//int		BAS_action(ACT, int);
void	C_CXX_action(const act*, int);
void	COB_action(const act*, int);
void	COB_name_init(bool);
void	COB_print_buffer(TEXT*, bool);
void	FTN_action(const act*, int);
void	FTN_fini();
void	FTN_print_buffer(TEXT*);
void	INT_action(const act*, int);
void	INT_CXX_action(const act*, int);
void	OBJ_CXX_action(const act*, int);
void	PAS_action(const act*, int);
//int		PLI_action(ACT, int);
void	RMC_action(const act*, int);
void	RMC_print_buffer(TEXT*, bool);

#endif // GPRE_LANG_PROTO_H

