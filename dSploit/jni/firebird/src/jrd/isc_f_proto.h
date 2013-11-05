/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		isc_f_proto.h
 *	DESCRIPTION:	Prototype header file for isc_file.cpp
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
 *
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 *
 */

#ifndef JRD_ISC_FILE_PROTO_H
#define JRD_ISC_FILE_PROTO_H

#include "../common/classes/fb_string.h"

enum iscProtocol {ISC_PROTOCOL_LOCAL, ISC_PROTOCOL_TCPIP, ISC_PROTOCOL_WLAN};

#ifndef NO_NFS
bool		ISC_analyze_nfs(Firebird::PathName&, Firebird::PathName&);
#endif
bool		ISC_analyze_pclan(Firebird::PathName&, Firebird::PathName&);
bool		ISC_analyze_tcp(Firebird::PathName&, Firebird::PathName&);
bool		ISC_analyze_xnet(Firebird::PathName&, Firebird::PathName&);
bool		ISC_check_if_remote(const Firebird::PathName&, bool);
iscProtocol	ISC_extract_host(Firebird::PathName&, Firebird::PathName&, bool);
bool		ISC_expand_filename(Firebird::PathName&, bool);
void		ISC_systemToUtf8(Firebird::AbstractString& str);
void		ISC_utf8ToSystem(Firebird::AbstractString& str);
void		ISC_escape(Firebird::AbstractString& str);
void		ISC_unescape(Firebird::AbstractString& str);

// This form of ISC_expand_filename makes epp files happy
inline bool	ISC_expand_filename(const TEXT* unexpanded, USHORT len_unexpanded,
								TEXT* expanded, size_t len_expanded,
								bool expand_share)
{
	Firebird::PathName pn(unexpanded, len_unexpanded ? len_unexpanded : strlen(unexpanded));
	ISC_expand_filename(pn, expand_share);
	// What do I return here if the previous call returns false?
	return (pn.copyTo(expanded, len_expanded) != 0);
}

void		ISC_expand_share(Firebird::PathName&);
int			ISC_file_lock(SSHORT);
int			ISC_file_unlock(SSHORT);

#endif // JRD_ISC_FILE_PROTO_H
