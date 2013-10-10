/*
    ettercap -- passive TCP finterprint module

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

    $Id: ec_fingerprint.c,v 1.23 2004/06/25 14:24:29 alor Exp $

*/

#include <ec.h>
#include <ec_file.h>
#include <ec_socket.h>
#include <ec_fingerprint.h>

#define LOAD_ENTRY(p,h,v) do {                                 \
   SAFE_CALLOC((p), 1, sizeof(struct entry));                  \
   memcpy((p)->finger, h, FINGER_LEN);                         \
   (p)->finger[FINGER_LEN] = '\0';                             \
   (p)->os = strdup (v);                                       \
   (p)->os[strlen(p->os)-1] = '\0';                            \
} while (0)

/* globals */

static SLIST_HEAD(, entry) finger_head;

struct entry {
   char finger[FINGER_LEN+1];
   char *os;
   SLIST_ENTRY(entry) next;
};

/* protos */

static void fingerprint_discard(void);
int fingerprint_init(void);
int fingerprint_search(const char *f, char *dst);

void fingerprint_default(char *finger);
void fingerprint_push(char *finger, int param, int value);
u_int8 TTL_PREDICTOR(u_int8 x);
int fingerprint_submit(char *finger, char *os);
   
/*****************************************/


static void fingerprint_discard(void)
{
   struct entry *l;

   while (SLIST_FIRST(&finger_head) != NULL) {
      l = SLIST_FIRST(&finger_head);
      SLIST_REMOVE_HEAD(&finger_head, next);
      SAFE_FREE(l->os);
      SAFE_FREE(l);
   }

   DEBUG_MSG("ATEXIT: fingerprint_discard");
   
   return;
}


int fingerprint_init(void)
{
   struct entry *p;
   struct entry *last = NULL;
   
   int i;

   char line[128];
   char os[OS_LEN+1];
   char finger[FINGER_LEN+1];
   char *ptr;

   FILE *f;

   i = 0;

   f = open_data("share", TCP_FINGERPRINTS, FOPEN_READ_TEXT);
   ON_ERROR(f, NULL, "Cannot open %s", TCP_FINGERPRINTS);

   while (fgets(line, 128, f) != 0) {
      
      if ( (ptr = strchr(line, '#')) )
         *ptr = 0;

      /*  skip 0 length line */
      if (!strlen(line))  
         continue;
        
      strncpy(finger, line, FINGER_LEN);
      strncpy(os, line + FINGER_LEN + 1, OS_LEN);

      LOAD_ENTRY(p, finger, os);

      /* sort the list ascending */
      if (last == NULL)
         SLIST_INSERT_HEAD(&finger_head, p, next);
      else
         SLIST_INSERT_AFTER(last, p, next);

      /* set the last entry */
      last = p;

      /* count the fingerprints */
      i++;

   }

   DEBUG_MSG("fingerprint_init -- %d fingers loaded", i);
   USER_MSG("%4d tcp OS fingerprint\n", i);
   
   fclose(f);

   atexit(fingerprint_discard);

   return i;
}

/*
 * search in the database for a given fingerprint
 */

int fingerprint_search(const char *f, char *dst)
{
   struct entry *l;

   if (!strcmp(f, "")) {
      strcpy(dst, "UNKNOWN");
      return ESUCCESS;
   }
   
   /* if the fingerprint matches, copy it in the dst and
    * return ESUCCESS.
    * if it is not found, copy the next finger in dst 
    * and return -ENOTFOUND, it is the nearest fingerprint
    */
   
   SLIST_FOREACH(l, &finger_head, next) {
   
      /* this is exact match */
      if ( memcmp(l->finger, f, FINGER_LEN) == 0) {
         strcpy(dst, l->os);
         return ESUCCESS;
      }
      
      /* 
       * if not found seach with wildcalderd MSS 
       * but he same WINDOW size
       */
      if ( memcmp(l->finger, f, FINGER_LEN) > 0) {
         
         /* the window field is FINGER_MSS bytes */
         char win[FINGER_MSS];
         char pattern[FINGER_LEN+1];
         
         /* the is the next in the list */
         strcpy(dst, l->os);  
        
         strncpy(win, f, FINGER_MSS);
         win[FINGER_MSS-1] = '\0';
         
         /* pattern will be something like:
          *
          *  0000:*:TT:WS:0:0:0:0:F:LT
          */
         sprintf(pattern, "%s:*:%s", win, f + FINGER_TTL);

         /* search for equal WINDOW but wildcarded MSS */
         while (l != SLIST_END(&finger_head) && !strncmp(l->finger, win, 4)) {
            if (match_pattern(l->finger, pattern)) {
               /* save the nearest one (wildcarded MSS) */
               strcpy(dst, l->os); 
               return -ENOTFOUND;
            }
            l = SLIST_NEXT(l, next);
         }
         return -ENOTFOUND;
      }
   }

   return -ENOTFOUND;
}

/*
 * initialize the fingerprint string
 */

void fingerprint_default(char *finger)
{
   /* 
    * initialize the fingerprint 
    *
    * WWWW:_MSS:TT:WS:S:N:D:T:F:LT
    */
   strcpy(finger,"0000:_MSS:TT:WS:0:0:0:0:F:LT");  
}

/*
 * add a parameter to the finger string
 */

void fingerprint_push(char *finger, int param, int value)
{
   char tmp[10];
   int lt_old = 0;

   ON_ERROR(finger, NULL, "finger_push used on NULL string !!");
   
   switch (param) {
      case FINGER_WINDOW:
         snprintf(tmp, sizeof(tmp), "%04X", value);
         strncpy(finger + FINGER_WINDOW, tmp, 4);
         break;
      case FINGER_MSS:
         snprintf(tmp, sizeof(tmp), "%04X", value);
         strncpy(finger + FINGER_MSS, tmp, 4);
         break;
      case FINGER_TTL:
         snprintf(tmp, sizeof(tmp), "%02X", TTL_PREDICTOR(value));
         strncpy(finger + FINGER_TTL, tmp, 2);
         break;
      case FINGER_WS:
         snprintf(tmp, sizeof(tmp), "%02X", value);
         strncpy(finger + FINGER_WS, tmp, 2);
         break;
      case FINGER_SACK:
         snprintf(tmp, sizeof(tmp), "%d", value);
         strncpy(finger + FINGER_SACK, tmp, 1);
         break;
      case FINGER_NOP:
         snprintf(tmp, sizeof(tmp), "%d", value);
         strncpy(finger + FINGER_NOP, tmp, 1);
         break;
      case FINGER_DF:
         snprintf(tmp, sizeof(tmp), "%d", value);
         strncpy(finger + FINGER_DF, tmp, 1);
         break;
      case FINGER_TIMESTAMP:
         snprintf(tmp, sizeof(tmp), "%d", value);
         strncpy(finger + FINGER_TIMESTAMP, tmp, 1);
         break;
      case FINGER_TCPFLAG:
         if (value == 1)
            strncpy(finger + FINGER_TCPFLAG, "A", 1);
         else
            strncpy(finger + FINGER_TCPFLAG, "S", 1);
         break;
      case FINGER_LT:
         /*
          * since the LENGHT is the sum of the IP header
          * and the TCP header, we have to calculate it
          * in two steps. (decoders are unaware of other layers)
          */
         lt_old = strtoul(finger + FINGER_LT, NULL, 16);
         snprintf(tmp, sizeof(tmp), "%02X", value + lt_old);
         strncpy(finger + FINGER_LT, tmp, 2);
         break;                                 
   }
}

/*
 * round the TTL to the nearest power of 2 (ceiling)
 */

u_int8 TTL_PREDICTOR(u_int8 x)
{                            
   register u_int8 i = x;
   register u_int8 j = 1;
   register u_int8 c = 0;

   do {
      c += i & 1;
      j <<= 1;
   } while ( i >>= 1 );

   if ( c == 1 )
      return x;
   else
      return ( j ? j : 0xff );
}


/*
 * submit a fingerprint to the ettercap website
 */
int fingerprint_submit(char *finger, char *os)
{
   int sock;
   char host[] = "ettercap.sourceforge.net";
   char page[] = "/fingerprint.php";
   char getmsg[1024];
   char *os_encoded;
   size_t i;
 
   memset(getmsg, 0, sizeof(getmsg));
  
   /* some sanity checks */
   if (strlen(finger) > FINGER_LEN || strlen(os) > OS_LEN)
      return -EINVALID;
   
   USER_MSG("Connecting to http://%s...\n", host);
      
   /* prepare the socket */
   sock = open_socket(host, 80);
   
   switch(sock) {
      case -ENOADDRESS:
         FATAL_MSG("Cannot resolve %s", host);
         break;
      case -EFATAL:
         FATAL_MSG("Cannot create the socket");
         break;
      case -ETIMEOUT:
         FATAL_MSG("Connect timeout to %s on port 80", host);
         break;
      case -EINVALID:
         FATAL_MSG("Error connecting to %s on port 80", host);
         break;
   }
  
   os_encoded = strdup(os);
   /* sanitize the os (encode the ' ' to '+') */
   for (i = 0; i < strlen(os_encoded); i++)
      if (os_encoded[i] == ' ') 
         os_encoded[i] = '+';
      
   /* prepare the HTTP request */
   snprintf(getmsg, sizeof(getmsg), "GET %s?finger=%s&os=%s HTTP/1.0\r\n"
                                     "Host: %s\r\n"
                                     "User-Agent: %s (%s)\r\n"
                                     "\r\n", page, finger, os_encoded, host, GBL_PROGRAM, GBL_VERSION );
  
   SAFE_FREE(os_encoded);

   USER_MSG("Submitting the fingerprint to %s...\n", page);
   
   /* send the request to the server */
   socket_send(sock, getmsg, strlen(getmsg));

   DEBUG_MSG("fingerprint_submit - SEND \n\n%s\n\n", getmsg);

   /* ignore the server response */
   close_socket(sock);

   USER_MSG("New fingerprint submitted to the ettercap website...\n");

   return ESUCCESS;
}


/* EOF */

// vim:ts=3:expandtab

