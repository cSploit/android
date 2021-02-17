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

#ifndef OS_WINDOWS

#include <ec.h>

#include <ec_gtk.h>
#include <ec_file.h>
#include <ec_version.h>

/* globals */

static GtkTreeSelection  *selection = NULL;
static GtkListStore      *liststore = NULL;
static GtkTextBuffer     *textbuf = NULL;

typedef struct {
   char *title;
   char *file;
} help_pair;

help_pair help_list[] = {
   { "ettercap", "ettercap" },
   { "logging", "etterlog" },
   { "filters", "etterfilter" },
   { "plugins", "ettercap_plugins" },
   { "settings", "etter.conf" },
   { "curses", "ettercap_curses" },
   { NULL, NULL }
};

/* proto */
void gtkui_help(void);
void gtkui_help_open(char *file);
void gtkui_help_selected(GtkTreeSelection *treeselection, gpointer data);

/***#****************************************/

void gtkui_help(void)
{
   GtkWidget *dialog, *scrolled, *treeview, *hbox, *textview;
   GtkCellRenderer   *renderer;
   GtkTreeViewColumn *column;
   GtkTreeIter iter;
   help_pair *section;

   DEBUG_MSG("gtkui_help");

   dialog = gtk_dialog_new_with_buttons(EC_PROGRAM" Help", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_CLOSE, GTK_RESPONSE_CLOSE, NULL);
   gtk_window_set_default_size(GTK_WINDOW (dialog), 780, 580);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), TRUE);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);

   hbox = gtk_hbox_new (FALSE, 6);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 0);

   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, FALSE, FALSE, 0);
   gtk_widget_show(scrolled);
   
   treeview = gtk_tree_view_new();
   gtk_tree_view_set_headers_visible(GTK_TREE_VIEW (treeview), FALSE);
   gtk_container_add(GTK_CONTAINER (scrolled), treeview);
   gtk_widget_show(treeview);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
   g_signal_connect(selection, "changed", G_CALLBACK (gtkui_help_selected), liststore);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Contents", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   liststore = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);

   for(section = help_list; section->title; section++) {
      gtk_list_store_append (liststore, &iter);
      gtk_list_store_set (liststore, &iter,
                          0, section->title,
                          1, section->file, -1);
   }
   
   gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (liststore));

   /* text area */
   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
   gtk_widget_show(scrolled);

   textview = gtk_text_view_new();
   gtk_text_view_set_editable(GTK_TEXT_VIEW (textview), FALSE);
   gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW (textview), FALSE);
   gtk_container_add(GTK_CONTAINER (scrolled), textview);
   gtk_widget_show(textview);

   textbuf = gtk_text_view_get_buffer(GTK_TEXT_VIEW (textview));

   gtk_widget_show_all(hbox);

   gtk_dialog_run(GTK_DIALOG (dialog));

   gtk_widget_destroy (dialog);
}

void gtkui_help_open(char *file) {
   const char *full = "sh -c \"man %s | col -b\"";
   char *data = NULL, *errors = NULL, *cmd;
   gboolean ret = FALSE;
   gint len = 0;

   len = strlen(file) + strlen(full);
   cmd = g_malloc(sizeof(char) * len);
   snprintf(cmd, len, full, file);
   ret = g_spawn_command_line_sync(cmd, &data, &errors, NULL, NULL);
   g_free(cmd);

   /* TODO: use local copy of manpages as backup */
   if(ret && errors && strlen(errors) > 0) {
      ui_error(errors);
      g_free(errors);
   }

   /* print output of command in help window */
   if(data && ret) {
      gtk_text_buffer_set_text(textbuf, "", -1);
   
      gtkui_details_print(textbuf, data);
      g_free(data);
   }
}

void gtkui_help_selected(GtkTreeSelection *treeselection, gpointer data) {
   GtkTreeIter iter;
   GtkTreeModel *model;
   gchar *file;

   if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (treeselection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, 1, &file, -1);
      if(!file) return;

      gtkui_help_open(file);
   }
}

#endif
