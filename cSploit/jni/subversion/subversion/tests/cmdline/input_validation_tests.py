#!/usr/bin/env python
#
#  input_validation_tests.py: testing input validation
#
#  Subversion is a tool for revision control.
#  See http://subversion.apache.org for more information.
#
# ====================================================================
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

import os
import svntest

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


######################################################################
# Utilities

# Common URL targets to pass where only path arguments are expected.
_invalid_wc_path_targets = ['file:///', '^/']

def run_and_verify_svn_in_wc(sbox, expected_stderr, *varargs):
  """Like svntest.actions.run_and_verify_svn, but temporarily
  changes the current working directory to the sandboxes'
  working copy and only checks the expected error output."""

  wc_dir = sbox.wc_dir
  old_dir = os.getcwd()
  try:
    os.chdir(wc_dir)
    svntest.actions.run_and_verify_svn(None, [], expected_stderr,
                                       *varargs)
  finally:
    os.chdir(old_dir)


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.
#----------------------------------------------------------------------

def invalid_wcpath_add(sbox):
  "non-working copy paths for 'add'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'add', target)

def invalid_wcpath_changelist(sbox):
  "non-working copy paths for 'changelist'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'changelist',
                             'foo', target)
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'changelist',
                             '--remove', target)

def invalid_wcpath_cleanup(sbox):
  "non-working copy paths for 'cleanup'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'cleanup',
                             target)

def invalid_wcpath_commit(sbox):
  "non-working copy paths for 'commit'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: '.*' is not a local path", 'commit', target)

def invalid_copy_sources(sbox):
  "invalid sources for 'copy'"
  sbox.build(read_only=True)
  for (src1, src2) in [("iota", "^/"), ("^/", "iota"), ("file://", "iota")]:
    run_and_verify_svn_in_wc(sbox, "svn: E200007: Cannot mix repository and working " +
                             "copy sources", 'copy', src1, src2, "A")

def invalid_copy_target(sbox):
  "invalid target for 'copy'"
  sbox.build(read_only=True)
  mu_path = os.path.join('A', 'mu')
  C_path = os.path.join('A', 'C')
  run_and_verify_svn_in_wc(sbox, "svn: E155007: Path '.*' is not a directory",
                           'copy', mu_path, C_path, "iota")

def invalid_delete_targets(sbox):
  "invalid targets for 'delete'"
  sbox.build(read_only=True)
  for (target1, target2) in [("iota", "^/"), ("file://", "iota")]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot mix repository and working "
                             "copy targets", 'delete', target1, target2)

def invalid_diff_targets(sbox):
  "invalid targets for 'diff'"
  sbox.build(read_only=True)
  for (target1, target2) in [("iota", "^/"), ("file://", "iota")]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot mix repository and working "
                             "copy targets", 'diff', target1, target2)
  run_and_verify_svn_in_wc(sbox, "svn: E200007: Summarizing diff can only compare "
                           "repository to repository",
                           'diff', '--summarize', "iota", "A")

def invalid_export_targets(sbox):
  "invalid targets for 'export'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn: (E000017|E720183): Can't create directory '.*iota':.*",
                           'export', '.', 'iota')
  for target in ["^/", "file://"]:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path",
                             'export', '.', target)

def invalid_import_args(sbox):
  "invalid arguments for 'import'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path",
                           'import', '^/', '^/')
  run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path",
                           'import', '^/', 'iota')
  run_and_verify_svn_in_wc(sbox, "svn: E205000: Invalid URL 'iota'",
                           'import', 'iota', 'iota')

def invalid_log_targets(sbox):
  "invalid targets for 'log'"
  sbox.build(read_only=True)
  for (target1, target2) in [('^/', '/a/b/c'), ('^/', '^/'), ('^/', 'file://')]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Only relative paths can be " +
                             "specified after a URL for 'svn log', but.*is " +
                             "not a relative path", 'log', target1, target2)

def invalid_merge_args(sbox):
  "invalid arguments for 'merge'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn: E195002: A working copy merge source needs "
                           "an explicit revision", 'merge', 'iota', '^/')
  for (src, target) in [('iota@HEAD', '^/'), ('iota@BASE', 'file://')]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Merge sources must both be either "
                             "paths or URLs", 'merge', src, target)
  run_and_verify_svn_in_wc(sbox, "svn: E155010: Path '.*' does not exist",
                           'merge', 'iota@BASE', 'iota@HEAD', 'nonexistent')
  run_and_verify_svn_in_wc(sbox, "svn: E205000: Too many arguments given",
                          'merge', '-c42', '^/A/B', '^/A/C', 'iota')
  run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot specify a revision range with" +
                           " two URLs", 'merge', '-c42', '^/mu', '^/')
  run_and_verify_svn_in_wc(sbox, "svn: E155010: Path '.*' does not exist",
                           'merge', '-c42', '^/mu', 'nonexistent')

def invalid_wcpath_upgrade(sbox):
  "non-working copy paths for 'upgrade'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'upgrade',
                             target, target)

def invalid_resolve_targets(sbox):
  "non-working copy paths for 'resolve'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'resolve',
                             '--accept', 'base', target)

def invalid_resolved_targets(sbox):
  "non-working copy paths for 'resolved'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'resolved',
                             target)

def invalid_revert_targets(sbox):
  "non-working copy paths for 'revert'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'revert',
                             target)

def invalid_lock_targets(sbox):
  "wc paths and repo URL target mixture for 'lock'"
  sbox.build(read_only=True)
  for (target1, target2) in [("iota", "^/"), ("file://", "iota")]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot mix repository and working "
                             "copy targets", 'lock', target1, target2)

def invalid_unlock_targets(sbox):
  "wc paths and repo URL target mixture for 'unlock'"
  sbox.build(read_only=True)
  for (target1, target2) in [("iota", "^/"), ("file://", "iota")]:
    run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot mix repository and working "
                             "copy targets", 'unlock', target1, target2)

def invalid_status_targets(sbox):
  "non-working copy paths for 'status'"
  sbox.build(read_only=True)
  for target in _invalid_wc_path_targets:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'status',
                             target)

def invalid_patch_targets(sbox):
  "non-working copy paths for 'patch'"
  sbox.build(read_only=True)
  for (target1, target2) in [("foo", "^/"), ("^/", "^/"), ("^/", "foo")]:
    run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'patch',
                             target1, target2)

def invalid_switch_targets(sbox):
  "non-working copy paths for 'switch'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'switch',
                           "^/", "^/")

def invalid_relocate_targets(sbox):
  "non-working copy paths for 'relocate'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'relocate',
                           "^/", "^/", "^/")

# See also basic_tests.py:basic_mkdir_mix_targets(), which tests
# the same thing the other way around.
def invalid_mkdir_targets(sbox):
  "invalid targets for 'mkdir'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn: E205000: Cannot mix repository and working "
                           "copy targets", 'mkdir', "folder", "^/folder")

def invalid_update_targets(sbox):
  "non-working copy paths for 'update'"
  sbox.build(read_only=True)
  run_and_verify_svn_in_wc(sbox, "svn:.*is not a local path", 'update',
                           "^/")

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              invalid_wcpath_add,
              invalid_wcpath_changelist,
              invalid_wcpath_cleanup,
              invalid_wcpath_commit,
              invalid_copy_sources,
              invalid_copy_target,
              invalid_delete_targets,
              invalid_diff_targets,
              invalid_export_targets,
              invalid_import_args,
              invalid_log_targets,
              invalid_merge_args,
              invalid_wcpath_upgrade,
              invalid_resolve_targets,
              invalid_resolved_targets,
              invalid_revert_targets,
              invalid_lock_targets,
              invalid_unlock_targets,
              invalid_status_targets,
              invalid_patch_targets,
              invalid_switch_targets,
              invalid_relocate_targets,
              invalid_mkdir_targets,
              invalid_update_targets,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
