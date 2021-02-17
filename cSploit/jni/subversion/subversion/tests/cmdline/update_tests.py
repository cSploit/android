#!/usr/bin/env python
#
#  update_tests.py:  testing update cases.
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
import sys, re, os, subprocess
import time

# Our testing module
import svntest
from svntest import wc, actions, verify
from merge_tests import expected_merge_output
from merge_tests import set_up_branch

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem
exp_noop_up_out = svntest.actions.expected_noop_update_output

from svntest.main import SVN_PROP_MERGEINFO, server_has_mergeinfo

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

# Helper for update_binary_file() test -- a custom singleton handler.
def detect_extra_files(node, extra_files):
  """NODE has been discovered as an extra file on disk.  Verify that
  it matches one of the regular expressions in the EXTRA_FILES list of
  lists, and that its contents matches the second part of the list
  item.  If it matches, remove the match from the list.  If it doesn't
  match, raise an exception."""

  # Baton is of the form:
  #
  #       [ [wc_dir, pattern, contents],
  #         [wc_dir, pattern, contents], ... ]

  for fdata in extra_files:
    wc_dir = fdata[0]
    pattern = fdata[1]
    contents = None
    if len(fdata) > 2:
      contents = fdata[2]
    match_obj = re.match(pattern, node.name)
    if match_obj:
      if contents is None:
        return
      else:
        # Strip the root_node_name from node path
        # (svntest.tree.root_node_name, currently `__SVN_ROOT_NODE'),
        # since it doesn't really exist. Also strip the trailing "slash".
        real_path = node.path
        if real_path.startswith(svntest.tree.root_node_name):
          real_path = real_path[len(svntest.tree.root_node_name) +
                                len(os.sep) :]
        real_path = os.path.join(wc_dir, real_path)

        real_contents = open(real_path).read()
        if real_contents == contents:
          extra_files.pop(extra_files.index(fdata)) # delete pattern from list
          return

  print("Found unexpected object: %s" % node.name)
  raise svntest.tree.SVNTreeUnequal



def update_binary_file(sbox):
  "update a locally-modified binary file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a binary file to the project.
  theta_contents = open(os.path.join(sys.path[0], "theta.bin"), 'rb').read()
  # Write PNG file data into 'A/theta'.
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  svntest.main.file_write(theta_path, theta_contents, 'wb')

  svntest.main.run_svn(None, 'add', theta_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Adding  (bin)'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    })

  # Commit the new binary file, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Make a backup copy of the working copy.
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)
  theta_backup_path = os.path.join(wc_backup, 'A', 'theta')

  # Make a change to the binary file in the original working copy
  svntest.main.file_append(theta_path, "revision 3 text")
  theta_contents_r3 = theta_contents + "revision 3 text"

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Sending'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=3),
    })

  # Commit original working copy again, creating revision 3.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Now start working in the backup working copy:

  # Make a local mod to theta
  svntest.main.file_append(theta_backup_path, "extra theta text")
  theta_contents_local = theta_contents + "extra theta text"

  # Create expected output tree for an update of wc_backup.
  expected_output = svntest.wc.State(wc_backup, {
    'A/theta' : Item(status='C '),
    })

  # Create expected disk tree for the update --
  #    look!  binary contents, and a binary property!
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta' : Item(theta_contents_local,
                     props={'svn:mime-type' : 'application/octet-stream'}),
    })

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 3)
  expected_status.add({
    'A/theta' : Item(status='C ', wc_rev=3),
    })

  # Extra 'singleton' files we expect to exist after the update.
  # In the case, the locally-modified binary file should be backed up
  # to an .orig file.
  #  This is a list of lists, of the form [ WC_DIR,
  #                                         [pattern, contents], ...]
  extra_files = [[wc_backup, 'theta.*\.r2', theta_contents],
                 [wc_backup, 'theta.*\.r3', theta_contents_r3]]

  # Do the update and check the results in three ways.  Pass our
  # custom singleton handler to verify the .orig file; this handler
  # will verify the existence (and contents) of both binary files
  # after the update finishes.
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        detect_extra_files, extra_files,
                                        None, None, 1)

  # verify that the extra_files list is now empty.
  if len(extra_files) != 0:
    print("Not all extra reject files have been accounted for:")
    print(extra_files)
    raise svntest.Failure

#----------------------------------------------------------------------

def update_binary_file_2(sbox):
  "update to an old revision of a binary files"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Suck up contents of a test .png file.
  theta_contents = open(os.path.join(sys.path[0], "theta.bin"), 'rb').read()

  # 102400 is svn_txdelta_window_size.  We're going to make sure we
  # have at least 102401 bytes of data in our second binary file (for
  # no reason other than we have had problems in the past with getting
  # svndiff data out of the repository for files > 102400 bytes).
  # How?  Well, we'll just keep doubling the binary contents of the
  # original theta.png until we're big enough.
  zeta_contents = theta_contents
  while(len(zeta_contents) < 102401):
    zeta_contents = zeta_contents + zeta_contents

  # Write our two files' contents out to disk, in A/theta and A/zeta.
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  svntest.main.file_write(theta_path, theta_contents, 'wb')
  zeta_path = os.path.join(wc_dir, 'A', 'zeta')
  svntest.main.file_write(zeta_path, zeta_contents, 'wb')

  # Now, `svn add' those two files.
  svntest.main.run_svn(None, 'add', theta_path, zeta_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Adding  (bin)'),
    'A/zeta' : Item(verb='Adding  (bin)'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    'A/zeta' : Item(status='  ', wc_rev=2),
    })

  # Commit the new binary filea, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Make some mods to the binary files.
  svntest.main.file_append(theta_path, "foobar")
  new_theta_contents = theta_contents + "foobar"
  svntest.main.file_append(zeta_path, "foobar")
  new_zeta_contents = zeta_contents + "foobar"

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Sending'),
    'A/zeta' : Item(verb='Sending'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=3),
    'A/zeta' : Item(status='  ', wc_rev=3),
    })

  # Commit original working copy again, creating revision 3.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create expected output tree for an update to rev 2.
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(status='U '),
    'A/zeta' : Item(status='U '),
    })

  # Create expected disk tree for the update --
  #    look!  binary contents, and a binary property!
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta' : Item(theta_contents,
                     props={'svn:mime-type' : 'application/octet-stream'}),
    'A/zeta' : Item(zeta_contents,
                    props={'svn:mime-type' : 'application/octet-stream'}),
    })

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    'A/zeta' : Item(status='  ', wc_rev=2),
    })

  # Do an update from revision 2 and make sure that our binary file
  # gets reverted to its original contents.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '2', wc_dir)


#----------------------------------------------------------------------

def update_missing(sbox):
  "update missing items (by name) in working copy"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove some files and dirs from the working copy.
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')

  # remove two files to verify that they get restored
  os.remove(mu_path)
  os.remove(rho_path)

  ### FIXME I think directories work because they generate 'A'
  ### feedback, is this the correct feedback?
  svntest.main.safe_rmtree(E_path)
  svntest.main.safe_rmtree(H_path)

  # In single-db mode all missing items will just be restored
  if svntest.main.wc_is_singledb(wc_dir):
    A_or_Restored = Item(verb='Restored')
  else:
    A_or_Restored = Item(status='A ')

  # Create expected output tree for an update of the missing items by name
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'        : Item(verb='Restored'),
    'A/D/G/rho'   : Item(verb='Restored'),
    'A/B/E'       : A_or_Restored,
    'A/B/E/alpha' : A_or_Restored,
    'A/B/E/beta'  : A_or_Restored,
    'A/D/H'       : A_or_Restored,
    'A/D/H/chi'   : A_or_Restored,
    'A/D/H/omega' : A_or_Restored,
    'A/D/H/psi'   : A_or_Restored,
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 0,
                                        mu_path, rho_path,
                                        E_path, H_path)

#----------------------------------------------------------------------

def update_ignores_added(sbox):
  "update should not munge adds or replaces"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Commit something so there's actually a new revision to update to.
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(rho_path, "More stuff in rho.\n")
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg', rho_path)

  # Create a new file, 'zeta', and schedule it for addition.
  zeta_path = os.path.join(wc_dir, 'A', 'B', 'zeta')
  svntest.main.file_append(zeta_path, "This is the file 'zeta'.\n")
  svntest.main.run_svn(None, 'add', zeta_path)

  # Schedule another file, say, 'gamma', for replacement.
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.run_svn(None, 'delete', gamma_path)
  svntest.main.file_append(gamma_path, "This is a new 'gamma' now.\n")
  svntest.main.run_svn(None, 'add', gamma_path)

  # Now update.  "zeta at revision 0" should *not* be reported at all,
  # so it should remain scheduled for addition at revision 0.  gamma
  # was scheduled for replacement, so it also should remain marked as
  # such, and maintain its revision of 1.

  # Create expected output tree for an update of the wc_backup.
  expected_output = svntest.wc.State(wc_dir, { })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/zeta' : Item("This is the file 'zeta'.\n"),
    })
  expected_disk.tweak('A/D/gamma', contents="This is a new 'gamma' now.\n")
  expected_disk.tweak('A/D/G/rho',
                      contents="This is the file 'rho'.\nMore stuff in rho.\n")

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)

  # Before WC-NG we couldn't bump the wc_rev for gamma from 1 to 2 because it could
  # be replaced with history and we couldn't store all the revision information.
  # WC-NG just bumps the revision as it can easily store different revisions.
  expected_status.tweak('A/D/gamma', wc_rev=2, status='R ')
  expected_status.add({
    'A/B/zeta' : Item(status='A ', wc_rev=0),
    })

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


#----------------------------------------------------------------------

def update_to_rev_zero(sbox):
  "update to revision 0"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  A_path = os.path.join(wc_dir, 'A')

  # Create expected output tree for an update to rev 0
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='D '),
    'A' : Item(status='D '),
    })

  # Create expected disk tree for the update to rev 0
  expected_disk = svntest.wc.State(wc_dir, { })

  # Do the update and check the results.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        '-r', '0', wc_dir)

#----------------------------------------------------------------------

def receive_overlapping_same_change(sbox):
  "overlapping identical changes should not conflict"

  ### (See http://subversion.tigris.org/issues/show_bug.cgi?id=682.)
  ###
  ### How this test works:
  ###
  ### Create working copy foo, modify foo/iota.  Duplicate foo,
  ### complete with locally modified iota, to bar.  Now we should
  ### have:
  ###
  ###    $ svn st foo
  ###    M    foo/iota
  ###    $ svn st bar
  ###    M    bar/iota
  ###    $
  ###
  ### Commit the change from foo, then update bar.  The repository
  ### change should get folded into bar/iota with no conflict, since
  ### the two modifications are identical.

  sbox.build()
  wc_dir = sbox.wc_dir

  # Modify iota.
  iota_path = os.path.join(wc_dir, 'iota')
  svntest.main.file_append(iota_path, "A change to iota.\n")

  # Duplicate locally modified wc, giving us the "other" wc.
  other_wc = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, other_wc)
  other_iota_path = os.path.join(other_wc, 'iota')

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)

  # Commit the change, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Expected output tree for update of other_wc.
  expected_output = svntest.wc.State(other_wc, {
    'iota' : Item(status='G '),
    })

  # Expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota',
                      contents="This is the file 'iota'.\nA change to iota.\n")

  # Expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(other_wc, 2)

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(other_wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

def update_to_resolve_text_conflicts(sbox):
  "delete files and update to resolve text conflicts"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Make a couple of local mods to files which will be committed
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(mu_path, 'Original appended text for mu\n')
  svntest.main.file_append(rho_path, 'Original appended text for rho\n')
  svntest.main.run_svn(None, 'propset', 'Kubla', 'Khan', rho_path)

  # Make a couple of local mods to files which will be conflicted
  mu_path_backup = os.path.join(wc_backup, 'A', 'mu')
  rho_path_backup = os.path.join(wc_backup, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(mu_path_backup,
                           'Conflicting appended text for mu\n')
  svntest.main.file_append(rho_path_backup,
                           'Conflicting appended text for rho\n')
  svntest.main.run_svn(None, 'propset', 'Kubla', 'Xanadu', rho_path_backup)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Sending'),
    })

  # Create expected status tree; all local revisions should be at 1,
  # but mu and rho should be at revision 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  expected_status.tweak('A/D/G/rho', wc_rev=2, status='  ')

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create expected output tree for an update of the wc_backup.
  expected_output = svntest.wc.State(wc_backup, {
    'A/mu' : Item(status='C '),
    'A/D/G/rho' : Item(status='CC'),
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu',
                      contents="\n".join(["This is the file 'mu'.",
                                          "<<<<<<< .mine",
                                          "Conflicting appended text for mu",
                                          "=======",
                                          "Original appended text for mu",
                                          ">>>>>>> .r2",
                                          ""]))
  expected_disk.tweak('A/D/G/rho',
                      contents="\n".join(["This is the file 'rho'.",
                                          "<<<<<<< .mine",
                                          "Conflicting appended text for rho",
                                          "=======",
                                          "Original appended text for rho",
                                          ">>>>>>> .r2",
                                          ""]))

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.tweak('A/mu', status='C ')
  expected_status.tweak('A/D/G/rho', status='CC')

  # "Extra" files that we expect to result from the conflicts.
  # These are expressed as list of regexps.  What a cool system!  :-)
  extra_files = ['mu.*\.r1', 'mu.*\.r2', 'mu.*\.mine',
                 'rho.*\.r1', 'rho.*\.r2', 'rho.*\.mine', 'rho.*\.prej']

  # Do the update and check the results in three ways.
  # All "extra" files are passed to detect_conflict_files().
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files)


  # verify that the extra_files list is now empty.
  if len(extra_files) != 0:
    print("didn't get expected extra files")
    raise svntest.Failure

  # remove the conflicting files to clear text conflict but not props conflict
  os.remove(mu_path_backup)
  os.remove(rho_path_backup)

  ### TODO: Can't get run_and_verify_update to work here :-( I get
  # the error "Unequal Types: one Node is a file, the other is a
  # directory". Use run_svn and then run_and_verify_status instead
  exit_code, stdout_lines, stdout_lines = svntest.main.run_svn(None, 'up',
                                                               wc_backup)
  if len (stdout_lines) > 0:
    print("update 2 failed")
    raise svntest.Failure

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.tweak('A/D/G/rho', status=' C')

  svntest.actions.run_and_verify_status(wc_backup, expected_status)

#----------------------------------------------------------------------

def update_delete_modified_files(sbox):
  "update that deletes modified files"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete a file
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  svntest.actions.run_and_verify_svn("Deleting alpha failed", None, [],
                                     'rm', alpha_path)

  # Delete a directory containing files
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  svntest.actions.run_and_verify_svn("Deleting G failed", None, [],
                                     'rm', G_path)

  # Commit
  svntest.actions.run_and_verify_svn("Committing deletes failed", None, [],
                                     'ci', '-m', 'log msg', wc_dir)

  ### Update before backdating to avoid obstructed update error for G
  svntest.actions.run_and_verify_svn("Updating after commit failed", None, [],
                                     'up', wc_dir)

  # Backdate to restore deleted items
  svntest.actions.run_and_verify_svn("Backdating failed", None, [],
                                     'up', '-r', '1', wc_dir)

  # Modify the file to be deleted, and a file in the directory to be deleted
  svntest.main.file_append(alpha_path, 'appended alpha text\n')
  pi_path = os.path.join(G_path, 'pi')
  svntest.main.file_append(pi_path, 'appended pi text\n')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', 'A/D/G/pi', status='M ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now update to 'delete' modified items -- that is, remove them from
  # version control, but leave them on disk.  It used to be we would
  # expect an 'obstructed update' error (see issue #1196), then we
  # expected success (see issue #1806), and now we expect tree conflicts
  # (see issue #2282) on the missing or unversioned items.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha' : Item(status='  ', treeconflict='C'),
    'A/D/G'       : Item(status='  ', treeconflict='C'),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/B/E/alpha',
                      contents=\
                      "This is the file 'alpha'.\nappended alpha text\n")
  expected_disk.tweak('A/D/G/pi',
                      contents=\
                      "This is the file 'pi'.\nappended pi text\n")

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  # A/B/E/alpha and the subtree rooted at A/D/G had local modificiations
  # prior to the update.  So there is a tree conflict and both A/B/E/alpha
  # A/D/G remain after the update, scheduled for addition as copies of
  # themselves from r1, along with the local modifications.
  expected_status.tweak('A/B/E/alpha', status='A ', copied='+', wc_rev='-',
                        treeconflict='C')
  expected_status.tweak('A/D/G/pi', status='M ')
  expected_status.tweak('A/D/G/pi', status='M ', copied='+', wc_rev='-')
  expected_status.tweak('A/D/G/rho', 'A/D/G/tau', status='  ', copied='+',
                        wc_rev='-')
  expected_status.tweak('A/D/G', status='A ', copied='+', wc_rev='-',
                        treeconflict='C')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

# Issue 847.  Doing an add followed by a remove for an item in state
# "deleted" caused the "deleted" state to get forgotten

def update_after_add_rm_deleted(sbox):
  "update after add/rm of deleted state"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete a file and directory from WC
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', alpha_path, F_path)

  # Commit deletion
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha' : Item(verb='Deleting'),
    'A/B/F'       : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha')
  expected_status.remove('A/B/F')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # alpha and F are now in state "deleted", next we add a new ones
  svntest.main.file_append(alpha_path, "new alpha")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', alpha_path)

  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', F_path)

  # New alpha and F should be in add state A
  expected_status.add({
    'A/B/E/alpha' : Item(status='A ', wc_rev=0),
    'A/B/F'       : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Forced removal of new alpha and F must restore "deleted" state

  svntest.actions.run_and_verify_svn(None, None, [], 'rm', '--force',
                                     alpha_path, F_path)
  if os.path.exists(alpha_path) or os.path.exists(F_path):
    raise svntest.Failure

  # "deleted" state is not visible in status
  expected_status.remove('A/B/E/alpha', 'A/B/F')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Although parent dir is already at rev 1, the "deleted" state will cause
  # alpha and F to be restored in the WC when updated to rev 1
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r', '1', wc_dir)

  expected_status.add({
    'A/B/E/alpha' : Item(status='  ', wc_rev=1),
    'A/B/F'       : Item(status='  ', wc_rev=1),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

# Issue 1591.  Updating a working copy which contains local
# obstructions marks a directory as incomplete.  Removal of the
# obstruction and subsequent update should clear the "incomplete"
# flag.

def obstructed_update_alters_wc_props(sbox):
  "obstructed update alters WC properties"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new dir in the repo in prep for creating an obstruction.
  #print "Adding dir to repo"
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', '-m',
                                     'prep for obstruction',
                                     sbox.repo_url + '/A/foo')

  # Create an obstruction, a file in the WC with the same name as
  # present in a newer rev of the repo.
  #print "Creating obstruction"
  obstruction_parent_path = os.path.join(wc_dir, 'A')
  obstruction_path = os.path.join(obstruction_parent_path, 'foo')
  svntest.main.file_append(obstruction_path, 'an obstruction')

  # Update the WC to that newer rev to trigger the obstruction.
  #print "Updating WC"
  # svntest.factory.make(sbox, 'svn update')
  # exit(0)
  expected_output = svntest.wc.State(wc_dir, {
    'A/foo'             : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/foo'             : Item(contents="an obstruction"),
  })

  expected_status = actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/foo'             : Item(status='D ', treeconflict='C', wc_rev=2),
  })

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)


  # Remove the file which caused the obstruction.
  #print "Removing obstruction"
  os.unlink(obstruction_path)

  svntest.main.run_svn(None, 'revert', obstruction_path)

  # Update the -- now unobstructed -- WC again.
  #print "Updating WC again"
  expected_output = svntest.wc.State(wc_dir, {
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/foo' : Item(),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/foo' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # The previously obstructed resource should now be in the WC.
  if not os.path.isdir(obstruction_path):
    raise svntest.Failure

#----------------------------------------------------------------------

# Issue 938.
def update_replace_dir(sbox):
  "update that replaces a directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete a directory
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', F_path)

  # Commit deletion
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F'       : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/F')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Add replacement directory
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', F_path)

  # Commit addition
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F'       : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/F', wc_rev=3)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update to HEAD
  expected_output = svntest.wc.State(wc_dir, {
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # Update to revision 1 replaces the directory
  ### I can't get this to work :-(
  #expected_output = svntest.wc.State(wc_dir, {
  #  'A/B/F'       : Item(verb='Adding'),
  #  'A/B/F'       : Item(verb='Deleting'),
  #  })
  #expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  #svntest.actions.run_and_verify_update(wc_dir,
  #                                      expected_output,
  #                                      expected_disk,
  #                                      expected_status,
  #                                      None, None, None, None, None, 0,
  #                                      '-r', '1', wc_dir)

  # Update to revision 1 replaces the directory
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r', '1', wc_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def update_single_file(sbox):
  "update with explicit file target"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_disk = svntest.main.greek_state.copy()

  # Make a local mod to a file which will be committed
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, '\nAppended text for mu')

  # Commit.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # At one stage 'svn up file' failed with a parent lock error
  was_cwd = os.getcwd()
  os.chdir(os.path.join(wc_dir, 'A'))

  ### Can't get run_and_verify_update to work having done the chdir.
  svntest.actions.run_and_verify_svn("update failed", None, [],
                                     'up', '-r', '1', 'mu')
  os.chdir(was_cwd)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
def prop_update_on_scheduled_delete(sbox):
  "receive prop update to file scheduled for deletion"

  sbox.build()
  wc_dir = sbox.wc_dir

  other_wc = sbox.add_wc_path('other')

  # Make the "other" working copy.
  svntest.actions.duplicate_dir(wc_dir, other_wc)

  iota_path = os.path.join(wc_dir, 'iota')
  other_iota_path = os.path.join(other_wc, 'iota')

  svntest.main.run_svn(None, 'propset', 'foo', 'bar', iota_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)

  # Commit the change, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  svntest.main.run_svn(None, 'rm', other_iota_path)

  # Expected output tree for update of other_wc.
  expected_output = svntest.wc.State(other_wc, {
    'iota' : Item(status='  ', treeconflict='C'),
    })

  # Expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')

  # Expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(other_wc, 2)
  expected_status.tweak('iota', status='D ', treeconflict='C')

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(other_wc,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

def update_receive_illegal_name(sbox):
  "bail when receive a file or dir named .svn"

  sbox.build()
  wc_dir = sbox.wc_dir

  # This tests the revision 4334 fix for issue #1068.

  legal_url = sbox.repo_url + '/A/D/G/svn'
  illegal_url = (sbox.repo_url
                 + '/A/D/G/' + svntest.main.get_admin_name())
  # Ha!  The client doesn't allow us to mkdir a '.svn' but it does
  # allow us to copy to a '.svn' so ...
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', '-m', 'log msg',
                                     legal_url)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', '-m', 'log msg',
                                     legal_url, illegal_url)

  # Do the update twice, both should fail.  After the first failure
  # the wc will be marked "incomplete".
  for n in range(2):
    exit_code, out, err = svntest.main.run_svn(1, 'up', wc_dir)
    for line in err:
      if line.find("of the same name") != -1:
        break
    else:
      raise svntest.Failure

  # At one stage an obstructed update in an incomplete wc would leave
  # a txn behind
  exit_code, out, err = svntest.main.run_svnadmin('lstxns', sbox.repo_dir)
  if out or err:
    raise svntest.Failure

#----------------------------------------------------------------------

def update_deleted_missing_dir(sbox):
  "update missing dir to rev in which it is absent"

  sbox.build()
  wc_dir = sbox.wc_dir

  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')

  # Create a new revision with directories deleted
  svntest.main.run_svn(None, 'rm', E_path)
  svntest.main.run_svn(None, 'rm', H_path)
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg', E_path, H_path)

  # Update back to the old revision
  svntest.main.run_svn(None,
                       'up', '-r', '1', wc_dir)

  # Delete the directories from disk
  svntest.main.safe_rmtree(E_path)
  svntest.main.safe_rmtree(H_path)

  # Create expected output tree for an update of the missing items by name
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E' : Item(status='D '),
    'A/D/H' : Item(status='D '),
    })

  # In single-db mode the missing items are restored before the update
  if svntest.main.wc_is_singledb(wc_dir):
    expected_output.add({
      'A/D/H/psi'         : Item(verb='Restored'),
      'A/D/H/omega'       : Item(verb='Restored'),
      'A/D/H/chi'         : Item(verb='Restored'),
      'A/B/E/beta'        : Item(verb='Restored'),
      'A/B/E/alpha'       : Item(verb='Restored')
      # A/B/E and A/D/H are also restored, but are then overriden by the delete
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  expected_disk.remove('A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  expected_status.remove('A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi')

  # Do the update, specifying the deleted paths explicitly.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        0, "-r", "2", E_path, H_path)

  # Update back to the old revision again
  svntest.main.run_svn(None,
                       'up', '-r', '1', wc_dir)

  # This time we're updating the whole working copy
  expected_status.tweak(wc_rev=2)

  # And now we don't expect restore operations
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E' : Item(status='D '),
    'A/D/H' : Item(status='D '),
    })

  # Do the update, on the whole working copy this time
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        0, "-r", "2", wc_dir)

#----------------------------------------------------------------------

# Issue 919.  This test was written as a regression test for "item
# should remain 'deleted' when an update deletes a sibling".
def another_hudson_problem(sbox):
  "another \"hudson\" problem: updates that delete"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete/commit gamma thus making it 'deleted'
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.run_svn(None, 'rm', gamma_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/gamma')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Delete directory G from the repository
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 3.\n'], [],
                                     'rm', '-m', 'log msg',
                                     sbox.repo_url + '/A/D/G')

  # Remove corresponding tree from working copy
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  svntest.main.safe_rmtree(G_path)

  # Update missing directory to receive the delete, this should mark G
  # as 'deleted' and should not alter gamma's entry.

  if not svntest.main.wc_is_singledb(wc_dir):
    expected_output = ["Updating '%s' ...\n" % (G_path),
                       'D    '+G_path+'\n',
                       'Updated to revision 3.\n',
                       ]
  else:
    expected_output = ["Updating '%s':\n" % (G_path),
                       'Restored \'' + G_path + '\'\n',
                       'Restored \'' + G_path + os.path.sep + 'pi\'\n',
                       'Restored \'' + G_path + os.path.sep + 'rho\'\n',
                       'Restored \'' + G_path + os.path.sep + 'tau\'\n',
                       'D    '+G_path+'\n',
                       'Updated to revision 3.\n',
                       ]

  # Sigh, I can't get run_and_verify_update to work (but not because
  # of issue 919 as far as I can tell)
  expected_output = svntest.verify.UnorderedOutput(expected_output)
  svntest.actions.run_and_verify_svn(None,
                                     expected_output, [],
                                     'up', G_path)

  # Both G and gamma should be 'deleted', update should produce no output
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                         'A/D/gamma')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                       'A/D/gamma')

  svntest.actions.run_and_verify_update(wc_dir,
                                        "",
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------
def update_deleted_targets(sbox):
  "explicit update of deleted=true targets"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete/commit thus creating 'deleted=true' entries
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  svntest.main.run_svn(None, 'rm', gamma_path, F_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Deleting'),
    'A/B/F'     : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/gamma', 'A/B/F')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Explicit update must not remove the 'deleted=true' entries
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(2), [],
                                     'update', gamma_path)
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(2), [],
                                     'update', F_path)

  # Update to r1 to restore items, since the parent directory is already
  # at r1 this fails if the 'deleted=true' entries are missing (issue 2250)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(status='A '),
    'A/B/F'     : Item(status='A '),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  expected_disk = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 0,
                                        '-r', '1', wc_dir)



#----------------------------------------------------------------------

def new_dir_with_spaces(sbox):
  "receive new dir with spaces in its name"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new directory ("spacey dir") directly in repository
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'mkdir', '-m', 'log msg',
                                     sbox.repo_url
                                     + '/A/spacey%20dir')

  # Update, and make sure ra_neon doesn't choke on the space.
  expected_output = svntest.wc.State(wc_dir, {
    'A/spacey dir'       : Item(status='A '),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/spacey dir'       : Item(status='  ', wc_rev=2),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/spacey dir' : Item(),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

def non_recursive_update(sbox):
  "non-recursive update"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Commit a change to A/mu and A/D/G/rho
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')

  svntest.main.file_append(mu_path, "new")
  svntest.main.file_append(rho_path, "new")

  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', 'A/D/G/rho', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Update back to revision 1
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(status='U '),
    'A/D/G/rho' : Item(status='U '),
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status.tweak('A/mu', 'A/D/G/rho', wc_rev=1)

  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, 0,
                                        '-r', '1', wc_dir)

  # Non-recursive update of A should change A/mu but not A/D/G/rho
  A_path = os.path.join(wc_dir, 'A')

  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(status='U '),
    })

  expected_status.tweak('A', 'A/mu', wc_rev=2)

  expected_disk.tweak('A/mu', contents="This is the file 'mu'.\nnew")

  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, 0,
                                        '-N', A_path)

#----------------------------------------------------------------------

def checkout_empty_dir(sbox):
  "check out an empty dir"
  # See issue #1472 -- checked out empty dir should not be marked as
  # incomplete ("!" in status).
  sbox.build(create_wc = False)
  wc_dir = sbox.wc_dir

  C_url = sbox.repo_url + '/A/C'

  svntest.main.safe_rmtree(wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout', C_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, [], [], 'status', wc_dir)


#----------------------------------------------------------------------
# Regression test for issue #919: "another ghudson bug".  Basically, if
# we fore- or back-date an item until it no longer exists, we were
# completely removing the entry, rather than marking it 'deleted'
# (which we now do.)

def update_to_deletion(sbox):
  "update target till it's gone, then get it back"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')

  # Update iota to rev 0, so it gets removed.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        '-r', '0', iota_path)

  # Update the wc root, so iota comes back.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        wc_dir)


#----------------------------------------------------------------------

def update_deletion_inside_out(sbox):
  "update child before parent of a deleted tree"

  sbox.build()
  wc_dir = sbox.wc_dir

  parent_path = os.path.join(wc_dir, 'A', 'B')
  child_path = os.path.join(parent_path, 'E')  # Could be a file, doesn't matter

  # Delete the parent directory.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', parent_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '', wc_dir)

  # Update back to r1.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'update', '-r', '1', wc_dir)

  # Update just the child to r2.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'update', '-r', '2', child_path)

  # Now try a normal update.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B' : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B', 'A/B/lambda', 'A/B/F',
                       'A/B/E', 'A/B/E/alpha', 'A/B/E/beta')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None)


#----------------------------------------------------------------------
# Regression test for issue #1793, whereby 'svn up dir' would delete
# dir if schedule-add.  Yikes.

def update_schedule_add_dir(sbox):
  "update a schedule-add directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete directory A/D/G in the repository via immediate commit
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  G_url = sbox.repo_url + '/A/D/G'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', G_url, '-m', 'rev 2')

  # Update the wc to HEAD (r2)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G' : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # Do a URL->wc copy, creating a new schedule-add A/D/G.
  # (Standard procedure when trying to resurrect the directory.)
  D_path = os.path.join(wc_dir, 'A', 'D')
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', G_url + '@1', D_path)

  # status should now show the dir scheduled for addition-with-history
  expected_status.add({
    'A/D/G'     : Item(status='A ', copied='+', wc_rev='-'),
    'A/D/G/pi'  : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/G/rho' : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/G/tau' : Item(status='  ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now update with the schedule-add dir as the target.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', G_path)

  # The update should be a no-op, and the schedule-add directory
  # should still exist!  'svn status' shouldn't change at all.
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
# Test updating items that do not exist in the current WC rev, but do
# exist at some future revision.

def update_to_future_add(sbox):
  "update target that was added in a future rev"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Update the entire WC to rev 0
  # Create expected output tree for an update to rev 0
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='D '),
    'A' : Item(status='D '),
    })

  # Create expected disk tree for the update to rev 0
  expected_disk = svntest.wc.State(wc_dir, { })

  # Do the update and check the results.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        '-r', '0', wc_dir)

  # Update iota to the current HEAD.
  iota_path = os.path.join(wc_dir, 'iota')

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='A '),
    })

  expected_disk = svntest.wc.State('', {
   'iota' : Item("This is the file 'iota'.\n")
   })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        iota_path)

  # Now try updating the directory into the future
  A_path = os.path.join(wc_dir, 'A')

  expected_output = svntest.wc.State(wc_dir, {
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

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        A_path);

#----------------------------------------------------------------------

def nested_in_read_only(sbox):
  "update a nested wc in a read-only wc"

  sbox.build()
  wc_dir = sbox.wc_dir

  if svntest.main.wc_is_singledb(wc_dir):
    raise svntest.Skip('Unsupported in single-db')

  # Delete/commit a file
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', alpha_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha' : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E/alpha')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  expected_status.tweak(wc_rev=2)

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Delete/commit a directory that used to contain the deleted file
  B_path = os.path.join(wc_dir, 'A', 'B')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', B_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B' : Item(verb='Deleting'),
    })

  expected_status.remove('A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/beta', 'A/B/F')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  expected_status.tweak(wc_rev=3)

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Replace the deleted directory with a new checkout of an old
  # version of the directory, this gives it a "plausible" URL that
  # could be part of the containing wc
  B_url = sbox.repo_url + '/A/B'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout', '-r', '1', B_url + "@1",
                                     B_path)

  expected_status = svntest.wc.State(B_path, {
    ''           : Item(),
    'lambda'     : Item(),
    'E'          : Item(),
    'E/alpha'    : Item(),
    'E/beta'     : Item(),
    'F'          : Item(),
    })
  expected_status.tweak(wc_rev=1, status='  ')

  svntest.actions.run_and_verify_status(B_path, expected_status)

  # Make enclosing wc read only
  os.chmod(os.path.join(wc_dir, 'A', svntest.main.get_admin_name()), 0555)

  try:
    # Update of nested wc should still work
    expected_output = svntest.wc.State(B_path, {
      'E/alpha' : Item(status='D '),
      })

    expected_disk = wc.State('', {
      'lambda'  : wc.StateItem("This is the file 'lambda'.\n"),
      'E'       : wc.StateItem(),
      'E/beta'  : wc.StateItem("This is the file 'beta'.\n"),
      'F'       : wc.StateItem(),
      })

    expected_status.remove('E/alpha')
    expected_status.tweak(wc_rev=2)

    svntest.actions.run_and_verify_update(B_path,
                                          expected_output,
                                          expected_disk,
                                          expected_status,
                                          None, None, None, None, None, 0,
                                          '-r', '2', B_path)
  finally:
    os.chmod(os.path.join(wc_dir, 'A', svntest.main.get_admin_name()), 0777)

#----------------------------------------------------------------------

def update_xml_unsafe_dir(sbox):
  "update dir with xml-unsafe name"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Make a couple of local mods to files
  test_path = os.path.join(wc_dir, ' foo & bar')
  svntest.main.run_svn(None, 'mkdir', test_path)

  # Created expected output tree for 'svn ci'
  expected_output = wc.State(wc_dir, {
    ' foo & bar' : Item(verb='Adding'),
    })

  # Create expected status tree; all local revisions should be at 1,
  # but 'foo & bar' should be at revision 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    ' foo & bar' : Item(status='  ', wc_rev=2),
    })

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # chdir into the funky path, and update from there.
  os.chdir(test_path)

  expected_output = wc.State('', {
    })

  expected_disk = wc.State('', {
    })

  expected_status = wc.State('', {
    '' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update('', expected_output, expected_disk,
                                        expected_status)

#----------------------------------------------------------------------
# eol-style handling during update with conflicts, scenario 1:
# when update creates a conflict on a file, make sure the file and files
# r<left>, r<right> and .mine are in the eol-style defined for that file.
#
# This test for 'svn merge' can be found in merge_tests.py as
# merge_conflict_markers_matching_eol.
def conflict_markers_matching_eol(sbox):
  "conflict markers should match the file's eol style"

  sbox.build()
  wc_dir = sbox.wc_dir
  filecount = 1

  mu_path = os.path.join(wc_dir, 'A', 'mu')

  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'

  # Checkout a second working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     sbox.repo_url, wc_backup)

  # set starting revision
  cur_rev = 1

  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, cur_rev)
  expected_backup_status = svntest.actions.get_virginal_state(wc_backup,
                                                              cur_rev)

  path_backup = os.path.join(wc_backup, 'A', 'mu')

  # do the test for each eol-style
  for eol, eolchar in zip(['CRLF', 'CR', 'native', 'LF'],
                          [crlf, '\015', '\n', '\012']):
    # rewrite file mu and set the eol-style property.
    svntest.main.file_write(mu_path, "This is the file 'mu'."+ eolchar, 'wb')
    svntest.main.run_svn(None, 'propset', 'svn:eol-style', eol, mu_path)

    expected_disk.add({
      'A/mu' : Item("This is the file 'mu'." + eolchar)
    })

    expected_output = svntest.wc.State(wc_dir, {
      'A/mu' : Item(verb='Sending'),
    })

    expected_status.tweak(wc_rev = cur_rev)
    expected_status.add({
      'A/mu' : Item(status='  ', wc_rev = cur_rev + 1),
    })

    # Commit the original change and note the 'base' revision number
    svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                          expected_status, None, wc_dir)
    cur_rev = cur_rev + 1
    base_rev = cur_rev

    svntest.main.run_svn(None, 'update', wc_backup)

    # Make a local mod to mu
    svntest.main.file_append(mu_path,
                             'Original appended text for mu' + eolchar)

    # Commit the original change and note the 'theirs' revision number
    svntest.main.run_svn(None, 'commit', '-m', 'test log', wc_dir)
    cur_rev = cur_rev + 1
    theirs_rev = cur_rev

    # Make a local mod to mu, will conflict with the previous change
    svntest.main.file_append(path_backup,
                             'Conflicting appended text for mu' + eolchar)

    # Create expected output tree for an update of the wc_backup.
    expected_backup_output = svntest.wc.State(wc_backup, {
      'A/mu' : Item(status='C '),
      })

    # Create expected disk tree for the update.
    expected_backup_disk = expected_disk.copy()

    # verify content of resulting conflicted file
    expected_backup_disk.add({
    'A/mu' : Item(contents= "This is the file 'mu'." + eolchar +
      "<<<<<<< .mine" + eolchar +
      "Conflicting appended text for mu" + eolchar +
      "=======" + eolchar +
      "Original appended text for mu" + eolchar +
      ">>>>>>> .r" + str(cur_rev) + eolchar),
    })
    # verify content of base(left) file
    expected_backup_disk.add({
    'A/mu.r' + str(base_rev ) : Item(contents= "This is the file 'mu'." +
      eolchar)
    })
    # verify content of theirs(right) file
    expected_backup_disk.add({
    'A/mu.r' + str(theirs_rev ) : Item(contents= "This is the file 'mu'." +
      eolchar +
      "Original appended text for mu" + eolchar)
    })
    # verify content of mine file
    expected_backup_disk.add({
    'A/mu.mine' : Item(contents= "This is the file 'mu'." +
      eolchar +
      "Conflicting appended text for mu" + eolchar)
    })

    # Create expected status tree for the update.
    expected_backup_status.add({
      'A/mu'   : Item(status='  ', wc_rev=cur_rev),
    })
    expected_backup_status.tweak('A/mu', status='C ')
    expected_backup_status.tweak(wc_rev = cur_rev)

    # Do the update and check the results in three ways.
    svntest.actions.run_and_verify_update(wc_backup,
                                          expected_backup_output,
                                          expected_backup_disk,
                                          expected_backup_status,
                                          None,
                                          None,
                                          None)

    # cleanup for next run
    svntest.main.run_svn(None, 'revert', '-R', wc_backup)
    svntest.main.run_svn(None, 'update', wc_dir)

# eol-style handling during update, scenario 2:
# if part of that update is a propchange (add, change, delete) of
# svn:eol-style, make sure the correct eol-style is applied before
# calculating the merge (and conflicts if any)
#
# This test for 'svn merge' can be found in merge_tests.py as
# merge_eolstyle_handling.
def update_eolstyle_handling(sbox):
  "handle eol-style propchange during update"

  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')

  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'

  # Checkout a second working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     sbox.repo_url, wc_backup)
  path_backup = os.path.join(wc_backup, 'A', 'mu')

  # Test 1: add the eol-style property and commit, change mu in the second
  # working copy and update; there should be no conflict!
  svntest.main.run_svn(None, 'propset', 'svn:eol-style', "CRLF", mu_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'set eol-style property', wc_dir)

  svntest.main.file_append_binary(path_backup, 'Added new line of text.\012')

  expected_backup_disk = svntest.main.greek_state.copy()
  expected_backup_disk.tweak(
  'A/mu', contents= "This is the file 'mu'." + crlf +
    "Added new line of text." + crlf)

  expected_backup_output = svntest.wc.State(wc_backup, {
    'A/mu' : Item(status='GU'),
    })

  expected_backup_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_backup_status.tweak('A/mu', status='M ')

  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_backup_output,
                                        expected_backup_disk,
                                        expected_backup_status,
                                        None, None, None)

  # Test 2: now change the eol-style property to another value and commit,
  # update the still changed mu in the second working copy; there should be
  # no conflict!
  svntest.main.run_svn(None, 'propset', 'svn:eol-style', "CR", mu_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'set eol-style property', wc_dir)

  expected_backup_disk = svntest.main.greek_state.copy()
  expected_backup_disk.add({
  'A/mu' : Item(contents= "This is the file 'mu'.\015" +
    "Added new line of text.\015")
  })

  expected_backup_output = svntest.wc.State(wc_backup, {
    'A/mu' : Item(status='GU'),
    })

  expected_backup_status = svntest.actions.get_virginal_state(wc_backup, 3)
  expected_backup_status.tweak('A/mu', status='M ')

  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_backup_output,
                                        expected_backup_disk,
                                        expected_backup_status,
                                        None, None, None)

  # Test 3: now delete the eol-style property and commit, update the still
  # changed mu in the second working copy; there should be no conflict!
  # EOL of mu should be unchanged (=CR).
  svntest.main.run_svn(None, 'propdel', 'svn:eol-style', mu_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'del eol-style property', wc_dir)

  expected_backup_disk = svntest.main.greek_state.copy()
  expected_backup_disk.add({
  'A/mu' : Item(contents= "This is the file 'mu'.\015" +
    "Added new line of text.\015")
  })

  expected_backup_output = svntest.wc.State(wc_backup, {
    'A/mu' : Item(status=' U'),
    })

  expected_backup_status = svntest.actions.get_virginal_state(wc_backup, 4)
  expected_backup_status.tweak('A/mu', status='M ')
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_backup_output,
                                        expected_backup_disk,
                                        expected_backup_status,
                                        None, None, None)

# Bug in which "update" put a bogus revision number on a schedule-add file,
# causing the wrong version of it to be committed.
def update_copy_of_old_rev(sbox):
  "update schedule-add copy of old rev"

  sbox.build()
  wc_dir = sbox.wc_dir

  dir = os.path.join(wc_dir, 'A')
  dir2 = os.path.join(wc_dir, 'A2')
  file = os.path.join(dir, 'mu')
  file2 = os.path.join(dir2, 'mu')
  url = sbox.repo_url + '/A/mu'
  url2 = sbox.repo_url + '/A2/mu'

  # Remember the original text of the file
  exit_code, text_r1, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                               'cat', '-r1',
                                                               url)

  # Commit a different version of the file
  svntest.main.file_write(file, "Second revision of 'mu'\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '', wc_dir)

  # Copy an old revision of its directory into a new path in the WC
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-r1', dir, dir2)

  # Update.  (Should do nothing, but added a bogus "revision" in "entries".)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', wc_dir)

  # Commit, and check that it says it's committing the right thing
  exp_out = ['Adding         ' + dir2 + '\n',
             '\n',
             'Committed revision 3.\n']
  svntest.actions.run_and_verify_svn(None, exp_out, [],
                                     'ci', '-m', '', wc_dir)

  # Verify the committed file's content
  svntest.actions.run_and_verify_svn(None, text_r1, [],
                                     'cat', url2)

#----------------------------------------------------------------------
def forced_update(sbox):
  "forced update tolerates obstructions to adds"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Make a couple of local mods to files
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(mu_path, 'appended mu text')
  svntest.main.file_append(rho_path, 'new appended text for rho')

  # Add some files
  nu_path = os.path.join(wc_dir, 'A', 'B', 'F', 'nu')
  svntest.main.file_append(nu_path, "This is the file 'nu'\n")
  svntest.main.run_svn(None, 'add', nu_path)
  kappa_path = os.path.join(wc_dir, 'kappa')
  svntest.main.file_append(kappa_path, "This is the file 'kappa'\n")
  svntest.main.run_svn(None, 'add', kappa_path)

  # Add a dir with two files
  I_path = os.path.join(wc_dir, 'A', 'C', 'I')
  os.mkdir(I_path)
  svntest.main.run_svn(None, 'add', I_path)
  upsilon_path = os.path.join(I_path, 'upsilon')
  svntest.main.file_append(upsilon_path, "This is the file 'upsilon'\n")
  svntest.main.run_svn(None, 'add', upsilon_path)
  zeta_path = os.path.join(I_path, 'zeta')
  svntest.main.file_append(zeta_path, "This is the file 'zeta'\n")
  svntest.main.run_svn(None, 'add', zeta_path)

  # Created expected output tree for 'svn ci'
  expected_output = wc.State(wc_dir, {
    'A/mu'          : Item(verb='Sending'),
    'A/D/G/rho'     : Item(verb='Sending'),
    'A/B/F/nu'      : Item(verb='Adding'),
    'kappa'         : Item(verb='Adding'),
    'A/C/I'         : Item(verb='Adding'),
    'A/C/I/upsilon' : Item(verb='Adding'),
    'A/C/I/zeta'    : Item(verb='Adding'),
    })

  # Create expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/nu'      : Item(status='  ', wc_rev=2),
    'kappa'         : Item(status='  ', wc_rev=2),
    'A/C/I'         : Item(status='  ', wc_rev=2),
    'A/C/I/upsilon' : Item(status='  ', wc_rev=2),
    'A/C/I/zeta'    : Item(status='  ', wc_rev=2),
    })
  expected_status.tweak('A/mu', 'A/D/G/rho', wc_rev=2)

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Make a local mod to mu that will merge cleanly.
  backup_mu_path = os.path.join(wc_backup, 'A', 'mu')
  svntest.main.file_append(backup_mu_path, 'appended mu text')

  # Create unversioned files and dir that will obstruct A/B/F/nu, kappa,
  # A/C/I, and A/C/I/upsilon coming from repos during update.
  # The obstructing nu has the same contents as  the repos, while kappa and
  # upsilon differ, which means the latter two should show as modified after
  # the forced update.
  nu_path = os.path.join(wc_backup, 'A', 'B', 'F', 'nu')
  svntest.main.file_append(nu_path, "This is the file 'nu'\n")
  kappa_path = os.path.join(wc_backup, 'kappa')
  svntest.main.file_append(kappa_path,
                           "This is the OBSTRUCTING file 'kappa'\n")
  I_path = os.path.join(wc_backup, 'A', 'C', 'I')
  os.mkdir(I_path)
  upsilon_path = os.path.join(I_path, 'upsilon')
  svntest.main.file_append(upsilon_path,
                           "This is the OBSTRUCTING file 'upsilon'\n")

  # Create expected output tree for an update of the wc_backup.
  # mu and rho are run of the mill update operations; merge and update
  # respectively.
  # kappa, nu, I, and upsilon all 'E'xisted as unversioned items in the WC.
  # While the dir I does exist, zeta does not so it's just an add.
  expected_output = wc.State(wc_backup, {
    'A/mu'          : Item(status='G '),
    'A/D/G/rho'     : Item(status='U '),
    'kappa'         : Item(status='E '),
    'A/B/F/nu'      : Item(status='E '),
    'A/C/I'         : Item(status='E '),
    'A/C/I/upsilon' : Item(status='E '),
    'A/C/I/zeta'    : Item(status='A '),
    })

  # Create expected output tree for an update of the wc_backup.
  #
  # - mu and rho are run of the mill update operations; merge and update
  #   respectively.
  #
  # - kappa, nu, I, and upsilon all 'E'xisted as unversioned items in the WC.
  #
  # - While the dir I does exist, I/zeta does not so it's just an add.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/F/nu'      : Item("This is the file 'nu'\n"),
    'kappa'         : Item("This is the OBSTRUCTING file 'kappa'\n"),
    'A/C/I'         : Item(),
    'A/C/I/upsilon' : Item("This is the OBSTRUCTING file 'upsilon'\n"),
    'A/C/I/zeta'    : Item("This is the file 'zeta'\n"),
    })
  expected_disk.tweak('A/mu',
                      contents=expected_disk.desc['A/mu'].contents
                      + 'appended mu text')
  expected_disk.tweak('A/D/G/rho',
                      contents=expected_disk.desc['A/D/G/rho'].contents
                      + 'new appended text for rho')

  # Create expected status tree for the update.  Since the obstructing
  # kappa and upsilon differ from the repos, they should show as modified.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.add({
    'A/B/F/nu'      : Item(status='  ', wc_rev=2),
    'A/C/I'         : Item(status='  ', wc_rev=2),
    'A/C/I/zeta'    : Item(status='  ', wc_rev=2),
    'kappa'         : Item(status='M ', wc_rev=2),
    'A/C/I/upsilon' : Item(status='M ', wc_rev=2),
    })

  # Perform forced update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 0,
                                        wc_backup, '--force')

#----------------------------------------------------------------------
def forced_update_failures(sbox):
  "forced up fails with some types of obstructions"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Add a file
  nu_path = os.path.join(wc_dir, 'A', 'B', 'F', 'nu')
  svntest.main.file_append(nu_path, "This is the file 'nu'\n")
  svntest.main.run_svn(None, 'add', nu_path)

  # Add a dir
  I_path = os.path.join(wc_dir, 'A', 'C', 'I')
  os.mkdir(I_path)
  svntest.main.run_svn(None, 'add', I_path)

  # Created expected output tree for 'svn ci'
  expected_output = wc.State(wc_dir, {
    'A/B/F/nu'      : Item(verb='Adding'),
    'A/C/I'         : Item(verb='Adding'),
    })

  # Create expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/nu'      : Item(status='  ', wc_rev=2),
    'A/C/I'         : Item(status='  ', wc_rev=2),
    })

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create an unversioned dir A/B/F/nu that will obstruct the file of the
  # same name coming from the repository.  Create an unversioned file A/C/I
  # that will obstruct the dir of the same name.
  nu_path = os.path.join(wc_backup, 'A', 'B', 'F', 'nu')
  os.mkdir(nu_path)
  I_path = os.path.join(wc_backup, 'A', 'C', 'I')
  svntest.main.file_append(I_path,
                           "This is the file 'I'...shouldn't I be a dir?\n")

  # A forced update that tries to add a file when an unversioned directory
  # of the same name already exists should fail.
  #svntest.factory.make(sbox, """svn up --force $WC_DIR.backup/A/B/F""")
  #exit(0)
  backup_A_B_F = os.path.join(wc_backup, 'A', 'B', 'F')

  # svn up --force $WC_DIR.backup/A/B/F
  expected_output = svntest.wc.State(wc_backup, {
    'A/B/F/nu'          : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/F/nu'          : Item(),
    'A/C/I'             :
    Item(contents="This is the file 'I'...shouldn't I be a dir?\n"),
  })

  expected_status = actions.get_virginal_state(wc_backup, 1)
  expected_status.add({
    'A/B/F/nu'          : Item(status='D ', treeconflict='C', wc_rev='2'),
  })
  expected_status.tweak('A/B/F', wc_rev='2')

  actions.run_and_verify_update(wc_backup, expected_output,
    expected_disk, expected_status, None, None, None, None, None, False,
    '--force', backup_A_B_F)


  # A forced update that tries to add a directory when an unversioned file
  # of the same name already exists should fail.
  # svntest.factory.make(sbox, """
  #   svn up --force wc_dir_backup/A/C
  #   rm -rf wc_dir_backup/A/C/I wc_dir_backup/A/B/F/nu
  #   svn up wc_dir_backup
  #   svn up -r1 wc_dir_backup/A/C
  #   svn co url/A/C/I wc_dir_backup/A/C/I
  #   svn up --force wc_dir_backup/A/C
  #   """)
  # exit(0)
  url = sbox.repo_url
  wc_dir_backup = sbox.wc_dir + '.backup'

  backup_A_B_F_nu = os.path.join(wc_dir_backup, 'A', 'B', 'F', 'nu')
  backup_A_C = os.path.join(wc_dir_backup, 'A', 'C')
  backup_A_C_I = os.path.join(wc_dir_backup, 'A', 'C', 'I')
  url_A_C_I = url + '/A/C/I'

  # svn up --force wc_dir_backup/A/C
  expected_output = svntest.wc.State(wc_dir_backup, {
    'A/C/I'             : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/F/nu'          : Item(),
    'A/C/I'             :
    Item(contents="This is the file 'I'...shouldn't I be a dir?\n"),
  })

  expected_status = actions.get_virginal_state(wc_dir_backup, 1)
  expected_status.add({
    'A/C/I'             : Item(status='D ', treeconflict='C', wc_rev=2),
    'A/B/F/nu'          : Item(status='D ', treeconflict='C', wc_rev=2),
  })
  expected_status.tweak('A/C', 'A/B/F', wc_rev='2')

  actions.run_and_verify_update(wc_dir_backup, expected_output,
    expected_disk, expected_status, None, None, None, None, None, False,
    '--force', backup_A_C)

  # rm -rf wc_dir_backup/A/C/I wc_dir_backup/A/B/F/nu
  os.remove(backup_A_C_I)
  svntest.main.safe_rmtree(backup_A_B_F_nu)

  svntest.main.run_svn(None, 'revert', backup_A_C_I, backup_A_B_F_nu)

  # svn up wc_dir_backup
  expected_output = svntest.wc.State(wc_dir_backup, {
  })

  expected_disk.tweak('A/B/F/nu', contents="This is the file 'nu'\n")
  expected_disk.tweak('A/C/I', contents=None)

  expected_status.tweak(wc_rev='2', status='  ')
  expected_status.tweak('A/C/I', 'A/B/F/nu', treeconflict=None)

  actions.run_and_verify_update(wc_dir_backup, expected_output,
    expected_disk, expected_status, None, None, None, None, None, False,
    wc_dir_backup)

  # svn up -r1 wc_dir_backup/A/C
  expected_output = svntest.wc.State(wc_dir_backup, {
    'A/C/I'             : Item(status='D '),
  })

  expected_disk.remove('A/C/I')

  expected_status.remove('A/C/I')
  expected_status.tweak('A/C', wc_rev='1')

  actions.run_and_verify_update(wc_dir_backup, expected_output,
    expected_disk, expected_status, None, None, None, None, None, False,
    '-r1', backup_A_C)

  # svn co url/A/C/I wc_dir_backup/A/C/I
  expected_output = svntest.wc.State(wc_dir_backup, {})

  expected_disk = svntest.wc.State(wc_dir, {})

  actions.run_and_verify_checkout2(False, url_A_C_I, backup_A_C_I,
    expected_output, expected_disk, None, None, None, None)

  # svn up --force wc_dir_backup/A/C
  expected_output = svntest.wc.State(wc_dir_backup, {
    'A/C/I'             : Item(verb='Skipped'),
  })

  actions.run_and_verify_update(wc_dir_backup, expected_output, None, None,
    None, None, None, None, None, False, '--force', backup_A_C)


#----------------------------------------------------------------------
# Test for issue #2556. The tests maps a virtual drive to a working copy
# and tries some basic update, commit and status actions on the virtual
# drive.
def update_wc_on_windows_drive(sbox):
  "update wc on the root of a Windows (virtual) drive"

  def find_the_next_available_drive_letter():
    "find the first available drive"

    # get the list of used drive letters, use some Windows specific function.
    try:
      import win32api

      drives=win32api.GetLogicalDriveStrings()
      drives=drives.split('\000')

      for d in range(ord('G'), ord('Z')+1):
        drive = chr(d)
        if not drive + ':\\' in drives:
          return drive
    except ImportError:
      # In ActiveState python x64 win32api is not available
      for d in range(ord('G'), ord('Z')+1):
        drive = chr(d)
        if not os.path.isdir(drive + ':\\'):
          return drive

    return None

  # Skip the test if not on Windows
  if not svntest.main.windows:
    raise svntest.Skip

  # just create an empty folder, we'll checkout later.
  sbox.build(create_wc = False)
  svntest.main.safe_rmtree(sbox.wc_dir)
  os.mkdir(sbox.wc_dir)

  # create a virtual drive to the working copy folder
  drive = find_the_next_available_drive_letter()
  if drive is None:
    raise svntest.Skip

  subprocess.call(['subst', drive +':', sbox.wc_dir])
  wc_dir = drive + ':/'
  was_cwd = os.getcwd()

  try:
    svntest.actions.run_and_verify_svn(None, None, [],
                                       'checkout',
                                       sbox.repo_url, wc_dir)

    # Make some local modifications
    mu_path = os.path.join(wc_dir, 'A', 'mu')
    svntest.main.file_append(mu_path, '\nAppended text for mu')
    zeta_path = os.path.join(wc_dir, 'zeta')
    svntest.main.file_append(zeta_path, "This is the file 'zeta'\n")
    svntest.main.run_svn(None, 'add', zeta_path)

    # Commit.
    expected_output = svntest.wc.State(wc_dir, {
      'A/mu' : Item(verb='Sending'),
      'zeta' : Item(verb='Adding'),
      })

    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.tweak('A/mu', wc_rev=2)
    expected_status.add({
    'zeta' : Item(status='  ', wc_rev=2),
    })

    svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                          expected_status, None,
                                          wc_dir, zeta_path)

    # Non recursive commit
    dir1_path = os.path.join(wc_dir, 'dir1')
    os.mkdir(dir1_path)
    svntest.main.run_svn(None, 'add', '-N', dir1_path)
    file1_path = os.path.join(dir1_path, 'file1')
    svntest.main.file_append(file1_path, "This is the file 'file1'\n")
    svntest.main.run_svn(None, 'add', '-N', file1_path)

    expected_output = svntest.wc.State(wc_dir, {
      'dir1' : Item(verb='Adding'),
      'dir1/file1' : Item(verb='Adding'),
      })

    expected_status.add({
      'dir1' : Item(status='  ', wc_rev=3),
      'dir1/file1' : Item(status='  ', wc_rev=3),
      })

    svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                          expected_status, None,
                                          '-N',
                                          wc_dir,
                                          dir1_path, file1_path)

    # revert to previous revision to test update
    os.chdir(wc_dir)

    expected_disk = svntest.main.greek_state.copy()

    expected_output = svntest.wc.State('', {
      'A/mu' : Item(status='U '),
      'zeta' : Item(status='D '),
      'dir1' : Item(status='D '),
      })

    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

    svntest.actions.run_and_verify_update(wc_dir,
                                          expected_output,
                                          expected_disk,
                                          expected_status,
                                          None, None, None, None, None, 0,
                                          '-r', '1', wc_dir)

    os.chdir(was_cwd)

    # update to the latest version, but use the relative path 'X:'
    wc_dir = drive + ":"

    expected_output = svntest.wc.State(wc_dir, {
      'A/mu' : Item(status='U '),
      'zeta' : Item(status='A '),
      'dir1' : Item(status='A '),
      'dir1/file1' : Item(status='A '),
      })

    expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
    expected_status.add({
      'dir1' : Item(status='  ', wc_rev=3),
      'dir1/file1' : Item(status='  ', wc_rev=3),
      'zeta' : Item(status='  ', wc_rev=3),
      })

    expected_disk.add({
      'zeta'    : Item("This is the file 'zeta'\n"),
      'dir1/file1': Item("This is the file 'file1'\n"),
      })
    expected_disk.tweak('A/mu', contents = expected_disk.desc['A/mu'].contents
                        + '\nAppended text for mu')

    svntest.actions.run_and_verify_update(wc_dir,
                                          expected_output,
                                          expected_disk,
                                          expected_status)

  finally:
    os.chdir(was_cwd)
    # cleanup the virtual drive
    subprocess.call(['subst', '/D', drive +':'])

# Issue #2618: "'Checksum mismatch' error when receiving
# update for replaced-with-history file".
def update_wc_with_replaced_file(sbox):
  "update wc containing a replaced-with-history file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy.
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # we need a change in the repository
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  iota_bu_path = os.path.join(wc_backup, 'iota')
  svntest.main.file_append(iota_bu_path, "New line in 'iota'\n")
  svntest.main.run_svn(None,
                       'ci', wc_backup, '-m', 'changed file')

  # First, a replacement without history.
  svntest.main.run_svn(None, 'rm', iota_path)
  svntest.main.file_append(iota_path, "")
  svntest.main.run_svn(None, 'add', iota_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='R ', wc_rev='1')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now update the wc.  The local replacement is a tree conflict with
  # the incoming edit on that deleted item.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='  ', treeconflict='C'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'iota' : Item(status='R ', wc_rev='2', treeconflict='C'),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents="")

  conflict_files = []

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        conflict_files)

  # Make us a working copy with a 'replace-with-history' file.
  svntest.main.run_svn(None, 'revert', iota_path)

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  expected_disk = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        None, None, None, None, 0,
                                        wc_dir, '-r1')

  svntest.main.run_svn(None, 'rm', iota_path)
  svntest.main.run_svn(None, 'cp', mu_path, iota_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='R ', copied='+', wc_rev='-')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now update the wc.  The local replacement is a tree conflict with
  # the incoming edit on that deleted item.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='  ', treeconflict='C'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'iota' : Item(status='R ', wc_rev='-', treeconflict='C', copied='+'),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents="This is the file 'mu'.\n")

  conflict_files = [ ]

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        conflict_files)

#----------------------------------------------------------------------
def update_with_obstructing_additions(sbox):
  "update handles obstructing paths scheduled for add"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Add files and dirs to the repos via the first WC.  Each of these
  # will be added to the backup WC via an update:
  #
  #  A/B/upsilon:   Identical to the file scheduled for addition in
  #                 the backup WC.
  #
  #  A/C/nu:        A "normal" add, won't exist in the backup WC.
  #
  #  A/D/kappa:     Textual and property conflict with the file scheduled
  #                 for addition in the backup WC.
  #
  #  A/D/epsilon:   Textual conflict with the file scheduled for addition.
  #
  #  A/D/zeta:      Prop conflict with the file scheduled for addition.
  #
  #                 Three new dirs that will also be scheduled for addition:
  #  A/D/H/I:         No props on either WC or REPOS.
  #  A/D/H/I/J:       Prop conflict with the scheduled add.
  #  A/D/H/I/K:       Same (mergeable) prop on WC and REPOS.
  #
  #  A/D/H/I/K/xi:  Identical to the file scheduled for addition in
  #                 the backup WC. No props.
  #
  #  A/D/H/I/L:     A "normal" dir add, won't exist in the backup WC.
  #
  #  A/D/H/I/J/eta: Conflicts with the file scheduled for addition in
  #                 the backup WC.  No props.
  upsilon_path = os.path.join(wc_dir, 'A', 'B', 'upsilon')
  svntest.main.file_append(upsilon_path, "This is the file 'upsilon'\n")
  nu_path = os.path.join(wc_dir, 'A', 'C', 'nu')
  svntest.main.file_append(nu_path, "This is the file 'nu'\n")
  kappa_path = os.path.join(wc_dir, 'A', 'D', 'kappa')
  svntest.main.file_append(kappa_path, "This is REPOS file 'kappa'\n")
  epsilon_path = os.path.join(wc_dir, 'A', 'D', 'epsilon')
  svntest.main.file_append(epsilon_path, "This is REPOS file 'epsilon'\n")
  zeta_path = os.path.join(wc_dir, 'A', 'D', 'zeta')
  svntest.main.file_append(zeta_path, "This is the file 'zeta'\n")
  I_path = os.path.join(wc_dir, 'A', 'D', 'H', 'I')
  os.mkdir(I_path)
  J_path = os.path.join(I_path, 'J')
  os.mkdir(J_path)
  K_path = os.path.join(I_path, 'K')
  os.mkdir(K_path)
  L_path = os.path.join(I_path, 'L')
  os.mkdir(L_path)
  xi_path = os.path.join(K_path, 'xi')
  svntest.main.file_append(xi_path, "This is the file 'xi'\n")
  eta_path = os.path.join(J_path, 'eta')
  svntest.main.file_append(eta_path, "This is REPOS file 'eta'\n")

  svntest.main.run_svn(None, 'add', upsilon_path, nu_path,
                       kappa_path, epsilon_path, zeta_path, I_path)

  # Set props that will conflict with scheduled adds.
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-REPOS',
                       kappa_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-REPOS',
                       zeta_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-REPOS',
                       J_path)

  # Set prop that will match with scheduled add.
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-SAME',
                       epsilon_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-SAME',
                       K_path)

  # Created expected output tree for 'svn ci'
  expected_output = wc.State(wc_dir, {
    'A/B/upsilon'   : Item(verb='Adding'),
    'A/C/nu'        : Item(verb='Adding'),
    'A/D/kappa'     : Item(verb='Adding'),
    'A/D/epsilon'   : Item(verb='Adding'),
    'A/D/zeta'      : Item(verb='Adding'),
    'A/D/H/I'       : Item(verb='Adding'),
    'A/D/H/I/J'     : Item(verb='Adding'),
    'A/D/H/I/J/eta' : Item(verb='Adding'),
    'A/D/H/I/K'     : Item(verb='Adding'),
    'A/D/H/I/K/xi'  : Item(verb='Adding'),
    'A/D/H/I/L'     : Item(verb='Adding'),
    })

  # Create expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/upsilon'   : Item(status='  ', wc_rev=2),
    'A/C/nu'        : Item(status='  ', wc_rev=2),
    'A/D/kappa'     : Item(status='  ', wc_rev=2),
    'A/D/epsilon'   : Item(status='  ', wc_rev=2),
    'A/D/zeta'      : Item(status='  ', wc_rev=2),
    'A/D/H/I'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/J/eta' : Item(status='  ', wc_rev=2),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=2),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=2),
    })

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create various paths scheduled for addition which will obstruct
  # the adds coming from the repos.
  upsilon_backup_path = os.path.join(wc_backup, 'A', 'B', 'upsilon')
  svntest.main.file_append(upsilon_backup_path,
                           "This is the file 'upsilon'\n")
  kappa_backup_path = os.path.join(wc_backup, 'A', 'D', 'kappa')
  svntest.main.file_append(kappa_backup_path,
                           "This is WC file 'kappa'\n")
  epsilon_backup_path = os.path.join(wc_backup, 'A', 'D', 'epsilon')
  svntest.main.file_append(epsilon_backup_path,
                           "This is WC file 'epsilon'\n")
  zeta_backup_path = os.path.join(wc_backup, 'A', 'D', 'zeta')
  svntest.main.file_append(zeta_backup_path, "This is the file 'zeta'\n")
  I_backup_path = os.path.join(wc_backup, 'A', 'D', 'H', 'I')
  os.mkdir(I_backup_path)
  J_backup_path = os.path.join(I_backup_path, 'J')
  os.mkdir(J_backup_path)
  K_backup_path = os.path.join(I_backup_path, 'K')
  os.mkdir(K_backup_path)
  xi_backup_path = os.path.join(K_backup_path, 'xi')
  svntest.main.file_append(xi_backup_path, "This is the file 'xi'\n")
  eta_backup_path = os.path.join(J_backup_path, 'eta')
  svntest.main.file_append(eta_backup_path, "This is WC file 'eta'\n")

  svntest.main.run_svn(None, 'add', upsilon_backup_path, kappa_backup_path,
                       epsilon_backup_path, zeta_backup_path, I_backup_path)

  # Set prop that will conflict with add from repos.
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-WC',
                       kappa_backup_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-WC',
                       zeta_backup_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-WC',
                       J_backup_path)

  # Set prop that will match add from repos.
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-SAME',
                       epsilon_backup_path)
  svntest.main.run_svn(None, 'propset', 'propname1', 'propval-SAME',
                       K_backup_path)

  # Create expected output tree for an update of the wc_backup.
  expected_output = wc.State(wc_backup, {
    'A/B/upsilon'   : Item(status='E '),
    'A/C/nu'        : Item(status='A '),
    'A/D/H/I'       : Item(status='E '),
    'A/D/H/I/J'     : Item(status='EC'),
    'A/D/H/I/J/eta' : Item(status='C '),
    'A/D/H/I/K'     : Item(status='EG'),
    'A/D/H/I/K/xi'  : Item(status='E '),
    'A/D/H/I/L'     : Item(status='A '),
    'A/D/kappa'     : Item(status='CC'),
    'A/D/epsilon'   : Item(status='CG'),
    'A/D/zeta'      : Item(status='EC'),
    })

  # Create expected disk for update of wc_backup.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/upsilon'   : Item("This is the file 'upsilon'\n"),
    'A/C/nu'        : Item("This is the file 'nu'\n"),
    'A/D/H/I'       : Item(),
    'A/D/H/I/J'     : Item(props={'propname1' : 'propval-WC'}),
    'A/D/H/I/J/eta' : Item("\n".join(["<<<<<<< .mine",
                                      "This is WC file 'eta'",
                                      "=======",
                                      "This is REPOS file 'eta'",
                                      ">>>>>>> .r2",
                                      ""])),
    'A/D/H/I/K'     : Item(props={'propname1' : 'propval-SAME'}),
    'A/D/H/I/K/xi'  : Item("This is the file 'xi'\n"),
    'A/D/H/I/L'     : Item(),
    'A/D/kappa'     : Item("\n".join(["<<<<<<< .mine",
                                      "This is WC file 'kappa'",
                                      "=======",
                                      "This is REPOS file 'kappa'",
                                      ">>>>>>> .r2",
                                      ""]),
                           props={'propname1' : 'propval-WC'}),
    'A/D/epsilon'     : Item("\n".join(["<<<<<<< .mine",
                                        "This is WC file 'epsilon'",
                                        "=======",
                                        "This is REPOS file 'epsilon'",
                                        ">>>>>>> .r2",
                                        ""]),
                             props={'propname1' : 'propval-SAME'}),
    'A/D/zeta'   : Item("This is the file 'zeta'\n",
                        props={'propname1' : 'propval-WC'}),
    })

  # Create expected status tree for the update.  Since the obstructing
  # kappa and upsilon differ from the repos, they should show as modified.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.add({
    'A/B/upsilon'   : Item(status='  ', wc_rev=2),
    'A/C/nu'        : Item(status='  ', wc_rev=2),
    'A/D/H/I'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'     : Item(status=' C', wc_rev=2),
    'A/D/H/I/J/eta' : Item(status='C ', wc_rev=2),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=2),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=2),
    'A/D/kappa'     : Item(status='CC', wc_rev=2),
    'A/D/epsilon'   : Item(status='C ', wc_rev=2),
    'A/D/zeta'      : Item(status=' C', wc_rev=2),
    })

  # "Extra" files that we expect to result from the conflicts.
  extra_files = ['eta\.r0', 'eta\.r2', 'eta\.mine',
                 'kappa\.r0', 'kappa\.r2', 'kappa\.mine',
                 'epsilon\.r0', 'epsilon\.r2', 'epsilon\.mine',
                 'kappa.prej', 'zeta.prej', 'dir_conflicts.prej']

  # Perform forced update and check the results in three
  # ways (including props).
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files, None, None, 1,
                                        wc_backup)

  # Some obstructions are still not permitted:
  #
  # Test that file and dir obstructions scheduled for addition *with*
  # history fail when update tries to add the same path.

  # URL to URL copy of A/D/G to A/M.
  G_URL = sbox.repo_url + '/A/D/G'
  M_URL = sbox.repo_url + '/A/M'
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', G_URL, M_URL, '-m', '')

  # WC to WC copy of A/D/H to A/M, M now scheduled for addition with
  # history in WC and pending addition from the repos.
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  A_path = os.path.join(wc_dir, 'A')
  M_path = os.path.join(wc_dir, 'A', 'M')

  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', H_path, M_path)

  # URL to URL copy of A/D/H/omega to omicron.
  omega_URL = sbox.repo_url + '/A/D/H/omega'
  omicron_URL = sbox.repo_url + '/omicron'
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', omega_URL, omicron_URL,
                                     '-m', '')

  # WC to WC copy of A/D/H/chi to omicron, omicron now scheduled for
  # addition with history in WC and pending addition from the repos.
  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  omicron_path = os.path.join(wc_dir, 'omicron')

  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', chi_path,
                                     omicron_path)

  # Try to update M's Parent.
  expected_output = wc.State(A_path, {
    'M'      : Item(status='  ', treeconflict='C'),
    'M/rho'  : Item(status='  ', treeconflict='A'),
    'M/pi'   : Item(status='  ', treeconflict='A'),
    'M/tau'  : Item(status='  ', treeconflict='A'),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/upsilon'   : Item("This is the file 'upsilon'\n"),
    'A/C/nu'        : Item("This is the file 'nu'\n"),
    'A/D/H/I'       : Item(),
    'A/D/H/I/J'     : Item(),
    'A/D/H/I/J/eta' : Item("This is REPOS file 'eta'\n"),
    'A/D/H/I/K'     : Item(),
    'A/D/H/I/K/xi'  : Item("This is the file 'xi'\n"),
    'A/D/H/I/L'     : Item(),
    'A/D/kappa'     : Item("This is REPOS file 'kappa'\n"),
    'A/D/epsilon'   : Item("This is REPOS file 'epsilon'\n"),
    'A/D/gamma'     : Item("This is the file 'gamma'.\n"),
    'A/D/zeta'      : Item("This is the file 'zeta'\n"),
    'A/M/I'         : Item(),
    'A/M/I/J'       : Item(),
    'A/M/I/J/eta'   : Item("This is REPOS file 'eta'\n"),
    'A/M/I/K'       : Item(),
    'A/M/I/K/xi'    : Item("This is the file 'xi'\n"),
    'A/M/I/L'       : Item(),
    'A/M/chi'       : Item("This is the file 'chi'.\n"),
    'A/M/psi'       : Item("This is the file 'psi'.\n"),
    'A/M/omega'     : Item("This is the file 'omega'.\n"),
    'omicron'       : Item("This is the file 'chi'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 4)
  expected_status.tweak('', 'iota', wc_rev=1)
  expected_status.add({
    'A/B/upsilon'   : Item(status='  ', wc_rev=4),
    'A/C/nu'        : Item(status='  ', wc_rev=4),
    'A/D/kappa'     : Item(status='  ', wc_rev=4),
    'A/D/epsilon'   : Item(status='  ', wc_rev=4),
    'A/D/gamma'     : Item(status='  ', wc_rev=4),
    'A/D/zeta'      : Item(status='  ', wc_rev=4),
    'A/D/H/I'       : Item(status='  ', wc_rev=4),
    'A/D/H/I/J'     : Item(status='  ', wc_rev=4),
    'A/D/H/I/J/eta' : Item(status='  ', wc_rev=4),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=4),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=4),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=4),
    'A/M'           : Item(status='R ', copied='+', wc_rev='-',
                           treeconflict='C'),
    'A/M/I'         : Item(status='A ', copied='+', wc_rev='-',
                           entry_status='  '), # New op_root
    'A/M/I/J'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/I/J/eta'   : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/I/K'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/I/K/xi'    : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/I/L'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/chi'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/psi'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/M/omega'     : Item(status='  ', copied='+', wc_rev='-'),
    'omicron'       : Item(status='A ', copied='+', wc_rev='-'),

    # Inserted under the tree conflict
    'A/M/pi'            : Item(status='D ', wc_rev='4'),
    'A/M/rho'           : Item(status='D ', wc_rev='4'),
    'A/M/tau'           : Item(status='D ', wc_rev='4'),
    })

  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, False,
                                        A_path)

  # Resolve the tree conflict.
  svntest.main.run_svn(None, 'resolve', '--accept', 'working', M_path)

  # Try to update omicron's parent, non-recusively so as not to
  # try and update M first.
  expected_output = wc.State(wc_dir, {
    'omicron'   : Item(status='  ', treeconflict='C'),
    })

  expected_status.tweak('', 'iota', status='  ', wc_rev=4)
  expected_status.tweak('omicron', status='R ', copied='+', wc_rev='-',
                        treeconflict='C')
  expected_status.tweak('A/M', treeconflict=None)

  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, False,
                                        wc_dir, '-N')

  # Resolve the tree conflict.
  svntest.main.run_svn(None, 'resolved', omicron_path)

  expected_output = wc.State(wc_dir, { })

  expected_status.tweak('omicron', treeconflict=None)

  # Again, --force shouldn't matter.
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, False,
                                        omicron_path, '-N', '--force')

# Test for issue #2022: Update shouldn't touch conflicted files.
def update_conflicted(sbox):
  "update conflicted files"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  D_path = os.path.join(wc_dir, 'A', 'D')
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Make some modifications to the files and a dir, creating r2.
  svntest.main.file_append(iota_path, 'Original appended text for iota\n')

  svntest.main.run_svn(None, 'propset', 'prop', 'val', lambda_path)

  svntest.main.file_append(mu_path, 'Original appended text for mu\n')

  svntest.main.run_svn(None, 'propset', 'prop', 'val', mu_path)
  svntest.main.run_svn(None, 'propset', 'prop', 'val', D_path)

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/mu': Item(verb='Sending'),
    'A/B/lambda': Item(verb='Sending'),
    'A/D': Item(verb='Sending'),
    })

  expected_status.tweak('iota', 'A/mu', 'A/B/lambda', 'A/D', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Do another change to each path that we will need later.
  # Also, change a file below A/D in the path.
  svntest.main.file_append(iota_path, 'Another line for iota\n')
  svntest.main.file_append(mu_path, 'Another line for mu\n')
  svntest.main.file_append(lambda_path, 'Another line for lambda\n')

  svntest.main.run_svn(None, 'propset', 'prop', 'val2', D_path)

  svntest.main.file_append(pi_path, 'Another line for pi\n')

  expected_status.tweak('iota', 'A/mu', 'A/B/lambda', 'A/D', 'A/D/G/pi',
                        wc_rev=3)

  expected_output.add({
    'A/D/G/pi': Item(verb='Sending')})

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Go back to revision 1.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    'A/B/lambda' : Item(status='UU'),
    'A/mu' : Item(status='UU'),
    'A/D': Item(status=' U'),
    'A/D/G/pi': Item(status='U '),
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        None, None,
                                        None, None, 1,
                                        '-r1', wc_dir)

  # Create modifications conflicting with rev 2.
  svntest.main.file_append(iota_path, 'Conflicting appended text for iota\n')
  svntest.main.run_svn(None, 'propset', 'prop', 'conflictval', lambda_path)
  svntest.main.file_append(mu_path, 'Conflicting appended text for mu\n')
  svntest.main.run_svn(None, 'propset', 'prop', 'conflictval', mu_path)
  svntest.main.run_svn(None, 'propset', 'prop', 'conflictval', D_path)

  # Update to revision 2, expecting conflicts.
  expected_output = svntest.wc.State(wc_dir, {
    'iota': Item(status='C '),
    'A/B/lambda': Item(status=' C'),
    'A/mu': Item(status='CC'),
    'A/D': Item(status=' C'),
    })

  expected_disk.tweak('iota',
                      contents="\n".join(["This is the file 'iota'.",
                                          "<<<<<<< .mine",
                                          "Conflicting appended text for iota",
                                          "=======",
                                          "Original appended text for iota",
                                          ">>>>>>> .r2",
                                          ""]))
  expected_disk.tweak('A/mu',
                      contents="\n".join(["This is the file 'mu'.",
                                          "<<<<<<< .mine",
                                          "Conflicting appended text for mu",
                                          "=======",
                                          "Original appended text for mu",
                                          ">>>>>>> .r2",
                                          ""]),
                      props={'prop': 'conflictval'})
  expected_disk.tweak('A/B/lambda', 'A/D', props={'prop': 'conflictval'})

  expected_status.tweak(wc_rev=2)
  expected_status.tweak('iota', status='C ')
  expected_status.tweak('A/B/lambda', 'A/D', status=' C')
  expected_status.tweak('A/mu', status='CC')

  extra_files = [ [wc_dir, 'iota.*\.(r1|r2|mine)'],
                  [wc_dir, 'mu.*\.(r1|r2|mine|prej)'],
                  [wc_dir, 'lambda.*\.prej'],
                  [wc_dir, 'dir_conflicts.prej']]

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        detect_extra_files, extra_files,
                                        None, None, 1,
                                        '-r2', wc_dir)

  # Now, update to HEAD, which should skip all the conflicted files, but
  # still update the pi file.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Skipped'),
    'A/B/lambda' : Item(verb='Skipped'),
    'A/mu' : Item(verb='Skipped'),
    'A/D' : Item(verb='Skipped'),
    })

  expected_status.tweak(wc_rev=3)
  expected_status.tweak('iota', 'A/B/lambda', 'A/mu', 'A/D', wc_rev=2)
  # We no longer update descendants of a prop-conflicted dir.
  expected_status.tweak('A/D/G',
                        'A/D/G/pi',
                        'A/D/G/rho',
                        'A/D/G/tau',
                        'A/D/H',
                        'A/D/H/chi',
                        'A/D/H/omega',
                        'A/D/H/psi',
                        'A/D/gamma', wc_rev=2)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        detect_extra_files, extra_files,
                                        None, None, 1)

#----------------------------------------------------------------------
@SkipUnless(server_has_mergeinfo)
def mergeinfo_update_elision(sbox):
  "mergeinfo does not elide after update"

  # No mergeinfo elision is performed when doing updates.  So updates may
  # result in equivalent mergeinfo on a path and it's nearest working copy
  # parent with explicit mergeinfo.  This is currently permitted and
  # honestly we could probably do without this test(?).

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  alpha_COPY_path = os.path.join(wc_dir, "A", "B_COPY", "E", "alpha")
  alpha_path  = os.path.join(wc_dir, "A", "B", "E", "alpha")
  B_COPY_path = os.path.join(wc_dir, "A", "B_COPY")
  E_COPY_path = os.path.join(wc_dir, "A", "B_COPY", "E")
  beta_path   = os.path.join(wc_dir, "A", "B", "E", "beta")
  lambda_path = os.path.join(wc_dir, "A", "B", "lambda")

  # Make a branch A/B_COPY
  expected_stdout =  verify.UnorderedOutput([
     "A    " + os.path.join(wc_dir, "A", "B_COPY", "lambda") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY", "E") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY", "E", "alpha") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY", "E", "beta") + "\n",
     "A    " + os.path.join(wc_dir, "A", "B_COPY", "F") + "\n",
     "Checked out revision 1.\n",
     "A         " + B_COPY_path + "\n",
    ])
  svntest.actions.run_and_verify_svn(None, expected_stdout, [], 'copy',
                                     sbox.repo_url + "/A/B", B_COPY_path)

  expected_output = wc.State(wc_dir, {'A/B_COPY' : Item(verb='Adding')})

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    "A/B_COPY"         : Item(status='  ', wc_rev=2),
    "A/B_COPY/lambda"  : Item(status='  ', wc_rev=2),
    "A/B_COPY/E"       : Item(status='  ', wc_rev=2),
    "A/B_COPY/E/alpha" : Item(status='  ', wc_rev=2),
    "A/B_COPY/E/beta"  : Item(status='  ', wc_rev=2),
    "A/B_COPY/F"       : Item(status='  ', wc_rev=2),})

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Make some changes under A/B

  # r3 - modify and commit A/B/E/beta
  svntest.main.file_write(beta_path, "New content")

  expected_output = wc.State(wc_dir, {'A/B/E/beta' : Item(verb='Sending')})

  expected_status.tweak('A/B/E/beta', wc_rev=3)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # r4 - modify and commit A/B/lambda
  svntest.main.file_write(lambda_path, "New content")

  expected_output = wc.State(wc_dir, {'A/B/lambda' : Item(verb='Sending')})

  expected_status.tweak('A/B/lambda', wc_rev=4)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # r5 - modify and commit A/B/E/alpha
  svntest.main.file_write(alpha_path, "New content")

  expected_output = wc.State(wc_dir, {'A/B/E/alpha' : Item(verb='Sending')})

  expected_status.tweak('A/B/E/alpha', wc_rev=5)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Merge r2:5 into A/B_COPY
  expected_output = wc.State(B_COPY_path, {
    'lambda'  : Item(status='U '),
    'E/alpha' : Item(status='U '),
    'E/beta'  : Item(status='U '),
    })

  expected_mergeinfo_output = wc.State(B_COPY_path, {
    '' : Item(status=' U'),
    })

  expected_elision_output = wc.State(B_COPY_path, {
    })

  expected_merge_status = wc.State(B_COPY_path, {
    ''        : Item(status=' M', wc_rev=2),
    'lambda'  : Item(status='M ', wc_rev=2),
    'E'       : Item(status='  ', wc_rev=2),
    'E/alpha' : Item(status='M ', wc_rev=2),
    'E/beta'  : Item(status='M ', wc_rev=2),
    'F'       : Item(status='  ', wc_rev=2),
    })

  expected_merge_disk = wc.State('', {
    ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B:3-5'}),
    'lambda'  : Item("New content"),
    'E'       : Item(),
    'E/alpha' : Item("New content"),
    'E/beta'  : Item("New content"),
    'F'       : Item(),
    })

  expected_skip = wc.State(B_COPY_path, { })

  svntest.actions.run_and_verify_merge(B_COPY_path, '2', '5',
                                       sbox.repo_url + '/A/B', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_merge_disk,
                                       expected_merge_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1)

  # r6 - Commit the merge
  expected_output = wc.State(wc_dir,
                             {'A/B_COPY'         : Item(verb='Sending'),
                              'A/B_COPY/E/alpha' : Item(verb='Sending'),
                              'A/B_COPY/E/beta'  : Item(verb='Sending'),
                              'A/B_COPY/lambda'  : Item(verb='Sending')})

  expected_status.tweak('A/B_COPY',         wc_rev=6)
  expected_status.tweak('A/B_COPY/E/alpha', wc_rev=6)
  expected_status.tweak('A/B_COPY/E/beta',  wc_rev=6)
  expected_status.tweak('A/B_COPY/lambda',  wc_rev=6)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update WC back to r5, A/COPY_B is at it's pre-merge state again
  expected_output = wc.State(wc_dir,
                             {'A/B_COPY'         : Item(status=' U'),
                              'A/B_COPY/E/alpha' : Item(status='U '),
                              'A/B_COPY/E/beta'  : Item(status='U '),
                              'A/B_COPY/lambda'  : Item(status='U '),})

  expected_status.tweak(wc_rev=5)

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B_COPY'         : Item(),
    'A/B_COPY/lambda'  : Item("This is the file 'lambda'.\n"),
    'A/B_COPY/E'       : Item(),
    'A/B_COPY/E/alpha' : Item("This is the file 'alpha'.\n"),
    'A/B_COPY/E/beta'  : Item("This is the file 'beta'.\n"),
    'A/B_COPY/F'       : Item(),
    })
  expected_disk.tweak('A/B/lambda',  contents="New content")
  expected_disk.tweak('A/B/E/alpha', contents="New content")
  expected_disk.tweak('A/B/E/beta',  contents="New content")

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '5', wc_dir)

  # Merge r2:5 to A/B_COPY/E/alpha
  expected_output = wc.State(alpha_COPY_path, {
    'alpha' : Item(status='U '),
    })
  expected_skip = wc.State(alpha_COPY_path, { })

  # run_and_verify_merge doesn't support merging to a file WCPATH
  # so use run_and_verify_svn.
  svntest.actions.run_and_verify_svn(None,
                                     expected_merge_output([[3,5]],
                                     ['U    ' + alpha_COPY_path + '\n',
                                      ' U   ' + alpha_COPY_path + '\n']),
                                     [], 'merge', '-r2:5',
                                     sbox.repo_url + '/A/B/E/alpha',
                                     alpha_COPY_path)


  expected_alpha_status = wc.State(alpha_COPY_path, {
    ''        : Item(status='MM', wc_rev=5),
    })

  svntest.actions.run_and_verify_status(alpha_COPY_path,
                                        expected_alpha_status)

  svntest.actions.run_and_verify_svn(None, ["/A/B/E/alpha:3-5\n"], [],
                                     'propget', SVN_PROP_MERGEINFO,
                                     alpha_COPY_path)

  # Update WC.  The local mergeinfo (r3-5) on A/B_COPY/E/alpha is
  # identical to that on added to A/B_COPY by the update, but update
  # doesn't support elision so this redundancy is permitted.
  expected_output = wc.State(wc_dir, {
    'A/B_COPY/lambda'  : Item(status='U '),
    'A/B_COPY/E/alpha' : Item(status='G '),
    'A/B_COPY/E/beta'  : Item(status='U '),
    'A/B_COPY'         : Item(status=' U'),
    })

  expected_disk.tweak('A/B_COPY', props={SVN_PROP_MERGEINFO : '/A/B:3-5'})
  expected_disk.tweak('A/B_COPY/lambda', contents="New content")
  expected_disk.tweak('A/B_COPY/E/beta', contents="New content")
  expected_disk.tweak('A/B_COPY/E/alpha', contents="New content",
                      props={SVN_PROP_MERGEINFO : '/A/B/E/alpha:3-5'})

  expected_status.tweak(wc_rev=6)
  expected_status.tweak('A/B_COPY/E/alpha', status=' M')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1)

  # Now test that an updated target's mergeinfo can itself elide.
  # r7 - modify and commit A/B/E/alpha
  svntest.main.file_write(alpha_path, "More new content")
  expected_output = wc.State(wc_dir, {
    'A/B/E/alpha' : Item(verb='Sending'),
    'A/B_COPY/E/alpha' : Item(verb='Sending')})
  expected_status.tweak('A/B/E/alpha', 'A/B_COPY/E/alpha', status='  ',
                        wc_rev=7)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update A to get all paths to the same working revision.
  svntest.actions.run_and_verify_svn(None, exp_noop_up_out(7), [],
                                     'up', wc_dir)

  # Merge r6:7 into A/B_COPY/E
  expected_output = wc.State(E_COPY_path, {
    'alpha' : Item(status='U '),
    })

  expected_mergeinfo_output = wc.State(E_COPY_path, {
    ''      : Item(status=' G'),
    'alpha' : Item(status=' U'),
    })

  expected_elision_output = wc.State(E_COPY_path, {
    'alpha' : Item(status=' U'),
    })

  expected_merge_status = wc.State(E_COPY_path, {
    ''        : Item(status=' M', wc_rev=7),
    'alpha' : Item(status='MM', wc_rev=7),
    'beta'  : Item(status='  ', wc_rev=7),
    })

  expected_merge_disk = wc.State('', {
    ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B/E:3-5,7'}),
    'alpha' : Item("More new content"),
    'beta'  : Item("New content"),
    })

  expected_skip = wc.State(E_COPY_path, { })

  svntest.actions.run_and_verify_merge(E_COPY_path, '6', '7',
                                       sbox.repo_url + '/A/B/E', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_merge_disk,
                                       expected_merge_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1)

  # r8 - Commit the merge
  svntest.actions.run_and_verify_svn(None,
                                     exp_noop_up_out(7),
                                     [], 'update', wc_dir)

  expected_output = wc.State(wc_dir,
                             {'A/B_COPY/E'       : Item(verb='Sending'),
                              'A/B_COPY/E/alpha' : Item(verb='Sending')})

  expected_status.tweak(wc_rev=7)
  expected_status.tweak('A/B_COPY/E', 'A/B_COPY/E/alpha', wc_rev=8)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update A/COPY_B/E back to r7
  expected_output = wc.State(wc_dir, {
    'A/B_COPY/E/alpha' : Item(status='UU'),
    'A/B_COPY/E'       : Item(status=' U'),
    })

  expected_status.tweak(wc_rev=7)

  expected_disk.tweak('A/B_COPY',
                      props={SVN_PROP_MERGEINFO : '/A/B:3-5'})
  expected_disk.tweak('A/B/E/alpha', contents="More new content")
  expected_disk.tweak('A/B_COPY/E/alpha', contents="New content")

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '7', E_COPY_path)

  # Merge r6:7 to A/B_COPY
  expected_output = wc.State(B_COPY_path, {
    'E/alpha' : Item(status='U '),
    })

  expected_mergeinfo_output = wc.State(B_COPY_path, {
    ''        : Item(status=' U'),
    'E/alpha' : Item(status=' U'),
    })

  expected_elision_output = wc.State(B_COPY_path, {
    'E/alpha' : Item(status=' U'),
    })

  expected_merge_status = wc.State(B_COPY_path, {
    ''        : Item(status=' M', wc_rev=7),
    'lambda'  : Item(status='  ', wc_rev=7),
    'E'       : Item(status='  ', wc_rev=7),
    'E/alpha' : Item(status='MM', wc_rev=7),
    'E/beta'  : Item(status='  ', wc_rev=7),
    'F'       : Item(status='  ', wc_rev=7),
    })

  expected_merge_disk = wc.State('', {
    ''        : Item(props={SVN_PROP_MERGEINFO : '/A/B:3-5,7'}),
    'lambda'  : Item("New content"),
    'E'       : Item(),
    'E/alpha' : Item("More new content"),
    'E/beta'  : Item("New content"),
    'F'       : Item(),
    })

  expected_skip = wc.State(B_COPY_path, { })

  svntest.actions.run_and_verify_merge(B_COPY_path, '6', '7',
                                       sbox.repo_url + '/A/B', None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_merge_disk,
                                       expected_merge_status,
                                       expected_skip,
                                       None, None, None, None,
                                       None, 1,alpha_COPY_path)

  # Update just A/B_COPY/E.  The mergeinfo (r3-5,7) reset on
  # A/B_COPY/E by the udpate is identical to the local info on
  # A/B_COPY, so should elide, leaving no mereginfo on E.
  expected_output = wc.State(wc_dir, {
    'A/B_COPY/E/alpha' : Item(status='GG'),
    'A/B_COPY/E/'      : Item(status=' U'),
    })

  expected_status.tweak('A/B_COPY', status=' M', wc_rev=7)
  expected_status.tweak('A/B_COPY/E', status='  ', wc_rev=8)
  expected_status.tweak('A/B_COPY/E/alpha', wc_rev=8)
  expected_status.tweak('A/B_COPY/E/beta', wc_rev=8)

  expected_disk.tweak('A/B_COPY',
                      props={SVN_PROP_MERGEINFO : '/A/B:3-5,7'})
  expected_disk.tweak('A/B_COPY/E',
                      props={SVN_PROP_MERGEINFO : '/A/B/E:3-5,7'})
  expected_disk.tweak('A/B_COPY/E/alpha', contents="More new content",
                      props={})

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1, E_COPY_path)


#----------------------------------------------------------------------
# Very obscure bug: Issue #2977.
# Let's say there's a revision with
#   $ svn mv b c
#   $ svn mv a b
#   $ svn ci
# and a later revision that modifies b.  We then try a fresh checkout.  If
# the server happens to send us 'b' first, then when it later gets 'c'
# (with a copyfrom of 'b') it might try to use the 'b' in the wc as the
# copyfrom base.  This is wrong, because 'b' was changed later; however,
# due to a bug, the setting of svn:entry:committed-rev on 'b' is not being
# properly seen by the client, and it chooses the wrong base.  Corruption!
#
# Note that because this test depends on the order that the server sends
# changes, it is very fragile; even changing the file names can avoid
# triggering the bug.

def update_copied_from_replaced_and_changed(sbox):
  "update chooses right copyfrom for double move"

  sbox.build()
  wc_dir = sbox.wc_dir

  fn1_relpath = os.path.join('A', 'B', 'E', 'aardvark')
  fn2_relpath = os.path.join('A', 'B', 'E', 'alpha')
  fn3_relpath = os.path.join('A', 'B', 'E', 'beta')
  fn1_path = os.path.join(wc_dir, fn1_relpath)
  fn2_path = os.path.join(wc_dir, fn2_relpath)
  fn3_path = os.path.join(wc_dir, fn3_relpath)

  # Move fn2 to fn1
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', fn2_path, fn1_path)

  # Move fn3 to fn2
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', fn3_path, fn2_path)

  # Commit that change, creating r2.
  expected_output = svntest.wc.State(wc_dir, {
    fn1_relpath : Item(verb='Adding'),
    fn2_relpath : Item(verb='Replacing'),
    fn3_relpath : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove(fn2_relpath, fn3_relpath)
  expected_status.add({
    fn1_relpath : Item(status='  ', wc_rev=2),
    fn2_relpath : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Modify fn2.
  fn2_final_contents = "I have new contents for the middle file."
  svntest.main.file_write(fn2_path, fn2_final_contents)

  # Commit the changes, creating r3.
  expected_output = svntest.wc.State(wc_dir, {
    fn2_relpath : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove(fn2_relpath, fn3_relpath)
  expected_status.add({
    fn1_relpath : Item(status='  ', wc_rev=2),
    fn2_relpath : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Go back to r1.
  expected_output = svntest.wc.State(wc_dir, {
    fn1_relpath: Item(status='D '),
    fn2_relpath: Item(status='A '), # though actually should be D and A
    fn3_relpath: Item(status='A '),
    })

  # Create expected disk tree for the update to rev 0
  expected_disk = svntest.main.greek_state.copy()

  # Do the update and check the results.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        None, None, None, None, 0,
                                        '-r', '1', wc_dir)

  # And back up to 3 again.
  expected_output = svntest.wc.State(wc_dir, {
    fn1_relpath: Item(status='A '),
    fn2_relpath: Item(status='A '), # though actually should be D and A
    fn3_relpath: Item(status='D '),
    })

  # Create expected disk tree for the update to rev 0
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    fn1_relpath : Item("This is the file 'alpha'.\n"),
    })
  expected_disk.tweak(fn2_relpath, contents=fn2_final_contents)
  expected_disk.remove(fn3_relpath)

  # reuse old expected_status, but at r3
  expected_status.tweak(wc_rev=3)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status, None,
                                        None, None, None, None, 0,
                                        wc_dir)

#----------------------------------------------------------------------
# Regression test: ra_neon assumes that you never delete a property on
# a newly-added file, which is wrong if it's add-with-history.
def update_copied_and_deleted_prop(sbox):
  "updating a copied file with a deleted property"

  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  iota2_path = os.path.join(wc_dir, 'iota2')

  # Add a property on iota
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'bar', iota_path)
  # Commit that change, creating r2.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status_mixed = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status_mixed.tweak('iota', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status_mixed, None, wc_dir)

  # Copy iota to iota2 and delete the property on it.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', iota_path, iota2_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propdel', 'foo', iota2_path)

  # Commit that change, creating r3.
  expected_output = svntest.wc.State(wc_dir, {
    'iota2' : Item(verb='Adding'),
    })

  expected_status_mixed.add({
    'iota2' : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status_mixed, None, wc_dir)

  # Update the whole wc, verifying disk as well.
  expected_output = svntest.wc.State(wc_dir, { })

  expected_disk_r3 = svntest.main.greek_state.copy()
  expected_disk_r3.add({
    'iota2' : Item("This is the file 'iota'.\n"),
    })
  expected_disk_r3.tweak('iota', props={'foo':'bar'})

  expected_status_r3 = expected_status_mixed.copy()
  expected_status_r3.tweak(wc_rev=3)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk_r3,
                                        expected_status_r3,
                                        check_props=True)

  # Now go back to r2.
  expected_output = svntest.wc.State(wc_dir, {'iota2': Item(status='D ')})

  expected_disk_r2 = expected_disk_r3.copy()
  expected_disk_r2.remove('iota2')

  expected_status_r2 = expected_status_r3.copy()
  expected_status_r2.tweak(wc_rev=2)
  expected_status_r2.remove('iota2')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk_r2,
                                        expected_status_r2,
                                        None, None, None, None, None,
                                        True,
                                        "-r2", wc_dir)

  # And finally, back to r3, getting an add-with-history-and-property-deleted
  expected_output = svntest.wc.State(wc_dir, {'iota2': Item(status='A ')})

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk_r3,
                                        expected_status_r3,
                                        check_props=True)

#----------------------------------------------------------------------


def update_accept_conflicts(sbox):
  "update --accept automatic conflict resolution"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Make a few local mods to files which will be committed
  iota_path = os.path.join(wc_dir, 'iota')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  beta_path = os.path.join(wc_dir, 'A', 'B', 'E', 'beta')
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(lambda_path, 'Their appended text for lambda\n')
  svntest.main.file_append(iota_path, 'Their appended text for iota\n')
  svntest.main.file_append(mu_path, 'Their appended text for mu\n')
  svntest.main.file_append(alpha_path, 'Their appended text for alpha\n')
  svntest.main.file_append(beta_path, 'Their appended text for beta\n')
  svntest.main.file_append(pi_path, 'Their appended text for pi\n')
  svntest.main.file_append(rho_path, 'Their appended text for rho\n')

  # Make a few local mods to files which will be conflicted
  iota_path_backup = os.path.join(wc_backup, 'iota')
  lambda_path_backup = os.path.join(wc_backup, 'A', 'B', 'lambda')
  mu_path_backup = os.path.join(wc_backup, 'A', 'mu')
  alpha_path_backup = os.path.join(wc_backup, 'A', 'B', 'E', 'alpha')
  beta_path_backup = os.path.join(wc_backup, 'A', 'B', 'E', 'beta')
  pi_path_backup = os.path.join(wc_backup, 'A', 'D', 'G', 'pi')
  rho_path_backup = os.path.join(wc_backup, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(iota_path_backup,
                           'My appended text for iota\n')
  svntest.main.file_append(lambda_path_backup,
                           'My appended text for lambda\n')
  svntest.main.file_append(mu_path_backup,
                           'My appended text for mu\n')
  svntest.main.file_append(alpha_path_backup,
                           'My appended text for alpha\n')
  svntest.main.file_append(beta_path_backup,
                           'My appended text for beta\n')
  svntest.main.file_append(pi_path_backup,
                           'My appended text for pi\n')
  svntest.main.file_append(rho_path_backup,
                           'My appended text for rho\n')

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/B/lambda' : Item(verb='Sending'),
    'A/mu' : Item(verb='Sending'),
    'A/B/E/alpha': Item(verb='Sending'),
    'A/B/E/beta': Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    'A/D/G/rho' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  expected_status.tweak('A/B/lambda', wc_rev=2)
  expected_status.tweak('A/mu', wc_rev=2)
  expected_status.tweak('A/B/E/alpha', wc_rev=2)
  expected_status.tweak('A/B/E/beta', wc_rev=2)
  expected_status.tweak('A/D/G/pi', wc_rev=2)
  expected_status.tweak('A/D/G/rho', wc_rev=2)

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Now we'll update each of our 5 files in wc_backup; each one will get
  # conflicts, and we'll handle each with a different --accept option.

  # Setup SVN_EDITOR and SVN_MERGE for --accept={edit,launch}.
  svntest.main.use_editor('append_foo')

  # iota: no accept option
  # Just leave the conflicts alone, since run_and_verify_svn already uses
  # the --non-interactive option.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (iota_path_backup),
                                      'C    %s\n' % (iota_path_backup,),
                                      'Updated to revision 2.\n',
                                      'Summary of conflicts:\n',
                                      '  Text conflicts: 1\n'],
                                     [],
                                     'update', iota_path_backup)

  # lambda: --accept=postpone
  # Just leave the conflicts alone.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (lambda_path_backup),
                                      'C    %s\n' % (lambda_path_backup,),
                                      'Updated to revision 2.\n',
                                      'Summary of conflicts:\n',
                                      '  Text conflicts: 1\n'],
                                     [],
                                     'update', '--accept=postpone',
                                     lambda_path_backup)

  # mu: --accept=base
  # Accept the pre-update base file.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (mu_path_backup),
                                      'G    %s\n' % (mu_path_backup,),
                                      'Updated to revision 2.\n'],
                                     [],
                                     'update', '--accept=base',
                                     mu_path_backup)

  # alpha: --accept=mine
  # Accept the user's working file.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (alpha_path_backup),
                                      'G    %s\n' % (alpha_path_backup,),
                                      'Updated to revision 2.\n'],
                                     [],
                                     'update', '--accept=mine-full',
                                     alpha_path_backup)

  # beta: --accept=theirs
  # Accept their file.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (beta_path_backup),
                                      'G    %s\n' % (beta_path_backup,),
                                      'Updated to revision 2.\n'],
                                     [],
                                     'update', '--accept=theirs-full',
                                     beta_path_backup)

  # pi: --accept=edit
  # Run editor and accept the edited file. The merge tool will leave
  # conflicts in place, so expect a message on stderr, but expect
  # svn to exit with an exit code of 0.
  svntest.actions.run_and_verify_svn2(None,
                                      ["Updating '%s':\n" % (pi_path_backup),
                                       'G    %s\n' % (pi_path_backup,),
                                       'Updated to revision 2.\n'],
                                      "system(.*) returned.*", 0,
                                      'update', '--accept=edit',
                                      pi_path_backup)

  # rho: --accept=launch
  # Run the external merge tool, it should leave conflict markers in place.
  svntest.actions.run_and_verify_svn(None,
                                     ["Updating '%s':\n" % (rho_path_backup),
                                      'C    %s\n' % (rho_path_backup,),
                                      'Updated to revision 2.\n',
                                      'Summary of conflicts:\n',
                                      '  Text conflicts: 1\n'],
                                     [],
                                     'update', '--accept=launch',
                                     rho_path_backup)

  # Set the expected disk contents for the test
  expected_disk = svntest.main.greek_state.copy()

  expected_disk.tweak('iota', contents=("This is the file 'iota'.\n"
                                        '<<<<<<< .mine\n'
                                        'My appended text for iota\n'
                                        '=======\n'
                                        'Their appended text for iota\n'
                                        '>>>>>>> .r2\n'))
  expected_disk.tweak('A/B/lambda', contents=("This is the file 'lambda'.\n"
                                              '<<<<<<< .mine\n'
                                              'My appended text for lambda\n'
                                              '=======\n'
                                              'Their appended text for lambda\n'
                                              '>>>>>>> .r2\n'))
  expected_disk.tweak('A/mu', contents="This is the file 'mu'.\n")
  expected_disk.tweak('A/B/E/alpha', contents=("This is the file 'alpha'.\n"
                                               'My appended text for alpha\n'))
  expected_disk.tweak('A/B/E/beta', contents=("This is the file 'beta'.\n"
                                              'Their appended text for beta\n'))
  expected_disk.tweak('A/D/G/pi', contents=("This is the file 'pi'.\n"
                                             '<<<<<<< .mine\n'
                                             'My appended text for pi\n'
                                             '=======\n'
                                             'Their appended text for pi\n'
                                             '>>>>>>> .r2\n'
                                             'foo\n'))
  expected_disk.tweak('A/D/G/rho', contents=("This is the file 'rho'.\n"
                                             '<<<<<<< .mine\n'
                                             'My appended text for rho\n'
                                             '=======\n'
                                             'Their appended text for rho\n'
                                             '>>>>>>> .r2\n'
                                             'foo\n'))

  # Set the expected extra files for the test
  extra_files = ['iota.*\.r1', 'iota.*\.r2', 'iota.*\.mine',
                 'lambda.*\.r1', 'lambda.*\.r2', 'lambda.*\.mine',
                 'rho.*\.r1', 'rho.*\.r2', 'rho.*\.mine']

  # Set the expected status for the test
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.tweak('iota', 'A/B/lambda', 'A/mu',
                        'A/B/E/alpha', 'A/B/E/beta',
                        'A/D/G/pi', 'A/D/G/rho', wc_rev=2)
  expected_status.tweak('iota', status='C ')
  expected_status.tweak('A/B/lambda', status='C ')
  expected_status.tweak('A/mu', status='M ')
  expected_status.tweak('A/B/E/alpha', status='M ')
  expected_status.tweak('A/B/E/beta', status='  ')
  expected_status.tweak('A/D/G/pi', status='M ')
  expected_status.tweak('A/D/G/rho', status='C ')

  # Set the expected output for the test
  expected_output = wc.State(wc_backup, {})

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files)

# Test for a wc corruption race condition (possibly introduced in
# r863416) which is easy to trigger if interactive conflict resolution
# dies in the middle of prompting.  Specifically, we run an update
# with interactive-conflicts on but close stdin immediately, so the
# prompt errors out; then the dir_baton pool cleanup handlers in the
# WC update editor flush and run incomplete logs and lead to WC
# corruption, detectable by another update command.

def eof_in_interactive_conflict_resolver(sbox):
  "eof in interactive resolution can't break wc"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Set up a custom config directory which *doesn't* turn off
  # interactive resolution
  config_contents = '''\
[auth]
password-stores =

[miscellany]
interactive-conflicts = true
'''
  tmp_dir = os.path.abspath(svntest.main.temp_dir)
  config_dir = os.path.join(tmp_dir, 'interactive-conflicts-config')
  svntest.main.create_config_dir(config_dir, config_contents)

  iota_path = os.path.join(wc_dir, 'iota')

  # Modify iota and commit for r2.
  svntest.main.file_append(iota_path, "Appended text in r2.\n")
  expected_output = svntest.wc.State(wc_dir, {
    'iota': Item(verb="Sending"),
  })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Go back to revision 1.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status='U '),
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        None, None,
                                        None, None, 1,
                                        '-r1', wc_dir)

  # Modify iota differently and try to update *with the interactive
  # resolver*.  ### The parser won't go so well with the output
  svntest.main.file_append(iota_path, "Local mods to r1 text.\n")
  svntest.actions.run_and_verify_update(wc_dir, None, None, None,
                                        "Can't read stdin: End of file found",
                                        None, None, None, None, 1,
                                        wc_dir, '--config-dir', config_dir)

  # Now update -r1 again.  Hopefully we don't get a checksum error!
  expected_output = svntest.wc.State(wc_dir, {})

  # note: it's possible that the correct disk here should be the
  # merged file?
  expected_disk.tweak('iota', contents=("This is the file 'iota'.\n"
                                        "Local mods to r1 text.\n"))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='M ')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        None, None,
                                        None, None, 1,
                                        '-r1', wc_dir)


#----------------------------------------------------------------------


def update_uuid_changed(sbox):
  "update fails when repos uuid changed"

  # read_only=False, since we don't want to run setuuid on the (shared)
  # pristine repository.
  sbox.build(read_only = False)

  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  uuid_before = svntest.actions.get_wc_uuid(wc_dir)

  # Change repository's uuid.
  svntest.actions.run_and_verify_svnadmin(None, None, [],
                                          'setuuid', repo_dir)

  # 'update' detected the new uuid...
  svntest.actions.run_and_verify_svn(None, None, '.*UUID.*',
                                     'update', wc_dir)

  # ...and didn't overwrite the old uuid.
  uuid_after = svntest.actions.get_wc_uuid(wc_dir)
  if uuid_before != uuid_after:
    raise svntest.Failure


#----------------------------------------------------------------------

# Issue #1672: if an update deleting a dir prop is interrupted (by a
# local obstruction, for example) then restarting the update will not
# delete the prop, causing the wc to become out of sync with the
# repository.
def restarted_update_should_delete_dir_prop(sbox):
  "restarted update should delete dir prop"
  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  zeta_path = os.path.join(A_path, 'zeta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Commit a propset on A.
  svntest.main.run_svn(None, 'propset', 'prop', 'val', A_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A': Item(verb='Sending'),
    })

  expected_status.tweak('A', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create a second working copy.
  ### Does this hack still work with wc-ng?
  other_wc = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, other_wc)

  other_A_path = os.path.join(other_wc, 'A')
  other_zeta_path = os.path.join(other_wc, 'A', 'zeta')

  # In the second working copy, delete A's prop and add a new file.
  svntest.main.run_svn(None, 'propdel', 'prop', other_A_path)
  svntest.main.file_write(other_zeta_path, 'New file\n')
  svntest.main.run_svn(None, 'add', other_zeta_path)

  expected_output = svntest.wc.State(other_wc, {
    'A': Item(verb='Sending'),
    'A/zeta' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(other_wc, 1)
  expected_status.tweak('A', wc_rev=3)
  expected_status.add({
    'A/zeta' : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(other_wc, expected_output,
                                        expected_status, None, other_wc)

  # Back in the first working copy, create an obstructing path and
  # update. The update will flag a tree conflict.
  svntest.main.file_write(zeta_path, 'Obstructing file\n')

  #svntest.factory.make(sbox, 'svn up')
  #exit(0)
  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'A'                 : Item(status=' U'),
    'A/zeta'            : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/zeta'            : Item(contents="Obstructing file\n"),
  })

  expected_status = actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/zeta'            : Item(status='D ', treeconflict='C', wc_rev='3'),
  })

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)

  # Now, delete the obstructing path and rerun the update.
  os.unlink(zeta_path)

  svntest.main.run_svn(None, 'revert', zeta_path)

  expected_output = svntest.wc.State(wc_dir, {
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A', props = {})
  expected_disk.add({
    'A/zeta' : Item("New file\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/zeta' : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        check_props = True)

#----------------------------------------------------------------------

# Detect tree conflicts among files and directories,
# edited or deleted in a deep directory structure.
#
# See use cases 1-3 in notes/tree-conflicts/use-cases.txt for background.

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


def tree_conflicts_on_update_1_1(sbox):
  "tree conflicts 1.1: tree del, leaf edit on update"

  # use case 1, as in notes/tree-conflicts/use-cases.txt
  # 1.1) local tree delete, incoming leaf edit

  sbox.build()

  expected_output = deep_trees_conflict_output.copy()
  expected_output.add({
    'DDF/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDF/D1/D2/gamma'   : Item(status='  ', treeconflict='U'),
    'DD/D1/D2'          : Item(status='  ', treeconflict='U'),
    'DD/D1/D2/epsilon'  : Item(status='  ', treeconflict='A'),
    'DDD/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3'      : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3/zeta' : Item(status='  ', treeconflict='A'),
    'D/D1/delta'        : Item(status='  ', treeconflict='A'),
    'DF/D1/beta'        : Item(status='  ', treeconflict='U'),
  })

  expected_disk = disk_empty_dirs.copy()
  if svntest.main.wc_is_singledb(sbox.wc_dir):
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

  # Update to the target rev.
  expected_status.tweak(wc_rev=3)

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .file.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .dir.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .dir.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .dir.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .dir.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .dir.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_update(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_leaf_edit",
                        tree_del,
                        leaf_edit,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


def tree_conflicts_on_update_1_2(sbox):
  "tree conflicts 1.2: tree del, leaf del on update"

  # 1.2) local tree delete, incoming leaf delete

  sbox.build()

  expected_output = deep_trees_conflict_output.copy()
  expected_output.add({
    'DDD/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDD/D1/D2/D3'      : Item(status='  ', treeconflict='D'),
    'DF/D1/beta'        : Item(status='  ', treeconflict='D'),
    'DD/D1/D2'          : Item(status='  ', treeconflict='D'),
    'DDF/D1/D2'         : Item(status='  ', treeconflict='U'),
    'DDF/D1/D2/gamma'   : Item(status='  ', treeconflict='D'),
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
        '^local delete, incoming delete upon update'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .dir.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .dir.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .dir.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming edit upon update'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .dir.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_update(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_leaf_del",
                        tree_del,
                        leaf_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


def tree_conflicts_on_update_2_1(sbox):
  "tree conflicts 2.1: leaf edit, tree del on update"

  # use case 2, as in notes/tree-conflicts/use-cases.txt
  # 2.1) local leaf edit, incoming tree delete

  expected_output = deep_trees_conflict_output

  expected_disk = disk_after_leaf_edit

  expected_status = deep_trees_status_local_leaf_edit.copy()
  # Adjust the status of the roots of the six subtrees scheduled for deletion
  # during the update.  Since these are all tree conflicts, they will all be
  # scheduled for addition as copies with history - see Issue #3334.
  expected_status.tweak(
    'D/D1',
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

  expected_info = {
    'F/alpha' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local edit, incoming delete upon update'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  ### D/D1/delta is locally-added during leaf_edit. when tree_del executes,
  ### it will delete D/D1, and the update reschedules local D/D1 for
  ### local-copy from its original revision. however, right now, we cannot
  ### denote that delta is a local-add rather than a child of that D/D1 copy.
  ### thus, it appears in the status output as a (M)odified child.
  svntest.actions.deep_trees_run_tests_scheme_for_update(sbox,
    [ DeepTreesTestCase("local_leaf_edit_incoming_tree_del",
                        leaf_edit,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )



def tree_conflicts_on_update_2_2(sbox):
  "tree conflicts 2.2: leaf del, tree del on update"

  # 2.2) local leaf delete, incoming tree delete

  ### Current behaviour fails to show conflicts when deleting
  ### a directory tree that has modifications. (Will be solved
  ### when dirs_same_p() is implemented)
  expected_output = deep_trees_conflict_output

  expected_disk = disk_empty_dirs.copy()

  expected_status = svntest.actions.deep_trees_virginal_state.copy()
  expected_status.add({'' : Item()})
  expected_status.tweak(contents=None, status='  ', wc_rev=3)
  # Tree conflicts.
  expected_status.tweak(
    'D/D1',
    'F/alpha',
    'DD/D1',
    'DF/D1',
    'DDD/D1',
    'DDF/D1',
    treeconflict='C', wc_rev=2)

  # Expect the incoming tree deletes and the local leaf deletes to mean
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
        '^local delete, incoming delete upon update'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_update(sbox,
    [ DeepTreesTestCase("local_leaf_del_incoming_tree_del",
                        leaf_del,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )


#----------------------------------------------------------------------
# Test for issue #3329 'Update throws error when skipping some tree
# conflicts'
#
# Marked as XFail until issue #3329 is resolved.
@Issue(3329)
def tree_conflicts_on_update_2_3(sbox):
  "tree conflicts 2.3: skip on 2nd update"

  # Test that existing tree conflicts are skipped

  expected_output = deep_trees_conflict_output_skipped

  expected_disk = disk_after_leaf_edit

  expected_status = deep_trees_status_local_leaf_edit.copy()

  # Adjust the status of the roots of the six subtrees scheduled for deletion
  # during the update.  Since these are all tree conflicts, they will all be
  # scheduled for addition as copies with history - see Issue #3334.
  expected_status.tweak(
    'D/D1',
    'F/alpha',
    'DD/D1',
    'DF/D1',
    'DDD/D1',
    'DDF/D1',
    status='A ', copied='+', wc_rev='-')
  # See the status of all the paths *under* the above six subtrees.  Only the
  # roots of the added subtrees show as schedule 'A', these child paths show
  # only that history is scheduled with the commit.
  expected_status.tweak(
    'DD/D1/D2',
    'DDD/D1/D2',
    'DDD/D1/D2/D3',
    'DF/D1/beta',
    'DDF/D1/D2',
    'DDF/D1/D2/gamma',
    copied='+', wc_rev='-')

  # Paths where output should be a single 'Skipped' message.
  skip_paths = [
    'D/D1',
    'F/alpha',
    'DDD/D1',
    'DDD/D1/D2/D3',
    ]

  # This is where the test fails.  Repeat updates on '', 'D', 'F', or
  # 'DDD' report no skips.
  chdir_skip_paths = [
    ('D', 'D1'),
    ('F', 'alpha'),
    ('DDD', 'D1'),
    ('', ['D/D1', 'F/alpha', 'DD/D1', 'DF/D1', 'DDD/D1', 'DDF/D1']),
    ]
  # Note: We don't step *into* a directory that's deleted in the repository.
  # E.g. ('DDD/D1/D2', '') would correctly issue a "path does not
  # exist" error, because at that point it can't know about the
  # tree-conflict on DDD/D1. ('D/D1', '') likewise, as tree-conflict
  # information is stored in the parent of a victim directory.

  svntest.actions.deep_trees_skipping_on_update(sbox,
    DeepTreesTestCase("local_leaf_edit_incoming_tree_del_skipping",
                      leaf_edit,
                      tree_del,
                      expected_output,
                      expected_disk,
                      expected_status),
                                                skip_paths,
                                                chdir_skip_paths)


def tree_conflicts_on_update_3(sbox):
  "tree conflicts 3: tree del, tree del on update"

  # use case 3, as in notes/tree-conflicts/use-cases.txt
  # local tree delete, incoming tree delete

  expected_output = deep_trees_conflict_output

  expected_disk = disk_empty_dirs.copy()

  expected_status = deep_trees_status_local_tree_del.copy()

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
        '^local delete, incoming delete upon update'
        + ' Source  left: .file.*/F/alpha@2'
        + ' Source right: .none.*/F/alpha@3$',
    },
    'DF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DF/D1@2'
        + ' Source right: .none.*/DF/D1@3$',
    },
    'DDF/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DDF/D1@2'
        + ' Source right: .none.*/DDF/D1@3$',
    },
    'D/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/D/D1@2'
        + ' Source right: .none.*/D/D1@3$',
    },
    'DD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DD/D1@2'
        + ' Source right: .none.*/DD/D1@3$',
    },
    'DDD/D1' : {
      'Tree conflict' :
        '^local delete, incoming delete upon update'
        + ' Source  left: .dir.*/DDD/D1@2'
        + ' Source right: .none.*/DDD/D1@3$',
    },
  }

  svntest.actions.deep_trees_run_tests_scheme_for_update(sbox,
    [ DeepTreesTestCase("local_tree_del_incoming_tree_del",
                        tree_del,
                        tree_del,
                        expected_output,
                        expected_disk,
                        expected_status,
                        expected_info = expected_info) ] )

# Issue #3334: a modify-on-deleted tree conflict should leave the node
# updated to the target revision but still scheduled for deletion.
def tree_conflict_uc1_update_deleted_tree(sbox):
  "tree conflicts on update UC1, update deleted tree"
  sbox.build()
  wc_dir = sbox.wc_dir

  from svntest.actions import run_and_verify_svn, run_and_verify_resolve
  from svntest.actions import run_and_verify_update, run_and_verify_commit
  from svntest.verify import AnyOutput

  """A directory tree 'D1' should end up exactly the same in these two
  scenarios:

  New scenario:
  [[[
    svn checkout -r1             # in which D1 has its original state
    svn delete D1
    svn update -r2               # update revs & bases to r2
    svn resolve --accept=mine    # keep the local, deleted version
  ]]]

  Existing scenario:
  [[[
    svn checkout -r2             # in which D1 is already modified
    svn delete D1
  ]]]
  """

  A = os.path.join(wc_dir, 'A')

  def modify_dir(dir):
    """Make some set of local modifications to an existing tree:
    A prop change, add a child, delete a child, change a child."""
    run_and_verify_svn(None, AnyOutput, [], 'propset', 'p', 'v', dir)

    path = os.path.join(dir, 'new_file')
    svntest.main.file_write(path, "This is the file 'new_file'.\n")
    svntest.actions.run_and_verify_svn(None, None, [], 'add', path)

    path = os.path.join(dir, 'C', 'N')
    os.mkdir(path)
    path2 = os.path.join(dir, 'C', 'N', 'nu')
    svntest.main.file_write(path2, "This is the file 'nu'.\n")
    svntest.actions.run_and_verify_svn(None, None, [], 'add', path)

    path = os.path.join(dir, 'B', 'lambda')
    svntest.actions.run_and_verify_svn(None, None, [], 'delete', path)

    path = os.path.join(dir, 'B', 'E', 'alpha')
    svntest.main.file_append(path, "An extra line.\n")

  # Prep for both scenarios
  modify_dir(A)
  run_and_verify_svn(None, AnyOutput, [], 'ci', A, '-m', 'modify_dir')
  run_and_verify_svn(None, AnyOutput, [], 'up', wc_dir)

  # Existing scenario
  wc2 = sbox.add_wc_path('wc2')
  A2 = os.path.join(wc2, 'A')
  svntest.actions.duplicate_dir(sbox.wc_dir, wc2)
  run_and_verify_svn(None, AnyOutput, [], 'delete', A2)

  # New scenario (starts at the revision before the committed mods)
  run_and_verify_svn(None, AnyOutput, [], 'up', A, '-r1')
  run_and_verify_svn(None, AnyOutput, [], 'delete', A)

  expected_output = None
  expected_disk = None
  expected_status = None

  run_and_verify_update(A, expected_output, expected_disk, expected_status)
  run_and_verify_resolve([A], '--recursive', '--accept=working', A)

  resolved_status = svntest.wc.State('', {
      ''            : Item(status='  ', wc_rev=2),
      'A'           : Item(status='D ', wc_rev=2),
      'A/B'         : Item(status='D ', wc_rev=2),
      'A/B/E'       : Item(status='D ', wc_rev=2),
      'A/B/E/alpha' : Item(status='D ', wc_rev=2),
      'A/B/E/beta'  : Item(status='D ', wc_rev=2),
      'A/B/F'       : Item(status='D ', wc_rev=2),
      'A/mu'        : Item(status='D ', wc_rev=2),
      'A/C'         : Item(status='D ', wc_rev=2),
      'A/C/N'       : Item(status='D ', wc_rev=2),
      'A/C/N/nu'    : Item(status='D ', wc_rev=2),
      'A/D'         : Item(status='D ', wc_rev=2),
      'A/D/gamma'   : Item(status='D ', wc_rev=2),
      'A/D/G'       : Item(status='D ', wc_rev=2),
      'A/D/G/pi'    : Item(status='D ', wc_rev=2),
      'A/D/G/rho'   : Item(status='D ', wc_rev=2),
      'A/D/G/tau'   : Item(status='D ', wc_rev=2),
      'A/D/H'       : Item(status='D ', wc_rev=2),
      'A/D/H/chi'   : Item(status='D ', wc_rev=2),
      'A/D/H/omega' : Item(status='D ', wc_rev=2),
      'A/D/H/psi'   : Item(status='D ', wc_rev=2),
      'A/new_file'  : Item(status='D ', wc_rev=2),
      'iota'        : Item(status='  ', wc_rev=2),
      })

  # The status of the new and old scenarios should be identical.
  expected_status = resolved_status.copy()
  expected_status.wc_dir = wc2

  svntest.actions.run_and_verify_status(wc2, expected_status)

  expected_status = resolved_status.copy()
  expected_status.wc_dir = wc_dir

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Just for kicks, try to commit.
  expected_output = svntest.wc.State(wc_dir, {
      'A'           : Item(verb='Deleting'),
      })

  expected_status = svntest.wc.State(wc_dir, {
      ''            : Item(status='  ', wc_rev=2),
      'iota'        : Item(status='  ', wc_rev=2),
      })

  run_and_verify_commit(wc_dir, expected_output, expected_status,
                        None, wc_dir, '-m', 'commit resolved tree')


# Issue #3334: a delete-onto-modified tree conflict should leave the node
# scheduled for re-addition.
@Issue(3334)
def tree_conflict_uc2_schedule_re_add(sbox):
  "tree conflicts on update UC2, schedule re-add"
  sbox.build()
  saved_cwd = os.getcwd()
  os.chdir(sbox.wc_dir)

  from svntest.actions import run_and_verify_svn, run_and_verify_resolve
  from svntest.actions import run_and_verify_update
  from svntest.verify import AnyOutput

  """A directory tree 'D1' should end up exactly the same in these two
  scenarios:

  New scenario:
  [[[
    svn checkout -r1             # in which D1 exists
    modify_d1                    # make local mods in D1
    svn update -r2               # tries to delete D1
    svn resolve --accept=mine    # keep the local, re-added version
  ]]]

  Existing scenario:
  [[[
    svn checkout -r2             # in which D1 does not exist
    svn copy -r1 D1 .            # make a pristine copy of D1@1
    modify_d1                    # make local mods in D1
  ]]]

  where modify_d1 makes property changes to D1 itself and/or
  adds/deletes/modifies any of D1's children.
  """

  dir = 'A'  # an existing tree in the WC and repos
  dir_url = sbox.repo_url + '/' + dir

  def modify_dir(dir):
    """Make some set of local modifications to an existing tree:
    A prop change, add a child, delete a child, change a child."""
    run_and_verify_svn(None, AnyOutput, [],
                       'propset', 'p', 'v', dir)
    path = os.path.join(dir, 'new_file')
    svntest.main.file_write(path, "This is the file 'new_file'.\n")
    svntest.actions.run_and_verify_svn(None, None, [], 'add', path)

    path = os.path.join(dir, 'B', 'lambda')
    svntest.actions.run_and_verify_svn(None, None, [], 'delete', path)

    path = os.path.join(dir, 'B', 'E', 'alpha')
    svntest.main.file_append(path, "An extra line.\n")

  # Prepare the repos so that a later 'update' has an incoming deletion:
  # Delete the dir in the repos, making r2
  run_and_verify_svn(None, AnyOutput, [],
                     '-m', '', 'delete', dir_url)

  # Existing scenario
  os.chdir(saved_cwd)
  wc2 = sbox.add_wc_path('wc2')
  dir2 = os.path.join(wc2, dir)
  svntest.actions.duplicate_dir(sbox.wc_dir, wc2)
  run_and_verify_svn(None, AnyOutput, [], 'up', wc2)
  run_and_verify_svn(None, AnyOutput, [], 'copy', dir_url + '@1', dir2)
  modify_dir(dir2)

  # New scenario
  # (The dir is already checked out.)
  os.chdir(sbox.wc_dir)
  modify_dir(dir)

  expected_output = None
  expected_disk = None
  expected_status = None
  run_and_verify_update('A', expected_output, expected_disk, expected_status)
  run_and_verify_resolve([dir], '--recursive', '--accept=working', dir)

  os.chdir(saved_cwd)

  def get_status(dir):
    expected_status = svntest.wc.State(dir, {
      ''            : Item(status='  ', wc_rev='2'),
      'A'           : Item(status='A ', wc_rev='-', copied='+'),
      'A/B'         : Item(status='  ', wc_rev='-', copied='+'),
      'A/B/lambda'  : Item(status='D ', wc_rev='-', copied='+'),
      'A/B/E'       : Item(status='  ', wc_rev='-', copied='+'),
      'A/B/E/alpha' : Item(status='M ', wc_rev='-', copied='+'),
      'A/B/E/beta'  : Item(status='  ', wc_rev='-', copied='+'),
      'A/B/F'       : Item(status='  ', wc_rev='-', copied='+'),
      'A/mu'        : Item(status='  ', wc_rev='-', copied='+'),
      'A/C'         : Item(status='  ', wc_rev='-', copied='+'),
      'A/D'         : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/gamma'   : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/G'       : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/G/pi'    : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/G/rho'   : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/G/tau'   : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/H'       : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/H/chi'   : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/H/omega' : Item(status='  ', wc_rev='-', copied='+'),
      'A/D/H/psi'   : Item(status='  ', wc_rev='-', copied='+'),
      'A/new_file'  : Item(status='A ', wc_rev=0),
      'iota'        : Item(status='  ', wc_rev=2),
    })
    return expected_status

  # The status of the new and old scenarios should be identical...
  expected_status = get_status(wc2)
  ### The following fails, as of Apr 6, 2010. The problem is that A/new_file
  ### has been *added* within a copy, yet the wc_db datastore cannot
  ### differentiate this from a copied-child. As a result, new_file is
  ### reported as a (M)odified node, rather than (A)dded.
  svntest.actions.run_and_verify_status(wc2, expected_status)

  # ...except for the revision of the root of the WC and iota, because
  # above 'A' was the target of the update, not the WC root.
  expected_status = get_status(sbox.wc_dir)
  expected_status.tweak('', 'iota', wc_rev=1)
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  ### Do we need to do more to confirm we got what we want here?

#----------------------------------------------------------------------
def set_deep_depth_on_target_with_shallow_children(sbox):
  "infinite --set-depth adds shallow children"

  # Regardless of what depth the update target is at, if it has shallow
  # subtrees and we update --set-depth infinity, these shallow subtrees
  # should be populated.
  #
  # See http://svn.haxx.se/dev/archive-2009-04/0344.shtml.

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  A_path = os.path.join(wc_dir, "A")
  B_path = os.path.join(wc_dir, "A", "B")
  D_path = os.path.join(wc_dir, "A", "D")

  # Trim the tree: Set A/B to depth empty and A/D to depth immediates.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'       : Item(status='D '),
    'A/B/lambda'  : Item(status='D '),
    'A/B/F'       : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/F',
                       'A/B/lambda',
                       'A/B/E',
                       'A/B/E/alpha',
                       'A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/F',
                         'A/B/lambda',
                         'A/B/E',
                         'A/B/E/alpha',
                         'A/B/E/beta')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '--set-depth', 'empty',
                                        B_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi'    : Item(status='D '),
    'A/D/G/rho'   : Item(status='D '),
    'A/D/G/tau'   : Item(status='D '),
    'A/D/H/chi'   : Item(status='D '),
    'A/D/H/omega' : Item(status='D '),
    'A/D/H/psi'   : Item(status='D '),
    })

  expected_status.remove('A/D/G/pi',
                         'A/D/G/rho',
                         'A/D/G/tau',
                         'A/D/H/chi',
                         'A/D/H/omega',
                         'A/D/H/psi')

  expected_disk.remove('A/D/G/pi',
                       'A/D/G/rho',
                       'A/D/G/tau',
                       'A/D/H/chi',
                       'A/D/H/omega',
                       'A/D/H/psi')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '--set-depth', 'immediates',
                                        D_path)

  # Now update A with --set-depth infinity.  All the subtrees we
  # removed above should come back.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/lambda'  : Item(status='A '),
    'A/B/F'       : Item(status='A '),
    'A/B/E'       : Item(status='A '),
    'A/B/E/alpha' : Item(status='A '),
    'A/B/E/beta'  : Item(status='A '),
    'A/D/G/pi'    : Item(status='A '),
    'A/D/G/rho'   : Item(status='A '),
    'A/D/G/tau'   : Item(status='A '),
    'A/D/H/chi'   : Item(status='A '),
    'A/D/H/omega' : Item(status='A '),
    'A/D/H/psi'   : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        '--set-depth', 'infinity',
                                        A_path)

#----------------------------------------------------------------------

def update_wc_of_dir_to_rev_not_containing_this_dir(sbox):
  "update wc of dir to rev not containing this dir"

  sbox.build()

  # Create working copy of 'A' directory
  A_url = sbox.repo_url + "/A"
  other_wc_dir = sbox.add_wc_path("other")
  svntest.actions.run_and_verify_svn(None, None, [], "co", A_url, other_wc_dir)

  # Delete 'A' directory from repository
  svntest.actions.run_and_verify_svn(None, None, [], "rm", A_url, "-m", "")

  # Try to update working copy of 'A' directory
  svntest.actions.run_and_verify_svn(None, None,
                                     "svn: E160005: Target path '/A' does not exist",
                                     "up", other_wc_dir)

#----------------------------------------------------------------------
# Test for issue #3569 svn update --depth <DEPTH> allows making a working
# copy incomplete.
@Issue(3569)
def update_empty_hides_entries(sbox):
  "svn up --depth empty hides entries for next update"
  sbox.build()
  wc_dir = sbox.wc_dir

  expected_disk_empty = []
  expected_status_empty = []

  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Update to revision 0 - Removes all files from WC
  svntest.actions.run_and_verify_update(wc_dir,
                                        None,
                                        expected_disk_empty,
                                        expected_status_empty,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '0',
                                        wc_dir)

  # Now update back to HEAD
  svntest.actions.run_and_verify_update(wc_dir,
                                        None,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        wc_dir)

  # Update to revision 0 - Removes all files from WC
  svntest.actions.run_and_verify_update(wc_dir,
                                        None,
                                        expected_disk_empty,
                                        expected_status_empty,
                                        None, None, None,
                                        None, None, 1,
                                        '-r', '0',
                                        wc_dir)

  # Update the directory itself back to HEAD
  svntest.actions.run_and_verify_update(wc_dir,
                                        None,
                                        expected_disk_empty,
                                        expected_status_empty,
                                        None, None, None,
                                        None, None, 1,
                                        '--depth', 'empty',
                                        wc_dir)

  # Now update the rest back to head

  # This operation is currently a NO-OP, because the WC-Crawler
  # tells the repository that it contains a full tree of the HEAD
  # revision.
  svntest.actions.run_and_verify_update(wc_dir,
                                        None,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        wc_dir)

#----------------------------------------------------------------------
# Test for issue #3573 'local non-inheritable mergeinfo changes not
# properly merged with updated mergeinfo'
def mergeinfo_updates_merge_with_local_mods(sbox):
  "local mergeinfo changes are merged with updates"

  # Copy A to A_COPY in r2, and make some changes to A_COPY in r3-r6.
  sbox.build()
  wc_dir = sbox.wc_dir
  expected_disk, expected_status = set_up_branch(sbox)

  # Some paths we'll care about
  A_path      = os.path.join(wc_dir, "A")
  A_COPY_path = os.path.join(wc_dir, "A_COPY")

  # Merge -c3 from A to A_COPY at --depth empty, commit as r7.
  ###
  ### No, we are not checking the merge output for these simple
  ### merges.  This is already covered *TO DEATH* in merge_tests.py.
  ###
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '-c3', '--depth', 'empty',
                                     sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Merge r3 from A to A_COPY at depth empty',
                                     wc_dir)
  # Merge -c5 from A to A_COPY (at default --depth infinity), commit as r8.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '-c5',
                                     sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'Merge r5 from A to A_COPY', wc_dir)

  # Update WC to r7, repeat merge of -c3 from A to A_COPY but this
  # time do it at --depth infinity.  Confirm that the mergeinfo
  # on A_COPY is no longer inheritable.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r7', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '-c3', '--depth', 'infinity',
                                     sbox.repo_url + '/A', A_COPY_path)
  svntest.actions.run_and_verify_svn(None, [A_COPY_path + " - /A:3\n"], [],
                                     'pg', SVN_PROP_MERGEINFO, '-R',
                                     A_COPY_path)

  # Update the WC (to r8), the mergeinfo on A_COPY should now have both
  # the local mod from the uncommitted merge (/A:3* --> /A:3) and the change
  # brought down by the update (/A:3* --> /A:3*,5) leaving us with /A:3,5.
  ### This was failing because of issue #3573.  The local mergeinfo change
  ### is reverted, leaving '/A:3*,5' on A_COPY.
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  svntest.actions.run_and_verify_svn(None, [A_COPY_path + " - /A:3,5\n"], [],
                                     'pg', SVN_PROP_MERGEINFO, '-R',
                                     A_COPY_path)

#----------------------------------------------------------------------
# A regression test for a 1.7-dev crash upon updating a WC to a different
# revision when it contained an excluded dir.
def update_with_excluded_subdir(sbox):
  """update with an excluded subdir"""
  sbox.build()

  wc_dir = sbox.wc_dir

  G = os.path.join(os.path.join(wc_dir, 'A', 'D', 'G'))

  # Make the directory 'G' excluded.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G' : Item(status='D '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau')
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status,
                                        None, None, None, None, None, False,
                                        '--set-depth=exclude', G)

  # Commit a new revision so there is something to update to.
  svntest.main.run_svn(None, 'mkdir', '-m', '', sbox.repo_url + '/New')

  # Test updating the WC.
  expected_output = svntest.wc.State(wc_dir, {
    'New' : Item(status='A ') })
  expected_disk.add({
    'New' : Item() })
  expected_status.add({
    'New' : Item(status='  ') })
  expected_status.tweak(wc_rev=2)
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status)

#----------------------------------------------------------------------
# Test for issue #3471 'svn up touches file w/ lock & svn:keywords property'
@Issue(3471)
def update_with_file_lock_and_keywords_property_set(sbox):
  """update with file lock & keywords property set"""
  sbox.build()

  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, '$Id$')
  svntest.main.run_svn(None, 'ps', 'svn:keywords', 'Id', mu_path)
  svntest.main.run_svn(None, 'lock', mu_path)
  mu_ts_before_update = os.path.getmtime(mu_path)

  # Make sure we are at a different timestamp to really notice a mtime change
  time.sleep(1.1)

  # Issue #3471 manifests itself here; The timestamp of 'mu' gets updated
  # to the time of the last "svn up".
  sbox.simple_update()
  mu_ts_after_update = os.path.getmtime(mu_path)
  if (mu_ts_before_update != mu_ts_after_update):
    print("The timestamp of 'mu' before and after update does not match.")
    raise svntest.Failure

#----------------------------------------------------------------------
# Updating a nonexistent or deleted path should be a successful no-op,
# when there is no incoming change.  In trunk@1035343, such an update
# within a copied directory triggered an assertion failure.
@Issue(3807)
def update_nonexistent_child_of_copy(sbox):
  """update a nonexistent child of a copied dir"""
  sbox.build()
  os.chdir(sbox.wc_dir)

  svntest.main.run_svn(None, 'copy', 'A', 'A2')

  # Try updating a nonexistent path in the copied dir.
  expected_output = svntest.wc.State('A2', {
    'nonexistent'             : Item(verb='Skipped'),
  })
  svntest.actions.run_and_verify_update(os.path.join('A2', 'nonexistent'),
                                        expected_output, None, None, None)

  # Try updating a deleted path in the copied dir.
  svntest.main.run_svn(None, 'delete', os.path.join('A2', 'mu'))

  expected_output = svntest.wc.State('A2', {
    'mu'             : Item(verb='Skipped'),
  })
  svntest.actions.run_and_verify_update(os.path.join('A2', 'mu'),
                                        expected_output, None, None, None)
  if os.path.exists('A2/mu'):
    raise svntest.Failure("A2/mu improperly revived")

@Issue(3807)
def revive_children_of_copy(sbox):
  """undelete a child of a copied dir"""
  sbox.build()
  os.chdir(sbox.wc_dir)

  chi2_path = os.path.join('A2/D/H/chi')
  psi2_path = os.path.join('A2/D/H/psi')

  svntest.main.run_svn(None, 'copy', 'A', 'A2')
  svntest.main.run_svn(None, 'rm', chi2_path)
  os.unlink(psi2_path)

  svntest.main.run_svn(None, 'revert', chi2_path, psi2_path)
  if not os.path.exists(chi2_path):
    raise svntest.Failure('chi unexpectedly non-existent')
  if not os.path.exists(psi2_path):
    raise svntest.Failure('psi unexpectedly non-existent')

@SkipUnless(svntest.main.is_os_windows)
def skip_access_denied(sbox):
  """access denied paths should be skipped"""

  # We need something to lock the file. 'msvcrt' looks common on Windows
  try:
    import msvcrt
  except ImportError:
    raise svntest.Skip

  sbox.build()
  wc_dir = sbox.wc_dir

  iota = sbox.ospath('iota')

  svntest.main.file_write(iota, 'Q')
  sbox.simple_commit()
  sbox.simple_update() # Update to r2

  # Open iota for writing to keep an handle open
  f = open(iota, 'w')

  # Write new text of exactly the same size to avoid the early out
  # on a different size without properties.
  f.write('R')
  f.flush()

  # And lock the first byte of the file
  msvcrt.locking(f.fileno(), 1, 1)

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Skipped'),
    })

  # Create expected status tree: iota isn't updated
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='M ', wc_rev=2)

  # And now check that update skips the path
  # *and* status shows the path as modified.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        expected_status,
                                        None,
                                        None, None,
                                        None, None, None, wc_dir, '-r', '1')

  f.close()

def update_to_HEAD_plus_1(sbox):
  "updating to HEAD+1 should fail"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Attempt the update, expecting an error.  (Sometimes the error
  # strings says "No such revision", sometimes "No such target
  # revision".)
  svntest.actions.run_and_verify_update(wc_dir,
                                        None, None, None,
                                        "E160006.*No such.*revision",
                                        None, None,
                                        None, None, None, wc_dir, '-r', '2')


#######################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              update_binary_file,
              update_binary_file_2,
              update_ignores_added,
              update_to_rev_zero,
              receive_overlapping_same_change,
              update_to_resolve_text_conflicts,
              update_delete_modified_files,
              update_after_add_rm_deleted,
              update_missing,
              update_replace_dir,
              update_single_file,
              prop_update_on_scheduled_delete,
              update_receive_illegal_name,
              update_deleted_missing_dir,
              another_hudson_problem,
              update_deleted_targets,
              new_dir_with_spaces,
              non_recursive_update,
              checkout_empty_dir,
              update_to_deletion,
              update_deletion_inside_out,
              update_schedule_add_dir,
              update_to_future_add,
              nested_in_read_only,
              obstructed_update_alters_wc_props,
              update_xml_unsafe_dir,
              conflict_markers_matching_eol,
              update_eolstyle_handling,
              update_copy_of_old_rev,
              forced_update,
              forced_update_failures,
              update_wc_on_windows_drive,
              update_wc_with_replaced_file,
              update_with_obstructing_additions,
              update_conflicted,
              mergeinfo_update_elision,
              update_copied_from_replaced_and_changed,
              update_copied_and_deleted_prop,
              update_accept_conflicts,
              eof_in_interactive_conflict_resolver,
              update_uuid_changed,
              restarted_update_should_delete_dir_prop,
              tree_conflicts_on_update_1_1,
              tree_conflicts_on_update_1_2,
              tree_conflicts_on_update_2_1,
              tree_conflicts_on_update_2_2,
              tree_conflicts_on_update_2_3,
              tree_conflicts_on_update_3,
              tree_conflict_uc1_update_deleted_tree,
              tree_conflict_uc2_schedule_re_add,
              set_deep_depth_on_target_with_shallow_children,
              update_wc_of_dir_to_rev_not_containing_this_dir,
              update_empty_hides_entries,
              mergeinfo_updates_merge_with_local_mods,
              update_with_excluded_subdir,
              update_with_file_lock_and_keywords_property_set,
              update_nonexistent_child_of_copy,
              revive_children_of_copy,
              skip_access_denied,
              update_to_HEAD_plus_1,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
