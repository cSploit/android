
/* $Id: ec_sniff.h,v 1.11 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_SNIFF_H
#define EC_SNIFF_H

#include <ec_packet.h>

struct sniffing_method {
   char type;              /* the type of the sniffing method */
      #define SM_UNIFIED      0
      #define SM_BRIDGED      1
   char active;            /* true if the sniff was started */
   void (*start)(void);
   void (*cleanup)(void);
   void (*check_forwarded)(struct packet_object *po);
   void (*set_forwardable)(struct packet_object *po);
   void (*forward)(struct packet_object *po);
   void (*interesting)(struct packet_object *po);  /* this function set the PO_IGNORE flag */
};

/* exported functions */

/* forwarder (the struct is in ec_globals.h) */
struct target_env;

EC_API_EXTERN void set_sniffing_method(struct sniffing_method *sm);

EC_API_EXTERN void set_unified_sniff(void);
EC_API_EXTERN void set_bridge_sniff(void);

EC_API_EXTERN int compile_display_filter(void);
EC_API_EXTERN int compile_target(char *string, struct target_env *target);

EC_API_EXTERN void reset_display_filter(struct target_env *t);

EC_API_EXTERN void del_ip_list(struct ip_addr *ip, struct target_env *t);
EC_API_EXTERN int cmp_ip_list(struct ip_addr *ip, struct target_env *t);
EC_API_EXTERN void add_ip_list(struct ip_addr *ip, struct target_env *t);
EC_API_EXTERN void free_ip_list(struct target_env *t);

#endif

/* EOF */

// vim:ts=3:expandtab

