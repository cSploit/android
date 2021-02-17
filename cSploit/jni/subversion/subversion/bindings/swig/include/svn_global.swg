/*
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 *
 * svn_global.swg: Central point for global definitions to be used in
 *  every bindings module.  This file is %included before anything
 *  else, in every Subversion .i file.
 */

/* 
 * Explicitly define SVN_DEPRECATED so SWIG doesn't have issues parsing
 * our headers.
 */
#ifndef SVN_DEPRECATED
#define SVN_DEPRECATED
#endif

%include typemaps.i
%include constraints.i
%include exception.i

/* Include this early, so it can redefine typemaps included from the
   SWIG standard library before they get %apply-ed anywhere. */
%include svn_swigcompat.swg

/* SWIG can't understand __attribute__(x), so we make it go away */
#define __attribute__(x)

/* SWIG doesn't handle variadic parameters well */
%ignore svn_string_createv;
%ignore svn_stringbuf_createv;

%{
#include "svn_time.h"
#include "svn_pools.h"
%}

#ifdef SWIGPYTHON
%{
#include "swigutil_py.h"
%}
#endif
#ifdef SWIGPERL
%{
#include "swigutil_pl.h"
%}
#endif
#ifdef SWIGRUBY
%{
#include "swigutil_rb.h"
%}
#endif


#ifdef SWIGPYTHON
%feature("autodoc",1);

%{
static PyObject * _global_py_pool = NULL;
%}

/* The SWIG $argnum variable reports the current argument number
   in the underlying C code. This is a bit counterintuitive for
   Python programmers, because the underlying C function may have
   very different argument numbers than the wrapper Python function.

   SWIG doesn't provide any good way of getting Python argument
   numbers, so we extract the argument number using macros. This
   isn't a perfect solution, but it does the job. */
#define $svn_argnum svn_argnum_$input

/* We assume here that Subversion functions have no more than
   40 fixed parameters. If you want to wrap a function that has
   more fixed parameters, you'll need to add more #define
   statements. */
%{
#define svn_argnum_obj0 1
#define svn_argnum_obj1 2
#define svn_argnum_obj2 3
#define svn_argnum_obj3 4
#define svn_argnum_obj4 5
#define svn_argnum_obj5 6
#define svn_argnum_obj6 7
#define svn_argnum_obj7 8
#define svn_argnum_obj8 9
#define svn_argnum_obj9 10
#define svn_argnum_obj10 11
#define svn_argnum_obj11 12
#define svn_argnum_obj12 13
#define svn_argnum_obj13 14
#define svn_argnum_obj14 15
#define svn_argnum_obj15 16
#define svn_argnum_obj16 17
#define svn_argnum_obj17 18
#define svn_argnum_obj18 19
#define svn_argnum_obj19 20
#define svn_argnum_obj20 21
#define svn_argnum_obj21 22
#define svn_argnum_obj22 23
#define svn_argnum_obj23 24
#define svn_argnum_obj24 25
#define svn_argnum_obj25 26
#define svn_argnum_obj26 27
#define svn_argnum_obj27 28
#define svn_argnum_obj28 29
#define svn_argnum_obj29 30
#define svn_argnum_obj30 31
#define svn_argnum_obj31 32
#define svn_argnum_obj32 33
#define svn_argnum_obj33 34
#define svn_argnum_obj34 35
#define svn_argnum_obj35 36
#define svn_argnum_obj36 37
#define svn_argnum_obj37 38
#define svn_argnum_obj38 39
#define svn_argnum_obj39 40
%}

/* Python format specifiers. Use Python instead of SWIG to parse
   these basic types, because Python reports better error messages
   (with correct argument numbers). */
%typemap (in, parse="s")
  char *, char const *, char * const, char const * const "";
%typemap (in, parse="c") char "";
%typemap (in, fragment=SWIG_As_frag(long)) long
{
  $1 = ($1_ltype)SWIG_As(long)($input);
  if (SWIG_arg_fail($svn_argnum)) {
    SWIG_fail;
  }
}
%typemap (in, fragment=SWIG_As_frag(unsigned long)) unsigned long
{
  $1 = ($1_ltype)SWIG_As(unsigned long)($input);
  if (SWIG_arg_fail($svn_argnum)) {
    SWIG_fail;
  }
}
%apply long { short, int, ssize_t, enum SWIGTYPE, svn_boolean_t,
              apr_seek_where_t, apr_fileperms_t }
%apply unsigned long { unsigned char, unsigned short, unsigned int, size_t }

%clear long long;
%clear unsigned long long;

/* These typemaps use apr_int64_t so that they will work on Win32 and Unix */
%typemap (in) long long
  " $1 = ($1_ltype) PyLong_AsLongLong($input); ";

%typemap (in) unsigned long long
  " $1 = ($1_ltype) PyLong_AsUnsignedLongLong($input); ";

%typemap (out) apr_time_t, apr_int64_t, long long, __int64
  " $result = PyLong_FromLongLong((apr_int64_t)($1)); ";

%typemap (out) apr_uint64_t, unsigned long long, unsigned __int64
  " $result = PyLong_FromUnsignedLongLong((apr_uint64_t)($1)); ";

%typemap(in,numinputs=0) long long *OUTPUT (apr_int64_t temp)
    "$1 = &temp;";
%typemap(argout) long long *OUTPUT {
  %append_output(PyLong_FromLongLong(*$1));
}

#endif


#ifdef SWIGRUBY
%clear long long;

%typemap (in) long long
{
  $1 = ($1_ltype)NUM2LL($input);
}

%typemap (out) long long
{
  $result = LL2NUM(($1_ltype)($1));
}

%typemap(argout) long long *OUTPUT
{
  %append_output(LL2NUM(*$1));
}

%clear unsigned long long;

%typemap (in) unsigned long long
{
  $1 = ($1_ltype)NUM2ULL($input);
}

%typemap (out) unsigned long long
{
  $result = ULL2NUM(($1_ltype)($1));
}

%typemap(argout) unsigned long long *OUTPUT
{
  %append_output(ULL2NUM(*$1));
}

#endif


#ifdef SWIGRUBY
/* Ruby has strict capitalization conventions -
   tell SWIG to not warn as it renames things to follow these. */
#pragma SWIG nowarn=801

%{
static VALUE _global_svn_swig_rb_pool = Qnil;
static apr_pool_t *_global_pool = NULL;
static VALUE vresult = Qnil;
static VALUE *_global_vresult_address = &vresult;
%}
#endif


/* Now, include the main Subversion typemap library. */
%include svn_types.swg
%include proxy.swg
