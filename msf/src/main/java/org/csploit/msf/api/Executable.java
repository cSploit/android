package org.csploit.msf.api;

import java.io.IOException;

/**
 * Something that can be executed
 */
public interface Executable {
  void execute() throws IOException, MsfException;
}
