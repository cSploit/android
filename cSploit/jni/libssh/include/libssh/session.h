/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#ifndef SESSION_H_
#define SESSION_H_
#include "libssh/priv.h"
#include "libssh/kex.h"
#include "libssh/packet.h"
#include "libssh/pcap.h"
#include "libssh/auth.h"
#include "libssh/channels.h"
#include "libssh/poll.h"

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
	SSH_PENDING_CALL_AUTH_PASSWORD,
	SSH_PENDING_CALL_AUTH_OFFER_PUBKEY,
	SSH_PENDING_CALL_AUTH_PUBKEY,
	SSH_PENDING_CALL_AUTH_AGENT,
	SSH_PENDING_CALL_AUTH_KBDINT_INIT,
	SSH_PENDING_CALL_AUTH_KBDINT_SEND,
	SSH_PENDING_CALL_AUTH_GSSAPI_MIC
};

/* libssh calls may block an undefined amount of time */
#define SSH_SESSION_FLAG_BLOCKING 1

/* Client successfully authenticated */
#define SSH_SESSION_FLAG_AUTHENTICATED 2

/* codes to use with ssh_handle_packets*() */
/* Infinite timeout */
#define SSH_TIMEOUT_INFINITE -1
/* Use the timeout defined by user if any. Mostly used with new connections */
#define SSH_TIMEOUT_USER -2
/* Use the default timeout, depending on ssh_is_blocking() */
#define SSH_TIMEOUT_DEFAULT -3
/* Don't block at all */
#define SSH_TIMEOUT_NONBLOCKING 0

/* members that are common to ssh_session and ssh_bind */
struct ssh_common_struct {
    struct error_struct error;
    ssh_callbacks callbacks; /* Callbacks to user functions */
    int log_verbosity; /* verbosity of the log functions */
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
    struct ssh_agent_state_struct *agent_state;
    struct ssh_auth_auto_state_struct *auth_auto_state;

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
    struct ssh_gssapi_struct *gssapi;
    int version; /* 1 or 2 */
    /* server host keys */
    struct {
        ssh_key rsa_key;
        ssh_key dsa_key;
        ssh_key ecdsa_key;

        /* The type of host key wanted by client */
        enum ssh_keytypes_e hostkey;
    } srv;
    /* auths accepted by server */
    int auth_methods;
    struct ssh_list *ssh_message_list; /* list of delayed SSH messages */
    int (*ssh_message_callback)( struct ssh_session_struct *session, ssh_message msg, void *userdata);
    void *ssh_message_callback_data;
    ssh_server_callbacks server_callbacks;
    void (*ssh_connection_callback)( struct ssh_session_struct *session);
    struct ssh_packet_callbacks_struct default_packet_callbacks;
    struct ssh_list *packet_callbacks;
    struct ssh_socket_callbacks_struct socket_callbacks;
    ssh_poll_ctx default_poll_ctx;
    /* options */
#ifdef WITH_PCAP
    ssh_pcap_context pcap_ctx; /* pcap debugging context */
#endif
    struct {
        struct ssh_list *identity;
        char *username;
        char *host;
        char *bindaddr; /* bind the client to an ip addr */
        char *sshdir;
        char *knownhosts;
        char *wanted_methods[10];
        char *ProxyCommand;
        char *custombanner;
        unsigned long timeout; /* seconds */
        unsigned long timeout_usec;
        unsigned int port;
        socket_t fd;
        int StrictHostKeyChecking;
        int ssh2;
        int ssh1;
        char compressionlevel;
        char *gss_server_identity;
        char *gss_client_identity;
        int gss_delegate_creds;
    } opts;
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
