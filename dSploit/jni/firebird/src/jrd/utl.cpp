/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		utl.cpp
 *	DESCRIPTION:	User callable routines
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
 * 2001.06.14 Claudio Valderrama: isc_set_path() will append slash if missing,
 *            so ISC_PATH environment variable won't fail for this cause.
 * 2002.02.15 Sean Leyne - Code Cleanup is required of obsolete "EPSON", "XENIX" ports
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "Apollo" port
 * 23-Feb-2002 Dmitry Yemanov - Events wildcarding
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix/MIPS" port
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2002.10.30 Sean Leyne - Removed support for obsolete "PC_PLATFORM" define
 *
 */

#include "firebird.h"
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../jrd/common.h"
#include "../jrd/license.h"
#include <stdarg.h>
//#include "../common/classes/timestamp.h"
#include "../jrd/gdsassert.h"

#include "../jrd/ibase.h"
#include "../jrd/msg.h"
#include "../jrd/event.h"
#include "../jrd/gds_proto.h"
#include "../jrd/utl_proto.h"
#include "../jrd/constants.h"
#include "../common/classes/ClumpletWriter.h"
#include "../common/utils_proto.h"
#include "../common/classes/MetaName.h"
#include "../common/classes/TempFile.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/types.h>
#include <sys/stat.h>

#if defined(WIN_NT)
#include <io.h> // mktemp, unlink ..
#include <process.h>
#endif

#ifdef HAVE_SYS_FILE_H
#include <sys/file.h>
#endif

// Bug 7119 - BLOB_load will open external file for read in BINARY mode.

#ifdef WIN_NT
static const char* const FOPEN_READ_TYPE		= "rb";
static const char* const FOPEN_WRITE_TYPE		= "wb";
static const char* const FOPEN_READ_TYPE_TEXT	= "rt";
static const char* const FOPEN_WRITE_TYPE_TEXT	= "wt";
#else
static const char* const FOPEN_READ_TYPE		= "r";
static const char* const FOPEN_WRITE_TYPE		= "w";
static const char* const FOPEN_READ_TYPE_TEXT	= FOPEN_READ_TYPE;
static const char* const FOPEN_WRITE_TYPE_TEXT	= FOPEN_WRITE_TYPE;
#endif

#define LOWER7(c) ( (c >= 'A' && c<= 'Z') ? c + 'a' - 'A': c )


// Blob stream stuff

const int BSTR_input	= 0;
const int BSTR_output	= 1;
const int BSTR_alloc	= 2;

static int dump(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, FILE*);
static int edit(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, SSHORT, const SCHAR*);
static int get_ods_version(FB_API_HANDLE*, USHORT*, USHORT*);
static void isc_expand_dpb_internal(const UCHAR** dpb, SSHORT* dpb_size, ...);
static int load(ISC_QUAD*, FB_API_HANDLE, FB_API_HANDLE, FILE*);


// Blob info stuff

static const char blob_items[] =
{
	isc_info_blob_max_segment, isc_info_blob_num_segments,
	isc_info_blob_total_length
};


// gds__version stuff

static const char info[] =
	{ isc_info_firebird_version, isc_info_implementation, isc_info_end };

static const char ods_info[] =
	{ isc_info_ods_version, isc_info_ods_minor_version, isc_info_end };

static const TEXT* const impl_class[] =
{
	NULL,						// 0
	"access method",			// 1
	"Y-valve",					// 2
	"remote interface",			// 3
	"remote server",			// 4
	NULL,						// 5
	NULL,						// 6
	"pipe interface",			// 7
	"pipe server",				// 8
	"central interface",		// 9
	"central server",			// 10
	"gateway",					// 11
	"classic server",			// 12
	"super server"				// 13
};

static const TEXT* const impl_implementation[] =
{
	NULL,							// 0
	"Rdb/VMS",						// 1
	"Rdb/ELN target",				// 2
	"Rdb/ELN development",			// 3
	"Rdb/VMS Y",					// 4
	"Rdb/ELN Y",					// 5
	"JRI",							// 6
	"JSV",							// 7
	NULL,							// 8
	NULL,							// 9
	NULL, // "Firebird/apollo",		// 10
	NULL, // "Firebird/ultrix",		// 11
	"Firebird/vms",					// 12
	"Firebird/sun",					// 13
	NULL, // "Firebird/OS2",		// 14
	NULL,							// 15
	NULL,							// 16
	NULL,							// 17
	NULL,							// 18
	NULL,							// 19
	NULL,							// 20
	NULL,							// 21
	NULL,							// 22
	NULL,							// 23
	NULL,							// 24
	NULL, // "Firebird/apollo",		// 25
	NULL, // "Firebird/ultrix",		// 26
	"Firebird/vms",					// 27
	"Firebird/sun",					// 28
	NULL, // "Firebird/OS2",		// 29
	"Firebird/sun4",				// 30
	"Firebird/hpux",				// 31
	"Firebird/sun386",				// 32
	"Firebird:ORACLE/vms",			// 33
	NULL, // "Firebird/mac/aux",	// 34
	"Firebird/ibm/aix",				// 35
	NULL, // "Firebird/mips/ultrix",	// 36
	NULL, // "Firebird/xenix",		// 37
	NULL, // "Firebird/AViiON",		// 38
	NULL, // "Firebird/hp/mpexl",	// 39
	NULL, // "Firebird/hp/ux300",	// 40
	NULL, // "Firebird/sgi",		// 41
	"Firebird/sco/unix",			// 42
	NULL, // "Firebird/Cray",		// 43
	NULL, // "Firebird/imp",		// 44
	NULL, // "Firebird/delta",		// 45
	NULL, // "Firebird/NeXT",		// 46
	NULL, // "Firebird/DOS",		// 47
	NULL, // "Firebird/m88k",		// 48
	NULL, // "Firebird/UNIXWARE",	// 49
	"Firebird/x86/Windows NT",		// 50
	NULL, // "Firebird/epson",		// 51
	NULL, // "Firebird/DEC/OSF",	// 52
	"Firebird/Alpha/OpenVMS",		// 53
	NULL, // "Firebird/NetWare",	// 54
	"Firebird/Windows",				// 55
	NULL, // "Firebird/NCR3000",	// 56
	NULL, // "Firebird/PPC/Windows NT", // 57
	NULL, // "Firebird/DG_X86",		// 58
	"Firebird/SCO_SV Intel",		// 59 // 5.5 SCO Port
	"Firebird/linux Intel",			// 60
	"Firebird/FreeBSD/i386",		// 61
	"Firebird/NetBSD/i386",			// 62
	"Firebird/Darwin/PowerPC",		// 63
	"Firebird/SINIX-Z",				// 64
	"Firebird/linux Sparc",			// 65
	"Firebird/linux AMD64",			// 66
	"Firebird/FreeBSD/amd64",		// 67
	"Firebird/x86-64/Windows NT",	// 68
	"Firebird/linux PowerPC",		// 69
	"Firebird/Darwin/Intel",		// 70
	"Firebird/linux MIPSEL",		// 71
	"Firebird/linux MIPS",			// 72
	"Firebird/Darwin/Intel64",		// 73
	"Firebird/sun/amd64",			// 74
	"Firebird/linux ARM",			// 75
	"Firebird/linux IA64",			// 76
	"Firebird/Darwin/PowerPC64",	// 77
	"Firebird/linux s390x",			// 78
	"Firebird/linux s390",			// 79
	"Firebird/linux SH",			// 80
	"Firebird/linux SHEB",			// 81
	"Firebird/linux HPPA",			// 82
	"Firebird/linux ALPHA"			// 83
};


#if (defined SOLARIS ) || (defined __cplusplus)
extern "C" {
#endif
// Avoid C++ linkage API functions


int API_ROUTINE gds__blob_size(FB_API_HANDLE* b, SLONG* size, SLONG* seg_count, SLONG* max_seg)
{
/**************************************
 *
 *	g d s _ $ b l o b _ s i z e
 *
 **************************************
 *
 * Functional description
 *	Get the size, number of segments, and max
 *	segment length of a blob.  Return TRUE
 *	if it happens to succeed.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	SCHAR buffer[64];

	if (isc_blob_info(status_vector, b, sizeof(blob_items), blob_items, sizeof(buffer), buffer))
	{
		isc_print_status(status_vector);
		return FALSE;
	}

	const UCHAR* p = reinterpret_cast<UCHAR*>(buffer);
	UCHAR item;
	while ((item = *p++) != isc_info_end)
	{
		const USHORT l = gds__vax_integer(p, 2);
		p += 2;
		const SLONG n = gds__vax_integer(p, l);
		p += l;
		switch (item)
		{
		case isc_info_blob_max_segment:
			if (max_seg)
				*max_seg = n;
			break;

		case isc_info_blob_num_segments:
			if (seg_count)
				*seg_count = n;
			break;

		case isc_info_blob_total_length:
			if (size)
				*size = n;
			break;

		default:
			return FALSE;
		}
	}

	return TRUE;
}


// 17 May 2001 - isc_expand_dpb is DEPRECATED
void API_ROUTINE_VARARG isc_expand_dpb(SCHAR** dpb, SSHORT* dpb_size, ...)
{
/**************************************
 *
 *	i s c _ e x p a n d _ d p b
 *
 **************************************
 *
 * Functional description
 *	Extend a database parameter block dynamically
 *	to include runtime info.  Generated
 *	by gpre to provide host variable support for
 *	READY statement	options.
 *	This expects the list of variable args
 *	to be zero terminated.
 *
 *	Note: dpb_size is signed short only for compatibility
 *	with other calls (isc_attach_database) that take a dpb length.
 *
 * TMN: Note: According to Ann Harrison:
 * That routine should be deprecated.  It doesn't do what it
 * should, and does do things it shouldn't, and is harder to
 * use than the natural alternative.
 *
 **************************************/
	SSHORT length;
	UCHAR* p = NULL;
	const char*	q;
	va_list	args;
	USHORT type;
	UCHAR* new_dpb;

	// calculate length of database parameter block, setting initial length to include version

	SSHORT new_dpb_length;
	if (!*dpb || !(new_dpb_length = *dpb_size))
	{
		new_dpb_length = 1;
	}

	va_start(args, dpb_size);

	while (type = va_arg(args, int))
	{
		switch (type)
		{
		case isc_dpb_user_name:
		case isc_dpb_password:
		case isc_dpb_sql_role_name:
		case isc_dpb_lc_messages:
		case isc_dpb_lc_ctype:
		case isc_dpb_reserved:
			q = va_arg(args, char*);
			if (q)
			{
				length = strlen(q);
				new_dpb_length += 2 + length;
			}
			break;

		default:
			va_arg(args, int);
			break;
		}
	}
	va_end(args);

	// if items have been added, allocate space
	// for the new dpb and copy the old one over

	if (new_dpb_length > *dpb_size)
	{
		// Note: gds__free done by GPRE generated code

		new_dpb = (UCHAR*) gds__alloc((SLONG)(sizeof(UCHAR) * new_dpb_length));
		p = new_dpb;
		// FREE: done by client process in GPRE generated code
		if (!new_dpb)
		{
			// NOMEM: don't trash existing dpb
			DEV_REPORT("isc_extend_dpb: out of memory");
			return;				// NOMEM: not really handled
		}

		q = *dpb;
		for (length = *dpb_size; length; length--)
		{
			*p++ = *q++;
		}

	}
	else
	{
		// CVC: Notice this case is new_dpb_length <= *dpb_size, but since
		// we have new_dpb_length = MAX(*dpb_size, 1) our case is reduced
		// to new_dpb_length == *dpb_size. Therefore, this code is a waste
		// of time, since the function didn't find any param to add and thus,
		// the loop below won't find anything worth adding either.
		// Notice, too that the original input dpb is used, yet the pointer "p"
		// is positioned exactly at the end, so if something was added at the
		// tail, it would be a memory failure, unless the caller lies and is
		// always passing a dpb bigger than *dpb_size.
		new_dpb = reinterpret_cast<UCHAR*>(*dpb);
		p = new_dpb + *dpb_size;
	}

	if (!*dpb_size)
		*p++ = isc_dpb_version1;

	// copy in the new runtime items

	va_start(args, dpb_size);

	while (type = va_arg(args, int))
	{
		switch (type)
		{
		case isc_dpb_user_name:
		case isc_dpb_password:
		case isc_dpb_sql_role_name:
		case isc_dpb_lc_messages:
		case isc_dpb_lc_ctype:
		case isc_dpb_reserved:
			q = va_arg(args, char*);
			if (q)
			{
				length = strlen(q);
				fb_assert(type <= CHAR_MAX);
				*p++ = (UCHAR) type;
				fb_assert(length <= CHAR_MAX);
				*p++ = (UCHAR) length;
				while (length--)
					*p++ = *q++;
			}
			break;

		default:
			va_arg(args, int);
			break;
		}
	}
	va_end(args);

	*dpb_size = p - new_dpb;
	*dpb = reinterpret_cast<SCHAR*>(new_dpb);
}


int API_ROUTINE isc_modify_dpb(SCHAR**	dpb,
							   SSHORT*	dpb_size,
							   USHORT	type,
							   const SCHAR*	str,
							   SSHORT	str_len)
{
/**************************************
 *
 *	i s c _ m o d i f y _ d p b
 *
 **************************************
 * CVC: This is exactly the same logic as isc_expand_dpb, but for one param.
 * However, the difference is that when presented with a dpb type it that's
 * unknown, it returns FB_FAILURE immediately. In contrast, isc_expand_dpb
 * doesn't complain and instead treats those as integers and tries to skip
 * them, hoping to sync in the next iteration.
 *
 * Functional description
 *	Extend a database parameter block dynamically
 *	to include runtime info.  Generated
 *	by gpre to provide host variable support for
 *	READY statement	options.
 *	This expects one arg at a time.
 *      the length of the string is passed by the caller and hence
 * 	is not expected to be null terminated.
 * 	this call is a variation of isc_expand_dpb without a variable
 * 	arg parameters.
 * 	Instead, this function is called recursively
 *	Alternatively, this can have a parameter list with all possible
 *	parameters either nulled or with proper value and type.
 *
 *  	**** This can be modified to be so at a later date, making sure
 *	**** all callers follow the same convention
 *
 *	Note: dpb_size is signed short only for compatibility
 *	with other calls (isc_attach_database) that take a dpb length.
 *
 **************************************/

	// calculate length of database parameter block, setting initial length to include version

	SSHORT new_dpb_length;
	if (!*dpb || !(new_dpb_length = *dpb_size))
	{
		new_dpb_length = 1;
	}

	switch (type)
	{
	case isc_dpb_user_name:
	case isc_dpb_password:
	case isc_dpb_sql_role_name:
	case isc_dpb_lc_messages:
	case isc_dpb_lc_ctype:
	case isc_dpb_reserved:
		new_dpb_length += 2 + str_len;
		break;

	default:
		return FB_FAILURE;
	}

	// if items have been added, allocate space
	// for the new dpb and copy the old one over

	UCHAR* new_dpb;
	if (new_dpb_length > *dpb_size)
	{
		// Note: gds__free done by GPRE generated code

		new_dpb = (UCHAR*) gds__alloc((SLONG)(sizeof(UCHAR) * new_dpb_length));

		// FREE: done by client process in GPRE generated code
		if (!new_dpb)
		{
			// NOMEM: don't trash existing dpb
			DEV_REPORT("isc_extend_dpb: out of memory");
			return FB_FAILURE;		// NOMEM: not really handled
		}

		memcpy(new_dpb, *dpb, *dpb_size);
	}
	else
		new_dpb = reinterpret_cast<UCHAR*>(*dpb);

	UCHAR* p = new_dpb + *dpb_size;

	if (!*dpb_size)
	{
		*p++ = isc_dpb_version1;
	}

	// copy in the new runtime items

	switch (type)
	{
	case isc_dpb_user_name:
	case isc_dpb_password:
	case isc_dpb_sql_role_name:
	case isc_dpb_lc_messages:
	case isc_dpb_lc_ctype:
	case isc_dpb_reserved:
		{
			const UCHAR* q = reinterpret_cast<const UCHAR*>(str);
			if (q)
			{
				SSHORT length = str_len;
				fb_assert(type <= MAX_UCHAR);
				*p++ = (UCHAR) type;
				fb_assert(length <= MAX_UCHAR);
				*p++ = (UCHAR) length;
				while (length--)
				{
					*p++ = *q++;
				}
			}
			break;
		}

	default:
		return FB_FAILURE;
	}

	*dpb_size = p - new_dpb;
	*dpb = (SCHAR*) new_dpb;

	return FB_SUCCESS;
}


#if defined(UNIX) || defined(WIN_NT)
int API_ROUTINE gds__edit(const TEXT* file_name, USHORT /*type*/)
{
/**************************************
 *
 *	g d s _ $ e d i t
 *
 **************************************
 *
 * Functional description
 *	Edit a file.
 *
 **************************************/
	Firebird::string editor;

#ifndef WIN_NT
	if (!fb_utils::readenv("VISUAL", editor) && !fb_utils::readenv("EDITOR", editor))
		editor = "vi";
#else
	if (!fb_utils::readenv("EDITOR", editor))
		editor = "Notepad";
#endif

	struct stat before;
	stat(file_name, &before);
	// The path of the editor + the path of the file + quotes + one space.
	// We aren't using quotes around the editor for now.
	TEXT buffer[MAXPATHLEN * 2 + 5];
	fb_utils::snprintf(buffer, sizeof(buffer), "%s \"%s\"", editor.c_str(), file_name);

	system(buffer);

	struct stat after;
	stat(file_name, &after);

	return (before.st_mtime != after.st_mtime || before.st_size != after.st_size);
}
#endif


SLONG API_ROUTINE gds__event_block(UCHAR** event_buffer, UCHAR** result_buffer, USHORT count, ...)
{
/**************************************
 *
 *	g d s _ $ e v e n t _ b l o c k
 *
 **************************************
 *
 * Functional description
 *	Create an initialized event parameter block from a
 *	variable number of input arguments.
 *	Return the size of the block.
 *
 *	Return 0 as the size if the event parameter block cannot be
 *	created for any reason.
 *
 **************************************/
	UCHAR* p;
	SCHAR* q;
	SLONG length;
	va_list ptr;
	USHORT i;

	va_start(ptr, count);

	// calculate length of event parameter block,
	// setting initial length to include version
	// and counts for each argument

	length = 1;
	i = count;
	while (i--)
	{
		q = va_arg(ptr, SCHAR*);
		length += strlen(q) + 5;
	}

	p = *event_buffer = (UCHAR*) gds__alloc((SLONG) (sizeof(UCHAR) * length));
	// FREE: unknown
	if (!*event_buffer)			// NOMEM:
		return 0;
	*result_buffer = (UCHAR*) gds__alloc((SLONG) (sizeof(UCHAR) * length));
	// FREE: unknown
	if (!*result_buffer)
	{
		// NOMEM:
		gds__free(*event_buffer);
		*event_buffer = NULL;
		return 0;
	}

#ifdef DEBUG_GDS_ALLOC
	// I can't find anywhere these items are freed
	// 1994-October-25 David Schnepper
	gds_alloc_flag_unfreed((void*) *event_buffer);
	gds_alloc_flag_unfreed((void*) *result_buffer);
#endif // DEBUG_GDS_ALLOC

	// initialize the block with event names and counts

	*p++ = EPB_version1;

	va_start(ptr, count);

	i = count;
	while (i--)
	{
		q = va_arg(ptr, SCHAR*);

		// Strip trailing blanks from string

		const SCHAR* end = q + strlen(q);
		while (--end >= q && *end == ' ')
			; // empty loop
		*p++ = end - q + 1;
		while (q <= end)
			*p++ = *q++;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}

	return p - *event_buffer;
}


USHORT API_ROUTINE gds__event_block_a(SCHAR** event_buffer,
									  SCHAR** result_buffer,
									  SSHORT count,
									  SCHAR** name_buffer)
{
/**************************************
 *
 *	g d s _ $ e v e n t _ b l o c k _ a
 *
 **************************************
 *
 * Functional description
 *	Create an initialized event parameter block from a
 *	vector of input arguments. (Ada needs this)
 *	Assume all strings are 31 characters long.
 *	Return the size of the block.
 *
 **************************************/
	const int MAX_NAME_LENGTH = 31;
	// calculate length of event parameter block,
	// setting initial length to include version
	// and counts for each argument

	USHORT i = count;
	const SCHAR* const* nb = name_buffer;
	SLONG length = 0;
	while (i--)
	{
		const SCHAR* q = *nb++;

		// Strip trailing blanks from string
		const SCHAR* end = q + MAX_NAME_LENGTH;
		while (--end >= q && *end == ' '); // null body
		length += end - q + 1 + 5;
	}

	i = count;
	SCHAR* p = *event_buffer = (SCHAR*) gds__alloc((SLONG) (sizeof(SCHAR) * length));
	// FREE: unknown
	if (!*event_buffer)			// NOMEM:
		return 0;
	*result_buffer = (SCHAR*) gds__alloc((SLONG) (sizeof(SCHAR) * length));
	// FREE: unknown
	if (!*result_buffer)
	{
		// NOMEM:
		gds__free(*event_buffer);
		*event_buffer = NULL;
		return 0;
	}

#ifdef DEBUG_GDS_ALLOC
	// I can't find anywhere these items are freed
	// 1994-October-25 David Schnepper
	gds_alloc_flag_unfreed((void*) *event_buffer);
	gds_alloc_flag_unfreed((void*) *result_buffer);
#endif // DEBUG_GDS_ALLOC

	*p++ = EPB_version1;

	nb = name_buffer;

	while (i--)
	{
		const SCHAR* q = *nb++;

		// Strip trailing blanks from string
		const SCHAR* end = q + MAX_NAME_LENGTH;
		while (--end >= q && *end == ' ')
			; // null body
		*p++ = end - q + 1;
		while (q <= end)
			*p++ = *q++;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
		*p++ = 0;
	}

	return (p - *event_buffer);
}


void API_ROUTINE gds__event_block_s(SCHAR** event_buffer,
									SCHAR** result_buffer,
									SSHORT count,
									SCHAR** name_buffer,
									SSHORT* return_count)
{
/**************************************
 *
 *	g d s _ $ e v e n t _ b l o c k _ s
 *
 **************************************
 *
 * Functional description
 *	THIS IS THE COBOL VERSION of gds__event_block_a for Cobols
 *	that don't permit return values.
 *
 **************************************/

	*return_count = gds__event_block_a(event_buffer, result_buffer, count, name_buffer);
}


void API_ROUTINE isc_event_counts(ULONG* result_vector,
								  SSHORT buffer_length,
								  UCHAR* event_buffer,
								  const UCHAR* result_buffer)
{
/**************************************
 *
 *	g d s _ $ e v e n t _ c o u n t s
 *
 **************************************
 *
 * Functional description
 *	Get the delta between two events in an event
 *	parameter block.  Used to update gds_events
 *	for GPRE support of events.
 *
 **************************************/
	ULONG* vec = result_vector;
	const UCHAR* p = event_buffer;
	const UCHAR* q = result_buffer;
	USHORT length = buffer_length;
	const UCHAR* const end = p + length;

	// analyze the event blocks, getting the delta for each event

	p++;
	q++;
	while (p < end)
	{
		// skip over the event name

		const USHORT i = (USHORT)* p++;
		p += i;
		q += i + 1;

		// get the change in count

		const ULONG initial_count = gds__vax_integer(p, sizeof(SLONG));
		p += sizeof(SLONG);
		const ULONG new_count = gds__vax_integer(q, sizeof(SLONG));
		q += sizeof(SLONG);
		*vec++ = new_count - initial_count;
	}

	// copy over the result to the initial block to prepare
	// for the next call to gds__event_wait

	memcpy(event_buffer, result_buffer, length);
}


void API_ROUTINE isc_get_client_version(SCHAR* buffer)
{
/**************************************
 *
 *	g d s _ $ g e t _ c l i e n t _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	if (buffer)
		strcpy(buffer, ISC_VERSION);
}


int API_ROUTINE isc_get_client_major_version()
{
/**************************************
 *
 *	g d s _ $ g e t _ c l i e n t _ m a j o r _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return atoi(ISC_MAJOR_VER);
}


int API_ROUTINE isc_get_client_minor_version()
{
/**************************************
 *
 *	g d s _ $ g e t _ c l i e n t _ m i n o r _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *
 **************************************/

	return atoi(ISC_MINOR_VER);
}


void API_ROUTINE gds__map_blobs(int* /*handle1*/, int* /*handle2*/)
{
/**************************************
 *
 *	g d s _ $ m a p _ b l o b s
 *
 **************************************
 *
 * Functional description
 *	Deprecated API function.
 *
 **************************************/
}


void API_ROUTINE isc_set_debug(int /*value*/)
{
/**************************************
 *
 *	G D S _ S E T _ D E B U G
 *
 **************************************
 *
 * Functional description
 *	Set debugging mode, whatever that may mean.
 *
 **************************************/

#pragma FB_COMPILER_MESSAGE("Empty function?!")
}


void API_ROUTINE isc_set_login(const UCHAR** dpb, SSHORT* dpb_size)
{
/**************************************
 *
 *	i s c _ s e t _ l o g i n
 *
 **************************************
 *
 * Functional description
 *	Pickup any ISC_USER and ISC_PASSWORD
 *	environment variables, and stuff them
 *	into the dpb (if there is no user name
 *	or password already referenced).
 *
 **************************************/
#ifndef SUPERSERVER

	// look for the environment variables

	Firebird::string username, password;
	if (!fb_utils::readenv(ISC_USER, username) && !fb_utils::readenv(ISC_PASSWORD, password))
		return;

	// figure out whether the username or password have already been specified

	bool user_seen = false, password_seen = false;

	if (*dpb && *dpb_size)
	{
	    const UCHAR* p = *dpb;
		for (const UCHAR* const end_dpb = p + *dpb_size; p < end_dpb;)
		{
			const int item = *p++;
			switch (item)
			{
			case isc_dpb_version1:
				continue;

			case isc_dpb_sys_user_name:
			case isc_dpb_user_name:
				user_seen = true;
				break;

			case isc_dpb_password:
			case isc_dpb_password_enc:
				password_seen = true;
				break;
			}

			// get the length and increment past the parameter.
			const USHORT l = *p++;
			p += l;
		}
	}

	if (username.length() && !user_seen)
	{
		if (password.length() && !password_seen)
			isc_expand_dpb_internal(dpb, dpb_size, isc_dpb_user_name, username.c_str(),
									isc_dpb_password, password.c_str(), 0);
		else
			isc_expand_dpb_internal(dpb, dpb_size, isc_dpb_user_name, username.c_str(), 0);
	}
	else if (password.length() && !password_seen)
		isc_expand_dpb_internal(dpb, dpb_size, isc_dpb_password, password.c_str(), 0);
#endif
}


void API_ROUTINE isc_set_single_user(const UCHAR** dpb, SSHORT* dpb_size, const TEXT* single_user)
{
/****************************************
 *
 *      i s c _ s e t _ s i n g l e _ u s e r
 *
 ****************************************
 *
 * Functional description
 *      Stuff the single_user flag into the dpb
 *      if the flag doesn't already exist in the dpb.
 *
 ****************************************/

	// Discover if single user access has already been specified

	bool single_user_seen = false;

	if (*dpb && *dpb_size)
	{
		const UCHAR* p = *dpb;
		for (const UCHAR* const end_dpb = p + *dpb_size; p < end_dpb;)
		{
			const int item = *p++;
			switch (item)
			{
			case isc_dpb_version1:
				continue;
			case isc_dpb_reserved:
				single_user_seen = true;
				break;
			}

			// Get the length and increment past the parameter.

			const USHORT l = *p++;
			p += l;

		}
	}

	if (!single_user_seen)
		isc_expand_dpb_internal(dpb, dpb_size, isc_dpb_reserved, single_user, 0);

}

static void print_version(void*, const char* version)
{
	printf("\t%s\n", version);
}


int API_ROUTINE isc_version(FB_API_HANDLE* handle, FPTR_VERSION_CALLBACK routine, void* user_arg)
{
/**************************************
 *
 *	g d s _ $ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Obtain and print information about a database.
 *
 **************************************/
	if (!routine)
		routine = print_version;

	UCHAR buffer[256];
	UCHAR* buf = buffer;
	USHORT buf_len = sizeof(buffer);

	ISC_STATUS_ARRAY status_vector;
	const TEXT* versions = 0;
	const TEXT* implementations = 0;
	bool redo;
	do {
		if (isc_database_info(status_vector, handle, sizeof(info), info,
							  buf_len, reinterpret_cast<char*>(buf)))
		{
			if (buf != buffer)
				gds__free(buf);
			return FB_FAILURE;
		}

		const UCHAR* p = buf;
		redo = false;

		while (!redo && *p != isc_info_end && p < buf + buf_len)
		{
			const UCHAR item = *p++;
			const USHORT len = static_cast<USHORT>(gds__vax_integer(p, 2));
			p += 2;
			switch (item)
			{
			case isc_info_firebird_version:
				versions = (TEXT*) p;
				break;

			case isc_info_implementation:
				implementations = (TEXT*) p;
				break;

			case isc_info_truncated:
				redo = true;
				break;

			default:
				if (buf != buffer)
					gds__free(buf);
				return FB_FAILURE;
			}
			p += len;
		}

		// Our buffer wasn't large enough to hold all the information,
		// make a larger one and try again.
		if (redo)
		{
			if (buf != buffer)
				gds__free(buf);
			buf_len += 1024;
			buf = (UCHAR*) gds__alloc((SLONG) (sizeof(UCHAR) * buf_len));
			// FREE: freed within this module
			if (!buf)			// NOMEM:
				return FB_FAILURE;
		}
	} while (redo);

	UCHAR count = MIN(*versions, *implementations);
	++versions;
	++implementations;

	TEXT s[128];

	while (count-- > 0)
	{
		const USHORT implementation_nr = *implementations++;
		const USHORT impl_class_nr = *implementations++;
		const int l = *versions++; // it was UCHAR
		const TEXT* implementation_string;
		if (implementation_nr >= FB_NELEM(impl_implementation) ||
			!(implementation_string = impl_implementation[implementation_nr]))
		{
			implementation_string = "**unknown**";
		}
		const TEXT* class_string;
		if (impl_class_nr >= FB_NELEM(impl_class) || !(class_string = impl_class[impl_class_nr]))
		{
			class_string = "**unknown**";
		}
		fb_utils::snprintf(s, sizeof(s), "%s (%s), version \"%.*s\"",
				implementation_string, class_string, l, versions);

		(*routine)(user_arg, s);
		versions += l;
	}

	if (buf != buffer)
		gds__free(buf);

	USHORT ods_version, ods_minor_version;
	if (get_ods_version(handle, &ods_version, &ods_minor_version) == FB_FAILURE)
		return FB_FAILURE;

	sprintf(s, "on disk structure version %d.%d", ods_version, ods_minor_version);
	(*routine)(user_arg, s);

	return FB_SUCCESS;
}


void API_ROUTINE isc_format_implementation(USHORT implementation_nr,
										   USHORT ibuflen, TEXT* ibuf,
										   USHORT impl_class_nr,
										   USHORT cbuflen, TEXT* cbuf)
{
/**************************************
 *
 *	i s c _ f o r m a t _ i m p l e m e n t a t i o n
 *
 **************************************
 *
 * Functional description
 *	Convert the implementation and class codes to strings
 * 	by looking up their values in the internal tables.
 *
 **************************************/
	if (ibuflen > 0)
	{
		if (implementation_nr >= FB_NELEM(impl_implementation) ||
			!(impl_implementation[implementation_nr]))
		{
			strncpy(ibuf, "**unknown**", ibuflen - 1);
			ibuf[MIN(11, ibuflen - 1)] = '\0';
		}
		else
		{
			strncpy(ibuf, impl_implementation[implementation_nr], ibuflen - 1);
			const int len = strlen(impl_implementation[implementation_nr]);
			ibuf[MIN(len, ibuflen - 1)] = '\0';
		}
	}

	if (cbuflen > 0)
	{
		if (impl_class_nr >= FB_NELEM(impl_class) || !(impl_class[impl_class_nr]))
		{
			strncpy(cbuf, "**unknown**", cbuflen - 1);
			cbuf[MIN(11, cbuflen - 1)] = '\0';
		}
		else
		{
			strncpy(cbuf, impl_class[impl_class_nr], cbuflen - 1);
			const int len = strlen(impl_class[impl_class_nr]);
			cbuf[MIN(len, cbuflen - 1)] = '\0';
		}
	}

}


uintptr_t API_ROUTINE isc_baddress(SCHAR* object)
{
/**************************************
 *
 *        i s c _ b a d d r e s s
 *
 **************************************
 *
 * Functional description
 *      Return the address of whatever is passed in
 *
 **************************************/

	return (uintptr_t) object;
}


void API_ROUTINE isc_baddress_s(const SCHAR* object, uintptr_t* address)
{
/**************************************
 *
 *        i s c _ b a d d r e s s _ s
 *
 **************************************
 *
 * Functional description
 *      Copy the address of whatever is passed in to the 2nd param.
 *
 **************************************/

	*address = (uintptr_t) object;
}


int API_ROUTINE BLOB_close(FB_BLOB_STREAM bstream)
{
/**************************************
 *
 *	B L O B _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	Close a blob stream.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;

	if (!bstream->bstr_blob)
		return FALSE;

	if (bstream->bstr_mode & BSTR_output)
	{
		const USHORT l = (bstream->bstr_ptr - bstream->bstr_buffer);
		if (l > 0)
		{
			if (isc_put_segment(status_vector, &bstream->bstr_blob, l, bstream->bstr_buffer))
			{
				return FALSE;
			}
		}
	}

	isc_close_blob(status_vector, &bstream->bstr_blob);

	if (bstream->bstr_mode & BSTR_alloc)
		gds__free(bstream->bstr_buffer);

	gds__free(bstream);

	return TRUE;
}


int API_ROUTINE blob__display(SLONG blob_id[2],
							  FB_API_HANDLE* database,
							  FB_API_HANDLE* transaction,
							  const TEXT* field_name, const SSHORT* name_length)
{
/**************************************
 *
 *	b l o b _ $ d i s p l a y
 *
 **************************************
 *
 * Functional description
 *	PASCAL callable version of EDIT_blob.
 *
 **************************************/
	const Firebird::MetaName temp(field_name, *name_length);

	return BLOB_display(reinterpret_cast<ISC_QUAD*>(blob_id), *database, *transaction, temp.c_str());
}


int API_ROUTINE BLOB_display(ISC_QUAD* blob_id,
							 FB_API_HANDLE database,
							 FB_API_HANDLE transaction,
							 const TEXT* /*field_name*/)
{
/**************************************
 *
 *	B L O B _ d i s p l a y
 *
 **************************************
 *
 * Functional description
 *	Open a blob, dump it to a file, allow the user to read the
 *	window.
 *
 **************************************/

	return dump(blob_id, database, transaction, stdout);
}


int API_ROUTINE blob__dump(SLONG blob_id[2],
						   FB_API_HANDLE* database,
						   FB_API_HANDLE* transaction,
						   const TEXT* file_name, const SSHORT* name_length)
{
/**************************************
 *
 *	b l o b _ $ d u m p
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable dump
 *	into an internal dump call.
 *
 **************************************/
	// CVC: The old logic passed garbage to BLOB_dump if !*name_length
	TEXT temp[129];
	USHORT l = *name_length;
	if (l != 0)
	{
		if (l >= sizeof(temp))
			l = sizeof(temp) - 1;

		memcpy(temp, file_name, l);
	}
	temp[l] = 0;

	return BLOB_dump(reinterpret_cast<ISC_QUAD*>(blob_id), *database, *transaction, temp);
}


int API_ROUTINE BLOB_text_dump(ISC_QUAD* blob_id,
							   FB_API_HANDLE database,
							   FB_API_HANDLE transaction,
							   const SCHAR* file_name)
{
/**************************************
 *
 *	B L O B _ t e x t _ d u m p
 *
 **************************************
 *
 * Functional description
 *	Dump a blob into a file.
 *      This call does CR/LF translation on NT.
 *
 **************************************/
	FILE* file = fopen(file_name, FOPEN_WRITE_TYPE_TEXT);
	if (!file)
		return FALSE;

	if (!dump(blob_id, database, transaction, file))
	{
		fclose(file);
		unlink(file_name);
		return FALSE;
	}
	fclose(file);
	return TRUE;
}


int API_ROUTINE BLOB_dump(ISC_QUAD* blob_id,
						  FB_API_HANDLE database,
						  FB_API_HANDLE transaction,
						  const SCHAR* file_name)
{
/**************************************
 *
 *	B L O B _ d u m p
 *
 **************************************
 *
 * Functional description
 *	Dump a blob into a file.
 *
 **************************************/
	FILE* file = fopen(file_name, FOPEN_WRITE_TYPE);
	if (!file)
		return FALSE;

	if (!dump(blob_id, database, transaction, file))
	{
		fclose(file);
		unlink(file_name);
		return FALSE;
	}
	fclose(file);
	return TRUE;
}


int API_ROUTINE blob__edit(SLONG blob_id[2],
						   FB_API_HANDLE* database,
						   FB_API_HANDLE* transaction,
						   const TEXT* field_name, const SSHORT* name_length)
{
/**************************************
 *
 *	b l o b _ $ e d i t
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable edit
 *	into an internal edit call.
 *
 **************************************/
	const Firebird::MetaName temp(field_name, *name_length);

	return BLOB_edit(reinterpret_cast<ISC_QUAD*>(blob_id), *database, *transaction, temp.c_str());
}


int API_ROUTINE BLOB_edit(ISC_QUAD* blob_id,
						  FB_API_HANDLE database,
						  FB_API_HANDLE transaction,
						  const SCHAR* field_name)
{
/**************************************
 *
 *	B L O B _ e d i t
 *
 **************************************
 *
 * Functional description
 *	Open a blob, dump it to a file, allow the user to edit the
 *	window, and dump the data back into a blob.  If the user
 *	bails out, return FALSE, otherwise return TRUE.
 *
 **************************************/

	return edit(blob_id, database, transaction, TRUE, field_name);
}


int API_ROUTINE BLOB_get(FB_BLOB_STREAM bstream)
{
/**************************************
 *
 *	B L O B _ g e t
 *
 **************************************
 *
 * Functional description
 *	Return the next byte of a blob.  If the blob is exhausted, return
 *	EOF.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;

	if (!bstream->bstr_buffer)
		return EOF;

	while (true)
	{
		if (--bstream->bstr_cnt >= 0)
			return *bstream->bstr_ptr++ & 0377;

		isc_get_segment(status_vector, &bstream->bstr_blob,
			// safe - cast from short, alignment is OK
			reinterpret_cast<USHORT*>(&bstream->bstr_cnt),
			bstream->bstr_length, bstream->bstr_buffer);
		if (status_vector[1] && status_vector[1] != isc_segment)
		{
			bstream->bstr_ptr = 0;
			bstream->bstr_cnt = 0;
			if (status_vector[1] != isc_segstr_eof)
				isc_print_status(status_vector);
			return EOF;
		}
		bstream->bstr_ptr = bstream->bstr_buffer;
	}
}


int API_ROUTINE blob__load(SLONG blob_id[2],
						   FB_API_HANDLE* database,
						   FB_API_HANDLE* transaction,
						   const TEXT* file_name, const SSHORT* name_length)
{
/**************************************
 *
 *	b l o b _ $ l o a d
 *
 **************************************
 *
 * Functional description
 *	Translate a pascal callable load
 *	into an internal load call.
 *
 **************************************/
	// CVC: The old logic passed garbage to BLOB_load if !*name_length
	TEXT temp[129];
	USHORT l = *name_length;
	if (l != 0)
	{
		if (l >= sizeof(temp))
			l = sizeof(temp) - 1;

		memcpy(temp, file_name, l);
	}
	temp[l] = 0;

	return BLOB_load(reinterpret_cast<ISC_QUAD*>(blob_id), *database, *transaction, temp);
}


int API_ROUTINE BLOB_text_load(ISC_QUAD* blob_id,
							   FB_API_HANDLE database,
							   FB_API_HANDLE transaction,
							   const TEXT* file_name)
{
/**************************************
 *
 *	B L O B _ t e x t _ l o a d
 *
 **************************************
 *
 * Functional description
 *	Load a  blob with the contents of a file.
 *      This call does CR/LF translation on NT.
 *      Return TRUE is successful.
 *
 **************************************/
	FILE* file = fopen(file_name, FOPEN_READ_TYPE_TEXT);
	if (!file)
		return FALSE;

	const int ret = load(blob_id, database, transaction, file);

	fclose(file);

	return ret;
}


int API_ROUTINE BLOB_load(ISC_QUAD* blob_id,
						  FB_API_HANDLE database,
						  FB_API_HANDLE transaction,
						  const TEXT* file_name)
{
/**************************************
 *
 *	B L O B _ l o a d
 *
 **************************************
 *
 * Functional description
 *	Load a blob with the contents of a file.  Return TRUE is successful.
 *
 **************************************/
	FILE* file = fopen(file_name, FOPEN_READ_TYPE);
	if (!file)
		return FALSE;

	const int ret = load(blob_id, database, transaction, file);

	fclose(file);

	return ret;
}


FB_BLOB_STREAM API_ROUTINE Bopen(ISC_QUAD* blob_id,
						   FB_API_HANDLE database,
						   FB_API_HANDLE transaction,
						   const SCHAR* mode)
{
/**************************************
 *
 *	B o p e n
 *
 **************************************
 *
 * Functional description
 *	Initialize a blob-stream block.
 *
 **************************************/
	// bpb is irrelevant, not used.
	const USHORT bpb_length = 0;
	const UCHAR* bpb = NULL;

	FB_API_HANDLE blob = 0;
	ISC_STATUS_ARRAY status_vector;

	switch (*mode)
	{
	case 'w':
	case 'W':
		if (isc_create_blob2(status_vector, &database, &transaction, &blob, blob_id,
							 bpb_length, reinterpret_cast<const char*>(bpb)))
		{
			return NULL;
		}
		break;
	case 'r':
	case 'R':
		if (isc_open_blob2(status_vector, &database, &transaction, &blob, blob_id,
						   bpb_length, bpb))
		{
			return NULL;
		}
		break;
	default:
		return NULL;
	}

	FB_BLOB_STREAM bstream = BLOB_open(blob, NULL, 0);

	if (*mode == 'w' || *mode == 'W')
	{
		bstream->bstr_mode |= BSTR_output;
		bstream->bstr_cnt = bstream->bstr_length;
		bstream->bstr_ptr = bstream->bstr_buffer;
	}
	else
	{
		bstream->bstr_cnt = 0;
		bstream->bstr_mode |= BSTR_input;
	}

	return bstream;
}


// CVC: This routine doesn't open a blob really!
FB_BLOB_STREAM API_ROUTINE BLOB_open(FB_API_HANDLE blob, SCHAR* buffer, int length)
{
/**************************************
 *
 *	B L O B _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Initialize a blob-stream block.
 *
 **************************************/
	if (!blob)
		return NULL;

	FB_BLOB_STREAM bstream = (FB_BLOB_STREAM) gds__alloc((SLONG) sizeof(BSTREAM));
	// FREE: This structure is freed by BLOB_close
	if (!bstream)				// NOMEM:
		return NULL;

#ifdef DEBUG_gds__alloc
	// This structure is handed to the user process, we depend on the client
	// to call BLOB_close() for it to be freed.
	gds_alloc_flag_unfreed((void*) bstream);
#endif

	bstream->bstr_blob = blob;
	bstream->bstr_length = length ? length : 512;
	bstream->bstr_mode = 0;
	bstream->bstr_cnt = 0;
	bstream->bstr_ptr = 0;

	if (!(bstream->bstr_buffer = buffer))
	{
		bstream->bstr_buffer = (SCHAR*) gds__alloc((SLONG) (sizeof(SCHAR) * bstream->bstr_length));
		// FREE: This structure is freed in BLOB_close()
		if (!bstream->bstr_buffer)
		{
			// NOMEM:
			gds__free(bstream);
			return NULL;
		}
#ifdef DEBUG_gds__alloc
		// This structure is handed to the user process, we depend on the client
		// to call BLOB_close() for it to be freed.
		gds_alloc_flag_unfreed((void*) bstream->bstr_buffer);
#endif

		bstream->bstr_mode |= BSTR_alloc;
	}

	return bstream;
}


int API_ROUTINE BLOB_put(SCHAR x, FB_BLOB_STREAM bstream)
{
/**************************************
 *
 *	B L O B _ p u t
 *
 **************************************
 *
 * Functional description
 *	Write a segment to a blob. First
 *	stick in the final character, then
 *	compute the length and send off the
 *	segment.  Finally, set up the buffer
 *	block and retun TRUE if all is well.
 *
 **************************************/
	if (!bstream->bstr_buffer)
		return FALSE;

	*bstream->bstr_ptr++ = (x & 0377);
	const USHORT l = (bstream->bstr_ptr - bstream->bstr_buffer);

	ISC_STATUS_ARRAY status_vector;
	if (isc_put_segment(status_vector, &bstream->bstr_blob, l, bstream->bstr_buffer))
	{
		return FALSE;
	}
	bstream->bstr_cnt = bstream->bstr_length;
	bstream->bstr_ptr = bstream->bstr_buffer;
	return TRUE;
}

#if (defined SOLARIS ) || (defined __cplusplus)
} //extern "C" {
#endif


static int dump(ISC_QUAD* blob_id,
				FB_API_HANDLE database,
				FB_API_HANDLE transaction,
				FILE* file)
{
/**************************************
 *
 *	d u m p
 *
 **************************************
 *
 * Functional description
 *	Dump a blob into a file.
 *
 **************************************/
	// bpb is irrelevant, not used.
	const USHORT bpb_length = 0;
	const UCHAR* bpb = NULL;

	// Open the blob.  If it failed, what the hell -- just return failure

	FB_API_HANDLE blob = 0;
	ISC_STATUS_ARRAY status_vector;
	if (isc_open_blob2(status_vector, &database, &transaction, &blob, blob_id, bpb_length, bpb))
	{
		isc_print_status(status_vector);
		return FALSE;
	}

	// Copy data from blob to scratch file

	SCHAR buffer[256];
	const SSHORT short_length = sizeof(buffer);

	for (;;)
	{
		USHORT l = 0;
		isc_get_segment(status_vector, &blob, &l, short_length, buffer);
		if (status_vector[1] && status_vector[1] != isc_segment)
		{
			if (status_vector[1] != isc_segstr_eof)
				isc_print_status(status_vector);
			break;
		}
		/*
		const TEXT* p = buffer;
		if (l)
			do {
				fputc(*p++, file);
			} while (--l);
		*/
		if (l)
			fwrite(buffer, 1, l, file);
	}

	// Close the blob

	isc_close_blob(status_vector, &blob);

	return TRUE;
}


static int edit(ISC_QUAD* blob_id,
				FB_API_HANDLE database,
				FB_API_HANDLE transaction,
				SSHORT type,
				const SCHAR* field_name)
{
/**************************************
 *
 *	e d i t
 *
 **************************************
 *
 * Functional description
 *	Open a blob, dump it to a file, allow the user to edit the
 *	window, and dump the data back into a blob.  If the user
 *	bails out, return FALSE, otherwise return TRUE.
 *
 *	If the field name coming in is too big, truncate it.
 *
 **************************************/
#if (defined WIN_NT)
	TEXT buffer[9];
#else
	TEXT buffer[25];
#endif

	const TEXT* q = field_name;
	if (!q)
		q = "gds_edit";

	TEXT* p;
	for (p = buffer; *q && p < buffer + sizeof(buffer) - 1; q++)
	{
		if (*q == '$')
			*p++ = '_';
		else
			*p++ = LOWER7(*q);
	}
	*p = 0;

	// Moved this out of #ifndef mpexl to get mktemp/mkstemp to work for Linux
	// This has been done in the inprise tree some days ago.
	// Would have saved me a lot of time, if I had seen this earlier :-(
	// FSG 15.Oct.2000
	Firebird::PathName tmpf = Firebird::TempFile::create(buffer);
	if (tmpf.empty()) {
		return FALSE;
	}

	FILE* file = fopen(tmpf.c_str(), FOPEN_WRITE_TYPE_TEXT);
	if (!file)
	{
		unlink(tmpf.c_str());
		return FALSE;
	}

	if (!dump(blob_id, database, transaction, file))
	{
		fclose(file);
		unlink(tmpf.c_str());
		return FALSE;
	}

	fclose(file);

	if (type = gds__edit(tmpf.c_str(), type))
	{

		if (!(file = fopen(tmpf.c_str(), FOPEN_READ_TYPE_TEXT)))
		{
			unlink(tmpf.c_str());
			return FALSE;
		}

		load(blob_id, database, transaction, file);

		fclose(file);

	}

	unlink(tmpf.c_str());

	return type;
}


static int get_ods_version(FB_API_HANDLE* handle,
						   USHORT* ods_version, USHORT* ods_minor_version)
{
/**************************************
 *
 *	g e t _ o d s _ v e r s i o n
 *
 **************************************
 *
 * Functional description
 *	Obtain information about a on-disk structure (ods) versions
 *	of the database.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	UCHAR buffer[16];

	isc_database_info(status_vector, handle, sizeof(ods_info), ods_info,
					  sizeof(buffer), reinterpret_cast<char*>(buffer));

	if (status_vector[1])
		return FB_FAILURE;

	const UCHAR* p = buffer;
	UCHAR item;

	while ((item = *p++) != isc_info_end)
	{
		const USHORT l = static_cast<USHORT>(gds__vax_integer(p, 2));
		p += 2;
		const USHORT n = static_cast<USHORT>(gds__vax_integer(p, l));
		p += l;
		switch (item)
		{
		case isc_info_ods_version:
			*ods_version = n;
			break;

		case isc_info_ods_minor_version:
			*ods_minor_version = n;
			break;

		default:
			return FB_FAILURE;
		}
	}

	return FB_SUCCESS;
}


// CVC: I just made this alternative function to let the original unchanged.
// However, the original logic doesn't make sense.
static void isc_expand_dpb_internal(const UCHAR** dpb, SSHORT* dpb_size, ...)
{
/**************************************
 *
 *	i s c _ e x p a n d _ d p b _ i n t e r n a l
 *
 **************************************
 *
 * Functional description
 *	Extend a database parameter block dynamically
 *	to include runtime info.  Generated
 *	by gpre to provide host variable support for
 *	READY statement	options.
 *	This expects the list of variable args
 *	to be zero terminated.
 *
 *	Note: dpb_size is signed short only for compatibility
 *	with other calls (isc_attach_database) that take a dpb length.
 *
 * TMN: Note: According to Ann Harrison:
 * That routine should be deprecated.  It doesn't do what it
 * should, and does do things it shouldn't, and is harder to
 * use than the natural alternative.
 *
 * CVC: This alternative version returns either the original dpb or a
 * new one, but never overwrites the original dpb. More accurately, it's
 * clearer than the original function that really never modifies its source
 * dpb, but there appears to be a logic failure on an impossible path.
 * Also, since the change from UCHAR** to const UCHAR** is not transparent,
 * a new version was needed to make sure the old world wasn't broken.
 *
 **************************************/
	SSHORT	length;
	unsigned char* p = 0;
	const char* q;
	const unsigned char* uq;
	va_list	args;
	USHORT	type;
	UCHAR* new_dpb;

	// calculate length of database parameter block, setting initial length to include version

	SSHORT new_dpb_length;
	if (!*dpb || !(new_dpb_length = *dpb_size))
	{
		new_dpb_length = 1;
	}

	va_start(args, dpb_size);

	while (type = va_arg(args, int))
	{
		switch (type)
		{
		case isc_dpb_user_name:
		case isc_dpb_password:
		case isc_dpb_sql_role_name:
		case isc_dpb_lc_messages:
		case isc_dpb_lc_ctype:
		case isc_dpb_reserved:
			q = va_arg(args, char*);
			if (q)
			{
				length = strlen(q);
				new_dpb_length += 2 + length;
			}
			break;

		default:
			va_arg(args, int);
			break;
		}
	}
	va_end(args);

	// if items have been added, allocate space for the new dpb and copy the old one over

	if (new_dpb_length > *dpb_size)
	{
		// Note: gds__free done by GPRE generated code

		new_dpb = (UCHAR*) gds__alloc((SLONG)(sizeof(UCHAR) * new_dpb_length));
		p = new_dpb;
		// FREE: done by client process in GPRE generated code
		if (!new_dpb)
		{
			// NOMEM: don't trash existing dpb
			DEV_REPORT("isc_extend_dpb: out of memory");
			return;				// NOMEM: not really handled
		}

		uq = *dpb;
		for (length = *dpb_size; length; length--)
		{
			*p++ = *uq++;
		}

	}
	else
	{
		// CVC: Notice the initialization is: new_dpb_length = *dpb_size
		// Therefore, the worst case is new_dpb_length == *dpb_size
		// Also, if *dpb_size == 0, then new_dpb_length is set to 1,
		// so there will be again a bigger new buffer.
		// Hence, this else just means "we found nothing that we can
		// recognize in the variable params list to add and thus,
		// there's nothing to do". The case for new_dpb_length being less
		// than the original length simply can't happen. Therefore,
		// the input can be declared const.
		return;
	}

	if (!*dpb_size)
		*p++ = isc_dpb_version1;

	// copy in the new runtime items

	va_start(args, dpb_size);

	while (type = va_arg(args, int))
	{
		switch (type)
		{
		case isc_dpb_user_name:
		case isc_dpb_password:
		case isc_dpb_sql_role_name:
		case isc_dpb_lc_messages:
		case isc_dpb_lc_ctype:
		case isc_dpb_reserved:
		    q = va_arg(args, char*);
			if (q)
			{
				length = strlen(q);
				fb_assert(type <= CHAR_MAX);
				*p++ = (unsigned char) type;
				fb_assert(length <= CHAR_MAX);
				*p++ = (unsigned char) length;
				while (length--)
					*p++ = *q++;
			}
			break;

		default:
			va_arg(args, int);
			break;
		}
	}
	va_end(args);

	*dpb_size = p - new_dpb;
	*dpb = new_dpb;
}


static int load(ISC_QUAD* blob_id,
				FB_API_HANDLE database,
				FB_API_HANDLE transaction,
				FILE* file)
{
/**************************************
 *
 *     l o a d
 *
 **************************************
 *
 * Functional description
 *      Load a blob from a file.
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;

	// Open the blob.  If it failed, what the hell -- just return failure

	FB_API_HANDLE blob = 0;
	if (isc_create_blob(status_vector, &database, &transaction, &blob, blob_id))
	{
		isc_print_status(status_vector);
		return FALSE;
	}

	// Copy data from file to blob.  Make up boundaries at end of of line.
	TEXT buffer[512];
	TEXT* p = buffer;
	const TEXT* const buffer_end = buffer + sizeof(buffer);

	for (;;)
	{
		const SSHORT c = fgetc(file);
		if (feof(file))
			break;
		*p++ = static_cast<TEXT>(c);
		if ((c != '\n') && p < buffer_end)
			continue;
		const SSHORT l = p - buffer;
		if (isc_put_segment(status_vector, &blob, l, buffer))
		{
			isc_print_status(status_vector);
			isc_close_blob(status_vector, &blob);
			return FALSE;
		}
		p = buffer;
	}

	const SSHORT l = p - buffer;
	if (l != 0)
	{
		if (isc_put_segment(status_vector, &blob, l, buffer))
		{
			isc_print_status(status_vector);
			isc_close_blob(status_vector, &blob);
			return FALSE;
		}
	}

	isc_close_blob(status_vector, &blob);

	return TRUE;
}

// new utl
static inline void setTag(Firebird::ClumpletWriter& dpb, UCHAR tag, const TEXT* value)
{
	if (! dpb.find(tag))
	{
		dpb.insertString(tag, value, strlen(value));
	}
}

void setLogin(Firebird::ClumpletWriter& dpb)
{
	if (!(dpb.find(isc_dpb_trusted_auth) || dpb.find(isc_dpb_address_path)))
	{
		Firebird::string username;
		if (fb_utils::readenv(ISC_USER, username) && !dpb.find(isc_dpb_sys_user_name))
		{
			setTag(dpb, isc_dpb_user_name, username.c_str());
		}

		Firebird::string password;
		if (fb_utils::readenv(ISC_PASSWORD, password) && !dpb.find(isc_dpb_password_enc))
		{
			setTag(dpb, isc_dpb_password, password.c_str());
		}
	}
}

