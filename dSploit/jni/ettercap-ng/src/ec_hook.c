/*
    ettercap -- hook points handling

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

    $Id: ec_hook.c,v 1.10 2004/05/20 10:06:21 alor Exp $
*/

#include <ec.h>
#include <ec_hook.h>
#include <ec_packet.h>

#include <pthread.h>

struct hook_list {
   int point;
   void (*func)(struct packet_object *po);
   LIST_ENTRY (hook_list) next;
};

/* global data */

/* the list for the HOOK_* */
static LIST_HEAD(, hook_list) hook_list_head;
/* the list for the PACKET_* */
static LIST_HEAD(, hook_list) hook_pck_list_head;

pthread_mutex_t hook_mutex = PTHREAD_MUTEX_INITIALIZER;
#define HOOK_LOCK     do{ pthread_mutex_lock(&hook_mutex); } while(0)
#define HOOK_UNLOCK   do{ pthread_mutex_unlock(&hook_mutex); } while(0)
   
pthread_mutex_t hook_pck_mutex = PTHREAD_MUTEX_INITIALIZER;
#define HOOK_PCK_LOCK     do{ pthread_mutex_lock(&hook_pck_mutex); } while(0)
#define HOOK_PCK_UNLOCK   do{ pthread_mutex_unlock(&hook_pck_mutex); } while(0)

/* protos... */

void hook_point(int point, struct packet_object *po);
void hook_add(int point, void (*func)(struct packet_object *po) );
int hook_del(int point, void (*func)(struct packet_object *po) );

/*******************************************/

/* execute the functions registered in that hook point */

void hook_point(int point, struct packet_object *po)
{
   struct hook_list *current;

   /* the hook is for a HOOK_PACKET_* type */
   if (point > HOOK_PACKET_BASE) {
      
      HOOK_PCK_LOCK;
   
      LIST_FOREACH(current, &hook_pck_list_head, next) 
         if (current->point == point)
            current->func(po);
   
      HOOK_PCK_UNLOCK;
   
   } else {
   
      HOOK_LOCK;
   
      LIST_FOREACH(current, &hook_list_head, next) 
         if (current->point == point)
            current->func(po);
   
      HOOK_UNLOCK;
   }
   
   return;
}


/* add a function to an hook point */

void hook_add(int point, void (*func)(struct packet_object *po) )
{
   struct hook_list *newelem;

   SAFE_CALLOC(newelem, 1, sizeof(struct hook_list));
              
   newelem->point = point;
   newelem->func = func;

   /* the hook is for a HOOK_PACKET_* type */
   if (point > HOOK_PACKET_BASE) {
      HOOK_PCK_LOCK;
      LIST_INSERT_HEAD(&hook_pck_list_head, newelem, next);
      HOOK_PCK_UNLOCK;
   } else {
      HOOK_LOCK;
      LIST_INSERT_HEAD(&hook_list_head, newelem, next);
      HOOK_UNLOCK;
   }
   
}

/* remove a function from an hook point */

int hook_del(int point, void (*func)(struct packet_object *po) )
{
   struct hook_list *current;


   /* the hook is for a HOOK_PACKET_* type */
   if (point > HOOK_PACKET_BASE) {
      HOOK_PCK_LOCK;
   
      LIST_FOREACH(current, &hook_pck_list_head, next) {
         if (current->point == point && current->func == func) {
            LIST_REMOVE(current, next);
            SAFE_FREE(current);
            HOOK_PCK_UNLOCK;
            DEBUG_MSG("hook_del -- %d [%p]", point, func);
            return ESUCCESS;
         }
      }
 
      HOOK_PCK_UNLOCK;
   } else {
      HOOK_LOCK;
   
      LIST_FOREACH(current, &hook_list_head, next) {
         if (current->point == point && current->func == func) {
            LIST_REMOVE(current, next);
            SAFE_FREE(current);
            HOOK_UNLOCK;
            DEBUG_MSG("hook_del -- %d [%p]", point, func);
            return ESUCCESS;
         }
      }
 
      HOOK_UNLOCK;
   }
   
   return -ENOTFOUND;
}


/* EOF */

// vim:ts=3:expandtab

