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

# log-police.py: Ensure that log messages end with a single newline.
# See usage() function for details, or just run with no arguments.

import os
import sys
import getopt
try:
  my_getopt = getopt.gnu_getopt
except AttributeError:
  my_getopt = getopt.getopt

import svn
import svn.fs
import svn.repos
import svn.core


def fix_log_message(log_message):
  """Return a fixed version of LOG_MESSAGE.  By default, this just
  means ensuring that the result ends with exactly one newline and no
  other whitespace.  But if you want to do other kinds of fixups, this
  function is the place to implement them -- all log message fixing in
  this script happens here."""
  return log_message.rstrip() + "\n"


def fix_txn(fs, txn_name):
  "Fix up the log message for txn TXN_NAME in FS.  See fix_log_message()."
  txn = svn.fs.svn_fs_open_txn(fs, txn_name)
  log_message = svn.fs.svn_fs_txn_prop(txn, "svn:log")
  if log_message is not None:
    new_message = fix_log_message(log_message)
    if new_message != log_message:
      svn.fs.svn_fs_change_txn_prop(txn, "svn:log", new_message)


def fix_rev(fs, revnum):
  "Fix up the log message for revision REVNUM in FS.  See fix_log_message()."
  log_message = svn.fs.svn_fs_revision_prop(fs, revnum, 'svn:log')
  if log_message is not None:
    new_message = fix_log_message(log_message)
    if new_message != log_message:
      svn.fs.svn_fs_change_rev_prop(fs, revnum, "svn:log", new_message)


def usage_and_exit(error_msg=None):
  """Write usage information and exit.  If ERROR_MSG is provide, that
  error message is printed first (to stderr), the usage info goes to
  stderr, and the script exits with a non-zero status.  Otherwise,
  usage info goes to stdout and the script exits with a zero status."""
  import os.path
  stream = error_msg and sys.stderr or sys.stdout
  if error_msg:
    stream.write("ERROR: %s\n\n" % error_msg)
  stream.write("USAGE: %s [-t TXN_NAME | -r REV_NUM | --all-revs] REPOS\n"
               % (os.path.basename(sys.argv[0])))
  stream.write("""
Ensure that log messages end with exactly one newline and no other
whitespace characters.  Use as a pre-commit hook by passing '-t TXN_NAME';
fix up a single revision by passing '-r REV_NUM'; fix up all revisions by
passing '--all-revs'.  (When used as a pre-commit hook, may modify the
svn:log property on the txn.)
""")
  sys.exit(error_msg and 1 or 0)


def main(ignored_pool, argv):
  repos_path = None
  txn_name = None
  rev_name = None
  all_revs = False

  try:
    opts, args = my_getopt(argv[1:], 't:r:h?', ["help", "all-revs"])
  except:
    usage_and_exit("problem processing arguments / options.")
  for opt, value in opts:
    if opt == '--help' or opt == '-h' or opt == '-?':
      usage_and_exit()
    elif opt == '-t':
      txn_name = value
    elif opt == '-r':
      rev_name = value
    elif opt == '--all-revs':
      all_revs = True
    else:
      usage_and_exit("unknown option '%s'." % opt)

  if txn_name is not None and rev_name is not None:
    usage_and_exit("cannot pass both -t and -r.")
  if txn_name is not None and all_revs:
    usage_and_exit("cannot pass --all-revs with -t.")
  if rev_name is not None and all_revs:
    usage_and_exit("cannot pass --all-revs with -r.")
  if rev_name is None and txn_name is None and not all_revs:
    usage_and_exit("must provide exactly one of -r, -t, or --all-revs.")
  if len(args) != 1:
    usage_and_exit("only one argument allowed (the repository).")

  repos_path = svn.core.svn_path_canonicalize(args[0])

  # A non-bindings version of this could be implemented by calling out
  # to 'svnlook getlog' and 'svnadmin setlog'.  However, using the
  # bindings results in much simpler code.

  fs = svn.repos.svn_repos_fs(svn.repos.svn_repos_open(repos_path))
  if txn_name is not None:
    fix_txn(fs, txn_name)
  elif rev_name is not None:
    fix_rev(fs, int(rev_name))
  elif all_revs:
    # Do it such that if we're running on a live repository, we'll
    # catch up even with commits that came in after we started.
    last_youngest = 0
    while True:
      youngest = svn.fs.svn_fs_youngest_rev(fs)
      if youngest >= last_youngest:
        for this_rev in range(last_youngest, youngest + 1):
          fix_rev(fs, this_rev)
        last_youngest = youngest + 1
      else:
        break


if __name__ == '__main__':
  sys.exit(svn.core.run_app(main, sys.argv))
