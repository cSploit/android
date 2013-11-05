/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		file_params.h
 *	DESCRIPTION:	File parameter definitions
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
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "EPSON" define*
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 * 2002.10.30 Sean Leyne - Code Cleanup, removed obsolete "SUN3_3" port
 *
 */

#ifndef JRD_FILE_PARAMS_H
#define JRD_FILE_PARAMS_H

static const char* const EVENT_FILE		= "fb_event_%s";
static const char* const LOCK_FILE		= "fb_lock_%s";
static const char* const MONITOR_FILE	= "fb_monitor_%s";
static const char* const TRACE_FILE		= "fb_trace";

#ifdef UNIX
static const char* const INIT_FILE		= "fb_init";
static const char* const SEM_FILE		= "fb_sem";
static const char* const PORT_FILE		= "fb_port_%d";
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_IPC_H
#include <sys/ipc.h>
#endif

#ifdef DARWIN
#undef FB_PREFIX
#define FB_PREFIX		"/all/files/are/in/framework/resources"
#define DARWIN_GEN_DIR		"var"
#define DARWIN_FRAMEWORK_ID	"com.firebirdsql.Firebird"
#endif

/* keep MSG_FILE_LANG in sync with build_file.epp */
#ifdef WIN_NT
static const char* const WORKFILE	= "c:\\temp\\";
static const char MSG_FILE_LANG[]	= "intl\\%.10s.msg";
#else
static const char* const WORKFILE	= "/tmp/";
static const char MSG_FILE_LANG[]	= "intl/%.10s.msg";
#endif

static const char* const LOCKDIR	= "firebird";		// created in WORKFILE
static const char* const LOGFILE	= "firebird.log";
static const char* const MSG_FILE	= "firebird.msg";
// Keep in sync with MSG_FILE_LANG
const int LOCALE_MAX	= 10;

#endif /* JRD_FILE_PARAMS_H */
