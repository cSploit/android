/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		unix.cpp
 *	DESCRIPTION:	UNIX (BSD) specific physical IO
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
 * 2001.07.06 Sean Leyne - Code Cleanup, removed "#ifdef READONLY_DATABASE"
 *                         conditionals, as the engine now fully supports
 *                         readonly databases.
 *
 * 2002.10.27 Sean Leyne - Completed removal of "DELTA" port
 *
 */

#include "firebird.h"
#include "../jrd/common.h"
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#ifdef HAVE_PWD_H
#include <pwd.h>
#endif
#include <sys/file.h>
#include <sys/stat.h>
#include <string.h>
#include <errno.h>
#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_AIO_H
#include <aio.h>
#endif

#include "../jrd/jrd.h"
#include "../jrd/os/pio.h"
#include "../jrd/ods.h"
#include "../jrd/lck.h"
#include "../jrd/cch.h"
#include "../jrd/ibase.h"
#include "gen/iberror.h"
#include "../jrd/cch_proto.h"
#include "../jrd/err_proto.h"
#include "../jrd/gds_proto.h"
#include "../jrd/isc_proto.h"
#include "../jrd/isc_f_proto.h"
#include "../jrd/os/isc_i_proto.h"
#include "../jrd/lck_proto.h"
#include "../jrd/mov_proto.h"
#include "../jrd/ods_proto.h"
#include "../jrd/os/pio_proto.h"
#include "../common/classes/init.h"

using namespace Jrd;
using namespace Firebird;

// Some operating systems have problems with use of write/read with
// big (>2Gb) files. On the other hand, pwrite/pread works fine for them.
// Therefore:
#if defined SOLARIS
#define BROKEN_IO_64
#endif
// which will force use of pread/pwrite even for CS.

#define IO_RETRY	20

#ifdef O_SYNC
#define SYNC		O_SYNC
#endif

// Changed to not redfine SYNC if O_SYNC already exists
// they seem to be the same values anyway. MOD 13-07-2001
#if (!(defined SYNC) && (defined O_FSYNC))
#define SYNC		O_FSYNC
#endif

#ifdef O_DSYNC
#undef SYNC
#define SYNC		O_DSYNC
#endif

#ifndef SYNC
#define SYNC		0
#endif

#ifndef O_BINARY
#define O_BINARY	0
#endif

static const mode_t MASK = 0660;

#define FCNTL_BROKEN
// please undefine FCNTL_BROKEN for operating systems,
// that can successfully change BOTH O_DIRECT and O_SYNC using fcntl()

static jrd_file* seek_file(jrd_file*, BufferDesc*, FB_UINT64*, ISC_STATUS*);
static jrd_file* setup_file(Database*, const PathName&, int, bool);
static bool unix_error(const TEXT*, const jrd_file*, ISC_STATUS, ISC_STATUS* = NULL);
#if !(defined HAVE_PREAD && defined HAVE_PWRITE)
static SLONG pread(int, SCHAR*, SLONG, SLONG);
static SLONG pwrite(int, SCHAR*, SLONG, SLONG);
#endif
#ifdef SUPPORT_RAW_DEVICES
static bool raw_devices_validate_database (int, const PathName&);
static int  raw_devices_unlink_database (const PathName&);
#endif
static int	openFile(const char*, const bool, const bool, const bool);
static void	maybeCloseFile(int&);


int PIO_add_file(Database* dbb, jrd_file* main_file, const PathName& file_name, SLONG start)
{
/**************************************
 *
 *	P I O _ a d d _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Add a file to an existing database.  Return the sequence
 *	number of the new file.  If anything goes wrong, return a
 *	sequence of 0.
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/
	jrd_file* new_file = PIO_create(dbb, file_name, false, false, false);
	if (!new_file)
		return 0;

	new_file->fil_min_page = start;
	USHORT sequence = 1;

	jrd_file* file;
	for (file = main_file; file->fil_next; file = file->fil_next)
		++sequence;

	file->fil_max_page = start - 1;
	file->fil_next = new_file;

	return sequence;
}


void PIO_close(jrd_file* main_file)
{
/**************************************
 *
 *	P I O _ c l o s e
 *
 **************************************
 *
 * Functional description
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/

	for (jrd_file* file = main_file; file; file = file->fil_next)
	{
		if (file->fil_desc && file->fil_desc != -1)
		{
			close(file->fil_desc);
			file->fil_desc = -1;
		}
	}
}


jrd_file* PIO_create(Database* dbb, const PathName& file_name,
	const bool overwrite, const bool temporary, const bool /*share_delete*/)
{
/**************************************
 *
 *	P I O _ c r e a t e
 *
 **************************************
 *
 * Functional description
 *	Create a new database file.
 *	NOTE:  This routine does not lock any mutexes on
 *	its own behalf.  It is assumed that mutexes will
 *	have been locked before entry.
 *
 **************************************/
#ifdef SUPERSERVER_V2
	const int flag = SYNC | O_RDWR | O_CREAT | (overwrite ? O_TRUNC : O_EXCL) | O_BINARY;
#else
#ifdef SUPPORT_RAW_DEVICES
	const int flag = O_RDWR |
			(PIO_on_raw_device(file_name) ? 0 : O_CREAT) |
			(overwrite ? O_TRUNC : O_EXCL) |
			O_BINARY;
#else
	const int flag = O_RDWR | O_CREAT | (overwrite ? O_TRUNC : O_EXCL) | O_BINARY;
#endif
#endif

	const int desc = open(file_name.c_str(), flag, 0666);
	if (desc == -1)
	{
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("open O_CREAT") << Arg::Str(file_name) <<
                 Arg::Gds(isc_io_create_err) << Arg::Unix(errno));
	}

#ifdef HAVE_FCHMOD
	if (fchmod(desc, MASK) < 0)
#else
	if (chmod(file_name.c_str(), MASK) < 0)
#endif
	{
		int chmodError = errno;
		// ignore possible errors in these calls - even if they have failed
		// we cannot help much with former recovery
		close(desc);
		unlink(file_name.c_str());
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("chmod") << Arg::Str(file_name) <<
				 Arg::Gds(isc_io_create_err) << Arg::Unix(chmodError));
	}

	if (temporary
#ifdef SUPPORT_RAW_DEVICES
		&& (!PIO_on_raw_device(file_name))
#endif
				 )
	{
		int rc = unlink(file_name.c_str());
		// it's no use throwing an error if unlink() failed for temp file in release build
#ifdef DEV_BUILD
		if (rc < 0)
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("unlink") << Arg::Str(file_name) <<
					 Arg::Gds(isc_io_create_err) << Arg::Unix(errno));
		}
#endif
	}

	// posix_fadvise(desc, 0, 0, POSIX_FADV_RANDOM);

	// File open succeeded.  Now expand the file name.

	PathName expanded_name(file_name);
	ISC_expand_filename(expanded_name, false);

	return setup_file(dbb, expanded_name, desc, false);
}


bool PIO_expand(const TEXT* file_name, USHORT file_length, TEXT* expanded_name, size_t len_expanded)
{
/**************************************
 *
 *	P I O _ e x p a n d
 *
 **************************************
 *
 * Functional description
 *	Fully expand a file name.  If the file doesn't exist, do something
 *	intelligent.
 *
 **************************************/

	return ISC_expand_filename(file_name, file_length, expanded_name, len_expanded, false);
}


void PIO_extend(Database* dbb, jrd_file* /*main_file*/, const ULONG /*extPages*/, const USHORT /*pageSize*/)
{
/**************************************
 *
 *	P I O _ e x t e n d
 *
 **************************************
 *
 * Functional description
 *	Extend file by extPages pages of pageSize size.
 *
 **************************************/
	// not implemented
	return;
}


void PIO_flush(Database* dbb, jrd_file* main_file)
{
/**************************************
 *
 *	P I O _ f l u s h
 *
 **************************************
 *
 * Functional description
 *	Flush the operating system cache back to good, solid oxide.
 *
 **************************************/

	// Since all SUPERSERVER_V2 database and shadow I/O is synchronous, this is a no-op.
#ifndef SUPERSERVER_V2
	MutexLockGuard guard(main_file->fil_mutex);

	Database::Checkout dcoHolder(dbb);
	for (jrd_file* file = main_file; file; file = file->fil_next)
	{
		if (file->fil_desc != -1)
		{
			// This really should be an error
			fsync(file->fil_desc);
		}
	}
#endif
}


#ifdef SOLARIS
// minimize #ifdefs inside PIO_force_write()
#define O_DIRECT 0
#endif

void PIO_force_write(jrd_file* file, const bool forcedWrites, const bool notUseFSCache)
{
/**************************************
 *
 *	P I O _ f o r c e _ w r i t e	( G E N E R I C )
 *
 **************************************
 *
 * Functional description
 *	Set (or clear) force write, if possible, for the database.
 *
 **************************************/

	// Since all SUPERSERVER_V2 database and shadow I/O is synchronous, this is a no-op.

#ifndef SUPERSERVER_V2
	const bool oldForce = (file->fil_flags & FIL_force_write) != 0;
	const bool oldNotUseCache = (file->fil_flags & FIL_no_fs_cache) != 0;

	if (forcedWrites != oldForce || notUseFSCache != oldNotUseCache)
	{

		const int control = (forcedWrites ? SYNC : 0) | (notUseFSCache ? O_DIRECT : 0);

#ifndef FCNTL_BROKEN
		if (fcntl(file->fil_desc, F_SETFL, control) == -1)
		{
			unix_error("fcntl() SYNC/DIRECT", file, isc_io_access_err);
		}
#else //FCNTL_BROKEN
		maybeCloseFile(file->fil_desc);
		file->fil_desc = openFile(file->fil_string, forcedWrites,
								  notUseFSCache, file->fil_flags & FIL_readonly);
		if (file->fil_desc == -1)
		{
			unix_error("re open() for SYNC/DIRECT", file, isc_io_open_err);
		}
#endif //FCNTL_BROKEN

#ifdef SOLARIS
		if (notUseFSCache != oldNotUseCache &&
			directio(file->fil_desc, notUseFSCache ? DIRECTIO_ON : DIRECTIO_OFF) != 0)
		{
			unix_error("directio()", file, isc_io_access_err);
		}
#endif

		file->fil_flags &= ~(FIL_force_write | FIL_no_fs_cache);
		file->fil_flags |= (forcedWrites ? FIL_force_write : 0) |
						   (notUseFSCache ? FIL_no_fs_cache : 0);
	}
#endif
}


ULONG PIO_get_number_of_pages(const jrd_file* file, const USHORT pagesize)
{
/**************************************
 *
 *	P I O _ g e t _ n u m b e r _ o f _ p a g e s
 *
 **************************************
 *
 * Functional description
 *	Compute number of pages in file, based only on file size.
 *
 **************************************/

	if (file->fil_desc == -1)
	{
		unix_error("fstat", file, isc_io_access_err);
		return (0);
	}

	struct stat statistics;
	if (fstat(file->fil_desc, &statistics)) {
		unix_error("fstat", file, isc_io_access_err);
	}

	const FB_UINT64 length = statistics.st_size;

	return (length + pagesize - 1) / pagesize;
}


void PIO_get_unique_file_id(const Jrd::jrd_file* file, UCharBuffer& id)
{
/**************************************
 *
 *	P I O _ g e t _ u n i q u e _ f i l e _ i d
 *
 **************************************
 *
 * Functional description
 *	Return a binary string that uniquely identifies the file.
 *
 **************************************/
	struct stat statistics;
	if (fstat(file->fil_desc, &statistics) != 0) {
		unix_error("fstat", file, isc_io_access_err);
	}

	const size_t len1 = sizeof(statistics.st_dev);
	const size_t len2 = sizeof(statistics.st_ino);

	UCHAR* p = id.getBuffer(len1 + len2);

	memcpy(p, &statistics.st_dev, len1);
	p += len1;
	memcpy(p, &statistics.st_ino, len2);
}


void PIO_header(Database* dbb, SCHAR* address, int length)
{
/**************************************
 *
 *	P I O _ h e a d e r
 *
 **************************************
 *
 * Functional description
 *	Read the page header.  This assumes that the file has not been
 *	repositioned since the file was originally mapped.
 *
 **************************************/
	int i;
	FB_UINT64 bytes;

	PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
	jrd_file* file = pageSpace->file;

	if (file->fil_desc == -1)
		unix_error("PIO_header", file, isc_io_read_err);

	for (i = 0; i < IO_RETRY; i++)
	{
#ifdef ISC_DATABASE_ENCRYPTION
		if (dbb->dbb_encrypt_key)
		{
			SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

			if ((bytes = pread(file->fil_desc, spare_buffer, length, 0)) == (FB_UINT64) -1)
			{
				if (SYSCALL_INTERRUPTED(errno))
					continue;
				unix_error("read", file, isc_io_read_err);
			}

			(*dbb->dbb_decrypt) (dbb->dbb_encrypt_key->str_data, spare_buffer, length, address);
		}
		else
#endif // ISC_DATABASE_ENCRYPTION
		if ((bytes = pread(file->fil_desc, address, length, 0)) == (FB_UINT64) -1)
		{
			if (SYSCALL_INTERRUPTED(errno))
				continue;
			unix_error("read", file, isc_io_read_err);
		}
		else
			break;
	}

	if (i == IO_RETRY)
	{
		if (bytes == 0)
		{
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_header: an empty page read!\n");
			fflush(stderr);
#endif
		}
		else
		{
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_header: retry count exceeded\n");
			fflush(stderr);
#endif
			unix_error("read_retry", file, isc_io_read_err);
		}
	}
}

// we need a class here only to return memory on shutdown and avoid
// false memory leak reports
static Firebird::InitInstance<ZeroBuffer> zeros;


USHORT PIO_init_data(Database* dbb, jrd_file* main_file, ISC_STATUS* status_vector,
					 ULONG startPage, USHORT initPages)
{
/**************************************
 *
 *	P I O _ i n i t _ d a t a
 *
 **************************************
 *
 * Functional description
 *	Initialize tail of file with zeros
 *
 **************************************/
	const char* const zero_buff = zeros().getBuffer();
	const size_t zero_buff_size = zeros().getSize();

	// Fake buffer, used in seek_file. Page space ID have no matter there
	// as we already know file to work with
	BufferDesc bdb;
	bdb.bdb_dbb = dbb;
	bdb.bdb_page = PageNumber(0, startPage);

	FB_UINT64 offset;

	Database::Checkout dcoHolder(dbb);

	jrd_file* file = seek_file(main_file, &bdb, &offset, status_vector);

	if (!file)
		return 0;

	if (file->fil_min_page + 8 > startPage)
		return 0;

	USHORT leftPages = initPages;
	const ULONG initBy = MIN(file->fil_max_page - startPage, leftPages);
	if (initBy < leftPages)
		leftPages = initBy;

	for (ULONG i = startPage; i < startPage + initBy; )
	{
		bdb.bdb_page = PageNumber(0, i);
		USHORT write_pages = zero_buff_size / dbb->dbb_page_size;
		if (write_pages > leftPages)
			write_pages = leftPages;

		SLONG to_write = write_pages * dbb->dbb_page_size;
		SLONG written;

		for (int r = 0; r < IO_RETRY; r++)
		{
			if (!(file = seek_file(file, &bdb, &offset, status_vector)))
				return false;
			if ((written = pwrite(file->fil_desc, zero_buff, to_write, LSEEK_OFFSET_CAST offset)) == to_write)
				break;
			if (written == (SLONG) -1 && !SYSCALL_INTERRUPTED(errno))
				return unix_error("write", file, isc_io_write_err, status_vector);
		}


		leftPages -= write_pages;
		i += write_pages;
	}

	return (initPages - leftPages);
}


jrd_file* PIO_open(Database* dbb,
				   const PathName& string,
				   const PathName& file_name,
				   const bool /*share_delete*/)
{
/**************************************
 *
 *	P I O _ o p e n
 *
 **************************************
 *
 * Functional description
 *	Open a database file.
 *
 **************************************/
	bool readOnly = false;
	const TEXT* const ptr = (string.hasData() ? string : file_name).c_str();
	int desc = openFile(ptr, false, false, false);

	if (desc == -1)
	{
		// Try opening the database file in ReadOnly mode. The database file could
		// be on a RO medium (CD-ROM etc.). If this fileopen fails, return error.

		desc = openFile(ptr, false, false, true);
		if (desc == -1)
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("open") << Arg::Str(file_name) <<
					 Arg::Gds(isc_io_open_err) << Arg::Unix(errno));
		}

		// If this is the primary file, set Database flag to indicate that it is
		// being opened ReadOnly. This flag will be used later to compare with
		// the Header Page flag setting to make sure that the database is set ReadOnly.

		PageSpace* pageSpace = dbb->dbb_page_manager.findPageSpace(DB_PAGE_SPACE);
		if (!pageSpace->file)
			dbb->dbb_flags |= DBB_being_opened_read_only;
		readOnly = true;
	}

	// posix_fadvise(desc, 0, 0, POSIX_FADV_RANDOM);

#ifdef SUPPORT_RAW_DEVICES
	// At this point the file has successfully been opened in either RW or RO
	// mode. Check if it is a special file (i.e. raw block device) and if a
	// valid database is on it. If not, return an error.

	if (PIO_on_raw_device(file_name) && !raw_devices_validate_database(desc, file_name))
	{
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("open") << Arg::Str(file_name) <<
				 Arg::Gds(isc_io_open_err) << Arg::Unix(ENOENT));
	}
#endif // SUPPORT_RAW_DEVICES

	return setup_file(dbb, string, desc, readOnly);
}


bool PIO_read(jrd_file* file, BufferDesc* bdb, Ods::pag* page, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	P I O _ r e a d
 *
 **************************************
 *
 * Functional description
 *	Read a data page.  Oh wow.
 *
 **************************************/
	int i;
	FB_UINT64 bytes, offset;

	if (file->fil_desc == -1) {
		return unix_error("read", file, isc_io_read_err, status_vector);
	}

	Database* dbb = bdb->bdb_dbb;
	Database::Checkout dcoHolder(dbb);

	const FB_UINT64 size = dbb->dbb_page_size;

#ifdef ISC_DATABASE_ENCRYPTION
	if (dbb->dbb_encrypt_key)
	{
		SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

		for (i = 0; i < IO_RETRY; i++)
		{
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
            if ((bytes = pread (file->fil_desc, spare_buffer, size, LSEEK_OFFSET_CAST offset)) == size)
			{
				(*dbb->dbb_decrypt) (dbb->dbb_encrypt_key->str_data, spare_buffer, size, page);
				break;
			}
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("read", file, isc_io_read_err, status_vector);
		}
	}
	else
#endif // ISC_DATABASE_ENCRYPTION
	{
		for (i = 0; i < IO_RETRY; i++)
		{
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
			if ((bytes = pread(file->fil_desc, page, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("read", file, isc_io_read_err, status_vector);
		}
	}


	if (i == IO_RETRY)
	{
		if (bytes == 0)
		{
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_read: an empty page read!\n");
			fflush(stderr);
#endif
		}
		else
		{
#ifdef DEV_BUILD
			fprintf(stderr, "PIO_read: retry count exceeded\n");
			fflush(stderr);
#endif
			unix_error("read_retry", file, isc_io_read_err);
		}
	}

	// posix_fadvise(file->desc, offset, size, POSIX_FADV_NOREUSE);
	return true;
}


bool PIO_write(jrd_file* file, BufferDesc* bdb, Ods::pag* page, ISC_STATUS* status_vector)
{
/**************************************
 *
 *	P I O _ w r i t e
 *
 **************************************
 *
 * Functional description
 *	Write a data page.  Oh wow.
 *
 **************************************/
	int i;
	SLONG bytes;
    FB_UINT64 offset;

	if (file->fil_desc == -1)
		return unix_error("write", file, isc_io_write_err, status_vector);

	Database* dbb = bdb->bdb_dbb;
	Database::Checkout dcoHolder(dbb);

	const SLONG size = dbb->dbb_page_size;

#ifdef ISC_DATABASE_ENCRYPTION
	if (dbb->dbb_encrypt_key)
	{
		SLONG spare_buffer[MAX_PAGE_SIZE / sizeof(SLONG)];

		(*dbb->dbb_encrypt) (dbb->dbb_encrypt_key->str_data, page, size, spare_buffer);

		for (i = 0; i < IO_RETRY; i++)
		{
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
			if ((bytes = pwrite(file->fil_desc, spare_buffer, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
			if (bytes == -1U && !SYSCALL_INTERRUPTED(errno))
				return unix_error("write", file, isc_io_write_err, status_vector);
		}
	}
	else
#endif // ISC_DATABASE_ENCRYPTION
	{
		for (i = 0; i < IO_RETRY; i++)
		{
			if (!(file = seek_file(file, bdb, &offset, status_vector)))
				return false;
			if ((bytes = pwrite(file->fil_desc, page, size, LSEEK_OFFSET_CAST offset)) == size)
				break;
			if (bytes == (SLONG) -1 && !SYSCALL_INTERRUPTED(errno))
				return unix_error("write", file, isc_io_write_err, status_vector);
		}
	}


	// posix_fadvise(file->desc, offset, size, POSIX_FADV_DONTNEED);
	return true;
}


static jrd_file* seek_file(jrd_file* file, BufferDesc* bdb, FB_UINT64* offset,
	ISC_STATUS* status_vector)
{
/**************************************
 *
 *	s e e k _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Given a buffer descriptor block, find the appropriate
 *	file block and seek to the proper page in that file.
 *
 **************************************/
	Database* const dbb = bdb->bdb_dbb;
	ULONG page = bdb->bdb_page.getPageNum();

	for (;; file = file->fil_next)
	{
		if (!file) {
			CORRUPT(158);		// msg 158 database file not available
		}
		else if (page >= file->fil_min_page && page <= file->fil_max_page)
			break;
	}

	if (file->fil_desc == -1)
	{
		unix_error("lseek", file, isc_io_access_err, status_vector);
		return 0;
	}

	page -= file->fil_min_page - file->fil_fudge;

    FB_UINT64 lseek_offset = page;
    lseek_offset *= dbb->dbb_page_size;

    if (lseek_offset != (FB_UINT64) LSEEK_OFFSET_CAST lseek_offset)
	{
		unix_error("lseek", file, isc_io_32bit_exceeded_err, status_vector);
		return 0;
    }

	*offset = lseek_offset;

	return file;
}


static int openFile(const char* name, const bool forcedWrites,
	const bool notUseFSCache, const bool readOnly)
{
/**************************************
 *
 *	o p e n F i l e
 *
 **************************************
 *
 * Functional description
 *	Open a file with appropriate flags.
 *
 **************************************/

	int flag = O_BINARY | (readOnly ? O_RDONLY : O_RDWR);
#ifdef SUPERSERVER_V2
	flag |= SYNC;
	// what to do with O_DIRECT here ?
#else
	if (forcedWrites)
		flag |= SYNC;
	if (notUseFSCache)
		flag |= O_DIRECT;
#endif

	for (int i = 0; i < IO_RETRY; i++)
	{
		int desc = open(name, flag);
		if (desc != -1)
			return desc;
		if (!SYSCALL_INTERRUPTED(errno))
			break;
	}

	return -1;
}


static void maybeCloseFile(int& desc)
{
/**************************************
 *
 *	m a y b e C l o s e F i l e
 *
 **************************************
 *
 * Functional description
 *	If the file is open, close it.
 *
 **************************************/

	if (desc >= 0)
	{
		close(desc);
		desc = -1;
	}
}


static jrd_file* setup_file(Database* dbb,
							const PathName& file_name,
							int desc,
							bool read_only)
{
/**************************************
 *
 *	s e t u p _ f i l e
 *
 **************************************
 *
 * Functional description
 *	Set up file and lock blocks for a file.
 *
 **************************************/
	jrd_file* file = NULL;

	try
	{
		file = FB_NEW_RPT(*dbb->dbb_permanent, file_name.length() + 1) jrd_file();
		file->fil_desc = desc;
		file->fil_max_page = MAX_ULONG;
		strcpy(file->fil_string, file_name.c_str());

		if (read_only)
			file->fil_flags |= FIL_readonly;
	}
	catch (const Exception&)
	{
		close(desc);
		delete file;
		throw;
	}

	fb_assert(file);
	return file;
}


static bool unix_error(const TEXT* string,
					   const jrd_file* file, ISC_STATUS operation,
					   ISC_STATUS* status_vector)
{
/**************************************
 *
 *	u n i x _ e r r o r
 *
 **************************************
 *
 * Functional description
 *	Somebody has noticed a file system error and expects error
 *	to do something about it.  Harumph!
 *
 **************************************/
	if (!status_vector)
	{
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str(string) << Arg::Str(file->fil_string) <<
				 Arg::Gds(operation) << Arg::Unix(errno));
	}

	ERR_build_status(status_vector,
					 Arg::Gds(isc_io_error) << Arg::Str(string) << Arg::Str(file->fil_string) <<
					 Arg::Gds(operation) << Arg::Unix(errno));

	gds__log_status(0, status_vector);

	return false;
}

#if !(defined HAVE_PREAD && defined HAVE_PWRITE)

/* pread() and pwrite() behave like read() and write() except that they
   take an additional 'offset' argument. The I/O takes place at the specified
   'offset' from the beginning of the file and does not affect the offset
   associated with the file descriptor.
   This is done in order to allow more than one thread to operate on
   individual records within the same file simultaneously. This is
   called Positioned I/O. Since positioned I/O is not currently directly
   available through the POSIX interfaces, it has been implemented
   using the POSIX asynchronous I/O calls.

   NOTE: pread() and pwrite() are defined in UNIX International system
         interface and are a part of POSIX systems.
*/

static SLONG pread(int fd, SCHAR* buf, SLONG nbytes, SLONG offset)
/**************************************
 *
 *	p r e a d
 *
 **************************************
 *
 * Functional description
 *
 *   This function uses Asynchronous I/O calls to implement
 *   positioned read from a given offset
 **************************************/
{
	struct aiocb io;
	io.aio_fildes = fd;
	io.aio_offset = offset;
	io.aio_buf = buf;
	io.aio_nbytes = nbytes;
	io.aio_reqprio = 0;
	io.aio_sigevent.sigev_notify = SIGEV_NONE;
	int err = aio_read(&io);	// atomically reads at offset
	if (err != 0)
		return (err);			// errno is set

	struct aiocb *list[1];
	list[0] = &io;
	err = aio_suspend(list, 1, NULL);	// wait for I/O to complete
	if (err != 0)
		return (err);			// errno is set
	return (aio_return(&io));	// return I/O status
}

static SLONG pwrite(int fd, SCHAR* buf, SLONG nbytes, SLONG offset)
/**************************************
 *
 *	p w r i t e
 *
 **************************************
 *
 * Functional description
 *
 *   This function uses Asynchronous I/O calls to implement
 *   positioned write from a given offset
 **************************************/
{
	struct aiocb io;
	io.aio_fildes = fd;
	io.aio_offset = offset;
	io.aio_buf = buf;
	io.aio_nbytes = nbytes;
	io.aio_reqprio = 0;
	io.aio_sigevent.sigev_notify = SIGEV_NONE;
	int err = aio_write(&io);	// atomically reads at offset
	if (err != 0)
		return (err);			// errno is set

	struct aiocb *list[1];
	list[0] = &io;
	err = aio_suspend(list, 1, NULL);	// wait for I/O to complete
	if (err != 0)
		return (err);			// errno is set
	return (aio_return(&io));	// return I/O status
}

#endif // !(HAVE_PREAD && HAVE_PWRITE)


#ifdef SUPPORT_RAW_DEVICES
int PIO_unlink(const PathName& file_name)
{
/**************************************
 *
 *	P I O _ u n l i n k
 *
 **************************************
 *
 * Functional description
 *	Delete a database file.
 *
 **************************************/

	if (PIO_on_raw_device(file_name))
		return raw_devices_unlink_database(file_name);

	return unlink(file_name.c_str());
}


bool PIO_on_raw_device(const PathName& file_name)
{
/**************************************
 *
 *	P I O _ o n _ r a w _ d e v i c e
 *
 **************************************
 *
 * Functional description
 *	Checks if the supplied file name is a special file
 *
 **************************************/
	struct stat s;

	return (stat(file_name.c_str(), &s) == 0 && (S_ISCHR(s.st_mode) || S_ISBLK(s.st_mode)));
}


static bool raw_devices_validate_database(int desc, const PathName& file_name)
{
/**************************************
 *
 *	raw_devices_validate_database
 *
 **************************************
 *
 * Functional description
 *	Checks if the special file contains a valid database
 *
 **************************************/
	char header[MIN_PAGE_SIZE];
	const Ods::header_page* hp = (Ods::header_page*)header;
	bool retval = false;

	// Read in database header. Code lifted from PIO_header.
	if (desc == -1)
	{
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("raw_devices_validate_database") <<
										   Arg::Str(file_name) <<
				 Arg::Gds(isc_io_read_err) << Arg::Unix(errno));
	}

	for (int i = 0; i < IO_RETRY; i++)
	{
		if (lseek (desc, LSEEK_OFFSET_CAST 0, 0) == (off_t) -1)
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("lseek") << Arg::Str(file_name) <<
					 Arg::Gds(isc_io_read_err) << Arg::Unix(errno));
		}

		const ssize_t bytes = read (desc, header, sizeof(header));
		if (bytes == sizeof(header))
			goto read_finished;

		if (bytes == -1 && !SYSCALL_INTERRUPTED(errno))
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("read") << Arg::Str(file_name) <<
					 Arg::Gds(isc_io_read_err) << Arg::Unix(errno));
		}
	}

	ERR_post(Arg::Gds(isc_io_error) << Arg::Str("read_retry") << Arg::Str(file_name) <<
			 Arg::Gds(isc_io_read_err) << Arg::Unix(errno));

  read_finished:
	// Rewind file pointer
	if (lseek (desc, LSEEK_OFFSET_CAST 0, 0) == (off_t) -1)
	{
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("lseek") << Arg::Str(file_name) <<
				 Arg::Gds(isc_io_read_err) << Arg::Unix(errno));
	}

	// Validate database header. Code lifted from PAG_header.
	if (hp->hdr_header.pag_type != pag_header /*|| hp->hdr_sequence*/)
		goto quit;

	if (!Ods::isSupported(hp->hdr_ods_version, hp->hdr_ods_minor))
		goto quit;

	if (hp->hdr_page_size < MIN_PAGE_SIZE || hp->hdr_page_size > MAX_PAGE_SIZE)
		goto quit;

	// At this point we think we have identified a database on the device.
 	// PAG_header will validate the entire structure later.
	retval = true;

  quit:
#ifdef DEV_BUILD
	gds__log ("raw_devices_validate_database: %s -> %s%s\n",
		 file_name.c_str(),
		 retval ? "true" : "false",
		 retval && hp->hdr_sequence != 0 ? " (continuation file)" : "");
#endif
	return retval;
}


static int raw_devices_unlink_database(const PathName& file_name)
{
	char header[MIN_PAGE_SIZE];
	int desc = -1;

	for (int i = 0; i < IO_RETRY; i++)
	{
		if ((desc = open (file_name.c_str(), O_RDWR | O_BINARY)) != -1)
			break;

		if (!SYSCALL_INTERRUPTED(errno))
		{
			ERR_post(Arg::Gds(isc_io_error) << Arg::Str("open") << Arg::Str(file_name) <<
					 Arg::Gds(isc_io_open_err) << Arg::Unix(errno));
		}
	}

	memset(header, 0xa5, sizeof(header));

	int i;

	for (i = 0; i < IO_RETRY; i++)
	{
		const ssize_t bytes = write (desc, header, sizeof(header));
		if (bytes == sizeof(header))
			break;
		if (bytes == -1 && SYSCALL_INTERRUPTED(errno))
			continue;
		ERR_post(Arg::Gds(isc_io_error) << Arg::Str("write") << Arg::Str(file_name) <<
				 Arg::Gds(isc_io_write_err) << Arg::Unix(errno));
	}

	//if (desc != -1) perhaps it's better to check this???
		close(desc);

#ifdef DEV_BUILD
	gds__log ("raw_devices_unlink_database: %s -> %s\n",
				file_name.c_str(), i < IO_RETRY ? "true" : "false");
#endif

	return 0;
}
#endif // SUPPORT_RAW_DEVICES
