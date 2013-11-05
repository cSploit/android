/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pio.h
 *	DESCRIPTION:	File system interface definitions
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
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#ifndef JRD_PIO_H
#define JRD_PIO_H

#include "../include/fb_blk.h"
#include "../jrd/thread_proto.h"
#include "../common/classes/rwlock.h"
#include "../common/classes/array.h"
#include "../common/classes/File.h"

namespace Jrd {

#ifdef UNIX

class jrd_file : public pool_alloc_rpt<SCHAR, type_fil>
{
public:
	jrd_file*	fil_next;		// Next file in database
	ULONG fil_min_page;			// Minimum page number in file
	ULONG fil_max_page;			// Maximum page number in file
	USHORT fil_sequence;		// Sequence number of file
	USHORT fil_fudge;			// Fudge factor for page relocation
	int fil_desc;
	//int *fil_trace;			// Trace file, if any
	Firebird::Mutex fil_mutex;
	USHORT fil_flags;
	SCHAR fil_string[1];		// Expanded file name
};

#endif


#ifdef WIN_NT
#ifdef SUPERSERVER_V2
const int MAX_FILE_IO	= 32;	// Maximum "allocated" overlapped I/O events
#endif

class jrd_file : public pool_alloc_rpt<SCHAR, type_fil>
{
public:

	~jrd_file()
	{
		delete fil_ext_lock;
	}

	jrd_file*	fil_next;				// Next file in database
	ULONG fil_min_page;					// Minimum page number in file
	ULONG fil_max_page;					// Maximum page number in file
	USHORT fil_sequence;				// Sequence number of file
	USHORT fil_fudge;					// Fudge factor for page relocation
	HANDLE fil_desc;					// File descriptor
	//int *fil_trace;					// Trace file, if any
	Firebird::Mutex fil_mutex;
	Firebird::RWLock* fil_ext_lock;		// file extend lock
#ifdef SUPERSERVER_V2
	void* fil_io_events[MAX_FILE_IO];	// Overlapped I/O events
#endif
	USHORT fil_flags;
	SCHAR fil_string[1];				// Expanded file name
};

#endif


const USHORT FIL_force_write		= 1;
const USHORT FIL_no_fs_cache		= 2;	// not using file system cache
const USHORT FIL_readonly			= 4;	// file opened in readonly mode

// Physical IO trace events

const SSHORT trace_create	= 1;
const SSHORT trace_open		= 2;
const SSHORT trace_page_size	= 3;
const SSHORT trace_read		= 4;
const SSHORT trace_write	= 5;
const SSHORT trace_close	= 6;

// Physical I/O status block, used only in SS v2 for Win32

#ifdef SUPERSERVER_V2
struct phys_io_blk
{
	jrd_file* piob_file;		// File being read/written
	SLONG piob_desc;			// File descriptor
	SLONG piob_io_length;		// Requested I/O transfer length
	SLONG piob_actual_length;	// Actual I/O transfer length
	USHORT piob_wait;			// Async or synchronous wait
	UCHAR piob_flags;
	SLONG piob_io_event[8];		// Event to signal I/O completion
};

// piob_flags
const UCHAR PIOB_error		= 1;	// I/O error occurred
const UCHAR PIOB_success	= 2;	// I/O successfully completed
const UCHAR PIOB_pending	= 4;	// Asynchronous I/O not yet completed
#endif

} //namespace Jrd

#endif // JRD_PIO_H

