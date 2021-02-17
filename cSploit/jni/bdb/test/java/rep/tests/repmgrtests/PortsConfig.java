/*-
 * See the file LICENSE for redistribution information.
 * 
 * Copyright (c) 2010, 2014 Oracle and/or its affiliates.  All rights reserved.
 *
 */

package repmgrtests;

import java.io.IOException;

public class PortsConfig {
    private int mgrPort;
    private int[] realPorts;
    private int[] spoofPorts;

    public PortsConfig(int nsites) throws IOException {
        
        // For each site we need two ports, a real port and a spoofed
        // port.
        // 
        int nPorts = 2 * nsites;
        realPorts = new int[nsites];
        spoofPorts = new int[nsites];

        int[] ports = Util.findAvailablePorts(nPorts);

        mgrPort = ports[0];

        int j = 0;
        for (int i=0; i<nsites; i++) {
            realPorts[i] = ports[j++];
            spoofPorts[i] = ports[j++];
        }
    }

    public int getRealPort(int n) { return realPorts[n]; }
    public int getSpoofPort(int n) { return spoofPorts[n]; }

    public String getFiddlerConfig() {
        StringBuilder sb = new StringBuilder();
        sb.append("[");
        sb.append("{" + spoofPorts[0] + "," + realPorts[0] + "}");
        for (int i=1; i<realPorts.length; i++) {
            sb.append(",");
            sb.append("{" + spoofPorts[i] + "," + realPorts[i] + "}");
        }
        sb.append("]");
        return sb.toString();
    }
}
