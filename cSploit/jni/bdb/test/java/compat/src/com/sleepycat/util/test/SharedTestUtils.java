/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.test;

import java.io.BufferedInputStream;
import java.io.BufferedOutputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.nio.channels.FileChannel;

import junit.framework.TestCase;

import com.sleepycat.db.DatabaseConfig;

/**
 * Test utility methods shared by JE and DB core tests.  Collections and
 * persist package test are used in both JE and DB core.
 */
public class SharedTestUtils {

    /* Common system properties for running tests */
    public static String DEST_DIR = "testdestdir";
    public static String TEST_ENV_DIR = "testenvdirroot";
    public static String FAILURE_DIR = "failurecopydir";
    public static String DEFAULT_DEST_DIR = "build/test/classes";
    public static String DEFAULT_TEST_DIR_ROOT = "build/test/envdata";
    public static String DEFAULT_FAIL_DIR = "build/test/failures";
    public static String NO_SYNC = "txnnosync";
    public static String LONG_TEST =  "longtest";
    public static String COPY_LIMIT = "copylimit";

    public static final DatabaseConfig DBCONFIG_CREATE = new DatabaseConfig();
    static {
        DBCONFIG_CREATE.setAllowCreate(true);
    }
    
    /**
     * The environment store compiled class files and generated environment by
     * test that is distinctive with test environment.
     */
    public static File getDestDir() {
        String dir = System.getProperty(DEST_DIR, DEFAULT_DEST_DIR);
        File file = new File(dir);
        if (!file.isDirectory())
            file.mkdir();
        
        return file;
    }

    /**
     * If not define system property "testenvdirroot", use build/test/envdata
     * as test environment root directory. 
     */
    public static File getTestDir() {
        String dir = System.getProperty(TEST_ENV_DIR, DEFAULT_TEST_DIR_ROOT);
        File file = new File(dir);
        if (!file.isDirectory())
            file.mkdir();
        
        return file;
    }
    
    /**
     * Allow to set up self defined directory store failure copy. 
     */
    public static File getFailureCopyDir() {
        String dir = System.getProperty(FAILURE_DIR, DEFAULT_FAIL_DIR);
        File file = new File(dir);
        if (!file.isDirectory())
            file.mkdir();
        
        return file;
    }

    /**
     * If test failed, copy its environment to other location. The default
     * limit is 10, but our test support the value via system property.  
     */
    public static int getCopyLimit() {
        String limit = System.getProperty(COPY_LIMIT, "10");

        return Integer.parseInt(limit);
    }

    /**
     * @return true if long running tests are enabled via setting the system
     * property longtest=true.
     */
    public static boolean runLongTests() {
        String longTestProp =  System.getProperty(LONG_TEST);
        if ((longTestProp != null)  &&
            longTestProp.equalsIgnoreCase("true")) {
            return true;
        } else {
            return false;
        }
    }

    public static void printTestName(String name) {
        // don't want verbose printing for now
        // System.out.println(name);
    }

    public static File getExistingDir(String name) {
        File dir = new File(getTestDir(), name);
        if (!dir.exists() || !dir.isDirectory()) {
            throw new IllegalStateException(
                    "Not an existing directory: " + dir);
        }
        return dir;
    }

    public static File getNewDir() {
        return getNewDir("test-dir");
    }

    public static void emptyDir(File dir) {
        if (dir.isDirectory()) {
            String[] files = dir.list();
            if (files != null) {
                for (int i = 0; i < files.length; i += 1) {
                    new File(dir, files[i]).delete();
                }
            }
        } else {
            dir.delete();
            dir.mkdirs();
        }
    }
    
    /**
     * @return A sub-directory of current test destination directory.
     */
    public static File getNewDir(String name) {
        File dir = new File(getTestDir(), name);
        emptyDir(dir);
        return dir;
    }
    
    public static File getNewFile() {
        return getNewFile("test-file");
    }

    public static File getNewFile(String name) {
        return getNewFile(getTestDir(), name);
    }

    public static File getNewFile(File dir, String name) {
        File file = new File(dir, name);
        file.delete();
        return file;
    }

    public static boolean copyResource(Class cls, String fileName, File toDir)
        throws IOException {

        InputStream in = cls.getResourceAsStream("testdata/" + fileName);
        if (in == null) {
            return false;
        }
        in = new BufferedInputStream(in);
        File file = new File(toDir, fileName);
        OutputStream out = new FileOutputStream(file);
        out = new BufferedOutputStream(out);
        int c;
        while ((c = in.read()) >= 0) out.write(c);
        in.close();
        out.close();
        return true;
    }

    public static String qualifiedTestName(TestCase test) {

        String s = test.getClass().getName();
        int i = s.lastIndexOf('.');
        if (i >= 0) {
            s = s.substring(i + 1);
        }
        return s + '.' + test.getName();
    }

    /**
     * Copies all files in fromDir to toDir.  Does not copy subdirectories.
     */
    public static void copyFiles(File fromDir, File toDir)
        throws IOException {

        String[] names = fromDir.list();
        if (names != null) {
            for (int i = 0; i < names.length; i += 1) {
                File fromFile = new File(fromDir, names[i]);
                if (fromFile.isDirectory()) {
                    continue;
                }
                File toFile = new File(toDir, names[i]);
                int len = (int) fromFile.length();
                byte[] data = new byte[len];
                FileInputStream fis = null;
                FileOutputStream fos = null;
                try {
                    fis = new FileInputStream(fromFile);
                    fos = new FileOutputStream(toFile);
                    fis.read(data);
                    fos.write(data);
                } finally {
                    if (fis != null) {
                        fis.close();
                    }
                    if (fos != null) {
                        fos.close();
                    }
                }
            }
        }
    }
    
    /**
     * Copy everything in test destination directory to another place for 
     * future evaluation when test failed. 
     */
    public static void copyDir(File fromDir, File toDir) 
        throws Exception {
        
        if (fromDir == null || toDir == null)
            throw new NullPointerException("File location error");
        
        if (!fromDir.isDirectory()) 
            throw new IllegalStateException("Test destination should be dir");
        
        if (!toDir.exists() && !toDir.mkdirs())
            throw new IllegalStateException("Unable to create copy dest dir");
        
        String[] list = fromDir.list();
        if (list != null) {
            
            for (String fileName : list) {
                File file = new File(fromDir, fileName);
                if (file.isDirectory())
                    copyDir(file, new File(toDir, fileName));
                else 
                    copyFile(file, new File(toDir, fileName));
            }
        }
    }
    
    /**
     * Copy file to specified location. 
     */
    private static void copyFile(File from, File to) 
        throws Exception {
        
        if (to.isDirectory())
            to = new File(to, from.getName());
        
        FileInputStream fis = null;
        FileOutputStream fos = null;
        FileChannel fcin = null;
        FileChannel fcout = null;
    
        try {
            fis = new FileInputStream(from);
            fos = new FileOutputStream(to);
            fcin = fis.getChannel();
            fcout = fos.getChannel();
            fcin.transferTo(0, fcin.size(), fcout);
        } finally {
            if (fis != null) {
                fis.close();
            }
            if (fos != null) {
                fos.close();
            }
        }
    }
    
    /**
     * Clean up everything in JE test destination directory including all kind
     * files and sub directories generated by last test except je.properties. 
     */
    public static void cleanUpTestDir(File dir) {
        if (!dir.isDirectory() || !dir.exists())
            throw new IllegalStateException(
                "Not an existing directory: " + dir); 
        File[] files = dir.listFiles();
        if (files == null)
            return;
        
        for (File file : files) {
            if ("je.properties".equals(file.getName()))
                continue;
            
            if (file.isDirectory()) { 
                cleanUpTestDir(file);

                if (file.list().length == 0 && !file.delete())
                    throw new IllegalStateException(
                            "Unable to delete" + file);
            } else {
                if(!file.delete())
                    throw new IllegalStateException(
                        "Unable to delete " + file);
            }
        }
    }
}
