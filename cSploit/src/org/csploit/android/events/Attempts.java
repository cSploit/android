package org.csploit.android.events;

/**
 * hydra attempts status
 */
public class Attempts implements Event {
  public final long sent;
  public final long left;

  public Attempts(long sent, long left) {
    this.sent = sent;
    this.left = left;
  }
  public String toString() {
    return String.format("AttemptsEvent: { sent=%d, left=%d }", sent, left);
  }
}
