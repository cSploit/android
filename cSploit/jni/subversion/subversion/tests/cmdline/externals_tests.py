#!/usr/bin/env python
#
#  module_tests.py:  testing modules / external sources.
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
import sys
import os
import re
import tempfile

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

### todo: it's inefficient to keep calling externals_test_setup() for
### every test.  It's slow.  But it's very safe -- we're guaranteed to
### have a clean repository, built from the latest Subversion, with
### the svn:externals properties preset in a known way.  Right now I
### can't think of any other way to achieve that guarantee, so the
### result is that each individual test is slow.

def externals_test_setup(sbox):
  """Set up a repository in which some directories have the externals property,
  and set up another repository, referred to by some of those externals.
  Both repositories contain greek trees with five revisions worth of
  random changes, then in the sixth revision the first repository --
  and only the first -- has some externals properties set.  ### Later,
  test putting externals on the second repository. ###

  The arrangement of the externals in the first repository is:

    /A/B/ ==>  ^/A/D/gamma                      gamma
    /A/C/ ==>  exdir_G                          <scheme>:///<other_repos>/A/D/G
               ../../../<other_repos_basename>/A/D/H@1 exdir_H

    /A/D/ ==>  ^/../<other_repos_basename>/A    exdir_A
               //<other_repos>/A/D/G/           exdir_A/G/
               exdir_A/H -r 1                   <scheme>:///<other_repos>/A/D/H
               /<some_paths>/A/B                x/y/z/blah

  A dictionary is returned keyed by the directory created by the
  external whose value is the URL of the external.
  """

  # The test itself will create a working copy
  sbox.build(create_wc = False)

  svntest.main.safe_rmtree(sbox.wc_dir)

  wc_init_dir    = sbox.add_wc_path('init')  # just for setting up props
  repo_dir       = sbox.repo_dir
  repo_url       = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  other_repo_basename = os.path.basename(other_repo_dir)

  # Get a scheme relative URL to the other repository.
  scheme_relative_other_repo_url = other_repo_url[other_repo_url.find(':')+1:]

  # Get a server root relative URL to the other repository by trimming
  # off the first three /'s.
  server_relative_other_repo_url = other_repo_url
  for i in range(3):
    j = server_relative_other_repo_url.find('/') + 1
    server_relative_other_repo_url = server_relative_other_repo_url[j:]
  server_relative_other_repo_url = '/' + server_relative_other_repo_url

  # These files will get changed in revisions 2 through 5.
  mu_path = os.path.join(wc_init_dir, "A/mu")
  pi_path = os.path.join(wc_init_dir, "A/D/G/pi")
  lambda_path = os.path.join(wc_init_dir, "A/B/lambda")
  omega_path = os.path.join(wc_init_dir, "A/D/H/omega")

  # These are the directories on which `svn:externals' will be set, in
  # revision 6 on the first repo.
  B_path = os.path.join(wc_init_dir, "A/B")
  C_path = os.path.join(wc_init_dir, "A/C")
  D_path = os.path.join(wc_init_dir, "A/D")

  # Create a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_init_dir)

  # Make revisions 2 through 5, but don't bother with pre- and
  # post-commit status checks.

  svntest.main.file_append(mu_path, "Added to mu in revision 2.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     '--quiet', wc_init_dir)

  svntest.main.file_append(pi_path, "Added to pi in revision 3.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     '--quiet', wc_init_dir)

  svntest.main.file_append(lambda_path, "Added to lambda in revision 4.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     '--quiet', wc_init_dir)

  svntest.main.file_append(omega_path, "Added to omega in revision 5.\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     '--quiet', wc_init_dir)

  # Get the whole working copy to revision 5.
  expected_output = svntest.wc.State(wc_init_dir, {
  })
  svntest.actions.run_and_verify_update(wc_init_dir,
                                        expected_output, None, None)

  # Now copy the initial repository to create the "other" repository,
  # the one to which the first repository's `svn:externals' properties
  # will refer.  After this, both repositories have five revisions
  # of random stuff, with no svn:externals props set yet.
  svntest.main.copy_repos(repo_dir, other_repo_dir, 5)

  # This is the returned dictionary.
  external_url_for = { }

  external_url_for["A/B/gamma"] = "^/A/D/gamma"
  external_url_for["A/C/exdir_G"] = other_repo_url + "/A/D/G"
  external_url_for["A/C/exdir_H"] = "../../../" + \
                                    other_repo_basename + \
                                    "/A/D/H@1"

  # Set up the externals properties on A/B/, A/C/ and A/D/.
  externals_desc = \
           external_url_for["A/B/gamma"] + " gamma\n"

  change_external(B_path, externals_desc, commit=False)

  externals_desc = \
           "exdir_G       " + external_url_for["A/C/exdir_G"] + "\n" + \
           external_url_for["A/C/exdir_H"] + " exdir_H\n"

  change_external(C_path, externals_desc, commit=False)

  external_url_for["A/D/exdir_A"]    = "^/../" + other_repo_basename + "/A"
  external_url_for["A/D/exdir_A/G/"] = scheme_relative_other_repo_url + \
                                       "/A/D/G/"
  external_url_for["A/D/exdir_A/H"]  = other_repo_url + "/A/D/H"
  external_url_for["A/D/x/y/z/blah"] = server_relative_other_repo_url + "/A/B"

  externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"           + \
           "\n"                                                  + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G/"    + \
           "\n"                                                  + \
           "exdir_A/H -r 1 " + external_url_for["A/D/exdir_A/H"] + \
           "\n"                                                  + \
           external_url_for["A/D/x/y/z/blah"] + " x/y/z/blah"    + \
           "\n"

  change_external(D_path, externals_desc, commit=False)

  # Commit the property changes.

  expected_output = svntest.wc.State(wc_init_dir, {
    'A/B' : Item(verb='Sending'),
    'A/C' : Item(verb='Sending'),
    'A/D' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_init_dir, 5)
  expected_status.tweak('A/B', 'A/C', 'A/D', wc_rev=6, status='  ')

  svntest.actions.run_and_verify_commit(wc_init_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_init_dir)

  return external_url_for

def change_external(path, new_val, commit=True):
  """Change the value of the externals property on PATH to NEW_VAL,
  and commit the change unless COMMIT is False."""
  (fd, tmp_f) = tempfile.mkstemp(dir=svntest.main.temp_dir)
  svntest.main.file_append(tmp_f, new_val)
  svntest.actions.run_and_verify_svn(None, None, [], 'pset',
                                     '-F', tmp_f, 'svn:externals', path)
  if commit:
    svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                       '-m', 'log msg', '--quiet', path)
  os.close(fd)
  os.remove(tmp_f)

def change_external_expect_error(path, new_val, expected_err):
  """Try to change the value of the externals property on PATH to NEW_VAL,
  but expect to get an error message that matches EXPECTED_ERR."""
  (fd, tmp_f) = tempfile.mkstemp(dir=svntest.main.temp_dir)
  svntest.main.file_append(tmp_f, new_val)
  svntest.actions.run_and_verify_svn(None, None, expected_err, 'pset',
                                     '-F', tmp_f, 'svn:externals', path)
  os.close(fd)
  os.remove(tmp_f)


def probe_paths_exist(paths):
  """ Probe each one of PATHS to see if it exists, otherwise throw a
      Failure exception. """

  for path in paths:
    if not os.path.exists(path):
      raise svntest.Failure("Probing for " + path + " failed.")


def probe_paths_missing(paths):
  """ Probe each one of PATHS to see if does not exist, otherwise throw a
      Failure exception. """

  for path in paths:
    if os.path.exists(path):
      raise svntest.Failure(path + " unexpectedly still exists.")


#----------------------------------------------------------------------


### todo: It would be great if everything used the new wc.py system to
### check output/status.  In fact, it would be great to do more output
### and status checking period!  But must first see how well the
### output checkers deal with multiple summary lines.  With external
### modules, you can get the first "Updated to revision X" line, and
### then there will be more "Updated to..." and "Checked out..." lines
### following it, one line for each new or changed external.


#----------------------------------------------------------------------

def checkout_with_externals(sbox):
  "test checkouts with externals"

  externals_test_setup(sbox)

  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Create a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Probe the working copy a bit, see if it's as expected.
  expected_existing_paths = [
    os.path.join(wc_dir, "A", "B", "gamma"),
    os.path.join(wc_dir, "A", "C", "exdir_G"),
    os.path.join(wc_dir, "A", "C", "exdir_G", "pi"),
    os.path.join(wc_dir, "A", "C", "exdir_H"),
    os.path.join(wc_dir, "A", "C", "exdir_H", "omega"),
    os.path.join(wc_dir, "A", "D", "x"),
    os.path.join(wc_dir, "A", "D", "x", "y"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah", "E", "alpha"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah", "E", "beta"),
    ]
  probe_paths_exist(expected_existing_paths)

  # Pick a file at random, make sure it has the expected contents.
  for path, contents in ((os.path.join(wc_dir, "A", "C", "exdir_H", "omega"),
                          "This is the file 'omega'.\n"),
                         (os.path.join(wc_dir, "A", "B", "gamma"),
                          "This is the file 'gamma'.\n")):
    if open(path).read() != contents:
      raise svntest.Failure("Unexpected contents for rev 1 of " + path)

#----------------------------------------------------------------------

def update_receive_new_external(sbox):
  "update to receive a new external module"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  other_wc_dir   = sbox.add_wc_path('other')
  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, other_wc_dir)

  # Add one new external item to the property on A/D.  The new item is
  # "exdir_E", deliberately added in the middle not at the end.
  new_externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"           + \
           "\n"                                                   + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G/"     + \
           "\n"                                                   + \
           "exdir_E           " + other_repo_url + "/A/B/E"       + \
           "\n"                                                   + \
           "exdir_A/H -r 1  " + external_url_for["A/D/exdir_A/H"] + \
           "\n"                                                   + \
           external_url_for["A/D/x/y/z/blah"] + " x/y/z/blah"     + \
           "\n"

  # Set and commit the property
  change_external(os.path.join(wc_dir, "A/D"), new_externals_desc)

  # Update the other working copy, see if we get the new item.
  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D'               : Item(status=' U'),
    'A/D/exdir_E/beta'  : Item(status='A '),
    'A/D/exdir_E/alpha' : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(other_wc_dir,
                                        expected_output, None, None)

  probe_paths_exist([os.path.join(other_wc_dir, "A", "D", "exdir_E")])

#----------------------------------------------------------------------

def update_lose_external(sbox):
  "update to lose an external module"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  other_wc_dir   = sbox.add_wc_path('other')
  repo_url       = sbox.repo_url

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, other_wc_dir)

  # Lose one new external item from A/D.  The lost item is
  # "exdir_A", chosen because there are two other externals underneath
  # it (G and H) which are not being removed.  We expect them to
  # remain -- in other words:
  #
  #      BEFORE                                AFTER
  #    ------------                          ------------
  #    A/D/exdir_A                           A/D/exdir_A
  #    A/D/exdir_A/.svn/...                    <GONE>
  #    A/D/exdir_A/mu                          <GONE>
  #    A/D/exdir_A/B/...                       <GONE>
  #    A/D/exdir_A/C/...                       <GONE>
  #    A/D/exdir_A/D/...                       <GONE>
  #    A/D/exdir_A/G/...                     A/D/exdir_A/G/...
  #    A/D/exdir_A/H/...                     A/D/exdir_A/H/...

  new_externals_desc = \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G"      + \
           "\n"                                                   + \
           "exdir_A/H -r 1  " + external_url_for["A/D/exdir_A/H"] + \
           "\n"                                                   + \
           external_url_for["A/D/x/y/z/blah"] + " x/y/z/blah"     + \
           "\n"

  # Set and commit the property
  change_external(os.path.join(wc_dir, "A/D"), new_externals_desc)

  # The code should handle a missing local externals item
  svntest.main.safe_rmtree(os.path.join(other_wc_dir, "A", "D", "exdir_A", \
                                        "D"))

  # Update other working copy, see if lose & preserve things appropriately
  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D'               : Item(status=' U'),
    'A/D/exdir_A'       : Item(verb='Removed external'),
  })
  svntest.actions.run_and_verify_update(other_wc_dir,
                                        expected_output, None, None)

  expected_existing_paths = [
    os.path.join(other_wc_dir, "A", "D", "exdir_A"),
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "G"),
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "H"),
    ]
  probe_paths_exist(expected_existing_paths)

  expected_missing_paths = [
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "mu"),
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "B"),
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "C"),
    os.path.join(other_wc_dir, "A", "D", "exdir_A", "D"),
    ]
  probe_paths_missing(expected_missing_paths)

#----------------------------------------------------------------------

def update_change_pristine_external(sbox):
  "update change to an unmodified external module"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  other_wc_dir   = sbox.add_wc_path('other')
  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, other_wc_dir)

  # Change the "x/y/z/blah" external on A/D to point to a different
  # URL.  Since no changes were made to the old checked-out external,
  # we should get a clean replace.
  new_externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"          + \
           "\n"                                                  + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G"     + \
           "\n"                                                  + \
           "exdir_A/H -r 1 " + external_url_for["A/D/exdir_A/H"] + \
           "\n"                                                  + \
           "x/y/z/blah     " + other_repo_url + "/A/B/F"         + \
           "\n"

  # Set and commit the property
  change_external(os.path.join(wc_dir, "A/D"), new_externals_desc)

  # Update other working copy, see if get the right change.
  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D'               : Item(status=' U'),
    'A/D/x/y/z/blah/F'  : Item(status='D '),
    'A/D/x/y/z/blah/E'  : Item(status='D '),
    'A/D/x/y/z/blah/lambda': Item(status='D '),
  })
  svntest.actions.run_and_verify_update(other_wc_dir,
                                        expected_output, None, None)

  xyzb_path = os.path.join(other_wc_dir, "x", "y", "z", "blah")

  expected_missing_paths = [
    os.path.join(xyzb_path, "alpha"),
    os.path.join(xyzb_path, "beta"),
    ]
  probe_paths_missing(expected_missing_paths)

def update_change_modified_external(sbox):
  "update changes to a modified external module"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  other_wc_dir   = sbox.add_wc_path('other')
  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, other_wc_dir)

  # Make a couple of mods in the "x/y/z/blah/" external.
  alpha_path = os.path.join(other_wc_dir, "A", "D",
                            "x", "y", "z", "blah", "alpha")
  svntest.main.file_append(alpha_path, "Some new text in alpha.\n")
  new_file = os.path.join(other_wc_dir, "A", "D",
                          "x", "y", "z", "blah", "fish.txt")
  svntest.main.file_append(new_file, "This is an unversioned file.\n")

  # Change the "x/y/z/blah" external on A/D to point to a different
  # URL.  There are some local mods under the old checked-out external,
  # so the old dir should be saved under a new name.
  new_externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"          + \
           "\n"                                                  + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G/"    + \
           "\n"                                                  + \
           "exdir_A/H -r 1 " + external_url_for["A/D/exdir_A/H"] + \
           "\n"                                                  + \
           "x/y/z/blah     " + other_repo_url + "/A/B/F"         + \
           "\n"

  # Set and commit the property
  change_external(os.path.join(wc_dir, "A/D"), new_externals_desc)

  # Update other working copy, see if get the right change.
  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D'               : Item(status=' U'),
    'A/D/x/y/z/blah/F'  : Item(status='D '),
    'A/D/x/y/z/blah/lambda': Item(status='D '),
    'A/D/x/y/z/blah/E'  : Item(status='D '),
  })
  svntest.actions.run_and_verify_update(other_wc_dir,
                                        expected_output, None, None)


  xyzb_path = os.path.join(other_wc_dir, "x", "y", "z", "blah")

  expected_missing_paths = [
    os.path.join(xyzb_path, "alpha"),
    os.path.join(xyzb_path, "beta"),
    ]
  probe_paths_missing(expected_missing_paths)

def update_receive_change_under_external(sbox):
  "update changes under an external module"

  externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  other_wc_dir   = sbox.add_wc_path('other')
  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     other_repo_url, other_wc_dir)

  # Commit some modifications from the other_wc.
  other_gamma_path = os.path.join(other_wc_dir, 'A', 'D', 'gamma')
  svntest.main.file_append(other_gamma_path, "New text in other gamma.\n")

  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D/gamma' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(other_wc_dir, 5)
  expected_status.tweak('A/D/gamma', wc_rev=6)
  svntest.actions.run_and_verify_commit(other_wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, other_wc_dir)

  # Now update the regular wc to see if we get the change.  Note that
  # none of the module *properties* in this wc have been changed; only
  # the source repository of the modules has received a change, and
  # we're verifying that an update here pulls that change.

  # The output's going to be all screwy because of the module
  # notifications, so don't bother parsing it, just run update
  # directly.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/exdir_A/D/gamma': Item(status='U '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  external_gamma_path = os.path.join(wc_dir, 'A', 'D', 'exdir_A', 'D', 'gamma')
  contents = open(external_gamma_path).read()
  if contents != ("This is the file 'gamma'.\n"
                  "New text in other gamma.\n"):
    raise svntest.Failure("Unexpected contents for externally modified " +
                          external_gamma_path)

  # Commit more modifications
  other_rho_path = os.path.join(other_wc_dir, 'A', 'D', 'G', 'rho')
  svntest.main.file_append(other_rho_path, "New text in other rho.\n")

  expected_output = svntest.wc.State(other_wc_dir, {
    'A/D/G/rho' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(other_wc_dir, 5)
  expected_status.tweak('A/D/gamma', wc_rev=6)
  expected_status.tweak('A/D/G/rho', wc_rev=7)
  svntest.actions.run_and_verify_commit(other_wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, other_wc_dir)

  expected_output = svntest.wc.State(sbox.ospath('A/C'), {
    'exdir_G/rho'       : Item(status='U '),
  })
  svntest.actions.run_and_verify_update(sbox.ospath('A/C'),
                                        expected_output, None, None)

  external_rho_path = os.path.join(wc_dir, 'A', 'C', 'exdir_G', 'rho')
  contents = open(external_rho_path).read()
  if contents != ("This is the file 'rho'.\n"
                  "New text in other rho.\n"):
    raise svntest.Failure("Unexpected contents for externally modified " +
                          external_rho_path)

#----------------------------------------------------------------------

def modify_and_update_receive_new_external(sbox):
  "commit and update additional externals"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Add one more external item
  B_path = os.path.join(wc_dir, "A/B")
  externals_desc = \
          external_url_for["A/D/exdir_A/G/"] + " exdir_G"     + \
          "\n"                                                + \
          "exdir_H -r 1 " + external_url_for["A/D/exdir_A/H"] + \
          "\n"                                                + \
          "exdir_Z      " + external_url_for["A/D/exdir_A/H"] + \
          "\n"

  change_external(B_path, externals_desc)

  # Now cd into A/B and try updating
  was_cwd = os.getcwd()
  os.chdir(B_path)

  # Once upon a time there was a core-dump here

  svntest.actions.run_and_verify_svn("update failed",
                                     svntest.verify.AnyOutput, [], 'up' )

  os.chdir(was_cwd)

  probe_paths_exist([os.path.join(B_path, "exdir_Z")])

#----------------------------------------------------------------------

def disallow_dot_or_dotdot_directory_reference(sbox):
  "error if external target dir involves '.' or '..'"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Try to set illegal externals in the original WC.
  def set_externals_for_path_expect_error(path, val):
    expected_err = ".*Invalid svn:externals property on '.*': target " + \
                   "'.*' is an absolute path or involves '..'.*"
    change_external_expect_error(path, val, expected_err)

  B_path = os.path.join(wc_dir, 'A', 'B')
  G_path = os.path.join(wc_dir, 'A', 'D', 'G')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  C_path = os.path.join(wc_dir, 'A', 'C')
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')

  external_urls = list(external_url_for.values())

  # The external_urls contains some examples of relative urls that are
  # ambiguous with these local test paths, so we have to use the
  # <url> <path> ordering here to check the local path validator.

  externals_value_1 = external_urls.pop() + " ../foo\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_2 = external_urls.pop() + " foo/bar/../baz\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_3 = external_urls.pop() + " foo/..\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_4 = external_urls.pop() + " .\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_5 = external_urls.pop() + " ./\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_6 = external_urls.pop() + " ..\n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_7 = external_urls.pop() + " ././/.///. \n"
  if not external_urls: external_urls = list(external_url_for.values())
  externals_value_8 = external_urls.pop() + " /foo \n"
  if not external_urls: external_urls = list(external_url_for.values())
  if svntest.main.is_os_windows():
    externals_value_9 = external_urls.pop() + " D:/foo\n"
    if not external_urls: external_urls = list(external_url_for.values())
    externals_value_10 = external_urls.pop() + " D:\\foo\n"
    if not external_urls: external_urls = list(external_url_for.values())
    externals_value_11 = external_urls.pop() + " D:foo\n"
    if not external_urls: external_urls = list(external_url_for.values())

  set_externals_for_path_expect_error(B_path, externals_value_1)
  set_externals_for_path_expect_error(G_path, externals_value_2)
  set_externals_for_path_expect_error(H_path, externals_value_3)
  set_externals_for_path_expect_error(C_path, externals_value_4)
  set_externals_for_path_expect_error(F_path, externals_value_5)
  set_externals_for_path_expect_error(B_path, externals_value_6)
  set_externals_for_path_expect_error(G_path, externals_value_7)
  set_externals_for_path_expect_error(H_path, externals_value_8)
  if svntest.main.is_os_windows():
    set_externals_for_path_expect_error(B_path, externals_value_9)
    set_externals_for_path_expect_error(B_path, externals_value_10)
    set_externals_for_path_expect_error(B_path, externals_value_11)


#----------------------------------------------------------------------

def export_with_externals(sbox):
  "test exports with externals"

  externals_test_setup(sbox)

  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Create a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'export',
                                     repo_url, wc_dir)

  # Probe the working copy a bit, see if it's as expected.
  expected_existing_paths = [
    os.path.join(wc_dir, "A", "C", "exdir_G"),
    os.path.join(wc_dir, "A", "C", "exdir_G", "pi"),
    os.path.join(wc_dir, "A", "C", "exdir_H"),
    os.path.join(wc_dir, "A", "C", "exdir_H", "omega"),
    os.path.join(wc_dir, "A", "D", "x"),
    os.path.join(wc_dir, "A", "D", "x", "y"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah", "E", "alpha"),
    os.path.join(wc_dir, "A", "D", "x", "y", "z", "blah", "E", "beta"),
    ]
  probe_paths_exist(expected_existing_paths)

  # Pick some files, make sure their contents are as expected.
  exdir_G_pi_path = os.path.join(wc_dir, "A", "C", "exdir_G", "pi")
  contents = open(exdir_G_pi_path).read()
  if contents != ("This is the file 'pi'.\n"
                  "Added to pi in revision 3.\n"):
    raise svntest.Failure("Unexpected contents for rev 1 of " +
                          exdir_G_pi_path)

  exdir_H_omega_path = os.path.join(wc_dir, "A", "C", "exdir_H", "omega")
  contents = open(exdir_H_omega_path).read()
  if contents != "This is the file 'omega'.\n":
    raise svntest.Failure("Unexpected contents for rev 1 of " +
                          exdir_H_omega_path)

#----------------------------------------------------------------------

# Test for issue #2429
@Issue(2429)
def export_wc_with_externals(sbox):
  "test exports from working copies with externals"

  paths_dict = externals_test_setup(sbox)

  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url
  export_target = sbox.add_wc_path('export')

  # Create a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)
  # Export the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'export', wc_dir, export_target)

  ### We should be able to check exactly the paths that externals_test_setup()
  ### set up; however, --ignore-externals fails to ignore 'A/B/gamma' so this
  ### doesn't work:
  # paths = [ os.path.join(export_target, path) for path in paths_dict.keys() ]
  ### Therefore currently we check only a particular selection of paths.
  paths = [
    os.path.join(export_target, "A", "C", "exdir_G"),
    os.path.join(export_target, "A", "C", "exdir_G", "pi"),
    os.path.join(export_target, "A", "C", "exdir_H"),
    os.path.join(export_target, "A", "C", "exdir_H", "omega"),
    os.path.join(export_target, "A", "D", "x"),
    os.path.join(export_target, "A", "D", "x", "y"),
    os.path.join(export_target, "A", "D", "x", "y", "z"),
    os.path.join(export_target, "A", "D", "x", "y", "z", "blah"),
    os.path.join(export_target, "A", "D", "x", "y", "z", "blah", "E", "alpha"),
    os.path.join(export_target, "A", "D", "x", "y", "z", "blah", "E", "beta"),
    ]
  probe_paths_exist(paths)

  svntest.main.safe_rmtree(export_target)

  # Export it again, without externals.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'export', '--ignore-externals',
                                     wc_dir, export_target)
  probe_paths_missing(paths)

#----------------------------------------------------------------------

def external_with_peg_and_op_revision(sbox):
  "use a peg revision to specify an external module"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # remove A/D/H in the other repo
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm',
                                     external_url_for["A/D/exdir_A/H"],
                                     '-m', 'remove original A/D/H')

  # Set an external property using peg revision syntax.
  new_externals_desc = \
           external_url_for["A/D/exdir_A/H"]  + "@4 exdir_A/H" + \
           "\n"                                                + \
           external_url_for["A/D/exdir_A/G/"] + "   exdir_A/G" + \
           "\n"

  # Set and commit the property.
  change_external(os.path.join(wc_dir, "A/D"), new_externals_desc)

  # Update other working copy, see if we get the right change.
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/x/y/z/blah'    : Item(verb='Removed external'),
    'A/D/exdir_A'       : Item(verb='Removed external'),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)


  external_chi_path = os.path.join(wc_dir, 'A', 'D', 'exdir_A', 'H', 'chi')
  contents = open(external_chi_path).read()
  if contents != "This is the file 'chi'.\n":
    raise svntest.Failure("Unexpected contents for externally modified " +
                          external_chi_path)

#----------------------------------------------------------------------

def new_style_externals(sbox):
  "check the new '-rN URL PATH' syntax"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Set an external property using the new '-rN URL PATH' syntax.
  new_externals_desc = \
           external_url_for["A/C/exdir_G"] + " exdir_G"           + \
           "\n"                                                   + \
           "-r 1 " + external_url_for["A/C/exdir_H"] + " exdir_H" + \
           "\n"                                                   + \
           "-r1 "  + external_url_for["A/C/exdir_H"] + " exdir_I" + \
           "\n"

  # Set and commit the property.
  change_external(os.path.join(wc_dir, "A/C"), new_externals_desc)

  # Update other working copy.
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/exdir_I/chi'   : Item(status='A '),
    'A/C/exdir_I/omega' : Item(status='A '),
    'A/C/exdir_I/psi'   : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)
  for dir_name in ["exdir_H", "exdir_I"]:
    exdir_X_omega_path = os.path.join(wc_dir, "A", "C", dir_name, "omega")
    contents = open(exdir_X_omega_path).read()
    if contents != "This is the file 'omega'.\n":
      raise svntest.Failure("Unexpected contents for rev 1 of " +
                            exdir_X_omega_path)

#----------------------------------------------------------------------

def disallow_propset_invalid_formatted_externals(sbox):
  "error if propset'ing external with invalid format"

  # Bootstrap
  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # It should not be possible to set these external properties on a
  # directory.
  for ext in [ 'arg1',
               'arg1 arg2 arg3',
               'arg1 arg2 arg3 arg4',
               'arg1 arg2 arg3 arg4 arg5',
               '-r',
               '-r1',
               '-r 1',
               '-r1 arg1',
               '-r 1 arg1',
               'arg1 -r',
               'arg1 -r1',
               'arg1 -r 1',
               ]:
    change_external_expect_error(A_path, ext,
                                 '.*Error parsing svn:externals.*')

  for ext in [ '-r abc arg1 arg2',
               '-rabc arg1 arg2',
               'arg1 -r abc arg2',
               'arg1 -rabc arg2',
               ]:
    change_external_expect_error(A_path, ext,
                                 '.*Error parsing svn:externals.*')

  for ext in [ 'http://example.com/ http://example.com/',
               '-r1 http://example.com/ http://example.com/',
               '-r 1 http://example.com/ http://example.com/',
               'http://example.com/ -r1 http://example.com/',
               'http://example.com/ -r 1 http://example.com/',
               ]:
    change_external_expect_error(A_path, ext,
                                 '.*cannot use two absolute URLs.*')

  for ext in [ 'http://example.com/ -r1 foo',
               'http://example.com/ -r 1 foo',
               '-r1 foo http://example.com/',
               '-r 1 foo http://example.com/'
               ]:
    change_external_expect_error(A_path, ext,
                                 '.*cannot use a URL \'.*\' as the ' \
                                   'target directory for an external ' \
                                   'definition.*')

#----------------------------------------------------------------------

def old_style_externals_ignore_peg_reg(sbox):
  "old 'PATH URL' format should ignore peg revisions"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Update the working copy.
  expected_output = svntest.wc.State(wc_dir, {
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  # Set an external property using the old 'PATH URL' syntax with
  # @HEAD in the URL.
  ext = "exdir_G " + external_url_for["A/C/exdir_G"] + "@HEAD\n"

  # Set and commit the property.
  change_external(os.path.join(wc_dir, "A"), ext)

  # Update the working copy.  This should succeed (exitcode 0) but
  # should print warnings on the external because the URL with '@HEAD'
  # does not exist.
  expected_error = "|".join([".*Error handling externals definition.*",
                             ".*URL .*/A/D/G@HEAD' .* doesn't exist.*",
                             ])
  svntest.actions.run_and_verify_svn2("External '%s' used pegs" % ext.strip(),
                                      None,
                                      expected_error,
                                      1,
                                      'up',
                                      wc_dir)


#----------------------------------------------------------------------

def cannot_move_or_remove_file_externals(sbox):
  "should not be able to mv or rm a file external"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Should not be able to delete the file external.
  svntest.actions.run_and_verify_svn("Able to delete file external",
                                     None,
                                     ".*Cannot remove the external at "
                                     ".*gamma.*; please .* "
                                     "the svn:externals .*",
                                     'rm',
                                     os.path.join(wc_dir, 'A', 'B', 'gamma'))

  # Should not be able to move the file external.
  svntest.actions.run_and_verify_svn("Able to move file external",
                                     None,
                                     ".*Cannot move the external at "
                                     ".*gamma.*; please .*edit.*"
                                     "svn:externals.*",
                                     'mv',
                                     os.path.join(wc_dir, 'A', 'B', 'gamma'),
                                     os.path.join(wc_dir, 'A', 'B', 'gamma1'))

  # But the directory that contains it can be deleted.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 6)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'rm',
                                     os.path.join(wc_dir, "A", "B"))

  expected_status.tweak('A/B', status='D ')
  expected_output = svntest.wc.State(wc_dir, {
      'A/B' : Item(verb='Deleting'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 6)
  expected_status.remove('A/B', 'A/B/E', 'A/B/E/alpha', 'A/B/E/beta',
                         'A/B/F', 'A/B/lambda')

  expected_status.add({
    'A/D/exdir_A'       : Item(status='X '),
    'A/D/x'             : Item(status='X '),
    'A/C/exdir_H'       : Item(status='X '),
    'A/C/exdir_G'       : Item(status='X '),
  })

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output, expected_status,
                                        None, wc_dir)

  # Bring the working copy up to date and check that the file the file
  # external is switched to still exists.
  expected_output = svntest.wc.State(wc_dir, {
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  open(os.path.join(wc_dir, 'A', 'D', 'gamma')).close()

#----------------------------------------------------------------------

def cant_place_file_external_into_dir_external(sbox):
  "place a file external into a directory external"

  external_url_for = externals_test_setup(sbox)
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout a working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Put a directory external into the same repository and then a file
  # external into that.
  ext = "^/A/D        A/D-copy\n" + \
        "^/A/B/E/beta A/D-copy/G/beta\n"
  change_external(wc_dir, ext)

  # Bring the working copy up to date and check that the file the file
  # external is switched to still exists.
  svntest.actions.run_and_verify_svn(None, None, 'svn: E205011: ' +
                                     'Failure occurred.*definitions',
                                     'up', wc_dir)

#----------------------------------------------------------------------

# Issue #2461.
@Issue(2461)
def external_into_path_with_spaces(sbox):
  "allow spaces in external local paths"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  ext = '^/A/D        "A/copy of D"\n' +\
        '^/A/D        A/another\ copy\ of\ D'
  change_external(wc_dir, ext)

  expected_output = svntest.wc.State(wc_dir, {
    'A/another copy of D/G': Item(status='A '),
    'A/another copy of D/G/pi': Item(status='A '),
    'A/another copy of D/G/tau': Item(status='A '),
    'A/another copy of D/G/rho': Item(status='A '),
    'A/another copy of D/H': Item(status='A '),
    'A/another copy of D/H/chi': Item(status='A '),
    'A/another copy of D/H/omega': Item(status='A '),
    'A/another copy of D/H/psi': Item(status='A '),
    'A/another copy of D/gamma': Item(status='A '),
    'A/copy of D/H'     : Item(status='A '),
    'A/copy of D/H/chi' : Item(status='A '),
    'A/copy of D/H/omega': Item(status='A '),
    'A/copy of D/H/psi' : Item(status='A '),
    'A/copy of D/gamma' : Item(status='A '),
    'A/copy of D/G'     : Item(status='A '),
    'A/copy of D/G/rho' : Item(status='A '),
    'A/copy of D/G/tau' : Item(status='A '),
    'A/copy of D/G/pi'  : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)
  probe_paths_exist([
      os.path.join(wc_dir, 'A', 'copy of D'),
      os.path.join(wc_dir, 'A', 'another copy of D'),
  ])

#----------------------------------------------------------------------

# Issue #3368
@Issue(3368)
def binary_file_externals(sbox):
  "binary file externals"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a binary file A/theta, write PNG file data into it.
  theta_contents = open(os.path.join(sys.path[0], "theta.bin"), 'rb').read()
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  svntest.main.file_write(theta_path, theta_contents, 'wb')

  svntest.main.run_svn(None, 'add', theta_path)

  # Commit the binary file
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Adding  (bin)'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)


  # Create a file external on the binary file A/theta
  C = os.path.join(wc_dir, 'A', 'C')
  external = os.path.join(C, 'external')
  externals_prop = "^/A/theta external\n"

  # Set and commit the property.
  change_external(C, externals_prop)


  # Now, /A/C/external is designated as a file external pointing to
  # the binary file /A/theta, but the external file is not there yet.
  # Try to actually insert the external file via a verified update:
  expected_output = svntest.wc.State(wc_dir, {
      'A/C/external'      : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta'      : Item(
                       theta_contents,
                       props={'svn:mime-type' : 'application/octet-stream'}),
    'A/C'          : Item(props={'svn:externals':externals_prop}),
    'A/C/external' : Item(
                       theta_contents,
                       props={'svn:mime-type' : 'application/octet-stream'}),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/theta'      : Item(status='  ', wc_rev=3),
    'A/C/external' : Item(status='  ', wc_rev=3, switched='X'),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

#----------------------------------------------------------------------

# Issue #3351.
@Issue(3351)
def update_lose_file_external(sbox):
  "delete a file external"

  sbox.build()
  wc_dir = sbox.wc_dir


  # Create a file external in A/C/external on the file A/mu
  C = os.path.join(wc_dir, 'A', 'C')
  external = os.path.join(C, 'external')
  externals_prop = "^/A/mu external\n"

  # Set and commit the property.
  change_external(C, externals_prop)


  # Now, /A/C/external is designated as a file external pointing to
  # the file /A/mu, but the external file is not there yet.
  # Try to actually insert the external file via an update:
  expected_output = svntest.wc.State(wc_dir, {
      'A/C/external'      : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/C'          : Item(props={'svn:externals':externals_prop}),
    'A/C/external' : Item("This is the file 'mu'.\n"),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/C/external' : Item(status='  ', wc_rev='2', switched='X'),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

  # now remove the svn:external prop
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propdel', 'svn:externals', C)

  # commit the property change
  expected_output = svntest.wc.State(wc_dir, {
    'A/C' : Item(verb='Sending'),
    })

  # (re-use above expected_status)
  expected_status.tweak('A/C', wc_rev = 3)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # try to actually get rid of the external via an update
  expected_output = svntest.wc.State(wc_dir, {
    'A/C/external'      : Item(verb='Removed external')
  })

  # (re-use above expected_disk)
  expected_disk.tweak('A/C', props = {})
  expected_disk.remove('A/C/external')

  # (re-use above expected_status)
  expected_status.tweak(wc_rev = 3)

  # And assume that the external will be removed.
  expected_status.remove('A/C/external')

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

  probe_paths_missing([os.path.join(wc_dir, 'A', 'C', 'external')])


#----------------------------------------------------------------------

# Issue #3351.
@Issue(3351)
def switch_relative_external(sbox):
  "switch a relative external"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Create a relative external in A/D on ../B
  A_path = os.path.join(wc_dir, 'A')
  A_copy_path = os.path.join(wc_dir, 'A_copy')
  A_copy_url = repo_url + '/A_copy'
  D_path = os.path.join(A_path, 'D')
  ext_path = os.path.join(D_path, 'ext')
  externals_prop = "../B ext\n"
  change_external(D_path, externals_prop)

  # Update our working copy, and create a "branch" (A => A_copy)
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/ext/E'         : Item(status='A '),
    'A/D/ext/E/beta'    : Item(status='A '),
    'A/D/ext/E/alpha'   : Item(status='A '),
    'A/D/ext/F'         : Item(status='A '),
    'A/D/ext/lambda'    : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)
  svntest.actions.run_and_verify_svn(None, None, [], 'cp',
                                     '--quiet', A_path, A_copy_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg',
                                     '--quiet', wc_dir)

  # Okay.  We now want to switch A to A_copy, which *should* cause
  # A/D/ext to point to the URL for A_copy/B (instead of A/B).
  svntest.actions.run_and_verify_svn(None, None, [], 'sw',
                                     A_copy_url, A_path)

  expected_infos = [
    { 'Path' : re.escape(D_path),
      'URL' : sbox.repo_url + '/A_copy/D',
      },
    { 'Path' : re.escape(ext_path),
      'URL' : sbox.repo_url + '/A_copy/B',
      },
    ]
  svntest.actions.run_and_verify_info(expected_infos, D_path, ext_path)

#----------------------------------------------------------------------

# A regression test for a bug in exporting externals from a mixed-depth WC.
def export_sparse_wc_with_externals(sbox):
  "export from a sparse working copy with externals"

  externals_test_setup(sbox)

  repo_url = sbox.repo_url + '/A/B'
  wc_dir = sbox.wc_dir
  # /A/B contains (dir 'E', dir 'F', file 'lambda', external dir 'gamma').
  children = [ 'E', 'F', 'lambda' ]
  ext_children = [ 'gamma' ]

  def wc_paths_of(relative_paths):
    return [ os.path.join(wc_dir, path) for path in relative_paths ]

  child_paths = wc_paths_of(children)
  ext_child_paths = wc_paths_of(ext_children)

  export_target = sbox.add_wc_path('export')

  # Create a working copy with depth=empty itself but children that are
  # depth=infinity.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout', '--depth=empty',
                                     repo_url, wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'update', *child_paths)
  # Export the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'export', wc_dir, export_target)
  # It failed with "'gamma' is not under version control" because the
  # depth-infinity children led it wrongly to try to process externals
  # in the parent.

  svntest.main.safe_rmtree(export_target)

#----------------------------------------------------------------------

# Change external from one repo to another
def relegate_external(sbox):
  "relegate external from one repo to another"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  A_path = os.path.join(wc_dir, 'A')

  # setup an external within the same repository
  externals_desc = '^/A/B/E        external'
  change_external(A_path, externals_desc)
  expected_output = svntest.wc.State(wc_dir, {
    'A/external/alpha'  : Item(status='A '),
    'A/external/beta'   : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  # create another repository
  other_repo_dir, other_repo_url = sbox.add_repo_path('other')
  svntest.main.copy_repos(repo_dir, other_repo_dir, 2)

  # point external to the other repository
  externals_desc = other_repo_url + '/A/B/E        external\n'
  change_external(A_path, externals_desc)

  # Update "relegates", i.e. throws-away and recreates, the external
  expected_output = svntest.wc.State(wc_dir, {
      'A/external'       : Item(), # No A?
      'A/external/alpha' : Item(status='A '),
      'A/external/beta'  : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A', props={'svn:externals' : externals_desc})
  expected_disk.add({
      'A/external'       : Item(),
      'A/external/alpha' : Item('This is the file \'alpha\'.\n'),
      'A/external/beta'  : Item('This is the file \'beta\'.\n'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/external'        : Item(status='X '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

#----------------------------------------------------------------------

# Issue #3552
@Issue(3552)
def wc_repos_file_externals(sbox):
  "tag directory with file externals from wc to url"

  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Add a file A/theta.
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  svntest.main.file_write(theta_path, 'theta', 'w')
  svntest.main.run_svn(None, 'add', theta_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Adding'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    })

  # Commit the new file, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)


  # Create a file external on the file A/theta
  C = os.path.join(wc_dir, 'A', 'C')
  external = os.path.join(C, 'theta')
  externals_prop = "^/A/theta theta\n"

  # Set and commit the property.
  change_external(C, externals_prop)


  # Now, /A/C/theta is designated as a file external pointing to
  # the file /A/theta, but the external file is not there yet.
  # Try to actually insert the external file via a verified update:
  expected_output = svntest.wc.State(wc_dir, {
      'A/C/theta'      : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta'      : Item('theta'),
    'A/C'          : Item(props={'svn:externals':externals_prop}),
    'A/C/theta'    : Item('theta'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/theta'   : Item(status='  ', wc_rev=3),
    'A/C/theta' : Item(status='  ', wc_rev=3, switched='X'),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

  # Copy A/C to a new tag in the repos
  tag_url = repo_url + '/A/I'
  svntest.main.run_svn(None, 'cp', C, tag_url, '-m', 'create tag')

  # Try to actually insert the external file (A/I/theta) via a verified update:
  expected_output = svntest.wc.State(wc_dir, {
      'A/I'            : Item(status='A '),
      'A/I/theta'      : Item(status='A '),
    })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta'      : Item('theta'),
    'A/C'          : Item(props={'svn:externals':externals_prop}),
    'A/C/theta'    : Item('theta'),
    'A/I'          : Item(props={'svn:externals':externals_prop}),
    'A/I/theta'    : Item('theta'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 4)
  expected_status.add({
    'A/theta'   : Item(status='  ', wc_rev=4),
    'A/C/theta' : Item(status='  ', wc_rev=4, switched='X'),
    'A/I'       : Item(status='  ', wc_rev=4),
    'A/I/theta' : Item(status='  ', wc_rev=4, switched='X'),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

#----------------------------------------------------------------------
@Issue(3843)
def merge_target_with_externals(sbox):
  "merge target with externals"

  # Test for a problem the plagued Subversion in the pre-1.7-single-DB world:
  # Externals in a merge target would get meaningless explicit mergeinfo set
  # on them.  See http://svn.haxx.se/dev/archive-2010-08/0088.shtml
  externals_test_setup(sbox)
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Some paths we'll care about
  A_path              = os.path.join(wc_dir, "A")
  A_branch_path       = os.path.join(wc_dir, "A-branch")
  A_gamma_branch_path = os.path.join(wc_dir, "A-branch", "D", "gamma")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Setup A/external as file external to A/mu
  # and A/external-pinned as a pinned file external to A/mu
  externals_prop = "^/A/mu external\n^/A/mu@6 external-pinned\n"
  change_external(sbox.ospath('A'), externals_prop)

  # Branch A@1 to A-branch and make a simple text change on the latter in r8.
  svntest.actions.run_and_verify_svn(None, None, [], 'copy', A_path + '@1',
                                     A_branch_path)
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'make a copy', wc_dir)
  svntest.main.file_write(A_gamma_branch_path, "The new gamma!\n")
  svntest.actions.run_and_verify_svn(None, None, [], 'ci',
                                     '-m', 'branch edit', wc_dir)
  expected_output = svntest.wc.State(wc_dir, {
    'A/external'        : Item(status='A '),
    'A/external-pinned' : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  # Merge r8 from A-branch back to A.  There should be explicit mergeinfo
  # only at the root of A; the externals should not get any.
  svntest.actions.run_and_verify_svn(None, None, [], 'merge', '-c8',
                                     repo_url + '/A-branch', A_path)
  svntest.actions.run_and_verify_svn(
    "Unexpected subtree mergeinfo created",
    ["Properties on '" + A_path + "':\n",
     "  svn:mergeinfo\n",
     "    /A-branch:8\n"],
    [], 'pg', svntest.main.SVN_PROP_MERGEINFO, '-vR', wc_dir)

def update_modify_file_external(sbox):
  "update that modifies a file external"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup A/external as file external to A/mu
  externals_prop = "^/A/mu external\n"
  change_external(sbox.ospath('A'), externals_prop)
  expected_output = svntest.wc.State(wc_dir, {
      'A/external'      : Item(status='A '),
    })
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A'          : Item(props={'svn:externals':externals_prop}),
    'A/external' : Item("This is the file 'mu'.\n"),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/external' : Item(status='  ', wc_rev='2', switched='X'),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

  # Modify A/mu
  svntest.main.file_append(sbox.ospath('A/mu'), 'appended mu text')
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status.tweak('A/mu', wc_rev=3)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir)

  # Update to modify the file external, this asserts in update_editor.c
  expected_output = svntest.wc.State(wc_dir, {
      'A/external'      : Item(status='U '),
    })
  expected_disk.tweak('A/mu', 'A/external',
                      contents=expected_disk.desc['A/mu'].contents
                      + 'appended mu text')
  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.add({
    'A/external' : Item(status='  ', wc_rev='3', switched='X'),
    })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        True)

# Test for issue #2267
@Issue(2267)
def update_external_on_locally_added_dir(sbox):
  "update an external on a locally added dir"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"

  # Checkout a working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Add one new external item to the property on A/foo.  The new item is
  # "exdir_E", deliberately added in the middle not at the end.
  new_externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"           + \
           "\n"                                                   + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G/"     + \
           "\n"                                                   + \
           "exdir_E           " + other_repo_url + "/A/B/E"       + \
           "\n"                                                   + \
           "exdir_A/H -r 1 " + external_url_for["A/D/exdir_A/H"]  + \
           "\n"                                                   + \
           external_url_for["A/D/x/y/z/blah"] + " x/y/z/blah"     + \
           "\n"

  # Add A/foo and set the property on it
  new_dir = sbox.ospath("A/foo")
  sbox.simple_mkdir("A/foo")
  change_external(new_dir, new_externals_desc, commit=False)

  # Update the working copy, see if we get the new item.
  expected_output = svntest.wc.State(wc_dir, {
    'A/foo/exdir_A/B'   : Item(status='A '),
    'A/foo/exdir_A/B/E' : Item(status='A '),
    'A/foo/exdir_A/B/E/beta': Item(status='A '),
    'A/foo/exdir_A/B/E/alpha': Item(status='A '),
    'A/foo/exdir_A/B/F' : Item(status='A '),
    'A/foo/exdir_A/B/lambda': Item(status='A '),
    'A/foo/exdir_A/D'   : Item(status='A '),
    'A/foo/exdir_A/D/G' : Item(status='A '),
    'A/foo/exdir_A/D/G/rho': Item(status='A '),
    'A/foo/exdir_A/D/G/pi': Item(status='A '),
    'A/foo/exdir_A/D/G/tau': Item(status='A '),
    'A/foo/exdir_A/D/gamma': Item(status='A '),
    'A/foo/exdir_A/D/H' : Item(status='A '),
    'A/foo/exdir_A/D/H/chi': Item(status='A '),
    'A/foo/exdir_A/D/H/omega': Item(status='A '),
    'A/foo/exdir_A/D/H/psi': Item(status='A '),
    'A/foo/exdir_A/C'   : Item(status='A '),
    'A/foo/exdir_A/mu'  : Item(status='A '),
    'A/foo/exdir_A/H/omega': Item(status='A '),
    'A/foo/exdir_A/H/psi': Item(status='A '),
    'A/foo/exdir_A/H/chi': Item(status='A '),
    'A/foo/exdir_A/G/tau': Item(status='A '),
    'A/foo/exdir_A/G/rho': Item(status='A '),
    'A/foo/exdir_A/G/pi': Item(status='A '),
    'A/foo/x/y/z/blah/F': Item(status='A '),
    'A/foo/x/y/z/blah/E': Item(status='A '),
    'A/foo/x/y/z/blah/E/beta': Item(status='A '),
    'A/foo/x/y/z/blah/E/alpha': Item(status='A '),
    'A/foo/x/y/z/blah/lambda': Item(status='A '),
    'A/foo/exdir_E/beta': Item(status='A '),
    'A/foo/exdir_E/alpha': Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None, None)

  probe_paths_exist([os.path.join(wc_dir, "A", "foo", "exdir_E")])

# Test for issue #2267
@Issue(2267)
def switch_external_on_locally_added_dir(sbox):
  "switch an external on a locally added dir"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir

  repo_url       = sbox.repo_url
  other_repo_url = repo_url + ".other"
  A_path         = repo_url + "/A"
  A_copy_path    = repo_url + "/A_copy"

  # Create a branch of A
  # Checkout a working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'copy',
                                     A_path, A_copy_path,
                                     '-m', 'Create branch of A')

  # Checkout a working copy
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     A_path, wc_dir)

  # Add one new external item to the property on A/foo.  The new item is
  # "exdir_E", deliberately added in the middle not at the end.
  new_externals_desc = \
           external_url_for["A/D/exdir_A"] + " exdir_A"           + \
           "\n"                                                   + \
           external_url_for["A/D/exdir_A/G/"] + " exdir_A/G/"     + \
           "\n"                                                   + \
           "exdir_E           " + other_repo_url + "/A/B/E"       + \
           "\n"                                                   + \
           "exdir_A/H -r 1 " + external_url_for["A/D/exdir_A/H"]  + \
           "\n"                                                   + \
           external_url_for["A/D/x/y/z/blah"] + " x/y/z/blah"     + \
           "\n"

  # Add A/foo and set the property on it
  new_dir = sbox.ospath("foo")
  sbox.simple_mkdir("foo")
  change_external(new_dir, new_externals_desc, commit=False)

  # Switch the working copy to the branch, see if we get the new item.
  svntest.actions.run_and_verify_svn(None, None, [], 'sw', A_copy_path, wc_dir)

  probe_paths_exist([os.path.join(wc_dir, "foo", "exdir_E")])

@Issue(3819)
def file_external_in_sibling(sbox):
  "update a file external in sibling dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Setup A2/iota as file external to ^/iota
  externals_prop = "^/iota iota\n"
  sbox.simple_mkdir("A2")
  change_external(sbox.ospath('A2'), externals_prop)
  sbox.simple_update()

  os.chdir(sbox.ospath("A"))
  svntest.actions.run_and_verify_svn(None,
                            svntest.actions.expected_noop_update_output(2),
                            [], 'update')

@Issue(3823)
def file_external_update_without_commit(sbox):
  "update a file external without committing target"

  sbox.build(read_only=True)

  # Setup A2/iota as file external to ^/iota
  externals_prop = "^/iota iota\n"
  sbox.simple_mkdir("A2")
  change_external(sbox.ospath('A2'), externals_prop, commit=False)
  # A2/ is an uncommitted added dir with an svn:externals property set.
  sbox.simple_update()

def incoming_file_on_file_external(sbox):
  "bring in a new file over a file external"

  sbox.build()
  repo_url = sbox.repo_url
  wc_dir = sbox.wc_dir

  change_external(sbox.wc_dir, "^/A/B/lambda ext\n")
  # And bring in the file external
  sbox.simple_update()

  svntest.main.run_svn(None, 'cp', repo_url + '/iota',
                       repo_url + '/ext', '-m', 'copied')

  # Until recently this took over the file external as 'E'xisting file, with
  # a textual conflict.
  expected_output = svntest.wc.State(wc_dir, {
    'ext' : Item(verb='Skipped'),
    })
  svntest.actions.run_and_verify_update(wc_dir, expected_output, None, None)

def incoming_file_external_on_file(sbox):
  "bring in a new file external over a file"

  sbox.build()
  wc_dir = sbox.wc_dir

  change_external(sbox.wc_dir, "^/A/B/lambda iota\n")

  # And bring in the file external
  # Returns an error: WC status of external unchanged.
  svntest.actions.run_and_verify_update(wc_dir, None, None, None,
                                        '.*The file external.*overwrite.*')


def exclude_externals(sbox):
  "try to exclude externals"

  external_url_for = externals_test_setup(sbox)
  wc_dir         = sbox.wc_dir
  repo_url       = sbox.repo_url

  # Checkout two working copies.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'checkout',
                                     repo_url, wc_dir)

  # Excluding a file external should either fail (current behavior)
  # or register the file external as excluded (preferred behavior)
  svntest.actions.run_and_verify_update(sbox.ospath('A/B/gamma'),
                                        None, None, None,
                                        '.*Cannot exclude.*',
                                        None, None, None, None, False,
                                        '--set-depth', 'exclude',
                                        sbox.ospath('A/B/gamma'))

  # Excluding a directory external should either fail (current behavior)
  # or register the directory external as excluded (preferred behavior)
  svntest.actions.run_and_verify_update(sbox.ospath('A/C/exdir_G'),
                                        None, None, None,
                                        '.*Cannot exclude.*',
                                        None, None, None, None, False,
                                        '--set-depth', 'exclude',
                                        sbox.ospath('A/C/exdir_G'))

  # And after an update with --set-depth infinity all externals should
  # be there again.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 6)
  expected_status.add({
      'A/B/gamma'         : Item(status='  ', wc_rev='6', switched='X'),
      'A/C/exdir_H'       : Item(status='X '),
      'A/C/exdir_G'       : Item(status='X '),
      'A/D/exdir_A'       : Item(status='X '),
      'A/D/x'             : Item(status='X '),
  })
  svntest.actions.run_and_verify_update(wc_dir,
                                        None, None, expected_status, None,
                                        None, None, None, None, False,
                                        '--set-depth', 'infinity', wc_dir)

def file_externals_different_url(sbox):
  "update file externals via different url"

  sbox.build()

  wc_dir = sbox.wc_dir
  r1_url = sbox.repo_url

  r2_dir, r2_url = sbox.add_repo_path('2')
  svntest.main.copy_repos(sbox.repo_dir, r2_dir, 1, 0)


  sbox.simple_propset('svn:externals',
                      'r1-e-1   ' + r1_url + '/iota\n' +
                      r1_url + '/iota  r1-e-2\n' +
                      'r2-e-1   ' + r2_url + '/iota\n' +
                      r2_url + '/iota  r2-e-2\n' +
                      '^/iota  rr-e-1\n', '')

  # All file externals appear in the working copy, with normalised URLs.
  expected_output = svntest.wc.State(wc_dir, {
    'r1-e-1'            : Item(status='A '),
    'r1-e-2'            : Item(status='A '),
    'r2-e-1'            : Item(status='A '),
    'r2-e-2'            : Item(status='A '),
    'rr-e-1'            : Item(status='A '),
  })

  expected_status = actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('', status=' M')
  expected_status.add({
    'r2-e-1'            : Item(status='  ', wc_rev='1', switched='X'),
    'r1-e-1'            : Item(status='  ', wc_rev='1', switched='X'),
    'r1-e-2'            : Item(status='  ', wc_rev='1', switched='X'),
    'rr-e-1'            : Item(status='  ', wc_rev='1', switched='X'),
    'r2-e-2'            : Item(status='  ', wc_rev='1', switched='X'),
  })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None,
                                        expected_status, None)

  # Verify that all file external URLs are descendants of r1_url
  for e in ['r1-e-1', 'r1-e-2', 'r2-e-1', 'r2-e-2', 'rr-e-1']:
    actions.run_and_verify_info([{'Repository Root' : r1_url}],
                                os.path.join(sbox.wc_dir, e))
    

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'relocate', r1_url, r2_url, wc_dir)


  # URLs of existing file externals are silently rewritten
  expected_output = svntest.wc.State(wc_dir, {
  })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output, None,
                                        expected_status, None)

  # Verify that all file external URLs are descendants of r2_url
  for e in ['r1-e-1', 'r1-e-2', 'r2-e-1', 'r2-e-2', 'rr-e-1']:
    actions.run_and_verify_info([{'Repository Root' : r2_url}],
                                os.path.join(sbox.wc_dir, e))


def file_external_in_unversioned(sbox):
  "file external in unversioned dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  sbox.simple_propset('svn:externals', '^/A/mu X/mu', 'A')

  expected_output = svntest.wc.State(wc_dir, {
    'A/X/mu' : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(wc_dir, expected_output, None, None)

  # At one point this failed with SVN_DEBUG wcng consistency checks enabled
  svntest.actions.run_and_verify_svn(None, None, [], 'cleanup', wc_dir)


from svntest import verify, actions, main

@Issue(3589, 4000)
def copy_file_externals(sbox):
  "a WC->WC copy should exclude file externals"

  #  svntest.factory.make(sbox,"""
  #  svn mkdir X
  #  svn ps svn:externals "^/iota xiota" X
  #  """)

  sbox.build()
  wc_dir = sbox.wc_dir

  X = os.path.join(wc_dir, 'X')

  # svn mkdir X
  expected_stdout = ['A         ' + X + '\n']

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'mkdir', X)

  # svn ps svn:externals "^/iota xiota" X
  expected_stdout = ["property 'svn:externals' set on '" + X + "'\n"]

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'ps',
    'svn:externals', '''
    ^/iota xiota
    ^/A/mu xmu
    ''', X)

  #  svntest.factory.make(sbox, '''
  #  svn ci
  #  svn up
  #  # have a commit on one of the files
  #  echo mod >> X/xmu
  #  svn ci X/xmu
  #  svn up
  #  # now perform the WC->WC copy
  #  svn cp X X_copy
  #  ### manual edit: add a verify_disk(check_props=True) here
  #  svn ci
  #  ### manual edit: add check_props=True to below update
  #  svn up
  #  ''')

  X = os.path.join(wc_dir, 'X')
  X_copy = os.path.join(wc_dir, 'X_copy')
  X_xmu = os.path.join(wc_dir, 'X', 'xmu')

  # svn ci
  expected_output = svntest.wc.State(wc_dir, {
    'X'                 : Item(verb='Adding'),
  })

  expected_status = actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'X'                 : Item(status='  ', wc_rev='2'),
  })

  actions.run_and_verify_commit(wc_dir, expected_output, expected_status,
    None, wc_dir)

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'X/xmu'             : Item(status='A '),
    'X/xiota'           : Item(status='A '),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'X'                 : Item(),
    'X/xiota'           : Item(contents="This is the file 'iota'.\n"),
    'X/xmu'             : Item(contents="This is the file 'mu'.\n"),
  })

  expected_status.add({
    'X/xiota'           : Item(status='  ', wc_rev='2', switched='X'),
    'X/xmu'             : Item(status='  ', wc_rev='2', switched='X'),
  })
  expected_status.tweak(wc_rev='2')

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)

  # have a commit on one of the files
  # echo mod >> X/xmu
  main.file_append(X_xmu, 'mod\n')

  # svn ci X/xmu
  expected_output = svntest.wc.State(wc_dir, {
    'X/xmu'             : Item(verb='Sending'),
  })

  expected_status.tweak('X/xmu', wc_rev='3')

  actions.run_and_verify_commit(wc_dir, expected_output, expected_status,
    None, X_xmu)

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'A/mu'              : Item(status='U '),
  })

  expected_disk.tweak('A/mu', 'X/xmu',
    contents="This is the file 'mu'.\nmod\n")

  expected_status.tweak(wc_rev='3')

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)

  # now perform the WC->WC copy
  # svn cp X X_copy
  expected_stdout = ['A         ' + X_copy + '\n']

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'cp', X,
    X_copy)

  # svn ci
  expected_output = svntest.wc.State(wc_dir, {
    'X_copy'            : Item(verb='Adding'),
  })

  expected_status.add({
    'X_copy'            : Item(status='  ', wc_rev='4'),
  })

  actions.run_and_verify_commit(wc_dir, expected_output, expected_status,
    None, wc_dir)

  # verify disk state, also verifying props
  expected_disk.add({
    'X_copy'            : Item(),
  })
  expected_disk.tweak('X', 'X_copy',
    props={'svn:externals' : '\n    ^/iota xiota\n    ^/A/mu xmu\n    \n'})

  actions.verify_disk(wc_dir, expected_disk, True)

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'X_copy/xmu'        : Item(status='A '),
    'X_copy/xiota'      : Item(status='A '),
  })

  expected_disk.add({
    'X_copy/xmu'        : Item(contents="This is the file 'mu'.\nmod\n"),
    'X_copy/xiota'      : Item(contents="This is the file 'iota'.\n"),
  })

  expected_status.add({
    'X_copy/xmu'        : Item(status='  ', wc_rev='4', switched='X'),
    'X_copy/xiota'      : Item(status='  ', wc_rev='4', switched='X'),
  })
  expected_status.tweak(wc_rev='4')

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, True, wc_dir)

# Test for issue #4093 'remapping a file external can segfault due to
# "deleted" props'.
@Issue(4093)
def remap_file_external_with_prop_del(sbox):
  "file external remap segfaults due to deleted props"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path  = os.path.join(wc_dir, "A")
  mu_path = os.path.join(wc_dir, "A", "mu")

  # Add a property to A/mu
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps', 'propname', 'propval', mu_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'commit', '-m', 'New property on a file',
                                     wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  # Add a new file external A/external pointing to ^/A/mu
  externals_prop = "^/A/mu external\n"
  change_external(A_path, externals_prop)
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  # Change A/external to point to ^/iota
  externals_prop = "^/iota external\n"
  change_external(A_path, externals_prop)

  # Now update to bring the new external down.
  # This previously segfaulted as described in
  # http://subversion.tigris.org/issues/show_bug.cgi?id=4093#desc1
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)


# Test for issue #4053 'svn:externals with explicit rev checks out HEAD'
@Issue(4053)
def dir_external_with_dash_r_only(sbox):
  "whether '-r1 ^/A B' updates properly"
  # svntest.factory.make(sbox,"""
  #   echo 'newer alpha' > A/B/E/alpha
  #   svn ci
  #   svn ps svn:externals ' -r1 ^/A/B/E E_ext' .
  #   svn up
  #   # ^ move the 'status.tweak(wc_rev=2)' above the 'add()' call
  #   svn info E_ext
  #   # ^ change the 'svn info' call to
  #   #  expected_info = { 'Revision': '1' }
  #   #  actions.run_and_verify_info([expected_info], E_ext)
  #   """)

  sbox.build()
  wc_dir = sbox.wc_dir
  url = sbox.repo_url

  A_B_E_alpha = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  E_ext = os.path.join(wc_dir, 'E_ext')

  # echo 'newer alpha' > A/B/E/alpha
  main.file_write(A_B_E_alpha, 'newer alpha\n')

  # svn ci
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/E/alpha'       : Item(verb='Sending'),
  })

  expected_status = actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/E/alpha', wc_rev='2')

  actions.run_and_verify_commit(wc_dir, expected_output, expected_status,
    None, wc_dir)

  # svn ps svn:externals ' -r1 ^/A/B/E E_ext' .
  expected_stdout = ["property 'svn:externals' set on '" + wc_dir + "'\n"]

  actions.run_and_verify_svn2('OUTPUT', expected_stdout, [], 0, 'ps',
    'svn:externals', ' -r1 ^/A/B/E E_ext', wc_dir)

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
    'E_ext/beta'        : Item(status='A '),
    'E_ext/alpha'       : Item(status='A '),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'E_ext'             : Item(),
    'E_ext/alpha'       : Item(contents="This is the file 'alpha'.\n"),
    'E_ext/beta'        : Item(contents="This is the file 'beta'.\n"),
  })
  expected_disk.tweak('A/B/E/alpha', contents='newer alpha\n')

  expected_status.tweak(wc_rev='2')
  expected_status.tweak('', status=' M')
  expected_status.add({
    'E_ext'             : Item(status='X '),
  })

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)

  # svn info E_ext/alpha
  expected_info = { 'Revision': '1' }
  actions.run_and_verify_info([expected_info], E_ext)

# Test for issue #4123 'URL-to-WC copy of externals fails on Windows'
@Issue(4123)
def url_to_wc_copy_of_externals(sbox):
  "url-to-wc copy of externals"

  sbox.build()

  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Create an external A/C/external pointing to ^/A/D/G.
  svntest.actions.run_and_verify_svn(None, None, [], 'ps',
                                     'svn:externals', '^/A/D/G external',
                                     os.path.join(wc_dir, 'A', 'C'))
  svntest.actions.run_and_verify_svn(None, None, [], 'ci', '-m',
                                     'create an external', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)

  # Copy ^/A/C to External-WC-to-URL-Copy.
  #
  # Previously this failed with:
  #   >svn copy ^^/A/C External-WC-to-URL-Copy
  #    U   External-WC-to-URL-Copy
  #   
  #   Fetching external item into 'External-WC-to-URL-Copy\external':
  #   A    External-WC-to-URL-Copy\external\pi
  #   A    External-WC-to-URL-Copy\external\rho
  #   A    External-WC-to-URL-Copy\external\tau
  #   Checked out external at revision 2.
  #
  #   Checked out revision 2.
  #   ..\..\..\subversion\libsvn_client\copy.c:2249: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_client\copy.c:1857: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_client\copy.c:1857: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_client\copy.c:1737: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_client\copy.c:1737: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_client\copy.c:1537: (apr_err=720005)
  #   ..\..\..\subversion\libsvn_subr\io.c:3416: (apr_err=720005)
  #   svn: E720005: Can't move 'C:\SVN\src-trunk-3\Debug\subversion\tests\
  #   cmdline\svn-test-work\working_copies\externals_tests-41\.svn\tmp\
  #   svn-F9E2C0EC' to 'C:\SVN\src-trunk-3\Debug\subversion\tests\cmdline\
  #   svn-test-work\working_copies\externals_tests-41\External-WC-to-URL-Copy':
  #   Access is denied.
  external_root_path = os.path.join(wc_dir, "External-WC-to-URL-Copy")
  external_ex_path = os.path.join(wc_dir, "External-WC-to-URL-Copy",
                                  "external")
  external_pi_path = os.path.join(wc_dir, "External-WC-to-URL-Copy",
                                  "external", "pi")
  external_rho_path = os.path.join(wc_dir, "External-WC-to-URL-Copy",
                                   "external", "rho")
  external_tau_path = os.path.join(wc_dir, "External-WC-to-URL-Copy",
                                   "external", "tau")
  expected_stdout = verify.UnorderedOutput([
    "\n",
    " U   " + external_root_path + "\n",
    "Fetching external item into '" + external_ex_path + "':\n",
    "A    " + external_pi_path + "\n",
    "A    " + external_rho_path + "\n",
    "A    " + external_tau_path + "\n",
    "Checked out external at revision 2.\n",
    "Checked out revision 2.\n",
    "A         " + external_root_path + "\n"
  ])
  exit_code, stdout, stderr = svntest.actions.run_and_verify_svn2(
    "OUTPUT", expected_stdout, [], 0, 'copy', repo_url + '/A/C',
    os.path.join(wc_dir, 'External-WC-to-URL-Copy'))

# Test for issue #3741 'externals not removed when working copy is made shallow'
@Issue(3741)
def update_dir_external_shallow(sbox):
  "shallow update should remove externals"

  sbox.build()

  # Create an external in r2
  sbox.simple_propset('svn:externals', '^/A/D/H X', 'A/B/E')
  sbox.simple_commit()
  sbox.simple_update()

  # Now make A/B/E shallow by updating with "--set-depth empty"
  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/B/E/alpha' : Item(status='D '),
    'A/B/E/X'     : Item(verb='Removed external'),
    'A/B/E/beta'  : Item(status='D '),
  })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output, None, None,
                                        None, None, None, None, None, False,
                                        '--set-depth=empty',
                                        sbox.ospath('A/B/E'))

  # And bring the external back by updating with "--set-depth infinity"
  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/B/E/X/psi'   : Item(status='A '),
    'A/B/E/X/chi'   : Item(status='A '),
    'A/B/E/X/omega' : Item(status='A '),
    'A/B/E/alpha'   : Item(status='A '),
    'A/B/E/beta'    : Item(status='A '),
  })
  svntest.actions.run_and_verify_update(sbox.wc_dir,
                                        expected_output, None, None,
                                        None, None, None, None, None, False,
                                        '--set-depth=infinity',
                                        sbox.ospath('A/B/E'))


########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              checkout_with_externals,
              update_receive_new_external,
              update_lose_external,
              update_change_pristine_external,
              update_change_modified_external,
              update_receive_change_under_external,
              modify_and_update_receive_new_external,
              disallow_dot_or_dotdot_directory_reference,
              export_with_externals,
              export_wc_with_externals,
              external_with_peg_and_op_revision,
              new_style_externals,
              disallow_propset_invalid_formatted_externals,
              old_style_externals_ignore_peg_reg,
              cannot_move_or_remove_file_externals,
              cant_place_file_external_into_dir_external,
              external_into_path_with_spaces,
              binary_file_externals,
              update_lose_file_external,
              switch_relative_external,
              export_sparse_wc_with_externals,
              relegate_external,
              wc_repos_file_externals,
              merge_target_with_externals,
              update_modify_file_external,
              update_external_on_locally_added_dir,
              switch_external_on_locally_added_dir,
              file_external_in_sibling,
              file_external_update_without_commit,
              incoming_file_on_file_external,
              incoming_file_external_on_file,
              exclude_externals,
              file_externals_different_url,
              file_external_in_unversioned,
              copy_file_externals,
              remap_file_external_with_prop_del,
              dir_external_with_dash_r_only,
              url_to_wc_copy_of_externals,
              update_dir_external_shallow,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
