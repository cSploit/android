/*
 * proxy.swg :  SWIG include file for defining automatic proxy classes
 *
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 */

#ifdef SWIGPYTHON

/* Note: See the big comment at the top of proxy_apr.swg for details on
 *       how this _is_valid stuff actually works.  It's part of the magic
 *       that lets us gracefully handle objects that are allocated from
 *       a pool that's been cleared or destroyed.
 */
 
%pythoncode %{
  def _copy_metadata_deep(value, old_value):
    """Copy all attributes of old_value into value, recursively traversing
    lists and dicts if needed."""
    if value is None or old_value is None or value is old_value: return
    
    if isinstance(value, dict):
      for k, v in value.iteritems():
        _copy_metadata_deep(v, old_value[k])
    elif isinstance(value, list):
      for v, old_v in zip(value, old_value):
        _copy_metadata_deep(v, old_v)
    else:
      try:
        value.__dict__.update(old_value.__dict__)
      except AttributeError:
        pass
        
  def _assert_valid_deep(value):
    """Assert value's validity, recursively traversing lists and dicts."""
    if isinstance(value, dict):
      for v in value.itervalues():
        _assert_valid_deep(v)
    elif isinstance(value, list):
      for v in value:
        _assert_valid_deep(v)
    else:
      if hasattr(value, "assert_valid"):
        value.assert_valid()
%}

/* Default code for all wrapped proxy classes in Python */
%define %proxy_pythoncode(TYPE)
%pythoncode {
  def set_parent_pool(self, parent_pool=None):
    """Create a new proxy object for TYPE"""
    import libsvn.core, weakref
    self.__dict__["_parent_pool"] = \
      parent_pool or libsvn.core.application_pool;
    if self.__dict__["_parent_pool"]:
      self.__dict__["_is_valid"] = weakref.ref(
        self.__dict__["_parent_pool"]._is_valid)

  def assert_valid(self):
    """Assert that this object is using valid pool memory"""
    if "_is_valid" in self.__dict__:
      assert self.__dict__["_is_valid"](), "Variable has already been deleted"

  def __getattr__(self, name):
    """Get an attribute from this object"""
    self.assert_valid()

    value = _swig_getattr(self, self.__class__, name)

    # If we got back a different object than we have, we need to copy all our
    # metadata into it, so that it looks identical
    members = self.__dict__.get("_members")
    if members is not None:
      _copy_metadata_deep(value, members.get(name))
        
    # Verify that the new object is good
    _assert_valid_deep(value)

    return value

  def __setattr__(self, name, value):
    """Set an attribute on this object"""
    self.assert_valid()

    # Save a copy of the object, so that the garbage
    # collector won't kill the object while it's in
    # SWIG-land
    self.__dict__.setdefault("_members",{})[name] = value

    return _swig_setattr(self, self.__class__, name, value)
}
%enddef

/* Define a proxy for wrapping an existing struct */
%define %proxy(TYPE)
%extend TYPE {
%proxy_pythoncode(TYPE);
}
%enddef

/* Define a proxy class for wrapping an opaque struct */
%define %opaque_proxy(TYPE)
struct TYPE {
%extend {
%proxy_pythoncode(TYPE);
}
}
%enddef

/* Treat typemapped function pointers as objects, which have a bound
 * __call__ method. This mapping depends on the function pointer being
 * typemapped as a CALLABLE_CALLBACK. */
%define %funcptr_proxy(TYPE, INVOKE)
%nodefault TYPE;
struct TYPE {
%extend {
%proxy_pythoncode(TYPE);
%pythoncode %{
  def __call__(self, *args):
    return INVOKE(self, *args)
%}
}
};

%enddef

/* Add member functions to objects so that their function pointers can
 * be invoked.
 *
 * Unlike the CALLABLE_CALLBACKS, these member functions don't have or
 * need typemaps, because the underlying C/SWIG object for the callback
 * is hidden.
 */
%define %funcptr_member_proxy(TYPE, MEMBER, INVOKE)
%extend TYPE {
%pythoncode %{
  def MEMBER(self, *args):
    return INVOKE(self, *args)
%}
}
%enddef


#endif
