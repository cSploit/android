/****************************************************************************
 * Copyright (c) 1998-2007,2009 Free Software Foundation, Inc.              *
 *                                                                          *
 * Permission is hereby granted, free of charge, to any person obtaining a  *
 * copy of this software and associated documentation files (the            *
 * "Software"), to deal in the Software without restriction, including      *
 * without limitation the rights to use, copy, modify, merge, publish,      *
 * distribute, distribute with modifications, sublicense, and/or sell       *
 * copies of the Software, and to permit persons to whom the Software is    *
 * furnished to do so, subject to the following conditions:                 *
 *                                                                          *
 * The above copyright notice and this permission notice shall be included  *
 * in all copies or substantial portions of the Software.                   *
 *                                                                          *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS  *
 * OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF               *
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.   *
 * IN NO EVENT SHALL THE ABOVE COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,   *
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR    *
 * OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR    *
 * THE USE OR OTHER DEALINGS IN THE SOFTWARE.                               *
 *                                                                          *
 * Except as contained in this notice, the name(s) of the above copyright   *
 * holders shall not be used in advertising or otherwise to promote the     *
 * sale, use or other dealings in this Software without prior written       *
 * authorization.                                                           *
 ****************************************************************************/
/* $Id: ncurses_dll.h.in,v 1.8 2009/04/04 22:26:27 tom Exp $ */

#ifndef NCURSES_DLL_H_incl
#define NCURSES_DLL_H_incl 1

/*
 * For reentrant code, we map the various global variables into SCREEN by
 * using functions to access them.
 */
#define NCURSES_PUBLIC_VAR(name) _nc_##name
#define NCURSES_WRAPPED_VAR(type,name) extern type NCURSES_PUBLIC_VAR(name)(void)

/* no longer needed on cygwin or mingw, thanks to auto-import       */
/* but this structure may be useful at some point for an MSVC build */
/* so, for now unconditionally define the important flags           */
/* "the right way" for proper static and dll+auto-import behavior   */
#undef NCURSES_DLL
#define NCURSES_STATIC

#if defined(__CYGWIN__) || defined(__MINGW32__)
#  if defined(NCURSES_DLL)
#    if defined(NCURSES_STATIC)
#      undef NCURSES_STATIC
#    endif
#  endif
#  undef NCURSES_IMPEXP
#  undef NCURSES_API
#  undef NCURSES_EXPORT
#  undef NCURSES_EXPORT_VAR
#  if defined(NCURSES_DLL)
/* building a DLL */
#    define NCURSES_IMPEXP __declspec(dllexport)
#  elif defined(NCURSES_STATIC)
/* building or linking to a static library */
#    define NCURSES_IMPEXP /* nothing */
#  else
/* linking to the DLL */
#    define NCURSES_IMPEXP __declspec(dllimport)
#  endif
#  define NCURSES_API __cdecl
#  define NCURSES_EXPORT(type) NCURSES_IMPEXP type NCURSES_API
#  define NCURSES_EXPORT_VAR(type) NCURSES_IMPEXP type
#endif

/* Take care of non-cygwin platforms */
#if !defined(NCURSES_IMPEXP)
#  define NCURSES_IMPEXP /* nothing */
#endif
#if !defined(NCURSES_API)
#  define NCURSES_API /* nothing */
#endif
#if !defined(NCURSES_EXPORT)
#  define NCURSES_EXPORT(type) NCURSES_IMPEXP type NCURSES_API
#endif
#if !defined(NCURSES_EXPORT_VAR)
#  define NCURSES_EXPORT_VAR(type) NCURSES_IMPEXP type
#endif

#endif /* NCURSES_DLL_H_incl */
