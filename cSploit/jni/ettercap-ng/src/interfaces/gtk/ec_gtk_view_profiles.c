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
#include <ec_format.h>
#include <ec_profiles.h>
#include <ec_manuf.h>
#include <ec_services.h>

/* proto */

void gtkui_show_profiles(void);
static void gtkui_profiles_detach(GtkWidget *child);
static void gtkui_profiles_attach(void);
static void gtkui_kill_profiles(void);
static gboolean refresh_profiles(gpointer data);
static void gtkui_profile_detail(void);
static void gtkui_profiles_local(void);
static void gtkui_profiles_remote(void);
static void gtkui_profiles_convert(void);
static void gtkui_profiles_dump(void *dummy);
static void dump_profiles(void);

extern void gtkui_refresh_host_list(void);
static struct host_profile *gtkui_profile_selected(void);

/* globals */

static char *logfile = NULL;
static GtkWidget  *profiles_window = NULL;
static GtkWidget         *treeview = NULL;
static GtkTreeSelection *selection = NULL;
static GtkListStore     *ls_profiles = NULL;
static guint profiles_idle; /* for removing the idle call */

/*******************************************/

/*
 * the auto-refreshing list of profiles 
 */
void gtkui_show_profiles(void)
{
   GtkWidget *scrolled, *vbox, *hbox, *button;
   GtkCellRenderer   *renderer;
   GtkTreeViewColumn *column;  

   DEBUG_MSG("gtk_show_profiles");

   /* if the object already exist, set the focus to it */
   if(profiles_window) {
      if(GTK_IS_WINDOW (profiles_window))
         gtk_window_present(GTK_WINDOW (profiles_window));
      else
         gtkui_page_present(profiles_window);
      return;
   }
   
   profiles_window = gtkui_page_new("Profiles", &gtkui_kill_profiles, &gtkui_profiles_detach);

   vbox = gtk_vbox_new(FALSE, 0);
   gtk_container_add(GTK_CONTAINER (profiles_window), vbox);
   gtk_widget_show(vbox);

  /* list */
   scrolled = gtk_scrolled_window_new(NULL, NULL);
   gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW (scrolled), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
   gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW (scrolled), GTK_SHADOW_IN);
   gtk_box_pack_start(GTK_BOX(vbox), scrolled, TRUE, TRUE, 0);
   gtk_widget_show(scrolled);

   treeview = gtk_tree_view_new();
   gtk_container_add(GTK_CONTAINER (scrolled), treeview);
   gtk_widget_show(treeview);

   selection = gtk_tree_view_get_selection (GTK_TREE_VIEW (treeview));
   gtk_tree_selection_set_mode (selection, GTK_SELECTION_SINGLE);
   g_signal_connect (G_OBJECT (treeview), "row_activated", G_CALLBACK (gtkui_profile_detail), NULL);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes (" ", renderer, "text", 0, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 0);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("IP Address", renderer, "text", 1, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 1);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   renderer = gtk_cell_renderer_text_new ();
   column = gtk_tree_view_column_new_with_attributes ("Hostname", renderer, "text", 2, NULL);
   gtk_tree_view_column_set_sort_column_id (column, 2);
   gtk_tree_view_append_column (GTK_TREE_VIEW(treeview), column);

   refresh_profiles(NULL);
   gtk_tree_view_set_model(GTK_TREE_VIEW (treeview), GTK_TREE_MODEL (ls_profiles));

   hbox = gtk_hbox_new(TRUE, 5);
   gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);

   button = gtk_button_new_with_mnemonic("Purge _Local");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_profiles_local), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

   button = gtk_button_new_with_mnemonic("Purge _Remote");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_profiles_remote), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   gtk_widget_show_all(hbox);

   button = gtk_button_new_with_mnemonic("_Convert to Host List");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_profiles_convert), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);

   button = gtk_button_new_with_mnemonic("_Dump to File");
   g_signal_connect(G_OBJECT (button), "clicked", G_CALLBACK (gtkui_profiles_dump), NULL);
   gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
   gtk_widget_show_all(hbox);

   gtk_widget_show(profiles_window);

   /* refresh the stats window every 1000 ms */
   /* GTK has a gtk_idle_add also but it calls too much and uses 100% cpu */
   profiles_idle = gtk_timeout_add(1000, refresh_profiles, NULL);
}

static void gtkui_profiles_detach(GtkWidget *child)
{
   profiles_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
   gtk_window_set_title(GTK_WINDOW (profiles_window), "Collected passive profiles");
   gtk_window_set_default_size(GTK_WINDOW (profiles_window), 400, 300);
   g_signal_connect (G_OBJECT (profiles_window), "delete_event", G_CALLBACK (gtkui_kill_profiles), NULL);

   /* make <ctrl>d shortcut turn the window back into a tab */
   gtkui_page_attach_shortcut(profiles_window, gtkui_profiles_attach);

   gtk_container_add(GTK_CONTAINER (profiles_window), child);

   gtk_window_present(GTK_WINDOW (profiles_window));
}

static void gtkui_profiles_attach(void)
{
   gtkui_kill_profiles();
   gtkui_show_profiles();
}

static void gtkui_kill_profiles(void)
{
   DEBUG_MSG("gtk_kill_profiles");

   gtk_timeout_remove(profiles_idle);

   gtk_widget_destroy(profiles_window);
   profiles_window = NULL;
}

static gboolean refresh_profiles(gpointer data)
{
   GtkTreeIter iter;
   GtkTreeModel *model;
   gboolean gotiter = FALSE;
   struct host_profile *hcurr, *hitem;
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   int found = 0;

   if(!ls_profiles) {
      ls_profiles = gtk_list_store_new (4, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_POINTER);
   }

   /* get iter for first item in list widget */
   model = GTK_TREE_MODEL(ls_profiles);
   gotiter = gtk_tree_model_get_iter_first(model, &iter);

   TAILQ_FOREACH(hcurr, &GBL_PROFILES, next) {
      /* see if the item is already in our list */
      gotiter = gtk_tree_model_get_iter_first(model, &iter);
      while(gotiter) {
         gtk_tree_model_get (model, &iter, 3, &hitem, -1);
         if(hcurr == hitem) {
            found = 0;
            /* search at least one account */
            LIST_FOREACH(o, &(hcurr->open_ports_head), next) {
               LIST_FOREACH(u, &(o->users_list_head), next) {
                  found = 1;
               }
            }

            gtk_list_store_set (ls_profiles, &iter, 0, (found)?"X":" ", -1);
            break;
         }
         gotiter = gtk_tree_model_iter_next(model, &iter);
      }

      /* if it is, move on to next item */
      if(gotiter)
         continue;

      found = 0;
      /* search at least one account */
      LIST_FOREACH(o, &(hcurr->open_ports_head), next) {
         LIST_FOREACH(u, &(o->users_list_head), next) {
            found = 1;
         }
      }

      /* otherwise, add the new item */
      gtk_list_store_append (ls_profiles, &iter);

      gtk_list_store_set (ls_profiles, &iter, 
                          0, (found)?"X":" ",
                          1, ip_addr_ntoa(&hcurr->L3_addr, tmp), 
                          2, (hcurr->hostname) ? hcurr->hostname : "",
                          3, hcurr, -1);
   }

   return TRUE;
}

/*
 * display details for a profile
 */
static void gtkui_profile_detail(void)
{
   GtkTextBuffer *textbuf;
   char line[200];

   struct host_profile *h = gtkui_profile_selected();
   struct open_port *o;
   struct active_user *u;
   char tmp[MAX_ASCII_ADDR_LEN];
   char os[OS_LEN+1];
   
   DEBUG_MSG("gtkui_profile_detail");

   textbuf = gtkui_details_window("Profile Details");

   memset(os, 0, sizeof(os));
   
   snprintf(line, 200, " IP Address: \t%s \n", ip_addr_ntoa(&h->L3_addr, tmp));
   gtkui_details_print(textbuf, line);

   if (strcmp(h->hostname, ""))
      snprintf(line, 200, " Hostname: \t%s \n\n", h->hostname);
   else
      snprintf(line, 200, "\n");   
   gtkui_details_print(textbuf, line);
      
   if (h->type & FP_HOST_LOCAL || h->type == FP_UNKNOWN) {
      snprintf(line, 200, " MAC address:  \t%s \n", mac_addr_ntoa(h->L2_addr, tmp));
      gtkui_details_print(textbuf, line);
      snprintf(line, 200, " Manufacturer: \t%s \n\n", manuf_search(h->L2_addr));
      gtkui_details_print(textbuf, line);
   }

   snprintf(line, 200, " Distance: \t\t%d   \n", h->distance);
   gtkui_details_print(textbuf, line);
   if (h->type & FP_GATEWAY)
      snprintf(line, 200, " Type: \t\tGATEWAY\n\n");
   else if (h->type & FP_HOST_LOCAL)
      snprintf(line, 200, " Type: \t\tLAN host\n\n");
   else if (h->type & FP_ROUTER)
      snprintf(line, 200, " Type: \t\tREMOTE ROUTER\n\n");
   else if (h->type & FP_HOST_NONLOCAL)
      snprintf(line, 200, " Type: \t\tREMOTE host\n\n");
   else if (h->type == FP_UNKNOWN)
      snprintf(line, 200, " Type: \t\tunknown\n\n");
   gtkui_details_print(textbuf, line);

   if (h->os)
   {
      snprintf(line, 200, " Observed OS:\t%s\n\n", h->os);
      gtkui_details_print(textbuf, line);
   }

   snprintf(line, 200, " Fingerprint: \t%s\n", h->fingerprint);
   gtkui_details_print(textbuf, line);
   if (fingerprint_search(h->fingerprint, os) == ESUCCESS)
      snprintf(line, 200, " Operating System: \t%s \n\n", os);
   else {
      snprintf(line, 200, " Operating System: \tunknown fingerprint (please submit it) \n");
      gtkui_details_print(textbuf, line);
      snprintf(line, 200, " Nearest one is: \t%s \n\n", os);
   }
   gtkui_details_print(textbuf, line);
      
   
   LIST_FOREACH(o, &(h->open_ports_head), next) {
      
      snprintf(line, 200, "   Port: \t%s %d | %s \t[%s]\n", 
                  (o->L4_proto == NL_TYPE_TCP) ? "TCP" : "UDP" , 
                  ntohs(o->L4_addr),
                  service_search(o->L4_addr, o->L4_proto), 
                  (o->banner) ? o->banner : "");
      gtkui_details_print(textbuf, line);
      
      LIST_FOREACH(u, &(o->users_list_head), next) {
        
         if (u->failed)
            snprintf(line, 200, "   Account: * %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         else
            snprintf(line, 200, "   Account: %s / %s  (%s)\n", u->user, u->pass, ip_addr_ntoa(&u->client, tmp));
         gtkui_details_print(textbuf, line);
         if (u->info)
            snprintf(line, 200, "   Info: \t%s\n\n", u->info);
         else
            snprintf(line, 200, "\n");
         gtkui_details_print(textbuf, line);
      }
   }
}

static void gtkui_profiles_local(void)
{
   profile_purge_local();
   gtk_list_store_clear(GTK_LIST_STORE (ls_profiles));
}

static void gtkui_profiles_remote(void)
{
   profile_purge_remote();
   gtk_list_store_clear(GTK_LIST_STORE (ls_profiles));
}

static void gtkui_profiles_convert(void)
{
   profile_convert_to_hostlist();
   gtkui_refresh_host_list();
   gtkui_message("The hosts list was populated with local profiles");
}

static void gtkui_profiles_dump(void *dummy)
{
   DEBUG_MSG("gtkui_profiles_dump");

   /* make sure to free if already set */
   SAFE_FREE(logfile);
   SAFE_CALLOC(logfile, 50, sizeof(char));

   gtkui_input("Log File :", logfile, 50, dump_profiles);

}

static void dump_profiles(void)
{
   /* dump the profiles */
   if (profile_dump_to_file(logfile) == ESUCCESS)
      gtkui_message("Profiles dumped to file");
}

static struct host_profile *gtkui_profile_selected(void) {
   GtkTreeIter iter;
   GtkTreeModel *model;
   struct host_profile *h = NULL;

   model = GTK_TREE_MODEL (ls_profiles);

   if (gtk_tree_selection_get_selected (GTK_TREE_SELECTION (selection), &model, &iter)) {
      gtk_tree_model_get (model, &iter, 3, &h, -1);
   } else
      return(NULL); /* nothing is selected */

   return(h);
}

/* EOF */

// vim:ts=3:expandtab

