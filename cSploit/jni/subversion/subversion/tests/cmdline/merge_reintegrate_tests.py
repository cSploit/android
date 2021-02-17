#!/usr/bin/env python
#
#  merge_reintegrate_tests.py:  testing merge --reintegrate
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
import shutil, sys, re, os
import time

# Our testing module
import svntest
from svntest import main, wc, verify, actions

# (abbreviation)
Item = wc.StateItem
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
exp_noop_up_out = svntest.actions.expected_noop_update_output

from svntest.main import SVN_PROP_MERGEINFO
from svntest.main import server_has_mergeinfo
from merge_tests import set_up_branch
from merge_tests import expected_merge_output

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
@Issue(3640)
def basic_reintegrate(sbox):
  "basic merge --reintegrate support"

  # Also includes test for issue #3640 'moved target breaks reintegrate merge'

  # Make A_COPY branch in r2, and do a few more commits to A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Make a change on the branch, to A/mu.  Commit in r7.
  svntest.main.file_write(os.path.join(wc_dir, "A_COPY", "mu"),
                          "Changed on the branch.")
  expected_output = wc.State(wc_dir, {'A_COPY/mu' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/mu', wc_rev=7)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/mu', contents='Changed on the branch.')

  # Update the wcs.
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='7')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # Merge from trunk to branch (ie, r3-6), using normal cherry-harvest.
  A_COPY_path = os.path.join(wc_dir, "A_COPY")
  expected_output = wc.State(A_COPY_path, {
    'D/H/psi'   : Item(status='U '),
    'D/G/rho'   : Item(status='U '),
    'B/E/beta'  : Item(status='U '),
    'D/H/omega' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_COPY_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_COPY_path, {
    })
  k_expected_status = wc.State(A_COPY_path, {
    "B"         : Item(status='  ', wc_rev=7),
    "B/lambda"  : Item(status='  ', wc_rev=7),
    "B/E"       : Item(status='  ', wc_rev=7),
    "B/E/alpha" : Item(status='  ', wc_rev=7),
    "B/E/beta"  : Item(status='M ', wc_rev=7),
    "B/F"       : Item(status='  ', wc_rev=7),
    "mu"        : Item(status='  ', wc_rev=7),
    "C"         : Item(status='  ', wc_rev=7),
    "D"         : Item(status='  ', wc_rev=7),
    "D/gamma"   : Item(status='  ', wc_rev=7),
    "D/G"       : Item(status='  ', wc_rev=7),
    "D/G/pi"    : Item(status='  ', wc_rev=7),
    "D/G/rho"   : Item(status='M ', wc_rev=7),
    "D/G/tau"   : Item(status='  ', wc_rev=7),
    "D/H"       : Item(status='  ', wc_rev=7),
    "D/H/chi"   : Item(status='  ', wc_rev=7),
    "D/H/omega" : Item(status='M ', wc_rev=7),
    "D/H/psi"   : Item(status='M ', wc_rev=7),
    ""          : Item(status=' M', wc_rev=7),
  })
  k_expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A:2-7'}),
    'B'         : Item(),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/F'       : Item(),
    'mu'        : Item("Changed on the branch."),
    'C'         : Item(),
    'D'         : Item(),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/omega' : Item("New content"),
    'D/H/psi'   : Item("New content"),
  })
  expected_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_COPY_path, None, None,
                                       sbox.repo_url + '/A', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       k_expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True)
  expected_disk.tweak('A_COPY', props={SVN_PROP_MERGEINFO: '/A:2-7'})
  expected_disk.tweak('A_COPY/B/E/beta', contents="New content")
  expected_disk.tweak('A_COPY/D/G/rho', contents="New content")
  expected_disk.tweak('A_COPY/D/H/omega', contents="New content")
  expected_disk.tweak('A_COPY/D/H/psi', contents="New content")

  # Commit the merge to branch (r8).
  expected_output = wc.State(wc_dir, {
    'A_COPY/D/H/psi'   : Item(verb='Sending'),
    'A_COPY/D/G/rho'   : Item(verb='Sending'),
    'A_COPY/B/E/beta'  : Item(verb='Sending'),
    'A_COPY/D/H/omega' : Item(verb='Sending'),
    'A_COPY'           : Item(verb='Sending'),
    })
  expected_status.tweak('A_COPY', 'A_COPY/D/H/psi', 'A_COPY/D/G/rho',
                        'A_COPY/B/E/beta', 'A_COPY/D/H/omega', wc_rev=8)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update the wcs again.
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='8')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)


  # *finally*, actually run merge --reintegrate in trunk with the
  # branch URL.  This should bring in the mu change and the tauprime
  # change.
  A_path = os.path.join(wc_dir, "A")
  expected_output = wc.State(A_path, {
    'mu'           : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  k_expected_status = wc.State(A_path, {
    "B"            : Item(status='  ', wc_rev=8),
    "B/lambda"     : Item(status='  ', wc_rev=8),
    "B/E"          : Item(status='  ', wc_rev=8),
    "B/E/alpha"    : Item(status='  ', wc_rev=8),
    "B/E/beta"     : Item(status='  ', wc_rev=8),
    "B/F"          : Item(status='  ', wc_rev=8),
    "mu"           : Item(status='M ', wc_rev=8),
    "C"            : Item(status='  ', wc_rev=8),
    "D"            : Item(status='  ', wc_rev=8),
    "D/gamma"      : Item(status='  ', wc_rev=8),
    "D/G"          : Item(status='  ', wc_rev=8),
    "D/G/pi"       : Item(status='  ', wc_rev=8),
    "D/G/rho"      : Item(status='  ', wc_rev=8),
    "D/G/tau"      : Item(status='  ', wc_rev=8),
    "D/H"          : Item(status='  ', wc_rev=8),
    "D/H/chi"      : Item(status='  ', wc_rev=8),
    "D/H/omega"    : Item(status='  ', wc_rev=8),
    "D/H/psi"      : Item(status='  ', wc_rev=8),
    ""             : Item(status=' M', wc_rev=8),
  })
  k_expected_disk.tweak('', props={SVN_PROP_MERGEINFO : '/A_COPY:2-8'})
  expected_skip = wc.State(A_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       k_expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True, True,
                                       '--reintegrate', A_path)

  # Test issue #3640:
  #
  # Revert the merge then move A to A_MOVED in r9.  Repeat the merge, but
  # targeting A_MOVED this time.  This should work with almost the same
  # results.  The only differences being the inclusion of r9 in the
  # mergeinfo and the A-->A_MOVED path difference.
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 9.\n'],
                                     [], 'move',
                                     sbox.repo_url + '/A',
                                     sbox.repo_url + '/A_MOVED',
                                     '-m', 'Copy A to A_MOVED')
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  A_MOVED_path = os.path.join(wc_dir, "A_MOVED")
  expected_output = wc.State(A_MOVED_path, {
    'mu'           : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_MOVED_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_MOVED_path, {
    })
  expected_status = wc.State(A_MOVED_path, {
    "B"            : Item(status='  '),
    "B/lambda"     : Item(status='  '),
    "B/E"          : Item(status='  '),
    "B/E/alpha"    : Item(status='  '),
    "B/E/beta"     : Item(status='  '),
    "B/F"          : Item(status='  '),
    "mu"           : Item(status='M '),
    "C"            : Item(status='  '),
    "D"            : Item(status='  '),
    "D/gamma"      : Item(status='  '),
    "D/G"          : Item(status='  '),
    "D/G/pi"       : Item(status='  '),
    "D/G/rho"      : Item(status='  '),
    "D/G/tau"      : Item(status='  '),
    "D/H"          : Item(status='  '),
    "D/H/chi"      : Item(status='  '),
    "D/H/omega"    : Item(status='  '),
    "D/H/psi"      : Item(status='  '),
    ""             : Item(status=' M'),
  })
  expected_status.tweak(wc_rev=9)
  k_expected_disk.tweak('', props={SVN_PROP_MERGEINFO : '/A_COPY:2-9'})
  expected_skip = wc.State(A_MOVED_path, {})
  svntest.actions.run_and_verify_merge(A_MOVED_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True, True,
                                       '--reintegrate', A_MOVED_path)

#----------------------------------------------------------------------
def reintegrate_with_rename(sbox):
  "merge --reintegrate with renamed file on branch"

  # Make A_COPY branch in r2, and do a few more commits to A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Make a change on the branch, to A/mu.  Commit in r7.
  svntest.main.file_write(os.path.join(wc_dir, "A_COPY", "mu"),
                          "Changed on the branch.")
  expected_output = wc.State(wc_dir, {'A_COPY/mu' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/mu', wc_rev=7)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/mu', contents='Changed on the branch.')

  # Update the wcs.
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='7')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # Merge from trunk to branch (ie, r3-6), using normal cherry-harvest.
  A_COPY_path = os.path.join(wc_dir, "A_COPY")
  expected_output = wc.State(A_COPY_path, {
    'D/H/psi'   : Item(status='U '),
    'D/G/rho'   : Item(status='U '),
    'B/E/beta'  : Item(status='U '),
    'D/H/omega' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_COPY_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_COPY_path, {
    })
  k_expected_status = wc.State(A_COPY_path, {
    "B"         : Item(status='  ', wc_rev=7),
    "B/lambda"  : Item(status='  ', wc_rev=7),
    "B/E"       : Item(status='  ', wc_rev=7),
    "B/E/alpha" : Item(status='  ', wc_rev=7),
    "B/E/beta"  : Item(status='M ', wc_rev=7),
    "B/F"       : Item(status='  ', wc_rev=7),
    "mu"        : Item(status='  ', wc_rev=7),
    "C"         : Item(status='  ', wc_rev=7),
    "D"         : Item(status='  ', wc_rev=7),
    "D/gamma"   : Item(status='  ', wc_rev=7),
    "D/G"       : Item(status='  ', wc_rev=7),
    "D/G/pi"    : Item(status='  ', wc_rev=7),
    "D/G/rho"   : Item(status='M ', wc_rev=7),
    "D/G/tau"   : Item(status='  ', wc_rev=7),
    "D/H"       : Item(status='  ', wc_rev=7),
    "D/H/chi"   : Item(status='  ', wc_rev=7),
    "D/H/omega" : Item(status='M ', wc_rev=7),
    "D/H/psi"   : Item(status='M ', wc_rev=7),
    ""          : Item(status=' M', wc_rev=7),
  })
  k_expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A:2-7'}),
    'B'         : Item(),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/F'       : Item(),
    'mu'        : Item("Changed on the branch."),
    'C'         : Item(),
    'D'         : Item(),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/omega' : Item("New content"),
    'D/H/psi'   : Item("New content"),
  })
  expected_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_COPY_path, None, None,
                                       sbox.repo_url + '/A', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       k_expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True)
  expected_disk.tweak('A_COPY', props={SVN_PROP_MERGEINFO: '/A:2-7'})
  expected_disk.tweak('A_COPY/B/E/beta', contents="New content")
  expected_disk.tweak('A_COPY/D/G/rho', contents="New content")
  expected_disk.tweak('A_COPY/D/H/omega', contents="New content")
  expected_disk.tweak('A_COPY/D/H/psi', contents="New content")

  # Commit the merge to branch (r8).
  expected_output = wc.State(wc_dir, {
    'A_COPY/D/H/psi'   : Item(verb='Sending'),
    'A_COPY/D/G/rho'   : Item(verb='Sending'),
    'A_COPY/B/E/beta'  : Item(verb='Sending'),
    'A_COPY/D/H/omega' : Item(verb='Sending'),
    'A_COPY'           : Item(verb='Sending'),
    })
  expected_status.tweak('A_COPY', 'A_COPY/D/H/psi', 'A_COPY/D/G/rho',
                        'A_COPY/B/E/beta', 'A_COPY/D/H/omega', wc_rev=8)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)


  # Update the wcs again.
  #
  # Note: this update had to be added because of r869016 (which was
  # merged into the reintegrate branch in r869021).  Without this
  # update, the mergeinfo will not be inherited properly as part of
  # the 'svn cp tau tauprime' step, and later (during the post-commit
  # update, with the new expected_disk) we'll get an error like this:
  #
  #   =============================================================
  #   Expected 'tauprime' and actual 'tauprime' in disk tree are different!
  #   =============================================================
  #   EXPECTED NODE TO BE:
  #   =============================================================
  #    * Node name:   tauprime
  #       Path:       A_COPY/D/G/tauprime
  #       Contents:   This is the file 'tau'.
  #
  #       Properties: {'svn:mergeinfo': '/A/D/G/tau:2-7'}
  #       Attributes: {}
  #       Children:   N/A (node is a file)
  #   =============================================================
  #   ACTUAL NODE FOUND:
  #   =============================================================
  #    * Node name:   tauprime
  #       Path:       G/tauprime
  #       Contents:   This is the file 'tau'.
  #
  #       Properties: {'svn:mergeinfo': ''}
  #       Attributes: {}
  #       Children:   N/A (node is a file)
  #
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='8')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # Make another change on the branch: copy tau to tauprime.  Commit
  # in r9.
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     sbox.repo_url + '/A_COPY/D/G/tau',
                                     sbox.repo_url + '/A_COPY/D/G/tauprime',
                                     '-m',
                                     'Repos to repos copy of tau to tauprime')

  # Update the trunk (well, the whole wc) to get the copy above and since
  # reintegrate really wants a clean wc.
  expected_output = wc.State(wc_dir, {
    'A_COPY/D/G/tauprime' : Item(verb='Adding')
    })
  expected_output = wc.State(A_COPY_path, {
    'D/G/tauprime' : Item(status='A '),
    })
  expected_status.add({'A_COPY/D/G/tauprime': Item(status='  ', wc_rev=9)})
  expected_disk.add({
    'A_COPY/D/G/tauprime' : Item(props={SVN_PROP_MERGEINFO: '/A/D/G/tau:2-7'},
                                 contents="This is the file 'tau'.\n")
    })
  expected_status.tweak(wc_rev='9')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # *finally*, actually run merge --reintegrate in trunk with the
  # branch URL.  This should bring in the mu change and the tauprime
  # change.
  A_path = os.path.join(wc_dir, "A")
  expected_output = wc.State(A_path, {
    'mu'           : Item(status='U '),
    'D/G/tauprime' : Item(status='A '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    ''             : Item(status=' U'),
    'D/G/tauprime' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  k_expected_status = wc.State(A_path, {
    "B"            : Item(status='  ', wc_rev=9),
    "B/lambda"     : Item(status='  ', wc_rev=9),
    "B/E"          : Item(status='  ', wc_rev=9),
    "B/E/alpha"    : Item(status='  ', wc_rev=9),
    "B/E/beta"     : Item(status='  ', wc_rev=9),
    "B/F"          : Item(status='  ', wc_rev=9),
    "mu"           : Item(status='M ', wc_rev=9),
    "C"            : Item(status='  ', wc_rev=9),
    "D"            : Item(status='  ', wc_rev=9),
    "D/gamma"      : Item(status='  ', wc_rev=9),
    "D/G"          : Item(status='  ', wc_rev=9),
    "D/G/pi"       : Item(status='  ', wc_rev=9),
    "D/G/rho"      : Item(status='  ', wc_rev=9),
    "D/G/tau"      : Item(status='  ', wc_rev=9),
    "D/G/tauprime" : Item(status='A ', wc_rev='-', copied='+'),
    "D/H"          : Item(status='  ', wc_rev=9),
    "D/H/chi"      : Item(status='  ', wc_rev=9),
    "D/H/omega"    : Item(status='  ', wc_rev=9),
    "D/H/psi"      : Item(status='  ', wc_rev=9),
    ""             : Item(status=' M', wc_rev=9),
  })
  k_expected_disk.tweak('', props={SVN_PROP_MERGEINFO : '/A_COPY:2-9'})
  k_expected_disk.add({
    'D/G/tauprime' : Item(props={SVN_PROP_MERGEINFO :
                                 '/A/D/G/tau:2-7\n/A_COPY/D/G/tauprime:9'},
                          contents="This is the file 'tau'.\n")
    })
  expected_skip = wc.State(A_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       k_expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True, True,
                                       '--reintegrate', A_path)

  # Finally, commit the result of the merge (r10).
  expected_output = wc.State(wc_dir, {
    'A/D/G/tauprime' : Item(verb='Adding'),
    'A/mu'           : Item(verb='Sending'),
    'A'              : Item(verb='Sending'),
    })
  expected_status.add({
    'A/D/G/tauprime' : Item(status='  ', wc_rev=10),
    })
  expected_status.tweak('A', 'A/mu', wc_rev=10)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

#----------------------------------------------------------------------
def reintegrate_branch_never_merged_to(sbox):
  "merge --reintegrate on a never-updated branch"

  # Make A_COPY branch in r2, and do a few more commits to A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Make a change on the branch, to A_COPY/mu.  Commit in r7.
  svntest.main.file_write(os.path.join(wc_dir, "A_COPY", "mu"),
                          "Changed on the branch.")
  expected_output = wc.State(wc_dir, {'A_COPY/mu' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/mu', wc_rev=7)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/mu', contents='Changed on the branch.')

  # Update the wcs.
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='7')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # Make another change on the branch: copy tau to tauprime.  Commit
  # in r8.
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'A_COPY', 'D', 'G',
                                                  'tau'),
                                     os.path.join(wc_dir, 'A_COPY', 'D', 'G',
                                                  'tauprime'))
  expected_output = wc.State(wc_dir, {
    'A_COPY/D/G/tauprime' : Item(verb='Adding')
    })
  expected_status.add({'A_COPY/D/G/tauprime': Item(status='  ', wc_rev=8)})
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.add({
    'A_COPY/D/G/tauprime' : Item(contents="This is the file 'tau'.\n")
    })

  # Update the trunk (well, the whole wc) (since reintegrate really
  # wants a clean wc).
  expected_output = wc.State(wc_dir, {})
  expected_status.tweak(wc_rev='8')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, True)

  # *finally*, actually run merge --reintegrate in trunk with the
  # branch URL.  This should bring in the mu change and the tauprime
  # change.
  A_path = os.path.join(wc_dir, "A")
  expected_output = wc.State(A_path, {
    'mu'           : Item(status='U '),
    'D/G/tauprime' : Item(status='A '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  k_expected_status = wc.State(A_path, {
    "B"            : Item(status='  ', wc_rev=8),
    "B/lambda"     : Item(status='  ', wc_rev=8),
    "B/E"          : Item(status='  ', wc_rev=8),
    "B/E/alpha"    : Item(status='  ', wc_rev=8),
    "B/E/beta"     : Item(status='  ', wc_rev=8),
    "B/F"          : Item(status='  ', wc_rev=8),
    "mu"           : Item(status='M ', wc_rev=8),
    "C"            : Item(status='  ', wc_rev=8),
    "D"            : Item(status='  ', wc_rev=8),
    "D/gamma"      : Item(status='  ', wc_rev=8),
    "D/G"          : Item(status='  ', wc_rev=8),
    "D/G/pi"       : Item(status='  ', wc_rev=8),
    "D/G/rho"      : Item(status='  ', wc_rev=8),
    "D/G/tau"      : Item(status='  ', wc_rev=8),
    "D/G/tauprime" : Item(status='A ', wc_rev='-', copied='+'),
    "D/H"          : Item(status='  ', wc_rev=8),
    "D/H/chi"      : Item(status='  ', wc_rev=8),
    "D/H/omega"    : Item(status='  ', wc_rev=8),
    "D/H/psi"      : Item(status='  ', wc_rev=8),
    ""             : Item(status=' M', wc_rev=8),
  })
  k_expected_disk = wc.State('', {
    ''             : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-8'}),
    'B'            : Item(),
    'B/lambda'     : Item("This is the file 'lambda'.\n"),
    'B/E'          : Item(),
    'B/E/alpha'    : Item("This is the file 'alpha'.\n"),
    'B/E/beta'     : Item("New content"),
    'B/F'          : Item(),
    'mu'           : Item("Changed on the branch."),
    'C'            : Item(),
    'D'            : Item(),
    'D/gamma'      : Item("This is the file 'gamma'.\n"),
    'D/G'          : Item(),
    'D/G/pi'       : Item("This is the file 'pi'.\n"),
    'D/G/rho'      : Item("New content"),
    'D/G/tau'      : Item("This is the file 'tau'.\n"),
    'D/G/tauprime' : Item("This is the file 'tau'.\n"),
    'D/H'          : Item(),
    'D/H/chi'      : Item("This is the file 'chi'.\n"),
    'D/H/omega'    : Item("New content"),
    'D/H/psi'      : Item("New content"),
  })
  expected_skip = wc.State(A_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       k_expected_disk,
                                       k_expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, True, True,
                                       '--reintegrate', A_path)

  # Finally, commit the result of the merge (r9).
  expected_output = wc.State(wc_dir, {
    'A/D/G/tauprime' : Item(verb='Adding'),
    'A/mu'           : Item(verb='Sending'),
    'A'              : Item(verb='Sending'),
    })
  expected_status.add({
    'A/D/G/tauprime' : Item(status='  ', wc_rev=9),
    })
  expected_status.tweak('A', 'A/mu', wc_rev=9)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

#----------------------------------------------------------------------
def reintegrate_fail_on_modified_wc(sbox):
  "merge --reintegrate should fail in modified wc"
  sbox.build()
  wc_dir = sbox.wc_dir
  A_path = os.path.join(wc_dir, "A")
  mu_path = os.path.join(A_path, "mu")
  ignored_expected_disk, ignored_expected_status = set_up_branch(sbox)
  svntest.main.file_write(mu_path, "Changed on 'trunk' (the merge target).")
  sbox.simple_update() # avoid mixed-revision error
  svntest.actions.run_and_verify_merge(
    A_path, None, None, sbox.repo_url + '/A_COPY', None, None, None, None,
    None, None, None,
    ".*Cannot merge into a working copy that has local modifications.*",
    None, None, None, None, True, False, '--reintegrate', A_path)

#----------------------------------------------------------------------
def reintegrate_fail_on_mixed_rev_wc(sbox):
  "merge --reintegrate should fail in mixed-rev wc"
  sbox.build()
  wc_dir = sbox.wc_dir
  A_path = os.path.join(wc_dir, "A")
  mu_path = os.path.join(A_path, "mu")
  ignored_expected_disk, expected_status = set_up_branch(sbox)
  # Make and commit a change, in order to get a mixed-rev wc.
  svntest.main.file_write(mu_path, "Changed on 'trunk' (the merge target).")
  expected_output = wc.State(wc_dir, {
    'A/mu'           : Item(verb='Sending'),
    })
  expected_status.tweak('A/mu', wc_rev=7)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  # Try merging into that same wc, expecting failure.
  svntest.actions.run_and_verify_merge(
    A_path, None, None, sbox.repo_url + '/A_COPY', None, None, None, None,
    None, None, None,
    ".*Cannot merge into mixed-revision working copy.*",
    None, None, None, None, True, False, '--reintegrate', A_path)

#----------------------------------------------------------------------
def reintegrate_fail_on_switched_wc(sbox):
  "merge --reintegrate should fail in switched wc"
  sbox.build()
  wc_dir = sbox.wc_dir
  A_path = os.path.join(wc_dir, "A")
  G_path = os.path.join(A_path, "D", "G")
  switch_url = sbox.repo_url + "/A/D/H"
  expected_disk, expected_status = set_up_branch(sbox)

  # Switch a subdir of the target.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi'          : Item(status='D '),
    'A/D/G/rho'         : Item(status='D '),
    'A/D/G/tau'         : Item(status='D '),
    'A/D/G/chi'         : Item(status='A '),
    'A/D/G/psi'         : Item(status='A '),
    'A/D/G/omega'       : Item(status='A '),
    })
  expected_disk.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_disk.add({
    'A/D/G/chi'   : Item(contents="This is the file 'chi'.\n"),
    'A/D/G/psi'   : Item(contents="New content"),
    'A/D/G/omega' : Item(contents="New content"),
    })
  expected_status.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status.add({
    'A/D/G'       : Item(status='  ', wc_rev=6, switched='S'),
    'A/D/G/chi'   : Item(status='  ', wc_rev=6),
    'A/D/G/psi'   : Item(status='  ', wc_rev=6),
    'A/D/G/omega' : Item(status='  ', wc_rev=6),
    })
  svntest.actions.run_and_verify_switch(wc_dir,
                                        G_path,
                                        switch_url,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        False, '--ignore-ancestry')
  sbox.simple_update() # avoid mixed-revision error
  svntest.actions.run_and_verify_merge(
    A_path, None, None, sbox.repo_url + '/A_COPY', None, None, None, None,
    None, None, None,
    ".*Cannot merge into a working copy with a switched subtree.*",
    None, None, None, None, True, False, '--reintegrate', A_path)

#----------------------------------------------------------------------
# Test for issue #3603 'allow reintegrate merges into WCs with
# missing subtrees'.
@Issue(3603)
def reintegrate_on_shallow_wc(sbox):
  "merge --reintegrate in shallow wc"

  # Create a standard greek tree, branch A to A_COPY in r2.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox, branch_only = True)

  # Some paths we'll care about
  A_path         = os.path.join(wc_dir, "A")
  A_D_path       = os.path.join(wc_dir, "A", "D")
  mu_COPY_path   = os.path.join(wc_dir, "A_COPY", "mu")
  psi_COPY_path  = os.path.join(wc_dir, "A_COPY", "D", "H", "psi")
  A_COPY_path    = os.path.join(wc_dir, "A_COPY")

  # r3 - Make a change on the A_COPY branch that will be
  # reintegrated back to A.
  svntest.main.file_write(mu_COPY_path, "branch work")
  svntest.main.run_svn(None, 'commit', '-m',
                       'Some work on the A_COPY branch', wc_dir)

  # First try a reintegrate where the target WC has a shallow subtree
  # that is not affected by the reintegrate.  In this case we set the
  # depth of A/D to empty.  Since the only change made on the branch
  # since the branch point is to A_COPY/mu, the reintegrate should
  # simply work and update A/mu with the branch's contents.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'empty', A_D_path)
  expected_output = wc.State(A_path, {
    'mu' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_A_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='M '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '), # Don't expect anything under D,
                                     # its depth is empty!
    })
  expected_A_status.tweak(wc_rev=3)
  expected_A_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-3'}),
    'B'         : Item(),
    'mu'        : Item("branch work"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("This is the file 'beta'.\n"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(), # Don't expect anything under D, its depth is empty!
    })
  expected_A_skip = wc.State(A_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_A_disk,
                                       expected_A_status,
                                       expected_A_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

  # Now revert the reintegrate and make a second change on the
  # branch in r4, but this time change a subtree that corresponds
  # to the missing (shallow) portion of the source.  The reintegrate
  # should still succeed, albeit skipping some paths.
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)
  svntest.main.file_write(psi_COPY_path, "more branch work")
  svntest.main.run_svn(None, 'commit', '-m',
                       'Some more work on the A_COPY branch', wc_dir)
  # Reuse the same expectations as the prior merge, except that
  # non-inheritable mergeinfo is set on the root of the missing subtree...
  expected_mergeinfo_output.add({
      'D' : Item(status=' U')
      })
  expected_A_status.tweak('D', status=' M')
  expected_A_disk.tweak('D', props={SVN_PROP_MERGEINFO : '/A_COPY/D:2-4*'})
  # ... a depth-restricted item is skipped ...
  expected_A_skip.add({
      'D/H' : Item()
  })
  # ... and the mergeinfo on the target root includes the latest rev on the branch.
  expected_A_disk.tweak('', props={SVN_PROP_MERGEINFO : '/A_COPY:2-4'})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_A_disk,
                                       expected_A_status,
                                       expected_A_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
def reintegrate_fail_on_stale_source(sbox):
  "merge --reintegrate should fail on stale source"
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)
  A_path = os.path.join(wc_dir, "A")
  mu_path = os.path.join(A_path, "mu")
  svntest.main.file_append(mu_path, 'some text appended to mu\n')
  svntest.actions.run_and_verify_svn(None, None, [], 'commit',
                                     '-m', 'a change to mu', mu_path);
  # Unmix the revisions in the working copy.
  svntest.actions.run_and_verify_svn(None, None, [], 'update', wc_dir);
  # The merge --reintegrate succeeds but since there were no changes
  # on A_COPY after it was branched the only result is updated mergeinfo
  # on the reintegrate target.
  expected_output = wc.State(A_path, {})
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='  '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=7)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-7'}),
    'B'         : Item(),
    'mu'        : Item("This is the file 'mu'.\nsome text appended to mu\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A_path, { })
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       [], None, None, None, None, True, True,
                                       '--reintegrate', A_path)

#----------------------------------------------------------------------
def merge_file_with_space_in_its_path(sbox):
  "merge a file with space in its path"

  sbox.build()
  wc_dir = sbox.wc_dir
  some_dir = os.path.join(wc_dir, "some dir")
  file1 = os.path.join(some_dir, "file1")
  file2 = os.path.join(some_dir, "file2")

  # Make r2.
  os.mkdir(some_dir)
  svntest.main.file_append(file1, "Initial text in the file.\n")
  svntest.main.run_svn(None, "add", some_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "ci", "-m", "r2", wc_dir)

  # Make r3.
  svntest.main.run_svn(None, "copy", file1, file2)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "ci", "-m", "r3", wc_dir)

  # Make r4.
  svntest.main.file_append(file2, "Next line of text in the file.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "ci", "-m", "r4", wc_dir)

  target_url = sbox.repo_url + '/some%20dir/file2'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "merge", "--reintegrate", target_url,
                                     file1)

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
def reintegrate_with_subtree_mergeinfo(sbox):
  "merge --reintegrate with subtree mergeinfo"

  # Create a standard greek tree, branch A to A_COPY in r2, A to A_COPY_2 in
  # r3, A to A_COPY_3 in r3, and then make some changes under A in r5-8.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox, False, 3)

  # Some paths we'll care about
  gamma_COPY_3_path     = os.path.join(wc_dir, "A_COPY_3", "D", "gamma")
  D_path                = os.path.join(wc_dir, "A", "D")
  gamma_path            = os.path.join(wc_dir, "A", "D", "gamma")
  mu_COPY_2_path        = os.path.join(wc_dir, "A_COPY_2", "mu")
  mu_path               = os.path.join(wc_dir, "A", "mu")
  mu_COPY_path          = os.path.join(wc_dir, "A_COPY", "mu")
  A_COPY_path           = os.path.join(wc_dir, "A_COPY")
  D_COPY_path           = os.path.join(wc_dir, "A_COPY")
  beta_COPY_path        = os.path.join(wc_dir, "A_COPY", "B", "E", "beta")
  gamma_COPY_path       = os.path.join(wc_dir, "A_COPY", "D", "gamma")
  gamma_moved_COPY_path = os.path.join(wc_dir, "A_COPY", "D", "gamma_moved")
  gamma_moved_path      = os.path.join(wc_dir, "A", "D", "gamma_moved")
  rho_COPY_path         = os.path.join(wc_dir, "A_COPY", "D", "G", "rho")
  omega_COPY_path       = os.path.join(wc_dir, "A_COPY", "D", "H", "omega")
  psi_COPY_path         = os.path.join(wc_dir, "A_COPY", "D", "H", "psi")
  D_COPY_path           = os.path.join(wc_dir, "A_COPY", "D")
  alpha_COPY_path       = os.path.join(wc_dir, "A_COPY", "B", "E", "alpha")
  A_path                = os.path.join(wc_dir, "A")

  # Now set up a situation where we try to reintegrate A_COPY back to A but
  # both of these paths have subtree mergeinfo.  Iff the mergeinfo on A_COPY
  # reflects that the same revisions have been applied across all of A_COPY,
  # then the reintegrate merge should succeed.
  #
  # r9 - Make a text change to A_COPY_3/D/gamma
  svntest.main.file_write(gamma_COPY_3_path, "New content")
  expected_output = wc.State(wc_dir, {'A_COPY_3/D/gamma' : Item(verb='Sending')})
  expected_status.tweak('A_COPY_3/D/gamma', wc_rev=9)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY_3/D/gamma', contents="New content")

  # r10 - Merge r9 from A_COPY_3/D to A/D, creating explicit subtree
  # mergeinfo under A.  For this and every subsequent merge we update the WC
  # first to allow full inheritance and elision.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(9), [], 'up',
                                     wc_dir)
  expected_status.tweak(wc_rev=9)
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[9]],
                          ['U    ' + gamma_path + '\n',
                           ' U   ' + D_path     + '\n',]),
    [], 'merge', '-c9', sbox.repo_url + '/A_COPY_3/D', D_path)
  expected_output = wc.State(wc_dir,
                             {'A/D'       : Item(verb='Sending'),
                              'A/D/gamma' : Item(verb='Sending')})
  expected_status.tweak('A/D', 'A/D/gamma', wc_rev=10)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A/D/gamma', contents="New content")
  expected_disk.tweak('A/D', props={SVN_PROP_MERGEINFO : '/A_COPY_3/D:9'})

  # r11 - Make a text change to A_COPY_2/mu
  svntest.main.file_write(mu_COPY_2_path, "New content")
  expected_output = wc.State(wc_dir, {'A_COPY_2/mu' : Item(verb='Sending')})
  expected_status.tweak('A_COPY_2/mu', wc_rev=11)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY_2/mu', contents="New content")

  # r12 - Merge r11 from A_COPY_2/mu to A_COPY/mu
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(11), [], 'up',
                                     wc_dir)
  expected_status.tweak(wc_rev=11)
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[11]],
                          ['U    ' + mu_COPY_path + '\n',
                           ' U   ' + mu_COPY_path + '\n',]),
    [], 'merge', '-c11', sbox.repo_url + '/A_COPY_2/mu', mu_COPY_path)
  expected_output = wc.State(wc_dir,
                             {'A_COPY/mu' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/mu', wc_rev=12)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/mu', contents="New content")

  # r13 - Do a 'synch' cherry harvest merge of all available revisions
  # from A to A_COPY
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(12), [], 'up',
                                     wc_dir)
  expected_status.tweak(wc_rev=12)
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[2,12]],
                          ['U    ' + beta_COPY_path  + '\n',
                           'U    ' + gamma_COPY_path + '\n',
                           'U    ' + rho_COPY_path   + '\n',
                           'U    ' + omega_COPY_path + '\n',
                           'U    ' + psi_COPY_path   + '\n',
                           ' U   ' + A_COPY_path     + '\n',
                           ' U   ' + D_COPY_path     + '\n',
                           ' G   ' + D_COPY_path     + '\n',]),
    [], 'merge', sbox.repo_url + '/A', A_COPY_path)
  expected_output = wc.State(wc_dir,
                             {'A_COPY'           : Item(verb='Sending'),
                              #'A_COPY/mu'        : Item(verb='Sending'),
                              'A_COPY/B/E/beta'  : Item(verb='Sending'),
                              'A_COPY/D'         : Item(verb='Sending'),
                              'A_COPY/D/G/rho'   : Item(verb='Sending'),
                              'A_COPY/D/H/omega' : Item(verb='Sending'),
                              'A_COPY/D/H/psi'   : Item(verb='Sending'),
                              'A_COPY/D/gamma'   : Item(verb='Sending')})
  expected_status.tweak('A_COPY',
                        #'A_COPY/mu',
                        'A_COPY/B/E/beta',
                        'A_COPY/D',
                        'A_COPY/D/G/rho',
                        'A_COPY/D/H/omega',
                        'A_COPY/D/H/psi',
                        'A_COPY/D/gamma',
                        wc_rev=13)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/B/E/beta',
                      'A_COPY/D',
                      'A_COPY/D/G/rho',
                      'A_COPY/D/H/omega',
                      'A_COPY/D/H/psi',
                      'A_COPY/D/gamma',
                      contents="New content")
  expected_disk.tweak('A_COPY',   props={SVN_PROP_MERGEINFO : '/A:2-12'})
  expected_disk.tweak('A_COPY/D',
                      props={SVN_PROP_MERGEINFO : '/A/D:2-12\n/A_COPY_3/D:9\n'})

  # r14 - Make a text change on A_COPY/B/E/alpha
  svntest.main.file_write(alpha_COPY_path, "New content")
  expected_output = wc.State(wc_dir, {'A_COPY/B/E/alpha' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/B/E/alpha', wc_rev=14)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.tweak('A_COPY/B/E/alpha', contents="New content")

  # Now, reintegrate A_COPY to A.  This should succeed.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(14), [], 'up',
                                     wc_dir)
  expected_status.tweak(wc_rev=14)
  expected_output = wc.State(A_path, {
    'B/E/alpha' : Item(status='U '),
    'mu'        : Item(status='UU'),
    'D'         : Item(status=' U'),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    ''   : Item(status=' U'),
    'mu' : Item(status=' G'),
    'D'  : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_A_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='MM'),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='M '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status=' M'),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_A_status.tweak(wc_rev=14)
  expected_A_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-14'}),
    'B'         : Item(),
    'mu'        : Item("New content",
                       props={SVN_PROP_MERGEINFO :
                              '/A_COPY/mu:2-14\n/A_COPY_2/mu:11'}),
    'B/E'       : Item(),
    'B/E/alpha' : Item("New content"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(props=
                       {SVN_PROP_MERGEINFO : '/A_COPY/D:2-14\n/A_COPY_3/D:9'}),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("New content"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_A_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_A_disk,
                                       expected_A_status,
                                       expected_A_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

  # Make some more changes to A_COPY so that the same revisions have *not*
  # been uniformly applied from A to A_COPY.  In this case the reintegrate
  # merge should fail, but should provide a helpful message as to where the
  # problems are.
  #
  # First revert the previous reintegrate merge
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', '-R', wc_dir)

  # r15 - Reverse Merge r8 from A/D to A_COPY/D.
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[-8]],
                          ['U    ' + omega_COPY_path + '\n',
                           ' U   ' + D_COPY_path     + '\n',]),
    [], 'merge', '-c-8', sbox.repo_url + '/A/D', D_COPY_path)
  expected_output = wc.State(wc_dir,
                             {'A_COPY/D'         : Item(verb='Sending'),
                              'A_COPY/D/H/omega' : Item(verb='Sending')})
  expected_status.tweak('A_COPY/D', 'A_COPY/D/H/omega', wc_rev=15)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Now reintegrate A_COPY back to A.  Since A_COPY/D no longer has r8 merged
  # to it from A, the merge should fail.  Further we expect an error message
  # that highlights the fact that A_COPY/D is the offending subtree.
  #
  # The actions.run_and_verify_* methods are happy if one line of the error
  # matches the regex, but we want to know that the error actually provides
  # specific information about the paths that are stopping --reintegrate from
  # working.  So we will pass the stderr to svntest.verify.verify_outputs()
  # ourselves, but as the 'actual_stdout' argument, that way each line of
  # error must match the regex.
  exit_code, out, err = svntest.actions.run_and_verify_svn(
    None, [], svntest.verify.AnyOutput,
    'merge', '--reintegrate', sbox.repo_url + '/A_COPY', A_path)

  svntest.verify.verify_outputs("Reintegrate failed but not "
                                "in the way expected",
                                err, None,
                                "(svn: E195016: Reintegrate can only be used if "
                                "revisions 2 through 15 were previously "
                                "merged from .*/A to the reintegrate source, "
                                "but this is not the case:\n)"
                                "|(  A_COPY/D\n)"
                                "|(    Missing ranges: /A/D:8\n)"
                                "|(  A_COPY/mu\n)"
                                "|(    Missing ranges: /A/mu:2-12\n)"
                                "|(\n)"
                                "|(.*apr_err.*)", # In case of debug build
                                None,
                                True) # Match *all* lines of stdout

  # Test another common situation that can break reintegrate as a result
  # of copies and moves:
  #
  #   A) On our 'trunk' rename a subtree in such a way as the new
  #      subtree has explicit mergeinfo.  Commit this rename as rev N.
  #
  #   B) Synch merge the rename in A) to our 'branch' in rev N+1.  The
  #      renamed subtree now has the same explicit mergeinfo on both
  #      the branch and trunk.
  #
  #   C) Make some more changes on the renamed subtree in 'trunk' and
  #      commit in rev N+2.
  #
  #   D) Synch merge the changes in C) from 'trunk' to 'branch' and commit in
  #      rev N+3.  The renamed subtree on 'branch' now has additional explicit
  #      mergeinfo decribing the synch merge from trunk@N+1 to trunk@N+2.
  #
  #   E) Reintegrate 'branch' to 'trunk'.  This fails as it appears not all
  #      of 'trunk' was previously merged to 'branch'

  # r16 - A) REPOS-to-REPOS rename of A/D/gamma to A/D/gamma_moved.  Since
  # r874258 WC-to-WC moves won't create mergeinfo on the dest if the source
  # doesn't have any.  So do a repos-to-repos move so explicit mergeinfo
  # *is* created on the destination.
  svntest.actions.run_and_verify_svn(None, None,[], 'move',
                                     sbox.repo_url + '/A/D/gamma',
                                     sbox.repo_url + '/A/D/gamma_moved',
                                     '-m', 'REPOS-to-REPOS move'
                                     )
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_disk.remove('A/D/gamma')
  expected_disk.add({
    'A/D/gamma_moved' : Item(props={SVN_PROP_MERGEINFO: '/A_COPY_3/D/gamma:9'},
                             contents="New content")
    })
  expected_status.tweak(wc_rev=16)
  expected_status.remove('A/D/gamma')
  expected_status.add({'A/D/gamma_moved' : Item(status='  ', wc_rev=16)})

  # r17 - B) Synch merge from A to A_COPY
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[8], [13,16], [2,16]],
                          ['U    ' + omega_COPY_path + '\n',
                           'A    ' + gamma_moved_COPY_path + '\n',
                           'D    ' + gamma_COPY_path + '\n',
                           ' U   ' + A_COPY_path     + '\n',
                           ' U   ' + D_COPY_path     + '\n',
                           ' U   ' + gamma_moved_COPY_path + '\n']),
    [], 'merge', sbox.repo_url + '/A',  A_COPY_path)
  expected_output = wc.State(
    wc_dir,
    {'A_COPY'               : Item(verb='Sending'), # Mergeinfo update
     'A_COPY/D'             : Item(verb='Sending'), # Mergeinfo update
     'A_COPY/D/gamma'       : Item(verb='Deleting'),
     'A_COPY/D/gamma_moved' : Item(verb='Adding'),
     'A_COPY/D/H/omega'     : Item(verb='Sending'), # Redoing r15's
                                                    # reverse merge of r8.
     })
  expected_status.remove('A_COPY/D/gamma')

  expected_status.tweak('A_COPY',
                        'A_COPY/D',
                        'A_COPY/D/H/omega',
                        wc_rev=17)
  expected_status.add({'A_COPY/D/gamma_moved' : Item(status='  ', wc_rev=17)})
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  expected_disk.remove('A_COPY/D/gamma')
  expected_disk.add({
    'A/D/gamma_moved' : Item(props={SVN_PROP_MERGEINFO: '/A_COPY_3/D/gamma:9'},
                             contents="New content")
    })

  # r18 - C) Text mod to A/D/gamma_moved
  svntest.main.file_write(gamma_moved_path, "Even newer content")
  expected_output = wc.State(wc_dir, {'A/D/gamma_moved' : Item(verb='Sending')})
  expected_status.tweak('A/D/gamma_moved', wc_rev=18)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # r19 - D) Synch merge from A to A_COPY
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[17,18], [2,18]],
                          ['U    ' + gamma_moved_COPY_path + '\n',
                           ' U   ' + A_COPY_path + '\n',
                           ' U   ' + D_COPY_path + '\n',
                           ' U   ' + gamma_moved_COPY_path + '\n']),
    [], 'merge', '--allow-mixed-revisions', sbox.repo_url + '/A',  A_COPY_path)
  expected_output = wc.State(
    wc_dir,
    {'A_COPY'               : Item(verb='Sending'), # Mergeinfo update
     'A_COPY/D'             : Item(verb='Sending'), # Mergeinfo update
     'A_COPY/D/gamma_moved' : Item(verb='Sending'), # Text change
     })
  expected_status.tweak('A_COPY',
                        'A_COPY/D',
                        'A_COPY/D/gamma_moved',
                        wc_rev=19)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Reintegrate A_COPY to A, this should work since
  # A_COPY/D/gamma_moved's natural history,
  #
  #   /A/D/gamma:1-15
  #   /A/D/gamma_moved:16
  #   /A_COPY/D/gamma_moved:17-19
  #
  # shows that it is fully synched up with trunk.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(19), [], 'up',
                                     wc_dir)
  expected_output = wc.State(A_path, {
    'B/E/alpha'     : Item(status='U '),
    'mu'            : Item(status='UU'),
    'D'             : Item(status=' U'),
    'D/gamma_moved' : Item(status=' U'),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    ''              : Item(status=' U'),
    'mu'            : Item(status=' G'),
    'D'             : Item(status=' U'),
    'D/gamma_moved' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_A_status = wc.State(A_path, {
    ''              : Item(status=' M'),
    'B'             : Item(status='  '),
    'mu'            : Item(status='MM'),
    'B/E'           : Item(status='  '),
    'B/E/alpha'     : Item(status='M '),
    'B/E/beta'      : Item(status='  '),
    'B/lambda'      : Item(status='  '),
    'B/F'           : Item(status='  '),
    'C'             : Item(status='  '),
    'D'             : Item(status=' M'),
    'D/G'           : Item(status='  '),
    'D/G/pi'        : Item(status='  '),
    'D/G/rho'       : Item(status='  '),
    'D/G/tau'       : Item(status='  '),
    'D/gamma_moved' : Item(status=' M'),
    'D/H'           : Item(status='  '),
    'D/H/chi'       : Item(status='  '),
    'D/H/psi'       : Item(status='  '),
    'D/H/omega'     : Item(status='  '),
    })
  expected_A_status.tweak(wc_rev=19)
  expected_A_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-19'}),
    'B'         : Item(),
    'mu'        : Item("New content",
                       props={SVN_PROP_MERGEINFO :
                              '/A_COPY/mu:2-19\n/A_COPY_2/mu:11'}),
    'B/E'           : Item(),
    'B/E/alpha'     : Item("New content"),
    'B/E/beta'      : Item("New content"),
    'B/lambda'      : Item("This is the file 'lambda'.\n"),
    'B/F'           : Item(),
    'C'             : Item(),
    'D'             : Item(props={SVN_PROP_MERGEINFO :
                                  '/A_COPY/D:2-19\n/A_COPY_3/D:9'}),
    'D/G'           : Item(),
    'D/G/pi'        : Item("This is the file 'pi'.\n"),
    'D/G/rho'       : Item("New content"),
    'D/G/tau'       : Item("This is the file 'tau'.\n"),
    # Why do we expect mergeinfo of '/A_COPY/D/G/tauprime:2-9' on
    # A/D/G/tauprime?  Because this --reintegrate merge is effectively a
    # two URL merge of %URL%/A@9 %URL%/A_COPY@9 to 'A'.  Since %URL%/A@9 and
    # %URL%/A_COPY@9 have a common ancestor in %URL%/A@1 we expect this 2-URL
    # merge to record mergeinfo and a component of that mergeinfo describes
    # the merge of %URL%/A_COPY@2 to %URL%/A_COPY@9.  We see that above on
    # A.  But we also get it on A's subtrees with explicit mergeinfo, namely
    # A/D/G/tauprime.  Now I know what you are thinking, "'A_COPY/D/G/tauprime'
    # doesn't even exist until r9!", and you are quite right.  But this
    # inheritance of bogus mergeinfo is a known problem, see
    # http://subversion.tigris.org/issues/show_bug.cgi?id=3157#desc8,
    # and is not what this test is about, so we won't fail because of it.
    'D/gamma_moved' : Item(
      "Even newer content", props={SVN_PROP_MERGEINFO :
                                   '/A_COPY/D/gamma_moved:17-19\n'
                                   '/A_COPY_3/D/gamma:9'}),
    'D/H'           : Item(),
    'D/H/chi'       : Item("This is the file 'chi'.\n"),
    'D/H/psi'       : Item("New content"),
    'D/H/omega'     : Item("New content"),
    })
  expected_A_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_A_disk,
                                       expected_A_status,
                                       expected_A_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
def multiple_reintegrates_from_the_same_branch(sbox):
  "multiple reintegrates create self-referential"

  # Make A_COPY branch in r2, and do a few more commits to A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Some paths we'll care about
  A_path              = os.path.join(wc_dir, "A")
  mu_path             = os.path.join(wc_dir, "A", "mu")
  A_COPY_path         = os.path.join(wc_dir, "A_COPY")
  psi_COPY_path       = os.path.join(wc_dir, "A_COPY", "D", "H", "psi")
  Feature_branch_path = os.path.join(wc_dir, "A_FEATURE_BRANCH")
  Feature_beta_path   = os.path.join(wc_dir, "A_FEATURE_BRANCH", "B", "E",
                                     "beta")

  # Create a feature branch and do multiple reintegrates from the branch
  # without deleting and recreating it.  We don't recommend doing this,
  # but regardless, it shouldn't create self-referential mergeinfo on
  # the reintegrate target.
  #
  # r7 - Create the feature branch.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', A_path, Feature_branch_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'Make a feature branch',
                                     wc_dir)

  # r8 - Make a change under 'A'.
  svntest.main.file_write(mu_path, "New trunk content.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "A text change under 'A'",
                                     wc_dir)

  # r9 - Make a change on the feature branch.
  svntest.main.file_write(Feature_beta_path, "New branch content.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "A text change on the feature branch",
                                     wc_dir)

  # r10 - Sync merge all changes from 'A' to the feature branch.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge',
                                     sbox.repo_url + '/A',
                                     Feature_branch_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "Sync merge 'A' to feature branch",
                                     wc_dir)

  # r11 - Reintegrate the feature branch back to 'A'.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '--reintegrate',
                                     sbox.repo_url + '/A_FEATURE_BRANCH',
                                     A_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "Reintegrate feature branch back to 'A'",
                                     wc_dir)

  # r12 - Do a --record-only merge from 'A' to the feature branch so we
  # don't try to merge r11 from trunk during the next sync merge.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge',
                                     '--record-only',
                                     sbox.repo_url + '/A',
                                     Feature_branch_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "Sync merge 'A' to feature branch",
                                     wc_dir)

  # r13 - Make another change on the feature branch.
  svntest.main.file_write(Feature_beta_path, "Even newer branch content.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "Different text on the feature branch",
                                     wc_dir)

  # r14 - Sync merge all changes from 'A' to the feature branch in
  # preparation for a second reintegrate from this branch.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge',
                                     sbox.repo_url + '/A',
                                     Feature_branch_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "2nd Sync merge 'A' to feature branch",
                                     wc_dir)

  # r15 - Reintegrate the feature branch back to 'A' a second time.
  # No self-referential mergeinfo should be applied on 'A'.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_output = wc.State(A_path, {
     #'' : Item(status=' U'), #<-- no self-referential mergeinfo applied!
    'B/E/beta' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='  '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='M '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=14)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO :
                              # Prior to r????? we'd get this
                              # self-referential mergeinfo:
                              #'/A:2-6\n/A_FEATURE_BRANCH:7-14'}),
                              '/A_FEATURE_BRANCH:7-14'}),
    'B'         : Item(),
    'mu'        : Item("New trunk content.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("Even newer branch content.\n"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A_path, { })
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_FEATURE_BRANCH',
                                       None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1, 1, '--reintegrate', A_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "2nd Reintegrate feature branch back to 'A'",
                                     wc_dir)

  # Demonstrate the danger of any self-referential mergeinfo on trunk.
  #
  # Merge all available revisions except r3 from 'A' to 'A_COPY'.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-r3:HEAD',
                                     sbox.repo_url + '/A',
                                     A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     "Merge -r3:HEAD from 'A' to 'A_COPY'",
                                     wc_dir)
  # No self-referential mergeinfo should have been carried on 'A_COPY' from
  # 'A' that would prevent the following merge from being operative.
  svntest.actions.run_and_verify_svn(
    None,
    expected_merge_output([[2,3],[2,16]],
                          ['U    ' + psi_COPY_path + '\n',
                           ' U   ' + A_COPY_path   + '\n',]),
    [], 'merge', '--allow-mixed-revisions', sbox.repo_url + '/A', A_COPY_path)

#----------------------------------------------------------------------
# Test for a reintegrate bug which can occur when the merge source
# has mergeinfo that explicitly describes common history with the reintegrate
# target, see http://svn.haxx.se/dev/archive-2009-12/0338.shtml
#
# Also tests Issue #3591 'reintegrate merges update subtree mergeinfo
# unconditionally'.
@Issue(3591)
def reintegrate_with_self_referential_mergeinfo(sbox):
  "source has target's history as explicit mergeinfo"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make some changes under 'A' in r2-5.
  wc_disk, wc_status = set_up_branch(sbox, nbr_of_branches=0)

  # Some paths we'll care about
  A_path       = os.path.join(wc_dir, "A")
  A2_path      = os.path.join(wc_dir, "A2")
  A2_B_path    = os.path.join(wc_dir, "A2", "B")
  A2_1_path    = os.path.join(wc_dir, "A2.1")
  A2_1_mu_path = os.path.join(wc_dir, "A2.1", "mu")

  # r6 Copy A to A2 and then manually set some self-referential mergeinfo on
  # A2/B and A2.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(5), [],
                                     'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', A_path, A2_path)
  # /A:3 describes A2's natural history, a.k.a. it's implicit mergeinfo, so
  # it is self-referential.  Same for /A/B:4 and A2/B.  Normally this is
  # redundant but not harmful.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'svn:mergeinfo', '/A:3', A2_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'svn:mergeinfo', '/A/B:4', A2_B_path)
  svntest.actions.run_and_verify_svn(
    None, None, [], 'ci', '-m',
    'copy A to A2 and set some self-referential mergeinfo on the latter.',
    wc_dir)

  # r7 Copy A2 to A2.1
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', A2_path, A2_1_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'copy A2to A2.1.', wc_dir)

  # r8 Make a change on A2.1/mu
  svntest.main.file_write(A2_1_mu_path, 'New A2.1 stuff')
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Work done on the A2.1 branch.',
                                     wc_dir)

  # Update to uniform revision and reintegrate A2.1 back to A2.
  # Note that the mergeinfo on A2/B is not changed by the reintegration
  # and so is not expected to by updated to describe the merge.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(8), [],
                                     'up', wc_dir)
  expected_output = wc.State(A2_path, {
    'mu' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A2_path, {
    ''  : Item(status=' U'),
    })
  expected_elision_output = wc.State(A2_path, {
    })
  expected_status = wc.State(A2_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='M '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=8)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A:3\n/A2.1:7-8'}),
    'B'         : Item(props={SVN_PROP_MERGEINFO : '/A/B:4'}),
    'mu'        : Item("New A2.1 stuff"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A2_path, { })
  # Previously failed with this error:
  #
  #   svn merge ^/A2.1" A2 --reintegrate
  #  ..\..\..\subversion\svn\merge-cmd.c:349: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_client\merge.c:9219: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_client\ra.c:728: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_client\mergeinfo.c:733: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_client\ra.c:526: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_repos\rev_hunt.c:908: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_repos\rev_hunt.c:607: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_fs_fs\tree.c:2886: (apr_err=160013)
  #  ..\..\..\subversion\libsvn_fs_fs\tree.c:669: (apr_err=160013)
  #  svn: File not found: revision 4, path '/A2'
  svntest.actions.run_and_verify_merge(A2_path, None, None,
                                       sbox.repo_url + '/A2.1', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1, 0, '--reintegrate', A2_path)

#----------------------------------------------------------------------
# Test for issue #3577 '1.7 subtree mergeinfo recording breaks reintegrate'.
@Issue(3577)
def reintegrate_with_subtree_merges(sbox):
  "reintegrate with prior subtree merges to source"

  # Create a standard greek tree, branch A to A_COPY in r2, and make
  # some changes under A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Some paths we'll care about
  A_path        = os.path.join(wc_dir, "A")
  mu_COPY_path  = os.path.join(wc_dir, "A_COPY", "mu")
  A_COPY_path   = os.path.join(wc_dir, "A_COPY")
  B_COPY_path   = os.path.join(wc_dir, "A_COPY", "B")
  rho_COPY_path = os.path.join(wc_dir, "A_COPY", "D", "G", "rho")
  H_COPY_path   = os.path.join(wc_dir, "A_COPY", "D", "H")

  # r7 - Make a change on the A_COPY branch that will be
  # reintegrated back to A.
  svntest.main.file_write(mu_COPY_path, "branch work")
  svntest.main.run_svn(None, 'commit', '-m',
                       'Some work on the A_COPY branch', wc_dir)

  # Update the WC to a uniform revision, then merge all of the changes
  # from A to A_COPY, but do it via subtree merges so the mergeinfo
  # record of the merges insn't neatly reflected in the root of the
  # branch.  Commit the merge as r8.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-c5',
                                     sbox.repo_url + '/A/B',
                                     B_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-c4',
                                     sbox.repo_url + '/A/D/G/rho',
                                     rho_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-c3',
                                     sbox.repo_url + '/A/D/H',
                                     H_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-c6',
                                     sbox.repo_url + '/A',
                                     A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'commit', '-m',
                                     'Merge everything from A to A_COPY',
                                     wc_dir)

  # Now update the WC and try to reintegrate.  Since we really have merged
  # everything from A to A_COPY, even though it was done via subtree merges,
  # the reintegrate should succeed.  Previously it failed because the naive
  # interpretation of the mergeinfo on A_COPY didn't reflect that it was
  # fully synced with A, resulting in this error:
  #
  #    svn merge ^/A_COPY A --reintegrate
  #    ..\..\..\subversion\svn\merge-cmd.c:358: (apr_err=195016)
  #    ..\..\..\subversion\libsvn_client\merge.c:9318: (apr_err=195016)
  #    svn: Reintegrate can only be used if revisions 2 through 7 were
  #    previously merged from file:///C%3A/SVN/src-trunk-2/Debug/subversion
  #    /tests/cmdline/svn-test-work/repositories/merge_tests-142/A to the
  #     reintegrate source, but this is not the case:
  #      A_COPY
  #        Missing ranges: /A:2-5
  #      A_COPY/B
  #        Missing ranges: /A/B:2-4,6
  #      A_COPY/D/G/rho
  #        Missing ranges: /A/D/G/rho:2-3,5-6
  #      A_COPY/D/H
  #        Missing ranges: /A/D/H:2,4-5
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_output = wc.State(A_path, {
    'mu' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' G'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_A_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='M '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_A_status.tweak(wc_rev=8)
  expected_A_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-8'}),
    'B'         : Item(),
    'mu'        : Item("branch work"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_A_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_A_disk,
                                       expected_A_status,
                                       expected_A_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

#----------------------------------------------------------------------
# Test for issue #3654 'added subtrees with mergeinfo break reintegrate'.
@Issue(3654)
def added_subtrees_with_mergeinfo_break_reintegrate(sbox):
  "added subtrees with mergeinfo break reintegrate"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  A_path           = os.path.join(wc_dir, "A")
  nu_path          = os.path.join(wc_dir, "A", "C", "nu")
  mu_path          = os.path.join(wc_dir, "A", "mu")
  A_COPY_path      = os.path.join(wc_dir, "A_COPY")
  lambda_COPY_path = os.path.join(wc_dir, "A_COPY", "B", "lambda")
  A_COPY_2_path    = os.path.join(wc_dir, "A_COPY_2")
  nu_COPY_2_path   = os.path.join(wc_dir, "A_COPY_2", "C", "nu")

  # Branch A@1 to A_COPY and A_COPY_2 in r2 and r3 respectively.
  # Make some changes under 'A' in r4-7.
  wc_disk, wc_status = set_up_branch(sbox, nbr_of_branches=2)

  # r8 - Add a new file A_COPY_2/C/nu.
  svntest.main.file_write(nu_COPY_2_path, "This is the file 'nu'.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', nu_COPY_2_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Add new file in A_COPY_2 branch',
                                     wc_dir)


  # r9 - Cyclic cherry pick merge r8 from A_COPY_2 back to A.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', '-c', '8',
                                     sbox.repo_url + '/A_COPY_2',
                                     A_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Merge r8 from A_COPY_2 to A.',
                                     wc_dir)

  # r10 - Make an edit to A_COPY_2/C/nu.
  svntest.main.file_write(nu_COPY_2_path, "A_COPY_2 edit to file 'nu'.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Edit new file on A_COPY_2 branch',
                                     wc_dir)

  # r11 - Cyclic subtree cherry pick merge r10 from A_COPY_2/C/nu
  # back to A/C/nu.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', '-c', '10',
                                     sbox.repo_url + '/A_COPY_2/C/nu',
                                     nu_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Merge r8 from A_COPY_2/C/nu to A/C/nu.',
                                     wc_dir)

  # r12 - Edit under A_COPY.
  svntest.main.file_write(mu_path, "mu edits on A_COPY.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Work on A_COPY branch.',
                                     wc_dir)

  # r13 - Sync merge A to A_COPY in preparation for reintegrate.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Prep for reintegrate: Sync A to A_COPY.',
                                     wc_dir)

  # r14 - Reintegrate A_COPY to A.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', '--reintegrate',
                                     sbox.repo_url + '/A_COPY', A_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Reintegrate A_COPY to A.',
                                     wc_dir)

  # r15 - Delete A_COPY.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'delete', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Delete A_COPY branch', wc_dir)

  # r16 - Create new A_COPY from A@HEAD=15.
  #
  # Update so we copy HEAD:
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'copy', A_path, A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Create new A_COPY branch from A', wc_dir)

  # r17 - Unrelated edits under both A and A_COPY.
  svntest.main.file_write(nu_path, "Trunk work on nu.\n")
  svntest.main.file_write(lambda_COPY_path, "lambda edit on A_COPY.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Unrelated edits on A and A_COPY branch.',
                                     wc_dir)

  # r18 - Sync A to A_COPY in preparation for another reintegrate.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Prep for reintegrate: Sync A to A_COPY.',
                                     wc_dir)

  # Reintegrate A_COPY back to A.  We just synced A_COPY with A, so this
  # should work.  The only text change should be the change made to
  # A_COPY/B/lambda in r17 after the new A_COPY was created.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_output = wc.State(A_path, {
    ''         : Item(status=' U'),
    'B/lambda' : Item(status='U '),
    'C/nu'     : Item(status=' U'),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    ''     : Item(status=' U'),
    'C/nu' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='  '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='M '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'C/nu'      : Item(status=' M'),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=18)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO :
                              '/A_COPY:2-13,16-18\n'
                              #         ^     ^
                              #         |     |
                              #   from _|     |
                              #    1st        |
                              # reintegrate   |
                              #               |
                              #        from this reintegrate
                              #
                              '/A_COPY_2:8'}), # <-- From cyclic merge in r9
    'B'         : Item(),
    'mu'        : Item("mu edits on A_COPY.\n"), # From earlier reintegrate.
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("lambda edit on A_COPY.\n"), # From this reintegrate.
    'B/F'       : Item(),
    'C'         : Item(),
    'C/nu'      : Item("Trunk work on nu.\n",
                       props={SVN_PROP_MERGEINFO :
                              '/A_COPY/C/nu:13,16-18\n'
                              '/A_COPY_2/C/nu:10'}), # <-- From cyclic
                                                     # merge in r11
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1, 1, "--reintegrate", A_path)

#----------------------------------------------------------------------
# Test for issue #3648 '2-URL merges incorrectly reverse-merge mergeinfo
# for merge target'.
@Issue(3648)
def two_URL_merge_removes_valid_mergeinfo_from_target(sbox):
  "2-URL merge removes valid mergeinfo from target"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  lambda_COPY_path = os.path.join(wc_dir, "A_COPY", "B", "lambda")
  mu_path          = os.path.join(wc_dir, "A", "mu")
  A_COPY_path      = os.path.join(wc_dir, "A_COPY")
  A_COPY_2_path    = os.path.join(wc_dir, "A_COPY_2")

  # Branch A@1 to A_COPY r2
  # Branch A@1 to A_COPY_2 in r3.
  # Make some changes under 'A' in r4-7.
  wc_disk, wc_status = set_up_branch(sbox, nbr_of_branches=2)

  # r8 - A simple text edit on the A_COPY branch.
  svntest.main.file_write(lambda_COPY_path, "Edit on 'branch 1'.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', "Work on 'branch 1'.",
                                     wc_dir)

  # r9 - Sync the A_COPY branch with A up the HEAD (r8).  Now A_COPY
  # differs from A only by the change made in r8 and by the mergeinfo
  # '/A:2-8' on A_COPY which was set to describe the merge.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Sync A to A_COPY.',
                                     wc_dir)

  # r10 - A simple text edit on our "trunk" A.
  svntest.main.file_write(mu_path, "Edit on 'trunk'.\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', "Work on 'trunk'",
                                     wc_dir)

  # r11 - Sync the A_COPY_2 branch with A up to HEAD (r10).
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'merge', sbox.repo_url + '/A',
                                     A_COPY_2_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'Sync A to A_COPY_2.',
                                     wc_dir)

  # Confirm that the mergeinfo on each branch is what we expect.
  svntest.actions.run_and_verify_svn(None,
                                     [A_COPY_path + ' - /A:2-8\n'],
                                     [], 'pg', SVN_PROP_MERGEINFO,
                                     '-R', A_COPY_path)
  svntest.actions.run_and_verify_svn(None,
                                     [A_COPY_2_path + ' - /A:3-10\n'],
                                     [], 'pg', SVN_PROP_MERGEINFO,
                                     '-R', A_COPY_2_path)

  # Now say we want to apply the changes made on the first branch (A_COPY)
  # to the second branch (A_COPY_2).  One way to do this is a 2-URL merge
  # between A at the revision last synced to A_COPY and A_COPY_2 at HEAD (r11),
  # i.e.:
  #
  #   svn merge ^/A@8 ^/A_COPY@11 A_COPY_2_WC
  #
  # Recall from the note on r9 that this diff is simply the one text change
  # made on branch 1 and some mergeinfo:
  #
  #   >svn diff ^/A@8 ^/A_COPY@11
  #   Index: B/lambda
  #   ===================================================================
  #   --- B/lambda    (.../A) (revision 8)
  #   +++ B/lambda    (.../A_COPY)    (revision 11)
  #   @@ -1 +1 @@
  #   -This is the file 'lambda'.
  #   +Edit on 'branch 1'.
  #
  #   Property changes on: .
  #   ___________________________________________________________________
  #   Added: svn:mergeinfo
  #      Merged /A:r2-8
  #
  # The mergeinfo diff is already represented in A_COPY_2's mergeinfo, so the
  # result of the merge should be the text change to lambda and the addition
  # of mergeinfo showing that the history of A_COPY is now part of A_COPY_2,
  # i.e. '/A_COPY:2-11'
  #
  # Before issue #3648 was fixed this test failed because the valid mergeinfo
  # '/A:r3-8' on A_COPY_2 was removed by the merge.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_output = wc.State(A_COPY_2_path, {
    ''         : Item(status=' G'),
    'B/lambda' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_COPY_2_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_COPY_2_path, {
    })
  expected_status = wc.State(A_COPY_2_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='  '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='M '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=11)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO :
                              '/A:3-10\n/A_COPY:2-11'}),
    'B'         : Item(),
    'mu'        : Item("Edit on 'trunk'.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("Edit on 'branch 1'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A_COPY_path, {})
  svntest.actions.run_and_verify_merge(A_COPY_2_path, 8, 11,
                                       sbox.repo_url + '/A',
                                       sbox.repo_url + '/A_COPY',
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1, 1)

#----------------------------------------------------------------------
# Test for issue #3867 'reintegrate merges create mergeinfo for
# non-existent paths'.
@Issue(3867)
def reintegrate_creates_bogus_mergeinfo(sbox):
  "reintegrate creates bogus mergeinfo"

  sbox.build()
  wc_dir=sbox.wc_dir

  mu_path         = os.path.join(sbox.wc_dir, "A", "mu")
  lambda_path     = os.path.join(sbox.wc_dir, "A", "B", "lambda")
  alpha_path      = os.path.join(sbox.wc_dir, "A", "B", "E", "alpha")
  beta_path       = os.path.join(sbox.wc_dir, "A", "B", "E", "beta")
  A_path          = os.path.join(sbox.wc_dir, "A")
  A_path_1        = os.path.join(sbox.wc_dir, "A@1")
  A_COPY_path     = os.path.join(sbox.wc_dir, "A_COPY")
  A_COPY_psi_path = os.path.join(sbox.wc_dir, "A_COPY", "D", "H", "psi")
  A_COPY_url      = sbox.repo_url + "/A_COPY"

  # Make 2 commits under /A pushing the repo to rev3

  svntest.main.file_write(mu_path, "New content.\n")
  svntest.main.run_svn(None, "ci", "-m", "simple text edit", wc_dir)
  svntest.main.file_write(lambda_path, "New content.\n")
  svntest.main.run_svn(None, "ci", "-m", "simple text edit", wc_dir)

  # Branch A@1 as A_COPY in revision 4

  svntest.main.run_svn(None, "cp", A_path_1, A_COPY_path)
  svntest.main.run_svn(None, "ci", "-m", "create a branch", wc_dir)

  # Make a text edit on the branch pushing the repo to r5
  svntest.main.file_write(A_COPY_psi_path, "Branch edit.\n")
  svntest.main.run_svn(None, "ci", "-m", "branch edit", wc_dir)

  # Sync the A_COPY with A in preparation for reintegrate and commit as r6.
  svntest.main.run_svn(None, "up", wc_dir)
  svntest.main.run_svn(None, "merge", sbox.repo_url + "/A", A_COPY_path)
  svntest.main.run_svn(None, "ci", "-m", "sync A_COPY with A", wc_dir)

  # Update the working copy to allow the merge
  svntest.main.run_svn(None, "up", wc_dir)

  # Reintegrate A_COPY to A.  The resulting merginfo on A should be
  # /A_COPY:4-10
  expected_output = wc.State(A_path, {
    'D/H/psi' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    '' : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO :
                              '/A_COPY:4-6'}),
    'B'         : Item(),
    'mu'        : Item("New content.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("This is the file 'beta'.\n"),
    'B/lambda'  : Item("New content.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("This is the file 'rho'.\n"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("Branch edit.\n"),
    'D/H/omega' : Item("This is the file 'omega'.\n"),
    })
  expected_skip = wc.State(A_COPY_path, {})

  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       A_COPY_url, None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk, None, expected_skip,
                                       None, None, None, None, None,
                                       1, 1, "--reintegrate", A_path)


#----------------------------------------------------------------------
# Test for regression on 1.6.x branch, merge fails when source without
# subtree mergeinfo is reintegrated into a target with subtree
# mergeinfo.  Deliberately written in a style that works with the 1.6
# testsuite.
@Issue(3957)
def no_source_subtree_mergeinfo(sbox):
  "source without subtree mergeinfo"

  sbox.build()
  wc_dir=sbox.wc_dir

  svntest.main.file_write(os.path.join(wc_dir, 'A', 'B', 'E', 'alpha'),
                          'AAA\n' +
                          'BBB\n' +
                          'CCC\n')
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Create branch-1
  svntest.main.run_svn(None, 'copy',
                       os.path.join(wc_dir, 'A', 'B'),
                       os.path.join(wc_dir, 'A', 'B1'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)

  # Create branch-1
  svntest.main.run_svn(None, 'copy',
                       os.path.join(wc_dir, 'A', 'B'),
                       os.path.join(wc_dir, 'A', 'B2'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)

  # Change on trunk
  svntest.main.file_write(os.path.join(wc_dir, 'A', 'B', 'E', 'alpha'),
                          'AAAxx\n' +
                          'BBB\n' +
                          'CCC\n')
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)

  # Change on branch-1
  svntest.main.file_write(os.path.join(wc_dir, 'A', 'B1', 'E', 'alpha'),
                          'AAA\n' +
                          'BBBxx\n' +
                          'CCC\n')
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)

  # Change on branch-2
  svntest.main.file_write(os.path.join(wc_dir, 'A', 'B2', 'E', 'alpha'),
                          'AAA\n' +
                          'BBB\n' +
                          'CCCxx\n')
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Merge trunk to branch-1
  svntest.main.run_svn(None, 'merge', '^/A/B', os.path.join(wc_dir, 'A', 'B1'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Reintegrate branch-1 subtree to trunk subtree
  svntest.main.run_svn(None, 'merge', '--reintegrate',
                       '^/A/B1/E', os.path.join(wc_dir, 'A', 'B', 'E'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Merge trunk to branch-2
  svntest.main.run_svn(None, 'merge', '^/A/B', os.path.join(wc_dir, 'A', 'B2'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Reverse merge branch-1 subtree to branch-2 subtree, this removes
  # the subtree mergeinfo from branch 2
  svntest.main.run_svn(None, 'merge', '-r8:2',
                       '^/A/B1/E', os.path.join(wc_dir, 'A', 'B2', 'E'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Merge trunk to branch-2
  svntest.main.run_svn(None, 'merge', '^/A/B', os.path.join(wc_dir, 'A', 'B2'))
  svntest.main.run_svn(None, 'commit', '-m', 'log message', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # Reintegrate branch-2 to trunk, this fails in 1.6.x from 1.6.13.
  # The error message states revisions /A/B/E:3-11 are missing from
  # /A/B2/E and yet the mergeinfo on /A/B2 is /A/B:3-11 and /A/B2/E
  # has no mergeinfo.
  expected_output = wc.State(os.path.join(wc_dir, 'A', 'B'), {
      'E'       : Item(status=' U'),
      'E/alpha' : Item(status='U '),
      })
  expected_mergeinfo = wc.State(os.path.join(wc_dir, 'A', 'B'), {
      '' : Item(status=' U'),
      })
  expected_elision = wc.State(os.path.join(wc_dir, 'A', 'B'), {
      })
  expected_disk = wc.State('', {
      ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B2:4-12'}),
      'E'       : Item(),
      'E/alpha' : Item("AAA\n" +
                       "BBB\n" +
                       "CCCxx\n"),
      'E/beta'  : Item("This is the file 'beta'.\n"),
      'F'       : Item(),
      'lambda'  : Item("This is the file 'lambda'.\n"),
      })
  expected_skip = wc.State(os.path.join(wc_dir, 'A', 'B'), {
      })
  svntest.actions.run_and_verify_merge(os.path.join(wc_dir, 'A', 'B'),
                                       None, None, '^/A/B2', None,
                                       expected_output, expected_mergeinfo,
                                       expected_elision, expected_disk,
                                       None, expected_skip,
                                       None, None, None, None, None,
                                       1, 1, '--reintegrate',
                                       os.path.join(wc_dir, 'A', 'B'))
  # For 1.6 testsuite use:
  # svntest.actions.run_and_verify_merge(os.path.join(wc_dir, 'A', 'B'),
  #                                      None, None, '^/A/B2',
  #                                      expected_output,
  #                                      expected_disk,
  #                                      None, expected_skip,
  #                                      None, None, None, None, None,
  #                                      1, 1, '--reintegrate')

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
@Issue(3961)
def reintegrate_replaced_source(sbox):
  "reintegrate a replaced source branch"

  # Make A_COPY branch in r2, and do a few more commits to A in r3-6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  A_path         = os.path.join(sbox.wc_dir, "A")
  A_COPY_path    = os.path.join(sbox.wc_dir, "A_COPY")
  beta_COPY_path = os.path.join(sbox.wc_dir, "A_COPY", "B", "E", "beta")
  mu_COPY_path   = os.path.join(sbox.wc_dir, "A_COPY", "mu")  

  # Using cherrypick merges, simulate a series of sync merges from A to
  # A_COPY with a replace of A_COPY along the way.
  #
  # r6 - Merge r3 from A to A_COPY
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'merge', sbox.repo_url + '/A', A_COPY_path,
                       '-c3')
  svntest.main.run_svn(None, 'ci', '-m', 'Merge r3 from A to A_COPY', wc_dir)

  # r8 - Merge r4 from A to A_COPY
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'merge', sbox.repo_url + '/A', A_COPY_path,
                       '-c4')
  svntest.main.run_svn(None, 'ci', '-m', 'Merge r4 from A to A_COPY', wc_dir)

  # r9 - Merge r5 from A to A_COPY. Make an additional edit to
  # A_COPY/B/E/beta.
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'merge', sbox.repo_url + '/A', A_COPY_path,
                       '-c5')
  svntest.main.file_write(beta_COPY_path, "Branch edit mistake.\n")
  svntest.main.run_svn(None, 'ci', '-m', 'Merge r5 from A to A_COPY', wc_dir)

  # r10 - Delete A_COPY and replace it with A_COPY@8. This removes the edit
  # we made above in r9 to A_COPY/B/E/beta.
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'delete', A_COPY_path)
  svntest.main.run_svn(None, 'copy', sbox.repo_url + '/A_COPY@8',
                       A_COPY_path)
  svntest.main.run_svn(None, 'ci', '-m', 'Replace A_COPY with A_COPY@8',
                       wc_dir)

  # r11 - Make an edit on A_COPY/mu.
  svntest.main.file_write(mu_COPY_path, "Branch edit.\n")
  svntest.main.run_svn(None, 'ci', '-m', 'Branch edit',
                       wc_dir)

  # r12 - Do a final sync merge of A to A_COPY in preparation for
  # reintegration.  
  svntest.main.run_svn(None, 'up', wc_dir)
  svntest.main.run_svn(None, 'merge', sbox.repo_url + '/A', A_COPY_path)
  svntest.main.run_svn(None, 'ci', '-m', 'Sycn A_COPY with A', wc_dir)

  # Reintegrate A_COPY to A.  The resulting mergeinfo should be
  # '/A_COPY:2-8,10-12' because of the replacement which removed /A_COPY:9
  # from the reintegrate source's history.
  svntest.main.run_svn(None, 'up', wc_dir)
  expected_output = wc.State(A_path, {
    'mu' : Item(status='U '),
    })
  expected_mergeinfo_output = wc.State(A_path, {
    ''   : Item(status=' U'),
    })
  expected_elision_output = wc.State(A_path, {
    })
  expected_status = wc.State(A_path, {
    ''          : Item(status=' M'),
    'B'         : Item(status='  '),
    'mu'        : Item(status='M '),
    'B/E'       : Item(status='  '),
    'B/E/alpha' : Item(status='  '),
    'B/E/beta'  : Item(status='  '),
    'B/lambda'  : Item(status='  '),
    'B/F'       : Item(status='  '),
    'C'         : Item(status='  '),
    'D'         : Item(status='  '),
    'D/G'       : Item(status='  '),
    'D/G/pi'    : Item(status='  '),
    'D/G/rho'   : Item(status='  '),
    'D/G/tau'   : Item(status='  '),
    'D/gamma'   : Item(status='  '),
    'D/H'       : Item(status='  '),
    'D/H/chi'   : Item(status='  '),
    'D/H/psi'   : Item(status='  '),
    'D/H/omega' : Item(status='  '),
    })
  expected_status.tweak(wc_rev=12)
  expected_disk = wc.State('', {
    ''          : Item(props={SVN_PROP_MERGEINFO : '/A_COPY:2-8,10-12'}),
    'B'         : Item(),
    'mu'        : Item("Branch edit.\n"),
    'B/E'       : Item(),
    'B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'B/E/beta'  : Item("New content"),
    'B/lambda'  : Item("This is the file 'lambda'.\n"),
    'B/F'       : Item(),
    'C'         : Item(),
    'D'         : Item(),
    'D/G'       : Item(),
    'D/G/pi'    : Item("This is the file 'pi'.\n"),
    'D/G/rho'   : Item("New content"),
    'D/G/tau'   : Item("This is the file 'tau'.\n"),
    'D/gamma'   : Item("This is the file 'gamma'.\n"),
    'D/H'       : Item(),
    'D/H/chi'   : Item("This is the file 'chi'.\n"),
    'D/H/psi'   : Item("New content"),
    'D/H/omega' : Item("New content"),
    })
  expected_skip = wc.State(A_path, { })
  svntest.actions.run_and_verify_merge(A_path, None, None,
                                       sbox.repo_url + '/A_COPY', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       [], None, None, None, None, True, True,
                                       '--reintegrate', A_path)
  
#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_posix_os)
@Issue(4052)
def reintegrate_symlink_deletion(sbox):
  "reintegrate symlink deletion"

  sbox.build()
  wc_dir = sbox.wc_dir

  ## path vars
  A_path = sbox.ospath('A')
  A_omicron_path = sbox.ospath('A/omicron')
  mu_path = sbox.ospath('A/mu')
  A_COPY_path = sbox.ospath('A_COPY')
  A_COPY_omicron_path = sbox.ospath('A_COPY/omicron')
  A_url = sbox.repo_url + "/A"
  A_COPY_url = sbox.repo_url + "/A_COPY"

  ## add symlink
  os.symlink(mu_path, A_omicron_path)
  sbox.simple_add('A/omicron')
  sbox.simple_commit(message='add symlink')

  ## branch
  sbox.simple_repo_copy('A', 'A_COPY')
  sbox.simple_update()

  ## branch rm
  sbox.simple_rm('A_COPY/omicron')
  sbox.simple_commit(message='remove symlink on branch')

  ## Note: running update at this point avoids the bug.

  ## reintegrate
  # ### TODO: verify something here
  svntest.main.run_svn(None, 'merge', '--reintegrate',
                       A_COPY_url, A_path)



########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              basic_reintegrate,
              reintegrate_with_rename,
              reintegrate_branch_never_merged_to,
              reintegrate_fail_on_modified_wc,
              reintegrate_fail_on_mixed_rev_wc,
              reintegrate_fail_on_switched_wc,
              reintegrate_on_shallow_wc,
              reintegrate_fail_on_stale_source,
              merge_file_with_space_in_its_path,
              reintegrate_with_subtree_mergeinfo,
              multiple_reintegrates_from_the_same_branch,
              reintegrate_with_self_referential_mergeinfo,
              added_subtrees_with_mergeinfo_break_reintegrate,
              two_URL_merge_removes_valid_mergeinfo_from_target,
              reintegrate_creates_bogus_mergeinfo,
              no_source_subtree_mergeinfo,
              reintegrate_replaced_source,
              reintegrate_symlink_deletion,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
