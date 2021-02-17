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

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include "tds_sysdep_public.h"
#include <freetds/thread.h>

#define int2ptr(i) ((void*)(((char*)0)+(i)))
#define ptr2int(p) ((int)(((char*)(p))-((char*)0)))

#if !defined(TDS_NO_THREADSAFE)

#if defined(__MINGW32__) || defined(_WIN32)
#define sleep(s) Sleep((s)*1000)
#endif

static tds_mutex mtx = TDS_MUTEX_INITIALIZER;

static TDS_THREAD_PROC_DECLARE(signal_proc, arg)
{
	tds_condition *cond = (tds_condition *) arg;

	tds_mutex_lock(&mtx);
	if (tds_cond_signal(cond)) {
		tds_mutex_unlock(&mtx);
		/* failure */
		return int2ptr(1);
	}
	tds_mutex_unlock(&mtx);
	/* success */
	return int2ptr(0);
}

static void check(int cond, const char *msg)
{
	if (cond) {
		fprintf(stderr, "%s\n", msg);
		exit(1);
	}
}

int main(void)
{
	tds_condition cond;
	tds_thread th;
	void *res;

	check(tds_cond_init(&cond), "failed initializing condition");

	tds_mutex_lock(&mtx);

	check(tds_thread_create(&th, signal_proc, &cond) != 0, "error creating thread");

	sleep(1);

	check(tds_cond_wait(&cond, &mtx), "failed waiting condition");

	check(tds_thread_join(th, &res) != 0, "error waiting thread");

	check(ptr2int(res) != 0, "error signaling condition");

	tds_mutex_unlock(&mtx);

	check(tds_cond_destroy(&cond), "failed destroying condition");
	return 0;
}

#else

int main(void)
{
	return 0;
}

#endif

