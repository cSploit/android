/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import org.junit.Before;

import java.io.File;
import java.net.URL;
import java.net.URLClassLoader;
import java.util.Properties;

public abstract class AbstractUpgTest {
    protected Object oldGroup_o, currentGroup_o;
    private String currentScript, oldScript, oldVersion;

    public AbstractUpgTest(String ov, String os, String cs) {
        oldVersion = ov;
        oldScript = os;
        currentScript = cs;
    }

    @Before public void create() throws Exception {
        Properties p = new Properties();
        p.load(getClass().getResourceAsStream("classpaths.properties"));

        URL[] urls = new URL[2];
        urls[0] = makeUrl(p.getProperty("db." + oldVersion));
        urls[1] = makeUrl(p.getProperty("test." + oldVersion));
        ClassLoader cl = new URLClassLoader(urls);

        oldGroup_o = cl.loadClass(oldScript).newInstance();

        urls[0] = makeUrl(p.getProperty("db"));
        urls[1] = makeUrl(p.getProperty("test"));
        cl = new URLClassLoader(urls);
        currentGroup_o = cl.loadClass(currentScript).newInstance();
    }

    private URL makeUrl(String fileName) throws Exception {
        return new File(fileName).toURI().toURL();
    }
}
