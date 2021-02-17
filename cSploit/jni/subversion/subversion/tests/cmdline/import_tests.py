#!/usr/bin/env python
#
#  import_tests.py:  import tests
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
import re, os.path

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
exp_noop_up_out = svntest.actions.expected_noop_update_output

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

#----------------------------------------------------------------------
# this test should be SKIPped on systems without the executable bit
@SkipUnless(svntest.main.is_posix_os)
def import_executable(sbox):
  "import of executable files"

  sbox.build()
  wc_dir = sbox.wc_dir

  # create a new directory with files of various permissions
  xt_path = os.path.join(wc_dir, "XT")
  os.makedirs(xt_path)
  all_path = os.path.join(wc_dir, "XT/all_exe")
  none_path = os.path.join(wc_dir, "XT/none_exe")
  user_path = os.path.join(wc_dir, "XT/user_exe")
  group_path = os.path.join(wc_dir, "XT/group_exe")
  other_path = os.path.join(wc_dir, "XT/other_exe")

  for path in [all_path, none_path, user_path, group_path, other_path]:
    svntest.main.file_append(path, "some text")

  # set executable bits
  os.chmod(all_path, 0777)
  os.chmod(none_path, 0666)
  os.chmod(user_path, 0766)
  os.chmod(group_path, 0676)
  os.chmod(other_path, 0667)

  # import new files into repository
  url = sbox.repo_url
  exit_code, output, errput =   svntest.actions.run_and_verify_svn(
    None, None, [], 'import',
    '-m', 'Log message for new import', xt_path, url)

  lastline = output.pop().strip()
  cm = re.compile ("(Committed|Imported) revision [0-9]+.")
  match = cm.search (lastline)
  if not match:
    ### we should raise a less generic error here. which?
    raise svntest.Failure

  # remove (uncontrolled) local files
  svntest.main.safe_rmtree(xt_path)

  # Create expected disk tree for the update (disregarding props)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'all_exe' :   Item('some text', props={'svn:executable' : ''}),
    'none_exe' :  Item('some text'),
    'user_exe' :  Item('some text', props={'svn:executable' : ''}),
    'group_exe' : Item('some text'),
    'other_exe' : Item('some text'),
    })

  # Create expected status tree for the update (disregarding props).
  # Newly imported file should be at revision 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'all_exe' : Item(status='  ', wc_rev=2),
    'none_exe' : Item(status='  ', wc_rev=2),
    'user_exe' : Item(status='  ', wc_rev=2),
    'group_exe' : Item(status='  ', wc_rev=2),
    'other_exe' : Item(status='  ', wc_rev=2),
    })

  # Create expected output tree for the update.
  expected_output = svntest.wc.State(wc_dir, {
    'all_exe' : Item(status='A '),
    'none_exe' : Item(status='A '),
    'user_exe' : Item(status='A '),
    'group_exe' : Item(status='A '),
    'other_exe' : Item(status='A '),
  })
  # do update and check three ways
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1)

#----------------------------------------------------------------------
def import_ignores(sbox):
  'do not import ignored files in imported dirs'

  # The bug was that
  #
  #   $ svn import dir
  #
  # where dir contains some items that match the ignore list and some
  # do not would add all items, ignored or not.
  #
  # This has been fixed by testing each item with the new
  # svn_wc_is_ignored function.

  sbox.build()
  wc_dir = sbox.wc_dir

  dir_path = os.path.join(wc_dir, 'dir')
  foo_c_path = os.path.join(dir_path, 'foo.c')
  foo_o_path = os.path.join(dir_path, 'foo.o')

  os.mkdir(dir_path, 0755)
  open(foo_c_path, 'w')
  open(foo_o_path, 'w')

  # import new dir into repository
  url = sbox.repo_url + '/dir'

  exit_code, output, errput = svntest.actions.run_and_verify_svn(
    None, None, [], 'import',
    '-m', 'Log message for new import',
    dir_path, url)

  lastline = output.pop().strip()
  cm = re.compile ("(Committed|Imported) revision [0-9]+.")
  match = cm.search (lastline)
  if not match:
    ### we should raise a less generic error here. which?
    raise svntest.verify.SVNUnexpectedOutput

  # remove (uncontrolled) local dir
  svntest.main.safe_rmtree(dir_path)

  # Create expected disk tree for the update (disregarding props)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'dir/foo.c' : Item(''),
    })

  # Create expected status tree for the update (disregarding props).
  # Newly imported file should be at revision 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'dir' : Item(status='  ', wc_rev=2),
    'dir/foo.c' : Item(status='  ', wc_rev=2),
    })

  # Create expected output tree for the update.
  expected_output = svntest.wc.State(wc_dir, {
    'dir' : Item(status='A '),
    'dir/foo.c' : Item(status='A '),
  })

  # do update and check three ways
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1)

#----------------------------------------------------------------------
def import_no_ignores(sbox):
  'import ignored files in imported dirs'

  # import ignored files using the "--no-ignore" option

  sbox.build()
  wc_dir = sbox.wc_dir

  dir_path = os.path.join(wc_dir, 'dir')
  foo_c_path = os.path.join(dir_path, 'foo.c')
  foo_o_path = os.path.join(dir_path, 'foo.o')
  foo_lo_path = os.path.join(dir_path, 'foo.lo')
  foo_rej_path = os.path.join(dir_path, 'foo.rej')

  os.mkdir(dir_path, 0755)
  open(foo_c_path, 'w')
  open(foo_o_path, 'w')
  open(foo_lo_path, 'w')
  open(foo_rej_path, 'w')

  # import new dir into repository
  url = sbox.repo_url + '/dir'

  exit_code, output, errput = svntest.actions.run_and_verify_svn(
    None, None, [], 'import',
    '-m', 'Log message for new import', '--no-ignore',
    dir_path, url)

  lastline = output.pop().strip()
  cm = re.compile ("(Committed|Imported) revision [0-9]+.")
  match = cm.search (lastline)
  if not match:
    raise svntest.Failure

  # remove (uncontrolled) local dir
  svntest.main.safe_rmtree(dir_path)

  # Create expected disk tree for the update (disregarding props)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'dir/foo.c' : Item(''),
    'dir/foo.o' : Item(''),
    'dir/foo.lo' : Item(''),
    'dir/foo.rej' : Item(''),
    })

  # Create expected status tree for the update (disregarding props).
  # Newly imported file should be at revision 2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'dir' : Item(status='  ', wc_rev=2),
    'dir/foo.c' : Item(status='  ', wc_rev=2),
    'dir/foo.o' : Item(status='  ', wc_rev=2),
    'dir/foo.lo' : Item(status='  ', wc_rev=2),
    'dir/foo.rej' : Item(status='  ', wc_rev=2),
    })

  # Create expected output tree for the update.
  expected_output = svntest.wc.State(wc_dir, {
    'dir' : Item(status='A '),
    'dir/foo.c' : Item(status='A '),
    'dir/foo.o' : Item(status='A '),
    'dir/foo.lo' : Item(status='A '),
    'dir/foo.rej' : Item(status='A '),
    })

  # do update and check three ways
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1)
#----------------------------------------------------------------------
def import_avoid_empty_revision(sbox):
  "avoid creating empty revisions with import"

  sbox.build()
  wc_dir = sbox.wc_dir

  # create a new directory
  empty_dir = os.path.join(wc_dir, "empty_dir")
  os.makedirs(empty_dir)

  url = sbox.repo_url
  svntest.actions.run_and_verify_svn(None, None, [], 'import',
                                     '-m', 'Log message for new import',
                                     empty_dir, url)

  svntest.main.safe_rmtree(empty_dir)

  # Verify that an empty revision has not been created
  svntest.actions.run_and_verify_svn(None,
                                     exp_noop_up_out(1),
                                     [], "update",
                                     empty_dir)
#----------------------------------------------------------------------

# test for issue 2433: "import" does not handle eol-style correctly
@Issue(2433)
def import_eol_style(sbox):
  "import should honor the eol-style property"

  sbox.build()
  os.chdir(sbox.wc_dir)

  # setup a custom config, we need autoprops
  config_contents = '''\
[auth]
password-stores =

[miscellany]
enable-auto-props = yes

[auto-props]
*.dsp = svn:eol-style=CRLF
'''
  tmp_dir = os.path.abspath(svntest.main.temp_dir)
  config_dir = os.path.join(tmp_dir, 'autoprops_config')
  svntest.main.create_config_dir(config_dir, config_contents)

  # create a new file and import it
  file_name = "test.dsp"
  file_path = file_name
  imp_dir_path = 'dir'
  imp_file_path = os.path.join(imp_dir_path, file_name)

  os.mkdir(imp_dir_path, 0755)
  svntest.main.file_write(imp_file_path, "This is file test.dsp.\n")

  svntest.actions.run_and_verify_svn(None, None, [], 'import',
                                     '-m', 'Log message for new import',
                                     imp_dir_path,
                                     sbox.repo_url,
                                     '--config-dir', config_dir)

  svntest.main.run_svn(None, 'update', '.', '--config-dir', config_dir)

  # change part of the file
  svntest.main.file_append(file_path, "Extra line\n")

  # get a diff of the file, if the eol style is handled correctly, we'll
  # only see our added line here.
  # Before the issue was fixed, we would have seen something like this:
  # @@ -1 +1,2 @@
  # -This is file test.dsp.
  # +This is file test.dsp.
  # +Extra line

  # eol styl of test.dsp is CRLF, so diff will use that too. Make sure we
  # define CRLF in a platform independent way.
  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'
  expected_output = [
  "Index: test.dsp\n",
  "===================================================================\n",
  "--- test.dsp\t(revision 2)\n",
  "+++ test.dsp\t(working copy)\n",
  "@@ -1 +1,2 @@\n",
  " This is file test.dsp." + crlf,
  "+Extra line" + crlf
  ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff',
                                     file_path,
                                     '--config-dir', config_dir)

#----------------------------------------------------------------------
@Issue(3983)
def import_into_foreign_repo(sbox):
  "import into a foreign repo"

  sbox.build(read_only=True)

  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.safe_rmtree(other_repo_dir, 1)
  svntest.main.create_repos(other_repo_dir)

  svntest.actions.run_and_verify_svn(None, None, [], 'import',
                                     '-m', 'Log message for new import',
                                     sbox.ospath('A/mu'), other_repo_url + '/f')

#----------------------------------------------------------------------
########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              import_executable,
              import_ignores,
              import_avoid_empty_revision,
              import_no_ignores,
              import_eol_style,
              import_into_foreign_repo,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
