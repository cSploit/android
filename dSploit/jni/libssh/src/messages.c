/*
 * messages.c - message parsion for the server
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2003-2009 by Aris Adamantiadis
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

#ifndef _WIN32
#include <netinet/in.h>
#include <arpa/inet.h>
#endif

#include "libssh/libssh.h"
#include "libssh/priv.h"
#include "libssh/ssh2.h"
#include "libssh/buffer.h"
#include "libssh/packet.h"
#include "libssh/channels.h"
#include "libssh/session.h"
#include "libssh/misc.h"
#include "libssh/keys.h"
#include "libssh/dh.h"
#include "libssh/messages.h"
#if WITH_SERVER
#include "libssh/server.h"
#endif

/**
 * @defgroup libssh_messages The SSH message functions
 * @ingroup libssh
 *
 * This file contains the Message parsing utilities for server programs using
 * libssh. The main loop of the program will call ssh_message_get(session) to
 * get messages as they come. they are not 1-1 with the protocol messages.
 * then, the user will know what kind of a message it is and use the appropriate
 * functions to handle it (or use the default handlers if she doesn't know what to
 * do
 *
 * @{
 */

static ssh_message ssh_message_new(ssh_session session){
  ssh_message msg = malloc(sizeof(struct ssh_message_struct));
  if (msg == NULL) {
    return NULL;
  }
  ZERO_STRUCTP(msg);
  msg->session = session;
  return msg;
}

#ifndef WITH_SERVER

/* Reduced version of the reply default that only reply with
 * SSH_MSG_UNIMPLEMENTED
 */
static int ssh_message_reply_default(ssh_message msg) {
  ssh_log(msg->session, SSH_LOG_FUNCTIONS, "Reporting unknown packet");

  if (buffer_add_u8(msg->session->out_buffer, SSH2_MSG_UNIMPLEMENTED) < 0)
    goto error;
  if (buffer_add_u32(msg->session->out_buffer,
      htonl(msg->session->recv_seq-1)) < 0)
    goto error;
  return packet_send(msg->session);
  error:
  return SSH_ERROR;
}

#endif

static int ssh_execute_message_callback(ssh_session session, ssh_message msg) {
    int ret;
    if(session->ssh_message_callback != NULL) {
        ret = session->ssh_message_callback(session, msg,
                session->ssh_message_callback_data);
        if(ret == 1) {
            ret = ssh_message_reply_default(msg);
            ssh_message_free(msg);
            if(ret != SSH_OK) {
                return ret;
            }
        } else {
            ssh_message_free(msg);
        }
    } else {
        ret = ssh_message_reply_default(msg);
        ssh_message_free(msg);
        if(ret != SSH_OK) {
            return ret;
        }
    }
    return SSH_OK;
}


/**
 * @internal
 *
 * @brief Add a message to the current queue of messages to be parsed.
 *
 * @param[in]  session  The SSH session to add the message.
 *
 * @param[in]  message  The message to add to the queue.
 */
void ssh_message_queue(ssh_session session, ssh_message message){
    if(message) {
        if(session->ssh_message_list == NULL) {
            if(session->ssh_message_callback != NULL) {
                ssh_execute_message_callback(session, message);
                return;
            }
            session->ssh_message_list = ssh_list_new();
        }
        ssh_list_append(session->ssh_message_list, message);
    }
}

/**
 * @internal
 *
 * @brief Pop a message from the message list and dequeue it.
 *
 * @param[in]  session  The SSH session to pop the message.
 *
 * @returns             The head message or NULL if it doesn't exist.
 */
ssh_message ssh_message_pop_head(ssh_session session){
  ssh_message msg=NULL;
  struct ssh_iterator *i;
  if(session->ssh_message_list == NULL)
    return NULL;
  i=ssh_list_get_iterator(session->ssh_message_list);
  if(i != NULL){
    msg=ssh_iterator_value(ssh_message,i);
    ssh_list_remove(session->ssh_message_list,i);
  }
  return msg;
}

/**
 * @brief Retrieve a SSH message from a SSH session.
 *
 * @param[in]  session  The SSH session to get the message.
 *
 * @returns             The SSH message received, NULL in case of error.
 *
 * @warning This function blocks until a message has been received. Betterset up
 *          a callback if this behavior is unwanted.
 */
ssh_message ssh_message_get(ssh_session session) {
  ssh_message msg = NULL;
  enter_function();

  msg=ssh_message_pop_head(session);
  if(msg) {
      leave_function();
      return msg;
  }
  if(session->ssh_message_list == NULL) {
      session->ssh_message_list = ssh_list_new();
  }
  do {
    if (ssh_handle_packets(session, -2) == SSH_ERROR) {
      leave_function();
      return NULL;
    }
    msg=ssh_list_pop_head(ssh_message, session->ssh_message_list);
  } while(msg==NULL);
  leave_function();
  return msg;
}

int ssh_message_type(ssh_message msg) {
  if (msg == NULL) {
    return -1;
  }

  return msg->type;
}

int ssh_message_subtype(ssh_message msg) {
  if (msg == NULL) {
    return -1;
  }

  switch(msg->type) {
    case SSH_REQUEST_AUTH:
      return msg->auth_request.method;
    case SSH_REQUEST_CHANNEL_OPEN:
      return msg->channel_request_open.type;
    case SSH_REQUEST_CHANNEL:
      return msg->channel_request.type;
    case SSH_REQUEST_GLOBAL:
      return msg->global_request.type;
  }

  return -1;
}

void ssh_message_free(ssh_message msg){
  if (msg == NULL) {
    return;
  }

  switch(msg->type) {
    case SSH_REQUEST_AUTH:
      SAFE_FREE(msg->auth_request.username);
      if (msg->auth_request.password) {
        memset(msg->auth_request.password, 0,
            strlen(msg->auth_request.password));
        SAFE_FREE(msg->auth_request.password);
      }
      publickey_free(msg->auth_request.public_key);
      break;
    case SSH_REQUEST_CHANNEL_OPEN:
      SAFE_FREE(msg->channel_request_open.originator);
      SAFE_FREE(msg->channel_request_open.destination);
      break;
    case SSH_REQUEST_CHANNEL:
      SAFE_FREE(msg->channel_request.TERM);
      SAFE_FREE(msg->channel_request.modes);
      SAFE_FREE(msg->channel_request.var_name);
      SAFE_FREE(msg->channel_request.var_value);
      SAFE_FREE(msg->channel_request.command);
      SAFE_FREE(msg->channel_request.subsystem);
      break;
    case SSH_REQUEST_SERVICE:
      SAFE_FREE(msg->service_request.service);
      break;
    case SSH_REQUEST_GLOBAL:
      SAFE_FREE(msg->global_request.bind_address);
      break;
  }
  ZERO_STRUCTP(msg);
  SAFE_FREE(msg);
}

SSH_PACKET_CALLBACK(ssh_packet_service_request){
  ssh_string service = NULL;
  char *service_c = NULL;
  ssh_message msg=NULL;

  enter_function();
  (void)type;
  (void)user;
  service = buffer_get_ssh_string(packet);
  if (service == NULL) {
    ssh_set_error(session, SSH_FATAL, "Invalid SSH_MSG_SERVICE_REQUEST packet");
    goto error;
  }

  service_c = ssh_string_to_char(service);
  if (service_c == NULL) {
    goto error;
  }
  ssh_log(session, SSH_LOG_PACKET,
        "Received a SERVICE_REQUEST for service %s", service_c);
  msg=ssh_message_new(session);
  if(!msg){
    SAFE_FREE(service_c);
    goto error;
  }
  msg->type=SSH_REQUEST_SERVICE;
  msg->service_request.service=service_c;
error:
  ssh_string_free(service);
  if(msg != NULL)
    ssh_message_queue(session,msg);
  leave_function();
  return SSH_PACKET_USED;
}

/**
 * @internal
 *
 * @brief Handle a SSH_MSG_MSG_USERAUTH_REQUEST packet and queue a
 * SSH Message
 */
SSH_PACKET_CALLBACK(ssh_packet_userauth_request){
  ssh_string user_s = NULL;
  ssh_string service = NULL;
  ssh_string method = NULL;
  ssh_message msg = NULL;
  char *service_c = NULL;
  char *method_c = NULL;
  uint32_t method_size = 0;

  enter_function();

  (void)user;
  (void)type;
  msg = ssh_message_new(session);
  if (msg == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  user_s = buffer_get_ssh_string(packet);
  if (user_s == NULL) {
    goto error;
  }
  service = buffer_get_ssh_string(packet);
  if (service == NULL) {
    goto error;
  }
  method = buffer_get_ssh_string(packet);
  if (method == NULL) {
    goto error;
  }

  msg->type = SSH_REQUEST_AUTH;
  msg->auth_request.username = ssh_string_to_char(user_s);
  if (msg->auth_request.username == NULL) {
    goto error;
  }
  ssh_string_free(user_s);
  user_s = NULL;

  service_c = ssh_string_to_char(service);
  if (service_c == NULL) {
    goto error;
  }
  method_c = ssh_string_to_char(method);
  if (method_c == NULL) {
    goto error;
  }
  method_size = ssh_string_len(method);

  ssh_string_free(service);
  service = NULL;
  ssh_string_free(method);
  method = NULL;

  ssh_log(session, SSH_LOG_PACKET,
      "Auth request for service %s, method %s for user '%s'",
      service_c, method_c,
      msg->auth_request.username);


  if (strncmp(method_c, "none", method_size) == 0) {
    msg->auth_request.method = SSH_AUTH_METHOD_NONE;
    SAFE_FREE(service_c);
    SAFE_FREE(method_c);
    goto end;
  }

  if (strncmp(method_c, "password", method_size) == 0) {
    ssh_string pass = NULL;
    uint8_t tmp;

    msg->auth_request.method = SSH_AUTH_METHOD_PASSWORD;
    SAFE_FREE(service_c);
    SAFE_FREE(method_c);
    buffer_get_u8(packet, &tmp);
    pass = buffer_get_ssh_string(packet);
    if (pass == NULL) {
      goto error;
    }
    msg->auth_request.password = ssh_string_to_char(pass);
    ssh_string_burn(pass);
    ssh_string_free(pass);
    pass = NULL;
    if (msg->auth_request.password == NULL) {
      goto error;
    }
    goto end;
  }

  if (strncmp(method_c, "publickey", method_size) == 0) {
    ssh_string algo = NULL;
    ssh_string publickey = NULL;
    uint8_t has_sign;

    msg->auth_request.method = SSH_AUTH_METHOD_PUBLICKEY;
    SAFE_FREE(method_c);
    buffer_get_u8(packet, &has_sign);
    algo = buffer_get_ssh_string(packet);
    if (algo == NULL) {
      goto error;
    }
    publickey = buffer_get_ssh_string(packet);
    if (publickey == NULL) {
      ssh_string_free(algo);
      algo = NULL;
      goto error;
    }
    msg->auth_request.public_key = publickey_from_string(session, publickey);
    ssh_string_free(algo);
    algo = NULL;
    ssh_string_free(publickey);
    publickey = NULL;
    if (msg->auth_request.public_key == NULL) {
       goto error;
    }
    msg->auth_request.signature_state = SSH_PUBLICKEY_STATE_NONE;
    // has a valid signature ?
    if(has_sign) {
      SIGNATURE *signature = NULL;
      ssh_public_key public_key = msg->auth_request.public_key;
      ssh_string sign = NULL;
      ssh_buffer digest = NULL;

      sign = buffer_get_ssh_string(packet);
      if(sign == NULL) {
        ssh_log(session, SSH_LOG_PACKET, "Invalid signature packet from peer");
        msg->auth_request.signature_state = SSH_PUBLICKEY_STATE_ERROR;
        goto error;
      }
      signature = signature_from_string(session, sign, public_key,
                                                       public_key->type);
      digest = ssh_userauth_build_digest(session, msg, service_c);
      if ((digest == NULL || signature == NULL) ||
          (digest != NULL && signature != NULL &&
          sig_verify(session, public_key, signature,
                     buffer_get_rest(digest), buffer_get_rest_len(digest)) < 0)) {
        ssh_log(session, SSH_LOG_PACKET, "Wrong signature from peer");

        ssh_string_free(sign);
        sign = NULL;
        ssh_buffer_free(digest);
        digest = NULL;
        signature_free(signature);
        signature = NULL;

        msg->auth_request.signature_state = SSH_PUBLICKEY_STATE_WRONG;
        goto error;
      }         
      else
        ssh_log(session, SSH_LOG_PACKET, "Valid signature received");

      ssh_buffer_free(digest);
      digest = NULL;
      ssh_string_free(sign);
      sign = NULL;
      signature_free(signature);
      signature = NULL;

      msg->auth_request.signature_state = SSH_PUBLICKEY_STATE_VALID;
    }
    SAFE_FREE(service_c);
    goto end;
  }

  msg->auth_request.method = SSH_AUTH_METHOD_UNKNOWN;
  SAFE_FREE(method_c);
  goto end;
error:
  ssh_string_free(user_s);
  ssh_string_free(service);
  ssh_string_free(method);

  SAFE_FREE(method_c);
  SAFE_FREE(service_c);

  ssh_message_free(msg);

  leave_function();
  return SSH_PACKET_USED;
end:
  ssh_message_queue(session,msg);
  leave_function();
  return SSH_PACKET_USED;
}

SSH_PACKET_CALLBACK(ssh_packet_channel_open){
  ssh_message msg = NULL;
  ssh_string type_s = NULL, originator = NULL, destination = NULL;
  char *type_c = NULL;
  uint32_t sender, window, packet_size, originator_port, destination_port;

  enter_function();
  (void)type;
  (void)user;
  msg = ssh_message_new(session);
  if (msg == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  msg->type = SSH_REQUEST_CHANNEL_OPEN;

  type_s = buffer_get_ssh_string(packet);
  if (type_s == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }
  type_c = ssh_string_to_char(type_s);
  if (type_c == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  ssh_log(session, SSH_LOG_PACKET,
      "Clients wants to open a %s channel", type_c);
  ssh_string_free(type_s);
  type_s=NULL;

  buffer_get_u32(packet, &sender);
  buffer_get_u32(packet, &window);
  buffer_get_u32(packet, &packet_size);

  msg->channel_request_open.sender = ntohl(sender);
  msg->channel_request_open.window = ntohl(window);
  msg->channel_request_open.packet_size = ntohl(packet_size);

  if (strcmp(type_c,"session") == 0) {
    msg->channel_request_open.type = SSH_CHANNEL_SESSION;
    SAFE_FREE(type_c);
    goto end;
  }

  if (strcmp(type_c,"direct-tcpip") == 0) {
    destination = buffer_get_ssh_string(packet);
	if (destination == NULL) {
    ssh_set_error_oom(session);
		goto error;
	}
	msg->channel_request_open.destination = ssh_string_to_char(destination);
	if (msg->channel_request_open.destination == NULL) {
    ssh_set_error_oom(session);
	  ssh_string_free(destination);
	  goto error;
	}
    ssh_string_free(destination);

    buffer_get_u32(packet, &destination_port);
    msg->channel_request_open.destination_port = (uint16_t) ntohl(destination_port);

    originator = buffer_get_ssh_string(packet);
	if (originator == NULL) {
    ssh_set_error_oom(session);
	  goto error;
	}
	msg->channel_request_open.originator = ssh_string_to_char(originator);
	if (msg->channel_request_open.originator == NULL) {
    ssh_set_error_oom(session);
	  ssh_string_free(originator);
	  goto error;
	}
    ssh_string_free(originator);

    buffer_get_u32(packet, &originator_port);
    msg->channel_request_open.originator_port = (uint16_t) ntohl(originator_port);

    msg->channel_request_open.type = SSH_CHANNEL_DIRECT_TCPIP;
    goto end;
  }

  if (strcmp(type_c,"forwarded-tcpip") == 0) {
    destination = buffer_get_ssh_string(packet);
	if (destination == NULL) {
    ssh_set_error_oom(session);
		goto error;
	}
	msg->channel_request_open.destination = ssh_string_to_char(destination);
	if (msg->channel_request_open.destination == NULL) {
    ssh_set_error_oom(session);
	  ssh_string_free(destination);
	  goto error;
	}
    ssh_string_free(destination);

    buffer_get_u32(packet, &destination_port);
    msg->channel_request_open.destination_port = (uint16_t) ntohl(destination_port);

    originator = buffer_get_ssh_string(packet);
	if (originator == NULL) {
    ssh_set_error_oom(session);
	  goto error;
	}
	msg->channel_request_open.originator = ssh_string_to_char(originator);
	if (msg->channel_request_open.originator == NULL) {
    ssh_set_error_oom(session);
	  ssh_string_free(originator);
	  goto error;
	}
    ssh_string_free(originator);

    buffer_get_u32(packet, &originator_port);
    msg->channel_request_open.originator_port = (uint16_t) ntohl(originator_port);

    msg->channel_request_open.type = SSH_CHANNEL_FORWARDED_TCPIP;
    goto end;
  }

  if (strcmp(type_c,"x11") == 0) {
    originator = buffer_get_ssh_string(packet);
	if (originator == NULL) {
    ssh_set_error_oom(session);
	  goto error;
	}
	msg->channel_request_open.originator = ssh_string_to_char(originator);
	if (msg->channel_request_open.originator == NULL) {
    ssh_set_error_oom(session);
	  ssh_string_free(originator);
	  goto error;
	}
    ssh_string_free(originator);

    buffer_get_u32(packet, &originator_port);
    msg->channel_request_open.originator_port = (uint16_t) ntohl(originator_port);

    msg->channel_request_open.type = SSH_CHANNEL_X11;
    goto end;
  }

  msg->channel_request_open.type = SSH_CHANNEL_UNKNOWN;
  goto end;

error:
  ssh_message_free(msg);
  msg=NULL;
end:
  if(type_s != NULL)
    ssh_string_free(type_s);
  SAFE_FREE(type_c);
  if(msg != NULL)
    ssh_message_queue(session,msg);
  leave_function();
  return SSH_PACKET_USED;
}

/* TODO: make this function accept a ssh_channel */
ssh_channel ssh_message_channel_request_open_reply_accept(ssh_message msg) {
  ssh_session session = msg->session;
  ssh_channel chan = NULL;

  enter_function();

  if (msg == NULL) {
    leave_function();
    return NULL;
  }

  chan = ssh_channel_new(session);
  if (chan == NULL) {
    leave_function();
    return NULL;
  }

  chan->local_channel = ssh_channel_new_id(session);
  chan->local_maxpacket = 35000;
  chan->local_window = 32000;
  chan->remote_channel = msg->channel_request_open.sender;
  chan->remote_maxpacket = msg->channel_request_open.packet_size;
  chan->remote_window = msg->channel_request_open.window;
  chan->state = SSH_CHANNEL_STATE_OPEN;

  if (buffer_add_u8(session->out_buffer, SSH2_MSG_CHANNEL_OPEN_CONFIRMATION) < 0) {
    goto error;
  }
  if (buffer_add_u32(session->out_buffer, htonl(chan->remote_channel)) < 0) {
    goto error;
  }
  if (buffer_add_u32(session->out_buffer, htonl(chan->local_channel)) < 0) {
    goto error;
  }
  if (buffer_add_u32(session->out_buffer, htonl(chan->local_window)) < 0) {
    goto error;
  }
  if (buffer_add_u32(session->out_buffer, htonl(chan->local_maxpacket)) < 0) {
    goto error;
  }

  ssh_log(session, SSH_LOG_PACKET,
      "Accepting a channel request_open for chan %d", chan->remote_channel);

  if (packet_send(session) == SSH_ERROR) {
    goto error;
  }

  leave_function();
  return chan;
error:
  ssh_channel_free(chan);

  leave_function();
  return NULL;
}

/**
 * @internal
 *
 * @brief This function parses the last end of a channel request packet.
 *
 * This is normally converted to a SSH message and placed in the queue.
 *
 * @param[in]  session  The SSH session.
 *
 * @param[in]  channel  The channel the request is made on.
 *
 * @param[in]  packet   The rest of the packet to be parsed.
 *
 * @param[in]  request  The type of request.
 *
 * @param[in]  want_reply The want_reply field from the request.
 *
 * @returns             SSH_OK on success, SSH_ERROR if an error occured.
 */
int ssh_message_handle_channel_request(ssh_session session, ssh_channel channel, ssh_buffer packet,
    const char *request, uint8_t want_reply) {
  ssh_message msg = NULL;
  enter_function();
  msg = ssh_message_new(session);
  if (msg == NULL) {
    ssh_set_error_oom(session);
    goto error;
  }

  ssh_log(session, SSH_LOG_PACKET,
      "Received a %s channel_request for channel (%d:%d) (want_reply=%hhd)",
      request, channel->local_channel, channel->remote_channel, want_reply);

  msg->type = SSH_REQUEST_CHANNEL;
  msg->channel_request.channel = channel;
  msg->channel_request.want_reply = want_reply;

  if (strcmp(request, "pty-req") == 0) {
    ssh_string term = NULL;
    char *term_c = NULL;
    term = buffer_get_ssh_string(packet);
    if (term == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }
    term_c = ssh_string_to_char(term);
    if (term_c == NULL) {
      ssh_set_error_oom(session);
      ssh_string_free(term);
      goto error;
    }
    ssh_string_free(term);

    msg->channel_request.type = SSH_CHANNEL_REQUEST_PTY;
    msg->channel_request.TERM = term_c;

    buffer_get_u32(packet, &msg->channel_request.width);
    buffer_get_u32(packet, &msg->channel_request.height);
    buffer_get_u32(packet, &msg->channel_request.pxwidth);
    buffer_get_u32(packet, &msg->channel_request.pxheight);

    msg->channel_request.width = ntohl(msg->channel_request.width);
    msg->channel_request.height = ntohl(msg->channel_request.height);
    msg->channel_request.pxwidth = ntohl(msg->channel_request.pxwidth);
    msg->channel_request.pxheight = ntohl(msg->channel_request.pxheight);
    msg->channel_request.modes = buffer_get_ssh_string(packet);
    if (msg->channel_request.modes == NULL) {
      SAFE_FREE(term_c);
      goto error;
    }
    goto end;
  }

  if (strcmp(request, "window-change") == 0) {
    msg->channel_request.type = SSH_CHANNEL_REQUEST_WINDOW_CHANGE;

    buffer_get_u32(packet, &msg->channel_request.width);
    buffer_get_u32(packet, &msg->channel_request.height);
    buffer_get_u32(packet, &msg->channel_request.pxwidth);
    buffer_get_u32(packet, &msg->channel_request.pxheight);

    msg->channel_request.width = ntohl(msg->channel_request.width);
    msg->channel_request.height = ntohl(msg->channel_request.height);
    msg->channel_request.pxwidth = ntohl(msg->channel_request.pxwidth);
    msg->channel_request.pxheight = ntohl(msg->channel_request.pxheight);

    goto end;
  }

  if (strcmp(request, "subsystem") == 0) {
    ssh_string subsys = NULL;
    char *subsys_c = NULL;
    subsys = buffer_get_ssh_string(packet);
    if (subsys == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }
    subsys_c = ssh_string_to_char(subsys);
    if (subsys_c == NULL) {
      ssh_set_error_oom(session);
      ssh_string_free(subsys);
      goto error;
    }
    ssh_string_free(subsys);

    msg->channel_request.type = SSH_CHANNEL_REQUEST_SUBSYSTEM;
    msg->channel_request.subsystem = subsys_c;

    goto end;
  }

  if (strcmp(request, "shell") == 0) {
    msg->channel_request.type = SSH_CHANNEL_REQUEST_SHELL;
    goto end;
  }

  if (strcmp(request, "exec") == 0) {
    ssh_string cmd = NULL;
    cmd = buffer_get_ssh_string(packet);
    if (cmd == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }
    msg->channel_request.type = SSH_CHANNEL_REQUEST_EXEC;
    msg->channel_request.command = ssh_string_to_char(cmd);
    ssh_string_free(cmd);
    if (msg->channel_request.command == NULL) {
      goto error;
    }
    goto end;
  }

  if (strcmp(request, "env") == 0) {
    ssh_string name = NULL;
    ssh_string value = NULL;
    name = buffer_get_ssh_string(packet);
    if (name == NULL) {
      ssh_set_error_oom(session);
      goto error;
    }
    value = buffer_get_ssh_string(packet);
    if (value == NULL) {
      ssh_set_error_oom(session);
      ssh_string_free(name);
      goto error;
    }

    msg->channel_request.type = SSH_CHANNEL_REQUEST_ENV;
    msg->channel_request.var_name = ssh_string_to_char(name);
    msg->channel_request.var_value = ssh_string_to_char(value);
    if (msg->channel_request.var_name == NULL ||
        msg->channel_request.var_value == NULL) {
      ssh_string_free(name);
      ssh_string_free(value);
      goto error;
    }
    ssh_string_free(name);
    ssh_string_free(value);

    goto end;
  }

  msg->channel_request.type = SSH_CHANNEL_UNKNOWN;
end:
  ssh_message_queue(session,msg);
  leave_function();
  return SSH_OK;
error:
  ssh_message_free(msg);

  leave_function();
  return SSH_ERROR;
}

int ssh_message_channel_request_reply_success(ssh_message msg) {
  uint32_t channel;

  if (msg == NULL) {
    return SSH_ERROR;
  }

  if (msg->channel_request.want_reply) {
    channel = msg->channel_request.channel->remote_channel;

    ssh_log(msg->session, SSH_LOG_PACKET,
        "Sending a channel_request success to channel %d", channel);

    if (buffer_add_u8(msg->session->out_buffer, SSH2_MSG_CHANNEL_SUCCESS) < 0) {
      return SSH_ERROR;
    }
    if (buffer_add_u32(msg->session->out_buffer, htonl(channel)) < 0) {
      return SSH_ERROR;
    }

    return packet_send(msg->session);
  }

  ssh_log(msg->session, SSH_LOG_PACKET,
      "The client doesn't want to know the request succeeded");

  return SSH_OK;
}

#ifdef WITH_SERVER
SSH_PACKET_CALLBACK(ssh_packet_global_request){
    ssh_message msg = NULL;
    ssh_string request_s=NULL;
    char *request=NULL;
    ssh_string bind_addr_s=NULL;
    char *bind_addr=NULL;
    uint32_t bind_port;
    uint8_t want_reply;
    int rc = SSH_PACKET_USED;
    (void)user;
    (void)type;
    (void)packet;

    request_s = buffer_get_ssh_string(packet);
    if (request_s != NULL) {
        request = ssh_string_to_char(request_s);
        ssh_string_free(request_s);
    }

    buffer_get_u8(packet, &want_reply);

    ssh_log(session,SSH_LOG_PROTOCOL,"Received SSH_MSG_GLOBAL_REQUEST packet");

    msg = ssh_message_new(session);
    msg->type = SSH_REQUEST_GLOBAL;

    if (request && strcmp(request, "tcpip-forward") == 0) {
        bind_addr_s = buffer_get_ssh_string(packet);
        if (bind_addr_s != NULL) {
            bind_addr = ssh_string_to_char(bind_addr_s);
            ssh_string_free(bind_addr_s);
        }

        buffer_get_u32(packet, &bind_port);
        bind_port = ntohl(bind_port);

        msg->global_request.type = SSH_GLOBAL_REQUEST_TCPIP_FORWARD;
        msg->global_request.want_reply = want_reply;
        msg->global_request.bind_address = bind_addr;
        msg->global_request.bind_port = bind_port;

        ssh_log(session, SSH_LOG_PROTOCOL, "Received SSH_MSG_GLOBAL_REQUEST %s %d %s:%d", request, want_reply, bind_addr, bind_port);

        if(ssh_callbacks_exists(session->common.callbacks, global_request_function)) {
            ssh_log(session, SSH_LOG_PROTOCOL, "Calling callback for SSH_MSG_GLOBAL_REQUEST %s %d %s:%d", request, want_reply, bind_addr, bind_port);
            session->common.callbacks->global_request_function(session, msg, session->common.callbacks->userdata);
        } else {
            ssh_message_reply_default(msg);
        }
    } else if (request && strcmp(request, "cancel-tcpip-forward") == 0) {
        bind_addr_s = buffer_get_ssh_string(packet);
        if (bind_addr_s != NULL) {
            bind_addr = ssh_string_to_char(bind_addr_s);
            ssh_string_free(bind_addr_s);
        }
        buffer_get_u32(packet, &bind_port);
        bind_port = ntohl(bind_port);

        msg->global_request.type = SSH_GLOBAL_REQUEST_CANCEL_TCPIP_FORWARD;
        msg->global_request.want_reply = want_reply;
        msg->global_request.bind_address = bind_addr;
        msg->global_request.bind_port = bind_port;

        ssh_log(session, SSH_LOG_PROTOCOL, "Received SSH_MSG_GLOBAL_REQUEST %s %d %s:%d", request, want_reply, bind_addr, bind_port);

        if(ssh_callbacks_exists(session->common.callbacks, global_request_function)) {
            session->common.callbacks->global_request_function(session, msg, session->common.callbacks->userdata);
        } else {
            ssh_message_reply_default(msg);
        }
    } else {
        ssh_log(session, SSH_LOG_PROTOCOL, "UNKNOWN SSH_MSG_GLOBAL_REQUEST %s %d", request, want_reply);
        rc = SSH_PACKET_NOT_USED;
    }

    SAFE_FREE(msg);
    SAFE_FREE(request);
    SAFE_FREE(bind_addr);

    return rc;
}

#endif /* WITH_SERVER */

/** @} */

/* vim: set ts=4 sw=4 et cindent: */
