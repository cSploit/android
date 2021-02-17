#!/usr/bin/env python
#
#  prop_tests.py:  testing versioned properties
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
import sys, re, os, stat, subprocess

# Our testing module
import svntest

from svntest.main import SVN_PROP_MERGEINFO

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem

def is_non_posix_and_non_windows_os():
  """lambda function to skip revprop_change test"""
  return (not svntest.main.is_posix_os()) and sys.platform != 'win32'

######################################################################
# Tests

#----------------------------------------------------------------------

def make_local_props(sbox):
  "write/read props in wc only (ps, pl, pdel, pe)"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add properties to one file and one directory
  sbox.simple_propset('blue', 'azul', 'A/mu')
  sbox.simple_propset('green', 'verde', 'A/mu')
  sbox.simple_propset('editme', 'the foo fighters', 'A/mu')
  sbox.simple_propset('red', 'rojo', 'A/D/G')
  sbox.simple_propset('yellow', 'amarillo', 'A/D/G')

  # Make sure they show up as local mods in status
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status=' M')
  expected_status.tweak('A/D/G', status=' M')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Remove one property
  sbox.simple_propdel('yellow', 'A/D/G')

  svntest.main.use_editor('foo_to_bar')
  # Edit one property
  svntest.main.run_svn(None, 'propedit', 'editme',
                       os.path.join(wc_dir, 'A', 'mu'))

  # What we expect the disk tree to look like:
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', props={'blue' : 'azul', 'green' : 'verde',
                                     'editme' : 'the bar fighters'})
  expected_disk.tweak('A/D/G', props={'red' : 'rojo'})

  # Read the real disk tree.  Notice we are passing the (normally
  # disabled) "load props" flag to this routine.  This will run 'svn
  # proplist' on every item in the working copy!
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)

  # Compare actual vs. expected disk trees.
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Edit without actually changing the property
  svntest.main.use_editor('identity')
  svntest.actions.run_and_verify_svn(None,
                                     "No changes to property 'editme' on '.*'",
                                     [],
                                     'propedit', 'editme',
                                     os.path.join(wc_dir, 'A', 'mu'))



#----------------------------------------------------------------------

def commit_props(sbox):
  "commit properties"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to a file and a directory
  sbox.simple_propset('blue', 'azul', 'A/mu')
  sbox.simple_propset('red', 'rojo', 'A/D/H')

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    'A/D/H' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', 'A/D/H', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)



#----------------------------------------------------------------------

@Issue(3951)
def update_props(sbox):
  "receive properties via update"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Add a property to a file and a directory
  sbox.simple_propset('blue', 'azul', 'A/mu')
  sbox.simple_propset('red', 'rojo', 'A/D/H')

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    'A/D/H' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', 'A/D/H', wc_rev=2, status='  ')

  # Commit property mods
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Add more properties
  sbox.simple_propset('blue2', 'azul2', 'A/mu')
  sbox.simple_propset('red2', 'rojo2', 'A/D/H')
  expected_status.tweak('A/mu', 'A/D/H', wc_rev=3, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Create expected output tree for an update of the wc_backup.
  expected_output = svntest.wc.State(wc_backup, {
    'A/mu' : Item(status=' U'),
    'A/D/H' : Item(status=' U'),
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', props={'blue' : 'azul'})
  expected_disk.tweak('A/D/H', props={'red' : 'rojo'})

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.tweak('A/mu', 'A/D/H', status='  ')

  # Do the update and check the results in three ways... INCLUDING PROPS
  # This adds properties to nodes that have none
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1,
                                        '-r', '2', wc_backup)

  # This adds properties to nodes that have properties
  expected_status.tweak(wc_rev=3)
  expected_disk.tweak('A/mu', props={'blue'  : 'azul',
                                     'blue2' : 'azul2'})
  expected_disk.tweak('A/D/H', props={'red'  : 'rojo',
                                      'red2' : 'rojo2'})
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1,
                                        '-r', '3', wc_backup)


#----------------------------------------------------------------------

def downdate_props(sbox):
  "receive property changes as part of a downdate"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = sbox.ospath('A/mu')

  # Add a property to a file
  sbox.simple_propset('cash-sound', 'cha-ching!', 'iota')

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Make some mod (something to commit)
  svntest.main.file_append(mu_path, "some mod")

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2, status='  ')
  expected_status.tweak('A/mu', wc_rev=3, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Create expected output tree for an update.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(status=' U'),
    'A/mu' : Item(status='U '),
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Do the update and check the results in three ways... INCLUDING PROPS
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1,
                                        '-r', '1', wc_dir)

#----------------------------------------------------------------------

def remove_props(sbox):
  "commit the removal of props"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to a file
  sbox.simple_propset('cash-sound', 'cha-ching!', 'iota')

  # Commit the file
  sbox.simple_commit('iota')

  # Now, remove the property
  sbox.simple_propdel('cash-sound', 'iota')

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=3, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

def update_conflict_props(sbox):
  "update with conflicting props"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to a file and a directory
  mu_path = sbox.ospath('A/mu')
  sbox.simple_propset('cash-sound', 'cha-ching!', 'A/mu')
  A_path = sbox.ospath('A')
  sbox.simple_propset('foo', 'bar', 'A')

  # Commit the file and directory
  sbox.simple_commit()

  # Update to rev 1
  svntest.main.run_svn(None, 'up', '-r', '1', wc_dir)

  # Add conflicting properties
  sbox.simple_propset('cash-sound', 'beep!', 'A/mu')
  sbox.simple_propset('foo', 'baz', 'A')

  # Create expected output tree for an update of the wc_backup.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(status=' C'),
    'A' : Item(status=' C'),
    })

  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', props={'cash-sound' : 'beep!'})
  expected_disk.tweak('A', props={'foo' : 'baz'})

  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/mu', 'A', status=' C')

  extra_files = ['mu.*\.prej', 'dir_conflicts.*\.prej']
  # Do the update and check the results in three ways... INCLUDING PROPS
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files,
                                        None, None, 1)

  if len(extra_files) != 0:
    print("didn't get expected conflict files")
    raise svntest.verify.SVNUnexpectedOutput

  # Resolve the conflicts
  svntest.actions.run_and_verify_resolved([mu_path, A_path])

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/mu', 'A', status=' M')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
@Issue(2608)
def commit_conflict_dirprops(sbox):
  "commit with conflicting dirprops"

  # Issue #2608: failure to see conflicting dirprops on root of
  # repository.

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_propset('foo', 'bar', '')

  # Commit the file and directory
  sbox.simple_commit()

  # Update to rev 1
  svntest.main.run_svn(None,
                       'up', '-r', '1', wc_dir)

  # Add conflicting properties
  sbox.simple_propset('foo', 'eek', '')

  svntest.actions.run_and_verify_commit(wc_dir, None, None,
                                        "[oO]ut[- ]of[- ]date",
                                        wc_dir)

#----------------------------------------------------------------------

# Issue #742: we used to screw up when committing a file replacement
# that also had properties.  It was fixed by teaching
# svn_wc_props_modified_p and svn_wc_transmit_prop_deltas to *ignore*
# leftover base-props when a file is scheduled for replacement.  (When
# we svn_wc_add a file, it starts life with no working props.)
@Issue(742)
def commit_replacement_props(sbox):
  "props work when committing a replacement"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to two files
  iota_path = sbox.ospath('iota')
  lambda_path = sbox.ospath('A/B/lambda')
  sbox.simple_propset('cash-sound', 'cha-ching!', 'iota')
  sbox.simple_propset('boson', 'W', 'A/B/lambda')

  # Commit (### someday use run_and_verify_commit for better coverage)
  sbox.simple_commit()

  # Schedule both files for deletion
  sbox.simple_rm('iota', 'A/B/lambda')

  # Now recreate the files, and schedule them for addition.
  # Poof, the 'new' files don't have any properties at birth.
  svntest.main.file_append(iota_path, 'iota TNG')
  svntest.main.file_append(lambda_path, 'lambda TNG')
  sbox.simple_add('iota', 'A/B/lambda')

  # Sanity check:  the two files should be scheduled for (R)eplacement.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2, status='R ')
  expected_status.tweak('A/B/lambda', wc_rev=2, status='R ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now add a property to lambda.  Iota still doesn't have any.
  sbox.simple_propset('capacitor', 'flux', 'A/B/lambda')

  # Commit, with careful output checking.  We're actually going to
  # scan the working copy for props after the commit.

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Replacing'),
    'A/B/lambda' : Item(verb='Replacing'),
    })

  # Expected status tree:  lambda has one prop, iota doesn't.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=3)
  expected_status.tweak('A/B/lambda', wc_rev=3, status='  ')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------

def revert_replacement_props(sbox):
  "props work when reverting a replacement"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to two files
  iota_path = sbox.ospath('iota')
  lambda_path = sbox.ospath('A/B/lambda')
  sbox.simple_propset('cash-sound', 'cha-ching!', 'iota')
  sbox.simple_propset('boson', 'W', 'A/B/lambda')

  # Commit rev 2. (### someday use run_and_verify_commit for better coverage)
  sbox.simple_commit()

  # Schedule both files for deletion
  sbox.simple_rm('iota', 'A/B/lambda')

  # Now recreate the files, and schedule them for addition.
  # Poof, the 'new' files don't have any properties at birth.
  svntest.main.file_append(iota_path, 'iota TNG')
  svntest.main.file_append(lambda_path, 'lambda TNG')
  sbox.simple_add('iota', 'A/B/lambda')

  # Sanity check:  the two files should be scheduled for (R)eplacement.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2, status='R ')
  expected_status.tweak('A/B/lambda', wc_rev=2, status='R ')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now add a property to lambda.  Iota still doesn't have any.
  sbox.simple_propset('capacitor', 'flux', 'A/B/lambda')

  # Now revert both files.
  sbox.simple_revert('iota', 'A/B/lambda')

  # Do an update; even though the update is really a no-op,
  # run_and_verify_update has the nice feature of scanning disk as
  # well as running status.  We want to verify that we truly have a
  # *pristine* revision 2 tree, with the original rev 2 props, and no
  # local mods at all.

  expected_output = svntest.wc.State(wc_dir, {
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('iota', status='  ')
  expected_status.tweak('A/B/lambda', status='  ')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', props={'cash-sound' : 'cha-ching!'})
  expected_disk.tweak('A/B/lambda', props={'boson' : 'W'})

  # scan disk for props too.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        1)

#----------------------------------------------------------------------
@Issues(920,2065)
def inappropriate_props(sbox):
  "try to set inappropriate props"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = sbox.ospath('A')
  E_path = sbox.ospath('A/B/E')
  iota_path = sbox.ospath('iota')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # These should produce an error
  svntest.actions.run_and_verify_svn('Illegal target',
                                     None, svntest.verify.AnyOutput,
                                     'propset', 'svn:executable', 'on', A_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:keywords', 'LastChangedDate',
                                     A_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:eol-style', 'native', A_path)

  svntest.actions.run_and_verify_svn('Invalid svn:eol-style', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:eol-style', 'invalid value',
                                     os.path.join(A_path, 'mu'))

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:mime-type', 'image/png', A_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:ignore', '*.o', iota_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:externals',
                                     'foo http://host.com/repos', iota_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:author', 'socrates', iota_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:log', 'log message', iota_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:date', 'Tue Jan 19 04:14:07 2038',
                                     iota_path)

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput, 'propset',
                                     'svn:original-date',
                                     'Thu Jan  1 01:00:00 1970', iota_path)

  # Status unchanged
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Recursive setting of inappropriate dir prop should work on files
  svntest.actions.run_and_verify_svn(None, None, [], 'propset', '-R',
                                     'svn:executable', 'on', E_path)

  expected_status.tweak('A/B/E/alpha', 'A/B/E/beta', status=' M')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Issue #920. Don't allow setting of svn:eol-style on binary files or files
  # with inconsistent eol types.

  path = sbox.ospath('binary')
  svntest.main.file_append(path, "binary")
  sbox.simple_add('binary')

  sbox.simple_propset('svn:mime-type', 'application/octet-stream', 'binary')

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput,
                                     'propset', 'svn:eol-style',
                                     'CRLF', path)

  path = sbox.ospath('multi-eol')
  svntest.main.file_append(path, "line1\rline2\n")
  sbox.simple_add('multi-eol')

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput,
                                     'propset', 'svn:eol-style',
                                     'LF', path)

  path = sbox.ospath('backwards-eol')
  svntest.main.file_append(path, "line1\n\r")
  sbox.simple_add('backwards-eol')

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput,
                                     'propset', 'svn:eol-style',
                                     'native', path)

  path = sbox.ospath('incomplete-eol')
  svntest.main.file_append(path, "line1\r\n\r")
  sbox.simple_add('incomplete-eol')

  svntest.actions.run_and_verify_svn('Illegal target', None,
                                     svntest.verify.AnyOutput,
                                     'propset', 'svn:eol-style',
                                     'CR', path)

  # Issue #2065. Do allow setting of svn:eol-style on binary files or files
  # with inconsistent eol types if --force is passed.

  path = sbox.ospath('binary')
  svntest.main.file_append(path, "binary")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--force',
                                     'svn:eol-style', 'CRLF',
                                     path)

  path = sbox.ospath('multi-eol')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--force',
                                     'svn:eol-style', 'LF',
                                     path)

  path = sbox.ospath('backwards-eol')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--force',
                                     'svn:eol-style', 'native',
                                     path)

  path = sbox.ospath('incomplete-eol')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--force',
                                     'svn:eol-style', 'CR',
                                     path)

  # Prevent setting of svn:mergeinfo prop values that are...
  path = sbox.ospath('A/D')

  # ...grammatically incorrect
  svntest.actions.run_and_verify_svn('illegal grammar', None,
                                     "svn: E200020: Pathname not terminated by ':'\n",
                                     'propset', SVN_PROP_MERGEINFO, '/trunk',
                                     path)
  svntest.actions.run_and_verify_svn('illegal grammar', None,
                                     "svn: E200022: Invalid revision number found "
                                      "parsing 'one'\n",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/trunk:one', path)

  # ...contain overlapping revision ranges of differing inheritability.
  svntest.actions.run_and_verify_svn('overlapping ranges', None,
                                     "svn: E200020: Unable to parse overlapping "
                                     "revision ranges '9-20\\*' and "
                                     "'18-22' with different "
                                     "inheritance types\n",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/branch:5-7,9-20*,18-22', path)

  svntest.actions.run_and_verify_svn('overlapping ranges', None,
                                     "svn: E200020: Unable to parse overlapping "
                                     "revision ranges "
                                     "(('3' and '3\\*')|('3\\*' and '3')) "
                                     "with different "
                                     "inheritance types\n",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/branch:3,3*', path)

  # ...contain revision ranges with start revisions greater than or
  #    equal to end revisions.
  svntest.actions.run_and_verify_svn('range start >= range end', None,
                                     "svn: E200020: Unable to parse reversed "
                                      "revision range '20-5'\n",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/featureX:4,20-5', path)

  # ...contain paths mapped to empty revision ranges
  svntest.actions.run_and_verify_svn('empty ranges', None,
                                     "svn: E200020: Mergeinfo for '/trunk' maps to "
                                      "an empty revision range\n",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/trunk:', path)

  # ...contain non-inheritable ranges when the target is a file.
  svntest.actions.run_and_verify_svn('empty ranges', None,
                                     "svn: E200020: Cannot set non-inheritable "
                                     "mergeinfo on a non-directory*",
                                     'propset', SVN_PROP_MERGEINFO,
                                     '/A/D/H/psi:1*', iota_path)

#----------------------------------------------------------------------

# Issue #976.  When copying a file, do not determine svn:executable
# and svn:mime-type values as though the file is brand new, instead
# use the copied file's property values.
@Issue(976)
def copy_inherits_special_props(sbox):
  "file copies inherit (not re-derive) special props"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  orig_mime_type = 'image/fake_image'

  # Create two paths
  new_path1 = sbox.ospath('new_file1.bin')
  new_path2 = sbox.ospath('new_file2.bin')

  # Create the first path as a binary file.  To have svn treat the
  # file as binary, have a 0x00 in the file.
  svntest.main.file_append(new_path1, "binary file\000")
  sbox.simple_add('new_file1.bin')

  # Add initial svn:mime-type to the file
  sbox.simple_propset('svn:mime-type', orig_mime_type, 'new_file1.bin')

  # Set the svn:executable property on the file if this is a system
  # that can handle chmod, in which case svn will turn on the
  # executable bits on the file.  Then remove the executable bits
  # manually on the file and see the value of svn:executable in the
  # copied file.
  if os.name == 'posix':
    sbox.simple_propset('svn:executable', 'on', 'new_file1.bin')
    os.chmod(new_path1, 0644)

  # Commit the file
  sbox.simple_commit()

  # Copy the file
  svntest.main.run_svn(None, 'cp', new_path1, new_path2)

  # Check the svn:mime-type
  actual_exit, actual_stdout, actual_stderr = svntest.main.run_svn(
    None, 'pg', 'svn:mime-type', new_path2)

  expected_stdout = [orig_mime_type + '\n']
  if actual_stdout != expected_stdout:
    print("svn pg svn:mime-type output does not match expected.")
    print("Expected standard output:  %s\n" % expected_stdout)
    print("Actual standard output:  %s\n" % actual_stdout)
    raise svntest.verify.SVNUnexpectedOutput

  # Check the svn:executable value.
  # The value of the svn:executable property is now always forced to '*'
  if os.name == 'posix':
    actual_exit, actual_stdout, actual_stderr = svntest.main.run_svn(
      None, 'pg', 'svn:executable', new_path2)

    expected_stdout = ['*\n']
    if actual_stdout != expected_stdout:
      print("svn pg svn:executable output does not match expected.")
      print("Expected standard output:  %s\n" % expected_stdout)
      print("Actual standard output:  %s\n" % actual_stdout)
      raise svntest.verify.SVNUnexpectedOutput

#----------------------------------------------------------------------
# Test for issue #3086 'mod-dav-svn ignores pre-revprop-change failure
# on revprop delete'
#
# If we learn how to write a pre-revprop-change hook for
# non-Posix platforms, we won't have to skip here:
@Skip(is_non_posix_and_non_windows_os)
@Issue(3086)
@XFail(svntest.main.is_ra_type_dav)
def revprop_change(sbox):
  "set, get, and delete a revprop change"

  sbox.build()

  # First test the error when no revprop-change hook exists.
  svntest.actions.run_and_verify_svn(None, None, '.*pre-revprop-change',
                                     'propset', '--revprop', '-r', '0',
                                     'cash-sound', 'cha-ching!', sbox.wc_dir)

  # Now test error output from revprop-change hook.
  svntest.actions.disable_revprop_changes(sbox.repo_dir)
  svntest.actions.run_and_verify_svn(None, None, '.*pre-revprop-change.* 0 jrandom cash-sound A',
                                     'propset', '--revprop', '-r', '0',
                                     'cash-sound', 'cha-ching!', sbox.wc_dir)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--revprop', '-r', '0',
                                     'cash-sound', 'cha-ching!', sbox.wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propget', '--revprop', '-r', '0',
                                     'cash-sound', sbox.wc_dir)

  # Now test that blocking the revprop delete.
  svntest.actions.disable_revprop_changes(sbox.repo_dir)
  svntest.actions.run_and_verify_svn(None, None, '.*pre-revprop-change.* 0 jrandom cash-sound D',
                                     'propdel', '--revprop', '-r', '0',
                                     'cash-sound', sbox.wc_dir)

  # Now test actually deleting the revprop.
  svntest.actions.enable_revprop_changes(sbox.repo_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propdel', '--revprop', '-r', '0',
                                     'cash-sound', sbox.wc_dir)

  actual_exit, actual_stdout, actual_stderr = svntest.main.run_svn(
    None, 'pg', '--revprop', '-r', '0', 'cash-sound', sbox.wc_dir)

  # The property should have been deleted.
  regex = 'cha-ching'
  for line in actual_stdout:
    if re.match(regex, line):
      raise svntest.Failure


#----------------------------------------------------------------------

def prop_value_conversions(sbox):
  "some svn: properties should be converted"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = sbox.ospath('A')
  B_path = sbox.ospath('A/B')
  iota_path = sbox.ospath('iota')
  lambda_path = sbox.ospath('A/B/lambda')
  mu_path = sbox.ospath('A/mu')

  # Leading and trailing whitespace should be stripped
  svntest.actions.set_prop('svn:mime-type', ' text/html\n\n', iota_path)
  svntest.actions.set_prop('svn:mime-type', 'text/html', mu_path)

  # Leading and trailing whitespace should be stripped
  svntest.actions.set_prop('svn:eol-style', '\nnative\n', iota_path)
  svntest.actions.set_prop('svn:eol-style', 'native', mu_path)

  # A trailing newline should be added
  svntest.actions.set_prop('svn:ignore', '*.o\nfoo.c', A_path)
  svntest.actions.set_prop('svn:ignore', '*.o\nfoo.c\n', B_path)

  # A trailing newline should be added
  svntest.actions.set_prop('svn:externals', 'foo http://foo.com/repos', A_path)
  svntest.actions.set_prop('svn:externals', 'foo http://foo.com/repos\n', B_path)

  # Leading and trailing whitespace should be stripped, but not internal
  # whitespace
  svntest.actions.set_prop('svn:keywords', ' Rev Date \n', iota_path)
  svntest.actions.set_prop('svn:keywords', 'Rev  Date', mu_path)

  # svn:executable value should be forced to a '*'
  svntest.actions.set_prop('svn:executable', 'foo', iota_path)
  svntest.actions.set_prop('svn:executable', '*', lambda_path)
  for pval in ('      ', '', 'no', 'off', 'false'):
    svntest.actions.set_prop('svn:executable', pval, mu_path,
                             "svn: warning: W125005.*use 'svn propdel'")

  # Anything else should be untouched
  svntest.actions.set_prop('svn:some-prop', 'bar', lambda_path)
  svntest.actions.set_prop('svn:some-prop', ' bar baz', mu_path)
  svntest.actions.set_prop('svn:some-prop', 'bar\n', iota_path)
  svntest.actions.set_prop('some-prop', 'bar', lambda_path)
  svntest.actions.set_prop('some-prop', ' bar baz', mu_path)
  svntest.actions.set_prop('some-prop', 'bar\n', iota_path)

  # NOTE: When writing out multi-line prop values in svn:* props, the
  # client converts to local encoding and local eoln style.
  # Therefore, the expected output must contain the right kind of eoln
  # strings. That's why we use os.linesep in the tests below, not just
  # plain '\n'. The _last_ \n is also from the client, but it's not
  # part of the prop value and it doesn't get converted in the pipe.

  # Check svn:mime-type
  svntest.actions.check_prop('svn:mime-type', iota_path, ['text/html'])
  svntest.actions.check_prop('svn:mime-type', mu_path, ['text/html'])

  # Check svn:eol-style
  svntest.actions.check_prop('svn:eol-style', iota_path, ['native'])
  svntest.actions.check_prop('svn:eol-style', mu_path, ['native'])

  # Check svn:ignore
  svntest.actions.check_prop('svn:ignore', A_path,
                             ['*.o'+os.linesep, 'foo.c'+os.linesep])
  svntest.actions.check_prop('svn:ignore', B_path,
                             ['*.o'+os.linesep, 'foo.c'+os.linesep])

  # Check svn:externals
  svntest.actions.check_prop('svn:externals', A_path,
                             ['foo http://foo.com/repos'+os.linesep])
  svntest.actions.check_prop('svn:externals', B_path,
                             ['foo http://foo.com/repos'+os.linesep])

  # Check svn:keywords
  svntest.actions.check_prop('svn:keywords', iota_path, ['Rev Date'])
  svntest.actions.check_prop('svn:keywords', mu_path, ['Rev  Date'])

  # Check svn:executable
  svntest.actions.check_prop('svn:executable', iota_path, ['*'])
  svntest.actions.check_prop('svn:executable', lambda_path, ['*'])
  svntest.actions.check_prop('svn:executable', mu_path, ['*'])

  # Check other props
  svntest.actions.check_prop('svn:some-prop', lambda_path, ['bar'])
  svntest.actions.check_prop('svn:some-prop', mu_path, [' bar baz'])
  svntest.actions.check_prop('svn:some-prop', iota_path, ['bar'+os.linesep])
  svntest.actions.check_prop('some-prop', lambda_path, ['bar'])
  svntest.actions.check_prop('some-prop', mu_path,[' bar baz'])
  svntest.actions.check_prop('some-prop', iota_path, ['bar\n'])


#----------------------------------------------------------------------

def binary_props(sbox):
  "test binary property support"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Some path convenience vars.
  A_path = sbox.ospath('A')
  B_path = sbox.ospath('A/B')
  iota_path = sbox.ospath('iota')
  lambda_path = sbox.ospath('A/B/lambda')
  mu_path = sbox.ospath('A/mu')

  A_path_bak = sbox.ospath('A', wc_dir=wc_backup)
  B_path_bak = sbox.ospath('A/B', wc_dir=wc_backup)
  iota_path_bak = sbox.ospath('iota', wc_dir=wc_backup)
  lambda_path_bak = sbox.ospath('A/B/lambda', wc_dir=wc_backup)
  mu_path_bak = sbox.ospath('A/mu', wc_dir=wc_backup)

  # Property value convenience vars.
  prop_zb   = "This property has a zer\000 byte."
  prop_ff   = "This property has a form\014feed."
  prop_xml  = "This property has an <xml> tag."
  prop_binx = "This property has an <xml> tag and a zer\000 byte."

  # Set some binary properties.
  svntest.actions.set_prop('prop_zb', prop_zb, B_path, )
  svntest.actions.set_prop('prop_ff', prop_ff, iota_path)
  svntest.actions.set_prop('prop_xml', prop_xml, lambda_path)
  svntest.actions.set_prop('prop_binx', prop_binx, mu_path)
  svntest.actions.set_prop('prop_binx', prop_binx, A_path)

  # Create expected output and status trees.
  expected_output = svntest.wc.State(wc_dir, {
    'A' : Item(verb='Sending'),
    'A/B' : Item(verb='Sending'),
    'iota' : Item(verb='Sending'),
    'A/B/lambda' : Item(verb='Sending'),
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A', 'A/B', 'iota', 'A/B/lambda', 'A/mu',
                        wc_rev=2, status='  ')

  # Commit the propsets.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Create expected output, disk, and status trees for an update of
  # the wc_backup.
  expected_output = svntest.wc.State(wc_backup, {
    'A' : Item(status=' U'),
    'A/B' : Item(status=' U'),
    'iota' : Item(status=' U'),
    'A/B/lambda' : Item(status=' U'),
    'A/mu' : Item(status=' U'),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)

  # Do the update and check the results.
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 0)

  # Now, check those properties.
  svntest.actions.check_prop('prop_zb', B_path_bak, [prop_zb])
  svntest.actions.check_prop('prop_ff', iota_path_bak, [prop_ff])
  svntest.actions.check_prop('prop_xml', lambda_path_bak, [prop_xml])
  svntest.actions.check_prop('prop_binx', mu_path_bak, [prop_binx])
  svntest.actions.check_prop('prop_binx', A_path_bak, [prop_binx])

#----------------------------------------------------------------------

# Ensure that each line of output contains the corresponding string of
# expected_out, and that errput is empty.
def verify_output(expected_out, output, errput):
  if errput != []:
    print('Error: stderr:')
    print(errput)
    raise svntest.Failure
  output.sort()
  ln = 0
  for line in output:
    if line.startswith('DBG:'):
      continue
    if ((line.find(expected_out[ln]) == -1) or
        (line != '' and expected_out[ln] == '')):
      print('Error: expected keywords:  %s' % expected_out)
      print('       actual full output: %s' % output)
      raise svntest.Failure
    ln = ln + 1
  if ln != len(expected_out):
    raise svntest.Failure

@Issue(1794)
def recursive_base_wc_ops(sbox):
  "recursive property operations in BASE and WC"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Files with which to test, in alphabetical order
  fp_add = sbox.ospath('A/added')
  fp_del = sbox.ospath('A/mu')
  #fp_keep= sbox.ospath('iota')

  # Set up properties
  sbox.simple_propset('p', 'old-del', 'A/mu')
  sbox.simple_propset('p', 'old-keep', 'iota')
  sbox.simple_commit()

  svntest.main.file_append(fp_add, 'blah')
  sbox.simple_add('A/added')
  sbox.simple_propset('p', 'new-add', 'A/added')
  sbox.simple_propset('p', 'new-del', 'A/mu')
  sbox.simple_propset('p', 'new-keep', 'iota')
  svntest.main.run_svn(None, 'del', '--force', fp_del)

  # Test recursive proplist
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist', '-R',
                                                   '-v', wc_dir, '-rBASE')
  verify_output([ 'old-del', 'old-keep', 'p', 'p',
                  'Properties on ', 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  exit_code, output, errput = svntest.main.run_svn(None, 'proplist', '-R',
                                                   '-v', wc_dir)
  verify_output([ 'new-add', 'new-keep', 'p', 'p',
                  'Properties on ', 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test recursive propget
  exit_code, output, errput = svntest.main.run_svn(None, 'propget', '-R',
                                                   'p', wc_dir, '-rBASE')
  verify_output([ 'old-del', 'old-keep' ], output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  exit_code, output, errput = svntest.main.run_svn(None, 'propget', '-R',
                                                   'p', wc_dir)
  verify_output([ 'new-add', 'new-keep' ], output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test recursive propset (issue 1794)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='D ', wc_rev=2)
  expected_status.tweak('iota', status=' M', wc_rev=2)
  expected_status.add({
    'A/added'     : Item(status='A ', wc_rev=0),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '-R', 'svn:keywords', 'Date',
                                     os.path.join(wc_dir, 'A', 'B'))
  expected_status.tweak('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta', status=' M')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def url_props_ops(sbox):
  "property operations on an URL"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  prop1 = 'prop1'
  propval1 = 'propval1 is foo'
  prop2 = 'prop2'
  propval2 = 'propval2'

  iota_url = sbox.repo_url + '/iota'
  A_url = sbox.repo_url + '/A'

  # Add a couple of properties
  sbox.simple_propset(prop1, propval1, 'iota')
  sbox.simple_propset(prop1, propval1, 'A')

  # Commit
  sbox.simple_commit()

  # Add a few more properties
  sbox.simple_propset(prop2, propval2, 'iota')
  sbox.simple_propset(prop2, propval2, 'A')

  # Commit again
  sbox.simple_commit()

  # Test propget
  svntest.actions.run_and_verify_svn(None, [ propval1 + '\n' ], [],
                                     'propget', prop1, iota_url)
  svntest.actions.run_and_verify_svn(None, [ propval1 + '\n' ], [],
                                     'propget', prop1, A_url)

  # Test normal proplist
  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', iota_url)
  verify_output([ prop1, prop2, 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', A_url)
  verify_output([ prop1, prop2, 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test verbose proplist
  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', '-v', iota_url)
  verify_output([ propval1, propval2, prop1, prop2,
                  'Properties on ' ], output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', '-v', A_url)
  verify_output([ propval1, propval2, prop1, prop2,
                  'Properties on ' ], output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test propedit
  svntest.main.use_editor('foo_to_bar')
  propval1 = propval1.replace('foo', 'bar')
  svntest.main.run_svn(None,
                       'propedit', prop1, '-m', 'editlog', iota_url)
  svntest.main.run_svn(None,
                       'propedit', prop1, '-m', 'editlog', A_url)
  svntest.actions.run_and_verify_svn(None, [ propval1 + '\n' ], [],
                                     'propget', prop1, iota_url)
  svntest.actions.run_and_verify_svn(None, [ propval1 + '\n' ], [],
                                     'propget', prop1, A_url)

  # Edit without actually changing the property
  svntest.main.use_editor('identity')
  svntest.actions.run_and_verify_svn(None,
                                     "No changes to property '%s' on '.*'"
                                       % prop1,
                                     [],
                                     'propedit', prop1, '-m', 'nocommit',
                                     iota_url)



#----------------------------------------------------------------------
def removal_schedule_added_props(sbox):
  "removal of schedule added file with properties"

  sbox.build()

  wc_dir = sbox.wc_dir
  newfile_path = sbox.ospath('newfile')
  file_add_output = ["A         " + newfile_path + "\n"]
  propset_output = ["property 'newprop' set on '" + newfile_path + "'\n"]
  file_rm_output = ["D         " + newfile_path + "\n"]
  propls_output = [
     "Properties on '" + newfile_path + "':\n",
     "  newprop\n",
     "    newvalue\n",
                  ]

  # create new fs file
  open(newfile_path, 'w').close()
  # Add it and set a property
  svntest.actions.run_and_verify_svn(None, file_add_output, [], 'add', newfile_path)
  svntest.actions.run_and_verify_svn(None, propset_output, [], 'propset',
                                     'newprop', 'newvalue', newfile_path)
  svntest.actions.run_and_verify_svn(None, propls_output, [],
                                     'proplist', '-v', newfile_path)
  # remove the file
  svntest.actions.run_and_verify_svn(None, file_rm_output, [],
                                     'rm', '--force', newfile_path)
  # recreate the file and add it again
  open(newfile_path, 'w').close()
  svntest.actions.run_and_verify_svn(None, file_add_output, [], 'add', newfile_path)

  # Now there should be NO properties leftover...
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'proplist', '-v', newfile_path)

#----------------------------------------------------------------------

def update_props_on_wc_root(sbox):
  "receive properties on the wc root via update"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Add a property to the root folder
  sbox.simple_propset('red', 'rojo', '')

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    '' : Item(verb='Sending')
    })

  # Created expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', wc_rev=2, status='  ')

  # Commit the working copy
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

 # Create expected output tree for an update of the wc_backup.
  expected_output = svntest.wc.State(wc_backup, {
    '' : Item(status=' U'),
    })
  # Create expected disk tree for the update.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    '' : Item(props = {'red' : 'rojo'}),
    })
  # Create expected status tree for the update.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.tweak('', status='  ')

  # Do the update and check the results in three ways... INCLUDING PROPS
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1)

# test for issue 2743
@Issue(2743)
def props_on_replaced_file(sbox):
  """test properties on replaced files"""

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add some properties to iota
  iota_path = sbox.ospath("iota")
  sbox.simple_propset('red', 'rojo', 'iota')
  sbox.simple_propset('blue', 'lagoon', 'iota')
  sbox.simple_commit()

  # replace iota_path
  sbox.simple_rm('iota')
  svntest.main.file_append(iota_path, "some mod")
  sbox.simple_add('iota')

  # check that the replaced file has no properties
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents="some mod")
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # now add a new property to iota
  sbox.simple_propset('red', 'mojo', 'iota')
  sbox.simple_propset('groovy', 'baby', 'iota')

  # What we expect the disk tree to look like:
  expected_disk.tweak('iota', props={'red' : 'mojo', 'groovy' : 'baby'})
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

#----------------------------------------------------------------------

def depthy_wc_proplist(sbox):
  """test proplist at various depths on a wc"""
  # Bootstrap.
  sbox.build()
  wc_dir = sbox.wc_dir

  # Set up properties.
  sbox.simple_propset('p', 'prop1', '')
  sbox.simple_propset('p', 'prop2', 'iota')
  sbox.simple_propset('p', 'prop3', 'A')
  sbox.simple_propset('p', 'prop4', 'A/mu')

  # Commit.
  sbox.simple_commit()

  # Test depth-empty proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist',
                                                   '--depth', 'empty',
                                                   '-v', wc_dir)
  verify_output([ 'prop1', 'p', 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-files proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist',
                                                   '--depth', 'files',
                                                   '-v', wc_dir)
  verify_output([ 'prop1', 'prop2', 'p', 'p',
                  'Properties on ', 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-immediates proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist', '--depth',
                                                   'immediates', '-v', wc_dir)
  verify_output([ 'prop1', 'prop2', 'prop3' ] +
                ['p'] * 3 + ['Properties on '] * 3,
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-infinity proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist', '--depth',
                                                   'infinity', '-v', wc_dir)
  verify_output([ 'prop1', 'prop2', 'prop3', 'prop4' ] +
                ['p'] * 4 + ['Properties on '] * 4,
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

#----------------------------------------------------------------------

def depthy_url_proplist(sbox):
  """test proplist at various depths on a url"""
  # Bootstrap.
  sbox.build()
  repo_url = sbox.repo_url
  wc_dir = sbox.wc_dir

  # Set up properties.
  sbox.simple_propset('p', 'prop1', '')
  sbox.simple_propset('p', 'prop2', 'iota')
  sbox.simple_propset('p', 'prop3', 'A')
  sbox.simple_propset('p', 'prop4', 'A/mu')
  sbox.simple_commit()

  # Test depth-empty proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist',
                                                   '--depth', 'empty',
                                                   '-v', repo_url)
  verify_output([ 'prop1', 'p', 'Properties on '],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-files proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist',
                                                   '--depth', 'files',
                                                   '-v', repo_url)
  verify_output([ 'prop1', 'prop2', 'p', 'p',
                 'Properties on ', 'Properties on ' ],
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-immediates proplist.
  exit_code, output, errput = svntest.main.run_svn(None, 'proplist',
                                                   '--depth', 'immediates',
                                                   '-v', repo_url)

  verify_output([ 'prop1', 'prop2', 'prop3' ] + ['p'] * 3 +
                ['Properties on '] * 3,
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Test depth-infinity proplist.
  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', '--depth',
                                                   'infinity', '-v', repo_url)
  verify_output([ 'prop1', 'prop2', 'prop3', 'prop4' ] + ['p'] * 4 +
                ['Properties on '] * 4,
                output, errput)
  svntest.verify.verify_exit_code(None, exit_code, 0)

#----------------------------------------------------------------------

def invalid_propnames(sbox):
  """test prop* handle invalid property names"""
  # Bootstrap.
  sbox.build()
  repo_url = sbox.repo_url
  wc_dir = sbox.wc_dir
  cwd = os.getcwd()
  os.chdir(wc_dir)

  propname = chr(8)
  propval = 'foo'

  expected_stdout = (".*Attempting to delete nonexistent property "
                     "'%s'.*" % (propname,))
  svntest.actions.run_and_verify_svn(None, expected_stdout, [],
                                     'propdel', propname)
  expected_stderr = (".*'%s' is not a valid Subversion"
                     ' property name' % (propname,))
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'propedit', propname)
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'propget', propname)
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'propset', propname, propval)

  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'commit', '--with-revprop',
                                     '='.join([propname, propval]))
  # Now swap them: --with-revprop should accept propname as a property
  # value; no concept of validity there.
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'commit', '--with-revprop',
                                     '='.join([propval, propname]))

  os.chdir(cwd)

@SkipUnless(svntest.main.is_posix_os)
@Issue(2581)
def perms_on_symlink(sbox):
  "propset shouldn't touch symlink perms"
  sbox.build()
  # We can't just run commands on absolute paths in the usual way
  # (e.g., os.path.join(sbox.wc_dir, 'newdir')), because for some
  # reason, if the symlink points to newdir as an absolute path, the
  # bug doesn't reproduce.  I have no idea why.  Since it does have to
  # point to newdir, the only other choice is to have it point to it
  # in the same directory, so we have to run the test from inside the
  # working copy.
  saved_cwd = os.getcwd()
  os.chdir(sbox.wc_dir)
  try:
    svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', 'newdir')
    os.symlink('newdir', 'symlink')
    svntest.actions.run_and_verify_svn(None, None, [], 'add', 'symlink')
    old_mode = os.stat('newdir')[stat.ST_MODE]
    # The only property on 'symlink' is svn:special, so attempting to remove
    # 'svn:executable' should result in an error
    expected_stdout = (".*Attempting to delete nonexistent property "
                       "'svn:executable'.*")
    svntest.actions.run_and_verify_svn(None, expected_stdout, [], 'propdel',
                                     'svn:executable', 'symlink')
    new_mode = os.stat('newdir')[stat.ST_MODE]
    if not old_mode == new_mode:
      # Chmod newdir back, so the test suite can remove this working
      # copy when cleaning up later.
      os.chmod('newdir', stat.S_IRWXU | stat.S_IRWXG | stat.S_IRWXO)
      raise svntest.Failure
  finally:
    os.chdir(saved_cwd)

# Use a property with a custom namespace, ie 'ns:prop' or 'mycompany:prop'.
def remove_custom_ns_props(sbox):
  "remove a property with a custom namespace"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a property to a file
  sbox.simple_propset('ns:cash-sound', 'cha-ching!', 'iota')

  # Commit the file
  sbox.simple_commit('iota')

  # Now, make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Remove the property
  sbox.simple_propdel('ns:cash-sound', 'iota')

  # Create expected trees.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=3, status='  ')

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Create expected trees for the update.
  expected_output = svntest.wc.State(wc_backup, {
    'iota' : Item(status=' U'),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_backup, 3)
  expected_status.tweak('iota', wc_rev=3, status='  ')

  # Do the update and check the results in three ways... INCLUDING PROPS
  svntest.actions.run_and_verify_update(wc_backup,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, 1)

def props_over_time(sbox):
  "property retrieval with peg and operative revs"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Convenience variables
  iota_path = sbox.ospath('iota')
  iota_url = sbox.repo_url + '/iota'

  # Add/tweak a property 'revision' with value revision-committed to a
  # file, commit, and then repeat this a few times.
  for rev in range(2, 4):
    sbox.simple_propset('revision', str(rev), 'iota')
    sbox.simple_commit('iota')

  # Backdate to r2 so the defaults for URL- vs. WC-style queries are
  # different.
  svntest.main.run_svn(None, 'up', '-r2', wc_dir)

  # Now, test propget of the property across many combinations of
  # pegrevs, operative revs, and wc-path vs. url style input specs.
  # NOTE: We're using 0 in these loops to mean "unspecified".
  for path in iota_path, iota_url:
    for peg_rev in range(0, 4):
      for op_rev in range(0, 4):
        # Calculate the expected property value.  If there is an
        # operative rev, we expect the output to match revisions
        # there.  Else, we'll be looking at the peg-rev value.  And if
        # neither are supplied, it depends on the path vs. URL
        # question.
        if op_rev > 1:
          expected = str(op_rev)
        elif op_rev == 1:
          expected = None
        else:
          if peg_rev > 1:
            expected = str(peg_rev)
          elif peg_rev == 1:
            expected = None
          else:
            if path == iota_url:
              expected = "3" # HEAD
            else:
              expected = "2" # BASE

        peg_path = path + (peg_rev != 0 and '@' + str(peg_rev) or "")

        ### Test 'svn propget'
        pget_expected = expected
        if pget_expected:
          pget_expected = [ pget_expected + "\n" ]
        if op_rev != 0:
          svntest.actions.run_and_verify_svn(None, pget_expected, [],
                                             'propget', 'revision', peg_path,
                                             '-r', str(op_rev))
        else:
          svntest.actions.run_and_verify_svn(None, pget_expected, [],
                                             'propget', 'revision', peg_path)

        ### Test 'svn proplist -v'
        if op_rev != 0 or peg_rev != 0:  # a revision-ful query output URLs
          path = iota_url
        plist_expected = expected
        if plist_expected:
          plist_expected = [ "Properties on '" + path + "':\n",
                             "  revision\n",
                             "    " + expected + "\n" ]

        if op_rev != 0:
          svntest.actions.run_and_verify_svn(None, plist_expected, [],
                                             'proplist', '-v', peg_path,
                                             '-r', str(op_rev))
        else:
          svntest.actions.run_and_verify_svn(None, plist_expected, [],
                                             'proplist', '-v', peg_path)


# XFail the same reason revprop_change() is.
@SkipUnless(svntest.main.server_enforces_date_syntax)
@XFail(svntest.main.is_ra_type_dav)
@Issue(3086)
def invalid_propvalues(sbox):
  "test handling invalid svn:* property values"

  sbox.build(create_wc = False)
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url

  svntest.actions.enable_revprop_changes(repo_dir)

  expected_stderr = '.*unexpected property value.*|.*Bogus date.*'
  svntest.actions.run_and_verify_svn(None, [], expected_stderr,
                                     'propset', '--revprop', '-r', '0',
                                     'svn:date', 'Sat May 10 12:12:31 2008',
                                     repo_url)

@Issue(3282)
def same_replacement_props(sbox):
  "commit replacement props when same as old props"
  # issue #3282
  sbox.build()

  foo_path = sbox.ospath('foo')

  open(foo_path, 'w').close()
  sbox.simple_add('foo')
  sbox.simple_propset('someprop', 'someval', 'foo')
  sbox.simple_commit('foo')
  sbox.simple_rm('foo')

  # Now replace 'foo'.
  open(foo_path, 'w').close()
  sbox.simple_add('foo')

  # Set the same property again, with the same value.
  sbox.simple_propset('someprop', 'someval', 'foo')
  sbox.simple_commit('foo')

  # Check if the property made it into the repository.
  foo_url = sbox.repo_url + '/foo'
  expected_out = [ "Properties on '" + foo_url + "':\n",
                   "  someprop\n",
                   "    someval\n" ]
  svntest.actions.run_and_verify_svn(None, expected_out, [],
                                     'proplist', '-v', foo_url)

def added_moved_file(sbox):
  "'svn mv added_file' preserves props"

  sbox.build()
  wc_dir = sbox.wc_dir

  # create it
  foo_path = sbox.ospath('foo')
  foo2_path = sbox.ospath('foo2')
  foo2_url = sbox.repo_url + '/foo2'

  open(foo_path, 'w').close()

  # add it
  sbox.simple_add('foo')
  sbox.simple_propset('someprop', 'someval', 'foo')

  # move it
  svntest.main.run_svn(None, 'mv', foo_path, foo2_path)

  # should still have the property
  svntest.actions.check_prop('someprop', foo2_path, ['someval'])

  # the property should get committed, too
  sbox.simple_commit()
  svntest.actions.check_prop('someprop', foo2_url, ['someval'])


# Issue 2220, deleting a non-existent property should error
@Issue(2220)
def delete_nonexistent_property(sbox):
  "remove a property which doesn't exist"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  # Remove one property
  expected_stdout = ".*Attempting to delete nonexistent property 'yellow'.*"
  svntest.actions.run_and_verify_svn(None, expected_stdout, [],
                                     'propdel', 'yellow',
                                     os.path.join(wc_dir, 'A', 'D', 'G'))

#----------------------------------------------------------------------
@Issue(3553)
def post_revprop_change_hook(sbox):
  "post-revprop-change hook"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir

  # Include a non-XML-safe message to regression-test issue #3553.
  error_msg = 'Text with <angle brackets> & ampersand'

  svntest.actions.enable_revprop_changes(repo_dir)
  svntest.actions.create_failing_hook(repo_dir, 'post-revprop-change',
                                      error_msg)

  # serf/neon/mod_dav_svn splits the "svn: hook failed" line
  expected_error = svntest.verify.RegexOutput([
    '(svn: E165001: |)post-revprop-change hook failed',
    error_msg + "\n",
  ], match_all = False)

  svntest.actions.run_and_verify_svn(None, [], expected_error,
                                     'ps', '--revprop', '-r0', 'p', 'v',
                                     wc_dir)

  # Verify change has stuck -- at one time mod_dav_svn would rollback
  # revprop changes on post-revprop-change hook errors
  svntest.actions.run_and_verify_svn(None, 'v', [],
                                     'pg', '--revprop', '-r0', 'p',
                                     wc_dir)

def rm_of_replaced_file(sbox):
  """properties after a removal of a replaced file"""

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add some properties to iota and mu
  iota_path = sbox.ospath('iota')
  sbox.simple_propset('red', 'rojo', 'iota')
  sbox.simple_propset('blue', 'lagoon', 'iota')

  mu_path = sbox.ospath('A/mu')
  sbox.simple_propset('yellow', 'submarine', 'A/mu')
  sbox.simple_propset('orange', 'toothpick', 'A/mu')

  sbox.simple_commit()

  # Copy iota over the top of mu
  sbox.simple_rm('A/mu')
  svntest.main.run_svn(None, 'cp', iota_path, mu_path)

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', props={'red': 'rojo', 'blue': 'lagoon'})
  expected_disk.tweak('A/mu', props={'red': 'rojo', 'blue': 'lagoon'},
                      contents="This is the file 'iota'.\n")
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Remove the copy. This should leave the original locally-deleted mu,
  # which should have no properties.
  svntest.main.run_svn(None, 'rm', '--force', mu_path)

  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', '-v', mu_path)
  if output or errput:
    raise svntest.Failure('no output/errors expected')
  svntest.verify.verify_exit_code(None, exit_code, 0)

  # Run it again, but ask for the pristine properties, which should
  # be mu's original props.
  exit_code, output, errput = svntest.main.run_svn(None,
                                                   'proplist', '-v',
                                                   mu_path + '@base')
  expected_output = svntest.verify.UnorderedRegexOutput([
      'Properties on',
      '  yellow',
      '    submarine',
      '  orange',
      '    toothpick',
      ])
  svntest.verify.compare_and_display_lines('message', 'label',
                                           expected_output, output)
  svntest.verify.verify_exit_code(None, exit_code, 0)


def prop_reject_grind(sbox):
  """grind through all variants of prop rejects"""

  sbox.build()
  wc_dir = sbox.wc_dir

  mu_path = sbox.ospath('A/mu')
  mu_prej_path = sbox.ospath('A/mu.prej')

  # Create r2 with all the properties we intend to use as incoming-change,
  # and as incoming-delete. Also set up our local-edit and local-delete
  # properties. We also need some properties that are simply different
  # from the incoming properties
  sbox.simple_propset('edit.diff', 'repos', 'iota')
  sbox.simple_propset('edit.edit', 'repos', 'iota')
  sbox.simple_propset('edit.del', 'repos', 'iota')
  sbox.simple_propset('edit.add', 'repos', 'iota')
  sbox.simple_propset('edit.none', 'repos', 'iota')
  sbox.simple_propset('del.edit', 'repos', 'iota')
  sbox.simple_propset('del.edit2', 'repos', 'iota')
  sbox.simple_propset('del.diff', 'repos', 'iota')
  sbox.simple_propset('del.del', 'repos', 'iota')
  sbox.simple_propset('del.add', 'repos', 'iota')

  sbox.simple_propset('edit.edit', 'local', 'A/mu')
  sbox.simple_propset('add.edit', 'local', 'A/mu')
  sbox.simple_propset('del.edit', 'local', 'A/mu')
  sbox.simple_propset('del.edit2', 'repos', 'A/mu')
  sbox.simple_propset('add.del', 'local', 'A/mu')
  sbox.simple_propset('edit.del', 'local', 'A/mu')
  sbox.simple_propset('del.del', 'local', 'A/mu')
  sbox.simple_propset('edit.diff', 'local', 'A/mu')
  sbox.simple_propset('add.diff', 'local', 'A/mu')
  sbox.simple_propset('del.diff', 'local', 'A/mu')

  sbox.simple_commit()

  # Create r3 with all the properties that we intend to use as incoming-add,
  # and then perform the incoming-edits and incoming-deletes.
  sbox.simple_propset('add.add', 'repos', 'iota')
  sbox.simple_propset('add.edit', 'repos', 'iota')
  sbox.simple_propset('add.del', 'repos', 'iota')
  sbox.simple_propset('add.diff', 'repos', 'iota')
  sbox.simple_propset('edit.diff', 'repos.changed', 'iota')
  sbox.simple_propset('edit.edit', 'repos.changed', 'iota')
  sbox.simple_propset('edit.del', 'repos.changed', 'iota')
  sbox.simple_propset('edit.add', 'repos.changed', 'iota')
  sbox.simple_propset('edit.none', 'repos.changed', 'iota')
  sbox.simple_propdel('del.edit', 'iota')
  sbox.simple_propdel('del.edit2', 'iota')
  sbox.simple_propdel('del.diff', 'iota')
  sbox.simple_propdel('del.del', 'iota')
  sbox.simple_propdel('del.add', 'iota')
  sbox.simple_commit()

  # Set up our victim for all the right rejects: local-adds, local-edits,
  # and local-deletes.
  sbox.simple_propset('edit.add', 'local', 'A/mu')
  sbox.simple_propset('add.add', 'local', 'A/mu')
  sbox.simple_propset('del.add', 'local', 'A/mu')
  sbox.simple_propset('edit.edit', 'local.changed', 'A/mu')
  sbox.simple_propset('add.edit', 'local.changed', 'A/mu')
  sbox.simple_propset('del.edit', 'local.changed', 'A/mu')
  sbox.simple_propset('del.edit2', 'repos.changed', 'A/mu')
  sbox.simple_propdel('add.del', 'A/mu')
  sbox.simple_propdel('edit.del', 'A/mu')
  sbox.simple_propdel('del.del', 'A/mu')

  # Now merge r2:3 into the victim to create all variants
  svntest.main.run_svn(False, 'merge', '-r2:3', sbox.repo_url + '/iota',
                       mu_path)

  # Check that A/mu.prej reports the expected conflicts:
  expected_prej = [
    "Trying to change property 'edit.none'\n"
    "but the property does not exist locally.\n"
    "<<<<<<< (local property value)\n"
    "=======\n"
    "repos.changed>>>>>>> (incoming property value)\n",

    "Trying to delete property 'del.del'\n"
    "but the property has been locally deleted and had a different value.\n",

    "Trying to delete property 'del.edit'\n"
    "but the local property value is different.\n"
    "<<<<<<< (local property value)\n"
    "local.changed=======\n"
    ">>>>>>> (incoming property value)\n",

    "Trying to change property 'edit.del'\n"
    "but the property has been locally deleted.\n"
    "<<<<<<< (local property value)\n"
    "=======\n"
    "repos.changed>>>>>>> (incoming property value)\n",

    "Trying to change property 'edit.edit'\n"
    "but the property has already been locally changed to a different value.\n"
    "<<<<<<< (local property value)\n"
    "local.changed=======\n"
    "repos.changed>>>>>>> (incoming property value)\n",

    "Trying to delete property 'del.edit2'\n"
    "but the property has been locally modified.\n"
    "<<<<<<< (local property value)\n"
    "repos.changed=======\n"
    ">>>>>>> (incoming property value)\n",

    "Trying to delete property 'del.add'\n"
    "but the property has been locally added.\n"
    "<<<<<<< (local property value)\n"
    "local=======\n"
    ">>>>>>> (incoming property value)\n",

    "Trying to delete property 'del.diff'\n"
    "but the local property value is different.\n"
    "<<<<<<< (local property value)\n"
    "local=======\n"
    ">>>>>>> (incoming property value)\n",

    "Trying to change property 'edit.add'\n"
    "but the property has been locally added with a different value.\n"
    "<<<<<<< (local property value)\n"
    "local=======\n"
    "repos.changed>>>>>>> (incoming property value)\n",

    "Trying to change property 'edit.diff'\n"
    "but the local property value conflicts with the incoming change.\n"
    "<<<<<<< (local property value)\n"
    "local=======\n"
    "repos.changed>>>>>>> (incoming property value)\n",

    "Trying to add new property 'add.add'\n"
    "but the property already exists.\n"
    "<<<<<<< (local property value)\n"
    "local=======\n"
    "repos>>>>>>> (incoming property value)\n",

    "Trying to add new property 'add.diff'\n"
    "but the property already exists.\n"
    "Local property value:\n"
    "local\n"
    "Incoming property value:\n"
    "repos\n",

    "Trying to add new property 'add.del'\n"
    "but the property has been locally deleted.\n"
    "<<<<<<< (local property value)\n"
    "=======\n"
    "repos>>>>>>> (incoming property value)\n",

    "Trying to add new property 'add.edit'\n"
    "but the property already exists.\n"
    "<<<<<<< (local property value)\n"
    "local.changed=======\n"
    "repos>>>>>>> (incoming property value)\n",
  ]

  # Get the contents of mu.prej.  The error messages are in the prej file
  # but there is no guarantee as to order. So try to locate each message
  # in the file individually.
  prej_file = open(mu_prej_path, 'r')
  n = 0
  for message in expected_prej:
    prej_file.seek(0)
    match = False
    i = 0
    j = 0
    msg_lines = message.split('\n')
    for file_line in prej_file:
      line = msg_lines[i] + '\n'
      match = (line == file_line)
      if match:
        # The last line in the list is always an empty string.
        if msg_lines[i + 1] == "":
          #print("found message %i in file at line %i" % (n, j))
          break
        i += 1
      else:
        i = 0
      j += 1
    n += 1
    if not match:
      raise svntest.main.SVNUnmatchedError(
              "Expected mu.prej doesn't match actual mu.prej")

def obstructed_subdirs(sbox):
  """test properties of obstructed subdirectories"""

  sbox.build()
  wc_dir = sbox.wc_dir

  # at one point during development, obstructed subdirectories threw
  # errors trying to fetch property information during 'svn status'.
  # this test ensures we won't run into that problem again.

  C_path = sbox.ospath('A/C')
  sbox.simple_propset('red', 'blue', 'A/C')

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/C', props={'red': 'blue'})
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, load_props=True)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Remove the subdir from disk, and validate the status
  svntest.main.safe_rmtree(C_path)

  expected_disk.remove('A/C')
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, load_props=True)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/C', status='!M', wc_rev='1')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Drop an empty file there to obstruct the now-deleted subdir
  open(C_path, 'w')

  expected_disk.add({'A/C': Item(contents='', props={'red': 'blue'})})
  expected_status.tweak('A/C', status='~M', wc_rev='1')

  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, load_props=True)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())


  svntest.actions.run_and_verify_status(wc_dir, expected_status)


def atomic_over_ra(sbox):
  "test revprop atomicity guarantees of libsvn_ra"

  sbox.build(create_wc=False)
  repo_url = sbox.repo_url

  # From this point on, similar to ../libsvn_fs-fs-test.c:revision_props().
  s1 = "violet"
  s2 = "wrong value"

  # But test "" explicitly, since the RA layers have to marshal "" and <unset>
  # differently.
  s3 = ""

  # Initial state.
  svntest.actions.enable_revprop_changes(sbox.repo_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--revprop', '-r', '0',
                                     'flower', s1, repo_url)

  # Helpers.

  def expect_old_server_fail(old_value, proposed_value):
    # We are setting a (possibly "not present") expectation for the old value,
    # so we should fail.
    expected_stderr = ".*doesn't advertise.*ATOMIC_REVPROP"
    svntest.actions.run_and_verify_atomic_ra_revprop_change(
       None, None, expected_stderr, 1, repo_url, 0, 'flower',
       old_value, proposed_value)

    # The original value is still there.
    svntest.actions.check_prop('flower', repo_url, [s1], 0)

  def FAILS_WITH_BPV(not_the_old_value, proposed_value):
    if svntest.main.server_has_atomic_revprop():
      svntest.actions.run_and_verify_atomic_ra_revprop_change(
         None, None, [], 0, repo_url, 0, 'flower',
         not_the_old_value, proposed_value, True)
    else:
      expect_old_server_fail(not_the_old_value, proposed_value)

  def PASSES_WITHOUT_BPV(yes_the_old_value, proposed_value):
    if svntest.main.server_has_atomic_revprop():
      svntest.actions.run_and_verify_atomic_ra_revprop_change(
         None, None, [], 0, repo_url, 0, 'flower',
         yes_the_old_value, proposed_value, False)
    else:
      expect_old_server_fail(yes_the_old_value, proposed_value)

  # Value of "flower" is 's1'.
  FAILS_WITH_BPV(s2, s1)
  FAILS_WITH_BPV(s3, s1)
  PASSES_WITHOUT_BPV(s1, s2)

  # Value of "flower" is 's2'.
  PASSES_WITHOUT_BPV(s2, s3)

  # Value of "flower" is 's3'.
  FAILS_WITH_BPV(None, s3)
  FAILS_WITH_BPV(s1, s3)
  PASSES_WITHOUT_BPV(s3, s2)

  # Value of "flower" is 's2'.
  FAILS_WITH_BPV(None, None)
  FAILS_WITH_BPV(s1, None)
  FAILS_WITH_BPV(s3, None)
  PASSES_WITHOUT_BPV(s2, None)

  # Value of "flower" is <not set>.
  FAILS_WITH_BPV(s2, s1)
  FAILS_WITH_BPV(s3, s1)
  PASSES_WITHOUT_BPV(None, s1)

  # Value of "flower" is 's1'.
  svntest.actions.check_prop('flower', repo_url, [s1], 0)

# Test for issue #3721 'redirection of svn propget output corrupted with
# large property values'
@Issue(3721)
def propget_redirection(sbox):
  """pg of large text properties redirects properly"""

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  B_path = os.path.join(wc_dir, "A", "B")
  C_path = os.path.join(wc_dir, "A", "C")
  D_path = os.path.join(wc_dir, "A", "D")

  prop_val_file = os.path.join(wc_dir, "prop_val")
  redirect_file = os.path.join(wc_dir, "pg.vR.out")

  # A 'big' mergeinfo property.  Yes, it is bogus in the sense that
  # it refers to non-existent path-revs, but that is not relevant to
  # this test.  What matters is that it is a realistic 'big' mergeinfo
  # value (it is from Subversion's own 1.6.x branch in fact).
  big_prop_val = "subversion/branches/1.5.x:872364-874936\n"           + \
  "subversion/branches/1.5.x-34184:874657-874741\n"                    + \
  "subversion/branches/1.5.x-34432:874744-874798\n"                    + \
  "subversion/branches/1.5.x-issue3067:872184-872314\n"                + \
  "subversion/branches/1.5.x-issue3157:872165-872175\n"                + \
  "subversion/branches/1.5.x-issue3174:872178-872348\n"                + \
  "subversion/branches/1.5.x-r30215:870310,870312,870319,870362\n"     + \
  "subversion/branches/1.5.x-r30756:874853-874870\n"                   + \
  "subversion/branches/1.5.x-r30868:870951-870970\n"                   + \
  "subversion/branches/1.5.x-r31314:874476-874605\n"                   + \
  "subversion/branches/1.5.x-r31516:871592-871649\n"                   + \
  "subversion/branches/1.5.x-r32470:872546-872676\n"                   + \
  "subversion/branches/1.5.x-r32968:873773-873872\n"                   + \
  "subversion/branches/1.5.x-r33447:873527-873547\n"                   + \
  "subversion/branches/1.5.x-r33465:873541-873549\n"                   + \
  "subversion/branches/1.5.x-r33641:873880-873883\n"                   + \
  "subversion/branches/1.5.x-r34050-followups:874639-874686\n"         + \
  "subversion/branches/1.5.x-r34487:874562-874581\n"                   + \
  "subversion/branches/1.5.x-ra_serf-backports:872354-872626\n"        + \
  "subversion/branches/1.5.x-rb-test-fix:874916-874919\n"              + \
  "subversion/branches/1.5.x-reintegrate-improvements:874586-874922\n" + \
  "subversion/branches/1.5.x-tests-pass:870925-870973\n"               + \
  "subversion/branches/dont-save-plaintext-passwords-by-default:"      + \
  "870728-871118\n"                                                    + \
  "subversion/branches/gnome-keyring:870558-871410\n"                  + \
  "subversion/branches/issue-3220-dev:872210-872226\n"                 + \
  "subversion/branches/kwallet:870785-871314\n"                        + \
  "subversion/branches/log-g-performance:870941-871032\n"              + \
  "subversion/branches/r30963-1.5.x:871056-871076\n"                   + \
  "subversion/branches/reintegrate-improvements:873853-874164\n"       + \
  "subversion/branches/svn-mergeinfo-enhancements:870196\n"            + \
  "subversion/branches/svnpatch-diff:871905\n"                         + \
  "subversion/trunk:869159-869165,869168-869181,869185,869188,869191," + \
  "869200-869201,869203-869207,869209-869224,869227-869238,869240-"    + \
  "869244,869248,869250-869260,869262-869263,869265,869267-869268,"    + \
  "869272-869280,869282-869325,869328-869330,869335,869341-869347,"    + \
  "869351,869354-869355,869358,869361-869377,869379-869381,869383-"    + \
  "869417,869419-869422,869432-869453,869455-869466,869471-869473,"    + \
  "869475,869483,869486,869488-869489,869491-869497,869499-869500,"    + \
  "869503,869506-869508,869510-869521,869523-869540,869542-869552,"    + \
  "869556,869558,869560-869561,869563,869565,869567,869570,869572,"    + \
  "869582,869601-869602,869605,869607,869613-869614,869616,869618,"    + \
  "869620,869625,869627,869630,869633,869639,869641-869643,869645-"    + \
  "869652,869655,869657,869665,869668,869674,869677,869681,869685,"    + \
  "869687-869688,869693,869697,869699-869700,869704-869708,869716,"    + \
  "869719,869722,869724,869730,869733-869734,869737-869740,869745-"    + \
  "869746,869751-869754,869766,869812-869813,869815-869818,869820,"    + \
  "869825,869837,869841,869843-869844,869858,869860-869861,869871,"    + \
  "869875,869889,869895,869898,869902,869907,869909,869926,869928-"    + \
  "869929,869931-869933,869942-869943,869950,869952,869957-869958,"    + \
  "869969,869972,869974,869988,869994,869996,869999,870004,870013-"    + \
  "870014,870016,870024,870032,870036,870039,870041-870043,870054,"    + \
  "870060,870068-870071,870078,870083,870094,870104,870124,870127-"    + \
  "870128,870133,870135-870136,870141,870144,870148,870160,870172,"    + \
  "870175,870191,870198,870203-870204,870211,870219,870225,870233,"    + \
  "870235-870236,870254-870255,870259,870307,870311,870313,870320,"    + \
  "870323,870330-870331,870352-870353,870355,870359-870360,870371,"    + \
  "870373,870378,870393-870395,870402,870409-870410,870414,870416,"    + \
  "870421,870436,870442,870447,870449,870452,870454,870466,870476,"    + \
  "870481-870483,870486,870500,870502,870505,870513-870518,870522-"    + \
  "870523,870527,870529,870534,870536-870538,870540-870541,870543-"    + \
  "870548,870554,870556,870561,870563,870584,870590-870592,870594-"    + \
  "870595,870597,870618,870620,870622,870625-870626,870641,870647,"    + \
  "870657,870665,870671,870681,870702-870703,870706-870708,870717-"    + \
  "870718,870727,870730,870737,870740,870742,870752,870758,870800,"    + \
  "870809,870815,870817,870820-870825,870830,870834-870836,870850-"    + \
  "870851,870853,870859,870861,870886,870894,870916-870918,870942,"    + \
  "870945,870957,870962,870970,870979,870981,870989,870996,871003,"    + \
  "871005,871009,871011,871023,871033,871035-871038,871041,871060,"    + \
  "871078,871080,871092,871097,871099,871105,871107,871120,871123-"    + \
  "871127,871130,871133-871135,871140,871149,871155-871156,871160,"    + \
  "871162,871164,871181,871191,871199-871200,871205,871211-871212,"    + \
  "871215,871219,871225,871227,871229,871231,871236,871270,871273,"    + \
  "871277,871283,871297,871302,871306,871308,871315-871320,871323-"    + \
  "871325,871333-871335,871345,871347-871350,871354,871357,871361,"    + \
  "871363-871366,871374-871375,871377,871382,871385-871388,871391,"    + \
  "871408,871411,871422,871435,871441,871443-871444,871465,871470,"    + \
  "871472-871476,871481,871489,871499,871501-871502,871505,871508,"    + \
  "871520,871523,871525-871527,871538,871542,871544,871547-871549,"    + \
  "871556,871559,871562-871563,871578,871581,871587,871589-871597,"    + \
  "871608,871613,871616-871617,871620,871624,871649,871668,871675,"    + \
  "871677,871693-871694,871696,871704,871732-871733,871744,871747,"    + \
  "871759,871762,871766,871769,871793,871796,871799,871801,871811,"    + \
  "871813,871821-871826,871831,871843,871860,871880,871891,871894,"    + \
  "871899,871907,871911,871926,871928,871933,871935,871941-871942,"    + \
  "871947-871949,871958,871974,872000-872001,872003,872005,872018,"    + \
  "872022,872038,872065,872068,872086,872091,872093,872097,872103,"    + \
  "872112,872130,872154,872157,872206,872216,872218-872219,872227,"    + \
  "872234,872238,872243,872253,872255,872259,872261,872278-872279,"    + \
  "872281,872310-872311,872362,872404,872416-872417,872429,872431,"    + \
  "872434,872439,872450-872453,872468,872470,872477-872478,872483,"    + \
  "872490-872491,872495,872515-872516,872518-872519,872537,872541,"    + \
  "872544,872565,872568,872571-872573,872584,872596-872597,872612,"    + \
  "872619,872624,872632,872656,872670,872706,872710,872713,872717,"    + \
  "872746-872748,872777,872780-872782,872791,872804,872813,872845,"    + \
  "872864,872870,872872,872947-872948,872961,872974,872981,872985-"    + \
  "872987,873004,873042,873049,873051,873076,873087,873090,873096,"    + \
  "873098,873100,873183,873186,873192,873195,873210-873211,873247,"    + \
  "873252,873256,873259,873275,873286,873288,873343,873379-873381,"    + \
  "873443,873521,873538-873539,873714-873715,873718,873733,873745,"    + \
  "873751,873767,873778,873781,873849,873856,873862,873914,873940,"    + \
  "873947-873948,873975-873976,873987,873998,874026-874027,874075,"    + \
  "874077-874078,874124-874125,874127,874156,874159,874161,874165,"    + \
  "874168,874170,874184,874189,874204,874223-874224,874245,874258,"    + \
  "874262,874270,874292-874297,874300-874301,874303,874305,874316-"    + \
  "874318,874330,874363,874380,874405,874421,874441,874459,874467,"    + \
  "874473,874497,874506,874545-874546,874561,874566,874568,874580,"    + \
  "874619,874621,874634,874636,874659,874673,874681,874727,874730,"    + \
  "874743,874765-874767,874806,874816,874848,874868,874888,874896,"    + \
  "874909,874912,874996,875051,875069,875129,875132,875134,875137,"    + \
  "875151-875153,875186-875188,875190,875235-875237,875242-875243,"    + \
  "875249,875388,875393,875406,875411\n"

  # Set the 'big' mergeinfo prop on A/B, A/C, and A/D.
  svntest.main.file_write(prop_val_file, big_prop_val)

  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     SVN_PROP_MERGEINFO, '-F', prop_val_file,
                                     B_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     SVN_PROP_MERGEINFO, '-F', prop_val_file,
                                     C_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     SVN_PROP_MERGEINFO, '-F', prop_val_file,
                                     D_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'ps some large svn:mergeinfos', wc_dir)

  # Run propget -vR svn:mergeinfo, redirecting the stdout to a file.
  arglist = [svntest.main.svn_binary, 'propget', SVN_PROP_MERGEINFO, '-vR',
             '--config-dir', svntest.main.default_config_dir, wc_dir]
  redir_file = open(redirect_file, 'wb')
  pg_proc = subprocess.Popen(arglist, stdout=redir_file)
  pg_proc.wait()
  redir_file.close()
  pg_stdout_redir = open(redirect_file, 'r').readlines()

  # Check if the redirected output of svn pg -vR on the root of the WC
  # is what we expect.
  expected_output = [
    "Properties on '" + B_path +  "':\n", # Should ocur only once!
    "Properties on '" + C_path +  "':\n", # Should ocur only once!
    "Properties on '" + D_path +  "':\n", # Should ocur only once!
    # Everything below should appear three times since this same
    # mergeinfo value is set on three paths in the WC.
    "  svn:mergeinfo\n",
    "    /subversion/branches/1.5.x:872364-874936\n",
    "    /subversion/branches/1.5.x-34184:874657-874741\n",
    "    /subversion/branches/1.5.x-34432:874744-874798\n",
    "    /subversion/branches/1.5.x-issue3067:872184-872314\n",
    "    /subversion/branches/1.5.x-issue3157:872165-872175\n",
    "    /subversion/branches/1.5.x-issue3174:872178-872348\n",
    "    /subversion/branches/1.5.x-r30215:870310,870312,870319,870362\n",
    "    /subversion/branches/1.5.x-r30756:874853-874870\n",
    "    /subversion/branches/1.5.x-r30868:870951-870970\n",
    "    /subversion/branches/1.5.x-r31314:874476-874605\n",
    "    /subversion/branches/1.5.x-r31516:871592-871649\n",
    "    /subversion/branches/1.5.x-r32470:872546-872676\n",
    "    /subversion/branches/1.5.x-r32968:873773-873872\n",
    "    /subversion/branches/1.5.x-r33447:873527-873547\n",
    "    /subversion/branches/1.5.x-r33465:873541-873549\n",
    "    /subversion/branches/1.5.x-r33641:873880-873883\n",
    "    /subversion/branches/1.5.x-r34050-followups:874639-874686\n",
    "    /subversion/branches/1.5.x-r34487:874562-874581\n",
    "    /subversion/branches/1.5.x-ra_serf-backports:872354-872626\n",
    "    /subversion/branches/1.5.x-rb-test-fix:874916-874919\n",
    "    /subversion/branches/1.5.x-reintegrate-improvements:874586-874922\n",
    "    /subversion/branches/1.5.x-tests-pass:870925-870973\n",
    "    /subversion/branches/dont-save-plaintext-passwords-by-default:"
    "870728-871118\n",
    "    /subversion/branches/gnome-keyring:870558-871410\n",
    "    /subversion/branches/issue-3220-dev:872210-872226\n",
    "    /subversion/branches/kwallet:870785-871314\n",
    "    /subversion/branches/log-g-performance:870941-871032\n",
    "    /subversion/branches/r30963-1.5.x:871056-871076\n",
    "    /subversion/branches/reintegrate-improvements:873853-874164\n",
    "    /subversion/branches/svn-mergeinfo-enhancements:870196\n",
    "    /subversion/branches/svnpatch-diff:871905\n",
    "    /subversion/trunk:869159-869165,869168-869181,869185,869188,869191,"
    "869200-869201,869203-869207,869209-869224,869227-869238,869240-869244,"
    "869248,869250-869260,869262-869263,869265,869267-869268,869272-869280,"
    "869282-869325,869328-869330,869335,869341-869347,869351,869354-869355,"
    "869358,869361-869377,869379-869381,869383-869417,869419-869422,869432-"
    "869453,869455-869466,869471-869473,869475,869483,869486,869488-869489,"
    "869491-869497,869499-869500,869503,869506-869508,869510-869521,869523-"
    "869540,869542-869552,869556,869558,869560-869561,869563,869565,869567,"
    "869570,869572,869582,869601-869602,869605,869607,869613-869614,869616,"
    "869618,869620,869625,869627,869630,869633,869639,869641-869643,869645-"
    "869652,869655,869657,869665,869668,869674,869677,869681,869685,869687-"
    "869688,869693,869697,869699-869700,869704-869708,869716,869719,869722,"
    "869724,869730,869733-869734,869737-869740,869745-869746,869751-869754,"
    "869766,869812-869813,869815-869818,869820,869825,869837,869841,869843-"
    "869844,869858,869860-869861,869871,869875,869889,869895,869898,869902,"
    "869907,869909,869926,869928-869929,869931-869933,869942-869943,869950,"
    "869952,869957-869958,869969,869972,869974,869988,869994,869996,869999,"
    "870004,870013-870014,870016,870024,870032,870036,870039,870041-870043,"
    "870054,870060,870068-870071,870078,870083,870094,870104,870124,870127-"
    "870128,870133,870135-870136,870141,870144,870148,870160,870172,870175,"
    "870191,870198,870203-870204,870211,870219,870225,870233,870235-870236,"
    "870254-870255,870259,870307,870311,870313,870320,870323,870330-870331,"
    "870352-870353,870355,870359-870360,870371,870373,870378,870393-870395,"
    "870402,870409-870410,870414,870416,870421,870436,870442,870447,870449,"
    "870452,870454,870466,870476,870481-870483,870486,870500,870502,870505,"
    "870513-870518,870522-870523,870527,870529,870534,870536-870538,870540-"
    "870541,870543-870548,870554,870556,870561,870563,870584,870590-870592,"
    "870594-870595,870597,870618,870620,870622,870625-870626,870641,870647,"
    "870657,870665,870671,870681,870702-870703,870706-870708,870717-870718,"
    "870727,870730,870737,870740,870742,870752,870758,870800,870809,870815,"
    "870817,870820-870825,870830,870834-870836,870850-870851,870853,870859,"
    "870861,870886,870894,870916-870918,870942,870945,870957,870962,870970,"
    "870979,870981,870989,870996,871003,871005,871009,871011,871023,871033,"
    "871035-871038,871041,871060,871078,871080,871092,871097,871099,871105,"
    "871107,871120,871123-871127,871130,871133-871135,871140,871149,871155-"
    "871156,871160,871162,871164,871181,871191,871199-871200,871205,871211-"
    "871212,871215,871219,871225,871227,871229,871231,871236,871270,871273,"
    "871277,871283,871297,871302,871306,871308,871315-871320,871323-871325,"
    "871333-871335,871345,871347-871350,871354,871357,871361,871363-871366,"
    "871374-871375,871377,871382,871385-871388,871391,871408,871411,871422,"
    "871435,871441,871443-871444,871465,871470,871472-871476,871481,871489,"
    "871499,871501-871502,871505,871508,871520,871523,871525-871527,871538,"
    "871542,871544,871547-871549,871556,871559,871562-871563,871578,871581,"
    "871587,871589-871597,871608,871613,871616-871617,871620,871624,871649,"
    "871668,871675,871677,871693-871694,871696,871704,871732-871733,871744,"
    "871747,871759,871762,871766,871769,871793,871796,871799,871801,871811,"
    "871813,871821-871826,871831,871843,871860,871880,871891,871894,871899,"
    "871907,871911,871926,871928,871933,871935,871941-871942,871947-871949,"
    "871958,871974,872000-872001,872003,872005,872018,872022,872038,872065,"
    "872068,872086,872091,872093,872097,872103,872112,872130,872154,872157,"
    "872206,872216,872218-872219,872227,872234,872238,872243,872253,872255,"
    "872259,872261,872278-872279,872281,872310-872311,872362,872404,872416-"
    "872417,872429,872431,872434,872439,872450-872453,872468,872470,872477-"
    "872478,872483,872490-872491,872495,872515-872516,872518-872519,872537,"
    "872541,872544,872565,872568,872571-872573,872584,872596-872597,872612,"
    "872619,872624,872632,872656,872670,872706,872710,872713,872717,872746-"
    "872748,872777,872780-872782,872791,872804,872813,872845,872864,872870,"
    "872872,872947-872948,872961,872974,872981,872985-872987,873004,873042,"
    "873049,873051,873076,873087,873090,873096,873098,873100,873183,873186,"
    "873192,873195,873210-873211,873247,873252,873256,873259,873275,873286,"
    "873288,873343,873379-873381,873443,873521,873538-873539,873714-873715,"
    "873718,873733,873745,873751,873767,873778,873781,873849,873856,873862,"
    "873914,873940,873947-873948,873975-873976,873987,873998,874026-874027,"
    "874075,874077-874078,874124-874125,874127,874156,874159,874161,874165,"
    "874168,874170,874184,874189,874204,874223-874224,874245,874258,874262,"
    "874270,874292-874297,874300-874301,874303,874305,874316-874318,874330,"
    "874363,874380,874405,874421,874441,874459,874467,874473,874497,874506,"
    "874545-874546,874561,874566,874568,874580,874619,874621,874634,874636,"
    "874659,874673,874681,874727,874730,874743,874765-874767,874806,874816,"
    "874848,874868,874888,874896,874909,874912,874996,875051,875069,875129,"
    "875132,875134,875137,875151-875153,875186-875188,875190,875235-875237,"
    "875242-875243,875249,875388,875393,875406,875411\n"]
  svntest.verify.verify_outputs(
    "Redirected pg -vR doesn't match pg -vR stdout",
    pg_stdout_redir, None,
    svntest.verify.UnorderedOutput(expected_output), None)
  # Because we are using UnorderedOutput above, this test would spuriously
  # pass if the redirected pg output contained duplicates.  This hasn't been
  # observed as part of issue #3721, but we might as well be thorough...
  #
  # ...Since we've set the same mergeinfo prop on A/B, A/C, and A/D, this
  # means the number of lines in the redirected output of svn pg -vR should
  # be three times the number of lines in EXPECTED_OUTPUT, adjusted for the
  # fact the "Properties on '[A/B | A/C | A/D]'" headers  appear only once.
  if ((len(expected_output) * 3) - 6) != len(pg_stdout_redir):
    raise svntest.Failure("Redirected pg -vR has unexpected duplicates")

@Issue(3852)
def file_matching_dir_prop_reject(sbox):
  "prop conflict for file matching dir prop reject"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add file with awkward name
  svntest.main.file_append(sbox.ospath('A/dir_conflicts'), "some content\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', sbox.ospath('A/dir_conflicts'))
  sbox.simple_propset('prop', 'val1', 'A/dir_conflicts')
  sbox.simple_propset('prop', 'val1', 'A')
  expected_output = svntest.wc.State(wc_dir, {
      'A'               : Item(verb='Sending'),
      'A/dir_conflicts' : Item(verb='Adding'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A', wc_rev=2)
  expected_status.add({
    'A/dir_conflicts' : Item(status='  ', wc_rev=2),
      })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Modify/commit property change
  sbox.simple_propset('prop', 'val2', 'A/dir_conflicts')
  sbox.simple_propset('prop', 'val2', 'A')
  expected_output = svntest.wc.State(wc_dir, {
      'A'               : Item(verb='Sending'),
      'A/dir_conflicts' : Item(verb='Sending'),
      })
  expected_status.tweak('A', 'A/dir_conflicts', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Local property mod
  sbox.simple_propset('prop', 'val3', 'A/dir_conflicts')
  sbox.simple_propset('prop', 'val3', 'A')

  # Update to trigger property conflicts
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/dir_conflicts' : Item('some content\n', props = {'prop' : 'val3'}),
    })
  expected_disk.tweak('A', props={'prop' : 'val3'})
  expected_output = svntest.wc.State(wc_dir, {
    'A'               : Item(status=' C'),
    'A/dir_conflicts' : Item(status=' C'),
    })
  expected_status.tweak(wc_rev=2)
  expected_status.tweak('A', 'A/dir_conflicts', status=' C')

  extra_files = ['dir_conflicts.prej', 'dir_conflicts.2.prej']
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None,
                                        svntest.tree.detect_conflict_files,
                                        extra_files,
                                        None, None, True, '-r', '2', wc_dir)
  if len(extra_files) != 0:
    print("didn't get expected conflict files")
    raise svntest.verify.SVNUnexpectedOutput

  # Revert and update to check that conflict files are removed
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', '-R', wc_dir)
  expected_status.tweak('A', 'A/dir_conflicts', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = svntest.wc.State(wc_dir, {
    'A'               : Item(status=' U'),
    'A/dir_conflicts' : Item(status=' U'),
    })
  expected_disk.tweak('A', 'A/dir_conflicts', props={'prop' : 'val2'})
  expected_status.tweak(wc_rev=3)
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None, True)

def pristine_props_listed(sbox):
  "check if pristine properties are visible"

  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_propset('prop', 'val', 'A')
  sbox.simple_commit()

  expected_output = ["Properties on '" + sbox.ospath('A') + "':\n", "  prop\n"]

  # Now we see the pristine properties
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'proplist', '-R', wc_dir, '-r', 'BASE')

  sbox.simple_propset('prop', 'needs-fix', 'A')

  # And now we see no property at all
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'proplist', '-R', wc_dir, '-r', 'BASE')

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              make_local_props,
              commit_props,
              update_props,
              downdate_props,
              remove_props,
              update_conflict_props,
              commit_conflict_dirprops,
              commit_replacement_props,
              revert_replacement_props,
              inappropriate_props,
              copy_inherits_special_props,
              revprop_change,
              prop_value_conversions,
              binary_props,
              recursive_base_wc_ops,
              url_props_ops,
              removal_schedule_added_props,
              update_props_on_wc_root,
              props_on_replaced_file,
              depthy_wc_proplist,
              depthy_url_proplist,
              invalid_propnames,
              perms_on_symlink,
              remove_custom_ns_props,
              props_over_time,
              invalid_propvalues,
              same_replacement_props,
              added_moved_file,
              delete_nonexistent_property,
              post_revprop_change_hook,
              rm_of_replaced_file,
              prop_reject_grind,
              obstructed_subdirs,
              atomic_over_ra,
              propget_redirection,
              file_matching_dir_prop_reject,
              pristine_props_listed,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
