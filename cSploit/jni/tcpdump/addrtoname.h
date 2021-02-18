/*
 * Copyright (c) 1990, 1992, 1993, 1994, 1995, 1996, 1997
 *	The Regents of the University of California.  All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that: (1) source code distributions
 * retain the above copyright notice and this paragraph in its entirety, (2)
 * distributions including binary code include the above copyright notice and
 * this paragraph in its entirety in the documentation or other materials
 * provided with the distribution, and (3) all advertising materials mentioning
 * features or use of this software display the following acknowledgement:
 * ``This product includes software developed by the University of California,
 * Lawrence Berkeley Laboratory and its contributors.'' Neither the name of
 * the University nor the names of its contributors may be used to endorse
 * or promote products derived from this software without specific prior
 * written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 * @(#) $Header: /tcpdump/master/tcpdump/addrtoname.h,v 1.18 2001/09/17 21:57:51 fenner Exp $ (LBL)
 */

/* Name to address translation routines. */

extern const char *linkaddr_string(const u_char *, const unsigned int);
extern const char *etheraddr_string(const u_char *);
extern const char *etherproto_string(u_short);
extern const char *tcpport_string(u_short);
extern const char *udpport_string(u_short);
extern const char *getname(const u_char *);
#ifdef INET6
extern const char *getname6(const u_char *);
#endif
extern const char *intoa(u_int32_t);

extern void init_addrtoname(u_int32_t, u_int32_t);
extern struct hnamemem *newhnamemem(void);
#ifdef INET6
extern struct h6namemem *newh6namemem(void);
#endif

#define ipaddr_string(p) getname((const u_char *)(p))
#ifdef INET6
#define ip6addr_string(p) getname6((const u_char *)(p))
#endif
