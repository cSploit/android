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

#ifndef AUTH_H_
#define AUTH_H_
#include "config.h"
#include "libssh/callbacks.h"

SSH_PACKET_CALLBACK(ssh_packet_userauth_banner);
SSH_PACKET_CALLBACK(ssh_packet_userauth_failure);
SSH_PACKET_CALLBACK(ssh_packet_userauth_success);
SSH_PACKET_CALLBACK(ssh_packet_userauth_pk_ok);
SSH_PACKET_CALLBACK(ssh_packet_userauth_info_request);

#ifdef WITH_SSH1
void ssh_auth1_handler(ssh_session session, uint8_t type);

/* auth1.c */
int ssh_userauth1_none(ssh_session session, const char *username);
int ssh_userauth1_offer_pubkey(ssh_session session, const char *username,
        int type, ssh_string pubkey);
int ssh_userauth1_password(ssh_session session, const char *username,
        const char *password);


#endif

/** @internal
 * States of authentication in the client-side. They describe
 * what was the last response from the server
 */
enum ssh_auth_state_e {
  /** No authentication asked */
  SSH_AUTH_STATE_NONE=0,
  /** Last authentication response was a partial success */
  SSH_AUTH_STATE_PARTIAL,
  /** Last authentication response was a success */
  SSH_AUTH_STATE_SUCCESS,
  /** Last authentication response was failed */
  SSH_AUTH_STATE_FAILED,
  /** Last authentication was erroneous */
  SSH_AUTH_STATE_ERROR,
  /** Last state was a keyboard-interactive ask for info */
  SSH_AUTH_STATE_INFO,
  /** Last state was a public key accepted for authentication */
  SSH_AUTH_STATE_PK_OK,
  /** We asked for a keyboard-interactive authentication */
  SSH_AUTH_STATE_KBDINT_SENT

};

/** @internal
 * @brief states of the authentication service request
 */
enum ssh_auth_service_state_e {
  /** initial state */
  SSH_AUTH_SERVICE_NONE=0,
  /** Authentication service request packet sent */
  SSH_AUTH_SERVICE_SENT,
  /** Service accepted */
  SSH_AUTH_SERVICE_ACCEPTED,
  /** Access to service denied (fatal) */
  SSH_AUTH_SERVICE_DENIED,
  /** Specific to SSH1 */
  SSH_AUTH_SERVICE_USER_SENT
};

#endif /* AUTH_H_ */
