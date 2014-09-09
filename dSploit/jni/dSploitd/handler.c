/* LICENSE
 * 
 * 
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <dirent.h>
#include <dlfcn.h>
#include <unistd.h>
#include <stdlib.h>

#include "list.h"
#include "handler.h"
#include "child.h"

list handlers;

/**
 * @brief load all handlers found in the @HANDLERS_DIR
 * @returns 0 on success, -1 on error.
 */
int load_handlers() {
  DIR *d, *test;
  struct dirent *de;
  handler *h,*tmp;
  char *path;
  void *handle;
  size_t len;
  
  d = opendir(HANDLERS_DIR);
  
  if(!d) {
    fprintf(stderr, "%s: opendir: %s\n", __func__, strerror(errno));
    return -1;
  }
  
  while((de=readdir(d))) {
    if(!strncmp(de->d_name, ".", 2) || !strncmp(de->d_name, "..", 3)) {
      continue;
    }
    
    len = strlen(de->d_name);
    
    if(strncmp(de->d_name + (len - 3), ".so", 3))
      continue;
    
    if(asprintf(&path, HANDLERS_DIR "/%s", de->d_name) < 0) {
      fprintf(stderr, "%s: asprintf: %s\n", __func__, strerror(errno));
      continue;
    }
    
    if(!(handle = dlopen(path, RTLD_NOW))) {
      fprintf(stderr, "%s: dlopen: %s\n", __func__, dlerror());
      free(path);
      continue;
    }
    
    if(!(h = (handler *)dlsym(handle, "handler_info"))) {
      fprintf(stderr, "%s: \"%s\": undefined reference to 'handler_info'\n", __func__, path);
      goto close;
    }
    
    if(!(h->enabled)) {
      fprintf(stderr, "%s: \"%s\": disabled\n", __func__, path);
      goto close;
    }
    
    if(h->argv0 && access(h->argv0, X_OK)) {
      fprintf(stderr, "%s: while loading \"%s\": access(\"%s\", X_OK): %s\n",
              __func__, path, h->argv0, strerror(errno));
      goto close;
    }
    
    if(h->workdir) {
      test = opendir(h->workdir);
      if(!test) {
        fprintf(stderr, "%s: \"%s\": opendir(\"%s\"): %s\n",
                __func__, path, h->workdir, strerror(errno));
        goto close;
      } else {
        closedir(test);
      }
    }
    
    h->dl_handle = handle;
    
    list_add(&(handlers), (node *) h);
    
    free(path);
    continue;
  
    close:
      
    if(dlclose(handle))
      fprintf(stderr, "%s: dlclose(\"%s\"): %s\n", __func__, path, dlerror());
    
    free(path);
  }
  
  closedir(d);
  
  if(!handlers.head) {
    fprintf(stderr, "%s: no handlers found\n", __func__);
    return -1;
  }
  
  for(h=(handler *) handlers.head;h->next;h=(handler *) h->next) {
    for(tmp=(handler *) h->next;tmp && tmp->id != h->id; tmp=(handler *) tmp->next);
    if(tmp) {
      fprintf(stderr, "%s: \"%s\" and \"%s\" has the same id. (id=%d)\n",
              __func__, h->name, tmp->name, h->id);
      return -1;
    }
  }
  
  return 0;
}

void unload_handlers() {
  handler *h;
  
  while((h=(handler *) queue_get(&(handlers)))) {
    if(dlclose(h->dl_handle))
      fprintf(stderr, "%s: dlclose on \"%s\": %s\n", __func__, h->name, dlerror());
  }
}
