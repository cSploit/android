#    Licensed to the Apache Software Foundation (ASF) under one
#    or more contributor license agreements.  See the NOTICE file
#    distributed with this work for additional information
#    regarding copyright ownership.  The ASF licenses this file
#    to you under the Apache License, Version 2.0 (the
#    "License"); you may not use this file except in compliance
#    with the License.  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#    Unless required by applicable law or agreed to in writing,
#    software distributed under the License is distributed on an
#    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
#    KIND, either express or implied.  See the License for the
#    specific language governing permissions and limitations
#    under the License.


from ctypes import *
import functions
from functions import *
from tempfile import TemporaryFile
import sys

# Constants which are defined in the Subversion headers, but aren't
# picked up by ctypesgen
TRUE = 1
FALSE = 0
SVN_DIRENT_ALL = -1
SVN_INVALID_REVNUM = -1
SVN_IGNORED_REVNUM = -1
SVN_INVALID_FILESIZE = -1
SVN_RA_SVN_UNSPECIFIED_NUMBER = -1

# Convert the standard files
stdout = PyFile_AsFile(sys.stdout)
stderr = PyFile_AsFile(sys.stderr)
stdin = PyFile_AsFile(sys.stdin)

def svn_pool_create(pool):
    return svn_pool_create_ex(pool, NULL)

def svn_pool_destroy(pool):
    return apr_pool_destroy(pool)

def svn_pool_clear(pool):
    return apr_pool_clear(pool)

def _mark_weakpool_invalid(weakpool):
  if weakpool and weakpool() and hasattr(weakpool(), "_is_valid"):
    del weakpool()._is_valid

class Pool(object):
  def __init__(self, parent_pool=None):
    """Create a new memory pool"""
    self._parent_pool = parent_pool
    self._as_parameter_ = svn_pool_create(self._parent_pool)
    self._mark_valid()

    # Protect important functions from GC
    self._svn_pool_destroy = apr_pool_destroy

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
    svn_pool_clear(self)
    self._mark_valid()

  def destroy(self):
    """Destroy embedded memory pool. If you do not destroy
    the memory pool manually, Python will destroy it
    automatically."""

    self.assert_valid()

    # Destroy pool
    self._svn_pool_destroy(self)

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
      weakparent = weakref.ref(self._parent_pool._is_valid,
        lambda x: _mark_weakpool_invalid(weakself))

    # Mark pool as valid
    self._is_valid = lambda: 1


# Special case for Subversion 1.5 and earlier: Make sure that platform-
# specific functions are available, even if the headers were generated on a
# different platform.

for f in ('svn_auth_get_windows_simple_provider',
          'svn_auth_get_windows_ssl_server_trust_provider',
          'svn_auth_get_keychain_simple_provider'):

    if hasattr(functions._libs['svn_subr-1'], f) and f not in locals():
        f = getattr(functions._libs['svn_subr-1'], f)
        f.restype = None
        f.argtypes = [POINTER(POINTER(svn_auth_provider_object_t)), POINTER(apr_pool_t)]

