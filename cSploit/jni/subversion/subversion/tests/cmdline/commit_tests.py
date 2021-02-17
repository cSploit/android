#!/usr/bin/env python
#
#  commit_tests.py:  testing fancy commit cases.
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
import sys, os, re

# Our testing module
import svntest
from svntest import wc

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem

from svntest.main import server_has_revprop_commit, \
    server_gets_client_capabilities
from svntest.actions import inject_conflict_into_wc

######################################################################
# Utilities
#

def is_non_posix_os_or_cygwin_platform():
  return (not svntest.main.is_posix_os()) or sys.platform == 'cygwin'

def get_standard_state(wc_dir):
  """Return a status list reflecting the local mods made by
  make_standard_slew_of_changes()."""

  state = svntest.actions.get_virginal_state(wc_dir, 1)

  state.tweak('', 'A/D', 'A/D/G/pi', status=' M')
  state.tweak('A/B/lambda', status='M ')
  state.tweak('A/B/E', 'A/D/H/chi', status='R ')
  state.tweak('A/B/E/alpha', 'A/B/E/beta', 'A/C', 'A/D/gamma',
              'A/D/G/rho', status='D ')
  state.tweak('A/D/H/omega', status='MM')

  # New things
  state.add({
    'Q' : Item(status='A ', wc_rev=0),
    'Q/floo' : Item(status='A ', wc_rev=0),
    'A/D/H/gloo' : Item(status='A ', wc_rev=0),
    'A/B/E/bloo' : Item(status='A ', wc_rev=0),
    })

  return state


def make_standard_slew_of_changes(wc_dir):
  """Make a specific set of local mods to WC_DIR.  These will be used
  by every commit-test.  Verify the 'svn status' output, and return the
  (pre-commit) status tree."""

  # Cache current working directory, move into wc_dir
  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  # Add a directory
  os.mkdir('Q')
  svntest.main.run_svn(None, 'add', 'Q')

  # Remove two directories
  A_B_E = os.path.join('A', 'B', 'E')
  svntest.main.run_svn(None, 'rm', A_B_E)
  svntest.main.run_svn(None, 'rm', os.path.join('A', 'C'))

  # Replace one of the removed directories
  # But first recreate if it doesn't exist (single-db)
  if not os.path.exists(A_B_E):
    os.mkdir(A_B_E)
  svntest.main.run_svn(None, 'add', A_B_E)

  # Make property mods to two directories
  svntest.main.run_svn(None, 'propset', 'foo', 'bar', os.curdir)
  svntest.main.run_svn(None, 'propset', 'foo2', 'bar2', os.path.join('A', 'D'))

  # Add three files
  svntest.main.file_append(os.path.join('A', 'B', 'E', 'bloo'), "hi")
  svntest.main.file_append(os.path.join('A', 'D', 'H', 'gloo'), "hello")
  svntest.main.file_append(os.path.join('Q', 'floo'), "yo")
  svntest.main.run_svn(None, 'add', os.path.join('A', 'B', 'E', 'bloo'))
  svntest.main.run_svn(None, 'add', os.path.join('A', 'D', 'H', 'gloo'))
  svntest.main.run_svn(None, 'add', os.path.join('Q', 'floo'))

  # Remove three files
  svntest.main.run_svn(None, 'rm', os.path.join('A', 'D', 'G', 'rho'))
  svntest.main.run_svn(None, 'rm', os.path.join('A', 'D', 'H', 'chi'))
  svntest.main.run_svn(None, 'rm', os.path.join('A', 'D', 'gamma'))

  # Replace one of the removed files
  svntest.main.file_append(os.path.join('A', 'D', 'H', 'chi'), "chi")
  svntest.main.run_svn(None, 'add', os.path.join('A', 'D', 'H', 'chi'))

  # Make textual mods to two files
  svntest.main.file_append(os.path.join('A', 'B', 'lambda'), "new ltext")
  svntest.main.file_append(os.path.join('A', 'D', 'H', 'omega'), "new otext")

  # Make property mods to three files
  svntest.main.run_svn(None, 'propset', 'blue', 'azul',
                       os.path.join('A', 'D', 'H', 'omega'))
  svntest.main.run_svn(None, 'propset', 'green', 'verde',
                       os.path.join('Q', 'floo'))
  svntest.main.run_svn(None, 'propset', 'red', 'rojo',
                       os.path.join('A', 'D', 'G', 'pi'))

  # Restore the CWD.
  os.chdir(was_cwd)

  # Build an expected status tree.
  expected_status = get_standard_state(wc_dir)

  # Verify status -- all local mods should be present.
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  return expected_status


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

def commit_one_file(sbox):
  "commit one file"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/omega' : Item(verb='Sending'),
    })
  expected_status.tweak('A/D/H/omega', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        omega_path)


#----------------------------------------------------------------------

def commit_one_new_file(sbox):
  "commit one newly added file"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  gloo_path = os.path.join(wc_dir, 'A', 'D', 'H', 'gloo')

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/gloo' : Item(verb='Adding'),
    })
  expected_status.tweak('A/D/H/gloo', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        gloo_path)


#----------------------------------------------------------------------

def commit_one_new_binary_file(sbox):
  "commit one newly added binary file"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  gloo_path = os.path.join(wc_dir, 'A', 'D', 'H', 'gloo')
  svntest.main.run_svn(None, 'propset', 'svn:mime-type',
                       'application/octet-stream', gloo_path)

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/gloo' : Item(verb='Adding  (bin)'),
    })
  expected_status.tweak('A/D/H/gloo', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        gloo_path)


#----------------------------------------------------------------------

def commit_multiple_targets(sbox):
  "commit multiple targets"

  sbox.build()
  wc_dir = sbox.wc_dir

  # This test will commit three targets:  psi, B, and pi.  In that order.

  # Make local mods to many files.
  AB_path = os.path.join(wc_dir, 'A', 'B')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  svntest.main.file_append(lambda_path, 'new appended text for lambda')
  svntest.main.file_append(rho_path, 'new appended text for rho')
  svntest.main.file_append(pi_path, 'new appended text for pi')
  svntest.main.file_append(omega_path, 'new appended text for omega')
  svntest.main.file_append(psi_path, 'new appended text for psi')

  # Just for kicks, add a property to A/D/G as well.  We'll make sure
  # that it *doesn't* get committed.
  ADG_path = os.path.join(wc_dir, 'A', 'D', 'G')
  svntest.main.run_svn(None, 'propset', 'foo', 'bar', ADG_path)

  # Create expected output tree for 'svn ci'.  We should see changes
  # only on these three targets, no others.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/psi' : Item(verb='Sending'),
    'A/B/lambda' : Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    })

  # Create expected status tree; all local revisions should be at 1,
  # but our three targets should be at 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H/psi', 'A/B/lambda', 'A/D/G/pi', wc_rev=2)

  # rho and omega should still display as locally modified:
  expected_status.tweak('A/D/G/rho', 'A/D/H/omega', status='M ')

  # A/D/G should still have a local property set, too.
  expected_status.tweak('A/D/G', status=' M')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        psi_path, AB_path, pi_path)

#----------------------------------------------------------------------


def commit_multiple_targets_2(sbox):
  "commit multiple targets, 2nd variation"

  sbox.build()
  wc_dir = sbox.wc_dir

  # This test will commit four targets:  psi, B, omega and pi.  In that order.

  # Make local mods to many files.
  AB_path = os.path.join(wc_dir, 'A', 'B')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  svntest.main.file_append(lambda_path, 'new appended text for lambda')
  svntest.main.file_append(rho_path, 'new appended text for rho')
  svntest.main.file_append(pi_path, 'new appended text for pi')
  svntest.main.file_append(omega_path, 'new appended text for omega')
  svntest.main.file_append(psi_path, 'new appended text for psi')

  # Just for kicks, add a property to A/D/G as well.  We'll make sure
  # that it *doesn't* get committed.
  ADG_path = os.path.join(wc_dir, 'A', 'D', 'G')
  svntest.main.run_svn(None, 'propset', 'foo', 'bar', ADG_path)

  # Created expected output tree for 'svn ci'.  We should see changes
  # only on these three targets, no others.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/psi' : Item(verb='Sending'),
    'A/B/lambda' : Item(verb='Sending'),
    'A/D/H/omega' : Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    })

  # Create expected status tree; all local revisions should be at 1,
  # but our four targets should be at 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H/psi', 'A/B/lambda', 'A/D/G/pi', 'A/D/H/omega',
                        wc_rev=2)

  # rho should still display as locally modified:
  expected_status.tweak('A/D/G/rho', status='M ')

  # A/D/G should still have a local property set, too.
  expected_status.tweak('A/D/G', status=' M')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        psi_path, AB_path,
                                        omega_path, pi_path)

#----------------------------------------------------------------------

def commit_inclusive_dir(sbox):
  "commit wc_dir/A/D -- includes D. (anchor=A, tgt=D)"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  D_path = os.path.join(wc_dir, 'A', 'D')

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D' : Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Deleting'),
    'A/D/H/gloo' : Item(verb='Adding'),
    'A/D/H/chi' : Item(verb='Replacing'),
    'A/D/H/omega' : Item(verb='Sending'),
    'A/D/gamma' : Item(verb='Deleting'),
    })

  expected_status.remove('A/D/G/rho', 'A/D/gamma')
  expected_status.tweak('A/D', 'A/D/G/pi', 'A/D/H/omega',
                        wc_rev=2, status='  ')
  expected_status.tweak('A/D/H/chi', 'A/D/H/gloo', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        D_path)

#----------------------------------------------------------------------

def commit_top_dir(sbox):
  "commit wc_dir -- (anchor=wc_dir, tgt={})"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    '' : Item(verb='Sending'),
    'Q' : Item(verb='Adding'),
    'Q/floo' : Item(verb='Adding'),
    'A/B/E' : Item(verb='Replacing'),
    'A/B/E/bloo' : Item(verb='Adding'),
    'A/B/lambda' : Item(verb='Sending'),
    'A/C' : Item(verb='Deleting'),
    'A/D' : Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Deleting'),
    'A/D/H/gloo' : Item(verb='Adding'),
    'A/D/H/chi' : Item(verb='Replacing'),
    'A/D/H/omega' : Item(verb='Sending'),
    'A/D/gamma' : Item(verb='Deleting'),
    })

  expected_status.remove('A/D/G/rho', 'A/D/gamma', 'A/C',
                         'A/B/E/alpha', 'A/B/E/beta')
  expected_status.tweak('A/D', 'A/D/G/pi', 'A/D/H/omega', 'Q/floo', '',
                        wc_rev=2, status='  ')
  expected_status.tweak('A/D/H/chi', 'Q', 'A/B/E', 'A/B/E/bloo', 'A/B/lambda',
                        'A/D/H/gloo', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Regression test for bug reported by Jon Trowbridge:
#
#    From: Jon Trowbridge <trow@ximian.com>
#    Subject:  svn segfaults if you commit a file that hasn't been added
#    To: dev@subversion.tigris.org
#    Date: 17 Jul 2001 03:20:55 -0500
#    Message-Id: <995358055.16975.5.camel@morimoto>
#
#    The problem is that report_single_mod in libsvn_wc/adm_crawler.c is
#    called with its entry parameter as NULL, but the code doesn't
#    check that entry is non-NULL before trying to dereference it.
#
# This bug never had an issue number.
#
def commit_unversioned_thing(sbox):
  "committing unversioned object produces error"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create an unversioned file in the wc.
  svntest.main.file_append(os.path.join(wc_dir, 'blorg'), "nothing to see")

  # Commit a non-existent file and *expect* failure:
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "is not under version control",
                                        os.path.join(wc_dir,'blorg'))

#----------------------------------------------------------------------

# regression test for bug #391

def nested_dir_replacements(sbox):
  "replace two nested dirs, verify empty contents"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_D = os.path.join(wc_dir, 'A', 'D')

  # Delete and re-add A/D (a replacement), and A/D/H (another replace).
  svntest.main.run_svn(None, 'rm', A_D)

  # Recreate directories for single-db
  if not os.path.exists(A_D):
    os.mkdir(A_D)
    os.mkdir(os.path.join(A_D, 'H'))
  svntest.main.run_svn(None, 'add', '--depth=empty', A_D)
  svntest.main.run_svn(None, 'add', '--depth=empty', os.path.join(A_D, 'H'))

  # For kicks, add new file A/D/bloo.
  svntest.main.file_append(os.path.join(A_D, 'bloo'), "hi")
  svntest.main.run_svn(None, 'add', os.path.join(A_D, 'bloo'))

  # Verify pre-commit status:
  #
  #    - A/D should both be scheduled as addition, A/D as "R" at rev 1
  #         (rev 1 because they both existed before at rev 1)
  #
  #    - A/D/H should be a local addition "A"
  #         (and exists as shadowed node in BASE)
  #
  #    - A/D/bloo scheduled as "A" at rev 0
  #         (rev 0 because it did not exist before)
  #
  #    - ALL other children of A/D scheduled as "D" at rev 1

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D', status='R ', wc_rev=1)
  # In the entries world we couldn't represent H properly, so it shows
  # A/D/H as a replacement against BASE
  expected_status.tweak('A/D/H', status='A ', wc_rev='-',
                                 entry_status='R ', entry_rev='1')
  expected_status.add({
    'A/D/bloo' : Item(status='A ', wc_rev=0),
    })
  expected_status.tweak('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                        'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi', 'A/D/gamma',
                        status='D ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Build expected post-commit trees:

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D' : Item(verb='Replacing'),
    'A/D/H' : Item(verb='Adding'),
    'A/D/bloo' : Item(verb='Adding'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D', 'A/D/H', wc_rev=2)
  expected_status.add({
    'A/D/bloo' : Item(status='  ', wc_rev=2),
    })
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                        'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi', 'A/D/gamma')

  # Commit from the top of the working copy and verify output & status.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Testing part 1 of the "Greg Hudson" problem -- specifically, that
# our use of the "existence=deleted" flag is working properly in cases
# where the parent directory's revision lags behind a deleted child's
# revision.

def hudson_part_1(sbox):
  "hudson prob 1.0:  delete file, commit, update"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove gamma from the working copy.
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.run_svn(None, 'rm', gamma_path)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Deleting'),
    })

  # After committing, status should show no sign of gamma.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/gamma')

  # Commit the deletion of gamma and verify.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now gamma should be marked as `deleted' under the hood.  When we
  # update, we should no output, and a perfect, virginal status list
  # at revision 2.  (The `deleted' entry should be removed.)

  # Expected output of update:  nothing.
  expected_output = svntest.wc.State(wc_dir, {})

  # Expected disk tree:  everything but gamma
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/gamma')

  # Expected status after update:  totally clean revision 2, minus gamma.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/D/gamma')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


#----------------------------------------------------------------------

# Testing part 1 of the "Greg Hudson" problem -- variation on previous
# test, removing a directory instead of a file this time.

def hudson_part_1_variation_1(sbox):
  "hudson prob 1.1:  delete dir, commit, update"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove H from the working copy.
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  svntest.main.run_svn(None, 'rm', H_path)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H' : Item(verb='Deleting'),
    })

  # After committing, status should show no sign of H or its contents
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')

  # Commit the deletion of H and verify.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now H should be marked as `deleted' under the hood.  When we
  # update, we should no see output, and a perfect, virginal status
  # list at revision 2.  (The `deleted' entry should be removed.)

  # Expected output of update:  H gets a no-op deletion.
  expected_output = svntest.wc.State(wc_dir, {})

  # Expected disk tree:  everything except files in H
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')

  # Expected status after update:  totally clean revision 2, minus H.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

# Testing part 1 of the "Greg Hudson" problem -- variation 2.  In this
# test, we make sure that a file that is BOTH `deleted' and scheduled
# for addition can be correctly committed & merged.

def hudson_part_1_variation_2(sbox):
  "hudson prob 1.2:  delete, commit, re-add, commit"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove gamma from the working copy.
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.run_svn(None, 'rm', gamma_path)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Deleting'),
    })

  # After committing, status should show no sign of gamma.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/gamma')

  # Commit the deletion of gamma and verify.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now gamma should be marked as `deleted' under the hood.
  # Go ahead and re-add gamma, so that is *also* scheduled for addition.
  svntest.main.file_append(gamma_path, "added gamma")
  svntest.main.run_svn(None, 'add', gamma_path)

  # For sanity, examine status: it should show a revision 2 tree with
  # gamma scheduled for addition.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=0, status='A ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Adding'),
    })

  # After committing, status should show only gamma at revision 3.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=3)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)


#----------------------------------------------------------------------

# Testing part 2 of the "Greg Hudson" problem.
#
# In this test, we make sure that we're UNABLE to commit a propchange
# on an out-of-date directory.

def hudson_part_2(sbox):
  "hudson prob 2.0:  prop commit on old dir fails"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove gamma from the working copy.
  D_path = os.path.join(wc_dir, 'A', 'D')
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.run_svn(None, 'rm', gamma_path)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Deleting'),
    })

  # After committing, status should show no sign of gamma.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/gamma')

  # Commit the deletion of gamma and verify.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now gamma should be marked as `deleted' under the hood, at
  # revision 2.  Meanwhile, A/D is still lagging at revision 1.

  # Make a propchange on A/D
  svntest.main.run_svn(None, 'ps', 'foo', 'bar', D_path)

  # Commit and *expect* a repository Merge failure:
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "[Oo]ut.of.date",
                                        wc_dir)

#----------------------------------------------------------------------

# Test a possible regression in our 'deleted' post-commit handling.
#
# This test moves files from one subdir to another, commits, then
# updates the empty directory.  Nothing should be printed, assuming
# all the moved files are properly marked as 'deleted' and reported to
# the server.

def hudson_part_2_1(sbox):
  "hudson prob 2.1:  move files, update empty dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Move all the files in H to G
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  chi_path = os.path.join(H_path, 'chi')
  psi_path = os.path.join(H_path, 'psi')
  omega_path = os.path.join(H_path, 'omega')

  svntest.main.run_svn(None, 'mv', chi_path, G_path)
  svntest.main.run_svn(None, 'mv', psi_path, G_path)
  svntest.main.run_svn(None, 'mv', omega_path, G_path)

  # Create expected commit output.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/chi' : Item(verb='Deleting'),
    'A/D/H/omega' : Item(verb='Deleting'),
    'A/D/H/psi' : Item(verb='Deleting'),
    'A/D/G/chi' : Item(verb='Adding'),
    'A/D/G/omega' : Item(verb='Adding'),
    'A/D/G/psi' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/H/chi')
  expected_status.remove('A/D/H/omega')
  expected_status.remove('A/D/H/psi')
  expected_status.add({ 'A/D/G/chi' :
                        Item(wc_rev=2, status='  ') })
  expected_status.add({ 'A/D/G/omega' :
                        Item(wc_rev=2, status='  ') })
  expected_status.add({ 'A/D/G/psi' :
                        Item(wc_rev=2, status='  ') })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now, assuming all three files in H are marked as 'deleted', an
  # update of H should print absolutely nothing.
  expected_output = svntest.wc.State(wc_dir, { })

  # Reuse expected_status
  expected_status.tweak(wc_rev=2)

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')
  expected_disk.add({
    'A/D/G/chi' : Item("This is the file 'chi'.\n"),
    })
  expected_disk.add({
    'A/D/G/omega' : Item("This is the file 'omega'.\n"),
    })
  expected_disk.add({
    'A/D/G/psi' : Item("This is the file 'psi'.\n"),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

def hook_test(sbox):
  "hook testing"

  sbox.build()

  # Get paths to the working copy and repository
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  # Create a hook that appends its name to a log file.
  hook_format = """import sys
fp = open(sys.argv[1] + '/hooks.log', 'a')
fp.write("%s\\n")
fp.close()"""

  # Setup the hook configs to log data to a file
  start_commit_hook = svntest.main.get_start_commit_hook_path(repo_dir)
  svntest.main.create_python_hook_script(start_commit_hook,
                                         hook_format % "start_commit_hook")

  pre_commit_hook = svntest.main.get_pre_commit_hook_path(repo_dir)
  svntest.main.create_python_hook_script(pre_commit_hook,
                                         hook_format % "pre_commit_hook")

  post_commit_hook = svntest.main.get_post_commit_hook_path(repo_dir)
  svntest.main.create_python_hook_script(post_commit_hook,
                                         hook_format % "post_commit_hook")

  # Modify iota just so there is something to commit.
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "More stuff in iota")

  # Commit, no output expected.
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'ci', '--quiet',
                                     '-m', 'log msg', wc_dir)

  # Now check the logfile
  expected_data = [ 'start_commit_hook\n', 'pre_commit_hook\n', 'post_commit_hook\n' ]

  logfilename = os.path.join(repo_dir, "hooks.log")
  if os.path.exists(logfilename):
    fp = open(logfilename)
  else:
    raise svntest.verify.SVNUnexpectedOutput("hook logfile %s not found")\
                                             % logfilename

  actual_data = fp.readlines()
  fp.close()
  os.unlink(logfilename)
  svntest.verify.compare_and_display_lines('wrong hook logfile content',
                                           'STDERR',
                                           expected_data, actual_data)

#----------------------------------------------------------------------

# Regression test for bug #469, whereby merge() was once reporting
# erroneous conflicts due to Ancestor < Target < Source, in terms of
# node-rev-id parentage.

def merge_mixed_revisions(sbox):
  "commit mixed-rev wc (no erroneous merge error)"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make some convenient paths.
  iota_path = os.path.join(wc_dir, 'iota')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')

  # Here's the reproduction formula, in 5 parts.
  # Hoo, what a buildup of state!

  # 1. echo "moo" >> iota; echo "moo" >> A/D/H/chi; svn ci
  svntest.main.file_append(iota_path, "moo")
  svntest.main.file_append(chi_path, "moo")

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/D/H/chi' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', 'A/D/H/chi', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)


  # 2. svn up A/D/H
  expected_status = svntest.wc.State(wc_dir, {
    'A/D/H' : Item(status='  ', wc_rev=2),
    'A/D/H/chi' : Item(status='  ', wc_rev=2),
    'A/D/H/omega' : Item(status='  ', wc_rev=2),
    'A/D/H/psi' : Item(status='  ', wc_rev=2),
    })
  expected_disk = svntest.wc.State('', {
    'omega' : Item("This is the file 'omega'.\n"),
    'chi' : Item("This is the file 'chi'.\nmoo"),
    'psi' : Item("This is the file 'psi'.\n"),
    })
  expected_output = svntest.wc.State(wc_dir, { })
  svntest.actions.run_and_verify_update(H_path,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


  # 3. echo "moo" >> iota; svn ci iota
  svntest.main.file_append(iota_path, "moo2")
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H', 'A/D/H/omega', 'A/D/H/chi', 'A/D/H/psi',
                        wc_rev=2)
  expected_status.tweak('iota', wc_rev=3)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)


  # 4. echo "moo" >> A/D/H/chi; svn ci A/D/H/chi
  svntest.main.file_append(chi_path, "moo3")
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/chi' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H/chi', wc_rev=4)
  expected_status.tweak('A/D/H', 'A/D/H/omega', 'A/D/H/psi', wc_rev=2)
  expected_status.tweak('iota', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # 5. echo "moo" >> iota; svn ci iota
  svntest.main.file_append(iota_path, "moomoo")
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/H', 'A/D/H/omega', 'A/D/H/psi', wc_rev=2)
  expected_status.tweak('A/D/H/chi', wc_rev=4)
  expected_status.tweak('iota', wc_rev=5)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # At this point, here is what our tree should look like:
  # _    1       (     5)  working_copies/commit_tests-10
  # _    1       (     5)  working_copies/commit_tests-10/A
  # _    1       (     5)  working_copies/commit_tests-10/A/B
  # _    1       (     5)  working_copies/commit_tests-10/A/B/E
  # _    1       (     5)  working_copies/commit_tests-10/A/B/E/alpha
  # _    1       (     5)  working_copies/commit_tests-10/A/B/E/beta
  # _    1       (     5)  working_copies/commit_tests-10/A/B/F
  # _    1       (     5)  working_copies/commit_tests-10/A/B/lambda
  # _    1       (     5)  working_copies/commit_tests-10/A/C
  # _    1       (     5)  working_copies/commit_tests-10/A/D
  # _    1       (     5)  working_copies/commit_tests-10/A/D/G
  # _    1       (     5)  working_copies/commit_tests-10/A/D/G/pi
  # _    1       (     5)  working_copies/commit_tests-10/A/D/G/rho
  # _    1       (     5)  working_copies/commit_tests-10/A/D/G/tau
  # _    2       (     5)  working_copies/commit_tests-10/A/D/H
  # _    4       (     5)  working_copies/commit_tests-10/A/D/H/chi
  # _    2       (     5)  working_copies/commit_tests-10/A/D/H/omega
  # _    2       (     5)  working_copies/commit_tests-10/A/D/H/psi
  # _    1       (     5)  working_copies/commit_tests-10/A/D/gamma
  # _    1       (     5)  working_copies/commit_tests-10/A/mu
  # _    5       (     5)  working_copies/commit_tests-10/iota

  # At this point, we're ready to modify omega and iota, and commit
  # from the top.  We should *not* get a conflict!

  svntest.main.file_append(iota_path, "finalmoo")
  svntest.main.file_append(omega_path, "finalmoo")

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/D/H/omega' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', 'A/D/H/omega', wc_rev=6)
  expected_status.tweak('A/D/H', 'A/D/H/psi', wc_rev=2)
  expected_status.tweak('A/D/H/chi', wc_rev=4)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

def commit_uri_unsafe(sbox):
  "commit files and dirs with URI-unsafe characters"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Note: on Windows, files can't have angle brackets in them, so we
  # don't tests that case.
  if svntest.main.windows or sys.platform == 'cygwin':
    angle_name = '_angle_'
    nasty_name = '#![]{}()__%'
  else:
    angle_name = '<angle>'
    nasty_name = '#![]{}()<>%'

  # Make some convenient paths.
  hash_dir = os.path.join(wc_dir, '#hash#')
  nasty_dir = os.path.join(wc_dir, nasty_name)
  space_path = os.path.join(wc_dir, 'A', 'D', 'space path')
  bang_path = os.path.join(wc_dir, 'A', 'D', 'H', 'bang!')
  bracket_path = os.path.join(wc_dir, 'A', 'D', 'H', 'bra[ket')
  brace_path = os.path.join(wc_dir, 'A', 'D', 'H', 'bra{e')
  angle_path = os.path.join(wc_dir, 'A', 'D', 'H', angle_name)
  paren_path = os.path.join(wc_dir, 'A', 'D', 'pare)(theses')
  percent_path = os.path.join(wc_dir, '#hash#', 'percen%')
  nasty_path = os.path.join(wc_dir, 'A', nasty_name)

  os.mkdir(hash_dir)
  os.mkdir(nasty_dir)
  svntest.main.file_append(space_path, "This path has a space in it.")
  svntest.main.file_append(bang_path, "This path has a bang in it.")
  svntest.main.file_append(bracket_path, "This path has a bracket in it.")
  svntest.main.file_append(brace_path, "This path has a brace in it.")
  svntest.main.file_append(angle_path, "This path has angle brackets in it.")
  svntest.main.file_append(paren_path, "This path has parentheses in it.")
  svntest.main.file_append(percent_path, "This path has a percent in it.")
  svntest.main.file_append(nasty_path, "This path has all sorts of ick in it.")

  add_list = [hash_dir,
              nasty_dir, # not xml-safe
              space_path,
              bang_path,
              bracket_path,
              brace_path,
              angle_path, # not xml-safe
              paren_path,
              percent_path,
              nasty_path, # not xml-safe
              ]
  for item in add_list:
    svntest.main.run_svn(None, 'add', '--depth=empty', item)

  expected_output = svntest.wc.State(wc_dir, {
    '#hash#' : Item(verb='Adding'),
    nasty_name : Item(verb='Adding'),
    'A/D/space path' : Item(verb='Adding'),
    'A/D/H/bang!' : Item(verb='Adding'),
    'A/D/H/bra[ket' : Item(verb='Adding'),
    'A/D/H/bra{e' : Item(verb='Adding'),
    'A/D/H/' + angle_name : Item(verb='Adding'),
    'A/D/pare)(theses' : Item(verb='Adding'),
    '#hash#/percen%' : Item(verb='Adding'),
    'A/' + nasty_name : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Items in our add list will be at rev 2
  for item in expected_output.desc.keys():
    expected_status.add({ item : Item(wc_rev=2, status='  ') })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)


#----------------------------------------------------------------------

def commit_deleted_edited(sbox):
  "commit deleted yet edited files"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make some convenient paths.
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Edit the files.
  svntest.main.file_append(iota_path, "This file has been edited.")
  svntest.main.file_append(mu_path, "This file has been edited.")

  # Schedule the files for removal.
  svntest.main.run_svn(None, 'remove', '--force', iota_path)
  svntest.main.run_svn(None, 'remove', '--force', mu_path)

  # Make our output list
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Deleting'),
    'A/mu' : Item(verb='Deleting'),
    })

  # Items in the status list are all at rev 1, except the two things
  # we changed...but then, they don't exist at all.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('iota', 'A/mu')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

def commit_in_dir_scheduled_for_addition(sbox):
  "commit a file inside dir scheduled for addition"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  Z_path = os.path.join(wc_dir, 'Z')
  Z_abspath = os.path.abspath(Z_path)
  mu_path = os.path.join(wc_dir, 'Z', 'mu')

  svntest.main.run_svn(None, 'move', A_path, Z_path)
  
  # Make sure mu is a committable
  svntest.main.file_write(mu_path, "xxxx")

  # Commit a copied thing inside an added-with-history directory,
  # expecting a specific error to occur!
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "svn: E200009: '" +
                                        re.escape(Z_abspath) +
                                        "' is not known to exist in the",
                                        mu_path)

  Q_path = os.path.join(wc_dir, 'Q')
  Q_abspath = os.path.abspath(Q_path)
  bloo_path = os.path.join(Q_path, 'bloo')

  os.mkdir(Q_path)
  svntest.main.file_append(bloo_path, "New contents.")
  svntest.main.run_svn(None, 'add', Q_path)

  # Commit a regular added thing inside an added directory,
  # expecting a specific error to occur!
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "svn: E200009: '" +
                                        re.escape(Q_abspath) +
                                        "' is not known to exist in the",
                                        bloo_path)

  R_path = sbox.ospath('Z/B/R')
  sbox.simple_mkdir('Z/B/R')

  # Commit a d added thing inside an added directory,
  # expecting a specific error to occur!
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "svn: E200009: '" +
                                        re.escape(Z_abspath) +
                                        "' is not known to exist in the.*",
                                        R_path)                                        

#----------------------------------------------------------------------

# Does this make sense now that deleted files are always removed from the wc?
def commit_rmd_and_deleted_file(sbox):
  "commit deleted (and missing) file"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # 'svn remove' mu
  svntest.main.run_svn(None, 'rm', mu_path)

  # Commit, hoping to see no errors
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'commit', '-m', 'logmsg', mu_path)

#----------------------------------------------------------------------

# Issue #644 which failed over ra_neon.
@Issue(644)
def commit_add_file_twice(sbox):
  "issue 644 attempt to add a file twice"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a file
  gloo_path = os.path.join(wc_dir, 'A', 'D', 'H', 'gloo')
  svntest.main.file_append(gloo_path, "hello")
  svntest.main.run_svn(None, 'add', gloo_path)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/gloo' : Item(verb='Adding'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/H/gloo' : Item(status='  ', wc_rev=2),
    })

  # Commit should succeed
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Update to state before commit
  svntest.main.run_svn(None, 'up', '-r', '1', wc_dir)

  # Create the file again
  svntest.main.file_append(gloo_path, "hello")
  svntest.main.run_svn(None, 'add', gloo_path)

  # Commit and *expect* a failure:
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        "already exists",
                                        wc_dir)

#----------------------------------------------------------------------

# There was a problem that committing from a directory that had a
# longer name than the working copy directory caused the commit notify
# messages to display truncated/random filenames.

def commit_from_long_dir(sbox):
  "commit from a dir with a longer name than the wc"

  sbox.build()
  wc_dir = sbox.wc_dir

  was_dir = os.getcwd()
  abs_wc_dir = os.path.realpath(os.path.join(was_dir, wc_dir))

  # something to commit
  svntest.main.file_append(os.path.join(wc_dir, 'iota'), "modified iota")

  # Create expected output tree.
  expected_output = svntest.wc.State('', {
    'iota' : Item(verb='Sending'),
    })

  # Any length name was enough to provoke the original bug, but
  # keeping its length less than that of the filename 'iota' avoided
  # random behaviour, but still caused the test to fail
  extra_name = 'xx'

  os.chdir(wc_dir)
  os.mkdir(extra_name)
  os.chdir(extra_name)

  svntest.actions.run_and_verify_commit(abs_wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        abs_wc_dir)

#----------------------------------------------------------------------

def commit_with_lock(sbox):
  "try to commit when directory is locked"

  sbox.build()
  # modify gamma and lock its directory
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  gamma_path = os.path.join(D_path, 'gamma')
  svntest.main.file_append(gamma_path, "modified gamma")
  svntest.actions.lock_admin_dir(D_path)

  # this commit should fail
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        'svn: E155004: '
                                        'Working copy \'.*\' locked',
                                        wc_dir)

  # unlock directory
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     [], [],
                                     'cleanup', D_path)

  # this commit should succeed
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Explicitly commit the current directory.  This did at one point fail
# in post-commit processing due to a path canonicalization problem.

def commit_current_dir(sbox):
  "commit the current directory"

  sbox.build()

  wc_dir = sbox.wc_dir
  svntest.main.run_svn(None, 'propset', 'pname', 'pval', wc_dir)

  was_cwd = os.getcwd()

  os.chdir(wc_dir)

  expected_output = svntest.wc.State('.', {
    '.' : Item(verb='Sending'),
    })
  svntest.actions.run_and_verify_commit('.',
                                        expected_output,
                                        None,
                                        None,
                                        '.')
  os.chdir(was_cwd)

  # I can't get the status check to work as part of run_and_verify_commit.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

# Check that the pending txn gets removed from the repository after
# a failed commit.

def failed_commit(sbox):
  "commit with conflicts and check txn in repo"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make the other working copy
  other_wc_dir = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, other_wc_dir)

  # Make different changes in the two working copies
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "More stuff in iota")

  other_iota_path = os.path.join(other_wc_dir, "iota")
  svntest.main.file_append(other_iota_path, "More different stuff in iota")

  # Commit both working copies. The second commit should fail.
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'commit', '-m', 'log', wc_dir)

  svntest.actions.run_and_verify_svn("Output on stderr expected",
                                     None, svntest.verify.AnyOutput,
                                     'commit', '-m', 'log', other_wc_dir)

  # Now list the txns in the repo. The list should be empty.
  exit_code, output, errput = svntest.main.run_svnadmin('lstxns',
                                                        sbox.repo_dir)
  svntest.verify.compare_and_display_lines(
    "Error running 'svnadmin lstxns'.",
    'STDERR', [], errput)
  svntest.verify.compare_and_display_lines(
    "Output of 'svnadmin lstxns' is unexpected.",
    'STDOUT', [], output)

#----------------------------------------------------------------------

# Commit from multiple working copies is being worked on as issue #2381.
# Also related to issue #959, this test here doesn't use svn:externals
# but the behaviour needs to be considered.
# In this test two WCs are nested, one WC is child of the other.
@Issue(2381)
def commit_multiple_wc_nested(sbox):
  "commit from two nested working copies"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Checkout a second working copy
  wc2_dir = os.path.join(wc_dir, 'A', 'wc2')
  url = sbox.repo_url
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'checkout',
                                     url, wc2_dir)

  # Modify both working copies
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, 'appended mu text')
  lambda2_path = os.path.join(wc2_dir, 'A', 'B', 'lambda')
  svntest.main.file_append(lambda2_path, 'appended lambda2 text')

  # Verify modified status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='M ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  expected_status2 = svntest.actions.get_virginal_state(wc2_dir, 1)
  expected_status2.tweak('A/B/lambda', status='M ')
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

  # Commit should succeed, even though one target is a "child" of the other.
  svntest.actions.run_and_verify_svn("Ouput on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'commit', '-m', 'log',
                                     wc_dir, wc2_dir)

  # Verify status changed
  expected_status.tweak('A/mu', status='  ', wc_rev=2)
  expected_status2.tweak('A/B/lambda', status='  ', wc_rev=2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

# Same as commit_multiple_wc_nested except that the two WCs are not nested.
@Issue(2381)
def commit_multiple_wc(sbox):
  "commit from two working copies"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Cleanup original wc
  svntest.sandbox._cleanup_test_path(wc_dir)

  # Checkout two wcs
  wc1_dir = os.path.join(wc_dir, 'wc1')
  wc2_dir = os.path.join(wc_dir, 'wc2')
  url = sbox.repo_url
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'checkout',
                                     url, wc1_dir)
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'checkout',
                                     url, wc2_dir)

  # Modify both working copies
  mu1_path = os.path.join(wc1_dir, 'A', 'mu')
  svntest.main.file_append(mu1_path, 'appended mu1 text')
  lambda2_path = os.path.join(wc2_dir, 'A', 'B', 'lambda')
  svntest.main.file_append(lambda2_path, 'appended lambda2 text')

  # Verify modified status
  expected_status1 = svntest.actions.get_virginal_state(wc1_dir, 1)
  expected_status1.tweak('A/mu', status='M ')
  svntest.actions.run_and_verify_status(wc1_dir, expected_status1)
  expected_status2 = svntest.actions.get_virginal_state(wc2_dir, 1)
  expected_status2.tweak('A/B/lambda', status='M ')
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

  # Commit should succeed.
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'commit', '-m', 'log',
                                     wc1_dir, wc2_dir)

  # Verify status changed
  expected_status1.tweak('A/mu', status='  ', wc_rev=2)
  expected_status2.tweak('A/B/lambda', status='  ', wc_rev=2)
  svntest.actions.run_and_verify_status(wc1_dir, expected_status1)
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

# Same as commit_multiple_wc except that the two WCs come
# from different repositories. Commits to multiple repositories
# are outside the scope of issue #2381.
@Issue(2381)
def commit_multiple_wc_multiple_repos(sbox):
  "committing two WCs from different repos fails"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create another repository
  repo2, url2 = sbox.add_repo_path("repo2")
  svntest.main.copy_repos(sbox.repo_dir, repo2, 1, 1)

  # Cleanup original wc
  svntest.sandbox._cleanup_test_path(wc_dir)

  # Checkout two wcs
  wc1_dir = os.path.join(wc_dir, 'wc1')
  wc2_dir = os.path.join(wc_dir, 'wc2')
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'checkout',
                                     sbox.repo_url, wc1_dir)
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'checkout',
                                     url2, wc2_dir)

  # Modify both working copies
  mu1_path = os.path.join(wc1_dir, 'A', 'mu')
  svntest.main.file_append(mu1_path, 'appended mu1 text')
  lambda2_path = os.path.join(wc2_dir, 'A', 'B', 'lambda')
  svntest.main.file_append(lambda2_path, 'appended lambda2 text')

  # Verify modified status
  expected_status1 = svntest.actions.get_virginal_state(wc1_dir, 1)
  expected_status1.tweak('A/mu', status='M ')
  svntest.actions.run_and_verify_status(wc1_dir, expected_status1)
  expected_status2 = svntest.actions.get_virginal_state(wc2_dir, 1)
  expected_status2.tweak('A/B/lambda', status='M ')
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

  # Commit should fail, since WCs come from different repositories.
  # The exact error message depends on whether or not the tests are
  # run below an existing working copy
  error_re = ( ".*(is not a working copy" +
                 "|Are all targets part of the same working copy" +
                 "|was not found).*" )
  svntest.actions.run_and_verify_svn("Expected output on stderr doesn't match",
                                     [], error_re,
                                     'commit', '-m', 'log',
                                     wc1_dir, wc2_dir)

  # Verify status unchanged
  svntest.actions.run_and_verify_status(wc1_dir, expected_status1)
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)

#----------------------------------------------------------------------
@Issues(1195,1239)
def commit_nonrecursive(sbox):
  "commit named targets with -N"

  sbox.build()
  wc_dir = sbox.wc_dir

  ### Note: the original recipes used 'add -N'.  These days, we use
  ### --depth={empty,files}, and both the code and the comments below
  ### have been adjusted to reflect this.

  #####################################################
  ### Issue #1195:
  ###
  ### 1. Create these directories and files:
  ###
  ###    file1
  ###    dir1
  ###    dir1/file2
  ###    dir1/file3
  ###    dir1/dir2
  ###    dir1/dir2/file4
  ###
  ### 2. run 'svn add --depth=empty <all of the above>'
  ###
  ### 3. run 'svn ci -N <all of the above>'
  ###
  ### (The bug was that only 4 entities would get committed, when it
  ### should be 6: dir2/ and file4 were left out.)

  # These paths are relative to the top of the test's working copy.
  file1_path = 'file1'
  dir1_path  = 'dir1'
  file2_path = os.path.join('dir1', 'file2')
  file3_path = os.path.join('dir1', 'file3')
  dir2_path  = os.path.join('dir1', 'dir2')
  file4_path = os.path.join('dir1', 'dir2', 'file4')

  # Create the new files and directories.
  svntest.main.file_append(os.path.join(wc_dir, file1_path), 'this is file1')
  os.mkdir(os.path.join(wc_dir, dir1_path))
  svntest.main.file_append(os.path.join(wc_dir, file2_path), 'this is file2')
  svntest.main.file_append(os.path.join(wc_dir, file3_path), 'this is file3')
  os.mkdir(os.path.join(wc_dir, dir2_path))
  svntest.main.file_append(os.path.join(wc_dir, file4_path), 'this is file4')

  # Add them to version control.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'add', '--depth=empty',
                                     os.path.join(wc_dir, file1_path),
                                     os.path.join(wc_dir, dir1_path),
                                     os.path.join(wc_dir, file2_path),
                                     os.path.join(wc_dir, file3_path),
                                     os.path.join(wc_dir, dir2_path),
                                     os.path.join(wc_dir, file4_path))

  # Commit.  We should see all 6 items (2 dirs, 4 files) get sent.
  expected_output = svntest.wc.State(
    wc_dir,
    { file1_path                    : Item(verb='Adding'),
      dir1_path                     : Item(verb='Adding'),
      file2_path                    : Item(verb='Adding'),
      file3_path                    : Item(verb='Adding'),
      dir2_path                     : Item(verb='Adding'),
      file4_path                    : Item(verb='Adding'),
      }
    )

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    file1_path   : Item(status='  ', wc_rev=2),
    dir1_path    : Item(status='  ', wc_rev=2),
    file2_path   : Item(status='  ', wc_rev=2),
    file3_path   : Item(status='  ', wc_rev=2),
    dir2_path    : Item(status='  ', wc_rev=2),
    file4_path   : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        '-N',
                                        os.path.join(wc_dir, file1_path),
                                        os.path.join(wc_dir, dir1_path),
                                        os.path.join(wc_dir, file2_path),
                                        os.path.join(wc_dir, file3_path),
                                        os.path.join(wc_dir, dir2_path),
                                        os.path.join(wc_dir, file4_path))

  #######################################################################
  ###
  ### There's some complex history here; please bear with me.
  ###
  ### First there was issue #1239, which had the following recipe:
  ###
  ###    1. Create these directories and files:
  ###
  ###       dirA
  ###       dirA/fileA
  ###       dirA/fileB
  ###       dirA/dirB
  ###       dirA/dirB/fileC
  ###       dirA/dirB/nocommit
  ###
  ###    2. run 'svn add --depth=empty <all of the above>'
  ###
  ###    3. run 'svn ci -N <all but nocommit>'
  ###
  ###    (In this recipe, 'add -N' has been changed to 'add --depth...',
  ###     but 'ci -N' has been left as-is, for reasons explained below.)
  ###
  ### Issue #1239 claimed a two-part bug: that step 3 would try to
  ### commit the file `nocommit' when it shouldn't, and that it would
  ### get an error anyway:
  ###
  ###       Adding         wc/dirA
  ###       Adding         wc/dirA/fileA
  ###       Adding         wc/dirA/fileB
  ###       Adding         wc/dirA/dirB
  ###       Adding         wc/dirA/dirB/nocommit
  ###       Adding         wc/dirA/dirB/fileC
  ###       Transmitting file data ....svn: A problem occurred; \
  ###          see later errors for details
  ###       svn: Commit succeeded, but other errors follow:
  ###       svn: Problem running log
  ###       svn: Error bumping revisions post-commit (details follow):
  ###       svn: in directory
  ###       'F:/Programmation/Projets/subversion/svnant/test/wc/dirA'
  ###       svn: start_handler: error processing command 'committed' in
  ###       'F:/Programmation/Projets/subversion/svnant/test/wc/dirA'
  ###       svn: Working copy not locked
  ###       svn: directory not locked
  ###       (F:/Programmation/Projets/subversion/svnant/test/wc)
  ###
  ### However, this was all in the days before --depth, and depended
  ### on an idiosyncratic interpretation of -N, one which required
  ### commit to behave differently from other commands taking -N.
  ###
  ### These days, -N should be equivalent to --depth=files in almost
  ### all cases.  There are some exceptions (e.g., status), and commit
  ### is one of them: 'commit -N' means 'commit --depth=empty'.
  ###
  ### The original implementation, as well as this test, mistakenly
  ### mapped 'commit -N' to 'commit --depth=files'; that was a bug that
  ### made 'svn ci -N' incompatible with 1.4 and earlier versions.
  ###
  ### See also 'commit_propmods_with_depth_empty' in depth_tests.py .

  # Now add these directories and files, except the last:
  dirA_path  = 'dirA'
  fileA_path = os.path.join('dirA', 'fileA')
  fileB_path = os.path.join('dirA', 'fileB')
  dirB_path  = os.path.join('dirA', 'dirB')
  nope_1_path = os.path.join(dirB_path, 'nope_1')
  nope_2_path = os.path.join(dirB_path, 'nope_2')

  # Create the new files and directories.
  os.mkdir(os.path.join(wc_dir, dirA_path))
  svntest.main.file_append(os.path.join(wc_dir, fileA_path), 'fileA')
  svntest.main.file_append(os.path.join(wc_dir, fileB_path), 'fileB')
  os.mkdir(os.path.join(wc_dir, dirB_path))
  svntest.main.file_append(os.path.join(wc_dir, nope_1_path), 'nope_1')
  svntest.main.file_append(os.path.join(wc_dir, nope_2_path), 'nope_2')

  # Add them to version control.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'add', '-N',
                                     os.path.join(wc_dir, dirA_path),
                                     os.path.join(wc_dir, fileA_path),
                                     # don't add fileB
                                     os.path.join(wc_dir, dirB_path),
                                     os.path.join(wc_dir, nope_1_path),
                                     # don't add nope_2
                                     )

  expected_output = svntest.wc.State(
    wc_dir,
    { dirA_path  : Item(verb='Adding'),
      # no children!
      }
    )

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Expect the leftovers from the first part of the test.
  expected_status.add({
    file1_path : Item(status='  ', wc_rev=2),
    dir1_path  : Item(status='  ', wc_rev=2),
    file2_path : Item(status='  ', wc_rev=2),
    file3_path : Item(status='  ', wc_rev=2),
    dir2_path  : Item(status='  ', wc_rev=2),
    file4_path : Item(status='  ', wc_rev=2),
    })

  # Expect some commits and some non-commits from this part of the test.
  expected_status.add({
    dirA_path     : Item(status='  ', wc_rev=3),
    fileA_path    : Item(status='A ', wc_rev=0),
    # no fileB
    dirB_path     : Item(status='A ', wc_rev=0),
    nope_1_path   : Item(status='A ', wc_rev=0),
    # no nope_2
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        '-N', os.path.join(wc_dir, dirA_path))

#----------------------------------------------------------------------
# Regression for #1017: ra_neon was allowing the deletion of out-of-date
# files or dirs, which majorly violates Subversion's semantics.
# An out-of-date error should be raised if the object to be committed has
# already been deleted or modified in the repo.

def commit_out_of_date_deletions(sbox):
  "commit deletion of out-of-date file or dir"

  # Path           WC 1    WC backup
  # ===========    ====    =========
  # A/C            pset    del
  # A/I            del     pset
  # A/B/F          del     del
  # A/D/H/omega    text    del
  # A/B/E/alpha    pset    del
  # A/D/H/chi      del     text
  # A/B/E/beta     del     pset
  # A/D/H/psi      del     del

  sbox.build()
  wc_dir = sbox.wc_dir

  # Need another empty dir
  I_path = os.path.join(wc_dir, 'A', 'I')
  os.mkdir(I_path)
  svntest.main.run_svn(None, 'add', I_path)
  svntest.main.run_svn(None, 'ci', '-m', 'prep', wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Edits in wc 1
  C_path = os.path.join(wc_dir, 'A', 'C')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  svntest.main.run_svn(None, 'propset', 'fooprop', 'foopropval', C_path)
  svntest.main.file_append(omega_path, 'appended omega text')
  svntest.main.run_svn(None, 'propset', 'fooprop', 'foopropval', alpha_path)

  # Deletions in wc 1
  I_path = os.path.join(wc_dir, 'A', 'I')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  beta_path = os.path.join(wc_dir, 'A', 'B', 'E', 'beta')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  svntest.main.run_svn(None, 'rm', I_path, F_path, chi_path, beta_path,
                       psi_path)

  # Commit in wc 1
  expected_output = svntest.wc.State(wc_dir, {
      'A/C' : Item(verb='Sending'),
      'A/I' : Item(verb='Deleting'),
      'A/B/F' : Item(verb='Deleting'),
      'A/D/H/omega' : Item(verb='Sending'),
      'A/B/E/alpha' : Item(verb='Sending'),
      'A/D/H/chi' : Item(verb='Deleting'),
      'A/B/E/beta' : Item(verb='Deleting'),
      'A/D/H/psi' : Item(verb='Deleting'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/C', 'A/D/H/omega', 'A/B/E/alpha', wc_rev=3,
                        status='  ')
  expected_status.remove('A/B/F', 'A/D/H/chi', 'A/B/E/beta', 'A/D/H/psi')
  commit = svntest.actions.run_and_verify_commit
  commit(wc_dir, expected_output, expected_status, None, wc_dir)

  # Edits in wc backup
  I_path = os.path.join(wc_backup, 'A', 'I')
  chi_path = os.path.join(wc_backup, 'A', 'D', 'H', 'chi')
  beta_path = os.path.join(wc_backup, 'A', 'B', 'E','beta')
  svntest.main.run_svn(None, 'propset', 'fooprop', 'foopropval', I_path)
  svntest.main.file_append(chi_path, 'appended chi text')
  svntest.main.run_svn(None, 'propset', 'fooprop', 'foopropval', beta_path)

  # Deletions in wc backup
  C_path = os.path.join(wc_backup, 'A', 'C')
  F_path = os.path.join(wc_backup, 'A', 'B', 'F')
  omega_path = os.path.join(wc_backup, 'A', 'D', 'H', 'omega')
  alpha_path = os.path.join(wc_backup, 'A', 'B', 'E', 'alpha')
  psi_path = os.path.join(wc_backup, 'A', 'D', 'H', 'psi')
  svntest.main.run_svn(None, 'rm', C_path, F_path, omega_path, alpha_path,
                       psi_path)

  # A commit of any one of these files or dirs should fail, preferably
  # with an out-of-date error message.
  error_re = "(out of date|not found)"
  commit(wc_backup, None, None, error_re, C_path)
  commit(wc_backup, None, None, error_re, I_path)
  commit(wc_backup, None, None, error_re, F_path)
  commit(wc_backup, None, None, error_re, omega_path)
  commit(wc_backup, None, None, error_re, alpha_path)
  commit(wc_backup, None, None, error_re, chi_path)
  commit(wc_backup, None, None, error_re, beta_path)
  commit(wc_backup, None, None, error_re, psi_path)

def commit_with_bad_log_message(sbox):
  "commit with a log message containing bad data"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  log_msg_path = os.path.join(wc_dir, 'log-message')

  # Make a random change, so there's something to commit.
  svntest.main.file_append(iota_path, 'fish')

  # Create a log message containing a zero-byte.
  svntest.main.file_append(log_msg_path, '\x00')

  # Commit and expect an error.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None, None,
                                        "contains a zero byte",
                                        '-F', log_msg_path,
                                        iota_path)

def commit_with_mixed_line_endings(sbox):
  "commit with log message with mixed EOL"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  iota_path = os.path.join(wc_dir, 'iota')
  log_msg_path = os.path.join(wc_dir, 'log-message')

  # Make a random change, so there's something to commit.
  svntest.main.file_append(iota_path, 'kebab')

  # Create a log message containing a zero-byte.
  svntest.main.file_append(log_msg_path, "test\nthis\n\rcase\r\n--This line, and those below, will be ignored--\n")

  # Commit and expect an error.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None, None,
                                        "Error normalizing log message to internal format",
                                        '-F', log_msg_path,
                                        iota_path)

def commit_with_mixed_line_endings_in_ignored_part(sbox):
  "commit with log message with mixed EOL in tail"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_status = make_standard_slew_of_changes(wc_dir)

  iota_path = os.path.join(wc_dir, 'iota')
  log_msg_path = os.path.join(wc_dir, 'log-message')

  # Make a random change, so there's something to commit.
  svntest.main.file_append(iota_path, 'cheeseburger')

  # Create a log message containing a zero-byte.
  svntest.main.file_append(log_msg_path, "test\n--This line, and those below, will be ignored--\nfoo\r\nbar\nbaz\n\r")

  # Create expected state.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status.tweak('iota', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        iota_path)

def from_wc_top_with_bad_editor(sbox):
  "commit with invalid external editor cmd"

  # Shortly after revision 5407, Vladimir Prus posted this bug recipe:
  #
  #   #!/bin/bash
  #   cd /tmp
  #   rm -rf repo wc
  #   svnadmin create repo
  #   svn mkdir file:///tmp/repo/foo -m ""
  #   svn co file:///tmp/repo/foo wc
  #   cd wc
  #   svn ps svn:externals "lib http://something.org/lib" .
  #   svn ci
  #
  # The final 'svn ci' would seg fault because of a problem in
  # calculating the paths to insert in the initial log message that
  # gets passed to the editor.
  #
  # So this regression test is primarily about making sure the seg
  # fault is gone, and only secondarily about testing that we get the
  # expected error from passing a bad editor cmd to Subversion.

  sbox.build()
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn("Unexpected failure from propset.",
                                     svntest.verify.AnyOutput, [],
                                     'pset', 'fish', 'food', wc_dir)
  os.chdir(wc_dir)
  exit_code, out, err = svntest.actions.run_and_verify_svn(
    "Commit succeeded when should have failed.",
    None, svntest.verify.AnyOutput,
    'ci', '--editor-cmd', 'no_such-editor')

  err = " ".join([x.strip() for x in err])
  if not (re.match(".*no_such-editor.*", err)
          and re.match(".*Commit failed.*", err)):
    print("Commit failed, but not in the way expected.")
    raise svntest.Failure


def mods_in_schedule_delete(sbox):
  "commit with mods in schedule delete"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Schedule a delete, then put in local mods
  C_path = os.path.join(wc_dir, 'A', 'C')
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'rm', C_path)

  if not os.path.exists(C_path):
    os.mkdir(C_path)
  foo_path = os.path.join(C_path, 'foo')
  foo_contents = 'zig\nzag\n'
  svntest.main.file_append(foo_path, foo_contents)

  # Commit should succeed
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/C')
  expected_output = svntest.wc.State(wc_dir, {
    'A/C' : Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Unversioned file still exists
  actual_contents = open(foo_path).read()
  if actual_contents != foo_contents:
    raise svntest.Failure


#----------------------------------------------------------------------
@Skip(is_non_posix_os_or_cygwin_platform)
@Issue(1954)
def tab_test(sbox):
  "tabs in paths"
  # For issue #1954.

  sbox.build()
  wc_dir = sbox.wc_dir

  tab_file = os.path.join(wc_dir, 'A', "tab\tfile")
  tab_dir  = os.path.join(wc_dir, 'A', "tab\tdir")
  source_url = sbox.repo_url + "/source_dir"
  tab_url = sbox.repo_url + "/tab%09dir"

  svntest.main.file_append(tab_file, "This file has a tab in it.")
  os.mkdir(tab_dir)

  def match_bad_tab_path(path, errlines):
    match_re = ".*: Invalid control character '0x09' in path .*"
    for line in errlines:
      if re.match (match_re, line):
        break
    else:
      raise svntest.Failure("Failed to find match_re in " + str(errlines))

  # add file to wc
  exit_code, outlines, errlines = svntest.main.run_svn(1, 'add', tab_file)
  match_bad_tab_path(tab_file, errlines)

  # add dir to wc
  exit_code, outlines, errlines = svntest.main.run_svn(1, 'add', tab_dir)
  match_bad_tab_path(tab_dir, errlines)

  # mkdir URL
  exit_code, outlines, errlines = svntest.main.run_svn(1, 'mkdir',
                                                       '-m', 'msg', tab_url)
  match_bad_tab_path(tab_dir, errlines)

  # copy URL
  svntest.main.run_svn(1,
                       'mkdir', '-m', 'msg', source_url)
  exit_code, outlines, errlines = svntest.main.run_svn(1, 'copy',
                                                       '-m', 'msg',
                                                       source_url, tab_url)
  match_bad_tab_path(tab_dir, errlines)

  # mv URL
  exit_code, outlines, errlines = svntest.main.run_svn(1, 'mv', '-m', 'msg',
                                                       source_url, tab_url)
  match_bad_tab_path(tab_dir, errlines)

#----------------------------------------------------------------------
@Issue(2285)
def local_mods_are_not_commits(sbox):
  "local ops should not be treated like commits"

  # For issue #2285.
  #
  # Some commands can run on either a URL or a local path.  These
  # commands take a log message, intended for the URL case.
  # Therefore, they should make sure that getting a log message for
  # a local operation errors (because not committing).
  #
  # This is in commit_tests.py because the unifying theme is that
  # commits are *not* happening.  And because there was no better
  # place to put it :-).

  sbox.build()
  wc_dir = sbox.wc_dir
  expected_error = '.*Local, non-commit operations do not take a log message.*'

  # copy wc->wc
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'cp', '-m', 'log msg',
                                     os.path.join(wc_dir, 'iota'),
                                     os.path.join(wc_dir, 'iota2'))

  # copy repos->wc
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'cp', '-m', 'log msg',
                                     sbox.repo_url + "/iota",
                                     os.path.join(wc_dir, 'iota2'))

  # delete
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'rm', '-m', 'log msg',
                                     os.path.join(wc_dir, 'A', 'D', 'gamma'))

  # mkdir
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'mkdir', '-m', 'log msg',
                                     os.path.join(wc_dir, 'newdir'))

  # rename
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'cp', '-m', 'log msg',
                                     os.path.join(wc_dir, 'A', 'mu'),
                                     os.path.join(wc_dir, 'A', 'yu'))


#----------------------------------------------------------------------
# Test if the post-commit error message is returned back to the svn
# client and is displayed as a warning.
@Issue(3553)
def post_commit_hook_test(sbox):
  "post commit hook failure case testing"

  sbox.build()

  # Get paths to the working copy and repository
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  # Create a hook that outputs a message to stderr and returns exit code 1
  # Include a non-XML-safe message to regression-test issue #3553.
  error_msg = "Text with <angle brackets> & ampersand"
  svntest.actions.create_failing_hook(repo_dir, "post-commit", error_msg)

  # Modify iota just so there is something to commit.
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "lakalakalakalaka")

  # Now, commit and examine the output (we happen to know that the
  # filesystem will report an absolute path because that's the way the
  # filesystem is created by this test suite.
  expected_output = [ "Sending        "+ iota_path + "\n",
                      "Transmitting file data .\n",
                      "Committed revision 2.\n",
                      "\n",
                      "Warning: " +
                        svntest.actions.hook_failure_message('post-commit'),
                      error_msg + "\n",
                    ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'ci', '-m', 'log msg', iota_path)

#----------------------------------------------------------------------
# Commit two targets non-recursively, but both targets should be the
# same folder (in multiple variations). Test that svn handles this correctly.
def commit_same_folder_in_targets(sbox):
  "commit two targets, both the same folder"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')

  svntest.main.file_append(iota_path, "added extra line to file iota")

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)

  # Commit the wc_dir and iota.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        '-N',
                                        wc_dir,
                                        iota_path)

#----------------------------------------------------------------------
# test for issue 2459: verify that commit fails when a file with mixed
# eol-styles is included, and show an error message which includes the
# filename.
@Issue(2459)
def commit_inconsistent_eol(sbox):
  "commit files with inconsistent eol should fail"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  svntest.main.run_svn(None, 'propset', 'svn:eol-style', 'native', iota_path)
  svntest.main.file_append_binary(iota_path,
                                  "added extra line to file iota\012"
                                  "added extra line to file iota\015")
  svntest.main.file_append(mu_path, "added extra line to file mu\n"
                                    "added extra line to file mu\n")

  expected_err = ".*iota.*"

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'commit', '-m', 'log message',
                                     wc_dir)


@SkipUnless(server_has_revprop_commit)
def mkdir_with_revprop(sbox):
  "set revision props during remote mkdir"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"

  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'bug=42', remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '42', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def delete_with_revprop(sbox):
  "set revision props during remote delete"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     remote_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'delete', '-m', 'msg',
                                     '--with-revprop', 'bug=52', remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 3:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 3, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '52', [], 'propget', 'bug',
                                     '--revprop', '-r', 3, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def commit_with_revprop(sbox):
  "set revision props during commit"

  sbox.build()
  wc_dir = sbox.wc_dir
  expected_status = make_standard_slew_of_changes(wc_dir)

  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  gloo_path = os.path.join(wc_dir, 'A', 'D', 'H', 'gloo')
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/omega' : Item(verb='Sending'),
    'A/D/H/gloo' : Item(verb='Adding'),
    })

  expected_status.tweak('A/D/H/omega', wc_rev=2, status='  ')
  expected_status.tweak('A/D/H/gloo', wc_rev=2, status='  ')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        '-m', 'msg',
                                        '--with-revprop', 'bug=62',
                                        omega_path, gloo_path)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '62', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def import_with_revprop(sbox):
  "set revision props during import"

  sbox.build()
  local_dir = os.path.join(sbox.wc_dir, 'folder')
  local_file = os.path.join(sbox.wc_dir, 'folder', 'file')
  os.mkdir(local_dir)
  svntest.main.file_write(local_file, "xxxx")

  svntest.actions.run_and_verify_svn(None, None, [], 'import', '-m', 'msg',
                                     '--with-revprop', 'bug=72', local_dir,
                                     sbox.repo_url)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '72', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def copy_R2R_with_revprop(sbox):
  "set revision props during repos-to-repos copy"

  sbox.build()
  remote_dir1 = sbox.repo_url + "/dir1"
  remote_dir2 = sbox.repo_url + "/dir2"
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     remote_dir1)

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', '-m', 'msg',
                                     '--with-revprop', 'bug=82', remote_dir1,
                                     remote_dir2)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 3:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 3, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '82', [], 'propget', 'bug',
                                     '--revprop', '-r', 3, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def copy_WC2R_with_revprop(sbox):
  "set revision props during wc-to-repos copy"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"
  local_dir = os.path.join(sbox.wc_dir, 'folder')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', local_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', '-m', 'msg',
                                     '--with-revprop', 'bug=92', local_dir,
                                     remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '92', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def move_R2R_with_revprop(sbox):
  "set revision props during repos-to-repos move"

  sbox.build()
  remote_dir1 = sbox.repo_url + "/dir1"
  remote_dir2 = sbox.repo_url + "/dir2"
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     remote_dir1)

  svntest.actions.run_and_verify_svn(None, None, [], 'move', '-m', 'msg',
                                     '--with-revprop', 'bug=102', remote_dir1,
                                     remote_dir2)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 3:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 3, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '102', [], 'propget', 'bug',
                                     '--revprop', '-r', 3, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def propedit_with_revprop(sbox):
  "set revision props during remote property edit"

  sbox.build()
  svntest.main.use_editor('append_foo')

  svntest.actions.run_and_verify_svn(None, None, [], 'propedit', '-m', 'msg',
                                     '--with-revprop', 'bug=112', 'prop',
                                     sbox.repo_url)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '112', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def set_multiple_props_with_revprop(sbox):
  "set multiple revision props during remote mkdir"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"

  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'bug=32',
                                     '--with-revprop', 'ref=22', remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n', '  ref\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '32', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '22', [], 'propget', 'ref',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def use_empty_value_in_revprop_pair(sbox):
  "set revprop without value ('') during remote mkdir"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"

  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'bug=',
                                     '--with-revprop', 'ref=', remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n', '  ref\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '', [], 'propget', 'ref',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def no_equals_in_revprop_pair(sbox):
  "set revprop without '=' during remote mkdir"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'bug',
                                     '--with-revprop', 'ref', remote_dir)

  expected = svntest.verify.UnorderedOutput(
                  ['Unversioned properties on revision 2:\n',
                   '  svn:author\n','  svn:date\n',  '  svn:log\n',
                   '  bug\n', '  ref\n'])
  svntest.actions.run_and_verify_svn(None, expected, [], 'proplist',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '', [], 'propget', 'bug',
                                     '--revprop', '-r', 2, sbox.repo_url)
  svntest.actions.run_and_verify_svn(None, '', [], 'propget', 'ref',
                                     '--revprop', '-r', 2, sbox.repo_url)


@SkipUnless(server_has_revprop_commit)
def set_invalid_revprops(sbox):
  "set invalid revision props during remote mkdir"

  sbox.build()
  remote_dir = sbox.repo_url + "/dir"
  # Try to set svn: revprops.
  expected = '.*Standard properties can\'t.*'
  svntest.actions.run_and_verify_svn(None, [], expected, 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'svn:author=42', remote_dir)
  svntest.actions.run_and_verify_svn(None, [], expected, 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'svn:log=42', remote_dir)
  svntest.actions.run_and_verify_svn(None, [], expected, 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'svn:date=42', remote_dir)
  svntest.actions.run_and_verify_svn(None, [], expected, 'mkdir', '-m', 'msg',
                                     '--with-revprop', 'svn:foo=bar', remote_dir)

  # Empty revprop pair.
  svntest.actions.run_and_verify_svn(None, [],
                                     'svn: E205000: '
                                     'Revision property pair is empty',
                                     'mkdir', '-m', 'msg',
                                     '--with-revprop', '',
                                     remote_dir)

#----------------------------------------------------------------------
@Issue(3553)
def start_commit_hook_test(sbox):
  "start-commit hook failure case testing"

  sbox.build()

  # Get paths to the working copy and repository
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  # Create a hook that outputs a message to stderr and returns exit code 1
  # Include a non-XML-safe message to regression-test issue #3553.
  error_msg = "Text with <angle brackets> & ampersand"
  svntest.actions.create_failing_hook(repo_dir, "start-commit", error_msg)

  # Modify iota just so there is something to commit.
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "More stuff in iota")

  # Commit, expect error code 1
  exit_code, actual_stdout, actual_stderr = svntest.main.run_svn(
    1, 'ci', '--quiet', '-m', 'log msg', wc_dir)

  # No stdout expected
  svntest.verify.compare_and_display_lines('Start-commit hook test',
                                           'STDOUT', [], actual_stdout)

  # Compare only the last two lines of stderr since the preceding ones
  # contain source code file and line numbers.
  if len(actual_stderr) > 2:
    actual_stderr = actual_stderr[-2:]
  expected_stderr = [ "svn: E165001: " +
                        svntest.actions.hook_failure_message('start-commit'),
                      error_msg + "\n",
                    ]
  svntest.verify.compare_and_display_lines('Start-commit hook test',
                                           'STDERR',
                                           expected_stderr, actual_stderr)

#----------------------------------------------------------------------
@Issue(3553)
def pre_commit_hook_test(sbox):
  "pre-commit hook failure case testing"

  sbox.build()

  # Get paths to the working copy and repository
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  # Create a hook that outputs a message to stderr and returns exit code 1
  # Include a non-XML-safe message to regression-test issue #3553.
  error_msg = "Text with <angle brackets> & ampersand"
  svntest.actions.create_failing_hook(repo_dir, "pre-commit", error_msg)

  # Modify iota just so there is something to commit.
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "More stuff in iota")

  # Commit, expect error code 1
  exit_code, actual_stdout, actual_stderr = svntest.main.run_svn(
    1, 'ci', '--quiet', '-m', 'log msg', wc_dir)

  # No stdout expected
  svntest.verify.compare_and_display_lines('Pre-commit hook test',
                                           'STDOUT', [], actual_stdout)

  # Compare only the last two lines of stderr since the preceding ones
  # contain source code file and line numbers.
  if len(actual_stderr) > 2:
    actual_stderr = actual_stderr[-2:]
  expected_stderr = [ "svn: E165001: " +
                        svntest.actions.hook_failure_message('pre-commit'),
                      error_msg + "\n",
                    ]
  svntest.verify.compare_and_display_lines('Pre-commit hook test',
                                           'STDERR',
                                           expected_stderr, actual_stderr)

#----------------------------------------------------------------------

def versioned_log_message(sbox):
  "'svn commit -F foo' when foo is a versioned file"

  sbox.build()

  os.chdir(sbox.wc_dir)

  iota_path = os.path.join('iota')
  mu_path = os.path.join('A', 'mu')
  log_path = os.path.join('A', 'D', 'H', 'omega')

  svntest.main.file_append(iota_path, "2")

  # try to check in a change using a versioned file as your log entry.
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'ci', '-F', log_path)

  # force it.  should not produce any errors.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-F', log_path, '--force-log')

  svntest.main.file_append(mu_path, "2")

  # try the same thing, but specifying the file to commit explicitly.
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'ci', '-F', log_path, mu_path)

  # force it...  should succeed.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-F', log_path,
                                     '--force-log', mu_path)

#----------------------------------------------------------------------

def changelist_near_conflict(sbox):
  "'svn commit --changelist=foo' above a conflict"

  sbox.build()

  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, "iota")
  mu_path = os.path.join(wc_dir, "A", "mu")
  gloo_path = os.path.join(wc_dir, "A", "D", "H", "gloo")

  expected_status = make_standard_slew_of_changes(wc_dir)

  # Create a changelist.
  changelist_name = "logical-changeset"
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "changelist", changelist_name,
                                     mu_path, gloo_path)

  # Create a conflict (making r2 in the process).
  inject_conflict_into_wc(sbox, 'iota', iota_path,
                          None, expected_status, 2)

  # Commit the changelist.
  expected_output = svntest.wc.State(wc_dir, {
    "A/D/H/gloo" : Item(verb='Adding'),
    })
  expected_status.tweak("A/D/H/gloo", wc_rev=3, status="  ")
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        "--changelist=" + changelist_name,
                                        "-m", "msg", wc_dir)


#----------------------------------------------------------------------

def commit_out_of_date_file(sbox):
  "try to commit a file that is out-of-date"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  backup_pi_path = os.path.join(wc_backup, 'A', 'D', 'G', 'pi')

  svntest.main.file_append(pi_path, "new line\n")
  expected_output = svntest.wc.State(wc_dir, {
    "A/D/G/pi" : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak("A/D/G/pi", wc_rev=2, status="  ")
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        "-m", "log message", wc_dir)

  svntest.main.file_append(backup_pi_path, "hello")
  expected_err = ".*(pi.*out of date|Out of date.*pi).*"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'commit', '-m', 'log message',
                                     wc_backup)

@SkipUnless(server_gets_client_capabilities)
@Issue(2991)
def start_commit_detect_capabilities(sbox):
  "start-commit hook sees client capabilities"  # Issue #2991
  sbox.build()
  wc_dir = sbox.wc_dir
  repos_dir = sbox.repo_dir

  # Create a start-commit hook that detects the "mergeinfo" capability.
  hook_text = "import sys\n"                                 + \
              "fp = open(sys.argv[1] + '/hooks.log', 'w')\n" + \
              "caps = sys.argv[3].split(':')\n"              + \
              "if 'mergeinfo' in caps:\n"                    + \
              "  fp.write('yes')\n"                          + \
              "else:\n"                                      + \
              "  fp.write('no')\n"                           + \
              "fp.close()\n"

  start_commit_hook = svntest.main.get_start_commit_hook_path(repos_dir)
  svntest.main.create_python_hook_script(start_commit_hook, hook_text)

  # Commit something.
  iota_path = os.path.join(wc_dir, "iota")
  svntest.main.file_append(iota_path, "More stuff in iota")
  svntest.actions.run_and_verify_svn(None, [], [], 'ci', '--quiet',
                                     '-m', 'log msg', wc_dir)

  # Check that "mergeinfo" was detected.
  log_path = os.path.join(repos_dir, "hooks.log")
  if os.path.exists(log_path):
    data = open(log_path).read()
    os.unlink(log_path)
  else:
    raise svntest.verify.SVNUnexpectedOutput("'%s' not found") % log_path
  if data != 'yes':
    raise svntest.Failure

# Test for issue #3198
@Issue(3198)
def commit_added_missing(sbox):
  "commit a missing to-be-added file should fail"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  a_path = os.path.join(wc_dir, 'A', 'a.txt')
  b_path = os.path.join(wc_dir, 'A', 'b.txt')

  # Make two copies of mu: a and b
  svntest.main.run_svn(None, 'cp', mu_path, a_path)
  svntest.main.run_svn(None, 'cp', mu_path, b_path)

  # remove b, make it missing
  os.remove(b_path)

  # Commit, hoping to see an error
  svntest.actions.run_and_verify_svn("Commit should have failed",
                                     [], ".* is scheduled for addition, but is missing",
                                     'commit', '-m', 'logmsg', wc_dir)

#----------------------------------------------------------------------

# Helper for commit-failure tests
def commit_fails_at_path(path, wc_dir, error_re):
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        error_re,
                                        path)

def tree_conflicts_block_commit(sbox):
  "tree conflicts block commit"

  # Commit is not allowed in a directory containing tree conflicts.
  # This test corresponds to use cases 1-3 (with file victims) in
  # notes/tree-conflicts/use-cases.txt.

  svntest.actions.build_greek_tree_conflicts(sbox)
  wc_dir = sbox.wc_dir
  A = os.path.join(wc_dir, 'A')
  D = os.path.join(wc_dir, 'A', 'D')
  G = os.path.join(wc_dir, 'A', 'D', 'G')

  error_re = "remains in conflict"
  commit_fails_at_path(wc_dir, wc_dir, error_re)
  commit_fails_at_path(A, A, error_re)
  commit_fails_at_path(D, D, error_re)
  commit_fails_at_path(G, G, error_re)
  commit_fails_at_path(os.path.join(G, 'pi'), G, error_re)


def tree_conflicts_resolved(sbox):
  "tree conflicts resolved"

  # Commit is allowed after tree conflicts are resolved.
  # This test corresponds to use cases 1-3 in
  # notes/tree-conflicts/use-cases.txt.

  svntest.actions.build_greek_tree_conflicts(sbox)
  wc_dir = sbox.wc_dir

  # Duplicate wc for tests
  wc_dir_2 = sbox.add_wc_path('2')
  svntest.actions.duplicate_dir(wc_dir, wc_dir_2)

  # Mark the tree conflict victims as resolved
  G = os.path.join(wc_dir, 'A', 'D', 'G')
  victims = [ os.path.join(G, v) for v in ['pi', 'rho', 'tau'] ]
  svntest.actions.run_and_verify_resolved(victims)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/D/G/pi',  status='D ')
  expected_status.tweak('A/D/G/rho', status='A ', copied='+', wc_rev='-')
  expected_status.remove('A/D/G/tau')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Recursively resolved in parent directory -- expect same result
  G2 = os.path.join(wc_dir_2, 'A', 'D', 'G')
  victims = [ os.path.join(G2, v) for v in ['pi', 'rho', 'tau'] ]
  svntest.actions.run_and_verify_resolved(victims, G2, '-R')

  expected_status.wc_dir = wc_dir_2
  svntest.actions.run_and_verify_status(wc_dir_2, expected_status)

#----------------------------------------------------------------------
def commit_multiple_nested_deletes(sbox):
  "committing multiple nested deletes"

  sbox.build()
  wc_dir = sbox.wc_dir

  A = os.path.join(wc_dir, 'A')
  A_B = os.path.join(A, 'B')

  sbox.simple_rm('A')

  svntest.main.run_svn(None, 'ci', A, A_B, '-m', 'Q')

@Issue(4042)
def commit_incomplete(sbox):
  "commit an incomplete dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_propset('pname', 'pval', 'A/B')
  svntest.actions.set_incomplete(sbox.ospath('A/B'), 1)

  expected_output = svntest.wc.State(wc_dir, {
      'A/B' : Item(verb='Sending'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B',  status='! ', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)
  
#----------------------------------------------------------------------
# Reported here:
#   Message-ID: <4EBF0FC9.300@gmail.com>
#   Date: Sun, 13 Nov 2011 13:31:05 +1300
#   From: Fergus Slorach <sugref@gmail.com>
#   Subject: svn commit --targets behaviour change in 1.7?
def commit_add_subadd(sbox):
  "committing add with explicit subadd targets"

  sbox.build()
  wc_dir = sbox.wc_dir

  targets_file = sbox.ospath('targets') # ### better tempdir?
  targets_file = os.path.abspath(targets_file)

  # prepare targets file
  targets = "A/D A/D/H A/D/H/chi A/D/H/omega A/D/H/psi".split()
  open(targets_file, 'w').write("\n".join(targets))

  # r2: rm A/D
  sbox.simple_rm('A/D')
  sbox.simple_commit(message='rm')

  # r3: revert r2, with specific invocation
  os.chdir(wc_dir)
  svntest.main.run_svn(None, 'up')
  svntest.main.run_svn(None, 'merge', '-c', '-2', './')
  svntest.main.run_svn(None, 'commit', '--targets', targets_file, '-mm')


########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              commit_one_file,
              commit_one_new_file,
              commit_one_new_binary_file,
              commit_multiple_targets,
              commit_multiple_targets_2,
              commit_inclusive_dir,
              commit_top_dir,
              commit_unversioned_thing,
              nested_dir_replacements,
              hudson_part_1,
              hudson_part_1_variation_1,
              hudson_part_1_variation_2,
              hudson_part_2,
              hudson_part_2_1,
              hook_test,
              merge_mixed_revisions,
              commit_uri_unsafe,
              commit_deleted_edited,
              commit_in_dir_scheduled_for_addition,
              commit_rmd_and_deleted_file,
              commit_add_file_twice,
              commit_from_long_dir,
              commit_with_lock,
              commit_current_dir,
              commit_multiple_wc_nested,
              commit_multiple_wc,
              commit_multiple_wc_multiple_repos,
              commit_nonrecursive,
              failed_commit,
              commit_out_of_date_deletions,
              commit_with_bad_log_message,
              commit_with_mixed_line_endings,
              commit_with_mixed_line_endings_in_ignored_part,
              from_wc_top_with_bad_editor,
              mods_in_schedule_delete,
              tab_test,
              local_mods_are_not_commits,
              post_commit_hook_test,
              commit_same_folder_in_targets,
              commit_inconsistent_eol,
              mkdir_with_revprop,
              delete_with_revprop,
              commit_with_revprop,
              import_with_revprop,
              copy_R2R_with_revprop,
              copy_WC2R_with_revprop,
              move_R2R_with_revprop,
              propedit_with_revprop,
              set_multiple_props_with_revprop,
              use_empty_value_in_revprop_pair,
              no_equals_in_revprop_pair,
              set_invalid_revprops,
              start_commit_hook_test,
              pre_commit_hook_test,
              versioned_log_message,
              changelist_near_conflict,
              commit_out_of_date_file,
              start_commit_detect_capabilities,
              commit_added_missing,
              tree_conflicts_block_commit,
              tree_conflicts_resolved,
              commit_multiple_nested_deletes,
              commit_incomplete,
              commit_add_subadd,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
