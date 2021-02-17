/*
 * session.c - non-networking functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2005-2013 by Aris Adamantiadis
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

#include "libssh/priv.h"
#include "libssh/libssh.h"
#include "libssh/crypto.h"
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
  char *id = NULL;
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
  session->maxchannel = FIRST_CHANNEL;

#ifndef _WIN32
    session->agent = agent_new(session);
    if (session->agent == NULL) {
      goto err;
    }
#endif /* _WIN32 */

    /* OPTIONS */
    session->opts.StrictHostKeyChecking = 1;
    session->opts.port = 22;
    session->opts.fd = -1;
    session->opts.ssh2 = 1;
    session->opts.compressionlevel=7;
#ifdef WITH_SSH1
    session->opts.ssh1 = 1;
#else
    session->opts.ssh1 = 0;
#endif

    session->opts.identity = ssh_list_new();
    if (session->opts.identity == NULL) {
      goto err;
    }

#ifdef HAVE_ECC
    id = strdup("%d/id_ecdsa");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->opts.identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }
#endif

    id = strdup("%d/id_rsa");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->opts.identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }

    id = strdup("%d/id_dsa");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->opts.identity, id);
    if (rc == SSH_ERROR) {
      goto err;
    }

    id = strdup("%d/identity");
    if (id == NULL) {
      goto err;
    }
    rc = ssh_list_append(session->opts.identity, id);
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

  /*
   * Delete all channels
   *
   * This needs the first thing we clean up cause if there is still an open
   * channel we call ssh_channel_close() first. So we need a working socket
   * and poll context for it.
   */
  for (it = ssh_list_get_iterator(session->channels);
       it != NULL;
       it = ssh_list_get_iterator(session->channels)) {
      ssh_channel_do_free(ssh_iterator_value(ssh_channel,it));
      ssh_list_remove(session->channels, it);
  }
  ssh_list_free(session->channels);
  session->channels = NULL;

#ifdef WITH_PCAP
  if (session->pcap_ctx) {
      ssh_pcap_context_free(session->pcap_ctx);
      session->pcap_ctx = NULL;
  }
#endif

  ssh_socket_free(session->socket);
  session->socket = NULL;

  if (session->default_poll_ctx) {
      ssh_poll_ctx_free(session->default_poll_ctx);
  }

  ssh_buffer_free(session->in_buffer);
  ssh_buffer_free(session->out_buffer);
  session->in_buffer = session->out_buffer = NULL;

  if (session->in_hashbuf != NULL) {
      ssh_buffer_free(session->in_hashbuf);
  }
  if (session->out_hashbuf != NULL) {
      ssh_buffer_free(session->out_hashbuf);
  }

  crypto_free(session->current_crypto);
  crypto_free(session->next_crypto);

#ifndef _WIN32
  agent_free(session->agent);
#endif /* _WIN32 */

  ssh_key_free(session->srv.dsa_key);
  ssh_key_free(session->srv.rsa_key);

  if (session->ssh_message_list) {
      ssh_message msg;

      for (msg = ssh_list_pop_head(ssh_message, session->ssh_message_list);
           msg != NULL;
           msg = ssh_list_pop_head(ssh_message, session->ssh_message_list)) {
          ssh_message_free(msg);
      }
      ssh_list_free(session->ssh_message_list);
  }

  if (session->packet_callbacks) {
    ssh_list_free(session->packet_callbacks);
  }

  /* options */
  if (session->opts.identity) {
      char *id;

      for (id = ssh_list_pop_head(char *, session->opts.identity);
           id != NULL;
           id = ssh_list_pop_head(char *, session->opts.identity)) {
          SAFE_FREE(id);
      }
      ssh_list_free(session->opts.identity);
  }

  SAFE_FREE(session->auth_auto_state);
  SAFE_FREE(session->serverbanner);
  SAFE_FREE(session->clientbanner);
  SAFE_FREE(session->banner);

  SAFE_FREE(session->opts.bindaddr);
  SAFE_FREE(session->opts.username);
  SAFE_FREE(session->opts.host);
  SAFE_FREE(session->opts.sshdir);
  SAFE_FREE(session->opts.knownhosts);
  SAFE_FREE(session->opts.ProxyCommand);
  SAFE_FREE(session->opts.gss_server_identity);
  SAFE_FREE(session->opts.gss_client_identity);

  for (i = 0; i < 10; i++) {
      if (session->opts.wanted_methods[i]) {
          SAFE_FREE(session->opts.wanted_methods[i]);
      }
  }

  /* burn connection, it could hang sensitive datas */
  BURN_BUFFER(session, sizeof(struct ssh_session_struct));
  SAFE_FREE(session);
}

/**
 * @brief get the client banner
 *
 * @param[in] session   The SSH session
 *
 * @return Returns the client banner string or NULL.
 */
const char* ssh_get_clientbanner(ssh_session session) {
    if (session == NULL) {
        return NULL;
    }

    return session->clientbanner;
}

/**
 * @brief get the server banner
 *
 * @param[in] session   The SSH session
 *
 * @return Returns the server banner string or NULL.
 */
const char* ssh_get_serverbanner(ssh_session session) {
	if(!session) {
		return NULL;
	}
	return session->serverbanner;
}

/**
 * @brief get the name of the input for the given session.
 *
 * @param[in] session The SSH session.
 *
 * @return Returns cipher name or NULL.
 */
const char* ssh_get_cipher_in(ssh_session session) {
    if ((session != NULL) &&
        (session->current_crypto != NULL) &&
        (session->current_crypto->in_cipher != NULL)) {
        return session->current_crypto->in_cipher->name;
    }
    return NULL;
}

/**
 * @brief get the name of the output cipher for the given session.
 *
 * @param[in] session The SSH session.
 *
 * @return Returns cipher name or NULL.
 */
const char* ssh_get_cipher_out(ssh_session session) {
    if ((session != NULL) &&
        (session->current_crypto != NULL) &&
        (session->current_crypto->out_cipher != NULL)) {
        return session->current_crypto->out_cipher->name;
    }
    return NULL;
}

/**
 * @brief Disconnect impolitely from a remote host by closing the socket.
 *
 * Suitable if you forked and want to destroy this session.
 *
 * @param[in]  session  The SSH session to disconnect.
 */
void ssh_silent_disconnect(ssh_session session) {
  if (session == NULL) {
    return;
  }

  ssh_socket_close(session->socket);
  session->alive = 0;
  ssh_disconnect(session);
}

/**
 * @brief Set the session in blocking/nonblocking mode.
 *
 * @param[in]  session  The ssh session to change.
 *
 * @param[in]  blocking Zero for nonblocking mode.
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

/* Waits until the output socket is empty */
static int ssh_flush_termination(void *c){
  ssh_session session = c;
  if (ssh_socket_buffered_write_bytes(session->socket) == 0 ||
      session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
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
    int rc;
    if (session == NULL) {
        return SSH_ERROR;
    }

    rc = ssh_handle_packets_termination(session, timeout,
            ssh_flush_termination, session);
    if (rc == SSH_ERROR) {
        return rc;
    }
    if (!ssh_flush_termination(session)) {
        rc = SSH_AGAIN;
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
 * callbacks. This function will not loop until the timeout is expired.
 *
 * This will block until one event happens.
 *
 * @param[in] session   The session handle to use.
 *
 * @param[in] timeout   Set an upper limit on the time for which this function
 *                      will block, in milliseconds. Specifying SSH_TIMEOUT_INFINITE
 *                      (-1) means an infinite timeout.
 *                      Specifying SSH_TIMEOUT_USER means to use the timeout
 *                      specified in options. 0 means poll will return immediately.
 *                      This parameter is passed to the poll() function.
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

    spoll_in = ssh_socket_get_poll_handle_in(session->socket);
    spoll_out = ssh_socket_get_poll_handle_out(session->socket);
    ssh_poll_add_events(spoll_in, POLLIN);
    ctx = ssh_poll_get_ctx(spoll_in);

    if (!ctx) {
        ctx = ssh_poll_get_default_ctx(session);
        ssh_poll_ctx_add(ctx, spoll_in);
        if (spoll_in != spoll_out) {
            ssh_poll_ctx_add(ctx, spoll_out);
        }
    }

    if (timeout == SSH_TIMEOUT_USER) {
        if (ssh_is_blocking(session))
          tm = ssh_make_milliseconds(session->opts.timeout,
                                     session->opts.timeout_usec);
        else
          tm = 0;
    }
    rc = ssh_poll_ctx_dopoll(ctx, tm);
    if (rc == SSH_ERROR) {
        session->session_state = SSH_SESSION_STATE_ERROR;
    }

    return rc;
}

/**
 * @internal
 *
 * @brief Poll the current session for an event and call the appropriate
 * callbacks.
 *
 * This will block until termination function returns true, or timeout expired.
 *
 * @param[in] session   The session handle to use.
 *
 * @param[in] timeout   Set an upper limit on the time for which this function
 *                      will block, in milliseconds. Specifying SSH_TIMEOUT_INFINITE
 *                      (-1) means an infinite timeout.
 *                      Specifying SSH_TIMEOUT_USER means to use the timeout
 *                      specified in options. 0 means poll will return immediately.
 *                      SSH_TIMEOUT_DEFAULT uses blocking parameters of the session.
 *                      This parameter is passed to the poll() function.
 *
 * @param[in] fct       Termination function to be used to determine if it is
 *                      possible to stop polling.
 * @param[in] user      User parameter to be passed to fct termination function.
 * @return              SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_handle_packets_termination(ssh_session session,
                                   int timeout,
                                   ssh_termination_function fct,
                                   void *user)
{
    struct ssh_timestamp ts;
    int ret = SSH_OK;
    int tm;

    if (timeout == SSH_TIMEOUT_USER) {
        if (ssh_is_blocking(session)) {
            timeout = ssh_make_milliseconds(session->opts.timeout,
                                            session->opts.timeout_usec);
        } else {
            timeout = SSH_TIMEOUT_NONBLOCKING;
        }
    } else if (timeout == SSH_TIMEOUT_DEFAULT) {
        if (ssh_is_blocking(session)) {
            timeout = SSH_TIMEOUT_INFINITE;
        } else {
            timeout = SSH_TIMEOUT_NONBLOCKING;
        }
    }

    /* avoid unnecessary syscall for the SSH_TIMEOUT_NONBLOCKING case */
    if (timeout != SSH_TIMEOUT_NONBLOCKING) {
        ssh_timestamp_init(&ts);
    }

    tm = timeout;
    while(!fct(user)) {
        ret = ssh_handle_packets(session, tm);
        if (ret == SSH_ERROR) {
            break;
        }
        if (ssh_timeout_elapsed(&ts,timeout)) {
            ret = fct(user) ? SSH_OK : SSH_AGAIN;
            break;
        }

        tm = ssh_timeout_update(&ts, timeout);
    }

    return ret;
}

/**
 * @brief Get session status
 *
 * @param session       The ssh session to use.
 *
 * @returns A bitmask including SSH_CLOSED, SSH_READ_PENDING, SSH_WRITE_PENDING
 *          or SSH_CLOSED_ERROR which respectively means the session is closed,
 *          has data to read on the connection socket and session was closed
 *          due to an error.
 */
int ssh_get_status(ssh_session session) {
  int socketstate;
  int r = 0;

  if (session == NULL) {
    return 0;
  }

  socketstate = ssh_socket_get_status(session->socket);

  if (session->session_state == SSH_SESSION_STATE_DISCONNECTED) {
    r |= SSH_CLOSED;
  }
  if (socketstate & SSH_READ_PENDING) {
    r |= SSH_READ_PENDING;
  }
  if (socketstate & SSH_WRITE_PENDING) {
      r |= SSH_WRITE_PENDING;
  }
  if ((session->session_state == SSH_SESSION_STATE_DISCONNECTED &&
       (socketstate & SSH_CLOSED_ERROR)) ||
      session->session_state == SSH_SESSION_STATE_ERROR) {
    r |= SSH_CLOSED_ERROR;
  }

  return r;
}

/**
 * @brief Get poll flags for an external mainloop
 *
 * @param session       The ssh session to use.
 *
 * @returns A bitmask including SSH_READ_PENDING or SSH_WRITE_PENDING.
 *          For SSH_READ_PENDING, your invocation of poll() should include
 *          POLLIN.  For SSH_WRITE_PENDING, your invocation of poll() should
 *          include POLLOUT.
 */
int ssh_get_poll_flags(ssh_session session)
{
  if (session == NULL) {
    return 0;
  }

  return ssh_socket_get_poll_flags (session->socket);
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

  if (session->session_state != SSH_SESSION_STATE_DISCONNECTED) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Connection not closed yet");
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
 * @brief Callback to be called when the socket received an exception code.
 * @param user is a pointer to session
 */
void ssh_socket_exception_callback(int code, int errno_code, void *user){
    ssh_session session=(ssh_session)user;

    SSH_LOG(SSH_LOG_RARE,"Socket exception callback: %d (%d)",code, errno_code);
    session->session_state = SSH_SESSION_STATE_ERROR;
    if (errno_code == 0 && code == SSH_SOCKET_EXCEPTION_EOF) {
        ssh_set_error(session, SSH_FATAL, "Socket error: disconnected");
    } else {
        ssh_set_error(session, SSH_FATAL, "Socket error: %s", strerror(errno_code));
    }

    session->ssh_connection_callback(session);
}

/**
 * @brief Send a message that should be ignored
 *
 * @param[in] session   The SSH session
 * @param[in] data      Data to be sent
 *
 * @return              SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_send_ignore (ssh_session session, const char *data) {
    ssh_string str;

    if (ssh_socket_is_open(session->socket)) {
        if (buffer_add_u8(session->out_buffer, SSH2_MSG_IGNORE) < 0) {
            goto error;
        }

        str = ssh_string_from_char(data);
        if (str == NULL) {
            goto error;
        }

        if (buffer_add_ssh_string(session->out_buffer, str) < 0) {
            ssh_string_free(str);
            goto error;
        }

        packet_send(session);
        ssh_handle_packets(session, 0);

        ssh_string_free(str);
    }

    return SSH_OK;

error:
    buffer_reinit(session->out_buffer);
    return SSH_ERROR;
}

/**
 * @brief Send a debug message
 *
 * @param[in] session          The SSH session
 * @param[in] message          Data to be sent
 * @param[in] always_display   Message SHOULD be displayed by the server. It
 *                             SHOULD NOT be displayed unless debugging
 *                             information has been explicitly requested.
 *
 * @return                     SSH_OK on success, SSH_ERROR otherwise.
 */
int ssh_send_debug (ssh_session session, const char *message, int always_display) {
    ssh_string str;
    int rc;

    if (ssh_socket_is_open(session->socket)) {
        if (buffer_add_u8(session->out_buffer, SSH2_MSG_DEBUG) < 0) {
            goto error;
        }

        if (buffer_add_u8(session->out_buffer, always_display) < 0) {
            goto error;
        }

        str = ssh_string_from_char(message);
        if (str == NULL) {
            goto error;
        }

        rc = buffer_add_ssh_string(session->out_buffer, str);
        ssh_string_free(str);
        if (rc < 0) {
            goto error;
        }

        /* Empty language tag */
        if (buffer_add_u32(session->out_buffer, 0) < 0) {
            goto error;
        }

        packet_send(session);
        ssh_handle_packets(session, 0);
    }

    return SSH_OK;

error:
    buffer_reinit(session->out_buffer);
    return SSH_ERROR;
}

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
