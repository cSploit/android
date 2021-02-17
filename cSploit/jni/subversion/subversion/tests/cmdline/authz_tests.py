#!/usr/bin/env python
#
#  authz_tests.py:  testing authentication.
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
import os

# Our testing module
import svntest

from svntest.main import write_restrictive_svnserve_conf
from svntest.main import write_authz_file
from svntest.main import server_authz_has_aliases
from upgrade_tests import (replace_sbox_with_tarfile,
                           replace_sbox_repo_with_tarfile,
                           wc_is_too_old_regex)

# (abbreviation)
Item = svntest.wc.StateItem
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

# regression test for issue #2486 - part 1: open_root
@Issue(2486)
@Skip(svntest.main.is_ra_type_file)
def authz_open_root(sbox):
  "authz issue #2486 - open root"

  sbox.build()

  write_authz_file(sbox, {"/": "", "/A": "jrandom = rw"})

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # we have write access in folder /A, but not in root. Test on too
  # restrictive access needed in open_root by modifying a file in /A
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, "hi")

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })

  # Commit the one file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        mu_path)

#----------------------------------------------------------------------

# regression test for issue #2486 - part 2: open_directory
@Issue(2486)
@Skip(svntest.main.is_ra_type_file)
def authz_open_directory(sbox):
  "authz issue #2486 - open directory"

  sbox.build()

  write_authz_file(sbox, {"/": "*=rw", "/A/B": "*=", "/A/B/E": "jrandom = rw"})

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # we have write access in folder /A/B/E, but not in /A/B. Test on too
  # restrictive access needed in open_directory by moving file /A/mu to
  # /A/B/E
  wc_dir = sbox.wc_dir

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  E_path = os.path.join(wc_dir, 'A', 'B', 'E')

  svntest.main.run_svn(None, 'mv', mu_path, E_path)

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Deleting'),
    'A/B/E/mu' : Item(verb='Adding'),
    })

  # Commit the working copy.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        wc_dir)

@Skip(svntest.main.is_ra_type_file)
def broken_authz_file(sbox):
  "broken authz files cause errors"

  sbox.build(create_wc = False)

  # No characters but 'r', 'w', and whitespace are allowed as a value
  # in an authz rule.
  write_authz_file(sbox, {"/": "jrandom = rw  # End-line comments disallowed"})

  write_restrictive_svnserve_conf(sbox.repo_dir)

  exit_code, out, err = svntest.main.run_svn(1,
                                             "delete",
                                             sbox.repo_url + "/A",
                                             "-m", "a log message");
  if out:
    raise svntest.verify.SVNUnexpectedStdout(out)
  if not err:
    raise svntest.verify.SVNUnexpectedStderr("Missing stderr")

# test whether read access is correctly granted and denied
@Skip(svntest.main.is_ra_type_file)
def authz_read_access(sbox):
  "test authz for read operations"

  sbox.build(create_wc = False)

  root_url = sbox.repo_url
  A_url = root_url + '/A'
  B_url = A_url + '/B'
  C_url = A_url + '/C'
  E_url = B_url + '/E'
  mu_url = A_url + '/mu'
  iota_url = root_url + '/iota'
  lambda_url = B_url + '/lambda'
  alpha_url = E_url + '/alpha'
  F_alpha_url = B_url + '/F/alpha'
  D_url = A_url + '/D'
  G_url = D_url + '/G'
  pi_url = G_url + '/pi'
  H_url = D_url + '/H'
  chi_url = H_url + '/chi'
  fws_url = B_url + '/folder with spaces'
  fws_empty_folder_url = fws_url + '/empty folder'

  if sbox.repo_url.startswith("http"):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  # create some folders with spaces in their names
  svntest.actions.run_and_verify_svn(None, None, [], 'mkdir', '-m', 'logmsg',
                                     fws_url, fws_empty_folder_url)

  write_restrictive_svnserve_conf(sbox.repo_dir)

  write_authz_file(sbox, { "/": "* = r",
                           "/A/B": "* =",
                           "/A/B/F": "* = rw",
                           "/A/D": "* = rw",
                           "/A/D/G": ("* = rw\n" +
                                      svntest.main.wc_author + " ="),
                           "/A/D/H": ("* = \n" +
                                      svntest.main.wc_author + " = rw"),
                           "/A/B/folder with spaces":
                                     (svntest.main.wc_author + " = r")})

  # read a remote file
  svntest.actions.run_and_verify_svn(None, ["This is the file 'iota'.\n"],
                                     [], 'cat',
                                     iota_url)

  # read a remote file, readably by user specific exception
  svntest.actions.run_and_verify_svn(None, ["This is the file 'chi'.\n"],
                                     [], 'cat',
                                     chi_url)

  # read a remote file, unreadable: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cat',
                                     lambda_url)

  # read a remote file, unreadable through recursion: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cat',
                                     alpha_url)

  # read a remote file, user specific authorization is ignored because * = rw
  svntest.actions.run_and_verify_svn(None, ["This is the file 'pi'.\n"],
                                     [], 'cat',
                                     pi_url)
  # open a remote folder(ls)
  svntest.actions.run_and_verify_svn("ls remote root folder",
                                     ["A/\n", "iota\n"],
                                     [], 'ls',
                                     root_url)

  # open a remote folder(ls), unreadable: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, svntest.verify.AnyOutput, 'ls',
                                     B_url)

  # open a remote folder(ls) with spaces, should succeed
  svntest.actions.run_and_verify_svn(None,
                                     None, [], 'ls',
                                     fws_empty_folder_url)

  # open a remote folder(ls), unreadable through recursion: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'ls',
                                     E_url)

  # copy a remote file
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     iota_url, D_url,
                                     '-m', 'logmsg')

  # copy a remote file, source is unreadable: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cp',
                                     '-m', 'logmsg',
                                     lambda_url, D_url)

  # copy a remote folder
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     C_url, D_url,
                                     '-m', 'logmsg')

  # copy a remote folder, source is unreadable: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cp',
                                     '-m', 'logmsg',
                                     E_url, D_url)

  # move a remote file, source/target ancestor is readonly: should fail
  #
  # Note: interesting, we deem it okay for someone to break this move
  # into two operations, a committed copy followed by a committed
  # deletion.  But the editor drive required to do these atomically
  # today is prohibitive.
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mv', '-m', 'logmsg',
                                     alpha_url, F_alpha_url)

  ## copy a remote file, source/target ancestor is readonly
  ## we fail here due to issue #3242.
  #svntest.actions.run_and_verify_svn(None,
  #                                   None, [],
  #                                   'cp', '-m', 'logmsg',
  #                                   alpha_url, F_alpha_url)


# test whether write access is correctly granted and denied
@Skip(svntest.main.is_ra_type_file)
def authz_write_access(sbox):
  "test authz for write operations"

  sbox.build(create_wc = False)

  write_restrictive_svnserve_conf(sbox.repo_dir)

  if sbox.repo_url.startswith('http'):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E220004: Access denied.*"

  write_authz_file(sbox, { "/": "* = r",
                           "/A/B": "* = rw",
                           "/A/C": "* = rw"})

  root_url = sbox.repo_url
  A_url = root_url + '/A'
  B_url = A_url + '/B'
  C_url = A_url + '/C'
  E_url = B_url + '/E'
  mu_url = A_url + '/mu'
  iota_url = root_url + '/iota'
  lambda_url = B_url + '/lambda'
  D_url = A_url + '/D'

  # copy a remote file, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cp',
                                     '-m', 'logmsg',
                                     lambda_url, D_url)

  # copy a remote folder, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cp',
                                     '-m', 'logmsg',
                                     E_url, D_url)

  # delete a file, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'rm',
                                     '-m', 'logmsg',
                                     iota_url)

  # delete a folder, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'rm',
                                     '-m', 'logmsg',
                                     D_url)

  # create a folder, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mkdir',
                                     '-m', 'logmsg',
                                     A_url+'/newfolder')

  # move a remote file, source is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mv',
                                     '-m', 'logmsg',
                                     mu_url, C_url)

  # move a remote folder, source is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mv',
                                     '-m', 'logmsg',
                                     D_url, C_url)

  # move a remote file, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mv',
                                     '-m', 'logmsg',
                                     lambda_url, D_url)

  # move a remote folder, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'mv',
                                     '-m', 'logmsg',
                                     B_url, D_url)

#----------------------------------------------------------------------

@Skip(svntest.main.is_ra_type_file)
def authz_checkout_test(sbox):
  "test authz for checkout"

  sbox.build(create_wc = False, read_only = True)
  local_dir = sbox.wc_dir

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # 1st part: disable all read access, checkout should fail

  # write an authz file with *= on /
  if sbox.repo_url.startswith('http'):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  write_authz_file(sbox, { "/": "* ="})

  # checkout a working copy, should fail
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'co', sbox.repo_url, local_dir)

  # 2nd part: now enable read access

  write_authz_file(sbox, { "/": "* = r"})

  # checkout a working copy, should succeed because we have read access
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = local_dir
  expected_output.tweak(status='A ', contents=None)

  expected_wc = svntest.main.greek_state

  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                          local_dir,
                          expected_output,
                          expected_wc)

@Skip(svntest.main.is_ra_type_file)
def authz_checkout_and_update_test(sbox):
  "test authz for checkout and update"

  sbox.build(create_wc = False, read_only = True)
  local_dir = sbox.wc_dir

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # 1st part: disable read access on folder A/B, checkout should not
  # download this folder

  # write an authz file with *= on /A/B and /A/mu.
  write_authz_file(sbox, { "/": "* = r",
                           "/A/B": "* =",
                           "/A/mu": "* =",
                           })

  # checkout a working copy, should not dl /A/B or /A/mu.
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = local_dir
  expected_output.tweak(status='A ', contents=None)
  expected_output.remove('A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                         'A/B/E/beta', 'A/B/F', 'A/mu')

  expected_wc = svntest.main.greek_state.copy()
  expected_wc.remove('A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                     'A/B/E/beta', 'A/B/F', 'A/mu')

  svntest.actions.run_and_verify_checkout(sbox.repo_url, local_dir,
                                          expected_output,
                                          expected_wc)

  # 2nd part: now enable read access

  # write an authz file with *=r on /. continue to exclude mu.
  write_authz_file(sbox, { "/": "* = r",
                           "/A/mu": "* =",
                           })

  # update the working copy, should download /A/B because we now have read
  # access
  expected_output = svntest.wc.State(local_dir, {
    'A/B' : Item(status='A '),
    'A/B/lambda' : Item(status='A '),
    'A/B/E' : Item(status='A '),
    'A/B/E/alpha' : Item(status='A '),
    'A/B/E/beta' : Item(status='A '),
    'A/B/F' : Item(status='A '),
    })

  expected_wc = svntest.main.greek_state.copy()
  expected_wc.remove('A/mu')
  expected_status = svntest.actions.get_virginal_state(local_dir, 1)
  expected_status.remove('A/mu')

  svntest.actions.run_and_verify_update(local_dir,
                                        expected_output,
                                        expected_wc,
                                        expected_status,
                                        None,
                                        None, None,
                                        None, None, 1)

@Skip(svntest.main.is_ra_type_file)
def authz_partial_export_test(sbox):
  "test authz for export with unreadable subfolder"

  sbox.build(create_wc = False, read_only = True)
  local_dir = sbox.wc_dir

  # cleanup remains of a previous test run.
  svntest.main.safe_rmtree(local_dir)

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # 1st part: disable read access on folder A/B, export should not
  # download this folder

  # write an authz file with *= on /A/B
  write_authz_file(sbox, { "/": "* = r", "/A/B": "* =" })

  # export a working copy, should not dl /A/B
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = local_dir
  expected_output.desc[''] = Item()
  expected_output.tweak(status='A ', contents=None)
  expected_output.remove('A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                         'A/B/E/beta', 'A/B/F')

  expected_wc = svntest.main.greek_state.copy()
  expected_wc.remove('A/B', 'A/B/lambda', 'A/B/E', 'A/B/E/alpha',
                     'A/B/E/beta', 'A/B/F')

  svntest.actions.run_and_verify_export(sbox.repo_url, local_dir,
                                        expected_output,
                                        expected_wc)

#----------------------------------------------------------------------

@Skip(svntest.main.is_ra_type_file)
def authz_log_and_tracing_test(sbox):
  "test authz for log and tracing path changes"

  sbox.build()
  wc_dir = sbox.wc_dir

  write_restrictive_svnserve_conf(sbox.repo_dir)

  # write an authz file with *=rw on /
  if sbox.repo_url.startswith('http'):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  write_authz_file(sbox, { "/": "* = rw\n" })

  root_url = sbox.repo_url
  D_url = root_url + '/A/D'
  G_url = D_url + '/G'

  # check if log doesn't spill any info on which you don't have read access
  rho_path = os.path.join(wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(rho_path, 'new appended text for rho')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'add file rho', sbox.wc_dir)

  svntest.main.file_append(rho_path, 'extra change in rho')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'changed file rho',
                                     sbox.wc_dir)

  # copy a remote file
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     rho_path, D_url,
                                     '-m', 'copy rho to readable area')

  # now disable read access on the first version of rho, keep the copy in
  # /A/D readable.
  if sbox.repo_url.startswith('http'):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  authz = { "/": "* = rw",
            "/A/D/G": "* ="}
  write_authz_file(sbox, authz)

  ## log

  # changed file in this rev. is not readable anymore, so author and date
  # should be hidden, like this:
  # r2 | (no author) | (no date) | 1 line
  svntest.actions.run_and_verify_svn(None,
                                     ".*(no author).*(no date).*|-+\n|\n", [],
                                     'log', '-r', '2', '--limit', '1',
                                     wc_dir)

  if sbox.repo_url.startswith('http'):
    expected_err2 = expected_err
  else:
    expected_err2 = ".*svn: E220001: Item is not readable.*"

  # if we do the same thing directly on the unreadable file, we get:
  # svn: Item is not readable
  svntest.actions.run_and_verify_svn(None, None, expected_err2,
                                     'log', rho_path)

  # while the HEAD rev of the copy is readable in /A/D, its parent in
  # /A/D/G is not, so don't spill any info there either.
  svntest.actions.run_and_verify_svn(None,
                                     ".*(no author).*(no date).*|-+\n|\n", [],
                                    'log', '-r', '2', '--limit', '1', D_url)

  # Test that only author/date are shown for partially visible revisions.
  svntest.actions.enable_revprop_changes(sbox.repo_dir)
  write_authz_file(sbox, { "/": "* = rw"})
  svntest.actions.run_and_verify_svn(
    None, None, [],        # message, expected_stdout, expected_stderr
    'ps', '--revprop', '-r1', 'foobar', 'foo bar', sbox.repo_url)
  svntest.actions.run_and_verify_log_xml(
    expected_revprops=[{'svn:author': svntest.main.wc_author, 'svn:date': '',
                        'svn:log': 'Log message for revision 1.',
                        'foobar': 'foo bar'}],
    args=['--with-all-revprops', '-r1', sbox.repo_url])
  write_authz_file(sbox, authz)
  svntest.actions.run_and_verify_log_xml(
    expected_revprops=[{'svn:author': svntest.main.wc_author, 'svn:date': ''}],
    args=['--with-all-revprops', '-r1', sbox.repo_url])


  ## cat

  # now see if we can look at the older version of rho
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'cat', '-r', '2', D_url+'/rho')

  if sbox.repo_url.startswith('http'):
    expected_err2 = expected_err
  else:
    expected_err2 = ".*svn: E220001: Unreadable path encountered; access denied.*"

  svntest.actions.run_and_verify_svn(None, None, expected_err2,
                                     'cat', '-r', '2', G_url+'/rho')

  ## diff

  # we shouldn't see the diff of a file in an unreadable path
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'diff', '-r', 'HEAD', G_url+'/rho')

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'diff', '-r', '2', D_url+'/rho')

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'diff', '-r', '2:4', D_url+'/rho')

# test whether read access is correctly granted and denied
@SkipUnless(server_authz_has_aliases)
@Skip(svntest.main.is_ra_type_file)
def authz_aliases(sbox):
  "test authz for aliases"

  sbox.build(create_wc = False)

  write_restrictive_svnserve_conf(sbox.repo_dir)

  if sbox.repo_url.startswith("http"):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  write_authz_file(sbox, { "/" : "* = r",
                           "/A/B" : "&jray = rw" },
                         { "aliases" : 'jray = jrandom' } )

  root_url = sbox.repo_url
  A_url = root_url + '/A'
  B_url = A_url + '/B'
  iota_url = root_url + '/iota'

  # copy a remote file, target is readonly for jconstant: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'cp',
                                     '--username', svntest.main.wc_author2,
                                     '-m', 'logmsg',
                                     iota_url, B_url)

  # try the same action, but as user jray (alias of jrandom), should work.
  svntest.actions.run_and_verify_svn(None,
                                     None, [],
                                     'cp',
                                     '-m', 'logmsg',
                                     iota_url, B_url)

@Skip(svntest.main.is_ra_type_file)
@Issue(2486)
def authz_validate(sbox):
  "test the authz validation rules"

  sbox.build(create_wc = False, read_only = True)

  write_restrictive_svnserve_conf(sbox.repo_dir)

  A_url = sbox.repo_url + '/A'

  # If any of the validate rules fail, the authz isn't loaded so there's no
  # access at all to the repository.

  # Test 1: Undefined group
  write_authz_file(sbox, { "/"  : "* = r",
                           "/A/B" : "@undefined_group = rw" })

  if sbox.repo_url.startswith("http"):
    expected_err = ".*[Ff]orbidden.*"
  elif sbox.repo_url.startswith("svn"):
    expected_err = ".*Invalid authz configuration"
  else:
    expected_err = ".*@undefined_group.*"

  # validation of this authz file should fail, so no repo access
  svntest.actions.run_and_verify_svn("ls remote folder",
                                     None, expected_err,
                                     'ls',
                                     A_url)

  # Test 2: Circular dependency
  write_authz_file(sbox, { "/"  : "* = r" },
                         { "groups" : """admins = admin1, admin2, @devs
devs1 = @admins, dev1
devs2 = @admins, dev2
devs = @devs1, dev3, dev4""" })

  if sbox.repo_url.startswith("http"):
    expected_err = ".*[Ff]orbidden.*"
  elif sbox.repo_url.startswith("svn"):
    expected_err = ".*Invalid authz configuration"
  else:
    expected_err = ".*Circular dependency.*"

  # validation of this authz file should fail, so no repo access
  svntest.actions.run_and_verify_svn("ls remote folder",
                                     None, expected_err,
                                     'ls',
                                     A_url)

  # Test 3: Group including other group 2 times (issue 2684)
  write_authz_file(sbox, { "/"  : "* = r" },
                         { "groups" : """admins = admin1, admin2
devs1 = @admins, dev1
devs2 = @admins, dev2
users = @devs1, @devs2, user1, user2""" })

  # validation of this authz file should *not* fail (where formerly,
  # it complained about circular dependencies that do not, in fact,
  # exist), so this is business as usual.
  svntest.actions.run_and_verify_svn("ls remote folder",
                                     ['B/\n', 'C/\n', 'D/\n', 'mu\n'],
                                     [],
                                     'ls',
                                     A_url)

# test locking/unlocking with authz
@Skip(svntest.main.is_ra_type_file)
@Issue(2700)
def authz_locking(sbox):
  "test authz for locking"

  sbox.build()

  write_authz_file(sbox, {"/": "", "/A": "jrandom = rw"})
  write_restrictive_svnserve_conf(sbox.repo_dir)

  if sbox.repo_url.startswith('http'):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  root_url = sbox.repo_url
  wc_dir = sbox.wc_dir
  iota_url = root_url + '/iota'
  iota_path = os.path.join(wc_dir, 'iota')
  A_url = root_url + '/A'
  mu_path = os.path.join(wc_dir, 'A', 'mu')

  # lock a file url, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'lock',
                                     '-m', 'lock msg',
                                     iota_url)

  # lock a file path, target is readonly: should fail
  svntest.actions.run_and_verify_svn(None,
                                     None, expected_err,
                                     'lock',
                                     '-m', 'lock msg',
                                     iota_path)

  # Test for issue 2700: we have write access in folder /A, but not in root.
  # Get a lock on /A/mu and try to commit it.

  # lock a file path, target is writeable: should succeed
  svntest.actions.run_and_verify_svn(None,
                                     None, [],
                                     'lock',
                                     '-m', 'lock msg',
                                     mu_path)

  svntest.main.file_append(mu_path, "hi")

  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        [],
                                        None,
                                        mu_path)

# test for issue #2712: if anon-access == read, svnserve should also check
# authz to determine whether a checkout/update is actually allowed for
# anonymous users, and, if not, attempt authentication.
@XFail()
@Issue(2712)
@SkipUnless(svntest.main.is_ra_type_svn)
def authz_svnserve_anon_access_read(sbox):
  "authz issue #2712"

  sbox.build(create_wc = False)
  svntest.main.safe_rmtree(sbox.wc_dir)
  B_path = os.path.join(sbox.wc_dir, 'A', 'B')
  other_B_path = B_path + '_other'
  B_url = sbox.repo_url + '/A/B'
  D_path = os.path.join(sbox.wc_dir, 'A', 'D')
  D_url = sbox.repo_url + '/A/D'

  # We want a svnserve.conf with anon-access = read.
  write_restrictive_svnserve_conf(sbox.repo_dir, "read")

  # Give jrandom read access to /A/B.  Anonymous users can only
  # access /A/D.
  write_authz_file(sbox, { "/A/B" : "jrandom = rw",
                           "/A/D" : "* = r" })

  # Perform a checkout of /A/B, expecting to see no errors.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     B_url, B_path)

  # Anonymous users should be able to check out /A/D.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     D_url, D_path)

  # Now try a switch.
  svntest.main.safe_rmtree(D_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'switch', D_url, B_path)

  # Check out /A/B with an unknown username, expect error.
  svntest.actions.run_and_verify_svn(
    None, None,
    ".*Authentication error from server: Username not found.*",
    'checkout',
    '--non-interactive',
    '--username', 'losing_user',
    B_url, B_path + '_unsuccessful')

  # Check out a second copy of /A/B, make changes for later merge.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     B_url, other_B_path)
  other_alpha_path = os.path.join(other_B_path, 'E', 'alpha')
  svntest.main.file_append(other_alpha_path, "fish\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', 'log msg',
                                     other_B_path)

  # Now try to merge.  This is an atypical merge, since our "branch"
  # is not really a branch (it's the same URL), but we only care about
  # authz here, not the semantics of the merge.  (Merges had been
  # failing in authz, for the reasons summarized in
  # http://subversion.tigris.org/issues/show_bug.cgi?id=2712#desc13.)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '-c', '2',
                                     B_url, B_path)

@XFail()
@Skip(svntest.main.is_ra_type_file)
def authz_switch_to_directory(sbox):
  "switched to directory, no read access on parents"

  sbox.build(read_only = True)

  write_authz_file(sbox, {"/": "*=rw", "/A/B": "*=", "/A/B/E": "jrandom = rw"})

  write_restrictive_svnserve_conf(sbox.repo_dir)

  wc_dir = sbox.wc_dir
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')

  # Switch /A/B/E to /A/B/F.
  svntest.main.run_svn(None, 'switch', sbox.repo_url + "/A/B/E", G_path)

# Test to reproduce the problem identified by Issue 3242 in which
# Subversion's authz, as of Subversion 1.5, requires access to the
# repository root for copy and move operations.
@Skip(svntest.main.is_ra_type_file)
@Issue(3242)
def authz_access_required_at_repo_root(sbox):
  "authz issue #3242 - access required at repo root"

  sbox.build(create_wc = False)
  root_url = sbox.repo_url

  # Create a copy-level copy of A, just so we have something to work with.
  svntest.main.run_svn(None, 'cp', '-m', 'logmsg',
                       root_url + '/A',
                       root_url + '/A-copy')

  # Now we get all restrictive.
  write_authz_file(sbox, {'/': '* =',
                          '/A': 'jrandom = rw',
                          '/A-copy': 'jrandom = rw'})
  write_restrictive_svnserve_conf(sbox.repo_dir)

  # Do some copies and moves where the common parents of the source(s)
  # and destination(s) are unreadable.  All we currently hope to support
  # is the case where the sources are individually (and recursively)
  # readable, and the destination tree is writable.

  svntest.main.run_svn(None, 'cp',
                       '-m', 'copy in readable space',
                       root_url + '/A/B',
                       root_url + '/A/B-copy')
  svntest.main.run_svn(None, 'cp',
                       '-m', 'copy across disjoint readable spaces',
                       root_url + '/A/B',
                       root_url + '/A-copy/B-copy')
  svntest.main.run_svn(None, 'cp',
                       '-m', 'multi-copy across disjoint readable spaces',
                       root_url + '/A/B',
                       root_url + '/A/mu',
                       root_url + '/A-copy/C')
  svntest.main.run_svn(None, 'cp',
                       '-m', 'copy from disjoint readable spaces',
                       root_url + '/A/B/E/alpha',
                       root_url + '/A-copy/B/E/beta',
                       root_url + '/A-copy/C')

@Skip(svntest.main.is_ra_type_file)
@Issue(3242)
def authz_access_required_at_repo_root2(sbox):
  "more authz issue #3242 - update to renamed file"

  sbox.build(create_wc = False)
  root_url = sbox.repo_url

  # Now we get all restrictive.
  write_authz_file(sbox, {'/': '* =',
                          '/A': 'jrandom = rw'})
  write_restrictive_svnserve_conf(sbox.repo_dir)

  # Rename a file.
  svntest.main.run_svn(None, 'mv',
                       '-m', 'rename file in readable writable space',
                       root_url + '/A/B/E/alpha',
                       root_url + '/A/B/E/alpha-renamed')

  # Check out original greek sub tree below /A/B/E
  # and update it to the above rename.
  wc_dir = sbox.add_wc_path('ABE')
  os.mkdir(wc_dir)
  svntest.main.run_svn(None, 'co', '-r', '1', root_url + '/A/B/E', wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  # Rename a directory.
  svntest.main.run_svn(None, 'mv',
                       '-m', 'rename diretory in readable writable space',
                       root_url + '/A/D/H',
                       root_url + '/A/D/a g e')

  # Check out original greek sub tree below /A/D
  # and update it to the above rename.
  wc_dir = sbox.add_wc_path('AD')
  os.mkdir(wc_dir)
  svntest.main.run_svn(None, 'co', '-r', '1', root_url + '/A/D', wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

@Skip(svntest.main.is_ra_type_file)
def multiple_matches(sbox):
  "multiple lines matching a user"

  sbox.build(create_wc = False)
  root_url = sbox.repo_url
  write_restrictive_svnserve_conf(sbox.repo_dir)
  if sbox.repo_url.startswith("http"):
    expected_err = ".*[Ff]orbidden.*"
  else:
    expected_err = ".*svn: E170001: Authorization failed.*"

  # Prohibit access and commit fails
  write_authz_file(sbox, {'/': 'jrandom ='})
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'cp', '-m', 'fail copy',
                                     root_url, root_url + '/fail')

  # At present if multiple lines match the permissions of all the
  # matching lines are amalgamated.  So jrandom gets access regardless
  # of the line prohibiting access and regardless of the  order of the
  # lines.  This might be a bug, but we probably can't simply fix it as
  # that would change the behaviour of lots of existing authz files.

  write_authz_file(sbox, {'/': 'jrandom =' + '\n' + '* = rw'})
  svntest.main.run_svn(None, 'cp',
                       '-m', 'first copy',
                       root_url, root_url + '/first')

  write_authz_file(sbox, {'/': '* = rw' + '\n' + 'jrandom ='})
  svntest.main.run_svn(None, 'cp',
                       '-m', 'second copy',
                       root_url, root_url + '/second')

@Issues(4025,4026)
@Skip(svntest.main.is_ra_type_file)
def wc_wc_copy_revert(sbox):
  "wc-to-wc-copy with absent nodes and then revert"

  sbox.build(create_wc = False)
  local_dir = sbox.wc_dir
  write_restrictive_svnserve_conf(sbox.repo_dir)

  write_authz_file(sbox, {'/'       : '* = r',
                          '/A/B/E'  : '* =', })

  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = local_dir
  expected_output.tweak(status='A ', contents=None)
  expected_output.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  expected_wc = svntest.main.greek_state.copy()
  expected_wc.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')

  svntest.actions.run_and_verify_checkout(sbox.repo_url, local_dir,
                                          expected_output,
                                          expected_wc)

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

  svntest.actions.run_and_verify_svn(None, None,
                             'svn: E155035: Cannot copy.*excluded by server',
                             'cp', sbox.ospath('A'), sbox.ospath('A2'))


  # The copy failed and A2/B/E is incomplete.  That means A2 and A2/B
  # are complete, but for the other parts of A2 the status is undefined.
  expected_output = svntest.verify.ExpectedOutput(
    ['A  +             -        1 jrandom      ' + sbox.ospath('A2') + '\n',
     '   +             -        1 jrandom      ' + sbox.ospath('A2/B') + '\n',
     '!                -       ?   ?           ' + sbox.ospath('A2/B/E') + '\n',
     ])
  expected_output.match_all = False
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '--verbose', sbox.ospath('A2'))


  # Issue 4025, info SEGV on incomplete working node
  svntest.actions.run_and_verify_svn(None, None,
                                     'svn: E145000: .*unrecognized node kind',
                                     'info', sbox.ospath('A2/B/E'))

  # Issue 4026, copy assertion on incomplete working node
  svntest.actions.run_and_verify_svn(None, None,
                             'svn: E145001: cannot handle node kind',
                             'cp', sbox.ospath('A2/B'), sbox.ospath('B3'))

  expected_output = svntest.verify.ExpectedOutput(
    ['A  +             -        1 jrandom      ' + sbox.ospath('B3') + '\n',
     '!                -       ?   ?           ' + sbox.ospath('B3/E') + '\n',
     ])
  expected_output.match_all = False
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'st', '--verbose', sbox.ospath('B3'))

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', '--recursive',
                                     sbox.ospath('A2'), sbox.ospath('B3'))

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  svntest.actions.run_and_verify_status(sbox.wc_dir, expected_status)

@Skip(svntest.main.is_ra_type_file)
def authz_recursive_ls(sbox):
  "recursive ls with private subtrees"

  sbox.build(create_wc = False)
  local_dir = sbox.wc_dir
  write_restrictive_svnserve_conf(sbox.repo_dir)

  write_authz_file(sbox, {'/'       : '* = r',
                          '/A/B/E'  : '* =',
                          '/A/mu'   : '* =',
                          })
  expected_entries = [
    'A/',
    'A/B/',
    'A/B/F/',
    'A/B/lambda',
    'A/C/',
    'A/D/',
    'A/D/G/',
    'A/D/G/pi',
    'A/D/G/rho',
    'A/D/G/tau',
    'A/D/H/',
    'A/D/H/chi',
    'A/D/H/omega',
    'A/D/H/psi',
    'A/D/gamma',
    'iota',
    ]
  svntest.actions.run_and_verify_svn('recursive ls from /',
                                     map(lambda x: x + '\n', expected_entries),
                                     [], 'ls', '-R',
                                     sbox.repo_url)

@Issue(3781)
@Skip(svntest.main.is_ra_type_file)
def case_sensitive_authz(sbox):
  "authz issue #3781, check case sensitiveness"

  sbox.build()

  wc_dir = sbox.wc_dir
  write_restrictive_svnserve_conf(sbox.repo_dir)

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  mu_url = sbox.repo_url + '/A/mu'
  mu_repo_path = sbox.repo_dir + "/A/mu"
  svntest.main.file_append(mu_path, "hi")

  # Create expected output tree.
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })

  # error messages
  expected_error_for_commit = "Commit failed"

  if sbox.repo_url.startswith("http"):
    expected_error_for_cat = ".*[Ff]orbidden.*"
  else:
    expected_error_for_cat = ".*svn: E170001: Authorization failed.*"

  # test the case-sensitivity of the path inside the repo
  write_authz_file(sbox, {"/": "jrandom = r",
                          "/A/mu": "jrandom =", "/a/Mu": "jrandom = rw"})
  svntest.actions.run_and_verify_svn2(None, None,
                                      expected_error_for_cat,
                                      1, 'cat', mu_url)

  write_authz_file(sbox, {"/": "jrandom = r",
                          "/A": "jrandom = r",
                          "/a/Mu": "jrandom = rw"})
  # Commit the file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        expected_error_for_commit,
                                        mu_path)

  def mixcases(repo_name):
    mixed_repo_name = ''
    for i in range(0, len(repo_name)):
      if i % 2 == 0:
        mixed_val = repo_name[i].upper()
        mixed_repo_name = mixed_repo_name + mixed_val
      else:
        mixed_val = repo_name[i].lower()
        mixed_repo_name = mixed_repo_name + mixed_val
    return mixed_repo_name

  mixed_case_repo_dir = mixcases(os.path.basename(sbox.repo_dir))

  # test the case-sensitivity of the repo name
  sec_mixed_case = {mixed_case_repo_dir + ":/": "jrandom = r",
                    mixed_case_repo_dir + ":/A": "jrandom = r",
                    os.path.basename(sbox.repo_dir) + ":/A/mu": "jrandom =",
                    mixed_case_repo_dir + ":/A/mu": "jrandom = rw"}
  write_authz_file(sbox, {}, sec_mixed_case)
  svntest.actions.run_and_verify_svn2(None, None,
                                      expected_error_for_cat,
                                      1, 'cat', mu_url)

  write_authz_file(sbox, {},
                   sections = {mixed_case_repo_dir + ":/": "jrandom = r",
                               mixed_case_repo_dir + ":/A": "jrandom = r",
                               mixed_case_repo_dir + ":/A/mu": "jrandom = rw"})

  # Commit the file again.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        None,
                                        None,
                                        expected_error_for_commit,
                                        mu_path)

  # test the case-sensitivity
  write_authz_file(sbox, {"/": "jrandom = r",
                          "/A": "jrandom = r", "/A/mu": "jrandom = rw"})

  svntest.actions.run_and_verify_svn2('No error',
                                      svntest.verify.AnyOutput, [],
                                      0, 'cat', mu_url)
  # Commit the file.
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        None,
                                        None,
                                        mu_path)

@Skip(svntest.main.is_ra_type_file)
def authz_tree_conflict(sbox):
  "authz should notice a tree conflict"

  sbox.build()
  wc_dir = sbox.wc_dir
  sbox.simple_rm('A/C')
  sbox.simple_commit()
  sbox.simple_update()

  write_authz_file(sbox, {"/": "jrandom = rw", "/A/C": "*="})
  write_restrictive_svnserve_conf(sbox.repo_dir)

  # And now create an obstruction
  sbox.simple_mkdir('A/C')

  expected_output = svntest.wc.State(wc_dir, {})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/C', status='A ', wc_rev='0')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        None,
                                        expected_status,
                                        "Failed to mark '.*C' absent:",
                                        None, None, None, None, 0,
                                        '-r', '1', wc_dir)

@Issue(3900)
@Skip(svntest.main.is_ra_type_file)
def wc_delete(sbox):
  "wc delete with absent nodes"

  sbox.build(create_wc = False)
  local_dir = sbox.wc_dir
  write_restrictive_svnserve_conf(sbox.repo_dir)

  write_authz_file(sbox, {'/'       : '* = r',
                          '/A/B/E'  : '* =', })

  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = local_dir
  expected_output.tweak(status='A ', contents=None)
  expected_output.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')
  expected_wc = svntest.main.greek_state.copy()
  expected_wc.remove('A/B/E', 'A/B/E/alpha', 'A/B/E/beta')

  svntest.actions.run_and_verify_checkout(sbox.repo_url, local_dir,
                                          expected_output,
                                          expected_wc)

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)

  expected_err = ".*svn: E155035: .*excluded by server*"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'rm', sbox.ospath('A/B/E'))
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'rm', sbox.ospath('A'))

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)


@Skip(svntest.main.is_ra_type_file)
def wc_commit_error_handling(sbox):
  "verify commit error reporting"

  sbox.build()
  wc_dir = sbox.wc_dir
  write_restrictive_svnserve_conf(sbox.repo_dir)

  sbox.simple_mkdir('A/Z')

  write_authz_file(sbox, {'/'   : '* = r', })

  # Creating editor fail: unfriendly error
  expected_err = "(svn: E175013: .*orbidden.*)|" + \
                 "(svn: E170001: Authorization failed)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')

  write_authz_file(sbox, {'/'   : '* = rw',
                          '/A'  : '* = r', })

  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing directory '.*Z' is forbidden)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')

  sbox.simple_revert('A/Z')

  svntest.main.file_write(sbox.ospath('A/zeta'), "Zeta")
  sbox.simple_add('A/zeta')

  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing file '.*zeta' is forbidden)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')
  sbox.simple_revert('A/zeta')

  sbox.simple_propset('a', 'b', 'A/D')

  # Allow a generic dav error and the ra_svn specific one that is returned
  # on editor->edit_close().
  expected_err = "(svn: E175013: .*orbidden.*)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')

  sbox.simple_revert('A/D')

  sbox.simple_propset('a', 'b', 'A/B/lambda')

  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing file '.*lambda' is forbidden.*)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')

  sbox.simple_revert('A/B/lambda')

  svntest.main.file_write(sbox.ospath('A/B/lambda'), "New lambda")
  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing file '.*lambda' is forbidden.*)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')

  sbox.simple_revert('A/B/lambda')

  sbox.simple_rm('A/B/F')
  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing directory '.*F' is forbidden.*)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')
  sbox.simple_revert('A/B/F')

  svntest.main.file_write(sbox.ospath('A/mu'), "Updated mu")
  # Allow the informative error for dav and the ra_svn specific one that is
  # returned on editor->edit_close().
  expected_err = "(svn: E195023: Changing file '.*mu' is forbidden.*)|" + \
                 "(svn: E220004: Access denied)"
  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'ci', wc_dir, '-m', '')


@Skip(svntest.main.is_ra_type_file)
def upgrade_absent(sbox):
  "upgrade absent nodes to server-excluded"

  # Install wc and repos
  replace_sbox_with_tarfile(sbox, 'upgrade_absent.tar.bz2')
  replace_sbox_repo_with_tarfile(sbox, 'upgrade_absent_repos.tar.bz2')

  # Update config for authz
  svntest.main.write_restrictive_svnserve_conf(sbox.repo_dir)
  svntest.main.write_authz_file(sbox, { "/"      : "*=rw",
                                        "/A/B"   : "*=",
                                        "/A/B/E" : "jrandom = rw"})

  # Attempt to use the working copy, this should give an error
  expected_stderr = wc_is_too_old_regex
  svntest.actions.run_and_verify_svn(None, None, expected_stderr,
                                     'info', sbox.wc_dir)

  # Now upgrade the working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'upgrade', sbox.wc_dir)

  # Relocate to allow finding the repository
  svntest.actions.run_and_verify_svn(None, None, [], 'relocate',
                                     'svn://127.0.0.1/authz_tests-2',
                                     sbox.repo_url, sbox.wc_dir)  

  expected_output = svntest.wc.State(sbox.wc_dir, {
  })

  # Expect no changes and certainly no errors
  svntest.actions.run_and_verify_update(sbox.wc_dir, expected_output,
                                        None, None)

@Skip(svntest.main.is_ra_type_file)
@Issue(4332)
def authz_del_from_subdir(sbox):
  "delete file without rights on the root"

  sbox.build(create_wc = False)

  write_authz_file(sbox, {"/": "* = ", "/A": "jrandom = rw"})

  write_restrictive_svnserve_conf(sbox.repo_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                      'rm', sbox.repo_url + '/A/mu',
                                      '-m', '')

########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              authz_open_root,
              authz_open_directory,
              broken_authz_file,
              authz_read_access,
              authz_write_access,
              authz_checkout_test,
              authz_log_and_tracing_test,
              authz_checkout_and_update_test,
              authz_partial_export_test,
              authz_aliases,
              authz_validate,
              authz_locking,
              authz_svnserve_anon_access_read,
              authz_switch_to_directory,
              authz_access_required_at_repo_root,
              authz_access_required_at_repo_root2,
              multiple_matches,
              wc_wc_copy_revert,
              authz_recursive_ls,
              case_sensitive_authz,
              authz_tree_conflict,
              wc_delete,
              wc_commit_error_handling,
              upgrade_absent,
              authz_del_from_subdir,
             ]
serial_only = True

if __name__ == '__main__':
  svntest.main.run_tests(test_list, serial_only = serial_only)
  # NOTREACHED


### End of file.
