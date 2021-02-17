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
 * svn_ra.i: SWIG interface file for svn_ra.h
 */

#if defined(SWIGPYTHON)
%module(package="libsvn") ra
#elif defined(SWIGPERL)
%module "SVN::_Ra"
#elif defined(SWIGRUBY)
%module "svn::ext::ra"
#endif

%include svn_global.swg
%import core.i
%import svn_delta.i

/* Bad pool convention, also these are not public interfaces, they were
   simply placed in the public header by mistake. */
%ignore svn_ra_svn_init;
%ignore svn_ra_local_init;
%ignore svn_ra_dav_init;
%ignore svn_ra_serf_init;

%apply Pointer NONNULL { svn_ra_callbacks2_t *callbacks };

/* -----------------------------------------------------------------------
   %apply-ing of typemaps defined elsewhere
*/
%apply const char *MAY_BE_NULL {
    const char *comment,
    const char *lock_token
};

#ifdef SWIGPYTHON
%apply svn_stream_t *WRAPPED_STREAM { svn_stream_t * };
#endif

/* ----------------------------------------------------------------------- */

#ifdef SWIGPYTHON
%typemap(in) (const svn_ra_callbacks2_t *callbacks, void *callback_baton) {
  svn_swig_py_setup_ra_callbacks(&$1, &$2, $input, _global_pool);
}
/* FIXME: svn_ra_callbacks_t ? */
#endif
#ifdef SWIGPERL
/* FIXME: svn_ra_callbacks2_t ? */
%typemap(in) (const svn_ra_callbacks_t *callbacks, void *callback_baton) {
  svn_ra_make_callbacks(&$1, &$2, $input, _global_pool);
}
#endif
#ifdef SWIGRUBY
%typemap(in) (const svn_ra_callbacks2_t *callbacks, void *callback_baton) {
  svn_swig_rb_setup_ra_callbacks(&$1, &$2, $input, _global_pool);
}
/* FIXME: svn_ra_callbacks_t ? */
#endif

#ifdef SWIGPYTHON
%callback_typemap(const svn_ra_reporter2_t *reporter, void *report_baton,
                  (svn_ra_reporter2_t *)&swig_py_ra_reporter2,
                  ,
                  )
%callback_typemap(svn_location_segment_receiver_t receiver, void *receiver_baton,
                  svn_swig_py_location_segment_receiver_func,
                  ,
                  )
#endif

#ifdef SWIGRUBY
%callback_typemap(const svn_ra_reporter3_t *reporter, void *report_baton,
                  ,
                  ,
                  svn_swig_rb_ra_reporter3)
#endif

#ifndef SWIGPERL
%callback_typemap(svn_ra_file_rev_handler_t handler, void *handler_baton,
                  svn_swig_py_ra_file_rev_handler_func,
                  ,
                  svn_swig_rb_ra_file_rev_handler)
#endif

%callback_typemap(svn_ra_lock_callback_t lock_func, void *lock_baton,
                  svn_swig_py_ra_lock_callback,
                  svn_swig_pl_ra_lock_callback,
                  svn_swig_rb_ra_lock_callback)

/* ----------------------------------------------------------------------- */

%include svn_ra_h.swg
