/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "libssh/priv.h"
#include "libssh/session.h"
#include "libssh/crypto.h"
#include "libssh/wrapper.h"
#include "libssh/libcrypto.h"

#ifdef HAVE_LIBCRYPTO

#include <openssl/sha.h>
#include <openssl/md5.h>
#include <openssl/dsa.h>
#include <openssl/rsa.h>
#include <openssl/hmac.h>
#include <openssl/opensslv.h>
#ifdef HAVE_OPENSSL_AES_H
#define HAS_AES
#include <openssl/aes.h>
#endif
#ifdef HAVE_OPENSSL_BLOWFISH_H
#define HAS_BLOWFISH
#include <openssl/blowfish.h>
#endif
#ifdef HAVE_OPENSSL_DES_H
#define HAS_DES
#include <openssl/des.h>
#endif

#if (OPENSSL_VERSION_NUMBER<0x00907000L)
#define OLD_CRYPTO
#endif

#include "libssh/crypto.h"

static int alloc_key(struct crypto_struct *cipher) {
    cipher->key = malloc(cipher->keylen);
    if (cipher->key == NULL) {
      return -1;
    }

    return 0;
}

SHACTX sha1_init(void) {
  SHACTX c = malloc(sizeof(*c));
  if (c == NULL) {
    return NULL;
  }
  SHA1_Init(c);

  return c;
}

void sha1_update(SHACTX c, const void *data, unsigned long len) {
  SHA1_Update(c,data,len);
}

void sha1_final(unsigned char *md, SHACTX c) {
  SHA1_Final(md, c);
  SAFE_FREE(c);
}

void sha1(unsigned char *digest, int len, unsigned char *hash) {
  SHA1(digest, len, hash);
}

MD5CTX md5_init(void) {
  MD5CTX c = malloc(sizeof(*c));
  if (c == NULL) {
    return NULL;
  }

  MD5_Init(c);

  return c;
}

void md5_update(MD5CTX c, const void *data, unsigned long len) {
  MD5_Update(c, data, len);
}

void md5_final(unsigned char *md, MD5CTX c) {
  MD5_Final(md,c);
  SAFE_FREE(c);
}

HMACCTX hmac_init(const void *key, int len, int type) {
  HMACCTX ctx = NULL;

  ctx = malloc(sizeof(*ctx));
  if (ctx == NULL) {
    return NULL;
  }

#ifndef OLD_CRYPTO
  HMAC_CTX_init(ctx); // openssl 0.9.7 requires it.
#endif

  switch(type) {
    case HMAC_SHA1:
      HMAC_Init(ctx, key, len, EVP_sha1());
      break;
    case HMAC_MD5:
      HMAC_Init(ctx, key, len, EVP_md5());
      break;
    default:
      SAFE_FREE(ctx);
      ctx = NULL;
  }

  return ctx;
}

void hmac_update(HMACCTX ctx, const void *data, unsigned long len) {
  HMAC_Update(ctx, data, len);
}

void hmac_final(HMACCTX ctx, unsigned char *hashmacbuf, unsigned int *len) {
  HMAC_Final(ctx,hashmacbuf,len);

#ifndef OLD_CRYPTO
  HMAC_CTX_cleanup(ctx);
#else
  HMAC_cleanup(ctx);
#endif

  SAFE_FREE(ctx);
}

#ifdef HAS_BLOWFISH
/* the wrapper functions for blowfish */
static int blowfish_set_key(struct crypto_struct *cipher, void *key){
  if (cipher->key == NULL) {
    if (alloc_key(cipher) < 0) {
      return -1;
    }
    BF_set_key(cipher->key, 16, key);
  }

  return 0;
}

static void blowfish_encrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
  BF_cbc_encrypt(in, out, len, cipher->key, IV, BF_ENCRYPT);
}

static void blowfish_decrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
  BF_cbc_encrypt(in, out, len, cipher->key, IV, BF_DECRYPT);
}
#endif /* HAS_BLOWFISH */

#ifdef HAS_AES
static int aes_set_encrypt_key(struct crypto_struct *cipher, void *key) {
  if (cipher->key == NULL) {
    if (alloc_key(cipher) < 0) {
      return -1;
    }
    if (AES_set_encrypt_key(key,cipher->keysize,cipher->key) < 0) {
      SAFE_FREE(cipher->key);
      return -1;
    }
  }

  return 0;
}
static int aes_set_decrypt_key(struct crypto_struct *cipher, void *key) {
  if (cipher->key == NULL) {
    if (alloc_key(cipher) < 0) {
      return -1;
    }
    if (AES_set_decrypt_key(key,cipher->keysize,cipher->key) < 0) {
      SAFE_FREE(cipher->key);
      return -1;
    }
  }

  return 0;
}

static void aes_encrypt(struct crypto_struct *cipher, void *in, void *out,
    unsigned long len, void *IV) {
  AES_cbc_encrypt(in, out, len, cipher->key, IV, AES_ENCRYPT);
}

static void aes_decrypt(struct crypto_struct *cipher, void *in, void *out,
    unsigned long len, void *IV) {
  AES_cbc_encrypt(in, out, len, cipher->key, IV, AES_DECRYPT);
}

#ifndef BROKEN_AES_CTR
/* OpenSSL until 0.9.7c has a broken AES_ctr128_encrypt implementation which
 * increments the counter from 2^64 instead of 1. It's better not to use it
 */

/** @internal
 * @brief encrypts/decrypts data with stream cipher AES_ctr128. 128 bits is actually
 * the size of the CTR counter and incidentally the blocksize, but not the keysize.
 * @param len[in] must be a multiple of AES128 block size.
 */
static void aes_ctr128_encrypt(struct crypto_struct *cipher, void *in, void *out,
    unsigned long len, void *IV) {
  unsigned char tmp_buffer[128/8];
  unsigned int num=0;
  /* Some things are special with ctr128 :
   * In this case, tmp_buffer is not being used, because it is used to store temporary data
   * when an encryption is made on lengths that are not multiple of blocksize.
   * Same for num, which is being used to store the current offset in blocksize in CTR
   * function.
   */
  AES_ctr128_encrypt(in, out, len, cipher->key, IV, tmp_buffer, &num);
}
#endif /* BROKEN_AES_CTR */
#endif /* HAS_AES */

#ifdef HAS_DES
static int des3_set_key(struct crypto_struct *cipher, void *key) {
  if (cipher->key == NULL) {
    if (alloc_key(cipher) < 0) {
      return -1;
    }

    DES_set_odd_parity(key);
    DES_set_odd_parity((void*)((uint8_t*)key + 8));
    DES_set_odd_parity((void*)((uint8_t*)key + 16));
    DES_set_key_unchecked(key, cipher->key);
    DES_set_key_unchecked((void*)((uint8_t*)key + 8), (void*)((uint8_t*)cipher->key + sizeof(DES_key_schedule)));
    DES_set_key_unchecked((void*)((uint8_t*)key + 16), (void*)((uint8_t*)cipher->key + 2 * sizeof(DES_key_schedule)));
  }

  return 0;
}

static void des3_encrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
  DES_ede3_cbc_encrypt(in, out, len, cipher->key,
      (void*)((uint8_t*)cipher->key + sizeof(DES_key_schedule)),
      (void*)((uint8_t*)cipher->key + 2 * sizeof(DES_key_schedule)),
      IV, 1);
}

static void des3_decrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
  DES_ede3_cbc_encrypt(in, out, len, cipher->key,
      (void*)((uint8_t*)cipher->key + sizeof(DES_key_schedule)),
      (void*)((uint8_t*)cipher->key + 2 * sizeof(DES_key_schedule)),
      IV, 0);
}

static void des3_1_encrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Encrypt IV before", IV, 24);
#endif
  DES_ncbc_encrypt(in, out, len, cipher->key, IV, 1);
  DES_ncbc_encrypt(out, in, len, (void*)((uint8_t*)cipher->key + sizeof(DES_key_schedule)),
      (void*)((uint8_t*)IV + 8), 0);
  DES_ncbc_encrypt(in, out, len, (void*)((uint8_t*)cipher->key + 2 * sizeof(DES_key_schedule)),
      (void*)((uint8_t*)IV + 16), 1);
#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Encrypt IV after", IV, 24);
#endif
}

static void des3_1_decrypt(struct crypto_struct *cipher, void *in,
    void *out, unsigned long len, void *IV) {
#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Decrypt IV before", IV, 24);
#endif

  DES_ncbc_encrypt(in, out, len, (void*)((uint8_t*)cipher->key + 2 * sizeof(DES_key_schedule)),
      IV, 0);
  DES_ncbc_encrypt(out, in, len, (void*)((uint8_t*)cipher->key + sizeof(DES_key_schedule)),
      (void*)((uint8_t*)IV + 8), 1);
  DES_ncbc_encrypt(in, out, len, cipher->key, (void*)((uint8_t*)IV + 16), 0);

#ifdef DEBUG_CRYPTO
  ssh_print_hexa("Decrypt IV after", IV, 24);
#endif
}

#endif /* HAS_DES */

/*
 * The table of supported ciphers
 *
 * WARNING: If you modify crypto_struct, you must make sure the order is
 * correct!
 */
static struct crypto_struct ssh_ciphertab[] = {
#ifdef HAS_BLOWFISH
  {
    "blowfish-cbc",
    8,
    sizeof (BF_KEY),
    NULL,
    128,
    blowfish_set_key,
    blowfish_set_key,
    blowfish_encrypt,
    blowfish_decrypt
  },
#endif /* HAS_BLOWFISH */
#ifdef HAS_AES
#ifndef BROKEN_AES_CTR
  {
    "aes128-ctr",
    16,
    sizeof(AES_KEY),
    NULL,
    128,
    aes_set_encrypt_key,
    aes_set_encrypt_key,
    aes_ctr128_encrypt,
    aes_ctr128_encrypt
  },
  {
    "aes192-ctr",
    16,
    sizeof(AES_KEY),
    NULL,
    192,
    aes_set_encrypt_key,
    aes_set_encrypt_key,
    aes_ctr128_encrypt,
    aes_ctr128_encrypt
  },
  {
    "aes256-ctr",
    16,
    sizeof(AES_KEY),
    NULL,
    256,
    aes_set_encrypt_key,
    aes_set_encrypt_key,
    aes_ctr128_encrypt,
    aes_ctr128_encrypt
  },
#endif /* BROKEN_AES_CTR */
  {
    "aes128-cbc",
    16,
    sizeof(AES_KEY),
    NULL,
    128,
    aes_set_encrypt_key,
    aes_set_decrypt_key,
    aes_encrypt,
    aes_decrypt
  },
  {
    "aes192-cbc",
    16,
    sizeof(AES_KEY),
    NULL,
    192,
    aes_set_encrypt_key,
    aes_set_decrypt_key,
    aes_encrypt,
    aes_decrypt
  },
  {
    "aes256-cbc",
    16,
    sizeof(AES_KEY),
    NULL,
    256,
    aes_set_encrypt_key,
    aes_set_decrypt_key,
    aes_encrypt,
    aes_decrypt
  },
#endif /* HAS_AES */
#ifdef HAS_DES
  {
    "3des-cbc",
    8,
    sizeof(DES_key_schedule) * 3,
    NULL,
    192,
    des3_set_key,
    des3_set_key,
    des3_encrypt,
    des3_decrypt
  },
  {
    "3des-cbc-ssh1",
    8,
    sizeof(DES_key_schedule) * 3,
    NULL,
    192,
    des3_set_key,
    des3_set_key,
    des3_1_encrypt,
    des3_1_decrypt
  },
#endif /* HAS_DES */
  {
    NULL,
    0,
    0,
    NULL,
    0,
    NULL,
    NULL,
    NULL,
    NULL
  }
};


struct crypto_struct *ssh_get_ciphertab(){
  return ssh_ciphertab;
}

#endif /* LIBCRYPTO */

