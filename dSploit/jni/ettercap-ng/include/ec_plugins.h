
/* $Id: ec_plugins.h,v 1.15 2004/07/24 10:43:21 alor Exp $ */

#ifndef EC_PLUGINS_H
#define EC_PLUGINS_H

#include <ec_stdint.h>
#include <ec_version.h>
#include <ec_ui.h>

struct plugin_ops
{
   char *ettercap_version;          /* ettercap version MUST be the global EC_VERSION */
   char *name;                      /* the name of the plugin */
   char *info;                      /* a short description of the plugin */
   char *version;                   /* the plugin version. note: 15 will be displayed as 1.5 */
   int (*init)(void *);          /* activation function */
   int (*fini)(void *);          /* deactivation function */
};

#define PLUGIN_PATTERN   "ec_*"

EC_API_EXTERN void plugin_load_all(void);
EC_API_EXTERN int plugin_load_single(char *path, char *name);
EC_API_EXTERN int plugin_register(void *handle, struct plugin_ops *ops);
EC_API_EXTERN int plugin_list_walk(int min, int max, void (*func)(char, struct plugin_ops *));
#define PLP_MIN   1
#define PLP_MAX   INT_MAX

EC_API_EXTERN int plugin_is_activated(char *name);
EC_API_EXTERN int search_plugin(char *name);

/* use these to activate and deactivate a plugin; these are *imported* from plugins */
EC_API_EXTERN int plugin_init(char *name);
EC_API_EXTERN int plugin_fini(char *name);

#define PLUGIN_FINISHED 0
#define PLUGIN_RUNNING  1

EC_API_EXTERN void plugin_list(void);

#endif 

/* EOF */

// vim:ts=3:expandtab

