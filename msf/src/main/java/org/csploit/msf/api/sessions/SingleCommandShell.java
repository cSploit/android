package org.csploit.msf.api.sessions;

import org.csploit.msf.api.Session;

/**
 * This interface is to be implemented by a session that is only capable of
 * providing an interface to a single command shell.
 */
public interface SingleCommandShell extends Session {
  void init();
  String read(int length);
  int write(String data);
  void close();
}
