/*
 * wrapper.c - wrapper for crytpo functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003      by Aris Adamantiadis
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

/*
 * Why a wrapper?
 *
 * Let's say you want to port libssh from libcrypto of openssl to libfoo
 * you are going to spend hours to remove every references to SHA1_Update()
 * to libfoo_sha1_update after the work is finished, you're going to have
 * only this file to modify it's not needed to say that your modifications
 * are welcome.
 */

#include "config.h"


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#ifdef WITH_LIBZ
#include <zlib.h>
#endif

#include "libssh/priv.h"
#include "libssh/session.h"
#include "libssh/crypto.h"
#include "libssh/wrapper.h"

/* it allocates a new cipher structure based on its offset into the global table */
static struct crypto_struct *cipher_new(int offset) {
  struct crypto_struct *cipher = NULL;

  cipher = malloc(sizeof(struct crypto_struct));
  if (cipher == NULL) {
    return NULL;
  }

  /* note the memcpy will copy the pointers : so, you shouldn't free them */
  memcpy(cipher, &ssh_get_ciphertab()[offset], sizeof(*cipher));

  return cipher;
}

static void cipher_free(struct crypto_struct *cipher) {
#ifdef HAVE_LIBGCRYPT
  unsigned int i;
#endif

  if (cipher == NULL) {
    return;
  }

  if(cipher->key) {
#ifdef HAVE_LIBGCRYPT
    for (i = 0; i < (cipher->keylen / sizeof(gcry_cipher_hd_t)); i++) {
      gcry_cipher_close(cipher->key[i]);
    }
#elif defined HAVE_LIBCRYPTO
    /* destroy the key */
    memset(cipher->key, 0, cipher->keylen);
#endif
    SAFE_FREE(cipher->key);
  }
  SAFE_FREE(cipher);
}

struct ssh_crypto_struct *crypto_new(void) {
   struct ssh_crypto_struct *crypto;

  crypto = malloc(sizeof(struct ssh_crypto_struct));
  if (crypto == NULL) {
    return NULL;
  }
  ZERO_STRUCTP(crypto);
  return crypto;
}

void crypto_free(struct ssh_crypto_struct *crypto){
  if (crypto == NULL) {
    return;
  }

  SAFE_FREE(crypto->server_pubkey);

  cipher_free(crypto->in_cipher);
  cipher_free(crypto->out_cipher);

  bignum_free(crypto->e);
  bignum_free(crypto->f);
  bignum_free(crypto->x);
  bignum_free(crypto->y);
  bignum_free(crypto->k);
  /* lot of other things */

#ifdef WITH_LIBZ
  if (crypto->compress_out_ctx &&
      (deflateEnd(crypto->compress_out_ctx) != 0)) {
    inflateEnd(crypto->compress_out_ctx);
  }
  if (crypto->compress_in_ctx &&
      (deflateEnd(crypto->compress_in_ctx) != 0)) {
    inflateEnd(crypto->compress_in_ctx);
  }
#endif

  /* i'm lost in my own code. good work */
  memset(crypto,0,sizeof(*crypto));

  SAFE_FREE(crypto);
}

static int crypt_set_algorithms2(ssh_session session){
  const char *wanted;
  int i = 0;
  struct crypto_struct *ssh_ciphertab=ssh_get_ciphertab();
  /* we must scan the kex entries to find crypto algorithms and set their appropriate structure */
  /* out */
  wanted = session->client_kex.methods[SSH_CRYPT_C_S];
  while (ssh_ciphertab[i].name && strcmp(wanted, ssh_ciphertab[i].name)) {
    i++;
  }

  if (ssh_ciphertab[i].name == NULL) {
    ssh_set_error(session, SSH_FATAL,
        "Crypt_set_algorithms2: no crypto algorithm function found for %s",
        wanted);
    return SSH_ERROR;
  }
  ssh_log(session, SSH_LOG_PACKET, "Set output algorithm to %s", wanted);

  session->next_crypto->out_cipher = cipher_new(i);
  if (session->next_crypto->out_cipher == NULL) {
    ssh_set_error(session, SSH_FATAL, "No space left");
    return SSH_ERROR;
  }
  i = 0;

  /* in */
  wanted = session->client_kex.methods[SSH_CRYPT_S_C];
  while (ssh_ciphertab[i].name && strcmp(wanted, ssh_ciphertab[i].name)) {
    i++;
  }

  if (ssh_ciphertab[i].name == NULL) {
    ssh_set_error(session, SSH_FATAL,
        "Crypt_set_algorithms: no crypto algorithm function found for %s",
        wanted);
    return SSH_ERROR;
  }
  ssh_log(session, SSH_LOG_PACKET, "Set input algorithm to %s", wanted);

  session->next_crypto->in_cipher = cipher_new(i);
  if (session->next_crypto->in_cipher == NULL) {
    ssh_set_error(session, SSH_FATAL, "Not enough space");
    return SSH_ERROR;
  }

  /* compression */
  if (strcmp(session->client_kex.methods[SSH_COMP_C_S], "zlib") == 0) {
    session->next_crypto->do_compress_out = 1;
  }
  if (strcmp(session->client_kex.methods[SSH_COMP_S_C], "zlib") == 0) {
    session->next_crypto->do_compress_in = 1;
  }
  if (strcmp(session->client_kex.methods[SSH_COMP_C_S], "zlib@openssh.com") == 0) {
    session->next_crypto->delayed_compress_out = 1;
  }
  if (strcmp(session->client_kex.methods[SSH_COMP_S_C], "zlib@openssh.com") == 0) {
    session->next_crypto->delayed_compress_in = 1;
  }
  return SSH_OK;
}

static int crypt_set_algorithms1(ssh_session session) {
  int i = 0;
  struct crypto_struct *ssh_ciphertab=ssh_get_ciphertab();

  /* right now, we force 3des-cbc to be taken */
  while (ssh_ciphertab[i].name && strcmp(ssh_ciphertab[i].name,
        "3des-cbc-ssh1")) {
    i++;
  }

  if (ssh_ciphertab[i].name == NULL) {
    ssh_set_error(session, SSH_FATAL, "cipher 3des-cbc-ssh1 not found!");
    return -1;
  }

  session->next_crypto->out_cipher = cipher_new(i);
  if (session->next_crypto->out_cipher == NULL) {
    ssh_set_error(session, SSH_FATAL, "No space left");
    return SSH_ERROR;
  }

  session->next_crypto->in_cipher = cipher_new(i);
  if (session->next_crypto->in_cipher == NULL) {
    ssh_set_error(session, SSH_FATAL, "No space left");
    return SSH_ERROR;
  }

  return SSH_OK;
}

int crypt_set_algorithms(ssh_session session) {
  return (session->version == 1) ? crypt_set_algorithms1(session) :
    crypt_set_algorithms2(session);
}

// TODO Obviously too much cut and paste here
int crypt_set_algorithms_server(ssh_session session){
    char *server = NULL;
    char *client = NULL;
    char *match = NULL;
    int i = 0;
    struct crypto_struct *ssh_ciphertab=ssh_get_ciphertab();

    if (session == NULL) {
        return SSH_ERROR;
    }

    /* we must scan the kex entries to find crypto algorithms and set their appropriate structure */
    enter_function();
    /* out */
    server = session->server_kex.methods[SSH_CRYPT_S_C];
    if(session->client_kex.methods) {
        client = session->client_kex.methods[SSH_CRYPT_S_C];
    } else {
        ssh_log(session,SSH_LOG_PROTOCOL, "Client KEX empty");
    }
    /* That's the client algorithms that are more important */
    match = ssh_find_matching(server,client);


    if(!match){
        ssh_set_error(session,SSH_FATAL,"Crypt_set_algorithms_server : no matching algorithm function found for %s",server);
        free(match);
        leave_function();
        return SSH_ERROR;
    }
    while(ssh_ciphertab[i].name && strcmp(match,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,SSH_FATAL,"Crypt_set_algorithms_server : no crypto algorithm function found for %s",server);
        free(match);
        leave_function();
        return SSH_ERROR;
    }
    ssh_log(session,SSH_LOG_PACKET,"Set output algorithm %s",match);
    SAFE_FREE(match);

    session->next_crypto->out_cipher = cipher_new(i);
    if (session->next_crypto->out_cipher == NULL) {
      ssh_set_error(session, SSH_FATAL, "No space left");
      leave_function();
      return SSH_ERROR;
    }
    i=0;
    /* in */
    client=session->client_kex.methods[SSH_CRYPT_C_S];
    server=session->server_kex.methods[SSH_CRYPT_S_C];
    match=ssh_find_matching(server,client);
    if(!match){
        ssh_set_error(session,SSH_FATAL,"Crypt_set_algorithms_server : no matching algorithm function found for %s",server);
        free(match);
        leave_function();
        return SSH_ERROR;
    }
    while(ssh_ciphertab[i].name && strcmp(match,ssh_ciphertab[i].name))
        i++;
    if(!ssh_ciphertab[i].name){
        ssh_set_error(session,SSH_FATAL,"Crypt_set_algorithms_server : no crypto algorithm function found for %s",server);
        free(match);
        leave_function();
        return SSH_ERROR;
    }
    ssh_log(session,SSH_LOG_PACKET,"Set input algorithm %s",match);
    SAFE_FREE(match);

    session->next_crypto->in_cipher = cipher_new(i);
    if (session->next_crypto->in_cipher == NULL) {
      ssh_set_error(session, SSH_FATAL, "No space left");
      leave_function();
      return SSH_ERROR;
    }

    /* compression */
    client=session->client_kex.methods[SSH_CRYPT_C_S];
    server=session->server_kex.methods[SSH_CRYPT_C_S];
    match=ssh_find_matching(server,client);
    if(match && !strcmp(match,"zlib")){
        ssh_log(session,SSH_LOG_PACKET,"enabling C->S compression");
        session->next_crypto->do_compress_in=1;
    }
    SAFE_FREE(match);

    client=session->client_kex.methods[SSH_CRYPT_S_C];
    server=session->server_kex.methods[SSH_CRYPT_S_C];
    match=ssh_find_matching(server,client);
    if(match && !strcmp(match,"zlib")){
        ssh_log(session,SSH_LOG_PACKET,"enabling S->C compression\n");
        session->next_crypto->do_compress_out=1;
    }
    SAFE_FREE(match);

    server=session->server_kex.methods[SSH_HOSTKEYS];
    client=session->client_kex.methods[SSH_HOSTKEYS];
    match=ssh_find_matching(server,client);
    if(match && !strcmp(match,"ssh-dss"))
        session->hostkeys=SSH_KEYTYPE_DSS;
    else if(match && !strcmp(match,"ssh-rsa"))
        session->hostkeys=SSH_KEYTYPE_RSA;
    else {
        ssh_set_error(session, SSH_FATAL, "Cannot know what %s is into %s",
            match ? match : NULL, server);
        SAFE_FREE(match);
        leave_function();
        return SSH_ERROR;
    }
    SAFE_FREE(match);
    leave_function();
    return SSH_OK;
}

/* vim: set ts=2 sw=2 et cindent: */
