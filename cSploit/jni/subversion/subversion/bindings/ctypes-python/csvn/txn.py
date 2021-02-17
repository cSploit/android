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


import csvn.core as svn
from csvn.core import *
import os

class Txn(object):
    def __init__(self, session):
        self.pool = Pool()
        self.iterpool = Pool()
        self.session = session
        self.root = _txn_operation(None, "OPEN", svn_node_dir)
        self.commit_callback = None
        self.ignore_func = None
        self.autoprop_func = None

    def ignore(self, ignore_func):
        """Setup a callback function which decides whether a
           new directory or path should be added to the repository.

           IGNORE_FUNC must be a function which accepts two arguments:
           (path, kind)

           PATH is the path which is about to be added to the repository.
           KIND is either svn_node_file or svn_node_dir, depending
           on whether the proposed path is a file or a directory.

           If IGNORE_FUNC returns True, the path will be ignored. Otherwise,
           the path will be added.

           Note that IGNORE_FUNC is only called when new files or
           directories are added to the repository. It is not called,
           for example, when directories within the repository are moved
           or copied, since these copies are not new."""

        self.ignore_func = ignore_func

    def autoprop(self, autoprop_func):
        """Setup a callback function which automatically sets up
           properties on new files or directories added to the
           repository.

           AUTOPROP_FUNC must be a function which accepts three
           arguments: (txn, path, kind)

           TXN is this transaction object.
           PATH is the path which was just added to the repository.
           KIND is either svn_node_file or svn_node_dir, depending
           on whether the newly added path is a file or a directory.

           If AUTOPROP_FUNC wants to set properties on PATH, it should
           call TXN.propset with the appropriate arguments.

           Note that AUTOPROP_FUNC is only called when new files or
           directories are added to the repository. It is not called,
           for example, when directories within the repository are moved
           or copied, since these copies are not new."""

        self.autoprop_func = autoprop_func

    def check_path(self, path, rev=None):
        """Check the status of PATH@REV. If PATH or any of its
           parents have been modified in this transaction, take this
           into consideration."""
        path = self.session._relative_path(path)
        return self._check_path(path, rev)[0]

    def delete(self, path, base_rev=None):
        """Delete PATH from the repository as of base_rev"""

        path = self.session._relative_path(path)

        kind, parent = self._check_path(path, base_rev)

        if kind == svn_node_none:
            if base_rev:
                message = "'%s' not found in rev %d" % (path, base_rev)
            else:
                message = "'%s' not found" % (path)
            raise SubversionException(SVN_ERR_BAD_URL, message)

        parent.open(path, "DELETE", kind)

    def mkdir(self, path):
        """Create a directory at PATH."""

        path = self.session._relative_path(path)

        if self.ignore_func and self.ignore_func(path, svn_node_dir):
            return

        kind, parent = self._check_path(path)

        if kind != svn_node_none:
            if kind == svn_node_dir:
                message = ("Can't create directory '%s': "
                           "Directory already exists" % path)
            else:
                message = ("Can't create directory '%s': "
                           "Path obstructed by file" % path)
            raise SubversionException(SVN_ERR_BAD_URL, message)

        parent.open(path, "ADD", svn_node_dir)

        # Trigger autoprop_func on new directory adds
        if self.autoprop_func:
            self.autoprop_func(self, path, svn_node_dir)

    def propset(self, path, key, value):
        """Set the property named KEY to VALUE on the specified PATH"""

        path = self.session._relative_path(path)

        kind, parent = self._check_path(path)

        if kind == svn_node_none:
            message = ("Can't set property on '%s': "
                       "No such file or directory" % path)
            raise SubversionException(SVN_ERR_BAD_URL, message)

        node = parent.open(path, "OPEN", kind)
        node.propset(key, value)

    def propdel(self, path, key):
        """Delete the property named KEY on the specified PATH"""

        path = self.session._relative_path(path)

        kind, parent = self._check_path(path)

        if kind == svn_node_none:
            message = ("Can't delete property on '%s': "
                       "No such file or directory" % path)
            raise SubversionException(SVN_ERR_BAD_URL, message)

        node = parent.open(path, "OPEN", kind)
        node.propdel(key)


    def copy(self, src_path, dest_path, src_rev=None, local_path=None):
        """Copy a file or directory from SRC_PATH@SRC_REV to DEST_PATH.
           If SRC_REV is not supplied, use the latest revision of SRC_PATH.
           If LOCAL_PATH is supplied, update the new copy to match
           LOCAL_PATH."""

        src_path = self.session._relative_path(src_path)
        dest_path = self.session._relative_path(dest_path)

        if not src_rev:
            src_rev = self.session.latest_revnum()

        kind = self.session.check_path(src_path, src_rev, encoded=False)
        _, parent = self._check_path(dest_path)

        if kind == svn_node_none:
            message = ("Can't copy '%s': "
                       "No such file or directory" % src_path)
            raise SubversionException(SVN_ERR_BAD_URL, message)

        if kind == svn_node_file or local_path is None:
            # Mark the file or directory as copied
            parent.open(dest_path, "ADD",
                        kind, copyfrom_path=src_path,
                        copyfrom_rev=src_rev,
                        local_path=local_path)
        else:
            # Mark the directory as copied
            parent.open(dest_path, "ADD",
                        kind, copyfrom_path=src_path,
                        copyfrom_rev=src_rev)

            # Upload any changes from the supplied local path
            # to the remote repository
            self.upload(dest_path, local_path)

    def upload(self, remote_path, local_path):
        """Upload a local file or directory into the remote repository.
           If the given file or directory already exists in the
           repository, overwrite it.

           This function does not add or update ignored files or
           directories."""

        remote_path = self.session._relative_path(remote_path)

        kind = svn_node_none
        if os.path.isdir(local_path):
            kind = svn_node_dir
        elif os.path.exists(local_path):
            kind = svn_node_file

        # Don't add ignored files or directories
        if self.ignore_func and self.ignore_func(remote_path, kind):
            return

        if (os.path.isdir(local_path) and
              self.check_path(remote_path) != svn_node_dir):
            self.mkdir(remote_path)
        elif not os.path.isdir(local_path) and os.path.exists(local_path):
            self._upload_file(remote_path, local_path)

        ignores = []

        for root, dirs, files in os.walk(local_path):

            # Convert the local root into a remote root
            remote_root = root.replace(local_path.rstrip(os.path.sep),
                                       remote_path.rstrip("/"))
            remote_root = remote_root.replace(os.path.sep, "/").rstrip("/")

            # Don't process ignored subdirectories
            if (self.ignore_func and self.ignore_func(root, svn_node_dir)
                or root in ignores):

                # Ignore children too
                for name in dirs:
                    ignores.append("%s/%s" % (remote_root, name))

                # Skip to the next tuple
                continue

            # Add all subdirectories
            for name in dirs:
                remote_dir = "%s/%s" % (remote_root, name)
                self.mkdir(remote_dir)

            # Add all files in this directory
            for name in files:
                remote_file = "%s/%s" % (remote_root, name)
                local_file = os.path.join(root, name)
                self._upload_file(remote_file, local_file)

    def _txn_commit_callback(self, info, baton, pool):
        self._txn_committed(info[0])

    def commit(self, message, base_rev = None):
        """Commit all changes to the remote repository"""

        if base_rev is None:
            base_rev = self.session.latest_revnum()

        commit_baton = c_void_p()

        self.commit_callback = svn_commit_callback2_t(self._txn_commit_callback)
        (editor, editor_baton) = self.session._get_commit_editor(message,
            self.commit_callback, commit_baton, self.pool)

        child_baton = c_void_p()
        try:
            self.root.replay(editor[0], self.session, base_rev, editor_baton)
        except SubversionException:
            try:
                SVN_ERR(editor[0].abort_edit(editor_baton, self.pool))
            except SubversionException:
                pass
            raise

        return self.committed_rev

    # This private function handles commits and saves
    # information about them in this object
    def _txn_committed(self, info):
        self.committed_rev = info.revision
        self.committed_date = info.date
        self.committed_author = info.author
        self.post_commit_err = info.post_commit_err

    # This private function uploads a single file to the
    # remote repository. Don't use this function directly.
    # Use 'upload' instead.
    def _upload_file(self, remote_path, local_path):

        if self.ignore_func and self.ignore_func(remote_path, svn_node_file):
            return

        kind, parent = self._check_path(remote_path)
        if svn_node_none == kind:
            mode = "ADD"
        else:
            mode = "OPEN"

        parent.open(remote_path, mode, svn_node_file,
                    local_path=local_path)

        # Trigger autoprop_func on new file adds
        if mode == "ADD" and self.autoprop_func:
            self.autoprop_func(self, remote_path, svn_node_file)

    # Calculate the kind of the specified file, and open a handle
    # to its parent operation.
    def _check_path(self, path, rev=None):
        path_components = path.split("/")
        parent = self.root
        copyfrom_path = None
        total_path = path_components[0]
        for path_component in path_components[1:]:
            parent = parent.open(total_path, "OPEN")
            if parent.copyfrom_path:
                copyfrom_path = parent.copyfrom_path
                rev = parent.copyfrom_rev

            total_path = "%s/%s" % (total_path, path_component)
            if copyfrom_path:
                copyfrom_path = "%s/%s" % (copyfrom_path, path_component)

        if path in parent.ops:
            node = parent.open(path)
            if node.action == "DELETE":
                kind = svn_node_none
            else:
                kind = node.kind
        else:
            kind = self.session.check_path(copyfrom_path or total_path, rev,
                                           encoded=False)

        return (kind, parent)



class _txn_operation(object):
    def __init__(self, path, action, kind, copyfrom_path = None,
                 copyfrom_rev = -1, local_path = None):
        self.path = path
        self.action = action
        self.kind = kind
        self.copyfrom_path = copyfrom_path
        self.copyfrom_rev = copyfrom_rev
        self.local_path = local_path
        self.ops = {}
        self.properties = {}

    def propset(self, key, value):
        """Set the property named KEY to VALUE on this file/dir"""
        self.properties[key] = value

    def propdel(self, key):
        """Delete the property named KEY on this file/dir"""
        self.properties[key] = None

    def open(self, path, action="OPEN", kind=svn_node_dir,
             copyfrom_path = None, copyfrom_rev = -1, local_path = None):
        if path in self.ops:
            op = self.ops[path]
            if action == "OPEN" and op.kind in (svn_node_dir, svn_node_file):
                return op
            elif action == "ADD" and op.action == "DELETE":
                op.action = "REPLACE"
                op.local_path = local_path
                op.copyfrom_path = copyfrom_path
                op.copyfrom_rev = copyfrom_rev
                op.kind = kind
                return op
            elif (action == "DELETE" and op.action == "OPEN" and
                  kind == svn_node_dir):
                op.action = action
                return op
            else:
                # throw error
                pass
        else:
            self.ops[path] = _txn_operation(path, action, kind,
                                            copyfrom_path = copyfrom_path,
                                            copyfrom_rev = copyfrom_rev,
                                            local_path = local_path)
            return self.ops[path]

    def replay(self, editor, session, base_rev, baton):
        subpool = Pool()
        child_baton = c_void_p()
        file_baton = c_void_p()
        if self.path is None:
            SVN_ERR(editor.open_root(baton, svn_revnum_t(base_rev), subpool,
                                     byref(child_baton)))
        else:
            if self.action == "DELETE" or self.action == "REPLACE":
                SVN_ERR(editor.delete_entry(self.path, base_rev, baton,
                                            subpool))
            elif self.action == "OPEN":
                if self.kind == svn_node_dir:
                    SVN_ERR(editor.open_directory(self.path, baton,
                            svn_revnum_t(base_rev), subpool,
                            byref(child_baton)))
                else:
                    SVN_ERR(editor.open_file(self.path, baton,
                                svn_revnum_t(base_rev), subpool,
                                byref(file_baton)))

            if self.action in ("ADD", "REPLACE"):
                copyfrom_path = None
                if self.copyfrom_path is not None:
                    copyfrom_path = session._abs_copyfrom_path(
                        self.copyfrom_path)
                if self.kind == svn_node_dir:
                    SVN_ERR(editor.add_directory(
                        self.path, baton, copyfrom_path,
                        svn_revnum_t(self.copyfrom_rev), subpool,
                        byref(child_baton)))
                else:
                    SVN_ERR(editor.add_file(self.path, baton,
                        copyfrom_path, svn_revnum_t(self.copyfrom_rev),
                        subpool, byref(file_baton)))

            # Write out changes to properties
            for (name, value) in self.properties.items():
                if value is None:
                    svn_value = POINTER(svn_string_t)()
                else:
                    svn_value = svn_string_ncreate(value, len(value),
                                                   subpool)
                if file_baton:
                    SVN_ERR(editor.change_file_prop(file_baton, name,
                            svn_value, subpool))
                elif child_baton:
                    SVN_ERR(editor.change_dir_prop(child_baton, name,
                            svn_value, subpool))

            # If there's a source file, and we opened a file to write,
            # write out the contents
            if self.local_path and file_baton:
                handler = svn_txdelta_window_handler_t()
                handler_baton = c_void_p()
                f = POINTER(apr_file_t)()
                SVN_ERR(editor.apply_textdelta(file_baton, NULL, subpool,
                        byref(handler), byref(handler_baton)))

                svn_io_file_open(byref(f), self.local_path, APR_READ,
                                 APR_OS_DEFAULT, subpool)
                contents = svn_stream_from_aprfile(f, subpool)
                svn_txdelta_send_stream(contents, handler, handler_baton,
                                        NULL, subpool)
                svn_io_file_close(f, subpool)

            # If we opened a file, we need to close it
            if file_baton:
                SVN_ERR(editor.close_file(file_baton, NULL, subpool))

        if self.kind == svn_node_dir and self.action != "DELETE":
            assert(child_baton)

            # Look at the children
            for op in self.ops.values():
                op.replay(editor, session, base_rev, child_baton)

            if self.path:
                # Close the directory
                SVN_ERR(editor.close_directory(child_baton, subpool))
            else:
                # Close the editor
                SVN_ERR(editor.close_edit(baton, subpool))

