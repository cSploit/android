package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an hop has been found
 */
public class Hop implements Event {
  public final int hop;
  public final long usec;
  public final InetAddress node;
  public final String name;

  public Hop(int hop, long usec, InetAddress node, String name) {
    this.hop = hop;
    this.usec = usec;
    this.node = node;
    this.name = name;
  }

  public String toString() {
    return String.format("HopEvent: { hop=%d, usec=%d, node=%s, name='%s' }", hop, usec, node.getHostAddress(), name);
  }
}
