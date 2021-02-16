package org.csploit.msf.api.sessions;

import org.csploit.msf.api.MsfException;
import org.csploit.msf.api.Session;

import java.io.IOException;

/**
 * This interface is to be implemented by a session that is only capable of
 * providing an interface to a single command shell.
 */
public interface SingleCommandShell extends Session {
  void init();
  String read() throws IOException, MsfException;
  int write(String data) throws IOException, MsfException;
}
