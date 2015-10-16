package org.csploit.msf.api.sessions;

/**
 * Executes multiple commands and optionally allows for reading/writing I/O
 * to the new processes.
 */
public interface MultiCommandExecution extends SingleCommandExecution {
  int exec(String cmd, String[] args, boolean hidden);
  String read(int length, int id);
  int write(String data, int id);
  void close(int id);
}
