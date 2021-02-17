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
#include <ec_file.h>
#include <ec_filter.h>
#include <ec_version.h>
#include <ec_gtk.h>

/* proto */

void gtkui_load_filter(void);
void gtkui_stop_filter(void);

/* globals */

/*******************************************/


/*
 * display the file open dialog
 */
void gtkui_load_filter(void)
{
   GtkWidget *dialog;
   const char *filename;
   int response = 0;
   char *path = get_full_path("share", "");

   DEBUG_MSG("gtk_load_filter");

   dialog = gtk_file_selection_new ("Select a precompiled filter file...");
   gtk_file_selection_set_filename(GTK_FILE_SELECTION(dialog), path);

   SAFE_FREE(path);

   response = gtk_dialog_run (GTK_DIALOG (dialog));

   if (response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);
      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      /* 
       * load the filters chain.
       * errors are spawned by the function itself
       */
      filter_load_file(filename, GBL_FILTERS, 1);
   }
   gtk_widget_destroy (dialog);
}

/*
 * uload the filter chain
 */
void gtkui_stop_filter(void)
{
   DEBUG_MSG("gtk_stop_filter");

   filter_unload(GBL_FILTERS);
   
   gtkui_message("Filters were unloaded");
}

/* EOF */

// vim:ts=3:expandtab

