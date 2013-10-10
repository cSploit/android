/*
    ettercap -- plugin handling

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

    $Id: ec_plugins.c,v 1.39 2004/07/12 19:57:26 alor Exp $
*/

#include <ec.h>
#include <ec_plugins.h>

#include <dirent.h>

#ifndef HAVE_SCANDIR
   #include <missing/scandir.h>
#endif

#ifdef HAVE_PLUGINS
   #ifdef HAVE_LTDL_H
      #include <ltdl.h>
   #endif
   #ifdef HAVE_DLFCN_H
      #include <dlfcn.h>
   #endif
#endif

#ifndef LTDL_SHLIB_EXT
   #define LTDL_SHLIB_EXT ""
#endif

/* symbol prefix for some OSes */
#ifdef NEED_USCORE
   #define SYM_PREFIX "_"
#else
   #define SYM_PREFIX ""
#endif
         

/* global data */

struct plugin_entry {
   void *handle;
   char activated;   /* if the init fuction was already called */
   struct plugin_ops *ops;
   SLIST_ENTRY(plugin_entry) next;
};

static SLIST_HEAD(, plugin_entry) plugin_head;

/* protos... */

void plugin_load_all(void);
void plugin_unload_all(void);
int plugin_load_single(char *path, char *name);
int plugin_register(void *handle, struct plugin_ops *ops);
int plugin_init(char *name);
int plugin_fini(char *name);
int plugin_list_walk(int min, int max, void (*func)(char, struct plugin_ops *));
int plugin_is_activated(char *name);
int search_plugin(char *name);
void plugin_list(void);
static void plugin_print(char active, struct plugin_ops *ops);
#if defined(OS_BSD) || defined(OS_DARWIN) || defined(__APPLE__)
int plugin_filter(struct dirent *d);
#else
int plugin_filter(const struct dirent *d);
#endif

/*******************************************/

/* 
 * load a plugin given the full path
 */

int plugin_load_single(char *path, char *name)
{
#ifdef HAVE_PLUGINS
   char file[strlen(path)+strlen(name)+2];
   void *handle;
   int (*plugin_load)(void *);
   
   snprintf(file, sizeof(file), "%s/%s", path, name);
  
   DEBUG_MSG("plugin_load_single: %s", file);
   
   /* load the plugin */
   handle = lt_dlopen(file);

   if (handle == NULL) {
      DEBUG_MSG("plugin_load_single - %s - lt_dlopen() | %s", file, lt_dlerror());
      return -EINVALID;
   }
   
   /* find the loading function */
   plugin_load = lt_dlsym(handle, SYM_PREFIX "plugin_load");
   
   if (plugin_load == NULL) {
      DEBUG_MSG("plugin_load_single - %s - lt_dlsym() | %s", file, lt_dlerror());
      lt_dlclose(handle);
      return -EINVALID;
   }

   /* 
    * return the same value of plugin_register 
    * we pass the handle to the plugin, which
    * in turn passes it to the plugin_register 
    * function
    */
   return plugin_load(handle);
#else
   return -EINVALID;
#endif
}

/*
 * filter for the scandir function
 */
#if defined(OS_BSD) || defined(OS_DARWIN) || defined(__APPLE__)
   int plugin_filter(struct dirent *d)
#else
   int plugin_filter(const struct dirent *d)
#endif
{
   if ( match_pattern(d->d_name, PLUGIN_PATTERN LTDL_SHLIB_EXT) )
      return 1;

   return 0;
}

/*
 * search and load all plugins in INSTALL_PREFIX/lib
 */

void plugin_load_all(void)
{
#ifdef HAVE_PLUGINS
   struct dirent **namelist = NULL;
   int n, i, ret;
   int t = 0;
   char *where;
   
   DEBUG_MSG("plugin_loadall");

   if (lt_dlinit() != 0)
      ERROR_MSG("lt_dlinit()");
#ifdef OS_WINDOWS
   /* Assume .DLLs are under "<ec_root>/lib". This should be unified for
    * all; ec_get_lib_path()?
    */
   char path[MAX_PATH];

   snprintf(path, sizeof(path), "%s%s", ec_win_get_ec_dir(), INSTALL_LIBDIR);
   where = path;
#else
   /* default path */
   where = INSTALL_LIBDIR "/" EC_PROGRAM;
#endif
   
   /* first search in  INSTALL_LIBDIR/ettercap" */
   n = scandir(where, &namelist, plugin_filter, alphasort);
   /* on error fall back to the current dir */
   if (n <= 0) {
      DEBUG_MSG("plugin_loadall: no plugin in %s searching locally...", where);
      /* change the path to the local one */
      where = ".";
      n = scandir(where, &namelist, plugin_filter, alphasort);
      DEBUG_MSG("plugin_loadall: %d found", n);
   }
  
   for(i = n-1; i >= 0; i--) {
      ret = plugin_load_single(where, namelist[i]->d_name);
      switch (ret) {
         case ESUCCESS:
            t++;
            break;
         case -EDUPLICATE:
            USER_MSG("plugin %s already loaded...\n", namelist[i]->d_name);
            DEBUG_MSG("plugin %s already loaded...", namelist[i]->d_name);
            break;
         case -EVERSION:
            USER_MSG("plugin %s was compiled for a different ettercap version...\n", namelist[i]->d_name);
            DEBUG_MSG("plugin %s was compiled for a different ettercap version...", namelist[i]->d_name);
            break;
         case -EINVALID:
         default:
            USER_MSG("plugin %s cannot be loaded...\n", namelist[i]->d_name);
            DEBUG_MSG("plugin %s cannot be loaded...", namelist[i]->d_name);
            break;
      }
      SAFE_FREE(namelist[i]);
   }
   
   USER_MSG("%4d plugins\n", t);

   SAFE_FREE(namelist);
   
   atexit(&plugin_unload_all);
#else
   USER_MSG("   0 plugins (disabled by configure...)\n");
#endif
}


/*
 * unload all the plugin
 */
void plugin_unload_all(void)
{
#ifdef HAVE_PLUGINS
   struct plugin_entry *p;
   
   DEBUG_MSG("ATEXIT: plugin_unload_all");   
   
   while (SLIST_FIRST(&plugin_head) != NULL) {
      p = SLIST_FIRST(&plugin_head);
      lt_dlclose(p->handle);
      SLIST_REMOVE_HEAD(&plugin_head, next);
      SAFE_FREE(p);
   }
   
   lt_dlexit();
#endif
}


/*
 * function used by plugins to register themself
 */
int plugin_register(void *handle, struct plugin_ops *ops)
{
#ifdef HAVE_PLUGINS
   struct plugin_entry *p, *pl;

   /* check for ettercap API version */
   if (strcmp(ops->ettercap_version, EC_VERSION)) {
      lt_dlclose(handle);
      return -EVERSION;
   }
   
   /* check that this plugin was not already loaded */
   SLIST_FOREACH(pl, &plugin_head, next) {
      /* same name and same version */
      if (!strcmp(ops->name, pl->ops->name) && !strcmp(ops->version, pl->ops->version)) {
         lt_dlclose(handle);
         return -EDUPLICATE;
      }
   }

   SAFE_CALLOC(p, 1, sizeof(struct plugin_entry));
   
   p->handle = handle;
   p->ops = ops;

   SLIST_INSERT_HEAD(&plugin_head, p, next);

   return ESUCCESS;
#else
   return -EINVALID;
#endif
}


/* 
 * activate a plugin.
 * it launch the plugin init function 
 */
int plugin_init(char *name)
{
   struct plugin_entry *p;
   int ret;

   SLIST_FOREACH(p, &plugin_head, next) {
      if (!strcmp(p->ops->name, name)) {
         /* get the response from the plugin */
         ret = p->ops->init(NULL);
         /* if it is still running, mark it as active */
         if (ret == PLUGIN_RUNNING)
            p->activated = 1;
         return ret;
      }
   }
   
   return -ENOTFOUND;
}


/* 
 * deactivate a plugin.
 * it launch the plugin fini function 
 */
int plugin_fini(char *name)
{
   struct plugin_entry *p;
   int ret;

   SLIST_FOREACH(p, &plugin_head, next) {
      if (p->activated == 1 && !strcmp(p->ops->name, name)) {
         /* get the response from the plugin */
         ret = p->ops->fini(NULL);
         /* if it is still running, mark it as active */
         if (ret == PLUGIN_FINISHED)
            p->activated = 0;
         return ret;
      }
   }
   
   return -ENOTFOUND;
}

/*
 * it print the list of the plugins.
 *
 * func is the callback function to which are passed
 *    - the plugin name
 *    - the plugin version
 *    - the plugin description
 *
 * min is the n-th plugin to start to print
 * max it the n-th plugin to stop to print
 */

int plugin_list_walk(int min, int max, void (*func)(char, struct plugin_ops *))
{
   struct plugin_entry *p;
   int i = min;

   SLIST_FOREACH(p, &plugin_head, next) {
      if (i > max)
         return (i-1);
      if (i < min) {
         i++;
         continue;
      }
      func(p->activated, p->ops);
      i++;
   }
   
   return (i == min) ? -ENOTFOUND : (i-1);
}

/*
 * returns the activation flag
 */

int plugin_is_activated(char *name)
{
   struct plugin_entry *p;

   SLIST_FOREACH(p, &plugin_head, next) {
      if (!strcmp(p->ops->name, name)) {
         return p->activated;
      }
   }
   
   return 0;
}

/*
 * search if a plugin exists
 */
int search_plugin(char *name)
{
   struct plugin_entry *p;

   SLIST_FOREACH(p, &plugin_head, next) {
      if (!strcmp(p->ops->name, name)) {
         return ESUCCESS;
      }
   }
   
   return -ENOTFOUND;
}

/*
 * print the list of available plugins
 */
void plugin_list(void)
{
   int ret;

   DEBUG_MSG("plugin_list");

   /* load all the plugins */
   plugin_load_all();
      
   /* print the list */
   fprintf(stdout, "\nAvailable plugins :\n\n");
   ret = plugin_list_walk(PLP_MIN, PLP_MAX, &plugin_print);
   if (ret == -ENOTFOUND) {
      fprintf(stdout, "No plugin found !\n\n");
      return;
   }
   fprintf(stdout, "\n\n");

}

/*
 * callback function for displaying the plugin list 
 */
static void plugin_print(char active, struct plugin_ops *ops)
{
   fprintf(stdout, " %15s %4s  %s\n", ops->name, ops->version, ops->info);  
}

/* EOF */

// vim:ts=3:expandtab

