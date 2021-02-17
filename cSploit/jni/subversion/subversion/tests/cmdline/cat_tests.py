#!/usr/bin/env python
#
#  cat_tests.py:  testing cat cases.
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
import os, re

# Our testing module
import svntest


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

def cat_local_directory(sbox):
  "cat a local directory"
  sbox.build(read_only = True)

  A_path = os.path.join(sbox.wc_dir, 'A')

  expected_err = "svn: warning: W195007: '" + \
                 re.escape(os.path.abspath(A_path)) + \
                 "' refers to a directory"

  svntest.actions.run_and_verify_svn2(None, None, expected_err,
                                      1, 'cat', A_path)

def cat_remote_directory(sbox):
  "cat a remote directory"
  sbox.build(create_wc = False, read_only = True)

  A_url = sbox.repo_url + '/A'
  expected_err = "svn: warning: W195007: URL '" + A_url + \
      "' refers to a directory\n.*"

  svntest.actions.run_and_verify_svn2(None, None, expected_err,
                                      1, 'cat', A_url)

def cat_base(sbox):
  "cat a file at revision BASE"
  sbox.build(read_only = True)

  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, 'Appended text')

  exit_code, outlines, errlines = svntest.main.run_svn(0, 'cat', mu_path)

  # Verify the expected output
  expected_output = svntest.main.greek_state.desc['A/mu'].contents
  if len(outlines) != 1 or outlines[0] != expected_output:
    raise svntest.Failure('Cat failed: expected "%s", but received "%s"' % \
      (expected_output, outlines))

def cat_nonexistent_file(sbox):
  "cat a nonexistent file"
  sbox.build(read_only = True)

  wc_dir = sbox.wc_dir

  bogus_path = os.path.join(wc_dir, 'A', 'bogus')
  expected_err = "svn: warning: W200005: '" + \
                 re.escape(os.path.abspath(bogus_path)) + \
                 "' is not under version control"

  svntest.actions.run_and_verify_svn2(None, None, expected_err, 1,
                                      'cat', bogus_path)

def cat_skip_uncattable(sbox):
  "cat should skip uncattable resources"
  sbox.build(read_only = True)

  wc_dir = sbox.wc_dir
  dir_path = os.path.join(wc_dir, 'A', 'D')
  new_file_path = os.path.join(dir_path, 'new')
  open(new_file_path, 'w')
  item_list = os.listdir(dir_path)

  # First we test running 'svn cat' on individual objects, expecting
  # warnings for unversioned files and for directories.  Then we try
  # running 'svn cat' on multiple targets at once, and make sure we
  # get the warnings we expect.

  # item_list has all the files and directories under 'dir_path'
  for file in item_list:
    if file == svntest.main.get_admin_name():
      continue
    item_to_cat = os.path.join(dir_path, file)
    if item_to_cat == new_file_path:
      expected_err = "svn: warning: W200005: '" + \
                     re.escape(os.path.abspath(item_to_cat)) + \
                     "' is not under version control"
      svntest.actions.run_and_verify_svn2(None, None, expected_err, 1,
                                           'cat', item_to_cat)

    elif os.path.isdir(item_to_cat):
      expected_err = "svn: warning: W195007: '" + \
                     re.escape(os.path.abspath(item_to_cat)) + \
                     "' refers to a directory"
      svntest.actions.run_and_verify_svn2(None, None, expected_err, 1,
                                           'cat', item_to_cat)
    else:
      svntest.actions.run_and_verify_svn(None,
                                         ["This is the file '"+file+"'.\n"],
                                         [], 'cat', item_to_cat)

  G_path = os.path.join(dir_path, 'G')
  rho_path = os.path.join(G_path, 'rho')

  expected_out = "This is the file 'rho'.\n"
  expected_err1 = "svn: warning: W195007: '" + \
                  re.escape(os.path.abspath(G_path)) + \
                  "' refers to a directory\n"
  svntest.actions.run_and_verify_svn2(None, expected_out, expected_err1, 1,
                                       'cat', rho_path, G_path)

  expected_err2 = "svn: warning: W200005: '" + \
                  re.escape(os.path.abspath(new_file_path)) + \
                  "' is not under version control\n"
  svntest.actions.run_and_verify_svn2(None, expected_out, expected_err2, 1,
                                       'cat', rho_path, new_file_path)

  expected_err3 = expected_err1 + expected_err2 + \
      ".*svn: E200009: Could not cat all targets because some targets"
  expected_err_re = re.compile(expected_err3, re.DOTALL)

  exit_code, output, error = svntest.main.run_svn(1, 'cat', rho_path, G_path, new_file_path)

  # Verify output
  if output[0] != expected_out:
    raise svntest.Failure('Cat failed: expected "%s", but received "%s"' % \
                          (expected_out, "".join(output)))

  # Verify error
  if not expected_err_re.match("".join(error)):
    raise svntest.Failure('Cat failed: expected error "%s", but received "%s"' % \
                          (expected_err3, "".join(error)))

# Test for issue #3560 'svn_wc_status3() returns incorrect status for
# unversioned files'.
@Issue(3560)
def cat_unversioned_file(sbox):
  "cat an unversioned file parent dir thinks exists"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')

  # Delete a file an commit the deletion.
  svntest.actions.run_and_verify_svn2(None, None, [], 0,
                                      'delete', iota_path)
  svntest.actions.run_and_verify_svn2(None, None, [], 0,
                                      'commit', '-m', 'delete a file',
                                      iota_path)

  # Now try to cat the deleted file, it should be reported as unversioned.
  expected_error = "svn: warning: W200005: '" + \
                   re.escape(os.path.abspath(iota_path)) + \
                   "' is not under version control"
  svntest.actions.run_and_verify_svn2(None, [], expected_error, 1,
                                       'cat', iota_path)

  # Put an unversioned file at 'iota' and try to cat it again, the result
  # should still be the same.
  svntest.main.file_write(iota_path, "This the unversioned file 'iota'.\n")
  svntest.actions.run_and_verify_svn2(None, [], expected_error, 1,
                                      'cat', iota_path)

def cat_keywords(sbox):
  "cat a file with the svn:keywords property"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')

  svntest.actions.run_and_verify_svn(None,
                                     ["This is the file 'iota'.\n"],
                                     [], 'cat', iota_path)

  svntest.main.file_append(iota_path, "$Revision$\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:keywords', 'Revision',
                                     iota_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'r2', wc_dir)

  svntest.actions.run_and_verify_svn(None,
                                     ["This is the file 'iota'.\n", "$Revision: 2 $\n"],
                                     [], 'cat', iota_path)

def cat_url_special_characters(sbox):
  """special characters in svn cat URL"""
  sbox.build(create_wc = False)
  wc_dir = sbox.wc_dir

  special_urls = [sbox.repo_url + '/A' + '/%2E',
                  sbox.repo_url + '%2F' + 'A']

  expected_err = "svn: warning: W195007: URL '" + sbox.repo_url + '/A' + \
      "' refers to a directory\n.*"

  for url in special_urls:
    svntest.actions.run_and_verify_svn2(None, None, expected_err, 1,
                                         'cat', url)

def cat_non_existing_remote_file(sbox):
  """cat non-existing remote file"""
  sbox.build(create_wc = False)
  non_existing_path = sbox.repo_url + '/non-existing'

  expected_err = "svn: warning: W160013: .*not found.*" + \
      non_existing_path.split('/')[1]

  # cat operation on non-existing remote path should return 1
  svntest.actions.run_and_verify_svn2(None, None, expected_err, 1,
                                      'cat', non_existing_path)

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              cat_local_directory,
              cat_remote_directory,
              cat_base,
              cat_nonexistent_file,
              cat_skip_uncattable,
              cat_unversioned_file,
              cat_keywords,
              cat_url_special_characters,
              cat_non_existing_remote_file,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
