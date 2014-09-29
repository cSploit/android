/* LICESE
 * 
 */
#ifndef SEQUENCE_H
#define SEQUENCE_H

#include <pthread.h>
#include <stdint.h>

uint16_t get_sequence(uint16_t *, pthread_mutex_t *);
uint16_t get_id(uint16_t *, pthread_mutex_t *);
void set_sequence(uint16_t , uint16_t *, pthread_mutex_t *);
#endif