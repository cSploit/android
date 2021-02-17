#
# fs.py: public Python interface for fs components
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

from libsvn.fs import *
from svn.core import _unprefix_names, Pool
_unprefix_names(locals(), 'svn_fs_')
_unprefix_names(locals(), 'SVN_FS_')
del _unprefix_names


# Names that are not to be exported
import sys as _sys, os as _os, tempfile as _tempfile, subprocess as _subprocess
try:
  # Python >=3.0
  import builtins
except ImportError:
  # Python <3.0
  import __builtin__ as builtins
import svn.core as _svncore


def entries(root, path, pool=None):
  "Call dir_entries returning a dictionary mappings names to IDs."
  e = dir_entries(root, path, pool)
  for name, entry in e.items():
    e[name] = dirent_t_id_get(entry)
  return e


class FileDiff:
  def __init__(self, root1, path1, root2, path2, pool=None, diffoptions=[]):
    assert path1 or path2

    self.tempfile1 = None
    self.tempfile2 = None

    self.root1 = root1
    self.path1 = path1
    self.root2 = root2
    self.path2 = path2
    self.diffoptions = diffoptions

  def either_binary(self):
    "Return true if either of the files are binary."
    if self.path1 is not None:
      prop = node_prop(self.root1, self.path1, _svncore.SVN_PROP_MIME_TYPE)
      if prop and _svncore.svn_mime_type_is_binary(prop):
        return 1
    if self.path2 is not None:
      prop = node_prop(self.root2, self.path2, _svncore.SVN_PROP_MIME_TYPE)
      if prop and _svncore.svn_mime_type_is_binary(prop):
        return 1
    return 0

  def _dump_contents(self, file, root, path, pool=None):
    fp = builtins.open(file, 'w+') # avoid namespace clash with
                                   # trimmed-down svn_fs_open()
    if path is not None:
      stream = file_contents(root, path, pool)
      try:
        while True:
          chunk = _svncore.svn_stream_read(stream, _svncore.SVN_STREAM_CHUNK_SIZE)
          if not chunk:
            break
          fp.write(chunk)
      finally:
        _svncore.svn_stream_close(stream)
    fp.close()


  def get_files(self):
    if self.tempfile1:
      # no need to do more. we ran this already.
      return self.tempfile1, self.tempfile2

    # Make tempfiles, and dump the file contents into those tempfiles.
    self.tempfile1 = _tempfile.mktemp()
    self.tempfile2 = _tempfile.mktemp()

    self._dump_contents(self.tempfile1, self.root1, self.path1)
    self._dump_contents(self.tempfile2, self.root2, self.path2)

    return self.tempfile1, self.tempfile2

  def get_pipe(self):
    self.get_files()

    # use an array for the command to avoid the shell and potential
    # security exposures
    cmd = ["diff"] \
          + self.diffoptions \
          + [self.tempfile1, self.tempfile2]

    # open the pipe, and return the file object for reading from the child.
    p = _subprocess.Popen(cmd, stdout=_subprocess.PIPE,
                          close_fds=_sys.platform != "win32")
    return p.stdout

  def __del__(self):
    # it seems that sometimes the files are deleted, so just ignore any
    # failures trying to remove them
    if self.tempfile1 is not None:
      try:
        _os.remove(self.tempfile1)
      except OSError:
        pass
    if self.tempfile2 is not None:
      try:
        _os.remove(self.tempfile2)
      except OSError:
        pass
