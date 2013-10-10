
/* $Id: ec_resolv.h,v 1.13 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_RESOLV_H
#define EC_RESOLV_H

#include <ec_inet.h>

#ifdef HAVE_ARPA_NAMESER_H
   #include <arpa/nameser.h>
   #ifndef OS_BSD_OPEN
      #include <arpa/nameser_compat.h>
   #endif
   #include <resolv.h>
#else
   #include <missing/nameser.h>
#endif

/*
 * glibc 2.1.x does not have new NG_GET* macros...
 * implement the hack here.
 */

#if !defined HAVE_NS_GET && !defined NS_GET16
   /* functions */
   #define NS_GET16     GETSHORT
   #define NS_GET32     GETLONG
   #define NS_PUT16     PUTSHORT
   #define NS_PUT32     PUTLONG
   /* constants */
   #define NS_MAXDNAME  MAXDNAME
   #define ns_c_in      C_IN
   #define ns_r_noerror NOERROR
   #define ns_t_cname   T_CNAME
   #define ns_t_ptr     T_PTR
   #define ns_t_a       T_A
   #define ns_t_mx      T_MX
   #define ns_o_query   QUERY
#endif



#define MAX_HOSTNAME_LEN   64

EC_API_EXTERN int host_iptoa(struct ip_addr *ip, char *name);

/* used by ec_dns to insert passively sniffed dns answers */
EC_API_EXTERN void resolv_cache_insert(struct ip_addr *ip, char *name);
   

   
#endif

/* EOF */

// vim:ts=3:expandtab

