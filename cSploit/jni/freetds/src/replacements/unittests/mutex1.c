/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2012  Frediano Ziglio
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <config.h>

#include <stdio.h>

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#include "tds_sysdep_public.h"
#include <freetds/thread.h>

#define int2ptr(i) ((void*)(((char*)0)+(i)))
#define ptr2int(p) ((int)(((char*)(p))-((char*)0)))

#if !defined(TDS_NO_THREADSAFE)

static tds_mutex mtx = TDS_MUTEX_INITIALIZER;

static TDS_THREAD_PROC_DECLARE(trylock_proc, arg)
{
	tds_mutex *mtx = (tds_mutex *) arg;

	if (!tds_mutex_trylock(mtx)) {
		/* got mutex, failure as should be locked */
		return int2ptr(1);
	}
	/* success */
	return int2ptr(0);
}

static void
test(tds_mutex *mtx)
{
	tds_thread th;
	void *res;

	if (tds_mutex_trylock(mtx)) {
		fprintf(stderr, "mutex should be unlocked\n");
		exit(1);
	}
	/* this success on Windows cause mutex are recursive */
#ifndef _WIN32
	if (!tds_mutex_trylock(mtx)) {
		fprintf(stderr, "mutex should be locked\n");
		exit(1);
	}
#endif

	if (tds_thread_create(&th, trylock_proc, mtx) != 0) {
		fprintf(stderr, "error creating thread\n");
		exit(1);
	}

	if (tds_thread_join(th, &res) != 0) {
		fprintf(stderr, "error waiting thread\n");
		exit(1);
	}

	if (ptr2int(res) != 0) {
		fprintf(stderr, "mutex should be locked inside thread\n");
		exit(1);
	}

	tds_mutex_unlock(mtx);
}

int main(void)
{
	tds_mutex local;

	test(&mtx);

	/* try allocating it */
	if (tds_mutex_init(&local)) {
		fprintf(stderr, "error creating mutex\n");
		exit(1);
	}
	test(&local);
	tds_mutex_free(&local);

	/* try again */
	if (tds_mutex_init(&local)) {
		fprintf(stderr, "error creating mutex\n");
		exit(1);
	}
	test(&local);
	tds_mutex_free(&local);

	return 0;
}

#else

int main(void)
{
	return 0;
}

#endif

