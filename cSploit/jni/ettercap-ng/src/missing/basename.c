
#include <ec.h>

char * basename (const char *filename);

char * basename (const char *filename)
{
  register char *p;
  
  p = strrchr (filename, '/');
  
  return p ? p + 1 : (char *) filename;
}
