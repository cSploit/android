
/* $Id: ec_fingerprint.h,v 1.9 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_FINGERPRINT_H
#define EC_FINGERPRINT_H

EC_API_EXTERN int fingerprint_init(void);
EC_API_EXTERN int fingerprint_search(const char *f, char *dst);
EC_API_EXTERN void fingerprint_default(char *finger);
EC_API_EXTERN void fingerprint_push(char *finger, int param, int value);
EC_API_EXTERN u_int8 TTL_PREDICTOR(u_int8 x);
EC_API_EXTERN int fingerprint_submit(char *finger, char *os);


/*
 *  The fingerprint database has the following structure:                  
 *                                                                         
 *  WWWW:MSS:TTL:WS:S:N:D:T:F:LEN:OS                                       
 *                                                                         
 *  WWWW: 4 digit hex field indicating the TCP Window Size                 
 *  MSS : 4 digit hex field indicating the TCP Option Maximum Segment Size 
 *        if omitted in the packet or unknown it is "_MSS"                 
 *  TTL : 2 digit hex field indicating the IP Time To Live                 
 *  WS  : 2 digit hex field indicating the TCP Option Window Scale         
 *        if omitted in the packet or unknown it is "WS"                   
 *  S   : 1 digit field indicating if the TCP Option SACK permitted is true
 *  N   : 1 digit field indicating if the TCP Options contain a NOP        
 *  D   : 1 digit field indicating if the IP Don't Fragment flag is set    
 *  T   : 1 digit field indicating if the TCP Timestamp is present         
 *  F   : 1 digit ascii field indicating the flag of the packet            
 *        S = SYN                                                          
 *        A = SYN + ACK                                                    
 *  LEN : 2 digit hex field indicating the length of the packet            
 *        if irrilevant or unknown it is "LT"                              
 *  OS  : an ascii string representing the OS                              
 */


enum {
   FINGER_LEN = 28,
   OS_LEN     = 60,
   FINGER_WINDOW     = 0,
   FINGER_MSS        = 5,
   FINGER_TTL        = 10,
   FINGER_WS         = 13,
   FINGER_SACK       = 16,
   FINGER_NOP        = 18,
   FINGER_DF         = 20,
   FINGER_TIMESTAMP  = 22,
   FINGER_TCPFLAG    = 24,
   FINGER_LT         = 26,
};

/* 
 * the structure for passive information
 * carried by PO
 */

struct passive_info {
   char fingerprint[FINGER_LEN+1];
   char flags;
      #define FP_UNKNOWN         0  /* this happen reading form a file */
      #define FP_HOST_LOCAL      1
      #define FP_HOST_NONLOCAL   1<<1
      #define FP_GATEWAY         1<<2
      #define FP_ROUTER          1<<3
};


#endif

/* EOF */

// vim:ts=3:expandtab

