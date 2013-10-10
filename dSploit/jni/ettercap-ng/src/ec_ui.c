/*
    ettercap -- user interface stuff

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

    $Id: ec_ui.c,v 1.30 2004/09/28 13:50:37 alor Exp $
*/

#include <ec.h>
#include <ec_ui.h>

#include <stdarg.h>
#include <pthread.h>

/* globals */

struct ui_message {
   char *message;
   STAILQ_ENTRY(ui_message) next;
};

static STAILQ_HEAD(, ui_message) messages_queue = STAILQ_HEAD_INITIALIZER(messages_queue);

/* global mutex on interface */

static pthread_mutex_t ui_msg_mutex = PTHREAD_MUTEX_INITIALIZER;
#define UI_MSG_LOCK     do{ pthread_mutex_lock(&ui_msg_mutex); }while(0)
#define UI_MSG_UNLOCK   do{ pthread_mutex_unlock(&ui_msg_mutex); }while(0)

/* protos... */

void ui_init(void);
void ui_start(void);
void ui_cleanup(void);
void ui_msg(const char *fmt, ...);
void ui_error(const char *fmt, ...);
void ui_fatal_error(const char *msg);
void ui_input(const char *title, char *input, size_t n, void (*callback)(void));
int ui_progress(char *title, int value, int max);
int ui_msg_flush(int max);
int ui_msg_purge_all(void);
void ui_register(struct ui_ops *ops);


/*******************************************/

/* called to initialize the user interface */

void ui_init(void)
{
   DEBUG_MSG("ui_init");

   EXECUTE(GBL_UI->init);

   GBL_UI->initialized = 1;
}

/* called to run the user interface */

void ui_start(void)
{
   DEBUG_MSG("ui_start");

   if (GBL_UI->initialized)
      EXECUTE(GBL_UI->start);
   else
      DEBUG_MSG("ui_start called initialized");
}

/* called to end the user interface */

void ui_cleanup(void)
{
   if (GBL_UI->initialized) {
      DEBUG_MSG("ui_cleanup");
      EXECUTE(GBL_UI->cleanup);
      GBL_UI->initialized = 0;
   }
}

/* implement the progress bar */
int ui_progress(char *title, int value, int max)
{
   if (GBL_UI->progress)
      return GBL_UI->progress(title, value, max);

   return UI_PROGRESS_UPDATED;
}


/*
 * the FATAL_MSG error handling function
 */
void ui_error(const char *fmt, ...)
{
   va_list ap;
   int n;
   size_t size = 50;
   char *msg;

   /* 
    * we hope the message is shorter
    * than 'size', else realloc it
    */
    
   SAFE_CALLOC(msg, size, sizeof(char));

   while (1) {
      /* Try to print in the allocated space. */
      va_start(ap, fmt);
      n = vsnprintf (msg, size, fmt, ap);
      va_end(ap);
      
      /* If that worked, we have finished. */
      if (n > -1 && (size_t)n < size)
         break;
   
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
         size = n+1; /* precisely what is needed */
      else           /* glibc 2.0 */
         size *= 2;  /* twice the old size */
      
      SAFE_REALLOC(msg, size);
   }

   /* dump the error in the debug file */
   DEBUG_MSG("%s", msg);
   
   /* call the function */
   if (GBL_UI->error)
      EXECUTE(GBL_UI->error, msg);
   /* the interface is not yet initialized */
   else
      fprintf(stderr, "\n%s\n", msg);
   
   /* free the message */
   SAFE_FREE(msg);
}


/*
 * the FATAL_ERROR error handling function
 */
void ui_fatal_error(const char *msg)
{
   /* 
    * call the function 
    * make sure that the globals have been alloc'd
    */
   if (GBLS && GBL_UI && GBL_UI->fatal_error && GBL_UI->initialized)
      EXECUTE(GBL_UI->fatal_error, msg);
   /* the interface is not yet initialized */
   else {
      fprintf(stderr, "\n%s\n\n", msg);
      exit(-1);
   }
   
}


/*
 * this fuction enqueues the messages displayed by
 * ui_msg_flush()
 */

void ui_msg(const char *fmt, ...)
{
   va_list ap;
   struct ui_message *msg;
   int n;
   size_t size = 50;

   SAFE_CALLOC(msg, 1, sizeof(struct ui_message));

   /* 
    * we hope the message is shorter
    * than 'size', else realloc it
    */
    
   SAFE_CALLOC(msg->message, size, sizeof(char));

   while (1) {
      /* Try to print in the allocated space. */
      va_start(ap, fmt);
      n = vsnprintf (msg->message, size, fmt, ap);
      va_end(ap);
      
      /* If that worked, we have finished. */
      if (n > -1 && (size_t)n < size)
         break;
   
      /* Else try again with more space. */
      if (n > -1)    /* glibc 2.1 */
         size = n+1; /* precisely what is needed */
      else           /* glibc 2.0 */
         size *= 2;  /* twice the old size */
      
      SAFE_REALLOC(msg->message, size);
   }

   /* log the messages if needed */
   if (GBL_OPTIONS->msg_fd) {
      fprintf(GBL_OPTIONS->msg_fd, "%s", msg->message);
      fflush(GBL_OPTIONS->msg_fd);
   }
   
   /* 
    * MUST use the mutex.
    * this MAY be a different thread !!
    */
   UI_MSG_LOCK;
   
   /* add the message to the queue */
   STAILQ_INSERT_TAIL(&messages_queue, msg, next);

   UI_MSG_UNLOCK;
   
}


/*
 * get the user input
 */
void ui_input(const char *title, char *input, size_t n, void (*callback)(void))
{
   DEBUG_MSG("ui_input");

   EXECUTE(GBL_UI->input, title, input, n, callback);
}

/* 
 * this function is used to display up to 'max' messages.
 * a user interface MUST use this to empty the message queue.
 */

int ui_msg_flush(int max)
{
   int i = 0;
   struct ui_message *msg;
   
   /* the queue is updated by other threads */
   UI_MSG_LOCK;
   
   /* sanity check */
   if (!GBL_UI->initialized)
      return 0;
      
   while ( (msg = STAILQ_FIRST(&messages_queue)) != NULL) {

      /* diplay the message */
      GBL_UI->msg(msg->message);

      STAILQ_REMOVE_HEAD(&messages_queue, msg, next);
      /* free the message */
      SAFE_FREE(msg->message);
      SAFE_FREE(msg);
      
      /* do not display more then 'max' messages */
      if (++i == max)
         break;
   }
   
   UI_MSG_UNLOCK;
   
   /* returns the number of displayed messages */
   return i;
   
}

/*
 * empty all the message queue
 */
int ui_msg_purge_all(void)
{
   int i = 0;
   struct ui_message *msg;

   /* the queue is updated by other threads */
   UI_MSG_LOCK;
      
   while ( (msg = STAILQ_FIRST(&messages_queue)) != NULL) {
      STAILQ_REMOVE_HEAD(&messages_queue, msg, next);
      /* free the message */
      SAFE_FREE(msg->message);
      SAFE_FREE(msg);
   }
   
   UI_MSG_UNLOCK;
   
   /* returns the number of purgeded messages */
   return i;
   
}

/*
 * register the function pointer for
 * the user interface.
 * a new user interface MUST implement this 
 * three function and use this function 
 * to hook in the right place.
 */

void ui_register(struct ui_ops *ops)
{
        
   BUG_IF(ops->init == NULL);
   GBL_UI->init = ops->init;
   
   BUG_IF(ops->cleanup == NULL);
   GBL_UI->cleanup = ops->cleanup;
   
   BUG_IF(ops->start == NULL);
   GBL_UI->start = ops->start;
        
   BUG_IF(ops->msg == NULL);
   GBL_UI->msg = ops->msg;
   
   BUG_IF(ops->error == NULL);
   GBL_UI->error = ops->error;
   
   BUG_IF(ops->fatal_error == NULL);
   GBL_UI->fatal_error = ops->fatal_error;
   
   BUG_IF(ops->input == NULL);
   GBL_UI->input = ops->input;
   
   BUG_IF(ops->progress == NULL);
   GBL_UI->progress = ops->progress;

   GBL_UI->type = ops->type;
}


/* EOF */

// vim:ts=3:expandtab

