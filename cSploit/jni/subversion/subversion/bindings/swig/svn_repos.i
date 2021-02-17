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
 * svn_repos.i: SWIG interface file for svn_repos.h
 */

#if defined(SWIGPYTHON)
%module(package="libsvn") repos
#elif defined(SWIGPERL)
%module "SVN::_Repos"
#elif defined(SWIGRUBY)
%module "svn::ext::repos"
#endif

%include svn_global.swg
%import core.i
%import svn_delta.i
%import svn_fs.i

/* -----------------------------------------------------------------------
   %apply-ing of typemaps defined elsewhere
*/
%apply const char *MAY_BE_NULL {
    const char *src_entry,
    const char *unused_1,
    const char *unused_2,
    const char *token,
    const char *user,
    const char *log_msg,
    const char *lock_token,
    const char *tgt_path,
    const char *parent_dir
};

#ifdef SWIGPYTHON
%apply svn_stream_t *WRAPPED_STREAM { svn_stream_t * };
#endif

#ifdef SWIGRUBY
%apply const char *NOT_NULL {
  const char *path
};

%apply svn_stream_t *MAY_BE_NULL {
    svn_stream_t *dumpstream_may_be_null,
    svn_stream_t *feedback_stream
};
#endif

%callback_typemap(svn_repos_history_func_t history_func, void *history_baton,
                  svn_swig_py_repos_history_func,
                  svn_swig_pl_thunk_history_func,
                  svn_swig_rb_repos_history_func)

%callback_typemap_maybenull(svn_repos_authz_func_t authz_read_func,
                            void *authz_read_baton,
                            svn_swig_py_repos_authz_func,
                            svn_swig_pl_thunk_authz_func,
                            svn_swig_rb_repos_authz_func)

#ifdef SWIGPYTHON
%callback_typemap(svn_location_segment_receiver_t receiver, void *receiver_baton,
                  svn_swig_py_location_segment_receiver_func,
                  ,
                  )
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_error_t *(*start_callback)(void *), void *start_callback_baton)
{
  $1 = svn_swig_rb_just_call;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}
#endif

#ifdef SWIGRUBY
%callback_typemap(svn_repos_file_rev_handler_t handler, void *handler_baton,
                  ,
                  ,
                  svn_swig_rb_repos_file_rev_handler)

%callback_typemap(svn_repos_authz_func_t authz_read_func,
                  void *authz_read_baton,
                  ,
                  ,
                  svn_swig_rb_repos_authz_func)

%callback_typemap(svn_repos_authz_callback_t authz_callback, void *authz_baton,
                  ,
                  ,
                  svn_swig_rb_repos_authz_callback)
#endif

/* -----------------------------------------------------------------------
   handle svn_repos_get_committed_info().
*/
#ifdef SWIGRUBY
%typemap(argout) const char **committed_date {
  %append_output(svn_swig_rb_svn_date_string_to_time(*$1));
}
#endif


/* ----------------------------------------------------------------------- */
/* Ruby fixups for functions not following the pool convention. */
#ifdef SWIGRUBY
%ignore svn_repos_fs;
%inline %{
static svn_fs_t *
svn_repos_fs_wrapper(svn_repos_t *fs, apr_pool_t *pool)
{
  return svn_repos_fs(fs);
}
%}
#endif

/* ----------------------------------------------------------------------- */

#ifdef SWIGRUBY
svn_error_t *svn_repos_dump_fs2(svn_repos_t *repos,
                                svn_stream_t *dumpstream_may_be_null,
                                svn_stream_t *feedback_stream,
                                svn_revnum_t start_rev,
                                svn_revnum_t end_rev,
                                svn_boolean_t incremental,
                                svn_boolean_t use_deltas,
                                svn_cancel_func_t cancel_func,
                                void *cancel_baton,
                                apr_pool_t *pool);
%ignore svn_repos_dump_fs2;
#endif

%include svn_repos_h.swg

#ifdef SWIGRUBY
%define_close_related_methods(repos)
#endif
