package org.csploit.android.events;

/**
 * os info found
 */
public class Os implements Event {
  public final short accuracy;
  public final String os;
  public final String type;

  public Os(short accuracy, String os, String type) {
    this.accuracy = accuracy;
    this.os = os;
    this.type = type;
  }

  public String toString() {
    return String.format("Os: { accuracy=%d, os='%s', type='%s' }", accuracy, os, type);
  }
}
