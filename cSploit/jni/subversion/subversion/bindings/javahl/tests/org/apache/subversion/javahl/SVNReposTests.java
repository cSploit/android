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
import java.io.OutputStream;
import java.io.InputStream;
import java.io.IOException;
import java.net.URI;
import java.util.Map;

/**
 * This class is used for testing the SVNAdmin class
 *
 * More methodes for testing are still needed
 */
public class SVNReposTests extends SVNTests
{
    public SVNReposTests()
    {
    }

    public SVNReposTests(String name)
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
        admin.setRevProp(thisTest.getRepository(), Revision.getInstance(0),
                         "svn:log", MSG, false, false);
        Map<String, byte[]> pdata = client.revProperties(
                           makeReposUrl(thisTest.getRepository()).toString(),
                           Revision.getInstance(0));
        assertNotNull("expect non null rev props");
        String logMessage = new String(pdata.get("svn:log"));
        assertEquals("expect rev prop change to take effect", MSG, logMessage);
    }

    /* This test only tests the call down to the C++ layer. */
    public void testVerify()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        admin.verify(thisTest.getRepository(), Revision.getInstance(0),
                     Revision.HEAD, null);
    }

    /* This test only tests the call down to the C++ layer. */
    public void testUpgrade()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        admin.upgrade(thisTest.getRepository(), null);
    }

    /* This test only tests the call down to the C++ layer. */
    public void testPack()
        throws SubversionException, IOException
    {
        OneTest thisTest = new OneTest(false);
        admin.pack(thisTest.getRepository(), null);
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
        URI repoUrl = makeReposUrl(thisTest.getRepository());
        final Info[] infoHolder = new Info[1];
        InfoCallback mycallback = new InfoCallback()
        {
            public void singleInfo(Info info)
            {
                infoHolder[0] = info;
            }
        };
        client.info2(repoUrl.toString(), Revision.HEAD, Revision.HEAD,
                Depth.immediates, null, mycallback);
        assertNotNull("expect info callback", infoHolder[0]);
        assertEquals("expect zero revisions in new repository",
                0L, infoHolder[0].getLastChangedRev());

        // locate dump file in test environment
        String testSrcdir = System.getProperty("test.srcdir",
                "subversion/bindings/javahl");
        File dump = new File(testSrcdir, "tests/data/issue2979.dump");
        InputStream input = new FileInputStream(dump);
        admin.load(thisTest.getRepository(),
                   input, true, true, false, false, null, null);
        // should have two revs after the load
        infoHolder[0] = null;
        client.info2(repoUrl.toString(), Revision.HEAD, Revision.HEAD,
                     Depth.immediates, null, mycallback);
        assertEquals("expect two revisions after load()",
                     2L, infoHolder[0].getLastChangedRev());
        // verify that the repos is faithful rep. of the dump file,
        // e.g., correct author
        assertEquals("expect 'svn4ant' as author of r2",
                     "svn4ant", infoHolder[0].getLastChangedAuthor());
    }
}
