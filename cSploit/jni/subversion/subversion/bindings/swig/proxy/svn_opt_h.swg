/* Proxy classes for svn_opt.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
#endif

/* Callback typemaps */

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton
};
#endif

/* Includes */
%{
#include "svn_opt.h"
%}
%include svn_opt.h

/* Structure definitions */
#ifdef SWIGPYTHON
%proxy(svn_opt_subcommand_desc2_t);
%proxy(svn_opt_subcommand_desc_t);
%proxy(svn_opt_revision_value_t);
%proxy(svn_opt_revision_t);
%proxy(svn_opt_revision_range_t);
#endif

/* Callbacks */

%inline %{
%}

#ifdef SWIGPYTHON

#endif
