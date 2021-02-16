package org.csploit.msf.api.exceptions;

import org.csploit.msf.api.MsfException;

/**
 * A requested resource cannot be found
 *
 * for instance a non-existing console ID, session ID or module name.
 */
public class ResourceNotFoundException extends MsfException {

  public ResourceNotFoundException(String message) {
    super(message);
  }

  public ResourceNotFoundException(String message, Throwable cause) {
    super(message, cause);
  }
}
