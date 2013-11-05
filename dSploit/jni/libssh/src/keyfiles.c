/*
 * keyfiles.c - private and public key handling for authentication.
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
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

#include "config.h"

#include <errno.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
# if _MSC_VER >= 1400
#  include <io.h>
#  undef open
#  define open _open
#  undef close
#  define close _close
#  undef read
#  define read _read
#  undef unlink
#  define unlink _unlink
# endif /* _MSC_VER */
#else
# include <arpa/inet.h>
#endif

#include "libssh/priv.h"
#include "libssh/buffer.h"
#include "libssh/keyfiles.h"
#include "libssh/session.h"
#include "libssh/wrapper.h"
#include "libssh/misc.h"
#include "libssh/keys.h"

/*todo: remove this include */
#include "libssh/string.h"


#ifdef HAVE_LIBGCRYPT
#include <gcrypt.h>
#elif defined HAVE_LIBCRYPTO
#include <openssl/pem.h>
#include <openssl/dsa.h>
#include <openssl/err.h>
#include <openssl/rsa.h>
#endif /* HAVE_LIBCRYPTO */

#define MAXLINESIZE 80
#define RSA_HEADER_BEGIN "-----BEGIN RSA PRIVATE KEY-----"
#define RSA_HEADER_END "-----END RSA PRIVATE KEY-----"
#define DSA_HEADER_BEGIN "-----BEGIN DSA PRIVATE KEY-----"
#define DSA_HEADER_END "-----END DSA PRIVATE KEY-----"

#ifdef HAVE_LIBGCRYPT

#define MAX_KEY_SIZE 32
#define MAX_PASSPHRASE_SIZE 1024
#define ASN1_INTEGER 2
#define ASN1_SEQUENCE 48
#define PKCS5_SALT_LEN 8

static int load_iv(char *header, unsigned char *iv, int iv_len) {
  int i;
  int j;
  int k;

  memset(iv, 0, iv_len);
  for (i = 0; i < iv_len; i++) {
    if ((header[2*i] >= '0') && (header[2*i] <= '9'))
      j = header[2*i] - '0';
    else if ((header[2*i] >= 'A') && (header[2*i] <= 'F'))
      j = header[2*i] - 'A' + 10;
    else if ((header[2*i] >= 'a') && (header[2*i] <= 'f'))
      j = header[2*i] - 'a' + 10;
    else
      return -1;
    if ((header[2*i+1] >= '0') && (header[2*i+1] <= '9'))
      k = header[2*i+1] - '0';
    else if ((header[2*i+1] >= 'A') && (header[2*i+1] <= 'F'))
      k = header[2*i+1] - 'A' + 10;
    else if ((header[2*i+1] >= 'a') && (header[2*i+1] <= 'f'))
      k = header[2*i+1] - 'a' + 10;
    else
      return -1;
    iv[i] = (j << 4) + k;
  }
  return 0;
}

static uint32_t char_to_u32(unsigned char *data, uint32_t size) {
  uint32_t ret;
  uint32_t i;

  for (i = 0, ret = 0; i < size; ret = ret << 8, ret += data[i++])
    ;
  return ret;
}

static uint32_t asn1_get_len(ssh_buffer buffer) {
  uint32_t len;
  unsigned char tmp[4];

  if (buffer_get_data(buffer,tmp,1) == 0) {
    return 0;
  }

  if (tmp[0] > 127) {
    len = tmp[0] & 127;
    if (len > 4) {
      return 0; /* Length doesn't fit in u32. Can this really happen? */
    }
    if (buffer_get_data(buffer,tmp,len) == 0) {
      return 0;
    }
    len = char_to_u32(tmp, len);
  } else {
    len = char_to_u32(tmp, 1);
  }

  return len;
}

static ssh_string asn1_get_int(ssh_buffer buffer) {
  ssh_string str;
  unsigned char type;
  uint32_t size;

  if (buffer_get_data(buffer, &type, 1) == 0 || type != ASN1_INTEGER) {
    return NULL;
  }
  size = asn1_get_len(buffer);
  if (size == 0) {
    return NULL;
  }

  str = ssh_string_new(size);
  if (str == NULL) {
    return NULL;
  }

  if (buffer_get_data(buffer, str->string, size) == 0) {
    ssh_string_free(str);
    return NULL;
  }

  return str;
}

static int asn1_check_sequence(ssh_buffer buffer) {
  unsigned char *j = NULL;
  unsigned char tmp;
  int i;
  uint32_t size;
  uint32_t padding;

  if (buffer_get_data(buffer, &tmp, 1) == 0 || tmp != ASN1_SEQUENCE) {
    return 0;
  }

  size = asn1_get_len(buffer);
  if ((padding = ssh_buffer_get_len(buffer) - buffer->pos - size) > 0) {
    for (i = ssh_buffer_get_len(buffer) - buffer->pos - size,
         j = (unsigned char*)ssh_buffer_get_begin(buffer) + size + buffer->pos;
         i;
         i--, j++)
    {
      if (*j != padding) {                   /* padding is allowed */
        return 0;                            /* but nothing else */
      }
    }
  }

  return 1;
}

static int read_line(char *data, unsigned int len, FILE *fp) {
  char tmp;
  unsigned int i;

  for (i = 0; fread(&tmp, 1, 1, fp) && tmp != '\n' && i < len; data[i++] = tmp)
    ;
  if (tmp == '\n') {
    return i;
  }

  if (i >= len) {
    return -1;
  }

  return 0;
}

static int passphrase_to_key(char *data, unsigned int datalen,
    unsigned char *salt, unsigned char *key, unsigned int keylen) {
  MD5CTX md;
  unsigned char digest[MD5_DIGEST_LEN] = {0};
  unsigned int i;
  unsigned int j;
  unsigned int md_not_empty;

  for (j = 0, md_not_empty = 0; j < keylen; ) {
    md = md5_init();
    if (md == NULL) {
      return -1;
    }

    if (md_not_empty) {
      md5_update(md, digest, MD5_DIGEST_LEN);
    } else {
      md_not_empty = 1;
    }

    md5_update(md, data, datalen);
    if (salt) {
      md5_update(md, salt, PKCS5_SALT_LEN);
    }
    md5_final(digest, md);

    for (i = 0; j < keylen && i < MD5_DIGEST_LEN; j++, i++) {
      if (key) {
        key[j] = digest[i];
      }
    }
  }

  return 0;
}

static int privatekey_decrypt(int algo, int mode, unsigned int key_len,
                       unsigned char *iv, unsigned int iv_len,
                       ssh_buffer data, ssh_auth_callback cb,
                       void *userdata,
                       const char *desc)
{
  char passphrase[MAX_PASSPHRASE_SIZE] = {0};
  unsigned char key[MAX_KEY_SIZE] = {0};
  unsigned char *tmp = NULL;
  gcry_cipher_hd_t cipher;
  int rc = -1;

  if (!algo) {
    return -1;
  }

  if (cb) {
    rc = (*cb)(desc, passphrase, MAX_PASSPHRASE_SIZE, 0, 0, userdata);
    if (rc < 0) {
      return -1;
    }
  } else if (cb == NULL && userdata != NULL) {
    snprintf(passphrase, MAX_PASSPHRASE_SIZE, "%s", (char *) userdata);
  }

  if (passphrase_to_key(passphrase, strlen(passphrase), iv, key, key_len) < 0) {
    return -1;
  }

  if (gcry_cipher_open(&cipher, algo, mode, 0)
      || gcry_cipher_setkey(cipher, key, key_len)
      || gcry_cipher_setiv(cipher, iv, iv_len)
      || (tmp = malloc(ssh_buffer_get_len(data) * sizeof (char))) == NULL
      || gcry_cipher_decrypt(cipher, tmp, ssh_buffer_get_len(data),
                       ssh_buffer_get_begin(data), ssh_buffer_get_len(data))) {
    gcry_cipher_close(cipher);
    return -1;
  }

  memcpy(ssh_buffer_get_begin(data), tmp, ssh_buffer_get_len(data));

  SAFE_FREE(tmp);
  gcry_cipher_close(cipher);

  return 0;
}

static int privatekey_dek_header(char *header, unsigned int header_len,
    int *algo, int *mode, unsigned int *key_len, unsigned char **iv,
    unsigned int *iv_len) {
  unsigned int iv_pos;

  if (header_len > 13 && !strncmp("DES-EDE3-CBC", header, 12))
  {
    *algo = GCRY_CIPHER_3DES;
    iv_pos = 13;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 24;
    *iv_len = 8;
  }
  else if (header_len > 8 && !strncmp("DES-CBC", header, 7))
  {
    *algo = GCRY_CIPHER_DES;
    iv_pos = 8;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 8;
    *iv_len = 8;
  }
  else if (header_len > 12 && !strncmp("AES-128-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES128;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 16;
    *iv_len = 16;
  }
  else if (header_len > 12 && !strncmp("AES-192-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES192;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 24;
    *iv_len = 16;
  }
  else if (header_len > 12 && !strncmp("AES-256-CBC", header, 11))
  {
    *algo = GCRY_CIPHER_AES256;
    iv_pos = 12;
    *mode = GCRY_CIPHER_MODE_CBC;
    *key_len = 32;
    *iv_len = 16;
  } else {
    return -1;
  }

  *iv = malloc(*iv_len);
  if (*iv == NULL) {
    return -1;
  }

  return load_iv(header + iv_pos, *iv, *iv_len);
}

static ssh_buffer privatekey_file_to_buffer(FILE *fp, int type,
    ssh_auth_callback cb, void *userdata, const char *desc) {
  ssh_buffer buffer = NULL;
  ssh_buffer out = NULL;
  char buf[MAXLINESIZE] = {0};
  unsigned char *iv = NULL;
  const char *header_begin;
  const char *header_end;
  unsigned int header_begin_size;
  unsigned int header_end_size;
  unsigned int key_len = 0;
  unsigned int iv_len = 0;
  int algo = 0;
  int mode = 0;
  int len;

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    return NULL;
  }

  switch(type) {
    case SSH_KEYTYPE_DSS:
      header_begin = DSA_HEADER_BEGIN;
      header_end = DSA_HEADER_END;
      break;
    case SSH_KEYTYPE_RSA:
      header_begin = RSA_HEADER_BEGIN;
      header_end = RSA_HEADER_END;
      break;
    default:
      ssh_buffer_free(buffer);
      return NULL;
  }

  header_begin_size = strlen(header_begin);
  header_end_size = strlen(header_end);

  while (read_line(buf, MAXLINESIZE, fp) &&
      strncmp(buf, header_begin, header_begin_size))
    ;

  len = read_line(buf, MAXLINESIZE, fp);
  if (len > 11 && strncmp("Proc-Type: 4,ENCRYPTED", buf, 11) == 0) {
    len = read_line(buf, MAXLINESIZE, fp);
    if (len > 10 && strncmp("DEK-Info: ", buf, 10) == 0) {
      if ((privatekey_dek_header(buf + 10, len - 10, &algo, &mode, &key_len,
                                 &iv, &iv_len) < 0)
          || read_line(buf, MAXLINESIZE, fp)) {
        ssh_buffer_free(buffer);
        SAFE_FREE(iv);
        return NULL;
      }
    } else {
      ssh_buffer_free(buffer);
      SAFE_FREE(iv);
      return NULL;
    }
  } else {
    if (buffer_add_data(buffer, buf, len) < 0) {
      ssh_buffer_free(buffer);
      SAFE_FREE(iv);
      return NULL;
    }
  }

  while ((len = read_line(buf,MAXLINESIZE,fp)) &&
      strncmp(buf, header_end, header_end_size) != 0) {
    if (len == -1) {
      ssh_buffer_free(buffer);
      SAFE_FREE(iv);
      return NULL;
    }
    if (buffer_add_data(buffer, buf, len) < 0) {
      ssh_buffer_free(buffer);
      SAFE_FREE(iv);
      return NULL;
    }
  }

  if (strncmp(buf,header_end,header_end_size) != 0) {
    ssh_buffer_free(buffer);
    SAFE_FREE(iv);
    return NULL;
  }

  if (buffer_add_data(buffer, "\0", 1) < 0) {
    ssh_buffer_free(buffer);
    SAFE_FREE(iv);
    return NULL;
  }

  out = base64_to_bin(ssh_buffer_get_begin(buffer));
  ssh_buffer_free(buffer);
  if (out == NULL) {
    SAFE_FREE(iv);
    return NULL;
  }

  if (algo) {
    if (privatekey_decrypt(algo, mode, key_len, iv, iv_len, out,
          cb, userdata, desc) < 0) {
      ssh_buffer_free(out);
      SAFE_FREE(iv);
      return NULL;
    }
  }
  SAFE_FREE(iv);

  return out;
}

static int read_rsa_privatekey(FILE *fp, gcry_sexp_t *r,
    ssh_auth_callback cb, void *userdata, const char *desc) {
  ssh_string n = NULL;
  ssh_string e = NULL;
  ssh_string d = NULL;
  ssh_string p = NULL;
  ssh_string q = NULL;
  ssh_string unused1 = NULL;
  ssh_string unused2 = NULL;
  ssh_string u = NULL;
  ssh_string v = NULL;
  ssh_buffer buffer = NULL;
  int rc = 1;

  buffer = privatekey_file_to_buffer(fp, SSH_KEYTYPE_RSA, cb, userdata, desc);
  if (buffer == NULL) {
    return 0;
  }

  if (!asn1_check_sequence(buffer)) {
    ssh_buffer_free(buffer);
    return 0;
  }

  v = asn1_get_int(buffer);
  if (ntohl(v->size) != 1 || v->string[0] != 0) {
    ssh_buffer_free(buffer);
    return 0;
  }

  n = asn1_get_int(buffer);
  e = asn1_get_int(buffer);
  d = asn1_get_int(buffer);
  q = asn1_get_int(buffer);
  p = asn1_get_int(buffer);
  unused1 = asn1_get_int(buffer);
  unused2 = asn1_get_int(buffer);
  u = asn1_get_int(buffer);

  ssh_buffer_free(buffer);

  if (n == NULL || e == NULL || d == NULL || p == NULL || q == NULL ||
      unused1 == NULL || unused2 == NULL|| u == NULL) {
    rc = 0;
    goto error;
  }

  if (gcry_sexp_build(r, NULL,
      "(private-key(rsa(n %b)(e %b)(d %b)(p %b)(q %b)(u %b)))",
      ntohl(n->size), n->string,
      ntohl(e->size), e->string,
      ntohl(d->size), d->string,
      ntohl(p->size), p->string,
      ntohl(q->size), q->string,
      ntohl(u->size), u->string)) {
    rc = 0;
  }

error:
  ssh_string_free(n);
  ssh_string_free(e);
  ssh_string_free(d);
  ssh_string_free(p);
  ssh_string_free(q);
  ssh_string_free(unused1);
  ssh_string_free(unused2);
  ssh_string_free(u);
  ssh_string_free(v);

  return rc;
}

static int read_dsa_privatekey(FILE *fp, gcry_sexp_t *r, ssh_auth_callback cb,
    void *userdata, const char *desc) {
  ssh_buffer buffer = NULL;
  ssh_string p = NULL;
  ssh_string q = NULL;
  ssh_string g = NULL;
  ssh_string y = NULL;
  ssh_string x = NULL;
  ssh_string v = NULL;
  int rc = 1;

  buffer = privatekey_file_to_buffer(fp, SSH_KEYTYPE_DSS, cb, userdata, desc);
  if (buffer == NULL) {
    return 0;
  }

  if (!asn1_check_sequence(buffer)) {
    ssh_buffer_free(buffer);
    return 0;
  }

  v = asn1_get_int(buffer);
  if (ntohl(v->size) != 1 || v->string[0] != 0) {
    ssh_buffer_free(buffer);
    return 0;
  }

  p = asn1_get_int(buffer);
  q = asn1_get_int(buffer);
  g = asn1_get_int(buffer);
  y = asn1_get_int(buffer);
  x = asn1_get_int(buffer);
  ssh_buffer_free(buffer);

  if (p == NULL || q == NULL || g == NULL || y == NULL || x == NULL) {
    rc = 0;
    goto error;
  }

  if (gcry_sexp_build(r, NULL,
        "(private-key(dsa(p %b)(q %b)(g %b)(y %b)(x %b)))",
        ntohl(p->size), p->string,
        ntohl(q->size), q->string,
        ntohl(g->size), g->string,
        ntohl(y->size), y->string,
        ntohl(x->size), x->string)) {
    rc = 0;
  }

error:
  ssh_string_free(p);
  ssh_string_free(q);
  ssh_string_free(g);
  ssh_string_free(y);
  ssh_string_free(x);
  ssh_string_free(v);

  return rc;
}
#endif /* HAVE_LIBGCRYPT */

#ifdef HAVE_LIBCRYPTO
static int pem_get_password(char *buf, int size, int rwflag, void *userdata) {
  ssh_session session = userdata;

  /* unused flag */
  (void) rwflag;
  if(buf==NULL)
    return 0;
  memset(buf,'\0',size);
  ssh_log(session, SSH_LOG_RARE,
      "Trying to call external authentication function");

  if (session && session->common.callbacks && session->common.callbacks->auth_function) {
    if (session->common.callbacks->auth_function("Passphrase for private key:", buf, size, 0, 0,
        session->common.callbacks->userdata) < 0) {
      return 0;
    }

    return strlen(buf);
  }

  return 0;
}
#endif /* HAVE_LIBCRYPTO */

static int privatekey_type_from_file(FILE *fp) {
  char buffer[MAXLINESIZE] = {0};

  if (!fgets(buffer, MAXLINESIZE, fp)) {
    return 0;
  }
  fseek(fp, 0, SEEK_SET);
  if (strncmp(buffer, DSA_HEADER_BEGIN, strlen(DSA_HEADER_BEGIN)) == 0) {
    return SSH_KEYTYPE_DSS;
  }
  if (strncmp(buffer, RSA_HEADER_BEGIN, strlen(RSA_HEADER_BEGIN)) == 0) {
    return SSH_KEYTYPE_RSA;
  }
  return 0;
}

/**
 * @addtogroup libssh_auth
 *
 * @{
 */

/**
 * @brief Reads a SSH private key from a file.
 *
 * @param[in] session  The SSH Session to use.
 *
 * @param[in] filename The filename of the the private key.
 *
 * @param[in] type     The type of the private key. This could be SSH_KEYTYPE_DSS or
 *                     SSH_KEYTYPE_RSA. Pass 0 to automatically detect the type.
 *
 * @param[in] passphrase The passphrase to decrypt the private key. Set to null
 *                       if none is needed or it is unknown.
 *
 * @return              A private_key object containing the private key, or
 *                       NULL on error.
 * @see privatekey_free()
 * @see publickey_from_privatekey()
 */
ssh_private_key privatekey_from_file(ssh_session session, const char *filename,
    int type, const char *passphrase) {
  ssh_private_key privkey = NULL;
  FILE *file = NULL;
#ifdef HAVE_LIBGCRYPT
  ssh_auth_callback auth_cb = NULL;
  void *auth_ud = NULL;

  gcry_sexp_t dsa = NULL;
  gcry_sexp_t rsa = NULL;
  int valid;
#elif defined HAVE_LIBCRYPTO
  DSA *dsa = NULL;
  RSA *rsa = NULL;
  BIO *bio = NULL;
#endif
  /* TODO Implement to read both DSA and RSA at once. */

  /* needed for openssl initialization */
  if (ssh_init() < 0) {
    return NULL;
  }

  ssh_log(session, SSH_LOG_RARE, "Trying to open %s", filename);
  file = fopen(filename,"r");
  if (file == NULL) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Error opening %s: %s", filename, strerror(errno));
    return NULL;
  }

#ifdef HAVE_LIBCRYPTO
  bio = BIO_new_file(filename,"r");
  if (bio == NULL) {
	  fclose(file);
      ssh_set_error(session, SSH_FATAL, "Could not create BIO.");
      return NULL;
  }
#endif

  ssh_log(session, SSH_LOG_RARE, "Trying to read %s, passphase=%s, authcb=%s",
      filename, passphrase ? "true" : "false",
      session->common.callbacks && session->common.callbacks->auth_function ? "true" : "false");

  if (type == 0) {
    type = privatekey_type_from_file(file);
    if (type == 0) {
      fclose(file);
      ssh_set_error(session, SSH_FATAL, "Invalid private key file.");
      return NULL;
    }
  }
  switch (type) {
    case SSH_KEYTYPE_DSS:
      if (passphrase == NULL) {
#ifdef HAVE_LIBGCRYPT
        if (session->common.callbacks && session->common.callbacks->auth_function) {
          auth_cb = session->common.callbacks->auth_function;
          auth_ud = session->common.callbacks->userdata;

          valid = read_dsa_privatekey(file, &dsa, auth_cb, auth_ud,
              "Passphrase for private key:");
        } else { /* authcb */
          valid = read_dsa_privatekey(file, &dsa, NULL, NULL, NULL);
        } /* authcb */
      } else { /* passphrase */
        valid = read_dsa_privatekey(file, &dsa, NULL,
            (void *) passphrase, NULL);
      }

      fclose(file);

      if (!valid) {
        ssh_set_error(session, SSH_FATAL, "Parsing private key %s", filename);
#elif defined HAVE_LIBCRYPTO
        if (session->common.callbacks && session->common.callbacks->auth_function) {
          dsa = PEM_read_bio_DSAPrivateKey(bio, NULL, pem_get_password, session);
        } else { /* authcb */
          /* openssl uses its own callback to get the passphrase here */
          dsa = PEM_read_bio_DSAPrivateKey(bio, NULL, NULL, NULL);
        } /* authcb */
      } else { /* passphrase */
        dsa = PEM_read_bio_DSAPrivateKey(bio, NULL, NULL, (void *) passphrase);
      }

	  BIO_free(bio);
      fclose(file);
      if (dsa == NULL) {
        ssh_set_error(session, SSH_FATAL,
            "Parsing private key %s: %s",
            filename, ERR_error_string(ERR_get_error(), NULL));
#endif
        return NULL;
      }
      break;
    case SSH_KEYTYPE_RSA:
      if (passphrase == NULL) {
#ifdef HAVE_LIBGCRYPT
        if (session->common.callbacks && session->common.callbacks->auth_function) {
          auth_cb = session->common.callbacks->auth_function;
          auth_ud = session->common.callbacks->userdata;
          valid = read_rsa_privatekey(file, &rsa, auth_cb, auth_ud,
              "Passphrase for private key:");
        } else { /* authcb */
          valid = read_rsa_privatekey(file, &rsa, NULL, NULL, NULL);
        } /* authcb */
      } else { /* passphrase */
        valid = read_rsa_privatekey(file, &rsa, NULL,
            (void *) passphrase, NULL);
      }

      fclose(file);

      if (!valid) {
        ssh_set_error(session,SSH_FATAL, "Parsing private key %s", filename);
#elif defined HAVE_LIBCRYPTO
        if (session->common.callbacks && session->common.callbacks->auth_function) {
			rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, pem_get_password, session);
        } else { /* authcb */
          /* openssl uses its own callback to get the passphrase here */
          rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);
        } /* authcb */
      } else { /* passphrase */
        rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, (void *) passphrase);
      }

	  BIO_free(bio);
      fclose(file);

      if (rsa == NULL) {
        ssh_set_error(session, SSH_FATAL,
            "Parsing private key %s: %s",
            filename, ERR_error_string(ERR_get_error(),NULL));
#endif
        return NULL;
      }
      break;
    default:
#ifdef HAVE_LIBCRYPTO
	  BIO_free(bio);
#endif
      fclose(file);
      ssh_set_error(session, SSH_FATAL, "Invalid private key type %d", type);
      return NULL;
  } /* switch */

  privkey = malloc(sizeof(struct ssh_private_key_struct));
  if (privkey == NULL) {
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_release(dsa);
    gcry_sexp_release(rsa);
#elif defined HAVE_LIBCRYPTO
    DSA_free(dsa);
    RSA_free(rsa);
#endif
    return NULL;
  }
  ZERO_STRUCTP(privkey);
  privkey->type = type;
  privkey->dsa_priv = dsa;
  privkey->rsa_priv = rsa;

  return privkey;
}

/**
 * @brief returns the type of a private key
 * @param[in] privatekey the private key handle
 * @returns one of SSH_KEYTYPE_RSA,SSH_KEYTYPE_DSS,SSH_KEYTYPE_RSA1
 * @returns SSH_KEYTYPE_UNKNOWN if the type is unknown
 * @see privatekey_from_file
 * @see ssh_userauth_offer_pubkey
 */
enum ssh_keytypes_e ssh_privatekey_type(ssh_private_key privatekey){
  if (privatekey==NULL)
    return SSH_KEYTYPE_UNKNOWN;
  return privatekey->type;
}

/* same that privatekey_from_file() but without any passphrase things. */
ssh_private_key _privatekey_from_file(void *session, const char *filename,
    int type) {
  ssh_private_key privkey = NULL;
#ifdef HAVE_LIBGCRYPT
  FILE *file = NULL;
  gcry_sexp_t dsa = NULL;
  gcry_sexp_t rsa = NULL;
  int valid;
#elif defined HAVE_LIBCRYPTO
  DSA *dsa = NULL;
  RSA *rsa = NULL;
  BIO *bio = NULL;
#endif

#ifdef HAVE_LIBGCRYPT
  file = fopen(filename,"r");
  if (file == NULL) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Error opening %s: %s", filename, strerror(errno));
    return NULL;
  }
#elif defined HAVE_LIBCRYPTO
  bio = BIO_new_file(filename,"r");
  if (bio == NULL) {
      ssh_set_error(session, SSH_FATAL, "Could not create BIO.");
      return NULL;
  }
#endif

  switch (type) {
    case SSH_KEYTYPE_DSS:
#ifdef HAVE_LIBGCRYPT
      valid = read_dsa_privatekey(file, &dsa, NULL, NULL, NULL);

      fclose(file);

      if (!valid) {
        ssh_set_error(session, SSH_FATAL, "Parsing private key %s", filename);
#elif defined HAVE_LIBCRYPTO
      dsa = PEM_read_bio_DSAPrivateKey(bio, NULL, NULL, NULL);

      BIO_free(bio);

      if (dsa == NULL) {
        ssh_set_error(session, SSH_FATAL,
            "Parsing private key %s: %s",
            filename, ERR_error_string(ERR_get_error(), NULL));
#else
      {
#endif
        return NULL;
      }
      break;
    case SSH_KEYTYPE_RSA:
#ifdef HAVE_LIBGCRYPT
      valid = read_rsa_privatekey(file, &rsa, NULL, NULL, NULL);

      fclose(file);

      if (!valid) {
        ssh_set_error(session, SSH_FATAL, "Parsing private key %s", filename);
#elif defined HAVE_LIBCRYPTO
      rsa = PEM_read_bio_RSAPrivateKey(bio, NULL, NULL, NULL);

      BIO_free(bio);

      if (rsa == NULL) {
        ssh_set_error(session, SSH_FATAL,
            "Parsing private key %s: %s",
            filename, ERR_error_string(ERR_get_error(), NULL));
#else
      {
#endif
        return NULL;
      }
      break;
    default:
#ifdef HAVE_LIBGCRYPT
        fclose(file);
#elif defined HAVE_LIBCRYPTO
        BIO_free(bio);
#endif
        ssh_set_error(session, SSH_FATAL, "Invalid private key type %d", type);
        return NULL;
  }

  privkey = malloc(sizeof(struct ssh_private_key_struct));
  if (privkey == NULL) {
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_release(dsa);
    gcry_sexp_release(rsa);
#elif defined HAVE_LIBCRYPTO
    DSA_free(dsa);
    RSA_free(rsa);
#endif
    return NULL;
  }

  privkey->type = type;
  privkey->dsa_priv = dsa;
  privkey->rsa_priv = rsa;

  return privkey;
}

/**
 * @brief Deallocate a private key object.
 *
 * @param[in]  prv      The private_key object to free.
 */
void privatekey_free(ssh_private_key prv) {
  if (prv == NULL) {
    return;
  }

#ifdef HAVE_LIBGCRYPT
  gcry_sexp_release(prv->dsa_priv);
  gcry_sexp_release(prv->rsa_priv);
#elif defined HAVE_LIBCRYPTO
  DSA_free(prv->dsa_priv);
  RSA_free(prv->rsa_priv);
#endif
  memset(prv, 0, sizeof(struct ssh_private_key_struct));
  SAFE_FREE(prv);
}

/**
 * @brief Write a public key to a file.
 *
 * @param[in]  session  The ssh session to use.
 *
 * @param[in]  file     The filename to write the key into.
 *
 * @param[in]  pubkey   The public key to write.
 *
 * @param[in]  type     The type of the public key.
 *
 * @return              0 on success, -1 on error.
 */
int ssh_publickey_to_file(ssh_session session, const char *file,
    ssh_string pubkey, int type) {
  FILE *fp;
  char *user;
  char buffer[1024];
  char host[256];
  unsigned char *pubkey_64;
  size_t len;
  int rc;
  if(session==NULL)
  	return SSH_ERROR;
  if(file==NULL || pubkey==NULL){
  	ssh_set_error(session, SSH_FATAL, "Invalid parameters");
  	return SSH_ERROR;
  }
  pubkey_64 = bin_to_base64(pubkey->string, ssh_string_len(pubkey));
  if (pubkey_64 == NULL) {
    return SSH_ERROR;
  }

  user = ssh_get_local_username(session);
  if (user == NULL) {
    SAFE_FREE(pubkey_64);
    return SSH_ERROR;
  }

  rc = gethostname(host, sizeof(host));
  if (rc < 0) {
    SAFE_FREE(user);
    SAFE_FREE(pubkey_64);
    return SSH_ERROR;
  }

  snprintf(buffer, sizeof(buffer), "%s %s %s@%s\n",
      ssh_type_to_char(type),
      pubkey_64,
      user,
      host);

  SAFE_FREE(pubkey_64);
  SAFE_FREE(user);

  ssh_log(session, SSH_LOG_RARE, "Trying to write public key file: %s", file);
  ssh_log(session, SSH_LOG_PACKET, "public key file content: %s", buffer);

  fp = fopen(file, "w+");
  if (fp == NULL) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Error opening %s: %s", file, strerror(errno));
    return SSH_ERROR;
  }

  len = strlen(buffer);
  if (fwrite(buffer, len, 1, fp) != 1 || ferror(fp)) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Unable to write to %s", file);
    fclose(fp);
    unlink(file);
    return SSH_ERROR;
  }

  fclose(fp);
  return SSH_OK;
}

/**
 * @brief Retrieve a public key from a file.
 *
 * @param[in]  session  The SSH session to use.
 *
 * @param[in]  filename The filename of the public key.
 *
 * @param[out] type     The Pointer to a integer. If it is not NULL, it will
 *                      contain the type of the key after execution.
 *
 * @return              A SSH String containing the public key, or NULL if it
 *                      failed.
 *
 * @see string_free()
 * @see publickey_from_privatekey()
 */
ssh_string publickey_from_file(ssh_session session, const char *filename,
    int *type) {
  ssh_buffer buffer = NULL;
  char buf[4096] = {0};
  ssh_string str = NULL;
  char *ptr = NULL;
  int key_type;
  int fd = -1;
  int r;

  fd = open(filename, O_RDONLY);
  if (fd < 0) {
    ssh_set_error(session, SSH_REQUEST_DENIED, "Public key file doesn't exist");
    return NULL;
  }

  if (read(fd, buf, 8) != 8) {
    close(fd);
    ssh_set_error(session, SSH_REQUEST_DENIED, "Invalid public key file");
    return NULL;
  }

  buf[7] = '\0';

  key_type = ssh_type_from_name(buf);
  if (key_type == -1) {
    close(fd);
    ssh_set_error(session, SSH_REQUEST_DENIED, "Invalid public key file");
    return NULL;
  }

  r = read(fd, buf, sizeof(buf) - 1);
  close(fd);
  if (r <= 0) {
    ssh_set_error(session, SSH_REQUEST_DENIED, "Invalid public key file");
    return NULL;
  }

  buf[r] = 0;
  ptr = strchr(buf, ' ');

  /* eliminate the garbage at end of file */
  if (ptr) {
    *ptr = '\0';
  }

  buffer = base64_to_bin(buf);
  if (buffer == NULL) {
    ssh_set_error(session, SSH_REQUEST_DENIED, "Invalid public key file");
    return NULL;
  }

  str = ssh_string_new(buffer_get_rest_len(buffer));
  if (str == NULL) {
    ssh_set_error(session, SSH_FATAL, "Not enough space");
    ssh_buffer_free(buffer);
    return NULL;
  }

  ssh_string_fill(str, buffer_get_rest(buffer), buffer_get_rest_len(buffer));
  ssh_buffer_free(buffer);

  if (type) {
    *type = key_type;
  }

  return str;
}

/**
 * @brief Try to read the public key from a given file.
 *
 * @param[in]  session  The ssh session to use.
 *
 * @param[in]  keyfile  The name of the private keyfile.
 *
 * @param[out] publickey A ssh_string to store the public key.
 *
 * @param[out] type     A pointer to an integer to store the type.
 *
 * @return              0 on success, -1 on error or the private key doesn't
 *                      exist, 1 if the public key doesn't exist.
 */
int ssh_try_publickey_from_file(ssh_session session, const char *keyfile,
    ssh_string *publickey, int *type) {
  char *pubkey_file;
  size_t len;
  ssh_string pubkey_string;
  int pubkey_type;

  if (session == NULL || keyfile == NULL || publickey == NULL || type == NULL) {
    return -1;
  }

  if (session->sshdir == NULL) {
    if (ssh_options_apply(session) < 0) {
      return -1;
    }
  }

  ssh_log(session, SSH_LOG_PACKET, "Trying to open privatekey %s", keyfile);
  if (!ssh_file_readaccess_ok(keyfile)) {
    ssh_log(session, SSH_LOG_PACKET, "Failed to open privatekey %s", keyfile);
    return -1;
  }

  len = strlen(keyfile) + 5;
  pubkey_file = malloc(len);
  if (pubkey_file == NULL) {
    return -1;
  }
  snprintf(pubkey_file, len, "%s.pub", keyfile);

  ssh_log(session, SSH_LOG_PACKET, "Trying to open publickey %s",
                                   pubkey_file);
  if (!ssh_file_readaccess_ok(pubkey_file)) {
    ssh_log(session, SSH_LOG_PACKET, "Failed to open publickey %s",
                                     pubkey_file);
    SAFE_FREE(pubkey_file);
    return 1;
  }

  ssh_log(session, SSH_LOG_PACKET, "Success opening public and private key");

  /*
   * We are sure both the private and public key file is readable. We return
   * the public as a string, and the private filename as an argument
   */
  pubkey_string = publickey_from_file(session, pubkey_file, &pubkey_type);
  if (pubkey_string == NULL) {
    ssh_log(session, SSH_LOG_PACKET,
        "Wasn't able to open public key file %s: %s",
        pubkey_file,
        ssh_get_error(session));
    SAFE_FREE(pubkey_file);
    return -1;
  }

  SAFE_FREE(pubkey_file);

  *publickey = pubkey_string;
  *type = pubkey_type;

  return 0;
}

ssh_string try_publickey_from_file(ssh_session session, struct ssh_keys_struct keytab,
    char **privkeyfile, int *type) {
  const char *priv;
  const char *pub;
  char *new;
  ssh_string pubkey;

  pub = keytab.publickey;
  if (pub == NULL) {
    return NULL;
  }
  priv = keytab.privatekey;
  if (priv == NULL) {
    return NULL;
  }

  if (session->sshdir == NULL) {
    if (ssh_options_apply(session) < 0) {
      return NULL;
    }
  }

  ssh_log(session, SSH_LOG_PACKET, "Trying to open publickey %s", pub);
  if (!ssh_file_readaccess_ok(pub)) {
    ssh_log(session, SSH_LOG_PACKET, "Failed to open publickey %s", pub);
    return NULL;
  }

  ssh_log(session, SSH_LOG_PACKET, "Trying to open privatekey %s", priv);
  if (!ssh_file_readaccess_ok(priv)) {
    ssh_log(session, SSH_LOG_PACKET, "Failed to open privatekey %s", priv);
    return NULL;
  }

  ssh_log(session, SSH_LOG_PACKET, "Success opening public and private key");

  /*
   * We are sure both the private and public key file is readable. We return
   * the public as a string, and the private filename as an argument
   */
  pubkey = publickey_from_file(session, pub, type);
  if (pubkey == NULL) {
    ssh_log(session, SSH_LOG_PACKET,
        "Wasn't able to open public key file %s: %s",
        pub,
        ssh_get_error(session));
    return NULL;
  }

  new = realloc(*privkeyfile, strlen(priv) + 1);
  if (new == NULL) {
    ssh_string_free(pubkey);
    return NULL;
  }

  strcpy(new, priv);
  *privkeyfile = new;

  return pubkey;
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
