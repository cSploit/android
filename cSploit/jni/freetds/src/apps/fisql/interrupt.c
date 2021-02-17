/* Free ISQL - An isql for DB-Library (C) 2007 Nicholas S. Castellano
 *
 * This program  is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include <setjmp.h>
#include <signal.h>
#include <sybfront.h>
#include <sybdb.h>
#include "interrupt.h"

sigjmp_buf restart;
volatile int os_interrupt = 0;

void
inactive_interrupt_handler(int sig)
{
	siglongjmp(restart, 1);
}

void
active_interrupt_handler(int sig)
{
	os_interrupt = sig;
}

void
maybe_handle_active_interrupt(void)
{
	int sig;

	if (os_interrupt) {
		sig = os_interrupt;
		os_interrupt = 0;
		inactive_interrupt_handler(sig);
	}
}

int
active_interrupt_pending(DBPROCESS * dbproc)
{
	if (os_interrupt) {
		return TRUE;
	}
	return FALSE;
}

int
active_interrupt_servhandler(DBPROCESS * dbproc)
{
	return INT_CANCEL;
}
