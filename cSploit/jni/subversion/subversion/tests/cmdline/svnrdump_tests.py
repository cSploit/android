#!/usr/bin/env python
#
#  svnrdump_tests.py: Tests svnrdump's remote repository dumping capabilities.
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
import sys, os
import re

# Our testing module
import svntest
from svntest.verify import SVNUnexpectedStdout, SVNUnexpectedStderr
from svntest.verify import SVNExpectedStderr
from svntest.main import write_restrictive_svnserve_conf
from svntest.main import server_has_partial_replay
from svnadmin_tests import test_create

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem

## Mismatched headers during dumping operation
# Text-copy-source-* and *-sha1 headers are not provided by the RA
# layer. `svnadmin dump` is able to provide them because it works on
# the FS layer. Also, svnrdump attaches "Prop-delta: true" with
# everything whether it's really a delta or a new prop (delta from
# /dev/null). This is really harmless, but `svnadmin dump` contains
# the logic for differentiating between these two cases.

mismatched_headers_re = re.compile(
    "Prop-delta: .*|Text-content-sha1: .*|Text-copy-source-md5: .*|"
    "Text-copy-source-sha1: .*|Text-delta-base-sha1: .*"
)

######################################################################
# Helper routines

def build_repos(sbox):
  """Build an empty sandbox repository"""

  # Cleanup after the last run by removing any left-over repository.
  svntest.main.safe_rmtree(sbox.repo_dir)

  # Create an empty repository.
  svntest.main.create_repos(sbox.repo_dir)

def compare_repos_dumps(svnrdump_sbox, svnadmin_dumpfile):
  """Compare two dumpfiles, one created from SVNRDUMP_SBOX, and other given
  by SVNADMIN_DUMPFILE.  The dumpfiles do not need to match linewise, as the
  SVNADMIN_DUMPFILE contents will first be loaded into a repository and then
  re-dumped to do the match, which should generate the same dumpfile as
  dumping SVNRDUMP_SBOX."""

  svnrdump_contents = svntest.actions.run_and_verify_dump(
                                                    svnrdump_sbox.repo_dir)

  svnadmin_sbox = svnrdump_sbox.clone_dependent()
  svntest.main.safe_rmtree(svnadmin_sbox.repo_dir)
  svntest.main.create_repos(svnadmin_sbox.repo_dir)

  svntest.actions.run_and_verify_load(svnadmin_sbox.repo_dir, svnadmin_dumpfile)

  svnadmin_contents = svntest.actions.run_and_verify_dump(
                                                    svnadmin_sbox.repo_dir)

  svntest.verify.compare_dump_files(
    "Dump files", "DUMP", svnadmin_contents, svnrdump_contents)

def run_dump_test(sbox, dumpfile_name, expected_dumpfile_name = None,
                  subdir = None, bypass_prop_validation = False,
                  ignore_base_checksums = False):
  """Load a dumpfile using 'svnadmin load', dump it with 'svnrdump
  dump' and check that the same dumpfile is produced or that
  expected_dumpfile_name is produced if provided. Additionally, the
  subdir argument appends itself to the URL"""

  # Create an empty sanbox repository
  build_repos(sbox)

  # This directory contains all the dump files
  svnrdump_tests_dir = os.path.join(os.path.dirname(sys.argv[0]),
                                   'svnrdump_tests_data')

  # Load the specified dump file into the sbox repository using
  # svnadmin load
  svnadmin_dumpfile = open(os.path.join(svnrdump_tests_dir,
                                        dumpfile_name),
                           'rb').readlines()

  svntest.actions.run_and_verify_load(sbox.repo_dir, svnadmin_dumpfile,
                                      bypass_prop_validation)

  repo_url = sbox.repo_url
  if subdir:
    repo_url = repo_url + subdir

  # Create a dump file using svnrdump
  svnrdump_dumpfile = \
      svntest.actions.run_and_verify_svnrdump(None, svntest.verify.AnyOutput,
                                              [], 0, '-q', 'dump',
                                              repo_url)

  if expected_dumpfile_name:
    svnadmin_dumpfile = open(os.path.join(svnrdump_tests_dir,
                                          expected_dumpfile_name),
                             'rb').readlines()
    # Compare the output from stdout
    if ignore_base_checksums:
      svnadmin_dumpfile = [l for l in svnadmin_dumpfile
                                    if not l.startswith('Text-delta-base-md5')]
      svnrdump_dumpfile = [l for l in svnrdump_dumpfile
                                    if not l.startswith('Text-delta-base-md5')]

    svnadmin_dumpfile = svntest.verify.UnorderedOutput(svnadmin_dumpfile)

    svntest.verify.compare_and_display_lines(
      "Dump files", "DUMP", svnadmin_dumpfile, svnrdump_dumpfile,
      None, mismatched_headers_re)
    
  else:
    compare_repos_dumps(sbox, svnadmin_dumpfile)

def run_load_test(sbox, dumpfile_name, expected_dumpfile_name = None,
                  expect_deltas = True):
  """Load a dumpfile using 'svnrdump load', dump it with 'svnadmin
  dump' and check that the same dumpfile is produced"""

  # Create an empty sandbox repository
  build_repos(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # This directory contains all the dump files
  svnrdump_tests_dir = os.path.join(os.path.dirname(sys.argv[0]),
                                   'svnrdump_tests_data')

  # Load the specified dump file into the sbox repository using
  # svnrdump load
  svnrdump_dumpfile = open(os.path.join(svnrdump_tests_dir,
                                        dumpfile_name),
                           'rb').readlines()

  # Set the UUID of the sbox repository to the UUID specified in the
  # dumpfile ### RA layer doesn't have a set_uuid functionality
  uuid = svnrdump_dumpfile[2].split(' ')[1][:-1]
  svntest.actions.run_and_verify_svnadmin2("Setting UUID", None, None, 0,
                                           'setuuid', sbox.repo_dir,
                                           uuid)

  svntest.actions.run_and_verify_svnrdump(svnrdump_dumpfile,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  # Create a dump file using svnadmin dump
  svnadmin_dumpfile = svntest.actions.run_and_verify_dump(sbox.repo_dir,
                                                          expect_deltas)

  if expected_dumpfile_name:
    svnrdump_dumpfile = open(os.path.join(svnrdump_tests_dir,
                                          expected_dumpfile_name),
                             'rb').readlines()

    # Compare the output from stdout
    svntest.verify.compare_and_display_lines(
      "Dump files", "DUMP", svnrdump_dumpfile, svnadmin_dumpfile)

  else:
    compare_repos_dumps(sbox, svnrdump_dumpfile)

######################################################################
# Tests

@Skip(svntest.main.is_ra_type_dav_serf)
def basic_dump(sbox):
  "dump: standard sbox repos"
  sbox.build(read_only = True, create_wc = False)

  out = \
      svntest.actions.run_and_verify_svnrdump(None, svntest.verify.AnyOutput,
                                              [], 0, '-q', 'dump',
                                              sbox.repo_url)

  if not out[0].startswith('SVN-fs-dump-format-version:'):
    raise svntest.Failure('No valid output')

@Skip(svntest.main.is_ra_type_dav_serf)
def revision_0_dump(sbox):
  "dump: revision zero"
  run_dump_test(sbox, "revision-0.dump")

def revision_0_load(sbox):
  "load: revision zero"
  run_load_test(sbox, "revision-0.dump")

# skeleton.dump repository layout
#
#   Projects/       (Added r1)
#     README        (Added r2)
#     Project-X     (Added r3)
#     Project-Y     (Added r4)
#     Project-Z     (Added r5)
#     docs/         (Added r6)
#       README      (Added r6)

@Skip(svntest.main.is_ra_type_dav_serf)
def skeleton_dump(sbox):
  "dump: skeleton repository"
  run_dump_test(sbox, "skeleton.dump")

def skeleton_load(sbox):
  "load: skeleton repository"
  run_load_test(sbox, "skeleton.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def sparse_propchanges_dump(sbox):
  "dump: sparse file/dir propchanges"
  run_dump_test(sbox, "sparse-propchanges.dump")

@Issue(3902)
def sparse_propchanges_load(sbox):
  "load: sparse file/dir propchanges"
  run_load_test(sbox, "sparse-propchanges.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def copy_and_modify_dump(sbox):
  "dump: copy and modify"
  run_dump_test(sbox, "copy-and-modify.dump")

def copy_and_modify_load(sbox):
  "load: copy and modify"
  run_load_test(sbox, "copy-and-modify.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def no_author_dump(sbox):
  "dump: copy revs with no svn:author revprops"
  run_dump_test(sbox, "no-author.dump")

def no_author_load(sbox):
  "load: copy revs with no svn:author revprops"
  run_load_test(sbox, "no-author.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def copy_from_previous_version_and_modify_dump(sbox):
  "dump: copy from previous version and modify"
  run_dump_test(sbox, "copy-from-previous-version-and-modify.dump")

def copy_from_previous_version_and_modify_load(sbox):
  "load: copy from previous version and modify"
  run_load_test(sbox, "copy-from-previous-version-and-modify.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def modified_in_place_dump(sbox):
  "dump: modified in place"
  run_dump_test(sbox, "modified-in-place.dump")

def modified_in_place_load(sbox):
  "load: modified in place"
  run_load_test(sbox, "modified-in-place.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def move_and_modify_in_the_same_revision_dump(sbox):
  "dump: move parent & modify child file in same rev"
  run_dump_test(sbox, "move-and-modify.dump")

def move_and_modify_in_the_same_revision_load(sbox):
  "load: move parent & modify child file in same rev"
  run_load_test(sbox, "move-and-modify.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def tag_empty_trunk_dump(sbox):
  "dump: tag empty trunk"
  run_dump_test(sbox, "tag-empty-trunk.dump")

def tag_empty_trunk_load(sbox):
  "load: tag empty trunk"
  run_load_test(sbox, "tag-empty-trunk.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def tag_trunk_with_file_dump(sbox):
  "dump: tag trunk containing a file"
  run_dump_test(sbox, "tag-trunk-with-file.dump")

def tag_trunk_with_file_load(sbox):
  "load: tag trunk containing a file"
  run_load_test(sbox, "tag-trunk-with-file.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def tag_trunk_with_file2_dump(sbox):
  "dump: tag trunk containing a file (#2)"
  run_dump_test(sbox, "tag-trunk-with-file2.dump")

def tag_trunk_with_file2_load(sbox):
  "load: tag trunk containing a file (#2)"
  run_load_test(sbox, "tag-trunk-with-file2.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def dir_prop_change_dump(sbox):
  "dump: directory property changes"
  run_dump_test(sbox, "dir-prop-change.dump")

def dir_prop_change_load(sbox):
  "load: directory property changes"
  run_load_test(sbox, "dir-prop-change.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def copy_parent_modify_prop_dump(sbox):
  "dump: copy parent and modify prop"
  run_dump_test(sbox, "copy-parent-modify-prop.dump")

def copy_parent_modify_prop_load(sbox):
  "load: copy parent and modify prop"
  run_load_test(sbox, "copy-parent-modify-prop.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def copy_revprops_dump(sbox):
  "dump: copy revprops other than svn:*"
  run_dump_test(sbox, "revprops.dump")

def copy_revprops_load(sbox):
  "load: copy revprops other than svn:*"
  run_load_test(sbox, "revprops.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def only_trunk_dump(sbox):
  "dump: subdirectory"
  run_dump_test(sbox, "trunk-only.dump", subdir="/trunk",
                expected_dumpfile_name="trunk-only.expected.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def only_trunk_A_with_changes_dump(sbox):
  "dump: subdirectory with changes on root"
  run_dump_test(sbox, "trunk-A-changes.dump", subdir="/trunk/A",
           expected_dumpfile_name="trunk-A-changes.expected.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def url_encoding_dump(sbox):
  "dump: url encoding issues"
  run_dump_test(sbox, "url-encoding-bug.dump")

def url_encoding_load(sbox):
  "load: url encoding issues"
  run_load_test(sbox, "url-encoding-bug.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def copy_bad_line_endings_dump(sbox):
  "dump: inconsistent line endings in svn:* props"
  run_dump_test(sbox, "copy-bad-line-endings.dump",
                expected_dumpfile_name="copy-bad-line-endings.expected.dump",
                bypass_prop_validation=True)

@Issue(4263)
def copy_bad_line_endings_load(sbox):
  "load: inconsistent line endings in svn:* props"
  run_load_test(sbox, "copy-bad-line-endings.dump",
                expected_dumpfile_name="copy-bad-line-endings.expected.dump")
          
@Skip(svntest.main.is_ra_type_dav_serf)
def copy_bad_line_endings2_dump(sbox):
  "dump: non-LF line endings in svn:* props"
  run_dump_test(sbox, "copy-bad-line-endings2.dump",
                expected_dumpfile_name="copy-bad-line-endings2.expected.dump",
                bypass_prop_validation=True, ignore_base_checksums=True)

@Skip(svntest.main.is_ra_type_dav_serf)
def commit_a_copy_of_root_dump(sbox):
  "dump: commit a copy of root"
  run_dump_test(sbox, "repo-with-copy-of-root-dir.dump")

def commit_a_copy_of_root_load(sbox):
  "load: commit a copy of root"
  run_load_test(sbox, "repo-with-copy-of-root-dir.dump")

@Skip(svntest.main.is_ra_type_dav_serf)
def descend_into_replace_dump(sbox):
  "dump: descending into replaced dir looks in src"
  run_dump_test(sbox, "descend-into-replace.dump", subdir='/trunk/H',
                expected_dumpfile_name = "descend-into-replace.expected.dump")

def descend_into_replace_load(sbox):
  "load: descending into replaced dir looks in src"
  run_load_test(sbox, "descend-into-replace.dump")

@Issue(3847)
@Skip(svntest.main.is_ra_type_dav_serf)
def add_multi_prop_dump(sbox):
  "dump: add with multiple props"
  run_dump_test(sbox, "add-multi-prop.dump")

@Issue(3844)
def multi_prop_edit_load(sbox):
  "load: multiple prop edits on a file"
  run_load_test(sbox, "multi-prop-edits.dump", None, False)

#----------------------------------------------------------------------
# This test replicates svnadmin_tests.py 16 'reflect dropped renumbered
# revs in svn:mergeinfo' but uses 'svnrdump load' in place of
# 'svnadmin load'.
@Issue(3890)
@Skip(svntest.main.is_ra_type_dav_serf)
def reflect_dropped_renumbered_revs(sbox):
  "svnrdump renumbers dropped revs in mergeinfo"

  # Create an empty sandbox repository
  build_repos(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # Load the specified dump file into the sbox repository using
  # svnrdump load
  dump_file = open(os.path.join(os.path.dirname(sys.argv[0]),
                                                'svnrdump_tests_data',
                                                'with_merges.dump'),
                   'rb')
  svnrdump_dumpfile = dump_file.readlines()
  dump_file.close()

  # svnrdump load the dump file.
  svntest.actions.run_and_verify_svnrdump(svnrdump_dumpfile,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  # Create the 'toplevel' directory in repository and then load the same
  # dumpfile into that subtree.
  svntest.actions.run_and_verify_svn(None, ['\n', 'Committed revision 10.\n'],
                                    [], "mkdir", sbox.repo_url + "/toplevel",
                                     "-m", "Create toplevel dir to load into")
  svntest.actions.run_and_verify_svnrdump(svnrdump_dumpfile,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load',
                                          sbox.repo_url + "/toplevel")
  # Verify the svn:mergeinfo properties
  url = sbox.repo_url
  expected_output = svntest.verify.UnorderedOutput([
    url + "/trunk - /branch1:4-8\n",
    url + "/toplevel/trunk - /toplevel/branch1:14-18\n",
    ])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propget', 'svn:mergeinfo', '-R',
                                     sbox.repo_url)

#----------------------------------------------------------------------
# Full or incremental dump-load cycles should result in the same
# mergeinfo in the loaded repository.
#
# Given a repository 'SOURCE-REPOS' with mergeinfo, and a repository
# 'TARGET-REPOS' (which may or may not be empty), either of the following
# methods to move 'SOURCE-REPOS' to 'TARGET-REPOS' should result in
# the same mergeinfo on 'TARGET-REPOS':
#
#   1) Dump -r1:HEAD from 'SOURCE-REPOS' and load it in one shot to
#      'TARGET-REPOS'.
#
#   2) Dump 'SOURCE-REPOS' in a series of incremental dumps and load
#      each of them to 'TARGET-REPOS'.
#
# See http://subversion.tigris.org/issues/show_bug.cgi?id=3020#desc13
#
# This test replicates svnadmin_tests.py 20 'don't filter mergeinfo revs
# from incremental dump' but uses 'svnrdump [dump|load]' in place of
# 'svnadmin [dump|load]'.
@Issue(3890)
@Skip(svntest.main.is_ra_type_dav_serf)
def dont_drop_valid_mergeinfo_during_incremental_svnrdump_loads(sbox):
  "don't drop mergeinfo revs in incremental svnrdump"

  # Create an empty repos.
  test_create(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # PART 1: Load a full dump to an empty repository.
  #
  # The test repository used here, 'mergeinfo_included_full.dump', is
  # this repos:
  #                       __________________________________________
  #                      |                                         |
  #                      |             ____________________________|_____
  #                      |            |                            |     |
  # trunk---r2---r3-----r5---r6-------r8---r9--------------->      |     |
  #   r1             |        |     |       |                      |     |
  # intial           |        |     |       |______                |     |
  # import         copy       |   copy             |            merge   merge
  #                  |        |     |            merge           (r5)   (r8)
  #                  |        |     |            (r9)              |     |
  #                  |        |     |              |               |     |
  #                  |        |     V              V               |     |
  #                  |        | branches/B2-------r11---r12---->   |     |
  #                  |        |     r7              |____|         |     |
  #                  |        |                        |           |     |
  #                  |      merge                      |___        |     |
  #                  |      (r6)                           |       |     |
  #                  |        |_________________           |       |     |
  #                  |                          |        merge     |     |
  #                  |                          |      (r11-12)    |     |
  #                  |                          |          |       |     |
  #                  V                          V          V       |     |
  #              branches/B1-------------------r10--------r13-->   |     |
  #                  r4                                            |     |
  #                   |                                            V     V
  #                  branches/B1/B/E------------------------------r14---r15->
  #
  #
  # The mergeinfo on this repos@15 is:
  #
  #   Properties on 'branches/B1':
  #     svn:mergeinfo
  #       /branches/B2:11-12
  #       /trunk:6,9
  #   Properties on 'branches/B1/B/E':
  #     svn:mergeinfo
  #       /branches/B2/B/E:11-12
  #       /trunk/B/E:5-6,8-9
  #   Properties on 'branches/B2':
  #     svn:mergeinfo
  #       /trunk:9
  dump_fp = open(os.path.join(os.path.dirname(sys.argv[0]),
                              'svnrdump_tests_data',
                              'mergeinfo_included_full.dump'),
                 'rb')
  dumpfile_full = dump_fp.readlines()
  dump_fp.close()

  svntest.actions.run_and_verify_svnrdump(dumpfile_full,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  # Check that the mergeinfo is as expected.
  url = sbox.repo_url + '/branches/'
  expected_output = svntest.verify.UnorderedOutput([
    url + "B1 - /branches/B2:11-12\n",
    "/trunk:6,9\n",
    url + "B2 - /trunk:9\n",
    url + "B1/B/E - /branches/B2/B/E:11-12\n",
    "/trunk/B/E:5-6,8-9\n"])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propget', 'svn:mergeinfo', '-R',
                                     sbox.repo_url)

  # PART 2: Load a series of incremental dumps to an empty repository.
  #
  # Incrementally dump the repository into three dump files:
  dump_file_r1_10 = svntest.main.temp_dir + "-r1-10.dump"
  output = svntest.actions.run_and_verify_svnrdump(None,
                                                   svntest.verify.AnyOutput,
                                                   [], 0, '-q', 'dump', '-r1:10',
                                                   sbox.repo_url)
  dump_fp = open(dump_file_r1_10, 'wb')
  dump_fp.writelines(output)
  dump_fp.close()

  dump_file_r11_13 = svntest.main.temp_dir + "-r11-13.dump"
  output = svntest.actions.run_and_verify_svnrdump(None,
                                                   svntest.verify.AnyOutput,
                                                   [], 0, '-q', 'dump',
                                                   '--incremental', '-r11:13',
                                                   sbox.repo_url)
  dump_fp = open(dump_file_r11_13, 'wb')
  dump_fp.writelines(output)
  dump_fp.close()

  dump_file_r14_15 = svntest.main.temp_dir + "-r14-15.dump"
  output = svntest.actions.run_and_verify_svnrdump(None,
                                                   svntest.verify.AnyOutput,
                                                   [], 0, '-q', 'dump',
                                                   '--incremental', '-r14:15',
                                                   sbox.repo_url)
  dump_fp = open(dump_file_r14_15, 'wb')
  dump_fp.writelines(output)
  dump_fp.close()

  # Blow away the current repos and create an empty one in its place.
  test_create(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # Load the three incremental dump files in sequence.
  dump_fp = open(dump_file_r1_10, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)
  dump_fp.close()
  dump_fp = open(dump_file_r11_13, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)
  dump_fp.close()
  dump_fp = open(dump_file_r14_15, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)
  dump_fp.close()

  # Check the mergeinfo, we use the same expected output as before,
  # as it (duh!) should be exactly the same as when we loaded the
  # repos in one shot.
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propget', 'svn:mergeinfo', '-R',
                                     sbox.repo_url)

  # Now repeat the above two scenarios, but with an initially non-empty target
  # repository.  First, try the full dump-load in one shot.
  #
  # PART 3: Load a full dump to an non-empty repository.
  #
  # Reset our sandbox.
  test_create(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # Load this skeleton repos into the empty target:
  #
  #   Projects/       (Added r1)
  #     README        (Added r2)
  #     Project-X     (Added r3)
  #     Project-Y     (Added r4)
  #     Project-Z     (Added r5)
  #     docs/         (Added r6)
  #       README      (Added r6)
  dump_fp = open(os.path.join(os.path.dirname(sys.argv[0]),
                              'svnrdump_tests_data',
                              'skeleton.dump'),
                 'rb')
  dumpfile_skeleton = dump_fp.readlines()
  dump_fp.close()
  svntest.actions.run_and_verify_svnrdump(dumpfile_skeleton,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  # Load 'svnadmin_tests_data/mergeinfo_included_full.dump' in one shot:
  svntest.actions.run_and_verify_svnrdump(dumpfile_full,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load',
                                          sbox.repo_url + '/Projects/Project-X')

  # Check that the mergeinfo is as expected.  This is exactly the
  # same expected mergeinfo we previously checked, except that the
  # revisions are all offset +6 to reflect the revions already in
  # the skeleton target before we began loading and the leading source
  # paths are adjusted by the --parent-dir:
  #
  #   Properties on 'branches/B1':
  #     svn:mergeinfo
  #       /Projects/Project-X/branches/B2:17-18
  #       /Projects/Project-X/trunk:12,15
  #   Properties on 'branches/B1/B/E':
  #     svn:mergeinfo
  #       /Projects/Project-X/branches/B2/B/E:17-18
  #       /Projects/Project-X/trunk/B/E:11-12,14-15
  #   Properties on 'branches/B2':
  #     svn:mergeinfo
  #       /Projects/Project-X/trunk:15
  url = sbox.repo_url + '/Projects/Project-X/branches/'
  expected_output = svntest.verify.UnorderedOutput([
    url + "B1 - /Projects/Project-X/branches/B2:17-18\n",
    "/Projects/Project-X/trunk:12,15\n",
    url + "B2 - /Projects/Project-X/trunk:15\n",
    url + "B1/B/E - /Projects/Project-X/branches/B2/B/E:17-18\n",
    "/Projects/Project-X/trunk/B/E:11-12,14-15\n"])
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propget', 'svn:mergeinfo', '-R',
                                     sbox.repo_url)

  # PART 4: Load a a series of incremental dumps to an non-empty repository.
  #
  # Reset our sandbox.
  test_create(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # Load the skeleton repos into the empty target:
  svntest.actions.run_and_verify_svnrdump(dumpfile_skeleton,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  # Load the three incremental dump files in sequence.
  #
  # The first load fails the same as PART 3.
  dump_fp = open(dump_file_r1_10, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load',
                                          sbox.repo_url + '/Projects/Project-X')
  dump_fp.close()
  dump_fp = open(dump_file_r11_13, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load',
                                          sbox.repo_url + '/Projects/Project-X')
  dump_fp.close()
  dump_fp = open(dump_file_r14_15, 'rb')
  svntest.actions.run_and_verify_svnrdump(dump_fp.readlines(),
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load',
                                          sbox.repo_url + '/Projects/Project-X')
  dump_fp.close()

  # Check the resulting mergeinfo.  We expect the exact same results
  # as Part 3.
  # See http://subversion.tigris.org/issues/show_bug.cgi?id=3020#desc16.
  svntest.actions.run_and_verify_svn(None, expected_output, [],
                                     'propget', 'svn:mergeinfo', '-R',
                                     sbox.repo_url)

#----------------------------------------------------------------------
@Issue(3890)
def svnrdump_load_partial_incremental_dump(sbox):
  "svnrdump load partial incremental dump"

  # Create an empty sandbox repository
  build_repos(sbox)

  # Create the revprop-change hook for this test
  svntest.actions.enable_revprop_changes(sbox.repo_dir)

  # Create the 'A' directory in repository and then load the partial
  # incremental dump into the root of the repository.
  svntest.actions.run_and_verify_svn(None, ['\n', 'Committed revision 1.\n'],
                                    [], "mkdir", sbox.repo_url + "/A",
                                     "-m", "Create toplevel dir to load into")

  # Load the specified dump file into the sbox repository using
  # svnrdump load
  dump_file = open(os.path.join(os.path.dirname(sys.argv[0]),
                                                'svnrdump_tests_data',
                                                'partial_incremental.dump'),
                   'rb')
  svnrdump_dumpfile = dump_file.readlines()
  dump_file.close()
  svntest.actions.run_and_verify_svnrdump(svnrdump_dumpfile,
                                          svntest.verify.AnyOutput,
                                          [], 0, 'load', sbox.repo_url)

  ########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              basic_dump,
              revision_0_dump,
              revision_0_load,
              skeleton_dump,
              skeleton_load,
              sparse_propchanges_dump,
              sparse_propchanges_load,
              copy_and_modify_dump,
              copy_and_modify_load,
              copy_from_previous_version_and_modify_dump,
              copy_from_previous_version_and_modify_load,
              modified_in_place_dump,
              modified_in_place_load,
              tag_empty_trunk_dump,
              tag_empty_trunk_load,
              tag_trunk_with_file_dump,
              tag_trunk_with_file_load,
              tag_trunk_with_file2_dump,
              tag_trunk_with_file2_load,
              dir_prop_change_dump,
              dir_prop_change_load,
              copy_parent_modify_prop_dump,
              copy_parent_modify_prop_load,
              url_encoding_dump,
              url_encoding_load,
              copy_revprops_dump,
              copy_revprops_load,
              only_trunk_dump,
              only_trunk_A_with_changes_dump,
              no_author_dump,
              no_author_load,
              move_and_modify_in_the_same_revision_dump,
              move_and_modify_in_the_same_revision_load,
              copy_bad_line_endings_dump,
              copy_bad_line_endings_load,
              copy_bad_line_endings2_dump,
              commit_a_copy_of_root_dump,
              commit_a_copy_of_root_load,
              descend_into_replace_dump,
              descend_into_replace_load,
              add_multi_prop_dump,
              multi_prop_edit_load,
              reflect_dropped_renumbered_revs,
              dont_drop_valid_mergeinfo_during_incremental_svnrdump_loads,
              svnrdump_load_partial_incremental_dump,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
