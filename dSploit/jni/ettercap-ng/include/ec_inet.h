
/* $Id: ec_inet.h,v 1.26 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_INET_H
#define EC_INET_H

#ifdef OS_WINDOWS
   #include <winsock2.h>
   #include <ws2tcpip.h>
   #include <missing/inet_aton.h>
#else
   #include <netinet/in.h>
   #include <arpa/inet.h>
   #include <sys/socket.h>
#endif

#include <sys/stat.h>

#ifdef OS_CYGWIN
   #ifndef AF_INET6
      /* XXX - ugly hack only to make it compile */
      #define AF_INET6 23
   #endif
#endif

enum {
   NS_IN6ADDRSZ            = 16,
   NS_INT16SZ              = 2,

   ETH_ADDR_LEN            = 6,
   TR_ADDR_LEN             = 6,
   FDDI_ADDR_LEN           = 6,
   MEDIA_ADDR_LEN          = 6,
   
   IP_ADDR_LEN             = 4,
   IP6_ADDR_LEN            = 16,
   MAX_IP_ADDR_LEN         = IP6_ADDR_LEN,

   ETH_ASCII_ADDR_LEN      = sizeof("ff:ff:ff:ff:ff:ff")+1,
   IP_ASCII_ADDR_LEN       = sizeof("255.255.255.255")+1,
   IP6_ASCII_ADDR_LEN      = sizeof("ffff:ffff:ffff:ffff:ffff:ffff:255.255.255.255")+1,
   MAX_ASCII_ADDR_LEN      = IP6_ASCII_ADDR_LEN,                  
};

/* 
 * this structure is used by ettercap to handle 
 * an IP packet disregarding its version 
 */
struct ip_addr {
   u_int16 addr_type;
   u_int16 addr_len;
   /* this must be aligned in memory */
   u_int8 addr[MAX_IP_ADDR_LEN];
};

EC_API_EXTERN int ip_addr_init(struct ip_addr *sa, u_int16 type, u_char *addr);
EC_API_EXTERN int ip_addr_cpy(u_char *addr, struct ip_addr *sa);
EC_API_EXTERN int ip_addr_cmp(struct ip_addr *sa, struct ip_addr *sb);
EC_API_EXTERN int ip_addr_null(struct ip_addr *sa);
EC_API_EXTERN int ip_addr_is_zero(struct ip_addr *sa);

EC_API_EXTERN char *ip_addr_ntoa(struct ip_addr *sa, char *dst);
EC_API_EXTERN char *mac_addr_ntoa(u_char *mac, char *dst);
EC_API_EXTERN int mac_addr_aton(char *str, u_char *mac);

EC_API_EXTERN int ip_addr_is_local(struct ip_addr *sa);

/*
 * this prototypes are implemented in ./os/.../
 * each OS implement its specific function
 */

EC_API_EXTERN void disable_ip_forward(void);
EC_API_EXTERN u_int16 get_iface_mtu(const char *iface);

/********************/

#ifdef WORDS_BIGENDIAN       
   /* BIG ENDIAN */
   #define phtos(x) ( (u_int16)                       \
                      ((u_int16)*((u_int8 *)x+1)<<8|  \
                      (u_int16)*((u_int8 *)x+0)<<0)   \
                    )

   #define phtol(x) ( (u_int32)*((u_int8 *)x+3)<<24|  \
                      (u_int32)*((u_int8 *)x+2)<<16|  \
                      (u_int32)*((u_int8 *)x+1)<<8|   \
                      (u_int32)*((u_int8 *)x+0)<<0    \
                    )

   #define pntos(x) ( (u_int16)                       \
                      ((u_int16)*((u_int8 *)x+1)<<0|  \
                      (u_int16)*((u_int8 *)x+0)<<8)   \
                    )

   #define pntol(x) ( (u_int32)*((u_int8 *)x+3)<<0|   \
                      (u_int32)*((u_int8 *)x+2)<<8|   \
                      (u_int32)*((u_int8 *)x+1)<<16|  \
                      (u_int32)*((u_int8 *)x+0)<<24   \
                    )
   
   /* return little endian */
   #define htons_inv(x) (u_int16)(x << 8) | (x >> 8) 

   #define ORDER_ADD_SHORT(a, b)   a = a + b
   #define ORDER_ADD_LONG(a, b)	  a = a + b

#else
   /* LITTLE ENDIAN */
   #define phtos(x) *(u_int16 *)(x)
   #define phtol(x) *(u_int32 *)(x)

   #define pntos(x) ntohs(*(u_int16 *)(x))
   #define pntol(x) ntohl(*(u_int32 *)(x))
      
   /* return little endian */
   #define htons_inv(x) (u_int16)x
   
   #define ORDER_ADD_SHORT(a, b)   a = htons(ntohs(a) + (int16)b)
   #define ORDER_ADD_LONG(a, b)	  a = htonl(ntohl(a) + (int32)b)

#endif
      
   
#define int_ntoa(x)   inet_ntoa(*((struct in_addr *)&(x)))

#define ip_addr_to_int32(x)  *(u_int32 *)(x)
  
#endif


/* EOF */

// vim:ts=3:expandtab

