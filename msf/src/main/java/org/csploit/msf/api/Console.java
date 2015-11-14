package org.csploit.msf.api;

import org.csploit.msf.api.sessions.Interactive;

import java.io.IOException;

/**
 * A console interface to a Msf instance
 */
public interface Console extends Interactive {
  /**
   * get the ID of the console
   * @return the console ID
   */
  int getId();

  /**
   * get the prompt of the console
   * @return a string that should be used as prompt for new commands
   */
  String getPrompt();

  /**
   * get the console busy state
   * @return {@code true} if the console is busy, {@code false} otherwise
   */
  boolean isBusy();

  /**
   * close this console
   */
  void close() throws IOException, MsfException;

  /**
   * write to this console
   * @param data the data to write
   */
  void write(String data) throws IOException, MsfException;

  /**
   * get tab completion for a command
   * @param part  partial command to complete
   * @return an array of possibilities
   */
  String[] tab(String part) throws IOException, MsfException;

  /**
   * kill the session attached to this console, if any
   */
  void killSession() throws IOException, MsfException;

  /**
   * detach from an attached session, if any
   */
  void detachSession() throws IOException, MsfException;
}
