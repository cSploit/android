/*-
 * See the file LICENSE for redistribution information.
 *
 * Copyright (c) 2002, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package com.sleepycat.util.test;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import java.io.IOException;
import java.io.PrintWriter;
import java.io.StringWriter;

import org.junit.Test;

import com.sleepycat.util.ExceptionUnwrapper;
import com.sleepycat.util.IOExceptionWrapper;
import com.sleepycat.util.RuntimeExceptionWrapper;

/**
 * @author Mark Hayes
 */
public class ExceptionWrapperTest extends TestBase {

    @Test
    public void testIOWrapper() {
        try {
            throw new IOExceptionWrapper(new RuntimeException("msg"));
        } catch (IOException e) {
            Exception ee = ExceptionUnwrapper.unwrap(e);
            assertTrue(ee instanceof RuntimeException);
            assertEquals("msg", ee.getMessage());

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof RuntimeException);
            assertEquals("msg", t.getMessage());
        }
    }

    @Test
    public void testRuntimeWrapper() {
        try {
            throw new RuntimeExceptionWrapper(new IOException("msg"));
        } catch (RuntimeException e) {
            Exception ee = ExceptionUnwrapper.unwrap(e);
            assertTrue(ee instanceof IOException);
            assertEquals("msg", ee.getMessage());

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof IOException);
            assertEquals("msg", t.getMessage());
        }
    }

    @Test
    public void testErrorWrapper() {
        try {
            throw new RuntimeExceptionWrapper(new Error("msg"));
        } catch (RuntimeException e) {
            try {
                ExceptionUnwrapper.unwrap(e);
                fail();
            } catch (Error ee) {
                assertTrue(ee instanceof Error);
                assertEquals("msg", ee.getMessage());
            }

            Throwable t = ExceptionUnwrapper.unwrapAny(e);
            assertTrue(t instanceof Error);
            assertEquals("msg", t.getMessage());
        }
    }

    /**
     * Generates a stack trace for a nested exception and checks the output
     * for the nested exception.
     */
    @Test
    public void testStackTrace() {

        /* Nested stack traces are not avilable in Java 1.3. */
        String version = System.getProperty("java.version");
        if (version.startsWith("1.3.")) {
            return;
        }

        Exception ex = new Exception("some exception");
        String causedBy = "Caused by: java.lang.Exception: some exception";

        try {
            throw new RuntimeExceptionWrapper(ex);
        } catch (RuntimeException e) {
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            String s = sw.toString();
            assertTrue(s.indexOf(causedBy) != -1);
        }

        try {
            throw new IOExceptionWrapper(ex);
        } catch (IOException e) {
            StringWriter sw = new StringWriter();
            e.printStackTrace(new PrintWriter(sw));
            String s = sw.toString();
            assertTrue(s.indexOf(causedBy) != -1);
        }
    }
}
