#!/usr/bin/env python
#
#  revert_tests.py:  testing 'svn revert'.
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
import re, os, stat, shutil

# Our testing module
import svntest
from svntest import wc, main, actions
from svntest.actions import run_and_verify_svn
from svntest.main import file_append, file_write, run_svn

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


######################################################################
# Helpers

def revert_replacement_with_props(sbox, wc_copy):
  """Helper implementing the core of
  revert_{repos,wc}_to_wc_replace_with_props().

  Uses a working copy (when wc_copy == True) or a URL (when wc_copy ==
  False) source to copy from."""

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
                                     'ps', 'phony-prop', '-F', prop_path,
                                     pi_path)
  os.remove(prop_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'svn:eol-style', 'LF', rho_path)

  # Verify props having been set
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
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
    props = { 'phony-prop' : '*' }
  else:
    props = { 'phony-prop' : '*' }

  expected_disk.tweak('A/D/G/rho',
                      contents="This is the file 'pi'.\n",
                      props=props)
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())

  # Now revert
  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_status.tweak('A/D/G/rho', status='  ', copied=None, wc_rev='2')
  expected_output = ["Reverted '" + rho_path + "'\n"]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '-R', wc_dir)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Check disk status
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk.tweak('A/D/G/pi',
                      props={ 'phony-prop': '*' })
  expected_disk.tweak('A/D/G/rho',
                      props={ 'svn:eol-style': 'LF' })
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())




######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

def revert_from_wc_root(sbox):
  "revert relative to wc root"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  os.chdir(wc_dir)

  # Mostly taken from basic_revert
  # Modify some files and props.
  beta_path = os.path.join('A', 'B', 'E', 'beta')
  gamma_path = os.path.join('A', 'D', 'gamma')
  iota_path = 'iota'
  rho_path = os.path.join('A', 'D', 'G', 'rho')
  zeta_path = os.path.join('A', 'D', 'H', 'zeta')
  svntest.main.file_append(beta_path, "Added some text to 'beta'.\n")
  svntest.main.file_append(iota_path, "Added some text to 'iota'.\n")
  svntest.main.file_append(rho_path, "Added some text to 'rho'.\n")
  svntest.main.file_append(zeta_path, "Added some text to 'zeta'.\n")

  svntest.actions.run_and_verify_svn("Add command", None, [],
                                     'add', zeta_path)
  svntest.actions.run_and_verify_svn("Add prop command", None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     gamma_path)
  svntest.actions.run_and_verify_svn("Add prop command", None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     iota_path)
  svntest.actions.run_and_verify_svn("Add prop command", None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     '.')
  svntest.actions.run_and_verify_svn("Add prop command", None, [],
                                     'ps', 'random-prop', 'propvalue',
                                     'A')

  # Verify modified status.
  expected_output = svntest.actions.get_virginal_state('', 1)
  expected_output.tweak('A/B/E/beta', 'A/D/G/rho', status='M ')
  expected_output.tweak('iota', status='MM')
  expected_output.tweak('', 'A/D/gamma', 'A', status=' M')
  expected_output.add({
    'A/D/H/zeta' : Item(status='A ', wc_rev=0),
    })

  svntest.actions.run_and_verify_status('', expected_output)

  # Run revert
  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', beta_path)

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', gamma_path)

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', iota_path)

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', rho_path)

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', zeta_path)

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', '.')

  svntest.actions.run_and_verify_svn("Revert command", None, [],
                                     'revert', 'A')

  # Verify unmodified status.
  expected_output = svntest.actions.get_virginal_state('', 1)

  svntest.actions.run_and_verify_status('', expected_output)

@Issue(1663)
def revert_reexpand_keyword(sbox):
  "revert reexpands manually contracted keyword"

  # This is for issue #1663.  The bug is that if the only difference
  # between a locally modified working file and the base version of
  # same was that the former had a contracted keyword that would be
  # expanded in the latter, then 'svn revert' wouldn't notice the
  # difference, and therefore wouldn't revert.  And why wouldn't it
  # notice?  Since text bases are always stored with keywords
  # contracted, and working files are contracted before comparison
  # with text base, there would appear to be no difference when the
  # contraction is the only difference.  For most commands, this is
  # correct -- but revert's job is to restore the working file, not
  # the text base.

  sbox.build()
  wc_dir = sbox.wc_dir
  newfile_path = os.path.join(wc_dir, "newfile")
  unexpanded_contents = "This is newfile: $Rev$.\n"

  # Put an unexpanded keyword into iota.
  svntest.main.file_write(newfile_path, unexpanded_contents)

  # Commit, without svn:keywords property set.
  svntest.main.run_svn(None, 'add', newfile_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'r2', newfile_path)

  # Set the property and commit.  This should expand the keyword.
  svntest.main.run_svn(None, 'propset', 'svn:keywords', 'rev', newfile_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'r3', newfile_path)

  # Verify that the keyword got expanded.
  def check_expanded(path):
    fp = open(path, 'r')
    lines = fp.readlines()
    fp.close()
    if lines[0] != "This is newfile: $Rev: 3 $.\n":
      raise svntest.Failure

  check_expanded(newfile_path)

  # Now un-expand the keyword again.
  svntest.main.file_write(newfile_path, unexpanded_contents)

  # Revert the file.  The keyword should reexpand.
  svntest.main.run_svn(None, 'revert', newfile_path)

  # Verify that the keyword got re-expanded.
  check_expanded(newfile_path)

  # Ok, the first part of this test was written in 2004. We are now in 2011
  # and note that there is more to test:

  # If the recorded timestamp and size match the file then revert won't
  # reinstall the file as the file was not modified when last compared in
  # the repository normal form.
  #
  # The easiest way to get the information recorded would be calling cleanup,
  # because that 'repairs' the recorded information. But some developers
  # (including me) would call that cheating, so I just use a failed commit.

  # Un-expand the keyword again.
  svntest.main.file_write(newfile_path, unexpanded_contents)

  # And now we trick svn in ignoring the file on newfile_path
  newfile2_path = newfile_path + '2'
  svntest.main.file_write(newfile2_path, 'This is file 2')
  svntest.main.run_svn(None, 'add', newfile2_path)
  os.remove(newfile2_path)

  # This commit fails because newfile2_path is missing, but only after
  # we call svn_wc__internal_file_modified_p() on new_file.
  svntest.actions.run_and_verify_commit(wc_dir, None, None, "2' is scheduled"+
                                        " for addition, but is missing",
                                        newfile_path, newfile2_path,
                                        '-m', "Shouldn't be committed")

  # Revert the file.  The file is not reverted!
  svntest.actions.run_and_verify_svn(None, [], [], 'revert', newfile_path)


#----------------------------------------------------------------------
# Regression test for issue #1775:
# Should be able to revert a file with no properties i.e. no prop-base
@Issue(1775)
def revert_replaced_file_without_props(sbox):
  "revert a replaced file with no properties"

  sbox.build()
  wc_dir = sbox.wc_dir

  file1_path = os.path.join(wc_dir, 'file1')

  # Add a new file, file1, that has no prop-base
  svntest.main.file_append(file1_path, "This is the file 'file1' revision 2.")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', file1_path)

  # commit file1
  expected_output = svntest.wc.State(wc_dir, {
    'file1' : Item(verb='Adding')
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'file1' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # delete file1
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', file1_path)

  # test that file1 is scheduled for deletion.
  expected_status.tweak('file1', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # recreate and add file1
  svntest.main.file_append(file1_path, "This is the file 'file1' revision 3.")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', file1_path)

  # Test to see if file1 is schedule for replacement
  expected_status.tweak('file1', status='R ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # revert file1
  svntest.actions.run_and_verify_svn(None, ["Reverted '" + file1_path + "'\n"],
                                     [], 'revert', file1_path)

  # test that file1 really was reverted
  expected_status.tweak('file1', status='  ', wc_rev=2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
# Regression test for issue #876:
# svn revert of an svn move'd file does not revert the file
@XFail()
@Issue(876)
def revert_moved_file(sbox):
    "revert a moved file"

    sbox.build(read_only = True)
    wc_dir = sbox.wc_dir
    iota_path = os.path.join(wc_dir, 'iota')
    iota_path_moved = os.path.join(wc_dir, 'iota_moved')

    svntest.actions.run_and_verify_svn(None, None, [], 'mv', iota_path,
                                        iota_path_moved)
    expected_output = svntest.actions.get_virginal_state(wc_dir, 1)
    expected_output.tweak('iota', status='D ')
    expected_output.add({
      'iota_moved' : Item(status='A ', copied='+', wc_rev='-'),
    })
    svntest.actions.run_and_verify_status(wc_dir, expected_output)

    # now revert the file iota
    svntest.actions.run_and_verify_svn(None,
      ["Reverted '" + iota_path + "'\n"], [], 'revert', iota_path)

    # at this point, svn status on iota_path_moved should return nothing
    # since it should disappear on reverting the move, and since svn status
    # on a non-existent file returns nothing.

    svntest.actions.run_and_verify_svn(None, [], [],
                                      'status', '-v', iota_path_moved)


#----------------------------------------------------------------------
# Test for issue 2135
#
# It is like merge_file_replace (in merge_tests.py), but reverts file
# instead of commit.
@Issue(2135)
def revert_file_merge_replace_with_history(sbox):
  "revert a merge replacement of file with history"

  sbox.build()
  wc_dir = sbox.wc_dir

  # File scheduled for deletion
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', rho_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho': Item(verb='Deleting'),
    })

  expected_status.remove('A/D/G/rho')

  # Commit rev 2
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)
  # create new rho file
  svntest.main.file_write(rho_path, "new rho\n")

  # Add the new file
  svntest.actions.run_and_verify_svn(None, None, [], 'add', rho_path)

  # Commit revsion 3
  expected_status.add({
    'A/D/G/rho' : Item(status='A ', wc_rev='0')
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho': Item(verb='Adding'),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None, None, wc_dir)

  # Update working copy
  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk   = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/G/rho', contents='new rho\n' )
  expected_status.tweak(wc_rev='3')
  expected_status.tweak('A/D/G/rho', status='  ')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status)

  # merge changes from r3:1
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho': Item(status='R ')
    })
  expected_mergeinfo_output = svntest.wc.State(wc_dir, {
    '' : Item(status=' U')
    })
  expected_elision_output = svntest.wc.State(wc_dir, {
    '' : Item(status=' U')
    })
  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  expected_skip = wc.State(wc_dir, { })
  expected_disk.tweak('A/D/G/rho', contents="This is the file 'rho'.\n")
  svntest.actions.run_and_verify_merge(wc_dir, '3', '1',
                                       sbox.repo_url, None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip)

  # Now revert
  svntest.actions.run_and_verify_svn(None,
                                     None,
                                     [], 'revert', rho_path)

  # test that rho really was reverted
  expected_status.tweak('A/D/G/rho', copied=None, status='  ', wc_rev=3)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  expected_disk.tweak('A/D/G/rho', contents="new rho\n")
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())

  # Make sure the revert removed the copy from information.
  expected_infos = [
      { 'Copied' : None }
    ]
  svntest.actions.run_and_verify_info(expected_infos, rho_path)

def revert_wc_to_wc_replace_with_props(sbox):
  "revert svn cp PATH PATH replace file with props"

  revert_replacement_with_props(sbox, 1)

def revert_repos_to_wc_replace_with_props(sbox):
  "revert svn cp URL PATH replace file with props"

  revert_replacement_with_props(sbox, 0)

def revert_after_second_replace(sbox):
  "revert file after second replace"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # File scheduled for deletion
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', rho_path)

  # Status before attempting copy
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Replace file for the first time
  pi_src = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', pi_src, rho_path)

  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now delete replaced file.
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', '--force', rho_path)

  # Status should be same as after first delete
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Replace file for the second time
  pi_src = os.path.join(wc_dir, 'A', 'D', 'G', 'pi')

  svntest.actions.run_and_verify_svn(None, None, [], 'cp', pi_src, rho_path)

  expected_status.tweak('A/D/G/rho', status='R ', copied='+', wc_rev='-')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now revert
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', '-R', wc_dir)

  # Check disk status
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())


#----------------------------------------------------------------------
# Tests for issue #2517.
#
# Manual conflict resolution leads to spurious revert report.
@Issue(2517)
def revert_after_manual_conflict_resolution__text(sbox):
  "revert after manual text-conflict resolution"

  # Make two working copies
  sbox.build()
  wc_dir_1 = sbox.wc_dir
  wc_dir_2 = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir_1, wc_dir_2)

  # Cause a (text) conflict
  iota_path_1 = os.path.join(wc_dir_1, 'iota')
  iota_path_2 = os.path.join(wc_dir_2, 'iota')
  svntest.main.file_write(iota_path_1, 'Modified iota text')
  svntest.main.file_write(iota_path_2, 'Conflicting iota text')
  svntest.main.run_svn(None,
                       'commit', '-m', 'r2', wc_dir_1)
  svntest.main.run_svn(None,
                       'update', wc_dir_2)

  # Resolve the conflict "manually"
  svntest.main.file_write(iota_path_2, 'Modified iota text')
  os.remove(iota_path_2 + '.mine')
  os.remove(iota_path_2 + '.r1')
  os.remove(iota_path_2 + '.r2')

  # Verify no output from status, diff, or revert
  svntest.actions.run_and_verify_svn(None, [], [], "status", wc_dir_2)
  svntest.actions.run_and_verify_svn(None, [], [], "diff", wc_dir_2)
  svntest.actions.run_and_verify_svn(None, [], [], "revert", "-R", wc_dir_2)

def revert_after_manual_conflict_resolution__prop(sbox):
  "revert after manual property-conflict resolution"

  # Make two working copies
  sbox.build()
  wc_dir_1 = sbox.wc_dir
  wc_dir_2 = sbox.add_wc_path('other')
  svntest.actions.duplicate_dir(wc_dir_1, wc_dir_2)

  # Cause a (property) conflict
  iota_path_1 = os.path.join(wc_dir_1, 'iota')
  iota_path_2 = os.path.join(wc_dir_2, 'iota')
  svntest.main.run_svn(None, 'propset', 'foo', '1', iota_path_1)
  svntest.main.run_svn(None, 'propset', 'foo', '2', iota_path_2)
  svntest.main.run_svn(None,
                       'commit', '-m', 'r2', wc_dir_1)
  svntest.main.run_svn(None,
                       'update', wc_dir_2)

  # Resolve the conflict "manually"
  svntest.main.run_svn(None, 'propset', 'foo', '1', iota_path_2)
  os.remove(iota_path_2 + '.prej')

  # Verify no output from status, diff, or revert
  svntest.actions.run_and_verify_svn(None, [], [], "status", wc_dir_2)
  svntest.actions.run_and_verify_svn(None, [], [], "diff", wc_dir_2)
  svntest.actions.run_and_verify_svn(None, [], [], "revert", "-R", wc_dir_2)

def revert_propset__dir(sbox):
  "revert a simple propset on a dir"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  a_path = os.path.join(wc_dir, 'A')
  svntest.main.run_svn(None, 'propset', 'foo', 'x', a_path)
  expected_output = re.escape("Reverted '" + a_path + "'")
  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert",
                                     a_path)

def revert_propset__file(sbox):
  "revert a simple propset on a file"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  svntest.main.run_svn(None, 'propset', 'foo', 'x', iota_path)
  expected_output = re.escape("Reverted '" + iota_path + "'")
  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert",
                                     iota_path)

def revert_propdel__dir(sbox):
  "revert a simple propdel on a dir"

  sbox.build()
  wc_dir = sbox.wc_dir
  a_path = os.path.join(wc_dir, 'A')
  svntest.main.run_svn(None, 'propset', 'foo', 'x', a_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'ps', a_path)
  svntest.main.run_svn(None, 'propdel', 'foo', a_path)
  expected_output = re.escape("Reverted '" + a_path + "'")
  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert",
                                     a_path)

def revert_propdel__file(sbox):
  "revert a simple propdel on a file"

  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  svntest.main.run_svn(None, 'propset', 'foo', 'x', iota_path)
  svntest.main.run_svn(None,
                       'commit', '-m', 'ps', iota_path)
  svntest.main.run_svn(None, 'propdel', 'foo', iota_path)
  expected_output = re.escape("Reverted '" + iota_path + "'")
  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert",
                                     iota_path)

def revert_replaced_with_history_file_1(sbox):
  "revert a committed replace-with-history == no-op"

  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Remember the original text of 'mu'
  exit_code, text_r1, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                               'cat', mu_path)
  # delete mu and replace it with a copy of iota
  svntest.main.run_svn(None, 'rm', mu_path)
  svntest.main.run_svn(None, 'mv', iota_path, mu_path)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='  ', wc_rev=2)
  expected_status.remove('iota')
  expected_output = svntest.wc.State(wc_dir, {
    'iota': Item(verb='Deleting'),
    'A/mu': Item(verb='Replacing'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # update the working copy
  svntest.main.run_svn(None, 'up', wc_dir)

  # now revert back to the state in r1
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu': Item(status='R '),
    'iota': Item(status='A ')
    })
  expected_mergeinfo_output = svntest.wc.State(wc_dir, {
    '': Item(status=' U'),
    })
  expected_elision_output = svntest.wc.State(wc_dir, {
    '': Item(status=' U'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/mu', status='R ', copied='+', wc_rev='-')
  expected_status.tweak('iota', status='A ', copied='+', wc_rev='-')
  expected_skip = wc.State(wc_dir, { })
  expected_disk = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_merge(wc_dir, '2', '1',
                                       sbox.repo_url, None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip)

  # and commit in r3
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/mu', status='  ', wc_rev=3)
  expected_status.tweak('iota', status='  ', wc_rev=3)
  expected_output = svntest.wc.State(wc_dir, {
    'iota': Item(verb='Adding'),
    'A/mu': Item(verb='Replacing'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Verify the content of 'mu'
  svntest.actions.run_and_verify_svn(None, text_r1, [], 'cat', mu_path)

  # situation: no local modifications, mu has its original content again.

  # revert 'mu' locally, shouldn't change a thing.
  svntest.actions.run_and_verify_svn(None, [], [], "revert",
                                     mu_path)

  # Verify the content of 'mu'
  svntest.actions.run_and_verify_svn(None, text_r1, [], 'cat', mu_path)

#----------------------------------------------------------------------
# Test for issue #2804.
@Issue(2804)
def status_of_missing_dir_after_revert(sbox):
  "status after schedule-delete, revert, and local rm"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  A_D_G_path = os.path.join(wc_dir, "A", "D", "G")

  svntest.actions.run_and_verify_svn(None, None, [], "rm", A_D_G_path)
  expected_output = re.escape("Reverted '" + A_D_G_path + "'")
  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert",
                                     A_D_G_path)

  deletes = [
     "D       " + os.path.join(A_D_G_path, "pi") + "\n",
     "D       " + os.path.join(A_D_G_path, "rho") + "\n",
     "D       " + os.path.join(A_D_G_path, "tau") + "\n"
  ]
  expected_output = svntest.verify.UnorderedOutput(deletes)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     "status", wc_dir)

  svntest.main.safe_rmtree(A_D_G_path)

  expected_output = ["!       " + A_D_G_path + "\n"]

  if svntest.main.wc_is_singledb(wc_dir):
    expected_output.extend(deletes)

  expected_output = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected_output, [], "status",
                                     wc_dir)

  # When using single-db, we can get back to the virginal state.
  if svntest.main.wc_is_singledb(wc_dir):
    svntest.actions.run_and_verify_svn(None, None, [], "revert",
                                       "-R", A_D_G_path)

    expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
    svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
# Test for issue #2804 with replaced directory
@Issue(2804)
def status_of_missing_dir_after_revert_replaced_with_history_dir(sbox):
  "status after replace+, revert, and local rm"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # delete A/D/G and commit
  G_path = os.path.join(wc_dir, "A", "D", "G")
  svntest.actions.run_and_verify_svn(None, None, [], "rm", G_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/D/G', 'A/D/G/rho', 'A/D/G/pi', 'A/D/G/tau')
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G': Item(verb='Deleting'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # copy A/D/G from A/B/E and commit
  E_path = os.path.join(wc_dir, "A", "B", "E")
  svntest.actions.run_and_verify_svn(None, None, [], "cp", E_path, G_path)
  expected_status.add({
    'A/D/G' : Item(status='  ', wc_rev='3'),
    'A/D/G/alpha' : Item(status='  ', wc_rev='3'),
    'A/D/G/beta' : Item(status='  ', wc_rev='3')
    })
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G': Item(verb='Adding'),
    })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # update the working copy
  svntest.main.run_svn(None, 'up', wc_dir)

  # now rollback to r1, thereby reinstating the old 'G'
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G': Item(status='R '),
    'A/D/G/rho': Item(status='A '),
    'A/D/G/pi': Item(status='A '),
    'A/D/G/tau': Item(status='A '),
    })
  expected_mergeinfo_output = svntest.wc.State(wc_dir, {
    '': Item(status=' U'),
    })
  expected_elision_output = svntest.wc.State(wc_dir, {
    '': Item(status=' U'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.tweak('A/D/G', status='R ', copied='+', wc_rev='-')
  expected_status.tweak('A/D/G/rho',
                        'A/D/G/pi',
                        'A/D/G/tau',
                        copied='+', wc_rev='-')
  expected_status.add({
    'A/D/G/alpha' : Item(status='D ', wc_rev='3'),
    'A/D/G/beta' : Item(status='D ', wc_rev='3'),
    })

  expected_skip = wc.State(wc_dir, { })
  expected_disk   = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_merge(wc_dir, '3', '1',
                                       sbox.repo_url, None,
                                       expected_output,
                                       expected_mergeinfo_output,
                                       expected_elision_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       dry_run = 0)

  # now test if the revert works ok
  revert_paths = [G_path] + [os.path.join(G_path, child)
                             for child in ['alpha', 'beta', 'pi', 'rho', 'tau']]

  expected_output = svntest.verify.UnorderedOutput([
    "Reverted '%s'\n" % path for path in revert_paths])

  svntest.actions.run_and_verify_svn(None, expected_output, [], "revert", "-R",
                                     G_path)

  svntest.actions.run_and_verify_svn(None, [], [],
                                     "status", wc_dir)

  svntest.main.safe_rmtree(G_path)

  if svntest.main.wc_is_singledb(wc_dir):
    expected_output = svntest.verify.UnorderedOutput(
      ["!       " + G_path + "\n",
       "!       " + os.path.join(G_path, "alpha") + "\n",
       "!       " + os.path.join(G_path, "beta") + "\n"])
  else:
    expected_output = svntest.verify.UnorderedOutput(
      ["!       " + G_path + "\n"])
  svntest.actions.run_and_verify_svn(None, expected_output, [], "status",
                                     wc_dir)

# Test for issue #2928.
@Issue(2928)
def revert_replaced_with_history_file_2(sbox):
  "reverted replace with history restores checksum"

  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Delete mu and replace it with a copy of iota
  svntest.main.run_svn(None, 'rm', mu_path)
  svntest.main.run_svn(None, 'cp', iota_path, mu_path)

  # Revert mu.
  svntest.main.run_svn(None, 'revert', mu_path)

  # If we make local mods to the reverted mu the commit will
  # fail if the checksum is incorrect.
  svntest.main.file_write(mu_path, "new text")
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu': Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='  ', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

def revert_tree_conflicts_in_updated_files(sbox):
  "revert tree conflicts in updated files"

  # See use cases 1-3 in notes/tree-conflicts/use-cases.txt for background.

  svntest.actions.build_greek_tree_conflicts(sbox)
  wc_dir = sbox.wc_dir
  G = os.path.join(wc_dir, 'A', 'D', 'G')
  G_pi  = os.path.join(G, 'pi');
  G_rho = os.path.join(G, 'rho');
  G_tau = os.path.join(G, 'tau');

  # Duplicate wc for tests
  wc_dir_2 =  sbox.add_wc_path('2')
  svntest.actions.duplicate_dir(wc_dir, wc_dir_2)
  G2 = os.path.join(wc_dir_2, 'A', 'D', 'G')
  G2_pi  = os.path.join(G2, 'pi');
  G2_rho = os.path.join(G2, 'rho');
  G2_tau = os.path.join(G2, 'tau');

  # Expectations
  expected_output = svntest.verify.UnorderedOutput(
   ["Reverted '%s'\n" % G_pi,
    "Reverted '%s'\n" % G_rho,
    "Reverted '%s'\n" % G_tau,
    ])

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/D/G/pi',  status='  ')
  expected_status.remove('A/D/G/rho')
  expected_status.remove('A/D/G/tau')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/G/rho')
  expected_disk.tweak('A/D/G/pi',
                      contents="This is the file 'pi'.\nIncoming edit.\n")
  expected_disk.remove('A/D/G/tau')

  # Revert individually in wc
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', G_pi, G_rho, G_tau)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  svntest.actions.verify_disk(wc_dir, expected_disk)

  # Expectations
  expected_output = svntest.verify.UnorderedOutput(
   ["Reverted '%s'\n" % G2_pi,
    "Reverted '%s'\n" % G2_rho,
    "Reverted '%s'\n" % G2_tau,
    ])

  expected_status.wc_dir = wc_dir_2

  # Revert recursively in wc 2
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '-R', G2)
  svntest.actions.run_and_verify_status(wc_dir_2, expected_status)
  svntest.actions.verify_disk(wc_dir_2, expected_disk)

def revert_add_over_not_present_dir(sbox):
  "reverting an add over not present directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  main.run_svn(None, 'rm', os.path.join(wc_dir, 'A/C'))
  main.run_svn(None, 'ci', wc_dir, '-m', 'Deleted dir')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.remove('A/C')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  main.run_svn(None, 'mkdir', os.path.join(wc_dir, 'A/C'))

  # This failed in some WC-NG intermediate format (r927318-r958992).
  main.run_svn(None, 'revert', os.path.join(wc_dir, 'A/C'))

  svntest.actions.run_and_verify_status(wc_dir, expected_status)


def revert_added_tree(sbox):
  "revert an added tree fails"

  sbox.build()
  wc_dir = sbox.wc_dir
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', sbox.ospath('X'), sbox.ospath('X/Y'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X'   : Item(status='A ', wc_rev=0),
    'X/Y' : Item(status='A ', wc_rev=0),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Revert is non-recursive and fails, status is unchanged
  expected_error = '.*Try \'svn revert --depth infinity\'.*'
  svntest.actions.run_and_verify_svn(None, None, expected_error,
                                     'revert', sbox.ospath('X'))
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


@Issue(3834)
def revert_child_of_copy(sbox):
  "revert a child of a copied directory"

  sbox.build()
  wc_dir = sbox.wc_dir
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp',
                                     sbox.ospath('A/B/E'),
                                     sbox.ospath('A/B/E2'))


  svntest.main.file_append(sbox.ospath('A/B/E2/beta'), 'extra text\n')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/E2'       : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E2/alpha' : Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E2/beta'  : Item(status='M ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # First revert removes text change, child is still copied
  expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E2/beta')]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', sbox.ospath('A/B/E2/beta'))
  expected_status.tweak('A/B/E2/beta', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Second revert of child does nothing, child is still copied
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', sbox.ospath('A/B/E2/beta'))
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3783)
def revert_non_recusive_after_delete(sbox):
  "non-recursive revert after delete"

  sbox.build(read_only=True)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None, None, [], 'rm', sbox.ospath('A/B'))
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B', 'A/B/E', 'A/B/E/alpha', 'A/B/E/beta', 'A/B/F',
                        'A/B/lambda', status='D ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # This appears to work but gets the op-depth wrong
  expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B')]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', sbox.ospath('A/B'))
  expected_status.tweak('A/B', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', sbox.ospath('A/B/E'))
  expected_status.tweak('A/B/E', status='R ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Since the op-depth was wrong A/B/E erroneously remains deleted
  expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E')]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', sbox.ospath('A/B/E'))
  expected_status.tweak('A/B/E', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

def revert_permissions_only(sbox):
  "permission-only reverts"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Helpers pinched/adapted from lock_tests.py.  Put them somewhere common?
  def check_writability(path, writable):
    bits = stat.S_IWGRP | stat.S_IWOTH | stat.S_IWRITE
    mode = os.stat(path)[0]
    if bool(mode & bits) != writable:
      raise svntest.Failure("path '%s' is unexpectedly %s (mode %o)"
                            % (path, ["writable", "read-only"][writable], mode))

  def is_writable(path):
    "Raise if PATH is not writable."
    check_writability(path, True)

  def is_readonly(path):
    "Raise if PATH is not readonly."
    check_writability(path, False)

  def check_executability(path, executable):
    bits = stat.S_IXGRP | stat.S_IXOTH | stat.S_IEXEC
    mode = os.stat(path)[0]
    if bool(mode & bits) != executable:
      raise svntest.Failure("path '%s' is unexpectedly %s (mode %o)"
                            % (path,
                               ["executable", "non-executable"][executable],
                               mode))

  def is_executable(path):
    "Raise if PATH is not executable."
    check_executability(path, True)

  def is_non_executable(path):
    "Raise if PATH is executable."
    check_executability(path, False)


  os.chmod(sbox.ospath('A/B/E/alpha'), 0444);  # read-only
  is_readonly(sbox.ospath('A/B/E/alpha'))
  expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E/alpha')]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', sbox.ospath('A/B/E/alpha'))
  is_writable(sbox.ospath('A/B/E/alpha'))

  if svntest.main.is_posix_os():
    os.chmod(sbox.ospath('A/B/E/beta'), 0777);   # executable
    is_executable(sbox.ospath('A/B/E/beta'))
    expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E/beta')]
    svntest.actions.run_and_verify_svn(None, expected_output, [],
                                       'revert', sbox.ospath('A/B/E/beta'))
    is_non_executable(sbox.ospath('A/B/E/beta'))

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:needs-lock', '1',
                                     sbox.ospath('A/B/E/alpha'))
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:executable', '1',
                                     sbox.ospath('A/B/E/beta'))

  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha': Item(verb='Sending'),
    'A/B/E/beta':  Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', wc_rev='2')
  expected_status.tweak('A/B/E/beta',  wc_rev='2')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  os.chmod(sbox.ospath('A/B/E/alpha'), 0666);  # not read-only
  is_writable(sbox.ospath('A/B/E/alpha'))
  expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E/alpha')]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', sbox.ospath('A/B/E/alpha'))
  is_readonly(sbox.ospath('A/B/E/alpha'))

  if svntest.main.is_posix_os():
    os.chmod(sbox.ospath('A/B/E/beta'), 0666);   # not executable
    is_non_executable(sbox.ospath('A/B/E/beta'))
    expected_output = ["Reverted '%s'\n" % sbox.ospath('A/B/E/beta')]
    svntest.actions.run_and_verify_svn(None, expected_output, [],
                                       'revert', sbox.ospath('A/B/E/beta'))
    is_executable(sbox.ospath('A/B/E/beta'))

@XFail()
@Issue(3851)
def revert_copy_depth_files(sbox):
  "revert a copy with depth=files"

  sbox.build(read_only=True)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy',
                                     sbox.ospath('A/B/E'),
                                     sbox.ospath('A/B/E2'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/E2'       : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/E2/alpha' : Item(status='  ', copied='+', wc_rev='-'),
    'A/B/E2/beta'  : Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.verify.UnorderedOutput([
    "Reverted '%s'\n" % sbox.ospath(path) for path in ['A/B/E2',
                                                       'A/B/E2/alpha',
                                                       'A/B/E2/beta']])

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '--depth', 'files',
                                     sbox.ospath('A/B/E2'))

  expected_status.remove('A/B/E2', 'A/B/E2/alpha', 'A/B/E2/beta')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@XFail()
@Issue(3851)
def revert_nested_add_depth_immediates(sbox):
  "revert a nested add with depth=immediates"

  sbox.build(read_only=True)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', '--parents', sbox.ospath('A/X/Y'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/X'       : Item(status='A ', wc_rev='0'),
    'A/X/Y'     : Item(status='A ', wc_rev='0'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.verify.UnorderedOutput([
    "Reverted '%s'\n" % sbox.ospath(path) for path in ['A/X', 'A/X/Y']])

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', '--depth', 'immediates',
                                     sbox.ospath('A/X'))

  expected_status.remove('A/X', 'A/X/Y')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

def create_superflous_actual_node(sbox):
  "create a superfluous actual node"

  sbox.build()
  wc_dir = sbox.wc_dir

  svntest.main.file_append(sbox.ospath('A/B/E/alpha'), 'their text\n')
  sbox.simple_commit()
  sbox.simple_update()

  # Create a NODES row with op-depth>0
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy', '-r', '1',
                                     sbox.repo_url + '/A/B/E/alpha',
                                     sbox.ospath('alpha'))

  # Merge to create an ACTUAL with a conflict
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'alpha' : Item(status='A ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)
  svntest.main.file_append(sbox.ospath('alpha'), 'my text\n')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '--accept', 'postpone',
                                     '^/A/B/E/alpha', sbox.ospath('alpha'))
  expected_status.tweak('alpha', status='CM', entry_status='A ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Clear merge property and remove conflict files
  sbox.simple_propdel('svn:mergeinfo', 'alpha')
  os.remove(sbox.ospath('alpha.merge-left.r1'))
  os.remove(sbox.ospath('alpha.merge-right.r2'))
  os.remove(sbox.ospath('alpha.working'))

  expected_status.tweak('alpha', status='A ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3859)
def revert_empty_actual(sbox):
  "revert with superfluous actual node"

  create_superflous_actual_node(sbox)
  wc_dir = sbox.wc_dir

  # Non-recursive code path works
  svntest.actions.run_and_verify_svn(None,
                                     ["Reverted '%s'\n" % sbox.ospath('alpha')],
                                     [],
                                     'revert', sbox.ospath('alpha'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3859)
def revert_empty_actual_recursive(sbox):
  "recusive revert with superfluous actual node"

  create_superflous_actual_node(sbox)
  wc_dir = sbox.wc_dir

  # Recursive code path fails, the superfluous actual node suppresses the
  # notification
  svntest.actions.run_and_verify_svn(None,
                                     ["Reverted '%s'\n" % sbox.ospath('alpha')],
                                     [],
                                     'revert', '-R', sbox.ospath('alpha'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3879)
def revert_tree_conflicts_with_replacements(sbox):
  "revert tree conflicts with replacements"

  sbox.build()
  wc_dir = sbox.wc_dir
  wc = sbox.ospath

  # Use case 1: local replace, incoming replace
  # A/mu
  # A/D/H --> A/D/H/chi, A/D/H/{loc,inc}_psi

  # Use case 2: local edit, incoming replace
  # A/D/gamma
  # A/D/G --> A/D/G/pi, A/D/G/inc_rho

  # Use case 3: local replace, incoming edit
  # A/B/lambda
  # A/B/E --> A/B/E/alpha, A/B/E/loc_beta

  # Case 1: incoming replacements
  sbox.simple_rm('A/mu', 'A/D/H')
  file_write(wc('A/mu'), "A fresh file.\n")
  os.mkdir(wc('A/D/H'))
  file_write(wc('A/D/H/chi'), "A fresh file.\n")
  file_write(wc('A/D/H/inc_psi'), "A fresh file.\n")
  sbox.simple_add('A/mu', 'A/D/H')

  # Case 2: incoming replacements
  sbox.simple_rm('A/D/gamma', 'A/D/G')
  file_write(wc('A/D/gamma'), "A fresh file.\n")
  os.mkdir(wc('A/D/G'))
  file_write(wc('A/D/G/pi'), "A fresh file.\n")
  file_write(wc('A/D/G/inc_rho'), "A fresh file.\n")
  sbox.simple_add('A/D/gamma','A/D/G')

  # Case 3: incoming edits
  file_append(wc('A/B/lambda'), "Incoming!\n")
  file_write(wc('A/B/E/alpha'), "Incoming!.\n")

  # Commit and roll back to r1.
  sbox.simple_commit()
  run_svn(None, 'up', wc_dir, '-r1', '-q')

  # Case 1: local replacements
  sbox.simple_rm('A/mu', 'A/D/H')
  file_write(wc('A/mu'), "A fresh file.\n")
  os.mkdir(wc('A/D/H'))
  file_write(wc('A/D/H/chi'), "A fresh local file.\n")
  file_write(wc('A/D/H/loc_psi'), "A fresh local file.\n")
  sbox.simple_add('A/mu', 'A/D/H')

  # Case 2: local edits
  file_append(wc('A/D/gamma'), "Local change.\n")
  file_append(wc('A/D/G/pi'), "Local change.\n")

  # Case 3: local replacements
  sbox.simple_rm('A/B/lambda', 'A/B/E')
  file_write(wc('A/B/lambda'), "A fresh local file.\n")
  os.mkdir(wc('A/B/E'))
  file_write(wc('A/B/E/alpha'), "A fresh local file.\n")
  file_write(wc('A/B/E/loc_beta'), "A fresh local file.\n")
  sbox.simple_add('A/B/lambda', 'A/B/E')

  # Update and check tree conflict status.
  run_svn(None, 'up', wc_dir)
  expected_status = svntest.wc.State(wc_dir, {
    ''                : Item(status='  ', wc_rev=2),
    'A'               : Item(status='  ', wc_rev=2),
    'A/B'             : Item(status='  ', wc_rev=2),
    'A/B/E'           : Item(status='R ', wc_rev=2, treeconflict='C'),
    'A/B/E/alpha'     : Item(status='A ', wc_rev='-'),
    'A/B/E/beta'      : Item(status='D ', wc_rev=2),
    'A/B/E/loc_beta'  : Item(status='A ', wc_rev='-'),
    'A/B/F'           : Item(status='  ', wc_rev=2),
    'A/B/lambda'      : Item(status='R ', wc_rev=2, treeconflict='C'),
    'A/C'             : Item(status='  ', wc_rev=2),
    'A/D'             : Item(status='  ', wc_rev=2),
    'A/D/G'           : Item(status='R ', wc_rev='-', copied='+',
                             treeconflict='C'),
    'A/D/G/inc_rho'   : Item(status='D ', wc_rev=2),
    'A/D/G/pi'        : Item(status='M ', wc_rev='-', copied='+'),
    'A/D/G/rho'       : Item(status='  ', wc_rev='-', copied='+'),
    'A/D/G/tau'       : Item(status='  ', wc_rev='-', copied='+'),
    'A/D/H'           : Item(status='R ', wc_rev=2, treeconflict='C'),
    'A/D/H/chi'       : Item(status='A ', wc_rev='-'),
    'A/D/H/inc_psi'   : Item(status='D ', wc_rev=2),
    'A/D/H/loc_psi'   : Item(status='A ', wc_rev='-'),
    'A/D/gamma'       : Item(status='R ', wc_rev='-', copied='+',
                             treeconflict='C'),
    'A/mu'            : Item(status='R ', wc_rev=2, treeconflict='C'),
    'iota'            : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_unquiet_status(wc_dir, expected_status)

  def cd_and_status_u(dir_target):
    was_cwd = os.getcwd()
    os.chdir(os.path.abspath(wc(dir_target)))
    run_svn(None, 'status', '-u')
    os.chdir(was_cwd)

  cd_and_status_u('A')
  cd_and_status_u('A/D')

  # Until r1102143, the following 'status -u' commands failed with "svn:
  # E165004: Two top-level reports with no target".
  cd_and_status_u('A/D/G')
  cd_and_status_u('A/D/H')

  # Revert everything (i.e., accept "theirs-full").
  svntest.actions.run_and_verify_revert([
    wc('A/B/E'),
    wc('A/B/E/alpha'),   # incoming
    wc('A/B/E/beta'),
    wc('A/B/E/loc_beta'),
    wc('A/B/lambda'),
    wc('A/D/G'),
    wc('A/D/G/pi'),
    wc('A/D/G/inc_rho'), # incoming
    wc('A/D/G/rho'),
    wc('A/D/G/tau'),
    wc('A/D/H'),
    wc('A/D/H/chi'),
    wc('A/D/H/inc_psi'), # incoming
    wc('A/D/H/loc_psi'),
    wc('A/D/gamma'),
    wc('A/mu'),
    wc('A/B/E/alpha'),
    ], '-R', wc_dir)

  # Remove a few unversioned files that revert left behind.
  os.remove(wc('A/B/E/loc_beta'))
  os.remove(wc('A/D/H/loc_psi'))

  # The update operation should have put all incoming items in place.
  expected_status = svntest.wc.State(wc_dir, {
    ''                : Item(status='  ', wc_rev=2),
    'A'               : Item(status='  ', wc_rev=2),
    'A/B'             : Item(status='  ', wc_rev=2),
    'A/B/E'           : Item(status='  ', wc_rev=2),
    'A/B/E/alpha'     : Item(status='  ', wc_rev=2),
    'A/B/E/beta'      : Item(status='  ', wc_rev=2),
    'A/B/F'           : Item(status='  ', wc_rev=2),
    'A/B/lambda'      : Item(status='  ', wc_rev=2),
    'A/C'             : Item(status='  ', wc_rev=2),
    'A/D'             : Item(status='  ', wc_rev=2),
    'A/D/G'           : Item(status='  ', wc_rev=2),
    'A/D/G/inc_rho'   : Item(status='  ', wc_rev=2),
    'A/D/G/pi'        : Item(status='  ', wc_rev=2),
    'A/D/H'           : Item(status='  ', wc_rev=2),
    'A/D/H/chi'       : Item(status='  ', wc_rev=2),
    'A/D/H/inc_psi'   : Item(status='  ', wc_rev=2),
    'A/D/gamma'       : Item(status='  ', wc_rev=2),
    'A/mu'            : Item(status='  ', wc_rev=2),
    'iota'            : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_unquiet_status(wc_dir, expected_status)

def create_no_text_change_conflict(sbox):
  "create conflict with no text change"

  sbox.build()
  wc_dir = sbox.wc_dir

  shutil.copyfile(sbox.ospath('A/B/E/alpha'), sbox.ospath('A/B/E/alpha-copy'))
  svntest.main.file_append(sbox.ospath('A/B/E/alpha'), 'their text\n')
  sbox.simple_commit()
  sbox.simple_update()

  # Update to create a conflict
  svntest.main.file_append(sbox.ospath('A/B/E/alpha'), 'my text\n')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r1', '--accept', 'postpone',
                                     wc_dir)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', status='C ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Reset the text with the file still marked as a conflict
  os.remove(sbox.ospath('A/B/E/alpha'))
  shutil.move(sbox.ospath('A/B/E/alpha-copy'), sbox.ospath('A/B/E/alpha'))

@Issue(3859)
def revert_no_text_change_conflict(sbox):
  "revert conflict with no text change"

  create_no_text_change_conflict(sbox)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None,
                                     ["Reverted '%s'\n"
                                      % sbox.ospath('A/B/E/alpha')],
                                     [],
                                     'revert', sbox.ospath('A/B/E/alpha'))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3859)
def revert_no_text_change_conflict_recursive(sbox):
  "revert -R conflict with no text change"

  create_no_text_change_conflict(sbox)
  wc_dir = sbox.wc_dir

  svntest.actions.run_and_verify_svn(None,
                                     ["Reverted '%s'\n"
                                      % sbox.ospath('A/B/E/alpha')],
                                     [],
                                     'revert', '-R', wc_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

@Issue(3938)
def revert_with_unversioned_targets(sbox):
  "revert with unversioned targets"

  sbox.build()
  wc_dir = sbox.wc_dir

  chi_path = sbox.ospath('A/D/H/chi')
  delta_path = sbox.ospath('A/D/H/delta')
  psi_path = sbox.ospath('A/D/H/psi')

  chi_contents = "modified chi\n"
  delta_contents = "This is the unversioned file 'delta'.\n"
  psi_contents = "modified psi\n"

  # touch delta
  open(delta_path, 'w').write(delta_contents)

  # modify chi psi
  open(chi_path, 'w').write(chi_contents)
  open(psi_path, 'w').write(psi_contents)

  # revert
  expected_output = svntest.verify.UnorderedOutput([
    "Reverted '%s'\n" % sbox.ospath('A/D/H/chi'),
    "Skipped '%s'\n" % sbox.ospath('A/D/H/delta'),
    "Reverted '%s'\n" % sbox.ospath('A/D/H/psi'),
  ])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'revert', chi_path, delta_path, psi_path)

  # verify status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/H/delta': Item(status='? '),
    })
  svntest.actions.run_and_verify_unquiet_status(wc_dir, expected_status)

  # verify disk
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/D/H/delta': Item(delta_contents),
  })
  actual_disk = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk, expected_disk.old_tree())

@Issue(4168)
def revert_obstructing_wc(sbox):
  "revert with an obstructing working copy"
  
  sbox.build(create_wc=False, read_only=True)
  wc_dir = sbox.wc_dir
  
  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.wc.State(wc_dir, {})  
  
  # Checkout wc as depth empty
  svntest.actions.run_and_verify_checkout(sbox.repo_url, wc_dir,
                                          expected_output, expected_disk,
                                          None, None, None, None,
                                          '--depth', 'empty')

  # And create an obstructing working copy as A
  svntest.actions.run_and_verify_checkout(sbox.repo_url, wc_dir + '/A',
                                          expected_output, expected_disk,
                                          None, None, None, None,
                                          '--depth', 'empty')

  # Now try to fetch the entire wc, which will find an obstruction
  expected_output = svntest.wc.State(wc_dir, {
    'A'     : Item(verb='Skipped'),
    'iota'  : Item(status='A '),
  })
  expected_status = svntest.wc.State(wc_dir, {
    ''      : Item(status='  ', wc_rev='1'),
    'iota'  : Item(status='  ', wc_rev='1'),
    # A is not versioned but exists
  })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, expected_status,
                                        None, None, None,
                                        None, None, None,
                                        wc_dir, '--set-depth', 'infinity')

  # Revert should do nothing (no local changes), and report the obstruction
  # (reporting the obstruction is nice for debuging, but not really required
  #  in this specific case, as the node was not modified)
  svntest.actions.run_and_verify_svn(None, "Skipped '.*A' -- .*obstruct.*", [],
                                     'revert', '-R', wc_dir)


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              revert_from_wc_root,
              revert_reexpand_keyword,
              revert_replaced_file_without_props,
              revert_moved_file,
              revert_wc_to_wc_replace_with_props,
              revert_file_merge_replace_with_history,
              revert_repos_to_wc_replace_with_props,
              revert_after_second_replace,
              revert_after_manual_conflict_resolution__text,
              revert_after_manual_conflict_resolution__prop,
              revert_propset__dir,
              revert_propset__file,
              revert_propdel__dir,
              revert_propdel__file,
              revert_replaced_with_history_file_1,
              status_of_missing_dir_after_revert,
              status_of_missing_dir_after_revert_replaced_with_history_dir,
              revert_replaced_with_history_file_2,
              revert_tree_conflicts_in_updated_files,
              revert_add_over_not_present_dir,
              revert_added_tree,
              revert_child_of_copy,
              revert_non_recusive_after_delete,
              revert_permissions_only,
              revert_copy_depth_files,
              revert_nested_add_depth_immediates,
              revert_empty_actual,
              revert_tree_conflicts_with_replacements,
              revert_empty_actual_recursive,
              revert_no_text_change_conflict,
              revert_no_text_change_conflict_recursive,
              revert_with_unversioned_targets,
              revert_obstructing_wc
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
