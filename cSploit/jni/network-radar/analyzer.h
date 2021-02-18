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
#ifndef ANALYZER_H
#define ANALYZER_H

#ifdef HAVE_CONFIG_H
# include <config.h>
#endif

#include <pthread.h>


#include "list.h"
#include "control.h"

extern struct analyzer_data {
  data_control control;
  list list;
  pthread_t tid;
#ifdef PROFILE
  unsigned long int queue_max_size;
  unsigned long int queue_size;
  unsigned long int queue_max_count;
  unsigned long int queue_count;
#endif
} analyze;

int start_analyzer(void);
void stop_analyzer(void);

int enqueue_packet(char *, int , int);

#endif
