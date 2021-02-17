/* cSploit - a simple penetration testing suite
 * Copyright (C) 2014  Massimo Dragano aka tux_mind <tux_mind@csploit.org>
 * 
 * cSploit is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * cSploit is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with cSploit.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <unistd.h>

#include "sequence.h"
#include "control_messages.h"

/**
 * @brief get an unique sequence number from @p seq by locking @p lock.
 * @param seq the equence number to get
 * @param lock the associated mutex
 * @returns the current sequence number
 * 
 * this function returns the sequence number @p seq in a thread safe manner.
 * it also increase it by one on every call.
 */
uint16_t get_sequence(uint16_t *seq, pthread_mutex_t *lock) {
  uint16_t ret;
  
  pthread_mutex_lock(lock);
  ret=(*seq)++;
  pthread_mutex_unlock(lock);
  
  return ret;
}

/**
 * @brief get an unique id number from @p id by locking @p lock
 * @param id the id number to get
 * @param lock the associated mutex
 * @returns the current id
 * 
 * this function returns the id number @p seq in a thread safe manner.
 * it also increase it by one on every call, and check that ::CTRL_ID is not returned.
 */
uint16_t get_id(uint16_t *id, pthread_mutex_t *lock) {
  uint16_t ret;
  
  pthread_mutex_lock(lock);
  ret=(*id)++;
  if(ret==CTRL_ID)
    ret=(*id)++;
  pthread_mutex_unlock(lock);
  
  return ret;
}

/**
 * @brief set @p seq to @p value
 * @param value the value to set @p seq to
 * @param seq the sequence to work with
 * @param lock the associated mutex
 */
void set_sequence(uint16_t value, uint16_t *seq, pthread_mutex_t *lock) {
  pthread_mutex_lock(lock);
  *seq=value;
  pthread_mutex_unlock(lock);
}
