package org.csploit.msf.impl;

import org.csploit.msf.api.Console;
import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * Provide access to hidden Console stuff
 */
interface InternalConsole extends Console {
  /**
   * read from this console
   * @return the data read from the console
   */
  String read() throws IOException, MsfException;
}
