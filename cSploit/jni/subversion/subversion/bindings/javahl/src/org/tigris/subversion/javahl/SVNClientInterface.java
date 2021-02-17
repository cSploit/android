/**
 * @copyright
 * ====================================================================
 *    Licensed to the Apache Software Foundation (ASF) under one
 *    or more contributor license agreements.  See the NOTICE file
 *    distributed with this work for additional information
 *    regarding copyright ownership.  The ASF licenses this file
 *    to you under the Apache License, Version 2.0 (the
 *    "License"); you may not use this file except in compliance
 *    with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *    Unless required by applicable law or agreed to in writing,
 *    software distributed under the License is distributed on an
 *    "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *    KIND, either express or implied.  See the License for the
 *    specific language governing permissions and limitations
 *    under the License.
 * ====================================================================
 * @endcopyright
 */

package org.tigris.subversion.javahl;

import java.io.OutputStream;
import java.util.Map;

/**
 * This interface is the commom interface for all subversion
 * operations. It is implemented by SVNClient and SVNClientSynchronized
 */
public interface SVNClientInterface
{
    /**
     * release the native peer (should not depend on finalize)
     */
    void dispose();

    /**
     * @return Version information about the underlying native libraries.
     * @since 1.0
     */
    public Version getVersion();

    /**
     * @return The name of the working copy's administrative
     * directory, which is usually <code>.svn</code>.
     * @see <a
     * href="http://svn.apache.org/repos/asf/subversion/trunk/notes/asp-dot-net-hack.txt">
     * Instructions on changing this as a work-around for the behavior of
     * ASP.Net on Windows.</a>
     * @since 1.3
     */
    public String getAdminDirectoryName();

    /**
     * @param name The name of the directory to compare.
     * @return Whether <code>name</code> is that of a working copy
     * administrative directory.
     * @since 1.3
     */
    public boolean isAdminDirectory(String name);

    /**
     * Returns the last destination path submitted.
     * @deprecated
     * @return path in Subversion format.
     * @since 1.0
     */
    String getLastPath();

    /**
     * Returns the status of a single file in the path.
     *
     * @param path      File to gather status.
     * @param onServer  Request status information from the server.
     * @return The Subversion status of the file, or <code>null</code>
     * if no status is available.
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    Status singleStatus(String path, boolean onServer) throws ClientException;

    /**
     * List a directory or file of the working copy.
     *
     * @param path      Path to explore.
     * @param descend   Recurse into subdirectories if they exist.
     * @param onServer  Request status information from server.
     * @param getAll    get status for uninteresting (unchanged) files.
     * @return Array of Status entries.
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    Status[] status(String path, boolean descend, boolean onServer,
                    boolean getAll) throws ClientException;

    /**
     * List a directory or file of the working copy.
     *
     * @param path      Path to explore.
     * @param descend   Recurse into subdirectories if they exist.
     * @param onServer  Request status information from server.
     * @param getAll    get status for uninteresting (unchanged) files.
     * @param noIgnore  get status for normaly ignored files and directories.
     * @return Array of Status entries.
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    Status[] status(String path, boolean descend, boolean onServer,
                    boolean getAll, boolean noIgnore) throws ClientException;

    /**
     * List a directory or file of the working copy.
     *
     * @param path      Path to explore.
     * @param descend   Recurse into subdirectories if they exist.
     * @param onServer  Request status information from server.
     * @param getAll    get status for uninteresting (unchanged) files.
     * @param noIgnore  get status for normaly ignored files and directories.
     * @param ignoreExternals if externals are ignored during status
     * @return Array of Status entries.
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.2
     */
    Status[] status(String path, boolean descend, boolean onServer,
                    boolean getAll, boolean noIgnore, boolean ignoreExternals)
            throws ClientException;

    /**
     * List a directory or file of the working copy.
     *
     * @param path        Path to explore.
     * @param depth       How deep to recurse into subdirectories.
     * @param onServer    Request status information from server.
     * @param getAll      get status for uninteresting (unchanged) files.
     * @param noIgnore    get status for normaly ignored files and directories.
     * @param ignoreExternals if externals are ignored during status
     * @param changelists changelists to filter by
     * @since 1.5
     */
    void status(String path, int depth, boolean onServer,
                boolean getAll, boolean noIgnore, boolean ignoreExternals,
                String[] changelists, StatusCallback callback)
            throws ClientException;

    /**
     * Lists the directory entries of an url on the server.
     * @param url       the url to list
     * @param revision  the revision to list
     * @param recurse   recurse into subdirectories
     * @return  Array of DirEntry objects.
     * @deprecated Use {@link #list(String, Revision, Revision, int, int,
     *                              boolean, ListCallback)} instead.
     * @since 1.0
     */
    DirEntry[] list(String url, Revision revision, boolean recurse)
            throws ClientException;

    /**
     * Lists the directory entries of an url on the server.
     * @param url       the url to list
     * @param revision  the revision to list
     * @param pegRevision the revision to interpret url
     * @param recurse   recurse into subdirectories
     * @return  Array of DirEntry objects.
     * @deprecated Use {@link #list(String, Revision, Revision, int, int,
     *                              boolean, ListCallback)} instead.
     * @since 1.2
     */
    DirEntry[] list(String url, Revision revision, Revision pegRevision,
                    boolean recurse)
            throws ClientException;

    /**
     * Lists the directory entries of an url on the server.
     * @param url             the url to list
     * @param revision        the revision to list
     * @param pegRevision     the revision to interpret url
     * @param depth           the depth to recurse into subdirectories
     * @param direntFields    the fields to retrieve
     * @param fetchLocks      whether to fetch lock information
     * @param callback        the callback to receive the directory entries
     * @since 1.5
     */
    void list(String url, Revision revision, Revision pegRevision,
              int depth, int direntFields, boolean fetchLocks,
              ListCallback callback)
            throws ClientException;

    /**
     * Sets the username used for authentication.
     * @param username The username, ignored if the empty string.  Set
     * to the empty string to clear it.
     * @throws IllegalArgumentException If <code>username</code> is
     * <code>null</code>.
     * @see #password(String)
     * @since 1.0
     */
    void username(String username);

    /**
     * Sets the password used for authentication.
     * @param password The password, ignored if the empty string.  Set
     * to the empty string to clear it.
     * @throws IllegalArgumentException If <code>password</code> is
     * <code>null</code>.
     * @see #username(String)
     * @since 1.0
     */
    void password(String password);

    /**
     * Register callback interface to supply username and password on demand.
     * This callback can also be used to provide theequivalent of the
     * <code>--no-auth-cache</code> and <code>--non-interactive</code> arguments
     * accepted by the command-line client.
     * @param prompt the callback interface
     * @since 1.0
     */
    void setPrompt(PromptUserPassword prompt);

    /**
     * Retrieve the log messages for an item
     * @param path          path or url to get the log message for.
     * @param revisionStart first revision to show
     * @param revisionEnd   last revision to show
     * @return array of LogMessages
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    LogMessage[] logMessages(String path, Revision revisionStart,
                             Revision revisionEnd) throws ClientException;

    /**
     * Retrieve the log messages for an item
     * @param path          path or url to get the log message for.
     * @param revisionStart first revision to show
     * @param revisionEnd   last revision to show
     * @param stopOnCopy    do not continue on copy operations
     * @return array of LogMessages
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    LogMessage[] logMessages(String path, Revision revisionStart,
                             Revision revisionEnd, boolean stopOnCopy)
            throws ClientException;

    /**
     * Retrieve the log messages for an item
     * @param path          path or url to get the log message for.
     * @param revisionStart first revision to show
     * @param revisionEnd   last revision to show
     * @param stopOnCopy    do not continue on copy operations
     * @param discoverPath  returns the paths of the changed items in the
     *                      returned objects
     * @return array of LogMessages
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    LogMessage[] logMessages(String path, Revision revisionStart,
                             Revision revisionEnd, boolean stopOnCopy,
                             boolean discoverPath)
            throws ClientException;

    /**
     * Retrieve the log messages for an item
     * @param path          path or url to get the log message for.
     * @param revisionStart first revision to show
     * @param revisionEnd   last revision to show
     * @param stopOnCopy    do not continue on copy operations
     * @param discoverPath  returns the paths of the changed items in the
     *                      returned objects
     * @param limit         limit the number of log messages (if 0 or less no
     *                      limit)
     * @return array of LogMessages
     * @since 1.2
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     */
    LogMessage[] logMessages(String path, Revision revisionStart,
                             Revision revisionEnd, boolean stopOnCopy,
                             boolean discoverPath, long limit)
            throws ClientException;

    /**
     * Retrieve the log messages for an item.
     * @param path          path or url to get the log message for.
     * @param pegRevision   revision to interpret path
     * @param revisionStart first revision to show
     * @param revisionEnd   last revision to show
     * @param stopOnCopy    do not continue on copy operations
     * @param discoverPath  returns the paths of the changed items in the
     *                      returned objects
     * @param includeMergedRevisions include log messages for revisions which
     *                               were merged.
     * @param revProps      the revprops to retrieve
     * @param limit         limit the number of log messages (if 0 or less no
     *                      limit)
     * @param callback      the object to receive the log messages
     * @since 1.5
     * @deprecated Use {@link #logMessages(String, Revision, RevisionRange[],
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     */
    void logMessages(String path, Revision pegRevision,
                     Revision revisionStart,
                     Revision revisionEnd, boolean stopOnCopy,
                     boolean discoverPath, boolean includeMergedRevisions,
                     String[] revProps, long limit,
                     LogMessageCallback callback)
            throws ClientException;

    /**
     * Retrieve the log messages for an item.
     * @param path          path or url to get the log message for.
     * @param pegRevision   revision to interpret path
     * @param ranges        an array of revision ranges to show
     * @param stopOnCopy    do not continue on copy operations
     * @param discoverPath  returns the paths of the changed items in the
     *                      returned objects
     * @param includeMergedRevisions include log messages for revisions which
     *                               were merged.
     * @param revProps      the revprops to retrieve
     * @param limit         limit the number of log messages (if 0 or less no
     *                      limit)
     * @param callback      the object to receive the log messages
     * @since 1.6
     */
    void logMessages(String path, Revision pegRevision,
                     RevisionRange[] ranges, boolean stopOnCopy,
                     boolean discoverPath, boolean includeMergedRevisions,
                     String[] revProps, long limit,
                     LogMessageCallback callback)
            throws ClientException;

    /**
     * Executes a revision checkout.
     * @param moduleName name of the module to checkout.
     * @param destPath destination directory for checkout.
     * @param revision the revision to checkout.
     * @param recurse whether you want it to checkout files recursively.
     * @throws ClientException
     * @deprecated Use {@link #checkout(String, String, Revision, Revision,
     *                                  int, boolean, boolean)} instead.
     * @since 1.0
     */
    long checkout(String moduleName, String destPath, Revision revision,
                  boolean recurse)
            throws ClientException;

    /**
     * Executes a revision checkout.
     * @param moduleName name of the module to checkout.
     * @param destPath destination directory for checkout.
     * @param revision the revision to checkout.
     * @param pegRevision the peg revision to interpret the path
     * @param recurse whether you want it to checkout files recursively.
     * @param ignoreExternals if externals are ignored during checkout
     * @throws ClientException
     * @deprecated Use {@link #checkout(String, String, Revision, Revision,
     *                                  int, boolean, boolean)} instead.
     * @since 1.2
     */
    long checkout(String moduleName, String destPath, Revision revision,
                  Revision pegRevision, boolean recurse,
                  boolean ignoreExternals) throws ClientException;

    /**
     * Executes a revision checkout.
     * @param moduleName name of the module to checkout.
     * @param destPath destination directory for checkout.
     * @param revision the revision to checkout.
     * @param pegRevision the peg revision to interpret the path
     * @param depth how deep to checkout files recursively.
     * @param ignoreExternals if externals are ignored during checkout
     * @param allowUnverObstructions allow unversioned paths that obstruct adds
     * @throws ClientException
     * @since 1.5
     */
    long checkout(String moduleName, String destPath, Revision revision,
                  Revision pegRevision, int depth,
                  boolean ignoreExternals,
                  boolean allowUnverObstructions) throws ClientException;

    /**
     * Sets the notification callback used to send processing information back
     * to the calling program.
     * @param notify listener that the SVN library should call on many
     *               file operations.
     * @deprecated Use {@link #notification2(Notify2)} instead.
     * @since 1.0
     */
    void notification(Notify notify);

    /**
     * Sets the notification callback used to send processing information back
     * to the calling program.
     * @param notify listener that the SVN library should call on many
     *               file operations.
     * @since 1.2
     */
    void notification2(Notify2 notify);

    /**
     * Set the conflict resolution callback.
     *
     * @param listener The conflict resolution callback.
     * @since 1.5
     */
    void setConflictResolver(ConflictResolverCallback listener);

    /**
     * Set the progress callback.
     *
     * @param listener The progress callback.
     * @since 1.5
     */
    void setProgressListener(ProgressListener listener);

    /**
     * Sets the commit message handler. This allows more complex commit message
     * with the list of the elements to be committed as input.
     * @param messageHandler    callback for entering commit messages
     *                          if this is set the message parameter is ignored.
     * @since 1.0
     */
    void commitMessageHandler(CommitMessage messageHandler);

    /**
     * Sets a file for deletion.
     * @param path      path or url to be deleted
     * @param message   if path is a url, this will be the commit message.
     * @param force     delete even when there are local modifications.
     * @throws ClientException
     * @deprecated Use {@link #remove(String[], String, boolean, boolean)}
     *             instead.
     * @since 1.0
     */
    void remove(String[] path, String message, boolean force)
            throws ClientException;

    /**
     * Sets a file for deletion.
     * @param path      path or url to be deleted
     * @param message   if path is a url, this will be the commit message.
     * @param force     delete even when there are local modifications.
     * @param keepLocal only remove the paths from the repository.
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException
     * @since 1.5
     */
    void remove(String[] path, String message, boolean force,
                boolean keepLocal, Map revpropTable)
            throws ClientException;

    /**
     * Reverts a file to a pristine state.
     * @param path      path of the file.
     * @param recurse   recurse into subdirectories
     * @throws ClientException
     * @deprecated Use {@link #revert(String, int)} instead.
     * @since 1.0
     */
    void revert(String path, boolean recurse) throws ClientException;

    /**
     * Reverts a file to a pristine state.
     * @param path      path of the file.
     * @param depth     the depth to recurse into subdirectories
     * @param changelists changelists to filter by
     * @throws ClientException
     * @since 1.5
     */
    void revert(String path, int depth, String[] changelists)
            throws ClientException;

    /**
     * Adds a file to the repository.
     * @param path      path to be added.
     * @param recurse   recurse into subdirectories
     * @throws ClientException
     * @since 1.0
     * @deprecated Use {@link #add(String, int, boolean, boolean, boolean)}
     *             instead.
     */
    void add(String path, boolean recurse) throws ClientException;

    /**
     * Adds a file to the repository.
     * @param path      path to be added.
     * @param recurse   recurse into subdirectories
     * @param force     if adding a directory and recurse true and path is a
     *                  directory, all not already managed files are added.
     * @throws ClientException
     * @since 1.2
     * @deprecated Use {@link #add(String, int, boolean, boolean, boolean)}
     *             instead.
     */
    void add(String path, boolean recurse, boolean force)
        throws ClientException;

    /**
     * Adds a file to the repository.
     * @param path      path to be added.
     * @param depth     the depth to recurse into subdirectories
     * @param force     if adding a directory and recurse true and path is a
     *                  directory, all not already managed files are added.
     * @param noIgnores if false, don't add files or directories matching
     *                  ignore patterns
     * @param addParents add any intermediate parents to the working copy
     * @throws ClientException
     * @since 1.5
     */
    void add(String path, int depth, boolean force, boolean noIgnores,
             boolean addParents)
        throws ClientException;

    /**
     * Updates the directory or file from repository
     * @param path target file.
     * @param revision the revision number to update.
     *                 Revision.HEAD will update to the
     *                 latest revision.
     * @param recurse recursively update.
     * @throws ClientException
     * @deprecated Use {@link #update(String, Revision, int, boolean,
     *                                boolean, boolean)} instead.
     * @since 1.0
     */
    long update(String path, Revision revision, boolean recurse)
            throws ClientException;

    /**
     * Updates the directories or files from repository
     * @param path array of target files.
     * @param revision the revision number to update.
     *                 Revision.HEAD will update to the
     *                 latest revision.
     * @param recurse recursively update.
     * @param ignoreExternals if externals are ignored during update
     * @throws ClientException
     * @deprecated Use {@link #update(String[], Revision, int, boolean,
     *                                boolean, boolean)} instead.
     * @since 1.2
     */
    long[] update(String[] path, Revision revision, boolean recurse,
                  boolean ignoreExternals) throws ClientException;

    /**
     * Updates the directory or file from repository
     * @param path target file.
     * @param revision the revision number to update.
     *                 Revision.HEAD will update to the
     *                 latest revision.
     * @param depth  the depth to recursively update.
     * @param depthIsSticky if set, and depth is not {@link Depth#unknown},
     *                      then also set the ambient depth value to depth.
     * @param ignoreExternals if externals are ignored during update
     * @param allowUnverObstructions allow unversioned paths that obstruct adds
     * @throws ClientException
     * @since 1.5
     */
    long update(String path, Revision revision, int depth,
                boolean depthIsSticky, boolean ignoreExternals,
                boolean allowUnverObstructions)
            throws ClientException;

    /**
     * Updates the directories or files from repository
     * @param path array of target files.
     * @param revision the revision number to update.
     *                 Revision.HEAD will update to the
     *                 latest revision.
     * @param depth  the depth to recursively update.
     * @param depthIsSticky if set, and depth is not {@link Depth#unknown},
     *                      then also set the ambient depth value to depth.
     * @param ignoreExternals if externals are ignored during update
     * @param allowUnverObstructions allow unversioned paths that obstruct adds
     * @throws ClientException
     * @since 1.5
     */
    long[] update(String[] path, Revision revision, int depth,
                  boolean depthIsSticky, boolean ignoreExternals,
                  boolean allowUnverObstructions) throws ClientException;

    /**
     * Commits changes to the repository.
     * @param path      files to commit.
     * @param message   log message.
     * @param recurse   whether the operation should be done recursively.
     * @return The new revision number created by the commit, or
     * {@link Revision#SVN_INVALID_REVNUM} if the revision number is
     * invalid.
     * @throws ClientException
     * @deprecated Use {@link #commit(String[], String, int, boolean, boolean,
     *                                String[])} instead.
     */
    long commit(String[] path, String message, boolean recurse)
            throws ClientException;

    /**
     * Commits changes to the repository.
     * @param path      files to commit.
     * @param message   log message.
     * @param recurse   whether the operation should be done recursively.
     * @param noUnlock  do remove any locks
     * @return The new revision number created by the commit, or
     * {@link Revision#SVN_INVALID_REVNUM} if the revision number is
     * invalid.
     * @throws ClientException
     * @deprecated Use {@link #commit(String[], String, int, boolean, boolean,
     *                                String[])} instead.
     * @since 1.2
     */
    long commit(String[] path, String message, boolean recurse,
                boolean noUnlock) throws ClientException;

    /**
     * Commits changes to the repository.
     * @param path            files to commit.
     * @param message         log message.
     * @param depth           how deep to recurse in subdirectories
     * @param noUnlock        do remove any locks
     * @param keepChangelist  keep changelist associations after the commit.
     * @param changelists  if non-null, filter paths using changelists
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @return The new revision number created by the commit, or
     * {@link Revision#SVN_INVALID_REVNUM} if the revision number is
     * invalid.
     * @throws ClientException
     * @since 1.5
     */
    long commit(String[] path, String message, int depth,
                boolean noUnlock, boolean keepChangelist,
                String[] changelists, Map revpropTable)
            throws ClientException;

    /**
     * Copy versioned paths with the history preserved.
     *
     * @param sources A list of <code>CopySource</code> objects.
     * @param destPath Destination path or URL.
     * @param message Commit message.  May be <code>null</code> if
     * <code>destPath</code> is not a URL.
     * @param copyAsChild Whether to copy <code>srcPaths</code> as
     * children of <code>destPath</code>.
     * @param makeParents Whether to create intermediate parents
     * @param ignoreExternals Whether or not to process external definitions
     *                        as part of this operation.
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException If the copy operation fails.
     * @since 1.7
     */
    void copy(CopySource[] sources, String destPath, String message,
              boolean copyAsChild, boolean makeParents,
              boolean ignoreExternals, Map revpropTable)
            throws ClientException;

    /**
     * Copy versioned paths with the history preserved.
     *
     * @param sources A list of <code>CopySource</code> objects.
     * @param destPath Destination path or URL.
     * @param message Commit message.  May be <code>null</code> if
     * <code>destPath</code> is not a URL.
     * @param copyAsChild Whether to copy <code>srcPaths</code> as
     * children of <code>destPath</code>.
     * @param makeParents Whether to create intermediate parents
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException If the copy operation fails.
     * @since 1.5
     * @deprecated Use {@link #copy(CopySource[], String, String, boolean,
     *                              boolean, boolean, Map)} instead.
     */
    void copy(CopySource[] sources, String destPath, String message,
              boolean copyAsChild, boolean makeParents, Map revpropTable)
        throws ClientException;

    /**
     * Copy versioned paths with the history preserved (with
     * <code>copyAsChild</code> behavior).
     *
     * @param srcPath   source path or url
     * @param destPath  destination path or url
     * @param message   commit message if destPath is an url
     * @param revision  source revision
     * @throws ClientException
     * @deprecated Use {@link #copy(CopySource[], String, String, boolean,
     *                              boolean, boolean, Map)} instead.
     */
    void copy(String srcPath, String destPath, String message,
              Revision revision) throws ClientException;

    /**
     * Move or rename versioned paths.
     *
     * @param srcPaths Source paths or URLs.
     * @param destPath Destination path or URL.
     * @param message Commit message.  May be <code>null</code> if
     * <code>destPath</code> is not a URL.
     * @param force Whether to perform the move even if local
     * modifications exist.
     * @param moveAsChild Whether to move <code>srcPaths</code> as
     * children of <code>destPath</code>.
     * @param makeParents Whether to create intermediate parents.
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException If the move operation fails.
     * @since 1.5
     */
    void move(String[] srcPaths, String destPath, String message,
              boolean force, boolean moveAsChild, boolean makeParents,
              Map revpropTable)
        throws ClientException;

    /**
     * @deprecated Use {@link #move(String[], String, String, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    void move(String srcPath, String destPath, String message,
              Revision ignored, boolean force)
        throws ClientException;

    /**
     * Move or rename versioned paths (with <code>moveAsChild</code>
     * behavior).
     *
     * @param srcPath   source path or url
     * @param destPath  destination path or url
     * @param message   commit message if destPath is an url
     * @param force     even with local modifications.
     * @throws ClientException
     * @deprecated Use {@link #move(String[], String, String, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    void move(String srcPath, String destPath, String message,
              boolean force) throws ClientException;

    /**
     * Creates a directory directly in a repository or creates a
     * directory on disk and schedules it for addition.
     * @param path      directories to be created
     * @param message   commit message to used if path contains urls
     * @param makeParents Whether to create intermediate parents
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException
     * @since 1.5
     */
    void mkdir(String[] path, String message, boolean makeParents,
               Map revpropTable)
            throws ClientException;

    /**
     * Creates a directory directly in a repository or creates a
     * directory on disk and schedules it for addition.
     * @param path      directories to be created
     * @param message   commit message to used if path contains urls
     * @throws ClientException
     */
    void mkdir(String[] path, String message) throws ClientException;

    /**
     * Recursively cleans up a local directory, finishing any
     * incomplete operations, removing lockfiles, etc.
     * @param path a local directory.
     * @throws ClientException
     */
    void cleanup(String path) throws ClientException;

    /**
     * Resolves the <i>conflicted</i> state on a WC path (or tree).
     * @param path The path to resolve.
     * @param depth How deep to recurse into child paths.
     * @param conflictResult Which version to choose in the event of a
     *                       conflict.
     * @throws SubversionException If an error occurs.
     * @since 1.5
     */
    void resolve(String path, int depth, int conflictResult)
        throws SubversionException;

    /**
     * Removes the <i>conflicted</i> state on a WC path (or tree).
     * @see #resolved(String, int, int)
     * @deprecated Use {@link #resolved(String, int, int)} instead.
     */
    void resolved(String path, boolean recurse) throws ClientException;

    /**
     * Exports the contents of either a subversion repository into a
     * 'clean' directory (meaning a directory with no administrative
     * directories).
     * @param srcPath   the url of the repository path to be exported
     * @param destPath  a destination path that must not already exist.
     * @param revision  the revsion to be exported
     * @param force     set if it is ok to overwrite local files
     * @throws ClientException
     * @deprecated Use {@link #doExport(String, String, Revision, Revision,
     *                                  boolean, boolean, int, String)} instead.
     * @since 1.0
     */
    long doExport(String srcPath, String destPath, Revision revision,
                  boolean force) throws ClientException;

    /**
     * Exports the contents of either a subversion repository into a
     * 'clean' directory (meaning a directory with no administrative
     * directories).
     * @param srcPath   the url of the repository path to be exported
     * @param destPath  a destination path that must not already exist.
     * @param revision  the revsion to be exported
     * @param pegRevision the revision to interpret srcPath
     * @param force     set if it is ok to overwrite local files
     * @param ignoreExternals ignore external during export
     * @param recurse   recurse to subdirectories
     * @param nativeEOL which EOL characters to use during export
     * @throws ClientException
     * @deprecated Use {@link #doExport(String, String, Revision, Revision,
     *                                  boolean, boolean, int, String)} instead.
     * @since 1.2
     */
    long doExport(String srcPath, String destPath, Revision revision,
                  Revision pegRevision, boolean force, boolean ignoreExternals,
                  boolean recurse, String nativeEOL) throws ClientException;

    /**
     * Exports the contents of either a subversion repository into a
     * 'clean' directory (meaning a directory with no administrative
     * directories).
     *
     * @param srcPath         the url of the repository path to be exported
     * @param destPath        a destination path that must not already exist.
     * @param revision        the revsion to be exported
     * @param pegRevision     the revision to interpret srcPath
     * @param force           set if it is ok to overwrite local files
     * @param ignoreExternals ignore external during export
     * @param depth           how deep to recurse in subdirectories
     * @param nativeEOL       which EOL characters to use during export
     * @throws ClientException
     * @since 1.5
     */
    long doExport(String srcPath, String destPath, Revision revision,
                  Revision pegRevision, boolean force, boolean ignoreExternals,
                  int depth, String nativeEOL)
            throws ClientException;

    /**
     * Update local copy to mirror a new url.
     * @param path      the working copy path
     * @param url       the new url for the working copy
     * @param revision  the new base revision of working copy
     * @param recurse   traverse into subdirectories
     * @throws ClientException
     * @deprecated Use {@link #doSwitch(String, String, Revision, boolean)}
     *             instead.
     * @since 1.0
     */
    long doSwitch(String path, String url, Revision revision, boolean recurse)
            throws ClientException;

    /**
     * Update local copy to mirror a new url.
     * @param path      the working copy path
     * @param url       the new url for the working copy
     * @param revision  the new base revision of working copy
     * @param pegRevision the revision at which to interpret <code>path</code>
     * @param depth     how deep to traverse into subdirectories
     * @param depthIsSticky if set, and depth is not {@link Depth#unknown},
     *                      then also set the ambient depth value to depth.
     * @param ignoreExternals whether to process externals definitions
     * @param allowUnverObstructions allow unversioned paths that obstruct adds
     * @throws ClientException
     * @since 1.5
     */
    long doSwitch(String path, String url, Revision revision,
                  Revision pegRevision, int depth, boolean depthIsSticky,
                  boolean ignoreExternals, boolean allowUnverObstructions)
            throws ClientException;

    /**
     * Import a file or directory into a repository directory  at
     * head.
     * @param path      the local path
     * @param url       the target url
     * @param message   the log message.
     * @param recurse   traverse into subdirectories
     * @throws ClientException
     * @deprecated Use {@link #doImport(String, String, String, int, boolean,
     *                                  boolean)} instead.
     * @since 1.0
     */
    void doImport(String path, String url, String message, boolean recurse)
            throws ClientException;

    /**
     * Import a file or directory into a repository directory  at
     * head.
     * @param path      the local path
     * @param url       the target url
     * @param message   the log message.
     * @param depth     depth to traverse into subdirectories
     * @param noIgnore  whether to add files matched by ignore patterns
     * @param ignoreUnknownNodeTypes whether to ignore files which
     *                  the node type is not konwn, just as pipes
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException
     *
     * @since 1.5
     */
    void doImport(String path, String url, String message, int depth,
                  boolean noIgnore, boolean ignoreUnknownNodeTypes,
                  Map revpropTable)
            throws ClientException;

    /**
     * Return an ordered list of suggested merge source URLs.
     * @param path The merge target path for which to suggest sources.
     * @param pegRevision Peg revision used to interpret path.
     * @return The list of URLs, empty if there are no suggestions.
     * @throws ClientException If an error occurs.
     * @since 1.5
     */
    String[] suggestMergeSources(String path, Revision pegRevision)
            throws SubversionException;

    /**
     * Merge changes from two paths into a new local path.
     * @param path1         first path or url
     * @param revision1     first revision
     * @param path2         second path or url
     * @param revision2     second revision
     * @param localPath     target local path
     * @param force         overwrite local changes
     * @param recurse       traverse into subdirectories
     * @throws ClientException
     * @deprecated Use {@link #merge(String, Revision, String, Revision,
     *                               String, boolean, int, boolean, boolean,
     *                               boolean)} instead.
     * @since 1.0
     */
    void merge(String path1, Revision revision1, String path2,
               Revision revision2, String localPath, boolean force,
               boolean recurse) throws ClientException;

    /**
     * Merge changes from two paths into a new local path.
     * @param path1         first path or url
     * @param revision1     first revision
     * @param path2         second path or url
     * @param revision2     second revision
     * @param localPath     target local path
     * @param force         overwrite local changes
     * @param recurse       traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param dryRun        do not change anything
     * @throws ClientException
     * @deprecated Use {@link #merge(String, Revision, String, Revision,
     *                               String, boolean, int, boolean, boolean,
     *                               boolean)} instead.
     * @since 1.2
     */
    void merge(String path1, Revision revision1, String path2,
               Revision revision2, String localPath, boolean force,
               boolean recurse, boolean ignoreAncestry, boolean dryRun)
            throws ClientException;

    /**
     * Merge changes from two paths into a new local path.
     *
     * @param path1          first path or url
     * @param revision1      first revision
     * @param path2          second path or url
     * @param revision2      second revision
     * @param localPath      target local path
     * @param force          overwrite local changes
     * @param depth          how deep to traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param dryRun         do not change anything
     * @param recordOnly     record mergeinfo but do not run merge
     * @throws ClientException
     * @since 1.5
     */
    void merge(String path1, Revision revision1, String path2,
               Revision revision2, String localPath, boolean force, int depth,
               boolean ignoreAncestry, boolean dryRun, boolean recordOnly)
            throws ClientException;

    /**
     * Merge changes from two paths into a new local path.
     * @param path          path or url
     * @param pegRevision   revision to interpret path
     * @param revision1     first revision
     * @param revision2     second revision
     * @param localPath     target local path
     * @param force         overwrite local changes
     * @param recurse       traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param dryRun        do not change anything
     * @throws ClientException
     * @deprecated Use {@link #merge(String, Revision, RevisionRange[],
     *                               String, boolean, int, boolean,
     *                               boolean, boolean)} instead.
     * @since 1.2
     */
    void merge(String path, Revision pegRevision, Revision revision1,
               Revision revision2, String localPath, boolean force,
               boolean recurse, boolean ignoreAncestry, boolean dryRun)
            throws ClientException;

    /**
     * Merge set of revisions into a new local path.
     * @param path          path or url
     * @param pegRevision   revision to interpret path
     * @param revisions     revisions to merge
     * @param localPath     target local path
     * @param force         overwrite local changes
     * @param depth         how deep to traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param dryRun        do not change anything
     * @param recordOnly    record mergeinfo but do not run merge
     * @throws ClientException
     * @since 1.5
     */
    void merge(String path, Revision pegRevision, RevisionRange[] revisions,
               String localPath, boolean force, int depth,
               boolean ignoreAncestry, boolean dryRun, boolean recordOnly)
             throws ClientException;

    /**
     * Perform a reintegration merge of path into localPath.
     * localPath must be a single-revision, infinite depth,
     * pristine, unswitched working copy -- in other words, it must
     * reflect a single revision tree, the "target".  The mergeinfo on
     * path must reflect that all of the target has been merged into it.
     * Then this behaves like a merge from the target's URL to the
     * localPath.
     *
     * The depth of the merge is always infinity.
     * @param path          path or url
     * @param pegRevision   revision to interpret path
     * @param localPath     target local path
     * @param dryRun        do not change anything
     * @throws ClientException
     * @since 1.5
     */
    void mergeReintegrate(String path, Revision pegRevision,
                          String localPath, boolean dryRun)
             throws ClientException;

    /**
     * Get mergeinfo for <code>path</code> at <code>pegRevision</code>.
     * @param path WC path or URL.
     * @param pegRevision peg revision at which to get the merge info for
     * <code>path</code>.
     * @return The merge history of <code>path</code>.
     * @throws SubversionException
     * @since 1.5
     */
    Mergeinfo getMergeinfo(String path, Revision pegRevision)
        throws SubversionException;

    /**
     * Retrieve either merged or eligible-to-be-merged revisions.
     * @param kind                   kind of revisions to receive:
     * See {@see org.apache.subversion.javahl.MergeinfoLogKind}.
     * @param pathOrUrl              target of merge
     * @param pegRevision            peg rev for pathOrUrl
     * @param mergeSourceUrl         the source of the merge
     * @param srcPegRevision         peg rev for mergeSourceUrl
     * @param discoverChangedPaths   return paths of changed items
     * @param depth                  the depth to recurse to
     * @param revProps               the revprops to retrieve
     * @param callback               the object to receive the log messages
     * @since 1.7
     */
    void getMergeinfoLog(int kind, String pathOrUrl,
                         Revision pegRevision, String mergeSourceUrl,
                         Revision srcPegRevision, boolean discoverChangedPaths,
                         int depth, String[] revProps,
                         LogMessageCallback callback)
        throws ClientException;

    /**
     * Retrieve either merged or eligible-to-be-merged revisions.
     * @param kind                   kind of revisions to receive:
     * See {@see org.apache.subversion.javahl.MergeinfoLogKind}.
     * @param pathOrUrl              target of merge
     * @param pegRevision            peg rev for pathOrUrl
     * @param mergeSourceUrl         the source of the merge
     * @param srcPegRevision         peg rev for mergeSourceUrl
     * @param discoverChangedPaths   return paths of changed items
     * @param revProps               the revprops to retrieve
     * @param callback               the object to receive the log messages
     * @deprecated Use {@link #getMergeinfoLog(int, String, Revision, String,
     *                                         Revision, boolean, int,
     *                                         String[], LogMessageCallback)}
     * @since 1.5
     */
    void getMergeinfoLog(int kind, String pathOrUrl,
                         Revision pegRevision, String mergeSourceUrl,
                         Revision srcPegRevision, boolean discoverChangedPaths,
                         String[] revProps, LogMessageCallback callback)
        throws ClientException;

    /**
     * Display the differences between two paths
     * @param target1       first path or url
     * @param revision1     first revision
     * @param target2       second path or url
     * @param revision2     second revision
     * @param outFileName   file name where difference are written
     * @param recurse       traverse into subdirectories
     * @throws ClientException
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.0
     */
    void diff(String target1, Revision revision1, String target2,
              Revision revision2, String outFileName, boolean recurse)
            throws ClientException;

    /**
     * Display the differences between two paths
     * @param target1       first path or url
     * @param revision1     first revision
     * @param target2       second path or url
     * @param revision2     second revision
     * @param outFileName   file name where difference are written
     * @param recurse       traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @throws ClientException
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    void diff(String target1, Revision revision1, String target2,
              Revision revision2, String outFileName, boolean recurse,
              boolean ignoreAncestry, boolean noDiffDeleted, boolean force)
            throws ClientException;

    /**
     * Display the differences between two paths
     * @param target1       first path or url
     * @param revision1     first revision
     * @param target2       second path or url
     * @param revision2     second revision
     * @param relativeToDir index path is relative to this path
     * @param outFileName   file name where difference are written
     * @param depth         how deep to traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @param copiesAsAdds  if set, copied files will be shown in their
     *                      entirety, not as diffs from their sources
     * @throws ClientException
     * @since 1.7
     */
    void diff(String target1, Revision revision1, String target2,
              Revision revision2, String relativeToDir, String outFileName,
              int depth, String[] changelists, boolean ignoreAncestry,
              boolean noDiffDeleted, boolean force, boolean copiesAsAdds)
            throws ClientException;

    /**
     * Display the differences between two paths
     * @param target1       first path or url
     * @param revision1     first revision
     * @param target2       second path or url
     * @param revision2     second revision
     * @param relativeToDir index path is relative to this path
     * @param outFileName   file name where difference are written
     * @param depth         how deep to traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @throws ClientException
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean, boolean)} instead.
     * @since 1.5
     */
    void diff(String target1, Revision revision1, String target2,
              Revision revision2, String relativeToDir, String outFileName,
              int depth, String[] changelists, boolean ignoreAncestry,
              boolean noDiffDeleted, boolean force)
            throws ClientException;

    /**
     * Display the differences between two paths.
     * @param target        path or url
     * @param pegRevision   revision tointerpret target
     * @param startRevision first Revision to compare
     * @param endRevision   second Revision to compare
     * @param outFileName   file name where difference are written
     * @param recurse       traverse into subdirectories
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @throws ClientException
     * @deprecated Use {@link #diff(String, Revision, Revision, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    void diff(String target, Revision pegRevision, Revision startRevision,
              Revision endRevision, String outFileName, boolean recurse,
              boolean ignoreAncestry, boolean noDiffDeleted, boolean force)
            throws ClientException;

    /**
     * Display the differences between two paths.
     * @param target        path or url
     * @param pegRevision   revision tointerpret target
     * @param startRevision first Revision to compare
     * @param endRevision   second Revision to compare
     * @param relativeToDir index path is relative to this path
     * @param outFileName   file name where difference are written
     * @param depth         how deep to traverse into subdirectories
     * @param changelists  if non-null, filter paths using changelists
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @param copiesAsAdds  if set, copied files will be shown in their
     *                      entirety, not as diffs from their sources
     * @throws ClientException
     * @since 1.7
     */
    void diff(String target, Revision pegRevision, Revision startRevision,
              Revision endRevision, String relativeToDir, String outFileName,
              int depth, String[] changelists, boolean ignoreAncestry,
              boolean noDiffDeleted, boolean force, boolean copiesAsAdds)
            throws ClientException;

    /**
     * Display the differences between two paths.
     * @param target        path or url
     * @param pegRevision   revision tointerpret target
     * @param startRevision first Revision to compare
     * @param endRevision   second Revision to compare
     * @param relativeToDir index path is relative to this path
     * @param outFileName   file name where difference are written
     * @param depth         how deep to traverse into subdirectories
     * @param changelists  if non-null, filter paths using changelists
     * @param ignoreAncestry ignore if files are not related
     * @param noDiffDeleted no output on deleted files
     * @param force         diff even on binary files
     * @throws ClientException
     * @deprecated Use {@link #diff(String, Revision, Revision, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean, boolean)} instead.
     * @since 1.5
     */
    void diff(String target, Revision pegRevision, Revision startRevision,
              Revision endRevision, String relativeToDir, String outFileName,
              int depth, String[] changelists, boolean ignoreAncestry,
              boolean noDiffDeleted, boolean force)
            throws ClientException;

    /**
     * Produce a diff summary which lists the items changed between
     * path and revision pairs.
     *
     * @param target1 Path or URL.
     * @param revision1 Revision of <code>target1</code>.
     * @param target2 Path or URL.
     * @param revision2 Revision of <code>target2</code>.
     * @param depth how deep to recurse.
     * @param changelists  if non-null, filter paths using changelists
     * @param ignoreAncestry Whether to ignore unrelated files during
     * comparison.  False positives may potentially be reported if
     * this parameter <code>false</code>, since a file might have been
     * modified between two revisions, but still have the same
     * contents.
     * @param receiver As each is difference is found, this callback
     * is invoked with a description of the difference.
     *
     * @throws ClientException
     * @since 1.5
     */
    void diffSummarize(String target1, Revision revision1,
                       String target2, Revision revision2,
                       int depth, String[] changelists, boolean ignoreAncestry,
                       DiffSummaryReceiver receiver)
            throws ClientException;

    /**
     * Produce a diff summary which lists the items changed between
     * path and revision pairs.
     *
     * @param target Path or URL.
     * @param pegRevision Revision at which to interpret
     * <code>target</code>.  If {@link RevisionKind#unspecified} or
     * <code>null</code>, behave identically to {@link
     * #diffSummarize(String, Revision, String, Revision, int,
     * boolean, DiffSummaryReceiver)}, using <code>path</code> for
     * both of that method's targets.
     * @param startRevision Beginning of range for comparsion of
     * <code>target</code>.
     * @param endRevision End of range for comparsion of
     * <code>target</code>.
     * @param depth how deep to recurse.
     * @param changelists  if non-null, filter paths using changelists
     * @param ignoreAncestry Whether to ignore unrelated files during
     * comparison.  False positives may potentially be reported if
     * this parameter <code>false</code>, since a file might have been
     * modified between two revisions, but still have the same
     * contents.
     * @param receiver As each is difference is found, this callback
     * is invoked with a description of the difference.
     *
     * @throws ClientException
     * @since 1.5
     */
    void diffSummarize(String target, Revision pegRevision,
                       Revision startRevision, Revision endRevision,
                       int depth, String[] changelists, boolean ignoreAncestry,
                       DiffSummaryReceiver receiver)
        throws ClientException;

    /**
     * Retrieves the properties of an item
     * @param path  the path of the item
     * @return array of property objects
     * @throws ClientException
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.0
     */
    PropertyData[] properties(String path) throws ClientException;

    /**
     * Retrieves the properties of an item
     * @param path      the path of the item
     * @param revision  the revision of the item
     * @return array of property objects
     * @throws ClientException
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.2
     */
    PropertyData[] properties(String path, Revision revision)
            throws ClientException;

    /**
     * Retrieves the properties of an item
     * @param path      the path of the item
     * @param revision  the revision of the item
     * @param pegRevision the revision to interpret path
     * @return array of property objects
     * @throws ClientException
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.2
     */
    PropertyData[] properties(String path, Revision revision,
                              Revision pegRevision)
            throws ClientException;

    /**
     * Retrieves the properties of an item
     *
     * @param path        the path of the item
     * @param revision    the revision of the item
     * @param pegRevision the revision to interpret path
     * @param depth       the depth to recurse into subdirectories
     * @param changelists changelists to filter by
     * @param callback    the callback to use to return the properties
     * @throws ClientException
     * @since 1.5
     */
    void properties(String path, Revision revision, Revision pegRevision,
                    int depth, String[] changelists, ProplistCallback callback)
            throws ClientException;

    /**
     * Sets one property of an item with a String value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @throws ClientException
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.0
     */
    void propertySet(String path, String name, String value, boolean recurse)
            throws ClientException;

    /**
     * Sets one property of an item with a String value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @param force     do not check if the value is valid
     * @throws ClientException
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.2
     */
    void propertySet(String path, String name, String value, boolean recurse,
                     boolean force)
            throws ClientException;

    /**
     * Sets one property of an item with a byte array value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @throws ClientException
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.0
     */
    void propertySet(String path, String name, byte[] value, boolean recurse)
            throws ClientException;

    /**
     * Sets one property of an item with a byte array value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @param force     do not check if the value is valid
     * @throws ClientException
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.2
     */
    void propertySet(String path, String name, byte[] value, boolean recurse,
                     boolean force)
            throws ClientException;

    /**
     * Sets one property of an item with a String value
     *
     * @param path    path of the item
     * @param name    name of the property
     * @param value   new value of the property
     * @param depth   the depth to recurse into subdirectories
     * @param changelists changelists to filter by
     * @param force   do not check if the value is valid
     * @param revpropTable A string-to-string mapping of revision properties
     *                     to values which will be set if this operation
     *                     results in a commit.
     * @throws ClientException
     * @since 1.5
     */
    void propertySet(String path, String name, String value, int depth,
                     String[] changelists, boolean force,
                     Map revpropTable)
            throws ClientException;

    /**
     * Remove one property of an item.
     * @param path      path of the item
     * @param name      name of the property
     * @param recurse   remove the property also on subdirectories
     * @throws ClientException
     * @deprecated Use {@link #propertyRemove(String, String, int)} instead.
     * @since 1.0
     */
    void propertyRemove(String path, String name, boolean recurse)
            throws ClientException;

    /**
     * Remove one property of an item.
     * @param path      path of the item
     * @param name      name of the property
     * @param depth     the depth to recurse into subdirectories
     * @param changelists changelists to filter by
     * @throws ClientException
     * @since 1.5
     */
    void propertyRemove(String path, String name, int depth,
                        String[] changelists)
            throws ClientException;

    /**
     * Create and sets one property of an item with a String value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @throws ClientException
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.0
     */
    void propertyCreate(String path, String name, String value,
                        boolean recurse)
            throws ClientException;

    /**
     * Create and sets one property of an item with a String value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @param force     do not check if the value is valid
     * @throws ClientException
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.2
     */
    void propertyCreate(String path, String name, String value,
                        boolean recurse, boolean force)
            throws ClientException;

    /**
     * Create and sets one property of an item with a byte array value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @throws ClientException
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.0
     */
    void propertyCreate(String path, String name, byte[] value,
                        boolean recurse)
            throws ClientException;

    /**
     * Create and sets one property of an item with a byte array value
     * @param path      path of the item
     * @param name      name of the property
     * @param value     new value of the property
     * @param recurse   set property also on the subdirectories
     * @param force     do not check if the value is valid
     * @throws ClientException
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.2
     */
    void propertyCreate(String path, String name, byte[] value,
                        boolean recurse, boolean force)
            throws ClientException;

    /**
     * Create and sets one property of an item with a byte array value
     *
     * @param path    path of the item
     * @param name    name of the property
     * @param value   new value of the property
     * @param depth   depth to set property on the subdirectories
     * @param changelists changelists to filter by
     * @param force   do not check if the value is valid
     * @throws ClientException
     * @since 1.5
     */
    void propertyCreate(String path, String name, String value, int depth,
                        String[] changelists, boolean force)
            throws ClientException;

    /**
     * Retrieve one revsision property of one item
     * @param path      path of the item
     * @param name      name of the property
     * @param rev       revision to retrieve
     * @return the Property
     * @throws ClientException
     * @since 1.0
     */
    PropertyData revProperty(String path, String name, Revision rev)
            throws ClientException;

    /**
     * Retrieve all revsision properties of one item
     * @param path      path of the item
     * @param rev       revision to retrieve
     * @return the Properties
     * @throws ClientException
     * @since 1.2
     */
    PropertyData[] revProperties(String path, Revision rev)
            throws ClientException;

    /**
     * set one revsision property of one item
     * @param path      path of the item
     * @param name      name of the property
     * @param rev       revision to retrieve
     * @param value     value of the property
     * @param force     use force to set
     * @throws ClientException
     * @since 1.2
     */
    void setRevProperty(String path, String name, Revision rev, String value,
                        boolean force)
            throws ClientException;

    /**
     * set one revsision property of one item
     * @param path      path of the item
     * @param name      name of the property
     * @param rev       revision to retrieve
     * @param value     value of the property
     * @param originalValue the original value of the property.
     * @param force     use force to set
     * @throws ClientException
     * @since 1.6
     */
    void setRevProperty(String path, String name, Revision rev, String value,
                        String originalValue, boolean force)
            throws ClientException;

    /**
     * Retrieve one property of one item
     * @param path      path of the item
     * @param name      name of property
     * @return the Property
     * @throws ClientException
     * @deprecated Use {@link #propertyGet(String, String, Revision)} instead.
     * @since 1.0
     */
    PropertyData propertyGet(String path, String name) throws ClientException;

    /**
     * Retrieve one property of one item
     * @param path      path of the item
     * @param name      name of property
     * @param revision  revision of the item
     * @return the Property
     * @throws ClientException
     * @since 1.2
     */
    PropertyData propertyGet(String path, String name, Revision revision)
            throws ClientException;

    /**
     * Retrieve one property of one item
     * @param path      path of the item
     * @param name      name of property
     * @param revision  revision of the item
     * @param pegRevision the revision to interpret path
     * @return the Property
     * @throws ClientException
     * @since 1.2
     */
    PropertyData propertyGet(String path, String name, Revision revision,
                             Revision pegRevision)
            throws ClientException;

    /**
     * Retrieve the content of a file
     * @param path      the path of the file
     * @param revision  the revision to retrieve
     * @return  the content as byte array
     * @throws ClientException
     * @deprecated Use {@link #fileContent(String, Revision, Revision)}
     *             instead.
     * @since 1.0
     */
    byte[] fileContent(String path, Revision revision) throws ClientException;

    /**
     * Retrieve the content of a file
     * @param path      the path of the file
     * @param revision  the revision to retrieve
     * @param pegRevision the revision to interpret path
     * @return  the content as byte array
     * @throws ClientException
     * @since 1.2
     */
    byte[] fileContent(String path, Revision revision, Revision pegRevision)
            throws ClientException;

    /**
     * Write the file's content to the specified output stream.  If
     * you need an InputStream, use a
     * PipedInputStream/PipedOutputStream combination.
     *
     * @param path        the path of the file
     * @param revision    the revision to retrieve
     * @param pegRevision the revision at which to interpret the path
     * @param stream      the stream to write the file's content to
     * @throws ClientException
     * @see java.io.PipedOutputStream
     * @see java.io.PipedInputStream
     * @since 1.0
     */
    void streamFileContent(String path, Revision revision, Revision pegRevision,
                           int bufferSize, OutputStream stream)
        throws ClientException;

    /**
     * Rewrite the url's in the working copy
     * @param from      old url
     * @param to        new url
     * @param path      working copy path
     * @param recurse   recurse into subdirectories
     * @throws ClientException
     * @since 1.0
     */
    void relocate(String from, String to, String path, boolean recurse)
            throws ClientException;

    /**
     * Return for each line of the file, the author and the revision of the
     * last together with the content.
     * @deprecated
     * @param path          the path
     * @param revisionStart the first revision to show
     * @param revisionEnd   the last revision to show
     * @return  the content together with author and revision of last change
     * @throws ClientException
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.0
     */
    byte[] blame(String path, Revision revisionStart, Revision revisionEnd)
            throws ClientException;

    /**
     * Retrieve the content together with the author, the revision and the date
     * of the last change of each line
     * @param path          the path
     * @param revisionStart the first revision to show
     * @param revisionEnd   the last revision to show
     * @param callback      callback to receive the file content and the other
     *                      information
     * @throws ClientException
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.0
     */
    void blame(String path, Revision revisionStart, Revision revisionEnd,
               BlameCallback callback) throws ClientException;

    /**
     * Retrieve the content together with the author, the revision and the date
     * of the last change of each line
     * @param path          the path
     * @param pegRevision   the revision to interpret the path
     * @param revisionStart the first revision to show
     * @param revisionEnd   the last revision to show
     * @param callback      callback to receive the file content and the other
     *                      information
     * @throws ClientException
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.2
     */
    void blame(String path, Revision pegRevision, Revision revisionStart,
               Revision revisionEnd,
               BlameCallback callback) throws ClientException;

    /**
     * Retrieve the content together with the author, the revision and the date
     * of the last change of each line
     * @param path          the path
     * @param pegRevision   the revision to interpret the path
     * @param revisionStart the first revision to show
     * @param revisionEnd   the last revision to show
     * @param ignoreMimeType whether or not to ignore the mime-type
     * @param includeMergedRevisions whether or not to include extra merge
     *                      information
     * @param callback      callback to receive the file content and the other
     *                      information
     * @throws ClientException
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback3)}
     *                               instead.
     * @since 1.5
     */
    void blame(String path, Revision pegRevision, Revision revisionStart,
               Revision revisionEnd, boolean ignoreMimeType,
               boolean includeMergedRevisions,
               BlameCallback2 callback) throws ClientException;

    /**
     * Retrieve the content together with the author, the revision and the date
     * of the last change of each line
     * @param path          the path
     * @param pegRevision   the revision to interpret the path
     * @param revisionStart the first revision to show
     * @param revisionEnd   the last revision to show
     * @param ignoreMimeType whether or not to ignore the mime-type
     * @param includeMergedRevisions whether or not to include extra merge
     *                      information
     * @param callback      callback to receive the file content and the other
     *                      information
     * @throws ClientException
     * @since 1.7
     */
    void blame(String path, Revision pegRevision, Revision revisionStart,
               Revision revisionEnd, boolean ignoreMimeType,
               boolean includeMergedRevisions,
               BlameCallback3 callback) throws ClientException;

    /**
     * Set directory for the configuration information, taking the
     * usual steps to ensure that Subversion's config file templates
     * exist in the specified location..  On Windows, setting a
     * non-<code>null</code> value will override lookup of
     * configuration in the registry.
     * @param configDir Path of the directory, or <code>null</code>
     * for the platform's default.
     * @throws ClientException
     * @since 1.0
     */
    void setConfigDirectory(String configDir) throws ClientException;

    /**
     * Get the configuration directory
     * @return  the directory
     * @throws ClientException
     * @since 1.0
     */
    String getConfigDirectory() throws ClientException;

    /**
     * cancel the active operation
     * @throws ClientException
     * @since 1.0
     */
    void cancelOperation() throws ClientException;

    /**
     * Retrieves the working copy information for an item
     * @param path  path of the item
     * @return      the information object
     * @throws ClientException
     * @deprecated Use {@link #info2(String, Revision, Revision, int,
     *                               InfoCallback)} instead.
     * @since 1.0
     */
    Info info(String path) throws ClientException;

    /**
     * Add paths to a changelist
     * @param paths       paths to add to the changelist
     * @param changelist  changelist name
     * @param depth       the depth to recurse
     * @param changelists changelists to filter by
     * @since 1.5
     */
    void addToChangelist(String[] paths, String changelist, int depth,
                         String[] changelists)
            throws ClientException;

    /**
     * Remove paths from a changelist
     * @param paths       paths to remove from the changelist
     * @param depth       the depth to recurse
     * @param changelists changelists to filter by
     * @since 1.5
     */
    void removeFromChangelists(String[] paths, int depth, String[] changelists)
            throws ClientException;

    /**
     * Recursively get the paths which belong to a changelist
     * @param rootPath    the wc path under which to check
     * @param changelists the changelists to look under
     * @param depth       the depth to recurse
     * @param callback    the callback to return the changelists through
     * @since 1.5
     */
    void getChangelists(String rootPath, String[] changelists, int depth,
                        ChangelistCallback callback)
            throws ClientException;

    /**
     * Lock a working copy item
     * @param path  path of the item
     * @param comment
     * @param force break an existing lock
     * @throws ClientException
     * @since 1.2
     */
    void lock(String[] path, String comment, boolean force)
            throws ClientException;

    /**
     * Unlock a working copy item
     * @param path  path of the item
     * @param force break an existing lock
     * @throws ClientException
     * @since 1.2
     */
    void unlock(String[] path, boolean force)
            throws ClientException;

    /**
     * Retrieve information about repository or working copy items.
     * @param pathOrUrl     the path or the url of the item
     * @param revision      the revision of the item to return
     * @param pegRevision   the revision to interpret pathOrUrl
     * @param recurse       flag if to recurse, if the item is a directory
     * @return              the information objects
     * @deprecated Use {@link #info2(String, Revision, Revision, int,
     *                               InfoCallback)} instead.
     * @since 1.2
     */
    Info2[] info2(String pathOrUrl, Revision revision, Revision pegRevision,
                  boolean recurse)
        throws ClientException;

    /**
     * Retrieve information about repository or working copy items.
     * @param pathOrUrl     the path or the url of the item
     * @param revision      the revision of the item to return
     * @param pegRevision   the revision to interpret pathOrUrl
     * @param depth         the depth to recurse
     * @param changelists   if non-null, filter paths using changelists
     * @param callback      a callback to receive the infos retrieved
     * @since 1.5
     */
    void info2(String pathOrUrl, Revision revision, Revision pegRevision,
               int depth, String[] changelists, InfoCallback callback)
        throws ClientException;

    /**
     * Produce a compact "version number" for a working copy
     * @param path          path of the working copy
     * @param trailUrl      to detect switches of the whole working copy
     * @param lastChanged   last changed rather than current revisions
     * @return      the compact "version number"
     * @throws ClientException
     * @since 1.2
     */
    String getVersionInfo(String path, String trailUrl, boolean lastChanged)
            throws ClientException;

    /**
     * Recursively upgrade a working copy to a new metadata storage format.
     * @param path                  path of the working copy
     * @throws ClientException
     * @since 1.7
     */
    void upgrade(String path)
            throws ClientException;
}
