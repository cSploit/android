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
 * svn_swigcompat.swg: Definitions providing compatibility with old
 *   SWIG version.
 */

/* %append_output(obj) */
#if SWIG_VERSION <= 0x010327
#ifdef SWIGPYTHON
%fragment("t_output_helper");
#define %append_output(obj) $result = t_output_helper($result, obj)
#endif

#ifdef SWIGPERL
#define %append_output(obj) \
  if (argvi >= items) EXTEND(sp,1); $result = obj; argvi++
#endif

#ifdef SWIGRUBY
%fragment("output_helper");
#define %append_output(obj) $result = output_helper($result, obj)
#endif
#endif


/* %set_constant(name, value) */
#if SWIG_VERSION <= 0x010327
#ifdef SWIGPYTHON
%define %set_constant(name, value)
PyDict_SetItemString(d, name, value);
%enddef
#endif
#endif


#ifdef SWIGPERL
#if SWIG_VERSION == 0x010324
%typemap(varin) apr_pool_t * {
  void *temp;
  if (SWIG_ConvertPtr($input, (void **) &temp, $1_descriptor,0) < 0) {
    croak("Type error in argument $argnum of $symname. Expected $1_mangle");
  }
  $1 = ($1_ltype) temp;
}
#endif
#endif


#ifdef SWIGRUBY
#if SWIG_VERSION <= 0x010327
/* Remove compiler warning (break strict-aliasing rules) */
%typemap(in) SWIGTYPE * (void *temp)
{
#if SWIG_VERSION <= 0x010325
  SWIG_ConvertPtr($input, &temp, $1_descriptor, 1);
#else
  SWIG_ConvertPtr($input, &temp, $1_descriptor, $disown);
#endif
  $1 = ($1_ltype)temp;
}
#endif

#if SWIG_VERSION <= 0x010324
/* for SWIG bug */
%define OUTPUT_TYPEMAP(type, converter, convtype)
%typemap(in, numinputs=0) type *OUTPUT ($*1_ltype temp),
                          type &OUTPUT ($*1_ltype temp) "$1 = &temp;";
%typemap(argout, fragment="output_helper") type *OUTPUT, type &OUTPUT {
   VALUE o = converter(convtype (*$1));
   $result = output_helper($result, o);
}
%enddef

OUTPUT_TYPEMAP(long, INT2NUM, (long));
OUTPUT_TYPEMAP(long long, LL2NUM, (apr_int64_t));
OUTPUT_TYPEMAP(unsigned long, UINT2NUM, (unsigned long));
OUTPUT_TYPEMAP(unsigned long long, ULL2NUM, (apr_uint64_t));

#undef OUTPUT_TYPEMAP
#endif
#endif
