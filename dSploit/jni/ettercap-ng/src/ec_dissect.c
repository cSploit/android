/*
    ettercap -- dissector module

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

    $Id: ec_dissect.c,v 1.27 2004/10/30 13:19:41 lordnaga Exp $
*/

#include <ec.h>
#include <ec_dissect.h>
#include <ec_packet.h>
#include <ec_sslwrap.h>

/* globals */

static SLIST_HEAD (, dissect_entry) dissect_list;

struct dissect_entry {
   char *name;
   u_int32 type;
   u_int8 level;
   FUNC_DECODER_PTR(decoder);
   SLIST_ENTRY (dissect_entry) next;
};

/* protos */

void dissect_add(char *name, u_int8 level, u_int32 port, FUNC_DECODER_PTR(decoder));
void dissect_del(char *name);
int dissect_modify(int mode, char *name, u_int32 port);

int dissect_match(void *id_sess, void *id_curr);
void dissect_create_session(struct ec_session **s, struct packet_object *po, u_int32 code);
size_t dissect_create_ident(void **i, struct packet_object *po, u_int32 code);            
void dissect_wipe_session(struct packet_object *po, u_int32 code);

int dissect_on_port(char *name, u_int16 port);

/*******************************************/

/*
 * compare two session ident
 *
 * return 1 if it matches
 */

int dissect_match(void *id_sess, void *id_curr)
{
   struct dissect_ident *ids = id_sess;
   struct dissect_ident *id = id_curr;

   /* sanity check */
   BUG_IF(ids == NULL);
   BUG_IF(id == NULL);
  
   /* 
    * is this ident from our level ?
    * check the magic !
    */
   if (ids->magic != id->magic)
      return 0;
   
   /* check the protocol */
   if (ids->L4_proto != id->L4_proto)
      return 0;

   /* from source to dest */
   if (ids->L4_src == id->L4_src &&
       ids->L4_dst == id->L4_dst &&
       !ip_addr_cmp(&ids->L3_src, &id->L3_src) &&
       !ip_addr_cmp(&ids->L3_dst, &id->L3_dst) )
      return 1;
   
   /* from dest to source */
   if (ids->L4_src == id->L4_dst &&
       ids->L4_dst == id->L4_src &&
       !ip_addr_cmp(&ids->L3_src, &id->L3_dst) &&
       !ip_addr_cmp(&ids->L3_dst, &id->L3_src) )
      return 1;

   return 0;
}


/*
 * prepare the ident and the pointer to match function
 * for a dissector.
 */

void dissect_create_session(struct ec_session **s, struct packet_object *po, u_int32 code)
{
   void *ident;

   DEBUG_MSG("dissect_create_session");
   
   /* allocate the session */
   SAFE_CALLOC(*s, 1, sizeof(struct ec_session));
   
   /* create the ident */
   (*s)->ident_len = dissect_create_ident(&ident, po, code);
   
   /* link to the session */
   (*s)->ident = ident;

   /* the matching function */
   (*s)->match = &dissect_match;
}

/*
 * create the ident for a session
 */

size_t dissect_create_ident(void **i, struct packet_object *po, u_int32 code)
{
   struct dissect_ident *ident;
   
   /* allocate the ident for that session */
   SAFE_CALLOC(ident, 1, sizeof(struct dissect_ident));
   
   /* the magic number (usually the pointer for the function) */
   ident->magic = code;
      
   /* prepare the ident */
   memcpy(&ident->L3_src, &po->L3.src, sizeof(struct ip_addr));
   memcpy(&ident->L3_dst, &po->L3.dst, sizeof(struct ip_addr));
   
   ident->L4_proto = po->L4.proto;
   
   ident->L4_src = po->L4.src;
   ident->L4_dst = po->L4.dst;

   /* return the ident */
   *i = ident;

   /* return the lenght of the ident */
   return sizeof(struct dissect_ident);
}

/*
 * totally destroy the session bound to this connection
 */
void dissect_wipe_session(struct packet_object *po, u_int32 code)
{
   void *ident;
   struct ec_session *s;   

   DEBUG_MSG("dissect_wipe_session");
   
   /* create an ident to retrieve the session */
   dissect_create_ident(&ident, po, code);

   /* retrieve the session and delete it */
   if (session_get_and_del(&s, ident, DISSECT_IDENT_LEN) == -ENOTFOUND) {
      SAFE_FREE(ident);
      return;
   }

   /* free the session */
   session_free(s);
   SAFE_FREE(ident);
}

/*
 * register a dissector in the dissectors list
 * and add it to the decoder list.
 * this list is used by dissect_modify during the parsing
 * of etter.conf to enable/disable the dissectors via their name.
 */
void dissect_add(char *name, u_int8 level, u_int32 port, FUNC_DECODER_PTR(decoder))
{
   struct dissect_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct dissect_entry));
   
   e->name = strdup(name);
   e->level = level;
   e->type = port;
   e->decoder = decoder;

   SLIST_INSERT_HEAD (&dissect_list, e, next); 

   /* add the default decoder */
   add_decoder(level, port, decoder);
      
   return;
}

/*
 * remove all istances of a dissector in the dissectors list
 */
void dissect_del(char *name)
{
   struct dissect_entry *e, *tmp = NULL;

   SLIST_FOREACH_SAFE(e, &dissect_list, next, tmp) {
      if (!strcasecmp(e->name, name)) {
         del_decoder(e->level, e->type);
         SLIST_REMOVE(&dissect_list, e, dissect_entry, next);
         SAFE_FREE(e);
      }
   }
   
   return;
}

/*
 * given the name of the dissector add or remove it 
 * from the decoders' table.
 * is it possible to add multiple port with MODE_ADD
 */
int dissect_modify(int mode, char *name, u_int32 port)
{
   struct dissect_entry *e;
   int level;
   void *decoder;

   SLIST_FOREACH (e, &dissect_list, next) {
      if (!strcasecmp(e->name, name)) {
         switch (mode) {
            case MODE_ADD:
               DEBUG_MSG("dissect_modify: %s added on %lu", name, (unsigned long)port);
               /* add in the lists */
               dissect_add(e->name, e->level, port, e->decoder);
               return ESUCCESS;
               break;
            case MODE_REP:

               /* save them because the dissect_del may delete this values */
               level = e->level;
               decoder = e->decoder;
               
               /* remove all the previous istances */
               dissect_del(name);

	       /* move the ssl wrapper (even if no wrapper) */
               sslw_dissect_move(name, port);
               
               /* a value of 0 will disable the dissector */
               if (port == 0) {
                  DEBUG_MSG("dissect_modify: %s disabled", name);
                  return ESUCCESS;
               }
              
               DEBUG_MSG("dissect_modify: %s replaced to %lu", name, (unsigned long)port);
               
               /* add the new value */
               dissect_add(name, level, port, decoder);	       
	       
               return ESUCCESS;
               break;
         }
      }
   }

   return -ENOTFOUND;
}

/*
 * return ESUCCESS if the dissector is on
 * the specified port 
 */
int dissect_on_port(char *name, u_int16 port)
{
   struct dissect_entry *e;

   /* 
    * return ESUCCESS if at least one port is bound 
    * to the dissector name
    */
   SLIST_FOREACH (e, &dissect_list, next) {
      if (!strcasecmp(e->name, name) && e->type == port) {
         return ESUCCESS;
      } 
   }
   
   return -ENOTFOUND;
}

/*
 * return ESUCCESS if the dissector is on
 * the specified port of the specified protocol (TCP or UDP)
 */
int dissect_on_port_level(char *name, u_int16 port, u_int8 level)
{
   struct dissect_entry *e;

   /* 
    * return ESUCCESS if at least one port is bound 
    * to the dissector name
    */
   SLIST_FOREACH (e, &dissect_list, next) {
      if (!strcasecmp(e->name, name) && e->type == port && e->level == level) {
         return ESUCCESS;
      } 
   }
   
   return -ENOTFOUND;
}


/* EOF */

// vim:ts=3:expandtab

