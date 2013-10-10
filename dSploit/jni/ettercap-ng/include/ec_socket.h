
/* $Id: ec_socket.h,v 1.6 2004/07/29 09:46:47 alor Exp $ */

#ifndef EC_SOCKET_H
#define EC_SOCKET_H

/* The never ending errno problems... */
#if defined(OS_WINDOWS) && !defined(OS_CYGWIN)
    #define GET_SOCK_ERRNO()  WSAGetLastError()
#else
    #define GET_SOCK_ERRNO()  errno
#endif

EC_API_EXTERN int open_socket(const char *host, u_int16 port);
EC_API_EXTERN int close_socket(int s);
EC_API_EXTERN void set_blocking(int s, int set);
EC_API_EXTERN int socket_send(int s, const u_char *payload, size_t size);
EC_API_EXTERN int socket_recv(int s, u_char *payload, size_t size);

#endif

/* EOF */

// vim:ts=3:expandtab

