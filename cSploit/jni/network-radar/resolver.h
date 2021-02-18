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
 */

#ifndef RESOLVER_H
#define RESOLVER_H

#include <pthread.h>
#include <stdint.h>
#include <ares.h>

#include "control.h"

/// maximum size of the hosts file ( in bytes )
#define HOSTS_MAXSIZE           ( 128 * 1024 )

/// path to the hosts file
#define PATH_HOSTS              "/etc/hosts"

void begin_dns_lookup(uint32_t);
int start_resolver(void);
void stop_resolver(void);

extern struct resolver_data {
  data_control control;
  pthread_t tid;
  ares_channel channel;
} resolver_info;

#endif
