package org.csploit.android.events;

/**
 * a line has been printed on child stderr
 */
public class StderrNewline implements Event {
  public final String line;

  public StderrNewline(String line) {
    this.line = line;
  }

  @Override
  public String toString() {
    return "StderrNewline: { line='" + this.line + "'  }";
  }
}
