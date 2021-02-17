#!/usr/bin/env python
#
#  trans_tests.py:  testing eol conversion and keyword substitution
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
from svntest import wc

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


######################################################################
# THINGS TO TEST
#
# *** Perhaps something like commit_tests.py:make_standard_slew_of_changes
#     is in order here in this file as well. ***
#
# status level 1:
#    enable translation, status
#    (now throw local text mods into the picture)
#
# commit level 1:
#    enable translation, commit
#    (now throw local text mods into the picture)
#
# checkout:
#    checkout stuff with translation enabled
#
# status level 2:
#    disable translation, status
#    change newline conversion to different style, status
#    (now throw local text mods into the picture)
#
# commit level 2:
#    disable translation, commit
#    change newline conversion to different style, commit
#    (now throw local text mods into the picture)
#    (now throw local text mods with tortured line endings into the picture)
#
# update:
#    update files from disabled translation to enabled translation
#    update files from enabled translation to disabled translation
#    update files with newline conversion style changes
#    (now throw local text mods into the picture)
#    (now throw conflicting local property mods into the picture)
#
####



# Paths that the tests test.
author_rev_unexp_path = ''
author_rev_exp_path = ''
bogus_keywords_path = ''
embd_author_rev_unexp_path = ''
embd_author_rev_exp_path = ''
embd_bogus_keywords_path = ''

def check_keywords(actual_kw, expected_kw, name):
  """A Helper function to compare two keyword lists"""

  if len(actual_kw) != len(expected_kw):
    print("Keyword lists are different by size")
    raise svntest.Failure

  for i in range(0,len(actual_kw)):
    if actual_kw[i] != expected_kw[i]:
      print('%s item %s, Expected: %s' % (name, i, expected_kw[i][:-1]))
      print('%s item %s, Got:      %s' % (name, i, actual_kw[i][:-1]))
      raise svntest.Failure

def setup_working_copy(wc_dir, value_len):
  """Setup a standard test working copy, then create (but do not add)
  various files for testing translation."""

  global author_rev_unexp_path
  global author_rev_exp_path
  global url_unexp_path
  global url_exp_path
  global id_unexp_path
  global id_exp_path
  global header_unexp_path
  global header_exp_path
  global bogus_keywords_path
  global embd_author_rev_unexp_path
  global embd_author_rev_exp_path
  global embd_bogus_keywords_path
  global fixed_length_keywords_path
  global id_with_space_path
  global id_exp_with_dollar_path

  # NOTE: Only using author and revision keywords in tests for now,
  # since they return predictable substitutions.

  # Unexpanded, expanded, and bogus keywords; sometimes as the only
  # content of the files, sometimes embedded in non-keyword content.
  author_rev_unexp_path = os.path.join(wc_dir, 'author_rev_unexp')
  author_rev_exp_path = os.path.join(wc_dir, 'author_rev_exp')
  url_unexp_path = os.path.join(wc_dir, 'url_unexp')
  url_exp_path = os.path.join(wc_dir, 'url_exp')
  id_unexp_path = os.path.join(wc_dir, 'id_unexp')
  id_exp_path = os.path.join(wc_dir, 'id_exp')
  header_unexp_path = os.path.join(wc_dir, 'header_unexp')
  header_exp_path = os.path.join(wc_dir, 'header_exp')
  bogus_keywords_path = os.path.join(wc_dir, 'bogus_keywords')
  embd_author_rev_unexp_path = os.path.join(wc_dir, 'embd_author_rev_unexp')
  embd_author_rev_exp_path = os.path.join(wc_dir, 'embd_author_rev_exp')
  embd_bogus_keywords_path = os.path.join(wc_dir, 'embd_bogus_keywords')
  fixed_length_keywords_path = os.path.join(wc_dir, 'fixed_length_keywords')
  id_with_space_path = os.path.join(wc_dir, 'id with space')
  id_exp_with_dollar_path = os.path.join(wc_dir, 'id_exp with_$_sign')

  svntest.main.file_append(author_rev_unexp_path, "$Author$\n$Rev$")
  svntest.main.file_append(author_rev_exp_path, "$Author: blah $\n$Rev: 0 $")
  svntest.main.file_append(url_unexp_path, "$URL$")
  svntest.main.file_append(url_exp_path, "$URL: blah $")
  svntest.main.file_append(id_unexp_path, "$Id$")
  svntest.main.file_append(id_exp_path, "$Id: blah $")
  svntest.main.file_append(header_unexp_path, "$Header$")
  svntest.main.file_append(header_exp_path, "$Header: blah $")
  svntest.main.file_append(bogus_keywords_path, "$Arthur$\n$Rev0$")
  svntest.main.file_append(embd_author_rev_unexp_path,
                           "one\nfish\n$Author$ two fish\n red $Rev$\n fish")
  svntest.main.file_append(embd_author_rev_exp_path,
                           "blue $Author: blah $ fish$Rev: 0 $\nI fish")
  svntest.main.file_append(embd_bogus_keywords_path,
                           "you fish $Arthur$then\n we$Rev0$ \n\nchew fish")

  keyword_test_targets = [
    # User tries to shoot him or herself on the foot
    "$URL::$\n",
    "$URL:: $\n",
    "$URL::  $\n",
    # Following are valid entries
    "$URL::   $\n",
    "$URL:: %s $\n" % (' ' * (value_len-1)),
    "$URL:: %s $\n" % (' ' * value_len),
    # Check we will clean the truncate marker when the value fits exactly
    "$URL:: %s#$\n" % ('a' * value_len),
    "$URL:: %s $\n" % (' ' * (value_len+1)),
    # These are syntactically wrong
    "$URL::x%s $\n" % (' ' * value_len),
    "$URL:: %sx$\n" % (' ' * value_len),
    "$URL::x%sx$\n" % (' ' * value_len)
    ]

  for i in keyword_test_targets:
    svntest.main.file_append(fixed_length_keywords_path, i)

  svntest.main.file_append(id_with_space_path, "$Id$")
  svntest.main.file_append(id_exp_with_dollar_path,
                   "$Id: id_exp with_$_sign 1 2006-06-10 11:10:00Z jrandom $")


### Helper functions for setting/removing properties

# Set the property keyword for PATH.  Turn on all possible keywords.
### todo: Later, take list of keywords to set.
def keywords_on(path):
  svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                     "svn:keywords",
                                     "Author Rev Date URL Id Header",
                                     path)

# Delete property NAME from versioned PATH in the working copy.
### todo: Later, take list of keywords to remove from the propval?
def keywords_off(path):
  svntest.actions.run_and_verify_svn(None, None, [], 'propdel',
                                     "svn:keywords", path)


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.

#----------------------------------------------------------------------

### This test is know to fail when Subversion is built in very deep
### directory structures, caused by SVN_KEYWORD_MAX_LEN being defined
### as 255.
def keywords_from_birth(sbox):
  "commit new files with keywords active from birth"

  sbox.build()
  wc_dir = sbox.wc_dir

  canonical_repo_url = svntest.main.canonicalize_url(sbox.repo_url)
  if canonical_repo_url[-1:] != '/':
    url_expand_test_data = canonical_repo_url + '/fixed_length_keywords'
  else:
    url_expand_test_data = canonical_repo_url + 'fixed_length_keywords'

  setup_working_copy(wc_dir, len(url_expand_test_data))

  # Add all the files
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'author_rev_unexp' : Item(status='A ', wc_rev=0),
    'author_rev_exp' : Item(status='A ', wc_rev=0),
    'url_unexp' : Item(status='A ', wc_rev=0),
    'url_exp' : Item(status='A ', wc_rev=0),
    'id_unexp' : Item(status='A ', wc_rev=0),
    'id_exp' : Item(status='A ', wc_rev=0),
    'header_unexp' : Item(status='A ', wc_rev=0),
    'header_exp' : Item(status='A ', wc_rev=0),
    'bogus_keywords' : Item(status='A ', wc_rev=0),
    'embd_author_rev_unexp' : Item(status='A ', wc_rev=0),
    'embd_author_rev_exp' : Item(status='A ', wc_rev=0),
    'embd_bogus_keywords' : Item(status='A ', wc_rev=0),
    'fixed_length_keywords' : Item(status='A ', wc_rev=0),
    'id with space' : Item(status='A ', wc_rev=0),
    'id_exp with_$_sign' : Item(status='A ', wc_rev=0),
    })

  svntest.main.run_svn(None, 'add', author_rev_unexp_path)
  svntest.main.run_svn(None, 'add', author_rev_exp_path)
  svntest.main.run_svn(None, 'add', url_unexp_path)
  svntest.main.run_svn(None, 'add', url_exp_path)
  svntest.main.run_svn(None, 'add', id_unexp_path)
  svntest.main.run_svn(None, 'add', id_exp_path)
  svntest.main.run_svn(None, 'add', header_unexp_path)
  svntest.main.run_svn(None, 'add', header_exp_path)
  svntest.main.run_svn(None, 'add', bogus_keywords_path)
  svntest.main.run_svn(None, 'add', embd_author_rev_unexp_path)
  svntest.main.run_svn(None, 'add', embd_author_rev_exp_path)
  svntest.main.run_svn(None, 'add', embd_bogus_keywords_path)
  svntest.main.run_svn(None, 'add', fixed_length_keywords_path)
  svntest.main.run_svn(None, 'add', id_with_space_path)
  svntest.main.run_svn(None, 'add', id_exp_with_dollar_path)

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Add the keyword properties.
  keywords_on(author_rev_unexp_path)
  keywords_on(url_unexp_path)
  keywords_on(url_exp_path)
  keywords_on(id_unexp_path)
  keywords_on(id_exp_path)
  keywords_on(header_unexp_path)
  keywords_on(header_exp_path)
  keywords_on(embd_author_rev_exp_path)
  keywords_on(fixed_length_keywords_path)
  keywords_on(id_with_space_path)
  keywords_on(id_exp_with_dollar_path)

  # Commit.
  expected_output = svntest.wc.State(wc_dir, {
    'author_rev_unexp' : Item(verb='Adding'),
    'author_rev_exp' : Item(verb='Adding'),
    'url_unexp' : Item(verb='Adding'),
    'url_exp' : Item(verb='Adding'),
    'id_unexp' : Item(verb='Adding'),
    'id_exp' : Item(verb='Adding'),
    'header_unexp' : Item(verb='Adding'),
    'header_exp' : Item(verb='Adding'),
    'bogus_keywords' : Item(verb='Adding'),
    'embd_author_rev_unexp' : Item(verb='Adding'),
    'embd_author_rev_exp' : Item(verb='Adding'),
    'embd_bogus_keywords' : Item(verb='Adding'),
    'fixed_length_keywords' : Item(verb='Adding'),
    'id with space' : Item(verb='Adding'),
    'id_exp with_$_sign' : Item(verb='Adding'),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, None, wc_dir)

  # Make sure the unexpanded URL keyword got expanded correctly.
  fp = open(url_unexp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$URL: (http|https|file|svn|svn\\+ssh)://",
                        lines[0]))):
    print("URL expansion failed for %s" % url_unexp_path)
    raise svntest.Failure
  fp.close()

  # Make sure the preexpanded URL keyword got reexpanded correctly.
  fp = open(url_exp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$URL: (http|https|file|svn|svn\\+ssh)://",
                        lines[0]))):
    print("URL expansion failed for %s" % url_exp_path)
    raise svntest.Failure
  fp.close()

  # Make sure the unexpanded Id keyword got expanded correctly.
  fp = open(id_unexp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Id: id_unexp", lines[0]))):
    print("Id expansion failed for %s" % id_exp_path)
    raise svntest.Failure
  fp.close()

  # Make sure the preexpanded Id keyword got reexpanded correctly.
  fp = open(id_exp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Id: id_exp", lines[0]))):
    print("Id expansion failed for %s" % id_exp_path)
    raise svntest.Failure
  fp.close()

  # Make sure the unexpanded Header keyword got expanded correctly.
  fp = open(header_unexp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Header: (https?|file|svn|svn\\+ssh)://.* jrandom",
                        lines[0]))):
    print("Header expansion failed for %s" % header_unexp_path)
    raise svntest.Failure
  fp.close()

  # Make sure the preexpanded Header keyword got reexpanded correctly.
  fp = open(header_exp_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Header: (https?|file|svn|svn\\+ssh)://.* jrandom",
                        lines[0]))):
    print("Header expansion failed for %s" % header_exp_path)
    raise svntest.Failure
  fp.close()

  # Check fixed length keywords.
  kw_workingcopy = [
    '$URL::$\n',
    '$URL:: $\n',
    '$URL::  $\n',
    '$URL:: %s#$\n' % url_expand_test_data[0:1],
    '$URL:: %s#$\n' % url_expand_test_data[:-1],
    '$URL:: %s $\n' % url_expand_test_data,
    '$URL:: %s $\n' % url_expand_test_data,
    '$URL:: %s  $\n'% url_expand_test_data,
    '$URL::x%s $\n' % (' ' * len(url_expand_test_data)),
    '$URL:: %sx$\n' % (' ' * len(url_expand_test_data)),
    '$URL::x%sx$\n' % (' ' * len(url_expand_test_data))
  ]

  fp = open(fixed_length_keywords_path, 'r')
  actual_workingcopy_kw = fp.readlines()
  fp.close()
  check_keywords(actual_workingcopy_kw, kw_workingcopy, "working copy")

  # Check text base for fixed length keywords.
  kw_textbase = [
    '$URL::$\n',
    '$URL:: $\n',
    '$URL::  $\n',
    '$URL::   $\n',
    '$URL:: %s $\n' % (' ' * len(url_expand_test_data[:-1])),
    '$URL:: %s $\n' % (' ' * len(url_expand_test_data)),
    '$URL:: %s $\n' % (' ' * len(url_expand_test_data)),
    '$URL:: %s  $\n'% (' ' * len(url_expand_test_data)),
    '$URL::x%s $\n' % (' ' * len(url_expand_test_data)),
    '$URL:: %sx$\n' % (' ' * len(url_expand_test_data)),
    '$URL::x%sx$\n' % (' ' * len(url_expand_test_data))
    ]

  fp = open(svntest.wc.text_base_path(fixed_length_keywords_path), 'r')
  actual_textbase_kw = fp.readlines()
  fp.close()
  check_keywords(actual_textbase_kw, kw_textbase, "text base")

  # Check the Id keyword for filename with spaces.
  fp = open(id_with_space_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Id: .*id with space", lines[0]))):
    print("Id expansion failed for %s" % id_with_space_path)
    raise svntest.Failure
  fp.close()

  # Check the Id keyword for filename with_$_signs.
  fp = open(id_exp_with_dollar_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$Id: .*id_exp with_\$_sign [^$]* jrandom \$",
                        lines[0]))):
    print("Id expansion failed for %s" % id_exp_with_dollar_path)

    raise svntest.Failure
  fp.close()

#----------------------------------------------------------------------

#def enable_translation(sbox):
#  "enable translation, check status, commit"

  # TODO: Turn on newline conversion and/or keyword substitution for all
  # sorts of files, with and without local mods, and verify that
  # status shows the right stuff.  The, commit those mods.

#----------------------------------------------------------------------

#def checkout_translated():
#  "checkout files that have translation enabled"

  # TODO: Checkout a tree which contains files with translation
  # enabled.

#----------------------------------------------------------------------

#def disable_translation():
#  "disable translation, check status, commit"

  # TODO: Disable translation on files which have had it enabled,
  # with and without local mods, check status, and commit.

#----------------------------------------------------------------------

# Regression test for bug discovered by Vladmir Prus <ghost@cs.msu.csu>.
# This is a slight rewrite of his test, to use the run_and_verify_* API.
# This is for issue #631.

def do_nothing(x, y):
  return 0

@Issue(631)
def update_modified_with_translation(sbox):
  "update modified file with eol-style 'native'"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Replace contents of rho and set eol translation to 'native'
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_write(rho_path, "1\n2\n3\n4\n5\n6\n7\n8\n9\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:eol-style', 'native',
                                     rho_path)

  # Create expected output and status trees of a commit.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  # rho has props
  expected_status.tweak('A/D/G/rho', wc_rev=2, status='  ')

  # Commit revision 2:  it has the new rho.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, rho_path)

  # Change rho again
  svntest.main.file_write(rho_path, "1\n2\n3\n4\n4.5\n5\n6\n7\n8\n9\n")

  # Commit revision 3
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/G/rho', wc_rev=3, status='  ')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, rho_path)

  # Locally modify rho again.
  svntest.main.file_write(rho_path, "1\n2\n3\n4\n4.5\n5\n6\n7\n8\n9\n10\n")

  # Prepare trees for an update to rev 1.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/rho' : Item(status='CU'),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/G/rho',
                      contents="\n".join(["<<<<<<< .mine",
                                          "1",
                                          "2",
                                          "3",
                                          "4",
                                          "4.5",
                                          "5",
                                          "6",
                                          "7",
                                          "8",
                                          "9",
                                          "10",
                                          "=======",
                                          "This is the file 'rho'.",
                                          ">>>>>>> .r1",
                                          ""]))

  # Updating back to revision 1 should not error; the merge should
  # work, with eol-translation turned on.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        None, None,
                                        do_nothing, None,
                                        None, None,
                                        0, '-r', '1', wc_dir)


#----------------------------------------------------------------------

# Regression test for issue #1085, whereby setting the eol-style to a
# fixed platform-incorrect value on a file whose line endings are
# platform-correct causes repository insanity (the eol-style prop
# claims one line ending style, the file is in another).  This test
# assumes that this can be testing by verifying that a) new file
# contents are transmitted to the server during commit, and b) that
# after the commit, the file and its text-base have been changed to
# have the new line-ending style.
@Issue(1085)
def eol_change_is_text_mod(sbox):
  "committing eol-style change forces text send"

  sbox.build()

  wc_dir = sbox.wc_dir

  # add a new file to the working copy.
  foo_path = os.path.join(wc_dir, 'foo')
  f = open(foo_path, 'wb')
  if svntest.main.windows:
    f.write("1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n")
  else:
    f.write("1\n2\n3\n4\n5\n6\n7\n8\n9\n")
  f.close()

  # commit the file
  svntest.actions.run_and_verify_svn(None, None, [], 'add', foo_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     foo_path)

  if svntest.main.windows:
    svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                       'svn:eol-style', 'LF', foo_path)
  else:
    svntest.actions.run_and_verify_svn(None, None, [], 'propset',
                                       'svn:eol-style', 'CRLF', foo_path)

  # check 1: did new contents get transmitted?
  expected_output = ["Sending        " + foo_path + "\n",
                     "Transmitting file data .\n",
                     "Committed revision 3.\n"]
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'ci', '-m', 'log msg', foo_path)

  # check 2: do the files have the right contents now?
  contents = open(foo_path, 'rb').read()
  if svntest.main.windows:
    if contents != "1\n2\n3\n4\n5\n6\n7\n8\n9\n":
      raise svntest.Failure
  else:
    if contents != "1\r\n2\r\n3\r\n4\r\n5\r\n6\r\n7\r\n8\r\n9\r\n":
      raise svntest.Failure

  foo_base_path = svntest.wc.text_base_path(foo_path)
  base_contents = open(foo_base_path, 'rb').read()
  if contents != base_contents:
    raise svntest.Failure

#----------------------------------------------------------------------
# Regression test for issue #1151.  A single file in a directory
# didn't get keywords expanded on checkout.
@Issue(1151)
def keyword_expanded_on_checkout(sbox):
  "keyword expansion for lone file in directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  # The bug didn't occur if there were multiple files in the
  # directory, so setup an empty directory.
  Z_path = os.path.join(wc_dir, 'Z')
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', Z_path)

  # Add the file that has the keyword to be expanded
  url_path = os.path.join(Z_path, 'url')
  svntest.main.file_append(url_path, "$URL$")
  svntest.actions.run_and_verify_svn(None, None, [], 'add', url_path)
  keywords_on(url_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', wc_dir)

  other_wc_dir = sbox.add_wc_path('other')
  other_url_path = os.path.join(other_wc_dir, 'Z', 'url')
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     sbox.repo_url,
                                     other_wc_dir)

  # Check keyword got expanded (and thus the mkdir, add, ps, commit
  # etc. worked)
  fp = open(other_url_path, 'r')
  lines = fp.readlines()
  if not ((len(lines) == 1)
          and (re.match("\$URL: (http|https|file|svn|svn\\+ssh)://",
                        lines[0]))):
    print("URL expansion failed for %s" % other_url_path)
    raise svntest.Failure
  fp.close()


#----------------------------------------------------------------------
def cat_keyword_expansion(sbox):
  "keyword expanded on cat"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')

  # Set up A/mu to do $Rev$ keyword expansion
  svntest.main.file_append(mu_path , "$Rev$\n$Author$")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:keywords', 'Rev Author',
                                     mu_path)

  expected_output = wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Change the author to value which will get truncated on expansion
  full_author = "x" * 400
  key_author = "x" * 244
  svntest.actions.enable_revprop_changes(sbox.repo_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', '--revprop', '-r2',
                                     'svn:author', full_author,
                                     sbox.wc_dir)
  svntest.actions.run_and_verify_svn(None, [ full_author ], [],
                                     'propget', '--revprop', '-r2',
                                     'svn:author', '--strict',
                                     sbox.wc_dir)

  # Make another commit so that the last changed revision for A/mu is
  # not HEAD.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'foo', 'bar', lambda_path)
  expected_output = wc.State(wc_dir, {
    'A/B/lambda' : Item(verb='Sending'),
    })
  expected_status.tweak('A/B/lambda', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # At one stage the keywords were expanded to values for the requested
  # revision, not to those committed revision
  svntest.actions.run_and_verify_svn(None,
                                     [ "This is the file 'mu'.\n",
                                       "$Rev: 2 $\n",
                                       "$Author: " + key_author + " $"], [],
                                     'cat', '-r', 'HEAD', mu_path)


#----------------------------------------------------------------------
def copy_propset_commit(sbox):
  "copy, propset svn:eol-style, commit"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  mu2_path = os.path.join(wc_dir, 'A', 'mu2')

  # Copy and propset
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', mu_path, mu2_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:eol-style', 'native',
                                     mu2_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/mu2' : Item(status='A ', wc_rev='-', copied='+')
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Commit, at one stage this dumped core
  expected_output = wc.State(wc_dir, {
    'A/mu2' : Item(verb='Adding'),
    })
  expected_status.tweak('A/mu2', status='  ', wc_rev=2, copied=None)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

#----------------------------------------------------------------------
#      Create a greek tree, commit a keyword into one file,
#      then commit a keyword property (i.e., turn on keywords), then
#      try to check out head somewhere else.
#      This should not cause seg fault
def propset_commit_checkout_nocrash(sbox):
  "propset, commit, check out into another wc"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Put a keyword in A/mu, commit
  svntest.main.file_append(mu_path, "$Rev$")
  expected_output = wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Set property to do keyword expansion on A/mu, commit.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:keywords', 'Rev', mu_path)
  expected_output = wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Check out into another wc dir
  other_wc_dir = sbox.add_wc_path('other')
  mu_other_path = os.path.join(other_wc_dir, 'A', 'mu')

  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     sbox.repo_url,
                                     other_wc_dir)

  mu_other_contents = open(mu_other_path).read()
  if mu_other_contents != "This is the file 'mu'.\n$Rev: 3 $":
    print("'%s' does not have the expected contents" % mu_other_path)
    raise svntest.Failure


#----------------------------------------------------------------------
#      Add the keyword property to a file, svn revert the file
#      This should not display any error message
def propset_revert_noerror(sbox):
  "propset, revert"

  sbox.build()
  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Set the Rev keyword for the mu file
  # could use the keywords_on()/keywords_off() functions to
  # set/del all svn:keywords
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:keywords', 'Rev', mu_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status=' M')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Revert the propset
  svntest.actions.run_and_verify_svn(None, None, [], 'revert', mu_path)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


def props_only_file_update(sbox):
  "retranslation occurs on a props-only update"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  content = ["This is the file 'iota'.\n",
             "$Author$\n",
             ]
  content_expanded = ["This is the file 'iota'.\n",
                      "$Author: jrandom $\n",
                      ]

  # Create r2 with iota's contents and svn:keywords modified
  open(iota_path, 'w').writelines(content)
  svntest.main.run_svn(None, 'propset', 'svn:keywords', 'Author', iota_path)

  expected_output = wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Create r3 that drops svn:keywords

  # put the content back to its untranslated form
  open(iota_path, 'w').writelines(content)

  svntest.main.run_svn(None, 'propdel', 'svn:keywords', iota_path)

  expected_status.tweak('iota', wc_rev=3)

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Now, go back to r2. iota should have the Author keyword expanded.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=''.join(content_expanded))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)

  svntest.actions.run_and_verify_update(wc_dir,
                                        None, None, expected_status,
                                        None,
                                        None, None, None, None,
                                        False,
                                        wc_dir, '-r', '2')

  if open(iota_path).read() != ''.join(content_expanded):
    raise svntest.Failure("$Author$ is not expanded in 'iota'")

  # Update to r3. this should retranslate iota, dropping the keyword expansion
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=''.join(content))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)

  svntest.actions.run_and_verify_update(wc_dir,
                                        None, expected_disk, expected_status,
                                        None,
                                        None, None, None, None,
                                        False,
                                        wc_dir)

  if open(iota_path).read() != ''.join(content):
    raise svntest.Failure("$Author$ is not contracted in 'iota'")

  # We used to leave some temporary files around. Make sure that we don't.
  temps = os.listdir(os.path.join(wc_dir, svntest.main.get_admin_name(), 'tmp'))
  if os.path.exists(os.path.join(wc_dir, svntest.main.get_admin_name(),
                                 'tmp', 'props')):
    temps.remove('prop-base')
    temps.remove('props')
  if temps:
    print('Temporary files leftover: %s' % (', '.join(temps),))
    raise svntest.Failure


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              keywords_from_birth,
              # enable_translation,
              # checkout_translated,
              # disable_translation,
              update_modified_with_translation,
              eol_change_is_text_mod,
              keyword_expanded_on_checkout,
              cat_keyword_expansion,
              copy_propset_commit,
              propset_commit_checkout_nocrash,
              propset_revert_noerror,
              props_only_file_update,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
