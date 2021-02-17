/*
 *	PROGRAM:	UNIX resource removal program
 *	MODULE:		drop.cpp
 *	DESCRIPTION:	Drop shared memory and semaphores
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
 *
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "DELTA" port
 * 2002.10.27 Sean Leyne - Completed removal of obsolete "IMP" port
 *
 * 2002.10.29 Sean Leyne - Removed obsolete "Netware" port
 *
*/

#include "firebird.h"
#include <stdio.h>
#include <errno.h>
#include <sys/param.h>
#include <sys/stat.h>

#include "../common/isc_s_proto.h"
#include "../common/isc_s_proto.h"
#include "../lock/lock_proto.h"
#include "../jrd/license.h"
#include "../yvalve/gds_proto.h"
#include "../common/isc_proto.h"
#include "../common/config/config.h"

#ifdef HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#ifdef HAVE_SYS_WAIT_H
# include <sys/wait.h>
#endif

#ifdef HAVE_STRING_H
#include <string.h>
#endif

const int FTOK_KEY = 15;

static SLONG get_key(const TEXT*);
static void remove_resource(const TEXT*, SLONG, const TEXT*);
#ifndef HAVE_MMAP
static void dummy_init();
static int shm_exclusive(SLONG, SLONG);
#endif

//static int orig_argc;
//static SCHAR **orig_argv;


int CLIB_ROUTINE main( int argc, char *argv[])
{
/**************************************
 *
 *	m a i n
 *
 **************************************
 *
 * Functional description
 *	Drop Lock Table and associated semaphores.
 *
 **************************************/
	bool sw_lockmngr = false;
	bool sw_events = false;
	bool sw_version = false;
	bool sw_shutmngr = false;

	//orig_argc = argc;
	//orig_argv = argv;

	SCHAR** const end = argv + argc;
	while (++argv < end)
		if (**argv == '-')
			for (const char* p = *argv + 1; *p; p++)
				switch (UPPER(*p))
				{

				case 'E':
					sw_events = true;
					break;

				case 'L':
					sw_lockmngr = true;
					break;

				case 'A':
					sw_events = sw_lockmngr = true;
					break;

				case 'S':
					sw_shutmngr = true;
					break;

				//case 'N':
				//	break;

				case 'Z':
					sw_version = true;
					break;

				default:
					printf("***Ignoring unknown switch %c.\n", *p);
					break;
				}

	if (sw_version)
		printf("gds_drop version %s\n", FB_VERSION);

	if (sw_events)
		remove_resource(EVENT_FILE, Config::getEventMemSize(), "events");

	if (sw_lockmngr)
		remove_resource(LOCK_FILE, Config::getLockMemSize(), "lock manager");


	exit(FINI_OK);
}


#ifndef HAVE_MMAP
static void dummy_init()
{
/**************************************
 *
 *	d u m m y _ i n i t
 *
 **************************************
 *
 * Functional description
 *	A dummy callback routine for ISC_map_file.
 *
 **************************************/
}
#endif

static SLONG get_key(const TEXT* filename)
{
/*************************************
 *
 *	g e t _ k e y
 *
 *************************************
 *
 * Functional description
 *	Find the semaphore/shared memory key for a file.
 *
 ************************************/
	TEXT expanded_filename[128], hostname[64];

#ifdef NOHOSTNAME
	strcpy(expanded_filename, filename);
#else
	sprintf(expanded_filename, filename, ISC_get_host(hostname, sizeof(hostname)));
#endif

	// Produce shared memory key for file

	return ftok(expanded_filename, FTOK_KEY);
}


#ifndef HAVE_MMAP
static void remove_resource(const TEXT* filename, SLONG shm_length, const TEXT* label)
{
/**************************************
 *
 *	r e m o v e _ r e s o u r c e		( n o n - m m a p )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	ISC_STATUS_ARRAY status_vector;
	SH_MEM_T shmem_data;
	TEXT expanded_filename[MAXPATHLEN];

	gds__prefix_lock(expanded_filename, filename);

	if (!ISC_map_file
#ifdef HP11
		(status_vector, expanded_filename,
		(void (*) (void*, sh_mem*, bool)) dummy_init, 0, shm_length, &shmem_data))
#else
		(status_vector, expanded_filename, dummy_init, 0, shm_length, &shmem_data))
#endif
	{
		printf("\n***Unable to access %s resources:\n", label);
		gds__print_status(status_vector);
		return;
	}

	const SLONG key = get_key(expanded_filename);
	if (key == -1)
	{
		printf("\n***Unable to get the key value of the %s file.\n", label);
		return;
	}

	SLONG shmid = shm_exclusive(key, shmem_data.sh_mem_length_mapped);
	if (shmid == -1)
	{
		printf("\n***File for %s is currently in use.\n", label);
		return;
	}

	if (shmctl(shmid, IPC_RMID, 0) == -1)
		printf("\n***Error trying to drop %s file.  ERRNO = %d.\n", label, errno);
	else
		printf("Successfully removed %s file.\n", label);
}
#endif


#ifdef HAVE_MMAP
static void remove_resource(const TEXT* filename, SLONG shm_length, const TEXT* label)
{
/**************************************
 *
 *	r e m o v e _ r e s o u r c e		( m m a p )
 *
 **************************************
 *
 * Functional description
 *
 **************************************/
	TEXT expanded_filename[MAXPATHLEN];

	gds__prefix_lock(expanded_filename, filename);

	const SLONG key = get_key(expanded_filename);
	if (key == -1) {
		printf("\n***Unable to get the key value of the %s file.\n", label);
	}
}
#endif


#ifndef HAVE_MMAP
static int shm_exclusive( SLONG key, SLONG length)
{
/**************************************
 *
 *	s h m _ e x c l u s i v e
 *
 **************************************
 *
 * Functional description
 *	Check to see if we are the only ones accessing
 *	shared memory.  Return a shared memory id
 *	if so, -1 otherwise.
 *
 **************************************/
	struct shmid_ds buf;

	const int id = shmget(key, (int) length, IPC_ALLOC);
	if (id == -1 || shmctl(id, IPC_STAT, &buf) == -1 || buf.shm_nattch != 1)
	{
		return -1;
	}

	return id;
}
#endif
