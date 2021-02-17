/*
 *	PROGRAM:		Firebird server manager
 *	MODULE:			ibmgr.h
 *	DESCRIPTION:	Header file for the FBMGR program
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
 */

#ifndef UTILITIES_IBMGR_H
#define UTILITIES_IBMGR_H

#define OUTFILE			stderr

#include "../jrd/constants.h"

static const char* const FIREBIRD_USER_NAME		= "firebird";
static const char* const INTERBASE_USER_NAME	= "interbase";
static const char* const INTERBASE_USER_SHORT	= "interbas";
static const char* const SERVER_GUARDIAN		= "fbguard";

const USHORT MSG_FAC	= 18;
const int MSG_LEN		= 128;


// Basic operation definitions
const USHORT OP_NONE		= 0;
const USHORT OP_START		= 1;
const USHORT OP_SHUT		= 2;
const USHORT OP_SET			= 3;
const USHORT OP_SHOW		= 4;
const USHORT OP_QUIT		= 5;
const USHORT OP_HELP		= 6;
const USHORT OP_VERSION		= 7;
const USHORT OP_PRINT		= 8;


// Suboperation definitions
const USHORT SOP_NONE			= 0;
const USHORT SOP_START_ONCE		= 1;
const USHORT SOP_START_FOREVER	= 2;
const USHORT SOP_SHUT_NOW		= 3;
const USHORT SOP_SHUT_NOAT		= 4;
const USHORT SOP_SHUT_NOTR		= 5;
const USHORT SOP_SHUT_IGN		= 6;
const USHORT SOP_PRINT_POOL		= 7;
const USHORT SOP_START_SIGNORE	= 8;
// Flags that are used in "par_entered" field of the IBMGR_DATA.
const USHORT ENT_HOST		= 0x1;
const USHORT ENT_PASSWORD	= 0x2;
const USHORT ENT_USER		= 0x4;


// Flags that are used in "reattach" field of the IBMGR_DATA.
// Note: "par_entered" field is cleared each time we are going
// to get a new command line, while the "reattach" field
// is cleared only after new attachment to service is made
const USHORT REA_HOST		= ENT_HOST;
const USHORT REA_PASSWORD	= ENT_PASSWORD;
const USHORT REA_USER		= ENT_USER;

// structure to hold all information
struct ibmgr_data_t
{
	USHORT		operation;				// what's to be done
	USHORT		suboperation;			// suboperation
	USHORT		par_entered;			// parameters that were entered
	TEXT		host[128];				// server's host
	TEXT		user[128];				// the user name
	TEXT		real_user[128];			// the os user name
	TEXT		password[32];			// user's passwd
	bool		shutdown;				// shutdown is in progress
	USHORT		reattach;				// need to reattach because host,
										// passwd or user has been changed
	isc_svc_handle	attached;			// !=NULL if we attached to service
	TEXT		print_file[MAXPATHLEN];	// Dump file name
	TEXT		pidfile[MAXPATHLEN];	// fbserver's PID file name
};


// Messages tag definitions

const USHORT MSG_PROMPT		= 1;	// "FBMGR> "  (the prompt)
const USHORT MSG_VERSION	= 2;	// fbmgr version

const USHORT MSG_OPSPEC		= 5;	// operation already specified
const USHORT MSG_NOOPSPEC	= 6;	// no operation specified
const USHORT MSG_INVSW		= 7;	// invalid switch
//const USHORT MSG_INVOP	= 8;	// invalid operation
const USHORT MSG_INVSWSW	= 9;	// invalid switch combination
const USHORT MSG_INVSWOP	= 10;	// invalid operation/switch combination
const USHORT MSG_AMBSW		= 11;	// ambiguous switch
const USHORT MSG_INVPAR		= 12;	// invalid parameter, no switch specified
const USHORT MSG_SWNOPAR	= 13;	// switch does not take any parameter
const USHORT MSG_SHUTDOWN	= 14;	// shutdown is in progress
const USHORT MSG_CANTSHUT	= 15;	// can not start another shutdown
const USHORT MSG_CANTQUIT	= 16;	// can not quit fbmgr now
const USHORT MSG_CANTCHANGE	= 17;	// can not change host, password or user
const USHORT MSG_WARNPASS	= 18;	// warning: only 8 significant bytes of password used
const USHORT MSG_INVUSER	= 19;	// invalid user (only 32 bytes allowed)
const USHORT MSG_REQPAR		= 20;	// switch requires parameter
const USHORT MSG_SYNTAX		= 21;	// syntax error in command line

const USHORT MSG_GETPWFAIL	= 25;	// can not get password entry
const USHORT MSG_ATTFAIL	= 26;	// can not attach to server
const USHORT MSG_SSHUTFAIL	= 27;	// can not start server shutdown
const USHORT MSG_SHUTOK		= 28;	// server shutdown completed
const USHORT MSG_STARTERR	= 29;	// can not start server
const USHORT MSG_STARTFAIL	= 30;	// can not start server
const USHORT MSG_SRVUP		= 31;	// server is alreary running
const USHORT MSG_SRVUPOK	= 32;	// server has been successfully started
const USHORT MSG_NOPERM		= 33;	// no permissions to perform operation
const USHORT MSG_PRPOOLFAIL	= 34;	// Failed to print pool info
const USHORT MSG_PRPOOLOK	= 35;	// Print pool successfull
const USHORT MSG_FLNMTOOLONG	= 36;	// File name too long


#endif // UTILITIES_IBMGR_H

