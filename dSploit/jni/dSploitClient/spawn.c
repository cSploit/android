/* LICENSE
 * 
 */
#define _GNU_SOURCE
#include <jni.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

#include "log.h"
#include "spawn.h"
#include "buffer.h"

void dump_stderr(int fd) {
  buffer buff;
  char read_buff[255];
  char *line;
  int count;
  
  memset(&buff, 0, sizeof(buff));
  
  while((count=read(fd, &read_buff, 255)) > 0) {
    if(append_to_buffer(&buff, read_buff, count)) {
      LOGE("%s: cannot append to buffer", __func__);
      break;
    }
    while((line=get_line_from_buffer(&buff))) {
      LOGE("%s: %s", __func__, line);
      free(line);
    }
  }
  
  if(count<0) {
    LOGE("%s: read: %s", __func__, strerror(errno));
  }
  
  release_buffer(&buff);
}

/**
 * @brief start the dSploitd daemon
 * @param path path to dSploitd working dir
 * @returns true on success, false on error.
 */
jboolean start_daemon(JNIEnv *env, jclass clazz _U_, jstring jpath) {
  pid_t pid;
  const char *path;
  char *pos;
  char dump;
  int status;
  int pexec[2], pin[2], perr[2];
  jboolean ret;
  
  static char *argv[] = {
    "su",
    NULL
  };
  
  pexec[0] = pexec[1] = pin[0] = pin[1] = perr[0] = perr[1] = -1;
  pid = -1;
  dump = 0;
  ret = JNI_FALSE;
  
  path = (*env)->GetStringUTFChars(env, jpath, NULL);
  if(!path) goto jni_error;
  
  LOGD("%s: path=\"%s\"", __func__, path);
  
  for(pos=(char *)path;*pos!='\0';pos++) {
    switch(*pos) {
      case '\'':
      case '`':
      case '$':
      case '(':
        LOGE("%s: bash exploitation detected", __func__);
        goto exit;
    }
  }
  
  if (pipe2(pexec, O_CLOEXEC)) {
    LOGE("%s: pipe2: %s", __func__, strerror(errno));
    goto exit;
  }
  
  if (pipe(pin) || pipe(perr)) {
    LOGE("%s: pipe: %s", __func__, strerror(errno));
    goto exit;
  }
  
  if((pid = fork()) < 0) {
    LOGE("%s: fork: %s", __func__, strerror(errno) );
    goto exit;
  } else if(!pid) {
    // child
    close(pexec[0]);
    close(pin[1]);
    close(perr[0]);
    
    dup2(pin[0], STDIN_FILENO);
    dup2(perr[1], STDERR_FILENO);

    close(pin[0]);
    close(perr[1]);
    
    execvp(argv[0], argv);
    status=errno;
    write(pexec[1], &status, sizeof(int));
    close(pexec[1]);
    exit(-1);
  } else {
    // parent
    close(pexec[1]);
    close(pin[0]);
    close(perr[1]);
    pexec[1] = pin[0] = perr[1] = -1;
    
    if(read(pexec[0],&status, sizeof(int))) {
      LOGE("%s: execvp: %s", __func__, strerror(status) );
      waitpid(pid, NULL, 0);
      goto exit;
    }
    close(pexec[0]);
    pexec[0] = -1;
  }
  
  // now we got a root shell,
  // chdir to the right working directory
  // and exec dSploitd
  // NOTE: we must chdir from su shell, maybe only root can access it.
  write(pin[1], "{ cd '", 6);
  write(pin[1], path, strlen(path));
  write(pin[1], "' && exec ./dSploitd ;} || exit 1\n", 34);
  
  close(pin[1]);
  pin[1] = -1;
  
  pid = waitpid(pid, &status, 0);
  
  if(pid==-1) {
    LOGE("%s: waitpid: %s", __func__, strerror(errno));
    goto exit;
  }
  
  dump=1;
  
  if(WIFEXITED(status)) {
    if(WEXITSTATUS(status)) {
      LOGE("%s: dSploitd returned %d", __func__, WEXITSTATUS(status));
      goto exit;
    }
  } else if(WIFSIGNALED(status)) {
    LOGE("%s: dSploitd terminated by signal #%d", __func__, WTERMSIG(status));
    goto exit;
  } else {
    LOGE("%s: dSploitd terminated unexpectedly: status=%0*X", __func__, sizeof(int), status);
    goto exit;
  }
  
  dump=0;
  
  ret = JNI_TRUE;
  
  goto exit;
  
  jni_error:
  
  if((*env)->ExceptionCheck(env)) {
    (*env)->ExceptionDescribe(env);
    (*env)->ExceptionClear(env);
  }
  
  exit:
  
  if(dump && perr[0] != -1 )
    dump_stderr(perr[0]);
  
  if(pexec[0] != -1)
    close(pexec[0]);
  if(pexec[1] != -1)
    close(pexec[1]);
  if(pin[0] != -1)
    close(pin[0]);
  if(pin[1] != -1)
    close(pin[1]);
  if(perr[0] != -1)
    close(perr[0]);
  if(perr[1] != -1)
    close(perr[1]);
  
  if(path)
    (*env)->ReleaseStringUTFChars(env, jpath, path);
  
  return ret;
}