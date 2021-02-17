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
# Copyright (C) 2005 Edgewall Software
# Copyright (C) 2005 Christopher Lenz <cmlenz@gmx.de>
#
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
#
# 1. Redistributions of source code must retain the above copyright
#    notice, this list of conditions and the following disclaimer.
# 2. Redistributions in binary form must reproduce the above copyright
#    notice, this list of conditions and the following disclaimer in the
#    the documentation and/or other materials provided with the
#    distribution.
# 3. The name of the author may not be used to endorse or promote
#    products derived from this software without specific prior written
#    permission.
#
# THIS SOFTWARE IS PROVIDED BY THE AUTHOR "AS IS" AND ANY EXPRESS
# OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
# WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
# ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY
# DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
# GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
# IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
# OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN
# IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

import os.path
import stat
import shutil
import sys
import tempfile
import unittest
from urllib import pathname2url

if sys.version_info[0] >= 3:
  # Python >=3.0
  from io import StringIO
else:
  # Python <3.0
  try:
    from cStringIO import StringIO
  except ImportError:
    from StringIO import StringIO

from svn import core, repos

from trac.test import TestSetup
from trac.versioncontrol import Changeset, Node
from trac.versioncontrol.svn_fs import SubversionRepository

temp_path = tempfile.mktemp("-trac-svnrepos")
REPOS_PATH = core.svn_dirent_internal_style(temp_path)
REPOS_URL = pathname2url(temp_path)
del temp_path

if REPOS_URL.startswith("///"):
  # Don't add extra slashes if they're already present.
  # (This is important for Windows compatibility).
  REPOS_URL = "file:" + REPOS_URL
else:
  # If the URL simply starts with '/', we need to add two
  # extra slashes to make it a valid 'file://' URL
  REPOS_URL = "file://" + REPOS_URL

REPOS_URL = core.svn_uri_canonicalize(REPOS_URL)

class SubversionRepositoryTestSetup(TestSetup):

    def setUp(self):
        dumpfile = open(os.path.join(os.path.split(__file__)[0],
                                     'svnrepos.dump'), 'rb')

        # Remove the trac-svnrepos directory, so that we can
        # ensure a fresh start.
        self.tearDown()

        r = repos.svn_repos_create(REPOS_PATH, '', '', None, None)
        repos.svn_repos_load_fs2(r, dumpfile, StringIO(),
                                repos.svn_repos_load_uuid_ignore, '',
                                0, 0, None)

    def tearDown(self):
        if os.path.exists(REPOS_PATH):
            repos.delete(REPOS_PATH)


class SubversionRepositoryTestCase(unittest.TestCase):

    def setUp(self):
        self.repos = SubversionRepository(REPOS_PATH, None)

    def tearDown(self):
        self.repos = None

    def test_rev_navigation(self):
        self.assertEqual(0, self.repos.oldest_rev)
        self.assertEqual(None, self.repos.previous_rev(0))
        self.assertEqual(0, self.repos.previous_rev(1))
        self.assertEqual(12, self.repos.youngest_rev)
        self.assertEqual(6, self.repos.next_rev(5))
        self.assertEqual(7, self.repos.next_rev(6))
        # ...
        self.assertEqual(None, self.repos.next_rev(12))

    def test_get_node(self):
        node = self.repos.get_node('/trunk')
        self.assertEqual('trunk', node.name)
        self.assertEqual('/trunk', node.path)
        self.assertEqual(Node.DIRECTORY, node.kind)
        self.assertEqual(6, node.rev)
        self.assertEqual(1112381806, node.last_modified)
        node = self.repos.get_node('/trunk/README.txt')
        self.assertEqual('README.txt', node.name)
        self.assertEqual('/trunk/README.txt', node.path)
        self.assertEqual(Node.FILE, node.kind)
        self.assertEqual(3, node.rev)
        self.assertEqual(1112361898, node.last_modified)

    def test_get_node_specific_rev(self):
        node = self.repos.get_node('/trunk', 1)
        self.assertEqual('trunk', node.name)
        self.assertEqual('/trunk', node.path)
        self.assertEqual(Node.DIRECTORY, node.kind)
        self.assertEqual(1, node.rev)
        self.assertEqual(1112349652, node.last_modified)
        node = self.repos.get_node('/trunk/README.txt', 2)
        self.assertEqual('README.txt', node.name)
        self.assertEqual('/trunk/README.txt', node.path)
        self.assertEqual(Node.FILE, node.kind)
        self.assertEqual(2, node.rev)
        self.assertEqual(1112361138, node.last_modified)

    def test_get_dir_entries(self):
        node = self.repos.get_node('/trunk')
        entries = node.get_entries()
        self.assertEqual('README2.txt', entries.next().name)
        self.assertEqual('dir1', entries.next().name)
        self.assertEqual('README.txt', entries.next().name)
        self.assertRaises(StopIteration, entries.next)

    def test_get_file_entries(self):
        node = self.repos.get_node('/trunk/README.txt')
        entries = node.get_entries()
        self.assertRaises(StopIteration, entries.next)

    def test_get_dir_content(self):
        node = self.repos.get_node('/trunk')
        self.assertEqual(None, node.content_length)
        self.assertEqual(None, node.content_type)
        self.assertEqual(None, node.get_content())

    def test_get_file_content(self):
        node = self.repos.get_node('/trunk/README.txt')
        self.assertEqual(8, node.content_length)
        self.assertEqual('text/plain', node.content_type)
        self.assertEqual('A test.\n', node.get_content().read())

    def test_get_dir_properties(self):
        f = self.repos.get_node('/trunk')
        props = f.get_properties()
        self.assertEqual(0, len(props))

    def test_get_file_properties(self):
        f = self.repos.get_node('/trunk/README.txt')
        props = f.get_properties()
        self.assertEqual('native', props['svn:eol-style'])
        self.assertEqual('text/plain', props['svn:mime-type'])

    # Revision Log / node history

    def test_get_node_history(self):
        node = self.repos.get_node('/trunk/README2.txt')
        history = node.get_history()
        self.assertEqual(('trunk/README2.txt', 6, 'copy'), history.next())
        self.assertEqual(('trunk/README.txt', 3, 'edit'), history.next())
        self.assertEqual(('trunk/README.txt', 2, 'add'), history.next())
        self.assertRaises(StopIteration, history.next)

    def test_get_node_history_follow_copy(self):
        node = self.repos.get_node('/tags/v1/README.txt')
        history = node.get_history()
        self.assertEqual(('tags/v1/README.txt', 7, 'copy'), history.next())
        self.assertEqual(('trunk/README.txt', 3, 'edit'), history.next())
        self.assertEqual(('trunk/README.txt', 2, 'add'), history.next())
        self.assertRaises(StopIteration, history.next)

    # Revision Log / path history

    def test_get_path_history(self):
        history = self.repos.get_path_history('/trunk/README2.txt', None)
        self.assertEqual(('trunk/README2.txt', 6, 'copy'), history.next())
        self.assertEqual(('trunk/README.txt', 3, 'unknown'), history.next())
        self.assertRaises(StopIteration, history.next)

    def test_get_path_history_copied_file(self):
        history = self.repos.get_path_history('/tags/v1/README.txt', None)
        self.assertEqual(('tags/v1/README.txt', 7, 'copy'), history.next())
        self.assertEqual(('trunk/README.txt', 3, 'unknown'), history.next())
        self.assertRaises(StopIteration, history.next)

    def test_get_path_history_copied_dir(self):
        history = self.repos.get_path_history('/branches/v1x', None)
        self.assertEqual(('branches/v1x', 12, 'copy'), history.next())
        self.assertEqual(('tags/v1.1', 10, 'unknown'), history.next())
        self.assertEqual(('branches/v1x', 11, 'delete'), history.next())
        self.assertEqual(('branches/v1x', 9, 'edit'), history.next())
        self.assertEqual(('branches/v1x', 8, 'copy'), history.next())
        self.assertEqual(('tags/v1', 7, 'unknown'), history.next())
        self.assertRaises(StopIteration, history.next)

    # Diffs

    def _cmp_diff(self, expected, got):
        if expected[0]:
            old = self.repos.get_node(*expected[0])
            self.assertEqual((old.path, old.rev), (got[0].path, got[0].rev))
        if expected[1]:
            new = self.repos.get_node(*expected[1])
            self.assertEqual((new.path, new.rev), (got[1].path, got[1].rev))
        self.assertEqual(expected[2], (got[2], got[3]))

    def test_diff_file_different_revs(self):
        diffs = self.repos.get_deltas('trunk/README.txt', 2, 'trunk/README.txt', 3)
        self._cmp_diff((('trunk/README.txt', 2),
                        ('trunk/README.txt', 3),
                        (Node.FILE, Changeset.EDIT)), diffs.next())
        self.assertRaises(StopIteration, diffs.next)

    def test_diff_file_different_files(self):
        diffs = self.repos.get_deltas('branches/v1x/README.txt', 12,
                                      'branches/v1x/README2.txt', 12)
        self._cmp_diff((('branches/v1x/README.txt', 12),
                        ('branches/v1x/README2.txt', 12),
                        (Node.FILE, Changeset.EDIT)), diffs.next())
        self.assertRaises(StopIteration, diffs.next)

    def test_diff_file_no_change(self):
        diffs = self.repos.get_deltas('trunk/README.txt', 7,
                                      'tags/v1/README.txt', 7)
        self.assertRaises(StopIteration, diffs.next)

    def test_diff_dir_different_revs(self):
        diffs = self.repos.get_deltas('trunk', 4, 'trunk', 8)
        expected = [
          (None, ('trunk/README2.txt', 6),
           (Node.FILE, Changeset.ADD)),
          (None, ('trunk/dir1/dir2', 8),
           (Node.DIRECTORY, Changeset.ADD)),
          (None, ('trunk/dir1/dir3', 8),
           (Node.DIRECTORY, Changeset.ADD)),
          (('trunk/dir2', 4), None,
           (Node.DIRECTORY, Changeset.DELETE)),
          (('trunk/dir3', 4), None,
           (Node.DIRECTORY, Changeset.DELETE)),
        ]
        actual = [diffs.next() for i in range(5)]
        actual = sorted(actual,
                        key=lambda diff: ((diff[0] or diff[1]).path,
                                          (diff[0] or diff[1]).rev))
        self.assertEqual(len(expected), len(actual))
        for e,a in zip(expected, actual):
          self._cmp_diff(e,a)
        self.assertRaises(StopIteration, diffs.next)

    def test_diff_dir_different_dirs(self):
        diffs = self.repos.get_deltas('trunk', 1, 'branches/v1x', 12)
        expected = [
          (None, ('branches/v1x/README.txt', 12),
           (Node.FILE, Changeset.ADD)),
          (None, ('branches/v1x/README2.txt', 12),
           (Node.FILE, Changeset.ADD)),
          (None, ('branches/v1x/dir1', 12),
           (Node.DIRECTORY, Changeset.ADD)),
          (None, ('branches/v1x/dir1/dir2', 12),
           (Node.DIRECTORY, Changeset.ADD)),
          (None, ('branches/v1x/dir1/dir3', 12),
           (Node.DIRECTORY, Changeset.ADD)),
        ]
        actual = [diffs.next() for i in range(5)]
        actual = sorted(actual, key=lambda diff: (diff[1].path, diff[1].rev))
        # for e,a in zip(expected, actual):
        #   t.write("%r\n" % (e,))
        #   t.write("%r\n" % ((None, (a[1].path, a[1].rev), (a[2], a[3])),) )
        #   t.write('\n')
        self.assertEqual(len(expected), len(actual))
        for e,a in zip(expected, actual):
          self._cmp_diff(e,a)
        self.assertRaises(StopIteration, diffs.next)

    def test_diff_dir_no_change(self):
        diffs = self.repos.get_deltas('trunk', 7,
                                      'tags/v1', 7)
        self.assertRaises(StopIteration, diffs.next)

    # Changesets

    def test_changeset_repos_creation(self):
        chgset = self.repos.get_changeset(0)
        self.assertEqual(0, chgset.rev)
        self.assertEqual(None, chgset.message)
        self.assertEqual(None, chgset.author)
        self.assertEqual(1112349461, chgset.date)
        self.assertRaises(StopIteration, chgset.get_changes().next)

    def test_changeset_added_dirs(self):
        chgset = self.repos.get_changeset(1)
        self.assertEqual(1, chgset.rev)
        self.assertEqual('Initial directory layout.', chgset.message)
        self.assertEqual('john', chgset.author)
        self.assertEqual(1112349652, chgset.date)

        changes = chgset.get_changes()
        self.assertEqual(('trunk', Node.DIRECTORY, Changeset.ADD, None, -1),
                         changes.next())
        self.assertEqual(('branches', Node.DIRECTORY, Changeset.ADD, None, -1),
                         changes.next())
        self.assertEqual(('tags', Node.DIRECTORY, Changeset.ADD, None, -1),
                         changes.next())
        self.assertRaises(StopIteration, changes.next)

    def test_changeset_file_edit(self):
        chgset = self.repos.get_changeset(3)
        self.assertEqual(3, chgset.rev)
        self.assertEqual('Fixed README.\n', chgset.message)
        self.assertEqual('kate', chgset.author)
        self.assertEqual(1112361898, chgset.date)

        changes = chgset.get_changes()
        self.assertEqual(('trunk/README.txt', Node.FILE, Changeset.EDIT,
                          'trunk/README.txt', 2), changes.next())
        self.assertRaises(StopIteration, changes.next)

    def test_changeset_dir_moves(self):
        chgset = self.repos.get_changeset(5)
        self.assertEqual(5, chgset.rev)
        self.assertEqual('Moved directories.', chgset.message)
        self.assertEqual('kate', chgset.author)
        self.assertEqual(1112372739, chgset.date)

        changes = chgset.get_changes()
        self.assertEqual(('trunk/dir1/dir2', Node.DIRECTORY, Changeset.MOVE,
                          'trunk/dir2', 4), changes.next())
        self.assertEqual(('trunk/dir1/dir3', Node.DIRECTORY, Changeset.MOVE,
                          'trunk/dir3', 4), changes.next())
        self.assertRaises(StopIteration, changes.next)

    def test_changeset_file_copy(self):
        chgset = self.repos.get_changeset(6)
        self.assertEqual(6, chgset.rev)
        self.assertEqual('More things to read', chgset.message)
        self.assertEqual('john', chgset.author)
        self.assertEqual(1112381806, chgset.date)

        changes = chgset.get_changes()
        self.assertEqual(('trunk/README2.txt', Node.FILE, Changeset.COPY,
                          'trunk/README.txt', 3), changes.next())
        self.assertRaises(StopIteration, changes.next)


def suite():
    loader = unittest.TestLoader()
    loader.suiteClass = SubversionRepositoryTestSetup
    return loader.loadTestsFromTestCase(SubversionRepositoryTestCase)

if __name__ == '__main__':
    runner = unittest.TextTestRunner()
    runner.run(suite())
