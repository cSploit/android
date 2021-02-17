/* Proxy classes for svn_version.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */
#ifdef SWIGPYTHON
%apply CALLABLE_CALLBACK {
  svn_version_func_t
};
%apply CALLABLE_CALLBACK * {
  svn_version_func_t *
};
#endif

/* Baton typemaps */

/* Includes */
%{
#include "svn_version.h"
%}
%include svn_version.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_version_checklist_t);
#endif

/* Callbacks */

%inline %{
static const svn_version_t * svn_version_checklist_invoke_version_query(
  svn_version_checklist_t * _obj) {
  return (_obj->version_query)();
}

static const svn_version_t * svn_version_invoke_func(
  svn_version_func_t _obj) {
  return _obj();
}

%}

#ifdef SWIGPYTHON
%funcptr_member_proxy(svn_version_checklist_t, version_query, svn_version_checklist_invoke_version_query);
%funcptr_proxy(svn_version_func_t, svn_version_invoke_func);

#endif
