/*
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
 */


// contains the main() and not shared routines for fbguard

#include "firebird.h"
#include <stdio.h>

#ifdef HAVE_STRING_H
#include <string.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_SYS_STAT_H
#include <sys/stat.h>
#endif

#ifdef HAVE_ERRNO_H
#include <errno.h>
#else
int errno = -1;
#endif

#include <time.h>

#include "../common/os/divorce.h"
#include "../common/isc_proto.h"
#include "../yvalve/gds_proto.h"
#include "../common/file_params.h"
#include "../utilities/guard/util_proto.h"
#include "../common/classes/fb_string.h"

const USHORT FOREVER	= 1;
const USHORT ONETIME	= 2;
const USHORT IGNORE		= 3;
const USHORT NORMAL_EXIT= 0;

const char* const SERVER_BINARY			= "firebird";

const char* const GUARD_FILE	= "fb_guard";

volatile sig_atomic_t shutting_down;


void shutdown_handler(int)
{
	shutting_down = 1;
}


int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
 *
 *      m a i n
 *
 **************************************
 *
 * Functional description
 *      The main for fbguard. This process is used to start
 *      the standalone server and keep it running after an abnormal termination.
 *
 **************************************/
	USHORT option = FOREVER;	// holds FOREVER or ONETIME  or IGNORE
	bool done = true;
	bool daemon = false;
	const TEXT* prog_name = argv[0];
	const TEXT* pidfilename = 0;
	int guard_exit_code = 0;

	const TEXT* const* const end = argc + argv;
	argv++;
	while (argv < end)
	{
		const TEXT* p = *argv++;
		if (*p++ == '-')
			switch (UPPER(*p))
			{
			case 'D':
				daemon = true;
				break;
			case 'F':
				option = FOREVER;
				break;
			case 'O':
				option = ONETIME;
				break;
			case 'S':
				option = IGNORE;
				break;
			case 'P':
				pidfilename = *argv++;
				break;
			default:
				fprintf(stderr,
						"Usage: %s [-signore | -onetime | -forever (default)] [-daemon] [-pidfile filename]\n",
						prog_name);
				exit(-1);
				break;
			}

	} // while

	// get and set the umask for the current process
	const ULONG new_mask = 0000;
	const ULONG old_mask = umask(new_mask);

	// exclusive lock the file
	int fd_guard;
	if ((fd_guard = UTIL_ex_lock(GUARD_FILE)) < 0)
	{
		// could not get exclusive lock -- some other guardian is running
		if (fd_guard == -2)
			fprintf(stderr, "%s: Program is already running.\n", prog_name);
		exit(-3);
	}

	// the umask back to orignal donot want to carry this to child process
	umask(old_mask);

	// move the server name into the argument to be passed
	TEXT process_name[1024];
	process_name[0] = '\0';
	TEXT* server_args[2];
	server_args[0] = process_name;
	server_args[1] = NULL;

	shutting_down = 0;
	if (UTIL_set_handler(SIGTERM, shutdown_handler, false) < 0)
	{
		fprintf(stderr, "%s: Cannot set signal handler (error %d).\n", prog_name, errno);
		exit(-5);
	}
	if (UTIL_set_handler(SIGINT, shutdown_handler, false) < 0)
	{
		fprintf(stderr, "%s: Cannot set signal handler (error %d).\n", prog_name, errno);
		exit(-5);
	}

	// detach from controlling tty
	if (daemon && fork()) {
		exit(0);
	}
	divorce_terminal(0);

	time_t timer = 0;

	do {
		int ret_code;

		if (shutting_down)
		{
			// don't start a child
			break;
		}

		if (timer == time(0))
		{
			// don't let server restart too often - avoid log overflow
			sleep(1);
			continue;
		}
		timer = time(0);

		pid_t child_pid =
			UTIL_start_process(SERVER_BINARY, server_args, prog_name);
		if (child_pid == -1)
		{
			// could not fork the server
			gds__log("%s: guardian could not start server\n", prog_name/*, process_name*/);
			fprintf(stderr, "%s: Could not start server\n", prog_name/*, process_name*/);
			UTIL_ex_unlock(fd_guard);
			exit(-4);
		}

		if (pidfilename)
		{
			FILE *pf = fopen(pidfilename, "w");
			if (pf)
			{
				fprintf(pf, "%d", child_pid);
				fclose(pf);
			}
			else
			{
				gds__log("%s: guardian could not open %s for writing, error %d\n",
						 prog_name, pidfilename, errno);
			}
		}

		// wait for child to die, and evaluate exit status
		bool shutdown_child = true;
		if (!shutting_down)
		{
			ret_code = UTIL_wait_for_child(child_pid, shutting_down);
			shutdown_child = (ret_code == -2);
		}
		if (shutting_down)
		{
			if (shutdown_child)
			{
				ret_code = UTIL_shutdown_child(child_pid, 3, 1);
				if (ret_code < 0)
				{
					gds__log("%s: error while shutting down %s (%d)\n",
							 prog_name, process_name, errno);
					guard_exit_code = -6;
				}
				else if (ret_code == 1) {
					gds__log("%s: %s killed (did not terminate)\n", prog_name, process_name);
				}
				else if (ret_code == 2) {
					gds__log("%s: unable to shutdown %s\n", prog_name, process_name);
				}
				else {
					gds__log("%s: %s terminated\n", prog_name, process_name);
				}
			}
			break;
		}
		if (ret_code != NORMAL_EXIT)
		{
			// check for startup error
			if (ret_code == STARTUP_ERROR)
			{
				gds__log("%s: %s terminated due to startup error (%d)\n",
						 prog_name, process_name, ret_code);
				if (option == IGNORE)
				{
					gds__log("%s: %s terminated due to startup error (%d)\n Trying again\n",
						 prog_name, process_name, ret_code);

					done = false;	// Try it again, Sam (even if it is a startup error) FSG 8.11.2000
				}
				else
				{
					gds__log("%s: %s terminated due to startup error (%d)\n",
							 prog_name, process_name, ret_code);

					done = true;	// do not restart we have a startup problem
				}
			}
			else
			{
				gds__log("%s: %s terminated abnormally (%d)\n", prog_name, process_name, ret_code);
				if (option == FOREVER || option == IGNORE)
					done = false;
			}
		}
		else
		{
			// Normal shutdown - don't restart the server
			gds__log("%s: %s normal shutdown.\n", prog_name, process_name);
			done = true;
		}
	} while (!done);

	if (pidfilename) {
		remove(pidfilename);
	}
	UTIL_ex_unlock(fd_guard);
	exit(guard_exit_code);
} // main
