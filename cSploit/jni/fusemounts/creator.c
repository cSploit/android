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
#include <linux/limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <libgen.h>
#include <string.h>

#include "bridge.h"
#include "creator.h"

bridge_t *open_unique_files(bridge_t *list) {
  
  bridge_t  *b,
            *b1,
            *b2;
  char      *fullname;
        
  for(b=list;b;b=b->next) {
    fullname = tempnam(b->mountpoint, NULL);
    if(fullname) {
      b->fname = strdup(basename(fullname));
      b->flen = strlen(b->fname);
      b->fd = open(fullname, O_CREAT | O_RDWR, S_IRWXU);
      unlink(fullname);
      free(fullname);
    }
  }
  
  for(b=NULL,b1=list;b1;b1=b2) {
    b2=b1->next;
    if(b1->fd < 0) {
      if(b)
        b->next=b2;
      else
        list = b2;
      free_bridge(b1);
    } else {
      b = b1;
    }
  }
  return list;
}