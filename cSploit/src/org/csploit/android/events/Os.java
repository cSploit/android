package org.csploit.android.events;

/**
 * os info found
 */
public class Os implements Event {
  public final byte accuracy;
  public final String os;
  public final String type;

  public Os(byte accuracy, String os, String type) {
    this.accuracy = accuracy;
    this.os = os;
    this.type = type;
  }
  @Override
  public String toString() {
    return String.format("Os: { accuracy=%d, os='%s', type='%s' }", accuracy, os, type);
  }
}
