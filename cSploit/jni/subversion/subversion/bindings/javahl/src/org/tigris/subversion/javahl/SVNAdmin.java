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

import java.util.Set;
import java.io.OutputStream;
import java.io.InputStream;
import java.io.File;
import java.io.IOException;

/**
 * This class offers the same commands as the svnadmin commandline
 * client.
 */
public class SVNAdmin
{
    private org.apache.subversion.javahl.SVNRepos aSVNAdmin;

    /**
     * Standard empty contructor, builds just the native peer.
     */
    public SVNAdmin()
    {
        aSVNAdmin = new org.apache.subversion.javahl.SVNRepos();
        cppAddr = aSVNAdmin.getCppAddr();
    }

    /**
     * release the native peer (should not depend on finalize)
     */
    public void dispose()
    {
        aSVNAdmin.dispose();
    }

    /**
     * release the native peer (should use dispose instead)
     */
    protected void finalize()
    {
    }

    /**
     * slot for the adress of the native peer. The JNI code is the only user
     * of this member
     */
    protected long cppAddr;

    /**
     * Filesystem in a Berkeley DB
     */
    public static final String BDB = "bdb";

    /**
     * Filesystem in the filesystem
     */
    public static final String FSFS = "fsfs";

    /**
     * @return Version information about the underlying native libraries.
     */
    public Version getVersion()
    {
        return new Version(
                    org.apache.subversion.javahl.NativeResources.getVersion());
    }

    /**
     * create a subversion repository.
     * @param path                  the path where the repository will been
     *                              created.
     * @param disableFsyncCommit    disable to fsync at the commit (BDB).
     * @param keepLog               keep the log files (BDB).
     * @param configPath            optional path for user configuration files.
     * @param fstype                the type of the filesystem (BDB or FSFS)
     * @throws ClientException  throw in case of problem
     */
    public void create(String path, boolean disableFsyncCommit, boolean keepLog,
                       String configPath, String fstype)
            throws ClientException
    {
        try {
            aSVNAdmin.create(new File(path), disableFsyncCommit, keepLog,
                             configPath == null ? null : new File(configPath),
                             fstype);
        } catch (org.apache.subversion.javahl.ClientException ex) {
            throw new ClientException(ex);
        }
    }

    /**
     * deltify the revisions in the repository
     * @param path              the path to the repository
     * @param start             start revision
     * @param end               end revision
     * @throws ClientException  throw in case of problem
     */
    public void deltify(String path, Revision start, Revision end)
            throws ClientException
    {
        try
        {
            aSVNAdmin.deltify(new File(path),
                              start == null ? null : start.toApache(),
                              end == null ? null : end.toApache());
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * dump the data in a repository
     * @param path              the path to the repository
     * @param dataOut           the data will be outputed here
     * @param errorOut          the messages will be outputed here
     * @param start             the first revision to be dumped
     * @param end               the last revision to be dumped
     * @param incremental       the dump will be incremantal
     * @throws ClientException  throw in case of problem
     */
    public void dump(String path, OutputInterface dataOut,
                     OutputInterface errorOut, Revision start,
                     Revision end, boolean incremental)
            throws ClientException
    {
        dump(path, dataOut, errorOut, start, end, incremental, false);
    }

    /**
     * dump the data in a repository
     * @param path              the path to the repository
     * @param dataOut           the data will be outputed here
     * @param errorOut          the messages will be outputed here
     * @param start             the first revision to be dumped
     * @param end               the last revision to be dumped
     * @param incremental       the dump will be incremantal
     * @param useDeltas         the dump will contain deltas between nodes
     * @throws ClientException  throw in case of problem
     * @since 1.5
     */
    public void dump(String path, OutputInterface dataOut,
                     OutputInterface errorOut, Revision start, Revision end,
                     boolean incremental, boolean useDeltas)
            throws ClientException
    {
        try
        {
            aSVNAdmin.dump(new File(path), new OutputWrapper(dataOut),
                           start == null ? null : start.toApache(),
                           end == null ? null : end.toApache(),
                           incremental, useDeltas,
                           new ReposNotifyHandler(errorOut));
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * make a hot copy of the repository
     * @param path              the path to the source repository
     * @param targetPath        the path to the target repository
     * @param cleanLogs         clean the unused log files in the source
     *                          repository
     * @throws ClientException  throw in case of problem
     */
    public void hotcopy(String path, String targetPath, boolean cleanLogs)
            throws ClientException
    {
        try
        {
            aSVNAdmin.hotcopy(new File(path), new File(targetPath), cleanLogs);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * list all logfiles (BDB) in use or not)
     * @param path              the path to the repository
     * @param receiver          interface to receive the logfile names
     * @throws ClientException  throw in case of problem
     */
    public void listDBLogs(String path, MessageReceiver receiver)
            throws ClientException
    {
        try
        {
            aSVNAdmin.listDBLogs(new File(path), receiver);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * list unused logfiles
     * @param path              the path to the repository
     * @param receiver          interface to receive the logfile names
     * @throws ClientException  throw in case of problem
     */
    public void listUnusedDBLogs(String path, MessageReceiver receiver)
            throws ClientException
    {
        try
        {
            aSVNAdmin.listUnusedDBLogs(new File(path), receiver);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * interface to receive the messages
     */
    public static interface MessageReceiver
        extends org.apache.subversion.javahl.ISVNRepos.MessageReceiver
    {
    }

    /**
     * load the data of a dump into a repository,
     * @param path              the path to the repository
     * @param dataInput         the data input source
     * @param messageOutput     the target for processing messages
     * @param ignoreUUID        ignore any UUID found in the input stream
     * @param forceUUID         set the repository UUID to any found in the
     *                          stream
     * @param relativePath      the directory in the repository, where the data
     *                          in put optional.
     * @throws ClientException  throw in case of problem
     */
    public void load(String path, InputInterface dataInput,
                     OutputInterface messageOutput, boolean ignoreUUID,
                     boolean forceUUID, String relativePath)
            throws ClientException
    {
        load(path, dataInput, messageOutput, ignoreUUID, forceUUID,
             false, false, relativePath);
    }

    /**
     * load the data of a dump into a repository,
     * @param path              the path to the repository
     * @param dataInput         the data input source
     * @param messageOutput     the target for processing messages
     * @param ignoreUUID        ignore any UUID found in the input stream
     * @param forceUUID         set the repository UUID to any found in the
     *                          stream
     * @param usePreCommitHook  use the pre-commit hook when processing commits
     * @param usePostCommitHook use the post-commit hook when processing commits
     * @param relativePath      the directory in the repository, where the data
     *                          in put optional.
     * @throws ClientException  throw in case of problem
     * @since 1.5
     */
    public void load(String path, InputInterface dataInput,
                     OutputInterface messageOutput, boolean ignoreUUID,
                     boolean forceUUID, boolean usePreCommitHook,
                     boolean usePostCommitHook, String relativePath)
            throws ClientException
    {
        try
        {
            aSVNAdmin.load(new File(path), new InputWrapper(dataInput),
                           ignoreUUID, forceUUID, usePreCommitHook,
                           usePostCommitHook, relativePath,
                           new ReposNotifyHandler(messageOutput));
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * list all open transactions in a repository
     * @param path              the path to the repository
     * @param receiver          receives one transaction name per call
     * @throws ClientException  throw in case of problem
     */
    public void lstxns(String path, MessageReceiver receiver)
            throws ClientException
    {
        try
        {
            aSVNAdmin.lstxns(new File(path), receiver);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * recover the berkeley db of a repository, returns youngest revision
     * @param path              the path to the repository
     * @throws ClientException  throw in case of problem
     */
    public long recover(String path)
            throws ClientException
    {
        try
        {
            return aSVNAdmin.recover(new File(path), null);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * remove open transaction in a repository
     * @param path              the path to the repository
     * @param transactions      the transactions to be removed
     * @throws ClientException  throw in case of problem
     */
    public void rmtxns(String path, String [] transactions)
            throws ClientException
    {
        try
        {
            aSVNAdmin.rmtxns(new File(path), transactions);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * set the log message of a revision
     * @param path              the path to the repository
     * @param rev               the revision to be changed
     * @param message           the message to be set
     * @param bypassHooks       if to bypass all repository hooks
     * @throws ClientException  throw in case of problem
     * @deprecated Use setRevProp() instead.
     */
    public void setLog(String path, Revision rev, String message,
                       boolean bypassHooks)
            throws ClientException
    {
        try
        {
            aSVNAdmin.setRevProp(new File(path),
                                 rev == null ? null : rev.toApache(),
                                 "svn:log", message,
                                 !bypassHooks, !bypassHooks);
        }
        catch (org.apache.subversion.javahl.SubversionException ex)
        {
            throw ClientException.fromException(ex);
        }
    }

    /**
     * Change the value of the revision property <code>propName</code>
     * to <code>propValue</code>.  By default, does not run
     * pre-/post-revprop-change hook scripts.
     *
     * @param path The path to the repository.
     * @param rev The revision for which to change a property value.
     * @param propName The name of the property to change.
     * @param propValue The new value to set for the property.
     * @param usePreRevPropChangeHook Whether to run the
     * <i>pre-revprop-change</i> hook script.
     * @param usePostRevPropChangeHook Whether to run the
     * <i>post-revprop-change</i> hook script.
     * @throws SubversionException If a problem occurs.
     * @since 1.5.0
     */
    public void setRevProp(String path, Revision rev, String propName,
                           String propValue, boolean usePreRevPropChangeHook,
                           boolean usePostRevPropChangeHook)
            throws SubversionException
    {
        try
        {
            aSVNAdmin.setRevProp(new File(path),
                                 rev == null ? null : rev.toApache(),
                                 propName, propValue,
                                 usePreRevPropChangeHook,
                                 usePostRevPropChangeHook);
        }
        catch (org.apache.subversion.javahl.SubversionException ex)
        {
            throw new SubversionException(ex);
        }
    }

    /**
     * Verify the repository at <code>path</code> between revisions
     * <code>start</code> and <code>end</code>.
     *
     * @param path              the path to the repository
     * @param messageOut        the receiver of all messages
     * @param start             the first revision
     * @param end               the last revision
     * @throws ClientException If an error occurred.
     */
    public void verify(String path, OutputInterface messageOut,
                       Revision start, Revision end)
            throws ClientException
    {
        try
        {
            aSVNAdmin.verify(new File(path),
                             start == null ? null : start.toApache(),
                             end == null ? null : end.toApache(),
                             new ReposNotifyHandler(messageOut));
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * list all locks in the repository
     * @param path              the path to the repository
     * @throws ClientException  throw in case of problem
     * @since 1.2
     */
    public Lock[] lslocks(String path)
            throws ClientException
    {
        try
        {
            Set<org.apache.subversion.javahl.types.Lock> aLocks =
                                                    aSVNAdmin.lslocks(
                                                        new File(path),
                                                        Depth.toADepth(
                                                              Depth.infinity));
            Lock[] locks = new Lock[aLocks.size()];

            int i = 0;
            for (org.apache.subversion.javahl.types.Lock lock : aLocks)
            {
                locks[i] = new Lock(lock);
                i++;
            }

            return locks;
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    /**
     * remove multiple locks from the repository
     * @param path              the path to the repository
     * @param locks             the name of the locked items
     * @throws ClientException  throw in case of problem
     * @since 1.2
     */
    public void rmlocks(String path, String [] locks)
            throws ClientException
    {
        try
        {
            aSVNAdmin.rmlocks(new File(path), locks);
        }
        catch (org.apache.subversion.javahl.ClientException ex)
        {
            throw new ClientException(ex);
        }
    }

    private class OutputWrapper extends OutputStream
    {
        private OutputInterface outputer;

        OutputWrapper(OutputInterface outputer)
        {
            this.outputer = outputer;
        }

        public void write(int b) throws IOException
        {
            outputer.write(new byte[]{ (byte) ( b & 0xFF) });
        }

        public void write(byte[] b) throws IOException
        {
            outputer.write(b);
        }

        public void close() throws IOException
        {
            outputer.close();
        }
    }

    private class InputWrapper extends InputStream
    {
        private InputInterface inputer;

        InputWrapper(InputInterface inputer)
        {
            this.inputer = inputer;
        }

        public int read() throws IOException
        {
            byte[] b = new byte[1];
            if (inputer.read(b) > 0)
                return b[0];
            else
                return -1;
        }

        public int read(byte[] b) throws IOException
        {
            return inputer.read(b);
        }

        public void close() throws IOException
        {
            inputer.close();
        }
    }

    private class ReposNotifyHandler
        implements org.apache.subversion.javahl.callback.ReposNotifyCallback
    {
        private OutputInterface outputer;

        public ReposNotifyHandler(OutputInterface outputer)
        {
            this.outputer = outputer;
        }

        public void onNotify(org.apache.subversion.javahl.ReposNotifyInformation
                                                                           info)
        {
            String val;

            switch (info.getAction())
            {
                case warning:
                    val = info.getWarning();
                    break;

                case dump_rev_end:
                    val = "* Dumped revision " + info.getRevision() + ".\n";
                    break;

                case verify_rev_end:
                    val = "* Verified revision " + info.getRevision() + ".\n";
                    break;

                case load_txn_committed:
                    if (info.getOldRevision() == Revision.SVN_INVALID_REVNUM)
                        val = "\n------- Committed revision " +
                              info.getNewRevision() + " >>>\n\n";
                    else
                        val = "\n------- Committed new rev " +
                               info.getNewRevision() +
                              " (loaded from original rev " +
                              info.getOldRevision() +
                              ") >>>\n\n";
                    break;

                case load_node_start:
                    switch (info.getNodeAction())
                    {
                        case change:
                            val = "     * editing path : " + info.getPath() +
                                  " ...";
                            break;

                        case deleted:
                            val = "     * deleting path : " + info.getPath() +
                                  " ...";
                            break;

                        case add:
                            val = "     * adding path : " + info.getPath() +
                                  " ...";
                            break;

                        case replace:
                            val = "     * replacing path : " + info.getPath() +
                                  " ...";
                            break;

                        default:
                            val = null;
                    }
                    break;

                case load_node_done:
                    val = " done.\n";
                    break;

                case load_copied_node:
                    val = "COPIED...";
                    break;

                case load_txn_start:
                    val = "<<< Started new transaction, based on " +
                          "original revision " + info.getOldRevision() + "\n";
                    break;

                case load_normalized_mergeinfo:
                    val = " removing '\\r' from svn:mergeinfo ...";
                    break;

                default:
                    val = null;
            }

            if (val != null)
                try
                {
                    outputer.write(val.getBytes());
                }
                catch (IOException ex)
                {
                    ; // ignore
                }
        }
    }
}
