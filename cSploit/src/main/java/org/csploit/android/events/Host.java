package org.csploit.android.events;

import java.net.InetAddress;
import java.util.Arrays;

/**
 * an host has been found
 */
public class Host implements Event {
  public final byte[] ethAddress;
  public final InetAddress ipAddress;
  public final String name;

  public Host(byte[] ethAddress, InetAddress ipAddress, String name) {
    this.ethAddress = Arrays.copyOf(ethAddress, ethAddress.length);
    this.ipAddress = ipAddress;
    this.name = name;
  }

  public String toString() {
    return String.format("Host: { ethAddress=[%02X:%02X:%02X:%02X:%02X:%02X], ipAddress='%s', name='%s' }",
            ethAddress[0], ethAddress[1], ethAddress[2], ethAddress[3], ethAddress[4], ethAddress[5],
            ipAddress.getHostAddress(), name);
  }
}
