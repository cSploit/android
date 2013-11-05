/*
 * session.c - non-networking functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2005-2008 by Aris Adamantiadis
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
#include <string.h>
#include <stdlib.h>
#include "libssh/libssh.h"
#include "libssh/priv.h"
#include "libssh/server.h"
#include "libssh/socket.h"
#include "libssh/ssh2.h"
#include "libssh/agent.h"
#include "libssh/packet.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/buffer.h"
#include "libssh/poll.h"

#define FIRST_CHANNEL 42 // why not ? it helps to find bugs.

/**
 * @defgroup libssh_session The SSH session functions.
 * @ingroup libssh
 *
 * Functions that manage a session.
 *
 * @{
 */

/**
 * @brief Create a new ssh session.
 *
 * @returns             A new ssh_session pointer, NULL on error.
 */
ssh_session ssh_new(void) {
  ssh_session session;
  char *id;
  int rc;

  session = malloc(sizeof (struct ssh_session_struct));
  if (session == NULL) {
    return NULL;
  }
  ZERO_STRUCTP(session);

  session->next_crypto = crypto_new();
  if (session->next_crypto == NULL) {
    goto err;
  }

  session->socket = ssh_socket_new(session);
  if (session->socket == NULL) {
    goto err;
  }

  session->out_buffer = ssh_buffer_new();
  if (session->out_buffer == NULL) {
    goto err;
  }

  session->in_buffer=ssh_buffer_new();
  if (session->in_buffer == NULL) {
    goto err;
  }

  session->alive = 0;
  session->auth_methods = 0;
  ssh_set_blocking(session, 1);
  session->common.log_indent = 0;
  session->maxchannel = FIRST_CHANNEL;

  /* options */
  session->StrictHostKeyChecking = 1;
  session->port = 22;
  session->fd = -1;
  session->ssh2 = 1;
  session->compressionlevel=7;
#ifdef WITH_SSH1
  session->ssh1 = 1;
#else
  session->ssh1 = 0;
#endif

#ifndef _WIN32
    session->agent = agent_new(session);
    if (session->agent == NULL) {
      goto err;
    }
#endif /* _WIN32 */

    session->identity = ssh_list_new();
    if (session->identity == NULL) {
      goto err;
    }

    id = strdup("%d/id_rsa");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }

    id = strdup("%d/id_dsa");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }

    id = strdup("%d/identity");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }

    return session;

err:
    free(id);
    ssh_free(session);
    return NULL;
}

/**
 * @brief Deallocate a SSH session handle.
 *
 * @param[in] session   The SSH session to free.
 *
 * @see ssh_disconnect()
 * @see ssh_new()
 */
void ssh_free(ssh_session session) {
  int i;
  struct ssh_iterator *it;

  if (session == NULL) {
    return;
  }

  /* delete all channels */
  while ((it=ssh_list_get_iterator(session->channels)) != NULL) {
    ssh_channel_free(ssh_iterator_value(ssh_channel,it));
    ssh_list_remove(session->channels, it);
  }
  ssh_list_free(session->channels);
  session->channels=NULL;

  ssh_socket_free(session->socket);

  if (session->default_poll_ctx) {
      ssh_poll_ctx_free(session->default_poll_ctx);
  }

#ifdef WITH_PCAP
  if(session->pcap_ctx){
  	ssh_pcap_context_free(session->pcap_ctx);
  	session->pcap_ctx=NULL;
  }
#endif
  ssh_buffer_free(session->in_buffer);
  ssh_buffer_free(session->out_buffer);
  if(session->in_hashbuf != NULL)
    ssh_buffer_free(session->in_hashbuf);
  if(session->out_hashbuf != NULL)
    ssh_buffer_free(session->out_hashbuf);
  session->in_buffer=session->out_buffer=NULL;
  crypto_free(session->current_crypto);
  crypto_free(session->next_crypto);
#ifndef _WIN32
  agent_free(session->agent);
#endif /* _WIN32 */
  if (session->client_kex.methods) {
    for (i = 0; i < 10; i++) {
      SAFE_FREE(session->client_kex.methods[i]);
    }
  }

  if (session->server_kex.methods) {
    for (i = 0; i < 10; i++) {
      SAFE_FREE(session->server_kex.methods[i]);
    }
  }
  SAFE_FREE(session->client_kex.methods);
  SAFE_FREE(session->server_kex.methods);

  privatekey_free(session->dsa_key);
  privatekey_free(session->rsa_key);
  if(session->ssh_message_list){
    ssh_message msg;
    while((msg=ssh_list_pop_head(ssh_message ,session->ssh_message_list))
        != NULL){
      ssh_message_free(msg);
    }
    ssh_list_free(session->ssh_message_list);
  }

  if (session->packet_callbacks)
    ssh_list_free(session->packet_callbacks);

  if (session->identity) {
    char *id;

    for (id = ssh_list_pop_head(char *, session->identity);
         id != NULL;
         id = ssh_list_pop_head(char *, session->identity)) {
      SAFE_FREE(id);
    }
    ssh_list_free(session->identity);
  }

  SAFE_FREE(session->serverbanner);
  SAFE_FREE(session->clientbanner);
  SAFE_FREE(session->bindaddr);
  SAFE_FREE(session->banner);

  /* options */
  SAFE_FREE(session->username);
  SAFE_FREE(session->host);
  SAFE_FREE(session->sshdir);
  SAFE_FREE(session->knownhosts);
  SAFE_FREE(session->ProxyCommand);

  for (i = 0; i < 10; i++) {
    if (session->wanted_methods[i]) {
      SAFE_FREE(session->wanted_methods[i]);
    }
  }

  /* burn connection, it could hang sensitive datas */
  ZERO_STRUCTP(session);
  SAFE_FREE(session);
}

/**
 * @brief Disconnect impolitely from a remote host by closing the socket.
 *
 * Suitable if you forked and want to destroy this session.
 *
 * @param[in]  session  The SSH session to disconnect.
 */
void ssh_silent_disconnect(ssh_session session) {
  enter_function();

  if (session == NULL) {
    return;
  }

  ssh_socket_close(session->socket);
  session->alive = 0;
  ssh_disconnect(session);
  leave_function();
}

/**
 * @brief Set the session in blocking/nonblocking mode.
 *
 * @param[in]  session  The ssh session to change.
 *
 * @param[in]  blocking Zero for nonblocking mode.
 *
 * \bug nonblocking code is in development and won't work as expected
 */
void ssh_set_blocking(ssh_session session, int blocking) {
	if (session == NULL) {
    return;
  }
  session->flags &= ~SSH_SESSION_FLAG_BLOCKING;
  session->flags |= blocking ? SSH_SESSION_FLAG_BLOCKING : 0;
}

/**
 * @brief Return the blocking mode of libssh
 * @param[in] session The SSH session
 * @returns 0 if the session is nonblocking,
 * @returns 1 if the functions may block.
 */
int ssh_is_blocking(ssh_session session){
	return (session->flags&SSH_SESSION_FLAG_BLOCKING) ? 1 : 0;
}

/**
 * @brief Blocking flush of the outgoing buffer
 * @param[in] session The SSH session
 * @param[in] timeout Set an upper limit on the time for which this function
 *                    will block, in milliseconds. Specifying -1
 *                    means an infinite timeout. This parameter is passed to
 *                    the poll() function.
 * @returns           SSH_OK on success, SSH_AGAIN if timeout occurred,
 *                    SSH_ERROR otherwise.
 */

int ssh_blocking_flush(ssh_session session, int timeout){
	ssh_socket s;
	int rc = SSH_OK;
	if(session==NULL)
		return SSH_ERROR;

	s=session->socket;
	while (ssh_socket_buffered_write_bytes(s) > 0 && session->alive) {
		rc = ssh_handle_packets(session, timeout);
		if(rc == SSH_AGAIN || rc == SSH_ERROR) break;
	}

	return rc;
}

/**
 * @brief Check if we are connected.
 *
 * @param[in]  session  The session to check if it is connected.
 *
 * @return              1 if we are connected, 0 if not.
 */
int ssh_is_connected(ssh_session session) {
    if (session == NULL) {
        return 0;
    }

    return session->alive;
}

/**
 * @brief Get the fd of a connection.
 *
 * In case you'd need the file descriptor of the connection to the server/client.
 *
 * @param[in] session   The ssh session to use.
 *
 * @return              The file descriptor of the connection, or -1 if it is
 *                      not connected
 */
socket_t ssh_get_fd(ssh_session session) {
  if (session == NULL) {
    return -1;
  }

  return ssh_socket_get_fd_in(session->socket);
}

/**
 * @brief Tell the session it has data to read on the file descriptor without
 * blocking.
 *
 * @param[in] session   The ssh session to use.
 */
void ssh_set_fd_toread(ssh_session session) {
  if (session == NULL) {
    return;
  }

  ssh_socket_set_read_wontblock(session->socket);
}

/**
 * @brief Tell the session it may write to the file descriptor without blocking.
 *
 * @param[in] session   The ssh session to use.
 */
void ssh_set_fd_towrite(ssh_session session) {
  if (session == NULL) {
    return;
  }

  ssh_socket_set_write_wontblock(session->socket);
}

/**
 * @brief Tell the session it has an exception to catch on the file descriptor.
 *
 * \param[in] session   The ssh session to use.
 */
void ssh_set_fd_except(ssh_session session) {
  if (session == NULL) {
    return;
  }

  ssh_socket_set_except(session->socket);
}

/**
 * @internal
 *
 * @brief Poll the current session for an event and call the appropriate
 * callbacks.
 *
 * This will block until one event happens.
 *
 * @param[in] session   The session handle to use.
 *
 * @param[in] timeout   Set an upper limit on the time for which this function
 *                      will block, in milliseconds. Specifying -1
 *                      means an infinite timeout.
 *                      Specifying -2 means to use the timeout specified in
 *                      options. 0 means poll will return immediately. This
 *                      parameter is passed to the poll() function.
 *
 * @return              SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_handle_packets(ssh_session session, int timeout) {
    ssh_poll_handle spoll_in,spoll_out;
    ssh_poll_ctx ctx;
    int tm = timeout;
    int rc;

    if (session == NULL || session->socket == NULL) {
        return SSH_ERROR;
    }
    enter_function();

    spoll_in = ssh_socket_get_poll_handle_in(session->socket);
    spoll_out = ssh_socket_get_poll_handle_out(session->socket);
    if (session->server) {
        ssh_poll_add_events(spoll_in, POLLIN);
    }
    ctx = ssh_poll_get_ctx(spoll_in);

    if (!ctx) {
        ctx = ssh_poll_get_default_ctx(session);
        ssh_poll_ctx_add(ctx, spoll_in);
        if (spoll_in != spoll_out) {
            ssh_poll_ctx_add(ctx, spoll_out);
        }
    }

    if (timeout == -2) {
        tm = ssh_make_milliseconds(session->timeout, session->timeout_usec);
    }
    rc = ssh_poll_ctx_dopoll(ctx, tm);
    if (rc == SSH_ERROR) {
        session->session_state = SSH_SESSION_STATE_ERROR;
    }

    leave_function();
    return rc;
}

/**
 * @internal
 *
 * @brief Poll the current session for an event and call the appropriate
 * callbacks.
 *
 * This will block until termination fuction returns true, or timeout expired.
 *
 * @param[in] session   The session handle to use.
 *
 * @param[in] timeout   Set an upper limit on the time for which this function
 *                      will block, in milliseconds. Specifying a negative value
 *                      means an infinite timeout. This parameter is passed to
 *                      the poll() function.
 * @param[in] fct       Termination function to be used to determine if it is
 *                      possible to stop polling.
 * @param[in] user      User parameter to be passed to fct termination function.
 * @return              SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_handle_packets_termination(ssh_session session, int timeout,
	ssh_termination_function fct, void *user){
	int ret = SSH_OK;
	struct ssh_timestamp ts;
	ssh_timestamp_init(&ts);
	while(!fct(user)){
		ret = ssh_handle_packets(session, timeout);
		if(ret == SSH_ERROR || ret == SSH_AGAIN)
			return ret;
		if(fct(user)) 
			return SSH_OK;
		else if(ssh_timeout_elapsed(&ts, timeout == -2 ? ssh_make_milliseconds(session->timeout, session->timeout_usec) : timeout))
			/* it is possible that we get unrelated packets but still timeout our request,
			 * so simply relying on the poll timeout is not enough */
			return SSH_AGAIN;
	}
	return ret;
}

/**
 * @brief Get session status
 *
 * @param session       The ssh session to use.
 *
 * @returns A bitmask including SSH_CLOSED, SSH_READ_PENDING or SSH_CLOSED_ERROR
 *          which respectively means the session is closed, has data to read on
 *          the connection socket and session was closed due to an error.
 */
int ssh_get_status(ssh_session session) {
  int socketstate;
  int r = 0;

  if (session == NULL) {
    return 0;
  }

  socketstate = ssh_socket_get_status(session->socket);

  if (session->closed) {
    r |= SSH_CLOSED;
  }
  if (socketstate & SSH_READ_PENDING) {
    r |= SSH_READ_PENDING;
  }
  if (session->closed && (socketstate & SSH_CLOSED_ERROR)) {
    r |= SSH_CLOSED_ERROR;
  }

  return r;
}

/**
 * @brief Get the disconnect message from the server.
 *
 * @param[in] session   The ssh session to use.
 *
 * @return              The message sent by the server along with the
 *                      disconnect, or NULL in which case the reason of the
 *                      disconnect may be found with ssh_get_error.
 *
 * @see ssh_get_error()
 */
const char *ssh_get_disconnect_message(ssh_session session) {
  if (session == NULL) {
    return NULL;
  }

  if (!session->closed) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Connection not closed yet");
  } else if(session->closed_by_except) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Connection closed by socket error");
  } else if(!session->discon_msg) {
    ssh_set_error(session, SSH_FATAL,
        "Connection correctly closed but no disconnect message");
  } else {
    return session->discon_msg;
  }

  return NULL;
}

/**
 * @brief Get the protocol version of the session.
 *
 * @param session       The ssh session to use.
 *
 * @return 1 or 2, for ssh1 or ssh2, < 0 on error.
 */
int ssh_get_version(ssh_session session) {
  if (session == NULL) {
    return -1;
  }

  return session->version;
}

/**
 * @internal
 *
 * @brief Handle a SSH_DISCONNECT packet.
 */
SSH_PACKET_CALLBACK(ssh_packet_disconnect_callback){
	uint32_t code;
	char *error=NULL;
	ssh_string error_s;
	(void)user;
	(void)type;
  buffer_get_u32(packet, &code);
  error_s = buffer_get_ssh_string(packet);
  if (error_s != NULL) {
    error = ssh_string_to_char(error_s);
    ssh_string_free(error_s);
  }
  ssh_log(session, SSH_LOG_PACKET, "Received SSH_MSG_DISCONNECT %d:%s",code,
      error != NULL ? error : "no error");
  ssh_set_error(session, SSH_FATAL,
      "Received SSH_MSG_DISCONNECT: %d:%s",code,
      error != NULL ? error : "no error");
  SAFE_FREE(error);

  ssh_socket_close(session->socket);
  session->alive = 0;
  session->session_state= SSH_SESSION_STATE_ERROR;
	/* TODO: handle a graceful disconnect */
	return SSH_PACKET_USED;
}

/**
 * @internal
 *
 * @brief Handle a SSH_IGNORE and SSH_DEBUG packet.
 */
SSH_PACKET_CALLBACK(ssh_packet_ignore_callback){
	(void)user;
	(void)type;
	(void)packet;
	ssh_log(session,SSH_LOG_PROTOCOL,"Received %s packet",type==SSH2_MSG_IGNORE ? "SSH_MSG_IGNORE" : "SSH_MSG_DEBUG");
	/* TODO: handle a graceful disconnect */
	return SSH_PACKET_USED;
}

/**
 * @internal
 * @brief Callback to be called when the socket received an exception code.
 * @param user is a pointer to session
 */
void ssh_socket_exception_callback(int code, int errno_code, void *user){
    ssh_session session=(ssh_session)user;
    enter_function();
    ssh_log(session,SSH_LOG_RARE,"Socket exception callback: %d (%d)",code, errno_code);
    session->session_state=SSH_SESSION_STATE_ERROR;
    ssh_set_error(session,SSH_FATAL,"Socket error: %s",strerror(errno_code));
    session->ssh_connection_callback(session);
    leave_function();
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
