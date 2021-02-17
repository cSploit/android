/* DO NOT EDIT! GENERATED AUTOMATICALLY! */
/* Provide a more complete sys/types.h.

   Copyright (C) 2011-2013 Free Software Foundation, Inc.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 3, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, see <http://www.gnu.org/licenses/>.  */

#if __GNUC__ >= 3
#pragma GCC system_header
#endif


#ifndef _GL_SYS_TYPES_H

/* The include_next requires a split double-inclusion guard.  */
#include_next <sys/types.h>

#ifndef _GL_SYS_TYPES_H
#define _GL_SYS_TYPES_H

/* Override off_t if Large File Support is requested on native Windows.  */
#if 0
/* Same as int64_t in <stdint.h>.  */
# if defined _MSC_VER
#  define off_t __int64
# else
#  define off_t long long int
# endif
/* Indicator, for gnulib internal purposes.  */
# define _GL_WINDOWS_64_BIT_OFF_T 1
#endif

/* MSVC 9 defines size_t in <stddef.h>, not in <sys/types.h>.  */
/* But avoid namespace pollution on glibc systems.  */
#if ((defined _WIN32 || defined __WIN32__) && ! defined __CYGWIN__) \
    && ! defined __GLIBC__
# include <stddef.h>
#endif

#endif /* _GL_SYS_TYPES_H */
#endif /* _GL_SYS_TYPES_H */
