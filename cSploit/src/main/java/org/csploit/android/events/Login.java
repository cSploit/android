package org.csploit.android.events;

import java.net.InetAddress;

/**
 * a login has been found
 */
public class Login implements Event {
  public final int port;
  public final InetAddress address;
  public final String login;
  public final String password;

  public Login(int port, InetAddress address, String login, String password) {
    this.port = port;
    this.address = address;
    this.login = login;
    this.password = password;
  }
}
