package org.csploit.android.events;

/**
 * a child is ended
 */
public class ChildEnd implements Event {
  public final int exit_status;

  public ChildEnd(int exit_status) {
    this.exit_status = exit_status;
  }

  public String toString() {
    return String.format("ChildEnd: { exit_status=%d }", exit_status);
  }
}
