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
 * svn_delta.i: SWIG interface file for svn_delta.h
 */

#if defined(SWIGPYTHON)
%module(package="libsvn") delta
#elif defined(SWIGPERL)
%module "SVN::_Delta"
#elif defined(SWIGRUBY)
%module "svn::ext::delta"
#endif

%include svn_global.swg
%import core.i

#ifdef SWIGRUBY
%ignore svn_compat_wrap_file_rev_handler;
#endif

/* -----------------------------------------------------------------------
   %apply-ing of typemaps defined elsewhere
*/

%apply const char *MAY_BE_NULL {
    const char *error_info,
    const char *copyfrom_path,
    const char *copy_path,
    const char *base_checksum,
    const char *text_checksum
};

#ifdef SWIGPYTHON
%apply svn_stream_t *WRAPPED_STREAM { svn_stream_t * };
#endif

/* -----------------------------------------------------------------------
   mark window.new_data as readonly since we would need a pool to set it
   properly (e.g. to allocate an svn_string_t structure).
*/
%immutable svn_txdelta_window_t::new_data;

/* -----------------------------------------------------------------------
   thunk editors for the various language bindings.
*/

#ifdef SWIGPYTHON
void svn_swig_py_make_editor(const svn_delta_editor_t **editor,
                             void **edit_baton,
                             PyObject *py_editor,
                             apr_pool_t *pool);
#endif

#ifdef SWIGPERL
%typemap(in) (const svn_delta_editor_t *EDITOR, void *BATON) {
    svn_delta_make_editor(&$1, &$2, $input, _global_pool);
}

void svn_delta_wrap_window_handler(svn_txdelta_window_handler_t *handler,
                                   void **handler_baton,
                                   SV *callback,
                                   apr_pool_t *pool);

#endif

#ifdef SWIGRUBY
%typemap(in) (const svn_delta_editor_t *EDITOR, void *BATON)
{
  if (RTEST(rb_obj_is_kind_of($input,
                              svn_swig_rb_svn_delta_editor()))) {
    $1 = svn_swig_rb_to_swig_type($input,
                                  "svn_delta_editor_t *",
                                  _global_pool);
    $2 = svn_swig_rb_to_swig_type(rb_funcall($input, rb_intern("baton"), 0),
                                  "void *", _global_pool);
  } else {
    svn_swig_rb_make_delta_editor(&$1, &$2, $input, _global_pool);
  }
}
#endif

#ifndef SWIGPYTHON
/* Python users have to use svn_swig_py_make_editor manually, which sucks.
   Maybe we could allow people to pass a python object in the editor parameter,
   and None as the baton, and automatically invoke svn_swig_py_make_editor,
   rather than forcing the svn_swig_py_make_editor to be done manually.
   Of course, ideally, the baton parameter would vanish from the python
   side entirely, but we can't kill compatibility like that until 2.0.
*/
%apply (const svn_delta_editor_t *EDITOR, void *BATON)
{
  (const svn_delta_editor_t *editor, void *baton),
  (const svn_delta_editor_t *editor, void *edit_baton),
  (const svn_delta_editor_t *editor, void *file_baton),
  (const svn_delta_editor_t *diff_editor, void *diff_baton),
  (const svn_delta_editor_t *update_editor, void *update_baton),
  (const svn_delta_editor_t *switch_editor, void *switch_baton),
  (const svn_delta_editor_t *status_editor, void *status_baton)
}
#endif

/* -----------------------------------------------------------------------
   handle svn_txdelta_window_handler_t/baton pair.
*/

#ifdef SWIGRUBY
%typemap(in) (svn_txdelta_window_handler_t handler,
                    void *handler_baton)
{
  if (RTEST(rb_obj_is_kind_of($input,
                              svn_swig_rb_svn_delta_text_delta_window_handler()))) {
    $1 = svn_swig_rb_to_swig_type($input,
                                  "svn_txdelta_window_handler_t",
                                  _global_pool);
    $2 = svn_swig_rb_to_swig_type(rb_funcall($input, rb_intern("baton"), 0),
                                  "void *", _global_pool);
  } else {
    $1 = svn_swig_rb_txdelta_window_handler;
    $2 = (void *)svn_swig_rb_make_baton($input, _global_svn_swig_rb_pool);
  }
}
#endif

/* -----------------------------------------------------------------------
   handle svn_delta_path_driver().
*/

#ifndef SWIGPERL
%callback_typemap(svn_delta_path_driver_cb_func_t callback_func,
                  void *callback_baton,
                  svn_swig_py_delta_path_driver_cb_func,
                  ,
                  svn_swig_rb_delta_path_driver_cb_func)
#endif

/* ----------------------------------------------------------------------- */

%{
#include <apr_md5.h>
#include "svn_md5.h"
%}

/* -----------------------------------------------------------------------
   handle svn_txdelta_window_t::ops
*/
#ifdef SWIGRUBY
%ignore svn_txdelta_window_t::ops;
%inline %{
static VALUE
svn_txdelta_window_t_ops_get(svn_txdelta_window_t *window)
{
  return svn_swig_rb_txdelta_window_t_ops_get(window);
}
%}
#endif

#ifdef SWIGPYTHON
%ignore svn_txdelta_window_t::ops;
%inline %{
static PyObject *
svn_txdelta_window_t_ops_get(PyObject *window_ob)
{
  void *window;
  PyObject *ops_list, *window_pool;
  int status;
  
  /* Kludge alert!
     Normally, these kinds of conversions would belong in a typemap.
     However, typemaps won't allow us to change the result type to an array,
     so we have to make this custom accessor function.
     A cleaner approach would be to use something like: 
     
     %extend svn_txdelta_window_t { void get_ops(apr_array_header_t ** ops); }
     
     But that means unnecessary copying, plus more hacks to get the pool for the
     array and for wrapping the individual op objects. So we just don't bother.
  */
  
  /* Note: the standard svn-python typemap releases the GIL while calling the
     wrapped function, but this function does Python stuff, so we have to
     reacquire it again. */
  svn_swig_py_acquire_py_lock();
  status = svn_swig_ConvertPtr(window_ob, &window,
    SWIG_TypeQuery("svn_txdelta_window_t *"));
    
  if (status != 0)
    {
      PyErr_SetString(PyExc_TypeError,
                      "expected an svn_txdelta_window_t* proxy");
      svn_swig_py_release_py_lock();
      return NULL;
    }
    
  window_pool = PyObject_GetAttrString(window_ob, "_parent_pool");

  if (window_pool == NULL)
    {
      svn_swig_py_release_py_lock();
      return NULL;
    }
    
  ops_list = svn_swig_py_txdelta_window_t_ops_get(window,
    SWIG_TypeQuery("svn_txdelta_op_t *"), window_pool);
    
  svn_swig_py_release_py_lock();
  
  return ops_list;
}
%}
#endif

%include svn_delta_h.swg

#ifdef SWIGRUBY
%inline %{
static VALUE
svn_swig_rb_delta_editor_get_target_revision(VALUE editor)
{
  static ID id_target_revision_address = 0;
  VALUE rb_target_address;
  svn_revnum_t *target_address;

  if (id_target_revision_address == 0)
    id_target_revision_address = rb_intern("@target_revision_address");

  if (!RTEST(rb_ivar_defined(editor, id_target_revision_address)))
    return Qnil;

  rb_target_address = rb_ivar_get(editor, id_target_revision_address);
  if (NIL_P(rb_target_address))
    return Qnil;

  target_address = (svn_revnum_t *)(NUM2LONG(rb_target_address));
  if (!target_address)
    return Qnil;

  return LONG2NUM(*target_address);
}
%}
#endif

/* -----------------------------------------------------------------------
   handle svn_txdelta_apply_instructions()
*/
#ifdef SWIGRUBY
%inline %{
static VALUE
svn_swig_rb_txdelta_apply_instructions(svn_txdelta_window_t *window,
                                       const char *sbuf)
{
  char *tbuf;
  apr_size_t tlen;

  tlen = window->tview_len + 1;
  tbuf = ALLOCA_N(char, tlen);
  svn_txdelta_apply_instructions(window, sbuf, tbuf, &tlen);

  return rb_str_new(tbuf, tlen);
}
%}
#endif

/* -----------------------------------------------------------------------
   handle svn_txdelta_to_svndiff().
*/
#ifdef SWIGRUBY
%inline %{
static void
svn_txdelta_apply_wrapper(svn_stream_t *source,
                          svn_stream_t *target,
                          unsigned char *result_digest,
                          const char *error_info,
                          svn_txdelta_window_handler_t *handler,
                          void **handler_baton,
                          apr_pool_t *pool)
{
  svn_txdelta_apply(source, target, result_digest, error_info,
                    pool, handler, handler_baton);
}

static svn_error_t *
svn_delta_editor_invoke_open_root_wrapper(svn_delta_editor_t *editor,
                                          void *edit_baton,
                                          svn_revnum_t base_revision,
                                          void **root_baton,
                                          apr_pool_t *dir_pool)
{
  return svn_delta_editor_invoke_open_root(editor, edit_baton,
                                           base_revision, dir_pool, root_baton);
}

static svn_error_t *
svn_delta_editor_invoke_add_directory_wrapper(svn_delta_editor_t *editor,
                                              const char *path,
                                              void *parent_baton,
                                              const char *copyfrom_path,
                                              svn_revnum_t copyfrom_revision,
                                              void **child_baton,
                                              apr_pool_t *dir_pool)
{
  return svn_delta_editor_invoke_add_directory(editor, path, parent_baton,
                                               copyfrom_path, copyfrom_revision,
                                               dir_pool, child_baton);
}

static svn_error_t *
svn_delta_editor_invoke_open_directory_wrapper(svn_delta_editor_t *editor,
                                               const char *path,
                                               void *parent_baton,
                                               svn_revnum_t base_revision,
                                               void **child_baton,
                                               apr_pool_t *dir_pool)
{
  return svn_delta_editor_invoke_open_directory(editor, path, parent_baton,
                                                base_revision, dir_pool,
                                                child_baton);
}

static svn_error_t *
svn_delta_editor_invoke_add_file_wrapper(svn_delta_editor_t *editor,
                                         const char *path,
                                         void *parent_baton,
                                         const char *copyfrom_path,
                                         svn_revnum_t copyfrom_revision,
                                         void **file_baton,
                                         apr_pool_t *file_pool)
{
  return svn_delta_editor_invoke_add_file(editor, path, parent_baton,
                                          copyfrom_path,
                                          copyfrom_revision,
                                          file_pool, file_baton);
}

static svn_error_t *
svn_delta_editor_invoke_open_file_wrapper(svn_delta_editor_t *editor,
                                          const char *path,
                                          void *parent_baton,
                                          svn_revnum_t base_revision,
                                          void **file_baton,
                                          apr_pool_t *file_pool)
{
  return svn_delta_editor_invoke_open_file(editor, path, parent_baton,
                                           base_revision,
                                           file_pool, file_baton);
}

static svn_error_t *
svn_delta_editor_invoke_apply_textdelta_wrapper(svn_delta_editor_t *editor,
                                                void *file_baton,
                                                const char *base_checksum,
                                                svn_txdelta_window_handler_t *handler,
                                                void **handler_baton,
                                                apr_pool_t *pool)
{
  return svn_delta_editor_invoke_apply_textdelta(editor, file_baton,
                                                 base_checksum, pool,
                                                 handler, handler_baton);
}

static svn_error_t *
svn_txdelta_invoke_window_handler_wrapper(VALUE obj,
                                          svn_txdelta_window_t *window,
                                          apr_pool_t *pool)
{
  return svn_swig_rb_invoke_txdelta_window_handler_wrapper(obj, window, pool);
}

static svn_error_t *
svn_txdelta_editor_invoke_apply_textdelta_wrapper(VALUE obj,
                                                  svn_txdelta_window_t *window,
                                                  apr_pool_t *pool)
{
  return svn_swig_rb_invoke_txdelta_window_handler_wrapper(obj, window, pool);
}

static const char *
svn_txdelta_md5_digest_as_cstring(svn_txdelta_stream_t *stream,
                                  apr_pool_t *pool)
{
  const unsigned char *digest;

  digest = svn_txdelta_md5_digest(stream);

  if (digest) {
    return svn_md5_digest_to_cstring(digest, pool);
  } else {
    return NULL;
  }
}

%}
#endif
