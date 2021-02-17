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
#include <ec_mitm.h>

/* proto */

void gtkui_arp_poisoning(void);
void gtkui_icmp_redir(void);
void gtkui_port_stealing(void);
void gtkui_dhcp_spoofing(void);
void gtkui_mitm_stop(void);

static void gtkui_start_mitm(void);

/* globals */

#define PARAMS_LEN   512

static char params[PARAMS_LEN+1];

/*******************************************/

void gtkui_arp_poisoning(void)
{
   GtkWidget *dialog, *vbox, *hbox, *image, *button1, *button2, *frame;
   gint response = 0;
   gboolean remote = FALSE;
   gboolean oneway = FALSE;

   DEBUG_MSG("gtk_arp_poisoning");
//   not needed, the \0 is already appended from snprintf
//   memset(params, '\0', PARAMS_LEN+1);

   dialog = gtk_dialog_new_with_buttons("MITM Attack: ARP Poisoning", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK, 
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);

   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
   gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.1);
   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
   gtk_widget_show(image);

   frame = gtk_frame_new("Optional parameters");
   gtk_container_set_border_width(GTK_CONTAINER (frame), 5);
   gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   vbox = gtk_vbox_new (FALSE, 2);
   gtk_container_set_border_width(GTK_CONTAINER (vbox), 5);
   gtk_container_add(GTK_CONTAINER (frame), vbox);
   gtk_widget_show(vbox);

   button1 = gtk_check_button_new_with_label("Sniff remote connections.");
   gtk_box_pack_start(GTK_BOX (vbox), button1, FALSE, FALSE, 0);
   gtk_widget_show(button1);

   button2 = gtk_check_button_new_with_label("Only poison one-way.");
   gtk_box_pack_start(GTK_BOX (vbox), button2, FALSE, FALSE, 0);
   gtk_widget_show(button2);

   response = gtk_dialog_run(GTK_DIALOG(dialog));
   if(response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);
      const char *s_remote = "", *comma = "", *s_oneway = "";

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button1))) {
	s_remote="remote";
        remote = TRUE;
      }

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button2))) {
         if(remote)
		comma = ",";

	 s_oneway = "oneway";	
         oneway = TRUE;
      } 

      if(!remote && !oneway) {
         ui_error("You must select at least one ARP mode");
         return;
      }
      snprintf(params, PARAMS_LEN+1, "arp:%s%s%s", s_remote, comma, s_oneway);
      gtkui_start_mitm();
   }

   gtk_widget_destroy(dialog);

   /* a simpler method:
      gtkui_input_call("Parameters :", params + strlen("arp:"), PARAMS_LEN - strlen("arp:"), gtkui_start_mitm);
    */
}

void gtkui_icmp_redir(void)
{
   GtkWidget *dialog, *table, *hbox, *image, *label, *entry1, *entry2, *frame;
   gint response = 0;

   DEBUG_MSG("gtk_icmp_redir");

   dialog = gtk_dialog_new_with_buttons("MITM Attack: ICMP Redirect", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);

   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
   gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.1);
   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
   gtk_widget_show(image);

   frame = gtk_frame_new("Gateway Information");
   gtk_container_set_border_width(GTK_CONTAINER (frame), 5);
   gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   table = gtk_table_new(2, 2, FALSE);
   gtk_table_set_row_spacings(GTK_TABLE (table), 5);
   gtk_table_set_col_spacings(GTK_TABLE (table), 5);
   gtk_container_set_border_width(GTK_CONTAINER (table), 8);
   gtk_container_add(GTK_CONTAINER (frame), table);
   gtk_widget_show(table);

   label = gtk_label_new("MAC Address");
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry1 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry1), ETH_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry1, 1, 2, 0, 1); 
   gtk_widget_show(entry1);

   label = gtk_label_new("IP Address");
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry2 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry2), IP6_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry2, 1, 2, 1, 2);
   gtk_widget_show(entry2);

   response = gtk_dialog_run(GTK_DIALOG(dialog));
   if(response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);

//      memset(params, '\0', PARAMS_LEN);

      snprintf(params, PARAMS_LEN+1, "icmp:%s/%s",  gtk_entry_get_text(GTK_ENTRY(entry1)),
                       gtk_entry_get_text(GTK_ENTRY(entry2)));

      gtkui_start_mitm();
   }

   gtk_widget_destroy(dialog);

   /* a simpler method:
      gtkui_input_call("Parameters :", params + strlen("icmp:"), PARAMS_LEN - strlen("icmp:"), gtkui_start_mitm);
    */
}

void gtkui_port_stealing(void)
{
   GtkWidget *dialog, *vbox, *hbox, *image, *button1, *button2, *frame;
   gint response = 0;
   gboolean remote = FALSE;
   
   DEBUG_MSG("gtk_port_stealing"); 
      
   dialog = gtk_dialog_new_with_buttons("MITM Attack: Port Stealing", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);
         
   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);
         
   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
   gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.1);
   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
   gtk_widget_show(image);
      
   frame = gtk_frame_new("Optional parameters");
   gtk_container_set_border_width(GTK_CONTAINER (frame), 5);
   gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);

   vbox = gtk_vbox_new (FALSE, 2);
   gtk_container_set_border_width(GTK_CONTAINER (vbox), 5);
   gtk_container_add(GTK_CONTAINER (frame), vbox);
   gtk_widget_show(vbox);

   button1 = gtk_check_button_new_with_label("Sniff remote connections.");
   gtk_box_pack_start(GTK_BOX (vbox), button1, FALSE, FALSE, 0);
   gtk_widget_show(button1);
   
   button2 = gtk_check_button_new_with_label("Propagate to other switches.");
   gtk_box_pack_start(GTK_BOX (vbox), button2, FALSE, FALSE, 0);
   gtk_widget_show(button2);

   response = gtk_dialog_run(GTK_DIALOG(dialog));
   if(response == GTK_RESPONSE_OK) {    
      gtk_widget_hide(dialog);          
      const char *s_remote= "", *tree = "", *comma = "";

      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button1))) {
         s_remote="remote";
         remote = TRUE;
      }
   
      if(gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (button2))) {
         if(remote)
              comma = ",";
	 tree = "tree";
      }
  
      snprintf(params, PARAMS_LEN+1, "port:%s%s%s", s_remote, comma, tree); 
      gtkui_start_mitm();
   }

   gtk_widget_destroy(dialog);

   /* a simpler method: 
      gtkui_input_call("Parameters :", params + strlen("port:"), PARAMS_LEN - strlen("port:"), gtkui_start_mitm);
    */
}

void gtkui_dhcp_spoofing(void)
{
   GtkWidget *dialog, *table, *hbox, *image, *label, *entry1, *entry2, *entry3, *frame;
   gint response = 0;
   
   DEBUG_MSG("gtk_dhcp_spoofing");
//   memset(params, '\0', PARAMS_LEN+1);
   
   dialog = gtk_dialog_new_with_buttons("MITM Attack: DHCP Spoofing", GTK_WINDOW (window),
                                        GTK_DIALOG_MODAL, GTK_STOCK_OK, GTK_RESPONSE_OK,
                                        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, NULL);
   gtk_container_set_border_width(GTK_CONTAINER (dialog), 5);
   gtk_dialog_set_has_separator(GTK_DIALOG (dialog), FALSE);

   hbox = gtk_hbox_new (FALSE, 5);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG (dialog)->vbox), hbox, FALSE, FALSE, 0);
   gtk_widget_show(hbox);
   
   image = gtk_image_new_from_stock (GTK_STOCK_DIALOG_QUESTION, GTK_ICON_SIZE_DIALOG);
   gtk_misc_set_alignment (GTK_MISC (image), 0.5, 0.1);
   gtk_box_pack_start (GTK_BOX (hbox), image, FALSE, FALSE, 5);
   gtk_widget_show(image);

   frame = gtk_frame_new("Server Information");
   gtk_container_set_border_width(GTK_CONTAINER (frame), 5);
   gtk_box_pack_start (GTK_BOX (hbox), frame, TRUE, TRUE, 0);
   gtk_widget_show(frame);
      
   table = gtk_table_new(3, 2, FALSE);
   gtk_table_set_row_spacings(GTK_TABLE (table), 5);
   gtk_table_set_col_spacings(GTK_TABLE (table), 5);
   gtk_container_set_border_width(GTK_CONTAINER (table), 8);
   gtk_container_add(GTK_CONTAINER (frame), table);
   gtk_widget_show(table);

   label = gtk_label_new("IP Pool (optional)");
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 0, 1, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry1 = gtk_entry_new(); 
   gtk_table_attach_defaults(GTK_TABLE (table), entry1, 1, 2, 0, 1);
   gtk_widget_show(entry1);
   
   label = gtk_label_new("Netmask"); 
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 1, 2, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry2 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry2), IP6_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry2, 1, 2, 1, 2);
   gtk_widget_show(entry2);

   label = gtk_label_new("DNS Server IP");   
   gtk_misc_set_alignment(GTK_MISC (label), 0, 0.5);
   gtk_table_attach(GTK_TABLE (table), label, 0, 1, 2, 3, GTK_FILL, GTK_FILL, 0, 0);
   gtk_widget_show(label);

   entry3 = gtk_entry_new();
   gtk_entry_set_max_length(GTK_ENTRY (entry3), IP6_ASCII_ADDR_LEN);
   gtk_table_attach_defaults(GTK_TABLE (table), entry3, 1, 2, 2, 3);
   gtk_widget_show(entry3);

   response = gtk_dialog_run(GTK_DIALOG(dialog));
   if(response == GTK_RESPONSE_OK) {
      gtk_widget_hide(dialog);
//      memset(params, '\0', PARAMS_LEN);

      snprintf(params, PARAMS_LEN+1, "dhcp:%s/%s/%s", gtk_entry_get_text(GTK_ENTRY(entry1)),
                       gtk_entry_get_text(GTK_ENTRY(entry2)), gtk_entry_get_text(GTK_ENTRY(entry3)));

      DEBUG_MSG("ec_gtk_dhcp: DHCP MITM %s", params);
      gtkui_start_mitm();
   }

   gtk_widget_destroy(dialog);

   /* a simpler method:
      gtkui_input_call("Parameters :", params + strlen("dhcp:"), PARAMS_LEN - strlen("dhcp:"), gtkui_start_mitm);
   */
}


/* 
 * start the mitm attack by passing the name and parameters 
 */
static void gtkui_start_mitm(void)
{
   DEBUG_MSG("gtk_start_mitm");
   
   mitm_set(params);
   mitm_start();
}


/*
 * stop all the mitm attack(s)
 */
void gtkui_mitm_stop(void)
{
   GtkWidget *dialog;
   
   DEBUG_MSG("gtk_mitm_stop");

   /* create the dialog */
   dialog = gtk_message_dialog_new(GTK_WINDOW (window), GTK_DIALOG_MODAL,
            GTK_MESSAGE_INFO, 0, "Stopping the mitm attack...");
   gtk_window_set_position(GTK_WINDOW (dialog), GTK_WIN_POS_CENTER);
   gtk_window_set_resizable(GTK_WINDOW (dialog), FALSE);
   gtk_widget_queue_draw(dialog);
   gtk_widget_show_now(dialog);

   /* for GTK to display the dialog now */
   while (gtk_events_pending ())
      gtk_main_iteration ();

   /* stop the mitm process */
   mitm_stop();

   gtk_widget_destroy(dialog);
   
   gtkui_message("MITM attack(s) stopped");
}

/* EOF */

// vim:ts=3:expandtab

