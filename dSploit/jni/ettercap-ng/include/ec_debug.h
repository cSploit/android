
/* $Id: ec_debug.h,v 1.9 2004/07/24 10:43:21 alor Exp $ */

#if defined (DEBUG) && !defined(EC_DEBUG_H)
#define EC_DEBUG_H

EC_API_EXTERN void debug_init(void);
EC_API_EXTERN void debug_msg(const char *message, ...);

EC_API_EXTERN FILE *debug_file;

#define DEBUG_INIT() debug_init()
#define DEBUG_MSG(x, ...) do {                                 \
   if (debug_file == NULL) {                                   \
      fprintf(stderr, "DEBUG: "x"\n", ## __VA_ARGS__ );            \
   } else                                                      \
      debug_msg(x, ## __VA_ARGS__ );                           \
} while(0)

#endif /* EC_DEBUG_H */

/* 
 * if DEBUG is not defined we expand the macros to null instructions...
 */

#ifndef DEBUG
   #define DEBUG_INIT()
   #define DEBUG_MSG(x, ...)
#endif

/* EOF */

// vim:ts=3:expandtab

