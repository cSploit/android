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
import unittest, setup_path
from sys import version_info # For Python version check
if version_info[0] >= 3:
  # Python >=3.0
  from io import StringIO
else:
  # Python <3.0
  from StringIO import StringIO
from svn import core, repos, fs, delta
from svn.core import SubversionException
import utils

class ChangeReceiver(delta.Editor):
  """A delta editor which saves textdeltas for later use"""

  def __init__(self, src_root, tgt_root):
    self.src_root = src_root
    self.tgt_root = tgt_root
    self.textdeltas = []

  def apply_textdelta(self, file_baton, base_checksum, pool=None):
    def textdelta_handler(textdelta):
      if textdelta is not None:
        self.textdeltas.append(textdelta)
    return textdelta_handler

def _authz_callback(root, path, pool):
  "A dummy authz callback which always returns success."
  return 1

class SubversionRepositoryTestCase(unittest.TestCase):
  """Test cases for the Subversion repository layer"""

  def setUp(self):
    """Load a Subversion repository"""
    self.temper = utils.Temper()
    (self.repos, _, _) = self.temper.alloc_known_repo(
      'trac/versioncontrol/tests/svnrepos.dump', suffix='-repository')
    self.fs = repos.fs(self.repos)
    self.rev = fs.youngest_rev(self.fs)

  def tearDown(self):
    self.fs = None
    self.repos = None
    self.temper.cleanup()

  def test_cease_invocation(self):
    """Test returning SVN_ERR_CEASE_INVOCATION from a callback"""

    revs = []
    def history_lookup(path, rev, pool):
      revs.append(rev)
      raise core.SubversionException(apr_err=core.SVN_ERR_CEASE_INVOCATION,
                                     message="Hi from history_lookup")

    repos.history2(self.fs, '/trunk/README2.txt', history_lookup, None, 0,
                   self.rev, True)
    self.assertEqual(len(revs), 1)

  def test_create(self):
    """Make sure that repos.create doesn't segfault when we set fs-type
       using a config hash"""
    fs_config = { "fs-type": "fsfs" }
    for i in range(5):
      path = self.temper.alloc_empty_dir(suffix='-repository-create%d' % i)
      repos.create(path, "", "", None, fs_config)

  def test_dump_fs2(self):
    """Test the dump_fs2 function"""

    self.callback_calls = 0

    def is_cancelled():
      self.callback_calls += 1
      return None

    dumpstream = StringIO()
    feedbackstream = StringIO()
    repos.dump_fs2(self.repos, dumpstream, feedbackstream, 0, self.rev, 0, 0,
                   is_cancelled)

    # Check that we can dump stuff
    dump = dumpstream.getvalue()
    feedback = feedbackstream.getvalue()
    expected_feedback = "* Dumped revision " + str(self.rev)
    self.assertEquals(dump.count("Node-path: trunk/README.txt"), 2)
    self.assertEquals(feedback.count(expected_feedback), 1)
    self.assertEquals(self.callback_calls, 13)

    # Check that the dump can be cancelled
    self.assertRaises(SubversionException, repos.dump_fs2,
      self.repos, dumpstream, feedbackstream, 0, self.rev, 0, 0, lambda: 1)

    dumpstream.close()
    feedbackstream.close()

    # Check that the dump fails when the dumpstream is closed
    self.assertRaises(ValueError, repos.dump_fs2,
      self.repos, dumpstream, feedbackstream, 0, self.rev, 0, 0, None)

    dumpstream = StringIO()
    feedbackstream = StringIO()

    # Check that we can grab the feedback stream, but not the dumpstream
    repos.dump_fs2(self.repos, None, feedbackstream, 0, self.rev, 0, 0, None)
    feedback = feedbackstream.getvalue()
    self.assertEquals(feedback.count(expected_feedback), 1)

    # Check that we can grab the dumpstream, but not the feedbackstream
    repos.dump_fs2(self.repos, dumpstream, None, 0, self.rev, 0, 0, None)
    dump = dumpstream.getvalue()
    self.assertEquals(dump.count("Node-path: trunk/README.txt"), 2)

    # Check that we can ignore both the dumpstream and the feedbackstream
    repos.dump_fs2(self.repos, dumpstream, None, 0, self.rev, 0, 0, None)
    self.assertEquals(feedback.count(expected_feedback), 1)

    # FIXME: The Python bindings don't check for 'NULL' values for
    #        svn_repos_t objects, so the following call segfaults
    #repos.dump_fs2(None, None, None, 0, self.rev, 0, 0, None)

  def test_get_logs(self):
    """Test scope of get_logs callbacks"""
    logs = []
    def addLog(paths, revision, author, date, message, pool):
      if paths is not None:
        logs.append(paths)

    # Run get_logs
    repos.get_logs(self.repos, ['/'], self.rev, 0, True, 0, addLog)

    # Count and verify changes
    change_count = 0
    for log in logs:
      for path_changed in log.values():
        change_count += 1
        path_changed.assert_valid()
    self.assertEqual(logs[2]["/tags/v1.1"].action, "A")
    self.assertEqual(logs[2]["/tags/v1.1"].copyfrom_path, "/branches/v1x")
    self.assertEqual(len(logs), 12)
    self.assertEqual(change_count, 19)

  def test_dir_delta(self):
    """Test scope of dir_delta callbacks"""
    # Run dir_delta
    this_root = fs.revision_root(self.fs, self.rev)
    prev_root = fs.revision_root(self.fs, self.rev-1)
    editor = ChangeReceiver(this_root, prev_root)
    e_ptr, e_baton = delta.make_editor(editor)
    repos.dir_delta(prev_root, '', '', this_root, '', e_ptr, e_baton,
                    _authz_callback, 1, 1, 0, 0)

    # Check results.
    # Ignore the order in which the editor delivers the two sibling files.
    self.assertEqual(set([editor.textdeltas[0].new_data,
                          editor.textdeltas[1].new_data]),
                     set(["This is a test.\n", "A test.\n"]))
    self.assertEqual(len(editor.textdeltas), 2)

  def test_retrieve_and_change_rev_prop(self):
    """Test playing with revprops"""
    self.assertEqual(repos.fs_revision_prop(self.repos, self.rev, "svn:log",
                                            _authz_callback),
                     "''(a few years later)'' Argh... v1.1 was buggy, "
                     "after all")

    # We expect this to complain because we have no pre-revprop-change
    # hook script for the repository.
    self.assertRaises(SubversionException, repos.fs_change_rev_prop3,
                      self.repos, self.rev, "jrandom", "svn:log",
                      "Youngest revision", True, True, _authz_callback)

    repos.fs_change_rev_prop3(self.repos, self.rev, "jrandom", "svn:log",
                              "Youngest revision", False, False,
                              _authz_callback)

    self.assertEqual(repos.fs_revision_prop(self.repos, self.rev, "svn:log",
                                            _authz_callback),
                     "Youngest revision")

def suite():
    return unittest.defaultTestLoader.loadTestsFromTestCase(
      SubversionRepositoryTestCase)

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
