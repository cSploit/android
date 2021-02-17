/* Proxy classes for svn_hash.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_hash_diff_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_hash_diff_func_t *
};
#endif

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *diff_func_baton
};
#endif

/* Includes */
%{
#include "svn_hash.h"
%}
%include svn_hash.h

/* Structure definitions */
#ifdef SWIGPYTHON
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_hash_invoke_diff_func(
  svn_hash_diff_func_t _obj, const void *key, apr_ssize_t klen, enum svn_hash_diff_key_status status, void *baton) {
  return _obj(key, klen, status, baton);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_hash_diff_func_t, svn_hash_invoke_diff_func);

#endif
