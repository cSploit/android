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

import java.lang.reflect.Constructor;
import java.util.StringTokenizer;

import junit.framework.TestCase;
import junit.framework.TestResult;
import junit.framework.TestSuite;
import junit.textui.TestRunner;

/**
 * A test runner, and comprehensive test suite definition.
 */
public class RunTests
{
    /**
     * The Subversion JavaHL test suite.
     */
    private static class SVNTestSuite extends TestSuite
    {
        /**
         * Create a conglomerate test suite containing all our test
         * suites, or the fully qualified method names specified by
         * the <code>test.tests</code> system property.
         *
         * @return The complete test suite.
         */
        public static TestSuite suite()
        {
            TestSuite suite = new SVNTestSuite();

            // Determine whether the caller requested that a specific
            // set of test cases be run, and verify that they exist.
            TestCase[] testCases = null;
            String testNames = System.getProperty("test.tests");
            if (testNames != null)
            {
                StringTokenizer tok = new StringTokenizer(testNames, ", ");
                testCases = new TestCase[tok.countTokens()];
                int testCaseIndex = 0;
                while (tok.hasMoreTokens())
                {
                    // ASSUMPTION: Class names are fully-qualified
                    // (with package), and are included with method
                    // names in test names.
                    String methodName = tok.nextToken();
                    int i = methodName.lastIndexOf('.');
                    String className = methodName.substring(0, i);
                    try
                    {
                        Class<?> clazz = Class.forName(className);
                        final Class<?>[] argTypes = new Class[] { String.class };
                        Constructor<?> ctor =
                            clazz.getDeclaredConstructor(argTypes);
                        methodName = methodName.substring(i + 1);
                        String[] args = { methodName };
                        testCases[testCaseIndex++] =
                            (TestCase) ctor.newInstance((Object[]) args);
                    }
                    catch (Exception e)
                    {
                        testCases = null;
                        break;
                    }
                }
            }

            // Add the appropriate set of tests to our test suite.
            if (testCases == null || testCases.length == 0)
            {
                // Add default test suites.
                suite.addTestSuite(SVNReposTests.class);
                suite.addTestSuite(BasicTests.class);
            }
            else
            {
                // Add specific test methods.
                for (int i = 0; i < testCases.length; i++)
                {
                    suite.addTest(testCases[i]);
                }
            }
            return suite;
        }
    }

    /**
     * Main method, will call all tests of all test classes
     * @param args command line arguments
     */
    public static void main(String[] args)
    {
        processArgs(args);
        TestResult testResult = TestRunner.run(SVNTestSuite.suite());
        if (testResult.errorCount() > 0 || testResult.failureCount() > 0)
        {
            System.exit(1);
        }
    }

    /**
     * Retrieve the root directory and the root url from the
     * command-line arguments, and set them on {@link
     * org.apache.subversion.javahl.tests.SVNTests}.
     *
     * @param args The command line arguments to process.
     */
    private static void processArgs(String[] args)
    {
        if (args == null)
            return;

        for (int i = 0; i < args.length; i++)
        {
            String arg = args[i];
            if ("-d".equals(arg))
            {
                if (i + 1 < args.length)
                {
                    SVNTests.rootDirectoryName = args[++i];
                }
            }
            else if ("-u".equals(arg))
            {
                if (i + 1 < args.length)
                {
                    SVNTests.rootUrl = args[++i];
                }
            }
        }
    }
}
