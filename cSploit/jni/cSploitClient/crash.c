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

#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>

#include "crash.h"
#include "log.h"

static void (* old_sa_handler)(int) = NULL;
static void (* old_sigaction)(int, siginfo_t *, void  *) = NULL;

/**
 * @brief handle a library crash by creating ::CRASH_FLAG_FILEPATH
 */
void crash_handler(int signal, siginfo_t *info, void * context) {
  
  if(creat(CRASH_FLAG_FILEPATH, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH) == -1) {
    LOGE("%s: unable to create '%s': %s", __func__, CRASH_FLAG_FILEPATH, strerror(errno));
  }
  
  if(old_sa_handler) {
    old_sa_handler(signal);
  } else if(old_sigaction) {
    old_sigaction(signal, info, context);
  }
}

/**
 * @brief check if library has previously crashed.
 * @return JNI_TRUE if ::CRASH_FLAG_FILEPATH exists, JNI_FALSE otherwise
 * 
 * remove ::CRASH_FLAG_FILEPATH if it exists.
 */
jboolean have_crash_flag(JNIEnv *env _U_, jclass clazz _U_) {
  
  if(!unlink(CRASH_FLAG_FILEPATH))
    return JNI_TRUE;
  
  return JNI_FALSE;
}

/**
 * @brief register our crash handler as SIGSEGV handler
 * @return 0 on success, -1 on error.
 * 
 * it also set SIGPIPE as ignored.
 */
int register_crash_handler() {
  struct sigaction new, old;
  
  new.sa_sigaction = crash_handler;
  sigemptyset(&(new.sa_mask));
  new.sa_flags = SA_SIGINFO;
  
  if(sigaction(SIGSEGV, &new, &old)) {
    LOGE("%s: sigaction(SIGSEGV): %s", __func__, strerror(errno));
    return -1;
  }
  
  if(old.sa_flags & SA_SIGINFO) {
    old_sigaction = old.sa_sigaction;
  } else {
    old_sa_handler = old.sa_handler;
  }
  
  signal(SIGPIPE, SIG_IGN);
  
  return 0;
}