/*
    ettercap -- session management

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

    $Id: ec_session.c,v 1.24 2004/04/18 10:02:02 alor Exp $
*/

#include <ec.h>
#include <ec_packet.h>
#include <ec_threads.h>
#include <ec_session.h>

#include <signal.h>

#define TABBIT    10             /* 2^10 bit tab entries: 1024 LISTS */
#define TABSIZE   (1 << TABBIT)
#define TABMASK   (TABSIZE - 1)

/* globals */

struct session_list {
   time_t ts;
   struct ec_session *s;
   LIST_ENTRY (session_list) next;
};

/* global data */

static LIST_HEAD(, session_list) session_list_head[TABSIZE];

/* protos */

void session_put(struct ec_session *s);
int session_get(struct ec_session **s, void *ident, size_t ident_len);
int session_del(void *ident, size_t ident_len);
int session_get_and_del(struct ec_session **s, void *ident, size_t ident_len);
u_int32 session_hash(void *ident, size_t ilen);

void session_free(struct ec_session *s);

static pthread_mutex_t session_mutex = PTHREAD_MUTEX_INITIALIZER;
#define SESSION_LOCK     do{ pthread_mutex_lock(&session_mutex); } while(0)
#define SESSION_UNLOCK   do{ pthread_mutex_unlock(&session_mutex); } while(0)

/************************************************/

/*
 * create a session if it does not exits
 * update a session if it already exists
 *
 * also check for timeouted session and remove them
 */

void session_put(struct ec_session *s)
{
   struct session_list *sl, *tmp = NULL;
   time_t ti = time(NULL);
   u_int32 h;

   SESSION_LOCK;
   
   /* calculate the hash */
   h = session_hash(s->ident, s->ident_len);
   
   /* search if it already exist */
   LIST_FOREACH_SAFE(sl, &session_list_head[h], next, tmp) {
      if ( sl->s->match(sl->s->ident, s->ident) ) {

         DEBUG_MSG("session_put: [%p] updated", sl->s->ident);
         /* destroy the old session */
         session_free(sl->s);
         /* link the new session */
         sl->s = s;
         /* renew the timestamp */
         sl->ts = ti;
         
         SESSION_UNLOCK;
         return;
      }

      if (sl->ts < (ti - GBL_CONF->connection_timeout) ) {
         DEBUG_MSG("session_put: [%p] timeouted", sl->s->ident);
         session_free(sl->s);
         LIST_REMOVE(sl, next);
         SAFE_FREE(sl);
      }
   }
   
   /* sanity check */
   BUG_IF(s->match == NULL);
  
   /* create the element in the list */
   SAFE_CALLOC(sl, 1, sizeof(struct session_list));
   
   /* the timestamp */
   sl->ts = ti;

   /* link the session */
   sl->s = s;
   
   DEBUG_MSG("session_put: [%p] new session", sl->s->ident);

   /* 
    * put it in the head.
    * it is likely to be retrived early
    */
   LIST_INSERT_HEAD(&session_list_head[h], sl, next);

   SESSION_UNLOCK;
  
}


/*
 * get the info contained in a session
 */

int session_get(struct ec_session **s, void *ident, size_t ident_len)
{
   struct session_list *sl;
   time_t ti = time(NULL);
   u_int32 h;

   SESSION_LOCK;
   
   /* calculate the hash */
   h = session_hash(ident, ident_len);

   /* search if it already exist */
   LIST_FOREACH(sl, &session_list_head[h], next) {
      if ( sl->s->match(sl->s->ident, ident) ) {
   
         //DEBUG_MSG("session_get: [%p]", sl->s->ident);
         /* return the session */
         *s = sl->s;
         
         /* renew the timestamp */
         sl->ts = ti;

         SESSION_UNLOCK;
         return ESUCCESS;
      }
   }
   
   SESSION_UNLOCK;
   
   return -ENOTFOUND;
}


/*
 * delete a session
 */

int session_del(void *ident, size_t ident_len)
{
   struct session_list *sl;
   u_int32 h;

   SESSION_LOCK;
   
   /* calculate the hash */
   h = session_hash(ident, ident_len);
   
   /* search if it already exist */
   LIST_FOREACH(sl, &session_list_head[h], next) {
      if ( sl->s->match(sl->s->ident, ident) ) {
         
         DEBUG_MSG("session_del: [%p]", sl->s->ident);

         /* free the session */
         session_free(sl->s);
         /* remove the element in the list */
         LIST_REMOVE(sl, next);
         /* free the element in the list */
         SAFE_FREE(sl);

         SESSION_UNLOCK;
         return ESUCCESS;
      }
   }
   
   SESSION_UNLOCK;
   
   return -ENOTFOUND;
}


/*
 * get the info and delete the session
 * atomic operations
 */

int session_get_and_del(struct ec_session **s, void *ident, size_t ident_len)
{
   struct session_list *sl;
   u_int32 h;

   SESSION_LOCK;
   
   /* calculate the hash */
   h = session_hash(ident, ident_len);
   
   /* search if it already exist */
   LIST_FOREACH(sl, &session_list_head[h], next) {
      if ( sl->s->match(sl->s->ident, ident) ) {
         
         DEBUG_MSG("session_get_and_del: [%p]", sl->s->ident);
         
         /* return the session */
         *s = sl->s;
         /* remove the element in the list */
         LIST_REMOVE(sl, next);
         /* free the element in the list */
         SAFE_FREE(sl);

         SESSION_UNLOCK;
         return ESUCCESS;
      }
   }
   
   SESSION_UNLOCK;
   
   return -ENOTFOUND;
}

/*
 * free a session structure
 */

void session_free(struct ec_session *s)
{
   SAFE_FREE(s->ident);
   /* call the cleanup function to free pointers in the data portion */
   if (s->free)
      s->free(s->data, s->data_len);
   /* data is free'd here, don't free it in the s->free function */
   SAFE_FREE(s->data);
   SAFE_FREE(s);
}

/*
 * calculate the hash for an ident.
 * use a IP-like checksum so if some word will be exchanged
 * the hash will be the same. it is useful for dissectors
 * to find the same session if the packet is goint to the
 * server or to the client.
 */
u_int32 session_hash(void *ident, size_t ilen)
{
   u_int32 hash = 0;
   u_int16 *buf = (u_int16 *)ident;

   while(ilen > 1) {
      hash += *buf++;
      ilen -= sizeof(u_int16);
   }

   if (ilen == 1) 
      hash += htons(*(u_char *)buf << 8);

   hash = (hash >> 16) + (hash & 0xffff);
   hash += (hash >> 16);

   /* the hash must be within the TABSIZE */
   return (u_int16)(~hash) & TABMASK;
}

/* EOF */

// vim:ts=3:expandtab

