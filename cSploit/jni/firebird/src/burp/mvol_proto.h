/*
 *	PROGRAM:	JRD Backup and Restore Program
 *	MODULE:		mvol_proto.h
 *	DESCRIPTION:	Prototype Header file for mvol.cpp
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

#ifndef BURP_MVOL_PROTO_H
#define BURP_MVOL_PROTO_H

#include "../burp/burp.h"


FB_UINT64		MVOL_fini_read();
FB_UINT64		MVOL_fini_write(int*, UCHAR**);
void			MVOL_init(ULONG);
void			MVOL_init_read(const char*, USHORT*, int*, UCHAR**);
void			MVOL_init_write(const char*, int*, UCHAR**);
bool			MVOL_split_hdr_write();
bool			MVOL_split_hdr_read();
int				MVOL_read(int*, UCHAR**);
UCHAR*			MVOL_read_block(BurpGlobals*, UCHAR*, ULONG);
void			MVOL_skip_block(BurpGlobals*, ULONG);
UCHAR			MVOL_write(const UCHAR, int*, UCHAR**);
const UCHAR*	MVOL_write_block(BurpGlobals*, const UCHAR*, ULONG);

#if defined WIN_NT
DESC			MVOL_open(const char*, ULONG, ULONG);
#endif


#endif	// BURP_MVOL_PROTO_H

