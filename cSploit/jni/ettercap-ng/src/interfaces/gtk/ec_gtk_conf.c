/*
    ettercap -- GTK+ GUI

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

*/

#include <ec.h>
#include <ec_gtk.h>

void gtkui_conf_set(char *name, short value);
short gtkui_conf_get(char *name);
void gtkui_conf_read(void);
void gtkui_conf_save(void);

static char *filename = NULL;
static struct gtk_conf_entry settings[] = {
   { "window_top", 0 },
   { "window_left", 0 },
   { "window_height", 440 },
   { "window_width", 600 },
   { NULL, 0 },
};

void gtkui_conf_set(char *name, short value) {
   short c = 0;

   DEBUG_MSG("gtkui_conf_set: name=%s value=%hu", name, value);

   for(c = 0; settings[c].name != NULL; c++) {
      if(!strcmp(name, settings[c].name)) {
          settings[c].value = value;
          break;
      }
   }
}

short gtkui_conf_get(char *name) {
   unsigned short c = 0;

   DEBUG_MSG("gtkui_conf_get: name=%s", name);

   for(c = 0; settings[c].name != NULL; c++) {
      if(!strcmp(name, settings[c].name))
          return(settings[c].value);
   }

   return(0);
}

void gtkui_conf_read(void) {
   FILE *fd;
   const char *path;
   char line[100], name[30];
   short value;

   /* If you launch ettercap using sudo, then the config file is your user config dir */
   path = g_get_user_config_dir();
   filename = g_build_filename(path, "ettercap_gtk", NULL);

   DEBUG_MSG("gtkui_conf_read: %s", filename);

   fd = fopen(filename, "r");
   if(!fd) 
      return;

   while(fgets(line, 100, fd)) {
      char *p = strchr(line, '=');
     if(!p)
         continue;
      *p = '\0';
      snprintf(name, sizeof(name), "%s", line);
      strlcpy(name, line, sizeof(name) - 1);
      g_strstrip(name);
      value = atoi(p + 1);
      gtkui_conf_set(name, value);
   }
   fclose(fd);
}

void gtkui_conf_save(void) {
   FILE *fd;
   int c;

   DEBUG_MSG("gtkui_conf_save");

   if(!filename) 
      return;

   fd = fopen(filename, "w");
   if(fd != NULL) {
      for(c = 0; settings[c].name != NULL; c++)
         fprintf(fd, "%s = %hd\n", settings[c].name, settings[c].value);
      fclose(fd);
   }

   free(filename);
   filename = NULL;
}

/* EOF */

// vim:ts=3:expandtab

