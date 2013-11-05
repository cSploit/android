/*
 * kex.c - key exchange
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2008 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */

#include "config.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#ifndef _WIN32
#include <arpa/inet.h>
#endif

#include "libssh/priv.h"
#include "libssh/ssh2.h"
#include "libssh/ssh1.h"
#include "libssh/buffer.h"
#include "libssh/packet.h"
#include "libssh/session.h"
#include "libssh/wrapper.h"
#include "libssh/keys.h"
#include "libssh/dh.h"
#include "libssh/kex.h"
#include "libssh/string.h"

#ifdef HAVE_LIBGCRYPT
#define BLOWFISH "blowfish-cbc,"
#define AES "aes256-ctr,aes192-ctr,aes128-ctr,aes256-cbc,aes192-cbc,aes128-cbc,"
#define DES "3des-cbc"
#elif defined HAVE_LIBCRYPTO
#ifdef HAVE_OPENSSL_BLOWFISH_H
#define BLOWFISH "blowfish-cbc,"
#else
#define BLOWFISH ""
#endif
#ifdef HAVE_OPENSSL_AES_H
#ifdef BROKEN_AES_CTR
#define AES "aes256-cbc,aes192-cbc,aes128-cbc,"
#else
#define AES "aes256-ctr,aes192-ctr,aes128-ctr,aes256-cbc,aes192-cbc,aes128-cbc,"
#endif /* BROKEN_AES_CTR */
#else
#define AES ""
#endif

#define DES "3des-cbc"
#endif

#if defined(HAVE_LIBZ) && defined(WITH_LIBZ)
#define ZLIB "none,zlib,zlib@openssh.com"
#else
#define ZLIB "none"
#endif

const char *default_methods[] = {
  "diffie-hellman-group1-sha1",
  "ssh-rsa,ssh-dss",
  AES BLOWFISH DES,
  AES BLOWFISH DES,
  "hmac-sha1",
  "hmac-sha1",
  "none",
  "none",
  "",
  "",
  NULL
};

const char *supported_methods[] = {
  "diffie-hellman-group1-sha1",
  "ssh-rsa,ssh-dss",
  AES BLOWFISH DES,
  AES BLOWFISH DES,
  "hmac-sha1",
  "hmac-sha1",
  ZLIB,
  ZLIB,
  "",
  "",
  NULL
};

/* descriptions of the key exchange packet */
const char *ssh_kex_nums[] = {
  "kex algos",
  "server host key algo",
  "encryption client->server",
  "encryption server->client",
  "mac algo client->server",
  "mac algo server->client",
  "compression algo client->server",
  "compression algo server->client",
  "languages client->server",
  "languages server->client",
  NULL
};

/* tokenize will return a token of strings delimited by ",". the first element has to be freed */
static char **tokenize(const char *chain){
    char **tokens;
    int n=1;
    int i=0;
    char *tmp;
    char *ptr;

    tmp = strdup(chain);
    if (tmp == NULL) {
      return NULL;
    }
    ptr = tmp;
    while(*ptr){
        if(*ptr==','){
            n++;
            *ptr=0;
        }
        ptr++;
    }
    /* now n contains the number of tokens, the first possibly empty if the list was empty too e.g. "" */
    tokens=malloc(sizeof(char *) * (n+1) ); /* +1 for the null */
    if (tokens == NULL) {
      SAFE_FREE(tmp);
      return NULL;
    }
    ptr=tmp;
    for(i=0;i<n;i++){
        tokens[i]=ptr;
        while(*ptr)
            ptr++; // find a zero
        ptr++; // then go one step further
    }
    tokens[i]=NULL;
    return tokens;
}

/* same as tokenize(), but with spaces instead of ',' */
/* TODO FIXME rewrite me! */
char **space_tokenize(const char *chain){
    char **tokens;
    int n=1;
    int i=0;
    char *tmp;
    char *ptr;

    tmp = strdup(chain);
    if (tmp == NULL) {
      return NULL;
    }
    ptr = tmp;

    while(*ptr==' ')
        ++ptr; /* skip initial spaces */
    while(*ptr){
        if(*ptr==' '){
            n++; /* count one token per word */
            *ptr=0;
            while(*(ptr+1)==' '){ /* don't count if the tokens have more than 2 spaces */
                *(ptr++)=0;
            }
        }
        ptr++;
    }
    /* now n contains the number of tokens, the first possibly empty if the list was empty too e.g. "" */
    tokens = malloc(sizeof(char *) * (n + 1)); /* +1 for the null */
    if (tokens == NULL) {
      SAFE_FREE(tmp);
      return NULL;
    }
    ptr=tmp; /* we don't pass the initial spaces because the "tmp" pointer is needed by the caller */
                    /* function to free the tokens. */
    for(i=0;i<n;i++){
        tokens[i]=ptr;
        if(i!=n-1){
            while(*ptr)
                ptr++; // find a zero
            while(!*(ptr+1))
                ++ptr; /* if the zero is followed by other zeros, go through them */
            ptr++; // then go one step further
        }
    }
    tokens[i]=NULL;
    return tokens;
}

/* find_matching gets 2 parameters : a list of available objects (available_d), separated by colons,*/
/* and a list of preferred objects (preferred_d) */
/* it will return a strduped pointer on the first preferred object found in the available objects list */

char *ssh_find_matching(const char *available_d, const char *preferred_d){
    char ** tok_available, **tok_preferred;
    int i_avail, i_pref;
    char *ret;

    if ((available_d == NULL) || (preferred_d == NULL)) {
      return NULL; /* don't deal with null args */
    }

    tok_available = tokenize(available_d);
    if (tok_available == NULL) {
      return NULL;
    }

    tok_preferred = tokenize(preferred_d);
    if (tok_preferred == NULL) {
      SAFE_FREE(tok_available[0]);
      SAFE_FREE(tok_available);
      return NULL;
    }

    for(i_pref=0; tok_preferred[i_pref] ; ++i_pref){
      for(i_avail=0; tok_available[i_avail]; ++i_avail){
        if(!strcmp(tok_available[i_avail],tok_preferred[i_pref])){
          /* match */
          ret=strdup(tok_available[i_avail]);
          /* free the tokens */
          SAFE_FREE(tok_available[0]);
          SAFE_FREE(tok_preferred[0]);
          SAFE_FREE(tok_available);
          SAFE_FREE(tok_preferred);
          return ret;
        }
      }
    }
    SAFE_FREE(tok_available[0]);
    SAFE_FREE(tok_preferred[0]);
    SAFE_FREE(tok_available);
    SAFE_FREE(tok_preferred);
    return NULL;
}

SSH_PACKET_CALLBACK(ssh_packet_kexinit){
	int server_kex=session->server;
  ssh_string str = NULL;
  char *strings[10];
  int i;

  enter_function();
  (void)type;
  (void)user;
  memset(strings, 0, sizeof(strings));
  if(session->session_state != SSH_SESSION_STATE_INITIAL_KEX){
  	ssh_set_error(session,SSH_FATAL,"SSH_KEXINIT received in wrong state");
  	goto error;
  }
  if (server_kex) {
      if (buffer_get_data(packet,session->client_kex.cookie,16) != 16) {
        ssh_set_error(session, SSH_FATAL, "ssh_packet_kexinit: no cookie in packet");
        goto error;
      }

      if (hashbufin_add_cookie(session, session->client_kex.cookie) < 0) {
        ssh_set_error(session, SSH_FATAL, "ssh_packet_kexinit: adding cookie failed");
        goto error;
      }
  } else {
      if (buffer_get_data(packet,session->server_kex.cookie,16) != 16) {
        ssh_set_error(session, SSH_FATAL, "ssh_packet_kexinit: no cookie in packet");
        goto error;
      }

      if (hashbufin_add_cookie(session, session->server_kex.cookie) < 0) {
        ssh_set_error(session, SSH_FATAL, "ssh_packet_kexinit: adding cookie failed");
        goto error;
      }
  }

  for (i = 0; i < 10; i++) {
    str = buffer_get_ssh_string(packet);
    if (str == NULL) {
      break;
    }

    if (buffer_add_ssh_string(session->in_hashbuf, str) < 0) {
    	ssh_set_error(session, SSH_FATAL, "Error adding string in hash buffer");
      goto error;
    }

    strings[i] = ssh_string_to_char(str);
    if (strings[i] == NULL) {
    	ssh_set_error_oom(session);
      goto error;
    }
    ssh_string_free(str);
    str = NULL;
  }

  /* copy the server kex info into an array of strings */
  if (server_kex) {
    session->client_kex.methods = malloc(10 * sizeof(char **));
    if (session->client_kex.methods == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }

    for (i = 0; i < 10; i++) {
      session->client_kex.methods[i] = strings[i];
    }
  } else { /* client */
    session->server_kex.methods = malloc(10 * sizeof(char **));
    if (session->server_kex.methods == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }

    for (i = 0; i < 10; i++) {
      session->server_kex.methods[i] = strings[i];
    }
  }

  leave_function();
  session->session_state=SSH_SESSION_STATE_KEXINIT_RECEIVED;
  session->ssh_connection_callback(session);
  return SSH_PACKET_USED;
error:
  ssh_string_free(str);
  for (i = 0; i < 10; i++) {
    SAFE_FREE(strings[i]);
  }

  session->session_state = SSH_SESSION_STATE_ERROR;
  leave_function();
  return SSH_PACKET_USED;
}

void ssh_list_kex(ssh_session session, KEX *kex) {
  int i = 0;

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("session cookie", kex->cookie, 16);
#endif
  if(kex->methods==NULL){
    ssh_log(session, SSH_LOG_RARE,"kex->methods is NULL");
    return;
  }
  for(i = 0; i < 10; i++) {
    ssh_log(session, SSH_LOG_FUNCTIONS, "%s: %s",
        ssh_kex_nums[i], kex->methods[i]);
  }
}

/* set_kex basicaly look at the option structure of the session and set the output kex message */
/* it must be aware of the server kex message */
/* it can fail if option is null, not any user specified kex method matches the server one, if not any default kex matches */

int set_kex(ssh_session session){
    KEX *server = &session->server_kex;
    KEX *client=&session->client_kex;
    int i;
    const char *wanted;
    enter_function();
    ssh_get_random(client->cookie,16,0);
    client->methods=malloc(10 * sizeof(char **));
    if (client->methods == NULL) {
      ssh_set_error(session, SSH_FATAL, "No space left");
      leave_function();
      return -1;
    }
    memset(client->methods,0,10*sizeof(char **));
    for (i=0;i<10;i++){
        if(!(wanted=session->wanted_methods[i]))
            wanted=default_methods[i];
        client->methods[i]=ssh_find_matching(server->methods[i],wanted);
        if(!client->methods[i] && i < SSH_LANG_C_S){
            ssh_set_error(session,SSH_FATAL,"kex error : did not find one of algos %s in list %s for %s",
            wanted,server->methods[i],ssh_kex_nums[i]);
            leave_function();
            return -1;
        } else {
          if ((i >= SSH_LANG_C_S) && (client->methods[i] == NULL)) {
            /* we can safely do that for languages */
            client->methods[i] = strdup("");
            if (client->methods[i] == NULL) {
              return -1;
            }
          }
        }
    }
    leave_function();
    return 0;
}

/* this function only sends the predefined set of kex methods */
int ssh_send_kex(ssh_session session, int server_kex) {
  KEX *kex = (server_kex ? &session->server_kex : &session->client_kex);
  ssh_string str = NULL;
  int i;

  enter_function();

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_KEXINIT) < 0) {
    goto error;
  }
  if (buffer_add_data(session->out_buffer, kex->cookie, 16) < 0) {
    goto error;
  }

  if (hashbufout_add_cookie(session) < 0) {
    goto error;
  }

  ssh_list_kex(session, kex);

  for (i = 0; i < 10; i++) {
    str = ssh_string_from_char(kex->methods[i]);
    if (str == NULL) {
      goto error;
    }

    if (buffer_add_ssh_string(session->out_hashbuf, str) < 0) {
      goto error;
    }
    if (buffer_add_ssh_string(session->out_buffer, str) < 0) {
      goto error;
    }
    ssh_string_free(str);
  }

  if (buffer_add_u8(session->out_buffer, 0) < 0) {
    goto error;
  }
  if (buffer_add_u32(session->out_buffer, 0) < 0) {
    goto error;
  }

  if (packet_send(session) == SSH_ERROR) {
    leave_function();
    return -1;
  }

  leave_function();
  return 0;
error:
  buffer_reinit(session->out_buffer);
  buffer_reinit(session->out_hashbuf);
  ssh_string_free(str);

  leave_function();
  return -1;
}

/* returns 1 if at least one of the name algos is in the default algorithms table */
int verify_existing_algo(int algo, const char *name){
    char *ptr;
    if(algo>9 || algo <0)
        return -1;
    ptr=ssh_find_matching(supported_methods[algo],name);
    if(ptr){
        free(ptr);
        return 1;
    }
    return 0;
}

#ifdef WITH_SSH1

/* makes a STRING contating 3 strings : ssh-rsa1,e and n */
/* this is a public key in openssh's format */
static ssh_string make_rsa1_string(ssh_string e, ssh_string n){
  ssh_buffer buffer = NULL;
  ssh_string rsa = NULL;
  ssh_string ret = NULL;

  buffer = ssh_buffer_new();
  rsa = ssh_string_from_char("ssh-rsa1");

  if (buffer_add_ssh_string(buffer, rsa) < 0) {
    goto error;
  }
  if (buffer_add_ssh_string(buffer, e) < 0) {
    goto error;
  }
  if (buffer_add_ssh_string(buffer, n) < 0) {
    goto error;
  }

  ret = ssh_string_new(ssh_buffer_get_len(buffer));
  if (ret == NULL) {
    goto error;
  }

  ssh_string_fill(ret, ssh_buffer_get_begin(buffer), ssh_buffer_get_len(buffer));
error:
  ssh_buffer_free(buffer);
  ssh_string_free(rsa);

  return ret;
}

static int build_session_id1(ssh_session session, ssh_string servern,
    ssh_string hostn) {
  MD5CTX md5 = NULL;

  md5 = md5_init();
  if (md5 == NULL) {
    return -1;
  }

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("host modulus",ssh_string_data(hostn),ssh_string_len(hostn));
  ssh_print_hexa("server modulus",ssh_string_data(servern),ssh_string_len(servern));
#endif
  md5_update(md5,ssh_string_data(hostn),ssh_string_len(hostn));
  md5_update(md5,ssh_string_data(servern),ssh_string_len(servern));
  md5_update(md5,session->server_kex.cookie,8);
  md5_final(session->next_crypto->session_id,md5);
#ifdef DEBUG_CRYPTO
  ssh_print_hexa("session_id",session->next_crypto->session_id,MD5_DIGEST_LEN);
#endif

  return 0;
}

/* returns 1 if the modulus of k1 is < than the one of k2 */
static int modulus_smaller(ssh_public_key k1, ssh_public_key k2){
    bignum n1;
    bignum n2;
    int res;
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t sexp;
    sexp=gcry_sexp_find_token(k1->rsa_pub,"n",0);
    n1=gcry_sexp_nth_mpi(sexp,1,GCRYMPI_FMT_USG);
    gcry_sexp_release(sexp);
    sexp=gcry_sexp_find_token(k2->rsa_pub,"n",0);
    n2=gcry_sexp_nth_mpi(sexp,1,GCRYMPI_FMT_USG);
    gcry_sexp_release(sexp);
#elif defined HAVE_LIBCRYPTO
    n1=k1->rsa_pub->n;
    n2=k2->rsa_pub->n;
#endif
    if(bignum_cmp(n1,n2)<0)
        res=1;
    else
        res=0;
#ifdef HAVE_LIBGCRYPT
    bignum_free(n1);
    bignum_free(n2);
#endif
    return res;
    
}

#define ABS(A) ( (A)<0 ? -(A):(A) )
static ssh_string encrypt_session_key(ssh_session session, ssh_public_key srvkey,
    ssh_public_key hostkey, int slen, int hlen) {
  unsigned char buffer[32] = {0};
  int i;
  ssh_string data1 = NULL;
  ssh_string data2 = NULL;

  /* first, generate a session key */
  ssh_get_random(session->next_crypto->encryptkey, 32, 1);
  memcpy(buffer, session->next_crypto->encryptkey, 32);
  memcpy(session->next_crypto->decryptkey, session->next_crypto->encryptkey, 32);

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("session key",buffer,32);
#endif

  /* xor session key with session_id */
  for (i = 0; i < 16; i++) {
    buffer[i] ^= session->next_crypto->session_id[i];
  }
  data1 = ssh_string_new(32);
  if (data1 == NULL) {
    return NULL;
  }
  ssh_string_fill(data1, buffer, 32);
  if (ABS(hlen - slen) < 128){
    ssh_log(session, SSH_LOG_FUNCTIONS,
        "Difference between server modulus and host modulus is only %d. "
        "It's illegal and may not work",
        ABS(hlen - slen));
  }

  if (modulus_smaller(srvkey, hostkey)) {
    data2 = ssh_encrypt_rsa1(session, data1, srvkey);
    ssh_string_free(data1);
    data1 = NULL;
    if (data2 == NULL) {
      return NULL;
    }
    data1 = ssh_encrypt_rsa1(session, data2, hostkey);
    ssh_string_free(data2);
    if (data1 == NULL) {
      return NULL;
    }
  } else {
    data2 = ssh_encrypt_rsa1(session, data1, hostkey);
    ssh_string_free(data1);
    data1 = NULL;
    if (data2 == NULL) {
      return NULL;
    }
    data1 = ssh_encrypt_rsa1(session, data2, srvkey);
    ssh_string_free(data2);
    if (data1 == NULL) {
      return NULL;
    }
  }

  return data1;
}

/* SSH-1 functions */
/*    2 SSH_SMSG_PUBLIC_KEY
 *
 *    8 bytes      anti_spoofing_cookie
 *    32-bit int   server_key_bits
 *    mp-int       server_key_public_exponent
 *    mp-int       server_key_public_modulus
 *    32-bit int   host_key_bits
 *    mp-int       host_key_public_exponent
 *    mp-int       host_key_public_modulus
 *    32-bit int   protocol_flags
 *    32-bit int   supported_ciphers_mask
 *    32-bit int   supported_authentications_mask
 */
/**
 * @brief Wait for a SSH_SMSG_PUBLIC_KEY and does the key exchange
 */
SSH_PACKET_CALLBACK(ssh_packet_publickey1){
  ssh_string server_exp = NULL;
  ssh_string server_mod = NULL;
  ssh_string host_exp = NULL;
  ssh_string host_mod = NULL;
  ssh_string serverkey = NULL;
  ssh_string hostkey = NULL;
  ssh_public_key srv = NULL;
  ssh_public_key host = NULL;
  uint32_t server_bits;
  uint32_t host_bits;
  uint32_t protocol_flags;
  uint32_t supported_ciphers_mask;
  uint32_t supported_authentications_mask;
  ssh_string enc_session = NULL;
  uint16_t bits;
  int ko;
  enter_function();
  (void)type;
  (void)user;
  ssh_log(session, SSH_LOG_PROTOCOL, "Got a SSH_SMSG_PUBLIC_KEY");
  if(session->session_state != SSH_SESSION_STATE_INITIAL_KEX){
    ssh_set_error(session,SSH_FATAL,"SSH_KEXINIT received in wrong state");
    goto error;
  }
  if (buffer_get_data(packet, session->server_kex.cookie, 8) != 8) {
    ssh_set_error(session, SSH_FATAL, "Can't get cookie in buffer");
    goto error;
  }

  buffer_get_u32(packet, &server_bits);
  server_exp = buffer_get_mpint(packet);
  if (server_exp == NULL) {
    goto error;
  }
  server_mod = buffer_get_mpint(packet);
  if (server_mod == NULL) {
    goto error;
  }
  buffer_get_u32(packet, &host_bits);
  host_exp = buffer_get_mpint(packet);
  if (host_exp == NULL) {
    goto error;
  }
  host_mod = buffer_get_mpint(packet);
  if (host_mod == NULL) {
    goto error;
  }
  buffer_get_u32(packet, &protocol_flags);
  buffer_get_u32(packet, &supported_ciphers_mask);
  ko = buffer_get_u32(packet, &supported_authentications_mask);

  if ((ko != sizeof(uint32_t)) || !host_mod || !host_exp
      || !server_mod || !server_exp) {
    ssh_log(session, SSH_LOG_RARE, "Invalid SSH_SMSG_PUBLIC_KEY packet");
    ssh_set_error(session, SSH_FATAL, "Invalid SSH_SMSG_PUBLIC_KEY packet");
    goto error;
  }

  server_bits = ntohl(server_bits);
  host_bits = ntohl(host_bits);
  protocol_flags = ntohl(protocol_flags);
  supported_ciphers_mask = ntohl(supported_ciphers_mask);
  supported_authentications_mask = ntohl(supported_authentications_mask);
  ssh_log(session, SSH_LOG_PROTOCOL,
      "Server bits: %d; Host bits: %d; Protocol flags: %.8lx; "
      "Cipher mask: %.8lx; Auth mask: %.8lx",
      server_bits,
      host_bits,
      (unsigned long int) protocol_flags,
      (unsigned long int) supported_ciphers_mask,
      (unsigned long int) supported_authentications_mask);

  serverkey = make_rsa1_string(server_exp, server_mod);
  if (serverkey == NULL) {
    goto error;
  }
  hostkey = make_rsa1_string(host_exp,host_mod);
  if (serverkey == NULL) {
    goto error;
  }
  if (build_session_id1(session, server_mod, host_mod) < 0) {
    goto error;
  }

  srv = publickey_from_string(session, serverkey);
  if (srv == NULL) {
    goto error;
  }
  host = publickey_from_string(session, hostkey);
  if (host == NULL) {
    goto error;
  }

  session->next_crypto->server_pubkey = ssh_string_copy(hostkey);
  if (session->next_crypto->server_pubkey == NULL) {
    goto error;
  }
  session->next_crypto->server_pubkey_type = "ssh-rsa1";

  /* now, we must choose an encryption algo */
  /* hardcode 3des */
  if (!(supported_ciphers_mask & (1 << SSH_CIPHER_3DES))) {
    ssh_set_error(session, SSH_FATAL, "Remote server doesn't accept 3DES");
    goto error;
  }
  ssh_log(session, SSH_LOG_PROTOCOL, "Sending SSH_CMSG_SESSION_KEY");

   if (buffer_add_u8(session->out_buffer, SSH_CMSG_SESSION_KEY) < 0) {
     goto error;
   }
   if (buffer_add_u8(session->out_buffer, SSH_CIPHER_3DES) < 0) {
     goto error;
   }
   if (buffer_add_data(session->out_buffer, session->server_kex.cookie, 8) < 0) {
     goto error;
   }

   enc_session = encrypt_session_key(session, srv, host, server_bits, host_bits);
   if (enc_session == NULL) {
     goto error;
   }

   bits = ssh_string_len(enc_session) * 8 - 7;
   ssh_log(session, SSH_LOG_PROTOCOL, "%d bits, %" PRIdS " bytes encrypted session",
       bits, ssh_string_len(enc_session));
   bits = htons(bits);
   /* the encrypted mpint */
   if (buffer_add_data(session->out_buffer, &bits, sizeof(uint16_t)) < 0) {
     goto error;
   }
   if (buffer_add_data(session->out_buffer, ssh_string_data(enc_session),
         ssh_string_len(enc_session)) < 0) {
     goto error;
   }
   /* the protocol flags */
   if (buffer_add_u32(session->out_buffer, 0) < 0) {
     goto error;
   }
   session->session_state=SSH_SESSION_STATE_KEXINIT_RECEIVED;
   if (packet_send(session) == SSH_ERROR) {
     goto error;
   }

   /* we can set encryption */
   if (crypt_set_algorithms(session)) {
     goto error;
   }

   session->current_crypto = session->next_crypto;
   session->next_crypto = NULL;
   goto end;
error:
  session->session_state=SSH_SESSION_STATE_ERROR;
end:

   ssh_string_free(host_mod);
   ssh_string_free(host_exp);
   ssh_string_free(server_mod);
   ssh_string_free(server_exp);
   ssh_string_free(serverkey);
   ssh_string_free(hostkey);
   ssh_string_free(enc_session);

   publickey_free(srv);
   publickey_free(host);

   leave_function();
   return SSH_PACKET_USED;
}

int ssh_get_kex1(ssh_session session) {
  int ret=SSH_ERROR;
  enter_function();
  ssh_log(session, SSH_LOG_PROTOCOL, "Waiting for a SSH_SMSG_PUBLIC_KEY");
  /* Here the callback is called */
  while(session->session_state==SSH_SESSION_STATE_INITIAL_KEX){
    ssh_handle_packets(session, -2);
  }
  if(session->session_state==SSH_SESSION_STATE_ERROR)
    goto error;
  ssh_log(session, SSH_LOG_PROTOCOL, "Waiting for a SSH_SMSG_SUCCESS");
  /* Waiting for SSH_SMSG_SUCCESS */
  while(session->session_state==SSH_SESSION_STATE_KEXINIT_RECEIVED){
    ssh_handle_packets(session, -2);
  }
  if(session->session_state==SSH_SESSION_STATE_ERROR)
      goto error;
  ssh_log(session, SSH_LOG_PROTOCOL, "received SSH_SMSG_SUCCESS\n");
  ret=SSH_OK;
error:
  leave_function();
  return ret;
}

#endif /* WITH_SSH1 */
/* vim: set ts=2 sw=2 et cindent: */
