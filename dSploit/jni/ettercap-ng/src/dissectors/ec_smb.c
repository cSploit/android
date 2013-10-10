/*
    ettercap -- dissector smb -- TCP 139, 445

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

    $Id: ec_smb.c,v 1.15 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>

typedef struct {
   u_char  proto[4];
   u_char  cmd;
   u_char  err[4];
   u_char  flags1;
   u_short flags2;
   u_short pad[6];
   u_short tid, pid, uid, mid;
} SMB_header;

typedef struct {
   u_char  mesg;
   u_char  flags;
   u_short len;
} NetBIOS_header;

typedef struct {
   u_char challenge[8];
   u_char response1[24]; /* LM response OR cleartext password */
   u_char response2[24]; /* NTLM respones */
      #define MAX_USER_LEN       28
   u_char user[MAX_USER_LEN];
   u_char domain[MAX_USER_LEN];
   u_char status;
      #define WAITING_PLAIN_TEXT       1
      #define WAITING_CHALLENGE        2
      #define WAITING_RESPONSE         3
      #define WAITING_ENCRYPT          4
      #define WAITING_LOGON_RESPONSE   5
      #define LOGON_COMPLETED_OK       6 
      #define LOGON_COMPLETED_FAILURE  7 

   u_char auth_type;
      #define PLAIN_TEXT_AUTH          1
      #define CHALLENGE_RESPONSE_AUTH  2
      #define NTLMSSP_AUTH             3

} smb_session_data;


/* protos */

FUNC_DECODER(dissector_smb);
void smb_init(void);
char *GetUser(char *user, char *dest, int len);
void GetBinaryE(unsigned char *binary, unsigned char *dest, int len);


/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init smb_init(void)
{
   dissect_add("smb", APP_LAYER_TCP, 139, dissector_smb);
   dissect_add("smb", APP_LAYER_TCP, 445, dissector_smb);
}

FUNC_DECODER(dissector_smb)
{
   u_char *ptr;
   struct ec_session *s = NULL;
   smb_session_data *session_data;
   void *ident = NULL;
   SMB_header *smb;
   NetBIOS_header *NetBIOS;
   char tmp[MAX_ASCII_ADDR_LEN];

   ptr = PACKET->DATA.data;

   /* Catch netbios and smb headers */
   NetBIOS = (NetBIOS_header *)ptr;
   smb = (SMB_header *)(NetBIOS + 1);
   
   /* Let's go to the data */
   ptr = (u_char *)(smb + 1);

   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_smb));
   
   /* Is this a brand new session? */
   if (session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
         
      /* Check if it's smb */
      if (memcmp(smb->proto, "\xffSMB", 4) != 0)  
         return NULL;   

      /* Negotiate Protocol Response */      
      if (smb->cmd == 0x72 && FROM_SERVER("smb", PACKET)) { 
         PACKET->DISSECTOR.banner = strdup("SMB");
         /* Create the session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_smb));
         SAFE_CALLOC(s->data, 1, sizeof(smb_session_data));
         session_put(s);
         session_data = (smb_session_data *)s->data;

         /* HOOK POINT: HOOK_PROTO_SMB */
         hook_point(HOOK_PROTO_SMB, PACKET);

         /* 
          * Check the Security Mode 
          * 010 (encrypted)  000 (plaintext)
          */
         if (!(ptr[3] & 2)) {
            session_data->auth_type = PLAIN_TEXT_AUTH;    
            session_data->status = WAITING_PLAIN_TEXT;
         } else {
            ptr += (*ptr) * 2; /* Last Word Field */
	 
            /* Check Encryption Key Len */
            if (*ptr != 0) {
               ptr += 3; /* Got to Blob */
               memcpy(session_data->challenge, ptr, 8);
               session_data->status = WAITING_ENCRYPT;
               session_data->auth_type = CHALLENGE_RESPONSE_AUTH;
            } else { /* If the challenge is not in this packet */
               session_data->status = WAITING_CHALLENGE;
               session_data->auth_type = NTLMSSP_AUTH;
            }
         }
      }
      
      /* Session Setup if Negotiate Protocol has been done */      
      if (smb->cmd == 0x73 && FROM_CLIENT("smb", PACKET)) {
         ptr += ( (*ptr) * 2 + 3 );
         if ( (ptr = (char *)memmem(ptr, 128, "NTLMSSP", 8)) == NULL) 
            return NULL;
         if (ptr[8]!=1)
            return NULL; 
         /* Create the session */
         dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_smb));
         SAFE_CALLOC(s->data, 1, sizeof(smb_session_data));
         session_put(s);
         session_data = (smb_session_data *)s->data;
         session_data->status = WAITING_CHALLENGE;
         session_data->auth_type = NTLMSSP_AUTH;
	 	    
         /* HOOK POINT: HOOK_PACKET_SMB_CHL */
         hook_point(HOOK_PROTO_SMB_CHL, PACKET);
      } 
   } else {
      SAFE_FREE(ident);
      session_data = (smb_session_data *)s->data;
      if (FROM_CLIENT("smb", PACKET)) {	 
         if (session_data->status == LOGON_COMPLETED_OK || session_data->status == LOGON_COMPLETED_FAILURE) {
            
            /* Logon negotiation completed. It's time to dump passwords */
            PACKET->DISSECTOR.user = strdup(session_data->user);
	    
            /* We get domain information */
            if (session_data->domain[0] != 0) {
               SAFE_CALLOC(PACKET->DISSECTOR.info, MAX_USER_LEN + 16, sizeof(char));
               snprintf(PACKET->DISSECTOR.info, MAX_USER_LEN + 16, "DOMAIN: %s", session_data->domain);
            }
	    	    
            if (session_data->auth_type == PLAIN_TEXT_AUTH) {
               PACKET->DISSECTOR.pass = strdup(session_data->response1);
               DISSECT_MSG("SMB : %s:%d -> USER: %s  PASS: %s ", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                          ntohs(PACKET->L4.dst),
                                                          PACKET->DISSECTOR.user,
                                                          PACKET->DISSECTOR.pass);
            } else {
               char ascii_hash[256];

               sprintf(ascii_hash, "%s:\"\":\"\":",session_data->user);  
               GetBinaryE(session_data->response1, ascii_hash, 24);
               strcat(ascii_hash, ":");
               GetBinaryE(session_data->response2, ascii_hash, 24);
               strcat(ascii_hash, ":");
               GetBinaryE(session_data->challenge, ascii_hash, 8);
	       
               PACKET->DISSECTOR.pass = strdup(ascii_hash);  
               
               DISSECT_MSG("SMB : %s:%d -> USER: %s  HASH: %s", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                          ntohs(PACKET->L4.dst),
                                                          PACKET->DISSECTOR.user,
                                                          PACKET->DISSECTOR.pass);
            }

            if (PACKET->DISSECTOR.info!=NULL)
               DISSECT_MSG(" %s", PACKET->DISSECTOR.info);
	    
            if (session_data->status == LOGON_COMPLETED_FAILURE) {
               PACKET->DISSECTOR.failed = 1;
               DISSECT_MSG(" (Login Failed)\n");
            } else
               DISSECT_MSG("\n");

            /* Catch multiple retries for NTLMSSP */
            if (session_data->auth_type == NTLMSSP_AUTH && session_data->status == LOGON_COMPLETED_FAILURE)
               session_data->status = WAITING_CHALLENGE;
            else 
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_smb));
	       
            return NULL;	    
         }
         /* Check if it's smb */
         if (memcmp(smb->proto, "\xffSMB", 4) != 0)  
            return NULL;   
 
         if (smb->cmd == 0x73) { /* Session SetUp Packets */ 
            if (session_data->status == WAITING_PLAIN_TEXT) {
               u_int16 pwlen, unilen;
               char *Blob;
	 
               Blob = ptr;
               Blob += ( (*ptr) * 2 + 3 );
        
               ptr += 15;
               pwlen = phtos(ptr);        /* ANSI password len */
               ptr += 2;
               unilen = phtos(ptr);       /* UNICODE password len */
         
               if (pwlen > 1) 
                  memcpy(session_data->response1, Blob, sizeof(session_data->response1) - 1);
               else
                  sprintf(session_data->response1, "(empty)");
		  	 
               Blob = GetUser(Blob+pwlen+unilen, session_data->user, 200);
               GetUser(Blob, session_data->domain, 200);
               session_data->status = WAITING_LOGON_RESPONSE;
            } else if (session_data->status == WAITING_CHALLENGE) {
	    
               /* HOOK POINT: HOOK_PACKET_SMB_CHL */
               hook_point(HOOK_PROTO_SMB_CHL, PACKET);
              
            } else if (session_data->status == WAITING_RESPONSE) {
               char *Blob;
               /* Jump the Words */
               ptr += ( (*ptr) * 2 + 3 );
	 
               /* Jump ID String */
               if ( (ptr = (char *)memmem(ptr, 128, "NTLMSSP", 8)) == NULL)  
                  return NULL; 
		     
               Blob = ptr;
               ptr = strchr(ptr, 0);
               ptr++;
	 
               if (*ptr == 3) { /* Msg Type AUTH */ 
                  int LM_Offset, LM_Len, NT_Offset, NT_Len, Domain_Offset, Domain_Len, User_Offset, User_Len;
                  ptr += 4;
                  LM_Len = *(u_int16 *)ptr;
                  ptr += 4;
                  LM_Offset = *(u_int32 *)ptr;
                  ptr += 4;
                  NT_Len = *(u_int16 *)ptr;
                  ptr += 4;
                  NT_Offset = *(u_int32 *)ptr;
                  ptr += 4;
                  Domain_Len = *(u_int16 *)ptr;
                  ptr += 4;
                  Domain_Offset = *(u_int32 *)ptr;
                  ptr += 4;
                  User_Len = *(u_int16 *)ptr;
                  ptr += 4;
                  User_Offset = *(u_int32 *)ptr;
	
                  if (NT_Len != 24) {
                     session_data->status = WAITING_CHALLENGE;     
                     return NULL;
                  }
		  
                  GetUser(Blob+User_Offset, session_data->user, User_Len);
                  GetUser(Blob+Domain_Offset, session_data->domain, Domain_Len);
		 
                  if (LM_Len == 24) 
                     memcpy(session_data->response1, Blob+LM_Offset, 24);
 
                  memcpy(session_data->response2, Blob+NT_Offset, 24);
                  session_data->status = WAITING_LOGON_RESPONSE;
               }
            } else if (session_data->status == WAITING_ENCRYPT) {
               char *Blob;
	 
               Blob = ptr;
               Blob += ( (*ptr) * 2 + 3 );

               memcpy(session_data->response1, Blob, 24);
               memcpy(session_data->response2, Blob + 24, 24);
               Blob = GetUser(Blob + 48, session_data->user, 200);
               /* Get the domain with GetUser :) */
               GetUser(Blob, session_data->domain, 200);
	       
               /* XXX - 5-minutes workaround for empty sessions */
               if ( !memcmp(session_data->response1, "\x00\x00\x00", 3) && 
                    memcmp(session_data->response1, "\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00", 24) ) {
                    memset(session_data->response1, 0, 24);
                    memset(session_data->response2, 0, 24);
                    strcpy(session_data->user, "(empty)");
                    session_data->domain[0]=0;		    		    
               }
	       
               session_data->status = WAITING_LOGON_RESPONSE;
            }   
         }
      } else { /* Packets coming from the server */ 
         if (smb->cmd == 0x73) { /* Session SetUp packets */
            if (session_data->status == WAITING_CHALLENGE) {
               ptr += ( (*ptr) * 2 + 3 );
               if ( (ptr = (char *)memmem(ptr, 128, "NTLMSSP", 8)) == NULL) 
                  return NULL;
               ptr = strchr(ptr, 0);
               ptr++;
	  
               if (*ptr == 2) {
                  ptr += 16; 
                  memcpy(session_data->challenge, ptr, 8);	   
                  session_data->status = WAITING_RESPONSE;
               }	     
            } else if (session_data->status == WAITING_LOGON_RESPONSE) {
               if(!memcmp(smb->err, "\x00\x00\x00\x00", 4))
                  session_data->status = LOGON_COMPLETED_OK;
               else	    
                  session_data->status = LOGON_COMPLETED_FAILURE;
            }
         }     
      }  
   }
   
   return NULL;
}      


/* 
 * If the username is Unicode convert into ASCII 
 * XXX - what about strange unicode charsets???
 */
char *GetUser(char *user, char *dest, int len)
{
   char Unicode = 1;
   int i = 0;

   /* Skip 0 padding for odd unicode chars */
   if (user[0] == 0) 
      user++;
        
   /* Does anyone uses 2 chars users? I think it's unicode */
   if (user[1] == 0) 
      Unicode = 2; 
                            
   while(*user != 0 && i < (MAX_USER_LEN - 1) && len > 0) {
      *dest = *user;
      dest++; 
      i++;
      len -= Unicode;
      user += Unicode;
   }
   if (*user == 0)
      user += Unicode;
      
   *dest = 0;
   return user;
}

/* Convert and attach a binary array to ASCII chars */
void GetBinaryE(unsigned char *binary, unsigned char *dest, int blen)
{
   char dummy[5];
   
   for (; blen > 0; blen--) {	
       sprintf( dummy, "%02X", *binary);
       binary++;
       strcat(dest, dummy);
   }
}

/* EOF */

// vim:ts=3:expandtab
