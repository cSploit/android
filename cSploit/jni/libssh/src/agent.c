/*
 * agent.c - ssh agent functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2008-2013 by Andreas Schneider <asn@cryptomilk.org>
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

/* This file is based on authfd.c from OpenSSH */

/*
 * How does the ssh-agent work?
 *
 * a) client sends a request to get a list of all keys
 *    the agent returns the count and all public keys
 * b) iterate over them to check if the server likes one
 * c) the client sends a sign request to the agent
 *    type, pubkey as blob, data to sign, flags
 *    the agent returns the signed data
 */

#ifndef _WIN32

#include "config.h"

#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>

#ifdef HAVE_UNISTD_H
#include <unistd.h>
#endif

#include <netinet/in.h>
#include <arpa/inet.h>

#include "libssh/agent.h"
#include "libssh/priv.h"
#include "libssh/socket.h"
#include "libssh/buffer.h"
#include "libssh/session.h"
#include "libssh/poll.h"
#include "libssh/pki.h"

/* macro to check for "agent failure" message */
#define agent_failed(x) \
  (((x) == SSH_AGENT_FAILURE) || ((x) == SSH_COM_AGENT2_FAILURE) || \
   ((x) == SSH2_AGENT_FAILURE))

static uint32_t agent_get_u32(const void *vp) {
  const uint8_t *p = (const uint8_t *)vp;
  uint32_t v;

  v  = (uint32_t)p[0] << 24;
  v |= (uint32_t)p[1] << 16;
  v |= (uint32_t)p[2] << 8;
  v |= (uint32_t)p[3];

  return v;
}

static void agent_put_u32(void *vp, uint32_t v) {
  uint8_t *p = (uint8_t *)vp;

  p[0] = (uint8_t)(v >> 24) & 0xff;
  p[1] = (uint8_t)(v >> 16) & 0xff;
  p[2] = (uint8_t)(v >> 8) & 0xff;
  p[3] = (uint8_t)v & 0xff;
}

static size_t atomicio(struct ssh_agent_struct *agent, void *buf, size_t n, int do_read) {
  char *b = buf;
  size_t pos = 0;
  ssize_t res;
  ssh_pollfd_t pfd;
  ssh_channel channel = agent->channel;
  socket_t fd;

  /* Using a socket ? */
  if (channel == NULL) {
    fd = ssh_socket_get_fd_in(agent->sock);
    pfd.fd = fd;
    pfd.events = do_read ? POLLIN : POLLOUT;

    while (n > pos) {
      if (do_read) {
        res = read(fd, b + pos, n - pos);
      } else {
        res = write(fd, b + pos, n - pos);
      }
      switch (res) {
      case -1:
        if (errno == EINTR) {
          continue;
        }
#ifdef EWOULDBLOCK
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
#else
          if (errno == EAGAIN) {
#endif
            (void) ssh_poll(&pfd, 1, -1);
            continue;
          }
          return 0;
      case 0:
        /* read returns 0 on end-of-file */
        errno = do_read ? 0 : EPIPE;
        return pos;
      default:
        pos += (size_t) res;
        }
      }
      return pos;
    } else {
      /* using an SSH channel */
      while (n > pos){
        if (do_read)
          res = ssh_channel_read(channel,b + pos, n-pos, 0);
        else
          res = ssh_channel_write(channel, b+pos, n-pos);
        if (res == SSH_AGAIN)
          continue;
        if (res == SSH_ERROR)
          return 0;
        pos += (size_t)res;
      }
      return pos;
    }
}

ssh_agent agent_new(struct ssh_session_struct *session) {
  ssh_agent agent = NULL;

  agent = malloc(sizeof(struct ssh_agent_struct));
  if (agent == NULL) {
    return NULL;
  }
  ZERO_STRUCTP(agent);

  agent->count = 0;
  agent->sock = ssh_socket_new(session);
  if (agent->sock == NULL) {
    SAFE_FREE(agent);
    return NULL;
  }
  agent->channel = NULL;
  return agent;
}

static void agent_set_channel(struct ssh_agent_struct *agent, ssh_channel channel){
  agent->channel = channel;
}

/** @brief sets the SSH agent channel.
 * The SSH agent channel will be used to authenticate this client using
 * an agent through a channel, from another session. The most likely use
 * is to implement SSH Agent forwarding into a SSH proxy.
 * @param[in] channel a SSH channel from another session.
 * @returns SSH_OK in case of success
 *          SSH_ERROR in case of an error
 */
int ssh_set_agent_channel(ssh_session session, ssh_channel channel){
  if (!session)
    return SSH_ERROR;
  if (!session->agent){
    ssh_set_error(session, SSH_REQUEST_DENIED, "Session has no active agent");
    return SSH_ERROR;
  }
  agent_set_channel(session->agent, channel);
  return SSH_OK;
}


void agent_close(struct ssh_agent_struct *agent) {
  if (agent == NULL) {
    return;
  }

  if (getenv("SSH_AUTH_SOCK")) {
    ssh_socket_close(agent->sock);
  }
}

void agent_free(ssh_agent agent) {
  if (agent) {
    if (agent->ident) {
      ssh_buffer_free(agent->ident);
    }
    if (agent->sock) {
      agent_close(agent);
      ssh_socket_free(agent->sock);
    }
    SAFE_FREE(agent);
  }
}

static int agent_connect(ssh_session session) {
  const char *auth_sock = NULL;

  if (session == NULL || session->agent == NULL) {
    return -1;
  }

  if (session->agent->channel != NULL)
    return 0;

  auth_sock = getenv("SSH_AUTH_SOCK");

  if (auth_sock && *auth_sock) {
    if (ssh_socket_unix(session->agent->sock, auth_sock) < 0) {
      return -1;
    }
    return 0;
  }

  return -1;
}

#if 0
static int agent_decode_reply(struct ssh_session_struct *session, int type) {
  switch (type) {
    case SSH_AGENT_FAILURE:
    case SSH2_AGENT_FAILURE:
    case SSH_COM_AGENT2_FAILURE:
      ssh_log(session, SSH_LOG_RARE, "SSH_AGENT_FAILURE");
      return 0;
    case SSH_AGENT_SUCCESS:
      return 1;
    default:
      ssh_set_error(session, SSH_FATAL,
          "Bad response from authentication agent: %d", type);
      break;
  }

  return -1;
}
#endif

static int agent_talk(struct ssh_session_struct *session,
    struct ssh_buffer_struct *request, struct ssh_buffer_struct *reply) {
  uint32_t len = 0;
  uint8_t payload[1024] = {0};

  len = buffer_get_rest_len(request);
  SSH_LOG(SSH_LOG_TRACE, "Request length: %u", len);
  agent_put_u32(payload, len);

  /* send length and then the request packet */
  if (atomicio(session->agent, payload, 4, 0) == 4) {
    if (atomicio(session->agent, buffer_get_rest(request), len, 0)
        != len) {
      SSH_LOG(SSH_LOG_WARN, "atomicio sending request failed: %s",
          strerror(errno));
      return -1;
    }
  } else {
    SSH_LOG(SSH_LOG_WARN,
        "atomicio sending request length failed: %s",
        strerror(errno));
    return -1;
  }

  /* wait for response, read the length of the response packet */
  if (atomicio(session->agent, payload, 4, 1) != 4) {
    SSH_LOG(SSH_LOG_WARN, "atomicio read response length failed: %s",
        strerror(errno));
    return -1;
  }

  len = agent_get_u32(payload);
  if (len > 256 * 1024) {
    ssh_set_error(session, SSH_FATAL,
        "Authentication response too long: %u", len);
    return -1;
  }
  SSH_LOG(SSH_LOG_TRACE, "Response length: %u", len);

  while (len > 0) {
    size_t n = len;
    if (n > sizeof(payload)) {
      n = sizeof(payload);
    }
    if (atomicio(session->agent, payload, n, 1) != n) {
      SSH_LOG(SSH_LOG_WARN,
          "Error reading response from authentication socket.");
      return -1;
    }
    if (buffer_add_data(reply, payload, n) < 0) {
      SSH_LOG(SSH_LOG_WARN, "Not enough space");
      return -1;
    }
    len -= n;
  }

  return 0;
}

int ssh_agent_get_ident_count(struct ssh_session_struct *session) {
  ssh_buffer request = NULL;
  ssh_buffer reply = NULL;
  unsigned int type = 0;
  unsigned int c1 = 0, c2 = 0;
  uint8_t buf[4] = {0};
  int rc;

  switch (session->version) {
    case 1:
      c1 = SSH_AGENTC_REQUEST_RSA_IDENTITIES;
      c2 = SSH_AGENT_RSA_IDENTITIES_ANSWER;
      break;
    case 2:
      c1 = SSH2_AGENTC_REQUEST_IDENTITIES;
      c2 = SSH2_AGENT_IDENTITIES_ANSWER;
      break;
    default:
      return 0;
  }

  /* send message to the agent requesting the list of identities */
  request = ssh_buffer_new();
  if (request == NULL) {
      ssh_set_error_oom(session);
      return -1;
  }
  if (buffer_add_u8(request, c1) < 0) {
      ssh_set_error_oom(session);
      ssh_buffer_free(request);
      return -1;
  }

  reply = ssh_buffer_new();
  if (reply == NULL) {
    ssh_buffer_free(request);
    ssh_set_error(session, SSH_FATAL, "Not enough space");
    return -1;
  }

  if (agent_talk(session, request, reply) < 0) {
    ssh_buffer_free(request);
    ssh_buffer_free(reply);
    return 0;
  }
  ssh_buffer_free(request);

  /* get message type and verify the answer */
  rc = buffer_get_u8(reply, (uint8_t *) &type);
  if (rc != sizeof(uint8_t)) {
    ssh_set_error(session, SSH_FATAL,
        "Bad authentication reply size: %d", rc);
    ssh_buffer_free(reply);
    return -1;
  }

  SSH_LOG(SSH_LOG_WARN,
      "Answer type: %d, expected answer: %d",
      type, c2);

  if (agent_failed(type)) {
      ssh_buffer_free(reply);
      return 0;
  } else if (type != c2) {
      ssh_set_error(session, SSH_FATAL,
          "Bad authentication reply message type: %d", type);
      ssh_buffer_free(reply);
      return -1;
  }

  buffer_get_u32(reply, (uint32_t *) buf);
  session->agent->count = agent_get_u32(buf);
  SSH_LOG(SSH_LOG_DEBUG, "Agent count: %d",
      session->agent->count);
  if (session->agent->count > 1024) {
    ssh_set_error(session, SSH_FATAL,
        "Too many identities in authentication reply: %d",
        session->agent->count);
    ssh_buffer_free(reply);
    return -1;
  }

  if (session->agent->ident) {
    buffer_reinit(session->agent->ident);
  }
  session->agent->ident = reply;

  return session->agent->count;
}

/* caller has to free commment */
ssh_key ssh_agent_get_first_ident(struct ssh_session_struct *session,
                              char **comment) {
    if (ssh_agent_get_ident_count(session) > 0) {
        return ssh_agent_get_next_ident(session, comment);
    }

    return NULL;
}

/* caller has to free commment */
ssh_key ssh_agent_get_next_ident(struct ssh_session_struct *session,
    char **comment) {
    struct ssh_key_struct *key;
    struct ssh_string_struct *blob = NULL;
    struct ssh_string_struct *tmp = NULL;
    int rc;

    if (session->agent->count == 0) {
        return NULL;
    }

    switch(session->version) {
        case 1:
            return NULL;
        case 2:
            /* get the blob */
            blob = buffer_get_ssh_string(session->agent->ident);
            if (blob == NULL) {
                return NULL;
            }

            /* get the comment */
            tmp = buffer_get_ssh_string(session->agent->ident);
            if (tmp == NULL) {
                ssh_string_free(blob);

                return NULL;
            }

            if (comment) {
                *comment = ssh_string_to_char(tmp);
            } else {
                ssh_string_free(blob);
                ssh_string_free(tmp);

                return NULL;
            }
            ssh_string_free(tmp);

            /* get key from blob */
            rc = ssh_pki_import_pubkey_blob(blob, &key);
            ssh_string_free(blob);
            if (rc == SSH_ERROR) {
                return NULL;
            }
            break;
        default:
            return NULL;
    }

    return key;
}

int agent_is_running(ssh_session session) {
  if (session == NULL || session->agent == NULL) {
    return 0;
  }

  if (ssh_socket_is_open(session->agent->sock)) {
    return 1;
  } else {
    if (agent_connect(session) < 0) {
      return 0;
    } else {
      return 1;
    }
  }

  return 0;
}

ssh_string ssh_agent_sign_data(ssh_session session,
                               const ssh_key pubkey,
                               struct ssh_buffer_struct *data)
{
    ssh_buffer request;
    ssh_buffer reply;
    ssh_string key_blob;
    ssh_string sig_blob;
    int type = SSH2_AGENT_FAILURE;
    int flags = 0;
    uint32_t dlen;
    int rc;

    request = ssh_buffer_new();
    if (request == NULL) {
        return NULL;
    }

    /* create request */
    if (buffer_add_u8(request, SSH2_AGENTC_SIGN_REQUEST) < 0) {
        ssh_buffer_free(request);
        return NULL;
    }

    rc = ssh_pki_export_pubkey_blob(pubkey, &key_blob);
    if (rc < 0) {
        ssh_buffer_free(request);
        return NULL;
    }

    /* adds len + blob */
    rc = buffer_add_ssh_string(request, key_blob);
    ssh_string_free(key_blob);
    if (rc < 0) {
        ssh_buffer_free(request);
        return NULL;
    }

    /* Add data */
    dlen = buffer_get_rest_len(data);
    if (buffer_add_u32(request, htonl(dlen)) < 0) {
        ssh_buffer_free(request);
        return NULL;
    }
    if (buffer_add_data(request, buffer_get_rest(data), dlen) < 0) {
        ssh_buffer_free(request);
        return NULL;
    }

    if (buffer_add_u32(request, htonl(flags)) < 0) {
        ssh_buffer_free(request);
        return NULL;
    }

    reply = ssh_buffer_new();
    if (reply == NULL) {
        ssh_buffer_free(request);
        return NULL;
    }

    /* send the request */
    if (agent_talk(session, request, reply) < 0) {
        ssh_buffer_free(request);
        ssh_buffer_free(reply);
        return NULL;
    }
    ssh_buffer_free(request);

    /* check if reply is valid */
    if (buffer_get_u8(reply, (uint8_t *) &type) != sizeof(uint8_t)) {
        ssh_buffer_free(reply);
        return NULL;
    }

    if (agent_failed(type)) {
        SSH_LOG(SSH_LOG_WARN, "Agent reports failure in signing the key");
        ssh_buffer_free(reply);
        return NULL;
    } else if (type != SSH2_AGENT_SIGN_RESPONSE) {
        ssh_set_error(session, SSH_FATAL, "Bad authentication response: %d", type);
        ssh_buffer_free(reply);
        return NULL;
    }

    sig_blob = buffer_get_ssh_string(reply);
    ssh_buffer_free(reply);

    return sig_blob;
}

#endif /* _WIN32 */

/* vim: set ts=4 sw=4 et cindent: */
