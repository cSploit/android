
/* $Id: ec_profiles.h,v 1.17 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_PROFILES_H
#define EC_PROFILES_H

#include <ec_fingerprint.h>
#include <ec_resolv.h>

struct dissector_info {
   char *user;
   char *pass;
   char *info;
   char *banner;
   char failed;
};


/* the list of users for each port */
struct active_user {
   char *user;
   char *pass;
   char *info;
   u_int8 failed;
   struct ip_addr client;
   LIST_ENTRY(active_user) next;
};

/* each port is indentified this way : */

struct open_port {
   u_int16 L4_addr;
   u_int8  L4_proto;
   /* the service banner */
   char *banner;
   
   /* the list of users */
   LIST_HEAD(, active_user) users_list_head;
   
   LIST_ENTRY(open_port) next;
};


/* this contains all the info related to an host */

struct host_profile {
   
   u_int8 L2_addr[MEDIA_ADDR_LEN];

   struct ip_addr L3_addr;

   char hostname[MAX_HOSTNAME_LEN];
   
   /* the list of open ports */
   LIST_HEAD(, open_port) open_ports_head;
   
   /* distance in hop (TTL) */
   u_int8 distance;
   /* local or not ? */
   u_int8 type;

   /* OS fingerprint */
   u_char fingerprint[FINGER_LEN+1];

   TAILQ_ENTRY(host_profile) next;
};

/* exported functions */
EC_API_EXTERN void profile_purge_local(void);
EC_API_EXTERN void profile_purge_remote(void);
EC_API_EXTERN void profile_purge_all(void);
EC_API_EXTERN int profile_convert_to_hostlist(void);
EC_API_EXTERN int profile_dump_to_file(char *filename);

/* fake forward declaration (profiles include packet and viceversa) */
struct packet_object;
EC_API_EXTERN void profile_parse(struct packet_object *po);

EC_API_EXTERN void * profile_print(int mode, void *list, char **desc, size_t len);

#endif

/* EOF */

// vim:ts=3:expandtab

