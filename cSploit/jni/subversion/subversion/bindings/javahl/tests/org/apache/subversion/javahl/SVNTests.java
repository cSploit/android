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

import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.net.URI;
import java.net.URISyntaxException;
import java.util.Set;
import java.util.HashSet;
import java.util.HashMap;
import java.util.Map;
import java.util.List;
import java.util.ArrayList;

import junit.framework.TestCase;

/**
 * common base class for the javahl binding tests
 */
class SVNTests extends TestCase
{
    /**
     * our admin object, mostly used for creating,dumping and loading
     * repositories
     */
    protected ISVNRepos admin;

    /**
     * the subversion client, what we want to test.
     */
    protected ISVNClient client;

    /**
     * The root directory for the test data. All other files and
     * directories will created under here.
     */
    protected File rootDir;

    /**
     * The Subversion file system type.
     */
    private String fsType;

    /**
     * the base name of the test. Together with the testCounter this will make
     * up the directory name of the test.
     */
    protected String testBaseName;

    /**
     * this counter will be incremented for every test in one suite
     * (test class)
     */
    protected static int testCounter;

    /**
     * the file in which the sample repository has been dumped.
     */
    protected File greekDump;

    /**
     * the directory of the sample repository.
     */
    protected File greekRepos;

    /**
     * the initial working copy of the sample repository.
     */
    protected WC greekWC;

    /**
     * the directory "local_tmp" in the rootDir.  This will be used
     * for the sample repository and its dumpfile and for the config
     * directory.
     */
    protected File localTmp;

    /**
     * the directory "repositories" in the rootDir. All test repositories will
     * be created here.
     */
    protected File repositories;

    /**
     * the directory "working_copies" in the rootDir. All test working copies
     * will be created here.
     */
    protected File workingCopies;

    /**
     * the directory "config" in the localTmp. It will be used as the
     * configuration directory for all the tests.
     */
    protected File conf;

    /**
     * standard log message. Used for all commits.
     */
    protected String logMessage = "Log Message";

    /**
     * the map of all items expected to be received by the callback for the
     * log message. After each commit, this will be cleared
     */
    protected Map<String, MyCommitItem> expectedCommitItems;

    /**
     * Common root directory for all tests. Can be set by the command
     * line or by the system property <code>test.rootdir</code>.  If
     * not set, the current working directory of this process is used.
     */
    protected static String rootDirectoryName;

    /**
     * common root URL for all tests. Can be set by the command line or by the
     * system property "test.rooturl". If not set, the file url of the
     * rootDirectoryName is used.
     */
    protected static String rootUrl;

    /**
     * Create a JUnit <code>TestCase</code> instance.
     */
    protected SVNTests()
    {
        init();
    }

    /**
     * Create a JUnit <code>TestCase</code> instance.
     */
    protected SVNTests(String name)
    {
        super(name);
        init();
    }

    private void init()
    {
        // if not already set, get a usefull value for rootDir
        if (rootDirectoryName == null)
            rootDirectoryName = System.getProperty("test.rootdir");
        if (rootDirectoryName == null)
            rootDirectoryName = System.getProperty("user.dir");
        rootDir = new File(rootDirectoryName);

        // if not already set, get a useful value for root url
        if (rootUrl == null)
            rootUrl = System.getProperty("test.rooturl");
        if (rootUrl == null || rootUrl.trim().length() == 0)
        {
            // if no root url, set build a file url
            rootUrl = rootDir.toURI().toString();
            // The JRE may have a different view about the number of
            // '/' characters to follow "file:" in a URL than
            // Subversion.  We convert to the Subversion view.
            if (rootUrl.startsWith("file:///"))
                ; // this is the form subversion needs
            else if (rootUrl.startsWith("file://"))
                rootUrl = rootUrl.replaceFirst("file://", "file:///");
            else if (rootUrl.startsWith("file:/"))
                rootUrl = rootUrl.replaceFirst("file:/", "file:///");

            // According to
            // http://java.sun.com/j2se/1.5.0/docs/api/java/io/File.html#toURL()
            // the URL from rootDir.toURI() may end with a trailing /
            // if rootDir exists and is a directory, so depending if
            // the test suite has been previously run and rootDir
            // exists, then the trailing / may or may not be there.
            // The makeReposUrl() method assumes that the rootUrl ends
            // in a trailing /, so add it now.
            if (!rootUrl.endsWith("/"))
                rootUrl = rootUrl + '/';
        }

        // Determine the Subversion file system type to use.
        if (this.fsType == null)
        {
            this.fsType =
                System.getProperty("test.fstype", ISVNRepos.FSFS).toLowerCase();
            if (!(ISVNRepos.FSFS.equals(this.fsType) ||
                  ISVNRepos.BDB.equals(this.fsType)))
            {
                this.fsType = ISVNRepos.FSFS;
            }
        }

        this.localTmp = new File(this.rootDir, "local_tmp");
        this.conf = new File(this.localTmp, "config");
        this.repositories = new File(this.rootDir, "repositories");
        this.workingCopies = new File(this.rootDir, "working_copies");
    }

    /**
     * Standard initialization of one test
     * @throws Exception
     */
    protected void setUp() throws Exception
    {
        super.setUp();

        createDirectories();

        // create and configure the needed subversion objects
        admin = new SVNRepos();
        initClient();

        // build and dump the sample repository
        File greekFiles = buildGreekFiles();
        greekRepos = new File(localTmp, "repos");
        greekDump = new File(localTmp, "greek_dump");
        admin.create(greekRepos, true,false, null, this.fsType);
        addExpectedCommitItem(greekFiles.getAbsolutePath(), null, null,
                              NodeKind.none, CommitItemStateFlags.Add);
        client.doImport(greekFiles.getAbsolutePath(),
                       makeReposUrl(greekRepos).toString(),
                        Depth.infinity, false, false, null,
                        new MyCommitMessage(), null);
        admin.dump(greekRepos, new FileOutputStream(greekDump),
                   null, null, false, false, null);
    }

    /**
     * Create a directory for the sample (Greek) repository, config
     * files, repositories and working copies.
     */
    private void createDirectories()
    {
        this.rootDir.mkdirs();

        if (this.localTmp.exists())
        {
            removeDirOrFile(this.localTmp);
        }
        this.localTmp.mkdir();
        this.conf.mkdir();

        this.repositories.mkdir();
        this.workingCopies.mkdir();
    }

    /**
     * Initialize {@link #client}, setting up its notifier, commit
     * message handler, user name, password, config directory, and
     * expected commit items.
     */
    private void initClient()
        throws SubversionException
    {
        this.client = new SVNClient();
        this.client.notification2(new MyNotifier());
        this.client.setPrompt(new DefaultPromptUserPassword());
        this.client.username("jrandom");
        this.client.setProgressCallback(new DefaultProgressListener());
        this.client.setConfigDirectory(this.conf.getAbsolutePath());
        this.expectedCommitItems = new HashMap<String, MyCommitItem>();
    }
    /**
     * the default prompt : never prompts the user, provides defaults answers
     */
    private static class DefaultPromptUserPassword implements UserPasswordCallback
    {

        public int askTrustSSLServer(String info, boolean allowPermanently)
        {
            return UserPasswordCallback.AcceptTemporary;
        }

        public String askQuestion(String realm, String question, boolean showAnswer)
        {
            return "";
        }

        public boolean askYesNo(String realm, String question, boolean yesIsDefault)
        {
            return yesIsDefault;
        }

        public String getPassword()
        {
            return "rayjandom";
        }

        public String getUsername()
        {
            return "jrandom";
        }

        public boolean prompt(String realm, String username)
        {
            return false;
        }

        public boolean prompt(String realm, String username, boolean maySave)
        {
            return false;
        }

        public String askQuestion(String realm, String question,
                boolean showAnswer, boolean maySave)
        {
            return "";
        }

        public boolean userAllowedSave()
        {
            return false;
        }
    }

    private static class DefaultProgressListener implements ProgressCallback
    {

        public void onProgress(ProgressEvent event)
        {
            // Do nothing, just receive the event
        }

    }
    /**
     * build a sample directory with test files to be used as import for
     * the sample repository. Create also the master working copy test set.
     * @return  the sample repository
     * @throws IOException
     */
    private File buildGreekFiles() throws IOException
    {
        File greekFiles = new File(localTmp, "greek_files");
        greekFiles.mkdir();
        greekWC = new WC();
        greekWC.addItem("",null);
        greekWC.addItem("iota", "This is the file 'iota'.");
        greekWC.addItem("A", null);
        greekWC.addItem("A/mu", "This is the file 'mu'.");
        greekWC.addItem("A/B", null);
        greekWC.addItem("A/B/lambda", "This is the file 'lambda'.");
        greekWC.addItem("A/B/E", null);
        greekWC.addItem("A/B/E/alpha", "This is the file 'alpha'.");
        greekWC.addItem("A/B/E/beta", "This is the file 'beta'.");
        greekWC.addItem("A/B/F", null);
        greekWC.addItem("A/C", null);
        greekWC.addItem("A/D", null);
        greekWC.addItem("A/D/gamma", "This is the file 'gamma'.");
        greekWC.addItem("A/D/H", null);
        greekWC.addItem("A/D/H/chi", "This is the file 'chi'.");
        greekWC.addItem("A/D/H/psi", "This is the file 'psi'.");
        greekWC.addItem("A/D/H/omega", "This is the file 'omega'.");
        greekWC.addItem("A/D/G", null);
        greekWC.addItem("A/D/G/pi", "This is the file 'pi'.");
        greekWC.addItem("A/D/G/rho", "This is the file 'rho'.");
        greekWC.addItem("A/D/G/tau", "This is the file 'tau'.");
        greekWC.materialize(greekFiles);
        return greekFiles;
    }

    /**
     * Remove a file or a directory and all its content.
     *
     * @param path The file or directory to be removed.
     */
    static final void removeDirOrFile(File path)
    {
        if (!path.exists())
        {
            return;
        }

        if (path.isDirectory())
        {
            // Recurse (depth-first), deleting contents.
            for (File file : path.listFiles())
            {
                removeDirOrFile(file);
            }
        }

        path.delete();
    }

    /**
     * cleanup after one test
     * @throws Exception
     */
    protected void tearDown() throws Exception
    {
        // take care of our subversion objects.
        admin.dispose();
        client.dispose();
        // remove the temporary directory
        removeDirOrFile(localTmp);
        super.tearDown();
    }

    /**
     * Create the url for the repository to be used for the tests.
     * @param file  the directory of the repository
     * @return the URL for the repository
     * @throws SubversionException
     */
    protected URI makeReposUrl(File file) throws SubversionException
    {
       try
       {
            // split the common part of the root directory
            String path = file.getAbsolutePath()
                 .substring(this.rootDir.getAbsolutePath().length() + 1);
            // append to the root url
            return new URI(rootUrl + path.replace(File.separatorChar, '/'));
       }
       catch (URISyntaxException ex)
       {
           throw new SubversionException(ex.getMessage());
       }
    }

    /**
     * add another commit item expected during the callback for the
     * log message.
     * @param workingCopyPath   the path of the of the working
     * @param baseUrl           the url for the repository
     * @param itemPath          the path of the item relative the working copy
     * @param nodeKind          expected node kind (dir or file or none)
     * @param stateFlags        expected commit state flags
     *                          (see CommitItemStateFlags)
     */
    protected void addExpectedCommitItem(String workingCopyPath,
                                         String baseUrl,
                                         String itemPath,
                                         NodeKind nodeKind,
                                         int stateFlags)
    {
        //determine the full working copy path and the full url of the item.
        String path = null;
        if (workingCopyPath != null)
            if (itemPath != null)
                path = workingCopyPath.replace(File.separatorChar, '/') +
                        '/' + itemPath;
            else
                path = workingCopyPath.replace(File.separatorChar, '/');
        String url = null;
        if (baseUrl != null)
            if (itemPath != null)
                url = baseUrl + '/' + itemPath;
            else
                url = baseUrl;

        // the key of the item is either the url or the path (if no url)
        String key;
        if (url != null)
            key = url;
        else
            key = path;
        expectedCommitItems.put(key, new MyCommitItem(path, nodeKind,
                                                      stateFlags, url));
    }

    /**
     * Intended to be called as part of test method execution
     * (post-{@link #setUp()}).  Calls <code>fail()</code> if the
     * directory name cannot be determined.
     *
     * @return The name of the working copy administrative directory.
     * @since 1.3
     */
    protected String getAdminDirectoryName() {
        String admDirName = null;
        if (this.client != null) {
            admDirName = client.getAdminDirectoryName();
        }
        if (admDirName == null) {
            fail("Unable to determine the WC admin directory name");
        }
        return admDirName;
    }

    /**
     * Represents the repository and (possibly) the working copy for
     * one test.
     */
    protected class OneTest
    {
        /**
         * the file name of repository (used by SVNAdmin)
         */
        protected File repository;

        /**
         * the file name of the working copy directory
         */
        protected File workingCopy;

        /**
         * the url of the repository (used by SVNClient)
         */
        protected URI url;

        /**
         * the expected layout of the working copy after the next subversion
         * command
         */
        protected WC wc;

        /**
         * The name of the test.
         */
        String testName;

        /**
         * Build a new test setup with a new repository.  If
         * <code>createWC</code> is <code>true</code>, create a
         * corresponding working copy and expected working copy
         * layout.
         *
         * @param createWC Whether to create the working copy on disk,
         * and initialize the expected working copy layout.
         * @param loadRepos Whether to load the sample repository, or
         * leave it with no initial revisions
         * @throws SubversionException If there is a problem
         * creating or loading the repository.
         * @throws IOException If there is a problem finding the
         * dump file.
         */
        protected OneTest(boolean createWC, boolean loadRepos)
            throws SubversionException, IOException
        {
            this.testName = testBaseName + ++testCounter;
            this.wc = greekWC.copy();
            this.repository = createInitialRepository(loadRepos);
            this.url = makeReposUrl(repository);

            if (createWC)
            {
                workingCopy = createInitialWorkingCopy(repository);
            }
        }

        /**
         * Build a new test setup with a new repository.  Create a
         * corresponding working copy and expected working copy
         * layout.
         *
         * @param createWC Whether to create the working copy on disk,
         * and initialize the expected working copy layout.
         *
         * @see #OneTest
         */
        protected OneTest(boolean createWC)
            throws SubversionException, IOException
        {
            this(createWC,true);
        }
        /**
         * Build a new test setup with a new repository.  Create a
         * corresponding working copy and expected working copy
         * layout.
         *
         * @see #OneTest
         */
        protected OneTest()
            throws SubversionException, IOException
        {
            this(true);
        }

        /**
         * Copy the working copy and the expected working copy layout for tests
         * which need multiple working copy
         * @param append    append to the working copy name of the original
         * @return second test object.
         * @throws Exception
         */
        protected OneTest copy(String append)
            throws SubversionException, IOException
        {
            return new OneTest(this, append);
        }

        /**
         * constructor for create a copy
         * @param orig      original test
         * @param append    append this to the directory name of the original
         *                  test
         * @throws Exception
         */
        private OneTest(OneTest orig, String append)
            throws SubversionException, IOException
        {
            this.testName = testBaseName + testCounter + append;
            repository = orig.getRepository();
            url = orig.getUrl();
            wc = orig.wc.copy();
            workingCopy = createInitialWorkingCopy(repository);
        }

        /**
         * Return the directory of the repository
         * @return the repository directory name
         */
        public File getRepository()
        {
            return repository;
        }

        /**
         * Return the working copy directory
         * @return the working copy directory
         */
        public File getWorkingCopy()
        {
            return workingCopy;
        }

        /**
         * Return the working copy directory name
         * @return the name of the working copy directory
         */
        public String getWCPath()
        {
            return workingCopy.getAbsolutePath();
        }

        /**
         * Return the working copy directory name as part of a Set
         * @return a Set containing only the working copy directory
         */
        public Set<String> getWCPathSet()
        {
            Set<String> paths = new HashSet<String>(1);
            paths.add(workingCopy.getAbsolutePath());
            return paths;
        }

        /**
         * Return the working copy subpath as part of a Set
         * @return a Set containing only the given working copy path
         */
        public Set<String> getWCPathSet(String subpath)
        {
            Set<String> paths = new HashSet<String>(1);
            paths.add(workingCopy.getAbsolutePath() + subpath);
            return paths;
        }

        /**
         * Returns the url of repository
         * @return  the url
         */
        public URI getUrl()
        {
            return url;
        }

        public Set<String> getUrlSet(String subpath)
        {
            Set<String> urls = new HashSet<String>(1);
            urls.add(url + subpath);
            return urls;
        }

        /**
         * Returns the expected working copy content
         * @return the expected working copy content
         */
        public WC getWc()
        {
            return wc;
        }

        /**
         * Create the repository for the beginning of the test.
         * Assumes that {@link #testName} has been set.
         *
         * @return  the repository directory
         * @throws SubversionException If there is a problem
         * creating or loading the repository.
         * @throws IOException If there is a problem finding the
         * dump file.
         */
        protected File createInitialRepository(boolean loadGreek)
            throws SubversionException, IOException
        {
            // build a clean repository directory
            File repos = new File(repositories, this.testName);
            removeDirOrFile(repos);
            // create and load the repository from the default repository dump
            admin.create(repos, true, false, conf, fsType);
            if (loadGreek)
            {
                admin.load(repos, new FileInputStream(greekDump), false, false,
                           false, false, null, null);
            }
            return repos;
        }

        /**
         * Create the working copy for the beginning of the test.
         * Assumes that {@link #testName} has been set.
         *
         * @param repos     the repository directory
         * @return the directory of the working copy
         * @throws Exception
         */
        protected File createInitialWorkingCopy(File repos)
            throws SubversionException, IOException
        {
            // build a clean working directory
            URI uri = makeReposUrl(repos);
            workingCopy = new File(workingCopies, this.testName);
            removeDirOrFile(workingCopy);
            // checkout the repository
            client.checkout(uri.toString(), workingCopy.getAbsolutePath(),
                   null, null, Depth.infinity, false, false);
            // sanity check the working with its expected status
            checkStatus();
            return workingCopy;
        }

        /**
         * Check if the working copy has the expected status.  Does
         * not extract "out of date" information from the repository.
         *
         * @throws SubversionException If there's a problem getting
         * WC status.
         * @throws IOException If there's a problem comparing the
         * WC to the expected state.
         * @see #checkStatus(boolean)
         */
        public void checkStatus()
            throws SubversionException, IOException
        {
            checkStatus(false);
        }

        /**
         * Check if the working copy has the expected status.
         *
         * @param checkRepos Whether to check the repository's "out of
         * date" information.
         * @throws SubversionException If there's a problem getting
         * WC status.
         * @throws IOException If there's a problem comparing the
         * WC to the expected state.
         */
        public void checkStatus(boolean checkRepos)
            throws SubversionException, IOException
        {
            MyStatusCallback statusCallback = new MyStatusCallback();
            client.status(workingCopy.getAbsolutePath(), Depth.unknown,
                          checkRepos, true, true, false, null, statusCallback);
            wc.check(statusCallback.getStatusArray(),
                    workingCopy.getAbsolutePath(), checkRepos);
        }

        /**
         * @return The name of this test.
         */
        public String toString()
        {
            return this.testName;
        }
    }

    /**
     * internal class to receive the request for the log messages to check if
     * the expected commit items are presented
     */
    class MyCommitMessage implements CommitMessageCallback
    {
        /**
         * Retrieve a commit message from the user based on the items
         * to be committed
         * @param elementsToBeCommitted  Array of elements to be committed
         * @return  the log message of the commit.
         */
        public String getLogMessage(Set<CommitItem> elementsToBeCommitted)
        {
            // check all received CommitItems are expected as received
            for (CommitItem commitItem : elementsToBeCommitted)
            {
                // since imports do not provide a url, the key is either url or
                // path
                String key;
                if (commitItem.getUrl() != null)
                    key = commitItem.getUrl();
                else
                    key = commitItem.getPath();

                MyCommitItem myItem = expectedCommitItems.get(key);
                // check commit item is expected and has the expected data
                assertNotNull("commit item for "+key+ " not expected", myItem);
                myItem.test(commitItem, key);
            }

            // all remaining expected commit items are missing
            for (String str : expectedCommitItems.keySet())
            {
                fail("commit item for "+str+" not found");
            }
            return logMessage;
        }
    }

    /**
     * internal class to describe an expected commit item
     */
    class MyCommitItem
    {
        /**
         * the path of the item
         */
        String myPath;
        /**
         * the kind of node (file, directory or none, see NodeKind)
         */
        NodeKind myNodeKind;
        /**
         * the reason why this item is committed (see CommitItemStateFlag)
         */
        int myStateFlags;
        /**
         * the url of the item
         */
        String myUrl;
        /**
         * build one expected commit item
         * @param path          the expected path
         * @param nodeKind      the expected node kind
         * @param stateFlags    the expected state flags
         * @param url           the expected url
         */
        private MyCommitItem(String path, NodeKind nodeKind, int stateFlags,
                             String url)
        {
            myPath = path;
            myNodeKind = nodeKind;
            myStateFlags = stateFlags;
            myUrl = url;
        }
        /**
         * Check if the commit item has the expected data
         * @param ci    the commit item to check
         * @param key   the key of the item
         */
        private void test(CommitItem ci, String key)
        {
            assertEquals("commit item path", myPath, ci.getPath());
            assertEquals("commit item node kind", myNodeKind, ci.getNodeKind());
            assertEquals("commit item state flags", myStateFlags,
                    ci.getStateFlags());
            assertEquals("commit item url", myUrl, ci.getUrl());
            // after the test, remove the item from the expected map
            expectedCommitItems.remove(key);
        }
    }

    class MyNotifier implements ClientNotifyCallback
    {

        /**
         * Handler for Subversion notifications.
         * <p/>
         * Override this function to allow Subversion to send notifications
         *
         * @param info everything to know about this event
         */
        public void onNotify(ClientNotifyInformation info)
        {
        }
    }

    private class MyStatusCallback implements StatusCallback
    {
        private List<Status> statuses = new ArrayList<Status>();

        public void doStatus(String path, Status status)
        {
            if (status != null)
                statuses.add(status);
        }

        public Status[] getStatusArray()
        {
            return (Status[]) statuses.toArray(new Status[statuses.size()]);
        }
    }
}
