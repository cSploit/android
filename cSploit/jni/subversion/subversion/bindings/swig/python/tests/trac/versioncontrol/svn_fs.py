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

from __future__ import generators

from trac.versioncontrol import Changeset, Node, Repository

import os.path
import time
import weakref
import posixpath

from svn import fs, repos, core, delta

_kindmap = {core.svn_node_dir: Node.DIRECTORY,
            core.svn_node_file: Node.FILE}

def _get_history(path, authz, fs_ptr, start, end, limit=None):
    history = []
    if hasattr(repos, 'svn_repos_history2'):
        # For Subversion >= 1.1
        def authz_cb(root, path, pool):
            if limit and len(history) >= limit:
                return 0
            return authz.has_permission(path) and 1 or 0
        def history2_cb(path, rev, pool):
            history.append((path, rev))
        repos.svn_repos_history2(fs_ptr, path, history2_cb, authz_cb,
                                 start, end, 1)
    else:
        # For Subversion 1.0.x
        def history_cb(path, rev, pool):
            if authz.has_permission(path):
                history.append((path, rev))
        repos.svn_repos_history(fs_ptr, path, history_cb, start, end, 1)
    for item in history:
        yield item


class SubversionRepository(Repository):
    """
    Repository implementation based on the svn.fs API.
    """

    def __init__(self, path, authz):
        Repository.__init__(self, authz)

        if core.SVN_VER_MAJOR < 1:
            raise TracError("Subversion >= 1.0 required: Found %d.%d.%d" % \
                  (core.SVN_VER_MAJOR, core.SVN_VER_MINOR, core.SVN_VER_MICRO))

        self.repos = None
        self.fs_ptr = None
        self.path = path

        # Remove any trailing slash or else subversion might abort
        if not os.path.split(path)[1]:
            path = os.path.split(path)[0]
        self.path = repos.svn_repos_find_root_path(path)
        if self.path is None:
            raise TracError("%s does not appear to be a Subversion repository." % (path, ))
        if self.path != path:
            self.scope = path[len(self.path):]
            if not self.scope[-1] == '/':
                self.scope += '/'
        else:
            self.scope = '/'

        self.repos = repos.svn_repos_open(self.path)
        self.fs_ptr = repos.svn_repos_fs(self.repos)
        self.rev = fs.youngest_rev(self.fs_ptr)

        self.history = None
        if self.scope != '/':
            self.history = []
            for path,rev in _get_history(self.scope[1:], self.authz,
                                         self.fs_ptr, 0, self.rev):
                self.history.append(rev)

    def __del__(self):
        self.close()

    def has_node(self, path, rev):
        rev_root = fs.revision_root(self.fs_ptr, rev)
        node_type = fs.check_path(rev_root, path)
        return node_type in _kindmap

    def normalize_path(self, path):
        return path == '/' and path or path and path.strip('/') or ''

    def normalize_rev(self, rev):
        try:
            rev =  int(rev)
        except (ValueError, TypeError):
            rev = None
        if rev is None:
            rev = self.youngest_rev
        elif rev > self.youngest_rev:
            raise TracError("Revision %s doesn't exist yet" % rev)
        return rev

    def close(self):
        if self.repos:
            self.repos = None
            self.fs_ptr = None
            self.rev = None

    def get_changeset(self, rev):
        return SubversionChangeset(int(rev), self.authz, self.scope,
                                   self.fs_ptr)

    def get_node(self, path, rev=None):
        self.authz.assert_permission(self.scope + path)
        if path and path[-1] == '/':
            path = path[:-1]

        rev = self.normalize_rev(rev)

        return SubversionNode(path, rev, self.authz, self.scope, self.fs_ptr)

    def get_oldest_rev(self):
        rev = 0
        if self.scope == '/':
            return rev
        return self.history[-1]

    def get_youngest_rev(self):
        rev = self.rev
        if self.scope == '/':
            return rev
        return self.history[0]

    def previous_rev(self, rev):
        rev = int(rev)
        if rev == 0:
            return None
        if self.scope == '/':
            return rev - 1
        idx = self.history.index(rev)
        if idx + 1 < len(self.history):
            return self.history[idx + 1]
        return None

    def next_rev(self, rev):
        rev = int(rev)
        if rev == self.rev:
            return None
        if self.scope == '/':
            return rev + 1
        if rev == 0:
            return self.oldest_rev
        idx = self.history.index(rev)
        if idx > 0:
            return self.history[idx - 1]
        return None

    def rev_older_than(self, rev1, rev2):
        return self.normalize_rev(rev1) < self.normalize_rev(rev2)

    def get_path_history(self, path, rev=None, limit=None):
        path = self.normalize_path(path)
        rev = self.normalize_rev(rev)
        expect_deletion = False
        while rev:
            if self.has_node(path, rev):
                if expect_deletion:
                    # it was missing, now it's there again: rev+1 must be a delete
                    yield path, rev+1, Changeset.DELETE
                newer = None # 'newer' is the previously seen history tuple
                older = None # 'older' is the currently examined history tuple
                for p, r in _get_history(path, self.authz, self.fs_ptr,
                                         0, rev, limit):
                    older = (self.normalize_path(p), r, Changeset.ADD)
                    rev = self.previous_rev(r)
                    if newer:
                        if older[0] == path: # still on the path: 'newer' was an edit
                            yield newer[0], newer[1], Changeset.EDIT
                        else: # the path changed: 'newer' was a copy
                            rev = self.previous_rev(newer[1]) # restart before the copy op
                            yield newer[0], newer[1], Changeset.COPY
                            older = (older[0], older[1], 'unknown')
                            break
                    newer = older
                if older: # either a real ADD or the source of a COPY
                    yield older
            else:
                expect_deletion = True
                rev = self.previous_rev(rev)

    def get_deltas(self, old_path, old_rev, new_path, new_rev, ignore_ancestry=0):
        old_node = new_node = None
        old_rev = self.normalize_rev(old_rev)
        new_rev = self.normalize_rev(new_rev)
        if self.has_node(old_path, old_rev):
            old_node = self.get_node(old_path, old_rev)
        else:
            raise TracError('The Base for Diff is invalid: path %s'
                              ' doesn\'t exist in revision %s' \
                              % (old_path, old_rev))
        if self.has_node(new_path, new_rev):
            new_node = self.get_node(new_path, new_rev)
        else:
            raise TracError('The Target for Diff is invalid: path %s'
                              ' doesn\'t exist in revision %s' \
                              % (new_path, new_rev))
        if new_node.kind != old_node.kind:
            raise TracError('Diff mismatch: Base is a %s (%s in revision %s) '
                              'and Target is a %s (%s in revision %s).' \
                              % (old_node.kind, old_path, old_rev,
                                 new_node.kind, new_path, new_rev))
        if new_node.isdir:
            editor = DiffChangeEditor()
            e_ptr, e_baton = delta.make_editor(editor)
            old_root = fs.revision_root(self.fs_ptr, old_rev)
            new_root = fs.revision_root(self.fs_ptr, new_rev)
            def authz_cb(root, path, pool): return 1
            text_deltas = 0 # as this is anyway re-done in Diff.py...
            entry_props = 0 # ("... typically used only for working copy updates")
            repos.svn_repos_dir_delta(old_root, old_path, '',
                                      new_root, new_path,
                                      e_ptr, e_baton, authz_cb,
                                      text_deltas,
                                      1, # directory
                                      entry_props,
                                      ignore_ancestry)
            for path, kind, change in editor.deltas:
                old_node = new_node = None
                if change != Changeset.ADD:
                    old_node = self.get_node(posixpath.join(old_path, path), old_rev)
                if change != Changeset.DELETE:
                    new_node = self.get_node(posixpath.join(new_path, path), new_rev)
                else:
                    kind = _kindmap[fs.check_path(old_root, old_node.path)]
                yield  (old_node, new_node, kind, change)
        else:
            old_root = fs.revision_root(self.fs_ptr, old_rev)
            new_root = fs.revision_root(self.fs_ptr, new_rev)
            if fs.contents_changed(old_root, old_path, new_root, new_path):
                yield (old_node, new_node, Node.FILE, Changeset.EDIT)


class SubversionNode(Node):

    def __init__(self, path, rev, authz, scope, fs_ptr):
        self.authz = authz
        self.scope = scope
        if scope != '/':
            self.scoped_path = scope + path
        else:
            self.scoped_path = path
        self.fs_ptr = fs_ptr
        self._requested_rev = rev

        self.root = fs.revision_root(fs_ptr, rev)
        node_type = fs.check_path(self.root, self.scoped_path)
        if not node_type in _kindmap:
            raise TracError("No node at %s in revision %s" % (path, rev))
        self.created_rev = fs.node_created_rev(self.root, self.scoped_path)
        self.created_path = fs.node_created_path(self.root, self.scoped_path)
        # Note: 'created_path' differs from 'path' if the last change was a copy,
        #        and furthermore, 'path' might not exist at 'create_rev'.
        #        The only guarantees are:
        #          * this node exists at (path,rev)
        #          * the node existed at (created_path,created_rev)
        # TODO: check node id
        self.rev = self.created_rev

        Node.__init__(self, path, self.rev, _kindmap[node_type])

    def get_content(self):
        if self.isdir:
            return None
        return core.Stream(fs.file_contents(self.root, self.scoped_path))

    def get_entries(self):
        if self.isfile:
            return
        entries = fs.dir_entries(self.root, self.scoped_path)
        for item in entries.keys():
            path = '/'.join((self.path, item))
            if not self.authz.has_permission(path):
                continue
            yield SubversionNode(path, self._requested_rev, self.authz,
                                 self.scope, self.fs_ptr)

    def get_history(self,limit=None):
        newer = None # 'newer' is the previously seen history tuple
        older = None # 'older' is the currently examined history tuple
        for path, rev in _get_history(self.scoped_path, self.authz, self.fs_ptr,
                                      0, self._requested_rev, limit):
            if rev > 0 and path.startswith(self.scope):
                older = (path[len(self.scope):], rev, Changeset.ADD)
                if newer:
                    change = newer[0] == older[0] and Changeset.EDIT or Changeset.COPY
                    newer = (newer[0], newer[1], change)
                    yield newer
                newer = older
        if newer:
            yield newer

#    def get_previous(self):
#        # FIXME: redo it with fs.node_history

    def get_properties(self):
        props = fs.node_proplist(self.root, self.scoped_path)
        for name,value in props.items():
            props[name] = str(value) # Make sure the value is a proper string
        return props

    def get_content_length(self):
        if self.isdir:
            return None
        return fs.file_length(self.root, self.scoped_path)

    def get_content_type(self):
        if self.isdir:
            return None
        return self._get_prop(core.SVN_PROP_MIME_TYPE)

    def get_last_modified(self):
        date = fs.revision_prop(self.fs_ptr, self.created_rev,
                                core.SVN_PROP_REVISION_DATE)
        return core.svn_time_from_cstring(date) / 1000000

    def _get_prop(self, name):
        return fs.node_prop(self.root, self.scoped_path, name)


class SubversionChangeset(Changeset):

    def __init__(self, rev, authz, scope, fs_ptr):
        self.rev = rev
        self.authz = authz
        self.scope = scope
        self.fs_ptr = fs_ptr
        message = self._get_prop(core.SVN_PROP_REVISION_LOG)
        author = self._get_prop(core.SVN_PROP_REVISION_AUTHOR)
        date = self._get_prop(core.SVN_PROP_REVISION_DATE)
        date = core.svn_time_from_cstring(date) / 1000000
        Changeset.__init__(self, rev, message, author, date)

    def get_changes(self):
        root = fs.revision_root(self.fs_ptr, self.rev)
        editor = repos.RevisionChangeCollector(self.fs_ptr, self.rev)
        e_ptr, e_baton = delta.make_editor(editor)
        repos.svn_repos_replay(root, e_ptr, e_baton)

        idx = 0
        copies, deletions = {}, {}
        changes = []
        for path, change in editor.changes.items():
            if not self.authz.has_permission(path):
                # FIXME: what about base_path?
                continue
            if not path.startswith(self.scope[1:]):
                continue
            base_path = None
            if change.base_path:
                if change.base_path.startswith(self.scope):
                    base_path = change.base_path[len(self.scope):]
                else:
                    base_path = None
            action = ''
            if change.action == repos.CHANGE_ACTION_DELETE:
                action = Changeset.DELETE
                deletions[change.base_path] = idx
            elif change.added:
                if change.base_path and change.base_rev:
                    action = Changeset.COPY
                    copies[change.base_path] = idx
                else:
                    action = Changeset.ADD
            else:
                action = Changeset.EDIT
            kind = _kindmap[change.item_kind]
            path = path[len(self.scope) - 1:]
            changes.append([path, kind, action, base_path, change.base_rev])
            idx += 1

        moves = []
        for k,v in copies.items():
            if k in deletions:
                changes[v][2] = Changeset.MOVE
                moves.append(deletions[k])
        offset = 0
        for i in moves:
            del changes[i - offset]
            offset += 1

        for change in changes:
            yield tuple(change)

    def _get_prop(self, name):
        return fs.revision_prop(self.fs_ptr, self.rev, name)


#
# Delta editor for diffs between arbitrary nodes
#
# Note 1: the 'copyfrom_path' and 'copyfrom_rev' information is not used
#         because 'repos.svn_repos_dir_delta' *doesn't* provide it.
#
# Note 2: the 'dir_baton' is the path of the parent directory
#

class DiffChangeEditor(delta.Editor):

    def __init__(self):
        self.deltas = []

    # -- svn.delta.Editor callbacks

    def open_root(self, base_revision, dir_pool):
        return ('/', Changeset.EDIT)

    def add_directory(self, path, dir_baton, copyfrom_path, copyfrom_rev, dir_pool):
        self.deltas.append((path, Node.DIRECTORY, Changeset.ADD))
        return (path, Changeset.ADD)

    def open_directory(self, path, dir_baton, base_revision, dir_pool):
        return (path, dir_baton[1])

    def change_dir_prop(self, dir_baton, name, value, pool):
        path, change = dir_baton
        if change != Changeset.ADD:
            self.deltas.append((path, Node.DIRECTORY, change))

    def delete_entry(self, path, revision, dir_baton, pool):
        self.deltas.append((path, None, Changeset.DELETE))

    def add_file(self, path, dir_baton, copyfrom_path, copyfrom_revision, dir_pool):
        self.deltas.append((path, Node.FILE, Changeset.ADD))

    def open_file(self, path, dir_baton, dummy_rev, file_pool):
        self.deltas.append((path, Node.FILE, Changeset.EDIT))


class TracError(Exception):
    def __init__(self, message, title=None, show_traceback=0):
        Exception.__init__(self, message)
        self.message = message
        self.title = title
        self.show_traceback = show_traceback
