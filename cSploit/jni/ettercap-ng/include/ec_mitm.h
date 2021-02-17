

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

LIST_HEAD(hosts_group, hosts_list);

EC_API_EXTERN struct hosts_group arp_group_one;
EC_API_EXTERN struct hosts_group arp_group_two;

#endif

/* EOF */

// vim:ts=3:expandtab

