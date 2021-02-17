

#ifndef EC_CONF_H
#define EC_CONF_H


struct conf_entry {
   char *name;
   void *value;
};

struct conf_section {
   char *title;
   struct conf_entry *entries;
};


/* exported functions */

EC_API_EXTERN void load_conf(void);
EC_API_EXTERN void conf_dissectors(void);

#endif

/* EOF */

// vim:ts=3:expandtab

