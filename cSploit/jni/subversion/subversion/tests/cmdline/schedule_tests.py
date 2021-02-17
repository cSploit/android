#!/usr/bin/env python
#
#  schedule_tests.py:  testing working copy scheduling
#                      (adds, deletes, reversion)
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

# General modules
import os

# Our testing module
import svntest

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem
exp_noop_up_out = svntest.actions.expected_noop_update_output

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.
#

#######################################################################
#  Stage I - Schedules and modifications, verified with `svn status'
#
#  These tests make schedule changes and local mods, and verify that status
#  output is as expected.  In a second stage, reversion of these changes is
#  tested.  Potentially, a third stage could test committing these same
#  changes.
#
#  NOTE: these tests are run within the Stage II tests, not on their own.
#

def add_files(sbox):
  "schedule: add some files"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create some files, then schedule them for addition
  delta_path = sbox.ospath('delta')
  zeta_path = sbox.ospath('A/B/zeta')
  epsilon_path = sbox.ospath('A/D/G/epsilon')

  svntest.main.file_append(delta_path, "This is the file 'delta'.")
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.")
  svntest.main.file_append(epsilon_path, "This is the file 'epsilon'.")

  sbox.simple_add('delta', 'A/B/zeta', 'A/D/G/epsilon')

  # Make sure the adds show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'delta' : Item(status='A ', wc_rev=0),
    'A/B/zeta' : Item(status='A ', wc_rev=0),
    'A/D/G/epsilon' : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def add_directories(sbox):
  "schedule: add some directories"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create some directories, then schedule them for addition
  X_path = sbox.ospath('X')
  Y_path = sbox.ospath('A/C/Y')
  Z_path = sbox.ospath('A/D/H/Z')

  os.mkdir(X_path)
  os.mkdir(Y_path)
  os.mkdir(Z_path)

  sbox.simple_add('X', 'A/C/Y', 'A/D/H/Z')

  # Make sure the adds show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X' : Item(status='A ', wc_rev=0),
    'A/C/Y' : Item(status='A ', wc_rev=0),
    'A/D/H/Z' : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def nested_adds(sbox):
  "schedule: add some nested files and directories"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create some directories then schedule them for addition
  X_path = sbox.ospath('X')
  Y_path = sbox.ospath('A/C/Y')
  Z_path = sbox.ospath('A/D/H/Z')

  os.mkdir(X_path)
  os.mkdir(Y_path)
  os.mkdir(Z_path)

  # Now, create some files and directories to put into our newly added
  # directories
  P_path = sbox.ospath('X/P')
  Q_path = sbox.ospath('A/C/Y/Q')
  R_path = sbox.ospath('A/D/H/Z/R')

  os.mkdir(P_path)
  os.mkdir(Q_path)
  os.mkdir(R_path)

  delta_path = sbox.ospath('X/delta')
  epsilon_path = sbox.ospath('A/C/Y/epsilon')
  upsilon_path = sbox.ospath('A/C/Y/upsilon')
  zeta_path = sbox.ospath('A/D/H/Z/zeta')

  svntest.main.file_append(delta_path, "This is the file 'delta'.")
  svntest.main.file_append(epsilon_path, "This is the file 'epsilon'.")
  svntest.main.file_append(upsilon_path, "This is the file 'upsilon'.")
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.")

  # Finally, let's try adding our new files and directories
  sbox.simple_add('X', 'A/C/Y', 'A/D/H/Z')

  # Make sure the adds show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X' : Item(status='A ', wc_rev=0),
    'A/C/Y' : Item(status='A ', wc_rev=0),
    'A/D/H/Z' : Item(status='A ', wc_rev=0),
    'X/P' : Item(status='A ', wc_rev=0),
    'A/C/Y/Q' : Item(status='A ', wc_rev=0),
    'A/D/H/Z/R' : Item(status='A ', wc_rev=0),
    'X/delta' : Item(status='A ', wc_rev=0),
    'A/C/Y/epsilon' : Item(status='A ', wc_rev=0),
    'A/C/Y/upsilon' : Item(status='A ', wc_rev=0),
    'A/D/H/Z/zeta' : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def add_executable(sbox):
  "schedule: add some executable files"

  sbox.build(read_only = True)

  def runTest(wc_dir, fileName, perm, executable):
    file_ospath = sbox.ospath(fileName)
    if executable:
      expected_out = ["*\n"]
    else:
      expected_out = []

    # create an empty file
    open(file_ospath, "w")

    os.chmod(file_ospath, perm)
    sbox.simple_add(fileName)
    svntest.actions.run_and_verify_svn(None, expected_out, [],
                                       'propget', "svn:executable", file_ospath)

  test_cases = [
    ("all_exe",   0777, 1),
    ("none_exe",  0666, 0),
    ("user_exe",  0766, 1),
    ("group_exe", 0676, 0),
    ("other_exe", 0667, 0),
    ]
  for test_case in test_cases:
    runTest(sbox.wc_dir, *test_case)

#----------------------------------------------------------------------

def delete_files(sbox):
  "schedule: delete some files"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Schedule some files for deletion
  sbox.simple_rm('iota', 'A/mu', 'A/D/G/rho', 'A/D/H/omega')

  # Make sure the deletes show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', 'A/mu', 'A/D/G/rho', 'A/D/H/omega',
                        status='D ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def delete_dirs(sbox):
  "schedule: delete some directories"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Schedule some directories for deletion (this is recursive!)
  sbox.simple_rm('A/B/E', 'A/B/F', 'A/D/H')

  # Make sure the deletes show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/B/F',
                        'A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi',
                        status='D ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#######################################################################
#  Stage II - Reversion of changes made in Stage I
#
#  Each test in Stage II calls the corresponding Stage I test
#  and then also tests reversion of those changes.
#

def check_reversion(files, output):
  expected_output = []
  for file in files:
    expected_output = expected_output + ["Reverted '" + file + "'\n"]
  output.sort()
  expected_output.sort()
  if output != expected_output:
    print("Expected output: %s" % expected_output)
    print("Actual output:   %s" % output)
    raise svntest.Failure

#----------------------------------------------------------------------

def revert_add_files(sbox):
  "revert: add some files"

  add_files(sbox)
  wc_dir = sbox.wc_dir

  # Revert our changes recursively from wc_dir.
  delta_path = sbox.ospath('delta')
  zeta_path = sbox.ospath('A/B/zeta')
  epsilon_path = sbox.ospath('A/D/G/epsilon')
  files = [delta_path, zeta_path, epsilon_path]

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)

#----------------------------------------------------------------------

def revert_add_directories(sbox):
  "revert: add some directories"

  add_directories(sbox)
  wc_dir = sbox.wc_dir

  # Revert our changes recursively from wc_dir.
  X_path = sbox.ospath('X')
  Y_path = sbox.ospath('A/C/Y')
  Z_path = sbox.ospath('A/D/H/Z')
  files = [X_path, Y_path, Z_path]

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)

#----------------------------------------------------------------------

def revert_nested_adds(sbox):
  "revert: add some nested files and directories"

  nested_adds(sbox)
  wc_dir = sbox.wc_dir

  # Revert our changes recursively from wc_dir.
  X_path = sbox.ospath('X')
  Y_path = sbox.ospath('A/C/Y')
  Z_path = sbox.ospath('A/D/H/Z')
  files = ([X_path, Y_path, Z_path]
           + [os.path.join(X_path, child)
              for child in ['P', 'delta']]
           + [os.path.join(Y_path, child)
              for child in ['Q', 'epsilon', 'upsilon']]
           + [os.path.join(Z_path, child)
              for child in ['R', 'zeta']])

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)

#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_posix_os)
def revert_add_executable(sbox):
  "revert: add some executable files"

  add_executable(sbox)
  wc_dir = sbox.wc_dir

  all_path = sbox.ospath('all_exe')
  none_path = sbox.ospath('none_exe')
  user_path = sbox.ospath('user_exe')
  group_path = sbox.ospath('group_exe')
  other_path = sbox.ospath('other_exe')
  files = [all_path, none_path, user_path, group_path, other_path]

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)

#----------------------------------------------------------------------

def revert_delete_files(sbox):
  "revert: delete some files"

  delete_files(sbox)
  wc_dir = sbox.wc_dir

  # Revert our changes recursively from wc_dir.
  iota_path = sbox.ospath('iota')
  mu_path = sbox.ospath('A/mu')
  rho_path = sbox.ospath('A/D/G/rho')
  omega_path = sbox.ospath('A/D/H/omega')
  files = [iota_path, mu_path, omega_path, rho_path]

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)

#----------------------------------------------------------------------

def revert_delete_dirs(sbox):
  "revert: delete some directories"

  delete_dirs(sbox)
  wc_dir = sbox.wc_dir

  # Revert our changes recursively from wc_dir.
  E_path = sbox.ospath('A/B/E')
  F_path = sbox.ospath('A/B/F')
  H_path = sbox.ospath('A/D/H')
  alpha_path = sbox.ospath('A/B/E/alpha')
  beta_path  = sbox.ospath('A/B/E/beta')
  chi_path   = sbox.ospath('A/D/H/chi')
  omega_path = sbox.ospath('A/D/H/omega')
  psi_path   = sbox.ospath('A/D/H/psi')
  files = [E_path, F_path, H_path,
           alpha_path, beta_path, chi_path, omega_path, psi_path]

  exit_code, output, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                              'revert',
                                                              '--recursive',
                                                              wc_dir)
  check_reversion(files, output)


#######################################################################
#  Regression tests
#

#----------------------------------------------------------------------
# Regression test for issue #863:
#
# Suppose here is a scheduled-add file or directory which is
# also missing.  If I want to make the working copy forget all
# knowledge of the item ("unschedule" the addition), then either 'svn
# revert' or 'svn rm' will make that happen by removing the entry from
# .svn/entries file. While 'svn revert' does with no error,
# 'svn rm' does it with error.
@Issue(863)
def unschedule_missing_added(sbox):
  "unschedule addition on missing items"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create some files and dirs, then schedule them for addition
  file1_path = sbox.ospath('file1')
  file2_path = sbox.ospath('file2')
  dir1_path = sbox.ospath('dir1')
  dir2_path = sbox.ospath('dir2')

  svntest.main.file_append(file1_path, "This is the file 'file1'.")
  svntest.main.file_append(file2_path, "This is the file 'file2'.")
  sbox.simple_add('file1', 'file2')
  sbox.simple_mkdir('dir1', 'dir2')

  # Make sure the 4 adds show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'file1' : Item(status='A ', wc_rev=0),
    'file2' : Item(status='A ', wc_rev=0),
    'dir1' : Item(status='A ', wc_rev=0),
    'dir2' : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Poof, all 4 added things are now missing in action.
  os.remove(file1_path)
  os.remove(file2_path)
  svntest.main.safe_rmtree(dir1_path)
  svntest.main.safe_rmtree(dir2_path)

  # Unschedule the additions, using 'svn rm' and 'svn revert'.
  # FILE1_PATH will throw an error. DIR1_PATH will not since the stub is
  # still available in the parent directory.
  svntest.main.run_svn(svntest.verify.AnyOutput, 'rm', file1_path)
  ### actually, the stub does not provide enough information to revert
  ### the addition, so this command will fail. marking as XFail
  sbox.simple_rm('dir1')
  sbox.simple_revert('file2', 'dir2')

  # 'svn st' should now show absolutely zero local mods.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
# Regression test for issue #962:
#
# Make sure 'rm foo; svn rm foo' works on files and directories.
# Also make sure that the deletion is committable.
@Issue(962)
def delete_missing(sbox):
  "schedule and commit deletion on missing items"

  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = sbox.ospath('A/mu')
  H_path = sbox.ospath('A/D/H')

  # Manually remove a file and a directory.
  os.remove(mu_path)
  svntest.main.safe_rmtree(H_path)

  # Now schedule them for deletion anyway, and make sure no error is output.
  sbox.simple_rm('A/mu', 'A/D/H')

  # Commit the deletions.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Deleting'),
    'A/D/H' : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/mu', 'A/D/H',
                         'A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi')
  expected_status.tweak(wc_rev=1)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------
# Regression test for issue #854:
# Revert . inside an svn added empty directory should generate an error.
# Not anymore!  wc-ng uses absolute paths for everything, which means we
# can handle this case without too much trouble.
@Issue(854)
def revert_inside_newly_added_dir(sbox):
  "revert inside a newly added dir"

  sbox.build(read_only = True)

  # Schedule a new directory for addition
  sbox.simple_mkdir('foo')

  # Now change into the newly added directory, revert and make sure
  # no error is output.
  os.chdir(sbox.ospath('foo'))
  svntest.main.run_svn(None, 'revert', '.')

#----------------------------------------------------------------------
# Regression test for issue #1609:
# 'svn status' should show a schedule-add directory as 'A' not '?'
@Issue(1609)
def status_add_deleted_directory(sbox):
  "status after add of deleted directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  # The original recipe:
  #
  # svnadmin create repo
  # svn mkdir file://`pwd`/repo/foo -m r1
  # svn co file://`pwd`/repo wc
  # svn rm wc/foo
  # rm -rf wc/foo
  # svn ci wc -m r2
  # svn mkdir wc/foo

  A_path = sbox.ospath('A')

  sbox.simple_rm('A')
  svntest.main.safe_rmtree(A_path)
  sbox.simple_commit()

  sbox.simple_mkdir('A')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status = svntest.wc.State(wc_dir,
                                     { ''     : Item(status='  ', wc_rev=1),
                                       'A'    : Item(status='A ', wc_rev=0),
                                       'iota' : Item(status='  ', wc_rev=1),
                                       })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Update will *not* remove the entry for A despite it being marked
  # deleted.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(2), [],
                                     'up', wc_dir)
  expected_status.tweak('', 'iota', wc_rev=2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
# Regression test for issue #939:
# Recursive 'svn add' should still traverse already-versioned dirs.
@Issue(939)
def add_recursive_already_versioned(sbox):
  "'svn add' should traverse already-versioned dirs"

  wc_dir = sbox.wc_dir

  svntest.actions.make_repo_and_wc(sbox)

  # Create some files, then schedule them for addition
  delta_path = sbox.ospath('delta')
  zeta_path = sbox.ospath('A/B/zeta')
  epsilon_path = sbox.ospath('A/D/G/epsilon')

  svntest.main.file_append(delta_path, "This is the file 'delta'.")
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.")
  svntest.main.file_append(epsilon_path, "This is the file 'epsilon'.")

  # Make sure the adds show up as such in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'delta' : Item(status='A ', wc_rev=0),
    'A/B/zeta' : Item(status='A ', wc_rev=0),
    'A/D/G/epsilon' : Item(status='A ', wc_rev=0),
    })

  # Perform the add with the --force flag, and check the status.
  ### TODO:  This part won't work -- you have to be inside the working copy
  ### or else Subversion will think you're trying to add the working copy
  ### to its parent directory, and will (possibly, if the parent directory
  ### isn't versioned) fail.
  #svntest.main.run_svn(None, 'add', '--force', wc_dir)
  #svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now revert, and do the adds again from inside the working copy.
  svntest.main.run_svn(None, 'revert', '--recursive', wc_dir)
  saved_wd = os.getcwd()
  os.chdir(wc_dir)
  svntest.main.run_svn(None, 'add', '--force', '.')
  os.chdir(saved_wd)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
# Regression test for the case where "svn mkdir" outside a working copy
# would create a directory, but then not clean up after itself when it
# couldn't add it to source control.
def fail_add_directory(sbox):
  "'svn mkdir' should clean up after itself on error"
  # This test doesn't use a working copy
  svntest.main.safe_rmtree(sbox.wc_dir)
  os.makedirs(sbox.wc_dir)

  os.chdir(sbox.wc_dir)
  svntest.actions.run_and_verify_svn('Failed mkdir',
                                     None, svntest.verify.AnyOutput,
                                     'mkdir', 'A')
  if os.path.exists('A'):
    raise svntest.Failure('svn mkdir created an unversioned directory')


#----------------------------------------------------------------------
# Regression test for #2440
# Ideally this test should test for the exit status of the
# 'svn rm non-existent' invocation.
# As the corresponding change to get the exit code of svn binary invoked needs
# a change in many testcase, for now this testcase checks the stderr.
def delete_non_existent(sbox):
  "'svn rm non-existent' should exit with an error"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  os.chdir(wc_dir)
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'rm', '--force', 'non-existent')


#----------------------------------------------------------------------
# Problem encountered by cmpilato when he inadvertantly upset an
# 'svn rm --keep-local' and had to retry it.
def delete_redelete_fudgery(sbox):
  "retry of manually upset --keep-local deletion"

  sbox.build()
  wc_dir = sbox.wc_dir
  B_path = os.path.join(wc_dir, 'A', 'B')

  # Delete 'A/B' using --keep-local, then remove at the OS level.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', '--keep-local', B_path)
  svntest.main.safe_rmtree(B_path)

  # Update the tree.
  #
  ### When WC-NG is running in single-DB mode (one .svn directory and
  ### database for the whole working copy), I suspect that this update
  ### will change.  Today it re-adds the directory which we just
  ### scheduled for deletion because the only record of that
  ### scheduling is stored -- you guessed it -- the directory's .svn/
  ### area... which we just deleted from disk.
  ###
  ### In single-DB-WC-NG-land, though, deleting the directory from
  ### disk should have no bearing whatsoever on the scheduling
  ### information stored now in the working copy root's one DB.  That
  ### could change the whole flow of this test, possible leading us to
  ### remove it as altogether irrelevant.  --cmpilato
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  # Now try to run
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', '--keep-local', B_path)

def propset_on_deleted_should_fail(sbox):
  "calling svn propset on a deleted node should fail"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota = os.path.join(wc_dir, 'iota')

  svntest.actions.run_and_verify_svn(None, None, [], 'rm', iota)

  svntest.actions.run_and_verify_svn(None, None, "svn: E155023: Can't set propert.*",
                                     'ps', 'prop', 'val', iota)

@Issue(3468)
def replace_dir_delete_child(sbox):
  "replace a dir, then delete a child"
  # The purpose of this test is to make sure that when a child of a
  # replaced directory is deleted, the result can be committed.

  sbox.build()

  # Replace A/D/H with a copy of A/B
  sbox.simple_rm('A/D/H')
  sbox.simple_copy('A/B', 'A/D/H')

  # Remove two children
  sbox.simple_rm('A/D/H/lambda')
  sbox.simple_rm('A/D/H/E')

  # Don't look at what "svn status" says before commit.  It's not clear
  # what it should be and that's not the point of this test.

  # Commit.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/D/H'         : Item(verb='Replacing'),
    'A/D/H/lambda'  : Item(verb='Deleting'),
    'A/D/H/E'       : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.add({
    'A/D/H/F' : Item(status='  ', wc_rev=0),
    })
  expected_status.tweak('A/D/H', 'A/D/H/F', wc_rev=2)
  expected_status.remove('A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi')

  svntest.actions.run_and_verify_commit(sbox.wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, sbox.wc_dir)


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              revert_add_files,
              revert_add_directories,
              revert_nested_adds,
              revert_add_executable,
              revert_delete_files,
              revert_delete_dirs,
              unschedule_missing_added,
              delete_missing,
              revert_inside_newly_added_dir,
              status_add_deleted_directory,
              add_recursive_already_versioned,
              fail_add_directory,
              delete_non_existent,
              delete_redelete_fudgery,
              propset_on_deleted_should_fail,
              replace_dir_delete_child,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
