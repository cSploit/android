#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#
import os.path, sys, tempfile, urllib
from svn import core, repos
from StringIO import StringIO

class Temper(object):
  """Class to simplify allocation and cleanup of dummy Subversion
     structures, such as repositories and working copies."""

  def __init__(self):
    self._cleanup_list = []

  def __del__(self):
    self.cleanup()

  def cleanup(self):
    """Destroy everything that was allocated so far."""
    for (target, clean_func) in self._cleanup_list:
      clean_func(target)
    self._cleanup_list = []

  def alloc_empty_dir(self, suffix = ""):
    """Create an empty temporary directory. Returns its full path
       in canonical internal form."""
    temp_dir_name = core.svn_dirent_internal_style(tempfile.mkdtemp(suffix))
    self._cleanup_list.append((temp_dir_name, core.svn_io_remove_dir))
    return temp_dir_name

  def alloc_empty_repo(self, suffix = ""):
    """Create an empty repository. Returns a tuple of its handle, path and
       file: URI in canonical internal form."""
    temp_path = tempfile.mkdtemp(suffix)
    repo_path = core.svn_dirent_internal_style(temp_path)
    repo_uri = core.svn_uri_canonicalize(file_uri_for_path(temp_path))
    handle = repos.create(repo_path, None, None, None, None)
    self._cleanup_list.append((repo_path, repos.svn_repos_delete))
    return (handle, repo_path, repo_uri)

  def alloc_known_repo(self, repo_id, suffix = ""):
    """Create a temporary repository and fill it with the contents of the
       specified dump. repo_id is the path to the dump, relative to the script's
       location. Returns the same as alloc_empty_repo."""
    dump_path = os.path.join(os.path.dirname(sys.argv[0]), repo_id)
    (handle, repo_path, repo_uri) = self.alloc_empty_repo(suffix=suffix)
    repos.svn_repos_load_fs2(handle, open(dump_path, 'rb'), StringIO(),
                             repos.load_uuid_default, None, False, False, None)
    return (handle, repo_path, repo_uri)

def file_uri_for_path(path):
  """Return the file: URI corresponding to the given path."""
  uri_path = urllib.pathname2url(path)

  # pathname2url claims to return the path part of the URI, but on Windows
  # it returns both the authority and path parts for no reason, which
  # means we have to trim the leading slashes to "normalize" the result.
  return 'file:///' + uri_path.lstrip('/')
