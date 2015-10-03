package org.csploit.msf.session;

/**
 * This interface is to be implemented by a session that is capable of
 * providing multiple command shell interfaces simultaneously.  Inherently,
 * MultiCommandShell classes must also provide a mechanism by which they can
 * implement the SingleCommandShell interface.
 */
public interface MultiCommandShell extends SingleCommandShell {
  int open();
  String read(int length, int id);
  int write(String data, int id);
  void close(int id);
}
