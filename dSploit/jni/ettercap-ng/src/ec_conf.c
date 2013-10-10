/*
    ettercap -- configuration (etter.conf) manipulation module

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

    $Id: ec_conf.c,v 1.39 2004/09/28 09:56:12 alor Exp $
*/

#include <ec.h>
#include <ec_conf.h>
#include <ec_file.h>
#include <ec_dissect.h>

/* globals */
   
/* used only to keep track of how many dissector are loaded */
int number_of_dissectors;
int number_of_ports;
   
static struct conf_entry privs[] = {
   { "ec_uid", NULL },
   { "ec_gid", NULL },
   { NULL, NULL },
};

static struct conf_entry mitm[] = {
   { "arp_storm_delay", NULL },
   { "arp_poison_delay", NULL },
   { "arp_poison_warm_up", NULL },
   { "arp_poison_icmp", NULL },
   { "arp_poison_reply", NULL },
   { "arp_poison_request", NULL },
   { "arp_poison_equal_mac", NULL },
   { "dhcp_lease_time", NULL },
   { "port_steal_delay", NULL },
   { "port_steal_send_delay", NULL },
   { NULL, NULL },
};

static struct conf_entry connections[] = {
   { "connection_timeout", NULL },
   { "connection_idle", NULL },
   { "connection_buffer", NULL },
   { "connect_timeout", NULL },
   { NULL, NULL },
};

static struct conf_entry stats[] = {
   { "sampling_rate", NULL },
   { NULL, NULL },
};

static struct conf_entry misc[] = {
   { "close_on_eof", NULL },
   { "store_profiles", NULL },
   { "aggressive_dissectors", NULL },
   { "skip_forwarded_pcks", NULL },
   { "checksum_warning", NULL },
   { "checksum_check", NULL },
   { NULL, NULL },
};

static struct conf_entry curses[] = {
   { "color_bg", NULL },
   { "color_fg", NULL },
   { "color_join1", NULL },
   { "color_join2", NULL },
   { "color_border", NULL },
   { "color_title", NULL },
   { "color_focus", NULL },
   { "color_menu_bg", NULL },
   { "color_menu_fg", NULL },
   { "color_window_bg", NULL },
   { "color_window_fg", NULL },
   { "color_selection_fg", NULL },
   { "color_selection_bg", NULL },
   { "color_error_bg", NULL },
   { "color_error_fg", NULL },
   { "color_error_border", NULL },
   { NULL, NULL },
};

static struct conf_entry strings[] = {
   { "redir_command_on", NULL },
   { "redir_command_off", NULL },
   { "remote_browser", NULL },
   { "utf8_encoding", NULL },
   { NULL, NULL },
};

/* this is fake, dissector use a different registration */
static struct conf_entry dissectors[] = {
   { "fake", &number_of_dissectors },
};

static struct conf_section sections[] = {
   { "privs", (struct conf_entry *)&privs},
   { "mitm", (struct conf_entry *)&mitm},
   { "connections", (struct conf_entry *)&connections},
   { "stats", (struct conf_entry *)&stats},
   { "misc", (struct conf_entry *)&misc},
   { "dissectors", (struct conf_entry *)&dissectors},
   { "curses", (struct conf_entry *)&curses},
   { "strings", (struct conf_entry *)&strings},
   { NULL, NULL },
};


/* protos */

void load_conf(void);
static void init_structures(void);
static void set_pointer(struct conf_entry *entry, char *name, void *ptr);

static void set_dissector(char *name, char *values, int lineno);
static struct conf_entry * search_section(char *title);
static void * search_entry(struct conf_entry *section, char *name);

/************************************************/

/*
 * since GBL_CONF is in the heap, it is not constant
 * so we have to initialize it here and not in the
 * structure definition
 */

static void init_structures(void)
{
   int i = 0, j = 0;
   
   DEBUG_MSG("init_structures");
   
   set_pointer((struct conf_entry *)&privs, "ec_uid", &GBL_CONF->ec_uid);
   set_pointer((struct conf_entry *)&privs, "ec_gid", &GBL_CONF->ec_gid);
   set_pointer((struct conf_entry *)&mitm, "arp_storm_delay", &GBL_CONF->arp_storm_delay);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_warm_up", &GBL_CONF->arp_poison_warm_up);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_delay", &GBL_CONF->arp_poison_delay);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_icmp", &GBL_CONF->arp_poison_icmp);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_reply", &GBL_CONF->arp_poison_reply);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_request", &GBL_CONF->arp_poison_request);
   set_pointer((struct conf_entry *)&mitm, "arp_poison_equal_mac", &GBL_CONF->arp_poison_equal_mac);
   set_pointer((struct conf_entry *)&mitm, "dhcp_lease_time", &GBL_CONF->dhcp_lease_time);
   set_pointer((struct conf_entry *)&mitm, "port_steal_delay", &GBL_CONF->port_steal_delay);
   set_pointer((struct conf_entry *)&mitm, "port_steal_send_delay", &GBL_CONF->port_steal_send_delay);
   set_pointer((struct conf_entry *)&connections, "connection_timeout", &GBL_CONF->connection_timeout);
   set_pointer((struct conf_entry *)&connections, "connection_idle", &GBL_CONF->connection_idle);
   set_pointer((struct conf_entry *)&connections, "connection_buffer", &GBL_CONF->connection_buffer);
   set_pointer((struct conf_entry *)&connections, "connect_timeout", &GBL_CONF->connect_timeout);
   set_pointer((struct conf_entry *)&stats, "sampling_rate", &GBL_CONF->sampling_rate);
   set_pointer((struct conf_entry *)&misc, "close_on_eof", &GBL_CONF->close_on_eof);
   set_pointer((struct conf_entry *)&misc, "store_profiles", &GBL_CONF->store_profiles);
   set_pointer((struct conf_entry *)&misc, "aggressive_dissectors", &GBL_CONF->aggressive_dissectors);
   set_pointer((struct conf_entry *)&misc, "skip_forwarded_pcks", &GBL_CONF->skip_forwarded);
   set_pointer((struct conf_entry *)&misc, "checksum_warning", &GBL_CONF->checksum_warning);
   set_pointer((struct conf_entry *)&misc, "checksum_check", &GBL_CONF->checksum_check);
   set_pointer((struct conf_entry *)&curses, "color_bg", &GBL_CONF->colors.bg);
   set_pointer((struct conf_entry *)&curses, "color_fg", &GBL_CONF->colors.fg);
   set_pointer((struct conf_entry *)&curses, "color_join1", &GBL_CONF->colors.join1);
   set_pointer((struct conf_entry *)&curses, "color_join2", &GBL_CONF->colors.join2);
   set_pointer((struct conf_entry *)&curses, "color_border", &GBL_CONF->colors.border);
   set_pointer((struct conf_entry *)&curses, "color_title", &GBL_CONF->colors.title);
   set_pointer((struct conf_entry *)&curses, "color_focus", &GBL_CONF->colors.focus);
   set_pointer((struct conf_entry *)&curses, "color_menu_bg", &GBL_CONF->colors.menu_bg);
   set_pointer((struct conf_entry *)&curses, "color_menu_fg", &GBL_CONF->colors.menu_fg);
   set_pointer((struct conf_entry *)&curses, "color_window_bg", &GBL_CONF->colors.window_bg);
   set_pointer((struct conf_entry *)&curses, "color_window_fg", &GBL_CONF->colors.window_fg);
   set_pointer((struct conf_entry *)&curses, "color_selection_bg", &GBL_CONF->colors.selection_bg);
   set_pointer((struct conf_entry *)&curses, "color_selection_fg", &GBL_CONF->colors.selection_fg);
   set_pointer((struct conf_entry *)&curses, "color_error_bg", &GBL_CONF->colors.error_bg);
   set_pointer((struct conf_entry *)&curses, "color_error_fg", &GBL_CONF->colors.error_fg);
   set_pointer((struct conf_entry *)&curses, "color_error_border", &GBL_CONF->colors.error_border);
   /* special case for strings */
   set_pointer((struct conf_entry *)&strings, "redir_command_on", &GBL_CONF->redir_command_on);
   set_pointer((struct conf_entry *)&strings, "redir_command_off", &GBL_CONF->redir_command_off);
   set_pointer((struct conf_entry *)&strings, "remote_browser", &GBL_CONF->remote_browser);
   set_pointer((struct conf_entry *)&strings, "utf8_encoding", &GBL_CONF->utf8_encoding);

   /* sanity check */
   do {
      do {
         if (sections[i].entries[j].value == NULL) {
            DEBUG_MSG("INVALID init: %s %s", sections[i].entries[j].name, sections[i].title);
            BUG("check the debug file...");
         }
      } while (sections[i].entries[++j].name != NULL);
      j = 0;
   } while (sections[++i].title != NULL);
}

/* 
 * associate the pointer to a struct
 */

static void set_pointer(struct conf_entry *entry, char *name, void *ptr)
{
   int i = 0;

   /* search the name */
   do {
      /* found ! set the pointer */
      if (!strcmp(entry[i].name, name))
         entry[i].value = ptr;
      
   } while (entry[++i].name != NULL);
}

/*
 * load the configuration from etter.conf file
 */

void load_conf(void)
{
   FILE *fc;
   char line[128];
   char *p, *q, **tmp;
   int lineno = 0;
   struct conf_entry *curr_section = NULL;
   void *value = NULL;

   /* initialize the structures */
   init_structures();
   
   DEBUG_MSG("load_conf");
  
   /* the user has specified an alternative config file */
   if (GBL_CONF->file) {
      DEBUG_MSG("load_conf: alternative config: %s", GBL_CONF->file);
      fc = fopen(GBL_CONF->file, FOPEN_READ_TEXT);
      ON_ERROR(fc, NULL, "Cannot open %s", GBL_CONF->file);
   } else {
      /* errors are handled by the function */
      fc = open_data("etc", ETTER_CONF, FOPEN_READ_TEXT);
      ON_ERROR(fc, NULL, "Cannot open %s", ETTER_CONF);
   }
  
   /* read the file */
   while (fgets(line, 128, fc) != 0) {
      
      /* update the line count */
      lineno++;
      
      /* trim out the comments */
      if ((p = strchr(line, '#')))
         *p = '\0';
      
      /* trim out the new line */
      if ((p = strchr(line, '\n')))
         *p = '\0';

      q = line;
      
      /* trim the initial spaces */
      while (q < line + sizeof(line) && *q == ' ')
         q++;
      
      /* skip empty lines */
      if (line[0] == '\0' || *q == '\0')
         continue;
      
      /* here starts a new section [...] */
      if (*q == '[') {
         
         /* remove the square brackets */
         if ((p = strchr(line, ']')))
            *p = '\0';
         else
            FATAL_ERROR("Missing ] in %s line %d", ETTER_CONF, lineno);
         
         p = q + 1;
         
         DEBUG_MSG("load_conf: SECTION: %s", p);

         /* get the pointer to the right structure */
         if ( (curr_section = search_section(p)) == NULL)
            FATAL_ERROR("Invalid section in %s line %d", ETTER_CONF, lineno);
         
         /* read the next line */
         continue;
      }
   
      /* variable outside a section */
      if (curr_section == NULL)
         FATAL_ERROR("Entry outside a section in %s line %d", ETTER_CONF, lineno);
      
      /* sanity check */
      if (!strchr(q, '='))
         FATAL_ERROR("Parse error %s line %d", ETTER_CONF, lineno);
      
      p = q;

      /* split the entry name from the value */
      do {
         if (*p == ' ' || *p == '='){
            *p = '\0';
            break;
         }
      } while (p++ < line + sizeof(line) );
      
      /* move p to the value */
      p++;
      do {
         if (*p != ' ' && *p != '=')
            break;
      } while (p++ < line + sizeof(line) );
      
      /* 
       * if it is the "dissector" section,
       * do it in a different way
       */
      if (curr_section == (struct conf_entry *)&dissectors) {
         set_dissector(q, p, lineno);
         continue;
      }
     
      /* search the entry name */
      if ( (value = search_entry(curr_section, q)) == NULL)
         FATAL_ERROR("Invalid entry in %s line %d", ETTER_CONF, lineno);
   
      /* strings must be handled in a different way */
      if (curr_section == (struct conf_entry *)&strings) {
         /* trim the quotes */
         if (*p == '\"')
            p++;
         
         /* set the string value */ 
         tmp = (char **)value;
         *tmp = strdup(p);
         
         /* trim the ending quotes */
         p = *tmp;
         do {
            if (*p == '\"')
               *p = 0;
         } while (p++ < *tmp + strlen(*tmp) );
         
         DEBUG_MSG("load_conf: \tENTRY: %s  [%s]", q, *tmp);
      } else {
         /* set the integer value */ 
         *(int *)value = strtol(p, (char **)NULL, 10);
         DEBUG_MSG("load_conf: \tENTRY: %s  %d", q, *(int *)value);
      }
   }
  
   fclose(fc);
}

/* 
 * returns the pointer to the struct
 * named "title"
 */
static struct conf_entry * search_section(char *title)
{
   int i = 0;
  
   do {
      /* the section was found */ 
      if (!strcasecmp(sections[i].title, title))
         return sections[i].entries;
      
   } while (sections[++i].title != NULL);

   return NULL;
}

/* 
 * returns the pointer to the value
 * named "name" of the sections "section"
 */

static void * search_entry(struct conf_entry *section, char *name)
{
   int i = 0;
  
   do {
      /* the section was found */ 
      if (!strcasecmp(section[i].name, name))
         return section[i].value;
      
   } while (section[++i].name != NULL);

   return NULL;
}

/*
 * handle the special case of dissectors 
 */
static void set_dissector(char *name, char *values, int lineno)
{
   char *p, *q = values;
   u_int32 value;
   int first = 0;

   /* remove trailing spaces */
   if ((p = strchr(values, ' ')) != NULL)
      *p = '\0';
   
   /* expand multiple ports dissectors */
   for(p=strsep(&values, ","); p != NULL; p=strsep(&values, ",")) {
      /* get the value for the port */
      value = atoi(p);
      //DEBUG_MSG("load_conf: \tDISSECTOR: %s\t%d", name, value);

      /* count the dissectors and the port monitored */
      if (value) {
         number_of_ports++;
         if (first == 0) {
            number_of_dissectors++;
            first = 1;
         }
      }
    
      /* the first value replaces all the previous */
      if (p == q) {
         if (dissect_modify(MODE_REP, name, value) != ESUCCESS)
            fprintf(stderr, "Dissector \"%s\" not supported (%s line %d)\n", name, ETTER_CONF, lineno);
      } else {
         /* the other values have to be added */
         if (dissect_modify(MODE_ADD, name, value) != ESUCCESS)
            fprintf(stderr, "Dissector \"%s\" not supported (%s line %d)\n", name, ETTER_CONF, lineno);
      }
      
   }

}


/*
 * print the number of dissectors loaded 
 */
void conf_dissectors(void)
{
   USER_MSG("%4d protocol dissectors\n", number_of_dissectors);   
   USER_MSG("%4d ports monitored\n", number_of_ports);   
}

/* EOF */

// vim:ts=3:expandtab

