/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 * 
 */
#ifndef AUTHENTICATOR_H
#define AUTHENTICATOR_H

#include "list.h"

typedef struct login {
  node *next;
  char *user;       ///< username
  char *pswd_hash;  ///< password hash
  size_t user_len;  ///< username length
  size_t hash_len;  ///< password hash len ( you can use any alghorithm, we just check the hash )
} login;

int load_users(void);
void unload_users(void);
int on_auth_request(conn_node *, message *);
int on_unathorized_request(conn_node *, message *);

extern list users;

#endif