package org.csploit.msf.api.sessions;

import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * Represent an interactive session
 */
public interface Interactive {
  void interact() throws IOException, MsfException;
  void suspend() throws IOException, MsfException;
  void complete() throws IOException, MsfException;
  boolean isInteracting() throws IOException, MsfException;
}
