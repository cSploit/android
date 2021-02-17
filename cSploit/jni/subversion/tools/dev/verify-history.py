#!/usr/bin/env python
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

# This program is used to verify the FS history code.
#
# The basic gist is this: given a repository, a path in that
# repository, and a revision at which to begin plowing through history
# (towards revision 1), verify that each history object returned by
# the svn_fs_history_prev() interface -- indirectly via
# svn_repos_history() -- represents a revision in which the node being
# tracked actually changed, or where a parent directory of the node
# was copied, according to the list of paths changed as reported by
# svn_fs_paths_changed().
#
# A fun way to run this:
#
#   #!/bin/sh
#
#   export VERIFY=/path/to/verify-history.py
#   export MYREPOS=/path/to/repos
#
#   # List the paths in HEAD of the repos (filtering out the directories)
#   for VCFILE in `svn ls -R file://${MYREPOS} | grep -v '/$'`; do
#     echo "Checking ${VCFILE}"
#     ${VERIFY} ${MYREPOS} ${VCFILE}
#   done

import sys
import string
from svn import core, repos, fs

class HistoryChecker:
  def __init__(self, fs_ptr):
    self.fs_ptr = fs_ptr

  def _check_history(self, path, revision):
    root = fs.revision_root(self.fs_ptr, revision)
    changes = fs.paths_changed(root)
    while True:
      if path in changes:
        return 1
      if path == '/':
        return 0
      idx = path.rfind('/')
      if idx != -1:
        path = path[:idx]
      else:
        return 0

  def add_history(self, path, revision, pool=None):
    if not self._check_history(path, revision):
      print("**WRONG** %8d %s" % (revision, path))
    else:
      print("          %8d %s" % (revision, path))


def check_history(fs_ptr, path, revision):
  history = HistoryChecker(fs_ptr)
  repos.history(fs_ptr, path, history.add_history, 1, revision, 1)


def main():
  argc = len(sys.argv)
  if argc < 3 or argc > 4:
    print("Usage: %s PATH-TO-REPOS PATH-IN-REPOS [REVISION]" % sys.argv[0])
    sys.exit(1)

  fs_ptr = repos.fs(repos.open(sys.argv[1]))
  if argc == 3:
    revision = fs.youngest_rev(fs_ptr)
  else:
    revision = int(sys.argv[3])
  check_history(fs_ptr, sys.argv[2], revision)
  sys.exit(0)


if __name__ == '__main__':
  main()
