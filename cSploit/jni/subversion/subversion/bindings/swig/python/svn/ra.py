#
# ra.py: public Python interface for ra components
#
# Subversion is a tool for revision control.
# See http://subversion.apache.org for more information.
#
######################################################################
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
######################################################################

from libsvn.ra import *
from svn.core import _unprefix_names
_unprefix_names(locals(), 'svn_ra_')
_unprefix_names(locals(), 'SVN_RA_')
del _unprefix_names

class Callbacks:
  """Base class for callbacks structure for svn.ra.open2.

  Ra users may pass an instance of this class as is to svn.ra.open2
  for some simple operations: as long as authentication is not
  required, auth_baton may be None, and some ra implementations do not
  use open_tmp_file at all.  These are not guarantees, however, and
  all but the simplest scripts should fill even these in.

  The wc_prop slots, on the other hand, are only necessary for commits
  and updates, and progress_func and cancel_func are always optional.

  A simple example:

  class Callbacks(svn.ra.Callbacks):
    def __init__(self, wc, username, password):
      self.wc = wc
      self.auth_baton = svn.core.svn_auth_open([
          svn.client.get_simple_provider(),
          svn.client.get_username_provider(),
          ])
      svn.core.svn_auth_set_parameter(self.auth_baton,
                                      svn.core.SVN_AUTH_PARAM_DEFAULT_USERNAME,
                                      username)
      svn.core.svn_auth_set_parameter(self.auth_baton,
                                      svn.core.SVN_AUTH_PARAM_DEFAULT_PASSWORD,
                                      password)
    def open_tmp_file(self, pool):
      path = '/'.join([self.wc, svn.wc.get_adm_dir(pool), 'tmp'])
      (fd, fn) = tempfile.mkstemp(dir=path)
      os.close(fd)
      return fn
    def cancel_func(self):
      if some_condition():
        return svn.core.SVN_ERR_CANCELLED
      return 0
  """
  open_tmp_file = None
  auth_baton = None
  get_wc_prop = None
  set_wc_prop = None
  push_wc_prop = None
  invalidate_wc_props = None
  progress_func = None
  cancel_func = None
  get_client_string = None
