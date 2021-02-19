package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an host has been found
 */
public class Host implements Event {
  public final InetAddress ipAddress;
  public final byte[] ethAddress;
  public final String name;

  public Host(InetAddress ipAddress, byte[] ethAddress, String name) {
    this.ipAddress = ipAddress;
    this.ethAddress = ethAddress;
    this.name = name;
  }
  @Override
  public String toString() {
    return String.format("Host: { ethAddress=[%02X:%02X:%02X:%02X:%02X:%02X], ipAddress='%s', name='%s' }",
            ethAddress[0], ethAddress[1], ethAddress[2], ethAddress[3], ethAddress[4], ethAddress[5],
            ipAddress.getHostAddress(), name);
  }
}
