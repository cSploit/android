package org.csploit.android.events;

import android.provider.CalendarContract;

import java.net.InetAddress;

/**
 * an hop has been found
 */
public class Hop implements Event {
  public final int hop;
  public final long usec;
  public final InetAddress node;

  public Hop(int hop, long usec, InetAddress node) {
    this.hop = hop;
    this.usec = usec;
    this.node = node;
  }

  public String toString() {
    return String.format("HopEvent: { hop=%d, usec=%d, node=%s }", hop, usec, node.getHostAddress());
  }
}
