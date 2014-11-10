/* LICENSE
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