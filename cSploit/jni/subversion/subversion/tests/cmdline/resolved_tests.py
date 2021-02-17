#!/usr/bin/env python
#
#  resolved_tests.py:  testing "resolved" cases.
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
import sys, re, os

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

from svntest.main import SVN_PROP_MERGEINFO, server_has_mergeinfo

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

def resolved_on_wc_root(sbox):
  "resolved on working copy root"

  sbox.build()
  wc = sbox.wc_dir

  i = os.path.join(wc, 'iota')
  B = os.path.join(wc, 'A', 'B')
  g = os.path.join(wc, 'A', 'D', 'gamma')

  # Create some conflicts...
  # Commit mods
  svntest.main.file_append(i, "changed iota.\n")
  svntest.main.file_append(g, "changed gamma.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', B)

  expected_output = svntest.wc.State(wc, {
      'iota'              : Item(verb='Sending'),
      'A/B'               : Item(verb='Sending'),
      'A/D/gamma'         : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('iota', 'A/B', 'A/D/gamma', wc_rev = 2)

  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc)

  # Go back to rev 1
  expected_output = svntest.wc.State(wc, {
    'iota'              : Item(status='U '),
    'A/B'               : Item(status=' U'),
    'A/D/gamma'         : Item(status='U '),
  })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_disk = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '-r1', wc)

  # Deletions so that the item becomes unversioned and
  # will have a tree-conflict upon update.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', i, B, g)

  # Update so that conflicts appear
  expected_output = svntest.wc.State(wc, {
    'iota'              : Item(status='  ', treeconflict='C'),
    'A/B'               : Item(status='  ', treeconflict='C'),
    'A/D/gamma'         : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota',
                       'A/B/lambda',
                       'A/B/E/alpha', 'A/B/E/beta',
                       'A/D/gamma')
  if svntest.main.wc_is_singledb(sbox.wc_dir):
    expected_disk.remove('A/B/E', 'A/B/F', 'A/B')

  expected_status = svntest.actions.get_virginal_state(wc, 2)
  expected_status.tweak('iota', 'A/B', 'A/D/gamma',
                        status='D ', treeconflict='C')
  expected_status.tweak('A/B/lambda', 'A/B/E', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/B/F', status='D ')
  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        None,
                                        None, None, None, None, None, False,
                                        wc)
  svntest.actions.run_and_verify_unquiet_status(wc, expected_status)

  # Resolve recursively
  svntest.actions.run_and_verify_resolved([i, B, g], '--depth=infinity', wc)

  expected_status.tweak('iota', 'A/B', 'A/D/gamma', treeconflict=None)
  svntest.actions.run_and_verify_unquiet_status(wc, expected_status)





def resolved_on_deleted_item(sbox):
  "resolved on deleted item"

  sbox.build()
  wc = sbox.wc_dir

  A = os.path.join(wc, 'A',)
  B = os.path.join(wc, 'A', 'B')
  g = os.path.join(wc, 'A', 'D', 'gamma')
  A2 = os.path.join(wc, 'A2')
  B2 = os.path.join(A2, 'B')
  g2 = os.path.join(A2, 'D', 'gamma')

  A_url = sbox.repo_url + '/A'
  A2_url = sbox.repo_url + '/A2'

  # make a copy of A
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', A_url, A2_url, '-m', 'm')

  expected_output = svntest.wc.State(wc, {
    'A2'                : Item(status='A '),
    'A2/B'              : Item(status='A '),
    'A2/B/lambda'       : Item(status='A '),
    'A2/B/E'            : Item(status='A '),
    'A2/B/E/alpha'      : Item(status='A '),
    'A2/B/E/beta'       : Item(status='A '),
    'A2/B/F'            : Item(status='A '),
    'A2/mu'             : Item(status='A '),
    'A2/C'              : Item(status='A '),
    'A2/D'              : Item(status='A '),
    'A2/D/gamma'        : Item(status='A '),
    'A2/D/G'            : Item(status='A '),
    'A2/D/G/pi'         : Item(status='A '),
    'A2/D/G/rho'        : Item(status='A '),
    'A2/D/G/tau'        : Item(status='A '),
    'A2/D/H'            : Item(status='A '),
    'A2/D/H/chi'        : Item(status='A '),
    'A2/D/H/omega'      : Item(status='A '),
    'A2/D/H/psi'        : Item(status='A '),
  })


  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A2/mu'             : Item(contents="This is the file 'mu'.\n"),
    'A2/D/gamma'        : Item(contents="This is the file 'gamma'.\n"),
    'A2/D/H/psi'        : Item(contents="This is the file 'psi'.\n"),
    'A2/D/H/omega'      : Item(contents="This is the file 'omega'.\n"),
    'A2/D/H/chi'        : Item(contents="This is the file 'chi'.\n"),
    'A2/D/G/rho'        : Item(contents="This is the file 'rho'.\n"),
    'A2/D/G/pi'         : Item(contents="This is the file 'pi'.\n"),
    'A2/D/G/tau'        : Item(contents="This is the file 'tau'.\n"),
    'A2/B/lambda'       : Item(contents="This is the file 'lambda'.\n"),
    'A2/B/F'            : Item(),
    'A2/B/E/beta'       : Item(contents="This is the file 'beta'.\n"),
    'A2/B/E/alpha'      : Item(contents="This is the file 'alpha'.\n"),
    'A2/C'              : Item(),
  })


  expected_status = svntest.actions.get_virginal_state(wc, 2)
  expected_status.add({
    'A2'                : Item(),
    'A2/B'              : Item(),
    'A2/B/lambda'       : Item(),
    'A2/B/E'            : Item(),
    'A2/B/E/alpha'      : Item(),
    'A2/B/E/beta'       : Item(),
    'A2/B/F'            : Item(),
    'A2/mu'             : Item(),
    'A2/C'              : Item(),
    'A2/D'              : Item(),
    'A2/D/gamma'        : Item(),
    'A2/D/G'            : Item(),
    'A2/D/G/pi'         : Item(),
    'A2/D/G/rho'        : Item(),
    'A2/D/G/tau'        : Item(),
    'A2/D/H'            : Item(),
    'A2/D/H/chi'        : Item(),
    'A2/D/H/omega'      : Item(),
    'A2/D/H/psi'        : Item(),
  })
  expected_status.tweak(status='  ', wc_rev='2')

  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        wc)

  # Create some conflicts...

  # Modify the paths in the one directory.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', B)
  svntest.main.file_append(g, "Modified gamma.\n")

  expected_output = svntest.wc.State(wc, {
      'A/B'               : Item(verb='Sending'),
      'A/D/gamma'         : Item(verb='Sending'),
    })

  expected_status.tweak('A/B', 'A/D/gamma', wc_rev='3')

  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc)

  # Delete the paths in the second directory.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', B2, g2)

  expected_output = svntest.wc.State(wc, {
      'A2/B'              : Item(verb='Deleting'),
      'A2/D/gamma'        : Item(verb='Deleting'),
    })

  expected_status.remove('A2/B', 'A2/B/lambda',
                         'A2/B/E', 'A2/B/E/alpha', 'A2/B/E/beta',
                         'A2/B/F',
                         'A2/D/gamma')

  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None,
                                        A2)

  # Now merge A to A2, creating conflicts...

  expected_output = svntest.wc.State(A2, {
      'B'                 : Item(status='  ', treeconflict='C'),
      'D/gamma'           : Item(status='  ', treeconflict='C'),
    })
  expected_mergeinfo_output = svntest.wc.State(A2, {
      '' : Item(status=' U')
    })
  expected_elision_output = svntest.wc.State(A2, {
    })
  expected_disk = svntest.wc.State('', {
      'mu'                : Item(contents="This is the file 'mu'.\n"),
      'D'                 : Item(),
      'D/H'               : Item(),
      'D/H/psi'           : Item(contents="This is the file 'psi'.\n"),
      'D/H/omega'         : Item(contents="This is the file 'omega'.\n"),
      'D/H/chi'           : Item(contents="This is the file 'chi'.\n"),
      'D/G'               : Item(),
      'D/G/rho'           : Item(contents="This is the file 'rho'.\n"),
      'D/G/pi'            : Item(contents="This is the file 'pi'.\n"),
      'D/G/tau'           : Item(contents="This is the file 'tau'.\n"),
      'C'                 : Item(),
    })


  expected_skip = svntest.wc.State(wc, {
    })

  expected_status = svntest.wc.State(A2, {
    ''                  : Item(status=' M', wc_rev='2'),
    'D'                 : Item(status='  ', wc_rev='2'),
    'D/gamma'           : Item(status='! ', treeconflict='C'),
    'D/G'               : Item(status='  ', wc_rev='2'),
    'D/G/pi'            : Item(status='  ', wc_rev='2'),
    'D/G/rho'           : Item(status='  ', wc_rev='2'),
    'D/G/tau'           : Item(status='  ', wc_rev='2'),
    'D/H'               : Item(status='  ', wc_rev='2'),
    'D/H/chi'           : Item(status='  ', wc_rev='2'),
    'D/H/omega'         : Item(status='  ', wc_rev='2'),
    'D/H/psi'           : Item(status='  ', wc_rev='2'),
    'B'                 : Item(status='! ', treeconflict='C'),
    'mu'                : Item(status='  ', wc_rev='2'),
    'C'                 : Item(status='  ', wc_rev='2'),
  })

  svntest.actions.run_and_verify_merge(A2, None, None, A_url, None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk, None, expected_skip,
                                       None, dry_run = False)
  svntest.actions.run_and_verify_unquiet_status(A2, expected_status)


  # Now resolve by recursing on the working copy root.
  svntest.actions.run_and_verify_resolved([B2, g2], '--depth=infinity', wc)

  expected_status.remove('B', 'D/gamma')
  svntest.actions.run_and_verify_unquiet_status(A2, expected_status)



def theirs_conflict_in_subdir(sbox):
  "resolve to 'theirs-conflict' in sub-directory"

  sbox.build()
  wc = sbox.wc_dir
  wc2 = sbox.add_wc_path('wc2')
  svntest.actions.duplicate_dir(sbox.wc_dir, wc2)

  alpha_path = os.path.join(wc, 'A', 'B', 'E', 'alpha')
  alpha_path2 = os.path.join(wc2, 'A', 'B', 'E', 'alpha')

  svntest.main.file_append(alpha_path, "Modified alpha.\n")
  svntest.main.run_svn(None, 'ci', '-m', 'logmsg', wc)

  svntest.main.file_append(alpha_path2, "Modified alpha, too.\n")
  svntest.main.run_svn(None, 'up', wc2)

  svntest.actions.run_and_verify_resolve([alpha_path2],
                                         '--accept=theirs-conflict',
                                         alpha_path2)

#######################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              resolved_on_wc_root,
              resolved_on_deleted_item,
              theirs_conflict_in_subdir,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
