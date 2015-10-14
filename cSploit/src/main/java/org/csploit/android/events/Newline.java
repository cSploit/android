package org.csploit.android.events;

/**
 * got a newline from a child.
 */
public class Newline implements Event {
  public final String line;

  public Newline(String line) {
    this.line = line;
  }

  public String toString() {
    return String.format("NewlineEvent: { line='%s' }", line);
  }
}
