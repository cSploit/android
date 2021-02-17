/* FreeTDS - Library of routines accessing Sybase and Microsoft databases
 * Copyright (C) 2010  Frediano Ziglio
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

#if HAVE_STDLIB_H
#include <stdlib.h>
#endif /* HAVE_STDLIB_H */

#ifdef _WIN32

#if HAVE_STRING_H
#include <string.h>
#endif /* HAVE_STRING_H */

#include <freetds/tds.h>
#include <freetds/thread.h>

TDS_RCSID(var, "$Id: win_mutex.c,v 1.4 2011-09-01 07:55:57 freddy77 Exp $");

#include "ptw32_MCS_lock.c"

void
tds_win_mutex_lock(tds_mutex * mutex)
{
	if (!InterlockedExchangeAdd(&mutex->done, 0)) {	/* MBR fence */
		ptw32_mcs_local_node_t node;

		ptw32_mcs_lock_acquire(&mutex->lock, &node);
		if (!mutex->done) {
			InitializeCriticalSection(&mutex->crit);
			mutex->done = 1;
		}
		ptw32_mcs_lock_release(&node);
	}
	EnterCriticalSection(&mutex->crit);
}

int
tds_mutex_trylock(tds_mutex * mutex)
{
	if (!mutex->done && !InterlockedExchangeAdd(&mutex->done, 0)) {	/* MBR fence */
		ptw32_mcs_local_node_t node;

		ptw32_mcs_lock_acquire(&mutex->lock, &node);
		if (!mutex->done) {
			InitializeCriticalSection(&mutex->crit);
			mutex->done = 1;
		}
		ptw32_mcs_lock_release(&node);
	}
	if (TryEnterCriticalSection(&mutex->crit))
		return 0;
	return -1;
}

#endif
