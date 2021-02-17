/* Proxy classes for svn_io.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_stream_t;
%nodefault svn_stream_mark_t;
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_read_fn_t,
  svn_stream_skip_fn_t,
  svn_write_fn_t,
  svn_close_fn_t,
  svn_stream_mark_fn_t,
  svn_stream_seek_fn_t,
  svn_io_walk_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_read_fn_t *,
  svn_stream_skip_fn_t *,
  svn_write_fn_t *,
  svn_close_fn_t *,
  svn_stream_mark_fn_t *,
  svn_stream_seek_fn_t *,
  svn_io_walk_func_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *cancel_baton,
  void *baton,
  void *walk_baton
};
#endif

/* Includes */
%{
#include "svn_io.h"
%}
%include svn_io.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_io_dirent2_t);
%proxy(svn_io_dirent_t);
%opaque_proxy(svn_stream_t);
%opaque_proxy(svn_stream_mark_t);
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_read_invoke_fn(
  svn_read_fn_t _obj, void *baton, char *buffer, apr_size_t *len) {
  return _obj(baton, buffer, len);
}

static svn_error_t * svn_stream_invoke_skip_fn(
  svn_stream_skip_fn_t _obj, void *baton, apr_size_t len) {
  return _obj(baton, len);
}

static svn_error_t * svn_write_invoke_fn(
  svn_write_fn_t _obj, void *baton, const char *data, apr_size_t *len) {
  return _obj(baton, data, len);
}

static svn_error_t * svn_close_invoke_fn(
  svn_close_fn_t _obj, void *baton) {
  return _obj(baton);
}

static svn_error_t * svn_stream_invoke_mark_fn(
  svn_stream_mark_fn_t _obj, void *baton, svn_stream_mark_t **mark, apr_pool_t *pool) {
  return _obj(baton, mark, pool);
}

static svn_error_t * svn_stream_invoke_seek_fn(
  svn_stream_seek_fn_t _obj, void *baton, const svn_stream_mark_t *mark) {
  return _obj(baton, mark);
}

static svn_error_t * svn_io_invoke_walk_func(
  svn_io_walk_func_t _obj, void *baton, const char *path, const apr_finfo_t *finfo, apr_pool_t *pool) {
  return _obj(baton, path, finfo, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_read_fn_t, svn_read_invoke_fn);
%funcptr_proxy(svn_stream_skip_fn_t, svn_stream_invoke_skip_fn);
%funcptr_proxy(svn_write_fn_t, svn_write_invoke_fn);
%funcptr_proxy(svn_close_fn_t, svn_close_invoke_fn);
%funcptr_proxy(svn_stream_mark_fn_t, svn_stream_invoke_mark_fn);
%funcptr_proxy(svn_stream_seek_fn_t, svn_stream_invoke_seek_fn);
%funcptr_proxy(svn_io_walk_func_t, svn_io_invoke_walk_func);

#endif
