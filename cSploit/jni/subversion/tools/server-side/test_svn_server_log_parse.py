#!/usr/bin/python

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
# ====================================================================

# Run this without arguments to run unit tests.
# Run with a path to a davautocheck ops log to test that it can parse that.

import os
import re
import sys
import tempfile
try:
  # Python >=3.0
  from urllib.parse import quote as urllib_parse_quote
except ImportError:
  # Python <3.0
  from urllib import quote as urllib_parse_quote
import unittest

import svn.core

import svn_server_log_parse

class TestCase(unittest.TestCase):
    def setUp(self):
        # Define a class to stuff everything passed to any handle_
        # method into self.result.
        class cls(svn_server_log_parse.Parser):
            def __getattr__(cls_self, attr):
                if attr.startswith('handle_'):
                    return lambda *a: setattr(self, 'result', a)
                raise AttributeError
        self.parse = cls().parse

    def test_unknown(self):
        line = 'unknown log line'
        self.parse(line)
        self.assertEqual(self.result, (line,))

    def test_open(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'open')
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'open 2 cap / SVN/1.60. fooclient')
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'open a cap=() / SVN/1.60. fooclient')
        self.assertEqual(self.parse('open 2 cap=() / SVN fooclient'), '')
        self.assertEqual(self.result, (2, [], '/', 'SVN', 'fooclient'))
        # TODO: Teach it about the capabilities, rather than allowing
        # any words at all.
        self.assertEqual(self.parse('open 2 cap=(foo) / SVN foo%20client'), '')
        self.assertEqual(self.result, (2, ['foo'], '/', 'SVN', 'foo client'))

    def test_reparent(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'reparent')
        self.assertEqual(self.parse('reparent /'), '')
        self.assertEqual(self.result, ('/',))

    def test_get_latest_rev(self):
        self.assertEqual(self.parse('get-latest-rev'), '')
        self.assertEqual(self.result, ())
        self.assertEqual(self.parse('get-latest-rev r3'), 'r3')
        self.assertEqual(self.result, ())

    def test_get_dated_rev(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse,
                          'get-dated-rev')
        self.assertEqual(self.parse('get-dated-rev 2008-04-15T20:41:24.000000Z'), '')
        self.assertEqual(self.result, ('2008-04-15T20:41:24.000000Z',))

    def test_commit(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'commit')
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'commit 3')
        self.assertEqual(self.parse('commit r3'), '')
        self.assertEqual(self.result, (3,))
        self.assertEqual(self.parse('commit r3 leftover'), ' leftover')
        self.assertEqual(self.result, (3,))

    def test_get_dir(self):
        self.get_dir_or_file('get-dir')

    def test_get_file(self):
        self.get_dir_or_file('get-file')

    def get_dir_or_file(self, c):
        self.assertRaises(svn_server_log_parse.Error, self.parse, c)
        self.assertRaises(svn_server_log_parse.Error, self.parse, c + ' foo')
        self.assertRaises(svn_server_log_parse.Error, self.parse, c + ' foo 3')
        self.assertEqual(self.parse(c + ' /a/b/c r3 ...'), ' ...')
        self.assertEqual(self.result, ('/a/b/c', 3, False, False))
        self.assertEqual(self.parse(c + ' / r3'), '')
        self.assertEqual(self.result, ('/', 3, False, False))
        # path must be absolute
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, c + ' a/b/c r3')
        self.assertEqual(self.parse(c + ' /k r27 text'), '')
        self.assertEqual(self.result, ('/k', 27, True, False))
        self.assertEqual(self.parse(c + ' /k r27 props'), '')
        self.assertEqual(self.result, ('/k', 27, False, True))
        self.assertEqual(self.parse(c + ' /k r27 text props'), '')
        self.assertEqual(self.result, ('/k', 27, True, True))
        # out of order not accepted
        self.assertEqual(self.parse(c + ' /k r27 props text'), ' text')
        self.assertEqual(self.result, ('/k', 27, False, True))

    def test_lock(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'lock')
        self.parse('lock (/foo)')
        self.assertEqual(self.result, (['/foo'], False))
        self.assertEqual(self.parse('lock (/foo) steal ...'), ' ...')
        self.assertEqual(self.result, (['/foo'], True))
        self.assertEqual(self.parse('lock (/foo) stear'), ' stear')

    def test_change_rev_prop(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'change-rev-prop r3')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'change-rev-prop r svn:log')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'change-rev-prop rX svn:log')
        self.assertEqual(self.parse('change-rev-prop r3 svn:log ...'), ' ...')
        self.assertEqual(self.result, (3, 'svn:log'))

    def test_rev_proplist(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'rev-proplist')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'rev-proplist r')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'rev-proplist rX')
        self.assertEqual(self.parse('rev-proplist r3 ...'), ' ...')
        self.assertEqual(self.result, (3,))

    def test_rev_prop(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'rev-prop')
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'rev-prop r')
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'rev-prop rX')
        self.assertEqual(self.parse('rev-prop r3 foo ...'), ' ...')
        self.assertEqual(self.result, (3, 'foo'))

    def test_unlock(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'unlock')
        self.parse('unlock (/foo)')
        self.assertEqual(self.result, (['/foo'], False))
        self.assertEqual(self.parse('unlock (/foo) break ...'), ' ...')
        self.assertEqual(self.result, (['/foo'], True))
        self.assertEqual(self.parse('unlock (/foo) bear'), ' bear')

    def test_get_lock(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'get-lock')
        self.parse('get-lock /foo')
        self.assertEqual(self.result, ('/foo',))

    def test_get_locks(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'get-locks')
        self.parse('get-locks /foo')
        self.assertEqual(self.result, ('/foo',))

    def test_get_locations(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse,
                          'get-locations')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-locations /foo 3')
        self.assertEqual(self.parse('get-locations /foo (3 4) ...'), ' ...')
        self.assertEqual(self.result, ('/foo', [3, 4]))
        self.assertEqual(self.parse('get-locations /foo (3)'), '')
        self.assertEqual(self.result, ('/foo', [3]))

    def test_get_location_segments(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse,
                          'get-location-segments')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-location-segments /foo 3')
        self.assertEqual(self.parse('get-location-segments /foo@2 r3:4'), '')
        self.assertEqual(self.result, ('/foo', 2, 3, 4))

    def test_get_file_revs(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'get-file-revs')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-file-revs /foo 3')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-file-revs /foo 3:a')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-file-revs /foo r3:a')
        self.assertEqual(self.parse('get-file-revs /foo r3:4 ...'), ' ...')
        self.assertEqual(self.result, ('/foo', 3, 4, False))
        self.assertEqual(self.parse('get-file-revs /foo r3:4'
                                    ' include-merged-revisions ...'), ' ...')
        self.assertEqual(self.result, ('/foo', 3, 4, True))

    def test_get_mergeinfo(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-mergeinfo')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-mergeinfo /foo')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-mergeinfo (/foo')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-mergeinfo (/foo /bar')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'get-mergeinfo (/foo)')
        self.assertRaises(svn_server_log_parse.BadMergeinfoInheritanceError,
                          self.parse, 'get-mergeinfo (/foo) bork')
        self.assertEqual(self.parse('get-mergeinfo (/foo) explicit'), '')
        self.assertEqual(self.result, (['/foo'],
                                       svn.core.svn_mergeinfo_explicit, False))
        self.assertEqual(self.parse('get-mergeinfo (/foo /bar) inherited ...'),
                         ' ...')
        self.assertEqual(self.result, (['/foo', '/bar'],
                                       svn.core.svn_mergeinfo_inherited, False))
        self.assertEqual(self.result, (['/foo', '/bar'],
                                       svn.core.svn_mergeinfo_inherited, False))

    def test_log(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'log')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'log /foo')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'log (/foo)')
        self.assertEqual(self.parse('log (/foo) r3:4'
                                    ' include-merged-revisions'), '')
        self.assertEqual(self.result,
                         (['/foo'], 3, 4, 0, False, False, True, []))
        self.assertEqual(self.parse('log (/foo /bar) r3:4 revprops=all ...'),
                         ' ...')
        self.assertEqual(self.result,
                         (['/foo', '/bar'], 3, 4, 0, False, False, False, None))
        self.assertEqual(self.parse('log (/foo) r3:4 revprops=(a b) ...'),
                         ' ...')
        self.assertEqual(self.result,
                         (['/foo'], 3, 4, 0, False, False, False, ['a', 'b']))
        self.assertEqual(self.parse('log (/foo) r8:1 limit=3'), '')
        self.assertEqual(self.result,
                         (['/foo'], 8, 1, 3, False, False, False, []))

    def test_check_path(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'check-path')
        self.assertEqual(self.parse('check-path /foo@9'), '')
        self.assertEqual(self.result, ('/foo', 9))

    def test_stat(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'stat')
        self.assertEqual(self.parse('stat /foo@9'), '')
        self.assertEqual(self.result, ('/foo', 9))

    def test_replay(self):
        self.assertRaises(svn_server_log_parse.Error, self.parse, 'replay')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'replay /foo')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'replay (/foo) r9')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'replay (/foo) r9:10')
        self.assertEqual(self.parse('replay /foo r9'), '')
        self.assertEqual(self.result, ('/foo', 9))

    def test_checkout_or_export(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'checkout-or-export')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'checkout-or-export /foo')
        self.assertEqual(self.parse('checkout-or-export /foo r9'), '')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_unknown))
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'checkout-or-export /foo r9 depth=INVALID-DEPTH')
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'checkout-or-export /foo r9 depth=bork')
        self.assertEqual(self.parse('checkout-or-export /foo r9 depth=files .'),
                         ' .')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_files))

    def test_diff_1path(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'diff')
        self.assertEqual(self.parse('diff /foo r9:10'), '')
        self.assertEqual(self.result, ('/foo', 9, 10,
                                       svn.core.svn_depth_unknown, False))
        self.assertEqual(self.parse('diff /foo r9:10'
                                    ' ignore-ancestry ...'), ' ...')
        self.assertEqual(self.result, ('/foo', 9, 10,
                                       svn.core.svn_depth_unknown, True))
        self.assertEqual(self.parse('diff /foo r9:10 depth=files'), '')
        self.assertEqual(self.result, ('/foo', 9, 10,
                                       svn.core.svn_depth_files, False))

    def test_diff_2paths(self):
        self.assertEqual(self.parse('diff /foo@9 /bar@10'), '')
        self.assertEqual(self.result, ('/foo', 9, '/bar', 10,
                                       svn.core.svn_depth_unknown, False))
        self.assertEqual(self.parse('diff /foo@9 /bar@10'
                                    ' ignore-ancestry ...'), ' ...')
        self.assertEqual(self.result, ('/foo', 9, '/bar', 10,
                                       svn.core.svn_depth_unknown, True))
        self.assertEqual(self.parse('diff /foo@9 /bar@10'
                                    ' depth=files ignore-ancestry'), '')
        self.assertEqual(self.result, ('/foo', 9, '/bar', 10,
                                       svn.core.svn_depth_files, True))

    def test_status(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'status')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'status /foo')
        self.assertEqual(self.parse('status /foo r9'), '')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_unknown))
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'status /foo r9 depth=INVALID-DEPTH')
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'status /foo r9 depth=bork')
        self.assertEqual(self.parse('status /foo r9 depth=files .'),
                         ' .')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_files))

    def test_switch(self):
        self.assertEqual(self.parse('switch /foo /bar@10 ...'), ' ...')
        self.assertEqual(self.result, ('/foo', '/bar', 10,
                                       svn.core.svn_depth_unknown))
        self.assertEqual(self.parse('switch /foo /bar@10'
                                    ' depth=files'), '')
        self.assertEqual(self.result, ('/foo', '/bar', 10,
                                       svn.core.svn_depth_files))

    def test_update(self):
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'update')
        self.assertRaises(svn_server_log_parse.Error,
                          self.parse, 'update /foo')
        self.assertEqual(self.parse('update /foo r9'), '')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_unknown,
                                       False))
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'update /foo r9 depth=INVALID-DEPTH')
        self.assertRaises(svn_server_log_parse.BadDepthError, self.parse,
                          'update /foo r9 depth=bork')
        self.assertEqual(self.parse('update /foo r9 depth=files .'), ' .')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_files,
                                       False))
        self.assertEqual(self.parse('update /foo r9 send-copyfrom-args .'),
                         ' .')
        self.assertEqual(self.result, ('/foo', 9, svn.core.svn_depth_unknown,
                                       True))

if __name__ == '__main__':
    if len(sys.argv) == 1:
        # No arguments so run the unit tests.
        unittest.main()
        sys.stderr.write('unittest.main failed to exit\n')
        sys.exit(2)

    # Use the argument as the path to a log file to test against.

    def uri_encode(s):
        # urllib.parse.quote encodes :&@ characters, svn does not.
        return urllib_parse_quote(s, safe='/:&@')

    # Define a class to reconstruct the SVN-ACTION string.
    class Test(svn_server_log_parse.Parser):
        def handle_unknown(self, line):
            sys.stderr.write('unknown log line at %d:\n%s\n' % (self.linenum,
                                                                line))
            sys.exit(2)

        def handle_open(self, protocol, capabilities, path, ra_client, client):
            capabilities = ' '.join(capabilities)
            if ra_client is None:
                ra_client = '-'
            if client is None:
                client = '-'
            path = uri_encode(path)
            self.action = ('open %d cap=(%s) %s %s %s'
                           % (protocol, capabilities, path, ra_client, client))

        def handle_reparent(self, path):
            path = uri_encode(path)
            self.action = 'reparent ' + path

        def handle_get_latest_rev(self):
            self.action = 'get-latest-rev'

        def handle_get_dated_rev(self, date):
            self.action = 'get-dated-rev ' + date

        def handle_commit(self, revision):
            self.action = 'commit r%d' % (revision,)

        def handle_get_dir(self, path, revision, text, props):
            path = uri_encode(path)
            self.action = 'get-dir %s r%d' % (path, revision)
            if text:
                self.action += ' text'
            if props:
                self.action += ' props'

        def handle_get_file(self, path, revision, text, props):
            path = uri_encode(path)
            self.action = 'get-file %s r%d' % (path, revision)
            if text:
                self.action += ' text'
            if props:
                self.action += ' props'

        def handle_lock(self, paths, steal):
            paths = [uri_encode(x) for x in paths]
            self.action = 'lock (%s)' % (' '.join(paths),)
            if steal:
                self.action += ' steal'

        def handle_change_rev_prop(self, revision, revprop):
            revprop = uri_encode(revprop)
            self.action = 'change-rev-prop r%d %s' % (revision, revprop)

        def handle_rev_prop(self, revision, revprop):
            revprop = uri_encode(revprop)
            self.action = 'rev-prop r%d %s' % (revision, revprop)

        def handle_rev_proplist(self, revision):
            self.action = 'rev-proplist r%d' % (revision,)

        def handle_unlock(self, paths, break_lock):
            paths = [uri_encode(x) for x in paths]
            self.action = 'unlock (%s)' % (' '.join(paths),)
            if break_lock:
                self.action += ' break'

        def handle_get_lock(self, path):
            path = uri_encode(path)
            self.action = 'get-lock ' + path

        def handle_get_locks(self, path):
            self.action = 'get-locks ' + path
            path = uri_encode(path)

        def handle_get_locations(self, path, revisions):
            path = uri_encode(path)
            self.action = ('get-locations %s (%s)'
                           % (path, ' '.join([str(x) for x in revisions])))

        def handle_get_location_segments(self, path, peg, left, right):
            path = uri_encode(path)
            self.action = 'get-location-segments %s@%d r%d:%d' % (path, peg,
                                                                  left, right)

        def handle_get_file_revs(self, path, left, right,
                                 include_merged_revisions):
            path = uri_encode(path)
            self.action = 'get-file-revs %s r%d:%d' % (path, left, right)
            if include_merged_revisions:
                self.action += ' include-merged-revisions'

        def handle_get_mergeinfo(self, paths, inheritance, include_descendants):
            paths = [uri_encode(x) for x in paths]
            self.action = ('get-mergeinfo (%s) %s'
                           % (' '.join(paths),
                              svn.core.svn_inheritance_to_word(inheritance)))
            if include_descendants:
                self.action += ' include-descendants'

        def handle_log(self, paths, left, right, limit, discover_changed_paths,
                       strict, include_merged_revisions, revprops):
            paths = [uri_encode(x) for x in paths]
            self.action = 'log (%s) r%d:%d' % (' '.join(paths),
                                               left, right)
            if limit != 0:
                self.action += ' limit=%d' % (limit,)
            if discover_changed_paths:
                self.action += ' discover-changed-paths'
            if strict:
                self.action += ' strict'
            if include_merged_revisions:
                self.action += ' include-merged-revisions'
            if revprops is None:
                self.action += ' revprops=all'
            elif len(revprops) > 0:
                revprops = [uri_encode(x) for x in revprops]
                self.action += ' revprops=(%s)' % (' '.join(revprops),)

        def handle_check_path(self, path, revision):
            path = uri_encode(path)
            self.action = 'check-path %s@%d' % (path, revision)

        def handle_stat(self, path, revision):
            path = uri_encode(path)
            self.action = 'stat %s@%d' % (path, revision)

        def handle_replay(self, path, revision):
            path = uri_encode(path)
            self.action = 'replay %s r%d' % (path, revision)

        def maybe_depth(self, depth):
            if depth != svn.core.svn_depth_unknown:
                self.action += ' depth=%s' % (
                    svn.core.svn_depth_to_word(depth),)

        def handle_checkout_or_export(self, path, revision, depth):
            path = uri_encode(path)
            self.action = 'checkout-or-export %s r%d' % (path, revision)
            self.maybe_depth(depth)

        def handle_diff_1path(self, path, left, right,
                                       depth, ignore_ancestry):
            path = uri_encode(path)
            self.action = 'diff %s r%d:%d' % (path, left, right)
            self.maybe_depth(depth)
            if ignore_ancestry:
                self.action += ' ignore-ancestry'

        def handle_diff_2paths(self, from_path, from_rev,
                                        to_path, to_rev,
                                        depth, ignore_ancestry):
            from_path = uri_encode(from_path)
            to_path = uri_encode(to_path)
            self.action = ('diff %s@%d %s@%d'
                           % (from_path, from_rev, to_path, to_rev))
            self.maybe_depth(depth)
            if ignore_ancestry:
                self.action += ' ignore-ancestry'

        def handle_status(self, path, revision, depth):
            path = uri_encode(path)
            self.action = 'status %s r%d' % (path, revision)
            self.maybe_depth(depth)

        def handle_switch(self, from_path, to_path, to_rev, depth):
            from_path = uri_encode(from_path)
            to_path = uri_encode(to_path)
            self.action = ('switch %s %s@%d'
                           % (from_path, to_path, to_rev))
            self.maybe_depth(depth)

        def handle_update(self, path, revision, depth, send_copyfrom_args):
            path = uri_encode(path)
            self.action = 'update %s r%d' % (path, revision)
            self.maybe_depth(depth)
            if send_copyfrom_args:
                self.action += ' send-copyfrom-args'

    tmp = tempfile.mktemp()
    try:
        fp = open(tmp, 'w')
        parser = Test()
        parser.linenum = 0
        log_file = sys.argv[1]
        log_type = None
        for line in open(log_file):
            if log_type is None:
                # Figure out which log type we have.
                if re.match(r'\d+ \d\d\d\d-', line):
                    log_type = 'svnserve'
                elif re.match(r'\[\d\d/', line):
                    log_type = 'mod_dav_svn'
                else:
                    sys.stderr.write("unknown log format in '%s'"
                                     % (log_file,))
                    sys.exit(3)
                sys.stderr.write('parsing %s log...\n' % (log_type,))
                sys.stderr.flush()

            words = line.split()
            if log_type == 'svnserve':
                # Skip over PID, date, client address, username, and repos.
                if words[5].startswith('ERR'):
                    # Skip error lines.
                    fp.write(line)
                    continue
                leading = ' '.join(words[:5])
                action = ' '.join(words[5:])
            else:
                # Find the SVN-ACTION string from the CustomLog format
                # davautocheck.sh uses.  If that changes, this will need
                # to as well.  Currently it's
                #   %t %u %{SVN-REPOS-NAME}e %{SVN-ACTION}e
                leading = ' '.join(words[:4])
                action = ' '.join(words[4:])

            # Parse the action and write the reconstructed action to
            # the temporary file.  Ignore the returned trailing text,
            # as we have none in the davautocheck ops log.
            parser.linenum += 1
            try:
                parser.parse(action)
            except svn_server_log_parse.Error:
                sys.stderr.write('error at line %d: %s\n'
                                 % (parser.linenum, action))
                raise
            fp.write(leading + ' ' + parser.action + '\n')
        fp.close()
        # Check differences between original and reconstructed files
        # (should be identical).
        result = os.spawnlp(os.P_WAIT, 'diff', 'diff', '-u', log_file, tmp)
        if result == 0:
            sys.stderr.write('OK\n')
        sys.exit(result)
    finally:
        try:
            os.unlink(tmp)
        except Exception, e:
            sys.stderr.write('os.unlink(tmp): %s\n' % (e,))
