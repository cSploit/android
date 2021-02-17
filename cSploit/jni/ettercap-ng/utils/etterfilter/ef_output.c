/*
    etterfilter -- the actual compiler

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

    $Id: ef_output.c,v 1.9 2004/09/30 16:01:45 alor Exp $
*/

#include <ef.h>
#include <ef_functions.h>
#include <ec_filter.h>
#include <ec_version.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* protos */

int write_output(void);
static void print_progress_bar(struct filter_op *fop);
static u_char * create_data_segment(struct filter_header *fh, struct filter_op *fop, size_t n);
static size_t add_data_segment(u_char **data, size_t base, u_char **string, size_t slen);

/*******************************************/

int write_output(void)
{
   int fd;
   struct filter_op *fop;
   struct filter_header fh;
   size_t ninst, i;
   u_char *data;

   /* conver the tree to an array of filter_op */
   ninst = compile_tree(&fop);

   if (fop == NULL)
      return -ENOTHANDLED;

   /* create the file */
   fd = open(GBL_OPTIONS.output_file, O_CREAT | O_RDWR | O_TRUNC | O_BINARY, 0644);
   ON_ERROR(fd, -1, "Can't create file %s", GBL_OPTIONS.output_file);

   /* display the message */
   fprintf(stdout, " Writing output to \'%s\' ", GBL_OPTIONS.output_file);
   fflush(stdout);
   
   /* compute the header */
   fh.magic = htons(EC_FILTER_MAGIC);
   strncpy(fh.version, EC_VERSION, sizeof(fh.version));
   fh.data = sizeof(fh);

   data = create_data_segment(&fh, fop, ninst);
   
   /* write the header */
   write(fd, &fh, sizeof(struct filter_header));

   /* write the data segment */
   write(fd, data, fh.code - fh.data);
   
   /* write the instructions */
   for (i = 0; i <= ninst; i++) {
      print_progress_bar(&fop[i]);
      write(fd, &fop[i], sizeof(struct filter_op));
   }

   close(fd);
   
   fprintf(stdout, " done.\n\n");
  
   fprintf(stdout, " -> Script encoded into %d instructions.\n\n", (int)(i - 1));
   
   return ESUCCESS;
}

/*
 * creates the data segment into an array
 * and update the file header
 */
static u_char * create_data_segment(struct filter_header *fh, struct filter_op *fop, size_t n)
{
   size_t i, len = 0;
   u_char *data = NULL;

   for (i = 0; i < n; i++) {
      
      switch(fop[i].opcode) {
         case FOP_FUNC:
            if (fop[i].op.func.slen) {
               ef_debug(1, "@");
               len += add_data_segment(&data, len, &fop[i].op.func.string, fop[i].op.func.slen);
            }
            if (fop[i].op.func.rlen) {
               ef_debug(1, "@");
               len += add_data_segment(&data, len, &fop[i].op.func.replace, fop[i].op.func.rlen);
            }
            break;
            
         case FOP_TEST:
            if (fop[i].op.test.slen) {
               ef_debug(1, "@");
               len += add_data_segment(&data, len, &fop[i].op.test.string, fop[i].op.test.slen);
            }
            break;

         case FOP_ASSIGN:
            if (fop[i].op.assign.slen) {
               ef_debug(1, "@");
               len += add_data_segment(&data, len, &fop[i].op.test.string, fop[i].op.test.slen);
            }
            break;
      }

   }
  
   /* where starts the code ? */
   fh->code = fh->data + len;
   
   return data;
}


/* 
 * add a string to the buffer 
 */
static size_t add_data_segment(u_char **data, size_t base, u_char **string, size_t slen)
{
   /* make room for the new string */
   SAFE_REALLOC(*data, base + slen + 1);

   /* copy the string, NULL separated */
   memcpy(*data + base, *string, slen + 1);

   /* 
    * change the pointer to the new string location 
    * it is an offset from the base of the data segment
    */
   *string = (u_char *)base;
   
   /* retur the len of the added string */
   return slen + 1;
}

/*
 * prints a differnt sign for every different instruction
 */
static void print_progress_bar(struct filter_op *fop)
{
   switch(fop->opcode) {
      case FOP_EXIT:
         ef_debug(1, "!");
         break;
      case FOP_TEST:
         ef_debug(1, "?");
         break;
      case FOP_ASSIGN:
         ef_debug(1, "=");
         break;
      case FOP_FUNC:
         ef_debug(1, ".");
         break;
      case FOP_JMP:
         ef_debug(1, ":");
         break;
      case FOP_JTRUE:
      case FOP_JFALSE:
         ef_debug(1, ";");
         break;
   }
}

/* EOF */

// vim:ts=3:expandtab

