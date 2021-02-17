#!/usr/bin/env python
#
#  depth_tests.py:  Testing that operations work as expected at
#                   various depths (depth-empty, depth-files,
#                   depth-immediates, depth-infinity).
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
import re

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

# For errors setting up the depthy working copies.
class DepthSetupError(Exception):
  def __init__ (self, args=None):
    self.args = args

def set_up_depthy_working_copies(sbox, empty=False, files=False,
                                 immediates=False, infinity=False):
  """Set up up to four working copies, at various depths.  At least
  one of depths EMPTY, FILES, IMMEDIATES, or INFINITY must be passed
  as True.  The corresponding working copy paths are returned in a
  four-element tuple in that order, with element value of None for
  working copies that were not created.  If all args are False, raise
  DepthSetupError."""

  if not (infinity or empty or files or immediates):
    raise DepthSetupError("At least one working copy depth must be passed.")

  wc = None
  if infinity:
    sbox.build()
    wc = sbox.wc_dir
  else:
    sbox.build(create_wc = False)
    sbox.add_test_path(sbox.wc_dir, True)

  wc_empty = None
  if empty:
    wc_empty = sbox.wc_dir + '-depth-empty'
    sbox.add_test_path(wc_empty, True)
    svntest.actions.run_and_verify_svn(
      "Unexpected error from co --depth=empty",
      svntest.verify.AnyOutput, [],
      "co", "--depth", "empty", sbox.repo_url, wc_empty)

  wc_files = None
  if files:
    wc_files = sbox.wc_dir + '-depth-files'
    sbox.add_test_path(wc_files, True)
    svntest.actions.run_and_verify_svn(
      "Unexpected error from co --depth=files",
      svntest.verify.AnyOutput, [],
      "co", "--depth", "files", sbox.repo_url, wc_files)

  wc_immediates = None
  if immediates:
    wc_immediates = sbox.wc_dir + '-depth-immediates'
    sbox.add_test_path(wc_immediates, True)
    svntest.actions.run_and_verify_svn(
      "Unexpected error from co --depth=immediates",
      svntest.verify.AnyOutput, [],
      "co", "--depth", "immediates",
      sbox.repo_url, wc_immediates)

  return wc_empty, wc_files, wc_immediates, wc

def verify_depth(msg, depth, path="."):
  """Verifies that PATH has depth DEPTH.  MSG is the failure message."""
  if depth == "infinity":
    # Check for absence of depth line.
    exit_code, out, err = svntest.actions.run_and_verify_svn(None, None,
                                                             [], "info", path)
    for line in out:
      if line.startswith("Depth:"):
        raise svntest.failure(msg)
  else:
    expected_stdout = svntest.verify.ExpectedOutput("Depth: %s\n" % depth,
                                                    match_all=False)
    svntest.actions.run_and_verify_svn(
      msg, expected_stdout, [], "info", path)

#----------------------------------------------------------------------
# Ensure that 'checkout --depth=empty' results in a depth-empty working copy.
def depth_empty_checkout(sbox):
  "depth-empty checkout"

  wc_empty, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox, empty=True)

  if os.path.exists(os.path.join(wc_empty, "iota")):
    raise svntest.Failure("depth-empty checkout created file 'iota'")

  if os.path.exists(os.path.join(wc_empty, "A")):
    raise svntest.Failure("depth-empty checkout created subdir 'A'")

  verify_depth("Expected depth empty for top of WC, got some other depth",
               "empty", wc_empty)


# Helper for two test functions.
def depth_files_same_as_nonrecursive(sbox, opt):
  """Run a depth-files or non-recursive checkout, depending on whether
  passed '-N' or '--depth=files' for OPT.  The two should get the same
  result, hence this helper containing the common code between the
  two tests."""

  # This duplicates some code from set_up_depthy_working_copies(), but
  # that's because it's abstracting out a different axis.

  sbox.build(create_wc = False, read_only = True)
  if os.path.exists(sbox.wc_dir):
    svntest.main.safe_rmtree(sbox.wc_dir)

  svntest.actions.run_and_verify_svn("Unexpected error during co %s" % opt,
                                     svntest.verify.AnyOutput, [],
                                     "co", opt, sbox.repo_url, sbox.wc_dir)

  # Should create a depth-files top directory, so both iota and A
  # should exist, and A should be empty and depth-empty.

  if not os.path.exists(os.path.join(sbox.wc_dir, "iota")):
    raise svntest.Failure("'checkout %s' failed to create file 'iota'" % opt)

  if os.path.exists(os.path.join(sbox.wc_dir, "A")):
    raise svntest.Failure("'checkout %s' unexpectedly created subdir 'A'" % opt)

  verify_depth("Expected depth files for top of WC, got some other depth",
               "files", sbox.wc_dir)


def depth_files_checkout(sbox):
  "depth-files checkout"
  depth_files_same_as_nonrecursive(sbox, "--depth=files")


def nonrecursive_checkout(sbox):
  "non-recursive checkout equals depth-files"
  depth_files_same_as_nonrecursive(sbox, "-N")


#----------------------------------------------------------------------
def depth_empty_update_bypass_single_file(sbox):
  "update depth-empty wc shouldn't receive file mod"

  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  iota_path = os.path.join(wc, 'iota')
  svntest.main.file_append(iota_path, "new text\n")

  # Commit in the "other" wc.
  expected_output = svntest.wc.State(wc, { 'iota' : Item(verb='Sending'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-empty wc, expecting not to receive the change to iota.
  expected_output = svntest.wc.State(wc_empty, { })
  expected_disk = svntest.wc.State('', { })
  expected_status = svntest.wc.State(wc_empty, { '' : svntest.wc.StateItem() })
  expected_status.tweak(contents=None, status='  ', wc_rev=2)
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)

  # And the wc should still be depth-empty.
  verify_depth(None, "empty", wc_empty)

  # Even if we explicitly ask for a depth-infinity update, we still shouldn't
  # get the change to iota.
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        "--depth=infinity", wc_empty)

  # And the wc should still be depth-empty.
  verify_depth(None, "empty", wc_empty)


#----------------------------------------------------------------------
def depth_immediates_get_top_file_mod_only(sbox):
  "update depth-immediates wc gets top file mod only"

  ign_a, ign_b, wc_immediates, wc \
         = set_up_depthy_working_copies(sbox, immediates=True, infinity=True)

  iota_path = os.path.join(wc, 'iota')
  svntest.main.file_append(iota_path, "new text in iota\n")
  mu_path = os.path.join(wc, 'A', 'mu')
  svntest.main.file_append(mu_path, "new text in mu\n")

  # Commit in the "other" wc.
  expected_output = svntest.wc.State(wc,
                                     { 'iota' : Item(verb='Sending'),
                                       'A/mu' : Item(verb='Sending'),
                                       })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')
  expected_status.tweak('A/mu', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-immediates wc, expecting to receive only the
  # change to iota.
  expected_output = svntest.wc.State(wc_immediates,
                                     { 'iota' : Item(status='U ') })
  expected_disk = svntest.wc.State('', { })
  expected_disk.add(\
    {'iota' : Item(contents="This is the file 'iota'.\nnew text in iota\n"),
     'A' : Item(contents=None) } )
  expected_status = svntest.wc.State(wc_immediates,
                                     { '' : svntest.wc.StateItem() })
  expected_status.tweak(contents=None, status='  ', wc_rev=2)
  expected_status.add(\
    {'iota' : Item(status='  ', wc_rev=2),
     'A' : Item(status='  ', wc_rev=2) } )
  svntest.actions.run_and_verify_update(wc_immediates,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)
  verify_depth(None, "immediates", wc_immediates)


#----------------------------------------------------------------------
def depth_empty_commit(sbox):
  "commit a file from a depth-empty working copy"
  # Bring iota into a depth-empty working copy, then commit a change to it.
  wc_empty, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox,
                                                               empty=True)

  # Form the working path of iota
  wc_empty_iota = os.path.join(wc_empty, 'iota')

  # Update 'iota' in the depth-empty working copy and modify it
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', wc_empty_iota)
  svntest.main.file_write(wc_empty_iota, "iota modified")

  # Commit the modified changes from a depth-empty working copy
  expected_output = svntest.wc.State(wc_empty, {
    'iota'        : Item(verb='Sending'),
    })
  expected_status = svntest.wc.State(wc_empty, { })
  expected_status.add({
    ''            : Item(status='  ', wc_rev=1),
    'iota'        : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_commit(wc_empty,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_empty)

#----------------------------------------------------------------------
def depth_empty_with_file(sbox):
  "act on a file in a depth-empty working copy"
  # Run 'svn up iota' to bring iota permanently into the working copy.
  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  iota_path = os.path.join(wc_empty, 'iota')
  if os.path.exists(iota_path):
    raise svntest.Failure("'%s' exists when it shouldn't" % iota_path)

  ### I'd love to do this using the recommended {expected_output,
  ### expected_status, expected_disk} method here, but after twenty
  ### minutes of trying to figure out how, I decided to compromise.

  # Update iota by name, expecting to receive it.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', iota_path)

  # Test that we did receive it.
  if not os.path.exists(iota_path):
    raise svntest.Failure("'%s' doesn't exist when it should" % iota_path)

  # Commit a change to iota in the "other" wc.
  other_iota_path = os.path.join(wc, 'iota')
  svntest.main.file_append(other_iota_path, "new text\n")
  expected_output = svntest.wc.State(wc, { 'iota' : Item(verb='Sending'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Delete iota in the "other" wc.
  other_iota_path = os.path.join(wc, 'iota')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', other_iota_path)
  expected_output = svntest.wc.State(wc, { 'iota' : Item(verb='Deleting'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.remove('iota')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-empty wc just a little, expecting to receive
  # the change in iota.
  expected_output = svntest.wc.State(\
    wc_empty, { 'iota' : Item(status='U ') })
  expected_disk = svntest.wc.State(\
    '', { 'iota' : Item(contents="This is the file 'iota'.\nnew text\n") })
  expected_status = svntest.wc.State(wc_empty,
    { ''     : Item(status='  ', wc_rev=2),
      'iota' : Item(status='  ', wc_rev=2),})
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '-r2', wc_empty)

  # Update the depth-empty wc all the way, expecting to receive the deletion
  # of iota.
  expected_output = svntest.wc.State(\
    wc_empty, { 'iota' : Item(status='D ') })
  expected_disk = svntest.wc.State('', { })
  expected_status = svntest.wc.State(\
    wc_empty, { '' : Item(status='  ', wc_rev=3) })
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)


#----------------------------------------------------------------------
def depth_empty_with_dir(sbox):
  "bring a dir into a depth-empty working copy"
  # Run 'svn up A' to bring A permanently into the working copy.
  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  A_path = os.path.join(wc_empty, 'A')
  other_mu_path = os.path.join(wc, 'A', 'mu')

  # We expect A to be added at depth infinity, so a normal 'svn up A'
  # should be sufficient to add all descendants.
  expected_output = svntest.wc.State(wc_empty, {
    'A'              : Item(status='A '),
    'A/mu'           : Item(status='A '),
    'A/B'            : Item(status='A '),
    'A/B/lambda'     : Item(status='A '),
    'A/B/E'          : Item(status='A '),
    'A/B/E/alpha'    : Item(status='A '),
    'A/B/E/beta'     : Item(status='A '),
    'A/B/F'          : Item(status='A '),
    'A/C'            : Item(status='A '),
    'A/D'            : Item(status='A '),
    'A/D/gamma'      : Item(status='A '),
    'A/D/G'          : Item(status='A '),
    'A/D/G/pi'       : Item(status='A '),
    'A/D/G/rho'      : Item(status='A '),
    'A/D/G/tau'      : Item(status='A '),
    'A/D/H'          : Item(status='A '),
    'A/D/H/chi'      : Item(status='A '),
    'A/D/H/psi'      : Item(status='A '),
    'A/D/H/omega'    : Item(status='A ')
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 1)
  expected_status.remove('iota')
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        A_path)

  # Commit a change to A/mu in the "other" wc.
  svntest.main.file_write(other_mu_path, "new text\n")
  expected_output = svntest.wc.State(\
    wc, { 'A/mu' : Item(verb='Sending'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('A/mu', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update "A" by name in wc_empty, expect to receive the change to A/mu.
  expected_output = svntest.wc.State(wc_empty, { 'A/mu' : Item(status='U ') })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')
  expected_disk.tweak('A/mu', contents='new text\n')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 2)
  expected_status.remove('iota')
  expected_status.tweak('', wc_rev=1)
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        A_path)

  # Commit the deletion of A/mu from the "other" wc.
  svntest.main.file_write(other_mu_path, "new text\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', other_mu_path)
  expected_output = svntest.wc.State(wc, { 'A/mu' : Item(verb='Deleting'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.remove('A/mu')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)


  # Update "A" by name in wc_empty, expect to A/mu to disappear.
  expected_output = svntest.wc.State(wc_empty, { 'A/mu' : Item(status='D ') })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')
  expected_disk.remove('A/mu')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 3)
  expected_status.remove('iota')
  expected_status.remove('A/mu')
  expected_status.tweak('', wc_rev=1)
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        A_path)



#----------------------------------------------------------------------
def depth_immediates_bring_in_file(sbox):
  "bring a file into a depth-immediates working copy"

  # Create an immediates working copy and form the paths
  ign_a, ign_b, wc_imm, wc = set_up_depthy_working_copies(sbox,
                                                          immediates=True)
  A_mu_path = os.path.join(wc_imm, 'A', 'mu')
  gamma_path = os.path.join(wc_imm, 'A', 'D', 'gamma')

  # Run 'svn up A/mu' to bring A/mu permanently into the working copy.
  expected_output = svntest.wc.State(wc_imm, {
    'A/mu'           : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/C', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/F', 'A/B', 'A/D/gamma', 'A/D/G',
                       'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau', 'A/D/H/chi',
                       'A/D/H/psi', 'A/D/H/omega', 'A/D/H', 'A/D')
  expected_status = svntest.actions.get_virginal_state(wc_imm, 1)
  expected_status.remove('A/C', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/F', 'A/B', 'A/D/gamma', 'A/D/G',
                       'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau', 'A/D/H/chi',
                       'A/D/H/psi', 'A/D/H/omega', 'A/D/H', 'A/D')
  svntest.actions.run_and_verify_update(wc_imm,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, None,
                                        A_mu_path)

  # Run 'svn up A/D/gamma' to test the edge case 'Skipped'.
  expected_output = svntest.wc.State(wc_imm, {
    'A/D/gamma'   : Item(verb='Skipped'),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/C', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/F', 'A/B', 'A/D/gamma', 'A/D/G',
                       'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau', 'A/D/H/chi',
                       'A/D/H/psi', 'A/D/H/omega', 'A/D/H', 'A/D')
  expected_status = svntest.actions.get_virginal_state(wc_imm, 1)
  expected_status.remove('A/C', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/F', 'A/B', 'A/D/gamma', 'A/D/G',
                       'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau', 'A/D/H/chi',
                       'A/D/H/psi', 'A/D/H/omega', 'A/D/H', 'A/D')
  svntest.actions.run_and_verify_update(wc_imm,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, None,
                                        gamma_path)

#----------------------------------------------------------------------
def depth_immediates_fill_in_dir(sbox):
  "bring a dir into a depth-immediates working copy"

  # Run 'svn up A --set-depth=infinity' to fill in A as a
  # depth-infinity subdir.
  ign_a, ign_b, wc_immediates, wc \
                        = set_up_depthy_working_copies(sbox, immediates=True)
  A_path = os.path.join(wc_immediates, 'A')
  expected_output = svntest.wc.State(wc_immediates, {
    'A/mu'           : Item(status='A '),
    'A/B'            : Item(status='A '),
    'A/B/lambda'     : Item(status='A '),
    'A/B/E'          : Item(status='A '),
    'A/B/E/alpha'    : Item(status='A '),
    'A/B/E/beta'     : Item(status='A '),
    'A/B/F'          : Item(status='A '),
    'A/C'            : Item(status='A '),
    'A/D'            : Item(status='A '),
    'A/D/gamma'      : Item(status='A '),
    'A/D/G'          : Item(status='A '),
    'A/D/G/pi'       : Item(status='A '),
    'A/D/G/rho'      : Item(status='A '),
    'A/D/G/tau'      : Item(status='A '),
    'A/D/H'          : Item(status='A '),
    'A/D/H/chi'      : Item(status='A '),
    'A/D/H/psi'      : Item(status='A '),
    'A/D/H/omega'    : Item(status='A ')
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_immediates, 1)
  svntest.actions.run_and_verify_update(wc_immediates,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'infinity',
                                        A_path)

#----------------------------------------------------------------------
def depth_mixed_bring_in_dir(sbox):
  "bring a dir into a mixed-depth working copy"

  # Run 'svn up --set-depth=immediates A' in a depth-empty working copy.
  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True)
  A_path = os.path.join(wc_empty, 'A')
  B_path = os.path.join(wc_empty, 'A', 'B')
  C_path = os.path.join(wc_empty, 'A', 'C')

  expected_output = svntest.wc.State(wc_empty, {
    'A'              : Item(status='A '),
    'A/mu'           : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota', 'A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/F', 'A/C', 'A/D', 'A/D/gamma',
                       'A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                       'A/D/H', 'A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 1)
  expected_status.remove('iota', 'A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                         'A/B/E/beta', 'A/B/F', 'A/C', 'A/D', 'A/D/gamma',
                         'A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                         'A/D/H', 'A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega')
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files',
                                        A_path)
  # Check that A was added at depth=files.
  verify_depth(None, "files", A_path)

  # Now, bring in A/B at depth-immediates.
  expected_output = svntest.wc.State(wc_empty, {
    'A/B'            : Item(status='A '),
    'A/B/lambda'     : Item(status='A '),
    'A/B/E'          : Item(status='A '),
    'A/B/F'          : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota', 'A/B/E/alpha', 'A/B/E/beta', 'A/C',
                       'A/D', 'A/D/gamma', 'A/D/G', 'A/D/G/pi', 'A/D/G/rho',
                       'A/D/G/tau', 'A/D/H', 'A/D/H/chi', 'A/D/H/psi',
                       'A/D/H/omega')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 1)
  expected_status.remove('iota', 'A/B/E/alpha', 'A/B/E/beta', 'A/C',
                         'A/D', 'A/D/gamma', 'A/D/G', 'A/D/G/pi', 'A/D/G/rho',
                         'A/D/G/tau', 'A/D/H', 'A/D/H/chi', 'A/D/H/psi',
                         'A/D/H/omega')
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates',
                                        B_path)
  # Check that A/B was added at depth=immediates.
  verify_depth(None, "immediates", B_path)

  # Now, bring in A/C at depth-empty.
  expected_output = svntest.wc.State(wc_empty, {
    'A/C'            : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota', 'A/B/E/alpha', 'A/B/E/beta',
                       'A/D', 'A/D/gamma', 'A/D/G', 'A/D/G/pi', 'A/D/G/rho',
                       'A/D/G/tau', 'A/D/H', 'A/D/H/chi', 'A/D/H/psi',
                       'A/D/H/omega')
  expected_status = svntest.actions.get_virginal_state(wc_empty, 1)
  expected_status.remove('iota', 'A/B/E/alpha', 'A/B/E/beta',
                         'A/D', 'A/D/gamma', 'A/D/G', 'A/D/G/pi', 'A/D/G/rho',
                         'A/D/G/tau', 'A/D/H', 'A/D/H/chi', 'A/D/H/psi',
                         'A/D/H/omega')
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty',
                                        C_path)
  # Check that A/C was added at depth=empty.
  verify_depth(None, "empty", C_path)

#----------------------------------------------------------------------
def depth_empty_unreceive_delete(sbox):
  "depth-empty working copy ignores a deletion"
  # Check out a depth-empty greek tree to wc1.  In wc2, delete iota and
  # commit.  Update wc1; should not receive the delete.
  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  iota_path = os.path.join(wc, 'iota')

  # Commit in the "other" wc.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', iota_path)
  expected_output = svntest.wc.State(wc, { 'iota' : Item(verb='Deleting'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.remove('iota')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-empty wc, expecting not to receive the deletion of iota.
  expected_output = svntest.wc.State(wc_empty, { })
  expected_disk = svntest.wc.State('', { })
  expected_status = svntest.wc.State(wc_empty, { '' : svntest.wc.StateItem() })
  expected_status.tweak(contents=None, status='  ', wc_rev=2)
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)


#----------------------------------------------------------------------
def depth_immediates_unreceive_delete(sbox):
  "depth-immediates working copy ignores a deletion"
  # Check out a depth-immediates greek tree to wc1.  In wc2, delete
  # A/mu and commit.  Update wc1; should not receive the delete.

  ign_a, ign_b, wc_immed, wc = set_up_depthy_working_copies(sbox,
                                                            immediates=True,
                                                            infinity=True)

  mu_path = os.path.join(wc, 'A', 'mu')

  # Commit in the "other" wc.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', mu_path)
  expected_output = svntest.wc.State(wc, { 'A/mu' : Item(verb='Deleting'), })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.remove('A/mu')
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-immediates wc, expecting not to receive the deletion
  # of A/mu.
  expected_output = svntest.wc.State(wc_immed, { })
  expected_disk = svntest.wc.State('', {
    'iota' : Item(contents="This is the file 'iota'.\n"),
    'A' : Item()
    })
  expected_status = svntest.wc.State(wc_immed, {
    '' : Item(status='  ', wc_rev=2),
    'iota' : Item(status='  ', wc_rev=2),
    'A' : Item(status='  ', wc_rev=2)
    })
  svntest.actions.run_and_verify_update(wc_immed,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)

#----------------------------------------------------------------------
def depth_immediates_receive_delete(sbox):
  "depth-immediates working copy receives a deletion"
  # Check out a depth-immediates greek tree to wc1.  In wc2, delete A and
  # commit.  Update wc1  should receive the delete.

  ign_a, ign_b, wc_immed, wc = set_up_depthy_working_copies(sbox,
                                                            immediates=True,
                                                            infinity=True)

  A_path = os.path.join(wc, 'A')

  # Commit in the "other" wc.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', A_path)
  expected_output = svntest.wc.State(wc, { 'A' : Item(verb='Deleting'), })
  expected_status = svntest.wc.State(wc, {
    '' : Item(status='  ', wc_rev=1),
    'iota' : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-immediates wc, expecting to receive the deletion of A.
  expected_output = svntest.wc.State(wc_immed, {
    'A'    : Item(status='D ')
    })
  expected_disk = svntest.wc.State('', {
    'iota' : Item(contents="This is the file 'iota'.\n"),
    })
  expected_status = svntest.wc.State(wc_immed, {
    ''     : Item(status='  ', wc_rev=2),
    'iota' : Item(status='  ', wc_rev=2)
    })
  svntest.actions.run_and_verify_update(wc_immed,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)

#----------------------------------------------------------------------
def depth_immediates_subdir_propset_1(sbox):
  "depth-immediates commit subdir propset, update"
  ign_a, ign_b, wc_immediates, ign_c \
         = set_up_depthy_working_copies(sbox, immediates=True)

  A_path = os.path.join(wc_immediates, 'A')

  # Set a property on an immediate subdirectory of the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'pset', 'foo', 'bar',
                                     A_path)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_immediates, {
    'A' : Item(verb='Sending'),
    })

  # Create expected status tree.
  expected_status = svntest.wc.State(wc_immediates, {
    '' : Item(status='  ', wc_rev=1),
    'iota' : Item(status='  ', wc_rev=1),
    'A' : Item(status='  ', wc_rev=2)
    })

  # Commit wc_immediates/A.
  svntest.actions.run_and_verify_commit(wc_immediates,
                                        expected_output,
                                        expected_status,
                                        None,
                                        A_path)

  # Create expected output tree for the update.
  expected_output = svntest.wc.State(wc_immediates, { })

  # Create expected disk tree.
  expected_disk = svntest.wc.State('', {
    'iota' : Item(contents="This is the file 'iota'.\n"),
    'A' : Item(contents=None, props={'foo' : 'bar'}),
    })

  expected_status.tweak(contents=None, status='  ', wc_rev=2)

  # Update the depth-immediates wc.
  svntest.actions.run_and_verify_update(wc_immediates,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1)

#----------------------------------------------------------------------
def depth_immediates_subdir_propset_2(sbox):
  "depth-immediates update receives subdir propset"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Make the other working copy.
  other_wc = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, other_wc)

  A_path = os.path.join(wc_dir, 'A')

  # Set a property on an immediate subdirectory of the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'pset', 'foo', 'bar',
                                     A_path)
  # Commit.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', 'logmsg', A_path)

  # Update at depth=immediates in the other wc, expecting to see no errors.
  svntest.actions.run_and_verify_svn("Output on stderr where none expected",
                                     svntest.verify.AnyOutput, [],
                                     'update', '--depth', 'immediates',
                                     other_wc)

#----------------------------------------------------------------------
def depth_update_to_more_depth(sbox):
  "gradually update an empty wc to depth=infinity"

  wc_dir, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox, empty=True)

  os.chdir(wc_dir)

  # Run 'svn up --set-depth=files' in a depth-empty working copy.
  expected_output = svntest.wc.State('', {
    'iota'              : Item(status='A '),
    })
  expected_status = svntest.wc.State('', {
    '' : Item(status='  ', wc_rev=1),
    'iota' : Item(status='  ', wc_rev=1),
    })
  expected_disk = svntest.wc.State('', {
    'iota' : Item("This is the file 'iota'.\n"),
    })
  svntest.actions.run_and_verify_update('',
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files')
  verify_depth(None, "files")

  # Run 'svn up --set-depth=immediates' in the now depth-files working copy.
  expected_output = svntest.wc.State('', {
    'A'              : Item(status='A '),
    })
  expected_status = svntest.wc.State('', {
    '' : Item(status='  ', wc_rev=1),
    'iota' : Item(status='  ', wc_rev=1),
    'A' : Item(status='  ', wc_rev=1),
    })
  expected_disk = svntest.wc.State('', {
    'iota' : Item("This is the file 'iota'.\n"),
    'A'    : Item(),
    })
  svntest.actions.run_and_verify_update('',
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates')
  verify_depth(None, "immediates")
  verify_depth(None, "empty", "A")

  # Upgrade 'A' to depth-files.
  expected_output = svntest.wc.State('', {
    'A/mu'           : Item(status='A '),
    })
  expected_status = svntest.wc.State('', {
    '' : Item(status='  ', wc_rev=1),
    'iota' : Item(status='  ', wc_rev=1),
    'A' : Item(status='  ', wc_rev=1),
    'A/mu' : Item(status='  ', wc_rev=1),
    })
  expected_disk = svntest.wc.State('', {
    'iota' : Item("This is the file 'iota'.\n"),
    'A'    : Item(),
    'A/mu' : Item("This is the file 'mu'.\n"),
    })
  svntest.actions.run_and_verify_update('',
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', 'A')
  verify_depth(None, "immediates")
  verify_depth(None, "files", "A")

  # Run 'svn up --set-depth=infinity' in the working copy.
  expected_output = svntest.wc.State('', {
    'A/B'            : Item(status='A '),
    'A/B/lambda'     : Item(status='A '),
    'A/B/E'          : Item(status='A '),
    'A/B/E/alpha'    : Item(status='A '),
    'A/B/E/beta'     : Item(status='A '),
    'A/B/F'          : Item(status='A '),
    'A/C'            : Item(status='A '),
    'A/D'            : Item(status='A '),
    'A/D/gamma'      : Item(status='A '),
    'A/D/G'          : Item(status='A '),
    'A/D/G/pi'       : Item(status='A '),
    'A/D/G/rho'      : Item(status='A '),
    'A/D/G/tau'      : Item(status='A '),
    'A/D/H'          : Item(status='A '),
    'A/D/H/chi'      : Item(status='A '),
    'A/D/H/psi'      : Item(status='A '),
    'A/D/H/omega'    : Item(status='A ')
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state('', 1)
  svntest.actions.run_and_verify_update('',
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'infinity')
  verify_depth("Non-infinity depth detected after an upgrade to depth-infinity",
               "infinity")
  verify_depth("Non-infinity depth detected after an upgrade to depth-infinity",
               "infinity", "A")

def commit_propmods_with_depth_empty_helper(sbox, depth_arg):
  """Helper for commit_propmods_with_depth_empty().
  DEPTH_ARG should be either '--depth=empty' or '-N'."""

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  A_path = os.path.join(wc_dir, 'A')
  D_path = os.path.join(A_path, 'D')
  gamma_path = os.path.join(D_path, 'gamma')
  G_path = os.path.join(D_path, 'G')
  pi_path = os.path.join(G_path, 'pi')
  H_path = os.path.join(D_path, 'H')
  chi_path = os.path.join(H_path, 'chi')

  # Set some properties, modify some files.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'bar', 'bar-val', D_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'baz', 'baz-val', G_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'qux', 'qux-val', H_path)
  svntest.main.file_append(iota_path, "new iota\n")
  svntest.main.file_append(gamma_path, "new gamma\n")
  svntest.main.file_append(pi_path, "new pi\n")
  svntest.main.file_append(chi_path, "new chi\n")

  # The only things that should be committed are two of the propsets.
  expected_output = svntest.wc.State(
    wc_dir,
    { ''    : Item(verb='Sending'),
      'A/D' : Item(verb='Sending'), }
    )
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  # Expect the two propsets to be committed:
  expected_status.tweak('', status='  ', wc_rev=2)
  expected_status.tweak('A/D', status='  ', wc_rev=2)
  # Expect every other change to remain uncommitted:
  expected_status.tweak('iota', status='M ', wc_rev=1)
  expected_status.tweak('A/D/G', status=' M', wc_rev=1)
  expected_status.tweak('A/D/H', status=' M', wc_rev=1)
  expected_status.tweak('A/D/gamma', status='M ', wc_rev=1)
  expected_status.tweak('A/D/G/pi', status='M ', wc_rev=1)
  expected_status.tweak('A/D/H/chi', status='M ', wc_rev=1)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        depth_arg,
                                        wc_dir, D_path)

# See also commit_tests 26: commit_nonrecursive
def commit_propmods_with_depth_empty(sbox):
  "commit property mods only, using --depth=empty"

  sbox2 = sbox.clone_dependent()

  # Run once with '-N' and once with '--depth=empty' to make sure they
  # function identically.
  commit_propmods_with_depth_empty_helper(sbox, '-N')
  commit_propmods_with_depth_empty_helper(sbox2, '--depth=empty')

# Test for issue #2845.
@Issue(2845)
def diff_in_depthy_wc(sbox):
  "diff at various depths in non-infinity wc"

  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  iota_path = os.path.join(wc, 'iota')
  A_path = os.path.join(wc, 'A')
  mu_path = os.path.join(wc, 'A', 'mu')
  gamma_path = os.path.join(wc, 'A', 'D', 'gamma')

  # Make some changes in the depth-infinity wc, and commit them
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', wc)
  svntest.main.file_write(iota_path, "new text\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'bar', 'bar-val', A_path)
  svntest.main.file_write(mu_path, "new text\n")
  svntest.main.file_write(gamma_path, "new text\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', '', wc)

  diff = [
    "Index: A/mu\n",
    "===================================================================\n",
    "--- A/mu\t(revision 2)\n",
    "+++ A/mu\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-new text\n",
    "+This is the file 'mu'.\n",
    "Index: A\n",
    "===================================================================\n",
    "--- A\t(revision 2)\n",
    "+++ A\t(working copy)\n",
    "\n",
    "Property changes on: A\n",
    "___________________________________________________________________\n",
    "Deleted: bar\n",
    "## -1 +0,0 ##\n",
    "-bar-val\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 2)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-new text\n",
    "+This is the file 'iota'.\n",
    "Index: .\n",
    "===================================================================\n",
    "--- .\t(revision 2)\n",
    "+++ .\t(working copy)\n",
    "\n",
    "Property changes on: .\n",
    "___________________________________________________________________\n",
    "Deleted: foo\n",
    "## -1 +0,0 ##\n",
    "-foo-val\n",
    "\\ No newline at end of property\n"]

  os.chdir(wc_empty)

  expected_output = svntest.verify.UnorderedOutput(diff[24:])
  # The diff should contain only the propchange on '.'
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-rHEAD')

  # Upgrade to depth-files.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'files', '-r1')
  # The diff should contain only the propchange on '.' and the
  # contents change on iota.
  expected_output = svntest.verify.UnorderedOutput(diff[17:])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-rHEAD')
  # Do a diff at --depth empty.
  expected_output = svntest.verify.UnorderedOutput(diff[24:])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '--depth', 'empty', '-rHEAD')

  # Upgrade to depth-immediates.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'immediates', '-r1')
  # The diff should contain the propchanges on '.' and 'A' and the
  # contents change on iota.
  expected_output = svntest.verify.UnorderedOutput(diff[7:])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                    'diff', '-rHEAD')
  # Do a diff at --depth files.
  expected_output = svntest.verify.UnorderedOutput(diff[17:])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '--depth', 'files', '-rHEAD')

  # Upgrade A to depth-files.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'files', '-r1', 'A')
  # The diff should contain everything but the contents change on
  # gamma (which does not exist in this working copy).
  expected_output = svntest.verify.UnorderedOutput(diff)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-rHEAD')
  # Do a diff at --depth immediates.
  expected_output = svntest.verify.UnorderedOutput(diff[7:])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                    'diff', '--depth', 'immediates', '-rHEAD')

@Issue(2882)
def commit_depth_immediates(sbox):
  "commit some files with --depth=immediates"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Test the fix for some bugs Mike Pilato reported here:
  #
  #    http://subversion.tigris.org/servlets/ReadMsg?list=dev&msgNo=128509
  #    From: "C. Michael Pilato" <cmpilato@collab.net>
  #    To: Karl Fogel <kfogel@red-bean.com>
  #    CC: dev@subversion.tigris.org
  #    References: <87d4yzcrro.fsf@red-bean.com>
  #    Subject: Re: [PATCH] Make 'svn commit --depth=foo' work.
  #    Message-ID: <46968831.2070906@collab.net>
  #    Date: Thu, 12 Jul 2007 15:59:45 -0400
  #
  # See also http://subversion.tigris.org/issues/show_bug.cgi?id=2882.
  #
  # Outline of the test:
  # ====================
  #
  # Modify these three files:
  #
  #    M      A/mu
  #    M      A/D/G/rho
  #    M      iota
  #
  # Then commit some of them using --depth=immediates:
  #
  #    svn ci -m "log msg" --depth=immediates wc_dir wc_dir/A/D/G/rho
  #
  # Before the bugfix, that would result in an error:
  #
  #    subversion/libsvn_wc/lock.c:570: (apr_err=155004)
  #    svn: Working copy '/blah/blah/blah/wc' locked
  #    svn: run 'svn cleanup' to remove locks \
  #         (type 'svn help cleanup' for details)
  #
  # After the bugfix, it correctly commits two of the three files:
  #
  #    Sending        A/D/G/rho
  #    Sending        iota
  #    Transmitting file data ..
  #    Committed revision 2.

  iota_path = os.path.join(wc_dir, 'iota')
  mu_path   = os.path.join(wc_dir, 'A', 'mu')
  G_path    = os.path.join(wc_dir, 'A', 'D', 'G')
  rho_path  = os.path.join(G_path, 'rho')

  svntest.main.file_append(iota_path, "new text in iota\n")
  svntest.main.file_append(mu_path,   "new text in mu\n")
  svntest.main.file_append(rho_path,  "new text in rho\n")

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota',       status='  ',  wc_rev=2)
  expected_status.tweak('A/mu',       status='M ',  wc_rev=1)
  expected_status.tweak('A/D/G/rho',  status='  ',  wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        '--depth', 'immediates',
                                        wc_dir, G_path)

def depth_immediates_receive_new_dir(sbox):
  "depth-immediates wc receives new directory"

  ign_a, ign_b, wc_immed, wc = set_up_depthy_working_copies(sbox,
                                                            immediates=True,
                                                            infinity=True)

  I_path = os.path.join(wc, 'I')
  zeta_path = os.path.join(wc, 'I', 'zeta')
  other_I_path = os.path.join(wc_immed, 'I')

  os.mkdir(I_path)
  svntest.main.file_write(zeta_path, "This is the file 'zeta'.\n")

  # Commit in the "other" wc.
  svntest.actions.run_and_verify_svn(None, None, [], 'add', I_path)
  expected_output = svntest.wc.State(wc, {
    'I'      : Item(verb='Adding'),
    'I/zeta' : Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.add({
    'I'      : Item(status='  ', wc_rev=2),
    'I/zeta' : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None, wc)

  # Update the depth-immediates wc, expecting to receive just the
  # new directory, without the file.
  expected_output = svntest.wc.State(wc_immed, {
    'I'    : Item(status='A '),
    })
  expected_disk = svntest.wc.State('', {
    'iota' : Item(contents="This is the file 'iota'.\n"),
    'A'    : Item(),
    'I'    : Item(),
    })
  expected_status = svntest.wc.State(wc_immed, {
    ''     : Item(status='  ', wc_rev=2),
    'iota' : Item(status='  ', wc_rev=2),
    'A'    : Item(status='  ', wc_rev=2),
    'I'    : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_update(wc_immed,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None)
  # Check that the new directory was added at depth=empty.
  verify_depth(None, "empty", other_I_path)

@Issue(2931)
def add_tree_with_depth(sbox):
  "add multi-subdir tree with --depth options"  # For issue #2931
  sbox.build()
  wc_dir = sbox.wc_dir
  new1_path = os.path.join(wc_dir, 'new1')
  new2_path = os.path.join(new1_path, 'new2')
  new3_path = os.path.join(new2_path, 'new3')
  new4_path = os.path.join(new3_path, 'new4')
  os.mkdir(new1_path)
  os.mkdir(new2_path)
  os.mkdir(new3_path)
  os.mkdir(new4_path)
  # Simple case, add new1 only, set depth to files
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "add", "--depth", "files", new1_path)
  verify_depth(None, "infinity", new1_path)

  # Force add new1 at new1 again, should include new2 at empty, the depth of
  # new1 should not change
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "add", "--depth", "immediates",
                                     "--force", new1_path)
  verify_depth(None, "infinity", new1_path)
  verify_depth(None, "infinity", new2_path)

  # add new4 with intermediate path, the intermediate path is added at empty
  svntest.actions.run_and_verify_svn(None, None, [],
                                     "add", "--depth", "immediates",
                                     "--parents", new4_path)
  verify_depth(None, "infinity", new3_path)
  verify_depth(None, "infinity", new4_path)

def upgrade_from_above(sbox):
  "upgrade a depth=empty wc from above"

  # The bug was that 'svn up --set-depth=files' worked from within the
  # working copy, but not from without with working copy top given
  # as an argument.  Both ways would correctly cause 'iota' to
  # appear, but only the former actually upgraded the depth of the
  # working copy to 'files'.  See this thread for details:
  #
  #   http://subversion.tigris.org/servlets/ReadMsg?list=dev&msgNo=130157
  #   From: Alexander Sinyushkin <Alexander.Sinyushkin@svnkit.com>
  #   To: dev@subversion.tigris.org
  #   Subject: Problem upgrading working copy depth
  #   Date: Wed, 19 Sep 2007 23:15:24 +0700
  #   Message-ID: <46F14B1C.8010406@svnkit.com>

  sbox2 = sbox.clone_dependent()

  wc, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox, empty=True)

  # First verify that upgrading from within works.
  saved_cwd = os.getcwd()
  try:
    os.chdir(wc)
    expected_output = svntest.wc.State('', {
        'iota'    : Item(status='A '),
        })
    expected_disk = svntest.wc.State('', {
        'iota' : Item(contents="This is the file 'iota'.\n"),
        })
    expected_status = svntest.wc.State('', {
        ''     : Item(status='  ', wc_rev=1),
        'iota' : Item(status='  ', wc_rev=1),
        })
    svntest.actions.run_and_verify_update('',
                                          expected_output,
                                          expected_disk,
                                          expected_status,
                                          None, None, None, None, None, None,
                                          '--set-depth=files')
    verify_depth(None, "files")
  finally:
    os.chdir(saved_cwd)

  # Do it again, this time from above the working copy.
  wc, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox2, empty=True)
  expected_output = svntest.wc.State(wc, {
      'iota'    : Item(status='A '),
      })
  expected_disk = svntest.wc.State('', {
      'iota' : Item(contents="This is the file 'iota'.\n"),
      })
  expected_status = svntest.wc.State(wc, {
      ''     : Item(status='  ', wc_rev=1),
      'iota' : Item(status='  ', wc_rev=1),
      })
  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, None,
                                        '--set-depth=files', wc)
  verify_depth(None, "files", wc)

def status_in_depthy_wc(sbox):
  "status -u at various depths in non-infinity wc"

  wc_empty, ign_a, ign_b, wc = set_up_depthy_working_copies(sbox, empty=True,
                                                            infinity=True)

  iota_path = os.path.join(wc, 'iota')
  A_path = os.path.join(wc, 'A')
  mu_path = os.path.join(wc, 'A', 'mu')
  gamma_path = os.path.join(wc, 'A', 'D', 'gamma')

  # Make some changes in the depth-infinity wc, and commit them
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', wc)
  svntest.main.file_write(iota_path, "new text\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'bar', 'bar-val', A_path)
  svntest.main.file_write(mu_path, "new text\n")
  svntest.main.file_write(gamma_path, "new text\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', '', wc)

  status = [
    "Status against revision:      2\n",
    "        *        1   .\n",
    "        *        1   iota\n",
    "        *        1   A\n",
    "        *        1   " + os.path.join('A', 'mu') + "\n",
  ]

  os.chdir(wc_empty)

  expected_output = svntest.verify.UnorderedOutput(status[:2])
  # The output should contain only the change on '.'.
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '-u')

  # Upgrade to depth-files.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'files', '-r1')
  # The output should contain only the changes on '.' and 'iota'.
  expected_output = svntest.verify.UnorderedOutput(status[:3])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '-u')
  # Do a status -u at --depth empty.
  expected_output = svntest.verify.UnorderedOutput(status[:2])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '-u', '--depth', 'empty')

  # Upgrade to depth-immediates.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'immediates', '-r1')
  # The output should contain the changes on '.', 'A' and 'iota'.
  expected_output = svntest.verify.UnorderedOutput(status[:4])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                    'st', '-u')
  # Do a status -u at --depth files.
  expected_output = svntest.verify.UnorderedOutput(status[:3])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '-u', '--depth', 'files')

  # Upgrade A to depth-files.
  svntest.actions.run_and_verify_svn(None, None, [], 'up',
                                     '--set-depth', 'files', '-r1', 'A')
  # The output should contain everything but the change on
  # gamma (which does not exist in this working copy).
  expected_output = svntest.verify.UnorderedOutput(status)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '-u')
  # Do a status -u at --depth immediates.
  expected_output = svntest.verify.UnorderedOutput(status[:4])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                    'st', '-u', '--depth', 'immediates')

#----------------------------------------------------------------------

# Issue #3039.
@Issue(3039)
def depthy_update_above_dir_to_be_deleted(sbox):
  "'update -N' above a WC path deleted in repos HEAD"
  sbox.build()

  sbox_for_depth = {
    "files" : sbox,
    "immediates" : sbox.clone_dependent(copy_wc=True),
    "empty" : sbox.clone_dependent(copy_wc=True),
    }

  exit_code, output, err = svntest.actions.run_and_verify_svn(
    None, None, [],
    "delete", "-m", "Delete A.", sbox.repo_url + "/A")

  def empty_output(wc_dir):
    return svntest.wc.State(wc_dir, { })

  def output_with_A(wc_dir):
    expected_output = empty_output(wc_dir)
    expected_output.add({
      "A" : Item(status="D "),
      })
    return expected_output

  initial_disk = svntest.main.greek_state.copy()
  disk_with_only_iota = svntest.wc.State("", {
    "iota" : Item("This is the file 'iota'.\n"),
    })

  def status_with_dot(wc_dir):
    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.tweak("", wc_rev=2)
    return expected_status

  def status_with_iota(wc_dir):
    expected_status = status_with_dot(wc_dir)
    expected_status.tweak("iota", wc_rev=2)
    return expected_status

  def status_with_only_iota(wc_dir):
    return svntest.wc.State(wc_dir, {
      ""     : Item(status="  ", wc_rev=2),
      "iota" : Item(status="  ", wc_rev=2),
      })

  expected_trees_for_depth = {
    "files"      : (empty_output, initial_disk, status_with_iota),
    "immediates" : (output_with_A, disk_with_only_iota, status_with_only_iota),
    "empty"      : (empty_output, initial_disk, status_with_dot),
    }

  for depth in sbox_for_depth.keys():
    wc_dir = sbox_for_depth[depth].wc_dir
    (expected_output_func, expected_disk, expected_status_func) = \
      expected_trees_for_depth[depth]
    #print depth
    svntest.actions.run_and_verify_update(wc_dir,
                                          expected_output_func(wc_dir),
                                          expected_disk,
                                          expected_status_func(wc_dir),
                                          None, None, None, None, None,
                                          False,
                                          "--depth=%s" % depth, wc_dir)


#----------------------------------------------------------------------

# Tests for deselection interface (a.k.a folding subtrees).
#----------------------------------------------------------------------
def depth_folding_clean_trees_1(sbox):
  "gradually fold wc from depth=infinity to empty"

  # Covers the following situations:
  #
  #  infinity->immediates (metadata only)
  #  immediates->files (metadata only)
  #  mixed(infinity+files)=>immediates
  #  infinity=>empty
  #  immediates=>empty
  #  mixed(infinity+empty)=>immediates
  #  mixed(infinity+empty/immediates)=>immediates
  #  immediates=>files
  #  files=>empty
  #  mixed(infinity+empty)=>files

  ign_a, ign_b, ign_c, wc_dir = set_up_depthy_working_copies(sbox,
                                                             infinity=True)

  A_path = os.path.join(wc_dir, 'A')
  C_path = os.path.join(A_path, 'C')
  B_path = os.path.join(A_path, 'B')
  D_path = os.path.join(A_path, 'D')
  E_path = os.path.join(B_path, 'E')
  F_path = os.path.join(B_path, 'F')
  G_path = os.path.join(D_path, 'G')
  H_path = os.path.join(D_path, 'H')

  # Run 'svn up --set-depth=immediates' to directory A/B/E.
  # This is an infinity=>immediates folding, changes on metadata only
  expected_output = svntest.wc.State(wc_dir, {})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', E_path)
  verify_depth(None, "immediates", E_path)

  # Run 'svn up --set-depth=files' to directory A/B/E.
  # This is an immediates=>files folding, changes on metadata only
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', E_path)
  verify_depth(None, "files", E_path)

  # Run 'svn up --set-depth=immediates' to directory A/B.
  # This is an mixed(infinity+files)=>immediates folding
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha'    : Item(status='D '),
    'A/B/E/beta'     : Item(status='D '),
    })
  expected_status.remove('A/B/E/alpha', 'A/B/E/beta')
  expected_disk.remove('A/B/E/alpha', 'A/B/E/beta')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', B_path)
  verify_depth(None, "immediates", B_path)
  verify_depth(None, "empty", E_path)
  verify_depth(None, "empty", F_path)

  # Run 'svn up --set-depth=empty' to directory A/D/H
  # This is an infinity=>empty folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/chi'      : Item(status='D '),
    'A/D/H/psi'      : Item(status='D '),
    'A/D/H/omega'    : Item(status='D ')
    })
  expected_status.remove( 'A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega')
  expected_disk.remove( 'A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty', H_path)
  verify_depth(None, "empty", H_path)

  # Run 'svn up --set-depth=immediates' to directory A/D
  # This is an mixed(infinity+empty)=>immediates folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi'       : Item(status='D '),
    'A/D/G/rho'      : Item(status='D '),
    'A/D/G/tau'      : Item(status='D '),
    })
  expected_status.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_disk.remove('A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', D_path)
  verify_depth(None, "immediates", D_path)
  verify_depth(None, "empty", G_path)

  # Run 'svn up --set-depth=empty' to directory A/D
  # This is an immediates=>empty folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G'          : Item(status='D '),
    'A/D/H'          : Item(status='D '),
    'A/D/gamma'      : Item(status='D ')
    })
  expected_status.remove('A/D/gamma', 'A/D/G', 'A/D/H')
  expected_disk.remove('A/D/gamma', 'A/D/G', 'A/D/H')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty', D_path)
  verify_depth(None, "empty", D_path)

  # Run 'svn up --set-depth=immediates' to directory A
  # This is an mixed(infinity+empty/immediates)=>immediates folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'          : Item(status='D '),
    'A/B/F'          : Item(status='D '),
    'A/B/lambda'     : Item(status='D ')
    })
  expected_status.remove('A/B/lambda', 'A/B/E', 'A/B/F')
  expected_disk.remove('A/B/lambda', 'A/B/E', 'A/B/F')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', A_path)
  verify_depth(None, "immediates", A_path)
  verify_depth(None, "empty", C_path)
  verify_depth(None, "empty", B_path)

  # Run 'svn up --set-depth=files' to directory A
  # This is an immediates=>files folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'            : Item(status='D '),
    'A/C'            : Item(status='D '),
    'A/D'            : Item(status='D ')
    })
  expected_status.remove('A/B', 'A/C', 'A/D')
  expected_disk.remove('A/B', 'A/C', 'A/D')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', A_path)
  verify_depth(None, "files", A_path)

  # Run 'svn up --set-depth=empty' to directory A
  # This is an files=>empty folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'            : Item(status='D ')
    })
  expected_status.remove('A/mu')
  expected_disk.remove('A/mu')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty', A_path)
  verify_depth(None, "empty", A_path)

  # Run 'svn up --set-depth=files' to wc
  # This is an mixed(infinity+empty)=>files folding.
  expected_output = svntest.wc.State(wc_dir, {
    'A'            : Item(status='D ')
    })
  expected_status.remove('A')
  expected_disk.remove('A')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', wc_dir)
  verify_depth(None, "files", wc_dir)


#------------------------------------------------------------------------------
def depth_folding_clean_trees_2(sbox):
  "gradually fold wc, focusing on depth=immediates"

  # Covers the following situations:
  #
  #  infinity=>immediates
  #  mixed(immediates+immediates)=>immediates
  #  mixed(immediates+infinity)=>immediates
  #  mixed(immediates+files)=>immediates
  #  immediates=>empty(remove the target since the parent is at files/empty)

  ign_a, wc_dir, ign_b, ign_c = set_up_depthy_working_copies(sbox, files=True)

  A_path = os.path.join(wc_dir, 'A')
  D_path = os.path.join(A_path, 'D')
  H_path = os.path.join(D_path, 'H')
  G_path = os.path.join(D_path, 'G')

  # pull in directory A at immediates
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--depth', 'immediates', A_path)
  # check to see if it's really at immediates
  verify_depth(None, "immediates", A_path)

  # pull in directory D at infinity
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'infinity', D_path)

  # Run 'svn up --set-depth=immediates' to directory A/D.
  # This is an infinity=>immediates folding
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi'       : Item(status='D '),
    'A/D/G/rho'      : Item(status='D '),
    'A/D/G/tau'      : Item(status='D '),
    'A/D/H/chi'      : Item(status='D '),
    'A/D/H/psi'      : Item(status='D '),
    'A/D/H/omega'    : Item(status='D ')
    })
  expected_status = svntest.wc.State(wc_dir, {
    ''               : Item(status='  ', wc_rev=1),
    'iota'           : Item(status='  ', wc_rev=1),
    'A'              : Item(status='  ', wc_rev=1),
    'A/mu'           : Item(status='  ', wc_rev=1),
    'A/B'            : Item(status='  ', wc_rev=1),
    'A/C'            : Item(status='  ', wc_rev=1),
    'A/D'            : Item(status='  ', wc_rev=1),
    'A/D/gamma'      : Item(status='  ', wc_rev=1),
    'A/D/G'          : Item(status='  ', wc_rev=1),
    'A/D/H'          : Item(status='  ', wc_rev=1)
    })
  expected_disk = svntest.wc.State('', {
    'iota'        : Item(contents="This is the file 'iota'.\n"),
    'A'           : Item(contents=None),
    'A/mu'        : Item(contents="This is the file 'mu'.\n"),
    'A/B'         : Item(contents=None),
    'A/C'         : Item(contents=None),
    'A/D'         : Item(contents=None),
    'A/D/gamma'   : Item(contents="This is the file 'gamma'.\n"),
    'A/D/G'       : Item(contents=None),
    'A/D/H'       : Item(contents=None),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', D_path)
  verify_depth(None, "immediates", D_path)
  verify_depth(None, "empty", G_path)
  verify_depth(None, "empty", H_path)

  # Run 'svn up --set-depth=immediates' to directory A.
  # This is an mixed(immediates+immediates)=>immediates folding
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G'      : Item(status='D '),
    'A/D/H'      : Item(status='D '),
    'A/D/gamma'  : Item(status='D ')
    })
  expected_status.remove( 'A/D/G', 'A/D/H', 'A/D/gamma')
  expected_disk.remove( 'A/D/G', 'A/D/H', 'A/D/gamma')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', A_path)
  verify_depth(None, "immediates", A_path)
  verify_depth(None, "empty", D_path)

  # pull in directory D at infinity
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'infinity', D_path)

  # Run 'svn up --set-depth=immediates' to directory A.
  # This is an mixed(immediates+infinity)=>immediates folding
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma'      : Item(status='D '),
    'A/D/G'          : Item(status='D '),
    'A/D/H'          : Item(status='D '),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', A_path)
  verify_depth(None, "immediates", A_path)
  verify_depth(None, "empty", D_path)

  # pull in directory D at files
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'files', D_path)

  # Run 'svn up --set-depth=immediates' to directory A.
  # This is an mixed(immediates+files)=>immediates folding
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma'      : Item(status='D ')
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', A_path)
  verify_depth(None, "immediates", A_path)
  verify_depth(None, "empty", D_path)

#  Comment the following out, since cropping out the root of tree is now
#  handled by svn_depth_exclude and should have a separate test case for all
#  influenced commands.
#
#  # Run 'svn up --set-depth=empty' to directory A.
#  # This is an immediates=>empty folding, the directory A should be deleted
#  # too since the parent directory is at files/empty
#  expected_output = svntest.wc.State(wc_dir, {
#    'A'              : Item(status='D '),
#    })
#  expected_status = svntest.wc.State(wc_dir, {
#    ''               : Item(status='  ', wc_rev=1),
#    'iota'           : Item(status='  ', wc_rev=1)
#    })
#  expected_disk = svntest.wc.State('', {
#    'iota'        : Item(contents="This is the file 'iota'.\n")
#    })
#  svntest.actions.run_and_verify_update(wc_dir,
#                                        expected_output,
#                                        expected_disk,
#                                        expected_status,
#                                        None, None,
#                                        None, None, None, None,
#                                        '--set-depth', 'empty', A_path)

def depth_fold_expand_clean_trees(sbox):
  "expand target while contracting subtree"
  #  --set-depth=immediates/files to an empty target with infinity
  #  sub-tree should both fold the subtree and expand the target

  wc_dir, ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox, empty=True)

  A_path = os.path.join(wc_dir, 'A')
  B_path = os.path.join(A_path, 'B')
  C_path = os.path.join(A_path, 'C')
  D_path = os.path.join(A_path, 'D')

  # pull in directory A at empty
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--depth', 'empty', A_path)
  verify_depth(None, "empty", A_path)

  # pull in directory D at infinity
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', D_path)

  # Make the other working copy.
  other_wc = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, other_wc)

  # Run 'svn up --set-depth=immediates' to directory A. This both folds
  # directory D to empty and expands directory A to immediates
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'           : Item(status='A '),
    'A/B'            : Item(status='A '),
    'A/C'            : Item(status='A '),
    'A/D/gamma'      : Item(status='D '),
    'A/D/G'          : Item(status='D '),
    'A/D/H'          : Item(status='D '),
    })
  expected_status = svntest.wc.State(wc_dir, {
    ''               : Item(status='  ', wc_rev=1),
    'A'              : Item(status='  ', wc_rev=1),
    'A/mu'           : Item(status='  ', wc_rev=1),
    'A/B'            : Item(status='  ', wc_rev=1),
    'A/C'            : Item(status='  ', wc_rev=1),
    'A/D'            : Item(status='  ', wc_rev=1)
    })
  expected_disk = svntest.wc.State('', {
    'A'           : Item(contents=None),
    'A/mu'        : Item(contents="This is the file 'mu'.\n"),
    'A/B'         : Item(contents=None),
    'A/C'         : Item(contents=None),
    'A/D'         : Item(contents=None)
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', A_path)
  verify_depth(None, "immediates", A_path)
  verify_depth(None, "empty", B_path)
  verify_depth(None, "empty", C_path)
  verify_depth(None, "empty", D_path)

  # Run 'svn up --set-depth=files' to directory A in other_wc. This both
  # removes directory D and expands directory A to files
  expected_output = svntest.wc.State(other_wc, {
    'A/mu'           : Item(status='A '),
    'A/D'            : Item(status='D '),
    })
  expected_status = svntest.wc.State(other_wc, {
    ''               : Item(status='  ', wc_rev=1),
    'A'              : Item(status='  ', wc_rev=1),
    'A/mu'           : Item(status='  ', wc_rev=1),
    })
  expected_disk = svntest.wc.State('', {
    'A'           : Item(contents=None),
    'A/mu'        : Item(contents="This is the file 'mu'.\n")
    })
  Other_A_path = os.path.join(other_wc, 'A')
  svntest.actions.run_and_verify_update(other_wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', Other_A_path)
  verify_depth(None, "files", Other_A_path)


def pull_in_tree_with_depth_option(sbox):
  """checkout and verify subtree with depth immediates"""

  wc_empty,ign_a, ign_b, ign_c = set_up_depthy_working_copies(sbox,
                                                              empty=True)
  A_path = os.path.join(wc_empty, 'A')
  expected_output = svntest.wc.State(wc_empty, {
    'A'      : Item(status='A '),
    'A/mu'   : Item(status='A '),
    'A/B'    : Item(status='A '),
    'A/C'    : Item(status='A '),
    'A/D'    : Item(status='A ')
    })
  expected_disk = svntest.wc.State('', {
    'A'      : Item(),
    'A/mu'   : Item("This is the file 'mu'.\n"),
    'A/B'    : Item(),
    'A/C'    : Item(),
    'A/D'    : Item(),
    })
  expected_status = svntest.wc.State(wc_empty, {
    ''       : Item(status='  ', wc_rev=1),
    'A'      : Item(status='  ', wc_rev=1),
    'A/mu'   : Item(status='  ', wc_rev=1),
    'A/B'    : Item(status='  ', wc_rev=1),
    'A/C'    : Item(status='  ', wc_rev=1),
    'A/D'    : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_update(wc_empty,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        "--depth=immediates", A_path)

  # Check that the A directory was pull ed in at depth=immediates.
  verify_depth(None, "immediates", A_path)

def fold_tree_with_unversioned_modified_items(sbox):
  "unversioned & modified items left untouched"
  ign_a, ign_b, ign_c, wc_dir = set_up_depthy_working_copies(sbox,
                                                             infinity=True)

  A_path = os.path.join(wc_dir, 'A')
  pi_path = os.path.join(A_path, 'D', 'G', 'pi')
  mu_path = os.path.join(A_path, 'mu')
  unv_path = os.path.join(A_path, 'B', 'unv')

  # Modify file pi
  svntest.main.file_write(pi_path, "pi modified\n")
  # Modify file mu
  svntest.main.file_write(mu_path, "mu modified\n")
  # Create an unversioned file
  svntest.main.file_write(unv_path, "new unversioned\n")

  # Fold the A dir to empty, expect the modified & unversioned ones left
  # unversioned rather than removed, along with paths to those items.

  # Even though the directory B and D is not deleted because of local
  # modificatoin or unversioned items, there will be only one notification at
  # B and D.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'            : Item(status='D '),
    'A/C'            : Item(status='D '),
    'A/D'            : Item(status='D '),
    'A/mu'           : Item(status='D '),
    })
  # unversioned items will be ignored in in the status tree, since the
  # run_and_verify_update() function uses a quiet version of svn status
  # Dir A is still versioned, since the wc root is in depth-infinity
  expected_status = svntest.wc.State(wc_dir, {
    ''               : Item(status='  ', wc_rev=1),
    'iota'           : Item(status='  ', wc_rev=1),
    'A'              : Item(status='  ', wc_rev=1)
    })
  expected_disk = svntest.wc.State('', {
    'iota'           : Item(contents="This is the file 'iota'.\n"),
    'A'              : Item(contents=None),
    'A/mu'           : Item(contents="mu modified\n"),
    'A/B'            : Item(contents=None),
    'A/B/unv'        : Item(contents="new unversioned\n"),
    'A/D'            : Item(contents=None),
    'A/D/G'          : Item(contents=None),
    'A/D/G/pi'       : Item(contents="pi modified\n")
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty', A_path)
  verify_depth(None, "empty", A_path)

def depth_empty_update_on_file(sbox):
  "depth-empty update on a file doesn't break it"
  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')

  # Change iota and commit it in r2.
  svntest.main.file_write(iota_path, 'Modified iota\n')
  expected_output = svntest.wc.State(wc_dir, { 'iota' : Item(verb='Sending'), })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Update iota with depth=empty.
  expected_output = svntest.wc.State(wc_dir,
                                     {'iota': Item(status='U ') })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '--depth=empty', '-r1', iota_path)

  # Check the revision and created rev.
  expected_infos = {
      'Revision'           : '^1$',
      'Last Changed Rev'   : '^1$',
    }
  svntest.actions.run_and_verify_info([expected_infos], iota_path)


@Issue(3544)
def excluded_path_update_operation(sbox):
  """make sure update handle svn_depth_exclude properly"""

  ign_a, ign_b, ign_c, wc_dir = set_up_depthy_working_copies(sbox,
                                                             infinity=True)
  A_path = os.path.join(wc_dir, 'A')
  B_path = os.path.join(A_path, 'B')
  L_path = os.path.join(A_path, 'L')
  E_path = os.path.join(B_path, 'E')
  iota_path = os.path.join(wc_dir, 'iota')

  # Simply exclude a subtree
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'            : Item(status='D '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha', 'A/B/E/beta', 'A/B/E');
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/E/alpha', 'A/B/E/beta', 'A/B/E');

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'exclude', E_path)
  # verify_depth exclude? not implemented yet

  # crop path B to immediates, this just pull in A/B/E again
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'            : Item(status='A '),
    })
  expected_status.add({
    'A/B/E'            : Item(status='  ', wc_rev=1)
    })
  expected_disk.add({
    'A/B/E'          : Item(contents=None),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', B_path)
  verify_depth(None, "immediates", B_path)

  # Exclude A/B/E again
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', E_path)

  # Exclude path B totally, in which contains an excluded subtree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'            : Item(status='D '),
    })
  expected_status.remove('A/B/F', 'A/B/E', 'A/B/lambda', 'A/B');
  expected_disk.remove('A/B/F', 'A/B/E', 'A/B/lambda', 'A/B');
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'exclude', B_path)

  # Explicitly pull in excluded path B.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'            : Item(status='A '),
    'A/B/lambda'     : Item(status='A '),
    'A/B/E'          : Item(status='A '),
    'A/B/E/alpha'    : Item(status='A '),
    'A/B/E/beta'     : Item(status='A '),
    'A/B/F'          : Item(status='A '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        B_path)

  # Test issue #
  # Exclude a file then set depth of WC to infinity, the file should return.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='D '),
    })
  expected_status.remove('iota');
  expected_disk.remove('iota');
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'exclude', iota_path)

  # Update the whole WC to depth=infinity.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='A '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk = svntest.main.greek_state.copy()
  # This update currently fails when iota is reported as added, but shows in
  # status as unversioned.  See issue #3544 'svn update does not restore
  # excluded files'.  This test is marked as XFail until that issue is fixed.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'infinity', wc_dir)

def excluded_path_misc_operation(sbox):
  """make sure other subcommands handle exclude"""

  ign_a, ign_b, ign_c, wc_dir = set_up_depthy_working_copies(sbox,
                                                             infinity=True)
  A_path = os.path.join(wc_dir, 'A')
  B_path = os.path.join(A_path, 'B')
  L_path = os.path.join(A_path, 'L')
  M_path = os.path.join(A_path, 'M')
  E_path = os.path.join(B_path, 'E')
  LE_path = os.path.join(L_path, 'E')

  # Simply exclude a subtree
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'            : Item(status='D '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha', 'A/B/E/beta', 'A/B/E');
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/E/alpha', 'A/B/E/beta', 'A/B/E');

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'exclude', E_path)

  # copy A/B to A/L, excluded entry should be copied too
  expected_output = ['A         '+L_path+'\n']
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'cp', B_path, L_path)
  # verify_depth exclude? not implemented yet
  #verify_depth(None, "empty", LE_path)

  # revert A/L, with an excluded item in the tree
  revert_paths = [L_path] + [os.path.join(L_path, child)
                             for child in ['E', 'F', 'lambda']]
  expected_output = svntest.verify.UnorderedOutput([
    "Reverted '%s'\n" % path for path in revert_paths])

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '--depth=infinity', L_path)

  # copy A/B to A/L and then cp A/L to A/M, excluded entry should be
  # copied both times
  expected_output = ['A         '+L_path+'\n']
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'cp', B_path, L_path)
  expected_output = ['A         '+M_path+'\n']
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'cp', L_path, M_path)

  # commit this copy, with an excluded item.
  expected_output = svntest.wc.State(wc_dir, { 'A/L' : Item(verb='Adding'),
                                               'A/M' : Item(verb='Adding'), })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha', 'A/B/E/beta', 'A/B/E')
  expected_status.add({
    'A/L'        : Item(status='  ', wc_rev=2),
    'A/L/lambda' : Item(status='  ', wc_rev=2),
    'A/L/F'      : Item(status='  ', wc_rev=2),
    'A/M'        : Item(status='  ', wc_rev=2),
    'A/M/lambda' : Item(status='  ', wc_rev=2),
    'A/M/F'      : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Relocate wc, with excluded items in it.
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 2, 0)
  svntest.main.safe_rmtree(repo_dir, 1)
  svntest.actions.run_and_verify_svn(None, None, [], 'switch', '--relocate',
                                     repo_url, other_repo_url, wc_dir)

  # remove the new directory A/L, with an excluded item.
  # If successed, no error will be thrown
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', L_path)

  # revert the delete
  # If successed, no error will be thrown
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', '--depth=infinity', L_path)


def excluded_receive_remote_removal(sbox):
  """exclude flag should be cleared upon remote removal"""
  ign_a, ign_b, ign_c, wc \
         = set_up_depthy_working_copies(sbox, infinity=True)

  A_path = os.path.join(wc, 'A')
  B_path = os.path.join(A_path, 'B')
  C_path = os.path.join(A_path, 'C')

  # Exclude path B from wc
  expected_output = svntest.wc.State(wc, {
    'A/B'            : Item(status='D '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                       'A/B/E', 'A/B/F', 'A/B')
  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.remove('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                         'A/B/E', 'A/B/F', 'A/B')
  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        "--set-depth", "exclude", B_path)

  # Remove path B in the repos.
  svntest.actions.run_and_verify_svn(None, None, [], "delete", "-m",
                                     "Delete B.", sbox.repo_url + "/A/B")

  # Update wc, should receive the removal of excluded path B
  # and handle it silently.
  expected_status = svntest.actions.get_virginal_state(wc, 2)
  expected_status.remove('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                         'A/B/E', 'A/B/F', 'A/B')
  svntest.actions.run_and_verify_update(wc,
                                        None,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None)

  # Introduce a new path with the same name B.
  # This should succeed if the exclude entry is gone with the update,
  # otherwise a name conflict will rise up.
  expected_output = ['A         '+B_path+'\n']
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'cp', C_path, B_path)


# Regression test for r876760.
def exclude_keeps_hidden_entries(sbox):
  "'up --set-depth exclude' doesn't lose entries"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  os.chdir(A_path)

  # the second 'up' used to cause the entry of 'C' to be lost.
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude', 'C')
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude', 'D')
  # we could grep the 'entries' file, but...
  # or we could use 'info', but info_excluded() is XFail.
  expected_stderr = ".*svn: E150002: '.*C' is already under version control.*"
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'mkdir', 'C')


@Issue(3792)
def info_excluded(sbox):
  "'info' should treat excluded item as versioned"

  # The problem: 'svn info' on an excluded item would behave as if it
  # was not versioned at all:
  #
  #     % svn up --set-depth exclude A
  #     D         A
  #     % svn info A
  #     A:  (Not a versioned resource)
  #
  #     ..\..\..\subversion\svn\info-cmd.c:562: (apr_err=200000)
  #     svn: A problem occurred; see other errors for details
  #
  # It should acknowledge the existence (in the repos) of ./A and print some
  # info about it, like it does if '--set-depth empty' is used instead.

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude', A_path)

  expected_info = {
      'Path' : re.escape(A_path),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : svntest.actions.get_wc_uuid(wc_dir),
      'Depth' : 'exclude',
  }
  svntest.actions.run_and_verify_info([expected_info], A_path)



#----------------------------------------------------------------------
# Check that "svn resolved" visits tree-conflicts *on unversioned items*
# according to the --depth parameter.

def make_depth_tree_conflicts(sbox):
  "Helper for tree_conflicts_resolved_depth_*"

  sbox.build()
  wc = sbox.wc_dir

  j = os.path.join
  A = j(wc, 'A')
  m =    j(A, 'mu')
  B =    j(A, 'B')
  D =    j(A, 'D')
  g =      j(D, 'gamma')

  # Store node modifications as rev 2
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'foo-val', B)
  svntest.main.file_append(m, "Modified mu.\n")
  svntest.main.file_append(g, "Modified gamma.\n")

  expected_output = svntest.wc.State(wc, {
      'A/mu'              : Item(verb='Sending'),
      'A/B'               : Item(verb='Sending'),
      'A/D/gamma'         : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc, 1)
  expected_status.tweak('A/mu', 'A/B', 'A/D/gamma',
                        wc_rev = 2)

  svntest.actions.run_and_verify_commit(wc,
                                        expected_output,
                                        expected_status,
                                        None,
                                        A)

  # Go back to rev 1
  expected_output = svntest.wc.State(wc, {
    'A/mu'              : Item(status='U '),
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
                                        '-r1', A)

  # Perform node deletions so that items become unversioned and
  # will have tree-conflicts upon update.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', m, B, g)

  # Update so that conflicts appear
  expected_output = svntest.wc.State(wc, {
    'A/mu'              : Item(status='  ', treeconflict='C'),
    'A/B'               : Item(status='  ', treeconflict='C'),
    'A/D/gamma'         : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/mu',
                       'A/B', 'A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                       'A/D/gamma');
  if svntest.main.wc_is_singledb(sbox.wc_dir):
    expected_disk.remove('A/B/E', 'A/B/F')

  # This test is set XFail because this (correct) status cannot be
  # verified due to an "svn update" bug. The tree-conflict on A/B
  # which is notified about during the update does not show in the
  # status. When removing file 'mu' from above 'rm' command, 'B' is
  # reported as tree-conflicted correctly. Also use these to verify:
  #  expected_output = None
  #  expected_disk = None
  expected_status = svntest.actions.get_virginal_state(wc, 2)
  expected_status.tweak('A/mu',
                        'A/B', 'A/B/lambda',
                        'A/B/E', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/B/F',
                        'A/D/gamma',
                        status='D ')
  expected_status.tweak('A/mu', 'A/B', 'A/D/gamma',
                        treeconflict='C')

  svntest.actions.run_and_verify_update(wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        wc)



def tree_conflicts_resolved_depth_empty(sbox):
  "tree conflicts resolved depth-empty"

  make_depth_tree_conflicts(sbox)

  wc = sbox.wc_dir
  A = os.path.join(wc, 'A')

  svntest.actions.run_and_verify_resolved([], '--depth=empty', A)


def tree_conflicts_resolved_depth_files(sbox):
  "tree conflicts resolved depth-files"

  make_depth_tree_conflicts(sbox)

  wc = sbox.wc_dir
  j = os.path.join
  A = j(wc, 'A')
  m =    j(A, 'mu')

  svntest.actions.run_and_verify_resolved([m], '--depth=files', A)


def tree_conflicts_resolved_depth_immediates(sbox):
  "tree conflicts resolved depth-immediates"

  make_depth_tree_conflicts(sbox)

  wc = sbox.wc_dir
  j = os.path.join
  A = j(wc, 'A')
  m =    j(A, 'mu')
  B =    j(A, 'B')

  svntest.actions.run_and_verify_resolved([m, B], '--depth=immediates', A)


def tree_conflicts_resolved_depth_infinity(sbox):
  "tree conflicts resolved depth-infinity"

  make_depth_tree_conflicts(sbox)

  wc = sbox.wc_dir
  j = os.path.join
  A = j(wc, 'A')
  m =    j(A, 'mu')
  B =    j(A, 'B')
  g =    j(A, 'D', 'gamma')

  svntest.actions.run_and_verify_resolved([m, B, g], '--depth=infinity', A)

def update_excluded_path_sticky_depths(sbox):
  """set-depth from excluded to all other depths"""

  ign_a, ign_b, ign_c, wc_dir = set_up_depthy_working_copies(sbox,
                                                             infinity=True)
  A_path = os.path.join(wc_dir, 'A')
  B_path = os.path.join(A_path, 'B')

  # Exclude the subtree 'A/B'
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'            : Item(status='D '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/E',
                         'A/B/F', 'A/B')
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/E',
                       'A/B/F', 'A/B')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'exclude', B_path)

  # Update to depth 'empty' for the excluded path A/B
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'         : Item(status='A '),
    })
  expected_status.add({
    'A/B'         : Item(status='  ', wc_rev=1)
    })
  expected_disk.add({
    'A/B'         : Item(contents=None),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'empty', B_path)
  verify_depth(None, "empty", B_path)
  expected_info = {
      'Path' : re.escape(B_path),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : svntest.actions.get_wc_uuid(wc_dir),
      'Depth' : 'empty',
  }
  svntest.actions.run_and_verify_info([expected_info], B_path)

  # Exclude A/B again
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', B_path)

  # Update to depth 'files' for the excluded path A/B
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'              : Item(status='A '),
    'A/B/lambda'       : Item(status='A '),
    })
  expected_status.add({
    'A/B'              : Item(status='  ', wc_rev=1),
    'A/B/lambda'       : Item(status='  ', wc_rev=1),
    })
  expected_disk.add({
    'A/B'            : Item(contents=None),
    'A/B/lambda'     : Item(contents="This is the file 'lambda'.\n"),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'files', B_path)
  verify_depth(None, "files", B_path)
  expected_info = {
      'Path' : re.escape(B_path),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : svntest.actions.get_wc_uuid(wc_dir),
      'Depth' : 'files',
  }
  svntest.actions.run_and_verify_info([expected_info], B_path)

  # Exclude A/B again
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', B_path)

  # Update to depth 'immediates' for the excluded path A/B
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'              : Item(status='A '),
    'A/B/lambda'       : Item(status='A '),
    'A/B/E'            : Item(status='A '),
    'A/B/F'            : Item(status='A '),
    })
  expected_status.add({
    'A/B'              : Item(status='  ', wc_rev=1),
    'A/B/lambda'       : Item(status='  ', wc_rev=1),
    'A/B/E'            : Item(status='  ', wc_rev=1),
    'A/B/F'            : Item(status='  ', wc_rev=1),
    })
  expected_disk.add({
    'A/B'              : Item(contents=None),
    'A/B/lambda'       : Item(contents="This is the file 'lambda'.\n"),
    'A/B/E'            : Item(contents=None),
    'A/B/F'            : Item(contents=None),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'immediates', B_path)
  verify_depth(None, "immediates", B_path)
  expected_info = {
      'Path' : re.escape(B_path),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : svntest.actions.get_wc_uuid(wc_dir),
      'Depth' : 'immediates',
  }
  svntest.actions.run_and_verify_info([expected_info], B_path)

  # Exclude A/B again
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', B_path)

  # Update to depth 'infinity' for the excluded path A/B
  expected_output = svntest.wc.State(wc_dir, {
    'A/B'              : Item(status='A '),
    'A/B/lambda'       : Item(status='A '),
    'A/B/E'            : Item(status='A '),
    'A/B/E/beta'       : Item(status='A '),
    'A/B/E/alpha'      : Item(status='A '),
    'A/B/F'            : Item(status='A '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None,
                                        '--set-depth', 'infinity', B_path)
  verify_depth(None, "infinity", B_path)
  expected_info = {
      'Path' : re.escape(B_path),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : svntest.actions.get_wc_uuid(wc_dir),
  #   'Depth' value is absent for 'infinity'
  }
  svntest.actions.run_and_verify_info([expected_info], B_path)


def update_depth_empty_root_of_infinite_children(sbox):
  """update depth=empty root of depth=infinite children"""

  wc_dir, ign_a, ign_b, wc_other = set_up_depthy_working_copies(sbox,
                                                                empty=True,
                                                                infinity=True)
  A_path = os.path.join(wc_dir, 'A')

  # Update A to depth 'infinity'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'infinity', A_path)

  # Tweak some files in the full working copy and commit.
  svntest.main.file_append(os.path.join(wc_other, 'A', 'B', 'E', 'alpha'),
                           "Modified alpha.\n")
  svntest.main.file_append(os.path.join(wc_other, 'A', 'D', 'G', 'rho'),
                           "Modified rho.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '', wc_other)

  # Now update the original working copy and make sure we get those changes.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha'      : Item(status='U '),
    'A/D/G/rho'        : Item(status='U '),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('iota')
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')
  expected_disk.tweak('A/B/E/alpha', contents="This is the file 'alpha'.\nModified alpha.\n")
  expected_disk.tweak('A/D/G/rho', contents="This is the file 'rho'.\nModified rho.\n")
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None,
                                        None, None, None, None, wc_dir)

def sparse_update_with_dash_dash_parents(sbox):
  """update --parents"""

  sbox.build(create_wc = False)
  sbox.add_test_path(sbox.wc_dir, True)
  alpha_path = os.path.join(sbox.wc_dir, 'A', 'B', 'E', 'alpha')
  pi_path = os.path.join(sbox.wc_dir, 'A', 'D', 'G', 'pi')
  omega_path = os.path.join(sbox.wc_dir, 'A', 'D', 'H', 'omega')

  # Start with a depth=empty root checkout.
  svntest.actions.run_and_verify_svn(
      "Unexpected error from co --depth=empty",
      svntest.verify.AnyOutput, [],
      "co", "--depth", "empty", sbox.repo_url, sbox.wc_dir)

  # Now, let's use --parents to pull in some scattered file children.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A'            : Item(status='A '),
    'A/B'          : Item(status='A '),
    'A/B/E'        : Item(status='A '),
    'A/B/E/alpha'  : Item(status='A '),
    })
  expected_disk = svntest.wc.State('', {
    'A'            : Item(contents=None),
    'A/B'          : Item(contents=None),
    'A/B/E'        : Item(contents=None),
    'A/B/E/alpha'  : Item(contents="This is the file 'alpha'.\n"),
    })
  expected_status = svntest.wc.State(sbox.wc_dir, {
    ''             : Item(status='  ', wc_rev=1),
    'A'            : Item(status='  ', wc_rev=1),
    'A/B'          : Item(status='  ', wc_rev=1),
    'A/B/E'        : Item(status='  ', wc_rev=1),
    'A/B/E/alpha'  : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '--parents', alpha_path)

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/D'          : Item(status='A '),
    'A/D/G'        : Item(status='A '),
    'A/D/G/pi'     : Item(status='A '),
    })
  expected_disk.add({
    'A/D'          : Item(contents=None),
    'A/D/G'        : Item(contents=None),
    'A/D/G/pi'     : Item(contents="This is the file 'pi'.\n"),
    })
  expected_status.add({
    'A/D'          : Item(status='  ', wc_rev=1),
    'A/D/G'        : Item(status='  ', wc_rev=1),
    'A/D/G/pi'     : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '--parents', pi_path)

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/D/H'        : Item(status='A '),
    'A/D/H/omega'  : Item(status='A '),
    })
  expected_disk.add({
    'A/D/H'        : Item(contents=None),
    'A/D/H/omega'  : Item(contents="This is the file 'omega'.\n"),
    })
  expected_status.add({
    'A/D/H'        : Item(status='  ', wc_rev=1),
    'A/D/H/omega'  : Item(status='  ', wc_rev=1),
    })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        '--parents', omega_path)

def update_below_depth_empty(sbox):
  "update below depth empty shouldn't be applied"
  sbox.build()

  repo_url = sbox.repo_url
  A = sbox.ospath('A')

  expected_output = svntest.wc.State(sbox.wc_dir, {
      'A/C'               : Item(status='D '),
      'A/B'               : Item(status='D '),
      'A/mu'              : Item(status='D '),
      'A/D'               : Item(status='D '),
    })
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output, None,
                                        None, None, None, None, None, None,
                                        False,
                                        '--set-depth', 'empty', A)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', repo_url + '/iota',
                                           repo_url + '/A/B',
                                     '-m', 'remote copy')

  expected_output = svntest.wc.State(sbox.wc_dir, {
    })

  # This update should just update the revision of the working copy
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output, None,
                                        None, None)

# Test for issue #4136.
@Issue(4136)
def commit_then_immediates_update(sbox):
  "deep commit followed by update --depth immediates"
  sbox.build()

  repo_url = sbox.repo_url
  wc_dir = sbox.wc_dir
  mu_path = sbox.ospath('A/mu')

  # Modify A/mu and commit the changes.
  svntest.main.file_write(mu_path, "modified mu\n")
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'        : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Now, update --depth immediates in the root of the working copy.
  expected_output = svntest.wc.State(wc_dir, { })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents="modified mu\n")
  expected_status = svntest.wc.State(wc_dir, { '' : svntest.wc.StateItem() })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('',     wc_rev=2, status='  ')
  expected_status.tweak('A',    wc_rev=2, status='  ')
  expected_status.tweak('A/mu', wc_rev=2, status='  ')
  expected_status.tweak('iota', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, False,
                                        "--depth=immediates", wc_dir)

def revert_depth_files(sbox):
  "depth immediate+files should revert deleted files"

  sbox.build(read_only = True)
  
  expected_output = "Reverted '" + re.escape(sbox.ospath('A/mu')) + "'"
  
  # Apply an unrelated delete one level to deep
  sbox.simple_rm('A/D/gamma')

  sbox.simple_rm('A/mu')
  # Expect reversion of just 'mu'
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '--depth=immediates', sbox.ospath('A'))

  # Apply an unrelated directory delete
  sbox.simple_rm('A/D')

  sbox.simple_rm('A/mu')
  # Expect reversion of just 'mu'
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '--depth=files', sbox.ospath('A'))


#----------------------------------------------------------------------
# list all tests here, starting with None:
test_list = [ None,
              depth_empty_checkout,
              depth_files_checkout,
              nonrecursive_checkout,
              depth_empty_update_bypass_single_file,
              depth_immediates_get_top_file_mod_only,
              depth_empty_commit,
              depth_empty_with_file,
              depth_empty_with_dir,
              depth_immediates_bring_in_file,
              depth_immediates_fill_in_dir,
              depth_mixed_bring_in_dir,
              depth_empty_unreceive_delete,
              depth_immediates_unreceive_delete,
              depth_immediates_receive_delete,
              depth_update_to_more_depth,
              depth_immediates_subdir_propset_1,
              depth_immediates_subdir_propset_2,
              commit_propmods_with_depth_empty,
              diff_in_depthy_wc,
              commit_depth_immediates,
              depth_immediates_receive_new_dir,
              add_tree_with_depth,
              upgrade_from_above,
              status_in_depthy_wc,
              depthy_update_above_dir_to_be_deleted,
              depth_folding_clean_trees_1,
              depth_folding_clean_trees_2,
              depth_fold_expand_clean_trees,
              pull_in_tree_with_depth_option,
              fold_tree_with_unversioned_modified_items,
              depth_empty_update_on_file,
              excluded_path_update_operation,
              excluded_path_misc_operation,
              excluded_receive_remote_removal,
              exclude_keeps_hidden_entries,
              info_excluded,
              tree_conflicts_resolved_depth_empty,
              tree_conflicts_resolved_depth_files,
              tree_conflicts_resolved_depth_immediates,
              tree_conflicts_resolved_depth_infinity,
              update_excluded_path_sticky_depths,
              update_depth_empty_root_of_infinite_children,
              sparse_update_with_dash_dash_parents,
              update_below_depth_empty,
              commit_then_immediates_update,
              revert_depth_files,
              ]

if __name__ == "__main__":
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
