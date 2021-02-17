#!/usr/bin/env python
#
#  svnmucc_tests.py: tests of svnmucc
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

import svntest

XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco

######################################################################

@Issues(3895,3953)
def reject_bogus_mergeinfo(sbox):
  "reject bogus mergeinfo"

  sbox.build(create_wc=False)

  expected_error = ".*(E200020.*Invalid revision|E175008.*property change)"

  # At present this tests the server, but if we ever make svnmucc
  # validate the mergeinfo up front then it will only test the client
  svntest.actions.run_and_verify_svnmucc(None, [], expected_error,
                                         'propset', 'svn:mergeinfo', '/B:0',
                                         sbox.repo_url + '/A')

######################################################################

test_list = [ None,
              reject_bogus_mergeinfo,
            ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
