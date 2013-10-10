
/* $Id: ec_passive.h,v 1.6 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_PASSIVE_H
#define EC_PASSIVE_H

EC_API_EXTERN int is_open_port(u_int8 proto, u_int16 port, u_int8 flags);
EC_API_EXTERN void print_host(struct host_profile *h); 
EC_API_EXTERN void print_host_xml(struct host_profile *h);

#endif

/* EOF */

// vim:ts=3:expandtab

