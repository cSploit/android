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
        stderrnewline,
        hop,
        port,
        os,
        ready,
        account,
        message,
        login,
        attempts,
        packet,
        fusebind,
        host,
        hostlost;
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

struct csploit_cache {
  struct events_cache events;
  struct core_cache   core;
};

extern struct cache {
  struct csploit_cache csploit;
  struct java_cache java;
} cache;

extern JavaVM *jvm;

int _init_cache(JNIEnv *);
int init_cache(JavaVM *);
void _free_cache(JNIEnv *);
void free_cache(void);
#endif
