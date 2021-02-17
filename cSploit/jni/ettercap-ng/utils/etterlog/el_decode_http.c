/*
    etterlog -- extractor for http and proxy -- TCP 80, 8080

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

    $Id: el_decode_http.c,v 1.5 2004/11/04 10:37:16 alor Exp $
*/

#include <el.h>
#include <el_functions.h>

/* globals */

/* protos */
FUNC_EXTRACTOR(extractor_http);
void http_init(void);

/************************************************/

/*
 * this function is the initializer.
 * it adds the entry in the table of registered decoder
 */

void __init http_init(void)
{
   add_extractor(APP_LAYER_TCP, 80, extractor_http);
   add_extractor(APP_LAYER_TCP, 8080, extractor_http);
}


FUNC_EXTRACTOR(extractor_http)
{
   char header[1024];
   struct so_list *ret, *ret2;
   char host[MAX_ASCII_ADDR_LEN];
   int len, fd;
   u_char *ptr, *data;
   int client, server;

   
   /* get the ip address of the server where the file are coming from */
   if (ntohs(STREAM->side1.so_curr->po.L4.dst) == 80) {
      ip_addr_ntoa(&STREAM->side1.so_curr->po.L3.dst, host);
      client = STREAM->side1.so_curr->side;
   } else {
      ip_addr_ntoa(&STREAM->side1.so_curr->po.L3.src, host);
      client = ~(STREAM->side1.so_curr->side);
   }

   server = ~client;
      
   /* 
    * steal all the files found in the stream
    * we only extract files requested via the GET method
    */
   do {
      ret = stream_search(STREAM, "GET ", 4, client);

      /* if found, read only from that direction */
      if (ret != NULL) {
         stream_move(STREAM, 4, SEEK_CUR, client);

         memset(header, 0, sizeof(header));
         stream_read(STREAM, header, 128, client);
          
         /* get the filename (until the first blank) */
         if ( (ptr = strchr(header, ' ')) != NULL )
            *ptr = '\0';
         
         /* 
          * the browser is requesting the root.
          * we save this in the index.html file
          */
         if (!strcmp(header, "/"))
            strcpy(header, "/index.html");
            
         /* open the file for writing */
         fd = decode_to_file(host, "HTTP", header);
         ON_ERROR(fd, -1, "Cannot create file: %s", header);

         fprintf(stdout, "\n\tExtracting file: %s  ", header);

         /* search the lenght of the file */
         ret2 = stream_search(STREAM, "Content-Length: ", 16, server);

         /* we need content lenght, if not found, skip it */
         if (ret2 == NULL) {
            close(fd);
            continue;
         }
         
         /* get the string until the \r */
         stream_move(STREAM, 16, SEEK_CUR, server);
         stream_read(STREAM, header, 10, server);
         if ( (ptr = strchr(header, '\r')) != NULL )
            *ptr = '\0';
         
         len = atoi(header);
         
         fprintf(stdout, " %d bytes", len);
         
         /* search the end of the headers */
         ret2 = stream_search(STREAM, "\r\n\r\n", 4, server);
         
         /* we need the end of the header, if not found, skip it */
         if (ret2 == NULL) {
            close(fd);
            continue;
         }
         
         stream_move(STREAM, 4, SEEK_CUR, server);

#if 0
         memset(header, 0, sizeof(header));
         stream_read(STREAM, header, 10, ret2->side);
         printf("header: [%s]\n", header);
         exit(0);
#endif    
         
         //continue;
         //printf(" move: %d", stream_move(STREAM, len, SEEK_CUR, ret2->side));
         
         SAFE_CALLOC(data, len, sizeof(u_char));
         
         stream_read(STREAM, data, len, server);
         write(fd, data, len);

         SAFE_FREE(data);
         
         close(fd);
         
      }
   } while (ret != NULL);
   
   return STREAM_DECODED;
}


/* EOF */

// vim:ts=3:expandtab
