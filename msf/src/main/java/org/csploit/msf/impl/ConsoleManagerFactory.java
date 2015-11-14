package org.csploit.msf.impl;

import org.csploit.msf.api.ConsoleManager;
import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * Create {@link ConsoleManager}s
 */
public final class ConsoleManagerFactory {
  public static ConsoleManager createMsgpackConsoleManager(String host, String username, String password, int port, boolean ssl) throws IOException, MsfException {
    MsgpackClient client = new MsgpackClient(host, username, password, port, ssl);
    return new MsgpackConsoleManager(client);
  }
}
