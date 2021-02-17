#!/usr/bin/env python
#  -*- coding: utf-8 -*-
#
#  patch_tests.py:  some basic patch tests
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
import base64
import os
import re
import sys
import tempfile
import textwrap
import zlib
import posixpath

# Our testing module
import svntest
from svntest import wc
from svntest.main import SVN_PROP_MERGEINFO, is_os_windows

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem

def make_patch_path(sbox, name='my.patch'):
  dir = sbox.add_wc_path('patches')
  os.mkdir(dir)
  return os.path.abspath(os.path.join(dir, name))

########################################################################
#Tests

def patch(sbox):
  "basic patch"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
    "\n",
    "Index: new\n",
    "===================================================================\n",
    "--- new	(revision 0)\n",
    "+++ new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(revision 1)\n",
    "+++ A/B/E/beta	(working copy)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run


def patch_absolute_paths(sbox):
  "patch containing absolute paths"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  os.chdir(wc_dir)

  # A patch with absolute paths.
  # The first diff points inside the working copy and should apply.
  # The second diff does not point inside the working copy so application
  # should fail.
  abs = os.path.abspath('.')
  if sys.platform == 'win32':
    abs = abs.replace("\\", "/")
  unidiff_patch = [
    "diff -ur A/B/E/alpha.orig A/B/E/alpha\n"
    "--- %s/A/B/E/alpha.orig\tThu Apr 16 19:49:53 2009\n" % abs,
    "+++ %s/A/B/E/alpha\tThu Apr 16 19:50:30 2009\n" % abs,
    "@@ -1 +1,2 @@\n",
    " This is the file 'alpha'.\n",
    "+Whoooo whooooo whoooooooo!\n",
    "diff -ur A/B/lambda.orig A/B/lambda\n"
    "--- /A/B/lambda.orig\tThu Apr 16 19:49:53 2009\n",
    "+++ /A/B/lambda\tThu Apr 16 19:51:25 2009\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'lambda'.\n",
    "+It's the file 'lambda', who would have thought!\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  lambda_path = os.path.join(os.path.sep, 'A', 'B', 'lambda')
  expected_output = [
    'U         %s\n' % os.path.join('A', 'B', 'E', 'alpha'),
    'Skipped missing target: \'%s\'\n' % lambda_path,
    'Summary of conflicts:\n',
    '  Skipped paths: 1\n'
  ]

  alpha_contents = "This is the file 'alpha'.\nWhoooo whooooo whoooooooo!\n"

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/B/E/alpha', contents=alpha_contents)

  expected_status = svntest.actions.get_virginal_state('.', 1)
  expected_status.tweak('A/B/E/alpha', status='M ')

  expected_skip = wc.State('', {
    lambda_path:  Item(),
  })

  svntest.actions.run_and_verify_patch('.', os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_offset(sbox):
  "patch with offset searching"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  iota_path = os.path.join(wc_dir, 'iota')

  mu_contents = [
    "Dear internet user,\n",
    # The missing line here will cause the first hunk to match early
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n",
  ]

  # iota's content will make both a late and early match possible.
  # The hunk to be applied is replicated here for reference:
  # @@ -5,6 +5,7 @@
  #  iota
  #  iota
  #  iota
  # +x
  #  iota
  #  iota
  #  iota
  #
  # This hunk wants to be applied at line 5, but that isn't
  # possible because line 8 ("zzz") does not match "iota".
  # The early match happens at line 2 (offset 3 = 5 - 2).
  # The late match happens at line 9 (offset 4 = 9 - 5).
  # Subversion will pick the early match in this case because it
  # is closer to line 5.
  iota_contents = [
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "zzz\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n"
  ]

  # Set mu and iota contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  svntest.main.file_write(iota_path, ''.join(iota_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    'iota'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota	(revision XYZ)\n",
    "+++ iota	(working copy)\n",
    "@@ -5,6 +5,7 @@\n",
    " iota\n",
    " iota\n",
    " iota\n",
    "+x\n",
    " iota\n",
    " iota\n",
    " iota\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  mu_contents = [
    "Dear internet user,\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  iota_contents = [
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "x\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "zzz\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
    "iota\n",
  ]

  os.chdir(wc_dir)

  expected_output = [
    'U         %s\n' % os.path.join('A', 'mu'),
    '>         applied hunk @@ -6,6 +6,9 @@ with offset -1\n',
    '>         applied hunk @@ -14,11 +17,8 @@ with offset 4\n',
    'U         iota\n',
    '>         applied hunk @@ -5,6 +5,7 @@ with offset -3\n',
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.tweak('iota', contents=''.join(iota_contents))

  expected_status = svntest.actions.get_virginal_state('.', 1)
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('iota', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch('.', os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_chopped_leading_spaces(sbox):
  "patch with chopped leading spaces"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
    "\n",
    "Index: new\n",
    "===================================================================\n",
    "--- new	(revision 0)\n",
    "+++ new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(revision 1)\n",
    "+++ A/B/E/beta	(working copy)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run


def patch_strip1(sbox):
  "patch with --strip 1"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: b/A/D/gamma\n",
    "===================================================================\n",
    "--- a/A/D/gamma\t(revision 1)\n",
    "+++ b/A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    "Index: x/iota\n",
    "===================================================================\n",
    "--- x/iota\t(revision 1)\n",
    "+++ x/iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
    "\n",
    "Index: /new\n",
    "===================================================================\n",
    "--- /new	(revision 0)\n",
    "+++ /new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "--- x/A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ x/A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- /A/B/E/beta	(revision 1)\n",
    "+++ /A/B/E/beta	(working copy)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1, # dry-run
                                       '--strip', '1')

def patch_no_index_line(sbox):
  "patch with no index lines"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  iota_path = os.path.join(wc_dir, 'iota')

  gamma_contents = [
    "\n",
    "Another line before\n",
    "A third line before\n",
    "This is the file 'gamma'.\n",
    "A line after\n",
    "Another line after\n",
    "A third line after\n",
  ]

  svntest.main.file_write(gamma_path, ''.join(gamma_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma'  : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  unidiff_patch = [
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1,7 +1,7 @@\n",
    " \n",
    " Another line before\n",
    " A third line before\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    " A line after\n",
    " Another line after\n",
    " A third line after\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = [
    "\n",
    "Another line before\n",
    "A third line before\n",
    "It is the file 'gamma'.\n",
    "A line after\n",
    "Another line after\n",
    "A third line after\n",
  ]
  iota_contents = [
    "This is the file 'iota'.\n",
    "Some more bytes\n",
  ]
  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=''.join(gamma_contents))
  expected_disk.tweak('iota', contents=''.join(iota_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ', wc_rev=2)
  expected_status.tweak('iota', status='M ', wc_rev=1)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_add_new_dir(sbox):
  "patch with missing dirs"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  # The first diff is adding 'new' with two missing dirs. The second is
  # adding 'new' with one missing dir to a 'A/B/E' that is locally deleted
  # (should be skipped). The third is adding 'new' to 'A/C' that is locally
  # deleted (should be skipped too). The fourth is adding 'new' with a
  # directory that is unversioned (should be skipped as well).
  unidiff_patch = [
    "Index: new\n",
    "===================================================================\n",
    "--- X/Y/new\t(revision 0)\n",
    "+++ X/Y/new\t(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "Index: new\n",
    "===================================================================\n",
    "--- A/B/E/Y/new\t(revision 0)\n",
    "+++ A/B/E/Y/new\t(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "Index: new\n",
    "===================================================================\n",
    "--- A/C/new\t(revision 0)\n",
    "+++ A/C/new\t(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "Index: new\n",
    "===================================================================\n",
    "--- A/Z/new\t(revision 0)\n",
    "+++ A/Z/new\t(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
  ]

  C_path = os.path.join(wc_dir, 'A', 'C')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')
  svntest.actions.run_and_verify_svn("Deleting C failed", None, [],
                                     'rm', C_path)
  svntest.actions.run_and_verify_svn("Deleting E failed", None, [],
                                     'rm', E_path)
  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  A_B_E_Y_new_path = os.path.join(wc_dir, 'A', 'B', 'E', 'Y', 'new')
  A_C_new_path = os.path.join(wc_dir, 'A', 'C', 'new')
  A_Z_new_path = os.path.join(wc_dir, 'A', 'Z', 'new')
  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'X'),
    'A         %s\n' % os.path.join(wc_dir, 'X', 'Y'),
    'A         %s\n' % os.path.join(wc_dir, 'X', 'Y', 'new'),
    'Skipped missing target: \'%s\'\n' % A_B_E_Y_new_path,
    'Skipped missing target: \'%s\'\n' % A_C_new_path,
    'Skipped missing target: \'%s\'\n' % A_Z_new_path,
    'Summary of conflicts:\n',
    '  Skipped paths: 3\n',
  ]

  # Create the unversioned obstructing directory
  os.mkdir(os.path.dirname(A_Z_new_path))

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
           'X/Y/new'   : Item(contents='new\n'),
           'A/Z'       : Item()
  })
  expected_disk.remove('A/B/E/alpha')
  expected_disk.remove('A/B/E/beta')
  if svntest.main.wc_is_singledb(wc_dir):
    expected_disk.remove('A/B/E')
    expected_disk.remove('A/C')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
           'X'         : Item(status='A ', wc_rev=0),
           'X/Y'       : Item(status='A ', wc_rev=0),
           'X/Y/new'   : Item(status='A ', wc_rev=0),
           'A/B/E'     : Item(status='D ', wc_rev=1),
           'A/B/E/alpha': Item(status='D ', wc_rev=1),
           'A/B/E/beta': Item(status='D ', wc_rev=1),
           'A/C'       : Item(status='D ', wc_rev=1),
  })

  expected_skip = wc.State('', {A_Z_new_path : Item(),
                                A_B_E_Y_new_path : Item(),
                                A_C_new_path : Item()})

  svntest.actions.run_and_verify_patch(wc_dir,
                                       os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run
def patch_remove_empty_dirs(sbox):
  "patch deleting all children of a directory"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  # Contents of B:
  # A/B/lamba
  # A/B/F
  # A/B/E/{alpha,beta}
  # Before patching we've deleted F, which means that B is empty after patching and
  # should be removed.
  #
  # Contents of H:
  # A/D/H/{chi,psi,omega}
  # Before patching, chi has been removed by a non-svn operation which means it has
  # status missing. The patch deletes the other two files but should not delete H.

  unidiff_patch = [
    "Index: psi\n",
    "===================================================================\n",
    "--- A/D/H/psi\t(revision 0)\n",
    "+++ A/D/H/psi\t(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'psi'.\n",
    "Index: omega\n",
    "===================================================================\n",
    "--- A/D/H/omega\t(revision 0)\n",
    "+++ A/D/H/omega\t(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'omega'.\n",
    "Index: lambda\n",
    "===================================================================\n",
    "--- A/B/lambda\t(revision 0)\n",
    "+++ A/B/lambda\t(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'lambda'.\n",
    "Index: alpha\n",
    "===================================================================\n",
    "--- A/B/E/alpha\t(revision 0)\n",
    "+++ A/B/E/alpha\t(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'alpha'.\n",
    "Index: beta\n",
    "===================================================================\n",
    "--- A/B/E/beta\t(revision 0)\n",
    "+++ A/B/E/beta\t(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  svntest.actions.run_and_verify_svn("Deleting F failed", None, [],
                                     'rm', F_path)
  svntest.actions.run_and_verify_svn("Update failed", None, [],
                                     'up', wc_dir)

  # We should be able to handle one path beeing missing.
  os.remove(os.path.join(wc_dir, 'A', 'D', 'H', 'chi'))

  expected_output = [
    'D         %s\n' % os.path.join(wc_dir, 'A', 'D', 'H', 'psi'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'D', 'H', 'omega'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'lambda'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'alpha'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/D/H/chi')
  expected_disk.remove('A/D/H/psi')
  expected_disk.remove('A/D/H/omega')
  expected_disk.remove('A/B/lambda')
  expected_disk.remove('A/B/E/alpha')
  expected_disk.remove('A/B/E/beta')
  if svntest.main.wc_is_singledb(wc_dir):
    expected_disk.remove('A/B/E')
    expected_disk.remove('A/B/F')
    expected_disk.remove('A/B')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'A/D/H/chi' : Item(status='! ', wc_rev=1)})
  expected_status.add({'A/D/H/omega' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/D/H/psi' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B/E' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B/E/beta' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B/E/alpha' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B/lambda' : Item(status='D ', wc_rev=1)})
  expected_status.add({'A/B/F' : Item(status='D ', wc_rev=1)})

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir,
                                       os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run


def patch_reject(sbox):
  "patch which is rejected"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Set gamma contents
  gamma_contents = "Hello there! I'm the file 'gamma'.\n"
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.file_write(gamma_path, gamma_contents)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  patch_file_path = make_patch_path(sbox)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is really the file 'gamma'.\n",
    "+It is really the file 'gamma'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'C         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    '>         rejected hunk @@ -1,1 +1,1 @@\n',
    'Summary of conflicts:\n',
    '  Text conflicts: 1\n',
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)

  reject_file_contents = [
    "--- A/D/gamma\n",
    "+++ A/D/gamma\n",
    "@@ -1,1 +1,1 @@\n",
    "-This is really the file 'gamma'.\n",
    "+It is really the file 'gamma'.\n",
  ]
  expected_disk.add({'A/D/gamma.svnpatch.rej' :
                     Item(contents=''.join(reject_file_contents))})

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=2)
  # ### not yet
  #expected_status.tweak('A/D/gamma', status='C ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_keywords(sbox):
  "patch containing keywords"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Set gamma contents
  gamma_contents = "$Rev$\nHello there! I'm the file 'gamma'.\n"
  gamma_path = os.path.join(wc_dir, 'A', 'D', 'gamma')
  svntest.main.file_write(gamma_path, gamma_contents)
  # Expand the keyword
  svntest.main.run_svn(None, 'propset', 'svn:keywords', 'Rev',
                       os.path.join(wc_dir, 'A', 'D', 'gamma'))
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/gamma'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  patch_file_path = make_patch_path(sbox)

  # Apply patch

  unidiff_patch = [
   "Index: gamma\n",
   "===================================================================\n",
   "--- A/D/gamma	(revision 3)\n",
   "+++ A/D/gamma	(working copy)\n",
   "@@ -1,2 +1,3 @@\n",
   " $Rev$\n",
   " Hello there! I'm the file 'gamma'.\n",
   "+booo\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  gamma_contents = "$Rev: 2 $\nHello there! I'm the file 'gamma'.\nbooo\n"
  expected_disk.tweak('A/D/gamma', contents=gamma_contents,
                      props={'svn:keywords' : 'Rev'})

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_with_fuzz(sbox):
  "patch with fuzz"

  sbox.build()
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # We have replaced a couple of lines to cause fuzz. Those lines contains
  # the word fuzz
  mu_contents = [
    "Line replaced for fuzz = 1\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "Line replaced for fuzz = 2 with only the second context line changed\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "This line is inserted to cause an offset of +1\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Line replaced for fuzz = 2\n",
    "Line replaced for fuzz = 2\n",
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                      expected_status, None, wc_dir)

  unidiff_patch = [
    "Index: mu\n",
    "===================================================================\n",
    "--- A/mu\t(revision 0)\n",
    "+++ A/mu\t(revision 0)\n",
    "@@ -1,6 +1,7 @@\n",
    " Dear internet user,\n",
    " \n",
    " We wish to congratulate you over your email success in our computer\n",
    "+A new line here\n",
    " Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    " in which email addresses were used. All participants were selected\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    "@@ -7,7 +8,9 @@\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    "+Another new line\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "+A third new line\n",
    " file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "@@ -19,6 +20,7 @@\n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "+A fourth new line\n",
    " \n",
    " Again, we wish to congratulate you over your email success in our\n"
    " computer Balloting. [No trailing newline here]"
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  mu_contents = [
    "Line replaced for fuzz = 1\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "A new line here\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "Line replaced for fuzz = 2 with only the second context line changed\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "Another new line\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "A third new line\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "This line is inserted to cause an offset of +1\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "A fourth new line\n",
    "\n",
    "Line replaced for fuzz = 2\n",
    "Line replaced for fuzz = 2\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    '>         applied hunk @@ -1,6 +1,7 @@ with fuzz 1\n',
    '>         applied hunk @@ -7,7 +8,9 @@ with fuzz 2\n',
    '>         applied hunk @@ -19,6 +20,7 @@ with offset 1 and fuzz 2\n',
  ]
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_reverse(sbox):
  "patch in reverse"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "+This is the file 'gamma'.\n",
    "-It is the file 'gamma'.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1,2 +1 @@\n",
    " This is the file 'iota'.\n",
    "-Some more bytes\n",
    "\n",
    "Index: new\n",
    "===================================================================\n",
    "--- new	(revision 0)\n",
    "+++ new	(revision 0)\n",
    "@@ -1 +0,0 @@\n",
    "-new\n",
    "\n",
    "--- A/mu	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu.orig	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,9 +6,6 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "-It is a promotional program aimed at encouraging internet users;\n",
    "-therefore you do not need to buy ticket to enter for it.\n",
    "-\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -17,8 +14,11 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "+and PROMOTION DATE: 13th June. 2009\n",
    "-and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "+\n",
    "+Again, we wish to congratulate you over your email success in our\n",
    "+computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(working copy)\n",
    "+++ A/B/E/beta	(revision 1)\n",
    "@@ -0,0 +1 @@\n",
    "+This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1, # dry-run
                                       '--reverse-diff')

def patch_no_svn_eol_style(sbox):
  "patch target with no svn:eol-style"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'
  eols = [crlf, '\015', '\n', '\012']

  for target_eol in eols:
    for patch_eol in eols:
      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        target_eol,
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        target_eol,
        "in which email addresses were used. All participants were selected",
        target_eol,
        "through a computer ballot system drawn from over 100,000 company",
        target_eol,
        "and 50,000,000 individual email addresses from all over the world.",
        target_eol,
        "It is a promotional program aimed at encouraging internet users;",
        target_eol,
      ]

      # Set mu contents
      svntest.main.file_write(mu_path, ''.join(mu_contents))

      unidiff_patch = [
        "Index: mu",
        patch_eol,
        "===================================================================",
        patch_eol,
        "--- A/mu\t(revision 0)",
        patch_eol,
        "+++ A/mu\t(revision 0)",
        patch_eol,
        "@@ -1,5 +1,6 @@",
        patch_eol,
        " We wish to congratulate you over your email success in our computer",
        patch_eol,
        " Balloting. This is a Millennium Scientific Electronic Computer Draw",
        patch_eol,
        "+A new line here",
        patch_eol,
        " in which email addresses were used. All participants were selected",
        patch_eol,
        " through a computer ballot system drawn from over 100,000 company",
        patch_eol,
        " and 50,000,000 individual email addresses from all over the world.",
        patch_eol,
      ]

      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        patch_eol,
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        patch_eol,
        "A new line here",
        patch_eol,
        "in which email addresses were used. All participants were selected",
        patch_eol,
        "through a computer ballot system drawn from over 100,000 company",
        patch_eol,
        "and 50,000,000 individual email addresses from all over the world.",
        patch_eol,
        "It is a promotional program aimed at encouraging internet users;",
        target_eol,
      ]

      svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

      expected_output = [
        'G         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
      ]
      expected_disk = svntest.main.greek_state.copy()
      expected_disk.tweak('A/mu', contents=''.join(mu_contents))

      expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
      expected_status.tweak('A/mu', status='M ', wc_rev=1)

      expected_skip = wc.State('', { })

      svntest.actions.run_and_verify_patch(wc_dir,
                                           os.path.abspath(patch_file_path),
                                           expected_output,
                                           expected_disk,
                                           expected_status,
                                           expected_skip,
                                           None, # expected err
                                           1, # check-props
                                           1) # dry-run

      expected_output = ["Reverted '" + mu_path + "'\n"]
      svntest.actions.run_and_verify_svn(None, expected_output, [], 'revert', '-R', wc_dir)

def patch_with_svn_eol_style(sbox):
  "patch target with svn:eol-style"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')


  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'

  eols = [crlf, '\015', '\n', '\012']
  eol_styles = ['CRLF', 'CR', 'native', 'LF']
  rev = 1
  for target_eol, target_eol_style in zip(eols, eol_styles):
    for patch_eol in eols:
      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        target_eol,
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        target_eol,
        "in which email addresses were used. All participants were selected",
        target_eol,
        "through a computer ballot system drawn from over 100,000 company",
        target_eol,
        "and 50,000,000 individual email addresses from all over the world.",
        target_eol,
        "It is a promotional program aimed at encouraging internet users;",
        target_eol,
      ]

      # Set mu contents
      svntest.main.run_svn(None, 'rm', mu_path)
      svntest.main.run_svn(None, 'commit', '-m', 'delete mu', mu_path)
      svntest.main.file_write(mu_path, ''.join(mu_contents))
      svntest.main.run_svn(None, 'add', mu_path)
      svntest.main.run_svn(None, 'propset', 'svn:eol-style', target_eol_style,
                           mu_path)
      svntest.main.run_svn(None, 'commit', '-m', 'set eol-style', mu_path)

      unidiff_patch = [
        "Index: mu",
        patch_eol,
        "===================================================================",
        patch_eol,
        "--- A/mu\t(revision 0)",
        patch_eol,
        "+++ A/mu\t(revision 0)",
        patch_eol,
        "@@ -1,5 +1,6 @@",
        patch_eol,
        " We wish to congratulate you over your email success in our computer",
        patch_eol,
        " Balloting. This is a Millennium Scientific Electronic Computer Draw",
        patch_eol,
        "+A new line here",
        patch_eol,
        " in which email addresses were used. All participants were selected",
        patch_eol,
        " through a computer ballot system drawn from over 100,000 company",
        patch_eol,
        " and 50,000,000 individual email addresses from all over the world.",
        patch_eol,
      ]

      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        target_eol,
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        target_eol,
        "A new line here",
        target_eol,
        "in which email addresses were used. All participants were selected",
        target_eol,
        "through a computer ballot system drawn from over 100,000 company",
        target_eol,
        "and 50,000,000 individual email addresses from all over the world.",
        target_eol,
        "It is a promotional program aimed at encouraging internet users;",
        target_eol,
      ]

      svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

      expected_output = [
        'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
      ]
      expected_disk = svntest.main.greek_state.copy()
      expected_disk.tweak('A/mu', contents=''.join(mu_contents),
                          props={'svn:eol-style' : target_eol_style})

      expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
      rev += 2
      expected_status.tweak('A/mu', status='M ', wc_rev=rev)

      expected_skip = wc.State('', { })

      svntest.actions.run_and_verify_patch(wc_dir,
                                           os.path.abspath(patch_file_path),
                                           expected_output,
                                           expected_disk,
                                           expected_status,
                                           expected_skip,
                                           None, # expected err
                                           1, # check-props
                                           1) # dry-run

      expected_output = ["Reverted '" + mu_path + "'\n"]
      svntest.actions.run_and_verify_svn(None, expected_output, [], 'revert', '-R', wc_dir)

def patch_with_svn_eol_style_uncommitted(sbox):
  "patch target with uncommitted svn:eol-style"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')


  if os.name == 'nt':
    crlf = '\n'
  else:
    crlf = '\r\n'

  eols = [crlf, '\015', '\n', '\012']
  eol_styles = ['CRLF', 'CR', 'native', 'LF']
  for target_eol, target_eol_style in zip(eols, eol_styles):
    for patch_eol in eols:
      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        '\n',
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        '\n',
        "in which email addresses were used. All participants were selected",
        '\n',
        "through a computer ballot system drawn from over 100,000 company",
        '\n',
        "and 50,000,000 individual email addresses from all over the world.",
        '\n',
        "It is a promotional program aimed at encouraging internet users;",
        '\n',
      ]

      # Set mu contents
      svntest.main.file_write(mu_path, ''.join(mu_contents))
      svntest.main.run_svn(None, 'propset', 'svn:eol-style', target_eol_style,
                           mu_path)

      unidiff_patch = [
        "Index: mu",
        patch_eol,
        "===================================================================",
        patch_eol,
        "--- A/mu\t(revision 0)",
        patch_eol,
        "+++ A/mu\t(revision 0)",
        patch_eol,
        "@@ -1,5 +1,6 @@",
        patch_eol,
        " We wish to congratulate you over your email success in our computer",
        patch_eol,
        " Balloting. This is a Millennium Scientific Electronic Computer Draw",
        patch_eol,
        "+A new line here",
        patch_eol,
        " in which email addresses were used. All participants were selected",
        patch_eol,
        " through a computer ballot system drawn from over 100,000 company",
        patch_eol,
        " and 50,000,000 individual email addresses from all over the world.",
        patch_eol,
      ]

      mu_contents = [
        "We wish to congratulate you over your email success in our computer",
        target_eol,
        "Balloting. This is a Millennium Scientific Electronic Computer Draw",
        target_eol,
        "A new line here",
        target_eol,
        "in which email addresses were used. All participants were selected",
        target_eol,
        "through a computer ballot system drawn from over 100,000 company",
        target_eol,
        "and 50,000,000 individual email addresses from all over the world.",
        target_eol,
        "It is a promotional program aimed at encouraging internet users;",
        target_eol,
      ]

      svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

      expected_output = [
        'G         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
      ]
      expected_disk = svntest.main.greek_state.copy()
      expected_disk.tweak('A/mu', contents=''.join(mu_contents),
                          props={'svn:eol-style' : target_eol_style})

      expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
      expected_status.tweak('A/mu', status='MM', wc_rev=1)

      expected_skip = wc.State('', { })

      svntest.actions.run_and_verify_patch(wc_dir,
                                           os.path.abspath(patch_file_path),
                                           expected_output,
                                           expected_disk,
                                           expected_status,
                                           expected_skip,
                                           None, # expected err
                                           1, # check-props
                                           1) # dry-run

      expected_output = ["Reverted '" + mu_path + "'\n"]
      svntest.actions.run_and_verify_svn(None, expected_output, [], 'revert', '-R', wc_dir)

def patch_with_ignore_whitespace(sbox):
  "ignore whitespace when patching"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company \n",
    "and 50,000,000\t\tindividual email addresses from all over the world. \n",
    " \n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch with leading and trailing spaces removed and tabs transformed
  # to spaces. The patch should match and the hunks should be written to the
  # target as-is.

  unidiff_patch = [
    "Index: A/mu\n",
    "===================================================================\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "@@ -14,11 +17,8 @@\n",
    "BATCH NUMBERS :\n",
    "EULO/1007/444/606/08;\n",
    "SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "BATCH NUMBERS :\n",
    "EULO/1007/444/606/08;\n",
    "SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1, # dry-run
                                       "--ignore-whitespace",)

def patch_replace_locally_deleted_file(sbox):
  "patch that replaces a locally deleted file"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Locally delete mu
  svntest.main.run_svn(None, 'rm', mu_path)

  # Apply patch that re-creates mu

  unidiff_patch = [
    "===================================================================\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  mu_contents = "new\n"

  expected_output = [
    'A         %s\n' % mu_path,
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='R ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run
# Regression test for #3643
def patch_no_eol_at_eof(sbox):
  "patch with no eol at eof"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')

  iota_contents = [
    "One line\n",
    "Another line\n",
    "A third line \n",
    "This is the file 'iota'.\n",
    "A line after\n",
    "Another line after\n",
    "The last line with missing eol",
  ]

  svntest.main.file_write(iota_path, ''.join(iota_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'iota'  : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  unidiff_patch = [
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1,7 +1,7 @@\n",
    " One line\n",
    " Another line\n",
    " A third line \n",
    "-This is the file 'iota'.\n",
    "+It is the file 'iota'.\n",
    " A line after\n",
    " Another line after\n",
    " The last line with missing eol\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  iota_contents = [
    "One line\n",
    "Another line\n",
    "A third line \n",
    "It is the file 'iota'.\n",
    "A line after\n",
    "Another line after\n",
    "The last line with missing eol\n",
  ]
  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=''.join(iota_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_with_properties(sbox):
  "patch with properties"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')

  modified_prop_contents = "This is the property 'modified'.\n"
  deleted_prop_contents = "This is the property 'deleted'.\n"

  # Set iota prop contents
  svntest.main.run_svn(None, 'propset', 'modified', modified_prop_contents,
                       iota_path)
  svntest.main.run_svn(None, 'propset', 'deleted', deleted_prop_contents,
                       iota_path)
  expected_output = svntest.wc.State(wc_dir, {
      'iota'    : Item(verb='Sending'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  # Apply patch

  unidiff_patch = [
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "Property changes on: iota\n",
    "-------------------------------------------------------------------\n",
    "Modified: modified\n",
    "## -1 +1 ##\n",
    "-This is the property 'modified'.\n",
    "+The property 'modified' has changed.\n",
    "Added: added\n",
    "## -0,0 +1 ##\n",
    "+This is the property 'added'.\n",
    "Deleted: deleted\n",
    "## -1 +0,0 ##\n",
    "-This is the property 'deleted'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  modified_prop_contents = "The property 'modified' has changed.\n"
  added_prop_contents = "This is the property 'added'.\n"

  expected_output = [
    ' U        %s\n' % os.path.join(wc_dir, 'iota'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', props={'modified' : modified_prop_contents,
                                     'added' : added_prop_contents})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status=' M', wc_rev='2')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_same_twice(sbox):
  "apply the same patch twice"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  beta_path = os.path.join(wc_dir, 'A', 'B', 'E', 'beta')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
    "\n",
    "Index: new\n",
    "===================================================================\n",
    "--- new	(revision 0)\n",
    "+++ new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(revision 1)\n",
    "+++ A/B/E/beta	(working copy)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % beta_path,
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run
  # apply the patch again
  expected_output = [
    'G         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    '>         hunk @@ -1,1 +1,1 @@ already applied\n',
    'G         %s\n' % os.path.join(wc_dir, 'iota'),
    # The iota patch inserts a line after the first line in the file,
    # with no trailing context. Currently, Subversion applies this patch
    # multiple times, which matches the behaviour of Larry Wall's patch
    # implementation. A more complicated hunk matching algorithm is needed
    # to detect the duplicate application in this case. GNU patch does detect
    # the duplicate application. Should Subversion be taught to detect it,
    # we need this line here:
    # '>         hunk @@ -1,1 +1,2 @@ already applied\n',
    'G         %s\n' % os.path.join(wc_dir, 'new'),
    '>         hunk @@ -0,0 +1,1 @@ already applied\n',
    'G         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    '>         hunk @@ -6,6 +6,9 @@ already applied\n',
    '>         hunk @@ -14,11 +17,8 @@ already applied\n',
    'Skipped \'%s\'\n' % beta_path,
    'Summary of conflicts:\n',
    '  Skipped paths: 1\n',
  ]

  expected_skip = wc.State('', {beta_path : Item()})

  # See above comment about the iota patch being applied twice.
  iota_contents += "Some more bytes\n"
  expected_disk.tweak('iota', contents=iota_contents)

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_dir_properties(sbox):
  "patch with dir properties"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  B_path = os.path.join(wc_dir, 'A', 'B')

  modified_prop_contents = "This is the property 'modified'.\n"
  deleted_prop_contents = "This is the property 'deleted'.\n"

  # Set the properties
  svntest.main.run_svn(None, 'propset', 'modified', modified_prop_contents,
                       wc_dir)
  svntest.main.run_svn(None, 'propset', 'deleted', deleted_prop_contents,
                       B_path)
  expected_output = svntest.wc.State(wc_dir, {
      '.'    : Item(verb='Sending'),
      'A/B'    : Item(verb='Sending'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', wc_rev=2)
  expected_status.tweak('A/B', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  # Apply patch

  unidiff_patch = [
    "Index: .\n",
    "===================================================================\n",
    "--- .\t(revision 1)\n",
    "+++ .\t(working copy)\n",
    "\n",
    "Property changes on: .\n",
    "-------------------------------------------------------------------\n",
    "Modified: modified\n",
    "## -1 +1 ##\n",
    "-This is the property 'modified'.\n",
    "+The property 'modified' has changed.\n",
    "Added: svn:ignore\n",
    "## -0,0 +1,3 ##\n",
    "+*.o\n",
    "+.libs\n",
    "+*.lo\n",
    "Index: A/B\n",
    "===================================================================\n",
    "--- A/B\t(revision 1)\n",
    "+++ A/B\t(working copy)\n",
    "\n",
    "Property changes on: A/B\n",
    "-------------------------------------------------------------------\n",
    "Deleted: deleted\n",
    "## -1 +0,0 ##\n",
    "-This is the property 'deleted'.\n",
    "Added: svn:executable\n",
    "## -0,0 +1 ##\n",
    "+*\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  modified_prop_contents = "The property 'modified' has changed.\n"
  ignore_prop_contents = "*.o\n.libs\n*.lo\n"

  expected_output = [
    ' U        %s\n' % wc_dir,
    ' C        %s\n' % os.path.join(wc_dir, 'A', 'B'),
    'Summary of conflicts:\n',
    '  Property conflicts: 1\n',
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    '' : Item(props={'modified' : modified_prop_contents,
                     'svn:ignore' : ignore_prop_contents}),
    'A/B.svnpatch.rej'  : Item(contents="--- A/B\n+++ A/B\n" +
                               "Property: svn:executable\n" +
                               "## -0,0 +1,1 ##\n+*\n"),
    })
  expected_disk.tweak('A/B', props={})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', status=' M', wc_rev=2)
  expected_status.tweak('A/B', status=' M', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_add_path_with_props(sbox):
  "patch that adds paths with props"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')

  # Apply patch that adds two files, one of which is empty.
  # Both files have properties.

  unidiff_patch = [
    "Index: new\n",
    "===================================================================\n",
    "--- new\t(revision 0)\n",
    "+++ new\t(working copy)\n",
    "@@ -0,0 +1 @@\n",
    "+This is the file 'new'\n",
    "\n",
    "Property changes on: new\n",
    "-------------------------------------------------------------------\n",
    "Added: added\n",
    "## -0,0 +1 ##\n",
    "+This is the property 'added'.\n",
    "Index: X\n",
    "===================================================================\n",
    "--- X\t(revision 0)\n",
    "+++ X\t(working copy)\n",
    "\n",
    "Property changes on: X\n",
    "-------------------------------------------------------------------\n",
    "Added: added\n",
    "## -0,0 +1 ##\n",
    "+This is the property 'added'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  added_prop_contents = "This is the property 'added'.\n"

  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'A         %s\n' % os.path.join(wc_dir, 'X'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'new': Item(contents="This is the file 'new'\n",
                                 props={'added' : added_prop_contents})})
  expected_disk.add({'X': Item(contents="",
                               props={'added' : added_prop_contents})})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'new': Item(status='A ', wc_rev='0')})
  expected_status.add({'X': Item(status='A ', wc_rev='0')})

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_prop_offset(sbox):
  "property patch with offset searching"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')

  prop1_content = ''.join([
    "Dear internet user,\n",
    # The missing line here will cause the first hunk to match early
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n",
  ])

  # prop2's content will make both a late and early match possible.
  # The hunk to be applied is replicated here for reference:
  # ## -5,6 +5,7 ##
  #  property
  #  property
  #  property
  # +x
  #  property
  #  property
  #  property
  #
  # This hunk wants to be applied at line 5, but that isn't
  # possible because line 8 ("zzz") does not match "property".
  # The early match happens at line 2 (offset 3 = 5 - 2).
  # The late match happens at line 9 (offset 4 = 9 - 5).
  # Subversion will pick the early match in this case because it
  # is closer to line 5.
  prop2_content = ''.join([
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "zzz\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n"
  ])

  # Set iota prop contents
  svntest.main.run_svn(None, 'propset', 'prop1', prop1_content,
                       iota_path)
  svntest.main.run_svn(None, 'propset', 'prop2', prop2_content,
                       iota_path)
  expected_output = svntest.wc.State(wc_dir, {
    'iota'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: iota\n",
    "===================================================================\n",
    "--- iota	(revision XYZ)\n",
    "+++ iota	(working copy)\n",
    "\n",
    "Property changes on: iota\n",
    "-------------------------------------------------------------------\n",
    "Modified: prop1\n",
    "## -6,6 +6,9 ##\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "## -14,11 +17,8 ##\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Modified: prop2\n",
    "## -5,6 +5,7 ##\n",
    " property\n",
    " property\n",
    " property\n",
    "+x\n",
    " property\n",
    " property\n",
    " property\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  prop1_content = ''.join([
    "Dear internet user,\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "These extra lines will cause the second hunk to match late\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ])

  prop2_content = ''.join([
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "x\n",
    "property\n",
    "property\n",
    "property\n",
    "zzz\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
    "property\n",
  ])

  os.chdir(wc_dir)

  # Changing two properties so output order not well defined.
  expected_output = svntest.verify.UnorderedOutput([
    ' U        iota\n',
    '>         applied hunk ## -6,6 +6,9 ## with offset -1 (prop1)\n',
    '>         applied hunk ## -14,11 +17,8 ## with offset 4 (prop1)\n',
    '>         applied hunk ## -5,6 +5,7 ## with offset -3 (prop2)\n',
  ])

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', props = {'prop1' : prop1_content,
                                       'prop2' : prop2_content})

  expected_status = svntest.actions.get_virginal_state('.', 1)
  expected_status.tweak('iota', status=' M', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch('.', os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_prop_with_fuzz(sbox):
  "property patch with fuzz"

  sbox.build()
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # We have replaced a couple of lines to cause fuzz. Those lines contains
  # the word fuzz
  prop_contents = ''.join([
    "Line replaced for fuzz = 1\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "Line replaced for fuzz = 2 with only the second context line changed\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "This line is inserted to cause an offset of +1\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Line replaced for fuzz = 2\n",
    "Line replaced for fuzz = 2\n",
  ])

  # Set mu prop contents
  svntest.main.run_svn(None, 'propset', 'prop', prop_contents,
                       mu_path)
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                      expected_status, None, wc_dir)

  unidiff_patch = [
    "Index: mu\n",
    "===================================================================\n",
    "--- A/mu\t(revision 0)\n",
    "+++ A/mu\t(revision 0)\n",
    "\n",
    "Property changes on: mu\n",
    "Modified: prop\n",
    "## -1,6 +1,7 ##\n",
    " Dear internet user,\n",
    " \n",
    " We wish to congratulate you over your email success in our computer\n",
    "+A new line here\n",
    " Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    " in which email addresses were used. All participants were selected\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    "## -7,7 +8,9 ##\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    "+Another new line\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "+A third new line\n",
    " file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "## -19,6 +20,7 ##\n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "+A fourth new line\n",
    " \n",
    " Again, we wish to congratulate you over your email success in our\n"
    " computer Balloting. [No trailing newline here]"
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  prop_contents = ''.join([
    "Line replaced for fuzz = 1\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "A new line here\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "Line replaced for fuzz = 2 with only the second context line changed\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "Another new line\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "A third new line\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "This line is inserted to cause an offset of +1\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "A fourth new line\n",
    "\n",
    "Line replaced for fuzz = 2\n",
    "Line replaced for fuzz = 2\n",
  ])

  expected_output = [
    ' U        %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    '>         applied hunk ## -1,6 +1,7 ## with fuzz 1 (prop)\n',
    '>         applied hunk ## -7,7 +8,9 ## with fuzz 2 (prop)\n',
    '>         applied hunk ## -19,6 +20,7 ## with offset 1 and fuzz 2 (prop)\n',
  ]
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', props = {'prop' : prop_contents})

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status=' M', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_git_empty_files(sbox):
  "patch that contains empty files"

  sbox.build()
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  new_path = os.path.join(wc_dir, 'new')

  unidiff_patch = [
    "Index: new\n",
    "===================================================================\n",
    "diff --git a/new b/new\n",
    "new file mode 10644\n",
    "Index: iota\n",
    "===================================================================\n",
    "diff --git a/iota b/iota\n",
    "deleted file mode 10644\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'D         %s\n' % os.path.join(wc_dir, 'iota'),
  ]
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'new' : Item(contents="")})
  expected_disk.remove('iota')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('iota', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_old_target_names(sbox):
  "patch using old target names"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "--- A/mu	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu.new	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  mu_contents = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_reverse_revert(sbox):
  "revert a patch by reverse patching"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  mu_contents_pre_patch = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 13th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
    "\n",
    "Again, we wish to congratulate you over your email success in our\n"
    "computer Balloting.\n"
  ]

  # Set mu contents
  svntest.main.file_write(mu_path, ''.join(mu_contents_pre_patch), 'wb')
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'       : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch

  unidiff_patch = [
    "Index: A/D/gamma\n",
    "===================================================================\n",
    "--- A/D/gamma\t(revision 1)\n",
    "+++ A/D/gamma\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file 'gamma'.\n",
    "+It is the file 'gamma'.\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes\n",
    "\n",
    "Index: new\n",
    "===================================================================\n",
    "--- new	(revision 0)\n",
    "+++ new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "--- A/mu.orig	2009-06-24 15:23:55.000000000 +0100\n",
    "+++ A/mu	2009-06-24 15:21:23.000000000 +0100\n",
    "@@ -6,6 +6,9 @@\n",
    " through a computer ballot system drawn from over 100,000 company\n",
    " and 50,000,000 individual email addresses from all over the world.\n",
    " \n",
    "+It is a promotional program aimed at encouraging internet users;\n",
    "+therefore you do not need to buy ticket to enter for it.\n",
    "+\n",
    " Your email address drew and have won the sum of  750,000 Euros\n",
    " ( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    " file with\n",
    "@@ -14,11 +17,8 @@\n",
    "     BATCH NUMBERS :\n",
    "     EULO/1007/444/606/08;\n",
    "     SERIAL NUMBER: 45327\n",
    "-and PROMOTION DATE: 13th June. 2009\n",
    "+and PROMOTION DATE: 14th June. 2009\n",
    " \n",
    " To claim your winning prize, you are to contact the appointed\n",
    " agent below as soon as possible for the immediate release of your\n",
    " winnings with the below details.\n",
    "-\n",
    "-Again, we wish to congratulate you over your email success in our\n",
    "-computer Balloting.\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(revision 1)\n",
    "+++ A/B/E/beta	(working copy)\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch), 'wb')

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\nSome more bytes\n"
  new_contents = "new\n"
  mu_contents_post_patch = [
    "Dear internet user,\n",
    "\n",
    "We wish to congratulate you over your email success in our computer\n",
    "Balloting. This is a Millennium Scientific Electronic Computer Draw\n",
    "in which email addresses were used. All participants were selected\n",
    "through a computer ballot system drawn from over 100,000 company\n",
    "and 50,000,000 individual email addresses from all over the world.\n",
    "\n",
    "It is a promotional program aimed at encouraging internet users;\n",
    "therefore you do not need to buy ticket to enter for it.\n",
    "\n",
    "Your email address drew and have won the sum of  750,000 Euros\n",
    "( Seven Hundred and Fifty Thousand Euros) in cash credited to\n",
    "file with\n",
    "    REFERENCE NUMBER: ESP/WIN/008/05/10/MA;\n",
    "    WINNING NUMBER : 14-17-24-34-37-45-16\n",
    "    BATCH NUMBERS :\n",
    "    EULO/1007/444/606/08;\n",
    "    SERIAL NUMBER: 45327\n",
    "and PROMOTION DATE: 14th June. 2009\n",
    "\n",
    "To claim your winning prize, you are to contact the appointed\n",
    "agent below as soon as possible for the immediate release of your\n",
    "winnings with the below details.\n",
  ]

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/D/gamma', contents=gamma_contents)
  expected_disk.tweak('iota', contents=iota_contents)
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.tweak('A/mu', contents=''.join(mu_contents_post_patch))
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/D/gamma', status='M ')
  expected_status.tweak('iota', status='M ')
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/mu', status='M ', wc_rev=2)
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

  # Applying the same patch in reverse should undo local mods
  expected_output = [
    'G         %s\n' % os.path.join(wc_dir, 'A', 'D', 'gamma'),
    'G         %s\n' % os.path.join(wc_dir, 'iota'),
    'D         %s\n' % os.path.join(wc_dir, 'new'),
    'G         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'A         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu', contents=''.join(mu_contents_pre_patch))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)

  ### svn patch should check whether the deleted file has the same
  ### content as the file added by the patch and revert the deletion
  ### instead of causing a replacement.
  expected_status.tweak('A/B/E/beta', status='R ')

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1, # dry-run
                                       '--reverse-diff')

def patch_one_property(sbox, trailing_eol):
  """Helper.  Apply a patch that sets the property 'k' to 'v\n' or to 'v',
   and check the results."""

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Apply patch

  unidiff_patch = [
    "Index: .\n",
    "===================================================================\n",
    "diff --git a/subversion/branches/1.6.x b/subversion/branches/1.6.x\n",
    "--- a/subversion/branches/1.6.x\t(revision 1033278)\n",
    "+++ b/subversion/branches/1.6.x\t(working copy)\n",
    "\n",
    "Property changes on: subversion/branches/1.6.x\n",
    "___________________________________________________________________\n",
    "Modified: svn:mergeinfo\n",
    "   Merged /subversion/trunk:r964349\n",
    "Added: k\n",
    "## -0,0 +1 ##\n",
    "+v\n",
  ]

  if trailing_eol:
    value = "v\n"
  else:
    value = "v"
    unidiff_patch += ['\ No newline at end of property']

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    ' U        %s\n' % os.path.join(wc_dir),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'': Item(props={'k' : value})})

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', status=' M')

  expected_skip = wc.State('.', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1, # dry-run
                                       '--strip', '3')

  if is_os_windows():
    # On Windows 'svn pg' uses \r\n as EOL.
    value = value.replace('\n', '\r\n')

  svntest.actions.check_prop('k', wc_dir, [value])

def patch_strip_cwd(sbox):
  "patch --strip propchanges cwd"
  return patch_one_property(sbox, True)

@Issue(3814)
def patch_set_prop_no_eol(sbox):
  "patch doesn't append newline to properties"
  return patch_one_property(sbox, False)

# Regression test for issue #3697
@SkipUnless(svntest.main.is_posix_os)
@Issue(3697)
def patch_add_symlink(sbox):
  "patch that adds a symlink"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  # Apply patch

  unidiff_patch = [
    "Index: iota_symlink\n",
    "===================================================================\n",
    "--- iota_symlink\t(revision 0)\n",
    "+++ iota_symlink\t(working copy)\n",
    "@@ -0,0 +1 @@\n",
    "+link iota\n",
    "\n",
    "Property changes on: iota_symlink\n",
    "-------------------------------------------------------------------\n",
    "Added: svn:special\n",
    "## -0,0 +1 ##\n",
    "+*\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'iota_symlink'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'iota_symlink': Item(contents="This is the file 'iota'.\n",
                                          props={'svn:special' : '*'})})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'iota_symlink': Item(status='A ', wc_rev='0')})

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run


@Issue(4003)
def patch_deletes_prop(sbox):
  "patch deletes prop, directly and via reversed add"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')

  svntest.main.run_svn(None, 'propset', 'propname', 'propvalue',
                       iota_path)
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Apply patch
  unidiff_patch = [
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "\n",
    "Property changes on: iota\n",
    "___________________________________________________________________\n",
    "Deleted: propname\n",
    "## -1 +0,0 ##\n",
    "-propvalue\n",
    ]
  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  # Expect the original state of the working copy in r1, exception
  # that iota is at r2 now.
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status=' M')
  expected_status.tweak('iota', wc_rev=2)
  expected_skip = wc.State('', { })
  expected_output = [
    ' U        %s\n' % os.path.join(wc_dir, 'iota'),
  ]
  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       0) # dry-run

  # Revert any local mods, then try to reverse-apply a patch which
  # *adds* the property.
  svntest.main.run_svn(None, 'revert', iota_path)

  # Apply patch 
  unidiff_patch = [
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    "\n",
    "Property changes on: iota\n",
    "___________________________________________________________________\n",
    "Added: propname\n",
    "## -0,0 +1 ##\n",
    "+propvalue\n",
    ]
  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       0, # dry-run
                                       '--reverse-diff') 

@Issue(4004)
def patch_reversed_add_with_props(sbox):
  "reverse patch new file+props atop uncommitted"

  sbox.build()
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  # Add a new file which also has props set on it.
  newfile_path = os.path.join(wc_dir, 'newfile')
  newfile_contents = ["This is the file 'newfile'.\n"]
  svntest.main.file_write(newfile_path, ''.join(newfile_contents))
  svntest.main.run_svn(None, 'add', newfile_path)
  svntest.main.run_svn(None, 'propset', 'propname', 'propvalue',
                       newfile_path)

  # Generate a patch file from our current diff (rooted at the working
  # copy root).
  cwd = os.getcwd()
  try:
    os.chdir(wc_dir)
    exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff')
  finally:
    os.chdir(cwd)
  svntest.main.file_write(patch_file_path, ''.join(diff_output))

  # Okay, now commit up.
  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    })

  # Now, we'll try to reverse-apply the very diff we just created.  We
  # expect the original state of the working copy in r1.
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_skip = wc.State('', { })
  expected_output = [
    'D         %s\n' % newfile_path,
  ]
  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       0, # dry-run
                                       '--reverse-diff') 

@Issue(4004)
def patch_reversed_add_with_props2(sbox):
  "reverse patch new file+props"

  sbox.build()
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  # Add a new file which also has props set on it.
  newfile_path = os.path.join(wc_dir, 'newfile')
  newfile_contents = ["This is the file 'newfile'.\n"]
  svntest.main.file_write(newfile_path, ''.join(newfile_contents))
  svntest.main.run_svn(None, 'add', newfile_path)
  svntest.main.run_svn(None, 'propset', 'propname', 'propvalue',
                       newfile_path)

  # Generate a patch file from our current diff (rooted at the working
  # copy root).
  cwd = os.getcwd()
  try:
    os.chdir(wc_dir)
    exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff')
  finally:
    os.chdir(cwd)
  svntest.main.file_write(patch_file_path, ''.join(diff_output))

  # Okay, now commit up.
  expected_output = svntest.wc.State(wc_dir, {
    'newfile' : Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'newfile' : Item(wc_rev=2, status='  ')})
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Now, we'll try to reverse-apply the very diff we just created.  We
  # expect the original state of the working copy in r1 plus 'newfile'
  # scheduled for deletion.
  expected_disk = svntest.main.greek_state.copy()
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'newfile' : Item(status='D ', wc_rev=2)})
  expected_skip = wc.State('', { })
  expected_output = [
    'D         %s\n' % newfile_path,
  ]
  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       0, # dry-run
                                       '--reverse-diff') 

def patch_dev_null(sbox):
  "patch with /dev/null filenames"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  # Git (and maybe other tools) use '/dev/null' as the old path for
  # newly added files, and as the new path for deleted files.
  # The path selection algorithm in 'svn patch' must detect this and
  # avoid using '/dev/null' as a patch target.
  unidiff_patch = [
    "Index: new\n",
    "===================================================================\n",
    "--- /dev/null\n",
    "+++ new	(revision 0)\n",
    "@@ -0,0 +1 @@\n",
    "+new\n",
    "\n",
    "Index: A/B/E/beta\n",
    "===================================================================\n",
    "--- A/B/E/beta	(revision 1)\n",
    "+++ /dev/null\n",
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  new_contents = "new\n"
  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'new'),
    'D         %s\n' % os.path.join(wc_dir, 'A', 'B', 'E', 'beta'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'new' : Item(contents=new_contents)})
  expected_disk.remove('A/B/E/beta')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({'new' : Item(status='A ', wc_rev=0)})
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

@Issue(4049)
def patch_delete_and_skip(sbox):
  "patch that deletes and skips"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)

  os.chdir(wc_dir)

  # We need to use abspaths to trigger the segmentation fault.
  abs = os.path.abspath('.')
  if sys.platform == 'win32':
      abs = abs.replace("\\", "/")

  outside_wc = os.path.join(os.pardir, 'X')
  if sys.platform == 'win32':
      outside_wc = outside_wc.replace("\\", "/")

  unidiff_patch = [
    "Index: %s/A/B/E/alpha\n" % abs,
    "===================================================================\n",
    "--- %s/A/B/E/alpha\t(revision 1)\n" % abs,
    "+++ %s/A/B/E/alpha\t(working copy)\n" % abs,
    "@@ -1 +0,0 @@\n",
    "-This is the file 'alpha'.\n",
    "Index: %s/A/B/E/beta\n" % abs,
    "===================================================================\n",
    "--- %s/A/B/E/beta\t(revision 1)\n" % abs,
    "+++ %s/A/B/E/beta\t(working copy)\n" % abs,
    "@@ -1 +0,0 @@\n",
    "-This is the file 'beta'.\n",
    "Index: %s/A/B/E/out-of-reach\n" % abs,
    "===================================================================\n",
    "--- %s/iota\t(revision 1)\n" % outside_wc,
    "+++ %s/iota\t(working copy)\n" % outside_wc,
    "\n",
    "Property changes on: iota\n",
    "___________________________________________________________________\n",
    "Added: propname\n",
    "## -0,0 +1 ##\n",
    "+propvalue\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  skipped_path = os.path.join(os.pardir, 'X', 'iota')
  expected_output = [
    'D         %s\n' % os.path.join('A', 'B', 'E', 'alpha'),
    'D         %s\n' % os.path.join('A', 'B', 'E', 'beta'),
    'Skipped missing target: \'%s\'\n' % skipped_path,
    'D         %s\n' % os.path.join('A', 'B', 'E'),
    'Summary of conflicts:\n',
    '  Skipped paths: 1\n'
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B/E/alpha')
  expected_disk.remove('A/B/E/beta')
  expected_disk.remove('A/B/E')

  expected_status = svntest.actions.get_virginal_state('.', 1)
  expected_status.tweak('A/B/E', status='D ')
  expected_status.tweak('A/B/E/alpha', status='D ')
  expected_status.tweak('A/B/E/beta', status='D ')

  expected_skip = wc.State('', {skipped_path: Item()})

  svntest.actions.run_and_verify_patch('.', os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

@Issue(3991)
def patch_lacking_trailing_eol(sbox):
  "patch file lacking trailing eol"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # Prepare
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)

  # Apply patch
  unidiff_patch = [
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 1)\n",
    "+++ iota\t(working copy)\n",
    # TODO: -1 +1
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Some more bytes", # No trailing \n on this line!
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  gamma_contents = "It is the file 'gamma'.\n"
  iota_contents = "This is the file 'iota'.\n"
  new_contents = "new\n"

  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
  ]

  # Expect a newline to be appended
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=iota_contents + "Some more bytes")

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='M ')

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_target_no_eol_at_eof(sbox):
  "patch target with no eol at eof"

  sbox.build()
  wc_dir = sbox.wc_dir

  patch_file_path = make_patch_path(sbox)
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = sbox.ospath('A/mu')

  iota_contents = [
    "This is the file iota."
  ]

  mu_contents = [
    "context\n",
    "context\n",
    "context\n",
    "context\n",
    "This is the file mu.\n",
    "context\n",
    "context\n",
    "context\n",
    "context", # no newline at end of file
  ]

  svntest.main.file_write(iota_path, ''.join(iota_contents))
  svntest.main.file_write(mu_path, ''.join(mu_contents))
  expected_output = svntest.wc.State(wc_dir, {
    'iota'  : Item(verb='Sending'),
    'A/mu'  : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  expected_status.tweak('A/mu', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)
  unidiff_patch = [
    "Index: A/mu\n",
    "===================================================================\n",
    "--- A/mu\t(revision 2)\n",
    "+++ A/mu\t(working copy)\n",
    "@@ -2,8 +2,8 @@ context\n",
    " context\n",
    " context\n",
    " context\n",
    "-This is the file mu.\n",
    "+It is really the file mu.\n",
    " context\n",
    " context\n",
    " context\n",
    " context\n",
    "\\ No newline at end of file\n",
    "Index: iota\n",
    "===================================================================\n",
    "--- iota\t(revision 2)\n",
    "+++ iota\t(working copy)\n",
    "@@ -1 +1 @@\n",
    "-This is the file iota.\n",
    "\\ No newline at end of file\n",
    "+It is really the file 'iota'.\n",
    "\\ No newline at end of file\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  iota_contents = [
    "It is really the file 'iota'."
  ]
  mu_contents = [
    "context\n",
    "context\n",
    "context\n",
    "context\n",
    "It is really the file mu.\n",
    "context\n",
    "context\n",
    "context\n",
    "context", # no newline at end of file
  ]
  expected_output = [
    'U         %s\n' % os.path.join(wc_dir, 'A', 'mu'),
    'U         %s\n' % os.path.join(wc_dir, 'iota'),
  ]

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=''.join(iota_contents))
  expected_disk.tweak('A/mu', contents=''.join(mu_contents))

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='M ', wc_rev=2)
  expected_status.tweak('A/mu', status='M ', wc_rev=2)

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

def patch_add_and_delete(sbox):
  "patch add multiple levels and delete"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  unidiff_patch = [
    "Index: foo\n",
    "===================================================================\n",
    "--- P/Q/foo\t(revision 0)\n"
    "+++ P/Q/foo\t(working copy)\n"
    "@@ -0,0 +1 @@\n",
    "+This is the file 'foo'.\n",
    "Index: iota\n"
    "===================================================================\n",
    "--- iota\t(revision 1)\n"
    "+++ iota\t(working copy)\n"
    "@@ -1 +0,0 @@\n",
    "-This is the file 'iota'.\n",
    ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'A         %s\n' % os.path.join(wc_dir, 'P'),
    'A         %s\n' % os.path.join(wc_dir, 'P', 'Q'),
    'A         %s\n' % os.path.join(wc_dir, 'P', 'Q', 'foo'),
    'D         %s\n' % os.path.join(wc_dir, 'iota'),
  ]
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('iota')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_disk.add({'P/Q/foo' : Item(contents="This is the file 'foo'.\n")})
  expected_status.tweak('iota', status='D ')
  expected_status.add({
      'P'       : Item(status='A ', wc_rev=0),
      'P/Q'     : Item(status='A ', wc_rev=0),
      'P/Q/foo' : Item(status='A ', wc_rev=0),
      })
  expected_skip = wc.State('', { })

  # Failed with "The node 'P' was not found" when erroneously checking
  # whether 'P/Q' should be deleted.
  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run


def patch_git_with_index_line(sbox):
  "apply git patch with 'index' line"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  patch_file_path = make_patch_path(sbox)

  unidiff_patch = [
    "diff --git a/src/tools/ConsoleRunner/hi.txt b/src/tools/ConsoleRunner/hi.txt\n",
    "new file mode 100644\n",
    "index 0000000..c82a38f\n",
    "--- /dev/null\n",
    "+++ b/src/tools/ConsoleRunner/hi.txt\n",
    "@@ -0,0 +1 @@\n",
    "+hihihihihihi\n",
    "\ No newline at end of file\n",
  ]

  svntest.main.file_write(patch_file_path, ''.join(unidiff_patch))

  expected_output = [
    'A         %s\n' % sbox.ospath('src'),
    'A         %s\n' % sbox.ospath('src/tools'),
    'A         %s\n' % sbox.ospath('src/tools/ConsoleRunner'),
    'A         %s\n' % sbox.ospath('src/tools/ConsoleRunner/hi.txt'),
  ]

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
      'src/'                            : Item(status='A ', wc_rev=0),
      'src/tools'                       : Item(status='A ', wc_rev=0),
      'src/tools/ConsoleRunner/'        : Item(status='A ', wc_rev=0),
      'src/tools/ConsoleRunner/hi.txt'  : Item(status='A ', wc_rev=0),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'src'                            : Item(),
                     'src/tools'                      : Item(),
                     'src/tools/ConsoleRunner'        : Item(),
                     'src/tools/ConsoleRunner/hi.txt' :
                        Item(contents="hihihihihihi")
                   })

  expected_skip = wc.State('', { })

  svntest.actions.run_and_verify_patch(wc_dir, os.path.abspath(patch_file_path),
                                       expected_output,
                                       expected_disk,
                                       expected_status,
                                       expected_skip,
                                       None, # expected err
                                       1, # check-props
                                       1) # dry-run

########################################################################
#Run the tests

# list all tests here, starting with None:
test_list = [ None,
              patch,
              patch_absolute_paths,
              patch_offset,
              patch_chopped_leading_spaces,
              patch_strip1,
              patch_no_index_line,
              patch_add_new_dir,
              patch_remove_empty_dirs,
              patch_reject,
              patch_keywords,
              patch_with_fuzz,
              patch_reverse,
              patch_no_svn_eol_style,
              patch_with_svn_eol_style,
              patch_with_svn_eol_style_uncommitted,
              patch_with_ignore_whitespace,
              patch_replace_locally_deleted_file,
              patch_no_eol_at_eof,
              patch_with_properties,
              patch_same_twice,
              patch_dir_properties,
              patch_add_path_with_props,
              patch_prop_offset,
              patch_prop_with_fuzz,
              patch_git_empty_files,
              patch_old_target_names,
              patch_reverse_revert,
              patch_strip_cwd,
              patch_set_prop_no_eol,
              patch_add_symlink,
              patch_deletes_prop,
              patch_reversed_add_with_props,
              patch_reversed_add_with_props2,
              patch_dev_null,
              patch_delete_and_skip,
              patch_lacking_trailing_eol,
              patch_target_no_eol_at_eof,
              patch_add_and_delete,
              patch_git_with_index_line,
            ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
