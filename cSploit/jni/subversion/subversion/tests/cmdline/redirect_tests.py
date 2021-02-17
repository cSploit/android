#!/usr/bin/env python
#
#  redirect_tests.py:  Test ra_dav handling of server-side redirects
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

# (abbreviations)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco

# Regular expression which matches the redirection notification
redirect_regex = re.compile(r"^Redirecting to URL '.*'")

# Generic UUID-matching regular expression
uuid_regex = re.compile(r"[a-fA-F0-9]{8}(-[a-fA-F0-9]{4}){3}-[a-fA-F0-9]{12}")


def verify_url(wc_path, url, wc_path_is_file=False):
  # check that we have a Repository Root and Repository UUID
  name = os.path.basename(wc_path)
  expected = {'Path' : re.escape(wc_path),
              'URL' : url,
              'Repository Root' : '.*',
              'Revision' : '.*',
              'Node Kind' : 'directory',
              'Repository UUID' : uuid_regex,
             }
  if wc_path_is_file:
    expected.update({'Name' : name,
                     'Node Kind' : 'file',
                     })
  svntest.actions.run_and_verify_info([expected], wc_path)


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_ra_type_dav)
def temporary_redirect(sbox):
  "temporary redirect should error out"

  sbox.build(create_wc=False)
  wc_dir = sbox.add_wc_path("my")
  co_url = sbox.redirected_root_url(temporary=True)

  # Try various actions against the repository, expecting an error
  # that indicates that some relocation has occurred.
  exit_code, out, err = svntest.main.run_svn('.*moved temporarily.*',
                                             'info', co_url)
  exit_code, out, err = svntest.main.run_svn('.*moved temporarily.*',
                                             'co', co_url, wc_dir)
  exit_code, out, err = svntest.main.run_svn('.*moved temporarily.*',
                                             'mkdir', '-m', 'MKDIR',
                                             co_url + '/newdir')
  exit_code, out, err = svntest.main.run_svn('.*moved temporarily.*',
                                             'delete', '-m', 'DELETE',
                                             co_url + '/iota')

#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_ra_type_dav)
def redirected_checkout(sbox):
  "redirected checkout"

  sbox.build(create_wc=False)
  wc_dir = sbox.add_wc_path("my")
  co_url = sbox.redirected_root_url()

  # Checkout the working copy via its redirect URL
  exit_code, out, err = svntest.main.run_svn(None, 'co', co_url, wc_dir)
  if err:
    raise svntest.Failure
  if not redirect_regex.match(out[0]):
    raise svntest.Failure

  # Verify that we have the expected URL.
  verify_url(wc_dir, sbox.repo_url)

#----------------------------------------------------------------------
@SkipUnless(svntest.main.is_ra_type_dav)
def redirected_update(sbox):
  "redirected update"

  sbox.build()
  wc_dir = sbox.wc_dir
  relocate_url = sbox.redirected_root_url()

  # Relocate (by cheating) the working copy to the redirect URL.  When
  # we then update, we'll expect to find ourselves automagically back
  # to the original URL.  (This is because we can't easily introduce a
  # redirect to the Apache configuration from the test suite here.)
  svntest.actions.no_relocate_validation()
  exit_code, out, err = svntest.main.run_svn(None, 'sw', '--relocate',
                                             sbox.repo_url, relocate_url,
                                             wc_dir)
  svntest.actions.do_relocate_validation()

  # Now update the working copy.
  exit_code, out, err = svntest.main.run_svn(None, 'up', wc_dir)
  if err:
    raise svntest.Failure
  if not re.match("^Updating '.*':", out[0]):
    raise svntest.Failure
  if not redirect_regex.match(out[1]):
    raise svntest.Failure

  # Verify that we have the expected URL.
  verify_url(wc_dir, sbox.repo_url)

#----------------------------------------------------------------------

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              temporary_redirect,
              redirected_checkout,
              redirected_update,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
