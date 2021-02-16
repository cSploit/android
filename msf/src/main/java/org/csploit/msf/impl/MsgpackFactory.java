package org.csploit.msf.impl;

import org.csploit.msf.api.*;
import org.csploit.msf.api.Framework;

import java.io.IOException;

/**
 * A factory that create MSF stuff using a Messagepack connection
 */
public class MsgpackFactory {
  private final MsgpackClient client;
  private final MsgpackProxy.FrameworkFactory frameworkFactory;

  /**
   * create a {@link MsgpackFactory} using the specified parameters
   * @param host to connect to
   * @param username username
   * @param password password
   * @param port port to connect to
   * @param ssl shall we connect using SSL ?
   */
  public MsgpackFactory(String host, String username, String password, int port, boolean ssl)
          throws IOException, MsfException {
    this.client = new MsgpackClient(host, username, password, port, ssl);
    frameworkFactory = new MsgpackProxy.FrameworkFactory(client);
  }

  /**
   * create a {@link Framework}
   * @return a new {@link Framework}
   */
  public Framework createFramework() {
    return frameworkFactory.createFramework();
  }

  /**
   * create a {@link ConsoleManager}
   * @return a new {@link ConsoleManager}
   */
  public ConsoleManager createConsoleManager() {
    return new MsgpackConsoleManager(client);
  }
}
