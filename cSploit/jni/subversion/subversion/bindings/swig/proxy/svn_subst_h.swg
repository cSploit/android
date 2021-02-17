/* Proxy classes for svn_subst.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *cancel_baton
};
#endif

/* Includes */
%{
#include "svn_subst.h"
%}
%include svn_subst.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_subst_keywords_t);
#endif

/* Callbacks */

%inline %{
%}

#ifdef SWIGPYTHON

#endif
