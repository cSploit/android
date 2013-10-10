
/* $Id: ec_conf.h,v 1.5 2004/07/24 10:43:21 alor Exp $ */

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

