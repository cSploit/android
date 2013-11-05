/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_i_proto.h
 *	DESCRIPTION:	Prototype header file for isc_ipc.c
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

#ifndef JRD_ISC_I_PROTO_H
#define JRD_ISC_I_PROTO_H

#ifdef WIN_NT
// This will poke event
int		ISC_kill(SLONG, SLONG, void *);
#else
// And that are functions to manage UNIX signals
bool	ISC_signal(int, FPTR_VOID_PTR, void *);
void	ISC_signal_cancel(int, FPTR_VOID_PTR, void *);
#endif

#ifdef WIN_NT
void*	ISC_make_signal(bool, bool, int, int);
#endif

void	ISC_signal_init();

#endif // JRD_ISC_I_PROTO_H
