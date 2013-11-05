/*
 * server.c - functions for creating a SSH server
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2004-2005 by Aris Adamantiadis
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
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
# include <winsock2.h>
# include <ws2tcpip.h>

  /*
   * <wspiapi.h> is necessary for getaddrinfo before Windows XP, but it isn't
   * available on some platforms like MinGW.
   */
# ifdef HAVE_WSPIAPI_H
#  include <wspiapi.h>
# endif
#else
# include <netinet/in.h>
#endif

#include "libssh/priv.h"
#include "libssh/libssh.h"
#include "libssh/server.h"
#include "libssh/ssh2.h"
#include "libssh/keyfiles.h"
#include "libssh/buffer.h"
#include "libssh/packet.h"
#include "libssh/socket.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/keys.h"
#include "libssh/dh.h"
#include "libssh/messages.h"

#define set_status(session, status) do {\
        if (session->common.callbacks && session->common.callbacks->connect_status_function) \
            session->common.callbacks->connect_status_function(session->common.callbacks->userdata, status); \
    } while (0)

static int dh_handshake_server(ssh_session session);


/**
 * @addtogroup libssh_server
 *
 * @{
 */

extern char *supported_methods[];
/** @internal
 * This functions sets the Key Exchange protocols to be accepted
 * by the server. They depend on
 * -What the user asked (via options)
 * -What is available (keys)
 * It should then accept the intersection of what the user asked
 * and what is available, and return an error if nothing matches
 */

static int server_set_kex(ssh_session session) {
  KEX *server = &session->server_kex;
  int i, j;
  char *wanted;

  ZERO_STRUCTP(server);
  ssh_get_random(server->cookie, 16, 0);
  if (session->dsa_key != NULL && session->rsa_key != NULL) {
    if (ssh_options_set_algo(session, SSH_HOSTKEYS,
          "ssh-dss,ssh-rsa") < 0) {
      return -1;
    }
  } else if (session->dsa_key != NULL) {
    if (ssh_options_set_algo(session, SSH_HOSTKEYS, "ssh-dss") < 0) {
      return -1;
    }
  } else {
    if (ssh_options_set_algo(session, SSH_HOSTKEYS, "ssh-rsa") < 0) {
      return -1;
    }
  }

  server->methods = (char **) malloc(10 * sizeof(char **));
  if (server->methods == NULL) {
    return -1;
  }

  for (i = 0; i < 10; i++) {
    if ((wanted = session->wanted_methods[i]) == NULL) {
      wanted = supported_methods[i];
    }
    server->methods[i] = strdup(wanted);
    if (server->methods[i] == NULL) {
      for (j = i - 1; j <= 0; j--) {
        SAFE_FREE(server->methods[j]);
      }
      SAFE_FREE(server->methods);
      return -1;
    }
  }

  return 0;
}

SSH_PACKET_CALLBACK(ssh_packet_kexdh_init){
  ssh_string e;
  (void)type;
  (void)user;enter_function();
  ssh_log(session,SSH_LOG_PACKET,"Received SSH_MSG_KEXDH_INIT");
  if(session->dh_handshake_state != DH_STATE_INIT){
    ssh_log(session,SSH_LOG_RARE,"Invalid state for SSH_MSG_KEXDH_INIT");
    goto error;
  }
  e = buffer_get_ssh_string(packet);
  if (e == NULL) {
    ssh_set_error(session, SSH_FATAL, "No e number in client request");
    return -1;
  }
  if (dh_import_e(session, e) < 0) {
    ssh_set_error(session, SSH_FATAL, "Cannot import e number");
    session->session_state=SSH_SESSION_STATE_ERROR;
  } else {
    session->dh_handshake_state=DH_STATE_INIT_SENT;
    dh_handshake_server(session);
  }
  ssh_string_free(e);

  error:
  leave_function();
  return SSH_PACKET_USED;
}

static int dh_handshake_server(ssh_session session) {
  ssh_string f;
  ssh_string pubkey;
  ssh_string sign;
  ssh_public_key pub;
  ssh_private_key prv;

  if (dh_generate_y(session) < 0) {
    ssh_set_error(session, SSH_FATAL, "Could not create y number");
    return -1;
  }
  if (dh_generate_f(session) < 0) {
    ssh_set_error(session, SSH_FATAL, "Could not create f number");
    return -1;
  }

  f = dh_get_f(session);
  if (f == NULL) {
    ssh_set_error(session, SSH_FATAL, "Could not get the f number");
    return -1;
  }

  switch(session->hostkeys){
    case SSH_KEYTYPE_DSS:
      prv = session->dsa_key;
      break;
    case SSH_KEYTYPE_RSA:
      prv = session->rsa_key;
      break;
    default:
      prv = NULL;
  }

  pub = publickey_from_privatekey(prv);
  if (pub == NULL) {
    ssh_set_error(session, SSH_FATAL,
        "Could not get the public key from the private key");
    ssh_string_free(f);
    return -1;
  }
  pubkey = publickey_to_string(pub);
  publickey_free(pub);
  if (pubkey == NULL) {
    ssh_set_error(session, SSH_FATAL, "Not enough space");
    ssh_string_free(f);
    return -1;
  }

  dh_import_pubkey(session, pubkey);
  if (dh_build_k(session) < 0) {
    ssh_set_error(session, SSH_FATAL, "Could not import the public key");
    ssh_string_free(f);
    return -1;
  }

  if (make_sessionid(session) != SSH_OK) {
    ssh_set_error(session, SSH_FATAL, "Could not create a session id");
    ssh_string_free(f);
    return -1;
  }

  sign = ssh_sign_session_id(session, prv);
  if (sign == NULL) {
    ssh_set_error(session, SSH_FATAL, "Could not sign the session id");
    ssh_string_free(f);
    return -1;
  }

  /* Free private keys as they should not be readable after this point */
  if (session->rsa_key) {
    privatekey_free(session->rsa_key);
    session->rsa_key = NULL;
  }
  if (session->dsa_key) {
    privatekey_free(session->dsa_key);
    session->dsa_key = NULL;
  }

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_KEXDH_REPLY) < 0 ||
      buffer_add_ssh_string(session->out_buffer, pubkey) < 0 ||
      buffer_add_ssh_string(session->out_buffer, f) < 0 ||
      buffer_add_ssh_string(session->out_buffer, sign) < 0) {
    ssh_set_error(session, SSH_FATAL, "Not enough space");
    buffer_reinit(session->out_buffer);
    ssh_string_free(f);
    ssh_string_free(sign);
    return -1;
  }
  ssh_string_free(f);
  ssh_string_free(sign);
  if (packet_send(session) == SSH_ERROR) {
    return -1;
  }

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_NEWKEYS) < 0) {
    buffer_reinit(session->out_buffer);
    return -1;
  }

  if (packet_send(session) == SSH_ERROR) {
    return -1;
  }
  ssh_log(session, SSH_LOG_PACKET, "SSH_MSG_NEWKEYS sent");
  session->dh_handshake_state=DH_STATE_NEWKEYS_SENT;

  return 0;
}

/**
 * @internal
 *
 * @brief A function to be called each time a step has been done in the
 * connection.
 */
static void ssh_server_connection_callback(ssh_session session){
	int ssh1,ssh2;
	enter_function();
	switch(session->session_state){
		case SSH_SESSION_STATE_NONE:
		case SSH_SESSION_STATE_CONNECTING:
		case SSH_SESSION_STATE_SOCKET_CONNECTED:
			break;
		case SSH_SESSION_STATE_BANNER_RECEIVED:
		  if (session->clientbanner == NULL) {
		    goto error;
		  }
		  set_status(session, 0.4f);
		  ssh_log(session, SSH_LOG_RARE,
		      "SSH client banner: %s", session->clientbanner);

		  /* Here we analyze the different protocols the server allows. */
		  if (ssh_analyze_banner(session, 1, &ssh1, &ssh2) < 0) {
		    goto error;
		  }
		  /* Here we decide which version of the protocol to use. */
		  if (ssh2 && session->ssh2) {
		    session->version = 2;
		  } else if(ssh1 && session->ssh1) {
		    session->version = 1;
		  } else if(ssh1 && !session->ssh1){
#ifdef WITH_SSH1
		    ssh_set_error(session, SSH_FATAL,
		        "SSH-1 protocol not available (configure session to allow SSH-1)");
		    goto error;
#else
		    ssh_set_error(session, SSH_FATAL,
		        "SSH-1 protocol not available (libssh compiled without SSH-1 support)");
		    goto error;
#endif
		  } else {
		    ssh_set_error(session, SSH_FATAL,
		        "No version of SSH protocol usable (banner: %s)",
		        session->clientbanner);
		    goto error;
		  }
		  /* from now, the packet layer is handling incoming packets */
		  if(session->version==2)
		    session->socket_callbacks.data=ssh_packet_socket_callback;
#ifdef WITH_SSH1
		  else
		    session->socket_callbacks.data=ssh_packet_socket_callback1;
#endif
		  ssh_packet_set_default_callbacks(session);
		  set_status(session, 0.5f);
		  session->session_state=SSH_SESSION_STATE_INITIAL_KEX;
          if (ssh_send_kex(session, 1) < 0) {
			goto error;
		  }
		  break;
		case SSH_SESSION_STATE_INITIAL_KEX:
		/* TODO: This state should disappear in favor of get_key handle */
#ifdef WITH_SSH1
			if(session->version==1){
				if (ssh_get_kex1(session) < 0)
					goto error;
				set_status(session,0.6f);
				session->connected = 1;
				break;
			}
#endif
			break;
		case SSH_SESSION_STATE_KEXINIT_RECEIVED:
			set_status(session,0.6f);
			ssh_list_kex(session, &session->client_kex); // log client kex
            crypt_set_algorithms_server(session);
			if (set_kex(session) < 0) {
				goto error;
			}
			set_status(session,0.8f);
			session->session_state=SSH_SESSION_STATE_DH;
            break;
		case SSH_SESSION_STATE_DH:
			if(session->dh_handshake_state==DH_STATE_FINISHED){
                if (generate_session_keys(session) < 0) {
                  goto error;
                }

                /*
                 * Once we got SSH2_MSG_NEWKEYS we can switch next_crypto and
                 * current_crypto
                 */
                if (session->current_crypto) {
                  crypto_free(session->current_crypto);
                }

                /* FIXME TODO later, include a function to change keys */
                session->current_crypto = session->next_crypto;
                session->next_crypto = crypto_new();
                if (session->next_crypto == NULL) {
                  goto error;
                }
				set_status(session,1.0f);
				session->connected = 1;
				session->session_state=SSH_SESSION_STATE_AUTHENTICATING;
            }
			break;
		case SSH_SESSION_STATE_AUTHENTICATING:
			break;
		case SSH_SESSION_STATE_ERROR:
			goto error;
		default:
			ssh_set_error(session,SSH_FATAL,"Invalid state %d",session->session_state);
	}
	leave_function();
	return;
	error:
	ssh_socket_close(session->socket);
	session->alive = 0;
	session->session_state=SSH_SESSION_STATE_ERROR;
	leave_function();
}

/**
 * @internal
 *
 * @brief Gets the banner from socket and saves it in session.
 * Updates the session state
 *
 * @param  data pointer to the beginning of header
 * @param  len size of the banner
 * @param  user is a pointer to session
 * @returns Number of bytes processed, or zero if the banner is not complete.
 */
static int callback_receive_banner(const void *data, size_t len, void *user) {
    char *buffer = (char *) data;
    ssh_session session = (ssh_session) user;
    char *str = NULL;
    size_t i;
    int ret=0;

    enter_function();

    for (i = 0; i < len; i++) {
#ifdef WITH_PCAP
        if(session->pcap_ctx && buffer[i] == '\n') {
            ssh_pcap_context_write(session->pcap_ctx,
                                   SSH_PCAP_DIR_IN,
                                   buffer,
                                   i + 1,
                                   i + 1);
        }
#endif
        if (buffer[i] == '\r') {
            buffer[i]='\0';
        }

        if (buffer[i] == '\n') {
            buffer[i]='\0';

            str = strdup(buffer);
            /* number of bytes read */
            ret = i + 1;
            session->clientbanner = str;
            session->session_state = SSH_SESSION_STATE_BANNER_RECEIVED;
            ssh_log(session, SSH_LOG_PACKET, "Received banner: %s", str);
            session->ssh_connection_callback(session);

            leave_function();
            return ret;
        }

        if(i > 127) {
            /* Too big banner */
            session->session_state = SSH_SESSION_STATE_ERROR;
            ssh_set_error(session, SSH_FATAL, "Receiving banner: too large banner");

            leave_function();
            return 0;
        }
    }

    leave_function();
    return ret;
}

/* Do the banner and key exchange */
int ssh_handle_key_exchange(ssh_session session) {
    int rc;

    rc = ssh_send_banner(session, 1);
    if (rc < 0) {
        return SSH_ERROR;
    }

    session->alive = 1;

    session->ssh_connection_callback = ssh_server_connection_callback;
    session->session_state = SSH_SESSION_STATE_SOCKET_CONNECTED;
    ssh_socket_set_callbacks(session->socket,&session->socket_callbacks);
    session->socket_callbacks.data=callback_receive_banner;
    session->socket_callbacks.exception=ssh_socket_exception_callback;
    session->socket_callbacks.userdata=session;

    rc = server_set_kex(session);
    if (rc < 0) {
        return SSH_ERROR;
    }

    while (session->session_state != SSH_SESSION_STATE_ERROR &&
           session->session_state != SSH_SESSION_STATE_AUTHENTICATING &&
           session->session_state != SSH_SESSION_STATE_DISCONNECTED) {
        /*
         * loop until SSH_SESSION_STATE_BANNER_RECEIVED or
         * SSH_SESSION_STATE_ERROR
         */
        ssh_handle_packets(session, -2);
        ssh_log(session,SSH_LOG_PACKET, "ssh_handle_key_exchange: Actual state : %d",
                session->session_state);
    }

    if (session->session_state == SSH_SESSION_STATE_ERROR ||
        session->session_state == SSH_SESSION_STATE_DISCONNECTED) {
        return SSH_ERROR;
    }

  return SSH_OK;
}

/* messages */

static int ssh_message_auth_reply_default(ssh_message msg,int partial) {
  ssh_session session = msg->session;
  char methods_c[128] = {0};
  ssh_string methods = NULL;
  int rc = SSH_ERROR;

  enter_function();

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_USERAUTH_FAILURE) < 0) {
    return rc;
  }

  if (session->auth_methods == 0) {
    session->auth_methods = SSH_AUTH_METHOD_PUBLICKEY | SSH_AUTH_METHOD_PASSWORD;
  }
  if (session->auth_methods & SSH_AUTH_METHOD_PUBLICKEY) {
    strcat(methods_c, "publickey,");
  }
  if (session->auth_methods & SSH_AUTH_METHOD_INTERACTIVE) {
    strcat(methods_c, "keyboard-interactive,");
  }
  if (session->auth_methods & SSH_AUTH_METHOD_PASSWORD) {
    strcat(methods_c, "password,");
  }
  if (session->auth_methods & SSH_AUTH_METHOD_HOSTBASED) {
    strcat(methods_c, "hostbased,");
  }

  /* Strip the comma. */
  methods_c[strlen(methods_c) - 1] = '\0'; // strip the comma. We are sure there is at

  ssh_log(session, SSH_LOG_PACKET,
      "Sending a auth failure. methods that can continue: %s", methods_c);

  methods = ssh_string_from_char(methods_c);
  if (methods == NULL) {
    goto error;
  }

  if (buffer_add_ssh_string(msg->session->out_buffer, methods) < 0) {
    goto error;
  }

  if (partial) {
    if (buffer_add_u8(session->out_buffer, 1) < 0) {
      goto error;
    }
  } else {
    if (buffer_add_u8(session->out_buffer, 0) < 0) {
      goto error;
    }
  }

  rc = packet_send(msg->session);
error:
  ssh_string_free(methods);

  leave_function();
  return rc;
}

static int ssh_message_channel_request_open_reply_default(ssh_message msg) {
  ssh_log(msg->session, SSH_LOG_FUNCTIONS, "Refusing a channel");

  if (buffer_add_u8(msg->session->out_buffer
        , SSH2_MSG_CHANNEL_OPEN_FAILURE) < 0) {
    goto error;
  }
  if (buffer_add_u32(msg->session->out_buffer,
        htonl(msg->channel_request_open.sender)) < 0) {
    goto error;
  }
  if (buffer_add_u32(msg->session->out_buffer,
        htonl(SSH2_OPEN_ADMINISTRATIVELY_PROHIBITED)) < 0) {
    goto error;
  }
  /* reason is an empty string */
  if (buffer_add_u32(msg->session->out_buffer, 0) < 0) {
    goto error;
  }
  /* language too */
  if (buffer_add_u32(msg->session->out_buffer, 0) < 0) {
    goto error;
  }

  return packet_send(msg->session);
error:
  return SSH_ERROR;
}

static int ssh_message_channel_request_reply_default(ssh_message msg) {
  uint32_t channel;

  if (msg->channel_request.want_reply) {
    channel = msg->channel_request.channel->remote_channel;

    ssh_log(msg->session, SSH_LOG_PACKET,
        "Sending a default channel_request denied to channel %d", channel);

    if (buffer_add_u8(msg->session->out_buffer, SSH2_MSG_CHANNEL_FAILURE) < 0) {
      return SSH_ERROR;
    }
    if (buffer_add_u32(msg->session->out_buffer, htonl(channel)) < 0) {
      return SSH_ERROR;
    }

    return packet_send(msg->session);
  }

  ssh_log(msg->session, SSH_LOG_PACKET,
      "The client doesn't want to know the request failed!");

  return SSH_OK;
}

static int ssh_message_service_request_reply_default(ssh_message msg) {
  /* The only return code accepted by specifications are success or disconnect */
  return ssh_message_service_reply_success(msg);
}

int ssh_message_service_reply_success(ssh_message msg) {
  struct ssh_string_struct *service;
  ssh_session session;

  if (msg == NULL) {
    return SSH_ERROR;
  }
  session = msg->session;

  ssh_log(session, SSH_LOG_PACKET,
      "Sending a SERVICE_ACCEPT for service %s", msg->service_request.service);
  if (buffer_add_u8(session->out_buffer, SSH2_MSG_SERVICE_ACCEPT) < 0) {
    return -1;
  }
  service=ssh_string_from_char(msg->service_request.service);
  if (buffer_add_ssh_string(session->out_buffer, service) < 0) {
    ssh_string_free(service);
    return -1;
  }
  ssh_string_free(service);
  return packet_send(msg->session);
}

int ssh_message_global_request_reply_success(ssh_message msg, uint16_t bound_port) {
    ssh_log(msg->session, SSH_LOG_FUNCTIONS, "Accepting a global request");

    if (msg->global_request.want_reply) {
        if (buffer_add_u8(msg->session->out_buffer
                    , SSH2_MSG_REQUEST_SUCCESS) < 0) {
            goto error;
        }

        if(msg->global_request.type == SSH_GLOBAL_REQUEST_TCPIP_FORWARD 
                                && msg->global_request.bind_port == 0) {
            if (buffer_add_u32(msg->session->out_buffer, htonl(bound_port)) < 0) {
                goto error;
            }
        }

        return packet_send(msg->session);
    }

    if(msg->global_request.type == SSH_GLOBAL_REQUEST_TCPIP_FORWARD 
                                && msg->global_request.bind_port == 0) {
        ssh_log(msg->session, SSH_LOG_PACKET,
                "The client doesn't want to know the remote port!");
    }

    return SSH_OK;
error:
    return SSH_ERROR;
}

static int ssh_message_global_request_reply_default(ssh_message msg) {
    ssh_log(msg->session, SSH_LOG_FUNCTIONS, "Refusing a global request");

    if (msg->global_request.want_reply) {
        if (buffer_add_u8(msg->session->out_buffer
                    , SSH2_MSG_REQUEST_FAILURE) < 0) {
            goto error;
        }
        return packet_send(msg->session);
    }
    ssh_log(msg->session, SSH_LOG_PACKET,
            "The client doesn't want to know the request failed!");

    return SSH_OK;
error:
    return SSH_ERROR;
}

int ssh_message_reply_default(ssh_message msg) {
  if (msg == NULL) {
    return -1;
  }

  switch(msg->type) {
    case SSH_REQUEST_AUTH:
      return ssh_message_auth_reply_default(msg, 0);
    case SSH_REQUEST_CHANNEL_OPEN:
      return ssh_message_channel_request_open_reply_default(msg);
    case SSH_REQUEST_CHANNEL:
      return ssh_message_channel_request_reply_default(msg);
    case SSH_REQUEST_SERVICE:
      return ssh_message_service_request_reply_default(msg);
    case SSH_REQUEST_GLOBAL:
      return ssh_message_global_request_reply_default(msg);
    default:
      ssh_log(msg->session, SSH_LOG_PACKET,
          "Don't know what to default reply to %d type",
          msg->type);
      break;
  }

  return -1;
}

char *ssh_message_service_service(ssh_message msg){
  if (msg == NULL) {
    return NULL;
  }
  return msg->service_request.service;
}

char *ssh_message_auth_user(ssh_message msg) {
  if (msg == NULL) {
    return NULL;
  }

  return msg->auth_request.username;
}

char *ssh_message_auth_password(ssh_message msg){
  if (msg == NULL) {
    return NULL;
  }

  return msg->auth_request.password;
}

/* Get the publickey of an auth request */
ssh_public_key ssh_message_auth_publickey(ssh_message msg){
  if (msg == NULL) {
    return NULL;
  }

  return msg->auth_request.public_key;
}

enum ssh_publickey_state_e ssh_message_auth_publickey_state(ssh_message msg){
	if (msg == NULL) {
	    return -1;
	  }
	  return msg->auth_request.signature_state;
}

int ssh_message_auth_set_methods(ssh_message msg, int methods) {
  if (msg == NULL || msg->session == NULL) {
    return -1;
  }

  msg->session->auth_methods = methods;

  return 0;
}

int ssh_message_auth_reply_success(ssh_message msg, int partial) {
  int r;

	if (msg == NULL) {
    return SSH_ERROR;
  }

  if (partial) {
    return ssh_message_auth_reply_default(msg, partial);
  }

  if (buffer_add_u8(msg->session->out_buffer,SSH2_MSG_USERAUTH_SUCCESS) < 0) {
    return SSH_ERROR;
  }

  r = packet_send(msg->session);
  if(msg->session->current_crypto && msg->session->current_crypto->delayed_compress_out){
  	ssh_log(msg->session,SSH_LOG_PROTOCOL,"Enabling delayed compression OUT");
  	msg->session->current_crypto->do_compress_out=1;
  }
  if(msg->session->current_crypto && msg->session->current_crypto->delayed_compress_in){
  	ssh_log(msg->session,SSH_LOG_PROTOCOL,"Enabling delayed compression IN");
  	msg->session->current_crypto->do_compress_in=1;
  }
  return r;
}

/* Answer OK to a pubkey auth request */
int ssh_message_auth_reply_pk_ok(ssh_message msg, ssh_string algo, ssh_string pubkey) {
  if (msg == NULL) {
    return SSH_ERROR;
  }

  if (buffer_add_u8(msg->session->out_buffer, SSH2_MSG_USERAUTH_PK_OK) < 0 ||
      buffer_add_ssh_string(msg->session->out_buffer, algo) < 0 ||
      buffer_add_ssh_string(msg->session->out_buffer, pubkey) < 0) {
    return SSH_ERROR;
  }

  return packet_send(msg->session);
}

int ssh_message_auth_reply_pk_ok_simple(ssh_message msg) {
	ssh_string algo;
	ssh_string pubkey;
	int ret;
	algo=ssh_string_from_char(msg->auth_request.public_key->type_c);
	pubkey=publickey_to_string(msg->auth_request.public_key);
	ret=ssh_message_auth_reply_pk_ok(msg,algo,pubkey);
	ssh_string_free(algo);
	ssh_string_free(pubkey);
	return ret;
}


char *ssh_message_channel_request_open_originator(ssh_message msg){
    return msg->channel_request_open.originator;
}

int ssh_message_channel_request_open_originator_port(ssh_message msg){
    return msg->channel_request_open.originator_port;
}

char *ssh_message_channel_request_open_destination(ssh_message msg){
    return msg->channel_request_open.destination;
}

int ssh_message_channel_request_open_destination_port(ssh_message msg){
    return msg->channel_request_open.destination_port;
}

ssh_channel ssh_message_channel_request_channel(ssh_message msg){
    return msg->channel_request.channel;
}

char *ssh_message_channel_request_pty_term(ssh_message msg){
    return msg->channel_request.TERM;
}

int ssh_message_channel_request_pty_width(ssh_message msg){
    return msg->channel_request.width;
}

int ssh_message_channel_request_pty_height(ssh_message msg){
    return msg->channel_request.height;
}

int ssh_message_channel_request_pty_pxwidth(ssh_message msg){
    return msg->channel_request.pxwidth;
}

int ssh_message_channel_request_pty_pxheight(ssh_message msg){
    return msg->channel_request.pxheight;
}

char *ssh_message_channel_request_env_name(ssh_message msg){
    return msg->channel_request.var_name;
}

char *ssh_message_channel_request_env_value(ssh_message msg){
    return msg->channel_request.var_value;
}

char *ssh_message_channel_request_command(ssh_message msg){
    return msg->channel_request.command;
}

char *ssh_message_channel_request_subsystem(ssh_message msg){
    return msg->channel_request.subsystem;
}

char *ssh_message_global_request_address(ssh_message msg){
    return msg->global_request.bind_address;
}

int ssh_message_global_request_port(ssh_message msg){
    return msg->global_request.bind_port;
}

/** @brief defines the ssh_message callback
 * @param session the current ssh session
 * @param[in] ssh_bind_message_callback a function pointer to a callback taking the
 * current ssh session and received message as parameters. the function returns
 * 0 if the message has been parsed and treated successfully, 1 otherwise (libssh
 * must take care of the response).
 * @param[in] data void pointer to be passed to callback functions
 */
void ssh_set_message_callback(ssh_session session,
        int(*ssh_bind_message_callback)(ssh_session session, ssh_message msg, void *data),
        void *data) {
  session->ssh_message_callback = ssh_bind_message_callback;
  session->ssh_message_callback_data = data;
}

int ssh_execute_message_callbacks(ssh_session session){
  ssh_message msg=NULL;
  int ret;
  ssh_handle_packets(session, 0);
  if(!session->ssh_message_list)
    return SSH_OK;
  if(session->ssh_message_callback){
    while((msg=ssh_message_pop_head(session)) != NULL) {
      ret=session->ssh_message_callback(session,msg,
                                        session->ssh_message_callback_data);
      if(ret==1){
        ret = ssh_message_reply_default(msg);
        ssh_message_free(msg);
        if(ret != SSH_OK)
          return ret;
      } else {
        ssh_message_free(msg);
      }
    }
  } else {
    while((msg=ssh_message_pop_head(session)) != NULL) {
      ret = ssh_message_reply_default(msg);
      ssh_message_free(msg);
      if(ret != SSH_OK)
        return ret;
    }
  }
  return SSH_OK;
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
