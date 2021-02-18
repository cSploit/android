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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <signal.h>
#include <unistd.h>

#include "logger.h"
#include "file_logger.h"

#include "event.h"
#include "sniffer.h"
#include "notifier.h"
#include "resolver.h"
#include "prober.h"
#include "host.h"
#include "ifinfo.h"
#include "main.h"
#include "analyzer.h"
#include "scanner.h"

void stop(int signal) {
  hosts.control.active = 0;
}

void update(int signal) {
  full_scan();
}

/**
 * @brief register main thread signal handlers
 * @returns 0 on success, -1 on error.
 */
int register_signal_handlers() {
  struct sigaction action;
  
  action.sa_handler = stop;
  sigemptyset(&(action.sa_mask));
  action.sa_flags = 0;
  
  if(sigaction(SIGINT, &action, NULL)) {
    print( ERROR, "sigaction(SIGINT): %s", strerror(errno) );
    return -1;
  }
  
  if(sigaction(SIGTERM, &action, NULL)) {
    print( ERROR, "sigaction(SIGTERM): %s", strerror(errno) );
    return -1;
  }
  
  action.sa_handler = update;
  
  if(sigaction(SIGHUP, &action, NULL)) {
    print( ERROR, "sigaction(SIGHUP): %s", strerror(errno) );
    return -1;
  }
  
  return 0;
}

int init_controls() {
  memset(&(events.control), 0, sizeof(struct data_control));
  memset(&(hosts.control), 0, sizeof(struct data_control));
  memset(&(sniffer_info.control), 0, sizeof(struct data_control));
  memset(&(resolver_info.control), 0, sizeof(struct data_control));
  memset(&(prober_info.control), 0, sizeof(struct data_control));
  
  if(control_init(&(events.control)))
    return -1;
  
  if(control_init(&(hosts.control))) goto e1;
    
  if(control_init(&(sniffer_info.control))) goto e2;
    
  if(control_init(&(resolver_info.control))) goto e3;
  
  if(control_init(&(prober_info.control))) goto e4;
  
  if(control_init(&(analyze.control))) goto e5;
  
  return 0;
  
  e5:
  control_destroy(&(prober_info.control));
  e4:
  control_destroy(&(resolver_info.control));
  e3:
  control_destroy(&(sniffer_info.control));
  e2:
  control_destroy(&(hosts.control));
  e1:
  control_destroy(&(events.control));
  
  return -1;
}

void destroy_controls() {
  control_destroy(&(prober_info.control));
  control_destroy(&(resolver_info.control));
  control_destroy(&(sniffer_info.control));
  control_destroy(&(hosts.control));
  control_destroy(&(events.control));
}

int main(int argc, char **argv) {
  char *prog_name;
  int ret;
  
  if(argc < 2 || !strncmp(argv[1], "-h", 3) || !strncmp(argv[1], "--help", 7) ) {
    prog_name = strrchr(argv[0], '/');
    
    if(!prog_name)
      prog_name = argv[0];
    else
      prog_name++;
    
    print( ERROR, "Usage: %s <interface>\n", prog_name);
    return EXIT_FAILURE;
  }
  
  if(open_logfile("./network-radar.log")) {
    print( ERROR, "cannot open logfile");
    return EXIT_FAILURE;
  }
  
  set_logger(file_logger);
  
  if(get_ifinfo(argv[1]))
    return EXIT_FAILURE;
  
  if(register_signal_handlers())
    return EXIT_FAILURE;
    
  if(init_controls())
    return EXIT_FAILURE;
  
  if(init_prober()) goto exit;
  
  if(start_analyzer()) goto exit;
  
  if(start_notifier()) goto exit;
  
  if(start_resolver()) goto exit;
  
  if(start_sniff()) goto exit;
  
  prober(NULL);
  
  ret = 0;
  
  exit:
  stop_prober();
  stop_analyzer();
  stop_sniff();
  stop_notifier();
  stop_resolver();
  
  fini_hosts();
  
  destroy_controls();
  
  return ret;
}
