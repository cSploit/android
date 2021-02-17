/* Proxy classes for svn_error.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_error_malfunction_handler_t
};
%apply CALLABLE_CALLBACK * {
  svn_error_malfunction_handler_t *
};
#endif

/* Baton typemaps */

/* Includes */
%{
#include "svn_error.h"
%}
%include svn_error.h

/* Structure definitions */
#ifdef SWIGPYTHON
#endif

/* Callbacks */

%inline %{
static svn_error_t * svn_error_invoke_malfunction_handler(
  svn_error_malfunction_handler_t _obj, svn_boolean_t can_return, const char *file, int line, const char *expr) {
  return _obj(can_return, file, line, expr);
}

%}

#ifdef SWIGPYTHON
%funcptr_proxy(svn_error_malfunction_handler_t, svn_error_invoke_malfunction_handler);

#endif
