/*
 *      PROGRAM:        Firebird Utility programs
 *      MODULE:         util.cpp
 *      DESCRIPTION:    Utility routines for fbguard & fbserver
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

#include "firebird.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>				// for open() and fcntl()
#include <errno.h>

#ifdef HAVE_FLOCK
#include <sys/file.h>			// for flock()
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>				// for fork()
#endif

#ifdef HAVE_WAIT_H
#include <wait.h>				// for waitpid()
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif
#ifndef WEXITSTATUS
# define WEXITSTATUS(stat_val) ((unsigned)(stat_val) >> 8)
#endif
#ifndef WIFEXITED
# define WIFEXITED(stat_val) (((stat_val) & 255) == 0)
#endif


#include "../common/gdsassert.h"
#include "../utilities/guard/util_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/utils_proto.h"


pid_t UTIL_start_process(const char* process, char** argv, const char* prog_name)
{
/**************************************
 *
 *      U T I L _ s t a r t _ p r o c e s s
 *
 **************************************
 *
 * Functional description
 *
 *     This function is used to create the specified process,
 *
 * Returns Codes:
 *	-1		Process spawn failed.
 *	pid		Successful creation. PID is returned.
 *
 *     Note: Make sure that the argument list ends with a null
 *     and the first argument is large enough to hold the complete
 *     expanded process name. (MAXPATHLEN recommended)
 *
 **************************************/
	fb_assert(process != NULL);
	fb_assert(argv != NULL);

	// prepend Firebird home directory to the program name
	Firebird::PathName string = fb_utils::getPrefix(Firebird::DirType::FB_DIR_SBIN, process);

	if (prog_name) {
		gds__log("%s: guardian starting %s\n", prog_name, string.c_str());
	}

	// add place in argv for visibility to "ps"
	strcpy(argv[0], string.c_str());
#if (defined SOLARIS)
	pid_t pid = fork1();
	if (!pid)
	{
		if (execv(string.c_str(), argv) == -1) {
			//ib_fprintf(ib_stderr, "Could not create child process %s with args %s\n", string, argv);
		}
		exit(FINI_ERROR);
	}

#else

	pid_t pid = vfork();
	if (!pid)
	{
		execv(string.c_str(), argv);
		_exit(FINI_ERROR);
	}
#endif
	return (pid);
}


int UTIL_wait_for_child(pid_t child_pid, const volatile sig_atomic_t& shutting_down)
{
/**************************************
 *
 *      U T I L _ w a i t _ f o r _ c h i l d
 *
 **************************************
 *
 * Functional description
 *
 *     This function is used to wait for the child specified by child_pid
 *
 * Return code:
 *	0	Normal exit
 *	-1	Abnormal exit - unknown reason.
 *	-2	TERM signal caught
 *	Other	Abnormal exit - process error code returned.
 *
 **************************************/
	int child_exit_status;

	fb_assert(child_pid != 0);

	// wait for the child process with child_pid to exit

	while (waitpid(child_pid, &child_exit_status, 0) == -1)
	{
		if (SYSCALL_INTERRUPTED(errno))
		{
			if (shutting_down)
				return -2;
			continue;
		}
		return (errno);
	}

	if (WIFEXITED(child_exit_status))
		return WEXITSTATUS(child_exit_status);

	return -1;
}


void alrm_handler(int)
{
	// handler for SIGALRM
	// doesn't do anything, just interrupts a syscall
}


int UTIL_shutdown_child(pid_t child_pid, unsigned timeout_term, unsigned timeout_kill)
{
/**************************************
 *
 *      U T I L _ s h u t d o w n _ c h i l d
 *
 **************************************
 *
 * Functional description
 *
 *     Terminates child using TERM signal, then KILL if it does not finish
 *     within specified timeout
 *
 * Return code:
 *	0	Child finished cleanly (TERM)
 *	1	Child killed (KILL)
 *	2	Child not killed by KILL
 *	-1	Syscall failed
 *
 **************************************/

	int r = kill(child_pid, SIGTERM);

	if (r < 0)
		return ((errno == ESRCH) ? 0 : -1);

	if (UTIL_set_handler(SIGALRM, alrm_handler, false) < 0)
		return -1;

	alarm(timeout_term);

	int child_status;
	r = waitpid(child_pid, &child_status, 0);

	if ((r < 0) && !SYSCALL_INTERRUPTED(errno))
		return -1;

	if (r == child_pid)
		return 0;

	r = kill(child_pid, SIGKILL);

	if (r < 0)
		return ((errno == ESRCH) ? 0 : -1);

	alarm(timeout_kill);
	r = waitpid(child_pid, &child_status, 0);

	if ((r < 0) && !SYSCALL_INTERRUPTED(errno))
		return -1;

	if (r == child_pid)
		return 1;

	return 2;
}


int UTIL_ex_lock(const TEXT* file)
{
/**************************************
 *
 *      U T I L _ e x _ l o c k
 *
 **************************************
 *
 * Functional description
 *
 *     This function is used to exclusively lock a file.
 *
 * Return Codes:
 *	-1		Failure to open file.
 *	-2		Failure to obtain exclusive lock on file.
 *	otherwise	Success - file descriptor is returned.
 *
 **************************************/

	// get the file name and prepend the complete path etc
	Firebird::PathName expanded_filename = fb_utils::getPrefix(Firebird::DirType::FB_DIR_GUARD, file);

	// file fd for the opened and locked file
	int fd_file = open(expanded_filename.c_str(), O_RDWR | O_CREAT, 0666);
	if (fd_file == -1)
	{
		fprintf(stderr, "Could not open %s for write\n", expanded_filename.c_str());
		return (-1);
	}

	fb_assert(fd_file != -2);

#ifndef HAVE_FLOCK
	// get an exclusive lock on the GUARD file without blocking on the call
	struct flock lock;
	lock.l_type = F_WRLCK;
	lock.l_whence = 0;
	lock.l_start = 0;
	lock.l_len = 0;
	if (fcntl(fd_file, F_SETLK, &lock) == -1)
#else
	if (flock(fd_file, LOCK_EX))
#endif
	{
		close(fd_file);
		return (-2);
	}
	return (fd_file);
}


void UTIL_ex_unlock( int fd_file)
{
/**************************************
 *
 *      U T I L _ e x _ l o c k
 *
 **************************************
 *
 * Functional description
 *
 *     This function is used to unlock the exclusive file.
 *
 **************************************/


#ifndef HAVE_FLOCK

	struct flock lock;

	// get an exclusive lock on the GUARD file with a block
	lock.l_type = F_UNLCK;
	lock.l_whence = 0;
	lock.l_start = 0;
	lock.l_len = 0;
	fcntl(fd_file, F_SETLKW, &lock);
#else
	flock(fd_file, LOCK_UN);
#endif
	close(fd_file);
}


int UTIL_set_handler(int sig, void (*handler) (int), bool restart)
{
/**************************************
 *
 *      U T I L _ s e t _ h a n d l e r
 *
 **************************************
 *
 * Functional description
 *
 *     This function sets signal handler
 *
 **************************************/

#if defined(HAVE_SIGACTION)
	struct sigaction sig_action;
	if (sigaction(sig, NULL, &sig_action) < 0)
		return -1;
	sig_action.sa_handler = handler;
	if (restart)
		sig_action.sa_flags |= SA_RESTART;
	else
		sig_action.sa_flags &= ~SA_RESTART;
	if (sigaction(sig, &sig_action, NULL) < 0)
		return -1;
#else
	if (signal(sig, handler) == SIG_ERR)
		return -1;
#endif
	return 0;
}
