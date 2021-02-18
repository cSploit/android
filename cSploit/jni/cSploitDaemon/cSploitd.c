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

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "cSploitd.h"
#include "message.h"
#include "connection.h"
#include "cleanup.h"
#include "handler.h"
#include "reaper.h"
#include "logger.h"
#include "authenticator.h"
#include "file_logger.h"

int sockfd;

/**
 * @brief delete an old socket
 * @returns 0 if cannot delete it, -1 otherwise.
 */
int remove_old_socket() {
  struct stat s;
  
  if(!stat(SOCKET_PATH, &s)) {
    // socket already exists
    if(S_ISSOCK(s.st_mode)) {
      if(unlink(SOCKET_PATH)) {
        print( FATAL, "unlink: %s", strerror(errno) );
        return -1;
      }
    } else {
      print( FATAL, "'%s' is not a socket, remove it manually", SOCKET_PATH );
      return -1;
    }
  }
  
  return 0;
}

/**
 * @brief initialize lists controls
 * @returns 0 on success, -1 on error.
 */
int init_structs() {
  if( control_init(&(connections.control)) ||
      control_init(&(graveyard.control)) )
    return -1;
  return 0;
}

/**
 * @brief destroy lists controls
 */
void destroy_structs() {
  control_destroy(&(connections.control));
  control_destroy(&(graveyard.control));
}

/**
 * @brief stop the main thread by closing the main socket
 */
void stop_daemon() {
  shutdown(sockfd, SHUT_RD);
}

int main(int argc, char **argv) {
  struct sockaddr_un addr;
  pid_t pid, sid;
  int pipefd[2];
  int clfd;
  char deamonize;
  
  if(argc==2 && !strncmp(argv[1], "-f", 3)) {
    deamonize=0;
  } else {
    deamonize=1;
  }
  
  if(deamonize) {
    if(pipe2(pipefd, O_CLOEXEC)) {
      print( FATAL, "pipe2: %s", strerror(errno) );
      return EXIT_FAILURE;
    }
    
    pid = fork();
    
    if(pid<0) {
      print( FATAL, "fork: %s", strerror(errno) );
      return EXIT_FAILURE;
    } else if(pid) {
      close(pipefd[1]);
      if(!read(pipefd[0], &clfd, 1))
        return EXIT_FAILURE;
      return EXIT_SUCCESS;
    }
    close(pipefd[0]);
  
    umask(0);

    if(open_logfile(LOG_PATH)) {
      print( FATAL, "cannot open logfile");
      return EXIT_FAILURE;
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    set_logger(file_logger);

    sid = setsid();
    if(sid<0) {
      print( FATAL, "setsid: %s", strerror(errno) );
      return EXIT_FAILURE;
    }
  }
  
  if(init_structs())
    return EXIT_FAILURE;
  
  if(load_handlers())
    return EXIT_FAILURE;
  
  if(load_users())
    return EXIT_FAILURE;
  
  if(remove_old_socket())
    return EXIT_FAILURE;
  
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  
  if(sockfd < 0) {
    print( FATAL, "socket: %s", strerror(errno) );
    return EXIT_FAILURE;
  }
  
  if(register_signal_handlers()) {
    close(sockfd);
    return EXIT_FAILURE;
  }
  
  memset(&addr, 0, sizeof(struct sockaddr_un));
  addr.sun_family = AF_UNIX;
  strncpy(addr.sun_path, SOCKET_PATH, sizeof(addr.sun_path)-1);
  if(bind(sockfd, (struct sockaddr*)&addr, sizeof(addr))) {
    print( FATAL, "bind: %s", strerror(errno) );
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
  if(listen(sockfd, 5)) {
    print( FATAL, "listen: %s", strerror(errno) );
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
  if(start_reaper()) {
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
#ifndef NDEBUG
  chmod(SOCKET_PATH, 0666);
#endif
  
  if(deamonize) {
    if(write(pipefd[1], "!", 1) != 1) {
      print( FATAL, "cannot notify that daemon started" );
      return EXIT_FAILURE;
    }
    close(pipefd[1]);
  }
  
  while(1) {
    if((clfd=accept(sockfd, NULL, NULL)) < 0) {
      if(errno == EINVAL) {
#ifndef NDEBUG
        print( DEBUG, "socket closed" );
#endif
      }
      print( ERROR, "accept: %s", strerror(errno) );
      break;
    }
    
    if(serve_new_client(clfd)) {
      print( WARNING, "cannot serve new connection" );
      close(clfd);
    }
  }
  
  unlink(SOCKET_PATH);
  
  close_connections();
  
  stop_reaper();
  
  destroy_structs();
  
  unload_users();
  unload_handlers();
  
  return EXIT_SUCCESS;
}
