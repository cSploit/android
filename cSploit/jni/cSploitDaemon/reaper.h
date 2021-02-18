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
 * 
 * 
 */

#ifndef REAPER_H
#define REAPER_H

#include <pthread.h>

#include "list.h"
#include "control.h"

typedef struct dead_node {
  node *next;
  pthread_t tid;
#ifndef NDEBUG
  char *name;
#endif
} dead_node;

extern struct graveyard {
  data_control control;
  list list;
} graveyard;

int start_reaper(void);
int stop_reaper(void);
#ifdef NDEBUG
void send_to_graveyard(pthread_t);
#else
void _send_to_graveyard(pthread_t, char *);
#define send_to_graveyard(t) _send_to_graveyard(t, strdup(__func__))
#endif


/// add yourself to the death list
#define send_me_to_graveyard() send_to_graveyard(pthread_self())

#endif
