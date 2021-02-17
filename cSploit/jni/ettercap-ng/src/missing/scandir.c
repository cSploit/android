
#include<dirent.h>
#include<stdlib.h>
#include<string.h>
#include<sys/types.h>

int scandir(const char *, struct dirent ***, int(*)(const struct dirent *), int(*)(const struct dirent **, const struct dirent **));
int alphasort(const struct dirent **a, const struct dirent **b);

/* This function is only required for SunOS, all other supported OS
have this function in their system library */

int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **))
{
  DIR *d;
  struct dirent *entry;
  register int i=0;
  size_t entrysize;

  if ((d=opendir(dir)) == NULL)
     return(-1);

  *namelist=NULL;

  while ((entry=readdir(d)) != NULL)
  {
    if (select == NULL || (select != NULL && (*select)(entry)))
    {
      *namelist=(struct dirent **)realloc((void *)(*namelist), (size_t)((i+1)*sizeof(struct dirent *)));
      if (*namelist == NULL) return(-1);

      entrysize=sizeof(struct dirent)-sizeof(entry->d_name)+strlen(entry->d_name)+1;
      (*namelist)[i]=(struct dirent *)malloc(entrysize);
      if ((*namelist)[i] == NULL) 
         return(-1);

      memcpy((*namelist)[i], entry, entrysize);
      i++;
    }
  }
  if (closedir(d)) return(-1);
  if (i == 0) return(-1);
  if (compar != NULL)
    qsort((void *)(*namelist), (size_t)i, sizeof(struct dirent *), (int (*)(const void *, const void *))compar);

  return(i);
}

int alphasort(const struct dirent **a, const struct dirent **b)
{
  return(strcmp((*a)->d_name, (*b)->d_name));
}
