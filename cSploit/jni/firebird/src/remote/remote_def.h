/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		remote_def.h
 *	DESCRIPTION:	Common descriptions
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - EPSON, XENIX, DELTA, IMP, NCR3000, M88K
 *                          - NT Power PC and HP9000 s300
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed support for obsolete IPX/SPX Protocol
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */

#ifndef REMOTE_REMOTE_DEF_H
#define REMOTE_REMOTE_DEF_H

#include "../remote/protocol.h"

#if defined(__sun)
#	ifdef sparc
const P_ARCH ARCHITECTURE	= arch_sun4;
#elif (defined i386 || defined AMD64)
const P_ARCH ARCHITECTURE	= arch_sunx86;
#	else
const P_ARCH ARCHITECTURE	= arch_sun;
#	endif
#elif defined(HPUX)
const P_ARCH ARCHITECTURE	= arch_hpux;
#elif (defined AIX || defined AIX_PPC)
const P_ARCH ARCHITECTURE	= arch_rt;
#elif defined(LINUX)
const P_ARCH ARCHITECTURE	= arch_linux;
#elif defined(FREEBSD)
const P_ARCH ARCHITECTURE	= arch_freebsd;
#elif defined(NETBSD)
const P_ARCH ARCHITECTURE	= arch_netbsd;
#elif defined(DARWIN) && defined(__ppc__)
const P_ARCH ARCHITECTURE	= arch_darwin_ppc;
#elif defined(WIN_NT) && defined(AMD64)
const P_ARCH ARCHITECTURE	= arch_winnt_64;
#elif defined(I386)
const P_ARCH ARCHITECTURE	= arch_intel_32;
#elif defined(DARWIN64)
const P_ARCH ARCHITECTURE	= arch_darwin_x64;
#elif defined(DARWINPPC64)
const P_ARCH ARCHITECTURE	= arch_darwin_ppc64;
#endif


// port_server_flags

const USHORT SRVR_server			= 1;	// server
const USHORT SRVR_multi_client		= 2;	// multi-client server
const USHORT SRVR_debug				= 4;	// debug run
const USHORT SRVR_inet				= 8;	// Inet protocol
const USHORT SRVR_wnet				= 16;	// Wnet (named pipe) protocol (WinNT)
const USHORT SRVR_xnet				= 32;	// Xnet protocol (Win32)
const USHORT SRVR_non_service		= 64;	// not running as an NT service
const USHORT SRVR_high_priority		= 128;	// fork off server at high priority
const USHORT SRVR_thread_per_port	= 256;	// bind thread to a port
const USHORT SRVR_no_icon			= 512;	// tell the server not to show the icon

#endif /* REMOTE_REMOTE_DEF_H */
