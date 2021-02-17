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

/* globals */

GtkItemFactoryEntry gmenu_start[] = {
   {"/_Start",                NULL,          NULL,                 0, "<Branch>" },
   {"/Start/Start sniffing",  "<control>w",  gtkui_start_sniffing, 0, "<StockItem>", GTK_STOCK_YES },
   {"/Start/Stop sniffing",   "<control>e",  gtkui_stop_sniffing,  0, "<StockItem>", GTK_STOCK_NO },
   {"/Start/sep1",            NULL,          NULL,                 0, "<Separator>" },
   {"/Start/E_xit",           "<control>x",  gtkui_exit,        0, "<StockItem>", GTK_STOCK_QUIT }
};

GtkItemFactoryEntry gmenu_targets[] = {
   {"/_Targets",                 NULL,          NULL,                  0, "<Branch>" },
   {"/Targets/Current _Targets", "t",           gtkui_current_targets, 0, "<StockItem>", GTK_STOCK_FIND },
   {"/Targets/Select TARGET(s)", "<control>t",  gtkui_select_targets,  0, "<StockItem>", GTK_STOCK_ADD },
   {"/Targets/sep1",             NULL,          NULL,                  0, "<Separator>" },
   {"/Targets/_Protocol...",     "p",           gtkui_select_protocol, 0, "<StockItem>", GTK_STOCK_JUMP_TO },
   {"/Targets/Reverse matching", NULL,          toggle_reverse,        0, "<ToggleItem>" },
   {"/Targets/sep2",             NULL,          NULL,                  0, "<Separator>" },
   {"/Targets/_Wipe targets",    "<shift>W",    wipe_targets,          0, "<StockItem>", GTK_STOCK_CLEAR }
};

GtkItemFactoryEntry gmenu_hosts[] = {
   {"/_Hosts",                  NULL,         NULL,             0, "<Branch>" },
   {"/Hosts/_Hosts list",       "h",          gtkui_host_list,  0, "<StockItem>", GTK_STOCK_INDEX },
   {"/Hosts/sep1",              NULL,         NULL,             0, "<Separator>" },
   {"/Hosts/_Scan for hosts",   "<control>s", gtkui_scan,       0, "<StockItem>", GTK_STOCK_FIND },
   {"/Hosts/Load from file...", NULL,         gtkui_load_hosts, 0, "<StockItem>", GTK_STOCK_OPEN },
   {"/Hosts/Save to file...",   NULL,         gtkui_save_hosts, 0, "<StockItem>", GTK_STOCK_SAVE }
};

GtkItemFactoryEntry gmenu_view[] = {
   {"/_View",                        NULL, NULL,                   0, "<Branch>" },
   {"/View/_Connections",      "<shift>C", gtkui_show_connections, 0, "<StockItem>", GTK_STOCK_JUSTIFY_FILL },
   {"/View/Pr_ofiles",         "<shift>O", gtkui_show_profiles,    0, "<StockItem>", GTK_STOCK_JUSTIFY_LEFT },
   {"/View/_Statistics",              "s", gtkui_show_stats,       0, "<StockItem>", GTK_STOCK_PROPERTIES },
   {"/View/sep1",                    NULL, NULL,                   0, "<Separator>" },
   {"/View/Resolve IP addresses",    NULL, toggle_resolve,         0, "<ToggleItem>" },
   {"/View/_Visualization method...", "v", gtkui_vis_method,       0, "<StockItem>", GTK_STOCK_PREFERENCES },
   {"/View/Visualization _regex...",  "R", gtkui_vis_regex,        0, "<StockItem>", GTK_STOCK_FIND },
   {"/View/sep1",                    NULL, NULL,                   0, "<Separator>" },
   {"/View/Set the _WiFi key...",      "w", gtkui_wifi_key,          0, "<StockItem>", GTK_STOCK_FIND }
};

GtkItemFactoryEntry gmenu_mitm[] = {
   {"/_Mitm",                    NULL, NULL,                0, "<Branch>" },
   {"/Mitm/Arp poisoning...",    NULL, gtkui_arp_poisoning, 0, "<Item>" },
   {"/Mitm/Icmp redirect...",    NULL, gtkui_icmp_redir,    0, "<Item>" },
   {"/Mitm/Port stealing...",    NULL, gtkui_port_stealing, 0, "<Item>" },
   {"/Mitm/Dhcp spoofing...",    NULL, gtkui_dhcp_spoofing, 0, "<Item>" },
   {"/Mitm/sep1",                NULL, NULL,                0, "<Separator>" },
   {"/Mitm/Stop mitm attack(s)", NULL, gtkui_mitm_stop,     0, "<StockItem>", GTK_STOCK_STOP }
};

GtkItemFactoryEntry gmenu_filters[] = {
   {"/_Filters",                 NULL,         NULL,              0, "<Branch>" },
   {"/Filters/Load a filter...", "<control>f", gtkui_load_filter, 0, "<StockItem>", GTK_STOCK_OPEN },
   {"/Filters/Stop _filtering",  "f",          gtkui_stop_filter, 0, "<StockItem>", GTK_STOCK_STOP }
};

GtkItemFactoryEntry gmenu_logging[] = {
   {"/_Logging",                             NULL, NULL,            0, "<Branch>" },
   {"/Logging/Log all packets and infos...", "<shift>I", gtkui_log_all, 0, "<StockItem>", GTK_STOCK_SAVE },
   {"/Logging/Log only infos...",            "i",  gtkui_log_info,  0, "<StockItem>", GTK_STOCK_SAVE_AS },
   {"/Logging/Stop logging infos",           NULL, gtkui_stop_log,  0, "<StockItem>", GTK_STOCK_STOP },
   {"/Logging/sep1",                         NULL, NULL,            0, "<Separator>" },
   {"/Logging/Log user messages...",         "m",  gtkui_log_msg,   0, "<StockItem>", GTK_STOCK_REVERT_TO_SAVED },
   {"/Logging/Stop logging messages",        NULL, gtkui_stop_msg,  0, "<StockItem>", GTK_STOCK_STOP },
   {"/Logging/sep2",                         NULL, NULL,            0, "<Separator>" },
   {"/Logging/Compressed file",              NULL, toggle_compress, 0, "<ToggleItem>" }
};

GtkItemFactoryEntry gmenu_plugins[] = {
   {"/_Plugins",                   NULL,         NULL,              0, "<Branch>" },
   {"/Plugins/Manage the plugins", "<control>p", gtkui_plugin_mgmt, 0, "<StockItem>", GTK_STOCK_EXECUTE },
   {"/Plugins/Load a plugin...",   NULL,         gtkui_plugin_load, 0, "<StockItem>", GTK_STOCK_OPEN }
};

#ifndef OS_WINDOWS
GtkItemFactoryEntry gmenu_help[] = {
   {"/_?",                   NULL,         NULL,              0, "<Branch>" },
   {"/?/Contents", " ", gtkui_help, 0, "<StockItem>", GTK_STOCK_HELP }
};
#endif

GtkItemFactoryEntry tab_menu[] = {
  { "/Detach page",    "<control>D", gtkui_page_detach_current, 0, "<StockItem>", GTK_STOCK_GO_UP },
  { "/Close page",     "<control>Q", gtkui_page_close_current,  0, "<StockItem>", GTK_STOCK_CLOSE },
  { "/sep1",           NULL,         NULL,                      0, "<Separator>"  },
  { "/Next page",      "<control>0", gtkui_page_right,    0, "<StockItem>", GTK_STOCK_GO_FORWARD },
  { "/Previous page",  "<control>9", gtkui_page_left,     0, "<StockItem>", GTK_STOCK_GO_BACK }
};

/* proto */

void gtkui_create_menu(int live);


/*******************************************/


void gtkui_create_menu(int live)
{
   GtkAccelGroup *accel_group;
   GtkWidget *vbox, *item;
   GtkItemFactory *root_menu;
   int num_items = 0;
   
   DEBUG_MSG("gtk_create_menu");

   /* remove old menu, it will be automatically destroyed by gtk_main */
   vbox = gtk_bin_get_child(GTK_BIN (window));
   gtk_container_remove(GTK_CONTAINER (vbox), main_menu);

   /* Prepare to generate menus from the definitions in ec_gtk.h */
   accel_group = gtk_accel_group_new ();
   root_menu = gtk_item_factory_new (GTK_TYPE_MENU_BAR, "<main>", accel_group);
   gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);
   
   /* Start Menu */
   num_items = sizeof (gmenu_start) / sizeof (gmenu_start[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_start, NULL);
   
   /* Targets Menu */
   num_items = sizeof (gmenu_targets) / sizeof (gmenu_targets[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_targets, NULL);
   
   /* Hosts Menu */
   if (live > 0 && GBL_SNIFF->type != SM_BRIDGED) {
      num_items = sizeof (gmenu_hosts) / sizeof (gmenu_hosts[0]);
      gtk_item_factory_create_items (root_menu, num_items, gmenu_hosts, NULL);
   }
   
   /* View Menu */
   num_items = sizeof (gmenu_view) / sizeof (gmenu_view[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_view, NULL);
   
   /* MITM Menu */
   if (live > 0 && GBL_SNIFF->type != SM_BRIDGED) {
      num_items = sizeof (gmenu_mitm) / sizeof (gmenu_mitm[0]);
      gtk_item_factory_create_items (root_menu, num_items, gmenu_mitm, NULL);
   }
   
   /* Filters Menu */
   num_items = sizeof (gmenu_filters) / sizeof (gmenu_filters[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_filters, NULL);
   
   /* Logging Menu */
   num_items = sizeof (gmenu_logging) / sizeof (gmenu_logging[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_logging, NULL);

#ifdef HAVE_PLUGINS
   /* Plugins Menu */
   if(live > 0) {
      num_items = sizeof (gmenu_plugins) / sizeof (gmenu_plugins[0]);
      gtk_item_factory_create_items (root_menu, num_items, gmenu_plugins, NULL);
   }
#endif

#ifndef OS_WINDOWS
   /* Help Menu */
   num_items = sizeof (gmenu_help) / sizeof (gmenu_help[0]);
   gtk_item_factory_create_items (root_menu, num_items, gmenu_help, NULL);
#endif

   if(GBL_OPTIONS->reversed) {
      GBL_OPTIONS->reversed = 0;
      item = gtk_item_factory_get_item(root_menu, "/Targets/Reverse matching");
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (item), TRUE);
   }

   if(GBL_OPTIONS->resolve) {
      GBL_OPTIONS->resolve = 0;
      item = gtk_item_factory_get_item(root_menu, "/View/Resolve IP addresses");
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (item), TRUE);
   }

   if(GBL_OPTIONS->compress) {
      GBL_OPTIONS->compress = 0;
      item = gtk_item_factory_get_item(root_menu, "/Logging/Compressed file");
      gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM (item), TRUE);
   }

   /* get the menu widget and add it to the window */
   main_menu = gtk_item_factory_get_widget (root_menu, "<main>");
   gtk_box_pack_start(GTK_BOX(vbox), main_menu, FALSE, FALSE, 0);
   gtk_widget_show(main_menu);
}

void gtkui_create_tab_menu(void)
{
   GtkAccelGroup *accel_group;
   GtkWidget *context;
   GtkItemFactory *if_tabs;

   accel_group = gtk_accel_group_new ();
   gtk_window_add_accel_group (GTK_WINDOW (window), accel_group);

   /* context menu for notebook */
   if_tabs = gtk_item_factory_new(GTK_TYPE_MENU, "<notebook>", accel_group);
   gtk_item_factory_create_items(if_tabs, 5, tab_menu, NULL);
   context = gtk_item_factory_get_widget(if_tabs, "<notebook>");

   g_signal_connect(G_OBJECT(notebook), "button-press-event", G_CALLBACK(gtkui_context_menu), context);
}


/* EOF */

// vim:ts=3:expandtab

