/* Proxy classes for svn_iter.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_iter_apr_hash_cb_t,
  svn_iter_apr_array_cb_t
};
%apply CALLABLE_CALLBACK * {
  svn_iter_apr_hash_cb_t *,
  svn_iter_apr_array_cb_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton
};
#endif

/* Includes */
%{
#include "svn_iter.h"
%}
%include svn_iter.h

/* Structure definitions */
#ifdef SWIGPYTHON
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_iter_invoke_apr_hash_cb(
  svn_iter_apr_hash_cb_t _obj, void *baton, const void *key, apr_ssize_t klen, void *val, apr_pool_t *pool) {
  return _obj(baton, key, klen, val, pool);
}

static svn_error_t * svn_iter_invoke_apr_array_cb(
  svn_iter_apr_array_cb_t _obj, void *baton, void *item, apr_pool_t *pool) {
  return _obj(baton, item, pool);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_iter_apr_hash_cb_t, svn_iter_invoke_apr_hash_cb);
%funcptr_proxy(svn_iter_apr_array_cb_t, svn_iter_invoke_apr_array_cb);

#endif
