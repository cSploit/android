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

#ifndef BIND_H_
#define BIND_H_

#include "libssh/priv.h"
#include "libssh/session.h"

struct ssh_bind_struct {
  struct ssh_common_struct common; /* stuff common to ssh_bind and ssh_session */
  struct ssh_bind_callbacks_struct *bind_callbacks;
  void *bind_callbacks_userdata;

  struct ssh_poll_handle_struct *poll;
  /* options */
  char *wanted_methods[10];
  char *banner;
  char *dsakey;
  char *rsakey;
  char *bindaddr;
  socket_t bindfd;
  unsigned int bindport;
  int blocking;
  int toaccept;
};

struct ssh_poll_handle_struct *ssh_bind_get_poll(struct ssh_bind_struct
    *sshbind);


#endif /* BIND_H_ */
