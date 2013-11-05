/*
 * dh.c - Diffie-Helman algorithm code against SSH 2
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2008 by Aris Adamantiadis
 * Copyright (c) 2009      by Andreas Schneider <mail@cynapses.org>
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
 * Let us resume the dh protocol.
 * Each side computes a private prime number, x at client side, y at server
 * side.
 * g and n are two numbers common to every ssh software.
 * client's public key (e) is calculated by doing:
 * e = g^x mod p
 * client sends e to the server.
 * the server computes his own public key, f
 * f = g^y mod p
 * it sends it to the client
 * the common key K is calculated by the client by doing
 * k = f^x mod p
 * the server does the same with the client public key e
 * k' = e^y mod p
 * if everything went correctly, k and k' are equal
 */

#include "config.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libssh/priv.h"
#include "libssh/crypto.h"
#include "libssh/buffer.h"
#include "libssh/session.h"
#include "libssh/keys.h"
#include "libssh/dh.h"

/* todo: remove it */
#include "libssh/string.h"
#ifdef HAVE_LIBCRYPTO
#include <openssl/rand.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#endif

static unsigned char p_value[] = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xC9, 0x0F, 0xDA, 0xA2,
        0x21, 0x68, 0xC2, 0x34, 0xC4, 0xC6, 0x62, 0x8B, 0x80, 0xDC, 0x1C, 0xD1,
        0x29, 0x02, 0x4E, 0x08, 0x8A, 0x67, 0xCC, 0x74, 0x02, 0x0B, 0xBE, 0xA6,
        0x3B, 0x13, 0x9B, 0x22, 0x51, 0x4A, 0x08, 0x79, 0x8E, 0x34, 0x04, 0xDD,
        0xEF, 0x95, 0x19, 0xB3, 0xCD, 0x3A, 0x43, 0x1B, 0x30, 0x2B, 0x0A, 0x6D,
        0xF2, 0x5F, 0x14, 0x37, 0x4F, 0xE1, 0x35, 0x6D, 0x6D, 0x51, 0xC2, 0x45,
        0xE4, 0x85, 0xB5, 0x76, 0x62, 0x5E, 0x7E, 0xC6, 0xF4, 0x4C, 0x42, 0xE9,
        0xA6, 0x37, 0xED, 0x6B, 0x0B, 0xFF, 0x5C, 0xB6, 0xF4, 0x06, 0xB7, 0xED,
        0xEE, 0x38, 0x6B, 0xFB, 0x5A, 0x89, 0x9F, 0xA5, 0xAE, 0x9F, 0x24, 0x11,
        0x7C, 0x4B, 0x1F, 0xE6, 0x49, 0x28, 0x66, 0x51, 0xEC, 0xE6, 0x53, 0x81,
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
#define P_LEN 128	/* Size in bytes of the p number */

static unsigned long g_int = 2 ;	/* G is defined as 2 by the ssh2 standards */
static bignum g;
static bignum p;
static int ssh_crypto_initialized;

int ssh_get_random(void *where, int len, int strong){

#ifdef HAVE_LIBGCRYPT
  /* variable not used in gcrypt */
  (void) strong;
  /* not using GCRY_VERY_STRONG_RANDOM which is a bit overkill */
  gcry_randomize(where,len,GCRY_STRONG_RANDOM);

  return 1;
#elif defined HAVE_LIBCRYPTO
  if (strong) {
    return RAND_bytes(where,len);
  } else {
    return RAND_pseudo_bytes(where,len);
  }
#endif

  /* never reached */
  return 1;
}

/*
 * This inits the values g and p which are used for DH key agreement
 * FIXME: Make the function thread safe by adding a semaphore or mutex.
 */
int ssh_crypto_init(void) {
  if (ssh_crypto_initialized == 0) {
#ifdef HAVE_LIBGCRYPT
    gcry_check_version(NULL);
    if (!gcry_control(GCRYCTL_INITIALIZATION_FINISHED_P,0)) {
      gcry_control(GCRYCTL_INIT_SECMEM, 4096);
      gcry_control(GCRYCTL_INITIALIZATION_FINISHED,0);
    }
#endif

    g = bignum_new();
    if (g == NULL) {
      return -1;
    }
    bignum_set_word(g,g_int);

#ifdef HAVE_LIBGCRYPT
    bignum_bin2bn(p_value, P_LEN, &p);
    if (p == NULL) {
      bignum_free(g);
      g = NULL;
      return -1;
    }
#elif defined HAVE_LIBCRYPTO
    p = bignum_new();
    if (p == NULL) {
      bignum_free(g);
      g = NULL;
      return -1;
    }
    bignum_bin2bn(p_value, P_LEN, p);
    OpenSSL_add_all_algorithms();

#endif

    ssh_crypto_initialized = 1;
  }

  return 0;
}

void ssh_crypto_finalize(void) {
  if (ssh_crypto_initialized) {
    bignum_free(g);
    g = NULL;
    bignum_free(p);
    p = NULL;
#ifdef HAVE_LIBGCRYPT
    gcry_control(GCRYCTL_TERM_SECMEM);
#elif defined HAVE_LIBCRYPTO
    EVP_cleanup();
    CRYPTO_cleanup_all_ex_data();
#endif
    ssh_crypto_initialized=0;
  }
}

/* prints the bignum on stderr */
void ssh_print_bignum(const char *which, bignum num) {
#ifdef HAVE_LIBGCRYPT
  unsigned char *hex = NULL;
  bignum_bn2hex(num, &hex);
#elif defined HAVE_LIBCRYPTO
  char *hex = NULL;
  hex = bignum_bn2hex(num);
#endif
  fprintf(stderr, "%s value: ", which);
  fprintf(stderr, "%s\n", (hex == NULL) ? "(null)" : (char *) hex);
  SAFE_FREE(hex);
}

/**
 * @brief Convert a buffer into a colon separated hex string.
 * The caller has to free the memory.
 *
 * @param  what         What should be converted to a hex string.
 *
 * @param  len          Length of the buffer to convert.
 *
 * @return              The hex string or NULL on error.
 */
char *ssh_get_hexa(const unsigned char *what, size_t len) {
  char *hexa = NULL;
  size_t i;

  if (len > (UINT_MAX - 1) / 3)
    return NULL;

  hexa = malloc(len * 3 + 1);
  if (hexa == NULL) {
    return NULL;
  }

  ZERO_STRUCTP(hexa);

  for (i = 0; i < len; i++) {
    char hex[4];
    snprintf(hex, sizeof(hex), "%02x:", what[i]);
    strcat(hexa, hex);
  }

  hexa[(len * 3) - 1] = '\0';

  return hexa;
}

/**
 * @brief Print a buffer as colon separated hex string.
 *
 * @param  descr        Description printed in front of the hex string.
 *
 * @param  what         What should be converted to a hex string.
 *
 * @param  len          Length of the buffer to convert.
 */
void ssh_print_hexa(const char *descr, const unsigned char *what, size_t len) {
    char *hexa = ssh_get_hexa(what, len);

    if (hexa == NULL) {
      return;
    }
    printf("%s: %s\n", descr, hexa);

    free(hexa);
}

int dh_generate_x(ssh_session session) {
  session->next_crypto->x = bignum_new();
  if (session->next_crypto->x == NULL) {
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_rand(session->next_crypto->x, 128);
#elif defined HAVE_LIBCRYPTO
  bignum_rand(session->next_crypto->x, 128, 0, -1);
#endif

  /* not harder than this */
#ifdef DEBUG_CRYPTO
  ssh_print_bignum("x", session->next_crypto->x);
#endif

  return 0;
}

/* used by server */
int dh_generate_y(ssh_session session) {
    session->next_crypto->y = bignum_new();
  if (session->next_crypto->y == NULL) {
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_rand(session->next_crypto->y, 128);
#elif defined HAVE_LIBCRYPTO
  bignum_rand(session->next_crypto->y, 128, 0, -1);
#endif

  /* not harder than this */
#ifdef DEBUG_CRYPTO
  ssh_print_bignum("y", session->next_crypto->y);
#endif

  return 0;
}

/* used by server */
int dh_generate_e(ssh_session session) {
#ifdef HAVE_LIBCRYPTO
  bignum_CTX ctx = bignum_ctx_new();
  if (ctx == NULL) {
    return -1;
  }
#endif

  session->next_crypto->e = bignum_new();
  if (session->next_crypto->e == NULL) {
#ifdef HAVE_LIBCRYPTO
    bignum_ctx_free(ctx);
#endif
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_mod_exp(session->next_crypto->e, g, session->next_crypto->x, p);
#elif defined HAVE_LIBCRYPTO
  bignum_mod_exp(session->next_crypto->e, g, session->next_crypto->x, p, ctx);
#endif

#ifdef DEBUG_CRYPTO
  ssh_print_bignum("e", session->next_crypto->e);
#endif

#ifdef HAVE_LIBCRYPTO
  bignum_ctx_free(ctx);
#endif

  return 0;
}

int dh_generate_f(ssh_session session) {
#ifdef HAVE_LIBCRYPTO
  bignum_CTX ctx = bignum_ctx_new();
  if (ctx == NULL) {
    return -1;
  }
#endif

  session->next_crypto->f = bignum_new();
  if (session->next_crypto->f == NULL) {
#ifdef HAVE_LIBCRYPTO
    bignum_ctx_free(ctx);
#endif
    return -1;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_mod_exp(session->next_crypto->f, g, session->next_crypto->y, p);
#elif defined HAVE_LIBCRYPTO
  bignum_mod_exp(session->next_crypto->f, g, session->next_crypto->y, p, ctx);
#endif

#ifdef DEBUG_CRYPTO
  ssh_print_bignum("f", session->next_crypto->f);
#endif

#ifdef HAVE_LIBCRYPTO
  bignum_ctx_free(ctx);
#endif

  return 0;
}

ssh_string make_bignum_string(bignum num) {
  ssh_string ptr = NULL;
  int pad = 0;
  unsigned int len = bignum_num_bytes(num);
  unsigned int bits = bignum_num_bits(num);

  /* Remember if the fist bit is set, it is considered as a
   * negative number. So 0's must be appended */
  if (!(bits % 8) && bignum_is_bit_set(num, bits - 1)) {
    pad++;
  }

#ifdef DEBUG_CRYPTO
  fprintf(stderr, "%d bits, %d bytes, %d padding\n", bits, len, pad);
#endif /* DEBUG_CRYPTO */
/* TODO: fix that crap !! */
  ptr = malloc(4 + len + pad);
  if (ptr == NULL) {
    return NULL;
  }
  ptr->size = htonl(len + pad);
  if (pad) {
    ptr->string[0] = 0;
  }

#ifdef HAVE_LIBGCRYPT
  bignum_bn2bin(num, len, ptr->string + pad);
#elif HAVE_LIBCRYPTO
  bignum_bn2bin(num, ptr->string + pad);
#endif

  return ptr;
}

bignum make_string_bn(ssh_string string){
  bignum bn = NULL;
  unsigned int len = ssh_string_len(string);

#ifdef DEBUG_CRYPTO
  fprintf(stderr, "Importing a %d bits, %d bytes object ...\n",
      len * 8, len);
#endif /* DEBUG_CRYPTO */

#ifdef HAVE_LIBGCRYPT
  bignum_bin2bn(string->string, len, &bn);
#elif defined HAVE_LIBCRYPTO
  bn = bignum_bin2bn(string->string, len, NULL);
#endif

  return bn;
}

ssh_string dh_get_e(ssh_session session) {
  return make_bignum_string(session->next_crypto->e);
}

/* used by server */
ssh_string dh_get_f(ssh_session session) {
  return make_bignum_string(session->next_crypto->f);
}

void dh_import_pubkey(ssh_session session, ssh_string pubkey_string) {
  session->next_crypto->server_pubkey = pubkey_string;
}

int dh_import_f(ssh_session session, ssh_string f_string) {
  session->next_crypto->f = make_string_bn(f_string);
  if (session->next_crypto->f == NULL) {
    return -1;
  }

#ifdef DEBUG_CRYPTO
  ssh_print_bignum("f",session->next_crypto->f);
#endif

  return 0;
}

/* used by the server implementation */
int dh_import_e(ssh_session session, ssh_string e_string) {
  session->next_crypto->e = make_string_bn(e_string);
  if (session->next_crypto->e == NULL) {
    return -1;
  }

#ifdef DEBUG_CRYPTO
    ssh_print_bignum("e",session->next_crypto->e);
#endif

  return 0;
}

int dh_build_k(ssh_session session) {
#ifdef HAVE_LIBCRYPTO
  bignum_CTX ctx = bignum_ctx_new();
  if (ctx == NULL) {
    return -1;
  }
#endif

  session->next_crypto->k = bignum_new();
  if (session->next_crypto->k == NULL) {
#ifdef HAVE_LIBCRYPTO
    bignum_ctx_free(ctx);
#endif
    return -1;
  }

    /* the server and clients don't use the same numbers */
#ifdef HAVE_LIBGCRYPT
  if(session->client) {
    bignum_mod_exp(session->next_crypto->k, session->next_crypto->f,
        session->next_crypto->x, p);
  } else {
    bignum_mod_exp(session->next_crypto->k, session->next_crypto->e,
        session->next_crypto->y, p);
  }
#elif defined HAVE_LIBCRYPTO
  if (session->client) {
    bignum_mod_exp(session->next_crypto->k, session->next_crypto->f,
        session->next_crypto->x, p, ctx);
  } else {
    bignum_mod_exp(session->next_crypto->k, session->next_crypto->e,
        session->next_crypto->y, p, ctx);
  }
#endif

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Session server cookie", session->server_kex.cookie, 16);
  ssh_print_hexa("Session client cookie", session->client_kex.cookie, 16);
  ssh_print_bignum("Shared secret key", session->next_crypto->k);
#endif

#ifdef HAVE_LIBCRYPTO
  bignum_ctx_free(ctx);
#endif

  return 0;
}

/*
static void sha_add(ssh_string str,SHACTX ctx){
    sha1_update(ctx,str,string_len(str)+4);
#ifdef DEBUG_CRYPTO
    ssh_print_hexa("partial hashed sessionid",str,string_len(str)+4);
#endif
}
*/

int make_sessionid(ssh_session session) {
  SHACTX ctx;
  ssh_string num = NULL;
  ssh_string str = NULL;
  ssh_buffer server_hash = NULL;
  ssh_buffer client_hash = NULL;
  ssh_buffer buf = NULL;
  uint32_t len;
  int rc = SSH_ERROR;

  enter_function();

  ctx = sha1_init();
  if (ctx == NULL) {
    return rc;
  }

  buf = ssh_buffer_new();
  if (buf == NULL) {
    return rc;
  }

  str = ssh_string_from_char(session->clientbanner);
  if (str == NULL) {
    goto error;
  }

  if (buffer_add_ssh_string(buf, str) < 0) {
    goto error;
  }
  ssh_string_free(str);

  str = ssh_string_from_char(session->serverbanner);
  if (str == NULL) {
    goto error;
  }

  if (buffer_add_ssh_string(buf, str) < 0) {
    goto error;
  }

  if (session->client) {
    server_hash = session->in_hashbuf;
    client_hash = session->out_hashbuf;
  } else {
    server_hash = session->out_hashbuf;
    client_hash = session->in_hashbuf;
  }

  if (buffer_add_u32(server_hash, 0) < 0) {
    goto error;
  }
  if (buffer_add_u8(server_hash, 0) < 0) {
    goto error;
  }
  if (buffer_add_u32(client_hash, 0) < 0) {
    goto error;
  }
  if (buffer_add_u8(client_hash, 0) < 0) {
    goto error;
  }

  len = ntohl(buffer_get_rest_len(client_hash));
  if (buffer_add_u32(buf,len) < 0) {
    goto error;
  }
  if (buffer_add_data(buf, buffer_get_rest(client_hash),
        buffer_get_rest_len(client_hash)) < 0) {
    goto error;
  }

  len = ntohl(buffer_get_rest_len(server_hash));
  if (buffer_add_u32(buf, len) < 0) {
    goto error;
  }
  if (buffer_add_data(buf, buffer_get_rest(server_hash),
        buffer_get_rest_len(server_hash)) < 0) {
    goto error;
  }

  len = ssh_string_len(session->next_crypto->server_pubkey) + 4;
  if (buffer_add_data(buf, session->next_crypto->server_pubkey, len) < 0) {
    goto error;
  }

  num = make_bignum_string(session->next_crypto->e);
  if (num == NULL) {
    goto error;
  }

  len = ssh_string_len(num) + 4;
  if (buffer_add_data(buf, num, len) < 0) {
    goto error;
  }

  ssh_string_free(num);
  num = make_bignum_string(session->next_crypto->f);
  if (num == NULL) {
    goto error;
  }

  len = ssh_string_len(num) + 4;
  if (buffer_add_data(buf, num, len) < 0) {
    goto error;
  }

  ssh_string_free(num);
  num = make_bignum_string(session->next_crypto->k);
  if (num == NULL) {
    goto error;
  }

  len = ssh_string_len(num) + 4;
  if (buffer_add_data(buf, num, len) < 0) {
    goto error;
  }

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("hash buffer", ssh_buffer_get_begin(buf), ssh_buffer_get_len(buf));
#endif

  sha1_update(ctx, buffer_get_rest(buf), buffer_get_rest_len(buf));
  sha1_final(session->next_crypto->session_id, ctx);

#ifdef DEBUG_CRYPTO
  printf("Session hash: ");
  ssh_print_hexa("session id", session->next_crypto->session_id, SHA_DIGEST_LEN);
#endif

  rc = SSH_OK;
error:
  ssh_buffer_free(buf);
  ssh_buffer_free(client_hash);
  ssh_buffer_free(server_hash);

  session->in_hashbuf = NULL;
  session->out_hashbuf = NULL;

  ssh_string_free(str);
  ssh_string_free(num);

  leave_function();

  return rc;
}

int hashbufout_add_cookie(ssh_session session) {
  session->out_hashbuf = ssh_buffer_new();
  if (session->out_hashbuf == NULL) {
    return -1;
  }

  if (buffer_add_u8(session->out_hashbuf, 20) < 0) {
    buffer_reinit(session->out_hashbuf);
    return -1;
  }

  if (session->server) {
    if (buffer_add_data(session->out_hashbuf,
          session->server_kex.cookie, 16) < 0) {
      buffer_reinit(session->out_hashbuf);
      return -1;
    }
  } else {
    if (buffer_add_data(session->out_hashbuf,
          session->client_kex.cookie, 16) < 0) {
      buffer_reinit(session->out_hashbuf);
      return -1;
    }
  }

  return 0;
}

int hashbufin_add_cookie(ssh_session session, unsigned char *cookie) {
  session->in_hashbuf = ssh_buffer_new();
  if (session->in_hashbuf == NULL) {
    return -1;
  }

  if (buffer_add_u8(session->in_hashbuf, 20) < 0) {
    buffer_reinit(session->in_hashbuf);
    return -1;
  }
  if (buffer_add_data(session->in_hashbuf,cookie, 16) < 0) {
    buffer_reinit(session->in_hashbuf);
    return -1;
  }

  return 0;
}

static int generate_one_key(ssh_string k,
    unsigned char session_id[SHA_DIGEST_LEN],
    unsigned char output[SHA_DIGEST_LEN],
    char letter) {
  SHACTX ctx = NULL;

  ctx = sha1_init();
  if (ctx == NULL) {
    return -1;
  }

  sha1_update(ctx, k, ssh_string_len(k) + 4);
  sha1_update(ctx, session_id, SHA_DIGEST_LEN);
  sha1_update(ctx, &letter, 1);
  sha1_update(ctx, session_id, SHA_DIGEST_LEN);
  sha1_final(output, ctx);

  return 0;
}

int generate_session_keys(ssh_session session) {
  ssh_string k_string = NULL;
  SHACTX ctx = NULL;
  int rc = -1;

  enter_function();

  k_string = make_bignum_string(session->next_crypto->k);
  if (k_string == NULL) {
    goto error;
  }

  /* IV */
  if (session->client) {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptIV, 'A') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptIV, 'B') < 0) {
      goto error;
    }
  } else {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptIV, 'A') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptIV, 'B') < 0) {
      goto error;
    }
  }
  if (session->client) {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptkey, 'C') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptkey, 'D') < 0) {
      goto error;
    }
  } else {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptkey, 'C') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptkey, 'D') < 0) {
      goto error;
    }
  }

  /* some ciphers need more than 20 bytes of input key */
  /* XXX verify it's ok for server implementation */
  if (session->next_crypto->out_cipher->keysize > SHA_DIGEST_LEN * 8) {
    ctx = sha1_init();
    if (ctx == NULL) {
      goto error;
    }
    sha1_update(ctx, k_string, ssh_string_len(k_string) + 4);
    sha1_update(ctx, session->next_crypto->session_id, SHA_DIGEST_LEN);
    sha1_update(ctx, session->next_crypto->encryptkey, SHA_DIGEST_LEN);
    sha1_final(session->next_crypto->encryptkey + SHA_DIGEST_LEN, ctx);
  }

  if (session->next_crypto->in_cipher->keysize > SHA_DIGEST_LEN * 8) {
    ctx = sha1_init();
    sha1_update(ctx, k_string, ssh_string_len(k_string) + 4);
    sha1_update(ctx, session->next_crypto->session_id, SHA_DIGEST_LEN);
    sha1_update(ctx, session->next_crypto->decryptkey, SHA_DIGEST_LEN);
    sha1_final(session->next_crypto->decryptkey + SHA_DIGEST_LEN, ctx);
  }
  if(session->client) {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptMAC, 'E') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptMAC, 'F') < 0) {
      goto error;
    }
  } else {
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->decryptMAC, 'E') < 0) {
      goto error;
    }
    if (generate_one_key(k_string, session->next_crypto->session_id,
          session->next_crypto->encryptMAC, 'F') < 0) {
      goto error;
    }
  }

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Encrypt IV", session->next_crypto->encryptIV, SHA_DIGEST_LEN);
  ssh_print_hexa("Decrypt IV", session->next_crypto->decryptIV, SHA_DIGEST_LEN);
  ssh_print_hexa("Encryption key", session->next_crypto->encryptkey,
      session->next_crypto->out_cipher->keysize);
  ssh_print_hexa("Decryption key", session->next_crypto->decryptkey,
      session->next_crypto->in_cipher->keysize);
  ssh_print_hexa("Encryption MAC", session->next_crypto->encryptMAC, SHA_DIGEST_LEN);
  ssh_print_hexa("Decryption MAC", session->next_crypto->decryptMAC, 20);
#endif

  rc = 0;
error:
  ssh_string_free(k_string);
  leave_function();

  return rc;
}

/**
 * @addtogroup libssh_session
 *
 * @{
 */

/**
 * @brief Allocates a buffer with the MD5 hash of the server public key.
 *
 * @param[in] session   The SSH session to use.
 *
 * @param[in] hash      The buffer to allocate.
 *
 * @return The bytes allocated or < 0 on error.
 *
 * @warning It is very important that you verify at some moment that the hash
 *          matches a known server. If you don't do it, cryptography wont help
 *          you at making things secure
 *
 * @see ssh_is_server_known()
 * @see ssh_get_hexa()
 * @see ssh_print_hexa()
 */
int ssh_get_pubkey_hash(ssh_session session, unsigned char **hash) {
  ssh_string pubkey;
  MD5CTX ctx;
  unsigned char *h;

  if (session == NULL || hash == NULL) {
    return SSH_ERROR;
  }
  *hash = NULL;
  if (session->current_crypto == NULL ||
      session->current_crypto->server_pubkey == NULL){
    ssh_set_error(session,SSH_FATAL,"No current cryptographic context");
    return SSH_ERROR;
  }

  h = malloc(sizeof(unsigned char *) * MD5_DIGEST_LEN);
  if (h == NULL) {
    return SSH_ERROR;
  }

  ctx = md5_init();
  if (ctx == NULL) {
    SAFE_FREE(h);
    return SSH_ERROR;
  }

  pubkey = session->current_crypto->server_pubkey;

  md5_update(ctx, pubkey->string, ssh_string_len(pubkey));
  md5_final(h, ctx);

  *hash = h;

  return MD5_DIGEST_LEN;
}

/**
 * @brief Deallocate the hash obtained by ssh_get_pubkey_hash.
 *
 * This is required under Microsoft platform as this library might use a 
 * different C library than your software, hence a different heap.
 *
 * @param[in] hash      The buffer to deallocate.
 *
 * @see ssh_get_pubkey_hash()
 */
void ssh_clean_pubkey_hash(unsigned char **hash) {
  SAFE_FREE(*hash);
  *hash = NULL;
}

ssh_string ssh_get_pubkey(ssh_session session){
	if(session==NULL || session->current_crypto ==NULL ||
      session->current_crypto->server_pubkey==NULL)
    return NULL;
	else
    return ssh_string_copy(session->current_crypto->server_pubkey);
}

static int match(const char *group, const char *object){
  const char *a;
  const char *z;

  z = group;
  do {
    a = strchr(z, ',');
    if (a == NULL) {
      if (strcmp(z, object) == 0) {
        return 1;
      }
      return 0;
    } else {
      if (strncmp(z, object, a - z) == 0) {
        return 1;
      }
    }
    z = a + 1;
  } while(1);

  /* not reached */
  return 0;
}

int sig_verify(ssh_session session, ssh_public_key pubkey,
    SIGNATURE *signature, unsigned char *digest, int size) {
#ifdef HAVE_LIBGCRYPT
  gcry_error_t valid = 0;
  gcry_sexp_t gcryhash;
#elif defined HAVE_LIBCRYPTO
  int valid = 0;
#endif
  unsigned char hash[SHA_DIGEST_LEN + 1] = {0};

  sha1(digest, size, hash + 1);

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Hash to be verified with dsa", hash + 1, SHA_DIGEST_LEN);
#endif

  switch(pubkey->type) {
    case SSH_KEYTYPE_DSS:
#ifdef HAVE_LIBGCRYPT
      valid = gcry_sexp_build(&gcryhash, NULL, "%b", SHA_DIGEST_LEN + 1, hash);
      if (valid != 0) {
        ssh_set_error(session, SSH_FATAL,
            "RSA error: %s", gcry_strerror(valid));
        return -1;
      }
      valid = gcry_pk_verify(signature->dsa_sign, gcryhash, pubkey->dsa_pub);
      gcry_sexp_release(gcryhash);
      if (valid == 0) {
        return 0;
      }

      if (gcry_err_code(valid) != GPG_ERR_BAD_SIGNATURE) {
        ssh_set_error(session, SSH_FATAL,
            "DSA error: %s", gcry_strerror(valid));
        return -1;
      }
#elif defined HAVE_LIBCRYPTO
      valid = DSA_do_verify(hash + 1, SHA_DIGEST_LEN, signature->dsa_sign,
          pubkey->dsa_pub);
      if (valid == 1) {
        return 0;
      }

      if (valid == -1) {
        ssh_set_error(session, SSH_FATAL,
            "DSA error: %s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
      }
#endif
      ssh_set_error(session, SSH_FATAL, "Invalid DSA signature");
      return -1;

    case SSH_KEYTYPE_RSA:
    case SSH_KEYTYPE_RSA1:
#ifdef HAVE_LIBGCRYPT
      valid = gcry_sexp_build(&gcryhash, NULL,
          "(data(flags pkcs1)(hash sha1 %b))", SHA_DIGEST_LEN, hash + 1);
      if (valid != 0) {
        ssh_set_error(session, SSH_FATAL,
            "RSA error: %s", gcry_strerror(valid));
        return -1;
      }
      valid = gcry_pk_verify(signature->rsa_sign,gcryhash,pubkey->rsa_pub);
      gcry_sexp_release(gcryhash);
      if (valid == 0) {
        return 0;
      }
      if (gcry_err_code(valid) != GPG_ERR_BAD_SIGNATURE) {
        ssh_set_error(session, SSH_FATAL,
            "RSA error: %s", gcry_strerror(valid));
        return -1;
      }
#elif defined HAVE_LIBCRYPTO
      valid = RSA_verify(NID_sha1, hash + 1, SHA_DIGEST_LEN,
          signature->rsa_sign->string, ssh_string_len(signature->rsa_sign),
          pubkey->rsa_pub);
      if (valid == 1) {
        return 0;
      }
      if (valid == -1) {
        ssh_set_error(session, SSH_FATAL,
            "RSA error: %s", ERR_error_string(ERR_get_error(), NULL));
        return -1;
      }
#endif
      ssh_set_error(session, SSH_FATAL, "Invalid RSA signature");
      return -1;
    default:
      ssh_set_error(session, SSH_FATAL, "Unknown public key type");
      return -1;
  }

  return -1;
}

int signature_verify(ssh_session session, ssh_string signature) {
  ssh_public_key pubkey = NULL;
  SIGNATURE *sign = NULL;
  int err;

  enter_function();

  pubkey = publickey_from_string(session,session->next_crypto->server_pubkey);
  if(pubkey == NULL) {
    leave_function();
    return -1;
  }

  if (session->wanted_methods[SSH_HOSTKEYS]) {
    if(!match(session->wanted_methods[SSH_HOSTKEYS],pubkey->type_c)) {
      ssh_set_error(session, SSH_FATAL,
          "Public key from server (%s) doesn't match user preference (%s)",
          pubkey->type_c, session->wanted_methods[SSH_HOSTKEYS]);
      publickey_free(pubkey);
      leave_function();
      return -1;
    }
  }

  sign = signature_from_string(session, signature, pubkey, pubkey->type);
  if (sign == NULL) {
    ssh_set_error(session, SSH_FATAL, "Invalid signature blob");
    publickey_free(pubkey);
    leave_function();
    return -1;
  }

  ssh_log(session, SSH_LOG_FUNCTIONS,
      "Going to verify a %s type signature", pubkey->type_c);

  err = sig_verify(session,pubkey,sign,
                            session->next_crypto->session_id,SHA_DIGEST_LEN);
  signature_free(sign);
  session->next_crypto->server_pubkey_type = pubkey->type_c;
  publickey_free(pubkey);

  leave_function();
  return err;
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
