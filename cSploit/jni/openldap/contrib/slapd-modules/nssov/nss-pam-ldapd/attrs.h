/*
   attrs.h - wrapper macros for the gcc __attribute__(()) directive

   Copyright (C) 2007, 2008 Arthur de Jong

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
   02110-1301 USA
*/

#ifndef COMPAT__ATTRS_H
#define COMPAT__ATTRS_H 1

/* macro for testing the version of GCC */
#define GCC_VERSION(major,minor) \
  ((__GNUC__ > (major)) || (__GNUC__ == (major) && __GNUC_MINOR__ >= (minor)))

/* These are macros to use some gcc-specific flags in case the're available
   and otherwise define them to empty strings. This allows us to give
   the compiler some extra information.
   See http://gcc.gnu.org/onlinedocs/gcc/Attribute-Syntax.html
   for a list of attributes supported by gcc */

/* this is used to flag function parameters that are not used in the function
   body. */
#if GCC_VERSION(3,0)
#define UNUSED(x)   x __attribute__((__unused__))
#else
#define UNUSED(x)   x
#endif

/* this is used to add extra format checking to the function calls as if this
   was a printf()-like function */
#if GCC_VERSION(3,0)
#define LIKE_PRINTF(format_idx,arg_idx) \
                    __attribute__((__format__(__printf__,format_idx,arg_idx)))
#else
#define LIKE_PRINTF(format_idx,arg_idx) /* no attribute */
#endif

/* indicates that the function is "pure": it's result is purely based on
   the parameters and has no side effects or used static data */
#if GCC_VERSION(3,0)
#define PURE        __attribute__((__pure__))
#else
#define PURE        /* no attribute */
#endif

/* the function returns a new data structure that has been freshly
   allocated */
#if GCC_VERSION(3,0)
#define LIKE_MALLOC __attribute__((__malloc__))
#else
#define LIKE_MALLOC /* no attribute */
#endif

/* the function's return value should be used by the caller */
#if GCC_VERSION(3,4)
#define MUST_USE    __attribute__((__warn_unused_result__))
#else
#define MUST_USE    /* no attribute */
#endif

/* the function's return value should be used by the caller */
#if GCC_VERSION(2,5)
#define NORETURN    __attribute__((__noreturn__))
#else
#define NORETURN    /* no attribute */
#endif

/* define __STRING if it's not yet defined */
#ifndef __STRING
#ifdef __STDC__
#define __STRING(x) #x
#else /* __STDC__ */
#define __STRING(x) "x"
#endif /* not __STDC__ */
#endif /* not __STRING */

#endif /* not COMPAT__ATTRS_H */
