
/* $Id: ec_log.h,v 1.23 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_LOG_H
#define EC_LOG_H

#include <ec_inet.h>
#include <ec_packet.h>
#include <ec_fingerprint.h>
#include <ec_resolv.h>

#include <zlib.h>
#include <sys/time.h>


struct log_fd {
   int type;
      #define LOG_COMPRESSED     1
      #define LOG_UNCOMPRESSED   0
   gzFile cfd;
   int fd;
};


/*******************************************
 * NOTE:  all the int variable are stored  *
 *        in network order in the logfile  *
 *                                         *
 * NOTE:  log files are compressed with    *
 *        the deflate algorithm            *
 *******************************************/

/*
 * at the beginning of the file there 
 * are the global information
 */

struct log_global_header {
   /* a magic number for file identification */
   u_int16 magic;
   #define EC_LOG_MAGIC 0xe77e
   /* 
    * offset to the first header in the log file 
    * this assure that we can change this header 
    * and the etterlog parser will be able to 
    * parse also files created by older version
    */
   u_int16 first_header;
   /* ettercap version */
   char version[16];
   /* creation time of the log */
   struct timeval tv;
   /* the type of the log (packet or info) */
   u_int32 type;
};


/* 
 * every packet in the log file has this format:
 * [header][data][header][data]...
 */

/* this is for a generic packet */
struct log_header_packet {

   struct timeval tv;
   
   u_int8 L2_src[MEDIA_ADDR_LEN];
   u_int8 L2_dst[MEDIA_ADDR_LEN];

   struct ip_addr L3_src;
   struct ip_addr L3_dst;
   
   u_int8 L4_proto;
   u_int8 L4_flags;
   u_int16 L4_src;
   u_int16 L4_dst;
   
   u_int32 len;
};


/* 
 * this is for host infos 
 * 
 * the format will be:
 *
 * [header][user][pass][info][banner][header][user][pass]....
 *
 * every header contains in the structur "variable" the lenght
 * of the successive banner, user, pass and info fields.
 */
struct log_header_info {
   
   u_int8 L2_addr[MEDIA_ADDR_LEN];
   
   struct ip_addr L3_addr;
   
   u_int16 L4_addr;
   u_int8 L4_proto;

   char hostname[MAX_HOSTNAME_LEN];

   u_int8 distance;
   u_int8 type;
      #define LOG_ARP_HOST (1<<7)
   
   u_char fingerprint[FINGER_LEN+1];

   /* account informations */
   u_int8 failed;
   struct ip_addr client;

   struct variable {
      u_int16 user_len;
      u_int16 pass_len;
      u_int16 info_len;
      u_int16 banner_len;
   } var;
};


EC_API_EXTERN int set_loglevel(int level, char *filename);
#define LOG_STOP     0
#define LOG_INFO     1
#define LOG_PACKET   2

EC_API_EXTERN int set_msg_loglevel(int level, char *filename);
#define LOG_TRUE     1
#define LOG_FALSE    0

EC_API_EXTERN int log_open(struct log_fd *fd, char *filename);
EC_API_EXTERN void log_close(struct log_fd *fd);
EC_API_EXTERN int log_write_header(struct log_fd *fd, int type);
EC_API_EXTERN void log_write_packet(struct log_fd *fd, struct packet_object *po);
EC_API_EXTERN void log_write_info(struct log_fd *fd, struct packet_object *po);
EC_API_EXTERN void log_write_info_arp_icmp(struct log_fd *fd, struct packet_object *po);


#endif

/* EOF */

// vim:ts=3:expandtab

