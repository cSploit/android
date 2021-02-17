/*
    ettercap -- poll events (use poll or select)

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>

#ifdef HAVE_SYS_POLL_H
   #include <sys/poll.h>
#elif defined (HAVE_POLL_H)
   #include <poll.h>
#endif

#ifdef HAVE_SYS_SELECT_H
   #include <sys/select.h>
#endif

/* protos */

int ec_poll_in(int fd, u_int msec);
int ec_poll_out(int fd, u_int msec);
int ec_poll_buffer(char *buf);

/************************************************/

/*
 * looks for event on INPUT
 */
int ec_poll_in(int fd, u_int msec)
{
#if defined(HAVE_POLL) && !defined(OS_DARWIN)
   int poll_result;
   struct pollfd poll_fd;
   
   /* set the correct fd */
   poll_fd.fd = fd;
   poll_fd.events = POLLIN;
         
   /* execute the syscall */
   poll_result = poll(&poll_fd, 1, msec);
   
   /* the event has occurred, return 1 */
   if (poll_result > 0 && poll_fd.revents & POLLIN)
      return 1;
  
   return 0;
   
#elif defined(HAVE_SELECT)

   fd_set msk_fd;
   struct timeval to;
     
   memset(&to, 0, sizeof(struct timeval));
   /* timeval uses microseconds */
   to.tv_usec = msec * 1000;
   
   FD_ZERO(&msk_fd);
  
   /* set the correct fd */
   FD_SET(fd, &msk_fd);

   /* execute the syscall */
   int fds = select(FOPEN_MAX, &msk_fd, (fd_set *) 0, (fd_set *) 0, &to);
  
   if (fds <= 0) {
      return 0;
   } 
   /* the even has occurred */
   if (FD_ISSET(0, &msk_fd))
      return 1;
               
   return 0;
#else
   #error "you don't have neither poll nor select"
#endif
}


/*
 * looks for event on OUTPUT
 */
int ec_poll_out(int fd, u_int msec)
{
#if defined(HAVE_POLL) && !defined(OS_DARWIN)
   struct pollfd poll_fd;
   
   /* set the correct fd */
   poll_fd.fd = fd;
   poll_fd.events = POLLOUT;
         
   /* execute the syscall */
   poll(&poll_fd, 1, msec);

   /* the event has occurred, return 1 */
   if (poll_fd.revents & POLLOUT)
      return 1;
  
   return 0;
   
#elif defined(HAVE_SELECT)

   fd_set msk_fd;
   struct timeval to;
     
   memset(&to, 0, sizeof(struct timeval));
   /* timeval uses microseconds */
   to.tv_usec = msec * 1000;
   
   FD_ZERO(&msk_fd);
  
   /* set the correct fd */
   FD_SET(fd, &msk_fd);

   /* execute the syscall */
   int fds = select(FOPEN_MAX, (fd_set *) 0, &msk_fd, (fd_set *) 0, &to);
    
   if (fds <=0)
      return 0; 
   /* the even has occurred */
   if (FD_ISSET(0, &msk_fd))
      return 1;
               
   return 0;
#else
   #error "you don't have neither poll nor select"
#endif
}

/*
 * a fake poll implementation on string buffers
 * it returns != 0 if there are char(s) to read,
 * 0 if the first char is null
 */
int ec_poll_buffer(char *buf)
{
   if (buf)
      return *buf;
   else
      return 0;
}

/* EOF */

// vim:ts=3:expandtab

