/*
 *	PROGRAM:		UNIX signal relay program
 *	MODULE:			relay.cpp
 *	DESCRIPTION:	Signal relay program
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
#include <sys/param.h>
#include <signal.h>

#include "../jrd/license.h"

#include <stdlib.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif



int CLIB_ROUTINE main( int argc, char **argv)
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Wait on a pipe for a message, then forward a signal.
 *
 **************************************/
#ifndef DEBUG
	if (setreuid(0, 0) < 0)
		printf("gds_relay: couldn't set uid to superuser\n");
#ifdef HAVE_SETPGRP
#ifdef SETPGRP_VOID
	setpgrp();
#else
	setpgrp(0, 0);
#endif // SETPGRP_VOID
#else
#ifdef HAVE_SETPGID
	setpgid(0, 0);
#endif // HAVE_SETPGID
#endif // HAVE_SETPGRP
	signal(SIGINT, SIG_IGN);
	signal(SIGQUIT, SIG_IGN);
#endif // !DEBUG

	// Get the file descriptor ID - if it is present - make sure it's valid
	int fd;
	if (argc < 2 || (!(fd = atoi(argv[1])) && strcmp(argv[1], "0")))
		fd = -1;

	TEXT** end = argv + argc;
	while (argv < end)
	{
		TEXT* p = *argv++;
		if (*p++ == '-')
		{
			TEXT c;
			while (c = *p++)
			{
				switch (UPPER(c))
				{
				case 'Z':
					printf("Firebird relay version %s\n", FB_VERSION);
					exit(FINI_OK);
				}
			}
		}
	}

	if (fd == -1)
		exit(FINI_OK);

	// Close all files, except for the pipe input
	for (int n = 0; n < NOFILE; n++)
	{
#ifdef DEV_BUILD
		// Don't close stderr - we might need to report something
		if ((n != fd) && (n != 2))
#else
		if (n != fd)
#endif
			close(n);
	}

	SLONG msg[3];
	while (read(fd, msg, sizeof(msg)) == sizeof(msg))
	{
#ifdef DEV_BUILD
		// This is #ifdef for DEV_BUILD just in case a V3 client will
		// attempt communication with this V4 version.
		if (msg[2] != (msg[0] ^ msg[1])) {
			fprintf(stderr, "gds_relay received inconsistent message");
		}
#endif
		if (kill(msg[0], msg[1]))
		{
#ifdef DEV_BUILD
			fprintf(stderr, "gds_relay error on kill()");
#endif
		}
	}

	exit(FINI_OK);
}
