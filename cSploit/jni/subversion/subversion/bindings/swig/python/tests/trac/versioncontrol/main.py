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

class Repository(object):
    """
    Base class for a repository provided by a version control system.
    """

    def __init__(self, authz):
        self.authz = authz or Authorizer()

    def close(self):
        """
        Close the connection to the repository.
        """
        raise NotImplementedError

    def get_changeset(self, rev):
        """
        Retrieve a Changeset object that describes the changes made in
        revision 'rev'.
        """
        raise NotImplementedError

    def has_node(self, path, rev):
        """
        Tell if there's a node at the specified (path,rev) combination.
        """
        raise NotImplementedError

    def get_node(self, path, rev=None):
        """
        Retrieve a Node (directory or file) from the repository at the
        given path. If the rev parameter is specified, the version of the
        node at that revision is returned, otherwise the latest version
        of the node is returned.
        """
        raise NotImplementedError

    def get_oldest_rev(self):
        """
        Return the oldest revision stored in the repository.
        """
        raise NotImplementedError
    oldest_rev = property(lambda x: x.get_oldest_rev())

    def get_youngest_rev(self):
        """
        Return the youngest revision in the repository.
        """
        raise NotImplementedError
    youngest_rev = property(lambda x: x.get_youngest_rev())

    def previous_rev(self, rev):
        """
        Return the revision immediately preceding the specified revision.
        """
        raise NotImplementedError

    def next_rev(self, rev):
        """
        Return the revision immediately following the specified revision.
        """
        raise NotImplementedError

    def rev_older_than(self, rev1, rev2):
        """
        Return True if rev1 is older than rev2, i.e. if rev1 comes before rev2
        in the revision sequence.
        """
        raise NotImplementedError

    def get_path_history(self, path, rev=None, limit=None):
        """
        Retrieve all the revisions containing this path (no newer than 'rev').
        The result format should be the same as the one of Node.get_history()
        """
        raise NotImplementedError

    def normalize_path(self, path):
        """
        Return a canonical representation of path in the repos.
        """
        return NotImplementedError

    def normalize_rev(self, rev):
        """
        Return a canonical representation of a revision in the repos.
        'None' is a valid revision value and represents the youngest revision.
        """
        return NotImplementedError

    def get_deltas(self, old_path, old_rev, new_path, new_rev, ignore_ancestry=1):
        """
        Generator that yields change tuples (old_node, new_node, kind, change)
        for each node change between the two arbitrary (path,rev) pairs.

        The old_node is assumed to be None when the change is an ADD,
        the new_node is assumed to be None when the change is a DELETE.
        """
        raise NotImplementedError


class Node(object):
    """
    Represents a directory or file in the repository.
    """

    DIRECTORY = "dir"
    FILE = "file"

    def __init__(self, path, rev, kind):
        assert kind in (Node.DIRECTORY, Node.FILE), "Unknown node kind %s" % kind
        self.path = str(path)
        self.rev = rev
        self.kind = kind

    def get_content(self):
        """
        Return a stream for reading the content of the node. This method
        will return None for directories. The returned object should provide
        a read([len]) function.
        """
        raise NotImplementedError

    def get_entries(self):
        """
        Generator that yields the immediate child entries of a directory, in no
        particular order. If the node is a file, this method returns None.
        """
        raise NotImplementedError

    def get_history(self, limit=None):
        """
        Generator that yields (path, rev, chg) tuples, one for each revision in which
        the node was changed. This generator will follow copies and moves of a
        node (if the underlying version control system supports that), which
        will be indicated by the first element of the tuple (i.e. the path)
        changing.
        Starts with an entry for the current revision.
        """
        raise NotImplementedError

    def get_previous(self):
        """
        Return the (path, rev, chg) tuple corresponding to the previous
        revision for that node.
        """
        skip = True
        for p in self.get_history(2):
            if skip:
                skip = False
            else:
                return p

    def get_properties(self):
        """
        Returns a dictionary containing the properties (meta-data) of the node.
        The set of properties depends on the version control system.
        """
        raise NotImplementedError

    def get_content_length(self):
        raise NotImplementedError
    content_length = property(lambda x: x.get_content_length())

    def get_content_type(self):
        raise NotImplementedError
    content_type = property(lambda x: x.get_content_type())

    def get_name(self):
        return self.path.split('/')[-1]
    name = property(lambda x: x.get_name())

    def get_last_modified(self):
        raise NotImplementedError
    last_modified = property(lambda x: x.get_last_modified())

    isdir = property(lambda x: x.kind == Node.DIRECTORY)
    isfile = property(lambda x: x.kind == Node.FILE)


class Changeset(object):
    """
    Represents a set of changes of a repository.
    """

    ADD = 'add'
    COPY = 'copy'
    DELETE = 'delete'
    EDIT = 'edit'
    MOVE = 'move'

    def __init__(self, rev, message, author, date):
        self.rev = rev
        self.message = message
        self.author = author
        self.date = date

    def get_changes(self):
        """
        Generator that produces a (path, kind, change, base_rev, base_path)
        tuple for every change in the changeset, where change can be one of
        Changeset.ADD, Changeset.COPY, Changeset.DELETE, Changeset.EDIT or
        Changeset.MOVE, and kind is one of Node.FILE or Node.DIRECTORY.
        """
        raise NotImplementedError


class PermissionDenied(Exception):
    """
    Exception raised by an authorizer if the user has insufficient permissions
    to view a specific part of the repository.
    """
    pass


class Authorizer(object):
    """
    Base class for authorizers that are responsible to granting or denying
    access to view certain parts of a repository.
    """

    def assert_permission(self, path):
        if not self.has_permission(path):
            raise PermissionDenied('Insufficient permissions to access %s' % path)

    def has_permission(self, path):
        return 1

    def has_permission_for_changeset(self, rev):
        return 1

