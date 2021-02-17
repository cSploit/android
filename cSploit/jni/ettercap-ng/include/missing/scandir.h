
#include <dirent.h>

extern int scandir(const char *dir, struct dirent ***namelist,
            int (*select)(const struct dirent *),
            int (*compar)(const struct dirent **, const struct dirent **));

extern int alphasort(const struct dirent **a, const struct dirent **b);

/* EOF */

