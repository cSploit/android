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

import java.io.File;
import java.io.FileOutputStream;
import java.io.FileNotFoundException;
import java.io.FileReader;
import java.io.IOException;
import java.io.PrintWriter;
import java.io.ByteArrayOutputStream;
import java.text.ParseException;
import java.util.Arrays;
import java.util.ArrayList;
import java.util.HashMap;
import java.util.Iterator;
import java.util.List;
import java.util.Set;
import java.util.Map;


/**
 * Tests the basic functionality of javahl binding (inspired by the
 * tests in subversion/tests/cmdline/basic_tests.py).
 */
public class BasicTests extends SVNTests
{
    /**
     * Base name of all our tests.
     */
    public final static String testName = "basic_test";

    public BasicTests()
    {
        init();
    }

    public BasicTests(String name)
    {
        super(name);
        init();
    }

    /**
     * Initialize the testBaseName and the testCounter, if this is the
     * first test of this class.
     */
    private void init()
    {
        if (!testName.equals(testBaseName))
        {
            testCounter = 0;
            testBaseName = testName;
        }
    }

    /**
     * Test LogDate().
     * @throws Throwable
     */
    public void testLogDate() throws Throwable
    {
        String goodDate = "2007-10-04T03:00:52.134992Z";
        String badDate = "2008-01-14";
        LogDate logDate;

        try
        {
            logDate = new LogDate(goodDate);
            assertEquals(1191466852134992L, logDate.getTimeMicros());
        } catch (ParseException e) {
            fail("Failed to parse date " + goodDate);
        }

        try
        {
            logDate = new LogDate(badDate);
            fail("Failed to throw exception on bad date " + badDate);
        } catch (ParseException e) {
        }
    }

    /**
     * Test SVNClient.getVersion().
     * @throws Throwable
     */
    public void testVersion() throws Throwable
    {
        try
        {
            Version version = client.getVersion();
            String versionString = version.toString();
            if (versionString == null || versionString.trim().length() == 0)
            {
                throw new Exception("Version string empty");
            }
        }
        catch (Exception e)
        {
            fail("Version should always be available unless the " +
                 "native libraries failed to initialize: " + e);
        }
    }

    /**
     * Tests Subversion path validation.
     */
    public void testPathValidation() throws Throwable
    {
        // Rather than segfaulting, JavaHL considers null an invalid path.
        assertFalse("Path validation produced false-positive for null path",
                    Path.isValid(null));

        String path = "valid-path";
        assertTrue("Validation check of valid path '" + path +
                   "' should succeed", Path.isValid(path));

        // File names cannot contain control characters.
        path = "invalid-\u0001-path";
        assertFalse("Validation check of invalid path '" + path +
                    "' (which contains control characters) should fail",
                    Path.isValid(path));
    }

    /**
     * Tests Subversion path as URL predicate.
     */
    public void testPathIsURL() throws Throwable
    {
        try
        {
            Path.isURL(null);
            fail("A null path should raise an exception");
        }
        catch (IllegalArgumentException expected)
        {
        }

        // Subversion "paths" which aren't URLs.
        String[] paths = { "/path", "c:\\path" };
        for (int i = 0; i < paths.length; i++)
        {
            assertFalse("'" + paths[i] + "' should not be considered a URL",
                        Path.isURL(paths[i]));
        }

        // Subversion "paths" which are URLs.
        paths = new String[] { "http://example.com", "svn://example.com",
                               "svn+ssh://example.com", "file:///src/svn/" };
        for (int i = 0; i < paths.length; i++)
        {
            assertTrue("'" + paths[i] + "' should be considered a URL",
                       Path.isURL(paths[i]));
        }
    }

    /**
     * Tests Mergeinfo and RevisionRange classes.
     * @since 1.5
     */
    public void testMergeinfoParser() throws Throwable
    {
        String mergeInfoPropertyValue =
            "/trunk:1-300,305,307,400-405\n/branches/branch:308-400";
        Mergeinfo info = new Mergeinfo(mergeInfoPropertyValue);
        String[] paths = info.getPaths();
        assertEquals(2, paths.length);
        RevisionRange[] trunkRange = info.getRevisionRange("/trunk");
        assertEquals(4, trunkRange.length);
        assertEquals("1-300", trunkRange[0].toString());
        assertEquals("305", trunkRange[1].toString());
        assertEquals("307", trunkRange[2].toString());
        assertEquals("400-405", trunkRange[3].toString());
        RevisionRange[] branchRange =
            info.getRevisionRange("/branches/branch");
        assertEquals(1, branchRange.length);
    }

    /**
     * Test the basic SVNClient.status functionality.
     * @throws Throwable
     */
    public void testBasicStatus() throws Throwable
    {
        // build the test setup
        OneTest thisTest = new OneTest();

        // check the status of the working copy
        thisTest.checkStatus();

        // Test status of non-existent file
        File fileC = new File(thisTest.getWorkingCopy() + "/A", "foo.c");

        Status s = client.singleStatus(fileToSVNPath(fileC, false), false);
        if (s != null)
            fail("File foo.c should not return a status.");

    }

    /**
     * Test the "out of date" info from {@link
     * org.tigris.subversion.javahl.SVNClient#status()}.
     *
     * @throws SubversionException
     * @throws IOException
     */
    public void testOODStatus() throws SubversionException, IOException
    {
        // build the test setup
        OneTest thisTest = new OneTest();

        // Make a whole slew of changes to a WC:
        //
        //  (root)               r7 - prop change
        //  iota
        //  A
        //  |__mu
        //  |
        //  |__B
        //  |   |__lambda
        //  |   |
        //  |   |__E             r12 - deleted
        //  |   |  |__alpha
        //  |   |  |__beta
        //  |   |
        //  |   |__F             r9 - prop change
        //  |   |__I             r6 - added dir
        //  |
        //  |__C                 r5 - deleted
        //  |
        //  |__D
        //     |__gamma
        //     |
        //     |__G
        //     |  |__pi          r3 - deleted
        //     |  |__rho         r2 - modify text
        //     |  |__tau         r4 - modify text
        //     |
        //     |__H
        //        |__chi         r10-11 replaced with file
        //        |__psi         r13-14 replaced with dir
        //        |__omega
        //        |__nu          r8 - added file
        File file, dir;
        PrintWriter pw;
        Status status;
        long rev;             // Resulting rev from co or update
        long expectedRev = 2;  // Keeps track of the latest rev committed

        // ----- r2: modify file A/D/G/rho --------------------------
        file = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("modification to rho");
        pw.close();
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[]{thisTest.getWCPath()},
                                         "log msg", true), expectedRev++);
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", rev);
        thisTest.getWc().setItemContent("A/D/G/rho",
            thisTest.getWc().getItemContent("A/D/G/rho")
            + "modification to rho");

        status = client.singleStatus(thisTest.getWCPath() + "/A/D/G/rho",
                                     false);
        long rhoCommitDate = status.getLastChangedDate().getTime();
        long rhoCommitRev = rev;
        String rhoAuthor = status.getLastCommitAuthor();

        // ----- r3: delete file A/D/G/pi ---------------------------
        client.remove(new String[] {thisTest.getWCPath() + "/A/D/G/pi"}, null,
                      false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/G/pi", NodeKind.file,
                              CommitItemStateFlags.Delete);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().removeItem("A/D/G/pi");

        thisTest.getWc().setItemWorkingCopyRevision("A/D/G", rev);
        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath() + "/A/D/G",
                                   null, true),
                     rev);
        long GCommitRev = rev;

        // ----- r4: modify file A/D/G/tau --------------------------
        file = new File(thisTest.getWorkingCopy(), "A/D/G/tau");
        pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("modification to tau");
        pw.close();
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/G/tau",NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/tau", rev);
        thisTest.getWc().setItemContent("A/D/G/tau",
                thisTest.getWc().getItemContent("A/D/G/tau")
                + "modification to tau");
        status = client.singleStatus(thisTest.getWCPath() + "/A/D/G/tau",
                                     false);
        long tauCommitDate = status.getLastChangedDate().getTime();
        long tauCommitRev = rev;
        String tauAuthor = status.getLastCommitAuthor();

        // ----- r5: delete dir with no children  A/C ---------------
        client.remove(new String[] {thisTest.getWCPath() + "/A/C"}, null,
                      false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/C", NodeKind.dir,
                              CommitItemStateFlags.Delete);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().removeItem("A/C");
        long CCommitRev = rev;

        // ----- r6: Add dir A/B/I ----------------------------------
        dir = new File(thisTest.getWorkingCopy(), "A/B/I");
        dir.mkdir();

        client.add(dir.getAbsolutePath(), true);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/B/I", NodeKind.dir, CommitItemStateFlags.Add);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg",  true),
                     expectedRev++);
        thisTest.getWc().addItem("A/B/I", null);
        status = client.singleStatus(thisTest.getWCPath() + "/A/B/I", false);
        long ICommitDate = status.getLastChangedDate().getTime();
        long ICommitRev = rev;
        String IAuthor = status.getLastCommitAuthor();

        // ----- r7: Update then commit prop change on root dir -----
        thisTest.getWc().setRevision(rev);
        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath(), null, true), rev);
        thisTest.checkStatus();
        client.propertySet(thisTest.getWCPath(), "propname", "propval", false);
        thisTest.getWc().setItemPropStatus("", Status.Kind.modified);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(), null,
                              NodeKind.dir, CommitItemStateFlags.PropMods);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().setItemWorkingCopyRevision("", rev);
        thisTest.getWc().setItemPropStatus("", Status.Kind.normal);

        // ----- r8: Add a file A/D/H/nu ----------------------------
        file = new File(thisTest.getWorkingCopy(), "A/D/H/nu");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the file 'nu'.");
        pw.close();
        client.add(file.getAbsolutePath(), false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/nu", NodeKind.file,
                              CommitItemStateFlags.TextMods +
                              CommitItemStateFlags.Add);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().addItem("A/D/H/nu", "This is the file 'nu'.");
        status = client.singleStatus(thisTest.getWCPath() + "/A/D/H/nu",
                                     false);
        long nuCommitDate = status.getLastChangedDate().getTime();
        long nuCommitRev = rev;
        String nuAuthor = status.getLastCommitAuthor();

        // ----- r9: Prop change on A/B/F ---------------------------
        client.propertySet(thisTest.getWCPath() + "/A/B/F", "propname",
                           "propval", false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/B/F", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().setItemPropStatus("A/B/F", Status.Kind.normal);
        thisTest.getWc().setItemWorkingCopyRevision("A/B/F", rev);
        status = client.singleStatus(thisTest.getWCPath() + "/A/B/F", false);
        long FCommitDate = status.getLastChangedDate().getTime();
        long FCommitRev = rev;
        String FAuthor = status.getLastCommitAuthor();

        // ----- r10-11: Replace file A/D/H/chi with file -----------
        client.remove(new String[] {thisTest.getWCPath() + "/A/D/H/chi"},
                      null, false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/chi", NodeKind.file,
                              CommitItemStateFlags.Delete);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().removeItem("A/D/G/pi");

        file = new File(thisTest.getWorkingCopy(), "A/D/H/chi");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the replacement file 'chi'.");
        pw.close();
        client.add(file.getAbsolutePath(), false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/chi", NodeKind.file,
                              CommitItemStateFlags.TextMods +
                              CommitItemStateFlags.Add);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().addItem("A/D/H/chi",
                                 "This is the replacement file 'chi'.");
        status = client.singleStatus(thisTest.getWCPath() + "/A/D/H/chi",
                                     false);
        long chiCommitDate = status.getLastChangedDate().getTime();
        long chiCommitRev = rev;
        String chiAuthor = status.getLastCommitAuthor();

        // ----- r12: Delete dir A/B/E with children ----------------
        client.remove(new String[] {thisTest.getWCPath() + "/A/B/E"}, null,
                      false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/B/E", NodeKind.dir,
                              CommitItemStateFlags.Delete);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().removeItem("A/B/E/alpha");
        thisTest.getWc().removeItem("A/B/E/beta");
        thisTest.getWc().removeItem("A/B/E");

        thisTest.getWc().setItemWorkingCopyRevision("A/B", rev);
        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath() + "/A/B", null, true),
                     rev);
        Info Binfo = client.info(thisTest.getWCPath() + "/A/B");
        long BCommitDate = Binfo.getLastChangedDate().getTime();
        long BCommitRev = rev;
        long ECommitRev = BCommitRev;
        String BAuthor = Binfo.getAuthor();

        // ----- r13-14: Replace file A/D/H/psi with dir ------------
        client.remove(new String[]{thisTest.getWCPath() + "/A/D/H/psi"}, null,
                      false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/psi", NodeKind.file,
                              CommitItemStateFlags.Delete);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        thisTest.getWc().removeItem("A/D/H/psi");
        thisTest.getWc().setRevision(rev);
        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath(), null, true), rev);
        thisTest.getWc().addItem("A/D/H/psi", null);
        dir = new File(thisTest.getWorkingCopy(), "A/D/H/psi");
        dir.mkdir();
        client.add(dir.getAbsolutePath(), true);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/psi", NodeKind.dir,
                              CommitItemStateFlags.Add);
        assertEquals("wrong revision number from commit",
                     rev = client.commit(new String[] {thisTest.getWCPath()},
                                         "log msg", true),
                     expectedRev++);
        status = client.singleStatus(thisTest.getWCPath() + "/A/D/H/psi",
                                     false);
        long psiCommitDate = status.getLastChangedDate().getTime();
        long psiCommitRev = rev;
        String psiAuthor = status.getLastCommitAuthor();

        // ----- Check status of modfied WC then update it back
        // -----  to rev 1 so it's out of date
        thisTest.checkStatus();

        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath(),
                                   Revision.getInstance(1), true),
                     1);
        thisTest.getWc().setRevision(1);

        thisTest.getWc().setItemOODInfo("A", psiCommitRev, psiAuthor,
                                        psiCommitDate, NodeKind.dir);

        thisTest.getWc().setItemOODInfo("A/B", BCommitRev, BAuthor,
                                        BCommitDate, NodeKind.dir);

        thisTest.getWc().addItem("A/B/I", null);
        thisTest.getWc().setItemOODInfo("A/B/I", ICommitRev, IAuthor,
                                        ICommitDate, NodeKind.dir);
        thisTest.getWc().setItemTextStatus("A/B/I", Status.Kind.none);
        thisTest.getWc().setItemNodeKind("A/B/I", NodeKind.unknown);

        thisTest.getWc().addItem("A/C", null);
        thisTest.getWc().setItemReposLastCmtRevision("A/C", CCommitRev);
        thisTest.getWc().setItemReposKind("A/C", NodeKind.dir);

        thisTest.getWc().addItem("A/B/E", null);
        thisTest.getWc().setItemReposLastCmtRevision("A/B/E", ECommitRev);
        thisTest.getWc().setItemReposKind("A/B/E", NodeKind.dir);
        thisTest.getWc().addItem("A/B/E/alpha", "This is the file 'alpha'.");
        thisTest.getWc().addItem("A/B/E/beta", "This is the file 'beta'.");

        thisTest.getWc().setItemPropStatus("A/B/F", Status.Kind.none);
        thisTest.getWc().setItemOODInfo("A/B/F", FCommitRev, FAuthor,
                                        FCommitDate, NodeKind.dir);

        thisTest.getWc().setItemOODInfo("A/D", psiCommitRev, psiAuthor,
                                        psiCommitDate, NodeKind.dir);

        thisTest.getWc().setItemOODInfo("A/D/G", tauCommitRev, tauAuthor,
                                        tauCommitDate, NodeKind.dir);

        thisTest.getWc().addItem("A/D/G/pi", "This is the file 'pi'.");
        thisTest.getWc().setItemReposLastCmtRevision("A/D/G/pi", GCommitRev);
        thisTest.getWc().setItemReposKind("A/D/G/pi", NodeKind.file);

        thisTest.getWc().setItemContent("A/D/G/rho",
                                        "This is the file 'rho'.");
        thisTest.getWc().setItemOODInfo("A/D/G/rho", rhoCommitRev, rhoAuthor,
                                        rhoCommitDate, NodeKind.file);

        thisTest.getWc().setItemContent("A/D/G/tau",
                                        "This is the file 'tau'.");
        thisTest.getWc().setItemOODInfo("A/D/G/tau", tauCommitRev, tauAuthor,
                                        tauCommitDate, NodeKind.file);

        thisTest.getWc().setItemOODInfo("A/D/H", psiCommitRev, psiAuthor,
                                        psiCommitDate, NodeKind.dir);

        thisTest.getWc().setItemWorkingCopyRevision("A/D/H/nu",
            Revision.SVN_INVALID_REVNUM);
        thisTest.getWc().setItemTextStatus("A/D/H/nu", Status.Kind.none);
        thisTest.getWc().setItemNodeKind("A/D/H/nu", NodeKind.unknown);
        thisTest.getWc().setItemOODInfo("A/D/H/nu", nuCommitRev, nuAuthor,
                                        nuCommitDate, NodeKind.file);

        thisTest.getWc().setItemContent("A/D/H/chi",
                                        "This is the file 'chi'.");
        thisTest.getWc().setItemOODInfo("A/D/H/chi", chiCommitRev, chiAuthor,
                                        chiCommitDate, NodeKind.file);

        thisTest.getWc().removeItem("A/D/H/psi");
        thisTest.getWc().addItem("A/D/H/psi", "This is the file 'psi'.");
        // psi was replaced with a directory
        thisTest.getWc().setItemOODInfo("A/D/H/psi", psiCommitRev, psiAuthor,
                                        psiCommitDate, NodeKind.dir);

        thisTest.getWc().setItemPropStatus("", Status.Kind.none);
        thisTest.getWc().setItemOODInfo("", psiCommitRev, psiAuthor,
                                        psiCommitDate, NodeKind.dir);

        thisTest.checkStatus(true);
    }

    /**
     * Test the basic SVNClient.checkout functionality.
     * @throws Throwable
     */
    public void testBasicCheckout() throws Throwable
    {
        // build the test setup
        OneTest thisTest = new OneTest();
        try
        {
            // obstructed checkout must fail
            client.checkout(thisTest.getUrl() + "/A", thisTest.getWCPath(),
                            null, true);
            fail("missing exception");
        }
        catch (ClientException expected)
        {
        }
        // modify file A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muWriter.print("appended mu text");
        muWriter.close();
        thisTest.getWc().setItemTextStatus("A/mu", Status.Kind.modified);

        // delete A/B/lambda without svn
        File lambda = new File(thisTest.getWorkingCopy(), "A/B/lambda");
        lambda.delete();
        thisTest.getWc().setItemTextStatus("A/B/lambda", Status.Kind.missing);

        // remove A/D/G
        client.remove(new String[]{thisTest.getWCPath() + "/A/D/G"}, null,
                      false);
        thisTest.getWc().setItemTextStatus("A/D/G", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/pi", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/rho", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/tau", Status.Kind.deleted);

        // check the status of the working copy
        thisTest.checkStatus();

        // recheckout the working copy
        client.checkout(thisTest.getUrl(), thisTest.getWCPath(), null, true);

        // deleted file should reapear
        thisTest.getWc().setItemTextStatus("A/B/lambda", Status.Kind.normal);

        // check the status of the working copy
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.commit functionality.
     * @throws Throwable
     */
    public void testBasicCommit() throws Throwable
    {
        // build the test setup
        OneTest thisTest = new OneTest();

        // modify file A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muWriter.print("appended mu text");
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        thisTest.getWc().setItemContent("A/mu",
                thisTest.getWc().getItemContent("A/mu") + "appended mu text");
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/mu",NodeKind.file,
                CommitItemStateFlags.TextMods);

        // modify file A/D/G/rho
        File rho = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        PrintWriter rhoWriter =
            new PrintWriter(new FileOutputStream(rho, true));
        rhoWriter.print("new appended text for rho");
        rhoWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        thisTest.getWc().setItemContent("A/D/G/rho",
                thisTest.getWc().getItemContent("A/D/G/rho")
                + "new appended text for rho");
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/D/G/rho",NodeKind.file,
                CommitItemStateFlags.TextMods);

        // commit the changes
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg",
                                   true),
                     2);

        // check the status of the working copy
        thisTest.checkStatus();
    }

    /**
     * Test the basic property setting/getting functionality.
     * @throws Throwable
     */
    public void testBasicProperties() throws Throwable
    {
        OneTest thisTest = new OneTest();
        WC wc = thisTest.getWc();

        // Check getting properties the non-callback way
        String itemPath = fileToSVNPath(new File(thisTest.getWCPath(),
                                                 "iota"),
                                        false);

        client.propertySet(itemPath, "abc", "def", false);
        PropertyData[] properties = client.properties(itemPath);

        PropertyData prop = properties[0];
        assertEquals("abc", prop.getName());
        assertEquals("def", prop.getValue());

        wc.setItemPropStatus("iota", Status.Kind.modified);
        thisTest.checkStatus();

        // Check getting properties the callback way
        itemPath = fileToSVNPath(new File(thisTest.getWCPath(),
                                          "/A/B/E/alpha"),
                                 false);
        client.propertyCreate(itemPath, "cqcq", "qrz", false, false);
        ProplistCallbackImpl callback = new ProplistCallbackImpl();

        client.properties(itemPath, null, null, Depth.empty, null, callback);
        Map propMap = callback.getProperties(itemPath);
        Iterator it = propMap.keySet().iterator();

        while (it.hasNext())
        {
            String key = (String) it.next();
            assertEquals("cqcq", key);
            assertEquals("qrz", (String) propMap.get(key));
        }

        wc.setItemPropStatus("A/B/E/alpha", Status.Kind.modified);
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.update functionality.
     * @throws Throwable
     */
    public void testBasicUpdate() throws Throwable
    {
        // build the test setup. Used for the changes
        OneTest thisTest = new OneTest();

        // build the backup test setup. That is the one that will be updated
        OneTest backupTest = thisTest.copy(".backup");

        // modify A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muWriter.print("appended mu text");
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        thisTest.getWc().setItemContent("A/mu",
                thisTest.getWc().getItemContent("A/mu") + "appended mu text");
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/mu",NodeKind.file,
                CommitItemStateFlags.TextMods);

        // modify A/D/G/rho
        File rho = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        PrintWriter rhoWriter =
            new PrintWriter(new FileOutputStream(rho, true));
        rhoWriter.print("new appended text for rho");
        rhoWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        thisTest.getWc().setItemContent("A/D/G/rho",
                thisTest.getWc().getItemContent("A/D/G/rho")
                + "new appended text for rho");
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/D/G/rho",NodeKind.file,
                CommitItemStateFlags.TextMods);

        // commit the changes
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg",
                                   true),
                     2);

        // check the status of the working copy
        thisTest.checkStatus();

        // update the backup test
        assertEquals("wrong revision number from update",
                     client.update(backupTest.getWCPath(), null, true),
                     2);

        // set the expected working copy layout for the backup test
        backupTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        backupTest.getWc().setItemContent("A/mu",
                backupTest.getWc().getItemContent("A/mu") + "appended mu text");
        backupTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        backupTest.getWc().setItemContent("A/D/G/rho",
                backupTest.getWc().getItemContent("A/D/G/rho")
                + "new appended text for rho");

        // check the status of the working copy of the backup test
        backupTest.checkStatus();
    }

    /**
     * Test basic SVNClient.mkdir with URL parameter functionality.
     * @throws Throwable
     */
    public void testBasicMkdirUrl() throws Throwable
    {
        // build the test setup.
        OneTest thisTest = new OneTest();

        // create Y and Y/Z directories in the repository
        addExpectedCommitItem(null, thisTest.getUrl(), "Y", NodeKind.none,
                              CommitItemStateFlags.Add);
        addExpectedCommitItem(null, thisTest.getUrl(), "Y/Z", NodeKind.none,
                              CommitItemStateFlags.Add);
        client.mkdir(new String[]{thisTest.getUrl() + "/Y",
                                  thisTest.getUrl() + "/Y/Z"}, "log_msg");

        // add the new directories the expected working copy layout
        thisTest.getWc().addItem("Y", null);
        thisTest.getWc().setItemWorkingCopyRevision("Y", 2);
        thisTest.getWc().addItem("Y/Z", null);
        thisTest.getWc().setItemWorkingCopyRevision("Y/Z", 2);

        // update the working copy
        assertEquals("wrong revision from update",
                     client.update(thisTest.getWCPath(), null, true),
                     2);

        // check the status of the working copy
        thisTest.checkStatus();
    }

    /**
     * Test the {@link SVNClientInterface.copy()} API.
     * @since 1.5
     */
    public void testCopy()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest();

        WC wc = thisTest.getWc();
        final Revision firstRevision = Revision.getInstance(1);
        final Revision pegRevision = null;  // Defaults to Revision.HEAD.

        // Copy files from A/B/E to A/B/F.
        String[] srcPaths = { "alpha", "beta" };
        CopySource[] sources = new CopySource[srcPaths.length];
        for (int i = 0; i < srcPaths.length; i++)
        {
            String fileName = srcPaths[i];
            sources[i] =
                new CopySource(new File(thisTest.getWorkingCopy(),
                                        "A/B/E/" + fileName).getPath(),
                               firstRevision, pegRevision);
            wc.addItem("A/B/F/" + fileName,
                       wc.getItemContent("A/B/E/" + fileName));
            wc.setItemWorkingCopyRevision("A/B/F/" + fileName, 2);
            addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                                  "A/B/F/" + fileName, NodeKind.file,
                                  CommitItemStateFlags.Add |
                                  CommitItemStateFlags.IsCopy);
        }
        client.copy(sources,
                    new File(thisTest.getWorkingCopy(), "A/B/F").getPath(),
                    null, true, false, null);

        // Commit the changes, and check the state of the WC.
        assertEquals("Unexpected WC revision number after commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "Copy files", true),
                     2);
        thisTest.checkStatus();

        assertExpectedSuggestion(thisTest.getUrl() + "/A/B/E/alpha", "A/B/F/alpha", thisTest);

        // Now test a WC to URL copy
        CopySource wcSource[] = new CopySource[1];
        wcSource[0] = new CopySource(new File(thisTest.getWorkingCopy(),
                                        "A/B").getPath(), Revision.WORKING, Revision.WORKING);
        client.commitMessageHandler(null);
        client.copy(wcSource,
                    thisTest.getUrl() + "/parent/A/B",
                    "Copy WC to URL", true, true, null);

        // update the WC to get new folder and confirm the copy
        assertEquals("wrong revision number from update",
                     client.update(thisTest.getWCPath(), null, true),
                     3);
    }

    /**
     * Test the {@link SVNClientInterface.move()} API.
     * @since 1.5
     */
    public void testMove()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest();
        WC wc = thisTest.getWc();

        // Move files from A/B/E to A/B/F.
        String[] srcPaths = { "alpha", "beta" };
        for (int i = 0; i < srcPaths.length; i++)
        {
            String fileName = srcPaths[i];
            srcPaths[i] = new File(thisTest.getWorkingCopy(),
                                   "A/B/E/" + fileName).getPath();

            wc.addItem("A/B/F/" + fileName,
                       wc.getItemContent("A/B/E/" + fileName));
            wc.setItemWorkingCopyRevision("A/B/F/" + fileName, 2);
            addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                                  "A/B/F/" + fileName, NodeKind.file,
                                  CommitItemStateFlags.Add |
                                  CommitItemStateFlags.IsCopy);

            wc.removeItem("A/B/E/" + fileName);
            addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                                  "A/B/E/" + fileName, NodeKind.file,
                                  CommitItemStateFlags.Delete);
        }
        client.move(srcPaths,
                    new File(thisTest.getWorkingCopy(), "A/B/F").getPath(),
                    null, false, true, false, null);

        // Commit the changes, and check the state of the WC.
        assertEquals("Unexpected WC revision number after commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "Move files", true), 2);
        thisTest.checkStatus();

        assertExpectedSuggestion(thisTest.getUrl() + "/A/B/E/alpha", "A/B/F/alpha", thisTest);
    }

    /**
     * Assert that the first merge source suggested for
     * <code>destPath</code> at {@link Revision#WORKING} and {@link
     * Revision#HEAD} is equivalent to <code>expectedSrc</code>.
     * @exception SubversionException If retrieval of the copy source fails.
     * @since 1.5
     */
    private void assertExpectedSuggestion(String expectedSrc,
                                          String destPath, OneTest thisTest)
        throws SubversionException
    {
        String wcPath = fileToSVNPath(new File(thisTest.getWCPath(),
                                               destPath), false);
        String[] suggestions = client.suggestMergeSources(wcPath,
                                                          Revision.WORKING);
        assertNotNull(suggestions);
        assertTrue(suggestions.length >= 1);
        assertTrue("Unexpected copy source path, expected " +
                   expectedSrc + ", got " + suggestions[0],
                   expectedSrc.equals(suggestions[0]));

        // Same test using URL
        String url = thisTest.getUrl() + "/" + destPath;
        suggestions = client.suggestMergeSources(url, Revision.HEAD);
        assertNotNull(suggestions);
        assertTrue(suggestions.length >= 1);
        assertTrue("Unexpected copy source path, expected " +
                   expectedSrc + ", got " + suggestions[0],
                   expectedSrc.equals(suggestions[0]));

    }

    /**
     * Tests that the passed start and end revision are contained
     * within the array of revisions.
     * @since 1.5
     */
    private void assertExpectedMergeRange(long start, long end,
                                          long[] revisions)
    {
        Arrays.sort(revisions);
        for (int i = 0; i < revisions.length; i++) {
            if (revisions[i] <= start) {
                for (int j = i; j < revisions.length; j++)
                {
                    if (end <= revisions[j])
                        return;
                }
                fail("End revision: " + end + " was not in range: " + revisions[0] +
                        " : " + revisions[revisions.length - 1]);
                return;
            }
        }
        fail("Start revision: " + start + " was not in range: " + revisions[0] +
                " : " + revisions[revisions.length - 1]);
    }

    /**
     * Test the basic SVNClient.update functionality with concurrent
     * changes in the repository and the working copy.
     * @throws Throwable
     */
    public void testBasicMergingUpdate() throws Throwable
    {
        // build the first working copy
        OneTest thisTest = new OneTest();

        // append 10 lines to A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        String muContent = thisTest.getWc().getItemContent("A/mu");
        for (int i = 2; i < 11; i++)
        {
            muWriter.print("\nThis is line " + i + " in mu");
            muContent = muContent + "\nThis is line " + i + " in mu";
        }
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        thisTest.getWc().setItemContent("A/mu", muContent);
        addExpectedCommitItem(thisTest.getWorkingCopy().getAbsolutePath(),
                              thisTest.getUrl(), "A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // append 10 line to A/D/G/rho
        File rho = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        PrintWriter rhoWriter =
            new PrintWriter(new FileOutputStream(rho, true));
        String rhoContent = thisTest.getWc().getItemContent("A/D/G/rho");
        for (int i = 2; i < 11; i++)
        {
            rhoWriter.print("\nThis is line " + i + " in rho");
            rhoContent = rhoContent + "\nThis is line " + i + " in rho";
        }
        rhoWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        thisTest.getWc().setItemContent("A/D/G/rho", rhoContent);
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // commit the changes
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg",
                                   true),
                     2);

        // check the status of the first working copy
        thisTest.checkStatus();

        // create a backup copy of the working copy
        OneTest backupTest = thisTest.copy(".backup");

        // change the last line of A/mu in the first working copy
        muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muContent = thisTest.getWc().getItemContent("A/mu");
        muWriter.print(" Appended to line 10 of mu");
        muContent = muContent + " Appended to line 10 of mu";
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 3);
        thisTest.getWc().setItemContent("A/mu", muContent);
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // change the last line of A/mu in the first working copy
        rhoWriter = new PrintWriter(new FileOutputStream(rho, true));
        rhoContent = thisTest.getWc().getItemContent("A/D/G/rho");
        rhoWriter.print(" Appended to line 10 of rho");
        rhoContent = rhoContent + " Appended to line 10 of rho";
        rhoWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 3);
        thisTest.getWc().setItemContent("A/D/G/rho", rhoContent);
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // commit these changes to the repository
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg",
                                   true),
                     3);

        // check the status of the first working copy
        thisTest.checkStatus();

        // modify the first line of A/mu in the backup working copy
        mu = new File(backupTest.getWorkingCopy(), "A/mu");
        muWriter = new PrintWriter(new FileOutputStream(mu));
        muWriter.print("This is the new line 1 in the backup copy of mu");
        muContent = "This is the new line 1 in the backup copy of mu";
        for (int i = 2; i < 11; i++)
        {
            muWriter.print("\nThis is line " + i + " in mu");
            muContent = muContent + "\nThis is line " + i + " in mu";
        }
        muWriter.close();
        backupTest.getWc().setItemWorkingCopyRevision("A/mu", 3);
        muContent = muContent + " Appended to line 10 of mu";
        backupTest.getWc().setItemContent("A/mu", muContent);
        backupTest.getWc().setItemTextStatus("A/mu", Status.Kind.modified);

        // modify the first line of A/D/G/rho in the backup working copy
        rho = new File(backupTest.getWorkingCopy(), "A/D/G/rho");
        rhoWriter = new PrintWriter(new FileOutputStream(rho));
        rhoWriter.print("This is the new line 1 in the backup copy of rho");
        rhoContent = "This is the new line 1 in the backup copy of rho";
        for (int i = 2; i < 11; i++)
        {
            rhoWriter.print("\nThis is line " + i + " in rho");
            rhoContent = rhoContent + "\nThis is line " + i + " in rho";
        }
        rhoWriter.close();
        backupTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 3);
        rhoContent = rhoContent + " Appended to line 10 of rho";
        backupTest.getWc().setItemContent("A/D/G/rho", rhoContent);
        backupTest.getWc().setItemTextStatus("A/D/G/rho", Status.Kind.modified);

        // update the backup working copy
        assertEquals("wrong revision number from update",
                     client.update(backupTest.getWCPath(), null, true),
                     3);

        // check the status of the backup working copy
        backupTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.update functionality with concurrent
     * changes in the repository and the working copy that generate
     * conflicts.
     * @throws Throwable
     */
    public void testBasicConflict() throws Throwable
    {
        // build the first working copy
        OneTest thisTest = new OneTest();

        // copy the first working copy to the backup working copy
        OneTest backupTest = thisTest.copy(".backup");

        // append a line to A/mu in the first working copy
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        String muContent = thisTest.getWc().getItemContent("A/mu");
        muWriter.print("\nOriginal appended text for mu");
        muContent = muContent + "\nOriginal appended text for mu";
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        thisTest.getWc().setItemContent("A/mu", muContent);
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // append a line to A/D/G/rho in the first working copy
        File rho = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        PrintWriter rhoWriter =
            new PrintWriter(new FileOutputStream(rho, true));
        String rhoContent = thisTest.getWc().getItemContent("A/D/G/rho");
        rhoWriter.print("\nOriginal appended text for rho");
        rhoContent = rhoContent + "\nOriginal appended text for rho";
        rhoWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        thisTest.getWc().setItemContent("A/D/G/rho", rhoContent);
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);

        // commit the changes in the first working copy
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg",
                                   true),
                     2);

        // test the status of the working copy after the commit
        thisTest.checkStatus();

        // append a different line to A/mu in the backup working copy
        mu = new File(backupTest.getWorkingCopy(), "A/mu");
        muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muWriter.print("\nConflicting appended text for mu");
        muContent = "<<<<<<< .mine\nThis is the file 'mu'.\n"+
                    "Conflicting appended text for mu=======\n"+
                    "This is the file 'mu'.\n"+
                    "Original appended text for mu>>>>>>> .r2";
        muWriter.close();
        backupTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        backupTest.getWc().setItemContent("A/mu", muContent);
        backupTest.getWc().setItemTextStatus("A/mu", Status.Kind.conflicted);
        backupTest.getWc().addItem("A/mu.r1", "");
        backupTest.getWc().setItemNodeKind("A/mu.r1", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/mu.r1",
                                             Status.Kind.unversioned);
        backupTest.getWc().addItem("A/mu.r2", "");
        backupTest.getWc().setItemNodeKind("A/mu.r2", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/mu.r2",
                                             Status.Kind.unversioned);
        backupTest.getWc().addItem("A/mu.mine", "");
        backupTest.getWc().setItemNodeKind("A/mu.mine", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/mu.mine",
                                             Status.Kind.unversioned);

        // append a different line to A/D/G/rho in the backup working copy
        rho = new File(backupTest.getWorkingCopy(), "A/D/G/rho");
        rhoWriter = new PrintWriter(new FileOutputStream(rho, true));
        rhoWriter.print("\nConflicting appended text for rho");
        rhoContent = "<<<<<<< .mine\nThis is the file 'rho'.\n"+
                    "Conflicting appended text for rho=======\n"+
                    "his is the file 'rho'.\n"+
                    "Original appended text for rho>>>>>>> .r2";
        rhoWriter.close();
        backupTest.getWc().setItemWorkingCopyRevision("A/D/G/rho", 2);
        backupTest.getWc().setItemContent("A/D/G/rho", rhoContent);
        backupTest.getWc().setItemTextStatus("A/D/G/rho",
                                             Status.Kind.conflicted);
        backupTest.getWc().addItem("A/D/G/rho.r1", "");
        backupTest.getWc().setItemNodeKind("A/D/G/rho.r1", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/D/G/rho.r1",
                                             Status.Kind.unversioned);
        backupTest.getWc().addItem("A/D/G/rho.r2", "");
        backupTest.getWc().setItemNodeKind("A/D/G/rho.r2", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/D/G/rho.r2",
                                             Status.Kind.unversioned);
        backupTest.getWc().addItem("A/D/G/rho.mine", "");
        backupTest.getWc().setItemNodeKind("A/D/G/rho.mine", NodeKind.unknown);
        backupTest.getWc().setItemTextStatus("A/D/G/rho.mine",
                                             Status.Kind.unversioned);

        // update the backup working copy from the repository
        assertEquals("wrong revision number from update",
                     client.update(backupTest.getWCPath(), null, true),
                     2);

        // check the status of the backup working copy
        backupTest.checkStatus();

        // flag A/mu as resolved
        client.resolved(backupTest.getWCPath()+"/A/mu", false);
        backupTest.getWc().setItemTextStatus("A/mu", Status.Kind.modified);
        backupTest.getWc().removeItem("A/mu.r1");
        backupTest.getWc().removeItem("A/mu.r2");
        backupTest.getWc().removeItem("A/mu.mine");

        // flag A/D/G/rho as resolved
        client.resolved(backupTest.getWCPath()+"/A/D/G/rho", false);
        backupTest.getWc().setItemTextStatus("A/D/G/rho",
                                             Status.Kind.modified);
        backupTest.getWc().removeItem("A/D/G/rho.r1");
        backupTest.getWc().removeItem("A/D/G/rho.r2");
        backupTest.getWc().removeItem("A/D/G/rho.mine");

        // check the status after the conflicts are flaged as resolved
        backupTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.cleanup functionality.
     * Without a way to force a lock, this test just verifies
     * the method can be called succesfully.
     * @throws Throwable
     */
    public void testBasicCleanup() throws Throwable
    {
        // create a test working copy
        OneTest thisTest = new OneTest();

        // run cleanup
        client.cleanup(thisTest.getWCPath());

    }

    /**
     * Test the basic SVNClient.revert functionality.
     * @throws Throwable
     */
    public void testBasicRevert() throws Throwable
    {
        // create a test working copy
        OneTest thisTest = new OneTest();

        // modify A/B/E/beta
        File file = new File(thisTest.getWorkingCopy(), "A/B/E/beta");
        PrintWriter pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("Added some text to 'beta'.");
        pw.close();
        thisTest.getWc().setItemTextStatus("A/B/E/beta", Status.Kind.modified);

        // modify iota
        file = new File(thisTest.getWorkingCopy(), "iota");
        pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("Added some text to 'iota'.");
        pw.close();
        thisTest.getWc().setItemTextStatus("iota", Status.Kind.modified);

        // modify A/D/G/rho
        file = new File(thisTest.getWorkingCopy(), "A/D/G/rho");
        pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("Added some text to 'rho'.");
        pw.close();
        thisTest.getWc().setItemTextStatus("A/D/G/rho", Status.Kind.modified);

        // create new file A/D/H/zeta and add it to subversion
        file = new File(thisTest.getWorkingCopy(), "A/D/H/zeta");
        pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("Added some text to 'zeta'.");
        pw.close();
        thisTest.getWc().addItem("A/D/H/zeta", "Added some text to 'zeta'.");
        thisTest.getWc().setItemTextStatus("A/D/H/zeta", Status.Kind.added);
        client.add(file.getAbsolutePath(), false);

        // test the status of the working copy
        thisTest.checkStatus();

        // revert the changes
        client.revert(thisTest.getWCPath()+"/A/B/E/beta", false);
        thisTest.getWc().setItemTextStatus("A/B/E/beta", Status.Kind.normal);
        client.revert(thisTest.getWCPath()+"/iota", false);
        thisTest.getWc().setItemTextStatus("iota", Status.Kind.normal);
        client.revert(thisTest.getWCPath()+"/A/D/G/rho", false);
        thisTest.getWc().setItemTextStatus("A/D/G/rho", Status.Kind.normal);
        client.revert(thisTest.getWCPath()+"/A/D/H/zeta", false);
        thisTest.getWc().setItemTextStatus("A/D/H/zeta",
                Status.Kind.unversioned);
        thisTest.getWc().setItemNodeKind("A/D/H/zeta", NodeKind.unknown);

        // test the status of the working copy
        thisTest.checkStatus();

        // delete A/B/E/beta and revert the change
        file = new File(thisTest.getWorkingCopy(), "A/B/E/beta");
        file.delete();
        client.revert(file.getAbsolutePath(), false);

        // resurected file should not be readonly
        assertTrue("reverted file is not readonly",
                file.canWrite()&& file.canRead());

        // test the status of the working copy
        thisTest.checkStatus();

        // create & add the directory X
        client.mkdir(new String[] {thisTest.getWCPath()+"/X"}, null);
        thisTest.getWc().addItem("X", null);
        thisTest.getWc().setItemTextStatus("X", Status.Kind.added);

        // test the status of the working copy
        thisTest.checkStatus();

        // remove & revert X
        removeDirOrFile(new File(thisTest.getWorkingCopy(), "X"));
        client.revert(thisTest.getWCPath()+"/X", false);
        thisTest.getWc().removeItem("X");

        // test the status of the working copy
        thisTest.checkStatus();

        // delete the directory A/B/E
        client.remove(new String[] {thisTest.getWCPath()+"/A/B/E"}, null, true);
        removeDirOrFile(new File(thisTest.getWorkingCopy(), "A/B/E"));
        thisTest.getWc().setItemTextStatus("A/B/E", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/B/E/alpha", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/B/E/beta", Status.Kind.deleted);

        // test the status of the working copy
        thisTest.checkStatus();

        // revert A/B/E -> this will resurect it
        client.revert(thisTest.getWCPath()+"/A/B/E", true);
        thisTest.getWc().setItemTextStatus("A/B/E", Status.Kind.normal);
        thisTest.getWc().setItemTextStatus("A/B/E/alpha", Status.Kind.normal);
        thisTest.getWc().setItemTextStatus("A/B/E/beta", Status.Kind.normal);

        // test the status of the working copy
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.switch functionality.
     * @throws Throwable
     */
    public void testBasicSwitch() throws Throwable
    {
        // create the test working copy
        OneTest thisTest = new OneTest();

        // switch iota to A/D/gamma
        String iotaPath = thisTest.getWCPath() + "/iota";
        String gammaUrl = thisTest.getUrl() + "/A/D/gamma";
        thisTest.getWc().setItemContent("iota",
                greekWC.getItemContent("A/D/gamma"));
        thisTest.getWc().setItemIsSwitched("iota", true);
        client.doSwitch(iotaPath, gammaUrl, null, true);

        // check the status of the working copy
        thisTest.checkStatus();

        // switch A/D/H to /A/D/G
        String adhPath = thisTest.getWCPath() + "/A/D/H";
        String adgURL = thisTest.getUrl() + "/A/D/G";
        thisTest.getWc().setItemIsSwitched("A/D/H",true);
        thisTest.getWc().removeItem("A/D/H/chi");
        thisTest.getWc().removeItem("A/D/H/omega");
        thisTest.getWc().removeItem("A/D/H/psi");
        thisTest.getWc().addItem("A/D/H/pi",
                thisTest.getWc().getItemContent("A/D/G/pi"));
        thisTest.getWc().addItem("A/D/H/rho",
                thisTest.getWc().getItemContent("A/D/G/rho"));
        thisTest.getWc().addItem("A/D/H/tau",
                thisTest.getWc().getItemContent("A/D/G/tau"));
        client.doSwitch(adhPath, adgURL, null, true);

        // check the status of the working copy
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.remove functionality.
     * @throws Throwable
     */
    public void testBasicDelete() throws Throwable
    {
        // create the test working copy
        OneTest thisTest = new OneTest();

        // modify A/D/H/chi
        File file = new File(thisTest.getWorkingCopy(), "A/D/H/chi");
        PrintWriter pw = new PrintWriter(new FileOutputStream(file, true));
        pw.print("added to chi");
        pw.close();
        thisTest.getWc().setItemTextStatus("A/D/H/chi", Status.Kind.modified);

        // set a property on A/D/G/rho file
        client.propertySet(thisTest.getWCPath()+"/A/D/G/rho", "abc", "def",
                true);
        thisTest.getWc().setItemPropStatus("A/D/G/rho", Status.Kind.modified);

        // set a property on A/B/F directory
        client.propertySet(thisTest.getWCPath()+"/A/B/F", "abc", "def", false);
        thisTest.getWc().setItemPropStatus("A/B/F", Status.Kind.modified);

        // create a unversioned A/C/sigma file
        file = new File(thisTest.getWCPath(),"A/C/sigma");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("unversioned sigma");
        pw.close();
        thisTest.getWc().addItem("A/C/sigma", "unversioned sigma");
        thisTest.getWc().setItemTextStatus("A/C/sigma", Status.Kind.unversioned);
        thisTest.getWc().setItemNodeKind("A/C/sigma", NodeKind.unknown);

        // create unversioned directory A/C/Q
        file = new File(thisTest.getWCPath(), "A/C/Q");
        file.mkdir();
        thisTest.getWc().addItem("A/C/Q", null);
        thisTest.getWc().setItemNodeKind("A/C/Q", NodeKind.unknown);
        thisTest.getWc().setItemTextStatus("A/C/Q", Status.Kind.unversioned);

        // create & add the directory A/B/X
        file = new File(thisTest.getWCPath(), "A/B/X");
        client.mkdir(new String[] {file.getAbsolutePath()}, null);
        thisTest.getWc().addItem("A/B/X", null);
        thisTest.getWc().setItemTextStatus("A/B/X", Status.Kind.added);

        // create & add the file A/B/X/xi
        file = new File(file, "xi");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("added xi");
        pw.close();
        client.add(file.getAbsolutePath(), false);
        thisTest.getWc().addItem("A/B/X/xi", "added xi");
        thisTest.getWc().setItemTextStatus("A/B/X/xi", Status.Kind.added);

        // create & add the directory A/B/Y
        file = new File(thisTest.getWCPath(), "A/B/Y");
        client.mkdir(new String[] {file.getAbsolutePath()}, null);
        thisTest.getWc().addItem("A/B/Y", null);
        thisTest.getWc().setItemTextStatus("A/B/Y", Status.Kind.added);

        // test the status of the working copy
        thisTest.checkStatus();

        // the following removes should all fail without force

        try
        {
            // remove of A/D/H/chi without force should fail, because it is
            // modified
            client.remove(new String[] {thisTest.getWCPath()+"/A/D/H/chi"},
                    null, false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/D/H without force should fail, because A/D/H/chi is
            // modified
            client.remove(new String[] {thisTest.getWCPath()+"/A/D/H"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/D/G/rho without force should fail, because it has
            // a new property
            client.remove(new String[] {thisTest.getWCPath()+"/A/D/G/rho"},
                    null, false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/D/G without force should fail, because A/D/G/rho has
            // a new property
            client.remove(new String[] {thisTest.getWCPath()+"/A/D/G"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/B/F without force should fail, because it has
            // a new property
            client.remove(new String[] {thisTest.getWCPath()+"/A/B/F"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/B without force should fail, because A/B/F has
            // a new property
            client.remove(new String[] {thisTest.getWCPath()+"/A/B"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/C/sigma without force should fail, because it is
            // unversioned
            client.remove(new String[] {thisTest.getWCPath()+"/A/C/sigma"},
                    null, false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/C without force should fail, because A/C/sigma is
            // unversioned
            client.remove(new String[] {thisTest.getWCPath()+"/A/C"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        try
        {
            // remove of A/B/X without force should fail, because it is new
            client.remove(new String[] {thisTest.getWCPath()+"/A/B/X"}, null,
                    false);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        // check the status of the working copy
        thisTest.checkStatus();

        // the following removes should all work
        client.remove(new String[] {thisTest.getWCPath()+"/A/B/E"}, null,
                false);
        thisTest.getWc().setItemTextStatus("A/B/E",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/B/E/alpha",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/B/E/beta",Status.Kind.deleted);
        client.remove(new String[] {thisTest.getWCPath()+"/A/D/H"}, null, true);
        thisTest.getWc().setItemTextStatus("A/D/H",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/chi",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/omega",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/psi",Status.Kind.deleted);
        client.remove(new String[] {thisTest.getWCPath()+"/A/D/G"}, null, true);
        thisTest.getWc().setItemTextStatus("A/D/G",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/rho",Status.Kind.deleted);
        thisTest.getWc().setItemPropStatus("A/D/G/rho", Status.Kind.none);
        thisTest.getWc().setItemTextStatus("A/D/G/pi",Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/tau",Status.Kind.deleted);
        client.remove(new String[] {thisTest.getWCPath()+"/A/B/F"}, null, true);
        thisTest.getWc().setItemTextStatus("A/B/F",Status.Kind.deleted);
        thisTest.getWc().setItemPropStatus("A/B/F", Status.Kind.none);
        client.remove(new String[] {thisTest.getWCPath()+"/A/C"}, null, true);
        thisTest.getWc().setItemTextStatus("A/C",Status.Kind.deleted);
        client.remove(new String[] {thisTest.getWCPath()+"/A/B/X"}, null, true);
        file = new File(thisTest.getWorkingCopy(), "iota");
        file.delete();
        client.remove(new String[] {file.getAbsolutePath()}, null, true);
        thisTest.getWc().setItemTextStatus("iota",Status.Kind.deleted);
        file = new File(thisTest.getWorkingCopy(), "A/D/gamma");
        file.delete();
        client.remove(new String[] {file.getAbsolutePath()}, null, false);
        thisTest.getWc().setItemTextStatus("A/D/gamma",Status.Kind.deleted);
        client.remove(new String[] {file.getAbsolutePath()}, null, true);
        client.remove(new String[] {thisTest.getWCPath()+"/A/B/E"}, null,
                false);
        thisTest.getWc().removeItem("A/B/X");
        thisTest.getWc().removeItem("A/B/X/xi");
        thisTest.getWc().removeItem("A/C/sigma");
        thisTest.getWc().removeItem("A/C/Q");
        thisTest.checkStatus();
        client.remove(new String[] {thisTest.getWCPath()+"/A/D"},null, true);
        thisTest.getWc().setItemTextStatus("A/D", Status.Kind.deleted);
        thisTest.getWc().removeItem("A/D/Y");

        // check the status of the working copy
        thisTest.checkStatus();

        // confirm that the file are realy deleted
        assertFalse("failed to remove text modified file",
                new File(thisTest.getWorkingCopy(), "A/D/G/rho").exists());
        assertFalse("failed to remove prop modified file",
                new File(thisTest.getWorkingCopy(), "A/D/H/chi").exists());
        assertFalse("failed to remove unversioned file",
                new File(thisTest.getWorkingCopy(), "A/C/sigma").exists());
        assertFalse("failed to remove unmodified file",
                new File(thisTest.getWorkingCopy(), "A/B/E/alpha").exists());
        file = new File(thisTest.getWorkingCopy(),"A/B/F");
        assertFalse("failed to remove versioned dir", file.exists());
        assertFalse("failed to remove unversioned dir",
                new File(thisTest.getWorkingCopy(), "A/C/Q").exists());
        assertFalse("failed to remove added dir",
                new File(thisTest.getWorkingCopy(), "A/B/X").exists());

        // delete unversioned file foo
        file = new File(thisTest.getWCPath(),"foo");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("unversioned foo");
        pw.close();
        client.remove(new String[] {file.getAbsolutePath()}, null, true);
        assertFalse("failed to remove unversioned file foo", file.exists());

        try
        {
            // delete non-existant file foo
            client.remove(new String[] {file.getAbsolutePath()}, null, true);
            fail("missing exception");
        }
        catch(ClientException expected)
        {
        }

        // delete file iota in the repository
        addExpectedCommitItem(null, thisTest.getUrl(), "iota", NodeKind.none,
                CommitItemStateFlags.Delete);
        client.remove(new String[] {thisTest.getUrl()+"/iota"},
                "delete iota URL", false);
    }

    public void testBasicCheckoutDeleted() throws Throwable
    {
        // create working copy
        OneTest thisTest = new OneTest();

        // delete A/D and its content
        client.remove(new String[] {thisTest.getWCPath()+"/A/D"}, null, true);
        thisTest.getWc().setItemTextStatus("A/D", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/rho", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/pi", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/G/tau", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/chi", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/psi", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/H/omega", Status.Kind.deleted);
        thisTest.getWc().setItemTextStatus("A/D/gamma", Status.Kind.deleted);

        // check the working copy status
        thisTest.checkStatus();

        // commit the change
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/D", NodeKind.dir,
                CommitItemStateFlags.Delete);
        assertEquals("wrong revision from commit",
                client.commit(new String[]{thisTest.getWCPath()}, "log message",
                        true),2);
        thisTest.getWc().removeItem("A/D");
        thisTest.getWc().removeItem("A/D/G");
        thisTest.getWc().removeItem("A/D/G/rho");
        thisTest.getWc().removeItem("A/D/G/pi");
        thisTest.getWc().removeItem("A/D/G/tau");
        thisTest.getWc().removeItem("A/D/H");
        thisTest.getWc().removeItem("A/D/H/chi");
        thisTest.getWc().removeItem("A/D/H/psi");
        thisTest.getWc().removeItem("A/D/H/omega");
        thisTest.getWc().removeItem("A/D/gamma");

        // check the working copy status
        thisTest.checkStatus();

        // check out the previous revision
        client.checkout(thisTest.getUrl()+"/A/D", thisTest.getWCPath()+"/new_D",
                new Revision.Number(1), true);
    }

    /**
     * Test the basic SVNClient.import functionality.
     * @throws Throwable
     */
    public void testBasicImport() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // create new_file
        File file = new File(thisTest.getWCPath(),"new_file");
        PrintWriter pw = new PrintWriter(new FileOutputStream(file));
        pw.print("some text");
        pw.close();

        // import new_file info dirA/dirB/newFile
        addExpectedCommitItem(thisTest.getWCPath(),
                null, "new_file", NodeKind.none, CommitItemStateFlags.Add);
        client.doImport(file.getAbsolutePath(),
                thisTest.getUrl()+"/dirA/dirB/new_file",
                "log message for new import", true);

        // delete new_file
        file.delete();

        // update the working
        assertEquals("wrong revision from update",
                client.update(thisTest.getWCPath(), null, true),2);
        thisTest.getWc().addItem("dirA", null);
        thisTest.getWc().setItemWorkingCopyRevision("dirA",2);
        thisTest.getWc().addItem("dirA/dirB", null);
        thisTest.getWc().setItemWorkingCopyRevision("dirA/dirB",2);
        thisTest.getWc().addItem("dirA/dirB/new_file", "some text");
        thisTest.getWc().setItemWorkingCopyRevision("dirA/dirB/new_file",2);

        // test the working copy status
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.fileContent functionality.
     * @throws Throwable
     */
    public void testBasicCat() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // modify A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter pw = new PrintWriter(new FileOutputStream(mu, true));
        pw.print("some text");
        pw.close();
        // get the content from the repository
        byte[] content = client.fileContent(thisTest.getWCPath()+"/A/mu", null);
        byte[] testContent = thisTest.getWc().getItemContent("A/mu").getBytes();

        // the content should be the same
        assertTrue("content changed", Arrays.equals(content, testContent));
    }

    /**
     * Test the basic SVNClient.fileContent functionality.
     * @throws Throwable
     */
    public void testBasicCatStream() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // modify A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter pw = new PrintWriter(new FileOutputStream(mu, true));
        pw.print("some text");
        pw.close();
        // get the content from the repository
        ByteArrayOutputStream baos = new ByteArrayOutputStream();
        client.streamFileContent(thisTest.getWCPath() + "/A/mu", null, null,
                                 100, baos);

        byte[] content = baos.toByteArray();
        byte[] testContent = thisTest.getWc().getItemContent("A/mu").getBytes();

        // the content should be the same
        assertTrue("content changed", Arrays.equals(content, testContent));
    }

    /**
     * Test the basic SVNClient.list functionality.
     * @throws Throwable
     */
    public void testBasicLs() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // list the repository root dir
        DirEntry[] entries = client.list(thisTest.getWCPath(), null, false);
        thisTest.getWc().check(entries, "", false);

        // list directory A
        entries = client.list(thisTest.getWCPath() + "/A", null, false);
        thisTest.getWc().check(entries, "A", false);

        // list directory A in BASE revision
        entries = client.list(thisTest.getWCPath() + "/A", Revision.BASE,
                              false);
        thisTest.getWc().check(entries, "A", false);

        // list file A/mu
        entries = client.list(thisTest.getWCPath() + "/A/mu", null, false);
        thisTest.getWc().check(entries, "A/mu");
    }

    /**
     * Test the basis SVNClient.add functionality with files that
     * should be ignored.
     * @throws Throwable
     */
    public void testBasicAddIgnores() throws Throwable
    {
        // create working copy
        OneTest thisTest = new OneTest();

        // create dir
        File dir = new File(thisTest.getWorkingCopy(), "dir");
        dir.mkdir();

        // create dir/foo.c
        File fileC = new File(dir, "foo.c");
        new FileOutputStream(fileC).close();

        // create dir/foo.o (should be ignored)
        File fileO = new File(dir, "foo.o");
        new FileOutputStream(fileO).close();

        // add dir
        client.add(dir.getAbsolutePath(), true);
        thisTest.getWc().addItem("dir", null);
        thisTest.getWc().setItemTextStatus("dir",Status.Kind.added);
        thisTest.getWc().addItem("dir/foo.c", "");
        thisTest.getWc().setItemTextStatus("dir/foo.c",Status.Kind.added);
        thisTest.getWc().addItem("dir/foo.o", "");
        thisTest.getWc().setItemTextStatus("dir/foo.o",Status.Kind.ignored);
        thisTest.getWc().setItemNodeKind("dir/foo.o", NodeKind.unknown);

        // test the working copy status
        thisTest.checkStatus();
    }

    /**
     * Test the basis SVNClient.import functionality with files that
     * should be ignored.
     * @throws Throwable
     */
    public void testBasicImportIgnores() throws Throwable
    {
        // create working copy
        OneTest thisTest = new OneTest();

        // create dir
        File dir = new File(thisTest.getWorkingCopy(), "dir");
        dir.mkdir();

        // create dir/foo.c
        File fileC = new File(dir, "foo.c");
        new FileOutputStream(fileC).close();

        // create dir/foo.o (should be ignored)
        File fileO = new File(dir, "foo.o");
        new FileOutputStream(fileO).close();

        // import dir
        addExpectedCommitItem(thisTest.getWCPath(),
                null, "dir", NodeKind.none, CommitItemStateFlags.Add);
        client.doImport(dir.getAbsolutePath(), thisTest.getUrl()+"/dir",
                "log message for import", true);

        // remove dir
        removeDirOrFile(dir);

        // udpate the working copy
        assertEquals("wrong revision from update", 2,
                client.update(thisTest.getWCPath(), null, true));
        thisTest.getWc().addItem("dir", null);
        thisTest.getWc().addItem("dir/foo.c", "");

        // test the working copy status
        thisTest.checkStatus();
    }

    /**
     * Test the basic SVNClient.info functionality.
     * @throws Throwable
     */
    public void testBasicInfo() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // get the item information and test it
        Info info = client.info(thisTest.getWCPath()+"/A/mu");
        assertEquals("wrong revision from info", 1,
                     info.getLastChangedRevision());
        assertEquals("wrong schedule kind from info", ScheduleKind.normal,
                     info.getSchedule());
        assertEquals("wrong node kind from info", NodeKind.file,
                     info.getNodeKind());
    }

    /**
     * Test the basic SVNClient.logMessages functionality.
     * @throws Throwable
     */
    public void testBasicLogMessage() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();

        // get the commit message of the initial import and test it
        LogMessage lm[] = client.logMessages(thisTest.getWCPath(), null,
                                             null, false, true);
        assertEquals("wrong number of objects", 1, lm.length);
        assertEquals("wrong message", "Log Message", lm[0].getMessage());
        assertEquals("wrong revision", 1, lm[0].getRevisionNumber());
        assertEquals("wrong user", "jrandom", lm[0].getAuthor());
        assertNotNull("changed paths set", lm[0].getChangedPaths());
        ChangePath cp[] = lm[0].getChangedPaths();
        assertEquals("wrong number of chang pathes", 20, cp.length);
        assertEquals("wrong path", "/A", cp[0].getPath());
        assertEquals("wrong copy source rev", -1, cp[0].getCopySrcRevision());
        assertNull("wrong copy source path", cp[0].getCopySrcPath());
        assertEquals("wrong action", 'A', cp[0].getAction());
        assertEquals("wrong time with getTimeMicros()",
                     lm[0].getTimeMicros()/1000,
                     lm[0].getDate().getTime());
        assertEquals("wrong time with getTimeMillis()",
                     lm[0].getTimeMillis(),
                     lm[0].getDate().getTime());
        assertEquals("wrong date with getTimeMicros()",
                     lm[0].getDate(),
                     new java.util.Date(lm[0].getTimeMicros()/1000));
        assertEquals("wrong date with getTimeMillis()",
                     lm[0].getDate(),
                     new java.util.Date(lm[0].getTimeMillis()));

        // Ensure that targets get canonicalized
        String non_canonical = thisTest.getUrl().toString() + "/";
        LogMessage lm2[] = client.logMessages(non_canonical, null,
                                              null, false, true);
    }

    /**
     * Test the basic SVNClient.getVersionInfo functionality.
     * @throws Throwable
     * @since 1.2
     */
    public void testBasicVersionInfo() throws Throwable
    {
        // create the working copy
        OneTest thisTest = new OneTest();
        assertEquals("wrong version info",
                     "1",
                     client.getVersionInfo(thisTest.getWCPath(), null, false));
    }

    /**
     * Test the basic SVNClient locking functionality.
     * @throws Throwable
     * @since 1.2
     */
    public void testBasicLocking() throws Throwable
    {
        // build the first working copy
        OneTest thisTest = new OneTest();

        client.propertySet(thisTest.getWCPath()+"/A/mu",
                           PropertyData.NEEDS_LOCK, "*", false);

        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/mu",NodeKind.file,
                              CommitItemStateFlags.PropMods);
        assertEquals("bad revision number on commit", 2,
                     client.commit(new String[] {thisTest.getWCPath()},
                                   "message", true));
        File f = new File(thisTest.getWCPath()+"/A/mu");
        assertEquals("file should be read only now", false, f.canWrite());
        client.lock(new String[] {thisTest.getWCPath()+"/A/mu"},
                    "comment", false);
        assertEquals("file should be read write now", true, f.canWrite());
        client.unlock(new String[]{thisTest.getWCPath()+"/A/mu"}, false);
        assertEquals("file should be read only now", false, f.canWrite());
        client.lock(new String[]{thisTest.getWCPath()+"/A/mu"},
                    "comment", false);
        assertEquals("file should be read write now", true, f.canWrite());
        addExpectedCommitItem(thisTest.getWCPath(),
                              thisTest.getUrl(), "A/mu",NodeKind.file,
                              0);
        assertEquals("rev number from commit", -1,
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "message", true));
        assertEquals("file should be read write now", true, f.canWrite());

        try
        {
            // Attempt to lock an invalid path
            client.lock(new String[]{thisTest.getWCPath()+"/A/mu2"}, "comment",
                        false);
            fail("missing exception");
        }
        catch (ClientException expected)
        {
        }
    }

    /**
     * Test the basic SVNClient.info2 functionality.
     * @throws Throwable
     * @since 1.2
     */
    public void testBasicInfo2() throws Throwable
    {
        // build the first working copy
        OneTest thisTest = new OneTest();

        final String failureMsg = "Incorrect number of info objects";
        Info2[] infos = client.info2(thisTest.getWCPath(), null, null, false);
        assertEquals(failureMsg, 1, infos.length);
        infos = client.info2(thisTest.getWCPath(), null, null, true);
        assertEquals(failureMsg, 21, infos.length);
        for (int i = 0; i < infos.length; i++)
        {
            Info2 info = infos[i];
            assertNull("Unexpected changelist present",
                       info.getChangelistName());

            boolean isFile = info.getKind() == NodeKind.file;
            assertTrue("Unexpected working file size " + info.getWorkingSize()
                       + " for '" + info + '\'',
                       (isFile ? info.getWorkingSize() > -1 :
                        info.getWorkingSize() == -1));
            // We shouldn't know the repository file size when only
            // examining the WC.
            assertEquals("Unexpected repos file size for '" + info + '\'',
                         -1, info.getReposSize());

           // Examine depth
           assertEquals("Unexpected depth for '" + info + "'",
                        (isFile ? Depth.unknown : Depth.infinity),
                        info.getDepth());
        }

        // Create wc with a depth of Depth.empty
        String secondWC = thisTest.getWCPath() + ".empty";
        removeDirOrFile(new File(secondWC));

        client.checkout(thisTest.getUrl(), secondWC, null, null, Depth.empty,
                        false, true);

        infos = client.info2(secondWC, null, null, false);

        // Examine that depth is Depth.empty
        assertEquals(Depth.empty, infos[0].getDepth());
    }

    /**
     * Test basic changelist functionality.
     * @throws Throwable
     * @since 1.5
     */
    public void testBasicChangelist() throws Throwable
    {
        // build the working copy
        OneTest thisTest = new OneTest();
        String changelistName = "changelist1";
        String[] changelists = new String[] { changelistName };
        MyChangelistCallback clCallback = new MyChangelistCallback();

        String[] paths = new String[]
          {fileToSVNPath(new File(thisTest.getWCPath(), "iota"), true)};
        // Add a path to a changelist, and check to see if it got added
        client.addToChangelist(paths, changelistName, Depth.infinity, null);
        String[] cl = new String[1];
        client.getChangelists(thisTest.getWCPath(), changelists,
                              Depth.infinity, clCallback);
        cl[0] = (String) clCallback.get(paths[0]).get(0);
        assertTrue(java.util.Arrays.equals(cl, changelists));
        // Does status report this changelist?
        Status[] status = client.status(paths[0], false, false, false, false,
                                        false);
        assertEquals(status[0].getChangelist(), changelistName);

        // Remove the path from the changelist, and check to see if the path is
        // actually removed.
        client.removeFromChangelists(paths, Depth.infinity, changelists);
        clCallback.clear();
        client.getChangelists(thisTest.getWCPath(), changelists,
                              Depth.infinity, clCallback);
        assertTrue(clCallback.isEmpty());
    }

    /**
     * Helper method for testing mergeinfo retrieval.  Assumes
     * that <code>targetPath</code> has both merge history and
     * available merges.
     * @param expectedMergedStart The expected start revision from the
     * merge history for <code>mergeSrc</code>.
     * @param expectedMergedEnd The expected end revision from the
     * merge history for <code>mergeSrc</code>.
     * @param expectedAvailableStart The expected start available revision
     * from the merge history for <code>mergeSrc</code>.  Zero if no need
     * to test the available range.
     * @param expectedAvailableEnd The expected end available revision
     * from the merge history for <code>mergeSrc</code>.
     * @param targetPath The path for which to acquire mergeinfo.
     * @param mergeSrc The URL from which to consider merges.
     */
    private void acquireMergeinfoAndAssertEquals(long expectedMergeStart,
                                                 long expectedMergeEnd,
                                                 long expectedAvailableStart,
                                                 long expectedAvailableEnd,
                                                 String targetPath,
                                                 String mergeSrc)
        throws SubversionException
    {
        // Verify expected merge history.
        Mergeinfo mergeInfo = client.getMergeinfo(targetPath, Revision.HEAD);
        assertNotNull("Missing merge info on '" + targetPath + '\'',
                      mergeInfo);
        List ranges = mergeInfo.getRevisions(mergeSrc);
        assertTrue("Missing merge info for source '" + mergeSrc + "' on '" +
                   targetPath + '\'', ranges != null && !ranges.isEmpty());
        RevisionRange range = (RevisionRange) ranges.get(0);
        String expectedMergedRevs = expectedMergeStart + "-" + expectedMergeEnd;
        assertEquals("Unexpected first merged revision range for '" +
                     mergeSrc + "' on '" + targetPath + '\'',
                     expectedMergedRevs, range.toString());

        // Verify expected available merges.
        if (expectedAvailableStart > 0)
        {
            long[] availableRevs =
                    getMergeinfoRevisions(MergeinfoLogKind.eligible, targetPath,
                                          Revision.HEAD, mergeSrc,
                                          Revision.HEAD);
            assertNotNull("Missing eligible merge info on '"+targetPath + '\'',
                          availableRevs);
            assertExpectedMergeRange(expectedAvailableStart,
                                     expectedAvailableEnd, availableRevs);
            }
    }

    /**
     * Calls the API to get mergeinfo revisions and returns
     * the revision numbers in a sorted array, or null if there
     * are no revisions to return.
     * @since 1.5
     */
    private long[] getMergeinfoRevisions(int kind, String pathOrUrl,
                                         Revision pegRevision,
                                         String mergeSourceUrl,
                                         Revision srcPegRevision)
        throws SubversionException
    {
        class Callback implements LogMessageCallback {

            List revList = new ArrayList();

            public void singleMessage(ChangePath[] changedPaths, long revision,
                    Map revprops, boolean hasChildren) {
                revList.add(new Long(revision));
            }

            public long[] getRevisions() {
                long[] revisions = new long[revList.size()];
                int i = 0;
                for (Iterator iter = revList.iterator(); iter.hasNext();) {
                    Long revision = (Long) iter.next();
                    revisions[i] = revision.longValue();
                    i++;
                }
                return revisions;
            }
        }
        Callback callback = new Callback();
        client.getMergeinfoLog(kind, pathOrUrl, pegRevision, mergeSourceUrl,
                               srcPegRevision, false, null, callback);
        return callback.getRevisions();
    }

    /**
     * Append the text <code>toAppend</code> to the WC file at
     * <code>path</code>, and update the expected WC state
     * accordingly.
     *
     * @param thisTest The test whose expected WC to tweak.
     * @param path The working copy-relative path to change.
     * @param toAppend The text to append to <code>path</code>.
     * @param rev The expected revision number for thisTest's WC.
     * @return The file created during the setup.
     * @since 1.5
     */
    private File appendText(OneTest thisTest, String path, String toAppend,
                            int rev)
        throws FileNotFoundException
    {
        File f = new File(thisTest.getWorkingCopy(), path);
        PrintWriter writer = new PrintWriter(new FileOutputStream(f, true));
        writer.print(toAppend);
        writer.close();
        if (rev > 0)
        {
            WC wc = thisTest.getWc();
            wc.setItemWorkingCopyRevision(path, rev);
            wc.setItemContent(path, wc.getItemContent(path) + toAppend);
        }
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(), path,
                              NodeKind.file, CommitItemStateFlags.TextMods);
        return f;
    }

    /**
     * Test the basic functionality of SVNClient.merge().
     * @throws Throwable
     * @since 1.2
     */
    public void testBasicMerge() throws Throwable
    {
        OneTest thisTest = setupAndPerformMerge();

        // Verify that there are now potential merge sources.
        String[] suggestedSrcs =
            client.suggestMergeSources(thisTest.getWCPath() + "/branches/A",
                                       Revision.WORKING);
        assertNotNull(suggestedSrcs);
        assertEquals(1, suggestedSrcs.length);

        // Test that getMergeinfo() returns null.
        assertNull(client.getMergeinfo(new File(thisTest.getWCPath(), "A")
                                       .toString(), Revision.HEAD));

        // Merge and commit some changes (r4).
        appendText(thisTest, "A/mu", "xxx", 4);
        appendText(thisTest, "A/D/G/rho", "yyy", 4);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     4);

        // Add a "begin merge" notification handler.
        final Revision[] actualRange = new Revision[2];
        Notify2 notify = new Notify2()
        {
            public void onNotify(NotifyInformation info)
            {
                if (info.getAction() == NotifyAction.merge_begin)
                {
                    RevisionRange r = info.getMergeRange();
                    actualRange[0] = r.getFromRevision();
                    actualRange[1] = r.getToRevision();
                }
            }
        };
        client.notification2(notify);

        // merge changes in A to branches/A
        String branchPath = thisTest.getWCPath() + "/branches/A";
        String modUrl = thisTest.getUrl() + "/A";
        // test --dry-run
        client.merge(modUrl, new Revision.Number(2), modUrl, Revision.HEAD,
                     branchPath, false, true, false, true);
        assertEquals("Notification of beginning of merge reported incorrect " +
                     "start revision", new Revision.Number(2), actualRange[0]);
        assertEquals("Notification of beginning of merge reported incorrect " +
                     "end revision", new Revision.Number(4), actualRange[1]);

        // now do the real merge
        client.merge(modUrl, new Revision.Number(2), modUrl, Revision.HEAD,
                     branchPath, false, true, false, false);
        assertEquals("Notification of beginning of merge reported incorrect " +
                     "start revision", new Revision.Number(2), actualRange[0]);
        assertEquals("Notification of beginning of merge reported incorrect " +
                     "end revision", new Revision.Number(4), actualRange[1]);

        // commit the changes so that we can verify merge
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true), 5);

        // Merge and commit some more changes (r6).
        appendText(thisTest, "A/mu", "xxxr6", 6);
        appendText(thisTest, "A/D/G/rho", "yyyr6", 6);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     6);

        // Test retrieval of mergeinfo from a WC path.
        String targetPath =
            new File(thisTest.getWCPath(), "branches/A/mu").getPath();
        final String mergeSrc = thisTest.getUrl() + "/A/mu";
        acquireMergeinfoAndAssertEquals(2, 4, 6, 6, targetPath, mergeSrc);

        // Test retrieval of mergeinfo from the repository.
        targetPath = thisTest.getUrl() + "/branches/A/mu";
        acquireMergeinfoAndAssertEquals(2, 4, 6, 6, targetPath, mergeSrc);
    }

    /**
     * Test merge with automatic source and revision determination
     * (e.g. 'svn merge -g').
     * @throws Throwable
     * @since 1.5
     */
    public void testMergeUsingHistory() throws Throwable
    {
        OneTest thisTest = setupAndPerformMerge();

        // Test that getMergeinfo() returns null.
        assertNull(client.getMergeinfo(new File(thisTest.getWCPath(), "A")
                                       .toString(), Revision.HEAD));

        // Merge and commit some changes (r4).
        appendText(thisTest, "A/mu", "xxx", 4);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     4);

        String branchPath = thisTest.getWCPath() + "/branches/A";
        String modUrl = thisTest.getUrl() + "/A";
        Revision unspec = new Revision(RevisionKind.unspecified);
        client.merge(modUrl, Revision.HEAD,
                     new RevisionRange[] { new RevisionRange(unspec, unspec) },
                     branchPath, true, Depth.infinity, false, false, false);

        // commit the changes so that we can verify merge
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true), 5);
    }

    /**
     * Test reintegrating a branch with trunk
     * (e.g. 'svn merge --reintegrate').
     * @throws Throwable
     * @since 1.5
     */
    public void testMergeReintegrate() throws Throwable
    {
        OneTest thisTest = setupAndPerformMerge();

        // Test that getMergeinfo() returns null.
        assertNull(client.getMergeinfo(new File(thisTest.getWCPath(), "A")
                                       .toString(), Revision.HEAD));

        // Merge and commit some changes to main (r4).
        appendText(thisTest, "A/mu", "xxx", 4);
        assertEquals("wrong revision number from main commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     4);
        // Merge and commit some changes to branch (r5).
        appendText(thisTest, "branches/A/D/G/rho", "yyy", -1);
        assertEquals("wrong revision number from branch commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     5);

        // update the branch WC (to r5) before merge
        client.update(thisTest.getWCPath() + "/branches", Revision.HEAD, true);

        String branchPath = thisTest.getWCPath() + "/branches/A";
        String modUrl = thisTest.getUrl() + "/A";
        Revision unspec = new Revision(RevisionKind.unspecified);
        client.merge(modUrl, Revision.HEAD,
                     new RevisionRange[] { new RevisionRange(unspec, unspec) },
                     branchPath, true, Depth.infinity, false, false, false);

        // commit the changes so that we can verify merge
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A/mu", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true), 6);

        // now we --reintegrate the branch with main
        String branchUrl = thisTest.getUrl() + "/branches/A";
        try
        {
            client.mergeReintegrate(branchUrl, Revision.HEAD,
                                    thisTest.getWCPath() + "/A", false);
            fail("reintegrate merged into a mixed-revision WC");
        }
        catch(ClientException e)
        {
            // update the WC (to r6) and try again
            client.update(thisTest.getWCPath(), Revision.HEAD, true);
            client.mergeReintegrate(branchUrl, Revision.HEAD,
                                    thisTest.getWCPath() + "/A", false);
        }
        // commit the changes so that we can verify merge
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/G/rho", NodeKind.file,
                              CommitItemStateFlags.TextMods);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true), 7);

    }

    /**
     * Test automatic merge conflict resolution.
     * @throws Throwable
     * @since 1.5
     */
    public void testMergeConflictResolution() throws Throwable
    {
        // Add a conflict resolution callback which always chooses the
        // user's version of a conflicted file.
        client.setConflictResolver(new ConflictResolverCallback()
            {
                public ConflictResult resolve(ConflictDescriptor descrip)
                {
                    return new ConflictResult(ConflictResult.chooseTheirsConflict,
                                              null);
                }
            });

        OneTest thisTest = new OneTest();
        String originalContents = thisTest.getWc().getItemContent("A/mu");
        String expectedContents = originalContents + "xxx";

        // Merge and commit a change (r2).
        File mu = appendText(thisTest, "A/mu", "xxx", 2);
        assertEquals("wrong revision number from commit", 2,
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true));

        // Backdate the WC to the previous revision (r1).
        client.update(thisTest.getWCPath(), Revision.getInstance(1), true);

        // Prep for a merge conflict by changing A/mu in a different
        // way.
        mu = appendText(thisTest, "A/mu", "yyy", 1);

        // Merge in the previous changes to A/mu (from r2).
        RevisionRange[] ranges = new RevisionRange[1];
        ranges[0] = new RevisionRange(new Revision.Number(1),
                                      new Revision.Number(2));
        client.merge(thisTest.getUrl(), Revision.HEAD, ranges,
                     thisTest.getWCPath(), false, Depth.infinity, false,
                     false, false);

        assertFileContentsEquals("Unexpected conflict resolution",
                                 expectedContents, mu);
    }

    /**
     * Test merge --record-only
     * @throws Throwable
     * @since 1.5
     */
    public void testRecordOnlyMerge() throws Throwable
    {
        OneTest thisTest = setupAndPerformMerge();

        // Verify that there are now potential merge sources.
        String[] suggestedSrcs =
            client.suggestMergeSources(thisTest.getWCPath() + "/branches/A",
                                       Revision.WORKING);
        assertNotNull(suggestedSrcs);
        assertEquals(1, suggestedSrcs.length);

        // Test that getMergeinfo() returns null.
        assertNull(client.getMergeinfo(new File(thisTest.getWCPath(), "A")
                                       .toString(), Revision.HEAD));

        // Merge and commit some changes (r4).
        appendText(thisTest, "A/mu", "xxx", 4);
        appendText(thisTest, "A/D/G/rho", "yyy", 4);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true),
                     4);

        // --record-only merge changes in A to branches/A
        String branchPath = thisTest.getWCPath() + "/branches/A";
        String modUrl = thisTest.getUrl() + "/A";

        RevisionRange[] ranges = new RevisionRange[1];
        ranges[0] = new RevisionRange(new Revision.Number(2),
                                      new Revision.Number(4));
        client.merge(modUrl, Revision.HEAD, ranges,
                     branchPath, true, Depth.infinity, false, false, true);

        // commit the changes so that we can verify merge
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "branches/A", NodeKind.dir,
                              CommitItemStateFlags.PropMods);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "log msg", true), 5);

        // Test retrieval of mergeinfo from a WC path.
        String targetPath =
            new File(thisTest.getWCPath(), "branches/A").getPath();
        final String mergeSrc = thisTest.getUrl() + "/A";
        acquireMergeinfoAndAssertEquals(2, 4, 0, 0, targetPath, mergeSrc);
    }

    /**
     * Setup a test with a WC.  In the repository, create a
     * "/branches" directory, with a branch of "/A" underneath it.
     * Update the WC to reflect these modifications.
     * @return This test.
     */
    private OneTest setupAndPerformMerge()
        throws Exception
    {
        OneTest thisTest = new OneTest();

        // Verify that there are initially no potential merge sources.
        String[] suggestedSrcs =
            client.suggestMergeSources(thisTest.getWCPath(),
                                       Revision.WORKING);
        assertNotNull(suggestedSrcs);
        assertEquals(0, suggestedSrcs.length);

        // create branches directory in the repository (r2)
        addExpectedCommitItem(null, thisTest.getUrl(), "branches",
                              NodeKind.none, CommitItemStateFlags.Add);
        client.mkdir(new String[]{thisTest.getUrl() + "/branches"}, "log_msg");

        // copy A to branches (r3)
        addExpectedCommitItem(null, thisTest.getUrl(), "branches/A",
                              NodeKind.none, CommitItemStateFlags.Add);
        client.copy(thisTest.getUrl() + "/A", thisTest.getUrl() +
                    "/branches/A", "create A branch", Revision.HEAD);

        // update the WC (to r3) so that it has the branches folder
        client.update(thisTest.getWCPath(), Revision.HEAD, true);

        return thisTest;
    }

    /**
     * Test the {@link SVNClientInterface.diff()} APIs.
     * @since 1.5
     */
    public void testDiff()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(true);
        File diffOutput = new File(super.localTmp, thisTest.testName);
        final String NL = System.getProperty("line.separator");
        final String sepLine =
            "===================================================================" + NL;
        final String underSepLine =
            "___________________________________________________________________" + NL;
        final String expectedDiffBody =
            "@@ -1 +1 @@" + NL +
            "-This is the file 'iota'." + NL +
            "\\ No newline at end of file" + NL +
            "+This is the file 'mu'." + NL +
            "\\ No newline at end of file" + NL;

        final String iotaPath = thisTest.getWCPath().replace('\\', '/') + "/iota";
        final String wcPath = fileToSVNPath(new File(thisTest.getWCPath()),
                false);

        // make edits to iota
        PrintWriter writer = new PrintWriter(new FileOutputStream(iotaPath));
        writer.print("This is the file 'mu'.");
        writer.flush();
        writer.close();

        /*
         * This test does tests with and without svn:eol-style set to native
         * We will first run all of the tests where this does not matter so
         * that they are not run twice.
         */

        // Two-path diff of URLs.
        String expectedDiffOutput = "Index: iota" + NL + sepLine +
            "--- iota\t(.../iota)\t(revision 1)" + NL +
            "+++ iota\t(.../A/mu)\t(revision 1)" + NL +
            expectedDiffBody;
        client.diff(thisTest.getUrl() + "/iota", Revision.HEAD,
                    thisTest.getUrl() + "/A/mu", Revision.HEAD,
                    diffOutput.getPath(), false, true, true, false);
        assertFileContentsEquals("Unexpected diff output in file '" +
                                 diffOutput.getPath() + '\'',
                                 expectedDiffOutput, diffOutput);

        // Test relativeToDir fails with urls. */
        try
        {
            client.diff(thisTest.getUrl() + "/iota", Revision.HEAD,
                        thisTest.getUrl() + "/A/mu", Revision.HEAD,
                        thisTest.getUrl(), diffOutput.getPath(),
                        Depth.infinity, null, true, true, false);

            fail("This test should fail becaus the relativeToDir parameter " +
                 "does not work with URLs");
        }
        catch (Exception ignored)
        {
        }

        /* Testing the expected failure when relativeToDir is not a parent
           path of the target. */
        try
        {
            client.diff(iotaPath, Revision.BASE, iotaPath, Revision.WORKING,
                        "/non/existent/path", diffOutput.getPath(),
                        Depth.infinity, null, true, true, false);

            fail("This test should fail because iotaPath is not a child of " +
                 "the relativeToDir parameter");
        }
        catch (Exception ignored)
        {
        }

        // Test diff with a relative path on a directory with prop
        // changes.
        String aPath = fileToSVNPath(new File(thisTest.getWCPath() + "/A"),
                                     false);

        expectedDiffOutput = "Index: A" + NL + sepLine +
            "--- A\t(revision 1)" + NL +
            "+++ A\t(working copy)" + NL +
            NL + "Property changes on: A" + NL +
            underSepLine +
            "Added: testprop" + NL +
            "## -0,0 +1 ##" + NL +
            "+Test property value." + NL;

        client.propertySet(aPath, "testprop", "Test property value." + NL,
                           false);
        client.diff(aPath, Revision.BASE, aPath, Revision.WORKING, wcPath,
                    diffOutput.getPath(), Depth.infinity, null, true, true,
                    false);
        assertFileContentsEquals("Unexpected diff output in file '" +
                                 diffOutput.getPath() + '\'',
                                 expectedDiffOutput, diffOutput);

        // Test diff where relativeToDir and path are the same.
        expectedDiffOutput = "Index: ." + NL + sepLine +
            "--- .\t(revision 1)" + NL +
            "+++ .\t(working copy)" + NL +
            NL + "Property changes on: ." + NL +
            underSepLine +
            "Added: testprop" + NL +
            "## -0,0 +1 ##" + NL +
            "+Test property value." + NL;

        client.propertySet(aPath, "testprop", "Test property value." + NL,
                           false);
        client.diff(aPath, Revision.BASE, aPath, Revision.WORKING, aPath,
                    diffOutput.getPath(), Depth.infinity, null, true, true,
                    false);
        assertFileContentsEquals("Unexpected diff output in file '" +
                                 diffOutput.getPath() + '\'',
                                 expectedDiffOutput, diffOutput);


        /*
         * The rest of these tests are run twice.  The first time
         * without svn:eol-style set and the second time with the
         * property set to native.  This is tracked by the int named
         * operativeRevision.  It will have a value = 2 after the
         * commit which sets the property
         */

        for (int operativeRevision = 1; operativeRevision < 3; operativeRevision++)
         {
                String revisionPrefix = "While processing operativeRevison=" + operativeRevision + ". ";
                String assertPrefix = revisionPrefix + "Unexpected diff output in file '";

                // Undo previous edits to working copy
                client.revert(wcPath, true);

                if (operativeRevision == 2) {
                    // Set svn:eol-style=native on iota
                    client.propertyCreate(iotaPath, "svn:eol-style", "native", false);
                    String[] paths = new String[] {iotaPath};
                    addExpectedCommitItem(thisTest.getWCPath(),
                            thisTest.getUrl(), "iota",NodeKind.file,
                            CommitItemStateFlags.PropMods);
                    client.commit(paths, "Set svn:eol-style to native", false);
                }

                // make edits to iota and set expected output.
                writer = new PrintWriter(new FileOutputStream(iotaPath));
                writer.print("This is the file 'mu'.");
                writer.flush();
                writer.close();
                expectedDiffOutput = "Index: " + iotaPath + NL + sepLine +
                    "--- " + iotaPath + "\t(revision " + operativeRevision + ")" + NL +
                    "+++ " + iotaPath + "\t(working copy)" + NL +
                    expectedDiffBody;

                try
                {
                    // Two-path diff of WC paths.
                    client.diff(iotaPath, Revision.BASE,
                                iotaPath, Revision.WORKING,
                                diffOutput.getPath(), false, true, true, false);
                    assertFileContentsEquals(assertPrefix +
                                             diffOutput.getPath() + '\'',
                                             expectedDiffOutput, diffOutput);
                    diffOutput.delete();
                }
                catch (ClientException e)
                {
                    fail(revisionPrefix + e.getMessage());
                }

                try
                {
                    // Peg revision diff of a single file.
                    client.diff(thisTest.getUrl() + "/iota", Revision.HEAD,
                                new Revision.Number(operativeRevision), Revision.HEAD,
                                diffOutput.getPath(), false, true, true, false);
                    assertFileContentsEquals(assertPrefix +
                                             diffOutput.getPath() + '\'',
                                             "", diffOutput);

                    diffOutput.delete();
                }
                catch (ClientException e)
                {
                    fail(revisionPrefix + e.getMessage());
                }

               // Test svn diff with a relative path.
                expectedDiffOutput = "Index: iota" + NL + sepLine +
                    "--- iota\t(revision " + operativeRevision + ")" + NL +
                    "+++ iota\t(working copy)" + NL +
                    expectedDiffBody;
                try
                {
                    client.diff(iotaPath, Revision.BASE, iotaPath,
                                Revision.WORKING, wcPath, diffOutput.getPath(),
                                Depth.infinity, null, true, true, false);
                    assertFileContentsEquals(assertPrefix +
                                             diffOutput.getPath() + '\'',
                                             expectedDiffOutput, diffOutput);
                    diffOutput.delete();
                }
                catch (ClientException e)
                {
                    fail(revisionPrefix + e.getMessage());
                }

                try
                {
                    // Test svn diff with a relative path and trailing slash.
                    client.diff(iotaPath, Revision.BASE, iotaPath,
                                Revision.WORKING, wcPath + "/",
                                diffOutput.getPath(), Depth.infinity, null,
                                true, true, false);
                    assertFileContentsEquals(assertPrefix +
                                             diffOutput.getPath() + '\'',
                                             expectedDiffOutput, diffOutput);
                    diffOutput.delete();
                }
                catch (ClientException e)
                {
                    fail(revisionPrefix + e.getMessage());
                }

            }

    }

    private void assertFileContentsEquals(String msg, String expected,
                                          File actual)
        throws IOException
    {
        FileReader reader = new FileReader(actual);
        StringBuffer buf = new StringBuffer();
        int ch;
        while ((ch = reader.read()) != -1)
        {
            buf.append((char) ch);
        }
        assertEquals(msg, expected, buf.toString());
    }

    /**
     * Test the {@link SVNClientInterface.diffSummarize()} API.
     * @since 1.5
     */
    public void testDiffSummarize()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        DiffSummaries summaries = new DiffSummaries();
        // Perform a recursive diff summary, ignoring ancestry.
        client.diffSummarize(thisTest.getUrl(), new Revision.Number(0),
                             thisTest.getUrl(), Revision.HEAD, Depth.infinity,
                             null, false, summaries);
        assertExpectedDiffSummaries(summaries);

        summaries.clear();
        // Perform a recursive diff summary with a peg revision,
        // ignoring ancestry.
        client.diffSummarize(thisTest.getUrl(), Revision.HEAD,
                             new Revision.Number(0), Revision.HEAD,
                             Depth.infinity, null, false, summaries);
        assertExpectedDiffSummaries(summaries);
    }

    private void assertExpectedDiffSummaries(DiffSummaries summaries)
    {
        assertEquals("Wrong number of diff summary descriptors", 20,
                     summaries.size());

        // Rigorously inspect one of our DiffSummary notifications.
        final String BETA_PATH = "A/B/E/beta";
        DiffSummary betaDiff = (DiffSummary) summaries.get(BETA_PATH);
        assertNotNull("No diff summary for " + BETA_PATH, betaDiff);
        assertEquals("Incorrect path for " + BETA_PATH, BETA_PATH,
                     betaDiff.getPath());
        assertTrue("Incorrect diff kind for " + BETA_PATH,
                   DiffSummary.DiffKind.ADDED.equals(betaDiff.getDiffKind()));
        assertEquals("Incorrect props changed notice for " + BETA_PATH,
                     false, betaDiff.propsChanged());
        assertEquals("Incorrect node kind for " + BETA_PATH, 1,
                     betaDiff.getNodeKind());
    }

    /**
     * test the basic SVNClient.isAdminDirectory functionality
     * @throws Throwable
     * @since 1.2
     */
    public void testBasicIsAdminDirectory() throws Throwable
    {
        // build the test setup
        OneTest thisTest = new OneTest();
        Notify2 notify = new Notify2()
        {
            public void onNotify(NotifyInformation info)
            {
                client.isAdminDirectory(".svn");
            }
        };
        client.notification2(notify);
        // update the test
        assertEquals("wrong revision number from update",
                client.update(thisTest.getWCPath(), null, true), 1);
    }

    public void testBasicCancelOperation() throws Throwable
    {
        // build the test setup
        OneTest thisTest = new OneTest();
        Notify2 notify = new Notify2()
        {
            public void onNotify(NotifyInformation info)
            {
                try
                {
                    client.cancelOperation();
                }
                catch (ClientException e)
                {
                    fail(e.getMessage());
                }
            }
        };
        client.notification2(notify);
        // update the test to try to cancel an operation
        try
        {
            client.update(thisTest.getWCPath(), null, true);
            fail("missing exception for canceled operation");
        }
        catch (ClientException e)
        {
            // this is expected
        }
    }

    public void testDataTransferProgressReport() throws Throwable
    {
        // ### FIXME: This isn't working over ra_local, because
        // ### ra_local is not invoking the progress callback.
        if (SVNTests.rootUrl.startsWith("file://"))
            return;

        // build the test setup
        OneTest thisTest = new OneTest();
        ProgressListener listener = new ProgressListener()
        {
            public void onProgress(ProgressEvent event)
            {
                // TODO: Examine the byte counts from "event".
                throw new RuntimeException("Progress reported as expected");
            }
        };
        client.setProgressListener(listener);

        // Perform an update to exercise the progress notification.
        try
        {
            client.update(thisTest.getWCPath(), null, true);
            fail("No progress reported");
        }
        catch (RuntimeException progressReported)
        {
        }
    }

    /**
     * Test the basic tree conflict functionality.
     * @throws Throwable
     */
    public void testTreeConflict() throws Throwable
    {
        // build the test setup. Used for the changes
        OneTest thisTest = new OneTest();
        WC wc = thisTest.getWc();

        // build the backup test setup. That is the one that will be updated
        OneTest tcTest = thisTest.copy(".tree-conflict");


        // Move files from A/B/E to A/B/F.
        String[] srcPaths = { "alpha" };
        for (int i = 0; i < srcPaths.length; i++)
        {
            String fileName = srcPaths[i];
            srcPaths[i] = new File(thisTest.getWorkingCopy(),
                                   "A/B/E/" + fileName).getPath();

            wc.addItem("A/B/F/" + fileName,
                       wc.getItemContent("A/B/E/" + fileName));
            wc.setItemWorkingCopyRevision("A/B/F/" + fileName, 2);
            addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                                  "A/B/F/" + fileName, NodeKind.file,
                                  CommitItemStateFlags.Add |
                                  CommitItemStateFlags.IsCopy);

            wc.removeItem("A/B/E/" + fileName);
            addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                                  "A/B/E/" + fileName, NodeKind.file,
                                  CommitItemStateFlags.Delete);
        }
        client.move(srcPaths,
                    new File(thisTest.getWorkingCopy(), "A/B/F").getPath(),
                    null, false, true, false, null);

        // Commit the changes, and check the state of the WC.
        assertEquals("Unexpected WC revision number after commit",
                     client.commit(new String[] { thisTest.getWCPath() },
                                   "Move files", true), 2);
        thisTest.checkStatus();

        // modify A/B/E/alpha in second working copy
        File alpha = new File(tcTest.getWorkingCopy(), "A/B/E/alpha");
        PrintWriter alphaWriter = new PrintWriter(new FileOutputStream(alpha, true));
        alphaWriter.print("appended alpha text");
        alphaWriter.close();

        // update the tc test
        assertEquals("wrong revision number from update",
                     client.update(tcTest.getWCPath(), null, true),
                     2);

        // set the expected working copy layout for the tc test
        tcTest.getWc().addItem("A/B/F/alpha",
                tcTest.getWc().getItemContent("A/B/E/alpha"));
        tcTest.getWc().setItemWorkingCopyRevision("A/B/F/alpha", 2);
        // we expect the tree conflict to turn the existing item into
        // a scheduled-add with history.
        tcTest.getWc().setItemTextStatus("A/B/E/alpha", StatusKind.added);
        tcTest.getWc().setItemTextStatus("A/B/F/alpha", StatusKind.normal);

        // check the status of the working copy of the tc test
        tcTest.checkStatus();

        // get the Info2 of the tree conflict
        MyInfoCallback callback = new MyInfoCallback();
        client.info2(tcTest.getWCPath() + "/A/B/E/alpha", null,
                null, Depth.unknown, null, callback);
        ConflictDescriptor conflict = callback.getInfo().getConflictDescriptor();

        assertNotNull("Conflict should not be null", conflict);

        assertEquals(conflict.getSrcLeftVersion().getNodeKind(), NodeKind.file);
        assertEquals(conflict.getSrcLeftVersion().getReposURL() + "/" +
                conflict.getSrcLeftVersion().getPathInRepos(), tcTest.getUrl() + "/A/B/E/alpha");
        assertEquals(conflict.getSrcLeftVersion().getPegRevision(), 1L);

        assertEquals(conflict.getSrcRightVersion().getNodeKind(), NodeKind.none);
        assertEquals(conflict.getSrcRightVersion().getReposURL(), tcTest.getUrl());
        assertEquals(conflict.getSrcRightVersion().getPegRevision(), 2L);

    }

    /**
     * Test tolerance of unversioned obstructions when adding paths with
     * {@link org.tigris.subversion.javahl.SVNClient#checkout()},
     * {@link org.tigris.subversion.javahl.SVNClient#update()}, and
     * {@link org.tigris.subversion.javahl.SVNClient#doSwitch()}
     * @throws IOException
     * @throws SubversionException
     */
    /*
      This is currently commented out, because we don't have an XFail method
      for JavaHL.  The resolution is pending the result of issue #3680:
      http://subversion.tigris.org/issues/show_bug.cgi?id=3680
    public void testObstructionTolerance()
            throws SubversionException, IOException
    {
        // build the test setup
        OneTest thisTest = new OneTest();

        File file;
        PrintWriter pw;

        // ----- TEST CHECKOUT -----
        // Use export to make unversioned obstructions for a second
        // WC checkout (deleting export target from previous tests
        // first if it exists).
        String secondWC = thisTest.getWCPath() + ".backup1";
        removeDirOrFile(new File(secondWC));
        client.doExport(thisTest.getUrl(), secondWC, null, false);

        // Make an obstructing file that conflicts with add coming from repos
        file = new File(secondWC, "A/B/lambda");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the conflicting obstructiong file 'lambda'.");
        pw.close();

        // Attempt to checkout backup WC without "--force"...
        try
        {
            // ...should fail
            client.checkout(thisTest.getUrl(), secondWC, null, null,
                            Depth.infinity, false, false);
            fail("obstructed checkout should fail by default");
        }
        catch (ClientException expected)
        {
        }

        // Attempt to checkout backup WC with "--force"
        // so obstructions are tolerated
        client.checkout(thisTest.getUrl(), secondWC, null, null,
                        Depth.infinity, false, true);

        // Check the WC status, the only status should be a text
        // mod to lambda.  All the other obstructing files were identical
        Status[] secondWCStatus = client.status(secondWC, true, false,
                                                false, false, false);
        if (!(secondWCStatus.length == 1 &&
            secondWCStatus[0].getPath().endsWith("A/B/lambda") &&
            secondWCStatus[0].getTextStatus() == StatusKind.modified &&
            secondWCStatus[0].getPropStatus() == StatusKind.none))
        {
            fail("Unexpected WC status after co with " +
                 "unversioned obstructions");
        }

        // Make a third WC to test obstruction tolerance of sw and up.
        OneTest backupTest = thisTest.copy(".backup2");

        // ----- TEST UPDATE -----
        // r2: Add a file A/D/H/nu
        file = new File(thisTest.getWorkingCopy(), "A/D/H/nu");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the file 'nu'.");
        pw.close();
        client.add(file.getAbsolutePath(), false);
        addExpectedCommitItem(thisTest.getWCPath(), thisTest.getUrl(),
                              "A/D/H/nu", NodeKind.file,
                              CommitItemStateFlags.TextMods +
                              CommitItemStateFlags.Add);
        assertEquals("wrong revision number from commit",
                     client.commit(new String[] {thisTest.getWCPath()},
                     "log msg", true), 2);
        thisTest.getWc().addItem("A/D/H/nu", "This is the file 'nu'.");
        Status status = client.singleStatus(thisTest.getWCPath() +
                                            "/A/D/H/nu",
                                            false);

        // Add an unversioned file A/D/H/nu to the backup WC
        file = new File(backupTest.getWorkingCopy(), "A/D/H/nu");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the file 'nu'.");
        pw.close();

        // Attempt to update backup WC without "--force"
        try
        {
            // obstructed update should fail
            client.update(backupTest.getWCPath(), null, true);
            fail("obstructed update should fail by default");
        }
        catch (ClientException expected)
        {
        }

        // Attempt to update backup WC with "--force"
        assertEquals("wrong revision from update",
                     client.update(backupTest.getWCPath(),
                                   null, Depth.infinity, false, false, true),
                     2);

        // ----- TEST SWITCH -----
        // Add an unversioned file A/B/E/nu to the backup WC
        // The file differs from A/D/H/nu
        file = new File(backupTest.getWorkingCopy(), "A/B/E/nu");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is yet another file 'nu'.");
        pw.close();

        // Add an unversioned file A/B/E/chi to the backup WC
        // The file is identical to A/D/H/chi.
        file = new File(backupTest.getWorkingCopy(), "A/B/E/chi");
        pw = new PrintWriter(new FileOutputStream(file));
        pw.print("This is the file 'chi'.");
        pw.close();

        // Attempt to switch A/B/E to A/D/H without "--force"
        try
        {
            // obstructed switch should fail
            client.doSwitch(backupTest.getWCPath() + "/A/B/E",
                            backupTest.getUrl() + "/A/D/H",
                            null, true);
            fail("obstructed switch should fail by default");
        }
        catch (ClientException expected)
        {
        }

        // Complete the switch using "--force" and check the status
        client.doSwitch(backupTest.getWCPath() + "/A/B/E",
                        backupTest.getUrl() + "/A/D/H",
                        Revision.HEAD, Revision.HEAD, Depth.infinity,
                        false, false, true);

        backupTest.getWc().setItemIsSwitched("A/B/E",true);
        backupTest.getWc().removeItem("A/B/E/alpha");
        backupTest.getWc().removeItem("A/B/E/beta");
        backupTest.getWc().addItem("A/B/E/nu",
                                   "This is yet another file 'nu'.");
        backupTest.getWc().setItemTextStatus("A/B/E/nu", Status.Kind.modified);
        backupTest.getWc().addItem("A/D/H/nu",
                                   "This is the file 'nu'.");
        backupTest.getWc().addItem("A/B/E/chi",
                                   backupTest.getWc().getItemContent("A/D/H/chi"));
        backupTest.getWc().addItem("A/B/E/psi",
                                   backupTest.getWc().getItemContent("A/D/H/psi"));
        backupTest.getWc().addItem("A/B/E/omega",
                                   backupTest.getWc().getItemContent("A/D/H/omega"));

        backupTest.checkStatus();
    }*/

    /**
     * Test basic blame functionality.  This test marginally tests blame
     * correctness, mainly just that the blame APIs link correctly.
     * @throws Throwable
     * @since 1.5
     */
    public void testBasicBlame() throws Throwable
    {
        OneTest thisTest = new OneTest();
        // Test the old interface to be sure it still works
        byte[] result = client.blame(thisTest.getWCPath() + "/iota", Revision
                                     .getInstance(1), Revision.getInstance(1));
        assertEquals("     1    jrandom This is the file 'iota'.\n",
                     new String(result));

        // Test the current interface
        BlameCallbackImpl callback = new BlameCallbackImpl();
        client.blame(thisTest.getWCPath() + "/iota", Revision.getInstance(1),
                     Revision.getInstance(1), callback);
        assertEquals(1, callback.numberOfLines());
        BlameCallbackImpl.BlameLine line = callback.getBlameLine(0);
        if (line != null)
        {
            assertEquals(1, line.getRevision());
            assertEquals("jrandom", line.getAuthor());
        }
    }

    /**
     * Test commit of arbitrary revprops.
     * @throws Throwable
     * @since 1.5
     */
    public void testCommitRevprops() throws Throwable
    {

        class RevpropLogCallback implements LogMessageCallback
        {
            Map revprops;

            public void singleMessage(ChangePath[] changedPaths,
                                      long revision,
                                      Map revprops,
                                      boolean hasChildren)
            {
                this.revprops = revprops;
            }

            public Map getRevprops()
            {
                return revprops;
            }
        }

        // build the test setup
        OneTest thisTest = new OneTest();

        // modify file A/mu
        File mu = new File(thisTest.getWorkingCopy(), "A/mu");
        PrintWriter muWriter = new PrintWriter(new FileOutputStream(mu, true));
        muWriter.print("appended mu text");
        muWriter.close();
        thisTest.getWc().setItemWorkingCopyRevision("A/mu", 2);
        thisTest.getWc().setItemContent("A/mu",
                thisTest.getWc().getItemContent("A/mu") + "appended mu text");
        addExpectedCommitItem(thisTest.getWCPath(),
                thisTest.getUrl(), "A/mu",NodeKind.file,
                CommitItemStateFlags.TextMods);

        // commit the changes, with some extra revprops
        Map revprops = new HashMap();
        revprops.put("kfogel", "rockstar");
        revprops.put("cmpilato", "theman");
        assertEquals("wrong revision number from commit",
                     client.commit(new String[]{thisTest.getWCPath()},
                                   "log msg", Depth.infinity, true, true,
                                   null, revprops),
                     2);

        // check the status of the working copy
        thisTest.checkStatus();

        // Fetch our revprops from the server
        RevpropLogCallback callback = new RevpropLogCallback();
        client.logMessages(thisTest.getWCPath(), Revision.getInstance(2),
                           Revision.getInstance(2),
                           Revision.getInstance(2), false, false, false,
                           new String[] {"kfogel", "cmpilato"}, 0,
                           callback);
        Map fetchedProps = callback.getRevprops();

        assertEquals("wrong number of fetched revprops", revprops.size(),
                     fetchedProps.size());
        Set keys = fetchedProps.keySet();
        for (Iterator it = keys.iterator(); it.hasNext(); )
          {
            String key = (String) it.next();
            assertEquals("revprops check", revprops.get(key),
                         fetchedProps.get(key));
          }
    }

    /**
     * @return <code>file</code> converted into a -- possibly
     * <code>canonical</code>-ized -- Subversion-internal path
     * representation.
     */
    private String fileToSVNPath(File file, boolean canonical)
    {
        // JavaHL need paths with '/' separators
        if (canonical)
        {
            try
            {
                return file.getCanonicalPath().replace('\\', '/');
            }
            catch (IOException e)
            {
                return null;
            }
        }
        else
        {
            return file.getPath().replace('\\', '/');
        }
    }


    /**
     * A DiffSummaryReceiver implementation which collects all DiffSummary
     * notifications.
     */
    private static class DiffSummaries extends HashMap
        implements DiffSummaryReceiver
    {
        // Update the serialVersionUID when there is a incompatible
        // change made to this class.
        private static final long serialVersionUID = 1L;

        public void onSummary(DiffSummary descriptor)
        {
            super.put(descriptor.getPath(), descriptor);
        }
    }

    private class MyChangelistCallback extends HashMap
        implements ChangelistCallback
    {
        public void doChangelist(String path, String changelist)
        {
            if (super.containsKey(path))
            {
                // Append the changelist to the existing list
                List changelistList = (List) super.get(path);
                changelistList.add(changelist);
            }
            else
            {
                // Create a new changelist with that list
                List changelistList = new ArrayList();
                changelistList.add(changelist);
                super.put(path, changelistList);
            }
        }

        public List get(String path)
        {
            return (List) super.get(path);
        }
    }

    private class MyInfoCallback implements InfoCallback {
        private Info2 info;

        public void singleInfo(Info2 info) {
            this.info = info;
        }

        public Info2 getInfo() {
            return info;
        }

    }
}
