package org.csploit.msf.api;

import java.io.IOException;

/**
 * This class represents the base class for a logical payload.  The framework
 * automatically generates payload combinations at runtime which are all
 * extended for this Payload as a base class.
 */
public interface Payload extends Module {
  byte[] generate() throws IOException, MsfException;
}
