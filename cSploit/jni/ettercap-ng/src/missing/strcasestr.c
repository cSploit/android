
/*
 * silly implementation for the strcasestr funcion.
 * 
 */

#include <ec.h>

char *strcasestr(const char *hailstack, const char *needle);
   
char *strcasestr(const char *hailstack, const char *needle)
{
   register int lneed = strlen(needle);
   register int lhail = strlen(hailstack);
   register int i;

   for (i = 0; i < lhail; i++) {
      if (!strncasecmp(hailstack + i, needle, lneed))
         return hailstack + i;
   }

   return NULL;
}

/* EOF */

