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
#include <dirent.h>
#include <unistd.h>
#include <linux/limits.h>
#include <string.h>
#include <math.h>

#include "bridge.h"
#include "finder.h"

#define FUSE_DEV_NEW "/dev/fuse"
#define FUSE_DEV_OLD "/proc/fs/fuse/dev"

#define FUSE_DEV_NEW_LEN 9
#define FUSE_DEV_OLD_LEN 17
#define FUSE_DEV_MAX_LEN FUSE_DEV_OLD_LEN
#define LINKBUFFER_SIZE (FUSE_DEV_MAX_LEN + 1)

/**
 * find all processes that handles a fuse filesystem.
 * @return a NULL-terminated array of found FUSE handlers
 */
pid_t *find_handlers() {
	
	DIR 								*d,
											*d1;
	struct dirent 			*entry,
											*e1;
	pid_t 							*res,
											cur_pid;
	char 								path_buffer[255],
											link_buffer[LINKBUFFER_SIZE];
	ssize_t 						link_size;
	size_t 							pid_size;
	unsigned long long 	pid_mask;
	int 					 			found;
	
	d = opendir("/proc");
	
	if(!d)
		return NULL;
	
	pid_size = sizeof(pid_t);
	pid_mask = pow(2,(pid_size*8)) - 1;
	found = 0;
	res = NULL;
	
	while((entry = readdir(d))) {
		if((cur_pid = (pid_t)(strtoul(entry->d_name, NULL, 0) & pid_mask) ) == 0)
			continue;
		
		snprintf(path_buffer, 255, "/proc/%s/fd/", entry->d_name);
		
		if(!(d1 = opendir(path_buffer)))
			continue;
		
		while((e1 = readdir(d1))) {
			snprintf(path_buffer, 255, "/proc/%s/fd/%s", entry->d_name, e1->d_name);
			
			link_size = readlink(path_buffer, link_buffer, LINKBUFFER_SIZE);
			
			if( ( link_size == FUSE_DEV_OLD_LEN && 
						!strncmp(FUSE_DEV_OLD, link_buffer, FUSE_DEV_OLD_LEN)) ||
					( link_size == FUSE_DEV_NEW_LEN &&
						!strncmp(FUSE_DEV_NEW, link_buffer, FUSE_DEV_NEW_LEN)) ) {
				res = realloc(res,((found+1)*pid_size));
				*(res + found) = cur_pid;
				found++;
				break;
			}
		}
		closedir(d1);
	}
	closedir(d);
	
	if(res)
		*(res+found+1) = 0;
	
	return res;
}

/**
 * find all FUSE mountpoints from "/proc/mounts".
 * @return a new list of `bridge_t`
 */
bridge_t *find_mountpoints() {
	
	FILE * fp;
	char buffer[1024],
			 *space,
			 *mountpoint;
	bridge_t *list,*b;
	
	fp = fopen("/proc/mounts", "r");
	
	if(!fp)
		return NULL;
	
	list = NULL;
	
	while(fgets(buffer, 1024, fp)) {
		if(!strstr(buffer, "fuse") ||
			!(space = strchr(buffer, ' '))
		)
			continue;
			
		mountpoint=space+1;
		space=strchr(mountpoint, ' ');
		
		if(!space)
			continue;
		
		*space = '\0';
		
		b = create_bridge();
		if(!b)
			break;
		b->mountpoint = strdup(mountpoint);
		list = add_bridge(list, b);
	}
	
	fclose(fp);
	
	return list;
}

/**
 * find directories that are mapped to fuse mounpoints
 * @param list list containing all mountpoints to resolve
 * @param handlers a NULL-terminated array of FUSE daemons processes
 * @return a list of resolved mountpoints
 */
bridge_t *find_sources(bridge_t* list, pid_t* handlers) {
	
	DIR 					*d;
	struct dirent *entry;
	pid_t 				*cur_pid;
	char 					path_buffer[255],
								link_buffer[PATH_MAX],
								*ptr,
								*ptr2;
	ssize_t 			link_size;
	bridge_t      *b,
								*b1,
								*b2;
	
	for(cur_pid=handlers;*cur_pid;cur_pid++) {
		
		snprintf(path_buffer, 255, "/proc/%u/fd/", *cur_pid);
		
		if(!(d=opendir(path_buffer)))
			continue;
		
		while((entry=readdir(d))) {
			if(!strncmp(entry->d_name, ".", 2) || !strncmp(entry->d_name, "..", 3))
				continue;
			
			snprintf(path_buffer, 255, "/proc/%u/fd/%s", *cur_pid, entry->d_name);
			
			link_size = readlink(path_buffer, link_buffer, PATH_MAX);
			if(link_size<0)
				continue;
			
			for(b=list;b;b=b->next) {
				
				if(link_size < b->flen)
					continue;
				
				ptr = strstr(link_buffer, b->fname);
				ptr2 = strstr(link_buffer, " (deleted)");
				
				if(!ptr)
					continue;
				
				if(ptr2) {
					*ptr2='\0';
					link_size-=10;
				}
				
				
				if((ptr + b->flen) == (link_buffer + link_size)) {
					*ptr='\0';
					b->source = strdup(link_buffer);
					break;
				}
			}
		}
		closedir(d);
	}
	
	for(b=NULL,b1=list;b1;b1=b2) {
		b2=b1->next;
		if(!(b1->source)) {
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