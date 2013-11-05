/*
 *	PROGRAM:	Windows NT installation utilities
 *	MODULE:		install_nt.h
 *	DESCRIPTION:	Defines for Windows NT installation utilities
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

#ifndef UTILITIES_INSTALL_NT_H
#define UTILITIES_INSTALL_NT_H

static const char* const REMOTE_SERVICE			= "FirebirdServer%s";
static const char* const REMOTE_DISPLAY_NAME	= "Firebird Server - %s";
static const char* const REMOTE_DISPLAY_DESCR	= "Firebird Database Server - www.firebirdsql.org";
static const char* const REMOTE_SS_EXECUTABLE	= "bin\\fbserver";
static const char* const REMOTE_CS_EXECUTABLE	= "bin\\fb_inet_server";

static const char* const ISCGUARD_SERVICE		= "FirebirdGuardian%s";
static const char* const ISCGUARD_DISPLAY_NAME	= "Firebird Guardian - %s";
static const char* const ISCGUARD_DISPLAY_DESCR	= "Firebird Server Guardian - www.firebirdsql.org";
static const char* const ISCGUARD_EXECUTABLE	= "bin\\fbguard";

static const char* const SERVER_MUTEX			= "FirebirdServerMutex%s";
static const char* const GUARDIAN_MUTEX			= "FirebirdGuardianMutex%s";

static const char* const FB_DEFAULT_INSTANCE	= "DefaultInstance";

// Starting with 128 the service params are user defined
const DWORD SERVICE_CREATE_GUARDIAN_MUTEX	= 128;
//#define REMOTE_DEPENDENCIES		"Tcpip\0\0"

// sw_command
const USHORT COMMAND_NONE		= 0;
const USHORT COMMAND_INSTALL	= 1;
const USHORT COMMAND_REMOVE		= 2;
const USHORT COMMAND_START		= 3;
const USHORT COMMAND_STOP		= 4;
//const USHORT COMMAND_CONFIG	= 5;
const USHORT COMMAND_QUERY		= 6;

// sw_startup
const USHORT STARTUP_DEMAND		= 0;
const USHORT STARTUP_AUTO		= 1;

// sw_guardian
const USHORT NO_GUARDIAN		= 0;
const USHORT USE_GUARDIAN		= 1;

// sw_mode
const USHORT DEFAULT_PRIORITY	= 0;
const USHORT NORMAL_PRIORITY	= 1;
const USHORT HIGH_PRIORITY		= 2;

// sw_arch
const USHORT ARCH_SS			= 0;
const USHORT ARCH_CS			= 1;
const USHORT ARCH_SCS			= 2;

// sw_client
const USHORT CLIENT_NONE		= 0;
const USHORT CLIENT_FB			= 1;
const USHORT CLIENT_GDS			= 2;

static const char* const GDS32_NAME			= "GDS32.DLL";
static const char* const FBCLIENT_NAME		= "FBCLIENT.DLL";

// instsvc status codes
const USHORT IB_SERVICE_ALREADY_DEFINED			= 100;
const USHORT IB_SERVICE_RUNNING					= 101;
const USHORT FB_PRIVILEGE_ALREADY_GRANTED		= 102;
const USHORT FB_SERVICE_STATUS_RUNNING			= 100;
const USHORT FB_SERVICE_STATUS_STOPPED			= 111;
const USHORT FB_SERVICE_STATUS_PENDING			= 112;
const USHORT FB_SERVICE_STATUS_NOT_INSTALLED	= 113;
const USHORT FB_SERVICE_STATUS_UNKNOWN			= 114;

// instclient status codes
const USHORT FB_INSTALL_COPY_REQUIRES_REBOOT	= 200;
const USHORT FB_INSTALL_SAME_VERSION_FOUND		= 201;
const USHORT FB_INSTALL_NEWER_VERSION_FOUND		= 202;
const USHORT FB_INSTALL_FILE_NOT_FOUND 			= 203;
const USHORT FB_INSTALL_CANT_REMOVE_ALIEN_VERSION	= 204;
const USHORT FB_INSTALL_FILE_PROBABLY_IN_USE	= 205;
const USHORT FB_INSTALL_SHARED_COUNT_ZERO  		= 206;

#endif // UTILITIES_INSTALL_NT_H
