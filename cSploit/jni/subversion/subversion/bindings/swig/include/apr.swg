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
 * apr.swg: This is a child file of svn_types.swg, and should not be
 *   included directly.  This file should contain typemaps that deal
 *   with APR functionality, not involving any parts of Subversion.
 */

/* -----------------------------------------------------------------------
   Create perl5 typemaps for long long datatypes
   PERL-FIXME: These typemaps are decidedly odd.  They convert long long
     values to strings before returning them.  This behaviour is either
     wrong, or needful of a comment explaining it.
*/
#ifdef SWIGPERL
%typemap(out) long long {
    char temp[256];
    sprintf(temp, "%" APR_INT64_T_FMT, (apr_int64_t) $1);
    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++), temp);
}

%typemap(out) unsigned long long {
    char temp[256];
    sprintf(temp, "%" APR_UINT64_T_FMT, (apr_uint64_t) $1);
    ST(argvi) = sv_newmortal();
    sv_setpv((SV*)ST(argvi++), temp);
}

%typemap(in, numinputs=0) long long *OUTPUT (apr_int64_t temp)
    "$1 = &temp;";
%typemap(argout) long long *OUTPUT {
  char temp[256];
  sprintf(temp, "%" APR_INT64_T_FMT, (apr_int64_t)*($1));
  %append_output(sv_2mortal(newSVpv(temp, 0)));
}

%typemap(in, numinputs=0) unsigned long long *OUTPUT (apr_uint64_t temp)
    "$1 = &temp;";
%typemap(argout) unsigned long long *OUTPUT {
  char temp[256];
  sprintf(temp, "%" APR_UINT64_T_FMT, (apr_uint64_t)*($1));
  %append_output(sv_2mortal(newSVpv(temp, 0)));
}
#endif

/* -----------------------------------------------------------------------
 * APR datatypes (from apr.h)
 */

/* Integers for which sizeof(type) <= sizeof(long) */
%apply long { apr_int16_t, apr_int32_t, apr_ssize_t }
%apply unsigned long { apr_byte_t, apr_uint16_t, apr_uint32_t, apr_size_t }
%apply unsigned long *OUTPUT { apr_uint32_t *, apr_size_t * }

/* If sizeof(apr_off_t) > sizeof(apr_int64_t), apr_off_t will get truncated
 * to a 64-bit integer */
%apply long long { apr_off_t, apr_int64_t }
%apply long long *OUTPUT { apr_int64_t * };
%apply unsigned long long { apr_uint64_t }

/* -----------------------------------------------------------------------
   apr_time_t
*/

/* Define the time type (rather than picking up all of apr_time.h) */
typedef apr_int64_t apr_time_t;

/* For apr_time_ansi_put().
   We guess, because only the system C compiler can successfully parse
   system headers if they incorporate weird include paths
   (e.g. /usr/lib/gcc-lib/plat/ver/include). */
typedef apr_int32_t time_t;

/* -----------------------------------------------------------------------
   Input of apr_file_t *
*/

#ifdef SWIGPYTHON
%typemap(in) apr_file_t * {
  $1 = svn_swig_py_make_file($input, _global_pool);
  if (!$1) SWIG_fail;
}
#endif

#ifdef SWIGPERL
%typemap(in) apr_file_t * {
  $1 = svn_swig_pl_make_file($input, _global_pool);
}
#endif

#ifdef SWIGRUBY
%typemap(in) apr_file_t * {
  $1 = svn_swig_rb_make_file($input, _global_pool);
}
#endif

/* -----------------------------------------------------------------------
   Output of apr_file_t *
   ... is currently being handled by the standard OUTPARAM code in
   svn_types.swg, but should probably be more comfortably translated to
   scripting language concepts.
*/
