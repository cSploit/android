/* Init a thread.
   Copyright (C) 2002 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#include <pthread.h>
#include <bthread.h>
#include <pt-internal.h>
#include <signal.h>

void pthread_cancel_handler(int signum) {
	pthread_exit(0);
}

void pthread_init(void) {
	struct sigaction sa;
	struct pthread_internal_t * p = (struct pthread_internal_t *)pthread_self();
	
	if(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_HANDLER)
		return;
	
	// set thread status as pthread_create should do.
	// ASYNCROUNOUS is not set, see pthread_setcancelstate(3)
	p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_HANDLER|PTHREAD_ATTR_FLAG_CANCEL_ENABLE;
	
	sa.sa_handler = pthread_cancel_handler;
	sigemptyset(&(sa.sa_mask));
	sa.sa_flags = 0;
	
	sigaction(SIGRTMIN, &sa, NULL);
}
