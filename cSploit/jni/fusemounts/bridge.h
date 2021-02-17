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

#ifndef BRIDGE_H
#define BRIDGE_H
/**
 * a struct that holds data about mountpoints,
 * opened files and found sources directories.
 */
typedef struct bridge {
  
  ///< the directory where the fs is mounted on
  char *mountpoint;
  ///< the file descriptor of the opened file
  int fd;
  ///< the name of the file opened over mountpoint
  char *fname;
  ///< the size of @fname
  size_t flen;
  ///< the path of the source directory
  char *source;
  
  struct bridge *next;
  
} bridge_t;

bridge_t *create_bridge();
bridge_t *add_bridge(bridge_t *, bridge_t *);
bridge_t *del_bridge(bridge_t *, bridge_t *);
void free_bridge(bridge_t *);
void free_bridge_list(bridge_t *);

#endif