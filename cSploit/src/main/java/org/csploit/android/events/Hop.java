package org.csploit.android.events;

import java.net.InetAddress;

/**
 * an hop has been found
 */
<<<<<<< HEAD:cSploit/src/org/csploit/android/events/Hop.java
public class Hop {
  public final short hop;
  public final int usec;
=======
public class Hop implements Event {
  public final int hop;
  public final long usec;
>>>>>>> develop:cSploit/src/main/java/org/csploit/android/events/Hop.java
  public final InetAddress node;
  public final String name;

<<<<<<< HEAD:cSploit/src/org/csploit/android/events/Hop.java
  public Hop(short hop, int usec, InetAddress node, String guai) {
=======
  public Hop(int hop, long usec, InetAddress node, String name) {
>>>>>>> develop:cSploit/src/main/java/org/csploit/android/events/Hop.java
    this.hop = hop;
    this.usec = usec;
    this.node = node;
    this.name = name;
  }

  public String toString() {
    return String.format("HopEvent: { hop=%d, usec=%d, node=%s, name='%s' }", hop, usec, node.getHostAddress(), name);
  }
}
