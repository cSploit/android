/*
 * This file is part of the SSH Library
 *
 * Copyright (c) 2009 by Aris Adamantiadis
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

#ifndef MISC_H_
#define MISC_H_

/* in misc.c */
/* gets the user home dir. */
char *ssh_get_user_home_dir(void);
char *ssh_get_local_username(ssh_session session);
int ssh_file_readaccess_ok(const char *file);

char *ssh_path_expand_tilde(const char *d);
char *ssh_path_expand_escape(ssh_session session, const char *s);
int ssh_analyze_banner(ssh_session session, int server, int *ssh1, int *ssh2);
int ssh_is_ipaddr_v4(const char *str);
int ssh_is_ipaddr(const char *str);

/* macro for byte ordering */
uint64_t ntohll(uint64_t);
#define htonll(x) ntohll(x)

/* list processing */

struct ssh_list {
  struct ssh_iterator *root;
  struct ssh_iterator *end;
};

struct ssh_iterator {
  struct ssh_iterator *next;
  const void *data;
};

struct ssh_timestamp {
  long seconds;
  long useconds;
};

struct ssh_list *ssh_list_new(void);
void ssh_list_free(struct ssh_list *list);
struct ssh_iterator *ssh_list_get_iterator(const struct ssh_list *list);
struct ssh_iterator *ssh_list_find(const struct ssh_list *list, void *value);
int ssh_list_append(struct ssh_list *list, const void *data);
int ssh_list_prepend(struct ssh_list *list, const void *data);
void ssh_list_remove(struct ssh_list *list, struct ssh_iterator *iterator);
char *ssh_lowercase(const char* str);
char *ssh_hostport(const char *host, int port);

const void *_ssh_list_pop_head(struct ssh_list *list);

#define ssh_iterator_value(type, iterator)\
  ((type)((iterator)->data))

/** @brief fetch the head element of a list and remove it from list
 * @param type type of the element to return
 * @param list the ssh_list to use
 * @return the first element of the list, or NULL if the list is empty
 */
#define ssh_list_pop_head(type, ssh_list)\
  ((type)_ssh_list_pop_head(ssh_list))

int ssh_make_milliseconds(long sec, long usec);
void ssh_timestamp_init(struct ssh_timestamp *ts);
int ssh_timeout_elapsed(struct ssh_timestamp *ts, int timeout);
int ssh_timeout_update(struct ssh_timestamp *ts, int timeout);

#endif /* MISC_H_ */
