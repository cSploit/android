package org.csploit.android.events;

/**
 * Created by max on 28/10/14.
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
