/* Set the cancel type for the calling thread.
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
#include <errno.h>

int
pthread_setcanceltype (int type, int *oldtype)
{
  struct pthread_internal_t *p = __get_thread ();
	int newflags;
	
	pthread_init();
	
  switch (type)
    {
    default:
      return EINVAL;
    case PTHREAD_CANCEL_DEFERRED:
    case PTHREAD_CANCEL_ASYNCHRONOUS:
      break;
    }

  pthread_mutex_lock (&p->cancel_lock);
  if (oldtype)
    *oldtype = p->attr.flags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	
	if(type == PTHREAD_CANCEL_ASYNCHRONOUS)
		p->attr.flags |= PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	else
		p->attr.flags &= ~PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS;
	newflags=p->attr.flags;
  pthread_mutex_unlock (&p->cancel_lock);

	if((newflags & PTHREAD_ATTR_FLAG_CANCEL_PENDING) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ENABLE) && (newflags & PTHREAD_ATTR_FLAG_CANCEL_ASYNCRONOUS))
		__pthread_do_cancel(p);
	
  return 0;
}