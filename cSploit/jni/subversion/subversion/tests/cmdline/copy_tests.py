#!/usr/bin/env python
#
#  copy_tests.py:  testing the many uses of 'svn cp' and 'svn mv'
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
import stat, os, re, shutil

# Our testing module
import svntest
from svntest import main
from svntest.main import (
  SVN_PROP_MERGEINFO,
  file_append,
  file_write,
  make_log_msg,
  run_svn,
)

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem
exp_noop_up_out = svntest.actions.expected_noop_update_output

#
#----------------------------------------------------------------------
# Helper for wc_copy_replacement and repos_to_wc_copy_replacement
def copy_replace(sbox, wc_copy):
  """Tests for 'R'eplace functionanity for files.

Depending on the value of wc_copy either a working copy (when true)
or a url (when false) copy source is used."""

  sbox.build()
  wc_dir = sbox.wc_dir

  # File scheduled for deletion
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', rho_path)

  # Status before attempting copies
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # The copy shouldn't fail
  if wc_copy:
    pi_src = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  else:
    pi_src = sbox.repo_url + '/A/D/G/pi'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', pi_src, rho_path)

  # Now commit
  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_status.tweak('A/D/G/rho', status='  ', copied=None,
                        wc_rev='2')
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho': Item(verb='Replacing'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

# Helper for wc_copy_replace_with_props and
# repos_to_wc_copy_replace_with_props
def copy_replace_with_props(sbox, wc_copy):
  """Tests for 'R'eplace functionanity for files with props.

  Depending on the value of wc_copy either a working copy (when true) or
  a url (when false) copy source is used."""

  sbox.build()
  wc_dir = sbox.wc_dir

  # Use a temp file to set properties with wildcards in their values
  # otherwise Win32/VS2005 will expand them
  prop_path = os.path.join(wc_dir, 'proptmp')
  svntest.main.file_append(prop_path, '*')

  # Set props on file which is copy-source later on
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'phony-prop', '-F',
                                     prop_path, pi_path)
  os.remove(prop_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'svn:eol-style', 'LF', rho_path)

  # Verify props having been set
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/G/pi',
                      props={ 'phony-prop': '*' })
  expected_disk.tweak('A/D/G/rho',
                      props={ 'svn:eol-style': 'LF' })

  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())

  # Commit props
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi':  Item(verb='Sending'),
    'A/D/G/rho': Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/pi',  wc_rev='2')
  expected_status.tweak('A/D/G/rho', wc_rev='2')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Bring wc into sync
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  # File scheduled for deletion
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', rho_path)

  # Status before attempting copies
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # The copy shouldn't fail
  if wc_copy:
    pi_src = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  else:
    pi_src = sbox.repo_url + '/A/D/G/pi'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', pi_src, rho_path)

  # Verify both content and props have been copied
  if wc_copy:
    props = { 'phony-prop' : '*'}
  else:
    props = { 'phony-prop' : '*'}

  expected_disk.tweak('A/D/G/rho',
                      contents="This is the file 'pi'.\n",
                      props=props)
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())

  # Now commit and verify
  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_status.tweak('A/D/G/rho', status='  ', copied=None,
                        wc_rev='3')
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho': Item(verb='Replacing'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

# (Taken from notes/copy-planz.txt:)
#
#  We have four use cases for 'svn cp' now.
#
#     A. svn cp wc_path1 wc_path2
#
#        This duplicates a path in the working copy, and schedules it
#        for addition with history.
#
#     B. svn cp URL [-r rev]  wc_path
#
#        This "checks out" URL (in REV) into the working copy at
#        wc_path, integrates it, and schedules it for addition with
#        history.
#
#     C. svn cp wc_path URL
#
#        This immediately commits wc_path to URL on the server;  the
#        commit will be an addition with history.  The commit will not
#        change the working copy at all.
#
#     D. svn cp URL1 [-r rev] URL2
#
#        This causes a server-side copy to happen immediately;  no
#        working copy is required.



# TESTS THAT NEED TO BE WRITTEN
#
#  Use Cases A & C
#
#   -- single files, with/without local mods, as both 'cp' and 'mv'.
#        (need to verify commit worked by updating a 2nd working copy
#        to see the local mods)
#
#   -- dir copy, has mixed revisions
#
#   -- dir copy, has local mods (an edit, an add, a delete, and a replace)
#
#   -- dir copy, has mixed revisions AND local mods
#
#   -- dir copy, has mixed revisions AND another previously-made copy!
#      (perhaps done as two nested 'mv' commands!)
#
#  Use Case D
#

#   By the time the copy setup algorithm is complete, the copy
#   operation will have four parts: SRC-DIR, SRC-BASENAME, DST-DIR,
#   DST-BASENAME.  In all cases, SRC-DIR/SRC-BASENAME and DST_DIR must
#   already exist before the operation, but DST_DIR/DST_BASENAME must
#   NOT exist.
#
#   Besides testing things that don't meet the above criteria, we want to
#   also test valid cases:
#
#   - where SRC-DIR/SRC-BASENAME is a file or a dir.
#   - where SRC-DIR (or SRC-DIR/SRC-BASENAME) is a parent/grandparent
#     directory of DST-DIR
#   - where SRC-DIR (or SRC-DIR/SRC-BASENAME) is a child/grandchild
#     directory of DST-DIR
#   - where SRC-DIR (or SRC-DIR/SRC-BASENAME) is not in the lineage
#     of DST-DIR at all



#----------------------------------------------------------------------
@Issue(1091)
def basic_copy_and_move_files(sbox):
  "basic copy and move commands -- on files only"

  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  iota_path = os.path.join(wc_dir, 'iota')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  D_path = os.path.join(wc_dir, 'A', 'D')
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  alpha2_path = os.path.join(wc_dir, 'A', 'C', 'alpha2')

  # Make local mods to mu and rho
  svntest.main.file_append(mu_path, 'appended mu text')
  svntest.main.file_append(rho_path, 'new appended text for rho')

  # Copy rho to D -- local mods
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', rho_path, D_path)

  # Copy alpha to C -- no local mods, and rename it to 'alpha2' also
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     alpha_path, alpha2_path)

  # Move mu to H -- local mods
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     mu_path, H_path)

  # Move iota to F -- no local mods
  svntest.actions.run_and_verify_svn(None, None, [], 'mv', iota_path, F_path)

  # Created expected output tree for 'svn ci':
  # We should see four adds, two deletes, and one change in total.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Sending'),
    'A/D/rho' : Item(verb='Adding'),
    'A/C/alpha2' : Item(verb='Adding'),
    'A/D/H/mu' : Item(verb='Adding'),
    'A/B/F/iota' : Item(verb='Adding'),
    'A/mu' : Item(verb='Deleting'),
    'iota' : Item(verb='Deleting'),
    })

  # Create expected status tree; all local revisions should be at 1,
  # but several files should be at revision 2.  Also, two files should
  # be missing.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', 'A/mu', wc_rev=2)

  expected_status.add({
    'A/D/rho' : Item(status='  ', wc_rev=2),
    'A/C/alpha2' : Item(status='  ', wc_rev=2),
    'A/D/H/mu' : Item(status='  ', wc_rev=2),
    'A/B/F/iota' : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/mu', 'iota')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Issue 1091, alpha2 would now have the wrong checksum and so a
  # subsequent commit would fail
  svntest.main.file_append(alpha2_path, 'appended alpha2 text')
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/alpha2' : Item(verb='Sending'),
    })
  expected_status.tweak('A/C/alpha2', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Assure that attempts at local copy and move fail when a log
  # message is provided.
  expected_stderr = \
    ".*Local, non-commit operations do not take a log message"
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'cp', '-m', 'op fails', rho_path, D_path)
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'mv', '-m', 'op fails', rho_path, D_path)


#----------------------------------------------------------------------

# This test passes over ra_local certainly; we're adding it because at
# one time it failed over ra_neon.  Specifically, it failed when
# mod_dav_svn first started sending vsn-rsc-urls as "CR/path", and was
# sending bogus CR/paths for items within copied subtrees.

def receive_copy_in_update(sbox):
  "receive a copied directory during update"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy.
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Define a zillion paths in both working copies.
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  newG_path = os.path.join(wc_dir, 'A', 'B', 'newG')

  # Copy directory A/D to A/B/newG
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', G_path, newG_path)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/newG' : Item(verb='Adding'),
    })

  # Create expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/newG' : Item(status='  ', wc_rev=2),
    'A/B/newG/pi' : Item(status='  ', wc_rev=2),
    'A/B/newG/rho' : Item(status='  ', wc_rev=2),
    'A/B/newG/tau' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Now update the other working copy; it should receive a full add of
  # the newG directory and its contents.

  # Expected output of update
  expected_output = svntest.wc.State(wc_backup, {
    'A/B/newG' : Item(status='A '),
    'A/B/newG/pi' : Item(status='A '),
    'A/B/newG/rho' : Item(status='A '),
    'A/B/newG/tau' : Item(status='A '),
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/newG' : Item(),
    'A/B/newG/pi' : Item("This is the file 'pi'.\n"),
    'A/B/newG/rho' : Item("This is the file 'rho'.\n"),
    'A/B/newG/tau' : Item("This is the file 'tau'.\n"),
    })

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.add({
    'A/B/newG' : Item(status='  ', wc_rev=2),
    'A/B/newG/pi' : Item(status='  ', wc_rev=2),
    'A/B/newG/rho' : Item(status='  ', wc_rev=2),
    'A/B/newG/tau' : Item(status='  ', wc_rev=2),
    })

  # Do the update and check the results in three ways.
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


#----------------------------------------------------------------------

# Regression test for issue #683.  In particular, this bug prevented
# us from running 'svn cp -r N src_URL dst_URL' as a means of
# resurrecting a deleted directory.  Also, the final 'update' at the
# end of this test was uncovering a ghudson 'deleted' edge-case bug.
# (In particular, re-adding G to D, when D already had a 'deleted'
# entry for G.  The entry-merge wasn't overwriting the 'deleted'
# attribute, and thus the newly-added G was ending up disconnected
# from D.)
@Issue(683)
def resurrect_deleted_dir(sbox):
  "resurrect a deleted directory"

  sbox.build()
  wc_dir = sbox.wc_dir
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')

  # Delete directory A/D/G, commit that as r2.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', '--force',
                                     G_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G' : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G')
  expected_status.remove('A/D/G/pi')
  expected_status.remove('A/D/G/rho')
  expected_status.remove('A/D/G/tau')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Use 'svn cp URL@1 URL' to resurrect the deleted directory, where
  # the two URLs are identical.  This used to trigger a failure.
  url = sbox.repo_url + '/A/D/G'
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     url + '@1', url,
                                     '-m', 'logmsg')

  # For completeness' sake, update to HEAD, and verify we have a full
  # greek tree again, all at revision 3.

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G'     : Item(status='A '),
    'A/D/G/pi'  : Item(status='A '),
    'A/D/G/rho' : Item(status='A '),
    'A/D/G/tau' : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

def copy_deleted_dir_into_prefix(sbox):
  "copy a deleted dir to a prefix of its old path"

  sbox.build()
  wc_dir = sbox.wc_dir
  D_path = os.path.join(wc_dir, 'A', 'D')

  # Delete directory A/D, commit that as r2.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', '--force',
                                     D_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D' : Item(verb='Deleting'),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        wc_dir)

  # Ok, copy from a deleted URL into a prefix of that URL, this used to
  # result in an assert failing.
  url1 = sbox.repo_url + '/A/D/G'
  url2 = sbox.repo_url + '/A/D'
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     url1 + '@1', url2,
                                     '-m', 'logmsg')

#----------------------------------------------------------------------

# Test that we're enforcing proper 'svn cp' overwrite behavior.  Note
# that svn_fs_copy() will always overwrite its destination if an entry
# by the same name already exists.  However, libsvn_client should be
# doing existence checks to prevent directories from being
# overwritten, and files can't be overwritten because the RA layers
# are doing out-of-dateness checks during the commit.


def no_copy_overwrites(sbox):
  "svn cp URL URL cannot overwrite destination"

  sbox.build()

  wc_dir = sbox.wc_dir

  fileURL1 =  sbox.repo_url + "/A/B/E/alpha"
  fileURL2 =  sbox.repo_url + "/A/B/E/beta"
  dirURL1  =  sbox.repo_url + "/A/D/G"
  dirURL2  =  sbox.repo_url + "/A/D/H"

  # Expect out-of-date failure if 'svn cp URL URL' tries to overwrite a file
  svntest.actions.run_and_verify_svn("Whoa, I was able to overwrite a file!",
                                     None, svntest.verify.AnyOutput,
                                     'cp', fileURL1, fileURL2,
                                     '-m', 'fooogle')

  # Create A/D/H/G by running 'svn cp ...A/D/G .../A/D/H'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', dirURL1, dirURL2,
                                     '-m', 'fooogle')

  # Repeat the last command.  It should *fail* because A/D/H/G already exists.
  svntest.actions.run_and_verify_svn(
    "Whoa, I was able to overwrite a directory!",
    None, svntest.verify.AnyOutput,
    'cp', dirURL1, dirURL2,
    '-m', 'fooogle')

#----------------------------------------------------------------------

# Issue 845. WC -> WC copy should not overwrite base text-base
@Issue(845)
def no_wc_copy_overwrites(sbox):
  "svn cp PATH PATH cannot overwrite destination"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # File simply missing
  tau_path = os.path.join(wc_dir, 'A', 'D', 'G', 'tau')
  os.remove(tau_path)

  # Status before attempting copies
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/tau', status='! ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # These copies should fail
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'cp', pi_path, rho_path)
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'cp', pi_path, tau_path)

  # Status after failed copies should not have changed
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

# Takes out working-copy locks for A/B2 and child A/B2/E. At one stage
# during issue 749 the second lock cause an already-locked error.
@Issue(749)
def copy_modify_commit(sbox):
  "copy a tree and modify before commit"

  sbox.build()
  wc_dir = sbox.wc_dir
  B_path = os.path.join(wc_dir, 'A', 'B')
  B2_path = os.path.join(wc_dir, 'A', 'B2')

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     B_path, B2_path)

  alpha_path = os.path.join(wc_dir, 'A', 'B2', 'E', 'alpha')
  svntest.main.file_append(alpha_path, "modified alpha")

  expected_output = svntest.wc.State(wc_dir, {
    'A/B2' : Item(verb='Adding'),
    'A/B2/E/alpha' : Item(verb='Sending'),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Issue 591, at one point copying a file from URL to WC didn't copy
# properties.
@Issue(591)
def copy_files_with_properties(sbox):
  "copy files with properties"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Set a property on a file
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'pname', 'pval', rho_path)

  # and commit it
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='  ', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Set another property, but don't commit it yet
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'pname2', 'pval2', rho_path)

  # WC to WC copy of file with committed and uncommitted properties
  rho_wc_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho_wc')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', rho_path, rho_wc_path)

  # REPOS to WC copy of file with properties
  rho_url_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho_url')
  rho_url = sbox.repo_url + '/A/D/G/rho'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', rho_url, rho_url_path)

  # Properties are not visible in WC status 'A'
  expected_status.add({
    'A/D/G/rho' : Item(status=' M', wc_rev='2'),
    'A/D/G/rho_wc' : Item(status='A ', wc_rev='-', copied='+'),
    'A/D/G/rho_url' : Item(status='A ', wc_rev='-', copied='+'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Check properties explicitly
  svntest.actions.run_and_verify_svn(None, ['pval\n'], [],
                                     'propget', 'pname', rho_wc_path)
  svntest.actions.run_and_verify_svn(None, ['pval2\n'], [],
                                     'propget', 'pname2', rho_wc_path)
  svntest.actions.run_and_verify_svn(None, ['pval\n'], [],
                                     'propget', 'pname', rho_url_path)

  # Commit and properties are visible in status
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Sending'),
    'A/D/G/rho_wc' : Item(verb='Adding'),
    'A/D/G/rho_url' : Item(verb='Adding'),
    })
  expected_status.tweak('A/D/G/rho', status='  ', wc_rev=3)
  expected_status.remove('A/D/G/rho_wc', 'A/D/G/rho_url')
  expected_status.add({
    'A/D/G/rho_wc' : Item(status='  ', wc_rev=3),
    'A/D/G/rho_url' : Item(status='  ', wc_rev=3),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

# Issue 918
@Issue(918)
def copy_delete_commit(sbox):
  "copy a tree and delete part of it before commit"

  sbox.build()
  wc_dir = sbox.wc_dir
  B_path = os.path.join(wc_dir, 'A', 'B')
  B2_path = os.path.join(wc_dir, 'A', 'B2')

  # copy a tree
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     B_path, B2_path)

  # delete two files
  lambda_path = os.path.join(wc_dir, 'A', 'B2', 'lambda')
  alpha_path = os.path.join(wc_dir, 'A', 'B2', 'E', 'alpha')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', alpha_path, lambda_path)

  # commit copied tree containing a deleted file
  expected_output = svntest.wc.State(wc_dir, {
    'A/B2' : Item(verb='Adding'),
    'A/B2/lambda' : Item(verb='Deleting'),
    'A/B2/E/alpha' : Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        wc_dir)

  # copy a tree
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'A', 'B'),
                                     os.path.join(wc_dir, 'A', 'B3'))

  # delete a directory
  E_path = os.path.join(wc_dir, 'A', 'B3', 'E')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', E_path)

  # commit copied tree containing a deleted directory
  expected_output = svntest.wc.State(wc_dir, {
    'A/B3' : Item(verb='Adding'),
    'A/B3/E' : Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        wc_dir)


#----------------------------------------------------------------------
@Issues(931,932)
def mv_and_revert_directory(sbox):
  "move and revert a directory"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  new_E_path = os.path.join(F_path, 'E')

  # Issue 931: move failed to lock the directory being deleted
  svntest.actions.run_and_verify_svn(None, None, [], 'move',
                                     E_path, F_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', status='D ')
  expected_status.add({
    'A/B/F/E' : Item(status='A ', wc_rev='-', copied='+'),
    'A/B/F/E/alpha' : Item(status='  ', wc_rev='-', copied='+'),
    'A/B/F/E/beta' : Item(status='  ', wc_rev='-', copied='+'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Issue 932: revert failed to lock the parent directory
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '--recursive',
                                     new_E_path)
  expected_status.remove('A/B/F/E', 'A/B/F/E/alpha', 'A/B/F/E/beta')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
# Issue 982.  When copying a file with the executable bit set, the copied
# file should also have its executable bit set.
@Issue(982)
@SkipUnless(svntest.main.is_posix_os)
def copy_preserve_executable_bit(sbox):
  "executable bit should be preserved when copying"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Create two paths
  newpath1 = os.path.join(wc_dir, 'newfile1')
  newpath2 = os.path.join(wc_dir, 'newfile2')

  # Create the first file.
  svntest.main.file_append(newpath1, "a new file")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', newpath1)

  mode1 = os.stat(newpath1)[stat.ST_MODE]

  # Doing this to get the executable bit set on systems that support
  # that -- the property itself is not the point.
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     'svn:executable', 'on', newpath1)

  mode2 = os.stat(newpath1)[stat.ST_MODE]

  if mode1 == mode2:
    print("setting svn:executable did not change file's permissions")
    raise svntest.Failure

  # Commit the file
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'create file and set svn:executable',
                                     wc_dir)

  # Copy the file
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', newpath1, newpath2)

  mode3 = os.stat(newpath2)[stat.ST_MODE]

  # The mode on the original and copied file should be identical
  if mode2 != mode3:
    print("permissions on the copied file are not identical to original file")
    raise svntest.Failure

#----------------------------------------------------------------------
# Issue 1029, copy failed with a "working copy not locked" error
@Issue(1029)
def wc_to_repos(sbox):
  "working-copy to repository copy"

  sbox.build()
  wc_dir = sbox.wc_dir

  beta_path = os.path.join(wc_dir, "A", "B", "E", "beta")
  beta2_url = sbox.repo_url + "/A/B/E/beta2"
  H_path = os.path.join(wc_dir, "A", "D", "H")
  H2_url = sbox.repo_url + "/A/D/H2"

  # modify some items to be copied
  svntest.main.file_append(os.path.join(wc_dir, 'A', 'D', 'H', 'omega'),
                           "new otext\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'propset', 'foo', 'bar',
                                     beta_path)

  # copy a file
  svntest.actions.run_and_verify_svn(None, None, [], '-m', 'fumble file',
                                     'copy', beta_path, beta2_url)
  # and a directory
  svntest.actions.run_and_verify_svn(None, None, [], '-m', 'fumble dir',
                                     'copy', H_path, H2_url)
  # copy a file to a directory
  svntest.actions.run_and_verify_svn(None, None, [], '-m', 'fumble file',
                                     'copy', beta_path, H2_url)

  # update the working copy.  post-update mereinfo elision will remove
  # A/D/H2/beta's mergeinfo, leaving a local mod.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/beta2'  : Item(status='A '),
    'A/D/H2'       : Item(status='A '),
    'A/D/H2/chi'   : Item(status='A '),
    'A/D/H2/omega' : Item(status='A '),
    'A/D/H2/psi'   : Item(status='A '),
    'A/D/H2/beta'  : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/H/omega',
                      contents="This is the file 'omega'.\nnew otext\n")
  expected_disk.add({
    'A/B/E/beta2'  : Item("This is the file 'beta'.\n"),
    'A/D/H2/chi'   : Item("This is the file 'chi'.\n"),
    'A/D/H2/omega' : Item("This is the file 'omega'.\nnew otext\n"),
    'A/D/H2/psi'   : Item("This is the file 'psi'.\n"),
    'A/D/H2/beta'  : Item("This is the file 'beta'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 4)
  expected_status.add({
    'A/B/E/beta'   : Item(status=' M', wc_rev=4),
    'A/D/H/omega'  : Item(status='M ', wc_rev=4),
    'A/B/E/beta2'  : Item(status='  ', wc_rev=4),
    'A/D/H2'       : Item(status='  ', wc_rev=4),
    'A/D/H2/chi'   : Item(status='  ', wc_rev=4),
    'A/D/H2/omega' : Item(status='  ', wc_rev=4),
    'A/D/H2/psi'   : Item(status='  ', wc_rev=4),
    'A/D/H2/beta'  : Item(status='  ', wc_rev=4),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # check local property was copied
  svntest.actions.run_and_verify_svn(None, ['bar\n'], [],
                                     'propget', 'foo',
                                     beta_path + "2")

#----------------------------------------------------------------------
# Issue 1090: various use-cases of 'svn cp URL wc' where the
# repositories might be different, or be the same repository.
@Issues(1090,1444)
def repos_to_wc(sbox):
  "repository to working-copy copy"

  sbox.build()
  wc_dir = sbox.wc_dir

  # We have a standard repository and working copy.  Now we create a
  # second repository with the same greek tree, but different UUID.
  repo_dir       = sbox.repo_dir
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 1)

  # URL->wc copy:
  # copy a file and a directory from the same repository.
  # we should get some scheduled additions *with history*.
  E_url = sbox.repo_url + "/A/B/E"
  pi_url = sbox.repo_url + "/A/D/G/pi"
  pi_path = os.path.join(wc_dir, 'pi')

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', E_url, wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', pi_url, wc_dir)

  # Extra test: modify file ASAP to check there was a timestamp sleep
  svntest.main.file_append(pi_path, 'zig\n')

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output.add({
    'pi' : Item(status='A ', copied='+', wc_rev='-'),
    'E' :  Item(status='A ', copied='+', wc_rev='-'),
    'E/alpha' :  Item(status='  ', copied='+', wc_rev='-'),
    'E/beta'  :  Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # Modification will only show up if timestamps differ
  exit_code, out, err = svntest.main.run_svn(None, 'diff', pi_path)
  if err or not out:
    print("diff failed")
    raise svntest.Failure
  for line in out:
    if line == '+zig\n': # Crude check for diff-like output
      break
  else:
    print("diff output incorrect %s" % out)
    raise svntest.Failure

  # Revert everything and verify.
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)

  svntest.main.safe_rmtree(os.path.join(wc_dir, 'E'))

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # URL->wc copy:
  # Copy an empty directory from the same repository, see issue #1444.
  C_url = sbox.repo_url + "/A/C"

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', C_url, wc_dir)

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output.add({
    'C' :  Item(status='A ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # Revert everything and verify.
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)

  svntest.main.safe_rmtree(os.path.join(wc_dir, 'C'))

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # URL->wc copy:
  # copy a file and a directory from a foreign repository.
  # we should get some scheduled additions *without history*.
  E_url = other_repo_url + "/A/B/E"
  pi_url = other_repo_url + "/A/D/G/pi"

  # Expect an error in the directory case until we allow this copy to succeed.
  expected_error = "svn: E200007: Source URL '.*foreign repository"
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'copy', E_url, wc_dir)

  # But file case should work fine.
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', pi_url, wc_dir)

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output.add({
    'pi' : Item(status='A ',  wc_rev='0', entry_rev='1'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # Revert everything and verify.
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)
  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)

  # URL->wc copy:
  # Copy a directory to a pre-existing WC directory.
  # The source directory should be copied *under* the target directory.
  B_url = sbox.repo_url + "/A/B"
  D_dir = os.path.join(wc_dir, 'A', 'D')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', B_url, D_dir)

  expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output.add({
    'A/D/B'         : Item(status='A ', copied='+', wc_rev='-'),
    'A/D/B/lambda'  : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/B/E'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/B/E/beta'  : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/B/E/alpha' : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/B/F'       : Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # Validate the merge info of the copy destination (we expect none)
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'propget', SVN_PROP_MERGEINFO,
                                     os.path.join(D_dir, 'B'))

#----------------------------------------------------------------------
# Issue 1084: ra_svn move/copy bug
@Issue(1084)
def copy_to_root(sbox):
  'copy item to root of repository'

  sbox.build()
  wc_dir = sbox.wc_dir

  root = sbox.repo_url
  mu = root + '/A/mu'

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     '-m', '',
                                     mu, root)

  # Update to HEAD, and check to see if the files really were copied in the
  # repo

  expected_output = svntest.wc.State(wc_dir, {
    'mu': Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'mu': Item(contents="This is the file 'mu'.\n")
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'mu': Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------
@Issue(1367)
def url_copy_parent_into_child(sbox):
  "copy URL URL/subdir"

  sbox.build()
  wc_dir = sbox.wc_dir

  B_url = sbox.repo_url + "/A/B"
  F_url = sbox.repo_url + "/A/B/F"

  # Issue 1367 parent/child URL-to-URL was rejected.
  svntest.actions.run_and_verify_svn(None,
                                     ['\n', 'Committed revision 2.\n'], [],
                                     'cp',
                                     '-m', 'a can of worms',
                                     B_url, F_url)

  # Do an update to verify the copy worked
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F/B'         : Item(status='A '),
    'A/B/F/B/E'       : Item(status='A '),
    'A/B/F/B/E/alpha' : Item(status='A '),
    'A/B/F/B/E/beta'  : Item(status='A '),
    'A/B/F/B/F'       : Item(status='A '),
    'A/B/F/B/lambda'  : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/F/B'         : Item(),
    'A/B/F/B/E'       : Item(),
    'A/B/F/B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'A/B/F/B/E/beta'  : Item("This is the file 'beta'.\n"),
    'A/B/F/B/F'       : Item(),
    'A/B/F/B/lambda'  : Item("This is the file 'lambda'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/B/F/B'         : Item(status='  ', wc_rev=2),
    'A/B/F/B/E'       : Item(status='  ', wc_rev=2),
    'A/B/F/B/E/alpha' : Item(status='  ', wc_rev=2),
    'A/B/F/B/E/beta'  : Item(status='  ', wc_rev=2),
    'A/B/F/B/F'       : Item(status='  ', wc_rev=2),
    'A/B/F/B/lambda'  : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------
@Issue(1367)
def wc_copy_parent_into_child(sbox):
  "copy WC URL/subdir"

  sbox.build(create_wc = False)
  wc_dir = sbox.wc_dir

  B_url = sbox.repo_url + "/A/B"
  F_B_url = sbox.repo_url + "/A/B/F/B"

  # Want a smaller WC
  svntest.main.safe_rmtree(wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     B_url, wc_dir)

  # Issue 1367: A) copying '.' to URL failed with a parent/child
  # error, and also B) copying root of a working copy attempted to
  # lock the non-working copy parent directory.
  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  svntest.actions.run_and_verify_svn(None,
                                     ['Adding copy of        .\n',
                                     '\n', 'Committed revision 2.\n'], [],
                                     'cp',
                                     '-m', 'a larger can',
                                     '.', F_B_url)

  os.chdir(was_cwd)

  # Do an update to verify the copy worked
  expected_output = svntest.wc.State(wc_dir, {
    'F/B'         : Item(status='A '),
    'F/B/E'       : Item(status='A '),
    'F/B/E/alpha' : Item(status='A '),
    'F/B/E/beta'  : Item(status='A '),
    'F/B/F'       : Item(status='A '),
    'F/B/lambda'  : Item(status='A '),
    })
  expected_disk = svntest.wc.State('', {
    'E'           : Item(),
    'E/alpha'     : Item("This is the file 'alpha'.\n"),
    'E/beta'      : Item("This is the file 'beta'.\n"),
    'F'           : Item(),
    'lambda'      : Item("This is the file 'lambda'.\n"),
    'F/B'         : Item(),
    'F/B/E'       : Item(),
    'F/B/E/alpha' : Item("This is the file 'alpha'.\n"),
    'F/B/E/beta'  : Item("This is the file 'beta'.\n"),
    'F/B/F'       : Item(),
    'F/B/lambda'  : Item("This is the file 'lambda'.\n"),
    })
  expected_status = svntest.wc.State(wc_dir, {
    ''            : Item(status='  ', wc_rev=2),
    'E'           : Item(status='  ', wc_rev=2),
    'E/alpha'     : Item(status='  ', wc_rev=2),
    'E/beta'      : Item(status='  ', wc_rev=2),
    'F'           : Item(status='  ', wc_rev=2),
    'lambda'      : Item(status='  ', wc_rev=2),
    'F/B'         : Item(status='  ', wc_rev=2),
    'F/B/E'       : Item(status='  ', wc_rev=2),
    'F/B/E/alpha' : Item(status='  ', wc_rev=2),
    'F/B/E/beta'  : Item(status='  ', wc_rev=2),
    'F/B/F'       : Item(status='  ', wc_rev=2),
    'F/B/lambda'  : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------
# Issue 1419: at one point ra_neon->get_uuid() was failing on a
# non-existent public URL, which prevented us from resurrecting files
# (svn cp -rOLD URL wc).
@Issue(1419)
def resurrect_deleted_file(sbox):
  "resurrect a deleted file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete a file in the repository via immediate commit
  rho_url = sbox.repo_url + '/A/D/G/rho'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', rho_url, '-m', 'rev 2')

  # Update the wc to HEAD (r2)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(status='D '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G/rho')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/D/G/rho')
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # repos->wc copy, to resurrect deleted file.
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', rho_url + '@1', wc_dir)

  # status should now show the file scheduled for addition-with-history
  expected_status.add({
    'rho' : Item(status='A ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#-------------------------------------------------------------
# Regression tests for Issue #1297:
# svn diff failed after a repository to WC copy of a single file
# This test checks just that.
@Issue(1297)
def diff_repos_to_wc_copy(sbox):
  "copy file from repos to working copy and run diff"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  iota_repos_path = sbox.repo_url + '/iota'
  target_wc_path = os.path.join(wc_dir, 'new_file')

  # Copy a file from the repository to the working copy.
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     iota_repos_path, target_wc_path)

  # Run diff.
  svntest.actions.run_and_verify_svn(None, None, [], 'diff', wc_dir)


#-------------------------------------------------------------
@Issue(1473)
def repos_to_wc_copy_eol_keywords(sbox):
  "repos->WC copy with keyword or eol property set"

  # See issue #1473: repos->wc copy would seg fault if svn:keywords or
  # svn:eol were set on the copied file, because we'd be querying an
  # entry for keyword values when the entry was still null (because
  # not yet been fully installed in the wc).

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_repos_path = sbox.repo_url + '/iota'
  iota_wc_path = os.path.join(wc_dir, 'iota')
  target_wc_path = os.path.join(wc_dir, 'new_file')

  # Modify iota to make it checkworthy.
  svntest.main.file_write(iota_wc_path,
                          "Hello\nSubversion\n$LastChangedRevision$\n",
                          "ab")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:eol-style',
                                     'CRLF', iota_wc_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:keywords',
                                     'Rev', iota_wc_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', 'log msg',
                                     wc_dir)

  # Copy a file from the repository to the working copy.
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     iota_repos_path, target_wc_path)

  # The original bug was that the copy would seg fault.  So we test
  # that the copy target exists now; if it doesn't, it's probably
  # because of the segfault.  Note that the crash would be independent
  # of whether there are actually any line breaks or keywords in the
  # file's contents -- the mere existence of the property would
  # trigger the bug.
  if not os.path.exists(target_wc_path):
    raise svntest.Failure

  # Okay, if we got this far, we might as well make sure that the
  # translations/substitutions were done correctly:
  f = open(target_wc_path, "rb")
  raw_contents = f.read()
  f.seek(0, 0)
  line_contents = f.readlines()
  f.close()

  if re.match('[^\\r]\\n', raw_contents):
    raise svntest.Failure

  if not re.match('.*\$LastChangedRevision:\s*\d+\s*\$', line_contents[3]):
    raise svntest.Failure

#-------------------------------------------------------------
# Regression test for revision 7331, with commented-out parts for a further
# similar bug.

def revision_kinds_local_source(sbox):
  "revision-kind keywords with non-URL source"

  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Make a file with different content in each revision and WC; BASE != HEAD.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'), })
  svntest.main.file_append(mu_path, "New r2 text.\n")
  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None,
                                        None, wc_dir)
  svntest.main.file_append(mu_path, "New r3 text.\n")
  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None,
                                        None, wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'up', '-r2', mu_path)
  svntest.main.file_append(mu_path, "Working copy.\n")

  r1 = "This is the file 'mu'.\n"
  r2 = r1 + "New r2 text.\n"
  r3 = r2 + "New r3 text.\n"
  rWC = r2 + "Working copy.\n"

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=rWC)

  # Test the various revision-kind keywords, and none.
  sub_tests = [ ('file0', 2, rWC, None),
                ('file1', 3, r3, 'HEAD'),
                ('file2', 2, r2, 'BASE'),
                # ('file3', 2, r2, 'COMMITTED'),
                # ('file4', 1, r1, 'PREV'),
              ]

  for dst, from_rev, text, peg_rev in sub_tests:
    dst_path = os.path.join(wc_dir, dst)
    if peg_rev is None:
      svntest.actions.run_and_verify_svn(None, None, [], "copy",
                                         mu_path, dst_path)
    else:
      svntest.actions.run_and_verify_svn(None, None, [], "copy",
                                         mu_path + "@" + peg_rev, dst_path)
    expected_disk.add({ dst: Item(contents=text) })

    # Check that the copied-from revision == from_rev.
    exit_code, output, errput = svntest.main.run_svn(None, "info", dst_path)
    for line in output:
      if line.rstrip() == "Copied From Rev: " + str(from_rev):
        break
    else:
      print("%s should have been copied from revision %s" % (dst, from_rev))
      raise svntest.Failure

  # Check that the new files have the right contents
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())


#-------------------------------------------------------------
# Regression test for issue 1581.
@Issue(1581)
def copy_over_missing_file(sbox):
  "copy over a missing file"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  iota_path = os.path.join(wc_dir, 'iota')
  iota_url = sbox.repo_url + "/iota"

  # Make the target missing.
  os.remove(mu_path)

  # Try both wc->wc copy and repos->wc copy, expect failures:
  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                     'cp', iota_path, mu_path)

  svntest.actions.run_and_verify_svn(None, None, svntest.verify.AnyOutput,
                                    'cp', iota_url, mu_path)

  # Make sure that the working copy is not corrupted:
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output = svntest.wc.State(wc_dir, {'A/mu' : Item(verb='Restored')})
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)



#----------------------------------------------------------------------
#  Regression test for issue 1634
@Issue(1634)
def repos_to_wc_1634(sbox):
  "copy a deleted directory back from the repos"

  sbox.build()
  wc_dir = sbox.wc_dir

  # First delete a subdirectory and commit.
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  svntest.actions.run_and_verify_svn(None, None, [], 'delete', E_path)
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E' : Item(verb='Deleting'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Now copy the directory back.
  E_url = sbox.repo_url + "/A/B/E@1"
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', E_url, E_path)
  expected_status.add({
    'A/B/E'       :  Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E/alpha' :  Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E/beta'  :  Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/B/E'       :  Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E/alpha' :  Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E/beta'  :  Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
#  Regression test for issue 1814
@Issue(1814)
def double_uri_escaping_1814(sbox):
  "check for double URI escaping in svn ls -R"

  sbox.build(create_wc = False)

  base_url = sbox.repo_url + '/base'

  # rev. 2
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', '-m', 'mybase',
                                     base_url)

  orig_url = base_url + '/foo%20bar'

  # rev. 3
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', '-m', 'r1',
                                     orig_url)
  orig_rev = 3

  # rev. 4
  new_url = base_url + '/foo_bar'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', '-m', 'r2',
                                     orig_url, new_url)

  # This had failed with ra_neon because "foo bar" would be double-encoded
  # "foo bar" ==> "foo%20bar" ==> "foo%2520bar"
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ls', ('-r'+str(orig_rev)),
                                     '-R', base_url)


#----------------------------------------------------------------------
#  Regression test for issues 2404
@Issue(2404)
def wc_to_wc_copy_between_different_repos(sbox):
  "wc to wc copy attempts between different repos"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  sbox2 = sbox.clone_dependent()
  sbox2.build()
  wc2_dir = sbox2.wc_dir

  # Attempt a copy between different repositories.
  exit_code, out, err = svntest.main.run_svn(1, 'cp',
                                             os.path.join(wc2_dir, 'A'),
                                             os.path.join(wc_dir, 'A', 'B'))
  for line in err:
    if line.find("it is not from repository") != -1:
      break
  else:
    raise svntest.Failure

#----------------------------------------------------------------------
#  Regression test for issues 2101, 2020 and 3776
@Issues(2101,2020,3776)
def wc_to_wc_copy_deleted(sbox):
  "wc to wc copy with presence=not-present items"

  sbox.build()
  wc_dir = sbox.wc_dir

  B_path = os.path.join(wc_dir, 'A', 'B')
  B2_path = os.path.join(wc_dir, 'A', 'B2')

  # Schedule for delete
  svntest.actions.run_and_verify_svn(None, None, [], 'rm',
                                     os.path.join(B_path, 'E', 'alpha'),
                                     os.path.join(B_path, 'lambda'),
                                     os.path.join(B_path, 'F'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', 'A/B/lambda', 'A/B/F', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Commit to get state not-present
  expected_status.remove('A/B/E/alpha', 'A/B/lambda', 'A/B/F')
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha' : Item(verb='Deleting'),
    'A/B/lambda'  : Item(verb='Deleting'),
    'A/B/F'       : Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Copy including stuff in state not-present
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', B_path, B2_path)
  expected_status.add({
    'A/B2'         : Item(status='A ', wc_rev='-', copied='+'),
    'A/B2/E'       : Item(status='  ', wc_rev='-', copied='+'),
    'A/B2/E/beta'  : Item(status='  ', wc_rev='-', copied='+'),
    'A/B2/E/alpha' : Item(status='D ', wc_rev='-', copied='+'),
    'A/B2/lambda'  : Item(status='D ', wc_rev='-', copied='+'),
    'A/B2/F'       : Item(status='D ', wc_rev='-', copied='+'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Reverting the copied not-present is a no-op.
  svntest.main.run_svn(1, 'revert', os.path.join(B2_path, 'F'))
  svntest.main.run_svn(1, 'revert', os.path.join(B2_path, 'lambda'))
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Revert the entire copy including the schedule not-present bits
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '--recursive',
                                     B2_path)
  expected_status.remove('A/B2',
                         'A/B2/E',
                         'A/B2/E/beta',
                         'A/B2/E/alpha',
                         'A/B2/lambda',
                         'A/B2/F')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  svntest.main.safe_rmtree(B2_path)

  # Copy again and commit
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', B_path, B2_path)

  expected_status.add({
    'A/B2'              : Item(status='A ', copied='+', wc_rev='-'),
    'A/B2/lambda'       : Item(status='D ', copied='+', wc_rev='-'),
    'A/B2/F'            : Item(status='D ', copied='+', wc_rev='-'),
    'A/B2/E'            : Item(status='  ', copied='+', wc_rev='-'),
    'A/B2/E/alpha'      : Item(status='D ', copied='+', wc_rev='-'),
    'A/B2/E/beta'       : Item(status='  ', copied='+', wc_rev='-')
  })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_status.remove('A/B2/lambda', 'A/B2/F', 'A/B2/E/alpha')
  expected_status.tweak('A/B2', 'A/B2/E', 'A/B2/E/beta', status='  ',
                        copied=None, wc_rev=3)
  expected_output = svntest.wc.State(wc_dir, {
    'A/B2'         : Item(verb='Adding'),
    'A/B2/E/alpha' : Item(verb='Deleting'),
    'A/B2/lambda'  : Item(verb='Deleting'),
    'A/B2/F'       : Item(verb='Deleting'),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------
# Test for copy into a non-existent URL path
def url_to_non_existent_url_path(sbox):
  "svn cp src-URL non-existent-URL-path"

  sbox.build(create_wc = False)

  dirURL1 = sbox.repo_url + "/A/B/E"
  dirURL2 = sbox.repo_url + "/G/C/E/I"

  # Look for both possible versions of the error message, as the DAV
  # error is worded differently from that of other RA layers.
  msg = ".*: (" + \
        "|".join(["Path 'G(/C/E)?' not present",
                  ".*G(/C/E)?' path not found",
                  "File not found.*'/G/C/E/I'",
                  ]) + ")"

  # Expect failure on 'svn cp SRC DST' where one or more ancestor
  # directories of DST do not exist
  exit_code, out, err = svntest.main.run_svn(1, 'cp', dirURL1, dirURL2,
                                             '-m', 'fooogle')
  for err_line in err:
    if re.match (msg, err_line):
      break
  else:
    print("message \"%s\" not found in error output: %s" % (msg, err))
    raise svntest.Failure


#----------------------------------------------------------------------
# Test for a copying (URL to URL) an old rev of a deleted file in a
# deleted directory.
def non_existent_url_to_url(sbox):
  "svn cp oldrev-of-deleted-URL URL"

  sbox.build(create_wc = False)

  adg_url = sbox.repo_url + '/A/D/G'
  pi_url = sbox.repo_url + '/A/D/G/pi'
  new_url = sbox.repo_url + '/newfile'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'delete',
                                     adg_url, '-m', '')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy',
                                     pi_url + '@1', new_url,
                                     '-m', '')

#----------------------------------------------------------------------
def old_dir_url_to_url(sbox):
  "test URL to URL copying edge case"

  sbox.build(create_wc = False)

  adg_url = sbox.repo_url + '/A/D/G'
  pi_url = sbox.repo_url + '/A/D/G/pi'
  iota_url = sbox.repo_url + '/iota'
  new_url = sbox.repo_url + '/newfile'

  # Delete a directory
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'delete',
                                     adg_url, '-m', '')

  # Copy a file to where the directory used to be
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy',
                                     iota_url, adg_url,
                                     '-m', '')

  # Try copying a file that was in the deleted directory that is now a
  # file
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy',
                                     pi_url + '@1', new_url,
                                     '-m', '')



#----------------------------------------------------------------------
# Test fix for issue 2224 - copying wc dir to itself causes endless
# recursion
@Issue(2224)
def wc_copy_dir_to_itself(sbox):
  "copy wc dir to itself"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  dnames = ['A','A/B']

  for dirname in dnames:
    dir_path = os.path.join(wc_dir, dirname)

    # try to copy dir to itself
    svntest.actions.run_and_verify_svn(None, [],
                                       '.*Cannot copy .* into its own child.*',
                                       'copy', dir_path, dir_path)


#----------------------------------------------------------------------
@Issue(2153)
def mixed_wc_to_url(sbox):
  "copy a complex mixed-rev wc"

  # For issue 2153.
  #
  # Copy a mixed-revision wc (that also has some uncommitted local
  # mods, and an entry marked as 'deleted') to a URL.  Make sure the
  # copy gets the uncommitted mods, and does not contain the deleted
  # file.

  sbox.build()

  wc_dir = sbox.wc_dir
  Z_url = sbox.repo_url + '/A/D/Z'
  Z2_url = sbox.repo_url + '/A/D/Z2'
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  B_path = os.path.join(wc_dir, 'A', 'B')
  X_path = os.path.join(wc_dir, 'A', 'D', 'G', 'X')
  Y_path = os.path.join(wc_dir, 'A', 'D', 'G', 'Y')
  E_path = os.path.join(wc_dir, 'A', 'D', 'G', 'X', 'E')
  alpha_path = os.path.join(wc_dir, 'A', 'D', 'G', 'X', 'E', 'alpha')
  pi_path = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')

  # Remove A/D/G/pi, then commit that removal.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', pi_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', "Delete pi.", wc_dir)

  # Make a modification to A/D/G/rho, then commit that modification.
  svntest.main.file_append(rho_path, "\nFirst modification to rho.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', "Modify rho.", wc_dir)

  # Make another modification to A/D/G/rho, but don't commit it.
  svntest.main.file_append(rho_path, "Second modification to rho.\n")

  # Copy into the source, delete part of the copy, add a non-copied directory
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', B_path, X_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', alpha_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', Y_path)

  # Now copy local A/D/G to create new directory A/D/Z the repository.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', "Make a copy.",
                                     G_path, Z_url)
  expected_output = svntest.verify.UnorderedOutput([
    'A + A/D/Z/\n',
    '    (from A/D/G/:r1)\n',
    'A + A/D/Z/X/\n',
    '    (from A/B/:r1)\n',
    'D   A/D/Z/X/E/alpha\n',
    'A   A/D/Z/Y/\n',
    'D   A/D/Z/pi\n',
    'D   A/D/Z/rho\n',
    'A + A/D/Z/rho\n',
    '    (from A/D/G/rho:r3)\n',
    ])
  svntest.actions.run_and_verify_svnlook(None, expected_output, [],
                                         'changed', sbox.repo_dir,
                                         '--copy-info')

  # Copy from copied source
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', "Make a copy.",
                                     E_path, Z2_url)
  expected_output = svntest.verify.UnorderedOutput([
    'A + A/D/Z2/\n',
    '    (from A/B/E/:r1)\n',
    'D   A/D/Z2/alpha\n',
    ])
  svntest.actions.run_and_verify_svnlook(None, expected_output, [],
                                         'changed', sbox.repo_dir,
                                         '--copy-info')

  # Check out A/D/Z.  If it has pi, that's a bug; or if its rho does
  # not have the second local mod, that's also a bug.
  svntest.main.safe_rmtree(wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'co', Z_url, wc_dir)

  if os.path.exists(os.path.join(wc_dir, 'pi')):
    raise svntest.Failure("Path 'pi' exists but should be gone.")

  fp = open(os.path.join(wc_dir, 'rho'), 'r')
  found_it = 0
  for line in fp.readlines():
    if re.match("^Second modification to rho.", line):
      found_it = 1
  if not found_it:
    raise svntest.Failure("The second modification to rho didn't make it.")


#----------------------------------------------------------------------

# Issue 845 and 1516: WC replacement of files requires
# a second text-base and prop-base
@Issues(845,1516)
def wc_copy_replacement(sbox):
  "svn cp PATH PATH replace file"

  copy_replace(sbox, 1)

def wc_copy_replace_with_props(sbox):
  "svn cp PATH PATH replace file with props"

  copy_replace_with_props(sbox, 1)


def repos_to_wc_copy_replacement(sbox):
  "svn cp URL PATH replace file"

  copy_replace(sbox, 0)

def repos_to_wc_copy_replace_with_props(sbox):
  "svn cp URL PATH replace file with props"

  copy_replace_with_props(sbox, 0)

# See also delete_replace_delete() which does the same for a directory.
def delete_replaced_file(sbox):
  "delete a file scheduled for replacement"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # File scheduled for deletion.
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', rho_path)

  # Status before attempting copies
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Copy 'pi' over 'rho' with history.
  pi_src = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', pi_src, rho_path)

  # Check that file copied.
  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now delete replaced file.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm',
                                     '--force', rho_path)

  # Verify status after deletion.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


@Issue(2436)
def mv_unversioned_file(sbox):
  "move an unversioned file"
  # Issue #2436: Attempting to move an unversioned file would seg fault.
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  unver_path_1 = os.path.join(wc_dir, 'unversioned1')
  dest_path_1 = os.path.join(wc_dir, 'dest')
  svntest.main.file_append(unver_path_1, "an unversioned file")

  unver_path_2 = os.path.join(wc_dir, 'A', 'unversioned2')
  dest_path_2 = os.path.join(wc_dir, 'A', 'dest_forced')
  svntest.main.file_append(unver_path_2, "another unversioned file")

  # Try to move an unversioned file.
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*unversioned1.* is not under version control.*",
                                     'mv', unver_path_1, dest_path_1)

  # Try to forcibly move an unversioned file.
  svntest.actions.run_and_verify_svn(None, None,
                                     ".*unversioned2.* is not under version control.*",
                                     'mv',
                                     unver_path_2, dest_path_2)

@Issue(2435)
def force_move(sbox):
  "'move' should not lose local mods"
  # Issue #2435: 'svn move' / 'svn mv' can lose local modifications.
  sbox.build()
  wc_dir = sbox.wc_dir
  file_name = "iota"
  file_path = os.path.join(wc_dir, file_name)

  # modify the content
  file_handle = open(file_path, "a")
  file_handle.write("Added contents\n")
  file_handle.close()
  expected_file_content = [ "This is the file 'iota'.\n",
                            "Added contents\n",
                          ]

  # check for the new content
  file_handle = open(file_path, "r")
  modified_file_content = file_handle.readlines()
  file_handle.close()
  if modified_file_content != expected_file_content:
    raise svntest.Failure("Test setup failed. Incorrect file contents.")

  # force move the file
  move_output = [ "A         dest\n",
                  "D         iota\n",
                ]
  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  svntest.actions.run_and_verify_svn(None, move_output,
                                     [],
                                     'move',
                                     file_name, "dest")
  os.chdir(was_cwd)

  # check for the new content
  file_handle = open(os.path.join(wc_dir, "dest"), "r")
  modified_file_content = file_handle.readlines()
  file_handle.close()
  # Error if we dont find the modified contents...
  if modified_file_content != expected_file_content:
    raise svntest.Failure("File modifications were lost on 'move'")

  # Commit the move and make sure the new content actually reaches
  # the repository.
  expected_output = svntest.wc.State(wc_dir, {
    'iota': Item(verb='Deleting'),
    'dest': Item(verb='Adding'),
  })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove("iota")
  expected_status.add({
    'dest': Item(status='  ', wc_rev='2'),
  })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)
  svntest.actions.run_and_verify_svn('Cat file', expected_file_content, [],
                                     'cat',
                                     sbox.repo_url + '/dest')


def copy_copied_file_and_dir(sbox):
  "copy a copied file and dir"
  # Improve support for copy and move
  # Allow copy of copied items without a commit between

  sbox.build()
  wc_dir = sbox.wc_dir

  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  rho_copy_path_1 = os.path.join(wc_dir, 'A', 'D', 'rho_copy_1')
  rho_copy_path_2 = os.path.join(wc_dir, 'A', 'B', 'F', 'rho_copy_2')

  # Copy A/D/G/rho to A/D/rho_copy_1
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     rho_path, rho_copy_path_1)

  # Copy the copied file: A/D/rho_copy_1 to A/B/F/rho_copy_2
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     rho_copy_path_1, rho_copy_path_2)

  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  E_path_copy_1 = os.path.join(wc_dir, 'A', 'B', 'F', 'E_copy_1')
  E_path_copy_2 = os.path.join(wc_dir, 'A', 'D', 'G', 'E_copy_2')

  # Copy A/B/E to A/B/F/E_copy_1
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     E_path, E_path_copy_1)

  # Copy the copied dir: A/B/F/E_copy_1 to A/D/G/E_copy_2
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     E_path_copy_1, E_path_copy_2)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/rho_copy_1'       : Item(verb='Adding'),
    'A/B/F/rho_copy_2'     : Item(verb='Adding'),
    'A/B/F/E_copy_1/'      : Item(verb='Adding'),
    'A/D/G/E_copy_2/'      : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/rho_copy_1'       : Item(status='  ', wc_rev=2),
    'A/B/F/rho_copy_2'     : Item(status='  ', wc_rev=2),
    'A/B/F/E_copy_1'       : Item(status='  ', wc_rev=2),
    'A/B/F/E_copy_1/alpha' : Item(status='  ', wc_rev=2),
    'A/B/F/E_copy_1/beta'  : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_2'       : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_2/alpha' : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_2/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_copied_file_and_dir(sbox):
  "move a copied file and dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  rho_copy_path = os.path.join(wc_dir, 'A', 'D', 'rho_copy')
  rho_copy_move_path = os.path.join(wc_dir, 'A', 'B', 'F', 'rho_copy_moved')

  # Copy A/D/G/rho to A/D/rho_copy
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     rho_path, rho_copy_path)

  # Move the copied file: A/D/rho_copy to A/B/F/rho_copy_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     rho_copy_path, rho_copy_move_path)

  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  E_path_copy = os.path.join(wc_dir, 'A', 'B', 'F', 'E_copy')
  E_path_copy_move = os.path.join(wc_dir, 'A', 'D', 'G', 'E_copy_moved')

  # Copy A/B/E to A/B/F/E_copy
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     E_path, E_path_copy)

  # Move the copied file: A/B/F/E_copy to A/D/G/E_copy_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     E_path_copy, E_path_copy_move)

  # Created expected output tree for 'svn ci':
  # Since we are moving items that were only *scheduled* for addition
  # we expect only to additions when checking in, rather than a
  # deletion/addition pair.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F/rho_copy_moved' : Item(verb='Adding'),
    'A/D/G/E_copy_moved/'  : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/rho_copy_moved'     : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_moved'       : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_moved/alpha' : Item(status='  ', wc_rev=2),
    'A/D/G/E_copy_moved/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_moved_file_and_dir(sbox):
  "move a moved file and dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  rho_move_path = os.path.join(wc_dir, 'A', 'D', 'rho_moved')
  rho_move_moved_path = os.path.join(wc_dir, 'A', 'B', 'F', 'rho_move_moved')

  # Move A/D/G/rho to A/D/rho_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     rho_path, rho_move_path)

  # Move the moved file: A/D/rho_moved to A/B/F/rho_move_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     rho_move_path, rho_move_moved_path)

  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  E_path_moved = os.path.join(wc_dir, 'A', 'B', 'F', 'E_moved')
  E_path_move_moved = os.path.join(wc_dir, 'A', 'D', 'G', 'E_move_moved')

  # Copy A/B/E to A/B/F/E_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     E_path, E_path_moved)

  # Move the moved file: A/B/F/E_moved to A/D/G/E_move_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     E_path_moved, E_path_move_moved)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E'                : Item(verb='Deleting'),
    'A/D/G/E_move_moved/'  : Item(verb='Adding'),
    'A/D/G/rho'            : Item(verb='Deleting'),
    'A/B/F/rho_move_moved' : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/G/E_move_moved/'      : Item(status='  ', wc_rev=2),
    'A/D/G/E_move_moved/alpha' : Item(status='  ', wc_rev=2),
    'A/D/G/E_move_moved/beta'  : Item(status='  ', wc_rev=2),
    'A/B/F/rho_move_moved'     : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/B/E',
                         'A/B/E/alpha',
                         'A/B/E/beta',
                         'A/D/G/rho')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_file_within_moved_dir(sbox):
  "move a file twice within a moved dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  D_path_moved = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved')

  # Move A/B/D to A/B/F/D_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     D_path, D_path_moved)

  chi_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H', 'chi')
  chi_moved_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved',
                                'H', 'chi_moved')
  chi_moved_again_path = os.path.join(wc_dir, 'A', 'B', 'F',
                                      'D_moved', 'H', 'chi_moved_again')

  # Move A/B/F/D_moved/H/chi to A/B/F/D_moved/H/chi_moved
  # then move that to A/B/F/D_moved/H/chi_moved_again
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     chi_path, chi_moved_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     chi_moved_path,
                                     chi_moved_again_path)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F/D_moved/'                  : Item(verb='Adding'),
    'A/B/F/D_moved/H/chi'             : Item(verb='Deleting'),
    'A/B/F/D_moved/H/chi_moved_again' : Item(verb='Adding'),
    'A/D'                             : Item(verb='Deleting'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/D_moved'                   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/gamma'             : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G'                 : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/pi'              : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/rho'             : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/tau'             : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H'                 : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H/omega'           : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H/psi'             : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H/chi_moved_again' : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/D',
                         'A/D/gamma',
                         'A/D/G',
                         'A/D/G/pi',
                         'A/D/G/rho',
                         'A/D/G/tau',
                         'A/D/H',
                         'A/D/H/chi',
                         'A/D/H/omega',
                         'A/D/H/psi',
                         )

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_file_out_of_moved_dir(sbox):
  "move a file out of a moved dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  D_path_moved = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved')

  # Move A/B/D to A/B/F/D_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     D_path, D_path_moved)

  chi_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H', 'chi')
  chi_moved_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved',
                                'H', 'chi_moved')
  chi_moved_again_path = os.path.join(wc_dir, 'A', 'C', 'chi_moved_again')

  # Move A/B/F/D_moved/H/chi to A/B/F/D_moved/H/chi_moved
  # then move that to A/C/chi_moved_again
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     chi_path, chi_moved_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     chi_moved_path,
                                     chi_moved_again_path)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/F/D_moved/'      : Item(verb='Adding'),
    'A/B/F/D_moved/H/chi' : Item(verb='Deleting'),
    'A/C/chi_moved_again' : Item(verb='Adding'),
    'A/D'                 : Item(verb='Deleting'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/D_moved'         : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/gamma'   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G'       : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/pi'    : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/rho'   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/tau'   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H'       : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H/omega' : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H/psi'   : Item(status='  ', wc_rev=2),
    'A/C/chi_moved_again'   : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/D',
                         'A/D/gamma',
                         'A/D/G',
                         'A/D/G/pi',
                         'A/D/G/rho',
                         'A/D/G/tau',
                         'A/D/H',
                         'A/D/H/chi',
                         'A/D/H/omega',
                         'A/D/H/psi',
                         )

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_dir_within_moved_dir(sbox):
  "move a dir twice within a moved dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  D_path_moved = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved')

  # Move A/D to A/B/F/D_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     D_path, D_path_moved)

  H_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H')
  H_moved_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H_moved')
  H_moved_again_path = os.path.join(wc_dir, 'A', 'B', 'F',
                                    'D_moved', 'H_moved_again')

  # Move A/B/F/D_moved/H to A/B/F/D_moved/H_moved
  # then move that to A/B/F/D_moved/H_moved_again
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     H_path, H_moved_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     H_moved_path,
                                     H_moved_again_path)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/D'                         : Item(verb='Deleting'),
    'A/B/F/D_moved'               : Item(verb='Adding'),
    'A/B/F/D_moved/H'             : Item(verb='Deleting'),
    'A/B/F/D_moved/H_moved_again' : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/D_moved'                     : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/gamma'               : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G'                   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/pi'                : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/rho'               : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/tau'               : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H_moved_again'       : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H_moved_again/omega' : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H_moved_again/psi'   : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/H_moved_again/chi'   : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/D',
                         'A/D/gamma',
                         'A/D/G',
                         'A/D/G/pi',
                         'A/D/G/rho',
                         'A/D/G/tau',
                         'A/D/H',
                         'A/D/H/chi',
                         'A/D/H/omega',
                         'A/D/H/psi',
                         )

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


def move_dir_out_of_moved_dir(sbox):
  "move a dir out of a moved dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  D_path_moved = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved')

  # Move A/D to A/B/F/D_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     D_path, D_path_moved)

  H_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H')
  H_moved_path = os.path.join(wc_dir, 'A', 'B', 'F', 'D_moved', 'H_moved')
  H_moved_again_path = os.path.join(wc_dir, 'A', 'C', 'H_moved_again')

  # Move A/B/F/D_moved/H to A/B/F/D_moved/H_moved
  # then move that to A/C/H_moved_again
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     H_path, H_moved_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     H_moved_path,
                                     H_moved_again_path)

  # Created expected output tree for 'svn ci':
  expected_output = svntest.wc.State(wc_dir, {
    'A/D'               : Item(verb='Deleting'),
    'A/B/F/D_moved'     : Item(verb='Adding'),
    'A/B/F/D_moved/H'   : Item(verb='Deleting'),
    'A/C/H_moved_again' : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/D_moved'           : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/gamma'     : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G'         : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/pi'      : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/rho'     : Item(status='  ', wc_rev=2),
    'A/B/F/D_moved/G/tau'     : Item(status='  ', wc_rev=2),
    'A/C/H_moved_again'       : Item(status='  ', wc_rev=2),
    'A/C/H_moved_again/omega' : Item(status='  ', wc_rev=2),
    'A/C/H_moved_again/psi'   : Item(status='  ', wc_rev=2),
    'A/C/H_moved_again/chi'   : Item(status='  ', wc_rev=2),
    })

  expected_status.remove('A/D',
                         'A/D/gamma',
                         'A/D/G',
                         'A/D/G/pi',
                         'A/D/G/rho',
                         'A/D/G/tau',
                         'A/D/H',
                         'A/D/H/chi',
                         'A/D/H/omega',
                         'A/D/H/psi',
                         )

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

# Includes regression testing for issue #3429 ("svn mv A B; svn mv B A"
# generates replace without history).
@Issue(3429)
def move_file_back_and_forth(sbox):
  "move a moved file back to original location"

  sbox.build()
  wc_dir = sbox.wc_dir

  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  rho_move_path = os.path.join(wc_dir, 'A', 'D', 'rho_moved')

  # Move A/D/G/rho away from and then back to its original path
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     rho_path, rho_move_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     rho_move_path, rho_path)

  # Check expected status before commit
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/G/rho' : Item(status='R ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Commit, and check expected output and status
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Replacing'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/G/rho' : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


# Includes regression testing for issue #3429 ("svn mv A B; svn mv B A"
# generates replace without history).
@Issue(3429)
def move_dir_back_and_forth(sbox):
  "move a moved dir back to original location"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  D_path = os.path.join(wc_dir, 'A', 'D')
  D_move_path = os.path.join(wc_dir, 'D_moved')

  # Move A/D to D_moved
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     D_path, D_move_path)

  # Move the moved dir: D_moved back to its starting
  # location at A/D.

  if svntest.main.wc_is_singledb(wc_dir):
    # In single-db target is gone on-disk after it was moved away, so this
    # move works ok
    expected_err = []
  else:
    # In !SINGLE_DB the target of the copy exists on-dir, so svn tries
    # to move the file below the deleted directory
    expected_err = '.*Cannot copy to .*as it is scheduled for deletion'

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'mv', D_move_path, D_path)

  if svntest.main.wc_is_singledb(wc_dir):
    # Verify that the status indicates a replace with history
    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_status.add({
      'A/D'               : Item(status='R ', copied='+', wc_rev='-'),
      'A/D/G'             : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/G/pi'          : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/G/rho'         : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/G/tau'         : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/gamma'         : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/H'             : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/H/chi'         : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/H/omega'       : Item(status='  ', copied='+', wc_rev='-'),
      'A/D/H/psi'         : Item(status='  ', copied='+', wc_rev='-'),
      })
    svntest.actions.run_and_verify_status(wc_dir, expected_status)

def copy_move_added_paths(sbox):
  "copy and move added paths without commits"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new file and schedule it for addition
  upsilon_path = os.path.join(wc_dir, 'A', 'D', 'upsilon')
  svntest.main.file_write(upsilon_path, "This is the file 'upsilon'\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', upsilon_path)

  # Create a dir with children and schedule it for addition
  I_path = os.path.join(wc_dir, 'A', 'D', 'I')
  J_path = os.path.join(I_path, 'J')
  eta_path = os.path.join(I_path, 'eta')
  theta_path = os.path.join(I_path, 'theta')
  kappa_path = os.path.join(J_path, 'kappa')
  os.mkdir(I_path)
  os.mkdir(J_path)
  svntest.main.file_write(eta_path, "This is the file 'eta'\n")
  svntest.main.file_write(theta_path, "This is the file 'theta'\n")
  svntest.main.file_write(kappa_path, "This is the file 'kappa'\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', I_path)

  # Create another dir and schedule it for addition
  K_path = os.path.join(wc_dir, 'K')
  os.mkdir(K_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'add', K_path)

  # Verify all the adds took place correctly.
  expected_status_after_adds = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status_after_adds.add({
    'A/D/I'         : Item(status='A ', wc_rev='0'),
    'A/D/I/eta'     : Item(status='A ', wc_rev='0'),
    'A/D/I/J'       : Item(status='A ', wc_rev='0'),
    'A/D/I/J/kappa' : Item(status='A ', wc_rev='0'),
    'A/D/I/theta'   : Item(status='A ', wc_rev='0'),
    'A/D/upsilon'   : Item(status='A ', wc_rev='0'),
    'K'             : Item(status='A ', wc_rev='0'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status_after_adds)

  # Scatter some unversioned paths within the added dir I.
  unversioned_path_1 = os.path.join(I_path, 'unversioned1')
  unversioned_path_2 = os.path.join(J_path, 'unversioned2')
  L_path = os.path.join(I_path, "L_UNVERSIONED")
  unversioned_path_3 = os.path.join(L_path, 'unversioned3')
  svntest.main.file_write(unversioned_path_1, "An unversioned file\n")
  svntest.main.file_write(unversioned_path_2, "An unversioned file\n")
  os.mkdir(L_path)
  svntest.main.file_write(unversioned_path_3, "An unversioned file\n")

  # Copy added dir A/D/I to added dir K/I
  I_copy_path = os.path.join(K_path, 'I')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     I_path, I_copy_path)

  # Copy added file A/D/upsilon into added dir K
  upsilon_copy_path = os.path.join(K_path, 'upsilon')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     upsilon_path, upsilon_copy_path)

  # Move added file A/D/upsilon to upsilon,
  # then move it again to A/upsilon
  upsilon_move_path = os.path.join(wc_dir, 'upsilon')
  upsilon_move_path_2 = os.path.join(wc_dir, 'A', 'upsilon')
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     upsilon_path, upsilon_move_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     upsilon_move_path, upsilon_move_path_2)

  # Move added dir A/D/I to A/B/I,
  # then move it again to A/D/H/I
  I_move_path = os.path.join(wc_dir, 'A', 'B', 'I')
  I_move_path_2 = os.path.join(wc_dir, 'A', 'D', 'H', 'I')
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     I_path, I_move_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     I_move_path, I_move_path_2)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/I'         : Item(verb='Adding'),
    'A/D/H/I/J'       : Item(verb='Adding'),
    'A/D/H/I/J/kappa' : Item(verb='Adding'),
    'A/D/H/I/eta'     : Item(verb='Adding'),
    'A/D/H/I/theta'   : Item(verb='Adding'),
    'A/upsilon'       : Item(verb='Adding'),
    'K'               : Item(verb='Adding'),
    'K/I'             : Item(verb='Adding'),
    'K/I/J'           : Item(verb='Adding'),
    'K/I/J/kappa'     : Item(verb='Adding'),
    'K/I/eta'         : Item(verb='Adding'),
    'K/I/theta'       : Item(verb='Adding'),
    'K/upsilon'       : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/H/I'         : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J/kappa' : Item(status='  ', wc_rev=2),
    'A/D/H/I/eta'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/theta'   : Item(status='  ', wc_rev=2),
    'A/upsilon'       : Item(status='  ', wc_rev=2),
    'K'               : Item(status='  ', wc_rev=2),
    'K/I'             : Item(status='  ', wc_rev=2),
    'K/I/J'           : Item(status='  ', wc_rev=2),
    'K/I/J/kappa'     : Item(status='  ', wc_rev=2),
    'K/I/eta'         : Item(status='  ', wc_rev=2),
    'K/I/theta'       : Item(status='  ', wc_rev=2),
    'K/upsilon'       : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Run_and_verify_commit() doesn't handle status of unversioned paths
  # so manually confirm unversioned paths got copied and moved too.
  unversioned_paths = [
    os.path.join(wc_dir, 'A', 'D', 'H', 'I', 'unversioned1'),
    os.path.join(wc_dir, 'A', 'D', 'H', 'I', 'L_UNVERSIONED'),
    os.path.join(wc_dir, 'A', 'D', 'H', 'I', 'L_UNVERSIONED',
                 'unversioned3'),
    os.path.join(wc_dir, 'A', 'D', 'H', 'I', 'J', 'unversioned2'),
    os.path.join(wc_dir, 'K', 'I', 'unversioned1'),
    os.path.join(wc_dir, 'K', 'I', 'L_UNVERSIONED'),
    os.path.join(wc_dir, 'K', 'I', 'L_UNVERSIONED', 'unversioned3'),
    os.path.join(wc_dir, 'K', 'I', 'J', 'unversioned2')]
  for path in unversioned_paths:
    if not os.path.exists(path):
      raise svntest.Failure("Unversioned path '%s' not found." % path)

def copy_added_paths_with_props(sbox):
  "copy added uncommitted paths with props"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new file, schedule it for addition and set properties
  upsilon_path = os.path.join(wc_dir, 'A', 'D', 'upsilon')
  svntest.main.file_write(upsilon_path, "This is the file 'upsilon'\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', upsilon_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     'foo', 'bar', upsilon_path)

  # Create a dir and schedule it for addition and set properties
  I_path = os.path.join(wc_dir, 'A', 'D', 'I')
  os.mkdir(I_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'add', I_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     'foo', 'bar', I_path)

  # Verify all the adds took place correctly.
  expected_status_after_adds = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status_after_adds.add({
    'A/D/upsilon'   : Item(status='A ', wc_rev='0'),
    'A/D/I'         : Item(status='A ', wc_rev='0'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status_after_adds)

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/D/upsilon' : Item(props={'foo' : 'bar'},
                         contents="This is the file 'upsilon'\n"),
    'A/D/I'       : Item(props={'foo' : 'bar'}),
    })

  # Read disk state with props
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)

  # Compare actual vs. expected disk trees.
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Copy added dir I to dir A/C
  I_copy_path = os.path.join(wc_dir, 'A', 'C', 'I')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     I_path, I_copy_path)

  # Copy added file A/upsilon into dir A/C
  upsilon_copy_path = os.path.join(wc_dir, 'A', 'C', 'upsilon')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     upsilon_path, upsilon_copy_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/upsilon'     : Item(verb='Adding'),
    'A/D/I'           : Item(verb='Adding'),
    'A/C/upsilon'     : Item(verb='Adding'),
    'A/C/I'           : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/upsilon'     : Item(status='  ', wc_rev=2),
    'A/D/I'           : Item(status='  ', wc_rev=2),
    'A/C/upsilon'     : Item(status='  ', wc_rev=2),
    'A/C/I'           : Item(status='  ', wc_rev=2),
    })

  # Tweak expected disk tree
  expected_disk.add({
    'A/C/upsilon' : Item(props={ 'foo' : 'bar'},
                         contents="This is the file 'upsilon'\n"),
    'A/C/I'       : Item(props={ 'foo' : 'bar'}),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)
  # Read disk state with props
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)

  # Compare actual vs. expected disk trees.
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

def copy_added_paths_to_URL(sbox):
  "copy added path to URL"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new file and schedule it for addition
  upsilon_path = os.path.join(wc_dir, 'A', 'D', 'upsilon')
  svntest.main.file_write(upsilon_path, "This is the file 'upsilon'\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', upsilon_path)

  # Create a dir with children and schedule it for addition
  I_path = os.path.join(wc_dir, 'A', 'D', 'I')
  J_path = os.path.join(I_path, 'J')
  eta_path = os.path.join(I_path, 'eta')
  theta_path = os.path.join(I_path, 'theta')
  kappa_path = os.path.join(J_path, 'kappa')
  os.mkdir(I_path)
  os.mkdir(J_path)
  svntest.main.file_write(eta_path, "This is the file 'eta'\n")
  svntest.main.file_write(theta_path, "This is the file 'theta'\n")
  svntest.main.file_write(kappa_path, "This is the file 'kappa'\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', I_path)

  # Verify all the adds took place correctly.
  expected_status_after_adds = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status_after_adds.add({
    'A/D/I'         : Item(status='A ', wc_rev='0'),
    'A/D/I/eta'     : Item(status='A ', wc_rev='0'),
    'A/D/I/J'       : Item(status='A ', wc_rev='0'),
    'A/D/I/J/kappa' : Item(status='A ', wc_rev='0'),
    'A/D/I/theta'   : Item(status='A ', wc_rev='0'),
    'A/D/upsilon'   : Item(status='A ', wc_rev='0'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status_after_adds)

  # Scatter some unversioned paths within the added dir I.
  # These don't get copied in a WC->URL copy obviously.
  unversioned_path_1 = os.path.join(I_path, 'unversioned1')
  unversioned_path_2 = os.path.join(J_path, 'unversioned2')
  L_path = os.path.join(I_path, "L_UNVERSIONED")
  unversioned_path_3 = os.path.join(L_path, 'unversioned3')
  svntest.main.file_write(unversioned_path_1, "An unversioned file\n")
  svntest.main.file_write(unversioned_path_2, "An unversioned file\n")
  os.mkdir(L_path)
  svntest.main.file_write(unversioned_path_3, "An unversioned file\n")

  # Copy added file A/D/upsilon to URL://A/C/upsilon
  upsilon_copy_URL = sbox.repo_url + '/A/C/upsilon'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', '',
                                     upsilon_path, upsilon_copy_URL)

  # Validate the merge info of the copy destination (we expect none).
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'propget',
                                     SVN_PROP_MERGEINFO, upsilon_copy_URL)

  # Copy added dir A/D/I to URL://A/D/G/I
  I_copy_URL = sbox.repo_url + '/A/D/G/I'
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', '',
                                     I_path, I_copy_URL)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/I'         : Item(verb='Adding'),
    'A/D/I/J'       : Item(verb='Adding'),
    'A/D/I/J/kappa' : Item(verb='Adding'),
    'A/D/I/eta'     : Item(verb='Adding'),
    'A/D/I/theta'   : Item(verb='Adding'),
    'A/D/upsilon'   : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/I'         : Item(status='  ', wc_rev=4),
    'A/D/I/J'       : Item(status='  ', wc_rev=4),
    'A/D/I/J/kappa' : Item(status='  ', wc_rev=4),
    'A/D/I/eta'     : Item(status='  ', wc_rev=4),
    'A/D/I/theta'   : Item(status='  ', wc_rev=4),
    'A/D/upsilon'   : Item(status='  ', wc_rev=4),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Created expected output for update
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/I'         : Item(status='A '),
    'A/D/G/I/theta'   : Item(status='A '),
    'A/D/G/I/J'       : Item(status='A '),
    'A/D/G/I/J/kappa' : Item(status='A '),
    'A/D/G/I/eta'     : Item(status='A '),
    'A/C/upsilon'     : Item(status='A '),
    })

  # Created expected disk for update
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/D/G/I'                          : Item(),
    'A/D/G/I/theta'                    : Item("This is the file 'theta'\n"),
    'A/D/G/I/J'                        : Item(),
    'A/D/G/I/J/kappa'                  : Item("This is the file 'kappa'\n"),
    'A/D/G/I/eta'                      : Item("This is the file 'eta'\n"),
    'A/C/upsilon'                      : Item("This is the file 'upsilon'\n"),
    'A/D/I'                            : Item(),
    'A/D/I/J'                          : Item(),
    'A/D/I/J/kappa'                    : Item("This is the file 'kappa'\n"),
    'A/D/I/eta'                        : Item("This is the file 'eta'\n"),
    'A/D/I/theta'                      : Item("This is the file 'theta'\n"),
    'A/D/upsilon'                      : Item("This is the file 'upsilon'\n"),
    'A/D/I/L_UNVERSIONED/unversioned3' : Item("An unversioned file\n"),
    'A/D/I/L_UNVERSIONED'              : Item(),
    'A/D/I/unversioned1'               : Item("An unversioned file\n"),
    'A/D/I/J/unversioned2'             : Item("An unversioned file\n"),
    })

  # Some more changes to the expected_status to reflect post update WC
  expected_status.tweak(wc_rev=4)
  expected_status.add({
    'A/C'             : Item(status='  ', wc_rev=4),
    'A/C/upsilon'     : Item(status='  ', wc_rev=4),
    'A/D/G'           : Item(status='  ', wc_rev=4),
    'A/D/G/I'         : Item(status='  ', wc_rev=4),
    'A/D/G/I/theta'   : Item(status='  ', wc_rev=4),
    'A/D/G/I/J'       : Item(status='  ', wc_rev=4),
    'A/D/G/I/J/kappa' : Item(status='  ', wc_rev=4),
    'A/D/G/I/eta'     : Item(status='  ', wc_rev=4),
    })

  # Update WC, the WC->URL copies above should be added
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


# Issue #1869.
@Issue(1869)
def move_to_relative_paths(sbox):
  "move file using relative dst path names"

  sbox.build()
  wc_dir = sbox.wc_dir
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  rel_path = os.path.join('..', '..', '..')

  current_dir = os.getcwd()
  os.chdir(E_path)
  svntest.main.run_svn(None, 'mv', 'beta', rel_path)
  os.chdir(current_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'beta'        : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E/beta'  : Item(status='D ', wc_rev='1')
  })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
def move_from_relative_paths(sbox):
  "move file using relative src path names"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  beta_rel_path = os.path.join('..', 'E', 'beta')

  current_dir = os.getcwd()
  os.chdir(F_path)
  svntest.main.run_svn(None, 'mv', beta_rel_path, '.')
  os.chdir(current_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/beta'  : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E/beta'  : Item(status='D ', wc_rev='1')
  })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
def copy_to_relative_paths(sbox):
  "copy file using relative dst path names"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  rel_path = os.path.join('..', '..', '..')

  current_dir = os.getcwd()
  os.chdir(E_path)
  svntest.main.run_svn(None, 'cp', 'beta', rel_path)
  os.chdir(current_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'beta'        : Item(status='A ', copied='+', wc_rev='-'),
  })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------
def copy_from_relative_paths(sbox):
  "copy file using relative src path names"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  beta_rel_path = os.path.join('..', 'E', 'beta')

  current_dir = os.getcwd()
  os.chdir(F_path)
  svntest.main.run_svn(None, 'cp', beta_rel_path, '.')
  os.chdir(current_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/beta'  : Item(status='A ', copied='+', wc_rev='-'),
  })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


#----------------------------------------------------------------------

# Test moving multiple files within a wc.

def move_multiple_wc(sbox):
  "svn mv multiple files to a common directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  C_path = os.path.join(wc_dir, 'A', 'C')

  # Move chi, psi, omega and E to A/C
  svntest.actions.run_and_verify_svn(None, None, [], 'mv', chi_path, psi_path,
                                     omega_path, E_path, C_path)

  # Create expected output
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(verb='Adding'),
    'A/C/psi'     : Item(verb='Adding'),
    'A/C/omega'   : Item(verb='Adding'),
    'A/C/E'       : Item(verb='Adding'),
    'A/D/H/chi'   : Item(verb='Deleting'),
    'A/D/H/psi'   : Item(verb='Deleting'),
    'A/D/H/omega' : Item(verb='Deleting'),
    'A/B/E'       : Item(verb='Deleting'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Add the moved files
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=2),
    'A/C/psi'     : Item(status='  ', wc_rev=2),
    'A/C/omega'   : Item(status='  ', wc_rev=2),
    'A/C/E'       : Item(status='  ', wc_rev=2),
    'A/C/E/alpha' : Item(status='  ', wc_rev=2),
    'A/C/E/beta'  : Item(status='  ', wc_rev=2),
    })

  # Removed the moved files
  expected_status.remove('A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega', 'A/B/E/alpha',
                         'A/B/E/beta', 'A/B/E')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Test copying multiple files within a wc.

def copy_multiple_wc(sbox):
  "svn cp multiple files to a common directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  C_path = os.path.join(wc_dir, 'A', 'C')

  # Copy chi, psi, omega and E to A/C
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', chi_path, psi_path,
                                     omega_path, E_path, C_path)

  # Create expected output
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(verb='Adding'),
    'A/C/psi'     : Item(verb='Adding'),
    'A/C/omega'   : Item(verb='Adding'),
    'A/C/E'       : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Add the moved files
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=2),
    'A/C/psi'     : Item(status='  ', wc_rev=2),
    'A/C/omega'   : Item(status='  ', wc_rev=2),
    'A/C/E'       : Item(status='  ', wc_rev=2),
    'A/C/E/alpha' : Item(status='  ', wc_rev=2),
    'A/C/E/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------

# Test moving multiple files within a repo.

def move_multiple_repo(sbox):
  "move multiple files within a repo"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_url = sbox.repo_url + '/A/D/H/chi'
  psi_url = sbox.repo_url + '/A/D/H/psi'
  omega_url = sbox.repo_url + '/A/D/H/omega'
  E_url = sbox.repo_url + '/A/B/E'
  C_url = sbox.repo_url + '/A/C'

  # Move three files and a directory in the repo to a different location
  # in the repo
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     chi_url, psi_url, omega_url, E_url, C_url,
                                     '-m', 'logmsg')

  # Update to HEAD, and check to see if the files really moved in the repo

  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(status='A '),
    'A/C/psi'     : Item(status='A '),
    'A/C/omega'   : Item(status='A '),
    'A/C/E'       : Item(status='A '),
    'A/C/E/alpha' : Item(status='A '),
    'A/C/E/beta'  : Item(status='A '),
    'A/D/H/chi'   : Item(status='D '),
    'A/D/H/psi'   : Item(status='D '),
    'A/D/H/omega' : Item(status='D '),
    'A/B/E'       : Item(status='D '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega', 'A/B/E/alpha',
                       'A/B/E/beta', 'A/B/E')
  expected_disk.add({
    'A/C/chi'     : Item(contents="This is the file 'chi'.\n"),
    'A/C/psi'     : Item(contents="This is the file 'psi'.\n"),
    'A/C/omega'   : Item(contents="This is the file 'omega'.\n"),
    'A/C/E'       : Item(),
    'A/C/E/alpha' : Item(contents="This is the file 'alpha'.\n"),
    'A/C/E/beta'  : Item(contents="This is the file 'beta'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega', 'A/B/E/alpha',
                         'A/B/E/beta', 'A/B/E')
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=2),
    'A/C/psi'     : Item(status='  ', wc_rev=2),
    'A/C/omega'   : Item(status='  ', wc_rev=2),
    'A/C/E'       : Item(status='  ', wc_rev=2),
    'A/C/E/alpha' : Item(status='  ', wc_rev=2),
    'A/C/E/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

# Test copying multiple files within a repo.

def copy_multiple_repo(sbox):
  "copy multiple files within a repo"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_url = sbox.repo_url + '/A/D/H/chi'
  psi_url = sbox.repo_url + '/A/D/H/psi'
  omega_url = sbox.repo_url + '/A/D/H/omega'
  E_url = sbox.repo_url + '/A/B/E'
  C_url = sbox.repo_url + '/A/C'

  # Copy three files and a directory in the repo to a different location
  # in the repo
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     chi_url, psi_url, omega_url, E_url, C_url,
                                     '-m', 'logmsg')

  # Update to HEAD, and check to see if the files really moved in the repo

  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(status='A '),
    'A/C/psi'     : Item(status='A '),
    'A/C/omega'   : Item(status='A '),
    'A/C/E'       : Item(status='A '),
    'A/C/E/alpha' : Item(status='A '),
    'A/C/E/beta'  : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/C/chi'     : Item(contents="This is the file 'chi'.\n"),
    'A/C/psi'     : Item(contents="This is the file 'psi'.\n"),
    'A/C/omega'   : Item(contents="This is the file 'omega'.\n"),
    'A/C/E'       : Item(),
    'A/C/E/alpha' : Item(contents="This is the file 'alpha'.\n"),
    'A/C/E/beta'  : Item(contents="This is the file 'beta'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=2),
    'A/C/psi'     : Item(status='  ', wc_rev=2),
    'A/C/omega'   : Item(status='  ', wc_rev=2),
    'A/C/E'       : Item(status='  ', wc_rev=2),
    'A/C/E/alpha' : Item(status='  ', wc_rev=2),
    'A/C/E/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

# Test moving copying multiple files from a repo to a wc
@Issue(2955)
def copy_multiple_repo_wc(sbox):
  "copy multiple files from a repo to a wc"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_url = sbox.repo_url + '/A/D/H/chi'
  psi_url = sbox.repo_url + '/A/D/H/psi'
  omega_with_space_url = sbox.repo_url + '/A/D/H/omega 2'
  E_url = sbox.repo_url + '/A/B/E'
  C_path = os.path.join(wc_dir, 'A', 'C')

  # We need this in order to check that we don't end up with URI-encoded
  # paths in the WC (issue #2955)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv', '-m', 'log_msg',
                                     sbox.repo_url + '/A/D/H/omega',
                                     omega_with_space_url)

  # Perform the copy and check the output
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     chi_url, psi_url, omega_with_space_url,
                                     E_url, C_path)

  # Commit the changes, and verify the content actually got copied
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(verb='Adding'),
    'A/C/psi'     : Item(verb='Adding'),
    'A/C/omega 2' : Item(verb='Adding'),
    'A/C/E'       : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=3),
    'A/C/psi'     : Item(status='  ', wc_rev=3),
    'A/C/omega 2' : Item(status='  ', wc_rev=3),
    'A/C/E'       : Item(status='  ', wc_rev=3),
    'A/C/E/alpha' : Item(status='  ', wc_rev=3),
    'A/C/E/beta'  : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

# Test moving copying multiple files from a wc to a repo

def copy_multiple_wc_repo(sbox):
  "copy multiple files from a wc to a repo"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')
  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  omega_path = os.path.join(wc_dir, 'A', 'D', 'H', 'omega')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  C_url = sbox.repo_url + '/A/C'

  # Perform the copy and check the output
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     chi_path, psi_path, omega_path, E_path,
                                     C_url, '-m', 'logmsg')

  # Update to HEAD, and check to see if the files really got copied in the repo

  expected_output = svntest.wc.State(wc_dir, {
    'A/C/chi'     : Item(status='A '),
    'A/C/psi'     : Item(status='A '),
    'A/C/omega'   : Item(status='A '),
    'A/C/E'       : Item(status='A '),
    'A/C/E/alpha' : Item(status='A '),
    'A/C/E/beta'  : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/C/chi': Item(contents="This is the file 'chi'.\n"),
    'A/C/psi': Item(contents="This is the file 'psi'.\n"),
    'A/C/omega': Item(contents="This is the file 'omega'.\n"),
    'A/C/E'       : Item(),
    'A/C/E/alpha' : Item(contents="This is the file 'alpha'.\n"),
    'A/C/E/beta'  : Item(contents="This is the file 'beta'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/C/chi'     : Item(status='  ', wc_rev=2),
    'A/C/psi'     : Item(status='  ', wc_rev=2),
    'A/C/omega'   : Item(status='  ', wc_rev=2),
    'A/C/E'       : Item(status='  ', wc_rev=2),
    'A/C/E/alpha' : Item(status='  ', wc_rev=2),
    'A/C/E/beta'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

#----------------------------------------------------------------------

# Test copying local files using peg revision syntax
# (Issue 2546)
@Issue(2546)
def copy_peg_rev_local_files(sbox):
  "copy local files using peg rev syntax"

  sbox.build()
  wc_dir = sbox.wc_dir

  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  new_iota_path = os.path.join(wc_dir, 'new_iota')
  iota_path = os.path.join(wc_dir, 'iota')
  sigma_path = os.path.join(wc_dir, 'sigma')

  psi_text = "This is the file 'psi'.\n"
  iota_text = "This is the file 'iota'.\n"

  # Play a shell game with some WC files, then commit the changes back
  # to the repository (making r2).
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     psi_path, new_iota_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     iota_path, psi_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     new_iota_path, iota_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 2',
                                     wc_dir)

  # Copy using a peg rev (remember, the object at iota_path at HEAD
  # was at psi_path back at r1).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp',
                                     iota_path + '@HEAD', '-r', '1',
                                     sigma_path)

  # Commit and verify disk contents
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', wc_dir,
                                     '-m', 'rev 3')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/H/psi', contents=iota_text)
  expected_disk.add({
    'iota'      : Item(contents=psi_text),
    'A/D/H/psi' : Item(contents=iota_text),
    'sigma'     : Item(contents=psi_text, props={}),
    })

  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 3)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())


#----------------------------------------------------------------------

# Test copying local directories using peg revision syntax
# (Issue 2546)
@Issue(2546)
def copy_peg_rev_local_dirs(sbox):
  "copy local dirs using peg rev syntax"

  sbox.build()
  wc_dir = sbox.wc_dir

  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  I_path = os.path.join(wc_dir, 'A', 'D', 'I')
  J_path = os.path.join(wc_dir, 'A', 'J')
  alpha_path = os.path.join(E_path, 'alpha')

  # Make some changes to the repository
  svntest.actions.run_and_verify_svn(None, None, [], 'rm',
                                     alpha_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 2',
                                     wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv',
                                     E_path, I_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 3',
                                     wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv',
                                     G_path, E_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 4',
                                     wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv',
                                     I_path, G_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 5',
                                     wc_dir)

  # Copy using a peg rev
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp',
                                     G_path + '@HEAD', '-r', '1',
                                     J_path)

  # Commit and verify disk contents
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', wc_dir,
                                     '-m', 'rev 6')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/E/beta')
  expected_disk.remove('A/B/E/alpha')
  expected_disk.remove('A/D/G/pi')
  expected_disk.remove('A/D/G/rho')
  expected_disk.remove('A/D/G/tau')
  expected_disk.add({
    'A/B/E'       : Item(),
    'A/B/E/pi'    : Item(contents="This is the file 'pi'.\n"),
    'A/B/E/rho'   : Item(contents="This is the file 'rho'.\n"),
    'A/B/E/tau'   : Item(contents="This is the file 'tau'.\n"),
    'A/D/G'       : Item(),
    'A/D/G/beta'  : Item(contents="This is the file 'beta'.\n"),
    'A/J'         : Item(),
    'A/J/alpha'   : Item(contents="This is the file 'alpha'.\n"),
    'A/J/beta'  : Item(contents="This is the file 'beta'.\n"),
    })

  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 5)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())


#----------------------------------------------------------------------

# Test copying urls using peg revision syntax
# (Issue 2546)
@Issues(2546,3651)
def copy_peg_rev_url(sbox):
  "copy urls using peg rev syntax"

  sbox.build()
  wc_dir = sbox.wc_dir

  psi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'psi')
  new_iota_path = os.path.join(wc_dir, 'new_iota')
  iota_path = os.path.join(wc_dir, 'iota')
  iota_url = sbox.repo_url + '/iota'
  sigma_url = sbox.repo_url + '/sigma'

  psi_text = "This is the file 'psi'.\n"
  iota_text = "This is the file 'iota'.\n"

  # Make some changes to the repository
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     psi_path, new_iota_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     iota_path, psi_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     new_iota_path, iota_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci',
                                     '-m', 'rev 2',
                                     wc_dir)

  # Copy using a peg rev
  # Add peg rev '@HEAD' to sigma_url when copying which tests for issue #3651.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp',
                                     iota_url + '@HEAD', '-r', '1',
                                     sigma_url + '@HEAD', '-m', 'rev 3')

  # Validate the copy destination's mergeinfo (we expect none).
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'propget', SVN_PROP_MERGEINFO, sigma_url)

  # Update to HEAD and verify disk contents
  expected_output = svntest.wc.State(wc_dir, {
    'sigma' : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=psi_text)
  expected_disk.tweak('A/D/H/psi', contents=iota_text)
  expected_disk.add({
    'sigma' : Item(contents=psi_text),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'sigma' : Item(status='  ', wc_rev=3)
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

# Test copying an older revision of a wc directory in the wc.
def old_dir_wc_to_wc(sbox):
  "copy old revision of wc dir to new dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  E = os.path.join(wc_dir, 'A', 'B', 'E')
  E2 = os.path.join(wc_dir, 'E2')
  E_url = sbox.repo_url + '/A/B/E'
  alpha_url = E_url + '/alpha'

  # delete E/alpha in r2
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', '-m', '', alpha_url)

  # delete E in r3
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm', '-m', '', E_url)

  # Copy an old revision of E into a new path in the WC
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-r1', E, E2)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'E2'      : Item(verb='Adding'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'E2' : Item(status='  ', wc_rev=4),
    'E2/alpha'  : Item(status='  ', wc_rev=4),
    'E2/beta'  : Item(status='  ', wc_rev=4),
    })
  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


#----------------------------------------------------------------------
# Test copying and creating parents in the wc

def copy_make_parents_wc_wc(sbox):
  "svn cp --parents WC_PATH WC_PATH"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  new_iota_path = os.path.join(wc_dir, 'X', 'Y', 'Z', 'iota')

  # Copy iota
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', '--parents',
                                     iota_path, new_iota_path)

  # Create expected output
  expected_output = svntest.wc.State(wc_dir, {
    'X'           : Item(verb='Adding'),
    'X/Y'         : Item(verb='Adding'),
    'X/Y/Z'       : Item(verb='Adding'),
    'X/Y/Z/iota'  : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Add the moved files
  expected_status.add({
    'X'           : Item(status='  ', wc_rev=2),
    'X/Y'         : Item(status='  ', wc_rev=2),
    'X/Y/Z'       : Item(status='  ', wc_rev=2),
    'X/Y/Z/iota'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

#----------------------------------------------------------------------
# Test copying and creating parents from the repo to the wc

def copy_make_parents_repo_wc(sbox):
  "svn cp --parents URL WC_PATH"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_url = sbox.repo_url + '/iota'
  new_iota_path = os.path.join(wc_dir, 'X', 'Y', 'Z', 'iota')

  # Copy iota
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '--parents',
                                     iota_url, new_iota_path)

  # Create expected output
  expected_output = svntest.wc.State(wc_dir, {
    'X'           : Item(verb='Adding'),
    'X/Y'         : Item(verb='Adding'),
    'X/Y/Z'       : Item(verb='Adding'),
    'X/Y/Z/iota'  : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Add the moved files
  expected_status.add({
    'X'           : Item(status='  ', wc_rev=2),
    'X/Y'         : Item(status='  ', wc_rev=2),
    'X/Y/Z'       : Item(status='  ', wc_rev=2),
    'X/Y/Z/iota'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)


#----------------------------------------------------------------------
# Test copying and creating parents from the wc to the repo

def copy_make_parents_wc_repo(sbox):
  "svn cp --parents WC_PATH URL"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  new_iota_url = sbox.repo_url + '/X/Y/Z/iota'

  # Copy iota
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '--parents',
                                     '-m', 'log msg',
                                     iota_path, new_iota_url)

  # Update to HEAD and verify disk contents
  expected_output = svntest.wc.State(wc_dir, {
    'X'           : Item(status='A '),
    'X/Y'         : Item(status='A '),
    'X/Y/Z'       : Item(status='A '),
    'X/Y/Z/iota'  : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'X'           : Item(),
    'X/Y'         : Item(),
    'X/Y/Z'       : Item(),
    'X/Y/Z/iota'  : Item(contents="This is the file 'iota'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'X'           : Item(status='  ', wc_rev=2),
    'X/Y'         : Item(status='  ', wc_rev=2),
    'X/Y/Z'       : Item(status='  ', wc_rev=2),
    'X/Y/Z/iota'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)


#----------------------------------------------------------------------
# Test copying and creating parents from repo to repo

def copy_make_parents_repo_repo(sbox):
  "svn cp --parents URL URL"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_url = sbox.repo_url + '/iota'
  new_iota_url = sbox.repo_url + '/X/Y/Z/iota'

  # Copy iota
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '--parents',
                                     '-m', 'log msg',
                                     iota_url, new_iota_url)

  # Update to HEAD and verify disk contents
  expected_output = svntest.wc.State(wc_dir, {
    'X'           : Item(status='A '),
    'X/Y'         : Item(status='A '),
    'X/Y/Z'       : Item(status='A '),
    'X/Y/Z/iota'  : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'X'           : Item(),
    'X/Y'         : Item(),
    'X/Y/Z'       : Item(),
    'X/Y/Z/iota'  : Item(contents="This is the file 'iota'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'X'           : Item(status='  ', wc_rev=2),
    'X/Y'         : Item(status='  ', wc_rev=2),
    'X/Y/Z'       : Item(status='  ', wc_rev=2),
    'X/Y/Z/iota'  : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

# Test for issue #2894
# Can't perform URL to WC copy if URL needs URI encoding.
@Issue(2894)
def URI_encoded_repos_to_wc(sbox):
  "copy a URL that needs URI encoding to WC"

  sbox.build()
  wc_dir = sbox.wc_dir
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk = svntest.main.greek_state.copy()

  def copy_URL_to_WC(URL_rel_path, dest_name, rev):
    lines = [
       "A    " + os.path.join(wc_dir, dest_name, "B") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "B", "lambda") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "B", "E") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "B", "E", "alpha") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "B", "E", "beta") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "B", "F") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "mu") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "C") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "gamma") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "G") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "G", "pi") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "G", "rho") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "G", "tau") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "H") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "H", "chi") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "H", "omega") + "\n",
       "A    " + os.path.join(wc_dir, dest_name, "D", "H", "psi") + "\n",
       "Checked out revision " + str(rev - 1) + ".\n",
       "A         " + os.path.join(wc_dir, dest_name) + "\n"]
    expected = svntest.verify.UnorderedOutput(lines)
    expected_status.add({
      dest_name + "/B"         : Item(status='  ', wc_rev=rev),
      dest_name + "/B/lambda"  : Item(status='  ', wc_rev=rev),
      dest_name + "/B/E"       : Item(status='  ', wc_rev=rev),
      dest_name + "/B/E/alpha" : Item(status='  ', wc_rev=rev),
      dest_name + "/B/E/beta"  : Item(status='  ', wc_rev=rev),
      dest_name + "/B/F"       : Item(status='  ', wc_rev=rev),
      dest_name + "/mu"        : Item(status='  ', wc_rev=rev),
      dest_name + "/C"         : Item(status='  ', wc_rev=rev),
      dest_name + "/D"         : Item(status='  ', wc_rev=rev),
      dest_name + "/D/gamma"   : Item(status='  ', wc_rev=rev),
      dest_name + "/D/G"       : Item(status='  ', wc_rev=rev),
      dest_name + "/D/G/pi"    : Item(status='  ', wc_rev=rev),
      dest_name + "/D/G/rho"   : Item(status='  ', wc_rev=rev),
      dest_name + "/D/G/tau"   : Item(status='  ', wc_rev=rev),
      dest_name + "/D/H"       : Item(status='  ', wc_rev=rev),
      dest_name + "/D/H/chi"   : Item(status='  ', wc_rev=rev),
      dest_name + "/D/H/omega" : Item(status='  ', wc_rev=rev),
      dest_name + "/D/H/psi"   : Item(status='  ', wc_rev=rev),
      dest_name                : Item(status='  ', wc_rev=rev)})
    expected_disk.add({
      dest_name                : Item(props={}),
      dest_name + '/B'         : Item(),
      dest_name + '/B/lambda'  : Item("This is the file 'lambda'.\n"),
      dest_name + '/B/E'       : Item(),
      dest_name + '/B/E/alpha' : Item("This is the file 'alpha'.\n"),
      dest_name + '/B/E/beta'  : Item("This is the file 'beta'.\n"),
      dest_name + '/B/F'       : Item(),
      dest_name + '/mu'        : Item("This is the file 'mu'.\n"),
      dest_name + '/C'         : Item(),
      dest_name + '/D'         : Item(),
      dest_name + '/D/gamma'   : Item("This is the file 'gamma'.\n"),
      dest_name + '/D/G'       : Item(),
      dest_name + '/D/G/pi'    : Item("This is the file 'pi'.\n"),
      dest_name + '/D/G/rho'   : Item("This is the file 'rho'.\n"),
      dest_name + '/D/G/tau'   : Item("This is the file 'tau'.\n"),
      dest_name + '/D/H'       : Item(),
      dest_name + '/D/H/chi'   : Item("This is the file 'chi'.\n"),
      dest_name + '/D/H/omega' : Item("This is the file 'omega'.\n"),
      dest_name + '/D/H/psi'   : Item("This is the file 'psi'.\n"),
      })

    # Make a copy
    svntest.actions.run_and_verify_svn(None, expected, [],
                                       'copy',
                                       sbox.repo_url + '/' + URL_rel_path,
                                       os.path.join(wc_dir,
                                                    dest_name))

    expected_output = svntest.wc.State(wc_dir,
                                       {dest_name : Item(verb='Adding')})
    svntest.actions.run_and_verify_commit(wc_dir,
                                          expected_output,
                                          expected_status,
                                          None, wc_dir)

  copy_URL_to_WC('A', 'A COPY', 2)
  copy_URL_to_WC('A COPY', 'A_COPY_2', 3)

#----------------------------------------------------------------------
# Issue #3068: copy source parent may be unversioned
@Issue(3068)
def allow_unversioned_parent_for_copy_src(sbox):
  "copy wc in unversioned parent to other wc"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Make the "other" working copy
  wc2_dir = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir, wc2_dir)
  copy_to_path = os.path.join(wc_dir, 'A', 'copy_of_wc2')

  # Copy the wc-in-unversioned-parent working copy to our original wc.
  svntest.actions.run_and_verify_svn(None,
                                     None,
                                     [],
                                     'cp',
                                     wc2_dir,
                                     copy_to_path)

def unneeded_parents(sbox):
  "svn cp --parents FILE_URL DIR_URL"

  # In this message...
  #
  #    http://subversion.tigris.org/servlets/ReadMsg?list=dev&msgNo=138738
  #    From: Alexander Kitaev <Alexander.Kitaev@svnkit.com>
  #    To: dev@subversion.tigris.org
  #    Subject: 1.5.x segmentation fault on Repos to Repos copy
  #    Message-ID: <4830332A.6060301@svnkit.com>
  #    Date: Sun, 18 May 2008 15:46:18 +0200
  #
  # ...Alexander Kitaev describes the bug:
  #
  #    svn cp --parents SRC_FILE_URL DST_DIR_URL -m "message"
  #
  #    SRC_FILE_URL - existing file
  #    DST_DIR_URL - existing directory
  #
  #    Omitting "--parents" option makes above copy operation work as
  #    expected.
  #
  #    Bug is in libsvn_client/copy.c:801, where "dir" should be
  #    checked for null before using it in svn_ra_check_path call.
  #
  # At first we couldn't reproduce it, but later he added this:
  #
  #   Looks like there is one more condition to reproduce the problem -
  #   dst URL should has no more segments count than source one.
  #
  # In other words, if we had "/A/B" below instead of "/A" (adjusting
  # expected_* accordingly, of course), the bug wouldn't reproduce.

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_url = sbox.repo_url + '/iota'
  A_url = sbox.repo_url + '/A'

  # The --parents is unnecessary, but should still work (not segfault).
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', '--parents',
                                     '-m', 'log msg', iota_url, A_url)

  # Verify that it worked.
  expected_output = svntest.wc.State(wc_dir, {
    'A/iota' : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/iota'  : Item(contents="This is the file 'iota'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/iota'  : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_update(
    wc_dir, expected_output, expected_disk, expected_status)


def double_parents_with_url(sbox):
  "svn cp --parents URL/src_dir URL/dst_dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  E_url = sbox.repo_url + '/A/B/E'
  Z_url = sbox.repo_url + '/A/B/Z'

  # --parents shouldn't result in a double commit of the same directory.
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', '--parents',
                                     '-m', 'log msg', E_url, Z_url)

  # Verify that it worked.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/Z/alpha' : Item(status='A '),
    'A/B/Z/beta'  : Item(status='A '),
    'A/B/Z'       : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/Z/alpha' : Item(contents="This is the file 'alpha'.\n"),
    'A/B/Z/beta'  : Item(contents="This is the file 'beta'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/B/Z/alpha' : Item(status='  ', wc_rev=2),
    'A/B/Z/beta'  : Item(status='  ', wc_rev=2),
    'A/B/Z'       : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_update(
    wc_dir, expected_output, expected_disk, expected_status)


# Used to cause corruption not fixable by 'svn cleanup'.
def copy_into_absent_dir(sbox):
  "copy file into absent dir"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  iota_path = os.path.join(wc_dir, 'iota')

  # Remove 'A'
  svntest.main.safe_rmtree(A_path)

  # Copy into the now-missing dir.  This used to give this error:
  #     svn: In directory '.'
  #     svn: Error processing command 'modify-entry' in '.'
  #     svn: Error modifying entry for 'A'
  #     svn: Entry 'A' is already under version control
  svntest.actions.run_and_verify_svn(None,
                                     None, ".*: Path '.*' is not a directory",
                                     'cp', iota_path, A_path)

  # 'cleanup' should not error.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cleanup', wc_dir)


def find_copyfrom_information_upstairs(sbox):
  "renaming inside a copied subtree shouldn't hang"

  # The final command in this series would cause the client to hang...
  #
  #    ${SVN} cp A A2
  #    cd A2/B
  #    ${SVN} mkdir blah
  #    ${SVN} mv lambda blah
  #
  # ...because it wouldn't walk up past "" to find copyfrom information
  # (which would be in A2/.svn/entries, not on A2/B/.svn/entries).
  # Instead, it would keep thinking the parent of "" is "", and so
  # loop forever, gobbling a little bit more memory with each iteration.
  #
  # Two things fixed this:
  #
  #   1) The client walks upward beyond CWD now, so it finds the
  #      copyfrom information.
  #
  #   2) Even if we do top out at "" without finding copyfrom information
  #      (say, because someone has corrupted their working copy), we'll
  #      still detect it and error, thus breaking the loop.
  #
  # This only tests case (1).  We could test that (2) gets the expected
  # error ("no parent with copyfrom information found above 'lambda'"),
  # but we'd need to chroot to the top of the working copy or manually
  # corrupt the wc by removing the copyfrom lines from A2/.svn/entries.

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  A2_path = os.path.join(wc_dir, 'A2')
  B2_path = os.path.join(A2_path, 'B')

  svntest.actions.run_and_verify_svn(None, None, [], 'cp', A_path, A2_path)
  saved_cwd = os.getcwd()
  try:
    os.chdir(B2_path)
    svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', 'blah')
    svntest.actions.run_and_verify_svn(None, None, [], 'mv', 'lambda', 'blah')
  finally:
    os.chdir(saved_cwd)

#----------------------------------------------------------------------

def change_case_of_hostname(input):
  "Change the case of the hostname, try uppercase first"

  m = re.match(r"^(.*://)([^/]*)(.*)", input)
  if m:
    scheme = m.group(1)
    host = m.group(2).upper()
    if host == m.group(2):
      host = m.group(2).lower()

    path = m.group(3)

  return scheme + host + path

# regression test for issue #2475 - move file and folder
@Issue(2475)
def path_move_and_copy_between_wcs_2475(sbox):
  "issue #2475 - move and copy between working copies"
  sbox.build()

  # checkout a second working copy, use repository url with different case
  wc2_dir = sbox.add_wc_path('2')
  repo_url2 = change_case_of_hostname(sbox.repo_url)

  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = wc2_dir
  expected_output.tweak(status='A ', contents=None)

  expected_wc = svntest.main.greek_state

  # Do a checkout, and verify the resulting output and disk contents.
  svntest.actions.run_and_verify_checkout(repo_url2,
                          wc2_dir,
                          expected_output,
                          expected_wc)

  # Copy a file from wc to wc2
  mu_path = os.path.join(sbox.wc_dir, 'A', 'mu')
  E_path = os.path.join(wc2_dir, 'A', 'B', 'E')

  svntest.main.run_svn(None, 'cp', mu_path, E_path)

  # Copy a folder from wc to wc2
  C_path = os.path.join(sbox.wc_dir, 'A', 'C')
  B_path = os.path.join(wc2_dir, 'A', 'B')

  svntest.main.run_svn(None, 'cp', C_path, B_path)

  # Move a file from wc to wc2
  mu_path = os.path.join(sbox.wc_dir, 'A', 'mu')
  B_path = os.path.join(wc2_dir, 'A', 'B')

  svntest.main.run_svn(None, 'mv', mu_path, B_path)

  # Move a folder from wc to wc2
  C_path = os.path.join(sbox.wc_dir, 'A', 'C')
  D_path = os.path.join(wc2_dir, 'A', 'D')

  svntest.main.run_svn(None, 'mv', C_path, D_path)

  # Verify modified status
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak('A/mu', 'A/C', status='D ')
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)
  expected_status2 = svntest.actions.get_virginal_state(wc2_dir, 1)
  expected_status2.add({ 'A/B/mu' :
                        Item(status='A ', copied='+', wc_rev='-') })
  expected_status2.add({ 'A/B/C' :
                        Item(status='A ', copied='+', wc_rev='-') })
  expected_status2.add({ 'A/B/E/mu' :
                        Item(status='A ', copied='+', wc_rev='-') })
  expected_status2.add({ 'A/D/C' :
                        Item(status='A ', copied='+', wc_rev='-') })
  svntest.actions.run_and_verify_status(wc2_dir, expected_status2)


# regression test for issue #2475 - direct copy in the repository
# this test handles the 'direct move' case too, that uses the same code.
@Issue(2475)
def path_copy_in_repo_2475(sbox):
  "issue #2475 - direct copy in the repository"
  sbox.build()

  repo_url2 = change_case_of_hostname(sbox.repo_url)

  # Copy a file from repo to repo2
  mu_url = sbox.repo_url + '/A/mu'
  E_url = repo_url2 + '/A/B/E'

  svntest.main.run_svn(None, 'cp', mu_url, E_url, '-m', 'copy mu to /A/B/E')

  # For completeness' sake, update to HEAD, and verify we have a full
  # greek tree again, all at revision 2.
  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/B/E/mu'  : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'A/B/E/mu' : Item("This is the file 'mu'.\n") })

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 2)
  expected_status.add({'A/B/E/mu' : Item(status='  ', wc_rev=2) })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

def commit_copy_depth_empty(sbox):
  "copy a wcdir, then commit it with --depth empty"
  sbox.build()

  a = os.path.join(sbox.wc_dir, 'A')
  new_a = os.path.join(sbox.wc_dir, 'new_A')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', a, new_a)

  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     new_a, '--depth', 'empty',
                                     '-m', 'Copied directory')

def copy_below_copy(sbox):
  "copy a dir below a copied dir"
  sbox.build()

  A = os.path.join(sbox.wc_dir, 'A')
  new_A = os.path.join(sbox.wc_dir, 'new_A')
  new_A_D = os.path.join(new_A, 'D')
  new_A_new_D = os.path.join(new_A, 'new_D')
  new_A_mu = os.path.join(new_A, 'mu')
  new_A_new_mu = os.path.join(new_A, 'new_mu')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', A, new_A)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', new_A_D, new_A_new_D)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', new_A_mu, new_A_new_mu)

  expected_output = svntest.wc.State(sbox.wc_dir, {
      'new_A'             : Item(verb='Adding'),
      'new_A/new_D'       : Item(verb='Adding'),
      'new_A/new_mu'      : Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  expected_status.add({
      'new_A'             : Item(status='  ', wc_rev='2'),
      'new_A/new_D'       : Item(status='  ', wc_rev='2'),
      'new_A/new_D/gamma' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G'     : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/pi'  : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/rho' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/tau' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H'     : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/chi' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/omega': Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/psi' : Item(status='  ', wc_rev='2'),
      'new_A/D'           : Item(status='  ', wc_rev='2'),
      'new_A/D/H'         : Item(status='  ', wc_rev='2'),
      'new_A/D/H/chi'     : Item(status='  ', wc_rev='2'),
      'new_A/D/H/omega'   : Item(status='  ', wc_rev='2'),
      'new_A/D/H/psi'     : Item(status='  ', wc_rev='2'),
      'new_A/D/G'         : Item(status='  ', wc_rev='2'),
      'new_A/D/G/rho'     : Item(status='  ', wc_rev='2'),
      'new_A/D/G/pi'      : Item(status='  ', wc_rev='2'),
      'new_A/D/G/tau'     : Item(status='  ', wc_rev='2'),
      'new_A/D/gamma'     : Item(status='  ', wc_rev='2'),
      'new_A/new_mu'      : Item(status='  ', wc_rev='2'),
      'new_A/B'           : Item(status='  ', wc_rev='2'),
      'new_A/B/E'         : Item(status='  ', wc_rev='2'),
      'new_A/B/E/alpha'   : Item(status='  ', wc_rev='2'),
      'new_A/B/E/beta'    : Item(status='  ', wc_rev='2'),
      'new_A/B/F'         : Item(status='  ', wc_rev='2'),
      'new_A/B/lambda'    : Item(status='  ', wc_rev='2'),
      'new_A/C'           : Item(status='  ', wc_rev='2'),
      'new_A/mu'          : Item(status='  ', wc_rev='2'),
    })

  svntest.actions.run_and_verify_commit(sbox.wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, sbox.wc_dir)

def move_below_move(sbox):
  "move a dir below a moved dir"
  sbox.build()

  A = os.path.join(sbox.wc_dir, 'A')
  new_A = os.path.join(sbox.wc_dir, 'new_A')
  new_A_D = os.path.join(new_A, 'D')
  new_A_new_D = os.path.join(new_A, 'new_D')
  new_A_mu = os.path.join(new_A, 'mu')
  new_A_new_mu = os.path.join(new_A, 'new_mu')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', A, new_A)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', new_A_D, new_A_new_D)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mv', new_A_mu, new_A_new_mu)

  expected_output = svntest.wc.State(sbox.wc_dir, {
      'A'                 : Item(verb='Deleting'),
      'new_A/D'           : Item(verb='Deleting'),
      'new_A/mu'          : Item(verb='Deleting'),
      'new_A'             : Item(verb='Adding'),
      'new_A/new_D'       : Item(verb='Adding'),
      'new_A/new_mu'      : Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  expected_status.add({
      'new_A'             : Item(status='  ', wc_rev='2'),
      'new_A/new_D'       : Item(status='  ', wc_rev='2'),
      'new_A/new_D/gamma' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G'     : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/pi'  : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/rho' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/G/tau' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H'     : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/chi' : Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/omega': Item(status='  ', wc_rev='2'),
      'new_A/new_D/H/psi' : Item(status='  ', wc_rev='2'),
      'new_A/new_mu'      : Item(status='  ', wc_rev='2'),
      'new_A/B'           : Item(status='  ', wc_rev='2'),
      'new_A/B/E'         : Item(status='  ', wc_rev='2'),
      'new_A/B/E/alpha'   : Item(status='  ', wc_rev='2'),
      'new_A/B/E/beta'    : Item(status='  ', wc_rev='2'),
      'new_A/B/F'         : Item(status='  ', wc_rev='2'),
      'new_A/B/lambda'    : Item(status='  ', wc_rev='2'),
      'new_A/C'           : Item(status='  ', wc_rev='2'),
    })

  expected_status.remove('A', 'A/D', 'A/D/gamma', 'A/D/G', 'A/D/G/pi',
                         'A/D/G/rho', 'A/D/G/tau', 'A/D/H', 'A/D/H/chi',
                         'A/D/H/omega', 'A/D/H/psi', 'A/B', 'A/B/E',
                         'A/B/E/alpha', 'A/B/E/beta', 'A/B/F', 'A/B/lambda',
                         'A/C', 'A/mu')

  svntest.actions.run_and_verify_commit(sbox.wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, sbox.wc_dir)


def reverse_merge_move(sbox):
  """reverse merge move"""

  # Alias for svntest.actions.run_and_verify_svn
  rav_svn = svntest.actions.run_and_verify_svn

  wc_dir = sbox.wc_dir
  a_dir = os.path.join(wc_dir, 'A')
  a_repo_url = sbox.repo_url + '/A'
  sbox.build()

  # Create another working copy path and checkout.
  wc2_dir = sbox.add_wc_path('2')
  rav_svn(None, None, [], 'co', sbox.repo_url, wc2_dir)

  # Update working directory and ensure that we are at revision 1.
  rav_svn(None, exp_noop_up_out(1), [], 'up', wc_dir)

  # Add new folder and file, later commit
  new_path = os.path.join(a_dir, 'New')
  os.mkdir(new_path)
  first_path = os.path.join(new_path, 'first')
  svntest.main.file_append(first_path, 'appended first text')
  svntest.main.run_svn(None, "add", new_path)
  rav_svn(None, None, [], 'ci', wc_dir, '-m', 'Add new folder %s' % new_path)
  rav_svn(None, exp_noop_up_out(2), [], 'up', wc_dir)

  # Reverse merge to revert previous changes and commit
  rav_svn(None, None, [], 'merge', '-c', '-2', a_repo_url, a_dir)
  rav_svn(None, None, [], 'ci', '-m', 'Reverting svn merge -c -2.', a_dir)
  rav_svn(None, exp_noop_up_out(3), [], 'up', wc_dir)

  # Reverse merge again to undo last revert.
  rav_svn(None, None, [], 'merge', '-c', '-3', a_repo_url, a_dir)

  # Move new added file to another one and commit.
  second_path = os.path.join(new_path, 'second')
  rav_svn(None, None, [], 'move', first_path, second_path)
  rav_svn(None, "Adding.*New|Adding.*first||Committed revision 4.", [],
          'ci', '-m',
          'Revert svn merge. svn mv %s %s.' % (first_path, second_path), a_dir)

  # Update second working copy. There was a bug (at least on the 1.6.x
  # branch) in which this update received both "first" and "second".
  expected_output = svntest.wc.State(wc2_dir, {
      'A/New'         : Item(status='A '),
      'A/New/second'  : Item(status='A '),
      })
  svntest.actions.run_and_verify_update(wc2_dir,
                                        expected_output,
                                        None,
                                        None)

@Issue(3699)
def nonrecursive_commit_of_copy(sbox):
  """commit only top of copy; check child behavior"""

  sbox.build()
  wc_dir = sbox.wc_dir

  main.run_svn(None, 'cp', os.path.join(wc_dir, 'A'),
               os.path.join(wc_dir, 'A_new'))
  main.run_svn(None, 'cp', os.path.join(wc_dir, 'A/D/G'),
               os.path.join(wc_dir, 'A_new/G_new'))
  main.run_svn(None, 'rm', os.path.join(wc_dir, 'A_new/C'))
  main.run_svn(None, 'rm', os.path.join(wc_dir, 'A_new/B/E'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
      'A_new'             : Item(status='A ', copied='+', wc_rev='-'),
      'A_new/D'           : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/G'         : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/G/pi'      : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/G/rho'     : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/G/tau'     : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/H'         : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/H/psi'     : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/H/chi'     : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/H/omega'   : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/D/gamma'     : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/B'           : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/B/lambda'    : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/B/E'         : Item(status='D ', copied='+', wc_rev='-'),
      'A_new/B/E/alpha'   : Item(status='D ', copied='+', wc_rev='-'),
      'A_new/B/E/beta'    : Item(status='D ', copied='+', wc_rev='-'),
      'A_new/B/F'         : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/mu'          : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/C'           : Item(status='D ', copied='+', wc_rev='-'),
      'A_new/G_new'       : Item(status='A ', copied='+', wc_rev='-'),
      'A_new/G_new/pi'    : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/G_new/rho'   : Item(status='  ', copied='+', wc_rev='-'),
      'A_new/G_new/tau'   : Item(status='  ', copied='+', wc_rev='-'),
    })


  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.wc.State(wc_dir, {
    'A_new': Item(verb='Adding'),
    })

  # These nodes are added by the commit
  expected_status.tweak('A_new', 'A_new/D', 'A_new/D/G', 'A_new/D/G/pi',
                        'A_new/D/G/rho', 'A_new/D/G/tau', 'A_new/D/H',
                        'A_new/D/H/psi', 'A_new/D/H/chi', 'A_new/D/H/omega',
                        'A_new/D/gamma', 'A_new/B', 'A_new/B/lambda',
                        'A_new/B/F', 'A_new/mu',
                        status='  ', copied=None, wc_rev='2')

  # And these are now normal deletes, because their parent was committed.
  expected_status.tweak('A_new/C', 'A_new/B/E', 'A_new/B/E/alpha',
                        'A_new/B/E/beta', copied=None, wc_rev='2')

  # 'A_new/G_new' and everything below should still be added
  # as their operation root was not committed
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir, '--depth', 'immediates')

# Regression test for issue #3474 - making a new subdir, moving files into it
# and then renaming the subdir, breaks history of the moved files.
@Issue(3474)
def copy_added_dir_with_copy(sbox):
  """copy of new dir with copied file keeps history"""

  sbox.build()
  wc_dir = sbox.wc_dir

  new_dir = os.path.join(wc_dir, 'NewDir');
  new_dir2 = os.path.join(wc_dir, 'NewDir2');

  # Alias for svntest.actions.run_and_verify_svn
  rav_svn = svntest.actions.run_and_verify_svn

  rav_svn(None, None, [], 'mkdir', new_dir)
  rav_svn(None, None, [], 'cp', os.path.join(wc_dir, 'A', 'mu'), new_dir)
  rav_svn(None, None, [], 'cp', new_dir, new_dir2)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  expected_status.add(
    {
      'NewDir'            : Item(status='A ', wc_rev='0'),
      'NewDir/mu'         : Item(status='A ', copied='+', wc_rev='-'),
      'NewDir2'           : Item(status='A ', wc_rev='0'),
      'NewDir2/mu'        : Item(status='A ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)


@SkipUnless(svntest.main.is_posix_os)
@Issue(3303)
def copy_broken_symlink(sbox):
  """copy broken symlink"""

  ## See http://subversion.tigris.org/issues/show_bug.cgi?id=3303. ##

  sbox.build()
  wc_dir = sbox.wc_dir

  new_symlink = os.path.join(wc_dir, 'new_symlink');
  copied_symlink = os.path.join(wc_dir, 'copied_symlink');
  os.symlink('linktarget', new_symlink)

  # Alias for svntest.actions.run_and_verify_svn
  rav_svn = svntest.actions.run_and_verify_svn

  rav_svn(None, None, [], 'add', new_symlink)
  rav_svn(None, None, [], 'cp', new_symlink, copied_symlink)

  # Check whether both new_symlink and copied_symlink are added to the
  # working copy
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  expected_status.add(
    {
      'new_symlink'       : Item(status='A ', wc_rev='0'),
      'copied_symlink'    : Item(status='A ', wc_rev='0'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)


def move_dir_containing_move(sbox):
  """move a directory containing moved node"""

  sbox.build()
  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('A/B/E/alpha'),
                                     sbox.ospath('A/B/E/alpha_moved'))

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('A/B/F'),
                                     sbox.ospath('A/B/F_moved'))

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('A/B'),
                                     sbox.ospath('A/B_tmp'))

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak('A/B',
                        'A/B/E',
                        'A/B/E/alpha',
                        'A/B/E/beta',
                        'A/B/F',
                        'A/B/lambda',
                        status='D ')
  expected_status.add({
      'A/B_tmp'               : Item(status='A ', copied='+', wc_rev='-'),
      # alpha has a revision that isn't reported by status.
      'A/B_tmp/E'             : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_tmp/E/alpha'       : Item(status='D ', copied='+', wc_rev='-'),
      'A/B_tmp/E/alpha_moved' : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_tmp/E/beta'        : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_tmp/F'             : Item(status='D ', copied='+', wc_rev='-'),
      'A/B_tmp/F_moved'       : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_tmp/lambda'        : Item(status='  ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('A/B_tmp'),
                                     sbox.ospath('A/B_moved'))
  expected_status.remove('A/B_tmp',
                         'A/B_tmp/E',
                         'A/B_tmp/E/alpha',
                         'A/B_tmp/E/alpha_moved',
                         'A/B_tmp/E/beta',
                         'A/B_tmp/F',
                         'A/B_tmp/F_moved',
                         'A/B_tmp/lambda')
  expected_status.add({
      'A/B_moved'               : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_moved/E'             : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_moved/E/alpha'       : Item(status='D ', copied='+', wc_rev='-'),
      'A/B_moved/E/alpha_moved' : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_moved/E/beta'        : Item(status='  ', copied='+', wc_rev='-'),
      'A/B_moved/F'             : Item(status='D ', copied='+', wc_rev='-'),
      'A/B_moved/F_moved'       : Item(status='A ', copied='+', wc_rev='-'),
      'A/B_moved/lambda'        : Item(status='  ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/B'                    : Item(verb='Deleting'),
    'A/B_moved'              : Item(verb='Adding'),
    'A/B_moved/E/alpha'      : Item(verb='Deleting'),
    'A/B_moved/E/alpha_moved': Item(verb='Adding'),
    'A/B_moved/F'            : Item(verb='Deleting'),
    'A/B_moved/F_moved'      : Item(verb='Adding'),
    })

  expected_status.tweak('A/B_moved',
                        'A/B_moved/E',
                        'A/B_moved/E/alpha_moved',
                        'A/B_moved/E/beta',
                        'A/B_moved/F_moved',
                        'A/B_moved/lambda',
                        status='  ', copied=None, wc_rev='2')
  expected_status.remove('A/B',
                         'A/B/E',
                         'A/B/E/alpha',
                         'A/B/E/beta',
                         'A/B/F',
                         'A/B/lambda',
                         'A/B_moved/E/alpha',
                         'A/B_moved/F')
  svntest.actions.run_and_verify_commit(sbox.wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, sbox.wc_dir)

def copy_dir_with_space(sbox):
  """copy a directory with whitespace to one without"""

  sbox.build()
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'A', 'B', 'E'),
                                     os.path.join(wc_dir, 'E with spaces'))

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'A', 'B', 'E', 'alpha'),
                                     os.path.join(wc_dir, 'E with spaces', 'al pha'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_output = svntest.wc.State(wc_dir, {
    'E with spaces'        : Item(verb='Adding'),
    'E with spaces/al pha' : Item(verb='Adding'),
    })
  expected_status.add({
    'E with spaces'        : Item(status='  ', wc_rev='2'),
    'E with spaces/alpha'  : Item(status='  ', wc_rev='2'),
    'E with spaces/beta'   : Item(status='  ', wc_rev='2'),
    'E with spaces/al pha' : Item(status='  ', wc_rev='2'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'E with spaces'),
                                     os.path.join(wc_dir, 'E also spaces')
                                     )

  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     os.path.join(wc_dir, 'E with spaces/al pha'),
                                     os.path.join(wc_dir, 'E also spaces/al b')
                                     )

  expected_output = svntest.wc.State(wc_dir, {
      'E also spaces'     : Item(verb='Adding'),
      'E also spaces/al b': Item(verb='Adding'),
    })
  expected_status.add({
      'E also spaces'     : Item(status='  ', wc_rev='3'),
      'E also spaces/beta': Item(status='  ', wc_rev='3'),
      'E also spaces/al b': Item(status='  ', wc_rev='3'),
      'E also spaces/alpha': Item(status='  ', wc_rev='3'),
      'E also spaces/al pha': Item(status='  ', wc_rev='3'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     os.path.join(wc_dir, 'E with spaces'),
                                     os.path.join(wc_dir, 'E new spaces')
                                     )

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     os.path.join(wc_dir, 'E new spaces/al pha'),
                                     os.path.join(wc_dir, 'E also spaces/al c')
                                     )

  expected_output = svntest.wc.State(wc_dir, {
      'E with spaces'     : Item(verb='Deleting'),
      'E also spaces/al c': Item(verb='Adding'),
      'E new spaces'      : Item(verb='Adding'),
      'E new spaces/al pha': Item(verb='Deleting'),
  })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
      'E also spaces'     : Item(status='  ', wc_rev='3'),
      'E also spaces/beta': Item(status='  ', wc_rev='3'),
      'E also spaces/al b': Item(status='  ', wc_rev='3'),
      'E also spaces/al c': Item(status='  ', wc_rev='4'),
      'E also spaces/alpha': Item(status='  ', wc_rev='3'),
      'E also spaces/al pha': Item(status='  ', wc_rev='3'),
      'E new spaces'      : Item(status='  ', wc_rev='4'),
      'E new spaces/alpha': Item(status='  ', wc_rev='4'),
      'E new spaces/beta' : Item(status='  ', wc_rev='4'),
  })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

# Regression test for issue #3676
@Issue(3676)
def changed_data_should_match_checkout(sbox):
  """changed data after commit should match checkout"""

  sbox.build()
  wc_dir = sbox.wc_dir
  A_B_E = os.path.join(wc_dir, 'A', 'B', 'E')
  E_new = os.path.join(wc_dir, 'E_new')

  verify_dir = sbox.add_wc_path('verify')

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', A_B_E, E_new)

  sbox.simple_commit()

  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'co', sbox.repo_url, verify_dir)

  was_cwd = os.getcwd()
  os.chdir(verify_dir)

  rv, verify_out, err = main.run_svn(None, 'status', '-v')

  os.chdir(was_cwd)
  os.chdir(wc_dir)
  verify_out = svntest.verify.UnorderedOutput(verify_out)
  svntest.actions.run_and_verify_svn(None, verify_out, [], 'status', '-v')
  os.chdir(was_cwd)

# Regression test for issue #3676 for copies including directories
@Issue(3676)
def changed_dir_data_should_match_checkout(sbox):
  """changed dir after commit should match checkout"""

  sbox.build()
  wc_dir = sbox.wc_dir
  A_B = os.path.join(wc_dir, 'A', 'B')
  B_new = os.path.join(wc_dir, 'B_new')

  verify_dir = sbox.add_wc_path('verify')

  svntest.actions.run_and_verify_svn(None, None, [], 'copy', A_B, B_new)

  sbox.simple_commit()

  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'co', sbox.repo_url, verify_dir)

  was_cwd = os.getcwd()
  os.chdir(verify_dir)

  rv, verify_out, err = main.run_svn(None, 'status', '-v')

  os.chdir(was_cwd)
  os.chdir(wc_dir)
  rv, verify_out2, err = main.run_svn (None, 'status', '-v')
  os.chdir(was_cwd)

  # The order of the status output is not absolutely defined, but
  # otherwise should match
  svntest.verify.verify_outputs(None,
                                sorted(verify_out2), None,
                                sorted(verify_out), None)

def move_added_nodes(sbox):
  """move added nodes"""

  sbox.build()

  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir',
                                     sbox.ospath('X'),
                                     sbox.ospath('X/Y'))

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.add({
      'X'   : Item(status='A ', wc_rev='0'),
      'X/Y' : Item(status='A ', wc_rev='0'),
      })
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('X/Y'),
                                     sbox.ospath('X/Z'))
  expected_status.remove('X/Y')
  expected_status.add({'X/Z' : Item(status='A ', wc_rev='0')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('X/Z'),
                                     sbox.ospath('Z'))
  expected_status.remove('X/Z')
  expected_status.add({'Z' : Item(status='A ', wc_rev='0')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'mv',
                                     sbox.ospath('Z'),
                                     sbox.ospath('X/Z'))
  expected_status.remove('Z')
  expected_status.add({'X/Z' : Item(status='A ', wc_rev='0')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

def copy_over_deleted_dir(sbox):
  "copy a directory over a deleted directory"
  sbox.build(read_only = True)

  main.run_svn(None, 'rm', os.path.join(sbox.wc_dir, 'A/B'))
  main.run_svn(None, 'cp', os.path.join(sbox.wc_dir, 'A/D'),
               os.path.join(sbox.wc_dir, 'A/B'))

@Issue(3314)
def mixed_rev_copy_del(sbox):
  """copy mixed-rev and delete children"""

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete and commit A/B/E/alpha
  svntest.main.run_svn(None, 'rm', sbox.ospath('A/B/E/alpha'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha': Item(verb='Deleting'),
    })
  expected_status.remove('A/B/E/alpha')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Update to r2, then update A/B/E/alpha and A/B/E/beta to r1
  svntest.main.run_svn(None, 'up', wc_dir)
  expected_status.tweak(wc_rev=2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  svntest.main.run_svn(None, 'up', '-r1',
                       sbox.ospath('A/B/E/alpha'),
                       sbox.ospath('A/B/E/beta'))
  expected_status.add({
    'A/B/E/alpha' : Item(status='  ', wc_rev=1),
    })
  expected_status.tweak('A/B/E/beta', wc_rev=1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Copy A/B/E to A/B/E_copy
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     sbox.ospath('A/B/E'),
                                     sbox.ospath('A/B/E_copy'))
  expected_status.add({
    'A/B/E_copy'       : Item(status='A ', copied='+', wc_rev='-'),
    # In the entries world mixed revision copies have only a single op_root
    'A/B/E_copy/alpha' : Item(status='A ', copied='+', wc_rev='-',
                              entry_status='  '),
    'A/B/E_copy/beta'  : Item(status='A ', copied='+', wc_rev='-',
                              entry_status='  '),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Delete A/B/E_copy/alpha and A/B/E_copy/beta
  svntest.main.run_svn(None, 'rm', '--force',
                       sbox.ospath('A/B/E_copy/alpha'),
                       sbox.ospath('A/B/E_copy/beta'))
  expected_status.tweak('A/B/E_copy/alpha', 'A/B/E_copy/beta', status='D ',
                        entry_status=None)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E_copy'      : Item(verb='Adding'),
    'A/B/E_copy/beta' : Item(verb='Deleting'),
    })
  expected_status.tweak('A/B/E_copy', wc_rev=3, copied=None, status='  ')
  expected_status.remove('A/B/E_copy/alpha', 'A/B/E_copy/beta')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

def copy_delete_undo(sbox, use_revert):
  "copy, delete child, undo"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Copy directory with children
  svntest.main.run_svn(wc_dir, 'copy',
                       sbox.ospath('A/B/E'), sbox.ospath('A/B/E-copied'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/E-copied'       : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E-copied/alpha' : Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E-copied/beta'  : Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Delete a child
  svntest.main.run_svn(wc_dir, 'rm', sbox.ospath('A/B/E-copied/alpha'))
  expected_status.tweak('A/B/E-copied/alpha', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Undo the whole copy
  if (use_revert):
    svntest.main.run_svn(wc_dir, 'revert', '--recursive',
                         sbox.ospath('A/B/E-copied'))
    svntest.main.safe_rmtree(os.path.join(wc_dir, 'A/B/E-copied'))
  else:
    svntest.main.run_svn(wc_dir, 'rm', '--force', sbox.ospath('A/B/E-copied'))
  expected_status.remove('A/B/E-copied',
                         'A/B/E-copied/alpha',
                         'A/B/E-copied/beta')

  # Undo via revert FAILs here because a wq item remains
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Copy a directory without children.
  svntest.main.run_svn(wc_dir, 'copy',
                       sbox.ospath('A/B/F'), sbox.ospath('A/B/E-copied'))
  expected_status.add({
    'A/B/E-copied'       : Item(status='A ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

def copy_delete_delete(sbox):
  "copy, delete child, delete copy"
  copy_delete_undo(sbox, False)

@Issue(3784)
def copy_delete_revert(sbox):
  "copy, delete child, revert copy"
  copy_delete_undo(sbox, True)

# See also delete_replaced_file() which does the same for a file.
def delete_replace_delete(sbox):
  "delete a directory scheduled for replacement"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Delete directory with children
  svntest.main.run_svn(wc_dir, 'rm', sbox.ospath('A/B/E'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E', 'A/B/E/alpha', 'A/B/E/beta', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Replace with directory with different children
  svntest.main.run_svn(wc_dir, 'copy',
                       sbox.ospath('A/D/G'), sbox.ospath('A/B/E'))
  expected_status.tweak('A/B/E', status='R ', copied='+', wc_rev='-')
  expected_status.add({
    'A/B/E/pi' : Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E/rho' : Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E/tau' : Item(status='  ', copied='+', wc_rev='-'),
    })
  # A/B/E/alpha and A/B/E/beta show up as deleted, is that right?
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Delete replacement
  svntest.main.run_svn(wc_dir, 'rm', '--force', sbox.ospath('A/B/E'))
  expected_status.tweak('A/B/E', status='D ', copied=None, wc_rev='1')
  expected_status.remove('A/B/E/pi', 'A/B/E/rho', 'A/B/E/tau')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

A_B_children = ['A/B/lambda', 'A/B/F', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/E']
A_D_children = ['A/D/gamma', 'A/D/G', 'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau',
                'A/D/H', 'A/D/H/chi', 'A/D/H/psi', 'A/D/H/omega']

def copy_repos_over_deleted_same_kind(sbox):
  "copy repos node over deleted node, same kind"
  sbox.build(read_only = True)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  # Set up some deleted paths
  sbox.simple_rm('iota', 'A/B')
  for path in ['iota', 'A/B'] + A_B_children:
    expected_status.tweak(path, status='D ')

  # Test copying
  main.run_svn(None, 'cp', sbox.repo_url + '/A/mu', sbox.ospath('iota'))
  expected_status.tweak('iota', status='R ', wc_rev='-', copied='+')
  main.run_svn(None, 'cp', sbox.repo_url + '/A/D', sbox.ospath('A/B'))
  expected_status.tweak('A/B', status='R ', wc_rev='-', copied='+')
  for child in A_D_children:
    expected_status.add({ child.replace('A/D', 'A/B'):
                          Item(status='  ', wc_rev='-', copied='+')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

def copy_repos_over_deleted_other_kind(sbox):
  "copy repos node over deleted node, other kind"
  sbox.build(read_only = True)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  # Set up some deleted paths
  sbox.simple_rm('iota', 'A/B')
  for path in ['iota', 'A/B'] + A_B_children:
    expected_status.tweak(path, status='D ')

  # Test copying
  main.run_svn(None, 'cp', sbox.repo_url + '/iota', sbox.ospath('A/B'))
  expected_status.tweak('A/B', status='R ', wc_rev='-', copied='+')
  expected_status.remove(*A_B_children)
  main.run_svn(None, 'cp', sbox.repo_url + '/A/B', sbox.ospath('iota'))
  expected_status.tweak('iota', status='R ', wc_rev='-', copied='+')
  for child in A_B_children:
    expected_status.add({ child.replace('A/B', 'iota'):
                          Item(status='  ', wc_rev='-', copied='+')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

def copy_wc_over_deleted_same_kind(sbox):
  "copy WC node over a deleted node, same kind"
  sbox.build(read_only = True)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  # Set up some deleted paths
  sbox.simple_rm('iota', 'A/B')
  for path in ['iota', 'A/B'] + A_B_children:
    expected_status.tweak(path, status='D ')

  # Test copying
  main.run_svn(None, 'cp', sbox.ospath('A/mu'), sbox.ospath('iota'))
  expected_status.tweak('iota', status='R ', wc_rev='-', copied='+')
  main.run_svn(None, 'cp', sbox.ospath('A/D'), sbox.ospath('A/B'))
  expected_status.tweak('A/B', status='R ', wc_rev='-', copied='+')
  for child in A_D_children:
    expected_status.add({ child.replace('A/D', 'A/B'):
                          Item(status='  ', wc_rev='-', copied='+')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

def copy_wc_over_deleted_other_kind(sbox):
  "copy WC node over deleted node, other kind"
  sbox.build(read_only = True)
  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  # Set up some deleted paths
  sbox.simple_rm('iota', 'A/B')
  for path in ['iota', 'A/B'] + A_B_children:
    expected_status.tweak(path, status='D ')

  # Test copying
  main.run_svn(None, 'cp', sbox.ospath('A/mu'), sbox.ospath('A/B'))
  expected_status.tweak('A/B', status='R ', wc_rev='-', copied='+')
  expected_status.remove(*A_B_children)
  main.run_svn(None, 'cp', sbox.ospath('A/D'), sbox.ospath('iota'))
  expected_status.tweak('iota', status='R ', wc_rev='-', copied='+')
  for child in A_D_children:
    expected_status.add({ child.replace('A/D', 'iota'):
                          Item(status='  ', wc_rev='-', copied='+')})
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

def move_wc_and_repo_dir_to_itself(sbox):
  "move wc and repo dir to itself"
  sbox.build(read_only = True)
  wc_dir = os.path.join(sbox.wc_dir, 'A')
  repo_url = sbox.repo_url + '/A'

  # try to move wc dir to itself
  svntest.actions.run_and_verify_svn(None, [],
                                     '.*Cannot move path.* into itself.*',
                                     'move', wc_dir, wc_dir)

  # try to move repo dir to itself
  svntest.actions.run_and_verify_svn(None, [],
                                     '.*Cannot move URL.* into itself.*',
                                     'move', repo_url, repo_url)

@Issues(2763,3314)
def copy_wc_url_with_absent(sbox):
  "copy wc to url with several absent children"
  sbox.build()
  wc_dir = sbox.wc_dir

  # A/B a normal delete
  sbox.simple_rm('A/B')

  # A/no not-present but in HEAD
  sbox.simple_copy('A/mu', 'A/no')
  sbox.simple_commit('A/no')
  svntest.main.run_svn(None, 'up', '-r', '1', sbox.ospath('A/no'))

  # A/mu not-present and not in HEAD
  sbox.simple_rm('A/mu')
  sbox.simple_commit('A/mu')

  # A/D excluded
  svntest.main.run_svn(None, 'up', '--set-depth', 'exclude',
                       os.path.join(sbox.wc_dir, 'A/D'))

  # Test issue #3314 after copy
  sbox.simple_copy('A', 'A_copied')
  svntest.main.run_svn(None, 'ci', os.path.join(sbox.wc_dir, 'A_copied'),
                       '-m', 'Commit A_copied')

  # This tests issue #2763
  svntest.main.run_svn(None, 'cp', os.path.join(sbox.wc_dir, 'A'),
                       '^/A_tagged', '-m', 'Tag A')

  # And perform a normal commit
  svntest.main.run_svn(None, 'ci', os.path.join(sbox.wc_dir, 'A'),
                       '-m', 'Commit A')

  expected_output = svntest.wc.State(wc_dir, {
    'A_tagged'          : Item(status='A '),
    'A_tagged/D'        : Item(status='A '),
    'A_tagged/D/gamma'  : Item(status='A '),
    'A_tagged/D/H'      : Item(status='A '),
    'A_tagged/D/H/psi'  : Item(status='A '),
    'A_tagged/D/H/chi'  : Item(status='A '),
    'A_tagged/D/H/omega': Item(status='A '),
    'A_tagged/D/G'      : Item(status='A '),
    'A_tagged/D/G/pi'   : Item(status='A '),
    'A_tagged/D/G/rho'  : Item(status='A '),
    'A_tagged/D/G/tau'  : Item(status='A '),
    'A_tagged/C'        : Item(status='A '),

    'A/no'              : Item(status='A '),
    })

  # This should bring in A_tagged and A/no
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        None)

  # And now bring in the excluded nodes from A and A_copied
  expected_output = svntest.wc.State(wc_dir, {
    'A/D'               : Item(status='A '),
    'A/D/G'             : Item(status='A '),
    'A/D/G/pi'          : Item(status='A '),
    'A/D/G/tau'         : Item(status='A '),
    'A/D/G/rho'         : Item(status='A '),
    'A/D/H'             : Item(status='A '),
    'A/D/H/psi'         : Item(status='A '),
    'A/D/H/chi'         : Item(status='A '),
    'A/D/H/omega'       : Item(status='A '),
    'A/D/gamma'         : Item(status='A '),

    'A_copied/D'        : Item(status='A '),
    'A_copied/D/H'      : Item(status='A '),
    'A_copied/D/H/omega': Item(status='A '),
    'A_copied/D/H/psi'  : Item(status='A '),
    'A_copied/D/H/chi'  : Item(status='A '),
    'A_copied/D/G'      : Item(status='A '),
    'A_copied/D/G/tau'  : Item(status='A '),
    'A_copied/D/G/rho'  : Item(status='A '),
    'A_copied/D/G/pi'   : Item(status='A '),
    'A_copied/D/gamma'  : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        None, None, None, None, None, False,
                                        wc_dir, '--set-depth', 'infinity')

  # Except for A/no, the 3 directories should now have the same children

  items = {
    ''                  : Item(status='  ', wc_rev='6'),
    'C'                 : Item(status='  ', wc_rev='6'),
    'D'                 : Item(status='  ', wc_rev='6'),
    'D/gamma'           : Item(status='  ', wc_rev='6'),
    'D/H'               : Item(status='  ', wc_rev='6'),
    'D/H/psi'           : Item(status='  ', wc_rev='6'),
    'D/H/chi'           : Item(status='  ', wc_rev='6'),
    'D/H/omega'         : Item(status='  ', wc_rev='6'),
    'D/G'               : Item(status='  ', wc_rev='6'),
    'D/G/pi'            : Item(status='  ', wc_rev='6'),
    'D/G/tau'           : Item(status='  ', wc_rev='6'),
    'D/G/rho'           : Item(status='  ', wc_rev='6'),
  }

  expected_status = svntest.wc.State(os.path.join(wc_dir, 'A_copied'), items)
  svntest.actions.run_and_verify_status(os.path.join(wc_dir, 'A_copied'),
                                        expected_status)

  expected_status = svntest.wc.State(os.path.join(wc_dir, 'A_tagged'), items)
  svntest.actions.run_and_verify_status(os.path.join(wc_dir, 'A_tagged'),
                                        expected_status)

  expected_status.add({
    'no'                : Item(status='  ', wc_rev='6')
  })

  expected_status = svntest.wc.State(os.path.join(wc_dir, 'A'), items)
  svntest.actions.run_and_verify_status(os.path.join(wc_dir, 'A'),
                                        expected_status)


def copy_url_shortcut(sbox):
  "copy using URL shortcut source"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Can't use ^/A/D/G shortcut here because wc/X is unversioned.
  svntest.actions.run_and_verify_svn(None, None, [], 'copy',
                                     sbox.ospath('A/D/G'), sbox.ospath('X'))

  svntest.actions.run_and_verify_svn(None, None, [], 'rm',
                                     sbox.ospath('X/pi'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X'     : Item(status='A ', copied='+', wc_rev='-'),
    'X/pi'  : Item(status='D ', copied='+', wc_rev='-'),
    'X/rho' : Item(status='  ', copied='+', wc_rev='-'),
    'X/tau' : Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Can use ^/A/D/G even though X/pi is a delete within a copy.
  svntest.actions.run_and_verify_svn(None, None, [], 'copy',
                                     '^/A/D/G/pi', sbox.ospath('X/pi'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X'     : Item(status='A ', copied='+', wc_rev='-'),
    'X/pi'  : Item(status='R ', copied='+', wc_rev='-', entry_status='  '),
    'X/rho' : Item(status='  ', copied='+', wc_rev='-'),
    'X/tau' : Item(status='  ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)


# Regression test for issue #3865: 'svn' on Windows cannot address
# scheduled-for-delete file, if another file differing only in case is
# present on disk
@Issue(3865)
def deleted_file_with_case_clash(sbox):
  """address a deleted file hidden by case clash"""

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  iota2_path = os.path.join(wc_dir, 'iota2')
  IOTA_path = os.path.join(wc_dir, 'IOTA')
  iota_url = sbox.repo_url + '/iota'

  # Perform a case-only rename in two steps.
  svntest.main.run_svn(None, 'move', iota_path, iota2_path)
  svntest.main.run_svn(None, 'move', iota2_path, IOTA_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota' : Item(status='D ', wc_rev=1),
    'IOTA' : Item(status='A ', copied='+', wc_rev='-'),
    })

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Perform 'info' calls on both the deleted and added paths, to see if
  # we get the correct information. The deleted path is not on disk and
  # hidden by the on-disk case-clashing file, but we should be able to
  # target it explicitly because it's in the wc-db.
  expected_info_iota = {'Path' : re.escape(iota_path),
                        'Schedule' : 'delete',
                        'Copied From URL': None,
                       }
  svntest.actions.run_and_verify_info([expected_info_iota], iota_path)

  expected_info_IOTA = {'Path' : re.escape(IOTA_path),
                        'Schedule' : 'add',
                        'Copied From URL': iota_url,
                       }
  svntest.actions.run_and_verify_info([expected_info_IOTA], IOTA_path)

def copy_base_of_deleted(sbox):
  """copy -rBASE deleted"""

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None, None, [], 'rm', sbox.ospath('A/mu'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [], 'cp', '-rBASE',
                                     sbox.ospath('A/mu'), sbox.ospath('A/mu2'))
  expected_status.add({
    'A/mu2' : Item(status='A ', copied='+', wc_rev='-'),
    })


# Regression test for issue #3702: Unable to perform case-only rename
# on windows.
@Issue(3702)
# APR's apr_filepath_merge() with APR_FILEPATH_TRUENAME is broken on OS X.
@XFail(svntest.main.is_os_darwin)
def case_only_rename(sbox):
  """case-only rename"""

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  IoTa_path = os.path.join(wc_dir, 'IoTa')
  B_path = os.path.join(wc_dir, 'A/B')
  b_path = os.path.join(wc_dir, 'A/b')

  # Perform a couple of case-only renames.
  svntest.main.run_svn(None, 'move', iota_path, IoTa_path)
  svntest.main.run_svn(None, 'move', B_path, b_path)

  # Create expected status.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota'              : Item(status='D ', wc_rev=1),
    'IoTa'              : Item(status='A ', copied='+', wc_rev='-'),
    'A/B'               : Item(status='D ', wc_rev='1'),
    'A/B/lambda'        : Item(status='D ', wc_rev='1'),
    'A/B/E'             : Item(status='D ', wc_rev='1'),
    'A/B/E/alpha'       : Item(status='D ', wc_rev='1'),
    'A/B/E/beta'        : Item(status='D ', wc_rev='1'),
    'A/B/F'             : Item(status='D ', wc_rev='1'),
    'A/b'               : Item(status='A ', copied='+', wc_rev='-'),
    'A/b/E'             : Item(status='  ', copied='+', wc_rev='-'),
    'A/b/E/beta'        : Item(status='  ', copied='+', wc_rev='-'),
    'A/b/E/alpha'       : Item(status='  ', copied='+', wc_rev='-'),
    'A/b/F'             : Item(status='  ', copied='+', wc_rev='-'),
    'A/b/lambda'        : Item(status='  ', copied='+', wc_rev='-'),
    })

  # Test that the necessary deletes and adds are present in status.
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@XFail()
@Issue(3899)
def copy_and_move_conflicts(sbox):
  """copy and move conflicts"""

  # The destination of a copy or move operation should *not* be
  # conflicted, and should contain the "mine-full" contents.

  sbox.build()
  wc = sbox.ospath
  def url(relpath):
    return '/'.join([sbox.repo_url, relpath])

  # Create an assortment of conflicts.
  #   text                                 A/B/E/alpha
  #   text (resolved by deleting markers)  A/B/E/alpha
  #   property (dir)                       A/D/H
  #   property (file)                      A/D/H/chi
  #   tree: local delete, incoming edit    A/D/gamma
  #   tree: local edit, incoming delete    A/D/G
  #   tree: local add, incoming add        A/Q
  #   tree: local missing, incoming edit   A/B/E/sigma

  ### As we improve tree-conflict handling, this test may need some
  ### maintenance.

  # Create a branch for merging.
  run_svn(None, 'cp', url('A'), url('A2'), '-m', make_log_msg()) # r2
  sbox.simple_update()

  # This revision won't be included in the merge, producing a "local
  # missing" tree conflict.
  file_write(wc('A2/B/E/sigma'), "New for merge.\n")
  sbox.simple_add('A2/B/E/sigma')

  sbox.simple_commit('A2') # r3

  # Make "incoming" changes in A2 for the merge
  # incoming edits
  file_append(wc('A2/B/E/alpha'), "Edit for merge\n")
  file_append(wc('A2/B/E/beta'), "Edit for merge\n")
  file_append(wc('A2/B/E/sigma'), "Edit for merge\n")
  sbox.simple_propset('foo', '99', 'A2/D/H')
  sbox.simple_propset('foo', '99', 'A2/D/H/chi')
  # incoming add
  sbox.simple_mkdir('A2/Q')
  file_write(wc('A2/Q/zeta'), "New for merge\n")
  sbox.simple_add('A2/Q/zeta')

  sbox.simple_commit('A2') # r4

  # Make some "local" changes in A before the merge.
  # local edit
  file_append(wc('A/B/E/alpha'), "Local edit\n")
  file_append(wc('A/B/E/beta'), "Local edit\n")
  sbox.simple_propset('foo', '100', 'A/D/H')
  sbox.simple_propset('foo', '100', 'A/D/H/chi')
  # local add
  sbox.simple_mkdir('A/Q')
  file_write(wc('A/Q/sigma'), "New local file\n")
  sbox.simple_add('A/Q/sigma')

  # Make some "incoming" changes in A before the update.
  # incoming edit
  file_append(wc('A/D/gamma'), "Edit for merge\n")
  # incoming delete
  sbox.simple_rm('A/D/G')

  sbox.simple_commit('A') # r5

  # Roll back, make local, uncommitted changes.
  run_svn(None, 'up', '-r', 4, sbox.wc_dir)
  # local delete
  sbox.simple_rm('A/D/gamma')
  # local edit
  file_append(wc('A/D/G/rho'), "Local edit\n")

  # Update to reveal the "local {delete,edit'}" tree conflicts,
  # which we can't yet catch when merging.
  sbox.simple_update()

  # Merge just one revision to reveal more conflicts.
  run_svn(None, 'merge', '-c', 4, url('A2'), wc('A'))

  # Resolve one text conflict via marker file deletion.
  os.remove(wc('A/B/E/beta.merge-left.r3'))
  os.remove(wc('A/B/E/beta.working'))
  os.remove(wc('A/B/E/beta'))
  os.rename(wc('A/B/E/beta.merge-right.r4'), wc('A/B/E/beta'))

  # Prepare for local copies and moves.
  sbox.simple_mkdir('copy-dest')
  sbox.simple_mkdir('move-dest')

  # Copy conflict victims.
  sbox.simple_copy('A/B/E/alpha', 'copy-dest')
  sbox.simple_copy('A/D/H', 'copy-dest')
  sbox.simple_copy('A/D/G', 'copy-dest')
  sbox.simple_copy('A/Q', 'copy-dest')

  # Copy directories with conflicted children.
  sbox.simple_copy('A/B', 'copy-dest')
  sbox.simple_copy('A/D', 'copy-dest')

  # Everything copied without conflicts.  The entry_status for D/G is
  # for 1.6 compatibility (see notes/api-errata/1.7/wc003.xt).
  expected_status = svntest.wc.State(wc('copy-dest'), {
    ''                  : Item(status='A ', wc_rev=0),
    'B'                 : Item(status='A ', copied='+', wc_rev='-'),
    'B/E'               : Item(status='  ', copied='+', wc_rev='-'),
    'B/E/alpha'         : Item(status='  ', copied='+', wc_rev='-'),
    'B/E/beta'          : Item(status='M ', copied='+', wc_rev='-'),
    'B/F'               : Item(status='  ', copied='+', wc_rev='-'),
    'B/lambda'          : Item(status='  ', copied='+', wc_rev='-'),
    'D'                 : Item(status='A ', copied='+', wc_rev='-'),
    'D/G'               : Item(status='A ', copied='+', wc_rev='-',
                               entry_status='  '),
    'D/G/pi'            : Item(status='  ', copied='+', wc_rev='-'),
    'D/G/rho'           : Item(status='M ', copied='+', wc_rev='-'),
    'D/G/tau'           : Item(status='  ', copied='+', wc_rev='-'),
    'D/H'               : Item(status='  ', copied='+', wc_rev='-'),
    'D/H/chi'           : Item(status='  ', copied='+', wc_rev='-'),
    'D/H/omega'         : Item(status='  ', copied='+', wc_rev='-'),
    'D/H/psi'           : Item(status='  ', copied='+', wc_rev='-'),
    'D/gamma'           : Item(status='D ', copied='+', wc_rev='-'),
    'G'                 : Item(status='A ', copied='+', wc_rev='-'),
    'G/pi'              : Item(status='  ', copied='+', wc_rev='-'),
    'G/rho'             : Item(status='M ', copied='+', wc_rev='-'),
    'G/tau'             : Item(status='  ', copied='+', wc_rev='-'),
    'H'                 : Item(status='A ', copied='+', wc_rev='-'),
    'H/chi'             : Item(status='  ', copied='+', wc_rev='-'),
    'H/omega'           : Item(status='  ', copied='+', wc_rev='-'),
    'H/psi'             : Item(status='  ', copied='+', wc_rev='-'),
    'Q'                 : Item(status='A ', copied='+', wc_rev='-'),
    'Q/sigma'           : Item(status='  ', copied='+', wc_rev='-'),
    'alpha'             : Item(status='A ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc('copy-dest'), expected_status)

  # Only the local changes appear at the copy destinations.  Note that
  # B/E/beta had been resolved via marker-file deletion before the copy.
  expected_disk = svntest.wc.State('', {
    'B/E/alpha'         : Item(contents="This is the file 'alpha'.\n"
                               "Local edit\n"),
    'B/E/beta'          : Item(contents="This is the file 'beta'.\n"
                               "Edit for merge\n"),
    'B/F'               : Item(),
    'B/lambda'          : Item(contents="This is the file 'lambda'.\n"),
    'D/G/pi'            : Item(contents="This is the file 'pi'.\n"),
    'D/G/rho'           : Item(contents="This is the file 'rho'.\n"
                               "Local edit\n"),
    'D/G/tau'           : Item(contents="This is the file 'tau'.\n"),
    'D/H'               : Item(props={'foo':'100'}),
    'D/H/chi'           : Item(contents="This is the file 'chi'.\n",
                               props={'foo':'100'}),
    'D/H/omega'         : Item(contents="This is the file 'omega'.\n"),
    'D/H/psi'           : Item(contents="This is the file 'psi'.\n"),
    'G/pi'              : Item(contents="This is the file 'pi'.\n"),
    'G/rho'             : Item(contents="This is the file 'rho'.\n"
                               "Local edit\n"),
    'G/tau'             : Item(contents="This is the file 'tau'.\n"),
    'H'                 : Item(props={'foo':'100'}),
    'H/chi'             : Item(contents="This is the file 'chi'.\n",
                               props={'foo':'100'}),
    'H/omega'           : Item(contents="This is the file 'omega'.\n"),
    'H/psi'             : Item(contents="This is the file 'psi'.\n"),
    'Q/sigma'           : Item(contents="New local file\n"),
    'alpha'             : Item(contents="This is the file 'alpha'.\n"
                               "Local edit\n"),
    })
  svntest.actions.verify_disk(wc('copy-dest'), expected_disk, True)

  # Move conflict victims.
  sbox.simple_move('A/B/E/alpha', 'move-dest')
  sbox.simple_move('A/D/H', 'move-dest')
  sbox.simple_move('A/D/G', 'move-dest')
  sbox.simple_move('A/Q', 'move-dest')

  # Move directories with conflicted children.
  sbox.simple_move('A/B', 'move-dest')
  sbox.simple_move('A/D', 'move-dest')

  # Expect same status and disk content as at the copy destination, except
  # that A/B/E/alpha, A/D/G, and A/D/H were moved away first.
  expected_status.wc_dir = wc('move-dest')
  expected_status.tweak('B/E/alpha',
                        'D/H',
                        'D/H/chi',
                        'D/H/omega',
                        'D/H/psi',
                        status='D ')
  # A/D/G had been re-added from r4 due to a "local edit, incoming delete"
  # tree conflict, so moving it away has a different effect.
  expected_status.remove('D/G',
                         'D/G/pi',
                         'D/G/rho',
                         'D/G/tau')
  svntest.actions.run_and_verify_status(wc('move-dest'), expected_status)

  expected_disk = svntest.wc.State('', {
    'B/E/beta'          : Item(contents="This is the file 'beta'.\n"
                               "Edit for merge\n"),
    'B/lambda'          : Item(contents="This is the file 'lambda'.\n"),
    'B/F'               : Item(),
    'H'                 : Item(props={'foo':'100'}),
    'H/chi'             : Item(contents="This is the file 'chi'.\n",
                               props={'foo':'100'}),
    'H/psi'             : Item(contents="This is the file 'psi'.\n"),
    'H/omega'           : Item(contents="This is the file 'omega'.\n"),
    'D'                 : Item(),
    'G/tau'             : Item(contents="This is the file 'tau'.\n"),
    'G/rho'             : Item(contents="This is the file 'rho'.\n"
                               "Local edit\n"),
    'G/pi'              : Item(contents="This is the file 'pi'.\n"),
    'Q/sigma'           : Item(contents="New local file\n"),
    'alpha'             : Item(contents="This is the file 'alpha'.\n"
                               "Local edit\n"),
    })
  svntest.actions.verify_disk(wc('move-dest'), expected_disk, True)

def copy_deleted_dir(sbox):
  "try to copy a deleted directory that exists"
  sbox.build(read_only = True)

  sbox.simple_rm('iota')
  sbox.simple_rm('A')

  svntest.actions.run_and_verify_svn(None, None,
                                     'svn: E145000: Path.* does not exist',
                                     'cp', sbox.ospath('iota'),
                                     sbox.ospath('new_iota'))
  svntest.actions.run_and_verify_svn(None, None,
                                     'svn: E145000: Path.* does not exist',
                                     'cp', sbox.ospath('A/D'),
                                     sbox.ospath('new_D'))

  svntest.main.file_write(sbox.ospath('iota'), 'Not iota!')
  os.mkdir(sbox.ospath('A'))
  os.mkdir(sbox.ospath('A/D'))

  # These two invocations raise an assertion.
  svntest.actions.run_and_verify_svn(None, None,
                                     'svn: E155035: Deleted node.* can\'t be.*',
                                     'cp', sbox.ospath('iota'),
                                     sbox.ospath('new_iota'))
  svntest.actions.run_and_verify_svn(None, None,
                                     'svn: E155035: Deleted node.* can\'t be.*',
                                     'cp', sbox.ospath('A/D'),
                                     sbox.ospath('new_D'))

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              basic_copy_and_move_files,
              receive_copy_in_update,
              resurrect_deleted_dir,
              no_copy_overwrites,
              no_wc_copy_overwrites,
              copy_modify_commit,
              copy_files_with_properties,
              copy_delete_commit,
              mv_and_revert_directory,
              copy_preserve_executable_bit,
              wc_to_repos,
              repos_to_wc,
              copy_to_root,
              url_copy_parent_into_child,
              wc_copy_parent_into_child,
              resurrect_deleted_file,
              diff_repos_to_wc_copy,
              repos_to_wc_copy_eol_keywords,
              revision_kinds_local_source,
              copy_over_missing_file,
              repos_to_wc_1634,
              double_uri_escaping_1814,
              wc_to_wc_copy_between_different_repos,
              wc_to_wc_copy_deleted,
              url_to_non_existent_url_path,
              non_existent_url_to_url,
              old_dir_url_to_url,
              wc_copy_dir_to_itself,
              mixed_wc_to_url,
              wc_copy_replacement,
              wc_copy_replace_with_props,
              repos_to_wc_copy_replacement,
              repos_to_wc_copy_replace_with_props,
              delete_replaced_file,
              mv_unversioned_file,
              force_move,
              copy_deleted_dir_into_prefix,
              copy_copied_file_and_dir,
              move_copied_file_and_dir,
              move_moved_file_and_dir,
              move_file_within_moved_dir,
              move_file_out_of_moved_dir,
              move_dir_within_moved_dir,
              move_dir_out_of_moved_dir,
              move_file_back_and_forth,
              move_dir_back_and_forth,
              copy_move_added_paths,
              copy_added_paths_with_props,
              copy_added_paths_to_URL,
              move_to_relative_paths,
              move_from_relative_paths,
              copy_to_relative_paths,
              copy_from_relative_paths,
              move_multiple_wc,
              copy_multiple_wc,
              move_multiple_repo,
              copy_multiple_repo,
              copy_multiple_repo_wc,
              copy_multiple_wc_repo,
              copy_peg_rev_local_files,
              copy_peg_rev_local_dirs,
              copy_peg_rev_url,
              old_dir_wc_to_wc,
              copy_make_parents_wc_wc,
              copy_make_parents_repo_wc,
              copy_make_parents_wc_repo,
              copy_make_parents_repo_repo,
              URI_encoded_repos_to_wc,
              allow_unversioned_parent_for_copy_src,
              unneeded_parents,
              double_parents_with_url,
              copy_into_absent_dir,
              find_copyfrom_information_upstairs,
              path_move_and_copy_between_wcs_2475,
              path_copy_in_repo_2475,
              commit_copy_depth_empty,
              copy_below_copy,
              move_below_move,
              reverse_merge_move,
              nonrecursive_commit_of_copy,
              copy_added_dir_with_copy,
              copy_broken_symlink,
              move_dir_containing_move,
              copy_dir_with_space,
              changed_data_should_match_checkout,
              changed_dir_data_should_match_checkout,
              move_added_nodes,
              copy_over_deleted_dir,
              mixed_rev_copy_del,
              copy_delete_delete,
              copy_delete_revert,
              delete_replace_delete,
              copy_repos_over_deleted_same_kind,
              copy_repos_over_deleted_other_kind,
              copy_wc_over_deleted_same_kind,
              copy_wc_over_deleted_other_kind,
              move_wc_and_repo_dir_to_itself,
              copy_wc_url_with_absent,
              copy_url_shortcut,
              deleted_file_with_case_clash,
              copy_base_of_deleted,
              case_only_rename,
              copy_and_move_conflicts,
              copy_deleted_dir,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
