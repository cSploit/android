/* posixjmp.h -- wrapper for setjmp.h with changes for POSIX systems. */

/* Copyright (C) 1987,1991 Free Software Foundation, Inc.

   This file is part of GNU Bash, the Bourne Again SHell.

   Bash is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   Bash is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with Bash.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef _POSIXJMP_H_
#define _POSIXJMP_H_

#include <setjmp.h>

/* This *must* be included *after* config.h */

#if defined (HAVE_POSIX_SIGSETJMP)
#  define procenv_t	sigjmp_buf
#  if !defined (__OPENNT)
#    undef setjmp
#    define setjmp(x)	sigsetjmp((x), 1)
#    define setjmp_nosigs(x)	sigsetjmp((x), 0)
#    undef longjmp
#    define longjmp(x, n)	siglongjmp((x), (n))
#  endif /* !__OPENNT */
#else
#  define procenv_t	jmp_buf
#  define setjmp_nosigs	setjmp
#endif

#endif /* _POSIXJMP_H_ */
