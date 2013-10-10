/*
    ettercap -- log handling module

    Copyright (C) ALoR & NaGA

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

    $Id: ec_log.c,v 1.41 2004/09/30 16:01:45 alor Exp $
*/

#include <ec.h>
#include <ec_log.h>
#include <ec_file.h>
#include <ec_packet.h>
#include <ec_passive.h>
#include <ec_threads.h>
#include <ec_hook.h>
#include <ec_resolv.h>

#include <fcntl.h>
#include <sys/time.h>
#include <sys/stat.h>

#include <zlib.h>
#include <regex.h>

/* globals */

static struct log_fd fdp;
static struct log_fd fdi;

/* protos */

int set_loglevel(int level, char *filename);
int set_msg_loglevel(int level, char *filename);
static void log_stop(void);

int log_open(struct log_fd *fd, char *filename);
void log_close(struct log_fd *fd);

int log_write_header(struct log_fd *fd, int type);

static void log_packet(struct packet_object *po);
static void log_info(struct packet_object *po);
void log_write_packet(struct log_fd *fd, struct packet_object *po);
void log_write_info(struct log_fd *fd, struct packet_object *po);
void log_write_info_arp_icmp(struct log_fd *fd, struct packet_object *po);

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
#define LOG_LOCK     do{ pthread_mutex_lock(&log_mutex); } while(0)
#define LOG_UNLOCK   do{ pthread_mutex_unlock(&log_mutex); } while(0)

/************************************************/

/* 
 * this function is executed at high privs.
 * open the file descriptor for later use
 * and set the log level
 * LOG_PACKET = packet + info
 * LOG_INFO = only info
 */

int set_loglevel(int level, char *filename)
{
   char eci[strlen(filename)+5];
   char ecp[strlen(filename)+5];
 
   /* close any previously opened file */
   log_stop();
  
   /* if we want to stop logging, return here */
   if (level == LOG_STOP) {
      DEBUG_MSG("set_loglevel: stopping the log process");
      return ESUCCESS;
   }
   
   DEBUG_MSG("set_loglevel(%d, %s)", level, filename); 

   /* all the host type will be unknown, warn the user */
   if (GBL_OPTIONS->read) {
      USER_MSG("*********************************************************\n");
      USER_MSG("WARNING: while reading form file we cannot determine    \n");
      USER_MSG("if an host is local or not because the ip address of     \n");
      USER_MSG("the NIC may have been changed from the time of the dump. \n");
      USER_MSG("*********************************************************\n\n");
   }
   
   sprintf(eci, "%s.eci", filename);
   sprintf(ecp, "%s.ecp", filename);
   
   memset(&fdp, 0, sizeof(struct log_fd));
   memset(&fdi, 0, sizeof(struct log_fd));

   /* open the file(s) */
   switch(level) {

      case LOG_PACKET:
         if (GBL_OPTIONS->compress) {
            fdp.type = LOG_COMPRESSED;
         } else {
            fdp.type = LOG_UNCOMPRESSED;
         }
         
         /* create the file */
         if (log_open(&fdp, ecp) != ESUCCESS)
            return -EFATAL;

         /* initialize the log file */
         log_write_header(&fdp, LOG_PACKET);
         
         /* add the hook point to DISPATCHER */
         hook_add(HOOK_DISPATCHER, &log_packet);

         /* no break here, loglevel is incremental */
         
      case LOG_INFO:
         if (GBL_OPTIONS->compress) {
            fdi.type = LOG_COMPRESSED;
         } else {
            fdi.type = LOG_UNCOMPRESSED;
         }
         
         /* create the file */
         if (log_open(&fdi, eci) != ESUCCESS)
            return -EFATAL;
         
         /* initialize the log file */
         log_write_header(&fdi, LOG_INFO);

         /* add the hook point to DISPATCHER */
         hook_add(HOOK_DISPATCHER, &log_info);
        
         /* add the hook for the ARP packets */
         hook_add(HOOK_PACKET_ARP, &log_info);
         
         /* add the hook for ICMP packets */
         hook_add(HOOK_PACKET_ICMP, &log_info);
         
         /* add the hook for DHCP packets */
         /* (fake icmp packets from DHCP discovered GW and DNS) */
         hook_add(HOOK_PROTO_DHCP_PROFILE, &log_info);

         break;
   }

   atexit(&log_stop);

   return ESUCCESS;
}

/*
 * removes the hook points and closes the log files
 */
static void log_stop(void)
{
   DEBUG_MSG("log_stop");
   
   /* remove all the hooks */
   hook_del(HOOK_DISPATCHER, &log_packet);
   hook_del(HOOK_DISPATCHER, &log_info);
   hook_del(HOOK_PACKET_ARP, &log_info);
   hook_del(HOOK_PACKET_ICMP, &log_info);
   hook_del(HOOK_PROTO_DHCP_PROFILE, &log_info);

   log_close(&fdp);
   log_close(&fdi);
}

/*
 * open a file in the appropriate log_fd struct
 */
int log_open(struct log_fd *fd, char *filename)
{
   int zerr;

   if (fd->type == LOG_COMPRESSED) {
      fd->cfd = gzopen(filename, "wb9");
      if (fd->cfd == NULL)
         SEMIFATAL_ERROR("%s", gzerror(fd->cfd, &zerr));
   } else {
      fd->fd = open(filename, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600);
      if (fd->fd == -1)
         SEMIFATAL_ERROR("Can't create %s: %s", filename, strerror(errno));
   }
   
   /* set the permissions */
   chmod(filename, 0600);

   return ESUCCESS;
}

/* 
 * closes a log_fd struct 
 */
void log_close(struct log_fd *fd)
{
   DEBUG_MSG("log_close: type: %d [%p][%d]", fd->type, fd->cfd, fd->fd);
   
   if (fd->type == LOG_COMPRESSED && fd->cfd) {
      gzclose(fd->cfd);
      fd->cfd = NULL;
   } else if (fd->type == LOG_UNCOMPRESSED && fd->fd) {
      close(fd->fd);
      fd->fd = 0;
   }
}

/* 
 * function registered to HOOK_DISPATCHER
 * check the regex (if present) and log packets
 */
static void log_packet(struct packet_object *po)
{
   /* 
    * skip packet sent (spoofed) by us
    * else we will get duplicated hosts with our mac address
    * this is necessary because check_forwarded() is executed
    * in ec_ip.c, but here we are getting even arp packets...
    */
   EXECUTE(GBL_SNIFF->check_forwarded, po);
   if (po->flags & PO_FORWARDED)
      return;

   /*
    * recheck if the packet is compliant with the visualization filters.
    * we need to redo the test, because here we are hooked to ARP and ICMP
    * packets that are before the test in ec_decode.c
    */
   po->flags |= PO_IGNORE;
   EXECUTE(GBL_SNIFF->interesting, po);
   if ( po->flags & PO_IGNORE )
       return;

   /* the regex is set, respect it */
   if (GBL_OPTIONS->regex) {
      if (regexec(GBL_OPTIONS->regex, po->DATA.disp_data, 0, NULL, 0) == 0)
         log_write_packet(&fdp, po);
   } else {
      /* if no regex is set, dump all the packets */
      log_write_packet(&fdp, po);
   }
   
}

/* 
 * function registered to HOOK_DISPATCHER
 * it is a wrapper to the real one
 */
static void log_info(struct packet_object *po)
{
   /* 
    * skip packet sent (spoofed) by us
    * else we will get duplicated hosts with our mac address
    * this is necessary because check_forwarded() is executed
    * in ec_ip.c, but here we are getting even arp packets...
    */
   EXECUTE(GBL_SNIFF->check_forwarded, po);
   if (po->flags & PO_FORWARDED)
      return;

   /*
    * recheck if the packet is compliant with the visualization filters.
    * we need to redo the test, because here we are hooked to ARP and ICMP
    * packets that are before the test in ec_decode.c
    */
   po->flags |= PO_IGNORE;
   EXECUTE(GBL_SNIFF->interesting, po);
   if ( po->flags & PO_IGNORE )
       return;

   /* if all the tests are ok, write it to the disk */
   if (po->L4.proto == NL_TYPE_ICMP || po->L3.proto == htons(LL_TYPE_ARP))
      log_write_info_arp_icmp(&fdi, po);      
   else
      log_write_info(&fdi, po);
}

/*
 * initialize the log file with 
 * the propre header
 */

int log_write_header(struct log_fd *fd, int type)
{
   struct log_global_header lh;
   int c, zerr;
   
   DEBUG_MSG("log_write_header : type %d", type);

   memset(&lh, 0, sizeof(struct log_global_header));

   /* the magic number */
   lh.magic = htons(EC_LOG_MAGIC);
   
   /* the offset of the first header is equal to the size of this header */
   lh.first_header = htons(sizeof(struct log_global_header));
   
   strlcpy(lh.version, GBL_VERSION, sizeof(lh.version));
   
   /* creation time of the file */
   gettimeofday(&lh.tv, 0);
   lh.tv.tv_sec = htonl(lh.tv.tv_sec);
   lh.tv.tv_usec = htonl(lh.tv.tv_usec);
      
   lh.type = htonl(type);

   LOG_LOCK;
   
   if (fd->type == LOG_COMPRESSED) {
      c = gzwrite(fd->cfd, &lh, sizeof(lh));
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
   } else {
      c = write(fd->fd, &lh, sizeof(lh));
      ON_ERROR(c, -1, "Can't write to logfile");
   }

   LOG_UNLOCK;
   
   return c;
}



/* log all the packet to the logfile */

void log_write_packet(struct log_fd *fd, struct packet_object *po)
{
   struct log_header_packet hp;
   int c, zerr;

   memset(&hp, 0, sizeof(struct log_header_packet));
   
   /* adjust the timestamp */
   memcpy(&hp.tv, &po->ts, sizeof(struct timeval));
   hp.tv.tv_sec = htonl(hp.tv.tv_sec);
   hp.tv.tv_usec = htonl(hp.tv.tv_usec);
  
   memcpy(&hp.L2_src, &po->L2.src, MEDIA_ADDR_LEN);
   memcpy(&hp.L2_dst, &po->L2.dst, MEDIA_ADDR_LEN);
   
   memcpy(&hp.L3_src, &po->L3.src, sizeof(struct ip_addr));
   memcpy(&hp.L3_dst, &po->L3.dst, sizeof(struct ip_addr));
  
   hp.L4_flags = po->L4.flags;
   hp.L4_proto = po->L4.proto;
   hp.L4_src = po->L4.src;
   hp.L4_dst = po->L4.dst;
 
   /* the length of the payload */
   hp.len = htonl(po->DATA.disp_len);

   LOG_LOCK;
   
   if (fd->type == LOG_COMPRESSED) {
      c = gzwrite(fd->cfd, &hp, sizeof(hp));
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));

      c = gzwrite(fd->cfd, po->DATA.disp_data, po->DATA.disp_len);
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
   } else {
      c = write(fd->fd, &hp, sizeof(hp));
      ON_ERROR(c, -1, "Can't write to logfile");

      c = write(fd->fd, po->DATA.disp_data, po->DATA.disp_len);
      ON_ERROR(c, -1, "Can't write to logfile");
   }
   
   LOG_UNLOCK;
}


/*
 * log passive informations
 *
 * hi is the source
 * hid is the dest, used to log password.
 *    since they must be associated to the 
 *    server and not to the client.
 *    so we create a new entry in the logfile
 */

void log_write_info(struct log_fd *fd, struct packet_object *po)
{
   struct log_header_info hi;
   struct log_header_info hid;
   int c, zerr;

   memset(&hi, 0, sizeof(struct log_header_info));
   memset(&hid, 0, sizeof(struct log_header_info));

   /* the mac address */
   memcpy(&hi.L2_addr, &po->L2.src, MEDIA_ADDR_LEN);
   memcpy(&hid.L2_addr, &po->L2.dst, MEDIA_ADDR_LEN);
   
   /* the ip address */
   memcpy(&hi.L3_addr, &po->L3.src, sizeof(struct ip_addr));
   /* the account must be associated with the server, so use dst */
   memcpy(&hid.L3_addr, &po->L3.dst, sizeof(struct ip_addr));
  
   /* the protocol */
   hi.L4_proto = po->L4.proto;
   hid.L4_proto = po->L4.proto;

   /* open on the source ? */
   if (is_open_port(po->L4.proto, po->L4.src, po->L4.flags))
      hi.L4_addr = po->L4.src;
   else if (po->DISSECTOR.banner)
      hi.L4_addr = po->L4.src;
   else
      hi.L4_addr = 0;
  
   /* open on the dest ? */
   if (is_open_port(po->L4.proto, po->L4.dst, po->L4.flags))
      hid.L4_addr = po->L4.dst;
   else if (po->DISSECTOR.user)
      hid.L4_addr = po->L4.dst;
   else
      hid.L4_addr = 0;

   /*
    * resolves the ip address.
    *
    * even if the resolv option was not specified,
    * the cache may have the dns answer passively sniffed.
    */
   
   host_iptoa(&po->L3.src, hi.hostname);
   host_iptoa(&po->L3.dst, hid.hostname);
   
   /* 
    * distance in hop :
    *
    * the distance is calculated as the difference between the
    * predicted initial ttl number and the current ttl value.
    */
   hi.distance = TTL_PREDICTOR(po->L3.ttl) - po->L3.ttl + 1;
   /* our machine is at distance 0 (special case) */
   if (!ip_addr_cmp(&po->L3.src, &GBL_IFACE->ip))
      hi.distance = 0;

   /* OS identification */
   memcpy(&hi.fingerprint, po->PASSIVE.fingerprint, FINGER_LEN);
   
   /* local, non local ecc ecc */
   hi.type = po->PASSIVE.flags;

   /* calculate if the dest is local or not */
   switch (ip_addr_is_local(&po->L3.dst)) {
      case ESUCCESS:
         hid.type |= FP_HOST_LOCAL;
         break;
      case -ENOTFOUND:
         hid.type |= FP_HOST_NONLOCAL;
         break;
      case -EINVALID:
         hid.type = FP_UNKNOWN;
         break;
   }
   
   /* set account informations */
   hid.failed = po->DISSECTOR.failed;
   memcpy(&hid.client, &po->L3.src, sizeof(struct ip_addr));
   
   /* set the length of the fields */
   if (po->DISSECTOR.user)
      hid.var.user_len = htons(strlen(po->DISSECTOR.user));

   if (po->DISSECTOR.pass)
      hid.var.pass_len = htons(strlen(po->DISSECTOR.pass));
   
   if (po->DISSECTOR.info)
      hid.var.info_len = htons(strlen(po->DISSECTOR.info));
   
   if (po->DISSECTOR.banner)
      hi.var.banner_len = htons(strlen(po->DISSECTOR.banner));
  
   /* check if the packet is interesting... else return */
   if (hi.L4_addr == 0 &&                 // the port is not open
       !strcmp(hi.fingerprint, "") &&     // no fingerprint
       hid.var.user_len == 0 &&           // no user and pass infos...
       hid.var.pass_len == 0 &&
       hid.var.info_len == 0 &&
       hi.var.banner_len == 0
       ) {
      return;
   }
   
   LOG_LOCK;
   
   if (fd->type == LOG_COMPRESSED) {
      c = gzwrite(fd->cfd, &hi, sizeof(hi));
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
    
      /* and now write the variable fields */
      
      if (po->DISSECTOR.banner) {
         c = gzwrite(fd->cfd, po->DISSECTOR.banner, strlen(po->DISSECTOR.banner) );
         ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
      }
      
   } else {
      c = write(fd->fd, &hi, sizeof(hi));
      ON_ERROR(c, -1, "Can't write to logfile");
      
      if (po->DISSECTOR.banner) {
         c = write(fd->fd, po->DISSECTOR.banner, strlen(po->DISSECTOR.banner) );
         ON_ERROR(c, -1, "Can't write to logfile");
      }
   }
  
   /* write hid only if there is user and pass infos */
   if (hid.var.user_len == 0 &&
       hid.var.pass_len == 0 &&
       hid.var.info_len == 0 
       ) {
      LOG_UNLOCK;
      return;
   }

   
   if (fd->type == LOG_COMPRESSED) {
      c = gzwrite(fd->cfd, &hid, sizeof(hi));
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
    
      /* and now write the variable fields */
      if (po->DISSECTOR.user) {
         c = gzwrite(fd->cfd, po->DISSECTOR.user, strlen(po->DISSECTOR.user) );
         ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
      }

      if (po->DISSECTOR.pass) {
         c = gzwrite(fd->cfd, po->DISSECTOR.pass, strlen(po->DISSECTOR.pass) );
         ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
      }

      if (po->DISSECTOR.info) {
         c = gzwrite(fd->cfd, po->DISSECTOR.info, strlen(po->DISSECTOR.info) );
         ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
      }
      
   } else {
      c = write(fd->fd, &hid, sizeof(hi));
      ON_ERROR(c, -1, "Can't write to logfile");
      
      if (po->DISSECTOR.user) {
         c = write(fd->fd, po->DISSECTOR.user, strlen(po->DISSECTOR.user) );
         ON_ERROR(c, -1, "Can't write to logfile");
      }

      if (po->DISSECTOR.pass) {
         c = write(fd->fd, po->DISSECTOR.pass, strlen(po->DISSECTOR.pass) );
         ON_ERROR(c, -1, "Can't write to logfile");
      }

      if (po->DISSECTOR.info) {
         c = write(fd->fd, po->DISSECTOR.info, strlen(po->DISSECTOR.info) );
         ON_ERROR(c, -1, "Can't write to logfile");
      }
      
   }

   LOG_UNLOCK;
}

/*
 * log hosts through ARP and ICMP discovery
 */

void log_write_info_arp_icmp(struct log_fd *fd, struct packet_object *po)
{
   struct log_header_info hi;
   int c, zerr;

   memset(&hi, 0, sizeof(struct log_header_info));

   /* the mac address */
   memcpy(&hi.L2_addr, &po->L2.src, MEDIA_ADDR_LEN);
   
   /* the ip address */
   memcpy(&hi.L3_addr, &po->L3.src, sizeof(struct ip_addr));
  
   /* set the distance */
   if (po->L3.ttl > 1)
      hi.distance = TTL_PREDICTOR(po->L3.ttl) - po->L3.ttl + 1;
   else
      hi.distance = po->L3.ttl;
   
   /* resolve the host */
   host_iptoa(&po->L3.src, hi.hostname);
   
   /* local, non local ecc ecc */
   if (po->L3.proto == htons(LL_TYPE_ARP)) {
      hi.type |= LOG_ARP_HOST;
      hi.type |= FP_HOST_LOCAL;
   } else {
      hi.type = po->PASSIVE.flags;
   }
   
   LOG_LOCK;
   
   if (fd->type == LOG_COMPRESSED) {
      c = gzwrite(fd->cfd, &hi, sizeof(hi));
      ON_ERROR(c, -1, "%s", gzerror(fd->cfd, &zerr));
   } else {
      c = write(fd->fd, &hi, sizeof(hi));
      ON_ERROR(c, -1, "Can't write to logfile");
   }

   LOG_UNLOCK;
}


/*
 * open/close the file to store all the USER_MSG
 */
int set_msg_loglevel(int level, char *filename)
{
   switch (level) {
      case LOG_TRUE:
         /* close the filedesc if already opened */
         set_msg_loglevel(LOG_FALSE, filename);

         GBL_OPTIONS->msg_fd = fopen(filename, FOPEN_WRITE_TEXT);
         if (GBL_OPTIONS->msg_fd == NULL)
            FATAL_MSG("Cannot open \"%s\" for writing", filename);

         break;
         
      case LOG_FALSE:
         /* close the file and set the pointer to NULL */
         if (GBL_OPTIONS->msg_fd) {
            fclose(GBL_OPTIONS->msg_fd);
            GBL_OPTIONS->msg_fd = NULL;
         }
         break;
   }

   return ESUCCESS;
}

/* EOF */

// vim:ts=3:expandtab

