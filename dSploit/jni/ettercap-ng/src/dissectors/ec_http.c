/*
    ettercap -- dissector http and proxy -- TCP 80, 8080

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

    $Id: ec_http.c,v 1.20 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_file.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_sslwrap.h>

/* globals */
#define USER 0
#define PASS 1

struct http_status {
   u_char c_status;
#define POST_WAIT_DELIMITER 1
#define POST_LAST_CHANCE 2
#define NTLM_WAIT_RESPONSE 3
#define PROXY_WAIT_OK 4
#define PROXY_WAIT_DELIMITER 5
   u_char c_data[150];
/* XXX - Manage this array dinamically (with session_destroyer) */
};

typedef struct {
   u_int16 len;
   u_int16 maxlen;
   u_int32 offset;
}tSmbStrHeader;

typedef struct {
   char 	ident[8];
   u_int32	msgType;
}tSmbStdHeader;

typedef struct {
   char			ident[8];
   u_int32		msgType;
   tSmbStrHeader	uDomain;
   u_int32		flags;
   u_int8		challengeData[8];
   u_int8		reserved[8];
   tSmbStrHeader	emptyString;
   u_int8		buffer[1024];
   u_int32		bufIndex;
}tSmbNtlmAuthChallenge;

typedef struct {
    char 		ident[8];
    u_int32		msgType;
    tSmbStrHeader	lmResponse;
    tSmbStrHeader	ntResponse;
    tSmbStrHeader	uDomain;
    tSmbStrHeader	uUser;
    tSmbStrHeader	uWks;
    tSmbStrHeader	sessionKey;
    u_int32		flags;
    u_int8		buffer[1024];
    u_int32		bufIndex;
}tSmbNtlmAuthResponse;

struct http_field_entry {
   char *name;
   SLIST_ENTRY(http_field_entry) next;
};

static SLIST_HEAD(, http_field_entry) http_fields[2];


/* protos */
FUNC_DECODER(dissector_http);
void http_init(void);
static void Parse_Method_Get(u_char *ptr, struct packet_object *po);
static void Parse_Method_Post(u_char *ptr, struct packet_object *po);
static void Decode_Url(u_char *src);
static int Check_CONNECT(u_char *ptr, struct packet_object *po);
static void Find_Url(u_char *to_parse, char **ret);
static void Find_Url_Referer(u_char *to_parse, char **ret);
static void Parse_Post_Payload(u_char *ptr, struct http_status *conn_status, struct packet_object *po);
static void Print_Pass(struct packet_object *po);
static void Get_Banner(u_char *ptr, struct packet_object *po);
static u_char Parse_Form(u_char *to_parse, char **ret, int mode);
static int Parse_Passport_Auth(char *ptr, char *from_here, struct packet_object *po);
static int Parse_NTLM_Auth(char *ptr, char *from_here, struct packet_object *po);
static int Parse_Basic_Auth(char *ptr, char *from_here, struct packet_object *po);
static char *unicodeToString(char *p, size_t len);
static void dumpRaw(char *str, unsigned char *buf, size_t len);
int http_fields_init(void);

#define CVAL(buf,pos) (((unsigned char *)(buf))[pos])
#define PVAL(buf,pos) ((unsigned)CVAL(buf,pos))
#define SVAL(buf,pos) (PVAL(buf,pos)|PVAL(buf,(pos)+1)<<8)
#define IVAL(buf,pos) (SVAL(buf,pos)|SVAL(buf,(pos)+2)<<16)

#define GetUnicodeString(structPtr, header) unicodeToString(((char*)structPtr) + IVAL(&structPtr->header.offset,0) , SVAL(&structPtr->header.len,0)/2)

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init http_init(void)
{
   sslw_dissect_add("https", 443, dissector_http, SSL_ENABLED);
   sslw_dissect_add("proxy", 8080, dissector_http, SSL_DISABLED);
   dissect_add("http", APP_LAYER_TCP, 80, dissector_http);
}


FUNC_DECODER(dissector_http)
{
   DECLARE_DISP_PTR_END(ptr, end);
   struct ec_session *s = NULL;
   void *ident = NULL;
   struct http_status *conn_status;
   char *from_here;

   /* unused variable */
   (void)end;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;

   /* XXX - This way we won't catch ProxyAuth on CONNECT */
   if (Check_CONNECT(ptr, PACKET))
      return NULL;         

   /* HOOK POINT: HOOK_PROTO_HTTP */
   hook_point(HOOK_PROTO_HTTP, PACKET);
   
   /* Parse client requests.
    * Check the request type first. 
    */
   if (FROM_CLIENT("http", PACKET) || FROM_CLIENT("proxy", PACKET) || FROM_CLIENT("https", PACKET)) {
      /* Check Proxy or WWW auth first
       * then password in the GET or POST.
       */
      if ((from_here = strstr(ptr, "Authorization: Passport")) && 
         Parse_Passport_Auth(ptr, from_here + strlen("Authorization: Passport"), PACKET));       
      else if ((from_here = strstr(ptr, ": NTLM ")) && 
         Parse_NTLM_Auth(ptr, from_here + strlen(": NTLM "), PACKET));
      else if ((from_here = strstr(ptr, ": Basic ")) &&
         Parse_Basic_Auth(ptr, from_here  + strlen(": Basic "), PACKET));
      else if (!strncmp(ptr, "GET ", 4))
         Parse_Method_Get(ptr + strlen("GET "), PACKET);
      else if (!strncmp(ptr, "POST ", 5))
         Parse_Method_Post(ptr + strlen("POST "), PACKET);
      else {
         dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_http));
         if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
            conn_status = (struct http_status *) s->data;
	 
            /* Are we waiting for post termination? */
            if (conn_status->c_status == POST_WAIT_DELIMITER ||
                conn_status->c_status == POST_LAST_CHANCE)
               Parse_Post_Payload(ptr, conn_status, PACKET);
         }
         SAFE_FREE(ident);
      } 	 
   } else { /* Server Replies */
      if (!strncmp(ptr, "HTTP", 4)) {
         Get_Banner(ptr, PACKET);

         /* Since the server replies there's no need to
          * wait for POST termination or client response
          */
         dissect_wipe_session(PACKET, DISSECT_CODE(dissector_http));

         /* Check Proxy or WWW Auth (server challenge) */
         /* XXX - Is the NTLM challenge always in the same 
          * packet as HTTP header? Otherwise put these lines
          * out from the if (decrease performances, checks all pcks)
          */
         if ((from_here = strstr(ptr, ": NTLM "))) 
            Parse_NTLM_Auth(ptr, from_here + strlen(": NTLM "), PACKET);
      }
   }

   return NULL;
}

/* Set the SSL flag (for ssl wrapper) when the CONNECT is finished */
static int Check_CONNECT(u_char *ptr, struct packet_object *po)
{
   void *ident = NULL;
   struct ec_session *s = NULL;
   struct http_status *conn_status = NULL;

   /* If we don't activate SSL wrappers we don't need to trace CONNECT */
   if (!GBL_CONF->aggressive_dissectors || GBL_OPTIONS->unoffensive || GBL_OPTIONS->read)
      return 0;
      
   dissect_create_ident(&ident, po, DISSECT_CODE(dissector_http));
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
      conn_status = (struct http_status *) s->data;

      if (FROM_SERVER("proxy", po)) {
         if (conn_status->c_status == PROXY_WAIT_OK && !strncmp(ptr + 8, " 200 ", 5)) 
            conn_status->c_status = PROXY_WAIT_DELIMITER;

         if (conn_status->c_status == PROXY_WAIT_DELIMITER && 
            (strstr(ptr, "\r\n\r\n") || (ptr[0]=='\r' && ptr[1]=='\n'))) {
            dissect_wipe_session(po, DISSECT_CODE(dissector_http));
            po->flags |= PO_SSLSTART;
            SAFE_FREE(ident);
            return 1;
         }
      }      
   } else if (FROM_CLIENT("proxy", po) && !strncmp(ptr, "CONNECT ", 8)) {
      dissect_create_session(&s, po, DISSECT_CODE(dissector_http));
      SAFE_CALLOC(s->data, 1, sizeof(struct http_status));                  
      conn_status = (struct http_status *) s->data;
      conn_status->c_status = PROXY_WAIT_OK;
      session_put(s);
   }

   SAFE_FREE(ident);
   if (conn_status && (conn_status->c_status == PROXY_WAIT_DELIMITER || conn_status->c_status == PROXY_WAIT_OK))  
      return 1;
      
   return 0;
}

/* Get the server banner from the headers */       
static void Get_Banner(u_char *ptr, struct packet_object *po)
{
   char *start, *end;
   u_int32 len;
   
   /* This is the banner of the remote 
    * server and not of the proxy
    */
    DEBUG_MSG("http - GET BANNER");
    
   if (FROM_SERVER("proxy", po))
      po->DISSECTOR.banner=strdup("Proxy");
   else {
      /* Get the server version */
      if ((start = strstr(ptr, "Server: ")) && (end = strstr(start, "\r"))) {
         start += strlen("Server: ");
         len = end - start;
	      
         if (len>0 && len<1024) {
            SAFE_CALLOC(po->DISSECTOR.banner, len+1, sizeof(char));
            memcpy(po->DISSECTOR.banner, start, len);
         }
      }
   }
}


/* Parse Passport Authentication */ 
static int Parse_Passport_Auth(char *ptr, char *from_here, struct packet_object *po)
{
   char *token, *to_decode, *tok;

   DEBUG_MSG("HTTP --> dissector http (Passport Auth)");

   if (!(to_decode = strdup(from_here)))
      return 1;

   if ( (token = strstr(to_decode, "OrgURL=")) == NULL ) {
      SAFE_FREE(to_decode);
      return 1;
   }
   
   /* Catch the original URL */
   ec_strtok(token, ",", &tok);
   po->DISSECTOR.info = strdup(token + strlen("OrgURL="));
   Decode_Url(po->DISSECTOR.info);
   
   /* Catch user and password */
   while ( (token = ec_strtok(NULL, ",", &tok)) != NULL ) {
      if (!strncmp(token, "sign-in=", strlen("sign-in="))) {
         po->DISSECTOR.user = strdup(token + strlen("sign-in="));
         Decode_Url(po->DISSECTOR.user);
      } else if (!strncmp(token, "pwd=", strlen("pwd="))) {
         po->DISSECTOR.pass = strdup(token + strlen("pwd="));
         Decode_Url(po->DISSECTOR.pass);
         /* password is the last interesting field */
         break;
      }
   }

   Print_Pass(po);
   SAFE_FREE(to_decode);
   return 1;      
}


/* Parse Basic Authentication for both Proxy and WWW Auth */ 
static int Parse_Basic_Auth(char *ptr, char *from_here, struct packet_object *po)
{
   int Proxy_Auth = 0;
   char *token, *to_decode, *tok;

   DEBUG_MSG("HTTP --> dissector http (Basic Auth)");

   /* If it's a proxy auth and we are not interested on proxy stuff
    * return 0, so the dissector will continue to parse GET and POST
    */
   /* It stands for both Proxy-Authenticate and Authorization ;) */    
   if (strstr(ptr, "Proxy-Auth") || strstr(ptr, "Proxy-auth")) {
      if (FROM_CLIENT("proxy", po))
         Proxy_Auth = 1;
      else
         return 0;
   }

   if (!(to_decode = strdup(from_here)))
      return 1;
       
   ec_strtok(to_decode, "\r", &tok);
   base64_decode(to_decode, to_decode);
   
   /* Parse the cleartext auth string */
   if ( (token = strsep(&to_decode, ":")) != NULL) {
      po->DISSECTOR.user = strdup(token);
      if ( (token = strsep(&to_decode, ":")) != NULL) {
         po->DISSECTOR.pass = strdup(token);
      
         /* Are we authenticating to the proxy or to a website? */
         if (Proxy_Auth)
            po->DISSECTOR.info = strdup("Proxy Authentication");
         else 
            Find_Url(ptr, &(po->DISSECTOR.info));
	    
         Print_Pass(po);
      }
   }

   SAFE_FREE(to_decode);
   return 1;
}

/* Parse NTLM challenge and response for both Proxy and WWW Auth */ 
static int Parse_NTLM_Auth(char *ptr, char *from_here, struct packet_object *po)
{
   char *to_decode, msgType, *tok; 
   tSmbStdHeader *hSmb;
   int Proxy_Auth = 0;
   void *ident = NULL;
   struct ec_session *s = NULL;
   struct http_status *conn_status;
       
   DEBUG_MSG("HTTP --> dissector http (NTLM Auth)");
   
   /* If it's a proxy auth and we are not interested on proxy stuff
    * return 0, so the dissector will continue to parse GET and POST
    */
   /* It stands for both Proxy-Authenticate and Authorization ;) */    
   if (strstr(ptr, "Proxy-Auth") || strstr(ptr, "Proxy-auth")) {
      if (FROM_CLIENT("proxy", po) || FROM_SERVER("proxy", po))
         Proxy_Auth = 1;
      else
         return 0;
   }
   
   if (!(to_decode = strdup(from_here)))
      return 1;
       
   ec_strtok(to_decode, "\r", &tok);

   base64_decode(to_decode, to_decode);
   hSmb = (tSmbStdHeader *) to_decode;
   msgType = IVAL(&hSmb->msgType, 0);

   /* msgType 2 -> Server challenge
    * msgType 3 -> Client response
    */    
   if (msgType==2) {
      tSmbNtlmAuthChallenge *challenge_struct;

      challenge_struct = (tSmbNtlmAuthChallenge *) to_decode;
      
      /* Create a session to remember the server challenge */
      dissect_create_session(&s, po, DISSECT_CODE(dissector_http));
      SAFE_CALLOC(s->data, 1, sizeof(struct http_status));                  
      conn_status = (struct http_status *) s->data;
      conn_status->c_status = NTLM_WAIT_RESPONSE;
      dumpRaw(conn_status->c_data, challenge_struct->challengeData, 8);
      session_put(s);

   } else if (msgType==3) {   
      tSmbNtlmAuthResponse  *response_struct;
      char *outstr;
      
      /* Take the challenge from the session */
      dissect_create_ident(&ident, po, DISSECT_CODE(dissector_http));
      if (session_get_and_del(&s, ident, DISSECT_IDENT_LEN) == ESUCCESS) {
         conn_status = (struct http_status *) s->data;
	 
         /* Are we waiting for client response? */
         /* XXX- POST Continuation may conflict with NTLM Proxy auth
          * if the client doesn't send Proxy-Authorization in the same 
          * packet as the POST
          */  
         if (conn_status->c_status == NTLM_WAIT_RESPONSE) {
            /* Fill the user and passwords */
            response_struct  = (tSmbNtlmAuthResponse *) to_decode;
            po->DISSECTOR.user = strdup(GetUnicodeString(response_struct, uUser));
            SAFE_CALLOC(po->DISSECTOR.pass, strlen(po->DISSECTOR.user) + 150, sizeof(char));
            sprintf(po->DISSECTOR.pass, "(NTLM) %s:\"\":\"\":", po->DISSECTOR.user);
            outstr = po->DISSECTOR.pass + strlen(po->DISSECTOR.pass);
            dumpRaw(outstr,((unsigned char*)response_struct)+IVAL(&response_struct->lmResponse.offset,0), 24);	    	 
            outstr[48] = ':';
            outstr+=49;
            dumpRaw(outstr,((unsigned char*)response_struct)+IVAL(&response_struct->ntResponse.offset,0), 24);	       	    
            outstr[48] = ':';
            outstr += 49;
            strcat(po->DISSECTOR.pass, conn_status->c_data);

            /* Are we authenticating to the proxy or to a website? */
            if (Proxy_Auth)
               po->DISSECTOR.info = strdup("Proxy Authentication");
            else 
               Find_Url(ptr, &(po->DISSECTOR.info));
	    
            Print_Pass(po);
         }
         session_free(s);
      }
      SAFE_FREE(ident);
   }
   SAFE_FREE(to_decode);
   return 1;
}


/* Deal with POST continuation */
static void Parse_Post_Payload(u_char *ptr, struct http_status *conn_status, struct packet_object *po)
{ 
   char *user=NULL, *pass=NULL;

   DEBUG_MSG("HTTP - Parse First chance");
   
   if (conn_status->c_status == POST_WAIT_DELIMITER)
      if ((ptr = strstr(ptr, "\r\n\r\n"))) { 
         ptr+=4;
         conn_status->c_status = POST_LAST_CHANCE;
      }
   DEBUG_MSG("HTTP - Parse Last chance");
   if (conn_status->c_status == POST_LAST_CHANCE) {
   DEBUG_MSG("HTTP - Parse Form");

      if (Parse_Form(ptr, &user, USER) && Parse_Form(ptr, &pass, PASS)) {
         po->DISSECTOR.user = user;
         po->DISSECTOR.pass = pass;
         po->DISSECTOR.info = strdup(conn_status->c_data);
         dissect_wipe_session(po, DISSECT_CODE(dissector_http));
         Print_Pass(po);
      } else
         SAFE_FREE(user);
   }
}


/* Parse the POST header */
static void Parse_Method_Post(u_char *ptr, struct packet_object *po) 
{
   char *url = NULL;
   struct ec_session *s = NULL;
   struct http_status *conn_status;
     
   DEBUG_MSG("HTTP --> dissector http (method POST)");
  
   Find_Url_Referer(ptr, &url);
   
   /* We create a session just in case the post was 
    * fragmented into more packets. The session will be
    * wiped on HTTP server reply.
    */
   dissect_create_session(&s, po, DISSECT_CODE(dissector_http));
   SAFE_CALLOC(s->data, 1, sizeof(struct http_status));                  
   conn_status = (struct http_status *) s->data;
   conn_status->c_status = POST_WAIT_DELIMITER;
   strlcpy(conn_status->c_data, url, sizeof(conn_status->c_data));
   session_put(s);

   Parse_Post_Payload(ptr, conn_status, po);
   
   SAFE_FREE(url);
}


/* Search for passwords in the URL */
static void Parse_Method_Get(u_char *ptr, struct packet_object *po) 
{
   u_char *to_parse = NULL;
   u_char *delimiter = NULL;
   char *user = NULL;
   char *pass = NULL;
   
   DEBUG_MSG("HTTP --> dissector http (method GET)");

   /* Isolate the parameters and copy them into another string */
   if (!(to_parse = strstr(ptr, "?")))
      return;
      
   if (!(to_parse = strdup(to_parse)))
      return;      
      
   if (!(delimiter = strstr(to_parse, " HTTP"))) 
      goto http_get_failure;
   
   /* NULL terminate the newly created parameter string */
   *delimiter = 0;
   
   /* Let's parse the parameter list */
   if (!Parse_Form(to_parse, &user, USER) || !Parse_Form(to_parse, &pass, PASS)) {
      SAFE_FREE(user);
      goto http_get_failure;
   }

   po->DISSECTOR.user = user;
   po->DISSECTOR.pass = pass;

   /* Fill the info with the URL */
   Find_Url_Referer(ptr, &(po->DISSECTOR.info));
   
   Print_Pass(po);
   
http_get_failure:   
   SAFE_FREE(to_parse);
}


/* Match users or passwords in a string */
static u_char Parse_Form(u_char *to_parse, char **ret, int mode)
{
   u_char *q;
   struct http_field_entry *d;

   /* Strip the '?' from a GET method */
   if (*to_parse == '?') to_parse++;    
   if (*to_parse == 0) 
      return 0;

   /* Search for users or passwords */   

   /* Search matches between each parameter and 
    * recognized users and passwords 
    */      

   SLIST_FOREACH(d, &(http_fields[mode]), next) {
      q = to_parse;  
      do {
         if (*q == '&') q++;
         
         if (!strncasecmp(q, d->name, strlen(d->name)) && *(q+strlen(d->name)) == '=' ) {

            /* Return the value past the '=' */
            if (!(*ret = strdup(q + strlen(d->name) + 1)))
               return 0;

            /* NULL terminate the value if it's not the last */
            if ((q = strchr(*ret, '&')))
               *q = 0; 

            Decode_Url((u_char *)*ret);
            return 1;
         }
      } while ( (q = strchr(q, '&')) );
   }
   
   return 0;
}


/* Unescape the string */
static void Decode_Url(u_char *src)
{
   u_char t[3];
   u_int32 i, j, ch;

   /* Paranoid test */
   if (!src)
      return;
      
   /* NULL terminate for the strtoul */
   t[3] = 0;
   
   for (i=0, j=0; src[i] != 0; i++, j++) {
      ch = (u_int32)src[i];
      if (ch == '%' && isxdigit((u_int32)src[i + 1]) && isxdigit((u_int32)src[i + 2])) {
         memcpy(t, src+i+1, 2);
         ch = strtoul(t, NULL, 16);
         i += 2;
      }
      src[j] = (u_char)ch;
   }
   src[j] = 0;
}


/* Gets the URL from the headers */
static void Find_Url_Referer(u_char *to_parse, char **ret) 
{
   u_char *fromhere, *page=NULL, *host=NULL;     
   u_int32 len;
   char *tok;

   /* If the referer exists */
   if ((fromhere = strstr(to_parse, "Referer: "))) {
      if ((*ret = strdup(fromhere + strlen("Referer: "))))
         ec_strtok(*ret, "\r", &tok);
   } else {
      /* Get the page from the request */
      page = strdup(to_parse);
      ec_strtok(page, " HTTP", &tok);

      /* If the path is relative, search for the Host */
      if ((*page=='/') && (fromhere = strstr(to_parse, "Host: "))) {
         host = strdup( fromhere + strlen("Host: ") );
         ec_strtok(host, "\r", &tok);
      } else 
         host = strdup("");
	 
      len = strlen(page) + strlen(host) + 2;
      SAFE_CALLOC(*ret, len, sizeof(char));
      sprintf(*ret, "%s%s", host, page);

      SAFE_FREE(page);
      SAFE_FREE(host);            
   }    
   
   Decode_Url((u_char *)*ret);
}


/* Gets the URL from the request */
static void Find_Url(u_char *to_parse, char **ret) 
{
   u_char *fromhere, *page=NULL, *host=NULL;     
   u_int32 len;
   char *tok;

   if (!strncmp(to_parse, "GET ", 4))
      to_parse += strlen("GET ");
   else if (!strncmp(to_parse, "POST ", 5))
      to_parse += strlen("POST ");
   else 
      return; 
      
   /* Get the page from the request */
   page = strdup(to_parse);
   ec_strtok(page, " HTTP", &tok);

   /* If the path is relative, search for the Host */
   if ((*page=='/') && (fromhere = strstr(to_parse, "Host: "))) {
      host = strdup( fromhere + strlen("Host: ") );
      ec_strtok(host, "\r", &tok);
   } else 
      host = strdup("");
	 
   len = strlen(page) + strlen(host) + 2;
   SAFE_CALLOC(*ret, len, sizeof(char));
   sprintf(*ret, "%s%s", host, page);

   SAFE_FREE(page);
   SAFE_FREE(host);            
       
   Decode_Url((u_char *)*ret);
}

/* Print the passwords from the PO */
static void Print_Pass(struct packet_object *po)
{
   char tmp[MAX_ASCII_ADDR_LEN];
   
   if (!po->DISSECTOR.user)
      po->DISSECTOR.user = strdup("");

   if (!po->DISSECTOR.pass)
      po->DISSECTOR.pass = strdup("");

   DISSECT_MSG("HTTP : %s:%d -> USER: %s  PASS: %s  INFO: %s\n", ip_addr_ntoa(&po->L3.dst, tmp),
                                                                 ntohs(po->L4.dst), 
                                                                 po->DISSECTOR.user,
                                                                 po->DISSECTOR.pass,
                                                                 po->DISSECTOR.info);
}


/* Load known user/pass fields from file */
int http_fields_init(void)
{
   FILE *f;
   struct http_field_entry *d;
   char line[128];
   char *ptr;
   int pass_flag = USER;

   /* open the file */
   f = open_data("share", ETTER_FIELDS, FOPEN_READ_TEXT);
   if (f == NULL) {
      USER_MSG("Cannot open %s", ETTER_FIELDS);
      return -EINVALID;
   }
         
   /* load it in the list */
   while (fgets(line, 128, f)) {
    
      /* trim comments */
      if ( (ptr = strchr(line, '#')) )
         *ptr = 0;

      /* trim \n */
      if ( (ptr = strchr(line, '\n')) )
         *ptr = 0;

      /* trim ' ' */
      if ( (ptr = strchr(line, ' ')) )
         *ptr = 0;

      /* skip empty lines */
      if (!strlen(line))
         continue;

      /* Identify the section */
      if(!strncmp(line, "[USER]", 6)) {
         pass_flag = USER;
         continue;
      }

      if(!strncmp(line, "[PASS]", 6)) {
         pass_flag = PASS;
         continue;
      }
       
      /* create the entry */
      SAFE_CALLOC(d, 1, sizeof(struct http_field_entry));
      d->name = strdup(line);

      /* insert in the right list */
      SLIST_INSERT_HEAD(&(http_fields[pass_flag]), d, next);
   }
   
   fclose(f);

   return ESUCCESS;
}


/* A little helper function */
static void dumpRaw(char *str, unsigned char *buf, size_t len)
{
   u_int32 i;

   for (i=0; i<len; ++i, str+=2)
      sprintf(str, "%02x", buf[i]);
}

/* A little helper function */
static char *unicodeToString(char *p, size_t len)
{
   u_int32 i;
   static char buf[1024];

   /* A string longer than 1024 chars???...it's a bougs packet */
   for (i=0; i<len && i<1023; ++i) {
      buf[i] = *p & 0x7f;
      p += 2;
   }
   buf[i] = '\0';
   return buf;
}

/* EOF */

// vim:ts=3:expandtab
