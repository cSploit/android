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

# TODO: Teach parse_open about capabilities, rather than allowing any
# words at all.

"""Parse subversion server operational logs.

SVN-ACTION strings
------------------

Angle brackets denote a variable, e.g. 'commit r<N>' means you'll see
lines like 'commit r17' for this action.

<N> and <M> are revision numbers.

<PATH>, <FROM-PATH>, and <TO-PATH> mean a URI-encoded path relative to
the repository root, including a leading '/'.

<REVPROP> means a revision property, e.g. 'svn:log'.

<I> represents a svn_mergeinfo_inheritance_t value and is one of these
words: explicit inherited nearest-ancestor.

<D> represents a svn_depth_t value and is one of these words: empty
files immediates infinity.  If the depth value for the operation was
svn_depth_unknown, the depth= portion is absent entirely.

The get-mergeinfo and log actions use lists for paths and revprops.
The lists are enclosed in parentheses and each item is separated by a
space (spaces in paths are encoded as %20).

The words will *always* be in this order, though some may be absent.

General::

    change-rev-prop r<N> <REVPROP>
    commit r<N>
    get-dir <PATH> r<N> text? props?
    get-file <PATH> r<N> text? props?
    lock (<PATH> ...) steal?
    rev-proplist r<N>
    unlock (<PATH> ...) break?

Reports::

    get-file-revs <PATH> r<N>:<M> include-merged-revisions?
    get-mergeinfo (<PATH> ...) <I> include-descendants?
    log (<PATH> ...) r<N>:<M> limit=<N>? discover-changed-paths? strict? include-merged-revisions? revprops=all|(<REVPROP> ...)?
    replay <PATH> r<N>

The update report::

    checkout-or-export <PATH> r<N> depth=<D>?
    diff <FROM-PATH>@<N> <TO-PATH>@<M> depth=<D>? ignore-ancestry?
    diff <PATH> r<N>:<M> depth=<D>? ignore-ancestry?
    status <PATH> r<N> depth=<D>?
    switch <FROM-PATH> <TO-PATH>@<N> depth=<D>?
    update <PATH> r<N> depth=<D>? send-copyfrom-args?
"""


import re
try:
  # Python >=3.0
  from urllib.parse import unquote as urllib_parse_unquote
except ImportError:
  # Python <3.0
  from urllib import unquote as urllib_parse_unquote

import svn.core

#
# Valid words for _parse_depth and _parse_mergeinfo_inheritance
#

DEPTH_WORDS = ['empty', 'files', 'immediates', 'infinity']
INHERITANCE_WORDS = {
    'explicit': svn.core.svn_mergeinfo_explicit,
    'inherited': svn.core.svn_mergeinfo_inherited,
    'nearest-ancestor': svn.core.svn_mergeinfo_nearest_ancestor,
}

#
# Patterns for _match
#

# <PATH>
pPATH = r'(/\S*)'
# (<PATH> ...)
pPATHS = r'\(([^)]*)\)'
# r<N>
pREVNUM = r'r(\d+)'
# (<N> ...)
pREVNUMS = r'\(((\d+\s*)*)\)'
# r<N>:<M>
pREVRANGE = r'r(-?\d+):(-?\d+)'
# <PATH>@<N>
pPATHREV = pPATH + r'@(\d+)'
pWORD = r'(\S+)'
pPROPERTY = pWORD
# depth=<D>?
pDEPTH = 'depth=' + pWORD

#
# Exceptions
#

class Error(Exception): pass
class BadDepthError(Error):
    def __init__(self, value):
        Error.__init__(self, 'bad svn_depth_t value ' + value)
class BadMergeinfoInheritanceError(Error):
    def __init__(self, value):
        Error.__init__(self, 'bad svn_mergeinfo_inheritance_t value ' + value)
class MatchError(Error):
    def __init__(self, pattern, line):
        Error.__init__(self, '/%s/ does not match log line:\n%s'
                       % (pattern, line))


#
# Helper functions
#

# TODO: Move to kitchensink.c like svn_depth_from_word?
try:
    from svn.core import svn_inheritance_from_word
except ImportError:
    def svn_inheritance_from_word(word):
        try:
            return INHERITANCE_WORDS[word]
        except KeyError:
            # XXX svn_inheritance_to_word uses explicit as default so...
            return svn.core.svn_mergeinfo_explicit

def _parse_depth(word):
    if word is None:
        return svn.core.svn_depth_unknown
    if word not in DEPTH_WORDS:
        raise BadDepthError(word)
    return svn.core.svn_depth_from_word(word)

def _parse_mergeinfo_inheritance(word):
    if word not in INHERITANCE_WORDS:
        raise BadMergeinfoInheritanceError(word)
    return svn_inheritance_from_word(word)

def _match(line, *patterns):
    """Return a re.match object from matching patterns against line.

    All optional arguments must be strings suitable for ''.join()ing
    into a single pattern string for re.match.  The last optional
    argument may instead be a list of such strings, which will be
    joined into the final pattern as *optional* matches.

    Raises:
    Error -- if re.match returns None (i.e. no match)
    """
    if isinstance(patterns[-1], list):
        optional = patterns[-1]
        patterns = patterns[:-1]
    else:
        optional = []
    pattern = r'\s+'.join(patterns)
    pattern += ''.join([r'(\s+' + x + ')?' for x in optional])
    m = re.match(pattern, line)
    if m is None:
        raise MatchError(pattern, line)
    return m


class Parser(object):
    """Subclass this and define the handle_ methods according to the
    "SVN-ACTION strings" section of this module's documentation.  For
    example, "lock <PATH> steal?" => def handle_lock(self, path, steal)
    where steal will be True if "steal" was present.

    See the end of test_svn_server_log_parse.py for a complete example.
    """
    def parse(self, line):
        """Parse line and call appropriate handle_ method.

        Returns one of:
        - line remaining after the svn action, if one was parsed
        - whatever your handle_unknown implementation returns

        Raises:
        BadDepthError                   -- for bad svn_depth_t values
        BadMergeinfoInheritanceError    -- for bad svn_mergeinfo_inheritance_t
                                           values
        Error                           -- any other parse error
        """
        self.line = line
        words = self.split_line = line.split(' ')
        try:
            method = getattr(self, '_parse_' + words[0].replace('-', '_'))
        except AttributeError:
            return self.handle_unknown(self.line)
        return method(' '.join(words[1:]))

    def _parse_commit(self, line):
        m = _match(line, pREVNUM)
        self.handle_commit(int(m.group(1)))
        return line[m.end():]

    def _parse_open(self, line):
        pINT = r'(\d+)'
        pCAP = r'cap=\(([^)]*)\)'
        pCLIENT = pWORD
        m = _match(line, pINT, pCAP, pPATH, pCLIENT, pCLIENT)
        protocol = int(m.group(1))
        if m.group(2) is None:
            capabilities = []
        else:
            capabilities = m.group(2).split()
        path = m.group(3)
        ra_client = urllib_parse_unquote(m.group(4))
        client = urllib_parse_unquote(m.group(5))
        self.handle_open(protocol, capabilities, path, ra_client, client)
        return line[m.end():]

    def _parse_reparent(self, line):
        m = _match(line, pPATH)
        self.handle_reparent(urllib_parse_unquote(m.group(1)))
        return line[m.end():]

    def _parse_get_latest_rev(self, line):
        self.handle_get_latest_rev()
        return line

    def _parse_get_dated_rev(self, line):
        m = _match(line, pWORD)
        self.handle_get_dated_rev(m.group(1))
        return line[m.end():]

    def _parse_get_dir(self, line):
        m = _match(line, pPATH, pREVNUM, ['text', 'props'])
        self.handle_get_dir(urllib_parse_unquote(m.group(1)), int(m.group(2)),
                            m.group(3) is not None,
                            m.group(4) is not None)
        return line[m.end():]

    def _parse_get_file(self, line):
        m = _match(line, pPATH, pREVNUM, ['text', 'props'])
        self.handle_get_file(urllib_parse_unquote(m.group(1)), int(m.group(2)),
                             m.group(3) is not None,
                             m.group(4) is not None)
        return line[m.end():]

    def _parse_lock(self, line):
        m = _match(line, pPATHS, ['steal'])
        paths = [urllib_parse_unquote(x) for x in m.group(1).split()]
        self.handle_lock(paths, m.group(2) is not None)
        return line[m.end():]

    def _parse_change_rev_prop(self, line):
        m = _match(line, pREVNUM, pPROPERTY)
        self.handle_change_rev_prop(int(m.group(1)),
                                    urllib_parse_unquote(m.group(2)))
        return line[m.end():]

    def _parse_rev_proplist(self, line):
        m = _match(line, pREVNUM)
        self.handle_rev_proplist(int(m.group(1)))
        return line[m.end():]

    def _parse_rev_prop(self, line):
        m = _match(line, pREVNUM, pPROPERTY)
        self.handle_rev_prop(int(m.group(1)), urllib_parse_unquote(m.group(2)))
        return line[m.end():]

    def _parse_unlock(self, line):
        m = _match(line, pPATHS, ['break'])
        paths = [urllib_parse_unquote(x) for x in m.group(1).split()]
        self.handle_unlock(paths, m.group(2) is not None)
        return line[m.end():]

    def _parse_get_lock(self, line):
        m = _match(line, pPATH)
        self.handle_get_lock(urllib_parse_unquote(m.group(1)))
        return line[m.end():]

    def _parse_get_locks(self, line):
        m = _match(line, pPATH)
        self.handle_get_locks(urllib_parse_unquote(m.group(1)))
        return line[m.end():]

    def _parse_get_locations(self, line):
        m = _match(line, pPATH, pREVNUMS)
        path = urllib_parse_unquote(m.group(1))
        revnums = [int(x) for x in m.group(2).split()]
        self.handle_get_locations(path, revnums)
        return line[m.end():]

    def _parse_get_location_segments(self, line):
        m = _match(line, pPATHREV, pREVRANGE)
        path = urllib_parse_unquote(m.group(1))
        peg = int(m.group(2))
        left = int(m.group(3))
        right = int(m.group(4))
        self.handle_get_location_segments(path, peg, left, right)
        return line[m.end():]

    def _parse_get_file_revs(self, line):
        m = _match(line, pPATH, pREVRANGE, ['include-merged-revisions'])
        path = urllib_parse_unquote(m.group(1))
        left = int(m.group(2))
        right = int(m.group(3))
        include_merged_revisions    = m.group(4) is not None
        self.handle_get_file_revs(path, left, right, include_merged_revisions)
        return line[m.end():]

    def _parse_get_mergeinfo(self, line):
        # <I>
        pMERGEINFO_INHERITANCE = pWORD
        pINCLUDE_DESCENDANTS = pWORD
        m = _match(line,
                   pPATHS, pMERGEINFO_INHERITANCE, ['include-descendants'])
        paths = [urllib_parse_unquote(x) for x in m.group(1).split()]
        inheritance = _parse_mergeinfo_inheritance(m.group(2))
        include_descendants = m.group(3) is not None
        self.handle_get_mergeinfo(paths, inheritance, include_descendants)
        return line[m.end():]

    def _parse_log(self, line):
        # limit=<N>?
        pLIMIT = r'limit=(\d+)'
        # revprops=all|(<REVPROP> ...)?
        pREVPROPS = r'revprops=(all|\(([^)]+)\))'
        m = _match(line, pPATHS, pREVRANGE,
                   [pLIMIT, 'discover-changed-paths', 'strict',
                    'include-merged-revisions', pREVPROPS])
        paths = [urllib_parse_unquote(x) for x in m.group(1).split()]
        left = int(m.group(2))
        right = int(m.group(3))
        if m.group(5) is None:
            limit = 0
        else:
            limit = int(m.group(5))
        discover_changed_paths      = m.group(6) is not None
        strict                      = m.group(7) is not None
        include_merged_revisions    = m.group(8) is not None
        if m.group(10) == 'all':
            revprops = None
        else:
            if m.group(11) is None:
                revprops = []
            else:
                revprops = [urllib_parse_unquote(x) for x in m.group(11).split()]
        self.handle_log(paths, left, right, limit, discover_changed_paths,
                        strict, include_merged_revisions, revprops)
        return line[m.end():]

    def _parse_check_path(self, line):
        m = _match(line, pPATHREV)
        path = urllib_parse_unquote(m.group(1))
        revnum = int(m.group(2))
        self.handle_check_path(path, revnum)
        return line[m.end():]

    def _parse_stat(self, line):
        m = _match(line, pPATHREV)
        path = urllib_parse_unquote(m.group(1))
        revnum = int(m.group(2))
        self.handle_stat(path, revnum)
        return line[m.end():]

    def _parse_replay(self, line):
        m = _match(line, pPATH, pREVNUM)
        path = urllib_parse_unquote(m.group(1))
        revision = int(m.group(2))
        self.handle_replay(path, revision)
        return line[m.end():]

    # the update report

    def _parse_checkout_or_export(self, line):
        m = _match(line, pPATH, pREVNUM, [pDEPTH])
        path = urllib_parse_unquote(m.group(1))
        revision = int(m.group(2))
        depth = _parse_depth(m.group(4))
        self.handle_checkout_or_export(path, revision, depth)
        return line[m.end():]

    def _parse_diff(self, line):
        # First, try 1-path form.
        try:
            m = _match(line, pPATH, pREVRANGE, [pDEPTH, 'ignore-ancestry'])
            f = self._parse_diff_1path
        except Error:
            # OK, how about 2-path form?
            m = _match(line, pPATHREV, pPATHREV, [pDEPTH, 'ignore-ancestry'])
            f = self._parse_diff_2paths
        return f(line, m)

    def _parse_diff_1path(self, line, m):
        path = urllib_parse_unquote(m.group(1))
        left = int(m.group(2))
        right = int(m.group(3))
        depth = _parse_depth(m.group(5))
        ignore_ancestry = m.group(6) is not None
        self.handle_diff_1path(path, left, right,
                               depth, ignore_ancestry)
        return line[m.end():]

    def _parse_diff_2paths(self, line, m):
        from_path = urllib_parse_unquote(m.group(1))
        from_rev = int(m.group(2))
        to_path = urllib_parse_unquote(m.group(3))
        to_rev = int(m.group(4))
        depth = _parse_depth(m.group(6))
        ignore_ancestry = m.group(7) is not None
        self.handle_diff_2paths(from_path, from_rev, to_path, to_rev,
                                depth, ignore_ancestry)
        return line[m.end():]

    def _parse_status(self, line):
        m = _match(line, pPATH, pREVNUM, [pDEPTH])
        path = urllib_parse_unquote(m.group(1))
        revision = int(m.group(2))
        depth = _parse_depth(m.group(4))
        self.handle_status(path, revision, depth)
        return line[m.end():]

    def _parse_switch(self, line):
        m = _match(line, pPATH, pPATHREV, [pDEPTH])
        from_path = urllib_parse_unquote(m.group(1))
        to_path = urllib_parse_unquote(m.group(2))
        to_rev = int(m.group(3))
        depth = _parse_depth(m.group(5))
        self.handle_switch(from_path, to_path, to_rev, depth)
        return line[m.end():]

    def _parse_update(self, line):
        m = _match(line, pPATH, pREVNUM, [pDEPTH, 'send-copyfrom-args'])
        path = urllib_parse_unquote(m.group(1))
        revision = int(m.group(2))
        depth = _parse_depth(m.group(4))
        send_copyfrom_args = m.group(5) is not None
        self.handle_update(path, revision, depth, send_copyfrom_args)
        return line[m.end():]
