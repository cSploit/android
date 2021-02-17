#!/usr/bin/env python
#  -*- coding: utf-8 -*-
#
#  diff_tests.py:  some basic diff tests
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
import sys, re, os, time, shutil

# Our testing module
import svntest
from svntest import err

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem


######################################################################
# Generate expected output

def make_diff_header(path, old_tag, new_tag):
  """Generate the expected diff header for file PATH, with its old and new
  versions described in parentheses by OLD_TAG and NEW_TAG. Return the header
  as an array of newline-terminated strings."""
  path_as_shown = path.replace('\\', '/')
  return [
    "Index: " + path_as_shown + "\n",
    "===================================================================\n",
    "--- " + path_as_shown + "\t(" + old_tag + ")\n",
    "+++ " + path_as_shown + "\t(" + new_tag + ")\n",
    ]

def make_no_diff_deleted_header(path, old_tag, new_tag):
  """Generate the expected diff header for a deleted file PATH when in
  'no-diff-deleted' mode. (In that mode, no further details appear after the
  header.) Return the header as an array of newline-terminated strings."""
  path_as_shown = path.replace('\\', '/')
  return [
    "Index: " + path_as_shown + " (deleted)\n",
    "===================================================================\n",
    ]

def make_git_diff_header(target_path, repos_relpath,
                         old_tag, new_tag, add=False, src_label=None,
                         dst_label=None, delete=False, text_changes=True,
                         cp=False, mv=False, copyfrom_path=None):
  """ Generate the expected 'git diff' header for file TARGET_PATH.
  REPOS_RELPATH is the location of the path relative to the repository root.
  The old and new versions ("revision X", or "working copy") must be
  specified in OLD_TAG and NEW_TAG.
  SRC_LABEL and DST_LABEL are paths or urls that are added to the diff
  labels if we're diffing against the repository. ADD, DELETE, CP and MV
  denotes the operations performed on the file. COPYFROM_PATH is the source
  of a copy or move.  Return the header as an array of newline-terminated
  strings."""

  path_as_shown = target_path.replace('\\', '/')
  if src_label:
    src_label = src_label.replace('\\', '/')
    src_label = '\t(.../' + src_label + ')'
  else:
    src_label = ''
  if dst_label:
    dst_label = dst_label.replace('\\', '/')
    dst_label = '\t(.../' + dst_label + ')'
  else:
    dst_label = ''

  if add:
    output = [
      "Index: " + path_as_shown + "\n",
      "===================================================================\n",
      "diff --git a/" + repos_relpath + " b/" + repos_relpath + "\n",
      "new file mode 10644\n",
    ]
    if text_changes:
      output.extend([
        "--- /dev/null\t(" + old_tag + ")\n",
        "+++ b/" + repos_relpath + dst_label + "\t(" + new_tag + ")\n"
      ])
  elif delete:
    output = [
      "Index: " + path_as_shown + "\n",
      "===================================================================\n",
      "diff --git a/" + repos_relpath + " b/" + repos_relpath + "\n",
      "deleted file mode 10644\n",
    ]
    if text_changes:
      output.extend([
        "--- a/" + repos_relpath + src_label + "\t(" + old_tag + ")\n",
        "+++ /dev/null\t(" + new_tag + ")\n"
      ])
  elif cp:
    output = [
      "Index: " + path_as_shown + "\n",
      "===================================================================\n",
      "diff --git a/" + copyfrom_path + " b/" + repos_relpath + "\n",
      "copy from " + copyfrom_path + "\n",
      "copy to " + repos_relpath + "\n",
    ]
    if text_changes:
      output.extend([
        "--- a/" + copyfrom_path + src_label + "\t(" + old_tag + ")\n",
        "+++ b/" + repos_relpath + "\t(" + new_tag + ")\n"
      ])
  elif mv:
    return [
      "Index: " + path_as_shown + "\n",
      "===================================================================\n",
      "diff --git a/" + copyfrom_path + " b/" + path_as_shown + "\n",
      "rename from " + copyfrom_path + "\n",
      "rename to " + repos_relpath + "\n",
    ]
    if text_changes:
      output.extend([
        "--- a/" + copyfrom_path + src_label + "\t(" + old_tag + ")\n",
        "+++ b/" + repos_relpath + "\t(" + new_tag + ")\n"
      ])
  else:
    output = [
      "Index: " + path_as_shown + "\n",
      "===================================================================\n",
      "diff --git a/" + repos_relpath + " b/" + repos_relpath + "\n",
      "--- a/" + repos_relpath + src_label + "\t(" + old_tag + ")\n",
      "+++ b/" + repos_relpath + dst_label + "\t(" + new_tag + ")\n",
    ]
  return output

def make_diff_prop_header(path):
  """Return a property diff sub-header, as a list of newline-terminated
     strings."""
  return [
    "\n",
    "Property changes on: " + path.replace('\\', '/') + "\n",
    "___________________________________________________________________\n"
  ]

def make_diff_prop_val(plus_minus, pval):
  "Return diff for prop value PVAL, with leading PLUS_MINUS (+ or -)."
  if len(pval) > 0 and pval[-1] != '\n':
    return [plus_minus + pval + "\n","\\ No newline at end of property\n"]
  return [plus_minus + pval]
  
def make_diff_prop_deleted(pname, pval):
  """Return a property diff for deletion of property PNAME, old value PVAL.
     PVAL is a single string with no embedded newlines.  Return the result
     as a list of newline-terminated strings."""
  return [
    "Deleted: " + pname + "\n",
    "## -1 +0,0 ##\n"
  ] + make_diff_prop_val("-", pval)

def make_diff_prop_added(pname, pval):
  """Return a property diff for addition of property PNAME, new value PVAL.
     PVAL is a single string with no embedded newlines.  Return the result
     as a list of newline-terminated strings."""
  return [
    "Added: " + pname + "\n",
    "## -0,0 +1 ##\n",
  ] + make_diff_prop_val("+", pval)

def make_diff_prop_modified(pname, pval1, pval2):
  """Return a property diff for modification of property PNAME, old value
     PVAL1, new value PVAL2.  PVAL is a single string with no embedded
     newlines.  Return the result as a list of newline-terminated strings."""
  return [
    "Modified: " + pname + "\n",
    "## -1 +1 ##\n",
  ] + make_diff_prop_val("-", pval1) + make_diff_prop_val("+", pval2)

######################################################################
# Diff output checker
#
# Looks for the correct filenames and a suitable number of +/- lines
# depending on whether this is an addition, modification or deletion.

def check_diff_output(diff_output, name, diff_type):
  "check diff output"

# On Windows, diffs still display / rather than \ in paths
  if svntest.main.windows == 1:
    name = name.replace('\\', '/')
  i_re = re.compile('^Index:')
  d_re = re.compile('^Index: (\\./)?' + name)
  p_re = re.compile('^--- (\\./)?' + name)
  add_re = re.compile('^\\+')
  sub_re = re.compile('^-')

  i = 0
  while i < len(diff_output) - 4:

    # identify a possible diff
    if (d_re.match(diff_output[i])
        and p_re.match(diff_output[i+2])):

      # count lines added and deleted
      i += 4
      add_lines = 0
      sub_lines = 0
      while i < len(diff_output) and not i_re.match(diff_output[i]):
        if add_re.match(diff_output[i][0]):
          add_lines += 1
        if sub_re.match(diff_output[i][0]):
          sub_lines += 1
        i += 1

      #print "add:", add_lines
      #print "sub:", sub_lines
      # check if this looks like the right sort of diff
      if add_lines > 0 and sub_lines == 0 and diff_type == 'A':
        return 0
      if sub_lines > 0 and add_lines == 0 and diff_type == 'D':
        return 0
      if add_lines > 0 and sub_lines > 0 and diff_type == 'M':
        return 0

    else:
      i += 1

  # no suitable diff found
  return 1

def count_diff_output(diff_output):
  "count the number of file diffs in the output"

  i_re = re.compile('Index:')
  diff_count = 0
  i = 0
  while i < len(diff_output) - 4:
    if i_re.match(diff_output[i]):
      i += 4
      diff_count += 1
    else:
      i += 1

  return diff_count

def verify_expected_output(diff_output, expected):
  "verify given line exists in diff output"
  for line in diff_output:
    if line.find(expected) != -1:
      break
  else:
    raise svntest.Failure

def verify_excluded_output(diff_output, excluded):
  "verify given line does not exist in diff output as diff line"
  for line in diff_output:
    if re.match("^(\\+|-)%s" % re.escape(excluded), line):
      print('Sought: %s' % excluded)
      print('Found:  %s' % line)
      raise svntest.Failure

def extract_diff_path(line):
  l2 = line[(line.find("(")+1):]
  l3 = l2[0:(l2.find(")"))]
  return l3

######################################################################
# diff on a repository subset and check the output

def diff_check_repo_subset(wc_dir, repo_subset, check_fn, do_diff_r):
  "diff and check for part of the repository"

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            repo_subset)
  if check_fn(diff_output):
    return 1

  if do_diff_r:
    exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                              '-r', 'HEAD',
                                                              repo_subset)
    if check_fn(diff_output):
      return 1

  os.chdir(was_cwd)

  return 0

######################################################################
# Changes makers and change checkers

def update_a_file():
  "update a file"
  svntest.main.file_write(os.path.join('A', 'B', 'E', 'alpha'), "new atext")
  # svntest.main.file_append(, "new atext")
  return 0

def check_update_a_file(diff_output):
  "check diff for update a file"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'E', 'alpha'),
                           'M')

def diff_check_update_a_file_repo_subset(wc_dir):
  "diff and check update a file for a repository subset"

  repo_subset = os.path.join('A', 'B')
  if diff_check_repo_subset(wc_dir, repo_subset, check_update_a_file, 1):
    return 1

  repo_subset = os.path.join('A', 'B', 'E', 'alpha')
  if diff_check_repo_subset(wc_dir, repo_subset, check_update_a_file, 1):
    return 1

  return 0


#----------------------------------------------------------------------

def add_a_file():
  "add a file"
  svntest.main.file_append(os.path.join('A', 'B', 'E', 'theta'), "theta")
  svntest.main.run_svn(None, 'add', os.path.join('A', 'B', 'E', 'theta'))
  return 0

def check_add_a_file(diff_output):
  "check diff for add a file"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'E', 'theta'),
                           'A')

def check_add_a_file_reverse(diff_output):
  "check diff for add a file"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'E', 'theta'),
                           'D')

def diff_check_add_a_file_repo_subset(wc_dir):
  "diff and check add a file for a repository subset"

  repo_subset = os.path.join('A', 'B')
  if diff_check_repo_subset(wc_dir, repo_subset, check_add_a_file, 1):
    return 1

  repo_subset = os.path.join('A', 'B', 'E', 'theta')
  ### TODO: diff -r HEAD doesn't work for added file
  if diff_check_repo_subset(wc_dir, repo_subset, check_add_a_file, 0):
    return 1

def update_added_file():
  svntest.main.file_append(os.path.join('A', 'B', 'E', 'theta'), "net ttext")
  "update added file"
  return 0

def check_update_added_file(diff_output):
  "check diff for update of added file"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'E', 'theta'),
                           'M')

#----------------------------------------------------------------------

def add_a_file_in_a_subdir():
  "add a file in a subdir"
  os.mkdir(os.path.join('A', 'B', 'T'))
  svntest.main.run_svn(None, 'add', os.path.join('A', 'B', 'T'))
  svntest.main.file_append(os.path.join('A', 'B', 'T', 'phi'), "phi")
  svntest.main.run_svn(None, 'add', os.path.join('A', 'B', 'T', 'phi'))
  return 0

def check_add_a_file_in_a_subdir(diff_output):
  "check diff for add a file in a subdir"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'T', 'phi'),
                           'A')

def check_add_a_file_in_a_subdir_reverse(diff_output):
  "check diff for add a file in a subdir"
  return check_diff_output(diff_output,
                           os.path.join('A', 'B', 'T', 'phi'),
                           'D')

def diff_check_add_a_file_in_a_subdir_repo_subset(wc_dir):
  "diff and check add a file in a subdir for a repository subset"

  repo_subset = os.path.join('A', 'B', 'T')
  ### TODO: diff -r HEAD doesn't work for added subdir
  if diff_check_repo_subset(wc_dir, repo_subset,
                            check_add_a_file_in_a_subdir, 0):
    return 1

  repo_subset = os.path.join('A', 'B', 'T', 'phi')
  ### TODO: diff -r HEAD doesn't work for added file in subdir
  if diff_check_repo_subset(wc_dir, repo_subset,
                            check_add_a_file_in_a_subdir, 0):
    return 1

#----------------------------------------------------------------------

def replace_a_file():
  "replace a file"
  svntest.main.run_svn(None, 'rm', os.path.join('A', 'D', 'G', 'rho'))
  svntest.main.file_append(os.path.join('A', 'D', 'G', 'rho'), "new rho")
  svntest.main.run_svn(None, 'add', os.path.join('A', 'D', 'G', 'rho'))
  return 0

def check_replace_a_file(diff_output):
  "check diff for replace a file"
  return check_diff_output(diff_output,
                       os.path.join('A', 'D', 'G', 'rho'),
                       'M')

#----------------------------------------------------------------------

def update_three_files():
  "update three files"
  svntest.main.file_write(os.path.join('A', 'D', 'gamma'), "new gamma")
  svntest.main.file_write(os.path.join('A', 'D', 'G', 'tau'), "new tau")
  svntest.main.file_write(os.path.join('A', 'D', 'H', 'psi'), "new psi")
  return 0

def check_update_three_files(diff_output):
  "check update three files"
  if check_diff_output(diff_output,
                        os.path.join('A', 'D', 'gamma'),
                        'M'):
    return 1
  if check_diff_output(diff_output,
                        os.path.join('A', 'D', 'G', 'tau'),
                        'M'):
    return 1
  if check_diff_output(diff_output,
                        os.path.join('A', 'D', 'H', 'psi'),
                        'M'):
    return 1
  return 0


######################################################################
# make a change, check the diff, commit the change, check the diff

def change_diff_commit_diff(wc_dir, revision, change_fn, check_fn):
  "make a change, diff, commit, update and diff again"

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  svntest.main.run_svn(None,
                       'up', '-r', 'HEAD')

  change_fn()

  # diff without revision doesn't use an editor
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff')
  if check_fn(diff_output):
    raise svntest.Failure

  # diff with revision runs an editor
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', 'HEAD')
  if check_fn(diff_output):
    raise svntest.Failure

  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')
  svntest.main.run_svn(None,
                       'up')
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', revision)
  if check_fn(diff_output):
    raise svntest.Failure

  os.chdir(was_cwd)

######################################################################
# check the diff

def just_diff(wc_dir, rev_check, check_fn):
  "update and check that the given diff is seen"

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', rev_check)
  if check_fn(diff_output):
    raise svntest.Failure
  os.chdir(was_cwd)

######################################################################
# update, check the diff

def update_diff(wc_dir, rev_up, rev_check, check_fn):
  "update and check that the given diff is seen"

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  svntest.main.run_svn(None,
                       'up', '-r', rev_up)

  os.chdir(was_cwd)

  just_diff(wc_dir, rev_check, check_fn)

######################################################################
# check a pure repository rev1:rev2 diff

def repo_diff(wc_dir, rev1, rev2, check_fn):
  "check that the given pure repository diff is seen"

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  exit_code, diff_output, err_output = svntest.main.run_svn(None,
                                                            'diff', '-r',
                                                            repr(rev2) + ':'
                                                                   + repr(rev1))
  if check_fn(diff_output):
    raise svntest.Failure

  os.chdir(was_cwd)

######################################################################
# Tests
#

# test 1
def diff_update_a_file(sbox):
  "update a file"

  sbox.build()

  change_diff_commit_diff(sbox.wc_dir, 1,
                          update_a_file,
                          check_update_a_file)

# test 2
def diff_add_a_file(sbox):
  "add a file"

  sbox.build()

  change_diff_commit_diff(sbox.wc_dir, 1,
                          add_a_file,
                          check_add_a_file)

#test 3
def diff_add_a_file_in_a_subdir(sbox):
  "add a file in an added directory"

  sbox.build()

  change_diff_commit_diff(sbox.wc_dir, 1,
                          add_a_file_in_a_subdir,
                          check_add_a_file_in_a_subdir)

# test 4
def diff_replace_a_file(sbox):
  "replace a file with a file"

  sbox.build()

  change_diff_commit_diff(sbox.wc_dir, 1,
                          replace_a_file,
                          check_replace_a_file)

# test 5
def diff_multiple_reverse(sbox):
  "multiple revisions diff'd forwards and backwards"

  sbox.build()
  wc_dir = sbox.wc_dir

  # rev 2
  change_diff_commit_diff(wc_dir, 1,
                          add_a_file,
                          check_add_a_file)

  #rev 3
  change_diff_commit_diff(wc_dir, 2,
                          add_a_file_in_a_subdir,
                          check_add_a_file_in_a_subdir)

  #rev 4
  change_diff_commit_diff(wc_dir, 3,
                          update_a_file,
                          check_update_a_file)

  # check diffs both ways
  update_diff(wc_dir, 4, 1, check_update_a_file)
  just_diff(wc_dir, 1, check_add_a_file_in_a_subdir)
  just_diff(wc_dir, 1, check_add_a_file)
  update_diff(wc_dir, 1, 4, check_update_a_file)
  just_diff(wc_dir, 4, check_add_a_file_in_a_subdir_reverse)
  just_diff(wc_dir, 4, check_add_a_file_reverse)

  # check pure repository diffs
  repo_diff(wc_dir, 4, 1, check_update_a_file)
  repo_diff(wc_dir, 4, 1, check_add_a_file_in_a_subdir)
  repo_diff(wc_dir, 4, 1, check_add_a_file)
  repo_diff(wc_dir, 1, 4, check_update_a_file)
  repo_diff(wc_dir, 1, 4, check_add_a_file_in_a_subdir_reverse)
  repo_diff(wc_dir, 1, 4, check_add_a_file_reverse)

# test 6
def diff_non_recursive(sbox):
  "non-recursive behaviour"

  sbox.build()
  wc_dir = sbox.wc_dir

  change_diff_commit_diff(wc_dir, 1,
                          update_three_files,
                          check_update_three_files)

  # The changes are in:   ./A/D/gamma
  #                       ./A/D/G/tau
  #                       ./A/D/H/psi
  # When checking D recursively there are three changes. When checking
  # D non-recursively there is only one change. When checking G
  # recursively, there is only one change even though D is the anchor

  # full diff has three changes
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', os.path.join(wc_dir, 'A', 'D'))

  if count_diff_output(diff_output) != 3:
    raise svntest.Failure

  # non-recursive has one change
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', '-N', os.path.join(wc_dir, 'A', 'D'))

  if count_diff_output(diff_output) != 1:
    raise svntest.Failure

  # diffing a directory doesn't pick up other diffs in the anchor
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', os.path.join(wc_dir, 'A', 'D', 'G'))

  if count_diff_output(diff_output) != 1:
    raise svntest.Failure


# test 7
def diff_repo_subset(sbox):
  "diff only part of the repository"

  sbox.build()
  wc_dir = sbox.wc_dir

  was_cwd = os.getcwd()
  os.chdir(wc_dir)

  update_a_file()
  add_a_file()
  add_a_file_in_a_subdir()

  os.chdir(was_cwd)

  if diff_check_update_a_file_repo_subset(wc_dir):
    raise svntest.Failure

  if diff_check_add_a_file_repo_subset(wc_dir):
    raise svntest.Failure

  if diff_check_add_a_file_in_a_subdir_repo_subset(wc_dir):
    raise svntest.Failure


# test 8
def diff_non_version_controlled_file(sbox):
  "non version controlled files"

  sbox.build()
  wc_dir = sbox.wc_dir

  svntest.main.file_append(os.path.join(wc_dir, 'A', 'D', 'foo'), "a new file")

  exit_code, diff_output, err_output = svntest.main.run_svn(
    1, 'diff', os.path.join(wc_dir, 'A', 'D', 'foo'))

  if count_diff_output(diff_output) != 0: raise svntest.Failure

  # At one point this would crash, so we would only get a 'Segmentation Fault'
  # error message.  The appropriate response is a few lines of errors.  I wish
  # there was a way to figure out if svn crashed, but all run_svn gives us is
  # the output, so here we are...
  for line in err_output:
    if re.search("foo' is not under version control$", line):
      break
  else:
    raise svntest.Failure

# test 9
def diff_pure_repository_update_a_file(sbox):
  "pure repository diff update a file"

  sbox.build()
  wc_dir = sbox.wc_dir

  os.chdir(wc_dir)

  # rev 2
  update_a_file()
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')

  # rev 3
  add_a_file_in_a_subdir()
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')

  # rev 4
  add_a_file()
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')

  # rev 5
  update_added_file()
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')

  svntest.main.run_svn(None,
                       'up', '-r', '2')

  url = sbox.repo_url

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-c', '2', url)
  if check_update_a_file(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2')
  if check_update_a_file(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-c', '3', url)
  if check_add_a_file_in_a_subdir(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '2:3')
  if check_add_a_file_in_a_subdir(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-c', '5', url)
  if check_update_added_file(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '4:5')
  if check_update_added_file(diff_output): raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', 'head')
  if check_add_a_file_in_a_subdir_reverse(diff_output): raise svntest.Failure


# test 10
def diff_only_property_change(sbox):
  "diff when property was changed but text was not"

  sbox.build()
  wc_dir = sbox.wc_dir

  expected_output = \
    make_diff_header("iota", "revision 1", "revision 2") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_added("svn:eol-style", "native")

  expected_reverse_output = \
    make_diff_header("iota", "revision 2", "revision 1") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_deleted("svn:eol-style", "native")

  expected_rev1_output = \
    make_diff_header("iota", "revision 1", "working copy") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_added("svn:eol-style", "native")

  os.chdir(sbox.wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'svn:eol-style', 'native', 'iota')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'empty-msg')

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-r', '1:2')

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-c', '2')

  svntest.actions.run_and_verify_svn(None, expected_reverse_output, [],
                                     'diff', '-r', '2:1')

  svntest.actions.run_and_verify_svn(None, expected_reverse_output, [],
                                     'diff', '-c', '-2')

  svntest.actions.run_and_verify_svn(None, expected_rev1_output, [],
                                     'diff', '-r', '1')

  svntest.actions.run_and_verify_svn(None, expected_rev1_output, [],
                                     'diff', '-r', 'PREV', 'iota')



#----------------------------------------------------------------------
# Regression test for issue #1019: make sure we don't try to display
# diffs when the file is marked as a binary type.  This tests all 3
# uses of 'svn diff':  wc-wc, wc-repos, repos-repos.
@Issue(1019)
def dont_diff_binary_file(sbox):
  "don't diff file marked as binary type"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Add a binary file to the project.
  theta_contents = open(os.path.join(sys.path[0], "theta.bin"), 'rb').read()
  # Write PNG file data into 'A/theta'.
  theta_path = os.path.join(wc_dir, 'A', 'theta')
  svntest.main.file_write(theta_path, theta_contents, 'wb')

  svntest.main.run_svn(None, 'add', theta_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Adding  (bin)'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    })

  # Commit the new binary file, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Update the whole working copy to HEAD (rev 2)
  expected_output = svntest.wc.State(wc_dir, {})

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/theta' : Item(theta_contents,
                     props={'svn:mime-type' : 'application/octet-stream'}),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_update(wc_dir,
                                        expected_output,
                                        expected_disk,
                                        expected_status,
                                        None, None, None, None, None,
                                        1)  # verify props, too.

  # Make a local mod to the binary file.
  svntest.main.file_append(theta_path, "some extra junk")

  # First diff use-case: plain old 'svn diff wc' will display any
  # local changes in the working copy.  (diffing working
  # vs. text-base)

  re_nodisplay = re.compile('^Cannot display:')

  exit_code, stdout, stderr = svntest.main.run_svn(None, 'diff', wc_dir)

  for line in stdout:
    if (re_nodisplay.match(line)):
      break
  else:
    raise svntest.Failure

  # Second diff use-case: 'svn diff -r1 wc' compares the wc against a
  # the first revision in the repository.

  exit_code, stdout, stderr = svntest.main.run_svn(None,
                                                   'diff', '-r', '1', wc_dir)

  for line in stdout:
    if (re_nodisplay.match(line)):
      break
  else:
    raise svntest.Failure

  # Now commit the local mod, creating rev 3.
  expected_output = svntest.wc.State(wc_dir, {
    'A/theta' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.add({
    'A/theta' : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Third diff use-case: 'svn diff -r2:3 wc' will compare two
  # repository trees.

  exit_code, stdout, stderr = svntest.main.run_svn(None, 'diff',
                                                   '-r', '2:3', wc_dir)

  for line in stdout:
    if (re_nodisplay.match(line)):
      break
  else:
    raise svntest.Failure


def diff_nonextant_urls(sbox):
  "svn diff errors against a non-existent URL"

  sbox.build(create_wc = False)
  non_extant_url = sbox.repo_url + '/A/does_not_exist'
  extant_url = sbox.repo_url + '/A/mu'

  exit_code, diff_output, err_output = svntest.main.run_svn(
    1, 'diff', '--old', non_extant_url, '--new', extant_url)

  for line in err_output:
    if re.search('was not found in the repository at revision', line):
      break
  else:
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(
    1, 'diff', '--old', extant_url, '--new', non_extant_url)

  for line in err_output:
    if re.search('was not found in the repository at revision', line):
      break
  else:
    raise svntest.Failure

def diff_head_of_moved_file(sbox):
  "diff against the head of a moved file"

  sbox.build()
  mu_path = os.path.join(sbox.wc_dir, 'A', 'mu')
  new_mu_path = mu_path + '.new'

  svntest.main.run_svn(None, 'mv', mu_path, new_mu_path)

  # Modify the file to ensure that the diff is non-empty.
  svntest.main.file_append(new_mu_path, "\nActually, it's a new mu.")

  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'diff', '-r', 'HEAD', new_mu_path)



#----------------------------------------------------------------------
# Regression test for issue #977: make 'svn diff -r BASE:N' compare a
# repository tree against the wc's text-bases, rather than the wc's
# working files.  This is a long test, which checks many variations.
@Issue(977)
def diff_base_to_repos(sbox):
  "diff text-bases against repository"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(sbox.wc_dir, 'iota')
  newfile_path = os.path.join(sbox.wc_dir, 'A', 'D', 'newfile')
  mu_path = os.path.join(sbox.wc_dir, 'A', 'mu')

  # Make changes to iota, commit r2, update to HEAD (r2).
  svntest.main.file_append(iota_path, "some rev2 iota text.\n")

  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota',
                      contents=\
                      "This is the file 'iota'.\nsome rev2 iota text.\n")
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status)

  # Now make another local mod to iota.
  svntest.main.file_append(iota_path, "an iota local mod.\n")

  # If we run 'svn diff -r 1', we should see diffs that include *both*
  # the rev2 changes and local mods.  That's because the working files
  # are being compared to the repository.
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '1', wc_dir)

  # Makes diff output look the same on all platforms.
  def strip_eols(lines):
    return [x.replace("\r", "").replace("\n", "") for x in lines]

  expected_output_lines = make_diff_header(iota_path, "revision 1",
                                           "working copy") + [
    "@@ -1 +1,3 @@\n",
    " This is the file 'iota'.\n",
    "+some rev2 iota text.\n",
    "+an iota local mod.\n"]

  if strip_eols(diff_output) != strip_eols(expected_output_lines):
    raise svntest.Failure

  # If we run 'svn diff -r BASE:1', we should see diffs that only show
  # the rev2 changes and NOT the local mods.  That's because the
  # text-bases are being compared to the repository.
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'BASE:1', wc_dir)

  expected_output_lines = make_diff_header(iota_path, "working copy",
                                           "revision 1") + [
    "@@ -1,2 +1 @@\n",
    " This is the file 'iota'.\n",
    "-some rev2 iota text.\n"]

  if strip_eols(diff_output) != strip_eols(expected_output_lines):
    raise svntest.Failure

  # But that's not all folks... no, no, we're just getting started
  # here!  There are so many other tests to do.

  # For example, we just ran 'svn diff -rBASE:1'.  The output should
  # look exactly the same as 'svn diff -r2:1'.  (If you remove the
  # header commentary)
  exit_code, diff_output2, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '2:1', wc_dir)

  diff_output[2:4] = []
  diff_output2[2:4] = []

  if (diff_output2 != diff_output):
    raise svntest.Failure

  # and similarly, does 'svn diff -r1:2' == 'svn diff -r1:BASE' ?
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '1:2', wc_dir)

  exit_code, diff_output2, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '1:BASE', wc_dir)

  diff_output[2:4] = []
  diff_output2[2:4] = []

  if (diff_output2 != diff_output):
    raise svntest.Failure

  # Now we schedule an addition and a deletion.
  svntest.main.file_append(newfile_path, "Contents of newfile\n")
  svntest.main.run_svn(None, 'add', newfile_path)
  svntest.main.run_svn(None, 'rm', mu_path)

  expected_output = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_output.add({
    'A/D/newfile' : Item(status='A ', wc_rev=0),
    })
  expected_output.tweak('A/mu', status='D ')
  expected_output.tweak('iota', status='M ')
  svntest.actions.run_and_verify_status(wc_dir, expected_output)

  # once again, verify that -r1:2 and -r1:BASE look the same, as do
  # -r2:1 and -rBASE:1.  None of these diffs should mention the
  # scheduled addition or deletion.
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '1:2', wc_dir)

  exit_code, diff_output2, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '1:BASE', wc_dir)

  exit_code, diff_output3, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '2:1', wc_dir)

  exit_code, diff_output4, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'BASE:1', wc_dir)

  diff_output[2:4] = []
  diff_output2[2:4] = []
  diff_output3[2:4] = []
  diff_output4[2:4] = []

  if (diff_output != diff_output2):
    raise svntest.Failure

  if (diff_output3 != diff_output4):
    raise svntest.Failure

  # Great!  So far, so good.  Now we commit our three changes (a local
  # mod, an addition, a deletion) and update to HEAD (r3).
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    'A/mu' : Item(verb='Deleting'),
    'A/D/newfile' : Item(verb='Adding')
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('iota', wc_rev=3)
  expected_status.remove('A/mu')
  expected_status.add({
    'A/D/newfile' : Item(status='  ', wc_rev=3),
    })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota',
                      contents="This is the file 'iota'.\n" + \
                      "some rev2 iota text.\nan iota local mod.\n")
  expected_disk.add({'A/D/newfile' : Item("Contents of newfile\n")})
  expected_disk.remove('A/mu')

  expected_status = svntest.actions.get_virginal_state(wc_dir, 3)
  expected_status.remove('A/mu')
  expected_status.add({
    'A/D/newfile' : Item(status='  ', wc_rev=3),
    })
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status)

  # Now 'svn diff -r3:2' should == 'svn diff -rBASE:2', showing the
  # removal of changes to iota, the adding of mu, and deletion of newfile.
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', '3:2', wc_dir)

  exit_code, diff_output2, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'BASE:2', wc_dir)

  # to do the comparison, remove all output lines starting with +++ or ---
  re_infoline = re.compile('^(\+\+\+|---).*$')
  list1 = []
  list2 = []

  for line in diff_output:
    if not re_infoline.match(line):
      list1.append(line)

  for line in diff_output2:
    if not re_infoline.match(line):
      list2.append(line)

  # Two files in diff may be in any order.
  list1 = svntest.verify.UnorderedOutput(list1)

  svntest.verify.compare_and_display_lines('', '', list1, list2)


#----------------------------------------------------------------------
# This is a simple regression test for issue #891, whereby ra_neon's
# REPORT request would fail, because the object no longer exists in HEAD.
@Issue(891)
def diff_deleted_in_head(sbox):
  "repos-repos diff on item deleted from HEAD"

  sbox.build()
  wc_dir = sbox.wc_dir

  A_path = os.path.join(sbox.wc_dir, 'A')
  mu_path = os.path.join(sbox.wc_dir, 'A', 'mu')

  # Make a change to mu, commit r2, update.
  svntest.main.file_append(mu_path, "some rev2 mu text.\n")

  expected_output = svntest.wc.State(wc_dir, {
    'A/mu' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  expected_output = svntest.wc.State(wc_dir, {})
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu',
                      contents="This is the file 'mu'.\nsome rev2 mu text.\n")
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  svntest.actions.run_and_verify_update(wc_dir, expected_output,
                                        expected_disk, expected_status)

  # Now delete the whole directory 'A', and commit as r3.
  svntest.main.run_svn(None, 'rm', A_path)
  expected_output = svntest.wc.State(wc_dir, {
    'A' : Item(verb='Deleting'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.remove('A', 'A/B', 'A/B/E', 'A/B/E/beta', 'A/B/E/alpha',
                         'A/B/F', 'A/B/lambda', 'A/D', 'A/D/G', 'A/D/G/rho',
                         'A/D/G/pi', 'A/D/G/tau', 'A/D/H', 'A/D/H/psi',
                         'A/D/H/omega', 'A/D/H/chi', 'A/D/gamma', 'A/mu',
                         'A/C')

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Doing an 'svn diff -r1:2' on the URL of directory A should work,
  # especially over the DAV layer.
  the_url = sbox.repo_url + '/A'
  diff_output = svntest.actions.run_and_verify_svn(None, None, [],
                                                   'diff', '-r',
                                                   '1:2', the_url + "@2")


#----------------------------------------------------------------------
def diff_targets(sbox):
  "select diff targets"

  sbox.build()
  os.chdir(sbox.wc_dir)

  update_a_file()
  add_a_file()

  update_path = os.path.join('A', 'B', 'E', 'alpha')
  add_path = os.path.join('A', 'B', 'E', 'theta')
  parent_path = os.path.join('A', 'B', 'E')
  update_url = sbox.repo_url + '/A/B/E/alpha'
  parent_url = sbox.repo_url + '/A/B/E'

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            update_path,
                                                            add_path)
  if check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            update_path)
  if check_update_a_file(diff_output) or not check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '--old', parent_path, 'alpha', 'theta')

  if check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '--old', parent_path, 'theta')

  if not check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'ci',
                                                            '-m', 'log msg')

  exit_code, diff_output, err_output = svntest.main.run_svn(1, 'diff', '-r1:2',
                                                            update_path,
                                                            add_path)

  if check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(1,
                                                            'diff', '-r1:2',
                                                            add_path)

  if not check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(
    1, 'diff', '-r1:2', '--old', parent_path, 'alpha', 'theta')

  if check_update_a_file(diff_output) or check_add_a_file(diff_output):
    raise svntest.Failure

  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r1:2', '--old', parent_path, 'alpha')

  if check_update_a_file(diff_output) or not check_add_a_file(diff_output):
    raise svntest.Failure


#----------------------------------------------------------------------
def diff_branches(sbox):
  "diff for branches"

  sbox.build()

  A_url = sbox.repo_url + '/A'
  A2_url = sbox.repo_url + '/A2'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg',
                                     A_url, A2_url)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', sbox.wc_dir)

  A_alpha = os.path.join(sbox.wc_dir, 'A', 'B', 'E', 'alpha')
  A2_alpha = os.path.join(sbox.wc_dir, 'A2', 'B', 'E', 'alpha')

  svntest.main.file_append(A_alpha, "\nfoo\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)

  svntest.main.file_append(A2_alpha, "\nbar\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)

  svntest.main.file_append(A_alpha, "zig\n")

  # Compare repository file on one branch against repository file on
  # another branch
  rel_path = os.path.join('B', 'E', 'alpha')
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '--old', A_url, '--new', A2_url, rel_path)

  verify_expected_output(diff_output, "-foo")
  verify_expected_output(diff_output, "+bar")

  # Same again but using whole branch
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '--old', A_url, '--new', A2_url)

  verify_expected_output(diff_output, "-foo")
  verify_expected_output(diff_output, "+bar")

  # Compare two repository files on different branches
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [],
    'diff', A_url + '/B/E/alpha', A2_url + '/B/E/alpha')

  verify_expected_output(diff_output, "-foo")
  verify_expected_output(diff_output, "+bar")

  # Compare two versions of a file on a single branch
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [],
    'diff', A_url + '/B/E/alpha@2', A_url + '/B/E/alpha@3')

  verify_expected_output(diff_output, "+foo")

  # Compare identical files on different branches
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, [], [],
    'diff', A_url + '/B/E/alpha@2', A2_url + '/B/E/alpha@3')


#----------------------------------------------------------------------
def diff_repos_and_wc(sbox):
  "diff between repos URLs and WC paths"

  sbox.build()

  A_url = sbox.repo_url + '/A'
  A2_url = sbox.repo_url + '/A2'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg',
                                     A_url, A2_url)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', sbox.wc_dir)

  A_alpha = os.path.join(sbox.wc_dir, 'A', 'B', 'E', 'alpha')
  A2_alpha = os.path.join(sbox.wc_dir, 'A2', 'B', 'E', 'alpha')

  svntest.main.file_append(A_alpha, "\nfoo\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)

  svntest.main.file_append(A2_alpha, "\nbar\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)

  svntest.main.file_append(A_alpha, "zig\n")

  # Compare working file on one branch against repository file on
  # another branch
  A_path = os.path.join(sbox.wc_dir, 'A')
  rel_path = os.path.join('B', 'E', 'alpha')
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [],
    'diff', '--old', A2_url, '--new', A_path, rel_path)

  verify_expected_output(diff_output, "-bar")
  verify_expected_output(diff_output, "+foo")
  verify_expected_output(diff_output, "+zig")

  # Same again but using whole branch
  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [],
    'diff', '--old', A2_url, '--new', A_path)

  verify_expected_output(diff_output, "-bar")
  verify_expected_output(diff_output, "+foo")
  verify_expected_output(diff_output, "+zig")

#----------------------------------------------------------------------
@Issue(1311)
def diff_file_urls(sbox):
  "diff between two file URLs"

  sbox.build()

  iota_path = os.path.join(sbox.wc_dir, 'iota')
  iota_url = sbox.repo_url + '/iota'
  iota_copy_path = os.path.join(sbox.wc_dir, 'A', 'iota')
  iota_copy_url = sbox.repo_url + '/A/iota'
  iota_copy2_url = sbox.repo_url + '/A/iota2'

  # Put some different text into iota, and commit.
  os.remove(iota_path)
  svntest.main.file_append(iota_path, "foo\nbar\nsnafu\n")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', iota_path)

  # Now, copy the file elsewhere, twice.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg',
                                     iota_url, iota_copy_url)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg',
                                     iota_url, iota_copy2_url)

  # Update (to get the copies)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', sbox.wc_dir)

  # Now, make edits to one of the copies of iota, and commit.
  os.remove(iota_copy_path)
  svntest.main.file_append(iota_copy_path, "foo\nsnafu\nabcdefg\nopqrstuv\n")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', iota_copy_path)

  # Finally, do a diff between the first and second copies of iota,
  # and verify that we got the expected lines.  And then do it in reverse!
  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff',
                                                           iota_copy_url,
                                                           iota_copy2_url)

  verify_expected_output(out, "+bar")
  verify_expected_output(out, "-abcdefg")
  verify_expected_output(out, "-opqrstuv")

  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff',
                                                           iota_copy2_url,
                                                           iota_copy_url)

  verify_expected_output(out, "-bar")
  verify_expected_output(out, "+abcdefg")
  verify_expected_output(out, "+opqrstuv")

#----------------------------------------------------------------------
def diff_prop_change_local_edit(sbox):
  "diff a property change plus a local edit"

  sbox.build()

  iota_path = os.path.join(sbox.wc_dir, 'iota')
  iota_url = sbox.repo_url + '/iota'

  # Change a property on iota, and commit.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'pname', 'pvalue', iota_path)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', iota_path)

  # Make local edits to iota.
  svntest.main.file_append(iota_path, "\nMore text.\n")

  # diff r1:COMMITTED should show the property change but not the local edit.
  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff',
                                                           '-r1:COMMITTED',
                                                           iota_path)
  for line in out:
    if line.find("+More text.") != -1:
      raise svntest.Failure
  verify_expected_output(out, "+pvalue")

  # diff r1:BASE should show the property change but not the local edit.
  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff', '-r1:BASE',
                                                           iota_path)
  for line in out:
    if line.find("+More text.") != -1:
      raise svntest.Failure                   # fails at r7481
  verify_expected_output(out, "+pvalue")  # fails at r7481

  # diff r1:WC should show the local edit as well as the property change.
  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff', '-r1',
                                                           iota_path)
  verify_expected_output(out, "+More text.")  # fails at r7481
  verify_expected_output(out, "+pvalue")

#----------------------------------------------------------------------
def check_for_omitted_prefix_in_path_component(sbox):
  "check for omitted prefix in path component"

  sbox.build()
  svntest.actions.do_sleep_for_timestamps()

  prefix_path = os.path.join(sbox.wc_dir, 'prefix_mydir')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', prefix_path)
  other_prefix_path = os.path.join(sbox.wc_dir, 'prefix_other')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'mkdir', other_prefix_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)


  file_path = os.path.join(prefix_path, "test.txt")
  svntest.main.file_write(file_path, "Hello\nThere\nIota\n")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', file_path)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', sbox.wc_dir)


  prefix_url = sbox.repo_url + "/prefix_mydir"
  other_prefix_url = sbox.repo_url + "/prefix_other/mytag"
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg', prefix_url,
                                     other_prefix_url)

  svntest.main.file_write(file_path, "Hello\nWorld\nIota\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg', prefix_path)

  exit_code, out, err = svntest.actions.run_and_verify_svn(None, None, [],
                                                           'diff', prefix_url,
                                                           other_prefix_url)

  src = extract_diff_path(out[2])
  dest = extract_diff_path(out[3])

  good_src = ".../prefix_mydir"
  good_dest = ".../prefix_other/mytag"

  if ((src != good_src) or (dest != good_dest)):
    print("src is '%s' instead of '%s' and dest is '%s' instead of '%s'" %
          (src, good_src, dest, good_dest))
    raise svntest.Failure

#----------------------------------------------------------------------
@XFail()
def diff_renamed_file(sbox):
  "diff a file that has been renamed"

  sbox.build()

  os.chdir(sbox.wc_dir)

  pi_path = os.path.join('A', 'D', 'G', 'pi')
  pi2_path = os.path.join('A', 'D', 'pi2')
  svntest.main.file_write(pi_path, "new pi")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg')

  svntest.main.file_append(pi_path, "even more pi")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg')

  svntest.main.run_svn(None, 'mv', pi_path, pi2_path)

  # Repos->WC diff of the file
  exit_code, diff_output, err_output = svntest.main.run_svn(None,
                                                            'diff', '-r', '1',
                                                            pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'M') :
    raise svntest.Failure

  # Repos->WC diff of the file showing copies as adds
  exit_code, diff_output, err_output = svntest.main.run_svn(
                                         None, 'diff', '-r', '1',
                                         '--show-copies-as-adds', pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'A') :
    raise svntest.Failure

  svntest.main.file_append(pi2_path, "new pi")

  # Repos->WC of the containing directory
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', os.path.join('A', 'D'))

  if check_diff_output(diff_output,
                       pi_path,
                       'D') :
    raise svntest.Failure

  if check_diff_output(diff_output,
                       pi2_path,
                       'M') :
    raise svntest.Failure

  # Repos->WC of the containing directory showing copies as adds
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', '--show-copies-as-adds', os.path.join('A', 'D'))

  if check_diff_output(diff_output,
                       pi_path,
                       'D') :
    raise svntest.Failure

  if check_diff_output(diff_output,
                       pi2_path,
                       'A') :
    raise svntest.Failure

  # WC->WC of the file
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'M') :
    raise svntest.Failure

  # WC->WC of the file showing copies as adds
  exit_code, diff_output, err_output = svntest.main.run_svn(
                                         None, 'diff',
                                         '--show-copies-as-adds', pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'A') :
    raise svntest.Failure


  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg')

  # Repos->WC diff of file after the rename
  exit_code, diff_output, err_output = svntest.main.run_svn(None,
                                                            'diff', '-r', '1',
                                                            pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'M') :
    raise svntest.Failure

  # Repos->WC diff of file after the rename. The local file is not
  # a copy anymore (it has schedule "normal"), so --show-copies-as-adds
  # should have no effect.
  exit_code, diff_output, err_output = svntest.main.run_svn(
                                         None, 'diff', '-r', '1',
                                         '--show-copies-as-adds', pi2_path)
  if check_diff_output(diff_output,
                       pi2_path,
                       'M') :
    raise svntest.Failure

  # Repos->repos diff after the rename
  ### --show-copies-as-adds has no effect
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '2:3',
                                                            pi2_path)
  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'pi'),
                       'M') :
    raise svntest.Failure

#----------------------------------------------------------------------
def diff_within_renamed_dir(sbox):
  "diff a file within a renamed directory"

  sbox.build()

  os.chdir(sbox.wc_dir)

  svntest.main.run_svn(None, 'mv', os.path.join('A', 'D', 'G'),
                                   os.path.join('A', 'D', 'I'))
  # svntest.main.run_svn(None, 'ci', '-m', 'log_msg')
  svntest.main.file_write(os.path.join('A', 'D', 'I', 'pi'), "new pi")

  # Check a repos->wc diff
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', os.path.join('A', 'D', 'I', 'pi'))

  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'I', 'pi'),
                       'M') :
    raise svntest.Failure

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg')

  # Check repos->wc after commit
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', os.path.join('A', 'D', 'I', 'pi'))

  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'I', 'pi'),
                       'M') :
    raise svntest.Failure

  # Test the diff while within the moved directory
  os.chdir(os.path.join('A','D','I'))

  exit_code, diff_output, err_output = svntest.main.run_svn(None,
                                                            'diff', '-r', '1')

  if check_diff_output(diff_output, 'pi', 'M') :
    raise svntest.Failure

  # Test a repos->repos diff while within the moved directory
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2')

  if check_diff_output(diff_output, 'pi', 'M') :
    raise svntest.Failure

#----------------------------------------------------------------------
def diff_prop_on_named_dir(sbox):
  "diff a prop change on a dir named explicitly"

  # Diff of a property change or addition should contain a "+" line.
  # Diff of a property change or deletion should contain a "-" line.
  # On a diff between repository revisions (not WC) of a dir named
  # explicitly, the "-" line was missing.  (For a file, and for a dir
  # recursed into, the result was correct.)

  sbox.build()
  wc_dir = sbox.wc_dir

  os.chdir(sbox.wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'p', 'v', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propdel', 'p', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '')

  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r2:3', 'A')
  # Check that the result contains a "-" line.
  verify_expected_output(diff_output, "-v")

#----------------------------------------------------------------------
def diff_keywords(sbox):
  "ensure that diff won't show keywords"

  sbox.build()

  iota_path = os.path.join(sbox.wc_dir, 'iota')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ps',
                                     'svn:keywords',
                                     'Id Rev Date',
                                     iota_path)

  fp = open(iota_path, 'w')
  fp.write("$Date$\n")
  fp.write("$Id$\n")
  fp.write("$Rev$\n")
  fp.write("$Date::%s$\n" % (' ' * 80))
  fp.write("$Id::%s$\n"   % (' ' * 80))
  fp.write("$Rev::%s$\n"  % (' ' * 80))
  fp.close()

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'keywords', sbox.wc_dir)

  svntest.main.file_append(iota_path, "bar\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'added bar', sbox.wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', sbox.wc_dir)

  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'prev:head', sbox.wc_dir)

  verify_expected_output(diff_output, "+bar")
  verify_excluded_output(diff_output, "$Date:")
  verify_excluded_output(diff_output, "$Rev:")
  verify_excluded_output(diff_output, "$Id:")

  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'head:prev', sbox.wc_dir)

  verify_expected_output(diff_output, "-bar")
  verify_excluded_output(diff_output, "$Date:")
  verify_excluded_output(diff_output, "$Rev:")
  verify_excluded_output(diff_output, "$Id:")

  # Check fixed length keywords will show up
  # when the length of keyword has changed
  fp = open(iota_path, 'w')
  fp.write("$Date$\n")
  fp.write("$Id$\n")
  fp.write("$Rev$\n")
  fp.write("$Date::%s$\n" % (' ' * 79))
  fp.write("$Id::%s$\n"   % (' ' * 79))
  fp.write("$Rev::%s$\n"  % (' ' * 79))
  fp.close()

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'keywords 2', sbox.wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', sbox.wc_dir)

  exit_code, diff_output, err = svntest.actions.run_and_verify_svn(
    None, None, [], 'diff', '-r', 'prev:head', sbox.wc_dir)

  # these should show up
  verify_expected_output(diff_output, "+$Id:: ")
  verify_expected_output(diff_output, "-$Id:: ")
  verify_expected_output(diff_output, "-$Rev:: ")
  verify_expected_output(diff_output, "+$Rev:: ")
  verify_expected_output(diff_output, "-$Date:: ")
  verify_expected_output(diff_output, "+$Date:: ")
  # ... and these won't
  verify_excluded_output(diff_output, "$Date: ")
  verify_excluded_output(diff_output, "$Rev: ")
  verify_excluded_output(diff_output, "$Id: ")


def diff_force(sbox):
  "show diffs for binary files with --force"

  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')

  # Append a line to iota and make it binary.
  svntest.main.file_append(iota_path, "new line")
  svntest.main.run_svn(None,
                       'propset', 'svn:mime-type',
                       'application/octet-stream', iota_path)

  # Created expected output tree for 'svn ci'
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  # Create expected status tree
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota' : Item(status='  ', wc_rev=2),
    })

  # Commit iota, creating revision 2.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Add another line, while keeping he file as binary.
  svntest.main.file_append(iota_path, "another line")

  # Commit creating rev 3.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota' : Item(status='  ', wc_rev=3),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Check that we get diff when the first, the second and both files are
  # marked as binary.

  re_nodisplay = re.compile('^Cannot display:')

  exit_code, stdout, stderr = svntest.main.run_svn(None,
                                                   'diff', '-r1:2', iota_path,
                                                   '--force')

  for line in stdout:
    if (re_nodisplay.match(line)):
      raise svntest.Failure

  exit_code, stdout, stderr = svntest.main.run_svn(None,
                                                   'diff', '-r2:1', iota_path,
                                                   '--force')

  for line in stdout:
    if (re_nodisplay.match(line)):
      raise svntest.Failure

  exit_code, stdout, stderr = svntest.main.run_svn(None,
                                                   'diff', '-r2:3', iota_path,
                                                   '--force')

  for line in stdout:
    if (re_nodisplay.match(line)):
      raise svntest.Failure

#----------------------------------------------------------------------
# Regression test for issue #2333: Renaming a directory should produce
# deletion and addition diffs for each included file.
@Issue(2333)
def diff_renamed_dir(sbox):
  "diff a renamed directory"

  sbox.build()

  os.chdir(sbox.wc_dir)

  svntest.main.run_svn(None, 'mv', os.path.join('A', 'D', 'G'),
                                   os.path.join('A', 'D', 'I'))

  # Check a repos->wc diff
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '--show-copies-as-adds', os.path.join('A', 'D'))

  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'I', 'pi'),
                       'A') :
    raise svntest.Failure

  # Commit
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log msg')

  # Check repos->wc after commit
  exit_code, diff_output, err_output = svntest.main.run_svn(
    None, 'diff', '-r', '1', os.path.join('A', 'D'))

  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'I', 'pi'),
                       'A') :
    raise svntest.Failure

  # Test a repos->repos diff after commit
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2')
  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('A', 'D', 'I', 'pi'),
                       'A') :
    raise svntest.Failure

  # repos->repos with explicit URL arg
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2',
                                                            '^/A')
  if check_diff_output(diff_output,
                       os.path.join('D', 'G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('D', 'I', 'pi'),
                       'A') :
    raise svntest.Failure

  # Go to the parent of the moved directory
  os.chdir(os.path.join('A','D'))

  # repos->wc diff in the parent
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1')

  if check_diff_output(diff_output,
                       os.path.join('G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('I', 'pi'),
                       'A') :
    raise svntest.Failure

  # repos->repos diff in the parent
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2')

  if check_diff_output(diff_output,
                       os.path.join('G', 'pi'),
                       'D') :
    raise svntest.Failure
  if check_diff_output(diff_output,
                       os.path.join('I', 'pi'),
                       'A') :
    raise svntest.Failure

  # Go to the move target directory
  os.chdir('I')

  # repos->wc diff while within the moved directory (should be empty)
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1')
  if diff_output:
    raise svntest.Failure

  # repos->repos diff while within the moved directory (should be empty)
  exit_code, diff_output, err_output = svntest.main.run_svn(None, 'diff',
                                                            '-r', '1:2')

  if diff_output:
    raise svntest.Failure


#----------------------------------------------------------------------
def diff_property_changes_to_base(sbox):
  "diff to BASE with local property mods"

  sbox.build()
  wc_dir = sbox.wc_dir


  add_diff = \
    make_diff_prop_header("A") + \
    make_diff_prop_added("dirprop", "r2value") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_added("fileprop", "r2value")

  del_diff = \
    make_diff_prop_header("A") + \
    make_diff_prop_deleted("dirprop", "r2value") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_deleted("fileprop", "r2value")


  expected_output_r1_r2 = list(make_diff_header('A', 'revision 1', 'revision 2')
                               + add_diff[:6]
                               + make_diff_header('iota', 'revision 1',
                                                   'revision 2')
                               + add_diff[7:])

  expected_output_r2_r1 = list(make_diff_header('A', 'revision 2',
                                                'revision 1')
                               + del_diff[:6]
                               + make_diff_header('iota', 'revision 2',
                                                  'revision 1')
                               + del_diff[7:])

  expected_output_r1 = list(make_diff_header('A', 'revision 1',
                                             'working copy')
                            + add_diff[:6]
                            + make_diff_header('iota', 'revision 1',
                                               'working copy')
                            + add_diff[7:])
  expected_output_base_r1 = list(make_diff_header('A', 'working copy',
                                                  'revision 1')
                                 + del_diff[:6]
                                 + make_diff_header('iota', 'working copy',
                                                    'revision 1')
                                 + del_diff[7:])

  os.chdir(sbox.wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'fileprop', 'r2value', 'iota')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'dirprop', 'r2value', 'A')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'empty-msg')

  # Check that forward and reverse repos-repos diffs are as expected.
  expected = svntest.verify.UnorderedOutput(expected_output_r1_r2)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '1:2')

  expected = svntest.verify.UnorderedOutput(expected_output_r2_r1)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '2:1')

  # Now check repos->WORKING, repos->BASE, and BASE->repos.
  # (BASE is r1, and WORKING has no local mods, so this should produce
  # the same output as above).
  expected = svntest.verify.UnorderedOutput(expected_output_r1)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '1')

  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '1:BASE')

  expected = svntest.verify.UnorderedOutput(expected_output_base_r1)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', 'BASE:1')

  # Modify some properties.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'fileprop', 'workingvalue', 'iota')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'dirprop', 'workingvalue', 'A')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'fileprop', 'workingvalue', 'A/mu')

  # Check that the earlier diffs against BASE are unaffected by the
  # presence of local mods (with the exception of diff header changes).
  expected = svntest.verify.UnorderedOutput(expected_output_r1)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '1:BASE')

  expected = svntest.verify.UnorderedOutput(expected_output_base_r1)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', 'BASE:1')

def diff_schedule_delete(sbox):
  "scheduled deleted"

  sbox.build()

  expected_output_r2_working = make_diff_header("foo", "revision 2",
                                                "working copy") + [
  "@@ -1 +0,0 @@\n",
  "-xxx\n"
  ]

  expected_output_r2_base = make_diff_header("foo", "revision 2",
                                                "working copy") + [
  "@@ -1 +1,2 @@\n",
  " xxx\n",
  "+yyy\n"
  ]
  expected_output_base_r2 = make_diff_header("foo", "working copy",
                                                "revision 2") + [
  "@@ -1,2 +1 @@\n",
  " xxx\n",
  "-yyy\n"
  ]

  expected_output_r1_base = make_diff_header("foo", "revision 0",
                                                "working copy") + [
  "@@ -0,0 +1,2 @@\n",
  "+xxx\n",
  "+yyy\n"
  ]
  expected_output_base_r1 = make_diff_header("foo", "working copy",
                                                "revision 1") + [
  "@@ -1,2 +0,0 @@\n",
  "-xxx\n",
  "-yyy\n"
  ]
  expected_output_base_working = expected_output_base_r1[:]
  expected_output_base_working[2] = "--- foo\t(revision 3)\n"
  expected_output_base_working[3] = "+++ foo\t(working copy)\n"

  wc_dir = sbox.wc_dir
  os.chdir(wc_dir)

  svntest.main.file_append('foo', "xxx\n")
  svntest.main.run_svn(None, 'add', 'foo')
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg r2')

  svntest.main.file_append('foo', "yyy\n")
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg r3')

  # Update everyone's BASE to r3, and mark 'foo' as schedule-deleted.
  svntest.main.run_svn(None,
                       'up')
  svntest.main.run_svn(None, 'rm', 'foo')

  # A file marked as schedule-delete should act as if were not present
  # in WORKING, but diffs against BASE should remain unaffected.

  # 1. repos-wc diff: file not present in repos.
  svntest.actions.run_and_verify_svn(None, [], [],
                                     'diff', '-r', '1')
  svntest.actions.run_and_verify_svn(None, expected_output_r1_base, [],
                                     'diff', '-r', '1:BASE')
  svntest.actions.run_and_verify_svn(None, expected_output_base_r1, [],
                                     'diff', '-r', 'BASE:1')

  # 2. repos-wc diff: file present in repos.
  svntest.actions.run_and_verify_svn(None, expected_output_r2_working, [],
                                     'diff', '-r', '2')
  svntest.actions.run_and_verify_svn(None, expected_output_r2_base, [],
                                     'diff', '-r', '2:BASE')
  svntest.actions.run_and_verify_svn(None, expected_output_base_r2, [],
                                     'diff', '-r', 'BASE:2')

  # 3. wc-wc diff.
  svntest.actions.run_and_verify_svn(None, expected_output_base_working, [],
                                     'diff')

#----------------------------------------------------------------------
def diff_mime_type_changes(sbox):
  "repos-wc diffs with local svn:mime-type prop mods"

  sbox.build()

  expected_output_r1_wc = make_diff_header("iota", "revision 1",
                                                "working copy") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+revision 2 text.\n" ]

  expected_output_wc_r1 = make_diff_header("iota", "working copy",
                                                "revision 1") + [
    "@@ -1,2 +1 @@\n",
    " This is the file 'iota'.\n",
    "-revision 2 text.\n" ]


  os.chdir(sbox.wc_dir)

  # Append some text to iota (r2).
  svntest.main.file_append('iota', "revision 2 text.\n")

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Check that forward and reverse repos-BASE diffs are as expected.
  svntest.actions.run_and_verify_svn(None, expected_output_r1_wc, [],
                                     'diff', '-r', '1:BASE')

  svntest.actions.run_and_verify_svn(None, expected_output_wc_r1, [],
                                     'diff', '-r', 'BASE:1')

  # Mark iota as a binary file in the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'svn:mime-type',
                                     'application/octet-stream', 'iota')

  # Check that the earlier diffs against BASE are unaffected by the
  # presence of local svn:mime-type property mods.
  svntest.actions.run_and_verify_svn(None, expected_output_r1_wc, [],
                                     'diff', '-r', '1:BASE')

  svntest.actions.run_and_verify_svn(None, expected_output_wc_r1, [],
                                     'diff', '-r', 'BASE:1')

  # Commit the change (r3) (so that BASE has the binary MIME type), then
  # mark iota as a text file again in the working copy.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propdel', 'svn:mime-type', 'iota')

  # Now diffs against BASE will fail, but diffs against WORKNG should be
  # fine.
  svntest.actions.run_and_verify_svn(None, expected_output_r1_wc, [],
                                     'diff', '-r', '1')


#----------------------------------------------------------------------
# Test a repos-WORKING diff, with different versions of the same property
# at repository, BASE, and WORKING.
def diff_prop_change_local_propmod(sbox):
  "diff a property change plus a local prop edit"

  sbox.build()

  expected_output_r2_wc = \
    make_diff_header("A", "revision 2", "working copy") + \
    make_diff_prop_header("A") + \
    make_diff_prop_modified("dirprop", "r2value", "workingvalue") + \
    make_diff_prop_added("newdirprop", "newworkingvalue") + \
    make_diff_header("iota", "revision 2", "working copy") + \
    make_diff_prop_header("iota") + \
    make_diff_prop_modified("fileprop", "r2value", "workingvalue") + \
    make_diff_prop_added("newfileprop", "newworkingvalue")

  os.chdir(sbox.wc_dir)

  # Set a property on A/ and iota, and commit them (r2).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'dirprop',
                                     'r2value', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'fileprop',
                                     'r2value', 'iota')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Change the property values on A/ and iota, and commit them (r3).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'dirprop',
                                     'r3value', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'fileprop',
                                     'r3value', 'iota')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Finally, change the property values one last time.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'dirprop',
                                     'workingvalue', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'fileprop',
                                     'workingvalue', 'iota')
  # And also add some properties that only exist in WORKING.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'newdirprop',
                                     'newworkingvalue', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'newfileprop',
                                     'newworkingvalue', 'iota')

  # Now, if we diff r2 to WORKING, we've got three property values
  # to consider: r2value (in the repository), r3value (in BASE), and
  # workingvalue (in WORKING).
  # The diff should only show the r2->WORKING change.
  #
  # We also need to make sure that the 'new' (WORKING only) properties
  # are included in the output, since they won't be listed in a simple
  # BASE->r2 diff.
  expected = svntest.verify.UnorderedOutput(expected_output_r2_wc)
  svntest.actions.run_and_verify_svn(None, expected, [],
                                     'diff', '-r', '2')


#----------------------------------------------------------------------
# repos->wc and BASE->repos diffs that add files or directories with
# properties should show the added properties.
def diff_repos_wc_add_with_props(sbox):
  "repos-wc diff showing added entries with props"

  sbox.build()

  diff_foo = [
    "@@ -0,0 +1 @@\n",
    "+content\n",
    ] + make_diff_prop_header("foo") + \
    make_diff_prop_added("propname", "propvalue")
  diff_X = \
    make_diff_prop_header("X") + \
    make_diff_prop_added("propname", "propvalue")
  diff_X_bar = [
    "@@ -0,0 +1 @@\n",
    "+content\n",
    ] + make_diff_prop_header("X/bar") + \
    make_diff_prop_added("propname", "propvalue")

  diff_X_r1_base = make_diff_header("X", "revision 1",
                                         "working copy") + diff_X
  diff_X_base_r3 = make_diff_header("X", "working copy",
                                         "revision 3") + diff_X
  diff_foo_r1_base = make_diff_header("foo", "revision 0",
                                             "revision 3") + diff_foo
  diff_foo_base_r3 = make_diff_header("foo", "revision 0",
                                             "revision 3") + diff_foo
  diff_X_bar_r1_base = make_diff_header("X/bar", "revision 0",
                                                 "revision 3") + diff_X_bar
  diff_X_bar_base_r3 = make_diff_header("X/bar", "revision 0",
                                                 "revision 3") + diff_X_bar

  expected_output_r1_base = svntest.verify.UnorderedOutput(diff_X_r1_base +
                                                           diff_X_bar_r1_base +
                                                           diff_foo_r1_base)
  expected_output_base_r3 = svntest.verify.UnorderedOutput(diff_foo_base_r3 +
                                                           diff_X_bar_base_r3 +
                                                           diff_X_base_r3)

  os.chdir(sbox.wc_dir)

  # Create directory X, file foo, and file X/bar, and commit them (r2).
  os.makedirs('X')
  svntest.main.file_append('foo', "content\n")
  svntest.main.file_append(os.path.join('X', 'bar'), "content\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', 'X', 'foo')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Set a property on all three items, and commit them (r3).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'propname',
                                     'propvalue', 'X', 'foo',
                                     os.path.join('X', 'bar'))
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Now, if we diff r1 to WORKING or BASE, we should see the content
  # addition for foo and X/bar, and property additions for all three.
  svntest.actions.run_and_verify_svn(None, expected_output_r1_base, [],
                                     'diff', '-r', '1')
  svntest.actions.run_and_verify_svn(None, expected_output_r1_base, [],
                                     'diff', '-r', '1:BASE')

  # Update the BASE and WORKING revisions to r1.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r', '1')

  # If we diff BASE to r3, we should see the same output as above.
  svntest.actions.run_and_verify_svn(None, expected_output_base_r3, [],
                                     'diff', '-r', 'BASE:3')


#----------------------------------------------------------------------
# repos-wc diffs on a non-recursively checked out wc that would normally
# (if recursively checked out) include a directory that is not present in
# the repos version should not segfault.
def diff_nonrecursive_checkout_deleted_dir(sbox):
  "nonrecursive diff + deleted directories"
  sbox.build()

  url = sbox.repo_url
  A_url = url + '/A'
  A_prime_url = url + '/A_prime'

  svntest.main.run_svn(None,
                       'cp', '-m', 'log msg', A_url, A_prime_url)

  svntest.main.run_svn(None,
                       'mkdir', '-m', 'log msg', A_prime_url + '/Q')

  wc = sbox.add_wc_path('wc')

  svntest.main.run_svn(None,
                       'co', '-N', A_prime_url, wc)

  os.chdir(wc)

  # We don't particular care about the output here, just that it doesn't
  # segfault.
  svntest.main.run_svn(None,
                       'diff', '-r1')


#----------------------------------------------------------------------
# repos->WORKING diffs that include directories with local mods that are
# not present in the repos version should work as expected (and not, for
# example, show an extraneous BASE->WORKING diff for the added directory
# after the repos->WORKING output).
def diff_repos_working_added_dir(sbox):
  "repos->WORKING diff showing added modifed dir"

  sbox.build()

  expected_output_r1_BASE = make_diff_header("X/bar", "revision 0",
                                                "revision 2") + [
    "@@ -0,0 +1 @@\n",
    "+content\n" ]
  expected_output_r1_WORKING = make_diff_header("X/bar", "revision 0",
                                                "revision 2") + [
    "@@ -0,0 +1,2 @@\n",
    "+content\n",
    "+more content\n" ]

  os.chdir(sbox.wc_dir)

  # Create directory X and file X/bar, and commit them (r2).
  os.makedirs('X')
  svntest.main.file_append(os.path.join('X', 'bar'), "content\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', 'X')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'log_msg')

  # Make a local modification to X/bar.
  svntest.main.file_append(os.path.join('X', 'bar'), "more content\n")

  # Now, if we diff r1 to WORKING or BASE, we should see the content
  # addition for X/bar, and (for WORKING) the local modification.
  svntest.actions.run_and_verify_svn(None, expected_output_r1_BASE, [],
                                     'diff', '-r', '1:BASE')
  svntest.actions.run_and_verify_svn(None, expected_output_r1_WORKING, [],
                                     'diff', '-r', '1')


#----------------------------------------------------------------------
# A base->repos diff of a moved file used to output an all-lines-deleted diff
def diff_base_repos_moved(sbox):
  "base->repos diff of moved file"

  sbox.build()

  os.chdir(sbox.wc_dir)

  oldfile = 'iota'
  newfile = 'kappa'

  # Move, modify and commit a file
  svntest.main.run_svn(None, 'mv', oldfile, newfile)
  svntest.main.file_write(newfile, "new content\n")
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '')

  # Check that a base->repos diff with copyfrom shows deleted and added lines.
  exit_code, out, err = svntest.actions.run_and_verify_svn(
    None, svntest.verify.AnyOutput, [], 'diff', '-rBASE:1', newfile)

  if check_diff_output(out, newfile, 'M'):
    raise svntest.Failure

  # Diff should recognise that the item's name has changed, and mention both
  # the current and the old name in parentheses, in the right order.
  if (out[2][:3] != '---' or out[2].find('kappa)') == -1 or
      out[3][:3] != '+++' or out[3].find('iota)') == -1):
    raise svntest.Failure

#----------------------------------------------------------------------
# A diff of an added file within an added directory should work, and
# shouldn't produce an error.
def diff_added_subtree(sbox):
  "wc->repos diff of added subtree"

  sbox.build()

  os.chdir(sbox.wc_dir)

  # Roll the wc back to r0 (i.e. an empty wc).
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r0')

  # We shouldn't get any errors when we request a diff showing the
  # addition of the greek tree.  The diff contains additions of files
  # and directories with parents that don't currently exist in the wc,
  # which is what we're testing here.
  svntest.actions.run_and_verify_svn(None, svntest.verify.AnyOutput, [],
                                     'diff', '-r', 'BASE:1')

#----------------------------------------------------------------------
def basic_diff_summarize(sbox):
  "basic diff summarize"

  sbox.build()
  wc_dir = sbox.wc_dir
  p = sbox.ospath

  # Add props to some items that will be deleted, and commit.
  sbox.simple_propset('prop', 'val',
                      'A/C',
                      'A/D/gamma',
                      'A/D/H/chi')
  sbox.simple_commit() # r2
  sbox.simple_update()

  # Content modification.
  svntest.main.file_append(p('A/mu'), 'new text\n')

  # Prop modification.
  sbox.simple_propset('prop', 'val', 'iota')

  # Both content and prop mods.
  svntest.main.file_append(p('A/D/G/tau'), 'new text\n')
  sbox.simple_propset('prop', 'val', 'A/D/G/tau')

  # File addition.
  svntest.main.file_append(p('newfile'), 'new text\n')
  svntest.main.file_append(p('newfile2'), 'new text\n')
  sbox.simple_add('newfile',
                  'newfile2')
  sbox.simple_propset('prop', 'val', 'newfile')

  # File deletion.
  sbox.simple_rm('A/B/lambda',
                 'A/D/gamma')

  # Directory addition.
  os.makedirs(p('P'))
  os.makedirs(p('Q/R'))
  svntest.main.file_append(p('Q/newfile'), 'new text\n')
  svntest.main.file_append(p('Q/R/newfile'), 'new text\n')
  sbox.simple_add('P',
                  'Q')
  sbox.simple_propset('prop', 'val',
                      'P',
                      'Q/newfile')

  # Directory deletion.
  sbox.simple_rm('A/D/H',
                 'A/C')

  # Commit, because diff-summarize handles repos-repos only.
  #svntest.main.run_svn(False, 'st', wc_dir)
  sbox.simple_commit() # r3

  # Get the differences between two versions of a file.
  expected_diff = svntest.wc.State(wc_dir, {
    'iota': Item(status=' M'),
    })
  svntest.actions.run_and_verify_diff_summarize(expected_diff,
                                                p('iota'), '-c3')
  svntest.actions.run_and_verify_diff_summarize(expected_diff,
                                                p('iota'), '-c-3')

  # wc-wc diff summary for a directory.
  expected_diff = svntest.wc.State(wc_dir, {
    'A/mu':           Item(status='M '),
    'iota':           Item(status=' M'),
    'A/D/G/tau':      Item(status='MM'),
    'newfile':        Item(status='A '),
    'newfile2':       Item(status='A '),
    'P':              Item(status='A '),
    'Q':              Item(status='A '),
    'Q/newfile':      Item(status='A '),
    'Q/R':            Item(status='A '),
    'Q/R/newfile':    Item(status='A '),
    'A/B/lambda':     Item(status='D '),
    'A/C':            Item(status='D '),
    'A/D/gamma':      Item(status='D '),
    'A/D/H':          Item(status='D '),
    'A/D/H/chi':      Item(status='D '),
    'A/D/H/psi':      Item(status='D '),
    'A/D/H/omega':    Item(status='D '),
    })

  expected_reverse_diff = svntest.wc.State(wc_dir, {
    'A/mu':           Item(status='M '),
    'iota':           Item(status=' M'),
    'A/D/G/tau':      Item(status='MM'),
    'newfile':        Item(status='D '),
    'newfile2':       Item(status='D '),
    'P':              Item(status='D '),
    'Q':              Item(status='D '),
    'Q/newfile':      Item(status='D '),
    'Q/R':            Item(status='D '),
    'Q/R/newfile':    Item(status='D '),
    'A/B/lambda':     Item(status='A '),
    'A/C':            Item(status='A '),
    'A/D/gamma':      Item(status='A '),
    'A/D/H':          Item(status='A '),
    'A/D/H/chi':      Item(status='A '),
    'A/D/H/psi':      Item(status='A '),
    'A/D/H/omega':    Item(status='A '),
    })

  svntest.actions.run_and_verify_diff_summarize(expected_diff,
                                                wc_dir, '-c3')
  svntest.actions.run_and_verify_diff_summarize(expected_reverse_diff,
                                                wc_dir, '-c-3')

  # Get the differences between a newly added file 
  expected_diff = svntest.wc.State(wc_dir, {
    'newfile': Item(status='A '),
    })
  expected_reverse_diff = svntest.wc.State(wc_dir, {
    'newfile': Item(status='D '),
    })
  svntest.actions.run_and_verify_diff_summarize(expected_diff,
                                                p('newfile'), '-c3')
  svntest.actions.run_and_verify_diff_summarize(expected_reverse_diff,
                                                p('newfile'), '-c-3')

  # Get the differences between a newly added dir 
  expected_diff = svntest.wc.State(wc_dir, {
    'P': Item(status='A '),
    })
  expected_reverse_diff = svntest.wc.State(wc_dir, {
    'P': Item(status='D '),
    })
  svntest.actions.run_and_verify_diff_summarize(expected_diff,
                                                p('P'), '-c3')
  svntest.actions.run_and_verify_diff_summarize(expected_reverse_diff,
                                                p('P'), '-c-3')

#----------------------------------------------------------------------
def diff_weird_author(sbox):
  "diff with svn:author that has < in it"

  sbox.build()

  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  svntest.main.file_write(os.path.join(sbox.wc_dir, 'A', 'mu'),
                          "new content\n")

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'A/mu': Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(sbox.wc_dir, 1)
  expected_status.tweak("A/mu", wc_rev=2)

  svntest.actions.run_and_verify_commit(sbox.wc_dir, expected_output,
                                        expected_status, None, sbox.wc_dir)

  svntest.main.run_svn(None,
                       "propset", "--revprop", "-r", "2", "svn:author",
                       "J. Random <jrandom@example.com>", sbox.repo_url)

  svntest.actions.run_and_verify_svn(None,
                                     ["J. Random <jrandom@example.com>\n"],
                                     [],
                                     "pget", "--revprop", "-r" "2",
                                     "svn:author", sbox.repo_url)

  expected_output = make_diff_header("A/mu", "revision 1", "revision 2") + [
    "@@ -1 +1 @@\n",
    "-This is the file 'mu'.\n",
    "+new content\n"
  ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-r1:2', sbox.repo_url)

# test for issue 2121, use -x -w option for ignoring whitespace during diff
@Issue(2121)
def diff_ignore_whitespace(sbox):
  "ignore whitespace when diffing"

  sbox.build()
  wc_dir = sbox.wc_dir

  file_name = "iota"
  file_path = os.path.join(wc_dir, file_name)

  svntest.main.file_write(file_path,
                          "Aa\n"
                          "Bb\n"
                          "Cc\n")
  expected_output = svntest.wc.State(wc_dir, {
      'iota' : Item(verb='Sending'),
      })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, None, wc_dir)

  # only whitespace changes, should return no changes
  svntest.main.file_write(file_path,
                          " A  a   \n"
                          "   B b  \n"
                          "    C    c    \n")

  svntest.actions.run_and_verify_svn(None, [], [],
                                     'diff', '-x', '-w', file_path)

  # some changes + whitespace
  svntest.main.file_write(file_path,
                          " A  a   \n"
                          "Xxxx X\n"
                          "   Bb b  \n"
                          "    C    c    \n")
  expected_output = make_diff_header(file_path, "revision 2",
                                     "working copy") + [
    "@@ -1,3 +1,4 @@\n",
    " Aa\n",
    "-Bb\n",
    "+Xxxx X\n",
    "+   Bb b  \n",
    " Cc\n" ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-x', '-w', file_path)

def diff_ignore_eolstyle(sbox):
  "ignore eol styles when diffing"

  sbox.build()
  wc_dir = sbox.wc_dir

  file_name = "iota"
  file_path = os.path.join(wc_dir, file_name)

  svntest.main.file_write(file_path,
                          "Aa\n"
                          "Bb\n"
                          "Cc\n")
  expected_output = svntest.wc.State(wc_dir, {
      'iota' : Item(verb='Sending'),
      })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, None, wc_dir)

  # commit only eol changes
  svntest.main.file_write(file_path,
                          "Aa\r"
                          "Bb\r"
                          "Cc")

  expected_output = make_diff_header(file_path, "revision 2",
                                     "working copy") + [
    "@@ -1,3 +1,3 @@\n",
    " Aa\n",
    " Bb\n",
    "-Cc\n",
    "+Cc\n",
    "\ No newline at end of file\n" ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-x', '--ignore-eol-style',
                                     file_path)

# test for issue 2600, diff revision of a file in a renamed folder
@Issue(2600)
def diff_in_renamed_folder(sbox):
  "diff a revision of a file in a renamed folder"

  sbox.build()
  wc_dir = sbox.wc_dir

  C_path = os.path.join(wc_dir, "A", "C")
  D_path = os.path.join(wc_dir, "A", "D")
  kappa_path = os.path.join(D_path, "C", "kappa")

  # add a new file to a renamed (moved in this case) folder.
  svntest.main.run_svn(None, 'mv', C_path, D_path)

  svntest.main.file_append(kappa_path, "this is file kappa.\n")
  svntest.main.run_svn(None, 'add', kappa_path)

  expected_output = svntest.wc.State(wc_dir, {
      'A/C' : Item(verb='Deleting'),
      'A/D/C' : Item(verb='Adding'),
      'A/D/C/kappa' : Item(verb='Adding'),
  })
  ### right now, we cannot denote that kappa is a local-add rather than a
  ### child of the A/D/C copy. thus, it appears in the status output as a
  ### (M)odified child.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, None, wc_dir)

  expected_output = svntest.wc.State(wc_dir, {
      'A/D/C/kappa' : Item(verb='Sending'),
  })

  # modify the file two times so we have something to diff.
  for i in range(3, 5):
    svntest.main.file_append(kappa_path, str(i) + "\n")
    svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                          None, None, wc_dir)

  expected_output = make_diff_header(kappa_path, "revision 3",
                                     "revision 4") + [
    "@@ -1,2 +1,3 @@\n",
    " this is file kappa.\n",
    " 3\n",
    "+4\n"
  ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '-r3:4', kappa_path)

def diff_with_depth(sbox):
  "test diffs at various depths"

  sbox.build()
  B_path = os.path.join('A', 'B')

  diff = make_diff_prop_header(".") + \
         make_diff_prop_added("foo1", "bar1") + \
         make_diff_prop_header("iota") + \
         make_diff_prop_added("foo2", "bar2") + \
         make_diff_prop_header("A") + \
         make_diff_prop_added("foo3", "bar3") + \
         make_diff_prop_header("A/B") + \
         make_diff_prop_added("foo4", "bar4")

  dot_header = make_diff_header(".", "revision 1", "working copy")
  iota_header = make_diff_header('iota', "revision 1", "working copy")
  A_header = make_diff_header('A', "revision 1", "working copy")
  B_header = make_diff_header(B_path, "revision 1", "working copy")

  expected_empty = svntest.verify.UnorderedOutput(dot_header + diff[:7])
  expected_files = svntest.verify.UnorderedOutput(dot_header + diff[:7]
                                                  + iota_header + diff[8:14])
  expected_immediates = svntest.verify.UnorderedOutput(dot_header + diff[:7]
                                                       + iota_header
                                                       + diff[8:14]
                                                       + A_header + diff[15:21])
  expected_infinity = svntest.verify.UnorderedOutput(dot_header + diff[:7]
                                                       + iota_header
                                                       + diff[8:14]
                                                       + A_header + diff[15:21]
                                                       + B_header + diff[22:])

  os.chdir(sbox.wc_dir)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo1', 'bar1', '.')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo2', 'bar2', 'iota')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo3', 'bar3', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo4', 'bar4', os.path.join('A', 'B'))

  # Test wc-wc diff.
  svntest.actions.run_and_verify_svn(None, expected_empty, [],
                                     'diff', '--depth', 'empty')
  svntest.actions.run_and_verify_svn(None, expected_files, [],
                                     'diff', '--depth', 'files')
  svntest.actions.run_and_verify_svn(None, expected_immediates, [],
                                     'diff', '--depth', 'immediates')
  svntest.actions.run_and_verify_svn(None, expected_infinity, [],
                                     'diff', '--depth', 'infinity')

  # Commit the changes.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', '')

  dot_header = make_diff_header(".", "revision 1", "revision 2")
  iota_header = make_diff_header('iota', "revision 1", "revision 2")
  A_header = make_diff_header('A', "revision 1", "revision 2")
  B_header = make_diff_header(B_path, "revision 1", "revision 2")

  expected_empty = svntest.verify.UnorderedOutput(dot_header + diff[:7])
  expected_files = svntest.verify.UnorderedOutput(dot_header + diff[:7]
                                                  + iota_header + diff[8:14])
  expected_immediates = svntest.verify.UnorderedOutput(dot_header + diff[:7]
                                                       + iota_header
                                                       + diff[8:14]
                                                       + A_header + diff[15:21])
  expected_infinity = svntest.verify.UnorderedOutput(dot_header + diff[:6]
                                                       + iota_header
                                                       + diff[8:14]
                                                       + A_header + diff[15:21]
                                                       + B_header + diff[22:])

  # Test repos-repos diff.
  svntest.actions.run_and_verify_svn(None, expected_empty, [],
                                     'diff', '-c2', '--depth', 'empty')
  svntest.actions.run_and_verify_svn(None, expected_files, [],
                                     'diff', '-c2', '--depth', 'files')
  svntest.actions.run_and_verify_svn(None, expected_immediates, [],
                                     'diff', '-c2', '--depth', 'immediates')
  svntest.actions.run_and_verify_svn(None, expected_infinity, [],
                                     'diff', '-c2', '--depth', 'infinity')

  diff_wc_repos = \
    make_diff_header("A/B", "revision 2", "working copy") + \
    make_diff_prop_header("A/B") + \
    make_diff_prop_modified("foo4", "bar4", "baz4") + \
    make_diff_header("A", "revision 2", "working copy") + \
    make_diff_prop_header("A") + \
    make_diff_prop_modified("foo3", "bar3", "baz3") + \
    make_diff_header("A/mu", "revision 1", "working copy") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'mu'.\n",
    "+new text\n",
    ] + make_diff_header("iota", "revision 2", "working copy") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+new text\n",
    ] + make_diff_prop_header("iota") + \
    make_diff_prop_modified("foo2", "bar2", "baz2") + \
    make_diff_header(".", "revision 2", "working copy") + \
    make_diff_prop_header(".") + \
    make_diff_prop_modified("foo1", "bar1", "baz1")

  expected_empty = svntest.verify.UnorderedOutput(diff_wc_repos[49:])
  expected_files = svntest.verify.UnorderedOutput(diff_wc_repos[33:])
  expected_immediates = svntest.verify.UnorderedOutput(diff_wc_repos[13:26]
                                                       +diff_wc_repos[33:])
  expected_infinity = svntest.verify.UnorderedOutput(diff_wc_repos[:])

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '-r1')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo1', 'baz1', '.')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo2', 'baz2', 'iota')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo3', 'baz3', 'A')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset',
                                     'foo4', 'baz4', os.path.join('A', 'B'))
  svntest.main.file_append(os.path.join('A', 'mu'), "new text\n")
  svntest.main.file_append('iota', "new text\n")

  # Test wc-repos diff.
  svntest.actions.run_and_verify_svn(None, expected_empty, [],
                                     'diff', '-rHEAD', '--depth', 'empty')
  svntest.actions.run_and_verify_svn(None, expected_files, [],
                                     'diff', '-rHEAD', '--depth', 'files')
  svntest.actions.run_and_verify_svn(None, expected_immediates, [],
                                     'diff', '-rHEAD', '--depth', 'immediates')
  svntest.actions.run_and_verify_svn(None, expected_infinity, [],
                                     'diff', '-rHEAD', '--depth', 'infinity')

# test for issue 2920: ignore eol-style on empty lines
@Issue(2920)
def diff_ignore_eolstyle_empty_lines(sbox):
  "ignore eol styles when diffing empty lines"

  sbox.build()
  wc_dir = sbox.wc_dir

  file_name = "iota"
  file_path = os.path.join(wc_dir, file_name)

  svntest.main.file_write(file_path,
                          "Aa\n"
                          "\n"
                          "Bb\n"
                          "\n"
                          "Cc\n")
  expected_output = svntest.wc.State(wc_dir, {
      'iota' : Item(verb='Sending'),
      })
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        None, None, wc_dir)

  # sleep to guarantee timestamp change
  time.sleep(1.1)

  # commit only eol changes
  svntest.main.file_write(file_path,
                          "Aa\012"
                          "\012"
                          "Bb\r"
                          "\r"
                          "Cc\012",
                          mode="wb")

  svntest.actions.run_and_verify_svn(None, [], [],
                                     'diff', '-x', '--ignore-eol-style',
                                     file_path)

def diff_backward_repos_wc_copy(sbox):
  "backward repos->wc diff with copied file"

  sbox.build()
  wc_dir = sbox.wc_dir
  os.chdir(wc_dir)

  # copy a file
  mu_path = os.path.join('A', 'mu')
  mucp_path = os.path.join('A', 'mucopy')
  svntest.main.run_svn(None, 'cp', mu_path, mucp_path)

  # commit r2 and update back to r1
  svntest.main.run_svn(None,
                       'ci', '-m', 'log msg')
  svntest.main.run_svn(None, 'up', '-r1')

  # diff r2 against working copy
  diff_repos_wc = make_diff_header("A/mucopy", "revision 2", "working copy")
  diff_repos_wc += [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'mu'.\n",
  ]

  svntest.actions.run_and_verify_svn(None, diff_repos_wc, [],
                                     'diff', '-r' , '2')

#----------------------------------------------------------------------

def diff_summarize_xml(sbox):
  "xml diff summarize"

  sbox.build()
  wc_dir = sbox.wc_dir

  # A content modification.
  svntest.main.file_append(os.path.join(wc_dir, "A", "mu"), "New mu content")

  # A prop modification.
  svntest.main.run_svn(None,
                       "propset", "prop", "val",
                       os.path.join(wc_dir, 'iota'))

  # Both content and prop mods.
  tau_path = os.path.join(wc_dir, "A", "D", "G", "tau")
  svntest.main.file_append(tau_path, "tautau")
  svntest.main.run_svn(None,
                       "propset", "prop", "val", tau_path)

  # A file addition.
  newfile_path = os.path.join(wc_dir, 'newfile')
  svntest.main.file_append(newfile_path, 'newfile')
  svntest.main.run_svn(None, 'add', newfile_path)

  # A file deletion.
  svntest.main.run_svn(None, "delete", os.path.join(wc_dir, 'A', 'B',
                                                    'lambda'))

  # A directory addition
  svntest.main.run_svn(None, "mkdir", os.path.join(wc_dir, 'newdir'))

  expected_output = svntest.wc.State(wc_dir, {
    'A/mu': Item(verb='Sending'),
    'iota': Item(verb='Sending'),
    'newfile': Item(verb='Adding'),
    'A/D/G/tau': Item(verb='Sending'),
    'A/B/lambda': Item(verb='Deleting'),
    'newdir': Item(verb='Adding'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'newfile': Item(status='  ', wc_rev=2),
    'newdir': Item(status='  ', wc_rev=2),
    })
  expected_status.tweak("A/mu", "iota", "A/D/G/tau", "newfile", "newdir",
                        wc_rev=2)
  expected_status.remove("A/B/lambda")

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # 1) Test --xml without --summarize
  svntest.actions.run_and_verify_svn(
    None, None, ".*--xml' option only valid with '--summarize' option",
    'diff', wc_dir, '--xml')

  # 2) Test --xml on invalid revision
  svntest.actions.run_and_verify_diff_summarize_xml(
    ".*No such revision 5555555",
    None, wc_dir, None, None, None, '-r0:5555555', wc_dir)

  # 3) Test working copy summarize
  svntest.actions.run_and_verify_diff_summarize_xml(
    ".*Summarizing diff can only compare repository to repository",
    None, wc_dir, None, None, wc_dir)

  # 4) Test --summarize --xml on -c2
  paths = ['iota',]
  items = ['none',]
  kinds = ['file',]
  props = ['modified',]

  svntest.actions.run_and_verify_diff_summarize_xml(
    [], wc_dir, paths, items, props, kinds, '-c2',
    os.path.join(wc_dir, 'iota'))

  # 5) Test --summarize --xml on -r1:2
  paths = ['A/mu', 'iota', 'A/D/G/tau', 'newfile', 'A/B/lambda',
           'newdir',]
  items = ['modified', 'none', 'modified', 'added', 'deleted', 'added',]
  kinds = ['file','file','file','file','file', 'dir',]
  props = ['none', 'modified', 'modified', 'none', 'none', 'none',]

  svntest.actions.run_and_verify_diff_summarize_xml(
    [], wc_dir, paths, items, props, kinds, '-r1:2', wc_dir)

  # 6) Same as test #5 but ran against a URL instead of a WC path
  paths = ['A/mu', 'iota', 'A/D/G/tau', 'newfile', 'A/B/lambda',
           'newdir',]
  items = ['modified', 'none', 'modified', 'added', 'deleted', 'added',]
  kinds = ['file','file','file','file','file', 'dir',]
  props = ['none', 'modified', 'modified', 'none', 'none', 'none',]

  svntest.actions.run_and_verify_diff_summarize_xml(
    [], sbox.repo_url, paths, items, props, kinds, '-r1:2', sbox.repo_url)

def diff_file_depth_empty(sbox):
  "svn diff --depth=empty FILE_WITH_LOCAL_MODS"
  # The bug was that no diff output would be generated.  Check that some is.
  sbox.build()
  iota_path = os.path.join(sbox.wc_dir, 'iota')
  svntest.main.file_append(iota_path, "new text in iota")
  exit_code, out, err = svntest.main.run_svn(None, 'diff',
                                             '--depth', 'empty', iota_path)
  if err:
    raise svntest.Failure
  if len(out) < 4:
    raise svntest.Failure

# This used to abort with ra_serf.
def diff_wrong_extension_type(sbox):
  "'svn diff -x wc -r#' should return error"

  sbox.build(read_only = True)
  svntest.actions.run_and_verify_svn(None, [], err.INVALID_DIFF_OPTION,
                                     'diff', '-x', sbox.wc_dir, '-r', '1')

# Check the order of the arguments for an external diff tool
def diff_external_diffcmd(sbox):
  "svn diff --diff-cmd provides the correct arguments"

  sbox.build(read_only = True)
  os.chdir(sbox.wc_dir)

  iota_path = 'iota'
  svntest.main.file_append(iota_path, "new text in iota")

  # Create a small diff mock object that prints its arguments to stdout.
  # (This path needs an explicit directory component to avoid searching.)
  diff_script_path = os.path.join('.', 'diff')
  # TODO: make the create function return the actual script name, and rename
  # it to something more generic.
  svntest.main.create_python_hook_script(diff_script_path, 'import sys\n'
    'for arg in sys.argv[1:]:\n  print(arg)\n')
  if sys.platform == 'win32':
    diff_script_path = "%s.bat" % diff_script_path

  expected_output = svntest.verify.ExpectedOutput([
    "Index: iota\n",
    "===================================================================\n",
    "-u\n",
    "-L\n",
    "iota\t(revision 1)\n",
    "-L\n",
    "iota\t(working copy)\n",
    os.path.abspath(svntest.wc.text_base_path("iota")) + "\n",
    os.path.abspath("iota") + "\n"])

  # Check that the output of diff corresponds with the expected arguments,
  # in the correct order.
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '--diff-cmd', diff_script_path,
                                     iota_path)


#----------------------------------------------------------------------
# Diffing an unrelated repository URL against working copy with
# local modifications (i.e. not committed). This is issue #3295 (diff
# local changes against arbitrary URL@REV ignores local add).

# Helper
def make_file_edit_del_add(dir):
  "make a file mod (M), a deletion (D) and an addition (A)."
  alpha = os.path.join(dir, 'B', 'E', 'alpha')
  beta = os.path.join(dir, 'B', 'E', 'beta')
  theta = os.path.join(dir, 'B', 'E', 'theta')

  # modify alpha, remove beta and add theta.
  svntest.main.file_append(alpha, "Edited file alpha.\n")
  svntest.main.run_svn(None, 'remove', beta)
  svntest.main.file_append(theta, "Created file theta.\n")

  svntest.main.run_svn(None, 'add', theta)


@XFail()
@Issue(3295)
def diff_url_against_local_mods(sbox):
  "diff URL against working copy with local mods"

  sbox.build()
  os.chdir(sbox.wc_dir)

  A = 'A'
  A_url = sbox.repo_url + '/A'

  # First, just make a copy.
  A2 = 'A2'
  A2_url = sbox.repo_url + '/A2'

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', '-m', 'log msg',
                                     A_url, A2_url)

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up')

  # In A, add, remove and change a file, and commit.
  make_file_edit_del_add(A);
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'committing A')

  # In A2, do the same changes but leave uncommitted.
  make_file_edit_del_add(A2);

  # Diff URL of A against working copy of A2. Output should be empty.
  expected_output = []
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', '--old', A_url, '--new', A2)


#----------------------------------------------------------------------
# Diff against old revision of the parent directory of a removed and
# locally re-added file.
@Issue(3797)
def diff_preexisting_rev_against_local_add(sbox):
  "diff -r1 of dir with removed-then-readded file"
  sbox.build()
  os.chdir(sbox.wc_dir)

  beta = os.path.join('A', 'B', 'E', 'beta')

  # remove
  svntest.main.run_svn(None, 'remove', beta)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'removing beta')

  # re-add, without committing
  svntest.main.file_append(beta, "Re-created file beta.\n")
  svntest.main.run_svn(None, 'add', beta)

  # diff against -r1, the diff should show both removal and re-addition
  exit_code, diff_output, err_output = svntest.main.run_svn(
                        None, 'diff', '-r1', 'A')

  verify_expected_output(diff_output, "-This is the file 'beta'.")
  verify_expected_output(diff_output, "+Re-created file beta.")

def diff_git_format_wc_wc(sbox):
  "create a diff in git unidiff format for wc-wc"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  new_path = os.path.join(wc_dir, 'new')
  lambda_path = os.path.join(wc_dir, 'A', 'B', 'lambda')
  lambda_copied_path = os.path.join(wc_dir, 'A', 'B', 'lambda_copied')
  alpha_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha')
  alpha_copied_path = os.path.join(wc_dir, 'A', 'B', 'E', 'alpha_copied')

  svntest.main.file_append(iota_path, "Changed 'iota'.\n")
  svntest.main.file_append(new_path, "This is the file 'new'.\n")
  svntest.main.run_svn(None, 'add', new_path)
  svntest.main.run_svn(None, 'rm', mu_path)
  svntest.main.run_svn(None, 'cp', lambda_path, lambda_copied_path)
  svntest.main.run_svn(None, 'cp', alpha_path, alpha_copied_path)
  svntest.main.file_append(alpha_copied_path, "This is a copy of 'alpha'.\n")

  ### We're not testing moved paths

  expected_output = make_git_diff_header(lambda_copied_path,
                                         "A/B/lambda_copied",
                                         "revision 1", "working copy",
                                         copyfrom_path="A/B/lambda", cp=True,
                                         text_changes=False) \
  + make_git_diff_header(mu_path, "A/mu", "revision 1",
                                         "working copy",
                                         delete=True) + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'mu'.\n",
  ] + make_git_diff_header(alpha_copied_path, "A/B/E/alpha_copied",
                         "revision 0", "working copy",
                         copyfrom_path="A/B/E/alpha", cp=True,
                         text_changes=True) + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'alpha'.\n",
    "+This is a copy of 'alpha'.\n",
  ] + make_git_diff_header(new_path, "new", "revision 0",
                           "working copy", add=True) + [
    "@@ -0,0 +1 @@\n",
    "+This is the file 'new'.\n",
  ] +  make_git_diff_header(iota_path, "iota", "revision 1",
                            "working copy") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Changed 'iota'.\n",
  ]

  expected = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected, [], 'diff',
                                     '--git', wc_dir)

@Issue(4294)
def diff_git_format_wc_wc_dir_mv(sbox):
  "create a diff in git unidff format for wc dir mv"
  sbox.build()
  wc_dir = sbox.wc_dir
  g_path = sbox.ospath('A/D/G')
  g2_path = sbox.ospath('A/D/G2')
  pi_path = sbox.ospath('A/D/G/pi')
  rho_path = sbox.ospath('A/D/G/rho')
  tau_path = sbox.ospath('A/D/G/tau')
  new_pi_path = sbox.ospath('A/D/G2/pi')
  new_rho_path = sbox.ospath('A/D/G2/rho')
  new_tau_path = sbox.ospath('A/D/G2/tau')

  svntest.main.run_svn(None, 'mv', g_path, g2_path)

  expected_output = make_git_diff_header(pi_path, "A/D/G/pi",
                                         "revision 1", "working copy",
                                         delete=True) \
  + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'pi'.\n"
  ] + make_git_diff_header(rho_path, "A/D/G/rho",
                           "revision 1", "working copy",
                           delete=True) \
  + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'rho'.\n"
  ] + make_git_diff_header(tau_path, "A/D/G/tau",
                           "revision 1", "working copy",
                           delete=True) \
  + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'tau'.\n"
  ] + make_git_diff_header(new_pi_path, "A/D/G2/pi", None, None, cp=True,
                           copyfrom_path="A/D/G/pi", text_changes=False) \
  + make_git_diff_header(new_rho_path, "A/D/G2/rho", None, None, cp=True,
                         copyfrom_path="A/D/G/rho", text_changes=False) \
  + make_git_diff_header(new_tau_path, "A/D/G2/tau", None, None, cp=True,
                         copyfrom_path="A/D/G/tau", text_changes=False)

  expected = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected, [], 'diff',
                                     '--git', wc_dir)

def diff_git_format_url_wc(sbox):
  "create a diff in git unidiff format for url-wc"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  new_path = os.path.join(wc_dir, 'new')
  svntest.main.file_append(iota_path, "Changed 'iota'.\n")
  svntest.main.file_append(new_path, "This is the file 'new'.\n")
  svntest.main.run_svn(None, 'add', new_path)
  svntest.main.run_svn(None, 'rm', mu_path)

  ### We're not testing copied or moved paths

  svntest.main.run_svn(None, 'commit', '-m', 'Committing changes', wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  expected_output = make_git_diff_header(new_path, "new", "revision 0",
                                         "revision 2", add=True) + [
    "@@ -0,0 +1 @@\n",
    "+This is the file 'new'.\n",
  ] + make_git_diff_header(mu_path, "A/mu", "revision 1", "working copy",
                           delete=True) + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'mu'.\n",
  ] +  make_git_diff_header(iota_path, "iota", "revision 1",
                            "working copy") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Changed 'iota'.\n",
  ]

  expected = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected, [], 'diff',
                                     '--git',
                                     '--old', repo_url + '@1', '--new',
                                     wc_dir)

def diff_git_format_url_url(sbox):
  "create a diff in git unidiff format for url-url"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url
  iota_path = os.path.join(wc_dir, 'iota')
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  new_path = os.path.join(wc_dir, 'new')
  svntest.main.file_append(iota_path, "Changed 'iota'.\n")
  svntest.main.file_append(new_path, "This is the file 'new'.\n")
  svntest.main.run_svn(None, 'add', new_path)
  svntest.main.run_svn(None, 'rm', mu_path)

  ### We're not testing copied or moved paths. When we do, we will not be
  ### able to identify them as copies/moves until we have editor-v2.

  svntest.main.run_svn(None, 'commit', '-m', 'Committing changes', wc_dir)
  svntest.main.run_svn(None, 'up', wc_dir)

  expected_output = make_git_diff_header("A/mu", "A/mu", "revision 1",
                                         "revision 2",
                                         delete=True) + [
    "@@ -1 +0,0 @@\n",
    "-This is the file 'mu'.\n",
    ] + make_git_diff_header("new", "new", "revision 0", "revision 2",
                             add=True) + [
    "@@ -0,0 +1 @@\n",
    "+This is the file 'new'.\n",
  ] +  make_git_diff_header("iota", "iota", "revision 1",
                            "revision 2") + [
    "@@ -1 +1,2 @@\n",
    " This is the file 'iota'.\n",
    "+Changed 'iota'.\n",
  ]

  expected = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected, [], 'diff',
                                     '--git',
                                     '--old', repo_url + '@1', '--new',
                                     repo_url + '@2')

# Regression test for an off-by-one error when printing intermediate context
# lines.
def diff_prop_missing_context(sbox):
  "diff for property has missing context"
  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  prop_val = "".join([
       "line 1\n",
       "line 2\n",
       "line 3\n",
       "line 4\n",
       "line 5\n",
       "line 6\n",
       "line 7\n",
     ])
  svntest.main.run_svn(None,
                       "propset", "prop", prop_val, iota_path)

  expected_output = svntest.wc.State(wc_dir, {
      'iota'    : Item(verb='Sending'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  prop_val = "".join([
               "line 3\n",
               "line 4\n",
               "line 5\n",
               "line 6\n",
             ])
  svntest.main.run_svn(None,
                       "propset", "prop", prop_val, iota_path)
  expected_output = make_diff_header(iota_path, 'revision 2',
                                     'working copy') + \
                    make_diff_prop_header(iota_path) + [
    "Modified: prop\n",
    "## -1,7 +1,4 ##\n",
    "-line 1\n",
    "-line 2\n",
    " line 3\n",
    " line 4\n",
    " line 5\n",
    " line 6\n",
    "-line 7\n",
  ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', iota_path)

def diff_prop_multiple_hunks(sbox):
  "diff for property with multiple hunks"
  sbox.build()
  wc_dir = sbox.wc_dir

  iota_path = os.path.join(wc_dir, 'iota')
  prop_val = "".join([
       "line 1\n",
       "line 2\n",
       "line 3\n",
       "line 4\n",
       "line 5\n",
       "line 6\n",
       "line 7\n",
       "line 8\n",
       "line 9\n",
       "line 10\n",
       "line 11\n",
       "line 12\n",
       "line 13\n",
     ])
  svntest.main.run_svn(None,
                       "propset", "prop", prop_val, iota_path)

  expected_output = svntest.wc.State(wc_dir, {
      'iota'    : Item(verb='Sending'),
      })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  prop_val = "".join([
               "line 1\n",
               "line 2\n",
               "line 3\n",
               "Add a line here\n",
               "line 4\n",
               "line 5\n",
               "line 6\n",
               "line 7\n",
               "line 8\n",
               "line 9\n",
               "line 10\n",
               "And add a line here\n",
               "line 11\n",
               "line 12\n",
               "line 13\n",
             ])
  svntest.main.run_svn(None,
                       "propset", "prop", prop_val, iota_path)
  expected_output = make_diff_header(iota_path, 'revision 2',
                                     'working copy') + \
                    make_diff_prop_header(iota_path) + [
    "Modified: prop\n",
    "## -1,6 +1,7 ##\n",
    " line 1\n",
    " line 2\n",
    " line 3\n",
    "+Add a line here\n",
    " line 4\n",
    " line 5\n",
    " line 6\n",
    "## -8,6 +9,7 ##\n",
    " line 8\n",
    " line 9\n",
    " line 10\n",
    "+And add a line here\n",
    " line 11\n",
    " line 12\n",
    " line 13\n",
  ]

  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'diff', iota_path)
def diff_git_empty_files(sbox):
  "create a diff in git format for empty files"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  new_path = os.path.join(wc_dir, 'new')
  svntest.main.file_write(iota_path, "")

  # Now commit the local mod, creating rev 2.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  svntest.main.file_write(new_path, "")
  svntest.main.run_svn(None, 'add', new_path)
  svntest.main.run_svn(None, 'rm', iota_path)

  expected_output = make_git_diff_header(new_path, "new", "revision 0",
                                         "working copy",
                                         add=True, text_changes=False) + [
  ] + make_git_diff_header(iota_path, "iota", "revision 2", "working copy",
                           delete=True, text_changes=False)

  # Two files in diff may be in any order.
  expected_output = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected_output, [], 'diff',
                                     '--git', wc_dir)

def diff_git_with_props(sbox):
  "create a diff in git format showing prop changes"
  sbox.build()
  wc_dir = sbox.wc_dir
  iota_path = os.path.join(wc_dir, 'iota')
  new_path = os.path.join(wc_dir, 'new')
  svntest.main.file_write(iota_path, "")

  # Now commit the local mod, creating rev 2.
  expected_output = svntest.wc.State(wc_dir, {
    'iota' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'iota' : Item(status='  ', wc_rev=2),
    })

  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  svntest.main.file_write(new_path, "")
  svntest.main.run_svn(None, 'add', new_path)
  svntest.main.run_svn(None, 'propset', 'svn:eol-style', 'native', new_path)
  svntest.main.run_svn(None, 'propset', 'svn:keywords', 'Id', iota_path)

  expected_output = make_git_diff_header(new_path, "new",
                                         "revision 0", "working copy",
                                         add=True, text_changes=False) + \
                    make_diff_prop_header("new") + \
                    make_diff_prop_added("svn:eol-style", "native") + \
                    make_git_diff_header(iota_path, "iota",
                                         "revision 1", "working copy",
                                         text_changes=False) + \
                    make_diff_prop_header("iota") + \
                    make_diff_prop_added("svn:keywords", "Id")

  # Files in diff may be in any order.
  expected_output = svntest.verify.UnorderedOutput(expected_output)

  svntest.actions.run_and_verify_svn(None, expected_output, [], 'diff',
                                     '--git', wc_dir)

def diff_git_with_props_on_dir(sbox):
  "diff in git format showing prop changes on dir"
  sbox.build()
  wc_dir = sbox.wc_dir

  # Now commit the local mod, creating rev 2.
  expected_output = svntest.wc.State(wc_dir, {
    '.' : Item(verb='Sending'),
    })

  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    '' : Item(status='  ', wc_rev=2),
    })

  svntest.main.run_svn(None, 'ps', 'a','b', wc_dir)
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  was_cwd = os.getcwd()
  os.chdir(wc_dir)
  expected_output = make_git_diff_header(".", "", "revision 1",
                                         "revision 2",
                                         add=False, text_changes=False) + \
                    make_diff_prop_header("") + \
                    make_diff_prop_added("a", "b")

  svntest.actions.run_and_verify_svn(None, expected_output, [], 'diff',
                                     '-c2', '--git')
  os.chdir(was_cwd)

@Issue(3826)
def diff_abs_localpath_from_wc_folder(sbox):
  "diff absolute localpath from wc folder"
  sbox.build(read_only = True)
  wc_dir = sbox.wc_dir

  A_path = os.path.join(wc_dir, 'A')
  B_abs_path = os.path.abspath(os.path.join(wc_dir, 'A', 'B'))
  os.chdir(os.path.abspath(A_path))
  svntest.actions.run_and_verify_svn(None, None, [], 'diff', B_abs_path)

@Issue(3449)
def no_spurious_conflict(sbox):
  "no spurious conflict on update"
  sbox.build()
  wc_dir = sbox.wc_dir

  svntest.actions.do_sleep_for_timestamps()

  data_dir = os.path.join(os.path.dirname(sys.argv[0]), 'diff_tests_data')
  shutil.copyfile(os.path.join(data_dir, '3449_spurious_v1'),
                  sbox.ospath('3449_spurious'))
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'add', sbox.ospath('3449_spurious'))
  sbox.simple_commit()
  shutil.copyfile(os.path.join(data_dir, '3449_spurious_v2'),
                  sbox.ospath('3449_spurious'))
  sbox.simple_commit()
  shutil.copyfile(os.path.join(data_dir, '3449_spurious_v3'),
                  sbox.ospath('3449_spurious'))
  sbox.simple_commit()

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'update', '-r2', wc_dir)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'merge', '-c4', '^/', wc_dir)

  expected_status = svntest.actions.get_virginal_state(wc_dir, 2)
  expected_status.tweak('', status=' M')
  expected_status.add({
      '3449_spurious' : Item(status='M ', wc_rev=2),
      })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # This update produces a conflict in 1.6
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'update', '--accept', 'postpone', wc_dir)
  expected_status.tweak(wc_rev=4)
  expected_status.tweak('3449_spurious', status='  ')
  svntest.actions.run_and_verify_status(wc_dir, expected_status)


########################################################################
#Run the tests


# list all tests here, starting with None:
test_list = [ None,
              diff_update_a_file,
              diff_add_a_file,
              diff_add_a_file_in_a_subdir,
              diff_replace_a_file,
              diff_multiple_reverse,
              diff_non_recursive,
              diff_repo_subset,
              diff_non_version_controlled_file,
              diff_pure_repository_update_a_file,
              diff_only_property_change,
              dont_diff_binary_file,
              diff_nonextant_urls,
              diff_head_of_moved_file,
              diff_base_to_repos,
              diff_deleted_in_head,
              diff_targets,
              diff_branches,
              diff_repos_and_wc,
              diff_file_urls,
              diff_prop_change_local_edit,
              check_for_omitted_prefix_in_path_component,
              diff_renamed_file,
              diff_within_renamed_dir,
              diff_prop_on_named_dir,
              diff_keywords,
              diff_force,
              diff_schedule_delete,
              diff_renamed_dir,
              diff_property_changes_to_base,
              diff_mime_type_changes,
              diff_prop_change_local_propmod,
              diff_repos_wc_add_with_props,
              diff_nonrecursive_checkout_deleted_dir,
              diff_repos_working_added_dir,
              diff_base_repos_moved,
              diff_added_subtree,
              basic_diff_summarize,
              diff_weird_author,
              diff_ignore_whitespace,
              diff_ignore_eolstyle,
              diff_in_renamed_folder,
              diff_with_depth,
              diff_ignore_eolstyle_empty_lines,
              diff_backward_repos_wc_copy,
              diff_summarize_xml,
              diff_file_depth_empty,
              diff_wrong_extension_type,
              diff_external_diffcmd,
              diff_url_against_local_mods,
              diff_preexisting_rev_against_local_add,
              diff_git_format_wc_wc,
              diff_git_format_url_wc,
              diff_git_format_url_url,
              diff_prop_missing_context,
              diff_prop_multiple_hunks,
              diff_git_empty_files,
              diff_git_with_props,
              diff_git_with_props_on_dir,
              diff_abs_localpath_from_wc_folder,
              no_spurious_conflict,
              diff_git_format_wc_wc_dir_mv,
              ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
