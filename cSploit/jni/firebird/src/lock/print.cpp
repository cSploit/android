/*
 *      PROGRAM:        JRD Lock Manager
 *      MODULE:         print.cpp
 *      DESCRIPTION:    Lock Table printer
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
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DELTA" port
 * 2002.10.27 Sean Leyne - Code Cleanup, removed obsolete "Ultrix" port
 *
 * 2002.10.28 Sean Leyne - Code cleanup, removed obsolete "SGI" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
 * 2008.04.04 Roman Simakov - Added html output support
 *
 */

#include "firebird.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../common/file_params.h"
#include "../jrd/jrd.h"
#include "../jrd/lck.h"
#include "../common/gdsassert.h"
#include "../common/db_alias.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/isc_s_proto.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <sys/stat.h>

#ifdef HAVE_SYS_PARAM_H
#  include <sys/param.h>
#endif

#ifdef HAVE_IO_H
#include <io.h>
#endif

#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifndef O_BINARY
#define O_BINARY 0
#endif

#ifndef FPRINTF
#define FPRINTF         fprintf
#endif

typedef FILE* OUTFILE;

const USHORT SW_I_ACQUIRE	= 1;
const USHORT SW_I_OPERATION	= 2;
const USHORT SW_I_TYPE		= 4;
const USHORT SW_I_WAIT		= 8;

#define SRQ_BASE                    ((UCHAR*) LOCK_header)

struct waitque
{
	USHORT waitque_depth;
	SRQ_PTR waitque_entry[30];
};

using namespace Firebird;

namespace
{
	class sh_mem FB_FINAL : public Firebird::IpcObject
	{
	public:
		explicit sh_mem(bool p_consistency, const char* filename)
		  :	sh_mem_consistency(p_consistency),
			shared_memory(FB_NEW(*getDefaultMemoryPool()) Firebird::SharedMemory<lhb>(filename, 0, this))
		{ }

		bool initialize(Firebird::SharedMemoryBase*, bool)
		{
			// Initialize a lock table to looking -- i.e. don't do nuthin.
			return sh_mem_consistency;
		}

		void mutexBug(int /*osErrorCode*/, const char* /*text*/)
		{
			// Do nothing - lock print always ignored mutex errors
		}

	private:
		bool sh_mem_consistency;

	public:
		Firebird::AutoPtr<Firebird::SharedMemory<lhb> > shared_memory;
	};
}

static void prt_lock_activity(OUTFILE, const lhb*, USHORT, ULONG, ULONG);
static void prt_history(OUTFILE, const lhb*, SRQ_PTR, const SCHAR*);
static void prt_lock(OUTFILE, const lhb*, const lbl*, USHORT);
static void prt_owner(OUTFILE, const lhb*, const own*, bool, bool, bool);
static void prt_owner_wait_cycle(OUTFILE, const lhb*, const own*, USHORT, waitque*);
static void prt_request(OUTFILE, const lhb*, const lrq*);
static void prt_que(OUTFILE, const lhb*, const SCHAR*, const srq*, USHORT, const TEXT* prefix = NULL);
static void prt_que2(OUTFILE, const lhb*, const SCHAR*, const srq*, USHORT, const TEXT* prefix = NULL);

// HTML print functions
bool sw_html_format = false;

static void prt_html_begin(OUTFILE);
static void prt_html_end(OUTFILE);

static const TEXT preOwn[] = "own";
static const TEXT preRequest[] = "request";
static const TEXT preLock[] = "lock";


class HtmlLink
{
public:
	HtmlLink(const TEXT* prefix, const SLONG value)
	{
		if (sw_html_format && value && prefix)
			sprintf(strBuffer, "<a href=\"#%s%"SLONGFORMAT"\">%6"SLONGFORMAT"</a>", prefix, value, value);
		else
			sprintf(strBuffer, "%6"SLONGFORMAT, value);
	}
	operator const TEXT*()
	{
		return strBuffer;
	}
private:
	TEXT strBuffer[256];
	HtmlLink(const HtmlLink&) {}
};


static const TEXT history_names[][10] =
{
	"n/a", "ENQ", "DEQ", "CONVERT", "SIGNAL", "POST", "WAIT",
	"DEL_PROC", "DEL_LOCK", "DEL_REQ", "DENY", "GRANT", "LEAVE",
	"SCAN", "DEAD", "ENTER", "BUG", "ACTIVE", "CLEANUP", "DEL_OWNER"
};


static const char* usage =
	"Firebird lock print utility.\n"
	"Usage: fb_lock_print (-d | -f) [<parameters>]\n"
	"\n"
	"One of -d or -f switches is mandatory:\n"
	"  -d  <database file name>      specify database which lock table to be printed\n"
	"  -f  <lock table file name>    specify lock table file itself to be printed\n"
	"\n"
	"Optional parameters are:\n"
	"  -o        print list of lock owners\n"
	"  -p        same as -o\n"
	"  -l        print locks\n"
	"  -r        print requests made by lock owners (valid only if -o specified)\n"
	"  -h        print recent events history\n"
	"  -a        print all of the above (equal to -o -l -r -h swithes)\n"
	"  -s <N>    print only locks of given series (valid only if -l specified)\n"
	"  -n        print only pending owners (if -o specified) or\n"
	"            pending locks (if -l specified)\n"
	"  -w        print \"waiting for\" list for every owner\n"
	"            (valid only if -o specified)\n"
	"  -c        acquire lock manager's mutex to print consistent view of\n"
	"            lock table (valid only if -d specified)\n"
	"  -m        make output in html format\n"
	"\n"
	"  -i[<counters>] [<N> [<M>]]    interactive mode:\n"
	"     print chosen lock manager activity counters during <N> seconds\n"
	"     with interval of <M> seconds. Defaults are 1 sec for both values.\n"
	"     Counters are:\n"
	"     a    number of mutex acquires, acquire blocks, etc\n"
	"     o    number of lock operations (enqueues, converts, downgrades, etc)\n"
	"     t    number of operations with most important lock series\n"
	"     w    number of waits, timeouts, deadlock scans, etc\n"
	"     Default is aotw\n"
	"\n"
	"  -?        this help screen\n"
	"\n";


// The same table is in lock.cpp, maybe worth moving to a common file?
static const UCHAR compatibility[LCK_max][LCK_max] =
{

/*							Shared	Prot	Shared	Prot
			none	null	Read	Read	Write	Write	Exclusive */

/* none */	{true,	true,	true,	true,	true,	true,	true},
/* null */	{true,	true,	true,	true,	true,	true,	true},
/* SR */	{true,	true,	true,	true,	true,	true,	false},
/* PR */	{true,	true,	true,	true,	false,	false,	false},
/* SW */	{true,	true,	true,	false,	true,	false,	false},
/* PW */	{true,	true,	true,	false,	false,	false,	false},
/* EX */	{true,	true,	false,	false,	false,	false,	false}
};

//#define COMPATIBLE(st1, st2)	compatibility [st1 * LCK_max + st2]

int CLIB_ROUTINE main( int argc, char *argv[])
{
/**************************************
 *
 *      m a i n
 *
 **************************************
 *
 * Functional description
 *	Check switches passed in and prepare to dump the lock table
 *	to stdout.
 *
 **************************************/
	OUTFILE outfile = stdout;

	// Perform some special handling when run as a Firebird service.  The
	// first switch can be "-svc" (lower case!) or it can be "-svc_re" followed
	// by 3 file descriptors to use in re-directing stdin, stdout, and stderr.

	if (argc > 1 && !strcmp(argv[1], "-svc"))
	{
		argv++;
		argc--;
	}
	else if (argc > 4 && !strcmp(argv[1], "-svc_re"))
	{
		long redir_in = atol(argv[2]);
		long redir_out = atol(argv[3]);
		long redir_err = atol(argv[4]);
#ifdef WIN_NT
		redir_in = _open_osfhandle(redir_in, 0);
		redir_out = _open_osfhandle(redir_out, 0);
		redir_err = _open_osfhandle(redir_err, 0);
#endif
		if (redir_in != 0)
		{
			if (dup2((int) redir_in, 0))
				close((int) redir_in);
		}
		if (redir_out != 1)
		{
			if (dup2((int) redir_out, 1))
				close((int) redir_out);
		}
		if (redir_err != 2)
		{
			if (dup2((int) redir_err, 2))
				close((int) redir_err);
		}
		argv += 4;
		argc -= 4;
	}
	//const int orig_argc = argc;
	//SCHAR** orig_argv = argv;

	// Handle switches, etc.

	argv++;
	bool sw_consistency = false;
	bool sw_waitlist = false;
	bool sw_requests = false;
	bool sw_locks = false;
	bool sw_history = false;
	bool sw_owners = false;
	bool sw_pending = false;

	USHORT sw_interactive;
	// Those variables should be signed to accept negative values from atoi
	SSHORT sw_series;
	SLONG sw_intervals;
	SLONG sw_seconds;
	sw_series = sw_interactive = sw_intervals = sw_seconds = 0;
	const TEXT* lock_file = NULL;
	const TEXT* db_file = NULL;

	while (--argc)
	{
		SCHAR* p = *argv++;
		if (*p++ != '-')
		{
			FPRINTF(outfile, "%s", usage);
			exit(FINI_OK);
		}
		SCHAR c;
		while (c = *p++)
			switch (c)
			{
			case '?':
				FPRINTF(outfile, "%s", usage);
				exit(FINI_OK);
				break;

			case 'o':
			case 'p':
				sw_owners = true;
				break;

			case 'c':
				sw_consistency = true;
				break;

			case 'l':
				sw_locks = true;
				break;

			case 'r':
				sw_requests = true;
				break;

			case 'a':
				sw_locks = true;
				sw_owners = true;
				sw_requests = true;
				sw_history = true;
				break;

			case 'h':
				sw_history = true;
				break;

			case 's':
				if (argc > 1)
					sw_series = atoi(*argv++);
				if (sw_series <= 0)
				{
					FPRINTF(outfile, "Please specify a positive value following option -s\n");
					exit(FINI_OK);
				}
				--argc;
				break;

			case 'i':
				while (c = *p++)
					switch (c)
					{
					case 'a':
						sw_interactive |= SW_I_ACQUIRE;
						break;

					case 'o':
						sw_interactive |= SW_I_OPERATION;
						break;

					case 't':
						sw_interactive |= SW_I_TYPE;
						break;

					case 'w':
						sw_interactive |= SW_I_WAIT;
						break;

					default:
						FPRINTF(outfile, "Valid interactive switches are: a, o, t, w\n");
						exit(FINI_OK);
						break;
					}
				if (!sw_interactive)
					sw_interactive = (SW_I_ACQUIRE | SW_I_OPERATION | SW_I_TYPE | SW_I_WAIT);
				sw_seconds = sw_intervals = 1;
				if (argc > 1)
				{
					sw_seconds = atoi(*argv++);
					--argc;
					if (argc > 1)
					{
						sw_intervals = atoi(*argv++);
						--argc;
					}
					if (sw_seconds <= 0 || sw_intervals < 0)
					{
						FPRINTF(outfile, "Please specify 2 positive values for option -i\n");
						exit(FINI_OK);
					}
				}
				--p;
				break;

			case 'w':
				sw_waitlist = true;
				break;

			case 'f':
				if (argc > 1)
				{
					lock_file = *argv++;
					--argc;
				}
				else
				{
					FPRINTF(outfile, "Usage: -f <filename>\n");
					exit(FINI_OK);
				}
				break;

			case 'd':
				if (argc > 1)
				{
					db_file = *argv++;
					--argc;
				}
				else
				{
					FPRINTF(outfile, "Usage: -d <filename>\n");
					exit(FINI_OK);
				}
				break;

			case 'm':
				sw_html_format = true;
				break;

			case 'n':
				sw_pending = true;
				break;

			default:
				FPRINTF(outfile, "%s", usage);
				exit(FINI_OK);
				break;
			}
	}

	Firebird::PathName filename;

	if (db_file && lock_file)
	{
		FPRINTF(outfile, "Switches -d and -f cannot be specified together\n");
		exit(FINI_OK);
	}
	else if (db_file)
	{
		Firebird::PathName org_name = db_file;
		Firebird::PathName db_name;
		expandDatabaseName(org_name, db_name, NULL);

		// Below code mirrors the one in JRD (PIO modules and Database class).
		// Maybe it's worth putting it into common, if no better solution is found.
#ifdef WIN_NT
		const HANDLE h = CreateFile(db_name.c_str(), GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
									NULL, OPEN_EXISTING, 0, 0);
		if (h == INVALID_HANDLE_VALUE)
		{
			FPRINTF(outfile, "Unable to open the database file (%d).\n", GetLastError());
			exit(FINI_OK);
		}
		BY_HANDLE_FILE_INFORMATION file_info;
		GetFileInformationByHandle(h, &file_info);
		const size_t len1 = sizeof(file_info.dwVolumeSerialNumber);
		const size_t len2 = sizeof(file_info.nFileIndexHigh);
		const size_t len3 = sizeof(file_info.nFileIndexLow);
		UCHAR buffer[len1 + len2 + len3], *p = buffer;
		memcpy(p, &file_info.dwVolumeSerialNumber, len1);
		p += len1;
		memcpy(p, &file_info.nFileIndexHigh, len2);
		p += len2;
		memcpy(p, &file_info.nFileIndexLow, len3);
		CloseHandle(h);
#else
		struct stat statistics;
		if (stat(db_name.c_str(), &statistics) == -1)
		{
			FPRINTF(outfile, "Unable to open the database file.\n");
			exit(FINI_OK);
		}
		const size_t len1 = sizeof(statistics.st_dev);
		const size_t len2 = sizeof(statistics.st_ino);
		UCHAR buffer[len1 + len2], *p = buffer;
		memcpy(p, &statistics.st_dev, len1);
		p += len1;
		memcpy(p, &statistics.st_ino, len2);
#endif

		Firebird::string file_id;
		for (size_t i = 0; i < sizeof(buffer); i++)
		{
			TEXT hex[3];
			sprintf(hex, "%02x", (int) buffer[i]);
			file_id.append(hex);
		}

		filename.printf(LOCK_FILE, file_id.c_str());
	}
	else if (lock_file)
	{
		filename = lock_file;
	}
	else
	{
		FPRINTF(outfile, "Please specify either -d <database name> or -f <lock file name>\n\n");
		FPRINTF(outfile, "%s", usage);
		exit(FINI_OK);
	}

	Firebird::AutoPtr<UCHAR, Firebird::ArrayDelete<UCHAR> > buffer;
	lhb* LOCK_header = NULL;
	Firebird::AutoPtr<sh_mem> shmem_data;

	if (db_file)
	{
	  try
	  {
		shmem_data.reset(FB_NEW(*getDefaultMemoryPool()) sh_mem(sw_consistency, filename.c_str()));
		LOCK_header = (lhb*) (shmem_data->shared_memory->sh_mem_header);

		// Make sure the lock file is valid - if it's a zero length file we
		// can't look at the header without causing a BUS error by going
		// off the end of the mapped region.

		if (shmem_data->shared_memory->sh_mem_length_mapped < sizeof(lhb))
		{
			// Mapped file is obviously too small to really be a lock file
			FPRINTF(outfile, "Unable to access lock table - file too small.\n");
			exit(FINI_OK);
		}

		if (sw_consistency)
		{
			shmem_data->shared_memory->mutexLock();
		}

#ifdef USE_SHMEM_EXT
		ULONG extentSize = shmem_data->sh_mem_length_mapped;
		ULONG totalSize = LOCK_header->lhb_length;
		ULONG extentsCount = totalSize / extentSize + (totalSize % extentSize == 0 ? 0 : 1);

		try
		{
			buffer = new UCHAR[extentsCount * extentSize];
		}
		catch (const Firebird::BadAlloc&)
		{
			FPRINTF(outfile, "Insufficient memory for lock statistics.\n");
			exit(FINI_OK);
		}

		memcpy((UCHAR*) buffer, LOCK_header, extentSize);

		for (ULONG extent = 1; extent < extentsCount; ++extent)
		{
			Firebird::PathName extName;
			extName.printf("%s.ext%d", filename.c_str(), extent);

			sh_mem extData(false);
			if (! extData.mapFile(statusVector, extName.c_str(), 0))
			{
				FPRINTF(outfile, "Could not map extent number %d, file %s.\n", extent, extName.c_str());
				exit(FINI_OK);
			}

			memcpy(((UCHAR*) buffer) + extent * extentSize, extData.sh_mem_header, extentSize);

			extData.unmapFile(statusVector);
		}

		LOCK_header = (lhb*)(UCHAR*) buffer;
#elif defined HAVE_OBJECT_MAP
		if (LOCK_header->lhb_length > shmem_data->shared_memory->sh_mem_length_mapped)
		{
			const ULONG length = LOCK_header->lhb_length;
			Firebird::Arg::StatusVector statusVector;
			shmem_data->shared_memory->remapFile(statusVector, length, false);
			LOCK_header = (lhb*)(shmem_data->shared_memory->sh_mem_header);
		}
#endif

		if (sw_consistency)
		{
#ifndef USE_SHMEM_EXT
			// To avoid changes in the lock file while we are dumping it - make
			// a local buffer, lock the lock file, copy it, then unlock the
			// lock file to let processing continue.  Printing of the lock file
			// will continue from the in-memory copy.

			try
			{
				buffer = new UCHAR[LOCK_header->lhb_length];
			}
			catch (const Firebird::BadAlloc&)
			{
				FPRINTF(outfile, "Insufficient memory for consistent lock statistics.\n");
				FPRINTF(outfile, "Try omitting the -c switch.\n");
				exit(FINI_OK);
			}

			memcpy((UCHAR*) buffer, LOCK_header, LOCK_header->lhb_length);
			LOCK_header = (lhb*)(UCHAR*) buffer;
#endif

			shmem_data->shared_memory->mutexUnlock();
		}
	  }
	  catch (const Firebird::Exception& ex)
	  {
		FPRINTF(outfile, "Unable to access lock table.\n");

		ISC_STATUS_ARRAY status;
		ex.stuff_exception(status);
		gds__print_status(status);

		exit(FINI_OK);
	  }
	}
	else if (lock_file)
	{
		const int fd = open(filename.c_str(), O_RDONLY | O_BINARY);
		if (fd == -1)
		{
			FPRINTF(outfile, "Unable to open lock file.\n");
			exit(FINI_OK);
		}

		struct stat file_stat;
		if (fstat(fd, &file_stat) == -1)
		{
			close(fd);
			FPRINTF(outfile, "Unable to retrieve lock file size.\n");
			exit(FINI_OK);
		}

		if (!file_stat.st_size)
		{
			close(fd);
			FPRINTF(outfile, "Lock file is empty.\n");
			exit(FINI_OK);
		}

		try
		{
			buffer = new UCHAR[file_stat.st_size];
		}
		catch (const Firebird::BadAlloc&)
		{
			FPRINTF(outfile, "Insufficient memory to read lock file.\n");
			exit(FINI_OK);
		}

		LOCK_header = (lhb*)(UCHAR*) buffer;
		const int bytes_read = read(fd, LOCK_header, file_stat.st_size);
		close(fd);

		if (bytes_read != file_stat.st_size)
		{
			FPRINTF(outfile, "Unable to read lock file.\n");
			exit(FINI_OK);
		}

#ifdef USE_SHMEM_EXT
		ULONG extentSize = file_stat.st_size;
		ULONG totalSize = LOCK_header->lhb_length;
		const ULONG extentsCount = totalSize / extentSize + (totalSize % extentSize == 0 ? 0 : 1);
		UCHAR* newBuf = NULL;

		try
		{
			newBuf = new UCHAR[extentsCount * extentSize];
		}
		catch (const Firebird::BadAlloc&)
		{
			FPRINTF(outfile, "Insufficient memory for lock statistics.\n");
			exit(FINI_OK);
		}

		memcpy(newBuf, LOCK_header, extentSize);
		buffer = newBuf;

		for (ULONG extent = 1; extent < extentsCount; ++extent)
		{
			Firebird::PathName extName;
			extName.printf("%s.ext%d", filename.c_str(), extent);

			const int fd = open(extName.c_str(), O_RDONLY | O_BINARY);
			if (fd == -1)
			{
				FPRINTF(outfile, "Unable to open lock file extent number %d, file %s.\n",
						extent, extName.c_str());
				exit(FINI_OK);
			}

			if (read(fd, ((UCHAR*) buffer) + extent * extentSize, extentSize) != extentSize)
			{
				FPRINTF(outfile, "Could not read lock file extent number %d, file %s.\n",
						extent, extName.c_str());
				exit(FINI_OK);
			}
			close(fd);
		}

		LOCK_header = (lhb*)(UCHAR*) buffer;
#endif
	}
	else
	{
		fb_assert(false);
	}

	fb_assert(LOCK_header);

	// if we can't read this version - admit there's nothing to say and return.

	if (LOCK_header->mhb_version != LHB_VERSION)
	{
		if (LOCK_header->mhb_type == 0 && LOCK_header->mhb_version == 0)
		{
			FPRINTF(outfile, "\tLock table is empty.\n");
		}
		else
		{
			FPRINTF(outfile, "\tUnable to read lock table version %d.\n",
				LOCK_header->mhb_version);
		}
		exit(FINI_OK);
	}

	// Print lock activity report

	if (sw_interactive)
	{
		sw_html_format = false;
		prt_lock_activity(outfile, LOCK_header, sw_interactive,
						  (ULONG) sw_seconds, (ULONG) sw_intervals);
		exit(FINI_OK);
	}

	// Print lock header block
	prt_html_begin(outfile);

	struct tm times;
	TimeStamp(LOCK_header->mhb_timestamp).decode(&times);

	FPRINTF(outfile, "LOCK_HEADER BLOCK\n");

	FPRINTF(outfile,
			"\tVersion: %d, Creation timestamp: %04d-%02d-%02d %02d:%02d:%02d\n",
			LOCK_header->mhb_version,
			times.tm_year + 1900, times.tm_mon + 1, times.tm_mday,
			times.tm_hour, times.tm_min, times.tm_sec);

	FPRINTF(outfile,
			"\tActive owner: %s, Length: %6"SLONGFORMAT", Used: %6"SLONGFORMAT"\n",
			(const TEXT*)HtmlLink(preOwn, LOCK_header->lhb_active_owner),
			LOCK_header->lhb_length, LOCK_header->lhb_used);

	FPRINTF(outfile,
			"\tEnqs: %6"UQUADFORMAT", Converts: %6"UQUADFORMAT
			", Rejects: %6"UQUADFORMAT", Blocks: %6"UQUADFORMAT"\n",
			LOCK_header->lhb_enqs, LOCK_header->lhb_converts,
			LOCK_header->lhb_denies, LOCK_header->lhb_blocks);

	FPRINTF(outfile,
			"\tDeadlock scans: %6"UQUADFORMAT", Deadlocks: %6"UQUADFORMAT
			", Scan interval: %3"ULONGFORMAT"\n",
			LOCK_header->lhb_scans, LOCK_header->lhb_deadlocks,
			LOCK_header->lhb_scan_interval);

	FPRINTF(outfile,
			"\tAcquires: %6"UQUADFORMAT", Acquire blocks: %6"UQUADFORMAT
			", Spin count: %3"ULONGFORMAT"\n",
			LOCK_header->lhb_acquires, LOCK_header->lhb_acquire_blocks,
			LOCK_header->lhb_acquire_spins);

	if (LOCK_header->lhb_acquire_blocks)
	{
		const float bottleneck =
			(float) ((100. * LOCK_header->lhb_acquire_blocks) / LOCK_header->lhb_acquires);
		FPRINTF(outfile, "\tMutex wait: %3.1f%%\n", bottleneck);
	}
	else
		FPRINTF(outfile, "\tMutex wait: 0.0%%\n");

	SLONG hash_total_count = 0;
	SLONG hash_max_count = 0;
	SLONG hash_min_count = 10000000;
	USHORT i = 0;
	for (const srq* slot = LOCK_header->lhb_hash; i < LOCK_header->lhb_hash_slots; slot++, i++)
	{
		SLONG hash_lock_count = 0;
		for (const srq* que_inst = (SRQ) SRQ_ABS_PTR(slot->srq_forward); que_inst != slot;
			 que_inst = (SRQ) SRQ_ABS_PTR(que_inst->srq_forward))
		{
			++hash_total_count;
			++hash_lock_count;
		}
		if (hash_lock_count < hash_min_count)
			hash_min_count = hash_lock_count;
		if (hash_lock_count > hash_max_count)
			hash_max_count = hash_lock_count;
	}

	FPRINTF(outfile, "\tHash slots: %4d, ", LOCK_header->lhb_hash_slots);

	FPRINTF(outfile, "Hash lengths (min/avg/max): %4"SLONGFORMAT"/%4"SLONGFORMAT"/%4"SLONGFORMAT"\n",
			hash_min_count, (hash_total_count / LOCK_header->lhb_hash_slots),
			hash_max_count);

	const shb* a_shb = (shb*) SRQ_ABS_PTR(LOCK_header->lhb_secondary);
	FPRINTF(outfile,
			"\tRemove node: %6"SLONGFORMAT", Insert queue: %6"SLONGFORMAT
			", Insert prior: %6"SLONGFORMAT"\n",
			a_shb->shb_remove_node, a_shb->shb_insert_que,
			a_shb->shb_insert_prior);

	prt_que(outfile, LOCK_header, "\tOwners", &LOCK_header->lhb_owners,
			OFFSET(own*, own_lhb_owners), preOwn);
	prt_que(outfile, LOCK_header, "\tFree owners",
			&LOCK_header->lhb_free_owners, OFFSET(own*, own_lhb_owners));
	prt_que(outfile, LOCK_header, "\tFree locks",
			&LOCK_header->lhb_free_locks, OFFSET(lbl*, lbl_lhb_hash));
	prt_que(outfile, LOCK_header, "\tFree requests",
			&LOCK_header->lhb_free_requests, OFFSET(lrq*, lrq_lbl_requests));

	FPRINTF(outfile, "\n");

	// Print known owners

	if (sw_owners)
	{
		const srq* que_inst;
		SRQ_LOOP(LOCK_header->lhb_owners, que_inst)
		{
			const own* owner = (own*) ((UCHAR*) que_inst - OFFSET(own*, own_lhb_owners));
			if (!sw_pending || !SRQ_EMPTY(owner->own_pending))
				prt_owner(outfile, LOCK_header, owner, sw_requests, sw_waitlist, sw_pending);
		}
	}

	// Print known locks

	if (sw_locks || sw_series)
	{
		USHORT i2 = 0;
		for (const srq* slot = LOCK_header->lhb_hash; i2 < LOCK_header->lhb_hash_slots; slot++, i2++)
		{
			for (const srq* que_inst = (SRQ) SRQ_ABS_PTR(slot->srq_forward); que_inst != slot;
				 que_inst = (SRQ) SRQ_ABS_PTR(que_inst->srq_forward))
			{
				const lbl* lock = (lbl*) ((UCHAR *) que_inst - OFFSET(lbl*, lbl_lhb_hash));
				if (!sw_pending || lock->lbl_pending_lrq_count)
					prt_lock(outfile, LOCK_header, lock, sw_series);
			}
		}
	}

	if (sw_history)
	{
		prt_history(outfile, LOCK_header, LOCK_header->lhb_history, "History");
		prt_history(outfile, LOCK_header, a_shb->shb_history, "Event log");
	}

	prt_html_end(outfile);

	return FINI_OK;
}


static void prt_lock_activity(OUTFILE outfile,
							  const lhb* header,
							  USHORT flag,
							  ULONG seconds,
							  ULONG intervals)
{
/**************************************
 *
 *	p r t _ l o c k _ a c t i v i t y
 *
 **************************************
 *
 * Functional description
 *	Print a time-series lock activity report
 *
 **************************************/
	time_t clock = time(NULL);
	tm d = *localtime(&clock);

	FPRINTF(outfile, "%02d:%02d:%02d ", d.tm_hour, d.tm_min, d.tm_sec);

	if (flag & SW_I_ACQUIRE)
		FPRINTF(outfile, "acquire/s acqwait/s  %%acqwait acqrtry/s rtrysuc/s ");

	if (flag & SW_I_OPERATION)
		FPRINTF(outfile, "enqueue/s convert/s downgrd/s dequeue/s readata/s wrtdata/s qrydata/s ");

	if (flag & SW_I_TYPE)
		FPRINTF(outfile, "  dblop/s  rellop/s pagelop/s tranlop/s relxlop/s idxxlop/s misclop/s ");

	if (flag & SW_I_WAIT)
		FPRINTF(outfile, "   wait/s  reject/s timeout/s blckast/s  wakeup/s dlkscan/s deadlck/s ");

	FPRINTF(outfile, "\n");

	lhb base = *header;
	lhb prior = *header;

	if (intervals == 0)
	{
		memset(&base, 0, sizeof(base));
	}

	for (ULONG i = 0; i < intervals; i++)
	{
		fflush(outfile);
#ifdef WIN_NT
		Sleep(seconds * 1000);
#else
		sleep(seconds);
#endif
		clock = time(NULL);
		d = *localtime(&clock);

		FPRINTF(outfile, "%02d:%02d:%02d ", d.tm_hour, d.tm_min, d.tm_sec);

		if (flag & SW_I_ACQUIRE)
		{
			FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" %9"UQUADFORMAT" ",
					(header->lhb_acquires - prior.lhb_acquires) / seconds,
					(header->lhb_acquire_blocks - prior.lhb_acquire_blocks) / seconds,
					(header->lhb_acquires - prior.lhb_acquires) ?
					 	(100 * (header->lhb_acquire_blocks - prior.lhb_acquire_blocks)) /
							(header->lhb_acquires - prior.lhb_acquires) : 0,
					(header->lhb_acquire_retries -
					 prior.lhb_acquire_retries) / seconds,
					(header->lhb_retry_success -
					 prior.lhb_retry_success) / seconds);

			prior.lhb_acquires = header->lhb_acquires;
			prior.lhb_acquire_blocks = header->lhb_acquire_blocks;
			prior.lhb_acquire_retries = header->lhb_acquire_retries;
			prior.lhb_retry_success = header->lhb_retry_success;
		}

		if (flag & SW_I_OPERATION)
		{
			FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" ",
					(header->lhb_enqs - prior.lhb_enqs) / seconds,
					(header->lhb_converts - prior.lhb_converts) / seconds,
					(header->lhb_downgrades - prior.lhb_downgrades) / seconds,
					(header->lhb_deqs - prior.lhb_deqs) / seconds,
					(header->lhb_read_data - prior.lhb_read_data) / seconds,
					(header->lhb_write_data - prior.lhb_write_data) / seconds,
					(header->lhb_query_data - prior.lhb_query_data) / seconds);

			prior.lhb_enqs = header->lhb_enqs;
			prior.lhb_converts = header->lhb_converts;
			prior.lhb_downgrades = header->lhb_downgrades;
			prior.lhb_deqs = header->lhb_deqs;
			prior.lhb_read_data = header->lhb_read_data;
			prior.lhb_write_data = header->lhb_write_data;
			prior.lhb_query_data = header->lhb_query_data;
		}

		if (flag & SW_I_TYPE)
		{
			FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" ",
					(header->lhb_operations[Jrd::LCK_database] -
					 	prior.lhb_operations[Jrd::LCK_database]) / seconds,
					(header->lhb_operations[Jrd::LCK_relation] -
					 	prior.lhb_operations[Jrd::LCK_relation]) / seconds,
					(header->lhb_operations[Jrd::LCK_bdb] -
					 	prior.lhb_operations[Jrd::LCK_bdb]) / seconds,
					(header->lhb_operations[Jrd::LCK_tra] -
					 	prior.lhb_operations[Jrd::LCK_tra]) / seconds,
					(header->lhb_operations[Jrd::LCK_rel_exist] -
					 	prior.lhb_operations[Jrd::LCK_rel_exist]) / seconds,
					(header->lhb_operations[Jrd::LCK_idx_exist] -
					 	prior.lhb_operations[Jrd::LCK_idx_exist]) / seconds,
					(header->lhb_operations[0] - prior.lhb_operations[0]) / seconds);

			prior.lhb_operations[Jrd::LCK_database] = header->lhb_operations[Jrd::LCK_database];
			prior.lhb_operations[Jrd::LCK_relation] = header->lhb_operations[Jrd::LCK_relation];
			prior.lhb_operations[Jrd::LCK_bdb] = header->lhb_operations[Jrd::LCK_bdb];
			prior.lhb_operations[Jrd::LCK_tra] = header->lhb_operations[Jrd::LCK_tra];
			prior.lhb_operations[Jrd::LCK_rel_exist] = header->lhb_operations[Jrd::LCK_rel_exist];
			prior.lhb_operations[Jrd::LCK_idx_exist] = header->lhb_operations[Jrd::LCK_idx_exist];
			prior.lhb_operations[0] = header->lhb_operations[0];
		}

		if (flag & SW_I_WAIT)
		{
			FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
					" %9"UQUADFORMAT" ",
					(header->lhb_waits - prior.lhb_waits) / seconds,
					(header->lhb_denies - prior.lhb_denies) / seconds,
					(header->lhb_timeouts - prior.lhb_timeouts) / seconds,
					(header->lhb_blocks - prior.lhb_blocks) / seconds,
					(header->lhb_wakeups - prior.lhb_wakeups) / seconds,
					(header->lhb_scans - prior.lhb_scans) / seconds,
					(header->lhb_deadlocks - prior.lhb_deadlocks) / seconds);

			prior.lhb_waits = header->lhb_waits;
			prior.lhb_denies = header->lhb_denies;
			prior.lhb_timeouts = header->lhb_timeouts;
			prior.lhb_blocks = header->lhb_blocks;
			prior.lhb_wakeups = header->lhb_wakeups;
			prior.lhb_scans = header->lhb_scans;
			prior.lhb_deadlocks = header->lhb_deadlocks;
		}

		FPRINTF(outfile, "\n");
	}

	FB_UINT64 factor = seconds * intervals;

	if (factor < 1)
		factor = 1;

	FPRINTF(outfile, "\nAverage: ");
	if (flag & SW_I_ACQUIRE)
	{
		FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" %9"UQUADFORMAT" ",
				(header->lhb_acquires - base.lhb_acquires) / factor,
				(header->lhb_acquire_blocks - base.lhb_acquire_blocks) / factor,
				(header->lhb_acquires - base.lhb_acquires) ?
				 	(100 * (header->lhb_acquire_blocks - base.lhb_acquire_blocks)) /
						(header->lhb_acquires - base.lhb_acquires) : 0,
				(header->lhb_acquire_retries - base.lhb_acquire_retries) / factor,
				(header->lhb_retry_success - base.lhb_retry_success) / factor);
	}

	if (flag & SW_I_OPERATION)
	{
		FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"
				UQUADFORMAT" ",
				(header->lhb_enqs - base.lhb_enqs) / factor,
				(header->lhb_converts - base.lhb_converts) / factor,
				(header->lhb_downgrades - base.lhb_downgrades) / factor,
				(header->lhb_deqs - base.lhb_deqs) / factor,
				(header->lhb_read_data - base.lhb_read_data) / factor,
				(header->lhb_write_data - base.lhb_write_data) / factor,
				(header->lhb_query_data - base.lhb_query_data) / factor);
	}

	if (flag & SW_I_TYPE)
	{
		FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" ",
				(header->lhb_operations[Jrd::LCK_database] -
				 	base.lhb_operations[Jrd::LCK_database]) / factor,
				(header->lhb_operations[Jrd::LCK_relation] -
				 	base.lhb_operations[Jrd::LCK_relation]) / factor,
				(header->lhb_operations[Jrd::LCK_bdb] -
				 	base.lhb_operations[Jrd::LCK_bdb]) / factor,
				(header->lhb_operations[Jrd::LCK_tra] -
				 	base.lhb_operations[Jrd::LCK_tra]) / factor,
				(header->lhb_operations[Jrd::LCK_rel_exist] -
				 	base.lhb_operations[Jrd::LCK_rel_exist]) / factor,
				(header->lhb_operations[Jrd::LCK_idx_exist] -
				 	base.lhb_operations[Jrd::LCK_idx_exist]) / factor,
				(header->lhb_operations[0] - base.lhb_operations[0]) / factor);
	}

	if (flag & SW_I_WAIT)
	{
		FPRINTF(outfile, "%9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" %9"UQUADFORMAT" %9"UQUADFORMAT
				" %9"UQUADFORMAT" ",
				(header->lhb_waits - base.lhb_waits) / factor,
				(header->lhb_denies - base.lhb_denies) / factor,
				(header->lhb_timeouts - base.lhb_timeouts) / factor,
				(header->lhb_blocks - base.lhb_blocks) / factor,
				(header->lhb_wakeups - base.lhb_wakeups) / factor,
				(header->lhb_scans - base.lhb_scans) / factor,
				(header->lhb_deadlocks - base.lhb_deadlocks) / factor);
	}

	FPRINTF(outfile, "\n");
}


static void prt_history(OUTFILE outfile,
						const lhb* LOCK_header,
						SRQ_PTR history_header,
						const SCHAR* title)
{
/**************************************
 *
 *      p r t _ h i s t o r y
 *
 **************************************
 *
 * Functional description
 *      Print history list of lock table.
 *
 **************************************/
	FPRINTF(outfile, "%s:\n", title);

	for (const his* history = (his*) SRQ_ABS_PTR(history_header); true;
		 history = (his*) SRQ_ABS_PTR(history->his_next))
	{
		if (history->his_operation)
			FPRINTF(outfile,
					"    %s:\towner = %s, lock = %s, request = %s\n",
					history_names[history->his_operation],
					(const TEXT*)HtmlLink(preOwn, history->his_process),
					(const TEXT*)HtmlLink(preLock, history->his_lock),
					(const TEXT*)HtmlLink(preRequest, history->his_request));
		if (history->his_next == history_header)
			break;
	}
}


static void prt_lock(OUTFILE outfile, const lhb* LOCK_header, const lbl* lock, USHORT sw_series)
{
/**************************************
 *
 *      p r t _ l o c k
 *
 **************************************
 *
 * Functional description
 *      Print a formatted lock block
 *
 **************************************/
	if (sw_series && lock->lbl_series != sw_series)
		return;

	if (!sw_html_format)
		FPRINTF(outfile, "LOCK BLOCK %6"SLONGFORMAT"\n", SRQ_REL_PTR(lock));
	else
	{
		const SLONG rel_lock = SRQ_REL_PTR(lock);
		FPRINTF(outfile, "<a name=\"%s%"SLONGFORMAT"\">LOCK BLOCK %6"SLONGFORMAT"</a>\n",
				preLock, rel_lock, rel_lock);
	}
	FPRINTF(outfile,
			"\tSeries: %d, State: %d, size: %d length: %d data: %"ULONGFORMAT"\n",
			lock->lbl_series, lock->lbl_state, lock->lbl_size, lock->lbl_length, lock->lbl_data);

	if ((lock->lbl_series == Jrd::LCK_bdb || lock->lbl_series == Jrd::LCK_btr_dont_gc) &&
		lock->lbl_length == Jrd::PageNumber::getLockLen())
	{
		// Since fb 2.1 lock keys for page numbers (series == 3) contains
		// page space number in high long of two-longs key. Lets print it
		// in <page_space>:<page_number> format
		const UCHAR* q = lock->lbl_key;

		SLONG key;
		memcpy(&key, q, sizeof(SLONG));
		q += sizeof(SLONG);

		ULONG pg_space;
		memcpy(&pg_space, q, sizeof(SLONG));

		FPRINTF(outfile, "\tKey: %04"ULONGFORMAT":%06"SLONGFORMAT",", pg_space, key);
	}
	else if (lock->lbl_length == 4)
	{
		SLONG key;
		memcpy(&key, lock->lbl_key, 4);

		FPRINTF(outfile, "\tKey: %06"SLONGFORMAT",", key);
	}
	else
	{
		UCHAR temp[512];
		fb_assert(sizeof(temp) - 1u >= lock->lbl_length); // Not enough, see <%d> below.
		UCHAR* p = temp;
		const UCHAR* end_temp = p + sizeof(temp) - 1;
  		const UCHAR* q = lock->lbl_key;
  		const UCHAR* const end = q + lock->lbl_length;
		for (; q < end && p < end_temp; q++)
		{
			const UCHAR c = *q;
			if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c >= '0' && c <= '9') || c == '/')
			{
				*p++ = c;
			}
			else
			{
				char buf[6] = "";
				int n = sprintf(buf, "<%d>", c);
				if (n < 1 || p + n >= end_temp)
				{
					while (p < end_temp)
						*p++ = '.';

					break;
				}
				memcpy(p, buf, n);
				p += n;
			}
		}
		*p = 0;
		FPRINTF(outfile, "\tKey: %s,", temp);
	}

	FPRINTF(outfile, " Flags: 0x%02X, Pending request count: %6d\n",
			lock->lbl_flags, lock->lbl_pending_lrq_count);

	prt_que(outfile, LOCK_header, "\tHash que", &lock->lbl_lhb_hash, OFFSET(lbl*, lbl_lhb_hash));

	prt_que(outfile, LOCK_header, "\tRequests", &lock->lbl_requests,
			OFFSET(lrq*, lrq_lbl_requests), preRequest);

	const srq* que_inst;
	SRQ_LOOP(lock->lbl_requests, que_inst)
	{
		const lrq* request = (lrq*) ((UCHAR*) que_inst - OFFSET(lrq*, lrq_lbl_requests));
		FPRINTF(outfile,
				"\t\tRequest %s, Owner: %s, State: %d (%d), Flags: 0x%02X\n",
				(const TEXT*) HtmlLink(preRequest, SRQ_REL_PTR(request)),
				(const TEXT*) HtmlLink(preOwn, request->lrq_owner), request->lrq_state,
				request->lrq_requested, request->lrq_flags);
	}

	FPRINTF(outfile, "\n");
}


static void prt_owner(OUTFILE outfile,
					  const lhb* LOCK_header,
					  const own* owner,
					  bool sw_requests,
					  bool sw_waitlist,
					  bool sw_pending)
{
/**************************************
 *
 *      p r t _ o w n e r
 *
 **************************************
 *
 * Functional description
 *      Print a formatted owner block.
 *
 **************************************/
	const prc* process = (prc*) SRQ_ABS_PTR(owner->own_process);

	if (!sw_html_format)
		FPRINTF(outfile, "OWNER BLOCK %6"SLONGFORMAT"\n", SRQ_REL_PTR(owner));
	else
	{
		const SLONG rel_owner = SRQ_REL_PTR(owner);
		FPRINTF(outfile, "<a name=\"%s%"SLONGFORMAT"\">OWNER BLOCK %6"SLONGFORMAT"</a>\n",
				preOwn, rel_owner, rel_owner);
	}
	FPRINTF(outfile, "\tOwner id: %6"QUADFORMAT"d, type: %1d\n",
			owner->own_owner_id, owner->own_owner_type);

	FPRINTF(outfile, "\tProcess id: %6d (%s), thread id: %6"SIZEFORMAT"\n",
			process->prc_process_id,
			ISC_check_process_existence(process->prc_process_id) ? "Alive" : "Dead",
			owner->own_thread_id);

	const USHORT flags = owner->own_flags;
	FPRINTF(outfile, "\tFlags: 0x%02X ", flags);
	FPRINTF(outfile, " %s", (flags & OWN_wakeup) ? "wake" : "    ");
	FPRINTF(outfile, " %s", (flags & OWN_scanned) ? "scan" : "    ");
	FPRINTF(outfile, " %s", (flags & OWN_signaled) ? "sgnl" : "    ");
	FPRINTF(outfile, "\n");

	prt_que(outfile, LOCK_header, "\tRequests", &owner->own_requests,
			OFFSET(lrq*, lrq_own_requests), preRequest);
	prt_que(outfile, LOCK_header, "\tBlocks", &owner->own_blocks,
			OFFSET(lrq*, lrq_own_blocks), preRequest);
	prt_que(outfile, LOCK_header, "\tPending", &owner->own_pending,
			OFFSET(lrq*, lrq_own_pending), preRequest);

	if (sw_waitlist)
	{
		waitque owner_list;
		owner_list.waitque_depth = 0;
		prt_owner_wait_cycle(outfile, LOCK_header, owner, 8, &owner_list);
	}

	FPRINTF(outfile, "\n");

	if (sw_requests)
	{
		if (sw_pending)
		{
			const srq* que_inst;
			SRQ_LOOP(owner->own_pending, que_inst)
				prt_request(outfile, LOCK_header,
							(lrq*) ((UCHAR*) que_inst - OFFSET(lrq*, lrq_own_pending)));
		}
		else
		{
			const srq* que_inst;
			SRQ_LOOP(owner->own_requests, que_inst)
				prt_request(outfile, LOCK_header,
							(lrq*) ((UCHAR*) que_inst - OFFSET(lrq*, lrq_own_requests)));
		}
	}
}


static void prt_owner_wait_cycle(OUTFILE outfile,
								 const lhb* LOCK_header,
								 const own* owner,
								 USHORT indent, waitque* waiters)
{
/**************************************
 *
 *      p r t _ o w n e r _ w a i t _ c y c l e
 *
 **************************************
 *
 * Functional description
 *	For the given owner, print out the list of owners
 *	being waited on.  The printout is recursive, up to
 *	a limit.  It is recommended this be used with
 *	the -c consistency mode.
 *
 **************************************/
	for (USHORT i = indent; i; i--)
		FPRINTF(outfile, " ");

	// Check to see if we're in a cycle of owners - this might be
	// a deadlock, or might not, if the owners haven't processed
	// their blocking queues

	for (USHORT i = 0; i < waiters->waitque_depth; i++)
		if (SRQ_REL_PTR(owner) == waiters->waitque_entry[i])
		{
			FPRINTF(outfile, "%s (potential deadlock).\n",
					(const TEXT*) HtmlLink(preOwn, SRQ_REL_PTR(owner)));
			return;
		}

	FPRINTF(outfile, "%s waits on ", (const TEXT*) HtmlLink(preOwn, SRQ_REL_PTR(owner)));

	bool found = false;

	srq* lock_srq;
	SRQ_LOOP(owner->own_pending, lock_srq)
	{
		if (waiters->waitque_depth >= FB_NELEM(waiters->waitque_entry))
		{
			FPRINTF(outfile, "Dependency too deep\n");
			return;
		}

		found = true;

		waiters->waitque_entry[waiters->waitque_depth++] = SRQ_REL_PTR(owner);

		FPRINTF(outfile, "\n");
		const lrq* const owner_request = (lrq*) ((UCHAR*) lock_srq - OFFSET(lrq*, lrq_own_pending));
		fb_assert(owner_request->lrq_type == type_lrq);
		const bool owner_conversion = (owner_request->lrq_state > LCK_null);

		const lbl* const lock = (lbl*) SRQ_ABS_PTR(owner_request->lrq_lock);
		fb_assert(lock->lbl_type == type_lbl);

		int counter = 0;
		const srq* que_inst;
		SRQ_LOOP(lock->lbl_requests, que_inst)
		{
			if (counter++ > 50)
			{
				for (USHORT i = indent + 6; i; i--)
					FPRINTF(outfile, " ");
				FPRINTF(outfile, "printout stopped after %d owners\n", counter - 1);
				break;
			}

			const lrq* lock_request = (lrq*) ((UCHAR *) que_inst - OFFSET(lrq*, lrq_lbl_requests));
			fb_assert(lock_request->lrq_type == type_lrq);

			if (owner_conversion)
			{
				// Requests AFTER our request CAN block us
				if (lock_request == owner_request)
					continue;

				if (compatibility[owner_request->lrq_requested][lock_request->lrq_state])
					continue;
			}
			else
			{
				// Requests AFTER our request can't block us
				if (owner_request == lock_request)
					break;

				const UCHAR max_state = MAX(lock_request->lrq_state, lock_request->lrq_requested);

				if (compatibility[owner_request->lrq_requested][max_state])
				{
					continue;
				}
			}

			const own* const lock_owner = (own*) SRQ_ABS_PTR(lock_request->lrq_owner);
			prt_owner_wait_cycle(outfile, LOCK_header, lock_owner, indent + 4, waiters);
		}

		waiters->waitque_depth--;
	}

	if (!found)
		FPRINTF(outfile, "nothing.\n");
}


static void prt_request(OUTFILE outfile, const lhb* LOCK_header, const lrq* request)
{
/**************************************
 *
 *      p r t _ r e q u e s t
 *
 **************************************
 *
 * Functional description
 *      Print a format request block.
 *
 **************************************/

	if (!sw_html_format)
		FPRINTF(outfile, "REQUEST BLOCK %6"SLONGFORMAT"\n", SRQ_REL_PTR(request));
	else
	{
		const SLONG rel_request = SRQ_REL_PTR(request);
		FPRINTF(outfile, "<a name=\"%s%"SLONGFORMAT"\">REQUEST BLOCK %6"SLONGFORMAT"</a>\n",
				preRequest, rel_request, rel_request);
	}
	FPRINTF(outfile, "\tOwner: %s, Lock: %s, State: %d, Mode: %d, Flags: 0x%02X\n",
			(const TEXT*) HtmlLink(preOwn, request->lrq_owner),
			(const TEXT*) HtmlLink(preLock, request->lrq_lock), request->lrq_state,
			request->lrq_requested, request->lrq_flags);
	FPRINTF(outfile, "\tAST: 0x%p, argument: 0x%p\n",
			request->lrq_ast_routine, request->lrq_ast_argument);
	prt_que2(outfile, LOCK_header, "\tlrq_own_requests",
			 &request->lrq_own_requests, OFFSET(lrq*, lrq_own_requests), preRequest);
	prt_que2(outfile, LOCK_header, "\tlrq_lbl_requests",
			 &request->lrq_lbl_requests, OFFSET(lrq*, lrq_lbl_requests), preRequest);
	prt_que2(outfile, LOCK_header, "\tlrq_own_blocks  ",
			 &request->lrq_own_blocks, OFFSET(lrq*, lrq_own_blocks), preRequest);
	prt_que2(outfile, LOCK_header, "\tlrq_own_pending ",
			 &request->lrq_own_pending, OFFSET(lrq*, lrq_own_pending), preRequest);
	FPRINTF(outfile, "\n");
}


static void prt_que(OUTFILE outfile,
					const lhb* LOCK_header,
					const SCHAR* string, const srq* que_inst, USHORT que_offset,
					const TEXT* prefix)
{
/**************************************
 *
 *      p r t _ q u e
 *
 **************************************
 *
 * Functional description
 *      Print the contents of a self-relative que.
 *
 **************************************/
	const SLONG offset = SRQ_REL_PTR(que_inst);

	if (offset == que_inst->srq_forward && offset == que_inst->srq_backward)
	{
		FPRINTF(outfile, "%s: *empty*\n", string);
		return;
	}

	SLONG count = 0;
	const srq* next;
	SRQ_LOOP((*que_inst), next)
		++count;

	FPRINTF(outfile, "%s (%"SLONGFORMAT"):\tforward: %s, backward: %s\n", string, count,
			(const TEXT*) HtmlLink(prefix, que_inst->srq_forward - que_offset),
			(const TEXT*) HtmlLink(prefix, que_inst->srq_backward - que_offset));
}


static void prt_que2(OUTFILE outfile,
					 const lhb* LOCK_header,
					 const SCHAR* string, const srq* que_inst, USHORT que_offset,
					 const TEXT* prefix)
{
/**************************************
 *
 *      p r t _ q u e 2
 *
 **************************************
 *
 * Functional description
 *      Print the contents of a self-relative que.
 *      But don't try to count the entries, as they might be invalid
 *
 **************************************/
	const SLONG offset = SRQ_REL_PTR(que_inst);

	if (offset == que_inst->srq_forward && offset == que_inst->srq_backward)
	{
		FPRINTF(outfile, "%s: *empty*\n", string);
		return;
	}

	FPRINTF(outfile, "%s:\tforward: %s, backward: %s\n", string,
			(const TEXT*) HtmlLink(prefix, que_inst->srq_forward - que_offset),
			(const TEXT*) HtmlLink(prefix, que_inst->srq_backward - que_offset));
}

static void prt_html_begin(OUTFILE outfile)
{
/**************************************
 *
 *      p r t _ h t m l _ b e g i n
 *
 **************************************
 *
 * Functional description
 *      Print the html header if heeded
 *
 **************************************/
	if (!sw_html_format)
		return;

	FPRINTF(outfile, "<html><head><title>%s</title></head><body>", "Lock table");
	FPRINTF(outfile, "<pre>");

}

static void prt_html_end(OUTFILE outfile)
{
/**************************************
 *
 *      p r t _ h t m l _ e n d
 *
 **************************************
 *
 * Functional description
 *      Print the html finishing items
 *
 **************************************/
	if (!sw_html_format)
		return;

	FPRINTF(outfile, "</pre>");
	FPRINTF(outfile, "</body></html>");
}
