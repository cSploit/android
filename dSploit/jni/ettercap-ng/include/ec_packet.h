
/* $Id: ec_packet.h,v 1.36 2004/07/24 10:43:21 alor Exp $ */

#if !defined(EC_PACKET_H)
#define EC_PACKET_H

#include <ec_proto.h>
#include <ec_profiles.h>
#include <ec_fingerprint.h>
#include <ec_inet.h>
#include <ec_session.h>

#include <sys/time.h>

struct packet_object {
 
   /* timestamp of the packet */
   struct timeval ts;
   
   struct L2 {
      u_int8 proto;
      u_char * header;
      size_t len;
      u_int8 src[MEDIA_ADDR_LEN];
      u_int8 dst[MEDIA_ADDR_LEN];
   } L2;
   
   struct L3 {
      u_int16 proto;
      u_char * header;
      u_char * options;
      size_t len;
      size_t payload_len;
      size_t optlen;
      struct ip_addr src;
      struct ip_addr dst;
      u_int8 ttl;
   } L3;
   
   struct L4 {
      u_int8 proto;
      u_int8 flags;
      u_char * header;
      u_char * options;
      size_t len;
      size_t optlen;
      u_int16 src;
      u_int16 dst;
      u_int32 seq;
      u_int32 ack;
   } L4;
   
   struct data {
      u_char * data;
      size_t len;
      /* 
       * buffer containing the data to be displayed.
       * some dissector decripts the traffic, but the packet must be forwarded as
       * is, so the decripted data must be placed in a different buffer. 
       * this is that bufffer and it is malloced by tcp or udp dissector.
       */
      size_t disp_len;
      u_char * disp_data;
      /* for modified packet this is the delta for the lenght */
      int delta;  
      size_t inject_len;      /* len of the injection */
      u_char *inject;         /* the fuffer used for injection */

   } DATA;

   size_t fwd_len;         /* lenght of the packet to be forwarded */
   u_char * fwd_packet;    /* the pointer to the buffer to be forwarded */
   
   size_t len;             /* total lenght of the packet */
   u_char * packet;        /* the buffer containing the real packet */

   /* Trace current session for injector chain */
   struct ec_session *session;  
    
   
   u_int16 flags;                       /* flags relative to the packet */
      #define PO_IGNORE       ((u_int16)(1))        /* this packet should not be processed (e.g. sniffing TARGETS didn't match it) */
      #define PO_DONT_DISSECT ((u_int16)(1<<1))     /* this packet should not be processed by dissector (used during the arp scan) */
      #define PO_FORWARDABLE  ((u_int16)(1<<2))     /* the packet has our MAC address, by the IP is not ours */
      #define PO_FORWARDED    ((u_int16)(1<<3))     /* the packet was forwarded by us */
      
      #define PO_FROMIFACE    ((u_int16)(1<<4))     /* this packet comes from the primary interface */
      #define PO_FROMBRIDGE   ((u_int16)(1<<5))     /* this packet comes form the bridged interface */
      
      #define PO_MODIFIED     ((u_int16)(1<<6))     /* it needs checksum recalculation before forwarding */
      #define PO_DROPPED      ((u_int16)(1<<7))     /* the packet has to be dropped */
  
      #define PO_DUP          ((u_int16)(1<<8))     /* the packet is a duplicate we have to free the buffer on destroy */
      
      #define PO_EOF          ((u_int16)(1<<9))     /* we are reading from a file and this is the last packet */

      #define PO_FROMSSL      ((u_int16)(1<<10))     /* the packet is coming from a ssl wrapper */

      #define PO_SSLSTART     ((u_int16)(1<<11))    /* ssl wrapper has to enter SSL state */
   
   /* 
    * here are stored the user and pass collected by dissectors 
    * the "char *" are malloc(ed) by dissectors
    */
   struct dissector_info DISSECTOR;
  
   /* the struct for passive identification */
   struct passive_info PASSIVE;
   
};

EC_API_EXTERN inline int packet_create_object(struct packet_object *po, u_char * buf, size_t len);
EC_API_EXTERN inline int packet_destroy_object(struct packet_object *po);
EC_API_EXTERN int packet_disp_data(struct packet_object *po, u_char *buf, size_t len);
EC_API_EXTERN struct packet_object * packet_dup(struct packet_object *po, u_char flag);

/* Do we want to duplicate data? */
#define PO_DUP_NONE     0
#define PO_DUP_PACKET   1

#endif

/* EOF */

// vim:ts=3:expandtab

