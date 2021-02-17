/* Cancel a thread.
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

#include <pt-internal.h>

#include <errno.h>

int
pthread_cancel (pthread_t t)
{
  int err = 0;
  struct pthread_internal_t *p = (struct pthread_internal_t*) t;
	
	pthread_init();
	
  pthread_mutex_lock (&p->cancel_lock);
  if (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_PENDING)
    {
      pthread_mutex_unlock (&p->cancel_lock);
      return 0;
    }
    
  p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_PENDING;

  if (!(p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE))
    {
      pthread_mutex_unlock (&p->cancel_lock);
      return 0;
    }

  if (p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS) {
		pthread_mutex_unlock (&p->cancel_lock);
    err = __pthread_do_cancel (p);
	} else {
		// DEFERRED CANCEL NOT IMPLEMENTED YET
		pthread_mutex_unlock (&p->cancel_lock);
	}

  return err;
}