package org.csploit.msf.session;

/**
 * This interface is to be implemented by a session that is only capable of
 * providing an interface to a single command shell.
 */
public interface SingleCommandShell {
  void init();
  String read(int length);
  int write(String data);
  void close();
}
