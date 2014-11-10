/* LICENSE
 * 
 */
#ifndef STR_ARRAY_H
#define STR_ARRAY_H

struct message;

int   string_array_add(struct message *, size_t , char *);
char *string_array_next(struct message *, char *, char *);
int   string_array_size(struct message *, char *);

#endif