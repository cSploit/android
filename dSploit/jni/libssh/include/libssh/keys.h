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

#ifndef KEYS_H_
#define KEYS_H_

#include "config.h"
#include "libssh/libssh.h"
#include "libssh/wrapper.h"

struct ssh_public_key_struct {
    int type;
    const char *type_c; /* Don't free it ! it is static */
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t dsa_pub;
    gcry_sexp_t rsa_pub;
#elif HAVE_LIBCRYPTO
    DSA *dsa_pub;
    RSA *rsa_pub;
#endif
};

struct ssh_private_key_struct {
    int type;
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t dsa_priv;
    gcry_sexp_t rsa_priv;
#elif defined HAVE_LIBCRYPTO
    DSA *dsa_priv;
    RSA *rsa_priv;
#endif
};

typedef struct signature_struct {
    int type;
#ifdef HAVE_LIBGCRYPT
    gcry_sexp_t dsa_sign;
    gcry_sexp_t rsa_sign;
#elif defined HAVE_LIBCRYPTO
    DSA_SIG *dsa_sign;
    ssh_string rsa_sign;
#endif
} SIGNATURE;

const char *ssh_type_to_char(int type);
int ssh_type_from_name(const char *name);
ssh_buffer ssh_userauth_build_digest(ssh_session session, ssh_message msg, char *service);

ssh_private_key privatekey_make_dss(ssh_session session, ssh_buffer buffer);
ssh_private_key privatekey_make_rsa(ssh_session session, ssh_buffer buffer,
    const char *type);
ssh_private_key privatekey_from_string(ssh_session session, ssh_string privkey_s);

ssh_public_key publickey_make_dss(ssh_session session, ssh_buffer buffer);
ssh_public_key publickey_make_rsa(ssh_session session, ssh_buffer buffer, int type);
ssh_public_key publickey_from_string(ssh_session session, ssh_string pubkey_s);
SIGNATURE *signature_from_string(ssh_session session, ssh_string signature,ssh_public_key pubkey,int needed_type);
void signature_free(SIGNATURE *sign);
ssh_string ssh_do_sign_with_agent(struct ssh_session_struct *session,
    struct ssh_buffer_struct *buf, struct ssh_public_key_struct *publickey);
ssh_string ssh_do_sign(ssh_session session,ssh_buffer sigbuf,
        ssh_private_key privatekey);
ssh_string ssh_sign_session_id(ssh_session session, ssh_private_key privatekey);
ssh_string ssh_encrypt_rsa1(ssh_session session, ssh_string data, ssh_public_key key);

#endif /* KEYS_H_ */
