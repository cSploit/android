package org.csploit.android.events;

import java.net.InetAddress;

/**
 * sniffed a packet
 */
public class Packet implements Event {
  public final InetAddress src;
  public final InetAddress dst;
  public final int len;

  public Packet(InetAddress src, InetAddress dst, int len) {
    this.src = src;
    this.dst = dst;
    this.len = len;
  }

  @Override
  public String toString() {
    return String.format("Packet: { src='%s', dst='%s', len=%d }", src.getHostAddress(), dst.getHostAddress(), len);
  }
}
