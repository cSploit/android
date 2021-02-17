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

#include <pthread.h>
#include <stdint.h>
#include <ares.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include <csploit/logger.h>
#include <csploit/control.h>

#include "event.h"
#include "host.h"
#include "resolver.h"

struct resolver_data resolver_info;

void on_query_end(void *arg, int status, int timeouts, struct hostent *ent) {
  uint32_t ip;
  struct host *h;
  struct event *e;
  
  if(status != ARES_SUCCESS)
    return;
  
  // avoid unuseful names
  if(strstr(ent->h_name, ".in-addr.arpa"))
    return;
  
  e = NULL;
  ip = *((uint32_t *) &arg);
  
  pthread_mutex_lock(&(hosts.control.mutex));
  
  h = get_host(ip);
  
  if(h && !(h->name)) {
    
    e = (struct event *) malloc(sizeof(struct event));
    
    if(!e) {
      print( ERROR, "malloc: %s\n", strerror(errno));
    } else {
      e->type = NEW_NAME;
      e->ip = ip;
    }
    
    h->name = strdup(ent->h_name);
  }
  
  pthread_mutex_unlock(&(hosts.control.mutex));
  
  if(e) add_event(e);
}

void begin_dns_lookup(uint32_t ip) {
  
  pthread_mutex_lock(&(resolver_info.control.mutex));
  
  ares_gethostbyaddr(resolver_info.channel, &ip, 4, AF_INET, on_query_end, ((void *)0) + ip);
  
  pthread_mutex_unlock(&(resolver_info.control.mutex));
  
  pthread_cond_broadcast(&(resolver_info.control.cond));
}

void *resolver(void *arg) {
  int nfds;
  fd_set readers, writers;
  struct timeval tv, *tvp;
  
  pthread_mutex_lock(&(resolver_info.control.mutex));
  while (resolver_info.control.active) {
    pthread_mutex_unlock(&(resolver_info.control.mutex));
    
    FD_ZERO(&readers);
    FD_ZERO(&writers);
    
    nfds = ares_fds(resolver_info.channel, &readers, &writers);
    
    if(!nfds) {
      // c-ares docs say that we should break and exit here,
      // but have 0 queries is an accaptable state, we have
      // just to wait for new ones.
      pthread_cond_wait(&(resolver_info.control.cond), &(resolver_info.control.mutex));
      continue;
    }
    
    tvp = ares_timeout(resolver_info.channel, NULL, &tv);
    
    select(nfds, &readers, &writers, NULL, tvp);
    ares_process(resolver_info.channel, &readers, &writers);
    
    pthread_mutex_lock(&(resolver_info.control.mutex));
  }
  pthread_mutex_unlock(&(resolver_info.control.mutex));
  
  ares_library_cleanup();
  
  return NULL;
}

static int can_use_hosts_file() {
  struct stat st;
  
  return !stat(PATH_HOSTS, &st) && st.st_size < HOSTS_MAXSIZE;
}

int start_resolver() {
  int ret, optmask;
  struct ares_options options;
  
  ret = ares_library_init(ARES_LIB_INIT_ALL);
  
  if(ret) {
    print( ERROR, "ares_library_init: %s\n", ares_strerror(ret));
    return -1;
  }
  
  memset(&options, 0, sizeof(struct ares_options));
  
  options.flags = ARES_FLAG_STAYOPEN;
  optmask = ARES_OPT_FLAGS;
  
  if(!can_use_hosts_file()) {
    optmask |= ARES_OPT_LOOKUPS;
    options.lookups = "b";
  }
  
  ret = ares_init_options(&(resolver_info.channel), &options, optmask);
  
  if(ret) {
    print( ERROR, "ares_init_options: %s\n", ares_strerror(ret));
    ares_library_cleanup();
    return -1;
  }
  
  if(pthread_create(&(resolver_info.tid), NULL, resolver, NULL)) {
    print( ERROR, "pthread_create: %s\n", strerror(errno));
    resolver_info.tid = 0;
    ares_destroy(resolver_info.channel);
    ares_library_cleanup();
    return -1;
  }
  
  return 0;
}

void stop_resolver() {
  pthread_t tid;
  
  pthread_mutex_lock(&(resolver_info.control.mutex));
  tid = resolver_info.tid;
  resolver_info.tid = 0;
  resolver_info.control.active = 0;
  pthread_mutex_unlock(&(resolver_info.control.mutex));
  
  if(!tid) return;
  
  pthread_cond_broadcast(&(resolver_info.control.cond));
  
  ares_destroy(resolver_info.channel);
  pthread_join(tid, NULL);
}
