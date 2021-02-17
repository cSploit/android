/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import java.io.File;
import java.io.BufferedOutputStream;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.io.FileOutputStream;

import java.net.ServerSocket;
import java.net.Socket;

/**
 * Miscellaneous utilities used by repmgr tests.
 */
public class Util {
    public static Process startFiddler(PortsConfig p, String testName, int mgrPort)
        throws Exception
    {
        ProcessBuilder pb =
            new ProcessBuilder("erl", "-noshell",
                               "-s", "fiddler1", "startn",
                               Integer.toString(mgrPort),
                               p.getFiddlerConfig());
        pb.directory(new File("fiddler")).redirectErrorStream(true);
        Process fiddler = pb.start();

        String fileName = testName + ".fiddler.out";
        final BufferedOutputStream bos = new BufferedOutputStream(new FileOutputStream(fileName));
        final BufferedInputStream bis = new BufferedInputStream(fiddler.getInputStream());

        // Spawn a thread to capture the fiddler's output and save it
        // into a file.
        // 
        Runnable copier = new Runnable() {
                public void run() {
                    try {
                        byte[] buf = new byte[1000];
                        int len;

                        for (;;) {
                            len = bis.read(buf, 0, buf.length);
                            if (len == -1) { break; }
                            bos.write(buf, 0, len);
                        }
                        bos.close();
                    } catch (IOException x) {
                        x.printStackTrace();
                    }
                }
            };
        Thread t = new Thread(copier);
        t.start();

        // wait til fiddler is ready, which we'll infer when it is
        // listening on its mgmt port (Hmm, is that really late
        // enough?  Well, it's certainly better'n nuttin'.)
        // 
        Socket s = null;
        long deadline = System.currentTimeMillis() + 5000; // allow 5 seconds, max
        do {
            try {
                s = new Socket("localhost", mgrPort);
            } catch (IOException x) {
                if (System.currentTimeMillis() > deadline) { throw x; }
                Thread.sleep(200);
            }
        } while (s == null);
        s.close();

        return fiddler;
    }

    public static File mkdir(String dname) {
        File d = new File("TESTDIR");
        d.mkdir();
        File f = new File(d, dname);
        rm_rf(f);
        f.mkdir();
        return f;
    }
    
    public static void rm_rf(File f) {
        if (f.isDirectory())
            for (File f2 : f.listFiles())
                rm_rf(f2);
        f.delete();
    }
    
    public static int[] findAvailablePorts(int n) throws IOException {
        int ports[] = new int[n];
        ServerSocket[] sockets = new ServerSocket[n];
        for (int i=0; i<n; i++) {
            ServerSocket s = new ServerSocket(0);
            s.setReuseAddress(true);
            ports[i] = s.getLocalPort();
            sockets[i] = s;
        }
        for (int i=0; i<n; i++)
            sockets[i].close();
        return ports;
    }
}
