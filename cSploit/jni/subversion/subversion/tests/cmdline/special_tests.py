#!/usr/bin/env python
#
#  special_tests.py:  testing special and reserved file handling
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
import sys, os, re

# Our testing module
import svntest

from svntest.main import server_has_mergeinfo

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_posix_os)
def general_symlink(sbox):
  "general symlink handling"

  sbox.build()
  wc_dir = sbox.wc_dir

  # First try to just commit a symlink
  newfile_path = os.path.join(wc_dir, 'newfile')
  linktarget_path = os.path.join(wc_dir, 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', newfile_path)
  svntest.main.run_svn(None, 'add', newfile_path, linktarget_path)

  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    'linktarget' : Item(verb='Adding'),
    })

  # Run a diff and verify that we get the correct output
  exit_code, stdout_lines, stderr_lines = svntest.main.run_svn(1, 'diff',
                                                               wc_dir)

  regex = '^\+link linktarget'
  for line in stdout_lines:
    if re.match(regex, line):
      break
  else:
    raise svntest.Failure

  # Commit and make sure everything is good
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev=2),
    'linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None,
                                        wc_dir)

  ## Now we should update to the previous version, verify that no
  ## symlink is present, then update back to HEAD and see if the symlink
  ## is regenerated properly.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r', '1', wc_dir)

  # Is the symlink gone?
  if os.path.isfile(newfile_path) or os.path.islink(newfile_path):
    raise svntest.Failure

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r', '2', wc_dir)

  # Is the symlink back?
  new_target = os.readlink(newfile_path)
  if new_target != 'linktarget':
    raise svntest.Failure

  ## Now change the target of the symlink, verify that it is shown as
  ## modified and that a commit succeeds.
  os.remove(newfile_path)
  os.symlink('A', newfile_path)

  was_cwd = os.getcwd()
  os.chdir(wc_dir)
  svntest.actions.run_and_verify_svn(None, [ "M       newfile\n" ], [], 'st')

  os.chdir(was_cwd)

  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev=3),
    'linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)


@SkipUnless(svntest.main.is_posix_os)
def replace_file_with_symlink(sbox):
  "replace a normal file with a special file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # First replace a normal file with a symlink and make sure we get an
  # error
  iota_path = os.path.join(wc_dir, 'iota')
  os.remove(iota_path)
  os.symlink('A', iota_path)

  # Does status show the obstruction?
  was_cwd = os.getcwd()
  os.chdir(wc_dir)
  svntest.actions.run_and_verify_svn(None, [ "~       iota\n" ], [], 'st')

  # And does a commit fail?
  os.chdir(was_cwd)
  exit_code, stdout_lines, stderr_lines = svntest.main.run_svn(1, 'ci', '-m',
                                                               'log msg',
                                                               wc_dir)

  regex = 'svn: E145001: Commit failed'
  for line in stderr_lines:
    if re.match(regex, line):
      break
  else:
    raise svntest.Failure


@SkipUnless(svntest.main.is_posix_os)
def import_export_symlink(sbox):
  "import and export a symlink"

  sbox.build()
  wc_dir = sbox.wc_dir

  # create a new symlink to import
  new_path = os.path.join(wc_dir, 'new_file')

  os.symlink('linktarget', new_path)

  # import this symlink into the repository
  url = sbox.repo_url + "/dirA/dirB/new_link"
  exit_code, output, errput = svntest.actions.run_and_verify_svn(
    'Import a symlink', None, [], 'import',
    '-m', 'log msg', new_path, url)

  regex = "(Committed|Imported) revision [0-9]+."
  for line in output:
    if re.match(regex, line):
      break
  else:
    raise svntest.Failure

  # remove the unversioned link
  os.remove(new_path)

  # run update and verify that the symlink is put back into place
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', wc_dir)

  # Is the symlink back?
  link_path = wc_dir + "/dirA/dirB/new_link"
  new_target = os.readlink(link_path)
  if new_target != 'linktarget':
    raise svntest.Failure

  ## Now we will try exporting from both the working copy and the
  ## repository directly, verifying that the symlink is created in
  ## both cases.

  for export_src, dest_dir in [(sbox.wc_dir, 'export-wc'),
                               (sbox.repo_url, 'export-url')]:
    export_target = sbox.add_wc_path(dest_dir)
    svntest.actions.run_and_verify_svn(None, None, [],
                                       'export', export_src, export_target)

    # is the link at the correct place?
    link_path = os.path.join(export_target, "dirA/dirB/new_link")
    new_target = os.readlink(link_path)
    if new_target != 'linktarget':
      raise svntest.Failure


#----------------------------------------------------------------------
# Regression test for issue 1986
@Issue(1986)
@SkipUnless(svntest.main.is_posix_os)
def copy_tree_with_symlink(sbox):
  "'svn cp dir1 dir2' which contains a symlink"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a versioned symlink within directory 'A/D/H'.
  newfile_path = os.path.join(wc_dir, 'A', 'D', 'H', 'newfile')
  linktarget_path = os.path.join(wc_dir, 'A', 'D', 'H', 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', newfile_path)
  svntest.main.run_svn(None, 'add', newfile_path, linktarget_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/H/newfile' : Item(verb='Adding'),
    'A/D/H/linktarget' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/D/H/newfile' : Item(status='  ', wc_rev=2),
    'A/D/H/linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  # Copy H to H2
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  H2_path = os.path.join(wc_dir, 'A', 'D', 'H2')
  svntest.actions.run_and_verify_svn(None, None, [], 'cp', H_path, H2_path)

  # 'svn status' should show just "A/D/H2  A +".  Nothing broken.
  expected_status.add({
    'A/D/H2' : Item(status='A ', copied='+', wc_rev='-'),
    'A/D/H2/chi' : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/H2/omega' : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/H2/psi' : Item(status='  ', copied='+', wc_rev='-'),
    # linktarget and newfile are from r2, while h2 is from r1.
    'A/D/H2/linktarget' : Item(status='A ', copied='+', wc_rev='-',
                               entry_status='  '),
    'A/D/H2/newfile' : Item(status='A ', copied='+', wc_rev='-',
                            entry_status='  '),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


@SkipUnless(svntest.main.is_posix_os)
def replace_symlink_with_file(sbox):
  "replace a special file with a non-special file"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new special file and commit it.
  newfile_path = os.path.join(wc_dir, 'newfile')
  linktarget_path = os.path.join(wc_dir, 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', newfile_path)
  svntest.main.run_svn(None, 'add', newfile_path, linktarget_path)

  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    'linktarget' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev=2),
    'linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)


  # Now replace the symlink with a normal file and try to commit, we
  # should get an error
  os.remove(newfile_path);
  svntest.main.file_append(newfile_path, "text of actual file");

  # Does status show the obstruction?
  was_cwd = os.getcwd()
  os.chdir(wc_dir)
  svntest.actions.run_and_verify_svn(None, [ "~       newfile\n" ], [], 'st')

  # And does a commit fail?
  os.chdir(was_cwd)
  exit_code, stdout_lines, stderr_lines = svntest.main.run_svn(1, 'ci', '-m',
                                                               'log msg',
                                                               wc_dir)

  regex = 'svn: E145001: Commit failed'
  for line in stderr_lines:
    if re.match(regex, line):
      break
  else:
    raise svntest.Failure


@SkipUnless(svntest.main.is_posix_os)
def remove_symlink(sbox):
  "remove a symlink"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Commit a symlink
  newfile_path = os.path.join(wc_dir, 'newfile')
  linktarget_path = os.path.join(wc_dir, 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', newfile_path)
  svntest.main.run_svn(None, 'add', newfile_path, linktarget_path)

  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    'linktarget' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev=2),
    'linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Now remove it
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', newfile_path)

  # Commit and verify that it worked
  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Deleting'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'linktarget' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

@SkipUnless(svntest.main.is_posix_os)
@SkipUnless(server_has_mergeinfo)
@Issue(2530)
def merge_symlink_into_file(sbox):
  "merge symlink into file"

  sbox.build()
  wc_dir = sbox.wc_dir
  d_url = sbox.repo_url + '/A/D'
  dprime_url = sbox.repo_url + '/A/Dprime'

  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  gamma_prime_path = os.path.join(wc_dir, 'A', 'Dprime', 'gamma')

  # create a copy of the D directory to play with
  svntest.main.run_svn(None,
                       'copy', d_url, dprime_url, '-m', 'copy')
  svntest.main.run_svn(None,
                       'update', sbox.wc_dir)

  # remove A/Dprime/gamma
  svntest.main.run_svn(None, 'delete', gamma_prime_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/Dprime/gamma' : Item(verb='Deleting'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)

  # Commit a symlink in its place
  linktarget_path = os.path.join(wc_dir, 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', gamma_prime_path)
  svntest.main.run_svn(None, 'add', gamma_prime_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/Dprime/gamma' : Item(verb='Adding'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)

  # merge the creation of the symlink into the original directory
  svntest.main.run_svn(None,
                       'merge', '-r', '2:4', dprime_url,
                       os.path.join(wc_dir, 'A', 'D'))

  # now revert, we once got a strange error
  svntest.main.run_svn(None, 'revert', '-R', wc_dir)

  # assuming we got past the revert because someone fixed that bug, lets
  # try the merge and a commit, since that apparently used to throw us for
  # a loop, see issue 2530
  svntest.main.run_svn(None,
                       'merge', '-r', '2:4', dprime_url,
                       os.path.join(wc_dir, 'A', 'D'))

  expected_output = svntest.wc.State(wc_dir, {
    'A/D'       : Item(verb='Sending'),
    'A/D/gamma' : Item(verb='Replacing'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)



@SkipUnless(svntest.main.is_posix_os)
def merge_file_into_symlink(sbox):
  "merge file into symlink"

  sbox.build()
  wc_dir = sbox.wc_dir
  d_url = sbox.repo_url + '/A/D'
  dprime_url = sbox.repo_url + '/A/Dprime'

  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  gamma_prime_path = os.path.join(wc_dir, 'A', 'Dprime', 'gamma')

  # create a copy of the D directory to play with
  svntest.main.run_svn(None,
                       'copy', d_url, dprime_url, '-m', 'copy')
  svntest.main.run_svn(None,
                       'update', sbox.wc_dir)

  # remove A/Dprime/gamma
  svntest.main.run_svn(None, 'delete', gamma_prime_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/Dprime/gamma' : Item(verb='Deleting'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)

  # Commit a symlink in its place
  linktarget_path = os.path.join(wc_dir, 'linktarget')
  svntest.main.file_append(linktarget_path, 'this is just a link target')
  os.symlink('linktarget', gamma_prime_path)
  svntest.main.run_svn(None, 'add', gamma_prime_path)

  expected_output = svntest.wc.State(wc_dir, {
    'A/Dprime/gamma' : Item(verb='Adding'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)

  svntest.main.file_write(gamma_path, 'changed file', 'w+')

  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma' : Item(verb='Sending'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output, None, None,
                                        wc_dir)

  # ok, now merge the change to the file into the symlink we created, this
  # gives us a weird error
  svntest.main.run_svn(None,
                       'merge', '-r', '4:5', '--allow-mixed-revisions', d_url,
                       os.path.join(wc_dir, 'A', 'Dprime'))

# Issue 2701: Tests to see repository with symlinks can be checked out on all
# platforms.
@Issue(2701)
def checkout_repo_with_symlinks(sbox):
  "checkout a repository containing symlinks"

  svntest.actions.load_repo(sbox, os.path.join(os.path.dirname(sys.argv[0]),
                                               'special_tests_data',
                                               'symlink.dump'))

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'from': Item(status='A '),
    'to': Item(status='A '),
    })

  if svntest.main.is_os_windows():
    expected_link_contents = 'link to'
  else:
    expected_link_contents = ''

  expected_wc = svntest.wc.State('', {
    'from' : Item(contents=expected_link_contents),
    'to'   : Item(contents=''),
    })
  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          sbox.wc_dir,
                                          expected_output,
                                          expected_wc)

# Issue 2716: 'svn diff' against a symlink to a directory within the wc
@Issue(2716)
@SkipUnless(svntest.main.is_posix_os)
def diff_symlink_to_dir(sbox):
  "diff a symlink to a directory"

  sbox.build(read_only = True)
  os.chdir(sbox.wc_dir)

  # Create a symlink to A/D/.
  d_path = os.path.join('A', 'D')
  link_path = 'link'
  os.symlink(d_path, link_path)

  # Add the symlink.
  svntest.main.run_svn(None, 'add', link_path)

  # Now diff the wc itself and check the results.
  expected_output = [
    "Index: link\n",
    "===================================================================\n",
    "--- link\t(revision 0)\n",
    "+++ link\t(working copy)\n",
    "@@ -0,0 +1 @@\n",
    "+link " + d_path + "\n",
    "\ No newline at end of file\n",
    "\n",
    "Property changes on: link\n",
    "___________________________________________________________________\n",
    "Added: svn:special\n",
    "## -0,0 +1 ##\n",
    "+*\n",
    "\\ No newline at end of property\n"
  ]
  svntest.actions.run_and_verify_svn(None, expected_output, [], 'diff',
                                     '.')
  # We should get the same output if we the diff the symlink itself.
  svntest.actions.run_and_verify_svn(None, expected_output, [], 'diff',
                                     link_path)

# Issue 2692 (part of): Check that the client can check out a repository
# that contains an unknown special file type.
@Issue(2692)
def checkout_repo_with_unknown_special_type(sbox):
  "checkout repository with unknown special file type"

  svntest.actions.load_repo(sbox, os.path.join(os.path.dirname(sys.argv[0]),
                                               'special_tests_data',
                                               'bad-special-type.dump'))

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'special': Item(status='A '),
    })
  expected_wc = svntest.wc.State('', {
    'special' : Item(contents='gimble wabe'),
    })
  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          sbox.wc_dir,
                                          expected_output,
                                          expected_wc)

def replace_symlink_with_dir(sbox):
  "replace a special file with a directory"

  svntest.actions.load_repo(sbox, os.path.join(os.path.dirname(sys.argv[0]),
                                               'special_tests_data',
                                               'symlink.dump'))

  wc_dir = sbox.wc_dir
  from_path = os.path.join(wc_dir, 'from')

  # Now replace the symlink with a directory and try to commit, we
  # should get an error
  os.remove(from_path);
  os.mkdir(from_path);

  # Does status show the obstruction?
  was_cwd = os.getcwd()
  os.chdir(wc_dir)
  svntest.actions.run_and_verify_svn(None, [ "~       from\n" ], [], 'st')

  # The commit shouldn't do anything.
  # I'd expect a failed commit here, but replacing a file locally with a
  # directory seems to make svn think the file is unchanged.
  os.chdir(was_cwd)
  expected_output = svntest.wc.State(wc_dir, {
  })

  if svntest.main.is_posix_os():
    error_re_string = '.*E145001: Entry.*has unexpectedly changed special.*'
  else:
    error_re_string = None
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, error_re_string, wc_dir)

# test for issue #1808: svn up deletes local symlink that obstructs
# versioned file
@Issue(1808)
@SkipUnless(svntest.main.is_posix_os)
def update_obstructing_symlink(sbox):
  "symlink obstructs incoming delete"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  mu_url = sbox.repo_url + '/A/mu'
  iota_path = os.path.join(wc_dir, 'iota')

  # delete A/mu and replace it with a symlink
  svntest.main.run_svn(None, 'rm', mu_path)
  os.symlink(iota_path, mu_path)

  svntest.main.run_svn(None, 'rm', mu_url,
                       '-m', 'log msg')

  svntest.main.run_svn(None,
                       'up', wc_dir)

  # check that the symlink is still there
  target = os.readlink(mu_path)
  if target != iota_path:
    raise svntest.Failure


def warn_on_reserved_name(sbox):
  "warn when attempt operation on a reserved name"
  sbox.build()
  reserved_path = os.path.join(sbox.wc_dir, svntest.main.get_admin_name())
  svntest.actions.run_and_verify_svn(
    "Locking a file with a reserved name failed to result in an error",
    None,
    ".*Skipping argument: E200025: '.+' ends in a reserved name.*",
    'lock', reserved_path)


def propvalue_normalized(sbox):
  "'ps svn:special' should normalize to '*'"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a "symlink"
  iota2_path = sbox.ospath('iota2')
  svntest.main.file_write(iota2_path, "symlink destination")
  svntest.main.run_svn(None, 'add', iota2_path)
  svntest.main.run_svn(None, 'propset', 'svn:special', 'yes', iota2_path)
  if svntest.main.is_posix_os():
    os.remove(iota2_path)
    os.symlink("symlink destination", iota2_path)

  # Property value should be SVN_PROP_BOOLEAN_TRUE
  expected_propval = ['*']
  svntest.actions.run_and_verify_svn(None, expected_propval, [],
                                     'propget', '--strict', 'svn:special',
                                     iota2_path)

  # Commit and check again.
  expected_output = svntest.wc.State(wc_dir, {
    'iota2' : Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota2' : Item(status='  ', wc_rev=2),
    })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None,
                                        wc_dir)

  svntest.main.run_svn(None, 'update', wc_dir)
  svntest.actions.run_and_verify_svn(None, expected_propval, [],
                                     'propget', '--strict', 'svn:special',
                                     iota2_path)


# on users@: http://mid.gmane.org/1292856447.8650.24.camel@nimble.325Bayport
@SkipUnless(svntest.main.is_posix_os)
def unrelated_changed_special_status(sbox):
  "commit foo while bar changed special status"

  sbox.build()
  wc_dir = sbox.wc_dir

  os.chdir(os.path.join(sbox.wc_dir, 'A/D/H'))

  open('chi', 'a').write('random local mod')
  os.unlink('psi')
  os.symlink('omega', 'psi') # omega is versioned!
  svntest.main.run_svn(None, 'changelist', 'chi cl', 'chi')
  svntest.actions.run_and_verify_svn(None, None, [], 'commit',
                                     '--changelist', 'chi cl',
                                     '-m', 'psi changed special status')


@Issue(3972)
@SkipUnless(svntest.main.is_posix_os)
def symlink_destination_change(sbox):
  "revert a symlink destination change"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Create a new symlink and commit it.
  newfile_path = os.path.join(wc_dir, 'newfile')
  os.symlink('linktarget', newfile_path)
  svntest.main.run_svn(None, 'add', newfile_path)

  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Modify the symlink to point somewhere else
  os.remove(newfile_path)
  os.symlink('linktarget2', newfile_path)

  expected_status.tweak('newfile', status='M ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Revert should restore the symlink to point to the original destination
  svntest.main.run_svn(None, 'revert', '-R', wc_dir)
  expected_status.tweak('newfile', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Issue 3972, repeat revert produces no output
  svntest.actions.run_and_verify_svn(None, [], [], 'revert', '-R', wc_dir)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Now replace the symlink with a normal file and try to commit, we

#----------------------------------------------------------------------
# This used to lose the special status in the target working copy
# (disk and metadata).
@Issue(3884)
@SkipUnless(svntest.main.is_posix_os)
@XFail()
def merge_foreign_symlink(sbox):
  "merge symlink-add from foreign repos"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a copy of this repository and associated working copy.  Both
  # should have nothing but a Greek tree in them, and the two
  # repository UUIDs should differ.
  sbox2 = sbox.clone_dependent(True)
  sbox2.build()
  wc_dir2 = sbox2.wc_dir

  # convenience variables
  zeta_path = sbox.ospath('A/zeta')
  zeta2_path = sbox2.ospath('A/zeta')

  # sbox2 r2: create zeta2 in sbox2
  os.symlink('target', zeta2_path)
  sbox2.simple_add('A/zeta')
  sbox2.simple_commit('A/zeta')


  # sbox1: merge that
  svntest.main.run_svn(None, 'merge', '-c', '2', sbox2.repo_url,
                       sbox.ospath(''))

  # Verify special status.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/zeta': Item(props={ 'svn:special': '*' })
  })
  svntest.actions.verify_disk(sbox.ospath(''), expected_disk, True)

  # TODO: verify status:
  #   expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  #   expected_status.add({
  #     'A/zeta' : Item(status='A ', wc_rev='-', props={'svn:special': '*'}),
  #     })

#----------------------------------------------------------------------
# See also symlink_to_wc_svnversion().
@Issue(2557,3987)
@SkipUnless(svntest.main.is_posix_os)
def symlink_to_wc_basic(sbox):
  "operate on symlink to wc"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create a symlink
  symlink_path = sbox.add_wc_path('2')
  assert not os.path.islink(symlink_path)
  os.symlink(os.path.basename(wc_dir), symlink_path) ### implementation detail
  symlink_basename = os.path.basename(symlink_path)

  # Some basic tests
  wc_uuid = svntest.actions.get_wc_uuid(wc_dir)
  expected_info = [{
      'Path' : re.escape(os.path.join(symlink_path)),
      'Working Copy Root Path' : re.escape(os.path.abspath(symlink_path)),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : wc_uuid,
      'Revision' : '1',
      'Node Kind' : 'directory',
      'Schedule' : 'normal',
  }, {
      'Name' : 'iota',
      'Path' : re.escape(os.path.join(symlink_path, 'iota')),
      'Working Copy Root Path' : re.escape(os.path.abspath(symlink_path)),
      'Repository Root' : sbox.repo_url,
      'Repository UUID' : wc_uuid,
      'Revision' : '1',
      'Node Kind' : 'file',
      'Schedule' : 'normal',
  }]
  svntest.actions.run_and_verify_info(expected_info,
                                      symlink_path, symlink_path + '/iota')

#----------------------------------------------------------------------
# Similar to #2557/#3987; see symlink_to_wc_basic().
@Issue(2557,3987)
@SkipUnless(svntest.main.is_posix_os)
def symlink_to_wc_svnversion(sbox):
  "svnversion on symlink to wc"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  # Create a symlink
  symlink_path = sbox.add_wc_path('2')
  assert not os.path.islink(symlink_path)
  os.symlink(os.path.basename(wc_dir), symlink_path) ### implementation detail
  symlink_basename = os.path.basename(symlink_path)

  # Some basic tests
  svntest.actions.run_and_verify_svnversion("Unmodified symlink to wc",
                                            symlink_path, sbox.repo_url,
                                            [ "1\n" ], [])

# Regression in 1.7.0: Update fails to change a symlink
@SkipUnless(svntest.main.is_posix_os)
def update_symlink(sbox):
  "update a symlink"

  svntest.actions.do_sleep_for_timestamps()

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = sbox.ospath('A/mu')
  iota_path = sbox.ospath('iota')
  symlink_path = sbox.ospath('symlink')

  # create a symlink to /A/mu
  os.symlink("A/mu", symlink_path)
  sbox.simple_add('symlink')
  sbox.simple_commit()

  # change the symlink to /iota
  os.remove(symlink_path)
  os.symlink("iota", symlink_path)
  sbox.simple_commit()

  # update back to r2
  svntest.main.run_svn(False, 'update', '-r', '2', wc_dir)

  # now update to head; 1.7.0 throws an assertion here
  expected_output = svntest.wc.State(wc_dir, {
    'symlink'          : Item(status='U '),
  })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'symlink': Item(contents="This is the file 'iota'.\n",
                                     props={'svn:special' : '*'})})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'symlink'           : Item(status='  ', wc_rev='3'),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1)

@Issue(4102)
@SkipUnless(svntest.main.is_posix_os)
def externals_as_symlink_targets(sbox):
  "externals as symlink targets"
  sbox.build()
  wc = sbox.ospath

  # Control: symlink to normal dir and file.
  os.symlink('E', wc('sym_E'))
  os.symlink('mu', wc('sym_mu'))

  # Test case: symlink to external dir and file.
  sbox.simple_propset("svn:externals",
                      '^/A/B/E ext_E\n'
                      '^/A/mu ext_mu',
                      '')
  sbox.simple_update()
  os.symlink('ext_E', wc('sym_ext_E'))
  os.symlink('ext_mu', wc('sym_ext_mu'))

  # Adding symlinks to normal items and to a file external is OK.
  sbox.simple_add('sym_E', 'sym_mu', 'sym_ext_mu')

  ### Adding a symlink to an external dir failed with
  ###   svn: E200009: Could not add all targets because some targets are
  ###   already versioned
  sbox.simple_add('sym_ext_E')

  sbox.simple_commit()
    

#----------------------------------------------------------------------
def incoming_symlink_changes(sbox):
  "verify incoming symlink change behavior"

  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_add_symlink('iota', 's-replace')
  sbox.simple_add_symlink('iota', 's-in-place')
  sbox.simple_add_symlink('iota', 's-type')
  sbox.simple_append('s-reverse', 'link iota')
  sbox.simple_add('s-reverse')
  sbox.simple_commit() # r2

  # Replace s-replace
  sbox.simple_rm('s-replace')
  # Note that we don't use 'A/mu' as the length of that matches 'iota', which
  # would make us depend on timestamp changes for detecting differences.
  sbox.simple_add_symlink('A/D/G/pi', 's-replace')

  # Change target of s-in-place
  if svntest.main.is_posix_os():
    os.remove(sbox.ospath('s-in-place'))
    os.symlink('A/D/G/pi', sbox.ospath('s-in-place'))
  else:
    sbox.simple_append('s-in-place', 'link A/D/G/pi', truncate = True)

  # r3
  expected_output = svntest.wc.State(wc_dir, {
    's-replace'         : Item(verb='Replacing'),
    's-in-place'        : Item(verb='Sending'),
  })
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, None, None,
                                        wc_dir)

  # r4
  svntest.main.run_svnmucc('propdel', 'svn:special',
                           sbox.repo_url + '/s-type',
                           '-m', 'Turn s-type into a file')

  # r5
  svntest.main.run_svnmucc('propset', 'svn:special', 'X',
                           sbox.repo_url + '/s-reverse',
                           '-m', 'Turn s-reverse into a symlink')

  # Currently we expect to see 'U'pdates, but we would like to see
  # replacements
  expected_output = svntest.wc.State(wc_dir, {
    's-reverse'         : Item(status=' U'),
    's-type'            : Item(status=' U'),
  })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 5)
  expected_status.add({
    's-type'            : Item(status='  ', wc_rev='5'),
    's-replace'         : Item(status='  ', wc_rev='5'),
    's-reverse'         : Item(status='  ', wc_rev='5'),
    's-in-place'        : Item(status='  ', wc_rev='5'),
  })

  # Update to HEAD/r5 to fetch the r4 and r5 symlink changes
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        expected_status,
                                        None, None, None, None, None,
                                        check_props=True)

  # Update back to r2, to prepare some local changes
  expected_output = svntest.wc.State(wc_dir, {
    # s-replace is D + A
    's-replace'         : Item(status='A '),
    's-in-place'        : Item(status='U '),
    's-reverse'         : Item(status=' U'),
    's-type'            : Item(status=' U'),
  })
  expected_status.tweak(wc_rev=2)

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        expected_status,
                                        None, None, None, None, None,
                                        True,
                                        wc_dir, '-r', '2')

  # Ok, now add a property on all of them to make future symlinkness changes
  # a tree conflict
  # ### We should also try this with a 'textual change'
  sbox.simple_propset('x', 'y', 's-replace', 's-in-place', 's-reverse', 's-type')

  expected_output = svntest.wc.State(wc_dir, {
    's-replace'         : Item(status='  ', treeconflict='A'),
    's-in-place'        : Item(status='U '),
    's-reverse'         : Item(status='  ', treeconflict='C'),
    's-type'            : Item(status='  ', treeconflict='C'),
  })
  expected_status.tweak(wc_rev=5)
  expected_status.tweak('s-replace', 's-reverse', 's-type', status='RM',
                        copied='+', treeconflict='C', wc_rev='-')
  expected_status.tweak('s-in-place', status=' M')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              general_symlink,
              replace_file_with_symlink,
              import_export_symlink,
              copy_tree_with_symlink,
              replace_symlink_with_file,
              remove_symlink,
              merge_symlink_into_file,
              merge_file_into_symlink,
              checkout_repo_with_symlinks,
              diff_symlink_to_dir,
              checkout_repo_with_unknown_special_type,
              replace_symlink_with_dir,
              update_obstructing_symlink,
              warn_on_reserved_name,
              propvalue_normalized,
              unrelated_changed_special_status,
              symlink_destination_change,
              merge_foreign_symlink,
              symlink_to_wc_basic,
              symlink_to_wc_svnversion,
              update_symlink,
              externals_as_symlink_targets,
              incoming_symlink_changes,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
