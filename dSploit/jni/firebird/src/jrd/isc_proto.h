/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_proto.h
 *	DESCRIPTION:	Prototype header file for isc.cpp
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

#ifndef JRD_ISC_PROTO_H
#define JRD_ISC_PROTO_H

#include "../common/classes/fb_string.h"

bool	ISC_check_process_existence(SLONG);
TEXT*	ISC_get_host(TEXT *, USHORT);
const TEXT*	ISC_get_host(Firebird::string&);
bool	ISC_get_user(Firebird::string*, int*, int*, const TEXT*);
SLONG	ISC_set_prefix(const TEXT*, const TEXT*);

// Does not add word "Database" in the beginning like gds__log_status
void	iscLogStatus(const TEXT* text, const ISC_STATUS* status_vector);
void	iscLogException(const TEXT* text, const Firebird::Exception& e);

#ifdef WIN9X_SUPPORT
bool	ISC_is_WinNT();
#endif

#ifdef WIN_NT
struct _SECURITY_ATTRIBUTES* ISC_get_security_desc();
#endif

#endif // JRD_ISC_PROTO_H
