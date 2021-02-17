#ifndef EC_API_EXTERN
#error Move this header somewhere else
#endif

EC_API_EXTERN void * memmem(const void *haystack, size_t haystacklen,
                     const void *needle, size_t needlelen);

/* EOF */
