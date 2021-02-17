/*
 *	PROGRAM:	JRD Remote Interface/Server
 *	MODULE:		window.h
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
 */

#ifndef OS_WIN32_WINDOW_H
#define OS_WIN32_WINDOW_H

const int APP_HSIZE			= 220;
const int APP_VSIZE			= 150;

// This is the title used by the hidden msgs window in the engine.
// Both fbclient.dll and fbserver.exe should use the same.

static const char* const APP_NAME	= "Firebird Server";
static const char* const APP_LABEL	= "Firebird Server";

#define ON_NOTIFYICON		WM_USER + 2

// Moved to window.cpp
//#define CHECK_VOLUME(a)		((a)->dbcv_devicetype == DBT_DEVTYP_VOLUME)
//#define CHECK_USAGE(a)		((a)->dbcv_unitmask & ulInUseMask)

// This is the class name used by the hidden msgs window in the engine.
// Both fbclient.dll and fbserver.exe should use the same.

static const char* const szClassName	= "FB_Server";
static const char* const szWindowName	= "Firebird Server"; // Used in iscguard.cpp

// Defines to make 32-bit only compilers happy

#ifndef _WIN64
#ifndef DWLP_MSGRESULT
#define DWLP_MSGRESULT DWL_MSGRESULT
#endif
#ifndef GWLP_HINSTANCE
#define GWLP_HINSTANCE GWL_HINSTANCE
#endif
#ifndef GCLP_HICON
#define GCLP_HICON GCL_HICON
#endif
#ifndef LONG_PTR
#define LONG_PTR LONG
#endif
#ifndef UINT_PTR
#define UINT_PTR UINT
#endif
#ifndef GetWindowLongPtr
#define GetWindowLongPtr GetWindowLong
#endif
#ifndef SetWindowLongPtr
#define SetWindowLongPtr SetWindowLong
#endif
#ifndef GetClassLongPtr
#define GetClassLongPtr GetClassLong
#endif
#ifndef SetClassLongPtr
#define SetClassLongPtr SetClassLong
#endif
#endif // _WIN64

#endif // OS_WIN32_WINDOW_H
