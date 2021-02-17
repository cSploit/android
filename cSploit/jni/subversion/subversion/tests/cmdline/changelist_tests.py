#!/usr/bin/env python
#
#  changelist_tests.py:  testing changelist uses.
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
import string, sys, os, re

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
# Utilities


def mod_all_files(wc_dir, new_text):
  """Walk over working copy WC_DIR, appending NEW_TEXT to all the
  files in that tree (but not inside the .svn areas of that tree)."""

  dot_svn = svntest.main.get_admin_name()
  for dirpath, dirs, files in os.walk(wc_dir):
    if dot_svn in dirs:
      dirs.remove(dot_svn)
    for name in files:
      svntest.main.file_append(os.path.join(dirpath, name), new_text)

def changelist_all_files(wc_dir, name_func):
  """Walk over working copy WC_DIR, adding versioned files to
  changelists named by invoking NAME_FUNC(full-path-of-file) and
  noting its string return value (or None, if we wish to remove the
  file from a changelist)."""

  dot_svn = svntest.main.get_admin_name()
  for dirpath, dirs, files in os.walk(wc_dir):
    if dot_svn in dirs:
      dirs.remove(dot_svn)
    for name in files:
        full_path = os.path.join(dirpath, name)
        clname = name_func(full_path)
        if not clname:
          svntest.main.run_svn(None, "changelist", "--remove", full_path)
        else:
          svntest.main.run_svn(None, "changelist", clname, full_path)

def clname_from_lastchar_cb(full_path):
  """Callback for changelist_all_files() that returns a changelist
  name matching the last character in the file's name.  For example,
  after running this on a greek tree where every file has some text
  modification, 'svn status' shows:

    --- Changelist 'a':
    M      A/B/lambda
    M      A/B/E/alpha
    M      A/B/E/beta
    M      A/D/gamma
    M      A/D/H/omega
    M      iota

    --- Changelist 'u':
    M      A/mu
    M      A/D/G/tau

    --- Changelist 'i':
    M      A/D/G/pi
    M      A/D/H/chi
    M      A/D/H/psi

    --- Changelist 'o':
    M      A/D/G/rho
    """
  return full_path[-1]


# Regular expressions for 'svn changelist' output.
_re_cl_rem_pattern = "^D \[(.*)\] (.*)"
_re_cl_skip = re.compile("Skipped '(.*)'")
_re_cl_add  = re.compile("^A \[(.*)\] (.*)")
_re_cl_rem  = re.compile(_re_cl_rem_pattern)

def verify_changelist_output(output, expected_adds=None,
                             expected_removals=None,
                             expected_skips=None):
  """Compare lines of OUTPUT from 'svn changelist' against
  EXPECTED_ADDS (a dictionary mapping paths to changelist names),
  EXPECTED_REMOVALS (a dictionary mapping paths to ... whatever), and
  EXPECTED_SKIPS (a dictionary mapping paths to ... whatever).

  EXPECTED_SKIPS is ignored if None."""

  num_expected = 0
  if expected_adds:
    num_expected += len(expected_adds)
  if expected_removals:
    num_expected += len(expected_removals)
  if expected_skips:
    num_expected += len(expected_skips)

  if not expected_skips:
    output = [line for line in output if (not _re_cl_skip.match(line))]

  if len(output) != num_expected:
    raise svntest.Failure("Unexpected number of 'svn changelist' output lines")

  for line in output:
    line = line.rstrip()
    match = _re_cl_rem.match(line)
    if match \
       and expected_removals \
       and match.group(2) in expected_removals:
        continue
    elif match:
      raise svntest.Failure("Unexpected changelist removal line: " + line)
    match = _re_cl_add.match(line)
    if match \
       and expected_adds \
       and expected_adds.get(match.group(2)) == match.group(1):
        continue
    elif match:
      raise svntest.Failure("Unexpected changelist add line: " + line)
    match = _re_cl_skip.match(line)
    if match \
       and expected_skips \
       and match.group(2) in expected_skips:
        continue
    elif match:
      raise svntest.Failure("Unexpected changelist skip line: " + line)
    raise svntest.Failure("Unexpected line: " + line)

def verify_pget_output(output, expected_props):
  """Compare lines of OUTPUT from 'svn propget' against EXPECTED_PROPS
  (a dictionary mapping paths to property values)."""

  _re_pget = re.compile('^(.*) - (.*)$')
  actual_props = {}
  for line in output:
    try:
      path, prop = line.rstrip().split(' - ')
    except:
      raise svntest.Failure("Unexpected output line: " + line)
    actual_props[path] = prop
  if expected_props != actual_props:
    raise svntest.Failure("Got unexpected property results\n"
                          "\tExpected: %s\n"
                          "\tActual: %s" % (str(expected_props),
                                            str(actual_props)))


######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

def add_remove_changelists(sbox):
  "add and remove files from changelists"

  sbox.build()
  wc_dir = sbox.wc_dir

  ### 'Skip' notifications

  def expected_skips_under(*greek_path):
    "return a dict mapping Greek-tree directories below GREEK_PATH to None"

    expected_skips = {}
    for path in expected_skips_all:
      if path.startswith(os.path.join(wc_dir, *greek_path)):
        expected_skips[path] = None

    return expected_skips

  def all_parents(expected_adds):
    """return a dict mapping Greek-tree directories above directories in
       EXPECTED_ADDS to None"""

    expected_skips = {}
    for path in expected_adds.keys():
      if not os.path.isdir(path):
        path = os.path.dirname(path)

      while path != wc_dir:
        expected_skips[path] = None
        path = os.path.dirname(path)

    expected_skips[wc_dir] = None
    return expected_skips

  # all dirs in the Greek tree
  expected_skips_all = dict([(x, None) for x in [
        os.path.join(wc_dir),
    os.path.join(wc_dir, 'A'),
    os.path.join(wc_dir, 'A', 'B'),
    os.path.join(wc_dir, 'A', 'B', 'E'),
    os.path.join(wc_dir, 'A', 'B', 'F'),
    os.path.join(wc_dir, 'A', 'C'),
    os.path.join(wc_dir, 'A', 'D'),
    os.path.join(wc_dir, 'A', 'D', 'G'),
    os.path.join(wc_dir, 'A', 'D', 'H'),
    ]])

  expected_skips_wc_dir = { wc_dir : None }

  ### First, we play with just adding to changelists ###

  # svn changelist foo WC_DIR
  exit_code, output, errput = svntest.main.run_svn(None, "changelist", "foo",
                                                   wc_dir)
  verify_changelist_output(output) # nothing expected

  # svn changelist foo WC_DIR --depth files
  exit_code, output, errput = svntest.main.run_svn(None, "changelist", "foo",
                                                   "--depth", "files",
                                                   wc_dir)
  expected_adds = {
    os.path.join(wc_dir, 'iota') : 'foo',
    }
  verify_changelist_output(output, expected_adds)

  # svn changelist foo WC_DIR --depth infinity
  exit_code, output, errput = svntest.main.run_svn(None, "changelist", "foo",
                                                   "--depth", "infinity",
                                                   wc_dir)
  expected_adds = {
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : 'foo',
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta') : 'foo',
    os.path.join(wc_dir, 'A', 'B', 'lambda') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'G', 'pi') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'G', 'rho') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'G', 'tau') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : 'foo',
    os.path.join(wc_dir, 'A', 'D', 'gamma') : 'foo',
    os.path.join(wc_dir, 'A', 'mu') : 'foo',
    }
  verify_changelist_output(output, expected_adds)

  ### Now, change some changelists ###

  # svn changelist bar WC_DIR/A/D --depth infinity
  exit_code, output, errput = svntest.main.run_svn(".*", "changelist", "bar",
                                                   "--depth", "infinity",
                                                   os.path.join(wc_dir,
                                                                'A', 'D'))
  expected_adds = {
    os.path.join(wc_dir, 'A', 'D', 'G', 'pi') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'G', 'rho') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'G', 'tau') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'gamma') : 'bar',
    }
  expected_removals = expected_adds
  verify_changelist_output(output, expected_adds, expected_removals)

  # svn changelist baz WC_DIR/A/D/H --depth infinity
  exit_code, output, errput = svntest.main.run_svn(".*", "changelist", "baz",
                                                   "--depth", "infinity",
                                                   os.path.join(wc_dir, 'A',
                                                                'D', 'H'))
  expected_adds = {
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : 'baz',
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : 'baz',
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : 'baz',
    }
  expected_removals = expected_adds
  verify_changelist_output(output, expected_adds, expected_removals)

  ### Now, let's selectively rename some changelists ###

  # svn changelist foo-rename WC_DIR --depth infinity --changelist foo
  exit_code, output, errput = svntest.main.run_svn(".*", "changelist",
                                                   "foo-rename",
                                                   "--depth", "infinity",
                                                   "--changelist", "foo",
                                                   wc_dir)
  expected_adds = {
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : 'foo-rename',
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta') : 'foo-rename',
    os.path.join(wc_dir, 'A', 'B', 'lambda') : 'foo-rename',
    os.path.join(wc_dir, 'A', 'mu') : 'foo-rename',
    os.path.join(wc_dir, 'iota') : 'foo-rename',
    }
  expected_removals = expected_adds
  verify_changelist_output(output, expected_adds, expected_removals)

  # svn changelist bar WC_DIR --depth infinity
  #     --changelist foo-rename --changelist baz
  exit_code, output, errput = svntest.main.run_svn(
    ".*", "changelist", "bar", "--depth", "infinity",
    "--changelist", "foo-rename", "--changelist", "baz", wc_dir)

  expected_adds = {
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : 'bar',
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta') : 'bar',
    os.path.join(wc_dir, 'A', 'B', 'lambda') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : 'bar',
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : 'bar',
    os.path.join(wc_dir, 'A', 'mu') : 'bar',
    os.path.join(wc_dir, 'iota') : 'bar',
    }
  expected_removals = expected_adds
  verify_changelist_output(output, expected_adds, expected_removals)

  ### Okay.  Time to remove some stuff from changelists now. ###

  # svn changelist --remove WC_DIR
  exit_code, output, errput = svntest.main.run_svn(None, "changelist",
                                                   "--remove", wc_dir)
  verify_changelist_output(output) # nothing expected

  # svn changelist --remove WC_DIR --depth files
  exit_code, output, errput = svntest.main.run_svn(None, "changelist",
                                                   "--remove",
                                                   "--depth", "files",
                                                   wc_dir)
  expected_removals = {
    os.path.join(wc_dir, 'iota') : None,
    }
  verify_changelist_output(output, None, expected_removals)

  # svn changelist --remove WC_DIR --depth infinity
  exit_code, output, errput = svntest.main.run_svn(None, "changelist",
                                                   "--remove",
                                                   "--depth", "infinity",
                                                   wc_dir)
  expected_removals = {
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : None,
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta') : None,
    os.path.join(wc_dir, 'A', 'B', 'lambda') : None,
    os.path.join(wc_dir, 'A', 'D', 'G', 'pi') : None,
    os.path.join(wc_dir, 'A', 'D', 'G', 'rho') : None,
    os.path.join(wc_dir, 'A', 'D', 'G', 'tau') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : None,
    os.path.join(wc_dir, 'A', 'D', 'gamma') : None,
    os.path.join(wc_dir, 'A', 'mu') : None,
    }
  verify_changelist_output(output, None, expected_removals)

  ### Add files to changelists based on the last character in their names ###

  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  ### Now, do selective changelist removal ###

  # svn changelist --remove WC_DIR --depth infinity --changelist a
  exit_code, output, errput = svntest.main.run_svn(None, "changelist",
                                                   "--remove",
                                                   "--depth", "infinity",
                                                   "--changelist", "a",
                                                   wc_dir)
  expected_removals = {
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : None,
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta') : None,
    os.path.join(wc_dir, 'A', 'B', 'lambda') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : None,
    os.path.join(wc_dir, 'A', 'D', 'gamma') : None,
    os.path.join(wc_dir, 'iota') : None,
    }
  verify_changelist_output(output, None, expected_removals)

  # svn changelist --remove WC_DIR --depth infinity
  #     --changelist i --changelist o
  exit_code, output, errput = svntest.main.run_svn(None, "changelist",
                                                   "--remove",
                                                   "--depth", "infinity",
                                                   "--changelist", "i",
                                                   "--changelist", "o",
                                                   wc_dir)
  expected_removals = {
    os.path.join(wc_dir, 'A', 'D', 'G', 'pi') : None,
    os.path.join(wc_dir, 'A', 'D', 'G', 'rho') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'chi') : None,
    os.path.join(wc_dir, 'A', 'D', 'H', 'psi') : None,
    }
  verify_changelist_output(output, None, expected_removals)

#----------------------------------------------------------------------

def commit_one_changelist(sbox):
  "commit with single --changelist"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a line of text to all the versioned files in the tree.
  mod_all_files(wc_dir, "New text.\n")

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Now, test a commit that uses a single changelist filter (--changelist a).
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/lambda' : Item(verb='Sending'),
    'A/B/E/alpha' : Item(verb='Sending'),
    'A/B/E/beta' : Item(verb='Sending'),
    'A/D/gamma' : Item(verb='Sending'),
    'A/D/H/omega' : Item(verb='Sending'),
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', 'A/D/G/tau', 'A/D/G/pi', 'A/D/H/chi',
                        'A/D/H/psi', 'A/D/G/rho', wc_rev=1, status='M ')
  expected_status.tweak('iota', 'A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/D/gamma', 'A/D/H/omega', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir,
                                        "--changelist",
                                        "a")

#----------------------------------------------------------------------

def commit_multiple_changelists(sbox):
  "commit with multiple --changelist's"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a line of text to all the versioned files in the tree.
  mod_all_files(wc_dir, "New text.\n")

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Now, test a commit that uses multiple changelist filters
  # (--changelist=a --changelist=i).
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/lambda' : Item(verb='Sending'),
    'A/B/E/alpha' : Item(verb='Sending'),
    'A/B/E/beta' : Item(verb='Sending'),
    'A/D/gamma' : Item(verb='Sending'),
    'A/D/H/omega' : Item(verb='Sending'),
    'iota' : Item(verb='Sending'),
    'A/D/G/pi' : Item(verb='Sending'),
    'A/D/H/chi' : Item(verb='Sending'),
    'A/D/H/psi' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', 'A/D/G/tau', 'A/D/G/rho',
                        wc_rev=1, status='M ')
  expected_status.tweak('iota', 'A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/D/gamma', 'A/D/H/omega', 'A/D/G/pi', 'A/D/H/chi',
                        'A/D/H/psi', wc_rev=2, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir,
                                        "--changelist", "a",
                                        "--changelist", "i")

#----------------------------------------------------------------------

def info_with_changelists(sbox):
  "info --changelist"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Now, test various combinations of changelist specification and depths.
  for clname in [['a'], ['i'], ['a', 'i']]:
    for depth in [None, 'files', 'infinity']:

      # Figure out what we expect to see in our info output.
      expected_paths = []
      if 'a' in clname:
        if depth == 'infinity':
          expected_paths.append('A/B/lambda')
          expected_paths.append('A/B/E/alpha')
          expected_paths.append('A/B/E/beta')
          expected_paths.append('A/D/gamma')
          expected_paths.append('A/D/H/omega')
        if depth == 'files' or depth == 'infinity':
          expected_paths.append('iota')
      if 'i' in clname:
        if depth == 'infinity':
          expected_paths.append('A/D/G/pi')
          expected_paths.append('A/D/H/chi')
          expected_paths.append('A/D/H/psi')
      expected_paths = sorted([os.path.join(wc_dir, x.replace('/', os.sep)) for x in expected_paths])

      # Build the command line.
      args = ['info', wc_dir]
      for cl in clname:
        args.append('--changelist')
        args.append(cl)
      if depth:
        args.append('--depth')
        args.append(depth)

      # Run 'svn info ...'
      exit_code, output, errput = svntest.main.run_svn(None, *args)

      # Filter the output for lines that begin with 'Path:', and
      # reduce even those lines to just the actual path.
      paths = sorted([x[6:].rstrip() for x in output if x[:6] == 'Path: '])

      # And, compare!
      if (paths != expected_paths):
        raise svntest.Failure("Expected paths (%s) and actual paths (%s) "
                              "don't gel" % (str(expected_paths), str(paths)))

#----------------------------------------------------------------------

def diff_with_changelists(sbox):
  "diff --changelist (wc-wc and repos-wc)"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a line of text to all the versioned files in the tree.
  mod_all_files(wc_dir, "New text.\n")

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Now, test various combinations of changelist specification and depths.
  for is_repos_wc in [0, 1]:
    for clname in [['a'], ['i'], ['a', 'i']]:
      for depth in ['files', 'infinity']:

        # Figure out what we expect to see in our diff output.
        expected_paths = []
        if 'a' in clname:
          if depth == 'infinity':
            expected_paths.append('A/B/lambda')
            expected_paths.append('A/B/E/alpha')
            expected_paths.append('A/B/E/beta')
            expected_paths.append('A/D/gamma')
            expected_paths.append('A/D/H/omega')
          if depth == 'files' or depth == 'infinity':
            expected_paths.append('iota')
        if 'i' in clname:
          if depth == 'infinity':
            expected_paths.append('A/D/G/pi')
            expected_paths.append('A/D/H/chi')
            expected_paths.append('A/D/H/psi')
        expected_paths = sorted([os.path.join(wc_dir, x.replace('/', os.sep)) for x in expected_paths])

        # Build the command line.
        args = ['diff']
        for cl in clname:
          args.append('--changelist')
          args.append(cl)
        if depth:
          args.append('--depth')
          args.append(depth)
        if is_repos_wc:
          args.append('--old')
          args.append(sbox.repo_url)
          args.append('--new')
          args.append(sbox.wc_dir)
        else:
          args.append(wc_dir)

        # Run 'svn diff ...'
        exit_code, output, errput = svntest.main.run_svn(None, *args)

        # Filter the output for lines that begin with 'Index:', and
        # reduce even those lines to just the actual path.
        paths = sorted([x[7:].rstrip() for x in output if x[:7] == 'Index: '])

        # Diff output on Win32 uses '/' path separators.
        if sys.platform == 'win32':
          paths = [x.replace('/', os.sep) for x in paths]

        # And, compare!
        if (paths != expected_paths):
          raise svntest.Failure("Expected paths (%s) and actual paths (%s) "
                                "don't gel"
                                % (str(expected_paths), str(paths)))

#----------------------------------------------------------------------

def propmods_with_changelists(sbox):
  "propset/del/get/list --changelist"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Set property 'name'='value' on all working copy items.
  svntest.main.run_svn(None, "pset", "--depth", "infinity",
                       "name", "value", wc_dir)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({'' : Item(props={ 'name' : 'value' })})
  expected_disk.tweak('A', 'A/B', 'A/B/E', 'A/B/E/alpha', 'A/B/E/beta',
                      'A/B/F', 'A/B/lambda', 'A/C', 'A/D', 'A/D/G',
                      'A/D/G/pi', 'A/D/G/rho', 'A/D/G/tau', 'A/D/H',
                      'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi', 'A/D/gamma',
                      'A/mu', 'iota', props={ 'name' : 'value' })
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Proplist the 'i' changelist
  exit_code, output, errput = svntest.main.run_svn(None, "proplist", "--depth",
                                                   "infinity", "--changelist",
                                                   "i", wc_dir)
  ### Really simple sanity check on the output of 'proplist'.  If we've got
  ### a proper proplist content checker anywhere, we should probably use it
  ### instead.
  if len(output) != 6:
    raise svntest.Failure

  # Remove the 'name' property from files in the 'o' and 'i' changelists.
  svntest.main.run_svn(None, "pdel", "--depth", "infinity",
                       "name", "--changelist", "o", "--changelist", "i",
                       wc_dir)
  expected_disk.tweak('A/D/G/pi', 'A/D/G/rho', 'A/D/H/chi', 'A/D/H/psi',
                      props={})
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Add 'foo'='bar' property on all files under A/B to depth files and
  # in changelist 'a'.
  svntest.main.run_svn(None, "pset", "--depth", "files",
                       "foo", "bar", "--changelist", "a",
                       os.path.join(wc_dir, 'A', 'B'))
  expected_disk.tweak('A/B/lambda', props={ 'name' : 'value',
                                            'foo'  : 'bar' })
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Add 'bloo'='blarg' property to all files in changelist 'a'.
  svntest.main.run_svn(None, "pset", "--depth", "infinity",
                       "bloo", "blarg", "--changelist", "a",
                       wc_dir)
  expected_disk.tweak('A/B/lambda', props={ 'name' : 'value',
                                            'foo'  : 'bar',
                                            'bloo' : 'blarg' })
  expected_disk.tweak('A/B/E/alpha', 'A/B/E/beta', 'A/D/H/omega', 'A/D/gamma',
                      'iota', props={ 'name' : 'value',
                                      'bloo' : 'blarg' })
  actual_disk_tree = svntest.tree.build_tree_from_wc(wc_dir, 1)
  svntest.tree.compare_trees("disk", actual_disk_tree,
                             expected_disk.old_tree())

  # Propget 'name' in files in changelists 'a' and 'i' to depth files.
  exit_code, output, errput = svntest.main.run_svn(None, "pget",
                                                   "--depth", "files", "name",
                                                   "--changelist", "a",
                                                   "--changelist", "i",
                                                   wc_dir)
  verify_pget_output(output, {
    os.path.join(wc_dir, 'iota') : 'value',
    })

  # Propget 'name' in files in changelists 'a' and 'i' to depth infinity.
  exit_code, output, errput = svntest.main.run_svn(None, "pget",
                                                   "--depth", "infinity",
                                                   "name",
                                                   "--changelist", "a",
                                                   "--changelist", "i",
                                                   wc_dir)
  verify_pget_output(output, {
    os.path.join(wc_dir, 'A', 'D', 'gamma')      : 'value',
    os.path.join(wc_dir, 'A', 'B', 'E', 'alpha') : 'value',
    os.path.join(wc_dir, 'iota')                 : 'value',
    os.path.join(wc_dir, 'A', 'B', 'E', 'beta')  : 'value',
    os.path.join(wc_dir, 'A', 'B', 'lambda')     : 'value',
    os.path.join(wc_dir, 'A', 'D', 'H', 'omega') : 'value',
    })


#----------------------------------------------------------------------

def revert_with_changelists(sbox):
  "revert --changelist"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  # Add a line of text to all the versioned files in the tree.
  mod_all_files(wc_dir, "Please, oh please, revert me!\n")
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/B/lambda', 'A/B/E/alpha', 'A/B/E/beta',
                        'A/D/gamma', 'A/D/H/omega', 'iota', 'A/mu',
                        'A/D/G/tau', 'A/D/G/pi', 'A/D/H/chi',
                        'A/D/H/psi', 'A/D/G/rho', status='M ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # 'svn revert --changelist a WC_DIR' (without depth, no change expected)
  svntest.main.run_svn(None, "revert", "--changelist", "a", wc_dir)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # 'svn revert --changelist o --depth files WC_DIR WC_DIR/A/B' (no change)
  svntest.main.run_svn(None, "revert", "--depth", "files",
                       "--changelist", "o",
                       wc_dir, os.path.join(wc_dir, 'A', 'B'))
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # 'svn revert --changelist a --depth files WC_DIR WC_DIR/A/B'
  # (iota, lambda reverted)
  svntest.main.run_svn(None, "revert", "--depth", "files",
                       "--changelist", "a",
                       wc_dir, os.path.join(wc_dir, 'A', 'B'))
  expected_status.tweak('iota', 'A/B/lambda', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # 'svn revert --changelist a --changelist i --depth infinity WC_DIR'
  # (alpha, beta, gamma, omega, pi, chi, psi reverted)
  svntest.main.run_svn(None, "revert", "--depth", "infinity",
                       "--changelist", "a", "--changelist", "i",
                       wc_dir)
  expected_status.tweak('A/B/E/alpha', 'A/B/E/beta', 'A/D/gamma',
                        'A/D/H/omega', 'A/D/G/pi', 'A/D/H/chi',
                        'A/D/H/psi', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # 'svn revert --depth infinity WC_DIR' (back to pristine-ness)
  svntest.main.run_svn(None, "revert", "--depth", "infinity",
                       wc_dir)
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------

def update_with_changelists(sbox):
  "update --changelist"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a line of text to all the versioned files in the tree, commit, update.
  mod_all_files(wc_dir, "Added line.\n")
  svntest.main.run_svn(None, "commit", "-m", "logmsg", wc_dir)
  svntest.main.run_svn(None, "update", wc_dir)

  # Add files to changelists based on the last character in their names.
  changelist_all_files(wc_dir, clname_from_lastchar_cb)

  ### Backdate only the files in the 'a' and 'i' changelists at depth
  ### files under WC_DIR and WC_DIR/A/B.

  # We expect update to only touch lambda and iota.
  expected_output = svntest.wc.State(wc_dir, {
    'A/B/lambda' : Item(status='U '),
    'iota' : Item(status='U '),
    })

  # Disk state should have all the files except iota and lambda
  # carrying new text.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/B/E/alpha',
                      contents="This is the file 'alpha'.\nAdded line.\n")
  expected_disk.tweak('A/B/E/beta',
                      contents="This is the file 'beta'.\nAdded line.\n")
  expected_disk.tweak('A/D/gamma',
                      contents="This is the file 'gamma'.\nAdded line.\n")
  expected_disk.tweak('A/D/H/omega',
                      contents="This is the file 'omega'.\nAdded line.\n")
  expected_disk.tweak('A/mu',
                      contents="This is the file 'mu'.\nAdded line.\n")
  expected_disk.tweak('A/D/G/tau',
                      contents="This is the file 'tau'.\nAdded line.\n")
  expected_disk.tweak('A/D/G/pi',
                      contents="This is the file 'pi'.\nAdded line.\n")
  expected_disk.tweak('A/D/H/chi',
                      contents="This is the file 'chi'.\nAdded line.\n")
  expected_disk.tweak('A/D/H/psi',
                      contents="This is the file 'psi'.\nAdded line.\n")
  expected_disk.tweak('A/D/G/rho',
                      contents="This is the file 'rho'.\nAdded line.\n")

  # Status is clean, but with iota and lambda at r1 and all else at r2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('iota', 'A/B/lambda', wc_rev=1)

  # Update.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        "-r", "1",
                                        "--changelist", "a",
                                        "--changelist", "i",
                                        "--depth", "files",
                                        wc_dir,
                                        os.path.join(wc_dir, 'A', 'B'))

  ### Backdate to depth infinity all changelists "a", "i", and "o" now.

  # We expect update to only touch all the files ending in 'a', 'i',
  # and 'o' (except lambda and iota which were previously updated).
  expected_output = svntest.wc.State(wc_dir, {
    'A/D/G/pi' : Item(status='U '),
    'A/D/H/chi' : Item(status='U '),
    'A/D/H/psi' : Item(status='U '),
    'A/D/G/rho' : Item(status='U '),
    'A/B/E/alpha' : Item(status='U '),
    'A/B/E/beta' : Item(status='U '),
    'A/D/gamma' : Item(status='U '),
    'A/D/H/omega' : Item(status='U '),
    })

  # Disk state should have only tau and mu carrying new text.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu',
                      contents="This is the file 'mu'.\nAdded line.\n")
  expected_disk.tweak('A/D/G/tau',
                      contents="This is the file 'tau'.\nAdded line.\n")

  # Status is clean, but with iota and lambda at r1 and all else at r2.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('iota', 'A/B/lambda', 'A/D/G/pi', 'A/D/H/chi',
                        'A/D/H/psi', 'A/D/G/rho', 'A/B/E/alpha',
                        'A/B/E/beta', 'A/D/gamma', 'A/D/H/omega', wc_rev=1)

  # Update.
  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None,
                                        None, None, 1,
                                        "-r", "1",
                                        "--changelist", "a",
                                        "--changelist", "i",
                                        "--changelist", "o",
                                        "--depth", "infinity",
                                        wc_dir)

def tree_conflicts_and_changelists_on_commit1(sbox):
  "tree conflicts, changelists and commit"
  svntest.actions.build_greek_tree_conflicts(sbox)
  wc_dir = sbox.wc_dir

  iota = os.path.join(wc_dir, "iota")
  rho = os.path.join(wc_dir, "A", "D", "G", "rho")

  # This file will ultimately be committed
  svntest.main.file_append(iota, "More stuff in iota")

  # Verify that the commit is blocked when we include a tree-conflicted
  # item.
  svntest.main.run_svn(None, "changelist", "list", iota, rho)

  expected_error = ("svn: E155015: Aborting commit: '.*" + re.escape(rho)
                    + "' remains in .*conflict")

  svntest.actions.run_and_verify_commit(wc_dir,
                                        None, None,
                                        expected_error,
                                        wc_dir,
                                        "--changelist",
                                        "list")

  # Now, test if we can commit iota without those tree-conflicts
  # getting in the way.
  svntest.main.run_svn(None, "changelist", "--remove", rho)

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/D/G/pi', status='D ', treeconflict='C')
  expected_status.tweak('A/D/G/tau', status='! ', treeconflict='C',
                        wc_rev=None)
  expected_status.tweak('A/D/G/rho', status='A ', copied='+',
                        treeconflict='C', wc_rev='-')
  expected_status.tweak('iota', wc_rev=3, status='  ')
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir,
                                        "--changelist",
                                        "list")


def tree_conflicts_and_changelists_on_commit2(sbox):
  "more tree conflicts, changelists and commit"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota = os.path.join(wc_dir, "iota")
  A = os.path.join(wc_dir, "A",)
  C = os.path.join(A, "C")

  # Make a tree-conflict on A/C:
  # Remove it, warp back, add a prop, update.
  svntest.main.run_svn(None, 'delete', C)

  expected_output = svntest.verify.UnorderedRegexOutput(
                                     ["Deleting.*" + re.escape(C)],
                                     False)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'commit', '-m', 'delete A/C', C)

  expected_output = svntest.verify.UnorderedRegexOutput(
                                     "A.*" + re.escape(C), False)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'update', C, "-r1")

  expected_output = svntest.verify.UnorderedRegexOutput(
                                     ".*'propname' set on '"
                                     + re.escape(C) + "'", False)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propset', 'propname', 'propval', C)

  expected_output = svntest.verify.UnorderedRegexOutput(
                                     "   C " + re.escape(C), False)
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'update', wc_dir)


  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('A/C', status='A ', copied='+',
                        treeconflict='C', wc_rev='-')

  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # So far so good. We have a tree-conflict on an absent dir A/C.

  # Verify that the current situation does not commit.
  expected_error = "svn: E155015: Aborting commit:.* remains in .*conflict";

  svntest.actions.run_and_verify_commit(wc_dir,
                                        None, None,
                                        expected_error,
                                        wc_dir)

  # Now try to commit with a changelist, not letting the
  # tree-conflict get in the way.
  svntest.main.file_append(iota, "More stuff in iota")
  svntest.main.run_svn(None, "changelist", "list", iota)

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status.tweak('iota', wc_rev=3, status='  ')

  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None,
                                        wc_dir,
                                        "--changelist",
                                        "list")


#----------------------------------------------------------------------

def move_keeps_changelist(sbox):
  "'svn mv' of existing keeps the changelist"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  iota_path  = os.path.join(wc_dir, 'iota')
  iota2_path = iota_path + '2'

  # 'svn mv' of existing file should *copy* the changelist to the new place
  svntest.main.run_svn(None, "changelist", 'foo', iota_path)
  svntest.main.run_svn(None, "rename", iota_path, iota2_path)
  expected_infos = [
    {
      'Name' : 'iota',
      'Schedule' : 'delete',
      'Changelist' : 'foo',
    },
    {
      'Name' : 'iota2',
      'Schedule' : 'add',
      'Changelist' : 'foo',  # this line fails the test
    },
  ]
  svntest.actions.run_and_verify_info(expected_infos, iota_path, iota2_path)

def move_added_keeps_changelist(sbox):
  "'svn mv' of added keeps the changelist"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  kappa_path  = os.path.join(wc_dir, 'kappa')
  kappa2_path = kappa_path + '2'

  # add 'kappa' (do not commit!)
  svntest.main.file_write(kappa_path, "This is the file 'kappa'.\n")
  svntest.main.run_svn(None, 'add', kappa_path)

  # 'svn mv' of added file should *move* the changelist to the new place
  svntest.main.run_svn(None, "changelist", 'foo', kappa_path)
  svntest.main.run_svn(None, "rename", kappa_path, kappa2_path)

  # kappa not under version control
  svntest.actions.run_and_verify_svnversion(None, kappa_path, repo_url,
                                            [], ".*doesn't exist.*")
  # kappa2 in a changelist
  expected_infos = [
    {
      'Name' : 'kappa2',
      'Schedule' : 'add',
      'Changelist' : 'foo',  # this line fails the test
    },
  ]
  svntest.actions.run_and_verify_info(expected_infos, kappa2_path)

@Issue(3820)
def change_to_dir(sbox):
  "change file in changelist to dir"

  sbox.build()

  # No changelist initially
  expected_infos = [{'Name' : 'mu', 'Changelist' : None}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu visible in changelist
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'changelist', 'qq', sbox.ospath('A/mu'))
  expected_infos = [{'Name' : 'mu', 'Changelist' : 'qq'}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu still visible after delete
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', sbox.ospath('A/mu'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu removed from changelist after replace with directory
  svntest.actions.run_and_verify_svn(None, '^A|' + _re_cl_rem_pattern, [],
                                     'mkdir', sbox.ospath('A/mu'))
  expected_infos = [{'Changelist' : None}] # No Name for directories?
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  svntest.main.run_svn(None, "commit", "-m", "r2: replace A/mu: file->dir",
                       sbox.ospath('A'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  svntest.main.run_svn(None, "update", "-r", "1", sbox.ospath('A'))
  expected_infos = [{'Name' : 'mu', 'Changelist' : None}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu visible in changelist
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'changelist', 'qq', sbox.ospath('A/mu'))
  expected_infos = [{'Name' : 'mu', 'Changelist' : 'qq'}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu removed from changelist after replace with dir via merge
  svntest.main.run_svn(None, "merge", "-c", "2", sbox.ospath('A'),
                       sbox.ospath('A'))
  expected_infos = [{'Changelist' : None}] # No Name for directories?
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))


@Issue(3822)
def revert_deleted_in_changelist(sbox):
  "revert a deleted file in a changelist"

  sbox.build(read_only = True)

  # No changelist initially
  expected_infos = [{'Name' : 'mu', 'Changelist' : None}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu visible in changelist
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'changelist', 'qq', sbox.ospath('A/mu'))
  expected_infos = [{'Name' : 'mu', 'Changelist' : 'qq'}]
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu still visible after delete
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', sbox.ospath('A/mu'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu still visible after revert
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', sbox.ospath('A/mu'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu still visible after parent delete
  svntest.actions.run_and_verify_svn(None, None, [], 'rm', sbox.ospath('A'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

  # A/mu still visible after revert
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'revert', '-R', sbox.ospath('A'))
  svntest.actions.run_and_verify_info(expected_infos, sbox.ospath('A/mu'))

def add_remove_non_existent_target(sbox):
  "add and remove non-existent target to changelist"

  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  bogus_path = os.path.join(wc_dir, 'A', 'bogus')

  expected_err = "svn: warning: W155010: The node '" + \
                 re.escape(os.path.abspath(bogus_path)) + \
                 "' was not found"

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'changelist', 'testlist',
                                     bogus_path)

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'changelist', bogus_path,
                                      '--remove')

def add_remove_unversioned_target(sbox):
  "add and remove unversioned target to changelist"

  sbox.build(read_only = True)
  unversioned = sbox.ospath('unversioned')
  svntest.main.file_write(unversioned, "dummy contents", 'w+')

  expected_err = "svn: warning: W155010: The node '" + \
                 re.escape(os.path.abspath(unversioned)) + \
                 "' was not found"

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'changelist', 'testlist',
                                     unversioned)

  svntest.actions.run_and_verify_svn(None, None, expected_err,
                                     'changelist', unversioned,
                                      '--remove')

@Issue(3985)
def readd_after_revert(sbox):
  "add new file to changelist, revert and readd"
  sbox.build(read_only = True)

  dummy = sbox.ospath('dummy')
  svntest.main.file_write(dummy, "dummy contents")

  sbox.simple_add('dummy')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'changelist', 'testlist',
                                     dummy)

  sbox.simple_revert('dummy')

  svntest.main.file_write(dummy, "dummy contents")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', dummy)


########################################################################
# Run the tests

# list all tests here, starting with None:
test_list = [ None,
              add_remove_changelists,
              commit_one_changelist,
              commit_multiple_changelists,
              info_with_changelists,
              diff_with_changelists,
              propmods_with_changelists,
              revert_with_changelists,
              update_with_changelists,
              tree_conflicts_and_changelists_on_commit1,
              tree_conflicts_and_changelists_on_commit2,
              move_keeps_changelist,
              move_added_keeps_changelist,
              change_to_dir,
              revert_deleted_in_changelist,
              add_remove_non_existent_target,
              add_remove_unversioned_target,
              readd_after_revert,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
