/*
    ettercap -- update databases from ettercap website

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

    $Id: ec_update.c,v 1.14 2004/06/25 14:24:29 alor Exp $
*/

#include <ec.h>
#include <ec_socket.h>
#include <ec_file.h>

#define ERR_MAX_LEN  128

/* protos */

void global_update(void);
static void update_file(char *tokens);
static int get_current_rev(char *file, char **curr, char *errbuf);
static int do_update(char *file, char *url, char *errbuf);

/*******************************************/

/*
 * perform the update 
 */
void global_update(void)
{
   int sock;
   char *ptr;
   char *latest;
   char getmsg[512];
   char buffer[8192];
   int len;
   char *tok;
   char host[] = "ettercap.sourceforge.net";
//   char host[] = "local.alor.org";
   char page[] = "/updateNG.php";

   DEBUG_MSG("global_update");

   memset(buffer, 0, sizeof(buffer));

   fprintf(stdout, "Connecting to http://%s\n", host);

   /* open the socket with the server */
   if ((sock = open_socket(host, 80)) < 0)
      clean_exit(-1);

   fprintf(stdout, "Requesting %s\n\n", page);

   /* prepare the HTTP request */
   snprintf(getmsg, sizeof(getmsg), "GET %s HTTP/1.0\r\n"
                                     "Host: %s\r\n"
                                     "User-Agent: %s (%s)\r\n"
                                     "\r\n", page, host, GBL_PROGRAM, GBL_VERSION );

   /* send the request to the server */
   socket_send(sock, getmsg, strlen(getmsg));

   DEBUG_MSG("global_update - SEND \n\n%s\n\n", getmsg);

   /* get the server response */
   len = socket_recv(sock, buffer, sizeof(buffer) - 1);

   if (len == 0)
      FATAL_ERROR(EC_COLOR_RED"ERROR"EC_COLOR_END" The server does not respond");
      
   DEBUG_MSG("global_update - RECEIVE \n\n%s\n\n", buffer);

   close_socket(sock);

   /* skip the HTTP headers */
   ptr = strstr(buffer, "\r\n\r\n");
   if (ptr == NULL)
      FATAL_ERROR(EC_COLOR_RED"ERROR"EC_COLOR_END" Bad response from server");
  
   ptr += 4;

   /* the first word MUST be "ettercap" */
   if (strncmp(ptr, GBL_PROGRAM, strlen(GBL_PROGRAM)))
      FATAL_ERROR(EC_COLOR_RED"ERROR"EC_COLOR_END" Bad response from server");
  
   ptr += strlen(GBL_PROGRAM) + 1;

   /* the first line in the response is the latest version */
   latest = strdup(ec_strtok(ptr, "\n", &tok));
   /* move the ptr after the first token */
   ptr += strlen(latest) + 1;
   
   fprintf(stdout, " + %-18s -> version "EC_COLOR_BLUE"%-4s"EC_COLOR_END" latest is ", GBL_PROGRAM, GBL_VERSION);
   if (!strcmp(GBL_VERSION, latest))
      fprintf(stdout, EC_COLOR_GREEN"%-4s"EC_COLOR_END"\n\n", latest );
   else
      fprintf(stdout, EC_COLOR_YELLOW"%-4s"EC_COLOR_END"\n\n", latest );
      
   SAFE_FREE(latest);

   /* update every entry in the response */
   for(latest = strsep(&ptr, "\n"); latest != NULL; latest = strsep(&ptr, "\n")) {
      update_file(latest);
   }
  
   fprintf(stdout, "\n\n");

   clean_exit(0);
}

/*
 * update a single file.
 * parse the "tokens" and check the version
 * if the version is newer, update it from the website
 */
static void update_file(char *tokens)
{
   char *file = NULL;
   char *rev = NULL;
   char *curr = NULL;
   char *url = NULL;
   size_t i, n = 0;
   char *tok;
   char errbuf[ERR_MAX_LEN];
  
   DEBUG_MSG("update_file");

   /* count the number of tokens delimited by ' ' */
   for (i = 0; i < strlen(tokens); i++)
      if (tokens[i] == ' ')
         n++;

   /* the token is invalid */
   if (n != 2)
      return;
   
   /* split the tokens */
   file = strdup(ec_strtok(tokens, " ", &tok));
   rev = strdup(ec_strtok(NULL, " ", &tok));
   url = strdup(ec_strtok(NULL, " ", &tok));
   
   /* get the current revision */
   if (get_current_rev(file, &curr, errbuf) == 0) {
      fprintf(stdout, " + %-18s -> "EC_COLOR_RED"ERROR"EC_COLOR_END"  %s\n", file, errbuf);
   } else {
      fprintf(stdout, " + %-18s -> revision "EC_COLOR_BLUE"%-4s"EC_COLOR_END" updating to "EC_COLOR_CYAN"%-4s"EC_COLOR_END"... ", file, curr, rev );
      fflush(stdout);
  
      /* update it if the current rev is different (newer) */
      if (!strcmp(curr, rev))
         fprintf(stdout, EC_COLOR_GREEN"OK"EC_COLOR_END"\n");
      else if (strcmp(curr, rev) < 0) {
         if (GBL_OPTIONS->silent)
            fprintf(stdout, EC_COLOR_YELLOW"NEED UPDATE"EC_COLOR_END"\n");
         else { 
            if (do_update(file, url, errbuf))
               fprintf(stdout, EC_COLOR_YELLOW"UPDATED"EC_COLOR_END"\n");
            else
               fprintf(stdout, EC_COLOR_RED"ERROR"EC_COLOR_END"  %s\n", errbuf);
         }
      } else
         fprintf(stdout, EC_COLOR_RED"NEWER"EC_COLOR_END"  %s\n", errbuf);
         
   }
   
   SAFE_FREE(curr);
   SAFE_FREE(file);
   SAFE_FREE(rev);
   SAFE_FREE(url);
}

/* 
 * get the current file revision 
 * it is stored in the cvs var $Revision: 1.14 $
 */
static int get_current_rev(char *file, char **curr, char *errbuf)
{
   FILE *fd;
   char line[128];
   char *ptr;
  
   /* do not permit to insert ../../../ in the filename */
   if (strchr(file, '/')) {
      snprintf(errbuf, ERR_MAX_LEN, "invalid file");
      return 0;
   }
   
   /* check if the file exists */
   fd = open_data("share", file, FOPEN_READ_TEXT);
   if (fd == NULL) {
      snprintf(errbuf, ERR_MAX_LEN, "cannot open file %s", file);
      return 0;
   }

   /* search the right line */
   while (fgets(line, sizeof(line), fd) != 0) {
      if ( (ptr = strstr(line, "Revision: ")) ) {
         /* get the revision */
         *curr = strdup(ptr + strlen("Revision: "));
         /* truncate at the first blank space */
         ptr = *curr;
         while (*ptr != ' ') ptr++;
         *ptr = '\0';

         return 1;
         break;
      }
   }
   
   snprintf(errbuf, ERR_MAX_LEN, "bad revision number");
   return 0;
}

/* 
 * download the file and replace 
 * the existing one
 */
static int do_update(char *file, char *url, char *errbuf)
{
   FILE *fd;
   int sock;
   int len, header_skipped = 0;
   char *ptr = NULL;
   char *host;
   char getmsg[512];
   char buffer[4096];
 
   memset(buffer, 0, sizeof(buffer));
   
   /* check if the url is valid */
   if (!match_pattern(url, "http://*/*")) {
      snprintf(errbuf, ERR_MAX_LEN, "invalid URL");
      return 0;
   }

   /* get the hostname */
   host = strdup(url + strlen("http://"));
   ptr = host;
   while (*ptr != '/') ptr++;
   *ptr = '\0';
   
   /* open the file for writing */
   fd = open_data("share", file, FOPEN_WRITE_TEXT);
   if (fd == NULL) {
      snprintf(errbuf, ERR_MAX_LEN, "cannot open %s", file);
      return 0;
   }
  
   sock = open_socket(host, 80);
   
   switch(sock) {
      case -ENOADDRESS:
         FATAL_MSG("Cannot resolve %s", host);
         break;
      case -EFATAL:
         FATAL_MSG("Cannot create the socket");
         break;
      case -ETIMEOUT:
         FATAL_MSG("Connect timeout to %s on port 80", host);
         break;
      case -EINVALID:
         FATAL_MSG("Error connecting to %s on port 80", host);
         break;
   }
   
   /* prepare the HTTP request */
   snprintf(getmsg, sizeof(getmsg), "GET %s HTTP/1.0\r\n"
                                     "Host: %s\r\n"
                                     "User-Agent: %s (%s)\r\n"
                                     "\r\n", url, host, GBL_PROGRAM, GBL_VERSION );
   
   /* send the request to the server */
   socket_send(sock, getmsg, strlen(getmsg));

   DEBUG_MSG("do_update - SEND \n\n%s\n\n", getmsg);

   /* get the server response */
   while ( (len = socket_recv(sock, buffer, sizeof(buffer) - 1)) ) {

      DEBUG_MSG("do_update - RECEIVE \n\n%s\n\n", buffer);

      /* skip the HTTP header */
      if ( (ptr = strstr(buffer, "\r\n\r\n")))
         header_skipped = 1;
   
      /* write the data in the file */
      if (header_skipped) {
         if (ptr) {
            write(fileno(fd), ptr + 4, len - (ptr + 4 - buffer));
         } else {
            write(fileno(fd), buffer, len);
         }
      }
   
      memset(buffer, 0, sizeof(buffer));
      ptr = NULL;

   }

   SAFE_FREE(host);
   close_socket(sock);
   fclose(fd);

   return 1;
}

/* EOF */

// vim:ts=3:expandtab

