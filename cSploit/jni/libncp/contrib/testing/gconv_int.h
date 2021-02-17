/* Copyright (C) 1997, 1998, 1999 Free Software Foundation, Inc.
   This file is part of the GNU C Library.
   Contributed by Ulrich Drepper <drepper@cygnus.com>, 1997.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Library General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Library General Public License for more details.

   You should have received a copy of the GNU Library General Public
   License along with the GNU C Library; see the file COPYING.LIB.  If not,
   write to the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
   Boston, MA 02111-1307, USA.  */

#ifndef _GCONV_INT_H
#define _GCONV_INT_H	1

#include "gconv.h"
#include <regex.h>

__BEGIN_DECLS


/* Structure for alias definition.  Simply to strings.  */
struct gconv_alias
{
  const char *fromname;
  const char *toname;
};


/* How many character should be conveted in one call?  */
#define GCONV_NCHAR_GOAL	8160


/* Structure describing one loaded shared object.  This normally are
   objects to perform conversation but as a special case the db shared
   object is also handled.  */
struct gconv_loaded_object
{
  /* Name of the object.  */
  const char *name;

  /* Reference counter for the db functionality.  If no conversion is
     needed we unload the db library.  */
  int counter;

  /* The handle for the shared object.  */
  struct link_map *handle;

  /* Pointer to the functions the module defines.  */
  gconv_fct fct;
  gconv_init_fct init_fct;
  gconv_end_fct end_fct;
};


/* Description for an available conversion module.  */
struct gconv_module
{
  const char *from_pattern;
  const char *from_constpfx;
  size_t from_constpfx_len;
  const regex_t *from_regex;
  regex_t from_regex_mem;

  const char *to_string;

  int cost_hi;
  int cost_lo;

  const char *module_name;

  struct gconv_module *left;	/* Prefix smaller.  */
  struct gconv_module *same;	/* List of entries with identical prefix.  */
  struct gconv_module *matching;/* Next node with more specific prefix.  */
  struct gconv_module *right;	/* Prefix larger.  */
};


/* Global variables.  */

/* Database of alias names.  */
extern void *__gconv_alias_db;

/* Array with available modules.  */
extern size_t __gconv_nmodules;
extern struct gconv_module *__gconv_modules_db;


/* Return in *HANDLE decriptor for transformation from FROMSET to TOSET.  */
extern int __gconv_open (const char *__toset, const char *__fromset,
			 gconv_t *__handle)
     internal_function;

/* Free resources associated with transformation descriptor CD.  */
extern int __gconv_close (gconv_t cd)
     internal_function;

/* Transform at most *INBYTESLEFT bytes from buffer starting at *INBUF
   according to rules described by CD and place up to *OUTBYTESLEFT
   bytes in buffer starting at *OUTBUF.  Return number of written
   characters in *CONVERTED if this pointer is not null.  */
extern int __gconv (gconv_t __cd, const char **__inbuf, const char *inbufend,
		    char **__outbuf, char *outbufend, size_t *converted)
     internal_function;

/* Return in *HANDLE a pointer to an array with *NSTEPS elements describing
   the single steps necessary for transformation from FROMSET to TOSET.  */
extern int __gconv_find_transform (const char *__toset, const char *__fromset,
				   struct gconv_step **__handle,
				   size_t *__nsteps)
     internal_function;

/* Read all the configuration data and cache it.  */
extern void __gconv_read_conf (void);

/* Comparison function to search alias.  */
extern int __gconv_alias_compare (const void *__p1, const void *__p2);

/* Clear reference to transformation step implementations which might
   cause the code to be unloaded.  */
extern int __gconv_close_transform (struct gconv_step *__steps,
				    size_t __nsteps)
     internal_function;

/* Load shared object named by NAME.  If already loaded increment reference
   count.  */
extern struct gconv_loaded_object *__gconv_find_shlib (const char *__name)
     internal_function;

/* Find function named NAME in shared object referenced by HANDLE.  */
void *__gconv_find_func (void *handle, const char *name)
     internal_function;

/* Release shared object.  If no further reference is available unload
   the object.  */
extern int __gconv_release_shlib (struct gconv_loaded_object *__handle)
     internal_function;

/* Fill STEP with information about builtin module with NAME.  */
extern void __gconv_get_builtin_trans (const char *__name,
				       struct gconv_step *__step)
     internal_function;



/* Builtin transformations.  */
#ifdef _LIBC
# define __BUILTIN_TRANS(Name) \
  extern int Name (struct gconv_step *__step, struct gconv_step_data *__data, \
		   const char **__inbuf, const char *__inbufend,	      \
		   size_t *__written, int __do_flush)

__BUILTIN_TRANS (__gconv_transform_ascii_internal);
__BUILTIN_TRANS (__gconv_transform_internal_ascii);
__BUILTIN_TRANS (__gconv_transform_utf8_internal);
__BUILTIN_TRANS (__gconv_transform_internal_utf8);
__BUILTIN_TRANS (__gconv_transform_ucs2_internal);
__BUILTIN_TRANS (__gconv_transform_internal_ucs2);
__BUILTIN_TRANS (__gconv_transform_ucs2little_internal);
__BUILTIN_TRANS (__gconv_transform_internal_ucs2little);
__BUILTIN_TRANS (__gconv_transform_internal_ucs4);
# undef __BUITLIN_TRANS

#endif

__END_DECLS

#endif /* gconv_int.h */
