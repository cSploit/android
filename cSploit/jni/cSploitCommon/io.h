/* LICENSE
 * 
 */


#ifndef IO_H
#define IO_H

char *read_chunk(int , unsigned int );
int read_wrapper(int , void *, unsigned int);
int write_wrapper(int , void *, unsigned int);

#endif