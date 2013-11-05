/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2010 by Aris Adamantiadis
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

/** functions in that file are wrappers to the newly named functions. All
 * of them are depreciated, but these wrapper will avoid breaking backward
 * compatibility
 */

#include "config.h"

#include <libssh/priv.h>
#include <libssh/server.h>
#include <libssh/buffer.h>

void buffer_free(ssh_buffer buffer){
  ssh_buffer_free(buffer);
}
void *buffer_get(ssh_buffer buffer){
  return ssh_buffer_get_begin(buffer);
}
uint32_t buffer_get_len(ssh_buffer buffer){
  return ssh_buffer_get_len(buffer);
}
ssh_buffer buffer_new(void){
  return ssh_buffer_new();
}

ssh_channel channel_accept_x11(ssh_channel channel, int timeout_ms){
  return ssh_channel_accept_x11(channel, timeout_ms);
}

int channel_change_pty_size(ssh_channel channel,int cols,int rows){
  return ssh_channel_change_pty_size(channel,cols,rows);
}

ssh_channel channel_forward_accept(ssh_session session, int timeout_ms){
  return ssh_forward_accept(session,timeout_ms);
}

int channel_close(ssh_channel channel){
  return ssh_channel_close(channel);
}

int channel_forward_cancel(ssh_session session, const char *address, int port){
  return ssh_forward_cancel(session, address, port);
}

int channel_forward_listen(ssh_session session, const char *address,
    int port, int *bound_port){
  return ssh_forward_listen(session, address, port, bound_port);
}

void channel_free(ssh_channel channel){
  ssh_channel_free(channel);
}

int channel_get_exit_status(ssh_channel channel){
  return ssh_channel_get_exit_status(channel);
}

ssh_session channel_get_session(ssh_channel channel){
  return ssh_channel_get_session(channel);
}

int channel_is_closed(ssh_channel channel){
  return ssh_channel_is_closed(channel);
}

int channel_is_eof(ssh_channel channel){
  return ssh_channel_is_eof(channel);
}

int channel_is_open(ssh_channel channel){
  return ssh_channel_is_open(channel);
}

ssh_channel channel_new(ssh_session session){
  return ssh_channel_new(session);
}

int channel_open_forward(ssh_channel channel, const char *remotehost,
    int remoteport, const char *sourcehost, int localport){
  return ssh_channel_open_forward(channel, remotehost, remoteport,
      sourcehost,localport);
}

int channel_open_session(ssh_channel channel){
  return ssh_channel_open_session(channel);
}

int channel_poll(ssh_channel channel, int is_stderr){
  return ssh_channel_poll(channel, is_stderr);
}

int channel_read(ssh_channel channel, void *dest, uint32_t count, int is_stderr){
  return ssh_channel_read(channel, dest, count, is_stderr);
}

/*
 * This function will completely be depreciated. The old implementation was not
 * renamed.
 * int channel_read_buffer(ssh_channel channel, ssh_buffer buffer, uint32_t count,
 *   int is_stderr);
 */

int channel_read_nonblocking(ssh_channel channel, void *dest, uint32_t count,
    int is_stderr){
  return ssh_channel_read_nonblocking(channel, dest, count, is_stderr);
}

int channel_request_env(ssh_channel channel, const char *name, const char *value){
  return ssh_channel_request_env(channel, name, value);
}

int channel_request_exec(ssh_channel channel, const char *cmd){
  return ssh_channel_request_exec(channel, cmd);
}

int channel_request_pty(ssh_channel channel){
  return ssh_channel_request_pty(channel);
}

int channel_request_pty_size(ssh_channel channel, const char *term,
    int cols, int rows){
  return ssh_channel_request_pty_size(channel, term, cols, rows);
}

int channel_request_shell(ssh_channel channel){
  return ssh_channel_request_shell(channel);
}

int channel_request_send_signal(ssh_channel channel, const char *signum){
  return ssh_channel_request_send_signal(channel, signum);
}

int channel_request_sftp(ssh_channel channel){
  return ssh_channel_request_sftp(channel);
}

int channel_request_subsystem(ssh_channel channel, const char *subsystem){
  return ssh_channel_request_subsystem(channel, subsystem);
}

int channel_request_x11(ssh_channel channel, int single_connection, const char *protocol,
    const char *cookie, int screen_number){
  return ssh_channel_request_x11(channel, single_connection, protocol, cookie,
      screen_number);
}

int channel_send_eof(ssh_channel channel){
  return ssh_channel_send_eof(channel);
}

int channel_select(ssh_channel *readchans, ssh_channel *writechans, ssh_channel *exceptchans, struct
    timeval * timeout){
  return ssh_channel_select(readchans, writechans, exceptchans, timeout);
}

void channel_set_blocking(ssh_channel channel, int blocking){
  ssh_channel_set_blocking(channel, blocking);
}

int channel_write(ssh_channel channel, const void *data, uint32_t len){
  return ssh_channel_write(channel, data, len);
}

/*
 * These functions have to be wrapped around the pki.c functions.

void privatekey_free(ssh_private_key prv);
ssh_private_key privatekey_from_file(ssh_session session, const char *filename,
    int type, const char *passphrase);
void publickey_free(ssh_public_key key);
int ssh_publickey_to_file(ssh_session session, const char *file,
    ssh_string pubkey, int type);
ssh_string publickey_from_file(ssh_session session, const char *filename,
    int *type);
ssh_public_key publickey_from_privatekey(ssh_private_key prv);
ssh_string publickey_to_string(ssh_public_key key);
 *
 */

void string_burn(ssh_string str){
  ssh_string_burn(str);
}

ssh_string string_copy(ssh_string str){
  return ssh_string_copy(str);
}

void *string_data(ssh_string str){
  return ssh_string_data(str);
}

int string_fill(ssh_string str, const void *data, size_t len){
  return ssh_string_fill(str,data,len);
}

void string_free(ssh_string str){
  ssh_string_free(str);
}

ssh_string string_from_char(const char *what){
  return ssh_string_from_char(what);
}

size_t string_len(ssh_string str){
  return ssh_string_len(str);
}

ssh_string string_new(size_t size){
  return ssh_string_new(size);
}

char *string_to_char(ssh_string str){
  return ssh_string_to_char(str);
}

/****************************************************************************
 * SERVER SUPPORT
 ****************************************************************************/

#ifdef WITH_SERVER
int ssh_accept(ssh_session session) {
    return ssh_handle_key_exchange(session);
}

int channel_write_stderr(ssh_channel channel, const void *data, uint32_t len) {
    return ssh_channel_write(channel, data, len);
}

/** @deprecated
 * @brief Interface previously exported by error.
 */
ssh_message ssh_message_retrieve(ssh_session session, uint32_t packettype){
	(void) packettype;
	ssh_set_error(session, SSH_FATAL, "ssh_message_retrieve: obsolete libssh call");
	return NULL;
}

#endif /* WITH_SERVER */
