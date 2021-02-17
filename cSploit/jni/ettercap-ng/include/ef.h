

#ifndef EF_H
#define EF_H

#include <config.h>

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#if !defined (__USE_GNU)  /* for memmem(), strsignal(), etc etc... */
   #define __USE_GNU
#endif
#include <string.h>
#if defined (__USE_GNU)
   #undef __USE_GNU
#endif
#include <strings.h>
#include <unistd.h>
#include <time.h>

#define EC_API_EXTERN
#define EF_API_EXTERN

#ifndef HAVE_STRSEP
   #include <missing/strsep.h>
#endif

#ifdef OS_WINDOWS
   #include <windows.h>
#endif

#include <ec_queue.h>
#include <ec_stdint.h>
#include <ec_error.h>
#include <ec_strings.h>

#define SAFE_CALLOC(x, n, s) do { \
   x = calloc(n, s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
} while(0)

#define SAFE_REALLOC(x, s) do { \
   x = realloc(x, s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
} while(0)

#define SAFE_FREE(x) do{ if(x) { free(x); x = NULL; } }while(0)

#define __init __attribute__ ((constructor))

#define LOOP for(;;)

/* file operations */ 
#ifndef OS_WINDOWS
   #define O_BINARY  0
#endif

struct globals {
   char *source_file;
   char *output_file;
   u_int32 lineno;
   u_int8 debug;
   u_int8 suppress_warnings;
};

/* in el_main.c */
extern struct globals gbls;

#define GBL_OPTIONS  gbls
#define GBL          gbls

#define GBL_PROGRAM "etterfilter"


#define BIT_SET(r,b)       ( r[b>>3] |=   1<<(b&7) )
#define BIT_RESET(r,b)     ( r[b>>3] &= ~ 1<<(b&7) )
#define BIT_TEST(r,b)      ( r[b>>3]  &   1<<(b&7) )
#define BIT_NOT(r,b)       ( r[b>>3] ^=   1<<(b&7) )


/* ANSI colors */
#ifndef OS_WINDOWS
   #define EC_COLOR_END    "\033[0m"
   #define EC_COLOR_BOLD   "\033[1m"

   #define EC_COLOR_RED    "\033[31m"EC_COLOR_BOLD
   #define EC_COLOR_YELLOW "\033[33m"EC_COLOR_BOLD
   #define EC_COLOR_GREEN  "\033[32m"EC_COLOR_BOLD
   #define EC_COLOR_BLUE   "\033[34m"EC_COLOR_BOLD
   #define EC_COLOR_CYAN   "\033[36m"EC_COLOR_BOLD
#else
   /* Windows console doesn't grok ANSI */
   #define EC_COLOR_END
   #define EC_COLOR_BOLD
   
   #define EC_COLOR_RED
   #define EC_COLOR_YELLOW
   #define EC_COLOR_GREEN
   #define EC_COLOR_BLUE
   #define EC_COLOR_CYAN
#endif

#endif   /*  EL_H */

/* EOF */

// vim:ts=3:expandtab

