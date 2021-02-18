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
#ifndef HOST_H
#define HOST_H

#include <stdint.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <time.h>
#include <stdlib.h>

#include "netdefs.h"
#include "sorted_arraylist.h"
#include "control.h"

/// we tried to DNS resolve this host
#define HOST_LOOKUP_DNS 1
/// we tried to NBNS resolve this host
#define HOST_LOOKUP_NBNS 2

struct host {
  uint32_t ip;
  uint8_t mac[ETH_ALEN];
  char *name;
  char lookup_status;
  time_t timeout;
};

extern struct hosts_data {
  struct host **array;
  uint32_t size;
  data_control control;
} hosts;

/** seconds of inactivity to mark an host as disconnected  */
#define HOST_TIMEOUT 60

#define free_host(h) do { if(h->name) free(h->name); free(h); } while(0)

int init_hosts();
struct host *create_host(uint32_t, uint8_t *, char *);
int add_host(struct host *);
int del_host(struct host *);
struct host *get_host(uint32_t);
void on_host_found(uint8_t *, uint32_t , char *, char);
void fini_hosts();

#endif
