/* Proxy classes for svn_ra_svn.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_ra_svn_conn_st;
#endif

/* Callback typemaps */

/* Baton typemaps */
#ifdef SWIGPYTHON
%apply void *PY_AS_VOID {
  void *baton,
  void *callback_baton,
  void *edit_baton
};
#endif

/* Includes */
%{
#include "svn_ra_svn.h"
%}
%include svn_ra_svn.h

/* Structure definitions */
#ifdef SWIGPYTHON
%opaque_proxy(svn_ra_svn_conn_st);
%proxy(svn_ra_svn_cmd_entry_t);
%proxy(svn_ra_svn_item_t);
#endif

/* Callbacks */

%inline %{
%}

#ifdef SWIGPYTHON

#endif
