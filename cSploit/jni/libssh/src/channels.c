/*
 * channels.c - SSH channel functions
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2013 by Aris Adamantiadis
 * Copyright (c) 2009-2013 by Andreas Schneider <asn@cryptomilk.org>
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

#include <limits.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libssh/priv.h"
#include "libssh/ssh2.h"
#include "libssh/buffer.h"
#include "libssh/packet.h"
#include "libssh/socket.h"
#include "libssh/channels.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/messages.h"
#if WITH_SERVER
#include "libssh/server.h"
#endif

#define WINDOWBASE 1280000
#define WINDOWLIMIT (WINDOWBASE/2)

/*
 * All implementations MUST be able to process packets with an
 * uncompressed payload length of 32768 bytes or less and a total packet
 * size of 35000 bytes or less.
 */
#define CHANNEL_MAX_PACKET 32768
#define CHANNEL_INITIAL_WINDOW 64000

/**
 * @defgroup libssh_channel The SSH channel functions
 * @ingroup libssh
 *
 * Functions that manage a SSH channel.
 *
 * @{
 */

static ssh_channel channel_from_msg(ssh_session session, ssh_buffer packet);

/**
 * @brief Allocate a new channel.
 *
 * @param[in]  session  The ssh session to use.
 *
 * @return              A pointer to a newly allocated channel, NULL on error.
 */
ssh_channel ssh_channel_new(ssh_session session) {
  ssh_channel channel = NULL;

  if(session == NULL) {
      return NULL;
  }

  channel = malloc(sizeof(struct ssh_channel_struct));
  if (channel == NULL) {
    ssh_set_error_oom(session);
    return NULL;
  }
  memset(channel,0,sizeof(struct ssh_channel_struct));

  channel->stdout_buffer = ssh_buffer_new();
  if (channel->stdout_buffer == NULL) {
    ssh_set_error_oom(session);
    SAFE_FREE(channel);
    return NULL;
  }

  channel->stderr_buffer = ssh_buffer_new();
  if (channel->stderr_buffer == NULL) {
    ssh_set_error_oom(session);
    ssh_buffer_free(channel->stdout_buffer);
    SAFE_FREE(channel);
    return NULL;
  }

  channel->session = session;
  channel->version = session->version;
  channel->exit_status = -1;
  channel->flags = SSH_CHANNEL_FLAG_NOT_BOUND;

  if(session->channels == NULL) {
    session->channels = ssh_list_new();
  }
  ssh_list_prepend(session->channels, channel);
  return channel;
}

/**
 * @internal
 *
 * @brief Create a new channel identifier.
 *
 * @param[in]  session  The SSH session to use.
 *
 * @return              The new channel identifier.
 */
uint32_t ssh_channel_new_id(ssh_session session) {
  return ++(session->maxchannel);
}

/**
 * @internal
 *
 * @brief Handle a SSH_PACKET_CHANNEL_OPEN_CONFIRMATION packet.
 *
 * Constructs the channel object.
 */
SSH_PACKET_CALLBACK(ssh_packet_channel_open_conf){
  uint32_t channelid=0;
  uint32_t tmp;
  ssh_channel channel;
  (void)type;
  (void)user;

  SSH_LOG(SSH_LOG_PACKET,"Received SSH2_MSG_CHANNEL_OPEN_CONFIRMATION");

  buffer_get_u32(packet, &channelid);
  channelid=ntohl(channelid);
  channel=ssh_channel_from_local(session,channelid);
  if(channel==NULL){
    ssh_set_error(session, SSH_FATAL,
        "Unknown channel id %lu",
        (long unsigned int) channelid);
    /* TODO: Set error marking in channel object */

    return SSH_PACKET_USED;
  }

  buffer_get_u32(packet, &tmp);
  channel->remote_channel = ntohl(tmp);

  buffer_get_u32(packet, &tmp);
  channel->remote_window = ntohl(tmp);

  buffer_get_u32(packet,&tmp);
  channel->remote_maxpacket=ntohl(tmp);

  SSH_LOG(SSH_LOG_PROTOCOL,
      "Received a CHANNEL_OPEN_CONFIRMATION for channel %d:%d",
      channel->local_channel,
      channel->remote_channel);
  SSH_LOG(SSH_LOG_PROTOCOL,
      "Remote window : %lu, maxpacket : %lu",
      (long unsigned int) channel->remote_window,
      (long unsigned int) channel->remote_maxpacket);

  channel->state = SSH_CHANNEL_STATE_OPEN;
  channel->flags &= ~SSH_CHANNEL_FLAG_NOT_BOUND;

  return SSH_PACKET_USED;
}

/**
 * @internal
 *
 * @brief Handle a SSH_CHANNEL_OPEN_FAILURE and set the state of the channel.
 */
SSH_PACKET_CALLBACK(ssh_packet_channel_open_fail){

  ssh_channel channel;
  ssh_string error_s;
  char *error = NULL;
  uint32_t code;
  (void)user;
  (void)type;
  channel=channel_from_msg(session,packet);
  if(channel==NULL){
    SSH_LOG(SSH_LOG_RARE,"Invalid channel in packet");
    return SSH_PACKET_USED;
  }
  buffer_get_u32(packet, &code);

  error_s = buffer_get_ssh_string(packet);
  if(error_s != NULL)
    error = ssh_string_to_char(error_s);
  ssh_string_free(error_s);
  if (error == NULL) {
    ssh_set_error_oom(session);
    return SSH_PACKET_USED;
  }

  ssh_set_error(session, SSH_REQUEST_DENIED,
      "Channel opening failure: channel %u error (%lu) %s",
      channel->local_channel,
      (long unsigned int) ntohl(code),
      error);
  SAFE_FREE(error);
  channel->state=SSH_CHANNEL_STATE_OPEN_DENIED;
  return SSH_PACKET_USED;
}

static int ssh_channel_open_termination(void *c){
  ssh_channel channel = (ssh_channel) c;
  if (channel->state != SSH_CHANNEL_STATE_OPENING ||
      channel->session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

/**
 * @internal
 *
 * @brief Open a channel by sending a SSH_OPEN_CHANNEL message and
 *        wait for the reply.
 *
 * @param[in]  channel  The current channel.
 *
 * @param[in]  type_c   A C string describing the kind of channel (e.g. "exec").
 *
 * @param[in]  window   The receiving window of the channel. The window is the
 *                      maximum size of data that can stay in buffers and
 *                      network.
 *
 * @param[in]  maxpacket The maximum packet size allowed (like MTU).
 *
 * @param[in]  payload   The buffer containing additional payload for the query.
 */
static int channel_open(ssh_channel channel, const char *type_c, int window,
    int maxpacket, ssh_buffer payload) {
  ssh_session session = channel->session;
  ssh_string type = NULL;
  int err=SSH_ERROR;

  switch(channel->state){
  case SSH_CHANNEL_STATE_NOT_OPEN:
    break;
  case SSH_CHANNEL_STATE_OPENING:
    goto pending;
  case SSH_CHANNEL_STATE_OPEN:
  case SSH_CHANNEL_STATE_CLOSED:
  case SSH_CHANNEL_STATE_OPEN_DENIED:
    goto end;
  default:
    ssh_set_error(session,SSH_FATAL,"Bad state in channel_open: %d",channel->state);
  }
  channel->local_channel = ssh_channel_new_id(session);
  channel->local_maxpacket = maxpacket;
  channel->local_window = window;

  SSH_LOG(SSH_LOG_PROTOCOL,
      "Creating a channel %d with %d window and %d max packet",
      channel->local_channel, window, maxpacket);

  type = ssh_string_from_char(type_c);
  if (type == NULL) {
    ssh_set_error_oom(session);

    return err;
  }

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_OPEN) < 0 ||
      buffer_add_ssh_string(session->out_buffer,type) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->local_channel)) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->local_window)) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->local_maxpacket)) < 0) {
    ssh_set_error_oom(session);
    ssh_string_free(type);

    return err;
  }

  ssh_string_free(type);

  if (payload != NULL) {
    if (buffer_add_buffer(session->out_buffer, payload) < 0) {
      ssh_set_error_oom(session);

      return err;
    }
  }
  channel->state = SSH_CHANNEL_STATE_OPENING;
  if (packet_send(session) == SSH_ERROR) {

    return err;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Sent a SSH_MSG_CHANNEL_OPEN type %s for channel %d",
      type_c, channel->local_channel);
pending:
  /* wait until channel is opened by server */
  err = ssh_handle_packets_termination(session,
                                       SSH_TIMEOUT_DEFAULT,
                                       ssh_channel_open_termination,
                                       channel);

  if (session->session_state == SSH_SESSION_STATE_ERROR)
    err = SSH_ERROR;
end:
  if(channel->state == SSH_CHANNEL_STATE_OPEN)
    err=SSH_OK;

  return err;
}

/* return channel with corresponding local id, or NULL if not found */
ssh_channel ssh_channel_from_local(ssh_session session, uint32_t id) {
  struct ssh_iterator *it;
  ssh_channel channel;

  for (it = ssh_list_get_iterator(session->channels); it != NULL ; it=it->next) {
    channel = ssh_iterator_value(ssh_channel, it);
    if (channel == NULL) {
      continue;
    }
    if (channel->local_channel == id) {
      return channel;
    }
  }

  return NULL;
}

/**
 * @internal
 * @brief grows the local window and send a packet to the other party
 * @param session SSH session
 * @param channel SSH channel
 * @param minimumsize The minimum acceptable size for the new window.
 */
static int grow_window(ssh_session session, ssh_channel channel, int minimumsize) {
  uint32_t new_window = minimumsize > WINDOWBASE ? minimumsize : WINDOWBASE;

#ifdef WITH_SSH1
  if (session->version == 1){
      channel->remote_window = new_window;

      return SSH_OK;
  }
#endif
  if(new_window <= channel->local_window){
    SSH_LOG(SSH_LOG_PROTOCOL,
        "growing window (channel %d:%d) to %d bytes : not needed (%d bytes)",
        channel->local_channel, channel->remote_channel, new_window,
        channel->local_window);

    return SSH_OK;
  }
  /* WINDOW_ADJUST packet needs a relative increment rather than an absolute
   * value, so we give here the missing bytes needed to reach new_window
   */
  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_WINDOW_ADJUST) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->remote_channel)) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(new_window - channel->local_window)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (packet_send(session) == SSH_ERROR) {
    goto error;
  }

  SSH_LOG(SSH_LOG_PROTOCOL,
      "growing window (channel %d:%d) to %d bytes",
      channel->local_channel,
      channel->remote_channel,
      new_window);

  channel->local_window = new_window;

  return SSH_OK;
error:
  buffer_reinit(session->out_buffer);

  return SSH_ERROR;
}

/**
 * @internal
 *
 * @brief Parse a channel-related packet to resolve it to a ssh_channel.
 *
 * This works on SSH1 sessions too.
 *
 * @param[in]  session  The current SSH session.
 *
 * @param[in]  packet   The buffer to parse packet from. The read pointer will
 *                      be moved after the call.
 *
 * @returns             The related ssh_channel, or NULL if the channel is
 *                      unknown or the packet is invalid.
 */
static ssh_channel channel_from_msg(ssh_session session, ssh_buffer packet) {
  ssh_channel channel;
  uint32_t chan;
#ifdef WITH_SSH1
  /* With SSH1, the channel is always the first one */
  if(session->version==1)
    return ssh_get_channel1(session);
#endif
  if (buffer_get_u32(packet, &chan) != sizeof(uint32_t)) {
    ssh_set_error(session, SSH_FATAL,
        "Getting channel from message: short read");
    return NULL;
  }

  channel = ssh_channel_from_local(session, ntohl(chan));
  if (channel == NULL) {
    ssh_set_error(session, SSH_FATAL,
        "Server specified invalid channel %lu",
        (long unsigned int) ntohl(chan));
  }

  return channel;
}

SSH_PACKET_CALLBACK(channel_rcv_change_window) {
  ssh_channel channel;
  uint32_t bytes;
  int rc;
  (void)user;
  (void)type;

  channel = channel_from_msg(session,packet);
  if (channel == NULL) {
    SSH_LOG(SSH_LOG_FUNCTIONS, "%s", ssh_get_error(session));
  }

  rc = buffer_get_u32(packet, &bytes);
  if (channel == NULL || rc != sizeof(uint32_t)) {
    SSH_LOG(SSH_LOG_PACKET,
        "Error getting a window adjust message: invalid packet");

    return SSH_PACKET_USED;
  }

  bytes = ntohl(bytes);
  SSH_LOG(SSH_LOG_PROTOCOL,
      "Adding %d bytes to channel (%d:%d) (from %d bytes)",
      bytes,
      channel->local_channel,
      channel->remote_channel,
      channel->remote_window);

  channel->remote_window += bytes;

  return SSH_PACKET_USED;
}

/* is_stderr is set to 1 if the data are extended, ie stderr */
SSH_PACKET_CALLBACK(channel_rcv_data){
  ssh_channel channel;
  ssh_string str;
  ssh_buffer buf;
  size_t len;
  int is_stderr;
  int rest;
  (void)user;

  if(type==SSH2_MSG_CHANNEL_DATA)
	  is_stderr=0;
  else
	  is_stderr=1;

  channel = channel_from_msg(session,packet);
  if (channel == NULL) {
    SSH_LOG(SSH_LOG_FUNCTIONS,
        "%s", ssh_get_error(session));

    return SSH_PACKET_USED;
  }

  if (is_stderr) {
    uint32_t ignore;
    /* uint32 data type code. we can ignore it */
    buffer_get_u32(packet, &ignore);
  }

  str = buffer_get_ssh_string(packet);
  if (str == NULL) {
    SSH_LOG(SSH_LOG_PACKET, "Invalid data packet!");

    return SSH_PACKET_USED;
  }
  len = ssh_string_len(str);

  SSH_LOG(SSH_LOG_PROTOCOL,
      "Channel receiving %" PRIdS " bytes data in %d (local win=%d remote win=%d)",
      len,
      is_stderr,
      channel->local_window,
      channel->remote_window);

  /* What shall we do in this case? Let's accept it anyway */
  if (len > channel->local_window) {
    SSH_LOG(SSH_LOG_RARE,
        "Data packet too big for our window(%" PRIdS " vs %d)",
        len,
        channel->local_window);
  }

  if (channel_default_bufferize(channel, ssh_string_data(str), len,
        is_stderr) < 0) {
    ssh_string_free(str);

    return SSH_PACKET_USED;
  }

  if (len <= channel->local_window) {
    channel->local_window -= len;
  } else {
    channel->local_window = 0; /* buggy remote */
  }

  SSH_LOG(SSH_LOG_PROTOCOL,
      "Channel windows are now (local win=%d remote win=%d)",
      channel->local_window,
      channel->remote_window);

  ssh_string_free(str);

  if(ssh_callbacks_exists(channel->callbacks, channel_data_function)) {
      if(is_stderr) {
        buf = channel->stderr_buffer;
      } else {
        buf = channel->stdout_buffer;
      }
      rest = channel->callbacks->channel_data_function(channel->session,
                                                channel,
                                                buffer_get_rest(buf),
                                                buffer_get_rest_len(buf),
                                                is_stderr,
                                                channel->callbacks->userdata);
      if(rest > 0) {
        buffer_pass_bytes(buf, rest);
      }
      if (channel->local_window + buffer_get_rest_len(buf) < WINDOWLIMIT) {
        if (grow_window(session, channel, 0) < 0) {
          return -1;
        }
      }
  }

  return SSH_PACKET_USED;
}

SSH_PACKET_CALLBACK(channel_rcv_eof) {
  ssh_channel channel;
  (void)user;
  (void)type;

  channel = channel_from_msg(session,packet);
  if (channel == NULL) {
    SSH_LOG(SSH_LOG_FUNCTIONS, "%s", ssh_get_error(session));

    return SSH_PACKET_USED;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Received eof on channel (%d:%d)",
      channel->local_channel,
      channel->remote_channel);
  /* channel->remote_window = 0; */
  channel->remote_eof = 1;

  if(ssh_callbacks_exists(channel->callbacks, channel_eof_function)) {
      channel->callbacks->channel_eof_function(channel->session,
                                               channel,
                                               channel->callbacks->userdata);
  }

  return SSH_PACKET_USED;
}

SSH_PACKET_CALLBACK(channel_rcv_close) {
	ssh_channel channel;
	(void)user;
	(void)type;

	channel = channel_from_msg(session,packet);
	if (channel == NULL) {
		SSH_LOG(SSH_LOG_FUNCTIONS, "%s", ssh_get_error(session));

		return SSH_PACKET_USED;
	}

	SSH_LOG(SSH_LOG_PACKET,
			"Received close on channel (%d:%d)",
			channel->local_channel,
			channel->remote_channel);

	if ((channel->stdout_buffer &&
			buffer_get_rest_len(channel->stdout_buffer) > 0) ||
			(channel->stderr_buffer &&
					buffer_get_rest_len(channel->stderr_buffer) > 0)) {
		channel->delayed_close = 1;
	} else {
		channel->state = SSH_CHANNEL_STATE_CLOSED;
	}
	if (channel->remote_eof == 0) {
		SSH_LOG(SSH_LOG_PACKET,
				"Remote host not polite enough to send an eof before close");
	}
	channel->remote_eof = 1;
	/*
	 * The remote eof doesn't break things if there was still data into read
	 * buffer because the eof is ignored until the buffer is empty.
	 */

	if(ssh_callbacks_exists(channel->callbacks, channel_close_function)) {
	  channel->callbacks->channel_close_function(channel->session,
	      channel,
	      channel->callbacks->userdata);
	}
	channel->flags |= SSH_CHANNEL_FLAG_CLOSED_REMOTE;
	if(channel->flags & SSH_CHANNEL_FLAG_FREED_LOCAL)
	  ssh_channel_do_free(channel);

	return SSH_PACKET_USED;
}

SSH_PACKET_CALLBACK(channel_rcv_request) {
	ssh_channel channel;
	ssh_string request_s;
	char *request;
    uint8_t status;
    int rc;
	(void)user;
	(void)type;

	channel = channel_from_msg(session,packet);
	if (channel == NULL) {
		SSH_LOG(SSH_LOG_FUNCTIONS,"%s", ssh_get_error(session));

		return SSH_PACKET_USED;
	}

	request_s = buffer_get_ssh_string(packet);
	if (request_s == NULL) {
		SSH_LOG(SSH_LOG_PACKET, "Invalid MSG_CHANNEL_REQUEST");

		return SSH_PACKET_USED;
	}

	request = ssh_string_to_char(request_s);
	ssh_string_free(request_s);
	if (request == NULL) {

		return SSH_PACKET_USED;
	}

	buffer_get_u8(packet, (uint8_t *) &status);

	if (strcmp(request,"exit-status") == 0) {
        uint32_t exit_status = 0;

		SAFE_FREE(request);
        buffer_get_u32(packet, &exit_status);
        channel->exit_status = ntohl(exit_status);
		SSH_LOG(SSH_LOG_PACKET, "received exit-status %d", channel->exit_status);

        if(ssh_callbacks_exists(channel->callbacks, channel_exit_status_function)) {
            channel->callbacks->channel_exit_status_function(channel->session,
                                                     channel,
                                                     channel->exit_status,
                                                     channel->callbacks->userdata);
        }

		return SSH_PACKET_USED;
	}

	if (strcmp(request,"signal") == 0) {
		ssh_string signal_str;
        char *sig;

		SAFE_FREE(request);
		SSH_LOG(SSH_LOG_PACKET, "received signal");

		signal_str = buffer_get_ssh_string(packet);
		if (signal_str == NULL) {
			SSH_LOG(SSH_LOG_PACKET, "Invalid MSG_CHANNEL_REQUEST");

			return SSH_PACKET_USED;
		}

		sig = ssh_string_to_char(signal_str);
		ssh_string_free(signal_str);
		if (sig == NULL) {

			return SSH_PACKET_USED;
		}


		SSH_LOG(SSH_LOG_PACKET,
				"Remote connection sent a signal SIG %s", sig);
        if(ssh_callbacks_exists(channel->callbacks, channel_signal_function)) {
            channel->callbacks->channel_signal_function(channel->session,
                                                     channel,
                                                     sig,
                                                     channel->callbacks->userdata);
        }
        SAFE_FREE(sig);

		return SSH_PACKET_USED;
	}

	if (strcmp(request, "exit-signal") == 0) {
		const char *core = "(core dumped)";
		ssh_string tmp;
		char *sig;
		char *errmsg = NULL;
		char *lang = NULL;
		uint8_t i;

		SAFE_FREE(request);

		tmp = buffer_get_ssh_string(packet);
		if (tmp == NULL) {
			SSH_LOG(SSH_LOG_PACKET, "Invalid MSG_CHANNEL_REQUEST");

			return SSH_PACKET_USED;
		}

		sig = ssh_string_to_char(tmp);
		ssh_string_free(tmp);
		if (sig == NULL) {

			return SSH_PACKET_USED;
		}

		buffer_get_u8(packet, &i);
		if (i == 0) {
			core = "";
		}

		tmp = buffer_get_ssh_string(packet);
		if (tmp == NULL) {
			SSH_LOG(SSH_LOG_PACKET, "Invalid MSG_CHANNEL_REQUEST");
            SAFE_FREE(sig);

			return SSH_PACKET_USED;
		}

		errmsg = ssh_string_to_char(tmp);
		ssh_string_free(tmp);
		if (errmsg == NULL) {
            SAFE_FREE(sig);

			return SSH_PACKET_USED;
		}

		tmp = buffer_get_ssh_string(packet);
		if (tmp == NULL) {
			SSH_LOG(SSH_LOG_PACKET, "Invalid MSG_CHANNEL_REQUEST");
            SAFE_FREE(errmsg);
            SAFE_FREE(sig);

			return SSH_PACKET_USED;
		}

		lang = ssh_string_to_char(tmp);
		ssh_string_free(tmp);
		if (lang == NULL) {
            SAFE_FREE(errmsg);
            SAFE_FREE(sig);

			return SSH_PACKET_USED;
		}

		SSH_LOG(SSH_LOG_PACKET,
				"Remote connection closed by signal SIG %s %s", sig, core);
        if(ssh_callbacks_exists(channel->callbacks, channel_exit_signal_function)) {
            channel->callbacks->channel_exit_signal_function(channel->session,
                                                     channel,
                                                     sig, i, errmsg, lang,
                                                     channel->callbacks->userdata);
        }

        SAFE_FREE(lang);
        SAFE_FREE(errmsg);
		SAFE_FREE(sig);

		return SSH_PACKET_USED;
	}
	if(strcmp(request,"keepalive@openssh.com")==0){
	  SAFE_FREE(request);
	  SSH_LOG(SSH_LOG_PROTOCOL,"Responding to Openssh's keepalive");
	  rc = buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_FAILURE);
      if (rc < 0) {
          return SSH_PACKET_USED;
      }
      rc = buffer_add_u32(session->out_buffer, htonl(channel->remote_channel));
      if (rc < 0) {
          return SSH_PACKET_USED;
      }
	  packet_send(session);

	  return SSH_PACKET_USED;
	}

  if (strcmp(request, "auth-agent-req@openssh.com") == 0) {
    SAFE_FREE(request);
    SSH_LOG(SSH_LOG_PROTOCOL, "Received an auth-agent-req request");
    if(ssh_callbacks_exists(channel->callbacks, channel_auth_agent_req_function)) {
        channel->callbacks->channel_auth_agent_req_function(channel->session, channel,
            channel->callbacks->userdata);
    }

    return SSH_PACKET_USED;
  }
#ifdef WITH_SERVER
	/* If we are here, that means we have a request that is not in the understood
	 * client requests. That means we need to create a ssh message to be passed
	 * to the user code handling ssh messages
	 */
	ssh_message_handle_channel_request(session,channel,packet,request,status);
#else
    SSH_LOG(SSH_LOG_WARNING, "Unhandled channel request %s", request);
#endif
	
	SAFE_FREE(request);

	return SSH_PACKET_USED;
}

/*
 * When data has been received from the ssh server, it can be applied to the
 * known user function, with help of the callback, or inserted here
 *
 * FIXME is the window changed?
 */
int channel_default_bufferize(ssh_channel channel, void *data, int len,
    int is_stderr) {
  ssh_session session;

  if(channel == NULL) {
      return -1;
  }

  session = channel->session;

  if(data == NULL) {
      ssh_set_error_invalid(session);
      return -1;
  }

  SSH_LOG(SSH_LOG_RARE,
      "placing %d bytes into channel buffer (stderr=%d)", len, is_stderr);
  if (is_stderr == 0) {
    /* stdout */
    if (channel->stdout_buffer == NULL) {
      channel->stdout_buffer = ssh_buffer_new();
      if (channel->stdout_buffer == NULL) {
        ssh_set_error_oom(session);
        return -1;
      }
    }

    if (buffer_add_data(channel->stdout_buffer, data, len) < 0) {
      ssh_set_error_oom(session);
      ssh_buffer_free(channel->stdout_buffer);
      channel->stdout_buffer = NULL;
      return -1;
    }
  } else {
    /* stderr */
    if (channel->stderr_buffer == NULL) {
      channel->stderr_buffer = ssh_buffer_new();
      if (channel->stderr_buffer == NULL) {
        ssh_set_error_oom(session);
        return -1;
      }
    }

    if (buffer_add_data(channel->stderr_buffer, data, len) < 0) {
      ssh_set_error_oom(session);
      ssh_buffer_free(channel->stderr_buffer);
      channel->stderr_buffer = NULL;
      return -1;
    }
  }

  return 0;
}

/**
 * @brief Open a session channel (suited for a shell, not TCP forwarding).
 *
 * @param[in]  channel  An allocated channel.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @see ssh_channel_open_forward()
 * @see ssh_channel_request_env()
 * @see ssh_channel_request_shell()
 * @see ssh_channel_request_exec()
 */
int ssh_channel_open_session(ssh_channel channel) {
  if(channel == NULL) {
      return SSH_ERROR;
  }

#ifdef WITH_SSH1
  if (channel->session->version == 1) {
    return channel_open_session1(channel);
  }
#endif

  return channel_open(channel,
                      "session",
                      CHANNEL_INITIAL_WINDOW,
                      CHANNEL_MAX_PACKET,
                      NULL);
}

/**
 * @brief Open an agent authentication forwarding channel. This type of channel
 * can be opened by a server towards a client in order to provide SSH-Agent services
 * to the server-side process. This channel can only be opened if the client
 * claimed support by sending a channel request beforehand.
 *
 * @param[in]  channel  An allocated channel.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @see ssh_channel_open_forward()
 */
int ssh_channel_open_auth_agent(ssh_channel channel){
  if(channel == NULL) {
      return SSH_ERROR;
  }

#ifdef WITH_SSH1
  if (channel->session->version == 1) {
    return SSH_ERROR;
  }
#endif

  return channel_open(channel,
                      "auth-agent@openssh.com",
                      CHANNEL_INITIAL_WINDOW,
                      CHANNEL_MAX_PACKET,
                      NULL);
}


/**
 * @brief Open a TCP/IP forwarding channel.
 *
 * @param[in]  channel  An allocated channel.
 *
 * @param[in]  remotehost The remote host to connected (host name or IP).
 *
 * @param[in]  remoteport The remote port.
 *
 * @param[in]  sourcehost The numeric IP address of the machine from where the
 *                        connection request originates. This is mostly for
 *                        logging purposes.
 *
 * @param[in]  localport  The port on the host from where the connection
 *                        originated. This is mostly for logging purposes.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @warning This function does not bind the local port and does not automatically
 *          forward the content of a socket to the channel. You still have to
 *          use channel_read and channel_write for this.
 */
int ssh_channel_open_forward(ssh_channel channel, const char *remotehost,
    int remoteport, const char *sourcehost, int localport) {
  ssh_session session;
  ssh_buffer payload = NULL;
  ssh_string str = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return rc;
  }

  session = channel->session;

  if(remotehost == NULL || sourcehost == NULL) {
      ssh_set_error_invalid(session);
      return rc;
  }

  payload = ssh_buffer_new();
  if (payload == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }
  str = ssh_string_from_char(remotehost);
  if (str == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(payload, str) < 0 ||
      buffer_add_u32(payload,htonl(remoteport)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  ssh_string_free(str);
  str = ssh_string_from_char(sourcehost);
  if (str == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(payload, str) < 0 ||
      buffer_add_u32(payload,htonl(localport)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  rc = channel_open(channel,
                    "direct-tcpip",
                    CHANNEL_INITIAL_WINDOW,
                    CHANNEL_MAX_PACKET,
                    payload);

error:
  ssh_buffer_free(payload);
  ssh_string_free(str);

  return rc;
}


/**
 * @brief Close and free a channel.
 *
 * @param[in]  channel  The channel to free.
 *
 * @warning Any data unread on this channel will be lost.
 */
void ssh_channel_free(ssh_channel channel) {
  ssh_session session;

  if (channel == NULL) {
    return;
  }

  session = channel->session;
  if (session->alive && channel->state == SSH_CHANNEL_STATE_OPEN) {
    ssh_channel_close(channel);
  }
  channel->flags |= SSH_CHANNEL_FLAG_FREED_LOCAL;

  /* The idea behind the flags is the following : it is well possible
   * that a client closes a channel that stills exists on the server side.
   * We definitively close the channel when we receive a close message *and*
   * the user closed it.
   */
  if((channel->flags & SSH_CHANNEL_FLAG_CLOSED_REMOTE)
      || (channel->flags & SSH_CHANNEL_FLAG_NOT_BOUND)){
    ssh_channel_do_free(channel);
  }
}

/**
 * @internal
 * @brief Effectively free a channel, without caring about flags
 */

void ssh_channel_do_free(ssh_channel channel){
  struct ssh_iterator *it;
  ssh_session session = channel->session;
  it = ssh_list_find(session->channels, channel);
  if(it != NULL){
    ssh_list_remove(session->channels, it);
  }
  ssh_buffer_free(channel->stdout_buffer);
  ssh_buffer_free(channel->stderr_buffer);

  /* debug trick to catch use after frees */
  memset(channel, 'X', sizeof(struct ssh_channel_struct));
  SAFE_FREE(channel);
}

/**
 * @brief Send an end of file on the channel.
 *
 * This doesn't close the channel. You may still read from it but not write.
 *
 * @param[in]  channel  The channel to send the eof to.
 *
 * @return              SSH_OK on success, SSH_ERROR if an error occurred.
 *
 * Example:
@code
   rc = ssh_channel_send_eof(channel);
   if (rc == SSH_ERROR) {
       return -1;
   }
   while(!ssh_channel_is_eof(channel)) {
       rc = ssh_channel_read(channel, buf, sizeof(buf), 0);
       if (rc == SSH_ERROR) {
           return -1;
       }
   }
   ssh_channel_close(channel);
@endcode
 *
 * @see ssh_channel_close()
 * @see ssh_channel_free()
 * @see ssh_channel_is_eof()
 */
int ssh_channel_send_eof(ssh_channel channel){
  ssh_session session;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return rc;
  }

  session = channel->session;

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_EOF) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
  if (buffer_add_u32(session->out_buffer,htonl(channel->remote_channel)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
  rc = packet_send(session);
  SSH_LOG(SSH_LOG_PACKET,
      "Sent a EOF on client channel (%d:%d)",
      channel->local_channel,
      channel->remote_channel);

  rc = ssh_channel_flush(channel);
  if(rc == SSH_ERROR)
    goto error;

  channel->local_eof = 1;

  return rc;
error:
  buffer_reinit(session->out_buffer);

  return rc;
}

/**
 * @brief Close a channel.
 *
 * This sends an end of file and then closes the channel. You won't be able
 * to recover any data the server was going to send or was in buffers.
 *
 * @param[in]  channel  The channel to close.
 *
 * @return              SSH_OK on success, SSH_ERROR if an error occurred.
 *
 * @see ssh_channel_free()
 * @see ssh_channel_is_eof()
 */
int ssh_channel_close(ssh_channel channel){
  ssh_session session;
  int rc = 0;

  if(channel == NULL) {
    return SSH_ERROR;
  }

  session = channel->session;

  if (channel->local_eof == 0) {
    rc = ssh_channel_send_eof(channel);
  }

  if (rc != SSH_OK) {
    return rc;
  }

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_CLOSE) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->remote_channel)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  rc = packet_send(session);
  SSH_LOG(SSH_LOG_PACKET,
      "Sent a close on client channel (%d:%d)",
      channel->local_channel,
      channel->remote_channel);

  if(rc == SSH_OK) {
    channel->state=SSH_CHANNEL_STATE_CLOSED;
  }

  rc = ssh_channel_flush(channel);
  if(rc == SSH_ERROR)
    goto error;

  return rc;
error:
  buffer_reinit(session->out_buffer);

  return rc;
}

/* this termination function waits for a window growing condition */
static int ssh_channel_waitwindow_termination(void *c){
  ssh_channel channel = (ssh_channel) c;
  if (channel->remote_window > 0 ||
      channel->session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

/* This termination function waits until the session is not in blocked status
 * anymore, e.g. because of a key re-exchange.
 */
static int ssh_waitsession_unblocked(void *s){
    ssh_session session = (ssh_session)s;
    switch (session->session_state){
        case SSH_SESSION_STATE_DH:
        case SSH_SESSION_STATE_INITIAL_KEX:
        case SSH_SESSION_STATE_KEXINIT_RECEIVED:
            return 0;
        default:
            return 1;
    }
}
/**
 * @internal
 * @brief Flushes a channel (and its session) until the output buffer
 *        is empty, or timeout elapsed.
 * @param channel SSH channel
 * @returns SSH_OK On success,
 *          SSH_ERROR on error
 *          SSH_AGAIN Timeout elapsed (or in nonblocking mode)
 */
int ssh_channel_flush(ssh_channel channel){
  return ssh_blocking_flush(channel->session, SSH_TIMEOUT_DEFAULT);
}

int channel_write_common(ssh_channel channel, const void *data,
    uint32_t len, int is_stderr) {
  ssh_session session;
  uint32_t origlen = len;
  size_t effectivelen;
  size_t maxpacketlen;
  int rc;

  if(channel == NULL) {
      return -1;
  }
  session = channel->session;
  if(data == NULL) {
      ssh_set_error_invalid(session);
      return -1;
  }

  if (len > INT_MAX) {
      SSH_LOG(SSH_LOG_PROTOCOL,
              "Length (%u) is bigger than INT_MAX", len);
      return SSH_ERROR;
  }

  /*
   * Handle the max packet len from remote side, be nice
   * 10 bytes for the headers
   */
  maxpacketlen = channel->remote_maxpacket - 10;

  if (channel->local_eof) {
    ssh_set_error(session, SSH_REQUEST_DENIED,
        "Can't write to channel %d:%d  after EOF was sent",
        channel->local_channel,
        channel->remote_channel);
    return -1;
  }

  if (channel->state != SSH_CHANNEL_STATE_OPEN || channel->delayed_close != 0) {
    ssh_set_error(session, SSH_REQUEST_DENIED, "Remote channel is closed");

    return -1;
  }
  if (channel->session->session_state == SSH_SESSION_STATE_ERROR){

    return SSH_ERROR;
  }
#ifdef WITH_SSH1
  if (channel->version == 1) {
    rc = channel_write1(channel, data, len);

    return rc;
  }
#endif
  if (ssh_waitsession_unblocked(session) == 0){
    rc = ssh_handle_packets_termination(session, SSH_TIMEOUT_DEFAULT,
            ssh_waitsession_unblocked, session);
    if (rc == SSH_ERROR || !ssh_waitsession_unblocked(session))
        goto out;
  }
  while (len > 0) {
    if (channel->remote_window < len) {
      SSH_LOG(SSH_LOG_PROTOCOL,
          "Remote window is %d bytes. going to write %d bytes",
          channel->remote_window,
          len);
      /* What happens when the channel window is zero? */
      if(channel->remote_window == 0) {
          /* nothing can be written */
          SSH_LOG(SSH_LOG_PROTOCOL,
                "Wait for a growing window message...");
          rc = ssh_handle_packets_termination(session, SSH_TIMEOUT_DEFAULT,
              ssh_channel_waitwindow_termination,channel);
          if (rc == SSH_ERROR ||
              !ssh_channel_waitwindow_termination(channel) ||
              channel->session->session_state == SSH_SESSION_STATE_ERROR)
            goto out;
          continue;
      }
      effectivelen = len > channel->remote_window ? channel->remote_window : len;
    } else {
      effectivelen = len;
    }
    effectivelen = effectivelen > maxpacketlen ? maxpacketlen : effectivelen;
    if (buffer_add_u8(session->out_buffer, is_stderr ?
				SSH2_MSG_CHANNEL_EXTENDED_DATA : SSH2_MSG_CHANNEL_DATA) < 0 ||
        buffer_add_u32(session->out_buffer,
          htonl(channel->remote_channel)) < 0) {
        ssh_set_error_oom(session);
        goto error;
    }
    /* stderr message has an extra field */
    if (is_stderr && 
        buffer_add_u32(session->out_buffer, htonl(SSH2_EXTENDED_DATA_STDERR)) < 0) {
        ssh_set_error_oom(session);
        goto error;
    }
    /* append payload data */
    if (buffer_add_u32(session->out_buffer, htonl(effectivelen)) < 0 ||
        buffer_add_data(session->out_buffer, data, effectivelen) < 0) {
      ssh_set_error_oom(session);
      goto error;
    }

    if (packet_send(session) == SSH_ERROR) {
      return SSH_ERROR;
    }

    SSH_LOG(SSH_LOG_RARE,
        "channel_write wrote %ld bytes", (long int) effectivelen);

    channel->remote_window -= effectivelen;
    len -= effectivelen;
    data = ((uint8_t*)data + effectivelen);
  }
  /* it's a good idea to flush the socket now */
  rc = ssh_channel_flush(channel);
  if(rc == SSH_ERROR)
    goto error;
out:
  return (int)(origlen - len);

error:
  buffer_reinit(session->out_buffer);

  return SSH_ERROR;
}

uint32_t ssh_channel_window_size(ssh_channel channel) {
    return channel->remote_window;
}

/**
 * @brief Blocking write on a channel.
 *
 * @param[in]  channel  The channel to write to.
 *
 * @param[in]  data     A pointer to the data to write.
 *
 * @param[in]  len      The length of the buffer to write to.
 *
 * @return              The number of bytes written, SSH_ERROR on error.
 *
 * @see ssh_channel_read()
 */
int ssh_channel_write(ssh_channel channel, const void *data, uint32_t len) {
  return channel_write_common(channel, data, len, 0);
}

/**
 * @brief Check if the channel is open or not.
 *
 * @param[in]  channel  The channel to check.
 *
 * @return              0 if channel is closed, nonzero otherwise.
 *
 * @see ssh_channel_is_closed()
 */
int ssh_channel_is_open(ssh_channel channel) {
    if(channel == NULL) {
        return 0;
    }
    return (channel->state == SSH_CHANNEL_STATE_OPEN && channel->session->alive != 0);
}

/**
 * @brief Check if the channel is closed or not.
 *
 * @param[in]  channel  The channel to check.
 *
 * @return              0 if channel is opened, nonzero otherwise.
 *
 * @see ssh_channel_is_open()
 */
int ssh_channel_is_closed(ssh_channel channel) {
    if(channel == NULL) {
        return SSH_ERROR;
    }
    return (channel->state != SSH_CHANNEL_STATE_OPEN || channel->session->alive == 0);
}

/**
 * @brief Check if remote has sent an EOF.
 *
 * @param[in]  channel  The channel to check.
 *
 * @return              0 if there is no EOF, nonzero otherwise.
 */
int ssh_channel_is_eof(ssh_channel channel) {
  if(channel == NULL) {
      return SSH_ERROR;
  }
  if ((channel->stdout_buffer &&
        buffer_get_rest_len(channel->stdout_buffer) > 0) ||
      (channel->stderr_buffer &&
       buffer_get_rest_len(channel->stderr_buffer) > 0)) {
    return 0;
  }

  return (channel->remote_eof != 0);
}

/**
 * @brief Put the channel into blocking or nonblocking mode.
 *
 * @param[in]  channel  The channel to use.
 *
 * @param[in]  blocking A boolean for blocking or nonblocking.
 *
 * @warning    A side-effect of this is to put the whole session
 *             in non-blocking mode.
 * @see ssh_set_blocking()
 */
void ssh_channel_set_blocking(ssh_channel channel, int blocking) {
  if(channel == NULL) {
      return;
  }
  ssh_set_blocking(channel->session,blocking);
}

/**
 * @internal
 *
 * @brief handle a SSH_CHANNEL_SUCCESS packet and set the channel state.
 *
 * This works on SSH1 sessions too.
 */
SSH_PACKET_CALLBACK(ssh_packet_channel_success){
  ssh_channel channel;
  (void)type;
  (void)user;

  channel=channel_from_msg(session,packet);
  if (channel == NULL) {
    SSH_LOG(SSH_LOG_FUNCTIONS, "%s", ssh_get_error(session));
    return SSH_PACKET_USED;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Received SSH_CHANNEL_SUCCESS on channel (%d:%d)",
      channel->local_channel,
      channel->remote_channel);
  if(channel->request_state != SSH_CHANNEL_REQ_STATE_PENDING){
    SSH_LOG(SSH_LOG_RARE, "SSH_CHANNEL_SUCCESS received in incorrect state %d",
        channel->request_state);
  } else {
    channel->request_state=SSH_CHANNEL_REQ_STATE_ACCEPTED;
  }

  return SSH_PACKET_USED;
}

/**
 * @internal
 *
 * @brief Handle a SSH_CHANNEL_FAILURE packet and set the channel state.
 *
 * This works on SSH1 sessions too.
 */
SSH_PACKET_CALLBACK(ssh_packet_channel_failure){
  ssh_channel channel;
  (void)type;
  (void)user;

  channel=channel_from_msg(session,packet);
  if (channel == NULL) {
    SSH_LOG(SSH_LOG_FUNCTIONS, "%s", ssh_get_error(session));

    return SSH_PACKET_USED;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Received SSH_CHANNEL_FAILURE on channel (%d:%d)",
      channel->local_channel,
      channel->remote_channel);
  if(channel->request_state != SSH_CHANNEL_REQ_STATE_PENDING){
    SSH_LOG(SSH_LOG_RARE, "SSH_CHANNEL_FAILURE received in incorrect state %d",
        channel->request_state);
  } else {
    channel->request_state=SSH_CHANNEL_REQ_STATE_DENIED;
  }

  return SSH_PACKET_USED;
}

static int ssh_channel_request_termination(void *c){
  ssh_channel channel = (ssh_channel)c;
  if(channel->request_state != SSH_CHANNEL_REQ_STATE_PENDING ||
      channel->session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

static int channel_request(ssh_channel channel, const char *request,
    ssh_buffer buffer, int reply) {
  ssh_session session = channel->session;
  ssh_string req = NULL;
  int rc = SSH_ERROR;

  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }

  req = ssh_string_from_char(request);
  if (req == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_REQUEST) < 0 ||
      buffer_add_u32(session->out_buffer, htonl(channel->remote_channel)) < 0 ||
      buffer_add_ssh_string(session->out_buffer, req) < 0 ||
      buffer_add_u8(session->out_buffer, reply == 0 ? 0 : 1) < 0) {
    ssh_set_error_oom(session);
    ssh_string_free(req);
    goto error;
  }
  ssh_string_free(req);

  if (buffer != NULL) {
    if (buffer_add_data(session->out_buffer, buffer_get_rest(buffer),
        buffer_get_rest_len(buffer)) < 0) {
      ssh_set_error_oom(session);
      goto error;
    }
  }
  channel->request_state = SSH_CHANNEL_REQ_STATE_PENDING;
  if (packet_send(session) == SSH_ERROR) {
    return rc;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Sent a SSH_MSG_CHANNEL_REQUEST %s", request);
  if (reply == 0) {
    channel->request_state = SSH_CHANNEL_REQ_STATE_NONE;
    return SSH_OK;
  }
pending:
  rc = ssh_handle_packets_termination(session,
                                      SSH_TIMEOUT_DEFAULT,
                                      ssh_channel_request_termination,
                                      channel);

  if(session->session_state == SSH_SESSION_STATE_ERROR || rc == SSH_ERROR) {
      channel->request_state = SSH_CHANNEL_REQ_STATE_ERROR;
  }
  /* we received something */
  switch (channel->request_state){
    case SSH_CHANNEL_REQ_STATE_ERROR:
      rc=SSH_ERROR;
      break;
    case SSH_CHANNEL_REQ_STATE_DENIED:
      ssh_set_error(session, SSH_REQUEST_DENIED,
          "Channel request %s failed", request);
      rc=SSH_ERROR;
      break;
    case SSH_CHANNEL_REQ_STATE_ACCEPTED:
      SSH_LOG(SSH_LOG_PROTOCOL,
          "Channel request %s success",request);
      rc=SSH_OK;
      break;
    case SSH_CHANNEL_REQ_STATE_PENDING:
      rc = SSH_AGAIN;
      return rc;
    case SSH_CHANNEL_REQ_STATE_NONE:
      /* Never reached */
      ssh_set_error(session, SSH_FATAL, "Invalid state in channel_request()");
      rc=SSH_ERROR;
      break;
  }
  channel->request_state=SSH_CHANNEL_REQ_STATE_NONE;

  return rc;
error:
  buffer_reinit(session->out_buffer);

  return rc;
}

/**
 * @brief Request a pty with a specific type and size.
 *
 * @param[in]  channel  The channel to sent the request.
 *
 * @param[in]  terminal The terminal type ("vt100, xterm,...").
 *
 * @param[in]  col      The number of columns.
 *
 * @param[in]  row      The number of rows.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 */
int ssh_channel_request_pty_size(ssh_channel channel, const char *terminal,
    int col, int row) {
  ssh_session session;
  ssh_string term = NULL;
  ssh_buffer buffer = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  session = channel->session;

  if(terminal == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }

#ifdef WITH_SSH1
  if (channel->version==1) {
    rc = channel_request_pty_size1(channel,terminal, col, row);

    return rc;
    }
#endif
  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  term = ssh_string_from_char(terminal);
  if (term == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, term) < 0 ||
      buffer_add_u32(buffer, htonl(col)) < 0 ||
      buffer_add_u32(buffer, htonl(row)) < 0 ||
      buffer_add_u32(buffer, 0) < 0 ||
      buffer_add_u32(buffer, 0) < 0 ||
      buffer_add_u32(buffer, htonl(1)) < 0 || /* Add a 0byte string */
      buffer_add_u8(buffer, 0) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
pending:
  rc = channel_request(channel, "pty-req", buffer, 1);
error:
  ssh_buffer_free(buffer);
  ssh_string_free(term);

  return rc;
}

/**
 * @brief Request a PTY.
 *
 * @param[in]  channel  The channel to send the request.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @see ssh_channel_request_pty_size()
 */
int ssh_channel_request_pty(ssh_channel channel) {
  return ssh_channel_request_pty_size(channel, "xterm", 80, 24);
}

/**
 * @brief Change the size of the terminal associated to a channel.
 *
 * @param[in]  channel  The channel to change the size.
 *
 * @param[in]  cols     The new number of columns.
 *
 * @param[in]  rows     The new number of rows.
 *
 * @return              SSH_OK on success, SSH_ERROR if an error occurred.
 *
 * @warning Do not call it from a signal handler if you are not sure any other
 *          libssh function using the same channel/session is running at same
 *          time (not 100% threadsafe).
 */
int ssh_channel_change_pty_size(ssh_channel channel, int cols, int rows) {
  ssh_session session = channel->session;
  ssh_buffer buffer = NULL;
  int rc = SSH_ERROR;

#ifdef WITH_SSH1
  if (channel->version == 1) {
    rc = channel_change_pty_size1(channel,cols,rows);

    return rc;
  }
#endif

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_u32(buffer, htonl(cols)) < 0 ||
      buffer_add_u32(buffer, htonl(rows)) < 0 ||
      buffer_add_u32(buffer, 0) < 0 ||
      buffer_add_u32(buffer, 0) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  rc = channel_request(channel, "window-change", buffer, 0);
error:
  ssh_buffer_free(buffer);

  return rc;
}

/**
 * @brief Request a shell.
 *
 * @param[in]  channel  The channel to send the request.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 */
int ssh_channel_request_shell(ssh_channel channel) {
    if(channel == NULL) {
        return SSH_ERROR;
    }
#ifdef WITH_SSH1
  if (channel->version == 1) {
    return channel_request_shell1(channel);
  }
#endif
  return channel_request(channel, "shell", NULL, 1);
}

/**
 * @brief Request a subsystem (for example "sftp").
 *
 * @param[in]  channel  The channel to send the request.
 *
 * @param[in]  subsys   The subsystem to request (for example "sftp").
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @warning You normally don't have to call it for sftp, see sftp_new().
 */
int ssh_channel_request_subsystem(ssh_channel channel, const char *subsys) {
  ssh_buffer buffer = NULL;
  ssh_string subsystem = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(subsys == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }
  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  subsystem = ssh_string_from_char(subsys);
  if (subsystem == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, subsystem) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
pending:
  rc = channel_request(channel, "subsystem", buffer, 1);
error:
  ssh_buffer_free(buffer);
  ssh_string_free(subsystem);

  return rc;
}

int ssh_channel_request_sftp( ssh_channel channel){
    if(channel == NULL) {
        return SSH_ERROR;
    }
    return ssh_channel_request_subsystem(channel, "sftp");
}

static ssh_string generate_cookie(void) {
  static const char *hex = "0123456789abcdef";
  char s[36];
  unsigned char rnd[16];
  int i;

  ssh_get_random(rnd,sizeof(rnd),0);
  for (i = 0; i < 16; i++) {
    s[i*2] = hex[rnd[i] & 0x0f];
    s[i*2+1] = hex[rnd[i] >> 4];
  }
  s[32] = '\0';
  return ssh_string_from_char(s);
}

/**
 * @brief Sends the "x11-req" channel request over an existing session channel.
 *
 * This will enable redirecting the display of the remote X11 applications to
 * local X server over an secure tunnel.
 *
 * @param[in]  channel  An existing session channel where the remote X11
 *                      applications are going to be executed.
 *
 * @param[in]  single_connection A boolean to mark only one X11 app will be
 *                               redirected.
 *
 * @param[in]  protocol A x11 authentication protocol. Pass NULL to use the
 *                      default value MIT-MAGIC-COOKIE-1.
 *
 * @param[in]  cookie   A x11 authentication cookie. Pass NULL to generate
 *                      a random cookie.
 *
 * @param[in] screen_number The screen number.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 */
int ssh_channel_request_x11(ssh_channel channel, int single_connection, const char *protocol,
    const char *cookie, int screen_number) {
  ssh_buffer buffer = NULL;
  ssh_string p = NULL;
  ssh_string c = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  p = ssh_string_from_char(protocol ? protocol : "MIT-MAGIC-COOKIE-1");
  if (p == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (cookie) {
    c = ssh_string_from_char(cookie);
  } else {
    c = generate_cookie();
  }
  if (c == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_u8(buffer, single_connection == 0 ? 0 : 1) < 0 ||
      buffer_add_ssh_string(buffer, p) < 0 ||
      buffer_add_ssh_string(buffer, c) < 0 ||
      buffer_add_u32(buffer, htonl(screen_number)) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
pending:
  rc = channel_request(channel, "x11-req", buffer, 1);

error:
  ssh_buffer_free(buffer);
  ssh_string_free(p);
  ssh_string_free(c);
  return rc;
}

static ssh_channel ssh_channel_accept(ssh_session session, int channeltype,
    int timeout_ms, int *destination_port) {
#ifndef _WIN32
  static const struct timespec ts = {
    .tv_sec = 0,
    .tv_nsec = 50000000 /* 50ms */
  };
#endif
  ssh_message msg = NULL;
  ssh_channel channel = NULL;
  struct ssh_iterator *iterator;
  int t;

  /*
   * We sleep for 50 ms in ssh_handle_packets() and later sleep for
   * 50 ms. So we need to decrement by 100 ms.
   */
  for (t = timeout_ms; t >= 0; t -= 100) {
    if (timeout_ms == 0) {
        ssh_handle_packets(session, 0);
    } else {
        ssh_handle_packets(session, 50);
    }

    if (session->ssh_message_list) {
      iterator = ssh_list_get_iterator(session->ssh_message_list);
      while (iterator) {
        msg = (ssh_message)iterator->data;
        if (ssh_message_type(msg) == SSH_REQUEST_CHANNEL_OPEN &&
            ssh_message_subtype(msg) == channeltype) {
          ssh_list_remove(session->ssh_message_list, iterator);
          channel = ssh_message_channel_request_open_reply_accept(msg);
          if(destination_port) {
            *destination_port=msg->channel_request_open.destination_port;
          }

          ssh_message_free(msg);
          return channel;
        }
        iterator = iterator->next;
      }
    }
    if(t>0){
#ifdef _WIN32
      Sleep(50); /* 50ms */
#else
      nanosleep(&ts, NULL);
#endif
    }
  }

  ssh_set_error(session, SSH_NO_ERROR, "No channel request of this type from server");
  return NULL;
}

/**
 * @brief Accept an X11 forwarding channel.
 *
 * @param[in]  channel  An x11-enabled session channel.
 *
 * @param[in]  timeout_ms Timeout in milliseconds.
 *
 * @return              A newly created channel, or NULL if no X11 request from
 *                      the server.
 */
ssh_channel ssh_channel_accept_x11(ssh_channel channel, int timeout_ms) {
  return ssh_channel_accept(channel->session, SSH_CHANNEL_X11, timeout_ms, NULL);
}

/**
 * @internal
 *
 * @brief Handle a SSH_REQUEST_SUCCESS packet normally sent after a global
 * request.
 */
SSH_PACKET_CALLBACK(ssh_request_success){
  (void)type;
  (void)user;
  (void)packet;

  SSH_LOG(SSH_LOG_PACKET,
      "Received SSH_REQUEST_SUCCESS");
  if(session->global_req_state != SSH_CHANNEL_REQ_STATE_PENDING){
    SSH_LOG(SSH_LOG_RARE, "SSH_REQUEST_SUCCESS received in incorrect state %d",
        session->global_req_state);
  } else {
    session->global_req_state=SSH_CHANNEL_REQ_STATE_ACCEPTED;
  }

  return SSH_PACKET_USED;
}

/**
 * @internal
 *
 * @brief Handle a SSH_REQUEST_DENIED packet normally sent after a global
 * request.
 */
SSH_PACKET_CALLBACK(ssh_request_denied){
  (void)type;
  (void)user;
  (void)packet;

  SSH_LOG(SSH_LOG_PACKET,
      "Received SSH_REQUEST_FAILURE");
  if(session->global_req_state != SSH_CHANNEL_REQ_STATE_PENDING){
    SSH_LOG(SSH_LOG_RARE, "SSH_REQUEST_DENIED received in incorrect state %d",
        session->global_req_state);
  } else {
    session->global_req_state=SSH_CHANNEL_REQ_STATE_DENIED;
  }

  return SSH_PACKET_USED;

}

static int ssh_global_request_termination(void *s){
  ssh_session session = (ssh_session) s;
  if (session->global_req_state != SSH_CHANNEL_REQ_STATE_PENDING ||
      session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

/**
 * @internal
 *
 * @brief Send a global request (needed for forward listening) and wait for the
 * result.
 *
 * @param[in]  session  The SSH session handle.
 *
 * @param[in]  request  The type of request (defined in RFC).
 *
 * @param[in]  buffer   Additional data to put in packet.
 *
 * @param[in]  reply    Set if you expect a reply from server.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 */
static int global_request(ssh_session session, const char *request,
    ssh_buffer buffer, int reply) {
  ssh_string req = NULL;
  int rc;

  switch (session->global_req_state) {
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }

  rc = buffer_add_u8(session->out_buffer, SSH2_MSG_GLOBAL_REQUEST);
  if (rc < 0) {
      goto error;
  }

  req = ssh_string_from_char(request);
  if (req == NULL) {
      ssh_set_error_oom(session);
      rc = SSH_ERROR;
      goto error;
  }

  rc = buffer_add_ssh_string(session->out_buffer, req);
  ssh_string_free(req);
  if (rc < 0) {
      ssh_set_error_oom(session);
      rc = SSH_ERROR;
      goto error;
  }

  rc = buffer_add_u8(session->out_buffer, reply == 0 ? 0 : 1);
  if (rc < 0) {
      ssh_set_error_oom(session);
      rc = SSH_ERROR;
      goto error;
  }

  if (buffer != NULL) {
      rc = buffer_add_data(session->out_buffer,
                           buffer_get_rest(buffer),
                           buffer_get_rest_len(buffer));
      if (rc < 0) {
          ssh_set_error_oom(session);
          rc = SSH_ERROR;
          goto error;
      }
  }

  session->global_req_state = SSH_CHANNEL_REQ_STATE_PENDING;
  rc = packet_send(session);
  if (rc == SSH_ERROR) {
      return rc;
  }

  SSH_LOG(SSH_LOG_PACKET,
      "Sent a SSH_MSG_GLOBAL_REQUEST %s", request);

  if (reply == 0) {
      session->global_req_state = SSH_CHANNEL_REQ_STATE_NONE;

      return SSH_OK;
  }
pending:
  rc = ssh_handle_packets_termination(session,
                                      SSH_TIMEOUT_DEFAULT,
                                      ssh_global_request_termination,
                                      session);

  if(rc==SSH_ERROR || session->session_state == SSH_SESSION_STATE_ERROR){
    session->global_req_state = SSH_CHANNEL_REQ_STATE_ERROR;
  }
  switch(session->global_req_state){
    case SSH_CHANNEL_REQ_STATE_ACCEPTED:
      SSH_LOG(SSH_LOG_PROTOCOL, "Global request %s success",request);
      rc=SSH_OK;
      break;
    case SSH_CHANNEL_REQ_STATE_DENIED:
      SSH_LOG(SSH_LOG_PACKET,
          "Global request %s failed", request);
      ssh_set_error(session, SSH_REQUEST_DENIED,
          "Global request %s failed", request);
      rc=SSH_ERROR;
      break;
    case SSH_CHANNEL_REQ_STATE_ERROR:
    case SSH_CHANNEL_REQ_STATE_NONE:
      rc = SSH_ERROR;
      break;
    case SSH_CHANNEL_REQ_STATE_PENDING:
      return SSH_AGAIN;
  }
  session->global_req_state = SSH_CHANNEL_REQ_STATE_NONE;

  return rc;
error:
  buffer_reinit(session->out_buffer);

  return rc;
}

/**
 * @brief Sends the "tcpip-forward" global request to ask the server to begin
 *        listening for inbound connections.
 *
 * @param[in]  session  The ssh session to send the request.
 *
 * @param[in]  address  The address to bind to on the server. Pass NULL to bind
 *                      to all available addresses on all protocol families
 *                      supported by the server.
 *
 * @param[in]  port     The port to bind to on the server. Pass 0 to ask the
 *                      server to allocate the next available unprivileged port
 *                      number
 *
 * @param[in]  bound_port The pointer to get actual bound port. Pass NULL to
 *                        ignore.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 **/
int ssh_forward_listen(ssh_session session, const char *address, int port, int *bound_port) {
  ssh_buffer buffer = NULL;
  ssh_string addr = NULL;
  int rc = SSH_ERROR;
  uint32_t tmp;

  if(session->global_req_state != SSH_CHANNEL_REQ_STATE_NONE)
    goto pending;

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  addr = ssh_string_from_char(address ? address : "");
  if (addr == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, addr) < 0 ||
      buffer_add_u32(buffer, htonl(port)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
pending:
  rc = global_request(session, "tcpip-forward", buffer, 1);

  /* TODO: FIXME no guarantee the last packet we received contains
   * that info */
  if (rc == SSH_OK && port == 0 && bound_port) {
    buffer_get_u32(session->in_buffer, &tmp);
    *bound_port = ntohl(tmp);
  }

error:
  ssh_buffer_free(buffer);
  ssh_string_free(addr);
  return rc;
}

/**
 * @brief Accept an incoming TCP/IP forwarding channel.
 *
 * @param[in]  session    The ssh session to use.
 *
 * @param[in]  timeout_ms A timeout in milliseconds.
 *
 * @return Newly created channel, or NULL if no incoming channel request from
 *         the server
 */
ssh_channel ssh_forward_accept(ssh_session session, int timeout_ms) {
  return ssh_channel_accept(session, SSH_CHANNEL_FORWARDED_TCPIP, timeout_ms, NULL);
}

/**
 * @brief Accept an incoming TCP/IP forwarding channel and get information
 * about incomming connection
 * @param[in]  session    The ssh session to use.
 *
 * @param[in]  timeout_ms A timeout in milliseconds.
 *
 * @param[in]  destination_port A pointer to destination port or NULL.
 *
 * @return Newly created channel, or NULL if no incoming channel request from
 *         the server
 */
ssh_channel ssh_channel_accept_forward(ssh_session session, int timeout_ms, int* destination_port) {
  return ssh_channel_accept(session, SSH_CHANNEL_FORWARDED_TCPIP, timeout_ms, destination_port);
}

/**
 * @brief Sends the "cancel-tcpip-forward" global request to ask the server to
 *        cancel the tcpip-forward request.
 *
 * @param[in]  session  The ssh session to send the request.
 *
 * @param[in]  address  The bound address on the server.
 *
 * @param[in]  port     The bound port on the server.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 */
int ssh_forward_cancel(ssh_session session, const char *address, int port) {
  ssh_buffer buffer = NULL;
  ssh_string addr = NULL;
  int rc = SSH_ERROR;

  if(session->global_req_state != SSH_CHANNEL_REQ_STATE_NONE)
    goto pending;

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  addr = ssh_string_from_char(address ? address : "");
  if (addr == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, addr) < 0 ||
      buffer_add_u32(buffer, htonl(port)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
pending:
  rc = global_request(session, "cancel-tcpip-forward", buffer, 1);

error:
  ssh_buffer_free(buffer);
  ssh_string_free(addr);
  return rc;
}

/**
 * @brief Set environment variables.
 *
 * @param[in]  channel  The channel to set the environment variables.
 *
 * @param[in]  name     The name of the variable.
 *
 * @param[in]  value    The value to set.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 * @warning Some environment variables may be refused by security reasons.
 */
int ssh_channel_request_env(ssh_channel channel, const char *name, const char *value) {
  ssh_buffer buffer = NULL;
  ssh_string str = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(name == NULL || value == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }
  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }
  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  str = ssh_string_from_char(name);
  if (str == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, str) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  ssh_string_free(str);
  str = ssh_string_from_char(value);
  if (str == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, str) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
pending:
  rc = channel_request(channel, "env", buffer,1);
error:
  ssh_buffer_free(buffer);
  ssh_string_free(str);

  return rc;
}

/**
 * @brief Run a shell command without an interactive shell.
 *
 * This is similar to 'sh -c command'.
 *
 * @param[in]  channel  The channel to execute the command.
 *
 * @param[in]  cmd      The command to execute
 *                      (e.g. "ls ~/ -al | grep -i reports").
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * Example:
@code
   rc = channel_request_exec(channel, "ps aux");
   if (rc > 0) {
       return -1;
   }

   while ((rc = channel_read(channel, buffer, sizeof(buffer), 0)) > 0) {
       if (fwrite(buffer, 1, rc, stdout) != (unsigned int) rc) {
           return -1;
       }
   }
@endcode
 *
 * @see ssh_channel_request_shell()
 */
int ssh_channel_request_exec(ssh_channel channel, const char *cmd) {
  ssh_buffer buffer = NULL;
  ssh_string command = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(cmd == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }

#ifdef WITH_SSH1
  if (channel->version == 1) {
    return channel_request_exec1(channel, cmd);
  }
#endif
  switch(channel->request_state){
  case SSH_CHANNEL_REQ_STATE_NONE:
    break;
  default:
    goto pending;
  }
  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  command = ssh_string_from_char(cmd);
  if (command == NULL) {
    goto error;
    ssh_set_error_oom(channel->session);
  }

  if (buffer_add_ssh_string(buffer, command) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
pending:
  rc = channel_request(channel, "exec", buffer, 1);
error:
  ssh_buffer_free(buffer);
  ssh_string_free(command);
  return rc;
}


/**
 * @brief Send a signal to remote process (as described in RFC 4254, section 6.9).
 *
 * Sends a signal 'sig' to the remote process.
 * Note, that remote system may not support signals concept.
 * In such a case this request will be silently ignored.
 * Only SSH-v2 is supported (I'm not sure about SSH-v1).
 *
 * OpenSSH doesn't support signals yet, see:
 * https://bugzilla.mindrot.org/show_bug.cgi?id=1424
 *
 * @param[in]  channel  The channel to send signal.
 *
 * @param[in]  sig      The signal to send (without SIG prefix)
 *                      \n\n
 *                      SIGABRT  -> ABRT \n
 *                      SIGALRM  -> ALRM \n
 *                      SIGFPE   -> FPE  \n
 *                      SIGHUP   -> HUP  \n
 *                      SIGILL   -> ILL  \n
 *                      SIGINT   -> INT  \n
 *                      SIGKILL  -> KILL \n
 *                      SIGPIPE  -> PIPE \n
 *                      SIGQUIT  -> QUIT \n
 *                      SIGSEGV  -> SEGV \n
 *                      SIGTERM  -> TERM \n
 *                      SIGUSR1  -> USR1 \n
 *                      SIGUSR2  -> USR2 \n
 *
 * @return              SSH_OK on success, SSH_ERROR if an error occurred
 *                      (including attempts to send signal via SSH-v1 session).
 */
int ssh_channel_request_send_signal(ssh_channel channel, const char *sig) {
  ssh_buffer buffer = NULL;
  ssh_string encoded_signal = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(sig == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }

#ifdef WITH_SSH1
  if (channel->version == 1) {
    return SSH_ERROR; // TODO: Add support for SSH-v1 if possible.
  }
#endif

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  encoded_signal = ssh_string_from_char(sig);
  if (encoded_signal == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_ssh_string(buffer, encoded_signal) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  rc = channel_request(channel, "signal", buffer, 0);
error:
  ssh_buffer_free(buffer);
  ssh_string_free(encoded_signal);
  return rc;
}


/**
 * @brief Read data from a channel into a buffer.
 *
 * @param[in]  channel  The channel to read from.
 *
 * @param[in]  buffer   The buffer which will get the data.
 *
 * @param[in]  count    The count of bytes to be read. If it is bigger than 0,
 *                      the exact size will be read, else (bytes=0) it will
 *                      return once anything is available.
 *
 * @param is_stderr     A boolean value to mark reading from the stderr stream.
 *
 * @return              The number of bytes read, 0 on end of file or SSH_ERROR
 *                      on error.
 * @deprecated          Please use ssh_channel_read instead
 * @warning             This function doesn't work in nonblocking/timeout mode
 * @see ssh_channel_read
 */
int channel_read_buffer(ssh_channel channel, ssh_buffer buffer, uint32_t count,
    int is_stderr) {
  ssh_session session;
  char buffer_tmp[8192];
  int r;
  uint32_t total=0;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  session = channel->session;

  if(buffer == NULL) {
      ssh_set_error_invalid(channel->session);
      return SSH_ERROR;
  }

  buffer_reinit(buffer);
  if(count==0){
    do {
      r=ssh_channel_poll(channel, is_stderr);
      if(r < 0){
        return r;
      }
      if(r > 0){
        r=ssh_channel_read(channel, buffer_tmp, r, is_stderr);
        if(r < 0){
          return r;
        }
        if(buffer_add_data(buffer,buffer_tmp,r) < 0){
          ssh_set_error_oom(session);
          r = SSH_ERROR;
        }

        return r;
      }
      if(ssh_channel_is_eof(channel)){
        return 0;
      }
      ssh_handle_packets(channel->session, SSH_TIMEOUT_INFINITE);
    } while (r == 0);
  }
  while(total < count){
    r=ssh_channel_read(channel, buffer_tmp, sizeof(buffer_tmp), is_stderr);
    if(r<0){
      return r;
    }
    if(r==0){
      return total;
    }
    if(buffer_add_data(buffer,buffer_tmp,r) < 0){
      ssh_set_error_oom(session);

      return SSH_ERROR;
    }
    total += r;
  }

  return total;
}

struct ssh_channel_read_termination_struct {
  ssh_channel channel;
  uint32_t count;
  ssh_buffer buffer;
};

static int ssh_channel_read_termination(void *s){
  struct ssh_channel_read_termination_struct *ctx = s;
  if (buffer_get_rest_len(ctx->buffer) >= ctx->count ||
      ctx->channel->remote_eof ||
      ctx->channel->session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

/* TODO FIXME Fix the delayed close thing */
/* TODO FIXME Fix the blocking behaviours */

/**
 * @brief Reads data from a channel.
 *
 * @param[in]  channel  The channel to read from.
 *
 * @param[in]  dest     The destination buffer which will get the data.
 *
 * @param[in]  count    The count of bytes to be read.
 *
 * @param[in]  is_stderr A boolean value to mark reading from the stderr flow.
 *
 * @return              The number of bytes read, 0 on end of file or SSH_ERROR
 *                      on error. In nonblocking mode it Can return 0 if no data
 *                      is available or SSH_AGAIN.
 *
 * @warning This function may return less than count bytes of data, and won't
 *          block until count bytes have been read.
 * @warning The read function using a buffer has been renamed to
 *          channel_read_buffer().
 */
int ssh_channel_read(ssh_channel channel, void *dest, uint32_t count, int is_stderr)
{
    return ssh_channel_read_timeout(channel, dest, count, is_stderr, -1);
}

/**
 * @brief Reads data from a channel.
 *
 * @param[in]  channel     The channel to read from.
 *
 * @param[in]  dest        The destination buffer which will get the data.
 *
 * @param[in]  count       The count of bytes to be read.
 *
 * @param[in]  is_stderr   A boolean value to mark reading from the stderr flow.
 *
 * @param[in]  timeout_ms  A timeout in milliseconds. A value of -1 means
 *                         infinite timeout.
 *
 * @return              The number of bytes read, 0 on end of file or SSH_ERROR
 *                      on error. In nonblocking mode it Can return 0 if no data
 *                      is available or SSH_AGAIN.
 *
 * @warning This function may return less than count bytes of data, and won't
 *          block until count bytes have been read.
 * @warning The read function using a buffer has been renamed to
 *          channel_read_buffer().
 */
int ssh_channel_read_timeout(ssh_channel channel,
                             void *dest,
                             uint32_t count,
                             int is_stderr,
                             int timeout)
{
  ssh_session session;
  ssh_buffer stdbuf;
  uint32_t len;
  struct ssh_channel_read_termination_struct ctx;
  int rc;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(dest == NULL) {
      ssh_set_error_invalid(channel->session);
      return SSH_ERROR;
  }

  session = channel->session;
  stdbuf = channel->stdout_buffer;

  if (count == 0) {
    return 0;
  }

  if (is_stderr) {
    stdbuf=channel->stderr_buffer;
  }

  /*
   * We may have problem if the window is too small to accept as much data
   * as asked
   */
  SSH_LOG(SSH_LOG_PROTOCOL,
      "Read (%d) buffered : %d bytes. Window: %d",
      count,
      buffer_get_rest_len(stdbuf),
      channel->local_window);

  if (count > buffer_get_rest_len(stdbuf) + channel->local_window) {
    if (grow_window(session, channel, count - buffer_get_rest_len(stdbuf)) < 0) {
      return -1;
    }
  }

  /* block reading until at least one byte has been read
  *  and ignore the trivial case count=0
  */
  ctx.channel = channel;
  ctx.buffer = stdbuf;
  ctx.count = 1;

  if (timeout < 0) {
      timeout = SSH_TIMEOUT_DEFAULT;
  }

  rc = ssh_handle_packets_termination(session,
                                      timeout,
                                      ssh_channel_read_termination,
                                      &ctx);
  if (rc == SSH_ERROR){
    return rc;
  }
  if (channel->session->session_state == SSH_SESSION_STATE_ERROR){
      return SSH_ERROR;
  }
  if (channel->remote_eof && buffer_get_rest_len(stdbuf) == 0) {
    return 0;
  }
  len = buffer_get_rest_len(stdbuf);
  /* Read count bytes if len is greater, everything otherwise */
  len = (len > count ? count : len);
  memcpy(dest, buffer_get_rest(stdbuf), len);
  buffer_pass_bytes(stdbuf,len);
  /* Authorize some buffering while userapp is busy */
  if (channel->local_window < WINDOWLIMIT) {
    if (grow_window(session, channel, 0) < 0) {
      return -1;
    }
  }

  return len;
}

/**
 * @brief Do a nonblocking read on the channel.
 *
 * A nonblocking read on the specified channel. it will return <= count bytes of
 * data read atomically.
 *
 * @param[in]  channel  The channel to read from.
 *
 * @param[in]  dest     A pointer to a destination buffer.
 *
 * @param[in]  count    The count of bytes of data to be read.
 *
 * @param[in]  is_stderr A boolean to select the stderr stream.
 *
 * @return              The number of bytes read, 0 if nothing is available or
 *                      SSH_ERROR on error.
 *
 * @warning Don't forget to check for EOF as it would return 0 here.
 *
 * @see ssh_channel_is_eof()
 */
int ssh_channel_read_nonblocking(ssh_channel channel, void *dest, uint32_t count,
    int is_stderr) {
  ssh_session session;
  int to_read;
  int rc;
  int blocking;

  if(channel == NULL) {
      return SSH_ERROR;
  }
  if(dest == NULL) {
      ssh_set_error_invalid(channel->session);
      return SSH_ERROR;
  }

  session = channel->session;

  to_read = ssh_channel_poll(channel, is_stderr);

  if (to_read <= 0) {
      if (channel->session->session_state == SSH_SESSION_STATE_ERROR){
          return SSH_ERROR;
      }

      return to_read; /* may be an error code */
  }

  if (to_read > (int)count) {
    to_read = (int)count;
  }
  blocking = ssh_is_blocking(session);
  ssh_set_blocking(session, 0);
  rc = ssh_channel_read(channel, dest, to_read, is_stderr);
  ssh_set_blocking(session,blocking);

  return rc;
}

/**
 * @brief Polls a channel for data to read.
 *
 * @param[in]  channel  The channel to poll.
 *
 * @param[in]  is_stderr A boolean to select the stderr stream.
 *
 * @return              The number of bytes available for reading, 0 if nothing
 *                      is available or SSH_ERROR on error.
 *
 * @warning When the channel is in EOF state, the function returns SSH_EOF.
 *
 * @see ssh_channel_is_eof()
 */
int ssh_channel_poll(ssh_channel channel, int is_stderr){
  ssh_buffer stdbuf;

  if(channel == NULL) {
      return SSH_ERROR;
  }

  stdbuf = channel->stdout_buffer;

  if (is_stderr) {
    stdbuf = channel->stderr_buffer;
  }

  if (buffer_get_rest_len(stdbuf) == 0 && channel->remote_eof == 0) {
    if (channel->session->session_state == SSH_SESSION_STATE_ERROR){
      return SSH_ERROR;
    }
    if (ssh_handle_packets(channel->session, SSH_TIMEOUT_NONBLOCKING)==SSH_ERROR) {
      return SSH_ERROR;
    }
  }

  if (buffer_get_rest_len(stdbuf) > 0){
  	return buffer_get_rest_len(stdbuf);
  }

  if (channel->remote_eof) {
    return SSH_EOF;
  }

  return buffer_get_rest_len(stdbuf);
}

/**
 * @brief Polls a channel for data to read, waiting for a certain timeout.
 *
 * @param[in]  channel   The channel to poll.
 * @param[in]  timeout   Set an upper limit on the time for which this function
 *                       will block, in milliseconds. Specifying a negative value
 *                       means an infinite timeout. This parameter is passed to
 *                       the poll() function.
 * @param[in]  is_stderr A boolean to select the stderr stream.
 *
 * @return              The number of bytes available for reading,
 *                      0 if nothing is available (timeout elapsed),
 *                      SSH_EOF on end of file,
 *                      SSH_ERROR on error.
 *
 * @warning When the channel is in EOF state, the function returns SSH_EOF.
 *
 * @see ssh_channel_is_eof()
 */
int ssh_channel_poll_timeout(ssh_channel channel, int timeout, int is_stderr){
  ssh_session session;
  ssh_buffer stdbuf;
  struct ssh_channel_read_termination_struct ctx;
  int rc;

  if(channel == NULL) {
      return SSH_ERROR;
  }

  session = channel->session;
  stdbuf = channel->stdout_buffer;

  if (is_stderr) {
    stdbuf = channel->stderr_buffer;
  }
  ctx.buffer = stdbuf;
  ctx.channel = channel;
  ctx.count = 1;
  rc = ssh_handle_packets_termination(channel->session, timeout,
      ssh_channel_read_termination, &ctx);
  if(rc ==SSH_ERROR || session->session_state == SSH_SESSION_STATE_ERROR){
    rc = SSH_ERROR;
    goto end;
  }
  rc = buffer_get_rest_len(stdbuf);
  if(rc > 0)
    goto end;
  if (channel->remote_eof)
    rc = SSH_EOF;
end:
  return rc;
}

/**
 * @brief Recover the session in which belongs a channel.
 *
 * @param[in]  channel  The channel to recover the session from.
 *
 * @return              The session pointer.
 */
ssh_session ssh_channel_get_session(ssh_channel channel) {
  if(channel == NULL) {
      return NULL;
  }

  return channel->session;
}

static int ssh_channel_exit_status_termination(void *c){
  ssh_channel channel = c;
  if(channel->exit_status != -1 ||
      /* When a channel is closed, no exit status message can
       * come anymore */
      (channel->flags & SSH_CHANNEL_FLAG_CLOSED_REMOTE) ||
      channel->session->session_state == SSH_SESSION_STATE_ERROR)
    return 1;
  else
    return 0;
}

/**
 * @brief Get the exit status of the channel (error code from the executed
 *        instruction).
 *
 * @param[in]  channel  The channel to get the status from.
 *
 * @returns             The exit status, -1 if no exit status has been returned
 *                      (yet).
 * @warning             This function may block until a timeout (or never)
 *                      if the other side is not willing to close the channel.
 */
int ssh_channel_get_exit_status(ssh_channel channel) {
  int rc;
  if(channel == NULL) {
      return SSH_ERROR;
  }
  rc = ssh_handle_packets_termination(channel->session,
                                      SSH_TIMEOUT_DEFAULT,
                                      ssh_channel_exit_status_termination,
                                      channel);
  if (rc == SSH_ERROR || channel->session->session_state ==
      SSH_SESSION_STATE_ERROR)
    return SSH_ERROR;
  return channel->exit_status;
}

/*
 * This function acts as a meta select.
 *
 * First, channels are analyzed to seek potential can-write or can-read ones,
 * then if no channel has been elected, it goes in a loop with the posix
 * select(2).
 * This is made in two parts: protocol select and network select. The protocol
 * select does not use the network functions at all
 */
static int channel_protocol_select(ssh_channel *rchans, ssh_channel *wchans,
    ssh_channel *echans, ssh_channel *rout, ssh_channel *wout, ssh_channel *eout) {
  ssh_channel chan;
  int i;
  int j = 0;

  for (i = 0; rchans[i] != NULL; i++) {
    chan = rchans[i];

    while (ssh_channel_is_open(chan) && ssh_socket_data_available(chan->session->socket)) {
      ssh_handle_packets(chan->session, SSH_TIMEOUT_NONBLOCKING);
    }

    if ((chan->stdout_buffer && buffer_get_rest_len(chan->stdout_buffer) > 0) ||
        (chan->stderr_buffer && buffer_get_rest_len(chan->stderr_buffer) > 0) ||
        chan->remote_eof) {
      rout[j] = chan;
      j++;
    }
  }
  rout[j] = NULL;

  j = 0;
  for(i = 0; wchans[i] != NULL; i++) {
    chan = wchans[i];
    /* It's not our business to seek if the file descriptor is writable */
    if (ssh_socket_data_writable(chan->session->socket) &&
        ssh_channel_is_open(chan) && (chan->remote_window > 0)) {
      wout[j] = chan;
      j++;
    }
  }
  wout[j] = NULL;

  j = 0;
  for (i = 0; echans[i] != NULL; i++) {
    chan = echans[i];

    if (!ssh_socket_is_open(chan->session->socket) || ssh_channel_is_closed(chan)) {
      eout[j] = chan;
      j++;
    }
  }
  eout[j] = NULL;

  return 0;
}

/* Just count number of pointers in the array */
static int count_ptrs(ssh_channel *ptrs) {
  int c;
  for (c = 0; ptrs[c] != NULL; c++)
    ;

  return c;
}

/**
 * @brief Act like the standard select(2) on channels.
 *
 * The list of pointers are then actualized and will only contain pointers to
 * channels that are respectively readable, writable or have an exception to
 * trap.
 *
 * @param[in]  readchans A NULL pointer or an array of channel pointers,
 *                       terminated by a NULL.
 *
 * @param[in]  writechans A NULL pointer or an array of channel pointers,
 *                        terminated by a NULL.
 *
 * @param[in]  exceptchans A NULL pointer or an array of channel pointers,
 *                         terminated by a NULL.
 *
 * @param[in]  timeout  Timeout as defined by select(2).
 *
 * @return             SSH_OK on a successful operation, SSH_EINTR if the
 *                     select(2) syscall was interrupted, then relaunch the
 *                     function.
 */
int ssh_channel_select(ssh_channel *readchans, ssh_channel *writechans,
    ssh_channel *exceptchans, struct timeval * timeout) {
  ssh_channel *rchans, *wchans, *echans;
  ssh_channel dummy = NULL;
  ssh_event event = NULL;
  int rc;
  int i;
  int tm, tm_base;
  int firstround=1;
  struct ssh_timestamp ts;

  if (timeout != NULL)
    tm_base = timeout->tv_sec * 1000 + timeout->tv_usec/1000;
  else
    tm_base = SSH_TIMEOUT_INFINITE;
  ssh_timestamp_init(&ts);
  tm = tm_base;
  /* don't allow NULL pointers */
  if (readchans == NULL) {
    readchans = &dummy;
  }

  if (writechans == NULL) {
    writechans = &dummy;
  }

  if (exceptchans == NULL) {
    exceptchans = &dummy;
  }

  if (readchans[0] == NULL && writechans[0] == NULL && exceptchans[0] == NULL) {
    /* No channel to poll?? Go away! */
    return 0;
  }

  /* Prepare the outgoing temporary arrays */
  rchans = malloc(sizeof(ssh_channel ) * (count_ptrs(readchans) + 1));
  if (rchans == NULL) {
    return SSH_ERROR;
  }

  wchans = malloc(sizeof(ssh_channel ) * (count_ptrs(writechans) + 1));
  if (wchans == NULL) {
    SAFE_FREE(rchans);
    return SSH_ERROR;
  }

  echans = malloc(sizeof(ssh_channel ) * (count_ptrs(exceptchans) + 1));
  if (echans == NULL) {
    SAFE_FREE(rchans);
    SAFE_FREE(wchans);
    return SSH_ERROR;
  }

  /*
   * First, try without doing network stuff then, use the ssh_poll
   * infrastructure to poll on all sessions.
   */
  do {
    channel_protocol_select(readchans, writechans, exceptchans,
        rchans, wchans, echans);
    if (rchans[0] != NULL || wchans[0] != NULL || echans[0] != NULL) {
      /* At least one channel has an event */
      break;
    }
    /* Add all channels' sessions right into an event object */
    if (event == NULL) {
      event = ssh_event_new();
      if (event == NULL) {
          SAFE_FREE(rchans);
          SAFE_FREE(wchans);
          SAFE_FREE(echans);

          return SSH_ERROR;
      }
      for (i = 0; readchans[i] != NULL; i++) {
        ssh_poll_get_default_ctx(readchans[i]->session);
        ssh_event_add_session(event, readchans[i]->session);
      }
      for (i = 0; writechans[i] != NULL; i++) {
        ssh_poll_get_default_ctx(writechans[i]->session);
        ssh_event_add_session(event, writechans[i]->session);
      }
      for (i = 0; exceptchans[i] != NULL; i++) {
        ssh_poll_get_default_ctx(exceptchans[i]->session);
        ssh_event_add_session(event, exceptchans[i]->session);
      }
    }
    /* Get out if the timeout has elapsed */
    if (!firstround && ssh_timeout_elapsed(&ts, tm_base)){
      break;
    }
    /* Here we go */
    rc = ssh_event_dopoll(event,tm);
    if (rc != SSH_OK){
      SAFE_FREE(rchans);
      SAFE_FREE(wchans);
      SAFE_FREE(echans);
      ssh_event_free(event);
      return rc;
    }
    tm = ssh_timeout_update(&ts, tm_base);
    firstround=0;
  } while(1);

  memcpy(readchans, rchans, (count_ptrs(rchans) + 1) * sizeof(ssh_channel ));
  memcpy(writechans, wchans, (count_ptrs(wchans) + 1) * sizeof(ssh_channel ));
  memcpy(exceptchans, echans, (count_ptrs(echans) + 1) * sizeof(ssh_channel ));
  SAFE_FREE(rchans);
  SAFE_FREE(wchans);
  SAFE_FREE(echans);
  if(event)
    ssh_event_free(event);
  return 0;
}

#if WITH_SERVER
/**
 * @brief Blocking write on a channel stderr.
 *
 * @param[in]  channel  The channel to write to.
 *
 * @param[in]  data     A pointer to the data to write.
 *
 * @param[in]  len      The length of the buffer to write to.
 *
 * @return              The number of bytes written, SSH_ERROR on error.
 *
 * @see ssh_channel_read()
 */
int ssh_channel_write_stderr(ssh_channel channel, const void *data, uint32_t len) {
  return channel_write_common(channel, data, len, 1);
}

/**
 * @brief Open a TCP/IP reverse forwarding channel.
 *
 * @param[in]  channel  An allocated channel.
 *
 * @param[in]  remotehost The remote host to connected (host name or IP).
 *
 * @param[in]  remoteport The remote port.
 *
 * @param[in]  sourcehost The source host (your local computer). It's optional
 *                        and for logging purpose.
 *
 * @param[in]  localport  The source port (your local computer). It's optional
 *                        and for logging purpose.
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 *
 * @warning This function does not bind the local port and does not automatically
 *          forward the content of a socket to the channel. You still have to
 *          use channel_read and channel_write for this.
 */
int ssh_channel_open_reverse_forward(ssh_channel channel, const char *remotehost,
    int remoteport, const char *sourcehost, int localport) {
  ssh_session session;
  ssh_buffer payload = NULL;
  ssh_string str = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return rc;
  }
  if(remotehost == NULL || sourcehost == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }


  session = channel->session;

  if(channel->state != SSH_CHANNEL_STATE_NOT_OPEN)
    goto pending;
  payload = ssh_buffer_new();
  if (payload == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }
  str = ssh_string_from_char(remotehost);
  if (str == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(payload, str) < 0 ||
      buffer_add_u32(payload,htonl(remoteport)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }

  ssh_string_free(str);
  str = ssh_string_from_char(sourcehost);
  if (str == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(payload, str) < 0 ||
      buffer_add_u32(payload,htonl(localport)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
pending:
  rc = channel_open(channel,
                    "forwarded-tcpip",
                    CHANNEL_INITIAL_WINDOW,
                    CHANNEL_MAX_PACKET,
                    payload);

error:
  ssh_buffer_free(payload);
  ssh_string_free(str);

  return rc;
}

/**
 * @brief Open a X11 channel.
 *
 * @param[in]  channel      An allocated channel.
 *
 * @param[in]  orig_addr    The source host (the local server).
 *
 * @param[in]  orig_port    The source port (the local server).
 *
 * @return              SSH_OK on success,
 *                      SSH_ERROR if an error occurred,
 *                      SSH_AGAIN if in nonblocking mode and call has
 *                      to be done again.
 * @warning This function does not bind the local port and does not automatically
 *          forward the content of a socket to the channel. You still have to
 *          use channel_read and channel_write for this.
 */
int ssh_channel_open_x11(ssh_channel channel, 
                                        const char *orig_addr, int orig_port) {
  ssh_session session;
  ssh_buffer payload = NULL;
  ssh_string str = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return rc;
  }
  if(orig_addr == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }
  session = channel->session;

  if(channel->state != SSH_CHANNEL_STATE_NOT_OPEN)
    goto pending;

  payload = ssh_buffer_new();
  if (payload == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  str = ssh_string_from_char(orig_addr);
  if (str == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  if (buffer_add_ssh_string(payload, str) < 0 ||
      buffer_add_u32(payload,htonl(orig_port)) < 0) {
    ssh_set_error_oom(session);
    goto error;
  }
pending:
  rc = channel_open(channel,
                    "x11",
                    CHANNEL_INITIAL_WINDOW,
                    CHANNEL_MAX_PACKET,
                    payload);

error:
  ssh_buffer_free(payload);
  ssh_string_free(str);

  return rc;
}

/**
 * @brief Send the exit status to the remote process
 *
 * Sends the exit status to the remote process (as described in RFC 4254,
 * section 6.10).
 * Only SSH-v2 is supported (I'm not sure about SSH-v1).
 *
 * @param[in]  channel  The channel to send exit status.
 *
 * @param[in]  exit_status  The exit status to send
 *
 * @return     SSH_OK on success, SSH_ERROR if an error occurred.
 *             (including attempts to send exit status via SSH-v1 session).
 */
int ssh_channel_request_send_exit_status(ssh_channel channel, int exit_status) {
  ssh_buffer buffer = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return SSH_ERROR;
  }

#ifdef WITH_SSH1
  if (channel->version == 1) {
    return SSH_ERROR; // TODO: Add support for SSH-v1 if possible.
  }
#endif

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_u32(buffer, ntohl(exit_status)) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  rc = channel_request(channel, "exit-status", buffer, 0);
error:
  ssh_buffer_free(buffer);
  return rc;
}

/**
 * @brief Send an exit signal to remote process (as described in RFC 4254, section 6.10).
 *
 * Sends a signal 'sig' to the remote process.
 * Note, that remote system may not support signals concept.
 * In such a case this request will be silently ignored.
 * Only SSH-v2 is supported (I'm not sure about SSH-v1).
 *
 * @param[in]  channel  The channel to send signal.
 *
 * @param[in]  sig      The signal to send (without SIG prefix)
 *                      (e.g. "TERM" or "KILL").
 * @param[in]  core     A boolean to tell if a core was dumped
 * @param[in]  errmsg   A CRLF explanation text about the error condition
 * @param[in]  lang     The language used in the message (format: RFC 3066)
 *
 * @return              SSH_OK on success, SSH_ERROR if an error occurred
 *                      (including attempts to send signal via SSH-v1 session).
 */
int ssh_channel_request_send_exit_signal(ssh_channel channel, const char *sig,
                            int core, const char *errmsg, const char *lang) {
  ssh_buffer buffer = NULL;
  ssh_string tmp = NULL;
  int rc = SSH_ERROR;

  if(channel == NULL) {
      return rc;
  }
  if(sig == NULL || errmsg == NULL || lang == NULL) {
      ssh_set_error_invalid(channel->session);
      return rc;
  }
#ifdef WITH_SSH1
  if (channel->version == 1) {
    return SSH_ERROR; // TODO: Add support for SSH-v1 if possible.
  }
#endif

  buffer = ssh_buffer_new();
  if (buffer == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  tmp = ssh_string_from_char(sig);
  if (tmp == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
  if (buffer_add_ssh_string(buffer, tmp) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  if (buffer_add_u8(buffer, core?1:0) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  ssh_string_free(tmp);
  tmp = ssh_string_from_char(errmsg);
  if (tmp == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
  if (buffer_add_ssh_string(buffer, tmp) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  ssh_string_free(tmp);
  tmp = ssh_string_from_char(lang);
  if (tmp == NULL) {
    ssh_set_error_oom(channel->session);
    goto error;
  }
  if (buffer_add_ssh_string(buffer, tmp) < 0) {
    ssh_set_error_oom(channel->session);
    goto error;
  }

  rc = channel_request(channel, "signal", buffer, 0);
error:
  ssh_buffer_free(buffer);
  if(tmp)
    ssh_string_free(tmp);
  return rc;
}

#endif

/* @} */

/* vim: set ts=4 sw=4 et cindent: */
