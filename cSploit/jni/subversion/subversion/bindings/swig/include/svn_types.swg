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
 * svn_types.swg: This is the main file of the Subversion typemap
 *   library.  Certain categories of typemaps are removed into child
 *   files that are then %included into this one, solely to promote
 *   code comprehensibility by grouping related parts.
 */

%include apr.swg
%include svn_string.swg
%include svn_containers.swg

/* -----------------------------------------------------------------------
   Generic handling for "type **" parameters.  Almost all such parameters
   are OUTPUT parameters.  Handling is as follows:
   - "in" typemap which allocates storage for a pointer, and points the
     argument to that storage.
   - Default "argout" typemap which complains at compile time, and raises
     an exception at runtime.
   - OUTPARAM "argout" typemap which wraps (SWIG_NewPointerObj) a C
     pointer for return to the scripting language.
   - Application of the OUTPARAM typemap to those types for which it makes
     sense (key examples for which it does not are strings, arrays, and
     hashes - which need to be turned into their scripting language
     equivalent).
   Known instances of "type **" which are *not* OUTPUT parameters are:
   - svn_stream_checksummed(..., const unsigned char **read_digest,
                                 const unsigned char **write_digest, ...)
*/
%typemap(in, numinputs=0) SWIGTYPE ** ($*1_ltype temp) "$1 = &temp;";

/* We have a long standing bug where svn_client_commit_info_t and
   svn_commit_info_t output parameters are not properly initialized to NULL
   when there is nothing to commit.  This is a workaround.  Yes, it is
   unspeakably evil that this was not just fixed at source when it was
   discovered initially. */
%typemap(in, numinputs=0) BAD_OUTPUT_INIT_HACK ** ($*1_ltype temp = NULL)
  "$1 = &temp;";
%apply BAD_OUTPUT_INIT_HACK ** {
  svn_commit_info_t **,
  svn_client_commit_info_t **
};

%typemap(argout,warning="900:FIXME: Missing argout typemap") SWIGTYPE ** {
  /* FIXME: Missing argout typemap: $symname arg $argnum ($1_type) */
#if defined(SWIGRUBY) && SWIG_VERSION <= 0x010329
  /* Ruby fails to define $symname. */
  SWIG_exception(SWIG_ValueError, "Function is not implemented yet");
#else
  SWIG_exception(SWIG_ValueError, "$symname is not implemented yet");
#endif
}

#ifdef SWIGPYTHON
%typemap(argout) SWIGTYPE **OUTPARAM {
  %append_output(svn_swig_NewPointerObj(*$1, $*1_descriptor,
                                        _global_py_pool, args));
}
#else
%typemap(argout) SWIGTYPE **OUTPARAM {
  %append_output(SWIG_NewPointerObj(*$1, $*1_descriptor, 0));
}
#endif

%apply SWIGTYPE **OUTPARAM {
  /* apr */
  apr_file_t **,
  /* svn_types.h */
  svn_commit_info_t **,
  svn_dirent_t **,
  svn_lock_t **,
  svn_stream_t **,
  svn_commit_callback2_t *,
  void **callback2_baton,
  /* core */
  svn_auth_baton_t **,
  svn_auth_cred_simple_t **cred,
  svn_auth_cred_username_t **cred,
  svn_auth_cred_ssl_client_cert_pw_t **cred,
  svn_auth_cred_ssl_client_cert_t **cred,
  svn_auth_cred_ssl_server_trust_t **cred,
  svn_auth_iterstate_t **,
  svn_auth_provider_object_t **,
  svn_config_t **,
  svn_diff_t **,
  void **credentials,
  void **iter_baton,
  void **token,
  /* svn_client */
  svn_client_commit_info_t **,
  svn_client_ctx_t **,
  const svn_client_commit_item3_t **,
  /* svn_delta */
  const svn_delta_editor_t **,
  svn_txdelta_stream_t **,
  svn_txdelta_window_t **,
#ifndef SWIGPYTHON
  /* Python wraps this as a CALLABLE_CALLBACK instead */
  svn_txdelta_window_handler_t *,
#endif
  void **handler_baton,
  void **root_baton,
  void **child_baton,
  void **file_baton,
  void **edit_baton,
  void **dir_baton,
  /* svn_fs */
  svn_fs_t **,
  svn_fs_access_t **,
  svn_fs_history_t **,
  svn_fs_id_t **,
  svn_fs_root_t **,
  svn_fs_txn_t **,
  void **contents_baton_p,
  /* svn_ra */
  svn_ra_callbacks2_t **,
  svn_ra_plugin_t **,
  const svn_ra_reporter_t **,
  const svn_ra_reporter2_t **,
  const svn_ra_reporter3_t **,
  svn_ra_session_t **,
  void **delta_baton,
  void **report_baton,
  void **ra_baton,
  void **session_baton,
  /* svn_repos */
  svn_authz_t **,
  svn_repos_t **,
  const svn_repos_parse_fns2_t **,
  const svn_repos_parser_fns_t **,
  void **parse_baton,
  void **revision_baton,
  void **node_baton,
  /* svn_wc */
  svn_wc_adm_access_t **,
  svn_wc_external_item2_t **,
  svn_wc_entry_t **,
  svn_wc_revision_status_t **,
  svn_wc_status_t **,
  svn_wc_status2_t **,
  svn_wc_committed_queue_t **,
#ifdef SWIGRUBY
  /* Only tested for Ruby */
  svn_wc_context_t **,
#endif
  void **set_locks_baton
};

#ifdef SWIGRUBY
%typemap(argout) SWIGTYPE **OUTPARAM_WITH_BLOCK {
  VALUE tmp;
  tmp = SWIG_NewPointerObj(*$1, $*1_descriptor, 0);
  if (rb_block_given_p()) {
    rb_yield(tmp);
    svn_swig_rb_destroy_pool(_global_svn_swig_rb_pool);
    DATA_PTR(tmp) = NULL;
  } else {
    %append_output(tmp);
  }
}

%apply SWIGTYPE **OUTPARAM_WITH_BLOCK {
  /* svn_fs */
  svn_fs_t **,
  /* svn_repos */
  svn_repos_t **
};
#endif


/* -----------------------------------------------------------------------
   Check whether fs/repos was closed or not.
*/
#ifdef SWIGRUBY
%define %raise_if_null(TYPE)
%typemap(check) svn_ ## TYPE ## _t *
{
  if (!$1) {
    svn_swig_rb_raise_svn_ ## TYPE ## _already_close();
  }
}
%enddef

%raise_if_null(fs);
%raise_if_null(repos);

%define %define_close_related_methods(TYPE)
%ignore svn_ ## TYPE ## _swig_rb_close;
%ignore svn_ ## TYPE ## _swig_rb_closed;

%inline %{
static VALUE
svn_ ## TYPE ## _swig_rb_close(VALUE self)
{
  if (!DATA_PTR(self)) {
    svn_swig_rb_raise_svn_ ## TYPE ## _already_close();
  }

  svn_swig_rb_destroy_internal_pool(self);
  DATA_PTR(self) = NULL;

  return Qnil;
}

static VALUE
svn_ ## TYPE ## _swig_rb_closed(VALUE self)
{
  return DATA_PTR(self) ? Qfalse : Qtrue;
}
%}

%init %{
  {
    VALUE cSvn ## TYPE;
    cSvn ## TYPE = rb_const_get(_mSWIG, rb_intern("TYPE_p_svn_" #TYPE "_t"));
    rb_define_method(cSvn ## TYPE, "close",
                     svn_ ## TYPE ## _swig_rb_close, 0);
    rb_define_method(cSvn ## TYPE, "closed?",
                     svn_ ## TYPE ## _swig_rb_closed, 0);
  }
%}
%enddef
#endif

/* -----------------------------------------------------------------------
   %apply-ing of typemaps
*/

%apply int *OUTPUT {
  int *,
  svn_boolean_t *,
  enum svn_wc_merge_outcome_t *
};
%apply long *OUTPUT { svn_revnum_t *, svn_node_kind_t * };
%apply long long *OUTPUT { svn_filesize_t * }

/* -----------------------------------------------------------------------
   Generic macros for callback typemaps
*/

#ifdef SWIGPYTHON
%define %callback_typemap(CallbackFunction, CallbackBaton,
                          PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  $1 = PythonThunk;
  $2 = $input;
}
%enddef
%define %callback_typemap_maybenull(CallbackFunction, CallbackBaton,
                                    PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  /* FIXME: Handle the NULL case. */
  $1 = PythonThunk;
  $2 = $input;
}
%enddef
#endif

#ifdef SWIGPERL
%define %callback_typemap(CallbackFunction, CallbackBaton,
                          PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  $1 = PerlThunk;
  $2 = $input;
}
%enddef
%define %callback_typemap_maybenull(CallbackFunction, CallbackBaton,
                                    PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  if (SvOK($input)) {
    $1 = PerlThunk;
    $2 = $input;
  } else {
    $1 = NULL;
    $2 = NULL;
  }
}
%enddef
#endif

#ifdef SWIGRUBY
%define %callback_typemap(CallbackFunction, CallbackBaton,
                          PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  $1 = RubyThunk;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}
%enddef
%define %callback_typemap_maybenull(CallbackFunction, CallbackBaton,
                                    PythonThunk, PerlThunk, RubyThunk)
%typemap(in) (CallbackFunction, CallbackBaton) {
  if (NIL_P($input)) {
    $1 = NULL;
    $2 = NULL;
  } else {
    $1 = RubyThunk;
    $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
  }
}
%enddef
#endif

/* -----------------------------------------------------------------------
   Create a typemap for specifying string args that may be NULL.
*/
#ifdef SWIGPYTHON
%typemap(in, parse="z") const char *MAY_BE_NULL "";
#endif

#ifdef SWIGPERL
%apply const char * { const char *MAY_BE_NULL };
#endif

#ifdef SWIGRUBY
%typemap(in) const char *MAY_BE_NULL
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    $1 = StringValuePtr($input);
  }
}

%typemap(in) const char *NOT_NULL
{
  $1 = StringValueCStr($input);
}

%typemap(out) const char *
{
  if ($1) {
    $result = rb_str_new2($1);
  } else {
    $result = Qnil;
  }
}
#endif

/* -----------------------------------------------------------------------
   const char *header_encoding
   svn_diff_file_output_unified2
   svn_client_diff3
*/
#ifdef SWIGRUBY
%typemap(in) const char *header_encoding
{
  $1 = NULL;

  if (NIL_P($input)) {
  } else if (TYPE($input) == T_FIXNUM) {
    $1 = (char *)NUM2INT($input);
    if (!($1 == APR_LOCALE_CHARSET || $1 == APR_DEFAULT_CHARSET)) {
      $1 = NULL;
    }
  } else {
    $1 = StringValuePtr($input);
  }

  if (!$1) {
    $1 = (char *)APR_LOCALE_CHARSET;
  }
}
#endif

/* -----------------------------------------------------------------------
   Define a more refined 'memberin' typemap for 'const char *' members. This
   is used in place of the 'char *' handler defined automatically.

   We need to do the free/malloc/strcpy special because of the const
*/
%typemap(memberin) const char * {
    apr_size_t len = strlen($input) + 1;
    char *copied;
    if ($1) free((char *)$1);
    copied = malloc(len);
    memcpy(copied, $input, len);
    $1 = copied;
}

/* -----------------------------------------------------------------------
   Specify how svn_error_t returns are turned into exceptions.
*/
#ifdef SWIGPYTHON
%typemap(out) svn_error_t * {
    if ($1 != NULL) {
        if ($1->apr_err != SVN_ERR_SWIG_PY_EXCEPTION_SET)
            svn_swig_py_svn_exception($1);
        else
            svn_error_clear($1);
        SWIG_fail;
    }
    Py_INCREF(Py_None);
    $result = Py_None;
}
#endif

#ifdef SWIGPERL
%typemap(out) svn_error_t * {
    if ($1) {
        SV *exception_handler = perl_get_sv ("SVN::Error::handler", FALSE);

        if (SvOK(exception_handler)) {
            SV *callback_result;

            svn_swig_pl_callback_thunk (CALL_SV, exception_handler,
                                        &callback_result, "S", $1,
                                        $1_descriptor);
        } else {
            $result = SWIG_NewPointerObj($1, $1_descriptor, 0);
            argvi++;
        }
    }
}
#endif

#ifdef SWIGRUBY
%typemap(out) svn_error_t *
{
  if ($1) {
    svn_swig_rb_destroy_pool(_global_svn_swig_rb_pool);
    svn_swig_rb_pop_pool(_global_svn_swig_rb_pool);
    svn_swig_rb_handle_svn_error($1);
  }
  $result = Qnil;
}
#endif

/* -----------------------------------------------------------------------
   Define a general ptr/len typemap. This takes a single script argument
   and expands it into a ptr/len pair for the native call.
*/
#ifdef SWIGPYTHON
%typemap(in) (const char *PTR, apr_size_t LEN) {
    if (!PyString_Check($input)) {
        PyErr_SetString(PyExc_TypeError, "expecting a string");
        SWIG_fail;
    }
    $1 = PyString_AS_STRING($input);
    $2 = PyString_GET_SIZE($input);
}
#endif

#ifdef SWIGPERL
%typemap(in) (const char *PTR, apr_size_t LEN) {
    if (SvPOK($input)) {
        $1 = SvPV($input, $2);
    } else {
        /* set to 0 to avoid warning */
        $1 = 0;
        $2 = 0;
        SWIG_croak("Expecting a string");
    }
}
#endif

#ifdef SWIGRUBY
%typemap(in) (const char *PTR, apr_size_t LEN)
{
  if (!RTEST(rb_obj_is_kind_of($input, rb_cString))) {
    rb_raise(rb_eArgError, "Expecting a string");
  }
  $1 = StringValuePtr($input);
  $2 = RSTRING_LEN(StringValue($input));
}
#endif

%apply (const char *PTR, apr_size_t LEN) {
    (const char *data, apr_size_t len)
}

/* -----------------------------------------------------------------------
   Handle retrieving the error message from svn_strerror
*/

#ifdef SWIGPERL
%typemap(in,numinputs=0) (char *buf, apr_size_t bufsize) ( char temp[128] ) {
    memset (temp,0,128); /* paranoia */
    $1 = temp;
    $2 = 128;
}
#endif

/* -----------------------------------------------------------------------
   Define a generic arginit mapping for pools.
*/

#ifdef SWIGPYTHON

%typemap(in) POINTER_TYPES
{
  $1 = ($1_ltype)svn_swig_MustGetPtr($input, $descriptor, $svn_argnum);
  if (PyErr_Occurred()) {
    SWIG_fail;
  }
}

%typemap(out) POINTER_TYPES
    "$result = svn_swig_NewPointerObj((void*)($1), $descriptor,
                                      _global_py_pool, args);";

%apply POINTER_TYPES { void *, SWIGTYPE *, SWIGTYPE [] };

%typemap(default, noblock=1) apr_pool_t
  *(apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (svn_swig_py_get_pool_arg(args, $descriptor,
      &_global_py_pool, &_global_pool))
    SWIG_fail;
  $1 = _global_pool;
}

%typemap(in, noblock=1) apr_pool_t * {
  /* Verify that the user supplied a valid pool */
  if ($input != Py_None && $input != _global_py_pool) {
    SWIG_Python_TypeError(SWIG_TypePrettyName($descriptor), $input);
    SWIG_arg_fail($svn_argnum);
    SWIG_fail;
  }
}

%typemap(freearg) apr_pool_t * {
  Py_XDECREF(_global_py_pool);
}
#endif

#ifdef SWIGPERL
%typemap(in) apr_pool_t *pool "";
%typemap(default) apr_pool_t *pool(apr_pool_t *_global_pool) {
    _global_pool = $1 = svn_swig_pl_make_pool (ST(items-1));
}
#endif
#ifdef SWIGRUBY
%typemap(in) apr_pool_t *pool "";
%typemap(default) apr_pool_t *pool
    (VALUE _global_svn_swig_rb_pool, apr_pool_t *_global_pool)
{
  svn_swig_rb_get_pool(argc, argv, self, &_global_svn_swig_rb_pool, &$1);
  _global_pool = $1;
  svn_swig_rb_push_pool(_global_svn_swig_rb_pool);
}

%typemap(default) (svn_client_ctx_t *ctx, apr_pool_t *pool)
     (VALUE _global_svn_swig_rb_pool, apr_pool_t *_global_pool)
{
  int adjusted_argc = argc;
  VALUE *adjusted_argv = argv;

  svn_swig_rb_adjust_arg_for_client_ctx_and_pool(&adjusted_argc,
                                                 &adjusted_argv);
  svn_swig_rb_get_pool(adjusted_argc, adjusted_argv, self,
                       &_global_svn_swig_rb_pool, &$2);
  _global_pool = $2;
  svn_swig_rb_push_pool(_global_svn_swig_rb_pool);
}

%typemap(freearg) apr_pool_t *pool
{
  VALUE target;
  target = _global_vresult_address == &vresult ? self : vresult;
  if (!svn_swig_rb_set_pool(target, _global_svn_swig_rb_pool))
    svn_swig_rb_destroy_pool(_global_svn_swig_rb_pool);
  svn_swig_rb_pop_pool(_global_svn_swig_rb_pool);
}
#endif

#ifdef SWIGPERL
%apply apr_pool_t *pool {
    apr_pool_t *dir_pool,
    apr_pool_t *file_pool,
    apr_pool_t *node_pool,
    apr_pool_t *result_pool,
    apr_pool_t *scratch_pool
};
#endif

#ifdef SWIGRUBY
%apply apr_pool_t *pool {
    apr_pool_t *dir_pool,
    apr_pool_t *file_pool,
    apr_pool_t *node_pool,
    apr_pool_t *result_pool,
    apr_pool_t *scratch_pool
};
#endif

/* -----------------------------------------------------------------------
 * The CALLABLE_CALLBACK typemap wraps callback types as objects which
 * can be called directly from Python. header_wrappers.py automatically
 * tags all callback types (in Python) with this callback.
 *
 * header_wrappers.py also generates a '__call__' function for the
 * callback objects, using the funcptr_proxy macro. See proxy.swg
 */

#ifdef SWIGPYTHON

/* Allocate memory for a pointer to the function pointer. */
%typemap(in, numinputs=0) CALLABLE_CALLBACK *
  (apr_pool_t *_global_pool = NULL, PyObject *_global_py_pool = NULL)
{
  if (_global_pool == NULL)
  {
    if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                    &_global_py_pool, &_global_pool))
      SWIG_fail;
  }
  $1 = ($ltype) apr_pcalloc(_global_pool, sizeof($*ltype));
  if ($1 == NULL) SWIG_fail;
}


/* Return a pointer to the function pointer.
 * This level of indirection makes it easier for SWIG to treat
 * this callback like an object (which can be extended using SWIG).
 */

%typemap(argout) CALLABLE_CALLBACK * {
  %append_output(svn_swig_NewPointerObj($1, $descriptor,
                                        _global_py_pool, args));
}

/* Convert the pointer to a function pointer back into a regular
 * function pointer
 */

%typemap(in) CALLABLE_CALLBACK {
  $&ltype tmp =
    svn_swig_MustGetPtr($input, $&descriptor, $svn_argnum);
  if (tmp == NULL || PyErr_Occurred()) {
    SWIG_fail;
  }
  $1 = *tmp;
}

/* Ensure that return values use a level of indirection.
 *
 * Allocate space on the heap for a pointer to the callback.
 * The memory gets freed when the return value itself is
 * freed.
 */

%typemap(out) CALLABLE_CALLBACK {
  PyObject *py_pool = NULL;
  apr_pool_t *pool = NULL;

  if (svn_swig_py_get_parent_pool(args, $descriptor(apr_pool_t *),
                                  &py_pool, &pool))
    SWIG_fail;
  if ($1 == NULL) {
    $result = Py_None;
    Py_INCREF($result);
  } else {
    $&ltype tmp = apr_palloc(pool, sizeof($ltype));
    if (tmp == NULL) {
      SWIG_fail;
    }
    *tmp = ($ltype) $1;
    $result = svn_swig_NewPointerObj(tmp, $&1_descriptor,
                                     py_pool, args);
  }
}


/* Ensure that constant callbacks use a level of indirection.
 *
 * Allocate space on the heap for a pointer to the callback.
 * Since this constant is always in scope, we don't need to
 * worry about ever freeing the memory.
 */

%typemap(constcode) CALLABLE_CALLBACK {

  $&ltype tmp = malloc(sizeof($ltype));
  *tmp = ($ltype) $value;

  %set_constant("$symname",
    svn_swig_NewPointerObj(tmp, $&descriptor, NULL, NULL)
  );

}

#endif

/* -----------------------------------------------------------------------
   Callback: svn_log_message_receiver_t
   svn_client_log()
   svn_ra get_log()
   svn_repos_get_logs()
*/

#ifdef SWIGPYTHON
%typemap(in) (svn_log_message_receiver_t receiver,
                      void *receiver_baton) {
    $1 = svn_swig_py_log_receiver;
    $2 = (void *)$input;
}
#endif
#ifdef SWIGPERL
%typemap(in) (svn_log_message_receiver_t receiver,
                     void *receiver_baton) {
    $1 = svn_swig_pl_thunk_log_receiver;
    $2 = (void *)$input;
}
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_log_message_receiver_t receiver,
                    void *receiver_baton) {
    $1 = svn_swig_rb_log_receiver;
    $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}
#endif

/* -----------------------------------------------------------------------
   Callback: svn_log_entry_receiver_t
   svn_client_log4()
   svn_ra_get_log2()
   svn_repos_get_logs4()
*/

#ifdef SWIGPYTHON
%typemap(in) (svn_log_entry_receiver_t receiver,
                      void *receiver_baton) {
    $1 = svn_swig_py_log_entry_receiver;
    $2 = (void *)$input;
}
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_log_entry_receiver_t receiver, void *receiver_baton)
{
  $1 = svn_swig_rb_log_entry_receiver;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}
#endif

/* -----------------------------------------------------------------------
   Callback: svn_commit_callback_t
   svn_ra get_commit_editor()
   svn_repos_get_commit_editor()
*/

#ifdef SWIGPERL
%typemap(in) (svn_commit_callback_t callback, void *callback_baton) {
    $1 = svn_swig_pl_thunk_commit_callback;
    $2 = (void *)$input;
    svn_swig_pl_hold_ref_in_pool (_global_pool, $input);
};
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_commit_callback_t callback, void *callback_baton)
{
  $1 = svn_swig_rb_commit_callback;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
};

%typemap(argout) (svn_commit_callback_t callback, void *callback_baton)
{
  svn_swig_rb_set_baton($result, (VALUE)$2);
};
#endif

#ifdef SWIGPYTHON
%typemap(in) (svn_commit_callback_t callback, void *callback_baton)
{
  $1 = svn_swig_py_commit_callback;
  $2 = (void *)$input;
}
#endif

/* -----------------------------------------------------------------------
   Callback: svn_commit_callback2_t
   svn_ra get_commit_editor2()
   svn_repos_get_commit_editor4()
*/

#ifdef SWIGRUBY
%typemap(in) (svn_commit_callback2_t callback, void *callback_baton)
{
  $1 = svn_swig_rb_commit_callback2;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
};

%typemap(argout) (svn_commit_callback2_t callback, void *callback_baton)
{
  svn_swig_rb_set_baton($result, (VALUE)$2);
};
#endif

#ifdef SWIGPYTHON
%typemap(in) (svn_commit_callback2_t callback, void *callback_baton)
{
  $1 = svn_swig_py_commit_callback2;
  $2 = (void *)$input;
}
#endif

/* -----------------------------------------------------------------------
   Callback: svn_cancel_func_t
*/

#ifdef SWIGPYTHON
%typemap(in) (svn_cancel_func_t cancel_func, void *cancel_baton) {
  $1 = svn_swig_py_cancel_func;
  $2 = $input; /* our function is the baton. */
}
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_cancel_func_t cancel_func, void *cancel_baton)
{
  $1 = svn_swig_rb_cancel_func;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}

%typemap(argout) (svn_cancel_func_t cancel_func, void *cancel_baton)
{
  svn_swig_rb_set_baton($result, (VALUE)$2);
};
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_wc_conflict_resolver_func_t conflict_func, void *conflict_baton)
{
  $1 = svn_swig_rb_conflict_resolver_func;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}

%typemap(argout) (svn_wc_conflict_resolver_func_t conflict_func, void *conflict_baton)
{
  svn_swig_rb_set_baton($result, (VALUE)$2);
};
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_wc_get_file_t fetch_func, void *fetch_baton)
{
  /* ### TODO(rb support fetch_fun): implement
  $1 = svn_swig_rb_fetch_func;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
  */
  $1 = NULL;
  $2 = NULL;
}

%typemap(argout) (svn_wc_get_file_t fetch_func, void *fetch_baton)
{
  svn_swig_rb_set_baton($result, (VALUE)$2);
};
#endif

/* -----------------------------------------------------------------------
   Callback: svn_info_receiver_t
*/

#ifdef SWIGPERL
%typemap(in) (svn_info_receiver_t receiver, void *receiver_baton)
{
  $1 = svn_swig_pl_info_receiver;
  $2 = (void *)$input;
}
#endif

#ifdef SWIGRUBY
%typemap(in) (svn_info_receiver_t receiver, void *receiver_baton)
{
  $1 = svn_swig_rb_info_receiver;
  $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
}
#endif

/* -----------------------------------------------------------------------
   Callback: svn_fs_warning_callback_t
*/

#ifdef SWIGRUBY
%typemap(in) (svn_fs_warning_callback_t warning, void *warning_baton)
{
  VALUE baton = svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
  svn_swig_rb_fs_warning_callback_baton_register(baton, _global_pool);
  $1 = svn_swig_rb_fs_warning_callback;
  $2 = (void *)baton;
}
#endif
#ifdef SWIGPERL
%ignore svn_fs_set_warning_func;
#endif
#ifdef SWIGPYTHON
%ignore svn_fs_set_warning_func;
#endif

/* -----------------------------------------------------------------------
   svn_stream_t interoperability with language native io handles
*/

#ifdef SWIGPYTHON
%typemap(in) svn_stream_t *WRAPPED_STREAM {
    $1 = svn_swig_py_make_stream ($input, _global_pool);
}
#endif

#ifdef SWIGPERL
%typemap(in) svn_stream_t * {
    svn_swig_pl_make_stream (&$1, $input);
}

%typemap(out) svn_stream_t * {
    $result = svn_swig_pl_from_stream ($1);
    argvi++;
}

%typemap(argout) svn_stream_t ** {
  %append_output(svn_swig_pl_from_stream(*$1));
}
#endif

#ifdef SWIGRUBY
%typemap(in) svn_stream_t * {
  $1 = svn_swig_rb_make_stream($input);
}

%typemap(in) svn_stream_t *MAY_BE_NULL {
  if (NIL_P($input)) {
    $1 = NULL;
  } else {
    $1 = svn_swig_rb_make_stream($input);
  }
}
#endif

/* -----------------------------------------------------------------------
   Mapper to automatically turn Python objects into void* batons on assignment
*/

#ifdef SWIGPYTHON

%typemap(in) void *PY_AS_VOID {
  if ($input == Py_None) {
    $1 = NULL;
  } else if (SWIG_ConvertPtr($input, (void **) &$1, 0, 0) == -1) {
    $1 = (void *) $input;
    PyErr_Clear();
  }
}


%typemap(out) void *PY_AS_VOID {
  PyObject *ownerObj = obj0;
  PyObject *members = PyObject_GetAttrString(ownerObj, "_members");

  $result = NULL;
  if (members != NULL) {
    $result = PyDict_GetItemString(members, "$1_name");
    Py_XINCREF($result);
    Py_DECREF(members);
  }

  if ($result == NULL) {
    if ($1 == NULL) {
      $result = Py_None;
      Py_INCREF($result);
    } else {
      /* We don't know the type of this reference, so we'll have to
       * treat it as an opaque void pointer.
       */
      $result = svn_swig_NewPointerObj($1, $descriptor,
        _global_py_pool, args);
    }
  }

}

#endif

/* -----------------------------------------------------------------------
   MD5 Digest Parameters: in the Subversion C API, there are:
   - 'char' digests, which are hexadecimal-encoded strings.
   - 'unsigned char' digests, which are raw blocks of bytes.
   This section of typemaps relates to the second variety.
   The number of subtly different forms in which these appear in our
   APIs is absolutely scary, and we desperately must introduce some sanity
   when we do Subversion 2.0.

   Function Parameters (Category 1):

   'unsigned char digest[]'
   'unsigned char *digest':

     Output parameter in: svn_fs_file_md5_checksum(), svn_io_file_checksum(),
                          svn_wc_transmit_text_deltas2()

     *but* appears as an input parameter in svn_base64_from_md5() -
     practically, this means that svn_base64_from_md5 is in error, and
     should be using 'const unsigned char digest[]', but fortunately, we
     do not wrap the svn_base64_* APIs, so do not have to care here.


   (Category 2):

   There is no category 2!


   Function Parameters (No typemaps):

   'const unsigned char **read_digest' (also, 'write_digest'):
     These are *delayed* output parameters in svn_stream_checksummed().
     Would be quite complex to wrap, so this function is %ignore-d in the
     svn_io.h cherrypicking section.

   'unsigned char *result_digest'
     This parameter serves as a buffer for the digest results of
     svn_txdelta_apply. As you supply data to this function via
     the handler callback, this digest gets filled. Since
     filling of this buffer is delayed, it is quite difficult
     to wrap.

    FIXME: Both Ruby and Perl have broken typemaps for result_digest.
    The Python bindings ignore this parameter altogether.

   Function Parameters (Category 3):

   'const unsigned char digest[]' (also, 'd1', 'd2'):
   'const unsigned char *digest':
     Input parameters in:
     svn_md5_digest_to_cstring_display()
     svn_md5_digest_to_cstring()
     svn_md5_digests_match()

   Function return values (Category 4):

   'const unsigned char *' in svn_txdelta_md5_digest()
*/

/* Category 1 */

%typemap(in, numinputs=0) unsigned char digest[ANY]
  ($*1_type temp[APR_MD5_DIGESTSIZE]) "$1 = temp;";

#ifdef SWIGPYTHON
%typemap(argout) unsigned char digest[ANY]
{
  %append_output(PyString_FromStringAndSize((char *)$1, APR_MD5_DIGESTSIZE));
}
#endif

#ifdef SWIGPERL
%typemap(argout) unsigned char digest[ANY] {
  %append_output(sv_2mortal(newSVpv(svn_md5_digest_to_cstring($1,
                                                              _global_pool),
                                    0)));
}
#endif
#ifdef SWIGRUBY
%typemap(argout) unsigned char digest[ANY] {
  char *digest_string = (char *)svn_md5_digest_to_cstring($1, _global_pool);
  %append_output(rb_str_new2(digest_string));
}
#endif

%apply unsigned char digest[ANY] { unsigned char *digest };

#ifdef SWIGRUBY
/*
 * Skip the md5sum
 * FIXME: Wrap the md5sum
 */
%typemap(in, numinputs=0) unsigned char *result_digest
  "$1 = NULL;";
#endif

#ifdef SWIGPYTHON
/*
 * Skip the md5sum
 * FIXME: Wrap the md5sum
 */
%typemap(in, numinputs=0) unsigned char *result_digest
  ($*1_type temp[APR_MD5_DIGESTSIZE]) "$1 = NULL;";
#endif

#ifdef SWIGPERL
%typemap(in, numinputs=0) unsigned char *result_digest {
    $1 = (unsigned char *)apr_palloc(_global_pool, APR_MD5_DIGESTSIZE);
}

%typemap(argout) unsigned char *result_digest {
  /* FIXME: This code is clearly buggy. The return value of sv_newmortal()
     is immediately overwritten by the return value
     of svn_swig_pl_from_md5(). */
    ST(argvi) = sv_newmortal();
    ST(argvi++) = svn_swig_pl_from_md5($1);
}
#endif

/* Category 3 */

/* Override the non-const typemaps defined for category 1, which would
   otherwise apply by default, and contribute nonsensical wrappings. */
%typemap(in)     const unsigned char digest[], const unsigned char *digest "";
%typemap(argout) const unsigned char digest[], const unsigned char *digest "";

/* FIXME: There is a lot of coverage missing here */

#ifdef SWIGPERL
%typemap(in) const unsigned char digest[] {
    SWIG_ConvertPtr($input, (void **)&$1, $1_descriptor, 0);
}
#endif

#ifdef SWIGPYTHON
%typemap(in) const unsigned char *digest {
    if ($input == Py_None) {
        $1 = NULL;
    } else {
        $1 = (unsigned char *) PyString_AsString($input);
        if ($1 == NULL) SWIG_fail;
    }
}
#endif

#ifdef SWIGRUBY
%typemap(in) const unsigned char *digest
{
  if (NIL_P($input)) {
    $1 = NULL;
  } else if (RSTRING_LEN($input) != APR_MD5_DIGESTSIZE) {
    rb_raise(rb_eArgError, "digest size (%d) must be %d",
             RSTRING_LEN($input), APR_MD5_DIGESTSIZE);
  } else {
    $1 = ($1_ltype)StringValuePtr($input);
  }
}
#endif

/* Category 4 */
/* FIXME: Write a typemap for category 4 */

/* -----------------------------------------------------------------------
  useful convertors for svn_opt_revision_t
*/
#ifdef SWIGPERL
%typemap(in) svn_opt_revision_t * (svn_opt_revision_t rev) {
    $1 = &rev;
    if ($input == NULL || $input == &PL_sv_undef || !SvOK($input)) {
        rev.kind = svn_opt_revision_unspecified;
    }
    else if (sv_isobject($input) && sv_derived_from($input, "_p_svn_opt_revision_t")) {
        SWIG_ConvertPtr($input, (void **)&$1, $1_descriptor, 0);
    }
    else if (looks_like_number($input)) {
        rev.kind = svn_opt_revision_number;
        rev.value.number = SvIV($input);
    }
    else if (SvPOK($input)) {
        char *input = SvPV_nolen($input);
        if (svn_cstring_casecmp(input, "BASE") == 0)
            rev.kind = svn_opt_revision_base;
        else if (svn_cstring_casecmp(input, "HEAD") == 0)
            rev.kind = svn_opt_revision_head;
        else if (svn_cstring_casecmp(input, "WORKING") == 0)
            rev.kind = svn_opt_revision_working;
        else if (svn_cstring_casecmp(input, "COMMITTED") == 0)
            rev.kind = svn_opt_revision_committed;
        else if (svn_cstring_casecmp(input, "PREV") == 0)
            rev.kind = svn_opt_revision_previous;
        else if (*input == '{') {
            svn_boolean_t matched;
            apr_time_t tm;
            svn_error_t *err;

            char *end = strchr(input,'}');
            if (!end)
                SWIG_croak("unknown opt_revision_t type");
            *end = '\0';
            err = svn_parse_date (&matched, &tm, input + 1, apr_time_now(),
                                  svn_swig_pl_make_pool ((SV *)NULL));
            if (err) {
                svn_error_clear (err);
                SWIG_croak("unknown opt_revision_t type");
            }
            if (!matched)
                SWIG_croak("unknown opt_revision_t type");

            rev.kind = svn_opt_revision_date;
            rev.value.date = tm;
        } else
            SWIG_croak("unknown opt_revision_t type");
    } else
        SWIG_croak("unknown opt_revision_t type");
}
#endif

#ifdef SWIGRUBY
%typemap(in) svn_opt_revision_t * (svn_opt_revision_t rev) {
  $1 = &rev;
  svn_swig_rb_set_revision(&rev, $input);
}
#endif

/* -----------------------------------------------------------------------
   useful convertors for svn_depth_t
*/

#ifdef SWIGRUBY
%typemap(in) svn_depth_t {
  $1 = svn_swig_rb_to_depth($input);
}
#endif


/* -----------------------------------------------------------------------
   useful convertors for svn_mergeinfo_inheritance_t
*/

#ifdef SWIGRUBY
%typemap(in) svn_mergeinfo_inheritance_t {
  $1 = svn_swig_rb_to_mergeinfo_inheritance($input);
}
#endif


/* -----------------------------------------------------------------------
   useful convertors for svn_merge_range_inheritance_t
*/

#ifdef SWIGRUBY
%typemap(in) svn_merge_range_inheritance_t {
  $1 = svn_swig_rb_to_merge_range_inheritance($input);
}
#endif


/* -----------------------------------------------------------------------
   Special boolean mapping for ruby.
*/

#ifdef SWIGRUBY
%typemap(in) svn_boolean_t "$1 = RTEST($input);";
%typemap(out) svn_boolean_t "$result = $1 ? Qtrue : Qfalse;";

%typemap(in, numinputs=0) svn_boolean_t * (svn_boolean_t temp) "$1 = &temp;";
%typemap(argout) svn_boolean_t * {
  %append_output(*$1 ? Qtrue : Qfalse);
}
#endif

/* -----------------------------------------------------------------------
   Filename to temporary file mapping for ruby.
*/

#ifdef SWIGRUBY
%typemap(argout) const char **TO_TEMP_FILE
{
  %append_output(svn_swig_rb_filename_to_temp_file(*$1));
}
#endif

/* -----------------------------------------------------------------------
   Handle python thread locking.

   Swig doesn't allow us to specify a language in the %exception command,
   so we have to use #ifdefs for the python-specific parts.
*/

%exception {
#ifdef SWIGPYTHON
    svn_swig_py_release_py_lock();
#endif
    $action
#ifdef SWIGPYTHON
    svn_swig_py_acquire_py_lock();
#endif
}
