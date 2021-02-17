/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import java.io.File;
import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;

public class Config {
    private File dir;
    private int port0, port1;

    public Config(File dir) throws IOException {
        this.dir = dir;

        int n = 2;
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
        port0 = ports[0];
        port1 = ports[1];
    }

    public int getMyPort(int siteId) {
        switch (siteId) {
        case 0:
            return port0;
        case 1:
            return port1;
        default:
            throw new RuntimeException("bad site ID: " + siteId);
        }
    }

    public int getOtherPort(int siteId) {
        switch (siteId) {
        case 0:
            return port1;
        case 1:
            return port0;
        default:
            throw new RuntimeException("bad site ID: " + siteId);
        }
    }

    public File getBaseDir() { return dir; }
}
