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

#include "../common/db_alias.h"
#include "../jrd/ods.h"
#include "../jrd/nbak.h"
#include "../yvalve/gds_proto.h"
#include "../common/os/path_utils.h"
#include "../common/os/guid.h"
#include "../jrd/ibase.h"
#include "../common/utils_proto.h"
#include "../common/classes/array.h"
#include "../common/classes/ClumpletWriter.h"
#include "../utilities/nbackup/nbk_proto.h"
#include "../jrd/license.h"
#include "../jrd/ods_proto.h"
#include "../common/classes/MsgPrint.h"
#include "../common/classes/Switches.h"
#include "../utilities/nbackup/nbkswi.h"
#include "../common/isc_f_proto.h"

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

	void printMsg(USHORT number, const SafeArg& arg, bool newLine = true)
	{
		char buffer[256];
		fb_msg_format(NULL, nbackup_msg_fac, number, sizeof(buffer), buffer, arg);
		if (newLine)
			printf("%s\n", buffer);
		else
			printf("%s", buffer);
	}

	void printMsg(USHORT number, bool newLine = true)
	{
		static const SafeArg dummy;
		printMsg(number, dummy, newLine);
	}

	void usage(UtilSvc* uSvc, const ISC_STATUS code, const char* message = NULL)
	{
		/*
		string msg;
		va_list params;
		if (message)
		{
			va_start(params, message);
			msg.vprintf(message, params);
			va_end(params);
		}
		*/

		if (uSvc->isService())
		{
			//fb_assert(message != NULL);
			//(Arg::Gds(isc_random) << msg).raise();
			fb_assert(code);
			Arg::Gds gds(code);
			if (message)
				gds << message;
			gds.raise();
		}

		if (code)
		{
			printMsg(1, false); // ERROR:
			USHORT dummy;
			USHORT number = (USHORT) gds__decode(code, &dummy, &dummy);
			fb_assert(number);
			if (message)
				printMsg(number, SafeArg() << message);
			else
				printMsg(number);
			printf("\n");
		}

		const int mainUsage[] = { 2, 3, 4, 5, 6, 0 };
		const int notes[] = { 19, 20, 21, 22, 26, 27, 28, 0 };
		const Switches::in_sw_tab_t* const base = nbackup_action_in_sw_table;

		for (int i = 0; mainUsage[i]; ++i)
			printMsg(mainUsage[i]);

		printMsg(7); // exclusive options are:
		for (const Switches::in_sw_tab_t* p = base; p->in_sw; ++p)
		{
			if (p->in_sw_msg && p->in_sw_optype == nboExclusive)
				printMsg(p->in_sw_msg);
		}

		printMsg(72); // special options are:
		for (const Switches::in_sw_tab_t* p = base; p->in_sw; ++p)
		{
			if (p->in_sw_msg && p->in_sw_optype == nboSpecial)
				printMsg(p->in_sw_msg);
		}

		printMsg(24); // general options are:
		for (const Switches::in_sw_tab_t* p = base; p->in_sw; ++p)
		{
			if (p->in_sw_msg && p->in_sw_optype == nboGeneral)
				printMsg(p->in_sw_msg);
		}

		printMsg(25); // msg 25 switches can be abbreviated to the unparenthesized characters

		for (int i = 0; notes[i]; ++i)
			printMsg(notes[i]);

		exit(FINI_ERROR);
	}

	void missingParameterForSwitch(UtilSvc* uSvc, const char* sw)
	{
		usage(uSvc, isc_nbackup_missing_param, sw);
	}

	void singleAction(UtilSvc* uSvc)
	{
		usage(uSvc, isc_nbackup_allowed_switches);
	}

	const int MSG_LEN = 1024;

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


static void checkCtrlC(UtilSvc* /*uSvc*/)
{
	if (flShutdown)
	{
		Arg::Gds(isc_nbackup_user_stop).raise();
	}
}


#ifdef WIN_NT
#define FILE_HANDLE HANDLE
#else
#define FILE_HANDLE int
#endif


const char localhost[] = "localhost";

const char backup_signature[4] = {'N','B','A','K'};

struct inc_header
{
	char signature[4];		// 'NBAK'
	SSHORT version;			// Incremental backup format version.
	SSHORT level;			// Backup level.
	// \\\\\ ---- this is 8 bytes. should not cause alignment problems
	Guid backup_guid;		// GUID of this backup
	Guid prev_guid;			// GUID of previous level backup
	ULONG page_size;		// Size of pages in the database and backup file
	// These fields are currently filled, but not used. May be used in future versions
	ULONG backup_scn;		// SCN of this backup
	ULONG prev_scn;			// SCN of previous level backup
};

class NBackup
{
public:
	NBackup(UtilSvc* _uSvc, const PathName& _database, const string& _username,
			const string& _password, bool _run_db_triggers/*, const string& _trustedUser,
			bool _trustedRole*/, bool _direct_io)
	  : uSvc(_uSvc), newdb(0), trans(0), database(_database),
		username(_username), password(_password), /*trustedUser(_trustedUser),*/
		run_db_triggers(_run_db_triggers), /*trustedRole(_trustedRole), */direct_io(_direct_io),
		dbase(0), backup(0), db_size_pages(0), m_odsNumber(0), m_silent(false), m_printed(false)
	{
		// Recognition of local prefix allows to work with
		// database using TCP/IP loopback while reading file locally.
		// RS: Maybe check if host is loopback via OS functions is more correct
		PathName db(_database), host_port;
		if (ISC_extract_host(db, host_port, false) == ISC_PROTOCOL_TCPIP)
		{
			const PathName host = host_port.substr(0, sizeof(localhost) - 1);
			const char delim = host_port.length() >= sizeof(localhost) ? host_port[sizeof(localhost) - 1] : '/';
			if (delim != '/' || !host.equalsNoCase(localhost))
				pr_error(status, "nbackup needs local access to database file");
		}

		expandDatabaseName(db, dbname, NULL);

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

	bool printed() const
	{
		return m_printed;
	}

private:
	UtilSvc* uSvc;

    ISC_STATUS_ARRAY status; // status vector
	isc_db_handle newdb; // database handle
    isc_tr_handle trans; // transaction handle

	PathName database;
	string username, password/*, trustedUser*/;
	bool run_db_triggers, /*trustedRole, */direct_io;

	PathName dbname; // Database file name
	PathName bakname;
	FILE_HANDLE dbase;
	FILE_HANDLE backup;
	ULONG db_size_pages;	// In pages
	USHORT m_odsNumber;
	bool m_silent;		// are we already handling an exception?
	bool m_printed;		// pr_error() was called to print status vector

	// IO functions
	size_t read_file(FILE_HANDLE &file, void *buffer, size_t bufsize);
	void write_file(FILE_HANDLE &file, void *buffer, size_t bufsize);
	void seek_file(FILE_HANDLE &file, SINT64 pos);

	void pr_error(const ISC_STATUS* status, const char* operation);

	void internal_lock_database();
	void get_database_size();
	void get_ods();
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
	if (ReadFile(file, buffer, bufsize, &bytesDone, NULL))
		return bytesDone;
#else
	const ssize_t res = read(file, buffer, bufsize);
	if (res >= 0)
		return res;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_read) <<
		(&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown") <<
		Arg::OsError());

	return 0; // silence compiler
}

void NBackup::write_file(FILE_HANDLE &file, void *buffer, size_t bufsize)
{
#ifdef WIN_NT
	DWORD bytesDone;
	if (WriteFile(file, buffer, bufsize, &bytesDone, NULL) && bytesDone == bufsize)
		return;
#else
	if (write(file, buffer, bufsize) == (ssize_t) bufsize)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_write) <<
		(&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown") <<
		Arg::OsError());
}

void NBackup::seek_file(FILE_HANDLE &file, SINT64 pos)
{
#ifdef WIN_NT
	LARGE_INTEGER offset;
	offset.QuadPart = pos;
	if (SetFilePointer(dbase, offset.LowPart, &offset.HighPart, FILE_BEGIN) !=
			INVALID_SET_FILE_POINTER ||
		GetLastError() == NO_ERROR)
	{
		return;
	}
#else
	if (lseek(file, pos, SEEK_SET) != (off_t) -1)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_seek) <<
		(&file == &dbase ? dbname.c_str() :
			&file == &backup ? bakname.c_str() : "unknown") <<
		Arg::OsError());
}

void NBackup::open_database_write()
{
#ifdef WIN_NT
	dbase = CreateFile(dbname.c_str(), GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
		NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (dbase != INVALID_HANDLE_VALUE)
		return;
#else
	dbase = open(dbname.c_str(), O_RDWR | O_LARGEFILE);
	if (dbase >= 0)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_opendb) << dbname.c_str() << Arg::OsError());
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
		status_exception::raise(Arg::Gds(isc_nbackup_err_opendb) << dbname.c_str() << Arg::OsError());

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
		status_exception::raise(Arg::Gds(isc_nbackup_err_opendb) << dbname.c_str() << Arg::OsError());
	}

#ifdef POSIX_FADV_SEQUENTIAL
	int rc = fb_fadvise(dbase, 0, 0, POSIX_FADV_SEQUENTIAL);
	if (rc)
	{
		status_exception::raise(Arg::Gds(isc_nbackup_err_fadvice) <<
								"SEQUENTIAL" << dbname.c_str() << Arg::Unix(rc));
	}
#endif // POSIX_FADV_SEQUENTIAL

#ifdef POSIX_FADV_NOREUSE
	if (direct_io)
	{
		rc = fb_fadvise(dbase, 0, 0, POSIX_FADV_NOREUSE);
		if (rc)
		{
			status_exception::raise(Arg::Gds(isc_nbackup_err_fadvice) <<
									"NOREUSE" << dbname.c_str() << Arg::Unix(rc));
		}
	}
#endif // POSIX_FADV_NOREUSE

#endif // WIN_NT
}

void NBackup::create_database()
{
#ifdef WIN_NT
	dbase = CreateFile(dbname.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_DELETE,
		NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL | FILE_FLAG_SEQUENTIAL_SCAN, NULL);
	if (dbase != INVALID_HANDLE_VALUE)
		return;
#else
	dbase = open(dbname.c_str(), O_RDWR | O_CREAT | O_EXCL | O_LARGEFILE, 0660);
	if (dbase >= 0)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_createdb) << dbname.c_str() << Arg::OsError());
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
	if (backup != INVALID_HANDLE_VALUE)
		return;
#else
	backup = open(bakname.c_str(), O_RDONLY | O_LARGEFILE);
	if (backup >= 0)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_openbk) << bakname.c_str() << Arg::OsError());
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
	if (backup != INVALID_HANDLE_VALUE)
		return;
#else
	if (bakname == "stdout")
	{
		backup = 1; // Posix file handle for stdout
		return;
	}
	backup = open(bakname.c_str(), O_WRONLY | O_CREAT | O_EXCL | O_LARGEFILE, 0660);
	if (backup >= 0)
		return;
#endif

	status_exception::raise(Arg::Gds(isc_nbackup_err_createbk) << bakname.c_str() << Arg::OsError());
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
		status_exception::raise(Arg::Gds(isc_nbackup_err_eofdb) << dbname.c_str());

	const int backup_state = header.hdr_flags & Ods::hdr_backup_mask;
	if (backup_state != Jrd::nbak_state_stalled)
	{
		status_exception::raise(Arg::Gds(isc_nbackup_fixup_wrongstate) << dbname.c_str() <<
			Arg::Num(Jrd::nbak_state_stalled));
	}
	header.hdr_flags = (header.hdr_flags & ~Ods::hdr_backup_mask) | Jrd::nbak_state_normal;
	seek_file(dbase, 0);
	write_file(dbase, &header, sizeof(header));
	close_database();
}


// Print the status, the SQLCODE, and exit.
// Also, indicate which operation the error occurred on.
void NBackup::pr_error(const ISC_STATUS* status, const char* operation)
{
	if (uSvc->isService())
		status_exception::raise(status);

	printf("[\n");
	printMsg(23, SafeArg() << operation); // PROBLEM ON "%s".
	isc_print_status(status);
	printf("SQLCODE:%"SLONGFORMAT"\n", isc_sqlcode(status));
	printf("]\n");

	m_printed = true;

	status_exception::raise(Arg::Gds(isc_nbackup_err_db));
}

void NBackup::attach_database()
{
	if (username.length() > 255 || password.length() > 255)
	{
		if (m_silent)
			return;
		status_exception::raise(Arg::Gds(isc_nbackup_userpw_toolong));
	}

	ClumpletWriter dpb(ClumpletReader::Tagged, MAX_DPB_SIZE, isc_dpb_version1);

	const unsigned char* authBlock;
	unsigned int authBlockSize = uSvc->getAuthBlock(&authBlock);
	if (authBlockSize)
	{
		dpb.insertBytes(isc_dpb_auth_block, authBlock, authBlockSize);
	}
	else
	{
		if (username.hasData())
			dpb.insertString(isc_dpb_user_name, username);

		if (password.hasData())
			dpb.insertString(isc_dpb_password, password);

		/***
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
		***/
	}

	if (!run_db_triggers)
		dpb.insertByte(isc_dpb_no_db_triggers, 1);

	if (m_silent)
	{
		ISC_STATUS_ARRAY temp;
		isc_attach_database(temp, 0, database.c_str(), &newdb,
			dpb.getBufferLength(), reinterpret_cast<const char*>(dpb.getBuffer()));
	}
	else if (isc_attach_database(status, 0, database.c_str(), &newdb,
				dpb.getBufferLength(), reinterpret_cast<const char*>(dpb.getBuffer())))
	{
		pr_error(status, "attach database");
	}
}

void NBackup::detach_database()
{
	if (m_silent)
	{
		ISC_STATUS_ARRAY temp;
		if (trans)
			isc_rollback_transaction(temp, &trans);

		isc_detach_database(temp, &newdb);
	}
	else
	{
		if (trans)
		{
			if (isc_rollback_transaction(status, &trans))
				pr_error(status, "rollback transaction");
		}
		if (isc_detach_database(status, &newdb))
			pr_error(status, "detach database");
	}
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

void NBackup::get_ods()
{
	m_odsNumber = 0;
	const char db_version_info[] = { isc_info_ods_version };
	char res[128];
	if (isc_database_info(status, &newdb, sizeof(db_version_info), db_version_info, sizeof(res), res))
	{
		pr_error(status, "ods info");
	}
	else if (res[0] == isc_info_ods_version)
	{
		USHORT len = isc_vax_integer (&res[1], 2);
		m_odsNumber = isc_vax_integer (&res[3], len);
	}
}

void NBackup::internal_unlock_database()
{
	if (m_silent)
	{
		ISC_STATUS_ARRAY temp;
		if (!isc_start_transaction(temp, &trans, 1, &newdb, 0, NULL))
		{
			if (isc_dsql_execute_immediate(temp, &newdb, &trans, 0, "ALTER DATABASE END BACKUP", 1, NULL))
				isc_rollback_transaction(temp, &trans);
			else if (isc_commit_transaction(temp, &trans))
				isc_rollback_transaction(temp, &trans);
		}
	}
	else
	{
		if (isc_start_transaction(status, &trans, 1, &newdb, 0, NULL))
			pr_error(status, "start transaction");
		if (isc_dsql_execute_immediate(status, &newdb, &trans, 0, "ALTER DATABASE END BACKUP", 1, NULL))
			pr_error(status, "end backup");
		if (isc_commit_transaction(status, &trans))
			pr_error(status, "end backup: commit");
	}
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
		m_silent = true;
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
		m_silent = true;
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
	ULONG page_writes = 0, page_reads = 0;

	time_t start = time(NULL);
	struct tm today;
#ifdef HAVE_LOCALTIME_R
	if (!localtime_r(&start, &today))
	{
		// What to do here?
	}
#else
	{ //scope
		struct tm* times = localtime(&start);
		if (!times)
		{
			// What do to here?
		}
		today = *times;
	} //
#endif

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
			sprintf(str, "SELECT RDB$GUID, RDB$SCN FROM RDB$BACKUP_HISTORY "
				"WHERE RDB$BACKUP_ID = "
				  "(SELECT MAX(RDB$BACKUP_ID) FROM RDB$BACKUP_HISTORY "
				   "WHERE RDB$BACKUP_LEVEL = %d)", level - 1);
			if (isc_dsql_prepare(status, &trans, &stmt, 0, str, 1, NULL))
				pr_error(status, "prepare history query");
			if (isc_dsql_describe(status, &stmt, 1, out_sqlda))
				pr_error(status, "describe history query");
			short guid_null, scn_null;
			out_sqlda->sqlvar[0].sqlind = &guid_null;
			out_sqlda->sqlvar[0].sqldata = prev_guid;
			out_sqlda->sqlvar[1].sqlind = &scn_null;
			out_sqlda->sqlvar[1].sqldata = (char*) &prev_scn;
			if (isc_dsql_execute(status, &trans, &stmt, 1, NULL))
				pr_error(status, "execute history query");

			switch (isc_dsql_fetch(status, &stmt, 1, out_sqlda))
			{
			case 100: // No more records available
				status_exception::raise(Arg::Gds(isc_nbackup_lostrec_db) << database.c_str() <<
										Arg::Num(level - 1));
			case 0:
				if (guid_null || scn_null)
					status_exception::raise(Arg::Gds(isc_nbackup_lostguid_db));
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

		if (fname.hasData())
			bakname = fname;
		else
		{
			// Let's generate nice new filename
			PathName begin, fil;
			PathUtils::splitLastComponent(begin, fil, database);
			bakname.printf("%s-%d-%04d%02d%02d-%02d%02d.nbk", fil.c_str(), level,
				today.tm_year + 1900, today.tm_mon + 1, today.tm_mday,
				today.tm_hour, today.tm_min);
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

		Ods::header_page *header = reinterpret_cast<Ods::header_page*>(
			FB_ALIGN((IPTR) unaligned_header_buffer, SECTOR_ALIGNMENT));

		if (read_file(dbase, header, SECTOR_ALIGNMENT/*sizeof(*header)*/) != SECTOR_ALIGNMENT/*sizeof(*header)*/)
			status_exception::raise(Arg::Gds(isc_nbackup_err_eofhdrdb) << dbname.c_str() << Arg::Num(1));

		if (!Ods::isSupported(header->hdr_ods_version, header->hdr_ods_minor))
		{
			const USHORT ods_version = header->hdr_ods_version & ~ODS_FIREBIRD_FLAG;
			status_exception::raise(Arg::Gds(isc_wrong_ods) << Arg::Str(database.c_str()) <<
									Arg::Num(ods_version) <<
									Arg::Num(header->hdr_ods_minor) <<
									Arg::Num(ODS_VERSION) <<
									Arg::Num(ODS_CURRENT));
		}

		if ((header->hdr_flags & Ods::hdr_backup_mask) != Jrd::nbak_state_stalled)
			status_exception::raise(Arg::Gds(isc_nbackup_db_notlock) << Arg::Num(header->hdr_flags));

		Array<UCHAR> unaligned_page_buffer;
		{ // scope
			UCHAR* buf = unaligned_page_buffer.getBuffer(header->hdr_page_size + SECTOR_ALIGNMENT);
			page_buff = reinterpret_cast<Ods::pag*>(FB_ALIGN((IPTR) buf, SECTOR_ALIGNMENT));
		} // end scope

		ULONG db_size = db_size_pages;
		seek_file(dbase, 0);

		if (read_file(dbase, page_buff, header->hdr_page_size) != header->hdr_page_size)
			status_exception::raise(Arg::Gds(isc_nbackup_err_eofhdrdb) << dbname.c_str() << Arg::Num(2));
		--db_size;
		page_reads++;

		Guid backup_guid;
		bool guid_found = false;
		const UCHAR* p = reinterpret_cast<Ods::header_page*>(page_buff)->hdr_data;
		while (true)
		{
			switch (*p)
			{
			case Ods::HDR_backup_guid:
				if (p[1] != sizeof(Guid))
					break;
				memcpy(&backup_guid, p + 2, sizeof(Guid));
				guid_found = true;
				break;
			case Ods::HDR_difference_file:
				p += p[1] + 2;
				continue;
			}
			break;
		}

		if (!guid_found)
			status_exception::raise(Arg::Gds(isc_nbackup_lostguid_bk));

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
		ULONG lastPage = FIRST_PIP_PAGE;
		const ULONG pagesPerPIP = Ods::pagesPerPIP(header->hdr_page_size);

		ULONG scnsSlot = 0;
		const ULONG pagesPerSCN = Ods::pagesPerSCN(header->hdr_page_size);

		Array<UCHAR> unaligned_scns_buffer;
		Ods::scns_page* scns = NULL, *scns_buf = NULL;
		{ // scope
			UCHAR* buf = unaligned_scns_buffer.getBuffer(header->hdr_page_size + SECTOR_ALIGNMENT);
			scns_buf = reinterpret_cast<Ods::scns_page*>(FB_ALIGN((IPTR) buf, SECTOR_ALIGNMENT));
		}

		while (true)
		{
			if (curPage && page_buff->pag_scn > backup_scn)
			{
				status_exception::raise(Arg::Gds(isc_nbackup_page_changed) << Arg::Num(curPage) <<
										Arg::Num(page_buff->pag_scn) << Arg::Num(backup_scn));
			}
			if (level)
			{
				if (page_buff->pag_scn > prev_scn)
				{
					write_file(backup, &curPage, sizeof(curPage));
					write_file(backup, page_buff, header->hdr_page_size);

					page_writes++;
				}
			}
			else
			{
				write_file(backup, page_buff, header->hdr_page_size);
				page_writes++;
			}

			checkCtrlC(uSvc);

			if ((db_size_pages != 0) && (db_size == 0))
				break;

			if (level)
			{
				fb_assert(scnsSlot < pagesPerSCN);
				fb_assert(scns && scns->scn_sequence * pagesPerSCN + scnsSlot == curPage ||
						 !scns && curPage % pagesPerSCN == scnsSlot);

				ULONG nextSCN = scns ? (scns->scn_sequence + 1) * pagesPerSCN : FIRST_SCN_PAGE;

				while (true)
				{
					curPage++;
					scnsSlot++;
					if (!scns || scns->scn_pages[scnsSlot] > prev_scn ||
						scnsSlot == pagesPerSCN ||
						curPage == nextSCN ||
						curPage == lastPage)
					{
						seek_file(dbase, (SINT64) curPage * header->hdr_page_size);
						break;
					}
				}

				if (scnsSlot == pagesPerSCN)
				{
					scnsSlot = 0;
					scns = NULL;
				}

				fb_assert(scnsSlot < pagesPerSCN);
				fb_assert(scns && scns->scn_sequence * pagesPerSCN + scnsSlot == curPage ||
						 !scns && curPage % pagesPerSCN == scnsSlot);
			}
			else
				curPage++;


			const size_t bytesDone = read_file(dbase, page_buff, header->hdr_page_size);
			--db_size;
			page_reads++;
			if (bytesDone == 0)
				break;
			if (bytesDone != header->hdr_page_size)
				status_exception::raise(Arg::Gds(isc_nbackup_dbsize_inconsistent));

			if (level && page_buff->pag_type == pag_scns)
			{
				fb_assert(scnsSlot == 0 || scnsSlot == FIRST_SCN_PAGE);

				// pick up next SCN's page
				memcpy(scns_buf, page_buff, header->hdr_page_size);
				scns = scns_buf;
			}


			if (curPage == lastPage)
			{
				// Starting from ODS 11.1 we can expand file but never use some last
				// pages in it. There are no need to backup this empty pages. More,
				// we can't be sure its not used pages have right SCN assigned.
				// How many pages are really used we know from page_inv_page::pip_used
				// where stored number of pages allocated from this pointer page.
				if (page_buff->pag_type == pag_pages)
				{
					Ods::page_inv_page* pip = (Ods::page_inv_page*) page_buff;
					if (lastPage == FIRST_PIP_PAGE)
						lastPage = pip->pip_used - 1;
					else
						lastPage += pip->pip_used;

					if (pip->pip_used < pagesPerPIP)
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

		const char* insHistory;
		get_ods();
		if (m_odsNumber >= ODS_VERSION12)
		{
			insHistory =
				"INSERT INTO RDB$BACKUP_HISTORY(RDB$BACKUP_ID, RDB$TIMESTAMP, "
					"RDB$BACKUP_LEVEL, RDB$GUID, RDB$SCN, RDB$FILE_NAME) "
				"VALUES(NULL, 'NOW', ?, ?, ?, ?)";
		}
		else
		{
			insHistory =
				"INSERT INTO RDB$BACKUP_HISTORY(RDB$BACKUP_ID, RDB$TIMESTAMP, "
					"RDB$BACKUP_LEVEL, RDB$GUID, RDB$SCN, RDB$FILE_NAME) "
				"VALUES(GEN_ID(RDB$BACKUP_HISTORY, 1), 'NOW', ?, ?, ?, ?)";
		}

		if (isc_dsql_prepare(status, &trans, &stmt, 0, insHistory, 1, NULL))
		{
			pr_error(status, "prepare history insert");
		}
		if (isc_dsql_describe_bind(status, &stmt, 1, in_sqlda))
			pr_error(status, "bind history insert");
		short null_flag = 0;
		in_sqlda->sqlvar[0].sqldata = (char*) &level;
		in_sqlda->sqlvar[0].sqlind = &null_flag;
		char temp[GUID_BUFF_SIZE];
		GuidToString(temp, &backup_guid);
		in_sqlda->sqlvar[1].sqldata = temp;
		in_sqlda->sqlvar[1].sqlind = &null_flag;
		in_sqlda->sqlvar[2].sqldata = (char*) &backup_scn;
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
		m_silent = true;
		if (delete_backup)
			remove(bakname.c_str());
		if (trans)
		{
			// Do not report a secondary exception
			//if (isc_rollback_transaction(status, &trans))
			//	pr_error(status, "rollback transaction");
			ISC_STATUS_ARRAY temp;
			isc_rollback_transaction(temp, &trans);
		}
		if (database_locked)
		{
			if (!newdb)
				attach_database();
			if (newdb)
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

	time_t finish = time(NULL);
	double elapsed = difftime(finish, start);
	uSvc->printf(false, "time elapsed\t%.0f sec \npage reads\t%u \npage writes\t%u\n",
		elapsed, page_reads, page_writes);
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
		Guid prev_guid;
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
						//Enter name of the backup file of level %d (\".\" - do not restore further):\n
						printMsg(69, SafeArg() << curLevel);
						char temp[256];
						FB_UNUSED(scanf("%255s", temp));
						bakname = temp;
					}
					if (bakname == ".")
					{
						close_database();
						if (!curLevel)
						{
							remove(dbname.c_str());
							status_exception::raise(Arg::Gds(isc_nbackup_failed_lzbk));
						}
						fixup_database();
						delete[] page_buffer;
						return;
					}
					// Never reaches this point when run as service
					try {
						fb_assert(!uSvc->isService());
#ifdef WIN_NT
						if (curLevel)
#endif
							open_backup_scan();
						break;
					}
					catch (const status_exception& e)
					{
						const ISC_STATUS* s = e.value();
						isc_print_status(s);
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
					status_exception::raise(Arg::Gds(isc_nbackup_err_eofhdrbk) << bakname.c_str());
				if (memcmp(bakheader.signature, backup_signature, sizeof(backup_signature)) != 0)
					status_exception::raise(Arg::Gds(isc_nbackup_invalid_incbk) << bakname.c_str());
				if (bakheader.version != 1)
				{
					status_exception::raise(Arg::Gds(isc_nbackup_unsupvers_incbk) <<
										Arg::Num(bakheader.version) << bakname.c_str());
				}
				if (bakheader.level != curLevel)
				{
					status_exception::raise(Arg::Gds(isc_nbackup_invlevel_incbk) <<
						Arg::Num(bakheader.level) << bakname.c_str() << Arg::Num(curLevel));
				}
				// We may also add SCN check, but GUID check covers this case too
				if (memcmp(&bakheader.prev_guid, &prev_guid, sizeof(Guid)) != 0)
					status_exception::raise(Arg::Gds(isc_nbackup_wrong_orderbk) << bakname.c_str());

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
						status_exception::raise(Arg::Gds(isc_nbackup_err_eofbk) << bakname.c_str());
					}
					seek_file(dbase, ((SINT64) pageNum) * bakheader.page_size);
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
					status_exception::raise(Arg::Gds(isc_nbackup_err_copy) <<
						dbname.c_str() << bakname.c_str() << Arg::OsError());
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
					status_exception::raise(Arg::Gds(isc_nbackup_err_eofhdr_restdb) << Arg::Num(1));
				page_buffer = FB_NEW(*getDefaultMemoryPool()) UCHAR[header.hdr_page_size];

				seek_file(dbase, 0);

				if (read_file(dbase, page_buffer, header.hdr_page_size) != header.hdr_page_size)
					status_exception::raise(Arg::Gds(isc_nbackup_err_eofhdr_restdb) << Arg::Num(2));

				bool guid_found = false;
				const UCHAR* p = reinterpret_cast<Ods::header_page*>(page_buffer)->hdr_data;
				while (true)
				{
					switch (*p)
					{
					case Ods::HDR_backup_guid:
						if (p[1] != sizeof(Guid))
							break;
						memcpy(&prev_guid, p + 2, sizeof(Guid));
						guid_found = true;
						break;
					case Ods::HDR_difference_file:
						p += p[1] + 2;
						continue;
					}
					break;
				}
				if (!guid_found)
					status_exception::raise(Arg::Gds(isc_nbackup_lostguid_l0bk));
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
		m_silent = true;
		delete[] page_buffer;
		if (delete_database)
			remove(dbname.c_str());
		throw;
	}
}

int NBACKUP_main(UtilSvc* uSvc)
{
	int exit_code = FB_SUCCESS;

	try {
		nbackup(uSvc);
	}
	catch (const status_exception& e)
	{
		if (!uSvc->isService())
		{
			const ISC_STATUS* s = e.value();
			isc_print_status(s);
		}
 		ISC_STATUS_ARRAY status;
 		e.stuff_exception(status);
 		uSvc->initStatus();
 		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}
	catch (const Exception& e)
	{
		if (!uSvc->isService())
			printf("%s\n", e.what());

 		ISC_STATUS_ARRAY status;
 		e.stuff_exception(status);
 		uSvc->initStatus();
 		uSvc->setServiceStatus(status);
		exit_code = FB_FAILURE;
	}

	return exit_code;
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
//	string trustedUser;
//	bool trustedRole = false;
	string onOff;

	const Switches switches(nbackup_action_in_sw_table, FB_NELEM(nbackup_action_in_sw_table),
							false, true);

	// Read global command line parameters
	for (int itr = 1; itr < argc; ++itr)
	{
		// We must recognize all parameters here
		if (argv[itr][0] != '-') {
			usage(uSvc, isc_nbackup_unknown_param, argv[itr]);
		}

		const Switches::in_sw_tab_t* rc = switches.findSwitch(argv[itr]);
		if (!rc)
		{
			usage(uSvc, isc_nbackup_unknown_switch, argv[itr]);
			break;
		}

		switch (rc->in_sw)
		{
		/***
		case IN_SW_NBK_TRUSTED_USER:
			uSvc->checkService();
			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			trustedUser = argv[itr];
			break;

		case IN_SW_NBK_TRUSTED_ROLE:
			uSvc->checkService();
			trustedRole = true;
			break;
		***/
		case IN_SW_NBK_USER_NAME:
			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			username = argv[itr];
			break;

		case IN_SW_NBK_PASSWORD:
			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			password = argv[itr];
			uSvc->hidePasswd(argv, itr);
			break;

		case IN_SW_NBK_NODBTRIG:
			run_db_triggers = false;
			break;

		case IN_SW_NBK_DIRECT:
 			if (++itr >= argc)
 				missingParameterForSwitch(uSvc, argv[itr - 1]);

 			onOff = argv[itr];
 			onOff.upper();
 			if (onOff == "ON")
 				direct_io = true;
 			else if (onOff == "OFF")
 				direct_io = false;
 			else
 				usage(uSvc, isc_nbackup_switchd_parameter, onOff.c_str());
			break;

		case IN_SW_NBK_FIXUP:
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbFixup;
			break;

		case IN_SW_NBK_FETCH:
			if (uSvc->isService())
				usage(uSvc, isc_nbackup_nofetchpw_svc);
			else
			{
				if (++itr >= argc)
					missingParameterForSwitch(uSvc, argv[itr - 1]);

				const char* passwd = NULL;
				if (fb_utils::fetchPassword(argv[itr], passwd) != fb_utils::FETCH_PASS_OK)
				{
					usage(uSvc, isc_nbackup_pwfile_error, argv[itr]);
					break;
				}
				password = passwd;
			}
			break;


		case IN_SW_NBK_LOCK:
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbLock;
			break;

		case IN_SW_NBK_UNLOCK:
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			database = argv[itr];
			op = nbUnlock;
			break;

		case IN_SW_NBK_BACKUP:
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			level = atoi(argv[itr]);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 2]);

			database = argv[itr];

			if (itr + 1 < argc)
				filename = argv[++itr];

			op = nbBackup;
			break;

		case IN_SW_NBK_RESTORE:
			if (op != nbNone)
				singleAction(uSvc);

			if (++itr >= argc)
				missingParameterForSwitch(uSvc, argv[itr - 1]);

			database = argv[itr];
			while (++itr < argc)
				backup_files.push(argv[itr]);

			op = nbRestore;
			break;

		case IN_SW_NBK_SIZE:
			print_size = true;
			break;

		case IN_SW_NBK_HELP:
			if (uSvc->isService())
				usage(uSvc, isc_nbackup_unknown_switch, argv[itr]);
			else
				usage(uSvc, 0);
			break;

		case IN_SW_NBK_VERSION:
			if (uSvc->isService())
				usage(uSvc, isc_nbackup_unknown_switch, argv[itr]);
			else
				version = true;
			break;

		default:
			usage(uSvc, isc_nbackup_unknown_switch, argv[itr]);
			break;
		}
	}

	if (version)
	{
		printMsg(68, SafeArg() << FB_VERSION);
	}

	if (op == nbNone)
	{
		if (!version)
		{
			usage(uSvc, isc_nbackup_no_switch);
		}
		exit(FINI_OK);
	}

	if (print_size && op != nbLock)
	{
		usage(uSvc, isc_nbackup_size_with_lock);
	}

	NBackup nbk(uSvc, database, username, password, run_db_triggers, /*trustedUser, trustedRole, */direct_io);
	try
	{
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
	catch (const Exception& e)
	{
		if (!uSvc->isService() && !nbk.printed())
		{
			ISC_STATUS_ARRAY status;
			e.stuff_exception(status);
			isc_print_status(status);
		}

		throw;
	}
}
