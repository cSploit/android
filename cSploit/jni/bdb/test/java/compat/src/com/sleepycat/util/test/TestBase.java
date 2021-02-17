/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.test;

import java.io.File;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.rules.TestRule;
import org.junit.rules.TestWatcher;
import org.junit.runner.Description;

/**
 * The base class for all JE unit tests. 
 */
public abstract class TestBase {

    /*
     * Need to provide a customized name suffix for those tests which are 
     * Parameterized.
     *
     * This is because we need to provide a unique directory name for those 
     * failed tests. Parameterized class would reuse test cases, so class name 
     * plus the test method is not unique. User should set the customName
     * in the constructor of a Parameterized test. 
     */
    protected String customName;
    
    /**
     * The rule we use to control every test case, the core of this rule is 
     * copy the testing environment, files, sub directories to another place
     * for future investigation, if any of test failed. But we do have a limit
     * to control how many times we copy because of disk space. So once the 
     * failure counter exceed limit, it won't copy the environment any more.
     */
    @Rule
    public TestRule watchman = new TestWatcher() {

        /* Copy Environments when the test failed. */
        @Override
        protected void failed(Throwable t, Description desc) {
            try {
                copyEnvironments(makeFileName(desc));
            } catch (Exception e) {
                throw new RuntimeException
                    ("can't copy env dir after failure", e);
            }
        }
        
        @Override
        protected void succeeded(Description desc){
        }};
    
    @Before
    public void setUp() 
        throws Exception {
        
        SharedTestUtils.cleanUpTestDir(SharedTestUtils.getTestDir());
    }
    
    @After
    public void tearDown() throws Exception {
        //Provision for future use
    }
    /**
     *  Copy the testing directory to other place. 
     */
    private void copyEnvironments(String path) throws Exception{
        
        File failureDir = SharedTestUtils.getFailureCopyDir();
        if (failureDir.list().length < SharedTestUtils.getCopyLimit())
            SharedTestUtils.copyDir
                (SharedTestUtils.getTestDir(), new File(failureDir, path));
    }
    
    /**
     * Get failure copy directory name. 
     */
    private String makeFileName(Description desc) {
        String name = desc.getClassName() + "-" + desc.getMethodName();
        if (customName != null) {
            name = name + "-" + customName;
        }
        return name;
    }
}
