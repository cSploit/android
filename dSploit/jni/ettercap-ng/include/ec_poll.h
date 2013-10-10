
/* $Id: ec_poll.h,v 1.5 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_POLL_H
#define EC_POLL_H


EC_API_EXTERN int ec_poll_in(int fd, u_int msec);
EC_API_EXTERN int ec_poll_out(int fd, u_int msec);
EC_API_EXTERN int ec_poll_buffer(char *buf);

#endif

/* EOF */

// vim:ts=3:expandtab

