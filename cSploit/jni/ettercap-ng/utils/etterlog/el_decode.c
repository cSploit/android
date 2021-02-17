/*
    etterlog -- decode a stream and extract file from it

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

    $Id: el_decode.c,v 1.7 2004/11/05 14:12:01 alor Exp $
*/


#include <el.h>
#include <el_functions.h>

#include <fcntl.h>
#ifdef HAVE_LIBGEN_H
   #include <libgen.h>
#endif

#ifdef OS_WINDOWS
   #define MKDIR(path,acc)  mkdir(path)
#else
   #define MKDIR(path,acc)  mkdir(path,acc)
#endif 

/* globals */

static SLIST_HEAD (, dec_entry) extractor_table;

struct dec_entry {
   u_int8 level;
   u_int32 type;
   FUNC_EXTRACTOR_PTR(extractor);
   SLIST_ENTRY (dec_entry) next;
};

/* protos */

int decode_stream(struct stream_object *so);

void add_extractor(u_int8 level, u_int32 type, FUNC_EXTRACTOR_PTR(extractor));
void * get_extractor(u_int8 level, u_int32 type);
int decode_to_file(char *host, char *proto, char *file);

/*******************************************/

/*
 * decode the stream 
 */
int decode_stream(struct stream_object *so)
{
   struct so_list *pl;
   FUNC_EXTRACTOR_PTR(app_extractor);
   int ret = 0;

   /* get the port used by the stream, looking at the first packet */
   pl = TAILQ_FIRST(&so->so_head);
   
   /* 
    * we should run the extractor on both the tcp/udp ports
    * since we may be interested in both client and server traffic.
    */
   switch (pl->po.L4.proto) {
      case NL_TYPE_TCP:
         app_extractor = get_extractor(APP_LAYER_TCP, ntohs(pl->po.L4.src));
         EXECUTE_EXTRACTOR(app_extractor, so, ret);
         app_extractor = get_extractor(APP_LAYER_TCP, ntohs(pl->po.L4.dst));
         EXECUTE_EXTRACTOR(app_extractor, so, ret);
         break;
         
      case NL_TYPE_UDP:
         app_extractor = get_extractor(APP_LAYER_UDP, ntohs(pl->po.L4.src));
         EXECUTE_EXTRACTOR(app_extractor, so, ret);
         app_extractor = get_extractor(APP_LAYER_UDP, ntohs(pl->po.L4.dst));
         EXECUTE_EXTRACTOR(app_extractor, so, ret);
         break;
   }
   
   /* if at least one extractor has found something ret is positive */
   return ret;
}


/*
 * add a extractor to the extractors table 
 */
void add_extractor(u_int8 level, u_int32 type, FUNC_EXTRACTOR_PTR(extractor))
{
   struct dec_entry *e;

   SAFE_CALLOC(e, 1, sizeof(struct dec_entry));
   
   e->level = level;
   e->type = type;
   e->extractor = extractor;

   SLIST_INSERT_HEAD(&extractor_table, e, next); 

   return;
}


/*
 * get a extractor from the extractors table 
 */
void * get_extractor(u_int8 level, u_int32 type)
{
   struct dec_entry *e;
   void *ret;

   SLIST_FOREACH (e, &extractor_table, next) {
      if (e->level == level && e->type == type) {
         ret = (void *)e->extractor;
         return ret;
      }
   }

   return NULL;
}

/*
 * open a file to write into.
 * the file are saved in a subdirectories structure like this:
 *    - host
 *       - proto
 *          - stealed_file
 * e.g.:
 *    - 192.168.0.1
 *       - HTTP
 *          - images
 *             - a.gif
 *             - b.gif
 *          - styles
 *             - style.css
 *          - index.html
 *          - foo.php
 */
int decode_to_file(char *host, char *proto, char *file)
{
#define BASE_DIR  "./decoded_files"
   char dir[1024];
   char *p;
   char *path = strdup(file);
   char *filetmp = strdup(file);
   int fd;

   /* always create the base, host and proto directory */
   strcpy(dir, BASE_DIR);
   MKDIR(dir, 0700);
   strlcat(dir, "/", sizeof(dir));
   strlcat(dir, host, sizeof(dir));
   MKDIR(dir, 0700);
   strlcat(dir, "/", sizeof(dir));
   strlcat(dir, proto, sizeof(dir));
   MKDIR(dir, 0700);

   /* now 'dir' contains "BASE_DIR/host/proto" */

   /* parse the file to be created and create the required subdirectories */
   for (p = strsep(&path, "/"); p != NULL; p = strsep(&path, "/")) {
      strlcat(dir, "/", sizeof(dir));
      strlcat(dir, p, sizeof(dir));
      
      /* the token is a directory, create it */
      if (strcmp(p, basename(filetmp))) {
         MKDIR(dir, 0700);
      } else {
         /* exit the parsing and open the file */
         break;
      }
   }

   /* actually open the file */
   fd = open(dir, O_CREAT | O_TRUNC | O_RDWR | O_BINARY, 0600);

   SAFE_FREE(filetmp);
   SAFE_FREE(path);
   
   return fd;
}


/* EOF */

// vim:ts=3:expandtab

