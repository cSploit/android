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

#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "logger.h"

#include "ifinfo.h"
#include "host.h"
#include "netdefs.h"
#include "event.h"
#include "prober.h"
#include "resolver.h"
#include "sorted_arraylist.h"

struct hosts_data hosts;

/**
 * @brief initalize host data
 * @returns 0 on success, -1 on error.
 */
int init_hosts() {
  
  hosts.array = NULL;
  hosts.size = 0;
  
  return 0;
}

struct host *get_host(uint32_t ip) {
  
  if(!(hosts.array)) return NULL;
  
  return (struct host *) sortedarray_get((array *) &hosts, ip);
}

struct host *create_host(uint32_t ip, uint8_t *mac, char *name) {
  struct host *h;
  
  h = malloc(sizeof(struct host));
  
  if(!h) {
    print( ERROR, "malloc: %s", strerror(errno));
    return NULL;
  }
  
  memset(h, 0, sizeof(struct host));
  
  h->ip = ip;
  memcpy(h->mac, mac, ETH_ALEN);
  h->name = name;
  
  return h;
}

/**
 * @brief add an host to the host array
 * @param h the host to add
 * @returns 0 on success, -1 on error.
 * 
 * do not add an already existing host!
 */
inline int add_host(struct host *h) {
  return sortedarray_ins((array *) &hosts, (comparable_item *) h);
}

/**
 * @brief delete an host from the host array
 * @param h the host to delete
 * @returns 0 on success, -1 on error.
 */
inline int del_host(struct host *h) {
  return sortedarray_del((array *) &hosts, (comparable_item *) h);
}

/**
 * @brief default callback to call whence an host has been found.
 * @param mac host mac address
 * @param ip host ip address
 * @param name hostname
 * @param lstatus lookup status
 */
void on_host_found(uint8_t *mac, uint32_t ip, char *name, char lstatus) {
  struct host *h;
  struct event *e;
  int old_errno;
  uint8_t e_type;
  char prev_lstatus, host_has_name;
  
  e_type = NONE;
  
  pthread_mutex_lock(&(hosts.control.mutex));
  
  h = get_host(ip);
  
  if(h) {
    
    if(name) {
      if(h->name) {
        e_type = NAME_CHANGED;
        free(h->name);
      } else {
        e_type = NEW_NAME;
      }
      h->name = name;
    }
    
    if(memcmp(mac, h->mac, ETH_ALEN)) {
      memcpy(h->mac, mac, ETH_ALEN);
      
      e_type = MAC_CHANGED;
    }
    
  } else {
    h = create_host(ip, mac, name);
    
    if(!h) {
      old_errno = errno;
      pthread_mutex_unlock(&(hosts.control.mutex));
      print(ERROR, "malloc: %s", strerror(old_errno));
      return;
    }
    
    if(sortedarray_ins((array *) &hosts, (comparable_item *) h)) {
      pthread_mutex_unlock(&(hosts.control.mutex));
      free_host(h);
      return;
    }
    
    e_type = (name ? NEW_NAME : NEW_MAC);
  }
  
  h->timeout = time(NULL) + HOST_TIMEOUT;
  
  prev_lstatus = h->lookup_status | lstatus;
  h->lookup_status |= (HOST_LOOKUP_DNS|HOST_LOOKUP_NBNS);
  
  host_has_name = h->name != NULL;
  
  pthread_mutex_unlock(&(hosts.control.mutex));
  
  if(!(prev_lstatus & HOST_LOOKUP_DNS) && !host_has_name) {
    begin_dns_lookup(ip);
  }
  
  if(!(prev_lstatus & HOST_LOOKUP_NBNS)) {
    begin_nbns_lookup(ip);
  }
  
  if(e_type == NONE)
    return;
  
  e = malloc(sizeof(struct event));
  
  if(!e) {
    print( ERROR, "malloc: %s", strerror(errno));
    return;
  }
  
  e->ip = ip;
  e->type = e_type;
  
  add_event(e);
}

void fini_hosts() {
  register uint32_t i;
  
  pthread_mutex_lock(&(hosts.control.mutex));
  
  for(i=0;i<hosts.size;i++) {
    free_host(hosts.array[i]);
  }
  
  free(hosts.array);
  
  hosts.array = NULL;
  hosts.size = 0;
  
  pthread_mutex_unlock(&(hosts.control.mutex));
}
