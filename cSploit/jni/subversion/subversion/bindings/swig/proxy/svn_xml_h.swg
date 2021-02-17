/* Proxy classes for svn_xml.h
 * DO NOT EDIT -- AUTOMATICALLY GENERATED */

/* No default constructors for opaque structs */
#ifdef SWIGPYTHON
%nodefault svn_xml_parser_t;
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
#include "svn_xml.h"
%}
%include svn_xml.h

/* Structure definitions */
#ifdef SWIGPYTHON
%opaque_proxy(svn_xml_parser_t);
#endif

/* Callbacks */

%inline %{
%}

#ifdef SWIGPYTHON

#endif
