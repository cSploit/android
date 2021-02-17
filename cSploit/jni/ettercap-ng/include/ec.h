

#ifndef EC_H
#define EC_H

#include <config.h>

#include <sys/types.h>
#include <sys/time.h>

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

#ifdef OS_WINDOWS
   #include <windows.h>
#endif

#if !defined (__USE_GNU)   /* for memmem(), strsignal(), etc etc... */
   #define __USE_GNU
#endif
#ifdef OS_SOLARIS
   #define _REENTRANT      /* for strtok_r() */
#endif
#include <string.h>
#if defined (__USE_GNU)
   #undef __USE_GNU
#endif
#include <strings.h>
#include <unistd.h>
#include <time.h>

/*
 * On Windows (MinGw) we must export all ettercap.exe variables/function
 * used in plugins and functions in plugins must be declared as 'importable'
 */
#if defined(OS_WINDOWS)
   #if defined(BUILDING_PLUGIN)
      #define EC_API_EXTERN __declspec(dllimport)
   #else
      #define EC_API_EXTERN __declspec(dllexport)
   #endif
#else
   #define EC_API_EXTERN extern
#endif

/* these are often needed... */
#include <ec_queue.h>
#include <ec_error.h>
#include <ec_debug.h>
#include <ec_stdint.h>
#include <ec_globals.h>
#include <ec_strings.h>

#ifdef OS_MINGW
   #include <ec_os_mingw.h>
#endif


/* wrappers for safe memory allocation */

#define SAFE_CALLOC(x, n, s) do { \
   x = calloc(n, s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
} while(0)

#define SAFE_MALLOC(x, s) do { \
   x = malloc(s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
} while (0)

#define SAFE_REALLOC(x, s) do { \
   x = realloc(x, s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
} while(0)

#define SAFE_STRDUP(x, s) do{ \
   x = strdup(s); \
   ON_ERROR(x, NULL, "virtual memory exhausted"); \
}while(0)

#define SAFE_FREE(x) do{ if(x) { free(x); x = NULL; } }while(0)


/* convert to string */
#define EC_STRINGIFY(in) #in
#define EC_TOSTRING(in) EC_STRINGIFY(in)

#ifdef OS_LINUX
#define __init       __attribute__((constructor(101)))
#define __init_last  __attribute__((constructor(200))
#else
#define __init __attribute__((constructor))
#define __init_last __init
#endif

#ifndef __set_errno
#define __set_errno(e) (errno = (e))
#endif

#define LOOP for(;;)

#define EXECUTE(x, ...) do{ if(x != NULL) x( __VA_ARGS__ ); }while(0)

/* couple of useful macros */
#ifndef offsetof
#define offsetof(type, member) ((size_t) &((type*)0)->member)
#endif
#ifndef containerof
#define containerof(ptr, type, member) ((type*)((char*)ptr - offsetof(type,member)))
#endif

/* min and max */

#ifndef MIN
   #define MIN(a, b)    (((a) < (b)) ? (a) : (b))
#endif
#ifndef MAX
   #define MAX(a, b)    (((a) > (b)) ? (a) : (b))
#endif

/* file operations */ 
#ifndef OS_WINDOWS
   #define O_BINARY  0
#endif

/* bit operations */

#define BIT_SET(r,b)       ( r[b>>3] |=   1<<(b&7) )
#define BIT_RESET(r,b)     ( r[b>>3] &= ~ 1<<(b&7) )
#define BIT_TEST(r,b)      ( r[b>>3]  &   1<<(b&7) )
#define BIT_NOT(r,b)       ( r[b>>3] ^=   1<<(b&7) )

/* Save and restore relative offsets for pointers into a buffer */
#define SAVE_OFFSET(o,b)     o=(u_int8 *)((int)o-(int)b)
#define RESTORE_OFFSET(o,b)  o=(u_int8 *)((int)o+(int)b)   

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

/* magic numbers */

#ifndef RANDMAGIC
   #define EC_MAGIC_8   0xec
   #define EC_MAGIC_16  0xe77e
   #define EC_MAGIC_32  0xe77ee77e
#else
   #define EC_MAGIC_8   ((RANDMAGIC & 0x0ff0) >> 4)
   #define EC_MAGIC_16  (RANDMAGIC & 0xffff)
   #define EC_MAGIC_32  (((RANDMAGIC & 0xff) << 8)|((RANDMAGIC & 0xff00) >> 8)|(RANDMAGIC & 0xffff0000))
#endif

/* exported by ec_main */
EC_API_EXTERN void clean_exit(int errcode);


#endif   /*  EC_H */

/* EOF */

// vim:ts=3:expandtab

