package org.csploit.android.events;

/**
 * a child has died
 */
public class ChildDied implements Event {
  public final int signal;

  public ChildDied(int signal) {
    this.signal = signal;
  }

  public String toString() {
    return String.format("ChildDied: { signal=%d }", signal);
  }
}
