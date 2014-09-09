/*
 * 
 * LICENSE
 * 
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <linux/limits.h>
#include <stdio.h>

/**
 * @brief search @p cmd in the PATH environment variable.
 * @param cmd the command to search
 * @return the string of the absolute command path, or NULL on error.
 */
char *find_cmd_in_path(char *cmd) {
  char *envpath,*lasts,*fpath,*dir;
  
  envpath = getenv("PATH");
  if(!envpath) {
    fprintf(stderr, "%s: cannot get PATH environment variable!\n", __func__);
    return NULL;
  }
  
  envpath = strdup(envpath);
  if(!envpath) {
    fprintf(stderr, "%s: strdup: %s\n", __func__, strerror(errno));
    return NULL;
  }
  
  fpath=NULL;
  for(dir=strtok_r(envpath, ":", &lasts);dir && !fpath; dir=strtok_r(NULL, ":", &lasts)) {
    if(asprintf(&fpath, "%s/%s", dir, cmd) < 0) {
      fprintf(stderr, "%s: asprintf: %s\n", __func__, strerror(errno));
      fpath=NULL; // from "man asprintf": the contents of strp is undefined.
    } else if(access(fpath, X_OK)) {
      free(fpath);
      fpath=NULL;
    }
  }
  
  free(envpath);
  
  return fpath;
}