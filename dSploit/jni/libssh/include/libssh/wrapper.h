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

#ifndef WRAPPER_H_
#define WRAPPER_H_

#include "config.h"
#include "libssh/libcrypto.h"
#include "libssh/libgcrypt.h"

MD5CTX md5_init(void);
void md5_update(MD5CTX c, const void *data, unsigned long len);
void md5_final(unsigned char *md,MD5CTX c);
SHACTX sha1_init(void);
void sha1_update(SHACTX c, const void *data, unsigned long len);
void sha1_final(unsigned char *md,SHACTX c);
void sha1(unsigned char *digest,int len,unsigned char *hash);
#define HMAC_SHA1 1
#define HMAC_MD5 2
HMACCTX hmac_init(const void *key,int len,int type);
void hmac_update(HMACCTX c, const void *data, unsigned long len);
void hmac_final(HMACCTX ctx,unsigned char *hashmacbuf,unsigned int *len);

int crypt_set_algorithms(ssh_session );
int crypt_set_algorithms_server(ssh_session session);
struct ssh_crypto_struct *crypto_new(void);
void crypto_free(struct ssh_crypto_struct *crypto);


#endif /* WRAPPER_H_ */
