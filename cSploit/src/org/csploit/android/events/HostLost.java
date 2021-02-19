package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an host has been lost
 */
public class HostLost implements Event {
  public final InetAddress ipAddress;

  public HostLost(InetAddress ipAddress) {
    this.ipAddress = ipAddress;
  }
  @Override
  public String toString() {
    return String.format("HostLost: { ipAddress='%s' }", ipAddress.getHostAddress());
  }
}
