
/* $Id: ec_scan.h,v 1.7 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_SCAN_H
#define EC_SCAN_H

EC_API_EXTERN void build_hosts_list(void);
EC_API_EXTERN void del_hosts_list(void);
EC_API_EXTERN void add_host(struct ip_addr *ip, u_int8 mac[MEDIA_ADDR_LEN], char *name);

EC_API_EXTERN int scan_load_hosts(char *filename);
EC_API_EXTERN int scan_save_hosts(char *filename);

#endif

/* EOF */

// vim:ts=3:expandtab

