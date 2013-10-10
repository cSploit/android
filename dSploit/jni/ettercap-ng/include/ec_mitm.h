
/* $Id: ec_mitm.h,v 1.9 2004/07/28 08:06:15 alor Exp $ */

#ifndef EC_MITM_H
#define EC_MITM_H


struct mitm_method {
   char *name;
   int (*start)(char *args);
   void (*stop)(void);
};


/* exported functions */

EC_API_EXTERN void mitm_add(struct mitm_method *mm);
EC_API_EXTERN int mitm_set(char *name);
EC_API_EXTERN int mitm_start(void);
EC_API_EXTERN void mitm_stop(void);
EC_API_EXTERN void only_mitm(void);
EC_API_EXTERN int is_mitm_active(char *name);


/* an ugly hack to make accessible the arp poisoning lists to plugins */

LIST_HEAD(arp_groups, hosts_list);

EC_API_EXTERN struct arp_groups arp_group_one;
EC_API_EXTERN struct arp_groups arp_group_two;

#endif

/* EOF */

// vim:ts=3:expandtab

