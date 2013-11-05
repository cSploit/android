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

#ifndef SESSION_H_
#define SESSION_H_
#include "libssh/priv.h"
#include "libssh/packet.h"
#include "libssh/pcap.h"
#include "libssh/auth.h"
#include "libssh/channels.h"
#include "libssh/poll.h"
typedef struct ssh_kbdint_struct* ssh_kbdint;

/* These are the different states a SSH session can be into its life */
enum ssh_session_state_e {
	SSH_SESSION_STATE_NONE=0,
	SSH_SESSION_STATE_CONNECTING,
	SSH_SESSION_STATE_SOCKET_CONNECTED,
	SSH_SESSION_STATE_BANNER_RECEIVED,
	SSH_SESSION_STATE_INITIAL_KEX,
	SSH_SESSION_STATE_KEXINIT_RECEIVED,
	SSH_SESSION_STATE_DH,
	SSH_SESSION_STATE_AUTHENTICATING,
	SSH_SESSION_STATE_AUTHENTICATED,
	SSH_SESSION_STATE_ERROR,
	SSH_SESSION_STATE_DISCONNECTED
};

enum ssh_dh_state_e {
  DH_STATE_INIT=0,
  DH_STATE_INIT_SENT,
  DH_STATE_NEWKEYS_SENT,
  DH_STATE_FINISHED
};

enum ssh_pending_call_e {
	SSH_PENDING_CALL_NONE = 0,
	SSH_PENDING_CALL_CONNECT,
	SSH_PENDING_CALL_AUTH_NONE,
	SSH_PENDING_CALL_AUTH_PASSWORD
};

/* libssh calls may block an undefined amount of time */
#define SSH_SESSION_FLAG_BLOCKING 1

/* members that are common to ssh_session and ssh_bind */
struct ssh_common_struct {
    struct error_struct error;
    ssh_callbacks callbacks; /* Callbacks to user functions */
    int log_verbosity; /* verbosity of the log functions */
    int log_indent; /* indentation level in enter_function logs */
};

struct ssh_session_struct {
    struct ssh_common_struct common;
    struct ssh_socket_struct *socket;
    char *serverbanner;
    char *clientbanner;
    int protoversion;
    int server;
    int client;
    int openssh;
    uint32_t send_seq;
    uint32_t recv_seq;
/* status flags */
    int closed;
    int closed_by_except;

    int connected;
    /* !=0 when the user got a session handle */
    int alive;
    /* two previous are deprecated */
    /* int auth_service_asked; */

    /* session flags (SSH_SESSION_FLAG_*) */
    int flags;

    ssh_string banner; /* that's the issue banner from
                       the server */
    char *discon_msg; /* disconnect message from
                         the remote host */
    ssh_buffer in_buffer;
    PACKET in_packet;
    ssh_buffer out_buffer;

    /* the states are used by the nonblocking stuff to remember */
    /* where it was before being interrupted */
    enum ssh_pending_call_e pending_call_state;
    enum ssh_session_state_e session_state;
    int packet_state;
    enum ssh_dh_state_e dh_handshake_state;
    enum ssh_auth_service_state_e auth_service_state;
    enum ssh_auth_state_e auth_state;
    enum ssh_channel_request_state_e global_req_state;
    ssh_string dh_server_signature; /* information used by dh_handshake. */
    KEX server_kex;
    KEX client_kex;
    ssh_buffer in_hashbuf;
    ssh_buffer out_hashbuf;
    struct ssh_crypto_struct *current_crypto;
    struct ssh_crypto_struct *next_crypto;  /* next_crypto is going to be used after a SSH2_MSG_NEWKEYS */

    struct ssh_list *channels; /* linked list of channels */
    int maxchannel;
    int exec_channel_opened; /* version 1 only. more
                                info in channels1.c */
    ssh_agent agent; /* ssh agent */

/* keyb interactive data */
    struct ssh_kbdint_struct *kbdint;
    int version; /* 1 or 2 */
    /* server host keys */
    ssh_private_key rsa_key;
    ssh_private_key dsa_key;
    /* auths accepted by server */
    int auth_methods;
    int hostkeys; /* contains type of host key wanted by client, in server impl */
    struct ssh_list *ssh_message_list; /* list of delayed SSH messages */
    int (*ssh_message_callback)( struct ssh_session_struct *session, ssh_message msg, void *userdata);
    void *ssh_message_callback_data;

    void (*ssh_connection_callback)( struct ssh_session_struct *session);
    struct ssh_packet_callbacks_struct default_packet_callbacks;
    struct ssh_list *packet_callbacks;
    struct ssh_socket_callbacks_struct socket_callbacks;
    ssh_poll_ctx default_poll_ctx;
    /* options */
#ifdef WITH_PCAP
    ssh_pcap_context pcap_ctx; /* pcap debugging context */
#endif
    char *username;
    char *host;
    char *bindaddr; /* bind the client to an ip addr */
    char *xbanner; /* TODO: looks like it is not needed */
    struct ssh_list *identity;
    char *sshdir;
    char *knownhosts;
    char *wanted_methods[10];
    char compressionlevel;
    unsigned long timeout; /* seconds */
    unsigned long timeout_usec;
    unsigned int port;
    socket_t fd;
    int ssh2;
    int ssh1;
    int StrictHostKeyChecking;
    char *ProxyCommand;
};

/** @internal
 * @brief a termination function evaluates the status of an object
 * @param user[in] object to evaluate
 * @returns 1 if the polling routine should terminate, 0 instead
 */
typedef int (*ssh_termination_function)(void *user);
int ssh_handle_packets(ssh_session session, int timeout);
int ssh_handle_packets_termination(ssh_session session, int timeout,
    ssh_termination_function fct, void *user);
void ssh_socket_exception_callback(int code, int errno_code, void *user);

#endif /* SESSION_H_ */
