#!/usr/bin/env python
#
#  tree_conflict_tests.py:  testing tree-conflict cases.
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
import sys, re, os, traceback

# Our testing module
import svntest
from svntest import main, wc, verify
from svntest.actions import run_and_verify_svn
from svntest.actions import run_and_verify_commit
from svntest.actions import run_and_verify_resolved
from svntest.actions import run_and_verify_update
from svntest.actions import run_and_verify_status
from svntest.actions import run_and_verify_info
from svntest.actions import get_virginal_state
import shutil

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = svntest.wc.StateItem
AnyOutput = svntest.verify.AnyOutput

# If verbose mode is enabled, print the LINE and a newline.
def verbose_print(line):
  if main.options.verbose:
    print(line)

# If verbose mode is enabled, print the (assumed newline-terminated) LINES.
def verbose_printlines(lines):
  if main.options.verbose:
    for line in lines:
      sys.stdout.write(line)

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.


#----------------------------------------------------------------------

# The tests in this file are for cases where a tree conflict is to be raised.
# (They do not check that conflicts are not raised in other cases.)

# Note: Delete, Replace and Move are presently tested together but probably
# will eventually need to be tested separately.

# A tree conflict being raised means:
#   - the conflict is reported initially
#   - the conflict is persistently visible
#   - the conflict blocks commits until resolved
#   - the conflict blocks (some?) further merges
# Desired:
#   - interactive conflict resolution

# A "tree conflict on file P/F" means:
#   - the operation reports action code "C" on path P/F
#   - "svn status" reports status code "C" on path P/F
#   - "svn info" reports details of the conflict on path P/F
#   - "svn commit" fails if the user-requested targets include path P/F
#   - "svn merge/update/switch" fails if it tries to modify P/F in any way

# A "tree conflict on dir P/D" means:
#   - the operation reports action code "C" on path P/D
#   - "svn status" reports status code "C" on path P/D
#   - "svn info" reports details of the conflict on P/D
#   - "svn commit" fails if it includes any part of the P/D sub-tree
#   - "svn merge/up/sw" fails if it modifies any part of the P/D sub-tree

#----------------------------------------------------------------------

# Two sets of paths. The paths to be used for the destination of a copy
# or move must differ between the incoming change and the local mods,
# otherwise scenarios involving a move onto a move would conflict on the
# destination node as well as on the source, and we only want to be testing
# one thing at a time in most tests.
def incoming_paths(root_dir, parent_dir):
  """Create a set of paths in which the victims of tree conflicts are
     children of PARENT_DIR. ROOT_DIR should be a shallower directory
     in which items "F1" and "D1" can pre-exist and be shared across
     multiple parent dirs."""
  return {
    'F1' : os.path.join(root_dir,   "F1"),
    'F'  : os.path.join(parent_dir, "F"),
    'F2' : os.path.join(parent_dir, "F2-in"),
    'F3' : os.path.join(root_dir,   "F3"),
    'D1' : os.path.join(root_dir,   "D1"),
    'D'  : os.path.join(parent_dir, "D"),
    'D2' : os.path.join(parent_dir, "D2-in"),
  }
def localmod_paths(root_dir, parent_dir):
  """Create a set of paths in which the victims of tree conflicts are
     children of PARENT_DIR. ROOT_DIR should be a shallower directory
     in which items "F1" and "D1" can pre-exist and be shared across
     multiple parent dirs."""
  return {
    'F1' : os.path.join(root_dir,   "F1"),
    'F'  : os.path.join(parent_dir, "F"),
    'F2' : os.path.join(parent_dir, "F2-local"),
    'F3' : os.path.join(root_dir,   "F3"),
    'D1' : os.path.join(root_dir,   "D1"),
    'D'  : os.path.join(parent_dir, "D"),
    'D2' : os.path.join(parent_dir, "D2-local"),
  }

# Perform the action MODACTION on the WC items given by PATHS. The
# available actions can be seen within this function.
def modify(modaction, paths, is_init=True):
  F1 = paths['F1']  # existing file to copy from
  F3 = paths['F3']  # existing file to copy from
  F  = paths['F']   # target file
  F2 = paths['F2']  # non-existing file to copy/move to
  D1 = paths['D1']  # existing dir to copy from
  D  = paths['D']   # target dir
  D2 = paths['D2']  # non-existing dir to copy/move to

  # print "  Mod: '" + modaction + "' '" + P + "'"

  if modaction == 'ft':    # file text-mod
    assert os.path.exists(F)
    main.file_append(F, "This is a text-mod of file F.\n")
  elif modaction == 'fP':  # file Prop-mod
    assert os.path.exists(F)
    main.run_svn(None, 'pset', 'fprop1', 'A prop set on file F.', F)
  elif modaction == 'dP':  # dir Prop-mod
    assert os.path.exists(D)
    main.run_svn(None, 'pset', 'dprop1', 'A prop set on dir D.', D)
  elif modaction == 'fD':  # file Delete
    assert os.path.exists(F)
    main.run_svn(None, 'del', F)
  elif modaction == 'dD':  # dir Delete
    assert os.path.exists(D)
    main.run_svn(None, 'del', D)
  elif modaction == 'fA':  # file Add (new)
    assert os.path.exists(F)
    main.run_svn(None, 'add', F)
    main.run_svn(None, 'pset', 'fprop2', 'A prop of added file F.', F)
  elif modaction == 'dA':  # dir Add (new)
    assert os.path.exists(D)
    main.run_svn(None, 'add', D)
    main.run_svn(None, 'pset', 'dprop2', 'A prop of added dir D.', D)
  elif modaction == 'fC':  # file Copy (from F1)
    if is_init:
      main.run_svn(None, 'copy', F1, F)
    else:
      main.run_svn(None, 'copy', F3, F)
  elif modaction == 'dC':  # dir Copy (from D1)
    main.run_svn(None, 'copy', D1, D)
  elif modaction == 'fM':  # file Move (to F2)
    main.run_svn(None, 'rename', F, F2)
  elif modaction == 'dM':  # dir Move (to D2)
    main.run_svn(None, 'rename', D, D2)
  elif modaction == 'fa':  # file add (new) on disk
    assert not os.path.exists(F)
    main.file_write(F, "This is file F.\n")
  elif modaction == 'da':  # dir add (new) on disk
    assert not os.path.exists(D)
    os.mkdir(D)
  elif modaction == 'fd':  # file delete from disk
    assert os.path.exists(F)
    os.remove(F)
  elif modaction == 'dd':  # dir delete from disk
    assert os.path.exists(D)
    os.remove(D)
  else:
    raise Exception("unknown modaction: '" + modaction + "'")

#----------------------------------------------------------------------

# Lists of change scenarios
#
# Each scenario expresses a change in terms of the client commands
# (including "move") that create that change. The change may exist in a
# repository, or may be applied to a WC by an "update" or "switch" or
# "merge", or may exist in a WC as a local modification.
#
# In addition, each scenario may include some local-modification actions
# that, if performed on the WC after this change, will make the disk state
# incompatible with the version-controlled state - e.g. by deleting a file
# that metadata says is present or vice-versa.

# File names:
#   F1 = any existing file
#   F3 = any existing file
#   F  = the file-path being acted on
#   F2 = any non-existent file-path
#   D1 = any existing dir
#   D  = the dir-path being acted on
#   D2 = any non-existent dir-path
#   P  = the parent dir of F and of D

# Format of a change scenario:
# (
#   list of actions to create the file/directory to be changed later,
#   list of actions to make the change
# )

# Action lists to initialise the repository with a file or directory absent
# or present, to provide the starting point from which we perform the changes
# that are to be tested.
absent_f = []
absent_d = []
create_f = ['fa','fA']
create_d = ['da','dA']

# Scenarios that start with no existing versioned item
#
# CREATE:
# file-add(F) = add-new(F)        or copy(F1,F)(and modify?)
# dir-add(D)  = add-new(D)(deep?) or copy(D1,D)(and modify?)

f_adds = [
  #( absent_f, ['fa','fA'] ), ### local add-without-history: not a tree conflict
  ( absent_f, ['fC'] ),
  ( absent_f, ['fC','ft'] ), ### Fails because update seems to assume that the
                             ### local file is unmodified (same as issue 1736?).
  #( absent_f, ['fC','fP'] ),  # don't test all combinations, just because it's slow
]
d_adds = [
  #( absent_d, ['da','dA'] ), ### local add-without-history: not a tree conflict
  ( absent_d, ['dC'] ),
  #( absent_d, ['dC','dP'] ),  # not yet
]

# Scenarios that start with an existing versioned item
#
# GO-AWAY: node is no longer at the path where it was.
# file-del(F) = del(F) or move(F,F2)
# dir-del(D)  = del(D) or move(D,D2)
#
# REPLACE: node is no longer at the path where it was, but another node is.
# file-rpl(F) = file-del(F) + file-add(F)
# dir-rpl(D)  = dir-del(D) + dir-add(D)
# Note: Schedule replace-by-different-node-type is unsupported in WC.
#
# MODIFY:
# file-mod(F) = text-mod(F) and/or prop-mod(F)
# dir-mod(D)  = prop-mod(D) and/or file-mod(child-F) and/or dir-mod(child-D)

f_dels = [
  ( create_f, ['fD'] ),
  ( create_f, ['fM'] ),
]
d_dels = [
  ( create_d, ['dD'] ),
  ( create_d, ['dM'] ),
]

f_rpls = [
  #( create_f, ['fD','fa','fA'] ),  # replacement - not yet
  #( create_f, ['fM','fa','fC'] ),  # don't test all combinations, just because it's slow
]
d_rpls = [
  #( create_d, ['dD','dA'] ),  # replacement - not yet
  #( create_d, ['dM','dC'] ),  # don't test all combinations, just because it's slow
  # Note that directory replacement differs from file replacement: the
  # schedule-delete dir is still on disk and is re-used for the re-addition.
]
f_rpl_d = [
  # File replaced by directory: not yet testable
]
d_rpl_f = [
  # Directory replaced by file: not yet testable
]

f_mods = [
  ( create_f, ['ft'] ),
  ( create_f, ['fP'] ),
  #( create_f, ['ft','fP'] ),  # don't test all combinations, just because it's slow
]
d_mods = [
  ( create_d, ['dP'] ),
  # These test actions for operating on a child of the directory are not yet implemented:
  #( create_d, ['f_fA'] ),
  #( create_d, ['f_ft'] ),
  #( create_d, ['f_fP'] ),
  #( create_d, ['f_fD'] ),
  #( create_d, ['d_dP'] ),
  #( create_d, ['d_f_fA'] ),
]

#----------------------------------------------------------------------

# Set up all of the given SCENARIOS in their respective unique paths.
# This means committing their initialisation actions in r2, and then
# committing their change actions in r3 (assuming the repos was at r1).
# (See also the somewhat related svntest.actions.build_greek_tree_conflicts()
# and tree-conflicts tests using deep_trees in various other .py files.)
# SCENARIOS is a list of scenario tuples: (init_actions, change_actions).
# WC_DIR is a local path of an existing WC.
# BR_DIR is a nonexistent path within WC_DIR.
# BR_DIR and any necessary parent directories will be created, and then the
# scenario will be set up within it, and committed to the repository.
def set_up_repos(wc_dir, br_dir, scenarios):

  if not os.path.exists(br_dir):
    main.run_svn(None, "mkdir", "--parents", br_dir)

  # create the file F1 and dir D1 which the tests regard as pre-existing
  paths = incoming_paths(wc_dir, wc_dir)  # second arg is bogus but unimportant
  F1 = paths['F1']  # existing file to copy from
  F3 = paths['F3']  # existing file to copy from
  main.file_write(F1, "This is initially file F1.\n")
  main.file_write(F3, "This is initially file F3.\n")
  main.run_svn(None, 'add', F1, F3)
  D1 = paths['D1']  # existing dir to copy from
  main.run_svn(None, 'mkdir', D1)

  # create the initial parent dirs, and each file or dir unless to-be-added
  for init_mods, action_mods in scenarios:
    path = "_".join(action_mods)
    P = os.path.join(br_dir, path)  # parent of items to be tested
    main.run_svn(None, 'mkdir', '--parents', P)
    for modaction in init_mods:
      modify(modaction, incoming_paths(wc_dir, P))
  run_and_verify_svn(None, AnyOutput, [],
                     'commit', '-m', 'Initial set-up.', wc_dir)
  # Capture the revision number
  init_rev = 2  ### hard-coded

  # modify all files and dirs in their various ways
  for _path, action_mods in scenarios:
    path = "_".join(action_mods)
    P = os.path.join(br_dir, path)  # parent
    for modaction in action_mods:
      modify(modaction, incoming_paths(wc_dir, P))

  # commit all the modifications
  run_and_verify_svn(None, AnyOutput, [],
                     'commit', '-m', 'Action.', wc_dir)
  # Capture the revision number
  changed_rev = 3  ### hard-coded

  return (init_rev, changed_rev)

#----------------------------------------------------------------------

# Apply each of the changes in INCOMING_SCENARIOS to each of the local
# modifications in LOCALMOD_SCENARIOS.
# Ensure that the result in each case includes a tree conflict on the parent.
# OPERATION = 'update' or 'switch' or 'merge'
# If COMMIT_LOCAL_MODS is true, the LOCALMOD_SCENARIOS will be committed to
# the target branch before applying the INCOMING_SCENARIOS.
def ensure_tree_conflict(sbox, operation,
                         incoming_scenarios, localmod_scenarios,
                         commit_local_mods):
  sbox.build()
  wc_dir = sbox.wc_dir

  def url_of(repo_relative_path):
    return sbox.repo_url + '/' + repo_relative_path

  verbose_print("")
  verbose_print("=== Starting a set of '" + operation + "' tests.")

  # Path to source branch, relative to wc_dir.
  # Source is where the "incoming" mods are made.
  source_br = "branch1"

  verbose_print("--- Creating changes in repos")
  source_wc_dir = os.path.join(wc_dir, source_br)
  source_left_rev, source_right_rev = set_up_repos(wc_dir, source_wc_dir,
                                                   incoming_scenarios)
  head_rev = source_right_rev  ### assumption

  # Local mods are the outer loop because cleaning up the WC is slow
  # ('svn revert' isn't sufficient because it leaves unversioned files)
  for _loc_init_mods, loc_action in localmod_scenarios:
    # Determine the branch (directory) in which local mods will be made.
    if operation == 'update':
      # Path to target branch (where conflicts are raised), relative to wc_dir.
      target_br = source_br
      target_start_rev = source_left_rev
    else:  # switch/merge
      # Make, and work in, a "branch2" that is a copy of "branch1".
      target_br = "branch2"
      run_and_verify_svn(None, AnyOutput, [],
                         'copy', '-r', str(source_left_rev), url_of(source_br),
                         url_of(target_br),
                         '-m', 'Create target branch.')
      head_rev += 1
      target_start_rev = head_rev

    main.run_svn(None, 'checkout', '-r', str(target_start_rev), sbox.repo_url,
                 wc_dir)

    saved_cwd = os.getcwd()
    os.chdir(wc_dir)

    for _inc_init_mods, inc_action in incoming_scenarios:
      scen_name = "_".join(inc_action)
      source_url = url_of(source_br + '/' + scen_name)
      target_path = os.path.join(target_br, scen_name)

      verbose_print("=== " + str(inc_action) + " onto " + str(loc_action))

      verbose_print("--- Making local mods")
      for modaction in loc_action:
        modify(modaction, localmod_paths(".", target_path), is_init=False)
      if commit_local_mods:
        run_and_verify_svn(None, AnyOutput, [],
                           'commit', target_path,
                           '-m', 'Mods in target branch.')
        head_rev += 1

      # For update, verify the pre-condition that WC is out of date.
      # For switch/merge, there is no such precondition.
      if operation == 'update':
        verbose_print("--- Trying to commit (expecting 'out-of-date' error)")
        run_and_verify_commit(".", None, None, "Commit failed",
                              target_path)

      if modaction.startswith('f'):
        victim_name = 'F'
      else:
        victim_name = 'D'
      victim_path = os.path.join(target_path, victim_name)

      # Perform the operation that tries to apply incoming changes to the WC.
      # The command is expected to do something (and give some output),
      # and it should raise a conflict but not an error.
      expected_stdout = svntest.verify.ExpectedOutput("   C " + victim_path
                                                      + "\n",
                                                      match_all=False)
      # Do the main action
      if operation == 'update':
        verbose_print("--- Updating")
        run_and_verify_svn(None, expected_stdout, [],
                           'update', target_path)
      elif operation == 'switch':
        verbose_print("--- Switching")
        run_and_verify_svn(None, expected_stdout, [],
                           'switch', source_url, target_path)
      elif operation == 'merge':
        verbose_print("--- Merging")
        run_and_verify_svn(None, expected_stdout, [],
                           'merge', '--ignore-ancestry',
                           '--allow-mixed-revisions',
                           '-r', str(source_left_rev) + ':' + str(source_right_rev),
                           source_url, target_path)
      else:
        raise Exception("unknown operation: '" + operation + "'")

      verbose_print("--- Checking that 'info' reports the conflict")
      if operation == 'update' or operation == 'switch':
        incoming_left_rev = target_start_rev
      else:
        incoming_left_rev = source_left_rev
      if operation == 'update' or operation == 'merge':
        incoming_right_rev = source_right_rev
      else:
        incoming_right_rev = head_rev
      expected_info = { 'Tree conflict' : '.* upon ' + operation +
          r'.* \((none|(file|dir).*' +
            re.escape(victim_name + '@' + str(incoming_left_rev)) + r')' +
          r'.* \((none|(file|dir).*' +
            re.escape(victim_name + '@' + str(incoming_right_rev)) + r')' }
      run_and_verify_info([expected_info], victim_path)

      verbose_print("--- Trying to commit (expecting 'conflict' error)")
      ### run_and_verify_commit() requires an "output_tree" argument, but
      #   here we get away with passing None because we know an implementation
      #   detail: namely that it's not going to look at that argument if it
      #   gets the stderr that we're expecting.
      run_and_verify_commit(".", None, None, ".*conflict.*", victim_path)

      verbose_print("--- Checking that 'status' reports the conflict")
      expected_stdout = svntest.verify.RegexOutput("^......C.* " +
                                                   re.escape(victim_path) + "$",
                                                   match_all=False)
      run_and_verify_svn(None, expected_stdout, [],
                         'status', victim_path)

      verbose_print("--- Resolving the conflict")
      # Make sure resolving the parent does nothing.
      run_and_verify_resolved([], os.path.dirname(victim_path))
      # The real resolved call.
      run_and_verify_resolved([victim_path])

      verbose_print("--- Checking that 'status' does not report a conflict")
      exitcode, stdout, stderr = run_and_verify_svn(None, None, [],
                                                'status', victim_path)
      for line in stdout:
        if line[6] == 'C': # and line.endswith(victim_path + '\n'):
          raise svntest.Failure("unexpected status C") # on victim_path

      # verbose_print("--- Committing (should now succeed)")
      # run_and_verify_svn(None, None, [],
      #                    'commit', '-m', '', target_path)
      # target_start_rev += 1

      verbose_print("")

    os.chdir(saved_cwd)

    # Clean up the target branch and WC
    main.run_svn(None, 'revert', '-R', wc_dir)
    main.safe_rmtree(wc_dir)
    if operation != 'update':
      run_and_verify_svn(None, AnyOutput, [],
                         'delete', url_of(target_br),
                         '-m', 'Delete target branch.')
      head_rev += 1

#----------------------------------------------------------------------

# The main entry points for testing a set of scenarios.

# Test 'update' and/or 'switch'
# See test_wc_merge() for arguments.
def test_tc_up_sw(sbox, incoming_scen, wc_scen):
  sbox2 = sbox.clone_dependent()
  ensure_tree_conflict(sbox, 'update', incoming_scen, wc_scen, False)
  ensure_tree_conflict(sbox2, 'switch', incoming_scen, wc_scen, False)

# Test 'merge'
# INCOMING_SCEN is a list of scenarios describing the incoming changes to apply.
# BR_SCEN  is a list of scenarios describing how the local branch has
#   been modified relative to the merge-left source.
# WC_SCEN is a list of scenarios describing the local WC mods.
# One of BR_SCEN or WC_SCEN must be given but not both.
def test_tc_merge(sbox, incoming_scen, br_scen=None, wc_scen=None):
  if br_scen:
    ensure_tree_conflict(sbox, 'merge', incoming_scen, br_scen, True)
  else:
    ensure_tree_conflict(sbox, 'merge', incoming_scen, wc_scen, False)

#----------------------------------------------------------------------

# Tests for update/switch affecting a file, where the incoming change
# conflicts with a scheduled change in the WC.
#
# WC state: as scheduled (no obstruction)

def up_sw_file_mod_onto_del(sbox):
  "up/sw file: modify onto del/rpl/mv"
  test_tc_up_sw(sbox, f_mods, f_dels + f_rpls)
  # Note: See UC1 in notes/tree-conflicts/use-cases.txt.

def up_sw_file_del_onto_mod(sbox):
  "up/sw file: del/rpl/mv onto modify"
  # Results: tree-conflict on F
  #          no other change to WC (except possibly other half of move)
  #          ### OR (see Nico's email <>):
  #          schedule-delete but leave F on disk (can only apply with
  #            text-mod; prop-mod can't be preserved in this way)
  test_tc_up_sw(sbox, f_dels + f_rpls, f_mods)
  # Note: See UC2 in notes/tree-conflicts/use-cases.txt.

def up_sw_file_del_onto_del(sbox):
  "up/sw file: del/rpl/mv onto del/rpl/mv"
  test_tc_up_sw(sbox, f_dels + f_rpls, f_dels + f_rpls)
  # Note: See UC3 in notes/tree-conflicts/use-cases.txt.

def up_sw_file_add_onto_add(sbox):
  "up/sw file: add onto add"
  test_tc_up_sw(sbox, f_adds, f_adds)

#----------------------------------------------------------------------

# Tests for update/switch affecting a dir, where the incoming change
# conflicts with a scheduled change in the WC.

def up_sw_dir_mod_onto_del(sbox):
  "up/sw dir: modify onto del/rpl/mv"
  # WC state: any (D necessarily exists; children may have any state)
  test_tc_up_sw(sbox, d_mods, d_dels + d_rpls)

def up_sw_dir_del_onto_mod(sbox):
  "up/sw dir: del/rpl/mv onto modify"
  # WC state: any (D necessarily exists; children may have any state)
  test_tc_up_sw(sbox, d_dels + d_rpls, d_mods)

def up_sw_dir_del_onto_del(sbox):
  "up/sw dir: del/rpl/mv onto del/rpl/mv"
  # WC state: any (D necessarily exists; children may have any state)
  test_tc_up_sw(sbox, d_dels + d_rpls, d_dels + d_rpls)

# This is currently set as XFail over ra_dav because it hits
# issue #3314 'DAV can overwrite directories during copy'
#
#   TRUNK@35827.DBG>svn st -v branch1
#                   2        2 jrandom      branch1
#                   2        2 jrandom      branch1\dC
#   A  +            -        2 jrandom      branch1\dC\D
#
#   TRUNK@35827.DBG>svn log -r2:HEAD branch1 -v
#   ------------------------------------------------------------------------
#   r2 | jrandom | 2009-02-12 09:26:52 -0500 (Thu, 12 Feb 2009) | 1 line
#   Changed paths:
#      A /D1
#      A /F1
#      A /branch1
#      A /branch1/dC
#
#   Initial set-up.
#   ------------------------------------------------------------------------
#   r3 | jrandom | 2009-02-12 09:26:52 -0500 (Thu, 12 Feb 2009) | 1 line
#   Changed paths:
#      A /branch1/dC/D (from /D1:2)
#
#   Action.
#   ------------------------------------------------------------------------
#
#   TRUNK@35827.DBG>svn ci -m "Should be ood" branch1
#   Adding         branch1\dC\D
#
#   Committed revision 4.
@XFail(svntest.main.is_ra_type_dav)
@Issue(3314)
def up_sw_dir_add_onto_add(sbox):
  "up/sw dir: add onto add"
  # WC state: as scheduled (no obstruction)
  test_tc_up_sw(sbox, d_adds, d_adds)

#----------------------------------------------------------------------

# Tests for merge affecting a file, where the incoming change
# conflicts with the target.

def merge_file_mod_onto_not_file(sbox):
  "merge file: modify onto not-file"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, f_mods, br_scen = f_dels + f_rpl_d)
  test_tc_merge(sbox2, f_mods, wc_scen = f_dels)
  # Note: See UC4 in notes/tree-conflicts/use-cases.txt.

def merge_file_del_onto_not_same(sbox):
  "merge file: del/rpl/mv onto not-same"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, f_dels + f_rpls, br_scen = f_mods)
  test_tc_merge(sbox2, f_dels + f_rpls, wc_scen = f_mods)
  # Note: See UC5 in notes/tree-conflicts/use-cases.txt.

def merge_file_del_onto_not_file(sbox):
  "merge file: del/rpl/mv onto not-file"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, f_dels + f_rpls, br_scen = f_dels + f_rpl_d)
  test_tc_merge(sbox2, f_dels + f_rpls, wc_scen = f_dels)
  # Note: See UC6 in notes/tree-conflicts/use-cases.txt.

def merge_file_add_onto_not_none(sbox):
  "merge file: add onto not-none"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, f_adds, br_scen = f_adds)  ### + d_adds (at path "F")
  test_tc_merge(sbox2, f_adds, wc_scen = f_adds)  ### + d_adds (at path "F")

#----------------------------------------------------------------------

# Tests for merge affecting a dir, where the incoming change
# conflicts with the target branch.

def merge_dir_mod_onto_not_dir(sbox):
  "merge dir: modify onto not-dir"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, d_mods, br_scen = d_dels + d_rpl_f)
  test_tc_merge(sbox2, d_mods, wc_scen = d_dels)

# Test for issue #3150 'tree conflicts with directories as victims'.
@XFail()
@Issue(3150)
def merge_dir_del_onto_not_same(sbox):
  "merge dir: del/rpl/mv onto not-same"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, d_dels + d_rpls, br_scen = d_mods)
  test_tc_merge(sbox2, d_dels + d_rpls, wc_scen = d_mods)

def merge_dir_del_onto_not_dir(sbox):
  "merge dir: del/rpl/mv onto not-dir"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, d_dels + d_rpls, br_scen = d_dels + d_rpl_f)
  test_tc_merge(sbox2, d_dels + d_rpls, wc_scen = d_dels)

def merge_dir_add_onto_not_none(sbox):
  "merge dir: add onto not-none"
  sbox2 = sbox.clone_dependent()
  test_tc_merge(sbox, d_adds, br_scen = d_adds)  ### + f_adds (at path "D")
  test_tc_merge(sbox2, d_adds, wc_scen = d_adds)  ### + f_adds (at path "D")

#----------------------------------------------------------------------

@Issue(3805)
def force_del_tc_inside(sbox):
  "--force del on dir with TCs inside"

  #          A/C       <-  delete with --force
  # A  +  C  A/C/dir
  # A  +  C  A/C/file

  sbox.build()
  wc_dir = sbox.wc_dir

  C   = os.path.join(wc_dir, "A", "C")
  dir = os.path.join(wc_dir, "A", "C", "dir")
  file = os.path.join(wc_dir, "A", "C", "file")

  # Add dir
  main.run_svn(None, 'mkdir', dir)

  # Add file
  content = "This is the file 'file'.\n"
  main.file_append(file, content)
  main.run_svn(None, 'add', file)

  main.run_svn(None, 'commit', '-m', 'Add dir and file', wc_dir)

  # Remove dir and file in r3.
  main.run_svn(None, 'delete', dir, file)
  main.run_svn(None, 'commit', '-m', 'Remove dir and file', wc_dir)

  # Warp back to -r2, dir and file coming back.
  main.run_svn(None, 'update', '-r2', wc_dir)

  # Set a meaningless prop on each dir and file
  run_and_verify_svn(None,
                     ["property 'propname' set on '" + dir + "'\n"],
                     [], 'ps', 'propname', 'propval', dir)
  run_and_verify_svn(None,
                     ["property 'propname' set on '" + file + "'\n"],
                     [], 'ps', 'propname', 'propval', file)

  # Update WC to HEAD, tree conflicts result dir and file
  # because there are local mods on the props.
  expected_output = wc.State(wc_dir, {
    'A/C/dir' : Item(status='  ', treeconflict='C'),
    'A/C/file' : Item(status='  ', treeconflict='C'),
    })

  expected_disk = main.greek_state.copy()
  expected_disk.add({
    'A/C/dir' : Item(props={'propname' : 'propval'}),
    'A/C/file' : Item(contents=content, props={'propname' : 'propval'}),
    })

  expected_status = get_virginal_state(wc_dir, 2)
  expected_status.tweak(wc_rev='3')
  expected_status.add({
    'A/C/dir' : Item(status='A ', wc_rev='-', copied='+', treeconflict='C'),
    'A/C/file' : Item(status='A ', wc_rev='-', copied='+', treeconflict='C'),
    })
  run_and_verify_update(wc_dir,
                        expected_output, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc_dir)

  # Delete A/C with --force, in effect disarming the tree-conflicts.
  run_and_verify_svn(None,
                     verify.UnorderedOutput(['D         ' + C + '\n',
                                             'D         ' + dir + '\n',
                                             'D         ' + file + '\n']),
                     [], 'delete', C, '--force')

  # Verify deletion status
  # Note: the tree conflicts are removed because we forced the delete.
  expected_status.tweak('A/C', status='D ')
  expected_status.remove('A/C/dir', 'A/C/file')

  run_and_verify_status(wc_dir, expected_status)

  # Commit, remove the "disarmed" tree-conflict.
  expected_output = wc.State(wc_dir, { 'A/C' : Item(verb='Deleting') })

  expected_status.remove('A/C')

  run_and_verify_commit(wc_dir,
                        expected_output, expected_status, None,
                        wc_dir)

#----------------------------------------------------------------------

@Issue(3805)
def force_del_tc_is_target(sbox):
  "--force del on tree-conflicted targets"
  #          A/C
  # A  +  C  A/C/dir   <-  delete with --force
  # A  +  C  A/C/file  <-  delete with --force

  sbox.build()
  wc_dir = sbox.wc_dir

  C   = os.path.join(wc_dir, "A", "C")
  dir = os.path.join(wc_dir, "A", "C", "dir")
  file = os.path.join(wc_dir, "A", "C", "file")

  # Add dir
  main.run_svn(None, 'mkdir', dir)

  # Add file
  content = "This is the file 'file'.\n"
  main.file_append(file, content)
  main.run_svn(None, 'add', file)

  main.run_svn(None, 'commit', '-m', 'Add dir and file', wc_dir)

  # Remove dir and file in r3.
  main.run_svn(None, 'delete', dir, file)
  main.run_svn(None, 'commit', '-m', 'Remove dir and file', wc_dir)

  # Warp back to -r2, dir and file coming back.
  main.run_svn(None, 'update', '-r2', wc_dir)

  # Set a meaningless prop on each dir and file
  run_and_verify_svn(None,
                     ["property 'propname' set on '" + dir + "'\n"],
                     [], 'ps', 'propname', 'propval', dir)
  run_and_verify_svn(None,
                     ["property 'propname' set on '" + file + "'\n"],
                     [], 'ps', 'propname', 'propval', file)

  # Update WC to HEAD, tree conflicts result dir and file
  # because there are local mods on the props.
  expected_output = wc.State(wc_dir, {
    'A/C/dir' : Item(status='  ', treeconflict='C'),
    'A/C/file' : Item(status='  ', treeconflict='C'),
    })

  expected_disk = main.greek_state.copy()
  expected_disk.add({
    'A/C/dir' : Item(props={'propname' : 'propval'}),
    'A/C/file' : Item(contents=content, props={'propname' : 'propval'}),
    })

  expected_status = get_virginal_state(wc_dir, 2)
  expected_status.tweak(wc_rev='3')
  expected_status.add({
    'A/C/dir' : Item(status='A ', wc_rev='-', copied='+', treeconflict='C'),
    'A/C/file' : Item(status='A ', wc_rev='-', copied='+', treeconflict='C'),
    })
  run_and_verify_update(wc_dir,
                        expected_output, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc_dir)

  # Delete nodes with --force, in effect disarming the tree-conflicts.
  run_and_verify_svn(None,
                     ['D         ' + dir + '\n',
                      'D         ' + file + '\n'],
                     [],
                     'delete', dir, file, '--force')

  # The rm --force now removes the nodes and the tree conflicts on them
  expected_status.remove('A/C/dir', 'A/C/file')
  run_and_verify_status(wc_dir, expected_status)

  # Commit, remove the "disarmed" tree-conflict.
  expected_output = wc.State(wc_dir, {})

  run_and_verify_commit(wc_dir,
                        expected_output, expected_status, None,
                        wc_dir)

#----------------------------------------------------------------------

# A regression test to check that "rm --keep-local" on a tree-conflicted
# node leaves the WC in a valid state in which simple commands such as
# "status" do not error out.  At one time the command left the WC in an
# invalid state.  (Before r989189, "rm --keep-local" used to have the effect
# of "disarming" the conflict in the sense that "commit" would ignore the
# conflict.)

def query_absent_tree_conflicted_dir(sbox):
  "query an unversioned tree-conflicted dir"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Some paths we'll care about
  C_path = os.path.join(wc_dir, "A", "C")
  C_C_path = os.path.join(wc_dir, "A", "C", "C")

  # Add a directory A/C/C as r2.
  main.run_svn(None, 'mkdir', C_C_path)
  main.run_svn(None, 'commit', '-m', 'Add directory A/C/C', wc_dir)

  # Remove that directory A/C/C as r3.
  main.run_svn(None, 'delete', C_C_path)
  main.run_svn(None, 'commit', '-m', 'Remove directory A/C/C', wc_dir)

  # Warp back to -r2 with the directory added.
  main.run_svn(None, 'update', '-r2', wc_dir)

  # Set a meaningless prop on A/C/C
  run_and_verify_svn(None,
                     ["property 'propname' set on '" + C_C_path + "'\n"],
                     [], 'ps', 'propname', 'propval', C_C_path)

  # Update WC to HEAD, a tree conflict results on A/C/C because of the
  # working prop on A/C/C.
  expected_output = wc.State(wc_dir, {
    'A/C/C' : Item(status='  ', treeconflict='C'),
    })
  expected_disk = main.greek_state.copy()
  expected_disk.add({'A/C/C' : Item(props={'propname' : 'propval'})})
  expected_status = get_virginal_state(wc_dir, 1)
  expected_status.tweak(wc_rev='3')
  expected_status.add({'A/C/C' : Item(status='A ',
                                      wc_rev='-',
                                      copied='+',
                                      treeconflict='C')})
  run_and_verify_update(wc_dir,
                        expected_output, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc_dir)

  # Delete A/C with --keep-local.
  run_and_verify_svn(None,
                     verify.UnorderedOutput(['D         ' + C_C_path + '\n',
                                             'D         ' + C_path + '\n']),
                     [],
                     'delete', C_path, '--keep-local')

  expected_status.tweak('A/C', status='D ')
  expected_status.remove('A/C/C')
  run_and_verify_status(wc_dir, expected_status)

  # Try to access the absent tree-conflict as explicit target.
  # These used to fail like this:
  ## CMD: svn status -v -u -q
  ## [...]
  ## subversion/svn/status-cmd.c:248: (apr_err=155035)
  ## subversion/svn/util.c:953: (apr_err=155035)
  ## subversion/libsvn_client/status.c:270: (apr_err=155035)
  ## subversion/libsvn_wc/lock.c:607: (apr_err=155035)
  ## subversion/libsvn_wc/entries.c:1607: (apr_err=155035)
  ## subversion/libsvn_wc/wc_db.c:3288: (apr_err=155035)
  ## svn: Expected node '/.../tree_conflict_tests-20/A/C' to be added.

  # A/C/C is now unversioned, using status:
  expected_output = wc.State(wc_dir, {
    })
  run_and_verify_status(C_C_path, expected_output)

  # using info:
  run_and_verify_svn(None, None, ".*W155010.*The node.*was not found.*",
                     'info', C_C_path)

#----------------------------------------------------------------------

@Issue(3608)
def up_add_onto_add_revert(sbox):
  "issue #3608: reverting an add onto add conflict"

  sbox.build()
  wc_dir = sbox.wc_dir
  wc2_dir = sbox.add_wc_path('wc2')
  svntest.actions.run_and_verify_svn(None, None, [], 'checkout',
                                     sbox.repo_url, wc2_dir)

  file1 = os.path.join(wc_dir, 'newfile')
  file2 = os.path.join(wc2_dir, 'newfile')

  dir1 = os.path.join(wc_dir, 'NewDir')
  dir2 = os.path.join(wc2_dir, 'NewDir')

  main.run_svn(None, 'cp', os.path.join(wc_dir, 'iota'), file1)
  main.run_svn(None, 'cp', os.path.join(wc2_dir, 'iota'), file2)

  main.run_svn(None, 'cp', os.path.join(wc_dir, 'A/C'), dir1)
  main.run_svn(None, 'cp', os.path.join(wc2_dir, 'A/C'), dir2)

  main.run_svn(None, 'ci', wc_dir, '-m', 'Added file')

  expected_disk = main.greek_state.copy()
  expected_disk.add({
    'newfile'           : Item(contents="This is the file 'iota'.\n"),
    'NewDir'            : Item(),
    })

  expected_status = get_virginal_state(wc2_dir, 2)
  expected_status.add({
    'newfile' : Item(status='R ', copied='+', treeconflict='C', wc_rev='-'),
    'NewDir'  : Item(status='R ', copied='+', treeconflict='C', wc_rev='-'),
    })

  run_and_verify_update(wc2_dir,
                        None, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc2_dir)

  # Currently (r927086), this removes dir2 and file2 in a way that
  # they don't reappear after update.
  main.run_svn(None, 'revert', file2)
  main.run_svn(None, 'revert', dir2)

  expected_status = get_virginal_state(wc2_dir, 2)
  expected_status.add({
    'newfile' : Item(status='  ', wc_rev='2'),
    'NewDir'  : Item(status='  ', wc_rev='2'),
    })

  # Expected behavior is that after revert + update the tree matches
  # the repository
  run_and_verify_update(wc2_dir,
                        None, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc2_dir)


#----------------------------------------------------------------------
# Regression test for issue #3525 and #3533
#
@Issues(3525,3533)
def lock_update_only(sbox):
  "lock status update shouldn't flag tree conflict"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a second copy of the working copy
  wc_b = sbox.add_wc_path('_b')
  svntest.actions.duplicate_dir(wc_dir, wc_b)

  fname = 'iota'
  file_path = os.path.join(sbox.wc_dir, fname)
  file_path_b = os.path.join(wc_b, fname)

  # Lock a file as wc_author, and schedule the file for deletion.
  svntest.actions.run_and_verify_svn(None, ".*locked by user", [], 'lock',
                                     '-m', '', file_path)
  svntest.main.run_svn(None, 'delete', file_path)

  # In our other working copy, steal that lock.
  svntest.actions.run_and_verify_svn(None, ".*locked by user", [], 'lock',
                                     '-m', '', '--force', file_path)

  # Now update the first working copy.  It should appear as a no-op.
  expected_disk = main.greek_state.copy()
  expected_disk.remove('iota')
  expected_status = get_virginal_state(wc_dir, 1)
  expected_status.tweak('iota', status='D ', writelocked='K')
  run_and_verify_update(wc_dir,
                        None, expected_disk, expected_status,
                        None, None, None, None, None, 1,
                        wc_dir)


#----------------------------------------------------------------------
@Issue(3469)
def at_directory_external(sbox):
  "tree conflict at directory external"

  sbox.build()
  wc_dir = sbox.wc_dir

  # r2: create a directory external: ^/E -> ^/A
  svntest.main.run_svn(None, 'ps', 'svn:externals', '^/A E', wc_dir)
  svntest.main.run_svn(None, 'commit', '-m', 'ps', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)

  # r3: modify ^/A/B/E/alpha
  open(sbox.ospath('A/B/E/alpha'), 'a').write('This is still A/B/E/alpha.\n')
  svntest.main.run_svn(None, 'commit', '-m', 'file mod', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)
  merge_rev = svntest.main.youngest(sbox.repo_dir)

  # r4: create ^/A/B/E/alpha2
  open(sbox.ospath('A/B/E/alpha2'), 'a').write("This is the file 'alpha2'.\n")
  svntest.main.run_svn(None, 'add', sbox.ospath('A/B/E/alpha2'))
  svntest.main.run_svn(None, 'commit', '-m', 'file add', wc_dir)
  svntest.main.run_svn(None, 'update', wc_dir)
  merge_rev2 = svntest.main.youngest(sbox.repo_dir)

  # r5: merge those
  svntest.main.run_svn(None, "merge", '-c', merge_rev, '^/A/B', wc_dir)
  svntest.main.run_svn(None, "merge", '-c', merge_rev2, '^/A/B', wc_dir)

#----------------------------------------------------------------------
@Issue(3779)
### This test currently passes on the current behaviour.
### However in many cases it is unclear whether the current behaviour is
### correct. Review is still required.
def actual_only_node_behaviour(sbox):
  "test behaviour with actual-only nodes"

  sbox.build()
  A_url = sbox.repo_url + '/A'
  A_copy_url = sbox.repo_url + '/A_copy'
  wc_dir = sbox.wc_dir
  foo_path = sbox.ospath('A/foo', wc_dir)

  # r2: copy ^/A -> ^/A_copy
  sbox.simple_repo_copy('A', 'A_copy')

  # r3: add a file foo on ^/A_copy branch
  wc2_dir = sbox.add_wc_path('wc2')
  foo2_path = sbox.ospath('foo', wc2_dir)
  svntest.main.run_svn(None, "checkout", A_copy_url, wc2_dir)
  svntest.main.file_write(foo2_path, "This is initially file foo.\n")
  svntest.main.run_svn(None, "add", foo2_path)
  svntest.main.run_svn(None, "commit", '-m', svntest.main.make_log_msg(),
                       foo2_path)

  # r4: make a change to foo
  svntest.main.file_append(foo2_path, "This is a new line in file foo.\n")
  svntest.main.run_svn(None, "commit", '-m', svntest.main.make_log_msg(),
                       wc2_dir)

  # cherry-pick r4 to ^/A -- the resulting tree conflict creates
  # an actual-only node for 'A/foo'
  sbox.simple_update()
  svntest.main.run_svn(None, "merge", '-c', '4', A_copy_url,
                       os.path.join(wc_dir, 'A'))

  # Attempt running various commands on foo and verify expected behavior

  # add
  expected_stdout = None
  expected_stderr = ".*foo.*is an existing item in conflict.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "add", foo_path)

  # add (with an existing obstruction of foo)
  svntest.main.file_write(foo_path, "This is an obstruction of foo.\n")
  expected_stdout = None
  expected_stderr = ".*foo.*is an existing item in conflict.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "add", foo_path)
  os.remove(foo_path) # remove obstruction

  # blame (praise, annotate, ann)
  expected_stdout = None
  expected_stderr = ".*foo.*not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "blame", foo_path)

  # cat
  expected_stdout = None
  expected_stderr = ".*foo.*not under version control.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "cat", foo_path)

  # cat -rBASE
  expected_stdout = None
  expected_stderr = ".*foo.*not under version control.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "cat", "-r", "BASE", foo_path)
  # changelist (cl)
  expected_stdout = None
  expected_stderr = ".*svn: warning: W155010: The node '.*foo' was not found."
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "changelist", "my_changelist", foo_path)

  # checkout (co)
  ### this does not error out -- needs review
  expected_stdout = None
  expected_stderr = []
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "checkout", A_copy_url, foo_path)
  ### for now, ignore the fact that checkout succeeds and remove the nested
  ### working copy so we can test more commands
  shutil.rmtree(foo_path)

  # cleanup
  expected_stdout = None
  expected_stderr = ".*foo.*is not a working copy directory"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "cleanup", foo_path)
  # commit (ci)
  expected_stdout = None
  expected_stderr = ".*foo.*remains in conflict.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "commit", foo_path)
  # copy (cp)
  expected_stdout = None
  expected_stderr = ".*foo.*does not exist.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "copy", foo_path, foo_path + ".copy")

  # delete (del, remove, rm)
  expected_stdout = None
  expected_stderr = ".*foo.*is not under version control.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "delete", foo_path)

  # diff (di)
  expected_stdout = None
  expected_stderr = ".*foo.*is not under version control.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "diff", foo_path)
  # export
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "export", foo_path, sbox.get_tempname())
  # import
  expected_stdout = None
  expected_stderr = ".*foo.*does not exist.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "import", '-m', svntest.main.make_log_msg(),
                     foo_path, sbox.repo_url + '/foo_imported')

  # info
  expected_info = {
    'Tree conflict': 'local missing, incoming edit upon merge.*',
    'Name': 'foo',
    'Schedule': 'normal',
    'Node Kind': 'none',
    'Path': re.escape(sbox.ospath('A/foo')),
  }
  run_and_verify_info([expected_info], foo_path)

  # list (ls)
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "list", foo_path)

  # lock
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "lock", foo_path)
  # log
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "log", foo_path)
  # merge
  # note: this is intentionally a no-op merge that does not record mergeinfo
  expected_stdout = None
  expected_stderr = ".*foo.*does not exist.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "merge", '--ignore-ancestry', '-c', '4',
                     A_copy_url + '/mu', foo_path)

  # mergeinfo
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "mergeinfo", A_copy_url + '/foo', foo_path)
  # mkdir
  expected_stdout = None
  expected_stderr = ".*foo.*is an existing item in conflict.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "mkdir", foo_path)

  # move (mv, rename, ren)
  expected_stdout = None
  expected_stderr = ".*foo.*does not exist.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "move", foo_path, foo_path + ".moved")
  # patch
  expected_stdout = None
  expected_stderr = ".*foo.*does not exist.*"
  patch_path = sbox.get_tempname()
  f = open(patch_path, 'w')
  patch_data = [
    "--- foo	(revision 2)\n"
    "+++ foo	(working copy)\n"
    "@@ -1 +1,2 @@\n"
    " foo\n"
    " +foo\n"
  ]
  for line in patch_data:
    f.write(line)
  f.close()
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "patch", patch_path, sbox.ospath("A/foo"))

  # propdel (pdel, pd)
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "propdel", "svn:eol-style", foo_path)

  # propget (pget, pg)
  expected_stdout = None
  expected_stderr = ".*foo.*is not under version control.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "propget", "svn:eol-style", foo_path)

  # proplist (plist, pl)
  expected_stdout = None
  expected_stderr = ".*foo.*is not under version control.*"
  svntest.actions.run_and_verify_svn(None, expected_stdout, expected_stderr,
                                     "proplist", foo_path)

  # propset (pset, ps)
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "propset", "svn:eol-style", "native", foo_path)

  # relocate
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "relocate", A_copy_url + "/foo", foo_path)

  # resolve
  expected_stdout = "Resolved conflicted state of.*foo.*"
  expected_stderr = []
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "resolve", "--accept", "working", foo_path)

  # revert the entire working copy and repeat the merge so we can test
  # more commands
  svntest.main.run_svn(None, "revert", "-R", wc_dir)
  svntest.main.run_svn(None, "merge", '-c', '4', A_copy_url,
                       os.path.join(wc_dir, 'A'))

  # revert
  expected_stdout = "Reverted.*foo.*"
  expected_stderr = []
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "revert", foo_path)

  # revert the entire working copy and repeat the merge so we can test
  # more commands
  svntest.main.run_svn(None, "revert", "-R", wc_dir)
  svntest.main.run_svn(None, "merge", '-c', '4', A_copy_url,
                       os.path.join(wc_dir, 'A'))

  # status (stat, st)
  expected_status = wc.State(foo_path, {
    '' : Item(status='! ', treeconflict='C'),
  })
  run_and_verify_status(foo_path, expected_status)

  # switch (sw)
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "switch", A_copy_url + "/foo", foo_path)

  # unlock
  expected_stdout = None
  expected_stderr = ".*foo.*was not found.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "unlock", foo_path)

  # update (up)
  expected_stdout = [
   "Skipped '%s' -- Node remains in conflict\n" % sbox.ospath('A/foo'),
   "Summary of conflicts:\n",
   "  Skipped paths: 1\n",
  ]
  expected_stderr = []
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "update", foo_path)

  # upgrade
  expected_stdout = None
  expected_stderr = ".*Can't upgrade.*foo.*"
  run_and_verify_svn(None, expected_stdout, expected_stderr,
                     "upgrade", foo_path)

#----------------------------------------------------------------------
# Regression test for an issue #3526 variant
#
@Issues(3526)
def update_dir_with_not_present(sbox):
  "lock status update shouldn't flag tree conflict"

  sbox.build()
  wc_dir = sbox.wc_dir

  newtxt = sbox.ospath('A/B/new.txt')

  main.file_write(newtxt, 'new.txt')
  sbox.simple_add('A/B/new.txt')
  sbox.simple_commit()

  sbox.simple_move('A/B/new.txt', 'A/C/newer.txt')
  sbox.simple_commit()
  sbox.simple_rm('A/B')

  # We can't commit this without updating (ra_svn produces its own error)
  run_and_verify_svn(None, None, "svn: (E155011|E160028): Dir.*B.*out of date",
                     'ci', '-m', '', wc_dir)

  # So we run update
  run_and_verify_svn(None, None, [],
                     'up', wc_dir)

  # And now we can commit
  run_and_verify_svn(None, None, [],
                     'ci', '-m', '', wc_dir)

#######################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              up_sw_file_mod_onto_del,
              up_sw_file_del_onto_mod,
              up_sw_file_del_onto_del,
              up_sw_file_add_onto_add,
              up_sw_dir_mod_onto_del,
              up_sw_dir_del_onto_mod,
              up_sw_dir_del_onto_del,
              up_sw_dir_add_onto_add,
              merge_file_mod_onto_not_file,
              merge_file_del_onto_not_same,
              merge_file_del_onto_not_file,
              merge_file_add_onto_not_none,
              merge_dir_mod_onto_not_dir,
              merge_dir_del_onto_not_same,
              merge_dir_del_onto_not_dir,
              merge_dir_add_onto_not_none,
              force_del_tc_inside,
              force_del_tc_is_target,
              query_absent_tree_conflicted_dir,
              up_add_onto_add_revert,
              lock_update_only,
              at_directory_external,
              actual_only_node_behaviour,
              update_dir_with_not_present,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
