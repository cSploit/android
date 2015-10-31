package org.csploit.msf.api;

/**
 * An error occurred in the framework
 */
public class MsfException extends Exception {
  public MsfException(String message) {
    super(message);
  }

  public MsfException(String message, Throwable cause) {
    super(message, cause);
  }
}
