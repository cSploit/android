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

/* proto */

void toggle_reverse(void);
void gtkui_select_protocol(void);
void wipe_targets(void);
void gtkui_select_targets(void);
void gtkui_current_targets(void);
static void set_protocol(void);
static void set_targets(void);
void gtkui_create_targets_array(void);
static void gtkui_add_target1(void *);
static void gtkui_add_target2(void *);
static void add_target1(void);
static void add_target2(void);
static void gtkui_delete_targets(GtkWidget *widget, gpointer data);
static void gtkui_targets_destroy(void);
static void gtkui_targets_detach(GtkWidget *child);
static void gtkui_targets_attach(void);

/* globals */

static char thost[MAX_ASCII_ADDR_LEN];

GtkWidget    *targets_window = NULL;
GtkTreeSelection *selection1 = NULL;
GtkTreeSelection *selection2 = NULL;
GtkListStore     *liststore1 = NULL;
GtkListStore     *liststore2 = NULL;

/*******************************************/

void toggle_reverse(void)
{
   if (GBL_OPTIONS->reversed) {
      GBL_OPTIONS->reversed = 0;
   } else {
      GBL_OPTIONS->reversed = 1;
   }
}

/*
 * wipe the targets struct setting both T1 and T2 to ANY/ANY/ANY
 */
void wipe_targets(void)
{
   DEBUG_MSG("wipe_targets");
   
   reset_display_filter(GBL_TARGET1);
   reset_display_filter(GBL_TARGET2);

   /* update the GTK liststores */
   gtkui_create_targets_array();

   /* display the message */
   gtkui_message("TARGETS were reset to ANY/ANY/ANY");
}

/*
 * display the protocol dialog
 */
void gtkui_select_protocol(void)
{
   DEBUG_MSG("gtk_select_protocol");

   /* this will contain 'all', 'tcp' or 'udp' */
   if (!GBL_OPTIONS->proto) {
      SAFE_CALLOC(GBL_OPTIONS->proto, 4, sizeof(char));
      strncpy(GBL_OPTIONS->proto, "all", 3);
   }

   gtkui_input("Protocol :", GBL_OPTIONS->proto, 3, set_protocol);
}

static void set_protocol(void)
{
   if (strcasecmp(GBL_OPTIONS->proto, "all") &&
       strcasecmp(GBL_OPTIONS->proto, "tcp") &&
       strcasecmp(GBL_OPTIONS->proto, "udp")) {
      ui_error("Invalid protocol");
      SAFE_FREE(GBL_OPTIONS->proto);
   }
}

/*
 * display the TARGET(s) dialog
 */
void gtkui_select_targets(void)
{
   GtkWidget *dialog, *hbox, *label, *entry1, *entry2;  
   
#define TARGET_LEN 50
   
   DEBUG_MSG("gtk_select_targets");

   dialog = gtk_dialog_new_with_buttons("Enter Targets", GTK_WINDOW(window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT, NULL);
   gtk_container_set_border_width (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), 20);

   hbox = gtk_hbox_new(FALSE, 0);
   label = gtk_label_new ("Target 1: ");
   
   gtk_box_pack_start(GTK_BOX (hbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   entry1 = gtk_entry_new_with_max_length(TARGET_LEN);
   gtk_entry_set_width_chars (GTK_ENTRY (entry1), TARGET_LEN);
   
   if (GBL_OPTIONS->target1)
      gtk_entry_set_text(GTK_ENTRY (entry1), GBL_OPTIONS->target1); 
   
   gtk_box_pack_start(GTK_BOX (hbox), entry1, FALSE, FALSE, 0);
   gtk_widget_show(entry1);
   gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 5);
   gtk_widget_show(hbox);

   hbox = gtk_hbox_new(FALSE, 0);
   label = gtk_label_new ("Target 2: ");
   gtk_box_pack_start(GTK_BOX (hbox), label, TRUE, TRUE, 0);
   gtk_widget_show(label);

   entry2 = gtk_entry_new_with_max_length(TARGET_LEN);
   gtk_entry_set_width_chars (GTK_ENTRY (entry2), TARGET_LEN);
   
   if (GBL_OPTIONS->target2)
      gtk_entry_set_text(GTK_ENTRY (entry2), GBL_OPTIONS->target2); 
   
   gtk_box_pack_start(GTK_BOX (hbox), entry2, FALSE, FALSE, 0);
   gtk_widget_show(entry2);
   gtk_box_pack_start(GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, TRUE, TRUE, 5);
   gtk_widget_show(hbox);

   if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
      gtk_widget_hide(dialog);

      SAFE_FREE(GBL_OPTIONS->target1);
      SAFE_FREE(GBL_OPTIONS->target2);

      SAFE_CALLOC(GBL_OPTIONS->target1, TARGET_LEN, sizeof(char));
      SAFE_CALLOC(GBL_OPTIONS->target2, TARGET_LEN, sizeof(char));

      strncpy(GBL_OPTIONS->target1, gtk_entry_get_text(GTK_ENTRY (entry1)), TARGET_LEN);
      strncpy(GBL_OPTIONS->target2, gtk_entry_get_text(GTK_ENTRY (entry2)), TARGET_LEN);

      set_targets();
   }
   gtk_widget_destroy(dialog);
}

/*
 * set the targets 
 */
static void set_targets(void)
{
   /* delete the previous filters */
   reset_display_filter(GBL_TARGET1);
   reset_display_filter(GBL_TARGET2);

   /* free empty filters */
   if (!strcmp(GBL_OPTIONS->target1, ""))
      SAFE_FREE(GBL_OPTIONS->target1);
   
   /* free empty filters */
   if (!strcmp(GBL_OPTIONS->target2, ""))
      SAFE_FREE(GBL_OPTIONS->target2);
   
   /* compile the filters */
   compile_display_filter();

   /* if the 'current targets' window is displayed, refresh it */
   if (targets_window)
      gtkui_current_targets();
}

/*
 * display the list of current targets
 */
void gtkui_current_targets(void)
{
   GtkWidget *scrolled, *treeview, *vbox, *hbox, *button;
   GtkCellRenderer   *renderer;
   GtkTreeViewColumn *column;

   DEBUG_MSG("gtk_current_targets");

   /* prepare the liststores for the target lists */
   gtkui_create_targets_array();
  
   if(targets_window) {
      if(GTK_IS_WINDOW (targets_window))
         gtk_window_present(GTK_WINDOW (targets_window));
      else
         gtkui_page_present(targets_window);
      return;
   }

   targets_window = gtkui_page_new("Targets", &gtkui_targets_destroy, &gtkui_targets_detach);

   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER (targets_window), vbox);
   gtk_widget_show(vbox);

   hbox = gtk_hbox_new(TRUE, 5);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
   gtk_widget_show(hbox);

   /* list one */
   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
   gtk_widget_show(scrolled);

   treeview = gtk_tree_view_new();
   gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (liststore1));
   gtk_container_add(GTK_CONTAINER (scrolled), treeview);
   gtk_widget_show(treeview);

   selection1 = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
   gtk_tree_selection_set_mode (selection1, GTK_SELECTION_MULTIPLE);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Target 1", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   /* list two */
   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(hbox), scrolled, TRUE, TRUE, 0);
   gtk_widget_show(scrolled);

   treeview = gtk_tree_view_new();
   gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (liststore2));
   gtk_container_add(GTK_CONTAINER (scrolled), treeview);
   gtk_widget_show(treeview);

   selection2 = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
   gtk_tree_selection_set_mode (selection2, GTK_SELECTION_MULTIPLE);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Target 2", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   /* buttons */
   hbox = gtk_hbox_new(TRUE, 5);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

   button = gtk_button_new_with_mnemonic("Delete");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_delete_targets), (gpointer)1);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   button = gtk_button_new_with_mnemonic("Add");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_add_target1), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   button = gtk_button_new_with_mnemonic("Delete");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_delete_targets), (gpointer)2);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   button = gtk_button_new_with_mnemonic("Add");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_add_target2), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

   gtk_widget_show_all(hbox);
   gtk_widget_show(targets_window);
}

static void gtkui_targets_detach(GtkWidget *child)
{
   targets_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (targets_window), "Current Targets");
   gtk_window_set_default_size(GTK_WINDOW (targets_window), 400, 300);
   g_signal_connect (G_OBJECT (targets_window), "delete_event", G_CALLBACK (gtkui_targets_destroy), NULL);

   /* make <ctrl>d shortcut turn the window back into a tab */
   gtkui_page_attach_shortcut(targets_window, gtkui_targets_attach);

   gtk_container_add(GTK_CONTAINER (targets_window), child);

   gtk_window_present(GTK_WINDOW (targets_window));
}

static void gtkui_targets_attach(void)
{
   gtkui_targets_destroy();
   gtkui_current_targets();
}

static void gtkui_targets_destroy(void)
{
   gtk_widget_destroy(targets_window);
   targets_window = NULL;
}

/*
 * create the array for the widget.
 * erase any previously alloc'd array 
 */
void gtkui_create_targets_array(void)
{
   GtkTreeIter iter;
   struct ip_list *il;
   char tmp[MAX_ASCII_ADDR_LEN];

   DEBUG_MSG("gtk_create_targets_array");

   if(liststore1)
      gtk_list_store_clear(GTK_LIST_STORE (liststore1));
   else
      liststore1 = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
   
   /* walk TARGET 1 */
   LIST_FOREACH(il, &GBL_TARGET1->ips, next) {
      /* enlarge the array */
      gtk_list_store_append (liststore1, &iter);
      /* fill the element */
      gtk_list_store_set (liststore1, &iter, 0, ip_addr_ntoa(&il->ip, tmp), 1, il, -1);
   }

   if(liststore2)
      gtk_list_store_clear(GTK_LIST_STORE (liststore2));
   else
      liststore2 = gtk_list_store_new (2, G_TYPE_STRING, G_TYPE_POINTER);
   
   /* walk TARGET 2 */
   LIST_FOREACH(il, &GBL_TARGET2->ips, next) {
      /* enlarge the array */
      gtk_list_store_append (liststore2, &iter);
      /* fill the element */
      gtk_list_store_set (liststore2, &iter, 0, ip_addr_ntoa(&il->ip, tmp), 1, il, -1);
   }
}

/*
 * display the "add host" dialog
 */
static void gtkui_add_target1(void *entry)
{
   DEBUG_MSG("gtk_add_target1");

   gtkui_input("IP address :", thost, MAX_ASCII_ADDR_LEN, add_target1);
}

static void gtkui_add_target2(void *entry)
{
   DEBUG_MSG("gtk_add_target2");

   gtkui_input("IP address :", thost, MAX_ASCII_ADDR_LEN, add_target2);
}

static void add_target1(void)
{
   struct in_addr ip;
   struct ip_addr host;
   
   if (inet_aton(thost, &ip) == 0) {
      gtkui_message("Invalid ip address");
      return;
   }
   
   ip_addr_init(&host, AF_INET, (char *)&ip);

   add_ip_list(&host, GBL_TARGET1);
   
   /* refresh the list */
   gtkui_create_targets_array();
}

static void add_target2(void)
{
   struct in_addr ip;
   struct ip_addr host;
   
   if (inet_aton(thost, &ip) == 0) {
      gtkui_message("Invalid ip address");
      return;
   }
   
   ip_addr_init(&host, AF_INET, (char *)&ip);

   add_ip_list(&host, GBL_TARGET2);
   
   /* refresh the list */
   gtkui_create_targets_array();
}

static void gtkui_delete_targets(GtkWidget *widget, gpointer data) {
   GList *list = NULL;
   GtkTreeIter iter;
   GtkTreeModel *model;
   struct ip_list *il = NULL;

   switch((int)data) {
      case 1:
         DEBUG_MSG("gtkui_delete_target: list 1");
         model = GTK_TREE_MODEL (liststore1);

         if(gtk_tree_selection_count_selected_rows(selection1) > 0) {
            list = gtk_tree_selection_get_selected_rows (selection1, &model);
            for(list = g_list_last(list); list; list = g_list_previous(list)) {
               gtk_tree_model_get_iter(model, &iter, list->data);
               gtk_tree_model_get (model, &iter, 1, &il, -1);

               /* remove the host from the list */
               del_ip_list(&il->ip, GBL_TARGET1);

               gtk_list_store_remove(GTK_LIST_STORE (liststore1), &iter);
            }
         }
         break;
      case 2:
         DEBUG_MSG("gtkui_delete_target: list 2");
         model = GTK_TREE_MODEL (liststore2);

         if(gtk_tree_selection_count_selected_rows(selection2) > 0) {
            list = gtk_tree_selection_get_selected_rows (selection2, &model);
            for(list = g_list_last(list); list; list = g_list_previous(list)) {
               gtk_tree_model_get_iter(model, &iter, list->data);
               gtk_tree_model_get (model, &iter, 1, &il, -1);

               /* remove the host from the list */
               del_ip_list(&il->ip, GBL_TARGET2);

               gtk_list_store_remove(GTK_LIST_STORE (liststore2), &iter);
            }
         }
         break;
   }
   
   /* free the list of selections */
   if(list) {
      g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);
   }
}

/* EOF */

// vim:ts=3:expandtab

