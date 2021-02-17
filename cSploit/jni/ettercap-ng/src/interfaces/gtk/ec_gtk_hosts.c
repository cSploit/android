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
#include <ec_scan.h>

/* proto */

void gtkui_scan(void);
void gtkui_load_hosts(void);
void gtkui_save_hosts(void);
void gtkui_host_list(void);
void gtkui_refresh_host_list(void);

static void load_hosts(char *file);
static void save_hosts(void);
static void gtkui_hosts_destroy(void);
static void gtkui_button_callback(GtkWidget *widget, gpointer data);
static void gtkui_hosts_detach(GtkWidget *child);
static void gtkui_hosts_attach(void);

/* globals */
static GtkWidget      *hosts_window = NULL;
static GtkTreeSelection  *selection = NULL;
static GtkListStore      *liststore = NULL;
enum { HOST_DELETE, HOST_TARGET1, HOST_TARGET2 };

/*******************************************/

/*
 * scan the lan for hosts 
 */
void gtkui_scan(void)
{
   /* wipe the current list */
   del_hosts_list();

   /* no target defined...  force a full scan */
   if (GBL_TARGET1->all_ip && GBL_TARGET2->all_ip &&
      !GBL_TARGET1->scan_all && !GBL_TARGET2->scan_all) {
      GBL_TARGET1->scan_all = 1;
      GBL_TARGET2->scan_all = 1;
   }
   
   /* perform a new scan */
   build_hosts_list();
}

/*
 * display the file open dialog
 */
void gtkui_load_hosts(void)
{
   GtkWidget *dialog;
   const char *filename;
   int response = 0;

   DEBUG_MSG("gtk_load_hosts");

   dialog = gtk_file_selection_new ("Select a hosts file...");

   response = gtk_dialog_run (GTK_DIALOG (dialog));
   
   if (response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);
      filename = gtk_file_selection_get_filename (GTK_FILE_SELECTION (dialog));

      load_hosts(filename);
      gtkui_refresh_host_list();
   }
   gtk_widget_destroy (dialog);
}

static void load_hosts(char *file)
{
   char *tmp;
   char current[PATH_MAX];
   
   DEBUG_MSG("load_hosts %s", file);
   
   SAFE_CALLOC(tmp, strlen(file)+1, sizeof(char));

   /* get the current working directory */
   getcwd(current, PATH_MAX);

   /* we are opening a file in the current dir.
    * use the relative path, so we can open files
    * in the current dir even if the complete path
    * is not traversable with ec_uid permissions
    */
   if (!strncmp(current, file, strlen(current)))
      snprintf(tmp, strlen(file)+1,"./%s", file+strlen(current));
   else
      snprintf(tmp, strlen(file), "%s", file);

   DEBUG_MSG("load_hosts path == %s", tmp);

   /* wipe the current list */
   del_hosts_list();

   /* load the hosts list */
   scan_load_hosts(tmp);
   
   SAFE_FREE(tmp);
   
   gtkui_host_list();
}

/*
 * display the write file menu
 */
void gtkui_save_hosts(void)
{
#define FILE_LEN  40
   
   DEBUG_MSG("gtk_save_hosts");

   SAFE_FREE(GBL_OPTIONS->hostsfile);
   SAFE_CALLOC(GBL_OPTIONS->hostsfile, FILE_LEN, sizeof(char));
   
   gtkui_input("Output file :", GBL_OPTIONS->hostsfile, FILE_LEN, save_hosts);
}

static void save_hosts(void)
{
   FILE *f;
   
   /* check if the file is writeable */
   f = fopen(GBL_OPTIONS->hostsfile, "w");
   if (f == NULL) {
      ui_error("Cannot write %s", GBL_OPTIONS->hostsfile);
      SAFE_FREE(GBL_OPTIONS->hostsfile);
      return;
   }
 
   /* if ok, delete it */
   fclose(f);
   unlink(GBL_OPTIONS->hostsfile);
   
   scan_save_hosts(GBL_OPTIONS->hostsfile);
}

/*
 * display the host list 
 */
void gtkui_host_list(void)
{
   GtkWidget *scrolled, *treeview, *vbox, *hbox, *button;
   GtkCellRenderer   *renderer;
   GtkTreeViewColumn *column;

   DEBUG_MSG("gtk_host_list");

   if(hosts_window) {
      if(GTK_IS_WINDOW (hosts_window))
         gtk_window_present(GTK_WINDOW (hosts_window));
      else
         gtkui_page_present(hosts_window);
      return;
   }
   
   hosts_window = gtkui_page_new("Host List", &gtkui_hosts_destroy, &gtkui_hosts_detach);

   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER (hosts_window), vbox);
   gtk_widget_show(vbox);

   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
   gtk_widget_show(scrolled);

   treeview = gtk_tree_view_new();
   gtk_container_add(GTK_CONTAINER (scrolled), treeview);
   gtk_widget_show(treeview);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_MULTIPLE);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("IP Address", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("MAC Address", renderer, "text", 1, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 1);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Description", renderer, "text", 2, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 2);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   /* populate the list or at least allocate a spot for it */
   gtkui_refresh_host_list();
  
   /* set the elements */
   gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (liststore));

   hbox = gtk_hbox_new(TRUE, 0);
   gtk_box_pack_start(GTK_BOX (vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);

   button = gtk_button_new_with_mnemonic("_Delete Host");
   gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_button_callback), (gpointer)HOST_DELETE);
   gtk_widget_show(button);

   button = gtk_button_new_with_mnemonic("Add to Target _1");
   gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_button_callback), (gpointer)HOST_TARGET1);
   gtk_widget_show(button);

   button = gtk_button_new_with_mnemonic("Add to Target _2");
   gtk_box_pack_start(GTK_BOX (hbox), button, TRUE, TRUE, 0);
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_button_callback), (gpointer)HOST_TARGET2);
   gtk_widget_show(button);

   gtk_widget_show(hosts_window);
}

static void gtkui_hosts_detach(GtkWidget *child)
{
   hosts_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (hosts_window), "Hosts list");
   gtk_window_set_default_size(GTK_WINDOW (hosts_window), 400, 300);
   g_signal_connect (G_OBJECT (hosts_window), "delete_event", G_CALLBACK (gtkui_hosts_destroy), NULL);

   gtk_container_add(GTK_CONTAINER (hosts_window), child);

   /* make <ctrl>d shortcut turn the window back into a tab */
   gtkui_page_attach_shortcut(hosts_window, gtkui_hosts_attach);

   gtk_window_present(GTK_WINDOW (hosts_window));
}

static void gtkui_hosts_attach(void)
{
   /* destroy the current window */
   gtkui_hosts_destroy();

   /* recreate the tab */
   gtkui_host_list();
}

void gtkui_hosts_destroy(void)
{
   gtk_widget_destroy(hosts_window);
   hosts_window = NULL;
}

/*
 * populate the list
 */
void gtkui_refresh_host_list(void)
{
   GtkTreeIter   iter;
   struct hosts_list *hl;
   char tmp[MAX_ASCII_ADDR_LEN];
   char tmp2[MAX_ASCII_ADDR_LEN];
   char name[MAX_HOSTNAME_LEN];

   DEBUG_MSG("gtk_refresh_host_list");

   /* The list store contains a 4th column that is NOT displayed 
      by the treeview widget. This is used to store the pointer
      for each entry's structure. */
   
   if(liststore) 
      gtk_list_store_clear(GTK_LIST_STORE (liststore));
   else
      liststore = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);

   /* walk the hosts list */
   LIST_FOREACH(hl, &GBL_HOSTLIST, next) {
      /* enlarge the list */ 
      gtk_list_store_append (liststore, &iter);
      /* fill the element */
      gtk_list_store_set (liststore, &iter, 
                          0, ip_addr_ntoa(&hl->ip, tmp),
                          1, mac_addr_ntoa(hl->mac, tmp2),
                          3, hl, -1);
      if (hl->hostname) {
         gtk_list_store_set (liststore, &iter, 2, hl->hostname, -1);
      } else {
         /* resolve the hostname (using the cache) */
         host_iptoa(&hl->ip, name);
         gtk_list_store_set (liststore, &iter, 2, name, -1);
      }
   }
}

void gtkui_button_callback(GtkWidget *widget, gpointer data)
{
   GList *list;
   GtkTreeIter iter;
   GtkTreeModel *model;
   char tmp[MAX_ASCII_ADDR_LEN];
   struct hosts_list *hl = NULL;

   model = GTK_TREE_MODEL (liststore);

   if(gtk_tree_selection_count_selected_rows(selection) > 0) {
      list = gtk_tree_selection_get_selected_rows (selection, &model);
      for(list = g_list_last(list); list; list = g_list_previous(list)) {
         gtk_tree_model_get_iter(model, &iter, list->data);
         gtk_tree_model_get(model, &iter, 3, &hl, -1);

         switch((int)data) {
            case HOST_DELETE:
               DEBUG_MSG("gtkui_button_callback: delete host");
               gtk_list_store_remove(GTK_LIST_STORE (liststore), &iter);

               /* remove the host from the list */
               LIST_REMOVE(hl, next);
               SAFE_FREE(hl->hostname);
               SAFE_FREE(hl);
               break;
            case HOST_TARGET1:
               DEBUG_MSG("gtkui_button_callback: add target1");
               /* add the ip to the target */
               add_ip_list(&hl->ip, GBL_TARGET1);
               gtkui_create_targets_array();

               USER_MSG("Host %s added to TARGET1\n", ip_addr_ntoa(&hl->ip, tmp));
               break;
            case HOST_TARGET2:
               DEBUG_MSG("gtkui_button_callback: add target2");
               /* add the ip to the target */
               add_ip_list(&hl->ip, GBL_TARGET2);
               gtkui_create_targets_array();

               USER_MSG("Host %s added to TARGET2\n", ip_addr_ntoa(&hl->ip, tmp));
               break;
         }
      }

      /* free the list of selections */
      g_list_foreach (list,(GFunc) gtk_tree_path_free, NULL);
      g_list_free (list);
   }
}


/* EOF */

// vim:ts=3:expandtab
