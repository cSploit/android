#!/usr/bin/env python
#
#  switch_tests.py:  testing `svn switch'.
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
import shutil, re, os

# Our testing module
import svntest
from svntest import verify, actions, main

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem

from svntest.main import SVN_PROP_MERGEINFO, server_has_mergeinfo
from externals_tests import change_external


### Bummer.  It would be really nice to have easy access to the URL
### member of our entries files so that switches could be testing by
### examining the modified ancestry.  But status doesn't show this
### information.  Hopefully in the future the cmdline binary will have
### a subcommand for dumping multi-line detailed information about
### versioned things.  Until then, we'll stick with the traditional
### verification methods.
###
### gjs says: we have 'svn info' now

def get_routine_status_state(wc_dir):
  """get the routine status list for WC_DIR at the completion of an
  initial call to do_routine_switching()"""

  # Construct some paths for convenience
  ADH_path = os.path.join(wc_dir, 'A', 'D', 'H')
  chi_path = os.path.join(ADH_path, 'chi')
  omega_path = os.path.join(ADH_path, 'omega')
  psi_path = os.path.join(ADH_path, 'psi')
  pi_path = os.path.join(ADH_path, 'pi')
  tau_path = os.path.join(ADH_path, 'tau')
  rho_path = os.path.join(ADH_path, 'rho')

  # Now generate a state
  state = svntest.actions.get_virginal_state(wc_dir, 1)
  state.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/F', 'A/B/lambda')
  state.add({
    'A/B/pi' : Item(status='  ', wc_rev=1),
    'A/B/tau' : Item(status='  ', wc_rev=1),
    'A/B/rho' : Item(status='  ', wc_rev=1),
    })

  return state

#----------------------------------------------------------------------

def get_routine_disk_state(wc_dir):
  """get the routine disk list for WC_DIR at the completion of an
  initial call to do_routine_switching()"""

  disk = svntest.main.greek_state.copy()

  # iota has the same contents as gamma
  disk.tweak('iota', contents=disk.desc['A/D/gamma'].contents)

  # A/B/* no longer exist, but have been replaced by copies of A/D/G/*
  disk.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/F', 'A/B/lambda')
  disk.add({
    'A/B/pi' : Item("This is the file 'pi'.\n"),
    'A/B/rho' : Item("This is the file 'rho'.\n"),
    'A/B/tau' : Item("This is the file 'tau'.\n"),
    })

  return disk

#----------------------------------------------------------------------

def do_routine_switching(wc_dir, repo_url, verify):
  """perform some routine switching of the working copy WC_DIR for
  other tests to use.  If VERIFY, then do a full verification of the
  switching, else don't bother."""

  ### Switch the file `iota' to `A/D/gamma'.

  # Construct some paths for convenience
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_url = repo_url + '/A/D/gamma'

  if verify:
    # Create expected output tree
    expected_output = svntest.wc.State(wc_dir, {
      'iota' : Item(status='U '),
      })

    # Create expected disk tree (iota will have gamma's contents)
    expected_disk = svntest.main.greek_state.copy()
    expected_disk.tweak('iota',
                        contents=expected_disk.desc['A/D/gamma'].contents)

    # Create expected status tree
    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.tweak('iota', switched='S')

    # Do the switch and check the results in three ways.
    svntest.actions.run_and_verify_switch(wc_dir, iota_path, gamma_url,
                                          expected_output,
                                          expected_disk,
                                          expected_status,
                                          None, None, None, None, None,
                                          False, '--ignore-ancestry')
  else:
    svntest.main.run_svn(None, 'switch', '--ignore-ancestry',
                         gamma_url, iota_path)

  ### Switch the directory `A/B' to `A/D/G'.

  # Construct some paths for convenience
  AB_path = os.path.join(wc_dir, 'A', 'B')
  ADG_url = repo_url + '/A/D/G'

  if verify:
    # Create expected output tree
    expected_output = svntest.wc.State(wc_dir, {
      'A/B/E'       : Item(status='D '),
      'A/B/F'       : Item(status='D '),
      'A/B/lambda'  : Item(status='D '),
      'A/B/pi' : Item(status='A '),
      'A/B/tau' : Item(status='A '),
      'A/B/rho' : Item(status='A '),
      })

    # Create expected disk tree (iota will have gamma's contents,
    # A/B/* will look like A/D/G/*)
    expected_disk = get_routine_disk_state(wc_dir)

    # Create expected status
    expected_status = get_routine_status_state(wc_dir)
    expected_status.tweak('iota', 'A/B', switched='S')

    # Do the switch and check the results in three ways.
    svntest.actions.run_and_verify_switch(wc_dir, AB_path, ADG_url,
                                          expected_output,
                                          expected_disk,
                                          expected_status,
                                          None, None, None, None, None,
                                          False, '--ignore-ancestry')
  else:
    svntest.main.run_svn(None, 'switch', '--ignore-ancestry',
                         ADG_url, AB_path)


#----------------------------------------------------------------------

def commit_routine_switching(wc_dir, verify):
  "Commit some stuff in a routinely-switched working copy."

  # Make some local mods
  iota_path = os.path.join(wc_dir, 'iota')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  Z_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')
  zeta_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z', 'zeta')

  svntest.main.file_append(iota_path, "apple")
  svntest.main.file_append(Bpi_path, "melon")
  svntest.main.file_append(Gpi_path, "banana")
  os.mkdir(Z_path)
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.\n")
  svntest.main.run_svn(None, 'add', Z_path)

  # Try to commit.  We expect this to fail because, if all the
  # switching went as expected, A/B/pi and A/D/G/pi point to the
  # same URL.  We don't allow this.
  svntest.actions.run_and_verify_commit(
    wc_dir, None, None,
    "svn: E195003: Cannot commit both .* as they refer to the same URL$",
    wc_dir)

  # Okay, that all taken care of, let's revert the A/D/G/pi path and
  # move along.  Afterward, we should be okay to commit.  (Sorry,
  # holsta, that banana has to go...)
  svntest.main.run_svn(None, 'revert', Gpi_path)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/Z' : Item(verb='Adding'),
    'A/D/G/Z/zeta' : Item(verb='Adding'),
    'iota' : Item(verb='Sending'),
    'A/B/pi' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('iota', 'A/B/pi', wc_rev=2, status='  ')
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2),
    })

  # Commit should succeed
  if verify:
    svntest.actions.run_and_verify_commit(wc_dir,
                                          expected_output,
                                          expected_status,
                                          None, wc_dir)
  else:
    svntest.main.run_svn(None,
                         'ci', '-m', 'log msg', wc_dir)


######################################################################
# Tests
#

#----------------------------------------------------------------------

def routine_switching(sbox):
  "test some basic switching operations"

  sbox.build(read_only = True)

  # Setup (and verify) some switched things
  do_routine_switching(sbox.wc_dir, sbox.repo_url, 1)


#----------------------------------------------------------------------

def commit_switched_things(sbox):
  "commits after some basic switching operations"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # Commit some stuff (and verify)
  commit_routine_switching(wc_dir, 1)


#----------------------------------------------------------------------

def full_update(sbox):
  "update wc that contains switched things"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # Copy wc_dir to a backup location
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Commit some stuff (don't bother verifying)
  commit_routine_switching(wc_backup, 0)

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  BZ_path = os.path.join(wc_dir, 'A', 'B', 'Z')
  Bzeta_path = os.path.join(wc_dir, 'A', 'B', 'Z', 'zeta')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  GZ_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')
  Gzeta_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z', 'zeta')

  # Create expected output tree for an update of wc_backup.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/D/gamma' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='A '),
    'A/B/Z/zeta' : Item(status='A '),
    'A/D/G/pi' : Item(status='U '),
    'A/D/G/Z' : Item(status='A '),
    'A/D/G/Z/zeta' : Item(status='A '),
    })

  # Create expected disk tree for the update
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('iota', contents="This is the file 'gamma'.\napple")
  expected_disk.tweak('A/D/gamma', contents="This is the file 'gamma'.\napple")
  expected_disk.tweak('A/B/pi', contents="This is the file 'pi'.\nmelon")
  expected_disk.tweak('A/D/G/pi', contents="This is the file 'pi'.\nmelon")
  expected_disk.add({
    'A/B/Z' : Item(),
    'A/B/Z/zeta' : Item(contents="This is the file 'zeta'.\n"),
    'A/D/G/Z' : Item(),
    'A/D/G/Z/zeta' : Item(contents="This is the file 'zeta'.\n"),
    })

  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(wc_rev=2)
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2),
    'A/B/Z' : Item(status='  ', wc_rev=2),
    'A/B/Z/zeta' : Item(status='  ', wc_rev=2),
    })
  expected_status.tweak('iota', 'A/B', switched='S')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

def full_rev_update(sbox):
  "reverse update wc that contains switched things"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # Commit some stuff (don't bother verifying)
  commit_routine_switching(wc_dir, 0)

  # Update to HEAD (tested elsewhere)
  svntest.main.run_svn(None, 'up', wc_dir)

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  Bpi_path = os.path.join(wc_dir, 'A', 'B', 'pi')
  BZ_path = os.path.join(wc_dir, 'A', 'B', 'Z')
  Gpi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  GZ_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Z')

  # Now, reverse update, back to the pre-commit state.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/D/gamma' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='D '),
    'A/D/G/pi' : Item(status='U '),
    'A/D/G/Z' : Item(status='D '),
    })

  # Create expected disk tree
  expected_disk = get_routine_disk_state(wc_dir)

  # Create expected status
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak('iota', 'A/B', switched='S')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '1', wc_dir)

#----------------------------------------------------------------------

def update_switched_things(sbox):
  "update switched wc things to HEAD"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # Copy wc_dir to a backup location
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Commit some stuff (don't bother verifying)
  commit_routine_switching(wc_backup, 0)

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  B_path = os.path.join(wc_dir, 'A', 'B')

  # Create expected output tree for an update of wc_backup.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='A '),
    'A/B/Z/zeta' : Item(status='A '),
    })

  # Create expected disk tree for the update
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('iota', contents="This is the file 'gamma'.\napple")

  expected_disk.tweak('A/B/pi', contents="This is the file 'pi'.\nmelon")
  expected_disk.add({
    'A/B/Z' : Item(),
    'A/B/Z/zeta' : Item("This is the file 'zeta'.\n"),
    })

  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('A/B', 'A/B/pi', 'A/B/rho', 'A/B/tau', 'iota',
                        wc_rev=2)
  expected_status.add({
    'A/B/Z' : Item(status='  ', wc_rev=2),
    'A/B/Z/zeta' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 0,
                                        B_path,
                                        iota_path)


#----------------------------------------------------------------------

def rev_update_switched_things(sbox):
  "reverse update switched wc things to an older rev"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # Commit some stuff (don't bother verifying)
  commit_routine_switching(wc_dir, 0)

  # Some convenient path variables
  iota_path = os.path.join(wc_dir, 'iota')
  B_path = os.path.join(wc_dir, 'A', 'B')

  # Update to HEAD (tested elsewhere)
  svntest.main.run_svn(None, 'up', wc_dir)

  # Now, reverse update, back to the pre-commit state.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/B/pi' : Item(status='U '),
    'A/B/Z' : Item(status='D '),
    })

  # Create expected disk tree
  expected_disk = get_routine_disk_state(wc_dir)
  expected_disk.tweak('A/D/gamma', contents="This is the file 'gamma'.\napple")
  expected_disk.tweak('A/D/G/pi', contents="This is the file 'pi'.\nmelon")
  expected_disk.add({
    'A/D/G/Z' : Item(),
    'A/D/G/Z/zeta' : Item("This is the file 'zeta'.\n"),
    })

  # Create expected status tree for the update.
  expected_status = get_routine_status_state(wc_dir)
  expected_status.tweak(wc_rev=2)
  expected_status.tweak('iota', 'A/B', switched='S')
  expected_status.tweak('A/B', 'A/B/pi', 'A/B/rho', 'A/B/tau', 'iota',
                        wc_rev=1)
  expected_status.add({
    'A/D/G/Z' : Item(status='  ', wc_rev=2),
    'A/D/G/Z/zeta' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '1',
                                        B_path,
                                        iota_path)


#----------------------------------------------------------------------

def log_switched_file(sbox):
  "show logs for a switched file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup some switched things (don't bother verifying)
  do_routine_switching(wc_dir, sbox.repo_url, 0)

  # edit and commit switched file 'iota'
  iota_path = os.path.join(wc_dir, 'iota')
  svntest.main.run_svn(None, 'ps', 'x', 'x', iota_path)
  svntest.main.run_svn(None,
                       'ci', '-m',
                       'set prop on switched iota',
                       iota_path)

  # log switched file 'iota'
  exit_code, output, error = svntest.main.run_svn(None, 'log', iota_path)
  for line in output:
    if line.find("set prop on switched iota") != -1:
      break
  else:
    raise svntest.Failure

#----------------------------------------------------------------------

def delete_subdir(sbox):
  "switch that deletes a sub-directory"
  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  A_url = sbox.repo_url + '/A'
  A2_url = sbox.repo_url + '/A2'
  A2_B_F_url = sbox.repo_url + '/A2/B/F'

  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'cp', '-m', 'make copy', A_url, A2_url)

  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 3.\n'], [],
                                     'rm', '-m', 'delete subdir', A2_B_F_url)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F' : Item(status='D '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/F')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.tweak('A', switched='S')
  expected_status.remove('A/B/F')
  expected_status.tweak('', 'iota', wc_rev=1)

  # Used to fail with a 'directory not locked' error for A/B/F
  svntest.actions.run_and_verify_switch(wc_dir, A_path, A2_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

#----------------------------------------------------------------------
# Issue 1532: Switch a file to a dir: can't switch it back to the file
@XFail()
@Issue(1532)
def file_dir_file(sbox):
  "switch a file to a dir and back to the file"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  file_path = os.path.join(wc_dir, 'iota')
  file_url = sbox.repo_url + '/iota'
  dir_url = sbox.repo_url + '/A/C'

  svntest.actions.run_and_verify_svn(None, None, [], 'switch',
                                     '--ignore-ancestry', dir_url, file_path)
  if not os.path.isdir(file_path):
    raise svntest.Failure

  svntest.actions.run_and_verify_svn(None, None, [], 'switch',
                                     '--ignore-ancestry', file_url, file_path)
  if not os.path.isfile(file_path):
    raise svntest.Failure

#----------------------------------------------------------------------
# Issue 1751: "svn switch --non-recursive" does not switch existing files,
# and generates the wrong URL for new files.

def nonrecursive_switching(sbox):
  "non-recursive switch"
  sbox.build()
  wc1_dir = sbox.wc_dir
  wc2_dir = os.path.join(wc1_dir, 'wc2')

  # "Trunk" will be the existing dir "A/", with existing file "mu".
  # "Branch" will be the new dir "branch/version1/", with added file "newfile".
  # "wc1" will hold the whole repository (including trunk and branch).
  # "wc2" will hold the "trunk" and then be switched to the "branch".
  # It is irrelevant that wc2 is located on disk as a sub-directory of wc1.
  trunk_url = sbox.repo_url + '/A'
  branch_url = sbox.repo_url + '/branch'
  version1_url = branch_url + '/version1'
  wc1_new_file = os.path.join(wc1_dir, 'branch', 'version1', 'newfile')
  wc2_new_file = os.path.join(wc2_dir, 'newfile')
  wc2_mu_file = os.path.join(wc2_dir, 'mu')
  wc2_B_dir = os.path.join(wc2_dir, 'B')
  wc2_C_dir = os.path.join(wc2_dir, 'C')
  wc2_D_dir = os.path.join(wc2_dir, 'D')

  # Check out the trunk as "wc2"
  svntest.main.run_svn(None, 'co', trunk_url, wc2_dir)

  # Make a branch, and add a new file, in "wc_dir" and repository
  svntest.main.run_svn(None,
                       'mkdir', '-m', '', branch_url)
  svntest.main.run_svn(None,
                       'cp', '-m', '', trunk_url, version1_url)
  svntest.main.run_svn(None,
                       'up', wc1_dir)
  svntest.main.file_append(wc1_new_file, "This is the file 'newfile'.\n")
  svntest.main.run_svn(None, 'add', wc1_new_file)
  svntest.main.run_svn(None, 'ci', '-m', '', wc1_dir)

  # Try to switch "wc2" to the branch (non-recursively)
  svntest.actions.run_and_verify_svn(None, None, [], 'switch', '-N',
                                     '--ignore-ancestry', version1_url, wc2_dir)

  # Check the URLs of the (not switched) directories.
  expected_infos = [
      { 'URL' : '.*/A/B$' },
      { 'URL' : '.*/A/C$' },
      { 'URL' : '.*/A/D$' },
    ]
  svntest.actions.run_and_verify_info(expected_infos,
                                      wc2_B_dir, wc2_C_dir, wc2_D_dir)

  # Check the URLs of the switched files.
  # ("svn status -u" might be a better check: it fails when newfile's URL
  # is bad, and shows "S" when mu's URL is wrong.)
  # mu: not switched
  expected_infos = [
      { 'URL' : '.*/branch/version1/mu$' },
      { 'URL' : '.*/branch/version1/newfile$' },   # newfile: wrong URL
    ]
  svntest.actions.run_and_verify_info(expected_infos,
                                      wc2_mu_file, wc2_new_file)


#----------------------------------------------------------------------
def failed_anchor_is_target(sbox):
  "anchor=target, try to replace a local-mod file"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Set up a switch from dir H, containing locally-modified file 'psi',
  # to dir G, containing a directory 'psi'. Expect a tree conflict.

  # Make a directory 'G/psi' in the repository.
  G_url = sbox.repo_url + '/A/D/G'
  G_psi_url = G_url + '/psi'
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'mkdir', '-m', 'log msg', G_psi_url)

  # Modify the file 'H/psi' locally.
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  psi_path = os.path.join(H_path, 'psi')
  svntest.main.file_append(psi_path, "more text")

  # This switch raises a tree conflict on 'psi', because of the local mods.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'switch', '--ignore-ancestry',
                                     G_url, H_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H', switched='S', wc_rev=2)
  expected_status.tweak('A/D/H/psi', status='R ', copied='+',
                        wc_rev='-', treeconflict='C')
  expected_status.remove('A/D/H/chi', 'A/D/H/omega')
  expected_status.add({
    'A/D/H/pi'      : Item(status='  ', wc_rev=2),
    'A/D/H/tau'     : Item(status='  ', wc_rev=2),
    'A/D/H/rho'     : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # There was a bug whereby the failed switch left the wrong URL in
  # the target directory H.  Check for that.
  expected_infos = [
      { 'URL' : '.*' + G_url + '$' },
    ]
  svntest.actions.run_and_verify_info(expected_infos, H_path)

  # Resolve tree conflict at psi.
  svntest.actions.run_and_verify_resolved([psi_path])

  # The switch should now be complete.
  ### Instead of "treeconflict=None" which means "don't check", we should
  # check "treeconflict=' '" but the test suite doesn't do the right thing.
  expected_status.tweak('A/D/H/psi', treeconflict=None)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
# Issue #1826 - svn switch temporarily drops invalid URLs into the entries
#               files (which become not-temporary if the switch fails).
def bad_intermediate_urls(sbox):
  "bad intermediate urls in use"
  sbox.build()
  wc_dir = sbox.wc_dir
  url = sbox.repo_url

  A = os.path.join(wc_dir, 'A')
  A_Z = os.path.join(wc_dir, 'A', 'Z')
  url_A_C = url + '/A/C'
  url_A_C_A = url + '/A/C/A'
  url_A_C_A_Z = url + '/A/C/A/Z'

  # We'll be switching our working copy to (a modified) A/C in the Greek tree.

  # First, make an extra subdirectory in C to match one in the root, plus
  # another one inside of that.
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'mkdir', '-m', 'log msg',
                                     url_A_C_A, url_A_C_A_Z)

  # Now, we'll drop a conflicting path under the root.
  svntest.main.file_append(A_Z, 'Look, Mom, a ... tree conflict.')

  #svntest.factory.make(sbox, """
  #  svn switch url/A/C wc_dir
  #  # svn info A
  #  # check that we can recover from the tree conflict
  #  rm A/Z
  #  svn up
  #  """)
  #exit(0)

  # svn switch url/A/C wc_dir
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'              : Item(status='D '),
    'A/Z'               : Item(status='  ', treeconflict='C'),
    'A/C'               : Item(status='D '),
    'A/B'               : Item(status='D '),
    'A/D'               : Item(status='D '),
    'iota'              : Item(status='D '),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota', 'A/B', 'A/B/E', 'A/B/E/beta', 'A/B/E/alpha',
    'A/B/F', 'A/B/lambda', 'A/D', 'A/D/G', 'A/D/G/rho', 'A/D/G/pi',
    'A/D/G/tau', 'A/D/H', 'A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi',
    'A/D/gamma', 'A/mu', 'A/C')
  expected_disk.add({
    'A/Z'               : Item(contents="Look, Mom, a ... tree conflict."),
  })

  expected_status = actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('iota', 'A/B', 'A/B/E', 'A/B/E/beta', 'A/B/E/alpha',
    'A/B/F', 'A/B/lambda', 'A/D', 'A/D/G', 'A/D/G/rho', 'A/D/G/pi',
    'A/D/G/tau', 'A/D/H', 'A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi',
    'A/D/gamma', 'A/mu', 'A/C')
  expected_status.add({
    # Obstructed node is currently turned into a delete to allow resolving.
    'A/Z'               : Item(status='D ', treeconflict='C', wc_rev=2),
  })

  actions.run_and_verify_switch(wc_dir, wc_dir, url_A_C, expected_output,
                                expected_disk, expected_status, None, None,
                                None, None, None, False, '--ignore-ancestry')

  # However, the URL for wc/A should now reflect ^/A/C/A, not something else.
  expected_infos = [
      { 'URL' : '.*/A/C/A$' },
    ]
  svntest.actions.run_and_verify_info(expected_infos, A)


  # check that we can recover from the tree conflict
  # rm A/Z
  os.remove(A_Z)
  svntest.main.run_svn(None, 'revert', A_Z)

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
  })

  expected_disk.tweak('A/Z', contents=None)

  expected_status.tweak(status='  ', wc_rev='2')
  expected_status.tweak('A/Z', treeconflict=None)

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)




#----------------------------------------------------------------------
# Regression test for issue #1825: failed switch may corrupt
# working copy
@Issue(1825)
def obstructed_switch(sbox):
  "obstructed switch"
  #svntest.factory.make(sbox, """svn cp -m msgcopy url/A/B/E url/A/B/Esave
  #                              svn rm A/B/E/alpha
  #                              svn commit
  #                              echo "hello" >> A/B/E/alpha
  #                              svn switch url/A/B/Esave A/B/E
  #                              svn status
  #                              svn info A/B/E/alpha""")
  sbox.build()
  wc_dir = sbox.wc_dir
  url = sbox.repo_url

  A_B_E = os.path.join(wc_dir, 'A', 'B', 'E')
  A_B_E_alpha = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  url_A_B_E = url + '/A/B/E'
  url_A_B_Esave = url + '/A/B/Esave'

  # svn cp -m msgcopy url/A/B/E url/A/B/Esave
  expected_stdout = verify.UnorderedOutput([
    '\n',
    'Committed revision 2.\n',
  ])

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'cp', '-m',
    'msgcopy', url_A_B_E, url_A_B_Esave)

  # svn rm A/B/E/alpha
  expected_stdout = ['D         ' + A_B_E_alpha + '\n']

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'rm',
    A_B_E_alpha)

  # svn commit
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha'       : Item(verb='Deleting'),
  })

  expected_status = actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha')

  actions.run_and_verify_commit(wc_dir, expected_output, expected_status,
    None, wc_dir)

  # echo "hello" >> A/B/E/alpha
  main.file_append(A_B_E_alpha, 'hello')

  # svn switch url/A/B/Esave A/B/E
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha'       : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/B/E/alpha', contents='hello')

  expected_status.add({
    'A/B/E/alpha'       : Item(status='D ', treeconflict='C', wc_rev=3),
  })
  expected_status.tweak('A/B/E', wc_rev='3', switched='S')
  expected_status.tweak('A/B/E/beta', wc_rev='3')

  actions.run_and_verify_switch(wc_dir, A_B_E, url_A_B_Esave,
                                expected_output, expected_disk,
                                expected_status, None, None, None, None,
                                None, False, '--ignore-ancestry')

  # svn status
  expected_status.add({
    'A/B/Esave'         : Item(status='  '),
    'A/B/Esave/beta'    : Item(status='  '),
    'A/B/Esave/alpha'   : Item(status='  '),
  })

  actions.run_and_verify_unquiet_status(wc_dir, expected_status)

  # svn info A/B/E/alpha
  expected_stdout = verify.RegexOutput(
                      ".*local unversioned, incoming add upon switch",
                      match_all=False)
  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'info',
    A_B_E_alpha)


#----------------------------------------------------------------------
# Issue 2353.
def commit_mods_below_switch(sbox):
  "commit with mods below switch"
  sbox.build()
  wc_dir = sbox.wc_dir

  C_path = os.path.join(wc_dir, 'A', 'C')
  B_url = sbox.repo_url + '/A/B'
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/E'       : Item(status='A '),
    'A/C/E/alpha' : Item(status='A '),
    'A/C/E/beta'  : Item(status='A '),
    'A/C/F'       : Item(status='A '),
    'A/C/lambda'  : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/C/E'       : Item(),
    'A/C/E/alpha' : Item(contents="This is the file 'alpha'.\n"),
    'A/C/E/beta'  : Item(contents="This is the file 'beta'.\n"),
    'A/C/F'       : Item(),
    'A/C/lambda'  : Item(contents="This is the file 'lambda'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/C', switched='S')
  expected_status.add({
    'A/C/E'       : Item(status='  ', wc_rev=1),
    'A/C/E/alpha' : Item(status='  ', wc_rev=1),
    'A/C/E/beta'  : Item(status='  ', wc_rev=1),
    'A/C/F'       : Item(status='  ', wc_rev=1),
    'A/C/lambda'  : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_switch(wc_dir, C_path, B_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

  D_path = os.path.join(wc_dir, 'A', 'D')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'x', 'x', C_path, D_path)

  expected_status.tweak('A/C', 'A/D', status=' M')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.wc.State(wc_dir, {
    'A/C' : Item(verb='Sending'),
    'A/D' : Item(verb='Sending'),
    })
  expected_status.tweak('A/C', 'A/D', status='  ', wc_rev=2)

  # A/C erroneously classified as a wc root caused the commit to fail
  # with "'A/C/E' is missing or not locked"
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, C_path, D_path)

#----------------------------------------------------------------------
# Issue 2306.
def refresh_read_only_attribute(sbox):
  "refresh the WC file system read-only attribute "

  # This test will fail when run as root. Since that's normal
  # behavior, just skip the test.
  if os.name == 'posix':
    if os.geteuid() == 0:
      raise svntest.Skip

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a branch.
  url = sbox.repo_url + '/A'
  branch_url = sbox.repo_url + '/A-branch'
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'cp', '-m', 'svn:needs-lock not set',
                                     url, branch_url)

  # Set the svn:needs-lock property on a file from the "trunk".
  A_path = os.path.join(wc_dir, 'A')
  mu_path = os.path.join(A_path, 'mu')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'svn:needs-lock', '1', mu_path)

  # Commit the propset of svn:needs-lock.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, mu_path)

  # The file on which svn:needs-lock was set is now expected to be read-only.
  if os.access(mu_path, os.W_OK):
    raise svntest.Failure("'%s' expected to be read-only after having had "
                          "its svn:needs-lock property set" % mu_path)

  # Switch to the branch with the WC state from before the propset of
  # svn:needs-lock.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(status=' U'),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.tweak('', wc_rev=1)
  expected_status.tweak('iota', wc_rev=1)
  expected_status.tweak('A', switched='S')
  svntest.actions.run_and_verify_switch(wc_dir, A_path, branch_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

  # The file with we set svn:needs-lock on should now be writable, but
  # is still read-only!
  if not os.access(mu_path, os.W_OK):
    raise svntest.Failure("'%s' expected to be writable after being switched "
                          "to a branch on which its svn:needs-lock property "
                          "is not set" % mu_path)

# Check that switch can't change the repository root.
def switch_change_repos_root(sbox):
  "switch shouldn't allow changing repos root"
  sbox.build()

  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url
  other_repo_url = repo_url

  # Strip trailing slashes and add something bogus to that other URL.
  while other_repo_url[-1] == '/':
    other_repos_url = other_repo_url[:-1]
  other_repo_url = other_repo_url + "_bogus"

  other_A_url = other_repo_url +  "/A"
  A_wc_dir = os.path.join(wc_dir, "A")

  # Test 1: A switch that changes to a non-existing repo shouldn't work.
  expected_err = ".*Unable to open repository.*|.*Could not open.*|"\
                 ".*No repository found.*"
  svntest.actions.run_and_verify_svn(None, None,
                                     expected_err,
                                     'switch', '--ignore-ancestry',
                                     other_A_url, A_wc_dir)

  # Test 2: A switch that changes the repo root part of the URL shouldn't work.
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  other_A_url = other_repo_url +  "/A"

  svntest.main.create_repos(other_repo_dir)
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*UUID.*",
                                     'switch', '--ignore-ancestry',
                                     other_A_url, A_wc_dir)

  # Make sure we didn't break the WC.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def forced_switch(sbox):
  "forced switch tolerates obstructions to adds"
  sbox.build(read_only = True)

  # Dir obstruction
  G_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G')
  os.mkdir(G_path)

  # Faux file obstructions
  shutil.copyfile(os.path.join(sbox.wc_dir, 'A', 'D', 'gamma'),
                  os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'gamma'))
  shutil.copyfile(os.path.join(sbox.wc_dir, 'A', 'D', 'G', 'tau'),
                  os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G', 'tau'))

  # Real file obstruction
  pi_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G', 'pi')
  svntest.main.file_write(pi_path,
                          "This is the OBSTRUCTING file 'pi'.\n")

  # Non-obstructing dir and file
  I_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'I')
  os.mkdir(I_path)
  upsilon_path = os.path.join(G_path, 'upsilon')
  svntest.main.file_write(upsilon_path,
                          "This is the unversioned file 'upsilon'.\n")

  # Setup expected results of switch.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    "A/B/F/gamma"   : Item(status='E '),
    "A/B/F/G"       : Item(status='E '),
    "A/B/F/G/pi"    : Item(status='E '),
    "A/B/F/G/rho"   : Item(status='A '),
    "A/B/F/G/tau"   : Item(status='E '),
    "A/B/F/H"       : Item(status='A '),
    "A/B/F/H/chi"   : Item(status='A '),
    "A/B/F/H/omega" : Item(status='A '),
    "A/B/F/H/psi"   : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    "A/B/F/gamma"     : Item("This is the file 'gamma'.\n"),
    "A/B/F/G"         : Item(),
    "A/B/F/G/pi"      : Item("This is the OBSTRUCTING file 'pi'.\n"),
    "A/B/F/G/rho"     : Item("This is the file 'rho'.\n"),
    "A/B/F/G/tau"     : Item("This is the file 'tau'.\n"),
    "A/B/F/G/upsilon" : Item("This is the unversioned file 'upsilon'.\n"),
    "A/B/F/H"         : Item(),
    "A/B/F/H/chi"     : Item("This is the file 'chi'.\n"),
    "A/B/F/H/omega"   : Item("This is the file 'omega'.\n"),
    "A/B/F/H/psi"     : Item("This is the file 'psi'.\n"),
    "A/B/F/I"         : Item(),
    })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak('A/B/F', switched='S')
  expected_status.add({
    "A/B/F/gamma"     : Item(status='  ', wc_rev=1),
    "A/B/F/G"         : Item(status='  ', wc_rev=1),
    "A/B/F/G/pi"      : Item(status='M ', wc_rev=1),
    "A/B/F/G/rho"     : Item(status='  ', wc_rev=1),
    "A/B/F/G/tau"     : Item(status='  ', wc_rev=1),
    "A/B/F/H"         : Item(status='  ', wc_rev=1),
    "A/B/F/H/chi"     : Item(status='  ', wc_rev=1),
    "A/B/F/H/omega"   : Item(status='  ', wc_rev=1),
    "A/B/F/H/psi"     : Item(status='  ', wc_rev=1),
    })

  # Do the switch and check the results in three ways.
  F_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F')
  AD_url = sbox.repo_url + '/A/D'
  svntest.actions.run_and_verify_switch(sbox.wc_dir, F_path, AD_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--force', '--ignore-ancestry')

#----------------------------------------------------------------------
def forced_switch_failures(sbox):
  "forced switch detects tree conflicts"
  #  svntest.factory.make(sbox,
  #    """
  #    # Add a directory to obstruct a file.
  #    mkdir A/B/F/pi
  #
  #    # Add a file to obstruct a directory.
  #    echo "The file 'H'" > A/C/H
  #
  #    # Test three cases where forced switch should cause a tree conflict
  #
  #    # 1) A forced switch that tries to add a file when an unversioned
  #    #    directory of the same name already exists.  (Currently fails)
  #    svn switch --force url/A/D A/C
  #
  #    # 2) A forced switch that tries to add a dir when a file of the same
  #    #    name already exists. (Tree conflict)
  #    svn switch --force url/A/D/G A/B/F
  #    svn info A/B/F/pi
  #
  #    # 3) A forced update that tries to add a directory when a versioned
  #    #    directory of the same name already exists.
  #
  #    # Make dir A/D/H/I in repos.
  #    svn mkdir -m "Log message" url/A/D/H/I
  #
  #    # Make A/D/G/I and co A/D/H/I into it.
  #    mkdir A/D/G/I
  #    svn co url/A/D/H/I A/D/G/I
  #
  #    # Try the forced switch.  A/D/G/I obstructs the dir A/D/G/I coming
  #    # from the repos, causing an error.
  #    svn switch --force url/A/D/H A/D/G
  #
  #    # Delete all three obstructions and finish the update.
  #    rm -rf A/D/G/I
  #    rm A/B/F/pi
  #    rm A/C/H
  #
  #    # A/B/F is switched to A/D/G
  #    # A/C is switched to A/D
  #    # A/D/G is switched to A/D/H
  #    svn up
  #    """)
  #  exit(0)
  sbox.build()
  wc_dir = sbox.wc_dir
  url = sbox.repo_url

  A_B_F = os.path.join(wc_dir, 'A', 'B', 'F')
  A_B_F_pi = os.path.join(wc_dir, 'A', 'B', 'F', 'pi')
  A_C = os.path.join(wc_dir, 'A', 'C')
  A_C_H = os.path.join(wc_dir, 'A', 'C', 'H')
  A_D_G = os.path.join(wc_dir, 'A', 'D', 'G')
  A_D_G_I = os.path.join(wc_dir, 'A', 'D', 'G', 'I')
  url_A_D = url + '/A/D'
  url_A_D_G = url + '/A/D/G'
  url_A_D_H = url + '/A/D/H'
  url_A_D_H_I = url + '/A/D/H/I'

  # Add a directory to obstruct a file.
  # mkdir A/B/F/pi
  os.makedirs(A_B_F_pi)

  # Add a file to obstruct a directory.
  # echo "The file 'H'" > A/C/H
  main.file_write(A_C_H, "The file 'H'\n")

  # Test three cases where forced switch should cause a tree conflict
  # 1) A forced switch that tries to add a file when an unversioned
  #    directory of the same name already exists.  (Currently fails)
  # svn switch --force url/A/D A/C
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/G'             : Item(status='A '),
    'A/C/G/pi'          : Item(status='A '),
    'A/C/G/rho'         : Item(status='A '),
    'A/C/G/tau'         : Item(status='A '),
    'A/C/gamma'         : Item(status='A '),
    'A/C/H'             : Item(status='  ', treeconflict='C'),
    'A/C/H/psi'         : Item(status='  ', treeconflict='A'),
    'A/C/H/omega'       : Item(status='  ', treeconflict='A'),
    'A/C/H/chi'         : Item(status='  ', treeconflict='A'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/C/gamma'         : Item(contents="This is the file 'gamma'.\n"),
    'A/C/G'             : Item(),
    'A/C/G/pi'          : Item(contents="This is the file 'pi'.\n"),
    'A/C/G/rho'         : Item(contents="This is the file 'rho'.\n"),
    'A/C/G/tau'         : Item(contents="This is the file 'tau'.\n"),
    'A/C/H'             : Item(contents="The file 'H'\n"),
    'A/B/F/pi'          : Item(),
  })

  expected_status = actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/C/G'             : Item(status='  ', wc_rev='1'),
    'A/C/G/rho'         : Item(status='  ', wc_rev='1'),
    'A/C/G/tau'         : Item(status='  ', wc_rev='1'),
    'A/C/G/pi'          : Item(status='  ', wc_rev='1'),
    'A/C/H'             : Item(status='D ', treeconflict='C', wc_rev='1'),
    'A/C/H/psi'         : Item(status='D ', wc_rev='1'),
    'A/C/H/omega'       : Item(status='D ', wc_rev='1'),
    'A/C/H/chi'         : Item(status='D ', wc_rev='1'),
    'A/C/gamma'         : Item(status='  ', wc_rev='1'),
  })
  expected_status.tweak('A/C', switched='S')

  actions.run_and_verify_switch(wc_dir, A_C, url_A_D, expected_output,
                                expected_disk, expected_status, None, None,
                                None, None, None, False, '--force',
                                '--ignore-ancestry')


  # 2) A forced switch that tries to add a dir when a file of the same
  #    name already exists. (Tree conflict)
  # svn switch --force url/A/D/G A/B/F
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F/rho'         : Item(status='A '),
    'A/B/F/pi'          : Item(status='  ', treeconflict='C'),
    'A/B/F/tau'         : Item(status='A '),
  })

  expected_disk.add({
    'A/B/F/rho'         : Item(contents="This is the file 'rho'.\n"),
    'A/B/F/tau'         : Item(contents="This is the file 'tau'.\n"),
  })

  expected_status.add({
    'A/B/F/tau'         : Item(status='  ', wc_rev='1'),
    'A/B/F/pi'          : Item(status='D ', treeconflict='C', wc_rev='1'),
    'A/B/F/rho'         : Item(status='  ', wc_rev='1'),
  })
  expected_status.tweak('A/B/F', switched='S')

  actions.run_and_verify_switch(wc_dir, A_B_F, url_A_D_G, expected_output,
                                expected_disk, expected_status, None, None,
                                None, None, None, False, '--force',
                                '--ignore-ancestry')

  # svn info A/B/F/pi
  expected_stdout = verify.ExpectedOutput(
    'Tree conflict: local unversioned, incoming add upon switch\n',
    match_all=False)

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'info',
    A_B_F_pi)


  # 3) A forced update that tries to add a directory when a versioned
  #    directory of the same name already exists.
  # Make dir A/D/H/I in repos.
  # svn mkdir -m "Log message" url/A/D/H/I
  expected_stdout = verify.UnorderedOutput([
    '\n',
    'Committed revision 2.\n',
  ])

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'mkdir',
    '-m', 'Log message', url_A_D_H_I)

  # Make A/D/G/I and co A/D/H/I into it.
  # mkdir A/D/G/I
  os.makedirs(A_D_G_I)

  # svn co url/A/D/H/I A/D/G/I
  expected_output = svntest.wc.State(wc_dir, {})

  expected_disk.add({
    'A/D/G/I'           : Item(),
  })

  exit_code, so, se = svntest.actions.run_and_verify_svn(
    "Unexpected error during co",
    ['Checked out revision 2.\n'], [],
    "co", url_A_D_H_I, A_D_G_I)

  # Try the forced switch.  A/D/G/I obstructs the dir A/D/G/I coming
  # from the repos, causing an error.
  # svn switch --force url/A/D/H A/D/G
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/chi'         : Item(status='A '),
    'A/D/G/tau'         : Item(status='D '),
    'A/D/G/omega'       : Item(status='A '),
    'A/D/G/psi'         : Item(status='A '),
    'A/D/G/I'           : Item(verb='Skipped'),
    'A/D/G/rho'         : Item(status='D '),
    'A/D/G/pi'          : Item(status='D '),
  })

  actions.run_and_verify_switch(wc_dir, A_D_G, url_A_D_H, expected_output,
                                None, None, None,
                                None, None, None, None,
                                False, '--force', '--ignore-ancestry')

  # Delete all three obstructions and finish the update.
  # rm -rf A/D/G/I
  main.safe_rmtree(A_D_G_I)

  # rm A/B/F/pi
  main.safe_rmtree(A_B_F_pi)

  # rm A/C/H
  os.remove(A_C_H)

  # Resolve the tree conflict on A_C_H and A_B_F_pi
  svntest.main.run_svn(None, 'resolved', A_C_H)
  svntest.main.run_svn(None, 'revert', A_B_F_pi)

  # A/B/F is switched to A/D/G
  # A/C is switched to A/D
  # A/D/G is switched to A/D/H
  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/H/I'           : Item(status='A '),
    'A/D/G/I'           : Item(status='A '),
    'A/D/H/I'           : Item(status='A '),
  })

  expected_disk.remove('A/D/G/tau', 'A/D/G/rho', 'A/D/G/pi')
  expected_disk.add({
    'A/D/H/I'           : Item(),
    'A/D/G/omega'       : Item(contents="This is the file 'omega'.\n"),
    'A/D/G/psi'         : Item(contents="This is the file 'psi'.\n"),
    'A/D/G/chi'         : Item(contents="This is the file 'chi'.\n"),
    'A/C/H/I'           : Item(),
    'A/C/H/omega'       : Item(contents="This is the file 'omega'.\n"),
    'A/C/H/psi'         : Item(contents="This is the file 'psi'.\n"),
    'A/C/H/chi'         : Item(contents="This is the file 'chi'.\n"),
  })
  expected_disk.tweak('A/C/H', contents=None)
  expected_disk.tweak('A/B/F/pi', contents="This is the file 'pi'.\n")

  expected_status.remove('A/D/G/tau', 'A/D/G/rho', 'A/D/G/pi')
  expected_status.add({
    'A/D/G/omega'       : Item(status='  ', wc_rev='2'),
    'A/D/G/I'           : Item(status='  ', wc_rev='2'),
    'A/D/G/psi'         : Item(status='  ', wc_rev='2'),
    'A/D/G/chi'         : Item(status='  ', wc_rev='2'),
    'A/D/H/I'           : Item(status='  ', wc_rev='2'),
    'A/C/H/psi'         : Item(status='  ', wc_rev='2'),
    'A/C/H/omega'       : Item(status='  ', wc_rev='2'),
    'A/C/H/chi'         : Item(status='  ', wc_rev='2'),
    'A/C/H/I'           : Item(status='  ', wc_rev='2'),
  })
  expected_status.tweak(wc_rev='2', status='  ')
  expected_status.tweak('A/B/F/pi', 'A/C/H', treeconflict=None)
  expected_status.tweak('A/D/G', switched='S')

  svntest.main.run_svn(None, 'revert', '-R', os.path.join(wc_dir, 'A/C/H'))

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)


def switch_with_obstructing_local_adds(sbox):
  "switch tolerates WC adds"
  sbox.build(read_only = True)

  # Dir obstruction scheduled for addition without history.
  G_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G')
  os.mkdir(G_path)

  # File obstructions scheduled for addition without history.
  # Contents identical to additions from switch.
  gamma_copy_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'gamma')
  shutil.copyfile(os.path.join(sbox.wc_dir, 'A', 'D', 'gamma'),
                  gamma_copy_path)
  shutil.copyfile(os.path.join(sbox.wc_dir, 'A', 'D', 'G', 'tau'),
                  os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G', 'tau'))

  # File obstruction scheduled for addition without history.
  # Contents conflict with addition from switch.
  pi_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'G', 'pi')
  svntest.main.file_write(pi_path,
                          "This is the OBSTRUCTING file 'pi'.\n")

  # Non-obstructing dir and file scheduled for addition without history.
  I_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F', 'I')
  os.mkdir(I_path)
  upsilon_path = os.path.join(G_path, 'upsilon')
  svntest.main.file_write(upsilon_path,
                          "This is the unversioned file 'upsilon'.\n")

  # Add the above obstructions.
  svntest.actions.run_and_verify_svn("Add error:", None, [],
                                     'add', G_path, I_path,
                                     gamma_copy_path)

  # Setup expected results of switch.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    "A/B/F/gamma"   : Item(status='  ', treeconflict='C'),
    "A/B/F/G"       : Item(status='  ', treeconflict='C'),
    'A/B/F/G/tau'   : Item(status='  ', treeconflict='A'),
    'A/B/F/G/rho'   : Item(status='  ', treeconflict='A'),
    'A/B/F/G/pi'    : Item(status='  ', treeconflict='A'),
    "A/B/F/H"       : Item(status='A '),
    "A/B/F/H/chi"   : Item(status='A '),
    "A/B/F/H/omega" : Item(status='A '),
    "A/B/F/H/psi"   : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    "A/B/F/gamma"     : Item("This is the file 'gamma'.\n"),
    "A/B/F/G"         : Item(),
    "A/B/F/G/pi"      : Item("This is the OBSTRUCTING file 'pi'.\n"),
    "A/B/F/G/tau"     : Item("This is the file 'tau'.\n"),
    "A/B/F/G/upsilon" : Item("This is the unversioned file 'upsilon'.\n"),
    "A/B/F/H"         : Item(),
    "A/B/F/H/chi"     : Item("This is the file 'chi'.\n"),
    "A/B/F/H/omega"   : Item("This is the file 'omega'.\n"),
    "A/B/F/H/psi"     : Item("This is the file 'psi'.\n"),
    "A/B/F/I"         : Item(),
    })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak('A/B/F', switched='S')
  expected_status.add({
    'A/B/F/gamma'     : Item(status='R ', treeconflict='C', wc_rev='1'),
    'A/B/F/G'         : Item(status='R ', treeconflict='C', wc_rev='1'),
    'A/B/F/G/pi'      : Item(status='A ', wc_rev='-'),
    'A/B/F/G/tau'     : Item(status='A ', wc_rev='-'),
    'A/B/F/G/upsilon' : Item(status='A ', wc_rev='-'),
    'A/B/F/G/rho'     : Item(status='D ', wc_rev='1'),
    'A/B/F/H'         : Item(status='  ', wc_rev='1'),
    'A/B/F/H/chi'     : Item(status='  ', wc_rev='1'),
    'A/B/F/H/omega'   : Item(status='  ', wc_rev='1'),
    'A/B/F/H/psi'     : Item(status='  ', wc_rev='1'),
    'A/B/F/I'         : Item(status='A ', wc_rev='-'),
  })

  # "Extra" files that we expect to result from the conflicts.
  extra_files = ['pi\.r0', 'pi\.r1', 'pi\.mine']

  # Do the switch and check the results in three ways.
  F_path = os.path.join(sbox.wc_dir, 'A', 'B', 'F')
  D_url = sbox.repo_url + '/A/D'

  svntest.actions.run_and_verify_switch(sbox.wc_dir, F_path, D_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files, None, None, False,
                                        '--ignore-ancestry')

#----------------------------------------------------------------------

def switch_scheduled_add(sbox):
  "switch a scheduled-add file"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  file_path = os.path.join(wc_dir, 'stub_file')
  switch_url = sbox.repo_url + '/iota'
  nodo_path = os.path.join(wc_dir, 'nodo')

  svntest.main.file_append(file_path, "")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', file_path)
  svntest.actions.run_and_verify_svn(None, None,
                                     "svn: E200007: Cannot switch '.*file' " +
                                     "because it is not in the repository yet",
                                     'switch', '--ignore-ancestry',
                                     switch_url, file_path)

  svntest.actions.run_and_verify_svn(None, None,
                                     "svn: E155010: The node '.*nodo' was not",
                                     'switch', '--ignore-ancestry',
                                     switch_url, nodo_path)

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
def mergeinfo_switch_elision(sbox):
  "mergeinfo does not elide post switch"

  # When a switch adds mergeinfo on a path which is identical to
  # the mergeinfo on one of the path's subtrees, the subtree's mergeinfo
  # should *not* elide!  If it did this could result in the switch of a
  # pristine tree producing local mods.

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  lambda_path   = os.path.join(wc_dir, "A", "B_COPY_1", "lambda")
  B_COPY_1_path = os.path.join(wc_dir, "A", "B_COPY_1")
  B_COPY_2_path = os.path.join(wc_dir, "A", "B_COPY_2")
  E_COPY_2_path = os.path.join(wc_dir, "A", "B_COPY_2", "E")
  alpha_path    = os.path.join(wc_dir, "A", "B", "E", "alpha")
  beta_path     = os.path.join(wc_dir, "A", "B", "E", "beta")

  # Make branches A/B_COPY_1 and A/B_COPY_2
  expected_stdout = verify.UnorderedOutput([
     "A    " + os.path.join(wc_dir, "A", "B_COPY_1", "lambda") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_1", "E") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_1", "E", "alpha") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_1", "E", "beta") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_1", "F") + "\n",
     "Checked out revision 1.\n",
     "A         " + B_COPY_1_path + "\n",
    ])
  svntest.actions.run_and_verify_svn(None, expected_stdout, [], 'copy',
                                     sbox.repo_url + "/A/B", B_COPY_1_path)

  expected_stdout = verify.UnorderedOutput([
     "A    " + os.path.join(wc_dir, "A", "B_COPY_2", "lambda") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_2", "E") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_2", "E", "alpha") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_2", "E", "beta") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY_2", "F") + "\n",
     "Checked out revision 1.\n",
     "A         " + B_COPY_2_path + "\n",
    ])
  svntest.actions.run_and_verify_svn(None, expected_stdout, [], 'copy',
                                     sbox.repo_url + "/A/B", B_COPY_2_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B_COPY_1' : Item(verb='Adding'),
    'A/B_COPY_2' : Item(verb='Adding')
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    "A/B_COPY_1"         : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/lambda"  : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/E"       : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/E/alpha" : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/E/beta"  : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/F"       : Item(status='  ', wc_rev=2),
    "A/B_COPY_2"         : Item(status='  ', wc_rev=2),
    "A/B_COPY_2/lambda"  : Item(status='  ', wc_rev=2),
    "A/B_COPY_2/E"       : Item(status='  ', wc_rev=2),
    "A/B_COPY_2/E/alpha" : Item(status='  ', wc_rev=2),
    "A/B_COPY_2/E/beta"  : Item(status='  ', wc_rev=2),
    "A/B_COPY_2/F"       : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Make some changes under A/B

  # r3 - modify and commit A/B/E/beta
  svntest.main.file_write(beta_path, "New content")
  expected_output = svntest.wc.State(wc_dir,
                                     {'A/B/E/beta' : Item(verb='Sending')})
  expected_status.tweak('A/B/E/beta', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # r4 - modify and commit A/B/E/alpha
  svntest.main.file_write(alpha_path, "New content")
  expected_output = svntest.wc.State(wc_dir,
                                     {'A/B/E/alpha' : Item(verb='Sending')})
  expected_status.tweak('A/B/E/alpha', wc_rev=4)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Merge r2:4 into A/B_COPY_1
  expected_output = svntest.wc.State(B_COPY_1_path, {
    'E/alpha' : Item(status='U '),
    'E/beta'  : Item(status='U '),
    })
  expected_mergeinfo_output = svntest.wc.State(B_COPY_1_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = svntest.wc.State(B_COPY_1_path, {
    })
  expected_merge_status = svntest.wc.State(B_COPY_1_path, {
    ''        : Item(status=' M', wc_rev=2),
    'lambda'  : Item(status='  ', wc_rev=2),
    'E'       : Item(status='  ', wc_rev=2),
    'E/alpha' : Item(status='M ', wc_rev=2),
    'E/beta'  : Item(status='M ', wc_rev=2),
    'F'       : Item(status='  ', wc_rev=2),
    })
  expected_merge_disk = svntest.wc.State('', {
    ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B:3-4'}),
    'lambda'  : Item("This is the file 'lambda'.\n"),
    'E'       : Item(),
    'E/alpha' : Item("New content"),
    'E/beta'  : Item("New content"),
    'F'       : Item(),
    })
  expected_skip = svntest.wc.State(B_COPY_1_path, { })
  svntest.actions.run_and_verify_merge(B_COPY_1_path, '2', '4',
                                       sbox.repo_url + '/A/B', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_merge_disk,
                                       expected_merge_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1)

  # r5 - Commit the merge into A/B_COPY_1/E
  expected_output = svntest.wc.State(
    wc_dir,
    {'A/B_COPY_1'         : Item(verb='Sending'),
     'A/B_COPY_1/E/alpha' : Item(verb='Sending'),
     'A/B_COPY_1/E/beta'  : Item(verb='Sending'),
     })
  expected_status.tweak('A/B_COPY_1',         wc_rev=5)
  expected_status.tweak('A/B_COPY_1/E/alpha', wc_rev=5)
  expected_status.tweak('A/B_COPY_1/E/beta',  wc_rev=5)
  expected_status.tweak('A/B_COPY_1/lambda',  wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Merge r2:4 into A/B_COPY_2/E
  expected_output = svntest.wc.State(E_COPY_2_path, {
    'alpha' : Item(status='U '),
    'beta'  : Item(status='U '),
    })
  expected_mergeinfo_output = svntest.wc.State(E_COPY_2_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = svntest.wc.State(E_COPY_2_path, {
    })
  expected_merge_status = svntest.wc.State(E_COPY_2_path, {
    ''      : Item(status=' M', wc_rev=2),
    'alpha' : Item(status='M ', wc_rev=2),
    'beta'  : Item(status='M ', wc_rev=2),
    })
  expected_merge_disk = svntest.wc.State('', {
    ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B/E:3-4'}),
    'alpha' : Item("New content"),
    'beta'  : Item("New content"),
    })
  expected_skip = svntest.wc.State(E_COPY_2_path, { })
  svntest.actions.run_and_verify_merge(E_COPY_2_path, '2', '4',
                                       sbox.repo_url + '/A/B/E', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_merge_disk,
                                       expected_merge_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1)

  # Switch A/B_COPY_2 to URL of A/B_COPY_1.  The local mergeinfo for r1,3-4
  # on A/B_COPY_2/E is identical to the mergeinfo added to A/B_COPY_2 as a
  # result of the switch, but we leave the former in place.

  # Setup expected results of switch.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    "A/B_COPY_2"         : Item(status=' U'),
    "A/B_COPY_2/E/alpha" : Item(status='G '),
    "A/B_COPY_2/E/beta"  : Item(status='G '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak("A/B/E/alpha", contents="New content")
  expected_disk.tweak("A/B/E/beta", contents="New content")
  expected_disk.add({
    "A/B_COPY_1"         : Item(props={SVN_PROP_MERGEINFO : '/A/B:3-4'}),
    "A/B_COPY_1/E"       : Item(),
    "A/B_COPY_1/F"       : Item(),
    "A/B_COPY_1/lambda"  : Item("This is the file 'lambda'.\n"),
    "A/B_COPY_1/E/alpha" : Item("New content"),
    "A/B_COPY_1/E/beta"  : Item("New content"),
    "A/B_COPY_2"         : Item(props={SVN_PROP_MERGEINFO : '/A/B:3-4'}),
    "A/B_COPY_2/E"       : Item(props={SVN_PROP_MERGEINFO : '/A/B/E:3-4'}),
    "A/B_COPY_2/F"       : Item(),
    "A/B_COPY_2/lambda"  : Item("This is the file 'lambda'.\n"),
    "A/B_COPY_2/E/alpha" : Item("New content"),
    "A/B_COPY_2/E/beta"  : Item("New content"),
    })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak("A/B/E/beta", wc_rev=3)
  expected_status.tweak("A/B/E/alpha", wc_rev=4)
  expected_status.add({
    "A/B_COPY_1"         : Item(status='  ', wc_rev=5),
    "A/B_COPY_1/E"       : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/F"       : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/lambda"  : Item(status='  ', wc_rev=2),
    "A/B_COPY_1/E/alpha" : Item(status='  ', wc_rev=5),
    "A/B_COPY_1/E/beta"  : Item(status='  ', wc_rev=5),
    "A/B_COPY_2"         : Item(status='  ', wc_rev=5, switched='S'),
    "A/B_COPY_2/E"       : Item(status=' M', wc_rev=5),
    "A/B_COPY_2/F"       : Item(status='  ', wc_rev=5),
    "A/B_COPY_2/lambda"  : Item(status='  ', wc_rev=5),
    "A/B_COPY_2/E/alpha" : Item(status='  ', wc_rev=5),
    "A/B_COPY_2/E/beta"  : Item(status='  ', wc_rev=5),
    })

  svntest.actions.run_and_verify_switch(sbox.wc_dir,
                                        B_COPY_2_path,
                                        sbox.repo_url + "/A/B_COPY_1",
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, True,
                                        '--ignore-ancestry')

  # Now check a switch which reverses and earlier switch and leaves
  # a path in an unswitched state.
  #
  # Switch A/B_COPY_1/lambda to iota.  Use propset to give A/B_COPY/lambda
  # the mergeinfo '/A/B/lambda:1,3-4'.  Then switch A/B_COPY_1/lambda back
  # to A/B_COPY_1/lambda.  The local mergeinfo for r1,3-4 should remain on
  # A/B_COPY_1/lambda.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    "A/B_COPY_1/lambda" : Item(status='U '),
    })
  expected_disk.tweak("A/B_COPY_1/lambda",
                      contents="This is the file 'iota'.\n")
  expected_status.tweak("A/B_COPY_1/lambda", wc_rev=5, switched='S')
  svntest.actions.run_and_verify_switch(sbox.wc_dir,
                                        lambda_path,
                                        sbox.repo_url + "/iota",
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, True,
                                        '--ignore-ancestry')

  svntest.actions.run_and_verify_svn(None,
                                     ["property '" + SVN_PROP_MERGEINFO +
                                      "' set on '" + lambda_path + "'" +
                                      "\n"], [], 'ps', SVN_PROP_MERGEINFO,
                                     '/A/B/lambda:3-4', lambda_path)

  expected_output = svntest.wc.State(sbox.wc_dir, {
    "A/B_COPY_1/lambda" : Item(status='U '),
    })
  expected_disk.tweak("A/B_COPY_1/lambda",
                      contents="This is the file 'lambda'.\n",
                      props={SVN_PROP_MERGEINFO : '/A/B/lambda:3-4'})
  expected_status.tweak("A/B_COPY_1/lambda", switched=None, status=' M')
  svntest.actions.run_and_verify_switch(sbox.wc_dir,
                                        lambda_path,
                                        sbox.repo_url + "/A/B_COPY_1/lambda",
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, True,
                                        '--ignore-ancestry')

#----------------------------------------------------------------------

def switch_with_depth(sbox):
  "basic tests to verify switch along with depth"

  sbox.build(read_only = True)

  # Form some paths and URLs required
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url
  AD_url = repo_url + '/A/D'
  AB_url = repo_url + '/A/B'
  AB_path = os.path.join(wc_dir, 'A', 'B')

  # Set up expected results of 'switch --depth=empty'
  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B', switched='S')
  expected_status.tweak('A/B/lambda', switched='S')
  expected_status.tweak('A/B/E', switched='S')
  expected_status.tweak('A/B/F', switched='S')

  # Do 'switch --depth=empty' and check the results in three ways.
  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AD_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--depth', 'empty', '--ignore-ancestry')

  # Set up expected results for reverting 'switch --depth=empty'
  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AB_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--depth', 'empty', '--ignore-ancestry')

  # Set up expected results of 'switch --depth=files'
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/lambda'  : Item(status='D '),
    'A/B/gamma'   : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/lambda')
  expected_disk.add({
    'A/B/gamma'   : Item("This is the file 'gamma'.\n")
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/lambda')
  expected_status.add({
    'A/B/gamma'   : Item(status='  ', wc_rev=1)
    })
  expected_status.tweak('A/B', switched='S')
  expected_status.tweak('A/B/E', switched='S')
  expected_status.tweak('A/B/F', switched='S')

  # Do 'switch --depth=files' and check the results in three ways.
  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AD_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--depth', 'files', '--ignore-ancestry')

  # Set up expected results for reverting 'switch --depth=files'
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/gamma'   : Item(status='D '),
    'A/B/lambda'  : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AB_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--depth', 'files', '--ignore-ancestry')

  # Putting the depth=immediates stuff in a subroutine, because we're
  # going to run it at least twice.
  def sw_depth_imm():
    # Set up expected results of 'switch --depth=immediates'
    expected_output = svntest.wc.State(wc_dir, {
        'A/B/lambda'  : Item(status='D '),
        'A/B/E'       : Item(status='D '),
        'A/B/F'       : Item(status='D '),
        'A/B/gamma'   : Item(status='A '),
        'A/B/G'       : Item(status='A '),
        'A/B/H'       : Item(status='A '),
        })
    expected_disk = svntest.main.greek_state.copy()
    expected_disk.remove('A/B/lambda', 'A/B/E/beta', 'A/B/E/alpha',
                         'A/B/E', 'A/B/F')
    expected_disk.add({
        'A/B/gamma'   : Item("This is the file 'gamma'.\n"),
        'A/B/G'       : Item(),
        'A/B/H'       : Item(),
        })
    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.remove('A/B/lambda', 'A/B/E/beta', 'A/B/E/alpha',
                           'A/B/E', 'A/B/F')
    expected_status.add({
        'A/B/gamma'   : Item(status='  ', wc_rev=1),
        'A/B/G'       : Item(status='  ', wc_rev=1),
        'A/B/H'       : Item(status='  ', wc_rev=1)
        })
    expected_status.tweak('A/B', switched='S')

    # Do 'switch --depth=immediates' and check the results in three ways.
    svntest.actions.run_and_verify_switch(wc_dir, AB_path, AD_url,
                                          expected_output,
                                          expected_disk,
                                          expected_status, None,
                                          None, None, None, None, False,
                                          '--depth', 'immediates',
                                          '--ignore-ancestry')

  sw_depth_imm()

  # Set up expected results for reverting 'switch --depth=immediates'.
  # (Reverting with default [infinite] depth, so that the result is a
  # standard Greek Tree working copy again.)
  expected_output = svntest.wc.State(wc_dir, {
      'A/B/gamma'   : Item(status='D '),
      'A/B/G'       : Item(status='D '),
      'A/B/H'       : Item(status='D '),
      'A/B/lambda'  : Item(status='A '),
      'A/B/E'       : Item(status='A '),
      'A/B/E/alpha' : Item(status='A '),
      'A/B/E/beta'  : Item(status='A '),
      'A/B/F'       : Item(status='A '),
      })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AB_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--ignore-ancestry')

  # Okay, repeat 'switch --depth=immediates'.  (Afterwards we'll
  # 'switch --depth=infinity', to test going all the way.)
  sw_depth_imm()

  # Set up expected results of 'switch --depth=infinity'
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/gamma'   : Item(status='D '),
    'A/B/G'       : Item(status='D '),
    'A/B/H'       : Item(status='D '),
    'A/B/lambda'  : Item(status='A '),
    'A/B/E'       : Item(status='A '),
    'A/B/E/alpha' : Item(status='A '),
    'A/B/E/beta'  : Item(status='A '),
    'A/B/F'       : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Do the 'switch --depth=infinity' and check the results in three ways.
  svntest.actions.run_and_verify_switch(wc_dir, AB_path, AB_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '--depth', 'infinity',
                                        '--ignore-ancestry')

#----------------------------------------------------------------------

def switch_to_dir_with_peg_rev(sbox):
  "switch to dir@peg where dir doesn't exist in HEAD"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # prepare two dirs X and Y in rev. 2
  X_path = os.path.join(wc_dir, 'X')
  Y_path = os.path.join(wc_dir, 'Y')
  svntest.main.run_svn(None, 'mkdir', X_path, Y_path)
  svntest.main.run_svn(None, 'ci',
                       '-m', 'log message',
                       wc_dir)

  # change tau in rev. 3
  ADG_path = os.path.join(wc_dir, 'A', 'D', 'G')
  tau_path = os.path.join(ADG_path, 'tau')
  svntest.main.file_append(tau_path, "new line\n")
  svntest.main.run_svn(None, 'ci',
                       '-m', 'log message',
                       wc_dir)

  # delete A/D/G in rev. 4
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'rm', ADG_path)
  svntest.main.run_svn(None, 'ci',
                       '-m', 'log message',
                       wc_dir)

  # Test 1: switch X to A/D/G@2
  ADG_url = repo_url + '/A/D/G'
  expected_output = svntest.wc.State(wc_dir, {
    'X/pi'   : Item(status='A '),
    'X/rho'   : Item(status='A '),
    'X/tau'   : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
      'X'           : Item(),
      'X/pi'        : Item("This is the file 'pi'.\n"),
      'X/rho'       : Item("This is the file 'rho'.\n"),
      'X/tau'       : Item("This is the file 'tau'.\n"),
      'Y'           : Item(),
      })
  expected_disk.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status.add({
      'X'           : Item(status='  ', wc_rev=2, switched='S'),
      'X/pi'        : Item(status='  ', wc_rev=2),
      'X/rho'       : Item(status='  ', wc_rev=2),
      'X/tau'       : Item(status='  ', wc_rev=2),
      'Y'           : Item(status='  ', wc_rev=3)
      })

  # Do the switch to rev. 2 of /A/D/G@3.
  svntest.actions.run_and_verify_switch(wc_dir, X_path, ADG_url + '@3',
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '-r', '2', '--ignore-ancestry')

def switch_urls_with_spaces(sbox):
  "switch file and dir to url containing spaces"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # add file and directory with spaces in their names.
  XYZ_path = os.path.join(wc_dir, 'X Y Z')
  ABC_path = os.path.join(wc_dir, 'A B C')
  svntest.main.run_svn(None, 'mkdir', XYZ_path, ABC_path)

  tpm_path = os.path.join(wc_dir, 'tau pau mau')
  bbb_path = os.path.join(wc_dir, 'bar baz bal')
  svntest.main.file_write(tpm_path, "This is the file 'tau pau mau'.\n")
  svntest.main.file_write(bbb_path, "This is the file 'bar baz bal'.\n")
  svntest.main.run_svn(None, 'add', tpm_path, bbb_path)

  svntest.main.run_svn(None, 'ci', '-m', 'log message', wc_dir)

  # Test 1: switch directory 'A B C' to url 'X Y Z'
  XYZ_url = repo_url + '/X Y Z'
  expected_output = svntest.wc.State(wc_dir, {
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
      'X Y Z'         : Item(),
      'A B C'         : Item(),
      'tau pau mau'   : Item("This is the file 'tau pau mau'.\n"),
      'bar baz bal'   : Item("This is the file 'bar baz bal'.\n"),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
      'X Y Z'         : Item(status='  ', wc_rev=2),
      'A B C'         : Item(status='  ', wc_rev=2, switched='S'),
      'tau pau mau'   : Item(status='  ', wc_rev=2),
      'bar baz bal'   : Item(status='  ', wc_rev=2),
      })

  svntest.actions.run_and_verify_switch(wc_dir, ABC_path, XYZ_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

  # Test 2: switch file 'bar baz bal' to 'tau pau mau'
  tpm_url = repo_url + '/tau pau mau'
  expected_output = svntest.wc.State(wc_dir, {
     'bar baz bal'    : Item(status='U '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
      'X Y Z'         : Item(),
      'A B C'         : Item(),
      'tau pau mau'   : Item("This is the file 'tau pau mau'.\n"),
      'bar baz bal'   : Item("This is the file 'tau pau mau'.\n"),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
      'X Y Z'         : Item(status='  ', wc_rev=2),
      'A B C'         : Item(status='  ', wc_rev=2, switched='S'),
      'tau pau mau'   : Item(status='  ', wc_rev=2),
      'bar baz bal'   : Item(status='  ', wc_rev=2, switched='S'),
      })

  svntest.actions.run_and_verify_switch(wc_dir, bbb_path, tpm_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

def switch_to_dir_with_peg_rev2(sbox):
  "switch to old rev of now renamed branch"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # prepare dir X in rev. 2
  X_path = os.path.join(wc_dir, 'X')
  svntest.main.run_svn(None, 'mkdir', X_path)
  svntest.main.run_svn(None, 'ci',
                       '-m', 'log message',
                       wc_dir)

  # make a change in ADG in rev. 3
  tau_path = os.path.join(wc_dir, 'A', 'D', 'G', 'tau')
  svntest.main.file_append(tau_path, "extra line\n")
  svntest.main.run_svn(None, 'ci', '-m', 'log message', wc_dir)

  # Rename ADG to ADY in rev 4
  svntest.main.run_svn(None, 'up', wc_dir)
  ADG_path = os.path.join(wc_dir, 'A', 'D', 'G')
  ADY_path = os.path.join(wc_dir, 'A', 'D', 'Y')
  svntest.main.run_svn(None, 'mv', ADG_path, ADY_path)
  svntest.main.run_svn(None, 'ci',
                       '-m', 'log message',
                       wc_dir)

  # Test switch X to rev 2 of A/D/Y@HEAD
  ADY_url = sbox.repo_url + '/A/D/Y'
  expected_output = svntest.wc.State(wc_dir, {
    'X/pi'   : Item(status='A '),
    'X/rho'   : Item(status='A '),
    'X/tau'   : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
      'X'         : Item(),
      'X/pi'  : Item("This is the file 'pi'.\n"),
      'X/rho' : Item("This is the file 'rho'.\n"),
      'X/tau' : Item("This is the file 'tau'.\n"),
      'A/D/Y'     : Item(),
      'A/D/Y/pi'  : Item("This is the file 'pi'.\n"),
      'A/D/Y/rho' : Item("This is the file 'rho'.\n"),
      'A/D/Y/tau' : Item("This is the file 'tau'.\nextra line\n"),
      })
  expected_disk.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status.add({
      'X'           : Item(status='  ', wc_rev=2, switched='S'),
      'X/pi'        : Item(status='  ', wc_rev=2),
      'X/rho'       : Item(status='  ', wc_rev=2),
      'X/tau'       : Item(status='  ', wc_rev=2),
      'A/D/Y'           : Item(status='  ', wc_rev=4),
      'A/D/Y/pi'        : Item(status='  ', wc_rev=4),
      'A/D/Y/rho'       : Item(status='  ', wc_rev=4),
      'A/D/Y/tau'       : Item(status='  ', wc_rev=4),
      })

  svntest.actions.run_and_verify_switch(wc_dir, X_path, ADY_url + '@HEAD',
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, False,
                                        '-r', '2', '--ignore-ancestry')

def switch_to_root(sbox):
  "switch a folder to the root of its repository"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  ADG_path = os.path.join(wc_dir, 'A', 'D', 'G')

  # Test switch /A/D/G to /
  AD_url = sbox.repo_url + '/A/D'
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi'          : Item(status='D '),
    'A/D/G/rho'         : Item(status='D '),
    'A/D/G/tau'         : Item(status='D '),
    'A/D/G/A'           : Item(status='A '),
    'A/D/G/A/B'         : Item(status='A '),
    'A/D/G/A/B/lambda'  : Item(status='A '),
    'A/D/G/A/B/E'       : Item(status='A '),
    'A/D/G/A/B/E/alpha' : Item(status='A '),
    'A/D/G/A/B/E/beta'  : Item(status='A '),
    'A/D/G/A/B/F'       : Item(status='A '),
    'A/D/G/A/mu'        : Item(status='A '),
    'A/D/G/A/C'         : Item(status='A '),
    'A/D/G/A/D'         : Item(status='A '),
    'A/D/G/A/D/gamma'   : Item(status='A '),
    'A/D/G/A/D/G'       : Item(status='A '),
    'A/D/G/A/D/G/pi'    : Item(status='A '),
    'A/D/G/A/D/G/rho'   : Item(status='A '),
    'A/D/G/A/D/G/tau'   : Item(status='A '),
    'A/D/G/A/D/H'       : Item(status='A '),
    'A/D/G/A/D/H/chi'   : Item(status='A '),
    'A/D/G/A/D/H/omega' : Item(status='A '),
    'A/D/G/A/D/H/psi'   : Item(status='A '),
    'A/D/G/iota'        : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_disk.add_state('A/D/G', svntest.main.greek_state.copy())

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status.add_state('A/D/G',
                            svntest.actions.get_virginal_state(wc_dir, 1))
  expected_status.tweak('A/D/G', switched = 'S')
  svntest.actions.run_and_verify_switch(wc_dir, ADG_path, sbox.repo_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

#----------------------------------------------------------------------
# Make sure that switch continue after deleting locally modified
# directories, as it update and merge do.

def tolerate_local_mods(sbox):
  "tolerate deletion of a directory with local mods"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  L_path = os.path.join(A_path, 'L')
  LM_path = os.path.join(L_path, 'local_mod')
  A_url = sbox.repo_url + '/A'
  A2_url = sbox.repo_url + '/A2'

  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'cp', '-m', 'make copy', A_url, A2_url)

  os.mkdir(L_path)
  svntest.main.run_svn(None, 'add', L_path)
  svntest.main.run_svn(None, 'ci', '-m', 'Commit added folder', wc_dir)

  # locally modified unversioned file
  svntest.main.file_write(LM_path, 'Locally modified file.\n', 'w+')

  expected_output = svntest.wc.State(wc_dir, {
    'A/L' : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/L' : Item(),
    'A/L/local_mod' : Item(contents='Locally modified file.\n'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.tweak('', 'iota', wc_rev=1)
  expected_status.tweak('A', switched='S')

  # Used to fail with locally modified or unversioned files
  svntest.actions.run_and_verify_switch(wc_dir, A_path, A2_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')

#----------------------------------------------------------------------

# Detect tree conflicts among files and directories,
# edited or deleted in a deep directory structure.
#
# See use cases 1-3 in notes/tree-conflicts/use-cases.txt for background.
# Note that we do not try to track renames.  The only difference from
# the behavior of Subversion 1.4 and 1.5 is the conflicted status of the
# parent directory.

# convenience definitions
leaf_edit = svntest.actions.deep_trees_leaf_edit
tree_del = svntest.actions.deep_trees_tree_del
leaf_del = svntest.actions.deep_trees_leaf_del

disk_after_leaf_edit = svntest.actions.deep_trees_after_leaf_edit
disk_after_leaf_del = svntest.actions.deep_trees_after_leaf_del
disk_after_tree_del = svntest.actions.deep_trees_after_tree_del

disk_empty_dirs = svntest.actions.deep_trees_empty_dirs

deep_trees_conflict_output = svntest.actions.deep_trees_conflict_output
deep_trees_conflict_output_skipped = \
    svntest.actions.deep_trees_conflict_output_skipped
deep_trees_status_local_tree_del = \
    svntest.actions.deep_trees_status_local_tree_del
deep_trees_status_local_leaf_edit = \
    svntest.actions.deep_trees_status_local_leaf_edit

DeepTreesTestCase = svntest.actions.DeepTreesTestCase

j = os.path.join


def tree_conflicts_on_switch_1_1(sbox):
  "tree conflicts 1.1: tree del, leaf edit on switch"

  sbox.build()

  # use case 1, as in notes/tree-conflicts/use-cases.txt
  # 1.1) local tree delete, incoming leaf edit

  expected_output = deep_trees_conflict_output.copy()
  expected_output.add({
    'DDD/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3'      : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3/zeta' : Item(status='  ', treeconflict='A'),
    'DD/D1/D2'          : Item(status='  ', treeconflict='U'),
    'DD/D1/D2/epsilon'  : Item(status='  ', treeconflict='A'),
    'DF/D1/beta'        : Item(status='  ', treeconflict='U'),
    'D/D1/delta'        : Item(status='  ', treeconflict='A'),
    'DDF/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDF/D1/D2/gamma'   : Item(status='  ', treeconflict='U')
  })

  expected_disk = disk_empty_dirs.copy()
  if  svntest.main.wc_is_singledb(sbox.wc_dir):
    expected_disk.remove('D/D1', 'DF/D1', 'DD/D1', 'DD/D1/D2',
                         'DDF/D1', 'DDF/D1/D2',
                         'DDD/D1', 'DDD/D1/D2', 'DDD/D1/D2/D3')

  # The files delta, epsilon, and zeta are incoming additions, but since
  # they are all within locally deleted trees they should also be schedule
  # for deletion.
  expected_status = deep_trees_status_local_tree_del.copy()
  expected_status.add({
    'D/D1/delta'        : Item(status='D '),
    'DD/D1/D2/epsilon'  : Item(status='D '),
    'DDD/D1/D2/D3/zeta' : Item(status='D '),
    })
  expected_status.tweak('', switched='S')

  # Update to the target rev.
  expected_status.tweak(wc_rev=3)

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .file.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .dir.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .dir.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .dir.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .dir.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .dir.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_switch(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_leaf_edit",
                        tree_del,
                        leaf_edit,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


@Issue(3334)
def tree_conflicts_on_switch_1_2(sbox):
  "tree conflicts 1.2: tree del, leaf del on switch"

  sbox.build()

  # 1.2) local tree delete, incoming leaf delete

  expected_output = deep_trees_conflict_output.copy()
  expected_output.add({
    'DD/D1/D2'          : Item(status='  ', treeconflict='D'),
    'DDF/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDF/D1/D2/gamma'   : Item(status='  ', treeconflict='D'),
    'DDD/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3'      : Item(status='  ', treeconflict='D'),
    'DF/D1/beta'        : Item(status='  ', treeconflict='D'),
  })

  expected_disk = disk_empty_dirs.copy()

  expected_status = deep_trees_status_local_tree_del.copy()

  # Expect the incoming leaf deletes to actually occur.  Even though they
  # are within (or in the case of F/alpha and D/D1 are the same as) the
  # trees locally scheduled for deletion we must still delete them and
  # update the scheduled for deletion items to the target rev.  Otherwise
  # once the conflicts are resolved we still have a mixed-rev WC we can't
  # commit without updating...which, you guessed it, raises tree conflicts
  # again, repeat ad infinitum - see issue #3334.
  #
  # Update to the target rev.
  expected_status.tweak(wc_rev=3)
  expected_status.tweak('F/alpha',
                        'D/D1',
                        status='! ', wc_rev=None)
  expected_status.tweak('', switched='S')
  # Remove the incoming deletes from status and disk.
  expected_status.remove('DD/D1/D2',
                         'DDD/D1/D2/D3',
                         'DDF/D1/D2/gamma',
                         'DF/D1/beta')
  ### Why does the deep trees state not include files?
  expected_disk.remove('D/D1',
                       'DD/D1/D2',
                       'DDD/D1/D2/D3')
  if svntest.main.wc_is_singledb(sbox.wc_dir):
    expected_disk.remove('DF/D1', 'DD/D1',
                         'DDF/D1', 'DDF/D1/D2',
                         'DDD/D1', 'DDD/D1/D2')

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .dir.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .dir.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .dir.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon switch'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .dir.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_switch(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_leaf_del",
                        tree_del,
                        leaf_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


@Issue(3334)
def tree_conflicts_on_switch_2_1(sbox):
  "tree conflicts 2.1: leaf edit, tree del on switch"

  # use case 2, as in notes/tree-conflicts/use-cases.txt
  # 2.1) local leaf edit, incoming tree delete

  expected_output = deep_trees_conflict_output

  expected_disk = disk_after_leaf_edit.copy()

  expected_status = deep_trees_status_local_leaf_edit.copy()

  # The expectation on 'alpha' reflects partial progress on issue #3334.
  expected_status.tweak('D/D1',
                        'F/alpha',
                        'DD/D1',
                        'DF/D1',
                        'DDD/D1',
                        'DDF/D1',
                        status='A ', copied='+', wc_rev='-')
  # See the status of all the paths *under* the above six subtrees.  Only the
  # roots of the added subtrees show as schedule 'A', these childs paths show
  # only that history is scheduled with the commit.
  expected_status.tweak(
    'DD/D1/D2',
    'DDD/D1/D2',
    'DDD/D1/D2/D3',
    'DF/D1/beta',
    'DDF/D1/D2',
    'DDF/D1/D2/gamma',
    copied='+', wc_rev='-')
  expected_status.tweak('', switched='S')

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon switch'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  ### D/D1/delta is locally-added during leaf_edit. when tree_del executes,
  ### it will delete D/D1, and the switch reschedules local D/D1 for
  ### local-copy from its original revision. however, right now, we cannot
  ### denote that delta is a local-add rather than a child of that D/D1 copy.
  ### thus, it appears in the status output as a (M)odified child.
  svntest.actions.deep_trees_run_tests_scheme_for_switch(sbox,
    [ DeepTreesTestCase("local_leaf_edit_incoming_tree_del",
                        leaf_edit,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


def tree_conflicts_on_switch_2_2(sbox):
  "tree conflicts 2.2: leaf del, tree del on switch"

  # 2.2) local leaf delete, incoming tree delete

  ### Current behaviour fails to show conflicts when deleting
  ### a directory tree that has modifications. (Will be solved
  ### when dirs_same_p() is implemented)
  expected_output = deep_trees_conflict_output

  expected_disk = disk_empty_dirs.copy()

  expected_status = svntest.actions.deep_trees_virginal_state.copy()
  expected_status.add({'' : Item(),
                       'F/alpha' : Item()})
  expected_status.tweak(contents=None, status='  ', wc_rev=3)
  expected_status.tweak('', switched='S')

  # Expect the incoming tree deletes and the local leaf deletes to mean
  # that all deleted paths are *really* gone, not simply scheduled for
  # deletion.
  expected_status.tweak('F/alpha',
                        'D/D1',
                        'DD/D1',
                        'DF/D1',
                        'DDD/D1',
                        'DDF/D1',
                        status='! ', treeconflict='C', wc_rev=None)
  # Remove from expected status and disk everything below the deleted paths.
  expected_status.remove('DD/D1/D2',
                         'DF/D1/beta',
                         'DDD/D1/D2',
                         'DDD/D1/D2/D3',
                         'DDF/D1/D2',
                         'DDF/D1/D2/gamma',)
  expected_disk.remove('D/D1',
                       'DD/D1',
                       'DD/D1/D2',
                       'DF/D1',
                       'DDD/D1',
                       'DDD/D1/D2',
                       'DDD/D1/D2/D3',
                       'DDF/D1',
                       'DDF/D1/D2',)

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_switch(sbox,
    [ DeepTreesTestCase("local_leaf_del_incoming_tree_del",
                        leaf_del,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


def tree_conflicts_on_switch_3(sbox):
  "tree conflicts 3: tree del, tree del on switch"

  # use case 3, as in notes/tree-conflicts/use-cases.txt
  # local tree delete, incoming tree delete

  expected_output = deep_trees_conflict_output

  expected_disk = disk_empty_dirs.copy()

  expected_status = deep_trees_status_local_tree_del.copy()
  expected_status.tweak('', switched='S')

  # Expect the incoming tree deletes and the local tree deletes to mean
  # that all deleted paths are *really* gone, not simply scheduled for
  # deletion.
  expected_status.tweak('F/alpha',
                        'D/D1',
                        'DD/D1',
                        'DF/D1',
                        'DDD/D1',
                        'DDF/D1',
                        status='! ', wc_rev=None)
  # Remove from expected status and disk everything below the deleted paths.
  expected_status.remove('DD/D1/D2',
                         'DF/D1/beta',
                         'DDD/D1/D2',
                         'DDD/D1/D2/D3',
                         'DDF/D1/D2',
                         'DDF/D1/D2/gamma',)
  expected_disk.remove('D/D1',
                       'DD/D1',
                       'DD/D1/D2',
                       'DF/D1',
                       'DDD/D1',
                       'DDD/D1/D2',
                       'DDD/D1/D2/D3',
                       'DDF/D1',
                       'DDF/D1/D2',)

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon switch'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_switch(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_tree_del",
                        tree_del,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )

def copy_with_switched_subdir(sbox):
  "copy directory with switched subdir"
  sbox.build()
  wc_dir = sbox.wc_dir
  D = os.path.join(wc_dir, 'A/D')
  G = os.path.join(D, 'G')

  E_url = sbox.repo_url + '/A/B/E'
  R = os.path.join(wc_dir, 'R')

  state = svntest.actions.get_virginal_state(wc_dir, 1)

  # Verify before switching
  svntest.actions.run_and_verify_status(wc_dir, state)

  # Switch A/D/G
  svntest.actions.run_and_verify_svn(None, None, [], 'switch',
                                     '--ignore-ancestry', E_url, G)

  state.tweak('A/D/G', switched='S')
  state.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau');
  state.add({
    'A/D/G/alpha' : Item(status='  ', wc_rev=1),
    'A/D/G/beta' : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_status(wc_dir, state)

  # And now copy A/D and everything below it to R
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', D, R)

  state.add({
    'R'         : Item(status='A ', copied='+', wc_rev='-'),
    'R/gamma'   : Item(status='  ', copied='+', wc_rev='-'),
    'R/G/alpha' : Item(status='  ', copied='+', wc_rev='-'),
    'R/G/beta'  : Item(status='  ', copied='+', wc_rev='-'),
    'R/H'       : Item(status='  ', copied='+', wc_rev='-'),
    'R/H/chi'   : Item(status='  ', copied='+', wc_rev='-'),
    'R/H/omega' : Item(status='  ', copied='+', wc_rev='-'),
    'R/H/psi'   : Item(status='  ', copied='+', wc_rev='-'),
    'R/G'       : Item(status='A ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, state)

  svntest.main.run_svn(None, 'ci', '-m', 'Commit added folder', wc_dir)

  # Additional test, it should commit to R/G/alpha.
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.file_append(os.path.join(wc_dir, 'R/G/alpha'), "apple")
  svntest.main.run_svn(None, 'ci', '-m', 'Commit changed file', wc_dir)

  # Checkout working copy to verify result
  svntest.main.safe_rmtree(wc_dir, 1)
  svntest.actions.run_and_verify_svn(None,
                                     None, [],
                                     'checkout',
                                     sbox.repo_url, wc_dir)

  # Switch A/D/G again to recreate state
  svntest.actions.run_and_verify_svn(None, None, [], 'switch',
                                     '--ignore-ancestry', E_url, G)

  # Clear the statuses
  state.tweak(status='  ', copied=None, wc_rev='3', entry_status=None)
  # But reset the switched state
  state.tweak('A/D/G', switched='S')

  svntest.actions.run_and_verify_status(wc_dir, state)

@Issue(3871)
def up_to_old_rev_with_subtree_switched_to_root(sbox):
  "up to old rev with subtree switched to root"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about.
  A_path = os.path.join(wc_dir, 'A')
  branch_path = os.path.join(wc_dir, 'branch')

  # Starting with a vanilla greek tree, create a branch of A, switch
  # that branch to the root of the repository, then update the WC to
  # r1.
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', A_path,
                                     branch_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', wc_dir,
                                     '-m', 'Create a branch')
  svntest.actions.run_and_verify_svn(None, None, [], 'sw', sbox.repo_url,
                                     branch_path, '--ignore-ancestry')

  # Now update the WC to r1.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r1', wc_dir)

def different_node_kind(sbox):
  "switch to a different node kind"
  sbox.build(read_only = True)
  os.chdir(sbox.wc_dir)
  sbox.wc_dir = ''

  pristine_disk = svntest.main.greek_state
  pristine_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_disk = pristine_disk.copy()
  expected_status = pristine_status.copy()

  def switch_to_dir(sbox, rel_url, rel_path):
    full_url = sbox.repo_url + '/' + rel_url
    full_path = sbox.ospath(rel_path)
    expected_disk.remove(rel_path)
    expected_disk.add({ rel_path : pristine_disk.desc[rel_url] })
    expected_disk.add_state(rel_path, pristine_disk.subtree(rel_url))
    expected_status.tweak(rel_path, switched='S')
    expected_status.add_state(rel_path, pristine_status.subtree(rel_url))
    svntest.actions.run_and_verify_switch(sbox.wc_dir, full_path, full_url,
                                          None, expected_disk, expected_status,
                                          None, None, None, None, None, False,
                                          '--ignore-ancestry')
    svntest.actions.run_and_verify_svn(None, None, [], 'info', full_path)
    if not os.path.isdir(full_path):
      raise svntest.Failure

  def switch_to_file(sbox, rel_url, rel_path):
    full_url = sbox.repo_url + '/' + rel_url
    full_path = sbox.ospath(rel_path)
    expected_disk.remove_subtree(rel_path)
    expected_disk.add({ rel_path : pristine_disk.desc[rel_url] })
    expected_status.remove_subtree(rel_path)
    expected_status.add({ rel_path : pristine_status.desc[rel_url] })
    expected_status.tweak(rel_path, switched='S')
    svntest.actions.run_and_verify_switch(sbox.wc_dir, full_path, full_url,
                                          None, expected_disk, expected_status,
                                          None, None, None, None, None, False,
                                          '--ignore-ancestry')
    svntest.actions.run_and_verify_svn(None, None, [], 'info', full_path)
    if not os.path.isfile(full_path):
      raise svntest.Failure

  # Switch two files to dirs and two dirs to files.
  # 'A/C' is an empty dir; 'A/D/G' is a non-empty dir.
  switch_to_dir(sbox, 'A/C', 'iota')
  switch_to_dir(sbox, 'A/D/G', 'A/D/gamma')
  switch_to_file(sbox, 'iota', 'A/C')
  switch_to_file(sbox, 'A/D/gamma', 'A/D/G')

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              routine_switching,
              commit_switched_things,
              full_update,
              full_rev_update,
              update_switched_things,
              rev_update_switched_things,
              log_switched_file,
              delete_subdir,
              file_dir_file,
              nonrecursive_switching,
              failed_anchor_is_target,
              bad_intermediate_urls,
              obstructed_switch,
              commit_mods_below_switch,
              refresh_read_only_attribute,
              switch_change_repos_root,
              forced_switch,
              forced_switch_failures,
              switch_scheduled_add,
              mergeinfo_switch_elision,
              switch_with_obstructing_local_adds,
              switch_with_depth,
              switch_to_dir_with_peg_rev,
              switch_urls_with_spaces,
              switch_to_dir_with_peg_rev2,
              switch_to_root,
              tolerate_local_mods,
              tree_conflicts_on_switch_1_1,
              tree_conflicts_on_switch_1_2,
              tree_conflicts_on_switch_2_1,
              tree_conflicts_on_switch_2_2,
              tree_conflicts_on_switch_3,
              copy_with_switched_subdir,
              up_to_old_rev_with_subtree_switched_to_root,
              different_node_kind,
              ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
