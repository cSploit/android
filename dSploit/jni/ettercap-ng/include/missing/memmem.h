#ifndef EC_API_EXTERN
#error Move this header somewhere else
#endif

EC_API_EXTERN void * memmem(const char *haystack, size_t haystacklen,
                     const char *needle, size_t needlelen);

/* EOF */
