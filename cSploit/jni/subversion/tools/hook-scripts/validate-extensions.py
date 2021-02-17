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

"""\
Check that any files pending commit into a Subversion repository have
suitable file extensions, printing an error and exiting with an
errorful value if any files fail validation.  This is intended to be
used as a Subversion pre-commit hook script.

Syntax 1:

   validate-extensions.py REPOS_PATH TXN_NAME deny EXT [...]

   Ensure that any newly added files do *not* have one of the provided
   file extensions.


Syntax 2:

   validate-extensions.py REPOS_PATH TXN_NAME allow EXT [...]

   Ensure that any newly added files *do* have one of the provided
   file extensions.  (Extension-less files are disallowed.)

"""

import sys
import os
from svn import repos, fs, core

def validate_added_extensions(repos_path, txn_name, extensions, action):
  # Open the repository and transaction.
  fs_ptr = repos.fs(repos.open(repos_path))
  txn_t = fs.open_txn(fs_ptr, txn_name)
  txn_root = fs.txn_root(txn_t)

  # Fetch the changes made in this transaction.
  changes = fs.svn_fs_paths_changed(txn_root)
  paths = changes.keys()

  # Check the changes.
  for path in paths:
    change = changes[path]

    # Always allow deletions.
    if change.change_kind == fs.path_change_delete:
      continue

    # Always allow non-files.
    kind = fs.check_path(txn_root, path)
    if kind != core.svn_node_file:
      continue

    # If this was a newly added (without history) file ...
    if ((change.change_kind == fs.path_change_replace) \
        or (change.change_kind == fs.path_change_add)):
      copyfrom_rev, copyfrom_path = fs.copied_from(txn_root, path)
      if copyfrom_rev == core.SVN_INVALID_REVNUM:

        # ... then check it for a valid extension.
        base, ext = os.path.splitext(path)
        if ext:
          ext = ext[1:].lower()
        if ((ext in extensions) and (action == 'deny')) \
           or ((ext not in extensions) and (action == 'allow')):
          sys.stderr.write("Path '%s' has an extension disallowed by server "
                           "configuration.\n" % (path))
          sys.exit(1)

def usage_and_exit(errmsg=None):
  stream = errmsg and sys.stderr or sys.stdout
  stream.write(__doc__)
  if errmsg:
    stream.write("ERROR: " + errmsg + "\n")
  sys.exit(errmsg and 1 or 0)

def main():
  argc = len(sys.argv)
  if argc < 5:
    usage_and_exit("Not enough arguments.")
  repos_path = sys.argv[1]
  txn_name = sys.argv[2]
  action = sys.argv[3]
  if action not in ("allow", "deny"):
    usage_and_exit("Invalid action '%s'.  Expected either 'allow' or 'deny'."
                   % (action))
  extensions = [x.lower() for x in sys.argv[4:]]
  validate_added_extensions(repos_path, txn_name, extensions, action)

if __name__ == "__main__":
  main()
