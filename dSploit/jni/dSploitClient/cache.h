#ifndef CACHE_H
#define CACHE_H

#include <jni.h>

struct class_cache {
  jclass class;
};

struct class_and_ctor_cache {
  jclass class;
  jmethodID ctor;
};

struct events_cache {
  struct class_and_ctor_cache
        newline,
        child_end,
        child_died,
        hop,
        port,
        os,
        ready,
        account,
        message,
        login,
        attempts,
        packet;
};

struct core_chlidmanager_cache {
  jclass class;
  jmethodID on_event;
};

struct core_cache {
  struct core_chlidmanager_cache    childmanager;
  struct class_cache                client;
};

struct java_net_inetaddress_cache {
  jclass      class;
  jmethodID   getByAddress;
};

struct java_net_cache {
  struct java_net_inetaddress_cache inetaddress;
};

struct java_cache {
  struct java_net_cache net;
};

struct dsploit_cache {
  struct events_cache events;
  struct core_cache   core;
};

extern struct cache {
  struct dsploit_cache dsploit;
  struct java_cache java;
} cache;

extern JavaVM *jvm;

int _init_cache(JNIEnv *);
int init_cache(JavaVM *);
void _free_cache(JNIEnv *);
void free_cache(void);
#endif
