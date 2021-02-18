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

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>

#include "logger.h"
#include "cSploitd.h"

/**
 * @brief SIGINT, SIGTERM handler
 */
void stop_handler(int sig) {
  // close the main socket
  stop_daemon();
}

/**
 * @brief register main thread signal handlers
 * @returns 0 on success, -1 on error.
 */
int register_signal_handlers() {
  struct sigaction action;
  
  action.sa_handler = stop_handler;
  sigemptyset(&(action.sa_mask));
  action.sa_flags = 0;
  
  if(sigaction(SIGINT, &action, NULL)) {
    print( ERROR, "sigaction(SIGINT): %s", strerror(errno) );
    return -1;
  } else if(sigaction(SIGTERM, &action, NULL)) {
    print( ERROR, "sigaction(SIGTERM): %s", strerror(errno) );
    return -1;
  }
  
   signal(SIGPIPE, SIG_IGN);
  
  //TODO: SIGSEGV core dumper ( like ettercap )
  
  return 0;
}
