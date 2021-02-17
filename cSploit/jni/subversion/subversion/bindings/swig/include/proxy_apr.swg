/*
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
 *
 * proxy_apr.swg: This file forms part of the core module (it is %included
 *   only in one place, core.i).  It contains Python pool related code.
 */

#ifdef SWIGPYTHON
%nodefault apr_array_header_t;
%nodefault apr_file_t;
%nodefault apr_hash_t;
%nodefault apr_pool_t;

%opaque_proxy(apr_array_header_t);
%opaque_proxy(apr_file_t);
%opaque_proxy(apr_hash_t);

/*
 * SWIG/Python Automatic Memory Management in Subversion: An Overview
 * ------------------------------------------------------------------
 *
 * The python memory management code is designed to mark pools as invalid
 * when their parent pools have been garbage collected.  This is implemented
 * by registering a callback with the Python garbage collector, so that when
 * the object's parent pool is deleted, we can be notified.  For more info on
 * how these callbacks work, read the Python documentation for the
 * weakref.ref() function.
 *
 * Each object has an _is_valid member, and stores a weakref to its parent
 * pool's _is_valid, and when that weakref is broken _mark_weakpool_invalid()
 * gets called in order to mark the object as invalid.
 *
 * You can destroy a pool in three ways:
 *   pool.destroy()
 *   pool.clear()
 *   pool.__del__()
 *
 * Each of the above functions destroys the pool's _is_valid member, setting
 * off a cascade of callback functions that set all the child objects that were
 * created in the pool to invalid.
 *
 * If a SWIG object is created from a memory pool, the Python wrapper should
 * store a full reference to the memory pool and a weakreference to _is_valid.
 * When you try to access the SWIG object, the Python wrapper will check the
 * _is_valid weakref to ensure that the pool has not been destroyed (see
 * proxy.swg to read the implementation details).
 *
 * This lets us gracefully throw an exception if you try to use an object
 * that was allocated out of a pool that was cleared, rather than crashing
 * like we used to do.
 *
 */

%pythoncode %{
import threading

application_pool = None
application_pool_lock = threading.Lock()
class GenericSWIGWrapper:
  def __init__(self, this, pool):
    """Create new Generic SWIG wrapper object"""
    import weakref
    self.this = this
    self._parent_pool = pool
    self._is_valid = weakref.ref(pool._is_valid)

  def set_parent_pool(self, pool):
    """Set the parent pool of this object"""
    self._parent_pool = pool

  def valid(self):
    """Is this object valid?"""
    return self._is_valid()

  def assert_valid(self):
    """Assert that this object is still valid"""
    assert self.valid(), "This object has already been destroyed"

  def _unwrap(self):
    """Return underlying SWIG object"""
    self.assert_valid()
    return self.this

def _mark_weakpool_invalid(weakpool):
  if weakpool and weakpool() and hasattr(weakpool(), "_is_valid"):
    del weakpool()._is_valid

%}

struct apr_pool_t {
  %extend {
    %pythoncode %{
      def set_parent_pool(self, parent_pool=None):
        """Create a new memory pool"""
        global application_pool

        try:
          application_pool_lock.acquire()

          self._parent_pool = parent_pool or application_pool
          self._mark_valid()

          # Protect important functions from GC
          self._apr_pool_destroy = _core.apr_pool_destroy
          self._svn_swig_py_clear_application_pool = \
            _core.svn_swig_py_clear_application_pool

          # If we are an application-level pool,
          # then set this pool to be the application-level pool
          if not self._parent_pool:
            svn_swig_py_set_application_pool(self, self)
            application_pool = self
        finally:
          application_pool_lock.release()

      def valid(self):
        """Check whether this memory pool and its parents
        are still valid"""
        return hasattr(self,"_is_valid")

      def assert_valid(self):
        """Assert that this memory_pool is still valid."""
        assert self.valid(), "This pool has already been destroyed"

      def clear(self):
        """Clear embedded memory pool. Invalidate all subpools."""
        pool = self._parent_pool
        apr_pool_clear(self)
        self.set_parent_pool(pool)

      def destroy(self):
        """Destroy embedded memory pool. If you do not destroy
        the memory pool manually, Python will destroy it
        automatically."""
        global application_pool

        self.assert_valid()

        is_application_pool = not self._parent_pool

        # Destroy pool
        self._apr_pool_destroy(self)

        # Clear application pool if necessary
        if is_application_pool:
          application_pool = None
          self._svn_swig_py_clear_application_pool()

        # Mark self as invalid
        if hasattr(self, "_parent_pool"):
          del self._parent_pool
        if hasattr(self, "_is_valid"):
          del self._is_valid

      def __del__(self):
        """Automatically destroy memory pools, if necessary"""
        if self.valid():
          self.destroy()

      def _mark_valid(self):
        """Mark pool as valid"""

        self._weakparent = None

        if self._parent_pool:
          import weakref

          # Make sure that the parent object is valid
          self._parent_pool.assert_valid()

          # Refer to self using a weakrefrence so that we don't
          # create a reference cycle
          weakself = weakref.ref(self)

          # Set up callbacks to mark pool as invalid when parents
          # are destroyed
          self._weakparent = weakref.ref(self._parent_pool._is_valid,
            lambda x: _mark_weakpool_invalid(weakself))

        # Mark pool as valid
        self._is_valid = lambda: 1

      def _wrap(self, obj):
        """Mark a SWIG object as owned by this pool"""
        self.assert_valid()
        if hasattr(obj, "set_parent_pool"):
          obj.set_parent_pool(self)
          return obj
        elif obj is None:
          return None
        else:
          return GenericSWIGWrapper(obj, self)

    %}
  }
};
%pythoncode %{
# Initialize a global pool
svn_pool_create()
%}
#endif

