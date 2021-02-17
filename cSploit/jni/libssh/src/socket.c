/*
 * socket.c - socket functions for the library
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2008-2010      by Aris Adamantiadis
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

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#if _MSC_VER >= 1400
#include <io.h>
#undef open
#define open _open
#undef close
#define close _close
#undef read
#define read _read
#undef write
#define write _write
#endif /* _MSC_VER */
#else /* _WIN32 */
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#endif /* _WIN32 */

#include "libssh/priv.h"
#include "libssh/callbacks.h"
#include "libssh/socket.h"
#include "libssh/buffer.h"
#include "libssh/poll.h"
#include "libssh/session.h"

/**
 * @internal
 *
 * @defgroup libssh_socket The SSH socket functions.
 * @ingroup libssh
 *
 * Functions for handling sockets.
 *
 * @{
 */

enum ssh_socket_states_e {
	SSH_SOCKET_NONE,
	SSH_SOCKET_CONNECTING,
	SSH_SOCKET_CONNECTED,
	SSH_SOCKET_EOF,
	SSH_SOCKET_ERROR,
	SSH_SOCKET_CLOSED
};

struct ssh_socket_struct {
  socket_t fd_in;
  socket_t fd_out;
  int fd_is_socket;
  int last_errno;
  int read_wontblock; /* reading now on socket will
                       not block */
  int write_wontblock;
  int data_except;
  enum ssh_socket_states_e state;
  ssh_buffer out_buffer;
  ssh_buffer in_buffer;
  ssh_session session;
  ssh_socket_callbacks callbacks;
  ssh_poll_handle poll_in;
  ssh_poll_handle poll_out;
};

static int sockets_initialized = 0;

static int ssh_socket_unbuffered_read(ssh_socket s, void *buffer, uint32_t len);
static int ssh_socket_unbuffered_write(ssh_socket s, const void *buffer,
		uint32_t len);

/**
 * \internal
 * \brief inits the socket system (windows specific)
 */
int ssh_socket_init(void) {
  if (sockets_initialized == 0) {
#ifdef _WIN32
    struct WSAData wsaData;

    /* Initiates use of the Winsock DLL by a process. */
    if (WSAStartup(MAKEWORD(2, 0), &wsaData) != 0) {
      return -1;
    }

#endif
    ssh_poll_init();

    sockets_initialized = 1;
  }

  return 0;
}

/**
 * @brief Cleanup the socket system.
 */
void ssh_socket_cleanup(void) {
  if (sockets_initialized == 1) {
    ssh_poll_cleanup();
#ifdef _WIN32
    WSACleanup();
#endif
    sockets_initialized = 0;
  }
}


/**
 * \internal
 * \brief creates a new Socket object
 */
ssh_socket ssh_socket_new(ssh_session session) {
  ssh_socket s;

  s = malloc(sizeof(struct ssh_socket_struct));
  if (s == NULL) {
    ssh_set_error_oom(session);
    return NULL;
  }
  s->fd_in = SSH_INVALID_SOCKET;
  s->fd_out= SSH_INVALID_SOCKET;
  s->last_errno = -1;
  s->fd_is_socket = 1;
  s->session = session;
  s->in_buffer = ssh_buffer_new();
  if (s->in_buffer == NULL) {
    ssh_set_error_oom(session);
    SAFE_FREE(s);
    return NULL;
  }
  s->out_buffer=ssh_buffer_new();
  if (s->out_buffer == NULL) {
    ssh_set_error_oom(session);
    ssh_buffer_free(s->in_buffer);
    SAFE_FREE(s);
    return NULL;
  }
  s->read_wontblock = 0;
  s->write_wontblock = 0;
  s->data_except = 0;
  s->poll_in=s->poll_out=NULL;
  s->state=SSH_SOCKET_NONE;
  return s;
}

/**
 * @internal
 * @brief Reset the state of a socket so it looks brand-new
 * @param[in] s socket to rest
 */
void ssh_socket_reset(ssh_socket s){
  s->fd_in = SSH_INVALID_SOCKET;
  s->fd_out= SSH_INVALID_SOCKET;
  s->last_errno = -1;
  s->fd_is_socket = 1;
  buffer_reinit(s->in_buffer);
  buffer_reinit(s->out_buffer);
  s->read_wontblock = 0;
  s->write_wontblock = 0;
  s->data_except = 0;
  s->poll_in=s->poll_out=NULL;
  s->state=SSH_SOCKET_NONE;
}

/**
 * @internal
 * @brief the socket callbacks, i.e. callbacks to be called
 * upon a socket event.
 * @param s socket to set callbacks on.
 * @param callbacks a ssh_socket_callback object reference.
 */

void ssh_socket_set_callbacks(ssh_socket s, ssh_socket_callbacks callbacks){
	s->callbacks=callbacks;
}

/**
 * @brief 							SSH poll callback. This callback will be used when an event
 *                      caught on the socket.
 *
 * @param p             Poll object this callback belongs to.
 * @param fd            The raw socket.
 * @param revents       The current poll events on the socket.
 * @param userdata      Userdata to be passed to the callback function,
 *                      in this case the socket object.
 *
 * @return              0 on success, < 0 when the poll object has been removed
 *                      from its poll context.
 */
int ssh_socket_pollcallback(struct ssh_poll_handle_struct *p, socket_t fd, int revents, void *v_s){
	ssh_socket s=(ssh_socket )v_s;
	char buffer[MAX_BUF_SIZE];
	int r;
	int err=0;
	socklen_t errlen=sizeof(err);
	/* Do not do anything if this socket was already closed */
	if(!ssh_socket_is_open(s)){
	  return -1;
	}
	if(revents & POLLERR || revents & POLLHUP){
		/* Check if we are in a connecting state */
		if(s->state==SSH_SOCKET_CONNECTING){
			s->state=SSH_SOCKET_ERROR;
			r = getsockopt(fd, SOL_SOCKET, SO_ERROR, (char *)&err, &errlen);
            if (r < 0) {
                err = errno;
            }
			s->last_errno=err;
			ssh_socket_close(s);
			if(s->callbacks && s->callbacks->connected)
				s->callbacks->connected(SSH_SOCKET_CONNECTED_ERROR,err,
						s->callbacks->userdata);
			return -1;
		}
		/* Then we are in a more standard kind of error */
		/* force a read to get an explanation */
		revents |= POLLIN;
	}
	if((revents & POLLIN) && s->state == SSH_SOCKET_CONNECTED){
		s->read_wontblock=1;
		r=ssh_socket_unbuffered_read(s,buffer,sizeof(buffer));
		if(r<0){
		if(p != NULL) {
			ssh_poll_remove_events(p, POLLIN);
		}
			if(s->callbacks && s->callbacks->exception){
				s->callbacks->exception(
						SSH_SOCKET_EXCEPTION_ERROR,
						s->last_errno,s->callbacks->userdata);
				/* p may have been freed, so don't use it
				* anymore in this function */
				p = NULL;
				return -2;
			}
		}
		if(r==0){
			if(p != NULL) {
				ssh_poll_remove_events(p, POLLIN);
			}
			if(p != NULL) {
				ssh_poll_remove_events(p, POLLIN);
			}
			if(s->callbacks && s->callbacks->exception){
				s->callbacks->exception(
						SSH_SOCKET_EXCEPTION_EOF,
						0,s->callbacks->userdata);
				/* p may have been freed, so don't use it
				* anymore in this function */
				p = NULL;
				return -2;
			}
		}
		if(r>0){
			/* Bufferize the data and then call the callback */
            r = buffer_add_data(s->in_buffer,buffer,r);
            if (r < 0) {
                return -1;
            }
			if(s->callbacks && s->callbacks->data){
				do {
					r= s->callbacks->data(buffer_get_rest(s->in_buffer),
							buffer_get_rest_len(s->in_buffer),
							s->callbacks->userdata);
					buffer_pass_bytes(s->in_buffer,r);
				} while ((r > 0) && (s->state == SSH_SOCKET_CONNECTED));
				/* p may have been freed, so don't use it
				* anymore in this function */
				p = NULL;
			}
		}
	}
#ifdef _WIN32
	if(revents & POLLOUT || revents & POLLWRNORM){
#else
	if(revents & POLLOUT){
#endif
		/* First, POLLOUT is a sign we may be connected */
		if(s->state == SSH_SOCKET_CONNECTING){
			SSH_LOG(SSH_LOG_PACKET,"Received POLLOUT in connecting state");
			s->state = SSH_SOCKET_CONNECTED;
			if (p != NULL) {
				ssh_poll_set_events(p,POLLOUT | POLLIN);
			}
			r = ssh_socket_set_blocking(ssh_socket_get_fd_in(s));
			if (r < 0) {
				return -1;
			}
			if(s->callbacks && s->callbacks->connected)
				s->callbacks->connected(SSH_SOCKET_CONNECTED_OK,0,s->callbacks->userdata);
			return 0;
		}
		/* So, we can write data */
		s->write_wontblock=1;
        if(p != NULL) {
            ssh_poll_remove_events(p, POLLOUT);
        }

		/* If buffered data is pending, write it */
		if(buffer_get_rest_len(s->out_buffer) > 0){
		  ssh_socket_nonblocking_flush(s);
		} else if(s->callbacks && s->callbacks->controlflow){
			/* Otherwise advertise the upper level that write can be done */
			s->callbacks->controlflow(SSH_SOCKET_FLOW_WRITEWONTBLOCK,s->callbacks->userdata);
		}
			/* TODO: Find a way to put back POLLOUT when buffering occurs */
	}
	/* Return -1 if one of the poll handlers disappeared */
	return (s->poll_in == NULL || s->poll_out == NULL) ? -1 : 0;
}

/** @internal
 * @brief returns the input poll handle corresponding to the socket,
 * creates it if it does not exist.
 * @returns allocated and initialized ssh_poll_handle object
 */
ssh_poll_handle ssh_socket_get_poll_handle_in(ssh_socket s){
	if(s->poll_in)
		return s->poll_in;
	s->poll_in=ssh_poll_new(s->fd_in,0,ssh_socket_pollcallback,s);
	if(s->fd_in == s->fd_out && s->poll_out == NULL)
    s->poll_out=s->poll_in;
	return s->poll_in;
}

/** @internal
 * @brief returns the output poll handle corresponding to the socket,
 * creates it if it does not exist.
 * @returns allocated and initialized ssh_poll_handle object
 */
ssh_poll_handle ssh_socket_get_poll_handle_out(ssh_socket s){
  if(s->poll_out)
    return s->poll_out;
  s->poll_out=ssh_poll_new(s->fd_out,0,ssh_socket_pollcallback,s);
  if(s->fd_in == s->fd_out && s->poll_in == NULL)
    s->poll_in=s->poll_out;
  return s->poll_out;
}

/** \internal
 * \brief Deletes a socket object
 */
void ssh_socket_free(ssh_socket s){
  if (s == NULL) {
    return;
  }
  ssh_socket_close(s);
  ssh_buffer_free(s->in_buffer);
  ssh_buffer_free(s->out_buffer);
  SAFE_FREE(s);
}

#ifndef _WIN32
int ssh_socket_unix(ssh_socket s, const char *path) {
  struct sockaddr_un sunaddr;
  socket_t fd;
  sunaddr.sun_family = AF_UNIX;
  snprintf(sunaddr.sun_path, sizeof(sunaddr.sun_path), "%s", path);

  fd = socket(AF_UNIX, SOCK_STREAM, 0);
  if (fd == SSH_INVALID_SOCKET) {
    ssh_set_error(s->session, SSH_FATAL,
		    "Error from socket(AF_UNIX, SOCK_STREAM, 0): %s",
		    strerror(errno));
    return -1;
  }

  if (fcntl(fd, F_SETFD, 1) == -1) {
    ssh_set_error(s->session, SSH_FATAL,
		    "Error from fcntl(fd, F_SETFD, 1): %s",
		    strerror(errno));
    close(fd);
    return -1;
  }

  if (connect(fd, (struct sockaddr *) &sunaddr,
        sizeof(sunaddr)) < 0) {
    ssh_set_error(s->session, SSH_FATAL, "Error from connect(): %s",
		    strerror(errno));
    close(fd);
    return -1;
  }
  ssh_socket_set_fd(s,fd);
  return 0;
}
#endif

/** \internal
 * \brief closes a socket
 */
void ssh_socket_close(ssh_socket s){
  if (ssh_socket_is_open(s)) {
#ifdef _WIN32
    closesocket(s->fd_in);
    /* fd_in = fd_out under win32 */
    s->last_errno = WSAGetLastError();
#else
    close(s->fd_in);
    if(s->fd_out != s->fd_in && s->fd_out != -1)
      close(s->fd_out);
    s->last_errno = errno;
#endif
    s->fd_in = s->fd_out = SSH_INVALID_SOCKET;
  }
  if(s->poll_in != NULL){
    if(s->poll_out == s->poll_in)
      s->poll_out = NULL;
    ssh_poll_free(s->poll_in);
    s->poll_in=NULL;
  }
  if(s->poll_out != NULL){
    ssh_poll_free(s->poll_out);
    s->poll_out=NULL;
  }

  s->state = SSH_SOCKET_CLOSED;
}

/**
 * @internal
 * @brief sets the file descriptor of the socket.
 * @param[out] s ssh_socket to update
 * @param[in] fd file descriptor to set
 * @warning this function updates boths the input and output
 * file descriptors
 */
void ssh_socket_set_fd(ssh_socket s, socket_t fd) {
    s->fd_in = s->fd_out = fd;

    if (s->poll_in) {
        ssh_poll_set_fd(s->poll_in,fd);
    } else {
        s->state = SSH_SOCKET_CONNECTING;

        /* POLLOUT is the event to wait for in a nonblocking connect */
        ssh_poll_set_events(ssh_socket_get_poll_handle_in(s), POLLOUT);
#ifdef _WIN32
        ssh_poll_add_events(ssh_socket_get_poll_handle_in(s), POLLWRNORM);
#endif
    }
}

/**
 * @internal
 * @brief sets the input file descriptor of the socket.
 * @param[out] s ssh_socket to update
 * @param[in] fd file descriptor to set
 */
void ssh_socket_set_fd_in(ssh_socket s, socket_t fd) {
  s->fd_in = fd;
  if(s->poll_in)
    ssh_poll_set_fd(s->poll_in,fd);
}

/**
 * @internal
 * @brief sets the output file descriptor of the socket.
 * @param[out] s ssh_socket to update
 * @param[in] fd file descriptor to set
 */
void ssh_socket_set_fd_out(ssh_socket s, socket_t fd) {
  s->fd_out = fd;
  if(s->poll_out)
    ssh_poll_set_fd(s->poll_out,fd);
}



/** \internal
 * \brief returns the input file descriptor of the socket
 */
socket_t ssh_socket_get_fd_in(ssh_socket s) {
  return s->fd_in;
}

/** \internal
 * \brief returns nonzero if the socket is open
 */
int ssh_socket_is_open(ssh_socket s) {
  return s->fd_in != SSH_INVALID_SOCKET;
}

/** \internal
 * \brief read len bytes from socket into buffer
 */
static int ssh_socket_unbuffered_read(ssh_socket s, void *buffer, uint32_t len) {
  int rc = -1;

  if (s->data_except) {
    return -1;
  }
  if(s->fd_is_socket)
    rc = recv(s->fd_in,buffer, len, 0);
  else
    rc = read(s->fd_in,buffer, len);
#ifdef _WIN32
  s->last_errno = WSAGetLastError();
#else
  s->last_errno = errno;
#endif
  s->read_wontblock = 0;

  if (rc < 0) {
    s->data_except = 1;
  }

  return rc;
}

/** \internal
 * \brief writes len bytes from buffer to socket
 */
static int ssh_socket_unbuffered_write(ssh_socket s, const void *buffer,
    uint32_t len) {
  int w = -1;

  if (s->data_except) {
    return -1;
  }
  if (s->fd_is_socket)
    w = send(s->fd_out,buffer, len, 0);
  else
    w = write(s->fd_out, buffer, len);
#ifdef _WIN32
  s->last_errno = WSAGetLastError();
#else
  s->last_errno = errno;
#endif
  s->write_wontblock = 0;
  /* Reactive the POLLOUT detector in the poll multiplexer system */
  if(s->poll_out){
      SSH_LOG(SSH_LOG_PACKET, "Enabling POLLOUT for socket");
  	ssh_poll_set_events(s->poll_out,ssh_poll_get_events(s->poll_out) | POLLOUT);
  }
  if (w < 0) {
    s->data_except = 1;
  }

  return w;
}

/** \internal
 * \brief returns nonzero if the current socket is in the fd_set
 */
int ssh_socket_fd_isset(ssh_socket s, fd_set *set) {
  if(s->fd_in == SSH_INVALID_SOCKET) {
    return 0;
  }
  return FD_ISSET(s->fd_in,set) || FD_ISSET(s->fd_out,set);
}

/** \internal
 * \brief sets the current fd in a fd_set and updates the max_fd
 */

void ssh_socket_fd_set(ssh_socket s, fd_set *set, socket_t *max_fd) {
  if (s->fd_in == SSH_INVALID_SOCKET) {
    return;
  }

  FD_SET(s->fd_in,set);
  FD_SET(s->fd_out,set);

  if (s->fd_in >= 0 &&
      s->fd_in >= *max_fd &&
      s->fd_in != SSH_INVALID_SOCKET) {
      *max_fd = s->fd_in + 1;
  }
  if (s->fd_out >= 0 &&
      s->fd_out >= *max_fd &&
      s->fd_out != SSH_INVALID_SOCKET) {
      *max_fd = s->fd_out + 1;
  }
}

/** \internal
 * \brief buffered write of data
 * \returns SSH_OK, or SSH_ERROR
 * \warning has no effect on socket before a flush
 */
int ssh_socket_write(ssh_socket s, const void *buffer, int len) {
  if(len > 0) {
    if (buffer_add_data(s->out_buffer, buffer, len) < 0) {
      ssh_set_error_oom(s->session);
      return SSH_ERROR;
    }
    ssh_socket_nonblocking_flush(s);
  }

  return SSH_OK;
}


/** \internal
 * \brief starts a nonblocking flush of the output buffer
 *
 */
int ssh_socket_nonblocking_flush(ssh_socket s) {
  ssh_session session = s->session;
  uint32_t len;
  int w;

  if (!ssh_socket_is_open(s)) {
    session->alive = 0;
    /* FIXME use ssh_socket_get_errno */
    ssh_set_error(session, SSH_FATAL,
        "Writing packet: error on socket (or connection closed): %s",
        strerror(s->last_errno));

    return SSH_ERROR;
  }

  len = buffer_get_rest_len(s->out_buffer);
  if (!s->write_wontblock && s->poll_out && len > 0) {
      /* force the poll system to catch pollout events */
      ssh_poll_add_events(s->poll_out, POLLOUT);

      return SSH_AGAIN;
  }
  if (s->write_wontblock && len > 0) {
    w = ssh_socket_unbuffered_write(s, buffer_get_rest(s->out_buffer), len);
    if (w < 0) {
      session->alive = 0;
      ssh_socket_close(s);
      /* FIXME use ssh_socket_get_errno() */
      /* FIXME use callback for errors */
      ssh_set_error(session, SSH_FATAL,
          "Writing packet: error on socket (or connection closed): %s",
          strerror(s->last_errno));

      return SSH_ERROR;
    }
    buffer_pass_bytes(s->out_buffer, w);
  }

  /* Is there some data pending? */
  len = buffer_get_rest_len(s->out_buffer);
  if (s->poll_out && len > 0) {
      /* force the poll system to catch pollout events */
      ssh_poll_add_events(s->poll_out, POLLOUT);

      return SSH_AGAIN;
  }

  /* all data written */
  return SSH_OK;
}

void ssh_socket_set_write_wontblock(ssh_socket s) {
  s->write_wontblock = 1;
}

void ssh_socket_set_read_wontblock(ssh_socket s) {
  s->read_wontblock = 1;
}

void ssh_socket_set_except(ssh_socket s) {
  s->data_except = 1;
}

int ssh_socket_data_available(ssh_socket s) {
  return s->read_wontblock;
}

int ssh_socket_data_writable(ssh_socket s) {
  return s->write_wontblock;
}

/** @internal
 * @brief returns the number of outgoing bytes currently buffered
 * @param s the socket
 * @returns numbers of bytes buffered, or 0 if the socket isn't connected
 */
int ssh_socket_buffered_write_bytes(ssh_socket s){
	if(s==NULL || s->out_buffer == NULL)
		return 0;
	return buffer_get_rest_len(s->out_buffer);
}


int ssh_socket_get_status(ssh_socket s) {
  int r = 0;

  if (buffer_get_len(s->in_buffer) > 0) {
      r |= SSH_READ_PENDING;
  }

  if (buffer_get_len(s->out_buffer) > 0) {
      r |= SSH_WRITE_PENDING;
  }

  if (s->data_except) {
    r |= SSH_CLOSED_ERROR;
  }

  return r;
}

int ssh_socket_get_poll_flags(ssh_socket s) {
  int r = 0;
  if (s->poll_in != NULL && (ssh_poll_get_events (s->poll_in) & POLLIN) > 0) {
    r |= SSH_READ_PENDING;
  }
  if (s->poll_out != NULL && (ssh_poll_get_events (s->poll_out) & POLLOUT) > 0) {
    r |= SSH_WRITE_PENDING;
  }
  return r;
}

#ifdef _WIN32
int ssh_socket_set_nonblocking(socket_t fd) {
  u_long nonblocking = 1;
  return ioctlsocket(fd, FIONBIO, &nonblocking);
}

int ssh_socket_set_blocking(socket_t fd) {
  u_long nonblocking = 0;
  return ioctlsocket(fd, FIONBIO, &nonblocking);
}

#else /* _WIN32 */
int ssh_socket_set_nonblocking(socket_t fd) {
  return fcntl(fd, F_SETFL, O_NONBLOCK);
}

int ssh_socket_set_blocking(socket_t fd) {
  return fcntl(fd, F_SETFL, 0);
}
#endif /* _WIN32 */

/**
 * @internal
 * @brief Launches a socket connection
 * If a the socket connected callback has been defined and
 * a poll object exists, this call will be non blocking.
 * @param s    socket to connect.
 * @param host hostname or ip address to connect to.
 * @param port port number to connect to.
 * @param bind_addr address to bind to, or NULL for default.
 * @returns SSH_OK socket is being connected.
 * @returns SSH_ERROR error while connecting to remote host.
 * @bug It only tries connecting to one of the available AI's
 * which is problematic for hosts having DNS fail-over.
 */

int ssh_socket_connect(ssh_socket s, const char *host, int port, const char *bind_addr){
	socket_t fd;

	if(s->state != SSH_SOCKET_NONE) {
		ssh_set_error(s->session, SSH_FATAL,
				"ssh_socket_connect called on socket not unconnected");
		return SSH_ERROR;
	}
	fd=ssh_connect_host_nonblocking(s->session,host,bind_addr,port);
	SSH_LOG(SSH_LOG_PROTOCOL,"Nonblocking connection socket: %d",fd);
	if(fd == SSH_INVALID_SOCKET)
		return SSH_ERROR;
	ssh_socket_set_fd(s,fd);

	return SSH_OK;
}

#ifndef _WIN32
/**
 * @internal
 * @brief executes a command and redirect input and outputs
 * @param command command to execute
 * @param in input file descriptor
 * @param out output file descriptor
 */
void ssh_execute_command(const char *command, socket_t in, socket_t out){
  const char *args[]={"/bin/sh","-c",command,NULL};
  /* redirect in and out to stdin, stdout and stderr */
  dup2(in, 0);
  dup2(out,1);
  dup2(out,2);
  close(in);
  close(out);
  execv(args[0],(char * const *)args);
  exit(1);
}

/**
 * @internal
 * @brief Open a socket on a ProxyCommand
 * This call will always be nonblocking.
 * @param s    socket to connect.
 * @param command Command to execute.
 * @returns SSH_OK socket is being connected.
 * @returns SSH_ERROR error while executing the command.
 */

int ssh_socket_connect_proxycommand(ssh_socket s, const char *command){
  socket_t in_pipe[2];
  socket_t out_pipe[2];
  int pid;
  int rc;

  if(s->state != SSH_SOCKET_NONE)
    return SSH_ERROR;

  rc = pipe(in_pipe);
  if (rc < 0) {
      return SSH_ERROR;
  }
  rc = pipe(out_pipe);
  if (rc < 0) {
      return SSH_ERROR;
  }

  SSH_LOG(SSH_LOG_PROTOCOL,"Executing proxycommand '%s'",command);
  pid = fork();
  if(pid == 0){
    ssh_execute_command(command,out_pipe[0],in_pipe[1]);
  }
  close(in_pipe[1]);
  close(out_pipe[0]);
  SSH_LOG(SSH_LOG_PROTOCOL,"ProxyCommand connection pipe: [%d,%d]",in_pipe[0],out_pipe[1]);
  ssh_socket_set_fd_in(s,in_pipe[0]);
  ssh_socket_set_fd_out(s,out_pipe[1]);
  s->state=SSH_SOCKET_CONNECTED;
  s->fd_is_socket=0;
  /* POLLOUT is the event to wait for in a nonblocking connect */
  ssh_poll_set_events(ssh_socket_get_poll_handle_in(s),POLLIN);
  ssh_poll_set_events(ssh_socket_get_poll_handle_out(s),POLLOUT);
  if(s->callbacks && s->callbacks->connected)
    s->callbacks->connected(SSH_SOCKET_CONNECTED_OK,0,s->callbacks->userdata);

  return SSH_OK;
}

#endif /* _WIN32 */
/** @} */

/* vim: set ts=4 sw=4 et cindent: */
