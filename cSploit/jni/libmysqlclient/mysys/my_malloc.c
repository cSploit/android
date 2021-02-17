/* Copyright (c) 2000, 2013, Oracle and/or its affiliates. All rights reserved.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; version 2 of the License.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301  USA */

#include "mysys_priv.h"
#include "mysys_err.h"
#include <m_string.h>

#ifdef HAVE_PSI_MEMORY_INTERFACE
#define USE_MALLOC_WRAPPER
#endif

#ifdef USE_MALLOC_WRAPPER
struct my_memory_header
{
  PSI_memory_key m_key;
  uint m_magic;
  size_t m_size;
};
typedef struct my_memory_header my_memory_header;
#define HEADER_SIZE 16

#define MAGIC 1234

#define USER_TO_HEADER(P) \
  ( (my_memory_header*) (((char *) P) - HEADER_SIZE ))
#define HEADER_TO_USER(P) \
  ( ((char*) P) + HEADER_SIZE )

void * my_malloc(PSI_memory_key key, size_t size, myf flags)
{
  my_memory_header *mh;
  size_t raw_size;
  compile_time_assert(sizeof(my_memory_header) <= HEADER_SIZE);

  raw_size= HEADER_SIZE + size;
  mh= (my_memory_header*) my_raw_malloc(raw_size, flags);
  if (likely(mh != NULL))
  {
    void *user_ptr;
    mh->m_magic= MAGIC;
    mh->m_size= size;
    mh->m_key= PSI_MEMORY_CALL(memory_alloc)(key, size);
    user_ptr= HEADER_TO_USER(mh);
    MEM_MALLOCLIKE_BLOCK(user_ptr, size, 0, (flags & MY_ZEROFILL));
    return user_ptr;
  }
  return NULL;
}

void *
my_realloc(PSI_memory_key key, void *ptr, size_t size, myf flags)
{
  my_memory_header *old_mh;
  size_t old_size;
  size_t min_size;
  void *new_ptr;

  if (ptr == NULL)
    return my_malloc(key, size, flags);

  old_mh= USER_TO_HEADER(ptr);
  DBUG_ASSERT((old_mh->m_key == key) || (old_mh->m_key == PSI_NOT_INSTRUMENTED));
  DBUG_ASSERT(old_mh->m_magic == MAGIC);

  old_size= old_mh->m_size;

  if (old_size == size)
    return ptr;

  new_ptr= my_malloc(key, size, flags);
  if (likely(new_ptr != NULL))
  {
#ifndef DBUG_OFF
    my_memory_header *new_mh= USER_TO_HEADER(new_ptr);
#endif

    DBUG_ASSERT((new_mh->m_key == key) || (new_mh->m_key == PSI_NOT_INSTRUMENTED));
    DBUG_ASSERT(new_mh->m_magic == MAGIC);
    DBUG_ASSERT(new_mh->m_size == size);

    min_size= (old_size < size) ? old_size : size;
    memcpy(new_ptr, ptr, min_size);
    my_free(ptr);

    return new_ptr;
  }
  return NULL;
}

void my_free(void *ptr)
{
  my_memory_header *mh;

  if (ptr == NULL)
    return;

  mh= USER_TO_HEADER(ptr);
  DBUG_ASSERT(mh->m_magic == MAGIC);
  PSI_MEMORY_CALL(memory_free)(mh->m_key, mh->m_size);
  /* Catch double free */
  mh->m_magic= 0xDEAD;
  MEM_FREELIKE_BLOCK(ptr, 0);
  my_raw_free(mh);
}
#endif

#ifndef USE_MALLOC_WRAPPER
void *my_malloc(PSI_memory_key key __attribute__((unused)),
                size_t size, myf my_flags)
{
  return my_raw_malloc(size, my_flags);
}

void *my_realloc(PSI_memory_key key __attribute__((unused)),
                 void *ptr, size_t size, myf flags)
{
  return my_raw_realloc(ptr, size, flags);
}

void my_free(void *ptr)
{
  my_raw_free(ptr);
}
#endif


/**
  Allocate a sized block of memory.

  @param size   The size of the memory block in bytes.
  @param flags  Failure action modifiers (bitmasks).

  @return A pointer to the allocated memory block, or NULL on failure.
*/
void *my_raw_malloc(size_t size, myf my_flags)
{
  void* point;
  DBUG_ENTER("my_raw_malloc");
  DBUG_PRINT("my",("size: %lu  my_flags: %d", (ulong) size, my_flags));

  /* Safety */
  if (!size)
    size=1;

  point= malloc(size);
  DBUG_EXECUTE_IF("simulate_out_of_memory",
                  {
                    free(point);
                    point= NULL;
                  });
  DBUG_EXECUTE_IF("simulate_persistent_out_of_memory",
                  {
                    free(point);
                    point= NULL;
                  });

  if (point == NULL)
  {
    my_errno=errno;
    if (my_flags & MY_FAE)
      error_handler_hook=fatal_error_handler_hook;
    if (my_flags & (MY_FAE+MY_WME))
      my_error(EE_OUTOFMEMORY, MYF(ME_BELL + ME_WAITTANG +
                                   ME_NOREFRESH + ME_FATALERROR),size);
    DBUG_EXECUTE_IF("simulate_out_of_memory",
                    DBUG_SET("-d,simulate_out_of_memory"););
    if (my_flags & MY_FAE)
      exit(1);
  }
  else if (my_flags & MY_ZEROFILL)
    memset(point, 0, size);
  DBUG_PRINT("exit",("ptr: %p", point));
  DBUG_RETURN(point);
}


/**
   @brief wrapper around realloc()

   @param  oldpoint        pointer to currently allocated area
   @param  size            new size requested, must be >0
   @param  my_flags        flags

   @note if size==0 realloc() may return NULL; my_realloc() treats this as an
   error which is not the intention of realloc()
*/
void *my_raw_realloc(void *oldpoint, size_t size, myf my_flags)
{
  void *point;
  DBUG_ENTER("my_realloc");
  DBUG_PRINT("my",("ptr: %p  size: %lu  my_flags: %d", oldpoint,
                   (ulong) size, my_flags));

  DBUG_ASSERT(size > 0);
  /* These flags are mutually exclusive. */
  DBUG_ASSERT(!((my_flags & MY_FREE_ON_ERROR) &&
                (my_flags & MY_HOLD_ON_ERROR)));
  DBUG_EXECUTE_IF("simulate_out_of_memory",
                  point= NULL;
                  goto end;);
  if (!oldpoint && (my_flags & MY_ALLOW_ZERO_PTR))
    DBUG_RETURN(my_raw_malloc(size, my_flags));
  point= realloc(oldpoint, size);
#ifndef DBUG_OFF
end:
#endif
  if (point == NULL)
  {
    if (my_flags & MY_HOLD_ON_ERROR)
      DBUG_RETURN(oldpoint);
    if (my_flags & MY_FREE_ON_ERROR)
      my_free(oldpoint);
    my_errno=errno;
    if (my_flags & (MY_FAE+MY_WME))
      my_error(EE_OUTOFMEMORY, MYF(ME_BELL+ ME_WAITTANG + ME_FATALERROR),
               size);
    DBUG_EXECUTE_IF("simulate_out_of_memory",
                    DBUG_SET("-d,simulate_out_of_memory"););
  }
  DBUG_PRINT("exit",("ptr: %p", point));
  DBUG_RETURN(point);
}

/**
  Free memory allocated with my_raw_malloc.

  @remark Relies on free being able to handle a NULL argument.

  @param ptr Pointer to the memory allocated by my_raw_malloc.
*/
void my_raw_free(void *ptr)
{
  DBUG_ENTER("my_free");
  DBUG_PRINT("my",("ptr: %p", ptr));
  free(ptr);
  DBUG_VOID_RETURN;
}


void *my_memdup(PSI_memory_key key, const void *from, size_t length, myf my_flags)
{
  void *ptr;
  if ((ptr= my_malloc(key, length, my_flags)) != 0)
    memcpy(ptr, from, length);
  return ptr;
}


char *my_strdup(PSI_memory_key key, const char *from, myf my_flags)
{
  char *ptr;
  size_t length= strlen(from)+1;
  if ((ptr= (char*) my_malloc(key, length, my_flags)))
    memcpy(ptr, from, length);
  return ptr;
}


char *my_strndup(PSI_memory_key key, const char *from, size_t length, myf my_flags)
{
  char *ptr;
  if ((ptr= (char*) my_malloc(key, length+1, my_flags)))
  {
    memcpy(ptr, from, length);
    ptr[length]= 0;
  }
  return ptr;
}

