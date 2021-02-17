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
import csvn.types as _types
from csvn.ext.callback_receiver import CallbackReceiver
from txn import Txn
from auth import User
import os

class RepositoryURI(object):
    """A URI to an object in a Subversion repository, stored internally in
       encoded format.

       When you supply URIs to a RemoteClient, or a transaction"""

    def __init__(self, uri, encoded=True):
        """Create a RepositoryURI object from a URI. If encoded=True, the
           input string may be URI-encoded."""
        pool = Pool()
        if not encoded:
            uri = svn_path_uri_encode(uri, pool)
        self._as_parameter_ = str(svn_path_canonicalize(uri, pool))

    def join(self, uri):
        """Join this URI and the specified relative URI,
           adding a slash if necessary."""
        pool = Pool()
        return RepositoryURI(svn_path_join(self, uri, pool))

    def dirname(self):
        """Get the parent directory of this URI"""
        pool = Pool()
        return RepositoryURI(svn_path_dirname(self, pool))

    def relative_path(self, uri, encoded=True):
        """Convert the supplied URI to a decoded path, relative to me."""
        pool = Pool()
        if not encoded:
            uri = svn_path_uri_encode(uri, pool)
        child_path = svn_path_is_child(self, uri, pool) or uri
        return str(svn_path_uri_decode(child_path, pool))

    def longest_ancestor(self, uri):
        """Get the longest ancestor of this URI and another URI"""
        pool = Pool()
        return RepositoryURI(svn_path_get_longest_ancestor(self, uri, pool))

    def __str__(self):
        """Return the URI as a string"""
        return self._as_parameter_

class RemoteRepository(object):

    def __init__(self, url, user=None):
        """Open a new session to URL with the specified USER.
           USER must be an object that implements the
           'csvn.auth.User' interface."""

        if user is None:
            user = User()

        self.pool = Pool()
        self.iterpool = Pool()
        self.url = RepositoryURI(url)
        self.user = user

        self.client = POINTER(svn_client_ctx_t)()
        svn_client_create_context(byref(self.client), self.pool)

        self.user.setup_auth_baton(pointer(self.client.contents.auth_baton))

        self._as_parameter_ = POINTER(svn_ra_session_t)()
        svn_client_open_ra_session(byref(self._as_parameter_), url,
                                   self.client, self.pool)

        self.client[0].log_msg_func2 = \
            svn_client_get_commit_log2_t(self._log_func_wrapper)
        self.client[0].log_msg_baton2 = c_void_p()
        self._log_func = None

    def close(self):
        """Close this RemoteRepository object, releasing any resources."""
        self.pool.clear()

    def txn(self):
        """Create a transaction"""
        return Txn(self)

    def latest_revnum(self):
        """Get the latest revision number in the repository"""
        revnum = svn_revnum_t()
        svn_ra_get_latest_revnum(self, byref(revnum), self.iterpool)
        self.iterpool.clear()
        return revnum.value

    def check_path(self, path, rev = None, encoded=True):
        """Check the status of PATH@REV. If REV is not specified,
           look at the latest revision in the repository.

        If the path is ...
          ... absent, then we return svn_node_none.
          ... a regular file, then we return svn_node_file.
          ... a directory, then we return svn_node_dir
          ... unknown, then we return svn_node_unknown

        If ENCODED is True, the path may be URI-encoded.
        """

        path = self._relative_path(path, encoded)
        if rev is None:
            rev = self.latest_revnum()
        kind = svn_node_kind_t()
        svn_ra_check_path(self, path, svn_revnum_t(rev), byref(kind),
                          self.iterpool)
        self.iterpool.clear()
        return kind.value

    def list(self, path, rev = SVN_INVALID_REVNUM, fields = SVN_DIRENT_ALL):
        """List the contents of the specified directory PATH@REV. This
           function returns a dictionary, which maps entry names to
           directory entries (svn_dirent_t objects).

           If REV is not specified, we look at the latest revision of the
           repository.

           FIELDS controls what portions of the svn_dirent_t object are
           filled in. To have them completely filled in, just pass in
           SVN_DIRENT_ALL (which is the default); otherwise, pass the
           bitwise OR of all the SVN_DIRENT_ fields you would like to
           have returned to you.
        """
        dirents = _types.Hash(POINTER(svn_dirent_t), None)
        svn_ra_get_dir2(self, dirents.byref(), NULL, NULL, path,
                        rev, fields, dirents.pool)
        self.iterpool.clear()
        return dirents

    def cat(self, buffer, path, rev = SVN_INVALID_REVNUM):
        """Get PATH@REV and save it to BUFFER. BUFFER must be a Python file
           or a StringIO object.

           If REV is not specified, we look at the latest revision of the
           repository."""
        stream = _types.Stream(buffer)
        svn_ra_get_file(self, path, rev, stream, NULL, NULL, stream.pool)
        self.iterpool.clear()

    def info(self, path, rev = None):
        """Get a pointer to a svn_dirent_t object associated with PATH@REV.
           If PATH does not exist, return None.

           If REV is not specified, we look at the latest revision of the
           file."""
        dirent = POINTER(svn_dirent_t)()
        dirent.pool = Pool()
        if rev is None:
            rev = self.latest_revnum()
        svn_ra_stat(self, path, rev, byref(dirent), dirent.pool)
        self.iterpool.clear()
        return dirent

    def proplist(self, path, rev = SVN_INVALID_REVNUM):
        """Return a dictionary containing the properties on PATH@REV

           If REV is not specified, we look at the latest revision of the
           repository."""

        props = _types.Hash(POINTER(svn_string_t), None,
                   wrapper=_types.SvnStringPtr)
        status = self.check_path(path, rev)
        if status == svn_node_dir:
            svn_ra_get_dir2(self, NULL, NULL, props.byref(), path,
                            rev, 0, props.pool)
        else:
            svn_ra_get_file(self, path, rev, NULL, NULL, props.byref(),
                            props.pool)
        self.iterpool.clear()
        return props

    def propget(self, name, path, rev = SVN_INVALID_REVNUM):
        """Get property NAME from PATH@REV.

           If REV is not specified, we look at the latest revision of the
           repository."""

        return self.proplist(path, rev)[name]

    def log(self, start_rev, end_rev, paths=None, limit=0,
            discover_changed_paths=FALSE, stop_on_copy=FALSE):
        """A generator function which returns information about the revisions
           between START_REV and END_REV. Each return value is a
           csvn.types.LogEntry object which describes a revision.

           For details on what fields are contained in a LogEntry object,
           please see the documentation from csvn.types.LogEntry.

           You can iterate through the log information for several revisions
           using a regular for loop. For example:
             for entry in session.log(start_rev, end_rev):
               print("Revision %d" % entry.revision)
               ...

           ARGUMENTS:

             If PATHS is not None and has one or more elements, then only
             show revisions in which at least one of PATHS was changed (i.e.,
             if file, text or props changed; if dir, props changed or an entry
             was added or deleted). Each PATH should be relative to the current
             session's root.

             If LIMIT is non-zero, only the first LIMIT logs are returned.

             If DISCOVER_CHANGED_PATHS is True, then changed_paths will contain
             a list of paths affected by this revision.

             If STOP_ON_COPY is True, then this function will not cross
             copies while traversing history.

             If START_REV or END_REV is a non-existent revision, we throw
             a SVN_ERR_FS_NO_SUCH_REVISION SubversionException, without
             returning any logs.

        """

        paths = _types.Array(c_char_p, paths is None and [""] or paths)
        return iter(_LogMessageReceiver(self, start_rev, end_rev, paths,
                                        limit, discover_changed_paths, stop_on_copy))


    # Private. Produces a delta editor for the commit, so that the Txn
    # class can commit its changes over the RA layer.
    def _get_commit_editor(self, message, commit_callback, commit_baton, pool):
        editor = POINTER(svn_delta_editor_t)()
        editor_baton = c_void_p()
        svn_ra_get_commit_editor2(self, byref(editor),
            byref(editor_baton), message, commit_callback,
            commit_baton, NULL, FALSE, pool)
        return (editor, editor_baton)

    # Private. Convert a URI to a repository-relative path
    def _relative_path(self, path, encoded=True):
        return self.url.relative_path(path, encoded)

    # Private. Convert a repository-relative copyfrom path into a proper
    # copyfrom URI
    def _abs_copyfrom_path(self, path):
        return self.url.join(RepositoryURI(path, False))

    def revprop_list(self, revnum=None):
        """Returns a hash of the revision properties of REVNUM. If REVNUM is
        not provided, it defaults to the head revision."""
        rev = svn_opt_revision_t()
        if revnum is not None:
            rev.kind = svn_opt_revision_number
            rev.value.number = revnum
        else:
            rev.kind = svn_opt_revision_head

        props = _types.Hash(POINTER(svn_string_t), None,
                   wrapper=_types.SvnStringPtr)

        set_rev = svn_revnum_t()

        svn_client_revprop_list(props.byref(),
                        self.url,
                        byref(rev),
                        byref(set_rev),
                        self.client,
                        props.pool)

        self.iterpool.clear()

        return props

    def revprop_get(self, propname, revnum=None):
        """Returns the value of PROPNAME at REVNUM. If REVNUM is not
        provided, it defaults to the head revision."""
        return self.revprop_list(revnum)[propname]

    def revprop_set(self, propname, propval=NULL, revnum=None, force=False):
        """Set PROPNAME to PROPVAL for REVNUM. If REVNUM is not given, it
        defaults to the head revision. Returns the actual revision number
        effected.

        If PROPVAL is not provided, the property will be deleted.

        If FORCE is True (False by default), newlines will be allowed in the
        author property.

        Be careful, this is a lossy operation."""
        rev = svn_opt_revision_t()
        if revnum is not None:
            rev.kind = svn_opt_revision_number
            rev.value.number = revnum
        else:
            rev.kind = svn_opt_revision_head

        set_rev = svn_revnum_t()

        svn_client_revprop_set(propname,
                svn_string_create(propval, self.iterpool), self.url,
                byref(rev), byref(set_rev), force, self.client,
                self.iterpool)

        try:
            return set_rev.value
        finally:
            self.iterpool.clear()


    def set_log_func(self, log_func):
        """Register a callback to get a log message for commit and
        commit-like operations. LOG_FUNC should take an array as an argument,
        which holds the files to be committed. It should return a list of the
        form [LOG, FILE] where LOG is a log message and FILE is the temporary
        file, if one was created instead of a log message. If LOG is None,
        the operation will be canceled and FILE will be treated as the
        temporary file holding the temporary commit message."""
        self._log_func = log_func

    def _log_func_wrapper(self, log_msg, tmp_file, commit_items, baton, pool):
        log_msg[0].raw = NULL
        tmp_file[0] = NULL

        if self._log_func:
            [log, file] = self._log_func(_types.Array(String, commit_items))

            if log:
                log_msg[0].raw = apr_pstrdup(pool, String(log)).raw
            if file:
                tmp_file[0] = apr_pstrdup(pool, String(file)).raw

    def svnimport(self, path, url=None, nonrecursive=False, no_ignore=True, log_func=None):

        if not url:
            url = self.url

        if log_func:
            self.set_log_func(log_func)

        pool = Pool()
        commit_info = POINTER(svn_commit_info_t)()
        svn_client_import2(byref(commit_info), path, url, nonrecursive,
                           no_ignore, self.client, pool)

        commit_info[0].pool = pool
        return commit_info[0]

class LocalRepository(object):
    """A client which accesses the repository directly. This class
       may allow you to perform some administrative actions which
       cannot be performed remotely (e.g. create repositories,
       dump repositories, etc.)

       Unlike RemoteRepository, the functions in this class do not
       accept URIs, and instead only accept local filesystem
       paths.

       By default, this class does not perform any checks to verify
       permissions, assuming that the specified user has full
       administrative access to the repository. To teach this class
       to enforce an authz policy, you must subclass csvn.auth.User
       and implement the allow_access function.
    """

    def __init__(self, path, create=False, user=None):
        """Open the repository at PATH. If create is True,
           create a new repository.

           If specified, user must be a csvn.auth.User instance.
        """
        if user is None:
            user = User()

        self.pool = Pool()
        self.iterpool = Pool()
        self._as_parameter_ = POINTER(svn_repos_t)()
        self.user = user
        if create:
            svn_repos_create(byref(self._as_parameter_), path,
                             None, None, None, None, self.pool)
        else:
            svn_repos_open(byref(self._as_parameter_), path, self.pool)
        self.fs = _fs(self)

    def __del__(self):
        self.close()

    def close(self):
        """Close this LocalRepository object, releasing any resources. In
           particular, this closes the rep-cache DB."""
        self.pool.clear()

    def latest_revnum(self):
        """Get the latest revision in the repository"""
        return self.fs.latest_revnum()

    def check_path(self, path, rev = None, encoded=False):
        """Check whether the given PATH exists in the specified REV. If REV
           is not specified, look at the latest revision.

        If the path is ...
          ... absent, then we return svn_node_none.
          ... a regular file, then we return svn_node_file.
          ... a directory, then we return svn_node_dir
          ... unknown, then we return svn_node_unknown
        """
        assert(not encoded)
        root = self.fs.root(rev=rev, pool=self.iterpool)
        try:
            return root.check_path(path)
        finally:
            self.iterpool.clear()

    def uuid(self):
        """Return a universally-unique ID for this repository"""
        return self.fs.uuid()

    def set_rev_prop(self, rev, name, value, author=NULL):
        """Set the NAME property to VALUE in the specified
           REV, attribute the change to AUTHOR if provided."""
        rev = svn_revnum_t(rev)
        svn_repos_fs_change_rev_prop2(self, rev, author, name, value,
                                      svn_repos_authz_func_t(),
                                      None, self.iterpool)
        self.iterpool.clear()

    def get_rev_prop(self, rev, name):
        """Returns the value of NAME in REV. If NAME does not exist in REV,
        returns None."""
        rev = svn_revnum_t(rev)
        value = POINTER(svn_string_t)()

        svn_repos_fs_revision_prop(byref(value), self, rev, name,
                                   svn_repos_authz_func_t(), None,
                                   self.iterpool)

        try:
            if value:
                return _types.SvnStringPtr.from_param(value)
            else:
                return None
        finally:
            self.iterpool.clear()

    def txn(self):
        """Open up a new transaction, so that you can commit a change
           to the repository"""
        assert self.user is not None, (
               "If you would like to commit changes to the repository, "
               "you must supply a user object when you initialize "
               "the repository object")
        return Txn(self)

    # Private. Produces a delta editor for the commit, so that the Txn
    # class can commit its changes over the RA layer.
    def _get_commit_editor(self, message, commit_callback, commit_baton, pool):
        editor = POINTER(svn_delta_editor_t)()
        editor_baton = c_void_p()
        svn_repos_get_commit_editor4(byref(editor),
            byref(editor_baton), self, None, "", "",
            self.user.username(), message,
            commit_callback, commit_baton, svn_repos_authz_callback_t(),
            None, pool)
        return (editor, editor_baton)

    def _relative_path(self, path):
        return path

    # Private. Convert a repository-relative copyfrom path into a proper
    # copyfrom URI
    def _abs_copyfrom_path(self, path):
        return path

    def load(self, dumpfile, feedbackfile=None,
            uuid_action=svn_repos_load_uuid_default, parent_dir="",
            use_pre_commit_hook=False, use_post_commit_hook=False,
            cancel_func=None):
        """Read and parse dumpfile-formatted DUMPFILE, reconstructing
        filesystem revisions. Dumpfile should be an open python file object
        or file like object. UUID will be handled according to UUID_ACTION
        which defaults to svn_repos_load_uuid_default.

        If FEEDBACKFILE is provided (in the form of a python file object or
        file like object), feedback will be sent to it.

        If PARENT_DIR is provided, everything loaded from the dump will be
        reparented to PARENT_DIR.

        USE_PRE_COMMIT_HOOK and USE_POST_COMMIT_HOOK are False by default,
        if either is set to True that hook will be used.

        If CANCEL_FUNC is provided, it will be called at various points to
        allow the operation to be cancelled. The cancel baton will be the
        LocalRepository object."""

        if not cancel_func:
            cancel_func = svn_cancel_func_t()

        apr_dump = _types.APRFile(dumpfile)
        stream_dump = svn_stream_from_aprfile2(apr_dump._as_parameter_,
                                               False, self.iterpool)

        if feedbackfile:
            apr_feedback = _types.APRFile(feedbackfile)
            stream_feedback = svn_stream_from_aprfile2(
                                apr_feedback._as_parameter_, False,
                                self.iterpool)
        else:
            stream_feedback = NULL

        svn_repos_load_fs2(self._as_parameter_, stream_dump, stream_feedback,
                           uuid_action, parent_dir, use_pre_commit_hook,
                           use_post_commit_hook, cancel_func,
                           c_void_p(), self.iterpool)

        apr_dump.close()
        if feedbackfile:
            apr_feedback.close()

        self.iterpool.clear()

class _fs(object):
    """NOTE: This is a private class. Don't use it outside of
       this module. Use the Repos class instead.

       This class represents an svn_fs_t object"""

    def __init__(self, repos):
        self.iterpool = Pool()
        self._as_parameter_ = svn_repos_fs(repos)

    def latest_revnum(self):
        """See Repos.latest_revnum"""
        rev = svn_revnum_t()
        svn_fs_youngest_rev(byref(rev), self, self.iterpool)
        self.iterpool.clear()
        return rev.value

    def uuid(self):
        """See Repos.uuid"""
        uuid_buffer = String()
        svn_fs_get_uuid(self, byref(uuid_buffer), self.iterpool)
        uuid_str = str(uuid_buffer)
        self.iterpool.clear()
        return uuid_str

    def root(self, rev = None, txn = None, pool = None,
             iterpool = None):
        """Create a new svn_fs_root_t object from txn or rev.
           If neither txn nor rev or set, this root object will
           point to the latest revision root.

           The svn_fs_root object itself will be allocated in pool.
           If iterpool is supplied, iterpool will be used for any
           temporary allocations. Otherwise, pool will be used for
           temporary allocations."""
        return _fs_root(self, rev, txn, pool, iterpool)

class _fs_root(object):
    """NOTE: This is a private class. Don't use it outside of
       this module. Use the Repos.txn() method instead.

       This class represents an svn_fs_root_t object"""

    def __init__(self, fs, rev = None, txn = None, pool = None,
                 iterpool = None):
        """See _fs.root()"""

        assert(pool)

        self.pool = pool
        self.iterpool = iterpool or pool
        self.fs = fs
        self._as_parameter_ = POINTER(svn_fs_root_t)()

        if txn and rev:
            raise Exception("You can't specify both a txn and a rev")

        if txn:
            svn_fs_txn_root(byref(self._as_parameter_), txn, self.pool)
        else:
            if not rev:
                rev = fs.latest_revnum()
            svn_fs_revision_root(byref(self._as_parameter_), fs, rev, self.pool)

    def check_path(self, path):
        """Check whether the specified path exists in this root.
           See Repos.check_path() for details."""

        kind = svn_node_kind_t()
        svn_fs_check_path(byref(kind), self, path, self.iterpool)

        return kind.value

class LogEntry(object):
    """REVISION, AUTHOR, DATE, and MESSAGE are straightforward, and
       contain what you expect. DATE is a csvn.types.SvnDate object.

       If no information about the paths changed in this revision is
       available, CHANGED_PATHS will be None. Otherwise, CHANGED_PATHS
       will contain a dictionary which maps every path committed
       in REVISION to svn_log_changed_path_t pointers."""

    __slots__ = ['changed_paths', 'revision',
                 'author', 'date', 'message']

class _LogMessageReceiver(CallbackReceiver):

    def collect(self, session, start_rev, end_rev, paths, limit,
                discover_changed_paths, stop_on_copy):
        self.discover_changed_paths = discover_changed_paths
        pool = Pool()
        baton = c_void_p()
        receiver = svn_log_message_receiver_t(self.receive)
        svn_ra_get_log(session, paths, start_rev, end_rev,
                       limit, discover_changed_paths, stop_on_copy, receiver,
                       baton, pool)

    def receive(self, baton, changed_paths, revision, author, date, message, pool):
        entry = LogEntry()

        # Save information about the log entry
        entry.revision = revision
        entry.author = str(author)
        entry.date = _types.SvnDate(date)
        entry.message = str(message)

        if self.discover_changed_paths:
            entry.changed_paths = _types.Hash(POINTER(svn_log_changed_path_t),
              changed_paths, dup = svn_log_changed_path_dup)
        else:
            entry.changed_paths = None

        self.send(entry)

