/*
    etterlog -- create, search and manipulate streams

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

    $Id: el_stream.c,v 1.7 2004/11/04 10:37:16 alor Exp $
*/


#include <el.h>
#include <el_functions.h>

/* protos... */

void stream_init(struct stream_object *so);
int stream_add(struct stream_object *so, struct log_header_packet *pck, char *buf);
int stream_read(struct stream_object *so, u_char *buf, size_t size, int mode);
int stream_move(struct stream_object *so, size_t offset, int whence, int mode);
struct so_list * stream_search(struct stream_object *so, u_char *buf, size_t buflen, int mode);
   
/*******************************************/

/*
 * initialize a stream object
 */
void stream_init(struct stream_object *so)
{
   TAILQ_INIT(&so->so_head);
   so->side1.po_off = 0;
   so->side2.po_off = 0;
   so->side1.so_curr = TAILQ_FIRST(&so->so_head);
   so->side2.so_curr = TAILQ_FIRST(&so->so_head);
}

/*
 * add a packet to a stream
 */
int stream_add(struct stream_object *so, struct log_header_packet *pck, char *buf)
{
   struct so_list *pl, *tmp;

   /* skip ack packet or zero lenght packet */
   if (pck->len == 0)
      return 0;

   /* the packet is good, add it */
   SAFE_CALLOC(pl, 1, sizeof(struct so_list));

   /* create the packet object */
   memcpy(&pl->po.L3.src, &pck->L3_src, sizeof(struct ip_addr));
   memcpy(&pl->po.L3.dst, &pck->L3_dst, sizeof(struct ip_addr));
   
   pl->po.L4.src = pck->L4_src;
   pl->po.L4.dst = pck->L4_dst;
   pl->po.L4.proto = pck->L4_proto;
  
   SAFE_CALLOC(pl->po.DATA.data, pck->len, sizeof(char));
   
   memcpy(pl->po.DATA.data, buf, pck->len);
   pl->po.DATA.len = pck->len;
   
   /* set the stream direction */

   /* this is the first packet in the stream */
   if (TAILQ_FIRST(&so->so_head) == TAILQ_END(&so->so_head)) {
      pl->side = STREAM_SIDE1;
      /* init the pointer to the first packet */
      so->side1.so_curr = pl;
      so->side2.so_curr = pl;
   /* check the previous one and set it accordingly */
   } else {
      tmp = TAILQ_LAST(&so->so_head, so_list_head);
      if (!ip_addr_cmp(&tmp->po.L3.src, &pl->po.L3.src))
         /* same direction */
         pl->side = tmp->side;
      else 
         /* change detected */
         pl->side = (tmp->side == STREAM_SIDE1) ? STREAM_SIDE2 : STREAM_SIDE1;
   }
      
   /* add to the queue */
   TAILQ_INSERT_TAIL(&so->so_head, pl, next);

   return pck->len;
}


/*
 * read data from the stream
 * mode can be: 
 *    STREAM_SIDE1 reads only from the first side (usually client to server)
 *    STREAM_SIDE2 reads only from the other side
 */
int stream_read(struct stream_object *so, u_char *buf, size_t size, int mode)
{
   size_t buf_off = 0;
   size_t tmp_size = 0;
   
   struct so_list *so_curr = NULL;
   size_t po_off = 0;

   /* get the values into temp variable */
   switch (mode) {
      case STREAM_SIDE1:
         so_curr = so->side1.so_curr;
         po_off = so->side1.po_off;
         break;
      case STREAM_SIDE2:
         so_curr = so->side2.so_curr;
         po_off = so->side2.po_off;
         break;
   }
   
   /* search the first packet matching the selected mode */
   while (so_curr->side != mode) {
      so_curr = TAILQ_NEXT(so_curr, next);
      /* don't go after the end of the stream */
      if (so_curr == TAILQ_END(&so->pl_head))
         return 0;
   }

   /* size is decremented while it is copied in the buffer */
   while (size) {

      /* get the size to be copied from the current po */
      tmp_size = (so_curr->po.DATA.len - po_off < size) ? so_curr->po.DATA.len - po_off : size;

      /* fill the buffer */
      memcpy(buf + buf_off, so_curr->po.DATA.data + po_off, tmp_size);
      
      /* the offset is the portion of the data copied into the buffer */
      po_off += tmp_size;

      /* update the pointer into the buffer */
      buf_off += tmp_size;

      /* decrement the total size to be copied */
      size -= tmp_size;
      
      /* we have reached the end of the packet, go to the next one */
      if (po_off == so_curr->po.DATA.len) {
         /* search the next packet matching the selected mode */
         do {
            /* don't go after the end of the stream */
            if (TAILQ_NEXT(so_curr, next) != TAILQ_END(&so->pl_head))
               so_curr = TAILQ_NEXT(so_curr, next);
            else
               goto read_end;
            
         } while (so_curr->side != mode);

         /* reset the offset for the packet */
         po_off = 0;
      }
   }
  
read_end:   
   /* restore the value in the real stream object */
   switch (mode) {
      case STREAM_SIDE1:
         so->side1.so_curr = so_curr;
         so->side1.po_off = po_off;
         break;
      case STREAM_SIDE2:
         so->side2.so_curr = so_curr;
         so->side2.po_off = po_off;
         break;
   }
  
   /* return the total byte read */
   return buf_off;
}

/*
 * move the pointers into the stream
 */
int stream_move(struct stream_object *so, size_t offset, int whence, int mode)
{
   size_t tmp_size = 0;
   size_t move = 0;
   
   struct so_list *so_curr = NULL;
   size_t po_off = 0;

   /* get the values into temp variable */
   switch (mode) {
      case STREAM_SIDE1:
         so_curr = so->side1.so_curr;
         po_off = so->side1.po_off;
         break;
      case STREAM_SIDE2:
         so_curr = so->side2.so_curr;
         po_off = so->side2.po_off;
         break;
   }

   /* no movement */
   if (offset == 0)
      return 0;
   
   /* 
    * the offest is calculated from the beginning,
    * so move to the first packet
    */
   if (whence == SEEK_SET) {
      so_curr = TAILQ_FIRST(&so->so_head);
      po_off = 0;
   }

   /* the other mode is SEEK_CUR */

   /* search the first packet matching the selected mode */
   while (so_curr->side != mode) {
      so_curr = TAILQ_NEXT(so_curr, next);
      /* don't go after the end of the stream */
      if (so_curr == TAILQ_END(&so->pl_head))
         return 0;
   }

   while (offset) {
      /* get the lenght to jump to in the current po */
      tmp_size = (so_curr->po.DATA.len - po_off < offset) ? so_curr->po.DATA.len - po_off : offset;

      /* update the offset */
      po_off += tmp_size;

      /* decrement the total offset by the packet lenght */
      offset -= tmp_size;

      /* update the total movement */
      move += tmp_size;

      /* we have reached the end of the packet, go to the next one */
      if (po_off == so_curr->po.DATA.len) {
         /* search the next packet matching the selected mode */
         do {
            /* don't go after the end of the stream */
            if (TAILQ_NEXT(so_curr, next) != TAILQ_END(&so->pl_head))
               so_curr = TAILQ_NEXT(so_curr, next);
            else
               goto move_end;
            
         } while (so_curr->side != mode);

         /* reset the offset for the packet */
         po_off = 0;
      }
   }
   
move_end:
   /* restore the value in the real stream object */
   switch (mode) {
      case STREAM_SIDE1:
         so->side1.so_curr = so_curr;
         so->side1.po_off = po_off;
         break;
      case STREAM_SIDE2:
         so->side2.so_curr = so_curr;
         so->side2.po_off = po_off;
         break;
   }
   
   return move;
}


/*
 * search a pattern into the stream 
 * returns  - NULL if not found
 *          - the packet containing the string if found
 */
struct so_list * stream_search(struct stream_object *so, u_char *buf, size_t buflen, int mode)
{
   u_char *tmpbuf = NULL, *find;
   size_t offset = 0, len = 0;
   struct so_list *so_curr = NULL, *first;
   size_t po_off = 0;
   
   /* get the values into temp variable */
   switch (mode) {
      case STREAM_SIDE1:
         so_curr = so->side1.so_curr;
         po_off = so->side1.po_off;
         break;
      case STREAM_SIDE2:
         so_curr = so->side2.so_curr;
         po_off = so->side2.po_off;
         break;
   }

   first = so_curr;
   
   /* create the buffer from the current position to the end */ 
   for ( ; so_curr != TAILQ_END(so->so_head); so_curr = TAILQ_NEXT(so_curr, next)) {
     
      /* skip packet in the wrong side */
      if (so_curr->side != mode) {
         po_off = 0;
         continue;
      }
      
      if (so_curr == first)
         len += so_curr->po.DATA.len - po_off;
      else
         len += so_curr->po.DATA.len;
         
      
      SAFE_REALLOC(tmpbuf, len);
      
      /* 
       * add the packet to the end of the buffer 
       * containing the whole conversation 
       */
      if (so_curr == first)
         memcpy(tmpbuf, so_curr->po.DATA.data + po_off, so_curr->po.DATA.len - po_off);
      else
         memcpy(tmpbuf + len - so_curr->po.DATA.len, so_curr->po.DATA.data, so_curr->po.DATA.len);
   }

   /* the buffer is found in the conversation */
   if ((find = memmem(tmpbuf, len, buf, buflen)) != NULL) {
      offset = find - tmpbuf;
     
      SAFE_FREE(tmpbuf);

      /* move the stream pointers to the buffer found */
      stream_move(so, offset, SEEK_CUR, mode);

      switch (mode) {
         case STREAM_SIDE1:
            return so->side1.so_curr;
         case STREAM_SIDE2:
            return so->side2.so_curr;
      }
   } 

   SAFE_FREE(tmpbuf);
  
   return NULL;
}


/* EOF */


// vim:ts=3:expandtab

