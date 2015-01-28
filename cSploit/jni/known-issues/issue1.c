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
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>

void issue1() {
  pid_t pid;
  char *argv[] = { "date", NULL };
  int exec_errno, i;
  int pexec[2];
  
  pexec[0] = pexec[1] = -1;
  
  if(pipe2(pexec, O_CLOEXEC)) {
    fprintf(stderr, "%s: pipe2: %s\n", __func__, strerror(errno));
    goto unsure;
  }
  
  pid = fork();
  
  if(pid == -1) {
    if(errno == EACCES) {
      goto yes;
    } else {
      fprintf(stderr, "%s: fork: %s\n", __func__, strerror(errno));
      goto unsure;
    }
  } else if(!pid) {
    // child
    close(pexec[0]);
    
    close(0);
    close(1);
    close(2);
    
    execvp("date", argv);
    
    exec_errno = errno;
    
    write(pexec[1], &exec_errno, sizeof(int));
    close(pexec[1]);
    exit(1);
  }
  
  close(pexec[1]);
  
  waitpid(pid, NULL, 0);
  
  i = read(pexec[0], &exec_errno, sizeof(int));
  
  if ( i < 0 ) {
    fprintf(stderr, "%s: read: %s\n", __func__, strerror(errno));
    goto unsure;
  } else if( i > 0 ) {
    if(exec_errno == EACCES) {
      goto yes;
    } else {
      fprintf(stderr, "%s: read: %s\n", __func__, strerror(errno));
      goto unsure;
    }
  }
  
  
  goto no;
  
  unsure:
  
  fprintf(stderr, "%s: assuming yes\n", __func__);
  
  yes:
  printf("1\n");
  
  no:
  
  close(pexec[0]);
  close(pexec[1]);
  
}
