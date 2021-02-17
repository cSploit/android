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
import java.io.IOException;

import org.tigris.subversion.javahl.Revision;
import org.tigris.subversion.javahl.SubversionException;

/**
 * This class is used for testing the SVNAdmin class
 *
 * More methodes for testing are still needed
 */
public class SVNAdminTests extends SVNTests
{
    public SVNAdminTests()
    {
    }

    public SVNAdminTests(String name)
    {
        super(name);
    }

    /**
     * Test the basic SVNAdmin.create functionality
     * @throws SubversionException
     */
    public void testCreate()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        assertTrue("repository exists", thisTest.getRepository().exists());
    }

    public void testSetRevProp()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        final String MSG = "Initial repository creation";
        admin.setRevProp(thisTest.getRepositoryPath(), Revision.getInstance(0),
                         "svn:log", MSG, false, false);
        PropertyData[] pdata = client.revProperties(
                                      makeReposUrl(thisTest.getRepository()),
                                      Revision.getInstance(0));
        assertNotNull("expect non null rev props");
        String logMessage = null;
        for (int i = 0; i < pdata.length; i++)
        {
            if ("svn:log".equals(pdata[i].getName()))
            {
                logMessage = pdata[i].getValue();
                break;
            }
        }
        assertEquals("expect rev prop change to take effect", MSG, logMessage);
    }
    public void testLoadRepo()
        throws SubversionException, IOException
    {
        /* Make sure SVNAdmin.load() works, with a repo dump file known
         * to provoke bug 2979
         */
        // makes repos with nothing in it
        OneTest thisTest = new OneTest(false,false);
        // verify zero revisions in new repos
        String repoUrl = makeReposUrl(thisTest.getRepository());
        final Info2[] infoHolder = new Info2[1];
        InfoCallback mycallback = new InfoCallback()
        {
            public void singleInfo(Info2 info)
            {
                infoHolder[0] = info;
            }
        };
        client.info2(repoUrl, Revision.HEAD, Revision.HEAD,
                Depth.immediates, null, mycallback);
        assertNotNull("expect info callback", infoHolder[0]);
        assertEquals("expect zero revisions in new repository",
                0L, infoHolder[0].getLastChangedRev());

        // locate dump file in test environment
        String testSrcdir = System.getProperty("test.srcdir",
                "subversion/bindings/javahl");
        File dump = new File(testSrcdir, "tests/data/issue2979.dump");
        InputInterface input = new FileInputer(dump);
        OutputInterface loadLog = new IgnoreOutputer();
        admin.load(thisTest.getRepositoryPath(),
                   input, loadLog, true, true, null);
        // should have two revs after the load
        infoHolder[0] = null;
        client.info2(repoUrl, Revision.HEAD, Revision.HEAD,
                     Depth.immediates, null, mycallback);
        assertEquals("expect two revisions after load()",
                     2L, infoHolder[0].getLastChangedRev());
        // verify that the repos is faithful rep. of the dump file,
        // e.g., correct author
        assertEquals("expect 'svn4ant' as author of r2",
                     "svn4ant", infoHolder[0].getLastChangedAuthor());
    }
}
