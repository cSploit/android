package org.csploit.msf.impl;

import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * This class represents the base class for a logical payload.  The framework
 * automatically generates payload combinations at runtime which are all
 * extended for this Payload as a base class.
 */
class Payload extends Module implements org.csploit.msf.api.Payload {
  public Payload(String refname) {
    super(refname);
  }

  @Override
  public String getType() {
    return "payload";
  }

  @Override
  public byte[] generate() throws IOException, MsfException {
    throw new UnsupportedOperationException("not implemented yet");
  }
}
