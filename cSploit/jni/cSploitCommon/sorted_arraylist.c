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

#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <errno.h>
#include <stdint.h>

#include "logger.h"
#include "sorted_arraylist.h"

/**
 * @brief retrieve the index of a ::comparable_item by it's @p id
 * @param a the ::array
 * @param id the ::comparable_item.id to search for
 * @returns the ::comparable_item index on success, -1 if not found.
 */
static int sortedarray_search_index(array *a, uint32_t id) {
  comparable_item *cur;
  uint32_t imin, imax, imid;
  
  assert(a!=NULL);
  assert(a->size > 0);
  
  imin = 0;
  imax = a->size - 1;
  
  while(imin < imax) {
  
    imid = (imax - imin)/2 + imin; // avoid overflow
    
    cur = (comparable_item *) a->data[imid];
    
    if(cur->id < id) {
      imin = imid + 1;
    } else {
      imax = imid;
    }
  }
  
  if(imax != imin)
    return -1;
  
  cur = (comparable_item *) a->data[imax];
  
  if (cur->id == id)
    return imax;
  
  return -1;
}

/**
 * @brief insert @p i in @p a
 * @param a the ::array
 * @param i the ::comparable_item
 * @returns 0 on success, -1 on error.
 */
int sortedarray_ins(array *a, comparable_item *i) {
  register comparable_item **itmp;
  
  itmp = realloc(a->data, (a->size +1) * sizeof(comparable_item *));
  
  if(!itmp) {
    print( ERROR, "realloc: %s", strerror(errno));
    return -1;
  }
  
  a->data = (void **) itmp;
  itmp += a->size;
  a->size++;
  
  while(((void **) itmp) > a->data &&  (**(itmp-1)).id > i->id) {
    *itmp = *(itmp-1);
    itmp--;
  }
  
  *itmp = i;
  
  return 0;
}

/**
 * @brief remove @p i from @p a
 * @param a the ::array
 * @param i the ::comparable_item
 * @returns 0 on success, -1 on error.
 */
int sortedarray_del(array *a, comparable_item *i) {
  void **atmp, **end, *last;
  uint32_t index;
  
  assert(a!=NULL);
  assert(i!=NULL);
  
  if(!(a->size)) {
    return 0;
  }
  
  index = sortedarray_search_index(a, i->id);
  
  if(index == -1) {
    return 0;
  }
  
  a->size--;
  
  if(!(a->size)) {
    free(a->data);
    a->data = NULL;
    return 0;
  }
  
  last = a->data[a->size];
  
  atmp = realloc(a->data, a->size * sizeof(comparable_item *));
  
  if(!atmp) {
    print( ERROR, "realloc: %s", strerror(errno));
    a->size++;
    return -1;
  }
  
  a->data = atmp;
  
  if(index == a->size) return 0;
  
  end = a->data + a->size - 1;
  
  for(atmp += index; atmp < end; atmp++) {
    *atmp = *(atmp+1);
  }
  
  *atmp = last;
  
  return 0;
}

/**
 * @brief get a ::comparable_item with ::comparable_item.id == @p id from @p a
 * @param a the ::array
 * @param i the ::comparable_item
 */
comparable_item *sortedarray_get(array *a, uint32_t id) {
  int index;
  
  index = sortedarray_search_index(a, id);
  
  if(index == -1)
    return NULL;
  
  return (comparable_item *) a->data[index];
}
