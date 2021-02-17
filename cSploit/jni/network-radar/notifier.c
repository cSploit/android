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
#include <arpa/inet.h>
#include <stdint.h>
#include <pthread.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <csploit/logger.h>

#include "event.h"
#include "host.h"

pthread_t notifier_tid = 0;

#define MAC_ADDRESS_STRLEN 17

static void print_host_event( char etype, uint32_t ip) {
	char mac_str[MAC_ADDRESS_STRLEN + 1],
       ip_str[INET_ADDRSTRLEN + 1],
       *etype_str,
       *name,
       next_etype;
  struct host *h;
  
  if(!inet_ntop(AF_INET, &(ip), ip_str, INET_ADDRSTRLEN)) {
		print( ERROR, "inet_ntop(%u): %s\n", ip, strerror(errno));
		return;
	}
  
  name = NULL;
  mac_str[0] = '\0';
  
  if(etype != MAC_LOST) {
    pthread_mutex_lock(&(hosts.control.mutex));
    
    h = get_host(ip);
    
    if(h) {
      snprintf(mac_str, MAC_ADDRESS_STRLEN + 1, "%02hhX:%02hhX:%02hhX:%02hhX:%02hhX:%02hhX",
        h->mac[0], h->mac[1], h->mac[2], h->mac[3], h->mac[4], h->mac[5] );
      
      if(h->name) {
        name = strdup(h->name);
      }
    }
    
    pthread_mutex_unlock(&(hosts.control.mutex));
  }
  
  if(mac_str[0] == '\0') {
    strncpy(mac_str, "00:00:00:00:00:00", MAC_ADDRESS_STRLEN + 1);
  }

	again:
	
	next_etype = NONE;

	switch(etype) {
		case NEW_MAC:
			etype_str = "HOST_ADD ";
			break;
		case MAC_CHANGED:
			next_etype = NEW_MAC;
		case MAC_LOST:
			etype_str = "HOST_DEL ";
			break;
		case NAME_CHANGED:
		case NEW_NAME:
			etype_str = "HOST_EDIT";
			break;
		default:
			print(WARNING, "unknown event 0x%02hhX", etype);
			return;
	}
	
	if ( etype == MAC_LOST || etype == MAC_CHANGED ) {
		printf("%s { ip: %s }\n", etype_str, ip_str);
	} else {
		printf("%s { mac: %s, ip: %s, name: %s }\n",
				etype_str, mac_str, ip_str, ( name ? name : "" ) );
	}
	
	if(next_etype != NONE) {
		etype = next_etype;
		goto again;
	}
  
  if(name) {
    free(name);
  }
}

void *notifier(void *arg) {
  struct event *e;
  
  pthread_mutex_lock(&(events.control.mutex));
  
  while(events.control.active) {
  
    while(!events.list.head && events.control.active)
      pthread_cond_wait(&(events.control.cond), &(events.control.mutex));
    
    e = (struct event *) queue_get(&(events.list));
    
    if(!e) continue;
    
    print_host_event(e->type, e->ip);
    
    free(e);
    
    fflush(stdout);
  }
  
  pthread_mutex_unlock(&(events.control.mutex));
  
  return NULL;
}


int start_notifier() {
  if(pthread_create(&notifier_tid, NULL, notifier, NULL)) {
    print( ERROR, "pthread_create: %s\n", strerror(errno));
    return -1;
  }
  return 0;
}

void stop_notifier() {
  pthread_t tid;
  
  if(notifier_tid) {
    tid = notifier_tid;
    notifier_tid = 0;
    
    control_deactivate(&(events.control));
    pthread_join(tid, NULL);
  }
}
