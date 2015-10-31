package org.csploit.msf.impl;

import org.csploit.msf.api.Framework;
import org.csploit.msf.api.MsfException;

import java.io.IOException;

/**
 * A factory to create Metasploit Frameworks
 */
public final class FrameworkFactory {
  public static Framework newLocalFramework() {
    return new org.csploit.msf.impl.Framework();
  }

  public static Framework newMsgpackFramework(String host, String username, String password, int port, boolean ssl)
          throws IOException, MsfException {
    MsgpackClient client = new MsgpackClient(host, username, password, port, ssl);
    return MsgpackProxy.Factory.newFramework(client);
  }
}
