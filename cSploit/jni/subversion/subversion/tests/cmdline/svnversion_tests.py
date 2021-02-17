#!/usr/bin/env python
#
#  svnversion_tests.py:  testing the 'svnversion' tool.
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
import os.path
import tempfile

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

#----------------------------------------------------------------------

def svnversion_test(sbox):
  "test 'svnversion' on files and directories"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Unmodified
  svntest.actions.run_and_verify_svnversion("Unmodified working copy",
                                            wc_dir, repo_url,
                                            [ "1\n" ], [])

  # Unmodified, whole wc switched
  svntest.actions.run_and_verify_svnversion("Unmodified switched working copy",
                                            wc_dir, "some/other/url",
                                            [ "1S\n" ], [])

  mu_path = os.path.join(wc_dir, 'A', 'mu')
  svntest.main.file_append(mu_path, 'appended mu text')

  # Modified file
  svntest.actions.run_and_verify_svnversion("Modified file",
                                            mu_path, repo_url + '/A/mu',
                                            [ "1M\n" ], [])

  # Text modified
  svntest.actions.run_and_verify_svnversion("Modified text", wc_dir, repo_url,
                                            [ "1M\n" ], [])

  expected_output = wc.State(wc_dir, {'A/mu' : Item(verb='Sending')})
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/mu', wc_rev=2)
  if svntest.actions.run_and_verify_commit(wc_dir,
                                           expected_output, expected_status,
                                           None, wc_dir):
    raise svntest.Failure

  # Unmodified, mixed
  svntest.actions.run_and_verify_svnversion("Unmodified mixed working copy",
                                            wc_dir, repo_url,
                                            [ "1:2\n" ], [])

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'propset', 'blue', 'azul',
                                     os.path.join(wc_dir, 'A', 'mu'))

  # Prop modified, mixed
  svntest.actions.run_and_verify_svnversion("Property modified mixed wc",
                                            wc_dir, repo_url,
                                            [ "1:2M\n" ], [])

  iota_path = os.path.join(wc_dir, 'iota')
  gamma_url = sbox.repo_url + '/A/D/gamma'
  expected_output = wc.State(wc_dir, {'iota' : Item(status='U ')})
  expected_status.tweak('A/mu', status=' M')
  expected_status.tweak('iota', switched='S', wc_rev=2)
  expected_disk = svntest.main.greek_state.copy()
  expected_disk.tweak('A/mu',
                      contents=expected_disk.desc['A/mu'].contents
                      + 'appended mu text')
  expected_disk.tweak('iota',
                      contents=expected_disk.desc['A/D/gamma'].contents)
  if svntest.actions.run_and_verify_switch(wc_dir, iota_path, gamma_url,
                                           expected_output,
                                           expected_disk,
                                           expected_status,
                                           None, None, None, None, None,
                                           False, '--ignore-ancestry'):
    raise svntest.Failure

  # Prop modified, mixed, part wc switched
  svntest.actions.run_and_verify_svnversion("Prop-mod mixed partly switched",
                                            wc_dir, repo_url,
                                            [ "1:2MS\n" ], [])

  # Plain (exported) directory that is a direct subdir of a versioned dir
  Q_path = os.path.join(wc_dir, 'Q')
  os.mkdir(Q_path)
  svntest.actions.run_and_verify_svnversion("Exported subdirectory",
                                            Q_path, repo_url,
                                            [ "Unversioned directory\n" ], [])

  # Plain (exported) directory that is not a direct subdir of a versioned dir
  R_path = os.path.join(Q_path, 'Q')
  os.mkdir(R_path)
  svntest.actions.run_and_verify_svnversion("Exported directory",
                                            R_path, repo_url,
                                            [ "Unversioned directory\n" ], [])

  # Switched file
  svntest.actions.run_and_verify_svnversion("Switched file",
                                            iota_path, repo_url + '/iota',
                                            [ "2S\n" ], [])

  # Unversioned file
  kappa_path = os.path.join(wc_dir, 'kappa')
  svntest.main.file_write(kappa_path, "This is the file 'kappa'.")
  svntest.actions.run_and_verify_svnversion("Unversioned file",
                                            kappa_path, repo_url,
                                            [ "Unversioned file\n" ], [])

  # Nonexistent file or directory
  X_path = os.path.join(wc_dir, 'Q', 'X')
  svntest.actions.run_and_verify_svnversion("Nonexistent file or directory",
                                            X_path, repo_url,
                                            None, [ "'%s' doesn't exist\n"
                                                   % os.path.abspath(X_path) ])

  # Perform a sparse checkout of under the existing WC, and confirm that
  # svnversion detects it as a "partial" WC.
  A_path = os.path.join(wc_dir, "A")
  A_A_path = os.path.join(A_path, "SPARSE_A")
  expected_output = wc.State(A_path, {
    "SPARSE_A"    : Item(),
    "SPARSE_A/mu" : Item(status='A '),
    })
  expected_disk = wc.State("", {
    "mu" : Item(expected_disk.desc['A/mu'].contents),
    })
  svntest.actions.run_and_verify_checkout(repo_url + "/A", A_A_path,
                                          expected_output, expected_disk,
                                          None, None, None, None,
                                          "--depth=files")

  # Partial (sparse) checkout
  svntest.actions.run_and_verify_svnversion("Sparse checkout", A_A_path,
                                            repo_url, [ "2SP\n" ], [])


#----------------------------------------------------------------------

@Issue(3816)
def ignore_externals(sbox):
  "test 'svnversion' with svn:externals"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Set up an external item
  C_path = os.path.join(wc_dir, "A", "C")
  externals_desc = """\
ext-dir -r 1 %s/A/D/G
ext-file -r 1 %s/A/D/H/omega
""" % (repo_url, repo_url)
  (fd, tmp_f) = tempfile.mkstemp(dir=wc_dir)
  svntest.main.file_append(tmp_f, externals_desc)
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'pset',
                                     '-F', tmp_f, 'svn:externals', C_path)
  os.close(fd)
  os.remove(tmp_f)
  expected_output = svntest.wc.State(wc_dir, {
   'A/C' : Item(verb='Sending'),
    })
  expected_status = svntest.actions.get_virginal_state(wc_dir, 1)
  expected_status.tweak('A/C', wc_rev=2)
  svntest.actions.run_and_verify_commit(wc_dir,
                                        expected_output,
                                        expected_status,
                                        None, wc_dir)

  # Update to get it on disk
  svntest.actions.run_and_verify_svn(None, None, [], 'up', wc_dir)
  ext_dir_path = os.path.join(C_path, 'ext-dir')
  ext_file_path = os.path.join(C_path, 'ext-file')
  expected_infos = [
      { 'Revision' : '^1$' },
      { 'Revision' : '^1$' },
    ]
  svntest.actions.run_and_verify_info(expected_infos, ext_dir_path, ext_file_path)

  svntest.actions.run_and_verify_svnversion("working copy with svn:externals",
                                            wc_dir, repo_url,
                                            [ "2\n" ], [])

#----------------------------------------------------------------------

# Test for issue #3461 'excluded subtrees are not detected by svnversion'
@Issue(3461)
def svnversion_with_excluded_subtrees(sbox):
  "test 'svnversion' with excluded subtrees"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  B_path   = os.path.join(wc_dir, "A", "B")
  D_path   = os.path.join(wc_dir, "A", "D")
  psi_path = os.path.join(wc_dir, "A", "D", "H", "psi")

  svntest.actions.run_and_verify_svnversion("working copy with excluded dir",
                                            wc_dir, repo_url,
                                            [ "1\n" ], [])

  # Exclude a directory and check that svnversion detects it.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', B_path)
  svntest.actions.run_and_verify_svnversion("working copy with excluded dir",
                                            wc_dir, repo_url,
                                            [ "1P\n" ], [])

  # Exclude a file and check that svnversion detects it.  Target the
  # svnversion command on a subtree that does not contain the excluded
  # directory to assure we a detecting the switched file.
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'up', '--set-depth', 'exclude', psi_path)
  svntest.actions.run_and_verify_svnversion("working copy with excluded file",
                                            D_path, repo_url + '/A/D',
                                            [ "1P\n" ], [])

def svnversion_with_structural_changes(sbox):
  "test 'svnversion' with structural changes"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Test a copy
  iota_path = os.path.join(wc_dir, 'iota')
  iota_copy_path = os.path.join(wc_dir, 'iota_copy')

  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', iota_path, iota_copy_path)

  svntest.actions.run_and_verify_svnversion("Copied file",
                                            iota_copy_path, repo_url +
                                            '/iota_copy',
                                            [ "Uncommitted local addition, "
                                            "copy or move\n" ],
                                            [])
  C_path = os.path.join(wc_dir, 'A', 'C')
  C_copy_path = os.path.join(wc_dir, 'C_copy')
  svntest.actions.run_and_verify_svn(None, None, [],
                                     'cp', C_path, C_copy_path)

  svntest.actions.run_and_verify_svnversion("Copied dir",
                                            C_copy_path, repo_url +
                                            '/C_copy',
                                            [ "Uncommitted local addition, "
                                            "copy or move\n" ],
                                            [])
  sbox.simple_commit()

  # Test deletion
  sbox.simple_rm('iota')
  svntest.actions.run_and_verify_svnversion("Deleted file",
                                            sbox.ospath('iota'),
                                            repo_url + '/iota',
                                            ["1M\n"],
                                            [],
                                            )
  svntest.actions.run_and_verify_svnversion("Deleted file", wc_dir, repo_url,
                                            [ "1:2M\n" ], [])

def committed_revisions(sbox):
  "test 'svnversion --committed'"
  sbox.build()
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  sbox.simple_copy('iota', 'iota2')
  sbox.simple_commit()
  sbox.simple_update()
  svntest.actions.run_and_verify_svnversion("Committed revisions", wc_dir, repo_url,
                                            [ "1:2\n" ], [],
                                            "--committed")

def non_reposroot_wc(sbox):
  "test 'svnversion' on a non-repos-root working copy"
  sbox.build(create_wc=False)
  wc_dir = sbox.add_wc_path('wc2')
  repo_url = sbox.repo_url + "/A/B"
  svntest.main.run_svn(None, 'checkout', repo_url, wc_dir)
  svntest.actions.run_and_verify_svnversion("Non-repos-root wc dir",
                                            wc_dir, repo_url,
                                            [ "1\n" ], [])

@Issue(3858)
def child_switched(sbox):
  "test svnversion output for switched children"
  sbox.build()#sbox.build(read_only = True)
  wc_dir = sbox.wc_dir
  repo_url = sbox.repo_url

  # Copy A to A2
  sbox.simple_copy('A', 'branch')
  sbox.simple_commit()
  sbox.simple_update()

  ### Target is repos root and WC root.

  # No switches.
  svntest.actions.run_and_verify_svnversion(None, wc_dir, None,
                                            [ "2\n" ], [])

  # Switch A/B to a sibling.
  sbox.simple_switch(repo_url + '/A/D', 'A/B')

  # This should detect the switch at A/B.
  svntest.actions.run_and_verify_svnversion(None, wc_dir, None,
                                            [ "2S\n" ], [])

  ### Target is neither repos root nor WC root.

  # But A/B/G and its children are not switched by itself.
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'A/B/G'),
                                            None, [ "2\n" ], [])

  # And A/B isn't switched when you look at it directly.
  svntest.actions.run_and_verify_svnversion(None, os.path.join(wc_dir, 'A/B'),
                                            None, [ "2\n" ], [])

  # Switch branch/D to ^/A, then switch branch/D/G back to ^/branch/D/G so
  # the latter is switched relative to its parent but not the WC root.
  sbox.simple_switch(repo_url + '/A/D', 'branch/D')
  sbox.simple_switch(repo_url + '/branch/D/G', 'branch/D/G')

  # This should detect the switch at branch/D and branch/D/G.
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'branch'),
                                            None, [ "2S\n" ], [])

  # Directly targeting the switched branch/D should still detect the switch
  # at branch/D/G even though the latter isn't switched against the root of
  # the working copy.
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'branch',
                                                         'D'),
                                            None, [ "2S\n" ], [])

  # Switch A/B to ^/.
  sbox.simple_switch(repo_url, 'A/B')
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir),
                                            None, [ "2S\n" ], [])
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'A'),
                                            None, [ "2S\n" ], [])

  ### Target is repos root but not WC root.

  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'A', 'B'),
                                            None, [ "2\n" ], [])

  # Switch A/B/A/D/G to ^/A/D/H.
  sbox.simple_switch(repo_url + '/A/D/H', 'A/B/A/D/G')
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'A', 'B'),
                                            None, [ "2S\n" ], [])

  ### Target is not repos root but is WC root.

  # Switch the root of the working copy to ^/branch, then switch D/G to
  # ^A/D/G.
  sbox.simple_switch(repo_url + '/branch', '.')
  sbox.simple_switch(repo_url + '/A/D/G', 'D/G')
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir,),
                                            None, [ "2S\n" ], [])

  ### Target is neither repos root nor WC root.

  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'D'),
                                            None, [ "2S\n" ], [])
  svntest.actions.run_and_verify_svnversion(None,
                                            os.path.join(wc_dir, 'D', 'H'),
                                            None, [ "2\n" ], [])

########################################################################
# Run the tests


# list all tests here, starting with None:
test_list = [ None,
              svnversion_test,
              ignore_externals,
              svnversion_with_excluded_subtrees,
              svnversion_with_structural_changes,
              committed_revisions,
              non_reposroot_wc,
              child_switched,
             ]

if __name__ == '__main__':
  svntest.main.run_tests(test_list)
  # NOTREACHED


### End of file.
