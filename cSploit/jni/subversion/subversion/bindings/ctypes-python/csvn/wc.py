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


from csvn.core import *
from csvn.auth import *
from ctypes import *
import csvn.types as _types
import os, sys

class WC(object):
    """A SVN working copy."""

    def __init__(self, path="", user=None):
        """Open a working copy directory relative to path.

        Keyword arguments:
        path -- path to the working copy (default current working directory)
        user -- object implementing the user interface representing the user
            performing the operation (defaults to an instance of the User class)
        """
        if user is None:
            user = User()

        self.pool = Pool()
        self.iterpool = Pool()
        self.path = path.replace(os.sep, "/")
        self.user = user

        self.client = POINTER(svn_client_ctx_t)()
        svn_client_create_context(byref(self.client), self.pool)
        self._as_parameter_ = POINTER(svn_ra_session_t)()

        self.user.setup_auth_baton(pointer(self.client.contents.auth_baton))

        self.client[0].notify_func2 = \
            svn_wc_notify_func2_t(self._notify_func_wrapper)
        self.client[0].notify_baton2 = c_void_p()
        self._notify_func = None

        self.client[0].cancel_func = \
            svn_cancel_func_t(self._cancel_func_wrapper)
        self.client[0].cancel_baton = c_void_p()
        self._cancel_func = None

        self.client[0].progress_func = \
            svn_ra_progress_notify_func_t(self._progress_func_wrapper)
        self.client[0].progress_baton =  c_void_p()
        self._progress_func = None

        self._status_func = \
            svn_wc_status_func2_t(self._status_wrapper)
        self._status = None

        self._info_func = \
            svn_info_receiver_t(self._info_wrapper)
        self._info = None

        self._list_func = \
            svn_client_list_func_t(self._list_wrapper)
        self._list = None

        self.client[0].log_msg_func2 = \
            svn_client_get_commit_log2_t(self._log_func_wrapper)
        self.client[0].log_msg_baton2 = c_void_p()
        self._log_func = None

    def close(self):
        """Close this WC object, releasing any resources."""
        self.pool.clear()

    def copy(self, src, dest, rev = ""):
        """Copy src to dest.

        Keyword arguments:
        src -- current file name
        dest -- new file name"""
        opt_rev = svn_opt_revision_t()
        dummy_rev = svn_opt_revision_t()
        info = POINTER(svn_commit_info_t)()
        svn_opt_parse_revision(byref(opt_rev), byref(dummy_rev),
                               str(rev), self.iterpool)

        svn_client_copy3(byref(info),
                         self._build_path(src),
                         byref(opt_rev),
                         self._build_path(dest),
                         self.client, self.iterpool)
        self.iterpool.clear()

    def move(self, src, dest, force = False):
        """Move src to dest.

        Keyword arguments:
        src -- current file name
        dest -- new file name
        force -- if True, operation will be forced (default False)"""

        info = POINTER(svn_commit_info_t)()
        svn_client_move3(byref(info),
                         self._build_path(src),
                         self._build_path(dest),
                         force,
                         self.client, self.iterpool)
        self.iterpool.clear()

    def delete(self, paths, force = False):
        """Schedule paths to be deleted.

        Keyword arguments:
        paths -- list of paths to marked for deletion
        force -- if True, operation will be forced (default False)"""

        info = POINTER(svn_commit_info_t)()
        svn_client_delete2(byref(info), self._build_path_list(paths),
                           force, self.client, self.iterpool)
        self.iterpool.clear()

    def add(self, path, recurse = True, force = False, no_ignore = False):
        """Schedule path to be added.

        Keyword arguments:
        path -- path to be marked for addition
        recurse -- if True, the contents of directories will also be
            added (default True)
        force -- if True, the operation will be forced (default False)
        no_ignore -- if True, nothing will be ignored (default False)"""

        svn_client_add3(self._build_path(path),
                        recurse, force, no_ignore, self.client,
                        self.iterpool)
        self.iterpool.clear()

    def resolve(self, path, recurse = True):
        """Resolve a conflict on path.

        This operation does not actually change path at all, it just marks the
        conflict as resolved.

        Keyword arguments:
        path -- path to be marked as resolved
        recurse -- if True, directories will be recursed (default True)"""

        svn_client_resolved(path, recurse, self.client, self.iterpool)
        self.iterpool.clear()

    def revert(self, paths, recurse = False):
        """Revert paths to the most recent version.

        Keyword arguments:
        paths -- list of paths to be reverted
        recurse -- if True, directories will be recursed (default True)"""

        svn_client_revert(self._build_path_list(paths), recurse,
                          self.client, self.iterpool)
        self.iterpool.clear()

    def _build_path_list(self, paths):
        """Build a list of canonicalized WC paths.

        In general, the user does not need to call this method, paths will be
        canonicalized as needed for you.

        Returns an array of canonicalized paths.

        Keyword arguments:
        paths -- list of paths to be canonicalized"""

        # If the user passed in a string, the user made a mistake.
        # The user should be passing in a list of paths.
        assert not isinstance(paths, str)

        canonicalized_paths = [self._build_path(path) for path in paths]
        return _types.Array(String, canonicalized_paths)

    def _build_path(self, path):
        """Build a canonicalized path.

        In general, the user does not need to call this method, paths will be
        canonicalized as needed for you.

        Returns a canonical path.

        Keyword arguments:
        path -- path to be canonicalized"""
        joined_path = os.path.join(self.path.replace("/", os.sep), path.replace("/", os.sep))
        return svn_path_canonicalize(joined_path.replace(os.sep, "/"), self.iterpool)

    def set_notify_func(self, notify_func):
        """Setup a callback so that you can be notified when paths are
        affected by WC operations.

        When paths are affected, we will call the function with an
        svn_wc_notify_t object. For details on the contents of an
        svn_wc_notify_t object, see the documentation for svn_wc_notify_t.

        Keyword arguments:
        notify_func -- function to be used in the callback"""
        self._notify_func = notify_func

    # A helper function which invokes the user notify function with
    # the appropriate arguments.
    def _notify_func_wrapper(self, baton, notify, pool):
        """Used to wrap the user callback."""
        if self._notify_func:
            pool = Pool()
            notify = svn_wc_dup_notify(notify, pool)[0]
            notify.pool = pool
            self._notify_func(notify[0])

    def set_cancel_func(self, cancel_func):
        """Setup a callback so that you can cancel operations.

        At various times the cancel function will be called, giving
        the option of cancelling the operation.

        Keyword arguments:
        cancel_func -- function to be used in the callback"""

        self._cancel_func = cancel_func

    #Just like _notify_func_wrapper, above.
    def _cancel_func_wrapper(self, baton):
        """Used to wrap the cancel callback."""
        if self._cancel_func:
            self._cancel_func()

    def set_progress_func(self, progress_func):
        """Setup a callback for network progress information.

        This callback should accept two integers, being the number of bytes
        sent and the number of bytes to send.

        Keyword arguments:
        progress_func -- function to be used in the callback"""

        self._progress_func = progress_func

    def _progress_func_wrapper(self, progress, total, baton, pool):
        if self._progress_func:
            self._progress_func(progress, total)

    def diff(self, path1="", revnum1=None, path2=None, revnum2=None,
             diff_options=[], recurse=True, ignore_ancestry=True,
             no_diff_deleted=False, ignore_content_type=False,
             header_encoding="", outfile = sys.stdout, errfile = sys.stderr):
        """Produce svn diff output that describes the difference between
        PATH1 at REVISION1 and PATH2 at REVISION2.

        Keyword arguments:
        path1 -- path to compare in diff (defaults to working copy root)
        revnum1 -- revision to look at path1 at (defaults to base revision)
        path2 -- second path to compare in diff (defaults to path1)
        revnum2 -- revision to look at path2 at (defaults to working revision)
        diff_options -- list of options to be passed to the diff process
            (defaults to an empty list)
        recurse -- if True, contents of directories will also be diffed
            (default True)
        ignore_ancestry -- if True then items will not be checked for
            relatedness before being diffed (default True)
        no_diff_deleted -- if True then deleted items will not be included in
            the diff (default False)
        ignore_content_type -- if True diffs will be generated for binary file
            types (default False)
        header_encoding -- generated headers will be encoded using this
            encoding (defaults to "")
        outfile -- file to save output to, which can be a file like object
            (defaults to sys.stdout)
        errfile -- file to save output to, which can be a file like object
            (defaults to sys.stderr)"""

        diff_options = self._build_path_list(diff_options)

        rev1 = svn_opt_revision_t()
        if revnum1:
            rev1.kind = svn_opt_revision_number
            rev1.value.number = revnum1
        else:
            rev1.kind = svn_opt_revision_base

        rev2 = svn_opt_revision_t()
        if revnum2:
            rev2.kind = svn_opt_revision_number
            rev2.value.number = revnum2
        else:
            rev2.kind = svn_opt_revision_working

        path1 = self._build_path(path1)
        if path2:
            path2 = self._build_path(path2)
        else:
            path2 = path1

        # Create temporary objects for output and errors
        apr_outfile = _types.APRFile(outfile)
        apr_errfile = _types.APRFile(errfile)

        svn_client_diff3(diff_options, path1, byref(rev1), path2, byref(rev2), recurse,
            ignore_ancestry, no_diff_deleted, ignore_content_type,
            header_encoding, apr_outfile, apr_errfile, self.client,
            self.iterpool)

        # Close the APR wrappers
        apr_outfile.close()
        apr_errfile.close()

        self.iterpool.clear()

    def cleanup(self, path=""):
        """Recursively cleanup the working copy.

        Cleanup means: finish any incomplete operations and release all locks.

        Keyword arguments:
        path -- path to cleanup (defaults to WC root)"""
        svn_client_cleanup(self._build_path(path), self.client,
                            self.iterpool)

        self.iterpool.clear()

    def export(self, from_path, to_path, overwrite=False,
                ignore_externals=True, recurse=True, eol=NULL):
        """Export from_path to to_path.

        An export creates a clean copy of the exported items, with no
        subversion metadata.

        Keyword arguments:
        from_path -- path to export
        to_path -- location to export to
        overwrite -- if True, existing items at to_path will be overwritten
            (default False)
        ignore_externals -- if True, export does not include externals (default
            True)
        recurse -- if True, directory contents will also be exported (default
            True)
        eol -- End of line character to use (defaults to standard eol marker)"""

        rev = svn_opt_revision_t()
        peg_rev = svn_opt_revision_t()

        svn_client_export3(POINTER(svn_revnum_t)(),
                           self._build_path(from_path),
                           self._build_path(to_path), byref(peg_rev),
                           byref(rev), overwrite, ignore_externals, recurse,
                           eol, self.client, self.iterpool)

        self.iterpool.clear()

    def resolved(self, path="", recursive=True):
        """Resolve a conflict on PATH. Marks the conflict as resolved, does
        not change the text of PATH.

        If RECURSIVE is True (True by default) then directories will be
        recursed."""
        svn_client_resolved(self._build_path(path), recursive, self.client,
                            self.iterpool)
        self.iterpool.clear()

    def mkdir(self, paths):
        """Create directories in the working copy.

        Keyword arguments:
        paths -- list of paths to be created"""
        paths = self._build_path_list(paths)

        # The commit info shouldn't matter, this is a method of the WC
        # class, so it isn't intended for remote operations.
        info = POINTER(svn_commit_info_t)()

        svn_client_mkdir2(byref(info), paths, self.client, self.iterpool)

        self.iterpool.clear()

    def propset(self, propname, propval, target="", recurse=True,
                skip_checks=False):
        """Set propname to propval for target.

        Keyword arguments:
        propname -- name of property to be set
        propval -- value to set property to
        target -- path to set property for (default WC root)
        recurse -- if True and target is a directory the operation will recurse
            setting the property for the contents of that directory (default
            True)
        skip_checks -- if True, no sanity checks will be performed on propname
            (default False)"""

        svn_client_propset2(propname,
                            svn_string_create(propval, self.iterpool),
                            self._build_path(target), recurse, skip_checks,
                            self.client, self.iterpool)

        self.iterpool.clear()

    def proplist(self, target="", recurse=True):
        """List the values of the normal properties of target.

        Returns an array of svn_client_proplist_item_t objects.

        Keyword arguments:
        target -- path to list properties for (defaults to WC root)
        recurse -- if True, the operation will recurse (default True)"""
        peg_revision = svn_opt_revision_t()
        peg_revision.kind = svn_opt_revision_unspecified

        revision = svn_opt_revision_t()
        revision.kind = svn_opt_revision_working

        props = _types.Array(svn_client_proplist_item_t)

        svn_client_proplist2(byref(props.header), self._build_path(target),
                             byref(peg_revision), byref(revision), recurse,
                             self.client, props.pool)

        self.iterpool.clear()

        return props

    def propget(self, propname, target="", recurse=True):
        """Get the value of propname for target.

        Returns a hash the keys of which are file paths and the values are the
        value of PROPNAME for the corresponding file. The values of the hash
        are c_char_p objects, which can be treated much like strings.

        Keyword arguments:
        propname -- name of property to get
        target -- item to get properties for (defaults to Wc root)
        recurse -- if True, operation will recurse into directories (default
            True)"""
        peg_revision = svn_opt_revision_t()
        peg_revision.kind = svn_opt_revision_unspecified

        revision = svn_opt_revision_t()
        revision.kind = svn_opt_revision_working

        props = _types.Hash(c_char_p)

        svn_client_propget2(byref(props.hash), propname,
                    self._build_path(target), byref(peg_revision),
                    byref(revision), recurse, self.client, props.pool)

        self.iterpool.clear()
        return props

    # Internal method to wrap status callback.
    def _status_wrapper(self, baton, path, status):
        """Wrapper for status callback."""
        if self._status:
            pool = Pool()
            status = svn_wc_dup_status2(status, pool)[0]
            status.pool = pool
            self._status(path, status)

    def set_status_func(self, status):
        """Set a callback function to be used the next time the status method
        is called.

        Keyword arguments:
        status -- function to be used in status callback"""
        self._status = status

    def status(self, path="", status=None, recurse=True,
                get_all=False, update=False, no_ignore=False,
                ignore_externals=False):
        """Get the status on path using callback to status.

        The status callback (which can be set when this method is called or
        earlier) will be called for each item.

        Keyword arguments:
        path -- items to get status for (defaults to WC root)
        status -- callback to be used (defaults to previously registered
            callback)
        recurse -- if True, the contents of directories will also be checked
            (default True)
        get_all -- if True, all entries will be retrieved, otherwise only
            interesting entries (local modifications and/or out-of-date) will
            be retrieved (default False)
        update -- if True, the repository will be contacted to get information
            about out-of-dateness and a revision number will be returned to
            indicate which revision the Wc was compared against (default False)
        no_ignore -- if True, nothing is ignored (default False)
        ignore_externals -- if True, externals will be ignored (default False)
        """
        rev = svn_opt_revision_t()
        rev.kind = svn_opt_revision_working

        if status:
            self.set_status_func(status)

        result_rev = svn_revnum_t()
        svn_client_status2(byref(result_rev), self._build_path(path),
                           byref(rev), self._status_func,
                           c_void_p(), recurse, get_all,
                           update, no_ignore, ignore_externals, self.client,
                           self.iterpool)

        self.iterpool.clear()

        return result_rev

    def _info_wrapper(self, baton, path, info, pool):
        """Internal wrapper for info method callbacks."""
        if self._info:
            pool = Pool()
            info = svn_info_dup(info, pool)[0]
            info.pool = pool
            self._info(path, info)

    def set_info_func(self, info):
        """Set a callback to be used to process svn_info_t objects during
        calls to the info method.

        The callback function should accept a path and a svn_info_t object as
        arguments.

        Keyword arguments:
        info -- callback to be used to process information during a call to
            the info method."""
        self._info = info

    def info(self, path="", recurse=True, info_func=None):
        """Get info about path.

        Keyword arguments:
        path -- path to get info about (defaults to WC root)
        recurse -- if True, directories will be recursed
        info_func -- callback function to use (defaults to previously
            registered callback)"""

        if info_func:
            self.set_info_func(info_func)

        svn_client_info(self._build_path(path), NULL, NULL, self._info_func,
                        c_void_p(), recurse, self.client,
                        self.iterpool)

        self.iterpool.clear()

    def checkout(self, url, revnum=None, path=None, recurse=True,
                 ignore_externals=False):
        """Checkout a new working copy.

        Keyword arguments:
        url -- url to check out from, should be of the form url@peg_revision
        revnum -- revision number to check out
        path -- location to checkout to (defaults to WC root)
        ignore_externals -- if True, externals will not be included in checkout
            (default False)"""
        rev = svn_opt_revision_t()
        if revnum is not None:
            rev.kind = svn_opt_revision_number
            rev.value.number = revnum
        else:
            rev.kind = svn_opt_revision_head

        peg_rev = svn_opt_revision_t()
        URL = String("")
        svn_opt_parse_path(byref(peg_rev), byref(URL), url, self.iterpool)

        if not path:
            path = self.path
        canon_path = svn_path_canonicalize(path, self.iterpool)

        result_rev = svn_revnum_t()

        svn_client_checkout2(byref(result_rev), URL, canon_path,
                             byref(peg_rev), byref(rev), recurse,
                             ignore_externals, self.client, self.iterpool)
        self.iterpool.clear()

    def set_log_func(self, log_func):
        """Register a callback to get a log message for commit and commit-like
        operations.

        LOG_FUNC should take an array as an argument, which holds the files to
        be committed. It should return a list of the form [LOG, FILE] where LOG
        is a log message and FILE is the temporary file, if one was created
        instead of a log message. If LOG is None, the operation will be
        canceled and FILE will be treated as the temporary file holding the
        temporary commit message.

        Keyword arguments:
        log_func -- callback to be used for getting log messages"""
        self._log_func = log_func

    def _log_func_wrapper(self, log_msg, tmp_file, commit_items, baton, pool):
        """Internal wrapper for log function callback."""
        if self._log_func:
            [log, file] = self._log_func(_types.Array(String, commit_items))

            if log:
                log_msg = pointer(c_char_p(log))
            else:
                log_msg = NULL

            if file:
                tmp_file = pointer(c_char_p(file))
            else:
                tmp_file = NULL

    def commit(self, paths=[""], recurse=True, keep_locks=False):
        """Commit changes in the working copy.

        Keyword arguments:
        paths -- list of paths that should be committed (defaults to WC root)
        recurse -- if True, the contents of directories to be committed will
            also be committed (default True)
        keep_locks -- if True, locks will not be released during commit
            (default False)"""
        commit_info = POINTER(svn_commit_info_t)()
        pool = Pool()
        svn_client_commit3(byref(commit_info), self._build_path_list(paths),
                           recurse, keep_locks, self.client, pool)
        commit_info[0].pool = pool
        return commit_info[0]

    def update(self, paths=[""], revnum=None, recurse=True,
                ignore_externals=True):
        """Update paths to a given revision number.

        Returns an array of revision numbers to which the revision number was
        resolved.

        Keyword arguments:
        paths -- list of path to be updated (defaults to WC root)
        revnum -- revision number to update to (defaults to head revision)
        recurse -- if True, the contents of directories will also be updated
                   (default True)
        ignore_externals -- if True, externals will not be updated (default
                            True)"""

        rev = svn_opt_revision_t()
        if revnum is not None:
            rev.kind = svn_opt_revision_number
            rev.value.number = revnum
        else:
            rev.kind = svn_opt_revision_head

        result_revs = _types.Array(svn_revnum_t, svn_revnum_t())

        svn_client_update2(byref(result_revs.header),
                self._build_path_list(paths), byref(rev), recurse,
                ignore_externals, self.client, self.iterpool)

        self.iterpool.clear()
        return result_revs

    #internal method to wrap ls callbacks
    def _list_wrapper(self, baton, path, dirent, abs_path, pool):
        """Internal method to wrap list callback."""
        if self._list:
            pool = Pool()
            dirent = svn_dirent_dup(dirent, pool)[0]
            lock = svn_lock_dup(lock, pool)[0]
            dirent.pool = pool
            lock.pool = pool
            self._list(path, dirent, lock, abs_path)

    def set_list_func(self, list_func):
        """Set the callback function for list operations.

        Keyword arguments:
        list_func -- function to be used for callbacks"""
        self._list = list_func

    def list(self, path="", recurse=True, fetch_locks=False, list_func=None):
        """List items in the working copy using a callback.

        Keyword arguments:
        path -- path to list (defaults to WC root)
        recurse -- if True, list contents of directories as well (default True)
        fetch_locks -- if True, get information from the repository about locks
            (default False)
        list_func -- callback to be used (defaults to previously registered
            callback)"""
        peg = svn_opt_revision_t()
        peg.kind = svn_opt_revision_working

        rev = svn_opt_revision_t()
        rev.kind = svn_opt_revision_working

        if list_func:
            self.set_list_func(list_func)

        svn_client_list(self._build_path(path), byref(peg), byref(rev),
            recurse, SVN_DIRENT_ALL, fetch_locks, self._list_func,
            c_void_p(), self.client, self.iterpool)

        self.iterpool.clear()

    def relocate(self, from_url, to_url, dir="", recurse=True):
        """Modify a working copy directory, changing repository URLs that begin
        with FROM_URL to begin with TO_URL instead, recursing into
        subdirectories if RECURSE is True (True by default).

        Keyword arguments:
        from_url -- url to be replaced, if this url is matched at the beginning
            of a url it will be replaced with to_url
        to_url -- url to replace from_url
        dir -- directory to relocate (defaults to WC root)
        recurse -- if True, directories will be recursed (default True)"""

        svn_client_relocate(self._build_path(dir), from_url, to_url, recurse,
                            self.client, self.iterpool)

        self.iterpool.clear()

    def switch(self, path, url, revnum=None, recurse=True):
        """Switch part of a working copy to a new url.

        Keyword arguments:
        path -- path to be changed to a new url
        url -- url to be used
        revnum -- revision to be switched to (defaults to head revision)
        recurse -- if True, the operation will recurse (default True)"""
        result_rev = svn_revnum_t()

        revision = svn_opt_revision_t()
        if revnum:
            revision.kind = svn_opt_revision_number
            revision.value.number = revnum
        else:
            revision.kind = svn_opt_revision_head

        svn_client_switch(byref(result_rev),
                  self._build_path(path), url, byref(revision),
                  recurse, self.client, self.iterpool)
        self.iterpool.clear()

    def lock(self, paths, comment=NULL, steal_lock=False):
        """Lock items.

        Keyword arguments:
        paths -- list of paths to be locked, may be WC paths (in which
            case this is a local operation) or urls (in which case this is a
            network operation)
        comment -- comment for the lock (default no comment)
        steal_lock -- if True, the lock will be created even if the file is
            already locked (default False)"""
        targets = self._build_path_list(paths)
        svn_client_lock(targets, comment, steal_lock, self.client, self.iterpool)
        self.iterpool.clear()

    def unlock(self, paths, break_lock=False):
        """Unlock items.

        Keyword arguments:
        paths - list of paths to be unlocked, may be WC paths (in which
            case this is a local operation) or urls (in which case this is a
            network operation)
        break_lock -- if True, locks will be broken (default False)"""
        targets = self._build_path_list(paths)
        svn_client_unlock(targets, break_lock, self.client, self.iterpool)
        self.iterpool.clear()

    def merge(self, source1, revnum1, source2, revnum2, target_wcpath,
                recurse=True, ignore_ancestry=False, force=False,
                dry_run=False, merge_options=[]):
        """Merge changes.

        This method merges the changes from source1@revnum1 to
        source2@revnum2 into target_wcpath.

        Keyword arguments:
        source1 -- path for beginning revision of changes to be merged
        revnum1 -- starting revnum
        source2 -- path for ending revision of changes to merge
        revnum2 -- ending revnum
        target_wcpath -- path to apply changes to
        recurse -- if True, apply changes recursively
        ignore_ancestry -- if True, relatedness will not be checked (False by
            default)
        force -- if False and the merge involves deleting locally modified
            files the merge will fail (default False)
        dry_run -- if True, notification will be provided but no local files
            will be modified (default False)
        merge_options -- a list of options to be passed to the diff process
            (default no options)"""
        revision1 = svn_opt_revision_t()
        revision1.kind = svn_opt_revision_number
        revision1.value.number = revnum1

        revision2 = svn_opt_revision_t()
        revision2.kind = svn_opt_revision_number
        revision2.value.number = revnum2

        merge_options = _types.Array(c_char_p, merge_options)

        svn_client_merge2(source1, byref(revision1), source2, byref(revision2),
            target_wcpath, recurse, ignore_ancestry, force, dry_run,
            merge_options.header, self.client, self.iterpool)

        self.iterpool.clear()
