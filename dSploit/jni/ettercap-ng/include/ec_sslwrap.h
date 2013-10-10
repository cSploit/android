
/* $Id: ec_sslwrap.h,v 1.6 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_SSLWRAP_H
#define EC_SSLWRAP_H

#include <ec_decode.h>
#include <ec_threads.h>

EC_API_EXTERN void sslw_dissect_add(char *name, u_int32 port, FUNC_DECODER_PTR(decoder), u_char status);
EC_API_EXTERN void sslw_dissect_move(char *name, u_int16 port);
EC_API_EXTERN EC_THREAD_FUNC(sslw_start);
EC_API_EXTERN void ssl_wrap_init(void);

#define SSL_DISABLED	0
#define SSL_ENABLED	1 

#endif

/* EOF */

// vim:ts=3:expandtab

