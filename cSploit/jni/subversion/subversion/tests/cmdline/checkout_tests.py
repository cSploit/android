#!/usr/bin/env python
#
#  checkout_tests.py:  Testing checkout --force behavior when local
#                      tree already exits.
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
import sys, re, os, time, subprocess

# Our testing module
import svntest
from svntest import wc, actions

# (abbreviation)
Skip = svntest.testcase.Skip_deco
SkipUnless = svntest.testcase.SkipUnless_deco
XFail = svntest.testcase.XFail_deco
Issues = svntest.testcase.Issues_deco
Issue = svntest.testcase.Issue_deco
Wimp = svntest.testcase.Wimp_deco
Item = wc.StateItem

#----------------------------------------------------------------------
# Helper function for testing stderr from co.
# If none of the strings in STDERR list matches the regular expression
# RE_STRING raise an error.
def test_stderr(re_string, stderr):
  exp_err_re = re.compile(re_string)
  for line in stderr:
    if exp_err_re.search(line):
      return
  if svntest.main.options.verbose:
    for x in stderr:
      sys.stdout.write(x)
    print("Expected stderr reg-ex: '" + re_string + "'")
  raise svntest.Failure("Checkout failed but not in the expected way")

#----------------------------------------------------------------------
# Helper function to set up an existing local tree that has paths which
# obstruct with the incoming WC.
#
# Build a sandbox SBOX without a WC.  Created the following paths
# rooted at SBOX.WC_DIR:
#
#    iota
#    A/
#    A/mu
#
# If MOD_FILES is FALSE, 'iota' and 'A/mu' have the same contents as the
# standard greek tree.  If TRUE the contents of each as set as follows:
#
#    iota : contents == "This is the local version of the file 'iota'.\n"
#    A/mu : contents == "This is the local version of the file 'mu'.\n"
#
# If ADD_UNVERSIONED is TRUE, add the following files and directories,
# rooted in SBOX.WC_DIR, that don't exist in the standard greek tree:
#
#    'sigma'
#    'A/upsilon'
#    'A/Z/'
#
# Return the expected output for svn co --force SBOX.REPO_URL SBOX.WC_DIR
#
def make_local_tree(sbox, mod_files=False, add_unversioned=False):
  """Make a local unversioned tree to checkout into."""

  sbox.build(create_wc = False)

  if os.path.exists(sbox.wc_dir):
    svntest.main.safe_rmtree(sbox.wc_dir)

  export_target = sbox.wc_dir
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = sbox.wc_dir
  expected_output.desc[""] = Item()
  expected_output.tweak(contents=None, status="A ")

  # Export an unversioned tree to sbox.wc_dir.
  svntest.actions.run_and_verify_export(sbox.repo_url,
                                        export_target,
                                        expected_output,
                                        svntest.main.greek_state.copy())

  # Remove everything remaining except for 'iota', 'A/', and 'A/mu'.
  svntest.main.safe_rmtree(os.path.join(sbox.wc_dir, "A", "B"))
  svntest.main.safe_rmtree(os.path.join(sbox.wc_dir, "A", "C"))
  svntest.main.safe_rmtree(os.path.join(sbox.wc_dir, "A", "D"))

  # Should obstructions differ from the standard greek tree?
  if mod_files:
    iota_path = os.path.join(sbox.wc_dir, "iota")
    mu_path = os.path.join(sbox.wc_dir, "A", "mu")
    svntest.main.file_write(iota_path,
                            "This is the local version of the file 'iota'.\n")
    svntest.main.file_write(mu_path,
                            "This is the local version of the file 'mu'.\n")

  # Add some files that won't obstruct anything in standard greek tree?
  if add_unversioned:
    sigma_path = os.path.join(sbox.wc_dir, "sigma")
    svntest.main.file_append(sigma_path, "unversioned sigma")
    upsilon_path = os.path.join(sbox.wc_dir, "A", "upsilon")
    svntest.main.file_append(upsilon_path, "unversioned upsilon")
    Z_path = os.path.join(sbox.wc_dir, "A", "Z")
    os.mkdir(Z_path)

  return wc.State(sbox.wc_dir, {
    "A"           : Item(status='E '), # Obstruction
    "A/B"         : Item(status='A '),
    "A/B/lambda"  : Item(status='A '),
    "A/B/E"       : Item(status='A '),
    "A/B/E/alpha" : Item(status='A '),
    "A/B/E/beta"  : Item(status='A '),
    "A/B/F"       : Item(status='A '),
    "A/mu"        : Item(status='E '), # Obstruction
    "A/C"         : Item(status='A '),
    "A/D"         : Item(status='A '),
    "A/D/gamma"   : Item(status='A '),
    "A/D/G"       : Item(status='A '),
    "A/D/G/pi"    : Item(status='A '),
    "A/D/G/rho"   : Item(status='A '),
    "A/D/G/tau"   : Item(status='A '),
    "A/D/H"       : Item(status='A '),
    "A/D/H/chi"   : Item(status='A '),
    "A/D/H/omega" : Item(status='A '),
    "A/D/H/psi"   : Item(status='A '),
    "iota"        : Item(status='E '), # Obstruction
    })

######################################################################
# Tests
#
#   Each test must return on success or raise on failure.
#----------------------------------------------------------------------

def checkout_with_obstructions(sbox):
  """co with obstructions conflicts without --force"""

  make_local_tree(sbox, False, False)

  #svntest.factory.make(sbox,
  #       """# Checkout with unversioned obstructions lying around.
  #          svn co url wc_dir
  #          svn status""")
  #svntest.factory.make(sbox,
  #       """# Now see to it that we can recover from the obstructions.
  #          rm -rf A iota
  #          svn up""")
  #exit(0)

  wc_dir = sbox.wc_dir
  url = sbox.repo_url

  # Checkout with unversioned obstructions causes tree conflicts.
  # svn co url wc_dir
  expected_output = svntest.wc.State(wc_dir, {
    'iota'              : Item(status='  ', treeconflict='C'),
    'A'                 : Item(status='  ', treeconflict='C'),
    # And the updates below the tree conflict
    'A/D'               : Item(status='  ', treeconflict='A'),
    'A/D/gamma'         : Item(status='  ', treeconflict='A'),
    'A/D/G'             : Item(status='  ', treeconflict='A'),
    'A/D/G/rho'         : Item(status='  ', treeconflict='A'),
    'A/D/G/pi'          : Item(status='  ', treeconflict='A'),
    'A/D/G/tau'         : Item(status='  ', treeconflict='A'),
    'A/D/H'             : Item(status='  ', treeconflict='A'),
    'A/D/H/chi'         : Item(status='  ', treeconflict='A'),
    'A/D/H/omega'       : Item(status='  ', treeconflict='A'),
    'A/D/H/psi'         : Item(status='  ', treeconflict='A'),
    'A/B'               : Item(status='  ', treeconflict='A'),
    'A/B/E'             : Item(status='  ', treeconflict='A'),
    'A/B/E/beta'        : Item(status='  ', treeconflict='A'),
    'A/B/E/alpha'       : Item(status='  ', treeconflict='A'),
    'A/B/F'             : Item(status='  ', treeconflict='A'),
    'A/B/lambda'        : Item(status='  ', treeconflict='A'),
    'A/C'               : Item(status='  ', treeconflict='A'),
    'A/mu'              : Item(status='  ', treeconflict='A'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B', 'A/B/E', 'A/B/E/beta', 'A/B/E/alpha', 'A/B/F',
    'A/B/lambda', 'A/D', 'A/D/G', 'A/D/G/rho', 'A/D/G/pi', 'A/D/G/tau',
    'A/D/H', 'A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi', 'A/D/gamma', 'A/C')

  actions.run_and_verify_checkout2(False, url, wc_dir, expected_output,
    expected_disk, None, None, None, None)

  # svn status
  expected_status = actions.get_virginal_state(wc_dir, 1)
  # A and iota are tree conflicted and obstructed
  expected_status.tweak('A', 'iota', status='D ', wc_rev=1,
                        treeconflict='C')

  expected_status.tweak('A/D', 'A/D/G', 'A/D/G/rho', 'A/D/G/pi', 'A/D/G/tau',
    'A/D/H', 'A/D/H/chi', 'A/D/H/omega', 'A/D/H/psi', 'A/D/gamma', 'A/B',
    'A/B/E', 'A/B/E/beta', 'A/B/E/alpha', 'A/B/F', 'A/B/lambda', 'A/C',
    status='D ')
  # A/mu exists on disk, but is deleted
  expected_status.tweak('A/mu', status='D ')

  actions.run_and_verify_unquiet_status(wc_dir, expected_status)


  # Now see to it that we can recover from the obstructions.
  # rm -rf A iota
  svntest.main.safe_rmtree( os.path.join(wc_dir, 'A') )
  os.remove( os.path.join(wc_dir, 'iota') )


  svntest.main.run_svn(None, 'revert', '-R', os.path.join(wc_dir, 'A'),
                       os.path.join(wc_dir, 'iota'))

  # svn up
  expected_output = svntest.wc.State(wc_dir, {
  })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = actions.get_virginal_state(wc_dir, 1)

  actions.run_and_verify_update(wc_dir, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir)



#----------------------------------------------------------------------

def forced_checkout_of_file_with_dir_obstructions(sbox):
  """forced co flags conflict if a dir obstructs a file"""
  # svntest.factory.make(sbox,
  #                      """mkdir $WC_DIR.other/iota
  #                         svn co --force url $WC_DIR.other """)
  sbox.build()
  url = sbox.repo_url
  wc_dir_other = sbox.add_wc_path('other')

  other_iota = os.path.join(wc_dir_other, 'iota')

  # mkdir $WC_DIR.other/iota
  os.makedirs(other_iota)

  # svn co --force url $WC_DIR.other
  expected_output = svntest.wc.State(wc_dir_other, {
    'A'                 : Item(status='A '),
    'A/B'               : Item(status='A '),
    'A/B/E'             : Item(status='A '),
    'A/B/E/alpha'       : Item(status='A '),
    'A/B/E/beta'        : Item(status='A '),
    'A/B/F'             : Item(status='A '),
    'A/B/lambda'        : Item(status='A '),
    'A/D'               : Item(status='A '),
    'A/D/H'             : Item(status='A '),
    'A/D/H/chi'         : Item(status='A '),
    'A/D/H/omega'       : Item(status='A '),
    'A/D/H/psi'         : Item(status='A '),
    'A/D/G'             : Item(status='A '),
    'A/D/G/pi'          : Item(status='A '),
    'A/D/G/rho'         : Item(status='A '),
    'A/D/G/tau'         : Item(status='A '),
    'A/D/gamma'         : Item(status='A '),
    'A/C'               : Item(status='A '),
    'A/mu'              : Item(status='A '),
    'iota'              : Item(status='  ', treeconflict='C'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('iota', contents=None)

  actions.run_and_verify_checkout(url, wc_dir_other, expected_output,
    expected_disk, None, None, None, None, '--force')


#----------------------------------------------------------------------

def forced_checkout_of_dir_with_file_obstructions(sbox):
  """forced co flags conflict if a file obstructs a dir"""

  make_local_tree(sbox, False, False)

  #svntest.factory.make(sbox,"""
  #          mkdir wc_dir_other
  #          echo "The file A" > wc_dir_other/A
  #          svn co --force url wc_dir_other
  #          """)
  #svntest.factory.make(sbox,"""
  #          # Now see to it that we can recover from the obstructions.
  #          rm wc_dir_other/A
  #          svn up wc_dir_other""")
  #exit(0)

  url = sbox.repo_url
  wc_dir_other = sbox.add_wc_path('other')

  other_A = os.path.join(wc_dir_other, 'A')

  # mkdir wc_dir_other
  os.makedirs(wc_dir_other)

  # echo "The file A" > wc_dir_other/A
  svntest.main.file_write(other_A, 'The file A\n')

  # svn co --force url wc_dir_other
  expected_output = svntest.wc.State(wc_dir_other, {
    'iota'              : Item(status='A '),
    'A'                 : Item(status='  ', treeconflict='C'),
    # And what happens below A
    'A/mu'              : Item(status='  ', treeconflict='A'),
    'A/D'               : Item(status='  ', treeconflict='A'),
    'A/D/G'             : Item(status='  ', treeconflict='A'),
    'A/D/G/tau'         : Item(status='  ', treeconflict='A'),
    'A/D/G/pi'          : Item(status='  ', treeconflict='A'),
    'A/D/G/rho'         : Item(status='  ', treeconflict='A'),
    'A/D/H'             : Item(status='  ', treeconflict='A'),
    'A/D/H/psi'         : Item(status='  ', treeconflict='A'),
    'A/D/H/omega'       : Item(status='  ', treeconflict='A'),
    'A/D/H/chi'         : Item(status='  ', treeconflict='A'),
    'A/D/gamma'         : Item(status='  ', treeconflict='A'),
    'A/C'               : Item(status='  ', treeconflict='A'),
    'A/B'               : Item(status='  ', treeconflict='A'),
    'A/B/E'             : Item(status='  ', treeconflict='A'),
    'A/B/E/beta'        : Item(status='  ', treeconflict='A'),
    'A/B/E/alpha'       : Item(status='  ', treeconflict='A'),
    'A/B/F'             : Item(status='  ', treeconflict='A'),
    'A/B/lambda'        : Item(status='  ', treeconflict='A'),
  })

  expected_disk = svntest.main.greek_state.copy()
  expected_disk.remove('A/B', 'A/B/E', 'A/B/E/beta', 'A/B/E/alpha', 'A/B/F',
    'A/B/lambda', 'A/D', 'A/D/G', 'A/D/G/rho', 'A/D/G/pi', 'A/D/G/tau',
    'A/D/H', 'A/D/H/psi', 'A/D/H/omega', 'A/D/H/chi', 'A/D/gamma', 'A/mu',
    'A/C')
  expected_disk.tweak('A', contents='The file A\n')

  actions.run_and_verify_checkout(url, wc_dir_other, expected_output,
    expected_disk, None, None, None, None, '--force')


  # Now see to it that we can recover from the obstructions.
  # rm wc_dir_other/A
  os.remove(other_A)

  # svn up wc_dir_other
  expected_output = svntest.wc.State(wc_dir_other, {
  })

  expected_disk = svntest.main.greek_state.copy()

  expected_status = actions.get_virginal_state(wc_dir_other, 1)

  svntest.main.run_svn(None, 'revert', '-R', os.path.join(wc_dir_other, 'A'))

  actions.run_and_verify_update(wc_dir_other, expected_output, expected_disk,
    expected_status, None, None, None, None, None, False, wc_dir_other)


#----------------------------------------------------------------------

def forced_checkout_with_faux_obstructions(sbox):
  """co with faux obstructions ok with --force"""

  # Make a local tree that partially obstructs the paths coming from the
  # repos but has no true differences.
  expected_output = make_local_tree(sbox, False, False)

  expected_wc = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          sbox.wc_dir, expected_output,
                                          expected_wc, None, None, None,
                                          None, '--force')

#----------------------------------------------------------------------

def forced_checkout_with_real_obstructions(sbox):
  """co with real obstructions ok with --force"""

  # Make a local tree that partially obstructs the paths coming from the
  # repos and make the obstructing files different from the standard greek
  # tree.
  expected_output = make_local_tree(sbox, True, False)

  expected_wc = svntest.main.greek_state.copy()
  expected_wc.tweak('A/mu',
                    contents="This is the local version of the file 'mu'.\n")
  expected_wc.tweak('iota',
                    contents="This is the local version of the file 'iota'.\n")

  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          sbox.wc_dir, expected_output,
                                          expected_wc, None, None, None,
                                          None, '--force')

#----------------------------------------------------------------------

def forced_checkout_with_real_obstructions_and_unversioned_files(sbox):
  """co with real obstructions and unversioned files"""

  # Make a local tree that partially obstructs the paths coming from the
  # repos, make the obstructing files different from the standard greek
  # tree, and finally add some files that don't exist in the stardard tree.
  expected_output = make_local_tree(sbox, True, True)

  expected_wc = svntest.main.greek_state.copy()
  expected_wc.tweak('A/mu',
                    contents="This is the local version of the file 'mu'.\n")
  expected_wc.tweak('iota',
                    contents="This is the local version of the file 'iota'.\n")
  expected_wc.add({'sigma'     : Item("unversioned sigma"),
                   'A/upsilon' : Item("unversioned upsilon"),
                   'A/Z'       : Item(),
                   })

  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          sbox.wc_dir, expected_output,
                                          expected_wc, None, None, None,
                                          None, '--force')

#----------------------------------------------------------------------

def forced_checkout_with_versioned_obstruction(sbox):
  """forced co with versioned obstruction"""

  # Make a greek tree working copy
  sbox.build(read_only = True)

  # Create a second repository with the same greek tree
  repo_dir = sbox.repo_dir
  repo_url = sbox.repo_url
  other_repo_dir, other_repo_url = sbox.add_repo_path("other")
  svntest.main.copy_repos(repo_dir, other_repo_dir, 1, 1)

  fresh_wc_dir = sbox.add_wc_path('fresh')
  fresh_wc_dir_A = os.path.join(fresh_wc_dir, 'A')
  os.mkdir(fresh_wc_dir)

  other_wc_dir = sbox.add_wc_path("other")
  other_wc_dir_A = os.path.join(other_wc_dir, "A")
  os.mkdir(other_wc_dir)

  # Checkout "A" from the first repos to a fresh dir.
  svntest.actions.run_and_verify_svn("Unexpected error during co",
                                     svntest.verify.AnyOutput, [],
                                     "co", repo_url + "/A",
                                     fresh_wc_dir_A)

  # Checkout "A" from the second repos to the other dir.
  svntest.actions.run_and_verify_svn("Unexpected error during co",
                                     svntest.verify.AnyOutput, [],
                                     "co", other_repo_url + "/A",
                                     other_wc_dir_A)

  # Checkout the entire first repos into the fresh dir.  This should
  # fail because A is already checked out.  (Ideally, we'd silently
  # incorporate A's working copy into its parent working copy.)
  expected_output = svntest.wc.State(fresh_wc_dir, {
    'iota'              : Item(status='A '),
    'A'                 : Item(verb='Skipped'),
  })
  expected_wc = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_checkout(repo_url, fresh_wc_dir,
                                          expected_output, expected_wc,
                                          None, None, None, None,
                                          '--force')

  # Checkout the entire first repos into the other dir.  This should
  # fail because it's a different repository.
  expected_output = svntest.wc.State(other_wc_dir, {
    'iota'              : Item(status='A '),
    'A'                 : Item(verb='Skipped'),
  })
  expected_wc = svntest.main.greek_state.copy()
  svntest.actions.run_and_verify_checkout(repo_url, other_wc_dir,
                                          expected_output, expected_wc,
                                          None, None, None, None,
                                          '--force')

  #ensure that other_wc_dir_A is not affected by this forced checkout.
  svntest.actions.run_and_verify_svn("empty status output", None,
                                     [], "st", other_wc_dir_A)
  exit_code, sout, serr = svntest.actions.run_and_verify_svn(
    "it should still point to other_repo_url/A", None, [], "info",
    other_wc_dir_A)

  #TODO rename test_stderr to test_regex or something.
  test_stderr("URL: " + other_repo_url + '/A$', sout)

  #ensure that other_wc_dir is in a consistent state though it may be
  #missing few items.
  exit_code, sout, serr = svntest.actions.run_and_verify_svn(
    "it should still point to other_repo_url", None, [], "info",
    other_wc_dir)
  #TODO rename test_stderr to test_regex or something.
  test_stderr("URL: " + sbox.repo_url + '$', sout)



#----------------------------------------------------------------------
# Ensure that an import followed by a checkout in place works correctly.
def import_and_checkout(sbox):
  """import and checkout"""

  sbox.build(read_only = True)

  other_repo_dir, other_repo_url = sbox.add_repo_path("other")
  import_from_dir = sbox.add_wc_path("other")

  # Export greek tree to import_from_dir
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = import_from_dir
  expected_output.desc[''] = Item()
  expected_output.tweak(contents=None, status='A ')
  svntest.actions.run_and_verify_export(sbox.repo_url,
                                        import_from_dir,
                                        expected_output,
                                        svntest.main.greek_state.copy())

  # Create the 'other' repos
  svntest.main.create_repos(other_repo_dir)

  # Import import_from_dir to the other repos
  expected_output = svntest.wc.State(sbox.wc_dir, {})

  svntest.actions.run_and_verify_svn(None, None, [], 'import',
                                     '-m', 'import', import_from_dir,
                                     other_repo_url)

  expected_output = wc.State(import_from_dir, {
    "A"           : Item(status='E '),
    "A/B"         : Item(status='E '),
    "A/B/lambda"  : Item(status='E '),
    "A/B/E"       : Item(status='E '),
    "A/B/E/alpha" : Item(status='E '),
    "A/B/E/beta"  : Item(status='E '),
    "A/B/F"       : Item(status='E '),
    "A/mu"        : Item(status='E '),
    "A/C"         : Item(status='E '),
    "A/D"         : Item(status='E '),
    "A/D/gamma"   : Item(status='E '),
    "A/D/G"       : Item(status='E '),
    "A/D/G/pi"    : Item(status='E '),
    "A/D/G/rho"   : Item(status='E '),
    "A/D/G/tau"   : Item(status='E '),
    "A/D/H"       : Item(status='E '),
    "A/D/H/chi"   : Item(status='E '),
    "A/D/H/omega" : Item(status='E '),
    "A/D/H/psi"   : Item(status='E '),
    "iota"        : Item(status='E ')
    })

  expected_wc = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_checkout(other_repo_url, import_from_dir,
                                          expected_output, expected_wc,
                                          None, None, None, None,
                                          '--force')

#----------------------------------------------------------------------
# Issue #2529.
@Issue(2529)
def checkout_broken_eol(sbox):
  "checkout file with broken eol style"

  svntest.actions.load_repo(sbox, os.path.join(os.path.dirname(sys.argv[0]),
                                               'update_tests_data',
                                               'checkout_broken_eol.dump'))

  URL = sbox.repo_url

  expected_output = svntest.wc.State(sbox.wc_dir, {
    'file': Item(status='A '),
    })

  expected_wc = svntest.wc.State('', {
    'file': Item(contents='line\nline2\n'),
    })
  svntest.actions.run_and_verify_checkout(URL,
                                          sbox.wc_dir,
                                          expected_output,
                                          expected_wc)

def checkout_creates_intermediate_folders(sbox):
  "checkout and create some intermediate folders"

  sbox.build(create_wc = False, read_only = True)

  checkout_target = os.path.join(sbox.wc_dir, 'a', 'b', 'c')

  # checkout a working copy in a/b/c, should create these intermediate
  # folders
  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = checkout_target
  expected_output.tweak(status='A ', contents=None)

  expected_wc = svntest.main.greek_state

  svntest.actions.run_and_verify_checkout(sbox.repo_url,
                                          checkout_target,
                                          expected_output,
                                          expected_wc)

# Test that, if a peg revision is provided without an explicit revision,
# svn will checkout the directory as it was at rPEG, rather than at HEAD.
def checkout_peg_rev(sbox):
  "checkout with peg revision"

  sbox.build()
  wc_dir = sbox.wc_dir
  # create a new revision
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, 'appended mu text')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'changed file mu', wc_dir)

  # now checkout the repo@1 in another folder, this should create our initial
  # wc without the change in mu.
  checkout_target = sbox.add_wc_path('checkout')
  os.mkdir(checkout_target)

  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = checkout_target
  expected_output.tweak(status='A ', contents=None)

  expected_wc = svntest.main.greek_state.copy()

  svntest.actions.run_and_verify_checkout(sbox.repo_url + '@1',
                                          checkout_target,
                                          expected_output,
                                          expected_wc)

#----------------------------------------------------------------------
# Issue 2602: Test that peg revision dates are correctly supported.
@Issue(2602)
def checkout_peg_rev_date(sbox):
  "checkout with peg revision date"

  sbox.build()
  wc_dir = sbox.wc_dir

  # note the current time to use it as peg revision date.
  current_time = time.strftime("%Y-%m-%dT%H:%M:%S")

  # sleep till the next second.
  time.sleep(1.1)

  # create a new revision
  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, 'appended mu text')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'ci', '-m', 'changed file mu', wc_dir)

  # now checkout the repo@current_time in another folder, this should create our
  # initial wc without the change in mu.
  checkout_target = sbox.add_wc_path('checkout')
  os.mkdir(checkout_target)

  expected_output = svntest.main.greek_state.copy()
  expected_output.wc_dir = checkout_target
  expected_output.tweak(status='A ', contents=None)

  expected_wc = svntest.main.greek_state.copy()

  # use an old date to checkout, that way we're sure we get the first revision
  svntest.actions.run_and_verify_checkout(sbox.repo_url +
                                          '@{' + current_time + '}',
                                          checkout_target,
                                          expected_output,
                                          expected_wc)

#----------------------------------------------------------------------
def co_with_obstructing_local_adds(sbox):
  "co handles obstructing paths scheduled for add"

  sbox.build()
  wc_dir = sbox.wc_dir

  # Make a backup copy of the working copy
  wc_backup = sbox.add_wc_path('backup')
  svntest.actions.duplicate_dir(wc_dir, wc_backup)

  # Add files and dirs to the repos via the first WC.  Each of these
  # will be added to the backup WC via a checkout:
  #
  #  A/B/upsilon:   Identical to the file scheduled for addition in
  #                 the backup WC.
  #
  #  A/C/nu:        A "normal" add, won't exist in the backup WC.
  #
  #  A/D/kappa:     Conflicts with the file scheduled for addition in
  #                 the backup WC.
  #
  #  A/D/H/I:       New dirs that will also be scheduled for addition
  #  A/D/H/I/J:     in the backup WC.
  #  A/D/H/I/K:
  #
  #  A/D/H/I/L:     A "normal" dir add, won't exist in the backup WC.
  #
  #  A/D/H/I/K/xi:  Identical to the file scheduled for addition in
  #                 the backup WC.
  #
  #  A/D/H/I/K/eta: Conflicts with the file scheduled for addition in
  #                 the backup WC.
  upsilon_path = os.path.join(wc_dir, 'A', 'B', 'upsilon')
  svntest.main.file_append(upsilon_path, "This is the file 'upsilon'\n")
  nu_path = os.path.join(wc_dir, 'A', 'C', 'nu')
  svntest.main.file_append(nu_path, "This is the file 'nu'\n")
  kappa_path = os.path.join(wc_dir, 'A', 'D', 'kappa')
  svntest.main.file_append(kappa_path, "This is REPOS file 'kappa'\n")
  I_path = os.path.join(wc_dir, 'A', 'D', 'H', 'I')
  os.mkdir(I_path)
  J_path = os.path.join(I_path, 'J')
  os.mkdir(J_path)
  K_path = os.path.join(I_path, 'K')
  os.mkdir(K_path)
  L_path = os.path.join(I_path, 'L')
  os.mkdir(L_path)
  xi_path = os.path.join(K_path, 'xi')
  svntest.main.file_append(xi_path, "This is file 'xi'\n")
  eta_path = os.path.join(K_path, 'eta')
  svntest.main.file_append(eta_path, "This is REPOS file 'eta'\n")
  svntest.main.run_svn(None, 'add', upsilon_path, nu_path,
                       kappa_path, I_path)

  # Created expected output tree for 'svn ci'
  expected_output = wc.State(wc_dir, {
    'A/B/upsilon'   : Item(verb='Adding'),
    'A/C/nu'        : Item(verb='Adding'),
    'A/D/kappa'     : Item(verb='Adding'),
    'A/D/H/I'       : Item(verb='Adding'),
    'A/D/H/I/J'     : Item(verb='Adding'),
    'A/D/H/I/K'     : Item(verb='Adding'),
    'A/D/H/I/K/xi'  : Item(verb='Adding'),
    'A/D/H/I/K/eta' : Item(verb='Adding'),
    'A/D/H/I/L'     : Item(verb='Adding'),
    })

  # Create expected status tree.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/upsilon'   : Item(status='  ', wc_rev=2),
    'A/C/nu'        : Item(status='  ', wc_rev=2),
    'A/D/kappa'     : Item(status='  ', wc_rev=2),
    'A/D/H/I'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/eta' : Item(status='  ', wc_rev=2),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=2),
    })

  # Commit.
  svntest.actions.run_and_verify_commit(wc_dir, expected_output,
                                        expected_status, None, wc_dir)

  # Create various paths scheduled for addition which will obstruct
  # the adds coming from the repos.
  upsilon_backup_path = os.path.join(wc_backup, 'A', 'B', 'upsilon')
  svntest.main.file_append(upsilon_backup_path,
                           "This is the file 'upsilon'\n")
  kappa_backup_path = os.path.join(wc_backup, 'A', 'D', 'kappa')
  svntest.main.file_append(kappa_backup_path,
                           "This is WC file 'kappa'\n")
  I_backup_path = os.path.join(wc_backup, 'A', 'D', 'H', 'I')
  os.mkdir(I_backup_path)
  J_backup_path = os.path.join(I_backup_path, 'J')
  os.mkdir(J_backup_path)
  K_backup_path = os.path.join(I_backup_path, 'K')
  os.mkdir(K_backup_path)
  xi_backup_path = os.path.join(K_backup_path, 'xi')
  svntest.main.file_append(xi_backup_path, "This is file 'xi'\n")
  eta_backup_path = os.path.join(K_backup_path, 'eta')
  svntest.main.file_append(eta_backup_path, "This is WC file 'eta'\n")
  svntest.main.run_svn(None, 'add',
                       upsilon_backup_path,
                       kappa_backup_path,
                       I_backup_path)

  # Create expected output tree for a checkout of the wc_backup.
  expected_output = wc.State(wc_backup, {
    'A/B/upsilon'   : Item(status='E '),
    'A/C/nu'        : Item(status='A '),
    'A/D/H/I'       : Item(status='E '),
    'A/D/H/I/J'     : Item(status='E '),
    'A/D/H/I/K'     : Item(status='E '),
    'A/D/H/I/K/xi'  : Item(status='E '),
    'A/D/H/I/K/eta' : Item(status='C '),
    'A/D/H/I/L'     : Item(status='A '),
    'A/D/kappa'     : Item(status='C '),
    })

  # Create expected disk for checkout of wc_backup.
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.add({
    'A/B/upsilon'   : Item("This is the file 'upsilon'\n"),
    'A/C/nu'        : Item("This is the file 'nu'\n"),
    'A/D/H/I'       : Item(),
    'A/D/H/I/J'     : Item(),
    'A/D/H/I/K'     : Item(),
    'A/D/H/I/K/xi'  : Item("This is file 'xi'\n"),
    'A/D/H/I/K/eta' : Item("\n".join(["<<<<<<< .mine",
                                      "This is WC file 'eta'",
                                      "=======",
                                      "This is REPOS file 'eta'",
                                      ">>>>>>> .r2",
                                      ""])),
    'A/D/H/I/L'     : Item(),
    'A/D/kappa'     : Item("\n".join(["<<<<<<< .mine",
                                      "This is WC file 'kappa'",
                                      "=======",
                                      "This is REPOS file 'kappa'",
                                      ">>>>>>> .r2",
                                      ""])),
    })

  # Create expected status tree for the checkout.  Since the obstructing
  # kappa and upsilon differ from the repos, they should show as modified.
  expected_status = svntest.actions.get_virginal_state(wc_backup, 2)
  expected_status.add({
    'A/B/upsilon'   : Item(status='  ', wc_rev=2),
    'A/C/nu'        : Item(status='  ', wc_rev=2),
    'A/D/H/I'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/eta' : Item(status='C ', wc_rev=2),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=2),
    'A/D/kappa'     : Item(status='C ', wc_rev=2),
    })

  # "Extra" files that we expect to result from the conflicts.
  extra_files = ['eta\.r0', 'eta\.r2', 'eta\.mine',
                 'kappa\.r0', 'kappa\.r2', 'kappa\.mine']

  # Perform the checkout and check the results in three ways.
  # We use --force here because run_and_verify_checkout() will delete
  # wc_backup before performing the checkout otherwise.
  svntest.actions.run_and_verify_checkout(sbox.repo_url, wc_backup,
                                          expected_output, expected_disk,
                                          svntest.tree.detect_conflict_files,
                                          extra_files, None, None,
                                          '--force')

  svntest.actions.run_and_verify_status(wc_backup, expected_status)

  # Some obstructions are still not permitted:
  #
  # Test that file and dir obstructions scheduled for addition *with*
  # history fail when checkout tries to add the same path.

  # URL to URL copy of A/D/G to A/D/M.
  G_URL = sbox.repo_url + '/A/D/G'
  M_URL = sbox.repo_url + '/A/D/M'
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', G_URL, M_URL, '-m', '')

  # WC to WC copy of A/D/H to A/D/M.  (M is now scheduled for addition
  # with history in WC and pending addition from the repos).
  D_path = os.path.join(wc_dir, 'A', 'D')
  H_path = os.path.join(wc_dir, 'A', 'D', 'H')
  M_path = os.path.join(wc_dir, 'A', 'D', 'M')

  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', H_path, M_path)

  # URL to URL copy of A/B/E/alpha to A/B/F/omicron.
  omega_URL = sbox.repo_url + '/A/B/E/alpha'
  omicron_URL = sbox.repo_url + '/A/B/F/omicron'
  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', omega_URL, omicron_URL,
                                     '-m', '')

  # WC to WC copy of A/D/H/chi to /A/B/F/omicron.  (omicron is now
  # scheduled for addition with history in WC and pending addition
  # from the repos).
  F_path = os.path.join(wc_dir, 'A', 'B', 'F')
  omicron_path = os.path.join(wc_dir, 'A', 'B', 'F', 'omicron')
  chi_path = os.path.join(wc_dir, 'A', 'D', 'H', 'chi')

  svntest.actions.run_and_verify_svn("Copy error:", None, [],
                                     'cp', chi_path,
                                     omicron_path)

  # Try to co M's Parent.
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.add({
    'A/B/F/omicron' : Item(status='A ', copied='+', wc_rev='-'),
    'A/B/upsilon'   : Item(status='  ', wc_rev=2),
    'A/C/nu'        : Item(status='  ', wc_rev=2),
    'A/D/kappa'     : Item(status='  ', wc_rev=2),
    'A/D/H/I'       : Item(status='  ', wc_rev=2),
    'A/D/H/I/J'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K'     : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/xi'  : Item(status='  ', wc_rev=2),
    'A/D/H/I/K/eta' : Item(status='  ', wc_rev=2),
    'A/D/H/I/L'     : Item(status='  ', wc_rev=2),
    'A/D/M'         : Item(status='A ', copied='+', wc_rev='-'),
    'A/D/M/psi'     : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/chi'     : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/omega'   : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/I'       : Item(status='A ', copied='+', wc_rev='-',
                           entry_status='  '), # A/D/MI is a new op_root
    'A/D/M/I/J'     : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/I/K'     : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/I/K/xi'  : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/I/K/eta' : Item(status='  ', copied='+', wc_rev='-'),
    'A/D/M/I/L'     : Item(status='  ', copied='+', wc_rev='-'),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  expected_output = wc.State(wc_dir, {
    'A/D/M'         : Item(status='  ', treeconflict='C'),
    'A/D/M/rho'     : Item(status='  ', treeconflict='A'),
    'A/D/M/pi'      : Item(status='  ', treeconflict='A'),
    'A/D/M/tau'     : Item(status='  ', treeconflict='A'),
    })
  expected_disk = wc.State('', {
    'gamma'     : Item("This is the file 'gamma'.\n"),
    'G/pi'      : Item("This is the file 'pi'.\n"),
    'G/rho'     : Item("This is the file 'rho'.\n"),
    'G/tau'     : Item("This is the file 'tau'.\n"),
    'H/I'       : Item(),
    'H/I/J'     : Item(),
    'H/I/K'     : Item(),
    'H/I/K/xi'  : Item("This is file 'xi'\n"),
    'H/I/K/eta' : Item("This is REPOS file 'eta'\n"),
    'H/I/L'     : Item(),
    'H/chi'     : Item("This is the file 'chi'.\n"),
    'H/psi'     : Item("This is the file 'psi'.\n"),
    'H/omega'   : Item("This is the file 'omega'.\n"),
    'M/I'       : Item(),
    'M/I/J'     : Item(),
    'M/I/K'     : Item(),
    'M/I/K/xi'  : Item("This is file 'xi'\n"),
    'M/I/K/eta' : Item("This is REPOS file 'eta'\n"),
    'M/I/L'     : Item(),
    'M/chi'     : Item("This is the file 'chi'.\n"),
    'M/psi'     : Item("This is the file 'psi'.\n"),
    'M/omega'   : Item("This is the file 'omega'.\n"),
    'kappa'     : Item("This is REPOS file 'kappa'\n"),
    })
  svntest.actions.run_and_verify_checkout(sbox.repo_url + '/A/D',
                                          D_path,
                                          expected_output,
                                          expected_disk,
                                          None, None, None, None,
                                          '--force')

  expected_status.tweak('A/D/M', treeconflict='C', status='R ')
  expected_status.tweak(
    'A/D',
    'A/D/G',
    'A/D/G/pi',
    'A/D/G/rho',
    'A/D/G/tau',
    'A/D/gamma',
    'A/D/kappa',
    'A/D/H',
    'A/D/H/I',
    'A/D/H/I/J',
    'A/D/H/I/K',
    'A/D/H/I/K/xi',
    'A/D/H/I/K/eta',
    'A/D/H/I/L', wc_rev=4)
  expected_status.add({
    'A/D/H/chi'      : Item(status='  ', wc_rev=4),
    'A/D/H/psi'      : Item(status='  ', wc_rev=4),
    'A/D/H/omega'    : Item(status='  ', wc_rev=4),
    'A/D/M/pi'       : Item(status='D ', wc_rev=4),
    'A/D/M/rho'      : Item(status='D ', wc_rev=4),
    'A/D/M/tau'      : Item(status='D ', wc_rev=4),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

  # Try to co omicron's parent.
  expected_output = wc.State(wc_dir, {
    'A/B/F/omicron'  : Item(status='  ', treeconflict='C'),
    })
  expected_disk = wc.State('', {
    'omicron'        : Item("This is the file 'chi'.\n"),
    })
  svntest.actions.run_and_verify_checkout(sbox.repo_url + '/A/B/F',
                                          F_path,
                                          expected_output,
                                          expected_disk,
                                          None, None, None, None,
                                          '--force')

  expected_status.tweak('A/B/F/omicron', treeconflict='C', status='R ')
  expected_status.add({
    'A/B/F'         : Item(status='  ', wc_rev=4),
    })
  svntest.actions.run_and_verify_status(wc_dir, expected_status)

#----------------------------------------------------------------------
# Test if checking out from a Windows driveroot is supported.
def checkout_wc_from_drive(sbox):
  "checkout from the root of a Windows drive"

  def find_the_next_available_drive_letter():
    "find the first available drive"

    # get the list of used drive letters, use some Windows specific function.
    try:
      import win32api

      drives=win32api.GetLogicalDriveStrings()
      drives=drives.split('\000')

      for d in range(ord('G'), ord('Z')+1):
        drive = chr(d)
        if not drive + ':\\' in drives:
          return drive
    except ImportError:
      # In ActiveState python x64 win32api is not available
      for d in range(ord('G'), ord('Z')+1):
        drive = chr(d)
        if not os.path.isdir(drive + ':\\'):
          return drive

    return None

  # Skip the test if not on Windows
  if not svntest.main.windows:
    raise svntest.Skip

  # just create an empty folder, we'll checkout later.
  sbox.build(create_wc = False)
  svntest.main.safe_rmtree(sbox.wc_dir)
  os.mkdir(sbox.wc_dir)

  # create a virtual drive to the working copy folder
  drive = find_the_next_available_drive_letter()
  if drive is None:
    raise svntest.Skip

  subprocess.call(['subst', drive +':', sbox.repo_dir])
  repo_url = 'file:///' + drive + ':/'
  wc_dir = sbox.wc_dir
  was_cwd = os.getcwd()

  try:
    expected_wc = svntest.main.greek_state.copy()
    expected_output = wc.State(wc_dir, {
      'A'                 : Item(status='A '),
      'A/D'               : Item(status='A '),
      'A/D/H'             : Item(status='A '),
      'A/D/H/psi'         : Item(status='A '),
      'A/D/H/chi'         : Item(status='A '),
      'A/D/H/omega'       : Item(status='A '),
      'A/D/G'             : Item(status='A '),
      'A/D/G/tau'         : Item(status='A '),
      'A/D/G/pi'          : Item(status='A '),
      'A/D/G/rho'         : Item(status='A '),
      'A/D/gamma'         : Item(status='A '),
      'A/C'               : Item(status='A '),
      'A/mu'              : Item(status='A '),
      'A/B'               : Item(status='A '),
      'A/B/E'             : Item(status='A '),
      'A/B/E/alpha'       : Item(status='A '),
      'A/B/E/beta'        : Item(status='A '),
      'A/B/F'             : Item(status='A '),
      'A/B/lambda'        : Item(status='A '),
      'iota'              : Item(status='A '),
    })
    svntest.actions.run_and_verify_checkout(repo_url, wc_dir,
                                            expected_output, expected_wc,
                                            None, None, None, None,
                                            '--force')

  finally:
    os.chdir(was_cwd)
    # cleanup the virtual drive
    subprocess.call(['subst', '/D', drive +':'])

#----------------------------------------------------------------------

# list all tests here, starting with None:
test_list = [ None,
              checkout_with_obstructions,
              forced_checkout_of_file_with_dir_obstructions,
              forced_checkout_of_dir_with_file_obstructions,
              forced_checkout_with_faux_obstructions,
              forced_checkout_with_real_obstructions,
              forced_checkout_with_real_obstructions_and_unversioned_files,
              forced_checkout_with_versioned_obstruction,
              import_and_checkout,
              checkout_broken_eol,
              checkout_creates_intermediate_folders,
              checkout_peg_rev,
              checkout_peg_rev_date,
              co_with_obstructing_local_adds,
              checkout_wc_from_drive
            ]

if __name__ == "__main__":
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
