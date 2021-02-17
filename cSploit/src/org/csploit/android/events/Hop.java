package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an hop has been found
 */
public class Hop {
  public final short hop;
  public final int usec;
  public final InetAddress node;

  public Hop(short hop, int usec, InetAddress node, String guai) {
    this.hop = hop;
    this.usec = usec;
    this.node = node;
  }

  public String toString() {
    return String.format("HopEvent: { hop=%d, usec=%d, node=%s }", hop, usec, node.getHostAddress());
  }
}
