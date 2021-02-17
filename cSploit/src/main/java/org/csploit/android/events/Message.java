package org.csploit.android.events;

/**
 * a child generated a message
 */
public class Message implements Event {
  public enum Severity {
    VERBOSE,
    INFO,
    WARNING,
    ERROR,
    FATAL
  }

  public final Severity severity;
  public final String message;

  public Message(String severity, String message) throws IllegalArgumentException {
    // we use String to avoid caching tens of jmethodID ( one for every enum value )
    this.severity = Severity.valueOf(severity);
    this.message = message;
  }

  @Override
  public String toString() {
    return String.format("Message: { Severity='%s', message='%s' }", severity.name(), message);
  }
}
