/*
 *	PROGRAM:	JRD Access Method
 *	MODULE:		nbackup.cpp
 *	DESCRIPTION:	Command line utility for physical backup/restore
 *
 *  The contents of this file are subject to the Initial
 *  Developer's Public License Version 1.0 (the "License");
 *  you may not use this file except in compliance with the
 *  License. You may obtain a copy of the License at
 *  http://www.ibphoenix.com/main.nfs?a=ibphoenix&page=ibp_idpl.
 *
 *  Software distributed under the License is distributed AS IS,
 *  WITHOUT WARRANTY OF ANY KIND, either express or implied.
 *  See the License for the specific language governing rights
 *  and limitations under the License.
 *
 *  The Original Code was created by Nickolay Samofatov
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2004 Nickolay Samofatov <nickolay@broadviewsoftware.com>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 *  Adriano dos Santos Fernandes
 *
 */


#include "firebird.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "../jrd/common.h"
#include "../jrd/db_alias.h"
#include "../jrd/ods.h"
#include "../jrd/nbak.h"
#include "../jrd/gds_proto.h"
#include "../jrd/os/path_utils.h"
#include "../jrd/os/guid.h"
#include "../jrd/ibase.h"
#include "../common/utils_proto.h"
#include "../common/classes/array.h"
#include "../common/classes/ClumpletWriter.h"
#include "../utilities/nbackup/nbk_proto.h"
#include "../jrd/license.h"
#include "../common/classes/MsgPrint.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#endif

#ifndef O_LARGEFILE
#define O_LARGEFILE 0
#endif

// How much we align memory when reading database header.
// Sector alignment of memory is necessary to use unbuffered IO on Windows.
// Actually, sectors may be bigger than 1K, but let's be consistent with
// JRD regarding the matter for the moment.
const size_t SECTOR_ALIGNMENT = MIN_PAGE_SIZE;

using namespace Firebird;

namespace
{
	using MsgFormat::SafeArg;
	const USHORT nbackup_msg_fac = 24;

	void printMsg(USHORT number, const SafeArg& arg)
	{
		char buffer[256];
		fb_msg_format(NULL, nbackup_msg_fac, number, sizeof(buffer), buffer, arg);
		printf("%s\n", buffer);
	}

	void printMsg(USHORT number)
	{
		static const SafeArg dummy;
		printMsg(number, dummy);
	}

	bool getMsg(USHORT number, char* buffer, size_t bufsize, const SafeArg& arg)
	{
		if (!number || !buffer || bufsize < 10)
			return false;
		return fb_msg_format(NULL, nbackup_msg_fac, number, bufsize, buffer, arg) > 0;
	}

	void usage(UtilSvc* uSvc, const char* message, ...)
	{
		string msg;
		va_list params;
		if (message)
		{
			va_start(params, message);
			msg.vprintf(message, params);
			va_end(params);
		}

		if (uSvc->isService())
		{
			fb_assert(message != NULL);
			(Arg::Gds(isc_random) << msg).raise();
		}

		if (message)
			printMsg(1, SafeArg() << msg.c_str()); // ERROR: @1.\n

		printMsg(2);
		printMsg(3);
		printMsg(4);
		printMsg(5);
		printMsg(6);
		printMsg(7);
		printMsg(8);
		printMsg(9);
		printMsg(10);
		printMsg(11);
		printMsg(12);
		printMsg(13);
		printMsg(14);
		printMsg(15);
		printMsg(16);
		printMsg(17);
		printMsg(70);
		printMsg(18);
		printMsg(19);
		printMsg(20);
		printMsg(21);
		printMsg(22);

		exit(FINI_ERROR);
	}

	void missing_parameter_for_switch(UtilSvc* uSvc, const char* sw)
	{
		usage(uSvc, "Missing parameter for switch %s", sw);
	}

	void singleAction(UtilSvc* uSvc)
	{
		usage(uSvc, "Only one of -L, -N, -F, -B or -R should be specified");
	}

	const int MSG_LEN = 1024;
	const size_t NBACKUP_FAILURE_SPACE = MSG_LEN * 4;

	// HPUX has non-posix-conformant method to return error codes from posix_fadvise().
	// Instead of error code, directly returned by function (like specified by posix),
	// -1 is returned in case of error and errno is set. Luckily, we can easily detect it runtime.
	// May be sometimes this function should be moved to os_util namespace.

#ifdef HAVE_POSIX_FADVISE
	int fb_fadvise(int fd, off_t offset, size_t len, int advice)
	{
		int rc = posix_fadvise(fd, offset, len, advice);

		if (rc < 0)
		{
			rc = errno;
		}
		if (rc == ENOTTY ||		// posix_fadvise is not supported by underlying file system
			rc == ENOSYS)		// hint is not supported by the underlying file object
		{
			rc = 0;				// ignore such errors
		}

		return rc;
	}
#else // HAVE_POSIX_FADVISE
	int fb_fadvise(int, off_t, size_t, int)
	{
		return 0;
	}
#endif // HAVE_POSIX_FADVISE

	bool flShutdown = false;

	int nbackupShutdown(const int reason, const int, void*)
	{
		if (reason == fb_shutrsn_signal)
		{
			flShutdown = true;
			return FB_FAILURE;
		}
		return FB_SUCCESS;
	}

} // namespace


class b_error : public LongJump
{
public:
	explicit b_error(const char* message)
	{
		size_t len = sizeof(txt) - 1;
		strncpy(txt, message, len);
		txt[len] = 0;
	}
	virtual ~b_error() throw() {}
	virtual const char* what() const throw() { return txt; }
	static void raise(UtilSvc* uSvc, const char* message, ...)
	{
		char temp[MSG_LEN];
		va_list params;
		va_start(params, message);
		VSNPRINTF(temp, sizeof(temp), message, params);
		temp[sizeof(temp) - 1] = 0;
		va_end(params);

		if (!uSvc->isService())
			fprintf(stderr, "Failure: %s\n", temp);

		throw b_error(temp);
	}
	virtual ISC_STATUS stuff_exception(ISC_STATUS* const status_vector) const throw()
	{
		(Arg::Gds(isc_random) << txt).copyTo(status_vector);
		makePermanentVector(status_vector);

		return status_vector[1];
	}
private:
	char txt[MSG_LEN];
};

static void checkCtrlC(UtilSvc* uSvc)
{
	if (flShutdown)
	{
		b_error::raise(uSvc, "\nClosing due to user request");
	}
}


#ifdef WIN_NT
#define FILE_HANDLE HANDLE
#else
#define FILE_HANDLE int
#endif


const char local_prefix[] = "localhost:";

const char backup_signature[4] = {'N','B','A','K'};

struct inc_header
{
	char signature[4];		// 'NBAK'
	SSHORT version;			// Incremental backup format version.
	SSHORT level;			// Backup level.
	// \\\\\ ---- this is 8 bytes. should not cause alignment problems
	FB_GUID backup_guid;	// GUID of this backup
	FB_GUID prev_guid;		// GUID of previous level backup
	ULONG page_size;		// Size of pages in the database and backup file
	// These fields are currently filled, but not used. May be used in future versions
	ULONG backup_scn;		// SCN of this backup
	ULONG prev_scn;			// SCN of previous level backup
};

class NBackup
{
public:
	NBackup(UtilSvc* _uSvc, const PathName& _database, const string& _username,
			const string& _password, bool _run_db_triggers, const string& _trustedUser,
			bool _trustedRole, bool _direct_io)
	  : uSvc(_uSvc), newdb(0), trans(0), database(_database),
		username(_username), password(_password), trustedUser(_trustedUser),
		run_db_triggers(_run_db_triggers), trustedRole(_trustedRole), direct_io(_direct_io),
		dbase(0), backup(0), db_size_pages(0)
	{
		// Recognition of local prefix allows to work with
		// database using TCP/IP loopback while reading file locally.
		// This makes NBACKUP compatible with Windows CS with XNET disabled
		PathName db(_database);
		if (strncmp(db.c_str(), local_prefix, sizeof(local_prefix) - 1) == 0)
			db = db.substr(sizeof(local_prefix) - 1);

		if (!ResolveDatabaseAlias(db, dbname))
			dbname = db;

		if (!uSvc->isService())
		{
			// It's time to take care about shutdown handling
			if (fb_shutdown_callback(status, nbackupShutdown, fb_shut_confirmation, NULL))
			{
				pr_error(status, "setting shutdown callback");
			}
		}
	}

	typedef ObjectsArray<PathName> BackupFiles;

	// External calls must clean up resources after themselves
	void fixup_database();
	void lock_database(bool get_size);
	void unlock_database();
	void backup_database(int level, const PathName& fname);
	void restore_database(const BackupFiles& files);

private:
	UtilSvc* uSvc;

    ISC_STATUS_ARRAY status; // status vector
	isc_db_handle newdb; // database handle
    isc_tr_handle trans; // transaction handle

	PathName database;
	string username, password, trustedUser;
	bool run_db_triggers, trustedRole, direct_io;

	PathName dbname; // Database file name
	PathName bakname;
	FILE_HANDLE dbase;
	FILE_HANDLE backup;
	ULONG  db_size_pages;	// In pages

	// IO functions
	size_t read_file(FILE_HANDLE &file, void *buffer, size_t bufsize);
	void write_file(FILE_HANDLE &file, void *buffer, size_t bufsize);
	void seek_file(FILE_HANDLE &file, SINT64 pos);

	void pr_error(const ISC_STATUS* status, const char* operation) const;

	void internal_lock_database();
	void get_database_size();
	void internal_unlock_database();
	void attach_database();
	void detach_database();

	// Create/open database and backup
	void open_database_write();
	void open_database_scan();
	void create_database();
	void close_database();

	void open_backup_scan();
	void create_backup();
	void close_backup();
};

size_t NBackup::read_file(FILE_HANDLE &file, void *buffer, size_t bufsize)
{
#ifdef WIN_NT
	DWORD bytesDone;
	if (!ReadFile(file, buffer, bufsize, &bytesDone, NULL))
	{
		b_error::raise(uSvc, "IO error (%d) reading file: %s",
			GetLastError(),
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
	return bytesDone;
#else
	const ssize_t res = read(file, buffer, bufsize);
	if (res < 0)
	{
		b_error::raise(uSvc, "IO error (%d) reading file: %s",
			errno,
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
	return res;
#endif
}

void NBackup::write_file(FILE_HANDLE &file, void *buffer, size_t bufsize)
{
#ifdef WIN_NT
	DWORD bytesDone;
	if (!WriteFile(file, buffer, bufsize, &bytesDone, NULL) || bytesDone != bufsize)
	{
		b_error::raise(uSvc, "IO error (%d) writing file: %s",
			GetLastError(),
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
#else
	if (write(file, buffer, bufsize) != (ssize_t) bufsize)
	{
		b_error::raise(uSvc, "IO error (%d) writing file: %s",
			errno,
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
#endif
}

void NBackup::seek_file(FILE_HANDLE &file, SINT64 pos)
{
#ifdef WIN_NT
	LARGE_INTEGER offset;
	offset.QuadPart = pos;
	DWORD error;
	if (SetFilePointer(dbase, offset.LowPart, &offset.HighPart, FILE_BEGIN) ==
			INVALID_SET_FILE_POINTER &&
		(error = GetLastError()) != NO_ERROR)
	{
		b_error::raise(uSvc, "IO error (%d) seeking file: %s",
			error,
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
#else
	if (lseek(file, pos, SEEK_SET) == (off_t) - 1)
	{
		b_error::raise(uSvc, "IO error (%d) seeking file: %s",
			errno,
			&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown");
	}
#endif
}

void NBackup::open_database_write()
{
#ifdef WIN_NT
	dbase = CreateFile(dbname.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dbase == INVALID_HANDLE_VALUE)
		b_error::raise(uSvc, "Error (%d) opening database file: %s", GetLastError(), dbname.c_str());
#else
	dbase = open(dbname.c_str(), O_RDWR | O_LARGEFILE);
	if (dbase < 0)
		b_error::raise(uSvc, "Error (%d) opening database file: %s", errno, dbname.c_str());
#endif
}

void NBackup::open_database_scan()
{
#ifdef WIN_NT

	// On Windows we use unbuffered IO to work around bug in Windows Server 2003
	// which has little problems with managing size of disk cache. If you read
	// very large file (5 GB or more) on this platform filesystem page cache
	// consumes all RAM of machine and causes excessive paging of user programs
	// and OS itself. Basically, reading any large file brings the whole system
	// down for extended period of time. Documented workaround is to avoid using
	// system cache when reading large files.
	dbase = CreateFile(dbname.c_str(),
		GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
		FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN | (direct_io ? FILE_FLAG_NO_BUFFERING : 0),
		NULL);
	if (dbase == INVALID_HANDLE_VALUE)
		b_error::raise(uSvc, "Error (%d) opening database file: %s", GetLastError(), dbname.c_str());

#else // WIN_NT

#ifndef O_NOATIME
#define O_NOATIME 0
#endif // O_NOATIME

//
// Solaris does not have O_DIRECT!!!
// TODO: Implement using Solaris directio or suffer performance problems. :-(
//
#ifndef O_DIRECT
#define O_DIRECT 0
#endif // O_DIRECT

	dbase = open(dbname.c_str(), O_RDONLY | O_LARGEFILE | O_NOATIME | (direct_io ? O_DIRECT : 0));
	if (dbase < 0)
	{
		// Non-root may fail when opening file of another user with O_NOATIME
		dbase = open(dbname.c_str(), O_RDONLY | O_LARGEFILE | (direct_io ? O_DIRECT : 0));
	}
	if (dbase < 0)
	{
		b_error::raise(uSvc, "Error (%d) opening database file: %s", errno, dbname.c_str());
	}

#ifdef POSIX_FADV_SEQUENTIAL
	int rc = fb_fadvise(dbase, 0, 0, POSIX_FADV_SEQUENTIAL);
	if (rc)
	{
		b_error::raise(uSvc, "Error (%d) in posix_fadvise(SEQUENTIAL) for %s", rc, dbname.c_str());
	}
#endif // POSIX_FADV_SEQUENTIAL
#ifdef POSIX_FADV_NOREUSE
	if (direct_io)
	{
		rc = fb_fadvise(dbase, 0, 0, POSIX_FADV_NOREUSE);
		if (rc)
			b_error::raise(uSvc, "Error (%d) in posix_fadvise(NOREUSE) for %s", rc, dbname.c_str());
	}
#endif // POSIX_FADV_NOREUSE

#endif // WIN_NT
}

void NBackup::create_database()
{
#ifdef WIN_NT
	dbase = CreateFile(dbname.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (dbase == INVALID_HANDLE_VALUE)
		b_error::raise(uSvc, "Error (%d) creating database file: %s", GetLastError(), dbname.c_str());
#else
	dbase = open(dbname.c_str(), O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE, 0660);
	if (dbase < 0)
		b_error::raise(uSvc, "Error (%d) creating database file: %s", errno, dbname.c_str());
#endif
}

void NBackup::close_database()
{
#ifdef WIN_NT
	CloseHandle(dbase);
#else
	close(dbase);
#endif
}

void NBackup::open_backup_scan()
{
#ifdef WIN_NT
	backup = CreateFile(bakname.c_str(), GENERIC_READ, 0,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (backup == INVALID_HANDLE_VALUE)
		b_error::raise(uSvc, "Error (%d) opening backup file: %s", GetLastError(), bakname.c_str());
#else
	backup = open(bakname.c_str(), O_RDONLY | O_LARGEFILE);
	if (backup < 0)
		b_error::raise(uSvc, "Error (%d) opening backup file: %s", errno, bakname.c_str());
#endif
}

void NBackup::create_backup()
{
#ifdef WIN_NT
	if (bakname == "stdout") {
		backup = GetStdHandle(STD_OUTPUT_HANDLE);
	}
	else
	{
		backup = CreateFile(bakname.c_str(), GENERIC_WRITE, FILE_SHARE_DELETE,
			NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	}
	if (backup == INVALID_HANDLE_VALUE)
		b_error::raise(uSvc, "Error (%d) creating backup file: %s", GetLastError(), bakname.c_str());
#else
	if (bakname == "stdout") {
		backup = 1; // Posix file handle for stdout
	}
	else
	{
		backup = open(bakname.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_LARGEFILE, 0660);
		if (backup < 0)
			b_error::raise(uSvc, "Error (%d) creating backup file: %s", errno, bakname.c_str());
	}
#endif
}

void NBackup::close_backup()
{
	if (bakname == "stdout")
		return;
#ifdef WIN_NT
	CloseHandle(backup);
#else
	close(backup);
#endif
}

void NBackup::fixup_database()
{
	open_database_write();
	Ods::header_page header;
	if (read_file(dbase, &header, sizeof(header)) != sizeof(header))
		b_error::raise(uSvc, "Unexpected end of database file", errno);
	const int backup_state = header.hdr_flags & Ods::hdr_backup_mask;
	if (backup_state != Jrd::nbak_state_stalled)
		b_error::raise(uSvc, "Database is not in state (%d) to be safely fixed up", backup_state);
	header.hdr_flags = (header.hdr_flags & ~Ods::hdr_backup_mask) | Jrd::nbak_state_normal;
	seek_file(dbase, 0);
	write_file(dbase, &header, sizeof(header));
	close_database();
}

/*
 *    Print the status, the SQLCODE, and exit.
 *    Also, indicate which operation the error occurred on.
 */
void NBackup::pr_error(const ISC_STATUS* status, const char* operation) const
{
	if (uSvc->isService())
		status_exception::raise(status);

	printf("[\n");
	printMsg(23, SafeArg() << operation); // PROBLEM ON "%s".

	isc_print_status(status);

	printf("SQLCODE:%"SLONGFORMAT"\n", isc_sqlcode(status));

	printf("]\n");

	b_error::raise(uSvc, "Database error");
}

void NBackup::attach_database()
{
	if (username.length() > 255 || password.length() > 255)
		b_error::raise(uSvc, "Username or password is too long");

	ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);

	if (username.hasData()) {
		dpb.insertString(isc_dpb_user_name, username);
	}

	if (password.hasData()) {
		dpb.insertString(isc_dpb_password, password);
	}

	if (trustedUser.hasData())
	{
		uSvc->checkService();
		dpb.insertString(isc_dpb_trusted_auth, trustedUser);
	}

	if (trustedRole)
	{
		uSvc->checkService();
		dpb.insertString(isc_dpb_trusted_role, ADMIN_ROLE, strlen(ADMIN_ROLE));
	}

	if (!run_db_triggers)
		dpb.insertByte(isc_dpb_no_db_triggers, 1);

	if (isc_attach_database(status, 0, database.c_str(), &newdb,
		dpb.getBufferLength(), reinterpret_cast<const char*>(dpb.getBuffer())))
	{
		pr_error(status, "attach database");
	}
}

void NBackup::detach_database()
{
	if (trans)
	{
		if (isc_rollback_transaction(status, &trans))
			pr_error(status, "rollback transaction");
	}
	if (isc_detach_database(status, &newdb))
		pr_error(status, "detach database");
}

void NBackup::internal_lock_database()
{
	if (isc_start_transaction(status, &trans, 1, &newdb, 0, NULL))
		pr_error(status, "start transaction");
	if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, "ALTER DATABASE BEGIN BACKUP", 1, NULL))
		pr_error(status, "begin backup");
	if (isc_commit_transaction(status, &trans))
		pr_error(status, "begin backup: commit");
}

void NBackup::get_database_size()
{
	db_size_pages = 0;
	const char fs[] = {isc_info_db_file_size};
	char res[128];
	if (isc_database_info(status, &newdb, sizeof(fs), fs, sizeof(res), res))
	{
		pr_error(status, "size info");
	}
	else if (res[0] == isc_info_db_file_size)
	{
		USHORT len = isc_vax_integer (&res[1], 2);
		db_size_pages = isc_vax_integer (&res[3], len);
	}
}

void NBackup::internal_unlock_database()
{
	if (isc_start_transaction(status, &trans, 1, &newdb, 0, NULL))
		pr_error(status, "start transaction");
	if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, "ALTER DATABASE END BACKUP", 1, NULL))
		pr_error(status, "end backup");
	if (isc_commit_transaction(status, &trans))
		pr_error(status, "end backup: commit");
}

void NBackup::lock_database(bool get_size)
{
	attach_database();
	db_size_pages = 0;
	try {
		internal_lock_database();
		if (get_size)
		{
			get_database_size();
			if (db_size_pages && (!uSvc->isService()))
				printf("%d\n", db_size_pages);
		}
	}
	catch (const Exception&)
	{
		detach_database();
		throw;
	}
	detach_database();
}

void NBackup::unlock_database()
{
	attach_database();
	try {
		internal_unlock_database();
	}
	catch (const Exception&)
	{
		detach_database();
		throw;
	}
	detach_database();
}

void NBackup::backup_database(int level, const PathName& fname)
{
	bool database_locked = false;
	// We set this flag when backup file is in inconsistent state
	bool delete_backup = false;
	ULONG prev_scn = 0;
	char prev_guid[GUID_BUFF_SIZE] = "";
	Ods::pag* page_buff = NULL;
	attach_database();
	try {
		// Look for SCN and GUID of previous-level backup in history table
		if (level)
		{
			if (isc_start_transaction(status, &trans, 1, &newdb, 0, NULL))
				pr_error(status, "start transaction");
			char out_sqlda_data[XSQLDA_LENGTH(2)];
			XSQLDA *out_sqlda = (XSQLDA*)out_sqlda_data;
			out_sqlda->version = SQLDA_VERSION1;
			out_sqlda->sqln = 2;

			isc_stmt_handle stmt = 0;
			if (isc_dsql_allocate_statement(status, &newdb, &stmt))
				pr_error(status, "allocate statement");
			char str[200];
			sprintf(str, "select rdb$guid, rdb$scn from rdb$backup_history "
				"where rdb$backup_id = "
				  "(select max(rdb$backup_id) from rdb$backup_history "
				   "where rdb$backup_level = %d)", level - 1);
			if (isc_dsql_prepare(status, &trans, &stmt, 0, str, 1, NULL))
				pr_error(status, "prepare history query");
			if (isc_dsql_describe(status, &stmt, 1, out_sqlda))
				pr_error(status, "describe history query");
			short guid_null, scn_null;
			out_sqlda->sqlvar[0].sqlind = &guid_null;
			out_sqlda->sqlvar[0].sqldata = prev_guid;
			out_sqlda->sqlvar[1].sqlind = &scn_null;
			out_sqlda->sqlvar[1].sqldata = (char*)&prev_scn;
			if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
				pr_error(status, "execute history query");

			switch (isc_dsql_fetch(status, &stmt, 1, out_sqlda))
			{
			case 100: // No more records available
				b_error::raise(uSvc, "Cannot find record for database \"%s\" backup level %d "
					"in the backup history", database.c_str(), level - 1);
			case 0:
				if (guid_null || scn_null)
					b_error::raise(uSvc, "Internal error. History query returned null SCN or GUID");
				prev_guid[sizeof(prev_guid) - 1] = 0;
				break;
			default:
				pr_error(status, "fetch history query");
			}
			isc_dsql_free_statement(status, &stmt, DSQL_close);
			if (isc_commit_transaction(status, &trans))
				pr_error(status, "commit history query");
		}

		// Lock database for backup
		internal_lock_database();
		database_locked = true;
		get_database_size();
		detach_database();

		time_t _time = time(NULL);
		const struct tm *today = localtime(&_time);

		if (fname.hasData())
			bakname = fname;
		else
		{
			// Let's generate nice new filename
			PathName begin, fil;
			PathUtils::splitLastComponent(begin, fil, database);
			bakname.printf("%s-%d-%04d%02d%02d-%02d%02d.nbk", fil.c_str(), level,
				today->tm_year + 1900, today->tm_mon + 1, today->tm_mday,
				today->tm_hour, today->tm_min);
			if (!uSvc->isService())
				printf("%s", bakname.c_str()); // Print out generated filename for script processing
		}

		// Level 0 backup is a full reconstructed database image that can be
		// used directly after fixup. Incremenal backups of other levels are
		// consisted of header followed by page data. Each page is preceded
		// by 4-byte integer page number

		// Actual IO is optimized to get maximum performance
		// from the IO subsystem while taking as little CPU time as possible

		// NOTE: this is still possible to improve performance by implementing
		// version using asynchronous unbuffered IO on NT series of OS.
		// But this task is for another day. 02 Aug 2003, Nickolay Samofatov.

		// Create backup file and open database file
		create_backup();
		delete_backup = true;

		open_database_scan();

		// Read database header
		char unaligned_header_buffer[SECTOR_ALIGNMENT * 2];

		Ods::header_page *header =
			reinterpret_cast<Ods::header_page*>(
				FB_ALIGN((IPTR) unaligned_header_buffer, SECTOR_ALIGNMENT));
		if (read_file(dbase, header, SECTOR_ALIGNMENT/*sizeof(*header)*/) != SECTOR_ALIGNMENT/*sizeof(*header)*/)
			b_error::raise(uSvc, "Unexpected end of file when reading header of database file");
		if ((header->hdr_flags & Ods::hdr_backup_mask) != Jrd::nbak_state_stalled)
		{
			b_error::raise(uSvc, "Internal error. Database file is not locked. Flags are %d",
				header->hdr_flags);
		}

		Array<UCHAR> unaligned_page_buffer;
		{ // scope
			UCHAR* buf = unaligned_page_buffer.getBuffer(header->hdr_page_size + SECTOR_ALIGNMENT);
			page_buff = reinterpret_cast<Ods::pag*>(FB_ALIGN((IPTR) buf, SECTOR_ALIGNMENT));
		} // end scope

		ULONG db_size = db_size_pages;
		seek_file(dbase, 0);

		if (read_file(dbase, page_buff, header->hdr_page_size) != header->hdr_page_size)
			b_error::raise(uSvc, "Unexpected end of file when reading header of database file (stage 2)");
		--db_size;

		FB_GUID backup_guid;
		bool guid_found = false;
		const UCHAR* p = reinterpret_cast<Ods::header_page*>(page_buff)->hdr_data;
		while (true)
		{
			switch (*p)
			{
			case Ods::HDR_backup_guid:
				if (p[1] != sizeof(FB_GUID))
					break;
				memcpy(&backup_guid, p + 2, sizeof(FB_GUID));
				guid_found = true;
				break;
			case Ods::HDR_difference_file:
				p += p[1] + 2;
				continue;
			}
			break;
		}

		if (!guid_found)
			b_error::raise(uSvc, "Internal error. Cannot get backup guid clumplet");

		// Write data to backup file
		ULONG backup_scn = header->hdr_header.pag_scn - 1;
		if (level)
		{
			inc_header bh;
			memcpy(bh.signature, backup_signature, sizeof(backup_signature));
			bh.version = 1;
			bh.level = level;
			bh.backup_guid = backup_guid;
			StringToGuid(&bh.prev_guid, prev_guid);
			bh.page_size = header->hdr_page_size;
			bh.backup_scn = backup_scn;
			bh.prev_scn = prev_scn;
			write_file(backup, &bh, sizeof(bh));
		}

		ULONG curPage = 0;

		// Starting from ODS 11.1 we can expand file but never use some last
		// pages in it. There are no need to backup this empty pages. More,
		// we can't be sure its not used pages have right SCN assigned.
		// How many pages are really used we know from pip_header.reserved
		// where stored number of pages allocated from this pointer page.
		// In ODS 12 it will be moved into corresponding field of page_inv_page.
		const bool isODS11_x = ((header->hdr_ods_version & ~ODS_FIREBIRD_FLAG) == 11) &&
								(header->hdr_ods_minor_original >= 1);
		ULONG lastPage = 1; // first PIP must be at page number 1
		const ULONG pagesPerPIP =
			(header->hdr_page_size - OFFSETA(Ods::page_inv_page*, pip_bits)) * 8;

		while (true)
		{
			if (curPage && page_buff->pag_scn > backup_scn)
				b_error::raise(uSvc, "Internal error. Database page %d had been changed during backup"
							   " (page SCN=%d, backup SCN=%d)", curPage,
							   page_buff->pag_scn, backup_scn);
			if (level)
			{
				if (page_buff->pag_scn > prev_scn)
				{
					write_file(backup, &curPage, sizeof(curPage));
					write_file(backup, page_buff, header->hdr_page_size);
				}
			}
			else
				write_file(backup, page_buff, header->hdr_page_size);

			checkCtrlC(uSvc);

			if ((db_size_pages != 0) && (db_size == 0))
				break;

			const size_t bytesDone = read_file(dbase, page_buff, header->hdr_page_size);
			--db_size;
			if (bytesDone == 0)
				break;
			if (bytesDone != header->hdr_page_size)
				b_error::raise(uSvc, "Database file size is not a multiply of page size");
			curPage++;

			if (isODS11_x && curPage == lastPage)
			{
				if (page_buff->pag_type == pag_pages)
				{
					if (lastPage == 1)
						lastPage = page_buff->reserved - 1;
					else
						lastPage += page_buff->reserved;

					if (page_buff->reserved < pagesPerPIP)
						lastPage++;
				}
				else
				{
					fb_assert(page_buff->pag_type == pag_undefined);
					break;
				}
			}
		}
		close_database();
		close_backup();

		delete_backup = false; // Backup file is consistent. No need to delete it

		attach_database();
		// Write about successful backup to backup history table
		if (isc_start_transaction(status, &trans, 1, &newdb, 0, NULL))
			pr_error(status, "start transaction");
		char in_sqlda_data[XSQLDA_LENGTH(4)];
		XSQLDA *in_sqlda = (XSQLDA *)in_sqlda_data;
		in_sqlda->version = SQLDA_VERSION1;
		in_sqlda->sqln = 4;
		isc_stmt_handle stmt = 0;
		if (isc_dsql_allocate_statement(status, &newdb, &stmt))
			pr_error(status, "allocate statement");
		if (isc_dsql_prepare(status, &trans, &stmt, 0,
			"insert into rdb$backup_history(rdb$backup_id, rdb$timestamp,"
			  "rdb$backup_level, rdb$guid, rdb$scn, rdb$file_name)"
			"values(gen_id(rdb$backup_history, 1), 'now', ?, ?, ?, ?)",
			1, NULL))
		{
			pr_error(status, "prepare history insert");
		}
		if (isc_dsql_describe_bind(status, &stmt, 1, in_sqlda))
			pr_error(status, "bind history insert");
		short null_flag = 0;
		in_sqlda->sqlvar[0].sqldata = (char*)&level;
		in_sqlda->sqlvar[0].sqlind = &null_flag;
		char temp[GUID_BUFF_SIZE];
		GuidToString(temp, &backup_guid);
		in_sqlda->sqlvar[1].sqldata = temp;
		in_sqlda->sqlvar[1].sqlind = &null_flag;
		in_sqlda->sqlvar[2].sqldata = (char*)&backup_scn;
		in_sqlda->sqlvar[2].sqlind = &null_flag;

		char buff[256]; // RDB$FILE_NAME has length of 253
		size_t len = bakname.length();
		if (len > 253)
			len = 253;
		*(USHORT*) buff = len;
		memcpy(buff + 2, bakname.c_str(), len);
		in_sqlda->sqlvar[3].sqldata = buff;
		in_sqlda->sqlvar[3].sqlind = &null_flag;
		if (isc_dsql_execute(status, &trans, &stmt, 1, in_sqlda))
			pr_error(status, "execute history insert");
		isc_dsql_free_statement(status, &stmt, DSQL_drop);
		if (isc_commit_transaction(status, &trans))
			pr_error(status, "commit history insert");

	}
	catch (const Exception&)
	{
		if (delete_backup)
			remove(bakname.c_str());
		if (trans)
		{
			if (isc_rollback_transaction(status, &trans))
				pr_error(status, "rollback transaction");
		}
		if (database_locked)
		{
			if (!newdb)
				attach_database();
			internal_unlock_database();
		}
		if (newdb)
			detach_database();
		throw;
	}

	if (!newdb)
		attach_database();
	internal_unlock_database();
	detach_database();
}

void NBackup::restore_database(const BackupFiles& files)
{
	// We set this flag when database file is in inconsistent state
	bool delete_database = false;
	const int filecount = files.getCount();
#ifndef WIN_NT
	create_database();
	delete_database = true;
#endif
	UCHAR *page_buffer = NULL;
	try {
		int curLevel = 0;
		FB_GUID prev_guid;
		while (true)
		{
			if (!filecount)
			{
				while (true)
				{
					if (uSvc->isService())
						bakname = ".";
					else
					{
						printf("Enter name of the backup file of level %d "
							   "(\".\" - do not restore further): \n", curLevel);
						char temp[256];
						scanf("%255s", temp);
						bakname = temp;
					}
					if (bakname == ".")
					{
						close_database();
						if (!curLevel)
						{
							remove(dbname.c_str());
							b_error::raise(uSvc, "Level 0 backup is not restored");
						}
						fixup_database();
						delete[] page_buffer;
						return;
					}
					// Never reaches this point when run as service
					try {
#ifdef WIN_NT
						if (curLevel)
#endif
							open_backup_scan();
						break;
					}
					catch (const Exception& e) {
						printf("%s\n", e.what());
					}
				}
			}
			else
			{
				if (curLevel >= filecount)
				{
					close_database();
					fixup_database();
					delete[] page_buffer;
					return;
				}
				bakname = files[curLevel];
#ifdef WIN_NT
				if (curLevel)
#endif
					open_backup_scan();
			}

			if (curLevel)
			{
				inc_header bakheader;
				if (read_file(backup, &bakheader, sizeof(bakheader)) != sizeof(bakheader))
					b_error::raise(uSvc, "Unexpected end of file when reading header of backup file: %s",
								   bakname.c_str());
				if (memcmp(bakheader.signature, backup_signature, sizeof(backup_signature)) != 0)
					b_error::raise(uSvc, "Invalid incremental backup file: %s", bakname.c_str());
				if (bakheader.version != 1)
					b_error::raise(uSvc, "Unsupported version %d of incremental backup file: %s",
								   bakheader.version, bakname.c_str());
				if (bakheader.level != curLevel)
					b_error::raise(uSvc, "Invalid level %d of incremental backup file: %s, expected %d",
						bakheader.level, bakname.c_str(), curLevel);
				// We may also add SCN check, but GUID check covers this case too
				if (memcmp(&bakheader.prev_guid, &prev_guid, sizeof(FB_GUID)) != 0)
				{
					b_error::raise(uSvc,
						"Wrong order of backup files or "
						"invalid incremental backup file detected, file: %s", bakname.c_str());
				}
				delete_database = true;
				prev_guid = bakheader.backup_guid;
				while (true)
				{
					ULONG pageNum;
					const size_t bytesDone = read_file(backup, &pageNum, sizeof(pageNum));
					if (bytesDone == 0)
						break;
					if (bytesDone != sizeof(pageNum) ||
						read_file(backup, page_buffer, bakheader.page_size) != bakheader.page_size)
					{
						b_error::raise(uSvc, "Unexpected end of backup file: %s", bakname.c_str());
					}
					seek_file(dbase, ((SINT64)pageNum)*bakheader.page_size);
					write_file(dbase, page_buffer, bakheader.page_size);
					checkCtrlC(uSvc);
				}
				delete_database = false;
			}
			else
			{
#ifdef WIN_NT
				if (!CopyFile(bakname.c_str(), dbname.c_str(), TRUE))
				{
					b_error::raise(uSvc, "Error (%d) creating database file: %s via copying from: %s",
						GetLastError(), dbname.c_str(), bakname.c_str());
				}
				checkCtrlC(uSvc);
				delete_database = true; // database is possibly broken
				open_database_write();
#else
				// Use relatively small buffer to make use of prefetch and lazy flush
				char buffer[65536];
				while (true)
				{
					const size_t bytesRead = read_file(backup, buffer, sizeof(buffer));
					if (bytesRead == 0)
						break;
					write_file(dbase, buffer, bytesRead);
					checkCtrlC(uSvc);
				}
				seek_file(dbase, 0);
#endif
				// Read database header
				Ods::header_page header;
				if (read_file(dbase, &header, sizeof(header)) != sizeof(header))
					b_error::raise(uSvc, "Unexpected end of file when reading restored database header");
				page_buffer = FB_NEW(*getDefaultMemoryPool()) UCHAR[header.hdr_page_size];

				seek_file(dbase, 0);

				if (read_file(dbase, page_buffer, header.hdr_page_size) != header.hdr_page_size)
					b_error::raise(uSvc, "Unexpected end of file when reading header of restored database file (stage 2)");

				bool guid_found = false;
				const UCHAR* p = reinterpret_cast<Ods::header_page*>(page_buffer)->hdr_data;
				while (true)
				{
					switch (*p)
					{
					case Ods::HDR_backup_guid:
						if (p[1] != sizeof(FB_GUID))
							break;
						memcpy(&prev_guid, p + 2, sizeof(FB_GUID));
						guid_found = true;
						break;
					case Ods::HDR_difference_file:
						p += p[1] + 2;
						continue;
					}
					break;
				}
				if (!guid_found)
					b_error::raise(uSvc, "Cannot get backup guid clumplet from L0 backup");
				// We are likely to have normal database here
				delete_database = false;
			}
#ifdef WIN_NT
			if (curLevel)
#endif
				close_backup();
			curLevel++;
		}
	}
	catch (const Exception&)
	{
		delete[] page_buffer;
		if (delete_database)
			remove(dbname.c_str());
		throw;
	}
}

THREAD_ENTRY_DECLARE NBACKUP_main(THREAD_ENTRY_PARAM arg)
{
	UtilSvc* uSvc = (UtilSvc*) arg;
	int exit_code = FB_SUCCESS;

	try {
		nbackup(uSvc);
	}
	catch (const Exception& e)
	{
		ISC_STATUS_ARRAY status;
		e.stuff_exception(status);
		uSvc->initStatus();
		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	uSvc->started();
	uSvc->finish();
	return (THREAD_ENTRY_RETURN)(IPTR) exit_code;
}

enum NbOperation {nbNone, nbLock, nbUnlock, nbFixup, nbBackup, nbRestore};

void nbackup(UtilSvc* uSvc)
{
	UtilSvc::ArgvType& argv = uSvc->argv;
	const int argc = argv.getCount();

	NbOperation op = nbNone;
	string username, password;
	PathName database, filename;
	bool run_db_triggers = true;
	bool direct_io = 
#ifdef WIN_NT
		true;
#else
		false;
#endif
	NBackup::BackupFiles backup_files;
	int level;
	bool print_size = false, version = false;
	string trustedUser;
	bool trustedRole = false;
	string onOff;

	// Read global command line parameters
	for (int itr = 1; itr < argc; ++itr)
	{
		// We must recognize all parameters here
		if (argv[itr][0] != '-') {
			usage(uSvc, "Unrecognized parameter %s", argv[itr]);
		}

		if (uSvc->isService())
		{
			PathName sw = &argv[itr][1];
			sw.upper();
			if (sw == TRUSTED_USER_SWITCH)
			{
				if (++itr >= argc)
					missing_parameter_for_switch(uSvc, argv[itr - 1]);
				trustedUser = argv[itr];
				continue;
			}
			if (sw == TRUSTED_ROLE_SWITCH)
			{
				trustedRole = true;
				continue;
			}
		}

		switch (UPPER(argv[itr][1]))
		{
		case 'U':
			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			username = argv[itr];
			break;

		case 'P':
			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			password = argv[itr];
			uSvc->hidePasswd(argv, itr);
			break;

		case 'T':
			run_db_triggers = false;
			break;

		case 'D':
			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			onOff = argv[itr];
			onOff.upper();
			if (onOff == "ON")
				direct_io = true;
			else if (onOff == "OFF")
				direct_io = false;
			else
				usage(uSvc, "Wrong parameter %s for switch -D, need ON or OFF", onOff.c_str());
			break;

		case 'F':
			if (UPPER(argv[itr][2]) == 'E')
			{
				if (uSvc->isService())
				{
					usage(uSvc, "Fetch password can't be used in service mode");
					break;
				}

				if (++itr >= argc)
					missing_parameter_for_switch(uSvc, argv[itr - 1]);

				const char* passwd = NULL;
				if (fb_utils::fetchPassword(argv[itr], passwd) != fb_utils::FETCH_PASS_OK)
				{
					usage(uSvc, "Error working with password file");
					break;
				}
				password = passwd;
				break;
			}

			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbFixup;
			break;

		case 'L':
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbLock;
			break;

		case 'N':
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbUnlock;
			break;

		case 'B':
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			level = atoi(argv[itr]);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 2]);

			database = argv[itr];

			if (itr + 1 < argc)
				filename = argv[++itr];

			op = nbBackup;
			break;

		case 'R':
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missing_parameter_for_switch(uSvc, argv[itr - 1]);

			database = argv[itr];
			while (++itr < argc)
				backup_files.push(argv[itr]);

			op = nbRestore;
			break;

		case 'S':
			print_size = true;
			break;

		case '?':
			if (uSvc->isService())
				usage(uSvc, "Unknown switch %s", argv[itr]);
			else
				usage(uSvc, NULL);
			break;

		case 'Z':
			if (uSvc->isService())
				usage(uSvc, "Unknown switch %s", argv[itr]);
			else
				version = true;
			break;

		default:
			usage(uSvc, "Unknown switch %s", argv[itr]);
			break;
		}
	}

	if (version)
	{
		printf("Physical Backup Manager version %s\n", FB_VERSION);
	}
	if (op == nbNone)
	{
		if (!version)
		{
			usage(uSvc, "None of -L, -N, -F, -B or -R specified");
		}
		exit(FINI_OK);
	}

	if (print_size && (op != nbLock))
		usage(uSvc, "Switch -S can be used only with -L");

	NBackup nbk(uSvc, database, username, password, run_db_triggers, trustedUser, trustedRole, direct_io);
	switch (op)
	{
		case nbLock:
			nbk.lock_database(print_size);
			break;

		case nbUnlock:
			nbk.unlock_database();
			break;

		case nbFixup:
			nbk.fixup_database();
			break;

		case nbBackup:
			nbk.backup_database(level, filename);
			break;

		case nbRestore:
			nbk.restore_database(backup_files);
			break;
	}
}
