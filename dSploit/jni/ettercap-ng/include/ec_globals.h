
/* $Id: ec_globals.h,v 1.67 2004/09/28 09:56:11 alor Exp $ */

#ifndef EC_GLOBALS_H
#define EC_GLOBALS_H

#include <ec_sniff.h>
#include <ec_inet.h>
#include <ec_ui.h>
#include <ec_stats.h>
#include <ec_profiles.h>
#include <ec_filter.h>
#include <ec_interfaces.h>

#include <regex.h>

/* options form etter.conf */
struct ec_conf {
   char *file;
   int ec_uid;
   int ec_gid;
   int arp_storm_delay;
   int arp_poison_warm_up;
   int arp_poison_delay;
   int arp_poison_icmp;
   int arp_poison_reply;
   int arp_poison_request;
   int arp_poison_equal_mac;
   int dhcp_lease_time;
   int port_steal_delay;
   int port_steal_send_delay;
   int connection_timeout;
   int connection_idle;
   int connection_buffer;
   int connect_timeout;
   int sampling_rate;
   int close_on_eof;
   int aggressive_dissectors;
   int skip_forwarded;
   int checksum_check;
   int checksum_warning;
   int store_profiles;
   struct curses_color colors;
   char *redir_command_on;
   char *redir_command_off;
   char *remote_browser;
   char *utf8_encoding;
};

/* options from getopt */
struct ec_options {
   char write:1;
   char read:1;
   char compress:1;
   char quiet:1;
   char superquiet:1;
   char silent:1;
   char unoffensive:1;
   char load_hosts:1;
   char save_hosts:1;
   char resolve:1;
   char ext_headers:1;
   char mitm:1;
   char only_mitm:1;
   char remote:1;
   char iflist:1;
   char reversed;
   char *hostsfile;
   char *plugin;
   char *proto;
   char *netmask;
   char *iface;
   char *iface_bridge;
   char *pcapfile_in;
   char *pcapfile_out;
   char *target1;
   char *target2;
   char *script;
   FILE *msg_fd;
   int (*format)(const u_char *, size_t, u_char *);
   regex_t *regex;
};

/* program name and version */
struct program_env {
   char *name;
   char *version;
   char *debug_file;
};

/* pcap structure */
struct pcap_env {
   void     *ifs;          /* this is a pcap_if_t pointer */
   void     *pcap;         /* this is a pcap_t pointer */
   void     *pcap_bridge;  /* this is a pcap_t pointer */
   void     *dump;         /* this is a pcap_dumper_t pointer */
   char     *buffer;       /* buffer to be used to handle all the packets */
   u_int8   align;         /* alignment needed on sparc 4*n - sizeof(media_hdr) */
   char     promisc;
   char     *filter;       /* pcap filter */
   u_int16  snaplen;
   int      dlt;
   u_int32  dump_size;     /* total dump size */
   u_int32  dump_off;      /* current offset */
};

/* lnet structure */
struct lnet_env {
   void *lnet_L3;       /* this is a libnet_t pointer */
   void *lnet;          /* this is a libnet_t pointer */ 
   void *lnet_bridge;   /* this is a libnet_t pointer */
};

/* per interface informations */
struct iface_env {
   struct ip_addr ip;
   struct ip_addr network;
   struct ip_addr netmask;
   u_int8 mac[MEDIA_ADDR_LEN];
   u_int16 mtu;
   char configured;
};

/* ip list per target */
struct ip_list {
   struct ip_addr ip;
   LIST_ENTRY(ip_list) next;
};

/* target specifications */
struct target_env {
   char scan_all:1;
   char all_mac:1;            /* these one bit flags are used as wildcards */
   char all_ip:1;
   char all_port:1;
   u_char mac[MEDIA_ADDR_LEN];
   LIST_HEAD (, ip_list) ips;
   u_int8 ports[1<<13];       /* in 8192 byte we have 65535 bits, use one bit per port */
};

/* scanned hosts list */
struct hosts_list {
   struct ip_addr ip;
   u_char mac[MEDIA_ADDR_LEN];
   char *hostname;
   LIST_ENTRY(hosts_list) next;
};


/* the globals container */
struct globals {
   struct ec_conf *conf;
   struct ec_options *options;
   struct gbl_stats *stats;
   struct ui_ops *ui;
   struct program_env *env;
   struct pcap_env *pcap;
   struct lnet_env *lnet;
   struct iface_env *iface;
   struct iface_env *bridge;
   struct sniffing_method *sm;
   struct target_env *t1;
   struct target_env *t2;
   LIST_HEAD(, hosts_list) hosts_list_head;
   TAILQ_HEAD(gbl_ptail, host_profile) profiles_list_head;
   struct filter_env *filters;
};

EC_API_EXTERN struct globals *gbls;

#define GBLS gbls

#define GBL_CONF           (GBLS->conf)
#define GBL_OPTIONS        (GBLS->options)
#define GBL_STATS          (GBLS->stats)
#define GBL_UI             (GBLS->ui)
#define GBL_ENV            (GBLS->env)
#define GBL_PCAP           (GBLS->pcap)
#define GBL_LNET           (GBLS->lnet)
#define GBL_IFACE          (GBLS->iface)
#define GBL_BRIDGE         (GBLS->bridge)
#define GBL_SNIFF          (GBLS->sm)
#define GBL_TARGET1        (GBLS->t1)
#define GBL_TARGET2        (GBLS->t2)
#define GBL_HOSTLIST       (GBLS->hosts_list_head)
#define GBL_PROFILES       (GBLS->profiles_list_head)
#define GBL_FILTERS        (GBLS->filters)

#define GBL_FORMAT         (GBL_OPTIONS->format)

#define GBL_PROGRAM        (GBL_ENV->name)
#define GBL_VERSION        (GBL_ENV->version)
#define GBL_DEBUG_FILE     (GBL_ENV->debug_file)


/* exported functions */

EC_API_EXTERN void globals_alloc(void);
EC_API_EXTERN void globals_free(void);

#endif

/* EOF */

// vim:ts=3:expandtab

