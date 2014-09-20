/*
 * LICENSE
 * 
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

#include "dSploitd.h"
#include "message.h"
#include "connection.h"
#include "cleanup.h"
#include "sender.h"
#include "receiver.h"
#include "handler.h"
#include "reaper.h"
#include "child.h"

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
        fprintf(stderr, "%s: unlink: %s\n", __func__, strerror(errno));
        return -1;
      }
    } else {
      fprintf(stderr, "%s: '%s' is not a socket, remove it manually\n", __func__, SOCKET_PATH);
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
  if( control_init(&(incoming_messages.control)) ||
      control_init(&(outcoming_messages.control)) ||
      control_init(&(connections.control)) ||
      control_init(&(graveyard.control)) )
    return -1;
  return 0;
}

/**
 * @brief destroy lists controls
 */
void destroy_structs() {
  control_destroy(&(incoming_messages.control));
  control_destroy(&(outcoming_messages.control));
  control_destroy(&(connections.control));
  control_destroy(&(graveyard.control));
}

/**
 * @brief stop the main thread by closing the main socket
 */
void stop_daemon() {
  close(sockfd);
}


int main(int argc, char **argv) {
  struct sockaddr_un addr;
  pid_t pid, sid;
  int pipefd[2];
  int clfd;
  
  if(pipe2(pipefd, O_CLOEXEC)) {
    fprintf(stderr, "%s: pipe2: %s\n", __func__, strerror(errno));
    return EXIT_FAILURE;
  }
  
  pid = fork();
  
  if(pid<0) {
    fprintf(stderr, "%s: fork: %s\n", __func__, strerror(errno));
    return EXIT_FAILURE;
  } else if(pid) {
    close(pipefd[1]);
    if(!read(pipefd[0], &clfd, 1))
      return EXIT_FAILURE;
    return EXIT_SUCCESS;
  }
  close(pipefd[0]);
  
  umask(0);
  
  
  close(STDIN_FILENO);
  close(STDOUT_FILENO);
  close(STDERR_FILENO);
  
  if(!freopen(LOG_PATH, "w", stderr)) {
    fprintf(stderr, "%s: freopen(\"%s\", \"w\", stderr): %s\n", __func__, LOG_PATH, strerror(errno));
    return EXIT_FAILURE;
  }
  
#ifndef NDEBUG
  if(!freopen(DEBUG_LOG_PATH, "w", stdout)) {
    fprintf(stderr, "%s: freopen(\"%s\", \"w\", stdout): %s\n", __func__, DEBUG_LOG_PATH, strerror(errno));
    return EXIT_FAILURE;
  }
#endif

  sid = setsid();
  if(sid<0) {
    fprintf(stderr, "%s: setsid: %s\n", __func__, strerror(errno));
    return EXIT_FAILURE;
  }
  
  if(init_structs())
    return EXIT_FAILURE;
  
  if(load_handlers())
    return EXIT_FAILURE;
  
  if(remove_old_socket())
    return EXIT_FAILURE;
  
  sockfd = socket(AF_UNIX, SOCK_STREAM, 0);
  
  if(sockfd < 0) {
    fprintf(stderr, "%s: socket: %s\n", __func__, strerror(errno));
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
    fprintf(stderr, "%s: bind: %s\n", __func__, strerror(errno));
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
  if(listen(sockfd, 5)) {
    fprintf(stderr, "%s: listen: %s\n", __func__, strerror(errno));
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
  if(start_receiver() || start_sender() || start_reaper()) {
    stop_receiver();
    stop_sender();
    close(sockfd);
    unlink(SOCKET_PATH);
    return EXIT_FAILURE;
  }
  
#ifndef NDEBUG
  chmod(SOCKET_PATH, 0666);
#endif
  
  if(write(pipefd[1], "!", 1) != 1) {
    fprintf(stderr, "%s: cannot notify that daemon started", __func__);
    return EXIT_FAILURE;
  }
  close(pipefd[1]);
  
  while(1) {
    if((clfd=accept(sockfd, NULL, NULL)) < 0) {
      if(errno == EBADF) {
#ifndef NDEBUG
        printf("%s: socket closed\n", __func__);
#endif
        break;
      }
      fprintf(stderr, "%s: accept: %s\n", __func__, strerror(errno));
      continue;
    }
    
    if(serve_new_client(clfd)) {
      fprintf(stderr, "%s: cannot serve new connection\n", __func__);
      close(clfd);
    }
  }
  
  unlink(SOCKET_PATH);
  
  if(control_deactivate(&(connections.control))) {
    fprintf(stderr, "%s: cannot deactivate new connections\n", __func__);
    close_connections();
  }
  
  stop_receiver();
  stop_children();
  stop_sender();
  
  close_connections();
  
  stop_reaper();
  
  destroy_structs();
  
  unload_handlers();
  
  return EXIT_SUCCESS;
}
