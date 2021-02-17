#!/usr/bin/env python
#
#
# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.
#
#

# Usage: svnmucc-test.py [build-dir-top [base-url]]

import sys
import os
import re
import shutil

# calculate the absolute directory in which this test script lives
this_dir = os.path.dirname(os.path.abspath(sys.argv[0]))

# add the Subversion Python test suite libraries to the path, and import
sys.path.insert(0, '%s/../../../subversion/tests/cmdline' % (this_dir))
import svntest

# setup the global 'svntest.main.options' object so functions in the
# module don't freak out.
svntest.main._parse_options(arglist=[])

# calculate the top of the build tree
if len(sys.argv) > 1:
  build_top = os.path.abspath(sys.argv[1])
else:
  build_top = os.path.abspath('%s/../../../' % (this_dir))

# where lives svnmucc?
svnmucc_binary = \
   os.path.abspath('%s/tools/client-side/svnmucc/svnmucc' % (build_top))

# override some svntest binary locations
svntest.main.svn_binary = \
   os.path.abspath('%s/subversion/svn/svn' % (build_top))
svntest.main.svnlook_binary = \
   os.path.abspath('%s/subversion/svnlook/svnlook' % (build_top))
svntest.main.svnadmin_binary = \
   os.path.abspath('%s/subversion/svnadmin/svnadmin' % (build_top))

# where lives the test repository?
repos_path = \
   os.path.abspath(('%s/tools/client-side/svnmucc/svnmucc-test-repos'
                    % (build_top)))

if (len(sys.argv) > 2):
  repos_url = sys.argv[2] + '/svnmucc-test-repos'
else:
  repos_url = 'file://' + repos_path

def die(msg):
  """Write MSG (formatted as a failure) to stderr, and exit with a
  non-zero errorcode."""

  sys.stderr.write("FAIL: " + msg + "\n")
  sys.exit(1)


_svnmucc_re = re.compile('^(r[0-9]+) committed by svnmuccuser at (.*)$')
_log_re = re.compile('^   ([ADRM] /[^\(]+($| \(from .*:[0-9]+\)$))')
_err_re = re.compile('^svnmucc: (.*)$')

def xrun_svnmucc(expected_errors, *varargs):
  """Run svnmucc with the list of SVNMUCC_ARGS arguments.  Verify that
  its run results match the list of EXPECTED_ERRORS."""

  # First, run svnmucc.
  exit_code, outlines, errlines = \
    svntest.main.run_command(svnmucc_binary, 1, 0,
                             '-U', repos_url,
                             '-u', 'svnmuccuser',
                             '-p', 'svnmuccpass',
                             '--config-dir', 'dummy',
                             *varargs)
  errors = []
  for line in errlines:
    match = _err_re.match(line)
    if match:
      errors.append(line.rstrip('\n\r'))
  if errors != expected_errors:
    raise svntest.main.SVNUnmatchedError(str(errors))


def run_svnmucc(expected_path_changes, *varargs):
  """Run svnmucc with the list of SVNMUCC_ARGS arguments.  Verify that
  its run results in a new commit with 'svn log -rHEAD' changed paths
  that match the list of EXPECTED_PATH_CHANGES."""

  # First, run svnmucc.
  exit_code, outlines, errlines = \
    svntest.main.run_command(svnmucc_binary, 1, 0,
                             '-U', repos_url,
                             '-u', 'svnmuccuser',
                             '-p', 'svnmuccpass',
                             '--config-dir', 'dummy',
                             *varargs)
  if errlines:
    raise svntest.main.SVNCommitFailure(str(errlines))
  if len(outlines) != 1 or not _svnmucc_re.match(outlines[0]):
    raise svntest.main.SVNLineUnequal(str(outlines))

  # Now, run 'svn log -vq -rHEAD'
  changed_paths = []
  exit_code, outlines, errlines = \
    svntest.main.run_svn(None, 'log', '-vqrHEAD', repos_url)
  if errlines:
    raise svntest.Failure("Unable to verify commit with 'svn log': %s"
                          % (str(errlines)))
  for line in outlines:
    match = _log_re.match(line)
    if match:
      changed_paths.append(match.group(1).rstrip('\n\r'))

  expected_path_changes.sort()
  changed_paths.sort()
  if changed_paths != expected_path_changes:
    raise svntest.Failure("Logged path changes differ from expectations\n"
                          "   expected: %s\n"
                          "     actual: %s" % (str(expected_path_changes),
                                               str(changed_paths)))


def main():
  """Test svnmucc."""

  # revision 1
  run_svnmucc(['A /foo'
               ], # ---------
              'mkdir', 'foo')

  # revision 2
  run_svnmucc(['A /z.c',
               ], # ---------
              'put', '/dev/null', 'z.c')

  # revision 3
  run_svnmucc(['A /foo/z.c (from /z.c:2)',
               'A /foo/bar (from /foo:2)',
               ], # ---------
              'cp', '2', 'z.c', 'foo/z.c',
              'cp', '2', 'foo', 'foo/bar')

  # revision 4
  run_svnmucc(['A /zig (from /foo:3)',
               'D /zig/bar',
               'D /foo',
               'A /zig/zag (from /foo:3)',
               ], # ---------
              'cp', '3', 'foo', 'zig',
              'rm',             'zig/bar',
              'mv',      'foo', 'zig/zag')

  # revision 5
  run_svnmucc(['D /z.c',
               'A /zig/zag/bar/y.c (from /z.c:4)',
               'A /zig/zag/bar/x.c (from /z.c:2)',
               ], # ---------
              'mv',      'z.c', 'zig/zag/bar/y.c',
              'cp', '2', 'z.c', 'zig/zag/bar/x.c')

  # revision 6
  run_svnmucc(['D /zig/zag/bar/y.c',
               'A /zig/zag/bar/y y.c (from /zig/zag/bar/y.c:5)',
               'A /zig/zag/bar/y%20y.c (from /zig/zag/bar/y.c:5)',
               ], # ---------
              'mv',         'zig/zag/bar/y.c', 'zig/zag/bar/y%20y.c',
              'cp', 'HEAD', 'zig/zag/bar/y.c', 'zig/zag/bar/y%2520y.c')

  # revision 7
  run_svnmucc(['D /zig/zag/bar/y y.c',
               'A /zig/zag/bar/z z1.c (from /zig/zag/bar/y y.c:6)',
               'A /zig/zag/bar/z%20z.c (from /zig/zag/bar/y%20y.c:6)',
               'A /zig/zag/bar/z z2.c (from /zig/zag/bar/y y.c:6)',
               ], #---------
              'mv',         'zig/zag/bar/y%20y.c',   'zig/zag/bar/z z1.c',
              'cp', 'HEAD', 'zig/zag/bar/y%2520y.c', 'zig/zag/bar/z%2520z.c',
              'cp', 'HEAD', 'zig/zag/bar/y y.c',     'zig/zag/bar/z z2.c')

  # revision 8
  run_svnmucc(['D /zig/zag',
               'A /zig/foo (from /zig/zag:7)',
               'D /zig/foo/bar/z%20z.c',
               'D /zig/foo/bar/z z2.c',
               'R /zig/foo/bar/z z1.c (from /zig/zag/bar/x.c:5)',
               ], #---------
              'mv',      'zig/zag',         'zig/foo',
              'rm',                         'zig/foo/bar/z z1.c',
              'rm',                         'zig/foo/bar/z%20z2.c',
              'rm',                         'zig/foo/bar/z%2520z.c',
              'cp', '5', 'zig/zag/bar/x.c', 'zig/foo/bar/z%20z1.c')

  # revision 9
  run_svnmucc(['R /zig/foo/bar (from /zig/z.c:8)',
               ], #---------
              'rm',                 'zig/foo/bar',
              'cp', '8', 'zig/z.c', 'zig/foo/bar')

  # revision 10
  run_svnmucc(['R /zig/foo/bar (from /zig/foo/bar:8)',
               'D /zig/foo/bar/z z1.c',
               ], #---------
              'rm',                     'zig/foo/bar',
              'cp', '8', 'zig/foo/bar', 'zig/foo/bar',
              'rm',                     'zig/foo/bar/z%20z1.c')

  # revision 11
  run_svnmucc(['R /zig/foo (from /zig/foo/bar:10)',
               ], #---------
              'rm',                        'zig/foo',
              'cp', 'head', 'zig/foo/bar', 'zig/foo')

  # revision 12
  run_svnmucc(['D /zig',
               'A /foo (from /foo:3)',
               'A /foo/foo (from /foo:3)',
               'A /foo/foo/foo (from /foo:3)',
               'D /foo/foo/bar',
               'R /foo/foo/foo/bar (from /foo:3)',
               ], #---------
              'rm',             'zig',
              'cp', '3', 'foo', 'foo',
              'cp', '3', 'foo', 'foo/foo',
              'cp', '3', 'foo', 'foo/foo/foo',
              'rm',             'foo/foo/bar',
              'rm',             'foo/foo/foo/bar',
              'cp', '3', 'foo', 'foo/foo/foo/bar')

  # revision 13
  run_svnmucc(['A /boozle (from /foo:3)',
               'A /boozle/buz',
               'A /boozle/buz/nuz',
               ], #---------
              'cp',    '3', 'foo', 'boozle',
              'mkdir',             'boozle/buz',
              'mkdir',             'boozle/buz/nuz')

  # revision 14
  run_svnmucc(['A /boozle/buz/svnmucc-test.py',
               'A /boozle/guz (from /boozle/buz:13)',
               'A /boozle/guz/svnmucc-test.py',
               ], #---------
              'put',      '/dev/null',  'boozle/buz/svnmucc-test.py',
              'cp', '13', 'boozle/buz', 'boozle/guz',
              'put',      '/dev/null',  'boozle/guz/svnmucc-test.py')

  # revision 15
  run_svnmucc(['M /boozle/buz/svnmucc-test.py',
               'R /boozle/guz/svnmucc-test.py',
               ], #---------
              'put', sys.argv[0], 'boozle/buz/svnmucc-test.py',
              'rm',               'boozle/guz/svnmucc-test.py',
              'put', sys.argv[0], 'boozle/guz/svnmucc-test.py')

  # revision 16
  run_svnmucc(['R /foo/bar (from /foo/foo:15)'], #---------
              'rm',                            'foo/bar',
              'cp', '15', 'foo/foo',           'foo/bar',
              'propset',  'testprop',  'true', 'foo/bar')

  # revision 17
  run_svnmucc(['M /foo/bar'], #---------
              'propdel', 'testprop', 'foo/bar')

  # revision 18
  run_svnmucc(['M /foo/z.c',
               'M /foo/foo',
               ], #---------
              'propset', 'testprop', 'true', 'foo/z.c',
              'propset', 'testprop', 'true', 'foo/foo')

  # revision 19
  run_svnmucc(['M /foo/z.c',
               'M /foo/foo',
               ], #---------
              'propsetf', 'testprop', sys.argv[0], 'foo/z.c',
              'propsetf', 'testprop', sys.argv[0], 'foo/foo')

  # Expected missing revision error
  xrun_svnmucc(["svnmucc: E200004: 'a' is not a revision"
                ], #---------
              'cp', 'a', 'b')

  # Expected cannot be younger error
  xrun_svnmucc(['svnmucc: E205000: Copy source revision cannot be younger ' +
                'than base revision',
                ], #---------
              'cp', '42', 'a', 'b')

  # Expected already exists error
  xrun_svnmucc(["svnmucc: E125002: 'foo' already exists",
                ], #---------
              'cp', '17', 'a', 'foo')

  # Expected copy_src already exists error
  xrun_svnmucc(["svnmucc: E125002: 'a/bar' (from 'foo/bar:17') already exists",
                ], #---------
              'cp', '17', 'foo', 'a',
              'cp', '17', 'foo/foo', 'a/bar')

  # Expected not found error
  xrun_svnmucc(["svnmucc: E125002: 'a' not found",
                ], #---------
              'cp', '17', 'a', 'b')

if __name__ == "__main__":
  try:
    # remove any previously existing repository, then create a new one
    if os.path.exists(repos_path):
      shutil.rmtree(repos_path)
    exit_code, outlines, errlines = \
      svntest.main.run_svnadmin('create', '--fs-type',
                                'fsfs', repos_path)
    if errlines:
      raise svntest.main.SVNRepositoryCreateFailure(repos_path)
    fp = open(os.path.join(repos_path, 'conf', 'svnserve.conf'), 'w')
    fp.write('[general]\nauth-access = write\npassword-db = passwd\n')
    fp.close()
    fp = open(os.path.join(repos_path, 'conf', 'passwd'), 'w')
    fp.write('[users]\nsvnmuccuser = svnmuccpass\n')
    fp.close()
    main()
  except SystemExit, e:
    raise
  except svntest.main.SVNCommitFailure, e:
    die("Error committing via svnmucc: %s" % (str(e)))
  except svntest.main.SVNLineUnequal, e:
    die("Unexpected svnmucc output line: %s" % (str(e)))
  except svntest.main.SVNRepositoryCreateFailure, e:
    die("Error creating test repository: %s" % (str(e)))
  except svntest.Failure, e:
    die("Test failed: %s" % (str(e)))
  except Exception, e:
    die("Something bad happened: %s" % (str(e)))

  # cleanup the repository on a successful run
  try:
    if os.path.exists(repos_path):
      shutil.rmtree(repos_path)
  except:
    pass
  print("SUCCESS!")
