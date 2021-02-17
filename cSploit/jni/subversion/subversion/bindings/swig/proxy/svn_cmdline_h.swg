/* Proxy classes for svn_cmdline.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *cancel_baton,
  void *baton
};
#endif

/* Includes */
%{
#include "svn_cmdline.h"
%}
%include svn_cmdline.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_cmdline_prompt_baton2_t);
%proxy(svn_cmdline_prompt_baton_t);
#endif

/* Callbacks */

%inline %{
%}

#ifdef SWIGPYTHON

#endif
