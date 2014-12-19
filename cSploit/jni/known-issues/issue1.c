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
#include <errno.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>

/* WARNING: this check works if you DO NOT USE threads
 *          when using threads fork(2) become clone(2),
 *          which is not restricted.
 *          if you use threads within this project
 *          check the execve(2) syscall insted of fork(2).
 */

void issue1() {
  pid_t pid;
  
  pid = fork();
  
  if(pid == -1) {
    if(errno == EACCES) {
      goto yes;
    } else {
      goto unsure;
    }
  } else if(!pid) {
    // child
    exit(0);
  }
  
  waitpid(pid, NULL, 0);
  
  return;
  
  unsure:
  
  fprintf(stderr, "%s: fork: %s\n", __func__, strerror(errno));
  
  yes:
  
  printf("1\n");
  
}