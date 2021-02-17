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

package org.apache.subversion.javahl;

import org.apache.subversion.javahl.callback.*;
import org.apache.subversion.javahl.types.*;

import java.io.OutputStream;
import java.io.ByteArrayOutputStream;

import java.util.Collection;
import java.util.Set;
import java.util.List;
import java.util.Map;

/**
 * This is the main client class.  All Subversion client APIs are
 * implemented in this class.  This class is not threadsafe; if you
 * need threadsafe access, use ClientSynchronized.
 */
public class SVNClient implements ISVNClient
{
    /**
     * Load the required native library.
     */
    static
    {
        NativeResources.loadNativeLibrary();
    }

    /**
     * Standard empty contructor, builds just the native peer.
     */
    public SVNClient()
    {
        cppAddr = ctNative();

        // Ensure that Subversion's config file area and templates exist.
        try
        {
            setConfigDirectory(null);
        }
        catch (ClientException suppressed)
        {
            // Not an exception-worthy problem, continue on.
        }
    }

    private long getCppAddr()
    {
        return cppAddr;
    }

    /**
     * Build the native peer
     * @return the adress of the peer
     */
    private native long ctNative();

     /**
     * release the native peer (should not depend on finalize)
     */
    public native void dispose();

    /**
     * release the native peer (should use dispose instead)
     */
    public native void finalize();

    /**
     * slot for the adress of the native peer. The JNI code is the only user
     * of this member
     */
    protected long cppAddr;

    private ClientContext clientContext = new ClientContext();

    public Version getVersion()
    {
        return NativeResources.getVersion();
    }

    public native String getAdminDirectoryName();

    public native boolean isAdminDirectory(String name);

    /**
      * @deprecated
      */
    public native String getLastPath();

    public native void status(String path, Depth depth, boolean onServer,
                              boolean getAll, boolean noIgnore,
                              boolean ignoreExternals,
                              Collection<String> changelists,
                              StatusCallback callback)
            throws ClientException;

    public native void list(String url, Revision revision,
                            Revision pegRevision, Depth depth, int direntFields,
                            boolean fetchLocks, ListCallback callback)
            throws ClientException;

    public native void username(String username);

    public native void password(String password);

    public native void setPrompt(UserPasswordCallback prompt);

    public native void logMessages(String path, Revision pegRevision,
                                   List<RevisionRange> revisionRanges,
                                   boolean stopOnCopy, boolean discoverPath,
                                   boolean includeMergedRevisions,
                                   Set<String> revProps, long limit,
                                   LogMessageCallback callback)
            throws ClientException;

    public native long checkout(String moduleName, String destPath,
                                Revision revision, Revision pegRevision,
                                Depth depth, boolean ignoreExternals,
                                boolean allowUnverObstructions)
            throws ClientException;

    public void notification2(ClientNotifyCallback notify)
    {
        clientContext.notify = notify;
    }

    public void setConflictResolver(ConflictResolverCallback listener)
    {
        clientContext.resolver = listener;
    }

    public void setProgressCallback(ProgressCallback listener)
    {
        clientContext.listener = listener;
    }

    public native void remove(Set<String> paths, boolean force,
                              boolean keepLocal,
                              Map<String, String> revpropTable,
                              CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native void revert(String path, Depth depth,
                              Collection<String> changelists)
            throws ClientException;

    public native void add(String path, Depth depth, boolean force,
                           boolean noIgnores, boolean addParents)
        throws ClientException;

    public native long[] update(Set<String> paths, Revision revision,
                                Depth depth, boolean depthIsSticky,
                                boolean makeParents,
                                boolean ignoreExternals,
                                boolean allowUnverObstructions)
            throws ClientException;

    public native void commit(Set<String> paths, Depth depth, boolean noUnlock,
                              boolean keepChangelist,
                              Collection<String> changelists,
                              Map<String, String> revpropTable,
                              CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native void copy(List<CopySource> sources, String destPath,
                            boolean copyAsChild, boolean makeParents,
                            boolean ignoreExternals,
                            Map<String, String> revpropTable,
                            CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native void move(Set<String> srcPaths, String destPath,
                            boolean force, boolean moveAsChild,
                            boolean makeParents,
                            Map<String, String> revpropTable,
                            CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native void mkdir(Set<String> paths, boolean makeParents,
                             Map<String, String> revpropTable,
                             CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native void cleanup(String path)
            throws ClientException;

    public native void resolve(String path, Depth depth,
                               ConflictResult.Choice conflictResult)
        throws SubversionException;

    public native long doExport(String srcPath, String destPath,
                                Revision revision, Revision pegRevision,
                                boolean force, boolean ignoreExternals,
                                Depth depth, String nativeEOL)
            throws ClientException;

    public native long doSwitch(String path, String url, Revision revision,
                                Revision pegRevision, Depth depth,
                                boolean depthIsSticky, boolean ignoreExternals,
                                boolean allowUnverObstructions,
                                boolean ignoreAncestry)
            throws ClientException;

    public native void doImport(String path, String url, Depth depth,
                                boolean noIgnore,
                                boolean ignoreUnknownNodeTypes,
                                Map<String, String> revpropTable,
                                CommitMessageCallback handler, CommitCallback callback)
            throws ClientException;

    public native Set<String> suggestMergeSources(String path,
                                                  Revision pegRevision)
            throws SubversionException;

    public native void merge(String path1, Revision revision1, String path2,
                             Revision revision2, String localPath,
                             boolean force, Depth depth,
                             boolean ignoreAncestry, boolean dryRun,
                             boolean recordOnly)
            throws ClientException;

    public native void merge(String path, Revision pegRevision,
                             List<RevisionRange> revisions, String localPath,
                             boolean force, Depth depth, boolean ignoreAncestry,
                             boolean dryRun, boolean recordOnly)
            throws ClientException;

    public native void mergeReintegrate(String path, Revision pegRevision,
                                        String localPath, boolean dryRun)
            throws ClientException;

    public native Mergeinfo getMergeinfo(String path, Revision pegRevision)
            throws SubversionException;

    public native void getMergeinfoLog(Mergeinfo.LogKind kind, String pathOrUrl,
                                       Revision pegRevision,
                                       String mergeSourceUrl,
                                       Revision srcPegRevision,
                                       boolean discoverChangedPaths, Depth depth,
                                       Set<String> revProps,
                                       LogMessageCallback callback)
        throws ClientException;

    public native void diff(String target1, Revision revision1, String target2,
                            Revision revision2, String relativeToDir,
                            String outFileName, Depth depth,
                            Collection<String> changelists,
                            boolean ignoreAncestry, boolean noDiffDeleted,
                            boolean force, boolean copiesAsAdds)
            throws ClientException;

    public native void diff(String target, Revision pegRevision,
                            Revision startRevision, Revision endRevision,
                            String relativeToDir, String outFileName,
                            Depth depth, Collection<String> changelists,
                            boolean ignoreAncestry, boolean noDiffDeleted,
                            boolean force, boolean copiesAsAdds)
            throws ClientException;

    public native void diffSummarize(String target1, Revision revision1,
                                     String target2, Revision revision2,
                                     Depth depth, Collection<String> changelists,
                                     boolean ignoreAncestry,
                                     DiffSummaryCallback receiver)
            throws ClientException;

    public native void diffSummarize(String target, Revision pegRevision,
                                     Revision startRevision,
                                     Revision endRevision, Depth depth,
                                     Collection<String> changelists,
                                     boolean ignoreAncestry,
                                     DiffSummaryCallback receiver)
            throws ClientException;

    public native void properties(String path, Revision revision,
                                  Revision pegRevision, Depth depth,
                                  Collection<String> changelists,
                                  ProplistCallback callback)
            throws ClientException;

    public native void propertySetLocal(Set<String> paths, String name,
                                        byte[] value, Depth depth,
                                        Collection<String> changelists,
                                        boolean force)
            throws ClientException;

    public native void propertySetRemote(String path, long baseRev,
                                         String name, byte[] value,
                                         CommitMessageCallback handler,
                                         boolean force,
                                         Map<String, String> revpropTable,
                                         CommitCallback callback)
            throws ClientException;

    public native byte[] revProperty(String path, String name, Revision rev)
            throws ClientException;

    public native Map<String, byte[]> revProperties(String path, Revision rev)
            throws ClientException;

    public native void setRevProperty(String path, String name, Revision rev,
                                      String value, String originalValue,
                                      boolean force)
            throws ClientException;

    public native byte[] propertyGet(String path, String name,
                                     Revision revision, Revision pegRevision)
            throws ClientException;

    public byte[] fileContent(String path, Revision revision,
                              Revision pegRevision)
            throws ClientException
    {
        ByteArrayOutputStream stream = new ByteArrayOutputStream();

        streamFileContent(path, revision, pegRevision, stream);
        return stream.toByteArray();
    }

    public native void streamFileContent(String path, Revision revision,
                                         Revision pegRevision,
                                         OutputStream stream)
            throws ClientException;

    public native void relocate(String from, String to, String path,
                                boolean ignoreExternals)
            throws ClientException;

    public native void blame(String path, Revision pegRevision,
                             Revision revisionStart,
                             Revision revisionEnd, boolean ignoreMimeType,
                             boolean includeMergedRevisions,
                             BlameCallback callback)
            throws ClientException;

    public native void setConfigDirectory(String configDir)
            throws ClientException;

    public native String getConfigDirectory()
            throws ClientException;

    public native void cancelOperation()
            throws ClientException;

    public native void addToChangelist(Set<String> paths, String changelist,
                                       Depth depth,
                                       Collection<String> changelists)
            throws ClientException;

    public native void removeFromChangelists(Set<String> paths, Depth depth,
                                             Collection<String> changelists)
            throws ClientException;

    public native void getChangelists(String rootPath,
                                      Collection<String> changelists,
                                      Depth depth, ChangelistCallback callback)
            throws ClientException;

    public native String getVersionInfo(String path, String trailUrl,
                                        boolean lastChanged)
            throws ClientException;

    public native void upgrade(String path)
            throws ClientException;

    /**
     * Enable logging in the JNI-code
     * @param logLevel      the level of information to log (See
     *                      ClientLogLevel)
     * @param logFilePath   path of the log file
     */
    public static native void enableLogging(ClientLogLevel logLevel,
                                            String logFilePath);

    /**
     * enum for the constants of the logging levels.
     */
    public enum ClientLogLevel
    {
        /** Log nothing */
        NoLog,

        /** Log fatal error */
        ErrorLog,

        /** Log exceptions thrown */
        ExceptionLog,

        /** Log the entry and exits of the JNI code */
        EntryLog;
    }

    /**
     * Returns version information of subversion and the javahl binding
     * @return version information
     */
    public static native String version();

    /**
     * Returns the major version of the javahl binding. Same version of the
     * javahl support the same interfaces
     * @return major version number
     */
    public static native int versionMajor();

    /**
     * Returns the minor version of the javahl binding. Same version of the
     * javahl support the same interfaces
     * @return minor version number
     */
    public static native int versionMinor();

    /**
     * Returns the micro (patch) version of the javahl binding. Same version of
     * the javahl support the same interfaces
     * @return micro version number
     */
    public static native int versionMicro();

    public native void lock(Set<String> paths, String comment, boolean force)
            throws ClientException;

    public native void unlock(Set<String> paths, boolean force)
            throws ClientException;

    public native void info2(String pathOrUrl, Revision revision,
                             Revision pegRevision, Depth depth,
                             Collection<String> changelists,
                             InfoCallback callback)
            throws ClientException;

    public native void patch(String patchPath, String targetPath,
                             boolean dryRun, int stripCount, boolean reverse,
                             boolean ignoreWhitespace, boolean removeTempfiles,
                             PatchCallback callback)
            throws ClientException;

    /**
     * A private class to hold the contextual information required to
     * persist in this object, such as notification handlers.
     */
    private class ClientContext
        implements ClientNotifyCallback, ProgressCallback,
            ConflictResolverCallback
    {
        public ClientNotifyCallback notify = null;
        public ProgressCallback listener = null;
        public ConflictResolverCallback resolver = null;

        public void onNotify(ClientNotifyInformation notifyInfo)
        {
            if (notify != null)
                notify.onNotify(notifyInfo);
        }

        public void onProgress(ProgressEvent event)
        {
            if (listener != null)
                listener.onProgress(event);
        }

        public ConflictResult resolve(ConflictDescriptor conflict)
            throws SubversionException
        {
            if (resolver != null)
                return resolver.resolve(conflict);
            else
                return new ConflictResult(ConflictResult.Choice.postpone,
                                          null);
        }
    }
}
