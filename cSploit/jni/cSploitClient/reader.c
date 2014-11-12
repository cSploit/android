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
 * 
 */

#include <jni.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>

#include "log.h"

#include "message.h"
#include "msgqueue.h"
#include "connection.h"

#include "reader.h"

struct msgqueue incoming_messages;
pthread_t reader_tid;

void *reader(void *arg) {
  message *m;
  
  while((m = read_message(sockfd))) {
    if(enqueue_message(&(incoming_messages), m)) {
      LOGE("%s: cannot enqueue messages", __func__);
      dump_message(m);
      free_message(m);
      break;
    }
  }
  
  control_deactivate(&(incoming_messages.control));
  
  LOGI("%s: quitting", __func__);
  
  return 0;
}

int start_reader() {
  if(reader_tid)
    stop_reader();
  
  if(control_activate(&(incoming_messages.control))) {
    LOGE("%s: cannot activate incoming messages queue", __func__);
    return -1;
  }
  if(pthread_create(&reader_tid, NULL, reader, NULL)) {
    LOGE("%s: pthread_create: %s", __func__, strerror(errno));
    return -1;
  }
  return 0;
}

/**
 * @brief stop the reader thread.
 * WARNING: ensure to shutdown ::sockfd before call this.
 */
void stop_reader() {
  if(reader_tid) {
    control_deactivate(&(incoming_messages.control));
    pthread_join(reader_tid, NULL);
    reader_tid = 0;
  }
}
