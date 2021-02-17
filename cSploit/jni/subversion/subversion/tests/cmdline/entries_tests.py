#!/usr/bin/env python
#
#  entries_tests.py:  test the old entries API using entries-dump
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

#
# This test series is to validate the old entries API using the entries-dump
# tool to see what the API reports. In particular, this test is designed to
# try and exercise all "extraordinary" code paths in the read_entries()
# function in libsvn_wc/entries.c. Much of that function is exercised by
# the regular test suite and its secondary "status" via entries-dump. This
# test tries to pick up the straggly little edge cases.
#

import os

import svntest

Item = svntest.wc.StateItem


SCHEDULE_NORMAL = 0
SCHEDULE_ADD = 1
SCHEDULE_DELETE = 2
SCHEDULE_REPLACE = 3


def validate(entry, **kw):
  for key, value in kw.items():
    if getattr(entry, key) != value:
      print("Entry '%s' has an incorrect value for .%s" % (entry.name, key))
      print("  Expected: %s" % value)
      print("    Actual: %s" % getattr(entry, key))
      raise svntest.Failure


def check_names(entries, *names):
  if entries is None:
    print('entries-dump probably exited with a failure.')
    raise svntest.Failure
  have = set(entries.keys())
  want = set(names)
  missing = want - have
  if missing:
    print("Entry name(s) not found: %s"
          % ', '.join("'%s'" % name for name in missing))
    raise svntest.Failure


def basic_entries(sbox):
  "basic entries behavior"

  sbox.build()
  wc_dir = sbox.wc_dir

  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  beta_path = os.path.join(wc_dir, 'A', 'B', 'E', 'beta')
  added_path = os.path.join(wc_dir, 'A', 'B', 'E', 'added')
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  G2_path = os.path.join(wc_dir, 'A', 'D', 'G2')
  iota_path = os.path.join(wc_dir, 'iota')
  iota2_path = os.path.join(wc_dir, 'A', 'B', 'E', 'iota2')

  # Remove 'alpha'. When it is committed, it will be marked DELETED.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', alpha_path)

  # Tweak 'beta' in order to bump its revision to ensure the replacement
  # gets the new revision (2), not the value from the parent (1).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     beta_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha' : Item(verb='Deleting'),
    'A/B/E/beta' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha')
  expected_status.tweak('A/B/E/beta', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None,
                                        alpha_path, beta_path)

  # bump 'G' and iota another revision (3) for later testing
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     G_path, iota_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G' : Item(verb='Sending'),
    'iota' : Item(verb='Sending'),
    })
  expected_status.tweak('A/D/G', 'iota', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None,
                                        G_path, iota_path)

  # Add a file over the DELETED 'alpha'. It should be schedule-add.
  open(alpha_path, 'w').write('New alpha contents\n')

  # Delete 'beta', then add a file over it. Should be schedule-replace.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', beta_path)
  open(beta_path, 'w').write('New beta contents\n')

  # Plain old add. Should have revision == 0.
  open(added_path, 'w').write('Added file contents\n')

  svntest.actions.run_and_verify_svn(None, None, [], 'add',
                                     alpha_path, beta_path, added_path)

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     iota_path, iota2_path)

  entries = svntest.main.run_entriesdump(os.path.join(wc_dir, 'A', 'B', 'E'))
  check_names(entries, 'alpha', 'beta', 'added', 'iota2')

  # plain add should be rev=0. over a DELETED, should be SCHEDULE_ADD
  validate(entries['alpha'], schedule=SCHEDULE_ADD, revision=0, copied=False)

  # should pick up the BASE node's revision
  validate(entries['beta'], schedule=SCHEDULE_REPLACE, revision=2,
           copied=False)

  # plain add should be rev=0
  validate(entries['added'], schedule=SCHEDULE_ADD, revision=0, copied=False)

  # copyfrom_rev is (3), but we inherit the rev from the parent (1)
  validate(entries['iota2'], schedule=SCHEDULE_ADD, revision=1, copied=True,
           copyfrom_rev=3)

  svntest.actions.run_and_verify_svn(None, None, [], 'cp', G_path, G2_path)

  entries = svntest.main.run_entriesdump(G2_path)
  check_names(entries, 'pi', 'rho', 'tau')

  # added, but revision should match the copyfrom_rev (directories don't
  # inherit a revision like iota2 did above)
  validate(entries[''], schedule=SCHEDULE_ADD, copied=True, revision=3)

  # children should be SCHEDULE_NORMAL. still rev=1 cuz of mixed-rev source.
  validate(entries['pi'], schedule=SCHEDULE_NORMAL, copied=True, revision=1)


def obstructed_entries(sbox):
  "validate entries when obstructions exist"

  sbox.build()
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')

  # blast a directory. its revision should become SVN_INVALID_REVNUM.
  entries = svntest.main.run_entriesdump(D_path)
  check_names(entries, 'H')
  validate(entries['H'], revision=1)

  svntest.main.safe_rmtree(H_path)

  entries = svntest.main.run_entriesdump(D_path)
  check_names(entries, 'H')

  if svntest.main.wc_is_singledb(wc_dir):
    # Data is not missing in single-db
    validate(entries['H'], revision=1)
  else:
    validate(entries['H'], revision=-1)

  ### need to get svn_wc__db_read_info() to generate obstructed_add


def deletion_details(sbox):
  "various details about deleted nodes"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  D_path = os.path.join(wc_dir, 'A', 'D')
  D2_path = os.path.join(wc_dir, 'A', 'D2')
  D2_G_path = os.path.join(wc_dir, 'A', 'D2', 'G')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')

  entries = svntest.main.run_entriesdump(wc_dir)
  check_names(entries, 'iota')
  iota = entries['iota']

  # blast iota, then verify the now-deleted entry still contains much of
  # the same information.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', iota_path)
  entries = svntest.main.run_entriesdump(wc_dir)
  check_names(entries, 'iota')
  validate(entries['iota'], revision=iota.revision,
           cmt_rev=iota.cmt_rev, cmt_author=iota.cmt_author)

  # even deleted nodes have a URL
  validate(entries['iota'], url='%s/iota' % sbox.repo_url)

  svntest.actions.run_and_verify_svn(None, None, [], 'cp', D_path, D2_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', D2_G_path)

  entries = svntest.main.run_entriesdump(D2_path)
  check_names(entries, 'gamma', 'G')

  # copied nodes have URLs
  validate(entries['gamma'], url='%s/A/D2/gamma' % sbox.repo_url,
           copied=True, schedule=SCHEDULE_NORMAL)

  entries = svntest.main.run_entriesdump(D2_G_path)
  check_names(entries, 'pi')

  # oh, and this sucker has a URL, too
  validate(entries['pi'], url='%s/A/D2/G/pi' % sbox.repo_url,
           copied=True, schedule=SCHEDULE_DELETE)

  ### hmm. somehow, subtrees can be *added* over a *deleted* subtree.
  ### maybe this can happen via 'svn merge' ? ... the operations below
  ### will fail because E_path is scheduled for deletion, disallowing
  ### any new node to sit on top of it. (tho it *should* allow it...)

  ### for now... this test case is done. just return
  return

  svntest.actions.run_and_verify_svn(None, None, [], 'rm', E_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', H_path, E_path)

  entries = svntest.main.run_entriesdump(E_path)
  check_names(entries, 'chi', 'omega', 'psi', 'alpha', 'beta')

  validate(entries['alpha'], schedule=SCHEDULE_DELETE)
  validate(entries['chi'], schedule=SCHEDULE_NORMAL, copied=True)



########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              basic_entries,
              obstructed_entries,
              deletion_details,
             ]


if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED
