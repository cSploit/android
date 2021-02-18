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
 */

#ifndef LOGGER_H
#define LOGGER_H

#include <pthread.h>

enum loglevel {
  DEBUG,
  INFO,
  WARNING,
  ERROR,
  FATAL,
};

extern void std_logger(int, char *, ...);

extern pthread_mutex_t print_lock;
extern void (*_print)(int, char *, ...);

#define print( level, fmt, args...) _print( level, "%s: " fmt, __func__, ##args)
#define set_logger(func) _print = func

#endif