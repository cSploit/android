/*
    etterfilter -- offset tables handling

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

    $Id: ef_tables.c,v 1.13 2004/06/25 14:24:30 alor Exp $
*/

#include <ef.h>
#include <ec_file.h>

#include <ctype.h>

/* globals */

struct off_entry {
   char *name;
   u_int16 offset;
   u_int8 size;
   SLIST_ENTRY(off_entry) next;
};

struct table_entry {
   char * name;
   u_int8 level;
   SLIST_HEAD (, off_entry) offsets;
   SLIST_ENTRY(table_entry) next;
};

static SLIST_HEAD (, table_entry) table_head;

struct const_entry {
   char *name;
   u_int32 value;
   SLIST_ENTRY(const_entry) next;
};

static SLIST_HEAD (, const_entry) const_head;

/* protos */

void load_tables(void);
static void add_virtualpointer(char *name, u_int8 level, char *offname, u_int16 offset, u_int8 size);
int get_virtualpointer(char *name, char *offname, u_int8 *level, u_int16 *offset, u_int8 *size);

void load_constants(void);
int get_constant(char *name, u_int32 *value);

/*******************************************/

/* 
 * load the tables for file 
 */
void load_tables(void)
{
   struct table_entry *t;
   FILE *fc;
   char line[128];
   int lineno = 0, ntables = 0;
   char *p, *q, *end;
   char *name = NULL, *oname = NULL;
   u_int8 level = 0, size = 0;
   u_int16 offset = 0;
   char *tok;

   /* open the file */ 
   fc = open_data("share", "etterfilter.tbl", FOPEN_READ_TEXT);
   ON_ERROR(fc, NULL, "Cannot find file etterfilter.tbl");

   /* read the file */
   while (fgets(line, 128, fc) != 0) {
     
      /* pointer to the end of the line */
      end = line + strlen(line);
      
      /* count the lines */
      lineno++;

      /* trim out the comments */
      if ((p = strchr(line, '#')))
         *p = '\0';

      /* trim out the new line */
      if ((p = strchr(line, '\n')))
         *p = '\0';

      /* skip empty lines */
      if (line[0] == '\0')
         continue;
      
      /* eat the empty spaces */
      for (q = line; *q == ' ' && q < end; q++);
      
      /* skip empty lines */
      if (*q == '\0')
         continue;

      /* begin of a new section */
      if (*q == '[') {
         SAFE_FREE(name);
         
         /* get the name in the brackets [ ] */
         if ((p = strchr(q, ']')))
            *p = '\0';
         else
            FATAL_ERROR("Parse error in etterfilter.conf on line %d", lineno);

         name = strdup(q + 1);
         ntables++;
         
         /* get the level in the next brackets [ ] */
         q = p + 1;
         if ((p = strchr(q, ']')))
            *p = '\0';
         else
            FATAL_ERROR("Parse error in etterfilter.conf on line %d", lineno);

         level = atoi(q + 1);

         continue;
      }
      
      /* parse the offsets and add them to the table */
      oname = ec_strtok(q, ":", &tok);
      q = ec_strtok(NULL, ":", &tok);
      if (q && ((p = strchr(q, ' ')) || (p = strchr(q, '='))))
         *p = '\0';
      else
         FATAL_ERROR("Parse error in etterfilter.conf on line %d", lineno);

      /* get the size */
      size = atoi(q);
      
      /* get the offset */
      for (q = p; !isdigit((int)*q) && q < end; q++);
      
      offset = atoi(q);

      /* add to the table */
      add_virtualpointer(name, level, oname, offset, size);
   }

   /* print some nice informations */
   fprintf(stdout, "\n%3d protocol tables loaded:\n", ntables);
   fprintf(stdout, "\t");
   SLIST_FOREACH(t, &table_head, next)
      fprintf(stdout, "%s ", t->name);
   fprintf(stdout, "\n");
  
}

/*
 * add a new virtual pointer to the table
 */
static void add_virtualpointer(char *name, u_int8 level, char *offname, u_int16 offset, u_int8 size)
{
   struct table_entry *t;
   struct off_entry *o;
   int found = 0;

   /* search if the table already exist */
   SLIST_FOREACH(t, &table_head, next) {
      if (!strcmp(t->name, name)) {
         found = 1;
         break;
      }
   }

   /* the table was not found */
   if (!found) {
      SAFE_CALLOC(t, 1, sizeof(struct table_entry));
     
      SAFE_CALLOC(o, 1, sizeof(struct off_entry));
      
      /* fill the structures */
      t->name = strdup(name);
      t->level = level;
      o->name = strdup(offname);
      o->offset = offset;
      o->size = size;

      SLIST_INSERT_HEAD(&t->offsets, o, next);
      SLIST_INSERT_HEAD(&table_head, t, next);
      
   } else {
      SAFE_CALLOC(o, 1, sizeof(struct off_entry));

      /* fill the structures */
      o->name = strdup(offname);
      o->offset = offset;
      o->size = size;

      /* t already points to the right tables */
      SLIST_INSERT_HEAD(&t->offsets, o, next);
   }

}

/*
 * get a virtual pointer from the table
 */
int get_virtualpointer(char *name, char *offname, u_int8 *level, u_int16 *offset, u_int8 *size)
{
   struct table_entry *t;
   struct off_entry *o;

   /* search the table and the offset */
   SLIST_FOREACH(t, &table_head, next) {
      if (!strcmp(t->name, name)) {
         SLIST_FOREACH(o, &t->offsets, next) {
            if (!strcmp(o->name, offname)) {
               /* set the return values */
               *size = o->size;
               *level = t->level;
               *offset = o->offset;
               
               return ESUCCESS;
            }
         }
         return -ENOTFOUND;
      }
   }
   
   return -ENOTFOUND;
}


/*
 * load constants from the file 
 */
void load_constants(void)
{
   struct const_entry *c = NULL;
   FILE *fc;
   char line[128];
   int lineno = 0, nconst = 0;
   char *p, *q, *end, *tok;
 
   /* open the file */ 
   fc = open_data("share", "etterfilter.cnt", FOPEN_READ_TEXT);
   ON_ERROR(fc, NULL, "Cannot find file etterfilter.cnt");

   /* read the file */
   while (fgets(line, 128, fc) != 0) {
     
      /* pointer to the end of the line */
      end = line + strlen(line);
      
      /* count the lines */
      lineno++;

      /* trim out the comments */
      if ((p = strchr(line, '#')))
         *p = '\0';

      /* trim out the new line */
      if ((p = strchr(line, '\n')))
         *p = '\0';

      /* skip empty lines */
      if (line[0] == '\0')
         continue;
      
      /* eat the empty spaces */
      for (q = line; *q == ' ' && q < end; q++);

      /* get the constant */
      if (strstr(line, "=") && (q = ec_strtok(line, "=", &tok)) != NULL) {
         /* trim out the space */
         if ((p = strchr(q, ' ')))
            *p = '\0';
        
         SAFE_CALLOC(c, 1, sizeof(struct const_entry));
         
         /* get the name */
         c->name = strdup(q);
         
         if ((q = ec_strtok(NULL, "=", &tok)) == NULL)
            FATAL_ERROR("Invalid constant on line %d", lineno);

         /* eat the empty spaces */
         for (p = q; *p == ' ' && p < end; p++);

         c->value = strtoul(p, NULL, 16);

         /* update the counter */
         nconst++;
      } else
         FATAL_ERROR("Invalid constant on line %d", lineno);

      /* insert in the list */
      SLIST_INSERT_HEAD(&const_head, c, next);
   }
   
   /* print some nice informations */
   fprintf(stdout, "\n%3d constants loaded:\n", nconst);
   fprintf(stdout, "\t");
   SLIST_FOREACH(c, &const_head, next)
      fprintf(stdout, "%s ", c->name);
   fprintf(stdout, "\n");
}


/*
 * return the value of a constant
 *
 * the value is integer32 and in host order
 */
int get_constant(char *name, u_int32 *value)
{
   struct const_entry *c;

   SLIST_FOREACH(c, &const_head, next) {
      if (!strcmp(name, c->name)) {
         *value = c->value;
         return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}

/* EOF */

// vim:ts=3:expandtab

