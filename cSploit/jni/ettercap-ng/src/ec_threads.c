/*
    ettercap -- thread handling

    Copyright (C) ALoR & NaGA

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

*/

#include <ec.h>
#include <ec_threads.h>

#include <pthread.h>
#include <errno.h>

struct thread_list {
   struct ec_thread t;
   LIST_ENTRY (thread_list) next;
};


/* global data */
#define DETACHED_THREAD 1
#define JOINABLE_THREAD 0

static LIST_HEAD(, thread_list) thread_list_head;

static pthread_mutex_t threads_mutex = PTHREAD_MUTEX_INITIALIZER;
#define THREADS_LOCK     do{ pthread_mutex_lock(&threads_mutex); } while(0)
#define THREADS_UNLOCK   do{ pthread_mutex_unlock(&threads_mutex); } while(0)

static pthread_mutex_t init_mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t init_cond = PTHREAD_COND_INITIALIZER;
#define INIT_LOCK     do{ DEBUG_MSG("thread_init_lock"); pthread_mutex_lock(&init_mtx); } while(0)
#define INIT_UNLOCK   do{ DEBUG_MSG("thread_init_unlock"); pthread_mutex_unlock(&init_mtx); } while(0)

/* protos... */

char * ec_thread_getname(pthread_t id);
pthread_t ec_thread_getpid(char *name);
char * ec_thread_getdesc(pthread_t id);
void ec_thread_register_detached(pthread_t id, char *name, char *desc, int detached);
void ec_thread_register(pthread_t id, char *name, char *desc);
pthread_t ec_thread_new(char *name, char *desc, void *(*function)(void *), void *args);
pthread_t ec_thread_detached(char *name, char *desc, void *(*function)(void *), void *args, int detached);
void ec_thread_destroy(pthread_t id);
void ec_thread_init(void);
void ec_thread_kill_all(void);
void ec_thread_exit(void);

/*******************************************/

/* returns the name of a thread */

char * ec_thread_getname(pthread_t id)
{
   struct thread_list *current;
   char *name;

   if (pthread_equal(id, EC_PTHREAD_SELF))
      id = pthread_self();

   /* don't lock here to avoid deadlock in debug messages */
#ifndef DEBUG   
   THREADS_LOCK;
#endif
   
   LIST_FOREACH(current, &thread_list_head, next) {
      if (pthread_equal(current->t.id, id)) {
         name = current->t.name;
#ifndef DEBUG
         THREADS_UNLOCK;
#endif
         return name;
      }
   }
   
#ifndef DEBUG
   THREADS_UNLOCK;
#endif

   return "NR_THREAD";
}

/* 
 * returns the pid of a thread 
 * ZERO if not found !! (take care, not -ENOTFOUND !)
 */

pthread_t ec_thread_getpid(char *name)
{
   struct thread_list *current;
   pthread_t pid;
   
   THREADS_LOCK;

   LIST_FOREACH(current, &thread_list_head, next) {
      if (!strcasecmp(current->t.name,name)) {
         pid = current->t.id;
         THREADS_UNLOCK;
         return pid;
      }
   }

   THREADS_UNLOCK;
  
   return EC_PTHREAD_NULL;
}

/* returns the description of a thread */

char * ec_thread_getdesc(pthread_t id)
{
   struct thread_list *current;
   char *desc;

   if (pthread_equal(id, EC_PTHREAD_SELF))
      id = pthread_self();
  
   THREADS_LOCK;
   
   LIST_FOREACH(current, &thread_list_head, next) {
      if (pthread_equal(current->t.id, id)) {
         desc = current->t.description;
         THREADS_UNLOCK;
         return desc;
      }
   }
   
   THREADS_UNLOCK;
   
   return "";
}


/* add a thread in the thread list */
void ec_thread_register(pthread_t id, char *name, char *desc)
{
   ec_thread_register_detached(id, name, desc, JOINABLE_THREAD);
}

void ec_thread_register_detached(pthread_t id, char *name, char *desc, int detached)
{
   struct thread_list *current, *newelem;

   if (pthread_equal(id, EC_PTHREAD_SELF))
      id = pthread_self();
   
   DEBUG_MSG("ec_thread_register -- [%lu] %s", PTHREAD_ID(id), name);

   SAFE_CALLOC(newelem, 1, sizeof(struct thread_list));
              
   newelem->t.id = id;
   newelem->t.name = strdup(name);
   newelem->t.description = strdup(desc);
   newelem->t.detached = detached;

   THREADS_LOCK;
   
   LIST_FOREACH(current, &thread_list_head, next) {
      if (pthread_equal(current->t.id, id)) {
         SAFE_FREE(current->t.name);
         SAFE_FREE(current->t.description);
         LIST_REPLACE(current, newelem, next);
         SAFE_FREE(current);
         THREADS_UNLOCK;
         return;
      }
   }

   LIST_INSERT_HEAD(&thread_list_head, newelem, next);
   
   THREADS_UNLOCK;
   
}

/*
 * creates a new thread on the given function
 */

pthread_t ec_thread_new(char *name, char *desc, void *(*function)(void *), void *args) {
   return ec_thread_new_detached(name, desc, function, args, JOINABLE_THREAD);
}

pthread_t ec_thread_new_detached(char *name, char *desc, void *(*function)(void *), void *args, int detached)
{
   pthread_t id;
   int e;

   DEBUG_MSG("ec_thread_new -- %s detached %d", name, detached);

   /* 
    * lock the mutex to syncronize with the new thread.
    * the newly created thread will call ec_thread_init(),
    * so at the end of this function we are sure that the 
    * thread had be initialized
    */
   INIT_LOCK; 

   if (detached == DETACHED_THREAD) {
      pthread_attr_t attr;
      pthread_attr_init(&attr);
      pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
      if ((e = pthread_create(&id, &attr, function, args) != 0))
         ERROR_MSG("not enough resources to create a new thread in this process: %s", strerror(e));
   }else {
      if ((e = pthread_create(&id, NULL, function, args) != 0))
         ERROR_MSG("not enough resources to create a new thread in this process: %s", strerror(e));
   }

   ec_thread_register_detached(id, name, desc, detached);

   DEBUG_MSG("ec_thread_new -- %lu created ", PTHREAD_ID(id));

   if ((e = pthread_cond_wait(&init_cond, &init_mtx)))
      ERROR_MSG("waiting on init_cond: %s", strerror(e));
   INIT_UNLOCK;
   
   return id;
}

/* 
 * set the state of a thread 
 * all the new thread MUST call this on startup
 */
void ec_thread_init(void)
{
   pthread_t id = pthread_self();
   int e;
   
   DEBUG_MSG("ec_thread_init -- %lu", PTHREAD_ID(id));

   INIT_LOCK;
   
   /* 
    * allow a thread to be cancelled as soon as the
    * cancellation  request  is received
    */
   pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
   pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

   /* sync with the creator */ 
   if ((e = pthread_cond_signal(&init_cond)))
      ERROR_MSG("raising init_cond: %s", strerror(e));
   INIT_UNLOCK;
   
   DEBUG_MSG("ec_thread_init -- (%lu) ready and syncronized",  PTHREAD_ID(id));
}

/*
 * destroy a thread in the list
 */
void ec_thread_destroy(pthread_t id)
{
   struct thread_list *current;

   if (pthread_equal(id, EC_PTHREAD_SELF))
      id = pthread_self();
   
   DEBUG_MSG("ec_thread_destroy -- terminating %lu [%s]", PTHREAD_ID(id), ec_thread_getname(id));


   /* send the cancel signal to the thread */
   pthread_cancel((pthread_t)id);


   DEBUG_MSG("ec_thread_destroy -- [%s] terminated", ec_thread_getname(id));
   
   THREADS_LOCK;
   
   LIST_FOREACH(current, &thread_list_head, next) {
      if (pthread_equal(current->t.id, id)) {
#ifndef BROKEN_PTHREAD_JOIN
         if (!current->t.detached) {
            DEBUG_MSG("ec_thread_destroy: pthread_join");
            /* wait until it has finished */
            pthread_join((pthread_t)id, NULL);
         }
#endif         
         SAFE_FREE(current->t.name);
         SAFE_FREE(current->t.description);
         LIST_REMOVE(current, next);
         SAFE_FREE(current);
         THREADS_UNLOCK;
         return;
      }
   }

   THREADS_UNLOCK;

}


/*
 * kill all the registerd thread but
 * the calling one
 */

void ec_thread_kill_all(void)
{
   struct thread_list *current, *old;
   pthread_t id = pthread_self();

   DEBUG_MSG("ec_thread_kill_all -- caller %lu [%s]", PTHREAD_ID(id), ec_thread_getname(id));

   THREADS_LOCK;

#ifdef OS_WINDOWS
   /* prevent hanging UI. Not sure how this works, but it does... */
   if (GBL_PCAP->pcap)
      ec_win_pcap_stop(GBL_PCAP->pcap);
#endif
   
   LIST_FOREACH_SAFE(current, &thread_list_head, next, old) {
      /* skip ourself */
      if (!pthread_equal(current->t.id, id)) {
         DEBUG_MSG("ec_thread_kill_all -- terminating %lu [%s]", PTHREAD_ID(current->t.id), current->t.name);

         /* send the cancel signal to the thread */
         pthread_cancel((pthread_t)current->t.id);
         
#ifndef BROKEN_PTHREAD_JOIN
         if (!current->t.detached) {
            DEBUG_MSG("ec_thread_destroy: pthread_join");
            /* wait until it has finished */
            pthread_join(current->t.id, NULL);
         }
#endif         

         DEBUG_MSG("ec_thread_kill_all -- [%s] terminated", current->t.name);
      
         SAFE_FREE(current->t.name);
         SAFE_FREE(current->t.description);
         LIST_REMOVE(current, next);
         SAFE_FREE(current);
      }
   }
   
   THREADS_UNLOCK;
}

/*
 * used by a thread that wants to terminate itself
 */
void ec_thread_exit(void)
{
   struct thread_list *current, *old;
   pthread_t id = pthread_self();

   DEBUG_MSG("ec_thread_exit -- caller %lu [%s]", PTHREAD_ID(id), ec_thread_getname(id));

   THREADS_LOCK;
   
   LIST_FOREACH_SAFE(current, &thread_list_head, next, old) {
      /* delete our entry */
      if (pthread_equal(current->t.id, id)) {

      /* thread is attempting to shut down on its own, check and see if the thread is detached,
         if not set is as a detached thread since when a thread calls this method, there is no thread
         that will do the pthread_join to force it to release all of its resources */
         if (!current->t.detached) {
            pthread_detach(id);
         }

         SAFE_FREE(current->t.name);
         SAFE_FREE(current->t.description);
         LIST_REMOVE(current, next);
         SAFE_FREE(current);
      }
   }

   THREADS_UNLOCK;

   /* perform a clean exit of the thread */
   pthread_exit(0);
   
}

/* EOF */

// vim:ts=3:expandtab

