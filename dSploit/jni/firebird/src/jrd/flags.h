/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		flags.h
 *	DESCRIPTION:	Various flag definitions
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

#ifndef JRD_FLAGS_H
#define JRD_FLAGS_H

// flags for RDB$FILE_FLAGS

const USHORT FILE_shadow		= 1;
const USHORT FILE_inactive		= 2;
const USHORT FILE_manual		= 4;
//const USHORT FILE_cache		= 8;
const USHORT FILE_conditional 	= 16;
// Flags for backup difference files
// File is difference
const USHORT FILE_difference 	= 32;
// Actively used for backup purposes (ALTER DATABASE BEGIN BACKUP issued)
const USHORT FILE_backing_up	= 64;

// flags for RDB$LOG_FILES
/*
const USHORT LOG_serial			= 1;
const USHORT LOG_default		= 2;
const USHORT LOG_raw			= 4;
const USHORT LOG_overflow		= 8;
*/

// flags for RDB$RELATIONS

const USHORT REL_sql			= 0x0001;

// flags for RDB$TRIGGERS

const USHORT TRG_sql			= 0x1;
const USHORT TRG_ignore_perm	= 0x2;		// trigger ignores permissions checks

#endif // JRD_FLAGS_H
