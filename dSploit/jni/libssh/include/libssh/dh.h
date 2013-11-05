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

#ifndef DH_H_
#define DH_H_
#include "config.h"

/* DH key generation */
#include "libssh/keys.h"

void ssh_print_bignum(const char *which,bignum num);
int dh_generate_e(ssh_session session);
int dh_generate_f(ssh_session session);
int dh_generate_x(ssh_session session);
int dh_generate_y(ssh_session session);

int ssh_crypto_init(void);
void ssh_crypto_finalize(void);

ssh_string dh_get_e(ssh_session session);
ssh_string dh_get_f(ssh_session session);
int dh_import_f(ssh_session session,ssh_string f_string);
int dh_import_e(ssh_session session, ssh_string e_string);
void dh_import_pubkey(ssh_session session,ssh_string pubkey_string);
int dh_build_k(ssh_session session);
int make_sessionid(ssh_session session);
/* add data for the final cookie */
int hashbufin_add_cookie(ssh_session session, unsigned char *cookie);
int hashbufout_add_cookie(ssh_session session);
int generate_session_keys(ssh_session session);
int sig_verify(ssh_session session, ssh_public_key pubkey,
    SIGNATURE *signature, unsigned char *digest, int size);
/* returns 1 if server signature ok, 0 otherwise. The NEXT crypto is checked, not the current one */
int signature_verify(ssh_session session,ssh_string signature);
bignum make_string_bn(ssh_string string);
ssh_string make_bignum_string(bignum num);


#endif /* DH_H_ */
