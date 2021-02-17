#!/usr/bin/env python
#
#  history_tests.py:  testing history-tracing code
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
from svntest import wc

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = wc.StateItem

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

#----------------------------------------------------------------------

def cat_traces_renames(sbox):
  "verify that 'svn cat' traces renames"

  sbox.build()
  wc_dir = sbox.wc_dir
  rho_path   = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  pi_path    = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  bloo_path  = os.path.join(wc_dir, 'A', 'D', 'G', 'bloo')

  # rename rho to bloo. commit r2.
  svntest.main.run_svn(None, 'mv', rho_path, bloo_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Deleting'),
    'A/D/G/bloo' : Item(verb='Adding')
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G/rho');
  expected_status.add({ 'A/D/G/bloo' :
                        Item(wc_rev=2, status='  ') })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # rename pi to rho.  commit r3.
  svntest.main.run_svn(None, 'mv', pi_path, rho_path)

  # svn cat -r1 rho  --> should show pi's contents.
  svntest.actions.run_and_verify_svn(None,
                                     [ "This is the file 'pi'.\n"], [],
                                     'cat',  '-r', '1', rho_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi' : Item(verb='Deleting'),
    'A/D/G/rho' : Item(verb='Adding')
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G/pi');
  expected_status.tweak('A/D/G/rho', wc_rev=3)
  expected_status.add({ 'A/D/G/bloo' :
                        Item(wc_rev=2, status='  ') })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # update whole wc to HEAD
  expected_output = svntest.wc.State(wc_dir, { }) # no output
  expected_status.tweak(wc_rev=3)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G/pi', 'A/D/G/rho')
  expected_disk.add({
    'A/D/G/rho' : Item("This is the file 'pi'.\n"),
    })
  expected_disk.add({
    'A/D/G/bloo' : Item("This is the file 'rho'.\n"),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # 'svn cat bloo' --> should show rho's contents.
  svntest.actions.run_and_verify_svn(None,
                                     [ "This is the file 'rho'.\n"], [],
                                     'cat',  bloo_path)

  # svn cat -r1 bloo --> should still show rho's contents.
  svntest.actions.run_and_verify_svn(None,
                                     [ "This is the file 'rho'.\n"], [],
                                     'cat',  '-r', '1', bloo_path)

  # svn cat -r1 rho  --> should show pi's contents.
  svntest.actions.run_and_verify_svn(None,
                                     [ "This is the file 'pi'.\n"], [],
                                     'cat',  '-r', '1', rho_path)

  # svn up -r1
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r', '1', wc_dir)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # svn cat -rHEAD rho --> should see 'unrelated object' error.
  svntest.actions.run_and_verify_svn("unrelated object",
                                     None, svntest.verify.AnyOutput,
                                     'cat',  '-r', 'HEAD', rho_path)

@Issue(1970)
def cat_avoids_false_identities(sbox):
  "verify that 'svn cat' avoids false identities"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Issue #1970
  #
  # Highlight a bug in the client side use of the repository's
  # location searching algorithm.
  #
  # The buggy history-following algorithm determines the paths that a
  # line of history would be *expected to be* found in a given revision,
  # but doesn't treat copies as gaps in the historical sequence.  If
  # some other object fills those gaps at the same expected path, the
  # client will find the wrong object.
  #
  # In the recipe below, iota gets created in r1.  In r2, it is
  # deleted and replaced with an unrelated object at the same path.
  # In r3, the interloper is deleted.  In r4, the original iota is
  # resurrected via a copy from r1.
  #
  #     ,- - - - - - --.
  #    o---| o---| o    o----->
  #
  #    |     |     |    |
  #   r1    r2    r3   r4
  #
  # In a working copy at r4, running
  #
  #    $ svn cat -r2 iota
  #
  # should result in an error, but with the bug it instead cats the r2
  # interloper.
  #
  # To reassure yourself that that's wrong, recall that the above
  # command is equivalent to
  #
  #    $ svn cat -r2 iota@4
  #
  # Now do you see the evil that lies within us?

  iota_path = os.path.join(wc_dir, 'iota')
  iota_url = sbox.repo_url + '/iota'

  # r2
  svntest.main.run_svn(None, 'del', iota_path)
  svntest.main.file_append(iota_path, "YOU SHOULD NOT SEE THIS\n")
  svntest.main.run_svn(None, 'add', iota_path)
  svntest.main.run_svn(None, 'ci', '-m', 'log msg',
                       wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  # r3
  svntest.main.run_svn(None, 'del', iota_path)
  svntest.main.run_svn(None, 'ci', '-m', 'log msg',
                       wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  # r4
  svntest.main.run_svn(None, 'cp', iota_url + '@1', wc_dir)
  svntest.main.run_svn(None, 'ci', '-m', 'log msg',
                       wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  # 'svn cat -r2 iota' should error, because the line of history
  # currently identified by /iota did not exist in r2, even though a
  # totally unrelated file of the same name did.
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'cat', '-r', '2', iota_path)


########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              cat_traces_renames,
              cat_avoids_false_identities,
              ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
