/*
    ettercap -- socket handling module

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

    $Id: ec_socket.c,v 1.15 2004/08/10 23:35:35 alor Exp $
*/

#include <ec.h>
#include <ec_signals.h>
#include <ec_poll.h>
#include <ec_socket.h>

#ifndef OS_WINDOWS
   #include <netdb.h>
   #include <sys/socket.h>
   #include <netinet/in.h>
   #include <arpa/inet.h>
#endif

#include <fcntl.h>

/* protos */

int open_socket(const char *host, u_int16 port);
int close_socket(int s);
void set_blocking(int s, int set);
int socket_send(int s, const u_char *payload, size_t size);
int socket_recv(int s, u_char *payload, size_t size);

/*******************************************/

/* 
 * set or unset blocking flag on a socket
 */
void set_blocking(int s, int set)
{
#ifdef OS_WINDOWS
   u_long on = set;
   ioctlsocket(s, FIONBIO, &on);
#else
   int ret;

   /* get the current flags */
   if ((ret = fcntl(s, F_GETFL, 0)) == -1)
      return;
   
   if (set) 
      ret &= ~O_NONBLOCK;
   else
      ret |= O_NONBLOCK;
   
   /* set the flag */
   fcntl (s, F_SETFL, ret);
#endif   
}


/*
 * open a socket to the specified host and port
 */
int open_socket(const char *host, u_int16 port)
{
   struct hostent *infh;
   struct sockaddr_in sa_in;
   int sh, ret, err = 0;
#define TSLEEP (50*1000) /* 50 milliseconds */
   int loops = (GBL_CONF->connect_timeout * 10e5) / TSLEEP;

   DEBUG_MSG("open_socket -- [%s]:[%d]", host, port);

   /* prepare the structures */
   memset((char*)&sa_in, 0, sizeof(sa_in));
   sa_in.sin_family = AF_INET;
   sa_in.sin_port = htons(port);

   /* resolve the hostname */
   if ( (infh = gethostbyname(host)) != NULL )
      memcpy(&sa_in.sin_addr, infh->h_addr, infh->h_length);
   else {
      if ( inet_aton(host, (struct in_addr *)&sa_in.sin_addr.s_addr) == 0 )
         return -ENOADDRESS;
   }

   /* open the socket */
   if ( (sh = socket(AF_INET, SOCK_STREAM, 0)) < 0)
      return -EFATAL;
 
   /* set nonblocking socket */
   set_blocking(sh, 0);
  
   do {
      /* connect to the server */
      ret = connect(sh, (struct sockaddr *)&sa_in, sizeof(sa_in));
      
      /* connect is in progress... */
      if (ret < 0) {
         err = GET_SOCK_ERRNO();
         if (err == EINPROGRESS || err == EALREADY || err == EWOULDBLOCK || err == EAGAIN) {
            /* sleep a quirk of time... */
            usleep(TSLEEP);
         }
      } else { 
         /* there was an error or the connect was successful */
         break;
      }
   } while(loops--);
 
   /* 
    * we cannot recall get_sock_errno because under windows
    * calling it twice would not return the same result
    */
   err = ret < 0 ? err : 0;
   
   /* reached the timeout */
   if (ret < 0 && (err == EINPROGRESS || err == EALREADY || err == EAGAIN)) {
      DEBUG_MSG("open_socket: connect() timeout: %d", err);
      close_socket(sh);
      return -ETIMEOUT;
   }

   /* error while connecting */
   if (ret < 0 && err != EISCONN) {
      DEBUG_MSG("open_socket: connect() error: %d", err);
      close_socket(sh);
      return -EINVALID;
   }
      
   DEBUG_MSG("open_socket: connect() connected.");
   
   /* reset the state to blocking socket */
   set_blocking(sh, 1);
   
   
   DEBUG_MSG("open_socket: %d", sh);
   
   return sh;
}

/*
 * close the given socket 
 */
int close_socket(int s)
{
   DEBUG_MSG("close_socket: %d", s);

   /* close the socket */
#ifdef OS_WINDOWS
   return closesocket(s);
#else   
   return close(s);
#endif   
}


/* 
 * send a buffer thru the socket 
 */
int socket_send(int s, const u_char *payload, size_t size)
{
   /* send data to the socket */
   return send(s, payload, size, 0);
}

/*
 * receive data from the socket
 */
int socket_recv(int sh, u_char *payload, size_t size)
{
   /* read up to size byte */
   return recv(sh, payload, size, 0);
}


/* EOF */

// vim:ts=3:expandtab

