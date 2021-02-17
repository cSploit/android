

#ifndef EC_STRINGS_H
#define EC_STRINGS_H

#ifdef HAVE_CTYPE_H
   #include <ctype.h>
#else
   extern int isprint(int c);
#endif

#ifndef HAVE_STRLCAT
   #include <missing/strlcat.h>
#endif
#ifndef HAVE_STRLCPY 
   #include <missing/strlcpy.h>
#endif
#ifndef HAVE_STRSEP 
   #include <missing/strsep.h>
#endif
#ifndef HAVE_STRCASESTR 
   #include <missing/strcasestr.h>
#endif
#ifndef HAVE_MEMMEM
   #include <missing/memmem.h>
#endif
#ifndef HAVE_BASENAME
   #include <missing/basename.h>
#endif

EC_API_EXTERN int match_pattern(const char *s, const char *pattern);
EC_API_EXTERN int base64_decode(char *bufplain, const char *bufcoded);
EC_API_EXTERN int strescape(char *dst, char *src);
EC_API_EXTERN int str_replace(char **text, const char *s, const char *d);   
EC_API_EXTERN size_t strlen_utf8(const char *s);
EC_API_EXTERN char * ec_strtok(char *s, const char *delim, char **ptrptr);
EC_API_EXTERN char getchar_buffer(char **buf);
EC_API_EXTERN int str_hex_to_bytes(char *string, u_char *bytes);
EC_API_EXTERN char * str_tohex(u_char *bin, size_t len, char *dst, size_t dst_len);

#define strtok(x,y) DON_T_USE_STRTOK_DIRECTLY_USE__EC_STRTOK__INSTEAD

#endif

/* EOF */

// vim:ts=3:expandtab

