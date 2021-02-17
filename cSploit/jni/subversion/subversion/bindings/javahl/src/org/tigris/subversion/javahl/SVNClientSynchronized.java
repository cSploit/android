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
 * This class provides a threadsafe wrapped for SVNClient
 */
public class SVNClientSynchronized implements SVNClientInterface
{
    /**
     * the wrapped object, which does all the work
     */
    private SVNClient worker;

    /**
     * our class, we synchronize on that.
     */
    static private Class clazz = SVNClientSynchronized.class;

    /**
     * Create our worker
     */
    public SVNClientSynchronized()
    {
        synchronized(clazz)
        {
            worker = new SVNClient();
        }
    }

    /**
     * release the native peer (should not depend on finalize)
     */
    public void dispose()
    {
        worker.dispose();
    }

    /**
     * @return Version information about the underlying native libraries.
     */
    public Version getVersion()
    {
        synchronized(clazz)
        {
            return worker.getVersion();
        }
    }

    /**
     * @return The name of the working copy's administrative
     * directory, which is usually <code>.svn</code>.
     * @see <a
     * href="http://svn.apache.org/repos/asf/subversion/trunk/notes/asp-dot-net-hack.txt">
     * Instructions on changing this as a work-around for the behavior of
     * ASP.Net on Windows.</a>
     * @since 1.3
     */
    public String getAdminDirectoryName()
    {
        synchronized(clazz)
        {
            return worker.getAdminDirectoryName();
        }
    }

    /**
     * @param name The name of the directory to compare.
     * @return Whether <code>name</code> is that of a working copy
     * administrative directory.
     * @since 1.3
     */
    public boolean isAdminDirectory(String name)
    {
        synchronized(clazz)
        {
            return worker.isAdminDirectory(name);
        }
    }

    /**
     * Returns the last destination path submitted.
     * @deprecated
     * @return path in Subversion format.
     */
    public String getLastPath()
    {
        synchronized(clazz)
        {
            return worker.getLastPath();
        }
    }

    /**
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    public Status[] status(String path, boolean descend, boolean onServer,
                           boolean getAll)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.status(path, descend, onServer, getAll);
        }
    }

    /**
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    public Status[] status(String path, boolean descend, boolean onServer,
                           boolean getAll, boolean noIgnore)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.status(path, descend, onServer, getAll, noIgnore);
        }
    }

    /**
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.2
     */
    public Status[] status(String path, boolean descend, boolean onServer,
                           boolean getAll, boolean noIgnore,
                           boolean ignoreExternals)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.status(path, descend, onServer, getAll, noIgnore,
                                 ignoreExternals);
        }
    }

    /**
     * @since 1.5
     */
    public void status(String path, int depth, boolean onServer,
                       boolean getAll, boolean noIgnore,
                       boolean ignoreExternals, String[] changelists,
                       StatusCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.status(path, depth, onServer, getAll, noIgnore,
                          ignoreExternals, changelists, callback);
        }
    }

    /**
     * @deprecated Use {@link #list(String, Revision, Revision, int, int,
     *                              boolean, ListCallback)} instead.
     * @since 1.0
     */
    public DirEntry[] list(String url, Revision revision, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.list(url, revision, recurse);
        }
    }

    /**
     * @deprecated Use {@link #list(String, Revision, Revision, int, int,
     *                              boolean, ListCallback)} instead.
     * @since 1.2
     */
    public DirEntry[] list(String url, Revision revision, Revision pegRevision,
                           boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.list(url, revision, pegRevision, recurse);
        }
    }

    /**
     * @since 1.5
     */
    public void list(String url, Revision revision, Revision pegRevision,
                     int depth, int direntFields, boolean fetchLocks,
                     ListCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.list(url, revision, pegRevision, depth, direntFields,
                        fetchLocks, callback);
        }
    }

    /**
     * @deprecated Use {@link #status(String, int, boolean, boolean,
     *                                boolean, boolean, StatusCallback)}
     *             instead.
     * @since 1.0
     */
    public Status singleStatus(String path, boolean onServer)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.singleStatus(path, onServer);
        }
    }

    /**
     * @since 1.0
     */
    public void username(String username)
    {
        synchronized(clazz)
        {
            worker.username(username);
        }
    }

    /**
     * @since 1.0
     */
    public void password(String password)
    {
        synchronized(clazz)
        {
            worker.password(password);
        }
    }

    /**
     * @since 1.0
     */
    public void setPrompt(PromptUserPassword prompt)
    {
        synchronized(clazz)
        {
            worker.setPrompt(prompt);
        }
    }

    /**
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    public LogMessage[] logMessages(String path, Revision revisionStart,
                                    Revision revisionEnd)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.logMessages(path, revisionStart, revisionEnd, true,
                                      false);
        }
    }

    /**
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    public LogMessage[] logMessages(String path, Revision revisionStart,
                                    Revision revisionEnd, boolean stopOnCopy)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.logMessages(path, revisionStart, revisionEnd,
                                      stopOnCopy, false);
        }
    }

    /**
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.0
     */
    public LogMessage[] logMessages(String path, Revision revisionStart,
                                    Revision revisionEnd, boolean stopOnCopy,
                                    boolean discoverPath)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.logMessages(path, revisionStart, revisionEnd,
                                      stopOnCopy, discoverPath);
        }
    }

    /**
     * @deprecated Use {@link #logMessages(String, Revision, Revision, Revision,
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.2
     */
    public LogMessage[] logMessages(String path, Revision revisionStart,
                                    Revision revisionEnd, boolean stopOnCopy,
                                    boolean discoverPath, long limit)
            throws ClientException
    {
        synchronized (clazz)
        {
            return worker.logMessages(path, revisionStart, revisionEnd,
                                      stopOnCopy, discoverPath, limit);
        }
    }

    /**
     * @deprecated Use {@link #logMessages(String, Revision, RevisionRange[],
     *                                     boolean, boolean, boolean, String[],
     *                                     long, LogMessageCallback)} instead.
     * @since 1.5
     */
    public void logMessages(String path, Revision pegRevision,
                            Revision revisionStart,
                            Revision revisionEnd, boolean stopOnCopy,
                            boolean discoverPath,
                            boolean includeMergedRevisions,
                            String[] revProps, long limit,
                            LogMessageCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.logMessages(path, pegRevision, revisionStart,
                               revisionEnd, stopOnCopy, discoverPath,
                               includeMergedRevisions, revProps,
                               limit, callback);
        }
    }

    /**
     * @since 1.6
     */
    public void logMessages(String path, Revision pegRevision,
                            RevisionRange[] revisionRanges, boolean stopOnCopy,
                            boolean discoverPath,
                            boolean includeMergedRevisions,
                            String[] revProps, long limit,
                            LogMessageCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.logMessages(path, pegRevision, revisionRanges,
                               stopOnCopy, discoverPath, includeMergedRevisions,
                               revProps, limit, callback);
        }
    }

    /**
     * @deprecated Use {@link #checkout(String, String, Revision, Revision,
     *                                  int, boolean, boolean)} instead.
     * @since 1.0
     */
    public long checkout(String moduleName, String destPath, Revision revision,
                         boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.checkout(moduleName, destPath, revision, recurse);
        }
    }

    /**
     * @deprecated Use {@link #checkout(String, String, Revision, Revision,
     *                                  int, boolean, boolean)} instead.
     * @since 1.2
     */
    public long checkout(String moduleName, String destPath, Revision revision,
                         Revision pegRevision, boolean recurse,
                         boolean ignoreExternals)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.checkout(moduleName, destPath, revision, pegRevision,
                                   recurse, ignoreExternals);
        }
    }

    /**
     * @since 1.5
     */
    public long checkout(String moduleName, String destPath, Revision revision,
                         Revision pegRevision, int depth,
                         boolean ignoreExternals,
                         boolean allowUnverObstructions)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.checkout(moduleName, destPath, revision, pegRevision,
                                   depth, ignoreExternals,
                                   allowUnverObstructions);
        }
    }

    /**
     * @deprecated Use {@link #notification2(Notify2)} instead.
     * @since 1.0
     */
    public void notification(Notify notify)
    {
        synchronized(clazz)
        {
            worker.notification(notify);
        }
    }

    /**
     * @since 1.2
     */
    public void notification2(Notify2 notify)
    {
        synchronized(clazz)
        {
            worker.notification2(notify);
        }
    }

    /**
     * @since 1.5
     */
    public void setConflictResolver(ConflictResolverCallback listener)
    {
        synchronized (clazz)
        {
            worker.setConflictResolver(listener);
        }
    }

    /**
     * @since 1.5
     */
    public void setProgressListener(ProgressListener listener)
    {
        synchronized (clazz)
        {
            worker.setProgressListener(listener);
        }
    }

    /**
     * @since 1.0
     */
    public void commitMessageHandler(CommitMessage messageHandler)
    {
        synchronized(clazz)
        {
            worker.commitMessageHandler(messageHandler);
        }
    }
    /**
     * @deprecated Use {@link #remove(String[], String, boolean, boolean)}
     *             instead.
     * @since 1.0
     */
    public void remove(String[] path, String message, boolean force)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.remove(path, message, force);
        }
    }

    /**
     * @since 1.5
     */
    public void remove(String[] path, String message, boolean force,
                       boolean keepLocal, Map revpropTable)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.remove(path, message, force, keepLocal, revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #revert(String, int)} instead.
     * @since 1.0
     */
    public void revert(String path, boolean recurse) throws ClientException
    {
        synchronized(clazz)
        {
            worker.revert(path, recurse);
        }
    }

    /**
     * @since 1.5
     */
    public void revert(String path, int depth, String[] changelists)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.revert(path, depth, changelists);
        }
    }

    /**
     * @deprecated Use {@link #add(String, int, boolean, boolean, boolean)}
     *             instead.
     * @since 1.0
     */
    public void add(String path, boolean recurse) throws ClientException
    {
        synchronized(clazz)
        {
            worker.add(path, recurse);
        }
    }

    /**
     * @deprecated Use {@link #add(String, int, boolean, boolean, boolean)}
     *             instead.
     * @since 1.2
     */
    public void add(String path, boolean recurse, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.add(path, recurse, force);
        }
    }

    /**
     * @since 1.5
     */
    public void add(String path, int depth, boolean force,
                    boolean noIgnores, boolean addParents)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.add(path, depth, force, noIgnores, addParents);
        }
    }

    /**
     * @deprecated Use {@link #update(String, Revision, int, boolean,
     *                                boolean, boolean)} instead.
     * @since 1.0
     */
    public long update(String path, Revision revision, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.update(path, revision, recurse);
        }
    }

    /**
     * @deprecated Use {@link #update(String[], Revision, int, boolean,
     *                                boolean, boolean)} instead.
     * @since 1.2
     */
    public long[] update(String[] path, Revision revision, boolean recurse,
                         boolean ignoreExternals)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.update(path, revision, recurse, ignoreExternals);
        }
    }

    /**
     * @since 1.5
     */
    public long update(String path, Revision revision, int depth,
                       boolean depthIsSticky, boolean ignoreExternals,
                       boolean allowUnverObstructions)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.update(path, revision, depth, depthIsSticky,
                                 ignoreExternals, allowUnverObstructions);
        }
    }

    /**
     * @since 1.5
     */
    public long[] update(String[] path, Revision revision, int depth,
                         boolean depthIsSticky, boolean ignoreExternals,
                         boolean allowUnverObstructions)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.update(path, revision, depth, depthIsSticky,
                                 ignoreExternals, allowUnverObstructions);
        }
    }

    /**
     * @deprecated Use {@link #commit(String[], String, int, boolean, boolean,
     *                                String[])} instead.
     * @since 1.0
     */
    public long commit(String[] path, String message, boolean recurse)
            throws ClientException
    {
        synchronized (clazz)
        {
            return worker.commit(path, message, recurse, false);
        }
    }

    /**
     * @deprecated Use {@link #commit(String[], String, int, boolean, boolean,
     *                                String[])} instead.
     * @since 1.2
     */
    public long commit(String[] path, String message, boolean recurse,
                       boolean noUnlock)
            throws ClientException
    {
        synchronized (clazz)
        {
            return worker.commit(path, message, recurse, noUnlock);
        }
    }

    /**
     * @since 1.5
     */
    public long commit(String[] path, String message, int depth,
                       boolean noUnlock, boolean keepChangelist,
                       String[] changelists, Map revpropTable)
            throws ClientException
    {
        synchronized (clazz)
        {
            return worker.commit(path, message, depth, noUnlock,
                                 keepChangelist, changelists, revpropTable);
        }
    }

    /**
     * @since 1.7
     */
    public void copy(CopySource[] sources, String destPath, String message,
                     boolean copyAsChild, boolean makeParents,
                     boolean ignoreExternals, Map revpropTable)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.copy(sources, destPath, message, copyAsChild,
                        makeParents, ignoreExternals, revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #copy(CopySource[], String, String, boolean,
     *                              boolean, boolean, Map)} instead.
     * @since 1.5
     */
    public void copy(CopySource[] sources, String destPath, String message,
                     boolean copyAsChild, boolean makeParents,
                     Map revpropTable)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.copy(sources, destPath, message, copyAsChild,
                        makeParents, revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #copy(CopySource[], String, String, boolean,
     *                              boolean, boolean, Map)} instead.
     * @since 1.0
     */
    public void copy(String srcPath, String destPath, String message,
                     Revision revision) throws ClientException
    {
        synchronized(clazz)
        {
            worker.copy(srcPath, destPath, message, revision);
        }
    }

    /**
     * @since 1.5
     */
    public void move(String[] srcPaths, String destPath, String message,
                     boolean force, boolean moveAsChild,
                     boolean makeParents, Map revpropTable)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.move(srcPaths, destPath, message, force, moveAsChild,
                        makeParents, revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #move(String[], String, String, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    public void move(String srcPath, String destPath, String message,
                     Revision revision, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.move(srcPath, destPath, message, revision, force);
        }
    }

    /**
     * @deprecated Use {@link #move(String[], String, String, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.0
     */
    public void move(String srcPath, String destPath, String message,
                     boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.move(srcPath, destPath, message, force);
        }
    }

    /**
     * @since 1.5
     */
    public void mkdir(String[] path, String message, boolean makeParents,
                      Map revpropTable)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.mkdir(path, message, makeParents, revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #mkdir(String[], String, boolean)} instead.
     * @since 1.0
     */
    public void mkdir(String[] path, String message) throws ClientException
    {
        synchronized(clazz)
        {
            worker.mkdir(path, message);
        }
    }

    /**
     * @since 1.0
     */
    public void cleanup(String path) throws ClientException
    {
        synchronized(clazz)
        {
            worker.cleanup(path);
        }
    }

    /**
     * @since 1.5
     */
    public void resolve(String path, int depth, int conflictResult)
        throws SubversionException
    {
        synchronized (clazz)
        {
            worker.resolve(path, depth, conflictResult);
        }
    }

    /**
     * @deprecated Use {@link #resolve(String, int, int)} instead.
     * @since 1.0
     */
    public void resolved(String path, boolean recurse) throws ClientException
    {
        synchronized (clazz)
        {
            worker.resolved(path, recurse);
        }
    }

    /**
     * @deprecated Use {@link #doExport(String, String, Revision, Revision,
     *                                  boolean, boolean, int, String)} instead.
     * @since 1.0
     */
    public long doExport(String srcPath, String destPath, Revision revision,
                         boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.doExport(srcPath, destPath, revision, force);
        }
    }

    /**
     * @deprecated Use {@link #doExport(String, String, Revision, Revision,
     *                                  boolean, boolean, int, String)} instead.
     * @since 1.2
     */
    public long doExport(String srcPath, String destPath, Revision revision,
                         Revision pegRevision, boolean force,
                         boolean ignoreExternals, boolean recurse,
                         String nativeEOL)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.doExport(srcPath, destPath, revision, pegRevision,
                                   force, ignoreExternals, recurse, nativeEOL);
        }
    }

    /**
     * @since 1.5
     */
    public long doExport(String srcPath, String destPath, Revision revision,
                  Revision pegRevision, boolean force, boolean ignoreExternals,
                  int depth, String nativeEOL)
            throws ClientException
    {
        synchronized (clazz)
        {
            return worker.doExport(srcPath, destPath, revision, pegRevision,
                                   force, ignoreExternals, depth, nativeEOL);
        }
    }

    /**
     * @since 1.5
     */
    public long doSwitch(String path, String url, Revision revision,
                         Revision pegRevision, int depth,
                         boolean depthIsSticky, boolean ignoreExternals,
                         boolean allowUnverObstructions)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.doSwitch(path, url, revision, pegRevision, depth,
                                   depthIsSticky, ignoreExternals,
                                   allowUnverObstructions);
        }
    }

    /**
     * @deprecated Use {@link #doSwitch(String, String, Revision, boolean)}
     *             instead.
     * @since 1.0
     */
    public long doSwitch(String path, String url, Revision revision,
                         boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.doSwitch(path, url, revision, recurse);
        }
    }

    /**
     * @deprecated Use {@link #doImport(String, String, String, int, boolean,
     *                                  boolean)} instead.
     * @since 1.0
     */
    public void doImport(String path, String url, String message,
                         boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.doImport(path, url, message, recurse);
        }
    }

    /**
     * @since 1.5
     */
    public void doImport(String path, String url, String message, int depth,
                         boolean noIgnore, boolean ignoreUnknownNodeTypes,
                         Map revpropTable)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.doImport(path, url, message, depth, noIgnore,
                            ignoreUnknownNodeTypes, revpropTable);
        }
    }

    /**
     * @since 1.5
     */
    public String[] suggestMergeSources(String path, Revision pegRevision)
            throws SubversionException
    {
        synchronized (clazz)
        {
            return worker.suggestMergeSources(path, pegRevision);
        }
    }

    /**
     * @deprecated Use {@link #merge(String, Revision, String, Revision,
     *                               String, boolean, int, boolean,
     *                               boolean, boolean)} instead.
     * @since 1.0
     */
    public void merge(String path1, Revision revision1, String path2,
                      Revision revision2, String localPath, boolean force,
                      boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.merge(path1, revision1, path2, revision2, localPath, force,
                         recurse);
        }
    }

    /**
     * @deprecated Use {@link #merge(String, Revision, String, Revision,
     *                               String, boolean, int, boolean,
     *                               boolean, boolean)} instead.
     * @since 1.2
     */
    public void merge(String path1, Revision revision1, String path2,
                      Revision revision2, String localPath, boolean force,
                      boolean recurse, boolean ignoreAncestry, boolean dryRun)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.merge(path1, revision1, path2, revision2, localPath, force,
                         recurse, ignoreAncestry, dryRun);
        }
    }

    /**
     * @since 1.5
     */
    public void merge(String path1, Revision revision1, String path2,
                      Revision revision2, String localPath, boolean force,
                      int depth, boolean ignoreAncestry, boolean dryRun,
                      boolean recordOnly) throws ClientException
    {
        synchronized (clazz)
        {
            worker.merge(path1, revision1, path2, revision2, localPath, force,
                         depth, ignoreAncestry, dryRun, recordOnly);
        }
    }

    /**
     * @deprecated Use {@link #merge(String, Revision, RevisionRange[],
     *                               String, boolean, int, boolean,
     *                               boolean, boolean)} instead.
     * @since 1.2
     */
    public void merge(String path, Revision pegRevision, Revision revision1,
                      Revision revision2, String localPath, boolean force,
                      boolean recurse, boolean ignoreAncestry, boolean dryRun)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.merge(path, pegRevision, revision1, revision2, localPath,
                         force, recurse, ignoreAncestry, dryRun);
        }
    }

    /**
     * @since 1.5
     */
    public void merge(String path, Revision pegRevision,
                      RevisionRange[] revisions, String localPath,
                      boolean force, int depth, boolean ignoreAncestry,
                      boolean dryRun, boolean recordOnly)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.merge(path, pegRevision, revisions, localPath, force,
                         depth, ignoreAncestry, dryRun, recordOnly);
        }
    }

    /**
     * @since 1.5
     */
    public void mergeReintegrate(String path, Revision pegRevision,
                                 String localPath, boolean dryRun)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.mergeReintegrate(path, pegRevision, localPath, dryRun);
        }
    }

    /**
     * @since 1.5
     */
    public Mergeinfo getMergeinfo(String path, Revision pegRevision)
        throws SubversionException
    {
        synchronized (clazz)
        {
            return worker.getMergeinfo(path, pegRevision);
        }
    }

    /**
     * @since 1.7
     */
    public void getMergeinfoLog(int kind, String pathOrUrl,
                                Revision pegRevision, String mergeSourceUrl,
                                Revision srcPegRevision,
                                boolean discoverChangedPaths, int depth,
                                String[] revprops, LogMessageCallback callback)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.getMergeinfoLog(kind, pathOrUrl, pegRevision, mergeSourceUrl,
                                   srcPegRevision, discoverChangedPaths, depth,
                                   revprops, callback);
        }
    }

    /**
     * @deprecated Use {@link #getMergeinfoLog(int, String, Revision, String,
     *                                         Revision, boolean, int,
     *                                         String[], LogMessageCallback)}
     * @since 1.5
     */
    public void getMergeinfoLog(int kind, String pathOrUrl,
                                Revision pegRevision, String mergeSourceUrl,
                                Revision srcPegRevision,
                                boolean discoverChangedPaths,
                                String[] revprops, LogMessageCallback callback)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.getMergeinfoLog(kind, pathOrUrl, pegRevision, mergeSourceUrl,
                                   srcPegRevision, discoverChangedPaths,
                                   revprops, callback);
        }
    }

    /**
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.0
     */
    public void diff(String target1, Revision revision1, String target2,
                     Revision revision2, String outFileName, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.diff(target1, revision1, target2, revision2, outFileName,
                        recurse);
        }
    }

    /**
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    public void diff(String target1, Revision revision1, String target2,
                     Revision revision2, String outFileName, boolean recurse,
                     boolean ignoreAncestry, boolean noDiffDeleted,
                     boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.diff(target1, revision1, target2, revision2, outFileName,
                        recurse, ignoreAncestry, noDiffDeleted, force);
        }
    }

    /**
     * @deprecated Use {@link #diff(String, Revision, String, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean, boolean)} instead.
     * @since 1.5
     */
    public void diff(String target1, Revision revision1, String target2,
                     Revision revision2, String relativeToDir,
                     String outFileName, int depth, String[] changelists,
                     boolean ignoreAncestry, boolean noDiffDeleted,
                     boolean force)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.diff(target1, revision1, target2, revision2, relativeToDir,
                        outFileName, depth, changelists, ignoreAncestry,
                        noDiffDeleted, force);
        }
    }

    /**
     * @since 1.7
     */
    public void diff(String target1, Revision revision1, String target2,
                     Revision revision2, String relativeToDir,
                     String outFileName, int depth, String[] changelists,
                     boolean ignoreAncestry, boolean noDiffDeleted,
                     boolean force, boolean copiesAsAdds)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.diff(target1, revision1, target2, revision2, relativeToDir,
                        outFileName, depth, changelists, ignoreAncestry,
                        noDiffDeleted, force, copiesAsAdds);
        }
    }

    /**
     * @deprecated Use {@link #diff(String, Revision, Revision, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean)} instead.
     * @since 1.2
     */
    public void diff(String target, Revision pegRevision,
                     Revision startRevision, Revision endRevision,
                     String outFileName, boolean recurse,
                     boolean ignoreAncestry, boolean noDiffDeleted,
                     boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.diff(target, pegRevision, startRevision, endRevision,
                        outFileName, recurse, ignoreAncestry, noDiffDeleted,
                        force);
        }
    }

    /**
     * @deprecated Use {@link #diff(String, Revision, Revision, Revision,
     *                              String, String, int, boolean, boolean,
     *                              boolean, boolean)} instead.
     * @since 1.5
     */
    public void diff(String target, Revision pegRevision,
                     Revision startRevision, Revision endRevision,
                     String relativeToDir, String outFileName, int depth,
                     String[] changelists, boolean ignoreAncestry,
                     boolean noDiffDeleted, boolean force)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.diff(target, pegRevision, startRevision, endRevision,
                        relativeToDir, outFileName, depth, changelists,
                        ignoreAncestry, noDiffDeleted, force);
        }
    }

    /**
     * @since 1.7
     */
    public void diff(String target, Revision pegRevision,
                     Revision startRevision, Revision endRevision,
                     String relativeToDir, String outFileName, int depth,
                     String[] changelists, boolean ignoreAncestry,
                     boolean noDiffDeleted, boolean force,
                     boolean copiesAsAdds)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.diff(target, pegRevision, startRevision, endRevision,
                        relativeToDir, outFileName, depth, changelists,
                        ignoreAncestry, noDiffDeleted, force, copiesAsAdds);
        }
    }

    /**
     * @since 1.5
     */
    public void diffSummarize(String target1, Revision revision1,
                              String target2, Revision revision2,
                              int depth, String[] changelists,
                              boolean ignoreAncestry,
                              DiffSummaryReceiver receiver)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.diffSummarize(target1, revision1, target2, revision2,
                                 depth, changelists, ignoreAncestry, receiver);
        }
    }

    /**
     * @since 1.5
     */
    public void diffSummarize(String target, Revision pegRevision,
                              Revision startRevision, Revision endRevision,
                              int depth, String[] changelists,
                              boolean ignoreAncestry,
                              DiffSummaryReceiver receiver)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.diffSummarize(target, pegRevision, startRevision,
                                 endRevision, depth, changelists,
                                 ignoreAncestry, receiver);
        }
    }

    /**
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.0
     */
    public PropertyData[] properties(String path) throws ClientException
    {
        synchronized(clazz)
        {
            return worker.properties(path);
        }
    }

    /**
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.2
     */
    public PropertyData[] properties(String path, Revision revision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.properties(path, revision);
        }
    }

    /**
     * @deprecated Use {@link #properties(String, Revision, Revision,
     *                                    int, ProplistCallback)} instead.
     * @since 1.2
     */
    public PropertyData[] properties(String path, Revision revision,
                                     Revision pegRevision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return properties(path, revision, pegRevision);
        }
    }

    /**
     * @since 1.5
     */
    public void properties(String path, Revision revision,
                           Revision pegRevision, int depth,
                           String[] changelists,
                           ProplistCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.properties(path, revision, pegRevision, depth, changelists,
                              callback);
        }
    }

    /**
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.0
     */
    public void propertySet(String path, String name, String value,
                            boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertySet(path, name, value, recurse);
        }
    }

    /**
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.2
     */
    public void propertySet(String path, String name, String value,
                            boolean recurse, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertySet(path, name, value, recurse, force);
        }
    }

    /**
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.0
     */
    public void propertySet(String path, String name, byte[] value,
                            boolean recurse) throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertySet(path, name, value, recurse);
        }
    }

    /**
     * @deprecated Use {@link #propertySet(String, String, String, int,
     *                                     boolean)} instead.
     * @since 1.2
     */
    public void propertySet(String path, String name, byte[] value,
                            boolean recurse, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertySet(path, name, value, recurse, force);
        }
    }

    /**
     * @since 1.5
     */
    public void propertySet(String path, String name, String value, int depth,
                            String[] changelists, boolean force,
                            Map revpropTable)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertySet(path, name, value, depth, changelists, force,
                               revpropTable);
        }
    }

    /**
     * @deprecated Use {@link #propertyRemove(String, String, int)} instead.
     * @since 1.0
     */
    public void propertyRemove(String path, String name, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyRemove(path, name, recurse);
        }
    }

    /**
     * @since 1.5
     */
    public void propertyRemove(String path, String name, int depth,
                               String[] changelists)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyRemove(path, name, depth, changelists);
        }
    }

    /**
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.0
     */
    public void propertyCreate(String path, String name, String value,
                               boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyCreate(path, name, value, recurse);
        }
    }

    /**
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.2
     */
    public void propertyCreate(String path, String name, String value,
                               boolean recurse, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyCreate(path, name, value, recurse, force);
        }
    }

    /**
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.0
     */
    public void propertyCreate(String path, String name, byte[] value,
                               boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyCreate(path, name, value, recurse);
        }
    }

    /**
     * @deprecated Use {@link #propertyCreate(String, String, String, int,
     *                                        boolean)} instead.
     * @since 1.2
     */
    public void propertyCreate(String path, String name, byte[] value,
                               boolean recurse, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyCreate(path, name, value, recurse, force);
        }
    }

    /**
     * @since 1.5
     */
    public void propertyCreate(String path, String name, String value,
                               int depth, String[] changelists, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.propertyCreate(path, name, value, depth, changelists, force);
        }
    }

    /**
     * @since 1.0
     */
    public PropertyData revProperty(String path, String name, Revision rev)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.revProperty(path, name, rev);
        }
    }

    /**
     * @since 1.2
     */
    public PropertyData[] revProperties(String path, Revision rev)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.revProperties(path, rev);
        }
    }

    /**
     * @since 1.2
     */
    public void setRevProperty(String path, String name, Revision rev,
                               String value, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.setRevProperty(path, name, rev, value, force);
        }
    }

    /**
     * @since 1.6
     */
    public void setRevProperty(String path, String name, Revision rev,
                               String value, String originalValue,
                               boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.setRevProperty(path, name, rev, value, originalValue, force);
        }
    }

    /**
     * @deprecated Use {@link #propertyGet(String, String, Revision)} instead.
     * @since 1.0
     */
    public PropertyData propertyGet(String path, String name)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.propertyGet(path, name);
        }
    }

    /**
     * @since 1.2
     */
    public PropertyData propertyGet(String path,
                                    String name,
                                    Revision revision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.propertyGet(path, name, revision);
        }
    }

    /**
     * @since 1.2
     */
    public PropertyData propertyGet(String path,
                                    String name,
                                    Revision revision,
                                    Revision pegRevision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.propertyGet(path, name, revision, pegRevision);
        }
    }

    /**
     * @deprecated Use {@link #fileContent(String, Revision, Revision)}
     *             instead.
     * @since 1.0
     */
    public byte[] fileContent(String path, Revision revision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.fileContent(path, revision);
        }
    }

    /**
     * @since 1.2
     */
    public byte[] fileContent(String path, Revision revision,
                              Revision pegRevision)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.fileContent(path, revision, pegRevision);
        }
    }

    /**
     * @since 1.0
     */
    public void streamFileContent(String path, Revision revision,
                                  Revision pegRevision, int bufferSize,
                                  OutputStream stream)
        throws ClientException
    {
        synchronized(clazz)
        {
            worker.streamFileContent(path, revision, pegRevision, bufferSize,
                                     stream);
        }
    }

    /**
     * @since 1.0
     */
    public void relocate(String from, String to, String path, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.relocate(from, to, path, recurse);
        }
    }

    /**
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.0
     */
    public byte[] blame(String path, Revision revisionStart,
                        Revision revisionEnd)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.blame(path,revisionStart, revisionEnd);
        }
    }

    /**
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.0
     */
    public void blame(String path,
                      Revision revisionStart,
                      Revision revisionEnd,
                      BlameCallback callback)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.blame(path, revisionStart, revisionEnd, callback);
        }
    }

    /**
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback2)}
     *                               instead.
     * @since 1.2
     */
    public void blame(String path,
                      Revision pegRevision,
                      Revision revisionStart,
                      Revision revisionEnd,
                      BlameCallback callback)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.blame(path, pegRevision, revisionStart, revisionEnd,
                         callback);
        }
    }

    /**
     * @deprecated Use {@link #blame(String, Revision, Revision, Revision,
     *                               boolean, boolean, BlameCallback3)}
     *                               instead.
     * @since 1.5
     */
    public void blame(String path,
                      Revision pegRevision,
                      Revision revisionStart,
                      Revision revisionEnd,
                      boolean ignoreMimeType,
                      boolean includeMergedRevisions,
                      BlameCallback2 callback)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.blame(path, pegRevision, revisionStart, revisionEnd,
                         ignoreMimeType, includeMergedRevisions, callback);
        }
    }


    /**
     * @since 1.7
     */
    public void blame(String path,
                      Revision pegRevision,
                      Revision revisionStart,
                      Revision revisionEnd,
                      boolean ignoreMimeType,
                      boolean includeMergedRevisions,
                      BlameCallback3 callback)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.blame(path, pegRevision, revisionStart, revisionEnd,
                         ignoreMimeType, includeMergedRevisions, callback);
        }
    }

    /**
     * @since 1.0
     */
    public void setConfigDirectory(String configDir) throws ClientException
    {
        synchronized(clazz)
        {
            worker.setConfigDirectory(configDir);
        }
    }

    /**
     * @since 1.0
     */
    public String getConfigDirectory() throws ClientException
    {
        synchronized(clazz)
        {
            return worker.getConfigDirectory();
        }
    }

    /**
     * @since 1.0
     */
    public void cancelOperation() throws ClientException
    {
        // this method is not synchronized, because it is designed to be called
        // from another thread
        worker.cancelOperation();
    }

    /**
     * @deprecated Use {@link #info2(String, Revision, Revision, int,
     *                               InfoCallback)} instead.
     * @since 1.0
     */
    public Info info(String path) throws ClientException
    {
        synchronized(clazz)
        {
            return worker.info(path);
        }
    }

    /**
     * @since 1.5
     */
    public void addToChangelist(String[] paths, String changelist, int depth,
                                String[] changelists)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.addToChangelist(paths, changelist, depth, changelists);
        }
    }

    /**
     * @since 1.5
     */
    public void removeFromChangelists(String[] paths, int depth,
                                      String[] changelists)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.removeFromChangelists(paths, depth, changelists);
        }
    }

    /**
     * @since 1.5
     */
    public void getChangelists(String rootPath, String[] changelists,
                               int depth, ChangelistCallback callback)
            throws ClientException
    {
        synchronized (clazz)
        {
            worker.getChangelists(rootPath, changelists, depth, callback);
        }
    }

    /**
     * @since 1.2
     */
    public void lock(String[] path, String comment, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.lock(path, comment, force);
        }
    }

    /**
     * @since 1.2
     */
    public void unlock(String[] path, boolean force)
            throws ClientException
    {
        synchronized(clazz)
        {
            worker.unlock(path, force);
        }
    }

    /**
     * @deprecated Use {@link #info2(String, Revision, Revision, int,
     *                               InfoCallback)} instead.
     * @since 1.2
     */
    public Info2[] info2(String pathOrUrl, Revision revision,
                         Revision pegRevision, boolean recurse)
            throws ClientException
    {
        synchronized(clazz)
        {
            return worker.info2(pathOrUrl, revision, pegRevision, recurse);
        }
    }

    /**
     * @since 1.5
     */
    public void info2(String pathOrUrl, Revision revision,
                      Revision pegRevision, int depth, String[] changelists,
                      InfoCallback callback)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.info2(pathOrUrl, revision, pegRevision, depth, changelists,
                         callback);
        }
    }

    /**
     * @since 1.2
     */
    public String getVersionInfo(String path, String trailUrl,
                                 boolean lastChanged) throws ClientException
    {
        synchronized(clazz)
        {
            return worker.getVersionInfo(path, trailUrl, lastChanged);
        }
    }

    /**
     * @since 1.7
     */
    public void upgrade(String path)
        throws ClientException
    {
        synchronized (clazz)
        {
            worker.upgrade(path);
        }
    }
}
