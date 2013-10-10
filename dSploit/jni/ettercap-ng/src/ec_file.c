/*
    ettercap -- data handling module (fingerprints databases etc)

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

    $Id: ec_file.c,v 1.13 2004/07/12 19:57:26 alor Exp $
*/

#include <ec.h>
#include <ec_file.h>
#include <ec_version.h>

/* protos */

char * get_full_path(const char *dir, const char *file);
char * get_local_path(const char *file);
FILE * open_data(char *dir, char *file, char *mode);

/*******************************************/

/*
 * add the prefix to a given filename
 */

char * get_full_path(const char *dir, const char *file)
{
   char *filename;
   int len = 256;

   SAFE_CALLOC(filename, len, sizeof(char));
   
   if (!strcmp(dir, "etc"))
      snprintf(filename, len, "%s/%s", INSTALL_SYSCONFDIR, file);
   else if (!strcmp(dir, "share"))
      snprintf(filename, len, "%s/%s/%s", INSTALL_DATADIR, EC_PROGRAM, file);

   DEBUG_MSG("get_full_path -- [%s] %s", dir, filename);
   
   return filename;
}

/*
 * add the local path to a given filename
 */

char * get_local_path(const char *file)
{
   char *filename;

#ifdef OS_WINDOWS
   /* get the path in wich ettercap is running */
   char *self_root = ec_win_get_ec_dir();
#else
   char *self_root = ".";
#endif

   SAFE_CALLOC(filename, strlen(self_root) + strlen("/share/") + strlen(file) + 1, sizeof(char));
   
   sprintf(filename, "%s/share/%s", self_root, file);
   
   DEBUG_MSG("get_local_path -- %s", filename);
   
   return filename;
}


/*
 * opens a file in the share directory.
 * first look in the installation path, then locally.
 */

FILE * open_data(char *dir, char *file, char *mode)
{
   FILE *fd;
   char *filename = NULL;

   filename = get_full_path(dir, file);
  
   DEBUG_MSG("open_data (%s)", filename);
   
   fd = fopen(filename, mode);
   if (fd == NULL) {
      SAFE_FREE(filename);
      filename = get_local_path(file);

      DEBUG_MSG("open_data dropping to %s", filename);
      
      fd = fopen(filename, mode);
      /* don't check the fd, it is done by the caller */
   }
 
   SAFE_FREE(filename);
   
   return fd;
}


/* EOF */

// vim:ts=3:expandtab

