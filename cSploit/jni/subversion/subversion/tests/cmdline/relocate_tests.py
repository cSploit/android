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
from switch_tests import do_routine_switching

#----------------------------------------------------------------------

def relocate_deleted_missing_copied(sbox):
  "relocate with deleted, missing and copied entries"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete A/mu to create a deleted entry for mu in A/.svn/entries
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', mu_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/mu')
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Remove A/B/F to create a missing entry
  svntest.main.safe_rmtree(os.path.join(wc_dir, 'A', 'B', 'F'))

  # Copy A/D to A/D2
  D_path = os.path.join(wc_dir, 'A', 'D')
  D2_path = os.path.join(wc_dir, 'A', 'D2')
  svntest.actions.run_and_verify_svn(None, None, [], 'copy',
                                     D_path, D2_path)
  # Delete within the copy
  D2G_path = os.path.join(wc_dir, 'A', 'D2', 'G')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', D2G_path)

  expected_status.add({
    'A/D2'         : Item(status='A ', wc_rev='-', copied='+'),
    'A/D2/gamma'   : Item(status='  ', wc_rev='-', copied='+'),
    'A/D2/G'       : Item(status='D ', wc_rev='-', copied='+'),
    'A/D2/G/pi'    : Item(status='D ', wc_rev='-', copied='+'),
    'A/D2/G/rho'   : Item(status='D ', wc_rev='-', copied='+'),
    'A/D2/G/tau'   : Item(status='D ', wc_rev='-', copied='+'),
    'A/D2/H'       : Item(status='  ', wc_rev='-', copied='+'),
    'A/D2/H/chi'   : Item(status='  ', wc_rev='-', copied='+'),
    'A/D2/H/omega' : Item(status='  ', wc_rev='-', copied='+'),
    'A/D2/H/psi'   : Item(status='  ', wc_rev='-', copied='+'),
    })
  if svntest.main.wc_is_singledb(wc_dir):
    expected_status.tweak('A/B/F', status='! ', wc_rev='1')
  else:
    expected_status.tweak('A/B/F', status='! ', wc_rev='?')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Relocate
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 2, 0)
  svntest.main.safe_rmtree(repo_dir, 1)
  svntest.actions.run_and_verify_svn(None, None, [], 'switch', '--relocate',
                                     repo_url, other_repo_url, wc_dir)

  # Deleted and missing entries should be preserved, so update should
  # show only A/B/F being reinstated
  if svntest.main.wc_is_singledb(wc_dir):
    expected_output = svntest.wc.State(wc_dir, {
        'A/B/F' : Item(verb='Restored'),
        })
  else:
    expected_output = svntest.wc.State(wc_dir, {
        'A/B/F' : Item(status='A '),
        })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/mu')
  expected_disk.add({
    'A/D2'       : Item(),
    'A/D2/gamma'   : Item("This is the file 'gamma'.\n"),
    'A/D2/H'       : Item(),
    'A/D2/H/chi'   : Item("This is the file 'chi'.\n"),
    'A/D2/H/omega' : Item("This is the file 'omega'.\n"),
    'A/D2/H/psi'   : Item("This is the file 'psi'.\n"),
    })
  if not svntest.main.wc_is_singledb(wc_dir):
    expected_disk.add({
        'A/D2/G'       : Item(),
        })
  expected_status.add({
    'A/B/F'       : Item(status='  ', wc_rev='2'),
    })
  expected_status.tweak(wc_rev=2)
  expected_status.tweak('A/D2', 'A/D2/gamma',
                        'A/D2/H', 'A/D2/H/chi', 'A/D2/H/omega', 'A/D2/H/psi',
                        wc_rev='-')
  expected_status.tweak('A/D2/G', 'A/D2/G/pi', 'A/D2/G/rho', 'A/D2/G/tau',
                        copied='+', wc_rev='-')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # Commit to verify that copyfrom URLs have been relocated
  expected_output = svntest.wc.State(wc_dir, {
    'A/D2'       : Item(verb='Adding'),
    'A/D2/G'     : Item(verb='Deleting'),
    })
  expected_status.tweak('A/D2', 'A/D2/gamma',
                        'A/D2/H', 'A/D2/H/chi', 'A/D2/H/omega', 'A/D2/H/psi',
                        status='  ', wc_rev='3', copied=None)
  expected_status.remove('A/D2/G', 'A/D2/G/pi', 'A/D2/G/rho', 'A/D2/G/tau')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

@Issue(2380)
def relocate_beyond_repos_root(sbox):
  "relocate with prefixes longer than repo root"
  sbox.build(read_only=True, create_wc=False)

  wc_backup = sbox.add_wc_path('backup')

  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  A_url = repo_url + "/A"
  A_wc_dir = wc_dir
  other_A_url = other_repo_url + "/A"
  other_B_url = other_repo_url + "/B"

  svntest.main.safe_rmtree(wc_dir, 1)
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     repo_url + '/A', wc_dir)

  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 0)

  # A relocate that changes the repo path part of the URL shouldn't work.
  # This tests for issue #2380.
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*Invalid relocation destination.*",
                                     'relocate',
                                     A_url, other_B_url, A_wc_dir)

  # Another way of trying to change the fs path, leading to an invalid
  # repository root.
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*is not the root.*",
                                     'relocate',
                                     repo_url, other_B_url, A_wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'relocate',
                                     A_url, other_A_url, A_wc_dir)

  # Check that we can contact the repository, meaning that the
  # relocate actually changed the URI.  Escape the expected URI to
  # avoid problems from any regex meta-characters it may contain
  # (e.g. '+').
  expected_infos = [
      { 'URL'                : re.escape(other_A_url) + '$',
        'Path'               : '.+',
        'Repository UUID'    : '.+',
        'Revision'           : '.+',
        'Node Kind'          : '.+',
        'Last Changed Date'  : '.+' },
    ]
  svntest.actions.run_and_verify_info(expected_infos, A_wc_dir, '-rHEAD')

#----------------------------------------------------------------------
# Issue 2578.
def relocate_and_propset(sbox):
  "out of date propset should fail after a relocate"

  # Create virgin repos and working copy
  svntest.main.safe_rmtree(sbox.repo_dir, 1)
  svntest.main.create_repos(sbox.repo_dir)

  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url

  # import the greek tree
  svntest.main.greek_state.write_to_disk(svntest.main.greek_dump_dir)
  exit_code, output, errput = svntest.main.run_svn(
    None, 'import', '-m', 'Log message for revision 1.',
    svntest.main.greek_dump_dir, sbox.repo_url)

  # checkout
  svntest.main.safe_rmtree(wc_dir, 1)
  svntest.actions.run_and_verify_svn(None,
                                     None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Relocate
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 0)
  svntest.main.safe_rmtree(repo_dir, 1)
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     repo_url, other_repo_url, wc_dir)

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

def single_file_relocate(sbox):
  "relocate a single file"

  # Create virgin repos and working copy
  svntest.main.safe_rmtree(sbox.repo_dir, 1)
  svntest.main.create_repos(sbox.repo_dir)

  wc_dir = sbox.wc_dir
  iota_path = os.path.join(sbox.wc_dir, 'iota')
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  iota_url = repo_url + '/iota'

  # import the greek tree
  svntest.main.greek_state.write_to_disk(svntest.main.greek_dump_dir)
  exit_code, output, errput = svntest.main.run_svn(
    None, 'import', '-m', 'Log message for revision 1.',
    svntest.main.greek_dump_dir, sbox.repo_url)

  # checkout
  svntest.main.safe_rmtree(wc_dir, 1)
  svntest.actions.run_and_verify_svn(None,
                                     None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Relocate
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  other_iota_url = other_repo_url + '/iota'
  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 0)
  svntest.main.safe_rmtree(repo_dir, 1)
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*Cannot relocate.*",
                                     'relocate',
                                     iota_url, other_iota_url, iota_path)

#----------------------------------------------------------------------

def relocate_with_switched_children(sbox):
  "relocate a directory with switched children"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup (and verify) some switched things
  do_routine_switching(sbox.wc_dir, sbox.repo_url, False)

  # Relocate
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 0)
  svntest.main.safe_rmtree(repo_dir, 1)

  # Do the switch and check the results in three ways.
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     repo_url, other_repo_url, wc_dir)

  # Attempt to commit changes and examine results
  expected_output = svntest.wc.State(wc_dir, { })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B', 'iota',
                        switched='S')
  expected_status.remove('A/B/E', 'A/B/F', 'A/B/E/alpha', 'A/B/E/beta',
                         'A/B/lambda')
  expected_status.add({
    'A/B/pi'       : Item(status='  ', wc_rev='1'),
    'A/B/rho'      : Item(status='  ', wc_rev='1'),
    'A/B/tau'      : Item(status='  ', wc_rev='1'),
    })

  # This won't actually do a commit, because nothing should be modified.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Check the URLs of various nodes.
  info_output = {
        wc_dir:                                '.*.other$',
        os.path.join(wc_dir, 'iota'):          '.*.other/A/D/gamma$',
        os.path.join(wc_dir, 'A', 'B'):        '.*.other/A/D/G$',
        os.path.join(wc_dir, 'A', 'B', 'pi'):  '.*.other/A/D/G/pi$',
    }

  for path, pattern in info_output.items():
    expected_info = { 'URL' : pattern }
    svntest.actions.run_and_verify_info([expected_info], path)

#----------------------------------------------------------------------


### regression test for issue #3597
@Issue(3597)
def relocate_with_relative_externals(sbox):
  "relocate a directory containing relative externals"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a relative external.
  change_external(os.path.join(wc_dir, 'A', 'B'),
                  "^/A/D/G G-ext\n../D/H H-ext", commit=True)
  svntest.actions.run_and_verify_svn(None, None, [], 'update', wc_dir)

  # Move our repository to another location.
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 2, 0)
  svntest.main.safe_rmtree(repo_dir, 1)

  # Now relocate our working copy.
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     repo_url, other_repo_url, wc_dir)

  # Check the URLs of the externals -- were they updated to point to the
  # .other repository URL?
  svntest.actions.run_and_verify_info([{ 'URL' : '.*.other/A/D/G$' }],
                                      os.path.join(wc_dir, 'A', 'B', 'G-ext'))
  svntest.actions.run_and_verify_info([{ 'URL' : '.*.other/A/D/H$' }],
                                      os.path.join(wc_dir, 'A', 'B', 'H-ext'))

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              relocate_deleted_missing_copied,
              relocate_beyond_repos_root,
              relocate_and_propset,
              single_file_relocate,
              relocate_with_switched_children,
              relocate_with_relative_externals,
              ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
