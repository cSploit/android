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

#include <stdlib.h>
#include <unistd.h>

#include "bridge.h"

/**
 * create a new `bridge_t`.
 * @return a pointer to the new `bridge_t` or NULL
 */
bridge_t *create_bridge() {
	
	bridge_t *res;
	
	res=malloc(sizeof(bridge_t));
	
	if(res) {
		res->mountpoint = NULL;
		res->fd = -1;
		res->fname = NULL;
		res->source = NULL;
		res->next = NULL;
	}
	return res;
}

/**
 * add @elem to @list .
 * @param list the list to work with
 * @param elem the `bridge_t` to add
 * @return the modified list
 */
bridge_t *add_bridge(bridge_t *list, bridge_t *elem) {
	
	bridge_t *tmp;
	
	if(list==NULL)
		return elem;
	
	for(tmp=list;tmp->next;tmp=tmp->next);
	
	tmp->next = elem;
	
	return list;
}

/**
 * remove @elem from @list .
 * @param list the list to work with
 * @param elem the `bridge_t` to add
 * @return the modified list
 */
bridge_t *del_bridge(bridge_t *list, bridge_t *elem) {
	
	bridge_t *tmp;
	
	if(list==NULL)
		return NULL;
	
	if(elem == list) {
		return list->next;
	}
	
	for(tmp=list;tmp->next!=elem&&tmp->next;tmp=tmp->next);
	
	if(!tmp->next)
		return list;
	
	tmp->next = elem->next;
	return list;
}

/**
 * free all resources used by @elem and @elem itself.
 * @param elem the `bridge_t` to free
 */
void free_bridge(bridge_t *elem) {
	
	if(elem==NULL)
		return;
	if(elem->mountpoint)
		free(elem->mountpoint);
	if(elem->fd >= 0)
		close(elem->fd);
	if(elem->fname)
		free(elem->fname);
	if(elem->source)
		free(elem->source);
	free(elem);
}

/**
 * free all `bridge_t` in @list .
 * @param list the list to free
 */
void free_bridge_list(bridge_t *list) {
	bridge_t *tmp1,*tmp2;
	
	for(tmp1=list;tmp1;tmp1=tmp2) {
		tmp2=tmp1->next;
		free_bridge(tmp1);
	}
}