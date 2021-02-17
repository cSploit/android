/*
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
 *  The Original Code was created by Alexander Peshkoff
 *  for the Firebird Open Source RDBMS project.
 *
 *  Copyright (c) 2009 Alexander Peshkoff <peshkoff@mail.ru>
 *  and all contributors signed below.
 *
 *  All Rights Reserved.
 *  Contributor(s): ______________________________________.
 *
 */


// =====================================
// File functions

#include "firebird.h"
#include "gen/iberror.h"

#include "../common/classes/init.h"
#include "../common/gdsassert.h"
#include "../common/os/os_utils.h"
#include "../jrd/constants.h"

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif
#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif
#ifdef HAVE_FCNTL_H
#include <fcntl.h>
#endif

#ifdef AIX_PPC
#define _UNIX95
#endif
#include <grp.h>
#ifdef AIX_PPC
#undef _UNIX95
#endif

#ifdef HAVE_PWD_H
#include <pwd.h>
#endif

using namespace Firebird;

namespace os_utils
{

static GlobalPtr<Mutex> grMutex;

// Return user group id if user group name found, otherwise return 0.
SLONG get_user_group_id(const TEXT* user_group_name)
{
	MutexLockGuard guard(grMutex, "get_user_group_id");

	const struct group* user_group = getgrnam(user_group_name);
	return user_group ? user_group->gr_gid : -1;
}

static GlobalPtr<Mutex> pwMutex;

// Return user id if user found, otherwise return -1.
SLONG get_user_id(const TEXT* user_name)
{
	MutexLockGuard guard(pwMutex, "get_user_id");

	const struct passwd* user = getpwnam(user_name);
	return user ? user->pw_uid : -1;
}

// Fills the buffer with home directory if user found
bool get_user_home(int user_id, PathName& homeDir)
{
	MutexLockGuard guard(pwMutex, "get_user_home");

	const struct passwd* user = getpwuid(user_id);
	if (user)
	{
		homeDir = user->pw_dir;
		return true;
	}
	return false;
}

namespace
{
	// runuser/rungroup
	const char* const FIREBIRD = "firebird";

	// change ownership and access of file
	void changeFileRights(const char* pathname, const mode_t mode)
	{
		uid_t uid = geteuid() == 0 ? get_user_id(FIREBIRD) : -1;
		gid_t gid = get_user_group_id(FIREBIRD);
		while (chown(pathname, uid, gid) < 0 && SYSCALL_INTERRUPTED(errno))
			;

		while (chmod(pathname, mode) < 0 && SYSCALL_INTERRUPTED(errno))
			;
	}
} // anonymous namespace

// create directory for lock files and set appropriate access rights
void createLockDirectory(const char* pathname)
{
	do
	{
		if (access(pathname, R_OK | W_OK | X_OK) == 0)
		{
			struct stat st;
			while (stat(pathname, &st) != 0)
			{
				if (SYSCALL_INTERRUPTED(errno))
				{
					continue;
				}
				system_call_failed::raise("stat");
			}

			if (S_ISDIR(st.st_mode))
			{
				return;
			}

			// not exactly original meaning, but very close to it
			system_call_failed::raise("access", ENOTDIR);
		}
	} while (SYSCALL_INTERRUPTED(errno));

	while (mkdir(pathname, 0700) != 0)		// anyway need chmod to avoid umask effects
	{
		if (SYSCALL_INTERRUPTED(errno))
		{
			continue;
		}
		(Arg::Gds(isc_lock_dir_access) << pathname).raise();
	}

	changeFileRights(pathname, 0770);
}

#ifndef S_IREAD
#define S_IREAD S_IRUSR
#endif
#ifndef S_IWRITE
#define S_IWRITE S_IWUSR
#endif

static void raiseError(int errCode, const char* filename)
{
	(Arg::Gds(isc_io_error) << "open" << filename << Arg::Gds(isc_io_open_err)
		<< SYS_ERR(errCode)).raise();
}


// open (or create if missing) and set appropriate access rights
int openCreateSharedFile(const char* pathname, int flags)
{
	int fd;
	do {
		fd = ::open(pathname, flags | O_RDWR | O_CREAT, S_IREAD | S_IWRITE);
	} while (fd < 0 && SYSCALL_INTERRUPTED(errno));

	if (fd < 0)
		raiseError(fd, pathname);

	// Security check - avoid symbolic links in /tmp.
	// Malicious user can create a symlink with this name pointing to say
	// security2.fdb and when the lock file is created the file will be damaged.

	struct stat st;
	int rc;

	do {
		rc = fstat(fd, &st);
	} while (fd != 0 && SYSCALL_INTERRUPTED(errno));

	if (rc != 0)
	{
		int e = errno;
		close(fd);
		raiseError(e, pathname);
	}

	if (S_ISLNK(st.st_mode))
	{
		close(fd);
		raiseError(ELOOP, pathname);
	}

	changeFileRights(pathname, 0660);

	return fd;
}

// set file's last access and modification time to current time
bool touchFile(const char* pathname)
{
#ifdef HAVE_UTIME_H
	while (utime(pathname, NULL) < 0)
	{
		if (SYSCALL_INTERRUPTED(errno))
		{
			continue;
		}
		return false;
	}

	return true;
#else
	return false;
#endif
}

} // namespace os_utils
