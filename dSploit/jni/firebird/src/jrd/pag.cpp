/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		pag.cpp
 *	DESCRIPTION:	Page level ods manager
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
 * Modified by: Patrick J. P. Griffin
 * Date: 11/29/2000
 * Problem:   Bug 116733 Too many generators corrupt database.
 *            DPM_gen_id was not calculating page and offset correctly.
 * Change:    Caculate pgc_gpg, number of generators per page,
 *            for use in DPM_gen_id.
 *
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2001.08.07 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, second attempt
 *
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "MAC" and "MAC_CP" defines
 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete "Apollo" port

 * 2002.02.15 Sean Leyne - Code Cleanup, removed obsolete ports:
 *                          - EPSON, DELTA, IMP, NCR3000, M88K
 *                          - HP9000 s300 and Apollo
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DG_X86" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "UNIXWARE" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix/MIPS" port
 *
 * 2002.10.28 Sean Leyne - Completed removal of obsolete "DGUX" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "MPEXL" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "DecOSF" port
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 */


#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <string.h>

#ifdef WIN_NT
#include <process.h>
#endif

#include "../common/config/config.h"
#include "../common/utils_proto.h"
#include "../jrd/fil.h"
#include "../jrd/jrd.h"
#include "../jrd/pag.h"
#include "../jrd/ods.h"
#include "../jrd/os/pio.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/ibase.h"
#include "../jrd/gdsassert.h"
#include "../jrd/lck.h"
#include "../jrd/sdw.h"
#include "../jrd/cch.h"
#include "../jrd/nbak.h"
#include "../jrd/tra.h"
#ifdef VIO_DEBUG
#include "../jrd/vio_debug.h"
#endif
#include "../jrd/cch_proto.h"
#include "../jrd/dpm_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/met_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/ods_proto.h"
#include "../jrd/pag_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../jrd/thread_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/TempSpace.h"
#include "../jrd/extds/ExtDS.h"

using namespace Jrd;
using namespace Ods;
using namespace Firebird;

static int blocking_ast_attachment(void*);
static void find_clump_space(thread_db* tdbb, SLONG, WIN*, pag**, USHORT, USHORT, const UCHAR*);
static bool find_type(thread_db* tdbb, SLONG, WIN*, pag**, USHORT, USHORT, UCHAR**, const UCHAR**);

inline void err_post_if_database_is_readonly(const Database* dbb)
{
	if (dbb->dbb_flags & DBB_read_only)
		ERR_post(Arg::Gds(isc_read_only_database));
}

// Class definitions (obsolete platforms are commented out)
// Class constant name consists of OS platform and CPU architecture.
//
// For ports created before Firebird 2.0 release 64-bit and 32-bit
// sub-architectures of the same CPU should use different classes.
// For 64-bit ports first created after or as a part of Firebird 2.0
// release CPU architecture may be the same for both variants.

static const int CLASS_UNKNOWN = 0;
//static const CLASS_APOLLO_68K = 1;		// Apollo 68K, Dn 10K
static const int CLASS_SOLARIS_SPARC = 2;	// Sun 68k, Sun Sparc, HP 9000/300, MAC AUX, IMP, DELTA, NeXT, UNIXWARE, DG_X86
static const int CLASS_SOLARIS_I386 = 3;	// Sun 386i
//static const CLASS_VMS_VAX = 4;			// VMS/VAX
//static const CLASS_ULTRIX_VAX = 5;		// Ultrix/VAX
//static const CLASS_ULTRIX_MIPS = 6;		// Ultrix/MIPS
static const int CLASS_HPUX_PA = 7;			// HP-UX on PA-RISC (was: HP 900/800 (precision))
//static const int CLASS_NETWARE_I386 = 8;	// NetWare
//static const CLASS_MAC_OS = 9;			// MAC-OS
static const int CLASS_AIX_PPC = 10;		// AIX on PowerPC platform (was: IBM RS/6000)
//static const CLASS_DG_AVIION = 11;		// DG AViiON
//static const CLASS_MPE_XL = 12;			// MPE/XL
//static const int CLASS_IRIX_MIPS = 13;	// Silicon Graphics/IRIS
//static const int CLASS_CRAY = 14;			// Cray
//static const int CLASS_TRU64_ALPHA = 15;	// Tru64 Unix running on Alpha (was: Dec OSF/1)
static const int CLASS_WINDOWS_I386 = 16;	// NT -- post 4.0 (delivered 4.0 as class 8)
//static const CLASS_OS2 = 17;				// OS/2
//static const CLASS_WIN16 = 18;			// Windows 16 bit
static const int CLASS_LINUX_I386 = 19;		// LINUX on Intel series
static const int CLASS_LINUX_SPARC = 20;	// LINUX on sparc systems
static const int CLASS_FREEBSD_I386 = 21;	// FreeBSD/i386
static const int CLASS_NETBSD_I386 = 22;	// NetBSD/i386
static const int CLASS_DARWIN_PPC = 23;		// Darwin/PowerPC
static const int CLASS_LINUX_AMD64 = 24;	// LINUX on AMD64 systems
static const int CLASS_FREEBSD_AMD64 = 25;	// FreeBSD/amd64
static const int CLASS_WINDOWS_AMD64 = 26;	// Windows/amd64
static const int CLASS_LINUX_PPC = 27;		// LINUX/PowerPC
static const int CLASS_DARWIN_I386 = 28;	// Darwin/Intel
static const int CLASS_LINUX_MIPSEL = 29;	// LINUX/MIPSEL
static const int CLASS_LINUX_MIPS = 30;		// LINUX/MIPS
static const int CLASS_DARWIN_X64 = 31;		// Darwin/x64
static const int CLASS_SOLARIS_AMD64 = 32;	// Solaris/amd64
static const int CLASS_LINUX_ARM = 33;		// LINUX/ARM
static const int CLASS_LINUX_IA64 = 34;		// LINUX/IA64
static const int CLASS_DARWIN_PPC64 = 35;	// Darwin/PowerPC64
static const int CLASS_LINUX_S390X = 36;	// LINUX/s390x
static const int CLASS_LINUX_S390 = 37;		// LINUX/s390
static const int CLASS_LINUX_SH = 38;		// LINUX/SH (little-endian)
static const int CLASS_LINUX_SHEB = 39;		// LINUX/SH (big-endian)
static const int CLASS_LINUX_HPPA = 40;		// LINUX/HPPA
static const int CLASS_LINUX_ALPHA = 41;	// LINUX/ALPHA

static const int CLASS_MAX10 = CLASS_LINUX_AMD64;	// This should not be changed, no new ports with ODS10
static const int CLASS_MAX = CLASS_LINUX_ALPHA;

// ARCHITECTURE COMPATIBILITY CLASSES

// For ODS10 and earlier things which normally define ODS compatibility are:
//  1) endianness (big-endian/little-endian)
//  2) alignment (32-bit or 64-bit), matters for record formats
//  3) pointer size (32-bit or 64-bit), also matters for record formats
//
// For ODS11 pointers are not stored in database and alignment is always 64-bit.
// So the only thing which normally matters for ODS11 is endiannes, but if
// endianness is wrong we are going to notice it during ODS version check,
// before architecture compatibility is tested. But we distinguish them here too,
// for consistency.

enum ArchitectureType {
	archUnknown,		// Unknown architecture, allow opening database only if CLASS matches exactly
	archIntel86,		// Little-endian platform with 32-bit pointers and 32-bit alignment (ODS10)
	archLittleEndian,	// Any little-endian platform with standard layout of data
	archBigEndian		// Any big-endian platform with standard layout of data
};

// Note that Sparc, HP and PowerPC disk structures should be compatible in theory,
// but in practice alignment on these platforms varies and actually depends on the
// compiler used to produce the build. Yes, some 32-bit RISC builds use 64-bit alignment.
// This is why we declare all such builds "Unknown" for ODS10.

static const ArchitectureType archMatrix10[CLASS_MAX10 + 1] =
{
	archUnknown, // CLASS_UNKNOWN
	archUnknown, // CLASS_APOLLO_68K
	archUnknown, // CLASS_SOLARIS_SPARC
	archIntel86, // CLASS_SOLARIS_I386
	archUnknown, // CLASS_VMS_VAX
	archUnknown, // CLASS_ULTRIX_VAX
	archUnknown, // CLASS_ULTRIX_MIPS
	archUnknown, // CLASS_HPUX_PA
	archUnknown, // CLASS_NETWARE_I386
	archUnknown, // CLASS_MAC_OS
	archUnknown, // CLASS_AIX_PPC
	archUnknown, // CLASS_DG_AVIION
	archUnknown, // CLASS_MPE_XL
	archUnknown, // CLASS_IRIX_MIPS
	archUnknown, // CLASS_CRAY
	archUnknown, // CLASS_TRU64_ALPHA
	archIntel86, // CLASS_WINDOWS_I386
	archUnknown, // CLASS_OS2
	archUnknown, // CLASS_WIN16
	archIntel86, // CLASS_LINUX_I386
	archUnknown, // CLASS_LINUX_SPARC
	archIntel86, // CLASS_FREEBSD_I386
	archIntel86, // CLASS_NETBSD_I386
	archUnknown, // CLASS_DARWIN_PPC
	archUnknown  // CLASS_LINUX_AMD64
};

static const ArchitectureType archMatrix[CLASS_MAX + 1] =
{
	archUnknown,      // CLASS_UNKNOWN
	archUnknown,      // CLASS_APOLLO_68K
	archBigEndian,    // CLASS_SOLARIS_SPARC
	archLittleEndian, // CLASS_SOLARIS_I386
	archUnknown,      // CLASS_VMS_VAX
	archUnknown,      // CLASS_ULTRIX_VAX
	archUnknown, 	  // CLASS_ULTRIX_MIPS
	archBigEndian,    // CLASS_HPUX_PA
	archUnknown,      // CLASS_NETWARE_I386
	archUnknown,      // CLASS_MAC_OS
	archBigEndian,    // CLASS_AIX_PPC
	archUnknown,      // CLASS_DG_AVIION
	archUnknown,      // CLASS_MPE_XL
	archBigEndian,    // CLASS_IRIX_MIPS
	archUnknown,      // CLASS_CRAY
	archBigEndian,    // CLASS_TRU64_ALPHA
	archLittleEndian, // CLASS_WINDOWS_I386
	archUnknown,      // CLASS_OS2
	archUnknown,      // CLASS_WIN16
	archLittleEndian, // CLASS_LINUX_I386
	archBigEndian,    // CLASS_LINUX_SPARC
	archLittleEndian, // CLASS_FREEBSD_I386
	archLittleEndian, // CLASS_NETBSD_I386
	archBigEndian,    // CLASS_DARWIN_PPC
	archLittleEndian, // CLASS_LINUX_AMD64
	archLittleEndian, // CLASS_FREEBSD_AMD64
	archLittleEndian, // CLASS_WINDOWS_AMD64
	archBigEndian,    // CLASS_LINUX_PPC
	archLittleEndian, // CLASS_DARWIN_I386
	archLittleEndian, // CLASS_LINUX_MIPSEL
	archBigEndian,    // CLASS_LINUX_MIPS
	archLittleEndian, // CLASS_DARWIN_X64
	archLittleEndian, // CLASS_SOLARIS_AMD64
	archLittleEndian, // CLASS_LINUX_ARM
	archLittleEndian, // CLASS_LINUX_IA64
	archBigEndian,	  // CLASS_DARWIN_PPC64
	archBigEndian,	  // CLASS_LINUX_S390X
	archBigEndian,	  // CLASS_LINUX_S390
	archLittleEndian, // CLASS_LINUX_SH
	archBigEndian,    // CLASS_LINUX_SHEB
	archBigEndian,    // CLASS_LINUX_HPPA
	archLittleEndian  // CLASS_LINUX_ALPHA
};

#ifdef __sun
#ifdef __i386
const SSHORT CLASS		= CLASS_SOLARIS_I386;
#elif defined (__sparc)
const SSHORT CLASS		= CLASS_SOLARIS_SPARC;
#elif defined (__amd64)
const SSHORT CLASS		= CLASS_SOLARIS_AMD64;
#else
#error no support for this hardware on SUN
#endif
#endif // __sun

#ifdef HPUX
const SSHORT CLASS		= CLASS_HPUX_PA;
#endif

#ifdef AIX_PPC
const SSHORT CLASS		= CLASS_AIX_PPC;
#endif

#ifdef WIN_NT
#if defined(I386)
const SSHORT CLASS		= CLASS_WINDOWS_I386;
#elif defined(AMD64)
const SSHORT CLASS		= CLASS_WINDOWS_AMD64;
#else
#error no support on other hardware for Windows
#endif
#endif	// WIN_NT

#ifdef LINUX
#if defined(i386) || defined(i586)
const SSHORT CLASS		= CLASS_LINUX_I386;
#elif defined(sparc)
const SSHORT CLASS		= CLASS_LINUX_SPARC;
#elif defined(AMD64)
const SSHORT CLASS		= CLASS_LINUX_AMD64;
#elif defined(ARM)
const SSHORT CLASS		= CLASS_LINUX_ARM;
#elif defined(PPC)
const SSHORT CLASS		= CLASS_LINUX_PPC;
#elif defined(MIPSEL)
const SSHORT CLASS		= CLASS_LINUX_MIPSEL;
#elif defined(MIPSEB)
const SSHORT CLASS		= CLASS_LINUX_MIPS;
#elif defined(IA64)
const SSHORT CLASS		= CLASS_LINUX_IA64;
#elif defined(__s390__)
# if defined(__s390x__)
const SSHORT CLASS		= CLASS_LINUX_S390X;
# else
const SSHORT CLASS		= CLASS_LINUX_S390;
# endif	    // defined(__s390x__)
#elif defined(SH)
const SSHORT CLASS		= CLASS_LINUX_SH;
#elif defined(SHEB)
const SSHORT CLASS		= CLASS_LINUX_SHEB;
#elif defined(HPPA)
const SSHORT CLASS		= CLASS_LINUX_HPPA;
#elif defined(ALPHA)
const SSHORT CLASS		= CLASS_LINUX_ALPHA;
#else
#error no support on other hardware for Linux
#endif
#endif	// LINUX

#ifdef FREEBSD
#if defined(i386)
const SSHORT CLASS		= CLASS_FREEBSD_I386;
#elif defined(AMD64)
const SSHORT CLASS		= CLASS_FREEBSD_AMD64;
#else
#error no support on other hardware for FreeBSD
#endif
#endif

#ifdef NETBSD
const SSHORT CLASS		= CLASS_NETBSD_I386;
#endif

#ifdef DARWIN
#if defined(i386)
const SSHORT CLASS		= CLASS_DARWIN_I386;
#elif defined(DARWIN64)
const SSHORT CLASS		= CLASS_DARWIN_X64;
#elif defined(powerpc)
const SSHORT CLASS		= CLASS_DARWIN_PPC;
#elif defined(DARWINPPC64)
const SSHORT CLASS		= CLASS_DARWIN_PPC64;
#endif
#endif  // DARWIN

static const char* const SCRATCH = "fb_table_";

static const int MIN_EXTEND_BYTES = 128 * 1024;	// 128KB

// CVC: Since nobody checks the result from this function (strange!), I changed
// bool to void as the return type but left the result returned as comment.
void PAG_add_clump(thread_db* tdbb,
				   SLONG page_num, USHORT type,
				   USHORT len, const UCHAR* entry, ClumpOper mode) // bool must_write
{
/***********************************************
 *
 *	P A G _ a d d _ c l u m p
 *
 ***********************************************
 *
 * Functional description
 *	Adds a clump to log/header page.
 *	mode
 *		0 - add			CLUMP_ADD
 *		1 - replace		CLUMP_REPLACE
 *		2 - replace only!	CLUMP_REPLACE_ONLY
 *	returns
 *		true - modified page
 *		false - nothing done => nobody checks this function's result.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	pag* page;
	header_page* header = 0;
	log_info_page* logp = 0;
	USHORT* end_addr;
	WIN window(DB_PAGE_SPACE, page_num);
	if (page_num == HEADER_PAGE)
	{
		page = CCH_FETCH(tdbb, &window, LCK_write, pag_header);
		header = (header_page*) page;
		end_addr = &header->hdr_end;
	}
	else
	{
		page = CCH_FETCH(tdbb, &window, LCK_write, pag_log);
		logp = (log_info_page*) page;
		end_addr = &logp->log_end;
	}

	UCHAR* entry_p;
	const UCHAR* clump_end;
	while (mode != CLUMP_ADD)
	{
		const bool found =
			find_type(tdbb, page_num, &window, &page, LCK_write, type, &entry_p, &clump_end);

		// If we did'nt find it and it is REPLACE_ONLY, return

		if (!found && mode == CLUMP_REPLACE_ONLY)
		{
			CCH_RELEASE(tdbb, &window);
			return; //false;
		}

		// If not found, just go and add the entry

		if (!found)
			break;

		// if same size, overwrite it

		if (entry_p[1] == len)
		{
			entry_p += 2;
			if (len)
			{
				//if (must_write)
					CCH_MARK_MUST_WRITE(tdbb, &window);
				//else
				//	CCH_MARK(tdbb, &window);

				memcpy(entry_p, entry, len);
			}
			CCH_RELEASE(tdbb, &window);
			return; // true;
		}

		// delete the entry

		// Page is marked must write because of precedence problems.  Later
		// on we may allocate a new page and set up a precedence relationship.
		// This may be the lower precedence page and so it cannot be dirty

		CCH_MARK_MUST_WRITE(tdbb, &window);

		*end_addr -= (2 + entry_p[1]);

		const UCHAR* r = entry_p + 2 + entry_p[1];
		USHORT l = clump_end - r + 1;
		if (l)
			memmove(entry_p, r, l);

		CCH_RELEASE(tdbb, &window);

		// refetch the page

		window.win_page = page_num;
		if (page_num == HEADER_PAGE)
		{
			page = CCH_FETCH(tdbb, &window, LCK_write, pag_header);
			header = (header_page*) page;
			end_addr = &header->hdr_end;
		}
		else
		{
			page = CCH_FETCH(tdbb, &window, LCK_write, pag_log);
			logp = (log_info_page*) page;
			end_addr = &logp->log_end;
		}
		break;
	}

	// Add the entry

	find_clump_space(tdbb, page_num, &window, &page, type, len, entry);

	CCH_RELEASE(tdbb, &window);
	return; // true;
}


USHORT PAG_add_file(thread_db* tdbb, const TEXT* file_name, SLONG start)
{
/**************************************
 *
 *	P A G _ a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to the current database.  Return the sequence number for the new file.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	// Find current last file

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	jrd_file* file = pageSpace->file;
	while (file->fil_next) {
		file = file->fil_next;
	}

	// Verify database file path against DatabaseAccess entry of firebird.conf
	if (!JRD_verify_database_access(file_name))
	{
		ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("additional database file") <<
													 Arg::Str(file_name));
	}

	// Create the file.  If the sequence number comes back zero, it didn't work, so punt

	const USHORT sequence = PIO_add_file(dbb, pageSpace->file, file_name, start);
	if (!sequence)
		return 0;

	// Create header page for new file

	jrd_file* next = file->fil_next;

	if (dbb->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
	{
		PIO_force_write(next, dbb->dbb_flags & DBB_force_write, dbb->dbb_flags & DBB_no_fs_cache);
	}

	WIN window(DB_PAGE_SPACE, next->fil_min_page);
	header_page* header = (header_page*) CCH_fake(tdbb, &window, 1);
	header->hdr_header.pag_type = pag_header;
	header->hdr_sequence = sequence;
	header->hdr_page_size = dbb->dbb_page_size;
	header->hdr_data[0] = HDR_end;
	header->hdr_end = HDR_SIZE;
	next->fil_sequence = sequence;

#ifdef SUPPORT_RAW_DEVICES
	// The following lines (taken from PAG_format_header) are needed to identify
	// this file in raw_devices_validate_database as a valid database attachment.
	*(ISC_TIMESTAMP*) header->hdr_creation_date = Firebird::TimeStamp::getCurrentTimeStamp().value();
	// should we include milliseconds or not?
	//Firebird::TimeStamp::round_time(header->hdr_creation_date->timestamp_time, 0);

	header->hdr_ods_version        = ODS_VERSION | ODS_FIREBIRD_FLAG;
	header->hdr_implementation     = CLASS;
	header->hdr_ods_minor          = ODS_CURRENT;
	header->hdr_ods_minor_original = ODS_CURRENT;
	if (dbb->dbb_flags & DBB_DB_SQL_dialect_3)
		header->hdr_flags |= hdr_SQL_dialect_3;
#endif

	header->hdr_header.pag_checksum = CCH_checksum(window.win_bdb);
	PIO_write(pageSpace->file, window.win_bdb, window.win_buffer, tdbb->tdbb_status_vector);
	CCH_RELEASE(tdbb, &window);
	next->fil_fudge = 1;

	// Update the previous header page to point to new file

	file->fil_fudge = 0;
	window.win_page = file->fil_min_page;
	header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	if (!file->fil_min_page)
		CCH_MARK_MUST_WRITE(tdbb, &window);
	else
		CCH_MARK(tdbb, &window);

	--start;

	if (file->fil_min_page)
	{
		PAG_add_header_entry(tdbb, header, HDR_file, strlen(file_name),
							 reinterpret_cast<const UCHAR*>(file_name));
		PAG_add_header_entry(tdbb, header, HDR_last_page, sizeof(SLONG), (UCHAR*) &start);
	}
	else
	{
		PAG_add_clump(tdbb, HEADER_PAGE, HDR_file, strlen(file_name),
					  reinterpret_cast<const UCHAR*>(file_name), CLUMP_REPLACE); //, true;
		PAG_add_clump(tdbb, HEADER_PAGE, HDR_last_page, sizeof(SLONG),
					  (UCHAR*) &start, CLUMP_REPLACE); //, true
	}

	header->hdr_header.pag_checksum = CCH_checksum(window.win_bdb);
	PIO_write(pageSpace->file, window.win_bdb, window.win_buffer, tdbb->tdbb_status_vector);
	CCH_RELEASE(tdbb, &window);
	if (file->fil_min_page)
		file->fil_fudge = 1;

	return sequence;
}


bool PAG_add_header_entry(thread_db* tdbb, header_page* header,
						 USHORT type, USHORT len, const UCHAR* entry)
{
/***********************************************
 *
 *	P A G _ a d d _ h e a d e r _ e n t r y
 *
 ***********************************************
 *
 * Functional description
 *	Add an entry to header page.
 *	This will be used mainly for the shadow header page and adding
 *	secondary files.
 *	Will not follow to hdr_next_page
 *	Will not journal changes made to page. => obsolete
 *	RETURNS
 *		true - modified page
 *		false - nothing done
 * CVC: Nobody checks the result of this function!
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	UCHAR* p = header->hdr_data;
	while (*p != HDR_end && *p != type)
		p += 2 + p[1];

	if (*p != HDR_end)
		return false;

	// We are at HDR_end, add the entry

	const int free_space = dbb->dbb_page_size - header->hdr_end;

	if (free_space > (2 + len))
	{
		fb_assert(type <= MAX_UCHAR);
		fb_assert(len <= MAX_UCHAR);
		*p++ = static_cast<UCHAR>(type);
		*p++ = static_cast<UCHAR>(len);

		if (len)
		{
			if (entry) {
				memcpy(p, entry, len);
			}
			else {
				memset(p, 0, len);
			}
			p += len;
		}

		*p = HDR_end;

		header->hdr_end = p - (UCHAR*) header;

		return true;
	}

	BUGCHECK(251);
	return false;				// Added to remove compiler warning
}


void PAG_attach_temp_pages(thread_db* tdbb, USHORT pageSpaceID)
{
/***********************************************
 *
 *	P A G _ a t t a c h _ t e m p _ p a g e s
 *
 ***********************************************
 *
 * Functional description
 *	Attach a temporary page space
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	PageSpace* pageSpaceTemp = dbb->dbb_page_manager.addPageSpace(pageSpaceID);
	if (!pageSpaceTemp->file)
	{
		Firebird::PathName file_name = TempFile::create(SCRATCH);
		pageSpaceTemp->file = PIO_create(dbb, file_name, true, true, false);
		PAG_format_pip(tdbb, *pageSpaceTemp);
	}
}


bool PAG_replace_entry_first(thread_db* tdbb, header_page* header,
							 USHORT type, USHORT len, const UCHAR* entry)
{
/***********************************************
 *
 *	P A G _ r e p l a c e _ e n t r y _ f i r s t
 *
 ***********************************************
 *
 * Functional description
 *	Replace an entry in the header page so it will become first entry
 *	This will be used mainly for the clumplets used for backup purposes
 *  because they are needed to be read without page lock
 *	Will not follow to hdr_next_page
 *	Will not journal changes made to page. => obsolete
 *	RETURNS
 *		true - modified page
 *		false - nothing done
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	UCHAR* p = header->hdr_data;
	while (*p != HDR_end && *p != type) {
		p += 2 + p[1];
	}

	// Remove item if found it somewhere
	if (*p != HDR_end)
	{
		UCHAR l = p[1] + 2;
		memmove(p, p + l, header->hdr_end - (p - (UCHAR*) header) - l + 1); // to preserve HDR_end
		header->hdr_end -= l;
	}

	if (!entry) {
		return false; // We were asked just to remove item. We finished.
	}

	// Check if we got enough space
	if (dbb->dbb_page_size - header->hdr_end <= len + 2) {
		BUGCHECK(251);
	}

	// Actually add the item
	memmove(header->hdr_data + len + 2, header->hdr_data, header->hdr_end - HDR_SIZE + 1);
	header->hdr_data[0] = type;
	header->hdr_data[1] = len;
	memcpy(header->hdr_data + 2, entry, len);
	header->hdr_end += len + 2;

	return true;
}

PAG PAG_allocate(thread_db* tdbb, WIN* window)
{
/**************************************
 *
 *	P A G _ a l l o c a t e
 *
 **************************************
 *
 * Functional description
 *	Allocate a page and fake a read with a write lock.  This is
 *	the universal sequence when allocating pages.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	PageManager& pageMgr = dbb->dbb_page_manager;
	PageSpace* pageSpace = pageMgr.findPageSpace(window->win_page.getPageSpaceID());
	fb_assert(pageSpace);

	// Not sure if this can be moved inside the loop. Maybe some data members
	// should persist across iterations?
	WIN pip_window(pageSpace->pageSpaceID, -1);
	// CVC: Not sure of the initial value. Notice bytes and bit are used after the loop.
	UCHAR* bytes = 0;
	UCHAR bit = 0;

	pag* new_page = 0; // NULL before the search for a new page.

	// Starting from ODS 11.1 we store in pip_header.reserved number of pages
	// allocated from this pointer page. There is intention to create dedicated
	// field at page_inv_page for this purpose in ODS 12.
	const bool isODS11_x = (dbb->dbb_ods_version == ODS_VERSION11 && dbb->dbb_minor_version >= 1);
	bool pipMarked = false;

	// Find an allocation page with something on it

	SLONG relative_bit = -1;
	SLONG sequence;
	SLONG pipMin;
	for (sequence = pageSpace->pipHighWater; true; sequence++)
	{
		pip_window.win_page = (sequence == 0) ?
			pageSpace->ppFirst : sequence * dbb->dbb_page_manager.pagesPerPIP - 1;
		page_inv_page* pip_page = (page_inv_page*) CCH_FETCH(tdbb, &pip_window, LCK_write, pag_pages);

		pipMin = MAX_SLONG;
		const UCHAR* end = (UCHAR*) pip_page + dbb->dbb_page_size;
		for (bytes = &pip_page->pip_bits[pip_page->pip_min >> 3]; bytes < end; bytes++)
		{
			if (*bytes != 0)
			{
				// 'byte' is not zero, so it describes at least one free page.
				bit = 1;
				for (SLONG i = 0; i < 8; i++, bit <<= 1)
				{
					if (bit & *bytes)
					{
						relative_bit = ((bytes - pip_page->pip_bits) << 3) + i;
						pipMin = MIN(pipMin, relative_bit);

						const SLONG pageNum = relative_bit + sequence * pageMgr.pagesPerPIP;
						window->win_page = pageNum;
						new_page = CCH_fake(tdbb, window, 0);	// don't wait on latch
						if (new_page)
						{
							if (!isODS11_x)
								break;

							BackupManager::StateReadGuard stateGuard(tdbb);
							const bool nbak_stalled =
								dbb->dbb_backup_manager->getState() == nbak_state_stalled;

							USHORT next_init_pages = 1;
							// ensure there are space on disk for faked page
							if (relative_bit + 1 > pip_page->pip_header.reserved)
							{
								fb_assert(relative_bit == pip_page->pip_header.reserved);

								USHORT init_pages = 0;
								if (!nbak_stalled)
								{
									init_pages = 1;
									if (!(dbb->dbb_flags & DBB_no_reserve))
									{
										const int minExtendPages =
											MIN_EXTEND_BYTES / dbb->dbb_page_size;

										init_pages = sequence ?
											64 : MIN(pip_page->pip_header.reserved / 16, 64);

										// don't touch pages belongs to the next PIP
										init_pages = MIN(init_pages,
											pageMgr.pagesPerPIP - pip_page->pip_header.reserved);

										if (init_pages < minExtendPages)
											init_pages = 1;

										next_init_pages = init_pages;
									}

									ISC_STATUS_ARRAY status;
									const ULONG start = sequence * pageMgr.pagesPerPIP +
										pip_page->pip_header.reserved;

									init_pages = PIO_init_data(dbb, pageSpace->file, status,
										start, init_pages);
								}

								if (init_pages) 
								{
									CCH_MARK(tdbb, &pip_window);
									pipMarked = true;
									pip_page->pip_header.reserved += init_pages;
								}
								else
								{
									// PIO_init_data returns zero - perhaps it is not supported,
									// no space left on disk or IO error occurred. Try to write
									// one page and handle IO errors if any.
									CCH_must_write(window);
									try
									{
										CCH_RELEASE(tdbb, window);
									}
									catch (Firebird::status_exception)
									{
										// forget about this page as if we never tried to fake it
										CCH_forget_page(tdbb, window);

										// normally all page buffers now released by CCH_unwind
										// only exception is when TDBB_no_cache_unwind flag is set
										if (tdbb->tdbb_flags & TDBB_no_cache_unwind)
											CCH_RELEASE(tdbb, &pip_window);

										throw;
									}

									CCH_MARK(tdbb, &pip_window);
									pipMarked = true;
									pip_page->pip_header.reserved = relative_bit + 1;

									new_page = CCH_fake(tdbb, window, 1);
								}

								fb_assert(new_page);
							}

							if (!(dbb->dbb_flags & DBB_no_reserve) && !nbak_stalled)
							{
								const ULONG initialized =
									sequence * pageMgr.pagesPerPIP + pip_page->pip_header.reserved;

								// At this point we ensure database has at least "initialized" pages
								// allocated. To avoid file growth by few pages when all this space
								// will be used, extend file up to initialized + next_init_pages now
								pageSpace->extend(tdbb, initialized + next_init_pages);
							}

							break;	// Found a page and successfully fake-ed it
						}
					}
				}
			}
			if (new_page)
				break;	// Found a page and successfully fake-ed it
		}

		if (new_page)
			break;		// Found a page and successfully fake-ed it

		CCH_RELEASE(tdbb, &pip_window);
	}

	pageSpace->pipHighWater = sequence;

	if (!pipMarked) {
		CCH_MARK(tdbb, &pip_window);
	}
	*bytes &= ~bit;
	page_inv_page* pip_page = (page_inv_page*) pip_window.win_buffer;

	if (pipMin == relative_bit)
		pipMin++;
	pip_page->pip_min = pipMin;

	if (relative_bit != pageMgr.pagesPerPIP - 1)
	{
		CCH_RELEASE(tdbb, &pip_window);
		CCH_precedence(tdbb, window, pip_window.win_page);
#ifdef VIO_DEBUG
		if (debug_flag > DEBUG_WRITES_INFO)
			printf("\tPAG_allocate:  allocated page %"SLONGFORMAT"\n",
					  window->win_page.getPageNum());
#endif
		return new_page;
	}

	// We've allocated the last page on the space management page. Rather
	// than returning it, format it as a page inventory page, and recurse.

	page_inv_page* new_pip_page = (page_inv_page*) new_page;
	new_pip_page->pip_header.pag_type = pag_pages;
	const UCHAR* end = (UCHAR*) new_pip_page + dbb->dbb_page_size;
	memset(new_pip_page->pip_bits, 0xff, end - new_pip_page->pip_bits);

	CCH_must_write(window);
	CCH_RELEASE(tdbb, window);
	CCH_must_write(&pip_window);
	CCH_RELEASE(tdbb, &pip_window);

	return PAG_allocate(tdbb, window);
}


SLONG PAG_attachment_id(thread_db* tdbb)
{
/******************************************
 *
 *	P A G _ a t t a c h m e n t _ i d
 *
 ******************************************
 *
 * Functional description
 *	Get attachment id.  If don't have one, get one.  As a side
 *	effect, get a lock on it as well.
 *
 ******************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	Attachment* attachment = tdbb->getAttachment();
	WIN window(DB_PAGE_SPACE, -1);

	// If we've been here before just return the id

	if (attachment->att_id_lock)
		return attachment->att_attachment_id;

	// Get new attachment id

	if (dbb->dbb_flags & DBB_read_only) {
		attachment->att_attachment_id = dbb->dbb_attachment_id + dbb->generateAttachmentId(tdbb);
	}
	else
	{
		window.win_page = HEADER_PAGE_NUMBER;
		header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
		CCH_MARK(tdbb, &window);
		attachment->att_attachment_id = ++header->hdr_attachment_id;

		CCH_RELEASE(tdbb, &window);
	}

	// Take out lock on attachment id

	Lock* lock = FB_NEW_RPT(*attachment->att_pool, sizeof(SLONG)) Lock();
	attachment->att_id_lock = lock;
	lock->lck_type = LCK_attachment;
	lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
	lock->lck_parent = dbb->dbb_lock;
	lock->lck_length = sizeof(SLONG);
	lock->lck_key.lck_long = attachment->att_attachment_id;
	lock->lck_dbb = dbb;
	lock->lck_ast = blocking_ast_attachment;
	lock->lck_object = attachment;
	LCK_lock(tdbb, lock, LCK_EX, LCK_WAIT);

	return attachment->att_attachment_id;
}


bool PAG_delete_clump_entry(thread_db* tdbb, SLONG page_num, USHORT type)
{
/***********************************************
 *
 *	P A G _ d e l e t e _ c l u m p _ e n t r y
 *
 ***********************************************
 *
 * Functional description
 *	Gets rid on the entry 'type' from page.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	WIN window(DB_PAGE_SPACE, page_num);

	pag* page;
	if (page_num == HEADER_PAGE)
		page = CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	else
		page = CCH_FETCH(tdbb, &window, LCK_write, pag_log);

	UCHAR* entry_p;
	const UCHAR* clump_end;
	if (!find_type(tdbb, page_num, &window, &page, LCK_write, type, &entry_p, &clump_end))
	{
		CCH_RELEASE(tdbb, &window);
		return false;
	}
	CCH_MARK(tdbb, &window);

	header_page* header = 0;
	log_info_page* logp = 0;
	USHORT* end_addr;
	if (page_num == HEADER_PAGE)
	{
		header = (header_page*) page;
		end_addr = &header->hdr_end;
	}
	else
	{
		logp = (log_info_page*) page;
		end_addr = &logp->log_end;
	}

	*end_addr -= (2 + entry_p[1]);

	const UCHAR* r = entry_p + 2 + entry_p[1];
	USHORT l = clump_end - r + 1;
	if (l)
		memmove(entry_p, r, l);

	CCH_RELEASE(tdbb, &window);

	return true;
}


void PAG_format_header(thread_db* tdbb)
{
/**************************************
 *
 *	 P A G _ f o r m a t _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Create the header page for a new file.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Initialize header page

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_fake(tdbb, &window, 1);
	header->hdr_header.pag_scn = 0;
	*(ISC_TIMESTAMP*) header->hdr_creation_date = Firebird::TimeStamp::getCurrentTimeStamp().value();
	// should we include milliseconds or not?
	//Firebird::TimeStamp::round_time(header->hdr_creation_date->timestamp_time, 0);
	header->hdr_header.pag_type = pag_header;
	header->hdr_page_size = dbb->dbb_page_size;
	header->hdr_ods_version = ODS_VERSION | ODS_FIREBIRD_FLAG;
	header->hdr_implementation = CLASS;
	header->hdr_ods_minor = ODS_CURRENT;
	header->hdr_ods_minor_original = ODS_CURRENT;
	header->hdr_oldest_transaction = 1;
	header->hdr_bumped_transaction = 1;
	header->hdr_end = HDR_SIZE;
	header->hdr_data[0] = HDR_end;
	header->hdr_flags |= hdr_force_write;

	if (dbb->dbb_flags & DBB_DB_SQL_dialect_3) {
		header->hdr_flags |= hdr_SQL_dialect_3;
	}

	dbb->dbb_ods_version = header->hdr_ods_version & ~ODS_FIREBIRD_FLAG;
	dbb->dbb_minor_version = header->hdr_ods_minor;
	dbb->dbb_minor_original = header->hdr_ods_minor_original;

	CCH_RELEASE(tdbb, &window);
}


// CVC: This function is mostly obsolete. Ann requested to keep it and the code that calls it.
// We won't read the log, anyway.
void PAG_format_log(thread_db* tdbb)
{
/***********************************************
 *
 *	P A G _ f o r m a t _ l o g
 *
 ***********************************************
 *
 * Functional description
 *	Initialize log page.
 *	Set all parameters to 0
 *
 **************************************/
	SET_TDBB(tdbb);

	WIN window(LOG_PAGE_NUMBER);
	log_info_page* logp = (log_info_page*) CCH_fake(tdbb, &window, 1);
	logp->log_header.pag_type = pag_log;

	CCH_RELEASE(tdbb, &window);
}


void PAG_format_pip(thread_db* tdbb, PageSpace& pageSpace)
{
/**************************************
 *
 *	 P A G _ f o r m a t _ p i p
 *
 **************************************
 *
 * Functional description
 *	Create a page inventory page to
 *	complete the formatting of a new file
 *	into a rudimentary database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	// Initialize Page Inventory Page

	WIN window(pageSpace.pageSpaceID, 1);
	pageSpace.ppFirst = 1;
	page_inv_page* pages = (page_inv_page*) CCH_fake(tdbb, &window, 1);

	pages->pip_header.pag_type = pag_pages;
	pages->pip_min = 4;
	pages->pip_header.reserved = pages->pip_min - 1;
	UCHAR* p = pages->pip_bits;
	int i = dbb->dbb_page_size - OFFSETA(page_inv_page*, pip_bits);

	while (i--) {
		*p++ = 0xff;
	}

	pages->pip_bits[0] &= ~(1 | 2 | 4);

	CCH_RELEASE(tdbb, &window);
}


bool PAG_get_clump(thread_db* tdbb, SLONG page_num, USHORT type, USHORT* inout_len, UCHAR* entry)
{
/***********************************************
 *
 *	P A G _ g e t _ c l u m p
 *
 ***********************************************
 *
 * Functional description
 *	Find 'type' clump in page_num
 *		true  - Found it
 *		false - Not present
 *	RETURNS
 *		value of clump in entry
 *		length in inout_len  <-> input and output value to avoid B.O.
 *
 **************************************/
	SET_TDBB(tdbb);

	WIN window(DB_PAGE_SPACE, page_num);

	pag* page;
	if (page_num == HEADER_PAGE)
		page = CCH_FETCH(tdbb, &window, LCK_read, pag_header);
	else
		page = CCH_FETCH(tdbb, &window, LCK_read, pag_log);

	UCHAR* entry_p;
	const UCHAR* dummy;
	if (!find_type(tdbb, page_num, &window, &page, LCK_read, type, &entry_p, &dummy))
	{
		CCH_RELEASE(tdbb, &window);
		*inout_len = 0;
		return false;
	}

	USHORT old_len = *inout_len;
	*inout_len = entry_p[1];
	entry_p += 2;

	if (*inout_len)
	{
		// Avoid the B.O. but inform the caller the buffer is bigger.
		if (*inout_len < old_len)
			old_len = *inout_len;
		memcpy(entry, entry_p, old_len);
	}

	CCH_RELEASE(tdbb, &window);

	return true;
}


void PAG_header(thread_db* tdbb, bool info)
{
/**************************************
 *
 *	P A G _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Checkout database header page.
 *  Done through the page cache.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	Attachment* attachment = tdbb->getAttachment();
	fb_assert(attachment);

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_header);

	try {

	if (header->hdr_next_transaction)
	{
		if (header->hdr_oldest_active > header->hdr_next_transaction)
			BUGCHECK(266);		// next transaction older than oldest active

		if (header->hdr_oldest_transaction > header->hdr_next_transaction)
			BUGCHECK(267);		// next transaction older than oldest transaction
	}

	if (header->hdr_flags & hdr_SQL_dialect_3)
		dbb->dbb_flags |= DBB_DB_SQL_dialect_3;

	jrd_rel* relation = MET_relation(tdbb, 0);
	RelationPages* relPages = relation->getBasePages();
	if (!relPages->rel_pages)
	{
		// 21-Dec-2003 Nickolay Samofatov
		// No need to re-set first page for RDB$PAGES relation since
		// current code cannot change its location after database creation.
		// Currently, this change only affects isc_database_info call,
		// the only call which may call PAG_header multiple times.
		// In fact, this isc_database_info behavior seems dangerous to me,
		// but let somebody else fix that problem, I just fix the memory leak.
		vcl* vector = vcl::newVector(*dbb->dbb_permanent, 1);
		relPages->rel_pages = vector;
		(*vector)[0] = header->hdr_PAGES;
	}

	dbb->dbb_next_transaction = header->hdr_next_transaction;

	if (!info || dbb->dbb_oldest_transaction < header->hdr_oldest_transaction) {
		dbb->dbb_oldest_transaction = header->hdr_oldest_transaction;
	}
	if (!info || dbb->dbb_oldest_active < header->hdr_oldest_active) {
		dbb->dbb_oldest_active = header->hdr_oldest_active;
	}
	if (!info || dbb->dbb_oldest_snapshot < header->hdr_oldest_snapshot) {
		dbb->dbb_oldest_snapshot = header->hdr_oldest_snapshot;
	}

	dbb->dbb_attachment_id = header->hdr_attachment_id;
	dbb->dbb_creation_date = *(ISC_TIMESTAMP*) header->hdr_creation_date;

	if (header->hdr_flags & hdr_read_only)
	{
		// If Header Page flag says the database is ReadOnly, gladly accept it.
		dbb->dbb_flags &= ~DBB_being_opened_read_only;
		dbb->dbb_flags |= DBB_read_only;
	}

	// If hdr_read_only is not set...
	if (!(header->hdr_flags & hdr_read_only) && (dbb->dbb_flags & DBB_being_opened_read_only))
	{
		// Looks like the Header page says, it is NOT ReadOnly!! But the database
		// file system permission gives only ReadOnly access. Punt out with
		// isc_no_priv error (no privileges)
		ERR_post(Arg::Gds(isc_no_priv) << Arg::Str("read-write") <<
										  Arg::Str("database") <<
										  Arg::Str(attachment->att_filename));
	}

	const bool useFSCache = dbb->dbb_bcb->bcb_count < Config::getFileSystemCacheThreshold();

	if ((header->hdr_flags & hdr_force_write) || !useFSCache)
	{
		dbb->dbb_flags |=
			(header->hdr_flags & hdr_force_write ? DBB_force_write : 0) |
			(useFSCache ? 0 : DBB_no_fs_cache);

		PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
		for (jrd_file* file = pageSpace->file; file; file = file->fil_next)
		{
			PIO_force_write(file,
				(dbb->dbb_flags & DBB_force_write) && !(header->hdr_flags & hdr_read_only),
				dbb->dbb_flags & DBB_no_fs_cache);
		}
	}

	if (header->hdr_flags & hdr_no_reserve)
		dbb->dbb_flags |= DBB_no_reserve;

	const USHORT sd_flags = header->hdr_flags & hdr_shutdown_mask;
	if (sd_flags)
	{
		dbb->dbb_ast_flags |= DBB_shutdown;
		if (sd_flags == hdr_shutdown_full)
			dbb->dbb_ast_flags |= DBB_shutdown_full;
		else if (sd_flags == hdr_shutdown_single)
			dbb->dbb_ast_flags |= DBB_shutdown_single;
	}

	}	// try
	catch (const Firebird::Exception&)
	{
		CCH_RELEASE(tdbb, &window);
		throw;
	}

	CCH_RELEASE(tdbb, &window);
}


void PAG_header_init(thread_db* tdbb)
{
/**************************************
 *
 *	P A G _ h e a d e r _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Checkout the core part of the database header page.
 *  It includes the fields required to setup the I/O layer:
 *    ODS version, page size, page buffers.
 *  Done using a physical page read.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();

	Attachment* const attachment = tdbb->getAttachment();
	fb_assert(attachment);

	// Allocate a spare buffer which is large enough,
	// and set up to release it in case of error; note
	// that dbb_page_size has not been set yet, so we
	// can't depend on this.
	//
	// Make sure that buffer is aligned on a page boundary
	// and unit of transfer is a multiple of physical disk
	// sector for raw disk access.

	SCHAR temp_buffer[2 * MIN_PAGE_SIZE];
	SCHAR* temp_page = (SCHAR*) FB_ALIGN((IPTR) temp_buffer, MIN_PAGE_SIZE);

	PIO_header(dbb, temp_page, MIN_PAGE_SIZE);
	const header_page* header = (header_page*) temp_page;

	if (header->hdr_header.pag_type != pag_header || header->hdr_sequence) {
		ERR_post(Arg::Gds(isc_bad_db_format) << Arg::Str(attachment->att_filename));
	}

	const USHORT ods_version = header->hdr_ods_version & ~ODS_FIREBIRD_FLAG;

	if (!Ods::isSupported(header->hdr_ods_version, header->hdr_ods_minor))
	{
		ERR_post(Arg::Gds(isc_wrong_ods) << Arg::Str(attachment->att_filename) <<
											Arg::Num(ods_version) <<
											Arg::Num(header->hdr_ods_minor) <<
											Arg::Num(ODS_VERSION) <<
											Arg::Num(ODS_CURRENT));
	}

	// Note that if this check is turned on, it should be recoded in order that
	// the Intel platforms can share databases.  At present (Feb 95) it is possible
	// to share databases between Windows and NT, but not with NetWare.  Sharing
	// databases with OS/2 is unknown and needs to be investigated.  The CLASS was
	// initially 8 for all Intel platforms, but was changed after 4.0 was released
	// in order to allow differentiation between databases created on various
	// platforms.  This should allow us in future to identify where databases were
	// created.  Even when we get to the stage where databases created on PC platforms
	// are sharable between all platforms, it would be useful to identify where they
	// were created for debugging purposes.  - Deej 2/6/95
	//
	// Re-enable and recode the check to avoid BUGCHECK messages when database
	// is accessed with engine built for another architecture. - Nickolay 9-Feb-2005

	if (header->hdr_implementation != CLASS)
	{
		const int classmax = ods_version < ODS_VERSION11 ? CLASS_MAX10 : CLASS_MAX;
		const ArchitectureType* matrix = ods_version < ODS_VERSION11 ? archMatrix10 : archMatrix;
		const int hdrImpl = header->hdr_implementation;
		if (hdrImpl < 0 || hdrImpl > classmax ||
			matrix[hdrImpl] == archUnknown || matrix[hdrImpl] != matrix[CLASS])
		{
			ERR_post(Arg::Gds(isc_bad_db_format) << Arg::Str(attachment->att_filename));
		}
	}

	if (header->hdr_page_size < MIN_PAGE_SIZE || header->hdr_page_size > MAX_PAGE_SIZE)
	{
		ERR_post(Arg::Gds(isc_bad_db_format) << Arg::Str(attachment->att_filename));
	}

	dbb->dbb_ods_version = ods_version;
	dbb->dbb_minor_version = header->hdr_ods_minor;
	dbb->dbb_minor_original = header->hdr_ods_minor_original;

	dbb->dbb_page_size = header->hdr_page_size;
	dbb->dbb_page_buffers = header->hdr_page_buffers;
}


void PAG_init(thread_db* tdbb)
{
/**************************************
 *
 *	P A G _ i n i t
 *
 **************************************
 *
 * Functional description
 *	Initialize stuff for page handling.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	PageManager& pageMgr = dbb->dbb_page_manager;
	PageSpace* pageSpace = pageMgr.findPageSpace(DB_PAGE_SPACE);
	fb_assert(pageSpace);

	pageMgr.bytesBitPIP = dbb->dbb_page_size - OFFSETA(page_inv_page*, pip_bits);
	pageMgr.pagesPerPIP = pageMgr.bytesBitPIP * 8;
	pageMgr.transPerTIP = (dbb->dbb_page_size - OFFSETA(tx_inv_page*, tip_transactions)) * 4;
	pageSpace->ppFirst = 1;
	// dbb_ods_version can be 0 when a new database is being created
	if ((dbb->dbb_ods_version == 0) || (dbb->dbb_ods_version >= ODS_VERSION10))
	{
		pageMgr.gensPerPage =
			(dbb->dbb_page_size -
			 OFFSETA(generator_page*, gpg_values)) / sizeof(((generator_page*) NULL)->gpg_values);
	}
	else
	{
		pageMgr.gensPerPage =
			(dbb->dbb_page_size -
			 OFFSETA(pointer_page*, ppg_page)) / sizeof(((pointer_page*) NULL)->ppg_page);
	}


	// Compute the number of data pages per pointer page.  Each data page
	// requires a 32 bit pointer and a 2 bit control field.

	dbb->dbb_dp_per_pp =
		(dbb->dbb_page_size - OFFSETA(pointer_page*, ppg_page)) * 8 / (BITS_PER_LONG + 2);

	// Compute the number of records that can fit on a page using the
	// size of the record index (dpb_repeat) and a record header.  This
	// gives an artificially high number, reducing the density of db_keys.

	dbb->dbb_max_records = (dbb->dbb_page_size - sizeof(data_page)) /
		(sizeof(data_page::dpg_repeat) + OFFSETA(rhd*, rhd_data));

	// Artifically reduce density of records to test high bits of record number
	// dbb->dbb_max_records = 32000;

	// Optimize record numbers for new 64-bit sparse bitmap implementation
	// We need to measure if it is beneficial from performance point of view.
	// Price is slightly reduced density of record numbers, but for
	// ODS11 it doesn't matter because record numbers are 40-bit.
	// Benefit is ~1.5 times smaller sparse bitmaps on average and faster bitmap iteration.

	//if (dbb->dbb_ods_version >= ODS_VERSION11)
	//	dbb->dbb_max_records = FB_ALIGN(dbb->dbb_max_records, 64);

	// Compute the number of index roots that will fit on an index root page,
	// assuming that each index has only one key

	dbb->dbb_max_idx = (dbb->dbb_page_size - OFFSETA(index_root_page*, irt_rpt)) /
		(sizeof(index_root_page::irt_repeat) + (1 * (dbb->dbb_ods_version >= ODS_VERSION11) ?
			sizeof(irtd) : sizeof(irtd_ods10)));

	// Compute prefetch constants from database page size and maximum prefetch
	// transfer size. Double pages per prefetch request so that cache reader
	// can overlap prefetch I/O with database computation over previously prefetched pages.
#ifdef SUPERSERVER_V2
	dbb->dbb_prefetch_sequence = PREFETCH_MAX_TRANSFER / dbb->dbb_page_size;
	dbb->dbb_prefetch_pages = dbb->dbb_prefetch_sequence * 2;
#endif
}


void PAG_init2(thread_db* tdbb, USHORT shadow_number)
{
/**************************************
 *
 *	P A G _ i n i t 2
 *
 **************************************
 *
 * Functional description
 *	Perform second phase of page initialization -- the eternal
 *	search for additional files.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* const dbb = tdbb->getDatabase();
	ISC_STATUS* status = tdbb->tdbb_status_vector;

	// allocate a spare buffer which is large enough,
	// and set up to release it in case of error. Align
	// the temporary page buffer for raw disk access.

	SCHAR* const temp_buffer = FB_NEW(*getDefaultMemoryPool()) SCHAR[dbb->dbb_page_size + MIN_PAGE_SIZE];
	SCHAR* temp_page =
		(SCHAR*) (((U_IPTR) temp_buffer + MIN_PAGE_SIZE - 1) & ~((U_IPTR) MIN_PAGE_SIZE - 1));

	try {

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	jrd_file* file = pageSpace->file;
	if (shadow_number)
	{
		Shadow* shadow = dbb->dbb_shadow;
		for (; shadow; shadow = shadow->sdw_next)
		{
			if (shadow->sdw_number == shadow_number)
			{
				file = shadow->sdw_file;
				break;
			}
		}
		if (!shadow)
			BUGCHECK(161);		// msg 161 shadow block not found
	}

	USHORT sequence = 1;
	WIN window(DB_PAGE_SPACE, -1);

	TEXT buf[MAXPATHLEN + 1];

	// Loop thru files and header pages until everything is open

	for (;;)
	{
		TEXT* file_name = NULL;
		window.win_page = file->fil_min_page;
		USHORT file_length = 0;
		ULONG last_page = 0;
		BufferDesc temp_bdb;
		SLONG next_page = 0;
		do {
			// note that we do not have to get a read lock on
			// the header page (except for header page 0) because
			// the only time it will be modified is when adding a file,
			// which must be done with an exclusive lock on the database --
			// if this changes, this policy will have to be reevaluated;
			// at any rate there is a problem with getting a read lock
			// because the corresponding page in the main database file may not exist

			if (!file->fil_min_page)
				CCH_FETCH(tdbb, &window, LCK_read, pag_header);

			header_page* header = (header_page*) temp_page;
			temp_bdb.bdb_buffer = (PAG) header;
			temp_bdb.bdb_page = window.win_page;
			temp_bdb.bdb_dbb = dbb;

			// Read the required page into the local buffer
			PIO_read(file, &temp_bdb, (PAG) header, status);

			if (shadow_number && !file->fil_min_page)
				CCH_RELEASE(tdbb, &window);

			for (const UCHAR* p = header->hdr_data; *p != HDR_end; p += 2 + p[1])
			{
				switch (*p)
				{
				case HDR_file:
					file_length = p[1];
					file_name = buf;
					memcpy(buf, p + 2, file_length);
					break;

				case HDR_last_page:
					memcpy(&last_page, p + 2, sizeof(last_page));
					break;

				case HDR_sweep_interval:
					// CVC: Let's copy it always.
					//if (!(dbb->dbb_flags & DBB_read_only))
						memcpy(&dbb->dbb_sweep_interval, p + 2, sizeof(SLONG));
					break;
				}
			}

			next_page = header->hdr_next_page;

			if (!shadow_number && !file->fil_min_page)
				CCH_RELEASE(tdbb, &window);

			window.win_page = next_page;

			// Make sure the header page and all the overflow header
			// pages are traversed.  For V4.0, only the header page for
			// the primary database page will have overflow pages.

		} while (next_page);

		if (file->fil_min_page)
			file->fil_fudge = 1;
		if (!file_name)
			break;

		// Verify database file path against DatabaseAccess entry of firebird.conf
		file_name[file_length] = 0;
		if (!JRD_verify_database_access(file_name))
		{
			ERR_post(Arg::Gds(isc_conf_access_denied) << Arg::Str("additional database file") <<
														 Arg::Str(file_name));
		}

		file->fil_next = PIO_open(dbb, file_name, file_name, false);
		file->fil_max_page = last_page;
		file = file->fil_next;
		if (dbb->dbb_flags & (DBB_force_write | DBB_no_fs_cache))
		{
			PIO_force_write(file, dbb->dbb_flags & DBB_force_write, dbb->dbb_flags & DBB_no_fs_cache);
		}
		file->fil_min_page = last_page + 1;
		file->fil_sequence = sequence++;
	}

	delete[] temp_buffer;
	}	// try
	catch (const Firebird::Exception&)
	{
		delete[] temp_buffer;
		throw;
	}
}


SLONG PAG_last_page(thread_db* tdbb)
{
/**************************************
 *
 *	P A G _ l a s t _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Compute the highest page allocated.  This is called by the
 *	shadow stuff to dump a database.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	PageManager& pageMgr = dbb->dbb_page_manager;
	PageSpace* pageSpace = pageMgr.findPageSpace(DB_PAGE_SPACE);
	fb_assert(pageSpace);

	const ULONG pages_per_pip = pageMgr.pagesPerPIP;
	WIN window(DB_PAGE_SPACE, -1);

	// Find the last page allocated

	ULONG relative_bit = 0;
	USHORT sequence;
	for (sequence = 0; true; ++sequence)
	{
		window.win_page = (!sequence) ? pageSpace->ppFirst : sequence * pages_per_pip - 1;
		const page_inv_page* page = (page_inv_page*) CCH_FETCH(tdbb, &window, LCK_read, pag_pages);
		const UCHAR* bits = page->pip_bits + (pages_per_pip >> 3) - 1;
		while (*bits == (UCHAR) - 1)
			--bits;
		SSHORT bit;
		for (bit = 7; bit >= 0; --bit)
		{
			if (!(*bits & (1 << bit)))
				break;
		}
		relative_bit = (bits - page->pip_bits) * 8 + bit;
		CCH_RELEASE(tdbb, &window);
		if (relative_bit != pages_per_pip - 1)
			break;
	}

	return sequence * pages_per_pip + relative_bit;
}


void PAG_release_page(thread_db* tdbb, const PageNumber& number, const PageNumber& prior_page)
{
/**************************************
 *
 *	P A G _ r e l e a s e _ p a g e
 *
 **************************************
 *
 * Functional description
 *	Release a page to the free page page.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

#ifdef VIO_DEBUG
	if (debug_flag > DEBUG_WRITES_INFO)
		printf("\tPAG_release_page:  about to release page %"SLONGFORMAT"\n", number.getPageNum());
#endif

	PageManager& pageMgr = dbb->dbb_page_manager;
	PageSpace* pageSpace = pageMgr.findPageSpace(number.getPageSpaceID());
	fb_assert(pageSpace);

	const SLONG sequence = number.getPageNum() / pageMgr.pagesPerPIP;
	const SLONG relative_bit = number.getPageNum() % pageMgr.pagesPerPIP;

	WIN pip_window(number.getPageSpaceID(), (sequence == 0) ?
		pageSpace->ppFirst : sequence * pageMgr.pagesPerPIP - 1);

	page_inv_page* pages = (page_inv_page*) CCH_FETCH(tdbb, &pip_window, LCK_write, pag_pages);
	CCH_precedence(tdbb, &pip_window, prior_page);
	CCH_MARK(tdbb, &pip_window);
	pages->pip_bits[relative_bit >> 3] |= 1 << (relative_bit & 7);
	pages->pip_min = MIN(pages->pip_min, relative_bit);

	CCH_RELEASE(tdbb, &pip_window);

	pageSpace->pipHighWater = MIN(pageSpace->pipHighWater, sequence);
}


void PAG_set_force_write(thread_db* tdbb, bool flag)
{
/**************************************
 *
 *	P A G _ s e t _ f o r c e _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Turn on/off force write.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	err_post_if_database_is_readonly(dbb);

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);

	if (flag)
	{
		header->hdr_flags |= hdr_force_write;
		dbb->dbb_flags |= DBB_force_write;
	}
	else
	{
		header->hdr_flags &= ~hdr_force_write;
		dbb->dbb_flags &= ~DBB_force_write;
	}

	CCH_RELEASE(tdbb, &window);

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	for (jrd_file* file = pageSpace->file; file; file = file->fil_next) {
		PIO_force_write(file, flag, dbb->dbb_flags & DBB_no_fs_cache);
	}

	for (Shadow* shadow = dbb->dbb_shadow; shadow; shadow = shadow->sdw_next)
	{
		for (jrd_file* file = shadow->sdw_file; file; file = file->fil_next) {
			PIO_force_write(file, flag, dbb->dbb_flags & DBB_no_fs_cache);
		}
	}
}


void PAG_set_no_reserve(thread_db* tdbb, bool flag)
{
/**************************************
 *
 *	P A G _ s e t _ n o _ r e s e r v e
 *
 **************************************
 *
 * Functional description
 *	Turn on/off reserving space for versions
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	err_post_if_database_is_readonly(dbb);

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);

	if (flag)
	{
		header->hdr_flags |= hdr_no_reserve;
		dbb->dbb_flags |= DBB_no_reserve;
	}
	else
	{
		header->hdr_flags &= ~hdr_no_reserve;
		dbb->dbb_flags &= ~DBB_no_reserve;
	}

	CCH_RELEASE(tdbb, &window);
}


void PAG_set_db_readonly(thread_db* tdbb, bool flag)
{
/*********************************************
 *
 *	P A G _ s e t _ d b _ r e a d o n l y
 *
 *********************************************
 *
 * Functional description
 *	Set database access mode to readonly OR readwrite
 *
 *********************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);

	if (!flag)
	{
		// If the database is transitioning from RO to RW, reset the
		// in-memory Database flag which indicates that the database is RO.
		// This will allow the CCH subsystem to allow pages to be MARK'ed
		// for WRITE operations
		header->hdr_flags &= ~hdr_read_only;
		dbb->dbb_flags &= ~DBB_read_only;
	}

	CCH_MARK_MUST_WRITE(tdbb, &window);

	if (flag)
	{
		header->hdr_flags |= hdr_read_only;
		dbb->dbb_flags |= DBB_read_only;
	}

	CCH_RELEASE(tdbb, &window);
}


void PAG_set_db_SQL_dialect(thread_db* tdbb, SSHORT flag)
{
/*********************************************
 *
 *	P A G _ s e t _ d b _ S Q L _ d i a l e c t
 *
 *********************************************
 *
 * Functional description
 *	Set database SQL dialect to SQL_DIALECT_V5 or SQL_DIALECT_V6
 *
 *********************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();

	const USHORT major_version = dbb->dbb_ods_version;
	const USHORT minor_original = dbb->dbb_minor_original;

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);

	if (flag && (ENCODE_ODS(major_version, minor_original) >= ODS_10_0))
	{
		switch (flag)
		{
		case SQL_DIALECT_V5:

			if (dbb->dbb_flags & DBB_DB_SQL_dialect_3 || header->hdr_flags & hdr_SQL_dialect_3)
			{
				// Check the returned value here!
				ERR_post_warning(Arg::Warning(isc_dialect_reset_warning));
			}

			dbb->dbb_flags &= ~DBB_DB_SQL_dialect_3;	// set to 0
			header->hdr_flags &= ~hdr_SQL_dialect_3;	// set to 0
			break;

		case SQL_DIALECT_V6:
			dbb->dbb_flags |= DBB_DB_SQL_dialect_3;	// set to dialect 3
			header->hdr_flags |= hdr_SQL_dialect_3;	// set to dialect 3
			break;

		default:
			CCH_RELEASE(tdbb, &window);
			ERR_post(Arg::Gds(isc_inv_dialect_specified) << Arg::Num(flag) <<
					 Arg::Gds(isc_valid_db_dialects) << Arg::Str("1 and 3") <<
					 Arg::Gds(isc_dialect_not_changed));
			break;
		}
	}

	CCH_MARK_MUST_WRITE(tdbb, &window);

	CCH_RELEASE(tdbb, &window);
}


void PAG_set_page_buffers(thread_db* tdbb, ULONG buffers)
{
/**************************************
 *
 *	P A G _ s e t _ p a g e _ b u f f e r s
 *
 **************************************
 *
 * Functional description
 *	Set database-specific page buffer cache
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	err_post_if_database_is_readonly(dbb);

	WIN window(HEADER_PAGE_NUMBER);
	header_page* header = (header_page*) CCH_FETCH(tdbb, &window, LCK_write, pag_header);
	CCH_MARK_MUST_WRITE(tdbb, &window);
	header->hdr_page_buffers = buffers;
	CCH_RELEASE(tdbb, &window);
}


void PAG_sweep_interval(thread_db* tdbb, SLONG interval)
{
/**************************************
 *
 *	P A G _ s w e e p _ i n t e r v a l
 *
 **************************************
 *
 * Functional description
 *	Set sweep interval.
 *
 **************************************/

 	SET_TDBB(tdbb);
	PAG_add_clump(tdbb, HEADER_PAGE, HDR_sweep_interval, sizeof(SLONG),
				  (UCHAR*) &interval, CLUMP_REPLACE); //, true
}


static int blocking_ast_attachment(void* ast_object)
{
	Attachment* const attachment = static_cast<Attachment*>(ast_object);

	try
	{
		Database* const dbb = attachment->att_database;
		Database::SyncGuard dsGuard(dbb, true);

		ThreadContextHolder tdbb;
		tdbb->setDatabase(dbb);
		tdbb->setAttachment(attachment);

		Jrd::ContextPoolHolder context(tdbb, dbb->dbb_permanent);

		attachment->att_flags |= ATT_shutdown;
		attachment->cancelExternalConnection(tdbb);
		LCK_cancel_wait(attachment);

		JRD_shutdown_attachments(dbb);

		LCK_release(tdbb, attachment->att_id_lock);
	}
	catch (const Firebird::Exception&)
	{} // no-op

	return 0;
}


static void find_clump_space(thread_db* tdbb,
							 SLONG page_num,
							 WIN* window,
							 PAG* ppage,
							 USHORT type,
							 USHORT len,
							 const UCHAR* entry) //USHORT must_write
{
/***********************************************
 *
 *	f i n d _ c l u m p _ s p a c e
 *
 ***********************************************
 *
 * Functional description
 *	Find space for the new clump.
 *	Add the entry at the end of clumplet list.
 *	Allocate a new page if required.
 *
 **************************************/
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	CHECK_DBB(dbb);

	pag* page = *ppage;
	header_page* header = 0; // used after the loop
	log_info_page* logp = 0; // used after the loop

	while (true)
	{
		SLONG next_page, free_space;
		USHORT* end_addr;
		UCHAR* p;

		if (page_num == HEADER_PAGE)
		{
			header = (header_page*) page;
			next_page = header->hdr_next_page;
			free_space = dbb->dbb_page_size - header->hdr_end;
			end_addr = &header->hdr_end;
			p = (UCHAR*) header + header->hdr_end;
		}
		else
		{
			logp = (log_info_page*) page;
			next_page = logp->log_next_page;
			free_space = dbb->dbb_page_size - logp->log_end;
			end_addr = &logp->log_end;
			p = (UCHAR*) logp + logp->log_end;
		}

		if (free_space > (2 + len))
		{
			//if (must_write)
				CCH_MARK_MUST_WRITE(tdbb, window);
			//else
			//	CCH_MARK(tdbb, window);

			fb_assert(type <= MAX_UCHAR);
			fb_assert(len <= MAX_UCHAR);
			*p++ = static_cast<UCHAR>(type);
			*p++ = static_cast<UCHAR>(len);

			if (len)
			{
				memcpy(p, entry, len);
				p += len;
			}

			*p = HDR_end;

			*end_addr = (USHORT) (p - (UCHAR*) page);
			return;
		}

		if (!next_page)
			break;

		// Follow chain of header pages

		if (page_num == HEADER_PAGE)
			*ppage = page = CCH_HANDOFF(tdbb, window, next_page, LCK_write, pag_header);
		else
			*ppage = page = CCH_HANDOFF(tdbb, window, next_page, LCK_write, pag_log);
	}

	WIN new_window(DB_PAGE_SPACE, -1);
	pag* new_page = (PAG) DPM_allocate(tdbb, &new_window);

	//if (must_write)
		CCH_MARK_MUST_WRITE(tdbb, &new_window);
	//else
	//	CCH_MARK(tdbb, &new_window);


	header_page* new_header = 0;
	log_info_page* new_logp = 0;
	SLONG next_page;
	USHORT* end_addr;
	UCHAR* p;
	if (page_num == HEADER_PAGE)
	{
		new_header = (header_page*) new_page;
		new_header->hdr_header.pag_type = pag_header;
		new_header->hdr_end = HDR_SIZE;
		new_header->hdr_page_size = dbb->dbb_page_size;
		new_header->hdr_data[0] = HDR_end;
		next_page = new_window.win_page.getPageNum();
		end_addr = &new_header->hdr_end;
		p = new_header->hdr_data;
	}
	else
	{
		new_logp = (log_info_page*) new_page;
		new_logp->log_header.pag_type = pag_log;
		new_logp->log_data[0] = LOG_end;
		new_logp->log_end = LIP_SIZE;
		next_page = new_window.win_page.getPageNum();
		end_addr = &new_logp->log_end;
		p = new_logp->log_data;
	}

	fb_assert(type <= MAX_UCHAR);
	fb_assert(len <= MAX_UCHAR);
	*p++ = static_cast<UCHAR>(type);
	*p++ = static_cast<UCHAR>(len);

	if (len)
	{
		memcpy(p, entry, len);
		p += len;
	}

	*p = HDR_end;
	*end_addr = (USHORT) (p - (UCHAR*) new_page);

	CCH_RELEASE(tdbb, &new_window);

	CCH_precedence(tdbb, window, next_page);

	CCH_MARK(tdbb, window);

	if (page_num == HEADER_PAGE)
		header->hdr_next_page = next_page;
	else
		logp->log_next_page = next_page;
}


static bool find_type(thread_db* tdbb,
					  SLONG page_num,
					  WIN* window,
					  PAG* ppage,
					  USHORT lock,
					  USHORT type,
					  UCHAR** entry_p,
					  const UCHAR** clump_end)
{
/***********************************************
 *
 *	f i n d _ t y p e
 *
 ***********************************************
 *
 * Functional description
 *	Find the requested type in a page.
 *	RETURNS
 *		pointer to type, pointer to end of page, header.
 *		true  - Found it
 *		false - Not present
 *
 **************************************/
	SET_TDBB(tdbb);

	while (true)
	{
		header_page* header = 0;
		log_info_page* logp = 0;
		UCHAR* p;
		SLONG next_page;
		if (page_num == HEADER_PAGE)
		{
			header = (header_page*) (*ppage);
			p = header->hdr_data;
			next_page = header->hdr_next_page;
		}
		else
		{
			logp = (log_info_page*) (*ppage);
			p = logp->log_data;
			next_page = logp->log_next_page;
		}

		UCHAR* q = 0;
		for (; (*p != HDR_end); p += 2 + p[1])
		{
			if (*p == type)
				q = p;
		}

		if (q)
		{
			*entry_p = q;
			*clump_end = p;
			return true;
		}

		// Follow chain of pages

		if (next_page)
		{
			if (page_num == HEADER_PAGE) {
				*ppage = CCH_HANDOFF(tdbb, window, next_page, lock, pag_header);
			}
			else {
				*ppage = CCH_HANDOFF(tdbb, window, next_page, lock, pag_log);
			}
		}
		else
			return false;
	}
}

PageSpace::~PageSpace()
{
	if (file)
	{
		PIO_close(file);

		while (file)
		{
			jrd_file* next = file->fil_next;
			delete file;
			file = next;
		}
	}
}

ULONG PageSpace::actAlloc(const USHORT pageSize)
{
/**************************************
 *
 * Functional description
 *  Compute actual number of physically allocated pages of database.
 *
 **************************************/

	// Traverse the linked list of files and add up the
	// number of pages in each file
	ULONG tot_pages = 0;
	for (const jrd_file* f = file; f != NULL; f = f->fil_next) {
		tot_pages += PIO_get_number_of_pages(f, pageSize);
	}

	return tot_pages;
}

ULONG PageSpace::actAlloc(const Database* dbb)
{
	PageSpace* pgSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	return pgSpace->actAlloc(dbb->dbb_page_size);
}

ULONG PageSpace::maxAlloc(const USHORT pageSize)
{
/**************************************
 *
 * Functional description
 *	Compute last physically allocated page of database.
 *
 **************************************/
	const jrd_file* f = file;
	while (f->fil_next) {
		f = f->fil_next;
	}

	const ULONG nPages = f->fil_min_page - f->fil_fudge + PIO_get_number_of_pages(f, pageSize);

	if (maxPageNumber < nPages)
		maxPageNumber = nPages;

	return nPages;
}

ULONG PageSpace::maxAlloc(const Database* dbb)
{
	PageSpace* pgSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	return pgSpace->maxAlloc(dbb->dbb_page_size);
}

bool PageSpace::extend(thread_db* tdbb, const ULONG pageNum)
{
/**************************************
 *
 * Functional description
 *	Extend database file(s) up to at least pageNum pages. Number of pages to
 *	extend can't be less than hardcoded value MIN_EXTEND_BYTES and more than
 *	configured value "DatabaseGrowthIncrement" (both values in bytes).
 *
 *	If "DatabaseGrowthIncrement" is less than MIN_EXTEND_BYTES then don't
 *	extend file(s)
 *
 **************************************/
	const int MAX_EXTEND_BYTES = Config::getDatabaseGrowthIncrement();

	if (pageNum < maxPageNumber || MAX_EXTEND_BYTES < MIN_EXTEND_BYTES)
		return true;

	Database* dbb = tdbb->getDatabase();

	if (pageNum >= maxAlloc(dbb->dbb_page_size))
	{
		const ULONG minExtendPages = MIN_EXTEND_BYTES / dbb->dbb_page_size;
		const ULONG maxExtendPages = MAX_EXTEND_BYTES / dbb->dbb_page_size;
		const ULONG reqPages = pageNum - maxPageNumber + 1;

		ULONG extPages;
		extPages = MIN(MAX(maxPageNumber / 16, minExtendPages), maxExtendPages);
		extPages = MAX(reqPages, extPages);

		while (true)
		{
			try
			{
				PIO_extend(dbb, file, extPages, dbb->dbb_page_size);
				break;
			}
			catch (Firebird::status_exception)
			{
				if (extPages > reqPages)
				{
					extPages = MAX(reqPages, extPages / 2);
					fb_utils::init_status(tdbb->tdbb_status_vector);
				}
				else
				{
					gds__log("Error extending file \"%s\" by %lu page(s).\nCurrently allocated %lu pages, requested page number %lu",
						file->fil_string, extPages, maxPageNumber, pageNum);
					return false;
				}
			}
		}

		maxPageNumber = 0;
	}
	return true;
}

PageSpace* PageManager::addPageSpace(const USHORT pageSpaceID)
{
	PageSpace* newPageSpace = findPageSpace(pageSpaceID);
	if (!newPageSpace)
	{
		newPageSpace = FB_NEW(pool) PageSpace(pageSpaceID);
		pageSpaces.add(newPageSpace);
	}

	return newPageSpace;
}

PageSpace* PageManager::findPageSpace(const USHORT pageSpace) const
{
	size_t pos;
	if (pageSpaces.find(pageSpace, pos)) {
		return pageSpaces[pos];
	}

	return 0;
}

void PageManager::delPageSpace(const USHORT pageSpace)
{
	size_t pos;
	if (pageSpaces.find(pageSpace, pos))
	{
		PageSpace* pageSpaceToDelete = pageSpaces[pos];
		pageSpaces.remove(pos);
		delete pageSpaceToDelete;
	}
}

void PageManager::closeAll()
{
	for (size_t i = 0; i < pageSpaces.getCount(); i++)
	{
		if (pageSpaces[i]->file) {
			PIO_close(pageSpaces[i]->file);
		}
	}
}

void PageManager::releaseLocks()
{
#ifdef WIN_NT
	for (size_t i = 0; i < pageSpaces.getCount(); i++)
	{
		if (pageSpaces[i]->file && pageSpaces[i]->file->fil_ext_lock)
		{
			delete pageSpaces[i]->file->fil_ext_lock;
			pageSpaces[i]->file->fil_ext_lock = NULL;
		}
	}
#endif
}

USHORT PageManager::getTempPageSpaceID(thread_db* tdbb)
{
	USHORT result;
#ifdef SUPERSERVER
	result = TEMP_PAGE_SPACE;
#else
	SET_TDBB(tdbb);
	Database* dbb = tdbb->getDatabase();
	Attachment* att = tdbb->getAttachment();
	if (!att->att_temp_pg_lock)
	{
		Lock* lock = FB_NEW_RPT(*att->att_pool, sizeof(SLONG)) Lock();
		lock->lck_type = LCK_page_space;
		lock->lck_owner_handle = LCK_get_owner_handle(tdbb, lock->lck_type);
		lock->lck_parent = dbb->dbb_lock;
		lock->lck_length = sizeof(SLONG);
		lock->lck_dbb = dbb;

		PAG_attachment_id(tdbb);

		while (true)
		{
			const double tmp = rand() * (MAX_USHORT - TEMP_PAGE_SPACE - 1.0) / (RAND_MAX + 1.0);
			lock->lck_key.lck_long = static_cast<SLONG>(tmp) + TEMP_PAGE_SPACE + 1;
			if (LCK_lock(tdbb, lock, LCK_write, LCK_NO_WAIT))
				break;
			fb_utils::init_status(tdbb->tdbb_status_vector);
		}

		att->att_temp_pg_lock = lock;
	}

	result = (USHORT) att->att_temp_pg_lock->lck_key.lck_long;
#endif

	if (!this->findPageSpace(result)) {
		PAG_attach_temp_pages(tdbb, result);
	}

	return result;
}

ULONG PAG_page_count(Database* database, PageCountCallback* cb)
{
/*********************************************
 *
 *	P A G _ p a g e _ c o u n t
 *
 *********************************************
 *
 * Functional description
 *	Count pages, used by database
 *
 *********************************************/
	fb_assert(cb);

	const bool isODS11_x =
		(database->dbb_ods_version == ODS_VERSION11 && database->dbb_minor_version >= 1);
	if (!isODS11_x) {
		return 0;
	}

	Firebird::Array<BYTE> temp;
	page_inv_page* pip = (Ods::page_inv_page*) // can't reinterpret_cast<> here
		FB_ALIGN((IPTR) temp.getBuffer(database->dbb_page_size + MIN_PAGE_SIZE), MIN_PAGE_SIZE);

	PageSpace* pageSpace = database->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	fb_assert(pageSpace);

	ULONG pageNo = pageSpace->ppFirst;
	const ULONG pagesPerPip = database->dbb_page_manager.pagesPerPIP;

	for (ULONG sequence = 0; true; pageNo = (pagesPerPip * ++sequence) - 1)
	{
		cb->newPage(pageNo, &pip->pip_header);
		fb_assert(pip->pip_header.pag_type == pag_pages);
		if (pip->pip_header.reserved == pagesPerPip)
		{
			// this is not last page, continue search
			continue;
		}

		return pip->pip_header.reserved + pageNo + (sequence ? 1 : -1);
	}

	// compiler warnings silencer
	return 0;
}
