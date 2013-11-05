/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2010 by Aris Adamantiadis
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

#ifndef PKI_H_
#define PKI_H_

#define SSH_KEY_FLAG_EMPTY 0
#define SSH_KEY_FLAG_PUBLIC  1
#define SSH_KEY_FLAG_PRIVATE 2

struct ssh_key_struct {
    enum ssh_keytypes_e type;
    int flags;
    const char *type_c; /* Don't free it ! it is static */
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t dsa;
    gcry_sexp_t rsa;
#elif HAVE_LIBCRYPTO
    DSA *dsa;
    RSA *rsa;
#endif
};

ssh_key ssh_key_new (void);
void ssh_key_clean (ssh_key key);
enum ssh_keytypes_e ssh_key_type(ssh_key key);
int ssh_key_import_private(ssh_key key, ssh_session session,
    const char *filename, const char *passphrase);
void ssh_key_free (ssh_key key);

#endif /* PKI_H_ */
