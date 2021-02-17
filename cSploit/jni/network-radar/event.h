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
#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>

#include <csploit/list.h>
#include <csploit/control.h>

#define NONE         0x00
#define NEW_MAC      0x01
#define MAC_CHANGED  0x02
#define NEW_NAME     0x03
#define NAME_CHANGED 0x04
#define MAC_LOST     0x05

struct event {
  node *next;
  uint8_t type;
  uint32_t ip;
};

void add_event(struct event *);

extern struct events_list {
  list list;
  data_control control;
} events;

#endif