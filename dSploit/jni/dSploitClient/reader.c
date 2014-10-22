/* LICENSE
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
  
  LOGI("%s: quitting", __func__);
  
  return 0;
}

int start_reader() {
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