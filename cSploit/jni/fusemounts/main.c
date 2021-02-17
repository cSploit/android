/**
 * fusermounts - show mounted fuse filesystems
 * Copyright (C) 2014  massimo dragano aka tux_mind <massimo.dragano@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "bridge.h"
#include "finder.h"
#include "creator.h"


int main() {
  pid_t *handlers;
  bridge_t *list, *b;
  int max_mountpoint_len, max_source_len, len, ret;
  
  list = NULL;
  handlers = NULL;
  max_mountpoint_len = max_source_len = 0;
  
  ret = 1;
  
  if(geteuid()) {
    fprintf(stderr, "got r00t?\n");
    goto exit;
  }
  
  handlers = find_handlers();
  
  if(!handlers) {
    fprintf(stderr, "no FUSE handlers found\n");
    goto exit;
  }
  
  list = find_mountpoints();
  
  if(!list) {
    fprintf(stderr, "cannot find FUSE mountpoints\n");
    goto exit;
  }
  
  list = open_unique_files(list);
  
  if(!list) {
    fprintf(stderr, "cannot create files\n");
    goto exit;
  }
  
  list = find_sources(list, handlers);
  
  if(!list) {
    fprintf(stderr, "cannot find source paths\n");
    goto exit;
  }
  
  for(b=list;b;b=b->next) {
    if(max_mountpoint_len < (len = strlen(b->mountpoint)))
      max_mountpoint_len = len;
    if(max_source_len < (len = strlen(b->source)))
      max_source_len = len;
  }
  
  for(b=list;b;b=b->next) {
    printf("%-*s %-*s\n", max_source_len, b->source, max_mountpoint_len, b->mountpoint);
  }
  
  ret = 0;
  
  exit:
  
  if(handlers)
    free(handlers);
  
  free_bridge_list(list);
  
  return ret;
}